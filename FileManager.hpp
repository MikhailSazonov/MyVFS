#pragma once

#include <optional>
#include <thread>
#include <unordered_map>
#include <fstream>
#include <stdio.h>
#include <cerrno>
#include <cstring>

#include "CacheManager.hpp"
#include "SegmentSystem.hpp"
#include "Journal.hpp"
#include "FileExceptions.hpp"
#include "Concurrency/TicketLock/Spin.hpp"
#include "VFSfwd.hpp"

namespace TestTask
{
    struct TasksGroup
    {
        SegmentSystem to_read_;
        SegmentSystem to_write_;
        std::vector<Task*> tasks_;
    };

    struct PhysicalFile
    {
        std::FILE* f_;
        size_t file_size_{0};
        SegmentSystem free_segments_;
    };

    struct ProcessingState
    {
        std::set<std::pair<uint64_t, uint64_t>>::iterator seg_iter;
        size_t pos_{0};
        bool finished_{false};
    };

    class FileManager
    {
        friend void SerializeVFS(const MyVFS&, std::ostream&);
        friend void DeserializeVFS(std::istream&, MyVFS&);

        public:
            FileManager(Cache::CacheManager&,
            const std::string&, std::optional<size_t> max_tasks_per_one = std::nullopt);

            ~FileManager();

            size_t Read(File*, char*, size_t);

            size_t Write(File*, const char*, size_t);

        private:
            void ProcessTasks();

            void freeStorage(TasksGroup&, File*);

            void findStorageForData(Task*, size_t, TasksGroup&, const char*);

            void addWriteSegment(Task*, TasksGroup&, const char*, const std::pair<uint64_t, uint64_t>&,
            const std::pair<uint64_t, uint64_t>&);

            void addReadSegments(Task*, TasksGroup&);

            void ExecuteTasks(const TasksGroup&);

            size_t ReadFromFile(char*, size_t, size_t);

            size_t WriteToFile(const char*, size_t, size_t);

        private:
            PhysicalFile phys_file_;
            std::string worker_id_;

            Cache::CacheManager& cache_;
            Journal tasks_journal_;

            const std::optional<size_t> max_tasks_per_once_;

            std::optional<std::thread> worker_;
            std::atomic<bool> quit_{false};
    };
}
