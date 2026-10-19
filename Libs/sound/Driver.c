#include <Windows.h> // timeGetTime, include first to avoid undefined warning due to WIN32_LEAN_AND_MEAN
#include "Driver.h"


#include <sound/AudioLib.h>

#include <std/General/stdMath.h>

#define SOUNDDRIVER_FARVOLUMETHRESHOLD 0.01

static float SoundDriver_maxVolume = 1.0f;
static float SoundDriver_defaultDistanceFactor = 0.3f;
static float SoundDriver_minDistance = 15.0f;
static float SoundDriver_maxDistance = 10000.0f;
static float SoundDriver_defaultRolloffFactor = 1.0f;
static float SoundDriver_defaultDopplerFactor = 1.0f;

static tPrintfFunc SoundDriver_pfLogError = 0;
static int SoundDriver_aPanTable[256] = { 0 };
static float SoundDriver_frontOrientATan = 0.0f;
static int SoundDriver_bNoDSound3D = 0;
static int SoundDriver_bUseGlobalFocusBuf = 0;
static HWND SoundDriver_hwnd = 0;
static int SoundDriver_bDSoundNotCreated = 0;
static SoundCalcListenerSoundMixFunc SoundDriver_pfCalcListenerSoundMix = 0;
static SoundDriverGetSoundBufferDataFunc SoundDriver_pfGetSoundBufferData = 0;
static LPDIRECTSOUND SoundDriver_pDirectSound = NULL;
static LPDIRECTSOUNDBUFFER SoundDriver_pDSBuffer = NULL;
static LPDIRECTSOUND3DLISTENER SoundDriver_pDS3DListener = NULL;
static rdVector3 SoundDriver_curListenerPos = { 0 };
static int SoundDriver_bUse3DCaps = 0;
static int SoundDriver_bHas3DHW = 0;

static const int SoundDriver_aDBTable[256] = {
    -9640, -4810, -4210, -3850, -3600, -3410, -3250, -3120, -3000, -2900, -2810, -2730, -2650, -2580, -2520, -2460,
    -2400, -2350, -2300, -2250, -2210, -2160, -2120, -2080, -2050, -2010, -1980, -1950, -1910, -1880, -1850, -1830,
    -1800, -1770, -1750, -1720, -1700, -1670, -1650, -1630, -1600, -1580, -1560, -1540, -1520, -1500, -1480, -1460,
    -1450, -1430, -1410, -1390, -1380, -1360, -1340, -1330, -1310, -1300, -1280, -1270, -1250, -1240, -1220, -1210,
    -1200, -1180, -1170, -1160, -1140, -1130, -1120, -1110, -1090, -1080, -1070, -1060, -1050, -1040, -1020, -1010,
    -1000,  -990,  -980,  -970,  -960,  -950,  -940,  -930,  -920,  -910,  -900,  -890,  -880,  -870,  -860,  -850,
     -840,  -830,  -830,  -820,  -810,  -800,  -790,  -780,  -770,  -770,  -760,  -750,  -740,  -730,  -730,  -720,
     -710,  -700,  -690,  -690,  -680,  -670,  -660,  -660,  -650,  -640,  -640,  -630,  -620,  -610,  -610,  -600,
     -590,  -590,  -580,  -570,  -570,  -560,  -550,  -550,  -540,  -530,  -530,  -520,  -520,  -510,  -500,  -500,
     -490,  -490,  -480,  -470,  -470,  -460,  -460,  -450,  -440,  -440,  -430,  -430,  -420,  -420,  -410,  -410,
     -400,  -390,  -390,  -380,  -380,  -370,  -370,  -360,  -360,  -350,  -350,  -340,  -340,  -330,  -330,  -320,
     -320,  -310,  -310,  -300,  -300,  -290,  -290,  -280,  -280,  -270,  -270,  -260,  -260,  -260,  -250,  -250,
     -240,  -240,  -230,  -230,  -220,  -220,  -210,  -210,  -210,  -200,  -200,  -190,  -190,  -180,  -180,  -180,
     -170,  -170,  -160,  -160,  -160,  -150,  -150,  -140,  -140,  -140,  -130,  -130,  -120,  -120,  -120,  -110,
     -110,  -100,  -100,  -100,   -90,   -90,   -80,   -80,   -80,   -70,   -70,   -70,   -60,   -60,   -50,   -50,
      -50,   -40,   -40,   -40,   -30,   -30,   -30,   -20,   -20,   -20,   -10,   -10,   -10,   -10,   -10,   -10
};

const DXStatus SoundDriver_aDStatusTbl[16] =
{
  { DSERR_ALLOCATED         , "DSERR_ALLOCATED"          },
  { DSERR_ALREADYINITIALIZED, "DSERR_ALREADYINITIALIZED" },
  { DSERR_BADFORMAT         , "DSERR_BADFORMAT"          },
  { DSERR_BUFFERLOST        , "DSERR_BUFFERLOST"         },
  { DSERR_CONTROLUNAVAIL    , "DSERR_CONTROLUNAVAIL"     },
  { DSERR_GENERIC           , "DSERR_GENERIC"            },
  { DSERR_INVALIDCALL       , "DSERR_INVALIDCALL"        },
  { DSERR_INVALIDPARAM      , "DSERR_INVALIDPARAM"       },
  { DSERR_NOAGGREGATION     , "DSERR_NOAGGREGATION"      },
  { DSERR_NODRIVER          , "DSERR_NODRIVER"           },
  { DSERR_OTHERAPPHASPRIO   , "DSERR_OTHERAPPHASPRIO"    },
  { DSERR_OUTOFMEMORY       , "DSERR_OUTOFMEMORY"        },
  { DSERR_PRIOLEVELNEEDED   , "DSERR_PRIOLEVELNEEDED"    },
  { DSERR_UNINITIALIZED     , "DSERR_UNINITIALIZED"      },
  { DSERR_UNSUPPORTED       , "DSERR_UNSUPPORTED"        },
  { 0                       , "Unknown error code."      }
};

