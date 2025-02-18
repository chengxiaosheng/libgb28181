#include <gb28181/message/device_control_message.h>
#include <gb28181/type_define_ext.h>

#include <utility>
using namespace gb28181;

DeviceControlRequestMessage::DeviceControlRequestMessage(const std::string &device_id, std::vector<std::string> &&extra)
    : MessageBase()
    , extra_info_(std::move(extra)) {
    device_id_ = device_id;
    root_ = MessageRootType::Control;
    cmd_ = MessageCmdType::DeviceControl;
}

bool DeviceControlRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();

    auto ele = root->FirstChildElement("ExtraInfo");
    while (ele) {
        extra_info_.emplace_back(ele->GetText());
        ele = ele->NextSiblingElement("ExtraInfo");
    }
    return true;
}

bool DeviceControlRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    for (auto &it : extra_info_) {
        if (it.empty())
            continue;
        auto ele = root->InsertNewChildElement("ExtraInfo");
        ele->SetText(it.c_str());
    }
    return true;
}

DeviceControlRequestMessage_PTZCmd::DeviceControlRequestMessage_PTZCmd(
    const std::string &device_id, PtzCmdType &&ptz_cmd, std::optional<gb28181::PtzCmdParams> &&params,
    std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , ptz_cmd_type_(std::move(ptz_cmd))
    , ptz_cmd_params_(std::move(params)) {}

bool DeviceControlRequestMessage_PTZCmd::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("PTZCmd")) {
        ptz_cmd_type_ = PtzCmdType(ele->GetText());
    } else {
        error_message_ = "PTZCmdType not found";
        return false;
    }
    if (auto ele = root->FirstChildElement("PTZCmdParams"); ele && !ele->NoChildren()) {
        ptz_cmd_params_ = PtzCmdParams();
        from_xml_element(ptz_cmd_params_->PresetName, ele, "PresetName");
        from_xml_element(ptz_cmd_params_->CruiseTrackName, ele, "CruiseTrackName");
    }
    return true;
}
bool DeviceControlRequestMessage_PTZCmd::parse_detail() {
    DeviceControlRequestMessage::parse_detail();

    auto root = xml_ptr_->RootElement();
    auto cmd_ele = root->FirstChildElement("PTZCmd");
    cmd_ele->SetText(ptz_cmd_type_.str().c_str());
    if (ptz_cmd_params_) {
        auto p_ele = root->FirstChildElement("PTZCmdParams");
        if (!ptz_cmd_params_->PresetName.empty()) {
            new_xml_element(ptz_cmd_params_->PresetName, p_ele, "PresetName");
        }
        if (!ptz_cmd_params_->CruiseTrackName.empty()) {
            new_xml_element(ptz_cmd_params_->CruiseTrackName, p_ele, "CruiseTrackName");
        }
    }
    return true;
}

DeviceControlRequestMessage_TeleBoot::DeviceControlRequestMessage_TeleBoot(
    const std::string &device_id, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra)) {}
bool DeviceControlRequestMessage_TeleBoot::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("TeleBoot"); !ele) {
        error_message_ = "TeleBoot not found";
        return false;
    }
    return true;
}
bool DeviceControlRequestMessage_TeleBoot::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(tele_boot_, root, "TeleBoot");
    return true;
}

DeviceControlRequestMessage_RecordCmd::DeviceControlRequestMessage_RecordCmd(
    const std::string &device_id, RecordType type, int8_t stream_number, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , record_type_(type)
    , stream_number_(stream_number) {}

bool DeviceControlRequestMessage_RecordCmd::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(record_type_, root, "RecordCmd")) {
        error_message_ = "RecordType not found";
        return false;
    }
    from_xml_element(stream_number_, root, "StreamNumber");
    return true;
}

bool DeviceControlRequestMessage_RecordCmd::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(record_type_, root, "RecordCmd");
    new_xml_element(stream_number_, root, "StreamNumber");
    return true;
}

DeviceControlRequestMessage_GuardCmd::DeviceControlRequestMessage_GuardCmd(
    const std::string &device_id, GuardType type, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , guard_type_(type) {}
bool DeviceControlRequestMessage_GuardCmd::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(guard_type_, root, "GuardCmd")) {
        error_message_ = "GuardType not found";
        return false;
    }
    return true;
}

