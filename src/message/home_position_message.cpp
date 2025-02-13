#include "gb28181/message/home_position_message.h"

using namespace gb28181;

HomePositionRequestMessage::HomePositionRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::HomePositionQuery;
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