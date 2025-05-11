#include "subordinate_platform_impl.h"

#include "gb28181/message/alarm_request_message.h"
#include "gb28181/message/catalog_message.h"
#include "gb28181/message/cruise_track_message.h"
#include "gb28181/message/device_control_message.h"
#include "gb28181/message/home_position_message.h"
#include "gb28181/message/notify_message.h"
#include "gb28181/message/ptz_position_message.h"
#include "gb28181/message/record_info_message.h"
#include "gb28181/message/sd_card_status_message.h"
#include "gb28181/request/subscribe_request.h"
#include "inner/sip_common.h"
#include "request/invite_request_impl.h"

#include <Util/NoticeCenter.h>
#include <gb28181/message/config_download_messsage.h>
#include <gb28181/message/device_config_message.h>
#include <gb28181/message/device_info_message.h>
#include <gb28181/message/device_status_message.h>
#include <gb28181/message/keepalive_message.h>
#include <gb28181/message/preset_message.h>
#include <gb28181/sip_event.h>
#include <inner/sip_server.h>
#include <inner/sip_session.h>
#include <request/RequestProxyImpl.h>
#include <sip-transport.h>
#include <sip-uas.h>

using namespace gb28181;
using namespace toolkit;

SubordinatePlatformImpl::SubordinatePlatformImpl(subordinate_account account, const std::shared_ptr<SipServer> &server)
    : SubordinatePlatform()
    , PlatformHelper()
    , account_(std::move(account)) {

    const auto & server_account = server->get_account();
    if (account_.local_host.empty()) {
        account_.local_host = server_account.local_host;
    }
    if (!account_.local_port) {
        account_.local_port = server_account.local_port;
    }
    local_server_weak_ = server;
    from_uri_ = "sip:" + server_account.platform_id + "@" + server_account.local_host + ":"
        + std::to_string(server_account.local_port);
    contact_uri_ = "sip:" + server_account.platform_id + "@" + account_.local_host + ":"
        + std::to_string(account_.local_port);
}

