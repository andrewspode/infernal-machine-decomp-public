/*
 * insane_decode: INSANE's pixel-format conversion and run-length helpers (retail 0x004EE6A0-0x004EE700 and 0x004EE710-0x004EF890;
 * the empty function between them was folded onto rdCanvas_FreeEntry by the linker). It has no LEC registration: blocky16, the
 * mixer object's clipped blit and Bitmap/Blit.c call it directly. Pixels are converted through a 32-bit R, G, B, A byte form, in
 * chunks of 0x500 through two buffers, or by INSANE's hand-written assembly converters (DecodeAsm.asm, retail 0x004F0590)
 * for the common 15/16-bit and palette cases. Retail has no names for any of this.
 *
 */
#include <stddef.h>
#include "../File/Framestream.h"
#include "Decode.h"

/* INSANE's assembly converters (MASM, 0x004F0590-0x004F09C0) */
void decode_Lut8(void* pDst, const void* pSrc, int n, int dstSize, const void* pTable);
void decode_Lut16(void* pDst, const void* pSrc, int n, int dstSize, const void* pTable);
void decode_RGBAto555(void* pDst, const void* pSrc, int n);
void decode_555toRGBA(void* pDst, const void* pSrc, int n);
void decode_RGBAto565(void* pDst, const void* pSrc, int n);
void decode_565toRGBA(void* pDst, const void* pSrc, int n);
void decode_RGBAto565B(void* pDst, const void* pSrc, int n);
void decode_565BtoRGBA(void* pDst, const void* pSrc, int n);


static int decode_masks[9] = { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };

unsigned char decode_buffer[2][0x1400];

static int decode_length;
static int decode_state;
static unsigned char decode_value;
static unsigned char* decode_pSrc;
static int decode_flip;

void decode_Rle(unsigned char* pDst, unsigned char* pSrc, int n)
{
    decode_RleInit(pSrc);
    while (n--) {
        *pDst++ = decode_RleByte();
    }
    decode_RleEnd();
}

void decode_RleInit(unsigned char* pSrc)
{
    decode_state = 2;
    decode_pSrc = pSrc;
}

void decode_RleEnd(void)
{
}

unsigned char decode_RleByte(void)
{
    unsigned char c;

    if (decode_state == 2) {
        decode_length = (*decode_pSrc >> 1) + 1;
        if (*decode_pSrc++ & 1) {
            decode_state = 1;
            decode_value = *decode_pSrc++;
        }
        else {
            decode_state = 0;
        }
    }
    switch (decode_state) {
    case 1:
        c = decode_value;
        break;
    case 0:
        c = *decode_pSrc++;
        break;
    }
    if (--decode_length == 0) {
        decode_state = 2;
    }
    return c;
}

void decode_Run(unsigned char* pDst, LecBitmapHeader* pBitmap, int n, unsigned char* pPixel)
{
    switch (pBitmap->pixelSize) {
    case 1:
        framestream_CopyB(pDst, *pPixel, n);
        break;
    case 2:
        if (pPixel[0] == pPixel[1]) {
            framestream_CopyB(pDst, pPixel[0], n * 2);
        }
        else {
            while (n--) {
                *(unsigned short*)pDst = *(unsigned short*)pPixel;
                pDst += 2;
            }
        }
        break;
    case 3:
        if (pPixel[0] == pPixel[1] && pPixel[2] == pPixel[1]) {
            framestream_CopyB(pDst, pPixel[0], n * 3);
        }
        else {
            while (n--) {
                *pDst++ = pPixel[0];
                *pDst++ = pPixel[1];
                *pDst++ = pPixel[2];
            }
        }
        break;
    case 4:
        if (pPixel[0] == pPixel[1] && pPixel[2] == pPixel[1] && pPixel[3] == pPixel[1]) {
            framestream_CopyB(pDst, pPixel[0], n * 4);
        }
        else {
            while (n--) {
                *(unsigned long*)pDst = *(unsigned long*)pPixel;
                pDst += 4;
            }
        }
        break;
    }
}

