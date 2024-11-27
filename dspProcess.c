#include "dspProcess.h"

#include <stdlib.h>

#define BYTES_PER_FRAME (7)

void DspPreprocess(u8* dspData) {
    DspFileHeader* fileHeader = (DspFileHeader*)dspData;

    fileHeader->format = __builtin_bswap16(fileHeader->format);
    if (fileHeader->format != DSP_FORMAT)
        panic("DSP format is nonmatching");

    fileHeader->sampleCount = __builtin_bswap32(fileHeader->sampleCount);
    fileHeader->adpcmNibbleCount = __builtin_bswap32(fileHeader->adpcmNibbleCount);
    fileHeader->sampleRate = __builtin_bswap32(fileHeader->sampleRate);

    fileHeader->isLooped = __builtin_bswap16(fileHeader->isLooped);

    // fileHeader->format = __builtin_bswap16(fileHeader->format);

    fileHeader->loopStartOffset = __builtin_bswap32(fileHeader->loopStartOffset);
    fileHeader->loopEndOffset = __builtin_bswap32(fileHeader->loopEndOffset);

    fileHeader->_unk18 = __builtin_bswap32(fileHeader->_unk18);

    for (unsigned i = 0; i < 8; i++) {
        fileHeader->decodeCoefficients[i][0] =
            __builtin_bswap16(fileHeader->decodeCoefficients[i][0]);
        fileHeader->decodeCoefficients[i][1] =
            __builtin_bswap16(fileHeader->decodeCoefficients[i][1]);
    }

    fileHeader->_unk3C = __builtin_bswap16(fileHeader->_unk3C);

    fileHeader->initialPredictor = __builtin_bswap16(fileHeader->initialPredictor);
    fileHeader->initialPredictedSampleA = __builtin_bswap16(fileHeader->initialPredictedSampleA);
    fileHeader->initialPredictedSampleB = __builtin_bswap16(fileHeader->initialPredictedSampleB);

    fileHeader->loopContextPredictor = __builtin_bswap16(fileHeader->loopContextPredictor);
    fileHeader->loopContextPredictedSampleA = __builtin_bswap16(fileHeader->loopContextPredictedSampleA);
    fileHeader->loopContextPredictedSampleB = __builtin_bswap16(fileHeader->loopContextPredictedSampleB);
}

u32 DspGetSampleCount(u8* dspData) {
    return ((DspFileHeader*)dspData)->sampleCount;
}
u32 DspGetSampleRate(u8* dspData) {
    return ((DspFileHeader*)dspData)->sampleRate;
}

static const s8 lutNibbleS8[16] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    -8, -7, -6, -5, -4, -3, -2, -1
};
static inline s8 nibbleLo(u8 byte) { return lutNibbleS8[(byte >> 0) & 0xF]; }
static inline s8 nibbleHi(u8 byte) { return lutNibbleS8[(byte >> 4) & 0xF]; }
 
static inline s16 clampS16(s32 val) {
    if (val < -32768)
        return -32768;
    if (val > 32767)
        return 32767;

    return (s16)val;
}

s16* DspDecodeSamples(u8* dspData) {
    DspFileHeader* fileHeader = (DspFileHeader*)dspData;

    s16* samples = (s16*)malloc(fileHeader->sampleCount * sizeof(s16));
    if (!samples)
        panic("malloc failed (samples)");

    s16 predictedSampleA = fileHeader->initialPredictedSampleA;
    s16 predictedSampleB = fileHeader->initialPredictedSampleB;

    const s16* samplesEnd = samples + fileHeader->sampleCount;
    s16* nextSample = samples;
    u8* adpcmData = (u8*)(fileHeader + 1);

    while (nextSample < samplesEnd) {
        u8 headerByte = *(adpcmData++);
        u16 scale = 1 << (headerByte & 0xF);
        u8 coefficientIdx = (headerByte >> 4);

        s16 coefficientA = fileHeader->decodeCoefficients[coefficientIdx][0];
        s16 coefficientB = fileHeader->decodeCoefficients[coefficientIdx][1];

        for (unsigned i = 0; (i < BYTES_PER_FRAME) && (nextSample < samplesEnd); i++) {
            u8 sampleByte = *(adpcmData++);
            
            for (unsigned j = 0; (j < 2) && (nextSample < samplesEnd); j++) {
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
