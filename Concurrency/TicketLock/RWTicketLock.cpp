#include "RWTicketLock.hpp"

using namespace TestTask::Concurrency;

void RWTicketLock::AcquireRead() {
    uint32_t my_ticket = acquire_.fetch_add(1, std::memory_order_acq_rel);
    while (enter_.load(std::memory_order_acquire) != my_ticket) {
        Spin();
    }
    current_state_.store(Detail::State::READ, std::memory_order_relaxed);
    enter_.fetch_add(1, std::memory_order_release);
}

void RWTicketLock::AcquireWrite() {
    uint32_t my_ticket = acquire_.fetch_add(1, std::memory_order_acq_rel);
    while (left_.load(std::memory_order_acquire) != my_ticket) {
        Spin();
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