void J3DAPI SoundDriver_InitPanTable(void);
HRESULT J3DAPI SoundDriver_DSCheckStatus(HRESULT code, size_t codeLine);

void J3DAPI SoundDriver_CalcListeneSoundMix(float x, float y, float z, float volume, float pitch, float* newVolume, float* pan, float* newPitch, float minRadius, float maxRadius, SoundEnvFlags envflags);
double J3DAPI SoundDriver_CalcDistToListener(float x1, float y1, float z1, float x2, float y2, float z2);

void J3DAPI SoundDriver_RestoreSoundBufferData(LPDIRECTSOUNDBUFFER pDSBuf);

int J3DAPI SoundDriver_Open(int bNoSound3D, int bGlobalFocus, HWND hwnd, LPDIRECTSOUND pDirectSound, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, SoundDriverGetSoundBufferDataFunc pfGetSoundBufferData, tPrintfFunc pfLogError)
{
    SoundDriver_pfLogError         = pfLogError;
    SoundDriver_bNoDSound3D        = bNoSound3D;
    SoundDriver_bUseGlobalFocusBuf = bGlobalFocus;
    SoundDriver_hwnd               = hwnd;

    SoundDriver_InitPanTable();

    if ( pDirectSound )
    {
        SoundDriver_pDirectSound = pDirectSound;
        SoundDriver_bDSoundNotCreated = 1;
    }
    else
    {
        SoundDriver_pDirectSound = SoundDriver_CreateDirectSound(SoundDriver_bNoDSound3D);
    }

    if ( SoundDriver_pDirectSound )
    {
        SoundDriver_CreateListener();
        SoundDriver_SetOuputFormat(SoundDriver_pDirectSound, 22050u, 16u, 2);
    }

    SoundDriver_pfCalcListenerSoundMix = pfCalcListenerSoundMix;
    SoundDriver_pfGetSoundBufferData   = pfGetSoundBufferData;
    return SoundDriver_pDirectSound != NULL;
}

void J3DAPI SoundDriver_Close(void)
{
    SoundDriver_ReleaseListener();
    if ( !SoundDriver_bDSoundNotCreated )
    {
        SoundDriver_ReleaseDirectSound(SoundDriver_pDirectSound);
    }

    SoundDriver_pDirectSound = NULL;
}

void J3DAPI SoundDriver_SetMaxVolume(float volume)
{
    SoundDriver_maxVolume = volume;
}

