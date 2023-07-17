#include "MyVFS.hpp"

using namespace TestTask;

MyVFS::MyVFS()
{
    // прочитать конфиг с диска
}

MyVFS::~MyVFS()
{
    // записать конфиг на диск
}

File* MyVFS::Open(const char* name)
{
    auto rg = Concurrency::ReadGuard(guarded_fileset_);
    const auto& fs = rg.get();
    auto file_iter = fs.find(std::string(name));
    // Проверка на существование
    if (file_iter == fs.end())
    {
        return nullptr;
    }
    // Проверяем через CAS, что файл открыт либо закрыт, либо открыт в readonly
    auto current_mode = File::Mode::CLOSED;
    if (!file_iter->mode_.compare_exchange_strong(current_mode, File::Mode::READONLY, std::memory_order_acq_rel) &&
        current_mode != File::Mode::READONLY)
    {
        return nullptr;   
    }
    return *file_iter;
}

File* MyVFS::Create(const char* name)
{
    {
        auto rg = Concurrency::ReadGuard(guarded_fileset_);
        auto& fs = rg.get();
        auto file_iter = fs.find(std::string(name));
        if (file_iter != fs.end())
        {
            auto current_mode = File::Mode::CLOSED;
            if (!file_iter->mode_.compare_exchange_strong(current_mode, File::Mode::WRITEONLY, std::memory_order_acq_rel))
            {
                return nullptr;
            }
            return *file_iter;
        }
    }
    auto wg = Concurrency::WriteGuard(guarded_fileset_);
    fs[std::string(name)] = File();
    BindFileToFS(fs[std::string(name)]);
    return fs[std::string(name)];
}

size_t MyVFS::Read(File *f, char *buff, size_t len)
{
    auto read_bytes = ram_cache_.Read(f, buff, len);
    if (read_bytes == -1)
    {
        // читаем с диска через журналы
        // пишем в кэш
    }
    return read_bytes;
}

size_t MyVFS::Write(File *f, char *buff, size_t len)
{
    auto read_bytes = ram_cache_.Write(f, buff, len);
    // пишем на диск через журналы
    // пишем в кэш
}

void Close(File* f)
{
    auto rg = Concurrency::ReadGuard(guarded_fileset_);
    const auto& fs = rg.get();
    auto file_iter = fs.find(std::string(name));
    if (file_iter == fs.end())
    {
        throw FileNotFoundError;
    }
    if (file_iter->mode_.load(std::memory_order_acquire) == File::Mode::CLOSED)
    {
        throw FileClosedError;
    }
    file_iter->mode_.store(File::Mode::CLOSED, std::memory_order_release);
}
