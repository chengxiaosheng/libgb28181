#ifndef gb28181_include_gb28181_SIP_COMMON_H
#define gb28181_include_gb28181_SIP_COMMON_H

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


}

#endif //gb28181_include_gb28181_SIP_COMMON_H



/**********************************************************************************************************
文件名称:   sip_common.h
创建时间:   25-2-5 下午1:51
作者名称:   Kevin
文件路径:   include/gb28181
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 下午1:51

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 下午1:51       描述:   创建文件

**********************************************************************************************************/