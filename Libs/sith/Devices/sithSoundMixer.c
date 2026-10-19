#include "sithSoundMixer.h"

/*
 * Retail calls rdVector_Neg3 out of line in sithSoundMixer_Update and rdVector_Set3 in
 * sithSoundMixer_CalcCameraRelativeSoundMix, but expands the Dot3 macro there.
 */
#define RDVECTOR_NO_INLINE_SET3
#define RDVECTOR_NO_INLINE_NEG3
#include <rdroid/Math/rdVector.h>

#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithThing.h>

#include <sound/Sound.h>

#include <std/General/stdMath.h>
#include <std/Win95/stdComm.h>
/* Forward prototypes; the parameters are left unnamed (release-chosen prototype style). */




static int sithSoundMixer_bStartup = 0; // Added: Init. to false
static uint16_t sithSoundMixer_channelGUIDSeed = 1;

static SithSector* sithSoundMixer_pCurSector = NULL; // Added: Init to null
tSoundChannelHandle sithSoundMixer_hCurAmbientChannel = 0;

int J3DAPI sithSoundMixer_GetNextChannelGUID(void);
tSoundChannelFlag J3DAPI sithSoundMixer_PlayFlagsToChannelFlags(SoundPlayFlag);
int J3DAPI sithSoundMixer_GetPlayPriority(SoundPlayFlag);
int J3DAPI sithSoundMixer_GetThingID(int);
int J3DAPI sithSoundMixer_GetThingIndex(int);

int J3DAPI sithSoundMixer_Startup(void)
{
    if ( sithSoundMixer_bStartup )
    {
#line 95
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Multiple open in soundmix.\n", 0, 0, 0, 0);
        return 1;
    }

    sithSoundMixer_ClearAmbientSector();
    sithSoundMixer_bStartup = 1;
    return 0;
}

void J3DAPI sithSoundMixer_Shutdown(void)
{
    if ( !sithSoundMixer_bStartup )
    {
        return;
    }

    sithSoundMixer_StopAll();
    sithSoundMixer_ClearAmbientSector();
    sithSoundMixer_bStartup = 0;
}

void J3DAPI sithSoundMixer_ClearAmbientSector(void)
{
    sithSoundMixer_pCurSector = NULL;
    sithSoundMixer_hCurAmbientChannel = 0;
}

/* Two functions the retail linker discarded (/OPT:REF); their bodies are the debug build's
   (0x42E4B8, 0x42E4CD). Name and signature inferred. */
void J3DAPI sithSoundMixer_Pause(void)
{
    if ( stdComm_IsGameActive() )
    {
        return;
    }

    Sound_Pause();
}

void J3DAPI sithSoundMixer_Resume(void)
{
    if ( stdComm_IsGameActive() )
    {
        return;
    }

    Sound_Resume();
}

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySound(tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags)
{
    if ( !sithSoundMixer_bStartup ) {
        return 0;
    }

    return Sound_Play(hSnd, volume, pan, sithSoundMixer_GetPlayPriority(playflags), sithSoundMixer_PlayFlagsToChannelFlags(playflags), sithSoundMixer_GetNextChannelGUID());
}

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySoundPos(tSoundHandle hSnd, rdVector3* pos, SithSector* pSector, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
    SoundEnvFlags envflags;

    if ( !sithSoundMixer_bStartup ) {
        return 0;
    }

    if ( !sithCamera_pCurCamera ) {
        return 0;
    }

    if ( (playflags & SOUNDPLAY_LOOP) == 0 )
    {
        rdVector3 curdvec;
        float dist;

        rdVector_Sub3(&curdvec, pos, &sithCamera_pCurCamera->lookPos);
        dist = rdVector_Normalize3QuickAcc(&curdvec);
        if ( dist > maxRadius ) {
            return 0;
        }
    }

    if ( pSector && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        envflags |= SOUND_ENV_UNDERWATER;
    }
    else
    {
        envflags = 0;
    }

    return Sound_PlayPos(hSnd, volume, sithSoundMixer_GetPlayPriority(playflags), sithSoundMixer_PlayFlagsToChannelFlags(playflags), pos->x, pos->y, pos->z, sithSoundMixer_GetNextChannelGUID(), minRadius, maxRadius, envflags);
}

