#include "subscribe_request_impl.h"

#include "RequestProxyImpl.h"
#include "gb28181/message/catalog_message.h"
#include "gb28181/message/message_base.h"
#include "gb28181/message/mobile_position_request_message.h"
#include "gb28181/message/ptz_position_message.h"
#include "gb28181/sip_event.h"
#include "inner/sip_common.h"
#include "inner/sip_server.h"
#include "inner/sip_session.h"
#include "sip-message.h"
#include "sip-subscribe.h"
#include "sip-uac.h"
#include "sip-uas.h"
#include "subordinate_platform_impl.h"
#include "super_platform_impl.h"

#include <Util/util.h>

#include <Util/NoticeCenter.h>
#include <utility>
using namespace gb28181;
using namespace toolkit;

static std::unordered_map<void *, std::shared_ptr<SubscribeRequestImpl>> subscribe_request_map_; // 对订阅对象本身的存储
static std::shared_mutex subscribe_request_map_mutex_;
static std::unordered_map<void *, std::shared_ptr<SubscribeRequest::SubscriberNotifyReplyCallback>>
    send_notify_map_; // 存储发送通知消息的结果回调
static std::mutex send_notify_map_mutex_;

std::shared_ptr<SubscribeRequest> SubscribeRequest::new_subscribe(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &message,
    subscribe_info &&info) {
    return std::make_shared<SubscribeRequestImpl>(
        std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform), message, std::move(info));
}

// ReSharper disable once CppDFAConstantFunctionResult
static int on_notify_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (param == nullptr)
        return 0;
    // 忽略中间信息
    if (SIP_IS_SIP_INFO(code)) {
        return 0;
    }
    std::shared_ptr<SubscribeRequest::SubscriberNotifyReplyCallback> rcb;
    {
        std::lock_guard<decltype(send_notify_map_mutex_)> lock(send_notify_map_mutex_);
        if (auto it = send_notify_map_.find(param); it != send_notify_map_.end()) {
            rcb = std::move(it->second);
            send_notify_map_.erase(it);
        }
    }
    if (rcb) {
        std::string reason = get_message_reason(reply);
        (*rcb)(SIP_IS_SIP_SUCCESS(code), toolkit::str_format("recv %d, %s", code, reason.c_str()));
    }
    return 0;
}

SubscribeRequestImpl::SubscribeRequestImpl(
    const std::shared_ptr<SubordinatePlatformImpl> &platform, const std::shared_ptr<MessageBase> &message,
    subscribe_info &&info)
    : SubscribeRequest()
    , platform_(platform)
    , subscribe_message_(message)
    , subscribe_info_(std::move(info))
    , incoming_(false) {}

SubscribeRequestImpl::SubscribeRequestImpl(
    const std::shared_ptr<SuperPlatformImpl> &platform, const std::shared_ptr<sip_subscribe_t> &subscribe_t)
    : SubscribeRequest()
    , platform_(platform)
    , subscribe_time_(getCurrentMicrosecond(true))
    , status_(pending)
    , incoming_(true)
    , expires_(subscribe_t->expires)
    , sip_subscribe_ptr_(subscribe_t) {}

SubscribeRequestImpl::~SubscribeRequestImpl() {
    if (subscribe_message_)
        DebugL << *subscribe_message_ << ", id = " << subscribe_info_.subscribe_id;
    else
        DebugL << "id = " << subscribe_info_.subscribe_id;
}

void SubscribeRequestImpl::start() {
    if (running_.exchange(true)) {
        return;
    }
    add_subscribe();
    if (incoming_) {
        // 告知对端订阅激活
        subscribe_send_notify(
            nullptr,
            [](bool, const std::string &) {

            },
            toolkit::str_format("%s;expires=%d", SIP_SUBSCRIPTION_STATE_ACTIVE, sip_subscribe_ptr_->expires));
        auto weak_this = weak_from_this();
        delay_task_
            = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(sip_subscribe_ptr_->expires, [weak_this]() {
                  auto this_ptr = weak_this.lock();
                  if (!this_ptr)
                      return 0;
                  if (this_ptr->time_remain() <= 1) {
                      this_ptr->shutdown("expiration of subscription");
                  }
                  return 0;
              });
    } else {
        // 如果平台在线，直接发起订阅请求
        if (platform_->sip_account().plat_status.status == PlatformStatusType::online) {
            to_subscribe(subscribe_info_.expires);
        }
        // 订阅平台在线状态
        toolkit::NoticeCenter::Instance().addListener(
            this, Broadcast::kEventSubordinatePlatformStatus,
            [weak_this = weak_from_this()](kEventSubordinatePlatformStatusArgs) {
                if (auto this_ptr = weak_this.lock()) {
                    // 当平台状态为在线时发起订阅请求
                    if (status == PlatformStatusType::online) {
                        this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
                    } else {
                        this_ptr->delay_task_.reset();
                        this_ptr->set_status(terminated, rejected, "platform offline");
                    }
                }
            });
    }
}