LPDIRECTSOUNDBUFFER J3DAPI SoundDriver_CreateAndPlay(int samplesPerSec, int nBitsPerSample, int numChannels, const uint8_t* pSoundData, size_t dataSize, tSoundChannelFlag* pFlags, LPDIRECTSOUNDBUFFER pSBuffer)
{
    DSBUFFERDESC dsbufd;
    LPDIRECTSOUNDBUFFER pDSBufferNew = NULL;
    HRESULT dsres;

#line 205
    if ( pSBuffer && SoundDriver_DSCheckStatus(IDirectSound_DuplicateSoundBuffer(SoundDriver_pDirectSound, pSBuffer, &pDSBufferNew), __LINE__) == DS_OK )
    {
#line 208
        SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetCurrentPosition(pDSBufferNew, 0), __LINE__);
        if ( SoundDriver_bUse3DCaps )
        {
            DSBCAPS dsbcaps;
            dsbcaps.dwSize = sizeof(DSBCAPS);
#line 214
            if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetCaps(pDSBufferNew, &dsbcaps), __LINE__) != DS_OK )
            {
                if ( pDSBufferNew )
                {
                    IDirectSoundBuffer_Release(pDSBufferNew);
                }

                return NULL;
            }

            if ( (dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) == 0 )
            {
                if ( pDSBufferNew )
                {
                    IDirectSoundBuffer_Release(pDSBufferNew);
                }

                return NULL;
            }

            *pFlags |= SOUND_CHANNEL_HWMIXING;
        }

        *pFlags |= SOUND_CHANNEL_UNKNOWN_800000;
    }
    else
    {
        WAVEFORMATEX wavefmt;
        DWORD dwAudioBytes1;
        DWORD dwAudioBytes2;
        void* lpvAudioPtr1;
        void* lpvAudioPtr2;

        if ( (*pFlags & SOUND_CHANNEL_COMPRESSED) != 0 )
        {
            dataSize = ((tAudioCompressedData*)pSoundData)->uncompressedSize;
        }

        wavefmt.wFormatTag      = WAVE_FORMAT_PCM;
        wavefmt.nChannels       = numChannels;
        wavefmt.nSamplesPerSec  = samplesPerSec;
        wavefmt.nAvgBytesPerSec = samplesPerSec * (nBitsPerSample >> 3) * numChannels;
        wavefmt.nBlockAlign     = (nBitsPerSample >> 3) * numChannels;
        wavefmt.wBitsPerSample  = nBitsPerSample;
        wavefmt.cbSize          = 0;

        memset(&dsbufd, 0, sizeof(DSBUFFERDESC));
        dsbufd.dwSize = sizeof(DSBUFFERDESC);
        dsbufd.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY;

        if ( SoundDriver_bUseGlobalFocusBuf || (*pFlags & SOUND_CHANNEL_GLOBALFOCUS) != 0 )
        {
            dsbufd.dwFlags |= DSBCAPS_GLOBALFOCUS;
            *pFlags |= SOUND_CHANNEL_GLOBALFOCUS;
        }

        if ( SoundDriver_bUse3DCaps && (*pFlags & SOUND_CHANNEL_USE3DCAPS) == 0 )
        {
            *pFlags |= SOUND_CHANNEL_3DSOUND;
            if ( numChannels == 2 )
            {
                *pFlags |= SOUND_CHANNEL_UNKNOWN_8000000;
            }
            else
            {
                dsbufd.dwFlags |= DSBCAPS_CTRL3D;
            }
        }

        dsbufd.dwBufferBytes = dataSize;
        dsbufd.lpwfxFormat = &wavefmt;

#line 272
        dsres = SoundDriver_DSCheckStatus(IDirectSound_CreateSoundBuffer(SoundDriver_pDirectSound, &dsbufd, &pDSBufferNew, NULL), __LINE__);
        if ( dsres == DS_OK )
        {
            DSBCAPS dsbcaps;
            dsbcaps.dwSize = sizeof(DSBCAPS);
#line 278
            if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetCaps(pDSBufferNew, &dsbcaps), __LINE__) != DS_OK )
            {
                if ( pDSBufferNew )
                {
                    IDirectSoundBuffer_Release(pDSBufferNew);
                }

                return NULL;
            }

            if ( SoundDriver_bUse3DCaps )
            {
                if ( (dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) == 0 )
                {
                    if ( pDSBufferNew )
                    {
                        IDirectSoundBuffer_Release(pDSBufferNew);
                    }

                    return NULL;
                }

                *pFlags |= SOUND_CHANNEL_HWMIXING;
            }
            else
            {
                *pFlags &= ~SOUND_CHANNEL_HWMIXING;
            }
        }

        if ( dsres == DS_OK )
        {
            if ( (*pFlags & SOUND_CHANNEL_RESTART) != 0 )
            {
#line 308
                SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetVolume(pDSBufferNew, (LONG)(SoundDriver_maxVolume * (double)DSBVOLUME_MIN)), __LINE__);
            }
            else
            {
#line 310
                SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetVolume(pDSBufferNew, (LONG)(SoundDriver_maxVolume * (double)DSBVOLUME_MAX)), __LINE__);
            }

            if ( (*pFlags & SOUND_CHANNEL_COMPRESSED) != 0 )
            {
                DWORD uncompressedSize = ((tAudioCompressedData*)pSoundData)->uncompressedSize;

#line 315
                dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBufferNew, 0, uncompressedSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
                if ( dsres == DSERR_BUFFERLOST )
                {
#line 317
                    dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Restore(pDSBufferNew), __LINE__);
                    if ( dsres == DS_OK )
                    {
#line 319
                        dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBufferNew, 0, uncompressedSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
                    }
                }

                if ( dsres == DS_OK )
                {
                    tAudioCompressorState compressor;
                    AudioLib_ResetCompressor(&compressor);

                    if ( lpvAudioPtr2 )
                    {
                        uint8_t* pUncompressedData = (uint8_t*)malloc(uncompressedSize);
                        AudioLib_Uncompress(&compressor, pUncompressedData, ((tAudioCompressedData*)pSoundData)->compressedData, uncompressedSize);
                        memcpy(lpvAudioPtr1, pUncompressedData, dwAudioBytes1);
                        memcpy(lpvAudioPtr2, &pUncompressedData[dwAudioBytes1], dwAudioBytes2);
                        free(pUncompressedData);
                    }
                    else
                    {
                        AudioLib_Uncompress(&compressor, (uint8_t*)lpvAudioPtr1, ((tAudioCompressedData*)pSoundData)->compressedData, dwAudioBytes1);
                    }

#line 337
                    dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Unlock(pDSBufferNew, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2), __LINE__);
                }
            }
            else
            {
#line 341
                dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBufferNew, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
                if ( dsres == DSERR_BUFFERLOST )
                {
#line 343
                    dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Restore(pDSBufferNew), __LINE__);
                    if ( dsres == DS_OK )
                    {
#line 345
                        dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBufferNew, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
                    }
                }

                if ( dsres == DS_OK )
                {
                    memcpy(lpvAudioPtr1, pSoundData, dwAudioBytes1);
                    if ( lpvAudioPtr2 )
                    {
                        memcpy(lpvAudioPtr2, &pSoundData[dwAudioBytes1], dwAudioBytes2);
                    }
                }

#line 352
                dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Unlock(pDSBufferNew, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2), __LINE__);
            }
        }
        else
        {
            if ( pDSBufferNew )
            {
                IDirectSoundBuffer_Release(pDSBufferNew);
            }

            return NULL;
        }
    }

    if ( (*pFlags & SOUND_CHANNEL_PLAYING) != 0 )
    {
#line 363
        dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Play(pDSBufferNew, 0, 0, (*pFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) ? DSBPLAY_LOOPING : 0), __LINE__);
        if ( dsres == DSERR_BUFFERLOST )
        {
#line 365
            dsres = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Restore(pDSBufferNew), __LINE__);
            if ( dsres == DS_OK )
            {
                SoundDriver_RestoreSoundBufferData(pDSBufferNew);
#line 368
                SoundDriver_DSCheckStatus(IDirectSoundBuffer_Play(pDSBufferNew, 0, 0, (*pFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) ? DSBPLAY_LOOPING : 0), __LINE__);
            }
        }
    }

    return pDSBufferNew;
}

void J3DAPI SoundDriver_Release(LPDIRECTSOUNDBUFFER pDSBuf)
{
    if ( pDSBuf )
    {
        IDirectSoundBuffer_Release(pDSBuf);
    }
}

void J3DAPI SoundDriver_Play(LPDIRECTSOUNDBUFFER pDSBuf, int bLoop)
{
    HRESULT hr;

    if ( pDSBuf )
    {
#line 397
        hr = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Play(pDSBuf, 0, 0, bLoop ? DSBPLAY_LOOPING : 0), __LINE__);
        if ( hr == DSERR_BUFFERLOST )
        {
#line 399
            hr = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Restore(pDSBuf), __LINE__);
            if ( hr == DS_OK )
            {
                SoundDriver_RestoreSoundBufferData(pDSBuf);
#line 402
                SoundDriver_DSCheckStatus(IDirectSoundBuffer_Play(pDSBuf, 0, 0, bLoop ? DSBPLAY_LOOPING : 0), __LINE__);
            }
        }
    }
}