tSoundChannelHandle J3DAPI sithSoundMixer_PlaySoundThing(tSoundHandle hSnd, const SithThing* pThing, float volume, float minRadius, float maxRadius, SoundPlayFlag playflags)
{
#line 294
    SITH_ASSERTREL(pThing);
    if ( !sithSoundMixer_bStartup ) {
        return 0;
    }

    if ( !sithCamera_pCurCamera ) {
        return 0;
    }

    if ( (playflags & SOUNDPLAY_LOOP) == 0 )
    {
        rdVector3 curdvec;
        float dist;

        rdVector_Sub3(&curdvec, &pThing->pos, &sithCamera_pCurCamera->lookPos);
        dist = rdVector_Normalize3QuickAcc(&curdvec);
        if ( dist > maxRadius ) {
            return 0;
        }
    }

    return Sound_PlayThing(hSnd, volume, sithSoundMixer_GetPlayPriority(playflags), sithSoundMixer_PlayFlagsToChannelFlags(playflags), sithSoundMixer_GetThingID(pThing->idx), sithSoundMixer_GetNextChannelGUID(), minRadius, maxRadius);
}

void J3DAPI sithSoundMixer_StopSound(tSoundChannelHandle hChannel)
{
    Sound_StopChannel(hChannel);
}

void J3DAPI sithSoundMixer_StopAll(void)
{
    Sound_StopAllSounds();
    sithSoundMixer_ClearAmbientSector();
}

void J3DAPI sithSoundMixer_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime)
{
    Sound_FadeVolume(hChannel, volume, secFadeTime);
}

// LINK-discarded; the debug build's 0x42E751 (between FadeVolume and FadePitch). Name inferred.
void J3DAPI sithSoundMixer_SetVolume(tSoundChannelHandle hChannel, float volume)
{
    Sound_SetVolume(hChannel, volume);
}

void J3DAPI sithSoundMixer_FadePitch(tSoundChannelHandle hSndChannel, float pitch, float secFadeTime)
{
    Sound_FadePitch(hSndChannel, pitch, secFadeTime);
}

// LINK-discarded; the debug build's 0x42E77F (after FadePitch). Name inferred.
void J3DAPI sithSoundMixer_SetPitch(tSoundChannelHandle hChannel, float pitch)
{
    Sound_SetPitch(hChannel, pitch);
}

void J3DAPI sithSoundMixer_SetPitchThing(const SithThing* pThing, tSoundHandleType handle, float pitch)
{
    Sound_SetPitchThing(sithSoundMixer_GetThingID(pThing->idx), handle, pitch);
}

void J3DAPI sithSoundMixer_SetVolumeThing(const SithThing* pThing, tSoundHandleType handle, float volume)
{
    Sound_SetVolumeThing(sithSoundMixer_GetThingID(pThing->idx), handle, volume);
}

void J3DAPI sithSoundMixer_FadeVolumeThing(const SithThing* pThing, tSoundHandleType handle, float startVolume, float endVolume)
{
    Sound_FadeVolumeThing(sithSoundMixer_GetThingID(pThing->idx), handle, startVolume, endVolume);
}

int J3DAPI sithSoundMixer_IsThingFadingVol(const SithThing* pThing, tSoundHandleType handle)
{
    return Sound_IsThingFadingVol(sithSoundMixer_GetThingID(pThing->idx), handle);
}

/* Discarded by the retail linker; restored from the debug build (0x0042E828, after IsThingFadingVol),
   called only by sithSoundClass' discarded IsModePitchFading. */
int J3DAPI sithSoundMixer_IsThingFadingPitch(const SithThing* pThing, tSoundHandleType handle)
{
    return Sound_IsThingFadingPitch(sithSoundMixer_GetThingID(pThing->idx), handle);
}

