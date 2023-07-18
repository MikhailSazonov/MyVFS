#pragma once

#include "Node.hpp"

#include <iostream>

namespace TestTask::Concurrency
{
    /*
        Лок-фри очередь в виде интрузивного списка
    */

    template <typename T>
    class MSQueue
    {
        public:
            Node<T>* TryPop()
            {
                while (true)
                {
                    Node<T>* curr_head = head_.load(std::memory_order_acquire);
                    if (curr_head->next_.load(std::memory_order_acquire) == nullptr)
                    {
                        return nullptr;
                    }
                    Node<T>* next = curr_head->next_.load(std::memory_order_acquire);
                    if (head_.compare_exchange_weak(curr_head, next, std::memory_order_release))
                    {
                        return next;
                    }
                }
            }

            void Push(Node<T>* new_node)
            {
                Node<T>* curr_tail;

                while (true)
                {
                    curr_tail = tail_.load(std::memory_order_acquire);

                    auto* next = curr_tail->next_.load(std::memory_order_acquire);
                    if (next != nullptr)
                    {
                        // Помогаем другому потоку, который (возможно) находится в промежуточном состоянии
                        tail_.compare_exchange_weak(curr_tail, next, std::memory_order_release);
                        continue;
                    }

                    Node<T>* null_node = nullptr;
                    if (curr_tail->next_.compare_exchange_weak(null_node, new_node, std::memory_order_release))
                    {
                        break;
                    }
                }

                tail_.compare_exchange_strong(curr_tail, new_node, std::memory_order_release);
            }

        private:
            Node<T> dummy_;
            std::atomic<Node<T>*> head_{&dummy_};
            std::atomic<Node<T>*> tail_{&dummy_};
    };
}