int decode_MakeTable(LecBitmapHeader* pSrcBitmap, LecBitmapHeader* pDstBitmap, void* pTable, void* pPalette)
{
    if (pSrcBitmap->colorMode == 0 && pSrcBitmap->pixelSize == 1 && pDstBitmap->colorMode != 0) {
        if (pSrcBitmap->indexed.pPalette) {
            pPalette = pSrcBitmap->indexed.pPalette;
        }
        if (pPalette) {
            decode_Begin(pTable, pDstBitmap, 256, 0, pPalette);
            return 0;
        }
    }
    return -1;
}

int decode_Convert(unsigned char* pSrc, LecBitmapHeader* pSrcBitmap, int n, void* pPalette, unsigned char* pDst, LecBitmapHeader* pDstBitmap, void* pTable)
{
    int srcRGBA;
    int dstRGBA;
    int chunk;
    int buf;

    if (pSrcBitmap->bpp == pDstBitmap->bpp && pSrcBitmap->colorMode == pDstBitmap->colorMode
        && ((pSrcBitmap->colorMode != 1 && pSrcBitmap->colorMode != 2)
            || (pSrcBitmap->format.redPosShift == pDstBitmap->format.redPosShift
                && pSrcBitmap->format.greenPosShift == pDstBitmap->format.greenPosShift
                && pSrcBitmap->format.bluePosShift == pDstBitmap->format.bluePosShift))) {
        framestream_Copy(pDst, pSrc, n * pDstBitmap->pixelSize);
        return 0;
    }
    if (pTable) {
        if (pSrcBitmap->colorMode == 0 && pSrcBitmap->pixelSize == 1) {
            decode_Lut8(pDst, pSrc, n, pDstBitmap->pixelSize, pTable);
            return 0;
        }
        if (pSrcBitmap->pixelSize == 2 && pDstBitmap->colorMode != 0) {
            decode_Lut16(pDst, pSrc, n, pDstBitmap->pixelSize, pTable);
            return 0;
        }
    }
    srcRGBA = pSrcBitmap->colorMode == 2 && pSrcBitmap->pixelSize == 4 && pSrcBitmap->format.redPosShift == 0
        && pSrcBitmap->format.greenPosShift == 8 && pSrcBitmap->format.bluePosShift == 16;
    dstRGBA = pDstBitmap->colorMode == 2 && pDstBitmap->pixelSize == 4 && pDstBitmap->format.redPosShift == 0
        && pDstBitmap->format.greenPosShift == 8 && pDstBitmap->format.bluePosShift == 16;
    for (chunk = n < 0x500 ? n : 0x500; chunk > 0; chunk = n < 0x500 ? n : 0x500) {
        if (srcRGBA) {
            if (decode_Begin(pDst, pDstBitmap, chunk, pPalette, pSrc)) {
                return -1;
            }
        }
        else if (dstRGBA) {
            if (decode_ToRGBA(pSrc, pSrcBitmap, chunk, pPalette, pDst)) {
                return -1;
            }
        }
        else {
            buf = ++decode_flip & 1;
            if (decode_ToRGBA(pSrc, pSrcBitmap, chunk, pPalette, decode_buffer[buf])) {
                return -1;
            }
            if (decode_Begin(pDst, pDstBitmap, chunk, pPalette, decode_buffer[buf])) {
                return -1;
            }
        }
        pSrc += pSrcBitmap->pixelSize * chunk;
        n -= chunk;
        pDst += pDstBitmap->pixelSize * chunk;
    }
    return 0;
}