void SubscribeRequestImpl::shutdown(std::string reason) {
    if (running_.exchange(false)) {

        // if (incoming_) {
        //     if (status_ != terminated) {
        //         subscribe_send_notify(
        //             nullptr,
        //             [](bool, const std::string &) {
        //
        //             },
        //             toolkit::str_format(
        //                 "%s;reason=%s", SIP_SUBSCRIPTION_STATE_TERMINATED, SIP_SUBSCRIPTION_REASON_REJECTED));
        //         set_status(terminated, rejected, reason);
        //     }
        // } else {
        //     if (status_ != terminated) {
        //         to_subscribe(0);
        //         set_status(terminated, deactivated, reason);
        //     }
        // }
        del_subscribe();
    }
}
std::shared_ptr<SubordinatePlatform> SubscribeRequestImpl::get_sub_platform() {
    return std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform_);
}
std::shared_ptr<SuperPlatform> SubscribeRequestImpl::get_super_platform() {
    return std::dynamic_pointer_cast<SuperPlatformImpl>(get_sub_platform());
}

std::shared_ptr<SubscribeRequestImpl> SubscribeRequestImpl::get_subscribe(void *ptr) {
    if (ptr == nullptr)
        return nullptr;
    std::shared_lock<std::shared_mutex> lock(subscribe_request_map_mutex_);
    if (auto it = subscribe_request_map_.find(ptr); it != subscribe_request_map_.end()) {
        return it->second;
    }
    return nullptr;
}

