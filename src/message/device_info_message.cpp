#include "gb28181/message/device_info_message.h"

#include <gb28181/type_define_ext.h>
using namespace gb28181;
DeviceInfoMessageRequest::DeviceInfoMessageRequest(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::DeviceInfo;
}

 DeviceInfoMessageResponse::DeviceInfoMessageResponse(const std::string &device_id, ResultType result) : MessageBase(), result_(result) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::DeviceInfo;
}

bool DeviceInfoMessageResponse::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "The Result field invalid";
        return false;
    }
    from_xml_element(device_name_, root, "DeviceName");
    from_xml_element(manufacturer_, root, "Manufacturer");
    from_xml_element(model_, root, "Model");
    from_xml_element(firmware_, root, "Firmware");
    from_xml_element(channel_, root, "Channel");
    auto ele = root->FirstChildElement("Info");
    while (ele) {
        info_.emplace_back(ele->GetText());
        ele = ele->NextSiblingElement("Info");
    }
    return true;
}

bool DeviceInfoMessageResponse::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (result_ == ResultType::invalid) {
        error_message_ = "The Result field invalid";
        return false;
    }
    new_xml_element(result_, root, "Result");
    new_xml_element(device_name_, root, "DeviceName");
    new_xml_element(manufacturer_, root, "Manufacturer");
    new_xml_element(model_, root, "Model");
    new_xml_element(firmware_, root, "Firmware");
    new_xml_element(channel_, root, "Channel");
    for (auto &it : info_) {
        auto ele = root->InsertNewChildElement("Info");
        ele->SetText(it.c_str());
    }
    return true;
}




/**********************************************************************************************************
文件名称:   device_info_message.cpp
创建时间:   25-2-10 下午2:39
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午2:39

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午2:39       描述:   创建文件

**********************************************************************************************************/