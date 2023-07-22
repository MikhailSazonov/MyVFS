#include "MyVFS.hpp"

using namespace TestTask;

MyVFS::MyVFS(size_t workers)
    :
        ram_cache_(1 << 20, 0.75, guarded_fileset_)
    ,   managers_(workers)
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
    // Проверяем через CAS, что файл открыт либо закрыт, либо открыт в readonly
    auto current_mode = File::Mode::CLOSED;
    if (!file_iter->second->mode_.compare_exchange_strong(current_mode, File::Mode::READONLY, std::memory_order_acq_rel) &&
        current_mode != File::Mode::READONLY)
    {
        return nullptr;   
    }
    return file_iter->second.get();
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
            auto current_mode = File::Mode::CLOSED;
            if (!file_iter->second->mode_.compare_exchange_strong(current_mode, File::Mode::WRITEONLY, std::memory_order_acq_rel))
            {
                return nullptr;
            }
            return file_iter->second.get();
        }
    }

    auto wg = Concurrency::WriteGuard(guarded_fileset_);
    auto& fs = wg.get();
    if (fs.find(std::string(name)) == fs.end())
    {
        fs[std::string(name)].reset(new File());
        fs[std::string(name)]->filename_ = filename;
        fs[std::string(name)]->full_filename_ = name;
        BindFileToFS(fs[std::string(name)].get());
        AddToTheTree(fs[std::string(name)].get(), name);
    }
    fs[std::string(name)]->mode_.store(File::Mode::WRITEONLY, std::memory_order_release);

    return fs[std::string(name)].get();
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
    if (file_iter->second->mode_.load(std::memory_order_acquire) == File::Mode::CLOSED)
    {
        throw FileClosedError();
    }
    file_iter->second->mode_.store(File::Mode::CLOSED, std::memory_order_release);
}

void MyVFS::BindFileToFS(File* f)
{
    f->manager_idx_ = round_robin_idx.fetch_add(1, std::memory_order_acq_rel) % 4;
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
