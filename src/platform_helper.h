#ifndef gb28181_src_PLATFORM_HELPER_H
#define gb28181_src_PLATFORM_HELPER_H
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <gb28181/type_define.h>
#include <Network/sockutil.h>


namespace gb28181 {
class SdpDescription;
struct sdp_description;
}
namespace gb28181 {
class InviteRequest;
}

#ifdef __cplusplus

extern "C" {
    struct timeval;
    struct sip_message_t;
    struct sip_uas_transaction_t;
    struct sip_uac_transaction_t;
    struct sip_agent_t;
}
#endif



namespace toolkit {
class EventPoller;
class SockException;
}
namespace gb28181 {
class MessageBase;
class RequestProxyImpl;
class SipServer;
class SipSession;
class PlatformHelper : public std::enable_shared_from_this<PlatformHelper> {
public:
    // 发送sip请求的结果回调
    using SipReplyCallback = std::function<int(const std::shared_ptr<SipSession> &session, const std::shared_ptr<struct sip_message_t> &reply, const std::shared_ptr<struct sip_uac_transaction_t> &transaction, int code)>;

    virtual ~PlatformHelper();
    virtual platform_account &sip_account()  = 0;
    virtual TransportType get_transport() const = 0;

    virtual CharEncodingType get_encoding() const = 0;

    struct sip_agent_t * get_sip_agent();

    std::shared_ptr<SipServer> get_sip_server() {
        return local_server_weak_.lock();
    }

    /**
     * 获取一个新的sn
     * @return
     */
    virtual int32_t get_new_sn();

    virtual bool update_remote_via(std::pair<std::string, uint32_t> val);
    virtual std::string get_from_uri();
    virtual std::string get_to_uri();
    virtual std::string get_contact_uri();
    /**
    * @brief 平台地址发生改变
    * @remark 当本地作为下级，发起注册的时候，对方返回了 301/302， 那么应该使用临时地址访问
    * 当本地作为上级，对方发来消息的地址与当前不一致（一般出现在移动设备上），也应该更新平台地址
    */
    void on_platform_addr_changed(struct sockaddr_storage& addr);

    void add_request_proxy(int32_t sn, const std::shared_ptr<RequestProxyImpl> &proxy);
    void remove_request_proxy(int32_t sn);
    int on_response(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);

    static int on_recv_message(
    const std::shared_ptr<SipSession> &session, const std ::shared_ptr<sip_uas_transaction_t> &transaction,
    const std ::shared_ptr<sip_message_t> &req, void *dialog_ptr);

    virtual int on_notify(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual int on_query(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual int on_control(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual void on_invite(const std::shared_ptr<InviteRequest> &invite_request, std::function<void(int, std::shared_ptr<SdpDescription>)> && resp) = 0;

    void uac_send(const std::shared_ptr<sip_uac_transaction_t>& transaction, std::string&& payload, const std::function<void(bool,std::string)> &rcb, bool force_tcp = false);

    void uac_send2(const std::shared_ptr<sip_uac_transaction_t>& transaction, std::string&& payload, const std::function<void(bool,std::string, const std::shared_ptr<SipSession> &)> &rcb, bool force_tcp = false);

    /**
    * @brief 发送
    */
    void uac_send3(const std::shared_ptr<sip_uac_transaction_t>& transaction, std::string&& payload, const std::function<void(bool,std::string)> &ecb, SipReplyCallback rcb, bool force_tcp = false);

    void set_tcp_session(const std::shared_ptr<SipSession> &session);

    void set_platform_status_cb(void * user_data, std::function<void(PlatformStatusType)> cb);
    void remove_platform_status_cb(void * user_data);

private:
    void get_session(
        const std::function<void(const toolkit::SockException &, std::shared_ptr<gb28181::SipSession>)> &cb,
        bool force_tcp = false);

protected:
    std::shared_ptr<SipSession> tcp_session_;
    // local_server
    std::weak_ptr<SipServer> local_server_weak_;
    // 发送消息时 本地uri
    std::string from_uri_;
    // 发送消息时 目标uri
    std::string to_uri_;
    // 存储等待应答的请求
    std::unordered_map<int32_t, std::shared_ptr<RequestProxyImpl>> request_map_;
    std::shared_mutex request_map_mutex_;
    std::unordered_map<void *, std::weak_ptr<InviteRequest>> invite_map_;
    std::recursive_mutex invite_map_mutex_;
    // 专门用来发送sip消息的 session, 采用udp server 监听的socket封装，内部不绑定对端地址, 仅仅复用监听sock 发送数据
    // std::unordered_map<toolkit::EventPoller*, std::shared_ptr<SipSession>> udp_sip_session_map_;
    // std::recursive_mutex udp_sip_session_map_mutex_;
    // 联系地址
    std::string contact_uri_;
    struct sockaddr_storage remote_addr_{};
    std::recursive_mutex remote_addr_mutex_;
    std::mutex status_cbs_mtx_;
    std::unordered_map<void*, std::function<void(PlatformStatusType)>> status_cbs_; // 平台在线状态回到
    std::atomic_int32_t platform_sn_ { 1 };
};
} // namespace gb28181

#endif // gb28181_src_PLATFORM_HELPER_H

/**********************************************************************************************************
文件名称:   platform_helper.h
创建时间:   25-2-11 下午4:37
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午4:37

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午4:37       描述:   创建文件

**********************************************************************************************************/