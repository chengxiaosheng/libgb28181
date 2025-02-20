#ifndef gb28181_src_SUPER_PLATFORM_IMPL_H
#define gb28181_src_SUPER_PLATFORM_IMPL_H

#include "gb28181/super_platform.h"
#include <gb28181/type_define.h>
#include <memory>
#include <platform_helper.h>

namespace gb28181 {
class SipServer;
class SuperPlatformImpl final
    : public SuperPlatform
    , public PlatformHelper {
public:
    SuperPlatformImpl(super_account account, const std::shared_ptr<SipServer> &server);
    void shutdown() override;
    ~SuperPlatformImpl() override;
    std::shared_ptr<SuperPlatformImpl> shared_from_this() {
        return std::dynamic_pointer_cast<SuperPlatformImpl>(PlatformHelper::shared_from_this());
    }
    std::weak_ptr<SuperPlatformImpl> weak_from_this() { return shared_from_this(); }

    const super_account &account() const override { return account_; }
    void set_encoding(CharEncodingType encoding) override { account_.encoding = std::move(encoding); }
    gb28181::sip_account &sip_account() const override { return *(gb28181::sip_account *)&account_; }
    bool update_local_via(std::string host, uint16_t port) override;

protected:
    void on_invite(const std::shared_ptr<InviteRequest> &invite_request, std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) override;

    int on_query(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) override;

    int on_control(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) override;

    int on_notify(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) override;

private:
    int on_device_status_query(MessageBase &&message, const std::shared_ptr<sip_uas_transaction_t> &transaction, std::shared_ptr<sip_message_t> request);

private:
    super_account account_;
    std::shared_ptr<event_defined> event_define_;
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