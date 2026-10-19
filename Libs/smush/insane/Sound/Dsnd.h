#ifndef SMUSH_INSANE_DSND_H
#define SMUSH_INSANE_DSND_H

/*
 * The `SNDDRIVER=insane_dsnd` interface Dsnd.c publishes (`dsnd_interface`) and the sound engine (Engine.c) subscribes to. The
 * driver's timer callback calls the engine's fill function once per block with the mixer's per-track feed (`mixer_Feed`).
 */
typedef void (*SoundFeed)(void* pSrc, int offset, int n, int bytes, int a5, int volume, int pan, int format);

typedef struct SndDriver
{
    int  (*Start)(int rate, int tracks, void (*pfnFill)(int n, SoundFeed pfnFeed));
    void (*Stop)(void);
    void (*SetVolume)(int volume);
    int  (*GetVolume)(void);
    int  (*SetRate)(int rate);  /* Dsnd's table holds sound_Null, an empty `return 0` */
    int  (*GetRate)(void);
} SndDriver;

#endif
