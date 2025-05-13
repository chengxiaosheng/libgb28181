#include "sip_session.h"
#include "http-parser.h"
#include "sip_common.h"

#include <gb28181/sip_event.h>
#include <sip-agent.h>
#include <sip-message.h>
#include <sip-transport.h>

using namespace toolkit;

namespace gb28181 {
SipSession::SipSession(const toolkit::Socket::Ptr &sock, bool is_client)
    : Session(sock)
    , _is_client(is_client)
    , _sip_parse(
          std::shared_ptr<http_parser_t>(
              http_parser_create(HTTP_PARSER_MODE::HTTP_PARSER_REQUEST, nullptr, nullptr),
              [](http_parser_t *parser) { http_parser_destroy(parser); })), ticker_(std::make_shared<toolkit::Ticker>()) {

    if (!is_client) {
        socklen_t addr_len = sizeof(_addr);
        getpeername(sock->rawFD(), (struct sockaddr *)&_addr, &addr_len);
    }
    _is_udp = sock->sockType() == SockNum::Sock_UDP;
    if(sock)
     TraceP(this) << "SipSession::SipSession()";
    else TraceL << "SipSession::SipSession()";
}
SipSession::~SipSession() {
    TraceP(this) << "SipSession::~SipSession()";
}

void SipSession::set_peer(const std::string &host, uint16_t port) {
    // tcp 模式下，与 udp session 模式下都不需要设置对端地址
    if(!is_udp() ||  !get_peer_ip().empty()) {
        return;
    }
    set_peer(SockUtil::make_sockaddr(host.c_str(), port));
}

struct sockaddr_storage SipSession::make_peer_addr(const struct sockaddr_storage &addr) {
    struct sockaddr_storage peer_addr{};
    auto &socket = getSock();
    if (!socket)
        return peer_addr;

    auto listen_ip = socket->get_local_ip();
    struct sockaddr_storage local_addr {};
    if (!SockUtil::get_sock_local_addr(socket->rawFD(), local_addr)) {
        ErrorL << "SockUtil::get_sock_local_addr() failed";
    }
    // 如果类型相同， 则直接复制
    peer_addr = addr;
    if (local_addr.ss_family == AF_INET6 && addr.ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
        struct sockaddr_in6 *peer6 = (struct sockaddr_in6 *)&peer_addr;
        peer6->sin6_family = AF_INET6;
        peer6->sin6_port = addr4->sin_port;
        memset(&peer6->sin6_addr, 0, 10); // 前 10 字节置 0
        peer6->sin6_addr.s6_addr[10] = 0xFF;
        peer6->sin6_addr.s6_addr[11] = 0xFF;
        memcpy(&peer6->sin6_addr.s6_addr[12], &addr4->sin_addr, 4);
    }
    // 应该不会存在这种情况吧？
    else if (local_addr.ss_family == AF_INET && addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
        if (IN6_IS_ADDR_V4MAPPED(&addr6->sin6_addr)) {
            struct sockaddr_in *peer4 = (struct sockaddr_in *)&peer_addr;
            peer4->sin_family = AF_INET;
            memcpy(&peer4->sin_addr, ((char *)&addr6->sin6_addr) + 12, 4);
            peer4->sin_port = addr6->sin6_port;
        } else {
            ErrorL << "Cannot convert non-mapped IPv6 address to IPv4";
        }
    } else {
        ErrorL << "Unknown peer address family";
        throw std::runtime_error("Unknown peer address family");
    }
    return peer_addr;

}

void SipSession::set_peer(const struct sockaddr_storage &addr) {
    // tcp 模式下，与 udp session 模式下都不需要设置对端地址
    if(!is_udp() ||  !get_peer_ip().empty()) {
        return;
    }
    _addr = make_peer_addr(addr);
}

void SipSession::startConnect(
    const std::string &host, uint16_t port, uint16_t local_port, const std::string &local_ip,
    const std::function<void(const toolkit::SockException &ex)> &cb, float timeout_sec) {
    std::weak_ptr<SipSession> weak_self = std::dynamic_pointer_cast<SipSession>(shared_from_this());

    _timer = std::make_shared<Timer>(
        2.0f,
        [weak_self]() {
            auto strong_self = weak_self.lock();
            if (!strong_self) {
                return false;
            }
            strong_self->onManager();
            return true;
        },
        getPoller());

    auto sock_ptr = getSock().get();

    sock_ptr->setOnErr([weak_self, sock_ptr](const SockException &ex) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->getSock().get()) {
            // 已经重连socket，上次的socket的事件忽略掉  [AUTO-TRANSLATED:9bf35a7a]
            // Socket has been reconnected, last socket's event is ignored
            return;
        }
        strong_self->_timer.reset();
        TraceL << strong_self->getIdentifier() << " on err: " << ex;
        strong_self->onError(ex);
    });

    TraceL << getIdentifier() << " start connect " << host << ":" << port;
    sock_ptr->connect(
        host, port,
        [weak_self, cb](const SockException &err) {
            auto strong_self = weak_self.lock();
            if (strong_self) {
                strong_self->onSockConnect(err);
                cb(err);
            }
        },
        timeout_sec);
}

