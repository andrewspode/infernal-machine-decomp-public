/*
 * insane_framestream: the chunked frame stream that drives INSANE's animation player (retail 0x004EBDE0-0x004EC560): a
 * FourCC state machine over the STREAMER service (header/frame/annotation/end/swap/wait chunks), plus two thin wrappers
 * around the hand-written MASM copy routines.
 */
#include <stddef.h>
#include "../lec.h"
#include "../animplay.h"
#include "Framestream.h"
#include "Cdstream.h"
#include "../Animate/Frame.h"
#include "../Bitmap/Blit.h"

typedef struct FrameHeader
{
    short version;      /* 0x00 */
    short count;        /* 0x02 */
    char  pad04[10];
    int   rate;         /* 0x0E */
    char  pad12[4];
    char  palette[0x400]; /* 0x16 */
} FrameHeader;

#define FS_NULL 'null'
#define FS_ANNO 'ANNO'
#define FS_FRME 'FRME'
#define FS_FLHD 'FLHD'
#define FS_SHDR 'SHDR'
#define FS_END  'end '
#define FS_SWAP 'paws'
#define FS_WAIT 'wait'

extern AnimFrameTable* animplay_pFrame;
extern StreamerInterface* animplay_pStreamer;
extern LecShare* animplay_pShare;
extern AnimPlayOps animplay_ops;
static int framestream_lastFrame;

void blit_MemCopy(void* pDst, const void* pSrc, int size);
void blit_MemCopyB(void* pDst, unsigned char value, int size);

static void framestream_Fetch(FrameStream* pFrames, int size);
static int framestream_Continue(FrameStream* pFrames);
static int framestream_Present(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11);
static void framestream_Header(FrameStream* pFrames, const FrameHeader* pHeader, void* pPalette, int* pFlags);

int framestream_Reset(FrameStream* pFrames)
{
    animplay_pStreamer->Reset(pFrames->pStream);
    pFrames->outFlags = 0;
    pFrames->frame = 0;
    pFrames->tag = FS_NULL;
    pFrames->frameCount = 1;
    pFrames->open = 0;
    pFrames->stalled = 0;
    pFrames->mode = 0;
    pFrames->hold = 0;
    pFrames->paused = 0;
    return 0;
}

void framestream_Close(FrameStream* pFrames)
{
}

