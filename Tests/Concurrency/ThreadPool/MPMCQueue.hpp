#pragma once

#include <optional>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace Executors
{

  // Unbounded blocking multi-producers/multi-consumers (MPMC) queue

  template <typename T>
  class MPMCQueue
  {
  public:
    template <typename U>
    bool Put(U &&value)
    {
      std::unique_lock lock(mutex_);
      if (closed_)
      {
        return false;
      }
      buffer_.push_back(std::forward<U>(value));
      if_buffer_empty_.notify_one();
      return true;
    }

    std::optional<T> Take()
    {
      std::unique_lock lock(mutex_);
      while (!closed_ && buffer_.empty())
      {
        if_buffer_empty_.wait(lock);
      }
      if (closed_ && buffer_.empty())
      {
        return std::nullopt;
      }
      T value = std::move(buffer_.front());
      buffer_.pop_front();
      return value;
    }

    void Close()
    {
      std::unique_lock lock(mutex_);
      closed_ = true;
      if_buffer_empty_.notify_all();
    }

    bool Empty()
    {
      return buffer_.empty();
    }

  private:
    bool closed_{false};
    std::deque<T> buffer_;
    std::mutex mutex_;
    std::condition_variable if_buffer_empty_;
  };

}
