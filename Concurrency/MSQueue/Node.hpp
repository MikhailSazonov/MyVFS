#pragma once

#include <atomic>
#include <optional>

namespace TestTask::Concurrency
{
    template <typename T>
    struct Node
    {
        std::optional<T> data_;
        std::atomic<Node<T>*> next_{nullptr};
    };
}