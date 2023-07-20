#include "RWTicketLock.hpp"

using namespace TestTask::Concurrency;

void TestTask::Concurrency::SpinThenYield(uint32_t& i)
{
    if (i++ % (1<<13) != 0)
    {
        Spin();
    }
    else
    {
        std::this_thread::yield();
    }
}

void RWTicketLock::AcquireRead() {
    uint32_t my_ticket = acquire_.fetch_add(1, std::memory_order_acq_rel);
    uint32_t i = 0;
    while (enter_.load(std::memory_order_acquire) != my_ticket) {
        SpinThenYield(i);
    }
    current_state_.store(Detail::State::READ, std::memory_order_relaxed);
    enter_.fetch_add(1, std::memory_order_release);
}

void RWTicketLock::AcquireWrite() {
    uint32_t my_ticket = acquire_.fetch_add(1, std::memory_order_acq_rel);
    uint32_t i = 0;
    while (left_.load(std::memory_order_acquire) != my_ticket) {
        SpinThenYield(i);
    }
    current_state_.store(Detail::State::WRITE, std::memory_order_relaxed);
}

void RWTicketLock::Release() {
    if (current_state_.load(std::memory_order_relaxed) == Detail::State::READ) {
        left_.fetch_add(1, std::memory_order_release);
    } else {
        left_.fetch_add(1, std::memory_order_release);
        enter_.fetch_add(1, std::memory_order_release);
    }
}
