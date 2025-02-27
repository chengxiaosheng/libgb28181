#include "gb28181/message/catalog_message.h"

#include <functional>
#include <gb28181/type_define_ext.h>

using namespace gb28181;

CatalogRequestMessage::CatalogRequestMessage(const std::string &device_id, std::string start_time, std::string end_time)
    : MessageBase()
    , start_time_(std::move(start_time))
    , end_time_(std::move(end_time)) {
    device_id_ = device_id;
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::Catalog;
}
bool CatalogRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (device_id_->empty()) {
        error_message_ = "The DeviceID field invalid";
        return false;
    }
    from_xml_element(start_time_, root, "StartTime");
    from_xml_element(end_time_, root, "EndTime");
    return true;
}
bool CatalogRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (!start_time_.empty()) {
        new_xml_element(start_time_, root, "StartTime");
    }
    if (!end_time_.empty()) {
        new_xml_element(end_time_, root, "EndTime");
    }
    return true;
}

CatalogResponseMessage::CatalogResponseMessage(
    const std::string &device_id, int sum_num, std::vector<ItemTypeInfo> &&items, std::vector<std::string> &&extra)
    : MessageBase()
    , sum_num_(sum_num)
    , items_(std::move(items))
    , extra_(std::move(extra)) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::Catalog;
}

static std::unordered_map<std::string_view,std::function<void(ItemTypeInfo &, const tinyxml2::XMLElement *)>> fields_map_ = {
{ "DeviceID", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.DeviceID, root, nullptr); } },
{ "Name", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Name, root, nullptr); } },
{ "Manufacturer", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Manufacturer, root, nullptr);} },
{ "Model", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Model, root, nullptr);} },
{ "CivilCode", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.CivilCode, root, nullptr);} },
{ "Block", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Block, root, nullptr);} },
{ "Address", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Address, root, nullptr);} },
{ "Parental", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Parental, root, nullptr);} },
{ "ParentID", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.ParentID, root, nullptr);} },
{ "RegisterWay", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.RegisterWay, root, nullptr);} },
{ "SecurityLevelCode", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.SecurityLevelCode, root, nullptr);} },
{ "Secrecy", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Secrecy, root, nullptr);} },
{ "IPAddress", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.IPAddress, root, nullptr);} },
{ "Port", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Port, root, nullptr);} },
{ "Password", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Password, root, nullptr);} },
{ "Status", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { item.Status = StatusType::invalid; from_xml_element(item.Status.value(), root, nullptr);} },
{ "Longitude", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Longitude, root, nullptr);} },
{ "Latitude", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Latitude, root, nullptr);} },
{ "Event", [](ItemTypeInfo &item, const tinyxml2::XMLElement *root) { item.Event = ItemEventType::invalid; from_xml_element(item.Event.value(), root, nullptr);} }};

static std::unordered_map<std::string_view, std::function<void(ItemTypeInfoDetail &, const tinyxml2::XMLElement *)>> sub_elements_map_ = {
    { "PTZType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.PTZType, root, nullptr); } },
{ "PhotoelectricImagingType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.PhotoelectricImagingType, root, nullptr); } },
{ "CapturePositionType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.CapturePositionType, root, nullptr); } },
{ "RoomType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.RoomType, root, nullptr); } },
{ "SupplyLightType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.SupplyLightType, root, nullptr); } },
{ "DirectionType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.DirectionType, root, nullptr); } },
{ "Resolution", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.Resolution, root, nullptr); } },
{ "StreamNumberList", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.StreamNumberList, root, nullptr); } },
{ "DownloadSpeed", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.DownloadSpeed, root, nullptr); } },
{ "SVCSpaceSupportMode", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.SVCSpaceSupportMode, root, nullptr); } },
{ "SVCTimeSupportMode", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.SVCTimeSupportMode, root, nullptr); } },
{ "SSVCRatioSupportList", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.SSVCRatioSupportList, root, nullptr); } },
{ "MobileDeviceType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.MobileDeviceType, root, nullptr); } },
{ "HorizontalFieldAngle", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.HorizontalFieldAngle, root, nullptr); } },
{ "VerticalFieldAngle", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.VerticalFieldAngle, root, nullptr); } },
{ "MaxViewDistance", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.MaxViewDistance, root, nullptr); } },
{ "GrassrootsCode", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.GrassrootsCode, root, nullptr); } },
{ "PointType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.PointType, root, nullptr); } },
{ "PointCommonName", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.PointCommonName, root, nullptr); } },
{ "MAC", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.MAC, root, nullptr); } },
{ "FunctionType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.FunctionType, root, nullptr); } },
{ "EncodeType", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.EncodeType, root, nullptr); } },
{ "InstallTime", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.InstallTime, root, nullptr); } },
{ "ManagementUnit", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.ManagementUnit, root, nullptr); } },
{ "ContactInfo", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.ContactInfo, root, nullptr); } },
{ "RecordSaveDays", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.RecordSaveDays, root, nullptr); } },
{ "IndustrialClassification", [](ItemTypeInfoDetail &item, const tinyxml2::XMLElement *root) { from_xml_element(item.IndustrialClassification, root, nullptr); } },
};



