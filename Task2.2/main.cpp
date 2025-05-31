#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"
#include "smbPitchShift.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <semaphore>
#include <vector>



#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define RING_SIZE 5
#define NUM_CHANNELS 1
#define SAMPLE_TYPE paFloat32
typedef float SAMPLE;

PaError err;

//GENERAL NOTES
//Should try use semaphore try_acquire instead of acquire, as this will not block the thread if the semaphore is not available, and will return false instead.

SAMPLE* audioQueue = new SAMPLE[FRAMES_PER_BUFFER]; // Temporary buffer for processing

std::vector<SAMPLE*> inputVec;
std::vector<SAMPLE*> outputVec;

std::binary_semaphore sem_audioQueue_is_empty(1); // Starts as empty
std::binary_semaphore sem_audioQueue_is_full(0);  // Starts as not full








// input function, for input thread. reads from the microphone and writes to the buffer, and when buffer full, writes this to audioqueue

// output function, for output thread. reads from audioqueue, modifies this buffer, and writes to the speaker

// main function, initializes portaudio, opens a stream, and starts the stream, initialises threads, and starts the threads, 

void InputFunction(PaStream* stream) {
    while(true){

        SAMPLE* newBuffer = new SAMPLE[FRAMES_PER_BUFFER];

        // PaError err;
        err = Pa_ReadStream(stream, newBuffer, FRAMES_PER_BUFFER);
        if (err && err != paInputOverflowed) goto error;
        //Write the pointer to the head of buffer to the inputVec

        inputVec.push_back(newBuffer);

        if (inputVec.size() > 0) {
            // Wait until the audio queue is not full
            sem_audioQueue_is_empty.acquire();   

            // Add the new buffer to the audio queue
            audioQueue = inputVec.front(); // Get the first element from inputVec
            sem_audioQueue_is_full.release(); // Signal that the audio queue is not empty
            inputVec.erase(inputVec.begin()); // Remove the first element from inputVec
        }
    }
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

    while (true) {
        sem_audioQueue_is_full.acquire(); // Wait until the audio queue is not empty

        if (audioQueue != nullptr) { //Check audioqueue has a value in
            SAMPLE* tempBuffer = new SAMPLE[FRAMES_PER_BUFFER]; // Temporary buffer for processing
            tempBuffer = audioQueue; // Get the buffer from the audio queue
            audioQueue = nullptr; // Clear the audio queue pointer
            sem_audioQueue_is_empty.release(); // Signal that the audio queue is empty

            // Apply pitch shift to the buffer
            smbPitchShift(0.75, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, tempBuffer, tempBuffer);

            //Write tempbfuffer into the output vector
            outputVec.push_back(tempBuffer); // Add the processed buffer to the output vector
        }

            // Write the modified buffer to the output stream
            err = Pa_WriteStream(stream, outputVect.front(), FRAMES_PER_BUFFER);
            if (err && err != paOutputUnderflowed) goto error;

             // Free the memory allocated for the buffer
            delete[] outputVec.front(); // Delete the first element from outputVec
            outputVec.erase(outputVec.begin()); // Remove the first element from outputVec --do we need to delete AND erase?
    }
}

int main(void) {
    PaStream *stream;
    // PaError err;
    // SAMPLE buffer[FRAMES_PER_BUFFER];

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

    std::thread inputThread(InputFunction, stream);
    std::thread outputThread(OutputFunction, stream);

    

    // printf("Audio passthrough started. Speak into the mic...\n");
    // while (1) {
    //     err = Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
    //     if (err && err != paInputOverflowed) goto error;

    //     // Optional: apply pitch shift here
    //     //pitch shifter will edit the contents of the buffer before output. 
    //     smbPitchShift(0.75, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, buffer, buffer);
        

    //     err = Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
    //     if (err && err != paOutputUnderflowed) goto error;
    // }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;

error:
    fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
    if (stream) Pa_CloseStream(stream);
    Pa_Terminate();
    return -1;
}





