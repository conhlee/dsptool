#ifndef DSPPROCESS_H
#define DSPPROCESS_H

#include "files.h"

#include "common.h"

#define DSP_FORMAT (0)

typedef struct __attribute((packed)) {
    u32 sampleCount;
    u32 adpcmNibbleCount;
    u32 sampleRate;

    u16 isLooped;

    u16 format; // Compare to DSP_FORMAT

    u32 loopStartOffset;
    u32 loopEndOffset;

    u32 _unk18;

    s16 decodeCoefficients[8][2];

    u16 _unk3C;

    u16 initialPredictor; // Matches first frame header
    u16 initialPredictedSampleA;
    u16 initialPredictedSampleB;

    u16 loopContextPredictor;
    u16 loopContextPredictedSampleA;
    u16 loopContextPredictedSampleB;

    u16 _reserved[11];
} DspFileHeader;

void DspPreprocess(u8* dspData);

u32 DspGetSampleCount(u8* dspData);
u32 DspGetSampleRate(u8* dspData);

// Get decoded PCM16 sample data (dynamically allocated).
s16* DspDecodeSamples(u8* dspData);

#endif // DSPPROCESS_H
