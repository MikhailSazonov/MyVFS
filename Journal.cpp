#include "Journal.hpp"

using namespace TestTask;

void Journal::AddTask(Concurrency::Node<Task>* new_node)
{
    tasks_.Push(new_node);
}

Concurrency::Node<Task>* Journal::ExtractTask()
{
    return tasks_.TryPop();
}

void Journal::MarkDone(Task* task)
{
    task->task_finished_.store(true, std::memory_order_release);
}
