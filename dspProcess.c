#include "dspProcess.h"

#include <stdlib.h>

#include <string.h>

#define BYTES_PER_FRAME (7)

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

    u32 loopStartOffset; // Start offset for loop (in nibbles)
    u32 loopEndOffset; // End offset for loop (in nibbles)

    u32 startOffset; // Start offset for playing (in nibbles)

    s16 decodeCoefficients[8][2];

    u16 _unk3C;

    u16 initialPredictor; // Matches first frame header
    s16 initialPredictedSampleA;
    s16 initialPredictedSampleB;

    u16 loopContextPredictor;
    s16 loopContextPredictedSampleA;
    s16 loopContextPredictedSampleB;

    u16 _pad[11];
} DspFileHeader;

void DspPreprocess(u8* dspData) {
    DspFileHeader* fileHeader = (DspFileHeader*)dspData;

    fileHeader->format = __builtin_bswap16(fileHeader->format);

    if (fileHeader->format != DSP_FORMAT_ADPCM)
        panic("Invalid DSP format (%u)", (unsigned)fileHeader->format);

    fileHeader->sampleCount = __builtin_bswap32(fileHeader->sampleCount);
    fileHeader->adpcmNibbleCount = __builtin_bswap32(fileHeader->adpcmNibbleCount);
    fileHeader->sampleRate = __builtin_bswap32(fileHeader->sampleRate);

    fileHeader->isLooped = __builtin_bswap16(fileHeader->isLooped);

    // fileHeader->format = __builtin_bswap16(fileHeader->format);

    fileHeader->loopStartOffset = __builtin_bswap32(fileHeader->loopStartOffset);
    fileHeader->loopEndOffset = __builtin_bswap32(fileHeader->loopEndOffset);

    fileHeader->startOffset = __builtin_bswap32(fileHeader->startOffset);

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

s16* DspDecodeSamples(u8* dspData, u32 loopCount, u32* sampleCountOut) {
    DspFileHeader* fileHeader = (DspFileHeader*)dspData;

    s16* samples = (s16*)malloc(fileHeader->sampleCount * loopCount * sizeof(s16));
    if (!samples)
        panic("malloc failed (samples)");

    u8 predictor = fileHeader->initialPredictor;
    s16 predictedSampleA = fileHeader->initialPredictedSampleA;
    s16 predictedSampleB = fileHeader->initialPredictedSampleB;

    const s16* samplesEnd = samples + (fileHeader->sampleCount * loopCount);
    s16* nextSample = samples;
    const u8* adpcmData = (u8*)(fileHeader + 1) + (fileHeader->startOffset / 2);

    const u8* adpcmDataLoopStart = (u8*)(fileHeader + 1) + (fileHeader->loopStartOffset / 2);
    const u8* adpcmDataLoopEnd   = (u8*)(fileHeader + 1) + (fileHeader->loopEndOffset / 2);

    *sampleCountOut = 0;

    unsigned loopedTimes = 0;

    while (nextSample < samplesEnd) {
        if (adpcmData >= adpcmDataLoopEnd) {
            if (loopedTimes++ >= loopCount)
                break;

            adpcmData = adpcmDataLoopStart;

            predictor = fileHeader->loopContextPredictor;
            predictedSampleA = fileHeader->initialPredictedSampleA;
            predictedSampleB = fileHeader->initialPredictedSampleB;
        }

        u16 scale = 1 << (predictor & 0xF);
        u8 coefficientIdx = (predictor >> 4);

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

                (*sampleCountOut)++;
            }
        }

        predictor = *adpcmData;
        adpcmData++;
    }

    return samples;
}