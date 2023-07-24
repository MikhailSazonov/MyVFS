#include <MyVFS.hpp>
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

void Remove()
{
    std::remove("./0");
    std::remove("./1");
    std::remove("./2");
    std::remove("./3");
    std::remove("./4");
    std::remove("./myvfs");
}

namespace TestTask
{
    Dir* GetRoot(MyVFS& vfs)
    {
        return vfs.root_;
    }
}

void ConcurrentTest(size_t threads, size_t workers, size_t iters)
{
    MyVFS vfs(workers);

    ThreadPool tp(threads);
    tp.Start();

    std::unordered_map<std::string, std::string> file_contents(threads * 2);

    for (size_t i = 0; i < threads; ++i)
    {
        std::string name = "/a";
        name[1] += i;
        file_contents[name].reserve(100);
    }

    std::atomic<size_t> reads{0};
    std::atomic<size_t> writes{0};

    for (size_t i = 0; i < threads; ++i)
    {
        tp.Submit([&, i]()
        {   
            for (size_t j = 0; j < iters; ++j)
            {
                size_t action = random() % 3;
                std::string filename = "/a";
                filename[1] += random() % threads;

                switch (action)
                {
                    case 0:
                    case 1:
                        {
                            File* f = vfs.Open(filename.c_str());
                            if (!f)
                            {
                                continue;
                            }
                            char buf[20] = {0};
                            vfs.Read(f, buf, 20);
                            assert_equal(std::string(buf), file_contents[filename]);
                            reads.fetch_add(1, std::memory_order_release);
                            vfs.Close(f);
                        }
                        break;
                    case 2:
                        {
                            File* f = vfs.Create(filename.c_str());
                            if (!f)
                            {
                                continue;
                            }
                            std::string data;
                            size_t sz = random() % 20;
                            for (size_t i = 0; i < sz; ++i)
                            {
                                char symb = 'a' + random() % 25;
                                data += symb;
                            }
                            file_contents[filename] = data;
                            vfs.Write(f, (char*)data.c_str(), data.size());
                            writes.fetch_add(1, std::memory_order_release);
                            vfs.Close(f);
                        }
                        break;
                }
            }
        });
    }

    tp.WaitIdle();
    tp.Stop();

    std::cout << "Reads: " << reads.load(std::memory_order_acquire) << '\n';
    std::cout << "Writes: " << writes.load(std::memory_order_acquire) << '\n';
}

