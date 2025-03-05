#ifndef gb28181_include_gb28181_SIP_COMMON_H
#define gb28181_include_gb28181_SIP_COMMON_H

#include <functional>
#include <memory>
#include <deque>

namespace gb28181 {

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

class SubscribeRequest;

namespace Broadcast {
//下级平台心跳
constexpr const char kEventSubKeepalive[] = "kEventSubKeepalive";
#define kEventSubKeepaliveArgs const std::shared_ptr<SubordinatePlatform>& platform, const std::shared_ptr<KeepaliveMessageRequest> & message

// 下级平台在线状态变更
constexpr const char kEventSubordinatePlatformStatus[] = "kEventSubordinatePlatformStatus";
#define kEventSubordinatePlatformStatusArgs const std::shared_ptr<SubordinatePlatform> & platform , const PlatformStatusType& status, const std::string& message

// 上级平台在线状态变更
constexpr const char kEventSuperPlatformStatus[] = "kEventSuperPlatformStatus";
#define kEventSuperPlatformStatusArgs const std::shared_ptr<SuperPlatform>& platform , const PlatformStatusType& status, const std::string& message

// 上级平台心跳通知
constexpr const char kEventSuperPlatformKeepalive[] = "kEventSuperPlatformKeepalive";
#define kEventSuperPlatformKeepaliveArgs const std::shared_ptr<SuperPlatform>& platform, const bool& success, const std::string & message

// 下级平台本地联系信息变更
constexpr const char kEventSubordinatePlatformContactChanged[] = "kEventSubordinatePlatformContactChanged";
#define kEventSubordinatePlatformContactChangedArgs const std::shared_ptr<SubordinatePlatform>& platform , const std::string& host, const uint16_t& port

// 上级平台本地联系信息变更
constexpr const char kEventSuperPlatformContactChanged[] = "kEventSuperPlatformContactChanged";
#define kEventSuperPlatformContactChangedArgs const std::shared_ptr<SuperPlatform> & platform , const std::string& host, const uint16_t& port

// 上级平台shutdown时 触发的事件， 用户收到这个事件后应该立即释放持有的平台智能指针
constexpr const char kEventOnSuperPlatformShutdown[] = "kEventOnSuperPlatformShutdown";
#define kEventOnSuperPlatformShutdownArgs const std::shared_ptr<SuperPlatform> & platform

// 下级平台shutdown时 触发的事件， 用户收到这个事件后应该立即释放持有的平台智能指针
constexpr const char kEventOnSubordinatePlatformShutdown[] = "kEventOnSubordinatePlatformShutdown";
#define kEventOnSubordinatePlatformShutdownArgs const std::shared_ptr<SubordinatePlatform> & platform

// 下级平台收到 告警通知
constexpr const char kEventSubordinateNotifyAlarm[] = "kEventSubordinateNotifyAlarm";
#define kEventSubordinateNotifyAlarmArgs const std::shared_ptr<SubordinatePlatform> & platform, const std::shared_ptr<AlarmNotifyMessage> & message

// 下级平台收到 目录通知 （仅仅来自Message 中的通知 ?）
constexpr const char kEventSubordinateNotifyCatalog[] = "kEventSubordinateNotifyCatalog";
#define kEventSubordinateNotifyCatalogArgs const std::shared_ptr<SubordinatePlatform>& platform , const std::shared_ptr<CatalogNotifyMessage> & message

// 下级平台收到 媒体状态通知
constexpr const char kEventSubordinateNotifyMediaStatus[] = "kEventSubordinateNotifyMediaStatus";
#define kEventSubordinateNotifyMediaStatusArgs const std::shared_ptr<SubordinatePlatform> & platform , const std::shared_ptr<MediaStatusNotifyMessage>& message

// 下级平台收到 移动设备位置通知
constexpr const char kEventSubordinateNotifyMobilePosition[] = "kEventSubordinateNotifyMobilePosition";
#define kEventSubordinateNotifyMobilePositionArgs const std::shared_ptr<SubordinatePlatform>& platform , const std::shared_ptr<MobilePositionNotifyMessage>& message

// 下级平台收到 截图上传完成通知
constexpr const char kEventSubordinateNotifyUploadSnapShotFinished[] = "kEventSubordinateNotifyUploadSnapShotFinished";
#define kEventSubordinateNotifyUploadSnapShotFinishedArgs const std::shared_ptr<SubordinatePlatform>& platform , const std::shared_ptr<UploadSnapShotFinishedNotifyMessage> & message

// 下级平台收到 视频上传完成通知
constexpr const char kEventSubordinateNotifyVideoUploadNotify[] = "kEventSubordinateNotifyVideoUploadNotify";
#define kEventSubordinateNotifyVideoUploadNotifyArgs const std::shared_ptr<SubordinatePlatform> & platform , const std::shared_ptr<VideoUploadNotifyMessage> & message

// 下级平台收到 设备升级结果通知
constexpr const char kEventSubordinateNotifyDeviceUpgradeResult[] = "kEventSubordinateNotifyDeviceUpgradeResult";
#define kEventSubordinateNotifyDeviceUpgradeResultArgs const std::shared_ptr<SubordinatePlatform> & platform , const std::shared_ptr<DeviceUpgradeResultNotifyMessage>& message


// 收到来自上级平台的 设备状态查询请求
constexpr const char kEventOnDeviceStatusRequest[] = "kEventOnDeviceStatusRequest";
#define kEventOnDeviceStatusRequestArgs const std::shared_ptr<SuperPlatform>& platform,const std::shared_ptr<DeviceStatusMessageRequest> & message, const std::function<void(std::shared_ptr<DeviceStatusMessageResponse>)> & callback

// 收到来自上级平台的 目录查询请求
constexpr const char kEventOnCatalogRequest[] = "kEventOnCatalogRequest";
/**
 * @brief 目录查询请求事件的参数
 * @param platform 平台指针
 * @param message 请求消息指针
 * @param callback 回复的数据回调
 *  @arg resposne 响应消息队列
 *      - 队列中的每个元素代表一个待发送的回复数据包
 *      - 单个请求最大长度为 8*1024 组装消息时请考虑最大包长度
 *  @arg concurrency 并发控制参数
 *      - 默认为1
 *      - 并发逻辑是在同一个线程中顺序发送 concurrency个回复， 后续得到一个reply== 200 ok 立即发送下一个。
 *  @arg ignore_4xx 忽略4xx 的返回
 *      - true: 当对端返回4xx 时继续发送下一个
 *      - false: 当对端返回4xx 时立即结束任务
 */
#define kEventOnCatalogRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<CatalogRequestMessage>& message, const std::function<void(std::deque<std::shared_ptr<CatalogResponseMessage>>&& response, int concurrency, bool ignore_4xx)>& callback

constexpr const char kEventOnDeviceInfoRequest[] = "kEventOnDeviceInfoRequest";
#define kEventOnDeviceInfoRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceInfoMessageRequest> & message, const std::function<void(std::shared_ptr<DeviceInfoMessageResponse>)>& callback

// 收到来自上级平台的 目录查询请求
constexpr const char kEventOnRecordInfoRequest[] = "kEventOnRecordInfoRequest";
/**
 * @brief 目录查询请求事件的参数
 * @param platform 平台指针
 * @param message 请求消息指针
 * @param callback 回复的数据回调
 *  @arg resposne 响应消息队列
 *      - 队列中的每个元素代表一个待发送的回复数据包
 *      - 单个请求最大长度为 8*1024 组装消息时请考虑最大包长度
 *  @arg concurrency 并发控制参数
 *      - 默认为1
 *      - 并发逻辑是在同一个线程中顺序发送 concurrency个回复， 后续得到一个reply== 200 ok 立即发送下一个。
 *  @arg ignore_4xx 忽略4xx 的返回
 *      - true: 当对端返回4xx 时继续发送下一个
 *      - false: 当对端返回4xx 时立即结束任务
 */
#define kEventOnRecordInfoRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<RecordInfoRequestMessage> message, const std::function<void(std::deque<std::shared_ptr<RecordInfoResponseMessage>>&& response, int concurrency, bool ignore_4xx)>& callback

// 收到来自上级平台的 目录查询请求
constexpr const char kEventOnConfigDownloadRequest[] = "kEventOnConfigDownloadRequest";
/**
 * @brief 目录查询请求事件的参数
 * @param platform 平台指针
 * @param message 请求消息指针
 * @param callback 回复的数据回调
 *  @arg resposne 响应消息队列
 *      - 队列中的每个元素代表一个待发送的回复数据包
 *      - 单个请求最大长度为 8*1024 组装消息时请考虑最大包长度
 *  @arg concurrency 并发控制参数
 *      - 默认为1
 *      - 并发逻辑是在同一个线程中顺序发送 concurrency个回复， 后续得到一个reply== 200 ok 立即发送下一个。
 *  @arg ignore_4xx 忽略4xx 的返回
 *      - true: 当对端返回4xx 时继续发送下一个
 *      - false: 当对端返回4xx 时立即结束任务
 */
#define kEventOnConfigDownloadRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<ConfigDownloadRequestMessage>& message, const std::function<void(std::deque<std::shared_ptr<ConfigDownloadResponseMessage>> && response, int concurrency, bool ignore_4xx)>& callback

// 收到来自上级平台的 目录查询请求
constexpr const char kEventOnPresetListRequest[] = "kEventOnPresetListRequest";
/**
 * @brief 目录查询请求事件的参数
 * @param platform 平台指针
 * @param message 请求消息指针
 * @param callback 回复的数据回调
 *  @arg resposne 响应消息队列
 *      - 队列中的每个元素代表一个待发送的回复数据包
 *      - 单个请求最大长度为 8*1024 组装消息时请考虑最大包长度
 *  @arg concurrency 并发控制参数
 *      - 默认为1
 *      - 并发逻辑是在同一个线程中顺序发送 concurrency个回复， 后续得到一个reply== 200 ok 立即发送下一个。
 *  @arg ignore_4xx 忽略4xx 的返回
 *      - true: 当对端返回4xx 时继续发送下一个
 *      - false: 当对端返回4xx 时立即结束任务
 */
#define kEventOnPresetListRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<PresetRequestMessage>& message, const std::function<void(std::deque<std::shared_ptr<PresetResponseMessage>>&& response, int concurrency, bool ignore_4xx)>& callback


constexpr const char kEventOnHomePositionRequest[] = "kEventOnHomePositionRequest";
#define kEventOnHomePositionRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<HomePositionRequestMessage>& message, const std::function<void(std::shared_ptr<HomePositionResponseMessage>)> & callback

constexpr const char kEventOnCruiseTrackListRequest[] = "kEventOnCruiseTrackListRequest";
/**
 * @brief 目录查询请求事件的参数
 * @param platform 平台指针
 * @param message 请求消息指针
 * @param callback 回复的数据回调
 *  @arg resposne 响应消息队列
 *      - 队列中的每个元素代表一个待发送的回复数据包
 *      - 单个请求最大长度为 8*1024 组装消息时请考虑最大包长度
 *  @arg concurrency 并发控制参数
 *      - 默认为1
 *      - 并发逻辑是在同一个线程中顺序发送 concurrency个回复， 后续得到一个reply== 200 ok 立即发送下一个。
 *  @arg ignore_4xx 忽略4xx 的返回
 *      - true: 当对端返回4xx 时继续发送下一个
 *      - false: 当对端返回4xx 时立即结束任务
 */
#define kEventOnCruiseTrackListRequestArgs const std::shared_ptr<SuperPlatform>& platform,const std::shared_ptr<CruiseTrackListRequestMessage>& message, const std::function<void(std::deque<std::shared_ptr<CruiseTrackListResponseMessage>>&& response, int concurrency, bool ignore_4xx)>& callback

constexpr const char kEventOnCruiseTrackRequest[] = "kEventOnCruiseTrackRequest";
/**
 * @brief 目录查询请求事件的参数
 * @param platform 平台指针
 * @param message 请求消息指针
 * @param callback 回复的数据回调
 *  @arg resposne 响应消息队列
 *      - 队列中的每个元素代表一个待发送的回复数据包
 *      - 单个请求最大长度为 8*1024 组装消息时请考虑最大包长度
 *  @arg concurrency 并发控制参数
 *      - 默认为1
 *      - 并发逻辑是在同一个线程中顺序发送 concurrency个回复， 后续得到一个reply== 200 ok 立即发送下一个。
 *  @arg ignore_4xx 忽略4xx 的返回
 *      - true: 当对端返回4xx 时继续发送下一个
 *      - false: 当对端返回4xx 时立即结束任务
 */
#define kEventOnCruiseTrackRequestArgs const std::shared_ptr<SuperPlatform>& platform,std::shared_ptr<CruiseTrackRequestMessage> message, const std::function<void(std::deque<std::shared_ptr<CruiseTrackResponseMessage>>&& response, int concurrency, bool ignore_4xx)> & callback


constexpr const char kEventOnPTZPositionRequest[] = "kEventOnPTZPositionRequest";
#define kEventOnPTZPositionRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<PTZPositionRequestMessage>& message, const std::function<void(std::shared_ptr<PTZPositionResponseMessage>)>& callback


constexpr const char kEventOnSdCardStatusRequest[] = "kEventOnSdCardStatusRequest";
#define kEventOnSdCardStatusRequestArgs const std::shared_ptr<SuperPlatform>& platform,const std::shared_ptr<SdCardRequestMessage>& message, const std::function<void(std::shared_ptr<SdCardResponseMessage>)> & callback


constexpr const char kEventOnDeviceControlPtzRequest[] = "kEventOnDeviceControlPtzRequest";
#define kEventOnDeviceControlPtzRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_PTZCmd>& message

constexpr const char kEventOnDeviceControlTeleBootRequest[] = "kEventOnDeviceControlTeleBootRequest";
#define kEventOnDeviceControlTeleBootRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_TeleBoot>& message

constexpr const char kEventOnDeviceControlRecordCmdRequest[] = "kEventOnDeviceControlRecordCmdRequest";
#define kEventOnDeviceControlRecordCmdRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_RecordCmd> & message, const std::function<void(std::shared_ptr<DeviceControlResponseMessage>)>& callback

constexpr const char kEventOnDeviceControlGuardCmdRequest[] = "kEventOnDeviceControlGuardCmdRequest";
#define kEventOnDeviceControlGuardCmdRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_GuardCmd>& message, const std::function<void(std::shared_ptr<DeviceControlResponseMessage>)>& callback

constexpr const char kEventOnDeviceControlAlarmCmdRequest[] = "kEventOnDeviceControlAlarmCmdRequest";
#define kEventOnDeviceControlAlarmCmdRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_AlarmCmd>& message, const std::function<void(std::shared_ptr<DeviceControlResponseMessage>)>& callback

constexpr const char kEventOnDeviceControlIFrameCmdRequest[] = "kEventOnDeviceControlIFrameCmdRequest";
#define kEventOnDeviceControlIFrameCmdRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_IFrameCmd>& message

constexpr const char kEventOnDeviceControlDragZoomInRequest[] = "kEventOnDeviceControlDragZoomInRequest";
#define kEventOnDeviceControlDragZoomInRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_DragZoomIn>& message

constexpr const char kEventOnDeviceControlDragZoomOutRequest[] = "kEventOnDeviceControlDragZoomOutRequest";
#define kEventOnDeviceControlDragZoomOutRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_DragZoomOut>& message

constexpr const char kEventOnDeviceControlHomePositionRequest[] = "kEventOnDeviceControlHomePositionRequest";
#define kEventOnDeviceControlHomePositionRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_HomePosition>& message, const std::function<void(std::shared_ptr<DeviceControlResponseMessage>)>& callback

constexpr const char kEventOnDeviceControlPTZPreciseCtrlRequest[] = "kEventOnDeviceControlPTZPreciseCtrlRequest";
#define kEventOnDeviceControlPTZPreciseCtrlRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_PtzPreciseCtrl>& message

constexpr const char kEventOnDeviceControlDeviceUpgradeRequest[] = "kEventOnDeviceControlDeviceUpgradeRequest";
#define kEventOnDeviceControlDeviceUpgradeRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_DeviceUpgrade>& message, const std::function<void(std::shared_ptr<DeviceControlResponseMessage>)>& callback

constexpr const char kEventOnDeviceControlFormatSDCardRequest[] = "kEventOnDeviceControlFormatSDCardRequest";
#define kEventOnDeviceControlFormatSDCardRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_FormatSDCard>& message

constexpr const char kEventOnDeviceControlTargetTrackRequest[] = "kEventOnDeviceControlTargetTrackRequest";
#define kEventOnDeviceControlTargetTrackRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceControlRequestMessage_TargetTrack>& message

constexpr const char kEventOnDeviceConfigRequest[] = "kEventOnDeviceConfigRequest";
#define kEventOnDeviceConfigRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<DeviceConfigRequestMessage>& message, const std::function<void(std::shared_ptr<DeviceConfigResponseMessage>)>& callback

constexpr const char kEventOnBroadcastNotifyRequest[] = "kEventOnBroadcastNotifyRequest";
#define kEventOnBroadcastNotifyRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<BroadcastNotifyRequest>& message, const std::function<void(std::shared_ptr<BroadcastNotifyResponse>)> & callback

// 收到订阅通知
constexpr const char kEventOnSubscribeNotify[] = "kEventOnSubscribeNotify";
#define kEventOnSubscribeNotifyArgs std::shared_ptr<SubscribeRequest> subscribe, const std::shared_ptr<tinyxml2::XMLDocument> &xml_ptr, const std::string &reason

constexpr const char kEventOnSubscribeRequest[] = "kEventOnSubscribeRequest";
#define kEventOnSubscribeRequestArgs const std::shared_ptr<SuperPlatform> & platform, const std::shared_ptr<SubscribeRequest> & subscribe, const std::shared_ptr<MessageBase> &message

// 收到播放请求
constexpr const char kEventOnInviteRequest[] = "kEventOnInviteRequest";
#define kEventOnInviteRequestArgs const std::shared_ptr<SuperPlatform>& platform, const std::shared_ptr<InviteRequest> &message, const std::function<void(int, std::shared_ptr<SdpDescription>)> &resp



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