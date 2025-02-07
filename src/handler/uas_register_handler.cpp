#include "uas_register_handler.h"
#include "inner/sip_event.h"
#include "inner/sip_session.h"
#include "sip-uas.h"

#include <Util/NoticeCenter.h>
#include <gb28181/local_server.h>
#include <inner/sip_common.h>
#include <inner/sip_server.h>
#include <subordinate_platform_impl.h>

using namespace gb28181;

static void *uas_register_handler_tag = nullptr;

static std::unordered_map<std::string,std::shared_ptr<SubordinatePlatform>> registering_platforms_; // 正在注册中的平台?
static std::mutex registering_platforms_mtx_;

static void on_register(EventOnRegisterArgs) {

    auto sip_server = session->getSipServer();
    if (!sip_server) {
        return (void)sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
    }
    auto server_ptr = sip_server->get_server();
    if (!server_ptr) {
        return (void)sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
    }
    server_ptr->get_subordinate_platform(user, [=](const std::shared_ptr<SubordinatePlatform>& subordinate_platform) {
        const auto & server_account = server_ptr->get_account();
        if (!subordinate_platform) {
            if (!server_ptr->allow_auto_register()) {
                return (void)sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
            }
            if (expires == 0) {
                return (void)sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
            }
            if (server_account.auth_type == SipAuthType::none || verify_authorization(req.get(), user, server_account.password)) {
                // 构建一个新的平台
                subordinate_account account;
                account.name = user;
                account.platform_id = user;
                account.domain = user.substr(0,10);
                account.host = session->get_peer_ip();
                account.port = session->get_peer_port();
                account.password = server_account.password;
                account.auth_type = server_account.auth_type;
                account.transport_type = session->is_udp() ? TransportType::udp : TransportType::tcp;
                auto platform = std::make_shared<SubordinatePlatformImpl>(account);
                platform->add_session(session);
                platform->set_status(PlatformStatusType::online, {});
                // 告知上层应用添加了新的下级平台？
                registering_platforms_.emplace(std::make_pair(user, platform));

                return (void)sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
            }
            set_message_www_authenticate(transaction.get(), server_account.domain);
            return (void)sip_uas_reply(transaction.get(), 401, nullptr, 0, session.get());
        }
        auto platform = std::dynamic_pointer_cast<SubordinatePlatformImpl>(subordinate_platform);
        // 将session 添加到平台
        platform->add_session(session);
        if (expires == 0) {
            platform->set_status(PlatformStatusType::offline, "logout");
            return (void)sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
        }
        auto & account = platform->account();
        if (account.auth_type == SipAuthType::none || verify_authorization(req.get(), user, account.password)) {
            platform->set_status(PlatformStatusType::online, {});
            return (void)sip_uas_reply(transaction.get(), 200, nullptr, 0, session.get());
        }
        set_message_www_authenticate(transaction.get(), server_account.domain);
        return (void)sip_uas_reply(transaction.get(), 401, nullptr, 0, session.get());
    });
}

void UasRegisterHandler::Init() {
    static toolkit::onceToken token(
        []() {
            // 监听收到的注册事件消息
            toolkit::NoticeCenter::Instance().addListener(&uas_register_handler_tag, kEventOnRegister, on_register);
        },
        []() { toolkit::NoticeCenter::Instance().delListener(&uas_register_handler_tag); });
}

/**********************************************************************************************************
文件名称:   uas_register_handler.cpp
创建时间:   25-2-6 下午6:41
作者名称:   Kevin
文件路径:   src/handler
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 下午6:41

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 下午6:41       描述:   创建文件

**********************************************************************************************************/