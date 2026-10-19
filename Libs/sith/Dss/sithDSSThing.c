#define RDVECTOR_NO_INLINE_LEN3
#include "sithDSSThing.h"
#include <string.h>

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPolyline.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithDSSThing_countfit_1, 30)
CF_PROTO(sithDSSThing_countfit_2, 30)
CF_PROTO(sithDSSThing_countfit_3, 30)
CF_PROTO(sithDSSThing_countfit_4, 30)
CF_PROTO(sithDSSThing_countfit_5, 30)
CF_PROTO(sithDSSThing_countfit_6, 30)
CF_PROTO(sithDSSThing_countfit_7, 30)
CF_PROTO(sithDSSThing_countfit_8, 30)
CF_PROTO(sithDSSThing_countfit_9, 30)
CF_PROTO(sithDSSThing_countfit_10, 30)
CF_PROTO(sithDSSThing_countfit_11, 30)
CF_PROTO(sithDSSThing_countfit_12, 30)
CF_PROTO(sithDSSThing_countfit_13, 30)
CF_PROTO(sithDSSThing_countfit_14, 30)
CF_PROTO(sithDSSThing_countfit_15, 30)
CF_PROTO(sithDSSThing_countfit_16, 30)
CF_PROTO(sithDSSThing_countfit_17, 30)
CF_PROTO(sithDSSThing_countfit_18, 30)
CF_PROTO(sithDSSThing_countfit_19, 30)
CF_PROTO(sithDSSThing_countfit_20, 30)
CF_PROTO(sithDSSThing_countfit_21, 30)
CF_PROTO(sithDSSThing_countfit_22, 30)
CF_PROTO(sithDSSThing_countfit_23, 30)
CF_PROTO(sithDSSThing_countfit_24, 30)
CF_PROTO(sithDSSThing_countfit_25, 30)
CF_PROTO(sithDSSThing_countfit_26, 30)
CF_PROTO(sithDSSThing_countfit_27, 30)
CF_PROTO(sithDSSThing_countfit_28, 10)
/* end count-fit */

int J3DAPI sithDSSThing_Pos(const SithThing* pThing, DPID toID, unsigned int dpFlags)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    rdVector3 curpyr;

    if ( !pThing || !pThing->type || !pThing->pInSector )
    {
#line 85
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Asked to send state for invalid thing pointer.\n", 0, 0, 0, 0);
        return 0;
    }

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHUINT16(pThing->attach.flags);

#line 95
    SITH_ASSERTREL(pThing->pInSector);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHVEC3(&pThing->pos);

    rdMatrix_ExtractAngles34(&pThing->orient, &curpyr);
    SITHDSS_PUSHVEC3(&curpyr);

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        SITHDSS_PUSHUINT32(pThing->moveInfo.physics.flags);
        SITHDSS_PUSHVEC3(&pThing->moveInfo.physics.velocity);
        SITHDSS_PUSHVEC3(&pThing->moveInfo.physics.angularVelocity);
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        SITHDSS_PUSHFLOAT(pThing->thingInfo.actorInfo.headPYR.x);
    }

    sithMulti_message.type = SITHDSS_POS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, toID, SITHMESSAGE_STREAM_ALL, dpFlags);
}

int J3DAPI sithDSSThing_ProcessPos(const SithMessage* pMsg)
{
    int index;
    SithThing* pThing;
    SithSector* pSector;
    SithAttachFlag attachFlags;
    rdVector3 loc;
    rdVector3 rotation;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    index = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(index);
    if ( !pThing )
    {
#line 166
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing guid %d in DSS::Pos.\n", index, 0, 0, 0);
        return 0;
    }

    if ( !pThing->type || !pThing->pInSector )
    {
#line 172
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing guid %d in DSS::Pos.\n", index, 0, 0, 0);
        return 0;
    }

    attachFlags = SITHDSS_POPUINT16();
    if ( !attachFlags && pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    index = SITHDSS_POPUINT16();
    pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, index);
    if ( !pSector )
    {
#line 187
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad sector index %d in DSS::Pos.\n", index, 0, 0, 0);
        return 0;
    }

    SITHDSS_POPVEC3(&loc);
    SITHDSS_POPVEC3(&rotation);
    rdMatrix_BuildRotate34(&pThing->orient, &rotation);

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.flags = SITHDSS_POPUINT32();
        SITHDSS_POPVEC3(&pThing->moveInfo.physics.velocity);
        SITHDSS_POPVEC3(&pThing->moveInfo.physics.angularVelocity);
        sithDSSThing_MoveToPos(pThing, &loc, pSector);
    }
    else
    {
        rdVector_Copy3(&pThing->pos, &loc);
        sithThing_SetSector(pThing, pSector, 0);
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        memset(&rotation, 0, sizeof(rotation));
        rotation.x = SITHDSS_POPFLOAT();
        sithActor_SetHeadPYR(pThing, &rotation);
    }

    return 1;
}

int J3DAPI sithDSSThing_MovePos(const SithThing* pThing, DPID idTo, unsigned int sendFlags)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    rdVector3 curpyr;

    if ( !pThing || !pThing->type || !pThing->pInSector )
    {
#line 232
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Asked to send state for invalid thing pointer.\n", 0, 0, 0, 0);
        return 0;
    }

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHVEC3(&pThing->pos);

    rdMatrix_ExtractAngles34(&pThing->orient, &curpyr);
    SITHDSS_PUSHVEC3(&curpyr);

    SITHDSS_PUSHINT32(pThing->moveStatus);

    sithMulti_message.type = SITHDSS_MOVEPOS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_ALL, sendFlags);
}

int J3DAPI sithDSSThing_ProcessMovePos(const SithMessage* pMsg)
{
    int guid;
    SithThing* pThing;
    unsigned int sectorIdx;
    SithSector* pSector;
    rdVector3 pos;
    rdVector3 pyr;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    guid = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
#line 271
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing guid %d in DSS::Pos.\n", guid, 0, 0, 0);
        return 0;
    }

    if ( !pThing->type || !pThing->pInSector )
    {
#line 277
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing guid %d in DSS::Pos.\n", guid, 0, 0, 0);
        return 0;
    }

    sectorIdx = SITHDSS_POPUINT16();
    pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, sectorIdx);
    if ( !pSector )
    {
#line 285
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad sector index %d in DSS::Pos.\n", sectorIdx, 0, 0, 0);
        return 0;
    }

    SITHDSS_POPVEC3(&pos);
    rdVector_Copy3(&pThing->pos, &pos);
    sithThing_SetSector(pThing, pSector, 0);

    SITHDSS_POPVEC3(&pyr);
    rdMatrix_BuildRotate34(&pThing->orient, &pyr);

    switch ( SITHDSS_POPINT32() )
    {
        case SITHPLAYERMOVE_PUSHING:
            break;

        case SITHPLAYERMOVE_PULLINGUP:
            sithPlayerActions_ClimbPullUp(pThing);
            break;

        case SITHPLAYERMOVE_PULLINGUP_1M:
            sithPlayerActions_ClimbOn1m(pThing);
            break;

        case SITHPLAYERMOVE_UNKNOWN_60:
            sithPlayerActions_JumpLeft(pThing);
            break;

        case SITHPLAYERMOVE_STRAFING_RIGHT:
            sithPlayerActions_StrafeRight(pThing);
            break;

        case SITHPLAYERMOVE_STRAFING_LEFT:
            sithPlayerActions_StrafeLeft(pThing);
            break;

        case SITHPLAYERMOVE_JUMPBACK:
            sithPlayerActions_HopBack(pThing);
            break;

        case SITHPLAYERMOVE_JUMP_READY:
            sithPlayerActions_JumpStart(pThing);
            break;
    }

    return 1;
}

int J3DAPI sithDSSThing_UpdateState(const SithThing* pThing, DPID toID, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    SithThingSwapEntry* pEntry = NULL;
    int i;

    if ( !pThing || !pThing->type || !pThing->pInSector || !sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing) )
    {
#line 368
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Asked to send state for invalid thing pointer.\n", 0, 0, 0, 0);
        return 0;
    }

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(pThing->unknownFlags);
    SITHDSS_PUSHUINT32(pThing->msecLifeLeft);

