[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/chengxiaosheng/libgb28181)

## 项目介绍
libgb28181是一个完整的GB/T 28181-2022国标协议栈实现，支持视频监控平台的级联和设备接入。

## GB/T 28181 支持情况
+ 工作环境
- [x] 本地作为上级
- [x] 本地作为下级
- [x] 本地同时作为上级下级

+ 消息支持
- [x] 支持基于GB/T 28181-2022的全部消息
- [x] [subordinate_platform](include/gb28181/subordinate_platform.h) 提供了简单的消息查询与发送, 也可以基于 [request_proxy](include/gb28181/request/request_proxy.h) 自行组装并发送消息
- [x] 本地作为下级时通过 [request_proxy](include/gb28181/request/request_proxy.h) 发送消息, 通过订阅 [sip_event](include/gb28181/sip_event.h) 中的事件监听来自上级的请求
- [x] 订阅采用的通用实现


## 项目依赖
+ [ireader/media-server/libsip](https://github.com/ireader/media-server/tree/master/libsip) - 提供 sip 协议栈支持
+ [ZLMediaKit/ZLToolKit](https://github.com/ZLMediaKit/ZLToolKit) - 提供通讯、线程、日志等支持
+ [tinyxml2](https://github.com/leethomason/tinyxml2) - 提供xml生成与解析等支持
+ [iconv](https://savannah.gnu.org/projects/libiconv) - 提供字符编码转换支持

## 贡献指南
欢迎提交Issue和Pull Request来改进本项目。

## 许可证
本项目采用开源许可证，具体请查看LICENSE文件。