#ifndef gb28181_include_gb28181_request_SDP2_H
#define gb28181_include_gb28181_request_SDP2_H
#include <cstdint>
#include <optional>
#include <ostream>
#include <unordered_map>
#include <vector>

namespace gb28181 {

#define CRLF "\r\n"
#define BLANK " "

enum class sdp_session_type { invalid, Play, Playback, Download, Talk };

/**
 * @brief  t=   (time the session is active)
 * t= 0 0
 */
struct sdp_time_description {
    int64_t start_time { 0 };
    int64_t end_time { 0 };
};

enum class sdp_network_address_type { IP4, IP6 };

struct sdp_connection {
    std::string addr;
    std::string nettype { "IN" };
    sdp_network_address_type addr_type { sdp_network_address_type::IP4 };
    // 是否多播
    bool multicast { false };
    // 多播时ttl范围，取值 0-255
    uint8_t multicast_ttl { 64 };
};
bool parse(sdp_connection &connection, const std::string &content);
struct sdp_attr_description {
    std::string key;
    std::string value;
};
bool parse(sdp_attr_description &connection, const std::string &content);

enum class sdp_media_type { none = 0, video, audio };

enum class sdp_media_proto { udp, tcp };
struct sdp_media_payload_item {
    uint8_t payload {};
    std::string name;
    uint32_t sample_rate {};
};

enum class sdp_media_tcp_type { none, active, passive };

enum class sdp_media_tcp_reuse_type { new_, existing };

enum class sdp_media_trans_dir {
    sendonly, // 只发送数据
    recvonly, // 只接受数据
    sendrecv, // 双向传输，国标中应该无此选项
    inactive, // 禁止发送数据， 国标中应该无此选项
};

enum class sdp_ice_candidate_type {
    host = 1, // 主机候选
    srflx = 2, // 反射候选
    relay = 3, // 中继候选
};

#define sdp_ice_candidate_host_priority_typ 126 // 主机候选优先级类别
#define sdp_ice_candidate_srflx_priority_typ 100 // 发射候选优先级类别
#define sdp_ice_candidate_relay_priority_typ 1 // 中继候选优先级类别

enum class sdp_ice_component_type {
    rtp = 1, // rtp 通道
    rtcp = 2 // rtcp 通道
};

// candidate:<foundation> <component-id> <transport> <priority> <connection-address> <port> typ <candidate-type> [raddr <rel-addr>] [rport <rel-port>] [<extension-att>...]
struct sdp_ice_candidate {
    std::string foundation; // 候选的基础
    uint32_t priority; // 优先级
    sdp_ice_candidate_type type; // 候选类型
    std::string address; // IP 地址
    uint16_t port; // 端口
    sdp_ice_component_type component_id; // 组件 ID
    sdp_media_proto net_type;
    std::string raddr; // srflx 专用；标记，客户端私有IP
    std::string rport; // srflx 专用；标记，客户端私有端口
};

struct sdp_ice_session {
    std::string ufrag; // 用户名片段
    std::string pwd; // 密码片段
    std::vector<std::string> options; // ice options
    std::vector<sdp_ice_candidate> candidates; // ICE 候选列表
};

struct sdp_media_description {
    // 类型
    sdp_media_type type { sdp_media_type::video };
    // 端口
    uint16_t port {};
    // TCP/RTP/AVP RTP/AVP
    sdp_media_proto proto { sdp_media_proto::udp };
    // 连接类型，new 新连接， EXISTING 使用旧的连接
    sdp_media_tcp_reuse_type tcp_reuse_type { sdp_media_tcp_reuse_type::new_ };
    // 连接模式，active 主动连接， passive 被动连接
    sdp_media_tcp_type tcp_type { sdp_media_tcp_type::passive };
    // c=
    std::vector<sdp_connection> connections;
    // 负载列表
    std::unordered_map<uint8_t, sdp_media_payload_item> payloads;
    // 空域编码
    std::optional<uint8_t> svc_space;
    // 时域编码
    std::optional<uint8_t> svc_time;
    // 空域编码增强层与基本层比例，4:3,2:1,4:1,6:1,8:1 等具体比例值， 可选
    std::string ssvc_ratio;
    // 码流编号 0 主码流 1 子码流 2 第三码流 ...
    std::optional<uint8_t> stream_number {};

    std::optional<int32_t> download_speed;
    std::optional<uint32_t> file_size;
    // 默认作为接收端
    sdp_media_trans_dir trans_dir { sdp_media_trans_dir::recvonly };
    // 其他a扩展
    std::vector<sdp_attr_description> attrs;

    std::optional<sdp_ice_session> ice;
};

struct sdp_origin_description {
    std::string username;
    std::string sess_id { "0" };
    std::string sess_version { "0" };
    std::string net_type { "IN" };
    sdp_network_address_type addr_type { sdp_network_address_type::IP4 };
    std::string addr;
};

enum class sdp_uri_simple_type : int8_t {
    // 全部
    all = 0,
    // 手动产生的录像
    manual,
    // 告警产生的录像
    alarm,
    // 基于时间的录像
    time
};

struct sdp_uri_description {
    bool is_simple { true };
    sdp_uri_simple_type simple_type { sdp_uri_simple_type::all };
    std::string path;
};
struct sdp_description {
    int version { 0 };
    sdp_origin_description origin;
    sdp_session_type s_name;
    std::optional<sdp_uri_description> uri;
    std::optional<sdp_connection> connection;
    std::optional<sdp_time_description> timing;
    std::vector<sdp_attr_description> attrs;
    sdp_media_description media;
    std::optional<uint32_t> y_ssrc;
    std::string f_media;
};
std::ostream &operator<<(std::ostream &os, const sdp_description &obj);
bool parse(sdp_description &obj, const std::string &content);
std::string get_sdp_description(const sdp_description &obj);

} // namespace gb28181

#endif // gb28181_include_gb28181_request_SDP2_H

/**********************************************************************************************************
文件名称:   sdp2.h
创建时间:   25-2-17 下午4:08
作者名称:   Kevin
文件路径:   include/gb28181/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-17 下午4:08

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-17 下午4:08       描述:   创建文件

**********************************************************************************************************/