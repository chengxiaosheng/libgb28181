#include "gb28181/message/record_info_message.h"

#include <gb28181/type_define_ext.h>
using namespace gb28181;

RecordInfoRequestMessage::RecordInfoRequestMessage(
    const std::string &device_id, std::string start_time, std::string end_time)
    : MessageBase()
    , start_time_(std::move(start_time))
    , end_time_(std::move(end_time)) {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::RecordInfo;
}

bool RecordInfoRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(start_time_, root, "StartTime")) {
        error_message_ = "StartTime not found";
        return false;
    }
    if (!from_xml_element(end_time_, root, "EndTime")) {
        error_message_ = "EndTime not found";
        return false;
    }
    from_xml_element(file_path_, root, "FilePath");
    from_xml_element(address_, root, "Address");
    from_xml_element(secrecy_, root, "Secrecy");
    from_xml_element(type_, root, "Type");
    from_xml_element(recorder_id_, root, "RecorderID");
    from_xml_element(indistinct_query_, root, "IndistinctQuery");
    from_xml_element(alarm_method_, root, "AlarmMethod");
    from_xml_element(alarm_type_, root, "AlarmType");
    from_xml_element(stream_number_, root, "StreamNumber");
    return true;
}
bool RecordInfoRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (start_time_.empty()) {
        error_message_ = "StartTime invalid";
        return false;
    }
    if (end_time_.empty()) {
        error_message_ = "EndTime invalid";
        return false;
    }
    new_xml_element(start_time_, root, "StartTime");
    new_xml_element(end_time_, root, "EndTime");

    if (!file_path_.empty())
        new_xml_element(file_path_, root, "FilePath");
    if (!address_.empty())
        new_xml_element(address_, root, "Address");
    from_xml_element(secrecy_, root, "Secrecy");
    if (!type_.empty())
        new_xml_element(type_, root, "Type");
    if (!recorder_id_.empty())
        new_xml_element(recorder_id_, root, "RecorderID");
    if (!indistinct_query_.empty())
        new_xml_element(indistinct_query_, root, "IndistinctQuery");
    if (!alarm_method_.empty())
        new_xml_element(alarm_method_, root, "AlarmMethod");
    if (!alarm_type_.empty())
        new_xml_element(alarm_type_, root, "AlarmType");
    if (stream_number_)
        new_xml_element(stream_number_, root, "StreamNumber");
    return true;
}

RecordInfoResponseMessage::RecordInfoResponseMessage(
    const std::string &device_id, std::string name, int32_t sum_num, std::vector<ItemFileType> &&record_list,
    std::vector<std::string> &&extra_info)
    : MessageBase()
    , name_(std::move(name))
    , record_list_(std::move(record_list))
    , extra_info_(std::move(extra_info)) {
    sum_num_ = sum_num;
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::RecordInfo;
}
RecordInfoResponseMessage::RecordInfoResponseMessage(
    const std::string &device_id, int32_t sum_num, std::vector<ItemFileType> &&record_list)
    : MessageBase()
    , record_list_(std::move(record_list)) {
    sum_num_ = sum_num;
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::RecordInfo;
}
bool RecordInfoResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(name_, root, "Name");
    from_xml_element(sum_num_, root, "SumNum"); // 由于海康部分设备在预置点查询中未写入 SumNum， 安全起见此处也不验证
    if (auto list_ele = root->FirstChildElement("RecordList"); list_ele && !list_ele->NoChildren()) {
        auto item_ele = list_ele->FirstChildElement("Item");
        while (item_ele) {
            if (!item_ele->NoChildren()) {
                item_ele = item_ele->NextSiblingElement("Item");
                continue;
            }
            ItemFileType item;
            from_xml_element(item.DeviceID, item_ele, "DeviceID");
            from_xml_element(item.Name, item_ele, "Name");
            from_xml_element(item.FilePath, item_ele, "FilePath ");
            from_xml_element(item.Address, item_ele, "Address");
            from_xml_element(item.StartTime, item_ele, "StartTime");
            from_xml_element(item.EndTime, item_ele, "EndTime");
            from_xml_element(item.Secrecy, item_ele, "Secrecy");
            from_xml_element(item.Type, item_ele, "Type");
            from_xml_element(item.RecorderID, item_ele, "RecorderID");
            from_xml_element(item.FileSize, item_ele, "FileSize");
            from_xml_element(item.RecordLocation, item_ele, "RecordLocation");
            from_xml_element(item.StreamNumber, item_ele, "StreamNumber");
            record_list_.emplace_back(std::move(item));
            item_ele = item_ele->NextSiblingElement("Item");
        }
    }
    if (auto ext_ele = root->FirstChildElement("ExtraInfo")) {
        while (ext_ele) {
            extra_info_.emplace_back(ext_ele->GetText());
            ext_ele = ext_ele->NextSiblingElement("ExtraInfo");
        }
    }

    return true;
}
bool RecordInfoResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(name_, root, "Name");
    new_xml_element(sum_num_, root, "SumNum"); // 由于海康部分设备在预置点查询中未写入 SumNum， 安全起见此处也不验证
    auto list_ele = root->InsertNewChildElement("RecordList");
    list_ele->SetAttribute("Num", record_list_.size());
    for (auto &it : record_list_) {
        auto item_ele = list_ele->InsertNewChildElement("Item");
        if (!it.DeviceID.empty())
            new_xml_element(it.DeviceID, item_ele, "DeviceID");
        if (!it.Name.empty())
            new_xml_element(it.Name, item_ele, "Name");
        if (!it.FilePath.empty())
            new_xml_element(it.FilePath, item_ele, "FilePath ");
        if (!it.Address.empty())
            new_xml_element(it.Address, item_ele, "Address");
        if (!it.StartTime.empty())
            new_xml_element(it.StartTime, item_ele, "StartTime");
        if (!it.EndTime.empty())
            new_xml_element(it.EndTime, item_ele, "EndTime");
        new_xml_element(it.Secrecy, item_ele, "Secrecy");
        if (!it.Type.empty())
            new_xml_element(it.Type, item_ele, "Type");
        if (!it.RecorderID.empty())
            new_xml_element(it.RecorderID, item_ele, "RecorderID");
        if (!it.FileSize.empty())
            new_xml_element(it.FileSize, item_ele, "FileSize");
        if (!it.RecordLocation.empty())
            new_xml_element(it.RecordLocation, item_ele, "RecordLocation");
        if (it.StreamNumber)
            new_xml_element(it.StreamNumber, item_ele, "StreamNumber");
    }
    for (auto &it : extra_info_) {
        auto item_ele = list_ele->InsertNewChildElement("ExtraInfo");
        item_ele->SetText(it.c_str());
    }
    return true;
}

/**********************************************************************************************************
文件名称:   record_info_message.cpp
创建时间:   25-2-10 下午3:18
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:18

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:18       描述:   创建文件

**********************************************************************************************************/