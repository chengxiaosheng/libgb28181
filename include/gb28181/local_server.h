#ifndef gb28181_include_gb28181_PLATFORM_SERVER_H
#define gb28181_include_gb28181_PLATFORM_SERVER_H

#include "gb28181/type_define.h"

#include <atomic>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace gb28181 {
class SubordinatePlatform;
}
namespace gb28181 {
class SuperPlatform;
}
namespace gb28181 {
class SipServer;

class LocalServer {
public:
    using subordinate_account_callback = std::function<void(
        std::shared_ptr<LocalServer> server, std::shared_ptr<subordinate_account> account,
        std::function<void(bool)> allow_cb)>;
    virtual ~LocalServer() = default;

    /**
     * 创建一个本地监听服务
     * @param account
     * @return
     */
    static std::shared_ptr<LocalServer> new_local_server(local_account account);

    /**
     * 启动服务
     */
    virtual void run() = 0;
    /**
     * 关闭服务
     */
    virtual void shutdown() = 0;

    /**
     *
     * @return 获取账户信息
     */
    virtual const sip_account &get_account() const = 0;

    virtual void set_passwd(const std::string &passwd) = 0;
    virtual void set_name(const std::string &name) = 0;
    virtual void set_platform_id(const std::string &id) = 0;
    virtual void reload_account(sip_account account) = 0;

    /**
     * 获取指定下级平台
     * @param platform_id
     * @return
     */
    virtual std::shared_ptr<SubordinatePlatform> get_subordinate_platform(const std::string &platform_id) = 0;
    /**
     * 获取指定上级平台
     * @param platform_id
     * @return
     */
    virtual std::shared_ptr<SuperPlatform> get_super_platform(const std::string &platform_id) = 0;

    /**
     * 允许自动注册
     * @return
     */
    virtual bool allow_auto_register() const = 0;
    /**
     * 允许自动注册
     * @param allow_auto_register
     */
    virtual void set_allow_auto_register(bool allow_auto_register) = 0;
    /**
     * 添加下级平台
     * @param account
     */
    virtual void add_subordinate_platform(subordinate_account &&account) = 0;
    /**
     * 添加上级平台
     * @param account
     */
    virtual void add_super_platform(super_account &&account) = 0;

    /**
     * 设置平台自动注册回调
     * @param cb
     */
    virtual void set_new_subordinate_account_callback(subordinate_account_callback cb) = 0;

protected:
    LocalServer() = default;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_PLATFORM_SERVER_H

/**********************************************************************************************************
文件名称:   local_server.h
创建时间:   25-2-6 上午9:56
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午9:56

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午9:56       描述:   创建文件

**********************************************************************************************************/