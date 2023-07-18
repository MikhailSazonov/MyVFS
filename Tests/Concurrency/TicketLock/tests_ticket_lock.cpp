#include <ThreadPool.hpp>
#include <RWTicketLock.hpp>
#include <Guard.hpp>
#include <test_utils.hpp>

#include <chrono>
#include <iostream>

using namespace Executors;
using namespace TestTask::Concurrency;

using namespace std::literals::chrono_literals;

int main() {
    section("ticket_lock");
    test([&]() {
        ThreadPool tp(2);
        RWTicketLock lock;
        tp.Start();
        tp.Submit([&]() {
            lock.AcquireRead();
            std::this_thread::sleep_for(200ms);
            lock.Release();
        });
        tp.Submit([&]() {
            std::this_thread::sleep_for(100ms);
            lock.AcquireWrite();
            lock.Release();
        });
        tp.WaitIdle();
        tp.Stop();
    }, "Just works");
    test([&]() {
        ThreadPool tp(3);
        RWTicketLock lock;
        std::atomic<int> dummy = 0;
        tp.Start();
        tp.Submit([&]() {
            lock.AcquireRead();
            std::this_thread::sleep_for(100ms);
            dummy.fetch_add(1, std::memory_order_release);
            lock.Release();
        });
        tp.Submit([&]() {
            lock.AcquireRead();
            std::this_thread::sleep_for(100ms);
            dummy.fetch_add(1, std::memory_order_release);
            lock.Release();
        });
        tp.Submit([&]() {
            lock.AcquireRead();
            std::this_thread::sleep_for(100ms);
            dummy.fetch_add(1, std::memory_order_release);
            lock.Release();
        });
        std::this_thread::sleep_for(200ms);
        assert_equal(dummy.load(std::memory_order_acquire), 3);
        tp.Stop();
    }, "Simultaneous reads");
    test([&]() {
        ThreadPool tp(4);
        RWTicketLock lock;
        int dummy = 0;
        tp.Start();
        tp.Submit([&]() {
            lock.AcquireWrite();
            std::this_thread::sleep_for(100ms);
            ++dummy;
            lock.Release();
        });
        tp.Submit([&]() {
            lock.AcquireWrite();
            std::this_thread::sleep_for(100ms);
            ++dummy;
            lock.Release();
        });
        tp.Submit([&]() {
            lock.AcquireWrite();
            std::this_thread::sleep_for(100ms);
            ++dummy;
            lock.Release();
        });
        tp.Submit([&]() {
            std::this_thread::sleep_for(200ms);
            lock.AcquireRead();
            assert_equal(dummy, 3);
            lock.Release();
        });
        tp.WaitIdle();
        tp.Stop();
    }, "Writes");
    test([&]() {
        ThreadPool tp(4);
        RWTicketLock lock;
        std::atomic<int> dummy = 0;
        tp.Start();
        for (int i = 0; i < 100; ++i)
        {
            tp.Submit([&]() {
                for (int i = 0; i < 100; ++i)
                {
                    lock.AcquireWrite();
                    dummy.store(dummy.load() + 1);
                    lock.Release();
                }
            });
        }
        for (int i = 0; i < 1000; ++i)
        {
            tp.Submit([&]() {
                for (int i = 0; i < 1000; ++i)
                {
                    lock.AcquireRead();
                    dummy.fetch_add(1, std::memory_order_release);
                    lock.Release();
                }
            });
        }
        tp.WaitIdle();
        assert_equal(dummy.load(std::memory_order_acquire), 1010000);
        tp.Stop();
    }, "Summation");
        test([&]() {
        ThreadPool tp(4);
        RWTicketLock lock;
        Guard<int> g(0);
        tp.Start();
        for (int i = 0; i < 10; ++i)
        {
            tp.Submit([&]() {
                for (int i = 0; i < 10; ++i)
                {
                    WriteGuard wg(g);
                    int& gd = wg.get();
                    ++gd;
                    assert_equal(gd, 1);
                    --gd;
                }
            });
        }
        for (int i = 0; i < 100; ++i)
        {
            tp.Submit([&]() {
                for (int i = 0; i < 100; ++i)
                {
                    ReadGuard rg(g);
                    const int& gd = rg.get();
                    assert_equal(gd, 0);
                }
            });
        }
        tp.WaitIdle();
        ReadGuard rg(g);
        const int& gd = rg.get();
        assert_equal(gd, 0);
        tp.Stop();
    }, "Guards");
}
