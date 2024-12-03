#ifndef DSPPROCESS_H
#define DSPPROCESS_H

#include "common.h"

void DspPreprocess(u8* dspData);

u32 DspGetSampleCount(u8* dspData);
u32 DspGetSampleRate(u8* dspData);

// Get decoded PCM16 sample data (dynamically allocated).
s16* DspDecodeSamples(u8* dspData, u32 loopCount, u32* sampleCountOut);

#endif // DSPPROCESS_H
