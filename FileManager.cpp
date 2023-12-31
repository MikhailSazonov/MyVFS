#include "FileManager.hpp"

using namespace TestTask;

FileManager::FileManager(Cache::CacheManager& cache,
const std::string& worker_id,
std::optional<size_t> max_tasks_per_one)
    : worker_id_(worker_id), cache_(cache), max_tasks_per_once_(max_tasks_per_one)
    {
        std::string storage_name = "./" + worker_id;
        phys_file_.f_ = fopen(storage_name.c_str(), "r+");
        if (!phys_file_.f_ && !(phys_file_.f_ = fopen(storage_name.c_str(), "w+")))
        {
            std::cerr << std::strerror(errno) << '\n';
            throw OpenStorageError();
        }
        worker_.emplace([this](){
            this->ProcessTasks();
        });
    }

FileManager::~FileManager()
{
    std::fclose(phys_file_.f_);
    quit_.store(true, std::memory_order_release);
    worker_->join();
}

size_t FileManager::Read(File* f, char* buf, size_t len)
{
    Task* new_task = tasks_journal_.AddTask(f, TaskType::READ, buf, len);
    return tasks_journal_.WaitForTask(new_task);
}

size_t FileManager::Write(File* f, const char* buf, size_t len)
{
    Task* new_task = tasks_journal_.AddTask(f, TaskType::WRITE, buf, len);
    return tasks_journal_.WaitForTask(new_task);
}

void FileManager::ProcessTasks()
{
    while (!quit_.load(std::memory_order_acquire))
    {
        TasksGroup tasks_group_;
        for (size_t i = 0; !max_tasks_per_once_.has_value() || i < *max_tasks_per_once_; ++i)
        {
            Task* task = tasks_journal_.ExtractTask();
            if (!task)
            {
                break;
            }
            switch (task->type_)
            {
                case TaskType::READ:
                    /* 
                        Последняя проверка перед тем, как добавить задачу:
                        возможно, мы недавно читали файл с диска и он лежит в кэше.
                        Если же нет, то нужно прочитать данные с диска (за данный файл отвечает этот
                        поток => другим путём данные в кэше не появятся)
                    */
                    if (task->f_->location_.chunks_info_.empty() ||
                    cache_.Read(task->f_, std::get<char*>(task->buf_), task->len_) >= 0)
                    {
                        tasks_journal_.MarkDone(&*task);
                        continue;
                    }
                    addReadSegments(&*task, tasks_group_);
                    break;
                case TaskType::WRITE:
                    freeStorage(tasks_group_, task->f_);
                    if (task->len_ > 0)
                    {
                        findStorageForData(&*task, task->len_, tasks_group_, std::get<const char*>(task->buf_));
                    }
                    else
                    {
                        tasks_journal_.MarkDone(&*task);
                        continue;
                    }
                    break;
            }
            tasks_group_.tasks_.push_back(&*task);
        }
        if (tasks_group_.tasks_.empty())
        {
            Concurrency::Spin();
            continue;
        }
        ExecuteTasks(tasks_group_);
    }
}

void FileManager::addReadSegments(Task* task, TasksGroup& tg)
{
    for (const auto& segment : task->f_->location_.chunks_info_)
    {
        tg.to_read_.AddSegment({segment, "", {task}});
    }
}

void FileManager::freeStorage(TasksGroup& tg, File* f)
{
    for (const auto& segment : f->location_.chunks_info_)
    {
        phys_file_.free_segments_.AddSegment({segment});
        tg.to_write_.RemoveSegment({segment});
    }
    f->location_.chunks_info_.clear();
}

void FileManager::addWriteSegment(Task* task, TasksGroup& tg, const char* buf,
const std::pair<uint64_t, uint64_t>& seg,
const std::pair<uint64_t, uint64_t>& copy_idxs)
{
    phys_file_.free_segments_.RemoveSegment({seg});
    task->f_->location_.chunks_info_.insert(seg);
    Segment new_seg{seg, std::string(buf + copy_idxs.first,
    copy_idxs.second - copy_idxs.first), {task}};
    tg.to_write_.AddSegment(std::move(new_seg));
}

