#include "gb28181/local_server.h"
#include "gb28181/message/catalog_message.h"
#include "gb28181/request/invite_request.h"
#include "gb28181/request/subscribe_request.h"

#include <iostream>

#include <Network/Buffer.h>
#include <Poller/EventPoller.h>
#include <Util/NoticeCenter.h>
#include <csignal>
#include <gb28181/message/device_info_message.h>
#include <gb28181/sip_event.h>
#include <gb28181/subordinate_platform.h>
#include <gb28181/type_define_ext.h>
#include <thread>

using namespace gb28181;

// 全局变量，用于检测是否收到退出信号
std::atomic<bool> is_running{true};

static std::shared_ptr<void> tag = nullptr;
std::shared_ptr<LocalServer> local_server;

void run() {

    gb28181::local_account account;
    {
        account.platform_id = "65010100002000100001";
        account.domain = "6501010000";
        account.name = "test platform";
        account.host = "::";
        account.password = "123456";
        account.port = 5060;
        account.local_host = "10.1.20.38";
        account.local_port = 5060;
        account.auth_type = gb28181::SipAuthType::digest;
        account.allow_auto_register = true;
    };
    local_server = gb28181::LocalServer::new_local_server(account);
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

        auto sdp_ptr =std::make_shared<SdpDescription>();
        sdp_ptr->session().origin.username = account.platform_id;
        sdp_ptr->session().origin.addr_type  = SDPAddressType::IP4;
        sdp_ptr->session().origin.address = "10.1.20.2";
        sdp_ptr->session().sessionType = SessionType::PLAY;
        sdp_ptr->session().connection.address = "10.1.20.2";
        MediaDescription media;
        media.port = 20002;
        media.direction = Direction::RECVONLY;
        media.payloads.emplace_back(PayloadInfo{96, 90000, "PS"});
        media.payloads.emplace_back(PayloadInfo{97, 90000, "MPEG4"});
        media.payloads.emplace_back(PayloadInfo{98, 90000, "H264"});
        media.payloads.emplace_back(PayloadInfo{99, 90000, "H265"});
        media.ssrc = 1;
        media.ice.pwd = "123456";
        media.ice.ufrag = "1234567adfa";
        media.ice.candidates.emplace_back(ICECandidate{"1",ICEComponentType::RTP,TransportProtocol::UDP,ICECandidateType::HOST,20002,0,"10.1.20.2",""});
        media.ice.candidates.emplace_back(ICECandidate{"2",ICEComponentType::RTCP,TransportProtocol::UDP,ICECandidateType::HOST,20003,0,"10.1.20.2",""});
        media.ice.candidates.emplace_back(ICECandidate{"3",ICEComponentType::RTP,TransportProtocol::UDP,ICECandidateType::SRFLX,20002,0,"10.1.20.2",""});
        media.ice.candidates.emplace_back(ICECandidate{"4",ICEComponentType::RTCP,TransportProtocol::UDP,ICECandidateType::SRFLX,20003,0,"10.1.20.2",""});

        sdp_ptr->media().emplace_back(std::move(media));

       auto invite_request = InviteRequest::new_invite_request(platform, sdp_ptr, platform->account().platform_id);
        invite_request->to_invite_request([invite_request](bool ret, std::string err, const std::shared_ptr<SdpDescription> &remote_sdp) {
            InfoL << "invite request ret = " << ret << ", err = " << err ;
            if (remote_sdp) {
                InfoL << "remote_sdp = " <<  remote_sdp->generate();
            }
            invite_request->to_bye("");
        });
    });


    super_account sup_account;
    {
        sup_account.platform_id = "65010400002000100005";
        sup_account.domain = "6501040000";
        sup_account.name = "级联测试平台";
        sup_account.host = "172.16.11.15";
        sup_account.port = 55060;
        sup_account.password = "123456";
        sup_account.transport_type = TransportType::udp;
    };
    local_server->add_super_platform(std::move(sup_account));


    std::cout << "Server stopped." << std::endl;
}


int main() {
    toolkit::EventPollerPool::setPoolSize(std::thread::hardware_concurrency());

    toolkit::Logger::Instance().add(
           std::make_shared<toolkit::ConsoleChannel>("ConsoleChannel", toolkit::LogLevel::LTrace));
    auto fileChannel = std::make_shared<toolkit::FileChannel>("FileChannel", "./logs", toolkit::LogLevel::LTrace);
    fileChannel->setMaxDay(7);
    fileChannel->setFileMaxCount(100);
    fileChannel->setFileMaxSize(1024 * 1024 * 100);
    toolkit::Logger::Instance().add(fileChannel);
    toolkit::Logger::Instance().setWriter(std::make_shared<toolkit::AsyncLogWriter>());
    InfoL << "start ....";
    TraceL << "start ....";

    toolkit::EventPollerPool::Instance().getPoller()->async(run);

    // 设置退出信号处理函数  [AUTO-TRANSLATED:4f047770]
    // set exit signal handler
    static toolkit::semaphore sem;
    signal(SIGINT, [](int) {
        InfoL << "SIGINT:exit";
        signal(SIGINT, SIG_IGN); // 设置退出信号
        sem.post();
    }); // 设置退出信号

    signal(SIGTERM, [](int) {
        WarnL << "SIGTERM:exit";
        signal(SIGTERM, SIG_IGN);
        sem.post();
    });
    sem.wait();
}