void J3DAPI SoundDriver_Stop(LPDIRECTSOUNDBUFFER pDSBuf)
{
    if ( pDSBuf )
    {
#line 416
        SoundDriver_DSCheckStatus(IDirectSoundBuffer_Stop(pDSBuf), __LINE__);
    }
}

void J3DAPI SoundDriver_SetVolume(LPDIRECTSOUNDBUFFER pDSBuf, float volume)
{
    if ( !pDSBuf )
    {
        return;
    }

#line 430
    SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetVolume(pDSBuf, SoundDriver_GetDecibelVolume(volume)), __LINE__);
}

float J3DAPI SoundDriver_GetVolume(LPDIRECTSOUNDBUFFER pDSBuf)
{
    LONG dbVol;
    if ( !pDSBuf )
    {
        return 1.0f;
    }

#line 445
    if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetVolume(pDSBuf, &dbVol), __LINE__) == DS_OK )
    {
        return SoundDriver_GetVolumeFromDecibels(dbVol);
    }

    return 1.0f;
}

void J3DAPI SoundDriver_SetPan(LPDIRECTSOUNDBUFFER pDSoundBuf, float pan)
{
    if ( !pDSoundBuf )
    {
        return;
    }

#line 462
    SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetPan(pDSoundBuf, SoundDriver_GetIntPan(pan)), __LINE__);
}

/* Retail's linker discarded this function (no retail symbol, address or caller), but its
   0.0f literal survives in the .rdata pool between GetVolume's 1.0f and GetFrequency's
   22050.0f, which fixes the pool order. Minimal placeholder body: only the literal is
   evidenced. */
float J3DAPI SoundDriver_GetPan(LPDIRECTSOUNDBUFFER pDSBuf)
{
    return 0.0f;
}

void J3DAPI SoundDriver_SetFrequency(LPDIRECTSOUNDBUFFER pDSBuf, float freq)
{
    if ( !pDSBuf )
    {
        return;
    }

#line 494
    SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetFrequency(pDSBuf, (int32_t)freq), __LINE__);
}

float J3DAPI SoundDriver_GetFrequency(LPDIRECTSOUNDBUFFER pDSBuf)
{
    DWORD freq;
    if ( !pDSBuf )
    {
        return 22050.0f;
    }

#line 509
    if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetFrequency(pDSBuf, &freq), __LINE__) == DS_OK )
    {
        return (float)(int)freq;
    }

    return 22050.0f;
}

size_t J3DAPI SoundDriver_GetCurrentPosition(LPDIRECTSOUNDBUFFER pDSoundBuf)
{
    DWORD dwCurPlayPosition;
    HRESULT hr;

    if ( !pDSoundBuf )
    {
        return 0;
    }

#line 528
    hr = SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetCurrentPosition(pDSoundBuf, &dwCurPlayPosition, NULL), __LINE__);
    if ( hr == DS_OK )
    {
        return dwCurPlayPosition;
    }

    return 0;
}

tSoundChannelFlag J3DAPI SoundDriver_GetStatusAndCaps(LPDIRECTSOUNDBUFFER pDSBuffer)
{
    DWORD dwStatus;
    DSBCAPS dsbcaps;
    tSoundChannelFlag flags = 0;

    if ( !pDSBuffer )
    {
        return 0;
    }

#line 549
    if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetStatus(pDSBuffer, &dwStatus), __LINE__) != DS_OK )
    {
        return 0;
    }

    if ( (dwStatus & DSBSTATUS_PLAYING) != 0 )
    {
        flags |= SOUND_CHANNEL_PLAYING;
    }

    if ( (dwStatus & DSBSTATUS_LOOPING) != 0 )
    {
        flags |= SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING;
    }

    dsbcaps.dwSize = sizeof(DSBCAPS);
#line 559
    if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_GetCaps(pDSBuffer, &dsbcaps), __LINE__) != DS_OK )
    {
        return 0;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_CTRL3D) != 0 )
    {
        flags |= SOUND_CHANNEL_3DSOUND;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_LOCSOFTWARE) != 0 )
    {
        flags |= SOUND_CHANNEL_SWMIXING;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE) != 0 )
    {
        flags |= SOUND_CHANNEL_HWMIXING;
    }

    if ( (dsbcaps.dwFlags & DSBCAPS_GLOBALFOCUS) != 0 )
    {
        flags |= SOUND_CHANNEL_GLOBALFOCUS;
    }

    return flags;
}

void J3DAPI SoundDriver_SetListenerPosition(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient, const rdVector3* pRightOrient)
{

    if ( pPos )
    {
        SoundDriver_curListenerPos.x = pPos->x;
        SoundDriver_curListenerPos.y = pPos->y;
        SoundDriver_curListenerPos.z = pPos->z;
    }

    if ( !SoundDriver_pfCalcListenerSoundMix && pTopOrient && pFrontOrient )
    {
        if ( pFrontOrient->x != 0.0f )
        {
            SoundDriver_frontOrientATan = (float)atan2(pFrontOrient->z, pFrontOrient->x);
            if ( errno == EDOM )
            {
                SoundDriver_frontOrientATan = 0.0f;
            }
        }
        else
        {
            SoundDriver_frontOrientATan = 0.0f;
        }
    }

    if ( SoundDriver_bUse3DCaps && SoundDriver_pDS3DListener )
    {
        if ( pPos )
        {
#line 612
            SoundDriver_DSCheckStatus(IDirectSound3DListener_SetPosition(SoundDriver_pDS3DListener, pPos->x, pPos->y, pPos->z, DS3D_DEFERRED), __LINE__);
        }

        if ( pVelocity )
        {
#line 617
            SoundDriver_DSCheckStatus(IDirectSound3DListener_SetVelocity(SoundDriver_pDS3DListener, pVelocity->x, pVelocity->y, pVelocity->z, DS3D_DEFERRED), __LINE__);
        }

        if ( pFrontOrient && pTopOrient )
        {
#line 623
            SoundDriver_DSCheckStatus(IDirectSound3DListener_SetOrientation(SoundDriver_pDS3DListener, pFrontOrient->x, pFrontOrient->y, pFrontOrient->z, pTopOrient->x, pTopOrient->y, pTopOrient->z, DS3D_DEFERRED), __LINE__);
        }
    }
}

