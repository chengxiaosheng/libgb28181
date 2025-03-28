#ifndef gb28181_include_gb28181_message_DEVICE_STATUS_MESSAGE_H
#define gb28181_include_gb28181_message_DEVICE_STATUS_MESSAGE_H
#include "gb28181/message/message_base.h"

namespace gb28181 {
class GB28181_EXPORT DeviceStatusMessageRequest final : public MessageBase {
public:
    explicit DeviceStatusMessageRequest(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceStatusMessageRequest(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceStatusMessageRequest(const std::string &device_id);
};

class GB28181_EXPORT DeviceStatusMessageResponse final : public MessageBase {
public:
    explicit DeviceStatusMessageResponse(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceStatusMessageResponse(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceStatusMessageResponse(
        const std::string &device_id, ResultType result = ResultType::OK, OnlineType online = OnlineType::ONLINE,
        ResultType status = ResultType::OK);

    /**
     * 查询结果标志 必选
     * @return
     */
    ResultType &result() { return result_; }
    /**
     * 是否在线 必选
     * @return
     */
    OnlineType &online() { return online_; }
    /**
     * 是否正常工作 必选
     * @return
     */
    ResultType &status() { return status_; }
    /**
     * 是否编码 可选
     * @return
     */
    StatusType &encode() { return encode_; }
    /**
     * 是否录像 可选
     * @return
     */
    StatusType &record() { return record_; }
    /**
     * 设备时间和日期 可选
     * @return
     */
    std::string &device_time() { return device_time_; }
    /**
     * 报警设备状态列表 可选
     * @return
     */
    DeviceStatusAlarmStatus &alarm_status() { return alarm_status_; }
    /**
     * 扩展信息, 可多项 可选
     * @return
     */
    std::vector<std::string> &info() { return info_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    // 查询结果标志 必选
    ResultType result_ { ResultType::invalid };
    // 是否在线 必选
    OnlineType online_ { OnlineType::invalid };
    // 是否正常工作 必选
    ResultType status_ { ResultType::invalid };
    // 是否编码 可选
    StatusType encode_ { StatusType::invalid };
    // 是否录像 可选
    StatusType record_ { StatusType::invalid };
    // 设备时间和日期 可选
    std::string device_time_{};
    // 报警设备状态列表 可选
    DeviceStatusAlarmStatus alarm_status_;
    // 扩展信息, 可多项 可选
    std::vector<std::string> info_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_DEVICE_STATUS_MESSAGE_H

/**********************************************************************************************************
文件名称:   device_status_message.h
创建时间:   25-2-10 下午3:12
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:12

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:12       描述:   创建文件

**********************************************************************************************************/