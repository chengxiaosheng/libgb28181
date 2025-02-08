#ifndef gb28181_include_gb28181_message_KEEPALIVE_MESSAGE_H
#define gb28181_include_gb28181_message_KEEPALIVE_MESSAGE_H

#include "gb28181/message/message_base.h"

namespace gb28181 {
class KeepaliveMessageRequest final : public MessageBase {
public:
  explicit KeepaliveMessageRequest(const std::shared_ptr<tinyxml2::XMLDocument> &xml) : MessageBase(xml) {}
  explicit KeepaliveMessageRequest(MessageBase &&messageBase) : MessageBase(messageBase) {}
  explicit KeepaliveMessageRequest(const std::string &device_id, ResultType status = ResultType::OK);

  ResultType & status() {
    return status_;
  }
  DeviceIdArr & info() {
    return info_;
  }

protected:
  bool load_detail() override;
  bool parse_detail() override;

private:
  ResultType status_{ResultType::invalid};
  DeviceIdArr info_;
};
class KeepaliveMessageResponse : public MessageBase {};
} // namespace gb28181

#endif // gb28181_include_gb28181_message_KEEPALIVE_MESSAGE_H

/**********************************************************************************************************
文件名称:   keepalive_message.h
创建时间:   25-2-8 下午4:33
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-8 下午4:33

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-8 下午4:33       描述:   创建文件

**********************************************************************************************************/