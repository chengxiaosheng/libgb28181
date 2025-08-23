#include "gb28181/request/sdp.h"

#include "RequestConfigDownloadImpl.h"
#include "tinyxml2.h"

#include <Util/logger.h>
#include <algorithm>
#include <cinttypes>
#include <filesystem>
#include <sstream>

namespace gb28181 {

// ICECandidate 方法实现
uint32_t ICECandidate::calculatePriority() const {
    const uint32_t typePref[] = { 126, 100, 110, 0 };
    return (typePref[static_cast<int>(type)] << 24) | (65535 << 8) | (256 - static_cast<int>(componentId));
}
std::string ice_candidate_type_to_string(ICECandidateType type) {
    switch (type) {
        case ICECandidateType::HOST: return "host";
        case ICECandidateType::SRFLX: return "srflx";
        case ICECandidateType::PRFLX: return "prflx";
        case ICECandidateType::RELAY: return "relay";
    }
    return "unknown";
}
ICECandidateType string_to_ice_candidate_type(const std::string &type) {
    if (type == "host")
        return ICECandidateType::HOST;
    if (type == "srflx")
        return ICECandidateType::SRFLX;
    if (type == "prflx")
        return ICECandidateType::PRFLX;
    if (type == "relay")
        return ICECandidateType::RELAY;
    return ICECandidateType::UNKNOWN;
}
std::ostream &operator<<(std::ostream &os, const ICECandidate &icecandidate) {
    os << "a=candidate:";
    os << icecandidate.foundation << " " << static_cast<int>(icecandidate.componentId) << " "
       << (icecandidate.transport == TransportProtocol::UDP ? "UDP" : "TCP") << " " << icecandidate.calculatePriority()
       << " " << icecandidate.address << " " << icecandidate.port << " typ "
       << ice_candidate_type_to_string(icecandidate.type);
    if (!icecandidate.raddr.empty())
        os << " raddr " << icecandidate.raddr;
    if (icecandidate.rport != 0)
        os << " rport " << icecandidate.rport;
    os << "\r\n";
    return os;
}

// 地址类型转换
SDPAddressType parse_address_type(const std::string &type) {
    if (type == "IP4")
        return SDPAddressType::IP4;
    if (type == "IP6")
        return SDPAddressType::IP6;
    throw std::invalid_argument("Invalid address type: " + type);
}
std::string address_type_to_string(SDPAddressType type) {
    switch (type) {
        case SDPAddressType::IP4: return "IP4";
        case SDPAddressType::IP6: return "IP6";
        default: return "IP4";
    }
}

Origin parseOrigin(const std::string &value) {
    auto parts = toolkit::split(value, " ");
    if (parts.size() < 6) {
        return {};
    }
    Origin origin;
    origin.username = parts[0];
    origin.session_id = parts[1];
    origin.session_version = parts[2];
    origin.net_type = parts[3];
    origin.addr_type = parse_address_type(parts[4]);
    origin.address = parts[5];

    return origin;
}
std::ostream &operator<<(std::ostream &os, const Origin &origin) {
    os << origin.username << " " << origin.session_id << " " << origin.session_version << " " << origin.net_type << " "
       << address_type_to_string(origin.addr_type) << " " << origin.address;
    return os;
}

Connection parseConnection(const std::string &value) {
    auto parts = toolkit::split(value, " ");
    if (parts.size() < 3) {
        return {};
    }
    Connection conn;
    conn.net_type = parts[0];
    conn.addr_type = parse_address_type(parts[1]);

    // 解析地址和可选参数
    auto addr_parts = toolkit::split(parts[2], "/");
    conn.address = addr_parts[0];

    if (addr_parts.size() > 1) {
        tinyxml2::XMLUtil::ToInt(addr_parts[1].c_str(), &conn.ttl);
    }
    if (addr_parts.size() > 2) {
        tinyxml2::XMLUtil::ToInt(addr_parts[2].c_str(), &conn.num_addr);
    }
    return conn;
}
std::ostream &operator<<(std::ostream &os, const Connection &conn) {
    if (conn.empty())
        return os;
    os << "c=" << conn.net_type << " " << address_type_to_string(conn.addr_type) << " " << conn.address;
    // 添加可选参数
    if (conn.ttl > 0 || conn.num_addr > 1) {
        os << "/" << conn.ttl;
        if (conn.num_addr > 1) {
            os << "/" << conn.num_addr;
        }
    }
    os << "\r\n";
    return os;
}

ICECandidate parseIceCandidate(const std::string &str) {
    // 跳过"a=candidate:"前缀

    auto parts = toolkit::split(str, " ");
    if (parts.size() < 8) {
        return {};
    }
    ICECandidate candidate {};
    try {
        // 解析基础字段
        candidate.foundation = parts[0];
        candidate.componentId = parts[1] == "2" ? ICEComponentType::RTCP : ICEComponentType::RTP;
        candidate.transport = parts[2] == "TCP" ? TransportProtocol::TCP : TransportProtocol::UDP;
        candidate.address = parts[4];
        candidate.port = static_cast<uint16_t>(std::stoul(parts[5]));
        candidate.type = string_to_ice_candidate_type(parts[7]);
        // 解析可选参数 (raddr/rport等)

        if (parts.size() > 9) {
            candidate.raddr = parts[9];
        }
        if (parts.size() > 11) {
            if (uint32_t port = 0; tinyxml2::XMLUtil::ToUnsigned(parts[11].c_str(), &port)) {
                candidate.rport = port;
            }
        }
    } catch (const std::exception &e) {
        WarnL << "Parse ICE candidate failed:" << e.what() << ", candidate = " << str;
    }
    return candidate;
}

std::ostream &operator<<(std::ostream &os, const URIInfo &uri) {
    if (uri.content.empty())
        return os;
    os << "u=";
    os << uri.content;
    if (uri.type != URIInfo::HTTP) {
        os << ":" << static_cast<int>(uri.file_type);
    }
    os << "\r\n";
    return os;
}

// 解析工具函数
SessionType parseSessionType(const std::string &s) {
    static const std::unordered_map<std::string, SessionType> mapping { { "Play", SessionType::PLAY },
                                                                        { "Playback", SessionType::PLAYBACK },
                                                                        { "Download", SessionType::DOWNLOAD },
                                                                        { "Talk", SessionType::TALK } };
    return mapping.count(s) ? mapping.at(s) : SessionType::UNKNOWN;
}

URIInfo parseURI(const std::string &value) {
    URIInfo uri;
    if (toolkit::start_with(value, "http://")) {
        uri.type = URIInfo::HTTP;
        uri.content = value;
        return uri;
    }
    // 处理SIMPLE模式
    uri.type = URIInfo::SIMPLE;
    size_t colon = value.find(':');
    uri.content = value.substr(0, colon);

    // 解析文件类型参数
    uri.file_type = URIInfo::ALL; // 默认值
    if (colon != std::string::npos) {
        const std::string param = value.substr(colon + 1);
        if (param == "1")
            uri.file_type = URIInfo::MANUAL;
        else if (param == "2")
            uri.file_type = URIInfo::ALARM;
        else if (param == "3")
            uri.file_type = URIInfo::TIME;
    }
    return uri;
}

MediaDescription mediaParser(const std::string &value) {
    MediaDescription md;
    std::istringstream iss(value);
    std::string protoStr;
    std::string media_type_str;
    iss >> media_type_str >> md.port >> protoStr;
    md.mediaType = media_type_str == "video" ? MediaDescription::video : MediaDescription::audio;
    md.proto = (protoStr == "TCP/RTP/AVP") ? TransportProtocol::TCP : TransportProtocol::UDP;
    return md;
}

void handlePayloadMap(const std::string_view &attr, MediaDescription *media) {
    size_t pos = attr.find(' ');
    if (pos == std::string_view::npos)
        return;
    PayloadInfo payload;
    if (uint32_t value { 0 }; tinyxml2::XMLUtil::ToUnsigned(attr.substr(0, pos).data(), &value) && value > 0) {
        payload.payload = static_cast<uint8_t>(value);
    }
    std::string_view next = attr.substr(pos + 1);
    pos = next.rfind('/');
    if (pos == std::string_view::npos)
        return;
    payload.name = next.substr(0, pos);
    if (uint32_t value { 0 }; tinyxml2::XMLUtil::ToUnsigned(next.data() + pos + 1, &value) && value > 0) {
        payload.sample_rate = value;
    }
    media->payloads.emplace_back(std::move(payload));
}

void SdpDescription::parseAttribute(const std::string &attr, MediaDescription *media) {
    std::string_view attr_view(attr);
    auto pos = attr_view.find(':');
    std::string_view k(attr_view), v("");
    if (pos != std::string_view::npos) {
        k = attr_view.substr(0, pos);
        v = attr_view.substr(pos + 1);
    }
    if (!media) {
        if (k == "ice-ufrag")
            session_.ice_ufrag = v;
        else if (k == "ice-pwd")
            session_.ice_pwd = v;
        else if (k == "ice-options")
            session_.ice_options = toolkit::split(v.data(), " ");
        else if (k == "ice-lite")
            session_.iceLite = true;
        else
            session_.attributes.emplace(k, v);
    } else {
        bool handle = true;
        if (k == "ice-ufrag")
            media->ice.ufrag = v;
        else if (k == "ice-pwd")
            media->ice.pwd = v;
        else if (k == "candidate")
            media->ice.candidates.emplace_back(parseIceCandidate(std::string(v.data(), v.size())));
        else if (k == "sendonly") {
            media->direction = Direction::SENDONLY;
        } else if (k == "recvonly") {
            media->direction = Direction::RECVONLY;
        } else if (k == "sendrecv") {
            media->direction = Direction::SENDRECV;
        } else if (k == "inactive") {
            media->direction = Direction::INACTIVE;
        } else if (k == "setup") {
            media->tcpInfo.setup = (v == "active") ? SetupType::ACTIVE
                : (v == "passive")                 ? SetupType::PASSIVE
                                                   : SetupType::ACTPASS;
        } else if (k == "connection") {
            media->tcpInfo.newConnection = v == "new";
        } else if (k == "rtpmap") {
            handlePayloadMap(v, media);
        } else if (k == "streamnumber" || k == "streamprofile" || k == "stream") {
            if (uint32_t value = 0; tinyxml2::XMLUtil::ToUnsigned(v.data(), &value)) {
                media->stream_number = value;
            }
        } else if (k == "streamMode") {
            if (v == "sub" || v == "Sub")
                media->stream_number = 1;
            else if (v == "main" || v == "Main")
                media->stream_number = 0;
            else
                handle = false;
        } else if (k == "filesize") {
            handle = tinyxml2::XMLUtil::ToUnsigned64(v.data(), &media->file_size);
        } else if (k == "downloadspeed") {
            handle = tinyxml2::XMLUtil::ToInt(v.data(), &media->download_speed);
        } else if (k == "rtcp") {
            if (v == "no") media->rtcp = false;
        }
        else {
            media->attributes.emplace(k, v);
        }
        if (!handle) {
            media->attributes.emplace(k, v);
        }
    }
}

std::ostream &operator<<(std::ostream &os, const SessionType &type) {
    static const std::string names[] = { "", "Play", "Playback", "Download", "Talk" };
    os << names[static_cast<int>(type)];
    return os;
}

std::string transportProtoToString(TransportProtocol proto) {
    return (proto == TransportProtocol::TCP) ? "TCP/RTP/AVP" : "RTP/AVP";
}
std::string setupTypeToString(SetupType type) {
    static const std::string names[] = { "active", "passive", "actpass" };
    return names[static_cast<int>(type)];
}

// SdpDescription 方法实现
bool SdpDescription::parse(const std::string &sdp) {
    std::istringstream iss(sdp);
    std::string line;
    MediaDescription *currentMedia = nullptr;
    try {
        while (std::getline(iss, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            if (line.size() < 2 || line[1] != '=') {
                continue;
            }
            char field = line[0];
            std::string value = line.substr(2);
            switch (field) {
                case 'v': tinyxml2::XMLUtil::ToInt(value.c_str(), &session_.version); break;
                case 'o': session_.origin = parseOrigin(value); break;
                case 's': session_.sessionType = parseSessionType(value); break;
                case 'u': session_.uri = parseURI(value); break;
                case 'c':
                    currentMedia ? currentMedia->connection = parseConnection(value)
                                 : session_.connection = parseConnection(value);
                    break;
                case 't': {
                    auto part = toolkit::split(value, " ");
                    if (part.size() == 2) {
                        tinyxml2::XMLUtil::ToUnsigned64(part[0].c_str(), &session_.timing.start_time);
                        tinyxml2::XMLUtil::ToUnsigned64(part[1].c_str(), &session_.timing.end_time);
                    }
                } break;
                case 'm':
                    media_.push_back(mediaParser(value));
                    currentMedia = &media_.back();
                    break;
                case 'a': parseAttribute(value, currentMedia); break;
                case 'y':
                    if (currentMedia)
                        tinyxml2::XMLUtil::ToUnsigned(value.c_str(), &currentMedia->ssrc);
                    break;
                case 'f':
                    if (currentMedia)
                        currentMedia->f_param = value;
                    break;
                default: break;
            }
        }
        return true;
    } catch (const std::exception &e) {
        WarnL << "parse sdp failed: " << e.what() << ", sdp = " << sdp;
        return false;
    }
}

std::string SdpDescription::generate() const {
    std::ostringstream oss;
    // 会话头
    oss << "v=" << session_.version << "\r\n"
        << "o=" << session_.origin << "\r\n"
        << "s=" << session_.sessionType << "\r\n"
        << session_.uri // u=
        << session_.connection // c=
        << "t=" << session_.timing.start_time << " " << session_.timing.end_time << "\r\n";

    if (!session_.ice_ufrag.empty() && !session_.ice_pwd.empty()) {
        oss << "a=ice-ufrag:" << session_.ice_ufrag << "\r\n";
        oss << "a=ice-pwd:" << session_.ice_pwd << "\r\n";
    }
    if (session_.iceLite)
        oss << "a=ice-lite\r\n";
    for (const auto &it : session_.ice_options) {
        oss << "a=ice-options:" << it << "\r\n";
    }
    // 会话级别扩展属性
    for (auto attr : session_.attributes) {
        oss << "a=" << attr.first << "=" << attr.second << "\r\n";
    }
    // 媒体块
    for (const auto &media : media_) {
        oss << "m=" << (media.mediaType == MediaDescription::video ? "video" : "audio") << " " << media.port << " "
            << transportProtoToString(media.proto);
        for (const auto &p : media.payloads)
            oss << " " << static_cast<int>(p.payload);
        oss << "\r\n";
        // TCP属性
        if (media.proto == TransportProtocol::TCP) {
            oss << "a=setup:" << setupTypeToString(media.tcpInfo.setup) << "\r\n"
                << "a=connection:" << (media.tcpInfo.newConnection ? "new" : "existing") << "\r\n";
        }
        // 方向属性
        oss << "a=";
        switch (media.direction) {
            case Direction::SENDRECV: oss << "sendrecv"; break;
            case Direction::SENDONLY: oss << "sendonly"; break;
            case Direction::RECVONLY: oss << "recvonly"; break;
            case Direction::INACTIVE: oss << "inactive"; break;
        }
        oss << "\r\n";
        // Payload描述
        for (const auto &p : media.payloads) {
            if (!p.name.empty())
                oss << "a=rtpmap:" << static_cast<int>(p.payload) << " " << p.name << "/" << p.sample_rate << "\r\n";
        }
        // 关闭rtcp
        if (!media.rtcp) {
            oss << "a=rtcp:no\r\n";
        }
        // ICE信息
        if (!media.ice.ufrag.empty())
            oss << "a=ice-ufrag:" << media.ice.ufrag << "\r\n";
        if (!media.ice.pwd.empty())
            oss << "a=ice-pwd:" << media.ice.pwd << "\r\n";
        for (const auto &candidate : media.ice.candidates)
            oss << candidate;
        oss << "a=streamnumber:" << media.stream_number << "\r\n";
        oss << "a=streamprofile:" << media.stream_number << "\r\n";
        oss << "a=stream:" << media.stream_number << "\r\n";
        if (media.stream_number == 0 || media.stream_number == 1) {
            oss << "a=streamMode:" << (media.stream_number == 0 ? "Main" : "Sub") << "\r\n";
        }
        if (media.file_size) {
            oss << "a=filesize:" << media.file_size << "\r\n";
        }
        if (media.download_speed) {
            oss << "a=downloadspeed:" << media.download_speed << "\r\n";
        }
        for (const auto &attr : media.attributes) {
            oss << "a=" << attr.first << "=" << attr.second << "\r\n";
        }
        // 国标扩展
        if (media.ssrc)
            oss << "y=" << std::setw(10) << std::setfill('0') << media.ssrc << "\r\n";
        if (!media.f_param.empty())
            oss << "f=" << media.f_param << "\r\n";
    }
    return oss.str();
}
} // namespace gb28181
/**********************************************************************************************************
文件名称:   sdp.cpp
创建时间:   25-2-19 上午10:47
作者名称:   Kevin
文件路径:   src/request
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-19 上午10:47

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-19 上午10:47       描述:   创建文件

**********************************************************************************************************/