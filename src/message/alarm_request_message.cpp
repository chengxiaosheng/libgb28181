#include <gb28181/message/alarm_request_message.h>
#include <gb28181/type_define_ext.h>

#include <utility>
using namespace gb28181;

AlarmRequestMessage::AlarmRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::Alarm;
}
bool AlarmRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(start_alarm_priority_, root, "StartAlarmPriority");
    from_xml_element(end_alarm_priority_, root, "EndAlarmPriority");
    from_xml_element(alarm_method_, root, "AlarmMethod");
    from_xml_element(alarm_type_, root, "AlarmType");
    from_xml_element(start_alarm_time_, root, "StartAlarmTime");
    from_xml_element(end_alarm_time_, root, "EndAlarmTime");
    return true;
}
bool AlarmRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (!start_alarm_priority_.empty())
        new_xml_element(start_alarm_priority_, root, "StartAlarmPriority");
    if (!end_alarm_priority_.empty())
        new_xml_element(end_alarm_priority_, root, "EndAlarmPriority");
    if (!alarm_method_.empty())
        new_xml_element(alarm_method_, root, "AlarmMethod");
    if (!alarm_type_.empty())
        new_xml_element(alarm_type_, root, "AlarmType");
    if (!start_alarm_time_.empty())
        new_xml_element(start_alarm_time_, root, "StartAlarmTime");
    if (!end_alarm_time_.empty())
        new_xml_element(end_alarm_time_, root, "EndAlarmTime");
    return true;
}
AlarmNotifyMessage::AlarmNotifyMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::Alarm;
}
bool AlarmNotifyMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(alarm_priority_, root, "AlarmPriority");
    from_xml_element(alarm_method_, root, "AlarmMethod");
    from_xml_element(alarm_time_, root, "AlarmTime");
    from_xml_element(alarm_description_, root, "AlarmDescription");
    from_xml_element(latitude_, root, "Latitude");
    from_xml_element(longitude_, root, "Longitude");
    if (auto ele = root->FirstChildElement("Info")) {
        from_xml_element(info_.alarm_type, ele, "AlarmType");
        if ((ele = ele->FirstChildElement("AlarmTypeParam")); ele) {
            from_xml_element(info_.alarm_type_param.event_type, ele, "EventType");
        }
    }
    auto ext = root->FirstChildElement("ExtraInfo");
    while (ext) {
        extra_info_.emplace_back(ext->GetText());
        ext = ext->NextSiblingElement("ExtraInfo");
    }
    return true;
}
bool AlarmNotifyMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(alarm_priority_, root, "AlarmPriority");
    new_xml_element(alarm_method_, root, "AlarmMethod");
    new_xml_element(alarm_time_, root, "AlarmTime");
    new_xml_element(alarm_description_, root, "AlarmDescription");
    if (longitude_)
        new_xml_element(longitude_, root, "Longitude");
    if (latitude_)
        new_xml_element(latitude_, root, "Latitude");
    if (info_.alarm_type) {
        auto info = root->InsertNewChildElement("Info");
        new_xml_element(info_.alarm_type, info, "AlarmType");
        if (info_.alarm_type_param.event_type) {
            auto ext = info->InsertNewChildElement("AlarmTypeParam");
            new_xml_element(info_.alarm_type_param.event_type, ext, "EventType");
        }
    }
    for (auto &it : extra_info_) {
        new_xml_element(it, root, "ExtraInfo");
    }
    return true;
}

AlarmNotifyResponseMessage::AlarmNotifyResponseMessage(
    const std::string &device_id, ResultType result, std::string reason)
    : MessageBase()
    , result_(result) {
    device_id_ = device_id;
    reason_ = std::move(reason);
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::Alarm;
}
bool AlarmNotifyResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "invalid result";
        return false;
    }
    return true;
}
bool AlarmNotifyResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (result_ == ResultType::invalid) {
        error_message_ = "invalid result";
        return false;
    }
    new_xml_element(result_, root, "Result");
    return true;
}

/**********************************************************************************************************
文件名称:   alarm_request_message.cpp
创建时间:   25-2-14 下午4:37
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午4:37

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午4:37       描述:   创建文件

**********************************************************************************************************/