bool CatalogResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(sum_num_, root, "SumNum");
    if (auto list = root->FirstChildElement("DeviceList")) {
        auto item = list->FirstChildElement("Item");
        while (item) {
            if (item->NoChildren()) {
                item = item->NextSiblingElement("Item");
                continue;
            }
            ItemTypeInfo item_val;
            auto item_ele = item->FirstChildElement();
            while (item_ele) {
                if (item_ele->Name() == "Info") {
                    ItemTypeInfoDetail detail;
                    auto detail_ele = item_ele->FirstChildElement();
                    while (detail_ele) {
                        if (auto detail_it = sub_elements_map_.find(detail_ele->Name()); detail_it != sub_elements_map_.end()) {
                            detail_it->second(detail, item_ele);
                        }
                        detail_ele = detail_ele->NextSiblingElement();
                    }
                    item_val.Info = std::move(detail);
                } else {
                    if (auto item_it = fields_map_.find(item_ele->Name()); item_it != fields_map_.end()) {
                        item_it->second(item_val, item_ele);
                    }
                }
                item_ele = item_ele->NextSiblingElement();
            }
            items_.push_back(std::move(item_val));
        }
    }

    auto ext_ele = root->FirstChildElement("ExtraInfo");
    while (ext_ele) {
        extra_.emplace_back(ext_ele->GetText());
        ext_ele = ext_ele->NextSiblingElement("ExtraInfo");
    }
    return true;
}

