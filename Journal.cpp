#include "Journal.hpp"

using namespace TestTask;

using namespace std::chrono_literals;

Journal::~Journal()
{
    while (freelist_tail_)
    {
        auto to_delete = freelist_tail_;
        freelist_tail_ = freelist_tail_->next_;
        if (to_delete == &queue_.dummy_)
        {
            continue;
        }
        delete *to_delete->data_;
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
    auto* curr_tail = queue_.tail_.load(std::memory_order_acquire);
    if (curr_tail != &queue_.dummy_)
    {
        (*curr_tail->data_)->contestants_.fetch_add(1, std::memory_order_release);
    }
    queue_.Push(new_node);
    inserted_.fetch_add(1, std::memory_order_release);
    return *new_node->data_;
}

Task* Journal::ExtractTask()
{
    auto* next_task = queue_.TryPop();
    if (next_task == nullptr)
    {
        return nullptr;
    }
    // Clear();
    return *next_task->data_;
}

void Journal::Clear()
{
    while (freelist_tail_->next_ &&
            (freelist_tail_ == &queue_.dummy_ ||
            ((*freelist_tail_->data_)->contestants_.load(std::memory_order_acquire) + addition_ == inserted_.load(std::memory_order_acquire) &&
            (*freelist_tail_->data_)->status_.load(std::memory_order_acquire) == TaskStatus::COPIED)))
    {
        auto to_delete = freelist_tail_;
        freelist_tail_ = freelist_tail_->next_;
        if (to_delete == &queue_.dummy_)
        {
            continue;
        }
        addition_ += (*to_delete->data_)->contestants_.load(std::memory_order_acquire);
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
    auto start = std::chrono::system_clock::now();
    while (task->status_.load(std::memory_order_acquire) == TaskStatus::INIT)
    {
        // ad-hoc
        if (std::chrono::system_clock::now() - start > 100ms)
        {
            break;
        }
        Concurrency::Spin();
    }
    size_t res = task->task_result_;
    task->status_.store(TaskStatus::COPIED, std::memory_order_release);
    return res;
}
