#include "gb28181/message/message_base.h"

#include <Util/logger.h>
#include <gb28181/type_define_ext.h>

using namespace gb28181;

MessageBase::MessageBase(const std::shared_ptr<tinyxml2::XMLDocument> &xml)
    : xml_ptr_(xml) {
}

bool MessageBase::load_from_xml() {
    if (!xml_ptr_ || !xml_ptr_->RootElement())
        return false;
    const auto &root = xml_ptr_->RootElement();
    root_ = getRootType(root->Name());
    if (root_ == MessageRootType::invalid)
        return false;
    if (auto cmd_ele = root->FirstChildElement("CmdType")) {
        cmd_ = getCmdType(cmd_ele->GetText());
    }
    if (auto ele = root->FirstChildElement("SN")) {
        sn_ = ele->IntText(0);
    }
    if (auto ele = root->FirstChildElement("DeviceID")) {
        device_id_ = ele->GetText();
    }
    if (auto ele = root->FirstChildElement("Reason")) {
        reason_ = ele->GetText();
    }
    if (encoding_ == CharEncodingType::invalid) {
        if (auto decl = xml_ptr_->FirstChild(); decl && decl->ToDeclaration()) {
            if (auto enc = getCharEncodingType(decl->ToDeclaration()->Value()); enc != CharEncodingType::invalid) {
                encoding_ = enc;
            }
        }
    }
    is_valid_ = load_detail();
    return  is_valid_;
}

bool MessageBase::parse_to_xml(bool coercion) {
    if (coercion) xml_ptr_.reset();
    if (xml_ptr_) return true;
    if (root_ == MessageRootType::invalid) {
        WarnL << "message root type is invalid";
        return false;
    }
    if (cmd_ == MessageCmdType::invalid) {
        WarnL << "command type is invalid";
        return false;
    }
    xml_ptr_ = std::make_shared<tinyxml2::XMLDocument>();
    switch (encoding_) {
        case CharEncodingType::gb2312: xml_ptr_->InsertFirstChild(xml_ptr_->NewDeclaration(R"(xml version="1.0" encoding="GB2312")")); break;
        case CharEncodingType::gbk: xml_ptr_->InsertFirstChild(xml_ptr_->NewDeclaration(R"(xml version="1.0" encoding="GBK")")); break;
        default: xml_ptr_->InsertFirstChild(xml_ptr_->NewDeclaration(R"(xml version="1.0" encoding="UTF-8")")); break;
    }
    auto root = xml_ptr_->NewElement(getRootTypeString(root_));
    xml_ptr_->InsertEndChild(root);
    auto ele = root->InsertNewChildElement("CmdType");
    ele->SetText(getCmdTypeString(cmd_));
    ele = root->InsertNewChildElement("SN");
    ele->SetText(sn_);
    if (device_id_ && !device_id_->empty()) {
        ele = root->InsertNewChildElement("DeviceID");
        ele->SetText(device_id_->c_str());
    }
    if (!reason_.empty()) {
        ele = root->InsertNewChildElement("Reason");
        ele->SetText(reason_.c_str());
    }
    if (!parse_detail()) {
        xml_ptr_.reset();
        return false;
    }
    load_extend_data();
    return true;
}

std::string MessageBase::str() const {
    if (!xml_ptr_) return "";
    tinyxml2::XMLPrinter xml_printer;
    xml_ptr_->Print(&xml_printer);
    std::string str;
    if (encoding_ == CharEncodingType::gb2312) {
        str = utf8_to_gb2312(xml_printer.CStr());
    } else if (encoding_ == CharEncodingType::gbk) {
        str = utf8_to_gbk(xml_printer.CStr());
    } else str = xml_printer.CStr();
    // 我觉得此函数只会执行一次， 没有存储到本地的必要~
    if (str.size() > 8*1024 - 500) {
        payload_too_big();
    }
    return str;
}


void MessageBase::extend(std::vector<ExtendData> &&data) {
    for (auto &&it : data) {
        extend_data_.emplace_back(std::move(it));
    }
}
void MessageBase::append_extend(ExtendData &&data) {
    extend_data_.emplace_back(std::move(data));
}

void append_extend_to_xml(tinyxml2::XMLElement *root, const ExtendData &data) { // NOLINT(*-no-recursion)
    // 验证参数合法性
    if (!root || data.key.empty()) {
        WarnL << "Invalid root or empty key in ExtendData";
        return;
    }
    // 创建新子节点
    auto *ele = root->InsertNewChildElement(data.key.c_str());
    if (!ele) {
        WarnL << "Failed to create new child element for key: " << data.key;
        return;
    }
    // 设置节点的文本内容
    if (!data.value.empty()) {
        ele->SetText(data.value.c_str());
    }
    // 递归处理子节点
    for (const auto &child : data.children) {
        append_extend_to_xml(ele, child);
    }
}

void MessageBase::load_extend_data() {
    if (!xml_ptr_ || !xml_ptr_->RootElement()) {
        WarnL << "XML document pointer or root pointer is null";
        return;
    }
    auto root = xml_ptr_->RootElement();
    for (auto &it : extend_data_) {
        append_extend_to_xml(root, it);
    }
}
std::ostream &gb28181::operator<<(std::ostream &os, const MessageBase &msg) {
    os << "[" <<  msg.root_ << "->" << msg.cmd_ << ":" << msg.sn_ << "] " ;
    return os;
}

/**********************************************************************************************************
文件名称:   message_base.cpp
创建时间:   25-2-8 下午12:34
作者名称:   Kevin
文件路径:   src/message
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-8 下午12:34

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-8 下午12:34       描述:   创建文件

**********************************************************************************************************/
