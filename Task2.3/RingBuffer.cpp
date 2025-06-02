#include "RingBuffer.h"

RingBuffer::RingBuffer(size_t size)
    : buffer(size, nullptr), capacity(size), head(0), tail(0) {}

RingBuffer::~RingBuffer() {}

bool RingBuffer::push(SAMPLE* item) {
    std::lock_guard<std::mutex> lock(mtx);

    // Check if buffer slot is already occupied (should not happen if semaphore logic is right)
    if (buffer[head] != nullptr) {
        return false;
    }

    buffer[head] = item;
    head = (head + 1) % capacity;
    return true;
}

bool RingBuffer::pop(SAMPLE*& item) {
    std::lock_guard<std::mutex> lock(mtx);

    item = buffer[tail];
    if (item == nullptr) {
        return false;
    }

    buffer[tail] = nullptr; // clear for safety
    tail = (tail + 1) % capacity;
    return true;
}
