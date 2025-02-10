#include "subordinate_platform_impl.h"

#include <Util/NoticeCenter.h>
#include <gb28181/message/device_info_message.h>
#include <gb28181/message/device_status_message.h>
#include <gb28181/sip_common.h>
#include <inner/sip_session.h>
#include <sip-transport.h>

using namespace gb28181;

SubordinatePlatformImpl::SubordinatePlatformImpl(subordinate_account account)
    : account_(std::move(account)) {}

SubordinatePlatformImpl::~SubordinatePlatformImpl() {}

void SubordinatePlatformImpl::shutdown() {}

void SubordinatePlatformImpl::add_session(const std::shared_ptr<SipSession> &session) {
    auto index = session->is_udp() ? 0 : 1;
    if (sip_session_[index] == session)
        return;
    sip_session_[index] = session;
    // todo: tcp 添加连接状态检测
}

void SubordinatePlatformImpl::set_status(PlatformStatusType status, std::string error) {
    if (status == account_.plat_status.status) return;
    account_.plat_status.status = status;
    account_.plat_status.error = std::move(error);
    if (status == PlatformStatusType::online) {
        account_.plat_status.register_time = toolkit::getCurrentMicrosecond();
    }
    // todo: 广播平台在线状态
}


int SubordinatePlatformImpl::on_keep_alive(
    std::shared_ptr<KeepaliveMessageRequest> request) {

    if (account_.plat_status.status != PlatformStatusType::online) {
        return 0;
    }
    account_.plat_status.keepalive_time = toolkit::getCurrentMicrosecond();
    if (on_keep_alive_callback_) {
        toolkit::EventPollerPool::Instance().getPoller()->async([this_ptr = shared_from_this(), request = std::move(request)]() {
            this_ptr->on_keep_alive_callback_(this_ptr, request);
        });
        // 广播通知心跳消息
        toolkit::NoticeCenter::Instance().emitEvent(Broadcast::kEventSubKeepalive, shared_from_this(), request);
    }
    return 200;
}
void SubordinatePlatformImpl::on_device_info(
    std::shared_ptr<DeviceInfoMessageRequest> request,
    std::function<void(std::shared_ptr<MessageBase>)> &&reply) {

    auto reply_ptr = std::make_shared<decltype(reply)>(std::move(reply));
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
    if (toolkit::NoticeCenter::Instance().emitEvent(Broadcast::kEventOnDeviceInfoRequest, shared_from_this(), request, [do_cb, time_out_task](std::shared_ptr<DeviceInfoMessageResponse> response) {
        time_out_task->cancel();
        do_cb(response);
    }) == 0) {
        WarnL << "DeviceInfo not Implementation";
        do_cb(nullptr);
        time_out_task->cancel();
    }
}
void SubordinatePlatformImpl::on_device_status(
    std::shared_ptr<DeviceStatusMessageRequest> request,
    std::function<void(std::shared_ptr<MessageBase>)> &&reply) {

    auto reply_ptr = std::make_shared<decltype(reply)>(std::move(reply));
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
    if (toolkit::NoticeCenter::Instance().emitEvent(Broadcast::kEventOnDeviceInfoRequest, shared_from_this(), request, [do_cb, time_out_task](std::shared_ptr<DeviceStatusMessageResponse> response) {
        time_out_task->cancel();
        do_cb(response);
    }) == 0) {
        WarnL << "DeviceStatus not Implementation";
        do_cb(nullptr);
        time_out_task->cancel();
    }
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