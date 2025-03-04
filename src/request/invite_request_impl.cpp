#include "invite_request_impl.h"

#include "gb28181/sip_event.h"
#include "http-reason.h"
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
#include "tinyxml2.h"
#include <Util/logger.h>
#include <regex>
#include <utility>

using namespace gb28181;

// 针对invite 会话的管理, 得到 sip_dialog_t 后加入到集合中, bye 时从集合中移除， 应该不会有问题
static std::unordered_map<void *, std::shared_ptr<InviteRequestImpl>> invite_request_map_;
static std::shared_mutex invite_request_map_mutex_;

// 针对会话内uac请求的管理, 生命周期应该等同 sip_uac_transaction
static std::unordered_map<
    void *, std::shared_ptr<std::function<void(bool, std::string, std::shared_ptr<sip_message_t>)>>>
    info_request_map_;
static std::mutex info_request_map_mutex_;

std::shared_ptr<InviteRequest> InviteRequest::new_invite_request(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<SdpDescription> &sdp,
    const std::string &device_id) {
    return std::make_shared<InviteRequestImpl>(
        std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform), sdp, device_id);
}
std::shared_ptr<InviteRequest> InviteRequest::new_invite_request(
    const std::shared_ptr<SuperPlatform> &platform, const std::shared_ptr<SdpDescription> &sdp,
    const std::string &device_id) {
    return std::make_shared<InviteRequestImpl>(std::dynamic_pointer_cast<SuperPlatformImpl>(platform), sdp, device_id);
}

std::shared_ptr<InviteRequestImpl> InviteRequestImpl::get_invite(void *p) {
    std::shared_lock<std::shared_mutex> lock(invite_request_map_mutex_);
    if (auto it = invite_request_map_.find(p); it != invite_request_map_.end()) {
        return it->second;
    }
    return {};
}

void InviteRequestImpl::add_invite() {
    std::unique_lock<std::shared_mutex> lock(invite_request_map_mutex_);
    invite_request_map_[this] = shared_from_this();
}

void InviteRequestImpl::remove_invite() {
    std::unique_lock<std::shared_mutex> lock(invite_request_map_mutex_);
    invite_request_map_.erase(this);
}

// 不关注结果的请求使用
static int on_bye_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (reply) {
        sip_message_destroy(const_cast<sip_message_t *>(reply));
    }
    return 0;
}

void InviteRequestImpl::to_teardown(const std::string &reason) {
    auto platform = platform_helper_.lock();
    if (!platform) {
        return;
    }
    if (invite_dialog_) {
        return to_bye("teardown");
    }
    std::stringstream ss;
    ss << "TEARDOWN RTSP/1.0" << "\r\n";
    ss << "CSeq:" << platform->get_new_sn() << "\r\n";

    // teardown 消息应该不需要关注返回值
    std::shared_ptr<sip_uac_transaction_t> transaction(
        sip_uac_info(platform->get_sip_agent(), invite_dialog_.get(), nullptr, on_bye_reply, this),
        [](sip_uac_transaction_t *t) {
            if (t)
                sip_uac_transaction_release(t);
        });
    set_message_header(transaction.get());
    set_message_content_type(transaction.get(), SipContentType::SipContentType_MANSRTSP);
    platform->uac_send(transaction, ss.str(), [](bool, const std::string&) {});
    // 发送 teardown 后立即发送 bye 防止对方不支持teardown
    return to_bye(reason);
}