void J3DAPI sithSoundMixer_Update(void)
{
    rdVector3 curfwdDirection;
    tSoundHandle hSoundPrev;
    tSoundHandle hwndSnd;
    float sndVol;
    float sndVolPrev;

    if ( sithCamera_pCurCamera && sithCamera_pCurCamera->pSector )
    {
        if ( sithCamera_pCurCamera->pSector != sithSoundMixer_pCurSector )
        {
            if ( sithSoundMixer_pCurSector )
            {
                hSoundPrev   = sithSoundMixer_pCurSector->hAmbientSound;
                sndVolPrev = sithSoundMixer_pCurSector->ambientSoundVolume;
            }
            else
            {
                hSoundPrev   = 0;
                sndVolPrev = 0.0f;
            }

            sithSoundMixer_pCurSector = sithCamera_pCurCamera->pSector;
            hwndSnd = sithSoundMixer_pCurSector->hAmbientSound;
            sndVol = sithSoundMixer_pCurSector->ambientSoundVolume;
            if ( sithSoundMixer_pCurSector->ambientSoundVolume == 0.0f )
            {
                hwndSnd = 0;
            }

            if ( hwndSnd != hSoundPrev )
            {
                if ( sithSoundMixer_hCurAmbientChannel )
                {
                    sithSoundMixer_FadeVolume(sithSoundMixer_hCurAmbientChannel, 0.0f, 0.5f);
                    sithSoundMixer_hCurAmbientChannel = 0;
                }

                if ( hwndSnd )
                {
                    sithSoundMixer_hCurAmbientChannel = sithSoundMixer_PlaySound(hwndSnd, 0.0f, 0.0f, SOUNDPLAY_LOOP);
                    if ( sithSoundMixer_hCurAmbientChannel )
                    {
                        sithSoundMixer_FadeVolume(sithSoundMixer_hCurAmbientChannel, sndVol, 0.5f);
                    }
                }
            }
            else if ( sndVol != sndVolPrev )
            {
                sithSoundMixer_FadeVolume(sithSoundMixer_hCurAmbientChannel, sndVol, 0.5f);
            }
        }

        rdVector_Neg3(&curfwdDirection, &sithCamera_pCurCamera->orient.lvec);
        Sound_Update(&sithCamera_pCurCamera->lookPos, NULL, &sithCamera_pCurCamera->orient.uvec, &curfwdDirection, NULL);
    }
    else
    {
        Sound_Update(NULL, NULL, NULL, NULL, NULL);
    }
}

int J3DAPI sithSoundMixer_GetThingInfo(int thingId, SoundThingInfo* pThingInfo)
{
    SithThing* pThing = sithThing_GetThingByIndex(sithSoundMixer_GetThingIndex(thingId));
    if ( !pThing )
    {
        return 0;
    }

    pThingInfo->pos.x = pThing->pos.x;
    pThingInfo->pos.y = pThing->pos.y;
    pThingInfo->pos.z = pThing->pos.z;
    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThingInfo->velocity.x = pThing->moveInfo.physics.velocity.x;
        pThingInfo->velocity.y = pThing->moveInfo.physics.velocity.y;
        pThingInfo->velocity.z = pThing->moveInfo.physics.velocity.z;
    }
    else
    {
        pThingInfo->velocity.x = pThingInfo->velocity.y = pThingInfo->velocity.z = 0.0f;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        pThingInfo->envflags |= SOUND_ENV_UNDERWATER;
    }
    else
    {
        pThingInfo->envflags &= ~SOUND_ENV_UNDERWATER;
    }

    return 1;
}

