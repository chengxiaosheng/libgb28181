#include "gb28181/message/broadcast_message.h"

#include <gb28181/type_define_ext.h>
using namespace gb28181;

BroadcastNotifyRequest::BroadcastNotifyRequest(const std::string &source_id, const std::string &target_id)
    : MessageBase()
    , source_id_(source_id)
    , target_id_(target_id) {
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::Broadcast;
}
bool BroadcastNotifyRequest::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(source_id_, root, "SourceID")) {
        error_message_ = "invalid parameter, SourceID missing";
        return false;
    }
    if (!from_xml_element(target_id_, root, "TargetID")) {
        error_message_ = "invalid parameter, TargetID missing";
        return false;
    }
    return true;
}

bool BroadcastNotifyRequest::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(source_id_, root, "SourceID");
    new_xml_element(target_id_, root, "TargetID");
    return true;
}

BroadcastNotifyResponse::BroadcastNotifyResponse(std::string device_id, ResultType result)
    : MessageBase()
    , result_(result) {
    device_id_ = std::move(device_id);
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::Broadcast;
}
bool BroadcastNotifyResponse::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "invalid parameter, Result missing";
        return false;
    }
    return true;
}
bool BroadcastNotifyResponse::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(result_, root, "Result");
    return true;
}

/**********************************************************************************************************
文件名称:   broadcast_message.cpp
创建时间:   25-2-10 下午2:06
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午2:06

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午2:06       描述:   创建文件

**********************************************************************************************************/