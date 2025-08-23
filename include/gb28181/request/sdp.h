#ifndef gb28181_include_gb28181_request_SDP_H
#define gb28181_include_gb28181_request_SDP_H

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "gb28181/exports.h"

namespace gb28181 {

// 协议枚举类型
enum class SessionType : uint8_t { UNKNOWN, PLAY = 1, PLAYBACK, DOWNLOAD, TALK };
enum class TransportProtocol : uint8_t { UNKNOWN, UDP = 1, TCP };
enum class SetupType : uint8_t { ACTIVE = 0, PASSIVE, ACTPASS };
enum class Direction : uint8_t { SENDRECV = 0, SENDONLY, RECVONLY, INACTIVE };
// 专用于SDP协议的地址类型表示
enum class SDPAddressType : uint8_t { IP4 = 0, IP6 = 1 };
enum class ICECandidateType : uint8_t { HOST = 0, SRFLX, PRFLX, RELAY, UNKNOWN };
enum class ICEComponentType : uint8_t { RTP = 1, RTCP };
// 负载类型描述
struct PayloadInfo {
    uint8_t payload{0};
    uint32_t sample_rate{0};
    std::string name;
};
// ICE候选地址结构
struct ICECandidate {
    std::string foundation;
    ICEComponentType componentId {};
    TransportProtocol transport{TransportProtocol::UNKNOWN};
    ICECandidateType type{ICECandidateType::UNKNOWN};
    uint16_t port{0};
    uint16_t rport {0};
    std::string address;
    std::string raddr;
    uint32_t calculatePriority() const;
};

// Origin信息结构体
struct Origin {
    std::string username; // 用户名
    std::string session_id{"0"}; // 会话标识
    std::string session_version{"0"}; // 会话版本
    std::string net_type{"IN"}; // 网络类型（通常"IN"）
    SDPAddressType addr_type{SDPAddressType::IP4}; // 地址类型（"IP4"或"IP6"）
    std::string address; // 单播地址
    bool empty() const { return session_id.empty() && session_version.empty() && address.empty(); }
};
// 连接信息结构体
struct Connection {
    std::string net_type{"IN"}; // 网络类型（"IN"）
    SDPAddressType addr_type{SDPAddressType::IP4}; // 地址类型（"IP4"/"IP6"）
    std::string address; // 连接地址
    int ttl = 0; // 生存时间（可选）
    int num_addr = 1; // 地址数量（可选）
    bool is_multicast() const { return address.find("224.") == 0 || address.find("ff") == 0; }
    bool empty() const { return address.empty(); }
};

struct URIInfo {
    enum Type { SIMPLE, HTTP } type;
    enum FileType { ALL = 0, MANUAL, ALARM, TIME } file_type = ALL;
    std::string content; // SIMPLE存设备ID，HTTP存完整URL
    bool empty() const { return content.empty(); }
};

// 会话级信息
struct SessionInfo {
    int version{0};
    Origin origin;
    SessionType sessionType = SessionType::PLAY;
    Connection connection; // 会话级c=信息 connection;
    URIInfo uri;
    struct {
        uint64_t start_time = 0;
        uint64_t end_time = 0;
    } timing;
    bool iceLite = false;
    std::string ice_ufrag; // 会话级别ice ufrag
    std::string ice_pwd; // 会话级别ice pwd
    std::vector<std::string> ice_options; // ice 的选项
    std::unordered_map<std::string, std::string> attributes;
    bool empty() const { return origin.empty(); }
};
// 媒体级描述
struct MediaDescription {
    enum { video, audio } mediaType{video};
    uint16_t port{0};
    uint16_t stream_number{0};
    int32_t download_speed{0};
    uint64_t file_size{0};
    TransportProtocol proto{TransportProtocol::UDP};
    bool rtcp{true};
    uint32_t ssrc{0};
    std::vector<PayloadInfo> payloads;
    Direction direction = Direction::SENDRECV;
    Connection connection;
    struct {
        SetupType setup;
        bool newConnection = false;
    } tcpInfo;
    struct {
        std::string ufrag;
        std::string pwd;
        std::vector<ICECandidate> candidates;
    } ice;
    std::string f_param;
    std::unordered_map<std::string, std::string> attributes;
    bool empty() const { return payloads.empty(); }
};

class GB28181_EXPORT SdpDescription {
public:
    bool parse(const std::string &sdp);
    std::string generate() const;
    SessionInfo &session() { return session_; }
    std::vector<MediaDescription> &media() { return media_; }

private:
    SessionInfo session_ {};
    std::vector<MediaDescription> media_;
    void parseAttribute(const std::string &attr, MediaDescription *media);
};

} // namespace gb28181

#endif // gb28181_include_gb28181_request_SDP_H

/**********************************************************************************************************
文件名称:   sdp.h
创建时间:   25-2-19 上午10:46
作者名称:   Kevin
文件路径:   include/gb28181/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-19 上午10:46

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-19 上午10:46       描述:   创建文件

**********************************************************************************************************/