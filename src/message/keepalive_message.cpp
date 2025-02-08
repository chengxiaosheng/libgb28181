#include "gb28181/message/keepalive_message.h"

#include <gb28181/type_define_ext.h>

using namespace gb28181;
 KeepaliveMessageRequest::KeepaliveMessageRequest(const std::string &device_id, ResultType status) : MessageBase(), status_(status) {
     root_ = MessageRootType::Notify;
     cmd_ = MessageCmdType::Keepalive;
     device_id_ = device_id;
 }

bool KeepaliveMessageRequest::load_detail() {
     auto root = xml_ptr_->RootElement();
     if (!from_xml_element(status_, root, "Status")) {
         return false;
     }
     from_xml_element(info_, root, "Info");
     return true;
}
bool KeepaliveMessageRequest::parse_detail() {
     auto root = xml_ptr_->RootElement();
     new_xml_element(status_, root, "Status");
     new_xml_element(info_, root, "Info");
     return true;
}






/**********************************************************************************************************
文件名称:   keepalive_message.cpp
创建时间:   25-2-8 下午4:35
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-8 下午4:35

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-8 下午4:35       描述:   创建文件

**********************************************************************************************************/