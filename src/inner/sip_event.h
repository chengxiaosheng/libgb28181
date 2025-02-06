#ifndef gb28181_src_inner_SIP_EVENT_H
#define gb28181_src_inner_SIP_EVENT_H

#include "string"

namespace gb28181 {

constexpr const char kEventOnRegister[] = "kEventOnRegister";
#define EventOnRegisterArgs                                                                                            \
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,             \
        const std::string &user, const std::string &location, int expires

} // namespace gb28181

#endif // gb28181_src_inner_SIP_EVENT_H

/**********************************************************************************************************
文件名称:   sip_event.h
创建时间:   25-2-6 下午6:21
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 下午6:21

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 下午6:21       描述:   创建文件

**********************************************************************************************************/