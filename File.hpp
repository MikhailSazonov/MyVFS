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

    enum class Mode : uint8_t {
        CLOSED = 0, READONLY = 1, WRITEONLY = 2
    };

    class File
    {
        friend class MyVFS;
        friend class TestGenerator;
        friend class FileManager;
        friend class Cache::CacheManager;

        public:
            File() {}

        public:
            std::string filename_;
            std::string full_filename_;

        private:
            Concurrency::Guard<CachedInfo> guarded_cache_;
            std::atomic<uint64_t> last_time_read_{0};

            // Информация о физическом местоположении данных
            PhysicalLocation location_;

            /*
                Здесь храним информацию о читателях/писателях файла
                0 => файл закрыт, -1 => WRONLY, >0 => READONLY
            */
            std::atomic<int> ref_count_{0};

            size_t manager_idx_;
    };
}
