#ifndef gb28181_include_gb28181_message_BROADCAST_MESSAGE_H
#define gb28181_include_gb28181_message_BROADCAST_MESSAGE_H
#include "gb28181/message/message_base.h"

namespace gb28181 {
class BroadcastNotifyRequest : public MessageBase {
public:
    explicit BroadcastNotifyRequest(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit BroadcastNotifyRequest(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    /**
     *
     * @param source_id 语音输入设备的设备编码
     * @param target_id 语音输出设备的设备编码
     */
    explicit BroadcastNotifyRequest(const std::string &source_id, const std::string &target_id);

    /**
     * 语音输入设备的设备编码
     * @return
     */
    std::string &source_id() { return source_id_; }
    /**
     * 语音输出设备的设备编码
     * @return
     */
    std::string &target_id() { return target_id_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string source_id_ {};
    std::string target_id_ {};
};

class BroadcastNotifyResponse : public MessageBase {
public:
    explicit BroadcastNotifyResponse(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit BroadcastNotifyResponse(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit BroadcastNotifyResponse(std::string device_id, ResultType result = ResultType::OK);

    ResultType &result() { return result_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_BROADCAST_MESSAGE_H

/**********************************************************************************************************
文件名称:   broadcast_message.h
创建时间:   25-2-10 下午1:48
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午1:48

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午1:48       描述:   创建文件

**********************************************************************************************************/