#ifndef gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#define gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#include "gb28181/subordinate_platform.h"
#include "gb28181/type_define.h"
#include "platform_helper.h"

#include <functional>
#include <memory>
#include <mutex>

namespace toolkit {
class SockException;
}
namespace gb28181 {
class SipServer;
class RequestProxyImpl;
class MessageBase;
class DeviceStatusMessageRequest;
class DeviceInfoMessageResponse;
class DeviceInfoMessageRequest;
class KeepaliveMessageRequest;
class DeviceStatusMessageResponse;

} // namespace gb28181

namespace toolkit {
class Timer;
}

namespace gb28181 {
class SipSession;
class LocalServer;

class SubordinatePlatformImpl final
    : public SubordinatePlatform
    , public PlatformHelper {
public:
    explicit SubordinatePlatformImpl(subordinate_account account, const std::shared_ptr<SipServer> &server);
    ~SubordinatePlatformImpl() override;

    void shutdown() override;

    void set_encoding(CharEncodingType encoding) override { account_.encoding = std::move(encoding); }

    std::shared_ptr<SubordinatePlatformImpl> shared_from_this() {
        return std::dynamic_pointer_cast<SubordinatePlatformImpl>(PlatformHelper::shared_from_this());
    }
    std::weak_ptr<SubordinatePlatformImpl> weak_from_this() { return shared_from_this(); }

    inline const subordinate_account &account() const override { return account_; }
    gb28181::sip_account &sip_account() const override { return *(gb28181::sip_account *)&account_; }

    void set_status(PlatformStatusType status, std::string error);

    void
    query_device_info(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void
    query_device_status(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void query_config(
        const std::string &device_id, DeviceConfigType config_type,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void query_preset(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_config(
        const std::string &device_id, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> &&config,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;

    bool update_local_via(std::string host, uint16_t port) override;

public:
    int on_keep_alive(std::shared_ptr<KeepaliveMessageRequest> request);
    void on_device_info(
        std::shared_ptr<DeviceInfoMessageRequest> request, std::function<void(std::shared_ptr<MessageBase>)> &&reply);
    void on_device_status(
        std::shared_ptr<DeviceStatusMessageRequest> request, std::function<void(std::shared_ptr<MessageBase>)> &&reply);

    int on_notify(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction,
        std::shared_ptr<sip_message_t> request) override;

    void on_invite(
        const std::shared_ptr<InviteRequest> &invite_request,
        std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) override;

private:
    subordinate_account account_; // 账户信息
    // 心跳检测
    std::shared_ptr<toolkit::Timer> keepalive_timer_;
    std::function<void(std::shared_ptr<SubordinatePlatform>, std::shared_ptr<KeepaliveMessageRequest>)>
        on_keep_alive_callback_;
};

} // namespace gb28181

#endif // gb28181_src_SUBORDINATE_PLATFORM_IMPL_H

/**********************************************************************************************************
文件名称:   subordinate_platform_impl.h
创建时间:   25-2-7 下午3:11
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午3:11

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午3:11       描述:   创建文件

**********************************************************************************************************/