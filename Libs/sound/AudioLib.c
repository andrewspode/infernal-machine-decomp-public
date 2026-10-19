#include "AudioLib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(AudioLib_countfit_1, 1)
/* end count-fit */

#define AUDIOLIB_SWAP16(x) ((((uint16_t)(x) & 0xFFu) << 8) | ((uint16_t)(x) >> 8))
#define AUDIOLIB_SWAP32(x) (((((x) >> 24) | (((x) >> 8) & 0xFF00)) | (((x) & 0xFF00) << 8)) | ((x) << 24))
#define AUDIOLIB_NOSWAP16(x) (x)
#define AUDIOLIB_NOSWAP32(x) (x)

// Writes one header field in the given byte order through a temporary and advances the pointer.
#define AUDIOLIB_WRITE16(p, val, order) { uint16_t field = (uint16_t)(val); field = order(field); *(uint16_t*)(p) = field; (p) += 2; }
#define AUDIOLIB_WRITE32(p, val, order) { uint32_t field = (val); field = order(field); *(uint32_t*)(p) = field; (p) += 4; }

static const int8_t AudioLib_aIndex2Bit[4] = { -1, 4, -1, 4 };
static const int8_t AudioLib_aIndex3Bit[8] = { -1, -1, 2, 6, -1, -1, 2, 6 };
static const int8_t AudioLib_aIndex4Bit[16] = { -1, -1, -1, -1, 1, 2, 4, 6, -1, -1, -1, -1, 1, 2, 4, 6 };
static const int8_t AudioLib_aIndex5Bit[32] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6,
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6
};
static const int8_t AudioLib_aIndex6Bit[64] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6
};
static const int8_t AudioLib_aIndex7Bit[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

const uint16_t AudioLib_aStepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

const uint8_t AudioLib_aStepBits[89] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7
};

