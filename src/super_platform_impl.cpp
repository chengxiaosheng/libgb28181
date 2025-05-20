#include "gb28181/message/broadcast_message.h"
#include "gb28181/message/catalog_message.h"
#include "gb28181/message/config_download_messsage.h"
#include "gb28181/message/cruise_track_message.h"
#include "gb28181/message/device_config_message.h"
#include "gb28181/message/device_control_message.h"
#include "gb28181/message/device_info_message.h"
#include "gb28181/message/device_status_message.h"
#include "gb28181/message/home_position_message.h"
#include "gb28181/message/keepalive_message.h"
#include "gb28181/message/message_base.h"
#include "gb28181/message/preset_message.h"
#include "gb28181/message/ptz_position_message.h"
#include "gb28181/message/record_info_message.h"
#include "gb28181/message/sd_card_status_message.h"
#include "gb28181/request/request_proxy.h"
#include "inner/sip_common.h"
#include "inner/sip_session.h"
#include "request/RequestProxyImpl.h"
#include "sip-header.h"
#include "sip-message.h"

#include "super_platform_impl.h"
#include <Poller/EventPoller.h>
#include <Util/NoticeCenter.h>
#include <algorithm>
#include <utility>
#include <gb28181/sip_event.h>
#include <inner/sip_server.h>
#include <sip-uac.h>

#include <Thread/WorkThreadPool.h>

using namespace gb28181;
using namespace toolkit;


SuperPlatformImpl::SuperPlatformImpl(super_account account, const std::shared_ptr<SipServer> &server)
    : SuperPlatform()
    , PlatformHelper()
    , account_(std::move(account))
    , keepalive_ticker_(std::make_shared<toolkit::Ticker>()) {
    local_server_weak_ = server;

    const auto &server_account = server->get_account();
    if (account_.local_host.empty()) {
        account_.local_host = server_account.local_host;
    }
    if (!account_.local_port) {
        account_.local_port = server_account.local_port;
    }
    from_uri_ = "sip:" + server_account.platform_id + "@" + server_account.local_host + ":"
        + std::to_string(server_account.local_port);
    contact_uri_
        = "sip:" + server_account.platform_id + "@" + account_.local_host + ":" + std::to_string(account_.local_port);
    to_uri_ = "sip:" + account_.platform_id + "@" + account_.host + ":" + std::to_string(account_.port);
}

SuperPlatformImpl::~SuperPlatformImpl() {
    DebugL << account_.platform_id;
}
void SuperPlatformImpl::shutdown() {
    if (!running_.exchange(false)) {
        return;
    }
    NOTICE_EMIT(
        kEventOnSuperPlatformShutdownArgs, Broadcast::kEventOnSuperPlatformShutdown,
        std::dynamic_pointer_cast<SuperPlatform>(shared_from_this()));
    if (account_.plat_status.status == PlatformStatusType::online) {
        to_register(0);
    }
}

void SuperPlatformImpl::start() {
    if (running_.exchange(true)) {
        return;
    }
    start_l();
}

void SuperPlatformImpl::start_l() {

    const char * host = temp_host_.empty() ? account_.host.c_str() : temp_host_.c_str();
    uint16_t port = temp_host_.empty() ? account_.port : temp_port_ == 0 ? account_.port : temp_port_;
    DebugL << "platform " << account_.platform_id << ", address = " << host << ":" << port;

    if (SockUtil::is_ipv4(host) || SockUtil::is_ipv6(host)) {
        struct sockaddr_storage addr = SockUtil::make_sockaddr(host, port);
        on_platform_addr_changed(addr);
        to_register(0);
    } else {
        auto poller = toolkit::EventPollerPool::Instance().getPoller();
        std::weak_ptr<SuperPlatformImpl> this_weak = shared_from_this();
        WorkThreadPool::Instance().getPoller()->async(
            [poller, host = host, port = port, this_weak]() {
                auto storage_self = this_weak.lock();
                if (!storage_self) {
                    return;
                }
                struct sockaddr_storage addr{};
                auto ret = SockUtil::getDomainIP(host, port, addr, AF_INET, SOCK_DGRAM, IPPROTO_TCP);
                if (ret) {
                    storage_self->on_platform_addr_changed(addr);
                    poller->async_first([storage_self]() { storage_self->to_register(0); });
                    return;
                }
                poller->doDelayTask(3 * 1000, [this_weak]() {
                    if (auto this_ptr = this_weak.lock()) {
                        this_ptr->start_l();
                    }
                    return 0;
                });
            });
    }
}

