#include "gb28181/sip_common.h"
#include "sip-agent.h"
#include "sip-transport.h"
#include "uri-parse.h"
#include <Thread/WorkThreadPool.h>
#include <Util/NoticeCenter.h>
#include <sip-uac.h>
#include <sip-uas.h>

#include "inner/sip_event.h"
#include "inner/sip_session.h"
#include "sip_server.h"

#include "../../3rdpart/media-server/libsip/src/uas/sip-uas-transaction.h"

using namespace toolkit;

namespace gb28181 {

static SipServer::Protocol get_sip_protocol(const struct cstring_t *protocol) {
    if (!protocol)
        return SipServer::Protocol::UDP;
    if (strcasestr(protocol->p, "udp"))
        return SipServer::Protocol::UDP;
    if (strcasestr(protocol->p, "tcp"))
        return SipServer::Protocol::TCP;
    return SipServer::Protocol::UDP;
}

SipServer::SipServer(LocalServer *local_server)
    : handler_(std::make_shared<sip_uas_handler_t>())
    , local_server_(local_server)

{
    init_agent();
}

void SipServer::init_agent() {
    handler_->send = send;
    handler_->onregister = onregister;
    handler_->oninvite = oninvite;
    handler_->onack = onack;
    handler_->onprack = onprack;
    handler_->onupdate = onupdate;
    handler_->oninfo = oninfo;
    handler_->onbye = onbye;
    handler_->oncancel = oncancel;
    handler_->onsubscribe = onsubscribe;
    handler_->onnotify = onnotify;
    handler_->onpublish = onpublish;
    handler_->onmessage = onmessage;
    handler_->onrefer = onrefer;

    sip_ = std::shared_ptr<sip_agent_t>(
        sip_agent_create(handler_.get()), [](sip_agent_t *agent) { sip_agent_destroy(agent); });
}

int SipServer::send_data(
    const std::shared_ptr<SipSession> &session_ptr, toolkit::Buffer::Ptr data, Protocol protocol,
    sockaddr_storage &addr) {
    if (!session_ptr)
        return sip_unknown_host;
    if (session_ptr->is_udp() && protocol == SipServer::Protocol::UDP) {
        // udp 需要指定目标地址
        session_ptr->getSock()->send(std::move(data), (sockaddr *)&addr);
        return sip_ok;
    } else if (!session_ptr->is_udp() && protocol == SipServer::Protocol::TCP) {
        session_ptr->send(std::move(data));
        return sip_ok;
    }
    return sip_no_network;
}

SipServer::~SipServer() = default;

void SipServer::start(uint16_t local_port, const char *local_ip, Protocol protocol) {
    if (running_.exchange(true)) {
        WarnL << "Server already started";
        return;
    }
    local_ip_ = local_ip;
    auto poller = EventPollerPool::Instance().getPoller();
    auto weak_this = weak_from_this();
    if (protocol & Protocol::UDP) {
        udp_server_ = std::make_shared<UdpServer>();
        udp_server_->setOnCreateSocket(
            [weak_this](const EventPoller::Ptr &poller, const Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
                auto socket = Socket::createSocket(poller, false);
                if (auto this_ptr = weak_this.lock()) {
                    this_ptr->udp_sockets_[poller.get()] = socket;
                }
                return socket;
            });
        udp_server_->start<SipSession>(local_port, local_ip, [weak_this](const std::shared_ptr<SipSession> &session) {
            session->_sip_server = weak_this;
            session->_sip_agent = weak_this.lock()->sip_.get();
        });
        udp_server_->setOnCreateSocket({});
    }
    if (protocol & Protocol::TCP) {
        tcp_server_ = std::make_shared<TcpServer>();
        tcp_server_->start<SipSession>(
            local_port, local_ip, 1024, [weak_this](const std::shared_ptr<SipSession> &session) {
                session->_sip_server = weak_this;
                session->_sip_agent = weak_this.lock()->sip_.get();
            });
    }
}
void SipServer::shutdown() {
    udp_sockets_.clear();
    udp_server_.reset();
    tcp_server_.reset();
}

uint16_t SipServer::getPort() {
    if (udp_server_) {
        return udp_server_->getPort();
    } else if (tcp_server_) {
        return tcp_server_->getPort();
    }
    return 0;
}

void SipServer::get_client(
    Protocol protocol, const std::string &host, uint16_t port,
    const std::function<void(const toolkit::SockException &e, std::shared_ptr<SipSession>)> &cb) {
    auto poller = EventPollerPool::Instance().getPoller();
    if (protocol == UDP && udp_server_) {
        auto sock_ptr = udp_sockets_[poller.get()].lock();
        if (!sock_ptr) {
            sock_ptr->bindUdpSock(udp_server_->getPort(), local_ip_);
        }
        auto session = std::make_shared<SipSession>(sock_ptr);
        session->_sip_server = weak_from_this();
        session->_sip_agent = sip_.get();
        // udp下，只是作为发送，不需要绑定数据接收逻辑~
        cb({}, session);
    } else if (protocol == TCP && tcp_server_) {
        auto sock_ptr = Socket::createSocket(poller, false);
        auto session = std::make_shared<SipSession>(sock_ptr);
        session->_sip_server = weak_from_this();
        session->_sip_agent = sip_.get();
        // tcp 下sipsession 模拟tcpclient 操作, 尝试往对端建立连接
        session->startConnect(
            host, port, tcp_server_->getPort(), local_ip_,
            [cb, session](const toolkit::SockException &e) { cb(e, !e ? session : nullptr); }, 5);
    } else {
        cb(toolkit::SockException(Err_other, "当前线程不存在udp socket", sip_no_network), nullptr);
    }
}

#define GetSipSession(s)                                                                                               \
    if (s == nullptr)                                                                                                  \
        return sip_unknown_host;                                                                                       \
    auto session_ptr = std::dynamic_pointer_cast<SipSession>((static_cast<SipSession *>(param))->shared_from_this());  \
    if (session_ptr == nullptr)                                                                                        \
        return sip_unknown_host;

int SipServer::send(
    void *param, const struct cstring_t *protocol, const struct cstring_t *peer, const struct cstring_t *received,
    int rport, const void *data, int bytes) {

    GetSipSession(param);

    std::shared_ptr<uri_t> uri(uri_parse(peer->p, peer->n), uri_free);
    if (uri == nullptr)
        return sip_unknown_host;
    std::string host = cstrvalid(received) ? received->p : uri->host;
    uint16_t port = rport > 0 ? rport : (uri->port ? uri->port : SIP_PORT);
    auto proto = get_sip_protocol(protocol);
    auto buffer = BufferRaw::create();
    buffer->assign((const char *)data, bytes);
    if (isIP(host.c_str())) {
        auto addr = SockUtil::make_sockaddr(
            cstrvalid(received) ? received->p : uri->host, rport > 0 ? rport : (uri->port ? uri->port : SIP_PORT));
        return send_data(session_ptr, std::move(buffer), proto, addr);
    } else {
        auto poller = toolkit::EventPollerPool::Instance().getPoller();
        WorkThreadPool::Instance().getExecutor()->async(
            [host, port, buffer = std::move(buffer), proto, poller, session_ptr]() mutable {
                struct sockaddr_storage addr {};
                if (SockUtil::getDomainIP(host.c_str(), port, addr)) {
                    send_data(session_ptr, std::move(buffer), proto, addr);
                }
                // todo:: 解析失败, 如何处理？
                poller->async([buffer = std::move(buffer), proto, addr, session_ptr]() mutable {
                    send_data(session_ptr, std::move(buffer), proto, addr);
                });
            });
        return sip_ok;
    }
}

int SipServer::onregister(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const char *user,
    const char *location, int expires) {

    GetSipSession(param);
    // 添加引用计数
    sip_uas_transaction_addref(t);
    std::shared_ptr<sip_uas_transaction_t> trans_ref(t, [](sip_uas_transaction_t *p) {
        sip_uas_transaction_release(p); // 释放引用计数
    });
    std::string user_str(user), location_str(location);
    if (NoticeCenter::Instance().emitEvent(kEventOnRegister, session_ptr, trans_ref, user_str, location_str, expires)
        == 0) {
        return sip_undefined_error;
    }
    return sip_ok;
}
int SipServer::oninvite(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, struct sip_dialog_t *dialog,
    const void *data, int bytes, void **session) {
    return sip_undefined_error;
}
int SipServer::onack(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, int code, const void *data, int bytes) {
    return sip_undefined_error;
}

int SipServer::onprack(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, const void *data, int bytes) {
    return sip_undefined_error;
}
int SipServer::onupdate(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, const void *data, int bytes) {
    return sip_undefined_error;
}
int SipServer::oninfo(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, const struct cstring_t *package, const void *data, int bytes) {
    return sip_undefined_error;
}
int SipServer::onbye(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session) {
    return sip_undefined_error;
}
int SipServer::oncancel(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session) {
    return sip_undefined_error;
}
int SipServer::onsubscribe(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, struct sip_subscribe_t *subscribe,
    void **sub) {
    return sip_undefined_error;
}
int SipServer::onnotify(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *sub,
    const struct cstring_t *event) {
    return sip_undefined_error;
}
int SipServer::onpublish(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const struct cstring_t *event) {
    return sip_undefined_error;
}
int SipServer::onmessage(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session, const void *data,
    int bytes) {
    return sip_undefined_error;
}
int SipServer::onrefer(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session) {
    return sip_undefined_error;
}

} // namespace gb28181

/**********************************************************************************************************
文件名称:   sip_server.cpp
创建时间:   25-2-5 上午10:48
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 上午10:48

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 上午10:48       描述:   创建文件

**********************************************************************************************************/