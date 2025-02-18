#include "invite_request_impl.h"

#include "gb28181/sip_common.h"
#include "inner/sip_common.h"
#include "inner/sip_server.h"
#include "inner/sip_session.h"
#include "platform_helper.h"
#include "sip-dialog.h"
#include "sip-message.h"
#include "sip-uac.h"
#include "sip-uas.h"
#include "subordinate_platform_impl.h"
#include "super_platform_impl.h"

#include <Util/logger.h>
using namespace gb28181;

static std::unordered_map<void *, std::shared_ptr<InviteRequestImpl>> invite_request_map_;
static std::shared_mutex invite_request_map_mutex_;

std::shared_ptr<InviteRequest> InviteRequest::new_invite_request(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<sdp_description> &sdp) {
    return std::make_shared<InviteRequestImpl>(std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform), sdp);
}

std::shared_ptr<InviteRequestImpl> InviteRequestImpl::get_invite(void *p) {
    std::shared_lock<std::shared_mutex> lock(invite_request_map_mutex_);
    if (auto it = invite_request_map_.find(p); it != invite_request_map_.end()) {
        return it->second;
    }
    return std::shared_ptr<InviteRequestImpl>();
}

void InviteRequestImpl::add_invite() {
    std::unique_lock<std::shared_mutex> lock(invite_request_map_mutex_);
    invite_request_map_[this] = shared_from_this();
}

void InviteRequestImpl::remove_invite() {
    std::unique_lock<std::shared_mutex> lock(invite_request_map_mutex_);
    invite_request_map_.erase(this);
}

void InviteRequestImpl::to_play(const std::function<void(bool, std::string)> &rcb) {}
void InviteRequestImpl::to_teardown(const std::function<void(bool, std::string)> &rcb) {}
void InviteRequestImpl::to_pause(const std::function<void(bool, std::string)> &rcb) {}
void InviteRequestImpl::to_scale(float scale, const std::function<void(bool, std::string)> &rcb) {}

void InviteRequestImpl::set_status(INVITE_STATUS_TYPE status, std::string error) {
    error_ = error;
    bool status_changed = status_ != status;
    status_ = status;
    if (status_changed && status_cb_) {
        status_cb_(status, error.c_str());
    }
    if (static_cast<int>(status_) > 2) {
        remove_invite();
    }
}

int InviteRequestImpl::on_invite_reply(
    void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, struct sip_dialog_t *dialog,
    int code, void **session) {
    if (param == nullptr)
        return 0;
    auto this_ptr = get_invite(param);
    if (this_ptr == nullptr) {
        // do cancel
        return 0;
    }
    // 临时回复
    if (SIP_IS_SIP_INFO(code)) {
        this_ptr->set_status(INVITE_STATUS_TYPE::trying, "");
        return 0;
    }

    // cancel 已不可用， 销毁uac invite 事务
    this_ptr->uac_invite_transaction_.reset();

    // 存储会话指针
    if (dialog) {
        // 将自身置入session中, 实现会话内消息的接收处理
        if (session) {
            *session = this_ptr.get();
        }
        // 添加dialog 的引用
        sip_dialog_addref(dialog);
        // 智能指针管理dialog
        this_ptr->invite_dialog_.reset(dialog, [](sip_dialog_t *dialog) {
            if (dialog)
                sip_dialog_release(dialog);
        });
    }

    // 回复ok
    bool result = true;
    std::string error;
    if (SIP_IS_SIP_SUCCESS(code)) {
        // 1. 解析sdp
        sdp_description sdp;
        if (parse(sdp, std::string((const char *)reply->payload, reply->size))) {
            this_ptr->remote_sdp_ = std::make_shared<sdp_description>(std::move(sdp));
            // 发送ack
            if (0 == sip_uac_ack(t, nullptr, 0, nullptr)) {
                this_ptr->set_status(INVITE_STATUS_TYPE::ack, "");
            } else {
                result = false;
                error = "send ack failed";
            }
        } else {
            result = false;
            error = "parse sdp failed";
            WarnL << "parse sdp failed , sdp = " << std::string_view((const char *)reply->payload, reply->size);
        }
        if (!result) {
            this_ptr->to_bye("");
        }
    } else {
        result = false;
        error = "remote reply code " + std::to_string(code);
    }
    if (!result) {
        this_ptr->set_status(INVITE_STATUS_TYPE::failed, error);
    }
    if (this_ptr->rcb_) {
        this_ptr->rcb_(result, this_ptr->error_, this_ptr->remote_sdp_);
        this_ptr->rcb_ = {};
    }
    return 0;
}

// 这个函数没啥用
static int on_bye_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    return 0;
}

