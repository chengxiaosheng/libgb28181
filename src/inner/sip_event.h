#ifndef gb28181_src_inner_SIP_EVENT_H
#define gb28181_src_inner_SIP_EVENT_H

#include "string"

namespace gb28181 {

constexpr const char kEventOnRegister[] = "kEventOnRegister";
/**
 * @param session 网络session
 * @param transaction uas事务 异步安全
 * @param req request 请求 异步安全
 * @param user 对端用户
 * @param location
 * @param expires 过期时间
 */
#define EventOnRegisterArgs                                                                                            \
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,             \
        const std::shared_ptr<sip_message_t> &req, const std::string &user, const std::string &location, int expires

constexpr const char kEventOnRequest[] = "EventOnMessage";

/**
 * @param session 网络session
 * @param transaction uas事务 异步安全
 * @param req request 请求 异步安全
 * @param dialog_ptr dialog 的用户数据
 */
#define EventOnMessageArgs                                                                                             \
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,             \
        const std::shared_ptr<sip_message_t> &req, void *dialog_ptr

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