#include <CacheManager.hpp>
#include <ThreadPool.hpp>

#include <test_utils.hpp>
#include <cstring>

#include <chrono>
#include <iostream>

using namespace Executors;
using namespace TestTask;
using namespace TestTask::Cache;
using namespace TestTask::Concurrency;

using namespace std::literals::chrono_literals;

namespace TestTask
{
    struct TestGenerator
    {
        static File* CreateFile()
        {
            return new File();
        }
    };
}

std::string CreateRandomString(int len)
{
    std::string str;
    str.resize(len);
    for (int i = 0; i < len; ++i)
    {
        str[i] = rand() % 26 + 65;        
    }
    return str;
}

int main() {
    section("cache");
    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager cache(10, 0.75, g);
        auto& um = g.WriteAccess();
        um["ab"] = TestGenerator::CreateFile();
        um["ac"] = TestGenerator::CreateFile();
        um["ad"] = TestGenerator::CreateFile();
        g.ReturnAccess();

        char buf[100] = {0};
        char ans[100] = {0};

        ssize_t rd = cache.Read(um["ab"], ans, 100);
        assert_equal(rd, -1);

        strcpy(buf, "ABCDE");
        cache.Write(um["ab"], buf, 5);

        rd = cache.Read(um["ab"], ans, 100);
        assert_equal(rd, 5);
        assert_equal(strcmp(ans, "ABCDE"), 0);

        cache.Write(um["ac"], buf, 5);
        cache.Write(um["ad"], buf, 5);

        std::this_thread::sleep_for(500ms);

        rd = cache.Read(um["ab"], ans, 100);
        assert_equal(rd, -1);

    }, "Just works");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager cache(100, 0.75, g);
        auto& um = g.WriteAccess();
        um["a"] = TestGenerator::CreateFile();
        um["b"] = TestGenerator::CreateFile();
        um["c"] = TestGenerator::CreateFile();
        g.ReturnAccess();

        ThreadPool tp(6);
        tp.Start();

        std::atomic<uint32_t> write_to_file{0};

        for (int i = 0; i < 3; ++i)
        {
            tp.Submit([&](){
                for (size_t j = 0; j < 16000; ++j)
                {
                    std::string random_str = CreateRandomString(10);
                    int rand_idx = random() % 3;
                    std::string a = "a";
                    a[0] += rand_idx;
                    cache.Write(um[a], random_str.c_str(), 10);
                    write_to_file.fetch_or(1 << rand_idx);
                }
            });
        }

        for (int i = 0; i < 3; ++i)
        {
            tp.Submit([&](){
                for (size_t j = 0; j < 64000; ++j)
                {
                    int rand_idx = random() % 3;
                    if ((write_to_file.load() & (1 << rand_idx)) > 0)
                    {
                        char buf[10];
                        std::string a = "a";
                        a[0] += rand_idx;
                        ssize_t rd = cache.Read(um[a], buf, 10);
                        assert(rd == 10);
                    }
                }
            });
        }

        tp.WaitIdle();
        tp.Stop();

    }, "Massive writes");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager cache(60, 0.75, g);
        auto& um = g.WriteAccess();
        um["a"] = TestGenerator::CreateFile();
        um["b"] = TestGenerator::CreateFile();
        um["c"] = TestGenerator::CreateFile();
        um["d"] = TestGenerator::CreateFile();
        um["e"] = TestGenerator::CreateFile();
        g.ReturnAccess();

        ThreadPool tp(10);
        tp.Start();

        for (int i = 0; i < 10; ++i)
        {
            tp.Submit([&](){
                for (size_t j = 0; j < 100000; ++j)
                {
                    int rand_idx = random() % 5;
                    std::string a = "a";
                    a[0] += rand_idx;

                    char buf[30];
                    ssize_t rd = cache.Read(um[a], buf, 20);

                    if (rd == -1)
                    {
                        std::string str;
                        for (int i = 0; i < 20; ++i)
                        {
                            str.push_back('a' + rand_idx);
                        }
                        cache.Write(um[a], str.c_str(), 20);
                    }
                    else
                    {
                        for (int i = 0; i < 20; ++i)
                        {
                            assert(buf[i] == 'a' + rand_idx);
                        }
                    }
                }
            });
        }

        tp.WaitIdle();
        tp.Stop();

    }, "Massive reads + writes");
}
