#include "sithCogFunctionSound.h"

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogexec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdMath.h>

void J3DAPI sithCogFunctionSound_PlaySoundThing(SithCog* pCog);
void J3DAPI sithCogFunctionSound_StopSoundThing(SithCog* pCog);
void J3DAPI sithCogFunctionSound_StopAllSoundsThing(SithCog* pCog);
void J3DAPI sithCogFunctionSound_PlaySoundPos(SithCog* pCog);
void J3DAPI sithCogFunctionSound_PlaySoundLocal(SithCog* pCog);
void J3DAPI sithCogFunctionSound_PlaySoundGlobal(SithCog* pCog);
void J3DAPI sithCogFunctionSound_StopSound(SithCog* pCog);
void J3DAPI sithCogFunctionSound_LoadSound(SithCog* pCog);
void J3DAPI sithCogFunctionSound_PlaySoundClass(SithCog* pCog);
void J3DAPI sithCogFunctionSound_StopSoundClass(SithCog* pCog);
void J3DAPI sithCogFunctionSound_PlayVoiceMode(SithCog* pCog);
void J3DAPI sithCogFunctionSound_ChangeVolume(SithCog* pCog);
void J3DAPI sithCogFunctionSound_ChangePitch(SithCog* pCog);
void J3DAPI sithCogFunctionSound_SectorSound(SithCog* pCog);
void J3DAPI sithCogFunctionSound_GetSoundLen(SithCog* pCog);
void J3DAPI sithCogFunctionSound_WaitForSound(SithCog* pCog);

