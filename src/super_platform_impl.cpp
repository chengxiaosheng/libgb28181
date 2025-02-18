#include "super_platform_impl.h"

#include <Poller/EventPoller.h>
#include <Util/NoticeCenter.h>
#include <gb28181/sip_common.h>
#include <inner/sip_server.h>

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
    from_uri_ = "sip:" + get_sip_server()->get_account().platform_id + "@" + account_.local_host + ":" + std::to_string(account_.local_port);
}

SuperPlatformImpl::~SuperPlatformImpl() {
    shutdown();
}
void SuperPlatformImpl::shutdown() {

}

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
    std::function<void(int, std::shared_ptr<sdp_description>)> &&resp) {
    resp(400, nullptr);
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