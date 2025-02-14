#ifndef gb28181_include_gb28181_message_PTZ_POSITION_MESSAGE_H
#define gb28181_include_gb28181_message_PTZ_POSITION_MESSAGE_H
#include <gb28181/message/message_base.h>

namespace gb28181 {

class PTZPositionRequestMessage final : public MessageBase {
public:
    explicit PTZPositionRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit PTZPositionRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit PTZPositionRequestMessage(const std::string &device_id);
};

class PTZPositionResponseMessage final : public MessageBase {
public:
    explicit PTZPositionResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit PTZPositionResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit PTZPositionResponseMessage(
        const std::string &device_id, ResultType result = ResultType::OK, const std::string &reason = "");

    ResultType &result() { return result_; };
    std::optional<double> &pan() { return pan_; }
    std::optional<double> &tilt() { return tilt_; }
    std::optional<double> &zoom() { return zoom_; }
    std::optional<double> &horizontal_field_angle() { return horizontal_field_angle_; }
    std::optional<double> &vertical_field_angle() { return vertical_field_angle_; }
    std::optional<double> &max_view_distance() { return max_view_distance_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
    std::optional<double> pan_;
    std::optional<double> tilt_;
    std::optional<double> zoom_;
    std::optional<double> horizontal_field_angle_;
    std::optional<double> vertical_field_angle_;
    std::optional<double> max_view_distance_;
};
}; // namespace gb28181

#endif // gb28181_include_gb28181_message_PTZ_POSITION_MESSAGE_H

/**********************************************************************************************************
文件名称:   ptz_position_message.h
创建时间:   25-2-10 下午3:17
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:17

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:17       描述:   创建文件

**********************************************************************************************************/