/*
 * insane_mixer: INSANE's software sound mixer (retail 0x004EC720-0x004EDEF0; 0x004EC560-0x004EC720 and 0x004EDEF0-0x004EE6A0
 * are MASM: MixerAsm.asm, Bloc16Asm.asm). Dsnd.c drives it: `mixer_Init` builds a 17x17 pan/volume table and a soft-clip curve and allocates the
 * 32-bit accumulation buffer, `mixer_Begin` clears it, the sound engine hands every playing sample to `mixer_Feed` (resampled by
 * one of eight kernels by sample format), and `mixer_Mix` clips the accumulated samples to 16 bits. The last function is a
 * clipped bitmap blit used by blocky16. Retail has no names for any of this.
 *
 */
#include <math.h>
#include <string.h>
#include "../Memory/Memory.h"
#include "../lec.h"
#include "Mixer.h"
#include "../Decode/Decode.h"
#include "../Bitmap/Blit.h"

/* byte-swaps a 16-bit sample (big-endian sources) */
#define MIXER_SWAP(x) ((((unsigned short)(x) & 0xFF) << 8) | ((unsigned short)(x) >> 8))

static int mixer_Setup(MemoryDebugInterface* pMemory, int channels, int size, int a4);
static int mixer_Free(MemoryDebugInterface* pMemory);
static int mixer_Clear(void);
static void mixer_Add(void* pSrc, unsigned count, int bits, int channels, unsigned n, int offset, int volume, int pan, int isSigned, int isSwapped);
static int mixer_Output(short* pDst, int n);
static void mixer_Mono8(unsigned char* pSrc, unsigned count, unsigned n, int offset, int left, int right);
static void mixer_Stereo8(unsigned char* pSrc, unsigned count, unsigned n, int offset, int volume);
static void mixer_Mono16(short* pSrc, unsigned count, unsigned n, int offset, int left, int right);
static void mixer_Stereo16(short* pSrc, unsigned count, unsigned n, int offset, int volume);
static void mixer_MonoU16(unsigned short* pSrc, unsigned count, unsigned n, int offset, int left, int right);
static void mixer_StereoU16(unsigned short* pSrc, unsigned count, unsigned n, int offset, int volume);
static void mixer_MonoSwap16(short* pSrc, unsigned count, unsigned n, int offset, int left, int right);
static void mixer_StereoSwap16(short* pSrc, unsigned count, unsigned n, int offset, int volume);

int mixer_headroom = 4;
static unsigned char mixer_curve1[32] = {
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x18, 0x19, 0x1A, 0x1A, 0x1B, 0x1C, 0x1C, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F, 0x20, 0x20, 0x20, 0x20
};
static unsigned char mixer_curve2[32] = {
    0x00, 0x01, 0x02, 0x05, 0x07, 0x14, 0x23, 0x32, 0x41, 0x50, 0x55, 0x5A, 0x5F, 0x64, 0x69, 0x6E,
    0x70, 0x72, 0x74, 0x76, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x80, 0x80, 0x80
};
static int mixer_formatSizes[6] = { 4, 2, 2, 1, 2, 1 };

/* Retail's .bss block (0x017EEBC8-0x017EED1C) is in declaration order, VC6's layout for zero-initialised statics.
   The four mixer_unused_* objects are storage no code in the image reaches (whole-.text dword scan); sizes are the
   gaps, names are invented after the address. */
static int mixer_channels = 0;
static short* mixer_pClip = NULL;
static int mixer_unused_17EEBD0 = 0;
static unsigned char mixer_pan[17][17] = { 0 };
static int mixer_unused_17EECF8[2] = { 0 };
static int mixer_size = 0;
static int mixer_a4 = 0;
static int mixer_unused_17EED08[3] = { 0 };
static int* mixer_pBuffer = NULL;
static int mixer_unused_17EED18 = 0;

int mixer_Init(MemoryDebugInterface* pMemory, int headroom, int size, int a4, int a5, int a6)
{
    mixer_headroom = headroom;
    return mixer_Setup(pMemory, 2, size * 4, 0);
}

void mixer_Done(MemoryDebugInterface* pMemory)
{
    mixer_Free(pMemory);
}

int mixer_Begin(int n)
{
    return mixer_Clear();
}

