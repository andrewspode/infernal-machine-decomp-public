/*
 * insane_blocky16_flobdec: INSANE's `FLOBDEC=insane_blocky16` frame-object decoder (retail 0x004E8510-0x004E9810): decodes the
 * `Bl16` chunk of a frame (LucasArts' 16-bit "blocky16" video codec, the 16-bit relative of SMUSH codec 47) into one of three
 * 16-bit bitmaps and hands the shown one to the host's blit callback (SmushPlay's) or to Mixer.c's clipped blit. Codec 2's block
 * decoder is hand-written MASM (Bloc16Asm.asm, retail 0x004EDF70); the glyph and motion tables it reads are built
 * here (the same tables ScummVM's codec47/blocky16 decoders build). Retail has no names for any of this.
 *
 */
#include <stdarg.h>
#include <stddef.h>
#include "../lec.h"
#include "../Memory/Memory.h"
#include "../Bitmap/Bitmap.h"
#include "../Sound/Mixer.h"




#define DBLOC16_ABS(a) ((a) < 0 ? -(a) : (a))
#define DBLOC16_MAX(a, b) ((a) > (b) ? (a) : (b))

/* the `Bl16` chunk as the decoder reads it (offsets from retail; the 16-bit fields are padded to 32) */
typedef struct Bl16Header
{
    short x;                        /* 0x000 */
    short pad02;
    short y;                        /* 0x004 */
    short pad06;
    short width;                    /* 0x008 */
    short pad0A;
    short height;                   /* 0x00C */
    short pad0E;
    short seq;                      /* 0x010 */
    unsigned char codec;            /* 0x012 */
    unsigned char rotate;           /* 0x013 */
    unsigned char pad14[4];
    unsigned char params[8];        /* 0x018: codec 2's motion parameters */
    unsigned char fill[4];          /* 0x020: the pixel the delta buffers start with */
    int rleSize;                    /* 0x024 */
    unsigned short palette[256];    /* 0x028 */
    short depth;                    /* 0x228 */
    short pad22A[3];
    unsigned char data[1];          /* 0x230 */
} Bl16Header;

/* the frame object's parse header */
typedef struct Bl16Parse
{
    short codec;
    short width;
    short height;
    short depth;
} Bl16Parse;

typedef struct Bl16Motion
{
    short x;
    short y;
} Bl16Motion;

/* the glyph tables: per (from-edge, to-edge) pair, the pixel offsets of the two halves of a 4x4 or 8x8 glyph */
/* Local state of dbloc16_DecodeFrame (release-chosen aggregate). Names after ScummVM Grim Blocky16 (inferred):
   the five members before offset1/offset2 are never used here. */
typedef struct Bl16DecodeState
{
    int prevSeqNb;
    int lastTableWidth;
    const unsigned char* pSrc;
    const unsigned char* pParams;
    int pitch;
    int offset1;
    int offset2;
} Bl16DecodeState;

typedef struct Bl16Small
{
    short off1[16];                 /* 0x00 */
    short off2[16];                 /* 0x20 */
    unsigned char idx1[16];         /* 0x40 */
    unsigned char idx2[16];         /* 0x50 */
    unsigned char n1;               /* 0x60 */
    unsigned char n2;               /* 0x61 */
    unsigned char pad[30];
} Bl16Small;

typedef struct Bl16Big
{
    short off1[64];                 /* 0x000 */
    short off2[64];                 /* 0x080 */
    unsigned char idx1[64];         /* 0x100 */
    unsigned char idx2[64];         /* 0x140 */
    unsigned char n1;               /* 0x180 */
    unsigned char n2;               /* 0x181 */
    unsigned char pad[2];
} Bl16Big;

typedef int (*Bl16Blit)(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable);

/* File/Framestream.c */
void framestream_Copy(void* pDst, const void* pSrc, int size);
void framestream_CopyB(void* pDst, unsigned char value, int size);
/* Decode/Decode.c */
void decode_Rle(unsigned char* pDst, unsigned char* pSrc, int n);
void decode_RleInit(unsigned char* pSrc);
void decode_RleEnd(void);
unsigned char decode_RleByte(void);
void decode_Run(unsigned char* pDst, LecBitmapHeader* pBitmap, int n, unsigned char* pPixel);
int decode_Convert(unsigned char* pSrc, LecBitmapHeader* pSrcBitmap, int n, void* pPalette, unsigned char* pDst, LecBitmapHeader* pDstBitmap, void* pTable);
/* codec 2's block decoder (MASM, 0x004EDF70) */
void dbloc16_Codec2(unsigned short* pDst, unsigned char* pSrc, int offset1, int offset2, int width, short* pMotion, unsigned char* pParams,
                    int height, int nextLine, Bl16Small* pSmall, Bl16Big* pBig, unsigned short* pPalette);

