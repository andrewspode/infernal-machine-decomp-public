#ifndef SMUSH_INSANE_BITMAP_H
#define SMUSH_INSANE_BITMAP_H

/*
 * INSANE's bitmap header and the `BITMAP=insane` service Bitmap.c publishes (`bitmap_interface`): Bitmap.c allocates the 0x54-byte
 * header with its pixels, the decoders (Decode.c, D_bloc16.c), the blitters (Blit.c, Mixer.c) and the player read it.
 */
typedef struct BitmapFormat
{
    int redBPP;
    int greenBPP;
    int blueBPP;
    int redPosShift;
    int greenPosShift;
    int bluePosShift;
    int redPosShiftRight;
    int greenPosShiftRight;
    int bluePosShiftRight;
    int alphaBPP;
    int alphaPosShift;
    int alphaPosShiftRight;
} BitmapFormat;

typedef struct LecBitmapHeader
{
    void* pPixels;          /* 0x00 */
    int width;              /* 0x04 */
    int height;             /* 0x08 */
    int pixelSize;          /* 0x0C */
    int bpp;                /* 0x10 */
    int pitch;              /* 0x14 */
    int unknown6;           /* 0x18 */
    int unknown7;           /* 0x1C */
    int colorMode;          /* 0x20 */
    union {
        BitmapFormat format;    /* 0x24 */
        struct {
            void* pPalette;
            int nColors;
        } indexed;
    };
} LecBitmapHeader;

typedef struct BitmapInterface
{
    int  (*Create)(LecBitmapHeader** ppBitmap, int width, int height, int type);
    int  (*CreateEx)(LecBitmapHeader** ppBitmap, int width, int height, int colorMode, int a5, int a6, const void* pFormat);
    void (*Destroy)(LecBitmapHeader** ppBitmap);
} BitmapInterface;

#endif
