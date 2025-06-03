// Include the libraries 
#include <stdio.h>             
#include <stdlib.h>             
#include <iostream>             
#include "portaudio.h"          // PortAudio API for audio streaming
#include "smbPitchShift.h"      // Pitch shifting algorithm header

// Define audio stream configuration
#define SAMPLE_RATE 44100       
#define FRAMES_PER_BUFFER 256   
#define NUM_CHANNELS 1          
#define SAMPLE_TYPE paFloat32   
typedef float SAMPLE;           

int main(void) {
    PaStream *stream;                   // Pointer to PortAudio stream
    PaError err;                        // Variable to store PortAudio error codes
    SAMPLE buffer[FRAMES_PER_BUFFER];  // Audio buffer for a block of samples

    // Initialize the PortAudio library
    err = Pa_Initialize();
    if (err != paNoError) goto error;  // If initialization fails, jump to error handling

    // Open the default input and output stream with the specified parameters
    err = Pa_OpenDefaultStream(&stream,
                               NUM_CHANNELS, NUM_CHANNELS,
                               SAMPLE_TYPE,
                               SAMPLE_RATE,
                               FRAMES_PER_BUFFER,
                               NULL, NULL);
    if (err != paNoError) goto error;

    // Start the audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;

    printf("Audio passthrough started. Speak ...\n");

    // Main processing loop — runs indefinitely
    while (1) {
        // Read audio input into buffer
        err = Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
        if (err && err != paInputOverflowed) goto error;  // Allow non-fatal overflow

        // Apply pitch shifting on the buffer in-place
        smbPitchShift(2,
                      FRAMES_PER_BUFFER,
                      2048,
                      8,
                      SAMPLE_RATE,
                      buffer, buffer);

        // Write the processed audio buffer to output
        err = Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
        if (err && err != paOutputUnderflowed) goto error;  // Allow non-fatal underflow
    }

    // Stop and close the stream
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;

error:
    // Print error message and clean up PortAudio resources
    fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
    if (stream) Pa_CloseStream(stream);
    Pa_Terminate();
    return -1;
}