SubordinatePlatformImpl::~SubordinatePlatformImpl() {}
int SubordinatePlatformImpl::on_recv_register(
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, const std::string &user, const std::string &location, int expires) {

    auto sip_server = session->getSipServer();
    if (!sip_server) {
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, session.get());
    }
    if (auto platform
        = std::dynamic_pointer_cast<SubordinatePlatformImpl>(sip_server->get_subordinate_platform(user))) {
        // 将session 添加到平台
        if (!session->is_udp()) {
            platform->set_tcp_session(session);
        }
        if (struct sockaddr_storage addr {}; SockUtil::get_sock_peer_addr(session->getSock()->rawFD(), addr)) {
            platform->on_platform_addr_changed(addr);
        }
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
            account.local_port = session->get_local_port();
            account.local_host = session->get_local_ip();// SockUtil::get_local_ip(session->getSock()->rawFD());
            if(account.local_host.empty()) {
                account.local_port = server_account.local_port;
            }
            if(account.local_port == 0) {
                account.local_host = server_account.local_host;
            }
            account.plat_status.register_time = getCurrentMicrosecond(true);
            account.plat_status.status = PlatformStatusType::registering;

            auto account_ptr = std::make_shared<subordinate_account>(std::move(account));
            sip_server->new_subordinate_account(
                account_ptr, [session, transaction, expires](std::shared_ptr<SubordinatePlatformImpl> platform) {
                    if (platform) {
                        if (session->is_udp()) {
                            platform->set_tcp_session(session);
                        }
                        if (struct sockaddr_storage addr{};SockUtil::get_sock_peer_addr(session->getSock()->rawFD(),addr)) {
                            platform->on_platform_addr_changed(addr);
                        }
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

void SubordinatePlatformImpl::shutdown() {
    NOTICE_EMIT(
        kEventOnSubordinatePlatformShutdownArgs, Broadcast::kEventOnSubordinatePlatformShutdown,
        std::dynamic_pointer_cast<SubordinatePlatform>(shared_from_this()));
}
std::shared_ptr<LocalServer> SubordinatePlatformImpl::get_local_server() const {
    return local_server_weak_.lock();
}

void SubordinatePlatformImpl::set_status(PlatformStatusType status, std::string error) {
    // 当为伪装在线时， 下次状态变更不论是否在线都需要广播
    if (status == account_.plat_status.status && !camouflage_online_)
        return;
    if (camouflage_online_) {
        camouflage_online_ = false;
    }
    account_.plat_status.status = status;
    account_.plat_status.error = std::move(error);
    if (status == PlatformStatusType::online) {
        account_.plat_status.register_time = toolkit::getCurrentMicrosecond(true);
    } else {
        keepalive_timer_.reset();
    }
    keepalive_timer_ = std::make_shared<toolkit::Timer>(
        5,
        [weak_this = weak_from_this()]() {
            if (auto this_ptr = weak_this.lock()) {
                auto last_time = ((std::max)(this_ptr->account_.plat_status.register_time,
                                             this_ptr->account_.plat_status.keepalive_time));
                auto now_time = toolkit::getCurrentMicrosecond(true);
                if (last_time + 3 * 30 * 1000000L <= now_time) {
                    this_ptr->set_status(PlatformStatusType::offline, "keepalive timeout");
                }
                return true;
            }
            return false;
        },
        nullptr);
    // 异步广播平台在线状态
        EventPollerPool::Instance().getPoller()->async(
        [this_ptr = shared_from_this(), status, error]() {
            {
                std::lock_guard<decltype(status_cbs_mtx_)> lck( this_ptr->status_cbs_mtx_);
                for(auto &it : this_ptr->status_cbs_) {
                    it.second(status);
                }
            }
            NOTICE_EMIT(
                kEventSubordinatePlatformStatusArgs, Broadcast::kEventSubordinatePlatformStatus,
                std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), status, error);
        },
        false);
}

int SubordinatePlatformImpl::on_keep_alive(std::shared_ptr<KeepaliveMessageRequest> request) {

    if (account_.plat_status.status != PlatformStatusType::online) {
        return 0;
    }
    account_.plat_status.keepalive_time = toolkit::getCurrentMicrosecond(true);
    if (on_keep_alive_callback_) {
        toolkit::EventPollerPool::Instance().getPoller()->async(
            [this_ptr = shared_from_this(), request = std::move(request)]() {
                this_ptr->on_keep_alive_callback_(this_ptr, request);
            });
    }
    // 广播通知心跳消息
    toolkit::EventPollerPool::Instance().getPoller()->async([this_ptr = shared_from_this(), request]() {
        NOTICE_EMIT(
            kEventSubKeepaliveArgs, Broadcast::kEventSubKeepalive,
            std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), request);
    });
    return 200;
}

int SubordinatePlatformImpl::on_notify(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    DebugL << "on notify , " << message;
    const auto type = message.command();
    switch (type) {
        case MessageCmdType::Keepalive: {
            auto keepalive_request = std::make_shared<KeepaliveMessageRequest>(std::move(message));
            return on_keep_alive(keepalive_request);
        }
        case MessageCmdType::Alarm: {
            auto alarm_ptr = std::make_shared<AlarmNotifyMessage>(std::move(message));
            alarm_ptr->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [alarm_ptr, this_ptr = shared_from_this()]() {
                    // 广播通知
                    NOTICE_EMIT(
                        kEventSubordinateNotifyAlarmArgs, Broadcast::kEventSubordinateNotifyAlarm,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), alarm_ptr);
                    // 回复确认
                    auto reqeust = std::make_shared<AlarmNotifyResponseMessage>(alarm_ptr->device_id().value_or(""));
                    std::make_shared<RequestProxyImpl>(
                        this_ptr, reqeust, RequestProxy::RequestType::NoResponse, alarm_ptr->sn())
                        ->send([](std::shared_ptr<RequestProxy>) {});
                },
                false);
            return 200;
        }
        case MessageCmdType::Catalog: {
            DebugL << "on catalog";
            auto notify = std::make_shared<CatalogNotifyMessage>(std::move(message));
            notify->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [this_ptr = shared_from_this(), notify] {
                    NOTICE_EMIT(
                        kEventSubordinateNotifyCatalogArgs, Broadcast::kEventSubordinateNotifyCatalog,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), notify);
                },
                false);
            return 200;
        }
        case MessageCmdType::MediaStatus: {
            auto notify = std::make_shared<MediaStatusNotifyMessage>(std::move(message));
            notify->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [this_ptr = shared_from_this(), notify] {
                    NOTICE_EMIT(
                        kEventSubordinateNotifyMediaStatusArgs, Broadcast::kEventSubordinateNotifyMediaStatus,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), notify);
                },
                false);
            return 200;
        }
        case MessageCmdType::MobilePosition: {
            auto notify = std::make_shared<MobilePositionNotifyMessage>(std::move(message));
            notify->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [this_ptr = shared_from_this(), notify] {
                    NOTICE_EMIT(
                        kEventSubordinateNotifyMobilePositionArgs, Broadcast::kEventSubordinateNotifyMobilePosition,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), notify);
                },
                false);
            return 200;
        }
        case MessageCmdType::UploadSnapShotFinished: {
            auto notify = std::make_shared<UploadSnapShotFinishedNotifyMessage>(std::move(message));
            notify->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [this_ptr = shared_from_this(), notify] {
                    NOTICE_EMIT(
                        kEventSubordinateNotifyUploadSnapShotFinishedArgs,
                        Broadcast::kEventSubordinateNotifyUploadSnapShotFinished,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), notify);
                },
                false);
            return 200;
        }
        case MessageCmdType::VideoUploadNotify: {
            auto notify = std::make_shared<VideoUploadNotifyMessage>(std::move(message));
            notify->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [this_ptr = shared_from_this(), notify] {
                    NOTICE_EMIT(
                        kEventSubordinateNotifyVideoUploadNotifyArgs,
                        Broadcast::kEventSubordinateNotifyVideoUploadNotify,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), notify);
                },
                false);
            return 200;
        }
        case MessageCmdType::DeviceUpgradeResult: {
            auto notify = std::make_shared<DeviceUpgradeResultNotifyMessage>(std::move(message));
            notify->load_from_xml();
            toolkit::EventPollerPool::Instance().getPoller()->async(
                [this_ptr = shared_from_this(), notify] {
                    NOTICE_EMIT(
                        kEventSubordinateNotifyDeviceUpgradeResultArgs,
                        Broadcast::kEventSubordinateNotifyDeviceUpgradeResult,
                        std::dynamic_pointer_cast<SubordinatePlatform>(this_ptr), notify);
                },
                false);
            return 200;
        }
        default: break;
    }
    return 404;
}
void SubordinatePlatformImpl::on_invite(
    const std::shared_ptr<InviteRequest> &invite_request,
    std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) {
    resp(400, nullptr);
}