void J3DAPI SoundDriver_ListenerCommitDeferred(void)
{
    if ( SoundDriver_bUse3DCaps && SoundDriver_pDS3DListener )
    {
#line 636
        SoundDriver_DSCheckStatus(IDirectSound3DListener_CommitDeferredSettings(SoundDriver_pDS3DListener), __LINE__);
    }
}

void J3DAPI SoundDriver_SetPosAndVelocity(LPDIRECTSOUNDBUFFER pBuffer, float x, float y, float z, float velX, float velY, float velZ, float minDistance, float maxDistance, SoundEnvFlags envflags)
{
    HRESULT result = DS_OK;  // unreferenced; the debug build's frame keeps its slot
    LPDIRECTSOUND3DBUFFER pDS3DBuffer;

    if ( !pBuffer )
    {
        return;
    }

    if ( SoundDriver_bUse3DCaps )
    {
#line 663
        if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_QueryInterface(pBuffer, &IID_IDirectSound3DBuffer, &pDS3DBuffer), __LINE__) == DS_OK )
        {
#line 665
            SoundDriver_DSCheckStatus(IDirectSound3DBuffer_SetPosition(pDS3DBuffer, x, y, z, DS3D_DEFERRED), __LINE__);
            SoundDriver_DSCheckStatus(IDirectSound3DBuffer_SetVelocity(pDS3DBuffer, velX, velY, velZ, DS3D_DEFERRED), __LINE__);

            if ( minDistance < 0.0 )
            {
                minDistance = SoundDriver_minDistance;
            }

            if ( maxDistance < 0.0 )
            {
                maxDistance = SoundDriver_maxDistance;
            }

#line 671
            SoundDriver_DSCheckStatus(IDirectSound3DBuffer_SetMinDistance(pDS3DBuffer, minDistance, DS3D_DEFERRED), __LINE__);
            SoundDriver_DSCheckStatus(IDirectSound3DBuffer_SetMaxDistance(pDS3DBuffer, maxDistance, DS3D_DEFERRED), __LINE__);
            IDirectSound3DBuffer_Release(pDS3DBuffer);
        }
    }
}

int J3DAPI SoundDriver_Update3DSound(LPDIRECTSOUNDBUFFER* ppDSBuf, float x, float y, float z, float volume, float pitch, tSoundChannelFlag* pChannelFlags, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    int b3DControl;
    float newVolume;
    float pan;
    float newPitch;

    LPDIRECTSOUNDBUFFER pDSBuf = ppDSBuf ? *ppDSBuf : NULL;

    b3DControl = 0;
    if ( (*pChannelFlags & SOUND_CHANNEL_USE3DCAPS) != 0 )
    {
        b3DControl = 1;
    }
    else if ( SoundDriver_Use3DCaps() && (*pChannelFlags & SOUND_CHANNEL_UNKNOWN_8000000) == 0 )
    {
        float distSquare = ((SoundDriver_curListenerPos.x - x) * (SoundDriver_curListenerPos.x - x))
            + ((SoundDriver_curListenerPos.y - y) * (SoundDriver_curListenerPos.y - y))
            + ((SoundDriver_curListenerPos.z - z) * (SoundDriver_curListenerPos.z - z));
        if ( distSquare <= 0.25f )
        {
            b3DControl = 1;
        }
        else
        {
            b3DControl = 0;
        }
    }

    if ( (*pChannelFlags & SOUND_CHANNEL_DISABLEHWMIXING) != 0 )
    {
        b3DControl = 0;
    }

    if ( !b3DControl )
    {
        SoundDriver_CalcListeneSoundMix(x, y, z, volume, pitch, &newVolume, &pan, &newPitch, minRadius, maxRadius, envflags);
        if ( newVolume < SOUNDDRIVER_FARVOLUMETHRESHOLD
            && (*pChannelFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
        {
            return 0;
        }
    }
    else
    {
        newVolume = volume;
        pan = 0.0f;
        newPitch = pitch;
    }

    if ( SoundDriver_Use3DCaps()
        && (*pChannelFlags & (SOUND_CHANNEL_USE3DCAPS && (*pChannelFlags & SOUND_CHANNEL_UNKNOWN_8000000) == 0)) == 0
        && pDSBuf )
    {
        LPDIRECTSOUND3DBUFFER pDS3DBuffer;
#line 735
        if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_QueryInterface(pDSBuf, &IID_IDirectSound3DBuffer, &pDS3DBuffer), __LINE__) == DS_OK )
        {
            IDirectSound3DBuffer_SetMode(pDS3DBuffer, b3DControl ? DS3DMODE_DISABLE : DS3DMODE_NORMAL, DS3D_IMMEDIATE);
            IDirectSound3DBuffer_Release(pDS3DBuffer);
        }
    }

    // Note: No need to check for null pDSBuf because all underlying functions do that
    if ( (*pChannelFlags & SOUND_CHANNEL_HWMIXING) == 0 || (*pChannelFlags & SOUND_CHANNEL_DISABLEHWMIXING) != 0 )
    {
        SoundDriver_SetVolume(pDSBuf, newVolume * SoundDriver_maxVolume);
        SoundDriver_SetPan(pDSBuf, pan);
        if ( newPitch != 1.0 )
        {
            float freq = SoundDriver_GetFrequency(pDSBuf);
            freq *= newPitch;
            freq /= pitch;
            SoundDriver_SetFrequency(pDSBuf, freq);
        }
    }

    if ( (*pChannelFlags & SOUND_CHANNEL_FAR) != 0 )
    {
        if ( newVolume > SOUNDDRIVER_FARVOLUMETHRESHOLD )
        {
            *pChannelFlags |= SOUND_CHANNEL_RESTART;
        }
    }
    else if ( (*pChannelFlags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
        && (*pChannelFlags & SOUND_CHANNEL_3DSOUND) == SOUND_CHANNEL_3DSOUND
        && newVolume < SOUNDDRIVER_FARVOLUMETHRESHOLD )
    {
        SoundDriver_Stop(pDSBuf);
        if ( pDSBuf )
        {
            IDirectSoundBuffer_Release(pDSBuf);
            *ppDSBuf = NULL;
        }

        *pChannelFlags &= ~(SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING);
        *pChannelFlags |= SOUND_CHANNEL_FAR;
    }

    return 1;
}

void J3DAPI SoundDriver_SetGlobals(float distanceFactor, float minDistance, float maxDistance, float rolloffFactor, float dopplerFactor)
{
    SoundDriver_defaultDistanceFactor = distanceFactor / 10.0f;
    SoundDriver_minDistance = minDistance;
    SoundDriver_maxDistance = maxDistance;
    SoundDriver_defaultRolloffFactor = rolloffFactor;
    SoundDriver_defaultDopplerFactor = dopplerFactor;

    if ( SoundDriver_bUse3DCaps && SoundDriver_pDS3DListener )
    {
#line 793
        SoundDriver_DSCheckStatus(IDirectSound3DListener_SetDistanceFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDistanceFactor, DS3D_IMMEDIATE), __LINE__);
#line 796
        SoundDriver_DSCheckStatus(IDirectSound3DListener_SetRolloffFactor(SoundDriver_pDS3DListener, SoundDriver_defaultRolloffFactor, DS3D_IMMEDIATE), __LINE__);
#line 799
        SoundDriver_DSCheckStatus(IDirectSound3DListener_SetDopplerFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDopplerFactor, DS3D_IMMEDIATE), __LINE__);
    }
}

