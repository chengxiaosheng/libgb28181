#ifndef gb28181_src_request_INVITE_REQUEST_IMPL_H
#define gb28181_src_request_INVITE_REQUEST_IMPL_H
#include "RequestConfigDownloadImpl.h"
#include "gb28181/request/invite_request.h"

#include <Poller/EventPoller.h>
#include <functional>
#include <memory>

#ifdef __cplusplus
namespace gb28181 {
class SipSession;
}
extern "C" {
struct sip_agent_t;
struct sip_uac_transaction_t;
struct sip_message_t;
struct sip_uas_transaction_t;
struct sip_dialog_t;
}
#endif

namespace gb28181 {



class PlatformHelper;
class InviteRequestImpl
    : public InviteRequest
    , public std::enable_shared_from_this<InviteRequestImpl> {
public:
    InviteRequestImpl() = default;
    InviteRequestImpl(const std::shared_ptr<PlatformHelper> &platform, const std::shared_ptr<SdpDescription> &sdp, std::string device_id);
    ~InviteRequestImpl() override;
    void
    to_invite_request(std::function<void(bool, std::string, const std::shared_ptr<SdpDescription> &)> rcb) override;

    void to_bye(const std::string &reason) override;

    static std::shared_ptr<InviteRequestImpl> get_invite(void *);

    std::shared_ptr<SdpDescription> local_sdp() const override { return local_sdp_; }
    std::shared_ptr<SdpDescription> remote_sdp() const override { return remote_sdp_; }
    uint64_t invite_time() const override { return invite_time_; }
    uint64_t ack_time() const override { return ack_time_; }
    uint64_t close_time() const override { return close_time_; }
    std::vector<std::string> &preferred_path() override { return preferred_path_; }
    std::vector<std::string> &route_path() override { return route_path_; }
    INVITE_STATUS_TYPE status() const override { return status_; }
    const std::string &error() const override { return error_; }
    void
    set_status_callback(std::function<void(INVITE_STATUS_TYPE status, const std::string &err)> status_cb) override {
        status_cb_ = std::move(status_cb);
    }
    void to_pause(const std::function<void(bool, std::string)> &rcb) override;
    void to_play(const BackPlayControlResponseCallback &rcb) override;
    void to_teardown(const std::string& reason) override;
    void to_seek_scale(std::optional<float> scale, std::optional<uint32_t> ntp, const BackPlayControlResponseCallback &rcb) override;

    const std::string &device_id() const override { return device_id_; }

    std::shared_ptr<toolkit::Session> get_connection_session() override;

private:
    void set_status(INVITE_STATUS_TYPE status, const std::string& error);

    void add_invite();
    void remove_invite();

    static int on_invite_reply(
        void *param, const struct sip_message_t *reply, struct sip_uac_transaction_t *t, struct sip_dialog_t *dialog,
        int code, void **session);
    static int on_recv_cancel(
        const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
        const std::shared_ptr<sip_uas_transaction_t> &transaction, void *session);
    static int on_recv_bye(
        const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
        const std::shared_ptr<sip_uas_transaction_t> &transaction, void *session);
    static int on_recv_invite(
        const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
        const std::shared_ptr<sip_uas_transaction_t> &transaction, const std::shared_ptr<sip_dialog_t> &dialog_ptr,
        void **session);
    static int on_recv_ack(
        const std::shared_ptr<SipSession> &sip_session, const std::shared_ptr<sip_message_t> &req,
        const std::shared_ptr<sip_uas_transaction_t> &transaction, const std::shared_ptr<sip_dialog_t> &dialog_ptr);
    static int on_recv_message(
        const std::shared_ptr<SipSession> &sip_session, const std ::shared_ptr<sip_uas_transaction_t> &transaction,
        const std ::shared_ptr<sip_message_t> &req, void *dialog_ptr);
    static int on_recv_info(
        const std::shared_ptr<SipSession> &sip_session, const std ::shared_ptr<sip_uas_transaction_t> &transaction,
        const std ::shared_ptr<sip_message_t> &req, void *dialog_ptr);

private:
    std::shared_ptr<SdpDescription> local_sdp_;
    std::shared_ptr<SdpDescription> remote_sdp_;
    std::weak_ptr<PlatformHelper> platform_helper_;
    std::shared_ptr<sip_dialog_t> invite_dialog_;
    std::shared_ptr<sip_uac_transaction_t> uac_invite_transaction_;
    std::weak_ptr<SipSession> invite_session_;
    std::string device_id_;
    std::string subject_;
    uint64_t invite_time_ { 0 };
    uint64_t ack_time_ { 0 };
    uint64_t close_time_ { 0 };
    std::vector<std::string> preferred_path_;
    std::vector<std::string> route_path_;
    std::string error_;
    INVITE_STATUS_TYPE status_ { INVITE_STATUS_TYPE::invite };
    std::function<void(bool, std::string, const std::shared_ptr<SdpDescription> &)> rcb_;
    std::function<void(INVITE_STATUS_TYPE status, const std::string &err)> status_cb_;
    BackPlayControlCallback play_control_callback_;
    friend class SipServer;
};
} // namespace gb28181

#endif // gb28181_src_request_INVITE_REQUEST_IMPL_H

/**********************************************************************************************************
文件名称:   invite_request_impl.h
创建时间:   25-2-17 上午11:27
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-17 上午11:27

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-17 上午11:27       描述:   创建文件

**********************************************************************************************************/