int dbloc16_Register(LecShare* pShare, void* pParam);
int dbloc16_Configure(int tag, ...);
static void dbloc16_Unregister(void);
static int dbloc16_Startup(void);
static void dbloc16_Shutdown(void);
static const char* dbloc16_Name(int index);
static int dbloc16_GetTag(void);
static int dbloc16_GetFlags(void);
static void dbloc16_Reset(void);
static int dbloc16_Parse(const Bl16Parse* pData, int size);
static int dbloc16_Alloc(int width, int height, int depth);
static void dbloc16_Free(void);
static int dbloc16_Decode(void* a1, int a2, int a3, int a4, void* a5, const unsigned char* pData, int size, int mode, int a9, int a10);
static int dbloc16_Query1(void);
static int dbloc16_Query(const Bl16Header* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
void dbloc16_MakeTables(int width);
static void dbloc16_MakeGlyphs(int size);
static int dbloc16_DecodeFrame(LecBitmapHeader* pBitmap, MixerRect* pClip, int x, int y, void* pPalette, Bl16Header* pHdr, int flags);
static void dbloc16_BuildLut(LecBitmapHeader* pBitmap);
static void dbloc16_Scale(unsigned short* pDst, unsigned short* pSrc, int width, int height);
static unsigned short dbloc16_Average(unsigned short a, unsigned short b);
static void dbloc16_ScalePal(unsigned short* pDst, unsigned char* pSrc, int width, int height, unsigned short* pPalette);

static LecModule dbloc16_module = { dbloc16_Register, dbloc16_Unregister, dbloc16_Startup, dbloc16_Shutdown };
static struct
{
    const char* (*Name)(int index);
    int  (*GetTag)(void);
    int  (*GetFlags)(void);
    void (*Reset)(void);
    int  (*Parse)(const Bl16Parse* pData, int size);
    int  (*Decode)(void* a1, int a2, int a3, int a4, void* a5, const unsigned char* pData, int size, int mode, int a9, int a10);
    int  (*Query1)(void);
    int  (*Query)(const Bl16Header* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight);
} dbloc16_interface = { dbloc16_Name, dbloc16_GetTag, dbloc16_GetFlags, dbloc16_Reset, dbloc16_Parse, dbloc16_Decode, dbloc16_Query1, dbloc16_Query };

/* codec 2's 255 motion vectors (x, y); dbloc16_MakeTables turns them into pixel offsets for the frame width */
static Bl16Motion dbloc16_motion[255] = {
    { 0, 0 }, { -1, -43 }, { 6, -43 }, { -9, -42 }, { 13, -41 }, { -16, -40 }, { 19, -39 }, { -23, -36 },
    { 26, -34 }, { -2, -33 }, { 4, -33 }, { -29, -32 }, { -9, -32 }, { 11, -31 }, { -16, -29 }, { 32, -29 },
    { 18, -28 }, { -34, -26 }, { -22, -25 }, { -1, -25 }, { 3, -25 }, { -7, -24 }, { 8, -24 }, { 24, -23 },
    { 36, -23 }, { -12, -22 }, { 13, -21 }, { -38, -20 }, { 0, -20 }, { -27, -19 }, { -4, -19 }, { 4, -19 },
    { -17, -18 }, { -8, -17 }, { 8, -17 }, { 18, -17 }, { 28, -17 }, { 39, -17 }, { -12, -15 }, { 12, -15 },
    { -21, -14 }, { -1, -14 }, { 1, -14 }, { -41, -13 }, { -5, -13 }, { 5, -13 }, { 21, -13 }, { -31, -12 },
    { -15, -11 }, { -8, -11 }, { 8, -11 }, { 15, -11 }, { -2, -10 }, { 1, -10 }, { 31, -10 }, { -23, -9 },
    { -11, -9 }, { -5, -9 }, { 4, -9 }, { 11, -9 }, { 42, -9 }, { 6, -8 }, { 24, -8 }, { -18, -7 },
    { -7, -7 }, { -3, -7 }, { -1, -7 }, { 2, -7 }, { 18, -7 }, { -43, -6 }, { -13, -6 }, { -4, -6 },
    { 4, -6 }, { 8, -6 }, { -33, -5 }, { -9, -5 }, { -2, -5 }, { 0, -5 }, { 2, -5 }, { 5, -5 },
    { 13, -5 }, { -25, -4 }, { -6, -4 }, { -3, -4 }, { 3, -4 }, { 9, -4 }, { -19, -3 }, { -7, -3 },
    { -4, -3 }, { -2, -3 }, { -1, -3 }, { 0, -3 }, { 1, -3 }, { 2, -3 }, { 4, -3 }, { 6, -3 },
    { 33, -3 }, { -14, -2 }, { -10, -2 }, { -5, -2 }, { -3, -2 }, { -2, -2 }, { -1, -2 }, { 0, -2 },
    { 1, -2 }, { 2, -2 }, { 3, -2 }, { 5, -2 }, { 7, -2 }, { 14, -2 }, { 19, -2 }, { 25, -2 },
    { 43, -2 }, { -7, -1 }, { -3, -1 }, { -2, -1 }, { -1, -1 }, { 0, -1 }, { 1, -1 }, { 2, -1 },
    { 3, -1 }, { 10, -1 }, { -5, 0 }, { -3, 0 }, { -2, 0 }, { -1, 0 }, { 1, 0 }, { 2, 0 },
    { 3, 0 }, { 5, 0 }, { 7, 0 }, { -10, 1 }, { -7, 1 }, { -3, 1 }, { -2, 1 }, { -1, 1 },
    { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { -43, 2 }, { -25, 2 }, { -19, 2 }, { -14, 2 },
    { -5, 2 }, { -3, 2 }, { -2, 2 }, { -1, 2 }, { 0, 2 }, { 1, 2 }, { 2, 2 }, { 3, 2 },
    { 5, 2 }, { 7, 2 }, { 10, 2 }, { 14, 2 }, { -33, 3 }, { -6, 3 }, { -4, 3 }, { -2, 3 },
    { -1, 3 }, { 0, 3 }, { 1, 3 }, { 2, 3 }, { 4, 3 }, { 19, 3 }, { -9, 4 }, { -3, 4 },
    { 3, 4 }, { 7, 4 }, { 25, 4 }, { -13, 5 }, { -5, 5 }, { -2, 5 }, { 0, 5 }, { 2, 5 },
    { 5, 5 }, { 9, 5 }, { 33, 5 }, { -8, 6 }, { -4, 6 }, { 4, 6 }, { 13, 6 }, { 43, 6 },
    { -18, 7 }, { -2, 7 }, { 0, 7 }, { 2, 7 }, { 7, 7 }, { 18, 7 }, { -24, 8 }, { -6, 8 },
    { -42, 9 }, { -11, 9 }, { -4, 9 }, { 5, 9 }, { 11, 9 }, { 23, 9 }, { -31, 10 }, { -1, 10 },
    { 2, 10 }, { -15, 11 }, { -8, 11 }, { 8, 11 }, { 15, 11 }, { 31, 12 }, { -21, 13 }, { -5, 13 },
    { 5, 13 }, { 41, 13 }, { -1, 14 }, { 1, 14 }, { 21, 14 }, { -12, 15 }, { 12, 15 }, { -39, 17 },
    { -28, 17 }, { -18, 17 }, { -8, 17 }, { 8, 17 }, { 17, 18 }, { -4, 19 }, { 0, 19 }, { 4, 19 },
    { 27, 19 }, { 38, 20 }, { -13, 21 }, { 12, 22 }, { -36, 23 }, { -24, 23 }, { -8, 24 }, { 7, 24 },
    { -3, 25 }, { 1, 25 }, { 22, 25 }, { 34, 26 }, { -18, 28 }, { -32, 29 }, { 16, 29 }, { -11, 31 },
    { 9, 32 }, { 29, 32 }, { -4, 33 }, { 2, 33 }, { -26, 34 }, { 23, 36 }, { -19, 39 }, { 16, 40 },
    { -13, 41 }, { 9, 42 }, { -6, 43 }, { 1, 43 }, { 0, 0 }, { 0, 0 }, { 0, 0 }
};
static int dbloc16_lastWidth = -1;
/* the glyph edge points (x and y tables for the 4x4 and the 8x8 glyphs) */
static int dbloc16_small1[16] = { 0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1 };
static int dbloc16_small2[16] = { 0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2 };
static int dbloc16_big1[16] = { 0, 2, 5, 7, 7, 7, 7, 7, 7, 5, 2, 0, 0, 0, 0, 0 };
static int dbloc16_big2[16] = { 0, 0, 0, 0, 1, 3, 4, 6, 7, 7, 7, 7, 6, 4, 3, 1 };
static int dbloc16_prevSeq = -1;

static MemoryDebugInterface* dbloc16_pMemory = NULL;
static BitmapInterface* dbloc16_pBitmap = NULL;
static LecShare* dbloc16_pShare = NULL;
static void* dbloc16_pParam = NULL;
static Bl16Blit dbloc16_pBlit = NULL;
static LecBitmapHeader* dbloc16_pShown = NULL;
static unsigned short* dbloc16_pCur = NULL;
static unsigned short* dbloc16_pLut = NULL;
static int dbloc16_red = 0;
static int dbloc16_green = 0;
static int dbloc16_blue = 0;
static int dbloc16_depth = 0;
static LecBitmapHeader* dbloc16_pBitmap0 = NULL;
static LecBitmapHeader* dbloc16_pBitmap1 = NULL;
static LecBitmapHeader* dbloc16_pBitmap2 = NULL;
static int dbloc16_size = 0;
static unsigned short* dbloc16_pLast = NULL;

/* uninitialised globals (COMMON): the two delta buffers, the motion offsets and the glyph tables codec 2 reads */
unsigned short* dbloc16_pBuf0;
Bl16Big dbloc16_big[256];
unsigned short* dbloc16_pBuf1;
Bl16Small dbloc16_small[256];
short dbloc16_offsets[256];

int dbloc16_Register(LecShare* pShare, void* pParam)
{
    dbloc16_pShare = pShare;
    dbloc16_pParam = pParam;
    pShare->Publish("LECMODULE=insane_blocky16_flobdec", &dbloc16_module);
    dbloc16_pShare->Publish("FLOBDEC=insane_blocky16", &dbloc16_interface);
    return 0;
}

int dbloc16_Configure(int tag, ...)
{
    va_list args;

    va_start(args, tag);
    while (tag) {
        switch (tag) {
            case 1:
                dbloc16_pBlit = va_arg(args, Bl16Blit);
                break;
            case 2:
                *va_arg(args, Bl16Blit*) = dbloc16_pBlit;
                break;
            case 3:
                *va_arg(args, LecBitmapHeader**) = dbloc16_pShown;
                break;
            default:
                return -1;
        }
        tag = va_arg(args, int);
    }
    return 0;
}

static void dbloc16_Unregister(void)
{
    dbloc16_pShare->Unpublish("FLOBDEC=insane_blocky16");
    dbloc16_pShare->Unpublish("LECMODULE=insane_blocky16_flobdec");
}

static int dbloc16_Startup(void)
{
    dbloc16_pBitmap = dbloc16_pShare->Subscribe("BITMAP.clear", dbloc16_pParam);
    dbloc16_pMemory = dbloc16_pShare->Subscribe("MEMORY.workspace", dbloc16_pParam);
    if (dbloc16_pBitmap == NULL) {
        dbloc16_pShare->ReportError("FlobDec-Blocky16 Module: Cannot allocate bitmap services.");
        return -1;
    }
    if (dbloc16_pMemory == NULL) {
        dbloc16_pShare->ReportError("FlobDec-Blocky16 Module: Cannot allocate memory services.");
        return -1;
    }
    return 0;
}

static void dbloc16_Shutdown(void)
{
    dbloc16_Reset();
    dbloc16_pShare->Unsubscribe(dbloc16_pBitmap);
    dbloc16_pShare->Unsubscribe(dbloc16_pMemory);
}

static const char* dbloc16_Name(int index)
{
    switch (index) {
    case 0:
        return "Blocky16-Tile";
    case 1:
        return "Decompresses a block16-tile video frame.";
    case 2:
        return "This module decompresses the block16-tile compressed animations used in Outlaws, Monkey Island III, and Shadows of the Empire PC. Images are deltad, so they must be decompressed sequentially from frame 0.";
    default:
        return "";
    }
}

static int dbloc16_GetTag(void)
{
    return 'Bl16';
}

static int dbloc16_GetFlags(void)
{
    return 0;
}

static void dbloc16_Reset(void)
{
    dbloc16_Free();
}

static int dbloc16_Parse(const Bl16Parse* pData, int size)
{
    int width = pData->width;
    int height = (pData->height + 7) & ~7;

    if (pData->depth != dbloc16_depth || dbloc16_pBitmap0 == NULL || width != dbloc16_pBitmap0->width || height != dbloc16_pBitmap0->height) {
        dbloc16_Free();
        dbloc16_Alloc(width, height, pData->depth);
    }
    return 0;
}

static int dbloc16_Alloc(int width, int height, int depth)
{
    int rc;

    dbloc16_size = width * height;
    dbloc16_MakeGlyphs(4);
    dbloc16_MakeGlyphs(8);
    dbloc16_red = 0;
    dbloc16_green = 0;
    dbloc16_blue = 0;
    rc = dbloc16_pBitmap->Create(&dbloc16_pBitmap0, width, height, depth);
    if (rc == 0) {
        rc = dbloc16_pBitmap->Create(&dbloc16_pBitmap1, width, height, depth);
        if (rc == 0) {
            rc = dbloc16_pBitmap->Create(&dbloc16_pBitmap2, width, height, depth);
            if (rc == 0) {
                dbloc16_depth = depth;
                dbloc16_pBuf0 = dbloc16_pBitmap0->pPixels;
                dbloc16_pBuf1 = dbloc16_pBitmap1->pPixels;
                dbloc16_pCur = dbloc16_pBitmap2->pPixels;
                framestream_CopyB(dbloc16_pBuf0, 0, dbloc16_size * 2);
                framestream_CopyB(dbloc16_pBuf1, 0, dbloc16_size * 2);
                framestream_CopyB(dbloc16_pCur, 0, dbloc16_size * 2);
#line 328
                dbloc16_pLut = dbloc16_pMemory->Alloc(0x20000, INSANE_FILE, __LINE__);
                rc = dbloc16_pLut ? 0 : -1;
                if (rc == 0) {
                    return 0;
                }
                dbloc16_pBitmap->Destroy(&dbloc16_pBitmap2);
            }
            dbloc16_pBitmap->Destroy(&dbloc16_pBitmap1);
        }
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap0);
    }
    return rc;
}

static void dbloc16_Free(void)
{
    if (dbloc16_pBitmap0 != NULL) {
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap0);
    }
    if (dbloc16_pBitmap1 != NULL) {
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap1);
    }
    if (dbloc16_pBitmap2 != NULL) {
        dbloc16_pBitmap->Destroy(&dbloc16_pBitmap2);
    }
    if (dbloc16_pLut != NULL) {
#line 352
        dbloc16_pMemory->Free(dbloc16_pLut, INSANE_FILE, __LINE__);
        dbloc16_pLut = NULL;
    }
    dbloc16_pShown = NULL;
    dbloc16_size = 0;
}

