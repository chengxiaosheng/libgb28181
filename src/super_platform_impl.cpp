#include "super_platform_impl.h"

#include "gb28181/message/device_status_message.h"
#include "gb28181/message/message_base.h"
#include "gb28181/request/request_proxy.h"
#include "inner/sip_common.h"
#include "request/RequestProxyImpl.h"

#include <Poller/EventPoller.h>
#include <Util/NoticeCenter.h>
#include <gb28181/sip_common.h>
#include <inner/sip_server.h>
#include <sip-uac.h>
#include <sip-uas.h>

using namespace gb28181;

SuperPlatformImpl::SuperPlatformImpl(super_account account, const std::shared_ptr<SipServer> &server)
    : SuperPlatform()
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

SuperPlatformImpl::~SuperPlatformImpl() {
    shutdown();
}
void SuperPlatformImpl::shutdown() {}

bool SuperPlatformImpl::update_local_via(std::string host, uint16_t port) {
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
                    Broadcast::kEventSuperPlatformContactChanged, this_ptr, this_ptr->account_.local_host,
                    this_ptr->account_.local_port);
            },
            false);
    }
    return changed;
}
void SuperPlatformImpl::on_invite(
    const std::shared_ptr<InviteRequest> &invite_request,
    std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) {
    resp(400, nullptr);
}

int SuperPlatformImpl::on_query(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {

    switch (message.command()) {
        case MessageCmdType::DeviceStatus: return on_device_status_query(std::move(message), transaction, request);
        case MessageCmdType::Catalog: break;
        case MessageCmdType::DeviceInfo: break;
        case MessageCmdType::RecordInfo: break;
        case MessageCmdType::ConfigDownload: break;
        case MessageCmdType::PresetQuery: break;
        case MessageCmdType::HomePositionQuery: break;
        case MessageCmdType::CruiseTrackListQuery: break;
        case MessageCmdType::CruiseTrackQuery: break;
        case MessageCmdType::PTZPosition: break;
        case MessageCmdType::SDCardStatus: break;
        default: return 404;
    }
    return 404;
}
int SuperPlatformImpl::on_control(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {

    return 200;
}
int SuperPlatformImpl::on_notify(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    // 从上级平台发来的notify ， 应该只有语音广播

    return 200;
}

int SuperPlatformImpl::on_device_status_query(
    MessageBase &&message, const std::shared_ptr<sip_uas_transaction_t> &transaction, std::shared_ptr<sip_message_t> request) {
    auto message_ptr = std::make_shared<DeviceStatusMessageRequest>(std::move(message));
    if (!message_ptr->load_from_xml()) {
        set_message_reason(transaction.get(), message_ptr->get_error().c_str());
        return 400;
    }

    auto flag = std::make_shared<std::atomic_flag>();
    auto timeout = std::shared_ptr<toolkit::EventPoller::DelayTask::Ptr>(nullptr);
    auto response = [message_ptr, weak_this = weak_from_this(), timeout,
                     flag](std::shared_ptr<DeviceStatusMessageResponse> response) -> void {
        if (flag->test_and_set()) {
            return;
        }
        if (timeout && *timeout) {
            (*timeout)->cancel();
        }
        if (auto this_ptr = weak_this.lock()) {
            response->sn(message_ptr->sn());
            auto proxy = std::make_shared<RequestProxyImpl>(this_ptr, response, RequestProxy::RequestType::NoResponse);
            proxy->send([message_ptr](const std::shared_ptr<RequestProxy> &proxy) {
                std::stringstream ss;
                ss << proxy->status();
                DebugL << "response " << *message_ptr << ", ret = " << proxy->status() << ", err = " << proxy->error();
            });
        }
    };

    if (event_define_ && event_define_->on_device_status_query) {
        event_define_->on_device_status_query(shared_from_this(), message_ptr, response);
    } else if (
        0
        == toolkit::NoticeCenter::Instance().emitEvent(
            Broadcast::kEventOnDeviceStatusRequest, shared_from_this(), message_ptr, response)) {
        return 404;
    }
    *timeout = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(
    GB28181_QUERY_TIMEOUT_MS, [device_id = message_ptr->device_id().value_or(""), response]() {
        auto ptr = std::make_shared<DeviceStatusMessageResponse>(device_id, ResultType::Error);
        ptr->reason() = "timeout";
        response(ptr);
        return 0;
    });
    return 200;
}

/**********************************************************************************************************
文件名称:   super_platform_impl.cpp
创建时间:   25-2-11 下午5:00
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午5:00

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午5:00       描述:   创建文件

**********************************************************************************************************/