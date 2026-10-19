#ifndef SMUSH_INSANE_MIXER_H
#define SMUSH_INSANE_MIXER_H

/*
 * INSANE's software mixer (Mixer.c): the DirectSound driver (Dsnd.c) clears and clips each block through these and hands
 * mixer_Feed (a SoundFeed, Dsnd.h) to the sound engine's fill callback (Engine.c) as the per-track feed. mixer_Blit is the
 * clipped bitmap blit the blocky16 decoder (D_bloc16.c) uses.
 */
#include "../Bitmap/Bitmap.h"

struct MemoryDebugInterface;

typedef struct MixerRect
{
    int x;
    int y;
    int width;
    int height;
} MixerRect;

int mixer_Init(struct MemoryDebugInterface* pMemory, int headroom, int size, int a4, int a5, int a6);
void mixer_Done(struct MemoryDebugInterface* pMemory);
int mixer_Begin(int n);
void mixer_Feed(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format);
int mixer_Mix(short* pDst, int n);
void mixer_Blit(LecBitmapHeader* pDst, MixerRect* pClip, int x, int y, LecBitmapHeader* pSrc, void* pPalette, void* pTable);

#endif
