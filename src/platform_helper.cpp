#include "gb28181/sip_event.h"
#include "gb28181/type_define_ext.h"
#include "inner/sip_common.h"
#include "sip-message.h"
#include "sip-uac.h"
#include "subordinate_platform_impl.h"
#include "super_platform_impl.h"
#include <sip-uas.h>

#include <Util/NoticeCenter.h>
#include <gb28181/message/message_base.h>
#include <inner/sip_server.h>
#include <inner/sip_session.h>
#include <request/RequestProxyImpl.h>

#include "platform_helper.h"


#include <Network/sockutil.h>
#include <algorithm>
#include <uac/sip-uac-transaction.h>

using namespace toolkit;

namespace gb28181 {
class MessageBase;
}
using namespace gb28181;

void add_session(const std::shared_ptr<SipSession> &session);

bool areAddressesEqual(const struct sockaddr_storage &a, const struct sockaddr_storage &b) {
    if (a.ss_family != b.ss_family)
        return false;

    switch (a.ss_family) {
        case AF_INET: {
            const sockaddr_in *a4 = reinterpret_cast<const sockaddr_in *>(&a);
            const sockaddr_in *b4 = reinterpret_cast<const sockaddr_in *>(&b);
            // 检查 BSD 系统的 sin_len 字段（条件编译）
#ifdef HAS_SIN_LEN
            if (a4->sin_len != b4->sin_len)
                return false;
#endif
            return (a4->sin_port == b4->sin_port) && (memcmp(&a4->sin_addr, &b4->sin_addr, sizeof(in_addr)) == 0);
        }

        case AF_INET6: {
            const sockaddr_in6 *a6 = reinterpret_cast<const sockaddr_in6 *>(&a);
            const sockaddr_in6 *b6 = reinterpret_cast<const sockaddr_in6 *>(&b);
#ifdef HAS_SIN6_LEN
            if (a6->sin6_len != b6->sin6_len)
                return false;
#endif
            return (a6->sin6_port == b6->sin6_port) && (memcmp(&a6->sin6_addr, &b6->sin6_addr, sizeof(in6_addr)) == 0)
                && (a6->sin6_scope_id == b6->sin6_scope_id);
        }
        default: return false;
    }
}

PlatformHelper::~PlatformHelper() {
    if (tcp_session_) {
        tcp_session_->safeShutdown();
    }
}
void PlatformHelper::get_session(
    const std::function<void(const toolkit::SockException &, std::shared_ptr<SipSession>)> &cb, bool force_tcp) {
    bool is_udp = !force_tcp && (get_transport() == TransportType::udp || get_transport() == TransportType::both);
    if (!is_udp && tcp_session_) {
        return cb({}, tcp_session_);
    }
    auto poller = toolkit::EventPollerPool::Instance().getPoller();
    if (auto server = local_server_weak_.lock()) {
        if (is_udp) {
            std::shared_ptr<SipSession> session_ptr;
            {
                std::lock_guard<decltype(udp_sip_session_map_mutex_)> lock(udp_sip_session_map_mutex_);
                if (auto it = udp_sip_session_map_.find(poller.get()); it != udp_sip_session_map_.end()) {
                    session_ptr = it->second;
                } else {
                    if (server->udp_server_sockets().size() == 1) {
                        session_ptr = std::make_shared<SipSession>(server->udp_server_sockets().begin()->second);
                        udp_sip_session_map_[poller.get()] = session_ptr;
                    } else {
                        session_ptr = std::make_shared<SipSession>(server->udp_server_sockets().at(poller.get()));
                        udp_sip_session_map_[poller.get()] = session_ptr;
                    }
                    if (session_ptr) {
                        if (remote_addr_.ss_family == AF_INET || remote_addr_.ss_family == AF_INET6) {
                            session_ptr->set_peer(remote_addr_);
                        } else {
                            // 此处只能设置IP地址， 如果是域名 一定会失败
                            try {
                                session_ptr->set_peer(sip_account().host, sip_account().port);
                            } catch (const std::exception &e) {
                                ErrorL << "Exception in set_peer(" << sip_account().host << ":" << sip_account().port <<  "): " << e.what();
                            }
                        }
                        session_ptr->set_local_ip(sip_account().local_host);
                        session_ptr->set_local_port(sip_account().local_port);
                    }
                }
            }

            return cb(
                session_ptr ? SockException() : SockException(toolkit::Err_other, "not found sip udp socket"),
                session_ptr);
        }
        server->get_tcp_client(
            sip_account().host, sip_account().port,
            [cb, weak_this = weak_from_this()](
                const toolkit::SockException &e, const std::shared_ptr<SipSession> &session) {
                if (e) {
                    cb(e, session);
                    return;
                }
                if (auto this_ptr = weak_this.lock()) {
                    session->set_local_ip(this_ptr->sip_account().local_host);
                    session->set_local_port(this_ptr->sip_account().local_port);
                    if (!session->is_udp()) {
                        this_ptr->set_tcp_session(session);
                    }
                }
                cb(e, session);
            });
    } else {
        cb(toolkit::SockException(toolkit::Err_other, "server de"), nullptr);
    }
}