int J3DAPI SoundDriver_Has3DHW(void)
{
    return SoundDriver_bHas3DHW;
}

int J3DAPI SoundDriver_Use3DCaps(void)
{
    return SoundDriver_bUse3DCaps;
}

LPDIRECTSOUND J3DAPI SoundDriver_GetDSound(void)
{
    return SoundDriver_pDirectSound;
}

LPDIRECTSOUND J3DAPI SoundDriver_CreateDirectSound(int bNoSound3D)
{
    DSCAPS dscaps;
    LPDIRECTSOUND pDSound = NULL;
    HRESULT res;

    res = DirectSoundCreate(NULL, &pDSound, NULL);
    if ( res != DS_OK )
    {
        if ( SoundDriver_pfLogError )
        {
            SoundDriver_pfLogError("DirectSound: Original DS fails\n");
        }

        return NULL;
    }

#line 862
    res = SoundDriver_DSCheckStatus(IDirectSound_SetCooperativeLevel(pDSound, SoundDriver_hwnd, DSSCL_PRIORITY), __LINE__);
    if ( res != DS_OK )
    {
        if ( pDSound )
        {
            IDirectSound_Release(pDSound);
        }

        if ( SoundDriver_pfLogError )
        {
            SoundDriver_pfLogError("DirectSound: Set co-operative level fails!\n");
        }

        SoundDriver_bUse3DCaps = 0;
        return NULL;
    }

    memset(&dscaps, 0, sizeof(DSCAPS));
    dscaps.dwSize = sizeof(DSCAPS);
#line 873
    SoundDriver_DSCheckStatus(IDirectSound_GetCaps(pDSound, &dscaps), __LINE__);

    SoundDriver_bUse3DCaps = 0;
    if ( dscaps.dwMaxHw3DAllBuffers )
    {
        if ( !SoundDriver_bNoDSound3D )
        {
            SoundDriver_bUse3DCaps = 1;
        }

        SoundDriver_bHas3DHW = 1;
    }

    return pDSound;
}

void J3DAPI SoundDriver_ReleaseDirectSound(LPDIRECTSOUND pDSound)
{
    SoundDriver_ReleasePrimaryDSBuffer();
    if ( pDSound )
    {
        IDirectSound_Release(pDSound);
    }
}

void J3DAPI SoundDriver_CreateListener(void)
{
    if ( SoundDriver_bUse3DCaps )
    {
        LPDIRECTSOUNDBUFFER pBuffer = SoundDriver_GetPrimaryDSBuffer(SoundDriver_pDirectSound);
#line 911
        if ( SoundDriver_DSCheckStatus(IDirectSoundBuffer_QueryInterface(pBuffer, &IID_IDirectSound3DListener, &SoundDriver_pDS3DListener), __LINE__) == DS_OK )
        {
#line 914
            SoundDriver_DSCheckStatus(IDirectSound3DListener_SetDistanceFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDistanceFactor, DS3D_IMMEDIATE), __LINE__);
#line 917
            SoundDriver_DSCheckStatus(IDirectSound3DListener_SetRolloffFactor(SoundDriver_pDS3DListener, SoundDriver_defaultRolloffFactor, DS3D_IMMEDIATE), __LINE__);
#line 920
            SoundDriver_DSCheckStatus(IDirectSound3DListener_SetDopplerFactor(SoundDriver_pDS3DListener, SoundDriver_defaultDopplerFactor, DS3D_IMMEDIATE), __LINE__);
        }
        else
        {
            SoundDriver_pDS3DListener = NULL;
        }
    }
}

void J3DAPI SoundDriver_ReleaseListener(void)
{
    if ( SoundDriver_pDS3DListener )
    {
        IDirectSound3DListener_Release(SoundDriver_pDS3DListener);
        SoundDriver_pDS3DListener = NULL;
    }
}

