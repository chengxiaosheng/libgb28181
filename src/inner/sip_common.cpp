#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif
#include "gb28181/type_define.h"
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include "sip-message.h"
#include "sip_common.h"

#include "tinyxml2.h"

#include <Util/logger.h>
#include <sip-uac.h>
#include <sip-uas.h>

#include <Util/util.h>
#include <Util/MD5.h>
#include <Network/sockutil.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

namespace gb28181 {

void set_message_agent(struct sip_uac_transaction_t *transaction) {
    sip_uac_add_header(transaction, SIP_HEADER_USER_AGENT_K, SIP_AGENT_STR);
}
void set_message_gbt_version(struct sip_uac_transaction_t *transaction, PlatformVersionType version) {
    switch (version) {
        case PlatformVersionType::v10: sip_uac_add_header(transaction, SIP_HEADER_X_GB_VERSION, "1.0"); break;
        case PlatformVersionType::v11: sip_uac_add_header(transaction, SIP_HEADER_X_GB_VERSION, "1.1"); break;
        case PlatformVersionType::v20: sip_uac_add_header(transaction, SIP_HEADER_X_GB_VERSION, "2.0"); break;
        case PlatformVersionType::v30: sip_uac_add_header(transaction, SIP_HEADER_X_GB_VERSION, "3.0"); break;
        default: void(); break;
    }
}
void set_message_header(struct sip_uac_transaction_t *transaction) {
    set_message_agent(transaction);
    set_message_gbt_version(transaction);
}
void set_message_contact(struct sip_uac_transaction_t *transaction, const char *contact) {
    sip_uac_add_header(transaction, SIP_HEADER_CONTACT, contact);
}
void set_message_content_type(struct sip_uac_transaction_t *transaction, enum SipContentType content_type) {
    if (content_type == SipContentType_XML) {
        sip_uac_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_XML);
    } else if (content_type == SipContentType_SDP) {
        sip_uac_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_SDP);
    } else if (content_type == SipContentType_MANSRTSP) {
        sip_uac_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_MANSRTSP);
    }
}
void set_message_reason(struct sip_uac_transaction_t *transaction, const char *reason) {
    sip_uac_add_header(transaction, SIP_HEADER_REASON, reason);
}
void set_x_preferred_path(struct sip_uac_transaction_t *transaction, const char *path) {
    sip_uac_add_header(transaction, SIP_HEADER_X_PREFERRED_PATH, path);
}
void set_x_preferred_path(struct sip_uac_transaction_t *transaction, const std::vector<std::string> &path) {
    std::stringstream ss;
    for (auto it = path.begin(); it != path.end(); ++it) {
        if (it != path.begin()) {
            ss << "-";
        }
        ss << *it;
    }
    auto data = ss.str();
    if (!data.empty()) {
        sip_uac_add_header(transaction, SIP_HEADER_X_PREFERRED_PATH, data.c_str());
    }
}

void set_message_authorization(struct sip_uac_transaction_t *transaction, const std::string &authorization) {
    if (!authorization.empty()) {
        sip_uac_add_header(transaction, SIP_HEADER_AUTHORIZATION, authorization.c_str());
    }
}
void set_invite_subject(struct sip_uac_transaction_t *transaction, const char *subject) {
    if (!transaction)
        return;
    sip_uac_add_header(transaction, SIP_HEADER_SUBJECT, subject);
}

void set_message_agent(struct sip_uas_transaction_t *transaction) {
    sip_uas_add_header(transaction, SIP_HEADER_USER_AGENT_K, SIP_AGENT_STR);
}

void set_message_gbt_version(struct sip_uas_transaction_t *transaction, PlatformVersionType version) {
    switch (version) {
        case PlatformVersionType::v10: sip_uas_add_header(transaction, SIP_HEADER_X_GB_VERSION, "1.0"); break;
        case PlatformVersionType::v11: sip_uas_add_header(transaction, SIP_HEADER_X_GB_VERSION, "1.1"); break;
        case PlatformVersionType::v20: sip_uas_add_header(transaction, SIP_HEADER_X_GB_VERSION, "2.0"); break;
        case PlatformVersionType::v30: sip_uas_add_header(transaction, SIP_HEADER_X_GB_VERSION, "3.0"); break;
        default: void(); break;
    }
}

