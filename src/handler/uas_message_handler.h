#ifndef gb28181_src_handler_UAS_MESSAGE_HANDLER_H
#define gb28181_src_handler_UAS_MESSAGE_HANDLER_H

namespace gb28181 {
class UasMessageHandler {
public:
  static UasMessageHandler &Instance();
  ~UasMessageHandler();


private:
  UasMessageHandler();
};
}

class uas_message_handler {

};



#endif //gb28181_src_handler_UAS_MESSAGE_HANDLER_H



/**********************************************************************************************************
文件名称:   uas_message_handler.h
创建时间:   25-2-7 下午6:53
作者名称:   Kevin
文件路径:   src/handler
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-7 下午6:53

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-7 下午6:53       描述:   创建文件

**********************************************************************************************************/