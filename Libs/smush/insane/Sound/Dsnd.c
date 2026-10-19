/*
 * insane_dsnd: INSANE's `SNDDRIVER=insane_dsnd` service (retail 0x004E74B0-0x004E7D60): a DirectSound output driver. A timer callback
 * keeps a looping secondary buffer of 3-16 blocks of 0x1800 bytes filled by the mixer.
 */
#include <windows.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <dsound.h>
#include "../lec.h"
#include "../Timer/Timer.h"
#include "Dsnd.h"
#include "Mixer.h"

extern int sound_Null(void);
extern void* smushplay_GetHwnd(void);

int dsnd_Register(LecShare* pShare, void* pParam);
int dsnd_Configure(int tag, ...);
static void dsnd_Unregister(void);
static int dsnd_Startup(void);
static void dsnd_Shutdown(void);
static int dsnd_Start(int rate, int a2, void (*pfnFill)(int, SoundFeed));
static void dsnd_Stop(void);
static void dsnd_SetVolume(int volume);
static int dsnd_GetVolume(void);
static int dsnd_GetRate(void);
static void dsnd_Callback(int index, int param);
static int dsnd_Init(void);
static void dsnd_Print(const char* pFormat, ...);
static void dsnd_OpenSecondary(int blocks);
static void dsnd_Release(void);
static int dsnd_GetPlayPos(void);

static LecModule dsnd_module = { dsnd_Register, dsnd_Unregister, dsnd_Startup, dsnd_Shutdown };
static SndDriver dsnd_interface = { dsnd_Start, dsnd_Stop, dsnd_SetVolume, dsnd_GetVolume, (int (*)(int))sound_Null, dsnd_GetRate };
static int dsnd_field54CA08 = -1;
static int dsnd_field54CA0C = -1;
static int dsnd_rate = 22050;
static int dsnd_volume = 127;
static int dsnd_savedVolume = -5000;
static WAVEFORMATEX dsnd_primaryFormat = { WAVE_FORMAT_PCM, 2, 22050, 88200, 4, 16, 0 };
static WAVEFORMATEX dsnd_streamFormat = { WAVE_FORMAT_PCM, 2, 22050, 88200, 4, 16, 0 };

/* Retail's .bss block (0x017E58B8-0x017E7308) is in declaration order, VC6's layout for zero-initialised statics. dsnd_scratch is
   one 0x1800-byte block (0x600 stereo 16-bit frames, the size Callback mixes into it). dsnd_unused_17E7304 is storage no code in
   the image reaches (whole-.text dword scan); its name is invented after the address. */
static void* dsnd_pMixerTable = NULL;
static int dsnd_cfg3 = 0;
static int dsnd_volTable[128] = { 0 };
static void (*dsnd_pfnFill)(int, SoundFeed) = NULL;
static int dsnd_cfg1 = 0;
static int dsnd_writePos = 0;
static LecShare* dsnd_pShare = NULL;
static int dsnd_numBlocks = 0;
static TimerInterface* dsnd_pTimer = NULL;
static int dsnd_playPos = 0;
static int dsnd_cfg4 = 0;
static void* dsnd_pParam = NULL;
static char dsnd_scratch[0x1800] = { 0 };
static int dsnd_cfg2 = 0;
static LPDIRECTSOUND dsnd_pDS = NULL;
static LPDIRECTSOUNDBUFFER dsnd_pPrimary = NULL;
static LPDIRECTSOUNDBUFFER dsnd_pSecondary = NULL;
static int dsnd_busy = 0;
static int dsnd_ownDS = 0;
static int dsnd_hTimer = 0;
static int dsnd_tablesReady = 0;
static int dsnd_unused_17E7304 = 0;

int dsnd_Register(LecShare* pShare, void* pParam)
{
    dsnd_pShare = pShare;
    dsnd_pParam = pParam;
    pShare->Publish("LECMODULE=insane_dsnd", &dsnd_module);
    dsnd_pShare->Publish("SNDDRIVER=insane_dsnd", &dsnd_interface);
    dsnd_cfg1 = -1;
    dsnd_cfg3 = -1;
    dsnd_cfg2 = -1;
    dsnd_cfg4 = -1;
    return 0;
}

int dsnd_Configure(int tag, ...)
{
    va_list args;

    va_start(args, tag);
    while (tag) {
        switch (tag) {
            case 4:
                dsnd_cfg4 = va_arg(args, int);
                break;
            case 2:
                dsnd_cfg2 = va_arg(args, int);
                break;
            case 3:
                dsnd_cfg3 = va_arg(args, int);
                break;
            case 1:
                dsnd_cfg1 = va_arg(args, int);
                break;
            case 5:
                dsnd_pDS = va_arg(args, LPDIRECTSOUND);
                dsnd_ownDS = 0;
                break;
            default:
                return -1;
        }
        tag = va_arg(args, int);
    }
    return 0;
}

