#include "RequestListImpl.h"

#include <Poller/Timer.h>
#include <Util/util.h>
#include <gb28181/message/message_base.h>

using namespace gb28181;

RequestListImpl::RequestListImpl(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request)
    : RequestProxyImpl(platform, request, MultipleResponses) {}

int RequestListImpl::on_response(const std::shared_ptr<MessageBase> &response) {
    int code = 200;
    ticker_.resetTime(); // 重置计时器
    auto resp = std::dynamic_pointer_cast<ListMessageBase>(response);
    // 之前收到的总数
    auto recv_num = recv_num_.fetch_add(resp->num());
    sum_num_ = resp->sum_num(); // 已知海康部分设备 在PresetQuery 、 CruiseTrackListQuery 中无SumNum 字段。
                                // 对于不合格的结构，我们直接忽略后续数据
    // 已完成所有数据接收
    bool completed = sum_num_ <= recv_num + resp->num();
    if (completed) {
        timer_.reset(); // 取消定时器
        response_end_time_ = toolkit::getCurrentMicrosecond(true);
        status_ = Succeeded;
    }
    // 存在实时接收回调， 立即返回数据
    if (response_callback_) {
        code = response_callback_(shared_from_this(), response, completed);
    }
    if (completed) {
        on_completed();
    }
    return code;
}

void RequestListImpl::on_completed_l() {
    timer_.reset(); // 目前timer 定义在 on_reply_l, 此处其实没有意义
}

void RequestListImpl::on_reply_l() {
    ticker_.resetTime(); // 收到确认后重置计时器
    if (status_ == Replied) {
        std::weak_ptr<RequestListImpl> weak_this = std::dynamic_pointer_cast<RequestListImpl>(shared_from_this());
        timer_ = std::make_shared<toolkit::Timer>(
            10,
            [weak_this]() {
                if (auto this_ptr = weak_this.lock()) {
                    if (this_ptr->ticker_.elapsedTime() > 1000) {
                        this_ptr->status_ = Timeout;
                        this_ptr->error_ = "the wait for a response has timed out";
                        this_ptr->on_completed();
                        return false;
                    }
                    return true;
                }
                return false;
            },
            nullptr);
    }
}

std::shared_ptr<MessageBase> RequestListImpl::response() {
    if (response_)
        return response_;
    // 合并所有的返回到一起？
    if (responses_.empty())
        return nullptr;
    if (responses_.size() == 1)
        return responses_[0];

    return nullptr;
}

/**********************************************************************************************************
文件名称:   RequestListImpl.cpp
创建时间:   25-2-13 下午5:03
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-13 下午5:03

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-13 下午5:03       描述:   创建文件

**********************************************************************************************************/