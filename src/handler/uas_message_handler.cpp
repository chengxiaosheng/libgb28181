#include "uas_message_handler.h"

#include "tinyxml2.h"

#include <Util/NoticeCenter.h>
#include <Util/util.h>
#include <gb28181/message/message_base.h>
#include <gb28181/sip_common.h>
#include <gb28181/type_define_ext.h>
#include <inner/sip_event.h>
#include <inner/sip_server.h>
#include <inner/sip_session.h>
#include <sip-message.h>
#include <sip-uas.h>

using namespace gb28181;
using namespace toolkit;

INSTANCE_IMP(UasMessageHandler)


static int on_message( EventOnMessageArgs) {
    auto sip_server = session->getSipServer();
    if (req->payload == nullptr) {
        WarnL << "SIP message payload is null or empty";
        sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    auto xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
    if (xml_ptr->Parse((const char *)req->payload, req->size) != tinyxml2::XML_SUCCESS) {
        WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")" << xml_ptr->ErrorStr() << ", xml = " << std::string_view((const char *)req->payload, req->size);
        sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    assert(sip_server);
    auto local_server = sip_server->get_server();
    assert(local_server);

    MessageBase message(xml_ptr);
    if (!message.load_from_xml()) {
        ErrorL << "SIP message load failed: " << message.get_error() << "xml = " << std::string_view((const char *)req->payload, req->size);
        sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
    }
    std::string convert_xml_str;
    switch (message.encoding()) {
        case CharEncodingType::gbk:
            convert_xml_str = gbk_to_utf8((const char *)req->payload);
            break;
        case CharEncodingType::gb2312:
            convert_xml_str = gb2312_to_utf8((const char *)req->payload);
            break;
        default: break;
    }
    if (!convert_xml_str.empty()) {
        xml_ptr = std::make_shared<tinyxml2::XMLDocument>();
        if (xml_ptr->Parse(convert_xml_str.c_str(),convert_xml_str.size()) != tinyxml2::XML_SUCCESS) {
            WarnL << "XML parse error (" << xml_ptr->ErrorID() << ":" << xml_ptr->ErrorName() << ")" << xml_ptr->ErrorStr() << ", xml = " << convert_xml_str;
            sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
        message = MessageBase(xml_ptr);
        if (!message.load_from_xml()) {
            ErrorL << "SIP message load failed: " << message.get_error() <<  ", xml = " << convert_xml_str;
            sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
    }

    switch (message.command()) {
        case MessageCmdType::DeviceControl: break;
        case MessageCmdType::DeviceConfig: break;
        case MessageCmdType::DeviceStatus: break;
        case MessageCmdType::Catalog: break;
        case MessageCmdType::DeviceInfo: break;
        case MessageCmdType::RecordInfo: break;
        case MessageCmdType::Alarm: break;
        case MessageCmdType::ConfigDownload: break;
        case MessageCmdType::PresetQuery: break;
        case MessageCmdType::MobilePosition: break;
        case MessageCmdType::HomePositionQuery: break;
        case MessageCmdType::CruiseTrackListQuery: break;
        case MessageCmdType::CruiseTrackQuery: break;
        case MessageCmdType::PTZPosition: break;
        case MessageCmdType::SDCardStatus: break;
        case MessageCmdType::Keepalive: break;
        case MessageCmdType::MediaStatus: break;
        case MessageCmdType::Broadcast: break;
        case MessageCmdType::UploadSnapShotFinished: break;
        case MessageCmdType::VideoUploadNotify: break;
        case MessageCmdType::DeviceUpgradeResult: break;
        default: {
            WarnL << "Unknown message command: " << message.command();
            sip_uas_reply(transaction.get(), 400, nullptr, 0, session.get());
        }
    }
    return sip_ok;
}



UasMessageHandler::UasMessageHandler() {
    NoticeCenter::Instance().addListener(this, kEventOnRequest, on_message);
}

UasMessageHandler::~UasMessageHandler() {
    NoticeCenter::Instance().delListener(this, kEventOnRequest);
}



/**********************************************************************************************************
文件名称:   uas_message_handler.cpp
创建时间:   25-2-7 下午6:53
作者名称:   Kevin
文件路径:   src/handler
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午6:53

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午6:53       描述:   创建文件

**********************************************************************************************************/