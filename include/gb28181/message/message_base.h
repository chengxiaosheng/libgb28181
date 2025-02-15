#ifndef gb28181_include_gb28181_messsage_MESSAGE_BASE_H
#define gb28181_include_gb28181_messsage_MESSAGE_BASE_H

#include "tinyxml2.h"
#include <gb28181/type_define.h>
#include <memory>

namespace gb28181 {

class MessageBase {
public:
    virtual ~MessageBase() = default;
    explicit MessageBase(const std::shared_ptr<tinyxml2::XMLDocument> &xml);
    MessageBase(MessageBase &&) = default;
    MessageBase &operator=(MessageBase &&) = default;
    /**
     * 消息类型
     * @return
     */
    MessageRootType root() const { return root_; }
    /**
     * 消息命令
     * @return
     */
    MessageCmdType command() const { return cmd_; }
    /**
     * xml文档字符编码
     * @return
     */
    CharEncodingType encoding() const { return encoding_; }
    void encoding(CharEncodingType encoding) { encoding_ = encoding; }
    /**
     * GB/T 28181 SN
     * @return
     */
    int sn() const { return sn_; }
    void sn(int sn) { sn_ = sn; }

    std::string &reason() { return reason_; }
    /**
     * 设备/平台编码
     * @return
     */
    const std::optional<std::string> &device_id() const { return device_id_; }
    /**
     * xml 文档指针
     * @return
     */
    std::shared_ptr<tinyxml2::XMLDocument> xml_ptr() const { return xml_ptr_; }
    /**
     * 追加用户扩展数据集合， 对回复无效 仅提交请求的时候有效
     * @param data
     */
    void extend(std::vector<ExtendData> &&data);
    /**
     * 追加用户扩展数据, 对回复无效 仅提交请求的时候有效
     * @param data
     */
    void append_extend(ExtendData &&data);
    /**
     * 获取扩展数据集， 其实没有多大的意义
     * @return
     */
    std::vector<ExtendData> &extend_data() { return extend_data_; }

    explicit operator bool() const { return is_valid_; }

    /**
     * 解析xml 文档
     * @return
     */
    bool load_from_xml();
    /**
     * 转换为xml文档
     * @param coercion 是否强制转换
     * @return
     */
    bool parse_to_xml(bool coercion = false);
    std::string str() const;

    std::string get_error() const { return error_message_; }

protected:
    MessageBase() = default;
    virtual void load_extend_data();
    virtual bool load_detail() { return true; }
    virtual bool parse_detail() { return true; }
    /**
     * 封装后的消息太长了
     */
    virtual void payload_too_big() const {}

protected:
    MessageRootType root_ { MessageRootType::invalid };
    MessageCmdType cmd_ { MessageCmdType::invalid };
    bool is_valid_ { false };
    CharEncodingType encoding_ { CharEncodingType::invalid };
    int sn_ { 0 };
    std::string reason_;
    std::optional<std::string> device_id_;
    std::shared_ptr<tinyxml2::XMLDocument> xml_ptr_ { nullptr };
    std::vector<ExtendData> extend_data_;
    std::string error_message_;
};

class ListMessageBase : public virtual MessageBase {
public:
    virtual int32_t num() = 0;
    int32_t &sum_num() { return sum_num_; }

protected:
    int32_t sum_num_ { 0 };
};

} // namespace gb28181

#endif // gb28181_include_gb28181_messsage_MESSAGE_BASE_H

/**********************************************************************************************************
文件名称:   message_base.h
创建时间:   25-2-8 下午12:33
作者名称:   Kevin
文件路径:   include/gb28181/messsage
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-8 下午12:33

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-8 下午12:33       描述:   创建文件

**********************************************************************************************************/