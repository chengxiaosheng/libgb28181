#ifndef gb28181_include_gb28181_TYPE_DEFINE_H
#define gb28181_include_gb28181_TYPE_DEFINE_H
#include "gb28181/sip_common.h"
#include <string>

#ifndef SIP_AGENT_STR
#define SIP_AGENT_STR "GB/T 28181-2022"
#endif

namespace gb28181 {

enum class SipAuthType : uint8_t {
    none, // 不认证
    digest, // 摘要认证
    sm // 国密认证， 35114启用
};

/**
 * 字符集编码类型
 */
enum class CharEncodingType : uint8_t { utf8 = 0, gb2312, gbk };

enum class TransportType : uint8_t { none = 0, udp = 1, tcp = 2, both = 3 };

enum class PlatformManufacturer : uint8_t {
    unknown = 0, // 未知
    self, // 自身
    hikvision, // 海康
    dahua, // 大华
    uniview, // 宇视
    huawei, // 华为
    gongan // 公安
};

/**
 * GB/T 28181 标准标识
 */
enum class PlatformVersionType : uint8_t {
    unknown = 0, // 未知
    v10 = 10, // GB/T 28181-2011
    v11 = 11, // GB/T 28181-2011 修改补充文件
    v20 = 20, // GB/T 28181-2016
    v30 = 30 // GB/T 28181-2022
};


/**
 * 平台状态
 */
enum class PlatformStatusType : uint8_t {
    offline = 0, // 离线
    registering = 1, // 注册中
    online = 2, // 在线
    network_error = 3, // 网络错误
    auth_failure = 4, // 认证失败
};

struct sip_account_status {
    uint64_t register_time { 0 }; // 注册时间
    uint64_t keepalive_time { 0 }; // 心跳时间
    uint64_t offline_time { 0 }; // 离线时间
    PlatformStatusType status { PlatformStatusType::offline }; // 平台状态
    std::string error; // 错误信息
};

/**
 * 账户基本信息
 */
struct sip_account {
    std::string platform_id; // 平台编码
    std::string domain; // 平台域
    std::string name; // 平台名称
    std::string host; // 平台地址
    std::string password; // 密码
    uint16_t port { 5060 }; // 平台端口
    SipAuthType auth_type { SipAuthType::none }; // 认证方式
};

/**
 * 下级平台账户信息
 */
struct subordinate_account : public sip_account {
    CharEncodingType encoding { CharEncodingType::gb2312 }; // 字符集编码
    TransportType transport_type { TransportType::udp }; // 网络传输方式
    PlatformManufacturer manufacturer { PlatformManufacturer::unknown }; // 厂商类型
    sip_account_status plat_status;
};

/**
 * 上级平台账户信息
 */
struct super_account : public sip_account {
    CharEncodingType encoding { CharEncodingType::gb2312 }; // 字符集编码
    TransportType transport_type { TransportType::udp }; // 网络传输方式
    PlatformManufacturer manufacturer { PlatformManufacturer::unknown }; // 厂商类型
    int register_expired { 60 * 60 * 24 }; // 注册过期时间
    int keepalive_interval { 30 }; // 心跳间隔
    int keepalive_times { 3 }; // 心跳超时次数
    sip_account_status plat_status; // 平台状态
};

} // namespace gb28181

#endif // gb28181_include_gb28181_TYPE_DEFINE_H

/**********************************************************************************************************
文件名称:   type_define.h
创建时间:   25-2-6 上午10:24
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午10:24

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午10:24       描述:   创建文件

**********************************************************************************************************/