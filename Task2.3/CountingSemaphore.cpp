#include "CountingSemaphore.h"

CountingSemaphore::CountingSemaphore(int initial_count)
    : count_(initial_count) {}

// Acquire a resource (decrement the count), blocking if count is 0
void CountingSemaphore::acquire() {
    std::unique_lock<std::mutex> lock(mtx_);       // Lock the mutex to access shared data
    cv_.wait(lock, [this]() { return count_ > 0; }); // Wait until count_ > 0
    count_--;                                       // "Take" a resource by decrementing the count
}

// Release a resource (increment the count), and notify one waiting thread
void CountingSemaphore::release() {
    std::lock_guard<std::mutex> lock(mtx_);  // Lock the mutex to access shared data
    count_++;                                // "Return" a resource by incrementing the count
    cv_.notify_one();                        // Wake up one waiting thread (if any)
}
