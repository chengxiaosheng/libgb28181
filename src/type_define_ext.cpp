#include "gb28181/type_define_ext.h"

#include "Util/util.h"
#include "tinyxml2.h"
#include <iconv.h>

#include <Util/logger.h>
#include <algorithm>

namespace gb28181 {

std::ostream &operator<<(std::ostream &os, const gb28181::MessageCmdType &type) {
#define EXPAND_XX(type, name, value, str)                                                                              \
    case gb28181::MessageCmdType::name: os << str; break;
    switch (type) {
        GB28181_XML_CMD_MAP(EXPAND_XX)
#undef EXPAND_XX
        default: os << "Unknown"; break;
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const gb28181::MessageRootType &type) {
#define EXPAND_XX(type, name, value, str)                                                                              \
    case gb28181::MessageRootType::name: os << str; break;
    switch (type) {
        GB28181_XML_ROOT_MAP(EXPAND_XX)
#undef EXPAND_XX
        default: os << "Unknown"; break;
    }
    return os;
}

DeviceConfigType operator|(const DeviceConfigType &lhs, const DeviceConfigType &rhs) {
    return static_cast<DeviceConfigType>(
        static_cast<std::underlying_type_t<DeviceConfigType>>(lhs)
        | static_cast<std::underlying_type_t<DeviceConfigType>>(rhs));
}
DeviceConfigType operator&(const DeviceConfigType &lhs, const DeviceConfigType &rhs) {
    return static_cast<DeviceConfigType>(
        static_cast<std::underlying_type_t<DeviceConfigType>>(lhs)
        & static_cast<std::underlying_type_t<DeviceConfigType>>(rhs));
}
DeviceConfigType &operator|=(DeviceConfigType &lhs, const DeviceConfigType &rhs) {
    lhs = lhs | rhs;
    return lhs;
}
DeviceConfigType &operator&=(DeviceConfigType &lhs, const DeviceConfigType &rhs) {
    lhs = lhs & rhs;
    return lhs;
}

#define EXPAND_XX(type, name, value, str)                                                                              \
    case type::name: return str;
#define GET_ENUM_TYPE_STR(map)                                                                                         \
    switch (type) { map(EXPAND_XX) default : return nullptr; }

#define EXPAND_XX_TYPE(type, name, value, str)                                                                         \
    if (strcasecmp(str, val) == 0)                                                                                     \
        return type::name;

MessageRootType getRootType(const char *val) {
    if (!val)
        return MessageRootType::invalid;
    GB28181_XML_ROOT_MAP(EXPAND_XX_TYPE)
    return MessageRootType::invalid;
}
const char *getRootTypeString(MessageRootType type) { GET_ENUM_TYPE_STR(GB28181_XML_ROOT_MAP) }

MessageCmdType getCmdType(const char *val) {
    if (!val)
        return MessageCmdType::invalid;
    GB28181_XML_CMD_MAP(EXPAND_XX_TYPE)
    return MessageCmdType::invalid;
}
const char *getCmdTypeString(MessageCmdType type) {
    GET_ENUM_TYPE_STR(GB28181_XML_CMD_MAP)
}

static std::unordered_map<std::string_view, CharEncodingType> encodingMap = {
    { "utf8", CharEncodingType::utf8 },
    { "utf-8", CharEncodingType::utf8 },
    { "gb2312", CharEncodingType::gb2312 },
    { "gbk", CharEncodingType::gbk },
};

CharEncodingType getCharEncodingType(const char *decl) {
    static const char *encodingKey = "encoding=";
    std::string_view decl_str(decl);
    auto encodingPos = decl_str.find(encodingKey);
    if (encodingPos == std::string::npos) {
        return CharEncodingType::invalid;
    }

    // 移动到 "encoding=" 后面的值起始位置
    encodingPos += strlen(encodingKey);
    // 跳过空白字符
    while (encodingPos < decl_str.size() && isspace(decl_str[encodingPos])) {
        encodingPos++;
    }
    // 确保第一个字符是引号
    if (encodingPos >= decl_str.size() || decl_str[encodingPos] != '"') {
        // 如果没有找到引号，返回空字符串（或其他处理逻辑）
        return CharEncodingType::invalid;
    }
    // 提取引号内的内容
    size_t start = ++encodingPos; // 跳过第一个引号
    while (encodingPos < decl_str.size() && decl_str[encodingPos] != '"') {
        encodingPos++;
    }
    if (!(encodingPos > start)) {
        return CharEncodingType::invalid;
    }
    std::string encoding(decl_str.substr(start, encodingPos - start));
    std::transform(encoding.begin(), encoding.end(), encoding.begin(), ::tolower);

    if (auto it = encodingMap.find(encoding); it != encodingMap.end()) {
        return it->second;
    }
    return CharEncodingType::invalid;
}

bool iconv_supports_ignore(const char *from, const char *to) {
    std::shared_ptr<void> cd(iconv_open(from, to), [](iconv_t cd) {
        if (cd != (iconv_t)-1) {
            iconv_close(cd);
        }
    });
    if (cd.get() == (iconv_t)-1) {
        WarnL << "iconv does not support conversion from " << from << " to " << to;
        return false;
    }
    return true;
}
static bool iconv_gb2312_support_ignore = iconv_supports_ignore("UTF-8", "GB2312//IGNORE");
static bool iconv_gbk_support_ignore = iconv_supports_ignore("UTF-8", "GBK//IGNORE");
static bool iconv_utf8_support_ignore = iconv_supports_ignore("GB2312", "UTF-8//IGNORE");

std::string encode_convert(const char *data, const char *from, const char *to) {
    if (!data || data[0] == '\0')
        return "";
    std::shared_ptr<void> cd(iconv_open(to, from), [](iconv_t cd) {
        if (cd != (iconv_t)-1) {
            iconv_close(cd);
        }
    });
    if (cd.get() == (iconv_t)-1) {
        ErrorL << "iconv descriptor is invalid , from= " << from << ", to= " << to;
        return data;
    }
    auto len = strlen(data);
    auto out_len = len * 2;
    std::unique_ptr<char[]> out_buf_start(new char[out_len]);
    char *out_buf = out_buf_start.get();
    char *in_buf = const_cast<char *>(data);
    if (iconv(cd.get(), &in_buf, &len, &out_buf, &out_len) == (size_t)-1) {
        WarnL << "iconv conversion failed , from= " << from << ", to= " << to;
        return data;
    }
    auto bytes_written = out_buf - out_buf_start.get();
    if (bytes_written <= 0) {
        WarnL << "No data written during iconv conversion!";
        return "";
    }
    return std::string(out_buf_start.get(), bytes_written);
}

std::string utf8_to_gb2312(const char *data) {
    if (iconv_gb2312_support_ignore) {
        return encode_convert(data, "UTF-8", "GB2312//IGNORE");
    }
    return encode_convert(data, "UTF-8", "GB2312");
}

std::string gb2312_to_utf8(const char *data) {
    if (iconv_utf8_support_ignore) {
        return encode_convert(data, "GB2312", "UTF-8//IGNORE");
    }
    return encode_convert(data, "GB2312", "UTF-8");
}

std::string gbk_to_utf8(const char *data) {
    if (iconv_utf8_support_ignore) {
        return encode_convert(data, "GBK", "UTF-8//IGNORE");
    }
    return encode_convert(data, "GBK", "UTF-8");
}

std::string utf8_to_gbk(const char *data) {
    if (iconv_gbk_support_ignore) {
        return encode_convert(data, "UTF-8", "GBK//IGNORE");
    }
    return encode_convert(data, "UTF-8", "GBK");
}

#define MAKE_NEW_XML_ELE                                                                                               \
    if (!key || key[0] == '\0' || root == nullptr)                                                                     \
        return;                                                                                                        \
    if (auto ele = root->InsertNewChildElement(key)) {                                                                 \
        ele->SetText(val);                                                                                             \
    }

void new_xml_element(int8_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(uint8_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(int16_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(uint16_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(int32_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(uint32_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(int64_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(uint64_t val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(float val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(double val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(const std::string &val, tinyxml2::XMLElement *root, const char *key) {
    if (!key || key[0] == '\0' || root == nullptr)
        return;
    if (auto ele = root->InsertNewChildElement(key)) {
        ele->SetText(val.c_str());
    }
}
#undef MAKE_NEW_XML_ELE

#define EXPEND_ENUM_XX_NAME(type, name, value, str)                                                                    \
    case type::name: name_str = str; break;
#define GET_ENUM_NAME(map)                                                                                             \
    switch (val) { map(EXPEND_ENUM_XX_NAME) default : name_str = nullptr; }
#define MAKE_ENUM_XML_ELE(type, map)                                                                                   \
    if (val == type::invalid)                                                                                          \
        return;                                                                                                        \
    if (root == nullptr || key == nullptr || key[0] == '\0')                                                           \
        return;                                                                                                        \
    const char *name_str = nullptr;                                                                                    \
    GET_ENUM_NAME(map)                                                                                                 \
    if (name_str) {                                                                                                    \
        auto ele = root->InsertNewChildElement(key);                                                                   \
        ele->SetText(name_str);                                                                                        \
    }

void new_xml_element(SubscribeType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(SubscribeType, SubscribeTypeMap)
}
void new_xml_element(ResultType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(ResultType, ResultTypeMap)
}
void new_xml_element(OnlineType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(OnlineType, OnlineTypeMap)
}

static std::unordered_map<DeviceConfigType, std::string_view> DeviceConfigTypeMap_ {
#define XX(type, name, value, str) { type::name, str },
    DeviceConfigTypeMap(XX)
#undef XX
};

void new_xml_element(DeviceConfigType val, tinyxml2::XMLElement *root, const char *key) {
    if (val == DeviceConfigType::invalid)
        return;
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto int_val = static_cast<std::underlying_type<DeviceConfigType>::type>(val);
    std::stringstream ss;
    for (auto &it : DeviceConfigTypeMap_) {
        if (static_cast<std::underlying_type<DeviceConfigType>::type>(it.first) & int_val) {
            ss << it.second << "/";
        }
    }
    std::string str = ss.str();
    if (!str.empty()) {
        str.pop_back();
        auto ele = root->InsertNewChildElement(key);
        ele->SetText(str.data());
    }
}
void new_xml_element(FrameMirrorType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(FrameMirrorType, FrameMirrorTypeMap)
}
void new_xml_element(RecordType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(RecordType, RecordTypeMap)
}
void new_xml_element(GuardType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(GuardType, GuardTypeMap)
}
void new_xml_element(StatusType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(StatusType, StatusTypeMap)
}
void new_xml_element(ItemEventType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(ItemEventType, ItemEventTypeMap)
}
void new_xml_element(TargetTraceType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(TargetTraceType, TargetTraceTypeMap)
}
void new_xml_element(DutyStatusType val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_ENUM_XML_ELE(DutyStatusType, DutyStatusTypeMap)
}
#undef MAKE_ENUM_XML_ELE
#undef GET_ENUM_NAME
#undef EXPEND_ENUM_XX_NAME

void new_xml_element(const DeviceIdArr &val, tinyxml2::XMLElement *root, const char *key) {
    if (val.DeviceID.empty() || root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    for (auto &it : val.DeviceID) {
        auto sub_ele = ele->InsertNewChildElement("DeviceID");
        sub_ele->SetText(it.c_str());
    }
}
void new_xml_element(const DeviceStatusAlarmStatusItem &val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement("key");
    new_xml_element(val.DeviceID, ele, "DeviceID");
    new_xml_element(val.DutyStatus, ele, "DutyStatus");
}
void new_xml_element(const DeviceStatusAlarmStatus &val, tinyxml2::XMLElement *root, const char *key) {
    if (val.Item.empty() || root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    for (auto &it : val.Item) {
        new_xml_element(it, ele, "Item");
    }
    ele->SetAttribute("Num", val.Item.size());
}

void new_xml_element(const BasicParamCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->Name, ele, "Name");
    new_xml_element(val->Expiration, ele, "Expiration");
    new_xml_element(val->HeartBeatInterval, ele, "HeartBeatInterval");
    new_xml_element(val->HeartBeatCount, ele, "HeartBeatCount");
}
void new_xml_element(const VideoParamOptCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->DownloadSpeed, ele, "DownloadSpeed");
    new_xml_element(val->Resolution, ele, "Resolution");
}
void new_xml_element(const SVACEncodeCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    if (val->ROIParma) {
        auto roi = ele->InsertNewChildElement("ROIParma");
        new_xml_element(val->ROIParma->ROIFlag, roi, "ROIFlag");
        new_xml_element(val->ROIParma->ROINumber, roi, "ROINumber");
        for (auto &it : val->ROIParma->Item) {
            auto item = roi->InsertNewChildElement("Item");
            new_xml_element(it.ROISeq, item, "ROISeq");
            new_xml_element(it.TopLeft, item, "TopLeft");
            new_xml_element(it.BottomRight, item, "BottomRight");
            new_xml_element(it.ROIQP, item, "ROIQP");
        }
    }
    if (val->SVACParam) {
        auto svac = ele->InsertNewChildElement("SVACParam");
        new_xml_element(val->SVACParam->SVCSpaceDomainMode, svac, "SVCSpaceDomainMode");
        new_xml_element(val->SVACParam->SVCTimeDomainMode, svac, "SVCTimeDomainMode");
        new_xml_element(val->SVACParam->SSVCRatioValue, svac, "SSVCRatioValue");
        new_xml_element(val->SVACParam->SVCSpaceSupportMode, svac, "SVCSpaceSupportMode");
        new_xml_element(val->SVACParam->SVCTimeSupportMode, svac, "SVCTimeSupportMode");
        new_xml_element(val->SVACParam->SSVCRatioSupportList, svac, "SSVCRatioSupportList");
    }
    if (val->SurveillanceParam) {
        auto surveil = ele->InsertNewChildElement("SurveillanceParam");
        new_xml_element(val->SurveillanceParam->TimeFlag, surveil, "TimeFlag");
        new_xml_element(val->SurveillanceParam->OSDFlag, surveil, "OSDFlag");
        new_xml_element(val->SurveillanceParam->AIFlag, surveil, "AIFlag");
        new_xml_element(val->SurveillanceParam->GISFlag, surveil, "GISFlag");
    }
    if (val->AudioParam) {
        auto audio = ele->InsertNewChildElement("AudioParam");
        new_xml_element(val->AudioParam->AudioRecognitionFlag, audio, "AudioRecognitionFlag");
    }
}
void new_xml_element(const SVACDecodeCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    if (val->SVCParam) {
        auto svc = ele->InsertNewChildElement("SVCParam");
        new_xml_element(val->SVCParam->SVCSTMMode, svc, "SVCSTMMode");
        new_xml_element(val->SVCParam->SVCSpaceSupportMode, svc, "SVCSpaceSupportMode");
        new_xml_element(val->SVCParam->SVCTimeSupportMode, svc, "SVCTimeSupportMode");
    }
    if (val->SurveillanceParam) {
        auto ssv = ele->InsertNewChildElement("SurveillanceParam");
        new_xml_element(val->SurveillanceParam->TimeFlag, ssv, "TimeFlag");
        new_xml_element(val->SurveillanceParam->OSDFlag, ssv, "OSDFlag");
        new_xml_element(val->SurveillanceParam->AIFlag, ssv, "AIFlag");
        new_xml_element(val->SurveillanceParam->GISFlag, ssv, "GISFlag");
    }
}
void new_xml_element(const VideoParamAttributeCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val) {
        ele->SetAttribute("Num", 0);
        return;
    }
    ele->SetAttribute("Num", val->Item.size());
    for (auto &it : val->Item) {
        auto item = ele->InsertNewChildElement("Item");
        new_xml_element(it.StreamNumber, item, "StreamNumber");
        new_xml_element(it.VideoFormat, item, "VideoFormat");
        new_xml_element(it.Resolution, item, "Resolution");
        new_xml_element(it.FrameRate, item, "FrameRate");
        new_xml_element(it.BitRateType, item, "BitRateType");
        new_xml_element(it.VideoBitRate, item, "VideoBitRate");
    }
}
void new_xml_element(const VideoRecordPlanCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->RecordEnable, ele, "RecordEnable");
    new_xml_element(val->RecordScheduleSumNum, ele, "RecordScheduleSumNum");
    for (auto &it : val->RecordSchedule) {
        auto item = ele->InsertNewChildElement("RecordSchedule");
        new_xml_element(it.WeekDayNum, item, "WeekDayNum");
        new_xml_element(it.TimeSegmentSumNum, item, "TimeSegmentSumNum");
        for (auto &t : it.TimeSegment) {
            auto item1 = ele->InsertNewChildElement("TimeSegment");
            new_xml_element(t.StartHour, item1, "StartHour");
            new_xml_element(t.StartMin, item1, "StartMin");
            new_xml_element(t.StartSec, item1, "StartSec");
            new_xml_element(t.StopHour, item1, "StopHour");
            new_xml_element(t.StopMin, item1, "StopMin");
            new_xml_element(t.StopSec, item1, "StopSec");
        }
    }
    new_xml_element(val->StreamNumber, ele, "StreamNumber");
}
void new_xml_element(const VideoAlarmRecordCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->RecordEnable, ele, "RecordEnable");
    new_xml_element(val->StreamNumber, ele, "StreamNumber");
    new_xml_element(val->RecordTime, ele, "RecordTime");
    new_xml_element(val->PreRecordTime, ele, "PreRecordTime");
}
void new_xml_element(const PictureMaskClgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->On, ele, "On");
    new_xml_element(val->SumNum, ele, "SumNum");
    if (val->RegionList) {
        auto region = ele->InsertNewChildElement("RegionList");
        region->SetAttribute("Num", val->RegionList->Item.size());
        for (auto &it : val->RegionList->Item) {
            auto item = region->InsertNewChildElement("Item");
            new_xml_element(it.Seq, ele, "Seq");
            auto str = toolkit::str_format("%d,%d,%d,%d", it.Point.lx, it.Point.ly, it.Point.rx, it.Point.ry);
            new_xml_element(str, ele, "Point");
        }
    }
}
void new_xml_element(const FrameMirrorCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    if (val) {
        new_xml_element(val->FrameMirror, root, "FrameMirror");
    } else {
        root->InsertNewChildElement(key);
    }
}
void new_xml_element(const AlarmReportCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->MotionDetection, ele, "MotionDetection");
    new_xml_element(val->FieldDetection, ele, "FieldDetection");
}

void new_xml_element(const OSDCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->Length, ele, "Length");
    new_xml_element(val->Width, ele, "Width");
    new_xml_element(val->TimeX, ele, "TimeX");
    new_xml_element(val->TimeY, ele, "TimeY");
    new_xml_element(val->TimeEnable, ele, "TimeEnable");
    new_xml_element(val->TextEnable, ele, "TextEnable");
    new_xml_element(val->TimeType, ele, "TimeType");
    new_xml_element(val->SumNum, ele, "SumNum");
    new_xml_element(val->FontSize, ele, "FontSize");
    for (auto &it : val->Item) {
        auto item = ele->InsertNewChildElement("Item");
        new_xml_element(it.Text, item, "Text");
        new_xml_element(it.X, item, "X");
        new_xml_element(it.Y, item, "Y");
        new_xml_element(it.FontSize, item, "FontSize");
    }
}
void new_xml_element(const SnapShotCfgType *val, tinyxml2::XMLElement *root, const char *key) {
    if (root == nullptr || key == nullptr || key[0] == '\0')
        return;
    auto ele = root->InsertNewChildElement(key);
    if (!val)
        return;
    new_xml_element(val->SnapNum, ele, "SnapNum");
    new_xml_element(val->Interval, ele, "Interval");
    new_xml_element(val->UploadURL, ele, "UploadURL");
    new_xml_element(val->SessionID, ele, "SessionID");
}

#define MAKE_NEW_XML_ELE                                                                                               \
    if (val)                                                                                                           \
        new_xml_element(val.value(), root, key);

void new_xml_element(std::optional<int8_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<uint8_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<int16_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<uint16_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<int32_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<uint32_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<int64_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<uint64_t> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<float> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(std::optional<double> val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
void new_xml_element(const std::optional<std::string> &val, tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
#undef MAKE_NEW_XML_ELE
#define MAKE_FROM_XML_ELE(type, origin_type, func)                                                                     \
    if (!key || key[0] == '\0' || root == nullptr)                                                                     \
        return false;                                                                                                  \
    if (auto ele = root->FirstChildElement(key)) {                                                                     \
        type value {};                                                                                                 \
        if (ele->func(&value) == tinyxml2::XML_SUCCESS) {                                                              \
            val = static_cast<origin_type>(value);                                                                     \
            return true;                                                                                               \
        }                                                                                                              \
    }                                                                                                                  \
    return false;

bool from_xml_element(int8_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(int, int8_t, QueryIntText)
}
bool from_xml_element(uint8_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(uint32_t, int8_t, QueryUnsignedText)
}
bool from_xml_element(int16_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(int, int16_t, QueryIntText)
}
bool from_xml_element(uint16_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(uint32_t, uint16_t, QueryUnsignedText)
}
bool from_xml_element(int32_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(int, int32_t, QueryIntText)
}
bool from_xml_element(uint32_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(uint32_t, uint32_t, QueryUnsignedText)
}
bool from_xml_element(int64_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(int64_t, int64_t, QueryInt64Text)
}
bool from_xml_element(uint64_t &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(uint64_t, uint64_t, QueryUnsigned64Text)
}
bool from_xml_element(float &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(float, float, QueryFloatText)
}
bool from_xml_element(double &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_FROM_XML_ELE(double, double, QueryDoubleText)
}
bool from_xml_element(std::string &val, const tinyxml2::XMLElement *root, const char *key) {
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        val = ele->GetText();
        return true;
    }
    return false;
}
#undef MAKE_FROM_XML_ELE

#define MAKE_GET_ENUM_TYPE_FUNC(type, map)                                                                             \
    type get##type(const char *val) {                                                                                  \
        if (!val || val[0] == '\0')                                                                                    \
            return type::invalid;                                                                                      \
        map(EXPAND_XX_TYPE) return type::invalid;                                                                      \
    }
MAKE_GET_ENUM_TYPE_FUNC(SubscribeType, SubscribeTypeMap)
#define FROM_ENUM_XML_ELE(type)                                                                                        \
    if (!key || key[0] == '\0' || root == nullptr)                                                                     \
        return false;                                                                                                  \
    if (auto ele = root->FirstChildElement(key)) {                                                                     \
        val = get##type(ele->GetText());                                                                               \
    }                                                                                                                  \
    return val != type::invalid;

bool from_xml_element(SubscribeType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(SubscribeType);
}
MAKE_GET_ENUM_TYPE_FUNC(ResultType, ResultTypeMap);
bool from_xml_element(ResultType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(ResultType);
}
MAKE_GET_ENUM_TYPE_FUNC(OnlineType, OnlineTypeMap);
bool from_xml_element(OnlineType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(OnlineType);
}
MAKE_GET_ENUM_TYPE_FUNC(DeviceConfigType, DeviceConfigTypeMap);
bool from_xml_element(DeviceConfigType &val, const tinyxml2::XMLElement *root, const char *key) {
    if (std::string str; from_xml_element(str, root, key)) {
        auto &&list = toolkit::split(str, "/");
        for (auto &it : list) {
            auto map_it = std::find_if(
                DeviceConfigTypeMap_.begin(), DeviceConfigTypeMap_.end(),
                [&](const std::pair<DeviceConfigType, std::string_view> &item) {
                    return strcasecmp(item.second.data(), it.c_str()) == 0;
                });
            if (map_it != DeviceConfigTypeMap_.end()) {
                val = static_cast<DeviceConfigType>(static_cast<uint32_t>(val) | static_cast<uint32_t>(map_it->first));
            }
        }
        return val != DeviceConfigType::invalid;
    }
    return false;
}
MAKE_GET_ENUM_TYPE_FUNC(FrameMirrorType, FrameMirrorTypeMap);
bool from_xml_element(FrameMirrorType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(FrameMirrorType);
}
MAKE_GET_ENUM_TYPE_FUNC(RecordType, RecordTypeMap);
bool from_xml_element(RecordType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(RecordType);
}
MAKE_GET_ENUM_TYPE_FUNC(GuardType, GuardTypeMap);
bool from_xml_element(GuardType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(GuardType);
}
MAKE_GET_ENUM_TYPE_FUNC(StatusType, StatusTypeMap);
bool from_xml_element(StatusType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(StatusType);
}
MAKE_GET_ENUM_TYPE_FUNC(ItemEventType, ItemEventTypeMap);
bool from_xml_element(ItemEventType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(ItemEventType);
}
MAKE_GET_ENUM_TYPE_FUNC(TargetTraceType, TargetTraceTypeMap);
bool from_xml_element(TargetTraceType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(TargetTraceType);
}
MAKE_GET_ENUM_TYPE_FUNC(DutyStatusType, DutyStatusTypeMap);
bool from_xml_element(DutyStatusType &val, const tinyxml2::XMLElement *root, const char *key) {
    FROM_ENUM_XML_ELE(DutyStatusType);
}
#undef MAKE_GET_ENUM_TYPE_FUNC
#undef FROM_ENUM_XML_ELE

bool from_xml_element(DeviceIdArr &val, const tinyxml2::XMLElement *root, const char *key) {
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        auto sub_ele = ele->FirstChildElement("DeviceID");
        while (sub_ele && sub_ele->GetText()) {
            val.DeviceID.emplace_back(sub_ele->GetText());
            sub_ele = sub_ele->NextSiblingElement("DeviceID");
        }
        return !val.DeviceID.empty();
    }
    return false;
}

bool from_xml_element(DeviceStatusAlarmStatus &val, const tinyxml2::XMLElement *root, const char *key) {
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        auto sub_ele = ele->FirstChildElement("Item");
        while (sub_ele) {
            DeviceStatusAlarmStatusItem item;
            if (from_xml_element(item.DeviceID, sub_ele, "DeviceID")
                && from_xml_element(item.DutyStatus, sub_ele, "DutyStatus")) {
                val.Item.emplace_back(std::move(item));
            }
            sub_ele = sub_ele->NextSiblingElement("Item");
        }
        val.Num = val.Item.size();
        return true;
    }
    return false;
}
bool from_xml_element(BasicParamCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.Name, ele, "Name");
        from_xml_element(val.Expiration, ele, "Expiration");
        from_xml_element(val.HeartBeatInterval, ele, "HeartBeatInterval");
        from_xml_element(val.HeartBeatCount, ele, "HeartBeatCount");
        return true;
    }
    return false;
}
bool from_xml_element(VideoParamOptCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.DownloadSpeed, ele, "DownloadSpeed");
        from_xml_element(val.Resolution, ele, "Resolution");
        return true;
    }
    return false;
}
bool from_xml_element(SVACEncodeCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        if (auto roi = ele->FirstChildElement("ROIParma"); roi && !roi->NoChildren()) {
            val.ROIParma = ROIParmaInfo();
            from_xml_element(val.ROIParma->ROIFlag, roi, "ROIFlag");
            from_xml_element(val.ROIParma->ROINumber, roi, "ROINumber");
            auto item = roi->FirstChildElement("Item");
            while (item) {
                if (item->NoChildren()) {
                    item = item->NextSiblingElement("Item");
                    continue;
                }
                ROIParmaInfoItem item_val;
                from_xml_element(item_val.ROISeq, item, "ROISeq");
                from_xml_element(item_val.TopLeft, item, "TopLeft");
                from_xml_element(item_val.BottomRight, item, "BottomRight");
                from_xml_element(item_val.ROIQP, item, "ROIQP");
                val.ROIParma->Item.emplace_back(std::move(item_val));
                item = item->NextSiblingElement("Item");
            }
        }
        if (auto svac = ele->FirstChildElement("SVACParam"); svac && !svac->NoChildren()) {
            val.SVACParam = SVACParamInfo();
            from_xml_element(val.SVACParam->SVCSpaceDomainMode, svac, "SVCSpaceDomainMode");
            from_xml_element(val.SVACParam->SVCTimeDomainMode, svac, "SVCTimeDomainMode");
            from_xml_element(val.SVACParam->SSVCRatioValue, svac, "SSVCRatioValue");
            from_xml_element(val.SVACParam->SVCSpaceSupportMode, svac, "SVCSpaceSupportMode");
            from_xml_element(val.SVACParam->SVCTimeSupportMode, svac, "SVCTimeSupportMode");
            from_xml_element(val.SVACParam->SSVCRatioSupportList, svac, "SSVCRatioSupportList");
        }
        if (auto item = ele->FirstChildElement("SurveillanceParam"); item && !item->NoChildren()) {
            val.SurveillanceParam = SurveillanceParamInfo();
            from_xml_element(val.SurveillanceParam->TimeFlag, item, "TimeFlag");
            from_xml_element(val.SurveillanceParam->OSDFlag, item, "OSDFlag");
            from_xml_element(val.SurveillanceParam->AIFlag, item, "AIFlag");
            from_xml_element(val.SurveillanceParam->GISFlag, item, "GISFlag");
        }
        if (auto item = ele->FirstChildElement("AudioParam"); item && !item->NoChildren()) {
            val.AudioParam = AudioParamInfo();
            from_xml_element(val.AudioParam->AudioRecognitionFlag, item, "AudioRecognitionFlag");
        }
        return true;
    }
    return false;
}
bool from_xml_element(SVACDecodeCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        if (auto item = ele->FirstChildElement("SVCParam"); item && !item->NoChildren()) {
            val.SVCParam = SVCParamInfo();
            from_xml_element(val.SVCParam->SVCSTMMode, item, "SVCSTMMode");
            from_xml_element(val.SVCParam->SVCSpaceSupportMode, item, "SVCSpaceSupportMode");
            from_xml_element(val.SVCParam->SVCTimeSupportMode, item, "SVCTimeSupportMode");
        }
        if (auto item = ele->FirstChildElement("SurveillanceParam"); item && !item->NoChildren()) {
            val.SurveillanceParam = SurveillanceParamInfo();
            from_xml_element(val.SurveillanceParam->TimeFlag, item, "TimeFlag");
            from_xml_element(val.SurveillanceParam->OSDFlag, item, "OSDFlag");
            from_xml_element(val.SurveillanceParam->AIFlag, item, "AIFlag");
            from_xml_element(val.SurveillanceParam->GISFlag, item, "GISFlag");
        }
        return true;
    }
    return false;
}
bool from_xml_element(VideoParamAttributeCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        auto item = ele->FirstChildElement("Item");
        while (item) {
            if (item->NoChildren()) {
                item = item->NextSiblingElement("Item");
                continue;
            }
            VideoParamAttributeCfgTypeItem item_val;
            from_xml_element(item_val.StreamNumber, item, "StreamNumber");
            from_xml_element(item_val.VideoFormat, item, "VideoFormat");
            from_xml_element(item_val.Resolution, item, "Resolution");
            from_xml_element(item_val.FrameRate, item, "FrameRate");
            from_xml_element(item_val.BitRateType, item, "BitRateType");
            from_xml_element(item_val.VideoBitRate, item, "VideoBitRate");
            val.Item.emplace_back(std::move(item_val));
            item = item->NextSiblingElement("Item");
        }
        val.Num = val.Item.size();
        return true;
    }
    return false;
}
bool from_xml_element(VideoRecordPlanCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.RecordEnable, ele, "RecordEnable");
        from_xml_element(val.StreamNumber, ele, "StreamNumber");
        from_xml_element(val.RecordScheduleSumNum, ele, "RecordScheduleSumNum");
        auto item = ele->FirstChildElement("RecordSchedule");
        while (item) {
            if (item->NoChildren()) {
                item = item->NextSiblingElement("RecordSchedule");
                continue;
            }
            VideoRecordPlanCfgTypeRecordSchedule item_val;
            from_xml_element(item_val.WeekDayNum, item, "WeekDayNum");
            from_xml_element(item_val.TimeSegmentSumNum, item, "TimeSegmentSumNum");

            auto item2 = item->FirstChildElement("TimeSegment");
            while (item2) {
                if (item->NoChildren()) {
                    item2 = item2->NextSiblingElement("TimeSegment");
                    continue;
                }
                VideoRecordPlanCfgTypeRecordScheduleTimeSegment item_val_t;
                from_xml_element(item_val_t.StartHour, item2, "StartHour");
                from_xml_element(item_val_t.StartMin, item2, "StartMin");
                from_xml_element(item_val_t.StartSec, item2, "StartSec");
                from_xml_element(item_val_t.StopHour, item2, "StopHour");
                from_xml_element(item_val_t.StopMin, item2, "StopMin");
                from_xml_element(item_val_t.StopSec, item2, "StopSec");
                item_val.TimeSegment.emplace_back(std::move(item_val_t));
                item2 = item2->NextSiblingElement("TimeSegment");
            }
            val.RecordSchedule.emplace_back(std::move(item_val));
            item = item->NextSiblingElement("RecordSchedule");
        }
        return true;
    }
    return false;
}
bool from_xml_element(VideoAlarmRecordCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.RecordEnable, ele, "RecordEnable");
        from_xml_element(val.StreamNumber, ele, "StreamNumber");
        from_xml_element(val.RecordTime, ele, "RecordTime");
        from_xml_element(val.PreRecordTime, ele, "PreRecordTime");
        return true;
    }
    return false;
}
bool from_xml_element(PictureMaskClgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.On, ele, "On");
        from_xml_element(val.SumNum, ele, "SumNum");
        if (auto region_list = ele->FirstChildElement("RegionList"); region_list && !region_list->NoChildren()) {
            auto item = region_list->FirstChildElement("Item");
            val.RegionList = RegionListInfo();
            while (item) {
                if (item->NoChildren()) {
                    item = item->NextSiblingElement("Item");
                    continue;
                    ;
                }
                RegionListItem item_val;
                from_xml_element(item_val.Seq, item, "Seq");
                if (auto point = item->FirstChildElement("Point")) {
                    if (sscanf_s(
                            point->GetText(), "%d,%d,%d,%d", &item_val.Point.lx, &item_val.Point.ly, &item_val.Point.rx,
                            &item_val.Point.ry)
                        != 4) {
                        WarnL << "parse PictureMask->RegionList->Item[]->Point failed, point = " << point->GetText();
                    }
                }
                val.RegionList->Item.emplace_back(item_val);
                item = item->NextSiblingElement("Item");
            }
            val.RegionList->Num = val.RegionList->Item.size();
        }
        return true;
    }
    return false;
}
bool from_xml_element(FrameMirrorCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        return from_xml_element(val.FrameMirror, root, key);
    }
    return false;
}
bool from_xml_element(AlarmReportCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.MotionDetection, ele, "MotionDetection");
        from_xml_element(val.FieldDetection, ele, "FieldDetection");
        return true;
    }
    return false;
}
bool from_xml_element(OSDCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.Length, ele, "Length");
        from_xml_element(val.Width, ele, "Width");
        from_xml_element(val.TimeX, ele, "TimeX");
        from_xml_element(val.TimeY, ele, "TimeY");
        from_xml_element(val.TimeEnable, ele, "TimeEnable");
        from_xml_element(val.TextEnable, ele, "TextEnable");
        from_xml_element(val.TimeType, ele, "TimeType");
        from_xml_element(val.SumNum, ele, "SumNum");
        from_xml_element(val.FontSize, ele, "FontSize");
        auto item = ele->FirstChildElement("Item");
        while (item) {
            if (item->NoChildren()) {
                item = item->NextSiblingElement("Item");
                continue;
            }
            OSDCfgTypeTextItem item_val;
            from_xml_element(item_val.Text, item, "Text");
            from_xml_element(item_val.X, item, "X");
            from_xml_element(item_val.Y, item, "Y");
            from_xml_element(item_val.FontSize, item, "FontSize");
            val.Item.emplace_back(std::move(item_val));
            item = item->NextSiblingElement("Item");
        }
        return true;
    }
    return false;
}
bool from_xml_element(SnapShotCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has) {
    has = false;
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        has = true;
        if (ele->NoChildren())
            return false;
        from_xml_element(val.SnapNum, ele, "SnapNum");
        from_xml_element(val.Interval, ele, "Interval");
        from_xml_element(val.UploadURL, ele, "UploadURL");
        from_xml_element(val.SessionID, ele, "SessionID");
        return true;
    }
    return false;
}

#define MAKE_NEW_XML_ELE                                                                                               \
    if (val)                                                                                                           \
        from_xml_element(val.value(), root, key);                                                                      \
    return false;

bool from_xml_element(std::optional<int8_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<uint8_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<int16_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<uint16_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<int32_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<uint32_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<int64_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<uint64_t> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<float> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<double> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}
bool from_xml_element(std::optional<std::string> &val, const tinyxml2::XMLElement *root, const char *key) {
    MAKE_NEW_XML_ELE
}

} // namespace gb28181

/**********************************************************************************************************
文件名称:   type_define_ext.cpp
创建时间:   25-2-8 下午1:09
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-8 下午1:09

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-8 下午1:09       描述:   创建文件

**********************************************************************************************************/