void SipSession::onSockConnect(const toolkit::SockException &ex) {
    TraceL << getIdentifier() << " connect result: " << ex;
    if (ex) {
        // 连接失败  [AUTO-TRANSLATED:33415985]
        // Connection failed
        _timer.reset();
        return;
    }
    if (local_ip_.empty()) {
        local_ip_ = SockUtil::get_local_ip(getSock()->rawFD());
    }

    auto sock_ptr = getSock().get();
    std::weak_ptr<SipSession> weak_self = std::dynamic_pointer_cast<SipSession>(shared_from_this());
    sock_ptr->setOnFlush([weak_self, sock_ptr]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return false;
        }
        if (sock_ptr != strong_self->getSock().get()) {
            // 已经重连socket，上传socket的事件忽略掉  [AUTO-TRANSLATED:243a8c95]
            // Socket has been reconnected, upload socket's event is ignored
            return false;
        }
        strong_self->onFlush();
        return true;
    });

    sock_ptr->setOnRead([weak_self, sock_ptr](const Buffer::Ptr &pBuf, struct sockaddr *, int) {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return;
        }
        if (sock_ptr != strong_self->getSock().get()) {
            // 已经重连socket，上传socket的事件忽略掉  [AUTO-TRANSLATED:243a8c95]
            // Socket has been reconnected, upload socket's event is ignored
            return;
        }
        try {
            strong_self->onRecv(pBuf);
        } catch (std::exception &ex) {
            strong_self->shutdown(SockException(Err_other, ex.what()));
        }
    });
}

int SipSession::sip_via(void *transport, const char *destination, char protocol[16], char local[128], char dns[128]) {
    if (transport == nullptr) return -1;
    auto session = std::dynamic_pointer_cast<SipSession>(static_cast<SipSession *>(transport)->shared_from_this());
    if (!session) {
        WarnL << "SipSession::sip_via: transport is not SipSession";
        return -1;
    }
    snprintf(protocol, 16, "%s", session->is_udp() ? "UDP" : "TCP");
    if (session->local_ip_.empty())
    {
        if (session->getSock() && session->getSock()->rawFD()) {
            session->local_ip_ = SockUtil::get_local_ip(session->getSock()->rawFD());
        }
    }
    if (session->local_ip_.empty()){
        session->local_ip_ = SockUtil::get_local_ip();
    }
    snprintf(local, 128, "%s:%d", session->local_ip_.c_str(), session->get_local_port());
    return 0;
}

