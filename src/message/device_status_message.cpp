#include "gb28181/message/device_status_message.h"

#include <gb28181/type_define_ext.h>
using namespace gb28181;

DeviceStatusMessageRequest::DeviceStatusMessageRequest(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::DeviceStatus;
}
DeviceStatusMessageResponse::DeviceStatusMessageResponse(
    const std::string &device_id, ResultType result, OnlineType online, ResultType status)
    : MessageBase()
    , result_(result)
    , online_(online)
    , status_(status) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::DeviceStatus;
}

bool DeviceStatusMessageResponse::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "The Result field invalid";
        return false;
    }
    if (!from_xml_element(online_, root, "Online")) {
        error_message_ = "The Online field invalid";
        return false;
    }
    if (!from_xml_element(status_, root, "Status")) {
        error_message_ = "The Status field invalid";
        return false;
    }
    from_xml_element(reason_, root, "Reason");
    from_xml_element(encode_, root, "Encode");
    from_xml_element(record_, root, "Record");
    from_xml_element(device_time_, root, "DeviceTime");
    from_xml_element(alarm_status_, root, "Alarmstatus");
    auto ele = root->FirstChildElement("Info");
    while (ele) {
        info_.emplace_back(ele->GetText());
        ele = ele->NextSiblingElement("Info");
    }
    return true;
}
bool DeviceStatusMessageResponse::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (result_ == ResultType::invalid) {
        error_message_ = "The Result field invalid";
        return false;
    }
    if (online_ == OnlineType::invalid) {
        error_message_ = "The Online field invalid";
        return false;
    }
    if (status_ == ResultType::invalid) {
        error_message_ = "The Status field invalid";
        return false;
    }
    new_xml_element(result_, root, "Result");
    new_xml_element(online_, root, "Online");
    new_xml_element(status_, root, "Status");
    new_xml_element(reason_, root, "Reason");
    if (encode_ != StatusType::invalid) {
        new_xml_element(encode_, root, "Encode");
    }
    if (record_ != StatusType::invalid) {
        new_xml_element(record_, root, "Record");
    }
    if (!device_time_.empty()) {
        new_xml_element(device_time_, root, "DeviceTime");
    }
    if (!alarm_status_.Item.empty()) {
        new_xml_element(alarm_status_, root, "Alarmstatus");
    }
    for (auto &it : info_) {
        auto ele = root->InsertNewChildElement("Info");
        ele->SetText(it.c_str());
    }
    return true;

}



/**********************************************************************************************************
文件名称:   device_status_message.cpp
创建时间:   25-2-10 下午3:13
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:13

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:13       描述:   创建文件

**********************************************************************************************************/