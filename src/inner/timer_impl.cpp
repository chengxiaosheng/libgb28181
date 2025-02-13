#include "Poller/EventPoller.h"
#include "sip-timer.h"

#include <gb28181/sip_common.h>

using namespace toolkit;

static std::unordered_map<uint32_t, std::weak_ptr<EventPoller::DelayTask>> timer_map_;
static std::mutex timer_mutex_;
static uint32_t timer_index_ = 1;

void sip_timer_init(void) {

}
void sip_timer_cleanup(void) {
    std::lock_guard<decltype(timer_mutex_)> lck(timer_mutex_);
    for (auto& [id, task] : timer_map_) {
        if (auto shared_task = task.lock()) {
            shared_task->cancel();
        }
    }
    timer_map_.clear();
}

sip_timer_t sip_timer_start(int timeout, sip_timer_handle handler, void* usrptr) {
    auto poller = EventPollerPool::Instance().getPoller();
    std::lock_guard<decltype(timer_mutex_)> lck(timer_mutex_);
    auto index = timer_index_++;
    if (timer_index_ == UINT32_MAX) {
        timer_index_ = 1;
    }
    timer_map_[index] = poller->doDelayTask(timeout, [handler, usrptr, index]() {
        handler(usrptr);
        std::lock_guard<decltype(timer_mutex_)> lck(timer_mutex_);
        timer_map_.erase(index);
        return 0;
    });
    return (void *)(uintptr_t)index;
}

int sip_timer_stop(sip_timer_t* id) {
    if (!id) return gb28181::sip_not_found;
    auto timer_id = (uint32_t)(uintptr_t)id;
    if (timer_id) {
        std::lock_guard<decltype(timer_mutex_)> lck(timer_mutex_);
        auto it = timer_map_.find(timer_id);
        if (it != timer_map_.end()) {
            if (auto task = it->second.lock()) {
                task->cancel();
            }
            timer_map_.erase(it);
            *id = nullptr;
            return gb28181::sip_ok;
        }
    }
    *id = nullptr;
    return gb28181::sip_not_found;
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