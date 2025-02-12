#ifndef gb28181_include_gb28181_message_CONFIG_DOWNLOAD_MESSSAGE_H
#define gb28181_include_gb28181_message_CONFIG_DOWNLOAD_MESSSAGE_H
#include "gb28181/message/message_base.h"

#include <any>

namespace gb28181 {
class ConfigDownloadRequestMessage final : public MessageBase {
public:
    explicit ConfigDownloadRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit ConfigDownloadRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit ConfigDownloadRequestMessage(const std::string &device_id, DeviceConfigType config_type);

    DeviceConfigType &config_type() { return config_type_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    DeviceConfigType config_type_ { DeviceConfigType::invalid };
};

class ConfigDownloadResponseMessage final : public MessageBase {
public:
    explicit ConfigDownloadResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit ConfigDownloadResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit ConfigDownloadResponseMessage(const std::string &device_id, ResultType result_type = ResultType::OK, std::unordered_map<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> config = {});

    void set_config(DeviceConfigType config_type, std::shared_ptr<DeviceConfigBase> value) {
        configs_[config_type] = value;
    }
    template <class T, typename = std::enable_if_t<std::is_base_of<DeviceConfigBase, T>::value>>
    std::shared_ptr<T> get_config(DeviceConfigType config_type) const {
        const auto it = configs_.find(config_type);
        if (it == configs_.end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<T>(it->second);
    }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
    std::unordered_map<DeviceConfigType, std::shared_ptr<DeviceConfigBase>> configs_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_CONFIG_DOWNLOAD_MESSSAGE_H

/**********************************************************************************************************
文件名称:   config_download_messsage.h
创建时间:   25-2-10 下午3:13
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:13

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:13       描述:   创建文件

**********************************************************************************************************/