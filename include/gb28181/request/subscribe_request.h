#ifndef gb28181_include_gb28181_request_SUBSCRIBE_REQUEST_H
#define gb28181_include_gb28181_request_SUBSCRIBE_REQUEST_H
#include "tinyxml2.h"

#include <functional>
#include <string>
#include <memory>

namespace gb28181 {
class SubordinatePlatform;
class MessageBase;
class SubscribeRequest {
public:
    enum TERMINATED_TYPE_E : uint8_t {
        invalid = 0, // 无效
        deactivated, // 订阅已被服务器手动停用。
        timeout, // 订阅因有效期超时而终止。
        rejected, // 订阅被服务器拒绝。
        noresource, // 订阅的资源不再可用。
        probation, // 订阅因某些限制暂时无效，可能稍后可以重新尝试。
        giveup, // 服务器多次尝试发送通知失败，放弃了订阅。
        expired, // 订阅的资源已过期。
        invariant, // 订阅因违反协议或服务端的约束条件而被终止。
    };
    enum SUBSCRIBER_STATUS_TYPE_E : uint8_t {
        active, // 表示订阅有效。
        pending, // 表示订阅已被服务器接收，但尚未完成相关处理。
        terminated, // 表示订阅已终止，订阅者不会再接收任何事件通知。
    };
    struct subscribe_info {
        std::string event; // 事件名称
        uint64_t subscribe_id { 0 }; // 订阅唯一标识
        uint32_t expires { 3600 }; // 订阅有效期
    };
    /**
     * 订阅状态变更回调
     */
    using SubscriberStatusCallback
        = std::function<void(std::shared_ptr<SubscribeRequest>, SUBSCRIBER_STATUS_TYPE_E, std::string)>;
    using SubscriberNotifyReplyCallback = std::function<void(int, std::string)>;
    using SubscriberNotifyCallback
        = std::function<int(std::shared_ptr<SubscribeRequest>, const std::shared_ptr<tinyxml2::XMLDocument> &xml_ptr, std::string &reason)>;

    virtual ~SubscribeRequest() = default;
    /**
     *设置状态回调
     * @param cb 设置订阅状态回调
     */
    virtual void set_status_callback(SubscriberStatusCallback cb) = 0;

    /**
     * 设置通知回调
     * @param cb
     * @remark 同步回调，请勿阻塞， 回调应返回一个sip状态码
     */
    virtual void set_notify_callback(SubscriberNotifyCallback cb) = 0;
    /**
     * 启动订阅
     * - 当 <b>is_incoming() = false</b> 时， 此函数用来启动订阅
     * - 当 <b>is_incoming() = true</b> 时， 此函数将回复200 ok 并启动订阅
     */
    virtual void start() = 0;
    /**
     * 结束订阅
     * - 当 <b>is_incoming() = false</b> 时， 发送取消订阅消息，并终止订阅
     * - 当 <b>is_incoming() = true</b> 时， 发送终止通知，并取消订阅
     * @param reason
     */
    virtual void shutdown(std::string reason) = 0;
    /**
     * 获取订阅状态
     */
    virtual SUBSCRIBER_STATUS_TYPE_E status() const = 0;
    /**
     * 获取订阅错误信息
     * @return
     */
    virtual const std::string &error() const = 0;
    /**
     * 发送通知
     * @param message
     * @param rcb
     */
    virtual void send_notify(const std::shared_ptr<MessageBase> &message, SubscriberNotifyReplyCallback rcb) = 0;

    /**
     * 获取订阅剩余的秒
     * @return
     */
    virtual int time_remain() const = 0;

    /**
     * 是否为传入的订阅
     * @return true: 是传入的， false 不是传入的
     */
    virtual bool is_incoming() const = 0;

    /**
     * 创建一个新的订阅器
     * @param platform
     * @param message
     * @param info
     * @return
     */
    static std::shared_ptr<SubscribeRequest> new_subscribe(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &message,
        subscribe_info &&info);

private:
};
} // namespace gb28181

#endif // gb28181_include_gb28181_request_SUBSCRIBE_REQUEST_H

/**********************************************************************************************************
文件名称:   subscribe_request.h
创建时间:   25-2-15 上午10:58
作者名称:   Kevin
文件路径:   include/gb28181/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-15 上午10:58

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-15 上午10:58       描述:   创建文件

**********************************************************************************************************/