// 本地作为客户端的时候会执行此函数
int SipSession::sip_send(void *transport, const void *data, size_t bytes) {
    if (transport == nullptr)
        return -1;
    std::shared_ptr<SipSession> session_ptr;
    if (auto p = dynamic_cast<SipSession*>(static_cast<SipSession*>(transport)); p != nullptr) {
        session_ptr = std::dynamic_pointer_cast<SipSession>(p->shared_from_this());
    }
    if (!session_ptr) {
        WarnL << "SipSession::sip_send: transport is not SipSession";
        return sip_unknown_host;
    }
    session_ptr->ticker_->resetTime(); // 重置保活
    auto buffer = toolkit::BufferRaw::create();
    buffer->assign((const char *)data, bytes);
    TraceL << "sip send :\n" << std::string_view(buffer->data(), buffer->size());

    if(session_ptr->is_udp() && session_ptr->getSock()->get_peer_ip().empty()) {
        if (session_ptr->_addr.ss_family != AF_INET && session_ptr->_addr.ss_family != AF_INET6) {
            WarnL << "SipSession::sip_send: invalid address family";
            return sip_unknown_host;
        }
        session_ptr->getSock()->send(std::move(buffer), (sockaddr *)&session_ptr->_addr, SockUtil::get_sock_len((sockaddr *)&session_ptr->_addr));
    } else {
        session_ptr->send(std::move(buffer));
    }
    return 0;
}
int SipSession::sip_send_reply(
    void *param, const struct cstring_t *protocol, const struct cstring_t *peer, const struct cstring_t *received, int rport, const void *data, int bytes) {

    if (param == nullptr)
        return sip_unknown_host;
    auto session_ptr = std::dynamic_pointer_cast<SipSession>((static_cast<SipSession *>(param))->shared_from_this());
    if (session_ptr == nullptr)
        return sip_unknown_host;
    auto buffer = BufferRaw::create();
    buffer->assign((const char *)data, bytes);
    TraceL << "sip send :\n" << std::string_view(buffer->data(), buffer->size());
    // 这里的判断应该没有意义， reply 一定来自 server 的 session
    if(session_ptr->is_udp() && session_ptr->getSock()->get_peer_ip().empty()) {
        if(received && cstrvalid(received)) {
            auto peer_storage = session_ptr->make_peer_addr(SockUtil::make_sockaddr(received->p, rport));
            auto peer_addr = (struct sockaddr *)&peer_storage;
            session_ptr->ticker_->resetTime();
            session_ptr->getSock()->send(std::move(buffer), peer_addr, SockUtil::get_sock_len(peer_addr));
            return 0;
        }
        if (peer && cstrvalid(peer)) {
            uint16_t port = 5060;
            auto && ip_port_vec = toolkit::split(peer->p, ":");
            port = std::strtoul(ip_port_vec[1].c_str(), nullptr, 10);
            auto peer_storage = session_ptr->make_peer_addr(SockUtil::make_sockaddr(ip_port_vec[0].c_str(), rport));
            auto peer_addr = (struct sockaddr *)&peer_storage;
            session_ptr->getSock()->send(std::move(buffer), peer_addr, SockUtil::get_sock_len(peer_addr));
            return 0;
        }
        session_ptr->ticker_->resetTime();
        session_ptr->getSock()->send(std::move(buffer), (sockaddr *)&session_ptr->_addr, SockUtil::get_sock_len((sockaddr *)&session_ptr->_addr));
    } else {
        session_ptr->ticker_->resetTime();
        session_ptr->send(std::move(buffer));
    }
    return 0;
}
sip_transport_t *SipSession::get_transport() {
    static sip_transport_t transport;
    transport.send = SipSession::sip_send;
    transport.via = SipSession::sip_via;
    return &transport;
}



std::string print_recv_message(const struct http_parser_t* parser, HTTP_PARSER_MODE mode) {
    std::stringstream ss;
    int i, r;
    {
        std::string protocol(10, ' ');
        int vermajor,verminor;
        r = http_get_version(parser, protocol.data(), &vermajor, &verminor);
        ss << trim(std::move(protocol), " ")  << "/" << vermajor << "." << verminor;
    }
    ss << "\n";
    if (mode == HTTP_PARSER_MODE::HTTP_PARSER_REQUEST) {
        ss << http_get_request_method(parser);
        ss << ' ';
        ss << http_get_request_uri(parser);
    } else {
        ss << http_get_status_code(parser);
        ss << ' ';
        ss << http_get_status_reason(parser);
    }
    ss << "\n";
    const char* name;
    const char* value;
    for (i = 0; i < http_get_header_count(parser) && 0 == r; i++)
    {
        if(0 != http_get_header(parser, i, &name, &value))
            continue;
        ss << name << ": " << value << "\n";
    }
    if (http_get_content_length(parser)) {
        ss << "\n";
        ss << std::string_view((char *)http_get_content(parser), http_get_content_length(parser));
    }
    return ss.str();
}

