#ifndef gb28181_NOTIFY_MESSAGE_H
#define gb28181_NOTIFY_MESSAGE_H

#include <gb28181/message/message_base.h>
namespace gb28181 {

/**
 * 图像抓拍传输完成通知
 */
class GB28181_EXPORT UploadSnapShotFinishedNotifyMessage final : public MessageBase {
public:
    explicit UploadSnapShotFinishedNotifyMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit UploadSnapShotFinishedNotifyMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit UploadSnapShotFinishedNotifyMessage(
        const std::string &device_id, std::string session_id, std::vector<std::string> &&list);
    std::string session_id() { return session_id_; }
    std::vector<std::string> &snap_shot_lists() { return snap_shot_lists_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string session_id_ {};
    std::vector<std::string> snap_shot_lists_ {};
};

class GB28181_EXPORT MobilePositionNotifyMessage : public MessageBase {
public:
    explicit MobilePositionNotifyMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit MobilePositionNotifyMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit MobilePositionNotifyMessage(
        const std::string &device_id, const std::string &time, int32_t sum_num,
        std::vector<ItemMobilePositionType> &&list);
    std::string &time() { return time_; }
    int32_t &sum_num() { return sum_num_; }
    std::vector<ItemMobilePositionType> &device_list() { return device_list_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string time_;
    int32_t sum_num_ { 0 };
    std::vector<ItemMobilePositionType> device_list_;
};

class GB28181_EXPORT MediaStatusNotifyMessage final : public MessageBase {
public:
    explicit MediaStatusNotifyMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit MediaStatusNotifyMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit MediaStatusNotifyMessage(const std::string &device_id, std::string notify_type = "121");
    std::string &notify_type() { return notify_type_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string notify_type_ {};
};

/**
 * 设备实时视音频回传通知
 */
class GB28181_EXPORT VideoUploadNotifyMessage final : public MessageBase {
public:
    explicit VideoUploadNotifyMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit VideoUploadNotifyMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit VideoUploadNotifyMessage(
        const std::string &device_id, const std::string &time, const std::optional<double> &longitude = std::nullopt,
        const std::optional<double> &latitude = std::nullopt);

    std::string &time() { return time_; }
    std::optional<double> &longitude() { return longitude_; }
    std::optional<double> &latitude() { return latitude_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string time_;
    std::optional<double> longitude_;
    std::optional<double> latitude_;
};

class GB28181_EXPORT DeviceUpgradeResultNotifyMessage final : public MessageBase {
public:
    explicit DeviceUpgradeResultNotifyMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit DeviceUpgradeResultNotifyMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit DeviceUpgradeResultNotifyMessage(
        const std::string &device_id, std::string session_id, std::string firmware, ResultType result = ResultType::OK,
        std::string reason = "");

    ResultType &result() { return result_; }
    std::string &session_id() { return session_id_; }
    std::string &firmware() { return firmware_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
    std::string session_id_ {};
    std::string firmware_ {};
};

} // namespace gb28181

#endif // gb28181_NOTIFY_MESSAGE_H

/**********************************************************************************************************
文件名称:   notify_message.h
创建时间:   25-2-14 下午7:13
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午7:13

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午7:13       描述:   创建文件

**********************************************************************************************************/