static int dbloc16_Decode(void* a1, int a2, int a3, int a4, void* a5, const unsigned char* pData, int size, int mode, int a9, int a10)
{
    return dbloc16_DecodeFrame(a1, (MixerRect*)a2, a3, a4, a5, (Bl16Header*)pData, mode);
}

static int dbloc16_Query1(void)
{
    return -1;
}

static int dbloc16_Query(const Bl16Header* pData, int tag, int* pX, int* pY, int* pWidth, int* pHeight)
{
    if (tag != 0x100) {
        return -1;
    }
    if (pX) {
        *pX = pData->x;
    }
    if (pY) {
        *pY = pData->y;
    }
    if (pWidth) {
        *pWidth = pData->width;
    }
    if (pHeight) {
        *pHeight = pData->height;
    }
    return 0;
}

void dbloc16_MakeTables(int width)
{
    int i;
    int j;

    if (width == dbloc16_lastWidth) {
        return;
    }
    dbloc16_lastWidth = width;
    for (i = 0; i < 255; i++) {
        dbloc16_offsets[i] = dbloc16_motion[i].y * width + dbloc16_motion[i].x;
    }
    for (i = 0; i < 256; i++) {
        for (j = 0; j < dbloc16_small[i].n1; j++) {
            dbloc16_small[i].off1[j] = (dbloc16_small[i].idx1[j] >> 2) * width + (dbloc16_small[i].idx1[j] & 3);
        }
        for (j = 0; j < dbloc16_small[i].n2; j++) {
            dbloc16_small[i].off2[j] = (dbloc16_small[i].idx2[j] >> 2) * width + (dbloc16_small[i].idx2[j] & 3);
        }
        for (j = 0; j < dbloc16_big[i].n1; j++) {
            dbloc16_big[i].off1[j] = (dbloc16_big[i].idx1[j] >> 3) * width + (dbloc16_big[i].idx1[j] & 7);
        }
        for (j = 0; j < dbloc16_big[i].n2; j++) {
            dbloc16_big[i].off2[j] = (dbloc16_big[i].idx2[j] >> 3) * width + (dbloc16_big[i].idx2[j] & 7);
        }
    }
}

