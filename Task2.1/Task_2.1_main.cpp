#include <stdio.h>
#include <stdlib.h>
#include "portaudio.h"
#include "smbPitchShift.h"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256
#define NUM_CHANNELS 1
#define SAMPLE_TYPE paFloat32
typedef float SAMPLE;

int main(void) {
    PaStream *stream;
    PaError err;
    SAMPLE buffer[FRAMES_PER_BUFFER];

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

    printf("Audio passthrough started. Speak into the mic...\n");
    while (1) {
        err = Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
        if (err && err != paInputOverflowed) goto error;

        // Optional: apply pitch shift here
        //pitch shifter will edit the contents of the buffer before output. 
        smbPitchShift(0.75, FRAMES_PER_BUFFER, 2048, 8, SAMPLE_RATE, buffer, buffer);
        

        err = Pa_WriteStream(stream, buffer, FRAMES_PER_BUFFER);
        if (err && err != paOutputUnderflowed) goto error;
    }

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