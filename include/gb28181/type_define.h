#ifndef gb28181_include_gb28181_TYPE_DEFINE_H
#define gb28181_include_gb28181_TYPE_DEFINE_H
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

#ifndef SIP_AGENT_STR
#define SIP_AGENT_STR "GB/T 28181-2022"
#endif

namespace gb28181 {

enum class SipAuthType : uint8_t {
    none, // 不认证
    digest, // 摘要认证
    sm // 国密认证， 35114启用
};

/**
 * 字符集编码类型
 */
enum class CharEncodingType : uint8_t { invalid = 0, utf8, gb2312, gbk };

enum class TransportType : uint8_t { none = 0, udp = 1, tcp = 2, both = 3 };

enum class PlatformManufacturer : uint8_t {
    unknown = 0, // 未知
    self, // 自身
    hikvision, // 海康
    dahua, // 大华
    uniview, // 宇视
    huawei, // 华为
    gongan // 公安
};

/**
 * GB/T 28181 标准标识
 */
enum class PlatformVersionType : uint8_t {
    unknown = 0, // 未知
    v10 = 10, // GB/T 28181-2011
    v11 = 11, // GB/T 28181-2011 修改补充文件
    v20 = 20, // GB/T 28181-2016
    v30 = 30 // GB/T 28181-2022
};

/**
 * 平台状态
 */
enum class PlatformStatusType : uint8_t {
    offline = 0, // 离线
    registering = 1, // 注册中
    online = 2, // 在线
    network_error = 3, // 网络错误
    auth_failure = 4, // 认证失败
};

struct sip_account_status {
    uint64_t register_time { 0 }; // 注册时间
    uint64_t keepalive_time { 0 }; // 心跳时间
    uint64_t offline_time { 0 }; // 离线时间
    PlatformStatusType status { PlatformStatusType::offline }; // 平台状态
    std::string error; // 错误信息
};

/**
 * 账户基本信息
 */
struct sip_account {
    std::string platform_id; // 平台编码
    std::string domain; // 平台域
    std::string name; // 平台名称
    std::string host; // 平台地址
    std::string password; // 密码
    uint16_t port { 5060 }; // 平台端口
    SipAuthType auth_type { SipAuthType::none }; // 认证方式
};

struct local_account : public sip_account {
    bool allow_auto_register { false }; // 是否允许自动注册
    TransportType transport_type { TransportType::both }; // 监听的网络
};
/**
 * 上下级连平台基本账户信息
 */
struct platform_account : public sip_account {
    CharEncodingType encoding { CharEncodingType::gb2312 }; // 字符集编码
    TransportType transport_type { TransportType::udp }; // 网络传输方式
    PlatformManufacturer manufacturer { PlatformManufacturer::unknown }; // 厂商类型
    PlatformVersionType version { PlatformVersionType::unknown }; // 平台版本
    uint16_t local_port { 0 }; // 本地端口 会根据via头域的值而改变
    std::string local_host; // 本地host, 会根据via头域的值而改变
    sip_account_status plat_status; // 平台状态
};

/**
 * 下级平台账户信息
 */
struct subordinate_account : public platform_account {

};
/**
 * 上级平台账户信息
 */
struct super_account : public platform_account {
    int register_expired { 60 * 60 * 24 }; // 注册过期时间
    int keepalive_interval { 30 }; // 心跳间隔
    int keepalive_times { 3 }; // 心跳超时次数
};

/**
 * 用户扩展数据
 */
struct ExtendData {
    std::string key;
    std::string value;
    std::vector<ExtendData> children;
};

#define SIP_CONTENT_TYPE_MAP(XX)                                                                                       \
    XX(SipContentType, XML, 1, "Application/MANSCDP+xml")                                                              \
    XX(SipContentType, SDP, 1, "Application/sdp")

#define GB28181_XML_ROOT_MAP(XX)                                                                                       \
    XX(MessageRootType, Query, 1, "Query")                                                                             \
    XX(MessageRootType, Control, 2, "Control")                                                                         \
    XX(MessageRootType, Notify, 3, "Notify")                                                                           \
    XX(MessageRootType, Response, 4, "Response")

/**
 * GB/T 28181 消息根节点定义
 */
enum class MessageRootType : int8_t {
    invalid = 0, // 无效的
#define XX(type, name, value, str) name = value,
    GB28181_XML_ROOT_MAP(XX)
#undef XX
};

#define GB28181_XML_CMD_MAP(XX)                                                                                        \
    XX(MessageCmdType, DeviceControl, 1, "DeviceControl")                                                              \
    XX(MessageCmdType, DeviceConfig, 2, "DeviceConfig")                                                                \
    XX(MessageCmdType, DeviceStatus, 3, "DeviceStatus")                                                                \
    XX(MessageCmdType, Catalog, 4, "Catalog")                                                                          \
    XX(MessageCmdType, DeviceInfo, 5, "DeviceInfo")                                                                    \
    XX(MessageCmdType, RecordInfo, 6, "RecordInfo")                                                                    \
    XX(MessageCmdType, Alarm, 7, "Alarm")                                                                              \
    XX(MessageCmdType, ConfigDownload, 8, "ConfigDownload")                                                            \
    XX(MessageCmdType, PresetQuery, 9, "PresetQuery")                                                                  \
    XX(MessageCmdType, MobilePosition, 10, "MobilePosition")                                                           \
    XX(MessageCmdType, HomePositionQuery, 11, "HomePositionQuery")                                                     \
    XX(MessageCmdType, CruiseTrackListQuery, 12, "CruiseTrackListQuery")                                               \
    XX(MessageCmdType, CruiseTrackQuery, 13, "CruiseTrackQuery")                                                       \
    XX(MessageCmdType, PTZPosition, 14, "PTZPosition")                                                                 \
    XX(MessageCmdType, SDCardStatus, 15, "SDCardStatus")                                                               \
    XX(MessageCmdType, Keepalive, 16, "Keepalive")                                                                     \
    XX(MessageCmdType, MediaStatus, 17, "MediaStatus")                                                                 \
    XX(MessageCmdType, Broadcast, 18, "Broadcast")                                                                     \
    XX(MessageCmdType, UploadSnapShotFinished, 19, "UploadSnapShotFinished")                                           \
    XX(MessageCmdType, VideoUploadNotify, 20, "VideoUploadNotify")                                                     \
    XX(MessageCmdType, DeviceUpgradeResult, 21, "DeviceUpgradeResult")

enum class MessageCmdType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    GB28181_XML_CMD_MAP(XX)
#undef XX
};

#define SubscribeTypeMap(XX)                                                                                           \
    XX(SubscribeType, Catalog, 1, "Catalog")                                                                           \
    XX(SubscribeType, Alarm, 2, "Alarm")                                                                               \
    XX(SubscribeType, MobilePosition, 3, "MobilePosition")                                                             \
    XX(SubscribeType, PTZPosition, 4, "PTZPosition")

