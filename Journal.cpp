#include "Journal.hpp"

using namespace TestTask;

Journal::~Journal()
{
    while (freelist_tail_)
    {
        auto to_delete = freelist_tail_;
        freelist_tail_ = freelist_tail_->next_;
        delete to_delete;
    }
}

Task* Journal::AddTask(File* f, TaskType type, std::variant<char*, const char*> buf, size_t len)
{
    Concurrency::Node<Task*>* new_node = new Concurrency::Node<Task*>();
    new_node->data_ = new Task();
    (*new_node->data_)->f_ = f;
    (*new_node->data_)->type_ = type;
    (*new_node->data_)->buf_ = buf;
    (*new_node->data_)->len_ = len;
    (*new_node->data_)->version_ = version_.load(std::memory_order_acquire);
    queue_.Push(new_node);
    return *new_node->data_;
}

Task* Journal::ExtractTask()
{
    auto* next_task = queue_.TryPop();
    if (next_task == nullptr)
    {
        return nullptr;
    } 
    if (freelist_tail_ == nullptr)
    {
        freelist_tail_ = next_task;
    }
    else
    {
        freelist_tail_->next_ = next_task;
    }
    version_.fetch_add(1, std::memory_order_release);
    Clear();
    return *next_task->data_;
}

void Journal::Clear()
{
    auto current_version = version_.load(std::memory_order_relaxed);
    while (freelist_tail_ && freelist_tail_->next_ &&
            (*freelist_tail_->next_.load(std::memory_order_relaxed)->data_)->version_ == current_version &&
            (*freelist_tail_->data_)->status_.load(std::memory_order_acquire) == TaskStatus::COPIED)
    {
        auto to_delete = freelist_tail_;
        freelist_tail_ = freelist_tail_->next_;
        delete *to_delete->data_;
        delete to_delete;
    }
}

void Journal::MarkDone(Task* task)
{
    task->status_.store(TaskStatus::FINISHED, std::memory_order_release);
}

size_t Journal::WaitForTask(Task* task)
{
    while (task->status_.load(std::memory_order_acquire) == TaskStatus::INIT)
    {
        Concurrency::Spin();
    }
    size_t res = task->task_result_;
    task->status_.store(TaskStatus::COPIED, std::memory_order_release);
    return res;
}