// The AIFF COMM chunk's 80-bit extended sample rates (11025, 22050 and 44100 Hz), read only by the debug-only
// WriteWaveFileHeader; retail keeps all three, unreferenced, at the end of this object's .rdata (0x0050A3B8).
static const uint8_t AudioLib_aAiffRate11025[10] = { 0x40, 0x0C, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t AudioLib_aAiffRate22050[10] = { 0x40, 0x0D, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const uint8_t AudioLib_aAiffRate44100[10] = { 0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const int8_t* AudioLib_aIndexTableTable[8] = {
    NULL,
    NULL,
    AudioLib_aIndex2Bit,
    AudioLib_aIndex3Bit,
    AudioLib_aIndex4Bit,
    AudioLib_aIndex5Bit,
    AudioLib_aIndex6Bit,
    AudioLib_aIndex7Bit
};

uint16_t AudioLib_aDeltaTable[89 * 64] = { 0 };
int AudioLib_bDeltaTableInitialized = 0;

// Returns pointer to snd data
const uint8_t* J3DAPI AudioLib_ParseWaveFileHeader(const uint8_t* pData, int* pType, uint32_t* pSampleRate, uint32_t* pBitsPerSample, uint32_t* pNumChannels, uint32_t* pExtraInfo, uint32_t* pSoundDataSize, uint32_t* pExtraDataOffset, uint32_t* pSoundDataOffset)
{
    int bCompressed;
    int count;

    bCompressed = 0;
    if ( pType )
    {
        *pType = 0;
    }

    if ( !pData )
    {
        return NULL;
    }

    if ( pExtraDataOffset )
    {
        *pExtraDataOffset = 0;
    }

    if ( pSoundDataOffset )
    {
        *pSoundDataOffset = 0;
    }

    if ( !memcmp(pData, "MCMP", 4) )
    {
        count = AUDIOLIB_SWAP16(*(uint16_t*)(pData + 4));
        pData += (9 * count) + 6;
        count = AUDIOLIB_SWAP16(*(uint16_t*)pData);
        pData += count + 2;
        bCompressed = 1;
    }

    if ( !memcmp(pData, "INDYWV", 6) )
    {
        uint32_t size;

        size = 0;
        if ( pType )
        {
            *pType = 6;
        }

        if ( pSampleRate )
        {
            *pSampleRate = *(uint32_t*)(pData + 6);
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = *(uint32_t*)(pData + 10);
        }

        if ( pNumChannels )
        {
            *pNumChannels = *(uint32_t*)(pData + 14);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = *(uint32_t*)(pData + 18);
        }

        if ( pSoundDataSize )
        {
            *pSoundDataSize = *(uint32_t*)(pData + 18);
        }

        if ( pExtraDataOffset && pSoundDataOffset )
        {
            size = *(uint32_t*)(pData + 22);
            *pSoundDataOffset = size;
            if ( *pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(pData + 26);
            }
            else
            {
                *pExtraDataOffset = 0;
            }
        }

        return pData + 26 + size;
    }

    if ( !memcmp(pData, "RIFF", 4) )
    {
        const uint8_t* pEnd;
        uint32_t size;

        pEnd = pData + *(uint32_t*)(pData + 4) + 8;
        if ( memcmp(pData + 8, "WAVEfmt ", 8) )
        {
            return NULL;
        }

        if ( *(int16_t*)(pData + 20) != 1 )
        {
            return NULL;
        }

        if ( pType )
        {
            if ( bCompressed )
            {
                *pType = 4;
            }
            else
            {
                *pType = 3;
            }
        }

        pData += 16;
        size = *(uint32_t*)pData;
        if ( pSampleRate )
        {
            *pSampleRate = *(uint32_t*)(pData + 8);
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = *(int16_t*)(pData + 18);
        }

        if ( pNumChannels )
        {
            *pNumChannels = *(int16_t*)(pData + 6);
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }

        do
        {
            pData += size + 8;
            size = *(uint32_t*)pData;
            if ( !memcmp(pData - 4, "sync", 4) && pExtraDataOffset && pSoundDataOffset )
            {
                *pExtraDataOffset = (uint32_t)(pData + 4);
                *pSoundDataOffset = size;
            }
        } while ( pData < pEnd && memcmp(pData - 4, "data", 4) );

        if ( pData < pEnd )
        {
            if ( pSoundDataSize )
            {
                *pSoundDataSize = size;
            }

            return pData + 4;
        }

        return NULL;
    }

    if ( !memcmp(pData, "iMUS", 4) )
    {
        uint32_t size;

        size = AUDIOLIB_SWAP32(*(uint32_t*)(pData + 12));
        if ( memcmp(pData + 16, "FRMT", 4) )
        {
            return NULL;
        }

        if ( pType )
        {
            if ( bCompressed )
            {
                *pType = 2;
            }
            else
            {
                *pType = 1;
            }
        }

        pData += 32;
        if ( pBitsPerSample )
        {
            *pBitsPerSample = AUDIOLIB_SWAP32(*(uint32_t*)pData);
        }

        if ( pSampleRate )
        {
            *pSampleRate = AUDIOLIB_SWAP32(*(uint32_t*)(pData + 4));
        }

        if ( pNumChannels )
        {
            *pNumChannels = AUDIOLIB_SWAP32(*(uint32_t*)(pData + 8));
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 1;
        }

        if ( pSoundDataSize )
        {
            *pSoundDataSize = AUDIOLIB_SWAP32(*(uint32_t*)&pData[size - 12]);
        }

        return pData + size - 8;
    }

    if ( bCompressed )
    {
        return NULL;
    }

    if ( !memcmp(pData, "FORM", 4) )
    {
        const uint8_t* pEnd;
        uint32_t size;

        pEnd = pData + AUDIOLIB_SWAP32(*(uint32_t*)(pData + 4)) + 8;
        if ( memcmp(pData + 8, "AIFFCOMM", 8) )
        {
            return NULL;
        }

        if ( pType )
        {
            *pType = 5;
        }

        pData += 16;
        size = AUDIOLIB_SWAP32(*(uint32_t*)pData);
        if ( pSampleRate )
        {
            *pSampleRate = pData[13] == 14 ? 44100 : pData[13] == 12 ? 11025 : 22050;
        }

        if ( pBitsPerSample )
        {
            *pBitsPerSample = pData[11];
        }

        if ( pNumChannels )
        {
            *pNumChannels = pData[5];
        }

        if ( pExtraInfo )
        {
            *pExtraInfo = 0;
        }

        do
        {
            pData += size + 8;
            size = AUDIOLIB_SWAP32(*(uint32_t*)pData);
        } while ( pData < pEnd && memcmp(pData - 4, "SSND", 4) );

        if ( pData < pEnd )
        {
            if ( pSoundDataSize )
            {
                *pSoundDataSize = size - 8;
            }

            return pData + 12;
        }

        return NULL;
    }

    return NULL;
}

#ifdef J3D_DEBUG
// Debug-only (debug 0x00532DED, after ParseWaveFileHeader; no caller in either image). The name is Jones3DRE's.
size_t J3DAPI AudioLib_WriteWaveFileHeader(uint8_t* pOutData, int type, uint32_t sampleRate, uint32_t bitsPerSample, uint32_t numChannels, uint32_t soundDataSize, const uint32_t* pSyncData)
{
    size_t extraDataSize;
    uint8_t* pOut;

    pOut = pOutData;
    switch ( type )
    {
        case 3:
            extraDataSize = 0;
            if ( pSyncData )
            {
                extraDataSize = pSyncData[1] * 4 + 16;
            }

            memcpy(pOut, "RIFF", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, soundDataSize + extraDataSize + 36, AUDIOLIB_NOSWAP32);
            memcpy(pOut, "WAVEfmt ", 8);
            pOut += 8;
            AUDIOLIB_WRITE32(pOut, 16, AUDIOLIB_NOSWAP32);
            AUDIOLIB_WRITE16(pOut, 1, AUDIOLIB_NOSWAP16);
            AUDIOLIB_WRITE16(pOut, numChannels, AUDIOLIB_NOSWAP16);
            AUDIOLIB_WRITE32(pOut, sampleRate, AUDIOLIB_NOSWAP32);
            AUDIOLIB_WRITE32(pOut, sampleRate * (bitsPerSample >> 3) * numChannels, AUDIOLIB_NOSWAP32);
            AUDIOLIB_WRITE16(pOut, (bitsPerSample >> 3) * numChannels, AUDIOLIB_NOSWAP16);
            AUDIOLIB_WRITE16(pOut, bitsPerSample, AUDIOLIB_NOSWAP16);
            if ( extraDataSize )
            {
                extraDataSize -= 8;
                memcpy(pOut, "sync", 4);
                pOut += 4;
                AUDIOLIB_WRITE32(pOut, extraDataSize, AUDIOLIB_NOSWAP32);
                memcpy(pOut, pSyncData, extraDataSize);
                pOut += extraDataSize;
            }

            memcpy(pOut, "data", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, soundDataSize, AUDIOLIB_NOSWAP32);
            break;

        case 1:
            memcpy(pOut, "iMUS", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, soundDataSize + 80, AUDIOLIB_SWAP32);
            memcpy(pOut, "MAP ", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, 56, AUDIOLIB_SWAP32);
            memcpy(pOut, "FRMT", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, 20, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, 80, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, 0, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, bitsPerSample, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, sampleRate, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, numChannels, AUDIOLIB_SWAP32);
            memcpy(pOut, "REGN", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, 8, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, 80, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, soundDataSize, AUDIOLIB_SWAP32);
            memcpy(pOut, "STOP", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, 4, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, soundDataSize + 80, AUDIOLIB_SWAP32);
            memcpy(pOut, "DATA", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, soundDataSize, AUDIOLIB_SWAP32);
            break;

        case 5:
            memcpy(pOut, "FORM", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, soundDataSize + 54, AUDIOLIB_SWAP32);
            memcpy(pOut, "AIFFCOMM", 8);
            pOut += 8;
            AUDIOLIB_WRITE32(pOut, 18, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE16(pOut, numChannels, AUDIOLIB_SWAP16);
            AUDIOLIB_WRITE32(pOut, soundDataSize / ((bitsPerSample >> 3) * numChannels), AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE16(pOut, bitsPerSample, AUDIOLIB_SWAP16);
            switch ( sampleRate )
            {
                case 11025:
                    memcpy(pOut, AudioLib_aAiffRate11025, sizeof(AudioLib_aAiffRate11025));
                    break;

                case 44100:
                    memcpy(pOut, AudioLib_aAiffRate44100, sizeof(AudioLib_aAiffRate44100));
                    break;

                default:
                    memcpy(pOut, AudioLib_aAiffRate22050, sizeof(AudioLib_aAiffRate22050));
                    break;
            }

            pOut += 10;
            memcpy(pOut, "SSND", 4);
            pOut += 4;
            AUDIOLIB_WRITE32(pOut, soundDataSize + 8, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, 0, AUDIOLIB_SWAP32);
            AUDIOLIB_WRITE32(pOut, 0, AUDIOLIB_SWAP32);
            break;
    }

    return pOut - pOutData;
}
#endif

int J3DAPI AudioLib_Compress(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, const uint8_t* pInBuffer, int size, unsigned int numChannels)
{
    uint8_t* pStart;
    unsigned int blockSize;

    if ( numChannels > 2 )
    {
        numChannels -= 2;

        *pOutBuffer++ = numChannels > 1 ? ~pCompressorState->aStepIndex[0] : pCompressorState->aStepIndex[0];
        *(int16_t*)pOutBuffer = AUDIOLIB_SWAP16(pCompressorState->aPrevSample[0]);
        pOutBuffer += 2;

        if ( numChannels > 1 )
        {
            *pOutBuffer++ = pCompressorState->aStepIndex[1];
            *(int16_t*)pOutBuffer = AUDIOLIB_SWAP16(pCompressorState->aPrevSample[1]);
            pOutBuffer += 2;
        }

        return (3 * numChannels) + AudioLib_CompressBlock(pCompressorState, pOutBuffer, (int16_t*)pInBuffer, size / (2 * numChannels), numChannels, 1, 1);
    }

    pStart = pOutBuffer;
    *pOutBuffer++ = 0xE4;
    *pOutBuffer++ = 0x11;
    *pOutBuffer++ = 0x11;
    *pOutBuffer++ = 0x64;
    *pOutBuffer++ = 0x22;
    *pOutBuffer++ = 0x22;
    *pOutBuffer++ = 'W';
    *pOutBuffer++ = 'V';
    *pOutBuffer++ = 'S';
    *pOutBuffer++ = 'M';

    while ( size )
    {
        blockSize = 0x1000;
        if ( (unsigned int)size > 0x1000 )
        {
            size -= 0x1000;
        }
        else
        {
            blockSize = size;
            size = 0;
        }

        pOutBuffer += AudioLib_WVSMCompressBlock(pOutBuffer, pInBuffer, blockSize, NULL);
        pInBuffer += 0x1000;
    }

    return pOutBuffer + 10 - pStart;
}

#ifdef J3D_DEBUG
// Debug-only (no caller in either image), the debug build's text in its order (0x005339BC, 0x00533BCD,
// 0x00533C86, after Compress): an MCMP container around a wave file (a two-entry index: the header stored, the sound data
// VIMA-compressed), the total uncompressed size of an MCMP file and its decompression. Names are inferred.
int J3DAPI AudioLib_CompressWave(uint8_t* pOutBuffer, const uint8_t* pWaveData)
{
    size_t headerSize;
    uint32_t numChannels;
    size_t compressedSize;
    uint8_t* pStart;
    uint32_t sndDataSize;
    int format;
    uint32_t bitsPerSample;
    tAudioCompressorState state;
    uint32_t extra;
    const uint8_t* pData;

    pStart = pOutBuffer;
    pData = AudioLib_ParseWaveFileHeader(pWaveData, &format, NULL, &bitsPerSample, &numChannels, &extra, &sndDataSize, NULL, NULL);
    if ( !pData || bitsPerSample != 16 )
    {
        return 0;
    }

    headerSize = pData - pWaveData;
    memcpy(pOutBuffer, "MCMP", 4);
    pOutBuffer += 4;
    *(uint32_t*)pOutBuffer = AUDIOLIB_SWAP16(2);
    pOutBuffer += 2;

    *pOutBuffer = 0;
    pOutBuffer += 1;
    *(uint32_t*)pOutBuffer = AUDIOLIB_SWAP32(headerSize);
    pOutBuffer += 4;
    *(uint32_t*)pOutBuffer = AUDIOLIB_SWAP32(headerSize);
    pOutBuffer += 4;

    *pOutBuffer = 1;
    pOutBuffer += 1;
    *(uint32_t*)pOutBuffer = AUDIOLIB_SWAP32(sndDataSize);
    pOutBuffer += 8;

    *(uint16_t*)pOutBuffer = AUDIOLIB_SWAP16(10);
    pOutBuffer += 2;
    memcpy(pOutBuffer, "NONE", 5);
    pOutBuffer += 5;
    memcpy(pOutBuffer, "VIMA", 5);
    pOutBuffer += 5;

    memcpy(pOutBuffer, pWaveData, headerSize);
    pOutBuffer += headerSize;

    AudioLib_ResetCompressor(&state);
    compressedSize = AudioLib_Compress(&state, pOutBuffer, pData, sndDataSize, numChannels);
    *(uint32_t*)(pStart + 20) = AUDIOLIB_SWAP32(compressedSize);
    return pOutBuffer + compressedSize - pStart;
}

int J3DAPI AudioLib_GetUncompressedSize(const uint8_t* pData)
{
    int size;
    int16_t count;

    if ( !memcmp(pData, "MCMP", 4) )
    {
        count = AUDIOLIB_SWAP16(*(uint16_t*)(pData + 4));
        size = 0;
        pData += 7;
        while ( count-- )
        {
            size += AUDIOLIB_SWAP32(*(uint32_t*)pData);
            pData += 9;
        }

        return size;
    }

    return 0;
}

int J3DAPI AudioLib_UncompressWave(uint8_t* pOutData, const uint8_t* pData)
{
    size_t headerSize;
    uint32_t soundDataSize;
    tAudioCompressorState state;

    if ( memcmp(pData, "MCMP", 4) )
    {
        return 1;
    }

    headerSize = AUDIOLIB_SWAP32(*(uint32_t*)(pData + 7));
    memcpy(pOutData, pData + 36, headerSize);
    AudioLib_ParseWaveFileHeader(pOutData, NULL, NULL, NULL, NULL, NULL, &soundDataSize, NULL, NULL);
    AudioLib_ResetCompressor(&state);
    AudioLib_Uncompress(&state, pOutData + headerSize, pData + headerSize + 36, soundDataSize);
    return 0;
}
#endif

void J3DAPI AudioLib_ResetCompressor(tAudioCompressorState* pState)
{
    pState->aStepIndex[0] = pState->aStepIndex[1] = 0;
    pState->aPrevSample[0] = pState->aPrevSample[1] = 0;
}

void J3DAPI AudioLib_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size)
{
    unsigned int blockSize;
    unsigned int numChannels;

    numChannels = 1;
    pCompressorState->aStepIndex[0] = *pCompressedData++;
    if ( pCompressorState->aStepIndex[0] < 0 )
    {
        pCompressorState->aStepIndex[0] = ~pCompressorState->aStepIndex[0];
        ++numChannels;
    }

    pCompressorState->aPrevSample[0] = AUDIOLIB_SWAP16(*(uint16_t*)pCompressedData);
    pCompressedData += 2;

    if ( numChannels > 1 )
    {
        pCompressorState->aStepIndex[1] = *pCompressedData++;
        pCompressorState->aPrevSample[1] = AUDIOLIB_SWAP16(*(uint16_t*)pCompressedData);
        pCompressedData += 2;
    }

    if ( numChannels == 2
        && pCompressorState->aPrevSample[0] == 0x1111
        && pCompressorState->aStepIndex[1] == 0x64
        && pCompressorState->aPrevSample[1] == 0x2222
        && !memcmp(pCompressedData, "WVSM", 4) )
    {
        pCompressedData += 4;
        while ( size )
        {
            blockSize = 0x1000;
            if ( size > 0x1000 )
            {
                size -= 0x1000;
            }
            else
            {
                blockSize = size;
                size = 0;
            }

            pCompressedData += AudioLib_WVSMUncompressBlock(pOutSndData, pCompressedData, blockSize);
            pOutSndData += 0x1000;
        }
    }
    else
    {
        AudioLib_UncompressBlock(pCompressorState, pOutSndData, pCompressedData, size / (2 * numChannels), numChannels, 1);
    }
}

#ifdef J3D_DEBUG
// Debug-only (no caller in either image), the debug build's text (0x00533F0A, after Uncompress): the size of
// a wave file converted to another format, rate, sample size and channel count (0 keeps the source's). The name is inferred.
size_t J3DAPI AudioLib_CalcConvertedSize(int outType, uint32_t outSampleRate, uint32_t outBitsPerSample, uint32_t outNumChannels, const uint8_t* pData)
{
    uint32_t srcRate;
    uint32_t numChannels;
    uint32_t sndDataSize;
    int inType;
    size_t hdrSize;
    int bInCompress;
    int bCompress;
    uint32_t bits;

    bInCompress = 0;
    bCompress = 0;
    AudioLib_ParseWaveFileHeader(pData, &inType, &srcRate, &bits, &numChannels, NULL, &sndDataSize, NULL, NULL);
    if ( !numChannels )
    {
        numChannels = 1;
    }

    if ( srcRate < 11025 )
    {
        srcRate = 11025;
    }

    if ( bits < 8 )
    {
        bits = 8;
    }

    switch ( inType )
    {
        default:
            return 0;

        case 1:
        case 3:
        case 5:
            break;

        case 2:
        case 4:
            bInCompress = 1;
            break;
    }

    if ( !outSampleRate )
    {
        outSampleRate = srcRate;
    }

    if ( !outBitsPerSample )
    {
        outBitsPerSample = bits;
    }

    if ( !outNumChannels )
    {
        outNumChannels = numChannels;
    }

    switch ( outType )
    {
        default:
            return 0;

        case 1:
            hdrSize = 80;
            break;

        case 3:
            hdrSize = 44;
            break;

        case 5:
            hdrSize = 54;
            break;

        case 2:
            hdrSize = 80;
            bCompress = 1;
            break;

        case 4:
            hdrSize = 44;
            bCompress = 1;
            break;
    }

    if ( srcRate != outSampleRate )
    {
        sndDataSize *= outSampleRate >> 10;
        sndDataSize /= srcRate >> 10;
    }

    if ( bits != outBitsPerSample )
    {
        sndDataSize *= outBitsPerSample >> 3;
        sndDataSize /= bits >> 3;
    }

    if ( numChannels != outNumChannels )
    {
        sndDataSize *= outNumChannels;
        sndDataSize /= numChannels;
    }

    if ( bCompress )
    {
        sndDataSize >>= 1;
    }

    return hdrSize + sndDataSize;
}
#endif

int J3DAPI AudioLib_GetMouthPosition(uint8_t* pData, uint32_t time, uint8_t* pMouthPosX, uint8_t* pMouthPosY)
{
    uint8_t mouthX;
    uint8_t mouthY;
    int result;
    int numEntries;
    uint32_t* aEntries;
    uint32_t probeEntry;
    uint32_t entry;
    int low;
    int high;
    int probe;
    int mid;

    mouthX = 0;
    mouthY = 0;
    result = 0;
    aEntries = (uint32_t*)pData;
    if ( memcmp(pData, "SYNC", 4) )
    {
        result = 1;
    }
    else
    {
        aEntries++;
        numEntries = *aEntries++;
        if ( time < 0 )
        {
        }
        else if ( time & 0xFFF00000 )
        {
            result = 1;
        }
        else
        {
            time <<= 12;
            time &= 0xFFFF0000;
            low = 0;
            high = numEntries - 1;
            mid = 0;
            if ( time > aEntries[high] )
            {
                mid = high;
                time = 0;
            }

            while ( time )
            {
                probe = (high - low) / 2 + low;
                if ( low + 1 >= high )
                {
                    mid = probe;
                    break;
                }

                probeEntry = aEntries[probe];
                if ( time == (probeEntry & 0xFFFF0000) )
                {
                    mid = probe;
                    break;
                }

                if ( time > probeEntry )
                {
                    low = probe;
                }
                else
                {
                    high = probe;
                }
            }

            entry = aEntries[mid];
            mouthX = (entry >> 8) & 0x7F;
            mouthY = entry & 0x7F;
        }
    }

    if ( pMouthPosX )
    {
        *pMouthPosX = mouthX;
    }

    if ( pMouthPosY )
    {
        *pMouthPosY = mouthY;
    }

    return result;
}

int J3DAPI AudioLib_GenerateLipSynchBlock(uint8_t* pOutData, const uint8_t* pSndData, unsigned int framesPerSec, uint8_t mouthXLevels, uint8_t mouthYLevels, int sampleRate, int bitsPerSample, int numChannels, int bSkipFirstByte, int sndDataSize)
{
    uint32_t* pEntry;
    int histIdx;
    uint32_t time;
    unsigned int totalAmp;
    unsigned int totalCrossings;
    int peakAmp;
    int crossingsPerFrame;
    int mouthX;
    int mouthY;
    int lastMouthX;
    int lastMouthY;
    int maskX;
    int maskY;
    int i;
    int j;
    uint32_t timeStep;
    const int16_t* pPrev;
    const int16_t* pCur;
    int16_t sample;
    int16_t prevSample;
    int amp;
    char aText[200];
    int aHistory[15];
    int frameSize;
    int openness;
    int shape;

    pEntry = (uint32_t*)pOutData;
    histIdx = 0;
    time = 0;
    totalAmp = 0;
    totalCrossings = 0;
    peakAmp = 0;
    crossingsPerFrame = 0;
    mouthX = 0;
    mouthY = 0;
    lastMouthX = -1;
    lastMouthY = -1;
    maskX = 0;
    maskY = 0;
    if ( !sndDataSize )
    {
        return 0;
    }

    if ( !pSndData || bitsPerSample != 16 || numChannels != 1 )
    {
        return 0;
    }

    if ( bSkipFirstByte )
    {
        ++pSndData;
    }

    strcpy((char*)pOutData, "SYNC");
    pEntry += 2;

    --mouthXLevels;
    --mouthYLevels;
    for ( i = 0; i < 7; ++i )
    {
        maskX <<= 1;
        maskY <<= 1;
        if ( mouthXLevels )
        {
            maskX |= 1;
        }

        if ( mouthYLevels )
        {
            maskY |= 1;
        }

        mouthXLevels >>= 1;
        mouthYLevels >>= 1;
    }

    timeStep = 0x3E8000 / framesPerSec;

    pPrev = (const int16_t*)pSndData;
    pCur = (const int16_t*)(pSndData + 2);
    for ( i = 2; i < sndDataSize; i += 2 )
    {
        sample = *pCur++;
        prevSample = *pPrev++;
        amp = abs(sample);
        if ( sample >= 0 && prevSample < 0 )
        {
            ++totalCrossings;
        }

        if ( amp > peakAmp )
        {
            peakAmp = amp;
        }

        totalAmp += amp;
    }

    totalAmp /= (unsigned int)sndDataSize >> 1;
    sprintf(aText, "MaxAmp: %d  Avg: %d", peakAmp, totalAmp);

    for ( i = 0; i < 15; ++i )
    {
        aHistory[i] = peakAmp;
    }

    frameSize = (unsigned int)(2 * sampleRate) / framesPerSec;
    crossingsPerFrame = totalCrossings * frameSize / sndDataSize;
    for ( i = 0; i + frameSize <= sndDataSize; i += frameSize, pSndData += frameSize )
    {
        int framePeak = 0;
        int numCrossings = 0;
        int historyAvg = 0;
        int frameAmp = 0;
        int16_t curSample;
        int curAmp;
        int16_t lastSample;

        pPrev = (const int16_t*)pSndData;
        pCur = (const int16_t*)(pSndData + 2);
        for ( j = 2; j < frameSize; j += 2 )
        {
            curSample = *pCur++;
            lastSample = *pPrev++;
            curAmp = abs(curSample);
            if ( curSample >= 0 && lastSample < 0 )
            {
                ++numCrossings;
            }

            if ( curAmp > framePeak )
            {
                framePeak = curAmp;
            }

            frameAmp += curAmp;
        }

        frameAmp /= frameSize >> 1;

        historyAvg = 0;
        for ( j = histIdx; j < histIdx + 15; ++j )
        {
            historyAvg += aHistory[j % 15];
        }

        historyAvg /= 15;
        if ( !historyAvg )
        {
            ++historyAvg;
        }

        if ( historyAvg < peakAmp / 4 )
        {
            historyAvg = peakAmp / 4;
        }

        aHistory[histIdx] = framePeak;
        histIdx = (histIdx + 1) % 15;

        openness = framePeak * 60 / historyAvg;
        shape = numCrossings * 50 / (crossingsPerFrame + 1);
        if ( openness < 25 )
        {
            shape = (openness * shape + (25 - openness) * 37) / 25;
        }

        shape = shape < 100 ? shape : 100;
        shape = shape * 127 / 100;
        openness = openness < 100 ? openness : 100;
        openness = openness * 127 / 100;

        if ( frameAmp < framePeak * 30 / 100 )
        {
            if ( mouthX > 47 )
            {
                shape = mouthX - 16;
            }
            else
            {
                shape = 0;
            }

            if ( mouthY > 15 )
            {
                openness = mouthY - 16;
            }
            else
            {
                openness = 0;
            }
        }

        mouthX = shape;
        mouthY = openness;
        shape &= maskX;
        openness &= maskY;
        if ( shape != lastMouthX || openness != lastMouthY )
        {
            uint32_t entry;

            entry = (time & 0xFFFF0000) | ((shape & 0x7F) * 256) | (openness & 0x7F);
            *pEntry++ = entry;
            lastMouthX = shape;
            lastMouthY = openness;
        }

        time += timeStep;
    }

    {
        uint32_t entry;
        int numEntries;

        entry = time & 0xFFFF0000;
        *pEntry++ = entry;
        numEntries = pEntry - (uint32_t*)pOutData - 2;
        pEntry = (uint32_t*)(pOutData + 4);
        *pEntry = numEntries;
        return 4 * numEntries + 8;
    }
}

// ADPCM compression
int J3DAPI AudioLib_CompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, int16_t* pInBuffer, int numSamples, unsigned int numChannels, int bNativeByteOrder, int bStateInitialized)
{
    int bitPos;
    uint8_t* pCurOut;
    uint16_t bitBuffer;
    unsigned int channel;
    int index;
    int predSample;
    int16_t* pSample;
    unsigned int i;
    int diff;
    int sample;
    int step;
    int delta;
    uint8_t code;
    uint8_t sign;
    uint8_t escape;
    int bit;
    int count;
    uint8_t freeBits;
    uint8_t numBits;

    bitPos = 0;
    pCurOut = pOutBuffer;
    bitBuffer = 0;
    if ( !bStateInitialized )
    {
        pCompressorState->aStepIndex[0] = pCompressorState->aStepIndex[1] = 0;
        pCompressorState->aPrevSample[0] = pCompressorState->aPrevSample[1] = 0;
    }

    for ( channel = 0; channel < numChannels; ++channel )
    {
        pSample = &pInBuffer[channel];
        index = pCompressorState->aStepIndex[channel];
        predSample = pCompressorState->aPrevSample[channel];
        for ( i = 0; i < numSamples; ++i, pSample += numChannels )
        {
            delta = 0;
            code = 0;
            sign = 0;
            step = AudioLib_aStepTable[index];
            if ( bNativeByteOrder )
            {
                diff = *pSample - predSample;
            }
            else
            {
                diff = AUDIOLIB_SWAP16(*(uint16_t*)pSample) - predSample;
            }

            numBits = AudioLib_aStepBits[index];
            count = numBits - 1;
            bit = 1 << count;
            escape = bit - 1;
            if ( diff < 0 )
            {
                sign = bit;
                diff = -diff;
            }

            while ( bit >>= 1, count-- )
            {
                if ( diff >= step )
                {
                    code |= bit;
                    diff -= step;
                    delta += step;
                }

                step >>= 1;
            }

            if ( code )
            {
                delta += step;
            }

            freeBits = 8 - (bitPos & 7);
            bitBuffer <<= numBits;
            bitBuffer |= code | sign;
            bitPos += numBits;
            if ( numBits >= freeBits )
            {
                *pCurOut++ = bitBuffer >> (numBits - freeBits);
            }

            if ( code == escape )
            {
                if ( bNativeByteOrder )
                {
                    sample = *pSample;
                }
                else
                {
                    sample = AUDIOLIB_SWAP16(*(uint16_t*)pSample);
                }

                predSample = sample;
                bitBuffer <<= 8;
                bitBuffer |= (predSample >> 8) & 0xFF;
                *pCurOut++ = bitBuffer >> (bitPos & 7);
                bitBuffer <<= 8;
                bitBuffer |= predSample & 0xFF;
                *pCurOut++ = bitBuffer >> (bitPos & 7);
            }
            else
            {
                predSample += sign ? -delta : delta;
                if ( predSample < -32768 )
                {
                    predSample = -32768;
                }
                else if ( predSample > 32767 )
                {
                    predSample = 32767;
                }
            }

            index += AudioLib_aIndexTableTable[numBits][code];
            if ( index < 0 )
            {
                index = 0;
            }

            if ( index > 88 )
            {
                index = 88;
            }
        }

        pCompressorState->aStepIndex[channel] = index;
        pCompressorState->aPrevSample[channel] = predSample;
    }

    if ( bitPos & 7 )
    {
        bitBuffer <<= 8 - (bitPos & 7);
        *pCurOut++ = bitBuffer & 0xFF;
    }

    return pCurOut - pOutBuffer;
}

// ADPCM decompress
void J3DAPI AudioLib_UncompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutData, const uint8_t* pInData, int numSamples, unsigned int numChannels, int bStateInitialized)
{
    unsigned int stepIdx;
    int numSamplesLeft;
    uint16_t step;
    unsigned int code;
    unsigned int bit;
    int delta;
    unsigned int channel;
    int bitPos;
    uint16_t bitBuffer;
    int index;
    int predSample;
    int16_t* pOut;
    const uint8_t* pIn;
    uint8_t numBits;
    int signBit;
    uint8_t escape;

    pIn = pInData;
    bitPos = 0;
    if ( !AudioLib_bDeltaTableInitialized )
    {
        for ( code = 0; code < 64; ++code )
        {
            for ( stepIdx = 0; stepIdx < 89; ++stepIdx )
            {
                step = AudioLib_aStepTable[stepIdx];
                bit = 32;
                delta = 0;
                while ( bit )
                {
                    if ( code & bit )
                    {
                        delta += step;
                    }

                    bit >>= 1;
                    step >>= 1;
                }

                AudioLib_aDeltaTable[code + stepIdx * 64] = delta;
            }
        }

        AudioLib_bDeltaTableInitialized = 1;
    }

    if ( !bStateInitialized )
    {
        pCompressorState->aStepIndex[0] = pCompressorState->aStepIndex[1] = 0;
        pCompressorState->aPrevSample[0] = pCompressorState->aPrevSample[1] = 0;
    }

    bitBuffer = *pIn++;
    bitBuffer <<= 8;
    bitBuffer |= *pIn++;
    for ( channel = 0; channel < numChannels; ++channel )
    {
        numSamplesLeft = numSamples;
        index = pCompressorState->aStepIndex[channel];
        predSample = pCompressorState->aPrevSample[channel];
        pOut = (int16_t*)pOutData + channel;

        // Hand-written assembly: retail's
        // per-sample decoder is this hand-written block; the rest of the function is C.
        //
        __asm {
            push edi
            push esi
            mov esi, pIn
            mov ecx, numSamples
            mov eax, numChannels
            shl eax, 1
            mov numChannels, eax
        L1:
            cmp ecx, 0
            je L9
            dec ecx
            push ecx
            lea eax, AudioLib_aStepBits
            add eax, index
            mov cl, [eax]
            mov numBits, cl
            dec cl
            mov eax, 1
            shl eax, cl
            mov signBit, eax
            dec eax
            mov escape, al
            mov edx, bitPos
            movzx ecx, numBits
            add ecx, edx
            mov bitPos, ecx
            neg ecx
            add ecx, 16
            movzx eax, word ptr bitBuffer
            shr eax, cl
            mov edx, signBit
            or dl, escape
            and eax, edx
            mov edi, eax
            mov ecx, bitPos
            cmp ecx, 7
            jle L2
            sub ecx, 8
            mov bitPos, ecx
            movzx eax, word ptr bitBuffer
            shl eax, 8
            mov al, [esi]
            mov word ptr bitBuffer, ax
            inc esi
        L2:
            mov eax, edi
            test signBit, eax
            je L3
            xor eax, signBit
            mov edi, eax
            jmp L4
        L3:
            mov signBit, 0
        L4:
            mov eax, edi
            cmp al, escape
            jne L5
            movzx edx, word ptr bitBuffer
            mov ecx, bitPos
            mov eax, edx
            shl eax, cl
            movsx eax, ax
            shl dx, 8
            mov dl, [esi]
            inc esi
            neg cl
            add cl, 8
            mov ebx, edx
            shr ebx, cl
            mov al, bl
            shl dx, 8
            mov dl, [esi]
            inc esi
            mov word ptr bitBuffer, dx
            jmp L7
        L5:
            mov eax, edi
            movzx ecx, numBits
            neg ecx
            add ecx, 7
            shl eax, cl
            mov edx, index
            mov ebx, edx
            shl edx, 6
            or eax, edx
            movzx eax, word ptr AudioLib_aDeltaTable[eax*2]
            test edi, 0xffff
            je L6
            movsx edx, word ptr AudioLib_aStepTable[ebx*2]
            movzx ecx, numBits
            dec ecx
            shr edx, cl
            add eax, edx
        L6:
            test signBit, 0xffff
            je L8
            neg eax
            add eax, predSample
            cmp eax, -32768
            jg L7
            mov eax, -32768
            jmp L7
        L8:
            add eax, predSample
            cmp eax, 32767
            jl L7
            mov eax, 32767
        L7:
            mov predSample, eax
            mov ebx, pOut
            mov [ebx], ax
            add ebx, numChannels
            mov pOut, ebx
            movzx eax, numBits
            lea eax, AudioLib_aIndexTableTable[eax*4]
            mov ebx, edi
            mov eax, [eax]
            add eax, ebx
            movsx eax, byte ptr [eax]
            mov edx, index
            add edx, eax
            cmp edx, 0
            jg L10
            mov edx, 0
            jmp L11
        L10:
            cmp edx, 88
            jl L11
            mov edx, 88
        L11:
            mov index, edx
            pop ecx
            jmp L1
        L9:
            mov pIn, esi
            pop esi
            pop edi
            mov eax, numChannels
            shr eax, 1
            mov numChannels, eax
        }

        pCompressorState->aStepIndex[channel] = index;
        pCompressorState->aPrevSample[channel] = predSample;
    }
}

int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile)
{
    const int16_t* pIn;
    uint8_t* pCurOut;
    int sample;
    int numBits;
    int kLeft;
    int kRight;
    int leftSaved;
    int rightSaved;
    int numSlotsLeft;
    int bLeft;
    int leftOffset;
    int rightOffset;
    int leftIn;
    int rightIn;
    int leftValue;
    int rightValue;
    int leftError;
    int rightError;
    int i;
    int leftBits[17];
    int rightBits[17];

    pCurOut = pOutBuffer;
    blockSize >>= 1;
    for ( i = 0; i < 17; ++i )
    {
        rightBits[i] = 0;
        leftBits[i] = 0;
    }

    for ( i = 0, pIn = (const int16_t*)pInBuffer; i < blockSize; i += 2 )
    {
        sample = *pIn++;
        sample = sample < 0 ? -sample : sample;
        for ( numBits = 1; sample; sample >>= 1, ++numBits )
        {
        }

        ++leftBits[numBits];

        sample = *pIn++;
        sample = sample < 0 ? -sample : sample;
        for ( numBits = 1; sample; sample >>= 1, ++numBits )
        {
        }

        ++rightBits[numBits];
    }

    kLeft = kRight = 8;
    while ( kLeft > 0 && !leftBits[kLeft + 8] )
    {
        --kLeft;
    }

    while ( kRight > 0 && !rightBits[kRight + 8] )
    {
        --kRight;
    }

    sample = 192;
    leftSaved = rightSaved = 0;
    while ( sample )
    {
        if ( kLeft > 0 && kRight > 0 )
        {
            if ( leftBits[kLeft + 8] < rightBits[kRight + 8] )
            {
                bLeft = 1;
            }
            else
            {
                bLeft = 0;
            }
        }
        else if ( kLeft > 0 )
        {
            bLeft = 1;
        }
        else if ( kRight > 0 )
        {
            bLeft = 0;
        }
        else
        {
            break;
        }

        if ( bLeft )
        {
            if ( leftBits[kLeft + 8] > sample )
            {
                break;
            }

            sample -= leftBits[kLeft + 8];
            --kLeft;
            ++leftSaved;
        }
        else
        {
            if ( rightBits[kRight + 8] > sample )
            {
                break;
            }

            sample -= rightBits[kRight + 8];
            --kRight;
            ++rightSaved;
        }
    }

    numSlotsLeft = sample;
    leftOffset = kLeft ? 1 << (kLeft - 1) : 0;
    rightOffset = kRight ? 1 << (kRight - 1) : 0;

    if ( pFile )
    {
        fprintf(pFile, "L = %4d:", kLeft);
        for ( i = 0; i < 16; ++i )
        {
            fprintf(pFile, "%4d,", leftBits[i]);
        }

        fprintf(pFile, "%4d\n", leftBits[16]);

        fprintf(pFile, "R = %4d:", kRight);
        for ( i = 0; i < 16; ++i )
        {
            fprintf(pFile, "%4d,", rightBits[i]);
        }

        fprintf(pFile, "%4d\n", rightBits[16]);

        fprintf(pFile, "%d escape slots used, %dL/%dR bits saved\n\n", 192 - sample, leftSaved, rightSaved);
    }

    *pCurOut++ = *pCurOut++ = 0;
    *pCurOut++ = (kLeft << 4) + kRight;

    leftError = rightError = 0;
    for ( i = 0, pIn = (const int16_t*)pInBuffer; i < blockSize; i += 2 )
    {
        leftIn = *pIn++;
        sample = leftIn;
        sample = sample < 0 ? sample - leftOffset : sample + leftOffset;
        sample = sample < -32767 ? -32767 : sample;
        sample = sample > 32767 ? 32767 : sample;
        sample >>= kLeft;
        leftValue = sample << kLeft;
        if ( sample > 127 || sample < -127 )
        {
            *pCurOut++ = 0x80;
            *pCurOut++ = leftValue >> 8;
            *pCurOut++ = leftValue;
        }
        else
        {
            *pCurOut++ = sample;
        }

        leftError = leftIn - leftValue;

        rightIn = *pIn++;
        sample = rightIn;
        sample = sample < 0 ? sample - rightOffset : sample + rightOffset;
        sample = sample < -32767 ? -32767 : sample;
        sample = sample > 32767 ? 32767 : sample;
        sample >>= kRight;
        rightValue = sample << kRight;
        if ( sample > 127 || sample < -127 )
        {
            *pCurOut++ = 0x80;
            *pCurOut++ = rightValue >> 8;
            *pCurOut++ = rightValue;
        }
        else
        {
            *pCurOut++ = sample;
        }

        rightError = rightIn - rightValue;
    }

    sample = pCurOut - pOutBuffer;
    pCurOut = pOutBuffer;
    *pCurOut++ = (sample - 2) >> 8;
    *pCurOut++ = sample - 2;
    return sample;
}

int J3DAPI AudioLib_WVSMUncompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize)
{
    int left;
    int16_t* pOut;
    int curleftShift;
    int rightShift;
    int curi;
    int value;
    const uint8_t* pIn;
    int right;

    pIn = pInBuffer;
    pOut = (int16_t*)pOutBuffer;
    blockSize &= ~1;
    blockSize >>= 1;

    value = *pIn++ << 8;
    value |= *pIn++;

    value = *pIn++;
    curleftShift = value >> 4;
    rightShift = value & 0xF;

    for ( curi = 0; curi < blockSize; curi += 2 )
    {
        left = *pIn++;
        if ( left == 0x80 )
        {
            value = *pIn++ << 8;
            *pOut++ = value + *pIn++;
        }
        else
        {
            left = (int8_t)left;
            *pOut++ = left << curleftShift;
        }

        if ( curi + 1 == blockSize )
        {
            break;
        }

        right = *pIn++;
        if ( right == 0x80 )
        {
            value = *pIn++ << 8;
            *pOut++ = value + *pIn++;
        }
        else
        {
            right = (int8_t)right;
            *pOut++ = right << rightShift;
        }
    }

    return pIn - pInBuffer;
}
