#ifndef gb28181_src_request_REQUESTPROXYIMPL_H
#define gb28181_src_request_REQUESTPROXYIMPL_H
#include "../../include/gb28181/request/request_proxy.h"

#include <Network/Buffer.h>
#include <atomic>
#include <variant>

namespace gb28181 {
class PlatformHelper;
}
namespace toolkit {
class Timer;
}
namespace gb28181 {
class SuperPlatformImpl;
class SubordinatePlatformImpl;
} // namespace gb28181
namespace gb28181 {
class SipSession;
}

#ifdef __cplusplus
extern "C" {
struct sip_agent_t;
struct sip_uac_transaction_t;
struct sip_message_t;
struct sip_uas_transaction_t;
}
#endif

namespace gb28181 {
class RequestProxyImpl
    : public RequestProxy
    , public std::enable_shared_from_this<RequestProxyImpl> {
public:
    RequestProxyImpl(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request,
        RequestType type, int32_t sn = 0);
    RequestProxyImpl(
    const std::shared_ptr<SuperPlatformImpl> &platform, const std::shared_ptr<MessageBase> &request,
    RequestType type, int32_t sn = 0);
    ~RequestProxyImpl() override;
    void set_sn(int sn) { request_sn_ = sn; }
    Status status() const override { return status_; }
    RequestType type() const override { return request_type_; }
    std::string error() const override { return error_; }
    uint64_t send_time() const override { return send_time_; }
    uint64_t reply_time() const override { return reply_time_; }
    uint64_t response_begin_time() const override { return response_end_time_; }
    uint64_t response_end_time() const override { return response_end_time_; }
    void set_reply_callback(ReplyCallback cb) override { reply_callback_ = std::move(cb); }
    void set_response_callback(ResponseCallback cb) override { response_callback_ = std::move(cb); }
    void send(std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    const std::vector<std::shared_ptr<MessageBase>> &all_response() const override { return responses_; }
    void set_sip_session(const std::shared_ptr<SipSession> &session_ptr) { send_session_ = session_ptr; }
    friend std::ostream &operator<<(std::ostream &os, const RequestProxyImpl &proxy);

protected:
    virtual int on_response(const std::shared_ptr<MessageBase> &response);

    void on_reply(std::shared_ptr<sip_message_t> sip_message, int code);

    void on_completed();

    virtual void on_reply_l() {}

    virtual void on_completed_l() {}

protected:
    uint64_t send_time_ { 0 };
    uint64_t reply_time_ { 0 };
    uint64_t response_begin_time_ { 0 };
    uint64_t response_end_time_ { 0 };
    std::vector<std::shared_ptr<MessageBase>> responses_;
    std::shared_ptr<PlatformHelper> platform_;
    std::shared_ptr<MessageBase> request_;
    std::shared_ptr<SipSession> send_session_;
    std::shared_ptr<sip_uac_transaction_t> uac_transaction_;
    std::string error_;
    int reply_code_ { 0 };
    int request_sn_ { 0 };
    Status status_ { Init };
    RequestType request_type_ { RequestType::invalid };
    std::atomic_flag result_flag_ = ATOMIC_FLAG_INIT;
    std::function<void(std::shared_ptr<RequestProxy>)> rcb_; // 结果回调
    ReplyCallback reply_callback_; // 确认回调
    ResponseCallback response_callback_; // 应答回调

private:
    std::shared_ptr<toolkit::Timer> timer_; // 等待应答超时
    int on_response(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction,
        std::shared_ptr<sip_message_t> request);
    static int on_recv_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code);
    friend class PlatformHelper;
};
} // namespace gb28181

#endif // gb28181_src_request_REQUESTPROXYIMPL_H

/**********************************************************************************************************
文件名称:   RequestProxyImpl.h
创建时间:   25-2-11 下午12:44
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午12:44

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午12:44       描述:   创建文件

**********************************************************************************************************/