std::shared_ptr<LocalServer> SuperPlatformImpl::get_local_server() const {
    return local_server_weak_.lock();
}
std::string SuperPlatformImpl::get_to_uri() {
    if (moved_uri_.empty())
        return PlatformHelper::get_to_uri();
    return moved_uri_;
}

struct RegisterContext {
    std::shared_ptr<SuperPlatformImpl> platform;
    int expires; // 注册有效期
    std::string authorization; // 认证信息
};

// ReSharper disable once CppDFAConstantFunctionResult
int SuperPlatformImpl::on_register_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    auto *context = reinterpret_cast<RegisterContext *>(param);
    if (!context)
        return 0;
    // 如果是中间状态，直接返回
    if(SIP_IS_SIP_INFO(code)) {
        return 0;
    }
    // 取出 注册信息，并删除指针
    std::string authorization_str = std::move(context->authorization);
    int expires = context->expires;
    auto platform_ptr = context->platform;
    delete context;

    std::weak_ptr<SuperPlatformImpl> this_weak = platform_ptr;

    // 平台已经没人用了？ 毁灭吧
    if (platform_ptr.use_count() == 1) {
        return 0;
    }

    // 获取当前线程
    auto poller = toolkit::EventPollerPool::Instance().getPoller();

    // 延迟注册
    auto delay_register = [poller, this_weak](int expires, std::string&& authorization_str ,int delay = 3 * 1000) {
        // 3秒后执行注册
        return poller->doDelayTask(delay, [this_weak, expires, authorization_str = std::move(authorization_str)]() {
            if(auto storage_self = this_weak.lock()) {
                storage_self->to_register(expires, authorization_str);
            }
            return 0;
        });
    };

    // 请求超时
    if (reply == nullptr || code == 408) {
        platform_ptr->set_status(PlatformStatusType::network_error, "register timeout");
        delay_register(expires, std::move(authorization_str));
        return 0;
    }

    // 更新rport received
    if (reply) {
        platform_ptr->update_remote_via(get_via_rport(reply));
    }

    // 如果是 200 ok, 表示注册成功
    if (SIP_IS_SIP_SUCCESS(code)) {
        platform_ptr->auth_failed_count_ = 0;
        // 注销成功
        if (expires == 0) {
            platform_ptr->set_status(PlatformStatusType::offline, "active logout");
            if (platform_ptr->running_.load()) {
                poller->async_first([platform_ptr]() {
                    platform_ptr->to_register(platform_ptr->account_.register_expired);
                });
            }
            return 0;
        }

        // 获取返回的注册有效期时长
        if (int resp_expires = get_expires(reply); resp_expires > 0) {
            expires = resp_expires;
        }

        // 设置平台版本
        if (auto version = get_message_gbt_version(reply); version != PlatformVersionType::unknown) {
            platform_ptr->account_.version = version;
        }

        // 定义刷新注册任务
        platform_ptr->refresh_register_timer_ = delay_register(expires, std::move(authorization_str), (std::max)(expires - 5, 5) * 1000);
        platform_ptr->keepalive_ticker_ = std::make_shared<toolkit::Ticker>();
        platform_ptr->keepalive_timer_ = std::make_shared<Timer>(platform_ptr->account_.keepalive_interval, [this_weak]() {
            if (auto storage_self = this_weak.lock()) {
                storage_self->to_keepalive();
                return true;
            }
            return false;
        }, poller);
        // 设置平台状态
        platform_ptr->set_status(PlatformStatusType::online, "");
        // 发送心跳
        poller->async([platform_ptr]() {
            platform_ptr->to_keepalive();
        }, false);
        return 0;
    }

    // 注册重定向
    if (SIP_IS_SIP_REDIRECT(code)) {
        platform_ptr->moved_uri_ = get_message_contact(reply);
        if (!platform_ptr->moved_uri_.empty()) {
            sip_contact_t contact{};
            // 获取临时地址
            if (sip_contact_write(&contact, platform_ptr->moved_uri_.data(), platform_ptr->moved_uri_.data() + platform_ptr->moved_uri_.size()) >= 0 && cstrvalid(&contact.uri.host)) {
                std::string_view sip_host_str(contact.uri.host.p, contact.uri.host.n);
                auto pos =  sip_host_str.find('@');
                if (pos != std::string_view::npos) {
                    sip_host_str = sip_host_str.substr(pos + 1);
                }
                pos = sip_host_str.find(':');
                platform_ptr->temp_port_ = 5060;
                if (pos != std::string_view::npos) {
                    try {
                        platform_ptr->temp_port_ = std::strtol(sip_host_str.data() + pos + 1, nullptr, 10);
                    } catch (const std::exception &e) {
                        ErrorL << "Exception thrown: " << e.what();
                    }
                    sip_host_str = sip_host_str.substr(0, pos);
                }
                platform_ptr->temp_host_ = sip_host_str;
                //
                EventPollerPool::Instance().getExecutor()->async([this_weak]() {
                    if (auto storage_self = this_weak.lock()) {
                        storage_self->start_l();
                    }
                });
                return 0;
            }
            platform_ptr->temp_host_ = "";
            platform_ptr->moved_uri_ = ""; // 解析重定向失败，清理资源
        }
        delay_register(expires, std::move(authorization_str));
        return 0;
    }
    if (code == 401) {
        platform_ptr->auth_failed_count_++;
        if (platform_ptr->auth_failed_count_ > 5) {
            platform_ptr->auth_failed_count_ = 0;
            if (!platform_ptr->moved_uri_.empty()) {
                platform_ptr->moved_uri_ = "";
                platform_ptr->temp_host_ = "";
                platform_ptr->temp_port_ = 0;
            }
            platform_ptr->start_l();
            return 0;
        }
        std::string auth_str = generate_authorization(
            reply, platform_ptr->get_local_server()->get_account().platform_id, platform_ptr->account_.password,
            platform_ptr->get_to_uri(), platform_ptr->nc_pair_);
        if (auth_str.empty()) {
            delay_register(expires, "");
            platform_ptr->set_status(PlatformStatusType::auth_failure, "generate authorization failed");
        } else {
            delay_register(expires, std::move(auth_str), 1);
        }
    } else {
        platform_ptr->set_status(PlatformStatusType::auth_failure, "reply " + std::to_string(code));
        // 清理重定向
        if (!platform_ptr->moved_uri_.empty()) {
            platform_ptr->moved_uri_ = "";
            platform_ptr->temp_host_ = "";
            platform_ptr->temp_port_ = 0;
        }
        platform_ptr->start_l();
    }
    return 0;
}

