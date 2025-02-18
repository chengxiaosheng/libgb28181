#include "gb28181/request/sdp2.h"
#include "tinyxml2.h"

#include <Util/util.h>
#include <cinttypes>
#include <iomanip>

namespace gb28181 {

std::ostream &operator<<(std::ostream &os, const sdp_session_type &obj) {
    switch (obj) {
        case sdp_session_type::invalid: os << ""; break;
        case sdp_session_type::Play: os << "Play"; break;
        case sdp_session_type::Playback: os << "Playback"; break;
        case sdp_session_type::Download: os << "Download"; break;
        case sdp_session_type::Talk: os << "Talk"; break;
    }
    return os;
}
sdp_session_type get_session_type(const std::string_view &type) {
    if (type == "Play")
        return sdp_session_type::Play;
    if (type == "Playback")
        return sdp_session_type::Playback;
    if (type == "Download")
        return sdp_session_type::Download;
    if (type == "Talk")
        return sdp_session_type::Talk;
    return sdp_session_type::invalid;
}

std::ostream &operator<<(std::ostream &os, const sdp_time_description &obj) {
    os << "t=";
    os << obj.start_time;
    os << BLANK;
    os << obj.end_time;
    os << CRLF;
    return os;
}

bool parse(sdp_time_description &time, const std::string &content) {
    auto list = toolkit::split(content.c_str() + 2, " ");
    if (list.empty())
        return false;
    tinyxml2::XMLUtil::ToInt64(list.at(0).c_str(), &time.start_time);
    if (list.size() > 1) {
        tinyxml2::XMLUtil::ToInt64(list.at(1).c_str(), &time.end_time);
    }
    return true;
}
std::ostream &operator<<(std::ostream &os, const sdp_network_address_type &obj) {
    if (obj == sdp_network_address_type::IP4)
        os << "IP4";
    else
        os << "IP6";
    return os;
}

std::ostream &operator<<(std::ostream &os, const sdp_connection &obj) {
    os << "c=";
    os << obj.nettype << BLANK;
    os << obj.addr_type << BLANK;
    os << obj.addr;
    if (obj.multicast) {
        os << "/" << obj.multicast_ttl;
    }
    os << CRLF;
    return os;
}
bool parse(sdp_connection &connection, const std::string &content) {
    auto list = toolkit::split(content.c_str() + 2, " ");
    if (list.empty() || list.size() < 3)
        return false;
    connection.nettype = list.at(0);
    connection.addr_type = list.at(1) == "IP4" ? sdp_network_address_type::IP4 : sdp_network_address_type::IP6;
    if (auto pos = list.at(2).find('/'); pos != std::string ::npos && pos + 1 != std::string ::npos) {
        connection.multicast = true;
        connection.addr = list.at(2).substr(0, pos);
        connection.multicast_ttl = atoi(list.at(2).c_str() + pos);
    } else {
        connection.addr = list.at(2);
        connection.multicast = false;
    }
    return true;
}
std::ostream &operator<<(std::ostream &os, const sdp_attr_description &obj) {
    os << "a=";
    os << obj.key;
    os << ":";
    os << obj.value;
    os << CRLF;
    return os;
}
bool parse(sdp_attr_description &connection, const std::string &content) {
    std::string_view data(content.c_str() + 2, content.length() - 2);
    if (data.length() < 1)
        return false;
    if (auto pos = data.find(':'); pos != std::string_view::npos) {
        connection.key = data.substr(0, pos);
        connection.value = data.substr(pos + 1);
    } else {
        connection.key = data;
    }
    return true;
}
std::ostream &operator<<(std::ostream &os, const sdp_media_type &obj) {
    switch (obj) {

        case sdp_media_type::none: break;
        case sdp_media_type::video: os << "video"; break;
        case sdp_media_type::audio: os << "audio"; break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const sdp_media_proto &obj) {
    switch (obj) {
        case sdp_media_proto::udp: os << "RTP/AVP"; break;
        case sdp_media_proto::tcp: os << "TCP/RTP/AVP"; break;
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_media_payload_item &obj) {
    os << "a=rtpmap:";
    os << (int)obj.payload;
    os << BLANK;
    os << obj.name << "/" << obj.sample_rate;
    os << CRLF;
    return os;
}
bool parse(sdp_media_payload_item &item, const std::string_view &value) {
    uint32_t payload { 0 };
    item.name.resize(20, '\0');
    if (3 == sscanf_s(value.data(), "%u %19[^/]/%u", &payload, item.name.data(), &item.sample_rate)) {
        item.payload = static_cast<uint8_t>(payload); // 确保类型匹配
        item.name.resize(strlen(item.name.data())); // 根据 sscanf_s 调整有效长度
        return true;
    }
    return false;
}

std::ostream &operator<<(std::ostream &os, const sdp_media_tcp_type &obj) {
    switch (obj) {
        case sdp_media_tcp_type::active: os << "active"; break;
        case sdp_media_tcp_type::passive: os << "passive"; break;
        case sdp_media_tcp_type::none: os << "none"; break;
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_media_tcp_reuse_type &obj) {
    switch (obj) {
        case sdp_media_tcp_reuse_type::new_: os << "new"; break;
        case sdp_media_tcp_reuse_type::existing: os << "existing"; break;
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_media_trans_dir &obj) {
    switch (obj) {

        case sdp_media_trans_dir::sendonly: os << "sendonly"; break;
        case sdp_media_trans_dir::recvonly: os << "recvonly"; break;
        case sdp_media_trans_dir::sendrecv: os << "sendrecv"; break;
        case sdp_media_trans_dir::inactive: os << "inactive"; break;
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_ice_candidate_type &obj) {
    switch (obj) {

        case sdp_ice_candidate_type::host: os << "host"; break;
        case sdp_ice_candidate_type::srflx: os << "srflx"; break;
        case sdp_ice_candidate_type::relay: os << "relay"; break;
    }
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_ice_session &obj) {
    os << "a=ice-ufrag:" << obj.ufrag << CRLF;
    os << "a=ice-pwd:" << obj.pwd << CRLF;
    for (auto it = obj.options.begin(); it != obj.options.end(); ++it) {
        if (it == obj.options.begin()) {
            os << "a=ice-options:" << *it;
        } else {
            os << " " << *it;
        }
    }
    if (!obj.options.empty()) {
        os << CRLF;
    }
    for (auto &it : obj.candidates) {
        os << "a=candidate:" << it.foundation << " " << static_cast<int>(it.component_id) << " "
           << (it.net_type == sdp_media_proto::udp ? "UDP" : "TCP") << " ";
        int priority_type = it.type == sdp_ice_candidate_type::host ? sdp_ice_candidate_host_priority_typ
            : it.type == sdp_ice_candidate_type::srflx              ? sdp_ice_candidate_srflx_priority_typ
                                                                    : sdp_ice_candidate_relay_priority_typ;
        if (it.priority == 0) {
            os
                << (1 << 24 * static_cast<int>(it.component_id) + (1 << 8 * priority_type) + (1 << 0 * 255)
                        + (1 << 16 * 255) * (1 - (it.type == sdp_ice_candidate_type::relay ? 1 : 0)));
        } else {
            os << it.priority;
        }
        os << " ";
        os << it.address << " " << it.port << " typ " << it.type;
        if (it.type == sdp_ice_candidate_type::srflx) {
            os << " raddr " << it.raddr << " rport " << it.rport;
        }
        os << CRLF;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const sdp_media_description &obj) {
    os << "m=";
    os << obj.type << BLANK;
    os << obj.port << BLANK;
    os << obj.proto << BLANK;
    for (auto &it : obj.payloads) {
        os << (int)it.first << BLANK;
    }
    os << CRLF;

    for (const auto &it : obj.connections) {
        os << it;
    }
    for (auto &it : obj.payloads) {
        os << it.second;
    }
    os << "a=" << obj.trans_dir << CRLF;
    if (obj.proto == sdp_media_proto::tcp) {
        os << "a=setup:" << obj.tcp_type << CRLF;
        os << "a=connection:" << obj.tcp_reuse_type << CRLF;
    }
    if (obj.ice) {
        for (auto &it : obj.ice->candidates) {
            os << "a=candidate:" << it.foundation << " " << static_cast<int>(it.component_id) << " "
               << (it.net_type == sdp_media_proto::udp ? "UDP" : "TCP") << " ";
            int priority_type = it.type == sdp_ice_candidate_type::host ? sdp_ice_candidate_host_priority_typ
                : it.type == sdp_ice_candidate_type::srflx              ? sdp_ice_candidate_srflx_priority_typ
                                                                        : sdp_ice_candidate_relay_priority_typ;
            if (it.priority == 0) {
                os
                    << (1 << 24 * static_cast<int>(it.component_id) + (1 << 8 * priority_type) + (1 << 0 * 255)
                            + (1 << 16 * 255) * (1 - (it.type == sdp_ice_candidate_type::relay ? 1 : 0)));
            } else {
                os << it.priority;
            }
            os << " ";
            os << it.address << " " << it.port << " typ " << it.type;
            if (it.type == sdp_ice_candidate_type::srflx) {
                os << " raddr " << it.raddr << " rport " << it.rport;
            }
            os << CRLF;
        }
    }

    if (obj.svc_space.has_value()) {
        os << "a=svcspace:" << obj.svc_space.value() << CRLF;
    }
    if (obj.svc_time.has_value()) {
        os << "a=svctime:" << obj.svc_time.value() << CRLF;
    }
    if (!obj.ssvc_ratio.empty()) {
        os << "a=ssvcratio:" << obj.ssvc_ratio << CRLF;
    }
    if (obj.download_speed.has_value()) {
        os << "a=download:" << obj.download_speed.value() << CRLF;
    }
    if (obj.file_size.has_value()) {
        os << "a=filesize:" << obj.file_size.value() << CRLF;
    }
    if (obj.stream_number.has_value()) {
        os << "a=streamnumber:" << (int)obj.stream_number.value() << CRLF;
        os << "a=streamprofile:" << (int)obj.stream_number.value() << CRLF;
        os << "a=stream:" << (int)obj.stream_number.value() << CRLF;
        if (obj.stream_number.value() == 0) {
            os << "a=streamMode:" << "main" << CRLF;
        } else {
            os << "a=streamMode:" << "sub" << CRLF;
        }
    }
    for (auto &it : obj.attrs) {
        os << it;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const sdp_origin_description &obj) {
    os << "o=";
    os << obj.username << BLANK;
    os << obj.sess_id << BLANK;
    os << obj.sess_version << BLANK;
    os << obj.net_type << BLANK;
    os << obj.addr_type << BLANK;
    os << obj.addr;
    os << CRLF;
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_uri_simple_type &obj) {
    os << static_cast<int>(obj);
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_uri_description &obj) {
    os << "u=";
    if (obj.is_simple) {
        os << obj.path << ":" << (int)obj.simple_type;
    } else {
        os << obj.path;
    }
    os << "\r\n";
    return os;
}
std::ostream &operator<<(std::ostream &os, const sdp_description &obj) {
    os << "v=" << obj.version << CRLF;
    os << obj.origin;
    os << "s=" << obj.s_name << CRLF;
    if (obj.uri.has_value()) {
        os << obj.uri.value();
    }
    if (obj.connection.has_value()) {
        os << obj.connection.value();
    }
    if (obj.timing.has_value()) {
        os << obj.timing.value();
    }
    // 写入会话级别ice信息
    if (obj.media.ice) {
        os << "a=ice-ufrag:" << obj.media.ice->ufrag << CRLF;
        os << "a=ice-pwd:" << obj.media.ice->pwd << CRLF;
        for (auto it = obj.media.ice->options.begin(); it != obj.media.ice->options.end(); ++it) {
            if (it == obj.media.ice->options.begin()) {
                os << "a=ice-options:" << *it;
            } else {
                os << " " << *it;
            }
        }
        if (!obj.media.ice->options.empty()) {
            os << CRLF;
        }
    }

    for (auto &it : obj.attrs) {
        os << it;
    }
    os << obj.media;
    if (obj.y_ssrc.has_value()) {
        os << "y=" << std::setw(10) << std::setfill('0') << obj.y_ssrc.value() << CRLF;
    }
    if (!obj.f_media.empty()) {
        os << "f=" << obj.f_media << CRLF;
    }
    return os;
}

sdp_network_address_type get_sdp_network_address_type(std::string_view value) {
    if (value == "IP6")
        return sdp_network_address_type::IP6;
    return sdp_network_address_type::IP4;
}

bool parse(sdp_origin_description &obj, std::string_view &content) {
    if (content.empty())
        return false;
    size_t pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.username = content.substr(0, pos);
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.sess_id = content.substr(0, pos);
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.sess_version = content.substr(0, pos);
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.net_type = content.substr(0, pos);
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.addr_type = get_sdp_network_address_type(content.substr(0, pos));
    obj.addr = content.substr(pos + 1);
    return true;
}

bool parse(sdp_uri_description &obj, std::string_view &content) {
    if (content.empty())
        return false;
    if (content[0] == 'h' && content[1] == 't' && content[2] == 't' && content[3] == 'p') {
        obj.is_simple = false;
        obj.path = content;
    } else {
        if (auto pos = content.find(':'); pos != std::string_view ::npos) {
            obj.path = content.substr(0, pos);
            obj.simple_type = (sdp_uri_simple_type)(content[1 + pos] - '0');
        } else {
            obj.path = content;
        }
    }
    return true;
}
bool parse(sdp_connection &obj, std::string_view &content) {
    if (content.empty())
        return false;

    size_t pos = content.find(' ');
    obj.nettype = content.substr(0, pos);
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.addr_type = get_sdp_network_address_type(content.substr(0, pos));
    content = content.substr(pos + 1);
    pos = content.find('/');
    if (pos == std::string_view::npos) {
        obj.addr = content;
        obj.multicast = false;
    } else {
        obj.addr = content.substr(0, pos);
        obj.multicast = true;
        if (int ttl = 0; tinyxml2::XMLUtil::ToInt(content.data() + pos + 1, &ttl)) {
            obj.multicast_ttl = ttl;
        }
    }
    return true;
}

sdp_media_tcp_type get_media_tcp_type(std::string_view value) {
    if (value == "active")
        return sdp_media_tcp_type::active;
    else if (value == "passive")
        return sdp_media_tcp_type::passive;
    return sdp_media_tcp_type::none;
}
sdp_media_tcp_reuse_type get_media_tcp_reuse_type(std::string_view value) {
    if (value == "new")
        return sdp_media_tcp_reuse_type::new_;
    return sdp_media_tcp_reuse_type::existing;
}
std::optional<sdp_ice_candidate> get_media_ice_candidate(std::string_view value) {
    auto list = toolkit::split(value.data(), " ");
    if (list.size() < 8)
        return std::nullopt;
    sdp_ice_candidate ice_candidate {};
    ice_candidate.foundation = list[0];
    ice_candidate.component_id = list[1] == "2" ? sdp_ice_component_type::rtcp : sdp_ice_component_type::rtp;
    ice_candidate.net_type = list[2] == "TCP" ? sdp_media_proto::tcp : sdp_media_proto::udp;
    tinyxml2::XMLUtil::ToUnsigned(list[3].c_str(), &ice_candidate.priority);
    ice_candidate.address = list[4];
    if (uint32_t port = 0; tinyxml2::XMLUtil::ToUnsigned(list[5].c_str(), &port)) {
        ice_candidate.port = port;
    }
    ice_candidate.type = list[7] == "host" ? sdp_ice_candidate_type::host
        : list[7] == "srflx"               ? sdp_ice_candidate_type::srflx
                                           : sdp_ice_candidate_type::relay;
    if (ice_candidate.type == sdp_ice_candidate_type::srflx && list.size() > 9) {
        ice_candidate.raddr = list[9];
    }
    if (ice_candidate.type == sdp_ice_candidate_type::srflx && list.size() > 11) {
        if (uint32_t port = 0; tinyxml2::XMLUtil::ToUnsigned(list[11].c_str(), &port)) {
            ice_candidate.rport = port;
        }
    }
    return ice_candidate;
}

bool parse_attr(sdp_description &obj, std::string_view &content, bool start_media) {
    auto split_pos = content.find(':');
    if (split_pos == std::string_view::npos) {
        return false;
    }
    std::string_view key = content.substr(0, split_pos);
    std::string_view value = content.substr(split_pos + 1);
    if (key == "ice-ufrag") {
        if (!obj.media.ice)
            obj.media.ice = sdp_ice_session();
        obj.media.ice->ufrag = content;
    } else if (key == "ice-pwd") {
        if (!obj.media.ice)
            obj.media.ice = sdp_ice_session();
        obj.media.ice->pwd = content;

    } else if (key == "ice-options") {
        if (!obj.media.ice)
            obj.media.ice = sdp_ice_session();
        obj.media.ice->options = toolkit::split(content.data(), " ");
    }
    if (start_media) {
        if (key == "rtpmap") {
            if (sdp_media_payload_item item; parse(item, value)) {
            }
            sdp_media_payload_item item;
            item.name.resize(20, '\0');
        } else if (key == "setup") {
            obj.media.tcp_type = get_media_tcp_type(value);
        } else if (key == "candidate") {
            if (auto ice_candidate = get_media_ice_candidate(value); ice_candidate) {
                obj.media.ice->candidates.emplace_back(std::move(ice_candidate).value());
            }
        } else if (key == "connection") {
            obj.media.tcp_reuse_type = get_media_tcp_reuse_type(value);
        } else if (key == "svcspace") {
            if (int temp = 0; tinyxml2::XMLUtil::ToInt(value.data(), &temp)) {
                obj.media.svc_space = temp;
            }
        } else if (key == "svctime") {
            if (int temp = 0; tinyxml2::XMLUtil::ToInt(value.data(), &temp)) {
                obj.media.svc_time = temp;
            }
        } else if (key == "ssvcratio") {
            obj.media.ssvc_ratio = value;
        } else if (key == "download") {
            if (int temp = 0; tinyxml2::XMLUtil::ToInt(value.data(), &temp)) {
                obj.media.download_speed = temp;
            }
        } else if (key == "filesize") {
            if (uint64_t temp = 0; tinyxml2::XMLUtil::ToUnsigned64(value.data(), &temp)) {
                obj.media.download_speed = temp;
            }
        } else if (key == "streamnumber" || key == "streamprofile" || key == "stream") {
            if (int temp = 0; tinyxml2::XMLUtil::ToInt(value.data(), &temp)) {
                obj.media.stream_number = temp;
            }
        } else if (key == "streamMode") {
            if (value == "sub" || value == "Sub") {
                obj.media.stream_number = 1;
            } else {
                obj.media.stream_number = 0;
            }
        } else {
            sdp_attr_description attr;
            attr.key = key;
            attr.value = value;
            obj.media.attrs.emplace_back(std::move(attr));
        }
    } else {
        sdp_attr_description attr;
        attr.key = key;
        attr.value = value;
        obj.attrs.emplace_back(std::move(attr));
    }
    return true;
}

sdp_media_type get_sdp_media_type(std::string_view value) {
    if (value == "video")
        return sdp_media_type::video;
    else if (value == "audio")
        return sdp_media_type::audio;
    else
        return sdp_media_type::none;
}
sdp_media_proto get_sdp_media_proto(std::string_view value) {
    if (value == "RTP/AVP" || value == "RTP/AVP/UDP") {
        return sdp_media_proto::udp;
    } else
        return sdp_media_proto::tcp;
}

bool parse(sdp_media_description &obj, std::string_view &content) {
    auto pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.type = get_sdp_media_type(content.substr(0, pos));
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    if (uint32_t port = 0; tinyxml2::XMLUtil::ToUnsigned(content.substr(0, pos).data(), &port)) {
        obj.port = port;
    } else
        return false;
    content = content.substr(pos + 1);
    pos = content.find(' ');
    if (pos == std::string_view::npos)
        return false;
    obj.proto = get_sdp_media_proto(content.substr(0, pos));
    return true;
}
bool parse(sdp_description &obj, const std::string &content) {
    std::istringstream iss(content);
    std::string line;
    bool start_media = false;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // 校验每一行是否包含至少2个字符（前缀和内容）
        if (line.size() < 2) {
            continue;
        }
        std::string_view prefix(line.data(), 2);
        std::string_view suffix(line.data() + 2, line.size() - 2);
        if (prefix == "v=") {
            tinyxml2::XMLUtil::ToInt(suffix.data(), &obj.version);
        } else if (prefix == "o=") {
            if (!parse(obj.origin, suffix)) {
                return false;
            }
        } else if (prefix == "s=") {
            obj.s_name = get_session_type(suffix);
        } else if (prefix == "u=") {
            if (sdp_uri_description uri; parse(uri, suffix)) {
                obj.uri = uri;
            }
        } else if (prefix == "c=") {
            if (sdp_connection conn; parse(conn, suffix)) {
                if (start_media) {
                    obj.media.connections.emplace_back(std::move(conn));
                } else {
                    obj.connection = std::move(conn);
                }
            }
        } else if (prefix == "t=") {
            sdp_time_description t;
            if (sscanf_s(suffix.data(), "%" PRId64 " %" PRId64, &t.start_time, &t.end_time) == 2) {
                obj.timing = std::move(t);
            }
        } else if (prefix == "a=") {
            parse_attr(obj, suffix, start_media);
            start_media = true;
        } else if (prefix == "m=") {
            parse(obj.media, suffix);
        } else if (prefix == "y=") {
            if (uint32_t ssrc = 0; tinyxml2::XMLUtil::ToUnsigned(suffix.data(), &ssrc)) {
                obj.y_ssrc = ssrc;
            }
        } else if (prefix == "f=") {
            obj.f_media = suffix;
        }
    }
    return obj.media.port && !obj.media.payloads.empty() && obj.s_name != sdp_session_type::invalid;
}

std::string get_sdp_description(const sdp_description &obj) {
    std::stringstream ss;
    ss << obj;
    return ss.str();
}

} // namespace gb28181

/**********************************************************************************************************
文件名称:   sdp2.cpp
创建时间:   25-2-17 下午4:11
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-17 下午4:11

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-17 下午4:11       描述:   创建文件

**********************************************************************************************************/