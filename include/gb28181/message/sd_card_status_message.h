#ifndef gb28181_include_gb28181_message_SD_CARD_STATUS_MESSAGE_H
#define gb28181_include_gb28181_message_SD_CARD_STATUS_MESSAGE_H
#include <gb28181/message/message_base.h>

namespace gb28181 {
class SdCardRequestMessage final : public MessageBase {
public:
    explicit SdCardRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit SdCardRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit SdCardRequestMessage(const std::string &device_id);
};

class SdCardResponseMessage final : public MessageBase {
public:
    explicit SdCardResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit SdCardResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit SdCardResponseMessage(
        const std::string &device_id, int32_t sum_num, std::vector<SdCardInfoType> &&list,
        ResultType result = ResultType::OK, const std::string &reason = "");

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
    int32_t sum_num_ { 0 };
    std::vector<SdCardInfoType> sd_cards_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_SD_CARD_STATUS_MESSAGE_H

/**********************************************************************************************************
文件名称:   sd_card_status_message.h
创建时间:   25-2-10 下午3:18
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:18

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:18       描述:   创建文件

**********************************************************************************************************/