void mixer_Feed(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format)
{
    int bits;
    int channels;
    int isSigned;

    switch (format) {
    case 0:
        channels = 1;
        bits = 8;
        isSigned = 1;
        break;
    case 1:
        channels = 2;
        bits = 8;
        isSigned = 1;
        break;
    case 2:
        channels = 1;
        bits = 16;
        isSigned = 0;
        break;
    case 3:
        channels = 2;
        bits = 16;
        isSigned = 0;
        break;
    case 4:
        channels = 1;
        bits = 16;
        isSigned = 1;
        break;
    case 5:
        channels = 2;
        bits = 16;
        isSigned = 1;
        break;
    }
    mixer_Add(pSrc, bytes / ((bits >> 3) * channels), bits, channels, n, offset, volume, (pan + 127) / 2, isSigned, 0);
}

int mixer_Mix(short* pDst, int n)
{
    return mixer_Output(pDst, n);
}

static int mixer_Setup(MemoryDebugInterface* pMemory, int channels, int size, int a4)
{
    int i;
    int j;
    unsigned char* p;
    short* pClip;

    p = &mixer_pan[0][0];
    for (i = 0; i < 17; i++) {
        for (j = 0; j < 17; j++) {
            *p++ = (unsigned char)((sin((j * 3.1415) * 0.03125) * i + 0.5) * 8.0);
        }
    }
    mixer_channels = channels;
#line 854
    mixer_pBuffer = pMemory->Alloc(mixer_size = size * 4, INSANE_FILE, __LINE__);
    if (!mixer_pBuffer) {
        return -1;
    }
    mixer_a4 = a4;
#line 860
    mixer_pClip = pMemory->Alloc(mixer_headroom << 16, INSANE_FILE, __LINE__);
    if (!mixer_pClip) {
        return -1;
    }
    pClip = mixer_pClip;
    for (j = 0; j < mixer_headroom << 15; j++) {
        *pClip++ = (short)((j * 524272.0 / (j * 7.0 + 262136.0) + 1.0) * 0.5);
    }
    return 0;
}

static int mixer_Free(MemoryDebugInterface* pMemory)
{
#line 889
    if (mixer_pClip) pMemory->Free(mixer_pClip, INSANE_FILE, __LINE__); mixer_pClip = NULL;
    if (mixer_pBuffer) pMemory->Free(mixer_pBuffer, INSANE_FILE, __LINE__); mixer_pBuffer = NULL;
    return 0;
}

static int mixer_Clear(void)
{
    memset(mixer_pBuffer, 0, mixer_size);
    return 0;
}

static void mixer_Add(void* pSrc, unsigned count, int bits, int channels, unsigned n, int offset, int volume, int pan, int isSigned, int isSwapped)
{
    unsigned v;
    int p;
    int left;
    int right;

    if (!mixer_pBuffer || !pSrc || !count) {
        return;
    }
    if (channels == 1) {
        v = volume >> 3;
        if (volume) {
            v++;
        }
        if (v >= 17) {
            v = 16;
        }
        p = (pan >> 3) - 8;
        if (pan > 64) {
            p++;
        }
        left = mixer_pan[v][8 - p];
        right = mixer_pan[v][8 + p];
        if (bits == 8) {
            mixer_Mono8(pSrc, count, n, offset, left, right);
        }
        else if (!isSigned) {
            mixer_MonoU16(pSrc, count, n, offset, left, right);
        }
        else if (isSwapped) {
            mixer_MonoSwap16(pSrc, count, n, offset, left, right);
        }
        else {
            mixer_Mono16(pSrc, count, n, offset, left, right);
        }
    }
    else {
        if (bits == 8) {
            mixer_Stereo8(pSrc, count, n, offset, volume + 1);
        }
        else if (!isSigned) {
            mixer_StereoU16(pSrc, count, n, offset, volume + 1);
        }
        else if (isSwapped) {
            mixer_StereoSwap16(pSrc, count, n, offset, volume + 1);
        }
        else {
            mixer_Stereo16(pSrc, count, n, offset, volume + 1);
        }
    }
}

