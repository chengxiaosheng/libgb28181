#ifndef gb28181_include_gb28181_SIP_COMMON_H
#define gb28181_include_gb28181_SIP_COMMON_H

namespace gb28181 {

enum SipError : int {
    sip_undefined_error = -1, // 未定义的错误
    sip_bad_parameter = -2, // 参数错误
    sip_wrong_state = -3, // 状态~？
    sip_syntax_error = -5, // 语法错误
    sip_not_found = -6, // 未找到
    sip_not_implemented = -7, // 为实现
    sip_no_network= -10, // 没有网络连接
    sip_port_busy = -11, // 端口被占用或忙碌
    sip_unknown_host = -12, // 未知的主机
    sip_timeout = -50, // 超时
    sip_too_much_call = -51, // call 过多？
    sip_wrong_format = -52, // 格式问题
    sip_retry_limit= -60, // 重试超过限制
    sip_ok = 0, // 无错误
};

#ifndef GB28181_QUERY_TIMEOUT_MS
#define GB28181_QUERY_TIMEOUT_MS 5*1000
#endif

/** is the status code informational */
#define SIP_IS_SIP_INFO(x) (((x) >= 100) && ((x) < 200))
/** is the status code OK ?*/
#define SIP_IS_SIP_SUCCESS(x) (((x) >= 200) && ((x) < 300))
/** is the status code a redirect */
#define SIP_IS_SIP_REDIRECT(x) (((x) >= 300) && ((x) < 400))
/** is the status code a error (client or server) */
#define SIP_IS_SIP_ERROR(x) (((x) >= 400) && ((x) < 600))
/** is the status code a client error  */
#define SIP_IS_SIP_CLIENT_ERROR(x) (((x) >= 400) && ((x) < 500))
/** is the status code a server error  */
#define SIP_IS_SIP_SERVER_ERROR(x) (((x) >= 500) && ((x) < 600))

namespace Broadcast {
//下级平台心跳
constexpr const char kEventSubKeepalive[] = "kEventSubKeepalive";
#define kEventSubKeepaliveArgs std::shared_ptr<SubordinatePlatform> platform , std::shared_ptr<KeepaliveMessageRequest> message

// 下级平台在线状态变更
constexpr const char kEventSubordinatePlatformStatus[] = "kEventSubordinatePlatformStatus";
#define kEventSubordinatePlatformStatusArgs std::shared_ptr<SubordinatePlatform> platform , PlatformStatusType status, const std::string& message

// 上级平台在线状态变更
constexpr const char kEventSuperPlatformStatus[] = "kEventSuperPlatformStatus";
#define kEventSuperPlatformStatusArgs std::shared_ptr<SuperPlatform> platform , PlatformStatusType status, const std::string& message

// 下级平台本地联系信息变更
constexpr const char kEventSubordinatePlatformContactChanged[] = "kEventSubordinatePlatformContactChanged";
#define kEventSubordinatePlatformContactChangedArgs std::shared_ptr<SubordinatePlatform> platform , const std::string& host,uint16_t port

// 上级平台本地联系信息变更
constexpr const char kEventSuperPlatformContactChanged[] = "kEventSuperPlatformContactChanged";
#define kEventSuperPlatformContactChangedArgs std::shared_ptr<SuperPlatform> platform , const std::string& host,uint16_t port



// 收到来自上级平台的 设备状态查询请求
constexpr const char kEventOnDeviceStatusRequest[] = "kEventOnDeviceStatusRequest";
#define kEventOnDeviceStatusRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceStatusMessageRequest> message, std::function<void(std::shared_ptr<DeviceStatusMessageResponse>)> callback

constexpr const char kEventOnDeviceInfoRequest[] = "kEventOnDeviceInfoRequest";
#define kEventOnDeviceInfoRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceInfoMessageRequest> message, std::function<void(std::shared_ptr<DeviceInfoMessageResponse>)> callback

constexpr const char kEventOnHomePositionRequest[] = "kEventOnHomePositionRequest";
#define kEventOnHomePositionRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<HomePositionRequestMessage> message, std::function<void(std::shared_ptr<HomePositionResponseMessage>)> callback

constexpr const char kEventOnCruiseTrackListRequest[] = "kEventOnCruiseTrackListRequest";
#define kEventOnCruiseTrackListRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<CruiseTrackListRequestMessage> message, std::function<void(std::shared_ptr<CruiseTrackListResponseMessage>)> callback

constexpr const char kEventOnPTZPositionRequest[] = "kEventOnPTZPositionRequest";
#define kEventOnPTZPositionRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<PTZPositionRequestMessage> message, std::function<void(std::shared_ptr<PTZPositionResponseMessage>)> callback


constexpr const char kEventOnSdCardStatusRequest[] = "kEventOnSdCardStatusRequest";
#define kEventOnSdCardStatusRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<SdCardRequestMessage> message, std::function<void(std::shared_ptr<SdCardResponseMessage>)> callback


constexpr const char kEventOnDeviceControlPtzRequest[] = "kEventOnDeviceControlPtzRequest";
#define kEventOnDeviceControlPtzRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_PTZCmd> message

constexpr const char kEventOnDeviceControlTeleBootRequest[] = "kEventOnDeviceControlTeleBootRequest";
#define kEventOnDeviceControlTeleBootRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_TeleBoot> message

constexpr const char kEventOnDeviceControlRecordCmdRequest[] = "kEventOnDeviceControlRecordCmdRequest";
#define kEventOnDeviceControlRecordCmdRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_RecordCmd> message, std::function<void(std::shared_ptr<DeviceControlResponseMessage>)> callback

constexpr const char kEventOnDeviceControlGuardCmdRequest[] = "kEventOnDeviceControlGuardCmdRequest";
#define kEventOnDeviceControlGuardCmdRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_GuardCmd> message, std::function<void(std::shared_ptr<DeviceControlResponseMessage>)> callback

constexpr const char kEventOnDeviceControlAlarmCmdRequest[] = "kEventOnDeviceControlAlarmCmdRequest";
#define kEventOnDeviceControlAlarmCmdRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_AlarmCmd> message, std::function<void(std::shared_ptr<DeviceControlResponseMessage>)> callback

constexpr const char kEventOnDeviceControlIFrameCmdRequest[] = "kEventOnDeviceControlIFrameCmdRequest";
#define kEventOnDeviceControlIFrameCmdRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_IFrameCmdCmd> message

constexpr const char kEventOnDeviceControlDragZoomInRequest[] = "kEventOnDeviceControlDragZoomInRequest";
#define kEventOnDeviceControlDragZoomInRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_DragZoomInCmd> message

constexpr const char kEventOnDeviceControlDragZoomOutRequest[] = "kEventOnDeviceControlDragZoomOutRequest";
#define kEventOnDeviceControlDragZoomOutRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_DragZoomOutCmd> message

constexpr const char kEventOnDeviceControlHomePositionRequest[] = "kEventOnDeviceControlHomePositionRequest";
#define kEventOnDeviceControlHomePositionRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_HomePosition> message, std::function<void(std::shared_ptr<DeviceControlResponseMessage>)> callback

constexpr const char kEventOnDeviceControlPTZPreciseCtrlRequest[] = "kEventOnDeviceControlPTZPreciseCtrlRequest";
#define kEventOnDeviceControlPTZPreciseCtrlRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_PTZPreciseCtrlCmd> message

constexpr const char kEventOnDeviceControlDeviceUpgradeRequest[] = "kEventOnDeviceControlDeviceUpgradeRequest";
#define kEventOnDeviceControlDeviceUpgradeRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_DeviceUpgrade> message, std::function<void(std::shared_ptr<DeviceControlResponseMessage>)> callback

constexpr const char kEventOnDeviceControlFormatSDCardRequest[] = "kEventOnDeviceControlFormatSDCardRequest";
#define kEventOnDeviceControlFormatSDCardRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_FormatSDCardCmd> message

constexpr const char kEventOnDeviceControlTargetTrackRequest[] = "kEventOnDeviceControlTargetTrackRequest";
#define kEventOnDeviceControlTargetTrackRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceControlRequestMessage_TargetTrackCmd> message

constexpr const char kEventOnDeviceConfigRequest[] = "kEventOnDeviceConfigRequest";
#define kEventOnDeviceConfigRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<DeviceConfigRequestMessage> message, std::function<void(std::shared_ptr<DeviceConfigResponseMessage>)> callback

constexpr const char kEventOnBroadcastNotifyRequest[] = "kEventOnBroadcastNotifyRequest";
#define kEventOnBroadcastNotifyRequestArgs std::shared_ptr<SuperPlatform> platform,std::shared_ptr<BroadcastNotifyRequest> message, std::function<void(std::shared_ptr<BroadcastNotifyResponse>)> callback


// 收到订阅通知
constexpr const char kEventOnSubscribeNotify[] = "kEventOnSubscribeNotify";
#define kEventOnSubscribeNotifyArgs std::shared_ptr<SubscribeRequest> subscribe, const std::shared_ptr<tinyxml2::XMLDocument> &xml_ptr, std::string &reason

constexpr const char kEventOnSubscribeRequest[] = "kEventOnSubscribeRequest";
#define kEventOnSubscribeRequestArgs const std::shared_ptr<SuperPlatform> platform,const std::shared_ptr<SubscribeRequest>& subscribe, const std::shared_ptr<MessageBase> &message



}

}

#endif //gb28181_include_gb28181_SIP_COMMON_H



/**********************************************************************************************************
文件名称:   sip_common.h
创建时间:   25-2-5 下午1:51
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 下午1:51

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 下午1:51       描述:   创建文件

**********************************************************************************************************/