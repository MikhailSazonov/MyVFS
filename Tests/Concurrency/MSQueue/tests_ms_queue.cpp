#include <ThreadPool.hpp>
#include <MSQueue.hpp>
#include <Node.hpp>
#include <test_utils.hpp>

#include <chrono>
#include <iostream>

using namespace Executors;
using namespace TestTask::Concurrency;

using namespace std::literals::chrono_literals;

int main() {
    section("lockfree_list");
    test([&]() {
        MSQueue<int> list;
        Node<int> n1{1};
        Node<int> n2{2};
        Node<int> n3{3};
        list.Push(&n1);
        list.Push(&n2);
        auto* node = list.TryPop();
        assert(node != nullptr);
        assert_equal(*node->data_, 1);
        node = list.TryPop();
        assert(node != nullptr);
        assert_equal(*node->data_, 2);
        node = list.TryPop();
        assert(node == nullptr);
        list.Push(&n3);
        node = list.TryPop();
        assert(node != nullptr);
        assert_equal(*node->data_, 3);
    }, "One thread");
    test([&]() {
        ThreadPool tp(10);
        MSQueue<int> list;
        tp.Start();
        std::vector<Node<int>> vn(10000);
        for (int i = 0; i < 10000; ++i)
        {
            vn[i].data_ = 1;
        }
        for (int i = 0; i < 10; ++i)
        {
            tp.Submit([&, i]() {
                for (int j = 0; j < 1000; ++j)
                {
                    std::this_thread::sleep_for(1ms);
                    list.Push(&vn[i * 1000 + j]);
                }
            });
        }
        int collected = 0;
        auto clock_start = std::chrono::system_clock::now();
        while (collected < 10000)
        {
            if (std::chrono::system_clock::now() - clock_start > 2s)
            {
                std::cerr << "Time limit exceeded.\n";
                assert(false);
            }
            auto* node = list.TryPop();
            if (node == nullptr)
            {
                continue;
            }
            collected += *node->data_;
        }
        tp.WaitIdle();
        tp.Stop();
    }, "Multi-push");
    test([&]() {
        ThreadPool tp(10);
        MSQueue<int> list;
        tp.Start();
        std::vector<Node<int>> vn(5000);
        for (int i = 0; i < 5000; ++i)
        {
            vn[i].data_ = 1;
        }
        for (int i = 0; i < 5; ++i)
        {
            tp.Submit([&, i]() {
                for (int j = 0; j < 1000; ++j)
                {
                    std::this_thread::sleep_for(1ms);
                    list.Push(&vn[i * 1000 + j]);
                }
            });
        }
        std::atomic<int> collected{0};
        for (int i = 0; i < 5; ++i)
        {
            while (collected.load() < 5000)
            {
                auto* node = list.TryPop();
                if (node == nullptr)
                {
                    continue;
                }
                collected.fetch_add(*node->data_);
            }
        }
        tp.WaitIdle();
        tp.Stop();
    }, "Multi-push+pop");
}