static int mixer_Output(short* pDst, int n)
{
    int* pBuf = mixer_pBuffer;
    short* pOut = pDst;
    int s;

    if (!pBuf || !pOut || !n) {
        return -1;
    }
    if (mixer_channels == 2) {
        n <<= 1;
    }
    while (n--) {
        s = *pBuf++ >> 7;
        if (s < -0x8000) {
            s = -0x8000;
        }
        else if (s > 0x7FFF) {
            s = 0x7FFF;
        }
        *pOut++ = (short)s;
    }
    return 0;
}

static void mixer_Mono8(unsigned char* pSrc, unsigned count, unsigned n, int offset, int left, int right)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
            *pBuf++ += (((pSrc[0] + pSrc[2]) << 7) ^ 0x8000) * left;
            *pBuf++ += (((pSrc[0] + pSrc[2]) << 7) ^ 0x8000) * right;
            pSrc++;
        }
        *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
        *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
        *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
        *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
        break;
    case 0x10000:
        while (n--) {
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
            pSrc++;
        }
        break;
    case 0x20000:
        while (n--) {
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
            pSrc += 2;
        }
        break;
    default:
        frac = 0;
        while (n--) {
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * left;
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * right;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_Stereo8(unsigned char* pSrc, unsigned count, unsigned n, int offset, int volume)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
            *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
            *pBuf++ += (((pSrc[2] + pSrc[0]) << 7) ^ 0x8000) * volume;
            pSrc++;
            *pBuf++ += (((pSrc[2] + pSrc[0]) << 7) ^ 0x8000) * volume;
            pSrc++;
        }
        *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
        *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
        *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
        *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
        break;
    case 0x10000:
        n <<= 1;
        while (n--) {
            *pBuf++ += ((*pSrc++ ^ 0x80) << 8) * volume;
        }
        break;
    case 0x20000:
        while (n--) {
            *pBuf++ += ((*pSrc++ ^ 0x80) << 8) * volume;
            *pBuf++ += ((*pSrc++ ^ 0x80) << 8) * volume;
            pSrc += 2;
        }
        break;
    default:
        frac = 0;
        step <<= 1;
        while (n--) {
            *pBuf++ += ((pSrc[0] ^ 0x80) << 8) * volume;
            *pBuf++ += ((pSrc[1] ^ 0x80) << 8) * volume;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_Mono16(short* pSrc, unsigned count, unsigned n, int offset, int left, int right)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            *pBuf++ += pSrc[0] * left;
            *pBuf++ += pSrc[0] * right;
            *pBuf++ += ((pSrc[0] + pSrc[2]) >> 1) * left;
            *pBuf++ += ((pSrc[0] + pSrc[2]) >> 1) * right;
            pSrc++;
        }
        *pBuf++ += pSrc[0] * left;
        *pBuf++ += pSrc[0] * right;
        *pBuf++ += pSrc[0] * left;
        *pBuf++ += pSrc[0] * right;
        break;
    case 0x10000:
        while (n--) {
            *pBuf++ += pSrc[0] * left;
            *pBuf++ += pSrc[0] * right;
            pSrc++;
        }
        break;
    case 0x20000:
        while (n--) {
            *pBuf++ += pSrc[0] * left;
            *pBuf++ += pSrc[0] * right;
            pSrc += 2;
        }
        break;
    default:
        frac = 0;
        while (n--) {
            *pBuf++ += pSrc[0] * left;
            *pBuf++ += pSrc[0] * right;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_Stereo16(short* pSrc, unsigned count, unsigned n, int offset, int volume)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            *pBuf++ += pSrc[0] * volume;
            *pBuf++ += pSrc[1] * volume;
            *pBuf++ += ((pSrc[2] + pSrc[0]) >> 1) * volume;
            pSrc++;
            *pBuf++ += ((pSrc[2] + pSrc[0]) >> 1) * volume;
            pSrc++;
        }
        *pBuf++ += pSrc[0] * volume;
        *pBuf++ += pSrc[1] * volume;
        *pBuf++ += pSrc[0] * volume;
        *pBuf++ += pSrc[1] * volume;
        break;
    case 0x10000:
        n <<= 1;
        while (n--) {
            *pBuf++ += *pSrc++ * volume;
        }
        break;
    case 0x20000:
        while (n--) {
            *pBuf++ += *pSrc++ * volume;
            *pBuf++ += *pSrc++ * volume;
            pSrc += 2;
        }
        break;
    default:
        frac = 0;
        step <<= 1;
        while (n--) {
            *pBuf++ += pSrc[0] * volume;
            *pBuf++ += pSrc[1] * volume;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_MonoU16(unsigned short* pSrc, unsigned count, unsigned n, int offset, int left, int right)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;
    short s;
    short t;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            s = (pSrc[0] ^ 0x8000);
            t = (pSrc[2] ^ 0x8000);
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            *pBuf++ += ((t + s) >> 1) * left;
            *pBuf++ += ((t + s) >> 1) * right;
            pSrc++;
        }
        s = (pSrc[0] ^ 0x8000);
        *pBuf++ += s * left;
        *pBuf++ += s * right;
        *pBuf++ += s * left;
        *pBuf++ += s * right;
        break;
    case 0x10000:
        while (n--) {
            s = *pSrc++;
            s ^= 0x8000;
            *pBuf++ += s * left;
            *pBuf++ += s * right;
        }
        break;
    case 0x20000:
        while (n--) {
            s = (pSrc[0] ^ 0x8000);
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            pSrc += 2;
        }
        break;
    default:
        frac = 0;
        while (n--) {
            s = (pSrc[0] ^ 0x8000);
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_StereoU16(unsigned short* pSrc, unsigned count, unsigned n, int offset, int volume)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;
    short s0;
    short s1;
    short s2;
    short s3;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            s0 = pSrc[0];
            s1 = pSrc[1];
            s2 = pSrc[2];
            s3 = pSrc[3];
            s0 ^= 0x8000;
            s1 ^= 0x8000;
            s2 ^= 0x8000;
            s3 ^= 0x8000;
            *pBuf++ += s0 * volume;
            *pBuf++ += s1 * volume;
            *pBuf++ += ((s2 + s0) >> 1) * volume;
            *pBuf++ += ((s3 + s1) >> 1) * volume;
            pSrc += 2;
        }
        s0 = pSrc[0];
        s0 ^= 0x8000;
        s1 = pSrc[1];
        s1 ^= 0x8000;
        *pBuf++ += s0 * volume;
        *pBuf++ += s1 * volume;
        *pBuf++ += s0 * volume;
        *pBuf++ += s1 * volume;
        break;
    case 0x10000:
        n <<= 1;
        while (n--) {
            s0 = *pSrc++;
            s0 ^= 0x8000;
            *pBuf++ += s0 * volume;
        }
        break;
    case 0x20000:
        while (n--) {
            s0 = *pSrc++;
            s1 = *pSrc;
            s0 ^= 0x8000;
            s1 ^= 0x8000;
            *pBuf++ += s0 * volume;
            *pBuf++ += s1 * volume;
            pSrc += 3;
        }
        break;
    default:
        frac = 0;
        step <<= 1;
        while (n--) {
            s0 = pSrc[0];
            s1 = pSrc[1];
            s0 ^= 0x8000;
            s1 ^= 0x8000;
            *pBuf++ += s0 * volume;
            *pBuf++ += s1 * volume;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_MonoSwap16(short* pSrc, unsigned count, unsigned n, int offset, int left, int right)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;
    short s;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            *pBuf++ += MIXER_SWAP(pSrc[0]) * left;
            *pBuf++ += MIXER_SWAP(pSrc[0]) * right;
            s = MIXER_SWAP((pSrc[2] + pSrc[0]) >> 1);
            *pBuf++ += s * left;
            *pBuf++ += s * right;
            pSrc++;
        }
        s = MIXER_SWAP(*pSrc);
        *pBuf++ += s * left;
        *pBuf++ += s * right;
        *pBuf++ += s * left;
        *pBuf++ += s * right;
        break;
    case 0x10000:
        while (n--) {
            *pBuf++ += MIXER_SWAP(*pSrc) * left;
            *pBuf++ += MIXER_SWAP(*pSrc) * right;
            pSrc++;
        }
        break;
    case 0x20000:
        while (n--) {
            *pBuf++ += MIXER_SWAP(*pSrc) * left;
            *pBuf++ += MIXER_SWAP(*pSrc) * right;
            pSrc += 2;
        }
        break;
    default:
        frac = 0;
        while (n--) {
            *pBuf++ += MIXER_SWAP(*pSrc) * left;
            *pBuf++ += MIXER_SWAP(*pSrc) * right;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}

