#include "request/RequestConfigDownloadImpl.h"

#include <Poller/Timer.h>
#include <gb28181/message/config_download_messsage.h>
#include <gb28181/type_define_ext.h>
using namespace gb28181;

RequestConfigDownloadImpl::RequestConfigDownloadImpl(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request)
    : RequestProxyImpl(platform, request, RequestType::MultipleResponses) {
    request_config_type_ = std::dynamic_pointer_cast<ConfigDownloadRequestMessage>(request)->config_type();
}

int RequestConfigDownloadImpl::on_response(const std::shared_ptr<MessageBase> &response) {
    int code = 200;

    auto resp = std::dynamic_pointer_cast<ConfigDownloadResponseMessage>(response);
    recv_config_type_ = recv_config_type_ | resp->get_config_type();
    bool is_end = recv_config_type_ == request_config_type_;
    if (is_end) {
        timer_.reset();
        response_end_time_ = toolkit::getCurrentMicrosecond(true);
        status_ = Succeeded;
    }
    if (resp->get_config_type() == DeviceConfigType::invalid && resp->result() != ResultType::OK) {
        timer_.reset();
        response_end_time_ = toolkit::getCurrentMicrosecond(true);
        status_ = Failed;
        error_ = "response failed, " + resp->reason();
        on_completed();
        return 200;
    }
    if (response_callback_) {
        code = response_callback_(shared_from_this(), response, is_end);
    }
    if (is_end) {
        on_completed();
    }
    return code;
}

void RequestConfigDownloadImpl::on_reply_l() {
    if (!timer_) {
        timer_ = std::make_shared<toolkit::Timer>(
            5,
            [weak_this = weak_from_this()]() {
                if (auto this_ptr = std::dynamic_pointer_cast<RequestConfigDownloadImpl>(weak_this.lock())) {
                    if (this_ptr->status_ == Replied) {
                        this_ptr->status_ = Timeout;
                        this_ptr->error_ = "the wait for a response has timed out";
                    }
                    this_ptr->on_completed();
                }
                return false;
            },
            nullptr);
    }
}

std::shared_ptr<MessageBase> RequestConfigDownloadImpl::response()  {
    if (response_) return response_;
    if (responses_.empty()) return nullptr;
    response_ = std::dynamic_pointer_cast<ConfigDownloadResponseMessage>(responses_.front());
    for (auto &it : responses_) {
        if (it == response_) continue;
        auto resp = std::dynamic_pointer_cast<ConfigDownloadResponseMessage>(it);
        for (auto &[fst, snd] : resp->configs_) {
            resp->configs_[fst] = snd;
        }
    }
    return response_;
}


void RequestConfigDownloadImpl::on_completed_l() {
    timer_.reset(); // 目前timer 定义在 on_reply_l, 此处其实没有意义
}

/**********************************************************************************************************
文件名称:   RequestConfigDownloadImpl.cpp
创建时间:   25-2-11 下午1:58
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午1:58

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午1:58       描述:   创建文件

**********************************************************************************************************/