#line 380
    SITH_ASSERTREL(pThing->pInSector);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHINT16(pThing->collide.type);
    SITHDSS_PUSHVEC3(&pThing->pos);
    SITHDSS_PUSHUINT32(pThing->flags);

    if ( pThing->pPuppetClass )
    {
        SITHDSS_PUSHINT16(pThing->pPuppetState->armedMode);
        SITHDSS_PUSHINT16(pThing->pPuppetState->moveMode);
    }

    SITHDSS_PUSHVEC3((rdVector3*)&pThing->light.color);

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
            SITHDSS_PUSHUINT32(pThing->thingInfo.actorInfo.flags);
            break;

        case SITH_THING_WEAPON:
            SITHDSS_PUSHUINT32(pThing->thingInfo.weaponInfo.flags);
            break;

        case SITH_THING_ITEM:
            SITHDSS_PUSHUINT32(pThing->thingInfo.itemInfo.flags);
            if ( (pThing->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) != 0 )
            {
                SITHDSS_PUSHINT16(pThing->thingInfo.itemInfo.backpack.numItems);
                for ( i = 0; i < pThing->thingInfo.itemInfo.backpack.numItems; i++ )
                {
                    SITHDSS_PUSHINT16(pThing->thingInfo.itemInfo.backpack.aItems[i].typeId);
                    SITHDSS_PUSHFLOAT(pThing->thingInfo.itemInfo.backpack.aItems[i].amount);
                }
            }
            break;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        SITHDSS_PUSHUINT32(pThing->moveInfo.physics.flags);
    }

    if ( !pThing->pSwapList )
    {
        SITHDSS_PUSHINT16(-1);
    }
    else
    {
        i = 0;
        for ( pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry, i++ )
        {
        }

        SITHDSS_PUSHINT16(i);
        pEntry = pThing->pSwapList;
        while ( pEntry )
        {
            SITHDSS_PUSHINT32(pEntry->entryNum);
            SITHDSS_PUSHINT32(pEntry->meshNum);
            strncpy((char*)pCurDSSMsgData_, pEntry->pSrcModel->aName, 63);
            pCurDSSMsgData_[63] = 0;
            pCurDSSMsgData_ += 64;
            SITHDSS_PUSHINT32(pEntry->srcMeshNum);
            pEntry = pEntry->pNextEntry;
        }
    }

    sithMulti_message.type = SITHDSS_THINGSTATE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, toID, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessStateUpdate(const SithMessage* pMsg)
{
    int idx;
    SithThing* pThing;
    SithSector* pSector;
    SithThingFlag flags;
    unsigned int armedMode;
    SithPuppetMoveMode moveMode;
    int i;
    int16_t numEntries;
    int modelNum;
    int meshNum;
    int srcMeshNum;
    rdModel3* pModel;
    char aModelName[64];
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    idx = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(idx);
    if ( !pThing )
    {
#line 494
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing guid %x not found.\n", idx, 0, 0, 0);
        return 0;
    }

    if ( !pThing->type || !pThing->pInSector )
    {
#line 502
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning - got state update for non-existant thing index %d.\n", idx, 0, 0, 0);
        return 0;
    }

    pThing->unknownFlags = SITHDSS_POPINT32();
    pThing->msecLifeLeft = SITHDSS_POPUINT32();

    idx = SITHDSS_POPUINT16();
    pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, idx);
    if ( !pSector )
    {
#line 514
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad sector index %d in DSS::Pos.\n", idx, 0, 0, 0);
        return 0;
    }

    pThing->collide.type = SITHDSS_POPUINT16();
    SITHDSS_POPVEC3(&pThing->pos);
    sithThing_SetSector(pThing, pSector, 0);

    flags = SITHDSS_POPUINT32();
    if ( (pThing->flags & SITH_TF_REMOTE) != 0 )
    {
        flags |= SITH_TF_REMOTE;
    }
    else
    {
        flags &= ~SITH_TF_REMOTE;
    }

    pThing->flags = flags;

    if ( pThing->pPuppetClass )
    {
        armedMode = SITHDSS_POPUINT16();
        moveMode = SITHDSS_POPUINT16();
        sithPuppet_SetArmedMode(pThing, armedMode);
        sithPuppet_SetMoveMode(pThing, moveMode);
    }

    SITHDSS_POPVEC3((rdVector3*)&pThing->light.color);
    pThing->light.color.alpha = 0.0f;

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_CORPSE:
        case SITH_THING_PLAYER:
            pThing->thingInfo.actorInfo.flags = SITHDSS_POPUINT32();
            break;

        case SITH_THING_WEAPON:
            pThing->thingInfo.weaponInfo.flags = SITHDSS_POPUINT32();
            break;

        case SITH_THING_ITEM:
            pThing->thingInfo.itemInfo.flags = SITHDSS_POPUINT32();
            if ( (pThing->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) != 0 )
            {
                pThing->thingInfo.itemInfo.backpack.numItems = SITHDSS_POPINT16();
                for ( i = 0; i < pThing->thingInfo.itemInfo.backpack.numItems; i++ )
                {
                    pThing->thingInfo.itemInfo.backpack.aItems[i].typeId = SITHDSS_POPINT16();
                    pThing->thingInfo.itemInfo.backpack.aItems[i].amount = SITHDSS_POPFLOAT();
                }
            }
            break;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.flags = SITHDSS_POPUINT32();
    }

    if ( pThing->pSwapList )
    {
        sithThing_ResetSwapList(pThing);
    }

    numEntries = SITHDSS_POPINT16();
    if ( numEntries != -1 )
    {
        for ( i = 0; i < numEntries; i++ )
        {
            modelNum = SITHDSS_POPINT32();
            meshNum = SITHDSS_POPINT32();
            strncpy(aModelName, (const char*)pCurDSSMsgData_, 63);
            aModelName[63] = 0;
            pCurDSSMsgData_ += 64;
            srcMeshNum = SITHDSS_POPINT32();
            pModel = sithModel_Load(aModelName, 1);
            if ( pModel )
            {
                sithThing_AddSwapEntry(pThing, meshNum, pModel, srcMeshNum);
            }
        }
    }

    return 1;
}

int J3DAPI sithDSSThing_PlaySound(const SithThing* pThing, const rdVector3* pPos, tSoundHandle hSnd, float volume, float pan, SoundPlayFlag playflags, int guid, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 633
    SITH_ASSERTREL(hSnd);

    SITHDSS_PUSHUINT32(playflags);
    SITHDSS_PUSHFLOAT(volume);
    SITHDSS_PUSHFLOAT(pan);
    SITHDSS_PUSHUINT16(Sound_GetSoundIndex(hSnd));

    if ( (playflags & SOUNDPLAY_THING_POS) != 0 )
    {
#line 648
        SITH_ASSERTREL(pThing);
        SITHDSS_PUSHINT32(pThing->guid);
    }
    else if ( (playflags & SOUNDPLAY_ABSOLUTE_POS) != 0 )
    {
        SITHDSS_PUSHVEC3(pPos);
    }

    SITHDSS_PUSHINT32(guid);

    sithMulti_message.type = SITHDSS_PLAYSOUND;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0);
}

int J3DAPI sithDSSThing_ProcessPlaySound(const SithMessage* pMsg)
{
    SoundPlayFlag playflags;
    float minRadius;
    float maxRadius;
    int idx;
    tSoundHandle hSnd;
    SithThing* pThing;
    tSoundChannelHandle hChannel;
    int guid;
    rdVector3 pos;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    playflags = SITHDSS_POPUINT32();
    minRadius = SITHDSS_POPFLOAT();
    maxRadius = SITHDSS_POPFLOAT();
    idx = SITHDSS_POPUINT16();

    hSnd = Sound_GetSoundHandle(idx);
    if ( !hSnd )
    {
#line 704
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound index %d not valid in DSS::PlaySound.\n", idx, 0, 0, 0);
        return 0;
    }

    if ( (playflags & SOUNDPLAY_THING_POS) != 0 )
    {
        idx = SITHDSS_POPINT32();
        pThing = sithThing_GetGuidThing(idx);
        if ( !pThing )
        {
#line 714
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing guid %d not found in DSS::PlaySound (sound=%s)\n", idx, Sound_GetSoundFilename(hSnd), 0, 0);
            return 0;
        }

        hChannel = sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, minRadius, maxRadius, playflags);
    }
    else if ( (playflags & SOUNDPLAY_ABSOLUTE_POS) != 0 )
    {
        SITHDSS_POPVEC3(&pos);
        hChannel = sithSoundMixer_PlaySoundPos(hSnd, &pos, NULL, 1.0f, minRadius, maxRadius, playflags);
    }
    else
    {
        hChannel = sithSoundMixer_PlaySound(hSnd, minRadius, maxRadius, playflags);
    }

    guid = SITHDSS_POPINT32();
    return 1;
}

int J3DAPI sithDSSThing_PlaySoundMode(const SithThing* pThing, SithSoundClassMode mode, int channel, float a4)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 768
    SITH_ASSERTREL(pThing);

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(channel);
    SITHDSS_PUSHFLOAT(a4);
    SITHDSS_PUSHINT16(mode);

    sithMulti_message.type = SITHDSS_PLAYSOUNDMODE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, -1, SITHMESSAGE_STREAM_ALL, 0);
}

int J3DAPI sithDSSThing_ProcessPlaySoundMode(const SithMessage* pMsg)
{
    int guid;
    SithThing* pThing;
    int channel;
    float selectionRatio;
    SithSoundClassMode mode;
    tSoundChannelHandle hChannel;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    guid = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
#line 818
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing guid %d not found in DSS::PlaySoundMode\n", guid, 0, 0, 0);
        return 0;
    }

    channel = SITHDSS_POPINT32();
    selectionRatio = SITHDSS_POPFLOAT();
    mode = SITHDSS_POPUINT16();

    if ( selectionRatio < 0.0f )
    {
        hChannel = sithSoundClass_PlayModeRandom(pThing, mode);
    }
    else
    {
        hChannel = sithSoundClass_PlayMode(pThing, mode, selectionRatio);
    }

    return 1;
}