int SubscribeRequestImpl::recv_subscribe_request(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &message,
    const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<struct sip_subscribe_t> &sip_subscribe_ptr, void **sub) {

    // 针对已有订阅的刷新或取消
    if (auto subscribe = get_subscribe(*sub); subscribe && subscribe->sip_subscribe_ptr_ == sip_subscribe_ptr) {
        auto sip_code = subscribe->on_subscribe(message, transaction);
        set_message_expires(transaction.get(), sip_subscribe_ptr->expires);
        return sip_uas_reply(transaction.get(), sip_code, nullptr, 0, sip_session.get());
    }
    // 广播订阅事件到上层应用
    auto platform_id = get_platform_id(message.get());
    if (platform_id.empty()) {
        set_message_reason(transaction.get(), "unable to verify provenance");
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    auto server = sip_session->getSipServer();
    if (server == nullptr) {
        set_message_reason(transaction.get(), "user not found");
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, sip_session.get());
    }
    // 得到上级平台指针
    auto platform_ptr = std::dynamic_pointer_cast<SuperPlatformImpl>(server->get_super_platform(platform_id));
    if (platform_ptr == nullptr) {
        set_message_reason(transaction.get(), "user not found");
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, sip_session.get());
    }
    if (sip_subscribe_ptr->expires == 0) {
        return sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
    }
    if (sip_subscribe_ptr->expires < 5) {
        // 有效期太短了，不予处理
        set_message_reason(transaction.get(), "validity is too short");
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }

    std::shared_ptr<MessageBase> subscribe_message_ptr = nullptr;
    if (message->payload && message->size) {
        std::shared_ptr<tinyxml2::XMLDocument> xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
        if (xml_ptr->Parse((const char *)message->payload, message->size) != tinyxml2::XML_SUCCESS) {
            WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")"
                  << xml_ptr->ErrorStr()
                  << ", xml = " << std::string_view((const char *)message->payload, message->size);
            set_message_reason(transaction.get(), xml_ptr->ErrorStr());
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
        }
        subscribe_message_ptr = std::make_shared<MessageBase>(xml_ptr);
        if (subscribe_message_ptr->load_from_xml()) {
            if (subscribe_message_ptr->command() == MessageCmdType::Catalog) {
                subscribe_message_ptr = std::make_shared<CatalogRequestMessage>(std::move(*subscribe_message_ptr));
                subscribe_message_ptr->load_from_xml();
            } else if (subscribe_message_ptr->command() == MessageCmdType::MobilePosition) {
                subscribe_message_ptr
                    = std::make_shared<MobilePositionRequestMessage>(std::move(*subscribe_message_ptr));
                subscribe_message_ptr->load_from_xml();
            } else if (subscribe_message_ptr->command() == MessageCmdType::PTZPosition) {
                subscribe_message_ptr = std::make_shared<PTZPositionRequestMessage>(std::move(*subscribe_message_ptr));
                subscribe_message_ptr->load_from_xml();
            }
        }
    }
    // 回复200 ok
    auto subscribe_ptr = std::make_shared<SubscribeRequestImpl>(platform_ptr, sip_subscribe_ptr);
    *sub = subscribe_ptr.get();
    subscribe_ptr->subscribe_message_ = subscribe_message_ptr;
    DebugL << "set subscribe sesstion = " << *sub;
    // 异步广播收到订阅请求
    EventPollerPool::Instance().getPoller()->async(
        [platform_ptr, subscribe_ptr, subscribe_message_ptr]() {
            NOTICE_EMIT(
                kEventOnSubscribeRequestArgs, Broadcast::kEventOnSubscribeRequest,
                std::dynamic_pointer_cast<SuperPlatform>(platform_ptr),
                std::dynamic_pointer_cast<SubscribeRequest>(subscribe_ptr), subscribe_message_ptr);
        },
        false);
    if (sip_subscribe_ptr->expires == 0) {
        sip_subscribe_ptr->expires = 3600;
    }
    set_message_expires(transaction.get(), sip_subscribe_ptr->expires);
    return sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
}

int SubscribeRequestImpl::on_subscribe(
    const std::shared_ptr<sip_message_t> &message, const std::shared_ptr<sip_uas_transaction_t> &transaction) {
    if (sip_subscribe_ptr_->expires == 0) {
        set_status(terminated, deactivated, "deactivated");
        return 200;
    }
    set_status(active, invalid, "");
    return 200;
}

