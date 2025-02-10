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


class UasMessageHandler : public UasMessage, public std::enable_shared_from_this<UasMessageHandler> {
public:
    UasMessageHandler(const std::shared_ptr<SipSession> &session, const std::shared_ptr<SubordinatePlatformImpl> &platform);
    int run(MessageBase && request, std::shared_ptr<sip_uas_transaction_t> transaction, const std ::shared_ptr<sip_message_t> &sip);
    void response(std::shared_ptr<MessageBase> response, const std::function<void(bool, std::string)> &rcb, bool end = true);

    std::shared_ptr<MessageBase> get_request() override {
        return request_;
    }
private:
    std::shared_ptr<SipSession> session_; // 收到请求的网络session
    std::shared_ptr<SubordinatePlatformImpl> platform_; // 目标平台指针
    std::shared_ptr<MessageBase> request_; // 请求消息
    std::string from_;
    std::string to_;
    bool has_response_{false}; // 是否需要response消息
    std::atomic_bool completed_; // 是否已经完成
    bool is_mutil{false}; // 是否多消息返回

};

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