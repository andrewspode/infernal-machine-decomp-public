/*
 * insane_animplay: INSANE's `ANIMPLAY=insane` service and its `DEBUG=insane_animplay` interface (retail 0x004E3810-0x004E4230): plays a
 * frame stream (File/Framestream.c) on two timers, a tick counter and a frame pacer.
 */
#include <stddef.h>
#include "../lec.h"
#include "../Memory/Memory.h"
#include "../animplay.h"
#include "../Timer/Timer.h"
#include "../File/Framestream.h"
#include "../File/Cdstream.h"
#include "Frame.h"
#include "../Sound/Engine.h"
#include "../Bitmap/Bitmap.h"

/* the services this module subscribes to (slot roles inferred from the calls) */
typedef struct DebugService
{
    void (*Log)(int a, int b);
    void* pfn04;
    void* pfn08;
    void* pfn0C;
    void* pfn10;
    void (*Show)(int a, int b, int c);
    void* pfn18;
    int  (*Query)(int a, int b);
} DebugService;

int animplay_Register(LecShare* pShare, void* pParam);
static void animplay_Unregister(void);
static int animplay_Startup(void);
static void animplay_Shutdown(void);
static int animplay_Play(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13);
static int animplay_Create(AnimPlay** ppPlay, const char* pName, int rate, int ringSize, int frameSize, int flags);
static int animplay_GetTime(AnimPlay* pPlay);
static int animplay_Pace(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10);
static void animplay_Draw(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5, void* pPalette, int a7, int a8, int a9);
static void animplay_Destroy(AnimPlay** ppPlay);
static void animplay_SetRate(AnimPlay* pPlay, int rate);
static void animplay_Rewind(AnimPlay* pPlay);
static int animplay_IsDone(AnimPlay* pPlay);
static int animplay_Step(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10);
static void animplay_TickCallback(int index, int param);
static void animplay_PaceCallback(int index, int param);
static const char* animplay_DebugName(int index);
static void animplay_SetDebug(int flags);
static int animplay_GetDebug(void);

typedef struct DebugInterface
{
    const char* (*Name)(int index);
    void (*SetFlags)(int flags);
    int  (*GetFlags)(void);
    int  speed;
} DebugInterface;

static LecModule animplay_module = { animplay_Register, animplay_Unregister, animplay_Startup, animplay_Shutdown };
static AnimPlayTable animplay_interface = {
    animplay_Play, animplay_Create, animplay_GetTime, animplay_Pace, animplay_Draw, animplay_Destroy,
    framestream_IsOpen, framestream_Stall, framestream_Open, framestream_Mode
};
AnimPlayOps animplay_ops = { animplay_SetRate, animplay_Rewind, animplay_IsDone, NULL };
static DebugInterface animplay_debug = { animplay_DebugName, animplay_SetDebug, animplay_GetDebug, 8 };

LecShare* animplay_pShare;
MemoryDebugInterface* animplay_pMemory;
DebugService* animplay_pDebug;
StreamerInterface* animplay_pStreamer;
AnimFrameTable* animplay_pFrame;

static void* animplay_pCookie;
static BitmapInterface* animplay_pBitmap;
static int animplay_bitmapSize;
static int animplay_rate;
static TimerInterface* animplay_timer;
static SoundInterface* animplay_pVoice;
static int animplay_debugMode;
static int animplay_lipFlag;

int animplay_Register(LecShare* pShare, void* pParam)
{
    animplay_pShare = pShare;
    animplay_pCookie = pParam;
    pShare->Publish("LECMODULE=insane_animplay", &animplay_module);
    animplay_pShare->Publish("ANIMPLAY=insane", &animplay_interface);
    animplay_pShare->Publish("DEBUG=insane_animplay", &animplay_debug);
    animplay_bitmapSize = 0x4B000;
    animplay_rate = 1000;
    return 0;
}

static void animplay_Unregister(void)
{
    animplay_pShare->Unpublish("ANIMPLAY=insane");
    animplay_pShare->Unpublish("LECMODULE=insane_animplay");
    animplay_pShare->Unpublish("DEBUG=insane_animplay");
}

static int animplay_Startup(void)
{
    animplay_pMemory = animplay_pShare->Subscribe("MEMORY.workspace", animplay_pCookie);
    animplay_pDebug = animplay_pShare->Subscribe("VIRTHERC.debug", animplay_pCookie);
    animplay_pStreamer = animplay_pShare->Subscribe("STREAMER.spooler", animplay_pCookie);
    animplay_pFrame = animplay_pShare->Subscribe("ANIMFRAME.render", animplay_pCookie);
    animplay_pBitmap = animplay_pShare->Subscribe("BITMAP.buffers", animplay_pCookie);
    animplay_timer = animplay_pShare->Subscribe("TIMER.clocks", animplay_pCookie);
    animplay_pVoice = animplay_pShare->Subscribe("SOUND.lipsync", animplay_pCookie);
    if (animplay_pMemory == NULL) {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate memory services.");
        return -1;
    }
    if (animplay_pStreamer == NULL) {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate streamer services.");
        return -1;
    }
    if (animplay_pFrame == NULL) {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate animframe services.");
        return -1;
    }
    if (animplay_pBitmap == NULL) {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate bitmap services.");
        return -1;
    }
    if (animplay_timer == NULL) {
        animplay_pShare->ReportError("AnimPlay Module: Cannot allocate timer services.");
        return -1;
    }
    return 0;
}

