/*
 * insane_animframe: INSANE's `ANIMFRAME=insane` service (retail 0x004E4230-0x004E4D50): walks the chunks of a frame, hands each to the
 * FLOBDEC decoder that claims its tag, and handles the frame-level chunks itself (`load`, `ftch`, `npal`, `stor`), palette fades
 * included.
 */
#include <stddef.h>
#include <string.h>
#include "../lec.h"
#include "../Memory/Memory.h"
#include "../File/Framestream.h"
#include "Frame.h"

typedef struct FlobDecoder
{
    void* pfn00;
    int  (*GetTag)(void);
    int  (*GetFlags)(void);
    void (*Reset)(void);
    void (*Parse)(const unsigned char* pData, int size);
    unsigned (*Decode)(void* pBitmap, int a2, int a3, int a4, void* pPalette, const unsigned char* pData, int size, int mode, int a9, int a10);
} FlobDecoder;

typedef struct DisplayInfo
{
    char pad[0x20];
    int f20;
} DisplayInfo;

typedef struct DisplayService
{
    void* pfn00;
    DisplayInfo* (*GetInfo)(void);
    void (*Blit)(void* pBitmap, int a3, int a4, int a2);
    void* pfn0C;
    void (*Present)(void* pPalette);
    void* pfn14;
    void (*SetPalette)(int a, int b, int count);
} DisplayService;

typedef struct AnimFrameDebug
{
    const char* (*Name)(int index);
    void (*SetFlags)(int flags);
    int  (*GetFlags)(void);
} AnimFrameDebug;

int animframe_Register(LecShare* pShare, void* pParam);
static void animframe_Unregister(void);
static int animframe_Startup(void);
static void animframe_Shutdown(void);
static void animframe_ResetDecoders(void);
static int animframe_Parse(const unsigned char* pData, int size);
static unsigned animframe_Decode(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, unsigned flags, int a9, int a10, const unsigned char* pData, int size);
static void animframe_Load(const short* pData, int size);
static void* animframe_Fetch(int n);
static void animframe_Draw(void* pBitmap, int a2, int a3, int a4, void* pPalette, int a6, int a7, int a8);
static const char* animframe_DebugName(int index);
static void animframe_SetDebug(int flags);
static int animframe_GetDebug(void);

static int animframe_lastDecoder = -1;
static int animframe_cursor = 8;
static LecModule animframe_module = { animframe_Register, animframe_Unregister, animframe_Startup, animframe_Shutdown };
static AnimFrameTable animframe_interface = {
    animframe_ResetDecoders, animframe_Parse, animframe_Decode, animframe_Draw, animframe_Load, animframe_Fetch
};
static AnimFrameDebug animframe_debug = { animframe_DebugName, animframe_SetDebug, animframe_GetDebug };

MemoryDebugInterface* animframe_pMemory;
void* animframe_pBitmap;
LecShare* animframe_pShare;
void* animframe_pDebug;
DisplayService* animframe_pDisplay;

static int animframe_flags[100] = { 0 };
static FlobDecoder* animframe_decoders[100] = { 0 };
static int animframe_tags[100] = { 0 };
static void* animframe_pParam = NULL;
static char animframe_palette[0x400] = { 0 };
static union {
    int mask;
    struct {
        unsigned bPad : 19;
        unsigned bParse : 1;
    };
} animframe_debugMask = { 0 };
static int animframe_count = 0;
static unsigned char* animframe_pOverlay = NULL;
static unsigned char* animframe_pPreload = NULL;
static int animframe_overlaySize = 0;
static int animframe_preloadSize = 0;
static int animframe_loaded = 0;
static int animframe_fade = 0;
static int animframe_lastFragment = 0;
static int animframe_fragTag = 0;

int animframe_Register(LecShare* pShare, void* pParam)
{
    animframe_pShare = pShare;
    animframe_pParam = pParam;
    pShare->Publish("LECMODULE=insane_animframe", &animframe_module);
    animframe_pShare->Publish("ANIMFRAME=insane", &animframe_interface);
    animframe_pShare->Publish("DEBUG=animframe", &animframe_debug);
    return 0;
}

