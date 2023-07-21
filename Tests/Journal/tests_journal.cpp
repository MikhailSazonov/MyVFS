#include <Journal.hpp>
#include <ThreadPool.hpp>

#include <test_utils.hpp>
#include <cstring>

#include <chrono>
#include <iostream>

using namespace Executors;
using namespace TestTask::Cache;
using namespace TestTask::Concurrency;

using namespace std::literals::chrono_literals;

int main() {
    section("journal");
    test([&]() {
        TestTask::Journal j;
        j.AddTask();
        auto* e = j.ExtractTask();
        assert(e != nullptr);
        j.MarkDone(e);
    }, "Just works");

    test([&]() {
        ThreadPool tp(5);
        TestTask::Journal j;

        for (int i = 0; i < 5; ++i)
        {
            tp.Submit([&]() mutable {
                for (int k = 0; k < 500; ++k)
                {
                    auto* t = j.AddTask();
                    assert(j.WaitForTask(t) == 5);
                }
            });
        }
        
        int cnt = 0;
        while (cnt < 2500)
        {
            auto* e = j.ExtractTask();
            if (e == nullptr)
            {
                continue;
            }
            e->task_result_ = 5;
            j.MarkDone(e);
            ++cnt;
        }

        tp.Stop();
    }, "Submits");
}
