#pragma once

#include "File.hpp"

#include "Concurrency/MSQueue/MSQueue.hpp"

#include <atomic>
#include <variant>

namespace TestTask
{
    enum class TaskType
    {
        WRITE, READ
    };

    struct Task
    {
        File* f_;
        TaskType type_;

        std::variant<char*, const char*> buf_;
        size_t len_;

        size_t task_result_{0};
        std::atomic<bool> task_finished_{false};
    };

    class Journal
    {
        public:
            void AddTask(Concurrency::Node<Task>*);

            Concurrency::Node<Task>* ExtractTask();

            void MarkDone(Task*);

        private:
            Concurrency::MSQueue<Task> tasks_;
    };
}
