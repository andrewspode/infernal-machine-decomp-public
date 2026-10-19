#ifndef SMUSH_INSANE_TIMER_H
#define SMUSH_INSANE_TIMER_H

/*
 * The `TIMER=insane` service Timer.c publishes (`timer_interface`), subscribed by the animation player (Pcplay.c), the DirectSound
 * driver (Dsnd.c) and the sound engine (Engine.c, as `TIMER.external_sync`).
 */
typedef void (*TimerCallback)(int index, int param);

typedef struct TimerInterface
{
    int  (*Alloc)(TimerCallback pfn, int interval, unsigned flags, int param);
    void (*Free)(int index);
    void (*Pause)(void);
    void (*Resume)(void);
    void (*SetInterval)(int index, int interval);
    void (*Wait)(int ticks);
    int  (*GetTicks)(void);
    int  (*GetTickLen)(void);
    void (*Sync)(int ticks);
} TimerInterface;

#endif