#define DeviceControlSubCommandTypeMap(XX)                                                                             \
    XX(ControlSubCommandType, PTZCmd, 1, "PTZCmd")                                                                     \
    XX(ControlSubCommandType, TeleBoot, 1, "TeleBoot")                                                                 \
    XX(ControlSubCommandType, RecordCmd, 1, "RecordCmd")                                                               \
    XX(ControlSubCommandType, GuardCmd, 1, "GuardCmd")                                                                 \
    XX(ControlSubCommandType, AlarmCmd, 1, "AlarmCmd")                                                                 \
    XX(ControlSubCommandType, IFrameCmd, 1, "IFrameCmd")                                                               \
    XX(ControlSubCommandType, DragZoomIn, 1, "DragZoomIn")                                                             \
    XX(ControlSubCommandType, DragZoomOut, 1, "DragZoomOut")                                                           \
    XX(ControlSubCommandType, HomePosition, 1, "HomePosition")                                                         \
    XX(ControlSubCommandType, PTZPreciseCtrl, 1, "PTZPreciseCtrl")                                                     \
    XX(ControlSubCommandType, DeviceUpgrade, 1, "DeviceUpgrade")                                                       \
    XX(ControlSubCommandType, FormatSDCard, 1, "FormatSDCard")                                                         \
    XX(ControlSubCommandType, TargetTrack, 1, "TargetTrack")

enum class DeviceControlSubCommandType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    GB28181_XML_CMD_MAP(XX)
#undef XX
};

/**
 * 订阅类型枚举
 */
enum class SubscribeType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    SubscribeTypeMap(XX)
#undef XX
};

/**
 * 订阅状态枚举
 */
enum class SubscribeStatusType : uint8_t {
    invalid = 0,
    Init, // 初始化
    Send, // 发送订阅请求
    Subscribed, // 已订阅
    Refresh, // 刷新中
    Trying, // 重试中
    Closed, // 已关闭
    Failed // 失败
};

#define ResultTypeMap(XX) XX(ResultType, OK, 1, "OK") XX(ResultType, Error, 2, "ERROR")
/**
 * 执行结果枚举
 */
enum class ResultType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    ResultTypeMap(XX)
#undef XX
};

#define OnlineTypeMap(XX) XX(OnlineType, ONLINE, 1, "ONLINE") XX(OnlineType, OFFLINE, 2, "OFFLINE")
/**
 * 通道设备在线状态
 */
enum class OnlineType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    OnlineTypeMap(XX)
#undef XX
};

#define DeviceConfigTypeMap(XX)                                                                                        \
    XX(DeviceConfigType, BasicParam, 1, "BasicParam")                                                                  \
    XX(DeviceConfigType, VideoParamOpt, 1 << 1, "VideoParamOpt")                                                       \
    XX(DeviceConfigType, SVACEncodeConfig, 1 << 2, "SVACEncodeConfig")                                                 \
    XX(DeviceConfigType, SVACDecodeConfig, 1 << 3, "SVACDecodeConfig")                                                 \
    XX(DeviceConfigType, VideoParamAttribute, 1 << 4, "VideoParamAttribute")                                           \
    XX(DeviceConfigType, VideoRecordPlan, 1 << 5, "VideoRecordPlan")                                                   \
    XX(DeviceConfigType, VideoAlarmRecord, 1 << 6, "VideoAlarmRecord")                                                 \
    XX(DeviceConfigType, PictureMask, 1 << 7, "PictureMask")                                                           \
    XX(DeviceConfigType, FrameMirror, 1 << 8, "FrameMirror")                                                           \
    XX(DeviceConfigType, AlarmReport, 1 << 9, "AlarmReport")                                                           \
    XX(DeviceConfigType, OSDConfig, 1 << 10, "OSDConfig")                                                              \
    XX(DeviceConfigType, SnapShotConfig, 1 << 11, "SnapShotConfig")

/**
 * 配置项枚举定义
 */
enum class DeviceConfigType : uint32_t {
    invalid = 0,
#define XX(type, name, value, str) name = (value),
    DeviceConfigTypeMap(XX)
#undef XX
};

#define FrameMirrorTypeMap(XX)                                                                                         \
    XX(FrameMirrorType, Horizontal, 1, "Horizontal")                                                                   \
    XX(FrameMirrorType, Vertical, 2, "Vertical")                                                                       \
    XX(FrameMirrorType, Centre, 3, "Centre")                                                                           \
    XX(FrameMirrorType, unknown, 4, "unknown")

/**
 * 画面翻转类型定义
 */
enum class FrameMirrorType : uint32_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    FrameMirrorTypeMap(XX)
#undef XX
};

/**
 * 点类型定义
 */
struct PointType {
    uint16_t lx { 0 }, ly { 0 }, rx { 0 }, ry { 0 };
};

#define RecordTypeMap(XX) XX(RecordType, Record, 1, "Record") XX(RecordType, StopRecord, 2, "StopRecord")
/**
 * 录制指令定义
 */
enum class RecordType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    RecordTypeMap(XX)
#undef XX
};

#define GuardTypeMap(XX) XX(GuardType, SetGuard, 1, "SetGuard") XX(GuardType, ResetGuard, 2, "ResetGuard")
/**
 * 布防指令定义
 */
enum class GuardType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    GuardTypeMap(XX)
#undef XX
};

#define StatusTypeMap(XX)                                                                                              \
    XX(StatusType, ON, 1, "ON")                                                                                        \
    XX(StatusType, OFF, 2, "OFF")

enum class StatusType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    StatusTypeMap(XX)
#undef XX
};

#define ItemEventTypeMap(XX)                                                                                           \
    XX(ItemEventType, ON, 1, "ON")                                                                                     \
    XX(ItemEventType, OFF, 2, "OFF")                                                                                   \
    XX(ItemEventType, VLOST, 3, "VLOST")                                                                               \
    XX(ItemEventType, DEFECT, 4, "DEFECT")                                                                             \
    XX(ItemEventType, ADD, 5, "ADD")                                                                                   \
    XX(ItemEventType, DEL, 6, "DEL")                                                                                   \
    XX(ItemEventType, UPDATE, 7, "UPDATE")
/**
 * 针对目录通知 事件类型的定义
 */
enum class ItemEventType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    ItemEventTypeMap(XX)
#undef XX
};

#define TargetTraceTypeMap(XX)                                                                                         \
    XX(TargetTraceType, Auto, 1, "Auto")                                                                               \
    XX(TargetTraceType, Manual, 2, "Manual")                                                                           \
    XX(TargetTraceType, Stop, 3, "Stop")
/**
 * 录像控制类型
 */
