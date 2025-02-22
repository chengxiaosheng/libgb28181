#ifndef gb28181_include_gb28181_SUBORDINATE_PLATFORM_H
#define gb28181_include_gb28181_SUBORDINATE_PLATFORM_H
#include "gb28181/type_define.h"
#include "request/request_proxy.h"

#include <functional>
namespace gb28181 {
class SdpDescription;
class InviteRequest;
class DeviceControlRequestMessage_TargetTrack;
}
namespace gb28181 {
class DeviceControlRequestMessage_DeviceUpgrade;
}
namespace gb28181 {
class DeviceControlRequestMessage_PtzPreciseCtrl;
}
namespace gb28181 {
class DeviceControlRequestMessage_HomePosition;
}
namespace gb28181 {
class RecordInfoRequestMessage;
}
namespace gb28181 {
class DeviceInfoMessageRequest;
}
namespace gb28181 {
class SubordinatePlatform {
public:
    virtual ~SubordinatePlatform() = default;
    virtual void shutdown() = 0;
    /**
     * @brief  获取账户信息
     * @return
     */
    virtual const subordinate_account &account() const = 0;

    virtual void set_encoding(CharEncodingType encoding) = 0;

    /**
     * A.2.4.2 设备状态查询请求
     * @param device_id
     * @param rcb
     */
    virtual void
    query_device_status(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;
    /**
     * A.2.4.3 目录查询
     * @param device_id
     * @param data_callback
     * @param rcb
     */
    virtual void query_catalog(
        const std::string &device_id, RequestProxy::ResponseCallback data_callback,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.4 设备信息查询
     * @param device_id
     * @param rcb
     */
    virtual void query_device_info(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.5 文件目录检索请求
     * @param req
     * @param rcb
     */
    virtual void query_record_info(
        const std::shared_ptr<RecordInfoRequestMessage> &req, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.7 设备配置查询请求
     * @param device_id
     * @param config_type
     * @param rcb
     */
    virtual void query_config(
        const std::string &device_id, DeviceConfigType config_type,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.8 设备预置位查询请求
     * @param device_id
     * @param rcb
     */
    virtual void query_preset(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb) = 0;

    /**
     * A.2.4.10 看守位信息查询请求
     * @param device_id
     * @param rcb
     */
    virtual void
    query_home_position(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.11 循环轨迹列表查询请求
     * @param device_id
     * @param rcb
     */
    virtual void query_cruise_list(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.12 循环轨迹查询请求
     * @param device_id
     * @param number
     * @param rcb
     */
    virtual void
    query_cruise(const std::string &device_id, int32_t number, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.13 PTZ 精准状态查询请求
     * @param device_id
     * @param rcb
     */
    virtual void
    query_ptz_position(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.4.14 存储卡状态查询请求
     * @param device_id
     * @param rcb
     */
    virtual void
    query_sd_card_status(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.2 摄像机云台控制命令
     * @param device_id
     * @param ptz_cmd
     * @param name 预置位名称或巡航轨迹名称
     * @param rcb
     */
    virtual void device_control_ptz(
        const std::string &device_id, PtzCmdType ptz_cmd, std::string name,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.3 远程启动控制命令
     * @param device_id
     * @param rcb
     */
    virtual void
    device_control_tele_boot(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.4 录像控制命令
     * @param device_id
     * @param type
     * @param stream_number
     * @param rcb
     */
    virtual void device_control_record_cmd(
        const std::string &device_id, RecordType type, int stream_number,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.5 报警布防/撤防控制命令
     * @param device_id
     * @param type
     * @param rcb
     */
    virtual void device_control_guard_cmd(
        const std::string &device_id, GuardType type, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.6 报警复位控制命令
     * @param device_id
     * @param info
     * @param rcb
     */
    virtual void device_control_alarm_cmd(
        const std::string &device_id, std::optional<AlarmCmdInfoType> info,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.7 强制关键帧控制命令
     * @param device_id
     * @param rcb
     */
    virtual void
    device_control_i_frame_cmd(const std::string &device_id, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.8 拉框放大控制命令
     * @param device_id
     * @param drag
     * @param rcb
     */
    virtual void device_control_drag_zoom_in(
        const std::string &device_id, DragZoomType drag, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.9 拉框缩小控制命令
     * @param device_id
     * @param drag
     * @param rcb
     */
    virtual void device_control_drag_zoom_out(
        const std::string &device_id, DragZoomType drag, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     *A.2.3.1.10 看守位控制命令
     * @param req
     * @param rcb
     */
    virtual void device_control_home_position(
        const std::shared_ptr<DeviceControlRequestMessage_HomePosition> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.11  PTZ精准控制控制命令
     * @param req
     * @param rcb
     */
    virtual void device_control_ptz_precise_ctrl(
        std::shared_ptr<DeviceControlRequestMessage_PtzPreciseCtrl> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     *A.2.3.1.12 设备软件升级控制命令
     * @param req
     * @param rcb
     */
    virtual void device_control_device_upgrade(
        const std::shared_ptr<DeviceControlRequestMessage_DeviceUpgrade> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     *A.2.3.1.13 存储卡格式化控制命令
     * @param device_id
     * @param index
     * @param rcb
     */
    virtual void device_control_format_sd_card(
        const std::string &device_id, int index, std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.1.14 目标跟踪控制命令
     * @param req
     * @param rcb
     */
    virtual void device_control_target_track(
        const std::shared_ptr<DeviceControlRequestMessage_TargetTrack> &req,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;

    /**
     * A.2.3.2 设备配置命令
     * @param device_
     * @param config
     * @param rcb
     */
    virtual void device_config(
        const std::string &device_, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> &&config,
        std::function<void(std::shared_ptr<RequestProxy>)> rcb)
        = 0;
    /**
     * 创建一个媒体请求
     * @param sdp
     * @return
     */
    virtual std::shared_ptr<InviteRequest> invite(const std::shared_ptr<SdpDescription> &sdp) = 0;

    /**
     * 创建一个订阅请求
     * @param request
     * @param info
     * @return
     */
    virtual std::shared_ptr<SubscribeRequest>
    subscribe(const std::shared_ptr<MessageBase> &request, SubscribeRequest::subscribe_info info) = 0;
};
} // namespace gb28181

#endif // gb28181_include_gb28181_SUBORDINATE_PLATFORM_H

/**********************************************************************************************************
文件名称:   subordinate_platform.h
创建时间:   25-2-6 上午9:57
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午9:57

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午9:57       描述:   创建文件

**********************************************************************************************************/