#ifndef gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#define gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#include "gb28181/subordinate_platform.h"
#include "gb28181/type_define.h"
#include <memory>

#ifdef __cplusplus
extern "C" {
  struct timeval;
}
#endif


namespace toolkit {
class Timer;
}

namespace gb28181 {
class SipSession;
class LocalServer;


class SubordinatePlatformImpl final: public SubordinatePlatform, public std::enable_shared_from_this<SubordinatePlatformImpl> {
public:
  explicit SubordinatePlatformImpl(subordinate_account account);
  ~SubordinatePlatformImpl() override;

  void shutdown() override;

  inline const subordinate_account &account() const override {
    return account_;
  }

  void add_session(const std::shared_ptr<SipSession> &session);

  void set_status(PlatformStatusType status, std::string error);

private:
  subordinate_account account_; // 账户信息
  // 连接信息 0 udp 1 tcp
  std::shared_ptr<SipSession> sip_session_[2];
  // 心跳检测
  std::shared_ptr<toolkit::Timer> keepalive_timer_;
  // local_server
  std::weak_ptr<LocalServer> local_server_weak_;

};

}


#endif //gb28181_src_SUBORDINATE_PLATFORM_IMPL_H



/**********************************************************************************************************
文件名称:   subordinate_platform_impl.h
创建时间:   25-2-7 下午3:11
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午3:11

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午3:11       描述:   创建文件

**********************************************************************************************************/