enum class TargetTraceType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    TargetTraceTypeMap(XX)
#undef XX
};

#define DutyStatusTypeMap(XX)                                                                                          \
    XX(DutyStatusType, ONDUTY, 1, "ONDUTY")                                                                            \
    XX(DutyStatusType, OFFDUTY, 2, "OFFDUTY")                                                                          \
    XX(DutyStatusType, ALARM, 3, "ALARM")
enum class DutyStatusType : uint8_t {
    invalid = 0,
#define XX(type, name, value, str) name = value,
    DutyStatusTypeMap(XX)
#undef XX
};

struct DeviceIdArr {
    std::vector<std::string> DeviceID;
};

// 报警设备列表项
struct DeviceStatusAlarmStatusItem {
    // 报警设备编码
    std::string DeviceID;
    // 设备报警状态
    DutyStatusType DutyStatus { DutyStatusType::invalid };
};
// 报警设备状态
struct DeviceStatusAlarmStatus {
    std::vector<DeviceStatusAlarmStatusItem> Item;
    int32_t Num { 0 };
};

class PTZCommand {
public:
    enum class CommandType : uint8_t { invalid, PTZ, FI, PRESET, CRUISE, SCAN, AUX };
    enum PTZ_MOVE_TYPE : uint8_t {
        PTZ_Stop = 0x00,
        PTZ_Up = 0x01 << 3,
        PTZ_Down = 0x01 << 2,
        PTZ_Left = 0x01 << 1,
        PTZ_Right = 0x01 << 0,
        PTZ_ZoomIn = 0x01 << 4,
        PTZ_ZoomOut = 0x01 << 5,
    };
    enum PTZ_FI_TYPE : uint8_t {
        FI_IrisOut = (0x01 << 6) | (0x01 << 3), // 光圈缩小
        FI_IrisIn = (0x01 << 6) | (0x01 << 2), // 光圈放大
        FI_FocusOut = (0x01 << 6) | (0x01 << 1), // 聚焦近
        FI_FocusIn = (0x01 << 6) | (0x01 << 0), // 聚焦远
        Fi_Close = (0x01 << 6), // 停止镜头FI的所有动作
    };
    enum PTZ_PRESET_TYPE : uint8_t {
        PRESET_SetPreset = (0x01 << 7) | 0x01, // 设置预置位
        PRESET_CallPreset = (0x01 << 7) | 0x02, // 调用预置位
        PRESET_DelPreset = (0x01 << 7) | 0x03, // 删除预置位
    };
    enum PTZ_CRUISE_TYPE : uint8_t {
        CRUISE_AddPoint = (0x01 << 7) | 0x04, // 新增巡航点
        CRUISE_DelPoint = (0x01 << 7) | 0x05, // 删除巡航点
        CRUISE_SetSpeed = (0x01 << 7) | 0x06, // 设置巡航速度
        CRUISE_SetStayTime = (0x01 << 7) | 0x07, // 设置停留时间
        CRUISE_StartCrusie = (0x01 << 7) | 0x08, // 开始巡航
    };
    enum PTZ_SCAN_TYPE : uint8_t {
        SCAN_Start = (0x01 << 7) | 0x09,
        SCAN_SetSpeed = (0x01 << 7) | 0x0A,
    };
    enum PTZ_AUX_TYPE : uint8_t {
        AUX_On = (0x01 << 7) | 0x0C,
        AUX_Off = (0x01 << 7) | 0x0D,
    };
    enum ScanSubType : uint8_t { BeginScan = 0, ScanLeft = 1, ScanRight = 2 };

    explicit PTZCommand(uint16_t device_address = 0) {
        bytes[2] = static_cast<uint8_t>(device_address);
        bytes[6] = (device_address >> 8) & 0x0F;
    }

    /**
     * PTZ 指令
     * @param type 指令类型
     * @param speed 移动速度
     * - type = ZoomIn ZoomOut 时，speed 有效值范围为 0-15
     * @return
     */
    PTZCommand &Move(PTZ_MOVE_TYPE type, uint8_t speed) {
        uint8_t cmd = bytes[3] | type;
        if (cmd > 0b00101010)
            return *this;
        constexpr uint8_t MASK_LR = 0x03; // 00000011 (b0-b1)
        constexpr uint8_t MASK_UD = 0x0C; // 00001100 (b2-b3)
        constexpr uint8_t MASK_ZM = 0x30; // 00110000 (b4-b5)
        if (((cmd & MASK_LR) == MASK_LR) || // 检测b0和b1同时为1
            ((cmd & MASK_UD) == MASK_UD) || // 检测b2和b3同时为1
            ((cmd & MASK_ZM) == MASK_ZM)) // 检测b4和b5同时为1
        {
            return *this;
        }
        bytes[3] = cmd;
        if (type == PTZ_Left || type == PTZ_Right) {
            bytes[4] = speed;
        } else if (type == PTZ_Up || type == PTZ_Down) {
            bytes[5] = speed;
        } else {
            bytes[6] = (((speed & 0x0F) << 4) | (bytes[6] & 0x0F));
        }
        return *this;
    }
    /**
     * FI 指令
     * @param type 指令类型
     * @param speed 指令对应的速度 0-255
     * @return
     */
    PTZCommand &FI(PTZ_FI_TYPE type, uint8_t speed) {
        if ((bytes[3] > 0b01001010) || (bytes[3] && bytes[3] < 0b01000000))
            return *this;
        uint8_t cmd = bytes[3] | type;
        constexpr uint8_t MASK_FOCUS = 0x03;
        constexpr uint8_t MASK_IRIS = 0x0C;
        if (((cmd & MASK_FOCUS) == MASK_FOCUS) || // 检测b0和b1同时为1
            ((cmd & MASK_IRIS) == MASK_IRIS))
            return *this;
        bytes[3] = cmd;
        if (type == FI_FocusIn || type == FI_FocusOut) {
            bytes[4] = speed;
        } else if (type == FI_IrisIn || type == FI_IrisOut) {
            bytes[5] = speed;
        }
        return *this;
    }
    /**
     * 预置位操作
     * @param type 动作枚举
     * @param preset_id 预置位编码 1-255
     * @return
     */
    PTZCommand &Preset(PTZ_PRESET_TYPE type, uint8_t preset_id) {
        bytes[3] = static_cast<uint8_t>(type);
        bytes[5] = preset_id;
        return *this;
    }
    /**
     *
     * @param type
     * @param cruise_id 巡航路径编码
     * @param preset_id
     *  - type = DelPoint
     *      - preset_id = 0: 删除cruise_id对应的整个巡航路径
     *  - type = SetSpeed
     *      - preset_id 表示 巡航速度
     *  - type = SetStayTime
     *      - preset_id 表示 停止时间 单位为s
     * @return
     */
    PTZCommand &Cruise(PTZ_CRUISE_TYPE type, uint8_t cruise_id, uint16_t preset_id) {
        bytes[3] = static_cast<uint8_t>(type);
        bytes[4] = cruise_id;
        bytes[5] = static_cast<uint8_t>(preset_id);
        if (type == CRUISE_SetSpeed || type == CRUISE_SetStayTime) {
            bytes[6] = ((preset_id >> 8) << 4) | (bytes[6] & 0x0F);
        }
        return *this;
    }
    PTZCommand &Scan(uint8_t group_id, ScanSubType sub_type) {
        bytes[3] = static_cast<uint8_t>(SCAN_Start);
        bytes[4] = group_id;
        bytes[5] = static_cast<uint8_t>(sub_type);
        return *this;
    }
    PTZCommand &ScanSpeed(PTZ_SCAN_TYPE type, uint8_t group_id, uint16_t speed) {
        bytes[3] = static_cast<uint8_t>(SCAN_SetSpeed);
        bytes[4] = group_id;
        bytes[5] = static_cast<uint8_t>(speed);
        bytes[6] = ((speed >> 8) << 4) | (bytes[6] & 0x0F);
        return *this;
    }