int decode_ToRGBA(unsigned char* pSrc, LecBitmapHeader* pBitmap, int n, void* pPalette, unsigned char* pDst)
{
    unsigned long pix;
    unsigned char* pIn;
    unsigned char* pOut;

    if (pBitmap->colorMode == 1 && pBitmap->pixelSize == 2) {
        if (pBitmap->format.redPosShift == 10 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0) {
            decode_555toRGBA(pDst, pSrc, n);
            return 0;
        }
        if (pBitmap->format.redPosShift == 0 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 11) {
            decode_565BtoRGBA(pDst, pSrc, n);
            return 0;
        }
        if (pBitmap->format.redPosShift == 11 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0) {
            decode_565toRGBA(pDst, pSrc, n);
            return 0;
        }
    }
    if (pBitmap->colorMode == 0) {
        if (pBitmap->indexed.pPalette) {
            pPalette = pBitmap->indexed.pPalette;
        }
        if (!pPalette) {
            return -1;
        }
        decode_Lut8(pDst, pSrc, n, 4, pPalette);
        return 0;
    }
    if (pBitmap->colorMode == 1) {
        unsigned char rMask;
        unsigned char gMask;
        unsigned char bMask;
        char rShift;
        char gShift;
        char bShift;
        char rShiftR;
        char gShiftR;
        char bShiftR;

        pOut = pDst;
        rMask = (unsigned char)decode_masks[pBitmap->format.redBPP];
        gMask = (unsigned char)decode_masks[pBitmap->format.greenBPP];
        bMask = (unsigned char)decode_masks[pBitmap->format.blueBPP];
        rShift = (char)pBitmap->format.redPosShift;
        gShift = (char)pBitmap->format.greenPosShift;
        bShift = (char)pBitmap->format.bluePosShift;
        rShiftR = (char)pBitmap->format.redPosShiftRight;
        gShiftR = (char)pBitmap->format.greenPosShiftRight;
        bShiftR = (char)pBitmap->format.bluePosShiftRight;
        if (pBitmap->pixelSize == 2) {
            for (pIn = pSrc; n != 0; n--) {
                pix = *(unsigned short*)pIn;
                pIn += 2;
                *pOut++ = (unsigned char)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (unsigned char)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (unsigned char)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = 0xFF;
            }
            return 0;
        }
        if (pBitmap->pixelSize == 3) {
            for (pIn = pSrc; n != 0; n--) {
                pix = *(unsigned long*)pIn & 0xFFFFFF;
                pIn += 3;
                *pOut++ = (unsigned char)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (unsigned char)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (unsigned char)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = 0xFF;
            }
            return 0;
        }
        if (pBitmap->pixelSize == 4) {
            for (pIn = pSrc; n != 0; n--) {
                pix = *(unsigned long*)pIn;
                pIn += 4;
                *pOut++ = (unsigned char)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (unsigned char)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (unsigned char)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = 0xFF;
            }
            return 0;
        }
        return -1;
    }
    if (pBitmap->colorMode == 2) {
        unsigned char rMask;
        unsigned char gMask;
        unsigned char bMask;
        unsigned char aMask;
        char rShift;
        char gShift;
        char bShift;
        char aShift;
        char rShiftR;
        char gShiftR;
        char bShiftR;
        char aShiftR;

        pOut = pDst;
        rMask = (unsigned char)decode_masks[pBitmap->format.redBPP];
        gMask = (unsigned char)decode_masks[pBitmap->format.greenBPP];
        bMask = (unsigned char)decode_masks[pBitmap->format.blueBPP];
        aMask = (unsigned char)decode_masks[pBitmap->format.alphaBPP];
        rShift = (char)pBitmap->format.redPosShift;
        gShift = (char)pBitmap->format.greenPosShift;
        bShift = (char)pBitmap->format.bluePosShift;
        aShift = (char)pBitmap->format.alphaPosShift;
        rShiftR = (char)pBitmap->format.redPosShiftRight;
        gShiftR = (char)pBitmap->format.greenPosShiftRight;
        bShiftR = (char)pBitmap->format.bluePosShiftRight;
        aShiftR = (char)pBitmap->format.alphaPosShiftRight;
        if (pBitmap->pixelSize == 2) {
            for (pIn = pSrc; n != 0; n--) {
                pix = *(unsigned short*)pIn;
                pIn += 2;
                *pOut++ = (unsigned char)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (unsigned char)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (unsigned char)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = (unsigned char)((pix >> aShift) << aShiftR) & aMask;
            }
            return 0;
        }
        if (pBitmap->pixelSize == 3) {
            for (pIn = pSrc; n != 0; n--) {
                pix = *(unsigned long*)pIn & 0xFFFFFF;
                pIn += 3;
                *pOut++ = (unsigned char)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (unsigned char)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (unsigned char)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = (unsigned char)((pix >> aShift) << aShiftR) & aMask;
            }
            return 0;
        }
        if (pBitmap->pixelSize == 4) {
            for (pIn = pSrc; n != 0; n--) {
                pix = *(unsigned long*)pIn;
                pIn += 4;
                *pOut++ = (unsigned char)((pix >> rShift) << rShiftR) & rMask;
                *pOut++ = (unsigned char)((pix >> gShift) << gShiftR) & gMask;
                *pOut++ = (unsigned char)((pix >> bShift) << bShiftR) & bMask;
                *pOut++ = (unsigned char)((pix >> aShift) << aShiftR) & aMask;
            }
            return 0;
        }
        return -1;
    }
    return -1;
}