void SuperPlatformImpl::to_register(int expires, const std::string &authorization) {
    if (refresh_register_timer_) {
        refresh_register_timer_->cancel();
        refresh_register_timer_.reset();
    }
    // 如果非注销，且平台已停止运行
    if (expires && !running_.load()) {
        return;
    }
    std::string from = get_from_uri();
    std::string to = get_to_uri();

    // 记录平台
    auto register_context = new RegisterContext { shared_from_this(), expires };
    // 构建注册事务
    std::shared_ptr<sip_uac_transaction_t> reg_trans(
        sip_uac_register(
            get_sip_agent(), from.c_str(), to.c_str(), expires, SuperPlatformImpl::on_register_reply, register_context),
        sip_uac_transaction_release);
    // 设置认证信息
    if (!authorization.empty()) {
        set_message_authorization(reg_trans.get(), authorization);
    }
    // 设置当前的状态为注册中
    if (account_.plat_status.status != PlatformStatusType::online) {
        set_status(PlatformStatusType::registering, "");
    }

    TraceL << "to register form " << from << " to " << to;
    // 发起注册请求
    uac_send(reg_trans, {}, [register_context](bool ret, const std::string &err) {
        // 发送失败 ?
        if (!ret) {
            if (register_context->platform.use_count() > 1) {
                register_context->platform->set_status(PlatformStatusType::network_error, err);
                std::weak_ptr<SuperPlatformImpl> weak_self = register_context->platform;
                EventPollerPool::Instance().getPoller()->doDelayTask(
                    3 * 1000,
                    [weak_self, authorization = std::move(register_context->authorization),
                     expires = register_context->expires]() {
                        if (auto strong_self = weak_self.lock()) {
                            strong_self->to_register(expires, authorization);
                        }
                        return 0;
                    });
            }
            delete register_context;
        }
    });
}

