#ifndef gb28181_src_SUPER_PLATFORM_IMPL_H
#define gb28181_src_SUPER_PLATFORM_IMPL_H

#include "gb28181/super_platform.h"
#include <Poller/EventPoller.h>
#include <gb28181/type_define.h>
#include <memory>
#include <platform_helper.h>

namespace toolkit {
class Ticker;
}
namespace toolkit {
class Timer;
}
namespace gb28181 {
class SipServer;
class SuperPlatformImpl final
    : public SuperPlatform
    , public PlatformHelper {
public:
    SuperPlatformImpl(super_account account, const std::shared_ptr<SipServer> &server);
    void shutdown() override;
    void start() override;
    ~SuperPlatformImpl() override;
    std::shared_ptr<SuperPlatformImpl> shared_from_this() {
        return std::dynamic_pointer_cast<SuperPlatformImpl>(PlatformHelper::shared_from_this());
    }
    std::weak_ptr<SuperPlatformImpl> weak_from_this() { return shared_from_this(); }

    const super_account &account() const override { return account_; }
    void set_encoding(CharEncodingType encoding) override { account_.encoding = std::move(encoding); }
    platform_account &sip_account()  override { return *(platform_account *)&account_; }

    std::string get_to_uri() override;
    CharEncodingType get_encoding() const override { return account_.encoding; }

    void on_invite(
        const std::shared_ptr<InviteRequest> &invite_request,
        std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) override;

    int on_query(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction,
        std::shared_ptr<sip_message_t> request) override;

    int on_control(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction,
        std::shared_ptr<sip_message_t> request) override;

    int on_notify(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction,
        std::shared_ptr<sip_message_t> request) override;

private:
    void to_register(int expires, const std::string &authorization = "");
    void to_keepalive();

    static int
    on_register_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code);
    void set_status(PlatformStatusType status, const std::string& error);

    TransportType get_transport() const override {
        return account_.transport_type;
    }
private:
    super_account account_;
    std::string moved_uri_;
    std::pair<std::string, int> nc_pair_;
    int auth_failed_count_ { 0 };
    std::atomic_bool running_ { false };
    std::shared_ptr<toolkit::Timer> keepalive_timer_; // 心跳定时器
    std::shared_ptr<toolkit::EventPoller::DelayTask> register_timer_;
    std::vector<std::string> fault_devices_;
    std::shared_ptr<toolkit::Ticker> keepalive_ticker_;
};
} // namespace gb28181

#endif // gb28181_src_SUPER_PLATFORM_IMPL_H

/**********************************************************************************************************
文件名称:   super_platform_impl.h
创建时间:   25-2-11 下午5:00
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午5:00

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午5:00       描述:   创建文件

**********************************************************************************************************/