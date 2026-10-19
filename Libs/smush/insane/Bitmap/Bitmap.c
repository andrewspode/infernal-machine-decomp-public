/*
 * insane_bitmap: INSANE's `BITMAP=insane` service (retail 0x004EA0F0-0x004EA7D0): allocates the 0x54-byte bitmap header and its pixel
 * (and palette) storage in one block, for a numbered pixel type or from an explicit format.
 */
#include <stddef.h>
#include <string.h>
#include "../lec.h"
#include "../Memory/Memory.h"
#include "../Util/List.h"
#include "Bitmap.h"

typedef struct BitmapModeService
{
    void* pfn00;
    LecBitmapHeader* (*GetFormat)(void);
} BitmapModeService;

extern void lec_Log(const char* pFormat, ...);

int bitmap_Register(LecShare* pShare, void* pParam);
static void bitmap_Unregister(void);
static int bitmap_Startup(void);
static void bitmap_Shutdown(void);
static int bitmap_Create(LecBitmapHeader** ppBitmap, int width, int height, int type);
static int bitmap_CreateEx(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int a5, int a6, const void* pFormat);
static void bitmap_Destroy(LecBitmapHeader** ppBitmap);

/* an empty statically linked list (retail .data 0x0054D738, head and tail sentinels pointing at each other); no code in
 * this build references it (name inferred) */
static LecList bitmap_list = { { NULL, &bitmap_list.tail }, { &bitmap_list.head, NULL } };
static LecModule bitmap_module = { bitmap_Register, bitmap_Unregister, bitmap_Startup, bitmap_Shutdown };
static BitmapInterface bitmap_interface = { bitmap_Create, bitmap_CreateEx, bitmap_Destroy };

/* written `= 0` in retail's .bss order (VC6 emits zero-initialised statics in declaration order) */
static MemoryDebugInterface* bitmap_pMemory = NULL;
static LecShare* bitmap_pShare = NULL;
static void* bitmap_pParam = NULL;
static BitmapModeService* bitmap_pMode = NULL;

int bitmap_Register(LecShare* pShare, void* pParam)
{
    bitmap_pShare = pShare;
    bitmap_pParam = pParam;
    pShare->Publish("LECMODULE=insane_bitmap", &bitmap_module);
    bitmap_pShare->Publish("BITMAP=insane", &bitmap_interface);
    return 0;
}

static void bitmap_Unregister(void)
{
    bitmap_pShare->Unpublish("BITMAP=insane");
    bitmap_pShare->Unpublish("LECMODULE=insane_bitmap");
}

static int bitmap_Startup(void)
{
    bitmap_pMemory = bitmap_pShare->Subscribe("MEMORY.workspace", bitmap_pParam);
    bitmap_pMode = bitmap_pShare->Subscribe("VIDEO.getbm", bitmap_pParam);
    if (bitmap_pMemory == NULL) {
        bitmap_pShare->ReportError("Video Module: Cannot allocate memory services.");
        return -1;
    }
    return 0;
}

static void bitmap_Shutdown(void)
{
    bitmap_pShare->Unsubscribe(bitmap_pMemory);
    if (bitmap_pMode) {
        bitmap_pShare->Unsubscribe(bitmap_pMode);
    }
}

