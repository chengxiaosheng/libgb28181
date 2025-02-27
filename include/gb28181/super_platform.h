#ifndef gb28181_include_gb28181_SUPER_PLATFORM_H
#define gb28181_include_gb28181_SUPER_PLATFORM_H
#include "gb28181/type_define.h"
#include <memory>
#include <functional>
namespace gb28181 {
class LocalServer;
class DeviceInfoMessageResponse;
class DeviceInfoMessageRequest;
class DeviceStatusMessageResponse;
class DeviceStatusMessageRequest;
class SuperPlatform;



class SuperPlatform {
public:
    using DeviceStatusQueryCallback = std::function<void(std::shared_ptr<SuperPlatform>, std::shared_ptr<DeviceStatusMessageRequest>, std::function<void(std::shared_ptr<DeviceStatusMessageResponse>)>)>;
    using DeviceInfoQueryCallback = std::function<void(std::shared_ptr<SuperPlatform>, std::shared_ptr<DeviceInfoMessageRequest>, std::function<void(std::shared_ptr<DeviceInfoMessageResponse>)>)>;

    struct event_defined {
        DeviceStatusQueryCallback on_device_status_query;
        DeviceInfoQueryCallback on_device_info_query;
    };
    virtual ~SuperPlatform() = default;
    virtual void shutdown() = 0;
    virtual void start() = 0;
    virtual const super_account &account() const = 0;
    virtual void set_encoding(CharEncodingType encoding) = 0;

    virtual std::shared_ptr<LocalServer> get_local_server() const = 0;

};
} // namespace gb28181

#endif // gb28181_include_gb28181_SUPER_PLATFORM_H

/**********************************************************************************************************
文件名称:   super_platform.h
创建时间:   25-2-6 上午9:57
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午9:57

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午9:57       描述:   创建文件

**********************************************************************************************************/