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

bool q = false;
bool pitch_shifter_en = false;
float pitch_shift_factor = 1;

void UserFunction() {
  while (running) {
    char userInput;
    std::cin >> userInput; 
    
    switch(userInput) {
      case 'q':
        q = true;
        running = false;
        full.release();  // unblock output thread
        empty.release(); // unblock input thread
        break;
      case 's':
        pitch_shifter_en = true;
        break;
      case 'p':
        pitch_shifter_en = false;
        break;
      case 'u':
        if (pitch_shifter_en && (pitch_shift_factor < 2)) {
          pitch_shift_factor += 0.5;
        }
        break;
      case 'd':
        if (pitch_shifter_en && (pitch_shift_factor > 0.5)) {
          pitch_shift_factor -= 0.5;
        }        
        break;
      default:
        std::cout << "Invalid option. Please enter q, s, p, u or d!" << std::endl;
        break;
    }
    
    
    if (userInput == 'q') {
      q == true;
    }
  }
}

void InputFunction(PaStream* stream) {
  while(running) {
    SAMPLE* newBuffer = new SAMPLE[FRAMES_PER_BUFFER];

    err = Pa_ReadStream(stream, newBuffer, FRAMES_PER_BUFFER);
    if (err && err != paOutputUnderflowed) {
      fprintf(stderr, "An input error occurred: %s\n", Pa_GetErrorText(err));
      break;
    }

    empty.acquire(); //wait for a empty slot
    if (!audioQueue.push(newBuffer)) {
      //delete[] newBuffer;
    } else {
      full.release();
    }

  }
  error:
  fprintf(stderr, "An input error occurred: %s\n", Pa_GetErrorText(err));
}

void OutputFunction(PaStream* stream) {
    while (running) {
      full.acquire();   // Wait for full slot
      // SAMPLE* tempBuffer = nullptr;
      // SAMPLE* tempBuffer = new SAMPLE[FRAMES_PER_BUFFER];
      SAMPLE* tempBuffer = nullptr;
        
      if (!audioQueue.pop(tempBuffer)) {
        std::cerr << "Pop failed unexpectedly!" << std::endl;
      }
      empty.release();  // Prevent deadlock

        // Apply pitch shift to the buffer
      if (pitch_shifter_en) {
        smbPitchShift(pitch_shift_factor, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, tempBuffer, tempBuffer);
      }

      // Write the modified buffer to the output stream
      err = Pa_WriteStream(stream, tempBuffer, FRAMES_PER_BUFFER);
      delete[] tempBuffer;
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
      //threads ar ein a new scope, so goto won't cross initialisation...
      std::thread userThread(UserFunction);
      std::thread inputThread(InputFunction, stream);
      std::thread outputThread(OutputFunction, stream);

      printf("...\n");
      while (running) {
        asm("nop");
      }

      inputThread.join();

      outputThread.join();
      userThread.join();

      
      Pa_StopStream(stream);
      Pa_CloseStream(stream);
      Pa_Terminate();

      printf("the end");
      return 0;
    }

error:
    fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
    if (stream) Pa_CloseStream(stream);
    Pa_Terminate();
    return -1;
}





