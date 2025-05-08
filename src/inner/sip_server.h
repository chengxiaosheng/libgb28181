#ifndef gb28181_src_inner_SIP_SERVER_H
#define gb28181_src_inner_SIP_SERVER_H
#include "Network/Socket.h"
#include "Network/TcpServer.h"
#include "Network/UdpServer.h"
#include <functional>
#include <gb28181/local_server.h>
#include <memory>

#ifdef __cplusplus
extern "C" {
struct sip_agent_t;
struct sip_uas_handler_t;
struct sip_message_t;
struct sip_uas_transaction_t;
struct sip_dialog_t;
}
#endif

namespace gb28181 {
class SuperPlatformImpl;
class SubordinatePlatformImpl;
struct sip_agent_param;
class SipSession;
class SipServer;
struct sip_agent_param {
    std::shared_ptr<SipSession> session_ptr;
    std::shared_ptr<SipServer> server_ptr;
};

class SipServer final: public LocalServer, public std::enable_shared_from_this<SipServer> {
public:
    using Ptr = std::shared_ptr<SipServer>;
    using onRecv = std::function<void(const toolkit::Buffer::Ptr &buf)>;

    SipServer(local_account account);
    ~SipServer() override;

    void run() override;
    void shutdown() override;
    const local_account &get_account() const override { return account_; }
    void set_passwd(const std::string &passwd) override { account_.password = passwd; }
    void set_name(const std::string &name) override { account_.name = name; }
    void set_platform_id(const std::string &id) override { account_.platform_id = id; }
    void set_allow_auto_register(bool allow_auto_register) override { account_.allow_auto_register = allow_auto_register; }
    bool allow_auto_register() const override { return account_.allow_auto_register; }
    void reload_account(sip_account account) override;


    /**
     * 获取一个客户端连接
     * @param protocol
     * @param host
     * @param port
     * @param cb
     * @remark 此函数应该只提供给本地作为下级时使用，
     * 当本地作为上级时，特别是在tcp模式下，应该严格复用下级平台的连接session
     * 在udp下，此处复用了udp监听的sock，理论上是可行的
     */
    void get_client(
        TransportType protocol, const std::string &host, uint16_t port,
        const std::function<void(const toolkit::SockException &e, std::shared_ptr<SipSession>)> cb);

    void get_client_l(bool is_udp, const struct sockaddr_storage &addr, const std::function<void(const toolkit::SockException &e, std::shared_ptr<SipSession>)> cb);

    inline std::shared_ptr<sip_agent_t> get_sip_agent() const { return sip_; }

    std::shared_ptr<SubordinatePlatform> get_subordinate_platform(const std::string &platform_id) override;
    std::vector<std::shared_ptr<SubordinatePlatform>> get_all_subordinate_platform() override;
    std::shared_ptr<SuperPlatform> get_super_platform(const std::string &platform_id) override;
    std::vector<std::shared_ptr<SuperPlatform>> get_all_super_platforms() override;
    std::shared_ptr<SubordinatePlatform> add_subordinate_platform(subordinate_account &&account) override;
    void remove_subordinate_platform(const std::string &platform_id) override;
    std::shared_ptr<SuperPlatform> add_super_platform(super_account &&account) override;
    void remove_super_platform(const std::string &platform_id) override;

    void set_new_subordinate_account_callback(subordinate_account_callback cb) override { new_subordinate_account_callback_ = std::move(cb); }
    void new_subordinate_account(const std::shared_ptr<subordinate_account> &account, const std::function<void(std::shared_ptr<SubordinatePlatformImpl>)>& allow_cb);

    uint32_t make_ssrc(bool is_playback) override;



private:
    void init_agent();


    static int onregister(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const char *user,
        const char *location, int expires);
    static int oninvite(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, struct sip_dialog_t *dialog,
        const void *data, int bytes, void **session);
    static int onack(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
        struct sip_dialog_t *dialog, int code, const void *data, int bytes);
    static int onprack(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
        struct sip_dialog_t *dialog, const void *data, int bytes);
    static int onupdate(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
        struct sip_dialog_t *dialog, const void *data, int bytes);
    static int oninfo(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
        struct sip_dialog_t *dialog, const struct cstring_t *package, const void *data, int bytes);
    static int onbye(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session);
    static int oncancel(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session);
    static int onsubscribe(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t,
        struct sip_subscribe_t *subscribe, void **sub);
    static int onnotify(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *sub,
        const struct cstring_t *event);
    static int onpublish(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const struct cstring_t *event);
    static int onmessage(
        void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session, const void *data,
        int bytes);
    static int onrefer(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session);

private:
    local_account account_; // 本地账户信息
    std::atomic_bool running_ { false };
    uint32_t server_ssrc_domain_{0};
    toolkit::UdpServer::Ptr udp_server_ { nullptr };
    toolkit::TcpServer::Ptr tcp_server_ { nullptr };
    std::shared_ptr<sip_uas_handler_t> handler_ { nullptr };
    std::shared_ptr<sip_agent_t> sip_ { nullptr };

    std::unordered_map<std::string, std::shared_ptr<SuperPlatformImpl>> super_platforms_; // 上级平台
    std::unordered_map<std::string, std::shared_ptr<SubordinatePlatformImpl>> sub_platforms_; // 下级平台
    std::shared_mutex platform_mutex_;
    subordinate_account_callback new_subordinate_account_callback_; // 查找下级平台?
    friend class SipSession;
};
} // namespace gb28181

#endif // gb28181_src_inner_SIP_SERVER_H

/**********************************************************************************************************
文件名称:   sip_server.h
创建时间:   25-2-5 上午10:48
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 上午10:48

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 上午10:48       描述:   创建文件

**********************************************************************************************************/