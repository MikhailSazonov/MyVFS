#pragma once

#include "Spin.hpp"

#include <atomic>

/*
    Мы можем использовать shared_mutex + shared_lock + unique_lock из STL,
    чтоы получить стандартный RW-Lock.
    
    К сожалению, стандарт не гарантирует отсутствие голодания (starvation)
    у пишущего потока (https://en.cppreference.com/w/cpp/thread/shared_mutex)

    Поэтому мы пишем свой ticket-lock, с гарантией того, что голодания не будет
*/

namespace TestTask::Concurrency::Detail {
    enum class State : uint32_t {
        READ, WRITE
    };
}

namespace TestTask::Concurrency {
    class RWTicketLock {
        public:
            Token AcquireRead();

            Token AcquireWrite();

            void Relase(const Detail::Token&);

        private:
            std::atomic<uint32_t> current_state_{0};
            std::atomic<uint32_t> acquire_{0};
            std::atomic<uint32_t> enter_{0};
            std::atomic<uint32_t> left_{0};
    }
}