static int bitmap_Create(LecBitmapHeader** ppBitmap, int width, int height, int type)
{
    int nColors = 0;
    int pixelSize;
    int pitch;
    char* pBlock;
    void* pPixels;

    switch (type) {
    case 1:
        nColors = 0x100;
        /* fall through */
    case 0:
        pixelSize = 1;
        break;
    case 2:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
        pixelSize = 2;
        break;
    case 4:
    case 9:
        pixelSize = 3;
        break;
    case 6:
    case 11:
    case 12:
        pixelSize = 4;
        break;
    case -1:
        if (bitmap_pMode != NULL) {
            pixelSize = bitmap_pMode->GetFormat()->pixelSize;
            break;
        }
#line 249
        lec_Log("%s, line %s:  Type %d not supported", INSANE_FILE, __LINE__, type);
        return -1;
    default:
#line 255
        lec_Log("%s, line %s:  Type %d not supported", INSANE_FILE, __LINE__, type);
        return -1;
    }
    pitch = pixelSize * width;
#line 259
    pBlock = bitmap_pMemory->Alloc(pitch * height + nColors * 4 + 0x5C, INSANE_FILE, __LINE__);
    if (pBlock != NULL) {
        *ppBitmap = (LecBitmapHeader*)(pBlock + 8);
        pPixels = pBlock + 0x5C;
        (*ppBitmap)->pPixels = pPixels;
        (*ppBitmap)->width = width;
        (*ppBitmap)->pitch = pitch;
        (*ppBitmap)->height = height;
        (*ppBitmap)->pixelSize = pixelSize;
        (*ppBitmap)->unknown7 = 0;
        (*ppBitmap)->unknown6 = 0;
        switch (type) {
        case 1:
            (*ppBitmap)->bpp = 8;
            (*ppBitmap)->colorMode = 0;
            (*ppBitmap)->indexed.nColors = nColors;
            (*ppBitmap)->indexed.pPalette = pPixels;
            break;
        case 0:
            (*ppBitmap)->bpp = 8;
            (*ppBitmap)->colorMode = 0;
            (*ppBitmap)->indexed.nColors = nColors;
            (*ppBitmap)->indexed.pPalette = NULL;
            break;
        case 5:
        case 10:
            (*ppBitmap)->bpp = 16;
            (*ppBitmap)->colorMode = 2;
            (*ppBitmap)->format.redBPP = 5;
            (*ppBitmap)->format.greenBPP = 5;
            (*ppBitmap)->format.blueBPP = 5;
            (*ppBitmap)->format.alphaBPP = 1;
            (*ppBitmap)->format.redPosShift = (type == 5) ? 10 : 0;
            (*ppBitmap)->format.greenPosShift = 5;
            (*ppBitmap)->format.bluePosShift = (type != 5) ? 10 : 0;
            (*ppBitmap)->format.alphaPosShift = 15;
            (*ppBitmap)->format.redPosShiftRight = 3;
            (*ppBitmap)->format.greenPosShiftRight = 3;
            (*ppBitmap)->format.bluePosShiftRight = 3;
            (*ppBitmap)->format.alphaPosShiftRight = 7;
            break;
        case 2:
        case 7:
            (*ppBitmap)->bpp = 15;
            (*ppBitmap)->colorMode = 1;
            (*ppBitmap)->format.redBPP = 5;
            (*ppBitmap)->format.greenBPP = 5;
            (*ppBitmap)->format.blueBPP = 5;
            (*ppBitmap)->format.redPosShift = (type == 2) ? 10 : 0;
            (*ppBitmap)->format.greenPosShift = 5;
            (*ppBitmap)->format.bluePosShift = (type != 2) ? 10 : 0;
            (*ppBitmap)->format.redPosShiftRight = 3;
            (*ppBitmap)->format.greenPosShiftRight = 3;
            (*ppBitmap)->format.bluePosShiftRight = 3;
            break;
        case 3:
        case 8:
            (*ppBitmap)->bpp = 16;
            (*ppBitmap)->colorMode = 1;
            (*ppBitmap)->format.redBPP = 5;
            (*ppBitmap)->format.greenBPP = 6;
            (*ppBitmap)->format.blueBPP = 5;
            (*ppBitmap)->format.redPosShift = (type == 3) ? 11 : 0;
            (*ppBitmap)->format.greenPosShift = 5;
            (*ppBitmap)->format.bluePosShift = (type != 3) ? 11 : 0;
            (*ppBitmap)->format.redPosShiftRight = 3;
            (*ppBitmap)->format.greenPosShiftRight = 2;
            (*ppBitmap)->format.bluePosShiftRight = 3;
            break;
        case 6:
        case 11:
        case 12:
            (*ppBitmap)->bpp = 32;
            (*ppBitmap)->colorMode = 2;
            (*ppBitmap)->format.redBPP = 8;
            (*ppBitmap)->format.greenBPP = 8;
            (*ppBitmap)->format.blueBPP = 8;
            (*ppBitmap)->format.alphaBPP = 8;
            (*ppBitmap)->format.redPosShift = (type == 6) ? 16 : ((type != 11) ? 24 : 0);
            (*ppBitmap)->format.greenPosShift = (type != 12) ? 8 : 16;
            (*ppBitmap)->format.bluePosShift = (type != 6) ? 8 : 0;
            (*ppBitmap)->format.alphaPosShift = (type != 12) ? 24 : 0;
            (*ppBitmap)->format.redPosShiftRight = 0;
            (*ppBitmap)->format.greenPosShiftRight = 0;
            (*ppBitmap)->format.bluePosShiftRight = 0;
            (*ppBitmap)->format.alphaPosShiftRight = 0;
            break;
        case 4:
        case 9:
            (*ppBitmap)->bpp = 24;
            (*ppBitmap)->colorMode = 1;
            (*ppBitmap)->format.redBPP = 8;
            (*ppBitmap)->format.greenBPP = 8;
            (*ppBitmap)->format.blueBPP = 8;
            (*ppBitmap)->format.redPosShift = (type == 4) ? 16 : 0;
            (*ppBitmap)->format.greenPosShift = 8;
            (*ppBitmap)->format.bluePosShift = (type != 4) ? 16 : 0;
            (*ppBitmap)->format.redPosShiftRight = 0;
            (*ppBitmap)->format.greenPosShiftRight = 0;
            (*ppBitmap)->format.bluePosShiftRight = 0;
            break;
        case -1:
            (*ppBitmap)->bpp = bitmap_pMode->GetFormat()->bpp;
            (*ppBitmap)->colorMode = bitmap_pMode->GetFormat()->colorMode;
            (*ppBitmap)->indexed = bitmap_pMode->GetFormat()->indexed;
            (*ppBitmap)->format = bitmap_pMode->GetFormat()->format;
            break;
        }
        return 0;
    }
    return -101;
}

