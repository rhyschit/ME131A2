#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <vector>
#include <mutex>
#include <condition_variable>

typedef float SAMPLE;

class RingBuffer {
public:
    RingBuffer(int framesPerBuffer);  // Constructor

    void write(const SAMPLE* data, size_t frames);
    void read(SAMPLE* data, size_t frames);

private:
    static const int NUM_BUFFERS = 5;     // Number of buffers in ring
    static const int BUFFER_SIZE = 256;   // Size of each buffer

    std::vector<SAMPLE*> RingBufferofBuffers;     // Ring of buffer pointers
    int head;                             // Write index
    int tail;                             // Read index
    int count;                            // Number of filled buffers
    int framesPerBuffer;                  // Frames per buffer

    std::array<std::mutex, NUM_BUFFERS> mutexes;
    std::condition_variable condition;
};

#endif  // RINGBUFFER_H_