void J3DAPI SoundDriver_SetOuputFormat(LPDIRECTSOUND pDSound, uint32_t nSamplesPerSec, uint32_t nBitsPerSample, uint32_t numChannels)
{
    WAVEFORMATEX fmt;

    LPDIRECTSOUNDBUFFER pBuffer = SoundDriver_GetPrimaryDSBuffer(pDSound);
    if ( !pBuffer )
    {
        if ( SoundDriver_pfLogError )
        {
            SoundDriver_pfLogError("Sound.DLL: SetOuputFormat: Grabbing primary buffer fails!\n");
        }

        return;
    }

    fmt.wFormatTag      = WAVE_FORMAT_PCM;
    fmt.nChannels       = numChannels;
    fmt.nSamplesPerSec  = nSamplesPerSec;
    fmt.nAvgBytesPerSec = nSamplesPerSec * (nBitsPerSample / 8) * numChannels;
    fmt.nBlockAlign     = (nBitsPerSample / 8) * numChannels;
    fmt.wBitsPerSample  = nBitsPerSample;
    fmt.cbSize          = 0;

#line 968
    SoundDriver_DSCheckStatus(IDirectSoundBuffer_SetFormat(pBuffer, &fmt), __LINE__);
    SoundDriver_ReleasePrimaryDSBuffer();
}

LPDIRECTSOUNDBUFFER J3DAPI SoundDriver_GetPrimaryDSBuffer(LPDIRECTSOUND pDSound)
{
    DSBUFFERDESC dsbd;
    HRESULT res;

    if ( !pDSound )
    {
        return NULL;
    }

    if ( SoundDriver_pDSBuffer )
    {
        return SoundDriver_pDSBuffer;
    }

    memset(&dsbd, 0, sizeof(DSBUFFERDESC));
    dsbd.dwSize = sizeof(DSBUFFERDESC);
    dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;

    if ( SoundDriver_bUse3DCaps )
    {
        dsbd.dwFlags |= DSBCAPS_CTRL3D;
    }

    if ( SoundDriver_bUseGlobalFocusBuf )
    {
        dsbd.dwFlags |= DSBCAPS_GLOBALFOCUS;
    }

#line 997
    res = SoundDriver_DSCheckStatus(IDirectSound_CreateSoundBuffer(pDSound, &dsbd, &SoundDriver_pDSBuffer, NULL), __LINE__);
    if ( res != DS_OK )
    {
        SoundDriver_pDSBuffer = NULL;
    }

    return SoundDriver_pDSBuffer;
}

void J3DAPI SoundDriver_ReleasePrimaryDSBuffer(void)
{
    if ( SoundDriver_pDSBuffer )
    {
        IDirectSoundBuffer_Release(SoundDriver_pDSBuffer);
        SoundDriver_pDSBuffer = NULL;
    }
}

int J3DAPI SoundDriver_GetDecibelVolume(float volume)
{
    int idx = (int)(volume * 255.0);
    if ( idx < 0 )
    {
        idx = 0;
    }

    return idx > 255 ? 0 : SoundDriver_aDBTable[idx];
}

int J3DAPI SoundDriver_GetIntPan(float pan)
{
    int idx = (int)((pan * 128.0) + 128.0);
    if ( idx < 0 )
    {
        idx = 0;
    }

    return idx > 255 ? 0 : SoundDriver_aPanTable[idx];
}

float J3DAPI SoundDriver_GetVolumeFromDecibels(int dB)
{
    int idx = 256;
    while ( idx-- && SoundDriver_aDBTable[idx] > dB )
    {
        ;
    }

    return ((float)(idx * 1.0 / 255.0));
}

/* Also discarded by retail's linker (no retail symbol, address or caller): only its
   1.0/128.0 double, orphaned in the .rdata pool between GetVolumeFromDecibels'
   1.0/255.0 and CalcListeneSoundMix' 100000.0, is evidenced. Minimal placeholder body.
 */
float J3DAPI SoundDriver_GetPanFromTable(int pan)
{
    return (float)(pan / 128.0);
}

void J3DAPI SoundDriver_InitPanTable(void)
{
    int i;
    for ( i = 0; i < 128; ++i )
    {
        SoundDriver_aPanTable[i + 128] = -SoundDriver_aDBTable[255 - 2 * i];
        SoundDriver_aPanTable[127 - i] = SoundDriver_aDBTable[255 - 2 * i];
    }
}

HRESULT J3DAPI SoundDriver_DSCheckStatus(HRESULT code, size_t codeLine)
{
    const DXStatus* pEntry;
    if ( code == DS_OK )
    {
        return DS_OK;
    }

    for ( pEntry = SoundDriver_aDStatusTbl; pEntry->code && pEntry->code != code; ++pEntry )
    {
        ;
    }

    if ( pEntry->code && SoundDriver_pfLogError )
    {
        SoundDriver_pfLogError("Sound.DLL Driver.C:%d Return value: %s\n", codeLine, pEntry->text);
    }

    return code;
}

