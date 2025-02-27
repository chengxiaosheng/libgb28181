#ifndef gb28181_src_request_REQUESTLISTIMPL_H
#define gb28181_src_request_REQUESTLISTIMPL_H
#include "RequestProxyImpl.h"

#include <Util/TimeTicker.h>
namespace toolkit {
class Ticker;
}
namespace toolkit {
class Timer;
}
namespace gb28181 {
class ListMessageBase;

class RequestListImpl final : public RequestProxyImpl {
public:
    ~RequestListImpl() override = default;
    RequestListImpl(const std::shared_ptr<PlatformHelper> &platform, const std::shared_ptr<MessageBase> &request, int sn = 0);

    std::shared_ptr<MessageBase> response() override;

protected:
    int on_response(const std::shared_ptr<MessageBase> &response) override;

    void on_completed_l() override;

    void on_reply_l() override;

private:
    int32_t sum_num_ { 0 };
    std::atomic_int32_t recv_num_ { 0 };
    toolkit::Ticker ticker_;
    std::shared_ptr<toolkit::Timer> timer_;
    std::shared_ptr<ListMessageBase> response_;
};
} // namespace gb28181

#endif // gb28181_src_request_REQUESTLISTIMPL_H

/**********************************************************************************************************
文件名称:   RequestListImpl.h
创建时间:   25-2-13 下午5:03
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-13 下午5:03

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-13 下午5:03       描述:   创建文件

**********************************************************************************************************/