static void dbloc16_MakeGlyphs(int size)
{
    int* pEdgeX;
    int* pEdgeY;
    int x;
    int y;
    int i;
    int k;
    int n;
    int edge1;
    int edge2;
    int px;
    int py;
    int x1;
    int y1;
    int x2;
    int y2;
    int entry;
    int glyph[64];

    if (size == 8) {
        pEdgeX = dbloc16_big1;
        pEdgeY = dbloc16_big2;
        for (i = 0; i < 256; i++) {
            dbloc16_big[i].n1 = 0;
        }
        for (i = 0; i < 256; i++) {
            dbloc16_big[i].n2 = 0;
        }
    } else if (size == 4) {
        pEdgeX = dbloc16_small1;
        pEdgeY = dbloc16_small2;
        for (i = 0; i < 256; i++) {
            dbloc16_small[i].n1 = 0;
        }
        for (i = 0; i < 256; i++) {
            dbloc16_small[i].n2 = 0;
        }
    }
    entry = 0;
    for (x = 0; x < 16; x++) {
        for (y = 0; y < 16; y++) {
            y1 = pEdgeY[x];
            if (y1 == 0) {
                edge1 = 0;
            } else if (y1 == size - 1) {
                edge1 = 1;
            } else if (pEdgeX[x] == 0) {
                edge1 = 2;
            } else if (pEdgeX[x] == size - 1) {
                edge1 = 3;
            } else {
                edge1 = 4;
            }
            y2 = pEdgeY[y];
            if (y2 == 0) {
                edge2 = 0;
            } else if (y2 == size - 1) {
                edge2 = 1;
            } else if (pEdgeX[y] == 0) {
                edge2 = 2;
            } else if (pEdgeX[y] == size - 1) {
                edge2 = 3;
            } else {
                edge2 = 4;
            }
            i = size * size;
            while (i-- != 0) {
                glyph[i] = 0;
            }
            n = DBLOC16_MAX(DBLOC16_ABS(pEdgeX[x] - pEdgeX[y]), DBLOC16_ABS(pEdgeY[x] - pEdgeY[y])) + 1;
            x1 = pEdgeX[x];
            x2 = pEdgeX[y];
            for (i = 0; i < n; i++) {
                if (n > 1) {
                    px = (x1 * i + ((n - 1) >> 1) + x2 * (n - 1 - i)) / (n - 1);
                    py = (y1 * i + ((n - 1) >> 1) + y2 * (n - 1 - i)) / (n - 1);
                } else {
                    px = x1;
                    py = y1;
                }
                glyph[py * size + px] = 1;
                if ((edge1 == 2 && edge2 == 3) || (edge2 == 2 && edge1 == 3) || (edge1 == 0 && edge2 != 1) || (edge2 == 0 && edge1 != 1)) {
                    for (k = py; k >= 0; k--) {
                        glyph[k * size + px] = 1;
                    }
                } else if ((edge1 == 1 && edge2 != 0) || (edge2 == 1 && edge1 != 0)) {
                    for (k = py; k < size; k++) {
                        glyph[k * size + px] = 1;
                    }
                } else if ((edge1 == 0 && edge2 == 1) || (edge2 == 0 && edge1 == 1) || (edge1 == 3 && edge2 != 2) || (edge2 == 3 && edge1 != 2)) {
                    for (k = px; k < size; k++) {
                        glyph[py * size + k] = 1;
                    }
                } else if ((edge1 == 2 && edge2 != 3) || (edge2 == 2 && edge1 != 3)) {
                    for (k = px; k >= 0; k--) {
                        glyph[py * size + k] = 1;
                    }
                }
            }
            if (size == 8) {
                i = 64;
                while (i-- != 0) {
                    if (glyph[i] != 0) {
                        dbloc16_big[entry].idx1[dbloc16_big[entry].n1] = i;
                        dbloc16_big[entry].n1++;
                    } else {
                        dbloc16_big[entry].idx2[dbloc16_big[entry].n2] = i;
                        dbloc16_big[entry].n2++;
                    }
                }
            } else if (size == 4) {
                i = 16;
                while (i-- != 0) {
                    if (glyph[i] != 0) {
                        dbloc16_small[entry].idx1[dbloc16_small[entry].n1] = i;
                        dbloc16_small[entry].n1++;
                    } else {
                        dbloc16_small[entry].idx2[dbloc16_small[entry].n2] = i;
                        dbloc16_small[entry].n2++;
                    }
                }
            }
            entry++;
        }
    }
}

