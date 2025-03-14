#include "sip-agent.h"
#include "subordinate_platform_impl.h"
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
#include <sip-message.h>
#include <super_platform_impl.h>

using namespace toolkit;

namespace gb28181 {

static std::unordered_map<uint32_t, std::atomic_int32_t> server_ssrc_sn_;
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
    tinyxml2::XMLUtil::ToUnsigned(account_.platform_id.substr(3, 5).data(), &server_ssrc_domain_);
    server_ssrc_sn_.try_emplace(server_ssrc_domain_, 1);
    init_agent();
}
void SipServer::init_agent() {
    handler_->send = SipSession::sip_send2;
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
std::vector<std::shared_ptr<SubordinatePlatform>> SipServer::get_all_subordinate_platform() {
    std::vector<std::shared_ptr<SubordinatePlatform>> platforms;
    {
        std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
        for (auto &it : sub_platforms_) {
            platforms.emplace_back(it.second);
        }
    }
    return platforms;
}
std::shared_ptr<SuperPlatform> SipServer::get_super_platform(const std::string &platform_id) {
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    if (auto it = super_platforms_.find(platform_id); it != super_platforms_.end()) {
        return it->second;
    }
    return nullptr;
}
std::vector<std::shared_ptr<SuperPlatform>> SipServer::get_all_super_platforms() {
    std::vector<std::shared_ptr<SuperPlatform>> platforms;
    {
        std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
        for (auto &it : super_platforms_) {
            platforms.emplace_back(it.second);
        }
    }
    return platforms;
}
std::shared_ptr<SubordinatePlatform> SipServer::add_subordinate_platform(subordinate_account &&account) {
    std::string platform_id = account.platform_id;
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    auto platform = std::make_shared<SubordinatePlatformImpl>(std::move(account), shared_from_this());
    sub_platforms_[platform_id] = platform;
    return platform;
}
void SipServer::remove_subordinate_platform(const std::string &platform_id) {
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    if (auto it = sub_platforms_.find(platform_id); it != sub_platforms_.end()) {
        it->second->shutdown();
        sub_platforms_.erase(it);
    }
}
std::shared_ptr<SuperPlatform> SipServer::add_super_platform(super_account &&account) {
    std::string platform_id = account.platform_id;
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    auto platform = std::make_shared<SuperPlatformImpl>(std::move(account), shared_from_this());
    super_platforms_[platform_id] = platform;
    platform->start();
    return platform;
}
void SipServer::remove_super_platform(const std::string &platform_id) {
    std::shared_lock<decltype(platform_mutex_)> lock(platform_mutex_);
    if (auto it = super_platforms_.find(platform_id); it != super_platforms_.end()) {
        it->second->shutdown();
        super_platforms_.erase(it);
    }
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
uint32_t SipServer::make_ssrc(bool is_playback) {
    auto &counter = server_ssrc_sn_[server_ssrc_domain_];
    int32_t old_value = counter.load(std::memory_order_relaxed);
    int32_t desired;
    do {
        desired = (old_value >= 9999) ? 1 : old_value + 1;
    } while (!counter.compare_exchange_weak(
        old_value, desired,
        std::memory_order_release, // 确保写入对其他线程可见
        std::memory_order_relaxed));
    return (is_playback ? 1'000'000'000 : 0)
           + (server_ssrc_domain_ * 100'00)
           + old_value;
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
        udp_server_->start<SipSession>(
            account_.port, local_ip_, [weak_this](const std::shared_ptr<SipSession> &session) {
                session->_sip_server = weak_this;
                session->_sip_agent = weak_this.lock()->sip_.get();
            });
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
    if (running_.exchange(false)) {
        std::unordered_map<std::string, std::shared_ptr<SuperPlatformImpl>> super_platforms;
        std::unordered_map<std::string, std::shared_ptr<SubordinatePlatformImpl>> sub_platforms;
        {
            std::unique_lock<decltype(platform_mutex_)> lock(platform_mutex_);
            super_platforms_.swap(super_platforms);
            sub_platforms_.swap(sub_platforms);
        }
        for (const auto &it : super_platforms) {
            it.second->shutdown();
        }
        for (const auto &it : sub_platforms) {
            it.second->shutdown();
        }
        udp_server_.reset();
        tcp_server_.reset();
    }
}

void SipServer::get_client_l(
    bool is_udp, const struct sockaddr_storage &addr,
    const std::function<void(const toolkit::SockException &e, std::shared_ptr<SipSession>)> cb) {
    if (is_udp && udp_server_) {
        if (auto session_ptr = udp_server_->getOrCreateSession(addr)) {
            cb({}, std::dynamic_pointer_cast<SipSession>(session_ptr));
        } else {
            cb(toolkit::SockException(Err_other, "get session failed"), nullptr);
        }
        return;
    }
    if (!tcp_server_)
        return cb(toolkit::SockException(Err_other, "no tcp server"), nullptr);
    auto poller = EventPollerPool::Instance().getPoller();
    auto sock_ptr = Socket::createSocket(poller, false);
    auto session = std::make_shared<SipSession>(sock_ptr);
    session->_sip_server = weak_from_this();
    session->_sip_agent = sip_.get();
    // tcp 下sipSession 模拟tcpclient 操作, 尝试往对端建立连接
    session->startConnect(
        SockUtil::inet_ntoa((const sockaddr *)&addr), SockUtil::inet_port((const sockaddr *)&addr), account_.port,
        local_ip_, [cb, session](const toolkit::SockException &e) { cb(e, !e ? session : nullptr); }, 5);
}

void SipServer::get_client(
    TransportType protocol, const std::string &host, uint16_t port,
    const std::function<void(const toolkit::SockException &e, std::shared_ptr<SipSession>)> cb) {
    auto poller = EventPollerPool::Instance().getPoller();
    bool is_udp = protocol == TransportType::udp;
    if (is_udp && !udp_server_) {
        is_udp = false;
    }
    bool is_ip = toolkit::isIP(host.c_str());
    auto weak_this = weak_from_this();
    if (!toolkit::isIP(host.c_str())) {
        WorkThreadPool::Instance().getPoller()->async([poller, host, port, cb, weak_this, is_udp]() {
            struct sockaddr_storage addr;
            if (!SockUtil::getDomainIP(
                    host.c_str(), port, addr, AF_INET, SOCK_DGRAM, is_udp ? IPPROTO_UDP : IPPROTO_TCP)) {
                poller->async(
                    [cb, host]() { cb(toolkit::SockException(Err_dns, "dns resolution failed" + host), nullptr); });
                return;
            }
            // 切回原始线程继续处理
            poller->async([weak_this, is_udp, addr, cb]() {
                if (auto this_ptr = weak_this.lock()) {
                    this_ptr->get_client_l(is_udp, addr, cb);
                } else {
                    cb(toolkit::SockException(Err_other, "server destruction"), nullptr);
                }
            });
        });
    } else {
        auto addr = toolkit::SockUtil::make_sockaddr(host.c_str(), port);
        get_client_l(is_udp, addr, cb);
    }
}

#define GetSipSession(s)                                                                                               \
    if (s == nullptr)                                                                                                  \
        return sip_unknown_host;                                                                                       \
    auto session_ptr = std::dynamic_pointer_cast<SipSession>((static_cast<SipSession *>(s))->shared_from_this());      \
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

int SipServer::onregister(
    void *param, const struct sip_message_t *req, struct sip_uas_transaction_t *t, const char *user,
    const char *location, int expires) {

    GetSipSession(param);
    RefTransaction(t);
    RefSipMessage(req);

    // 设置通用 header
    set_message_header(t);
    std::string user_str(user), location_str(location);
    return SubordinatePlatformImpl::on_recv_register(session_ptr, trans_ref, req_ptr, user_str, location_str, expires);
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
    DebugL << "on notify , session = " << sub;
    if (auto subscribe = SubscribeRequestImpl::get_subscribe(sub)) {
        GetSipSession(param);
        RefTransaction(t);
        RefSipMessage(req);
        return subscribe->on_recv_notify(session_ptr, req_ptr, trans_ref);
    }
    // 如果通知负载数据，但无法找到订阅, 将notify消息降级为 message 消息处理
    if (req->payload && req->size > 0) {
        return onmessage(param, req, t, sub, req->payload, req->size);
    }
    // 直接返回481 会话不存在
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
    return PlatformHelper::on_recv_message(session_ptr, trans_ref, req_ptr, session);
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