void SubordinatePlatformImpl::query_device_status(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> ret) {
    auto request = std::make_shared<DeviceStatusMessageRequest>(device_id.empty() ? account_.platform_id : device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(ret));
}
void SubordinatePlatformImpl::query_catalog(
    const std::string &device_id, RequestProxy::ResponseCallback data_callback,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<CatalogRequestMessage>(device_id);
    auto proxy = RequestProxy::newRequestProxy(shared_from_this(), request);
    if (rcb) {
        proxy->set_response_callback(std::forward<decltype(data_callback)>(data_callback));
    }
    proxy->send(std::move(rcb));
}

void SubordinatePlatformImpl::query_device_info(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> ret) {
    auto request = std::make_shared<DeviceInfoMessageRequest>(device_id.empty() ? account_.platform_id : device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(ret));
}
void SubordinatePlatformImpl::query_record_info(
    const std::shared_ptr<RecordInfoRequestMessage> &req, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    RequestProxy::newRequestProxy(shared_from_this(), req)->send(std::move(rcb));
}
void SubordinatePlatformImpl::query_home_position(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<HomePositionRequestMessage>(device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::query_cruise_list(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<CruiseTrackListRequestMessage>(device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::query_cruise(
    const std::string &device_id, int32_t number, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<CruiseTrackRequestMessage>(device_id, number);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::query_ptz_position(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<PTZPositionRequestMessage>(device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::query_sd_card_status(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<SdCardRequestMessage>(device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_ptz(
    const std::string &device_id, PTZCommand ptz_cmd, std::string name,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_PTZCmd>(device_id, std::move(ptz_cmd), std::nullopt);
    if (!name.empty()) {
        request->ptz_cmd_params() = PtzCmdParams();
        request->ptz_cmd_params().value().CruiseTrackName = name;
        request->ptz_cmd_params().value().PresetName = name;
    }
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_tele_boot(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_TeleBoot>(device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_record_cmd(
    const std::string &device_id, RecordType type, int stream_number,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_RecordCmd>(device_id, type, stream_number);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_guard_cmd(
    const std::string &device_id, GuardType type, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_GuardCmd>(device_id, type);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_alarm_cmd(
    const std::string &device_id, std::optional<AlarmCmdInfoType> info,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_AlarmCmd>(device_id, std::move(info));
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_i_frame_cmd(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_IFrameCmd>(device_id, std::vector<std::string>());
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_drag_zoom_in(
    const std::string &device_id, DragZoomType drag, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_DragZoomIn>(device_id, std::move(drag));
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_drag_zoom_out(
    const std::string &device_id, DragZoomType drag, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_DragZoomOut>(device_id, std::move(drag));
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_home_position(
    const std::shared_ptr<DeviceControlRequestMessage_HomePosition> &req,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    RequestProxy::newRequestProxy(shared_from_this(), req)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_ptz_precise_ctrl(
    std::shared_ptr<DeviceControlRequestMessage_PtzPreciseCtrl> &req,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    RequestProxy::newRequestProxy(shared_from_this(), req)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_device_upgrade(
    const std::shared_ptr<DeviceControlRequestMessage_DeviceUpgrade> &req,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    RequestProxy::newRequestProxy(shared_from_this(), req)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_format_sd_card(
    const std::string &device_id, int index, std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request = std::make_shared<DeviceControlRequestMessage_FormatSDCard>(device_id, index);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}
void SubordinatePlatformImpl::device_control_target_track(
    const std::shared_ptr<DeviceControlRequestMessage_TargetTrack> &req,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    RequestProxy::newRequestProxy(shared_from_this(), req)->send(std::move(rcb));
}

void SubordinatePlatformImpl::query_config(
    const std::string &device_id, DeviceConfigType config_type,
    std::function<void(std::shared_ptr<RequestProxy>)> ret) {
    auto request = std::make_shared<ConfigDownloadRequestMessage>(
        device_id.empty() ? account_.platform_id : device_id, config_type);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(ret));
}
void SubordinatePlatformImpl::query_preset(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> ret) {
    auto request = std::make_shared<PresetRequestMessage>(device_id.empty() ? account_.platform_id : device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(ret));
}
void SubordinatePlatformImpl::device_config(
    const std::string &device_id, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> &&config,
    std::function<void(std::shared_ptr<RequestProxy>)> rcb) {
    auto request
        = std::make_shared<DeviceConfigRequestMessage>(device_id.empty() ? account_.platform_id : device_id, config);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(rcb));
}

std::shared_ptr<InviteRequest>
SubordinatePlatformImpl::invite(const std::shared_ptr<SdpDescription> &sdp, const std::string &device_id) {
    return InviteRequest::new_invite_request(shared_from_this(), sdp, device_id);
}

std::shared_ptr<SubscribeRequest>
SubordinatePlatformImpl::subscribe(const std::shared_ptr<MessageBase> &request, SubscribeRequest::subscribe_info info) {
    return SubscribeRequest::new_subscribe(shared_from_this(), request, std::move(info));
}
void SubordinatePlatformImpl::camouflage_online(uint64_t register_time, uint64_t keepalive_time) {
    if (account_.plat_status.status == PlatformStatusType::online)
        return;
    account_.plat_status.register_time = register_time;
    account_.plat_status.keepalive_time = keepalive_time;
    account_.plat_status.status = PlatformStatusType::online;
    camouflage_online_ = true;
    keepalive_timer_ = std::make_shared<toolkit::Timer>(
        5,
        [weak_this = weak_from_this()]() {
            if (auto this_ptr = weak_this.lock()) {
                auto last_time = ((std::max)(
                    this_ptr->account_.plat_status.register_time, this_ptr->account_.plat_status.keepalive_time));
                auto now_time = toolkit::getCurrentMicrosecond(true);
                if (last_time + 3 * 30 * 1000000L <= now_time) {
                    this_ptr->set_status(PlatformStatusType::offline, "keepalive timeout");
                }
                return true;
            }
            return false;
        },
        nullptr);
}

/**********************************************************************************************************
文件名称:   subordinate_platform_impl.cpp
创建时间:   25-2-7 下午3:11
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午3:11

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午3:11       描述:   创建文件

**********************************************************************************************************/