#include "subscribe_request_impl.h"

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

static int on_notify_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (param == nullptr)
        return 0;
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
    , subscribe_info_(std::move(info)) {}

SubscribeRequestImpl::SubscribeRequestImpl(
    const std::shared_ptr<SuperPlatformImpl> &platform, const std::shared_ptr<sip_subscribe_t> &subscribe_t)
    : SubscribeRequest()
    , platform_(platform)
    , incoming_(true)
    , sip_subscribe_ptr_(subscribe_t) {}

SubscribeRequestImpl::~SubscribeRequestImpl() = default;

void SubscribeRequestImpl::start() {
    if (running_.load()) {
        return;
    }
    add_subscribe();
    running_.exchange(true);
    if (incoming_) {
        // 告知对端订阅激活
        subscribe_send_notify(
            nullptr,
            [](bool, const std::string &) {

            },
            toolkit::str_format("%s;expires=%d", SIP_SUBSCRIPTION_STATE_ACTIVE, sip_subscribe_ptr_->expires));
    } else {
        // 如果平台在线，直接发起订阅请求
        std::visit(
            [&](const auto &platform) {
                if (platform->account().plat_status.status == PlatformStatusType::online) {
                    to_subscribe(subscribe_info_.expires);
                }
            },
            platform_);
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
    if (!running_.load()) {
        if (incoming_) {
            subscribe_send_notify(
                nullptr,
                [](bool, const std::string &) {

                },
                toolkit::str_format(
                    "%s;reason=%s", SIP_SUBSCRIPTION_STATE_TERMINATED, SIP_SUBSCRIPTION_REASON_REJECTED));
            set_status(terminated, rejected, reason);
        }
    } else {
        running_.exchange(false);
        if (incoming_) {
            subscribe_send_notify(
                nullptr,
                [](bool, const std::string &) {

                },
                toolkit::str_format(
                    "%s;reason=%s", SIP_SUBSCRIPTION_STATE_TERMINATED, SIP_SUBSCRIPTION_REASON_DEACTIVATED));
            set_status(terminated, deactivated, reason);
        } else {
            to_subscribe(0);
        }
        delay_task_.reset();
    }
    del_subscribe();
}

std::shared_ptr<SubscribeRequestImpl> SubscribeRequestImpl::get_subscribe(void *ptr) {
    if (!ptr)
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
    const std::shared_ptr<struct sip_subscribe_t> &subscribe_ptr, void **sub) {

    // 针对已有订阅的刷新或取消
    if (auto subscribe = get_subscribe(*sub); subscribe && subscribe->sip_subscribe_ptr_ == subscribe_ptr) {
        auto sip_code = subscribe->on_subscribe(message, transaction);
        return sip_uas_reply(transaction.get(), sip_code, nullptr, 0, sip_session.get());
    }
    // 广播订阅事件到上层应用
    auto platform_id = get_platform_id(message.get());
    if (platform_id.empty()) {
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, sip_session.get());
    }
    auto server = sip_session->getSipServer();
    if (server == nullptr) {
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, sip_session.get());
    }
    // 得到上级平台指针
    auto platform_ptr = std::dynamic_pointer_cast<SuperPlatformImpl>(server->get_super_platform(platform_id));
    if (platform_ptr == nullptr) {
        return sip_uas_reply(transaction.get(), 403, nullptr, 0, sip_session.get());
    }

    std::shared_ptr<MessageBase> gb_message_ptr = nullptr;
    if (message->payload && message->size) {
        std::shared_ptr<tinyxml2::XMLDocument> xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
        if (xml_ptr->Parse((const char *)message->payload, message->size) != tinyxml2::XML_SUCCESS) {
            WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")"
                  << xml_ptr->ErrorStr()
                  << ", xml = " << std::string_view((const char *)message->payload, message->size);
            set_message_reason(transaction.get(), xml_ptr->ErrorStr());
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
        }
        gb_message_ptr = std::make_shared<MessageBase>(xml_ptr);
        if (gb_message_ptr->load_from_xml()) {
            if (gb_message_ptr->command() == MessageCmdType::Catalog) {
                gb_message_ptr = std::make_shared<CatalogRequestMessage>(std::move(*gb_message_ptr));
                gb_message_ptr->load_from_xml();
            } else if (gb_message_ptr->command() == MessageCmdType::MobilePosition) {
                gb_message_ptr = std::make_shared<MobilePositionRequestMessage>(std::move(*gb_message_ptr));
                gb_message_ptr->load_from_xml();
            } else if (gb_message_ptr->command() == MessageCmdType::PTZPosition) {
                gb_message_ptr = std::make_shared<PTZPositionRequestMessage>(std::move(*gb_message_ptr));
                gb_message_ptr->load_from_xml();
            }
        }
    }
    auto ptr = std::make_shared<SubscribeRequestImpl>(platform_ptr, subscribe_ptr);
    if (0
        == toolkit::NoticeCenter::Instance().emitEvent(
            Broadcast::kEventOnSubscribeRequest, platform_ptr, ptr, gb_message_ptr)) {
        return sip_uas_reply(transaction.get(), 404, nullptr, 0, sip_session.get());
    }
    *sub = ptr.get();
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
        toolkit::NoticeCenter::Instance().emitEvent(
            Broadcast::kEventOnSubscribeNotify, shared_from_this(), xml_ptr, reason);
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
    return static_cast<int>((toolkit::getCurrentMicrosecond(true) - subscribe_time_) / 1000000L);
}