int32_t PlatformHelper::get_new_sn() {
    int32_t old_value = platform_sn_.load(std::memory_order_relaxed);
    while (true) {
        int32_t next_value = (old_value == INT32_MAX) ? 1 : old_value + 1;
        // 比较并更新，如果成功则返回旧值
        if (platform_sn_.compare_exchange_weak(
                old_value, // 期望值
                next_value, // 更新值
                std::memory_order_acquire, // 成功时的内存顺序
                std::memory_order_relaxed // 失败时的内存顺序
                )) {
            return old_value;
        }
    }
}

struct sip_agent_t *PlatformHelper::get_sip_agent() {
    auto server = local_server_weak_.lock();
    if (!server)
        return nullptr;
    return server->get_sip_agent().get();
}
bool PlatformHelper::update_remote_via(std::pair<std::string, uint32_t> val) {
    if (val.first.empty() && val.second == 0)
        return false;
    std::string &host = val.first;
    uint32_t &port = val.second;

    auto &account = sip_account();
    if (account.keep_local_host)
        return false;

    bool changed = false;
    if (!host.empty() && sip_account().local_host != host) {
        sip_account().local_host = host;
        changed = true;
    }
    if (port && sip_account().local_port != port) {
        sip_account().local_port = port;
        changed = true;
    }
    if (changed) {
        contact_uri_ = "sip:" + get_sip_server()->get_account().platform_id + "@" + sip_account().local_host + ":"
            + std::to_string(sip_account().local_port);
        if (tcp_session_) {
            tcp_session_->set_local_ip(host);
            tcp_session_->set_local_port(port);
        }
        // 通知上层应用？
        toolkit::EventPollerPool::Instance().getExecutor()->async(
            [this_ptr = shared_from_this()]() {
                if (auto platform = std::dynamic_pointer_cast<SuperPlatformImpl>(this_ptr)) {
                    NOTICE_EMIT(
                        kEventSuperPlatformContactChangedArgs, Broadcast::kEventSuperPlatformContactChanged,
                        std::dynamic_pointer_cast<SuperPlatform>(platform), this_ptr->sip_account().local_host,
                        this_ptr->sip_account().local_port);
                }
                if (auto platform = std::dynamic_pointer_cast<SubordinatePlatformImpl>(this_ptr)) {
                    NOTICE_EMIT(
                        kEventSubordinatePlatformContactChangedArgs, Broadcast::kEventSubordinatePlatformContactChanged,
                        std::dynamic_pointer_cast<SubordinatePlatform>(platform), this_ptr->sip_account().local_host,
                        this_ptr->sip_account().local_port);
                }
            },
            false);
    }
    return changed;
}
std::string PlatformHelper::get_from_uri() {
    return from_uri_;
}
std::string PlatformHelper::get_to_uri() {
    if (to_uri_.empty()) {
        to_uri_
            = "sip:" + sip_account().platform_id + "@" + sip_account().host + ":" + std::to_string(sip_account().port);
    }
    return to_uri_;
}

std::string PlatformHelper::get_contact_uri() {
    return contact_uri_.empty() ? get_from_uri() : contact_uri_;
}

