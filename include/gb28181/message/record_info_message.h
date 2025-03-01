#ifndef gb28181_include_gb28181_message_RECORD_INFO_MESSAGE_H
#define gb28181_include_gb28181_message_RECORD_INFO_MESSAGE_H

#include <gb28181/message/message_base.h>
namespace gb28181 {
class RecordInfoRequestMessage final : public MessageBase {
public:
    explicit RecordInfoRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit RecordInfoRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit RecordInfoRequestMessage(const std::string &device_id, std::string start_time, std::string end_time);
    /**
     * 录像检索起始时间(必选)-
     */
    std::string &start_time() { return start_time_; }
    /**
     * 像检索终止时间(必选)
     */
    std::string &end_time() { return end_time_; }
    /**
     * 件路径名(可选)
     */
    std::string &file_path() { return file_path_; }
    /**
     * 录像地址(可选 支持不完全查询)
     */
    std::string &address() { return address_; }
    /**
     * 密属性(可选)缺省为0;
     * - 0-不涉密；
     * - 1-涉密
     */
    int &secrecy() { return secrecy_; }
    /**
     * 像产生类型(可选)time或 alarm或 manual或 all
     */
    std::string &type() { return type_; }
    /**
     * 像触发者 ID(可选)--
     */
    std::string &recorder_id() { return recorder_id_; }
    /**
     * 录像模糊查询属性 (可选)
     *
     * 此属性决定录像查询的方式，缺省值为 0。
     *
     * ### 属性取值说明
     * - **0**：不进行模糊查询
     *   - 根据 SIP 消息中 `To` 头域 URI 的 `ID` 值确定查询录像的位置：
     *     - 若 `ID` 值为本域系统 ID，则进行中心历史记录检索；
     *     - 若 `ID` 值为前端设备 ID，则进行前端设备历史记录检索。
     *
     * - **1**：进行模糊查询
     *   - 此时，设备所在域应同时进行中心检索和前端检索，并将检索结果统一返回。
     */

    std::string &indistinct_query() { return indistinct_query_; }

    /**
     * 码流编号(可选):
     * - 0: 主码流；
     * - 1: 子码流1;
     * - 2: 子码流2;
     * - 以此类推
     */
    std::optional<int8_t> &stream_number() { return stream_number_; }
    /**
     * 警方式条件 (可选)
     *
     * 表示报警的方式条件，取值范围如下：
     *
     * ### 取值说明
     * - 0：全部报警；
     * - 1：电话报警；
     * - 2：设备报警；
     * - 3：短信报警；
     * - 4：GPS 报警；
     * - 5：视频报警；
     * - 6：设备故障报警；
     * - 7：其他报警。
     *
     * ### 组合条件
     * - 支持多个报警方式组合，例如：
     *   - 1/2：表示电话报警或设备报警；
     *   - 3/6：表示短信报警或设备故障报警。
     *
     */

    std::string &alarm_method() { return alarm_method_; }
    /**
     * 警类型 (可选)
     *
     * 表示报警类型，根据不同的报警方式（通过 alarm_mode 变量）决定是否携带 `AlarmType`，
     * 以及 `AlarmType` 的具体含义。
     *
     * ### 报警方式及对应取值
     *
     * 1. **报警方式 2**：设备报警（默认）
     *    - 如果 `AlarmType` 不携带，表示设备默认报警；
     *    - 如果携带，`AlarmType` 取值及含义如下：
     *      - 1：视频丢失报警；
     *      - 2：设备防拆报警；
     *      - 3：存储设备磁盘满报警；
     *      - 4：设备高温报警；
     *      - 5：设备低温报警。
     *
     * 2. **报警方式 5**：智能视频分析报警
     *    - `AlarmType` 取值及含义如下：
     *      - 1：人工视频报警；
     *      - 2：运动目标检测报警；
     *      - 3：遗留物检测报警；
     *      - 4：物体移除检测报警；
     *      - 5：绊线检测报警；
     *      - 6：入侵检测报警；
     *      - 7：逆行检测报警；
     *      - 8：徘徊检测报警；
     *      - 9：流量统计报警；
     *      - 10：密度检测报警；
     *      - 11：视频异常检测报警；
     *      - 12：快速移动报警；
     *      - 13：图像遮挡报警。
     *
     * 3. **报警方式 6**：设备故障报警
     *    - `AlarmType` 取值及含义如下：
     *      - 1：存储设备磁盘故障报警；
     *      - 2：存储设备风扇故障报警。
     */
    std::string &alarm_type() { return alarm_type_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string start_time_;
    std::string end_time_;
    std::string file_path_;
    std::string address_;
    std::string type_;
    std::string recorder_id_;
    std::string indistinct_query_;
    std::string alarm_method_;
    std::string alarm_type_;
    int secrecy_ { 0 };
    std::optional<int8_t> stream_number_;
};

class RecordInfoResponseMessage final : public ListMessageBase {
public:
    explicit RecordInfoResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit RecordInfoResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit RecordInfoResponseMessage(
        const std::string &device_id, std::string name, int32_t sum_num, std::vector<ItemFileType> &&record_list,
        std::vector<std::string> &&extra_info = {});
    explicit RecordInfoResponseMessage(const std::string &device_id,int32_t sum_num, std::vector<ItemFileType> &&record_list);


    std::string &name() { return name_; }
    /**
     * 件目录项列表，
     */
    std::vector<ItemFileType> &record_list() { return record_list_; }
    /**
     * 扩展信息
     */
    std::vector<std::string> &extra_info() { return extra_info_; }

    int32_t num() override { return record_list_.size(); }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string name_;
    std::vector<ItemFileType> record_list_;
    std::vector<std::string> extra_info_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_RECORD_INFO_MESSAGE_H

/**********************************************************************************************************
文件名称:   record_info_message.h
创建时间:   25-2-10 下午3:18
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:18

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:18       描述:   创建文件

**********************************************************************************************************/