void SubscribeRequestImpl::send_notify(const std::shared_ptr<MessageBase> &message, SubscriberNotifyReplyCallback rcb) {
    // 如果订阅可用, 则在订阅会话中的 notify 消息中发送
    if (status_ == active) {
        subscribe_send_notify(message, std::forward<SubscriberNotifyReplyCallback>(rcb));
    }
    // 如果订阅不可用，则在message 消息中发送
    else {
        auto platform = std::get<std::shared_ptr<SuperPlatformImpl>>(platform_);
        auto request = std::make_shared<RequestProxyImpl>(platform, message, RequestProxy::RequestType::NoResponse);
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
    sip_agent_t *sip_agent = nullptr;
    std::visit(
        [&](auto &platform) {
            sip_agent = platform->get_sip_agent();
            if (message) {
                message->sn(platform->get_new_sn());
                if (platform->account().encoding != CharEncodingType::invalid) {
                    message->encoding(platform->account().encoding);
                }
            }
        },
        platform_);
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
    std::visit(
        [&](auto &platform) {
            platform->uac_send(transaction_ptr, std::move(payload), [callback_ptr](bool ret, std::string err) {
                if (!ret) {
                    std::lock_guard<decltype(send_notify_map_mutex_)> lock(send_notify_map_mutex_);
                    send_notify_map_.erase(callback_ptr.get());
                    return (*callback_ptr)(ret, std::move(err));
                }
            });
        },
        platform_);
}

void SubscribeRequestImpl::set_status(
    SUBSCRIBER_STATUS_TYPE_E status, TERMINATED_TYPE_E terminated_type, std::string reason) {
    status_ = status;
    terminated_type_ = terminated_type;
    error_ = std::move(reason);
    // 状态回调放入异步执行， 避免阻塞
    if (subscriber_callback_) {
        auto this_ptr = shared_from_this();
        toolkit::EventPollerPool::Instance().getPoller()->async(
            [this_ptr]() { this_ptr->subscriber_callback_(this_ptr, this_ptr->status_, this_ptr->error_); }, false);
    }
}

void SubscribeRequestImpl::to_subscribe(uint32_t expires) {
    sip_agent_t *sip_agent = nullptr;
    std::string from;
    std::string to;
    CharEncodingType encoding_type { CharEncodingType::invalid };
    std::visit(
        [&](auto &platform) {
            sip_agent = platform->get_sip_agent();
            from = platform->get_from_uri();
            to = platform->get_to_uri();
            encoding_type = platform->account().encoding;
            if (subscribe_message_) {
                if (subscribe_message_->sn() == 0) {
                    subscribe_message_->sn(platform->get_new_sn());
                }
                if (!subscribe_info_.subscribe_id) {
                    subscribe_info_.subscribe_id = subscribe_message_->sn();
                }
            } else if (subscribe_info_.subscribe_id == 0) {
                subscribe_info_.subscribe_id = platform->get_new_sn();
            }
        },
        platform_);
    std::string event = toolkit::str_format("%s;id=%d", subscribe_info_.event.c_str(), subscribe_info_.subscribe_id);

    std::shared_ptr<sip_uac_transaction_t> uac_subscribe_transaction;
    if (sip_subscribe_ptr_) {
        uac_subscribe_transaction.reset(
            sip_uac_resubscribe(
                sip_agent, sip_subscribe_ptr_.get(), static_cast<int>(expires),
                SubscribeRequestImpl::on_subscribe_reply, this),
            [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
    } else {
        uac_subscribe_transaction.reset(
            sip_uac_subscribe(
                sip_agent, from.c_str(), to.c_str(), event.c_str(), static_cast<int>(subscribe_info_.expires),
                SubscribeRequestImpl::on_subscribe_reply, this),
            [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
    }
    // 设置通用头信息
    set_message_header(uac_subscribe_transaction.get());

    if (subscribe_message_) {
        if (encoding_type != CharEncodingType::invalid) {
            subscribe_message_->encoding(encoding_type);
        }
        // 设置content_type 头
        set_message_content_type(uac_subscribe_transaction.get(), SipContentType_XML);
        subscribe_message_->parse_to_xml();
    }
    // 如果sip_subscribe_t 不存在, 则设置状态为pending
    if (!sip_subscribe_ptr_) {
        set_status(pending, terminated_type_, "");
    }
    std::visit(
        [&](auto &platform) {
            platform->uac_send(
                uac_subscribe_transaction, subscribe_message_ ? subscribe_message_->str() : std::string(),
                [this_ptr = shared_from_this()](bool ret, std::string err) {
                    if (!ret)
                        this_ptr->set_status(terminated, invalid, std::move(err));
                });
        },
        platform_);
}

int SubscribeRequestImpl::on_subscribe_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, struct sip_subscribe_t *subscribe,
    int code, void **session) {
    if (!param)
        return 0;
    // 得到智能指针，保持强类型引用，避免对象被 突然释放（虽然这个情况不太可能）
    auto this_ptr = static_cast<SubscribeRequestImpl *>(param)->shared_from_this();
    std::weak_ptr<SubscribeRequestImpl> weak_self = this_ptr;
    // 是否订阅成功
    if (SIP_IS_SIP_SUCCESS(code)) {
        // 取消订阅的操作
        if (subscribe->expires == 0) {
            this_ptr->terminated_time_ = toolkit::getCurrentMicrosecond(true);
            this_ptr->set_status(terminated, deactivated, "");
            this_ptr->delay_task_.reset();
            this_ptr->sip_subscribe_ptr_.reset();
        }
        // 订阅/刷新订阅的操作
        else {
            // 如果是刷新订阅 subscribe 应该是不变的, 这里不做判断，直接覆写
            sip_subscribe_addref(subscribe);
            this_ptr->sip_subscribe_ptr_.reset(subscribe, [](struct sip_subscribe_t *p) { sip_subscribe_release(p); });
            subscribe->evtsession = this_ptr.get();
            this_ptr->subscribe_time_ = toolkit::getCurrentMicrosecond(true); // 延迟 过期事件秒执行重订阅(刷新订阅)
            this_ptr->delay_task_ = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(
                (subscribe->expires - 30) * 1000, [weak_self]() {
                    if (auto this_ptr = weak_self.lock()) {
                        this_ptr->to_subscribe(this_ptr->subscribe_info_.expires);
                    }
                    return 0;
                });
            this_ptr->set_status(active, invalid, "");
        }
    } else {
        this_ptr->terminated_time_ = toolkit::getCurrentMicrosecond(true);
        this_ptr->sip_subscribe_ptr_.reset();
        this_ptr->delay_task_.reset();
        this_ptr->set_status(terminated, rejected, toolkit::str_format("peer return %d", code));
        // 如果是服务器错误那就多试几次
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