#include "MyVFS.hpp"

using namespace TestTask;

thread_local std::unordered_map<File*, Mode> opened_files_;

MyVFS::MyVFS(size_t workers)
    :
        ram_cache_(1 << 20, 0.75, guarded_fileset_)
    ,   managers_(workers)
    ,   workers_(workers)
{
    root_ = new Dir();
    for (size_t i = 0; i < workers; ++i)
    {
        managers_[i].emplace(ram_cache_, std::to_string(i));
    }
}

MyVFS::~MyVFS()
{
    root_->Clear();
}

static std::string GetFilename(const std::string& name)
{
    size_t pos = name.rfind("/");
    return name.substr(pos + 1);
}

File* MyVFS::Open(const char* name)
{
    auto rg = Concurrency::ReadGuard(guarded_fileset_);
    const auto& fs = rg.get();
    auto file_iter = fs.find(name);
    // Проверка на существование
    if (file_iter == fs.end())
    {
        return nullptr;
    }
    auto* f = file_iter->second;
    // Проверяем режим файла в данном потоке
    if (opened_files_[f] == Mode::WRITEONLY)
    {
        return nullptr;
    }
    /* Пытаемся открыть файл в READONLY-режиме */
    
    while (true)
    {
        auto file_state = f->ref_count_.load(std::memory_order_acquire);
        if (file_state < 0)
        {
            return nullptr;
        }
        if (f->ref_count_.compare_exchange_weak(file_state, file_state + 1, std::memory_order_acq_rel))
        {
            opened_files_[f] = Mode::READONLY;
            return f;
        }
    }
    return nullptr;
}

File* MyVFS::Create(const char* name)
{
    std::string filename = GetFilename(name);
    {
        auto rg = Concurrency::ReadGuard(guarded_fileset_);
        const auto& fs = rg.get();
        auto file_iter = fs.find(std::string(name));
        if (file_iter != fs.end())
        {
            auto* f = file_iter->second;
            int zero_ref = 0;
            if (opened_files_[f] == Mode::READONLY
                || !f->ref_count_.compare_exchange_strong(zero_ref, -1, std::memory_order_acq_rel))
            {
                return nullptr;
            }
            opened_files_[f] = Mode::WRITEONLY;
            return f;
        }
    }

    auto wg = Concurrency::WriteGuard(guarded_fileset_);
    auto& fs = wg.get();
    if (fs.find(std::string(name)) == fs.end())
    {
        files_.emplace_back();
        auto* f = &files_.back();
        fs[std::string(name)] = f;
        f->filename_ = filename;
        f->full_filename_ = name;
        BindFileToFS(f);
        AddToTheTree(f, name);
        f->ref_count_.fetch_sub(1, std::memory_order_release);
        opened_files_[f] = Mode::WRITEONLY;
    }
    else
    {
        /* На случай, если два потока одновременно будут создавать один и тот же файл */
        return nullptr;
    }

    return fs[std::string(name)];
}

size_t MyVFS::Read(File *f, char *buff, size_t len)
{
    size_t result;
    ssize_t read_bytes = ram_cache_.Read(f, buff, len);
    if (read_bytes == -1)
    {
        // читаем с диска
        auto& manager = *managers_[f->manager_idx_];
        result = manager.Read(f, buff, len);
        // пишем в кэш
        if (result == len)
        {
            ram_cache_.Write(f, buff, len);
        }
    }
    else
    {
        result = read_bytes;
    }
    return result;
}

size_t MyVFS::Write(File *f, char *buff, size_t len)
{
    // пишем на диск
    auto& manager = *managers_[f->manager_idx_];
    auto result = manager.Write(f, buff, len);
    if (result == len)
    {
        // пишем в кэш, если всё записалось на диск
        // (гарантируем целостность данных)
        ram_cache_.Write(f, buff, len);
    }
    return result;
}

void MyVFS::Close(File* f)
{
    auto rg = Concurrency::ReadGuard(guarded_fileset_);
    const auto& fs = rg.get();
    auto file_iter = fs.find(std::string(f->full_filename_));
    if (file_iter == fs.end())
    {
        throw FileNotFoundError();
    }
    switch (opened_files_[f])
    {
        case Mode::READONLY:
            f->ref_count_.fetch_sub(1, std::memory_order_release);
            break;
        case Mode::WRITEONLY:
            f->ref_count_.fetch_add(1, std::memory_order_acq_rel);
            break;
        case Mode::CLOSED:
            throw FileClosedError();
    }
    opened_files_[f] = Mode::CLOSED;
}

void MyVFS::BindFileToFS(File* f)
{
    f->manager_idx_ = round_robin_idx.fetch_add(1, std::memory_order_acq_rel) % workers_;
}

void MyVFS::AddToTheTree(File* f, const std::string& name)
{
    Dir* cur_dir = root_;
    size_t idx_1 = 1;
    size_t idx_2 = name.find("/", idx_1);
    std::string name_cur = name.substr(idx_1, idx_2 - idx_1);
    while (idx_2 != std::string::npos)
    {
        Dir* next_dir = cur_dir->FindDir(name_cur);
        if (next_dir == nullptr)
        {
            next_dir = cur_dir->AddDir(name_cur);
        }
        cur_dir = next_dir;
        idx_1 = idx_2 + 1;
        idx_2 = name.find("/", idx_1);
        name_cur = name.substr(idx_1, idx_2 - idx_1);
    }
    cur_dir->AddFile(f);
}
