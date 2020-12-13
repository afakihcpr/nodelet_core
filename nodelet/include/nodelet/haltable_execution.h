#ifndef NODELET_HALTABLE_EXECUTION_H
#define NODELET_HALTABLE_EXECUTION_H

#include <mutex>
#include <iostream>
#include <chrono>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace nodelet
{

class HaltableExecution
{

public:

HaltableExecution(const std::function<void()>& f, const std::function<void(const uint8_t)>& on_failure, const int timeout, const uint8_t max_attempts) :
 f_ ([&f, this](){f(); done_ = true; cv_.notify_one();}),
 timeout_(timeout),
 max_attempts_(max_attempts),
 on_failure_(on_failure)
{}

uint8_t operator()() {
  for(uint8_t attempt = 0; attempt < max_attempts_; ++attempt)
  {
    done_ = false;
    std::thread t(f_);
    const auto h = t.native_handle();
    t.detach();
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_.wait_for(lock, std::chrono::milliseconds(int(timeout_)) , [this] { return done_.load();}))
    {
      pthread_cancel(h);
      on_failure_(attempt);
    } 
    else 
    {
      return attempt;
    }
  }
  return max_attempts_;
}

private:

std::function<void()> f_;
std::atomic<bool> done_;
std::mutex mutex_;
std::condition_variable cv_;
double timeout_;
uint8_t max_attempts_;
std::function<void(const uint8_t)> on_failure_;

};

} // namespace nodelet

#endif // NODELET_HALTABLE_EXECUTION_H

