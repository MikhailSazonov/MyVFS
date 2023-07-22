#include "CacheManager.hpp"

using namespace TestTask::Cache;
using namespace TestTask::Concurrency;
using namespace std::chrono_literals;

#include <iostream>

namespace TestTask::Cache::Detail
{
    std::atomic<uint8_t> CURRENT_EPOCH = 0;
}

CacheManager::CacheManager(size_t max_capacity, double stop_cleaning_threshold,
Guard<std::unordered_map<std::string, std::unique_ptr<File>>>& guarded_fileset_ref)
    :
    max_capacity_(max_capacity),
    stop_cleaning_threshold_(stop_cleaning_threshold),
    worker_(guarded_fileset_ref)
    {
        worker_.worker_.emplace([this]() {
            this->InvalidateCache(this->worker_);
        });
    }

CacheManager::~CacheManager()
{
    worker_.stop_.store(true, std::memory_order_release);
    worker_.worker_->join();
}

void CacheManager::IncreaseCounter(File* f)
{
    auto ltr = f->last_time_read_.exchange(usage_counter_.fetch_add(1, std::memory_order_acq_rel),
                    std::memory_order_acq_rel);

    if (ltr == (1ull << 63) || ltr == 0)
    {
        Detail::CURRENT_EPOCH.fetch_xor(1, std::memory_order_release);
    }
}

ssize_t CacheManager::Read(File* f, char* buff, size_t len)
{
    Concurrency::ReadGuard g(f->guarded_cache_);
    const auto& cache = g.get();
    if (cache.data_.has_value())
    {
        IncreaseCounter(f);
        cache.data_->copy(buff, len);

        buff[cache.data_->size()] = 0;
        return cache.data_->size();
    }
    // Cache miss
    return -1;  
}

void CacheManager::Write(File* f, const char* buff, size_t len)
{
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
        IncreaseCounter(f);
        std::copy(buff, buff + len, std::back_inserter(*cache.data_));
    }
    
    busy_capacity_.fetch_add(len, std::memory_order_release);
}

void CacheManager::InvalidateCache(CacheWorker& worker)
{
    while (true)
    {
        while (busy_capacity_.load(std::memory_order_acquire) < max_capacity_ && !worker.stop_.load(std::memory_order_acquire))
        {
            std::this_thread::sleep_for(250ms);
        }
        if (worker.stop_.load(std::memory_order_acquire))
        {
            return;
        }

        uint8_t current_epoch;
        bool epoch_changed = false;

        auto epoch_compare = [](uint64_t left, uint64_t right)
        {
            uint8_t epoch = Detail::CURRENT_EPOCH.load(std::memory_order_acquire);
            if (epoch == 0)
            {
                return (left ^ (1ull << 63)) < (right ^ (1ull << 63));
            }
            else
            {
                return left < right;
            }
        };
        std::map<uint64_t, File*, decltype(epoch_compare)> cache_freshness;


        /*
            Избегаем случая, когда эпоха меняется
            во время заполнения std::map.
            Цикл должен почти всегда выполняться 1,
            максимум 2 раза
        */

        while (true)
        {
            current_epoch = Detail::CURRENT_EPOCH.load(std::memory_order_acquire);
            Concurrency::ReadGuard g(worker.guarded_fileset_ref_);
            const auto& fileset = g.get();
            for (const auto& name_file : fileset)
            {
                auto last_time_read = name_file.second->last_time_read_.load(std::memory_order_acquire); 
                cache_freshness[last_time_read] = name_file.second.get();
                uint8_t epoch = Detail::CURRENT_EPOCH.load(std::memory_order_acquire);
                if (epoch != current_epoch)
                {
                    epoch_changed = true;
                    break;
                }
            }
            if (epoch_changed)
            {
                cache_freshness.clear();
                continue;
            }
            break;
        }

        for (auto& freshness_file : cache_freshness)
        {
            File* f = freshness_file.second;
            Concurrency::WriteGuard g(f->guarded_cache_);
            auto& f_cache = g.get();

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
