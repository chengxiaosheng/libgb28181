#include "gb28181/local_server.h"
#include "gb28181/message/catalog_message.h"
#include "gb28181/request/invite_request.h"
#include "gb28181/request/sdp2.h"
#include "gb28181/request/subscribe_request.h"

#include <iostream>

#include <Network/Buffer.h>
#include <Poller/EventPoller.h>
#include <Util/NoticeCenter.h>
#include <csignal>
#include <gb28181/message/device_info_message.h>
#include <gb28181/sip_common.h>
#include <gb28181/subordinate_platform.h>
#include <gb28181/type_define_ext.h>
#include <thread>

using namespace gb28181;

// 全局变量，用于检测是否收到退出信号
std::atomic<bool> is_running{true};
// 信号处理函数
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nSignal received, shutting down...\n";
        is_running = false; // 设置退出标志
    }
}
static std::shared_ptr<void> tag = nullptr;

int main() {
    gb28181::local_account account;
    {
        account.platform_id = "65010100002000100001";
        account.domain = "6501010000";
        account.name = "test platform";
        account.host = "::";
        account.password = "123456";
        account.port = 5060;
        account.auth_type = gb28181::SipAuthType::digest;
        account.allow_auto_register = true;
    };
    auto local_server = gb28181::LocalServer::new_local_server(account);
    local_server->set_new_subordinate_account_callback([](std::shared_ptr<gb28181::LocalServer> server, std::shared_ptr<gb28181::subordinate_account> account, std::function<void(bool)> allow_cb) {
        allow_cb(true);
    });
    local_server->run();
    std::shared_ptr<SubscribeRequest> catalog_sub;
    toolkit::NoticeCenter::Instance().addListener(&tag, gb28181::Broadcast::kEventSubordinatePlatformStatus, [&](std::shared_ptr<gb28181::SubordinatePlatform> platform , gb28181::PlatformStatusType status, const std::string& message) {
        platform->query_device_info("", [](const std::shared_ptr<gb28181::RequestProxy> &proxy) {
            if (*proxy) {
                auto resp = std::dynamic_pointer_cast<gb28181::DeviceInfoMessageResponse>(proxy->response());
                InfoL << "send_time = " << proxy->send_time() << ", reply_time = " << proxy->reply_time() << ", recv_time1 = " << proxy->response_begin_time() << ", recv_time2 = " << proxy->response_end_time();
                InfoL << "device_id = " << resp->device_id().value_or("") << ", = " << resp->model() << ", " << resp->firmware() << ", " << resp->manufacturer();
            }
        });
        platform->query_config("", DeviceConfigType::BasicParam | DeviceConfigType::VideoParamOpt | DeviceConfigType::SVACEncodeConfig, [](const std::shared_ptr<gb28181::RequestProxy> &proxy) {
            InfoL << "get query config result, ret = " << (int)proxy->status() << ", response_count = " << proxy->all_response().size() << ", error = " << proxy->error();
        });
        platform->query_preset("", [](const std::shared_ptr<gb28181::RequestProxy> &proxy) {
            InfoL << "get query prset result, ret = " << (int)proxy->status() << ", response_count = " << proxy->all_response().size() << ", error = " << proxy->error();
        });

        auto catalog_sub_message = std::make_shared<CatalogRequestMessage>(platform->account().platform_id);
        SubscribeRequest::subscribe_info info;
        info.event = "Catalog";
        catalog_sub = SubscribeRequest::new_subscribe(platform, catalog_sub_message, std::move(info));
        catalog_sub->start();

        auto sdp_ptr =std::make_shared<sdp_description>();
        sdp_ptr->origin.username = account.platform_id;
        sdp_ptr->origin.addr_type  = sdp_network_address_type::IP4;
        sdp_ptr->origin.addr = "10.1.20.2";
        sdp_ptr->s_name = sdp_session_type::Play;
        sdp_ptr->timing = sdp_time_description();
        sdp_ptr->timing->start_time = 0;
        sdp_ptr->timing->end_time = 0;
        sdp_ptr->connection = sdp_connection();
        sdp_ptr->connection.value().addr = "10.1.20.2";
        sdp_ptr->media.type = sdp_media_type::video;
        sdp_ptr->media.port = 20002;
        sdp_ptr->media.payloads[96] = {96, "PS", 90000};
        sdp_ptr->media.payloads[97] = {97, "MPEG4", 90000};
        sdp_ptr->media.payloads[98] = {98, "H264", 90000};
        sdp_ptr->media.payloads[99] = {99, "H265", 90000};
        sdp_ptr->y_ssrc = 1;

       auto invite_request = InviteRequest::new_invite_request(platform, sdp_ptr);
        invite_request->to_invite_request([invite_request](bool ret, std::string err, const std::shared_ptr<sdp_description> &remote_sdp) {
            InfoL << "invite request ret = " << ret << ", err = " << err ;
            if (remote_sdp) {
                InfoL << "remote_sdp = " << get_sdp_description(*remote_sdp);
            }
        });

    });

    // 捕获 SIGINT 和 SIGTERM 信号
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 主线程保持运行直到收到退出信号
    std::cout << "Press Ctrl+C to exit..." << std::endl;
    while (is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 防止忙等
    }

    std::cout << "Server stopped." << std::endl;
    return 0;
}
