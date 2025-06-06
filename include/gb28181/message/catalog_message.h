#ifndef gb28181_include_gb28181_message_CATALOG_MESSAGE_H
#define gb28181_include_gb28181_message_CATALOG_MESSAGE_H
#include "gb28181/message/message_base.h"

namespace gb28181 {
class GB28181_EXPORT CatalogRequestMessage : public gb28181::MessageBase {
public:
    explicit CatalogRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit CatalogRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit CatalogRequestMessage(
        const std::string &device_id, std::string start_time = "", std::string end_time = "");

    std::string &start_time() { return start_time_; }
    std::string &end_time() { return end_time_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string start_time_;
    std::string end_time_;
};

class GB28181_EXPORT CatalogResponseMessage : public gb28181::ListMessageBase {
public:
    explicit CatalogResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit CatalogResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit CatalogResponseMessage(
        const std::string &device_id, int sum_num, std::vector<ItemTypeInfo> &&items, std::vector<std::string> &&extra = {});

    std::vector<ItemTypeInfo> &items() { return items_; }
    std::vector<std::string> &extra_info() { return extra_; }
    int32_t num() override {
        return items_.size();
    }

protected:
    CatalogResponseMessage() = default;
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::vector<ItemTypeInfo> items_;
    std::vector<std::string> extra_;
};

class GB28181_EXPORT CatalogNotifyMessage final: public CatalogResponseMessage {
public:
    explicit CatalogNotifyMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml)
        , CatalogResponseMessage() {}
    explicit CatalogNotifyMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase))
        , CatalogResponseMessage() {}
    explicit CatalogNotifyMessage(
        const std::string &device_id, int sum_num, std::vector<ItemTypeInfo> &&items, std::vector<std::string> &&extra);

protected:
    bool load_detail() override;
    bool parse_detail() override;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_CATALOG_MESSAGE_H

/**********************************************************************************************************
文件名称:   catalog_message.h
创建时间:   25-2-10 下午3:14
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:14

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:14       描述:   创建文件

**********************************************************************************************************/