// 专门接收会话内请求消息回复
static int info_reply(void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, int code) {
    if (SIP_IS_SIP_INFO(code))
        return 0;

    std::shared_ptr<sip_message_t> reply_msg(const_cast<sip_message_t *>(reply), [](sip_message_t *p) {
        if (p)
            sip_message_destroy(p);
    });

    std::shared_ptr<std::function<void(bool, std::string, std::shared_ptr<sip_message_t>)>> rcb_ptr;
    {
        std::lock_guard<decltype(info_request_map_mutex_)> lock(info_request_map_mutex_);
        if (auto it = info_request_map_.find(param); it != info_request_map_.end()) {
            rcb_ptr = it->second;
            info_request_map_.erase(it);
        }
    }
    if (rcb_ptr) {
        if (SIP_IS_SIP_SUCCESS(code))
            (*rcb_ptr)(true, "", reply_msg);
        else
            (*rcb_ptr)(false, "reply " + std::to_string(code), reply_msg);
    }
    return 0;
}
void InviteRequestImpl::to_play(const BackPlayControlResponseCallback &rcb) {
    to_seek_scale({}, {}, std::forward<decltype(rcb)>(rcb));
}

void InviteRequestImpl::to_pause(const std::function<void(bool, std::string)> &rcb) {
    auto platform = platform_helper_.lock();
    if (!platform) {
        return rcb(false, "illegal operation! the session closed .");
    }
    if (!invite_dialog_ || !local_sdp_ || !remote_sdp_) {
        return rcb(false, "illegal operation! the session was closed or not established.");
    }
    if (local_sdp_->session().sessionType != SessionType::PLAYBACK
        && local_sdp_->session().sessionType != SessionType::DOWNLOAD) {
        return rcb(false, "illegal operation! this operation is not supported by the session.");
    }
    std::stringstream ss;
    ss << "PAUSE RTSP/1.0" << "\r\n";
    ss << "CSeq: " << platform->get_new_sn() << "\r\n";
    ss << "PauseTime: now" << "\r\n";

    auto rcb_ptr = std::make_shared<std::function<void(bool, std::string, std::shared_ptr<sip_message_t>)>>(
        [rcb, weak_this = weak_from_this()](bool ret, const std::string& err, const std::shared_ptr<sip_message_t>&) {
            if (!ret) {
                return rcb(false, err);
            }
            rcb(ret, err);
        });
    std::shared_ptr<sip_uac_transaction_t> transaction(
        sip_uac_info(platform->get_sip_agent(), invite_dialog_.get(), nullptr, info_reply, rcb_ptr.get()),
        [](sip_uac_transaction_t *t) {
            if (t)
                sip_uac_transaction_release(t);
        });
    {
        std::lock_guard<decltype(info_request_map_mutex_)> lock(info_request_map_mutex_);
        info_request_map_.emplace(rcb_ptr.get(), rcb_ptr);
    }
    set_message_header(transaction.get());
    set_message_content_type(transaction.get(), SipContentType::SipContentType_MANSRTSP);

    // 发送消息
    platform->uac_send(transaction, ss.str(), [rcb_ptr](bool ret, std::string err) {
        if (!ret) {
            std::lock_guard<decltype(info_request_map_mutex_)> lock(info_request_map_mutex_);
            info_request_map_.erase(rcb_ptr.get());
            (*rcb_ptr)(ret, std::move(err), {});
        }
    });
}