bool DeviceControlRequestMessage_GuardCmd::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(guard_type_, root, "GuardCmd");
    return true;
}

DeviceControlRequestMessage_AlarmCmd::DeviceControlRequestMessage_AlarmCmd(
    const std::string &device_id, std::optional<AlarmCmdInfoType> &&info, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , info_(std::move(info)) {}
bool DeviceControlRequestMessage_AlarmCmd::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(alarm_cmd_, root, "AlarmCmd")) {
        error_message_ = "AlarmCmd not found";
        return false;
    }
    if (auto ele = root->FirstChildElement("Info")) {
        info_ = AlarmCmdInfoType();
        from_xml_element(info_->AlarmMethod, ele, "AlarmMethod");
        from_xml_element(info_->AlarmType, ele, "AlarmType");
    }
    return true;
}

bool DeviceControlRequestMessage_AlarmCmd::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(alarm_cmd_, root, "AlarmCmd");
    if (info_) {
        auto ele = root->InsertNewChildElement("Info");
        new_xml_element(info_->AlarmMethod, ele, "AlarmMethod");
        new_xml_element(info_->AlarmType, ele, "AlarmType");
    }
    return true;
}

DeviceControlRequestMessage_IFrameCmd::DeviceControlRequestMessage_IFrameCmd(
    const std::string &device_id, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra)) {}
bool DeviceControlRequestMessage_IFrameCmd::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (!(root->FirstChildElement("IFrameCmd") || root->FirstChildElement("IFameCmd"))) {
        error_message_ = "IFrameCmd not found";
        return false;
    }
    return true;
}
bool DeviceControlRequestMessage_IFrameCmd::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(iframe_cmd_, root, "IFrameCmd"); // GB/T 28181-2022 为 IFrameCmd
    new_xml_element(iframe_cmd_, root, "IFameCmd"); // GB/T 28181-2022以下为 IFameCmd
    return true;
}
DeviceControlRequestMessage_DragZoomIn::DeviceControlRequestMessage_DragZoomIn(
    const std::string &device_id, DragZoomType &&drag_zoom, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , drag_zoom_(drag_zoom) {}
bool DeviceControlRequestMessage_DragZoomIn::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("DragZoomIn")) {
        from_xml_element(drag_zoom_.Length, ele, "Length");
        from_xml_element(drag_zoom_.Width, ele, "Width");
        from_xml_element(drag_zoom_.MidPointX, ele, "MidPointX");
        from_xml_element(drag_zoom_.MidPointY, ele, "MidPointY");
        from_xml_element(drag_zoom_.LengthX, ele, "LengthX");
        from_xml_element(drag_zoom_.LengthY, ele, "LengthY");
        return true;
    }
    error_message_ = "DragZoomIn not found";
    return false;
}
bool DeviceControlRequestMessage_DragZoomIn::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    auto ele = root->InsertNewChildElement("DragZoomIn");
    new_xml_element(drag_zoom_.Length, ele, "Length");
    new_xml_element(drag_zoom_.Width, ele, "Width");
    new_xml_element(drag_zoom_.MidPointX, ele, "MidPointX");
    new_xml_element(drag_zoom_.MidPointY, ele, "MidPointY");
    new_xml_element(drag_zoom_.LengthX, ele, "LengthX");
    new_xml_element(drag_zoom_.LengthY, ele, "LengthY");
    return true;
}
DeviceControlRequestMessage_DragZoomOut::DeviceControlRequestMessage_DragZoomOut(
    const std::string &device_id, DragZoomType &&drag_zoom, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage_DragZoomIn(
          device_id, std::forward<decltype(drag_zoom)>(drag_zoom), std::forward<decltype(extra)>(extra)) {}

bool DeviceControlRequestMessage_DragZoomOut::load_detail() {
    DeviceControlRequestMessage::parse_detail(); // NOLINT(*-parent-virtual-call)
    auto root = xml_ptr_->RootElement();
    auto ele = root->InsertNewChildElement("DragZoomOut");
    new_xml_element(drag_zoom_.Length, ele, "Length");
    new_xml_element(drag_zoom_.Width, ele, "Width");
    new_xml_element(drag_zoom_.MidPointX, ele, "MidPointX");
    new_xml_element(drag_zoom_.MidPointY, ele, "MidPointY");
    new_xml_element(drag_zoom_.LengthX, ele, "LengthX");
    new_xml_element(drag_zoom_.LengthY, ele, "LengthY");
    return true;
}
bool DeviceControlRequestMessage_DragZoomOut::parse_detail() {
    DeviceControlRequestMessage::parse_detail(); // NOLINT(*-parent-virtual-call)
    auto root = xml_ptr_->RootElement();
    auto ele = root->InsertNewChildElement("DragZoomOut");
    new_xml_element(drag_zoom_.Length, ele, "Length");
    new_xml_element(drag_zoom_.Width, ele, "Width");
    new_xml_element(drag_zoom_.MidPointX, ele, "MidPointX");
    new_xml_element(drag_zoom_.MidPointY, ele, "MidPointY");
    new_xml_element(drag_zoom_.LengthX, ele, "LengthX");
    new_xml_element(drag_zoom_.LengthY, ele, "LengthY");
    return true;
}

DeviceControlRequestMessage_HomePosition::DeviceControlRequestMessage_HomePosition(
    const std::string &device_id, int8_t &&enabled, std::optional<uint8_t> &&preset_index,
    std::optional<int32_t> &&reset_time, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , enabled_(enabled)
    , preset_index_(preset_index)
    , reset_time_(reset_time) {}

bool DeviceControlRequestMessage_HomePosition::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("HomePosition"); ele) {
        if (!from_xml_element(enabled_, ele, "Enabled")) {
            error_message_ = "HomePosition->Enabled not found";
            return false;
        }
        from_xml_element(reset_time_, ele, "ResetTime");
        from_xml_element(preset_index_, ele, "PresetIndex");
        return true;
    }
    error_message_ = "HomePosition not found";
    return false;
}
bool DeviceControlRequestMessage_HomePosition::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    auto ele = root->InsertNewChildElement("HomePosition");
    new_xml_element(enabled_, ele, "Enabled");
    if (reset_time_)
        new_xml_element(reset_time_, ele, "ResetTime");
    if (preset_index_)
        new_xml_element(preset_index_, ele, "PresetIndex");
    return true;
}

