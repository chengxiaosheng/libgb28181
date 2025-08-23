#ifndef gb28181_include_gb28181_request_INVATE_REQUEST_H
#define gb28181_include_gb28181_request_INVATE_REQUEST_H
#include "sdp.h"

namespace toolkit {
class Session;
}
namespace gb28181 {
class SuperPlatform;
class SubordinatePlatform;
enum class INVITE_STATUS_TYPE { invite = 0, trying, ack, bye, cancel, failed };
inline const char *to_string(INVITE_STATUS_TYPE e) {
    switch (e) {
        case INVITE_STATUS_TYPE::invite: return "invite";
        case INVITE_STATUS_TYPE::trying: return "trying";
        case INVITE_STATUS_TYPE::ack: return "ack";
        case INVITE_STATUS_TYPE::bye: return "bye";
        case INVITE_STATUS_TYPE::cancel: return "cancel";
        case INVITE_STATUS_TYPE::failed: return "failed";
        default: return "unknown";
    }
}

struct PlaybackState {
    float scale = 1.0; // 播放倍速
    uint32_t current_npt = 0; // 当前播放位置（秒）
    bool is_now = false; // 是否从当前位置开始播放
    bool is_paused = false; // 暂停状态
    bool is_reverse = false; // 是否倒放
    uint32_t last_seq = 0; // 最后发送的RTP序列号
    uint32_t last_rtptime = 0; // 最后RTP时间戳
};

struct PlaybackControl {
    enum { invalid = 0, Play, Pause, Teardown } action;
    bool is_now = false;
    float scale { 0 }; // 为0时表示无效
    double ntp { 0.00 };
    double end_ntp { 0.00 }; // 这个参数应该不会有值吧
};
struct PlaybackControlResponse {
    int rtsp_code { 0 }; // 返回的状态码
    uint32_t seq { 0 }; // 最后发送的RTP序列号
    uint32_t rtptime { 0 }; // 最后RTP时间戳
    double ntp { 0.00 };
    std::string reason; // 错误信息
};

class GB28181_EXPORT InviteRequest {
public:
    /**
     * 回放控制的结果回调
     * @param PlaybackControlResponse 回放控制的结果
     */
    using BackPlayControlResponseCallback = std::function<void(bool, std::string, PlaybackControlResponse)>;
    /**
     * 回放控制请求回调
     * @param PlaybackControl 请求信息
     * @param BackPlayControlResponseCallback 回放控制的结果回调
     * @remark Teardown 不会被触发; 因为Teardown 会走到bye中， 通过 InviteStatusCallback 进行回调
     */
    using BackPlayControlCallback = std::function<void(PlaybackControl, BackPlayControlResponseCallback)>;

    /**
     * invite事务的状态回调
     * @param status 状态
     * @param err 错误信息
     */
    using InviteStatusCallback = std::function<void(INVITE_STATUS_TYPE status, const std::string &err)>;

    virtual ~InviteRequest() = default;
    static std::shared_ptr<InviteRequest> new_invite_request(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<SdpDescription> &sdp, const std::string &device_id);
    static std::shared_ptr<InviteRequest>
    new_invite_request(const std::shared_ptr<SuperPlatform> &platform, const std::shared_ptr<SdpDescription> &sdp, const std::string &device_id);

    virtual void to_invite_request(std::function<void(bool, std::string, const std::shared_ptr<SdpDescription> &)> rcb)
        = 0;
    virtual void to_bye(const std::string &reason) = 0;
    virtual std::shared_ptr<SdpDescription> local_sdp() const = 0;
    virtual std::shared_ptr<SdpDescription> remote_sdp() const = 0;
    virtual uint64_t invite_time() const = 0;
    virtual uint64_t ack_time() const = 0;
    virtual uint64_t close_time() const = 0;
    virtual std::vector<std::string> &preferred_path() = 0;
    virtual std::vector<std::string> &route_path() = 0;
    virtual INVITE_STATUS_TYPE status() const = 0;
    virtual const std::string &error() const = 0;
    virtual void set_status_callback(std::function<void(INVITE_STATUS_TYPE status, const std::string &err)> status_cb)
        = 0;

    virtual void to_teardown(const std::string &reason) = 0;
    virtual void to_pause(const std::function<void(bool, std::string)> &rcb) = 0;
    virtual void to_play(const BackPlayControlResponseCallback &rcb) = 0;
    virtual void
    to_seek_scale(std::optional<float> scale, std::optional<uint32_t> ntp, const BackPlayControlResponseCallback &rcb)
        = 0;
    virtual const std::string& device_id() const = 0;
};
} // namespace gb28181

#endif // gb28181_include_gb28181_request_INVATE_REQUEST_H

/**********************************************************************************************************
文件名称:   invite_request.h
创建时间:   25-2-15 上午11:23
作者名称:   Kevin
文件路径:   include/gb28181/request
功能描述:   用来实现 invite 请求
修订时间:   25-2-15 上午11:23

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-15 上午11:23       描述:   创建文件

**********************************************************************************************************/