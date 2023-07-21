#pragma once

#include "File.hpp"

#include "Concurrency/MSQueue/MSQueue.hpp"
#include "Concurrency/TicketLock/Spin.hpp"

#include <atomic>
#include <variant>
#include <deque>

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
    };

    /*
        Класс для отслеживания лайфтаймов задач

        Основная идея - "версия" записи в журнале.
        Версия увеличивается каждый раз, когда из журнала читается следующая запись.
        При этом можно стирать все записи, которые были с меньшей версией (т.к. потоки, которые
        их писали, уже вышли из MSQueue, и можно безопасно удалять запись)
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
            Concurrency::Node<Task*>* freelist_tail_{nullptr};
            std::atomic<uint32_t> version_{0};
            Concurrency::MSQueue<Task*> queue_;
    };
}
