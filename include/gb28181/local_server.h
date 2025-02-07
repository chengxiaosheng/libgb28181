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

class LocalServer : public std::enable_shared_from_this<LocalServer> {
public:
    explicit LocalServer(sip_account account);
    ~LocalServer();

    void run();
    void shutdown();

    /**
     *
     * @return 获取账户信息
     */
    inline const sip_account &get_account() const { return account_; }

    inline void set_passwd(const std::string &passwd) { account_.password = passwd; }
    inline void set_name(const std::string &name) { account_.name = name; }
    inline void set_platform_id(const std::string &id) { account_.platform_id = id; }
    void reload_account(sip_account account);

    void get_subordinate_platform(const std::string &platform_id, const std::function<void(std::shared_ptr<SubordinatePlatform>)> &cb);

    inline bool allow_auto_register() const {
        return allow_auto_register_;
    }
    inline void set_allow_auto_register(bool allow_auto_register) { allow_auto_register_ = allow_auto_register; }



private:
    sip_account account_;
    std::atomic_bool running_; // 是否正在运行
    bool allow_auto_register_{true};
    TransportType transport_type_{TransportType::both};
    std::shared_ptr<SipServer> server_; // sip服务
    std::unordered_map<std::string, std::shared_ptr<SuperPlatform>> super_platforms_; // 上级平台
    std::unordered_map<std::string, std::shared_ptr<SubordinatePlatform>> sub_platforms_; // 下级平台
    std::shared_mutex platform_mutex_;

    std::function<void(const std::string &platform_id, std::function<void(std::shared_ptr<SubordinatePlatform>)> cb)> find_sub_platform_callback_;
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