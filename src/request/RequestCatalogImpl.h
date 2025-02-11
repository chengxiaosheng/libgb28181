#ifndef gb28181_src_request_REQUESTCATALOGIMPL_H
#define gb28181_src_request_REQUESTCATALOGIMPL_H
#include "request/RequestProxyImpl.h"
namespace gb28181 {
class RequestCatalogImpl final : public RequestProxyImpl {
public:
    RequestCatalogImpl(
        const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request)
        : RequestProxyImpl(platform, request, RequestType::MultipleResponses) {}
  protected:
  int on_response(const std::shared_ptr<MessageBase> &response) override;
};

} // namespace gb28181

#endif // gb28181_src_request_REQUESTCATALOGIMPL_H

/**********************************************************************************************************
文件名称:   RequestCatalogImpl.h
创建时间:   25-2-11 下午1:57
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午1:57

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午1:57       描述:   创建文件

**********************************************************************************************************/