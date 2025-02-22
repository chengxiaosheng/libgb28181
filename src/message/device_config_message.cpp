#include <gb28181/message/device_config_message.h>
#include <gb28181/type_define_ext.h>

using namespace gb28181;

DeviceConfigRequestMessage::DeviceConfigRequestMessage(
    const std::string &device_id, std::pair<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> config)
    : MessageBase()
    , config_(std::move(config)) {
    device_id_ = device_id;
    root_ = MessageRootType::Control;
    cmd_ = MessageCmdType::DeviceConfig;
}
bool DeviceConfigRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    bool has = false;
    for (auto ele = root->FirstChildElement(); ele; ele = ele->NextSiblingElement()) {
        if (ele->Name() == "BasicParam") {
            BasicParamCfgType config;
            from_xml_element(config, root, "BasicParam", has);
            config_
                = std::make_pair(DeviceConfigType::BasicParam, std::make_shared<BasicParamCfgType>(std::move(config)));
        } else if (ele->Name() == "VideoParamOpt") {
            VideoParamOptCfgType config;
            from_xml_element(config, root, "VideoParamOpt", has);
            config_ = std::make_pair(
                DeviceConfigType::VideoParamOpt, std::make_shared<VideoParamOptCfgType>(std::move(config)));
        } else if (ele->Name() == "SVACEncodeConfig") {
            SVACEncodeCfgType config;
            from_xml_element(config, root, "SVACEncodeConfig", has);
            config_ = std::make_pair(
                DeviceConfigType::SVACEncodeConfig, std::make_shared<SVACEncodeCfgType>(std::move(config)));
        } else if (ele->Name() == "SVACDecodeConfig") {
            SVACDecodeCfgType config;
            from_xml_element(config, root, "SVACDecodeConfig", has);
            config_ = std::make_pair(
                DeviceConfigType::SVACDecodeConfig, std::make_shared<SVACDecodeCfgType>(std::move(config)));
        } else if (ele->Name() == "VideoParamAttribute") {
            VideoParamAttributeCfgType config;
            from_xml_element(config, root, "VideoParamAttribute", has);
            config_ = std::make_pair(
                DeviceConfigType::VideoParamAttribute, std::make_shared<VideoParamAttributeCfgType>(std::move(config)));
        } else if (ele->Name() == "VideoRecordPlan") {
            VideoRecordPlanCfgType config;
            from_xml_element(config, root, "VideoRecordPlan", has);
            config_ = std::make_pair(
                DeviceConfigType::VideoRecordPlan, std::make_shared<VideoRecordPlanCfgType>(std::move(config)));
        } else if (ele->Name() == "VideoAlarmRecord") {
            VideoAlarmRecordCfgType config;
            from_xml_element(config, root, "VideoAlarmRecord", has);
            config_ = std::make_pair(
                DeviceConfigType::VideoAlarmRecord, std::make_shared<VideoAlarmRecordCfgType>(std::move(config)));
        } else if (ele->Name() == "PictureMask") {
            PictureMaskClgType config;
            from_xml_element(config, root, "PictureMask", has);
            config_ = std::make_pair(
                DeviceConfigType::PictureMask, std::make_shared<PictureMaskClgType>(std::move(config)));
        } else if (ele->Name() == "FrameMirror") {
            FrameMirrorCfgType config;
            from_xml_element(config, root, "FrameMirror", has);
            config_ = std::make_pair(
                DeviceConfigType::FrameMirror, std::make_shared<FrameMirrorCfgType>(std::move(config)));
        } else if (ele->Name() == "AlarmReport") {
            AlarmReportCfgType config;
            from_xml_element(config, root, "AlarmReport", has);
            config_ = std::make_pair(
                DeviceConfigType::AlarmReport, std::make_shared<AlarmReportCfgType>(std::move(config)));
        } else if (ele->Name() == "OSDConfig") {
            OSDCfgType config;
            from_xml_element(config, root, "OSDConfig", has);
            config_ = std::make_pair(DeviceConfigType::OSDConfig, std::make_shared<OSDCfgType>(std::move(config)));
        } else if (ele->Name() == "SnapShotConfig") {
            SnapShotCfgType config;
            from_xml_element(config, root, "SnapShotConfig", has);
            config_ = std::make_pair(
                DeviceConfigType::SnapShotConfig, std::make_shared<SnapShotCfgType>(std::move(config)));
        }
        if (config_.first != DeviceConfigType::invalid)
            break;
    }
    if (config_.first == DeviceConfigType::invalid) {
        error_message_ = "Invalid device config";
        return false;
    }
    return true;
}
bool DeviceConfigRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (config_.first == DeviceConfigType::invalid) {
        error_message_ = "invalid config";
        return false;
    }
    switch (config_.first) {
        case DeviceConfigType::BasicParam:
            new_xml_element(std::dynamic_pointer_cast<BasicParamCfgType>(config_.second).get(), root, "BasicParam");
            break;
        case DeviceConfigType::VideoParamOpt:
            new_xml_element(
                std::dynamic_pointer_cast<VideoParamOptCfgType>(config_.second).get(), root, "VideoParamOpt");
            break;
        case DeviceConfigType::SVACEncodeConfig:
            new_xml_element(
                std::dynamic_pointer_cast<SVACEncodeCfgType>(config_.second).get(), root, "SVACEncodeConfig");
            break;
        case DeviceConfigType::SVACDecodeConfig:
            new_xml_element(
                std::dynamic_pointer_cast<SVACDecodeCfgType>(config_.second).get(), root, "SVACDecodeConfig");
            break;
        case DeviceConfigType::VideoParamAttribute:
            new_xml_element(
                std::dynamic_pointer_cast<VideoParamAttributeCfgType>(config_.second).get(), root,
                "VideoParamAttribute");
            break;
        case DeviceConfigType::VideoRecordPlan:
            new_xml_element(
                std::dynamic_pointer_cast<VideoRecordPlanCfgType>(config_.second).get(), root, "VideoRecordPlan");
            break;
        case DeviceConfigType::VideoAlarmRecord:
            new_xml_element(
                std::dynamic_pointer_cast<VideoAlarmRecordCfgType>(config_.second).get(), root, "VideoAlarmRecord");
            break;
        case DeviceConfigType::PictureMask:
            new_xml_element(std::dynamic_pointer_cast<PictureMaskClgType>(config_.second).get(), root, "PictureMask");
            break;
        case DeviceConfigType::FrameMirror:
            new_xml_element(std::dynamic_pointer_cast<FrameMirrorCfgType>(config_.second).get(), root, "FrameMirror");
            break;
        case DeviceConfigType::AlarmReport:
            new_xml_element(std::dynamic_pointer_cast<AlarmReportCfgType>(config_.second).get(), root, "AlarmReport");
            break;
        case DeviceConfigType::OSDConfig:
            new_xml_element(std::dynamic_pointer_cast<OSDCfgType>(config_.second).get(), root, "OSDConfig");
            break;
        case DeviceConfigType::SnapShotConfig:
            new_xml_element(std::dynamic_pointer_cast<SnapShotCfgType>(config_.second).get(), root, "SnapShotConfig");
            break;
        default: break;
    }
    return true;
}

DeviceConfigResponseMessage::DeviceConfigResponseMessage(
    const std::string &device_id, ResultType result, std::string reason)
    : MessageBase()
    , result_(result) {
    device_id_ = device_id;
    reason_ = reason;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::DeviceConfig;
}

bool DeviceConfigResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "invalid result";
        return false;
    }
    return true;
}

bool DeviceConfigResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (root == nullptr) {
        error_message_ = "root element is null";
        return false;
    }
    if (result_ == ResultType::invalid) {
        error_message_ = "invalid result";
        return false;
    }
    new_xml_element(result_, root, "Result");
    return true;
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