static void animplay_Shutdown(void)
{
    animplay_pShare->Unsubscribe(animplay_pMemory);
    animplay_pShare->Unsubscribe(animplay_pStreamer);
    animplay_pShare->Unsubscribe(animplay_pFrame);
    animplay_pShare->Unsubscribe(animplay_pBitmap);
    animplay_pShare->Unsubscribe(animplay_timer);
    if (animplay_pDebug) {
        animplay_pShare->Unsubscribe(animplay_pDebug);
    }
    if (animplay_pVoice) {
        animplay_pShare->Unsubscribe(animplay_pVoice);
    }
}

static int animplay_Play(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13)
{
    AnimPlay* pPlay;
    LecBitmapHeader* pBitmap;
    int v2 = 0;
    int v1 = 0;
    char palette[0x400];
    int loops;
    int cnt;
    int rc;
    int r;

    if (a2 == 0) {
        a2 = 15;
    }
    if (animplay_pBitmap->Create(&pBitmap, a6, a7, -1)) {
        animplay_pShare->ReportError("Cannot allocate render bitmap.");
        return -1;
    }
    loops = a11;
    if (loops == 0) {
        loops = -1;
    }
    cnt = loops - 1;
    if (loops != 0) {
        do {
            rc = animplay_Create(&pPlay, (const char*)a1, a2, a12, a13, a3);
            if (rc == 0) {
                do {
                    v2 = 0;
                    v1 = 0;
                    r = animplay_Pace(pPlay, pBitmap, 0, 0, 0, palette, &v1, &v2, a8, a9);
                    if (r == 0 || r == 2 || r == 1) {
                        if (a10) {
                            int old = pPlay->stalled;

                            ((void (*)(AnimPlay*, void*, void*, int, int))a10)(pPlay, pBitmap, palette, v1, v2);
                            if (old == 0 && pPlay->stalled != 0) {
                                cnt = 0;
                                break;
                            }
                        }
                        animplay_Draw(pPlay, pBitmap, 0, a4, a5, palette, v1, v2, a3);
                    }
                } while (r >= 0 && r != 4);
                animplay_Destroy(&pPlay);
            }
        } while (cnt-- != 0);
    }
    animplay_pBitmap->Destroy(&pBitmap);
    return rc;
}

static int animplay_Create(AnimPlay** ppPlay, const char* pName, int rate, int ringSize, int frameSize, int flags)
{
    AnimPlay* pPlay;

#line 322
    pPlay = animplay_pMemory->Alloc(sizeof(AnimPlay), INSANE_FILE, __LINE__);
    if (pPlay) {
        if (animplay_pStreamer->Create(&pPlay->pStream, ringSize, frameSize) == 0) {
            pPlay->interval = 1193180 / rate;
            pPlay->flags = flags;
            pPlay->lead = 0;
            pPlay->lipFlags = 0;
            pPlay->state = 0;
            pPlay->playing = 0;
            pPlay->interval2 = pPlay->interval;
            *ppPlay = pPlay;
            if (framestream_Reset(pPlay) == 0) {
                if (framestream_Open(*ppPlay, pName, flags, -1, -1, 0) == 0) {
                    pPlay->hTick = animplay_timer->Alloc(animplay_TickCallback, 1, 2, (int)pPlay);
                    if (pPlay->hTick) {
                        pPlay->hPace = animplay_timer->Alloc(animplay_PaceCallback, pPlay->interval, 8, (int)pPlay);
                        if (pPlay->hPace) {
                            pPlay->done = 0;
                            return 0;
                        }
                        animplay_timer->Free(pPlay->hTick);
                    }
                }
                framestream_Close(*ppPlay);
            }
            animplay_pStreamer->Destroy(&pPlay->pStream);
        }
#line 362
        animplay_pMemory->Free(pPlay, INSANE_FILE, __LINE__);
    }
    *ppPlay = NULL;
    return -1;
}

static int animplay_GetTime(AnimPlay* pPlay)
{
    return pPlay->interval2 * 1000 / 1193180;
}