    PTZCommand &Aux(PTZ_AUX_TYPE type, uint8_t aux_id) {
        bytes[3] = static_cast<uint8_t>(type);
        bytes[4] = aux_id;
        return *this;
    }

    // 协议操作接口
    static std::optional<PTZCommand> FromHex(std::string_view hex) {
        if (hex.length() != 16)
            return std::nullopt;
        PTZCommand frame;
        for (size_t i = 0; i < sizeof(frame); ++i) {
            auto byte = HexToByte(hex.substr(i * 2, 2));
            if (!byte)
                return std::nullopt;
            reinterpret_cast<uint8_t *>(&frame)[i] = *byte;
        }
        if (!frame.ValidateFrame())
            return std::nullopt;
        return PTZCommand(frame);
    }
    std::string ToHex() {
        CalculateBytes2();
        CalculateChecksum();
        return FrameToHex(*this);
    }
    // 数据访问接口
    uint16_t DeviceAddress() const noexcept { return ((bytes[6] & 0x0F) << 8) | bytes[2]; }
    uint8_t CommandByte() const noexcept { return bytes[3]; }
    /**
     * 字节5
     *  - PTZ 表示水平速度
     *  - FI 表示聚焦速度
     *  - PRESET 无效
     *  - CRUISE 巡航路径编号
     *  - SCAN 扫描组编码
     *  - AUX 开关编号
     * @return
     */
    uint8_t Param1() const noexcept { return bytes[4]; }
    /**
     * 字节6
     * - PTZ 表示垂直速度
     * - FI 表示光圈速度
     * - PRESET 表示预置位编号
     * - CRUISE
     *  - 设置巡航速度时 表示巡航速度
     *  - 设置巡航停留时间时 表示停留时间
     *  - 其他情况 表示预置位编号
     * - SCAN
     *  - 当为设置扫描速度时 表示扫描速度
     *  - 其他情况无效
     *  - 当AUX 时 无效
     * @return
     */
    uint16_t Param2() const noexcept {
        if (bytes[3] == 0x86 || bytes[3] == 0x87 || bytes[3] == 0x8A) {
            return ((bytes[6] & 0xF0) << 4) | bytes[5];
        }
        return bytes[5];
    }
    /**
     * 字节7
     * - PTZ 变倍速度
     * - 其他 无效
     * @return
     */
    uint8_t Param3() const noexcept {
        if (get_command_type() == CommandType::PTZ) {
            return (bytes[6] & 0xF0) >> 4;
        }
        return 0;
    }

    CommandType get_command_type() const noexcept {
        if (bytes[3] == 0)
            return CommandType::PTZ;
        if (bytes[3] <= 0b00101010)
            return CommandType::PTZ;
        if (bytes[3] <= 0b01001010)
            return CommandType::FI;
        if (bytes[3] >= 0x81 && bytes[3] >= 0x83)
            return CommandType::PRESET;
        if (bytes[3] >= 0x84 && bytes[3] <= 0x88)
            return CommandType::CRUISE;
        if (bytes[3] >= 0x89 && bytes[3] <= 0x8A)
            return CommandType::SCAN;
        if (bytes[3] >= 0x8C && bytes[3] <= 0x8D)
            return CommandType::AUX;
        return CommandType::invalid;
    }

    PTZ_MOVE_TYPE get_move_type() const noexcept {
        if (get_command_type() == CommandType::PTZ) {
            return static_cast<PTZ_MOVE_TYPE>(bytes[4]);
        }
        return PTZ_Stop;
    }

    /**
     * 获取水平速度
     * @return
     * @remark get_command_type() == PTZ 时有效
     */
    uint8_t pan_speed() const noexcept {
        if (get_command_type() == CommandType::PTZ) {
            return bytes[4];
        }
        return 0;
    }

    /**
     * 获取垂直速度
     * @return
     * @remark get_command_type() == PTZ 时有效
     */
    uint8_t tilt_speed() const noexcept {
        if (get_command_type() == CommandType::PTZ) {
            return bytes[5];
        }
        return 0;
    }
    /**
     * 获取变倍速度
     * @return
     * @remark get_command_type() == PTZ 时有效
     */
    uint8_t zoom_speed() const noexcept {
        if (get_command_type() == CommandType::PTZ) {
            return (bytes[6] & 0xF0) >> 4;
        }
        return 0;
    }
    /**
     * 获取光圈速度
     * @return
     * @remark get_command_type() == FI 时有效
     */
    uint8_t iris_speed() const noexcept {
        if (get_command_type() == CommandType::FI) {
            return bytes[5];
        }
        return 0;
    }
    /**
     * 获取聚焦速度
     * @return
     * @remark get_command_type() == FI 时有效
     */
    uint8_t focus_speed() const noexcept {
        if (get_command_type() == CommandType::FI) {
            return bytes[4];
        }
    }
    /**
     * 获取预置位编号
     * @return
     * @remark get_command_type() == PRESET 或 CRUISE时有效
     */
    uint8_t get_preset_id() const noexcept {
        if (get_command_type() == CommandType::PRESET) {
            return bytes[5];
        } else if (get_command_type() == CommandType::CRUISE) {
            if (bytes[3] == 0x84 || bytes[3] == 0x85) {
                return bytes[5];
            }
        }
        return 0;
    }
    /**
     * 获取续航路径编号
     * @return
     * @remark get_command_type() == PRESET 时有效
     */
    uint8_t get_crusie_id() const noexcept {
        if (get_command_type() == CommandType::CRUISE) {
            return bytes[4];
        }
        return 0;
    }
    uint16_t get_cruise_speed() const noexcept {
        if (bytes[3] == 0x86) {
            return (bytes[5] | ((bytes[6] & 0xF0) << 4));
        }
        return 0;
    }
    uint16_t get_cruise_stay_time() const noexcept {
        if (bytes[3] == 0x87) {
            return (bytes[5] | ((bytes[6] & 0xF0) << 4));
        }
        return 0;
    }
    uint8_t get_aux_id() const noexcept {
        if (get_command_type() == CommandType::AUX) {
            return bytes[4];
        }
        return 0;
    }