DeviceControlRequestMessage_PtzPreciseCtrl::DeviceControlRequestMessage_PtzPreciseCtrl(
    const std::string &device_id, PTZPreciseCtrlType &&ptz_precise_ctrl, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , ptz_precise_ctrl_(ptz_precise_ctrl) {}

bool DeviceControlRequestMessage_PtzPreciseCtrl::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("PTZPreciseCtrl"); ele && !ele->NoChildren()) {
        from_xml_element(ptz_precise_ctrl_.Pan, ele, "Pan");
        from_xml_element(ptz_precise_ctrl_.Tilt, ele, "Tilt");
        from_xml_element(ptz_precise_ctrl_.Zoom, ele, "Zoom");
        return true;
    }
    error_message_ = "PTZPreciseCtrl not found";
    return false;
}

bool DeviceControlRequestMessage_PtzPreciseCtrl::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    auto ele = root->InsertNewChildElement("PTZPreciseCtrl");
    new_xml_element(ptz_precise_ctrl_.Pan, ele, "Pan");
    new_xml_element(ptz_precise_ctrl_.Tilt, ele, "Tilt");
    new_xml_element(ptz_precise_ctrl_.Zoom, ele, "Zoom");
    return true;
}

DeviceControlRequestMessage_DeviceUpgrade::DeviceControlRequestMessage_DeviceUpgrade(
    const std::string &device_id, DeviceUpgradeType &&device_upgrade, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , device_upgrade_(std::move(device_upgrade)) {}

bool DeviceControlRequestMessage_DeviceUpgrade::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (auto ele = root->FirstChildElement("DeviceUpgrade"); ele && !ele->NoChildren()) {
        from_xml_element(device_upgrade_.Firmware, ele, "Firmware");
        from_xml_element(device_upgrade_.FileURL, ele, "FileURL");
        from_xml_element(device_upgrade_.Manufacturer, ele, "Manufacturer");
        from_xml_element(device_upgrade_.SessionID, ele, "SessionID");
        return true;
    }
    return false;
}
bool DeviceControlRequestMessage_DeviceUpgrade::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    auto ele = root->InsertNewChildElement("DeviceUpgrade");
    new_xml_element(device_upgrade_.Firmware, ele, "Firmware");
    new_xml_element(device_upgrade_.FileURL, ele, "FileURL");
    new_xml_element(device_upgrade_.Manufacturer, ele, "Manufacturer");
    new_xml_element(device_upgrade_.SessionID, ele, "SessionID");
    return true;
}
DeviceControlRequestMessage_FormatSDCard::DeviceControlRequestMessage_FormatSDCard(
    const std::string &device_id, uint8_t &&index, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , index_(index) {}

bool DeviceControlRequestMessage_FormatSDCard::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(index_, root, "FormatSDCard")) {
        error_message_ = "FormatSDCard not found";
        return false;
    }
    return true;
}
bool DeviceControlRequestMessage_FormatSDCard::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(index_, root, "FormatSDCard");
    return true;
}

