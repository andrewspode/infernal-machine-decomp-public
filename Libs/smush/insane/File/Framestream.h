#ifndef SMUSH_INSANE_FRAMESTREAM_H
#define SMUSH_INSANE_FRAMESTREAM_H

/*
 * The chunked frame stream (Framestream.c) the animation player (Pcplay.c) drives, and its two thin wrappers around the MASM
 * block copy and byte fill (MemCopy.asm, MemFill.asm) that the other modules call.
 */
typedef struct AnimPlay FrameStream;

int framestream_Reset(FrameStream* pFrames);
void framestream_Close(FrameStream* pFrames);
int framestream_Tick(FrameStream* pFrames, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10, int a11);
int framestream_IsOpen(FrameStream* pFrames);
void framestream_Stall(FrameStream* pFrames);
int framestream_Open(FrameStream* pFrames, const char* pName, int flags, int seekLen, int seekTo, int hold);
void framestream_Seek(FrameStream* pFrames, int length, int to);
int framestream_Mode(FrameStream* pFrames, int op);
void framestream_Copy(void* pDst, const void* pSrc, int size);
void framestream_CopyB(void* pDst, unsigned char value, int size);

#endif
