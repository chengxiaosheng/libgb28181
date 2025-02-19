#include "subordinate_platform_impl.h"

#include <Util/NoticeCenter.h>
#include <gb28181/message/config_download_messsage.h>
#include <gb28181/message/device_config_message.h>
#include <gb28181/message/device_info_message.h>
#include <gb28181/message/device_status_message.h>
#include <gb28181/message/keepalive_message.h>
#include <gb28181/message/preset_message.h>
#include <gb28181/sip_common.h>
#include <inner/sip_server.h>
#include <inner/sip_session.h>
#include <request/RequestProxyImpl.h>
#include <sip-transport.h>

using namespace gb28181;

SubordinatePlatformImpl::SubordinatePlatformImpl(subordinate_account account, const std::shared_ptr<SipServer> &server)
    : SubordinatePlatform()
    , PlatformHelper()
    , account_(std::move(account)) {
    local_server_weak_ = server;
    if (account_.local_host.empty()) {
        account_.local_host = toolkit::SockUtil::get_local_ip();
    }
    if (account_.local_port == 0) {
        account_.local_port = server->get_account().port;
    }
    from_uri_ = "sip:" + get_sip_server()->get_account().platform_id + "@" + account_.local_host + ":"
        + std::to_string(account_.local_port);
}

SubordinatePlatformImpl::~SubordinatePlatformImpl() {}

void SubordinatePlatformImpl::shutdown() {}

void SubordinatePlatformImpl::set_status(PlatformStatusType status, std::string error) {
    if (status == account_.plat_status.status)
        return;
    account_.plat_status.status = status;
    account_.plat_status.error = std::move(error);
    if (status == PlatformStatusType::online) {
        account_.plat_status.register_time = toolkit::getCurrentMicrosecond(true);
    }
    // 异步广播平台在线状态
    toolkit::EventPollerPool::Instance().getPoller()->async(
        [this_ptr = shared_from_this(), status, error]() {
            toolkit::NoticeCenter::Instance().emitEvent(
                Broadcast::kEventSubordinatePlatformStatus, this_ptr, status, error);
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
        // 广播通知心跳消息
        toolkit::NoticeCenter::Instance().emitEvent(Broadcast::kEventSubKeepalive, shared_from_this(), request);
    }
    return 200;
}
void SubordinatePlatformImpl::on_device_info(
    std::shared_ptr<DeviceInfoMessageRequest> request, std::function<void(std::shared_ptr<MessageBase>)> &&reply) {

    auto reply_ptr = std::make_shared<std::function<void(std::shared_ptr<MessageBase>)>>(std::move(reply));
    auto reply_flag = std::make_shared<std::atomic_flag>();
    auto do_cb = [reply_ptr, reply_flag](std::shared_ptr<MessageBase> msg) {
        if (!reply_flag->test_and_set()) {
            (*reply_ptr)(msg);
        }
    };
    auto time_out_task = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(T2, [do_cb]() {
        do_cb(nullptr);
        return 0;
    });
    if (toolkit::NoticeCenter::Instance().emitEvent(
            Broadcast::kEventOnDeviceInfoRequest, shared_from_this(), request,
            [do_cb, time_out_task](std::shared_ptr<DeviceInfoMessageResponse> response) {
                time_out_task->cancel();
                do_cb(response);
            })
        == 0) {
        WarnL << "DeviceInfo not Implementation";
        do_cb(nullptr);
        time_out_task->cancel();
    }
}
void SubordinatePlatformImpl::on_device_status(
    std::shared_ptr<DeviceStatusMessageRequest> request, std::function<void(std::shared_ptr<MessageBase>)> &&reply) {

    auto reply_ptr = std::make_shared<std::function<void(std::shared_ptr<MessageBase>)>>(std::move(reply));
    auto reply_flag = std::make_shared<std::atomic_flag>();
    auto do_cb = [reply_ptr, reply_flag](std::shared_ptr<MessageBase> msg) {
        if (!reply_flag->test_and_set()) {
            (*reply_ptr)(msg);
        }
    };
    auto time_out_task = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(T2, [do_cb]() {
        do_cb(nullptr);
        return 0;
    });
    if (toolkit::NoticeCenter::Instance().emitEvent(
            Broadcast::kEventOnDeviceInfoRequest, shared_from_this(), request,
            [do_cb, time_out_task](std::shared_ptr<DeviceStatusMessageResponse> response) {
                time_out_task->cancel();
                do_cb(response);
            })
        == 0) {
        WarnL << "DeviceStatus not Implementation";
        do_cb(nullptr);
        time_out_task->cancel();
    }
}

int SubordinatePlatformImpl::on_notify(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    if (message.command() == MessageCmdType::Keepalive) {
        auto keepalive_request = std::make_shared<KeepaliveMessageRequest>(std::move(message));
        return on_keep_alive(keepalive_request);
    }
    if (message.command() == MessageCmdType::Alarm) {
        return 200;
    }
    if (message.command() == MessageCmdType::MediaStatus) {
        return 200;
    }
    if (message.command() == MessageCmdType::MobilePosition) {
        return 200;
    }
    if (message.command() == MessageCmdType::UploadSnapShotFinished) {
        return 200;
    }
    if (message.command() == MessageCmdType::VideoUploadNotify) {
        return 200;
    }
    if (message.command() == MessageCmdType::DeviceUpgradeResult) {
        return 200;
    }
    return 0;
}
void SubordinatePlatformImpl::on_invite(
    const std::shared_ptr<InviteRequest> &invite_request,
    std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) {
    resp(400, nullptr);
}

bool SubordinatePlatformImpl::update_local_via(std::string host, uint16_t port) {
    bool changed = false;
    if (!host.empty() && account_.local_host != host) {
        account_.local_host = host;
        changed = true;
    }
    if (port && account_.local_port != port) {
        account_.local_port = port;
        changed = true;
    }
    if (changed) {
        from_uri_ = "sip:" + get_sip_server()->get_account().platform_id + "@" + account_.local_host + ":"
            + std::to_string(account_.local_port);
        // 通知上层应用？
        toolkit::EventPollerPool::Instance().getExecutor()->async(
            [this_ptr = shared_from_this()]() {
                toolkit::NoticeCenter::Instance().emitEvent(
                    Broadcast::kEventSubordinatePlatformContactChanged, this_ptr, this_ptr->account_.local_host,
                    this_ptr->account_.local_port);
            },
            false);
    }
    return changed;
}

void SubordinatePlatformImpl::query_device_info(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> ret) {
    auto request = std::make_shared<DeviceInfoMessageRequest>(device_id.empty() ? account_.platform_id : device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(ret));
}

void SubordinatePlatformImpl::query_device_status(
    const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> ret) {
    auto request = std::make_shared<DeviceStatusMessageRequest>(device_id.empty() ? account_.platform_id : device_id);
    RequestProxy::newRequestProxy(shared_from_this(), request)->send(std::move(ret));
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