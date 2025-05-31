#include "CountingSemaphore.h"

CountingSemaphore::CountingSemaphore(int initial_count)
    : count_(initial_count) {}

void CountingSemaphore::acquire() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return count_ > 0; });
    count_--;
}

void CountingSemaphore::release() {
    std::lock_guard<std::mutex> lock(mtx_);
    count_++;
    cv_.notify_one();
}