void SuperPlatformImpl::to_keepalive() {

    auto keepalive_ptr
        = std::make_shared<KeepaliveMessageRequest>(local_server_weak_.lock()->get_account().platform_id);
    keepalive_ptr->info() = fault_devices_;
    std::make_shared<RequestProxyImpl>(shared_from_this(), keepalive_ptr, RequestProxy::RequestType::NoResponse)
        ->send([weak_this = weak_from_this()](const std::shared_ptr<RequestProxy> &proxy) {
            if (auto this_ptr = weak_this.lock()) {
                if (proxy->status() == RequestProxy::Status::Succeeded) {
                    this_ptr->account_.plat_status.keepalive_time = getCurrentMicrosecond(true);
                    this_ptr->keepalive_ticker_->resetTime();
                    // 广播心跳事件
                    NOTICE_EMIT(
                        kEventSuperPlatformKeepaliveArgs, Broadcast::kEventSuperPlatformKeepalive,
                        std::dynamic_pointer_cast<SuperPlatform>(this_ptr), true, proxy->error());
                    return;
                }
                EventPollerPool::Instance().getPoller()->async([weak_this, proxy]() {
                    if (auto this_ptr = weak_this.lock()) {
                        NOTICE_EMIT(
                            kEventSuperPlatformKeepaliveArgs, Broadcast::kEventSuperPlatformKeepalive,
                            std::dynamic_pointer_cast<SuperPlatform>(this_ptr), false, proxy->error());
                    }
                });
                // 心跳超时 ? 应该重新注册
                if (this_ptr->keepalive_ticker_->elapsedTime()
                    >= this_ptr->account_.keepalive_interval * this_ptr->account_.keepalive_times * 1000) {
                    // 停止心跳检测
                    this_ptr->keepalive_timer_.reset();
                    this_ptr->set_status(
                        PlatformStatusType::offline,
                        "keepalive timeout, " + std::to_string(this_ptr->keepalive_ticker_->elapsedTime() / 1000));
                    // 发起注册请求
                    this_ptr->start_l();
                }
            }
        });
}

void SuperPlatformImpl::on_invite(
    const std::shared_ptr<InviteRequest> &invite_request,
    std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) {
    NOTICE_EMIT(
        kEventOnInviteRequestArgs, Broadcast::kEventOnInviteRequest,
        std::dynamic_pointer_cast<SuperPlatform>(shared_from_this()), invite_request, std::move(resp));
}