static void animframe_Unregister(void)
{
    animframe_pShare->Unpublish("ANIMFRAME=insane");
    animframe_pShare->Unpublish("LECMODULE=insane_animframe");
    animframe_pShare->Unpublish("DEBUG=animframe");
}

static int animframe_Startup(void)
{
    int rc;
    int i;

    animframe_pBitmap = animframe_pShare->Subscribe("BITMAP.buffers", animframe_pParam);
    animframe_pMemory = animframe_pShare->Subscribe("MEMORY.workspace", animframe_pParam);
    if (animframe_pBitmap == NULL) {
        animframe_pShare->ReportError("AnimFrame Module: Cannot allocate bitmap services.");
        return -1;
    }
    if (animframe_pMemory == NULL) {
        animframe_pShare->ReportError("AnimFrame Module: Cannot allocate memory services.");
        return -1;
    }
    animframe_pDebug = animframe_pShare->Subscribe("VIRTHERC.debug", animframe_pParam);
    animframe_pDisplay = animframe_pShare->Subscribe("VIDEO.display", animframe_pParam);
    if (animframe_overlaySize) {
#line 205
        animframe_pOverlay = animframe_pMemory->Alloc(animframe_overlaySize, INSANE_FILE, __LINE__);
        rc = animframe_pOverlay ? 0 : -1;
        if (rc) {
            animframe_pShare->ReportError("AnimFrame Module: Can't allocate overlay buffer (%ld bytes)", animframe_overlaySize);
            return rc;
        }
    }
    if (animframe_preloadSize) {
#line 206
        animframe_pPreload = animframe_pMemory->Alloc(animframe_preloadSize, INSANE_FILE, __LINE__);
        rc = animframe_pPreload ? 0 : -1;
        if (rc) {
            animframe_pShare->ReportError("AnimFrame Module: Can't allocate preload buffer (%ld bytes)", animframe_preloadSize);
            return rc;
        }
    }
    memset(animframe_decoders, 0, sizeof(animframe_decoders));
    animframe_count = animframe_pShare->Count("FLOBDEC");
    if (animframe_count > 100) {
        animframe_count = 100;
    }
    for (i = 0; i < animframe_count; i++) {
        void* pName = animframe_pShare->Enumerate("FLOBDEC", i);

        if (pName) {
            animframe_decoders[i] = animframe_pShare->Subscribe("FLOBDEC.temp", pName);
            animframe_tags[i] = animframe_decoders[i]->GetTag();
            animframe_flags[i] = animframe_decoders[i]->GetFlags();
        }
    }
    return 0;
}

static void animframe_Shutdown(void)
{
    int i;

    animframe_pShare->Unsubscribe(animframe_pBitmap);
    animframe_pShare->Unsubscribe(animframe_pMemory);
    if (animframe_pDisplay) {
        animframe_pShare->Unsubscribe(animframe_pDisplay);
    }
    if (animframe_pDebug) {
        animframe_pShare->Unsubscribe(animframe_pDebug);
    }
    if (animframe_pOverlay) {
#line 256
        animframe_pMemory->Free(animframe_pOverlay, INSANE_FILE, __LINE__);
        animframe_pOverlay = NULL;
    }
    if (animframe_pPreload) {
#line 257
        animframe_pMemory->Free(animframe_pPreload, INSANE_FILE, __LINE__);
        animframe_pPreload = NULL;
    }
    for (i = 0; i < animframe_count; i++) {
        if (animframe_decoders[i]) {
            animframe_pShare->Unsubscribe(animframe_decoders[i]);
        }
        animframe_decoders[i] = NULL;
    }
    animframe_count = 0;
}

static void animframe_ResetDecoders(void)
{
    int i;

    for (i = 0; i < animframe_count; i++) {
        if (animframe_decoders[i]) {
            animframe_decoders[i]->Reset();
        }
    }
}

