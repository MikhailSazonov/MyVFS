#include "FileManager.hpp"

using namespace TestTask;

FileManager::FileManager(const std::string& worker_id,
std::optional<size_t> max_tasks_per_one)
    : max_tasks_per_once_(max_tasks_per_one)
    {
        if (!(phys_file_.f_ = fopen("/" + worker_id, "a+")))
        {
            throw OpenStorageError();
        }
    }

FileManager::~FileManager()
{
    fclose(phys_file_.f_);
    quit_.store(true, std::memory_order_release);
    worker_->join();
}

/*
    Асинхронное чтение/запись на диск было бы гораздо эффективнее,
    зато в синхронном случае в этом месте нет динамических аллокаций
*/

ssize_t FileManager::Read(File* f, char* buf, size_t len);
{
    Node<Task> new_node;
    new_node.data_.f_ = f;
    new_node.data_.type_ = TaskType::READ;
    new_node.data_.buf_ = buf;
    new_node.data_.len_ = len;
    tasks_journal_.AddTask(&new_node);
    while (!new_node.task_finished_.load(std::memory_order_acquire))
    {
        Spin();
    }
    return new_node.data_.task_result_;
}

ssize_t FileManager::Write(File* f, const char* buf, size_t len)
{
    Node<Task> new_node;
    new_node.data_.f_ = f;
    new_node.data_.type_ = TaskType::WRITE;
    new_node.data_.buf_ = buf;
    new_node.data_.len_ = len;
    tasks_journal_.AddTask(&new_node);
    while (!new_node.task_finished_.load(std::memory_order_acquire))
    {
        Spin();
    }
    return new_node.task_result_;
}

