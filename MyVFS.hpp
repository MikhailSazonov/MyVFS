#pragma once

#include "IVFS.hpp"

#include "CacheManager.hpp"
#include "FileManager.hpp"

#include "Concurrency/Guard.hpp"

#include <unordered_set>

namespace TestTask
{
    struct MyVFS : public IVFS
    {
        MyVFS();
        ~MyVFS();
        File *Open(const char *name);
        File *Create(const char *name);

        size_t Read(File *f, char *buff, size_t len);
        size_t Write(File *f, char *buff, size_t len);
        void Close(File *f);

        private:
            void BindFileToFS(File& f);

            void WriteToCache(File *f, char *buff, size_t len);

        private:
            Concurrency::Guard<std::unordered_map<std::string, File*>> guarded_fileset_;
            Cache::CacheManager ram_cache_;
    };   
}
