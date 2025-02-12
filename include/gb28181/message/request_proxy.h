#ifndef gb28181_include_gb28181_message_REQUEST_PROXY_H
#define gb28181_include_gb28181_message_REQUEST_PROXY_H
#include <functional>
#include <memory>
#include <string>

namespace gb28181 {
class SubordinatePlatform;
}
namespace gb28181 {
class PlatformBase;
}
namespace gb28181 {
class MessageBase;
class RequestProxy {
public:
    virtual ~RequestProxy() = default;
    enum Status {
        Init = 0, // 初始化阶段
        Sending = 1, // 发送中，或已发送
        Replied = 2, // 已经收到回复
        Succeeded = 3, // 成功
        Timeout = 4, // 等待Response 超时
        Failed = 5, // 失败
    };
    enum RequestType {
        invalid = 0, // 无效的，
        NoResponse = 1, // 无应答的请求
        OneResponse = 2, // 单个应答的请求
        MultipleResponses = 3, // 多个应答的请求
    };
    using ResponseCallback = std::function<int(std::shared_ptr<RequestProxy>, std::shared_ptr<MessageBase>, bool end)>;
    using ReplyCallback = std::function<void(std::shared_ptr<RequestProxy>, int code)>;

    /**
     * 获取所有应答
     * @return
     */
    virtual const std::vector<std::shared_ptr<MessageBase>> &all_response() const = 0;

    /**
     * 获取第一个应答
     * @return
     */
    const std::shared_ptr<MessageBase>  response() const {
        auto list = all_response();
        if (list.empty()) return nullptr;
        return list.front();
    }

    /**
     * 获取请求状态
     */
    virtual Status status() const = 0;
    /**
     * 获取请求类型
     */
    virtual RequestType type() const = 0;

    /**
     * 获取错误信息
     */
    virtual std::string error() const = 0;

    /**
     * 发送请求
     */
    virtual void send(std::function<void(std::shared_ptr<RequestProxy>)> rcb) = 0;

    /**
     * 设置确认回调
     */
    virtual void set_reply_callback(ReplyCallback) = 0;

    /**
     * 设置数据回调
     * @remark 每得到一个应答都会执行一次回调
     */
    virtual void set_response_callback(ResponseCallback) = 0;

    /**
     * 发送时间
     * @return
     */
    virtual uint64_t send_time() const = 0;
    /**
     * 确认时间
     * @return
     */
    virtual uint64_t reply_time() const = 0;
    /**
     * 收到第一个应答的时间
     * @return
     */
    virtual uint64_t response_begin_time() const = 0;
    /**
     * 收到最后一个应答的时间
     * @return
     */
    virtual uint64_t response_end_time() const = 0;

    /**
     * 设置超时时间
     * @param sec
     */
    virtual void set_timeout(int sec) = 0;

    /**
     * 构建一个请求
     */
    static std::shared_ptr<RequestProxy> newRequestProxy(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request);

    explicit operator bool() const { return status() == Succeeded; }
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_REQUEST_PROXY_H

/**********************************************************************************************************
文件名称:   request_proxy.h
创建时间:   25-2-11 下午12:23
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午12:23

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午12:23       描述:   创建文件

**********************************************************************************************************/