TasksGroup FileManager::ProcessTasks()
{
    while (!quit_.load(std::memory_order_acquire))
    {
        TasksGroup tasks_group_;
        size_t i = 0;
        for (; !max_tasks_per_once_.has_value() || i < *max_tasks_per_one; ++i)
        {
            Node<Task>* next_task_ = tasks_journal_.ExtractTask();
            if (!next_task_)
            {
                break;
            }
            auto& task = next_task_->data_;
            switch (task.type_)
            {
                case (TaskType::READ)
                {
                    /* 
                        Последняя проверка перед тем, как добавить задачу:
                        возможно, мы недавно читали файл с диска и он лежит в кэше.
                        Если же нет, то нужно прочитать данные с диска (за данный файл отвечает этот
                        поток => другим путём данные в кэше не появятся)
                    */
                    if (cache_.Read(task.f_, std::get<char*>(task.buf_), task.len_) >= 0)
                    {
                        tasks_journal_.MarkDone(&task);
                        continue;
                    }
                    addReadSegments(&task, tasks_group_);
                    break;
                }
                case (TaskType::WRITE)
                {
                    freeStorage(task.f_);
                    findStorageForData(&task, task.len_, tasks_group_, std::get<const char*>(task.buf_));
                    break;
                }
            }
            tasks_group_.tasks_.push_back(&task);
        }
        if (i == 0)
        {
            Spin();
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

void FileManager::freeStorage(File* f)
{
    for (const auto& segment : f->location_.chunks_info_)
    {
        phys_file_.free_segments_.AddSegment({segment});
    }
}

void FileManager::addWriteSegment(Task* task, TasksGroup& tg, const char* buf,
const std::pair<uint64_t, uint64_t>& seg,
const std::pair<uint64_t, uint64_t>& copy_idxs)
{
    phys_file_.free_segments_.RemoveSegment({seg});
    f->location_.chunks_info_.insert(seg);
    Segment new_seg{seg, buf + copy_idxs.first, buf + copy_idxs.second, task->f_};
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
        for (size_t i = 1; i < size / 2; ++i)
        {
            auto chunk_1 = phys_file_.free_segments_.GetSegmentBySize(i);
            auto chunk_2 = phys_file_.free_segments_.GetSegmentBySize(size - i);
            if (chunk_1.has_value() && chunk_2.has_value())
            {
                addWriteSegment(task, tg, buf, chunk_1->points_, {0, i});
                addWriteSegment(task, tg, buf, chunk_2->points_, {i, size});
                return;
            }   
        }
    }
    /* Просто пишем в свободные сегменты, с начала файла */
    size_t total_len = 0;
    for (const auto& seg : sorted_segments)
    {
        if (total_len == size)
        {
            break;
        }
        uint64_t right_side = std::min(seg.second, seg.first + (size - total_len) - 1);
        uint64_t length = right_side - seg.first + 1;
        addWriteSegment(task, tg, buf, {seg.first, right_side}, {total_len, total_len + length});
    }
    /* Если их не хватило, добавляем в конец файла */
    if (total_len < size)
    {
        uint64_t rest = size - total_len;
        addWriteSegment(task, tg, buf, {phys_file_.file_size_, phys_file_.file_size_ + rest - 1}, {total_len, size});
    }
}

void FileManager::ExecuteTasks(const TasksGroup& tg)
{
    /* Читаем из файла */
    const auto& read_segments = tg.to_read_.GetAllSegmentsSorted();
    std::unordered_map<File*, ProcessingState> read_states;
    for (const auto& seg : read_segments)
    {
        size_t count = seg.second - seg.first + 1;
        std::string buf;
        buf.resize(count);
        ssize_t result = ReadFromFile(buf.c_str(), seg.first, count);

        const auto& seg_struct = tg.to_read_.GetSegmentByPoints(seg.first, seg.second);
        for (const auto* task : seg_struct.tasks_)
        {
            const auto* f = task->f_;
            if (read_states.find(f) == read_states.end())
            {
                read_states[f] = {f->location_.chunks_info_.begin()};
            }
            size_t pos1 = read_states[f].seg_iter->first - seg.first;
            size_t pos2 = read_states[f].seg_iter->second - seg.first;
            
            /* Добавляем несколько проверок, на случай, если чтение с диска выдаст ошибку */
            if (result >= pos1)
            {
                size_t count_f = std::min(pos2 - pos1 + 1, result - pos1);
                buf.copy(std::get<char*>(task->buf_) + read_states[f].pos_, count_f, pos1);
                read_states[f].pos_ += count_f;
            }
            if (++read_states[f].seg_iter == f->location_.chunks_info_.end() ||
                        pos2 > result)
            {
                task->task_result_ = read_states[f].pos;
                tasks_journal_.MarkDone(task);
            }
        }
    }

    /* Пишем в файл */
    const auto& write_segments = tg.to_write_.GetAllSegmentsSorted();
    std::unordered_map<File*, ProcessingState> write_states;

    for (const auto& seg : write_segments)
    {
        size_t count = seg.second - seg.first + 1;
        ssize_t result = WriteToFile(buf.c_str(), seg.first, count);

        const auto& seg_struct = tg.to_write_.GetSegmentByPoints(seg.first, seg.second);
        for (auto* task : seg_struct.tasks_)
        {
            auto* f = task->f_;
            if (write_states.find(f) == write_states.end())
            {
                write_states[f] = {f->location_.chunks_info_.begin()};
            }
            size_t pos1 = write_states[f].seg_iter->first - seg.first;
            size_t pos2 = write_states[f].seg_iter->second - seg.first;
            
            /* Добавляем несколько проверок, на случай, если запись на диск выдаст ошибку */
            if (result >= pos1)
            {
                size_t count_f = std::min(pos2 - pos1 + 1, result - pos1);
                write_states[f].pos_ += count_f;
            }
            if (++write_states[f].seg_iter == f->location_.chunks_info_.end() ||
                        (result == -1 || pos2 > (size_t)result))
            {
                task->task_result_ = write_states[f].pos_;
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
    return std::fread(read_to, count, count, phys_file_.f_);
}

size_t FileManager::WriteToFile(const char* write_from, size_t position, size_t count)
{
    std::fseek(phys_file_.f_, position, SEEK_SET);
    return std::fwrite(write_from, count, count, phys_file_.f_);
}
