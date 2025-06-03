#include "RingBuffer.h"

// Constructor: initialize the buffer with specified capacity, all slots set to nullptr
RingBuffer::RingBuffer(size_t size)
    : buffer(size, nullptr), capacity(size), head(0), tail(0) {}

// Destructor: no dynamic memory to manually free here; cleanup is handled in clear()
RingBuffer::~RingBuffer() {}

// Push a pointer to a SAMPLE into the buffer
bool RingBuffer::push(SAMPLE* item) {
    std::lock_guard<std::mutex> lock(mtx); // Ensure thread-safe access

    // If the current head slot is not empty, buffer is full at this position
    // (this should not occur if external semaphore logic is correctly controlling access)
    if (buffer[head] != nullptr) {
        return false;
    }

    buffer[head] = item;                  // Place the new item into the buffer at head
    head = (head + 1) % capacity;         // Move head forward, wrap around if needed
    return true;
}

// Pop a pointer to a SAMPLE from the buffer
bool RingBuffer::pop(SAMPLE*& item) {
    std::lock_guard<std::mutex> lock(mtx); // Ensure thread-safe access

    item = buffer[tail];                   // Retrieve the item at the tail
    if (item == nullptr) {                // If slot is empty, nothing to pop
        return false;
    }

    buffer[tail] = nullptr;               // Clear the slot for safety and reusability
    tail = (tail + 1) % capacity;         // Move tail forward, wrap around if needed
    return true;
}

// Clear the entire buffer and free all dynamically allocated SAMPLE arrays
void RingBuffer::clear() {
    std::lock_guard<std::mutex> lock(mtx); // Ensure thread-safe access

    // Iterate over all slots and delete non-null SAMPLE arrays
    for (size_t i = 0; i < capacity; ++i) {
        if (buffer[i] != nullptr) {
            delete[] buffer[i];           // Free dynamically allocated memory
            buffer[i] = nullptr;          // Mark slot as empty
        }
    }

    // Reset head and tail positions to start fresh
    head = 0;
    tail = 0;
}