void InviteRequestImpl::to_seek_scale(
    std::optional<float> scale, std::optional<uint32_t> ntp, const BackPlayControlResponseCallback &rcb) {
    auto platform = platform_helper_.lock();
    if (!platform) {
        return rcb(false, "illegal operation! the session closed .", {});
    }
    if (!invite_dialog_ || !local_sdp_ || !remote_sdp_) {
        return rcb(false, "illegal operation! the session was closed or not established.", {});
    }
    if (local_sdp_->session().sessionType != SessionType::PLAYBACK
        && local_sdp_->session().sessionType != SessionType::DOWNLOAD) {
        return rcb(false, "illegal operation! this operation is not supported by the session.", {});
    }
    std::stringstream ss;
    ss << "PLAY RTSP/1.0" << "\r\n";
    ss << "CSeq: " << platform->get_new_sn() << "\r\n";
    if (ntp) {
        ss << "Range: ntp=" << ntp.value() << "-" << "\r\n";
    } else {
        ss << "Range: ntp=now-" << "\r\n";
    }
    if (scale) {
        ss << "Scale: " << scale.value() << "\r\n";
    }
    auto rcb_ptr = std::make_shared<std::function<void(bool, std::string, std::shared_ptr<sip_message_t>)>>(
        [rcb, weak_this = weak_from_this()](bool ret, const std::string& err, const std::shared_ptr<sip_message_t>& reply) {
            if (!ret) {
                return rcb(false, err, {});
            }
            if (!reply) {
                // 其实一定不会走到这里， reply = nullptr 时ret一定是false, 这里图个安心
                return rcb(false, "request timeout", {});
            }
            PlaybackControlResponse response;
            try {
                std::istringstream iss(std::string(static_cast<const char *>(reply->payload), reply->size));
                std::string line;
                if (!std::getline(iss, line)) {
                    return rcb(false, "bad response!", {});
                }
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                std::regex status_regex(R"(RTSP/(\d\.\d)\s(\d{3})\s(.+?)\r?)");
                std::smatch matches;

                if (std::regex_match(line, matches, status_regex)) {
                    tinyxml2::XMLUtil::ToInt(matches[2].str().c_str(), &response.rtsp_code);
                    response.rtsp_code = std::stoi(matches[2]);
                    response.reason = matches[3].str();
                } else {
                    return rcb(false, "Invalid status line format!", {});
                }
                while (std::getline(iss, line)) {
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    auto pos = line.find(':');
                    if (pos == std::string::npos) {
                        continue;
                    }
                    std::string_view key(line.data(), pos);
                    std::string_view value(line.data() + pos + 1, line.size() - pos - 1);
                    while (!value.empty() && std::isspace(value[0])) {
                        value = value.substr(1, value.size() - 1);
                    }
                    if (key == "Range") {
                        std::regex npt_regex(R"(npt=([\d\.]+|now)-?([\d\.]*)");
                        std::smatch match;
                        std::string value_str(value);
                        if (std::regex_search(value_str, match, npt_regex)) {
                            if (match[1] != "now") {
                                tinyxml2::XMLUtil::ToDouble(match[1].str().c_str(), &response.ntp);
                            }
                        }
                    } else if (key == "RTP-Info") {
                        std::string value_str(value);
                        std::istringstream stream1(value_str);
                        std::string pair;
                        std::unordered_map<std::string, std::string> params;
                        while (std::getline(stream1, pair, ';')) {
                            size_t eq_pos = pair.find('=');
                            if (eq_pos != std::string::npos) {
                                params[pair.substr(0, eq_pos)] = pair.substr(eq_pos + 1);
                            }
                        }
                        if (auto it = params.find("seq"); it != params.end()) {
                            tinyxml2::XMLUtil::ToUnsigned(it->second.c_str(), &response.seq);
                        }
                        if (auto it = params.find("rtptime"); it != params.end()) {
                            tinyxml2::XMLUtil::ToUnsigned(it->second.c_str(), &response.rtptime);
                        }
                    }
                }
            } catch (const std::exception &e) {
                ErrorL << "parse invite info response failed, " << e.what()
                       << ", content = " << std::string_view(static_cast<const char *>(reply->payload), reply->size);
            }
            rcb(ret, err, response);
        });
    std::shared_ptr<sip_uac_transaction_t> transaction(
        sip_uac_info(platform->get_sip_agent(), invite_dialog_.get(), nullptr, info_reply, rcb_ptr.get()),
        [](sip_uac_transaction_t *t) {
            if (t)
                sip_uac_transaction_release(t);
        });
    {
        std::lock_guard<decltype(info_request_map_mutex_)> lock(info_request_map_mutex_);
        info_request_map_.emplace(rcb_ptr.get(), rcb_ptr);
    }
    set_message_header(transaction.get());
    set_message_content_type(transaction.get(), SipContentType::SipContentType_MANSRTSP);

    // 发送消息
    platform->uac_send(transaction, ss.str(), [rcb_ptr](bool ret, std::string err) {
        if (!ret) {
            std::lock_guard<decltype(info_request_map_mutex_)> lock(info_request_map_mutex_);
            info_request_map_.erase(rcb_ptr.get());
            (*rcb_ptr)(ret, std::move(err), {});
        }
    });
}

