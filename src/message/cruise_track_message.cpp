#include "gb28181/message/cruise_track_message.h"

#include <gb28181/type_define_ext.h>

#include <utility>
using namespace gb28181;
CruiseTrackListRequestMessage::CruiseTrackListRequestMessage(const std::string &device_id)
    : MessageBase() {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::CruiseTrackListQuery;
}

CruiseTrackRequestMessage::CruiseTrackRequestMessage(const std::string &device_id, int32_t number)
    : MessageBase()
    , number_(number) {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::CruiseTrackQuery;
}

bool CruiseTrackRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(number_, root, "Number")) {
        error_message_ = "Number not found";
        return false;
    }
    return true;
}
bool CruiseTrackRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(number_, root, "Number");
    return true;
}

CruiseTrackListResponseMessage::CruiseTrackListResponseMessage(
    const std::string &device_id, int32_t sum_num, std::vector<CruiseTrackListItemType> &&list, ResultType result,
    const std::string &reason)
    : MessageBase()
    , result_(result)
    , sum_num_(sum_num)
    , cruise_track_list_(std::move(list)) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::CruiseTrackListQuery;
}
CruiseTrackListResponseMessage::CruiseTrackListResponseMessage(
    const std::string &device_id, int32_t sum_num, std::vector<CruiseTrackListItemType> &&list)
    : MessageBase()
    , result_(ResultType::OK)
    , sum_num_(sum_num)
    , cruise_track_list_(std::move(list)) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::CruiseTrackListQuery;
}
bool CruiseTrackListResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(result_, root, "Result");
    from_xml_element(sum_num_, root, "SumNum");
    if (auto list = root->FirstChildElement("CruiseTrackList")) {
        auto item_ele = list->FirstChildElement("CruiseTrack");
        while (item_ele) {
            CruiseTrackListItemType item;
            from_xml_element(item.Number, item_ele, "Number");
            from_xml_element(item.Name, item_ele, "Name");
            cruise_track_list_.emplace_back(std::move(item));
            item_ele = item_ele->NextSiblingElement("CruiseTrack");
        }
    }
    return true;
}
bool CruiseTrackListResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(result_, root, "Result");
    new_xml_element(sum_num_, root, "SumNum");
    if (!cruise_track_list_.empty()) {
        auto list = root->InsertNewChildElement("CruiseTrackList");
        list->SetAttribute("Num", cruise_track_list_.size());
        for (auto &it : cruise_track_list_) {
            auto item_ele = list->InsertNewChildElement("CruiseTrack");
            new_xml_element(it.Number, item_ele, "Number");
            new_xml_element(it.Name, item_ele, "Name");
        }
    }
    return true;
}

CruiseTrackResponseMessage::CruiseTrackResponseMessage(
    const std::string &device_id, std::string name, int32_t sum_num, std::vector<CruisePointType> &&list,
    ResultType result, const std::string &reason)
    : MessageBase()
    , result_(result)
    , sum_num_(sum_num)
    , name_(std::move(name))
    , cruise_points_(std::move(list)) {
    device_id_ = device_id;
    reason_ = reason;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::CruiseTrackQuery;
}
CruiseTrackResponseMessage::CruiseTrackResponseMessage(
    const std::string &device_id, int sum_num, std::vector<CruisePointType> &&list)
    : MessageBase()
    , result_(ResultType::OK)
    , sum_num_(sum_num)
    , cruise_points_(std::move(list)) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::CruiseTrackQuery;
}

bool CruiseTrackResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "Result not found";
        return false;
    }
    from_xml_element(sum_num_, root, "SumNum");
    from_xml_element(name_, root, "Name");
    if (auto list = root->FirstChildElement("CruisePointList")) {
        auto item_ele = list->FirstChildElement("CruisePoint");
        while (item_ele) {
            CruisePointType item;
            from_xml_element(item.PresetIndex, item_ele, "PresetIndex");
            from_xml_element(item.StayTime, item_ele, "StayTime");
            from_xml_element(item.Speed, item_ele, "Speed");
            cruise_points_.emplace_back(item);
            item_ele = item_ele->NextSiblingElement("CruisePoint");
        }
    }
    return true;
}
bool CruiseTrackResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(result_, root, "Result");
    new_xml_element(sum_num_, root, "SumNum");
    from_xml_element(name_, root, "Name");
    if (!cruise_points_.empty()) {
        auto list = root->InsertNewChildElement("CruisePointList");
        list->SetAttribute("Num", cruise_points_.size());
        for (auto &it : cruise_points_) {
            auto item_ele = list->InsertNewChildElement("CruisePoint");
            new_xml_element(it.PresetIndex, item_ele, "PresetIndex");
            new_xml_element(it.StayTime, item_ele, "StayTime");
            new_xml_element(it.Speed, item_ele, "Speed");
        }
    }
    return true;
}

/**********************************************************************************************************
文件名称:   cruise_track_message.cpp
创建时间:   25-2-10 下午3:18
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:18

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:18       描述:   创建文件

**********************************************************************************************************/