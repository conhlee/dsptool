#include <stdlib.h>
#include <stdio.h>

#include "files.h"

#include "dspProcess.h"
#include "wavProcess.h"

void usage() {
    printf("usage: dsptool <towav|todsp> <input dsp/wav> [output dsp/wav]\n");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 3)
        usage();

    const int outputSpecified = argc >= 3;

    const char* mode = argv[1];
    const char* inputPath = argv[2];

    char* outputPath = NULL;

    FileHandle inputHndl = { 0 };
    FileHandle outputHndl = { 0 };

    if (strcasecmp(mode, "towav") == 0) {
        outputPath = outputSpecified ? argv[3] : strcat(strdup(inputPath), ".wav");

        inputHndl = FileCreateHandle(inputPath);

        printf("dsptool > towav: exporting to \"%s/\"..\n", outputPath);

        printf("Decoding DSP data..");

        DspPreprocess(inputHndl.data_u8);

        s16* samples = DspDecodeSamples(inputHndl.data_u8);
        u32 sampleCount = DspGetSampleCount(inputHndl.data_u8);
        u32 sampleRate = DspGetSampleRate(inputHndl.data_u8);

        LOG_OK;

        FileDestroyHandle(inputHndl);

        printf("    - Decoded %u samples. (sampleRate=%u)\n", sampleCount, sampleRate);

        printf("Writing WAV to \"%s\"..", outputPath);

        outputHndl = WavBuild(samples, sampleCount, sampleRate, 1);

        FileWriteHandle(outputHndl, outputPath);

        LOG_OK;
    }
    else if (strcasecmp(mode, "todsp") == 0) {
        outputPath = outputSpecified ? argv[3] : strcat(strdup(inputPath), ".dsp");

        panic("Unimplemented");
    }
    else
        usage();
    
    if (!outputSpecified)
        free(outputPath);

    printf("\nAll done!\n");

    return 0;
}