std::shared_ptr<toolkit::Session> InviteRequestImpl::get_connection_session() {
    return invite_session_.lock();
}
void InviteRequestImpl::set_status(INVITE_STATUS_TYPE status, const std::string& error) {
    error_ = error;
    bool status_changed = status_ != status;
    status_ = status;
    if (status_changed && status_cb_) {
        status_cb_(status, error);
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
        if (reply->payload  == nullptr || reply->size == 0) {
            result = false;
            error = "no reply remote sdp";
            ErrorL << "no reply remote sdp";
        } else {
            // 1. 解析sdp
            SdpDescription sdp;
            if (sdp.parse(std::string(static_cast<const char *>(reply->payload), reply->size))) {
                this_ptr->remote_sdp_ = std::make_shared<SdpDescription>(std::move(sdp));
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
                WarnL << "parse sdp failed , sdp = " << std::string_view(static_cast<const char *>(reply->payload), reply->size);
            }
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

// todo: 此函数需要进一步完善
void InviteRequestImpl::to_bye(const std::string &reason) {
    if (status_ > INVITE_STATUS_TYPE::ack)
        return;
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
        platform->uac_send(transaction, "", [](bool, const std::string&) {});
        set_status(INVITE_STATUS_TYPE::cancel, reason);
    } else if (invite_dialog_) {
        std::shared_ptr<sip_uac_transaction_t> transaction(
            sip_uac_bye(sip_agent, invite_dialog_.get(), on_bye_reply, this), [](sip_uac_transaction_t *t) {
                if (t)
                    sip_uac_transaction_release(t);
            });
        set_message_header(transaction.get());
        platform->uac_send(transaction, "", [](bool, const std::string&) {});
        set_status(INVITE_STATUS_TYPE::bye, reason);
    } else {
        // 不太可能全部没有
        set_status(INVITE_STATUS_TYPE::cancel, reason);
    }
}

InviteRequestImpl::InviteRequestImpl(
    const std::shared_ptr<PlatformHelper> &platform, const std::shared_ptr<SdpDescription> &sdp,
    std::string device_id)
    : InviteRequest()
    , local_sdp_(sdp)
    , platform_helper_(platform)
    , device_id_(std::move(device_id)) {
    TraceL << "InviteRequestImpl::InviteRequestImpl()";
}
InviteRequestImpl::~InviteRequestImpl() {
    TraceL << "InviteRequestImpl::~InviteRequestImpl()";
    if (static_cast<int>(status_) < 3) {
        InviteRequestImpl::to_bye("destruction");
    }
}
void InviteRequestImpl::to_invite_request(
    std::function<void(bool, std::string, const std::shared_ptr<SdpDescription> &)> rcb) {
    auto platform = platform_helper_.lock();
    auto from = platform->get_from_uri();
    auto to = platform->get_to_uri();
    toolkit::replace(to, platform->sip_account().platform_id, device_id_);
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
    // 自动生成ssrc
    if (local_sdp_->media().front().ssrc == 0) {
        local_sdp_->media().front().ssrc = platform->get_sip_server()->make_ssrc(
            local_sdp_->session().sessionType == SessionType::PLAYBACK
            || local_sdp_->session().sessionType == SessionType::DOWNLOAD);
    }
    // 创建subject
    if (subject_.empty()) {
        subject_ = toolkit::str_format(
            "%s:%u,%s:%u", platform->sip_account().platform_id.c_str(), local_sdp_->media().front().ssrc,
            platform->get_sip_server()->get_account().platform_id.c_str(), local_sdp_->media().front().ssrc);
    }
    set_invite_subject(uac_invite_transaction_.get(), subject_.c_str());
    std::string sdp_str = local_sdp_->generate();
    if (sdp_str.empty()) {
        rcb(false, "No local sdp description", nullptr);
        return;
    }
    rcb_ = std::move(rcb);
    add_invite();
    platform->uas_send2(
        uac_invite_transaction_, sdp_str.c_str(),
        [this_ptr = shared_from_this()](bool ret, const std::string& err, const std::shared_ptr<SipSession> &session) {
            if (!ret && this_ptr->rcb_) {
                this_ptr->invite_session_ = session;
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

    SdpDescription sdp {};
    if (!sdp.parse(std::string(static_cast<const char *>(req->payload), req->size))) {
        WarnL << "parse sdp failed, " << std::string_view(static_cast<const char *>(req->payload), req->size);
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    auto sip_server = sip_session->getSipServer();
    auto platform_id = get_platform_id(req.get());
    if (sdp.session().sessionType == SessionType::UNKNOWN) {
        WarnL << "unknown invite sdk session , sdp = " << std::string_view(static_cast<const char *>(req->payload), req->size);
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    auto sdp_ptr = std::make_shared<SdpDescription>(std::move(sdp));
    std::shared_ptr<PlatformHelper> platform_ptr;
    if (sdp.session().sessionType == SessionType::TALK) {
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
    invite_ptr->device_id_ = get_invite_device_id(req.get());
    invite_ptr->subject_ = get_invite_subject(req.get());
    invite_ptr->invite_session_ = sip_session;
    // 发送 临时回复
    sip_uas_reply(transaction.get(), 100, nullptr, 0, sip_session.get());
    platform_ptr->on_invite(
        invite_ptr,
        [invite_ptr, sip_session, transaction, platform_ptr](int sip_code, std::shared_ptr<SdpDescription> sdp_ptr) {
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
                std::string payload = sdp_ptr->generate();
                invite_ptr->local_sdp_ = std::move(sdp_ptr);
                if (payload.empty()) {
                    WarnL << "sdp_ptr is empty";
                    sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
                    invite_ptr->set_status(INVITE_STATUS_TYPE::failed, "local sdp error");
                    return;
                }
                set_message_content_type(transaction.get(), SipContentType::SipContentType_SDP);
                invite_ptr->add_invite();

                sip_uas_reply(transaction.get(), sip_code, payload.c_str(), payload.size(), sip_session.get());

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
                sip_uas_reply(transaction.get(), sip_code, nullptr, 0, sip_session.get());
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

static const std::unordered_map<int, std::string> rtsp_reasons = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 408, "Request Timeout" },
    { 451, "Invalid Parameter" },
    { 452, "Conference Not Found" },
    { 453, "Not Enough Bandwidth" },
    { 454, "Session Not Found" },
    { 455, "Method Not Valid in This State" },
    { 456, "Header Field Not Valid for Resource" },
    { 457, "Invalid Range" },
    { 459, "Aggregate Operation Not Allowed" },
    { 460, "Only Aggregate Operation Allowed" },
    { 462, "Destination Unreachable" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 505, "RTSP Version Not Supported" },
    { 551, "Option not supported" } // GB28181扩展状态码
};

static const char *get_rtsp_reason(int code) {
    auto it = rtsp_reasons.find(code);
    if (it != rtsp_reasons.end()) {
        return it->second.c_str();
    }
    return http_reason_phrase(code);
}

int InviteRequestImpl::on_recv_info(
    const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_uas_transaction_t> &transaction,
    const std::shared_ptr<sip_message_t> &req, void *dialog_ptr) {
    auto this_ptr = InviteRequestImpl::get_invite(dialog_ptr);
    if (!this_ptr) {
        return sip_uas_reply(transaction.get(), 481, nullptr, 0, sip_session.get());
    }
    // 对消息进行解析
    if (!req->payload || req->size == 0) {
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }

    uint32_t cseq { 0 };
    auto reply_flag = std::make_shared<std::atomic_flag>();
    auto do_reply
        = [transaction, sip_session, reply_flag](uint32_t seq, int code, const std::optional<PlaybackControlResponse> &data) {
              // 确保回复仅仅执行一次
              if (reply_flag->test_and_set()) {
                  return;
              }
              std::stringstream ss;
              ss << "RTSP/1.0 " << code << get_rtsp_reason(code) << "\r\n";
              ss << "CSeq: " << seq << "\r\n";
              if (data) {
                  if (data->ntp > 0) {
                      ss << "Range: " << data->ntp << "-\r\n";
                  }
                  if (data->seq || data->rtptime) {
                      ss << "RTP-Info: ";
                      if (data->seq) {
                          ss << "seq=" << data->seq;
                      }
                      if (data->rtptime) {
                          if (data->seq)
                              ss << ";";
                          ss << "rtptime=" << data->rtptime;
                      }
                      ss << "\r\n";
                  }
              }
              std::string payload = ss.str();
              sip_uas_reply(transaction.get(), 200, payload.data(), payload.size(), sip_session.get());
          };

    std::istringstream stream(std::string(static_cast<const char *>(req->payload), req->size));
    std::string line;
    PlaybackControl control;
    if (std::getline(stream, line)) {
        std::regex method_regex("(PLAY|PAUSE|TEARDOWN) RTSP/1.0");
        std::smatch match;
        if (std::regex_search(line, match, method_regex)) {
            if (match[1] == "PLAY")
                control.action = PlaybackControl::Play;
            else if (match[1] == "PAUSE")
                control.action = PlaybackControl::Pause;
            else if (match[1] == "TEARDOWN")
                control.action = PlaybackControl::Teardown;
            else
                control.action = PlaybackControl::invalid;
        } else
            return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }

    try {
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            auto pos = line.find(':');
            if (pos == std::string::npos) {
                continue;
            }
            std::string_view key(line.data(), pos);
            std::string_view value(line.data() + pos + 1, line.size() - pos - 1);

            if (key == "CSeq") {
                tinyxml2::XMLUtil::ToUnsigned(value.data(), &cseq);
            } else if (key == "Scale") {
                tinyxml2::XMLUtil::ToFloat(value.data(), &control.scale);
            } else if (key == "Range") {
                std::regex npt_regex(R"(npt=([\d\.]+|now)-?([\d\.]*)");
                std::smatch match;
                std::string value_str(value);
                if (std::regex_search(value_str, match, npt_regex)) {
                    control.is_now = (match[1] == "now");
                    if (!control.is_now) {
                        tinyxml2::XMLUtil::ToDouble(match[1].str().c_str(), &control.ntp);
                        control.ntp = std::stod(match[1]);
                        if (!match[2].str().empty()) {
                            tinyxml2::XMLUtil::ToDouble(match[2].str().c_str(), &control.end_ntp);
                        }
                    }
                }
            } else if (key == "PauseTime" && control.action == PlaybackControl::Pause) {
                control.is_now = true;
            }
        }
    } catch (const std::exception &e) {
        ErrorL << "parse invite info request payload failed, error: " << e.what();
        if (cseq) {
            do_reply(cseq, 400, {});
            return 0;
        }
        return sip_uas_reply(transaction.get(), 400, nullptr, 0, sip_session.get());
    }
    if (control.action == PlaybackControl::Teardown) {
        do_reply(cseq, 200, {});
        this_ptr->to_bye("recv teardown");
        return 0;
    }
    if (!this_ptr->play_control_callback_) {
        do_reply(cseq, 406, {});
        return 0;
    }
    auto timeout = toolkit::EventPollerPool::Instance().getPoller()->doDelayTask(5 * 1000, [do_reply, cseq]() {
        do_reply(cseq, 408, {});
        return 0;
    });
    this_ptr->play_control_callback_(
        control, [do_reply, cseq, timeout](bool ret, const std::string& err, PlaybackControlResponse resp) {
            timeout->cancel();
            do_reply(cseq, resp.rtsp_code, std::forward<decltype(resp)>(resp));
        });
    return 0;
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