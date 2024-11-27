#include "dspProcess.h"

#include <stdlib.h>

#define BYTES_PER_FRAME (7)

void DspPreprocess(u8* dspData) {
    DspFileHeader* fileHeader = (DspFileHeader*)dspData;

    if (__builtin_bswap16(fileHeader->format) != DSP_FORMAT)
        panic("DSP format is nonmatching");
}

u32 DspGetSampleCount(u8* dspData) {
    return __builtin_bswap32(((DspFileHeader*)dspData)->sampleCount);
}
u32 DspGetSampleRate(u8* dspData) {
    return __builtin_bswap32(((DspFileHeader*)dspData)->sampleRate);
}

static const s8 lutNibbleS8[16] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    -8, -7, -6, -5, -4, -3, -2, -1
};
static inline s8 nibbleLo(u8 byte) { return lutNibbleS8[(byte >> 0) & 0xF]; }
static inline s8 nibbleHi(u8 byte) { return lutNibbleS8[(byte >> 4) & 0xF]; }
 
static s16 clampS16(s32 val) {
    if (val < -32768)
        return -32768;
    if (val > 32767)
        return 32767;

    return (s16)val;
}

s16* DspDecodeSamples(u8* dspData) {
    DspFileHeader* fileHeader = (DspFileHeader*)dspData;

    u32 sampleCount = __builtin_bswap32(fileHeader->sampleCount);

    s16* samples = (s16*)malloc(sampleCount * sizeof(s16));
    if (!samples)
        panic("malloc failed (samples)");

    s16 predictedSampleA = __builtin_bswap16(fileHeader->initialPredictedSampleA);
    s16 predictedSampleB = __builtin_bswap16(fileHeader->initialPredictedSampleB);

    const s16* samplesEnd = samples + sampleCount;
    s16* nextSample = samples;
    u8* adpcmData = (u8*)(fileHeader + 1);

    while (nextSample < samplesEnd) {
        u8 headerByte = *(adpcmData++);
        u16 scale = 1 << (headerByte & 0xF);
        u8 coefficientIdx = (headerByte >> 4);

        s16 coefficientA = __builtin_bswap16(fileHeader->decodeCoefficients[coefficientIdx][0]);
        s16 coefficientB = __builtin_bswap16(fileHeader->decodeCoefficients[coefficientIdx][1]);

        for (unsigned i = 0; i < BYTES_PER_FRAME && nextSample < samplesEnd; i++) {
            u8 sampleByte = *(adpcmData++);
            
            for (unsigned j = 0; j < 2 && nextSample < samplesEnd; j++) {
                s8 adpcmNibble = (j == 0) ? nibbleHi(sampleByte) : nibbleLo(sampleByte);

                s16 sample = clampS16((
                    ((adpcmNibble * scale) * 2048) + 1024 + 
                    (coefficientA * predictedSampleA) + (coefficientB * predictedSampleB)
                ) / 2048);

                predictedSampleB = predictedSampleA;
                predictedSampleA = sample;

                *(nextSample++) = sample;
            }
        }
    }

    return samples;
}
