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

#include <Poller/EventPoller.h>
#include <Util/NoticeCenter.h>
#include <gb28181/sip_event.h>
#include <inner/sip_server.h>
#include <sip-uac.h>
#include "super_platform_impl.h"


using namespace gb28181;
using namespace toolkit;

static std::unordered_map<void *, std::shared_ptr<SuperPlatformImpl>> platform_registry_map_;
static std::shared_mutex platform_registry_mutex_;

std::shared_ptr<SuperPlatformImpl> get_platform(void *p) {
    std::shared_lock<decltype(platform_registry_mutex_)> lock(platform_registry_mutex_);
    if (auto it = platform_registry_map_.find(p); it != platform_registry_map_.end()) {
        return it->second;
    }
    return nullptr;
}
void add_platform_to_map(std::shared_ptr<SuperPlatformImpl> &&platform) {
    auto p = platform.get();
    std::unique_lock<std::shared_mutex> lock(platform_registry_mutex_);
    platform_registry_map_[p] = std::move(platform);
}
void remove_platform_from_map(void *p) {
    std::unique_lock<std::shared_mutex> lock(platform_registry_mutex_);
    platform_registry_map_.erase(p);
}

SuperPlatformImpl::SuperPlatformImpl(super_account account, const std::shared_ptr<SipServer> &server)
    : SuperPlatform()
    , PlatformHelper()
    , account_(std::move(account))
    , keepalive_ticker_(std::make_shared<toolkit::Ticker>()) {
    local_server_weak_ = server;

    if (account_.local_host.empty()) {
        account_.local_host = toolkit::SockUtil::get_local_ip();
    }
    if (account_.local_port == 0) {
        account_.local_port = server->get_account().port;
    }
    from_uri_ = "sip:" + get_sip_server()->get_account().platform_id + "@" + account_.local_host + ":"
        + std::to_string(account_.local_port);
}

SuperPlatformImpl::~SuperPlatformImpl() {
    DebugL << account_.platform_id;
}
void SuperPlatformImpl::shutdown() {
    if (!running_.load()) {
        return;
    }
    NOTICE_EMIT(kEventOnSuperPlatformShutdownArgs, Broadcast::kEventOnSuperPlatformShutdown, std::dynamic_pointer_cast<SuperPlatform>(shared_from_this()));
    running_.store(false);
    if (account_.plat_status.status == PlatformStatusType::online) {
        to_register(0);
    }
}
void SuperPlatformImpl::start() {
    if (running_.load()) {
        return;
    }
    running_.store(true);
    add_platform_to_map(shared_from_this());
    to_register(account_.register_expired);
}

std::shared_ptr<LocalServer> SuperPlatformImpl::get_local_server() const {
    return local_server_weak_.lock();
}
std::string SuperPlatformImpl::get_to_uri() {
    if (moved_uri_.empty())
        return PlatformHelper::get_to_uri();
    return moved_uri_;
}