void FileManager::findStorageForData(Task* task, size_t size, TasksGroup& tg, const char* buf)
{
    const auto& sorted_segments = phys_file_.free_segments_.GetAllSegmentsSorted();

    /* Проверяем, можно ли файл записать на диск целиком в свободный чанк */

    auto full_file_chunk = phys_file_.free_segments_.GetSegmentBySize(size);
    if (full_file_chunk.has_value())
    {
        addWriteSegment(task, tg, buf, full_file_chunk->points_, {0, size});
        return;
    }
    /* Проверяем, есть ли 2 свободных сегмента, в которые файл целиком поместится */

    if (sorted_segments.size() >= 2)
    {
        for (size_t i = 1; i <= size / 2; ++i)
        {
            auto chunk_1 = phys_file_.free_segments_.GetSegmentBySize(i);
            auto chunk_2 = phys_file_.free_segments_.GetSegmentBySize(size - i, (i == size - i ? 1 : 0));
            if (chunk_1.has_value() && chunk_2.has_value())
            {
                if (chunk_1->points_.first < chunk_2->points_.first)
                {
                    addWriteSegment(task, tg, buf, chunk_1->points_, {0, i});
                    addWriteSegment(task, tg, buf, chunk_2->points_, {i, size});
                }
                else
                {
                    addWriteSegment(task, tg, buf, chunk_1->points_, {size - i, size});
                    addWriteSegment(task, tg, buf, chunk_2->points_, {0, size - i});
                }
                return;
            }   
        }
    }

    /* Просто пишем в свободные сегменты, с начала файла */
    size_t total_len = 0;
    std::map<uint64_t, uint64_t> segments_to_write;
    for (const auto& seg : sorted_segments)
    {
        if (total_len == size)
        {
            break;
        }
        uint64_t right_side = std::min(seg.second, seg.first + (size - total_len) - 1);
        uint64_t length = right_side - seg.first + 1;
        total_len += length;
        segments_to_write.insert(seg);
    }

    total_len = 0;
    for (const auto& seg : segments_to_write)
    {
        uint64_t right_side = std::min(seg.second, seg.first + (size - total_len) - 1);
        uint64_t length = right_side - seg.first + 1;
        addWriteSegment(task, tg, buf, {seg.first, right_side}, {total_len, total_len + length});
        total_len += length;
    }

    /* Если их не хватило, добавляем в конец файла */
    if (total_len < size)
    {
        uint64_t rest = size - total_len;
        addWriteSegment(task, tg, buf, {phys_file_.file_size_, phys_file_.file_size_ + rest - 1}, {total_len, size});
        phys_file_.file_size_ += rest;
    }
}

void FileManager::ExecuteTasks(const TasksGroup& tg)
{
    /* Пишем в файл */
    const auto& write_segments = tg.to_write_.GetAllSegmentsSorted();
    std::unordered_map<File*, ProcessingState> write_states;

    for (const auto& seg : write_segments)
    {
        const auto& seg_struct = tg.to_write_.GetSegmentByPoints(seg.first, seg.second);

        size_t count = seg.second - seg.first + 1;
        size_t result = WriteToFile(seg_struct.data_.c_str(), seg.first, count);

        for (auto* task : seg_struct.tasks_)
        {
            auto* f = task->f_;
            if (write_states.find(f) == write_states.end())
            {
                write_states[f] = ProcessingState{f->location_.chunks_info_.begin()};
            }
            size_t pos1 = write_states[f].seg_iter->first - seg.first;
            size_t pos2 = write_states[f].seg_iter->second - seg.first;
            
            /* Добавляем проверку, на случай, если запись на диск выдаст ошибку */
            if (result >= pos1)
            {
                size_t count_f = std::min(pos2 - pos1 + 1, result - pos1);
                write_states[f].pos_ += count_f;
            }
            if (++write_states[f].seg_iter == f->location_.chunks_info_.end() ||
                        pos2 > result)
            {
                task->task_result_ = write_states[f].pos_;
                tasks_journal_.MarkDone(task);
            }
        }
    }

    /* Читаем из файла */
    const auto& read_segments = tg.to_read_.GetAllSegmentsSorted();
    std::unordered_map<File*, ProcessingState> read_states;

    for (const auto& seg : read_segments)
    {
        size_t count = seg.second - seg.first + 1;
        std::string buf;
        buf.resize(count);
        size_t result = ReadFromFile(buf.data(), seg.first, count);

        const auto& seg_struct = tg.to_read_.GetSegmentByPoints(seg.first, seg.second);
        for (auto* task : seg_struct.tasks_)
        {
            auto* f = task->f_;
            if (read_states.find(f) == read_states.end())
            {
                read_states[f] = {f->location_.chunks_info_.begin()};
            }
            size_t pos1 = read_states[f].seg_iter->first - seg.first;
            size_t pos2 = read_states[f].seg_iter->second - seg.first;
            
            /* Добавляем проверку, на случай, если чтение с диска выдаст ошибку */
            if (result >= pos1)
            {
                size_t count_f = std::min(pos2 - pos1 + 1, result - pos1);
                buf.copy(std::get<char*>(task->buf_) + read_states[f].pos_, count_f, pos1);
                read_states[f].pos_ += count_f;
            }
            if (++read_states[f].seg_iter == f->location_.chunks_info_.end() ||
                        pos2 > result)
            {
                task->task_result_ = read_states[f].pos_;
                tasks_journal_.MarkDone(task);
            }
        }
    }

    /* Записываем внутренний буфер на диск */
    std::fflush(phys_file_.f_);
}

size_t FileManager::ReadFromFile(char* read_to, size_t position, size_t count)
{
    std::fseek(phys_file_.f_, position, SEEK_SET);
    auto res = std::fread(read_to, sizeof(char), count, phys_file_.f_);
    return res;
}

size_t FileManager::WriteToFile(const char* write_from, size_t position, size_t count)
{
    std::fseek(phys_file_.f_, position, SEEK_SET);
    return std::fwrite(write_from, sizeof(char), count, phys_file_.f_);
}
