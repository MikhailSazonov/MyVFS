#pragma once

namespace TestTask::Concurrency
{
    /*
        Лок-фри очередь в виде интрузивного списка
    */

    template <typename T>
    class LockFreeList
    {
        public:
            // Может вызываться многими потоками
            void AddNode(LockFreeNode<T>* new_node)
            {
                auto prev_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
                prev_tail->next_node_ = new_node;
                if (head_.load(std::memory_order_acquire) == nullptr)
                {
                    head_.store(new_node, std::memory_order_release);
                }
            }

            // Всегда вызывается одним потоком
            LockFreeNode<T>* RemoveNode()
            {
                auto cur_head = head_.load(std::memory_order_acquire);
                if (cur_head == nullptr)
                {
                    return nullptr;
                }
                head_.store(cur_head->next_node_, std::memory_order_release);
                return cur_head;
            }

        private:
            std::atomic<LockFreeNode<T>*> head_;
            std::atomic<LockFreeNode<T>*> tail_;
    };
}