// todo: 此函数需要进一步完善
void InviteRequestImpl::to_bye(const std::string &reason) {
    auto platform = platform_helper_.lock();
    if (!platform) {
        set_status(INVITE_STATUS_TYPE::bye, reason);
        return;
    }
    auto sip_agent = platform->get_sip_agent();
    // 当 uac_invite_transaction_ 事务不为空时, 说明事务应该还没有建立
    if (uac_invite_transaction_) {
        std::shared_ptr<sip_uac_transaction_t> transaction(
            sip_uac_cancel(sip_agent, uac_invite_transaction_.get(), on_bye_reply, this), [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
        set_message_header(transaction.get());
        platform->uac_send(transaction, "", [](bool, std::string) {});
        set_status(INVITE_STATUS_TYPE::cancel, reason);
    } else if (invite_dialog_) {
        std::shared_ptr<sip_uac_transaction_t> transaction(
            sip_uac_bye(sip_agent, invite_dialog_.get(), on_bye_reply, this), [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
        set_message_header(transaction.get());
        platform->uac_send(transaction, "", [](bool, std::string) {});
        set_status(INVITE_STATUS_TYPE::bye, reason);
    } else {
        // 不太可能全部没有
        set_status(INVITE_STATUS_TYPE::cancel, reason);
    }
}

InviteRequestImpl::InviteRequestImpl(
    const std::shared_ptr<PlatformHelper> &platform, const std::shared_ptr<sdp_description> &sdp)
    : InviteRequest()
    , platform_helper_(platform)
    , local_sdp_(sdp) {
    TraceL << "InviteRequestImpl::InviteRequestImpl()";
}
InviteRequestImpl::~InviteRequestImpl() {
    TraceL << "InviteRequestImpl::~InviteRequestImpl()";
}
void InviteRequestImpl::to_invite_request(
    std::function<void(bool, std::string, const std::shared_ptr<sdp_description> &)> rcb) {
    auto platform = platform_helper_.lock();
    auto from = platform->get_from_uri();
    auto to = platform->get_to_uri();
    uac_invite_transaction_.reset(
        sip_uac_invite(platform->get_sip_agent(), from.c_str(), to.c_str(), InviteRequestImpl::on_invite_reply, this),
        [](sip_uac_transaction_t *t) {
            if (t) {
                sip_uac_transaction_release(t);
            }
        });
    set_message_header(uac_invite_transaction_.get());
    set_x_preferred_path(uac_invite_transaction_.get(), preferred_path_);
    set_message_content_type(uac_invite_transaction_.get(), SipContentType::SipContentType_SDP);
    std::string sdp_str = get_sdp_description(*local_sdp_);
    if (sdp_str.empty()) {
        rcb(false, "No local sdp description", nullptr);
        return;
    }
    rcb_ = std::move(rcb);
    add_invite();
    platform->uac_send(
        uac_invite_transaction_, sdp_str.c_str(), [rcb, this_ptr = shared_from_this()](bool ret, std::string err) {
            if (!ret && this_ptr->rcb_) {
                this_ptr->set_status(INVITE_STATUS_TYPE::failed, err);
                this_ptr->rcb_(ret, err, nullptr);
                this_ptr->rcb_ = {};
            }
        });
}

int InviteRequestImpl::on_recv_bye(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
    const std::shared_ptr<sip_uas_transaction_t> &transaction, void *session) {
    auto this_ptr = InviteRequestImpl::get_invite(session);
    if (!this_ptr) {
        return sip_uas_reply(transaction.get(), 481, nullptr, 0, sip_session.get());
    }
    this_ptr->set_status(INVITE_STATUS_TYPE::bye, "they bye");
    return sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
}

int InviteRequestImpl::on_recv_cancel(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
    const std::shared_ptr<sip_uas_transaction_t> &transaction, void *session) {
    auto this_ptr = InviteRequestImpl::get_invite(session);
    if (!this_ptr) {
        return sip_uas_reply(transaction.get(), 481, nullptr, 0, sip_session.get());
    }
    this_ptr->set_status(INVITE_STATUS_TYPE::cancel, "they cancel");
    return sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
}

int InviteRequestImpl::on_recv_invite(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
    const std::shared_ptr<sip_uas_transaction_t> &transaction, const std::shared_ptr<sip_dialog_t> &dialog_ptr,
    void **session) {

    sdp_description sdp {};
    if (!parse(sdp, std::string((const char *)req->payload, req->size))) {
        WarnL << "parse sdp failed, " << std::string_view((const char *)req->payload, req->size);
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    auto sip_server = sip_session->getSipServer();
    auto platform_id = get_platform_id(req.get());
    if (sdp.s_name == sdp_session_type::invalid) {
        WarnL << "unknown invite sdk session , sdp = " << std::string_view((const char *)req->payload, req->size);
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    auto sdp_ptr = std::make_shared<sdp_description>(std::move(sdp));
    std::shared_ptr<PlatformHelper> platform_ptr;
    if (sdp.s_name == sdp_session_type::Talk) {
        platform_ptr
            = std::dynamic_pointer_cast<SubordinatePlatformImpl>(sip_server->get_subordinate_platform(platform_id));
    } else {
        platform_ptr = std::dynamic_pointer_cast<SuperPlatformImpl>(sip_server->get_super_platform(platform_id));
    }
    if (!platform_ptr) {
        return sip_uas_reply(transaction.get(), 480, nullptr, 0, sip_session.get());
    }
    auto invite_ptr = std::make_shared<InviteRequestImpl>();
    *session = invite_ptr.get();
    invite_ptr->remote_sdp_ = sdp_ptr;
    invite_ptr->platform_helper_ = platform_ptr;
    invite_ptr->invite_dialog_ = dialog_ptr;
    invite_ptr->invite_time_ = toolkit::getCurrentMicrosecond(true);
    invite_ptr->set_status(INVITE_STATUS_TYPE::invite, "");
    invite_ptr->preferred_path_ = get_x_preferred_path(req.get());
    // 发送 临时回复
    sip_uas_reply(transaction.get(), 100, nullptr, 0, sip_session.get());
    platform_ptr->on_invite(
        invite_ptr,
        [invite_ptr, sip_session, transaction, platform_ptr](int sip_code, std::shared_ptr<sdp_description> sdp_ptr) {
            // GB/T 28181-2022 x-route-path
            if (invite_ptr->route_path_.empty()) {
                invite_ptr->route_path_.emplace_back(platform_ptr->get_sip_server()->get_account().platform_id);
            }
            set_x_route_path(transaction.get(), invite_ptr->route_path_);
            if (SIP_IS_SIP_SUCCESS(sip_code)) {
                if (!sdp_ptr) {
                    WarnL << "sdp_ptr is null";
                    sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
                    invite_ptr->set_status(INVITE_STATUS_TYPE::failed, "local sdp error");
                    return;
                }
                std::string payload = get_sdp_description(*sdp_ptr);
                invite_ptr->local_sdp_ = sdp_ptr;
                if (payload.empty()) {
                    WarnL << "sdp_ptr is empty";
                    sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
                    invite_ptr->set_status(INVITE_STATUS_TYPE::failed, "local sdp error");
                    return;
                }
                set_message_content_type(transaction.get(), SipContentType::SipContentType_SDP);
                invite_ptr->add_invite();
                sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());

                // 添加一个超时定时器， 一段时间内没有收到ack 则关闭会话？
                std::weak_ptr<InviteRequestImpl> invite_weak = invite_ptr;
                toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(5 * 1000, [invite_weak]() {
                    if (auto this_ptr = invite_weak.lock()) {
                        if (this_ptr->status_ != INVITE_STATUS_TYPE::ack) {
                            this_ptr->to_bye("wait ack timeout");
                        }
                    }
                    return 0;
                });
            } else {
                sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
                invite_ptr->set_status(INVITE_STATUS_TYPE::failed, "rejected");
            }
        });
    return 0;
}

int InviteRequestImpl::on_recv_ack(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
    const std::shared_ptr<sip_uas_transaction_t> &transaction, const std::shared_ptr<sip_dialog_t> &dialog_ptr) {
    if (!dialog_ptr)
        return 0;
    auto this_ptr = InviteRequestImpl::get_invite(dialog_ptr->session);
    if (this_ptr == nullptr)
        return 0;
    this_ptr->set_status(INVITE_STATUS_TYPE::ack, "");
    return 0;
}

// GB/T 35114 通过会话内 Message消息传递 ClientVKEKNotify 消息
int InviteRequestImpl::on_recv_message(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, void *dialog_ptr) {
    auto this_ptr = InviteRequestImpl::get_invite(dialog_ptr);
    if (!this_ptr) {
        return sip_uas_reply(transaction.get(), 481, nullptr, 0, sip_session.get());
    }
    // 暂时不处理35114的消息 ，此处直接回复 404
    return sip_uas_reply(transaction.get(), 404, nullptr, 0, sip_session.get());
}
int InviteRequestImpl::on_recv_info(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, void *dialog_ptr) {
    auto this_ptr = InviteRequestImpl::get_invite(dialog_ptr);
    if (!this_ptr) {
        return sip_uas_reply(transaction.get(), 481, nullptr, 0, sip_session.get());
    }
    // todo： gb/t 28181 中通过 info 控制点播

    return sip_uas_reply(transaction.get(), 200, nullptr, 0, sip_session.get());
}

/**********************************************************************************************************
文件名称:   invite_request_impl.cpp
创建时间:   25-2-17 上午11:27
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-17 上午11:27

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-17 上午11:27       描述:   创建文件

**********************************************************************************************************/