static int bitmap_CreateEx(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int a5, int a6, const void* pFormat)
{
    int nColors = 0;
    int pitch;
    char* pBlock;
    void* pPixels;

    if (colorMode == 0) {
        nColors = ((const int*)pFormat)[1];
    }
#line 414
    pBlock = bitmap_pMemory->Alloc(width * height * a5 + nColors * 4 + 0x5C, INSANE_FILE, __LINE__);
    if (pBlock != NULL) {
        *ppBitmap = (LecBitmapHeader*)(pBlock + 8);
        pPixels = pBlock + 0x5C;
        (*ppBitmap)->pPixels = pPixels;
        (*ppBitmap)->width = width;
        pitch = width * a5;
        (*ppBitmap)->pitch = pitch;
        (*ppBitmap)->height = height;
        (*ppBitmap)->pixelSize = a5;
        (*ppBitmap)->bpp = a6;
        (*ppBitmap)->unknown7 = 0;
        (*ppBitmap)->unknown6 = 0;
        (*ppBitmap)->colorMode = colorMode;
        switch (colorMode) {
        case 0:
            (*ppBitmap)->indexed.nColors = nColors;
            (*ppBitmap)->indexed.pPalette = pPixels;
            return 0;
        case 1:
        case 3:
        case 5:
            memcpy(&(*ppBitmap)->format, pFormat, 9 * sizeof(int));
            return 0;
        case 2:
        case 4:
        case 6:
            memcpy(&(*ppBitmap)->format, pFormat, sizeof(BitmapFormat));
            return 0;
        case 7:
            memcpy(&(*ppBitmap)->format, pFormat, 3 * sizeof(int));
            return 0;

        }
        return 0;
    }
    return -101;
}

static void bitmap_Destroy(LecBitmapHeader** ppBitmap)
{
    char* pBlock = (char*)*ppBitmap - 8;

    if (pBlock) {
#line 516
        bitmap_pMemory->Free(pBlock, INSANE_FILE, __LINE__);
    }
    *ppBitmap = NULL;
}