int J3DAPI sithDSSThing_PlayKey(const SithThing* pThing, const rdKeyframe* pKey, rdKeyframeFlags flags, int lo, unsigned int trackGUID, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 864
    SITH_ASSERTREL(pThing && pKey);

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(pKey->idx);
    SITHDSS_PUSHINT16(lo);
    SITHDSS_PUSHINT32(flags);
    SITHDSS_PUSHINT32(trackGUID);

    sithMulti_message.type = SITHDSS_PLAYKEY;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0);
}

int J3DAPI sithDSSThing_ProcessPlayKey(const SithMessage* pMsg)
{
    int index;
    SithThing* pThing;
    rdKeyframe* pKey;
    int bLowPriority;
    rdKeyframeFlags flags;
    unsigned int curTrackGUID;
    int trackNum;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    index = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(index);
    if ( !pThing )
    {
#line 920
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::PlayKey message for thing GUID %d - not found.\n", index, 0, 0, 0);
        return 0;
    }

    if ( !pThing->renderData.pPuppet )
    {
#line 927
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::PlayKey message for thing %s - no puppet.\n", pThing->aName, 0, 0, 0);
        return 0;
    }

    index = SITHDSS_POPINT32();
    pKey = sithPuppet_GetKeyframeByIndex(index);
    if ( !pKey )
    {
#line 935
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Key %d not found in DSS::PlayKey.\n", index, 0, 0, 0);
        return 0;
    }

    bLowPriority = SITHDSS_POPUINT16();
    flags = SITHDSS_POPINT32();
    curTrackGUID = SITHDSS_POPUINT32();

    trackNum = sithPuppet_PlayKey(pThing->renderData.pPuppet, pKey, bLowPriority, bLowPriority + 2, flags, NULL);
    if ( trackNum >= 0 )
    {
        pThing->renderData.pPuppet->aTracks[trackNum].guid = curTrackGUID;
    }

    return 1;
}

int J3DAPI sithDSSThing_PlayKeyMode(const SithThing* pThing, SithPuppetSubMode mode, unsigned int trackGUID, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 976
    SITH_ASSERTREL(pThing);

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(trackGUID);
    SITHDSS_PUSHINT16(mode);

    sithMulti_message.type = SITHDSS_PLAYKEYMODE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0);
}

int J3DAPI sithDSSThing_ProcessPlayKeyMode(const SithMessage* pMsg)
{
    int thingGuid;
    SithThing* pThing;
    unsigned int trackGUID;
    SithPuppetSubMode mode;
    int trackNum;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    thingGuid = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(thingGuid);
    if ( !pThing )
    {
#line 1024
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::PlayKey message for thing GUID %d - not found.\n", thingGuid, 0, 0, 0);
        return 0;
    }

    if ( !pThing->renderData.pPuppet )
    {
#line 1031
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::PlayKeyMode message for thing %s - no puppet.\n", pThing->aName, 0, 0, 0);
        return 0;
    }

    trackGUID = SITHDSS_POPUINT32();
    mode = SITHDSS_POPUINT16();

    trackNum = sithPuppet_PlayMode(pThing, mode, NULL);
    if ( trackNum >= 0 )
    {
        pThing->renderData.pPuppet->aTracks[trackNum].guid = trackGUID;
    }

    return 1;
}

int J3DAPI sithDSSThing_SetModel(const SithThing* pThing, DPID idTo)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 1055
    SITH_ASSERTREL(pThing);

    if ( !pThing || pThing->renderData.type != RD_THING_MODEL3 || !pThing->renderData.data.pModel3 )
    {
#line 1059
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Asked to send state for invalid thing pointer.\n", 0, 0, 0, 0);
        return 0;
    }

    SITHDSS_PUSHINT32(pThing->guid);

    strncpy((char*)pCurDSSMsgData_, pThing->renderData.data.pModel3->aName, 63);
    pCurDSSMsgData_[63] = 0;
    pCurDSSMsgData_ += 64;

    sithMulti_message.type = SITHDSS_SETMODEL;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessSetModel(const SithMessage* pMsg)
{
    int guid;
    SithThing* pThing;
    rdModel3* pModel;
    char paModelName[64];
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    guid = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
#line 1090
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::PlayKey message for thing GUID %d - not found.\n", guid, 0, 0, 0);
        return 0;
    }

    strncpy(paModelName, (const char*)pCurDSSMsgData_, 63);
    paModelName[63] = 0;
    pCurDSSMsgData_ += 64;

    pModel = sithModel_Load(paModelName, 1);
    if ( !pModel )
    {
        return 0;
    }

#line 1101
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Changing %s to use model %s.\n", pThing->aName, pModel->aName, 0, 0);
    sithThing_SetThingModel(pThing, pModel);
    return 1;
}

