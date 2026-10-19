#ifndef SMUSH_INSANE_ENGINE_H
#define SMUSH_INSANE_ENGINE_H

/*
 * The `SOUND=insane` service Engine.c publishes (`sound_interface`): the wave decoder (D_wave.c) plays its packets through it, the
 * animation player (Pcplay.c, as `SOUND.lipsync`) pauses and resumes it, SmushPlay sets the volume.
 */
typedef struct SoundInterface
{
    void (*Play)(unsigned char* pData, int id, int volume, int pan, int priority);
    void (*Stop)(int id);
    void (*Pause)(void);
    void (*Resume)(void);
    void (*StopAll)(void);
    int  (*GetVolume)(int id);
    void (*SetVolume)(int id, int volume);
    int  (*GetPan)(int id);
    void (*SetPan)(int id, int pan);
    int  (*GetMasterVolume)(void);
    void (*SetMasterVolume)(int volume);
    int  (*GetRate)(void);
    void (*SetRate)(int rate);
    int  (*GetPosition)(int id);
    int  (*Query)(int a1, int tag, int* pA, int* pB);
    int  (*Control)(int a1, int tag, int a3, int a4);
} SoundInterface;

#endif
