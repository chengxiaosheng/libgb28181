#include "gb28181/message/preset_message.h"

#include <gb28181/type_define_ext.h>

using namespace gb28181;

PresetRequestMessage::PresetRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::PresetQuery;
}

PresetResponseMessage::PresetResponseMessage(
    const std::string &device_id, std::vector<PresetListItem> &&vec, int32_t sum_num)
    : MessageBase()
    , sum_num_(sum_num)
    , preset_list_(std::move(vec)) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::PresetQuery;
}

bool PresetResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    // 海康部分设备在回复时没有 SumNum 字段，此处强校验会导致无法确认到结果
    // if (!from_xml_element(sum_num_, root, "SumNum")) {
    //     error_message_ = "SumNum element does not exist";
    //     return false;
    // }
    from_xml_element(sum_num_, root, "SumNum");
    if (auto ele = root->FirstChildElement("PresetList")) {
        auto item = ele->FirstChildElement("Item");
        while (item != nullptr) {
            if (item->NoChildren()) {
                item = item->NextSiblingElement("Item");
                continue;
            }
            PresetListItem item_val;
            from_xml_element(item_val.PresetID, item, "PresetID");
            from_xml_element(item_val.PresetName, item, "PresetName");
            preset_list_.emplace_back(std::move(item_val));
            item = item->NextSiblingElement("Item");
        }
        return true;
    }
    error_message_ = "the PresetList node does not exist";
    return false;
}

bool PresetResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    new_xml_element(sum_num_ ? sum_num() : preset_list_.size(), root, "SumNum");
    auto ele = root->InsertNewChildElement("PresetList");
    ele->SetAttribute("Num", preset_list_.size());
    for (auto &p : preset_list_) {
        auto item = ele->InsertNewChildElement("Item");
        new_xml_element(p.PresetID, item, "PresetID");
        new_xml_element(p.PresetName, item, "PresetName");
    }
    return true;
}

/**********************************************************************************************************
文件名称:   preset_message.cpp
创建时间:   25-2-10 下午3:16
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:16

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:16       描述:   创建文件

**********************************************************************************************************/