static int dbloc16_DecodeFrame(LecBitmapHeader* pBitmap, MixerRect* pClip, int x, int y, void* pPalette, Bl16Header* pHdr, int flags)
{
    int width;
    int height;
    unsigned char* pGfx;
    Bl16DecodeState state; /* release-chosen local: no retail trace; the aggregate gives offset1/offset2 retail's frame slots */
    int nextLine;
    int n;
    unsigned short* pDst;
    unsigned short* pLut;
    unsigned short* pTmp;
    int rc;

    width = pHdr->width;
    height = pHdr->height;
    pGfx = pHdr->data;
    if (dbloc16_pBitmap0 == NULL || width != dbloc16_pBitmap0->width || ((height + 7) & ~7) != dbloc16_pBitmap0->height) {
        dbloc16_Free();
        dbloc16_Alloc(width, (height + 7) & ~7, pHdr->depth ? pHdr->depth : 8);
    }
    state.offset1 = dbloc16_pBuf1 - dbloc16_pCur;
    state.offset2 = dbloc16_pBuf0 - dbloc16_pCur;
    if (pBitmap) {
        dbloc16_BuildLut(pBitmap);
    }
    nextLine = width * 8 - (width & ~7);
    if (pHdr->seq == 0) {
        dbloc16_MakeTables(width);
        decode_Run((unsigned char*)dbloc16_pBuf0, dbloc16_pBitmap0, dbloc16_pBitmap0->width * dbloc16_pBitmap0->height, pHdr->fill);
        decode_Run((unsigned char*)dbloc16_pBuf1, dbloc16_pBitmap0, dbloc16_pBitmap0->width * dbloc16_pBitmap0->height, pHdr->fill);
        dbloc16_prevSeq = -1;
    }
    switch (pHdr->codec) {
        case 0:
            framestream_Copy(dbloc16_pCur, pGfx, width * height * 2);
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 6:
            pDst = dbloc16_pCur;
            n = width * height;
            while (n-- != 0) {
                *pDst++ = pHdr->palette[*pGfx++];
            }
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 1:
            dbloc16_Scale(dbloc16_pCur, (unsigned short*)pGfx, width, height);
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 7:
            dbloc16_ScalePal(dbloc16_pCur, pGfx, width, height, pHdr->palette);
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 8:
            decode_RleInit(pGfx);
            pDst = dbloc16_pCur;
            n = width * height;
            while (n-- != 0) {
                *pDst++ = pHdr->palette[decode_RleByte()];
            }
            decode_RleEnd();
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 5:
            decode_Rle((unsigned char*)dbloc16_pCur, pGfx, pHdr->rleSize);
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 4:
            if ((flags & 4) && !pHdr->rotate) {
                dbloc16_prevSeq = pHdr->seq;
                return 0;
            }
            framestream_Copy(dbloc16_pCur, dbloc16_pBuf0, width * height * 2);
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 3:
            if ((flags & 4) && !pHdr->rotate) {
                dbloc16_prevSeq = pHdr->seq;
                return 0;
            }
            framestream_Copy(dbloc16_pCur, dbloc16_pBuf1, width * height * 2);
            dbloc16_pLast = dbloc16_pCur;
            break;
        case 2:
            if ((flags & 4) && !pHdr->rotate) {
                dbloc16_prevSeq = pHdr->seq;
                return 0;
            }
            if (pHdr->seq == dbloc16_prevSeq + 1) {
                dbloc16_Codec2(dbloc16_pCur, pGfx, state.offset1, state.offset2, width, dbloc16_offsets, pHdr->params, height, nextLine + width,
                               dbloc16_small, dbloc16_big, pHdr->palette);
                dbloc16_pLast = dbloc16_pCur;
            }
            break;
    }
    if (dbloc16_pLast != NULL && pBitmap != NULL) {
        pLut = (dbloc16_red || dbloc16_green) ? dbloc16_pLut : NULL;
        if (dbloc16_pBlit != NULL) {
            if (dbloc16_pLast == dbloc16_pBitmap0->pPixels) {
                dbloc16_pShown = dbloc16_pBitmap0;
                rc = dbloc16_pBlit(pBitmap, pClip, x, y, dbloc16_pBitmap0, pPalette, pLut);
            } else if (dbloc16_pLast == dbloc16_pBitmap1->pPixels) {
                dbloc16_pShown = dbloc16_pBitmap1;
                rc = dbloc16_pBlit(pBitmap, pClip, x, y, dbloc16_pBitmap1, pPalette, pLut);
            } else if (dbloc16_pLast == dbloc16_pBitmap2->pPixels) {
                dbloc16_pShown = dbloc16_pBitmap2;
                rc = dbloc16_pBlit(pBitmap, pClip, x, y, dbloc16_pBitmap2, pPalette, pLut);
            }
        } else {
            if (dbloc16_pLast == dbloc16_pBitmap0->pPixels) {
                dbloc16_pShown = dbloc16_pBitmap0;
                mixer_Blit(pBitmap, pClip, x, y, dbloc16_pBitmap0, pPalette, pLut);
            } else if (dbloc16_pLast == dbloc16_pBitmap1->pPixels) {
                dbloc16_pShown = dbloc16_pBitmap1;
                mixer_Blit(pBitmap, pClip, x, y, dbloc16_pBitmap1, pPalette, pLut);
            } else if (dbloc16_pLast == dbloc16_pBitmap2->pPixels) {
                dbloc16_pShown = dbloc16_pBitmap2;
                mixer_Blit(pBitmap, pClip, x, y, dbloc16_pBitmap2, pPalette, pLut);
            }
            rc = 1;
        }
    }
    if (pHdr->seq == dbloc16_prevSeq + 1) {
        if (pHdr->rotate == 1) {
            pTmp = dbloc16_pCur;
            dbloc16_pCur = dbloc16_pBuf1;
            dbloc16_pBuf1 = pTmp;
        } else if (pHdr->rotate == 2) {
            pTmp = dbloc16_pBuf0;
            dbloc16_pBuf0 = dbloc16_pBuf1;
            dbloc16_pBuf1 = dbloc16_pCur;
            dbloc16_pCur = pTmp;
        }
    }
    dbloc16_prevSeq = pHdr->seq;
    return rc;
}