static int animframe_Parse(const unsigned char* pData, int size)
{
    if (size > 4) {
        do {
            unsigned tag = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
            unsigned len = (pData[4] << 24) | (pData[5] << 16) | (pData[6] << 8) | pData[7];
            int i;

            for (i = 0; i < animframe_count; i++) {
                if (tag != 0 && tag == (unsigned)animframe_tags[i] && animframe_decoders[i]) {
                    animframe_decoders[i]->Parse(pData + 8, len);
                }
            }
            size += -8 - len;
            pData += 8 + len;
            if (len & 1) {
                pData++;
                size--;
            }
        } while (size > 4);
    }
    return 0;
}

static unsigned animframe_Decode(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, unsigned flags, int a9, int a10, const unsigned char* pData, int size)
{
    int seen = 0;
    unsigned result = 0;
    int mode = (flags & 0x30) | ((flags >> 13) & 4);
    int i;

    while (size > 4) {
        unsigned tag = (pData[0] << 24) | (pData[1] << 16) | (pData[2] << 8) | pData[3];
        unsigned len = (pData[4] << 24) | (pData[5] << 16) | (pData[6] << 8) | pData[7];
        unsigned ret;

        if (animframe_pDebug && animframe_debugMask.bParse) {
            ((void (*)(const char*, ...))((void**)animframe_pDebug)[3])("Parsing Block '%c%c%c%c' (%ld)\n", pData[0], pData[1], pData[2], pData[3], len);
        }
        for (i = 0; i < animframe_count; i++) {
            if (tag != 0 && tag == (unsigned)animframe_tags[i] && animframe_decoders[i]) {
                if (seen && animframe_pOverlay && (animframe_flags[i] & 0x10)) {
                    if (len + 8 < (unsigned)animframe_overlaySize) {
                        framestream_Copy(animframe_pOverlay, pData, len + 8);
                        animframe_lastDecoder = i;
                    }
                    else {
                        animframe_lastDecoder = -1;
                    }
                    seen = 0;
                }
                ret = animframe_decoders[i]->Decode(pBitmap, a2, a3, a4, pPalette, pData + 8, len, mode, a9, a10);
                if (ret & 1) {
                    *pOutA |= 1;
                }
                if (ret & 2) {
                    *pOutB |= 1;
                }
                if (ret & 4) {
                    *pOutB |= 2;
                }
                if (ret & 0x80) {
                    result |= 2;
                }
                break;
            }
        }
        pData += 8;
        if (i >= animframe_count) {
            switch (tag) {
            case 'load':
                animframe_Load((const short*)pData, len);
                break;
            case 'ftch':
                if (animframe_pOverlay && animframe_lastDecoder >= 0) {
                    ret = animframe_decoders[animframe_lastDecoder]->Decode(pBitmap, a2, a3, a4, pPalette, animframe_pOverlay + 8,
                                                                            (animframe_pOverlay[4] << 24) | (animframe_pOverlay[5] << 16) | (animframe_pOverlay[6] << 8) | animframe_pOverlay[7],
                                                                            mode, a9, a10);
                    if (ret & 1) {
                        *pOutA |= 1;
                    }
                    if (ret & 2) {
                        *pOutB |= 1;
                    }
                    if (ret & 4) {
                        *pOutB |= 2;
                    }
                }
                break;
            case 'npal':
                if (*(const short*)(pData + 0xC) == 0) {
                    framestream_Copy((char*)pPalette + (*(const short*)(pData + 0xE) << 4), pData + 0x12, *(const short*)(pData + 0x10) << 2);
                    *pOutB |= *(const int*)(pData + 8);
                    animframe_fade = 0;
                }
                else {
                    framestream_Copy(animframe_palette, pPalette, 0x400);
                    framestream_Copy(animframe_palette + (*(const short*)(pData + 0xE) << 4), pData + 0x12, *(const short*)(pData + 0x10) << 2);
                    animframe_fade = *(const short*)(pData + 0xC);
                }
                break;
            case 'stor':
                seen = 1;
                break;
            default:
                if (animframe_pDebug && (animframe_debugMask.mask & 0x40000)) {
                    ((void (*)(const char*, ...))((void**)animframe_pDebug)[3])("Found Unknown Block '%c%c%c%c'\n", pData[-4], pData[-3], pData[-2], pData[-1]);
                }
                if (((unsigned char*)&tag)[0] < '0' || ((unsigned char*)&tag)[0] > 'z' || ((unsigned char*)&tag)[1] < '0' || ((unsigned char*)&tag)[1] > 'z'
                    || ((unsigned char*)&tag)[2] < '0' || ((unsigned char*)&tag)[2] > 'z' || ((unsigned char*)&tag)[3] < '0' || ((unsigned char*)&tag)[3] > 'z') {
                    if (animframe_pDebug && (animframe_debugMask.mask & 0x20000)) {
                        ((void (*)(const char*, ...))((void**)animframe_pDebug)[3])("Bad Header Found '%c%c%c%c'\n", pData[-4], pData[-3], pData[-2], pData[-1]);
                    }
                    return result | 0x8000;
                }
                break;
            }
        }
        pData += len;
        size -= len + 8;
        if (len & 1) {
            pData++;
            size--;
        }
    }
    if (animframe_fade) {
        unsigned* pCur = (unsigned*)pPalette;
        unsigned* pTgt = (unsigned*)animframe_palette;

        *pOutB |= 1;
        for (i = 0; i < 256; i++) {
            unsigned c = pCur[i];
            unsigned b0 = c & 0xFF;
            unsigned b1 = (c >> 8) & 0xFF;
            unsigned b2 = (c >> 16) & 0xFF;
            unsigned b3 = c >> 24;
            unsigned t = pTgt[i];

            b0 = (unsigned char)(b0 + ((t & 0xFF) - b0) / (unsigned)animframe_fade);
            b1 = (unsigned char)(b1 + (((t >> 8) & 0xFF) - b1) / (unsigned)animframe_fade);
            b2 = (unsigned char)(b2 + (((t >> 16) & 0xFF) - b2) / (unsigned)animframe_fade);
            b3 = (unsigned char)(b3 + ((t >> 24) - b3) / (unsigned)animframe_fade);
            pCur[i] = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
        }
        animframe_fade--;
    }
    return result;
}

