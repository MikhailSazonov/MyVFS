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
            Concurrency::Guard<std::unordered_map<std::string, File*>> guarded_fileset_;

            std::deque<File> files_;

            Cache::CacheManager ram_cache_;

            std::vector<std::optional<FileManager>> managers_;
            std::atomic<uint32_t> round_robin_idx{0};

            Dir* root_;
    };   
}