void set_message_date(struct sip_uas_transaction_t *transaction) {
    sip_uas_add_header(transaction, SIP_HEADER_DATE, toolkit::getTimeStr("%Y-%m-%dT%H:%M:%S").c_str());
}
void set_message_expires(struct sip_uas_transaction_t *transaction, int expires) {
    sip_uas_add_header_int(transaction, SIP_HEADER_EXPIRES, expires);
}

void set_message_header(struct sip_uas_transaction_t *transaction) {
    set_message_agent(transaction);
    set_message_gbt_version(transaction);
}
void set_message_reason(struct sip_uas_transaction_t *transaction, const char *reason) {
    sip_uas_add_header(transaction, SIP_HEADER_REASON, reason);
}
void set_x_route_path(struct sip_uas_transaction_t *transaction, const char *local, const char *sub) {
    std::string path(local);
    if (sub) {
        path.append("-").append(sub);
    }
    sip_uas_add_header(transaction, SIP_HEADER_X_ROUTE_PATH, path.c_str());
}
void set_x_route_path(struct sip_uas_transaction_t *transaction, const std::vector<std::string> &path) {
    std::stringstream ss;
    for (auto it = path.begin(); it != path.end(); ++it) {
        if (it != path.begin()) {
            ss << "-";
        }
        ss << *it;
    }
    auto data = ss.str();
    if (!data.empty()) {
        sip_uas_add_header(transaction, SIP_HEADER_X_ROUTE_PATH, data.c_str());
    }
}
void set_message_content_type(struct sip_uas_transaction_t *transaction, enum SipContentType content_type) {
    if (content_type == SipContentType_XML) {
        sip_uas_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_XML);
    } else if (content_type == SipContentType_SDP) {
        sip_uas_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_SDP);
    } else if (content_type == SipContentType_MANSRTSP) {
        sip_uas_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_MANSRTSP);
    }
}

PlatformVersionType get_message_gbt_version(const struct sip_message_t *msg) {
    if (!msg)
        return PlatformVersionType::unknown;
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_X_GB_VERSION);
    if (hv && hv->n >= 3) {
        if (hv->p[0] == '1' && hv->p[1] == '.' && hv->p[2] == '0')
            return PlatformVersionType::v10;
        if (hv->p[0] == '1' && hv->p[1] == '.' && hv->p[2] == '1')
            return PlatformVersionType::v11;
        if (hv->p[0] == '2' && hv->p[1] == '.' && hv->p[2] == '0')
            return PlatformVersionType::v20;
        if (hv->p[0] == '3' && hv->p[1] == '.' && hv->p[2] == '0')
            return PlatformVersionType::v30;
    }
    return PlatformVersionType::unknown;
}
std::string get_platform_id(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    cstring_t str {};
    sip_uri_username(&msg->from.uri, &str);
    if (cstrvalid(&str)) {
        return std::string(str.p, str.n);
    }
    return "";
}
std::string get_from_uri(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    std::string str(128, '\0');
    if (auto len = sip_uri_write(&msg->from.uri, str.data(), str.data() + 128)) {
        str.resize(len);
        return str;
    }
    return "";
}

std::string get_to_uri(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    std::string str(128, '\0');
    if (auto len = sip_uri_write(&msg->to.uri, str.data(), str.data() + 128)) {
        str.resize(len);
        return str;
    }
    return "";
}

