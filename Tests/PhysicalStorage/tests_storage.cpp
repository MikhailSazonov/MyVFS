#include <FileManager.hpp>
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

        FileManager mg("testing");

        File* a = TestGenerator::CreateFile();

        std::string content = "ABC";

        ssize_t wr = mg.Write(a, content.c_str(), 3);
        
        assert_equal(wr, 3);

        char buf[100] = {0};
        ssize_t r = mg.Read(a, buf, 10);
        
        std::string rd(buf);

        assert_equal(r, 3);
        assert_equal(rd, "ABC");

    }, "Just works");
}
