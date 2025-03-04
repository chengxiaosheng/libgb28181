#include "RequestProxyImpl.h"

#include "RequestCatalogImpl.h"
#include "RequestConfigDownloadImpl.h"
#include "RequestListImpl.h"
#include "subordinate_platform_impl.h"
#include "super_platform_impl.h"

#include <Network/Session.h>
#include <gb28181/message/broadcast_message.h>
#include <gb28181/message/catalog_message.h>
#include <gb28181/message/config_download_messsage.h>
#include <gb28181/message/cruise_track_message.h>
#include <gb28181/message/device_config_message.h>
#include <gb28181/message/device_control_message.h>
#include <gb28181/message/device_info_message.h>
#include <gb28181/message/device_status_message.h>
#include <gb28181/message/home_position_message.h>
#include <gb28181/message/message_base.h>
#include <gb28181/message/preset_message.h>
#include <gb28181/message/ptz_position_message.h>
#include <gb28181/message/record_info_message.h>
#include <gb28181/message/sd_card_status_message.h>
#include <gb28181/sip_event.h>
#include <gb28181/type_define_ext.h>
#include <inner/sip_common.h>
#include <inner/sip_session.h>
#include <sip-message.h>
#include <sip-uac.h>

using namespace gb28181;

std::ostream &gb28181::operator<<(std::ostream &os, const RequestProxyImpl &proxy) {
    os << "(";
    os << proxy.platform_->sip_account().platform_id << ":" << proxy.request_->root() << ":"
       << proxy.request_->command();

    if (proxy.request_sn_)
        os << ":" << proxy.request_sn_;
    os << ") ";
    return os;
}
std::ostream &gb28181::operator<<(std::ostream &os, const RequestProxy &proxy) {
    os << dynamic_cast<const RequestProxyImpl &>(proxy);
    return os;
}
std::ostream &gb28181::operator<<(std::ostream &os, const std::shared_ptr<RequestProxy> &proxy) {
    if (auto ptr = std::dynamic_pointer_cast<RequestProxyImpl>(proxy)) {
        os << *ptr;
    }
    return os;
}

std::shared_ptr<RequestProxy> newRequestProxy_(
    const std::shared_ptr<PlatformHelper> &platform, const std::shared_ptr<MessageBase> &request, int sn = 0) {
    RequestProxy::RequestType type { RequestProxy::RequestType::OneResponse };
    auto command = request->command();
    auto root = request->root();
    if (root == MessageRootType::Query) {
        // 可能存在多个回复的消息类型掩码
        constexpr uint64_t multi_response_cmds = (1ULL << static_cast<int>(MessageCmdType::Catalog))
            | (1ULL << static_cast<int>(MessageCmdType::ConfigDownload))
            | (1ULL << static_cast<int>(MessageCmdType::RecordInfo))
            | (1ULL << static_cast<int>(MessageCmdType::PresetQuery))
            | (1ULL << static_cast<int>(MessageCmdType::CruiseTrackQuery))
            | (1ULL << static_cast<int>(MessageCmdType::CruiseTrackListQuery));
        if (multi_response_cmds & (1ULL << static_cast<int>(command))) {
            type = RequestProxy::MultipleResponses;
        }
    } else if (root == MessageRootType::Control) {
        if (command == MessageCmdType::DeviceControl) {
            // 源设备向目标设备发送摄像机云台控制、远程启动、强制关键帧、拉框放大、拉框缩小、PTZ精准控制、
            // 存储卡格式化、 目标跟踪命令后，目标设备不发送应答命令，命令流程见9.3.2.1;
            // 不需要回复的控制消息掩码
            constexpr uint64_t no_response_cmds = (1ULL << static_cast<int>(DeviceControlType::PTZCmd))
                | (1ULL << static_cast<int>(DeviceControlType::TeleBoot))
                | (1ULL << static_cast<int>(DeviceControlType::IFrameCmd))
                | (1ULL << static_cast<int>(DeviceControlType::DragZoomIn))
                | (1ULL << static_cast<int>(DeviceControlType::DragZoomOut))
                | (1ULL << static_cast<int>(DeviceControlType::PtzPreciseCtrl))
                | (1ULL << static_cast<int>(DeviceControlType::FormatSDCard))
                | (1ULL << static_cast<int>(DeviceControlType::TargetTrack));
            if (auto control_request = std::dynamic_pointer_cast<DeviceControlRequestMessage>(request)) {
                if (no_response_cmds & (1ULL << static_cast<int>(control_request->control_type()))) {
                    type = RequestProxy::NoResponse;
                }
            }
        }
    } else if (root == MessageRootType::Notify && command != MessageCmdType::Alarm) {
        type = RequestProxy::NoResponse;
    } else if (root == MessageRootType::Notify || root == MessageRootType::Response) {
        type = RequestProxy::NoResponse;
    } else {
        return nullptr;
    }
    if (type == RequestProxy::MultipleResponses) {
        if (command == MessageCmdType::ConfigDownload) {
            return std::make_shared<RequestConfigDownloadImpl>(platform, request, sn);
        }
        return std::make_shared<RequestListImpl>(platform, request, sn);
    }
    return std::make_shared<RequestProxyImpl>(platform, request, type, sn);
}

