#include "uas_register_handler.h"
#include "../../3rdpart/media-server/libsip/src/uas/sip-uas-transaction.h"
#include "inner/sip_event.h"
#include "inner/sip_session.h"
#include "sip-uas.h"

#include <Util/NoticeCenter.h>
#include <gb28181/local_server.h>
#include <gb28181/subordinate_platform.h>
#include <inner/sip_server.h>

using namespace gb28181;

static void *uas_register_handler_tag = nullptr;

static std::unordered_map<std::string,std::shared_ptr<SubordinatePlatform>> registering_platforms_;
static std::mutex registering_platforms_mtx_;

static void on_register(EventOnRegisterArgs) {

    std::shared_ptr<SubordinatePlatform> platform_ptr;
    {
        std::unique_lock<std::mutex> lock(registering_platforms_mtx_);
        if (auto it = registering_platforms_.find(user); it != registering_platforms_.end()) {
            platform_ptr = it->second;
        }
    }
    LocalServer* server_ptr;
    if (!platform_ptr) {
        auto sip_server = session->getSipServer();
        if (!sip_server) {
            sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
            return;
        }
        server_ptr = sip_server->get_server();
        if (!server_ptr) {
            sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
            return;
        }
    }












}

void UasRegisterHandler::Init() {
    static toolkit::onceToken token(
        []() {
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