void SipSession::handle_recv() {
    std::shared_ptr<Buffer> buffer;
    {
        std::lock_guard<decltype(_recv_buffers_mutex)> lck(_recv_buffers_mutex);
        if (_recv_buffers.empty())
            return;
        buffer = _recv_buffers.front();
        _recv_buffers.pop_front();
    }
    if (buffer->size() == 0) {
        return handle_recv();
    }

    if (_wait_type == 0) {
        if (strncasecmp("SIP", buffer->data(), 3) == 0) {
            http_parser_reset(_sip_parse.get(), HTTP_PARSER_RESPONSE);
            _wait_type = HTTP_PARSER_RESPONSE;
        } else {
            http_parser_reset(_sip_parse.get(), HTTP_PARSER_REQUEST);
            _wait_type = HTTP_PARSER_REQUEST;
        }
    }
    auto len = buffer->size();
    auto ret = http_parser_input(_sip_parse.get(), buffer->data(), &len);
    if (ret == 0) {
        struct sip_message_t *request
            = sip_message_create(_wait_type == HTTP_PARSER_REQUEST ? SIP_MESSAGE_REQUEST : SIP_MESSAGE_REPLY);
        ret = sip_message_load(request, _sip_parse.get());
        TraceP(this) << " recv: \n" << print_recv_message(_sip_parse.get(), (HTTP_PARSER_MODE)_wait_type);
        sip_agent_set_rport(request, get_peer_ip().c_str(), get_peer_port());

        // 捕获this_ptr 防止当前session 被干掉
        // 异步执行 sip_agent_input 避免消息在网络侧积压
        // 一一般一个平台都在同一个poller中处理， 这里是否有必要将事件分摊到不同的线程？
        // EventPollerPool::Instance().getPoller(false);
        getPoller()->async([this_ptr = std::dynamic_pointer_cast<SipSession>(shared_from_this()), request]() {
            auto ret = sip_agent_input(this_ptr->_sip_agent, request, this_ptr.get());
            if (ret != 0) {
                WarnL << "input failed";
            }
        });

        _wait_type = 0;
        // 数据有余量
        if (len) {
            auto temp_buffer = BufferRaw::create();
            temp_buffer->assign(buffer->data() + buffer->size() - len, len);
            std::lock_guard<decltype(_recv_buffers_mutex)> lck(_recv_buffers_mutex);
            _recv_buffers.insert(_recv_buffers.begin(), temp_buffer);
        }
    } else if (ret < 0) {
        _wait_type = 0;
        WarnP(this)  << ", can't parse sip message :" << buffer->data();
    }
    handle_recv();
}


void SipSession::onRecv(const toolkit::Buffer::Ptr &buffer) {
    TraceL << "recv " << buffer->size() << " bytes" << ", local: " << get_local_ip() << ", remote: " << get_peer_ip();
    ticker_->resetTime();
    {
        std::lock_guard<decltype(_recv_buffers_mutex)> lck(_recv_buffers_mutex);
        _recv_buffers.push_back(buffer);
    }
    getPoller()->async([weak_this = weak_from_this()]() {
        if (auto this_ptr = std::dynamic_pointer_cast<SipSession>(weak_this.lock())) {
            this_ptr->handle_recv();
        }
    }, false);
}

void SipSession::onError(const toolkit::SockException &err) {
    WarnP(this) << ", " << err;
    if (_on_error) _on_error(err);
}
void SipSession::onManager() {
    // udp client 未纳入udpServer session 管理，所以不会执行次函数
    // Tcp client 同样未纳入
    if (ticker_->elapsedTime() > 60 * 1000) {
        shutdown(SockException(Err_timeout, "session timeout"));
    }
}

} // namespace gb28181

/**********************************************************************************************************
文件名称:   sip_session.cpp
创建时间:   25-2-5 上午10:56
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 上午10:56

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 上午10:56       描述:   创建文件

**********************************************************************************************************/