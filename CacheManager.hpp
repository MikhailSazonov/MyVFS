#pragma once

#include "File.hpp"

#include "Concurrency/TicketLock/Guard.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>
#include <condition_variable>
#include <chrono>

namespace TestTask::Cache
{
    namespace Detail
    {
        extern std::atomic<uint8_t> CURRENT_EPOCH;
    }

    struct CacheData
    {
        std::string data_;
        uint64_t last_time_used_{0};
    };

    /*
        LRU-cache для данных файлов
        Для инвалидации кэша используется 64-битный счётчик,
        устроенный следующим образом:

        1 старший бит - номер эпохи,
        остальные 63 бита - собственно счётчик

        Как только 63 бита у данных с самым большим счётчиком (самых свежих)
        меняются, меняется номер эпохи, и любой счётчик в новой эпохи > любого счётчика
        в старой

        Ключевой рассчёт состоит в том, что 64-битный общий 
        счётчик не переполнится (т.е., не будет такой
        ситуации, что есть данные со счётчиком 2 ^ 64 - 1, и данные со счётчиком 0, и при этом
        не нужно что-то инвалидировать) 
    */

    struct CacheWorker
    {
        CacheWorker(Concurrency::Guard<std::unordered_map<std::string, File*>>& guarded_fileset_ref)
            : guarded_fileset_ref_(guarded_fileset_ref) {}

        Concurrency::Guard<std::unordered_map<std::string, File*>>& guarded_fileset_ref_;
        std::optional<std::thread> worker_;
        std::atomic<bool> stop_{false};
    };

    class CacheManager
    {
        public:
            CacheManager(size_t, double, Concurrency::Guard<std::unordered_map<std::string, File*>>&);

            ~CacheManager();

            void Write(File*, const char*, size_t);

            ssize_t Read(File*, char*, size_t);

        private:
            void InvalidateCache(CacheWorker&);

            void IncreaseCounter(File*);
        
        private:
            std::atomic<uint64_t> usage_counter_{0};

            std::atomic<uint64_t> busy_capacity_{0};
            const size_t max_capacity_;
            const double stop_cleaning_threshold_;

            CacheWorker worker_;
    };
}
