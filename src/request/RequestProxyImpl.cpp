#include "RequestProxyImpl.h"

#include "RequestCatalogImpl.h"
#include "RequestConfigDownloadImpl.h"
#include "subordinate_platform_impl.h"
#include "super_platform_impl.h"

#include <Network/Session.h>
#include <gb28181/message/device_info_message.h>
#include <gb28181/message/device_status_message.h>
#include <gb28181/message/message_base.h>
#include <gb28181/sip_common.h>
#include <gb28181/type_define_ext.h>
#include <inner/sip_common.h>
#include <inner/sip_session.h>
#include <sip-message.h>
#include <sip-uac.h>

using namespace gb28181;

std::ostream &gb28181::operator<<(std::ostream &os, const RequestProxyImpl &proxy) {
    os << "(";
    std::visit(
        [&](auto platform) {
            os << platform->account().platform_id << ":" << proxy.request_->root() << ":" << proxy.request_->command();
        },
        proxy.platform_);
    if (proxy.request_sn_)
        os << ":" << proxy.request_sn_;
    os << ") ";
    return os;
}

RequestProxyImpl::RequestProxyImpl(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request, RequestType type)
    : RequestProxy()
    , platform_(std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform))
    , request_(request)
    , request_type_(type) {
    DebugL << *this;
}
RequestProxyImpl::~RequestProxyImpl() {
    DebugL << *this;
}

std::shared_ptr<RequestProxy> RequestProxy::newRequestProxy(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request) {
    RequestType type { RequestType::invalid };
    auto command = request->command();
    auto root = request->root();
    if (root == MessageRootType::Query) {
        if (command == MessageCmdType::Catalog || command == MessageCmdType::ConfigDownload)
            type = MultipleResponses;
        else
            type = OneResponse;
    } else if (root == MessageRootType::Control) {
        // 源设备向目标设备发送摄像机云台控制、远程启动、强制关键帧、拉框放大、拉框缩小、PTZ精准控制、 存储卡格式化、
        // 目标跟踪命令后，目标设备不发送应答命令，命令流程见9.3.2.1;

    } else if (root == MessageRootType::Notify || root == MessageRootType::Response) {
        type = NoResponse;
    } else {
        return nullptr;
    }
    if (type == MultipleResponses) {
        if (command == MessageCmdType::Catalog) {
            return std::make_shared<RequestCatalogImpl>(platform, request);
        }
        return std::make_shared<RequestConfigDownloadImpl>(platform, request);
    }
    return std::make_shared<RequestProxyImpl>(platform, request, type);
}
int RequestProxyImpl::on_recv_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (param == nullptr)
        return 0;
    auto request = static_cast<RequestProxyImpl *>(param);
    if (auto via = sip_vias_get(&reply->vias, 0)) {
        std::string host = cstrvalid(&via->received) ? std::string(via->received.p, via->received.n) : "";
        std::visit([&](auto & platform) {
            platform->update_local_via(host, static_cast<uint16_t>(via->rport));
        }, request->platform_);
    }
    std::shared_ptr<sip_message_t> sip_message(const_cast<sip_message_t *>(reply), [](sip_message_t *reply) {
        if (reply) {
            sip_message_destroy(reply);
        }
    });
    request->on_reply(sip_message, code);
    return 0;
}
void RequestProxyImpl::on_reply(std::shared_ptr<sip_message_t> sip_message, int code) {
    reply_time_ = toolkit::getCurrentMicrosecond();
    reply_code_ = code;
    uac_transaction_.reset();
    if (SIP_IS_SIP_SUCCESS(code)) {
        status_ = request_type_ == NoResponse ? Succeeded : Replied;
    } else {
        error_ = code == 0 || !sip_message ? "no reply" : "status code = " + std::to_string(code);
        status_ = code == 0 || !sip_message ? Timeout : Failed;
    }
    if (reply_callback_) {
        reply_callback_(shared_from_this(), code);
        reply_callback_ = {};
    }
    if (status_ != Replied) {
        on_completed();
    }
}
void RequestProxyImpl::on_completed() {
    if (!result_flag_.test_and_set()) {
        // 1. 移除请求map
        // 2. 发送结果回调
        // 3. 清理数据回调与结果回调
        uac_transaction_.reset();
        auto this_ptr = shared_from_this();
        if (rcb_) {
            rcb_(this_ptr);
        }
        reply_callback_ = nullptr;
        response_callback_ = nullptr;
        rcb_ = nullptr;
        std::visit([&](auto &&platform) { platform->remove_request_proxy(request_sn_); }, platform_);
    }
}

