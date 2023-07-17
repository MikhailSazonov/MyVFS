#pragma once

#include <atomic>

namespace TestTask::Concurrency
{
    template <typename T>
    struct LockFreeNode
    {
        T data_;
        std::atomic<LockFreeNode<T>*> next_node_;
    };
}