// ReSharper disable once CppDFAConstantFunctionResult
int SuperPlatformImpl::on_register_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (!param)
        return 0;
    auto this_ptr = get_platform(param);
    if (!this_ptr)
        return 0; // 平台对象已经被析构 ?

    auto poller = toolkit::EventPollerPool::Instance().getPoller();

    auto do_register = [&]() {
        // 3秒后执行注册
        this_ptr->register_timer_ = poller->doDelayTask(3 * 1000, [weak_this = this_ptr->weak_from_this()]() {
            if (auto this_ptr = weak_this.lock()) {
                this_ptr->to_register(this_ptr->account_.register_expired);
            }
            return 0;
        });
    };

    // 请求超时
    if (reply == nullptr || code == 408) {
        this_ptr->set_status(PlatformStatusType::network_error, "register timeout");
        do_register();
        return 0;
    }

    // 更新rport received

    if (reply) {
        this_ptr->update_remote_via(get_via_rport(reply));
    }
    // if (auto via = sip_vias_get(&reply->vias, 0)) {
    //     std::string host = cstrvalid(&via->received) ? std::string(via->received.p, via->received.n) : "";
    //     this_ptr->update_local_via(host, static_cast<uint16_t>(via->rport));
    // }

    // 如果是 200 ok, 表示注册成功
    if (SIP_IS_SIP_SUCCESS(code)) {
        this_ptr->auth_failed_count_ = 0;
        // 获取 头部状态码判断是否为注册
        int expires = get_expires(reply);
        if (expires == 0) {
            expires = this_ptr->account_.register_expired;
        }
        if (expires) {
            if (auto version = get_message_gbt_version(reply); version != PlatformVersionType::unknown) {
                this_ptr->account_.version = version;
            }
            // 在有效期还剩余5秒的时候发起注册
            this_ptr->register_timer_
                = poller->doDelayTask((expires - 5) * 1000, [weak_this = this_ptr->weak_from_this()]() {
                      if (auto this_ptr = weak_this.lock()) {
                          this_ptr->to_register(this_ptr->account_.register_expired);
                      }
                      return 0;
                  });
            // 先发一次心跳意思一下
            this_ptr->keepalive_ticker_->resetTime();
            poller->async(
                [weak_this = this_ptr->weak_from_this()]() {
                    if (auto this_ptr = weak_this.lock()) {
                        this_ptr->to_keepalive();
                    }
                },
                false);
            // 设置心跳定时器
            this_ptr->keepalive_timer_ = std::make_shared<toolkit::Timer>(
                this_ptr->account_.keepalive_interval,
                [weak_this = this_ptr->weak_from_this()]() {
                    if (auto this_ptr = weak_this.lock()) {
                        this_ptr->to_keepalive();
                        return true;
                    }
                    return false;
                },
                poller);
            // 设置平台状态
            this_ptr->set_status(PlatformStatusType::online, "");
        } else {
            this_ptr->set_status(PlatformStatusType::offline, "unregistered");
        }
        return 0;
    }
    // 注册重定向
    if (SIP_IS_SIP_REDIRECT(code)) {
        this_ptr->moved_uri_ = get_message_contact(reply);
        if (!this_ptr->moved_uri_.empty()) {
            toolkit::EventPollerPool::Instance().getExecutor()->async(
                [this_ptr]() { this_ptr->to_register(this_ptr->account_.register_expired); }, false);
            return 0;
        }
        do_register();
    } else if (code == 401) {
        this_ptr->auth_failed_count_++;
        if (this_ptr->auth_failed_count_ > 5) {
            this_ptr->auth_failed_count_ = 0;
            do_register();
            return 0;
        }
        std::string auth_str = generate_authorization(
            reply, this_ptr->get_local_server()->get_account().platform_id, this_ptr->account_.password, this_ptr->get_to_uri(),
            this_ptr->nc_pair_);
        if (auth_str.empty()) {
            do_register();
            this_ptr->set_status(PlatformStatusType::auth_failure, "generate authorization failed");
        } else {
            toolkit::EventPollerPool::Instance().getExecutor()->async(
                [this_ptr, auth_str] { this_ptr->to_register(this_ptr->account_.register_expired, auth_str); }, false);
        }
    } else {
        this_ptr->set_status(PlatformStatusType::auth_failure, "reply " + std::to_string(code));
        // 清理重定向
        if (!this_ptr->moved_uri_.empty()) {
            this_ptr->moved_uri_ = "";
        }
        do_register();
    }
    return 0;
}