bool CatalogResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    new_xml_element(sum_num_, root, "SumNum");
    auto list_ele = root->InsertNewChildElement("DeviceList");
    list_ele->SetAttribute("Num", items_.size());
    for (auto &it: items_) {
        auto item_ele = list_ele->InsertNewChildElement("Item");
        new_xml_element(it.DeviceID, item_ele, "DeviceID");
        if (!it.Name.empty()) new_xml_element(it.Name, item_ele, "Name");
        if (!it.Manufacturer.empty()) new_xml_element(it.Manufacturer, item_ele, "Manufacturer");
        if (!it.Model.empty()) new_xml_element(it.Model, item_ele, "Model");
        if (!it.CivilCode.empty()) new_xml_element(it.CivilCode, item_ele, "CivilCode");
        if (!it.Block.empty()) new_xml_element(it.Block, item_ele, "Block");
        if (!it.Address.empty()) new_xml_element(it.Address, item_ele, "Address");
        if (it.Parental) new_xml_element(it.Parental.value(), item_ele, "Parental");
        if (!it.ParentID.empty()) new_xml_element(it.ParentID, item_ele, "ParentID");
        if (it.RegisterWay) new_xml_element(it.RegisterWay, item_ele, "RegisterWay");
        if (!it.SecurityLevelCode.empty()) new_xml_element(it.SecurityLevelCode, item_ele, "SecurityLevelCode");
        if (it.Secrecy)  new_xml_element(it.Secrecy, item_ele, "Secrecy");
        if (!it.IPAddress.empty()) new_xml_element(it.IPAddress, item_ele, "IPAddress");
        if (it.Port) new_xml_element(it.Port.value(), item_ele, "Port");
        if (!it.Password.empty()) new_xml_element(it.Password, item_ele, "Password");
        if (it.Status && it.Status.value() != StatusType::invalid) new_xml_element(it.Status.value(), item_ele, "Status");
        if (it.Longitude) new_xml_element(it.Longitude, item_ele, "Longitude");
        if (it.Latitude) new_xml_element(it.Latitude, item_ele, "Latitude");
        if (!it.BusinessGroupID.empty()) new_xml_element(it.BusinessGroupID, item_ele, "BusinessGroupID");
        if (it.Event) new_xml_element(it.Event.value(), item_ele, "Event");
        if (it.Info) {
            auto info_ele = item_ele->InsertNewChildElement("Info");
            if (!it.Info->PTZType.empty()) new_xml_element(it.Info->PTZType, info_ele, "PTZType");
            if (!it.Info->PhotoelectricImagingType.empty()) new_xml_element(it.Info->PhotoelectricImagingType, info_ele, "PhotoelectricImagingType");
            if (!it.Info->CapturePositionType.empty()) new_xml_element(it.Info->CapturePositionType, info_ele, "CapturePositionType");
            new_xml_element(it.Info->RoomType, info_ele, "RoomType");
            new_xml_element(it.Info->SupplyLightType, info_ele, "SupplyLightType");
            if (it.Info->DirectionType) new_xml_element(it.Info->DirectionType, info_ele, "Direction");
            if (!it.Info->Resolution.empty()) new_xml_element(it.Info->Resolution, info_ele, "Resolution");
            if (!it.Info->StreamNumberList.empty()) new_xml_element(it.Info->StreamNumberList, info_ele, "StreamNumberList");
            if (!it.Info->DownloadSpeed.empty()) new_xml_element(it.Info->DownloadSpeed, info_ele, "DownloadSpeed");
            new_xml_element(it.Info->SVCSpaceSupportMode, info_ele, "SVCSpaceSupportMode");
            new_xml_element(it.Info->SVCTimeSupportMode, info_ele, "SVCTimeSupportMode");
            if (!it.Info->SSVCRatioSupportList.empty()) new_xml_element(it.Info->SSVCRatioSupportList, info_ele, "SSVCRatioSupportList");
            if (it.Info->MobileDeviceType) new_xml_element(it.Info->MobileDeviceType, info_ele, "MobileDeviceType");
            if (it.Info->HorizontalFieldAngle) new_xml_element(it.Info->HorizontalFieldAngle, info_ele, "HorizontalFieldAngle");
            if (it.Info->VerticalFieldAngle) new_xml_element(it.Info->VerticalFieldAngle, info_ele, "VerticalFieldAngle");
            if (it.Info->MaxViewDistance) new_xml_element(it.Info->MaxViewDistance, info_ele, "MaxViewDistance");
            if (!it.Info->GrassrootsCode.empty()) new_xml_element(it.Info->GrassrootsCode, info_ele, "GrassrootsCode");
            if (it.Info->PointType) new_xml_element(it.Info->PointType, info_ele, "PointType");
            if (!it.Info->PointCommonName.empty()) new_xml_element(it.Info->PointCommonName, info_ele, "PointCommonName");
            if (!it.Info->MAC.empty()) new_xml_element(it.Info->MAC, info_ele, "MAC");
            if (!it.Info->FunctionType.empty()) new_xml_element(it.Info->FunctionType, info_ele, "FunctionType");
            if (!it.Info->EncodeType.empty()) new_xml_element(it.Info->EncodeType, info_ele, "EncodeType");
            if (!it.Info->InstallTime.empty()) new_xml_element(it.Info->InstallTime, info_ele, "InstallTime");
            if (!it.Info->ManagementUnit.empty()) new_xml_element(it.Info->ManagementUnit, info_ele, "ManagementUnit");
            if (!it.Info->ContactInfo.empty()) new_xml_element(it.Info->ContactInfo, info_ele, "ContactInfo");
            if (it.Info->RecordSaveDays) new_xml_element(it.Info->RecordSaveDays, info_ele, "RecordSaveDays");
            if (!it.Info->IndustrialClassification.empty()) new_xml_element(it.Info->IndustrialClassification, info_ele, "IndustrialClassification");
        }
    }
    for(auto &it : extra_) {
        auto ext_ele = root->InsertNewChildElement("ExtraInfo");
        ext_ele->SetText(it.c_str());

    }
    return true;
}

 CatalogNotifyMessage::CatalogNotifyMessage(
    const std::string &device_id, int sum_num, std::vector<ItemTypeInfo> &&items, std::vector<std::string> &&extra) : CatalogResponseMessage(device_id, sum_num,std::move(items), std::move(extra)) {
    root_ = MessageRootType::Notify;
}


/**********************************************************************************************************
文件名称:   catalog_message.cpp
创建时间:   25-2-10 下午3:14
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:14

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:14       描述:   创建文件

**********************************************************************************************************/