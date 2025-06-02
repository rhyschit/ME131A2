#ifndef COUNTINGSEMAPHORE_H
#define COUNTINGSEMAPHORE_H

#include <mutex>
#include <condition_variable>

class CountingSemaphore {
public:
    // Constructor: set the initial number of permits
    explicit CountingSemaphore(int initial_count);

    // Acquire a permit (block if none are available)
    void acquire();

    // Release a permit (wake one waiting thread, if any)
    void release();

    // Deleted copy/move constructors and assignment for safety
    CountingSemaphore(const CountingSemaphore&) = delete;
    CountingSemaphore& operator=(const CountingSemaphore&) = delete;

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    int count_; // current number of permits
};

#endif // COUNTINGSEMAPHORE_H
