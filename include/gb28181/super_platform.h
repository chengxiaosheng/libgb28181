#ifndef gb28181_include_gb28181_SUPER_PLATFORM_H
#define gb28181_include_gb28181_SUPER_PLATFORM_H

namespace gb28181 {
class SuperPlatform {
public:
    virtual ~SuperPlatform() = default;
    virtual void shutdown() = 0;
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