static void animframe_Load(const short* pData, int size)
{
    int seq = pData[1];
    int base = pData[0];

    if (!seq) {
        animframe_cursor = 8;
        animframe_loaded = 0;
        animframe_lastFragment = -1;
        animframe_fragTag = base;
    }
    if (seq == animframe_lastFragment + 1) {
        if (animframe_loaded + size < animframe_preloadSize) {
            framestream_Copy(animframe_pPreload + animframe_loaded, (const char*)pData + 10, size - 10);
            animframe_loaded += size - 10;
        }
        animframe_lastFragment = seq;
    }
}

static void* animframe_Fetch(int n)
{
    if (n < 0) {
        if (animframe_cursor - n <= animframe_loaded) {
            return animframe_pPreload - n + animframe_cursor;
        }
    }
    else {
        if (animframe_cursor + n <= animframe_loaded) {
            animframe_cursor += n;
            return animframe_pPreload - n + animframe_cursor;
        }
        animframe_cursor = 8;
    }
    return NULL;
}

static void animframe_Draw(void* pBitmap, int a2, int a3, int a4, void* pPalette, int a6, int a7, int a8)
{
    if (animframe_pDisplay && (a7 & 2)) {
        if (pBitmap == NULL || animframe_pDisplay->GetInfo()->f20 == 0) {
            if (!(a8 & 0x2000)) {
                animframe_pDisplay->SetPalette(0, 0, 0x100);
            }
        }
    }
    if ((a6 & 1) && !(a8 & 0x4000) && animframe_pDisplay) {
        animframe_pDisplay->Blit(pBitmap, a3, a4, a2);
    }
    if ((a7 & 1) && !(a8 & 0x2000) && animframe_pDisplay) {
        animframe_pDisplay->Present(pPalette);
    }
}

static const char* animframe_DebugName(int index)
{
    switch (index) {
    case 0:
        return "Insane AnimFrame";
    case 1:
        return "Insane AnimFrame Debug Interface";
    case 2:
        return "This is the debugging interface for the Insane Animation Frame module.";
    }
    return "";
}

static void animframe_SetDebug(int flags)
{
    animframe_debugMask.mask = flags;
}

static int animframe_GetDebug(void)
{
    return animframe_debugMask.mask;
}
