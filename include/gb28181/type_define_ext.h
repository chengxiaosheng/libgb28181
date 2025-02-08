#ifndef gb28181_include_gb28181_TYPE_DEFINE_EXT_H
#define gb28181_include_gb28181_TYPE_DEFINE_EXT_H

#include "gb28181/type_define.h"

namespace tinyxml2 {
class XMLElement;
}

namespace gb28181 {

CharEncodingType getCharEncodingType(const char *decl);
MessageRootType getRootType(const char *val);
const char *getRootTypeString(MessageRootType type);
MessageCmdType getCmdType(const char *val);
const char *getCmdTypeString(MessageCmdType type);

std::string utf8_to_gb2312(const char *data);
std::string gb2312_to_utf8(const char *data);
std::string gbk_to_utf8(const char *data);
std::string utf8_to_gbk(const char *data);

void new_xml_element(int8_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(uint8_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(int16_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(uint16_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(int32_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(uint32_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(int64_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(uint64_t val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(float val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(double val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const std::string &val, tinyxml2::XMLElement *root, const char *key);

void new_xml_element(SubscribeType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(ResultType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(OnlineType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(DeviceConfigType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(FrameMirrorType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(RecordType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(GuardType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(StatusType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(ItemEventType val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(TargetTraceType val, tinyxml2::XMLElement *root, const char *key);

void new_xml_element(const DeviceIdArr& val, tinyxml2::XMLElement *root, const char *key);


void new_xml_element(std::optional<int8_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<uint8_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<int16_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<uint16_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<int32_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<uint32_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<int64_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<uint64_t> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<float> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(std::optional<double> val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const std::optional<std::string> &val, tinyxml2::XMLElement *root, const char *key);

bool from_xml_element(int8_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(uint8_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(int16_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(uint16_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(int32_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(uint32_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(int64_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(uint64_t &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(float &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(double &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::string &val, const tinyxml2::XMLElement *root, const char *key);

bool from_xml_element(SubscribeType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(ResultType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(OnlineType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(DeviceConfigType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(FrameMirrorType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(RecordType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(GuardType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(StatusType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(ItemEventType &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(TargetTraceType &val, const tinyxml2::XMLElement *root, const char *key);

bool from_xml_element(DeviceIdArr &val, const tinyxml2::XMLElement *root, const char *key);


bool from_xml_element(std::optional<int8_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<uint8_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<int16_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<uint16_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<int32_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<uint32_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<int64_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<uint64_t> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<float> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<double> &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(std::optional<std::string> &val, const tinyxml2::XMLElement *root, const char *key);

} // namespace gb28181

#endif // gb28181_include_gb28181_TYPE_DEFINE_EXT_H

/**********************************************************************************************************
文件名称:   type_define_ext.h
创建时间:   25-2-8 下午1:09
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-8 下午1:09

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-8 下午1:09       描述:   创建文件

**********************************************************************************************************/