int SubscribeRequestImpl::on_recv_notify(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &notify,
    const std::shared_ptr<sip_uas_transaction_t> &transaction) {

    auto sub_state_cstr = sip_message_get_header_by_name(notify.get(), SIP_HEADER_SUBSCRIBE_STATE);
    if (cstrvalid(sub_state_cstr)) {
        sip_substate_t sub_state {};
        if (0 == sip_header_substate(sub_state_cstr->p, sub_state_cstr->p + sub_state_cstr->n, &sub_state)) {
            if (0 == cstrcasecmp(&sub_state.state, SIP_SUBSCRIPTION_STATE_ACTIVE)) {
                // 如果通知中的有效期大于本地有效期+5秒， 认为服务器自动延长了订阅会话
                if (sub_state.expires && sub_state.expires > time_remain() + 5 && sub_state.expires > 30) {
                    subscribe_time_ = toolkit::getCurrentMicrosecond(true);
                    sip_subscribe_ptr_->expires = subscribe_info_.expires = sub_state.expires;
                    delay_task_ = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(
                        (sub_state.expires - 30) * 1000, [weak_this = weak_from_this()]() {
                            if (auto this_ptr = weak_this.lock()) {
                                this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
                            }
                            return 0;
                        });
                }
                set_status(active, invalid, "");
            } else if (0 == cstrcasecmp(&sub_state.state, SIP_SUBSCRIPTION_STATE_PENDING)) {
                set_status(pending, invalid, "");
            } else if (0 == cstrcasecmp(&sub_state.state, SIP_SUBSCRIPTION_STATE_TERMINATED)) {
                delay_task_.reset();
                if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_DEACTIVATED)) {
                    set_status(terminated, deactivated, "recv deactivated");
                } else if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_PROBATION)) {
                    set_status(terminated, probation, "recv probation");
                } else if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_REJECTED)) {
                    set_status(terminated, rejected, "recv rejected");
                } else if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_TIMEOUT)) {
                    set_status(terminated, timeout, "recv timeout");
                } else if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_GIVEUP)) {
                    set_status(terminated, giveup, "recv giveup");
                } else if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_NORESOURCE)) {
                    set_status(terminated, noresource, "recv noresource");
                } else if (0 == cstrcasecmp(&sub_state.reason, SIP_SUBSCRIPTION_REASON_INVARIANT)) {
                    set_status(terminated, invariant, "recv invariant");
                }
                auto weak_this = weak_from_this();
                // 异步执行重新订阅
                toolkit::EventPollerPool::Instance().getPoller()->async(
                    [weak_this]() {
                        if (auto this_ptr = weak_this.lock()) {
                            this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
                        }
                    },
                    false);
            }
        }
    }
    // 开始处理消息
    if (notify->payload == nullptr || notify->size == 0) {
        return sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
    }

    auto xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
    if (xml_ptr->Parse((const char *)notify->payload, notify->size) != tinyxml2::XML_SUCCESS) {
        WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")" << xml_ptr->ErrorStr()
              << ", xml = " << std::string_view((const char *)notify->payload, notify->size);
        set_message_reason(transaction.get(), xml_ptr->ErrorStr());
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    std::string reason;
    int sip_code = 200;
    if (notify_callback_) {
        sip_code = notify_callback_(shared_from_this(), xml_ptr, reason);
    } else {
        NOTICE_EMIT(
            kEventOnSubscribeNotifyArgs, Broadcast::kEventOnSubscribeNotify,
            std::dynamic_pointer_cast<SubscribeRequest>(shared_from_this()), xml_ptr, reason);
    }
    if (!reason.empty()) {
        set_message_reason(transaction.get(), reason.c_str());
    }
    return sip_uas_reply(transaction.get(), sip_code, nullptr, 0, sip_session.get());
}

void SubscribeRequestImpl::add_subscribe() {
    std::unique_lock<std::shared_mutex> lock(subscribe_request_map_mutex_);
    subscribe_request_map_[this] = shared_from_this();
}
void SubscribeRequestImpl::del_subscribe() {
    std::unique_lock<std::shared_mutex> lock(subscribe_request_map_mutex_);
    subscribe_request_map_.erase(this);
}

int SubscribeRequestImpl::time_remain() const {
    if (!sip_subscribe_ptr_)
        return 0;
    if (sip_subscribe_ptr_->expires == 0)
        return 0;
    return sip_subscribe_ptr_->expires
        - static_cast<int>((toolkit::getCurrentMicrosecond(true) - subscribe_time_) / 1000000L);
}

void SubscribeRequestImpl::send_notify(const std::shared_ptr<MessageBase> &message, SubscriberNotifyReplyCallback rcb) {
    // 如果订阅可用, 则在订阅会话中的 notify 消息中发送
    if (status_ == active) {
        subscribe_send_notify(message, std::forward<SubscriberNotifyReplyCallback>(rcb));
    }
    // 如果订阅不可用，则在message 消息中发送
    else {
        auto request = std::make_shared<RequestProxyImpl>(platform_, message, RequestProxy::RequestType::NoResponse);
        auto callback_ptr = std::make_shared<SubscriberNotifyReplyCallback>(std::move(rcb));
        request->send([callback_ptr](const std::shared_ptr<RequestProxy> &proxy) {
            (*callback_ptr)(proxy->status() == RequestProxy::Status::Succeeded, proxy->error());
        });
    }
}