    static std::string FrameToHex(const PTZCommand &frame) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&frame);
        for (size_t i = 0; i < sizeof(frame); ++i) {
            ss << std::setw(2) << static_cast<int>(bytes[i]);
        }
        return ss.str();
    }

private:
    void CalculateChecksum() {
        bytes[7] = (bytes[6] + bytes[5] + bytes[4] + bytes[3] + bytes[2] + bytes[1] + bytes[0]) % 256;
    }
    void CalculateBytes2() {
        // 字节2 ： 组合码1， 高4位是版本信息，低4位是校验位， 校验位 = （字节1的高4位 + 字节1的低4位+字节2的高4位）
        // % 16. 本文件的版本号 1.0， 版本信息 0H
        bytes[1] = (bytes[1] & 0xF0) | (((bytes[0] >> 4) + (bytes[0] & 0x0F) + (bytes[1] >> 4)) % 16);
    }
    bool ValidateFrame() {
        if (bytes[0] != 0xA5)
            return false;
        PTZCommand temp = *this;
        temp.CalculateChecksum();
        return temp.bytes[7] == bytes[7];
    }
    static std::optional<uint8_t> HexToByte(std::string_view hex) {
        if (hex.size() != 2 || !std::isxdigit(hex[0]) || !std::isxdigit(hex[1])) {
            return std::nullopt;
        }
        return static_cast<uint8_t>(std::stoi(std::string(hex), nullptr, 16));
    }

    uint8_t bytes[8] = {0xA5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
};

/**
 * 云台配置附带参数
 */
struct PtzCmdParams {
    // 预置位名称（PTZCmd 为设置预置位命令时可选）
    std::string PresetName;
    // 巡航轨迹名称（最长32字节，PTZCmd为巡航指令时可选）
    std::string CruiseTrackName;
};

struct DeviceConfigBase {
    virtual ~DeviceConfigBase() = default;
};

/**
 * 基本参数配置
 */
struct BasicParamCfgType : public DeviceConfigBase {
    /**
     * @brief 设备名称(可选)
     */
    std::string Name;
    /**
     * @brief 注册过期时间(可选)
     */
    std::optional<int> Expiration;
    /**
     * @brief 心跳间隔时间(可选)
     */
    std::optional<int> HeartBeatInterval;
    /**
     * @brief 心跳超时次数(可选)
     */
    std::optional<int> HeartBeatCount;
};

/**
 * 视频参数范围配置
 */
struct VideoParamOptCfgType : DeviceConfigBase {
    /**
     * @brief 下载速度
     */
    std::string DownloadSpeed;
    /**
     * @brief 支持的分辨率
     */
    std::string Resolution;
};
/**
 * ROI 配置
 */
struct ROIParmaInfoItem {
    // 感兴趣区域编号，取值范围 1~16(配置可选，查询应答必选)
    std::optional<int> ROISeq;
    // 感兴趣区域左上角坐标，取值为将图像按 32X32 划分后该坐标所在块按光栅扫描顺序的序号(配置可选,查询应答必选)
    std::optional<int> TopLeft;
    // 感兴趣区域右下角坐标，取值为将图像按 32X32 划分后该坐标所在块按光栅扫描顺序的序号(配置可选，查询应答必选)
    std::optional<int> BottomRight;
    // ROI区城编码质量等级,取值 0-一般:1-较好:2-好:3-很好(配置可选，查询应答必选
    std::optional<int> ROIQP;
};
struct ROIParmaInfo {
    // 感兴趣区域开关,取值 0:关闭1:打开(配置可选,查询应答必选)
    std::optional<int> ROIFlag;
    //  感兴趣区域数量,取值范围 0~16(配置可选，查询应答必选)
    std::optional<int> ROINumber;
    // 感兴趣区域(可选) 最多16条
    std::vector<ROIParmaInfoItem> Item;
};

struct SVACParamInfo {
    // 空域编码方式，取值 0-基本层;1-1 级增强(1个增强层):2-2 级增强(2个增强层);3-3 级增强(3 个增强层)(必选)
    int SVCSpaceDomainMode {};
    // 时域编码方式,取值 0-基本层;1-1 级增强:2-2 级增强:3-3 级增强(必选)
    int SVCTimeDomainMode {};
    // SSVC 增强层与基本层比例值,取值字符串,如 4;3,21,4;1,61,81等具体比例值(可选)
    std::string SSVCRatioValue;
    // 空域编码能力，取值 0-不支持:1-1 级增强(1个增强层):2-2 级增强(2个增强层);3-3 级增强(3 个增强层)(仅查询应答必选)
    std::optional<int> SVCSpaceSupportMode;
    //  时域编码能力，取值 0:不支持;1-1 级增强2-2 级增强3-3 级增强(仅查询应答必选)
    std::optional<int> SVCTimeSupportMode;
    // SSVC 增强层与基本层比例能力，取值字符串，多个取值间用英文半角“/”分割，如4:3/2:1/4:1/6:1/8:1
    // 等具体比值的一种或者多种(仅查询应答可选)
    std::string SSVCRatioSupportList;
};
struct SurveillanceParamInfo {
    // 绝对时间信息开关,取值 0-关闭1-打开(必选)
    int TimeFlag {};
    //  OSD 信息开关，取值 0-关闭;1-打开(必选)
    int OSDFlag {};
    // 智能分析信息开关，取值 0-关闭;1-打开(必选)
    int AIFlag {};
    // 地理信息开关，取值 0-关闭;1-打开(必选)
    int GISFlag {};
};
struct AudioParamInfo {
    //  声音识别特征参数开关，取值 0-关闭;1-打开(必选)
    int AudioRecognitionFlag;
};
struct SVACEncodeCfgType : DeviceConfigBase {
    //  感兴趣区域参数(必选)
    std::optional<ROIParmaInfo> ROIParma;
    // SVC参数(可选)
    std::optional<SVACParamInfo> SVACParam;
    // 监控专用信息参数(仅查询应答可选)
    std::optional<SurveillanceParamInfo> SurveillanceParam;
    // 音频参数(可选)
    std::optional<AudioParamInfo> AudioParam;
};
struct SVCParamInfo {
    // 码流显示模式,取值 0-基本层码流单独显示方式;-基本层+1 个增强层码流方式;2-基本层+2个增强层码流方式;3-基本层+3
    // 个增强层码流方式:(配置必选，查询应答可选)
    std::optional<int> SVCSTMMode;
    // 空域编码能力,取值 0-不支持:-1 级增强(1个增强层):2-2 级增强(2个增强层);3-3 级增强(3 个增强层)(仅查询应答必选)
    std::optional<int> SVCSpaceSupportMode;
    //  时城编码能力，取值 0-不支持;1-1 级增强;2-2 级增强3-3 级增强(仅查询应答必选-
    std::optional<int> SVCTimeSupportMode;
};