void RequestProxyImpl::send(std::function<void(std::shared_ptr<RequestProxy>)> rcb) {

    if (!request_) {
        error_ = "the request message is empty";
        status_ = Failed;
        return on_completed();
    }
    struct sip_agent_t *sip_agent = nullptr;
    std::string from, to;
    CharEncodingType encoding_type { CharEncodingType::gb2312 };
    std::visit(
        [&](auto &platform) {
            sip_agent = platform->get_sip_agent();
            from = platform->get_from_uri();
            to = platform->get_to_uri();
            request_sn_ = platform->get_new_sn();
            encoding_type = platform->account().encoding;
        },
        platform_);

    uac_transaction_ = std::shared_ptr<sip_uac_transaction_t>(
        sip_uac_message(sip_agent, from.c_str(), to.c_str(), RequestProxyImpl::on_recv_reply, this),
        [](sip_uac_transaction_t *t) {
            if (t)
                sip_uac_transaction_release(t);
        });
    rcb_ = std::move(rcb);
    request_->sn(request_sn_);
    request_->encoding(encoding_type);
    set_message_content_type(uac_transaction_.get(), SipContentType_XML);
    set_message_header(uac_transaction_.get());
    if (!request_->parse_to_xml()) {
        error_ = "make request payload failed, error = " + request_->get_error();
        status_ = Failed;
        return on_completed();
    }
    std::visit(
        [&](auto &platform) {
            platform->add_request_proxy(request_sn_, shared_from_this());
            platform->get_session(
                [this_ptr = shared_from_this()](const toolkit::SockException &e, std::shared_ptr<SipSession> &session) {
                    if (e) {
                        this_ptr->status_ = Failed;
                        this_ptr->error_ = "failed to obtain or create a connection";
                        return this_ptr->on_completed();
                    }
                    auto payload = this_ptr->request_->str();
                    this_ptr->send_time_ = toolkit::getCurrentMicrosecond();
                    if (0
                        != sip_uac_send(
                            this_ptr->uac_transaction_.get(), payload.data(), payload.size(),
                            SipSession::get_transport(), session.get())) {
                        this_ptr->status_ = Failed;
                        this_ptr->error_ = "failed to send request, call sip_uac_send failed";
                        return this_ptr->on_completed();
                    }
                });
        },
        platform_);
}

int RequestProxyImpl::on_response(const std::shared_ptr<MessageBase> &response) {
    bool completed = request_type_ == OneResponse;
    if (completed) {
        response_end_time_ = toolkit::getCurrentMicrosecond();
        status_ = Succeeded;
    }
    int code = 200;
    if (response_callback_) {
        code = response_callback_(shared_from_this(), response, completed);
    }
    if (completed) {
        toolkit::EventPollerPool::Instance().getPoller()->async([this_ptr = shared_from_this()]() {
            this_ptr->on_completed();
        }, false);
    }
    return code ? code : 400;
}
int RequestProxyImpl::on_response(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    std::shared_ptr<MessageBase> response;
    switch (message.command()) {
        case MessageCmdType::DeviceControl: break;
        case MessageCmdType::DeviceConfig: break;
        case MessageCmdType::DeviceStatus:
            response = std::make_shared<DeviceStatusMessageResponse>(std::move(message));
            break;
        case MessageCmdType::Catalog: break;
        case MessageCmdType::DeviceInfo:
            response = std::make_shared<DeviceInfoMessageResponse>(std::move(message));
            break;
        case MessageCmdType::RecordInfo: break;
        case MessageCmdType::Alarm: break;
        case MessageCmdType::ConfigDownload: break;
        case MessageCmdType::PresetQuery: break;
        case MessageCmdType::MobilePosition: break;
        case MessageCmdType::HomePositionQuery: break;
        case MessageCmdType::CruiseTrackListQuery: break;
        case MessageCmdType::CruiseTrackQuery: break;
        case MessageCmdType::PTZPosition: break;
        case MessageCmdType::SDCardStatus: break;
        case MessageCmdType::MediaStatus: break;
        case MessageCmdType::Broadcast: break;
        case MessageCmdType::UploadSnapShotFinished: break;
        case MessageCmdType::VideoUploadNotify: break;
        case MessageCmdType::DeviceUpgradeResult: break;
        default: break;
    }
    if (!response) {
        WarnL << "unknown message command " << message.command();
        return 400;
    }
    if (!response->load_from_xml()) {
        WarnL << "load_from_xml failed, error = " << response->get_error();
        return 400;
    }
    responses_.push_back(response);
    if (response_begin_time_ == 0) {
        response_begin_time_ = toolkit::getCurrentMicrosecond();
    }
    return on_response(std::move(response));
}

/**********************************************************************************************************
文件名称:   RequestProxyImpl.cpp
创建时间:   25-2-11 下午12:44
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午12:44

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午12:44       描述:   创建文件

**********************************************************************************************************/
