#ifndef gb28181_include_gb28181_SUBORDINATE_PLATFORM_H
#define gb28181_include_gb28181_SUBORDINATE_PLATFORM_H
#include "gb28181/type_define.h"
namespace gb28181 {
class SubordinatePlatform {
public:
    virtual ~SubordinatePlatform() = default;
    virtual void shutdown() = 0;

    /**
     * @brief  获取账户信息
     * @return
     */
    virtual const subordinate_account & account() const = 0;

private:
};
} // namespace gb28181

#endif // gb28181_include_gb28181_SUBORDINATE_PLATFORM_H

/**********************************************************************************************************
文件名称:   subordinate_platform.h
创建时间:   25-2-6 上午9:57
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午9:57

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午9:57       描述:   创建文件

**********************************************************************************************************/