void PlatformHelper::on_platform_addr_changed(struct sockaddr_storage &addr) {
    bool flag = ((addr.ss_family == AF_INET || addr.ss_family == AF_INET6)) && !areAddressesEqual(remote_addr_, addr);
    if (flag) {
        DebugL << "change platform address " << SockUtil::inet_ntoa((sockaddr *)&addr);
        memcpy(&remote_addr_, &addr, sizeof(struct sockaddr_storage));
        std::lock_guard<decltype(udp_sip_session_map_mutex_)> lock(udp_sip_session_map_mutex_);
        for (auto &it : udp_sip_session_map_) {
            it.second->set_peer(addr); // 这个函数会自动纠正ipv4 与 ipv6
        }
    }
}
void PlatformHelper::add_request_proxy(int32_t sn, const std::shared_ptr<RequestProxyImpl> &proxy) {
    std::unique_lock<decltype(request_map_mutex_)> lck(request_map_mutex_);
    request_map_[sn] = proxy;
}
void PlatformHelper::remove_request_proxy(int32_t sn) {
    std::unique_lock<decltype(request_map_mutex_)> lck(request_map_mutex_);
    request_map_.erase(sn);
}

void PlatformHelper::uac_send(
    const std::shared_ptr<sip_uac_transaction_t> &transaction, std::string &&payload,
    const std::function<void(bool, std::string)> &rcb, bool force_tcp) {
    set_message_contact(transaction.get(), get_contact_uri().c_str());
    set_message_header(transaction.get());
    get_session(
        [transaction, payload = std::move(payload), weak_this = weak_from_this(),
         rcb](const toolkit::SockException &e, const std::shared_ptr<SipSession> &session) {
            if (e) {
                return rcb(false, e.what());
            }
            if (!session) {
                return rcb(false, "got session failed");
            }
            if (0
                != sip_uac_send(
                    transaction.get(), payload.data(), static_cast<int>(payload.size()), SipSession::get_transport(),
                    session.get())) {
                return rcb(false, "failed to send request, call sip_uac_send failed");
            }
            rcb(true, {});
        },
        force_tcp);
}
void PlatformHelper::uac_send2(
    const std::shared_ptr<sip_uac_transaction_t> &transaction, std::string &&payload,
    const std::function<void(bool, std::string, const std::shared_ptr<SipSession> &)> &rcb, bool force_tcp) {
    set_message_contact(transaction.get(), get_contact_uri().c_str());
    set_message_header(transaction.get());
    get_session(
        [transaction, payload = std::move(payload), weak_this = weak_from_this(),
         rcb](const toolkit::SockException &e, const std::shared_ptr<SipSession> &session) {
            if (e) {
                return rcb(false, e.what(), session);
            }
            if (!session) {
                return rcb(false, "got session failed", nullptr);
            }
            if (0
                != sip_uac_send(
                    transaction.get(), payload.data(), static_cast<int>(payload.size()), SipSession::get_transport(),
                    session.get())) {
                return rcb(false, "failed to send request, call sip_uac_send failed", session);
            }
            rcb(true, {}, session);
        },
        force_tcp);
}
void PlatformHelper::uac_send3(
    const std::shared_ptr<sip_uac_transaction_t> &transaction, std::string &&payload,
    const std::function<void(bool, std::string)> &ecb, SipReplyCallback rcb, bool force_tcp) {

    struct uac_context {
        SipReplyCallback rcb; // 结果回调函数
        std::shared_ptr<SipSession> session; // 加一层保险， 避免session 中途被释放
        std::shared_ptr<sip_uac_transaction_t> transaction;
    };
    static auto adapter = [](void* param, const struct sip_message_t* reply, struct sip_uac_transaction_t* t, int code) -> int {
        auto context = reinterpret_cast<uac_context*>(param);
        if (!context) return -1;
        SipReplyCallback callback = context->rcb;
        std::shared_ptr<SipSession> session_ptr = context->session;
        auto transaction = context->transaction;

        std::shared_ptr<sip_message_t> reply_ptr(const_cast<sip_message_t *>(reply), sip_message_destroy);

        if (!SIP_IS_SIP_INFO(code)) {
            delete context;
        }
        return callback ? callback(session_ptr, reply_ptr, transaction, code) : 0;
    };
    set_message_contact(transaction.get(), get_contact_uri().c_str());
    set_message_header(transaction.get());

    get_session(
    [transaction, payload = std::move(payload), weak_this = weak_from_this(),rcb = std::move(rcb),ecb](const toolkit::SockException &e, const std::shared_ptr<SipSession> &session) {
        if (e) {
            return ecb(false, e.what());
        }
        if (!session) {
            return ecb(false, "got session failed");
        }
        auto context = new uac_context();
        context->rcb = rcb;
        context->session = session;
        context->transaction = transaction;
        transaction->onreply = adapter;
        transaction->param = context;


        // todo: 采用自定义状态码， 来明确处理错误信息
        int ret = sip_uac_send(
                transaction.get(), payload.data(), static_cast<int>(payload.size()), SipSession::get_transport(),
                session.get());
        if(ret != 0) {
            delete context; // 发送失败，清理资源
            ecb(false, "failed to send request, call sip_uac_send failed");
        }
    },
    force_tcp);
}


