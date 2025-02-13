#ifndef gb28181_include_gb28181_message_HOME_POSITION_MESSAGE_H
#define gb28181_include_gb28181_message_HOME_POSITION_MESSAGE_H
#include <gb28181/message/message_base.h>
namespace gb28181 {
class HomePositionRequestMessage : public MessageBase {
public:
    explicit HomePositionRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit HomePositionRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit HomePositionRequestMessage(const std::string &device_id);
};

class HomePositionResponseMessage : public MessageBase {
public:
    explicit HomePositionResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit HomePositionResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit HomePositionResponseMessage(const std::string &device_id, HomePositionType &&home_position);

protected:
    bool load_detail() override;
    bool parse_detail() override;
private:
    HomePositionType home_position_;
};
} // namespace gb28181

#endif // gb28181_include_gb28181_message_HOME_POSITION_MESSAGE_H

/**********************************************************************************************************
文件名称:   home_position_message.h
创建时间:   25-2-10 下午3:15
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:15

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:15       描述:   创建文件

**********************************************************************************************************/