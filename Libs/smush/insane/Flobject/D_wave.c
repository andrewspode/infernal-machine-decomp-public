/*
 * insane_wave_flobdec: INSANE's `FLOBDEC=insane_wave` frame-object decoder (retail 0x004E7D60-0x004E8510): decodes the `Wave` chunk of a
 * frame (variable-bit-rate 16-bit ADPCM, "VIMA") into packets for the sound service.
 */
#include <stddef.h>
#include <string.h>
#include "../lec.h"
#include "../Memory/Memory.h"
#include "../Sound/Engine.h"

#define DWAVE_SWAP32(v) (((((v) & 0xFF0000) | ((v) >> 16)) >> 8) | ((((v) << 16) | ((v) & 0xFF00)) << 8))

static const signed char dwave_delta2[4] = {
    -1, 4, -1, 4
};

static const signed char dwave_delta3[8] = {
    -1, -1, 2, 6, -1, -1, 2, 6
};

static const signed char dwave_delta4[16] = {
    -1, -1, -1, -1, 1, 2, 4, 6, -1, -1, -1, -1, 1, 2, 4, 6
};

static const signed char dwave_delta5[32] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 2, 2, 4, 5, 6
};

static const signed char dwave_delta6[64] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 4, 4, 4, 5, 5, 6, 6
};

static const signed char dwave_delta7[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6
};

