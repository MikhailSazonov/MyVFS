#pragma once

#include <atomic>
#include <optional>
#include <vector>
#include <set>

#include "Concurrency/TicketLock/Guard.hpp"

#include "ManagersFwd.hpp"

namespace TestTask
{
    struct PhysicalLocation
    {
        /* Информация о чанках в формате: начало чанка, конец чанка */
        std::set<std::pair<uint64_t, uint64_t>> chunks_info_;
    };

    struct CachedInfo
    {
        std::optional<std::string> data_{std::nullopt};
    };

    /*
        Не реализуем потокобезопасную запись в файл, чтобы не тратить ресурсы
    */

    class File
    {
        friend class MyVFS;
        friend class TestGenerator;
        friend class FileManager;
        friend class Cache::CacheManager;

        File() {}

        enum class Mode : uint8_t {
            CLOSED = 0, READONLY = 1, WRITEONLY = 2
        };

        // ?
        const char *filename_;
        std::atomic<Mode> mode_{Mode::CLOSED};

        Concurrency::Guard<CachedInfo> guarded_cache_;
        std::atomic<uint64_t> last_time_read_{0};

        // Информация о физическом местоположении данных
        PhysicalLocation location_;
    };
}
