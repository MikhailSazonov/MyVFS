#include <FileManager.hpp>
#include <CacheManager.hpp>
#include <Journal.hpp>
#include <ThreadPool.hpp>

#include <test_utils.hpp>
#include <cstring>

#include <chrono>
#include <iostream>
#include <memory>

using namespace Executors;
using namespace TestTask;
using namespace TestTask::Cache;
using namespace TestTask::Concurrency;

using namespace std::literals::chrono_literals;

namespace TestTask
{
    struct TestGenerator
    {
        File* CreateFile()
        {
            File* new_file = new File();
            files_.insert(new_file); 
            return new_file;
        }

        ~TestGenerator()
        {
            for (File* file : files_)
            {
                delete file;
            }
        }

        std::set<File*> files_;
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

void ConcurrentTest(size_t threads, size_t iters)
{
    Guard<std::unordered_map<std::string, File*>> gu;
    CacheManager dummy_cache(10, 0.75, gu);
    FileManager mg(dummy_cache, "storage_test");
    TestGenerator tgen;

    std::vector<File*> files;
    for (size_t i = 0; i < threads; ++i)
    {
        files.push_back(tgen.CreateFile());
    }

    ThreadPool tp(threads);
    tp.Start();

    for (size_t i = 0; i < threads; ++i)
    {
        tp.Submit([&, i]()
        {
            std::string data;
            
            for (size_t j = 0; j < iters; ++j)
            {
                size_t action = random() % 2;
                switch (action)
                {
                    case 0:
                        {
                            data.clear();
                            size_t sz = random() % 5 + 4;
                            for (size_t k = 0; k < sz; ++k)
                            {
                                data += 'a' + random() % 25;
                            }
                            mg.Write(files[i], data.c_str(), data.size());
                        }
                        break;
                    case 1:
                        {
                            char buf[10];
                            assert(mg.Read(files[i], buf, 10) == data.size());
                            assert_equal(std::string(buf, data.size()), data);
                        }
                        break;
                }
            }
        });
    }

    tp.WaitIdle();
    tp.Stop();   
}

int main() {
    section("storage");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> g;
        CacheManager dummy_cache(10, 0.75, g);
        FileManager mg(dummy_cache, "storage_test");
        TestGenerator tgen;

        File* a = tgen.CreateFile();

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
        TestGenerator tgen;

        File* a = tgen.CreateFile();
        File* b = tgen.CreateFile();

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
        TestGenerator tgen;

        File* a = tgen.CreateFile();
        File* b = tgen.CreateFile();
        File* c = tgen.CreateFile();

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
        TestGenerator tgen;

        File* a = tgen.CreateFile();
        File* b = tgen.CreateFile();
        File* c = tgen.CreateFile();
        File* d = tgen.CreateFile();
        File* e = tgen.CreateFile();

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
        mg.Write(c, content4.c_str(), 0);
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

    std::remove("./storage_test");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> gu;
        CacheManager dummy_cache(10, 0.75, gu);
        FileManager mg(dummy_cache, "storage_test");
        TestGenerator tgen;

        File* a = tgen.CreateFile();
        File* b = tgen.CreateFile();
        File* c = tgen.CreateFile();
        File* d = tgen.CreateFile();
        File* e = tgen.CreateFile();
        File* f = tgen.CreateFile();
        File* g = tgen.CreateFile();

        std::string content1 = "AAAAA";
        std::string content2 = "BBBBB";
        std::string content3 = "CCCCC";
        std::string content4 = "DDDDD";
        std::string content5 = "EEEEE";
        std::string content6 = "FFFFF";
        std::string content7 = "GGGGGGGGGGGGGGGG";

        mg.Write(a, content1.c_str(), 5);
        mg.Write(b, content2.c_str(), 5);
        mg.Write(c, content3.c_str(), 5);
        mg.Write(d, content4.c_str(), 5);
        mg.Write(e, content5.c_str(), 5);
        mg.Write(f, content6.c_str(), 5);

        mg.Write(a, content1.c_str(), 0);
        mg.Write(c, content3.c_str(), 0);
        mg.Write(e, content5.c_str(), 0);

        mg.Write(g, content7.c_str(), 16);

        char buf[100] = {0};

        assert(mg.Read(a, buf, 10) == 0);
        assert(mg.Read(b, buf + 10, 10) == 5);
        assert(std::string(buf + 10, 5) == content2);
        assert(mg.Read(c, buf, 10) == 0);
        assert(mg.Read(d, buf + 20, 10) == 5);
        assert(std::string(buf + 20, 5) == content4);
        assert(mg.Read(e, buf, 10) == 0);
        assert(mg.Read(f, buf + 30, 10) == 5);
        assert(std::string(buf + 30, 5) == content6);
        assert(mg.Read(g, buf + 40, 20) == 16);
        assert(std::string(buf + 40, 16) == content7);

        std::ifstream is("./storage_test");
        std::string data;
        is >> data;
        assert(data.starts_with("GGGGGBBBBBGGGGGDDDDDGGGGGFFFFFG"));

    }, "Fragmentation #2");

    std::remove("./storage_test");

    test([&]() {
        Guard<std::unordered_map<std::string, File*>> gu;
        CacheManager dummy_cache(10, 0.75, gu);
        FileManager mg(dummy_cache, "storage_test");
        TestGenerator tgen;

        File* a = tgen.CreateFile();
        File* b = tgen.CreateFile();
        File* c = tgen.CreateFile();
        File* d = tgen.CreateFile();

        std::string content1 = "AAAAAA";
        std::string content2 = "BBB";
        std::string content3 = "CCCC";
        std::string content4 = "DD";

        mg.Write(a, content1.c_str(), 6);
        mg.Write(b, content2.c_str(), 3);

        mg.Write(a, content1.c_str(), 0);

        mg.Write(c, content3.c_str(), 4);
        mg.Write(d, content4.c_str(), 2);

        char buf[100] = {0};

        assert(mg.Read(a, buf, 10) == 0);
        assert(mg.Read(b, buf + 10, 10) == 3);
        assert(std::string(buf + 10, 3) == content2);
        assert(mg.Read(c, buf + 20, 10) == 4); 
        assert(std::string(buf + 20, 4) == content3);
        assert(mg.Read(d, buf + 30, 10) == 2);
        assert(std::string(buf + 30, 2) == content4);

        std::ifstream is("./storage_test");
        std::string data;
        is >> data;
        assert(data.starts_with("CCCCDDBBB"));

    }, "Filling the space");

    std::remove("./storage_test");

    test([&]() {

        ConcurrentTest(2, 10000);

    }, "Concurrent #1");

    std::remove("./storage_test");

    test([&]() {

        ConcurrentTest(5, 5000);

    }, "Concurrent #2");

    std::remove("./storage_test");

}
