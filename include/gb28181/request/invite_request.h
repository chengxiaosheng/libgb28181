#ifndef gb28181_include_gb28181_request_INVATE_REQUEST_H
#define gb28181_include_gb28181_request_INVATE_REQUEST_H

namespace gb28181 {
struct sdp_description;
enum class INVITE_STATUS_TYPE { invite = 0, trying, ack, bye, cancel, failed };
struct invite_mansrtsp {};
class InviteRequest {
public:
    virtual ~InviteRequest() = default;
    static std::shared_ptr<InviteRequest> new_invite_request(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<sdp_description> &sdp);

    virtual void to_invite_request(std::function<void(bool, std::string, const std::shared_ptr<sdp_description> &)> rcb)
        = 0;
    virtual void to_bye(const std::string &reason) = 0;
    virtual std::shared_ptr<sdp_description> local_sdp() const = 0;
    virtual std::shared_ptr<sdp_description> remote_sdp() const = 0;
    virtual uint64_t invite_time() const = 0;
    virtual uint64_t ack_time() const = 0;
    virtual uint64_t close_time() const = 0;
    virtual std::vector<std::string> &preferred_path() = 0;
    virtual std::vector<std::string> &route_path() = 0;
    virtual INVITE_STATUS_TYPE status() const = 0;
    virtual const std::string &error() const = 0;
    virtual void set_status_callback(std::function<void(INVITE_STATUS_TYPE status, const std::string &err)> status_cb)
        = 0;

    virtual void to_scale(float scale, const std::function<void(bool, std::string)> &rcb) = 0;
    virtual void to_teardown(const std::function<void(bool, std::string)> &rcb) = 0;
    virtual void to_pause(const std::function<void(bool, std::string)> &rcb) = 0;
    virtual void to_play(const std::function<void(bool, std::string)> &rcb) = 0;
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