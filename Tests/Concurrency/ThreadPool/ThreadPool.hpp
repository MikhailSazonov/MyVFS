#pragma once

#include "MPMCQueue.hpp"
#include "Task.hpp"

#include <thread>
#include <atomic>
#include <vector>
#include <cstdlib>
#include <cassert>

#include <condition_variable>
#include <mutex>

namespace Executors
{
    class ThreadPool
    {
    public:
        ThreadPool(size_t);

        ~ThreadPool();

        void Start();

        void Submit(Task);

        static ThreadPool *Current();

        void WaitIdle();

        void Stop();

    private:
        void Process();

    private:
        size_t threads_;
        std::vector<std::thread> workers_;

        MPMCQueue<Task> task_queue_;
        std::atomic<uint32_t> tasks_count_{0};

        std::condition_variable wake_if_no_tasks_;
        std::mutex mutex_;
        std::atomic<bool> stop_processing_tasks_{false};
    };

}
