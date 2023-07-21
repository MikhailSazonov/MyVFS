#include <FileManager.hpp>
#include <CacheManager.hpp>
#include <Journal.hpp>
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
    section("storage");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager dummy_cache(10, 0.75, g);
        FileManager mg(dummy_cache, "testing");

        std::cout << "fm spawned\n";

        File* a = TestGenerator::CreateFile();

        std::string content = "ABC";

        ssize_t wr = mg.Write(a, content.c_str(), 3);

        std::cout << "written\n";
        
        assert_equal(wr, 3);

        char buf[100] = {0};

        std::cout << "about to read\n";

        ssize_t r = mg.Read(a, buf, 10);

        std::cout << "read\n";
        
        std::string rd(buf);

        assert_equal(r, 3);
        assert_equal(rd, "ABC");

    }, "Just works");
}
