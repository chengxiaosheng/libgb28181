#include "gb28181/sip_common.h"
#include "sip-agent.h"
#include "sip-transport.h"
#include "subordinate_platform_impl.h"
#include "uri-parse.h"
#include <Thread/WorkThreadPool.h>
#include <Util/NoticeCenter.h>
#include <sip-uac.h>
#include <sip-uas.h>

#include "inner/sip_session.h"
#include "sip_server.h"

#include "request/invite_request_impl.h"
#include "request/subscribe_request_impl.h"
#include "sip-subscribe.h"
#include "sip_common.h"

#include <gb28181/super_platform.h>
#include <handler/uas_message_handler.h>
#include <sip-message.h>
#include <super_platform_impl.h>

using namespace toolkit;

namespace gb28181 {

static TransportType get_sip_protocol(const struct cstring_t *protocol) {
    if (!protocol)
        return TransportType::udp;
    if (strcasestr(protocol->p, "udp"))
        return TransportType::udp;
    if (strcasestr(protocol->p, "tcp"))
        return TransportType::tcp;
    return TransportType::udp;
}

SipServer::SipServer(local_account account)
    : LocalServer()
    , account_(std::move(account))
    , handler_(std::make_shared<sip_uas_handler_t>())

{
    if (!account_.host.empty()) {
        local_ip_ = account_.host;
    }
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
std::shared_ptr<LocalServer> LocalServer::new_local_server(local_account account) {
    return std::make_shared<SipServer>(std::move(account));
}

std::shared_ptr<SubordinatePlatform> SipServer::get_subordinate_platform(const std::string &platform_id) {
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    if (auto it = sub_platforms_.find(platform_id); it != sub_platforms_.end()) {
        return it->second;
    }
    return nullptr;
}
std::shared_ptr<SuperPlatform> SipServer::get_super_platform(const std::string &platform_id) {
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    if (auto it = super_platforms_.find(platform_id); it != super_platforms_.end()) {
        return it->second;
    }
    return nullptr;
}
void SipServer::add_subordinate_platform(subordinate_account &&account) {
    std::string platform_id = account.platform_id;
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    auto platform = std::make_shared<SubordinatePlatformImpl>(std::move(account), shared_from_this());
    sub_platforms_[platform_id] = platform;
}
void SipServer::add_super_platform(super_account &&account) {
    std::string platform_id = account.platform_id;
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    auto platform = std::make_shared<SuperPlatformImpl>(std::move(account), shared_from_this());
    super_platforms_[platform_id] = platform;
}
void SipServer::reload_account(sip_account account) {}

void SipServer::new_subordinate_account(
    const std::shared_ptr<subordinate_account> &account,
    std::function<void(std::shared_ptr<SubordinatePlatformImpl>)> allow_cb) {
    if (new_subordinate_account_callback_) {
        auto weak_this = weak_from_this();
        new_subordinate_account_callback_(shared_from_this(), account, [weak_this, account, allow_cb](bool allow) {
            if (!allow)
                return allow_cb(nullptr);
            if (auto this_ptr = weak_this.lock()) {
                if (auto platform = this_ptr->get_subordinate_platform(account->platform_id)) {
                    return allow_cb(std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform));
                }
                std::shared_ptr<SubordinatePlatformImpl> platform
                    = std::make_shared<SubordinatePlatformImpl>(*account, this_ptr);
                ;
                {
                    std::unique_lock<decltype(platform_mutex_)> lock(this_ptr->platform_mutex_);
                    this_ptr->sub_platforms_[account->platform_id] = platform;
                }
                return allow_cb(platform);
            }
            return allow_cb(nullptr);
        });
    } else {
        allow_cb(nullptr);
    }
}

int SipServer::send_data(
    const std::shared_ptr<SipSession> &session_ptr, toolkit::Buffer::Ptr data, TransportType protocol,
    sockaddr_storage &addr) {
    if (!session_ptr)
        return sip_unknown_host;
    if (session_ptr->is_udp() && protocol == TransportType::udp) {
        // udp 需要指定目标地址
        session_ptr->getSock()->send(std::move(data), (sockaddr *)&addr);
        return sip_ok;
    } else if (!session_ptr->is_udp() && protocol == TransportType::tcp) {
        session_ptr->send(std::move(data));
        return sip_ok;
    }
    return sip_no_network;
}

SipServer::~SipServer() = default;

void SipServer::run() {
    if (running_.exchange(true)) {
        WarnL << "Server already started";
        return;
    }
    auto poller = EventPollerPool::Instance().getPoller();
    auto weak_this = weak_from_this();
    if (account_.transport_type == TransportType::both || account_.transport_type == TransportType::udp) {
        udp_server_ = std::make_shared<UdpServer>();
        udp_server_->setOnCreateSocket(
            [weak_this](const EventPoller::Ptr &poller, const Buffer::Ptr &buf, struct sockaddr *addr, int addr_len) {
                auto socket = Socket::createSocket(poller, false);
                if (auto this_ptr = weak_this.lock()) {
                    this_ptr->udp_sockets_[poller.get()] = socket;
                }
                return socket;
            });
        udp_server_->start<SipSession>(
            account_.port, local_ip_, [weak_this](const std::shared_ptr<SipSession> &session) {
                session->_sip_server = weak_this;
                session->_sip_agent = weak_this.lock()->sip_.get();
            });
        udp_server_->setOnCreateSocket({});
    }
    if (account_.transport_type == TransportType::both || account_.transport_type == TransportType::tcp) {
        tcp_server_ = std::make_shared<TcpServer>();
        tcp_server_->start<SipSession>(
            account_.port, local_ip_, 1024, [weak_this](const std::shared_ptr<SipSession> &session) {
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

void SipServer::get_client(
    TransportType protocol, const std::string &host, uint16_t port,
    const std::function<void(const toolkit::SockException &e, std::shared_ptr<SipSession>)> cb) {
    auto poller = EventPollerPool::Instance().getPoller();
    if (protocol == TransportType::udp && udp_server_) {
        auto sock_ptr = udp_sockets_[poller.get()].lock();
        if (!sock_ptr) {
            cb(SockException(Err_other, "not found"), nullptr);
            return;
        }
        auto session = std::make_shared<SipSession>(sock_ptr);
        session->_sip_server = weak_from_this();
        session->_sip_agent = sip_.get();
        // udp下，只是作为发送，不需要绑定数据接收逻辑~
        cb({}, session);
    } else if (protocol == TransportType::tcp && tcp_server_) {
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

#define RefTransaction(t)                                                                                              \
    sip_uas_transaction_addref(t);                                                                                     \
    std::shared_ptr<sip_uas_transaction_t> trans_ref(                                                                  \
        t, [](sip_uas_transaction_t *p) { sip_uas_transaction_release(p); });

#define RefSipMessage(m)                                                                                               \
    std::shared_ptr<sip_message_t> req_ptr(                                                                            \
        const_cast<sip_message_t *>(req), [](sip_message_t *req) { sip_message_destroy(req); });

bool isPeerAddressBound(int sock) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (getpeername(sock, (struct sockaddr *)&addr, &addr_len) == -1) {
        return false;
    }
    return true;
}

int SipServer::send(
    void *param, const struct cstring_t *protocol, const struct cstring_t *peer, const struct cstring_t *received,
    int rport, const void *data, int bytes) {

    GetSipSession(param);
    auto buffer = BufferRaw::create();
    buffer->assign((const char *)data, bytes);

    TraceL << "sip send :\n" << std::string_view(buffer->data(), buffer->size());
    // 判断是否绑定了对端地址，如果绑定，则直接发送
    if (isPeerAddressBound(session_ptr->getSock()->rawFD())) {
        session_ptr->send(std::move(buffer));
        return sip_ok;
    }
    std::shared_ptr<uri_t> uri(uri_parse(peer->p, peer->n), uri_free);
    if (uri == nullptr)
        return sip_unknown_host;
    std::string host = (received && cstrvalid(received)) ? std::string(received->p, received->n) : uri->host;
    uint16_t port = rport > 0 ? rport : (uri->port ? uri->port : SIP_PORT);

    auto proto = get_sip_protocol(protocol);
    if (isIP(host.c_str())) {
        auto addr = SockUtil::make_sockaddr(host.c_str(), port);
        session_ptr->getSock()->send(
            std::move(buffer), (sockaddr *)(&addr), SockUtil::get_sock_len((sockaddr *)(&addr)));
        return sip_ok;
    } else {
        auto poller = toolkit::EventPollerPool::Instance().getPoller();
        WorkThreadPool::Instance().getExecutor()->async(
            [host, port, buffer = std::move(buffer), proto, poller, session_ptr]() mutable {
                struct sockaddr_storage addr {};
                if (SockUtil::getDomainIP(host.c_str(), port, addr)) {
                    session_ptr->getSock()->send(
                        std::move(buffer), (sockaddr *)(&addr), SockUtil::get_sock_len((sockaddr *)(&addr)));
                }
            });
        return sip_ok;
    }
}

int SipServer::onregister(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const char *user,
    const char *location, int expires) {

    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);

    // 设置通用 header
    set_message_header(t);
    std::string user_str(user), location_str(location);
    return on_uas_register(session_ptr, trans_ref, req_ptr, user_str, location_str, expires);
}
int SipServer::oninvite(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, struct sip_dialog_t *dialog,
    const void *data, int bytes, void **session) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    std::shared_ptr<sip_dialog_t> dialog_ptr(dialog, [](sip_dialog_t *dialog) {
        if (dialog != nullptr)
            sip_dialog_release(dialog);
    });
    if (dialog) {
        sip_dialog_addref(dialog);
    }
    return InviteRequestImpl::on_recv_invite(session_ptr, req_ptr, trans_ref, dialog_ptr, session);
}
int SipServer::onack(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, int code, const void *data, int bytes) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    std::shared_ptr<sip_dialog_t> dialog_ptr(dialog, [](sip_dialog_t *dialog) {
        if (dialog != nullptr)
            sip_dialog_release(dialog);
    });
    if (dialog) {
        sip_dialog_addref(dialog);
    }
    return InviteRequestImpl::on_recv_ack(session_ptr, req_ptr, trans_ref, dialog_ptr);
}

int SipServer::onprack(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, const void *data, int bytes) {
    sip_message_destroy(const_cast<sip_message_t *>(req));
    set_message_agent(t);
    return sip_uas_reply(t, 404, nullptr, 0, param);
}
int SipServer::onupdate(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, const void *data, int bytes) {
    sip_message_destroy(const_cast<sip_message_t *>(req));
    set_message_agent(t);
    return sip_uas_reply(t, 404, nullptr, 0, param);
}
int SipServer::oninfo(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session,
    struct sip_dialog_t *dialog, const struct cstring_t *package, const void *data, int bytes) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    return InviteRequestImpl::on_recv_info(session_ptr, trans_ref, req_ptr, session);
}
int SipServer::onbye(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    return InviteRequestImpl::on_recv_bye(session_ptr, req_ptr, trans_ref, session);
}
int SipServer::oncancel(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    return InviteRequestImpl::on_recv_cancel(session_ptr, req_ptr, trans_ref, session);
}
int SipServer::onsubscribe(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, struct sip_subscribe_t *subscribe,
    void **sub) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    std::shared_ptr<sip_subscribe_t> subscribe_ptr = nullptr;
    if (subscribe) {
        sip_subscribe_addref(subscribe);
        subscribe_ptr.reset(subscribe, [](struct sip_subscribe_t *ptr) {
            if (ptr)
                sip_subscribe_release(ptr);
        });
    }
    return SubscribeRequestImpl::recv_subscribe_request(session_ptr, req_ptr, trans_ref, subscribe_ptr, sub);
}

int SipServer::onnotify(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *sub,
    const struct cstring_t *event) {
    set_message_agent(t);
    if (auto subscribe = SubscribeRequestImpl::get_subscribe(sub)) {
        GetSipSession(param);
        RefTransaction(t);
        RefSipMessage(req);
        return subscribe->on_recv_notify(session_ptr, req_ptr, trans_ref);
    }
    return sip_uas_reply(t, 481, nullptr, 0, param);
}
int SipServer::onpublish(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const struct cstring_t *event) {
    sip_message_destroy(const_cast<sip_message_t *>(req));
    set_message_agent(t);
    return sip_uas_reply(t, 404, nullptr, 0, param);
}
int SipServer::onmessage(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session, const void *data,
    int bytes) {
    set_message_header(t);
    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);
    if (session) {
        if (InviteRequestImpl::on_recv_message(session_ptr, trans_ref, req_ptr, session) == 0) {
            return 0;
        }
    }
    // 设置通用 header
    return on_uas_message(session_ptr, trans_ref, req_ptr, session);
}
int SipServer::onrefer(void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, void *session) {
    set_message_agent(t);
    sip_message_destroy(const_cast<sip_message_t *>(req));
    return sip_uas_reply(t, 404, nullptr, 0, param);
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