#ifndef gb28181_src_handler_UAS_MESSAGE_HANDLER_H
#define gb28181_src_handler_UAS_MESSAGE_HANDLER_H

#include <functional>
#include <gb28181/message/message_base.h>
#include <gb28181/message/uas_message.h>
#include <memory>
#include <string>

#ifdef __cplusplus
namespace gb28181 {
class SubordinatePlatformImpl;
}
extern "C" {
struct sip_uas_handler_t;
struct sip_message_t;
struct sip_uas_transaction_t;
struct sip_dialog_t;
}
#endif

namespace gb28181 {
class SipSession;

int on_uas_register(
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, const std::string &user, const std::string &location, int expires);

/**
 * 收到Message消息
 * @param session 网络session
 * @param transaction 事务指针
 * @param req sip请求
 * @param dialog_ptr 会话指针
 * @return
 */
int on_uas_message(
    const std::shared_ptr<SipSession> &session, const std ::shared_ptr<sip_uas_transaction_t> &transaction,
    const std ::shared_ptr<sip_message_t> &req, void *dialog_ptr);

} // namespace gb28181

#endif // gb28181_src_handler_UAS_MESSAGE_HANDLER_H

/**********************************************************************************************************
文件名称:   uas_message_handler.h
创建时间:   25-2-7 下午6:53
作者名称:   Kevin
文件路径:   src/handler
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午6:53

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午6:53       描述:   创建文件

**********************************************************************************************************/