#ifndef gb28181_src_request_SUBSCRIBE_REQUEST_IMPL_H
#define gb28181_src_request_SUBSCRIBE_REQUEST_IMPL_H
#include "RequestConfigDownloadImpl.h"
#include "gb28181/request/subscribe_request.h"
#include "sip-subscribe.h"
#include <Poller/EventPoller.h>
#include <memory>
#include <string>
#include <variant>

#if defined(__cplusplus)
namespace gb28181 {
class SuperPlatformImpl;
}
extern "C" {
struct sip_subscribe_t;
struct sip_uac_transaction_t;
struct sip_uas_transaction_t;
}
#endif

namespace gb28181 {
class MessageBase;
}
namespace gb28181 {
class SubordinatePlatformImpl;
}
namespace gb28181 {
class SubscribeRequestImpl
    : public SubscribeRequest
    , public std::enable_shared_from_this<SubscribeRequestImpl> {
public:
    SubscribeRequestImpl(
        const std::shared_ptr<SubordinatePlatformImpl> &platform, const std::shared_ptr<MessageBase> &message,
        subscribe_info &&info);
    SubscribeRequestImpl(
        const std::shared_ptr<SuperPlatformImpl> &platform, const std::shared_ptr<sip_subscribe_t> &subscribe_t);
    ~SubscribeRequestImpl() override;

    void set_status_callback(SubscriberStatusCallback cb) override { subscriber_callback_ = std::move(cb); }
    void set_notify_callback(SubscriberNotifyCallback cb) override { notify_callback_ = std::move(cb); }
    void start() override;
    void shutdown(std::string reason) override;

    SUBSCRIBER_STATUS_TYPE_E status() const override { return status_; }
    const std::string &error() const override { return error_; }
    int time_remain() const override;
    bool is_incoming() const override { return incoming_; }

    void send_notify(const std::shared_ptr<MessageBase> &message, SubscriberNotifyReplyCallback rcb) override;

    static std::shared_ptr<SubscribeRequestImpl> get_subscribe(void *ptr);

    static int recv_subscribe_request(
        const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &message,
        const std::shared_ptr<sip_uas_transaction_t> &transaction,
        const std::shared_ptr<struct sip_subscribe_t> &subscribe_ptr, void **sub);

    /**
     * 收到一个通知
     * @param notify
     * @param transaction
     * @return
     */
    int on_recv_notify(
        const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &notify,
        const std::shared_ptr<sip_uas_transaction_t> &transaction);

protected:
    void set_status(SUBSCRIBER_STATUS_TYPE_E status, TERMINATED_TYPE_E terminated_type, std::string reason);
    void to_subscribe(uint32_t expires);

private:
    // 订阅请求的回复
    static int on_subscribe_reply(
        void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t,
        struct sip_subscribe_t *subscribe, int code, void **session);

    void add_subscribe();
    void del_subscribe();

    void subscribe_send_notify(
        const std::shared_ptr<MessageBase> &message, SubscriberNotifyReplyCallback rcb, std::string state = "");

    int on_subscribe(
        const std::shared_ptr<sip_message_t> &message, const std::shared_ptr<sip_uas_transaction_t> &transaction);

    std::string get_notify_state_str() const;

private:
    std::variant<std::shared_ptr<SubordinatePlatformImpl>, std::shared_ptr<SuperPlatformImpl>>
        platform_; // 订阅所属平台指针
    std::shared_ptr<MessageBase> subscribe_message_; // 订阅请求的
    std::string error_; // 错误信息
    uint64_t subscribe_time_ { 0 }; // 订阅成功时间
    uint64_t terminated_time_ { 0 }; // 订阅终止时间
    subscribe_info subscribe_info_ {}; // 订阅基础信息
    SUBSCRIBER_STATUS_TYPE_E status_ { terminated }; // 订阅状态
    TERMINATED_TYPE_E terminated_type_ { invalid }; // 终止状态
    bool incoming_ { false }; // 是否为传入
    std::atomic<bool> running_ { false }; // 是否正在运行
    std::shared_ptr<sip_subscribe_t> sip_subscribe_ptr_; // sip订阅指针
    SubscriberStatusCallback subscriber_callback_; // 订阅状态回调
    SubscriberNotifyCallback notify_callback_; // 收到通知的回调
    toolkit::EventPoller::DelayTask::Ptr delay_task_; // 延迟任务指针
};
} // namespace gb28181

#endif // gb28181_src_request_SUBSCRIBE_REQUEST_IMPL_H

/**********************************************************************************************************
文件名称:   subscribe_request_impl.h
创建时间:   25-2-15 上午10:54
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-15 上午10:54

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-15 上午10:54       描述:   创建文件

**********************************************************************************************************/