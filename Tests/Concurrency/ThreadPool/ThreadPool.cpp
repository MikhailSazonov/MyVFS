#include "ThreadPool.hpp"

#include <iostream>

namespace Executors {

thread_local ThreadPool* current_pool{nullptr};

ThreadPool::ThreadPool(size_t threads)
    : threads_(threads) {
}

void ThreadPool::Start() {
  for (size_t i = 0; i < threads_; ++i) {
    workers_.emplace_back([this]() {
      current_pool = this;
      for (;;) {
        {
          std::optional<Task> current_task = task_queue_.Take();
          if (current_task == std::nullopt || stop_processing_tasks_.load()) {
            break;
          }
          (*current_task)();
        }
        {
          std::unique_lock lock(mutex_);
          if (tasks_count_.fetch_sub(1) == 1) {
            wake_if_no_tasks_.notify_one();
          }
        }
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  assert(stop_processing_tasks_);
}

void ThreadPool::Submit(Task task) {
  tasks_count_.fetch_add(1);
  task_queue_.Put(task);
}

ThreadPool* ThreadPool::Current() {
  return current_pool;
}

void ThreadPool::WaitIdle() {
  std::unique_lock lock(mutex_);
  while (tasks_count_.load() > 0) {
    wake_if_no_tasks_.wait(lock);
  }
}

void ThreadPool::Stop() {
  stop_processing_tasks_.store(true);
  task_queue_.Close();
  for (size_t i = 0; i < threads_; ++i) {
    workers_[i].join();
  }
}

}