void J3DAPI SoundDriver_CalcListeneSoundMix(float x, float y, float z, float volume, float pitch, float* newVolume, float* pan, float* newPitch, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    float curdist;
    float dx;
    float curfactor;
    float dz;
    float atan;

    if ( SoundDriver_pfCalcListenerSoundMix )
    {
        SoundSpatialInfo spatialInfo;
        spatialInfo.pos.x     = x;
        spatialInfo.pos.y     = y;
        spatialInfo.pos.z     = z;
        spatialInfo.volume    = volume;
        spatialInfo.pitch     = pitch;
        spatialInfo.minRadius = minRadius;
        spatialInfo.maxRadius = maxRadius;
        spatialInfo.flags     = envflags;

        if ( SoundDriver_curListenerPos.x >= 100000.0 )
        {
            if ( newVolume )
            {
                *newVolume = 0.0f;
            }

            if ( pan )
            {
                *pan = 0.0f;
            }
        }
        else
        {
            SoundDriver_pfCalcListenerSoundMix(&spatialInfo, newVolume, pan, newPitch);
        }
    }
    else
    {
        if ( !newVolume )
        {
            return;
        }

        *newVolume = volume;
        if ( pan )
        {
            *pan = 0.0f;
        }

        if ( newPitch )
        {
            *newPitch = 1.0f;
        }

        curdist = (float)SoundDriver_CalcDistToListener(x, y, z, SoundDriver_curListenerPos.x, SoundDriver_curListenerPos.y, SoundDriver_curListenerPos.z);
        if ( curdist > 500.0f )
        {
            *newVolume = 0.0f;
            return;
        }

        if ( curdist < 200.0f )
        {
            if ( curdist >= 30.0f )
            {
                curdist = J3DMIN(curdist, 200.0f);
                curdist -= 30.0f;
                curdist = 170.0f - curdist;
                curfactor = curdist / 170.0f;
                *newVolume = (*newVolume - 0.0f) * curfactor + 0.0f;
            }
            else
            {
                *newVolume = 1.0f;
                return;
            }
        }
        else
        {
            *newVolume = 0.0f;
        }

        if ( !pan )
        {
            return;
        }

        dx = x - SoundDriver_curListenerPos.x;
        dz = z - SoundDriver_curListenerPos.z;
        if ( dx > -0.5 && dx < 0.5 )
        {
            return;
        }

        if ( dz > -0.5 && dz < 0.5 )
        {
            if ( dx > 0.0 )
            {
                *pan = 1.0f;
            }
            else
            {
                *pan = -1.0f;
            }

            return;
        }

        atan = (float)atan2(dz, dx);
        if ( errno == EDOM )
        {
            atan = 0.0f;
        }

        atan -= SoundDriver_frontOrientATan;
        *pan = -sin(atan);
    }
}

double J3DAPI SoundDriver_CalcDistToListener(float x1, float y1, float z1, float x2, float y2, float z2)
{
    if ( (x2 = x2 - x1) < 0.0f )
    {
        x2 = -x2;
    }

    z2 = z2 - z1;
    if ( z2 < 0.0f )
    {
        z2 = -z2;
    }

    x1 = x2;
    if ( x2 > z2 )
    {
        x2 = z2;
    }

    x2 = x2 / 2.0f;
    return x1 + z2 - x2 + (y2 - y1 < 0.0f ? -(y2 - y1) : y2 - y1);
}

void J3DAPI SoundDriver_RestoreSoundBufferData(LPDIRECTSOUNDBUFFER pDSBuf)
{
    uint32_t dataSize;
    unsigned int bCompressed;
    DWORD dwAudioBytes1;
    DWORD dwAudioBytes2;
    void* lpvAudioPtr1 = NULL;
    void* lpvAudioPtr2 = NULL;
    HRESULT res;
    uint8_t* pSoundData;

    if ( !SoundDriver_pfGetSoundBufferData )
    {
        return;
    }

    pSoundData = SoundDriver_pfGetSoundBufferData(pDSBuf, &dataSize, &bCompressed);
    if ( bCompressed )
    {
        DWORD uncompressedSize = ((tAudioCompressedData*)pSoundData)->uncompressedSize;
#line 1333
        res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBuf, 0, uncompressedSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
        if ( res == DSERR_BUFFERLOST )
        {
#line 1335
            res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Restore(pDSBuf), __LINE__);
            if ( res == DS_OK )
            {
#line 1337
                res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBuf, 0, uncompressedSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
            }
        }

        if ( res == DS_OK )
        {
            tAudioCompressorState compressor;
            AudioLib_ResetCompressor(&compressor);

            if ( lpvAudioPtr2 )
            {
                uint8_t* pUncompressedData = (uint8_t*)malloc(uncompressedSize);
                AudioLib_Uncompress(&compressor, pUncompressedData, ((tAudioCompressedData*)pSoundData)->compressedData, uncompressedSize);
                memcpy(lpvAudioPtr1, pUncompressedData, dwAudioBytes1);
                memcpy(lpvAudioPtr2, &pUncompressedData[dwAudioBytes1], dwAudioBytes2);
                free(pUncompressedData);
            }
            else
            {
                AudioLib_Uncompress(&compressor, (uint8_t*)lpvAudioPtr1, ((tAudioCompressedData*)pSoundData)->compressedData, dwAudioBytes1);
            }

#line 1356
            res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Unlock(pDSBuf, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2), __LINE__);
        }
    }
    else
    {
#line 1360
        res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBuf, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
        if ( res == DSERR_BUFFERLOST )
        {
#line 1362
            res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Restore(pDSBuf), __LINE__);
            if ( res == DS_OK )
            {
#line 1364
                res = SoundDriver_DSCheckStatus(IDirectSoundBuffer_Lock(pDSBuf, 0, dataSize, &lpvAudioPtr1, &dwAudioBytes1, &lpvAudioPtr2, &dwAudioBytes2, 0), __LINE__);
            }
        }

        if ( res == DS_OK )
        {
            memcpy(lpvAudioPtr1, pSoundData, dwAudioBytes1);
            if ( lpvAudioPtr2 )
            {
                memcpy(lpvAudioPtr2, &pSoundData[dwAudioBytes1], dwAudioBytes2);
            }
        }

#line 1372
        SoundDriver_DSCheckStatus(IDirectSoundBuffer_Unlock(pDSBuf, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2), __LINE__);
    }
}

uint32_t J3DAPI SoundDriver_GetTimeMsec(void)
{
    return timeGetTime();
}

void J3DAPI SoundDriver_Sleep(unsigned int dwMilliseconds)
{
    Sleep(dwMilliseconds);
}