struct SVACDecodeCfgType : DeviceConfigBase {
    // SVC参数(可选)--
    std::optional<SVCParamInfo> SVCParam;
    // 监控专用信息参数(可选)
    std::optional<SurveillanceParamInfo> SurveillanceParam;
};
struct VideoParamAttributeCfgTypeItem {
    // 视频流编号(必选),用于实时视音频点播时指定码流编号。0-主码流;-子码流1:2-子码流 2，以此类推
    int StreamNumber { 0 };
    // 视频编码格式当前配置值(必选)取值应符合附录 G中SDP 字段规定
    std::string VideoFormat;
    //  分辨率当前配置值(必选),取值应符合附录G中SDP字段规定
    std::string Resolution;
    //  顿率当前配置值(必选),取值应符合附录 G 中SDP 字段规定 0~99
    std::string FrameRate;
    // 码率类型配置值(必选),取值应符合附录 G中SDP字段规定 1:固定码率（CBR） 2：可变码率（VBR）
    std::string BitRateType;
    // 视频码率配置值(固定码率时必选),取值应符合附录 G中SDP 字段规定
    std::string VideoBitRate;
};
struct VideoParamAttributeCfgType : DeviceConfigBase {
    std::vector<VideoParamAttributeCfgTypeItem> Item {};
    int Num { 0 };
};
struct VideoRecordPlanCfgTypeRecordScheduleTimeSegment {
    // 开始时间:时,0~23
    int8_t StartHour {};
    // 开始时间:分,0~59
    int8_t StartMin {};
    // 开始时间:秒,0~59
    int8_t StartSec {};
    // 结束时间:时,0~23
    int8_t StopHour {};
    // 结束时间:分.0~59
    int8_t StopMin {};
    // 结束时间:秒,0~59
    int8_t StopSec {};
};
struct VideoRecordPlanCfgTypeRecordSchedule {
    // 周几(必选)取值 1~7,表示周一到周日如当天无录像计划可缺少
    int8_t WeekDayNum {};
    // 每天录像计划时间段总数(必选)
    int16_t TimeSegmentSumNum {};
    // 每天录像计划时间段(必选);每天支持最多8个时间段
    std::vector<VideoRecordPlanCfgTypeRecordScheduleTimeSegment> TimeSegment;
};
struct VideoRecordPlanCfgType : DeviceConfigBase {
    // 是否启用时间计划录像配置:0-否1-是(必选)-
    int8_t RecordEnable {};
    // 码流类型:0-主码流,1-子码流 1,2-子码流 2,以此类推(必选)
    int8_t StreamNumber {};
    //  每周录像计划总天数(必选)
    uint16_t RecordScheduleSumNum {};
    // 一个星期的录像计划,可配置 7天,对应周一至周日,每天最大支持 8 个时间段配置(必选）
    std::vector<VideoRecordPlanCfgTypeRecordSchedule> RecordSchedule;
};
struct VideoAlarmRecordCfgType : DeviceConfigBase {
    // 是否启用报警录像配置:-否，1-是(必选)
    int RecordEnable {};
    // 码流编号:0-主码流,1-子码流 1,2-子码流 2,以此类推(必选)
    int StreamNumber {};
    // 录像延时时间,报警时间点后的时间,单位“秒”(可选)
    std::optional<int> RecordTime;
    // 预录时间:报警时间点前的时间,单位"秒”(可选)
    std::optional<int> PreRecordTime;
};
struct RegionListItem {
    // 区城编号，取值范围 1~4(必选)
    int Seq {};
    // 区域左上角、右下角坐标(lx,ly,rx,ry,单位像素)，格式如“20，30,50,60"(必选)
    PointType Point {};
};
struct RegionListInfo {
    // 当前区域个数，当无区域时取值为 0(必选)
    int Num {};
    // 区域(必选)
    std::vector<RegionListItem> Item;
};

struct FrameMirrorCfgType : DeviceConfigBase {
    FrameMirrorType FrameMirror {};
};

struct PictureMaskClgType : DeviceConfigBase {
    // 面面遮挡开关,取值 0-关闭,1-打开(必选)
    int On { 0 };
    // 区域总数(必选)
    int SumNum { 0 };
    // 区域列表 可选
    std::optional<RegionListInfo> RegionList {};
};
struct AlarmReportCfgType : DeviceConfigBase {
    // 移动侦测事件上报开关，取值 0-关闭，1-打开(必选)
    int8_t MotionDetection { 0 };
    // 区域入侵事件上报开关,取值 0-关闭,1-打开(必选)
    int8_t FieldDetection { 0 };
};

struct OSDCfgTypeTextItem {
    // 文字内容，长度的取值范围 0~32(必选)
    std::string Text;
    // 文字X坐标(必选)
    int X { 0 };
    // 文字Y坐标(必选)
    int Y { 0 };
    // 字体大小
    std::optional<int> FontSize {};
};
struct OSDCfgType : DeviceConfigBase {
    // 配置窗口长度像素值(必选)-
    uint16_t Length { 0 };
    // 配置窗口宽度像素值(必选)
    uint16_t Width { 0 };
    // 时间 X像素坐标(必选),以播放窗口左上角像素为原点，水平向右为正
    uint16_t TimeX { 0 };
    // 时间 Y 像素坐标(必选),以播放窗口左上角像素为原点，竖直向下为正
    uint16_t TimeY { 0 };
    // 显示时间开关(可选),0-关闭;1-打开(默认值)
    int8_t TimeEnable { 0 };
    // 显示文字开关(可选),0-关闭;1-打开(默认值)
    int8_t TextEnable { 1 };
    // 时间显示类型(可选);0-YYYY-MM-DD HHMMSS 1- YYYY 年MM月DD日HH;MM;SS
    std::optional<int8_t> TimeType;
    // 显示文字行数总数(必选)
    int8_t SumNum { 0 };
    // 字体大小
    std::optional<int8_t> FontSize {};
    // 显示文字，可选
    std::vector<OSDCfgTypeTextItem> Item {};
};
struct SnapShotCfgType : DeviceConfigBase {
    //  连拍张数(必选),最多 10 张，当手动抓拍时，取值为 1
    int SnapNum {};
    // 单张抓拍间隔时间,单位:秒(必选),取值范围;最短 1秒
    int Interval {};
    // 抓拍图像上传路径(必选)
    std::string UploadURL;
    // 会话 ID，由平台生成，用于关联抓拍的图像与平台请求(必选)SessionID 由大小写英文字母、数字、短划线组成,长度不小于 32
    // 字节,不大于 128 字节
    std::string SessionID;
};
struct ItemMobilePositionType {
    // 目标设备编码(必选)
    std::string DeviceID;
    // 位置采集时间(必选)
    std::string CaptureTime;
    // 经度(必选)，WGS-84 坐标系
    double Longitude {};
    //  纬度(必选),WGS-84 坐标系
    double Latitude {};
    // 速度，单位;km/h(可选)
    std::optional<double> Speed;
    //  方向夹角(可选),取值为当前摄像头方向与正北方的顺时针夹角，取值范围为大于等于0 小于 360.单位:度
    std::optional<double> Direction;
    // 海拔高度，单位;米(可选)
    std::optional<double> Altitude;
    // -地面高度，单位:米(可选)
    std::optional<double> Height;
};

