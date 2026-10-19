#ifndef SMUSH_INSANE_DECODE_H
#define SMUSH_INSANE_DECODE_H

/*
 * INSANE's pixel-format conversion and fill (Decode.c), used by the blitters (Blit.c, Mixer.c) and the blocky16 decoder
 * (D_bloc16.c). decode_Begin converts n pixels from 32-bit RGBA to the bitmap's format; decode_Run fills n pixels of the
 * bitmap's format with one converted pixel.
 */
#include "../Bitmap/Bitmap.h"

void decode_Rle(unsigned char* pDst, unsigned char* pSrc, int n);
void decode_RleInit(unsigned char* pSrc);
void decode_RleEnd(void);
unsigned char decode_RleByte(void);
void decode_Run(unsigned char* pDst, LecBitmapHeader* pBitmap, int n, unsigned char* pPixel);
int decode_MakeTable(LecBitmapHeader* pSrcBitmap, LecBitmapHeader* pDstBitmap, void* pTable, void* pPalette);
int decode_Convert(unsigned char* pSrc, LecBitmapHeader* pSrcBitmap, int n, void* pPalette, unsigned char* pDst, LecBitmapHeader* pDstBitmap, void* pTable);
int decode_ToRGBA(unsigned char* pSrc, LecBitmapHeader* pBitmap, int n, void* pPalette, unsigned char* pDst);
int decode_Begin(unsigned char* pDst, LecBitmapHeader* pBitmap, int n, void* pPalette, unsigned char* pSrc);

#endif
