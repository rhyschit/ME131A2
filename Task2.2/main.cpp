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
std::atomic<bool> running = true;
RingBuffer audioQueue(5);

std::vector<SAMPLE*> inputVec;
std::vector<SAMPLE*> outputVec;

CountingSemaphore empty(5); // 5 empty slots
CountingSemaphore full(0);  // 0 full slots


void InputFunction(PaStream* stream) {
  
  while(running) {
    SAMPLE* newBuffer = new SAMPLE[FRAMES_PER_BUFFER];

    err = Pa_ReadStream(stream, newBuffer, FRAMES_PER_BUFFER);
    if (err && err != paInputOverflowed) goto error;
    std::cout << "First sample before shift: " << newBuffer[0] << std::endl;


    empty.acquire(); //wait for a empty slot
    if (!audioQueue.push(newBuffer)) {
      std::cerr << "Push failed unexpectedly!" << std::endl;
      delete[] newBuffer;
    } else {
      full.release();
    }
  }
  error:
  fprintf(stderr, "An input error occurred: %s\n", Pa_GetErrorText(err));
}

void OutputFunction(PaStream* stream) {
    //Thinkign about when pitch shift can take place, cannot be while reading from the audio queue, as then we will occupy the semaphore for the 
    //duration of the pitch shift, and the input thread will not be able to write to the audio queue. Could be done so we grab audio data from outputvec
    // and then pitch shift data, then call Pa_WriteStream to write the data to the output stream. This could cause some issue as write data is a blocking function
    // until there is space in the internal buffer, are there any issues with this?
    //Other option is to have a seperate buffer for the output thread, and then pitch shift the data in that buffer, and then write to the output stream.

    //Outputfunction will only occur as fast as the output can write 256 samples, e.g every 5ms. This is because PAwritestream is a blocking function
    // and will wait until there is space in the internal buffer. Any computation has to be complete in one cycle, as the function will only run once
    //for every 256 samples.

    //is it worth leaving Pa_writeStream outside of the if(audioQueue) block? could mean even if there is no data ready in time, it will still output
    //something?? Even if it is re printing the last buffer

    //Remember to delete the buffer after use, to avoid memory leaks
    //Remember to release the semaphore when done with the audio queue

    while (running) {
        full.acquire();   // Wait for full slot
        SAMPLE* tempBuffer = nullptr;
        
        
        if (!audioQueue.pop(tempBuffer)) {
          std::cerr << "Pop failed unexpectedly!" << std::endl;
          empty.release();  // Prevent deadlock
          continue;
        }

        // Apply pitch shift to the buffer
        printf("pitch shifting");
        smbPitchShift(0.75, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, tempBuffer, tempBuffer);


        // Write the modified buffer to the output stream
        err = Pa_WriteStream(stream, tempBuffer, FRAMES_PER_BUFFER);
        if (err && err != paOutputUnderflowed) goto error;

    }
    error:
    fprintf(stderr, "An output error occurred: %s\n", Pa_GetErrorText(err));
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
      //threads ar ein a new scope, so goto won't cross initialisation...
      std::thread inputThread(InputFunction, stream);
      std::thread outputThread(OutputFunction, stream);

      printf("...\n");
      while (running) {
        asm("nop");
      }

      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      Pa_Terminate();

      inputThread.join();
      outputThread.join();
      return 0;
    }

error:
    fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
    if (stream) Pa_CloseStream(stream);
    Pa_Terminate();
    return -1;
}