int J3DAPI sithDSSThing_StopKey(const SithThing* pThing, int trackGUID, float fadeTime, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 1113
    SITH_ASSERTREL(pThing);

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(trackGUID);
    SITHDSS_PUSHFLOAT(fadeTime);

    sithMulti_message.type = SITHDSS_STOPKEY;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessStopKey(const SithMessage* pMsg)
{
    int id;
    SithThing* pThing;
    unsigned int trackGUID;
    float fadeTime;
    size_t track;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    id = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(id);
    if ( !pThing || !pThing->renderData.pPuppet )
    {
#line 1147
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing guid %d in DSS::StopKey.\n", id, 0, 0, 0);
        return 0;
    }

    trackGUID = SITHDSS_POPUINT32();
    fadeTime = SITHDSS_POPFLOAT();

    for ( track = 0; track < RDPUPPET_MAX_TRACKS; track++ )
    {
        if ( pThing->renderData.pPuppet->aTracks[track].guid == trackGUID )
        {
            break;
        }
    }

    if ( track < RDPUPPET_MAX_TRACKS )
    {
        sithPuppet_StopKey(pThing->renderData.pPuppet, track, fadeTime);
    }
    else
    {
#line 1164
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Track with guid %x not found.\n", trackGUID, 0, 0, 0);
    }

    return 1;
}

int J3DAPI sithDSSThing_StopSound(tSoundChannelHandle hChannel, float secFadeTime, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    int guid;

    guid = hChannel ? Sound_GetChannelGUID(hChannel) : -1;
    SITHDSS_PUSHINT32(guid);
    SITHDSS_PUSHFLOAT(secFadeTime);

    sithMulti_message.type = SITHDSS_STOPSOUND;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessStopSound(const SithMessage* pMsg)
{
    int guid;
    float cursecFadeTime;
    tSoundChannelHandle hwndCurrentChannel;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    guid = SITHDSS_POPINT32();
    cursecFadeTime = SITHDSS_POPFLOAT();

    hwndCurrentChannel = sithSoundMixer_GetChannelHandle(guid);
    if ( hwndCurrentChannel )
    {
        if ( cursecFadeTime <= 0.0f )
        {
            sithSoundMixer_StopSound(hwndCurrentChannel);
        }
        else
        {
            sithSoundMixer_FadeVolume(hwndCurrentChannel, 0.0f, cursecFadeTime);
        }
    }
    else
    {
#line 1214
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not find a channel with GUID %d.\n", guid, 0, 0, 0);
    }

    return 1;
}

int J3DAPI sithDSSThing_Fire(const SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* fireDir, const rdVector3* firePos, tSoundHandle hFireSnd, SithPuppetSubMode puppetSubmode, float extra, SithFireProjectileFlags projectileFlags, float sedRapidFireTime, int projectileGUID, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 1241
    SITH_ASSERTREL(pShooter && fireDir && firePos);

    SITHDSS_PUSHINT32(pShooter->guid);
    SITHDSS_PUSHINT16(projectileFlags);
    SITHDSS_PUSHINT16(sithTemplate_GetTemplateIndex(pProjectileTemplate));
    if ( hFireSnd )
    {
        SITHDSS_PUSHINT16(Sound_GetSoundIndex(hFireSnd));
    }
    else
    {
        SITHDSS_PUSHINT16(-1);
    }

    SITHDSS_PUSHINT16(puppetSubmode);
    SITHDSS_PUSHVEC3(firePos);
    SITHDSS_PUSHVEC3(fireDir);
    SITHDSS_PUSHFLOAT(extra);
    SITHDSS_PUSHFLOAT(sedRapidFireTime);
    SITHDSS_PUSHINT32(projectileGUID);

    sithMulti_message.type = SITHDSS_FIRE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0);
}

int J3DAPI sithDSSThing_ProcessFire(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    SithThing* pTemplate = NULL;
    tSoundHandle hSnd = NULL;
    int idx;
    SithThing* pShooter;
    SithFireProjectileFlags flags;
    SithPuppetSubMode submode;
    rdVector3 firePos;
    rdVector3 fireDir;
    float extra;
    float secDeltaTime;
    int projectileGUID;
    SithThing* pProjectile;

    idx = SITHDSS_POPINT32();
    pShooter = sithThing_GetGuidThing(idx);
    if ( !pShooter )
    {
#line 1313
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad shooter guid %d in DSS:Fire.\n", idx, 0, 0, 0);
        return 0;
    }

    flags = SITHDSS_POPUINT16();
    idx = SITHDSS_POPUINT16();
    pTemplate = sithTemplate_GetTemplateByIndex(idx);
    idx = SITHDSS_POPUINT16();
    hSnd = Sound_GetSoundHandle(idx);
    submode = SITHDSS_POPUINT16();
    SITHDSS_POPVEC3(&firePos);
    SITHDSS_POPVEC3(&fireDir);
    extra = SITHDSS_POPFLOAT();
    secDeltaTime = SITHDSS_POPFLOAT();
    projectileGUID = SITHDSS_POPINT32();

    pProjectile = sithWeapon_WeaponFireProjectile(pShooter, pTemplate, &fireDir, &firePos, hSnd, submode, extra, flags, secDeltaTime);
    if ( pProjectile )
    {
        pProjectile->flags |= SITH_TF_REMOTE;
        pProjectile->guid = projectileGUID;
    }

    return 1;
}

int J3DAPI sithDSSThing_Death(const SithThing* pThing, const SithThing* pKiller, char bKillPlayer, DPID to, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHINT32(pThing->guid);
    if ( pKiller )
    {
        SITHDSS_PUSHINT32(pKiller->guid);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITHDSS_PUSHUINT8(bKillPlayer);

    sithMulti_message.type = SITHDSS_DEATH;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, to, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessDeath(const SithMessage* pMsg)
{
    int curguid;
    int killerGuid;
    int bKillPlayer;
    SithThing* pCurThing;
    SithThing* pKiller;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1411
    SITH_ASSERTREL(pMsg);

    curguid = SITHDSS_POPINT32();
    killerGuid = SITHDSS_POPINT32();
    bKillPlayer = SITHDSS_POPUINT8();

    pCurThing = sithThing_GetGuidThing(curguid);
    if ( !pCurThing )
    {
#line 1420
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::Death message for thing guid %d I don't have locally.\n", curguid, 0, 0, 0);
        return 0;
    }

    pKiller = sithThing_GetGuidThing(killerGuid);
    switch ( pCurThing->type )
    {
        case SITH_THING_ACTOR:
            sithActor_KillActor(pCurThing, pKiller, 0);
            break;

        case SITH_THING_PLAYER:
            if ( bKillPlayer == 1 )
            {
                sithPlayer_KillPlayer(pCurThing);
            }
            else
            {
                sithActor_KillActor(pCurThing, pKiller, 0);
            }
            break;

        case SITH_THING_FREE:
#line 1441
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got death message from unused thing %d.\n", pCurThing->idx, 0, 0, 0);

        default:
#line 1444
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got death message for unhandled thing type %d, index %d.\n", pCurThing->type, pCurThing->idx, 0, 0);
            break;
    }

    return 1;
}

int J3DAPI sithDSSThing_DamageThing(const SithThing* pVictim, const SithThing* pPurpetrator, float damage, SithDamageType hitType, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 1456
    SITH_ASSERTREL(pVictim);

    SITHDSS_PUSHINT32(pVictim->guid);
    if ( pPurpetrator )
    {
        SITHDSS_PUSHINT32(pPurpetrator->guid);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITHDSS_PUSHFLOAT(damage);
    SITHDSS_PUSHINT16(hitType);

    sithMulti_message.type = SITHDSS_DAMAGE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessDamage(const SithMessage* pMsg)
{
    int guid;
    int purpGuid;
    SithThing* pThing;
    SithThing* pPurpetrator;
    float damage;
    SithDamageType hitType;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1496
    SITH_ASSERTREL(pMsg);

    if ( sithMessage_GetSenderID() != sithMulti_server.idServer )
    {
        return 0;
    }

    guid = SITHDSS_POPINT32();
    purpGuid = SITHDSS_POPINT32();

    pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
#line 1507
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got DSS::Death message for thing guid %d I don't have locally.\n", guid, 0, 0, 0);
        return 0;
    }

    pPurpetrator = sithThing_GetGuidThing(purpGuid);
    if ( !pPurpetrator )
    {
        pPurpetrator = pThing;
    }

    damage = SITHDSS_POPFLOAT();
    hitType = SITHDSS_POPUINT16();
    sithThing_DamageThing(pThing, pPurpetrator, damage, hitType);
    return 1;
}

int J3DAPI sithDSSThing_FullDescription(const SithThing* pThing, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t i;
    SithThingSwapEntry* pEntry = NULL;
    rdPolyline* pPolyline;
    const SithActorInfo* pActorInfo;
    const SithWeaponInfo* pWeaponInfo;
    const SithExplosionInfo* pExplosionInfo;
    const SithSpriteInfo* pSpriteInfo;
    const SithPhysicsInfo* pPhysics;
    const SithPathMoveInfo* pPath;

    SITHDSS_PUSHINT16(pThing->idx);
    SITHDSS_PUSHINT16(pThing->type);

    if ( !pThing->type )
    {
        // Empty arm: the debug build jumps from here straight to the message tail (jne/jmp)
    }
    else
    {
        SITHDSS_PUSHINT16(sithTemplate_GetTemplateIndex(pThing->pTemplate));
        SITHDSS_PUSHUINT32(pThing->signature);
        SITHDSS_PUSHINT32(pThing->guid);
        SITHDSS_PUSHVEC3(&pThing->pos);
        SITHDSS_PUSHVEC3(&pThing->forceMoveStartPos);
        SITHDSS_PUSHVEC3(&pThing->orient.rvec);
        SITHDSS_PUSHVEC3(&pThing->orient.lvec);
        SITHDSS_PUSHVEC3(&pThing->orient.uvec);
        SITHDSS_PUSHINT32(pThing->moveStatus);
        SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pThing->pInSector));
        SITHDSS_PUSHUINT32(pThing->flags);
        SITHDSS_PUSHUINT32(pThing->msecLifeLeft);
        SITHDSS_PUSHUINT32(pThing->msecTimerTime);
        SITHDSS_PUSHUINT32(pThing->msecNextPulseTime);
        SITHDSS_PUSHUINT32(pThing->msecPulseInterval);
        SITHDSS_PUSHFLOAT(pThing->userval);
        SITHDSS_PUSHINT16(pThing->collide.type);
        SITHDSS_PUSHFLOAT(pThing->collide.size);
        SITHDSS_PUSHFLOAT(pThing->collide.movesize);
        SITHDSS_PUSHVEC4(&pThing->light.color);
        SITHDSS_PUSHFLOAT(pThing->light.minRadius);
        SITHDSS_PUSHFLOAT(pThing->light.maxRadius);
        SITHDSS_PUSHINT32(pThing->unknownFlags);

        if ( (pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            if ( pThing->pCog )
            {
                SITHDSS_PUSHINT16(pThing->pCog->idx);
            }
            else
            {
                SITHDSS_PUSHINT16(-1);
            }

            if ( pThing->pCaptureCog )
            {
                SITHDSS_PUSHINT16(pThing->pCaptureCog->idx);
            }
            else
            {
                SITHDSS_PUSHINT16(-1);
            }
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_CORPSE:
            case SITH_THING_PLAYER:
                pActorInfo = &pThing->thingInfo.actorInfo;
                SITHDSS_PUSHUINT32(pActorInfo->flags);
                SITHDSS_PUSHFLOAT(pActorInfo->health);
                SITHDSS_PUSHFLOAT(pActorInfo->maxHealth);
                SITHDSS_PUSHFLOAT(pActorInfo->extraSpeed);
                SITHDSS_PUSHVEC3(&pActorInfo->headPYR);
                SITHDSS_PUSHVEC3((rdVector3*)&pActorInfo->headLightIntensity);
                SITHDSS_PUSHINT32(pActorInfo->aiState);
                SITHDSS_PUSHINT32(pActorInfo->aiArmedModeState);
                SITHDSS_PUSHUINT8(pActorInfo->bForceMovePlay);
                SITHDSS_PUSHUINT8(pActorInfo->bControlsDisabled);
                SITHDSS_PUSHUINT32(pActorInfo->endurance.msecUnderwater);
                SITHDSS_PUSHFLOAT(pActorInfo->maxThrust);
                SITHDSS_PUSHFLOAT(pActorInfo->maxRotVelocity);
                SITHDSS_PUSHFLOAT(pActorInfo->minHeadPitch);
                SITHDSS_PUSHFLOAT(pActorInfo->maxHeadPitch);
                SITHDSS_PUSHFLOAT(pActorInfo->maxHeadYaw);
                SITHDSS_PUSHFLOAT(pActorInfo->maxHeadVelocity);
                SITHDSS_PUSHINT16(sithThing_GetThingIndex(pActorInfo->pThingMeshAttached));
                SITHDSS_PUSHINT32(pActorInfo->attachMeshNum);
                if ( pActorInfo->pPlayer )
                {
                    SITHDSS_PUSHINT16(pActorInfo->pPlayer - sithPlayer_aPlayers);
                }
                else
                {
                    SITHDSS_PUSHINT16(-1);
                }
                break;

            case SITH_THING_WEAPON:
                pWeaponInfo = &pThing->thingInfo.weaponInfo;
                SITHDSS_PUSHUINT32(pWeaponInfo->flags);
                SITHDSS_PUSHINT16(pWeaponInfo->numRicochets);
                break;

            case SITH_THING_EXPLOSION:
                pExplosionInfo = &pThing->thingInfo.explosionInfo;
                SITHDSS_PUSHUINT32(pExplosionInfo->flags);
                break;

            case SITH_THING_SPRITE:
                pSpriteInfo = &pThing->thingInfo.spriteInfo;
                SITHDSS_PUSHFLOAT(pSpriteInfo->width);
                SITHDSS_PUSHFLOAT(pSpriteInfo->height);
                SITHDSS_PUSHFLOAT(pSpriteInfo->alpha);
                SITHDSS_PUSHFLOAT(pSpriteInfo->rollAngle);
                SITHDSS_PUSHINT16(sithThing_GetThingIndex(pSpriteInfo->pThingMeshAttached));
                SITHDSS_PUSHINT32(pSpriteInfo->attachMeshNum);
                break;

            case SITH_THING_POLYLINE:
#line 1674
                SITH_ASSERTREL(pThing->renderData.data.pPolyline);
                pPolyline = pThing->renderData.data.pPolyline;
                if ( !strcmp(pThing->aName, "+plcogend") )
                {
                    SITHDSS_PUSHINT16(sithThing_GetThingIndex(pThing->pParent));
                    pPolyline->face.extraLight.alpha = pThing->alpha;
                }

                strncpy((char*)pCurDSSMsgData_, pPolyline->face.pMaterial->aName, 63);
                pCurDSSMsgData_[63] = 0;
                pCurDSSMsgData_ += 64;

                SITHDSS_PUSHFLOAT(pPolyline->length);
                SITHDSS_PUSHFLOAT(pPolyline->baseRadius);
                SITHDSS_PUSHFLOAT(pPolyline->tipRadius);
                SITHDSS_PUSHINT16(pPolyline->geoMode);
                SITHDSS_PUSHINT16(pPolyline->lightMode);
                SITHDSS_PUSHVEC4(&pPolyline->face.extraLight);
                break;
        }

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                pPhysics = &pThing->moveInfo.physics;
                SITHDSS_PUSHUINT32(pPhysics->flags);
                SITHDSS_PUSHVEC3(&pPhysics->velocity);
                SITHDSS_PUSHVEC3(&pPhysics->angularVelocity);
                SITHDSS_PUSHVEC3(&pPhysics->thrust);
                SITHDSS_PUSHVEC3(&pPhysics->rotThrust);
                SITHDSS_PUSHFLOAT(pPhysics->maxRotationVelocity);
                SITHDSS_PUSHFLOAT(pPhysics->maxVelocity);
                SITHDSS_PUSHFLOAT(pPhysics->orientSpeed);
                SITHDSS_PUSHFLOAT(pPhysics->buoyancy);
                SITHDSS_PUSHFLOAT(pPhysics->height);
                SITHDSS_PUSHFLOAT(pPhysics->mass);
                break;

            case SITH_MT_PATH:
                pPath = &pThing->moveInfo.pathMovement;
                SITHDSS_PUSHINT16(pPath->mode);
                SITHDSS_PUSHVEC3(&pPath->vecDeltaPos);
                SITHDSS_PUSHFLOAT(pPath->moveTimeRemaining);
                SITHDSS_PUSHFLOAT(pPath->moveVel);
                SITHDSS_PUSHFLOAT(pPath->rotDeltaTime);
                SITHDSS_PUSHVEC3(&pPath->rotOffset);
                SITHDSS_PUSHVEC3(&pPath->goalPYR);
                SITHDSS_PUSHFLOAT(pPath->rotDelta);
                SITHDSS_PUSHINT16(pPath->numBlockedMoves);
                SITHDSS_PUSHINT16(pPath->currentFrame);
                SITHDSS_PUSHINT16(pPath->nextFrame);
                SITHDSS_PUSHINT16(pPath->goalFrame);
                SITHDSS_PUSHVEC3(&pPath->curOrient.rvec);
                SITHDSS_PUSHVEC3(&pPath->curOrient.lvec);
                SITHDSS_PUSHVEC3(&pPath->curOrient.uvec);
                SITHDSS_PUSHVEC3(&pPath->curOrient.dvec);
                SITHDSS_PUSHVEC3(&pPath->rotateToPYR);
                SITHDSS_PUSHINT16(pPath->numFrames);
                for ( i = 0; i < pPath->numFrames; i++ )
                {
                    SITHDSS_PUSHVEC3(&pPath->aFrames[i].pos);
                    SITHDSS_PUSHVEC3(&pPath->aFrames[i].pyr);
                }
                break;
        }

        if ( !pThing->pSwapList )
        {
            SITHDSS_PUSHINT16(-1);
        }
        else
        {
            for ( i = 0, pEntry = pThing->pSwapList; pEntry; pEntry = pEntry->pNextEntry, i++ )
            {
                // Counts the entries: the debug build steps and counts in the loop's increment
            }

            SITHDSS_PUSHINT16(i);
            pEntry = pThing->pSwapList;
            while ( pEntry )
            {
                SITHDSS_PUSHINT32(pEntry->entryNum);
                SITHDSS_PUSHINT32(pEntry->meshNum);
                SITHDSS_PUSHINT16(sithModel_GetModelIndex(pEntry->pSrcModel));
                SITHDSS_PUSHINT32(pEntry->srcMeshNum);
                pEntry = pEntry->pNextEntry;
            }
        }

        SITHDSS_PUSHUINT32(pThing->numSwapEntries);

        if ( pThing->renderData.data.pModel3 )
        {
            SITHDSS_PUSHINT16(sithModel_GetModelIndex(pThing->renderData.data.pModel3));
            SITHDSS_PUSHVEC3(&pThing->renderData.data.pModel3->insertOffset);
        }
        else
        {
            SITHDSS_PUSHINT16(-1);
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
            {
                const SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
                const SithActorWeaponInfo* pWeaponInfo = &pActorInfo->weaponInfo;
                const SithActorVoiceInfo* pVoiceInfo = &pActorInfo->voiceInfo;

                *(*(float**)&pCurDSSMsgData_)++ = pWeaponInfo->secWeaponActivationStartTime;
                SITHDSS_PUSHFLOAT(pWeaponInfo->secTimeLastRapidFired);
                SITHDSS_PUSHFLOAT(pWeaponInfo->secWeaponActivationWaitEndTime);
                SITHDSS_PUSHFLOAT(pWeaponInfo->secWeaponActivationWaitTime);
                SITHDSS_PUSHFLOAT(pWeaponInfo->secAimWaitEndTime);
                SITHDSS_PUSHFLOAT(pWeaponInfo->secWeaponSwapTime);
                SITHDSS_PUSHINT32(pWeaponInfo->selectedWeaponID);
                SITHDSS_PUSHINT32(pWeaponInfo->curWeaponID);
                SITHDSS_PUSHINT32(pWeaponInfo->deselectedWeaponID);
                SITHDSS_PUSHINT32(pWeaponInfo->weaponSwapRefNum);
                SITHDSS_PUSHVEC3(&pWeaponInfo->vecUnknown0);
                SITHDSS_PUSHVEC3(&pActorInfo->fireOffset);
                SITHDSS_PUSHVEC4(&pVoiceInfo->voiceColor.top);
                SITHDSS_PUSHINT32(pVoiceInfo->hSndChannel);
                SITHDSS_PUSHINT32(pVoiceInfo->voiceHeadInfo.headMeshNum);
                SITHDSS_PUSHINT32(pVoiceInfo->voiceHeadInfo.headSwapRefNum);
                for ( i = 0; i < STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels); i++ )
                {
                    SITHDSS_PUSHINT16(sithModel_GetModelIndex(pVoiceInfo->voiceHeadInfo.apSoundHeadModels[i]));
                }
                break;
            }
        }

        SITHDSS_PUSHFLOAT(pThing->alpha);
    }

    sithMulti_message.type = SITHDSS_THINGFULLDESC;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessFullDescription(const SithMessage* pMsg)
{
    SithWorld* pWorld;
    SithThingType type;
    SithThing* pThing;
    SithThing* pTemplate;
    SithSector* pSector;
    int idx;
    char aMatName[64];
    float length;
    float baseRadius;
    float tipRadius;
    rdGeometryMode geoMode;
    rdLightMode lightMode;
    rdVector4 color;
    rdPolyline* pPolyline;
    size_t i;
    int16_t numEntries;
    int count;
    int entryNum;
    int meshNum;
    rdModel3* pModel;
    int srcMeshNum;
    int16_t modelIdx;
    int16_t data16;
    SithActorInfo* pActorInfo;
    SithPhysicsInfo* pPhysics;
    SithPathMoveInfo* pPath;
    SithWeaponInfo* pWeaponInfo;
    SithExplosionInfo* pExplosionInfo;
    SithSpriteInfo* pSpriteInfo;
    float alpha;
    int result;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    pWorld = sithWorld_pCurrentWorld;
    result = 0;
#line 1863
    SITH_ASSERTREL(pMsg);

    if ( stdComm_IsGameActive() && (sith_mode.subModeFlags & 8) == 0 )
    {
#line 1867
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Received DSS::ThingFullDesc message when not in sync mode.\n", 0, 0, 0, 0);
        return 0;
    }

    idx = SITHDSS_POPUINT16();
    if ( idx < 0 || idx >= (int)sithWorld_pCurrentWorld->numThings )
    {
#line 1877
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "FullDescription received for thing id %d, out of range.\n", idx, 0, 0, 0);
        return 0;
    }

    {
        pThing = &sithWorld_pCurrentWorld->aThings[idx];
        if ( pThing->type )
        {
            sithThing_RemoveThing(pWorld, pThing);
        }

        pWorld->lastThingIdx = J3DMAX(pWorld->lastThingIdx, idx);

        type = SITHDSS_POPUINT16();
        if ( !type )
        {
            return 1;
        }

        pThing = &pWorld->aThings[idx];
        sithThing_Reset(pThing);

        idx = SITHDSS_POPUINT16();
        pTemplate = sithTemplate_GetTemplateByIndex(idx);
        if ( pTemplate )
        {
            sithThing_SetThingBasedOn(pThing, &pWorld->aThingTemplates[idx]);
        }

        pThing->type = type;
        pThing->signature = SITHDSS_POPUINT32();
        pThing->guid = SITHDSS_POPINT32();
        SITHDSS_POPVEC3(&pThing->pos);
        SITHDSS_POPVEC3(&pThing->forceMoveStartPos);
        SITHDSS_POPVEC3(&pThing->orient.rvec);
        SITHDSS_POPVEC3(&pThing->orient.lvec);
        SITHDSS_POPVEC3(&pThing->orient.uvec);
        pThing->moveStatus = SITHDSS_POPINT32();

        idx = SITHDSS_POPUINT16();
        pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, idx);
        if ( pSector )
        {
            sithThing_SetSector(pThing, pSector, 1);
        }

        pThing->flags = SITHDSS_POPUINT32();
        pThing->msecLifeLeft = SITHDSS_POPUINT32();
        pThing->msecTimerTime = SITHDSS_POPUINT32();
        pThing->msecNextPulseTime = SITHDSS_POPUINT32();
        pThing->msecPulseInterval = SITHDSS_POPUINT32();
        pThing->userval = SITHDSS_POPFLOAT();
        pThing->collide.type = SITHDSS_POPUINT16();
        pThing->collide.size = SITHDSS_POPFLOAT();
        pThing->collide.movesize = SITHDSS_POPFLOAT();
        SITHDSS_POPVEC4(&pThing->light.color);
        pThing->light.minRadius = SITHDSS_POPFLOAT();
        pThing->light.maxRadius = SITHDSS_POPFLOAT();
        pThing->unknownFlags = SITHDSS_POPINT32();

        if ( (pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            idx = SITHDSS_POPUINT16();
            if ( idx >= 0 )
            {
                pThing->pCog = sithCog_GetCogByIndex(idx);
            }

            idx = SITHDSS_POPUINT16();
            if ( idx >= 0 )
            {
                pThing->pCaptureCog = sithCog_GetCogByIndex(idx);
            }
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_CORPSE:
            case SITH_THING_PLAYER:
                pActorInfo = &pThing->thingInfo.actorInfo;
                pActorInfo->flags = SITHDSS_POPUINT32();
                pActorInfo->health = SITHDSS_POPFLOAT();
                pActorInfo->maxHealth = SITHDSS_POPFLOAT();
                pActorInfo->extraSpeed = SITHDSS_POPFLOAT();
                SITHDSS_POPVEC3(&pActorInfo->headPYR);
                SITHDSS_POPVEC3((rdVector3*)&pActorInfo->headLightIntensity);
                pActorInfo->headLightIntensity.alpha = 0.0f;
                pActorInfo->aiState = SITHDSS_POPINT32();
                pActorInfo->aiArmedModeState = SITHDSS_POPINT32();
                pActorInfo->bForceMovePlay = SITHDSS_POPUINT8();
                pActorInfo->bControlsDisabled = SITHDSS_POPUINT8();
                pActorInfo->endurance.msecUnderwater = SITHDSS_POPUINT32();
                pActorInfo->maxThrust = SITHDSS_POPFLOAT();
                pActorInfo->maxRotVelocity = SITHDSS_POPFLOAT();
                pActorInfo->minHeadPitch = SITHDSS_POPFLOAT();
                pActorInfo->maxHeadPitch = SITHDSS_POPFLOAT();
                pActorInfo->maxHeadYaw = SITHDSS_POPFLOAT();
                pActorInfo->maxHeadVelocity = SITHDSS_POPFLOAT();

                idx = SITHDSS_POPUINT16();
                if ( idx >= 0 && idx < (int)sithWorld_pCurrentWorld->numThings )
                {
                    pActorInfo->pThingMeshAttached = &sithWorld_pCurrentWorld->aThings[idx];
                }

                pActorInfo->attachMeshNum = SITHDSS_POPINT32();

                idx = SITHDSS_POPUINT16();
                if ( idx >= 0 && idx < SITHPLAYER_MAX_PLAYERS )
                {
                    pActorInfo->pPlayer = &sithPlayer_aPlayers[idx];
                }
                break;

            case SITH_THING_WEAPON:
                pWeaponInfo = &pThing->thingInfo.weaponInfo;
                pWeaponInfo->flags = SITHDSS_POPUINT32();
                pWeaponInfo->numRicochets = SITHDSS_POPUINT16();
                break;

            case SITH_THING_EXPLOSION:
                pExplosionInfo = &pThing->thingInfo.explosionInfo;
                pExplosionInfo->flags = SITHDSS_POPUINT32();
                break;

            case SITH_THING_SPRITE:
                pSpriteInfo = &pThing->thingInfo.spriteInfo;
                pSpriteInfo->width = SITHDSS_POPFLOAT();
                pSpriteInfo->height = SITHDSS_POPFLOAT();
                pSpriteInfo->alpha = SITHDSS_POPFLOAT();
                pSpriteInfo->rollAngle = SITHDSS_POPFLOAT();

                idx = SITHDSS_POPUINT16();
                if ( idx >= 0 && idx < (int)sithWorld_pCurrentWorld->numThings )
                {
                    pSpriteInfo->pThingMeshAttached = &sithWorld_pCurrentWorld->aThings[idx];
                }

                pSpriteInfo->attachMeshNum = SITHDSS_POPINT32();
                break;

            case SITH_THING_POLYLINE:
                if ( !strcmp(pThing->aName, "+plcogend") )
                {
                    idx = SITHDSS_POPUINT16();
                    if ( idx >= 0 && idx < (int)sithWorld_pCurrentWorld->numThings )
                    {
                        pThing->pParent = &sithWorld_pCurrentWorld->aThings[idx];
                    }
                }

                strncpy(aMatName, (const char*)pCurDSSMsgData_, 63);
                aMatName[63] = 0;
                pCurDSSMsgData_ += 64;

                length = SITHDSS_POPFLOAT();
                baseRadius = SITHDSS_POPFLOAT();
                tipRadius = SITHDSS_POPFLOAT();
                geoMode = SITHDSS_POPUINT16();
                lightMode = SITHDSS_POPUINT16();
                SITHDSS_POPVEC4(&color);

                pPolyline = rdPolyline_New(pThing->aName, aMatName, aMatName, length, baseRadius, tipRadius, geoMode, lightMode, &color);
                if ( pPolyline )
                {
                    rdThing_NewEntry(&pThing->renderData, NULL);
                    rdThing_SetPolyline(&pThing->renderData, pPolyline);
                }

                if ( !strcmp(pThing->aName, "+plcogend") )
                {
                    pThing->alpha = color.alpha;
                }
                break;
        }

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                pPhysics = &pThing->moveInfo.physics;
                pPhysics->flags = SITHDSS_POPUINT32();
                SITHDSS_POPVEC3(&pPhysics->velocity);
                SITHDSS_POPVEC3(&pPhysics->angularVelocity);
                SITHDSS_POPVEC3(&pPhysics->thrust);
                SITHDSS_POPVEC3(&pPhysics->rotThrust);
                pPhysics->maxRotationVelocity = SITHDSS_POPFLOAT();
                pPhysics->maxVelocity = SITHDSS_POPFLOAT();
                pPhysics->orientSpeed = SITHDSS_POPFLOAT();
                pPhysics->buoyancy = SITHDSS_POPFLOAT();
                pPhysics->height = SITHDSS_POPFLOAT();
                pPhysics->mass = SITHDSS_POPFLOAT();
                break;

            case SITH_MT_PATH:
                pPath = &pThing->moveInfo.pathMovement;
                data16 = *(int16_t*)pCurDSSMsgData_;
                pCurDSSMsgData_ += sizeof(int16_t);
                pPath->mode = data16;
                SITHDSS_POPVEC3(&pPath->vecDeltaPos);
                pPath->moveTimeRemaining = SITHDSS_POPFLOAT();
                pPath->moveVel = SITHDSS_POPFLOAT();
                pPath->rotDeltaTime = SITHDSS_POPFLOAT();
                SITHDSS_POPVEC3(&pPath->rotOffset);
                SITHDSS_POPVEC3(&pPath->goalPYR);
                pPath->rotDelta = SITHDSS_POPFLOAT();
                data16 = *(int16_t*)pCurDSSMsgData_;
                pCurDSSMsgData_ += sizeof(int16_t);
                pPath->numBlockedMoves = data16;
                data16 = *(int16_t*)pCurDSSMsgData_;
                pCurDSSMsgData_ += sizeof(int16_t);
                pPath->currentFrame = data16;
                data16 = *(int16_t*)pCurDSSMsgData_;
                pCurDSSMsgData_ += sizeof(int16_t);
                pPath->nextFrame = data16;
                data16 = *(int16_t*)pCurDSSMsgData_;
                pCurDSSMsgData_ += sizeof(int16_t);
                pPath->goalFrame = data16;
                SITHDSS_POPVEC3(&pPath->curOrient.rvec);
                SITHDSS_POPVEC3(&pPath->curOrient.lvec);
                SITHDSS_POPVEC3(&pPath->curOrient.uvec);
                SITHDSS_POPVEC3(&pPath->curOrient.dvec);
                SITHDSS_POPVEC3(&pPath->rotateToPYR);

                data16 = *(int16_t*)pCurDSSMsgData_;
                pCurDSSMsgData_ += sizeof(int16_t);
                pPath->numFrames = data16;
                if ( pPath->numFrames )
                {
                    pPath->sizeFrames = pPath->numFrames;
#line 2132
                    pPath->aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * pPath->numFrames);
                }

                for ( i = 0; i < pPath->numFrames; i++ )
                {
                    SITHDSS_POPVEC3(&pPath->aFrames[i].pos);
                    SITHDSS_POPVEC3(&pPath->aFrames[i].pyr);
                }
                break;
        }

        sithThing_Initialize(sithWorld_pCurrentWorld, pThing, 0);

        if ( pThing->pSwapList )
        {
            sithThing_ResetSwapList(pThing);
        }

        numEntries = SITHDSS_POPINT16();
        if ( numEntries != -1 )
        {
            for ( count = 0; count < numEntries; count++ )
            {
                entryNum = SITHDSS_POPINT32();
                meshNum = SITHDSS_POPINT32();
                modelIdx = SITHDSS_POPINT16();
                pModel = sithModel_GetModelByIndex(modelIdx);
                srcMeshNum = SITHDSS_POPINT32();
                if ( pModel )
                {
                    pThing->numSwapEntries = entryNum;
                    sithThing_AddSwapEntry(pThing, meshNum, pModel, srcMeshNum);
                }
            }
        }

        pThing->numSwapEntries = SITHDSS_POPUINT32();

        modelIdx = SITHDSS_POPINT16();
        if ( modelIdx != -1 )
        {
            pModel = sithModel_GetModelByIndex(modelIdx);
            if ( pModel && pThing->renderData.data.pModel3 != pModel )
            {
                sithThing_SetThingModel(pThing, pModel);
            }

            SITHDSS_POPVEC3(&pThing->renderData.data.pModel3->insertOffset);
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
            {
                SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
                SithActorWeaponInfo* pWeaponInfo = &pActorInfo->weaponInfo;
                SithActorVoiceInfo* pVoiceInfo = &pActorInfo->voiceInfo;

                pWeaponInfo->secWeaponActivationStartTime = SITHDSS_POPFLOAT();
                pWeaponInfo->secTimeLastRapidFired = SITHDSS_POPFLOAT();
                pWeaponInfo->secWeaponActivationWaitEndTime = SITHDSS_POPFLOAT();
                pWeaponInfo->secWeaponActivationWaitTime = SITHDSS_POPFLOAT();
                pWeaponInfo->secAimWaitEndTime = SITHDSS_POPFLOAT();
                pWeaponInfo->secWeaponSwapTime = SITHDSS_POPFLOAT();
                pWeaponInfo->selectedWeaponID = SITHDSS_POPINT32();
                pWeaponInfo->curWeaponID = SITHDSS_POPINT32();
                pWeaponInfo->deselectedWeaponID = SITHDSS_POPINT32();
                pWeaponInfo->weaponSwapRefNum = SITHDSS_POPINT32();
                SITHDSS_POPVEC3(&pWeaponInfo->vecUnknown0);
                SITHDSS_POPVEC3(&pActorInfo->fireOffset);
                SITHDSS_POPVEC4(&pVoiceInfo->voiceColor.top);
                pVoiceInfo->hSndChannel = SITHDSS_POPUINT32();
                pVoiceInfo->voiceHeadInfo.headMeshNum = SITHDSS_POPINT32();
                pVoiceInfo->voiceHeadInfo.headSwapRefNum = SITHDSS_POPINT32();
                for ( i = 0; i < STD_ARRAYLEN(pVoiceInfo->voiceHeadInfo.apSoundHeadModels); i++ )
                {
                    modelIdx = SITHDSS_POPINT16();
                    pVoiceInfo->voiceHeadInfo.apSoundHeadModels[i] = sithModel_GetModelByIndex(modelIdx);
                }
                break;
            }
        }

        alpha = SITHDSS_POPFLOAT();
        if ( strcmp(pThing->aName, "+plcogend") )
        {
            pThing->alpha = alpha;
        }

        return 1;
    }
}

