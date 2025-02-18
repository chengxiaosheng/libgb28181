#ifndef gb28181_DEVICE_CONTROL_MESSAGE_H
#define gb28181_DEVICE_CONTROL_MESSAGE_H
#include <gb28181/message/message_base.h>
namespace gb28181 {

class DeviceControlRequestMessage : public virtual MessageBase {
public:
    explicit DeviceControlRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceControlRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage(const std::string &device_id, std::vector<std::string> &&extra = {});

    std::vector<std::string> &extra_info() { return extra_info_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::vector<std::string> extra_info_;
};

class DeviceControlRequestMessage_PTZCmd final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_PTZCmd(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_PTZCmd(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_PTZCmd(
        const std::string &device_id, PtzCmdType &&ptz_cmd, std::optional<gb28181::PtzCmdParams> &&params,
        std::vector<std::string> &&extra = {});

    PtzCmdType &ptz_cmd() { return ptz_cmd_type_; }

    std::optional<PtzCmdParams> &ptz_cmd_params() { return ptz_cmd_params_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    gb28181::PtzCmdType ptz_cmd_type_;
    std::optional<gb28181::PtzCmdParams> ptz_cmd_params_;
};

class DeviceControlRequestMessage_TeleBoot : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_TeleBoot(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_TeleBoot(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_TeleBoot(const std::string &device_id, std::vector<std::string> &&extra = {});

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string tele_boot_ { "Boot" };
};

class DeviceControlRequestMessage_RecordCmd final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_RecordCmd(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_RecordCmd(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_RecordCmd(
        const std::string &device_id, RecordType type, int8_t stream_number = 0, std::vector<std::string> &&extra = {});

    gb28181::RecordType &record_type() { return record_type_; }
    int8_t &stream_numbre() { return stream_number_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    gb28181::RecordType record_type_ { RecordType::invalid };
    int8_t stream_number_ { 0 };
};

class DeviceControlRequestMessage_GuardCmd final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_GuardCmd(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_GuardCmd(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_GuardCmd(
        const std::string &device_id, GuardType type, std::vector<std::string> &&extra = {});
    GuardType &guard_type() { return guard_type_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    gb28181::GuardType guard_type_ { GuardType::invalid };
};

class DeviceControlRequestMessage_AlarmCmd final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_AlarmCmd(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_AlarmCmd(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_AlarmCmd(
        const std::string &device_id, std::optional<AlarmCmdInfoType> &&info = {},
        std::vector<std::string> &&extra = {});
    std::optional<AlarmCmdInfoType> &info() { return info_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string alarm_cmd_ { "ResetAlarm" };
    std::optional<AlarmCmdInfoType> info_;
};

class DeviceControlRequestMessage_IFrameCmd final : public DeviceControlRequestMessage {
    explicit DeviceControlRequestMessage_IFrameCmd(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_IFrameCmd(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_IFrameCmd(const std::string &device_id, std::vector<std::string> &&extra = {});

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string iframe_cmd_ { "Send" };
};

class DeviceControlRequestMessage_DragZoomIn : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_DragZoomIn(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_DragZoomIn(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_DragZoomIn(
        const std::string &device_id, DragZoomType &&drag_zoom, std::vector<std::string> &&extra = {});

    DragZoomType &drag_zoom() { return drag_zoom_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

    DragZoomType drag_zoom_ {};
};

class DeviceControlRequestMessage_DragZoomOut final : public DeviceControlRequestMessage_DragZoomIn {
public:
    explicit DeviceControlRequestMessage_DragZoomOut(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage_DragZoomIn(xml) {}
    explicit DeviceControlRequestMessage_DragZoomOut(MessageBase &&messageBase)
        : DeviceControlRequestMessage_DragZoomIn(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_DragZoomOut(
        const std::string &device_id, DragZoomType &&drag_zoom, std::vector<std::string> &&extra = {});

protected:
    bool load_detail() override;
    bool parse_detail() override;
};

class DeviceControlRequestMessage_HomePosition final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_HomePosition(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_HomePosition(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_HomePosition(
        const std::string &device_id, int8_t &&enabled, std::optional<uint8_t> &&preset_index = {},
        std::optional<int32_t> &&reset_time = {}, std::vector<std::string> &&extra = {});

    int8_t &enabled() { return enabled_; }
    std::optional<uint8_t> &preset_index() { return preset_index_; }
    std::optional<int32_t> &reset_time() { return reset_time_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    int8_t enabled_ { 0 };
    std::optional<uint8_t> preset_index_ {};
    std::optional<int32_t> reset_time_ {};
};

class DeviceControlRequestMessage_PtzPreciseCtrl final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_PtzPreciseCtrl(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_PtzPreciseCtrl(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_PtzPreciseCtrl(
        const std::string &device_id, PTZPreciseCtrlType &&ptz_precise_ctrl, std::vector<std::string> &&extra = {});

    PTZPreciseCtrlType &ptz_precise_ctrl() { return ptz_precise_ctrl_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    PTZPreciseCtrlType ptz_precise_ctrl_;
};

class DeviceControlRequestMessage_DeviceUpgrade final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_DeviceUpgrade(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_DeviceUpgrade(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_DeviceUpgrade(
        const std::string &device_id, DeviceUpgradeType &&device_upgrade, std::vector<std::string> &&extra = {});

    DeviceUpgradeType &device_upgrade() { return device_upgrade_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    DeviceUpgradeType device_upgrade_ {};
};

class DeviceControlRequestMessage_FormatSDCard final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_FormatSDCard(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_FormatSDCard(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_FormatSDCard(
        const std::string &device_id, uint8_t &&index, std::vector<std::string> &&extra = {});
    uint8_t &index() { return index_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    uint8_t index_ { 0 };
};

class DeviceControlRequestMessage_TargetTrack final : public DeviceControlRequestMessage {
public:
    explicit DeviceControlRequestMessage_TargetTrack(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : DeviceControlRequestMessage(xml) {}
    explicit DeviceControlRequestMessage_TargetTrack(MessageBase &&messageBase)
        : DeviceControlRequestMessage(std::move(messageBase)) {}
    explicit DeviceControlRequestMessage_TargetTrack(
        const std::string &device_id, TargetTraceType target_trace, std::string device_id2,
        std::optional<DragZoomType> &&target_area, std::vector<std::string> &&extra = {});
    TargetTraceType &target_trace() { return target_trace_; }
    std::string &device_id2() { return device_id2_; }
    std::optional<DragZoomType> &target_area() { return target_area_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    TargetTraceType target_trace_ { TargetTraceType::invalid };
    std::string device_id2_ {};
    std::optional<DragZoomType> target_area_ {};
};

class DeviceControlResponseMessage final : public MessageBase {
public:
    explicit DeviceControlResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceControlResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceControlResponseMessage(
        const std::string &device_id, ResultType result = ResultType::OK, const std::string &reason = "");

    ResultType &result() { return result_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
};

} // namespace gb28181

#endif // gb28181_DEVICE_CONTROL_MESSAGE_H

/**********************************************************************************************************
文件名称:   device_control_message.h
创建时间:   25-2-14 下午12:44
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午12:44

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午12:44       描述:   创建文件

**********************************************************************************************************/