struct PTZPreciseCtrlType {
    /**
     * @brief 设定云台水平角度(可选) 0~360.00 度。
     * @remark 0 度:绝对0 度以球机水平光为基准,相对0 度位置以实际设置为准。
     * @remark 由用户先设定相对原点坐标，确定相对 0 度位置，以相对0 度为起点设定云台角度。
     * @remark
     * 方向;球机竖立安装,从上向下看,顺时针方向增大,逆时针方向减小;如果接收指令参数超出设备实际可转动角度限值则动作至实际最大角度。
     */
    std::optional<double> Pan;

    /**
     * @brief -设定云台垂直角度(可选),一般取值 -30.00~90.00 度;
     * @remark  0度:球机竖立安装时，镜头水平位置为 0度。
     * @remark
     * 方向;球机竖立安装时，镜头向上转，度数变小;向下转，度数变大。如果接收指令参数超出设备实际可达度数，则动作至实际最大限位角度位置。
     * 例如接收指令值为 -28.00,最大限位角为一20.00，则动作至-20.00 即可
     */
    std::optional<double> Tilt;

    /**
     * @brief 设定变焦倍数(可选)取值一般大于 1.00
     * @remark 若接收指令参数在光学变焦最大值以内的动作至对应光学变焦倍数，超出光学变焦最大值时，启动相应数字变焦。
     * @remark 设备须检查参数值有效性
     * 例如若设备光学变焦最大值 36.00,接收指今参数为 72.00则动作至36.00 倍光学变售36.00
     * 倍数字变焦(若设备数字变焦最大值小于 36.00 则按实际最大执行)。
     */
    std::optional<double> Zoom;
};

struct ItemTypeInfoDetail {
    // 摄像机结构类型,标识摄像机类型;1-球机;2-半球;3-固定枪机;4-遥控枪机;5遥控半球;6-多目设备的全景/拼接通道;7-多目设备的分制通道。当为摄像机时可选
    std::string PTZType;
    // 摄像机光电成像类型。1-可见光成像;2-热成像:3-雷达成像:4-X
    // 光成像;5-深度光场成像;9-其他。可多值,用英文半角“/”分制。当为摄像机时可选
    std::string PhotoelectricImagingType;
    // 摄像机采集部位类型。应符合附录 O中的规定。当为摄像机时可选。
    std::string CapturePositionType;
    // 摄像机安装位置室外、室内属性。1-室外、2-室内。当为摄像机时可选，缺省为1
    int RoomType { 1 };
    // 摄像机补光属性。1-无补光;2-红外补光;3-白光补光;4-激光补光:9-其他。当为摄像机时可选，缺省为 1。
    int SupplyLightType { 1 };
    // 摄像机监视方位(光轴方向)属性。1-东(西向东)2-西(东向西)3-南(北向南),4-北(南向北),5-东南(西北到东南)6-东北(西南到东北)7-西南(东北到西南).8-西北(东南到西北)。当为摄像机时且为固定摄像机或设置看守位摄像机时可选
    std::optional<int> DirectionType;
    // 摄像机支持的分辨率,可多值，用英文半角“/”。分辨率取值应符合附录 G中SDP f字段规定。当为摄像机时可选
    std::string Resolution;
    // 摄像机支持的码流编号列表，用于实时点播时指定码流编号(可选),多个取值间用英文半角“/”分割。如“0/1/2”,表示支持主码流,子码流
    // 1,子码流 2,以此类推
    std::string StreamNumberList;
    // 下载倍速(可选),可多值，用英文半角“/”分制，如设备支持 1,2,4 倍速下载则应写为“1/2/4”
    std::string DownloadSpeed;
    // 空域编码能力，取值 0-不支持;1-1 级增强(1个增强层):2-2 级增强(2个增强层):3-3 级增强(3 个增强层)(可选)
    int SVCSpaceSupportMode { 0 };
    // 时域编码能力,取值 0-不支持:1-1 级增强;2-2 级增强:3-3 级增强(可选)
    int SVCTimeSupportMode { 0 };
    // SSVC
    // 增强层与基本层比例能力，多个取值间用英文半角“/”分割。如“4;3/2:(1--1/4:1/6;1/81”等具体比例值一种或多种(可选)
    std::string SSVCRatioSupportList;
    // 移动采集设备类型(仅移动采集设备适用，必选):1-移动机器人载摄像机:2-执法记录仪:3-移动单兵设备:4-车载视频记录设备:5-无人机载摄像机:9-其他
    std::optional<int> MobileDeviceType;
    // 摄像机水平视场角(可选),取值范围大于0度小于等于 360 度
    std::optional<double> HorizontalFieldAngle;
    // 摄像机直视场角(可选),取值范围大于0度小于等于 360 度
    std::optional<double> VerticalFieldAngle;
    // 摄像机可视距离(可选).单位;米
    std::optional<double> MaxViewDistance;
    // 基层组织编码(必选非基层建设时为“000000”),编码规则采用附录 E.3 中规定的格式。
    std::string GrassrootsCode { "000000" };
    // 监控点位类型(当为摄像机时必选),1-一类视频监控点:2-二类视频监控点:3三类视频监控点;9-其他点位
    std::optional<int> PointType;
    // 点位俗称(可选),监控点位附近如有标志性建筑,场所或监控点位处于公众约定俗成的地点，可以填写标志性建设名称和地点俗称
    std::string PointCommonName;
    // 设备MAC地址(可选),用“XX-XX-XX-XX-XX-XX”格式表达，其中“XX”表示 2位十六进制数，用英文半角“-”隔开
    std::string MAC;
    // 1--摄像机卡口功能类型01-人脸卡口:02-人员卡口:03-机动车卡口:04-非机动车卡口;05-物品卡口;99-其他。可多值，用英文半角“/”分割。当为摄像机时可选
    std::string FunctionType;
    // 摄像机视频编码格式(可选),取值应符合附录 G中SDP字段规定
    std::string EncodeType;
    // 摄像机安装使用时间。一类视频监控点必选;二类、三类可选
    std::string InstallTime;
    // 摄像机所属管理单位名称(可选)
    std::string ManagementUnit;
    // 摄像机所属管理单位联系人的联系方式(电话号码,可多值，用英文半角“/”分制)。一类视频监控点必填;二类、三类选填
    std::string ContactInfo;
    // 录像保存天数(可选),一类视频监控点必填;二类、三类选填
    std::optional<int> RecordSaveDays { 0 };
    // 国民经济行业分类代码(可选),代码见 GB/T 4754 第5章
    std::string IndustrialClassification;
};