int J3DAPI sithCogFunctionSound_RegisterFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundThing, "playsoundthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopSoundThing, "stopsoundthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundPos, "playsoundpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundLocal, "playsoundlocal");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundGlobal, "playsoundglobal");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopSound, "stopsound");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_LoadSound, "loadsound");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlaySoundClass, "playsoundclass");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopSoundClass, "stopsoundclass");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_PlayVoiceMode, "playvoicemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_ChangeVolume, "changesoundvol");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_ChangePitch, "changesoundpitch");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_SectorSound, "sectorsound");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_GetSoundLen, "getsoundlen");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_WaitForSound, "waitforsound");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSound_StopAllSoundsThing, "stopallsoundsthing");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithCogFunctionSound_PlaySoundThing(SithCog* pCog)
{
    tSoundChannelHandle sndChannel;
    int guid = -1;

    SoundPlayFlag curPlayflags = sithCogExec_PopInt(pCog);
    float maximumRadius         = sithCogExec_PopFlex(pCog);
    float radius         = sithCogExec_PopFlex(pCog);
    float volume            = sithCogExec_PopFlex(pCog);
    SithThing* pObject       = sithCogExec_PopThing(pCog);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);

    if ( !hSnd )
    {
#line 162
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sound handle passed to PlaySoundThing().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Convert distances to IM units
    if ( radius < 0.0f )
    {
        radius = 0.5f;
    }
    else
    {
        radius = 0.1f * radius;
    }

    if ( maximumRadius < 0.0f )
    {
        maximumRadius = 2.5f;
    }
    else
    {
        maximumRadius = 0.1f * maximumRadius;
    }

    maximumRadius = maximumRadius > radius ? maximumRadius : radius;

    if ( pObject )
    {
        if ( (curPlayflags & SOUNDPLAY_THING_POS) != 0 )
        {
            curPlayflags &= ~SOUNDPLAY_ABSOLUTE_POS;
            sndChannel = sithSoundMixer_PlaySoundThing(hSnd, pObject, volume, radius, maximumRadius, curPlayflags);
        }
        else
        {
            curPlayflags |= SOUNDPLAY_ABSOLUTE_POS;
            sndChannel = sithSoundMixer_PlaySoundPos(hSnd, &pObject->pos, pObject->pInSector, volume, radius, maximumRadius, curPlayflags);
        }

        guid = sndChannel != SOUND_INVALIDHANDLE ? Sound_GetChannelGUID(sndChannel) : -1;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_PlaySound(pObject, &pObject->pos, hSnd, radius, maximumRadius, curPlayflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }
    else
    {

        curPlayflags &= ~(SOUNDPLAY_THING_POS | SOUNDPLAY_ABSOLUTE_POS);
        sndChannel = sithSoundMixer_PlaySound(hSnd, volume, 0.0f, curPlayflags);
        guid = sndChannel != SOUND_INVALIDHANDLE ? Sound_GetChannelGUID(sndChannel) : -1;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_PlaySound(NULL, NULL, hSnd, volume, 0.0f, curPlayflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_StopSoundThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    tSoundHandle hSnd = sithCogExec_PopSound(pCog);
    if ( !pThing )
    {
#line 246
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing reference in StopSoundThing.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithSoundMixer_StopSoundThing(pThing, hSnd);
}

void J3DAPI sithCogFunctionSound_StopAllSoundsThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 287
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing reference in StopSoundThing.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithSoundMixer_StopAllSoundsThing(pThing);
}

void J3DAPI sithCogFunctionSound_PlaySoundPos(SithCog* pCog)
{
    tSoundChannelHandle hChannel;
    int guid = -1;

    rdVector3 pos;
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float maxRadius         = sithCogExec_PopFlex(pCog);
    float minRadius         = sithCogExec_PopFlex(pCog);
    float  volume           = sithCogExec_PopFlex(pCog);
    int bPos                = sithCogExec_PopVector(pCog, &pos);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);

    if ( !hSnd )
    {
#line 340
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sound handle passed to PlaySoundPos().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !bPos )
    {
#line 347
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sound position vector passed to PlaySoundPos().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( minRadius < 0.0f )
    {
        minRadius = 0.5f;
    }
    else
    {
        minRadius = 0.1f * minRadius;
    }

    if ( maxRadius < 0.0f )
    {
        maxRadius = 2.5f;
    }
    else
    {
        maxRadius = 0.1f * maxRadius;
    }

    maxRadius = minRadius > maxRadius ? minRadius : maxRadius;

    playflags |= SOUNDPLAY_ABSOLUTE_POS;
    hChannel = sithSoundMixer_PlaySoundPos(hSnd, &pos, 0, volume, minRadius, maxRadius, playflags);

    guid = hChannel != SOUND_INVALIDHANDLE ? Sound_GetChannelGUID(hChannel) : -1;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PlaySound(NULL, &pos, hSnd, minRadius, maxRadius, playflags, guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_PlaySoundLocal(SithCog* pCog)
{
    tSoundChannelHandle hVoice;

    int bWaitForEnd         = sithCogExec_PopInt(pCog);
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float pan               = sithCogExec_PopFlex(pCog);
    float gain              = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);
    int chanGuid;

    if ( !hSnd )
    {
#line 410
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sound handle passed to PlaySoundLocal().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pan = STDMATH_CLAMP(pan, -1.0f, 1.0f);

    playflags &= ~(SOUNDPLAY_THING_POS | SOUNDPLAY_ABSOLUTE_POS);
    hVoice = sithSoundMixer_PlaySound(hSnd, gain, pan, playflags);
    if ( hVoice != SOUND_INVALIDHANDLE )
    {
        chanGuid = hVoice != SOUND_INVALIDHANDLE ? Sound_GetChannelGUID(hVoice) : -1;
        if ( bWaitForEnd )
        {
            pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
            pCog->statusParams[0] = chanGuid;
        }

        sithCogExec_PushInt(pCog, chanGuid);
    }
    else
    {
#line 434
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to play sound in PlaySoundLocal().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSound_PlaySoundGlobal(SithCog* pCog)
{
    tSoundChannelHandle hVoice;

    int bWaitForEnd         = sithCogExec_PopInt(pCog);
    SoundPlayFlag playflags = sithCogExec_PopInt(pCog);
    float pan               = sithCogExec_PopFlex(pCog);
    float gain              = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd       = sithCogExec_PopSound(pCog);
    int chanGuid;

    if ( !hSnd )
    {
#line 472
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sound handle passed to PlaySoundGlobal().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pan = STDMATH_CLAMP(pan, -1.0f, 1.0f);

    playflags &= ~(SOUNDPLAY_THING_POS | SOUNDPLAY_ABSOLUTE_POS);
    hVoice = sithSoundMixer_PlaySound(hSnd, gain, pan, playflags);

    chanGuid = hVoice != SOUND_INVALIDHANDLE ? Sound_GetChannelGUID(hVoice) : -1;

    if ( hVoice != SOUND_INVALIDHANDLE )
    {
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_PlaySound(0, 0, hSnd, gain, pan, playflags, chanGuid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }

        if ( bWaitForEnd )
        {
            pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
            pCog->statusParams[0] = chanGuid;
        }

        sithCogExec_PushInt(pCog, chanGuid);
    }
    else
    {
#line 503
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to play sound in PlaySoundLocal().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSound_StopSound(SithCog* pCog)
{
    float cursecFadeTime = sithCogExec_PopFlex(pCog);
    int guid          = sithCogExec_PopInt(pCog);

    tSoundChannelHandle hSndChannel = sithSoundMixer_GetChannelHandle(guid);
    if ( hSndChannel == SOUND_INVALIDHANDLE )
    {
#line 532
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "%s: Couldn't find sound instance in StopSound().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_StopSound(hSndChannel, cursecFadeTime, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    if ( cursecFadeTime <= 0.0f )
    {
        sithSoundMixer_StopSound(hSndChannel);
    }
    else
    {
        sithSoundMixer_FadeVolume(hSndChannel, 0.0f, cursecFadeTime);
    }
}

void J3DAPI sithCogFunctionSound_LoadSound(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    const char* pFilename = sithCogExec_PopString(pCog);

    tSoundHandle hSnd;
    if ( pFilename )
    {
        hSnd = sithSound_Load(pWorld, pFilename);
        if ( hSnd )
        {
            sithCogExec_PushInt(pCog, Sound_GetSoundIndex(hSnd));
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSound_PlaySoundClass(SithCog* pCog)
{
    int guid = -1;
    SithSoundClassMode mode = sithCogExec_PopInt(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pSoundClass )
    {
    }
    else
    {
        tSoundChannelHandle hChannel = sithSoundClass_PlayModeRandom(pThing, mode);
        guid = hChannel != SOUND_INVALIDHANDLE ? Sound_GetChannelGUID(hChannel) : -1;

        if ( hChannel != SOUND_INVALIDHANDLE && (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PlaySoundMode(pThing, mode, guid, -1.0f);
        }
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionSound_StopSoundClass(SithCog* pCog)
{
    SithSoundClassMode subMode = sithCogExec_PopInt(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pSoundClass )
    {
#line 639
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid THING for call to StopSoundClass()", 0, 0, 0, 0);
    }
    else
    {
        sithSoundClass_StopMode(pThing, subMode);
    }
}

void J3DAPI sithCogFunctionSound_PlayVoiceMode(SithCog* pCog)
{
    SithSoundClassMode subMode = sithCogExec_PopInt(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);
    sithSoundClass_PlayVoiceModeRandom(pThing, subMode);
}

void J3DAPI sithCogFunctionSound_ChangeVolume(SithCog* pCog)
{
    float cursecFadeTime               = sithCogExec_PopFlex(pCog);
    float volume                    = sithCogExec_PopFlex(pCog);
    int guid                        = sithCogExec_PopInt(pCog);
    tSoundChannelHandle sndChannel = sithSoundMixer_GetChannelHandle(guid);

    if ( sndChannel == SOUND_INVALIDHANDLE )
    {
#line 689
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No sound playing on channel %d in ChangeVolume().\n", pCog->aName, guid, 0, 0);
        return;
    }

    if ( cursecFadeTime <= 0.0f )
    {
#line 695
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad fade time %f in ChangePitch() (time must be > 0).\n", pCog->aName, cursecFadeTime, 0, 0);
        return;
    }

    sithSoundMixer_FadeVolume(sndChannel, volume, cursecFadeTime);
}

void J3DAPI sithCogFunctionSound_ChangePitch(SithCog* pCog)
{
    float cursecFadeTime               = sithCogExec_PopFlex(pCog);
    float curpitch                     = sithCogExec_PopFlex(pCog);
    int guid                        = sithCogExec_PopInt(pCog);
    tSoundChannelHandle hSndChan = sithSoundMixer_GetChannelHandle(guid);

    if ( hSndChan == SOUND_INVALIDHANDLE )
    {
#line 718
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No sound playing on channel %d in ChangePitch().\n", pCog->aName, guid, 0, 0);
        return;
    }

    if ( cursecFadeTime <= 0.0f )
    {
#line 724
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad fade time %f  in ChangePitch() (time must be > 0).\n", pCog->aName, cursecFadeTime, 0, 0);
        return;
    }

    sithSoundMixer_FadePitch(hSndChan, curpitch, cursecFadeTime);
}

void J3DAPI sithCogFunctionSound_SectorSound(SithCog* pCog)
{
    float volume        = sithCogExec_PopFlex(pCog);
    tSoundHandle hSnd   = sithCogExec_PopSound(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( !pSector )
    {
        goto error;
    }

    sithSoundMixer_SetSectorAmbientSound(pSector, hSnd, volume);
    return;

error:
#line 759
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad arguments in SectorSound verb.\n", 0, 0, 0, 0);
}

void J3DAPI sithCogFunctionSound_GetSoundLen(SithCog* pCog)
{
    int msecLen;
    tSoundHandle hCurSound = sithCogExec_PopSound(pCog);
    if ( !hCurSound )
    {
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    msecLen = Sound_GetLengthMsec(hCurSound);
    sithCogExec_PushFlex(pCog, (float)msecLen * 0.001f);
}

void J3DAPI sithCogFunctionSound_WaitForSound(SithCog* pCog)
{
    int channel;

#line 811
    SITH_ASSERTREL(pCog);

    channel = sithCogExec_PopInt(pCog);
    if ( !channel )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
    pCog->statusParams[0] = channel;
    sithCogExec_PushInt(pCog, channel);
}
