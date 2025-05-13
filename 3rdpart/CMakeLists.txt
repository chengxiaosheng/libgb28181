set(LIB_SIP_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/source/http-parser.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/source/http-reason.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/source/uri-parse.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/source/uuid.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/source/darray.c

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

add_library(ireader_sip STATIC ${LIB_SIP_SRC})
target_include_directories(ireader_sip
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/include>
        $<INSTALL_INTERFACE:include>

        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/media-server/libsip/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ireader-sdk/libhttp/include>
)
target_link_libraries(ireader_sip PUBLIC ${PROJECT_NAME}_interface)
find_package(Threads)
target_link_libraries(ireader_sip PUBLIC Threads::Threads)
if (WIN32)
    target_compile_definitions(ireader_sip PUBLIC -DOS_WINDOWS)
endif ()