static int animplay_Pace(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10)
{
    if (pPlay->done == 0) {
        int n = animplay_pStreamer->Percent(pPlay->pStream);

        if (n > 60 && animplay_debug.speed > 4) {
            animplay_debug.speed--;
        }
        else if (n < 60 && animplay_debug.speed < 12) {
            animplay_debug.speed++;
        }
        animplay_pStreamer->Fill(pPlay->pStream, animplay_debug.speed << 11);
        return animplay_Step(pPlay, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }
    return -1;
}

static void animplay_Draw(AnimPlay* pPlay, void* pBitmap, int a3, int a4, int a5, void* pPalette, int a7, int a8, int a9)
{
    animplay_pFrame->Draw(pBitmap, a3, a4, a5, pPalette, a7, a8, a9);
}

static void animplay_Destroy(AnimPlay** ppPlay)
{
    AnimPlay* pPlay = *ppPlay;

    animplay_timer->Free(pPlay->hTick);
    animplay_timer->Free(pPlay->hPace);
    if (animplay_lipFlag && animplay_pVoice) {
        animplay_pVoice->Resume();
    }
    framestream_Close(pPlay);
    animplay_pStreamer->Destroy(&pPlay->pStream);
    if (*ppPlay) {
#line 457
        animplay_pMemory->Free(*ppPlay, INSANE_FILE, __LINE__);
    }
    *ppPlay = NULL;
    animplay_pFrame->Reset();
}

static void animplay_SetRate(AnimPlay* pPlay, int rate)
{
    int interval = 1193180 / rate;

    pPlay->interval = interval;
    pPlay->interval2 = interval;
    animplay_timer->SetInterval(pPlay->hTick, interval);
    animplay_timer->SetInterval(pPlay->hPace, pPlay->interval);
}

static void animplay_Rewind(AnimPlay* pPlay)
{
    pPlay->lead = 0;
    animplay_timer->SetInterval(pPlay->hTick, pPlay->interval2);
}

static int animplay_IsDone(AnimPlay* pPlay)
{
    if (pPlay->state == 5 || pPlay->ticks) {
        return 1;
    }
    return 0;
}

static int animplay_Step(AnimPlay* pPlay, void* a2, int a3, int a4, int a5, void* a6, int* a7, int* a8, int a9, int a10)
{
    unsigned flags;

    if (pPlay->state == 5) {
        pPlay->state = framestream_Tick(pPlay, a2, a3, a4, a5, a6, a7, a8, pPlay->flags, a9, a10);
        if (pPlay->state < 0) {
            pPlay->done = 1;
        }
        return pPlay->state;
    }
    if (pPlay->ticks) {
        animplay_pStreamer->Percent(pPlay->pStream);
        pPlay->ticks = 0;
        pPlay->lead--;
        pPlay->interval2 = pPlay->interval - pPlay->lead * 79545;
        if (pPlay->interval2 < pPlay->interval * 3 / 4) {
            pPlay->interval2 = pPlay->interval * 3 / 4;
        }
        if (pPlay->interval2 > pPlay->interval * 4 / 3) {
            pPlay->interval2 = pPlay->interval * 4 / 3;
        }
        if (pPlay->lead >= 3) {
            if (!(pPlay->lipFlags & 0x80)) {
                pPlay->lipFlags |= 0x80;
                pPlay->lipFlags++;
            }
        }
        else if (pPlay->lead <= 0) {
            if ((pPlay->lipFlags & 0x80) && (pPlay->lipFlags & 0x7F) < 5) {
                pPlay->lipFlags &= ~0x80;
            }
        }
        animplay_timer->SetInterval(pPlay->hTick, pPlay->interval2);
        flags = ((pPlay->lipFlags & 0x80) && !(pPlay->flags & 2)) ? (pPlay->flags | 0x8000) : pPlay->flags;
        pPlay->state = framestream_Tick(pPlay, a2, a3, a4, a5, a6, a7, a8, flags, a9, a10);
        if (animplay_pDebug && (animplay_debugMode & 0xC0000000)) {
            animplay_pDebug->Show(0, 10, animplay_pDebug->Query(4, 32));
            animplay_pDebug->Log(0, 10);
        }
        if (pPlay->state < 0) {
            pPlay->done = 1;
        }
        if (pPlay->state == 1) {
            pPlay->playing = 1;
            pPlay->lipFlags = 0;
        }
        return pPlay->state;
    }
    return 6;
}

static void animplay_TickCallback(int index, int param)
{
    ((AnimPlay*)param)->ticks++;
}

static void animplay_PaceCallback(int index, int param)
{
    AnimPlay* pPlay = (AnimPlay*)param;

    if (pPlay->playing) {
        if ((pPlay->flags & 1) && animplay_lipFlag == 0) {
            if (pPlay->lead >= 7) {
                animplay_lipFlag = 1;
                if (animplay_pVoice) {
                    animplay_pVoice->Pause();
                }
            }
        }
        else if (animplay_lipFlag) {
            if (pPlay->lead < 0) {
                animplay_lipFlag = 0;
                if (animplay_pVoice) {
                    animplay_pVoice->Resume();
                }
            }
        }
        if (pPlay->lead < 20 && animplay_lipFlag == 0) {
            pPlay->lead++;
        }
    }
}

static const char* animplay_DebugName(int index)
{
    switch (index) {
    case 0:
        return "Insane AnimPlay";
    case 1:
        return "Insane AnimPlay Debug Interface";
    case 2:
        return "This is the debugging interface for the Insane Animation module.";
    }
    return "";
}

static void animplay_SetDebug(int flags)
{
    animplay_debugMode = flags;
}

static int animplay_GetDebug(void)
{
    return animplay_debugMode;
}
