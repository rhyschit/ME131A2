#pragma once
#include <vector>
#include <mutex>

typedef float SAMPLE; // Define SAMPLE as float for portability and clarity

class RingBuffer {
private:
    std::vector<SAMPLE*> buffer; // Underlying circular buffer storing pointers to SAMPLE
    size_t capacity;             // Maximum number of items the buffer can hold
    size_t head;                 // Index where the next item will be pushed
    size_t tail;                 // Index where the next item will be popped
    std::mutex mtx;              // Mutex for synchronizing access in multithreaded context

public:
    RingBuffer(size_t size);     // Constructor: initialize buffer with given size
    ~RingBuffer();               // Destructor

    // Attempts to insert a SAMPLE* into the buffer.
    // Returns true on success, false if the target slot is already occupied (shouldn't happen with correct semaphore use).
    bool push(SAMPLE* item);

    // Attempts to retrieve a SAMPLE* from the buffer.
    // Returns true on success, and assigns the item to the provided reference.
    // Returns false if the buffer slot is empty (shouldn't happen with correct semaphore use).
    bool pop(SAMPLE*& item);

    void clear();
};
