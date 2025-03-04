#ifndef gb28181_src_inner_SIP_COMMON_H
#define gb28181_src_inner_SIP_COMMON_H

#define SIP_HEADER_USER_AGENT_K "User-Agent"
#define SIP_HEADER_AUTHORIZATION "Authorization"
#define SIP_HEADER_WWW_AUTHENTICATE "WWW-Authenticate"
#define SIP_HEADER_X_GB_VERSION "X-GB-Ver" // GB/T 28181-2022 -> 附录J 协议版本标识
#define SIP_HEADER_DATE "Date"
#define SIP_HEADER_EXPIRES "Expires"
#define SIP_HEADER_CONTENT_TYPE "Content-Type"
#define SIP_HEADER_REASON "Reason"
#define SIP_CONTENT_TYPE_XML "Application/MANSCDP+xml"
#define SIP_CONTENT_TYPE_SDP "Application/sdp"
#define SIP_CONTENT_TYPE_MANSRTSP "Application/MANSRTSP"
#define SIP_HEADER_X_ROUTE_PATH "X-RoutePath"
#define SIP_HEADER_X_PREFERRED_PATH "X-PreferredPath"
#define SIP_HEADER_SUBJECT "Subject"

#include <gb28181/type_define.h>

#ifdef __cplusplus
extern "C" {
struct sip_uas_transaction_t;
struct sip_uac_transaction_t;
struct sip_message_t;
}
#endif

namespace gb28181 {

enum SipError : int {
    sip_undefined_error = -1, // 未定义的错误
    sip_bad_parameter = -2, // 参数错误
    sip_wrong_state = -3, // 状态~？
    sip_syntax_error = -5, // 语法错误
    sip_not_found = -6, // 未找到
    sip_not_implemented = -7, // 为实现
    sip_no_network= -10, // 没有网络连接
    sip_port_busy = -11, // 端口被占用或忙碌
    sip_unknown_host = -12, // 未知的主机
    sip_timeout = -50, // 超时
    sip_too_much_call = -51, // call 过多？
    sip_wrong_format = -52, // 格式问题
    sip_retry_limit= -60, // 重试超过限制
    sip_ok = 0, // 无错误
};

enum SipContentType {
    SipContentType_XML,
    SipContentType_SDP,
    SipContentType_MANSRTSP,
};

void set_message_agent(struct sip_uac_transaction_t *transaction);
void set_message_gbt_version(
    struct sip_uac_transaction_t *transaction, PlatformVersionType version = PlatformVersionType::v30);
void set_message_header(struct sip_uac_transaction_t *transaction);
void set_message_content_type(struct sip_uac_transaction_t *transaction, enum SipContentType content_type);
void set_message_reason(struct sip_uac_transaction_t *transaction, const char *reason);
void set_x_preferred_path(struct sip_uac_transaction_t *transaction, const char *path);
void set_x_preferred_path(struct sip_uac_transaction_t *transaction, const std::vector<std::string> &path);
void set_message_authorization(struct sip_uac_transaction_t *transaction, const std::string &authorization);
void set_invite_subject(struct sip_uac_transaction_t *transaction, const char *subject);

void set_message_agent(struct sip_uas_transaction_t *transaction);
void set_message_gbt_version(
    struct sip_uas_transaction_t *transaction, PlatformVersionType version = PlatformVersionType::v30);
void set_message_date(struct sip_uas_transaction_t *transaction);
void set_message_expires(struct sip_uas_transaction_t *transaction, int expires);
void set_message_header(struct sip_uas_transaction_t *transaction);
void set_message_content_type(struct sip_uas_transaction_t *transaction, enum SipContentType content_type);
void set_message_reason(struct sip_uas_transaction_t *transaction, const char *reason);
void set_x_route_path(struct sip_uas_transaction_t *transaction, const char *local, const char *sub);
void set_x_route_path(struct sip_uas_transaction_t *transaction, const std::vector<std::string> &path);

PlatformVersionType get_message_gbt_version(const struct sip_message_t *msg);
std::string get_platform_id(const struct sip_message_t *msg);
std::string get_from_uri(const struct sip_message_t *msg);
std::string get_to_uri(const struct sip_message_t *msg);
std::string get_message_reason(const struct sip_message_t *transaction);
std::vector<std::string> get_x_preferred_path(const struct sip_message_t *msg);
std::vector<std::string> get_x_route_path(const struct sip_message_t *msg);
int get_expires(const struct sip_message_t *msg);
std::string get_message_contact(const struct sip_message_t *msg);
std::pair<std::string,uint32_t> get_via_rport(const struct sip_message_t *msg);
std::string get_invite_subject(const struct sip_message_t *msg);
std::string get_invite_device_id(const struct sip_message_t *msg);


bool verify_authorization(struct sip_message_t *msg, const std::string &user, const std::string &password);
std::string generate_www_authentication_(const std::string &realm);
void set_message_www_authenticate(struct sip_uas_transaction_t *transaction, const std::string &realm);
std::string get_www_authenticate(const struct sip_message_t *msg);
std::string generate_authorization(const struct sip_message_t *msg, const std::string &username, const std::string &password, const std::string &uri, std::pair<std::string,int> &nc_pair);

} // namespace gb28181

#endif // gb28181_src_inner_SIP_COMMON_H

/**********************************************************************************************************
文件名称:   sip_common.h
创建时间:   25-2-7 下午1:14
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午1:14

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午1:14       描述:   创建文件

**********************************************************************************************************/