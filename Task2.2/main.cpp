#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"
#include "smbPitchShift.h"
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define NUM_CHANNELS 1
#define SAMPLE_TYPE paFloat32
typedef float SAMPLE;

PaError err;
SAMPLE buffer[FRAMES_PER_BUFFER];
//make a ring buffercalled audio ringOfBuffers to hold 5 buffers of audio data. It should have a head and a tail pointer, and a mutex to protect it. Each elemen in the ring buffer is a 256 long audio buffer of type SAMPLE.
std::mutex audioMutex;
std::condition_variable audioCondition;






// input function, for input thread. reads from the microphone and writes to the buffer, and when buffer full, writes this to audioqueue

// output function, for output thread. reads from audioqueue, modifies this buffer, and writes to the speaker

// main function, initializes portaudio, opens a stream, and starts the stream, initialises threads, and starts the threads, 

void InputFunction(PaStream* stream) {
    // PaError err;
    err = Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
    if (err && err != paInputOverflowed) goto error;
}

void OutputFunction(PaStream* stream) {

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