static void mixer_StereoSwap16(short* pSrc, unsigned count, unsigned n, int offset, int volume)
{
    int* pBuf = mixer_pBuffer + offset * 2;
    unsigned step = (count << 16) / n;
    unsigned frac;

    switch (step) {
    case 0x8000:
        count--;
        while (count--) {
            *pBuf++ += MIXER_SWAP(pSrc[0]) * volume;
            *pBuf++ += MIXER_SWAP(pSrc[1]) * volume;
            *pBuf++ += MIXER_SWAP((pSrc[2] + pSrc[0]) >> 1) * volume;
            pSrc++;
            *pBuf++ += MIXER_SWAP((pSrc[2] + pSrc[0]) >> 1) * volume;
            pSrc++;
        }
        *pBuf++ += MIXER_SWAP(pSrc[0]) * volume;
        *pBuf++ += MIXER_SWAP(pSrc[1]) * volume;
        *pBuf++ += MIXER_SWAP(pSrc[0]) * volume;
        *pBuf++ += MIXER_SWAP(pSrc[1]) * volume;
        break;
    case 0x10000:
        n <<= 1;
        while (n--) {
            *pBuf++ += MIXER_SWAP(*pSrc) * volume;
        }
        break;
    case 0x20000:
        while (n--) {
            *pBuf++ += MIXER_SWAP(*pSrc) * volume;
            pSrc++;
            *pBuf++ += MIXER_SWAP(*pSrc) * volume;
            pSrc += 3;
        }
        break;
    default:
        frac = 0;
        step <<= 1;
        while (n--) {
            *pBuf++ += MIXER_SWAP(pSrc[0]) * volume;
            *pBuf++ += MIXER_SWAP(pSrc[1]) * volume;
            frac += step;
            pSrc += (int)frac >> 16;
            frac &= 0xFFFF;
        }
        break;
    }
}


