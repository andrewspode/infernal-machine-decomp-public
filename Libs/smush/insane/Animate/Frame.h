#ifndef SMUSH_INSANE_FRAME_H
#define SMUSH_INSANE_FRAME_H

/*
 * The `ANIMFRAME=insane` service Frame.c publishes (`animframe_interface`): the frame stream (Framestream.c) parses, decodes and
 * fetches frames through it, the animation player (Pcplay.c) resets and draws.
 */
typedef struct AnimFrameTable
{
    void (*Reset)(void);
    int  (*Parse)(const unsigned char* pData, int size);
    unsigned (*Decode)(void* pBitmap, int a2, int a3, int a4, void* pPalette, int* pOutA, int* pOutB, unsigned flags, int a9, int a10, const unsigned char* pData, int size);
    void (*Draw)(void* pBitmap, int a2, int a3, int a4, void* pPalette, int a6, int a7, int a8);
    void (*Load)(const short* pData, int size);
    void* (*Fetch)(int n);
} AnimFrameTable;

#endif