void J3DAPI sithSoundMixer_CalcCameraRelativeSoundMix(const SoundSpatialInfo* pSpatialInfo, float* volume, float* pan, float* pitch)
{
    float vol = pSpatialInfo->volume;
    float dist;
    SoundEnvFlags flags = pSpatialInfo->flags;
    rdVector3 pos;
    rdVector3 dir;
    float dot;
    float panDot;
    float dminmax;
    float volLev;
    float minDist;

    if ( !sithCamera_pCurCamera )
    {
        return;
    }

    rdVector_Set3(&pos, pSpatialInfo->pos.x, pSpatialInfo->pos.y, pSpatialInfo->pos.z);
    rdVector_Sub3(&dir, &pos, &sithCamera_pCurCamera->lookPos);
    dist = rdVector_Normalize3QuickAcc(&dir);

    dot = rdVector_Dot3(&sithCamera_pCurCamera->orient.lvec, &dir);
    if ( dot < 0.0f )
    {
        vol = (1.0f - (-dot) * 0.3f) * vol;
    }

    if ( pan )
    {
        panDot = rdVector_Dot3(&sithCamera_pCurCamera->orient.rvec, &dir);
        *pan = panDot;
    }

    if ( dist < pSpatialInfo->maxRadius && sithCamera_pCurCamera->pSector )
    {
        if ( (flags & SOUND_ENV_UNDERWATER) != 0 && (sithCamera_pCurCamera->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            dist = dist * 1.5f;
        }
        else if ( (flags & SOUND_ENV_UNDERWATER) == 0 && (sithCamera_pCurCamera->pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            dist = dist * 1.5f;
        }
    }

    if ( volume )
    {
        minDist = dist - pSpatialInfo->minRadius;
        if ( minDist > 0.0f )
        {
            if ( pSpatialInfo->minRadius != pSpatialInfo->maxRadius )
            {
                dminmax = 1.0f / (pSpatialInfo->maxRadius - pSpatialInfo->minRadius);
            }
            else
            {
                dminmax = 0.0f;
            }

            volLev  = minDist * dminmax;
            volLev  = STDMATH_CLAMP(volLev, 0.0f, 1.0f);
            vol     = (1.0f - volLev) * vol;
        }
        else
        {
            vol = pSpatialInfo->volume;
        }

        *volume = vol;
    }

    if ( pitch )
    {
        *pitch = 1.0f;
    }
}

void J3DAPI sithSoundMixer_StopAllSoundsThing(const SithThing* pThing)
{
    Sound_StopThing(sithSoundMixer_GetThingID(pThing->idx), SOUND_ALLTHINGSOUNDHANDLE);
}

tSoundChannelHandle J3DAPI sithSoundMixer_GetChannelHandle(int guid)
{
    if ( !guid ) {
        return 0;
    }

    return Sound_GetChannelHandle(guid);
}

void J3DAPI sithSoundMixer_StopSoundThing(const SithThing* pThing, tSoundHandleType handle)
{
    Sound_StopThing(sithSoundMixer_GetThingID(pThing->idx), handle);
}

void J3DAPI sithSoundMixer_SetSectorAmbientSound(SithSector* pSector, tSoundHandle hSnd, float volume)
{
#line 661
    SITH_ASSERTREL(pSector);

    pSector->hAmbientSound      = hSnd;
    pSector->ambientSoundVolume = volume;
    if ( sithSoundMixer_pCurSector == pSector )
    {
        sithSoundMixer_pCurSector = NULL;
    }
}

int J3DAPI sithSoundMixer_GameSave(tFileHandle fh)
{
    size_t size = sizeof(sithSoundMixer_hCurAmbientChannel);
    if ( size != sith_pHS->pFileWrite(fh, &sithSoundMixer_hCurAmbientChannel, size) )
    {
        return 1;
    }

    if ( !Sound_Save(fh) )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithSoundMixer_GameRestore(tFileHandle fh)
{
    tSoundChannelHandle hAmbChannel;
    size_t size;

    sithSoundMixer_StopAll();

    size = sizeof(sithSoundMixer_hCurAmbientChannel);
    if ( size != sith_pHS->pFileRead(fh, &hAmbChannel, sizeof(sithSoundMixer_hCurAmbientChannel)) )
    {
        return 1;
    }

    sithSoundMixer_ClearAmbientSector();

    if ( !Sound_Restore(fh) )
    {
        return 1;
    }

    Sound_StopChannel(hAmbChannel); // TODO: Why hAmbChannel is not set to sithSoundMixer_hCurAmbientChannel?
    return 0;
}

int J3DAPI sithSoundMixer_GetNextChannelGUID(void)
{
    if ( !sithSoundMixer_channelGUIDSeed ) {
        sithSoundMixer_channelGUIDSeed = 1;
    }

    return ((sithPlayer_playerNum + 1) << 16) | (sithSoundMixer_channelGUIDSeed++ & 0xFFFF);
}

tSoundChannelFlag J3DAPI sithSoundMixer_PlayFlagsToChannelFlags(SoundPlayFlag flags)
{
    tSoundChannelFlag chflags = (flags & SOUNDPLAY_LOOP) != 0 ? SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING : SOUND_CHANNEL_PLAYING;
    if ( (flags & SOUNDPLAY_PLAYONCE) != 0 ) {
        chflags |= SOUND_CHANNEL_PLAYONCE;
    }

    if ( (flags & SOUNDPLAY_PLAYTHINGONCE) != 0 ) {
        chflags |= SOUND_CHANNEL_PLAYTHINGONCE;
    }

    return chflags;
}

int J3DAPI sithSoundMixer_GetPlayPriority(SoundPlayFlag playflags)
{
    int priority;

    switch ( playflags & (SOUNDPLAY_HIGHEST_PRIORITY | SOUNDPLAY_HIGH_PRIORITY) )
    {
        default:
            priority = 0;
            break;

        case SOUNDPLAY_HIGH_PRIORITY:
            priority = 500;
            break;

        case SOUNDPLAY_HIGHEST_PRIORITY:
            priority = 1000;
            break;

        case SOUNDPLAY_HIGHEST_PRIORITY | SOUNDPLAY_HIGH_PRIORITY:
            priority = 2000;
            break;
    }

    return priority;
}

int J3DAPI sithSoundMixer_GetThingID(int idx)
{
    return idx + 1;
}

int J3DAPI sithSoundMixer_GetThingIndex(int thingID)
{
    return thingID - 1;
}
