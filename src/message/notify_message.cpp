#include <Util/util.h>
#include <gb28181/message/notify_message.h>
#include <gb28181/type_define_ext.h>

using namespace gb28181;

UploadSnapShotFinishedNotifyMessage::UploadSnapShotFinishedNotifyMessage(
    const std::string &device_id, std::string session_id, std::vector<std::string> &&list)
    : MessageBase()
    , session_id_(std::move(session_id))
    , snap_shot_lists_(std::move(list)) {
    device_id_ = device_id;
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::UploadSnapShotFinished;
}

bool UploadSnapShotFinishedNotifyMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(session_id_, root, "SessionID")) {
        error_message_ = "SessionID not found";
        return false;
    }
    if (auto list = root->FirstChildElement("SnapShotList")) {
        auto item = list->FirstChildElement("SnapShotFileID");
        while (item) {
            snap_shot_lists_.emplace_back(item->GetText());
            item = item->NextSiblingElement("SnapShotFileID");
        }
        return true;
    }
    error_message_ = "SnapShotList not found";
    return false;
}
bool UploadSnapShotFinishedNotifyMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (session_id_.empty()) {
        error_message_ = "SessionID not found";
        return false;
    }
    new_xml_element(session_id_, root, "SessionID");
    auto list = root->InsertNewChildElement("SnapShotList");
    for (auto &it : snap_shot_lists_) {
        auto item = list->FirstChildElement("SnapShotFileID");
        item->SetText(it.c_str());
    }
    return true;
}

MobilePositionNotifyMessage::MobilePositionNotifyMessage(
    const std::string &device_id, const std::string &time, int32_t sum_num, std::vector<ItemMobilePositionType> &&list)
    : MessageBase()
    , time_(time)
    , sum_num_(sum_num)
    , device_list_(std::move(list)) {
    device_id_ = device_id;
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::MobilePosition;
}
bool MobilePositionNotifyMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    from_xml_element(time_, root, "Time");
    from_xml_element(sum_num_, root, "SumNum");
    if (auto list = root->FirstChildElement("DeviceList")) {
        auto item = list->FirstChildElement("Item");
        while (item) {
            ItemMobilePositionType item_val;
            from_xml_element(item_val.DeviceID, item, "DeviceID");
            from_xml_element(item_val.CaptureTime, item, "CaptureTime");
            from_xml_element(item_val.Longitude, item, "Longitude");
            from_xml_element(item_val.Latitude, item, "Latitude");
            from_xml_element(item_val.Speed, item, "Speed");
            from_xml_element(item_val.Direction, item, "Direction");
            from_xml_element(item_val.Altitude, item, "Altitude");
            from_xml_element(item_val.Height, item, "Height");
            device_list_.emplace_back(std::move(item_val));
            item = item->NextSiblingElement("Item");
        }
    }
    return true;
}
bool MobilePositionNotifyMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (time_.empty()) {
        time_ = toolkit::getTimeStr("%Y-%m-%dT%H:%M:%S");
    }
    new_xml_element(time_, root, "Time");
    new_xml_element(sum_num_, root, "SumNum");
    auto list = root->InsertNewChildElement("DeviceList");
    for (const auto &item_val : device_list_) {
        auto item = list->InsertNewChildElement("Item");
        new_xml_element(item_val.DeviceID, item, "DeviceID");
        new_xml_element(item_val.CaptureTime, item, "CaptureTime");
        new_xml_element(item_val.Longitude, item, "Longitude");
        new_xml_element(item_val.Latitude, item, "Latitude");
        if (item_val.Speed)
            new_xml_element(item_val.Speed, item, "Speed");
        if (item_val.Direction)
            new_xml_element(item_val.Direction, item, "Direction");
        if (item_val.Altitude)
            new_xml_element(item_val.Altitude, item, "Altitude");
        if (item_val.Height)
            new_xml_element(item_val.Height, item, "Height");
    }
    return true;
}

MediaStatusNotifyMessage::MediaStatusNotifyMessage(const std::string &device_id, std::string notify_type)
    : MessageBase()
    , notify_type_(std::move(notify_type)) {
    device_id_ = device_id;
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::MediaStatus;
}
bool MediaStatusNotifyMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(notify_type_, root, "NotifyType")) {
        error_message_ = "NotifyType not found";
        return false;
    }
    return true;
}

bool MediaStatusNotifyMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (notify_type_.empty()) {
        error_message_ = "NotifyType not found";
        return false;
    }
    new_xml_element(notify_type_, root, "NotifyType");
    return true;
}

VideoUploadNotifyMessage::VideoUploadNotifyMessage(
    const std::string &device_id, const std::string &time, const std::optional<double> &longitude,
    const std::optional<double> &latitude)
    : MessageBase()
    , time_(time)
    , longitude_(longitude)
    , latitude_(latitude) {
    device_id_ = device_id;
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::VideoUploadNotify;
}

bool VideoUploadNotifyMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(time_, root, "Time")) {
        error_message_ = "Time not found";
        return false;
    }
    from_xml_element(longitude_, root, "Longitude");
    from_xml_element(latitude_, root, "Latitude");
    return true;
}
bool VideoUploadNotifyMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (time_.empty()) {
        time_ = toolkit::getTimeStr("%Y-%m-%dT%H:%M:%S");
    }
    new_xml_element(time_, root, "Time");
    new_xml_element(longitude_, root, "Longitude");
    new_xml_element(latitude_, root, "Latitude");
    return true;
}

DeviceUpgradeResultNotifyMessage::DeviceUpgradeResultNotifyMessage(
    const std::string &device_id, std::string session_id, std::string firmware, ResultType result, std::string reason)
    : MessageBase()
    , result_(result)
    , session_id_(std::move(session_id))
    , firmware_(std::move(firmware)) {
    device_id_ = device_id;
    reason_ = std::move(reason);
    root_ = MessageRootType::Notify;
    cmd_ = MessageCmdType::DeviceUpgradeResult;
}

bool DeviceUpgradeResultNotifyMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(session_id_, root, "SessionID")) {
        error_message_ = "SessionID not found";
        return false;
    }
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "Result not found";
        return false;
    }
    from_xml_element(firmware_, root, "Firmware");
    from_xml_element(reason_, root, "UpgradeFailedReason");
    return true;
}

bool DeviceUpgradeResultNotifyMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (session_id_.empty()) {
        error_message_ = "SessionID not found";
        return false;
    }
    if (result_ == ResultType::invalid) {
        error_message_ = "Result not found";
        return false;
    }
    new_xml_element(session_id_, root, "SessionID");
    new_xml_element(result_, root, "Result");
    new_xml_element(firmware_, root, "Firmware");
    new_xml_element(reason_, root, "UpgradeFailedReason");
    return true;
}

/**********************************************************************************************************
文件名称:   notify_message.cpp
创建时间:   25-2-14 下午7:13
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午7:13

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午7:13       描述:   创建文件

**********************************************************************************************************/