std::string SubscribeRequestImpl::get_notify_state_str() const {
    std::stringstream ss;
    if (status_ == active) {
        ss << SIP_SUBSCRIPTION_STATE_ACTIVE;
        ss << ";expires=";
        ss << time_remain();
    } else if (status_ == pending) {
        ss << SIP_SUBSCRIPTION_STATE_PENDING;
    } else {
        ss << SIP_SUBSCRIPTION_STATE_TERMINATED;
        ss << ";reason=";
        switch (terminated_type_) {
            case deactivated: ss << SIP_SUBSCRIPTION_REASON_DEACTIVATED; break;
            case timeout: ss << SIP_SUBSCRIPTION_REASON_TIMEOUT; break;
            case rejected: ss << SIP_SUBSCRIPTION_REASON_REJECTED; break;
            case noresource: ss << SIP_SUBSCRIPTION_REASON_NORESOURCE; break;
            case probation: ss << SIP_SUBSCRIPTION_REASON_PROBATION; break;
            case giveup: ss << SIP_SUBSCRIPTION_REASON_GIVEUP; break;
            case expired: ss << "expired"; break;
            case invariant: ss << "invariant"; break;
            default: break;
        }
    }
    return ss.str();
}

void SubscribeRequestImpl::subscribe_send_notify(
    const std::shared_ptr<MessageBase> &message, SubscriberNotifyReplyCallback rcb, std::string state) {
    std::string notify_state(std::move(state));
    if (notify_state.empty()) {
        notify_state = get_notify_state_str();
    }
    sip_agent_t *sip_agent = platform_->get_sip_agent();
    if (!sip_agent) {
        return rcb(false, "sip_agent is nullptr");
    }
    if (message) {
        message->sn(platform_->get_new_sn());
        if (platform_->sip_account().encoding != CharEncodingType::invalid) {
            message->encoding(platform_->sip_account().encoding);
        }
    }
    std::string payload;
    if (message) {
        if (!message->parse_to_xml()) {
            return rcb(false, message->get_error());
        }
        payload = message->str();
    }
    auto callback_ptr = std::make_shared<SubscriberNotifyReplyCallback>(std::move(rcb));
    std::shared_ptr<sip_uac_transaction_t> transaction_ptr(
        sip_uac_notify(sip_agent, sip_subscribe_ptr_.get(), notify_state.c_str(), on_notify_reply, callback_ptr.get()),
        [](sip_uac_transaction_t *t) {
            if (t) {
                sip_uac_transaction_release(t);
            }
        });
    if (!transaction_ptr) {
        return (*callback_ptr)(false, "make sip_uac_transaction_t failed");
    }
    {
        std::lock_guard<decltype(send_notify_map_mutex_)> lock(send_notify_map_mutex_);
        send_notify_map_[callback_ptr.get()] = callback_ptr;
    }
    platform_->uac_send(transaction_ptr, std::move(payload), [callback_ptr](bool ret, std::string err) {
        if (!ret) {
            std::lock_guard<decltype(send_notify_map_mutex_)> lock(send_notify_map_mutex_);
            send_notify_map_.erase(callback_ptr.get());
            return (*callback_ptr)(ret, std::move(err));
        }
    });
}

void SubscribeRequestImpl::set_status(
    SUBSCRIBER_STATUS_TYPE_E status, TERMINATED_TYPE_E terminated_type, std::string reason) {
    status_ = status;
    terminated_type_ = terminated_type;
    error_ = std::move(reason);
    if (status == terminated) {
        terminated_time_ = getCurrentMicrosecond(true);
    }
    auto weak_this = weak_from_this();
    toolkit::EventPollerPool::Instance().getPoller()->async(
        [weak_this]() {
            auto this_ptr = weak_this.lock();
            if (!this_ptr)
                return;
            if (this_ptr->subscriber_callback_) {
                this_ptr->subscriber_callback_(this_ptr, this_ptr->status_, this_ptr->error_);
            }
        },
        false);
}

