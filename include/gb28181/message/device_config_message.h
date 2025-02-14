#ifndef gb28181_include_gb28181_message_DEVICE_CONFIG_MESSAGE_H
#define gb28181_include_gb28181_message_DEVICE_CONFIG_MESSAGE_H
#include <gb28181/message/message_base.h>

namespace gb28181 {
class DeviceConfigRequestMessage : public MessageBase {
public:
    explicit DeviceConfigRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceConfigRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceConfigRequestMessage(
        const std::string &device_id, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> config = {});

    std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> &config() { return config_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> config_;
};

class DeviceConfigResponseMessage final : public MessageBase {
public:
    explicit DeviceConfigResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceConfigResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceConfigResponseMessage(
        const std::string &device_id, ResultType result = ResultType::OK, std::string reason = "");

    ResultType result() const { return result_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_DEVICE_CONFIG_MESSAGE_H

/**********************************************************************************************************
文件名称:   device_config_message.h
创建时间:   25-2-13 下午6:26
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-13 下午6:26

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-13 下午6:26       描述:   创建文件

**********************************************************************************************************/