/*
 * insane_blit: C wrappers around INSANE's hand-written assembly blitters (retail 0x004F0460-0x004F04D0 and
 * 0x004F0550-0x004F0590; the assembly is MASM: MemCopy.asm and MemFill.asm at 0x004F04D0-0x004F0550, BlitAsm.asm
 * at 0x004F2220).
 *
 */
#include <stddef.h>
#include "../lec.h"
#include "../File/Framestream.h"
#include "../Decode/Decode.h"
#include "Blit.h"

void blit_Masm(void* pSrc, void* pDst, int a5, int a6, int a3, int a4, int width, int height, int srcPitch, int dstPitch);

/* Clears the bitmap to `color`: a byte fill for colour mode 0, else the colour (32-bit RGBA) converted to the bitmap's pixel
   format once and repeated over every pixel. */
void blit_Fill(LecBitmapHeader* pBitmap, int color)
{
    unsigned char pixel[20];

    if (pBitmap->pitch > 0) {
        if (pBitmap->colorMode == 0) {
            framestream_CopyB(pBitmap->pPixels, color, pBitmap->pixelSize * pBitmap->height * pBitmap->pitch);
        }
        else {
            void* pPixels = pBitmap->pPixels;
            int size = pBitmap->height * pBitmap->width;

            decode_Begin(pixel, pBitmap, 1, 0, (unsigned char*)&color);
            decode_Run(pPixels, pBitmap, size, pixel);
        }
    }
}

void blit_Wrapper(void* pDst, void* pSrc, int a3, int a4, int a5, int a6, int width, int height, int dstPitch, int srcPitch)
{
    blit_Masm(pSrc, pDst, a5, a6, a3, a4, width, height, srcPitch, dstPitch);
}