std::shared_ptr<RequestProxy> RequestProxy::newRequestProxy(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request, int sn) {
    return newRequestProxy_(std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform), request, sn);
}
std::shared_ptr<RequestProxy> RequestProxy::newRequestProxy(
    const std::shared_ptr<SuperPlatform> &platform, const std::shared_ptr<MessageBase> &request, int sn) {
    return newRequestProxy_(std::dynamic_pointer_cast<SuperPlatformImpl>(platform), request, sn);
}

RequestProxyImpl::RequestProxyImpl(
    const std::shared_ptr<PlatformHelper> &platform, const std::shared_ptr<MessageBase> &request, RequestType type,
    int32_t sn)
    : RequestProxy()
    , platform_(platform)
    , request_(request)
    , request_sn_(sn == 0 ? platform_->get_new_sn() : sn)
    , request_type_(type) {
    DebugL << *this;
}

RequestProxyImpl::~RequestProxyImpl() {
    DebugL << *this;
}

int RequestProxyImpl::on_recv_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (param == nullptr)
        return 0;
    auto request = static_cast<RequestProxyImpl *>(param);
    if (reply) {
        request->platform_->update_remote_via(get_via_rport(reply));
        // if (auto via = sip_vias_get(&reply->vias, 0)) {
        //     std::string host = cstrvalid(&via->received) ? std::string(via->received.p, via->received.n) : "";
        //     request->platform_->update_local_via(host, static_cast<uint16_t>(via->rport));
        // }
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
    reply_time_ = toolkit::getCurrentMicrosecond(true);
    reply_code_ = code;
    uac_transaction_.reset();
    if (SIP_IS_SIP_SUCCESS(code)) {
        status_ = request_type_ == NoResponse ? Succeeded : Replied;
    } else {
        error_ = code == 408 || !sip_message ? "no reply" : "status code = " + std::to_string(code);
        status_ = code == 408 || !sip_message ? Timeout : Failed;
    }
    on_reply_l();
    if (reply_callback_) {
        reply_callback_(shared_from_this(), code);
        reply_callback_ = {};
    }
    if (status_ != Replied) {
        on_completed();
        return;
    }
    // 超时定时器
    timer_ = std::make_shared<toolkit::Timer>(
        5,
        [this_ptr = shared_from_this()]() {
            if (this_ptr->status_ == Replied) {
                this_ptr->status_ = Timeout;
                this_ptr->error_ = "the wait for a response has timed out";
                this_ptr->on_completed();
            }
            return false;
        },
        nullptr);
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
        on_completed_l();
        platform_->remove_request_proxy(request_sn_);
    }
}