/* retail compiles this one function without optimisation (EBP frame, every operand reloaded, `0xCC` linker fill after it) */
#pragma optimize("", off)
static void dbloc16_BuildLut(LecBitmapHeader* pBitmap)
{
    int i;

    if (dbloc16_pLut != NULL && pBitmap->pixelSize == 2 && (pBitmap->colorMode == 1 || pBitmap->colorMode == 2)
        && (pBitmap->format.redPosShift != dbloc16_pBitmap0->format.redPosShift || pBitmap->format.bluePosShift != dbloc16_pBitmap0->format.bluePosShift
            || pBitmap->format.greenPosShift != dbloc16_pBitmap0->format.greenPosShift)
        && (pBitmap->format.redPosShift != dbloc16_red || pBitmap->format.bluePosShift != dbloc16_blue || pBitmap->format.greenPosShift != dbloc16_green)) {
        dbloc16_red = pBitmap->format.redPosShift;
        dbloc16_green = pBitmap->format.greenPosShift;
        dbloc16_blue = pBitmap->format.bluePosShift;
        for (i = 0; i < 0x10000; i++) {
            dbloc16_pLut[i] = i;
        }
        decode_Convert((unsigned char*)dbloc16_pLut, dbloc16_pBitmap0, 0x10000, NULL, (unsigned char*)dbloc16_pLut, pBitmap, NULL);
    }
}
#pragma optimize("", on)

