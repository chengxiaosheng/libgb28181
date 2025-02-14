#ifndef gb28181_ALARM_REQUEST_MESSAGE_H
#define gb28181_ALARM_REQUEST_MESSAGE_H
#include <gb28181/message/message_base.h>

namespace gb28181 {
class AlarmRequestMessage : public MessageBase {
public:
    explicit AlarmRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit AlarmRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit AlarmRequestMessage(const std::string &device_id);

    /**
     * 警起始级别 (可选)
     *
     * 取值说明：
     * - 0：全部警情；
     * - 1：一级警情；
     * - 2：二级警情；
     * - 3：三级警情；
     * - 4：四级警情。
     */
    std::string &start_alarm_priority() { return start_alarm_priority_; }

    /**
     * 警终止级别 (可选)
     *
     * 取值说明：
     * - 0：全部警情；
     * - 1：一级警情；
     * - 2：二级警情；
     * - 3：三级警情；
     * - 4：四级警情。
     */
    std::string &end_alarm_priority() { return end_alarm_priority_; }

    /**
     * 警方式条件 (可选)
     *
     * 表示报警的方式条件，取值范围如下：
     * - 0：全部；
     * - 1：电话报警；
     * - 2：设备报警；
     * - 3：短信报警；
     * - 4：GPS 报警；
     * - 5：视频报警；
     * - 6：设备故障报警；
     * - 7：其他报警。
     *
     * 支持多个报警方式的组合，例如：
     * - `1/2`：表示电话报警或设备报警。
     */
    std::string &alarm_method() { return alarm_method_; }

    /**
     * 报警类型 (可选)
     *
     * 根据报警方式 (`alarm_method`) 的取值，定义不同的报警类型：
     *
     * ### 报警方式 2 (设备报警)
     * - 不携带 `AlarmType` 时，默认为设备报警；
     * - 携带 `AlarmType` 时，取值及意义如下：
     *   - 1：视频丢失报警；
     *   - 2：设备防拆报警；
     *   - 3：存储设备磁盘满报警；
     *   - 4：设备高温报警；
     *   - 5：设备低温报警。
     *
     * ### 报警方式 5 (视频报警)
     * - `AlarmType` 取值及意义：
     *   - 1：人工视频报警；
     *   - 2：运动目标检测报警；
     *   - 3：遗留物检测报警；
     *   - 4：物体移除检测报警；
     *   - 5：绊线检测报警；
     *   - 6：入侵检测报警；
     *   - 7：逆行检测报警；
     *   - 8：徘徊检测报警；
     *   - 9：流量统计报警；
     *   - 10：密度检测报警；
     *   - 11：视频异常检测报警；
     *   - 12：快速移动报警；
     *   - 13：图像遮挡报警。
     *
     * ### 报警方式 6 (设备故障报警)
     * - `AlarmType` 取值及意义：
     *   - 1：存储设备磁盘故障报警；
     *   - 2：存储设备风扇故障报警。
     */
    std::string &alarm_type() { return alarm_type_; }

    /**
     * 报警发生起始时间 (可选)
     *
     * 表示报警的开始时间。
     */
    std::string &start_alarm_time() { return start_alarm_time_; }

    /**
     * 报警发生结束时间 (可选)
     *
     * 表示报警的结束时间。
     */
    std::string &end_alarm_time() { return end_alarm_time_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    std::string start_alarm_priority_;
    std::string end_alarm_priority_;
    std::string alarm_method_;
    std::string alarm_type_;
    std::string start_alarm_time_;
    std::string end_alarm_time_;
};

class AlarmNotifyResponseMessage : public MessageBase {
public:
    explicit AlarmNotifyResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit AlarmNotifyResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit AlarmNotifyResponseMessage(
        const std::string &device_id, ResultType result = ResultType::OK, std::string reason = "");

    ResultType result() const { return result_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
};
} // namespace gb28181

#endif // gb28181_ALARM_REQUEST_MESSAGE_H

/**********************************************************************************************************
文件名称:   alarm_request_message.h
创建时间:   25-2-14 下午4:33
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${FILE_DESCRIPTION}
修订时间:   25-2-14 下午4:33

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-14 下午4:33       描述:   创建文件

**********************************************************************************************************/