static void dsnd_Unregister(void)
{
    dsnd_pShare->Unpublish("SNDDRIVER=insane_dsnd");
    dsnd_pShare->Unpublish("LECMODULE=insane_dsnd");
}

static int dsnd_Startup(void)
{
    dsnd_pTimer = dsnd_pShare->Subscribe("TIMER=clock", dsnd_pParam);
    if (dsnd_pTimer == NULL) {
        dsnd_pShare->ReportError("DirectSoundDriver Module: Cannot allocate timer services.");
        return -1;
    }
    dsnd_pMixerTable = dsnd_pShare->Subscribe("MEMORY.mixertable", dsnd_pParam);
    if (dsnd_pMixerTable == NULL) {
        dsnd_pShare->ReportError("DirectSoundDriver Module: Cannot allocate timer services.");
        return -1;
    }
    return 0;
}

static void dsnd_Shutdown(void)
{
    dsnd_pShare->Unsubscribe(dsnd_pTimer);
    dsnd_pShare->Unsubscribe(dsnd_pMixerTable);
}

static int dsnd_Start(int rate, int a2, void (*pfnFill)(int, SoundFeed))
{
    if (rate < 22050) {
        rate = 22050;
    }
    if (dsnd_Init()) {
        return -1;
    }
    if (mixer_Init(dsnd_pMixerTable, 4, 0x400, 0, 1, 1)) {
        return -1;
    }
    dsnd_pfnFill = pfnFill;
    dsnd_rate = rate;
    dsnd_hTimer = dsnd_pTimer->Alloc(dsnd_Callback, 1193180 / (rate / 1024), 2, 0);
    return 0;
}

static void dsnd_Stop(void)
{
    if (dsnd_hTimer) {
        dsnd_pTimer->Free(dsnd_hTimer);
    }
    dsnd_hTimer = 0;
    mixer_Done(dsnd_pMixerTable);
    dsnd_Release();
}

static void dsnd_SetVolume(int volume)
{
    if (!dsnd_tablesReady) {
        int* pEntry;
        int v;

        dsnd_volTable[0] = -10000;
        dsnd_volTable[1] = -7000;
        dsnd_volTable[2] = -6000;
        dsnd_volTable[3] = -5200;
        dsnd_volTable[4] = -5000;
        dsnd_volTable[5] = -4500;
        dsnd_volTable[6] = -4200;
        dsnd_volTable[7] = -4100;
        dsnd_volTable[8] = -4000;
        pEntry = &dsnd_volTable[8];
        for (v = 0; v < 48000; v += 2000) {
            *pEntry++ = v / 23 - 4000;
        }
        pEntry = &dsnd_volTable[32];
        for (v = 0; v < 32000; v += 1000) {
            *pEntry++ = v / 31 - 2000;
        }
        pEntry = &dsnd_volTable[64];
        for (v = 0; v < 64000; v += 1000) {
            *pEntry++ = v / 63 - 1000;
        }
        dsnd_tablesReady = 1;
    }
    dsnd_volume = volume & 0x7F;
    IDirectSoundBuffer_SetVolume(dsnd_pSecondary, dsnd_volume < 0      ? dsnd_volTable[0]
                                                : dsnd_volume > 0x7F ? dsnd_volTable[127]
                                                                     : dsnd_volTable[dsnd_volume]);
}

static int dsnd_GetVolume(void)
{
    return dsnd_volume;
}

static int dsnd_GetRate(void)
{
    return 22050;
}

static void dsnd_Callback(int index, int param)
{
    LPVOID pPtr1;
    LPVOID pPtr2;
    int len1;
    int len2;
    int cur = dsnd_GetPlayPos();

    if (cur != -1) {
        int w = dsnd_field54CA0C;
        int n = dsnd_numBlocks;

        if (cur != (w + 1) % n) {
            char* pDst;
            int chunk;

            if (w < 0) {
                w = cur;
            }
            dsnd_field54CA0C = (w + 1) % n;
            if (IDirectSoundBuffer_Lock(dsnd_pSecondary, dsnd_field54CA0C * 0x1800, 0x1800, &pPtr1, (LPDWORD)&len1, &pPtr2, (LPDWORD)&len2, 0) == 0) {
                if (pPtr2 && len2) {
                    chunk = 0x600;
                    pDst = dsnd_scratch;
                }
                else {
                    chunk = len1 / 4;
                    pDst = pPtr1;
                }
                mixer_Begin(chunk);
                dsnd_pfnFill(chunk, mixer_Feed);
                mixer_Mix((short*)pDst, chunk);
                if (pPtr2 && len2) {
                    memcpy(pPtr1, dsnd_scratch, len1);
                    memcpy(pPtr2, dsnd_scratch + len1, len2);
                }
                IDirectSoundBuffer_Unlock(dsnd_pSecondary, pPtr1, len1, pPtr2, len2);
            }
        }
    }
    dsnd_pTimer->SetInterval(dsnd_hTimer, 1193180 / (dsnd_rate / 1024));
}

