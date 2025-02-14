#define WIN32_LEAN_AND_MEAN
#include "gb28181/type_define.h"
#include <iomanip>
#include <sstream>
#include <unordered_map>

#include "sip-message.h"
#include "sip_common.h"

#include <algorithm>
#include <sip-uac.h>
#include <sip-uas.h>

#include <Util/util.h>
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
void set_message_content_type(struct sip_uac_transaction_t* transaction, enum SipContentType content_type) {
    if (content_type == SipContentType_XML) {
        sip_uac_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_XML);
    } else if (content_type == SipContentType_SDP) {
        sip_uac_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_SDP);
    }
}
void set_message_reason(struct sip_uac_transaction_t* transaction, const char* reason) {
    sip_uac_add_header(transaction, SIP_HEADER_REASON, reason);
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

void set_message_date(struct sip_uas_transaction_t * transaction) {
    sip_uas_add_header(transaction, SIP_HEADER_DATE, toolkit::getTimeStr("%Y-%m-%dT%H:%M:%S").c_str());
}
void set_message_expires(struct sip_uas_transaction_t * transaction, int expires) {
    sip_uas_add_header_int(transaction, SIP_HEADER_EXPIRES, expires);
}

void set_message_header(struct sip_uas_transaction_t *transaction) {
    set_message_agent(transaction);
    set_message_gbt_version(transaction);
}
void set_message_reason(struct sip_uas_transaction_t* transaction, const char* reason) {
    sip_uas_add_header(transaction, SIP_HEADER_REASON, reason);
}
void set_message_content_type(struct sip_uas_transaction_t *transaction, enum SipContentType content_type) {
    if (content_type == SipContentType_XML) {
        sip_uas_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_XML);
    } else if (content_type == SipContentType_SDP) {
        sip_uas_add_header(transaction, SIP_HEADER_CONTENT_TYPE, SIP_CONTENT_TYPE_SDP);
    }
}

PlatformVersionType get_message_gbt_version(struct sip_message_t *msg) {
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
std::string get_platform_id(struct sip_message_t *msg) {
    if (!msg)
        return "";
    cstring_t str{};
    sip_uri_username(&msg->from.uri, &str);
    if (cstrvalid(&str)) {
        return std::string(str.p, str.n);
    }
    return "";
}
std::string get_from_uri(struct sip_message_t* msg) {
    if (!msg)
        return "";
    std::string str(128, '\0');
    if (auto len = sip_uri_write(&msg->from.uri, str.data(), str.data() + 128)) {
        str.resize(len);
        return str;
    }
    return "";
}

std::string get_to_uri(struct sip_message_t* msg) {
    if (!msg)
        return "";
    std::string str(128, '\0');
    if (auto len = sip_uri_write(&msg->to.uri, str.data(), str.data() + 128)) {
        str.resize(len);
        return str;
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

    auto H = fields["algorithm"] == "MD5" ? md5 : sha256;

    auto A1 = user + ":" + fields["realm"] + ":" + password;

    std::string A2 = std::string(msg->u.c.method.p, msg->u.c.method.n) + ":" + fields["uri"];
    auto qop = fields["qop"];
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
        response
            = H(H(A1) + ":" + fields["nonce"] + ":" + fields["nc"] + ":" + fields["cnonce"] + ":" + qop + ":" + H(A2));
    }
    return response == fields["response"];
}
std::string generate_www_authentication_(const std::string &realm) {
    std::string nonce = toolkit::makeRandStr(16);
    std::ostringstream oss;
    oss << "Digest realm=\"" << realm << "\", ";
    oss << "qop=\"auth\", ";
    oss << "nonce=\"" << nonce << "\", ";
    oss << "algorithm=\"MD5\"";
    return oss.str();
}
void set_message_www_authenticate(struct sip_uas_transaction_t *transaction, const std::string &realm) {
    sip_uas_add_header(transaction, SIP_HEADER_WWW_AUTHENTICATE, generate_www_authentication_(realm).c_str());
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