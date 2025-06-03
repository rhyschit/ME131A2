#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <semaphore>
#include <vector>

#include "portaudio.h"
#include "smbPitchShift.h"
#include "CountingSemaphore.h"
#include "RingBuffer.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define RING_SIZE 5
#define NUM_CHANNELS 1
#define SAMPLE_TYPE paFloat32
typedef float SAMPLE;

PaError err;
std::atomic<bool> running = true;  // Flag to control running state for threads
RingBuffer audioQueue(5);          // Ring buffer holding audio buffers (pointers to SAMPLE arrays)

CountingSemaphore empty(5); // Counting semaphore tracking empty slots in buffer (initially all empty)
CountingSemaphore full(0);  // Counting semaphore tracking full slots in buffer (initially none full)

bool q = false;                // Flag for quit command
bool pitch_shifter_en = false; // Flag to enable/disable pitch shifting
float pitch_shift_factor = 1;  // Pitch shift multiplier factor

// Thread function to read user input from console and update flags accordingly
void UserFunction() {
  while (running) {
    char userInput;
    std::cin >> userInput; 
    
    switch(userInput) {
      case 'q':
        q = true;
        running = false;      // Signal other threads to stop
        full.release();       // Release semaphores to unblock any waiting threads
        empty.release();
        break;
      case 's':
        pitch_shifter_en = true;  // Enable pitch shifting
        break;
      case 'p':
        pitch_shifter_en = false; // Disable pitch shifting
        break;
      case 'u':
        if (pitch_shifter_en && (pitch_shift_factor < 2)) {
          pitch_shift_factor += 0.5;  // Increase pitch shift factor up to max 2.0
        }
        break;
      case 'd':
        if (pitch_shifter_en && (pitch_shift_factor > 0.5)) {
          pitch_shift_factor -= 0.5;  // Decrease pitch shift factor down to min 0.5
        }        
        break;
      default:
        std::cout << "Invalid option. Please enter q, s, p, u or d!" << std::endl;
        break;
    }
    
    // Redundant check (can be removed) - does nothing here:
    if (userInput == 'q') {
      q == true;
    }
  }
}

// Thread function to read audio from input stream, push buffers to ring buffer
void InputFunction(PaStream* stream) {
  while(running) {
    SAMPLE* inputBuffer = new SAMPLE[FRAMES_PER_BUFFER];  // Allocate buffer for input audio frames

    err = Pa_ReadStream(stream, inputBuffer, FRAMES_PER_BUFFER);  // Read audio input
    if (err && err != paInputOverflow) { // Ignore overflow warnings
      fprintf(stderr, "An input error occurred: %s\n", Pa_GetErrorText(err));
      break;
    }

    empty.acquire(); // Wait for empty slot in ring buffer

    // Try to push new buffer into ring buffer
    if (!audioQueue.push(inputBuffer)) {
      delete[] inputBuffer; // Cleanup if push fails (should not happen normally)
      empty.release();      // Undo empty acquire to prevent deadlock
      if (!running) {
        continue;           // If program is stopping, skip error message
      }
      std::cerr << "Push failed unexpectedly!" << std::endl;
    } else {
      full.release();       // Signal a full slot available to output thread
    }
  }
}

// Thread function to pop audio buffers from ring buffer, optionally pitch shift, write to output stream
void OutputFunction(PaStream* stream) {
  while (running) {
    full.acquire();     // Wait for full slot in ring buffer
    SAMPLE* outputBuffer = nullptr;
      
    if (!audioQueue.pop(outputBuffer)) {
      if (!running) {
        delete[] outputBuffer; // Free buffer on shutdown
        continue;              // Skip further processing
      }
      std::cerr << "Pop failed unexpectedly!" << std::endl;
      full.release();          // Undo full acquire on failure
    }
    empty.release();  // Signal an empty slot available to input thread

    // Apply pitch shift effect if enabled
    if (pitch_shifter_en) {
      smbPitchShift(pitch_shift_factor, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, outputBuffer, outputBuffer);
    }

    // Write processed audio buffer to output stream
    err = Pa_WriteStream(stream, outputBuffer, FRAMES_PER_BUFFER);
    delete[] outputBuffer;    // Free buffer after writing
    if (err && err != paOutputUnderflowed) {
      fprintf(stderr, "An output error occurred: %s\n", Pa_GetErrorText(err));
      break;
    }
  }
}

int main(void) {
  PaStream *stream = nullptr;

  err = Pa_Initialize();
  if (err != paNoError) goto error;

  err = Pa_OpenDefaultStream(&stream,
                             NUM_CHANNELS, NUM_CHANNELS,
                             SAMPLE_TYPE,
                             SAMPLE_RATE,
                             FRAMES_PER_BUFFER,
                             NULL, NULL);
  if (err != paNoError) goto error;

  err = Pa_StartStream(stream);
  if (err != paNoError) goto error;

  {
    // Launch threads for user input, audio input, and audio output
    std::thread userThread(UserFunction);
    std::thread inputThread(InputFunction, stream);
    std::thread outputThread(OutputFunction, stream);

    printf("...\n");

    // Busy wait loop, can consider better synchronization if desired
    while (running) {
      asm("nop");
    }

    // Wait for all threads to finish cleanly
    inputThread.join();
    outputThread.join();
    userThread.join();

    audioQueue.clear();  // Clear any remaining buffers in ring buffer
    
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    printf("The end");
    return 0;
  }

error:
  fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
  if (stream) Pa_CloseStream(stream);
  Pa_Terminate();
  return -1;
}