std::string get_message_reason(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_REASON);
    if (hv && cstrvalid(hv)) {
        return std::string(hv->p, hv->n);
    }
    return "";
}
std::vector<std::string> get_x_preferred_path(const struct sip_message_t *msg) {
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_X_PREFERRED_PATH);
    if (hv && cstrvalid(hv)) {
        return toolkit::split(std::string(hv->p, hv->n), "-");
    }
    return {};
}
std::vector<std::string> get_x_route_path(const struct sip_message_t *msg) {
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_X_ROUTE_PATH);
    if (hv && cstrvalid(hv)) {
        return toolkit::split(std::string(hv->p, hv->n), "-");
    }
    return {};
}
int get_expires(const struct sip_message_t *msg) {
    if (!msg)
        return 0;
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_EXPIRES);
    if (hv && cstrvalid(hv)) {
        int expires = 0;
        tinyxml2::XMLUtil::ToInt(hv->p, &expires);
        return expires;
    }
    return 0;
}
std::string get_message_contact(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_CONTACT);
    if (cstrvalid(hv)) {
        return std::string(hv->p, hv->n);
    }
    return "";
}
std::pair<std::string, uint32_t> get_via_rport(const struct sip_message_t *msg) {
    if (!msg)
        return std::make_pair("", 0);
    if (auto via = sip_vias_get(&msg->vias, 0)) {
        std::string host = cstrvalid(&via->received) ? std::string(via->received.p, via->received.n) : "";
        if (via->rport <= 0) {
            return std::make_pair(host, 0);
        } else {
            return std::make_pair(host, via->rport);
        }
    }
    return std::make_pair("", 0);
}
std::string get_invite_subject(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    auto hv = sip_message_get_header_by_name(msg, SIP_HEADER_SUBJECT);
    if (cstrvalid(hv)) {
        return std::string(hv->p, hv->n);
    }
    return "";
}
std::string get_invite_device_id(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    cstring_t str {};
    if (0 == sip_uri_username(&msg->u.c.uri, &str) && cstrvalid(&str)) {
        return std::string(str.p, str.n);
    }
    if (0 == sip_uri_username(&msg->to.uri, &str) && cstrvalid(&str)) {
        return std::string(str.p, str.n);
    }
    return "";
}

std::unordered_map<std::string, std::string> parseAuthorizationHeader(const std::string &authHeader) {
    std::unordered_map<std::string, std::string> fields;
    size_t start = authHeader.find("Digest");
    if (start == std::string::npos) {
        return fields;
    }
    std::istringstream iss(authHeader.substr(start + 7)); // 跳过 "Digest"
    std::string keyValue;
    while (std::getline(iss, keyValue, ',')) {
        size_t pos = keyValue.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = keyValue.substr(0, pos);
        std::string value = keyValue.substr(pos + 1);

        // 去掉引号
        if (!value.empty() && value[0] == '"')
            value = value.substr(1, value.size() - 2);

        // 去掉前后空格
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);

        fields[key] = value;
    }

    return fields;
}

#if ENABLE_OPENSSL
std::string md5(const std::string &data) {
    std::vector<unsigned char> hash(16);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, data.c_str(), data.size());
    MD5_Final(hash.data(), &ctx);
    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}
std::string sha256(const std::string &data) {
    std::vector<unsigned char> hash(32);

    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(hash.data(), &ctx);
    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}

#endif