void PlatformHelper::set_tcp_session(const std::shared_ptr<SipSession> &session) {
    if (session->is_udp()) {
        return;
    }
    session->set_on_error([session_p = session.get(), weak_this = weak_from_this()](const toolkit::SockException &e) {
        if (auto this_ptr = weak_this.lock()) {
            if (this_ptr->tcp_session_.get() == session_p) {
                this_ptr->tcp_session_.reset();
            }
        }
    });
    tcp_session_ = session;
}
void PlatformHelper::set_platform_status_cb(void *user_data, std::function<void(PlatformStatusType)> cb) {
    std::lock_guard<decltype(status_cbs_mtx_)> lck(status_cbs_mtx_);
    status_cbs_[user_data] = std::move(cb);
}
void PlatformHelper::remove_platform_status_cb(void *user_data) {
    std::lock_guard<decltype(status_cbs_mtx_)> lck(status_cbs_mtx_);
    status_cbs_.erase(&user_data);
}

int PlatformHelper::on_response(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    std::shared_ptr<RequestProxyImpl> proxy;
    {
        std::shared_lock<decltype(request_map_mutex_)> lock_guard(request_map_mutex_);
        auto it = request_map_.find(message.sn());
        if (it != request_map_.end()) {
            proxy = it->second;
        }
    }
    if (proxy) {
        return proxy->on_response(std::move(message), std::move(transaction), std::move(request));
    }
    return 404;
}
int PlatformHelper::on_recv_message(
    const std::shared_ptr<SipSession> &session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, void *dialog_ptr) {
    // 验证消息负载
    if (req->payload == nullptr) {
        WarnL << "SIP message payload is null or empty";
        set_message_reason(transaction.get(), "payload is empty");
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    // 验证from 标记中的用户
    auto &&platform_id = get_platform_id(req.get());
    if (platform_id.empty()) {
        ErrorL << "SIP message platform id is empty";
        set_message_reason(transaction.get(), "unable to identify the user");
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }

    auto sip_server = session->getSipServer();
    if (!sip_server) {
        WarnL << "sip_server has been destroyed";
        set_message_reason(transaction.get(), "service is temporarily unavailable");
        return sip_uas_reply(transaction.get(), 503, nullptr, 0, session.get());
    }

    auto xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
    if (xml_ptr->Parse((const char *)req->payload, req->size) != tinyxml2::XML_SUCCESS) {
        WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")" << xml_ptr->ErrorStr()
              << ", xml = " << std::string_view((const char *)req->payload, req->size);
        set_message_reason(transaction.get(), xml_ptr->ErrorStr());
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }

    MessageBase message(xml_ptr);
    if (!message.load_from_xml()) {
        ErrorL << "SIP message load failed: " << message.get_error()
               << "xml = " << std::string_view((const char *)req->payload, req->size);
        set_message_reason(transaction.get(), message.get_error().c_str());
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    DebugL << "recv message " << message;
    if (message.root() == MessageRootType::invalid) {
        WarnL << "SIP message root is invalid";
        set_message_reason(transaction.get(), "invalid root message");
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    if (message.command() == MessageCmdType::invalid) {
        WarnL << "SIP message command is invalid";
        set_message_reason(transaction.get(), "invalid Command");
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }

    // [fold] region get platform
    bool from_super_platform = false;
    // 查询请求 与 设备控制 一定来自上级平台
    if (message.root() == MessageRootType::Query || message.root() == MessageRootType::Control) {
        from_super_platform = true;
    } else if (message.root() == MessageRootType::Notify && message.command() == MessageCmdType::Broadcast) {
        // 语音广播 一般由上级发起
        from_super_platform = true;
    }
    std::shared_ptr<PlatformHelper> platform_;
    if (from_super_platform) {
        platform_ = std::dynamic_pointer_cast<SuperPlatformImpl>(sip_server->get_super_platform(platform_id));
    } else {
        platform_
            = std::dynamic_pointer_cast<SubordinatePlatformImpl>(sip_server->get_subordinate_platform(platform_id));
        if (platform_) {
            // 更新来源地址, 方便向下级平台发送消息
            struct sockaddr_storage addr {};
            if (SockUtil::get_sock_peer_addr(session->getSock()->rawFD(), addr)) {
                platform_->on_platform_addr_changed(addr);
            }
        }
    }
    if (!platform_) {
        WarnL << "platform was not found";
        if (message.command() == MessageCmdType::Keepalive) {
            return 0;
        }
        set_message_reason(transaction.get(), "user not logged in");
        return sip_uas_reply(transaction.get(), 401, nullptr, 0, session.get());
    }
    CharEncodingType message_encoding { message.encoding() };
    if (message_encoding == CharEncodingType::invalid) {
        message_encoding = platform_->sip_account().encoding;
        message.encoding(message_encoding);
    } else if (platform_->sip_account().encoding == CharEncodingType::invalid) {
        platform_->sip_account().encoding = message_encoding;
    }
    // [fold] endregion get platform
    std::string convert_xml_str;
    switch (message.encoding()) {
        case CharEncodingType::gbk: convert_xml_str = gbk_to_utf8(static_cast<const char *>(req->payload)); break;
        case CharEncodingType::gb2312: convert_xml_str = gb2312_to_utf8(static_cast<const char *>(req->payload)); break;
        default: break;
    }
    // 基于指定编码重新解析xml
    if (!convert_xml_str.empty()) {
        xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
        if (xml_ptr->Parse(convert_xml_str.c_str(), convert_xml_str.size()) != tinyxml2::XML_SUCCESS) {
            WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")"
                  << xml_ptr->ErrorStr() << ", xml = " << convert_xml_str;
            set_message_reason(transaction.get(), xml_ptr->ErrorStr());
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
        message = MessageBase(xml_ptr);
        if (!message.load_from_xml()) {
            ErrorL << "SIP message load failed: " << message.get_error() << ", xml = " << convert_xml_str;
            set_message_reason(transaction.get(), message.get_error().c_str());
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
    }

    DebugL << "handle message " << message;

    // 如果是应答消息，一定来自下级平台
    int sip_code = 0;
    switch (message.root()) {
        case MessageRootType::Query: sip_code = platform_->on_query(std::move(message), transaction, req); break;
        case MessageRootType::Control: sip_code = platform_->on_control(std::move(message), transaction, req); break;
        case MessageRootType::Notify: sip_code = platform_->on_notify(std::move(message), transaction, req); break;
        case MessageRootType::Response: sip_code = platform_->on_response(std::move(message), transaction, req); break;
        default: break;
    }
    if (sip_code > 0) {
        return sip_uas_reply(transaction.get(), sip_code, nullptr, 0, session.get());
    }
    return 0;
}

int PlatformHelper::on_query(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    return 400;
}
int PlatformHelper::on_notify(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    return 400;
}
int PlatformHelper::on_control(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    return 400;
}

/**********************************************************************************************************
文件名称:   platform_helper.cpp
创建时间:   25-2-11 下午5:21
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午5:21

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午5:21       描述:   创建文件

**********************************************************************************************************/