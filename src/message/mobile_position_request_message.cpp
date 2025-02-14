#include <Network/Buffer.h>
#include <gb28181/message/mobile_position_request_message.h>
#include <gb28181/type_define_ext.h>
using namespace gb28181;

MobilePositionRequestMessage::MobilePositionRequestMessage(const std::string &device_id, int32_t interval)
    : MessageBase()
    , interval_(interval) {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::MobilePosition;
}

bool MobilePositionRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(interval_, root, "Interval");
    return true;
}
bool MobilePositionRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(interval_, root, "Interval");
    return true;
}


/**********************************************************************************************************
文件名称:   mobile_position_request_message.cpp
创建时间:   25-2-14 下午4:38
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午4:38

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午4:38       描述:   创建文件

**********************************************************************************************************/