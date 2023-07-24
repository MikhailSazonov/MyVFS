#pragma once

#include "IVFS.hpp"

#include "CacheManager.hpp"
#include "FileManager.hpp"
#include "DirTree.hpp"

#include "Concurrency/TicketLock/Guard.hpp"

#include <unordered_set>
#include <optional>
#include <deque>
#include <memory>

namespace TestTask
{
    struct MyVFS : public IVFS
    {
        friend Dir* GetRoot(MyVFS&);
        friend void SerializeVFS(const MyVFS&, std::ostream&);
        friend void DeserializeVFS(std::istream&, MyVFS&);

        MyVFS(size_t workers = 4);
        ~MyVFS();
        File *Open(const char *name);
        File *Create(const char *name);

        size_t Read(File *f, char *buff, size_t len);
        size_t Write(File *f, char *buff, size_t len);
        void Close(File *f);

        private:
            void BindFileToFS(File* f);

            void WriteToCache(File *f, char *buff, size_t len);

            void AddToTheTree(File *, const std::string&);

        private:
            Cache::CacheManager ram_cache_;

            std::deque<File> files_;
            std::atomic<uint32_t> round_robin_idx{0};
            size_t workers_;
            std::vector<std::optional<FileManager>> managers_;
            
            Concurrency::Guard<std::unordered_map<std::string, File*>> guarded_fileset_;
            Dir* root_;
    };

    void SerializeVFS(const MyVFS&, std::ostream&);

    void DeserializeVFS(std::istream&, MyVFS&);
}
