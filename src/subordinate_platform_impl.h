#ifndef gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#define gb28181_src_SUBORDINATE_PLATFORM_IMPL_H
#include "gb28181/subordinate_platform.h"
#include "gb28181/type_define.h"
#include "platform_helper.h"

#include <functional>
#include <memory>
#include <mutex>


namespace toolkit {
class SockException;
}
namespace gb28181 {
class SipServer;
class RequestProxyImpl;
class MessageBase;
class DeviceStatusMessageRequest;
class DeviceInfoMessageResponse;
class DeviceInfoMessageRequest;
class KeepaliveMessageRequest;
class DeviceStatusMessageResponse;

}

namespace toolkit {
class Timer;
}

namespace gb28181 {
class SipSession;
class LocalServer;


class SubordinatePlatformImpl final: public SubordinatePlatform, public PlatformHelper {
public:
  explicit SubordinatePlatformImpl(subordinate_account account, const std::shared_ptr<SipServer> &server);
  ~SubordinatePlatformImpl() override;

  void shutdown() override;

  std::shared_ptr<SubordinatePlatformImpl> shared_from_this() {
    return std::dynamic_pointer_cast<SubordinatePlatformImpl>(PlatformHelper::shared_from_this());
  }
  std::weak_ptr<SubordinatePlatformImpl> weak_from_this() {
    return shared_from_this();
  }

  inline const subordinate_account &account() const override {
    return account_;
  }
  gb28181::sip_account &sip_account() override {
    return account_;
  }

  void set_status(PlatformStatusType status, std::string error);


public:
  int on_keep_alive(std::shared_ptr<KeepaliveMessageRequest> request);
  void on_device_info(std::shared_ptr<DeviceInfoMessageRequest> request, std::function<void(std::shared_ptr<MessageBase>)> &&reply);
  void on_device_status(std::shared_ptr<DeviceStatusMessageRequest> request, std::function<void(std::shared_ptr<MessageBase>)> &&reply);


  int on_response(MessageBase &&message, std::shared_ptr<sip_uas_transaction_t> transaction, std::shared_ptr<sip_message_t> request);

private:
  subordinate_account account_; // 账户信息
  // 心跳检测
  std::shared_ptr<toolkit::Timer> keepalive_timer_;
  std::function<void(std::shared_ptr<SubordinatePlatform>, std::shared_ptr<KeepaliveMessageRequest>)> on_keep_alive_callback_;
  std::atomic_int32_t platform_sn_{1};


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