#include "platform_helper.h"

#include "sip-uac.h"

#include <gb28181/message/message_base.h>
#include <inner/sip_server.h>
#include <inner/sip_session.h>
#include <request/RequestProxyImpl.h>

namespace gb28181 {
class MessageBase;
}
using namespace gb28181;

void add_session(const std::shared_ptr<SipSession> &session);


void PlatformHelper::add_session(const std::shared_ptr<SipSession> &session) {
    std::lock_guard<decltype(sip_session_mutex_)> lck(sip_session_mutex_);
    auto index = session->is_udp() ? 0 : 1;
    if (sip_session_[index] == session)
        return;
    sip_session_[index] = session;

    // 发送错误时主动关闭释放
    session->set_on_error([weak_this = weak_from_this(), index, ptr = session.get()](const toolkit::SockException &e) {
        if (auto this_ptr = weak_this.lock()) {
            std::lock_guard<decltype(sip_session_mutex_)> lck(this_ptr->sip_session_mutex_);
            if (this_ptr->sip_session_[index] && this_ptr->sip_session_[index].get() == ptr) {
                this_ptr->sip_session_[index].reset();
            }
        }
    });
}

void PlatformHelper::get_session(
    const std::function<void(const toolkit::SockException &, std::shared_ptr<SipSession>)> &cb, bool udp) {
    std::lock_guard<decltype(sip_session_mutex_)> lck(sip_session_mutex_);
    if (udp && sip_session_[0]) {
        return cb({}, sip_session_[0]);
    }
    if (!udp && sip_session_[1]) {
        return cb({}, sip_session_[1]);
    }
    if (auto server = local_server_weak_.lock()) {
        server->get_client(
            udp ? TransportType::udp : TransportType::tcp, sip_account().host, sip_account().port,
            [weak_this = weak_from_this(), cb](const toolkit::SockException &e, std::shared_ptr<SipSession> session) {
                if (!e && session) {
                    if (auto this_ptr = weak_this.lock()) {
                        this_ptr->add_session(session);
                    }
                }
                cb(e, session);
            });
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
    if (!server) return nullptr;
    return server->get_sip_agent().get();
}
std::string PlatformHelper::get_from_uri() {
    return from_uri_;
}
std::string PlatformHelper::get_to_uri() {
    if (to_uri_.empty()) {
        to_uri_ = "sip:" + sip_account().platform_id + "@" + sip_account().host + ":" + std::to_string(sip_account().port);
    }
    return to_uri_;
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
    std::shared_ptr<sip_uac_transaction_t> transaction, std::string &&payload,
    const std::function<void(bool, std::string)> &rcb, bool udp) {
    get_session([transaction, payload = std::move(payload),rcb](const toolkit::SockException &e, const std::shared_ptr<SipSession>& session) {
        if (e) {
            return rcb(false, e.what());
        }
        if (0!= sip_uac_send(transaction.get(), payload.data(), static_cast<int>(payload.size()),SipSession::get_transport(), session.get())) {
            return rcb(false, "failed to send request, call sip_uac_send failed");
        }
        rcb(true, {});
    },udp);
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