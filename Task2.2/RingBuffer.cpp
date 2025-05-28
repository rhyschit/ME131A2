//create the logic for a ring buffer, for 5 buffers of audio data, each buffer is 256 long, and the ring buffer has a head and a tail pointer, and a mutex to protect it.
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>
#include <cstring>
#include "RingBuffer.h"
typedef float SAMPLE; // Define SAMPLE type as float

RingBuffer::RingBuffer(int framesPerBuffer) : head(0), tail(0), count(0)  {
    // Allocate memory for each buffer and push into vector
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        SAMPLE* buf = new SAMPLE[BUFFER_SIZE];
        RingBufferofBuffers.push_back(buf);
    }
}


void RingBuffer::write(const SAMPLE* data, size_t frames) {
    std::lock_guard<std::mutex> lock(mutexes[tail]);
    for (int i = 0; i < framesPerBuffer; i++) {
        RingBufferofBuffers[tail] = data[i];
        tail = (tail + 1) % 5;
        ++count;
    }
    condition.notify_all();
}

void RingBuffer::read(SAMPLE* data, size_t frames) {
    std::unique_lock<std::mutex> lock(mutex);
    for (size_t i = 0; i < frames; ++i) {
        if (count == 0) {
            // Buffer is empty, wait for data
            condition.wait(lock, [this] { return count > 0; });
        }
        data[i] = buffer[tail];
        tail = (tail + 1) % size;
        --count;
    }
    condition.notify_all();
}