int J3DAPI sithDSSThing_PathMove(const SithThing* pThing, int frame, float speed, int moveType, DPID idTO, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    rdVector3 curpyr;

    if ( !pThing || pThing->moveType != SITH_MT_PATH || !pThing->type || !pThing->pInSector )
    {
        return 0;
    }

    SITHDSS_PUSHINT32(moveType);
    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHINT32(sithMain_frameNumber);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pThing->pInSector));
    SITHDSS_PUSHVEC3(&pThing->pos);

    rdMatrix_ExtractAngles34(&pThing->orient, &curpyr);
    SITHDSS_PUSHVEC3(&curpyr);

    SITHDSS_PUSHINT16(frame);
    SITHDSS_PUSHFLOAT(speed);

    sithMulti_message.type = SITHDSS_PATHMOVE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTO, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessPathMove(const SithMessage* pMsg)
{
    SithThing* pThing;
    int moveClass;
    int sectIndex;
    unsigned int moveFrame;
    SithSector* pSector;
    rdVector3 curPyr;
    unsigned int curFrm;
    float velocity;
    int curGuid;
    uint8_t* pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];

#line 2314
    SITH_ASSERTREL(pMsg);

    moveClass = SITHDSS_POPINT32();
    curGuid = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(curGuid);
    if ( !pThing )
    {
        goto error;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
        goto error;
    }

    moveFrame = SITHDSS_POPUINT32();
    if ( pThing->moveFrame > moveFrame )
    {
#line 2332
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Received old move command: Frame %d < frame %d.  Ignored.\n", moveFrame, pThing->moveFrame, 0, 0);
        return 1;
    }

    pThing->moveFrame = moveFrame;
    sectIndex = SITHDSS_POPUINT16();
    pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, sectIndex);
    if ( !pSector )
    {
        goto error;
    }

    SITHDSS_POPVEC3(&pThing->pos);
    sithThing_SetSector(pThing, pSector, 0);

    SITHDSS_POPVEC3(&curPyr);
    rdMatrix_BuildRotate34(&pThing->orient, &curPyr);

    curFrm = SITHDSS_POPUINT16();
    velocity = SITHDSS_POPFLOAT();

    switch ( moveClass )
    {
        case 0:
            sithPathMove_MoveToFrame(pThing, curFrm, velocity);
            break;

        case 1:
            sithPathMove_SkipToFrame(pThing, curFrm, velocity);
            break;

        case 2:
            sithPathMove_Finish(pThing);
            break;
    }

    return 1;

