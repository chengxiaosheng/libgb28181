#ifndef gb28181_include_gb28181_message_PRESET_MESSAGE_H
#define gb28181_include_gb28181_message_PRESET_MESSAGE_H

#include <gb28181/message/message_base.h>

namespace gb28181 {
class PresetRequestMessage final : public MessageBase {
public:
    explicit PresetRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit PresetRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit PresetRequestMessage(const std::string &device_id);
};

class PresetResponseMessage final : public ListMessageBase {
public:
    explicit PresetResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit PresetResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit PresetResponseMessage(
        const std::string &device_id, std::vector<PresetListItem> &&vec, int32_t sum_num = 0);

    std::vector<PresetListItem> &preset_list() { return preset_list_; }

    int32_t num() override { return preset_list_.size(); }


protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    int32_t sum_num_ { 0 };
    std::vector<PresetListItem> preset_list_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_PRESET_MESSAGE_H

/**********************************************************************************************************
文件名称:   preset_message.h
创建时间:   25-2-10 下午3:15
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:15

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:15       描述:   创建文件

**********************************************************************************************************/