int main() {
    section("VFS");

    test([&]() {
        MyVFS vfs;

        assert(!vfs.Open("/some_file"));

        File* f;
        assert(f = vfs.Create("/good_file"));
        vfs.Close(f);

        assert(f = vfs.Open("/good_file"));

        vfs.Close(f);
    }, "Just works");

    Remove();

    test([&]() {
        MyVFS vfs;
        File* f;

        assert(f = vfs.Create("/good_file"));
        std::string h = "Hello world!\n";

        vfs.Write(f, (char*)h.c_str(), h.size());        

        char buf[100] = {0};
        vfs.Read(f, buf, 20);

        assert(std::string(buf) == h);

    }, "Write-read");

    Remove();

    test([&]() {
        MyVFS vfs;
        File* f;

        assert(f = vfs.Create("/good_file"));
        std::string h = "Hello world!\n";

        vfs.Write(f, (char*)h.c_str(), h.size());        

        char buf[100] = {0};
        vfs.Read(f, buf, 20);

        assert(std::string(buf) == h);
        
        buf[0] = 'M';
        vfs.Write(f, buf, h.size());

        char buf2[100] = {0};
        vfs.Read(f, buf2, 20);

        assert(std::string(buf2) == "Mello world!\n");

    }, "RMW");

    Remove();

    test([&]() {
        MyVFS vfs;
        File* f1;
        File* f2;
        File* f3;
        File* f4;
        File* f5;

        assert(f1 = vfs.Create("/good_file"));
        assert(f2 = vfs.Create("/folder/good_file"));
        assert(f3 = vfs.Create("/folder/folder2/good_file"));
        assert(f4 = vfs.Create("/good_folder/good_file"));
        assert(f5 = vfs.Create("/good_folder/folder2/good_file"));
        
        auto* root = GetRoot(vfs);

        assert(root->dirs_[0]->name_ == "folder");
        assert(root->dirs_[1]->name_ == "good_folder");
        assert(root->dirs_[0]->dirs_[0]->name_ == "folder2");
        assert(root->dirs_[1]->dirs_[0]->name_ == "folder2");
        assert(std::string(root->files_[0]->filename_) == "good_file");
        assert(std::string(root->dirs_[0]->files_[0]->filename_) == "good_file");
        assert(std::string(root->dirs_[0]->dirs_[0]->files_[0]->filename_) == "good_file");
        assert(std::string(root->dirs_[1]->files_[0]->filename_) == "good_file");
        assert(std::string(root->dirs_[1]->dirs_[0]->files_[0]->filename_) == "good_file");

        vfs.Close(f1);
        vfs.Close(f2);
        vfs.Close(f3);
        vfs.Close(f4);
        vfs.Close(f5);

    }, "Directories");

    Remove();

    test([&]() {
        std::string content1 = "mein";
        std::string content2 = "herz";
        std::string content3 = "brennt";
        {
            MyVFS vfs;
            File* f1;
            File* f2;

            f1 = vfs.Create("/good_file");
            f2 = vfs.Create("/folder/good_file");
            
            vfs.Write(f1, &content1[0], content1.size());
            vfs.Write(f2, &content2[0], content2.size());

            vfs.Close(f1);
            vfs.Close(f2);
        }

        {
            MyVFS vfs;
            File* f1;
            File* f2;
            File* f3;

            f3 = vfs.Create("/folder/folder2/good_file");
            vfs.Write(f3, &content3[0], content3.size());
            vfs.Close(f3);

            assert(f1 = vfs.Open("/good_file"));
            assert(f2 = vfs.Open("/folder/good_file"));
            assert(f3 = vfs.Open("/folder/folder2/good_file"));

            char buf[10] = {0};

            size_t rd = vfs.Read(f1, buf, 10);
            assert(std::string(buf, rd) == content1);

            rd = vfs.Read(f2, buf, 10);
            assert(std::string(buf, rd) == content2);

            rd = vfs.Read(f3, buf, 10);
            assert(std::string(buf, rd) == content3);

            auto* root = GetRoot(vfs);

            assert(root->dirs_[0]->name_ == "folder");
            assert(root->dirs_[0]->dirs_[0]->name_ == "folder2");
            assert(std::string(root->files_[0]->filename_) == "good_file");
            assert(std::string(root->dirs_[0]->files_[0]->filename_) == "good_file");
            assert(std::string(root->dirs_[0]->dirs_[0]->files_[0]->filename_) == "good_file");

            vfs.Close(f1);
            vfs.Close(f2);
            vfs.Close(f3);
        }

    }, "Mount");

    Remove();

    test([&]() {
        MyVFS vfs(1);
        ThreadPool tp(8);
        tp.Start();

        File* f;
        f = vfs.Create("/test_file");
        vfs.Close(f);

        std::atomic<int> contestants_{0};

        for (size_t i = 0; i < 2; ++i)
        {
            tp.Submit([&]() {
                size_t j = 0;
                while (j < 50)
                {
                    auto* f = vfs.Create("/test_file");
                    if (f)
                    {
                        contestants_.store(contestants_.load() + 1);
                        assert(contestants_.load() == 1);
                        contestants_.store(contestants_.load() - 1);
                        ++j;
                        vfs.Close(f);
                    }
                }
            });
        }

        for (size_t i = 0; i < 6; ++i)
        {
            tp.Submit([&]() {
                size_t j = 0;
                while (j < 800)
                {
                    auto* f = vfs.Open("/test_file");
                    if (f)
                    {
                        contestants_.fetch_add(1);
                        std::this_thread::sleep_for(1ms);
                        contestants_.fetch_sub(1);
                        ++j;
                        vfs.Close(f);
                    }
                }
            });
        }

        tp.WaitIdle();
        tp.Stop();

    }, "Reader-writer pattern");

    Remove();

    test([&]() {

        ConcurrentTest(2, 1, 5000);
    
    }, "Concurrent #1");

    Remove();

    test([&]() {

        ConcurrentTest(8, 4, 1000);
    
    }, "Concurrent #2");

    Remove();

}