DeviceControlRequestMessage_TargetTrack::DeviceControlRequestMessage_TargetTrack(
    const std::string &device_id, TargetTraceType target_trace, std::string device_id2,
    std::optional<DragZoomType> &&target_area, std::vector<std::string> &&extra)
    : DeviceControlRequestMessage(device_id, std::move(extra))
    , target_trace_(target_trace)
    , device_id2_(std::move(device_id2))
    , target_area_(target_area) {}

bool DeviceControlRequestMessage_TargetTrack::load_detail() {
    DeviceControlRequestMessage::load_detail();
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(target_trace_, root, "TargetTrack")) {
        error_message_ = "TargetTrack trace not found";
        return false;
    }
    from_xml_element(device_id2_, root, "DeviceID2");
    if (auto ele = root->FirstChildElement("TargetArea"); ele && !ele->NoChildren()) {
        target_area_ = DragZoomType();
        from_xml_element(target_area_->Length, ele, "Length");
        from_xml_element(target_area_->Width, ele, "Width");
        from_xml_element(target_area_->MidPointX, ele, "MidPointX");
        from_xml_element(target_area_->MidPointY, ele, "MidPointY");
        from_xml_element(target_area_->LengthX, ele, "LengthX");
        from_xml_element(target_area_->LengthY, ele, "LengthY");
    }
    return true;
}
bool DeviceControlRequestMessage_TargetTrack::parse_detail() {
    DeviceControlRequestMessage::parse_detail();
    auto root = xml_ptr_->RootElement();
    new_xml_element(target_trace_, root, "TargetTrack");
    new_xml_element(device_id2_, root, "DeviceID2");
    if (target_area_) {
        auto ele = root->InsertNewChildElement("TargetArea");
        new_xml_element(target_area_->Length, ele, "Length");
        new_xml_element(target_area_->Width, ele, "Width");
        new_xml_element(target_area_->MidPointX, ele, "MidPointX");
        new_xml_element(target_area_->MidPointY, ele, "MidPointY");
        new_xml_element(target_area_->LengthX, ele, "LengthX");
        new_xml_element(target_area_->LengthY, ele, "LengthY");
    }
    return true;
}

DeviceControlResponseMessage::DeviceControlResponseMessage(
    const std::string &device_id, ResultType result, const std::string &reason)
    : MessageBase()
    , result_(result) {
    device_id_ = device_id;
    reason_ = reason;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::DeviceControl;
}

bool DeviceControlResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "Result not found";
        return false;
    }
    return true;
}
bool DeviceControlResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (result_ == ResultType::invalid) {
        error_message_ = "Result invalid";
        return false;
    }
    new_xml_element(result_, root, "Result");
    return true;
}


/**********************************************************************************************************
文件名称:   device_control_message.cpp
创建时间:   25-2-14 下午12:44
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午12:44

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午12:44       描述:   创建文件

**********************************************************************************************************/