void RequestProxyImpl::send(std::function<void(std::shared_ptr<RequestProxy>)> rcb) {

    if (!request_) {
        error_ = "the request message is empty";
        status_ = Failed;
        return on_completed();
    }
    struct sip_agent_t *sip_agent = platform_->get_sip_agent();
    std::string from = platform_->get_from_uri(), to = platform_->get_to_uri();
    CharEncodingType encoding_type = platform_->get_encoding();

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
    platform_->add_request_proxy(request_sn_, shared_from_this());
    send_time_ = toolkit::getCurrentMicrosecond(true);
    platform_->uac_send(uac_transaction_, request_->str(), [weak_this = weak_from_this()](bool ret, std::string err) {
        if (auto this_ptr = weak_this.lock()) {
            if (!ret) {
                this_ptr->status_ = Failed;
                this_ptr->error_ = std::move(err);
                return this_ptr->on_completed();
            }
        }
    });
}

int RequestProxyImpl::on_response(const std::shared_ptr<MessageBase> &response) {
    bool completed = request_type_ == OneResponse;
    if (completed) {
        response_end_time_ = toolkit::getCurrentMicrosecond(true);
        status_ = Succeeded;
    }
    int code = 200;
    if (response_callback_) {
        code = response_callback_(shared_from_this(), response, completed);
    }
    if (completed) {
        toolkit::EventPollerPool::Instance().getPoller()->async(
            [this_ptr = shared_from_this()]() { this_ptr->on_completed(); }, false);
    }
    return code ? code : 400;
}
int RequestProxyImpl::on_response(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    std::shared_ptr<MessageBase> response;
    switch (message.command()) {
        case MessageCmdType::DeviceControl:
            response = std::make_shared<DeviceControlResponseMessage>(std::move(message));
            break;
        case MessageCmdType::DeviceConfig:
            response = std::make_shared<DeviceConfigResponseMessage>(std::move(message));
            break;
        case MessageCmdType::DeviceStatus:
            response = std::make_shared<DeviceStatusMessageResponse>(std::move(message));
            break;
        case MessageCmdType::Catalog: response = std::make_shared<CatalogResponseMessage>(std::move(message)); break;
        case MessageCmdType::DeviceInfo:
            response = std::make_shared<DeviceInfoMessageResponse>(std::move(message));
            break;
        case MessageCmdType::RecordInfo:
            response = std::make_shared<RecordInfoResponseMessage>(std::move(message));
            break;
        case MessageCmdType::ConfigDownload:
            response = std::make_shared<ConfigDownloadResponseMessage>(std::move(message));
            break;
        case MessageCmdType::PresetQuery: response = std::make_shared<PresetResponseMessage>(std::move(message)); break;
        case MessageCmdType::HomePositionQuery:
            response = std::make_shared<HomePositionResponseMessage>(std::move(message));
            break;
        case MessageCmdType::CruiseTrackListQuery:
            response = std::make_shared<CruiseTrackListResponseMessage>(std::move(message));
            break;
        case MessageCmdType::CruiseTrackQuery:
            response = std::make_shared<CruiseTrackResponseMessage>(std::move(message));
            break;
        case MessageCmdType::PTZPosition:
            response = std::make_shared<PTZPositionResponseMessage>(std::move(message));
            break;
        case MessageCmdType::SDCardStatus:
            response = std::make_shared<SdCardResponseMessage>(std::move(message));
            break;
        case MessageCmdType::Broadcast: response = std::make_shared<BroadcastNotifyResponse>(std::move(message)); break;
        default: break;
    }
    if (!response) {
        WarnL << "unknown message command " << message.command();
        return 400;
    }
    timer_.reset(); // 收到回复，立即取消超时定时器
    responses_.push_back(response);
    if (response_begin_time_ == 0) {
        response_begin_time_ = toolkit::getCurrentMicrosecond(true);
    }
    if (!response->load_from_xml()) {
        status_ = Failed;
        error_ = "load_from_xml failed, error = " + response->get_error();
        WarnL << error_;
        // 这里放入异步， 避免 on_completed 阻塞
        toolkit::EventPollerPool::Instance().getPoller()->async(
            [this_ptr = shared_from_this()]() { this_ptr->on_completed(); }, false);
        return 400;
    }
    return on_response(response);
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
