#include "gb28181/message/config_download_messsage.h"

#include <gb28181/type_define_ext.h>

using namespace gb28181;

ConfigDownloadRequestMessage::ConfigDownloadRequestMessage(const std::string &device_id, DeviceConfigType config_type)
    : MessageBase()
    , config_type_(config_type) {
    root_ = MessageRootType::Query;
    cmd_ = MessageCmdType::ConfigDownload;
    device_id_ = device_id;
}

bool ConfigDownloadRequestMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (config_type_ == DeviceConfigType::invalid) {
        error_message_ = "The ConfigType field invalid";
        return false;
    }
    new_xml_element(config_type_, root, "ConfigType");
    return true;
}

bool ConfigDownloadRequestMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(config_type_, root, "ConfigType")) {
        error_message_ = "The ConfigType field invalid";
        return false;
    }
    return true;
}

ConfigDownloadResponseMessage::ConfigDownloadResponseMessage(
    const std::string &device_id, ResultType result_type,
    std::unordered_map<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> config)
    : MessageBase()
    , result_(result_type)
    , configs_(std::move(config)) {
    device_id_ = device_id;
    root_ = MessageRootType::Response;
    cmd_ = MessageCmdType::ConfigDownload;
}
DeviceConfigType ConfigDownloadResponseMessage::get_config_type() const {
    DeviceConfigType config_type = DeviceConfigType::invalid;
    for (auto &it : configs_) {
        config_type |= it.first;
    }
    return config_type;
}


bool ConfigDownloadResponseMessage::load_detail() {
    auto root = xml_ptr_->RootElement();
    if (!from_xml_element(result_, root, "Result")) {
        error_message_ = "The Result field invalid";
        return false;
    }
    bool has_ele = false;
    if (BasicParamCfgType item; from_xml_element(item, root, "BasicParam", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::BasicParam,
            has_ele ? std::make_shared<BasicParamCfgType>(std::move(item)) : std::shared_ptr<BasicParamCfgType>());
    }
    if (VideoParamOptCfgType item; from_xml_element(item, root, "VideoParamOpt", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::VideoParamOpt,
            has_ele ? std::make_shared<VideoParamOptCfgType>(std::move(item))
                    : std::shared_ptr<VideoParamOptCfgType>());
    }
    if (SVACEncodeCfgType item; from_xml_element(item, root, "SVACEncodeConfig", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::SVACEncodeConfig,
            has_ele ? std::make_shared<SVACEncodeCfgType>(std::move(item)) : std::shared_ptr<SVACEncodeCfgType>());
    }
    if (SVACDecodeCfgType item; from_xml_element(item, root, "SVACDecodeConfig", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::SVACDecodeConfig,
            has_ele ? std::make_shared<SVACDecodeCfgType>(std::move(item)) : std::shared_ptr<SVACDecodeCfgType>());
    }
    if (VideoParamAttributeCfgType item; from_xml_element(item, root, "VideoParamAttribute", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::VideoParamAttribute,
            has_ele ? std::make_shared<VideoParamAttributeCfgType>(std::move(item))
                    : std::shared_ptr<VideoParamAttributeCfgType>());
    }
    if (VideoRecordPlanCfgType item; from_xml_element(item, root, "VideoRecordPlan", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::VideoRecordPlan,
            has_ele ? std::make_shared<VideoRecordPlanCfgType>(std::move(item))
                    : std::shared_ptr<VideoRecordPlanCfgType>());
    }
    if (VideoAlarmRecordCfgType item; from_xml_element(item, root, "VideoAlarmRecord", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::VideoAlarmRecord,
            has_ele ? std::make_shared<VideoAlarmRecordCfgType>(std::move(item))
                    : std::shared_ptr<VideoAlarmRecordCfgType>());
    }
    if (PictureMaskClgType item; from_xml_element(item, root, "PictureMask", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::PictureMask,
            has_ele ? std::make_shared<PictureMaskClgType>(std::move(item)) : std::shared_ptr<PictureMaskClgType>());
    }
    if (FrameMirrorCfgType item; from_xml_element(item, root, "FrameMirror", has_ele) || has_ele) {
        configs_.emplace(DeviceConfigType::FrameMirror, std::make_shared<FrameMirrorCfgType>(std::move(item)));
    }
    if (AlarmReportCfgType item; from_xml_element(item, root, "AlarmReport", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::AlarmReport,
            has_ele ? std::make_shared<AlarmReportCfgType>(std::move(item)) : std::shared_ptr<AlarmReportCfgType>());
    }
    if (OSDCfgType item; from_xml_element(item, root, "OSDConfig", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::OSDConfig,
            has_ele ? std::make_shared<OSDCfgType>(std::move(item)) : std::shared_ptr<OSDCfgType>());
    }
    if (SnapShotCfgType item; from_xml_element(item, root, "SnapShotConfig", has_ele) || has_ele) {
        configs_.emplace(
            DeviceConfigType::SnapShotConfig,
            has_ele ? std::make_shared<SnapShotCfgType>(std::move(item)) : std::shared_ptr<SnapShotCfgType>());
    }
    return true;
}