error:
#line 2369
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad format in DSS::PathMove message, GUID=%x.\n", curGuid, 0, 0, 0);
    return 0;
}

int J3DAPI sithDSSThing_Attachment(const SithThing* pThing, DPID idTo, unsigned int outstream, unsigned int sendFlags)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    SithThing* pObjAttached;

    SITHDSS_PUSHINT32(pThing->guid);
    SITHDSS_PUSHUINT16(pThing->attach.flags);

    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 || (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        SITHDSS_PUSHINT16(sithSurface_GetSurfaceIndex(pThing->attach.attachedToStructure.pSurfaceAttached));
    }
    else if ( (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
    {
        SITHDSS_PUSHINT32(pThing->attach.attachedToStructure.pThingAttached->guid);
        if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
        {
            pObjAttached = pThing->attach.attachedToStructure.pThingAttached;
            if ( pObjAttached && pObjAttached->renderData.data.pModel3 )
            {
                SITHDSS_PUSHINT16(pThing->attach.pFace - pThing->attach.attachedToStructure.pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->aFaces);
            }
            else
            {
                SITHDSS_PUSHINT16(-1);
            }
        }
        else
        {
            SITHDSS_PUSHVEC3(&pThing->attach.posOffset);
        }
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_THINGCLIMBWHIP) != 0 )
    {
        SITHDSS_PUSHFLOAT(pThing->attach.posOffset.z);
    }

    sithMulti_message.type = SITHDSS_ATTACHMENT;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, sendFlags);
}

