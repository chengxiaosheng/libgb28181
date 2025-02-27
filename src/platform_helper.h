#ifndef gb28181_src_PLATFORM_HELPER_H
#define gb28181_src_PLATFORM_HELPER_H
#include <atomic>
#include <functional>
#include <gb28181/type_define.h>
#include <mutex>
#include <shared_mutex>

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
class SockException;
}
namespace gb28181 {
class MessageBase;
class RequestProxyImpl;
class SipServer;
class SipSession;
class PlatformHelper : public std::enable_shared_from_this<PlatformHelper> {
public:
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

    void add_request_proxy(int32_t sn, const std::shared_ptr<RequestProxyImpl> &proxy);
    void remove_request_proxy(int32_t sn);
    int on_response(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual int on_notify(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual int on_query(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual int on_control(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);
    virtual void on_invite(const std::shared_ptr<InviteRequest> &invite_request, std::function<void(int, std::shared_ptr<SdpDescription>)> && resp) = 0;

    void uac_send(std::shared_ptr<sip_uac_transaction_t> transaction, std::string&& payload, const std::function<void(bool,std::string)> &rcb, bool force_tcp = false);

    void set_tcp_session(const std::shared_ptr<SipSession> &session);

private:
    virtual void get_session(
    const std::function<void(const toolkit::SockException &, std::shared_ptr<gb28181::SipSession>)> &cb,bool force_tcp = false);


protected:
    std::shared_ptr<SipSession> tcp_session_;
    // local_server
    std::weak_ptr<SipServer> local_server_weak_;
    std::atomic_int32_t platform_sn_ { 1 };
    // 发送消息时 本地uri
    std::string from_uri_;
    // 发送消息时 目标uri
    std::string to_uri_;
    // 存储等待应答的请求
    std::unordered_map<int32_t, std::shared_ptr<RequestProxyImpl>> request_map_;
    std::shared_mutex request_map_mutex_;
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