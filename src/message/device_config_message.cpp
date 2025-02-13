#include <gb28181/message/device_config_message.h>
#include <gb28181/type_define_ext.h>

using namespace gb28181;


 DeviceConfigRequestMessage::DeviceConfigRequestMessage(
    const std::string &device_id, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> config) : MessageBase(), config_(std::move(config)) {
     device_id_ = device_id;
     root_ = MessageRootType::Control;
     cmd_ = MessageCmdType::DeviceConfig;


}
bool DeviceConfigRequestMessage::load_detail() {
     auto root = xml_ptr_->RootElement();
     for (auto ele = root->FirstChildElement(); ele; ele = ele->NextSiblingElement()) {
        if (ele->Name() == )
     }

}
bool DeviceConfigRequestMessage::parse_detail() {

}







/**********************************************************************************************************
文件名称:   device_config_message.cpp
创建时间:   25-2-13 下午7:06
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-13 下午7:06

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-13 下午7:06       描述:   创建文件

**********************************************************************************************************/