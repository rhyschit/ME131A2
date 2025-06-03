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

#define SAMPLE_RATE 44100            // Audio sample rate in Hz
#define FRAMES_PER_BUFFER 256        // Number of frames per buffer
#define RING_SIZE 5                  // Size of the ring buffer
#define NUM_CHANNELS 1               // Mono audio
#define SAMPLE_TYPE paFloat32        // Sample format (float)
typedef float SAMPLE;                // Define SAMPLE as float for convenience

PaError err;                        // PortAudio error code variable
std::atomic<bool> running = true;  // Atomic flag to control running state across threads
RingBuffer audioQueue(5);          // Ring buffer with size 5 to hold audio buffers

CountingSemaphore empty(5);        // Semaphore tracking empty slots in the buffer (initially all empty)
CountingSemaphore full(0);         // Semaphore tracking full slots in the buffer (initially none full)


// Function to read audio input from device into buffers
void InputFunction(PaStream* stream) {
  while(running) {

    // Allocate a buffer for audio input
    SAMPLE* inputBuffer = new SAMPLE[FRAMES_PER_BUFFER];

    // Read audio data from input stream
    err = Pa_ReadStream(stream, inputBuffer, FRAMES_PER_BUFFER);
    // If error other than overflow, jump to error handler
    if (err && err != paInputOverflowed) goto error;

    empty.acquire();  // Wait until there is an empty slot in the queue

    // Try to push the input buffer into the audio queue
    if (!audioQueue.push(inputBuffer)) {
      // Push failed - output error, free buffer, release empty slot and continue loop
      std::cerr << "Push failed unexpectedly!" << std::endl;
      delete[] inputBuffer;
      empty.release();
      continue;
    } else {
      // Successfully pushed, so release one full slot semaphore
      full.release();
    }
  }
  error:
  // Print input error message and exit input thread
  fprintf(stderr, "An input error occurred: %s\n", Pa_GetErrorText(err));
}


// Function to read audio buffers from the queue and write them to output
void OutputFunction(PaStream* stream) {
    while (running) {
      full.acquire();   // Wait until there is a full slot (buffer available to process)
      SAMPLE* outputBuffer = nullptr;
        
      // Attempt to pop a buffer from the audio queue
      if (!audioQueue.pop(outputBuffer)) {
        // Pop failed unexpectedly - output error, release semaphore, continue loop
        std::cerr << "Pop failed unexpectedly!" << std::endl;
        full.release();
        continue;
      }

      empty.release();  // Release an empty slot to signal space available

      // Apply pitch shifting effect on the audio buffer
      // Fixed factor 0.75 for now; parameters: pitch shift, buffer size, FFT size, oversampling, sample rate
      smbPitchShift(2, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, outputBuffer, outputBuffer);

      // Write processed audio buffer to output stream
      err = Pa_WriteStream(stream, outputBuffer, FRAMES_PER_BUFFER);

      // Free allocated buffer memory after use
      delete[] outputBuffer;

      // Handle write errors except for underflow (which can be ignored)
      if (err && err != paOutputUnderflowed) goto error;
    }
    error:
    // Print output error message and exit output thread
    fprintf(stderr, "An output error occurred: %s\n", Pa_GetErrorText(err));
}


int main(void) {
    PaStream *stream = nullptr;

    // Initialize PortAudio library
    err = Pa_Initialize();
    if (err != paNoError) goto error;

    // Open the default audio stream with specified input/output channels and sample format
    err = Pa_OpenDefaultStream(&stream,
                               NUM_CHANNELS, NUM_CHANNELS,
                               SAMPLE_TYPE,
                               SAMPLE_RATE,
                               FRAMES_PER_BUFFER,
                               NULL, NULL);
    if (err != paNoError) goto error;

    // Start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;

    {
      // Create separate threads for audio input and output processing
      // Threads scoped here to prevent goto crossing initialization
      std::thread inputThread(InputFunction, stream);
      std::thread outputThread(OutputFunction, stream);

      printf("...\n");

      // Main thread idle loop, waiting for running to be set false
      while (running) {
        asm("nop");  // No operation - keep CPU busy
      }

      // Wait for threads to finish before proceeding
      inputThread.join();
      outputThread.join();

      // Clear ring buffer resources
      audioQueue.clear();
      
      // Stop and close audio stream, then terminate PortAudio
      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      Pa_Terminate();

      return 0;
    }

error:
    // Error handling: print error, close stream and terminate PortAudio
    fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
    if (stream) Pa_CloseStream(stream);
    Pa_Terminate();
    return -1;
}
