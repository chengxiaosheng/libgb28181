#include "Poller/EventPoller.h"
#include "sip-timer.h"
#include "sip_common.h"

#include <gb28181/sip_event.h>

using namespace toolkit;

struct sip_timer_context {
    std::weak_ptr<EventPoller::DelayTask> task; // 任务
    std::shared_ptr<toolkit::EventPoller> poller; // 任务所属线程
    std::atomic_flag handle_flag{ATOMIC_FLAG_INIT}; // 是否已经执行
    std::mutex mutex;
};


static std::atomic_bool timer_running{false};

void sip_timer_init(void) {
    InfoL << "sip_timer_init";
    timer_running.exchange(true);
}
void sip_timer_cleanup(void) {
    InfoL << "sip_timer_cleanup";
    timer_running.exchange(false);
}

sip_timer_t sip_timer_start(int timeout, sip_timer_handle handler, void* usrptr) {
    auto poller = EventPollerPool::Instance().getPoller();

    auto context = new sip_timer_context();
    context->poller = poller;
    context->task = poller->doDelayTask(timeout, [handler, usrptr, context]() {
        if (!timer_running.load()) {
            return 0;
        }
        TraceL << "handle timer " << context;
        // 自旋控制，避免重复执行
        std::unique_lock<std::mutex> lock(context->mutex);
        if(!context->handle_flag.test_and_set()) {
            handler(usrptr);
        }
        return 0;
    });
    TraceL << "add timer " << context << ", timeout=" << timeout << " ms";
    return context;
}

int sip_timer_stop(sip_timer_t* id) {
    if (nullptr == id || nullptr == *id) {
        return -1;
    }
    TraceL << "stop timer " << *id;
    auto context = static_cast<sip_timer_context *>(*id);
    if (context == nullptr) {
        *id = nullptr;
        return -1;
    }
    bool flag = false;
    {
        std::unique_lock<std::mutex> lock(context->mutex);
        flag = context->handle_flag.test_and_set();
    }
    // 切换到延时任务所在线程去取消任务
    context->poller->async_first([task_weak = context->task]() {
        if (auto task = task_weak.lock()) {
            task->cancel();
        }
    });
    delete context;
    *id = nullptr;
    return flag ? -1 : 0;
}


/**********************************************************************************************************
文件名称:   timer_impl.cpp
创建时间:   25-2-5 上午10:14
作者名称:   Kevin
文件路径:   src
功能描述:   ${MY_FILE_DESCRIPTION}
修订时间:   25-2-5 上午10:14

修订记录
-----------------------------------------------------------------------------------------------------------
1. Kevin       25-2-5 上午10:14       描述:   创建文件

**********************************************************************************************************/