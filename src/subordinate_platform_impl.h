#ifndef gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#define gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#include "gb28181/request/subscribe_request.h"
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
class SubscribeRequest;
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

    static int on_recv_register(
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, const std::string &user, const std::string &location, int expires);

    void shutdown() override;

    void set_encoding(CharEncodingType encoding) override { account_.encoding = std::move(encoding); }

    std::shared_ptr<LocalServer> get_local_server() const override;

    CharEncodingType get_encoding() const override { return account_.encoding; }

    std::shared_ptr<SubordinatePlatformImpl> shared_from_this() {
        return std::dynamic_pointer_cast<SubordinatePlatformImpl>(PlatformHelper::shared_from_this());
    }
    std::weak_ptr<SubordinatePlatformImpl> weak_from_this() { return shared_from_this(); }

    const subordinate_account &account() const override { return account_; }

    platform_account &sip_account() override { return *(platform_account *)&account_; }

    void set_status(PlatformStatusType status, std::string error);

    void query_config(
        const std::string &device_id, DeviceConfigType config_type,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void query_preset(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_config(
        const std::string &device_id, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> &&config,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;

    int on_keep_alive(std::shared_ptr<KeepaliveMessageRequest> request);


    int on_notify(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction,
        std::shared_ptr<sip_message_t> request) override;

    void on_invite(
        const std::shared_ptr<InviteRequest> &invite_request,
        std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) override;

    void
    query_device_status(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;

    void query_catalog(
        const std::string &device_id, RequestProxy::ResponseCallback data_callback,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;

    void
    query_device_info(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;

    void query_record_info(
        const std::shared_ptr<RecordInfoRequestMessage> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void
    query_home_position(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void
    query_cruise_list(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void query_cruise(
        const std::string &device_id, int32_t number, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void
    query_ptz_position(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void
    query_sd_card_status(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_ptz(
        const std::string &device_id, PTZCommand ptz_cmd, std::string name,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_tele_boot(
        const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_record_cmd(
        const std::string &device_id, RecordType type, int stream_number,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_guard_cmd(
        const std::string &device_id, GuardType type, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_alarm_cmd(
        const std::string &device_id, std::optional<AlarmCmdInfoType> info,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_i_frame_cmd(
        const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_drag_zoom_in(
        const std::string &device_id, DragZoomType drag,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_drag_zoom_out(
        const std::string &device_id, DragZoomType drag,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_home_position(
        const std::shared_ptr<DeviceControlRequestMessage_HomePosition> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_ptz_precise_ctrl(
        std::shared_ptr<DeviceControlRequestMessage_PtzPreciseCtrl> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_device_upgrade(
        const std::shared_ptr<DeviceControlRequestMessage_DeviceUpgrade> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_format_sd_card(
        const std::string &device_id, int index, std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;
    void device_control_target_track(
        const std::shared_ptr<DeviceControlRequestMessage_TargetTrack> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb) override;

    std::shared_ptr<InviteRequest>
    invite(const std::shared_ptr<SdpDescription> &sdp, const std::string &device_id) override;

    std::shared_ptr<SubscribeRequest>
    subscribe(const std::shared_ptr<MessageBase> &request, SubscribeRequest::subscribe_info info) override;

    void camouflage_online(uint64_t register_time, uint64_t keepalive_time) override;

private:
    bool camouflage_online_ = false; // 伪装在线
    TransportType get_transport() const override { return account_.transport_type; }


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