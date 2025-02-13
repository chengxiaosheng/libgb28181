#include "uas_message_handler.h"

#include "tinyxml2.h"

#include <Util/NoticeCenter.h>
#include <Util/util.h>
#include <gb28181/message/device_info_message.h>
#include <gb28181/message/device_status_message.h>
#include <gb28181/message/keepalive_message.h>
#include <gb28181/message/message_base.h>
#include <gb28181/sip_common.h>
#include <gb28181/type_define_ext.h>
#include <inner/sip_common.h>
#include <inner/sip_server.h>
#include <inner/sip_session.h>
#include <iostream>
#include <sip-message.h>
#include <sip-transport.h>
#include <sip-uac.h>
#include <sip-uas.h>
#include <subordinate_platform_impl.h>
#include <super_platform_impl.h>
#include <variant>

using namespace gb28181;
using namespace toolkit;

static std::unordered_map<std::string, std::shared_ptr<SubordinatePlatform>>
    registering_platforms_; // 正在注册中的平台?
static std::mutex registering_platforms_mtx_;

namespace gb28181 {
class SubordinatePlatformImpl;
int on_uas_register(
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, const std::string &user, const std::string &location, int expires) {

    auto sip_server = session->getSipServer();
    if (!sip_server) {
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
    }
    if (auto platform
        = std::dynamic_pointer_cast<SubordinatePlatformImpl>(sip_server->get_subordinate_platform(user))) {
        // 将session 添加到平台
        platform->add_session(session);
        if (expires == 0) {
            platform->set_status(PlatformStatusType::offline, "logout");
            return sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
        }
        auto &account = platform->account();
        if (account.auth_type == SipAuthType::none || verify_authorization(req.get(), user, account.password)) {
            platform->set_status(PlatformStatusType::online, {});
            set_message_date(transaction.get());
            set_message_expires(transaction.get(), expires);
            return sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
        }
        set_message_www_authenticate(transaction.get(), sip_server->get_account().domain);
        return sip_uas_reply(transaction.get(), 401, nullptr, 0, session.get());
    }
    if (sip_server->allow_auto_register()) {
        if (expires == 0) {
            return sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
        }
        const auto &server_account = sip_server->get_account();
        if (server_account.auth_type == SipAuthType::none
            || verify_authorization(req.get(), user, server_account.password)) {
            // 构建一个新的平台
            subordinate_account account;
            account.name = user;
            account.platform_id = user;
            account.domain = user.substr(0, 10);
            account.host = session->get_peer_ip();
            account.port = session->get_peer_port();
            account.password = server_account.password;
            account.auth_type = server_account.auth_type;
            account.transport_type = session->is_udp() ? TransportType::udp : TransportType::tcp;
            auto account_ptr = std::make_shared<subordinate_account>(std::move(account));
            sip_server->new_subordinate_account(
                account_ptr, [session, transaction, expires](std::shared_ptr<SubordinatePlatformImpl> platform) {
                    if (platform) {
                        platform->add_session(session);
                        platform->set_status(PlatformStatusType::online, {});
                        set_message_date(transaction.get());
                        set_message_expires(transaction.get(), expires);
                        return sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
                    }
                    return sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
                });
            return sip_ok;
        }
        set_message_www_authenticate(transaction.get(), server_account.domain);
        return sip_uas_reply(transaction.get(), 401, nullptr, 0, session.get());
    }
    return sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
}

int on_uas_message(
    const std::shared_ptr<SipSession> &session, const std ::shared_ptr<sip_uas_transaction_t> &transaction,
    const std ::shared_ptr<sip_message_t> &req, void *dialog_ptr) {
    // 验证消息负载
    if (req->payload == nullptr) {
        WarnL << "SIP message payload is null or empty";
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    // 验证from 标记中的用户
    auto &&platform_id = get_platform_id(req.get());
    if (platform_id.empty()) {
        ErrorL << "SIP message platform id is empty";
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }

    auto sip_server = session->getSipServer();
    if (!sip_server) {
        WarnL << "sip_server has been destroyed";
        return sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
    }

    auto xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
    if (xml_ptr->Parse((const char *)req->payload, req->size) != tinyxml2::XML_SUCCESS) {
        WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")" << xml_ptr->ErrorStr()
              << ", xml = " << std::string_view((const char *)req->payload, req->size);
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }

    MessageBase message(xml_ptr);
    if (!message.load_from_xml()) {
        ErrorL << "SIP message load failed: " << message.get_error()
               << "xml = " << std::string_view((const char *)req->payload, req->size);
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    if (message.root() == MessageRootType::invalid) {
        WarnL << "SIP message root is invalid";
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    if (message.command() == MessageCmdType::invalid) {
        WarnL << "SIP message command is invalid";
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }

    CharEncodingType message_encoding{message.encoding()};
    // [fold] region get platform
    bool from_super_platform = false;
    // 查询请求 与 设备控制 一定来自上级平台
    if (message.root() == MessageRootType::Query || message.root() == MessageRootType::Control) {
        from_super_platform = true;
    } else if (message.root() == MessageRootType::Notify && message.command() == MessageCmdType::Broadcast) {
        // 语音广播 一般由上级发起
        from_super_platform = true;
    }
    std::variant<std::shared_ptr<SubordinatePlatformImpl>, std::shared_ptr<SuperPlatformImpl>> platform_;
    if (from_super_platform) {
        platform_ = std::dynamic_pointer_cast<SuperPlatformImpl>(sip_server->get_super_platform(platform_id));
    } else {
        platform_ = std::dynamic_pointer_cast<SubordinatePlatformImpl>(sip_server->get_subordinate_platform(platform_id));
    }
    bool has_platform = false;
    if (std::holds_alternative<std::shared_ptr<SuperPlatformImpl>>(platform_)) {
        if (auto platform = std::get<std::shared_ptr<SuperPlatformImpl>>(platform_)) {
            has_platform = true;
            if (message_encoding == CharEncodingType::invalid) {
                message_encoding = platform->account().encoding;
            } else if (platform->account().encoding == CharEncodingType::invalid){
                // 反向设置是否合理？
                platform->set_encoding(message_encoding);
            }
        }
    } else if (std::holds_alternative<std::shared_ptr<SubordinatePlatformImpl>>(platform_)) {
        if (auto platform = std::get<std::shared_ptr<SubordinatePlatformImpl>>(platform_)) {
            has_platform = true;
            if (message_encoding == CharEncodingType::invalid) {
                message_encoding = platform->account().encoding;
            } else if (platform->account().encoding == CharEncodingType::invalid) {
                // 反向设置是否合理？
                platform->set_encoding(message_encoding);
            }
        }
    }
    if (!has_platform) {
        WarnL << "platform was not found";
        if (message.command() == MessageCmdType::Keepalive) {
            return 0;
        }
        return sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
    }
    // [fold] endregion get platform
    std::string convert_xml_str;
    switch (message.encoding()) {
        case CharEncodingType::gbk: convert_xml_str = gbk_to_utf8((const char *)req->payload); break;
        case CharEncodingType::gb2312: convert_xml_str = gb2312_to_utf8((const char *)req->payload); break;
        default: break;
    }
    // 基于指定编码重新解析xml
    if (!convert_xml_str.empty()) {
        xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
        if (xml_ptr->Parse(convert_xml_str.c_str(), convert_xml_str.size()) != tinyxml2::XML_SUCCESS) {
            WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")"
                  << xml_ptr->ErrorStr() << ", xml = " << convert_xml_str;
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
        message = MessageBase(xml_ptr);
        if (!message.load_from_xml()) {
            ErrorL << "SIP message load failed: " << message.get_error() << ", xml = " << convert_xml_str;
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
    }

            DebugL
        << "handler sip message, root = " << message.root() << ", command = " << message.command()
        << ", sn = " << message.sn();

    // 如果是应答消息，一定来自下级平台
    int sip_code = 0;
    std::visit([&](auto &platform) {
        if (message.root() == MessageRootType::Response) sip_code = platform->on_response(std::move(message), transaction, req);
        else if (message.root() == MessageRootType::Notify) sip_code = platform->on_notify(std::move(message), transaction, req);
        else if (message.root() == MessageRootType::Control) sip_code = platform->on_control(std::move(message), transaction, req);
        else if (message.root() == MessageRootType::Query) sip_code = platform->on_query(std::move(message), transaction, req);
    }, platform_);

    if (sip_code > 0) {
        return sip_uas_reply(transaction.get(), sip_code, nullptr, 0, session.get());
    }
    return sip_code;
    //
    // if (message.root() == MessageRootType::Response) {
    //
    //
    //
    //     // 验证from 标记中的用户
    //     auto &&platform_id = get_platform_id(req.get());
    //     if (platform_id.empty()) {
    //         ErrorL << "SIP message platform id is empty";
    //         return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    //     }
    //     auto sip_server = session->getSipServer();
    //     if (!sip_server) {
    //         WarnL << "sip_server has been destroyed";
    //         return sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
    //     }
    //
    //     auto platform_ptr= std::dynamic_pointer_cast<SubordinatePlatformImpl>(sip_server->get_subordinate_platform(platform_id));
    //     if (!platform_ptr) {
    //         WarnL << "platform was not found";
    //         return sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
    //     }
    //     auto ret = platform_ptr->on_response(std::move(message), transaction, req);
    //     if (ret == 0) {
    //         return sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
    //     }
    //     if (ret > 0) {
    //         return sip_uas_reply(transaction.get(), ret, nullptr, 0, session.get());
    //     }
    //     return sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
    // }
    //
    // return std::make_shared<UasMessageHandler>(session)->run(std::move(message), transaction, req);
}

UasMessageHandler::UasMessageHandler(
    const std::shared_ptr<SipSession> &session)
    : UasMessage()
    , session_(session) {}

int UasMessageHandler::run(MessageBase &&request, std::shared_ptr<sip_uas_transaction_t> transaction, const std ::shared_ptr<sip_message_t> &sip) {
    if (request.root() == MessageRootType::Notify) {
        has_response_ = false;
    } else if (request.root() == MessageRootType::Control) {
        const auto &command = request.command();
        if (command == MessageCmdType::DeviceControl) {}
    } else if (request.root() == MessageRootType::Response) { // 是应答消息

        return 0;
    }
    switch (request.command()) {
        case MessageCmdType::DeviceControl: break;
        case MessageCmdType::DeviceConfig: break;
        case MessageCmdType::DeviceStatus: request_ = std::make_shared<DeviceStatusMessageRequest>(std::move(request)); break;
        case MessageCmdType::Catalog: break;
        case MessageCmdType::DeviceInfo: request_ = std::make_shared<DeviceInfoMessageRequest>(std::move(request)); break;
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
        case MessageCmdType::Keepalive:
            request_ = std::make_shared<KeepaliveMessageRequest>(std::move(request));
            {
            const auto message_ptr = std::make_shared<KeepaliveMessageRequest>(std::move(request));
            if (!message_ptr->load_from_xml()) {
                WarnL << "parse keepalive payload failed, " << message_ptr->get_error();
                return sip_uas_reply(transaction.get(), 400, nullptr, 0, session_.get());
            }
            if (auto ret = platform_->on_keep_alive(message_ptr)) {
                return sip_uas_reply(transaction.get(), ret, nullptr, 0, session_.get());
            }
        } break;
        case MessageCmdType::MediaStatus: break;
        case MessageCmdType::Broadcast: break;
        case MessageCmdType::UploadSnapShotFinished: break;
        case MessageCmdType::VideoUploadNotify: break;
        case MessageCmdType::DeviceUpgradeResult: break;
        default: break;
    }
    if (!request_) {
        WarnL << "Unknown message command: " << request.command();
        completed_.exchange(true);
        sip_uas_reply(transaction.get(), 400, nullptr, 0, session_.get());
        return 0;
    }
    if (!request_->load_from_xml()) {
        WarnL << "parse " << request_->command() << " failed, " << request_->get_error();
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session_.get());
    }
    if (request_->command() == MessageCmdType::Keepalive) {
        if (auto ret = platform_->on_keep_alive(std::dynamic_pointer_cast<KeepaliveMessageRequest>(request_))) {
            completed_.exchange(true);
            return sip_uas_reply(transaction.get(), ret, nullptr, 0, session_.get());
        }
        WarnL << "ignore keepalive messages, platform_id =" << platform_->account().platform_id;
        return 0;
    }
    if (has_response_) {
        from_ = get_to_uri(sip.get());
        to_ = get_from_uri(sip.get());
    }

    return 0;
}

void UasMessageHandler::response(
    std::shared_ptr<MessageBase> response, const std::function<void(bool, std::string)> &rcb, bool end) {

    sip_transport_t transport{
        SipSession::sip_via,
        SipSession::sip_send
    };
    std::shared_ptr<sip_uac_transaction_t> transaction_ptr(sip_uac_message(session_->getSipServer()->get_sip_agent().get(),from_.c_str(),to_.c_str(), nullptr, nullptr), [](sip_uac_transaction_t *t) {
        sip_uac_transaction_release(t);
    });
    sip_uac_send(transaction_ptr.get(),nullptr,0, &transport,session_.get());

}



} // namespace gb28181

/**********************************************************************************************************
文件名称:   uas_message_handler.cpp
创建时间:   25-2-7 下午6:53
作者名称:   Kevin
文件路径:   src/handler
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午6:53

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午6:53       描述:   创建文件

**********************************************************************************************************/