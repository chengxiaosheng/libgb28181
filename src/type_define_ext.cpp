#include "gb28181/type_define_ext.h"

#include "Util/util.h"
#include "tinyxml2.h"
#include <iconv.h>

#include <Util/logger.h>
#include <algorithm>

namespace gb28181 {
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
            ss << "BasicParam/";
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
#undef MAKE_ENUM_XML_ELE
#undef GET_ENUM_NAME
#undef EXPEND_ENUM_XX_NAME

void new_xml_element(const DeviceIdArr& val, tinyxml2::XMLElement *root, const char *key) {
    if (val.DeviceId.empty() || root == nullptr || key == nullptr || key[0] == '\0') return;
    auto ele = root->InsertNewChildElement(key);
    for (auto &it : val.DeviceId) {
        auto sub_ele = ele->InsertNewChildElement("DeviceId");
        sub_ele->SetText(it.c_str());
    }
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
#undef MAKE_GET_ENUM_TYPE_FUNC
#undef FROM_ENUM_XML_ELE

bool from_xml_element(DeviceIdArr &val, const tinyxml2::XMLElement *root, const char *key) {
    if (!key || key[0] == '\0' || root == nullptr)
        return false;
    if (auto ele = root->FirstChildElement(key)) {
        auto sub_ele = ele->FirstChildElement("DeviceId");
        while (sub_ele && sub_ele->GetText()) {
            val.DeviceId.emplace_back(sub_ele->GetText());
            sub_ele = sub_ele->NextSiblingElement("DeviceId");
        }
        return !val.DeviceId.empty();
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