bool ConfigDownloadResponseMessage::parse_detail() {
    auto root = xml_ptr_->RootElement();
    if (result_ == ResultType::invalid) {
        error_message_ = "The Result field invalid";
        return false;
    }
    if (configs_.empty() && result_ != ResultType::Error) {
        error_message_ = "The Config is empty";
        return false;
    }
    new_xml_element(result_, root, "Result");
    for (auto &it : configs_) {
        switch (it.first) {
            case DeviceConfigType::BasicParam:
                new_xml_element(std::dynamic_pointer_cast<BasicParamCfgType>(it.second).get(), root, "BasicParam");
                break;
            case DeviceConfigType::VideoParamOpt:
                new_xml_element(
                    std::dynamic_pointer_cast<VideoParamOptCfgType>(it.second).get(), root, "VideoParamOpt");
                break;
            case DeviceConfigType::SVACEncodeConfig:
                new_xml_element(
                    std::dynamic_pointer_cast<SVACEncodeCfgType>(it.second).get(), root, "SVACEncodeConfig");
                break;
            case DeviceConfigType::SVACDecodeConfig:
                new_xml_element(
                    std::dynamic_pointer_cast<SVACDecodeCfgType>(it.second).get(), root, "SVACDecodeConfig");
                break;
            case DeviceConfigType::VideoParamAttribute:
                new_xml_element(
                    std::dynamic_pointer_cast<VideoParamAttributeCfgType>(it.second).get(), root,
                    "VideoParamAttribute");
                break;
            case DeviceConfigType::VideoRecordPlan:
                new_xml_element(
                    std::dynamic_pointer_cast<VideoRecordPlanCfgType>(it.second).get(), root, "VideoRecordPlan");
                break;
            case DeviceConfigType::VideoAlarmRecord:
                new_xml_element(
                    std::dynamic_pointer_cast<VideoAlarmRecordCfgType>(it.second).get(), root, "VideoAlarmRecord");
                break;
            case DeviceConfigType::PictureMask:
                new_xml_element(std::dynamic_pointer_cast<PictureMaskClgType>(it.second).get(), root, "PictureMask");
                break;
            case DeviceConfigType::FrameMirror:
                new_xml_element(std::dynamic_pointer_cast<FrameMirrorCfgType>(it.second).get(), root, "FrameMirror");
                break;
            case DeviceConfigType::AlarmReport:
                new_xml_element(std::dynamic_pointer_cast<AlarmReportCfgType>(it.second).get(), root, "AlarmReport");
                break;
            case DeviceConfigType::OSDConfig:
                new_xml_element(std::dynamic_pointer_cast<OSDCfgType>(it.second).get(), root, "OSDConfig");
                break;
            case DeviceConfigType::SnapShotConfig:
                new_xml_element(std::dynamic_pointer_cast<SnapShotCfgType>(it.second).get(), root, "SnapShotConfig");
                break;
            default: break;
        }
    }
    return true;
}

/**********************************************************************************************************
文件名称:   config_download_messsage.cpp
创建时间:   25-2-10 下午3:13
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:13

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:13       描述:   创建文件

**********************************************************************************************************/