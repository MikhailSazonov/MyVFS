#pragma once

#include "File.hpp"

#include "Concurrency/MSQueue/MSQueue.hpp"
#include "Concurrency/TicketLock/Spin.hpp"

#include <atomic>
#include <variant>
#include <deque>
#include <chrono>

namespace TestTask
{
    enum class TaskType
    {
        WRITE, READ
    };

    enum class TaskStatus
    {
        INIT, FINISHED, COPIED
    };

    struct Task
    {
        File* f_;
        TaskType type_;

        std::variant<char*, const char*> buf_;
        size_t len_;

        size_t task_result_{0};
        std::atomic<TaskStatus> status_{TaskStatus::INIT};

        uint32_t version_;

        std::atomic<uint32_t> contestants_{0};
    };

    /*
        Класс для отслеживания лайфтаймов задач
    */

    class Journal
    {
        public:
            ~Journal();

            Task* AddTask(File* f = nullptr,
                        TaskType type = TaskType::READ,
                        std::variant<char*, const char*> = (char*)nullptr,
                        size_t len = 0);

            Task* ExtractTask();

            void MarkDone(Task*);

            size_t WaitForTask(Task*);

        private:
            void Clear();

        private:
            std::atomic<uint32_t> inserted_;
            uint32_t addition_;
            Concurrency::MSQueue<Task*> queue_;
            Concurrency::Node<Task*>* freelist_tail_{&queue_.dummy_};
    };
}