void SubscribeRequestImpl::to_subscribe(uint32_t expires) {
    sip_agent_t *sip_agent = platform_->get_sip_agent();
    if (!sip_agent)
        return;
    std::string from = platform_->get_from_uri();
    std::string to = platform_->get_to_uri();
    CharEncodingType encoding_type = platform_->get_encoding();
    if (subscribe_message_->sn() == 0) {
        subscribe_message_->sn(platform_->get_new_sn());
    }
    if (!subscribe_info_.subscribe_id) {
        subscribe_info_.subscribe_id = subscribe_message_->sn();
    }
    expires_ = expires;
    std::string event = toolkit::str_format("%s;id=%lu", subscribe_info_.event.c_str(), subscribe_info_.subscribe_id);

    std::shared_ptr<sip_uac_transaction_t> uac_subscribe_transaction;
    if (sip_subscribe_ptr_) {
        // 重订阅，刷新订阅
        uac_subscribe_transaction.reset(
            sip_uac_resubscribe(
                sip_agent, sip_subscribe_ptr_.get(), static_cast<int>(expires),
                SubscribeRequestImpl::on_subscribe_reply, this),
            [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
    } else {
        // 全新订阅
        uac_subscribe_transaction.reset(
            sip_uac_subscribe(
                sip_agent, from.c_str(), to.c_str(), event.c_str(), static_cast<int>(subscribe_info_.expires),
                SubscribeRequestImpl::on_subscribe_reply, this),
            [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
    }
    if (!uac_subscribe_transaction)
        return;
    // 设置通用头信息
    set_message_header(uac_subscribe_transaction.get());
    if (encoding_type != CharEncodingType::invalid) {
        subscribe_message_->encoding(encoding_type);
    }
    set_message_content_type(uac_subscribe_transaction.get(), SipContentType_XML);
    subscribe_message_->parse_to_xml();
    // 如果sip_subscribe_t 不存在, 则设置状态为pending
    if (!sip_subscribe_ptr_) {
        set_status(pending, terminated_type_, "");
    }

    auto weak_this = weak_from_this();
    platform_->uac_send(
        uac_subscribe_transaction, subscribe_message_ ? subscribe_message_->str() : std::string(),
        [weak_this](bool ret, std::string err) {
            auto this_ptr = weak_this.lock();
            if (!this_ptr)
                return;
            if (!ret) {
                this_ptr->delay_task_ = EventPollerPool::Instance().getPoller()->doDelayTask(3 * 1000, [weak_this]() {
                    if (auto this_ptr = weak_this.lock()) {
                        this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
                    }
                    return 0;
                });
                this_ptr->set_status(terminated, invalid, std::move(err));
            }
        });
}

int SubscribeRequestImpl::on_subscribe_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, struct sip_subscribe_t *subscribe,
    int code, void **session) {
    if (!param)
        return 0;
    auto this_ptr = get_subscribe(param);
    if (!this_ptr)
        return 0;
    auto weak_self = this_ptr->weak_from_this();
    // 忽略 100-199
    if (SIP_IS_SIP_ERROR(code)) {
        return 0;
    }
    // 是否订阅成功
    if (SIP_IS_SIP_SUCCESS(code)) {
        if (*session != this_ptr.get()) {
            *session = this_ptr.get();
            DebugL << "set subscribe session = " << this_ptr.get();
        }
        sip_subscribe_addref(subscribe);
        this_ptr->sip_subscribe_ptr_.reset(subscribe, [](struct sip_subscribe_t *p) { sip_subscribe_release(p); });
        // 取消订阅的操作
        subscribe->expires = this_ptr->expires_;
        if (subscribe->expires) {
            this_ptr->subscribe_time_ = toolkit::getCurrentMicrosecond(true);
            this_ptr->delay_task_ = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(
                (subscribe->expires - 5) * 1000, [weak_self]() {
                    if (auto this_ptr = weak_self.lock()) {
                        this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
                    }
                    return 0;
                });
            this_ptr->set_status(active, invalid, "");
        } else {
            this_ptr->delay_task_.reset();
            this_ptr->terminated_time_ = toolkit::getCurrentMicrosecond(true);
            this_ptr->set_status(terminated, deactivated, "");
        }
    } else {
        if (this_ptr->subscribe_time_) {
            this_ptr->terminated_time_ = toolkit::getCurrentMicrosecond(true);
        }
        this_ptr->sip_subscribe_ptr_.reset();
        this_ptr->delay_task_.reset();
        this_ptr->set_status(terminated, rejected, toolkit::str_format("peer return %d", code));
        // 如果是服务器错误后续再试，否则认为不支持
        if (SIP_IS_SIP_SERVER_ERROR(code)) {
            this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
        }
    }
    return 0;
}

/**********************************************************************************************************
文件名称:   subscribe_request_impl.cpp
创建时间:   25-2-15 上午10:54
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-15 上午10:54

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-15 上午10:54       描述:   创建文件

**********************************************************************************************************/