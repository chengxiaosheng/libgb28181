#include "gb28181/message/ptz_position_message.h"

#include <gb28181/type_define_ext.h>
using namespace gb28181;

PTZPositionRequestMessage::PTZPositionRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::PTZPosition;
}

PTZPositionResponseMessage::PTZPositionResponseMessage(
    const std::string &device_id, ResultType result, const std::string &reason)
    : MessageBase()
    , result_(result) {
    device_id_ = device_id;
    reason_ = reason;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::PTZPosition;
}
bool PTZPositionResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(result_, root, "Result");
    from_xml_element(pan_, root, "Pan");
    from_xml_element(tilt_, root, "Tilt");
    from_xml_element(zoom_, root, "Zoom");
    from_xml_element(horizontal_field_angle_, root, "HorizontalFieldAngle");
    from_xml_element(vertical_field_angle_, root, "VerticalFieldAngle");
    from_xml_element(max_view_distance_, root, "MaxViewDistance");
    return true;
}
bool PTZPositionResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(result_, root, "Result");
    if (pan_)
        new_xml_element(pan_, root, "Pan");
    if (tilt_)
        new_xml_element(tilt_, root, "Tilt");
    if (zoom_)
        new_xml_element(zoom_, root, "Zoom");
    if (horizontal_field_angle_)
        new_xml_element(horizontal_field_angle_, root, "HorizontalFieldAngle");
    if (vertical_field_angle_)
        new_xml_element(vertical_field_angle_, root, "VerticalFieldAngle");
    if (max_view_distance_)
        new_xml_element(max_view_distance_, root, "MaxViewDistance");
    return true;
}

/**********************************************************************************************************
文件名称:   ptz_position_message.cpp
创建时间:   25-2-10 下午3:17
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:17

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:17       描述:   创建文件

**********************************************************************************************************/