static void dbloc16_Scale(unsigned short* pDst, unsigned short* pSrc, int width, int height)
{
    unsigned short* pOut;
    unsigned prev;
    unsigned cur;
    int x;
    int y;

    pOut = pDst + width;
    for (y = 0; y < height; y += 2) {
        cur = *pSrc++;
        *(unsigned*)pOut = (cur << 16) | cur;
        pOut += 2;
        for (x = width - 2; x > 0; x -= 2) {
            prev = cur;
            cur = *pSrc++;
            *(unsigned*)pOut = (cur << 16) | dbloc16_Average(prev, cur);
            pOut += 2;
        }
        pOut += width;
    }
    framestream_Copy(pDst, pDst + width, width * 2);
    pOut = pDst + width * 2;
    for (y = 0; y < height - 2; y += 2) {
        x = width;
        while (x-- != 0) {
            *pOut = dbloc16_Average(pOut[width], pOut[-width]);
            pOut++;
        }
        pOut += width;
    }
}

static unsigned short dbloc16_Average(unsigned short a, unsigned short b)
{
    return ((((b & 0x7E0) + (a & 0x7E0)) & 0xFC0) | (((b & 0xF800) + (a & 0xF800)) & 0x1F000) | ((b & 0x1F) + (a & 0x1F))) >> 1;
}

static void dbloc16_ScalePal(unsigned short* pDst, unsigned char* pSrc, int width, int height, unsigned short* pPalette)
{
    unsigned short* pOut;
    unsigned prev;
    unsigned cur;
    int x;
    int y;

    pOut = pDst + width;
    for (y = 0; y < height; y += 2) {
        cur = pPalette[*pSrc++];
        *(unsigned*)pOut = (cur << 16) | cur;
        pOut += 2;
        for (x = width - 2; x > 0; x -= 2) {
            prev = cur;
            cur = pPalette[*pSrc++];
            *(unsigned*)pOut = (cur << 16) | dbloc16_Average(prev, cur);
            pOut += 2;
        }
        pOut += width;
    }
    framestream_Copy(pDst, pDst + width, width * 2);
    pOut = pDst + width * 2;
    for (y = 0; y < height - 2; y += 2) {
        x = width;
        while (x-- != 0) {
            *pOut = dbloc16_Average(pOut[width], pOut[-width]);
            pOut++;
        }
        pOut += width;
    }
}
