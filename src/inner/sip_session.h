#ifndef gb28181_src_inner_SIP_SESSION_H
#define gb28181_src_inner_SIP_SESSION_H

#include "Network/Session.h"
#ifdef __cplusplus
extern "C" {
struct http_parser_t;
struct sip_agent_t;
struct sip_transport_t;
struct cstring_t;
}
#endif

namespace gb28181 {
class SipServer;

class SipSession : public toolkit::Session {
public:
    SipSession(const toolkit::Socket::Ptr &sock);
    ~SipSession() override;

    void onRecv(const toolkit::Buffer::Ptr &) override;
    void onError(const toolkit::SockException &err) override;
    void onManager() override;
    inline bool is_udp() const { return _is_udp; }
    void startConnect(
        const std::string &host, uint16_t port, uint16_t local_port, const std::string &local_ip,
        const std::function<void(const toolkit::SockException &ex)> &cb, float timeout_sec);

    std::shared_ptr<SipServer> getSipServer() const { return _sip_server.lock(); }

    void set_on_manager(std::function<void()> cb) { _on_manager = std::move(cb); }
    void set_on_error(std::function<void(const toolkit::SockException &ex)> cb) { _on_error = std::move(cb); }

    static int sip_send(void *transport, const void *data, size_t bytes);
    static int sip_send2(void *param, const struct cstring_t *protocol, const struct cstring_t *peer, const struct cstring_t *received, int rport, const void *data, int bytes);
    static int sip_via(void *transport, const char *destination, char protocol[16], char local[128], char dns[128]);

    static sip_transport_t *get_transport();

    void onSockConnect(const toolkit::SockException &ex);

    void set_local_ip(const std::string &ip) { local_ip_ = ip; }
    void set_local_port(uint16_t port) { local_port_ = port; }

private:
    bool _is_udp = false;
    int8_t _wait_type { 0 };
    toolkit::Ticker _ticker;
    struct sockaddr_storage _addr {};
    std::shared_ptr<http_parser_t> _sip_parse;
    sip_agent_t *_sip_agent {};
    std::weak_ptr<SipServer> _sip_server;
    std::shared_ptr<toolkit::Timer> _timer;
    std::string local_ip_;
    uint16_t local_port_ { 0 };
    friend class SipServer;

    std::function<void()> _on_manager;
    std::function<void(const toolkit::SockException &)> _on_error;
    std::shared_ptr<toolkit::Ticker> ticker_; // 计时器
};

} // namespace gb28181

#endif // gb28181_src_inner_SIP_SESSION_H

/**********************************************************************************************************
文件名称:   sip_session.h
创建时间:   25-2-5 上午10:56
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 上午10:56

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 上午10:56       描述:   创建文件

**********************************************************************************************************/