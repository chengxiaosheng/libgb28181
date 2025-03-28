#ifndef gb28181_include_gb28181_TYPE_DEFINE_EXT_H
#define gb28181_include_gb28181_TYPE_DEFINE_EXT_H

#include "gb28181/type_define.h"
#include "exports.h"

namespace tinyxml2 {
class XMLElement;
}



namespace gb28181 {

GB28181_EXPORT std::ostream& operator<<(std::ostream& os, const gb28181::MessageCmdType& type);
GB28181_EXPORT std::ostream& operator<<(std::ostream& os, const gb28181::MessageRootType& type);

GB28181_EXPORT DeviceConfigType operator|(const DeviceConfigType& lhs, const DeviceConfigType& rhs);
GB28181_EXPORT DeviceConfigType operator&(const DeviceConfigType& lhs, const DeviceConfigType& rhs);
GB28181_EXPORT DeviceConfigType& operator|=(DeviceConfigType& lhs, const DeviceConfigType& rhs);
GB28181_EXPORT DeviceConfigType& operator&=(DeviceConfigType& lhs, const DeviceConfigType& rhs);

GB28181_EXPORT CharEncodingType getCharEncodingType(const char *decl);
GB28181_EXPORT MessageRootType getRootType(const char *val);
GB28181_EXPORT const char *getRootTypeString(MessageRootType type);
GB28181_EXPORT MessageCmdType getCmdType(const char *val);
GB28181_EXPORT const char *getCmdTypeString(MessageCmdType type);

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
void new_xml_element(DutyStatusType val, tinyxml2::XMLElement *root, const char *key);


void new_xml_element(const DeviceIdArr& val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const DeviceStatusAlarmStatusItem& val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const DeviceStatusAlarmStatus& val, tinyxml2::XMLElement *root, const char *key);

void new_xml_element(const BasicParamCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const VideoParamOptCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const SVACEncodeCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const SVACDecodeCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const VideoParamAttributeCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const VideoRecordPlanCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const VideoAlarmRecordCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const PictureMaskClgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const FrameMirrorCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const AlarmReportCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const OSDCfgType *val, tinyxml2::XMLElement *root, const char *key);
void new_xml_element(const SnapShotCfgType *val, tinyxml2::XMLElement *root, const char *key);





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
bool from_xml_element(DutyStatusType &val, const tinyxml2::XMLElement *root, const char *key);

bool from_xml_element(DeviceIdArr &val, const tinyxml2::XMLElement *root, const char *key);
bool from_xml_element(DeviceStatusAlarmStatus& val, const tinyxml2::XMLElement *root, const char *key);

bool from_xml_element(BasicParamCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(VideoParamOptCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(SVACEncodeCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(SVACDecodeCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(VideoParamAttributeCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(VideoRecordPlanCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(VideoAlarmRecordCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(PictureMaskClgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(FrameMirrorCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(AlarmReportCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(OSDCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);
bool from_xml_element(SnapShotCfgType &val, const tinyxml2::XMLElement *root, const char *key, bool &has);




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