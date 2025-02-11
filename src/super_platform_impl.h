#ifndef gb28181_src_SUPER_PLATFORM_IMPL_H
#define gb28181_src_SUPER_PLATFORM_IMPL_H

#include "gb28181/super_platform.h"
#include <gb28181/type_define.h>
#include <memory>
#include <mutex>
#include <platform_helper.h>

namespace gb28181 {
class SipServer;
class SuperPlatformImpl final: public SuperPlatform , public PlatformHelper {
public:
  SuperPlatformImpl(super_account account, const std::shared_ptr<SipServer> &server);

private:
  super_account account_;
  std::recursive_mutex sip_session_mutex_;
  std::shared_ptr<SipSession> sip_session_[2];
  std::weak_ptr<SipServer> local_server_weak_;
  std::atomic_int32_t platform_sn_{1};


};
}




#endif //gb28181_src_SUPER_PLATFORM_IMPL_H



/**********************************************************************************************************
文件名称:   super_platform_impl.h
创建时间:   25-2-11 下午5:00
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-11 下午5:00

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-11 下午5:00       描述:   创建文件

**********************************************************************************************************/