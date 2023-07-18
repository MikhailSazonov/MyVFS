#include "CacheManager.hpp"

using namespace TestTask::Cache;

bool compare_with_epoch(uint64_t left, uint64_t right)
{
    auto current = Detail::CURRENT_EPOCH.load(std::memory_order_acquire);
    if (current == 0)
    {
        return (left ^ (1ull << 63)) < (right ^ (1ull << 63));
    }
    else
    {
        return left < right;
    }
}

CacheManager::CacheManager(size_t max_capacity,
Concurrency::Guard<std::unordered_map<std::string, File*>> guarded_fileset_ref)
    :
    worker_{[this]() {
        this->InvalidateCache(this->worker_);
    }, guarded_fileset_ref}
    max_capacity_(max_capacity) {}

ssize_t CacheManager::Read(File* f, char* buff, size_t len)
{
    Concurrency::ReadGuard g(f->guarded_cache_);
    const auto& cache = g.get();
    if (cache.data_.has_value())
    {
        cache.data_->copy(buff, len);
        f->last_time_read_ = usage_counter_.fetch_add(1. std::memory_order_acq_rel);

        if ((f->last_time_read_ >> 63) != Detail::CURRENT_EPOCH.load(std::memory_order_acquire))
        {
            Detail::CURRENT_EPOCH.fetch_xor(1, std::memory_order_release);
        }

        buff[len] = 0;
        return len;
    }
    // Cache miss
    return -1;
}

void CacheManager::Write(File* f, const char* buff, size_t len)
{
    Concurrency::WriteGuard g(f->guarded_cache_);
    auto& cache = g.get();
    
    if (cache.data_.has_value())
    {
        busy_capacity_.fetch_sub(cache.data_->size());
        cache.data_->clear();
    }
    else
    {
        cache.data_.emplace();
    }
    std::copy(buff, buff + len, std::back_inserter(*cache.data_));
    
    
    if (busy_capacity_.fetch_add(len, std::memory_order_acq_rel) >= max_capacity_ - len) {
        std::unique_lock l(worker.mutex_);
        worker_.if_clean_.notify_one();
    }
}

void CacheManager::InvalidateCache(CacheWorker worker)
{
    while (true)
    {
        while (busy_capacity_.load(std::memory_order_acquire) < max_capacity_)
        {
            std::unique_lock l(worker.mutex_);
            worker.if_clean_.wait();
        }
        std::map<uint64_t, File*, compare_with_epoch> cache_freshness;
        {
            Concurrency::ReadGuard g(worker.guarded_fileset_ref_);
            const auto& fileset = g.get();
            for (const auto& name_file : fileset)
            {
                auto last_time_read = name_file.second->last_time_read_.load(std::memory_order_acquire); 
                cache_freshness_[last_time_read] = name_file.second;
            }
        }
        for (auto& freshness_file : cache_freshness)
        {
            File* f = freshness_file.second;
            Concurrency::WriteGuard g(f->guarded_cache_);
            auto& f_cache = g.get();

            // Не уверен, что никакое отношение happens-before не нарушится, если переставить порядок
            size_t file_size = f_cache.data_->size();
            f_cache.data_.reset();
            if (busy_capacity_.fetch_sub(file_size, std::memory_order_acq_rel) <
                static_cast<uint64_t>(max_capacity_ * stop_cleaning_threshold_) + file_size)
            {
                break;
            }
        }
    }
}