void mixer_Blit(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable)
{
    MixerRect rect;
    int skipX;
    int skipY;
    int width;
    int height;
    int d;
    int n;
    unsigned char* pOut;
    unsigned char* pIn;
    unsigned char aTable[1024];

    if (!pClip) {
        pClip = &rect;
        rect.x = 0;
        rect.y = 0;
        rect.width = pDst->width;
        rect.height = pDst->height;
    }
    skipX = 0;
    skipY = 0;
    width = pSrc->width;
    height = pSrc->height;
    d = pClip->x - x;
    if (d > 0) {
        skipX = d;
        width -= d;
        x += d;
    }
    d = pClip->y - y;
    if (d > 0) {
        skipY = d;
        height -= d;
        y += d;
    }
    if (x + width > pClip->x + pClip->width) {
        width = pClip->x + pClip->width - x;
    }
    if (y + height > pClip->y + pClip->height) {
        height = pClip->y + pClip->height - y;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    pOut = (unsigned char*)pDst->pPixels + pDst->pitch * y + pDst->pixelSize * x;
    pIn = (unsigned char*)pSrc->pPixels + pSrc->pitch * skipY + pSrc->pixelSize * skipX;
    if (pSrc->bpp == pDst->bpp && pSrc->colorMode == pDst->colorMode
        && ((pSrc->colorMode != 1 && pSrc->colorMode != 2)
            || (pSrc->format.redPosShift == pDst->format.redPosShift
                && pSrc->format.greenPosShift == pDst->format.greenPosShift
                && pSrc->format.bluePosShift == pDst->format.bluePosShift))) {
        blit_Wrapper(pOut, pIn, 0, 0, 0, 0, pDst->pixelSize * width, height, pDst->pitch, pSrc->pitch);
        return;
    }
    if (!pTable && pSrc->pixelSize == 1) {
        decode_MakeTable(pSrc, pDst, aTable, pPalette);
        pTable = aTable;
    }
    n = height;
    while (n--) {
        if (decode_Convert(pIn, pSrc, width, pPalette, pOut, pDst, pTable)) {
            break;
        }
        pOut += pDst->pitch;
        pIn += pSrc->pitch;
    }
}