int J3DAPI sithDSSThing_ProcessAttachment(const SithMessage* pMsg)
{
    int idx;
    SithThing* pThing;
    SithAttachFlag attachflags;
    SithSurface* pSurf;
    SithThing* pThingAttached;
    rdFace* pFace;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 2425
    SITH_ASSERTREL(pMsg);

    idx = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(idx);
    if ( !pThing )
    {
#line 2431
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithDSSThing_ProcessAttachment: Cannot find GUID for thing index %d.\n", idx, 0, 0, 0);
        return 0;
    }

    attachflags = SITHDSS_POPUINT16();
    if ( (attachflags & (SITH_ATTACH_SURFACE | SITH_ATTACH_CLIMBSURFACE)) != 0 )
    {
        idx = SITHDSS_POPUINT16();
        pSurf = sithSurface_GetSurfaceEx(sithWorld_pCurrentWorld, idx);
        if ( !pSurf )
        {
#line 2442
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithDSSThing_ProcessAttachment: Invalid surface index %d.\n", idx, 0, 0, 0);
            return 0;
        }

        pThing->attach.flags = attachflags;
        rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &sithWorld_pCurrentWorld->aVertices[*pSurf->face.aVertices]);
        pThing->attach.pFace = &pSurf->face;
        pThing->attach.attachedToStructure.pSurfaceAttached = pSurf;
    }
    else if ( (attachflags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
    {
        idx = SITHDSS_POPINT32();
        pThingAttached = sithThing_GetGuidThing(idx);
        if ( !pThingAttached )
        {
#line 2457
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithDSSThing_ProcessAttachment: Cannot find GUID for thing index %d.\n", idx, 0, 0, 0);
            return 0;
        }

        pThing->attach.attachedToStructure.pThingAttached = pThingAttached;
        pThing->attach.flags = attachflags;
        sithThing_AddThingToAttachedThings(pThing, pThingAttached);

        if ( (attachflags & SITH_ATTACH_THINGFACE) != 0 )
        {
            idx = SITHDSS_POPUINT16();
            pFace = &pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->aFaces[idx];
            rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &pThingAttached->renderData.data.pModel3->aGeos[0].aMeshes->apVertices[*pFace->aVertices]);
            pThing->attach.pFace = pFace;
        }
        else
        {
            SITHDSS_POPVEC3(&pThing->attach.posOffset);
        }
    }
    else if ( (attachflags & SITH_ATTACH_THINGCLIMBWHIP) != 0 )
    {
        pThing->attach.flags = attachflags;
        pThing->attach.posOffset.z = SITHDSS_POPFLOAT();
    }
    else if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    return 1;
}

