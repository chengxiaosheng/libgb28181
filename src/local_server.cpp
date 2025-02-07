#include "gb28181/local_server.h"

#include <Util/logger.h>
#include <gb28181/subordinate_platform.h>
#include <gb28181/super_platform.h>
#include <handler/uas_register_handler.h>
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
    UasRegisterHandler::Init();
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

void LocalServer::get_subordinate_platform(
    const std::string &platform_id, const std::function<void(std::shared_ptr<SubordinatePlatform>)> &cb) {
        {
            std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
            if (auto it = sub_platforms_.find(platform_id); it != sub_platforms_.end()) {
                return cb(it->second);
            }
        }
    auto weak_this = weak_from_this();
    if (find_sub_platform_callback_) {
        find_sub_platform_callback_(platform_id, [weak_this, platform_id, cb](const std::shared_ptr<SubordinatePlatform>& platform) {
            if (platform == nullptr) {
                return cb(platform);
            }
            if (auto this_ptr = weak_this.lock()) {
                std::unique_lock<decltype(platform_mutex_)> lock(this_ptr->platform_mutex_);
                if (auto it = this_ptr->sub_platforms_.find(platform_id); it != this_ptr->sub_platforms_.end()) {
                    return cb(it->second);
                }
                this_ptr->sub_platforms_.emplace(platform_id, platform);
                return cb(platform);
            }
            return cb(nullptr);
        });
    } else return cb(nullptr);
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