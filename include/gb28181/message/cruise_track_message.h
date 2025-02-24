#ifndef gb28181_include_gb28181_message_CRUISE_TRACK_MESSAGE_H
#define gb28181_include_gb28181_message_CRUISE_TRACK_MESSAGE_H
#include <gb28181/message/message_base.h>

namespace gb28181 {
class CruiseTrackListRequestMessage : public MessageBase {
public:
    explicit CruiseTrackListRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit CruiseTrackListRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit CruiseTrackListRequestMessage(const std::string &device_id);
};
class CruiseTrackRequestMessage : public MessageBase {
public:
    explicit CruiseTrackRequestMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit CruiseTrackRequestMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit CruiseTrackRequestMessage(const std::string &device_id, int32_t number);

    int32_t &number() { return number_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    int32_t number_ { 0 };
};

class CruiseTrackListResponseMessage : public MessageBase {
public:
    explicit CruiseTrackListResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit CruiseTrackListResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit CruiseTrackListResponseMessage(
        const std::string &device_id, int32_t sum_num, std::vector<CruiseTrackListItemType> &&list,
        ResultType result = ResultType::OK, const std::string &reason = "");
    explicit CruiseTrackListResponseMessage(
        const std::string &device_id, int32_t sum_num, std::vector<CruiseTrackListItemType> &&list);

    std::vector<CruiseTrackListItemType> &cruise_track_list() { return cruise_track_list_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
    int32_t sum_num_ { 0 };
    std::vector<CruiseTrackListItemType> cruise_track_list_;
};

class CruiseTrackResponseMessage : public MessageBase {
public:
    explicit CruiseTrackResponseMessage(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
        : MessageBase(xml) {}
    explicit CruiseTrackResponseMessage(MessageBase &&messageBase)
        : MessageBase(std::move(messageBase)) {}
    explicit CruiseTrackResponseMessage(
        const std::string &device_id, std::string name, int32_t sum_num, std::vector<CruisePointType> &&list,
        ResultType result = ResultType::OK, const std::string &reason = "");

    explicit CruiseTrackResponseMessage(const std::string &device_id, int sum_num, std::vector<CruisePointType> &&list);

    ResultType &result() { return result_; }
    int32_t &sum_num() { return sum_num_; }
    std::string &name() { return name_; }
    std::vector<CruisePointType> &points() { return cruise_points_; }

protected:
    bool load_detail() override;
    bool parse_detail() override;

private:
    ResultType result_ { ResultType::invalid };
    int32_t sum_num_ { 0 };
    std::string name_;
    std::vector<CruisePointType> cruise_points_;
};

} // namespace gb28181

#endif // gb28181_include_gb28181_message_CRUISE_TRACK_MESSAGE_H

/**********************************************************************************************************
文件名称:   cruise_track_message.h
创建时间:   25-2-10 下午3:18
作者名称:   Kevin
文件路径:   include/gb28181/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-10 下午3:18

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-10 下午3:18       描述:   创建文件

**********************************************************************************************************/