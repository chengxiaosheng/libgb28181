
set(SDK_HEADER
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/cstring.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/darray.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/uuid.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/uri-parse.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/cpm/param.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/sys/atomic.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include/sys/locker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include/http-parser.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include/http-reason.h
)
set(SDK_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/source/http-parser.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/source/http-reason.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/source/uri-parse.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/source/uuid.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/source/darray.c
)
add_library(ireader_sdk_lib STATIC ${SDK_HEADER} ${SDK_SRC})
target_include_directories(ireader_sdk_lib PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include)
if (WIN32)
    target_compile_definitions(ireader_sdk_lib PUBLIC -DOS_WINDOWS)
endif ()


set(SIP_SRC

        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/sip-agent.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/sip-dialog.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/sip-message.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/sip-reason.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/sip-subscribe.c

        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-contact.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-cseq.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-param.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-route.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-substate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-uri.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/header/sip-header-via.c

        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-ack.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-bye.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-cancel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-info.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-invite.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-options.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-register.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-subscribe.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-transaction.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-transaction-invite.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uac/sip-uac-transaction-noninvite.c

        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-bye.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-cancel.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-info.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-options.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-prack.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-refer.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-register.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-subscribe.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-transaction.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-transaction-invite.c
        ${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src/uas/sip-uas-transaction-noninvite.c
)

add_library(sip STATIC ${SIP_SRC})
target_link_libraries(sip PUBLIC ireader_sdk_lib)
target_compile_definitions(sip PUBLIC -DSIP_REGISTER_WITH_USERINFO) # 基于GB/T 28181-2016 附录J 信令消息示范 J.1 注册信令消息示范 中描述 "J.1.1 Registersip:SIP服务器编码@目的域名或IP地址端口SIP/2.0" 注册头包含 SIP服务器编码
target_compile_definitions(sip PUBLIC -DUDP_PACKET_SIZE=20480)
target_include_directories(sip
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include>
)