bool verify_authorization(struct sip_message_t *msg, const std::string &user, const std::string &password) {
    auto auth_str = sip_message_get_header_by_name(msg, SIP_HEADER_AUTHORIZATION);
    if (!auth_str || !cstrvalid(auth_str))
        return false;
    auto &&fields = parseAuthorizationHeader(std::string(auth_str->p, auth_str->n));
    if (fields.empty())
        return false;
    std::string algorithm = fields.count("algorithm") ? fields["algorithm"] : "MD5";
    auto H = (algorithm == "MD5") ? md5 : sha256;

    std::string A1 = user + ":" + fields["realm"] + ":" + password;
    std::string A2 = std::string(msg->u.c.method.p, msg->u.c.method.n) + ":" + fields["uri"];
    std::string qop = fields.count("qop") ? fields["qop"] : "";
    if (qop == "auth-int") {
        if (msg->payload && msg->size) {
            A2 += ":" + H(std::string((char *)msg->payload, msg->size));
        } else {
            A2 += ":";
        }
    }
    std::string response;
    if (qop.empty()) {
        response = H(H(A1) + ":" + fields["nonce"] + ":" + H(A2));
    } else {
        response = H(H(A1) + ":" + fields["nonce"] + ":" + fields["nc"] + ":" + fields["cnonce"] + ":" + qop + ":" + H(A2));
    }
    return response == fields["response"];
}
std::string generate_www_authentication_(const std::string &realm) {
    std::string nonce = toolkit::makeRandStr(16);
    std::ostringstream oss;
    oss << "Digest realm=\"" << realm << "\", ";
    // oss << "qop=auth, ";
    oss << "nonce=\"" << nonce << "\"";
    // oss << "algorithm=MD5";
    return oss.str();
}
void set_message_www_authenticate(struct sip_uas_transaction_t *transaction, const std::string &realm) {
    sip_uas_add_header(transaction, SIP_HEADER_WWW_AUTHENTICATE, generate_www_authentication_(realm).c_str());
}
std::string get_www_authenticate(const struct sip_message_t *msg) {
    if (!msg)
        return "";
    auto auth_str = sip_message_get_header_by_name(msg, SIP_HEADER_WWW_AUTHENTICATE);
    if (cstrvalid(auth_str)) {
        return std::string(auth_str->p, auth_str->n);
    }
    return "";
}
std::string generate_authorization(
    const struct sip_message_t *msg, const std::string &username, const std::string &password, const std::string &uri,
    std::pair<std::string, int> &nc_pair) {
    std::string www_authorization = get_www_authenticate(msg);
    if (www_authorization.empty())
        return "";
    auto &&fields = parseAuthorizationHeader(www_authorization);
    if (fields.empty())
        return "";
    // 校验必要参数
    if (!fields.count("realm") || !fields.count("nonce")) {
        return "";
    }
    // 提取认证参数
    const std::string &realm = fields["realm"];
    const std::string &nonce = fields["nonce"];
    const std::string &qop = fields.count("qop") ? fields["qop"] : "";
    const std::string &algorithm = fields.count("algorithm") ? fields["algorithm"] : "MD5";

    if (nc_pair.first == nonce) {
        nc_pair.second++;
        if (nc_pair.second >= 0xFFFFFFFF) {
            nc_pair.second = 1;
        }
    } else {
        nc_pair.first = nonce;
        nc_pair.second = 1;
    }
    std::stringstream nc_ss;
    nc_ss << std::setw(8) << std::setfill('0') << std::hex << nc_pair.second;
    std::string nc_str = nc_ss.str();

    // 选择哈希算法
    auto H = (algorithm == "MD5") ? md5 : sha256;
    // 生成客户端随机数
    std::string cnonce = toolkit::makeRandStr(16);
    // 计算H(A1)
    std::string A1 = username + ":" + realm + ":" + password;
    std::string HA1 = H(A1);
    // 计算H(A2)
    std::string A2 = "REGISTER:" + uri;
    if (qop == "auth-int") {
        if (msg->payload && msg->size) {
            A2 += ":" + H(std::string((char *)msg->payload, msg->size));
        } else {
            A2 += ":";
        }
    }
    std::string HA2 = H(A2);
    // 生成响应值
    std::string response;
    if (qop.empty()) {
        response = H(HA1 + ":" + nonce + ":" + HA2);
    } else {
        response = H(HA1 + ":" + nonce + ":" + nc_str + ":" + cnonce + ":" + qop + ":" + HA2);
    }
    // 构建Authorization头
    std::ostringstream oss;
    oss << "Digest "
        << "username=\"" << username << "\", "
        << "realm=\"" << realm << "\", "
        << "nonce=\"" << nonce << "\", "
        << "uri=\"" << uri << "\", "
        << "response=\"" << response << "\", "
        << "algorithm=" << algorithm;
    if (!qop.empty()) {
        oss << ", qop=" << qop << ", "
            << "nc=" << nc_str << ", "
            << "cnonce=\"" << cnonce << "\"";
    }
    return oss.str();
}

bool is_loopback_ip(const char *ip){
    if(strcasecmp(ip, "::") == 0 || strcasecmp(ip, "0.0.0.0") == 0) return true;
    // 尝试解析为 IPv4 地址
    struct in_addr ipv4_addr;
    if (inet_pton(AF_INET, ip, &ipv4_addr) == 1) {
        // 检查是否为 IPv4 环回地址（127.0.0.0/8）
        uint32_t ip_addr = ntohl(ipv4_addr.s_addr);
        return (ip_addr >= 0x7F000001 && ip_addr <= 0x7FFFFFFF);
    }

    // 尝试解析为 IPv6 地址
    struct in6_addr ipv6_addr;
    if (inet_pton(AF_INET6, ip, &ipv6_addr) == 1) {
        // 检查是否为 IPv6 环回地址（::1）
        static const uint8_t ipv6_loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        return memcmp(&ipv6_addr, ipv6_loopback, 16) == 0;
    }

    // 不是有效的 IPv4 或 IPv6 地址
    return false;
}

} // namespace gb28181

/**********************************************************************************************************
文件名称:   sip_common.cpp
创建时间:   25-2-7 下午1:14
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午1:14

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午1:14       描述:   创建文件

**********************************************************************************************************/