static const unsigned short dwave_step[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const unsigned char dwave_bits[89] = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

int dwave_Register(LecShare* pShare, void* pParam);
static void dwave_Unregister(void);
static int dwave_Startup(void);
static void dwave_Shutdown(void);
static const char* dwave_Name(int index);
static int dwave_GetTag(void);
static int dwave_GetFlags(void);
static void dwave_Reset(void);
static int dwave_Parse(const unsigned char* pData, int size);
static unsigned dwave_Decode(void* a1, int a2, int a3, int a4, void* a5, const unsigned char* pData, int size, int mode, int a9, int a10);
static int dwave_Query1(void);
static int dwave_Query(const unsigned char* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
static void dwave_DecodeVima(short* pDst, const unsigned char* pSrc, unsigned count, unsigned channels);

static LecModule dwave_module = { dwave_Register, dwave_Unregister, dwave_Startup, dwave_Shutdown };
static struct
{
    const char* (*Name)(int index);
    int  (*GetTag)(void);
    int  (*GetFlags)(void);
    void (*Reset)(void);
    int  (*Parse)(const unsigned char* pData, int size);
    unsigned (*Decode)(void* a1, int a2, int a3, int a4, void* a5, const unsigned char* pData, int size, int mode, int a9, int a10);
    int  (*Query1)(void);
    int  (*Query)(const unsigned char* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
} dwave_interface = { dwave_Name, dwave_GetTag, dwave_GetFlags, dwave_Reset, dwave_Parse, dwave_Decode, dwave_Query1, dwave_Query };
static int dwave_lastHeader = -1;
static const signed char* dwave_deltas[8] = { NULL, NULL, dwave_delta2, dwave_delta3, dwave_delta4, dwave_delta5, dwave_delta6, dwave_delta7 };

static MemoryDebugInterface* dwave_pMemory = NULL;
static struct
{
    signed char idx[2];
    short pred[2];
    unsigned short packets;
} dwave_state = { 0 };
static unsigned dwave_total = 0;
static LecShare* dwave_pShare = NULL;
static unsigned short dwave_quant[89 * 64] = { 0 };
static void* dwave_pParam = NULL;
static SoundInterface* dwave_pSound = NULL;
static char* dwave_pPacket = NULL;
static int dwave_tableReady = 0;

int dwave_Register(LecShare* pShare, void* pParam)
{
    dwave_pShare = pShare;
    dwave_pParam = pParam;
    pShare->Publish("LECMODULE=insane_wave_flobdec", &dwave_module);
    dwave_pShare->Publish("FLOBDEC=insane_wave", &dwave_interface);
    return 0;
}

static void dwave_Unregister(void)
{
    dwave_pShare->Unpublish("FLOBDEC=insane_wave");
    dwave_pShare->Unpublish("LECMODULE=insane_wave_flobdec");
}

static int dwave_Startup(void)
{
    int rc;

    dwave_pSound = dwave_pShare->Subscribe("SOUND", dwave_pParam);
    if (dwave_pSound == NULL) {
        dwave_pShare->ReportError("D_WAVE: Cannot allocate sound services.");
        return -1;
    }
    dwave_pMemory = dwave_pShare->Subscribe("MEMORY", dwave_pParam);
    if (dwave_pMemory == NULL) {
        dwave_pShare->ReportError("D_WAVE: Cannot allocate memory services.");
        return -1;
    }
#line 114
    dwave_pPacket = dwave_pMemory->Alloc(500000, INSANE_FILE, __LINE__);
    rc = dwave_pPacket ? 0 : -1;
    return rc;
}

static void dwave_Shutdown(void)
{
    dwave_pShare->Unsubscribe(dwave_pSound);
    dwave_pShare->Unsubscribe(dwave_pMemory);
    if (dwave_pPacket) {
#line 130
        dwave_pMemory->Free(dwave_pPacket, INSANE_FILE, __LINE__);
    }
    dwave_pPacket = NULL;
}

static const char* dwave_Name(int index)
{
    switch (index) {
    case 0:
        return "Audio";
    case 1:
        return "Decompresses VBR-ADPCM 16-bit audio.";
    case 2:
        return "Decompresses Variable Bit Rate ADPCM 16-bit audio.";
    }
    return "";
}

static int dwave_GetTag(void)
{
    return 'Wave';
}

static int dwave_GetFlags(void)
{
    return 4;
}

static void dwave_Reset(void)
{
    dwave_state.packets = 0;
}

static int dwave_Parse(const unsigned char* pData, int size)
{
    unsigned total = *(const unsigned*)(pData + 8);

    dwave_state.packets = 0;
    dwave_total = total;
    return 0;
}

static unsigned dwave_Decode(void* a1, int a2, int a3, int a4, void* a5, const unsigned char* pData, int size, int mode, int a9, int a10)
{
    unsigned channels = 1;
    unsigned v;
    unsigned char c;
    unsigned short w;
    const unsigned char* src = pData;

    v = DWAVE_SWAP32(*(const unsigned*)src);
    src += 4;
    if ((int)v < 0) {
        unsigned h = DWAVE_SWAP32(*(const unsigned*)src);

        src += 4;
        if ((int)h != dwave_lastHeader) {
            dwave_lastHeader = h;
            dwave_state.packets = 0;
        }
        v = DWAVE_SWAP32(*(const unsigned*)src);
        src += 4;
    }
    c = *src++;
    dwave_state.idx[0] = c;
    if ((signed char)c < 0) {
        dwave_state.idx[0] = ~c;
        channels = 2;
    }
    w = *(const unsigned short*)src;
    dwave_state.pred[0] = (w >> 8) | ((w & 0xFF) << 8);
    src += 2;
    if (channels > 1) {
        dwave_state.idx[1] = *src++;
        w = *(const unsigned short*)src;
        dwave_state.pred[1] = (w >> 8) | ((w & 0xFF) << 8);
        src += 2;
    }
    if (dwave_pSound) {
        unsigned total = dwave_total;

        if (dwave_state.packets == 0) {
            unsigned len;

            memcpy(dwave_pPacket, "PSAD", 4);
            len = v * channels * 2 + 0x2E;
            *(unsigned*)(dwave_pPacket + 4) = DWAVE_SWAP32(len);
            *(short*)(dwave_pPacket + 8) = 100;
            *(short*)(dwave_pPacket + 10) = dwave_state.packets;
            *(short*)(dwave_pPacket + 12) = 100;
            *(char*)(dwave_pPacket + 16) = 0x7F;
            *(char*)(dwave_pPacket + 17) = 0;
            len = total + 0x24;
            *(unsigned*)(dwave_pPacket + 22) = DWAVE_SWAP32(len);
            *(unsigned*)(dwave_pPacket + 30) = 0x0C000000;
            *(char*)(dwave_pPacket + 34) = 1;
            *(char*)(dwave_pPacket + 35) = 8;
            *(unsigned*)(dwave_pPacket + 36) = 0;
            len = total;
            *(unsigned*)(dwave_pPacket + 40) = DWAVE_SWAP32(len);
            *(char*)(dwave_pPacket + 44) = 5;
            *(char*)(dwave_pPacket + 45) = 0;
            if (channels == 1) {
                memcpy(dwave_pPacket + 46, "S6MZ", 4);
            }
            else {
                memcpy(dwave_pPacket + 46, "S6SZ", 4);
            }
            *(unsigned*)(dwave_pPacket + 50) = DWAVE_SWAP32(len);
            dwave_DecodeVima((short*)(dwave_pPacket + 54), src, v, channels);
        }
        else {
            unsigned len;

            memcpy(dwave_pPacket, "PSAD", 4);
            len = v * channels * 2 + 10;
            *(unsigned*)(dwave_pPacket + 4) = DWAVE_SWAP32(len);
            *(short*)(dwave_pPacket + 8) = 100;
            *(short*)(dwave_pPacket + 10) = dwave_state.packets;
            *(short*)(dwave_pPacket + 12) = 100;
            *(char*)(dwave_pPacket + 16) = 0x7F;
            *(char*)(dwave_pPacket + 17) = 0;
            dwave_DecodeVima((short*)(dwave_pPacket + 18), src, v, channels);
        }
        dwave_pSound->Play(dwave_pPacket, -1, 0x7F, 0, 0x40);
        dwave_state.packets++;
    }
    return (dwave_state.packets != 0 && dwave_total > 100000) ? 0 : 0x80;
}


static int dwave_Query1(void)
{
    return -1;
}

/* the bounds query (D_bloc16's slot 8); a wave chunk has none. LINK folded this body onto dwave_Query1 (relocations.yml) */
static int dwave_Query(const unsigned char* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight)
{
    return -1;
}

static void dwave_DecodeVima(short* pDst, const unsigned char* pSrc, unsigned count, unsigned channels)
{
    int bitPtr = 0;
    unsigned ch;
    unsigned short bits;
    const unsigned char* src = pSrc;

    if (!dwave_tableReady) {
        int start;
        int incer;

        for (start = 0, incer = 0; start < 64; start++, incer++) {
            int pos;

            for (pos = 0; pos < 89; pos++) {
                int put;
                unsigned short value;
                unsigned j;

                for (j = 0x20, value = dwave_step[pos], put = 0; j != 0; j >>= 1, value >>= 1) {
                    if (incer & j) {
                        put += value;
                    }
                }
                dwave_quant[pos * 64 + start] = put;
            }
        }
        dwave_tableReady = 1;
    }
    bits = *src++;
    bits = (bits << 8) | *src++;
    for (ch = 0; ch < channels; ch++) {
        short* pOut = pDst + ch;
        int idx = dwave_state.idx[ch];
        int pred = dwave_state.pred[ch];
        unsigned n;

        for (n = count; n != 0; n--) {
            unsigned char numBits = dwave_bits[idx];
            int highBit = 1 << (numBits - 1);
            unsigned char lowBits = highBit - 1;
            int val;

            bitPtr += numBits;
            val = ((bits & 0xFFFF) >> (16 - bitPtr)) & (highBit | lowBits);
            if (bitPtr > 7) {
                bits = ((bits & 0xFF) << 8) | *src++;
                bitPtr -= 8;
            }
            if (val & highBit) {
                val ^= highBit;
            }
            else {
                highBit = 0;
            }
            if (val == lowBits) {
                pred = (short)(bits << bitPtr) & 0xFFFFFF00;
                bits = ((bits & 0xFF) << 8) | *src++;
                pred |= (unsigned char)(bits >> (8 - bitPtr));
                bits = ((bits & 0xFF) << 8) | *src++;
            }
            else {
                int delta = dwave_quant[(val << (7 - numBits)) | (idx << 6)];

                if (val) {
                    delta += dwave_step[idx] >> (numBits - 1);
                }
                pred += highBit ? -delta : delta;
                if (pred < -32768) {
                    pred = -32768;
                }
                else if (pred > 32767) {
                    pred = 32767;
                }
            }
            *pOut = (short)pred;
            pOut += channels;
            idx += dwave_deltas[numBits][val];
            if (idx < 0) {
                idx = 0;
            }
            else if (idx > 88) {
                idx = 88;
            }
        }
        dwave_state.idx[ch] = (signed char)idx;
        dwave_state.pred[ch] = (short)pred;
    }
}
