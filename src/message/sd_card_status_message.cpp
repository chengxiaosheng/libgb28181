#include <gb28181/message/sd_card_status_message.h>
#include <gb28181/type_define_ext.h>
using namespace gb28181;

SdCardRequestMessage::SdCardRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::SDCardStatus;
}
SdCardResponseMessage::SdCardResponseMessage(
    const std::string &device_id, int32_t sum_num, std::vector<SdCardInfoType> &&list, ResultType result,
    const std::string &reason)
    : MessageBase()
    , sum_num_(sum_num)
    , sd_cards_(std::move(list))
    , result_(result) {
    device_id_ = device_id;
    reason_ = reason;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::SDCardStatus;
}

bool SdCardResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(result_, root, "Result");
    from_xml_element(sum_num_, root, "SumNum");
    if (auto list = root->FirstChildElement("SDCardStatusInfo")) {
        auto item = list->FirstChildElement("Item");
        while (item) {
            SdCardInfoType item_val;
            from_xml_element(item_val.ID, item, "ID");
            from_xml_element(item_val.HddName, item, "HddName");
            from_xml_element(item_val.Status, item, "Status");
            from_xml_element(item_val.FormatProgress, item, "FormatProgress");
            from_xml_element(item_val.Capacity, item, "Capacity");
            from_xml_element(item_val.FreeSpace, item, "FreeSpace");
            sd_cards_.emplace_back(std::move(item_val));
            item = item->NextSiblingElement("Item");
        }
    }
    return true;
}

bool SdCardResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(result_, root, "Result");
    new_xml_element(sum_num_, root, "SumNum");
    if (!sd_cards_.empty()) {
        auto list = root->InsertNewChildElement("SDCardStatusInfo");
        for (const auto &item_val : sd_cards_) {
            auto item = list->InsertNewChildElement("Item");
            new_xml_element(item_val.ID, item, "ID");
            new_xml_element(item_val.HddName, item, "HddName");
            new_xml_element(item_val.Status, item, "Status");
            new_xml_element(item_val.FormatProgress, item, "FormatProgress");
            new_xml_element(item_val.Capacity, item, "Capacity");
            new_xml_element(item_val.FreeSpace, item, "FreeSpace");
        }
    }
    return true;
}

/**********************************************************************************************************
文件名称:   sd_card_status_message.cpp
创建时间:   25-2-10 下午3:18
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:18

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:18       描述:   创建文件

**********************************************************************************************************/