template <typename Response>
std::shared_ptr<Response> create_response(const std::string &device_id) {
    return nullptr;
}
template <>
std::shared_ptr<DeviceStatusMessageResponse> create_response(const std::string &device_id) {
    return std::make_shared<DeviceStatusMessageResponse>(device_id, ResultType::Error);
}
template <>
std::shared_ptr<CatalogResponseMessage> create_response(const std::string &device_id) {
    auto response = std::make_shared<CatalogResponseMessage>(
        device_id, 0, std::vector<ItemTypeInfo>(), std::vector<std::string>());
    response->reason() = "timeout";
    return response;
}
template <>
std::shared_ptr<DeviceInfoMessageResponse> create_response(const std::string &device_id) {
    return std::make_shared<DeviceInfoMessageResponse>(device_id, ResultType::Error);
}
template <>
std::shared_ptr<HomePositionResponseMessage> create_response(const std::string &device_id) {
    return std::make_shared<HomePositionResponseMessage>(device_id, std::nullopt);
}
template <>
std::shared_ptr<CruiseTrackListResponseMessage> create_response(const std::string &device_id) {
    return std::make_shared<CruiseTrackListResponseMessage>(
        device_id, 0, std::vector<CruiseTrackListItemType>(), ResultType::Error, "timeout");
}
template <>
std::shared_ptr<PTZPositionResponseMessage> create_response(const std::string &device_id) {
    return std::make_shared<PTZPositionResponseMessage>(device_id, ResultType::Error, "timeout");
}
template <>
std::shared_ptr<SdCardResponseMessage> create_response(const std::string &device_id) {
    return std::make_shared<SdCardResponseMessage>(
        device_id, 0, std::vector<SdCardInfoType>(), ResultType::Error, "timeout");
}

template <typename Request, typename Response, const char *EventType, typename T>
struct OneResponseQueryHandler;

template <typename Request, typename Response, const char *EventType, typename RET, typename... Args>
struct OneResponseQueryHandler<Request, Response, EventType, RET(Args...)> {
public:
    using RequestPtr = std::shared_ptr<Request>;
    using ResponsePtr = std::shared_ptr<Response>;
    using CallbackFunc = std::function<void(ResponsePtr)>;
    struct Context {
        std::atomic_flag flag { ATOMIC_FLAG_INIT };
        toolkit::EventPoller::DelayTask::Ptr timeout;
        RequestPtr request;
    };

    // 主处理模板方法
    int process(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> &&transaction,
        std::shared_ptr<SuperPlatformImpl> &&platform) {
        auto ctx = std::make_shared<Context>();
        ctx->request = std::make_shared<Request>(std::move(message));

        // XML解析校验
        if (!ctx->request->load_from_xml()) {
            set_message_reason(transaction.get(), ctx->request->get_error().c_str());
            return 400;
        }

        // 构建响应处理器
        CallbackFunc response = [ctx, weak_impl = platform->weak_from_this()](auto response_ptr) {
            if (ctx->flag.test_and_set())
                return;
            if (ctx->timeout)
                ctx->timeout->cancel();

            if (auto impl = weak_impl.lock()) {
                response_ptr->sn(ctx->request->sn());
                auto proxy = std::make_shared<RequestProxyImpl>(
                    impl, std::move(response_ptr), RequestProxy::RequestType::NoResponse);
                proxy->send([ctx](const auto &proxy) {
                    DebugL << "response " << *ctx->request << ", status:" << proxy->status()
                           << ", error:" << proxy->error();
                });
            }
        };
        // 设置超时任务
        ctx->timeout = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(
            GB28181_QUERY_TIMEOUT_MS, [device_id = ctx->request->device_id().value_or(""), response]() {
                auto resp = create_response<Response>(device_id);
                resp->reason() = "timeout";
                response(resp);
                return 0;
            });
        // 事件派发
        if (0
            == NOTICE_EMIT(
                Args..., EventType, std::dynamic_pointer_cast<SuperPlatform>(platform), ctx->request, response)) {
            ctx->timeout->cancel();
            ctx->timeout.reset();
            return 404;
        }

        return 200;
    }
};

template <typename Request, const char *EventType, typename T>
struct NoResponseQueryHandler;

template <typename Request, const char *EventType, typename RET, typename... Args>
struct NoResponseQueryHandler<Request, EventType, RET(Args...)> {
public:
    using RequestPtr = std::shared_ptr<Request>;
    int process(
        MessageBase &&message, const std::shared_ptr<sip_uas_transaction_t> &transaction,
        std::shared_ptr<SuperPlatformImpl> platform) {
        auto request = std::make_shared<Request>(std::move(message));
        // XML解析校验
        if (!request->load_from_xml()) {
            set_message_reason(transaction.get(), request->get_error().c_str());
            return 400;
        }
        // 事件派发
        if (0 == NOTICE_EMIT(Args..., EventType, std::dynamic_pointer_cast<SuperPlatform>(platform), request)) {
            return 404;
        }
        return 200;
    }
};