int decode_Begin(unsigned char* pDst, LecBitmapHeader* pBitmap, int n, void* pPalette, unsigned char* pSrc)
{
    BitmapFormat* pFormat = &pBitmap->format; /* release-chosen local: no retail trace; it gives the mode-1 cases their term order */

    if (pBitmap->colorMode == 1 && pBitmap->pixelSize == 2) {
        if (pBitmap->format.redPosShift == 10 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0) {
            decode_RGBAto555(pDst, pSrc, n);
            return 0;
        }
        if (pBitmap->format.redPosShift == 0 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 11) {
            decode_RGBAto565B(pDst, pSrc, n);
            return 0;
        }
        if (pBitmap->format.redPosShift == 11 && pBitmap->format.greenPosShift == 5 && pBitmap->format.bluePosShift == 0) {
            decode_RGBAto565(pDst, pSrc, n);
            return 0;
        }
    }
    if (pBitmap->colorMode == 1) {
        unsigned char* pIn;
        unsigned char* pOut;
        unsigned long pix;
        unsigned char rMask;
        unsigned char gMask;
        unsigned char bMask;
        char rShift;
        char gShift;
        char bShift;
        char rShiftR;
        char gShiftR;
        char bShiftR;

        rMask = (unsigned char)decode_masks[pFormat->redBPP];
        gMask = (unsigned char)decode_masks[pFormat->greenBPP];
        bMask = (unsigned char)decode_masks[pFormat->blueBPP];
        rShift = (char)pFormat->redPosShift;
        gShift = (char)pFormat->greenPosShift;
        bShift = (char)pFormat->bluePosShift;
        rShiftR = (char)pFormat->redPosShiftRight;
        gShiftR = (char)pFormat->greenPosShiftRight;
        bShiftR = (char)pFormat->bluePosShiftRight;
        if (pBitmap->pixelSize == 1) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                *pOut++ = (((pIn[2] & bMask) << bShift) >> bShiftR) | (((pIn[0] & rMask) << rShift) >> rShiftR) | (((pIn[1] & gMask) << gShift) >> gShiftR);
                pIn += 4;
            }
        }
        else if (pBitmap->pixelSize == 2) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                *(unsigned short*)pOut = (((pIn[2] & bMask) << bShift) >> bShiftR) | (((pIn[0] & rMask) << rShift) >> rShiftR) | (((pIn[1] & gMask) << gShift) >> gShiftR);
                pOut += 2;
                pIn += 4;
            }
        }
        else if (pBitmap->pixelSize == 3) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                pix = (((unsigned long)(pIn[0] & rMask) << rShift) >> rShiftR) | (((unsigned long)(pIn[2] & bMask) << bShift) >> bShiftR) | (((unsigned long)(pIn[1] & gMask) << gShift) >> gShiftR);
                *pOut++ = ((unsigned char*)&pix)[0];
                *pOut++ = ((unsigned char*)&pix)[1];
                *pOut++ = ((unsigned char*)&pix)[2];
                pIn += 4;
            }
        }
        else if (pBitmap->pixelSize == 4) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                *(unsigned long*)pOut = (((unsigned long)(pIn[0] & rMask) << rShift) >> rShiftR) | (((unsigned long)(pIn[2] & bMask) << bShift) >> bShiftR) | (((unsigned long)(pIn[1] & gMask) << gShift) >> gShiftR);
                pOut += 4;
                pIn += 4;
            }
        }
        else {
            return -1;
        }
        return 0;
    }
    if (pBitmap->colorMode == 2) {
        unsigned char* pIn;
        unsigned char* pOut;
        unsigned long pix;
        unsigned char rMask;
        unsigned char gMask;
        unsigned char bMask;
        unsigned char aMask;
        char rShift;
        char gShift;
        char bShift;
        char aShift;
        char rShiftR;
        char gShiftR;
        char bShiftR;
        char aShiftR;

        rMask = (unsigned char)decode_masks[pFormat->redBPP];
        gMask = (unsigned char)decode_masks[pFormat->greenBPP];
        bMask = (unsigned char)decode_masks[pFormat->blueBPP];
        aMask = (unsigned char)decode_masks[pFormat->alphaBPP];
        rShift = (char)pFormat->redPosShift;
        gShift = (char)pFormat->greenPosShift;
        bShift = (char)pFormat->bluePosShift;
        aShift = (char)pFormat->alphaPosShift;
        rShiftR = (char)pFormat->redPosShiftRight;
        gShiftR = (char)pFormat->greenPosShiftRight;
        bShiftR = (char)pFormat->bluePosShiftRight;
        aShiftR = (char)pFormat->alphaPosShiftRight;
        if (pBitmap->pixelSize == 2) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                *(unsigned short*)pOut = (((pIn[2] & bMask) << bShift) >> bShiftR) | (((pIn[0] & rMask) << rShift) >> rShiftR) | (((pIn[1] & gMask) << gShift) >> gShiftR) | (((pIn[3] & aMask) << aShift) >> aShiftR);
                pOut += 2;
                pIn += 4;
            }
        }
        else if (pBitmap->pixelSize == 3) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                pix = (((unsigned long)(pIn[0] & rMask) << rShift) >> rShiftR) | (((unsigned long)(pIn[2] & bMask) << bShift) >> bShiftR) | (((unsigned long)(pIn[1] & gMask) << gShift) >> gShiftR) | (((unsigned long)(pIn[3] & aMask) << aShift) >> aShiftR);
                *pOut++ = ((unsigned char*)&pix)[0];
                *pOut++ = ((unsigned char*)&pix)[1];
                *pOut++ = ((unsigned char*)&pix)[2];
                pIn += 4;
            }
        }
        else if (pBitmap->pixelSize == 4) {
            pOut = pDst;
            for (pIn = pSrc; n != 0; n--) {
                *(unsigned long*)pOut = (((unsigned long)(pIn[0] & rMask) << rShift) >> rShiftR) | (((unsigned long)(pIn[2] & bMask) << bShift) >> bShiftR) | (((unsigned long)(pIn[1] & gMask) << gShift) >> gShiftR) | (((unsigned long)(pIn[3] & aMask) << aShift) >> aShiftR);
                pOut += 4;
                pIn += 4;
            }
        }
        else {
            return -1;
        }
        return 0;
    }
    return -1;
}
