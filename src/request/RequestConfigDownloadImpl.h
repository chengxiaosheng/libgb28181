#ifndef gb28181_src_request_REQUESTCONFIGDOWNLOADIMPL_H
#define gb28181_src_request_REQUESTCONFIGDOWNLOADIMPL_H

#include "RequestProxyImpl.h"

#include <Network/Buffer.h>
#include <gb28181/type_define.h>
namespace gb28181 {
class ConfigDownloadResponseMessage;
}
namespace toolkit {
class Timer;
}
namespace gb28181 {
class RequestConfigDownloadImpl final : public RequestProxyImpl {
public:
    ~RequestConfigDownloadImpl() override = default;
    RequestConfigDownloadImpl(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request);

    std::shared_ptr<MessageBase> response() override;

protected:
    int on_response(const std::shared_ptr<MessageBase> &response) override;

    void on_completed_l() override;

    void on_reply_l() override;

private:
    std::shared_ptr<toolkit::Timer> timer_;
    DeviceConfigType recv_config_type_ { DeviceConfigType::invalid };
    DeviceConfigType request_config_type_ { DeviceConfigType::invalid };
    std::shared_ptr<ConfigDownloadResponseMessage> response_;
};
} // namespace gb28181

#endif // gb28181_src_request_REQUESTCONFIGDOWNLOADIMPL_H

/**********************************************************************************************************
文件名称:   RequestConfigDownloadImpl.h
创建时间:   25-2-11 下午1:58
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午1:58

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午1:58       描述:   创建文件

**********************************************************************************************************/