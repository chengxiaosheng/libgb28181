#include "sip_session.h"
#include "http-parser.h"

#include <sip-agent.h>
#include <sip-message.h>

using namespace toolkit;

namespace gb28181 {
SipSession::SipSession(const toolkit::Socket::Ptr &sock)
    : Session(sock)
    , _sip_parse(
          std::shared_ptr<http_parser_t>(
              http_parser_create(HTTP_PARSER_MODE::HTTP_PARSER_REQUEST, nullptr, nullptr),
              [](http_parser_t *parser) { http_parser_destroy(parser); })) {
    socklen_t addr_len = sizeof(_addr);
    getpeername(sock->rawFD(), (struct sockaddr *)&_addr, &addr_len);
    _is_udp = sock->sockType() == SockNum::Sock_UDP;
}
SipSession::~SipSession() {}

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
        timeout_sec, local_ip, local_port);
}

void SipSession::onSockConnect(const toolkit::SockException &ex) {
    TraceL << getIdentifier() << " connect result: " << ex;
    if (ex) {
        // 连接失败  [AUTO-TRANSLATED:33415985]
        // Connection failed
        _timer.reset();
        return;
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
        ss << http_get_content(parser);
    }
    return ss.str();
}

void SipSession::onRecv(const toolkit::Buffer::Ptr &buffer) {
    TraceL << "recv " << buffer->size() << " bytes";
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
        struct sip_message_t *request = sip_message_create(_wait_type == HTTP_PARSER_REQUEST ? SIP_MESSAGE_REQUEST : SIP_MESSAGE_REPLY);
        ret = sip_message_load(request, _sip_parse.get());
        TraceP(this) << " recv: \n" << print_recv_message(_sip_parse.get(), (HTTP_PARSER_MODE)_wait_type);
        sip_agent_set_rport(request, get_peer_ip().c_str(), get_peer_port());
        assert(0 == sip_agent_input(_sip_agent, request, this));
        sip_message_destroy(request); // 释放sip_message_t
        _wait_type = 0;
    } else if (ret < 0) {
        _wait_type = 0;
        WarnP(this)  << ", can't parse sip message :" << buffer->data();
    }
}

void SipSession::onError(const toolkit::SockException &err) {

}
void SipSession::onManager() {}

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