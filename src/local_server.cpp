#include "gb28181/local_server.h"

#include <Util/logger.h>
#include <gb28181/subordinate_platform.h>
#include <gb28181/super_platform.h>
#include <inner/sip_server.h>
#include <sip-timer.h>
using namespace gb28181;

LocalServer::LocalServer(sip_account account)
    : account_(std::move(account)) {
    static toolkit::onceToken token([]() {
        sip_timer_init();
    },[]() {
        sip_timer_cleanup();
    });
}

void LocalServer::run() {
    if (running_.exchange(true)) {
        WarnL << "the local server " << account_.platform_id << " is already running";
        return;
    }
    InfoL << "begin run local server " << account_.platform_id << "...";
    if (!server_) {
        server_ = std::make_shared<SipServer>(this);
    }
    server_->start(account_.port, account_.host.empty() ? "::" : account_.host.c_str(), (SipServer::Protocol)transport_type_);
    InfoL << "local server " << account_.platform_id << " running";

}
void LocalServer::shutdown() {
    if (!running_) return;
    InfoL << "begin shutdown local server " << account_.platform_id << "...";
    for (auto &it : super_platforms_) {
        it.second->shutdown();
    }
    for (auto &it : sub_platforms_) {
        it.second->shutdown();
    }
    server_.reset();
    running_.exchange(false);
    InfoL << "shutdown local server " << account_.platform_id;
}
 LocalServer::~LocalServer() {
    shutdown();
}




/**********************************************************************************************************
文件名称:   local_server.cpp
创建时间:   25-2-6 上午10:17
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午10:17

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午10:17       描述:   创建文件

**********************************************************************************************************/