int framestream_Tick(FrameStream* pFrames, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, volatile int a9, int a10, int a11) /* retail's scheduler treats a9's load as a volatile barrier at all three Present calls */
{
    if (pFrames->stalled) {
        pFrames->stalled = 0;
        return -1;
    }
    for (;;) {
        switch (pFrames->tag) {
        case FS_FLHD:
            framestream_Fetch(pFrames, pFrames->chunkSize);
            if ((int)pFrames->pCur == -1) {
                return 5;
            }
            if ((int)pFrames->pCur == -2) {
                return -2;
            }
            animplay_pFrame->Parse(pFrames->pCur, pFrames->chunkSize);
            pFrames->tag = FS_NULL;
            continue;
        case FS_ANNO:
            framestream_Fetch(pFrames, pFrames->chunkSize);
            if ((int)pFrames->pCur == -1) {
                return 5;
            }
            if ((int)pFrames->pCur == -2) {
                return -2;
            }
            pFrames->tag = FS_NULL;
            continue;
        case FS_FRME:
            framestream_Fetch(pFrames, pFrames->chunkSize);
            if ((int)pFrames->pCur == -1) {
                return 5;
            }
            if ((int)pFrames->pCur == -2) {
                return -2;
            }
            pFrames->tag = FS_NULL;
            if (pFrames->pCur == NULL) {
                continue;
            }
            pFrames->frame++;
            if (!pFrames->paused && framestream_Continue(pFrames)) {
                if (pFrames->frame >= pFrames->frameCount - 1) {
                    if (animplay_pStreamer->IsActive(pFrames->pStream) != 0 || pFrames->hold != 0) {
                        pFrames->tag = FS_NULL;
                    }
                    else {
                        pFrames->tag = FS_END;
                    }
                }
                continue;
            }
            framestream_Present(pFrames, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
            if (pFrames->frame >= pFrames->frameCount - 1 && !pFrames->paused) {
                if (animplay_pStreamer->IsActive(pFrames->pStream) == 0 && pFrames->hold == 0) {
                    pFrames->tag = FS_END;
                }
                else {
                    pFrames->tag = FS_NULL;
                }
            }
            else if (pFrames->mode == 1) {
                pFrames->tag = FS_SWAP;
            }
            else if (pFrames->mode == 3) {
                pFrames->mode = 1;
            }
            if (pFrames->paused) {
                return 2;
            }
            if (pFrames->frame < 10) {
                return 1;
            }
            return (pFrames->frame >= pFrames->frameCount - 1) ? 4 : 0;
        case FS_SHDR:
            framestream_Fetch(pFrames, pFrames->chunkSize);
            if ((int)pFrames->pCur == -1) {
                return 5;
            }
            if ((int)pFrames->pCur == -2) {
                return -2;
            }
            pFrames->tag = FS_NULL;
            if (pFrames->pCur != NULL) {
                framestream_Header(pFrames, (const FrameHeader*)pFrames->pCur, a6, a8);
            }
            continue;
        case FS_NULL:
            if (pFrames->hold) {
                pFrames->hold = 0;
                pFrames->paused = 1;
            }
            framestream_Fetch(pFrames, 8);
            if ((int)pFrames->pCur == -1) {
                return 5;
            }
            if ((int)pFrames->pCur == -2) {
                return -2;
            }
            if (pFrames->pCur != NULL) {
                const unsigned char* pBytes = (const unsigned char*)pFrames->pCur;

                pFrames->tag = (pBytes[0] << 24) | (pBytes[1] << 16) | (pBytes[2] << 8) | pBytes[3];
                pFrames->chunkSize = (((pBytes[4] << 24) | (pBytes[5] << 16) | (pBytes[6] << 8) | pBytes[7]) + 1) & ~1;
            }
            continue;
        case FS_END:
            pFrames->frame++;
            if (framestream_Continue(pFrames) == 0) {
                framestream_Present(pFrames, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
            }
            if (animplay_pStreamer->IsActive(pFrames->pStream) || (pFrames->hold && animplay_pFrame->Fetch(-8))) {
                pFrames->tag = FS_NULL;
            }
            return 4;
        case FS_WAIT:
            if (animplay_pStreamer->IsActive(pFrames->pStream) || (pFrames->hold && animplay_pFrame->Fetch(-8))) {
                pFrames->tag = FS_NULL;
            }
            return 5;
        case FS_SWAP:
            framestream_Present(pFrames, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
            if (pFrames->mode != 1) {
                if (pFrames->mode == 3) {
                    pFrames->mode = 1;
                }
                pFrames->tag = FS_NULL;
            }
            return 3;
        default:
            animplay_pShare->ReportError("Read invalid frame header %c%c%c%c -- CD may be dirty or damaged.",
                                            (unsigned)pFrames->tag >> 24, (unsigned)pFrames->tag >> 16, (unsigned)pFrames->tag >> 8, pFrames->tag);
            return -2;
        }
    }
}

static void framestream_Fetch(FrameStream* pFrames, int size)
{
    if (pFrames->paused) {
        pFrames->pCur = animplay_pFrame->Fetch(size);
        animplay_pStreamer->Advance(pFrames->pStream, -1);
        if (pFrames->pCur == NULL) {
            pFrames->paused = 0;
            pFrames->seekLen = 0;
            pFrames->seekBase = 0;
            pFrames->tag = FS_WAIT;
        }
    }
    else {
        pFrames->pCur = (void*)animplay_pStreamer->Advance(pFrames->pStream, size);
        if ((int)pFrames->pCur != -1 && pFrames->pCur == NULL) {
            pFrames->frame = -1;
            pFrames->open = 0;
        }
    }
}

static int framestream_Continue(FrameStream* pFrames)
{
    if (!pFrames->paused) {
        if (pFrames->open) {
            if (pFrames->seekLen >= 0 && pFrames->frame > pFrames->seekBase + pFrames->seekLen) {
                return 1;
            }
        }
        else if (pFrames->seekTo >= 0) {
            if (pFrames->frame < pFrames->seekTo) {
                return 1;
            }
            pFrames->seekTo = -1;
        }
        framestream_lastFrame = pFrames->frame;
    }
    return 0;
}

static int framestream_Present(FrameStream* pFrames, void* pBitmap, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11)
{
    int result;

    if (!((a9 | pFrames->outFlags) & 4)) {
        blit_Fill(pBitmap, 0);
    }
    result = animplay_pFrame->Decode(pBitmap, a3, a4, a5, a6, a7, a8, a9 | pFrames->outFlags, a10, a11, pFrames->pCur, pFrames->chunkSize);
    if (result & 1) {
        framestream_Stall(pFrames);
    }
    if (result & 2) {
        animplay_ops.Rewind(pFrames);
    }
    return 0;
}

static void framestream_Header(FrameStream* pFrames, const FrameHeader* pHeader, void* pPalette, int* pFlags)
{
    pFrames->outFlags = pFrames->flagsIn;
    if (!(pFrames->flagsIn & 0x2000)) {
        framestream_Copy(pPalette, pHeader->palette, 0x400);
        *pFlags |= 3;
    }
    pFrames->frameCount = pHeader->count;
    if (pHeader->rate && !(pFrames->outFlags & 8)) {
        animplay_ops.SetRate(pFrames, 1000000 / pHeader->rate);
        animplay_ops.Rewind(pFrames);
    }
}

int framestream_IsOpen(FrameStream* pFrames)
{
    return pFrames->open;
}

void framestream_Stall(FrameStream* pFrames)
{
    animplay_pFrame->Reset();
    pFrames->stalled = 1;
}

int framestream_Open(FrameStream* pFrames, const char* pName, int flags, int seekLen, int seekTo, int hold)
{
    pFrames->hold = hold;
    pFrames->flagsIn = flags;
    if (animplay_pStreamer->SetFile(pFrames->pStream, pName, 8)) {
        return -1;
    }
    pFrames->open = 1;
    framestream_Seek(pFrames, seekLen, seekTo);
    return 0;
}

void framestream_Seek(FrameStream* pFrames, int length, int to)
{
    pFrames->seekBase = pFrames->frame;
    pFrames->seekTo = to;
    pFrames->seekLen = length;
}

int framestream_Mode(FrameStream* pFrames, int op)
{
    if (op == 2) {
        pFrames->mode = (pFrames->mode == 0);
    }
    else if (op == 4) {
        if (pFrames->mode) {
            pFrames->mode = 3;
        }
    }
    else if (op != 5) {
        pFrames->mode = op;
    }
    return pFrames->mode;
}


void framestream_Copy(void* pDst, const void* pSrc, int size)
{
    blit_MemCopy(pDst, pSrc, size);
}

void framestream_CopyB(void* pDst, unsigned char value, int size)
{
    blit_MemCopyB(pDst, value, size);
}
