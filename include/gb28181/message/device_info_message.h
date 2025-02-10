#ifndef gb28181_include_gb28181_message_DEVICE_INFO_MESSAGE_H
#define gb28181_include_gb28181_message_DEVICE_INFO_MESSAGE_H

#include "gb28181/message/message_base.h"
namespace gb28181 {
class DeviceInfoMessageRequest final : public MessageBase {
public:
    explicit DeviceInfoMessageRequest(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceInfoMessageRequest(MessageBase &&messageBase)
        : MessageBase(messageBase) {}
    explicit DeviceInfoMessageRequest(const std::string &device_id);
};

class DeviceInfoMessageResponse final : public MessageBase {
public:
    explicit DeviceInfoMessageResponse(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceInfoMessageResponse(MessageBase &&messageBase)
        : MessageBase(messageBase) {}
    explicit DeviceInfoMessageResponse(const std::string &device_id, ResultType result = ResultType::OK);

    /**
     *目标设备的名称 （可选）
     * @return
     */
    std::string &device_name() { return device_name_; }
    /**
     *查询结果 （必选）
     * @return
     */
    ResultType &result() { return result_; }
    /**
     *设备生产商 （可选）
     * @return
     */
    std::string &manufacturer() { return manufacturer_; }
    /**
     *  设备型号 （可选）
     * @return
     */
    std::string &model() { return model_; }
    /**
     * 设备固件版本 （可选）
     * @return
     */
    std::string &firmware() { return firmware_; }
    /**
     * 设备通道数 （可选）
     * @return
     */
    std::optional<int32_t> &channel() { return channel_; }
    /**
     * 扩展信息，可多项
     * @return
     */
    std::vector<std::string> &info() { return info_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    // 目标设备的名称 （可选）
    std::string device_name_;
    // 查询结果 （必选）
    ResultType result_{ResultType::invalid};
    // 设备生产商 （可选）
    std::string manufacturer_;
    // 设备型号 （可选）
    std::string model_;
    // 设备固件版本 （可选）
    std::string firmware_;
    // 设备通道数 （可选）
    std::optional<int32_t> channel_;
    // 扩展信息，可多项
    std::vector<std::string> info_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_DEVICE_INFO_MESSAGE_H

/**********************************************************************************************************
文件名称:   device_info_message.h
创建时间:   25-2-10 下午2:35
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午2:35

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午2:35       描述:   创建文件

**********************************************************************************************************/