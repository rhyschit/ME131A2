#pragma once
#include <vector>
#include <mutex>

typedef float SAMPLE;

class RingBuffer {
private:
    std::vector<SAMPLE*> buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    std::mutex mtx;

public:
    RingBuffer(size_t size);
    ~RingBuffer();

    // Return true if push succeeds
    bool push(SAMPLE* item);

    // Return true if pop succeeds, and writes result into `item`
    bool pop(SAMPLE*& item);
};
