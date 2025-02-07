#ifndef gb28181_src_inner_SIP_SESSION_H
#define gb28181_src_inner_SIP_SESSION_H

#include "Network/Session.h"
#ifdef __cplusplus
extern "C" {
  struct http_parser_t;
  struct sip_agent_t;
}
#endif

namespace gb28181 {
class SipServer;

class SipSession : public toolkit::Session {
public:
  SipSession(const toolkit::Socket::Ptr &sock);
  ~SipSession() override;

  void onRecv(const toolkit::Buffer::Ptr &) override;
  void onError(const toolkit::SockException &err) override;
  void onManager() override;
  inline bool is_udp() const {
    return _is_udp;
  }
  void startConnect(const std::string &host, uint16_t port, uint16_t local_port, const std::string &local_ip, const std::function<void(const toolkit::SockException &ex)> &cb, float timeout_sec);

  inline std::shared_ptr<SipServer> getSipServer() const {
     return _sip_server.lock();
  }

  inline void set_on_manager(std::function<void()> cb) {
    _on_manager = std::move(cb);
  }
  inline void set_on_error(std::function<void(const toolkit::SockException &ex)> cb) {
    _on_error = std::move(cb);
  }

protected:
  void onSockConnect(const toolkit::SockException &ex);


private:
  bool _is_udp = false;
  int8_t _wait_type{0};
  toolkit::Ticker _ticker;
  struct sockaddr_storage _addr{};
  std::shared_ptr<http_parser_t> _sip_parse;
  sip_agent_t * _sip_agent{};
  std::weak_ptr<SipServer> _sip_server;
  std::shared_ptr<toolkit::Timer> _timer;
  friend class SipServer;

  std::function<void()> _on_manager;
  std::function<void(const toolkit::SockException &)> _on_error;
};

}



#endif //gb28181_src_inner_SIP_SESSION_H



/**********************************************************************************************************
文件名称:   sip_session.h
创建时间:   25-2-5 上午10:56
作者名称:   Kevin
文件路径:   src/inner
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 上午10:56

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 上午10:56       描述:   创建文件

**********************************************************************************************************/