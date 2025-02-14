#include "gb28181/message/home_position_message.h"

#include <gb28181/type_define_ext.h>

using namespace gb28181;

HomePositionRequestMessage::HomePositionRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::HomePositionQuery;
}

HomePositionResponseMessage::HomePositionResponseMessage(
    const std::string &device_id, std::optional<HomePositionType> &&home_position)
    : MessageBase()
    , home_position_(home_position) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::HomePositionQuery;
}
bool HomePositionResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("HomePosition"); ele && !ele->NoChildren()) {
        HomePositionType home_position;
        from_xml_element(home_position.Enabled, ele, "Enabled");
        from_xml_element(home_position.PresetIndex, ele, "PresetIndex");
        from_xml_element(home_position.ResetTime, ele, "ResetTime");
        home_position_ = home_position;
    }
    return true;
}
bool HomePositionResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (home_position_) {
        auto ele = root->InsertNewChildElement("HomePosition");
        new_xml_element(home_position_->Enabled, ele, "Enabled");
        new_xml_element(home_position_->PresetIndex, ele, "PresetIndex");
        new_xml_element(home_position_->ResetTime, ele, "ResetTime");
    }
    return true;
}

/**********************************************************************************************************
文件名称:   home_position_message.cpp
创建时间:   25-2-10 下午3:15
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:15

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:15       描述:   创建文件

**********************************************************************************************************/