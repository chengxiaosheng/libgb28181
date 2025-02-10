#include "gb28181/local_server.h"
#include <iostream>

#include <csignal>
#include <thread>

// 全局变量，用于检测是否收到退出信号
std::atomic<bool> is_running{true};
// 信号处理函数
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nSignal received, shutting down...\n";
        is_running = false; // 设置退出标志
    }
}

int main() {
    gb28181::local_account account;
    {
        account.platform_id = "65010100002000100001";
        account.domain = "6501010000";
        account.name = "test platform";
        account.host = "::";
        account.password = "123456";
        account.port = 5060;
        account.auth_type = gb28181::SipAuthType::digest;
        account.allow_auto_register = true;
    };
    auto local_server = gb28181::LocalServer::new_local_server(account);
    local_server->set_new_subordinate_account_callback([](std::shared_ptr<gb28181::LocalServer> server, std::shared_ptr<gb28181::subordinate_account> account, std::function<void(bool)> allow_cb) {
        allow_cb(true);
    });
    local_server->run();

    // 捕获 SIGINT 和 SIGTERM 信号
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 主线程保持运行直到收到退出信号
    std::cout << "Press Ctrl+C to exit..." << std::endl;
    while (is_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 防止忙等
    }

    std::cout << "Server stopped." << std::endl;
    return 0;
}