template <typename Request, typename Response, const char *EventType, typename T>
struct MultiResponseQueryHandler;

template <typename Request, typename Response, const char *EventType, typename RET, typename... Args>
struct MultiResponseQueryHandler<Request, Response, EventType, RET(Args...)> {
    using RequestPtr = std::shared_ptr<Request>;
    using ResponsePtr = std::shared_ptr<Response>;
    using CallbackFunc = std::function<void(std::deque<ResponsePtr> &&, int, bool)>;
    struct Context {
        std::atomic_flag flag { ATOMIC_FLAG_INIT };
        toolkit::EventPoller::DelayTask::Ptr timeout;
        std::mutex response_mutex {};
        std::deque<ResponsePtr> responses; // 回复数据据队列
        RequestPtr request;
    };
    // 主处理模板方法
    int process(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> &&transaction,
        std::shared_ptr<SuperPlatformImpl> &&platform) {
        auto ctx = std::make_shared<Context>();
        ctx->request = std::make_shared<Request>(std::move(message));

        // XML解析校验
        if (!ctx->request->load_from_xml()) {
            set_message_reason(transaction.get(), ctx->request->get_error().c_str());
            return 400;
        }

        // 构建响应处理器
        CallbackFunc response = [ctx, weak_impl = platform->weak_from_this()](
                                    std::deque<ResponsePtr> &&response, int concurrency, bool ignore_4xx) {
            if (ctx->flag.test_and_set())
                return;
            if (ctx->timeout) {
                ctx->timeout->cancel();
                ctx->timeout.reset();
            }

            ctx->responses = std::move(response);
            auto flag = std::make_shared<std::atomic_bool>();
            auto do_send_response = [weak_impl, ctx, flag, ignore_4xx](auto &&self) {
                if (flag->load())
                    return;

                std::shared_ptr<Response> ptr;
                {
                    std::lock_guard<std::mutex> lck(ctx->response_mutex);
                    if (!ctx->responses.empty()) {
                        ptr = std::move(ctx->responses.front());
                        ctx->responses.pop_front();
                    }
                }
                if (ptr) {
                    auto platform_ptr = weak_impl.lock();
                    if (!platform_ptr) {
                        WarnL << "Platform already destroyed";
                        flag->exchange(true);
                        return;
                    }
                    auto proxy = std::make_shared<RequestProxyImpl>(
                        weak_impl.lock(), std::move(ptr), RequestProxy::RequestType::NoResponse, ctx->request->sn());
                    proxy->send([self = std::forward<decltype(self)>(self), ignore_4xx,
                                 flag](const std::shared_ptr<RequestProxy> &proxy) {
                        // 发送失败? 是否应该结束整个流程
                        if (proxy->status() != RequestProxy::Succeeded) {
                            WarnL << "response " << proxy << ", status:" << proxy->status()
                                  << ",error:" << proxy->error();
                            if (!(ignore_4xx && SIP_IS_SIP_CLIENT_ERROR(proxy->reply_code())
                                  && proxy->reply_code() != 408)) {
                                flag->exchange(true);
                                return;
                            }
                        }
                        self(self);
                    });
                } else {
                    flag->exchange(true);
                }
            };
            if (concurrency == 0) {
                return do_send_response(do_send_response);
            }
            for (auto i = 0; i < concurrency; ++i) {
                do_send_response(do_send_response);
            }
        };
        // 设置超时任务
        ctx->timeout = EventPollerPool::Instance().getPoller()->doDelayTask(
            GB28181_QUERY_TIMEOUT_MS, [device_id = ctx->request->device_id().value_or(""), response]() {
                auto resp = create_response<Response>(device_id);
                resp->reason() = "timeout";
                std::deque<ResponsePtr> responses;
                responses.push_back(std::move(resp));
                response(std::move(responses), 1, true);
                return 0;
            });
        // 事件派发
        try {
            if (0
                == NOTICE_EMIT(
                    Args..., EventType, std::dynamic_pointer_cast<SuperPlatform>(platform), ctx->request, response)) {
                ctx->timeout->cancel();
                ctx->timeout.reset();
                return 404;
            }
        } catch (std::exception &e) {
            ErrorL << "notify message " << *ctx->request << " failed: " << e.what();
        }
        return 200;
    }
};

