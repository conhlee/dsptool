#ifndef DSPPROCESS_H
#define DSPPROCESS_H

#include "files.h"

#include "common.h"

#define DSP_FORMAT_ADPCM (0x0000)

// Never used.
//#define DSP_FORMAT_PCMU8 (0x0009)
//#define DSP_FORMAT_PCM16 (0x000A)

typedef struct __attribute((packed)) {
    u32 sampleCount;
    u32 adpcmNibbleCount; // Includes frame headers
    u32 sampleRate;

    u16 isLooped;

    u16 format; // Compare to DSP_FORMAT_ADPCM

    u32 loopStartOffset; // Start offset for loop
    u32 loopEndOffset; // End offset for loop

    u32 _currentAddress; // Not used by us, leave at 0

    s16 decodeCoefficients[8][2];

    u16 _unk3C;

    u16 initialPredictor; // Matches first frame header
    u16 initialPredictedSampleA;
    u16 initialPredictedSampleB;

    u16 loopContextPredictor;
    u16 loopContextPredictedSampleA;
    u16 loopContextPredictedSampleB;

    u16 _pad[11];
} DspFileHeader;

void DspPreprocess(u8* dspData);

u32 DspGetSampleCount(u8* dspData);
u32 DspGetSampleRate(u8* dspData);

// Get decoded PCM16 sample data (dynamically allocated).
s16* DspDecodeSamples(u8* dspData);

#endif // DSPPROCESS_H
