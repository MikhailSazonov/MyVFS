#pragma once

#include <atomic>
#include <optional>

#include "Concurrency/TicketLock/Guard.hpp"

namespace TestTask
{
    struct ChunkInfo
    {
        size_t size_;
        size_t offset_;
    };

    struct PhysicalLocation
    {
        const char* real_filename_;
        std::vector<ChunkInfo> chunks_info_;
    };

    struct CachedInfo
    {
        std::optional<std::string> data_{std::nullopt};
    }

    /*
        Не реализуем потокобезопасную запись в файл, чтобы не тратить ресурсы
    */

    class File
    {
        friend class MyVFS;
        File();

        enum class Mode : uint32_t {
            CLOSED = 0, READONLY = 1, WRITEONLY = 2
        };

        // ?
        const char *filename_;
        std::atomic<uint8_t> mode_{Mode::CLOSED};

        Concurrency::Guard<CachedInfo> guarded_cache_;
        std::atomic<uint64_t> last_time_read_{0};

        // Информация о физическом местоположении данных
        PhysicalLocation location_;
    };
}
