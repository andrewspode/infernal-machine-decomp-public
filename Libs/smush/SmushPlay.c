/*
 * SmushPlay: the host glue between the game and LucasArts' INSANE movie-player library (retail 0x004E30B0-0x004E33F0).
 * It registers every INSANE module with the LEC share registry, subscribes to the services it drives (`NAME.app`) and plays
 * a movie by looping the ANIMPLAY service's frame pacer, handing each decoded frame to the game's blit callback.
 * Retail has no names for this module's internals: they are inferred from the calls.
 *
 */
#include <string.h>
#include "insane/lec.h"
#include "insane/animplay.h"
#include "insane/Lec/LecShare.h"
#include "insane/Bitmap/Bitmap.h"
#include "insane/Sound/Engine.h"

/* the modules' registration functions */
extern int readauto_Register(LecShare* pShare, void* pParam);
extern int animplay_Register(LecShare* pShare, void* pParam);
extern int animframe_Register(LecShare* pShare, void* pParam);
extern int streamer_Register(LecShare* pShare, void* pParam);
extern int sound_Register(LecShare* pShare, void* pParam);
extern int dsnd_Register(LecShare* pShare, void* pParam);
extern int dwave_Register(LecShare* pShare, void* pParam);
extern int dbloc16_Register(LecShare* pShare, void* pParam);
extern int stdfile_Register(LecShare* pShare, void* pParam);
extern int zfilio_Register(LecShare* pShare, void* pParam);
extern int stdio_Register(LecShare* pShare, void* pParam);
extern int bitmap_Register(LecShare* pShare, void* pParam);
extern int timer_Register(LecShare* pShare, void* pParam);
extern int memory_Register(LecShare* pShare, void* pParam);

/* module configuration entry points */
extern int zfilio_Configure(int tag, ...);
extern int dsnd_Configure(int tag, ...);
extern int dbloc16_Configure(int tag, ...);

int SmushPlay_SysStartup(void* hWnd, void* pDSound);
int SmushPlay_PlayMovie(const char* pFilename, int rate, int flags, int a4, int a5, int width, int height, int a8, int a9,
                        int (*pBlit)(void* pBitmap, int frame), int count, int ringSize, int frameSize);
static int smushplay_Blit(int a1, int a2, int a3, int a4, void* pBitmap);
void SmushPlay_SysShutdown(void);
void SmushPlay_SetGlobalVolume(int volume);
void* smushplay_GetHwnd(void);

void* smushplay_pMemory;
SoundInterface* smushplay_pSound;
void* smushplay_pFile;
void* smushplay_pAnimFrame;
LecShare* smushplay_pShare;
AnimPlayTable* smushplay_pAnimPlay;
BitmapInterface* smushplay_pBitmap;
void* smushplay_pTimer;

static LecBatch smushplay_services[] = {
    { &smushplay_pMemory, "MEMORY.app", 0 },
    { &smushplay_pTimer, "TIMER.app", 0 },
    { &smushplay_pFile, "FILE.app", 0 },
    { (void**)&smushplay_pBitmap, "BITMAP.app", 0 },
    { (void**)&smushplay_pSound, "SOUND.app", 0 },
    { &smushplay_pAnimFrame, "ANIMFRAME.app", 0 },
    { (void**)&smushplay_pAnimPlay, "ANIMPLAY.app", 0 },
    { 0, 0, 0 }
};

static LecModuleEntry smushplay_modules[] = {
    { memory_Register, 0 },
    { timer_Register, 0 },
    { bitmap_Register, 0 },
    { stdio_Register, 0 },
    { zfilio_Register, "FILIO=insane_stdio" },
    { stdfile_Register, 0 },
    { dbloc16_Register, 0 },
    { dwave_Register, 0 },
    { dsnd_Register, 0 },
    { sound_Register, 0 },
    { streamer_Register, 0 },
    { animframe_Register, 0 },
    { animplay_Register, 0 },
    { readauto_Register, 0 },
    { 0, 0 }
};

static int smushplay_volume = 127;

static void* smushplay_hWnd;
static AnimPlay* smushplay_play;
static int smushplay_abort;
static int (*smushplay_pBlit)(void* pBitmap, int frame);

int SmushPlay_SysStartup(void* hWnd, void* pDSound)
{
    smushplay_hWnd = hWnd;
    smushplay_pShare = LecShare_Init(lec_Log, smushplay_modules, "READANIM=insane_auto FILIO=insane_zfilio", "");
    if (smushplay_pShare) {
        zfilio_Configure(1, ".snm", 0);
        zfilio_Configure(1, ".znm", 0);
        dsnd_Configure(5, pDSound);
        if (LecShare_Startup() == 0) {
            if (((int (*)(LecBatch*, const char*))smushplay_pShare->pfn10)(smushplay_services, 0) == 0) {
                return 1;
            }
            LecShare_Shutdown();
        }
        LecShare_UnregisterAll();
    }
    return 0;
}

int SmushPlay_PlayMovie(const char* pFilename, int rate, int flags, int a4, int a5, int width, int height, int a8, int a9,
                        int (*pBlit)(void* pBitmap, int frame), int count, int ringSize, int frameSize)
{
    int v1 = 0;
    int v2 = 0;
    LecBitmapHeader* pBitmap;
    int i;
    int rc;
    int r;
    char aPath[260];
    unsigned char aPalette[1024];

    strcpy(aPath, pFilename);
    if (rate == 0) {
        rate = 15;
    }
    if (smushplay_pBitmap->Create(&pBitmap, width, height, 3)) {
        return 0;
    }
    if (count == 0) {
        count = -1;
    }
    smushplay_pSound->SetVolume(-255, smushplay_volume);
    smushplay_pBlit = pBlit;
    smushplay_abort = 0;
    dbloc16_Configure(1, smushplay_Blit);
    for (i = 0; i < count; i++) {
        rc = smushplay_pAnimPlay->Create(&smushplay_play, aPath, rate, ringSize, frameSize, flags | 0xD);
        if (rc == 0) {
            do {
                v1 = 0;
                v2 = 0;
                r = smushplay_pAnimPlay->Pace(smushplay_play, pBitmap, 0, 0, 0, aPalette, &v2, &v1, a8, a9);
                if (smushplay_abort) {
                    i = count;
                    break;
                }
            } while (r >= 0 && r != 4);
            smushplay_pAnimPlay->Destroy(&smushplay_play);
        }
    }
    smushplay_play = 0;
    smushplay_pBitmap->Destroy(&pBitmap);
    return rc == 0;
}

static int smushplay_Blit(int a1, int a2, int a3, int a4, void* pBitmap)
{
    if (smushplay_pBlit) {
        smushplay_abort = smushplay_pBlit(pBitmap, smushplay_play->frame);
    }
    return 0;
}

void SmushPlay_SysShutdown(void)
{
    LecShare_Shutdown();
    LecShare_UnregisterAll();
    ((int (*)(LecBatch*))smushplay_pShare->pfn14)(smushplay_services);
}

void SmushPlay_SetGlobalVolume(int volume)
{
    if (volume >= 0 && volume < 128) {
        smushplay_volume = volume;
    }
}

void* smushplay_GetHwnd(void)
{
    return smushplay_hWnd;
}