int J3DAPI sithDSSThing_Take(const SithThing* pItem, const SithThing* pThing, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHINT32(pItem->guid);
    SITHDSS_PUSHINT32(pThing->guid);

    sithMulti_message.type = SITHDSS_TAKE;
    SITHDSS_ENDOUT;

    if ( stdComm_IsGameHost() )
    {
        return sithDSSThing_ProcessTake(&sithMulti_message);
    }

    return sithMessage_SendMessage(&sithMulti_message, sithMulti_server.idServer, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessTake(const SithMessage* pMsg)
{
    SithThing* pCurItem;
    int guid;
    SithThing* pCurThing;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    guid = SITHDSS_POPINT32();
    pCurItem = sithThing_GetGuidThing(guid);
    if ( !pCurItem && stdComm_IsGameHost() )
    {
        sithDSSThing_DestroyThing(guid, sithMessage_GetSenderID());
        return 0;
    }

    guid = SITHDSS_POPINT32();
    pCurThing = sithThing_GetGuidThing(guid);
    if ( !pCurItem || !pCurThing )
    {
#line 2563
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing or taker guid in DSS::Take.\n", 0, 0, 0, 0);
        stdComm_IsGameHost();
        return 0;
    }

    if ( pMsg->type == SITHDSS_TAKE )
    {
        if ( !stdComm_IsGameHost() )
        {
            return 0;
        }

        if ( pCurItem->type != SITH_THING_ITEM || (pCurItem->flags & (SITH_TF_DISABLED | SITH_TF_DESTROYED)) != 0 )
        {
            return 1;
        }

        ((SithMessage*)pMsg)->type = SITHDSS_TAKE2;
        sithMessage_SendMessage((SithMessage*)pMsg, -1, SITHMESSAGE_STREAM_NET, DPSEND_GUARANTEED);
    }

    sithItem_SetItemTaken(pCurItem, pCurThing, 1);
    return 1;
}

int J3DAPI sithDSSThing_CreateThing(const SithThing* pTemplate, const SithThing* pNewThing, const SithThing* pMarker, const SithSector* pSector, const rdVector3* pos, const rdVector3* pyr, unsigned int outstream, unsigned int sendFlags)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHINT16(sithTemplate_GetTemplateIndex(pTemplate));

    if ( pMarker )
    {
        SITHDSS_PUSHINT32(pMarker->guid);
    }
    else
    {
#line 2613
        SITH_ASSERTREL(pSector && pos && pyr);
        SITHDSS_PUSHINT32(-1);
        SITHDSS_PUSHINT32(sithSector_GetSectorIndex(pSector));
        SITHDSS_PUSHVEC3(pos);
        SITHDSS_PUSHVEC3(pyr);
    }

    SITHDSS_PUSHINT32(pNewThing->guid);

    sithMulti_message.type = SITHDSS_CREATETHING;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, -1, outstream, sendFlags);
}

int J3DAPI sithDSSThing_ProcessCreateThing(const SithMessage* pMsg)
{
    rdVector3 pos;
    SithThing* pTemplate;
    SithThing* pMarker;
    SithSector* pSector;
    int id;
    rdMatrix34 rotation;
    rdVector3 curPyr;
    SithThing* pCurThing;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    id = SITHDSS_POPUINT16();
    pTemplate = sithTemplate_GetTemplateByIndex(id);
    if ( !pTemplate )
    {
#line 2655
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad template index %d in DSS::CreateThing message.\n", id, 0, 0, 0);
        return 0;
    }

    id = SITHDSS_POPINT32();
    if ( id >= 0 )
    {
        pMarker = sithThing_GetGuidThing(id);
        if ( !pMarker )
        {
#line 2665
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cannot find GUID %d to create template %s.\n", id, pTemplate->aName, 0, 0);
            return 0;
        }

        pCurThing = sithThing_CreateThing(pTemplate, pMarker);
    }
    else
    {
        id = SITHDSS_POPINT32();
        pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, id);
        if ( !pSector )
        {
#line 2680
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad sector %d in DSS::CreateThing, template %s.\n", id, pTemplate->aName, 0, 0);
            return 0;
        }

        SITHDSS_POPVEC3(&pos);
        SITHDSS_POPVEC3(&curPyr);
        rdMatrix_BuildRotate34(&rotation, &curPyr);
        pCurThing = sithThing_CreateThingAtPos(pTemplate, &pos, &rotation, pSector, NULL);
    }

    if ( !pCurThing )
    {
#line 2696
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to create thing of type %s from network instruction.\n", pTemplate->aName, 0, 0, 0);
        return 0;
    }

    if ( pCurThing )
    {
        pCurThing->guid = SITHDSS_POPINT32();
        pCurThing->flags |= SITH_TF_REMOTE;
    }
    return 1;
}

int J3DAPI sithDSSThing_DestroyThing(int guid, DPID idTo)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHINT32(guid);

    sithMulti_message.type = SITHDSS_DESTROYTHING;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSThing_ProcessDestroyThing(const SithMessage* pMsg)
{
    int guid;
    SithThing* pThing;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    guid = SITHDSS_POPINT32();
    pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
#line 2741
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing guid %d in DSS::DestroyThing.\n", guid, 0, 0, 0);
        return 0;
    }

    sithThing_DestroyThing(pThing);
    return 1;
}

void J3DAPI sithDSSThing_MoveToPos(SithThing* pThing, const rdVector3* newPos, SithSector* pSector)
{
    rdVector3 currentpredictedPos;
    rdVector3 curdir;
    float dist;

#line 2780
    SITH_ASSERTREL(pThing && (pThing->moveType == SITH_MT_PHYSICS) && pSector && newPos);

    currentpredictedPos.x = D3DMultiply(pThing->moveInfo.physics.velocity.x, 0.25f) + newPos->x;
    currentpredictedPos.y = D3DMultiply(pThing->moveInfo.physics.velocity.y, 0.25f) + newPos->y;
    currentpredictedPos.z = D3DMultiply(pThing->moveInfo.physics.velocity.z, 0.25f) + newPos->z;

    curdir.x = currentpredictedPos.x - pThing->pos.x;
    curdir.y = currentpredictedPos.y - pThing->pos.y;
    curdir.z = currentpredictedPos.z - pThing->pos.z;

    dist = rdVector_Len3(&curdir);
    if ( dist != 0.0f && dist < 0.5f )
    {
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &curdir, 4.0f);
    }
    else
    {
        rdVector_Copy3(&pThing->pos, newPos);
        sithThing_SetSector(pThing, pSector, 0);
    }
}
