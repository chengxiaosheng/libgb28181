#include "subordinate_platform_impl.h"

#include <inner/sip_session.h>

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