#ifndef gb28181_src_message_UAS_MESSAGE_H
#define gb28181_src_message_UAS_MESSAGE_H

namespace gb28181 {

class UasMessage {
public:
    virtual ~UasMessage() = default;
    virtual std::shared_ptr<MessageBase> get_request() = 0;
    template <class T, typename = std::enable_if_t<std::is_base_of<MessageBase, T>::value>>
    std::shared_ptr<T> get_request() {
        return std::dynamic_pointer_cast<T>(get_request());
    }

protected:
    UasMessage() = default;
};

} // namespace gb28181

#endif // gb28181_src_message_UAS_MESSAGE_H

/**********************************************************************************************************
文件名称:   uas_message.h
创建时间:   25-2-10 下午5:30
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午5:30

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午5:30       描述:   创建文件

**********************************************************************************************************/