struct ItemTypeInfo {
    // 目标设备/区域/系统/业务分组/虚拟组织编码(必选)
    std::string DeviceID;
    // 设备/区城/系统/业务分组/虚拟组织名称(必选)
    std::string Name;
    // 当为设备时,设备厂商(必选)
    std::string Manufacturer;
    // 当为设备时,设备型号(必选)
    std::string Model;
    // 行政区域，可为2,4,6,8位(必选)
    std::string CivilCode;
    // 警区(可选)
    std::string Block;
    // 当为设备时,安装地址(必选)
    std::string Address;
    // 当为设备时,是否有子设备(必选)1-有,0-没有
    std::optional<int> Parental;
    /**
     * @brief 当为设备时,父节点 ID(必选);当无父设备时，为设备所属系统 ID;当有父设备时,为设备父设备 ID;
     * @remark 当为业务分组时,父节点 ID(必选);所属系统 ID;
     * @remark 当为虚拟组织时，父节点 ID(上级节点为虚拟组织时必选:上级节点为业务分组时，无此字段):父节点虚拟组织 ID;
     * @remark 当为系统时,父节点 ID(有父节点系统时必选):父节点系统 ID:
     * @remark 当为区域时,无父节点 ID;
     * @remark 可多值，用英文半角“/”分割
     */
    std::string ParentID;
    // 注册方式(必选)缺省为 1;1-符合 IETF RFC 3261
    // 标准的认证注册模式;2-基于口令的双向认证注册模式;3-基于数字证书的双向认证注册模式(高安全级别要求):4-基于数字证书的单向认证注册模式(高安全级别要求)
    std::optional<int> RegisterWay;
    // 摄像机安全能力等级代码(可选);A-GB 35114 前端设备安全能力 A 级BGB 35114前端设备安全能力 B级;CGB 35114
    // 前端设备安全能力C级
    std::string SecurityLevelCode;
    // 保密属性(必选)缺省为 0:0-不涉密.1-涉密
    std::optional<int> Secrecy {};
    // 设备/系统IPv/IPv6 地址(可选)
    std::string IPAddress;
    // 设备/系统 端口(可选)
    std::optional<int> Port;
    // 设备口令(可选)
    std::string Password;
    // 备状态(必选)
    std::optional<StatusType> Status;
    // 当为设备时，经度(一类、二类视频监控点必选)WGS-84 坐标系
    std::optional<double> Longitude;
    // 当为设备时,纬度(一类、二类视频监控点必选)WGS-84 坐标系
    std::optional<double> Latitude;
    // 虚拟组织所属的业务分组 ID,业务分组根据特定的业务需求制定，一个业务分组包含一组特定的虚拟组织
    std::string BusinessGroupID;

    std::optional<ItemTypeInfoDetail> Info;

    // 状态改变事件 ON: 上线， OFF：离线， VLOST：视频丢失，DEFECT：故障，ADD：增加，DEL：删除，UPDATE：更新
    std::optional<ItemEventType> Event;
};

struct PresetListItem {
    std::string PresetID {};
    std::string PresetName {};
};

struct AlarmCmdInfoType {
    std::string AlarmMethod { "0" };
    std::string AlarmType;
};
struct DragZoomType {
    // 全景播放窗口长度像素值
    int32_t Length{0};
    // 全景播放窗口宽度像素值
    int32_t Width{0};
    // 跟踪框中心点横轴坐标
    int32_t MidPointX{0};
    // 跟踪框中心点纵轴坐标
    int32_t MidPointY{0};
    // 跟踪框长度
    int32_t LengthX{0};
    // 跟踪框宽度
    int32_t LengthY{0};
};
struct DeviceUpgradeType {
    std::string Firmware;
    std::string FileURL;
    std::string Manufacturer;
    std::string SessionID;
};

struct HomePositionType {
    // 看守位开关（必选）0 关闭 1 开启
    int8_t Enabled { 1 };
    // 调用预置位编号, 开启看守位时使用(可选)
    std::optional<uint16_t> PresetIndex;
    // 自动归位间隔时间，开启看守位时使用, 单位：秒 （可选）
    std::optional<int32_t> ResetTime;
};
struct ItemFileType {
    // 目标设备编码
    std::string DeviceID;
    // 目标设备名称
    std::string Name;
    // 文件路径名
    std::string FilePath;
    // 录像地址
    std::string Address;
    // 录像开始时间
    std::string StartTime;
    // 录像结束时间
    std::string EndTime;
    // 保密属性
    int32_t Secrecy{0};
    // 录像产生类型 time alarm manual
    std::string Type;
    // 录像触发者
    std::string RecorderID;
    // 录像文件大小
    std::string FileSize;
    // 存储录像文件的设备
    std::string RecordLocation;
    // 码流类型： 0：主码流 1 子码流 2 以此类推
    std::optional<int8_t> StreamNumber;
};

struct CruiseTrackListItemType {
    int32_t Number{0};
    std::string Name;
};
struct CruisePointType {
    int32_t PresetIndex {0};
    int32_t StayTime{0};
    int32_t Speed{0};
};

struct SdCardInfoType {
    // SD卡编号
    int ID{0};
    // SD卡名称
    std::string HddName;
    // 状态，ok-正常，formatting-格式化，unformatted-未格式化，idle-空闲，error-错误
    std::string Status;
    //格式化进度(可选)0-100,百分比
    int32_t FormatProgress{0};
    // 储容量，单位：MB
    int32_t Capacity{0};
    // 剩余存储容量，单位：MB
    int32_t FreeSpace{0};
};

} // namespace gb28181

#endif // gb28181_include_gb28181_TYPE_DEFINE_H

/**********************************************************************************************************
文件名称:   type_define.h
创建时间:   25-2-6 上午10:24
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-6 上午10:24

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-6 上午10:24       描述:   创建文件

**********************************************************************************************************/