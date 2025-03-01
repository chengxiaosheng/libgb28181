#include "RequestCatalogImpl.h"

#include "subordinate_platform_impl.h"
using namespace gb28181;
RequestCatalogImpl::RequestCatalogImpl(
    const std::shared_ptr<SubordinatePlatform> &platform, const std::shared_ptr<MessageBase> &request)
    : RequestProxyImpl(
          std::dynamic_pointer_cast<SubordinatePlatformImpl>(platform), request, RequestType::MultipleResponses) {}
int RequestCatalogImpl::on_response(const std::shared_ptr<MessageBase> &response) {
    return 0;
}

/**********************************************************************************************************
文件名称:   RequestCatalogImpl.cpp
创建时间:   25-2-11 下午1:57
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午1:57

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午1:57       描述:   创建文件

**********************************************************************************************************/