void SuperPlatformImpl::to_register(int expires, const std::string &authorization) {
    register_timer_.reset();
    std::string from = get_from_uri();
    std::string to = get_to_uri();

    // 构建注册事务
    std::shared_ptr<sip_uac_transaction_t> reg_trans(
        sip_uac_register(
            get_sip_agent(), from.c_str(), to.c_str(), expires, SuperPlatformImpl::on_register_reply, this),
        [](sip_uac_transaction_t *t) {
            if (t)
                sip_uac_transaction_release(t);
        });
    set_message_header(reg_trans.get());
    set_message_authorization(reg_trans.get(), authorization);
    auto weak_this = weak_from_this();

    // 设置当前的状态为注册中
    if (account_.plat_status.status != PlatformStatusType::online) {
        set_status(PlatformStatusType::registering, "");
    }
    TraceL << "to register form " << from << " to " << to;
    // 发起注册请求
    uac_send(reg_trans, {}, [weak_this](bool ret, std::string err) {
        if (auto this_ptr = weak_this.lock()) {
            if (!ret) {
                this_ptr->set_status(PlatformStatusType::network_error, std::move(err));
                this_ptr->register_timer_
                    = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(3 * 1000, [weak_this]() {
                          if (auto this_ptr = weak_this.lock()) {
                              this_ptr->to_register(this_ptr->account_.register_expired);
                          }
                          return 0;
                      });
            }
        }
    });
}
void SuperPlatformImpl::to_keepalive() {
    auto keepalive_ptr
        = std::make_shared<KeepaliveMessageRequest>(local_server_weak_.lock()->get_account().platform_id);
    keepalive_ptr->info() = fault_devices_;
    std::make_shared<RequestProxyImpl>(shared_from_this(), keepalive_ptr, RequestProxy::RequestType::NoResponse)
        ->send([weak_this = weak_from_this()](const std::shared_ptr<RequestProxy>& proxy) {
            if (auto this_ptr = weak_this.lock()) {
                if (proxy->status() == RequestProxy::Status::Succeeded) {
                    this_ptr->account_.plat_status.keepalive_time = getCurrentMicrosecond(true);
                    this_ptr->keepalive_ticker_->resetTime();
                    // 广播心跳事件
                    NOTICE_EMIT(kEventSuperPlatformKeepaliveArgs, Broadcast::kEventSuperPlatformKeepalive, std::dynamic_pointer_cast<SuperPlatform>(this_ptr), true, proxy->error());
                    return;
                }
                EventPollerPool::Instance().getPoller()->async([weak_this, proxy]() {
                    if (auto this_ptr = weak_this.lock()) {
                        NOTICE_EMIT(kEventSuperPlatformKeepaliveArgs, Broadcast::kEventSuperPlatformKeepalive, std::dynamic_pointer_cast<SuperPlatform>(this_ptr), false, proxy->error());
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
                    this_ptr->to_register(this_ptr->account_.register_expired);
                }
            }
        });
}

void SuperPlatformImpl::on_invite(
    const std::shared_ptr<InviteRequest> &invite_request,
    std::function<void(int, std::shared_ptr<SdpDescription>)> &&resp) {
    NOTICE_EMIT(kEventOnInviteRequestArgs, Broadcast::kEventOnInviteRequest, std::dynamic_pointer_cast<SuperPlatform>(shared_from_this()), invite_request, std::move(resp));
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

template <typename Request, typename Response, const char *EventType, typename RET, typename ...Args>
struct OneResponseQueryHandler<Request,Response,EventType, RET(Args...)> {
public:
    using RequestPtr = std::shared_ptr<Request>;
    using ResponsePtr = std::shared_ptr<Response>;
    using CallbackFunc = std::function<void(ResponsePtr)>;
    struct Context {
        std::atomic_flag flag{ATOMIC_FLAG_INIT};
        toolkit::EventPoller::DelayTask::Ptr timeout;
        RequestPtr request;
    };

    // 主处理模板方法
    int process(
        MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> && transaction,
        std::shared_ptr<SuperPlatformImpl>&& platform) {
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
        if (0 == NOTICE_EMIT(Args..., EventType, std::dynamic_pointer_cast<SuperPlatform>(platform), ctx->request, response)) {
            ctx->timeout->cancel();
            ctx->timeout.reset();
            return 404;
        }

        return 200;
    }
};

template <typename Request, const char *EventType, typename T>
struct NoResponseQueryHandler;

template <typename Request, const char *EventType, typename RET, typename ...Args>
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
        if (0 == NOTICE_EMIT(Args..., EventType, std::dynamic_pointer_cast<SuperPlatform>(platform), request) ) {
            return 404;
        }
        return 200;
    }
};

template <typename Request, typename Response, const char *EventType, typename T>
struct  MultiResponseQueryHandler;

template <typename Request, typename Response, const char *EventType, typename RET, typename ...Args>
struct  MultiResponseQueryHandler<Request,Response,EventType, RET(Args...)> {
    using RequestPtr = std::shared_ptr<Request>;
    using ResponsePtr = std::shared_ptr<Response>;
    using CallbackFunc = std::function<void(std::deque<ResponsePtr>&&, int,bool)>;
    struct Context {
        std::atomic_flag flag{ATOMIC_FLAG_INIT};
        toolkit::EventPoller::DelayTask::Ptr timeout;
        std::mutex response_mutex{};
        std::deque<ResponsePtr> responses; // 回复数据据队列
        RequestPtr request;
    };
    // 主处理模板方法
    int process(
        MessageBase &&message,  std::shared_ptr<sip_uas_transaction_t> &&transaction,
        std::shared_ptr<SuperPlatformImpl>&& platform) {
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
            if (0 == NOTICE_EMIT(Args..., EventType, std::dynamic_pointer_cast<SuperPlatform>(platform), ctx->request, response)) {
                ctx->timeout->cancel();
                ctx->timeout.reset();
                return 404;
            }
        } catch (std::exception &e) {
            ErrorL << "notify message " << *ctx->request <<  " failed: " << e.what();
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
                       DeviceStatusMessageRequest, DeviceStatusMessageResponse,
                       Broadcast::kEventOnDeviceStatusRequest,void(kEventOnDeviceStatusRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::Catalog:
            return MultiResponseQueryHandler<
                CatalogRequestMessage, CatalogResponseMessage, Broadcast::kEventOnCatalogRequest,void(kEventOnCatalogRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::DeviceInfo:
            return OneResponseQueryHandler<
                       DeviceInfoMessageRequest, DeviceInfoMessageResponse, Broadcast::kEventOnDeviceInfoRequest,void(kEventOnDeviceInfoRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::RecordInfo:
            return MultiResponseQueryHandler<
                       RecordInfoRequestMessage, RecordInfoResponseMessage, Broadcast::kEventOnRecordInfoRequest, void(kEventOnRecordInfoRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::ConfigDownload:
            return MultiResponseQueryHandler<
                       ConfigDownloadRequestMessage, ConfigDownloadResponseMessage,
                       Broadcast::kEventOnConfigDownloadRequest, void(kEventOnConfigDownloadRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::PresetQuery:
            return MultiResponseQueryHandler<
                       PresetRequestMessage, PresetResponseMessage, Broadcast::kEventOnPresetListRequest, void(kEventOnPresetListRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::HomePositionQuery:
            return OneResponseQueryHandler<
                       HomePositionRequestMessage, HomePositionResponseMessage,
                       Broadcast::kEventOnHomePositionRequest, void(kEventOnHomePositionRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::CruiseTrackListQuery:
            return MultiResponseQueryHandler<
                       CruiseTrackListRequestMessage, CruiseTrackListResponseMessage,
                       Broadcast::kEventOnCruiseTrackListRequest, void(kEventOnCruiseTrackListRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::CruiseTrackQuery:
            return MultiResponseQueryHandler<
                       CruiseTrackRequestMessage, CruiseTrackResponseMessage, Broadcast::kEventOnCruiseTrackRequest, void(kEventOnCruiseTrackRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::PTZPosition:
            return OneResponseQueryHandler<
                       PTZPositionRequestMessage, PTZPositionResponseMessage, Broadcast::kEventOnPTZPositionRequest, void(kEventOnPTZPositionRequestArgs)>()
                .process(std::move(message), std::move(transaction), shared_from_this());
        case MessageCmdType::SDCardStatus:
            return OneResponseQueryHandler<
                       SdCardRequestMessage, SdCardResponseMessage, Broadcast::kEventOnSdCardStatusRequest, void(kEventOnSdCardStatusRequestArgs)>()
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
                           DeviceControlRequestMessage_PTZCmd, Broadcast::kEventOnDeviceControlPtzRequest, void(kEventOnDeviceControlPtzRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "RecordCmd")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_RecordCmd, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlRecordCmdRequest, void(kEventOnDeviceControlRecordCmdRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "IFrameCmd" || name == "IFameCmd")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_IFrameCmd, Broadcast::kEventOnDeviceControlIFrameCmdRequest, void(kEventOnDeviceControlIFrameCmdRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "DragZoomIn")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_DragZoomIn, Broadcast::kEventOnDeviceControlDragZoomInRequest, void(kEventOnDeviceControlDragZoomInRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "DragZoomOut")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_DragZoomOut,
                           Broadcast::kEventOnDeviceControlDragZoomOutRequest, void(kEventOnDeviceControlDragZoomOutRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "TargetTrack")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_TargetTrack,
                           Broadcast::kEventOnDeviceControlTargetTrackRequest, void(kEventOnDeviceControlTargetTrackRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "PTZPreciseCtrl")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_PtzPreciseCtrl,
                           Broadcast::kEventOnDeviceControlPTZPreciseCtrlRequest, void(kEventOnDeviceControlPTZPreciseCtrlRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "HomePosition")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_HomePosition, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlHomePositionRequest, void(kEventOnDeviceControlHomePositionRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "AlarmCmd")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_AlarmCmd, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlAlarmCmdRequest, void(kEventOnDeviceControlAlarmCmdRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "GuardCmd")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_GuardCmd, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlGuardCmdRequest, void(kEventOnDeviceControlGuardCmdRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
            if (name == "TeleBoot")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_TeleBoot, Broadcast::kEventOnDeviceControlTeleBootRequest, void(kEventOnDeviceControlTeleBootRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "FormatSDCard")
                return NoResponseQueryHandler<
                           DeviceControlRequestMessage_FormatSDCard,
                           Broadcast::kEventOnDeviceControlFormatSDCardRequest, void(kEventOnDeviceControlFormatSDCardRequestArgs)>()
                    .process(std::move(message), transaction, shared_from_this());
            if (name == "DeviceUpgrade")
                return OneResponseQueryHandler<
                           DeviceControlRequestMessage_DeviceUpgrade, DeviceControlResponseMessage,
                           Broadcast::kEventOnDeviceControlDeviceUpgradeRequest, void(kEventOnDeviceControlDeviceUpgradeRequestArgs)>()
                    .process(std::move(message), std::move(transaction), shared_from_this());
        }
    } else if (message.command() == MessageCmdType::DeviceConfig) {
        return OneResponseQueryHandler<
                   DeviceConfigRequestMessage, DeviceConfigResponseMessage, Broadcast::kEventOnDeviceConfigRequest, void(kEventOnDeviceConfigRequestArgs)>()
            .process(std::move(message), std::move(transaction), shared_from_this());
    }
    return 404;
}
int SuperPlatformImpl::on_notify(
    MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request) {
    // 从上级平台发来的notify ， 应该只有语音广播
    if (message.command() == MessageCmdType::Broadcast) {
        return OneResponseQueryHandler<
                   BroadcastNotifyRequest, BroadcastNotifyResponse, Broadcast::kEventOnBroadcastNotifyRequest, void(kEventOnBroadcastNotifyRequestArgs)>()
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
            NOTICE_EMIT(kEventSuperPlatformStatusArgs, Broadcast::kEventSuperPlatformStatus, std::dynamic_pointer_cast<SuperPlatform>(this_ptr), status, error);
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