#ifndef SMUSH_INSANE_BLIT_H
#define SMUSH_INSANE_BLIT_H

/*
 * Blit.c's C wrappers around the MASM blitter: blit_Fill clears a bitmap to one colour (the frame stream's clear), blit_Wrapper
 * reorders a clipped copy's arguments for BlitAsm.asm (Mixer.c's blit).
 */
#include "Bitmap.h"

void blit_Fill(LecBitmapHeader* pBitmap, int color);
void blit_Wrapper(void* pDst, void* pSrc, int a3, int a4, int a5, int a6, int width, int height, int dstPitch, int srcPitch);

#endif
