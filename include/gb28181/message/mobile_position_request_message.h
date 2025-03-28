#ifndef gb28181_MOBILE_POSITION_REQUEST_MESSAGE_H
#define gb28181_MOBILE_POSITION_REQUEST_MESSAGE_H
#include <gb28181/message/message_base.h>

namespace gb28181 {
class GB28181_EXPORT MobilePositionRequestMessage : public MessageBase {
public:
    explicit MobilePositionRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit MobilePositionRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit MobilePositionRequestMessage(const std::string &device_id, int32_t interval = 5);

    /**
     * 动设备位置信息上报时间间隔，单位：秒，默认值5(可选)
     */
    int32_t &interval() { return interval_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    int32_t interval_ { 5 };
};

} // namespace gb28181

#endif // gb28181_MOBILE_POSITION_REQUEST_MESSAGE_H

/**********************************************************************************************************
文件名称:   mobile_position_request_message.h
创建时间:   25-2-14 下午4:34
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午4:34

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午4:34       描述:   创建文件

**********************************************************************************************************/