static int dsnd_Init(void)
{
    HRESULT hr;
    DSBUFFERDESC desc;

    if (dsnd_pDS != NULL) {
        dsnd_OpenSecondary(3);
        return 0;
    }
    hr = DirectSoundCreate(NULL, &dsnd_pDS, NULL);
    if (hr) {
        dsnd_pDS = NULL;
        dsnd_Print("Cannot create DirectSound object (%d)", hr);
        return -1;
    }
    hr = IDirectSound_SetCooperativeLevel(dsnd_pDS, (HWND)smushplay_GetHwnd(), DSSCL_EXCLUSIVE);
    if (hr) {
        dsnd_Print("DirectSound: Error %08X while setting cooperative level", hr);
        return -1;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;
    desc.dwBufferBytes = 0;
    desc.dwReserved = 0;
    desc.lpwfxFormat = NULL;
    hr = IDirectSound_CreateSoundBuffer(dsnd_pDS, &desc, &dsnd_pPrimary, NULL);
    if (hr) {
        dsnd_Print("DirectSound: Error %08X while creating primary buffer", hr);
        return -1;
    }
    if (IDirectSoundBuffer_SetFormat(dsnd_pPrimary, &dsnd_primaryFormat)) {
        return -1;
    }
    if (IDirectSoundBuffer_GetVolume(dsnd_pPrimary, (LPLONG)&dsnd_savedVolume)) {
        return -1;
    }
    if (IDirectSoundBuffer_SetVolume(dsnd_pPrimary, 0)) {
        return -1;
    }
    dsnd_OpenSecondary(3);
    dsnd_ownDS = 1;
    return 0;
}

/* The driver's error print: an empty body here and in the debug build (0x0053CB50, between Init and OpenSecondary); retail
   LINK folded it onto the game's return-only stdPlatform_UnlockHandle (0x0046D9E0). */
static void dsnd_Print(const char* pFormat, ...)
{
}

static void dsnd_OpenSecondary(int blocks)
{
    DSBUFFERDESC desc;

    dsnd_busy = 1;
    if (dsnd_pSecondary) {
        IDirectSoundBuffer_Release(dsnd_pSecondary);
    }
    dsnd_pSecondary = NULL;
    if (blocks < 3) {
        blocks = 3;
    }
    else if (blocks > 16) {
        blocks = 16;
    }
    dsnd_numBlocks = blocks;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwBufferBytes = blocks * 0x1800;
    desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
    desc.dwReserved = 0;
    desc.lpwfxFormat = &dsnd_streamFormat;
    if (IDirectSound_CreateSoundBuffer(dsnd_pDS, &desc, &dsnd_pSecondary, NULL) == 0) {
        LPVOID pPtr1;
        LPVOID pPtr2;
        DWORD len1;
        DWORD len2;

        IDirectSoundBuffer_SetVolume(dsnd_pSecondary, 0);
        if (IDirectSoundBuffer_Lock(dsnd_pSecondary, 0, dsnd_numBlocks * 0x1800, &pPtr1, &len1, &pPtr2, &len2, 0) == 0) {
            memset(pPtr1, 0, len1);
            if (pPtr2) {
                memset(pPtr2, 0, len2);
            }
            IDirectSoundBuffer_Unlock(dsnd_pSecondary, pPtr1, len1, pPtr2, len2);
        }
        IDirectSoundBuffer_Play(dsnd_pSecondary, 0, 0, DSBPLAY_LOOPING);
    }
    dsnd_busy = 0;
}

static void dsnd_Release(void)
{
    if (dsnd_pSecondary) {
        IDirectSoundBuffer_Release(dsnd_pSecondary);
    }
    dsnd_pSecondary = NULL;
    if (dsnd_pPrimary && dsnd_ownDS) {
        IDirectSoundBuffer_SetVolume(dsnd_pPrimary, dsnd_savedVolume);
        IDirectSoundBuffer_Release(dsnd_pPrimary);
        dsnd_pPrimary = NULL;
        dsnd_ownDS = 0;
    }
    if (dsnd_pDS && dsnd_ownDS) {
        IDirectSound_Release(dsnd_pDS);
    }
    dsnd_pDS = NULL;
}

static int dsnd_GetPlayPos(void)
{
    if (dsnd_pSecondary) {
        if (IDirectSoundBuffer_GetCurrentPosition(dsnd_pSecondary, (LPDWORD)&dsnd_playPos, (LPDWORD)&dsnd_writePos) == 0) {
            return (dsnd_playPos / 0x1800) % dsnd_numBlocks;
        }
    }
    return -1;
}
