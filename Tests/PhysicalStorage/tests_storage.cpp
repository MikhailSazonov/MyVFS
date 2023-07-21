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
        FileManager mg(dummy_cache, "storage_test");

        File* a = TestGenerator::CreateFile();

        std::string content = "ABC";

        ssize_t wr = mg.Write(a, content.c_str(), 3);
        
        assert(wr == 3);

        char buf[100] = {0};

        ssize_t r = mg.Read(a, buf, 10);
        
        std::string rd(buf);

        assert(r == 3);
        assert(rd == "ABC");

    }, "Just works");

    std::remove("./storage_test");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager dummy_cache(10, 0.75, g);
        FileManager mg(dummy_cache, "storage_test");

        File* a = TestGenerator::CreateFile();
        File* b = TestGenerator::CreateFile();

        std::string content1 = "ABCDE";
        std::string content2 = "FGHIJ";

        ssize_t wr1 = mg.Write(a, content1.c_str(), 5);
        ssize_t wr2 = mg.Write(b, content2.c_str(), 5);
        
        assert(wr1 == 5);
        assert(wr2 == 5);

        char buf[100] = {0};

        ssize_t r1 = mg.Read(a, buf, 10);
        ssize_t r2 = mg.Read(b, buf + 5, 10);
        
        std::string rd(buf);

        assert(r1 == 5 && r2 == 5);
        assert(rd == "ABCDEFGHIJ");

    }, "Two files");

    std::remove("./storage_test");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager dummy_cache(10, 0.75, g);
        FileManager mg(dummy_cache, "storage_test");

        File* a = TestGenerator::CreateFile();
        File* b = TestGenerator::CreateFile();
        File* c = TestGenerator::CreateFile();

        std::string content1 = "ABCDE";
        std::string content2 = "RRRRR";
        std::string content3 = "AAAAA";

        size_t wr1 = mg.Write(a, content1.c_str(), 5);
        size_t wr2 = mg.Write(b, content2.c_str(), 5);
        
        assert(wr1 == 5);
        assert(wr2 == 5);

        wr1 = mg.Write(a, content1.c_str(), 0);
        assert(wr1 == 0);

        size_t wr3 = mg.Write(c, content3.c_str(), 5);
        assert(wr3 == 5);

        char buf[100] = {0};
        ssize_t r1 = mg.Read(a, buf, 10);

        ssize_t r2 = mg.Read(b, buf + 5, 10);
        ssize_t r3 = mg.Read(c, buf, 10);

        std::string rd(buf);

        assert(r1 == 0 && r2 == 5 && r3 == 5);
        assert(rd == "AAAAARRRRR");

        std::ifstream is("./storage_test");
        std::string data;
        is >> data;
        assert(data.starts_with("AAAAARRRRR"));

    }, "Use previous slots");

    std::remove("./storage_test");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager dummy_cache(10, 0.75, g);
        FileManager mg(dummy_cache, "storage_test");

        File* a = TestGenerator::CreateFile();
        File* b = TestGenerator::CreateFile();
        File* c = TestGenerator::CreateFile();
        File* d = TestGenerator::CreateFile();
        File* e = TestGenerator::CreateFile();

        std::string content1 = "AAAAA";
        std::string content2 = "BBBBB";
        std::string content3 = "CCCCC";
        std::string content4 = "DDDDD";
        std::string content5 = "EEEEEEEEEE";

        mg.Write(a, content1.c_str(), 5);
        mg.Write(b, content2.c_str(), 5);
        mg.Write(c, content3.c_str(), 5);
        mg.Write(d, content4.c_str(), 5);

        mg.Write(a, content2.c_str(), 0);
        mg.Write(c, content2.c_str(), 0);
        mg.Write(e, content5.c_str(), 10);

        char buf[100] = {0};

        assert(mg.Read(a, buf, 10) == 0);
        assert(mg.Read(b, buf + 10, 10) == 5);
        assert(std::string(buf + 10, 5) == content2);
        assert(mg.Read(c, buf, 10) == 0);
        assert(mg.Read(d, buf + 20, 10) == 5);
        assert(std::string(buf + 20, 5) == content4);
        assert(mg.Read(e, buf + 30, 10) == 10);
        assert(std::string(buf + 30, 10) == content5);

        std::ifstream is("./storage_test");
        std::string data;
        is >> data;
        assert(data.starts_with("EEEEEBBBBBEEEEEDDDDD"));

    }, "Fragmentation #1");

}