int SuperPlatformImpl::on_query(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    WarnL << "on query " << message;
    const MessageCmdType type = message.command();
    switch (type) {
        case MessageCmdType::DeviceStatus:
            return OneResponseQueryHandler<
                       DeviceStatusMessageRequest, DeviceStatusMessageResponse, Broadcast::kEventOnDeviceStatusRequest,
                       void(kEventOnDeviceStatusRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::Catalog:
            return MultiResponseQueryHandler<
                       CatalogRequestMessage, CatalogResponseMessage, Broadcast::kEventOnCatalogRequest,
                       void(kEventOnCatalogRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::DeviceInfo:
            return OneResponseQueryHandler<
                       DeviceInfoMessageRequest, DeviceInfoMessageResponse, Broadcast::kEventOnDeviceInfoRequest,
                       void(kEventOnDeviceInfoRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::RecordInfo:
            return MultiResponseQueryHandler<
                       RecordInfoRequestMessage, RecordInfoResponseMessage, Broadcast::kEventOnRecordInfoRequest,
                       void(kEventOnRecordInfoRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::ConfigDownload:
            return MultiResponseQueryHandler<
                       ConfigDownloadRequestMessage, ConfigDownloadResponseMessage,
                       Broadcast::kEventOnConfigDownloadRequest, void(kEventOnConfigDownloadRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::PresetQuery:
            return MultiResponseQueryHandler<
                       PresetRequestMessage, PresetResponseMessage, Broadcast::kEventOnPresetListRequest,
                       void(kEventOnPresetListRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::HomePositionQuery:
            return OneResponseQueryHandler<
                       HomePositionRequestMessage, HomePositionResponseMessage, Broadcast::kEventOnHomePositionRequest,
                       void(kEventOnHomePositionRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::CruiseTrackListQuery:
            return MultiResponseQueryHandler<
                       CruiseTrackListRequestMessage, CruiseTrackListResponseMessage,
                       Broadcast::kEventOnCruiseTrackListRequest, void(kEventOnCruiseTrackListRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::CruiseTrackQuery:
            return MultiResponseQueryHandler<
                       CruiseTrackRequestMessage, CruiseTrackResponseMessage, Broadcast::kEventOnCruiseTrackRequest,
                       void(kEventOnCruiseTrackRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::PTZPosition:
            return OneResponseQueryHandler<
                       PTZPositionRequestMessage, PTZPositionResponseMessage, Broadcast::kEventOnPTZPositionRequest,
                       void(kEventOnPTZPositionRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::SDCardStatus:
            return OneResponseQueryHandler<
                       SdCardRequestMessage, SdCardResponseMessage, Broadcast::kEventOnSdCardStatusRequest,
                       void(kEventOnSdCardStatusRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        default: return 404;
    }
}

int SuperPlatformImpl::on_control(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    const auto &xml_root = message.xml_ptr()->RootElement();
    if (message.command() == MessageCmdType::DeviceControl) {
        // 源设备向目标设备发送摄像机 云台控制、远程启动、强制关键帧、拉框放大、拉框缩小、PTZ精准控制、 存储卡格式化、
        // 目标跟踪命令后，目标设备不发送应答命令，命令流程见9.3.2.1;
        for (auto ele = xml_root->FirstChildElement(); ele != nullptr; ele = ele->NextSiblingElement()) {
            std::string name = ele->Name();
            if (name == "CmdType" || name == "SN" || name == "DeviceID")
                continue;
            if (name == "PTZCmd")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_PTZCmd, Broadcast::kEventOnDeviceControlPtzRequest,
                           void(kEventOnDeviceControlPtzRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "RecordCmd")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_RecordCmd, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlRecordCmdRequest,
                           void(kEventOnDeviceControlRecordCmdRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "IFrameCmd" || name == "IFameCmd")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_IFrameCmd, Broadcast::kEventOnDeviceControlIFrameCmdRequest,
                           void(kEventOnDeviceControlIFrameCmdRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "DragZoomIn")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_DragZoomIn, Broadcast::kEventOnDeviceControlDragZoomInRequest,
                           void(kEventOnDeviceControlDragZoomInRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "DragZoomOut")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_DragZoomOut, Broadcast::kEventOnDeviceControlDragZoomOutRequest,
                           void(kEventOnDeviceControlDragZoomOutRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "TargetTrack")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_TargetTrack, Broadcast::kEventOnDeviceControlTargetTrackRequest,
                           void(kEventOnDeviceControlTargetTrackRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "PTZPreciseCtrl")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_PtzPreciseCtrl,
                           Broadcast::kEventOnDeviceControlPTZPreciseCtrlRequest,
                           void(kEventOnDeviceControlPTZPreciseCtrlRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "HomePosition")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_HomePosition, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlHomePositionRequest,
                           void(kEventOnDeviceControlHomePositionRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "AlarmCmd")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_AlarmCmd, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlAlarmCmdRequest,
                           void(kEventOnDeviceControlAlarmCmdRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "GuardCmd")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_GuardCmd, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlGuardCmdRequest,
                           void(kEventOnDeviceControlGuardCmdRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "TeleBoot")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_TeleBoot, Broadcast::kEventOnDeviceControlTeleBootRequest,
                           void(kEventOnDeviceControlTeleBootRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "FormatSDCard")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_FormatSDCard,
                           Broadcast::kEventOnDeviceControlFormatSDCardRequest,
                           void(kEventOnDeviceControlFormatSDCardRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "DeviceUpgrade")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_DeviceUpgrade, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlDeviceUpgradeRequest,
                           void(kEventOnDeviceControlDeviceUpgradeRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
        }
    } else if (message.command() == MessageCmdType::DeviceConfig) {
        return OneResponseQueryHandler<
                   DeviceConfigRequestMessage, DeviceConfigResponseMessage, Broadcast::kEventOnDeviceConfigRequest,
                   void(kEventOnDeviceConfigRequestArgs)>()
            .process(std::move(message), std::move(transaction), shared_from_this());
    }
    return 404;
}
int SuperPlatformImpl::on_notify(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    // 从上级平台发来的notify ， 应该只有语音广播
    if (message.command() == MessageCmdType::Broadcast) {
        return OneResponseQueryHandler<
                   BroadcastNotifyRequest, BroadcastNotifyResponse, Broadcast::kEventOnBroadcastNotifyRequest,
                   void(kEventOnBroadcastNotifyRequestArgs)>()
            .process(std::move(message), std::move(transaction), shared_from_this());
    }
    return 404;
}
void SuperPlatformImpl::set_status(PlatformStatusType status, const std::string &error) {
    account_.plat_status.error = error;
    if (status == PlatformStatusType::online) {
        account_.plat_status.register_time = toolkit::getCurrentMicrosecond(true);
    }
    if (status == account_.plat_status.status)
        return;
    if (status == PlatformStatusType::offline) {
        account_.plat_status.offline_time = toolkit::getCurrentMicrosecond(true);
    }
    account_.plat_status.status = status;
    toolkit::EventPollerPool::Instance().getExecutor()->async(
        [this_ptr = shared_from_this(), status, error]() {
            {
                std::lock_guard<decltype(status_cbs_mtx_)> lck( this_ptr->status_cbs_mtx_);
                for(auto &it : this_ptr->status_cbs_) {
                    it.second(status);
                }
            }
            NOTICE_EMIT(
                kEventSuperPlatformStatusArgs, Broadcast::kEventSuperPlatformStatus,
                std::dynamic_pointer_cast<SuperPlatform>(this_ptr), status, error);
        },
        false);
}

/**********************************************************************************************************
文件名称:   super_platform_impl.cpp
创建时间:   25-2-11 下午5:00
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午5:00

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午5:00       描述:   创建文件

**********************************************************************************************************/