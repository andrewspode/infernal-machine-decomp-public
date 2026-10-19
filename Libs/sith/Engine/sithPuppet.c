#include "sithPuppet.h"

#include <rdroid/Engine/rdKeyframe.h>
#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithControl.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct sCndKeyframeMarker
{
    float frame;
    rdKeyMarkerType type;
} CndKeyframeMarker;

typedef struct sCndKeyframeInfo
{
    char name[64];
    int flags;
    int type;
    uint32_t numFrames;
    float fps;
    uint32_t numMarkers;
    uint32_t numJoints;
    uint32_t numNodes;
} CndKeyframeInfo;

typedef struct sCndKeyframeNode
{
    char meshName[64];
    uint32_t nodeNum;
    uint32_t numEntries;
} CndKeyframeNode;

int sithPuppet_bPlayerLeapForward = 0;

static tHashTable* sithPuppet_pHashtblSubmodes;
static tHashTable* sithPuppet_pClassHashtable;
static tHashTable* sithPuppet_pKeyHashtable;

static const char* sithPuppet_aStrSubModes[SITH_PUPPET_NUMSUBMODES] =
{
    "--RESERVED--",
    "stand",
    "walk",
    "run",
    "walkback",
    "hopback",
    "hopleft",
    "hopright",
    "strafeleft",
    "straferight",
    "turnleft",
    "turnright",
    "slidedownfwd",
    "slidedownback",
    "leap",
    "jumpready",
    "jumpup",
    "jumpfwd",
    "rising",
    "fall",
    "death",
    "death2",
    "fidget",
    "fidget2",
    "pickup",
    "pushpullready",
    "pushitem",
    "pullitem",
    "mountledge",
    "grabledge",
    "hangledge",
    "hangshimleft",
    "hangshimright",
    "mountwall",
    "climbwallidle",
    "climbwallup",
    "climbwalldown",
    "climbwallleft",
    "climbwallright",
    "climbpullingup",
    "whipclimbmount",
    "whipclimbidle",
    "whipclimbup",
    "whipclimbdown",
    "whipclimbleft",
    "whipclimbright",
    "whipclimbdismount",
    "whipswingmount",
    "whipswing",
    "mountfromwater",
    "divefromsurface",
    "mount1mstep",
    "mount2mledge",
    "jumprollback",
    "jumprollfwd",
    "land",
    "hitheadl",
    "hitheadr",
    "hitsidel",
    "hitsider",
    "activate",
    "activatehigh",
    "drawweapon",
    "aimweapon",
    "holsterweapon",
    "fire",
    "fire2",
    "fire3",
    "fire4",
    "stand2walk",
    "walk2stand",
    "stand2crawl",
    "crawl2stand",
    "walk2attack",
    "victory",
    "hit",
    "hit2",
    "grabarms",
    "reserved",
    "climbtoclimb",
    "climbtohang",
    "leapleft",
    "leapright",
    "fallforward"
};

SithPuppetClass* J3DAPI sithPuppet_ClassCacheFind(const char* pName);
int J3DAPI sithPuppet_ClassCacheAdd(const SithPuppetClass* pClass);
int J3DAPI sithPuppet_ClassCacheRemove(const SithPuppetClass* pClass);

rdKeyframe* J3DAPI sithPuppet_KeyCacheFind(const char* pName);
void J3DAPI sithPuppet_KeyCacheAdd(rdKeyframe* pKeyframe);
void J3DAPI sithPuppet_KeyCacheRemove(rdKeyframe* pKeyframe);

SithPuppetTrack* J3DAPI sithPuppet_GetModeTrackImpl(const SithThing* pThing, SithPuppetSubMode submode);

static __inline float J3DAPI sithPuppet_GetThingLocalVelocityAxis(const SithThing* pThing, size_t* pAxis, float* pVelX, float* pVelY, float* pVelZ);

int J3DAPI sithPuppet_Startup(void)
{
    size_t modeNum;

    sithPuppet_pClassHashtable = stdHashtbl_New(64u);
    sithPuppet_pKeyHashtable   = stdHashtbl_New(512u);
    if ( !sithPuppet_pClassHashtable || !sithPuppet_pKeyHashtable )
    {
#line 221
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not allocate memory of puppets.\n", 0, 0, 0, 0);
        return 1;
    }

    sithPuppet_pHashtblSubmodes = stdHashtbl_New(168u);
    for ( modeNum = 1; modeNum < STD_ARRAYLEN(sithPuppet_aStrSubModes); ++modeNum )
    {
        stdHashtbl_Add(sithPuppet_pHashtblSubmodes, sithPuppet_aStrSubModes[modeNum], (void*)modeNum);
    }

    return 0;
}

void J3DAPI sithPuppet_Shutdown(void)
{
    if ( sithPuppet_pClassHashtable )
    {
        stdHashtbl_Free(sithPuppet_pClassHashtable);
        sithPuppet_pClassHashtable = NULL;
    }

    if ( sithPuppet_pKeyHashtable )
    {
        stdHashtbl_Free(sithPuppet_pKeyHashtable);
        sithPuppet_pKeyHashtable = NULL;
    }

    if ( sithPuppet_pHashtblSubmodes )
    {
        stdHashtbl_Free(sithPuppet_pHashtblSubmodes);
        sithPuppet_pHashtblSubmodes = NULL;
    }
}

void J3DAPI sithPuppet_New(SithThing* pThing)
{
#line 284
    SITH_ASSERTREL(pThing && pThing->renderData.pPuppet);

#line 286
    pThing->pPuppetState = (SithPuppetState*)STDMALLOC(sizeof(SithPuppetState));
    if ( !pThing->pPuppetState )
    {
        pThing->pPuppetClass = NULL;
    }

    memset(pThing->pPuppetState, 0, sizeof(SithPuppetState));

    if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
        || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 && pThing->type == SITH_THING_PLAYER )
    {
        pThing->pPuppetState->moveMode = SITHPUPPET_MOVEMODE_SWIM;
    }
    else
    {
        pThing->pPuppetState->moveMode = SITHPUPPET_MOVEMODE_NORMAL;
    }

    pThing->pPuppetState->pFirstTrack = NULL;
}

void J3DAPI sithPuppet_Free(SithThing* pThing)
{
    SithPuppetTrack* pTrack;
    SithPuppetTrack* pNextTrack;

    if ( pThing->pPuppetState )
    {
        if ( pThing->pPuppetState->pFirstTrack )
        {
            pTrack = pThing->pPuppetState->pFirstTrack;
            while ( pTrack )
            {
                if ( pTrack == STDMEMORY_FREEDPTR )
                {
                    break;
                }

                pNextTrack = pTrack->pNextTrack;
                stdMemory_Free(pTrack);
                pTrack = pNextTrack;
            }
        }

        stdMemory_Free(pThing->pPuppetState);
        pThing->pPuppetState = NULL;
    }
}

void J3DAPI sithPuppet_RemoveAllTracks(SithThing* pThing)
{
    SithPuppetTrack* pTrack;
    SithPuppetTrack* pNextTrack;
    int track;

#line 358
    SITH_ASSERTREL(pThing && pThing->renderData.pPuppet);

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
#line 366
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Pup track list is messed up, pointer to freed memory!\n", 0, 0, 0, 0);
            break;
        }

        if ( pTrack->trackNum == (int)STDMEMORY_FREEDPTR )
        {
#line 371
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Pup track list is messed up, pointer to freed memory!\n", 0, 0, 0, 0);
            break;
        }

        pNextTrack = pTrack->pNextTrack;
        rdPuppet_RemoveTrack(pThing->renderData.pPuppet, pTrack->trackNum);
        if ( pTrack->pNextTrack != STDMEMORY_FREEDPTR )
        {
            stdMemory_Free(pTrack);
        }

        pTrack = pNextTrack;
    }

    pThing->pPuppetState->pFirstTrack = NULL;

    for ( track = 0; track < (int)RDPUPPET_MAX_TRACKS; ++track )
    {
        if ( pThing->renderData.pPuppet->aTracks[track].status )
        {
            rdPuppet_RemoveTrack(pThing->renderData.pPuppet, track);
        }
    }
}

void J3DAPI sithPuppet_UpdatePuppet(SithThing* pThing, float secDeltaTime)
{
    SithPuppetTrack* pTrack;
    SithPuppetTrack* pNextTrack;
    int majorMode;
    int numUpdated;

#line 414
    SITH_ASSERTREL(pThing != NULL);

    if ( !pThing->pPuppetClass || !pThing->pPuppetState || !pThing->renderData.pPuppet )
    {
        return;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_PUPPETSYSTEM_DISABLED) != 0 )
    {
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
    {
        sithPuppet_UpdateThingMoveTracks(pThing, secDeltaTime);
        sithPuppet_PlayFidgetMode(pThing);
    }

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack->pNextTrack == STDMEMORY_FREEDPTR )
        {
            pTrack->pNextTrack = NULL;
        }

        pNextTrack = pTrack->pNextTrack;
        if ( !pThing->renderData.pPuppet->aTracks[pTrack->trackNum].pKFTrack )
        {
#line 441
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Removed submode %d from pup list, no KFTrack!\n", pTrack->submode, 0, 0, 0);
            sithPuppet_FreeTrack(pThing, pTrack);
        }
        else if ( (pThing->renderData.pPuppet->aTracks[pTrack->trackNum].status & RDPUPPET_TRACK_PLAYING) == 0 )
        {
#line 448
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Removed submode %d from pup list, not playing!\n", pTrack->submode, 0, 0, 0);
            sithPuppet_FreeTrack(pThing, pTrack);
        }

        pTrack = pNextTrack;
    }

    majorMode  = pThing->pPuppetState->majorMode;
    numUpdated = rdPuppet_UpdateTracks(pThing->renderData.pPuppet, secDeltaTime);
    if ( pThing->pPuppetState->majorMode != majorMode )
    {
        numUpdated = rdPuppet_UpdateTracks(pThing->renderData.pPuppet, secDeltaTime);
    }

    if ( numUpdated && pThing->moveType == SITH_MT_PATH )
    {
        rdVector3 moveNorm;
        size_t i;

        memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));

        pThing->renderData.bSkipBuildingJoints = 0;
        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        pThing->renderData.bSkipBuildingJoints = 1;

        rdVector_Add3(&moveNorm, &pThing->renderData.paJointMatrices->dvec, &pThing->moveInfo.pathMovement.curOrient.dvec);
        rdVector_Sub3Acc(&moveNorm, &pThing->pos);
        if ( moveNorm.x != 0.0f || moveNorm.y != 0.0f || moveNorm.z != 0.0f )
        {
            float moveDist;

            moveDist = rdVector_Normalize3Acc(&moveNorm);
            sithCollision_MoveThing(pThing, &moveNorm, moveDist, 0);
        }

        rdVector_Sub3(&moveNorm, &pThing->pos, &pThing->renderData.paJointMatrices->dvec);
        for ( i = 0; i < pThing->renderData.data.pModel3->numHNodes; ++i )
        {
            rdVector_Add3Acc(&pThing->renderData.paJointMatrices[i].dvec, &moveNorm);
        }
    }

    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        int bSkipUpdatingJoints;
        rdVector3 moveNorm;
        float distMoved;
        float moveDist;
        size_t i;

        bSkipUpdatingJoints = 0;
        memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));

        pThing->renderData.bSkipBuildingJoints = 0;
        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        pThing->renderData.bSkipBuildingJoints = 1;

        rdVector_Add3(&moveNorm, &pThing->renderData.paJointMatrices->dvec, &pThing->forceMoveStartPos);
        rdVector_Sub3Acc(&moveNorm, &pThing->pos);
        if ( moveNorm.x != 0.0f || moveNorm.y != 0.0f || moveNorm.z != 0.0f )
        {
            int flags;

            flags = 0xA00;
            rdVector_Copy3(&pThing->moveInfo.physics.velocity, &moveNorm);
            moveDist  = rdVector_Normalize3Acc(&moveNorm);
            distMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, flags);
        }
        else
        {
            moveDist = distMoved = 0.0f;
        }

        if ( distMoved < D3DMultiply(moveDist, 0.25f) && pThing->type == SITH_THING_PLAYER )
        {
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_STILL:
                case SITHPLAYERMOVE_CRAWL_STILL:
                case SITHPLAYERMOVE_UNKNOWN_4:
                case SITHPLAYERMOVE_UNKNOWN_5:
                case SITHPLAYERMOVE_HANGING:
                case SITHPLAYERMOVE_CLIMBIDLE:
                case SITHPLAYERMOVE_WHIPSWINGING:
                case SITHPLAYERMOVE_SWIMIDLE:
                case SITHPLAYERMOVE_CLIMBING_UP:
                case SITHPLAYERMOVE_CLIMBING_DOWN:
                case SITHPLAYERMOVE_CLIMBING_LEFT:
                case SITHPLAYERMOVE_CLIMBING_RIGHT:
                case SITHPLAYERMOVE_PULLINGUP:
                case SITHPLAYERMOVE_PULLINGUP_1M:
                case SITHPLAYERMOVE_WHIP_CLIMB_START:
                case SITHPLAYERMOVE_STAND_TO_CRAWL:
                case SITHPLAYERMOVE_CRAWL_TO_STAND:
                case SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT:
                case SITHPLAYERMOVE_CLIMB_TO_HANG:
                    break;

                default:
                    sithPuppet_StopForceMove(pThing, 1);
                    bSkipUpdatingJoints = 1;
                    break;
            }
        }

        if ( !bSkipUpdatingJoints )
        {
            rdVector_Sub3(&moveNorm, &pThing->pos, &pThing->renderData.paJointMatrices->dvec);
            for ( i = 0; i < pThing->renderData.data.pModel3->numHNodes; ++i )
            {
                rdVector_Add3Acc(&pThing->renderData.paJointMatrices[i].dvec, &moveNorm);
            }
        }
    }
}

void J3DAPI sithPuppet_PlayFidgetMode(SithThing* pThing)
{
    SithPuppetTrack* pTrack;
    float rnd;

#line 589
    SITH_ASSERTREL(pThing);
#line 590
    SITH_ASSERTREL(pThing->pPuppetState);

    if ( pThing->type == SITH_THING_PLAYER && pThing->thingInfo.actorInfo.weaponInfo.curWeaponID )
    {
        return;
    }

    if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal && (pThing->controlInfo.aiControl.pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        return;
    }

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            return;
        }

        if ( pTrack->submode == SITHPUPPETSUBMODE_FIDGET || pTrack->submode == SITHPUPPETSUBMODE_FIDGET2 )
        {
            return;
        }

        pTrack = pTrack->pNextTrack;
    }

    if ( pThing->pPuppetState->submode == SITHPUPPETSUBMODE_STAND && sithTime_msecGameTime > pThing->pPuppetState->msecLastFidgetStillMoveTime + 30000.0f )
    {
        rnd = SITH_RAND();
        if ( rnd < 0.3f )
        {
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FIDGET, NULL);
            pThing->pPuppetState->msecLastFidgetStillMoveTime = sithTime_msecGameTime;
        }
        else if ( rnd < 0.6f )
        {
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FIDGET2, NULL);
        }
    }
}

void J3DAPI sithPuppet_StopFridgetTrack(SithThing* pThing)
{
    SithPuppetTrack* pTrack;

#line 639
    SITH_ASSERTREL(pThing);
#line 640
    SITH_ASSERTREL(pThing->pPuppetState);

    pThing->pPuppetState->msecLastFidgetStillMoveTime = sithTime_msecGameTime;
    if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal && (pThing->controlInfo.aiControl.pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        return;
    }

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        if ( pTrack->submode == SITHPUPPETSUBMODE_FIDGET || pTrack->submode == SITHPUPPETSUBMODE_FIDGET2 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.0f);
            return;
        }

        pTrack = pTrack->pNextTrack;
    }
}

int J3DAPI sithPuppet_IsModeOnTrack(const SithThing* pThing, SithPuppetSubMode mode)
{
    SithPuppetClassSubmode* submode;
    SithPuppetTrack* pTrack;
    rdKeyframe* pKeyframe;

#line 693
    SITH_ASSERTREL(pThing);

    if ( !pThing->pPuppetClass || mode < 0 || mode >= SITH_PUPPET_NUMSUBMODES )
    {
        return 0;
    }

    submode  = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][mode];
    pKeyframe = submode->pKeyframe;
    if ( !pKeyframe )
    {
        return 0;
    }

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        if ( pTrack->submode == mode )
        {
            return 1;
        }

        pTrack = pTrack->pNextTrack;
    }

    return 0;
}

int J3DAPI sithPuppet_IsAnyModeOnTrack(const SithThing* pThing, SithPuppetSubMode firstMode, SithPuppetSubMode lastMode)
{
    SithPuppetSubMode startMode;
    SithPuppetSubMode endMode;
    SithPuppetSubMode curSubMode;
    rdKeyframe* pKeyframe;
    int trackNum;

#line 745
    SITH_ASSERTREL(pThing);

    startMode = firstMode < lastMode ? firstMode : lastMode;
    endMode   = firstMode > lastMode ? firstMode : lastMode;

    for ( curSubMode = startMode; curSubMode <= endMode; ++curSubMode )
    {
        pKeyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][curSubMode].pKeyframe;
        if ( pKeyframe )
        {
            for ( trackNum = 0; trackNum < (int)RDPUPPET_MAX_TRACKS; ++trackNum )
            {
                if ( pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack == pKeyframe )
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

SithPuppetTrack* J3DAPI sithPuppet_FindActiveTrack(const SithThing* pThing, const rdKeyframe* pKfTrack)
{
    SithPuppetTrack* pTrack;

#line 795
    SITH_ASSERTREL(pThing);

    if ( !pKfTrack )
    {
#line 799
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "FindActiveTrack(): NULL track to search for THING '%s' passed.\n", pThing->aName, 0, 0, 0);
        return NULL;
    }

    if ( !pThing->pPuppetState )
    {
        return NULL;
    }

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        if ( pTrack->pSubmode->pKeyframe == pKfTrack )
        {
            if ( (pThing->renderData.pPuppet->aTracks[pTrack->trackNum].status & RDPUPPET_TRACK_PLAYING) == 0 )
            {
                return NULL;
            }

            if ( (pThing->renderData.pPuppet->aTracks[pTrack->trackNum].status & RDPUPPET_TRACK_FADEOUT) != 0 )
            {
                return NULL;
            }

            return pTrack;
        }

        pTrack = pTrack->pNextTrack;
    }

    return NULL;
}

void J3DAPI sithPuppet_UpdateThingMoveTracks(SithThing* pThing, float secDeltaTime)
{
    float moveSpeed;
    SithPuppetTrack* pTrack;
    float numFrames;
    int actorFlags;

#line 859
    SITH_ASSERTREL(pThing);
#line 860
    SITH_ASSERTREL(pThing->renderData.pPuppet);
#line 861
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    if ( pThing->controlType == SITH_CT_AI
        && pThing->controlInfo.aiControl.pLocal
        && (pThing->moveInfo.physics.flags & (SITH_PF_UNKNOWN_8000000 | SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0 )
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0 && (pThing->thingInfo.actorInfo.flags & SITH_AF_HUMAN) != 0 )
        {
            moveSpeed = D3DMultiply(sithPuppet_UpdateThingMove(pThing, NULL), secDeltaTime);
        }
        else
        {
            moveSpeed = D3DMultiply(sithAIMove_UpdateAIMove(pThing->controlInfo.aiControl.pLocal), secDeltaTime);
        }
    }
    else
    {
        moveSpeed = D3DMultiply(sithPuppet_UpdateThingMove(pThing, NULL), secDeltaTime);
    }

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        if ( (pTrack->pSubmode->flags & RDKEYFRAME_PUPPET_CONTROLLED) != 0 )
        {
            if ( pTrack->submode == SITHPUPPETSUBMODE_RUN )
            {
                numFrames = D3DMultiply(moveSpeed < 0.0f ? -moveSpeed : moveSpeed, 90.0f);
            }
            else
            {
                actorFlags = pThing->thingInfo.actorInfo.flags;
                if ( (actorFlags & SITH_AF_FASTMOVE15) != 0 )
                {
                    numFrames = D3DMultiply(moveSpeed < 0.0f ? -moveSpeed : moveSpeed, 500.0f);
                }
                else if ( (actorFlags & SITH_AF_FASTMOVE10) != 0 )
                {
                    numFrames = D3DMultiply(moveSpeed < 0.0f ? -moveSpeed : moveSpeed, 300.0f);
                }
                else
                {
                    numFrames = D3DMultiply(moveSpeed < 0.0f ? -moveSpeed : moveSpeed, 180.0f);
                }
            }

            rdPuppet_AdvanceTrack(pThing->renderData.pPuppet, pTrack->trackNum, numFrames);
        }

        pTrack = pTrack->pNextTrack;
    }
}

float J3DAPI sithPuppet_UpdateThingMove(SithThing* pThing, rdPuppetTrackCallback pfCallback)
{
    float velX;
    float velY;
    float velZ;
    float moveSpeed;
    size_t axis;
    SithPuppetSubMode submode;
    SithThingMoveStatus moveStatus;
    int bFloating;
    SithPuppetTrack* pTrack;
    int bAIMoving;
    float unusedSpeed;
    int bSwimModeSet;

    unusedSpeed  = 0.2f;
    bSwimModeSet = 0;
    bFloating    = 0;

#line 947
    SITH_ASSERTREL(pThing);
#line 948
    SITH_ASSERTREL(pThing->pPuppetClass);
#line 949
    SITH_ASSERTREL(pThing->renderData.pPuppet);

    moveSpeed = sithPuppet_GetThingLocalVelocityAxis(pThing, &axis, &velX, &velY, &velZ);

    if ( (pThing->type == SITH_THING_PLAYER && pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
        || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0)
        && !pThing->thingInfo.actorInfo.bForceMovePlay
        && pThing->moveStatus != SITHPLAYERMOVE_ACTIVATING )
    {
        pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
    }

    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        return moveSpeed;
    }

    if ( pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        return moveSpeed;
    }

    submode = pThing->pPuppetState->submode;
    if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
    {
        bAIMoving = sithAIMove_AIGetMoveState(pThing->controlInfo.aiControl.pLocal);
        if ( bAIMoving )
        {
        }
        else
        {
            sithAIMove_GetAIMoveModes(pThing, moveSpeed, axis, &moveStatus, &submode);
            pThing->moveStatus = moveStatus;
        }
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_WALK2STAND:
        case SITHPLAYERMOVE_STAND2WALK:
            return moveSpeed;

        case SITHPLAYERMOVE_TURNING_LEFT:
            submode = SITHPUPPETSUBMODE_TURNLEFT;
            break;

        case SITHPLAYERMOVE_TURNING_RIGHT:
            submode = SITHPUPPETSUBMODE_TURNRIGHT;
            break;

        case SITHPLAYERMOVE_LAND:
            submode = SITHPUPPETSUBMODE_STAND;
            break;

        case SITHPLAYERMOVE_STILL:
            switch ( axis )
            {
                case 2:
                    if ( moveSpeed < 0.0f && (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
                    {
                        if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f )
                        {
                            submode = SITHPUPPETSUBMODE_FALLFORWARD;
                        }
                        else
                        {
                            submode = SITHPUPPETSUBMODE_FALL;
                        }

                        pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                    }
                    break;

                case 0:
                case 1:
                    if ( moveSpeed < 0.0f )
                    {
                        submode = SITHPUPPETSUBMODE_WALKBACK;
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    }
                    else if ( moveSpeed > 0.0001f )
                    {
                        submode = SITHPUPPETSUBMODE_WALK;
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    }
                    break;

                default:
                    moveSpeed = D3DMultiply(D3DVAL(pThing->moveInfo.physics.angularVelocity.y), 0.0002f);
                    if ( moveSpeed < -0.01f )
                    {
                        submode = SITHPUPPETSUBMODE_TURNRIGHT;
                    }
                    else if ( moveSpeed > 0.01f )
                    {
                        submode = SITHPUPPETSUBMODE_TURNLEFT;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                    }
                    break;
            }
            break;

        case SITHPLAYERMOVE_WALKING:
            switch ( axis )
            {
                case 1:
                    if ( moveSpeed < 0.0f )
                    {
                        submode = SITHPUPPETSUBMODE_WALKBACK;
                    }
                    else if ( moveSpeed > 0.0001f )
                    {
                        submode = SITHPUPPETSUBMODE_WALK;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    }
                    break;

                case 0:
                    if ( moveSpeed != 0.0f )
                    {
                        submode = SITHPUPPETSUBMODE_WALK;
                    }
                    break;

                default:
                    submode = SITHPUPPETSUBMODE_STAND;
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;
            }
            break;

        case SITHPLAYERMOVE_RUNNING:
            switch ( axis )
            {
                case 1:
                    if ( moveSpeed != 0.0f )
                    {
                        submode = SITHPUPPETSUBMODE_RUN;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    }
                    break;

                case 0:
                    if ( moveSpeed != 0.0f )
                    {
                        submode = SITHPUPPETSUBMODE_RUN;
                    }
                    else
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        submode = SITHPUPPETSUBMODE_STAND;
                    }
                    break;

                default:
                    if ( moveSpeed < 0.0f && (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
                    {
                        if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f )
                        {
                            submode = SITHPUPPETSUBMODE_FALLFORWARD;
                        }
                        else
                        {
                            submode = SITHPUPPETSUBMODE_FALL;
                        }

                        pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    }
                    break;
            }
            break;

        case SITHPLAYERMOVE_CRAWL_STILL:
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                float yawSpeed;

                switch ( axis )
                {
                    case 1:
                        if ( moveSpeed < 0.0f )
                        {
                            submode = SITHPUPPETSUBMODE_WALKBACK;
                        }
                        else if ( moveSpeed > 0.0001f )
                        {
                            submode = SITHPUPPETSUBMODE_WALK;
                        }
                        break;

                    default:
                        yawSpeed = D3DMultiply(D3DVAL(pThing->moveInfo.physics.angularVelocity.y), 0.0002f);
                        if ( yawSpeed < -0.01f )
                        {
                            submode = SITHPUPPETSUBMODE_TURNRIGHT;
                        }
                        else if ( yawSpeed > 0.01f )
                        {
                            submode = SITHPUPPETSUBMODE_TURNLEFT;
                        }
                        else
                        {
                            submode = SITHPUPPETSUBMODE_STAND;
                        }
                        break;
                }
            }
            break;

        case SITHPLAYERMOVE_HANGING:
            submode = SITHPUPPETSUBMODE_HANGLEDGE;
            break;

        case SITHPLAYERMOVE_RAFT_STILL:
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT:
        case SITHPLAYERMOVE_UNKNOWN_45:
        case SITHPLAYERMOVE_UNKNOWN_46:
        case SITHPLAYERMOVE_SLIDING:
        case SITHPLAYERMOVE_RAFT_BOARDING:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            pThing->pPuppetState->submode = SITHPUPPETSUBMODE_STAND;
            return moveSpeed;

        case SITHPLAYERMOVE_JEEP_STILL:
        {
            float yawSpeed;

                yawSpeed = D3DMultiply(D3DVAL(pThing->moveInfo.physics.angularVelocity.y), 0.0002f);
                if ( moveSpeed > 0.001f || moveSpeed < -0.001f && !sithControl_GetKey(SITHCONTROL_BACK, 0) )
                {
                    if ( sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_WALKBACK) )
                    {
                        sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_WALKBACK, 0.1f);
                        sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_HITHEADL, 0.01f);
                        sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_HITHEADR, 0.01f);
                        submode = SITHPUPPETSUBMODE_LEAP;
                    }
                    else if ( yawSpeed < -0.001f )
                    {
                        submode = SITHPUPPETSUBMODE_TURNRIGHT;
                    }
                    else if ( yawSpeed > 0.001f )
                    {
                        submode = SITHPUPPETSUBMODE_TURNLEFT;
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                    }
                }
                else if ( moveSpeed < -0.001f )
                {
                    submode = SITHPUPPETSUBMODE_WALKBACK;
                    if ( sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_WALKBACK) )
                    {
                        if ( yawSpeed < -0.001f )
                        {
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HITHEADL, pfCallback);
                        }
                        else if ( yawSpeed > 0.001f )
                        {
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HITHEADR, pfCallback);
                        }
                        else
                        {
                            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_HITHEADL, 0.1f);
                            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_HITHEADR, 0.1f);
                        }
                    }
                }
                else if ( sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_WALKBACK) )
                {
                    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_WALKBACK, 0.1f);
                    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_HITHEADL, 0.01f);
                    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_HITHEADR, 0.01f);
                    submode = SITHPUPPETSUBMODE_LEAP;
                }
                else if ( sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_LEAP) )
                {
                    return moveSpeed;
                }
                else if ( !pThing->pPuppetState->pFirstTrack )
                {
                    submode = SITHPUPPETSUBMODE_STAND;
                }
                else
                {
                    return moveSpeed;
                }
        } break;

        case SITHPLAYERMOVE_UNKNOWN_4:
        case SITHPLAYERMOVE_UNKNOWN_5:
        case SITHPLAYERMOVE_PUSHING:
        case SITHPLAYERMOVE_PULLING:
        case SITHPLAYERMOVE_WHIPSWINGING:
        case SITHPLAYERMOVE_JUMPFWD:
        case SITHPLAYERMOVE_JUMPUP:
        case SITHPLAYERMOVE_JUMP_READY:
        case SITHPLAYERMOVE_PUSHPULL_READY:
        case SITHPLAYERMOVE_MOUNTING_WALL:
        case SITHPLAYERMOVE_CLIMBING_UP:
        case SITHPLAYERMOVE_CLIMBING_DOWN:
        case SITHPLAYERMOVE_CLIMBING_LEFT:
        case SITHPLAYERMOVE_CLIMBING_RIGHT:
        case SITHPLAYERMOVE_PULLINGUP:
        case SITHPLAYERMOVE_PULLINGUP_1M:
        case SITHPLAYERMOVE_MINECAR_IDLE:
        case SITHPLAYERMOVE_MINECAR_BOARDING:
        case SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT:
        case SITHPLAYERMOVE_MINECAR_DUCKING:
        case SITHPLAYERMOVE_MINECAR_DUCKED:
        case SITHPLAYERMOVE_MINECAR_GETTING_UP:
        case SITHPLAYERMOVE_JEEP_BOARDING:
        case SITHPLAYERMOVE_ROLLING_LEFT:
        case SITHPLAYERMOVE_ROLLING_RIGHT:
        case SITHPLAYERMOVE_STRAFING_LEFT:
        case SITHPLAYERMOVE_STRAFING_RIGHT:
        case SITHPLAYERMOVE_JUMPBACK:
        case SITHPLAYERMOVE_STAND_TO_CRAWL:
        case SITHPLAYERMOVE_CRAWL_TO_STAND:
        case SITHPLAYERMOVE_JUMPROLLBACK:
        case SITHPLAYERMOVE_JUMPROLLFWD:
        case SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT:
        case SITHPLAYERMOVE_CLIMB_TO_HANG:
        case SITHPLAYERMOVE_JUMPLEFT:
        case SITHPLAYERMOVE_JUMPRIGHT:
        case SITHPLAYERMOVE_LEAPFWD:
        case SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES:
            return moveSpeed;

        case SITHPLAYERMOVE_JEWELFLYING:
            return moveSpeed;

        case SITHPLAYERMOVE_WHIPCLIMBIDLE:
        case SITHPLAYERMOVE_WHIP_CLIMB_START:
            submode = SITHPUPPETSUBMODE_WHIPCLIMBIDLE;
            break;

        case SITHPLAYERMOVE_CLIMBIDLE:
            submode = SITHPUPPETSUBMODE_CLIMBWALLIDLE;
            break;

        case SITHPLAYERMOVE_ACTIVATING:
            return moveSpeed;

        case SITHPLAYERMOVE_SWIMIDLE:
            if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 || (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
            {
                bFloating = 1;
            }

            if ( pThing->pInSector
                && ((pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || bFloating)
                && pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
            {
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
                bSwimModeSet = 1;
            }

            switch ( axis )
            {
                case 1:
                    if ( moveSpeed < -0.04f )
                    {
                        submode = SITHPUPPETSUBMODE_WALKBACK;
                        if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 8) && SITH_RAND() < 0.4f) && !bFloating )
                        {
                            sithFX_CreateWaterRipple(pThing);
                        }
                    }
                    else if ( moveSpeed > 0.04f )
                    {
                        if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 || bFloating )
                        {
                            submode = SITHPUPPETSUBMODE_RUN;
                            if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 4) && SITH_RAND() < 0.3f) && !bFloating )
                            {
                                sithFX_CreateBubble(pThing);
                            }
                        }
                        else
                        {
                            submode = SITHPUPPETSUBMODE_WALK;
                            if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 8) && SITH_RAND() < 0.3f) && !bFloating )
                            {
                                sithFX_CreateWaterRipple(pThing);
                            }
                        }
                    }
                    else if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 || bFloating )
                    {
                        submode = SITHPUPPETSUBMODE_RISING;
                        if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 4) && SITH_RAND() < 0.3f) && !bFloating )
                        {
                            sithFX_CreateBubble(pThing);
                        }
                    }
                    else
                    {
                        submode = SITHPUPPETSUBMODE_STAND;
                        if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 8) && SITH_RAND() < 0.3f) && !bFloating )
                        {
                            sithFX_CreateWaterRipple(pThing);
                        }
                    }
                    break;

                default:
    if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 || bFloating )
                {
                    submode = SITHPUPPETSUBMODE_RISING;
                    if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 8) && SITH_RAND() < 0.3f) && !bFloating )
                    {
                        sithFX_CreateBubble(pThing);
                    }
                }
                else
                {
                    submode = SITHPUPPETSUBMODE_STAND;
                    if ( (submode != pThing->pPuppetState->submode || SITH_ISFRAMECYCLE(pThing->idx, 8) && SITH_RAND() < 0.3f) && !bFloating )
                    {
                        sithFX_CreateWaterRipple(pThing);
                    }
                }
                    break;
            }
            break;

        case SITHPLAYERMOVE_FALLING:
            if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f )
            {
                submode = SITHPUPPETSUBMODE_FALLFORWARD;
            }
            else
            {
                submode = SITHPUPPETSUBMODE_FALL;
            }
            break;

        case SITHPLAYERMOVE_UNKNOWN_63:
            pTrack = sithPuppet_GetModeTrackImpl(pThing, SITHPUPPETSUBMODE_JUMPREADY);
            if ( pTrack )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.0f);
                sithPuppet_FreeTrack(pThing, pTrack);
            }
            break;

        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
            submode = SITHPUPPETSUBMODE_SLIDEDOWNFWD;
            break;

        case SITHPLAYERMOVE_SLIDEDOWNBACK:
            submode = SITHPUPPETSUBMODE_SLIDEDOWNBACK;
            break;
    }

    sithPuppet_SetSubMode(pThing, submode, pfCallback);
    return moveSpeed;
}

int J3DAPI sithPuppet_SetSubMode(SithThing* pThing, SithPuppetSubMode newSubMode, rdPuppetTrackCallback pfCallback)
{
    int bTrackFading = 0;
    int bTrackPlaying = 0;
    int bFadeTrack = 0;
    int trackNum = -1;
    SithPuppetTrack* pCurTrack;
    SithPuppetTrack* pNextTrack;

#line 1615
    SITH_ASSERTREL(newSubMode < SITH_PUPPET_NUMSUBMODES);
    if ( !pThing->pPuppetClass || !pThing->pPuppetState )
    {
        return -1;
    }

    pCurTrack = pThing->pPuppetState->pFirstTrack;
    while ( pCurTrack )
    {
        if ( pCurTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        bTrackFading = 0;
        pNextTrack   = pCurTrack->pNextTrack;
        if ( pCurTrack->submode == newSubMode )
        {
            if ( (pThing->renderData.pPuppet->aTracks[pCurTrack->trackNum].status & RDPUPPET_TRACK_FADEOUT) == 0 )
            {
                bTrackPlaying = 1;
                trackNum = pCurTrack->trackNum;
            }
            else
            {
                bTrackFading = 1;
            }
        }

        if ( !bTrackPlaying && ((unsigned int)pCurTrack->submode < SITHPUPPETSUBMODE_JUMPROLLFWD || (unsigned int)pCurTrack->submode > SITHPUPPETSUBMODE_GRABARMS) )
        {
            bFadeTrack = 1;
            if ( bTrackFading )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, pCurTrack->trackNum, 0.0f);
            }
            else if ( pThing->controlType == SITH_CT_AI && (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_QUICKMODEFADE) != 0 )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, pCurTrack->trackNum, 0.005f);
            }
            else if ( pThing->controlType == SITH_CT_AI && (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_SLOWMODEFADE) != 0 )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, pCurTrack->trackNum, 0.25f);
            }
            else if ( pThing->moveStatus == SITHPLAYERMOVE_CRAWL_STILL )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, pCurTrack->trackNum, 0.0f);
            }
            else
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, pCurTrack->trackNum, 0.07f);
            }
        }

        pCurTrack = pNextTrack;
    }

    pThing->pPuppetState->submode = newSubMode;
    if ( !bTrackPlaying )
    {
        trackNum = sithPuppet_PlayMode(pThing, newSubMode, pfCallback);
        if ( bFadeTrack
            && trackNum != -1
            && trackNum != SITHPUPPET_PLAYERROR_ALREADYPLAYING
            && pThing->controlType == SITH_CT_AI
            && (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_SLOWMODEFADE) != 0 )
        {
            rdPuppet_FadeInTrack(pThing->renderData.pPuppet, trackNum, 0.25f);
        }
    }

    return trackNum;
}

void J3DAPI sithPuppet_SetMoveMode(SithThing* pThing, unsigned int newMode)
{
    SithPuppetTrack* pTrack;
    SithPuppetTrack* pNextTrack;

#line 1720
    SITH_ASSERTREL(newMode < SITH_PUPPET_NUMMOVEMODES);

    if ( !pThing->pPuppetClass || !pThing->pPuppetState )
    {
        return;
    }

    pThing->pPuppetState->majorMode = pThing->pPuppetState->armedMode + (SITH_PUPPET_NUMARMEDMODES * newMode);
    pThing->pPuppetState->moveMode  = newMode;

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        pNextTrack = pTrack->pNextTrack;
        sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.0f);
        pTrack = pNextTrack;
    }

    sithPuppet_ClearTrackList(pThing);
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
}

void J3DAPI sithPuppet_SetArmedMode(SithThing* pThing, unsigned int newMode)
{
#line 1760
    SITH_ASSERTREL(newMode < SITH_PUPPET_NUMARMEDMODES);
    if ( !pThing->pPuppetClass )
    {
        return;
    }

    pThing->pPuppetState->armedMode = newMode;
    pThing->pPuppetState->majorMode = newMode + (SITH_PUPPET_NUMARMEDMODES * pThing->pPuppetState->moveMode);
}

int J3DAPI sithPuppet_PlayMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback)
{
    int trackNum;
    rdKeyframeFlags flags;
    int lowPriority;
    SithPuppetClassSubmode* pClassMode;
    SithPuppetTrack* pTrack;
    int prioHigh;
    rdKeyframe* pKeyframe;
    SithPuppetTrack* pNewTrack;

#line 1802
    SITH_ASSERTREL(pThing);
    if ( !pThing->pPuppetClass || submode < 0 || submode >= SITH_PUPPET_NUMSUBMODES )
    {
#line 1807
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "PlayMode(): Invalid data for THING '%s' submode %d.\n", pThing->aName, submode, 0, 0);
        return -1;
    }

    pClassMode  = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][submode];
    pKeyframe = pClassMode->pKeyframe;
    if ( !pKeyframe )
    {
        return -1;
    }

    flags        = pClassMode->flags;
    lowPriority  = pClassMode->lowPriority;
    prioHigh = pClassMode->highPriority;

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        if ( pTrack->submode == submode )
        {
            return SITHPUPPET_PLAYERROR_ALREADYPLAYING;
        }

        pTrack = pTrack->pNextTrack;
    }

    if ( submode != SITHPUPPETSUBMODE_FIDGET && submode != SITHPUPPETSUBMODE_FIDGET2 )
    {
        sithPuppet_StopFridgetTrack(pThing);
    }

    trackNum = sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, lowPriority, prioHigh, flags, pfCallback);
    if ( trackNum < 0 )
    {
#line 1851
        stdPrintf(sith_pHS->pDebugPrint, J3D_FILE, __LINE__, "PlayMode(): Attempt to add track for THING '%s' submode '%s' failed.\n", pThing->aName, sithPuppet_aStrSubModes[submode], 0, 0);
        return -1;
    }

#line 1856
    pNewTrack = (SithPuppetTrack*)STDMALLOC(sizeof(SithPuppetTrack));
#line 1857
    SITH_ASSERTREL(pNewTrack);

    pNewTrack->pSubmode   = pClassMode;
    pNewTrack->trackNum   = trackNum;
    pNewTrack->submode    = submode;
    pNewTrack->pNextTrack = NULL;
    sithPuppet_AddTrack(pThing, pNewTrack);
    return trackNum;
}

int J3DAPI sithPuppet_PlayForceMoveMode(SithThing* pThing, SithPuppetSubMode submode, rdPuppetTrackCallback pfCallback)
{
    int trackNum;
    rdKeyframeFlags flags;
    int lowPriority;
    SithPuppetClassSubmode* pClassMode;
    SithPuppetTrack* pTrack;
    int prioHigh;
    rdKeyframe* pKeyframe;
    SithPuppetTrack* pNewTrack;

#line 1878
    SITH_ASSERTREL(pThing);
    if ( (pThing->pPuppetClass == NULL) | (submode < 0) || submode > SITH_PUPPET_NUMSUBMODES )
    {
        return -1;
    }

    pClassMode  = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][submode];
    pKeyframe = pClassMode->pKeyframe;
    if ( !pKeyframe )
    {
#line 1887
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No valid track for force-move anim.\n", 0, 0, 0, 0);
        return -1;
    }

    flags        = pClassMode->flags;
    lowPriority  = pClassMode->lowPriority;
    prioHigh = pClassMode->highPriority;

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack == STDMEMORY_FREEDPTR )
        {
            break;
        }

        if ( pTrack->submode == submode )
        {
            return SITHPUPPET_PLAYERROR_ALREADYPLAYING;
        }

        pTrack = pTrack->pNextTrack;
    }

    sithPuppet_StopFridgetTrack(pThing);

    trackNum = sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, lowPriority, prioHigh, flags, pfCallback);
    if ( trackNum < 0 )
    {
#line 1913
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No valid track for force-move anim.\n", 0, 0, 0, 0);
        return -1;
    }

    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
#line 1918
    pNewTrack = (SithPuppetTrack*)STDMALLOC(sizeof(SithPuppetTrack));
#line 1919
    SITH_ASSERTREL(pNewTrack);

    pNewTrack->pSubmode   = pClassMode;
    pNewTrack->trackNum   = trackNum;
    pNewTrack->submode    = submode;
    pNewTrack->pNextTrack = NULL;
    sithPuppet_AddTrack(pThing, pNewTrack);
    return trackNum;
}

int J3DAPI sithPuppet_PlayKey(rdPuppet* pPuppet, rdKeyframe* pTrack, int lowPriority, int highPriority, rdKeyframeFlags flags, rdPuppetTrackCallback pfCallback)
{
    int bAddTrack = 1;
    int track;

#line 1960
    SITH_ASSERTREL(pPuppet && pTrack);

    if ( (flags & RDKEYFRAME_RESTART_ACTIVE) != 0 )
    {
        for ( track = 0; track < (int)RDPUPPET_MAX_TRACKS; ++track )
        {
            if ( pPuppet->aTracks[track].pKFTrack == pTrack )
            {
                rdPuppet_ResetTrack(pPuppet, track);
                bAddTrack = 0;
                break;
            }
        }
    }

    if ( bAddTrack )
    {
        track = rdPuppet_AddTrack(pPuppet, pTrack, lowPriority, highPriority);
        if ( track < 0 )
        {
#line 1982
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not get free track to play in.\n", 0, 0, 0, 0);
            return -1;
        }
    }

    if ( pfCallback )
    {
        rdPuppet_SetCallback(pPuppet, track, pfCallback);
    }
    else
    {
        rdPuppet_SetCallback(pPuppet, track, sithPuppet_DefaultCallback);
    }

    if ( (flags & RDKEYFRAME_NOLOOP) != 0 )
    {
        rdPuppet_SetStatus(pPuppet, track, RDPUPPET_TRACK_NOLOOP);
    }
    else if ( (flags & RDKEYFRAME_FADEOUT_NOLOOP) != 0 )
    {
        rdPuppet_SetStatus(pPuppet, track, RDPUPPET_TRACK_FADEOUT_NOLOOP);
    }
    else if ( (flags & RDKEYFRAME_PAUSE_ON_LAST_FRAME) != 0 )
    {
        rdPuppet_SetStatus(pPuppet, track, RDPUPPET_TRACK_PAUSE_ON_LAST_FRAME);
    }

    if ( (flags & RDKEYFRAME_PUPPET_CONTROLLED) != 0 )
    {
        rdPuppet_SetTrackSpeed(pPuppet, track, 0.0f);
    }

    if ( (flags & RDKEYFRAME_DISABLE_FADEIN) == 0 )
    {
        rdPuppet_FadeInTrack(pPuppet, track, 0.07f);
    }
    else
    {
        rdPuppet_PlayTrack(pPuppet, track);
    }

    pPuppet->aTracks[track].guid = ((sithPlayer_playerNum + 1) << 16) | (uint16_t)(track + 1);
    return track;
}

int J3DAPI sithPuppet_GetTrackNumForGUID(const SithThing* pThing, uint32_t guid)
{
    size_t idx;
    int trackNum = -1;
    const rdPuppet* pPuppet;

#line 2029
    SITH_ASSERTREL(pThing);
#line 2030
    SITH_ASSERTREL(pThing->renderData.pPuppet);

    pPuppet = pThing->renderData.pPuppet;
    for ( idx = 0; idx < RDPUPPET_MAX_TRACKS; ++idx )
    {
        if ( pPuppet->aTracks[idx].guid == guid )
        {
            trackNum = idx;
            break;
        }
    }

    return trackNum;
}

int J3DAPI sithPuppet_StopKey(rdPuppet* pPuppet, int track, float fadeTime)
{
#line 2063
    SITH_ASSERTREL(pPuppet);
    if ( track > RDPUPPET_MAX_TRACKS )
    {
        return 0;
    }

    if ( !pPuppet->aTracks[track].pKFTrack )
    {
        return 0;
    }

    if ( fadeTime > 0.0f )
    {
        rdPuppet_FadeOutTrack(pPuppet, track, fadeTime);
    }
    else
    {
        rdPuppet_RemoveTrack(pPuppet, track);
    }

    return 1;
}

/* Linker-discarded: retail __LINE__ gap 2063..2166 (sithPuppet_StopKey -> sithPuppet_StopForceMove), where the original
   defined it from Indy's DEBUG build; Jedi Knight's sub_4E4A20 (0x004E4A20, after StopKey) is its relative.
   Body restored from that debug-build text, declarations hoisted for C89. */
void J3DAPI sithPuppet_SwapSubMode(SithThing* pThing, SithPuppetSubMode newMode, SithPuppetSubMode oldMode)
{
    SithPuppetClassSubmode* pMode;
    int bStopTrack = 0;
    SithPuppetTrack* pTrack;
    SithPuppetClassSubmode* pOldMode;

#line 2110
    SITH_ASSERTREL(pThing);
    if ( !pThing->renderData.pPuppet || !pThing->pPuppetClass )
    {
        return;
    }

#line 2116
    SITH_ASSERTREL(pThing->pPuppetClass);
    SITH_ASSERTREL(pThing->renderData.pPuppet);

    pMode    = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][newMode];
    pOldMode = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][oldMode];
    if ( !pMode || pMode->pKeyframe != pOldMode->pKeyframe )
    {
        sithPuppet_StopFridgetTrack(pThing);

        pTrack = pThing->pPuppetState->pFirstTrack;
        while ( pTrack )
        {
            if ( pTrack == STDMEMORY_FREEDPTR )
            {
                break;
            }

            if ( pTrack->submode == oldMode )
            {
                bStopTrack = 1;
                break;
            }

            pTrack = pTrack->pNextTrack;
        }

        if ( bStopTrack )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.07f);
        }

        if ( pMode->pKeyframe )
        {
            sithPuppet_PlayMode(pThing, newMode, sithPuppet_DefaultCallback);
        }
    }
}

void J3DAPI sithPuppet_StopForceMove(SithThing* pThing, int bStopTracks)
{
    int bStopTrack;
    SithPuppetTrack* pTrack;
    int i;

#line 2166
    SITH_ASSERTREL(pThing);

    pThing->thingInfo.actorInfo.bForceMovePlay = 0;
    if ( pThing->type != SITH_THING_PLAYER )
    {
        sithPhysics_ResetThingMovement(pThing);
        sithPhysics_FindFloor(pThing, 1);

        pThing->renderData.bSkipBuildingJoints = 0;
        rdVector_Copy3(&pThing->orient.dvec, &pThing->pos);
        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));

        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        return;
    }

    if ( bStopTracks )
    {
        bStopTrack = 0;
        pTrack = pThing->pPuppetState->pFirstTrack;
        while ( pTrack )
        {
            if ( pTrack == STDMEMORY_FREEDPTR )
            {
                break;
            }

            if ( pTrack->pSubmode && (pTrack->pSubmode->flags & RDKEYFRAME_FORCEMOVE) != 0 )
            {
                bStopTrack = 1;
                break;
            }

            pTrack = pTrack->pNextTrack;
        }

        if ( bStopTrack )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.0f);
        }

        pThing->renderData.bSkipBuildingJoints = 0;
        rdVector_Copy3(&pThing->orient.dvec, &pThing->pos);
        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));
    }
    else
    {
        pThing->renderData.bSkipBuildingJoints = 0;
        rdVector_Copy3(&pThing->orient.dvec, &pThing->pos);
        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT:
            pThing->collide.movesize = 0.04f;
            sithPhysics_ResetThingMovement(pThing);
            sithPlayerActions_ClimbDownToIdle(pThing);
            return;

        case SITHPLAYERMOVE_CLIMB_TO_HANG:
        {
            rdVector3 moveNorm;
            SithSurface* pHitSurf;
            rdModel3Mesh* pHitMesh;
            SithThing* pHitThing;
            rdFace* pHitFace;
            float distance;
            SithCollision* pCollision;

                pHitSurf  = NULL;
                pHitFace  = NULL;
                pHitThing = NULL;
                pHitMesh  = NULL;

                sithPhysics_ResetThingMovement(pThing);
                pThing->collide.movesize = 0.01f;
                sithPhysics_ResetThingMovement(pThing);

                rdVector_Neg3(&moveNorm, &rdroid_zVector3);
                sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, 0.07f, pThing->collide.movesize, 0xA00);
                while ( (pCollision = sithCollision_PopStack()) != NULL )
                {
                    if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                    {
                        if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) == 0 )
                        {
                            break;
                        }
                    }
                    else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
                    {
                        if ( (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) == 0 )
                        {
                            break;
                        }
                    }
                    else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && (pCollision->pThingCollided->flags & SITH_TF_STANDON) != 0 )
                    {
                        break;
                    }
                }

                sithCollision_DecreaseStackLevel();

                pThing->collide.movesize = 0.04f;
                distance = sithPlayerActions_SearchForCollision(pThing, &pThing->orient.lvec, &pHitSurf, &pHitFace, &pHitMesh, &pHitThing);
                if ( distance >= 0.0f )
                {
                    sithPlayerActions_ActorGrabLedge(pThing, distance, pHitSurf, pHitFace, pHitMesh, pHitThing);
                    return;
                }

                pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                sithInventory_SetSwimmingInventory(pThing, 1);
                return;
        }

        case SITHPLAYERMOVE_STAND_TO_CRAWL:
            pThing->moveStatus = SITHPLAYERMOVE_CRAWL_STILL;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
            return;

        case SITHPLAYERMOVE_CRAWL_TO_STAND:
            pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
            pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
            pThing->moveInfo.physics.height = 0.09f;
            sithPhysics_FindFloor(pThing, 1);
            sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            return;

        case SITHPLAYERMOVE_PULLINGUP_1M:
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithPhysics_ResetThingMovement(pThing);
            pThing->collide.movesize = 0.04f;
            sithPhysics_FindFloor(pThing, 1);
            sithInventory_SetSwimmingInventory(pThing, 1);
            return;

        case SITHPLAYERMOVE_CRAWL_STILL:
            return;

        case SITHPLAYERMOVE_WHIPSWINGING:
            if ( (pThing->flags & SITH_TF_DESTROYED) == 0 && (pThing->flags & SITH_TF_DYING) == 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }

            sithPhysics_ResetThingMovement(pThing);
            sithPhysics_FindFloor(pThing, 1);
            return;

        case SITHPLAYERMOVE_WHIP_CLIMB_START:
            sithWhip_SetActorWhipClimbIdle(pThing);
            return;

        case SITHPLAYERMOVE_WHIPCLIMBIDLE:
            sithWhip_FinishWhipClimbDismount(pThing);
            return;

        case SITHPLAYERMOVE_JUMPUP:
            pThing->collide.movesize = 0.04f;
            sithPhysics_ResetThingMovement(pThing);
            if ( (pThing->flags & SITH_TF_DESTROYED) == 0 && (pThing->flags & SITH_TF_DYING) == 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_FALLING;
            }

            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FALL, NULL);
            return;

        case SITHPLAYERMOVE_CLIMBIDLE:
        case SITHPLAYERMOVE_CLIMBING_UP:
        case SITHPLAYERMOVE_CLIMBING_DOWN:
        case SITHPLAYERMOVE_CLIMBING_LEFT:
        case SITHPLAYERMOVE_CLIMBING_RIGHT:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBIDLE;
            pThing->collide.movesize = 0.04f;
            if ( pThing->attach.attachedToStructure.pSurfaceAttached )
            {
                sithPlayerActions_MoveToClimbSector(pThing, pThing->attach.attachedToStructure.pSurfaceAttached);
            }

            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLIDLE, NULL);
            return;

        case SITHPLAYERMOVE_HANGING:
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HANGLEDGE, NULL);
            sithPhysics_ResetThingMovement(pThing);
            pThing->collide.movesize = 0.04f;
            return;

        case SITHPLAYERMOVE_PULLINGUP:
            pThing->collide.movesize = 0.04f;
            pThing->collide.type = SITH_COLLIDE_SPHERE;
            sithPhysics_ResetThingMovement(pThing);
            if ( (pThing->flags & SITH_TF_DESTROYED) == 0 && (pThing->flags & SITH_TF_DYING) == 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }

            pThing->moveInfo.physics.flags &= ~SITH_PF_ONWATERSURFACE;
            sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
            sithPhysics_FindFloor(pThing, 1);
            sithInventory_SetSwimmingInventory(pThing, 1);
            return;

        case SITHPLAYERMOVE_STRAFING_LEFT:
        case SITHPLAYERMOVE_STRAFING_RIGHT:
        case SITHPLAYERMOVE_JUMPBACK:
            pThing->collide.movesize = 0.04f;
            sithPhysics_FindFloor(pThing, 0);
            if ( pThing->attach.flags )
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                sithPhysics_ResetThingMovement(pThing);
                return;
            }

            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
            return;

        case SITHPLAYERMOVE_SWIMIDLE:
            sithPhysics_ResetThingMovement(pThing);
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RISING, NULL);
            for ( i = 0; i < 8; ++i )
            {
                sithFX_CreateBubble(pThing);
            }
            return;

        case SITHPLAYERMOVE_PULLING:
            ; // Retail gives PULLING its own jump-table slot (target shared with default); a null statement makes it a distinct label
        default:
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            pThing->collide.movesize = 0.04f;
            sithPhysics_ResetThingMovement(pThing);
            sithPlayerControls_bCutsceneMode = 0;
            if ( !pThing->attach.flags )
            {
                sithPhysics_FindFloor(pThing, 1);
            }
            return;
    }
}

void J3DAPI sithPuppet_DefaultCallback(SithThing* pPuppetThing, int track, rdKeyMarkerType markerType)
{
    SithSoundClassMode sndmode = 0;
    size_t sndModeOffset = 0;
    SithThing* pThing = pPuppetThing;
    SithPuppetTrack* pTrack;
    SithPuppetTrack* pRunTrack;
    SithThing* pThingAttached;
    SithSurface* pSurfaceAttached;
    rdVector3 moveNorm;
    float dist;

    switch ( markerType )
    {
        case (rdKeyMarkerType)0:
            if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
            {
                pTrack = pThing->pPuppetState->pFirstTrack;
                while ( pTrack )
                {
                    if ( pTrack->trackNum == track )
                    {
                        if ( (pTrack->pSubmode->flags & RDKEYFRAME_FORCEMOVE) != 0 )
                        {
                            sithPuppet_StopForceMove(pThing, 0);
                        }

                        break;
                    }

                    pTrack = pTrack->pNextTrack;
                }

                if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
                {
                    sithAIMove_SetGoalReached(pThing->controlInfo.aiControl.pLocal);
                }
            }

            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_PUSHPULL_READY:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;

                case SITHPLAYERMOVE_JUMPROLLBACK:
                    sithCamera_RestoreExtCamera();

                case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
                case SITHPLAYERMOVE_SLIDEDOWNBACK:
                    sithPhysics_ResetThingMovement(pThing);
                    if ( (pThing == sithPlayer_pLocalPlayerThing || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0)
                        && (pThing->moveInfo.physics.flags & (SITH_PF_UNKNOWN_8000000 | SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0 )
                    {
                        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                        if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                            if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
                            {
                                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
                            }
                        }
                        else if ( pThing->attach.flags )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        }
                        else
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                        }
                    }
                    break;

                case SITHPLAYERMOVE_JUMPROLLFWD:
                case SITHPLAYERMOVE_JUMPLEFT:
                case SITHPLAYERMOVE_JUMPRIGHT:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;

                case SITHPLAYERMOVE_JUMPFWD:
                case SITHPLAYERMOVE_JUMPBACK:
                case SITHPLAYERMOVE_LEAPFWD:
                    pRunTrack = sithPuppet_GetModeTrackImpl(pThing, SITHPUPPETSUBMODE_RUN);
                    if ( pRunTrack && pRunTrack->trackNum == track )
                    {
                        break;
                    }

                    if ( pThing->moveStatus == SITHPLAYERMOVE_JUMPBACK && (pThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        pThing->collide.movesize = 0.04f;
                    }
                    else
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                        pThing->collide.movesize = 0.04f;
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD, NULL);
                    }
                    break;
            }

            sithPuppet_FreeTrackByIndex(pThing, track);
            return;

        case RDKEYMARKER_LEFTFOOT:
        case RDKEYMARKER_RIGHTFOOT:
        case RDKEYMARKER_RUNLEFTFOOT:
        case RDKEYMARKER_RUNRIGHTFOOT:
            if ( pThing->renderData.pPuppet->aTracks[track].playSpeed < 0.5f )
            {
                return;
            }

            if ( (pThing->flags & SITH_TF_DYING) != 0 || pThing->type == SITH_THING_CORPSE )
            {
                return;
            }

            if ( !pThing->attach.flags || !pThing->pSoundClass )
            {
                return;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_SLIDE1);
                return;
            }

            sndModeOffset = markerType - 1;
            if ( sndModeOffset > 1 )
            {
                sndModeOffset = markerType - 6;
            }

            if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
            {
                pThingAttached = pThing->attach.attachedToStructure.pThingAttached;
                if ( (pThingAttached->flags & SITH_TF_METAL) != 0 )
                {
                    sndmode = SITHSOUNDCLASS_CREATE;
                }
                else if ( (pThingAttached->flags & SITH_TF_EARTH) != 0 )
                {
                    sndmode = SITHSOUNDCLASS_STOPMOVE;
                }
                else if ( (pThingAttached->flags & SITH_TF_SNOW) != 0 )
                {
                    sndmode = SITHSOUNDCLASS_MOVING;
                }
                else if ( (pThingAttached->flags & SITH_TF_WOOD) != 0 )
                {
                    sndmode = SITHSOUNDCLASS_LWALKHARD;
                }
            }
            else if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
            {
                pSurfaceAttached = pThing->attach.attachedToStructure.pSurfaceAttached;
                if ( (pSurfaceAttached->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_ECHO | SITH_SURFACE_WOOD | SITH_SURFACE_SNOW | SITH_SURFACE_WEB | SITH_SURFACE_EARTH | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER | SITH_SURFACE_METAL | SITH_SURFACE_AETHERIUM | SITH_SURFACE_EARTHECHO)) != 0 )
                {
                    if ( (pSurfaceAttached->flags & SITH_SURFACE_METAL) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_CREATE;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_WATER) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_ACTIVATE;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_SHALLOWWATER) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_STARTMOVE;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_EARTH) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_STOPMOVE;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_SNOW) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_MOVING;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_WOOD) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_LWALKHARD;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_ECHO) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_RWALKHARD;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_WOODECHO) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_LRUNHARD;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_EARTHECHO) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_RRUNHARD;
                    }
                    else if ( (pSurfaceAttached->flags & SITH_SURFACE_AETHERIUM) != 0 )
                    {
                        sndmode = SITHSOUNDCLASS_LWALKMETAL;
                    }
                    else
                    {
                        sndmode = 0;
                    }
                }
            }

            sithSoundClass_PlayModeRandom(pThing, (SithSoundClassMode)(sndModeOffset + (4 * sndmode) + 6));
            if ( sndmode == SITHSOUNDCLASS_ACTIVATE || sndmode == SITHSOUNDCLASS_STARTMOVE )
            {
                sithFX_CreateWaterRipple(pThing);
            }

            if ( pThing == sithPlayer_pLocalPlayerThing
                && (markerType == RDKEYMARKER_RUNLEFTFOOT || markerType == RDKEYMARKER_RUNRIGHTFOOT)
                && sithPuppet_bPlayerLeapForward == 1
                && pThing->attach.flags )
            {
                if ( pThing->moveStatus != SITHPLAYERMOVE_LEAPFWD )
                {
                    int unusedLeap = 0;
                    int leapTrack;

                    pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
                    if ( markerType == RDKEYMARKER_RUNLEFTFOOT )
                    {
                        leapTrack = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPLEFT, NULL);
                    }
                    else
                    {
                        leapTrack = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPRIGHT, NULL);
                    }

                    sithPuppet_bPlayerLeapForward = 0;
                }
                else
                {
                    sithPuppet_bPlayerLeapForward = 0;
                }
            }
            return;

        case RDKEYMARKER_SWING:
            pThing->unknownFlags |= 2u;
            return;

        case RDKEYMARKER_SWINGFINISH:
            pThing->unknownFlags &= ~2u;
            return;

        case RDKEYMARKER_SWIMLEFT:
            if ( pThing->renderData.pPuppet->aTracks[track].playSpeed < 0.5f )
            {
                return;
            }

            if ( !pThing->pSoundClass )
            {
                return;
            }

            if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LSWIMAET);
                return;
            }

            if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LSWIMSURFACE);
            }
            else
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LSWIMUNDER);
            }
            return;

        case RDKEYMARKER_SWIMRIGHT:
            if ( pThing->renderData.pPuppet->aTracks[track].playSpeed < 0.5f )
            {
                return;
            }

            if ( !pThing->pSoundClass )
            {
                return;
            }

            if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RSWIMAET);
                return;
            }

            if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RSWIMSURFACE);
            }
            else
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RSWIMUNDER);
            }
            return;

        case RDKEYMARKER_TREAD:
            if ( pThing->renderData.pPuppet->aTracks[track].playSpeed < 0.5f )
            {
                return;
            }

            if ( !pThing->pSoundClass )
            {
                return;
            }

            if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_TREADAET);
                return;
            }

            if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_TREADSURFACE);
            }
            else
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_TREADUNDER);
            }
            return;

        case RDKEYMARKER_DIED:
            if ( pThing->renderData.pPuppet->aTracks[track].playSpeed < 0.5f )
            {
                return;
            }

            if ( pThing != sithPlayer_pLocalPlayerThing && !pThing->attach.flags )
            {
                return;
            }

            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_CORPSEHIT);
            return;

        case RDKEYMARKER_JUMP:
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_JUMPLEFT:
                    sithPlayerActions_JumpLeft(pThing);
                    return;

                case SITHPLAYERMOVE_JUMPRIGHT:
                    sithPlayerActions_JumpRight(pThing);
                    return;

                case SITHPLAYERMOVE_JUMPROLLBACK:
                    sithPlayerActions_JumpRollBack(pThing);
                    return;

                case SITHPLAYERMOVE_JUMPROLLFWD:
                    sithPlayerActions_JumpRollForward(pThing);
                    return;

                case SITHPLAYERMOVE_JUMPBACK:
                    sithPlayerActions_HopBack(pThing);
                    return;

                case SITHPLAYERMOVE_JUMPFWD:
                    sithPlayerActions_JumpForward(pThing);
                    return;

                case SITHPLAYERMOVE_LEAPFWD:
                    pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
                    if ( pThing->attach.flags )
                    {
                        sithPlayerActions_LeapForward(pThing);
                    }
                    return;
            }

            if ( pThing->controlType == SITH_CT_AI )
            {
                pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_MOVING;
            }
            return;

        case RDKEYMARKER_JUMPUP:
            if ( !pThing->attach.flags )
            {
                return;
            }

            sithPlayerActions_Jump(pThing, 2.0f, 0);
            if ( pThing->controlType == SITH_CT_AI )
            {
                pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_MOVING;
            }
            return;

        case RDKEYMARKER_ATTACK:
            if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
            {
                int weaponID;
                SithInventoryType* pInvType;
                SithAIControlBlock* pLocal;

                weaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
                pInvType = sithInventory_GetType(weaponID);
                if ( pInvType->pCog )
                {
                    sithCog_SendMessageEx(pInvType->pCog, SITHCOG_MSG_FIRE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0, -1, 0, 0, 0);
                }
                else
                {
                    pLocal = pThing->controlInfo.aiControl.pLocal;
                    sithAIUtil_AIFire(pLocal, pLocal->minFireDist, pLocal->maxFireDist, pLocal->fireDot, pLocal->aimError, pLocal->fireWeaponNum, pLocal->fireFlags, 1);
                }
            }
            else if ( pThing->controlType == SITH_CT_PLAYER )
            {
                int weaponID;
                SithInventoryType* pInvType;

                weaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
                pInvType = sithInventory_GetType(weaponID);
                if ( pInvType->pCog )
                {
                    sithCog_SendMessageEx(pInvType->pCog, SITHCOG_MSG_FIRE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0, -1, 0, 0, 0);
                }
            }
            return;

        case RDKEYMARKER_ACTIVATE:
        case RDKEYMARKER_PLACERIGHTARM:
        case RDKEYMARKER_PLACERIGHTARMREST:
        case RDKEYMARKER_REACHRIGHTARM:
        case RDKEYMARKER_REACHRIGHTARMREST:
        case RDKEYMARKER_PICKUP:
        case RDKEYMARKER_DROP:
        case RDKEYMARKER_MOVE:
        case RDKEYMARKER_INVENTORYPULL:
        case RDKEYMARKER_INVENTORYPUT:
        case RDKEYMARKER_ATTACKFINISH:
        case RDKEYMARKER_TURNOFF:
            if ( (pThing->moveStatus == SITHPLAYERMOVE_PUSHING || pThing->moveStatus == SITHPLAYERMOVE_PULLING) && markerType == RDKEYMARKER_MOVE )
            {
                SithSurface* pSurface;
                int scrapeMode;

                pSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
                if ( !pSurface )
                {
                    return;
                }

                if ( (pSurface->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_ECHO | SITH_SURFACE_WOOD | SITH_SURFACE_SNOW | SITH_SURFACE_WEB | SITH_SURFACE_EARTH | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER | SITH_SURFACE_METAL | SITH_SURFACE_AETHERIUM | SITH_SURFACE_EARTHECHO)) != 0 )
                {
                    if ( (pSurface->flags & SITH_SURFACE_METAL) != 0 )
                    {
                        scrapeMode = 1;
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_EARTH) != 0 )
                    {
                        scrapeMode = 4;
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_SNOW) != 0 )
                    {
                        scrapeMode = 5;
                    }
                    else if ( (pSurface->flags & SITH_SURFACE_WOOD) != 0 )
                    {
                        scrapeMode = 6;
                    }
                    else
                    {
                        scrapeMode = 0;
                    }
                }

                switch ( scrapeMode )
                {
                    case 0:
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_SCRAPEHARD);
                        break;

                    case 1:
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_SCRAPEMETAL);
                        break;

                    default:
                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_SCRAPEEARTH);
                        break;
                }

                return;
            }
            else
            {
                sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, track, markerType, 0, 0);
            }
            return;

        case RDKEYMARKER_RUNJUMPLAND:
            if ( pThing->moveType != SITH_MT_PHYSICS )
            {
                return;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_CRAWL_STILL )
            {
                pThing->forceMoveStartPos.z = pThing->forceMoveStartPos.z + 0.04f;
                return;
            }

            moveNorm.x = 0.0f;
            moveNorm.y = 0.0f;
            moveNorm.z = -1.0f;
            if ( (dist = sithCollision_CheckDistance(pThing, &moveNorm)) > 0.14f )
            {
                sithPuppet_StopForceMove(pThing, 1);
                if ( pThing->moveStatus != SITHPLAYERMOVE_JUMPBACK && pThing->moveStatus != SITHPLAYERMOVE_FALLING )
                {
                    memset(&pThing->moveInfo.physics.thrust, 0, sizeof(pThing->moveInfo.physics.thrust));
                    pThing->moveInfo.physics.velocity.x *= 80.0f;
                    pThing->moveInfo.physics.velocity.y *= 80.0f;
                }

                pThing->moveStatus = SITHPLAYERMOVE_FALLING;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FALL, NULL);
                return;
            }
            return;

        case RDKEYMARKER_LEFTHAND:
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);
            return;

        case RDKEYMARKER_RIGHTHAND:
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDRIGHT);
            return;

        case RDKEYMARKER_CRAWL:
            if ( pThing->moveStatus == SITHPLAYERMOVE_STAND_TO_CRAWL )
            {
                pThing->moveInfo.physics.height = 0.045f;
                pThing->moveInfo.physics.flags |= SITH_PF_ALIGNSURFACE;
                pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNUP;
                sithPlayerActions_Crawl(pThing);
            }
            return;
    }
}

/* Linker-discarded: retail __LINE__ gap 2166..2954 (sithPuppet_StopForceMove -> sithPuppet_ReadStaticPuppetsListText), where the original
   defined it from Indy's DEBUG build. Body restored from that debug-build text, declarations
   hoisted for C89. */
int J3DAPI sithPuppet_WriteStaticPuppetsListText(const SithWorld* pWorld)
{
    size_t i;

    if ( stdConffile_WriteLine("###### Animation Classes #######\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: ANIMCLASS\n\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World puppets %d\n", pWorld->numPuppetClasses) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numPuppetClasses; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aPuppetClasses[i].aName) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("end\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithPuppet_ReadStaticPuppetsListText(SithWorld* pWorld, int bSkip)
{
    size_t numPuppets;

#line 2954
    SITH_ASSERTREL(pWorld);

    if ( bSkip )
    {
        return 1;
    }

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "world") || strcmp(stdConffile_entry.aArgs[1].argValue, "puppets") )
    {
        goto error;
    }

    numPuppets = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !numPuppets )
    {
        return 0;
    }

    if ( sithPuppet_AllocWorldPuppets(pWorld, numPuppets) )
    {
        return 1;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        if ( !sithPuppet_LoadPuppetClass(stdConffile_entry.aArgs[1].argValue) )
        {
#line 2980
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Load failed for puppet %s.\n", stdConffile_entry.aArgs[1].argValue, 0, 0, 0);
        }
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithPuppet_WriteStaticPuppetsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    size_t written;

    for ( i = 0; i < pWorld->numPuppetClasses; ++i )
    {
        written = sith_pHS->pFileWrite(fh, &pWorld->aPuppetClasses[i].aName, STD_ARRAYLEN(pWorld->aPuppetClasses[i].aName));
        if ( written != STD_ARRAYLEN(pWorld->aPuppetClasses[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithPuppet_ReadStaticPuppetsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    const char* pName;
    char* aFilenames = NULL;
    size_t numPuppetClasses = pWorld->numPuppetClasses;
    size_t sizeNames;
    size_t sizeRead;

    if ( sithPuppet_AllocWorldPuppets(pWorld, pWorld->numPuppetClasses) )
    {
        goto error;
    }

    sizeNames = numPuppetClasses * 64;
#line 3015
    aFilenames = (char*)STDMALLOC(sizeNames);
    if ( !aFilenames )
    {
        goto error;
    }

    sizeRead = sith_pHS->pFileRead(fh, aFilenames, sizeNames);
    if ( sizeRead != sizeNames )
    {
        goto error;
    }

    pName = aFilenames;
    for ( i = 0; i < numPuppetClasses; ++i )
    {
        if ( !sithPuppet_LoadPuppetClass(pName) )
        {
            goto error;
        }

        pName += 64;
    }

    stdMemory_Free(aFilenames);
    return 0;

error:
    if ( aFilenames )
    {
        stdMemory_Free(aFilenames);
    }

    return 1;
}

SithPuppetClass* J3DAPI sithPuppet_LoadPuppetClass(const char* pFilename)
{
    SithWorld* pWorld;
    SithPuppetClass* pClass;
    char szPath[128];

    pWorld = sithWorld_pLastLoadedWorld;
#line 3063
    SITH_ASSERTREL(pWorld);
#line 3064
    SITH_ASSERTREL(pFilename);

    pClass = sithPuppet_ClassCacheFind(pFilename);
    if ( pClass )
    {
        return pClass;
    }

    if ( pWorld->numPuppetClasses == pWorld->sizePuppetClasses )
    {
#line 3073
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many puppet classes loaded, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return NULL;
    }

    pClass = &pWorld->aPuppetClasses[pWorld->numPuppetClasses];
    memset(pClass, 0, sizeof(SithPuppetClass));
    strncpy(pClass->aName, pFilename, STD_ARRAYLEN(pClass->aName) - 1);
    pClass->aName[STD_ARRAYLEN(pClass->aName) - 1] = 0;

    sprintf(szPath, "%s%c%s", "misc\\pup", '\\', pFilename);
    if ( !sithPuppet_LoadPuppetClassEntry(pClass, szPath) )
    {
        ++pWorld->numPuppetClasses;
        sithPuppet_ClassCacheAdd(pClass);
        return pClass;
    }

#line 3098
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to load anim class %s.\n", szPath, 0, 0, 0);
    return NULL;
}

int J3DAPI sithPuppet_LoadPuppetClassEntry(SithPuppetClass* pClass, const char* pPath)
{
    size_t modeNum = 0;
    size_t basedOn;
    size_t jointNum;
    int jointId;
    size_t animNum;
    rdKeyframeFlags flags;
    int lowPriority;
    int highPriority;
    rdKeyframe* pKeyframe;

#line 3135
    SITH_ASSERTREL(pPath);
#line 3136
    SITH_ASSERTREL(pClass);

    if ( !stdConffile_Open(pPath) )
    {
        return 1;
    }

    for ( jointNum = 0; jointNum < STD_ARRAYLEN(pClass->aJoints); ++jointNum )
    {
        pClass->aJoints[jointNum] = -1;
    }

    while ( stdConffile_ReadArgs() )
    {
        if ( stdConffile_entry.numArgs < 1 )
        {
#line 3153
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad line encountered line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
            continue;
        }

        if ( !strcmp(stdConffile_entry.aArgs[0].argName, "mode") )
        {
            modeNum = atoi(stdConffile_entry.aArgs[0].argValue);
#line 3161
            SITH_ASSERTREL((modeNum >= 0) && (modeNum < (SITH_PUPPET_NUMARMEDMODES*SITH_PUPPET_NUMMOVEMODES)));

            if ( stdConffile_entry.numArgs > 1 && !strcmp(stdConffile_entry.aArgs[1].argName, "basedon") )
            {
                basedOn = atoi(stdConffile_entry.aArgs[1].argValue);
#line 3167
                SITH_ASSERTREL((basedOn >= 0) && (basedOn < (SITH_PUPPET_NUMARMEDMODES*SITH_PUPPET_NUMMOVEMODES)));
                memcpy(pClass->aModes[modeNum], pClass->aModes[basedOn], sizeof(pClass->aModes[modeNum]));
            }
        }
        else if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "joints") )
        {
            while ( stdConffile_ReadArgs() && stdConffile_entry.numArgs >= 1 && strcmp(stdConffile_entry.aArgs[0].argName, "end") )
            {
                jointNum = atoi(stdConffile_entry.aArgs[0].argName);
                jointId  = atoi(stdConffile_entry.aArgs[0].argValue);
                if ( jointNum >= STD_ARRAYLEN(pClass->aJoints) )
                {
#line 3183
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Joint id %d out of range, line %d, file %s.\n", jointNum, stdConffile_GetLineNumber(), stdConffile_GetFilename(), 0);
                    continue;
                }

                pClass->aJoints[jointNum] = jointId;
            }
        }
        else if ( stdConffile_entry.numArgs > 1
            && (animNum = (size_t)stdHashtbl_Find(sithPuppet_pHashtblSubmodes, stdConffile_entry.aArgs[0].argValue)) != 0 )
        {
#line 3193
            SITH_ASSERTREL(animNum < SITH_PUPPET_NUMSUBMODES);

            if ( stdConffile_entry.numArgs > 2 )
            {
                sscanf(stdConffile_entry.aArgs[2].argValue, "%x", &flags);
            }
            else
            {
                flags = 0;
            }

            if ( stdConffile_entry.numArgs > 3 )
            {
                lowPriority = atoi(stdConffile_entry.aArgs[3].argValue);
            }
            else
            {
                lowPriority = 0;
            }

            if ( stdConffile_entry.numArgs > 4 )
            {
                highPriority = atoi(stdConffile_entry.aArgs[4].argValue);
            }
            else
            {
                highPriority = lowPriority;
            }

            if ( strcmp(stdConffile_entry.aArgs[1].argValue, "none") )
            {
                pKeyframe = sithPuppet_LoadKeyframe(stdConffile_entry.aArgs[1].argValue);
            }
            else
            {
                pKeyframe = NULL;
            }

            pClass->aModes[modeNum][animNum].pKeyframe    = pKeyframe;
            pClass->aModes[modeNum][animNum].flags        = flags;
            pClass->aModes[modeNum][animNum].lowPriority  = lowPriority;
            pClass->aModes[modeNum][animNum].highPriority = highPriority;
        }
        else
        {
#line 3225
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "File %s, unrecognized command %s on line %d.\n", pPath, stdConffile_entry.aArgs[0].argValue, stdConffile_GetLineNumber(), 0);
        }
    }

    stdConffile_Close();
    return 0;
}

int J3DAPI sithPuppet_AllocWorldPuppets(SithWorld* pWorld, size_t size)
{
#line 3252
    SITH_ASSERTREL(pWorld != NULL);
    if ( size )
    {
#line 3256
        pWorld->aPuppetClasses = (SithPuppetClass*)STDMALLOC(sizeof(SithPuppetClass) * size);
        if ( !pWorld->aPuppetClasses )
        {
#line 3258
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No memory to allocate %d anim classes.\n", size, 0, 0, 0);
            return 1;
        }
    }

    pWorld->sizePuppetClasses = size;
    pWorld->numPuppetClasses  = 0;
    memset(pWorld->aPuppetClasses, 0, sizeof(SithPuppetClass) * pWorld->sizePuppetClasses);
    return 0;
}

void J3DAPI sithPuppet_FreeWorldPuppets(SithWorld* pWorld)
{
    size_t i;

#line 3290
    SITH_ASSERTREL(pWorld != NULL);
    if ( !pWorld->sizePuppetClasses )
    {
#line 3294
        SITH_ASSERTREL(pWorld->aPuppetClasses == NULL);
        return;
    }

#line 3298
    SITH_ASSERTREL(pWorld->aPuppetClasses != NULL);
    for ( i = 0; i < pWorld->numPuppetClasses; ++i )
    {
        sithPuppet_ClassCacheRemove(&pWorld->aPuppetClasses[i]);
    }

    stdMemory_Free(pWorld->aPuppetClasses);
    pWorld->aPuppetClasses    = NULL;
    pWorld->numPuppetClasses  = 0;
    pWorld->sizePuppetClasses = 0;
}

/* Linker-discarded: retail __LINE__ gap 3298..3359 (sithPuppet_FreeWorldPuppets -> sithPuppet_ReadStaticKeyframesListText), where the original
   defined it from Indy's DEBUG build. Body restored from that debug-build text, declarations
   hoisted for C89. */
int J3DAPI sithPuppet_WriteStaticKeyframesListText(const SithWorld* pWorld)
{
    size_t i;

    if ( stdConffile_WriteLine("##### Keyframe information #####\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: KEYFRAMES\n\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World keyframes %d\n", pWorld->numKeyframes + 32) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numKeyframes; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aKeyframes[i].aName) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("end\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithPuppet_ReadStaticKeyframesListText(SithWorld* pWorld, int bSkip)
{
    float progress;
    float progressDelta;
    int numKeyframes;

    progress = 80.0f;
    if ( bSkip )
    {
        return 1;
    }

#line 3359
    SITH_ASSERTREL(pWorld != NULL);
#line 3360
    SITH_ASSERTREL(pWorld->aKeyframes == NULL);
#line 3361
    SITH_ASSERTREL(pWorld->numKeyframes == 0);
#line 3362
    SITH_ASSERTREL(pWorld->sizeKeyframes == 0);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "world") || strcmp(stdConffile_entry.aArgs[1].argValue, "keyframes") )
    {
#line 3368
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading keyframe list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    numKeyframes = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !numKeyframes )
    {
#line 3376
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Keyframes section not needed if numKeyframes == 0.\n", 0, 0, 0, 0);
        return 0;
    }

    progressDelta = SITHWORLD_PROGRESSSTEP(15.0f, numKeyframes);
    if ( sithPuppet_AllocWorldKeyframes(pWorld, numKeyframes) )
    {
        goto alloc_error;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        if ( !sithPuppet_LoadKeyframe(stdConffile_entry.aArgs[1].argValue) )
        {
            goto parse_error;
        }

        progress = (progress += progressDelta) < 95.0f ? progress : 95.0f;
        sithWorld_UpdateLoadProgress(progress);
    }

#line 3400
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Keyframes read.  Keyframe array space %d, used %d.\n", pWorld->sizeKeyframes, pWorld->numKeyframes, 0, 0);
#line 3402
    SITH_ASSERTREL(pWorld->numKeyframes <= pWorld->sizeKeyframes);
    return 0;

parse_error:
#line 3407
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error while reading keyframes, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;

alloc_error:
#line 3411
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory error while reading keyframes, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;
}

int J3DAPI sithPuppet_WriteStaticKeyframesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    int bError = 1;
    CndKeyframeInfo* aKeyInfos    = NULL;
    CndKeyframeMarker* aMarkers   = NULL;
    CndKeyframeNode* aNodes       = NULL;
    rdKeyframeNodeEntry* aEntries = NULL;
    size_t sizeInfos = sizeof(CndKeyframeInfo) * pWorld->numKeyframes;
    uint32_t aSizes[3]; // Note, must be 32 bit
    size_t nWritten;
    size_t sizeMarkers;
    size_t sizeNodes;
    size_t sizeEntries;
    CndKeyframeInfo* pCurInfo;
    CndKeyframeMarker* pCurMarker;
    CndKeyframeNode* pCurNode;
    rdKeyframeNodeEntry* pCurEntry;
    size_t i;
    size_t j;
    size_t k;
    rdKeyframe* pKey;

#line 3460
    aKeyInfos = (CndKeyframeInfo*)STDMALLOC(sizeInfos);
    if ( !aKeyInfos )
    {
        goto cleanup;
    }

    memset(aKeyInfos, 0, sizeInfos);
    memset(aSizes, 0, sizeof(aSizes));

    // Pre-calculate all sizes
    for ( i = 0; i < pWorld->numKeyframes; ++i )
    {
        aSizes[0] += pWorld->aKeyframes[i].numMarkers;
        for ( j = 0; j < pWorld->aKeyframes[i].numJoints; ++j )
        {
            if ( pWorld->aKeyframes[i].aNodes[j].numEntries > 0 )
            {
                ++aSizes[1];
            }

            aSizes[2] += pWorld->aKeyframes[i].aNodes[j].numEntries;
        }
    }

    sizeMarkers = sizeof(CndKeyframeMarker) * aSizes[0];
#line 3478
    aMarkers = (CndKeyframeMarker*)STDMALLOC(sizeMarkers);
    if ( !aMarkers )
    {
        goto cleanup;
    }

    memset(aMarkers, 0, sizeMarkers);
    sizeNodes = sizeof(CndKeyframeNode) * aSizes[1];
#line 3483
    aNodes = (CndKeyframeNode*)STDMALLOC(sizeNodes);
    if ( !aNodes )
    {
        goto cleanup;
    }

    memset(aNodes, 0, sizeNodes);
    sizeEntries = sizeof(rdKeyframeNodeEntry) * aSizes[2];
#line 3488
    aEntries = (rdKeyframeNodeEntry*)STDMALLOC(sizeEntries);
    if ( !aEntries )
    {
        goto cleanup;
    }

    memset(aEntries, 0, sizeEntries);

    pCurInfo   = aKeyInfos;
    pCurMarker = aMarkers;
    pCurNode   = aNodes;
    pCurEntry  = aEntries;

    for ( i = 0; i < pWorld->numKeyframes; ++i )
    {
        pKey = &pWorld->aKeyframes[i];

        strncpy(pCurInfo->name, pKey->aName, STD_ARRAYLEN(pCurInfo->name) - 1);
        pCurInfo->name[STD_ARRAYLEN(pCurInfo->name) - 1] = 0;
        pCurInfo->flags      = pKey->flags;
        pCurInfo->type       = pKey->type;
        pCurInfo->numFrames  = pKey->numFrames;
        pCurInfo->fps        = pKey->fps;
        pCurInfo->numMarkers = pKey->numMarkers;
        pCurInfo->numJoints  = pKey->numJoints;
        pCurInfo->numNodes   = 0;

        for ( j = 0; j < pKey->numJoints; ++j )
        {
            if ( pKey->aNodes[j].numEntries > 0 )
            {
                ++pCurInfo->numNodes;
                pCurNode->nodeNum = j;
                pCurNode->numEntries = pKey->aNodes[j].numEntries;

                strncpy(pCurNode->meshName, pKey->aNodes[j].aMeshName, STD_ARRAYLEN(pCurNode->meshName) - 1);
                pCurNode->meshName[STD_ARRAYLEN(pCurNode->meshName) - 1] = 0;
                memcpy(pCurEntry, pKey->aNodes[j].aEntries, sizeof(rdKeyframeNodeEntry) * pCurNode->numEntries);

                pCurEntry += pCurNode->numEntries;
                ++pCurNode;
            }
        }

        for ( k = 0; k < pKey->numMarkers; ++k )
        {
            pCurMarker->frame = pKey->aMarkerFrames[k];
            pCurMarker->type  = pKey->aMarkerTypes[k];
            ++pCurMarker;
        }

        ++pCurInfo;
    }

    bError = 0; // TODO: this should probably be but in the most inner if scope
    nWritten = sith_pHS->pFileWrite(fh, aSizes, sizeof(aSizes));
    if ( nWritten != sizeof(aSizes) )
    {
        goto cleanup;
    }

    nWritten = sith_pHS->pFileWrite(fh, aKeyInfos, sizeInfos);
    if ( nWritten != sizeInfos )
    {
        goto cleanup;
    }

    nWritten = sith_pHS->pFileWrite(fh, aMarkers, sizeMarkers);
    if ( nWritten != sizeMarkers )
    {
        goto cleanup;
    }

    nWritten = sith_pHS->pFileWrite(fh, aNodes, sizeNodes);
    if ( nWritten != sizeNodes )
    {
        goto cleanup;
    }

    nWritten = sith_pHS->pFileWrite(fh, aEntries, sizeEntries);
    if ( nWritten != sizeEntries )
    {
        goto cleanup;
    }

cleanup:
    if ( aKeyInfos )
    {
        stdMemory_Free(aKeyInfos);
    }

    if ( aMarkers )
    {
        stdMemory_Free(aMarkers);
    }

    if ( aNodes )
    {
        stdMemory_Free(aNodes);
    }

    if ( aEntries )
    {
        stdMemory_Free(aEntries);
    }

    return bError;
}

int J3DAPI sithPuppet_ReadStaticKeyframesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    int bError = 1;
    CndKeyframeInfo* aInfos       = NULL;
    CndKeyframeMarker* aMarkers   = NULL;
    CndKeyframeNode* aNodes       = NULL;
    rdKeyframeNodeEntry* aEntries = NULL;
    size_t numKeyframes = pWorld->numKeyframes;
    size_t sizeInfos;
    uint32_t aSizes[3];
    size_t sizeMarkers;
    size_t sizeNodes;
    size_t sizeEntries;
    size_t nRead;
    const CndKeyframeInfo* pCurInfo;
    const CndKeyframeMarker* pCurMarker;
    const CndKeyframeNode* pCurNode;
    const rdKeyframeNodeEntry* pCurEntry;
    size_t i;
    size_t j;
    size_t k;
    rdKeyframe* pKeyframe;
    rdKeyframeNode* pNode;
    size_t sizeKeyNodes;
    size_t nodeNum;

    if ( sithPuppet_AllocWorldKeyframes(pWorld, pWorld->numKeyframes) )
    {
        goto cleanup;
    }

    sizeInfos = sizeof(CndKeyframeInfo) * numKeyframes;
#line 3582
    aInfos = (CndKeyframeInfo*)STDMALLOC(sizeInfos);
    if ( !aInfos )
    {
        goto cleanup;
    }

    memset(aInfos, 0, sizeInfos);
    nRead = sith_pHS->pFileRead(fh, aSizes, sizeof(aSizes));
    if ( nRead != sizeof(aSizes) )
    {
        goto cleanup;
    }

    sizeMarkers = sizeof(CndKeyframeMarker) * aSizes[0];
#line 3590
    aMarkers = (CndKeyframeMarker*)STDMALLOC(sizeMarkers);
    if ( !aMarkers )
    {
        goto cleanup;
    }

    memset(aMarkers, 0, sizeMarkers);
    sizeNodes = sizeof(CndKeyframeNode) * aSizes[1];
#line 3595
    aNodes = (CndKeyframeNode*)STDMALLOC(sizeNodes);
    if ( !aNodes )
    {
        goto cleanup;
    }

    memset(aNodes, 0, sizeNodes);
    sizeEntries = sizeof(rdKeyframeNodeEntry) * aSizes[2];
#line 3600
    aEntries = (rdKeyframeNodeEntry*)STDMALLOC(sizeEntries);
    if ( !aEntries )
    {
        goto cleanup;
    }

    memset(aEntries, 0, sizeEntries);
    nRead = sith_pHS->pFileRead(fh, aInfos, sizeInfos);
    if ( nRead != sizeInfos )
    {
        goto cleanup;
    }

    nRead = sith_pHS->pFileRead(fh, aMarkers, sizeMarkers);
    if ( nRead != sizeMarkers )
    {
        goto cleanup;
    }

    nRead = sith_pHS->pFileRead(fh, aNodes, sizeNodes);
    if ( nRead != sizeNodes )
    {
        goto cleanup;
    }

    nRead = sith_pHS->pFileRead(fh, aEntries, sizeEntries);
    if ( nRead != sizeEntries )
    {
        goto cleanup;
    }

    pCurInfo   = aInfos;
    pCurMarker = aMarkers;
    pCurNode   = aNodes;
    pCurEntry  = aEntries;

    for ( i = 0; i < numKeyframes; ++i )
    {
        if ( sithPuppet_KeyCacheFind(pCurInfo->name) )
        {
            pCurMarker += pCurInfo->numMarkers;
            for ( j = 0; j < pCurInfo->numNodes; ++j )
            {
                pCurEntry += pCurNode->numEntries;
                ++pCurNode;
            }

            ++pCurInfo;
            continue;
        }

        pKeyframe = &pWorld->aKeyframes[pWorld->numKeyframes];
        strncpy(pKeyframe->aName, pCurInfo->name, STD_ARRAYLEN(pKeyframe->aName) - 1);
        pKeyframe->aName[STD_ARRAYLEN(pKeyframe->aName) - 1] = 0;

        pKeyframe->flags      = pCurInfo->flags;
        pKeyframe->type       = pCurInfo->type;
        pKeyframe->numFrames  = pCurInfo->numFrames;
        pKeyframe->fps        = pCurInfo->fps;
        pKeyframe->numMarkers = pCurInfo->numMarkers;
        pKeyframe->numJoints  = pCurInfo->numJoints;

        for ( k = 0; k < pCurInfo->numMarkers; ++k )
        {
            pKeyframe->aMarkerFrames[k] = pCurMarker->frame;
            pKeyframe->aMarkerTypes[k]  = pCurMarker->type;
            ++pCurMarker;
        }

        pKeyframe->idx = pWorld->numKeyframes;
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pKeyframe->idx = SITHWORLD_STATICINDEX(pKeyframe->idx);
        }

        sizeKeyNodes = sizeof(rdKeyframeNode) * pCurInfo->numJoints;
#line 3662
        pKeyframe->aNodes = (rdKeyframeNode*)STDMALLOC(sizeKeyNodes);
        if ( !pKeyframe->aNodes )
        {
            goto cleanup;
        }

        memset(pKeyframe->aNodes, 0, sizeKeyNodes);
        for ( j = 0; j < pCurInfo->numNodes; ++j )
        {
            nodeNum = pCurNode->nodeNum;
            pNode = &pKeyframe->aNodes[nodeNum];
            pNode->nodeNum    = nodeNum;
            pNode->numEntries = pCurNode->numEntries;
            strncpy(pNode->aMeshName, pCurNode->meshName, STD_ARRAYLEN(pNode->aMeshName) - 1);
            pNode->aMeshName[STD_ARRAYLEN(pNode->aMeshName) - 1] = 0;

#line 3675
            pNode->aEntries = (rdKeyframeNodeEntry*)STDMALLOC(sizeof(rdKeyframeNodeEntry) * pNode->numEntries);
            if ( !pNode->aEntries )
            {
                goto cleanup;
            }

            memcpy(pNode->aEntries, pCurEntry, sizeof(rdKeyframeNodeEntry) * pNode->numEntries);
            pCurEntry += pCurNode->numEntries;
            ++pCurNode;
        }

        sithPuppet_KeyCacheAdd(pKeyframe);
        ++pCurInfo;
        ++pWorld->numKeyframes;
    }

    bError = 0;

cleanup:
    if ( aInfos )
    {
        stdMemory_Free(aInfos);
    }

    if ( aMarkers )
    {
        stdMemory_Free(aMarkers);
    }

    if ( aNodes )
    {
        stdMemory_Free(aNodes);
    }

    if ( aEntries )
    {
        stdMemory_Free(aEntries);
    }

    return bError;
}

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithPuppet_countfit_1, 15)
CF_PROTO(sithPuppet_countfit_2, 15)
CF_PROTO(sithPuppet_countfit_3, 15)
CF_PROTO(sithPuppet_countfit_4, 15)
CF_PROTO(sithPuppet_countfit_5, 15)
CF_PROTO(sithPuppet_countfit_6, 15)
CF_PROTO(sithPuppet_countfit_7, 15)
CF_PROTO(sithPuppet_countfit_8, 15)
CF_PROTO(sithPuppet_countfit_9, 15)
CF_PROTO(sithPuppet_countfit_10, 15)
CF_PROTO(sithPuppet_countfit_11, 8)

/* end count-fit */

rdKeyframe* J3DAPI sithPuppet_GetKeyframeByIndex(int index)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        pWorld = sithWorld_pStaticWorld;
        index = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( index < 0 || index >= (signed int)pWorld->numKeyframes )
    {
        return NULL;
    }

    return &pWorld->aKeyframes[index];
}

rdKeyframe* J3DAPI sithPuppet_LoadKeyframe(const char* pName)
{
    SithWorld* pWorld;
    rdKeyframe* pKey;
    char aCurPath[128];

    pWorld = sithWorld_pLastLoadedWorld;
#line 3753
    SITH_ASSERTREL(pWorld != NULL);
#line 3754
    SITH_ASSERTREL(pName != NULL);
#line 3755
    SITH_ASSERTREL(strlen(pName) < 128);

    if ( !pWorld->aKeyframes )
    {
#line 3759
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to load keyframe '%s'.\n", pName, 0, 0, 0);
        return NULL;
    }

#line 3763
    SITH_ASSERT(pWorld->aKeyframes != NULL);
    sprintf(aCurPath, "%s%c%s", "3do\\key", '\\', pName);
    pKey = sithPuppet_KeyCacheFind(pName);
    if ( pKey )
    {
        return pKey;
    }

#line 3775
    SITH_ASSERTREL(pWorld->numKeyframes <= pWorld->sizeKeyframes);
    if ( pWorld->numKeyframes >= pWorld->sizeKeyframes )
    {
#line 3780
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to load new keyframe '%s'.\n", pName, 0, 0, 0);
        return NULL;
    }

    pKey = &pWorld->aKeyframes[pWorld->numKeyframes];
    if ( rdKeyframe_LoadEntry(aCurPath, pKey) )
    {
        pKey->idx = pWorld->numKeyframes;
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pKey->idx = SITHWORLD_STATICINDEX(pKey->idx);
        }

        sithPuppet_KeyCacheAdd(pKey);
        ++pWorld->numKeyframes;
        return pKey;
    }

#line 3802
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to load keyframe '%s'.\n", aCurPath, 0, 0, 0);
    return NULL;
}

int J3DAPI sithPuppet_AllocWorldKeyframes(SithWorld* pWorld, size_t size)
{
#line 3827
    SITH_ASSERTREL(pWorld != NULL);

#line 3829
    pWorld->aKeyframes = (rdKeyframe*)STDMALLOC(sizeof(rdKeyframe) * size);
    if ( !pWorld->aKeyframes )
    {
#line 3831
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No memory to allocate %d keyframe buffers.\n", size, 0, 0, 0);
        return 1;
    }

    pWorld->sizeKeyframes = size;
    pWorld->numKeyframes  = 0;
    memset(pWorld->aKeyframes, 0, sizeof(rdKeyframe) * pWorld->sizeKeyframes);
    return 0;
}

void J3DAPI sithPuppet_FreeWorldKeyframes(SithWorld* pWorld)
{
    size_t i;

#line 3862
    SITH_ASSERTREL(pWorld != NULL);
    if ( !pWorld->sizeKeyframes )
    {
#line 3866
        SITH_ASSERTREL(pWorld->aKeyframes == NULL);
        return;
    }

#line 3870
    SITH_ASSERTREL(pWorld->aKeyframes != NULL);
    for ( i = 0; i < pWorld->numKeyframes; ++i )
    {
        sithPuppet_KeyCacheRemove(&pWorld->aKeyframes[i]);
        rdKeyframe_FreeEntry(&pWorld->aKeyframes[i]);
    }

    stdMemory_Free(pWorld->aKeyframes);
    pWorld->aKeyframes    = NULL;
    pWorld->numKeyframes  = 0;
    pWorld->sizeKeyframes = 0;
}

SithPuppetClass* J3DAPI sithPuppet_ClassCacheFind(const char* pName)
{
#line 3892
    SITH_ASSERTREL(pName != NULL);
#line 3893
    SITH_ASSERTREL(sithPuppet_pClassHashtable != NULL);
    return (SithPuppetClass*)stdHashtbl_Find(sithPuppet_pClassHashtable, pName);
}

int J3DAPI sithPuppet_ClassCacheAdd(const SithPuppetClass* pClass)
{
#line 3901
    SITH_ASSERTREL(pClass != NULL);
#line 3902
    SITH_ASSERTREL(strlen(pClass->aName) > 0);
#line 3903
    SITH_ASSERTREL(sithPuppet_pClassHashtable != NULL);
    return stdHashtbl_Add(sithPuppet_pClassHashtable, pClass->aName, (void*)pClass);
}

int J3DAPI sithPuppet_ClassCacheRemove(const SithPuppetClass* pClass)
{
#line 3911
    SITH_ASSERTREL(pClass != NULL);
#line 3912
    SITH_ASSERTREL(strlen(pClass->aName) > 0);
#line 3913
    SITH_ASSERTREL(sithPuppet_pClassHashtable != NULL);
    return stdHashtbl_Remove(sithPuppet_pClassHashtable, pClass->aName);
}

rdKeyframe* J3DAPI sithPuppet_GetKeyframe(const char* pName)
{
    return sithPuppet_KeyCacheFind(pName);
}

rdKeyframe* J3DAPI sithPuppet_KeyCacheFind(const char* pName)
{
#line 3925
    SITH_ASSERTREL(pName != NULL);
#line 3926
    SITH_ASSERTREL(sithPuppet_pKeyHashtable != NULL);
    return (rdKeyframe*)stdHashtbl_Find(sithPuppet_pKeyHashtable, pName);
}

void J3DAPI sithPuppet_KeyCacheAdd(rdKeyframe* pKeyframe)
{
#line 3934
    SITH_ASSERTREL(pKeyframe != NULL);
#line 3935
    SITH_ASSERTREL(strlen(pKeyframe->aName) > 0);
#line 3936
    SITH_ASSERTREL(sithPuppet_pKeyHashtable != NULL);
    stdHashtbl_Add(sithPuppet_pKeyHashtable, pKeyframe->aName, pKeyframe);
}

void J3DAPI sithPuppet_KeyCacheRemove(rdKeyframe* pKeyframe)
{
#line 3944
    SITH_ASSERTREL(pKeyframe != NULL);
#line 3945
    SITH_ASSERTREL(strlen(pKeyframe->aName) > 0);
#line 3946
    SITH_ASSERTREL(sithPuppet_pKeyHashtable != NULL);
    stdHashtbl_Remove(sithPuppet_pKeyHashtable, pKeyframe->aName);
}

static __inline float J3DAPI sithPuppet_GetThingLocalVelocityAxis(const SithThing* pThing, size_t* pAxis, float* pVelX, float* pVelY, float* pVelZ)
{
    const SithPhysicsInfo* pPhysics;
    rdVector3 localVel;

    pPhysics = &pThing->moveInfo.physics;

#line 3981
    SITH_ASSERTREL(pAxis);
#line 3982
    SITH_ASSERTREL(pThing);

    if ( !pThing->pInSector )
    {
        *pVelX = 0.0f;
        *pVelY = 0.0f;
        *pVelZ = 0.0f;
        return 0.0f;
    }

    if ( pPhysics->velocity.x == 0.0f && pPhysics->velocity.y == 0.0f && pPhysics->velocity.z == 0.0f )
    {
        *pVelX = 0.0f;
        *pVelY = 0.0f;
        *pVelZ = 0.0f;
        *pAxis = 99;
        return 0.0f;
    }

    rdMatrix_TransformVectorOrtho34(&localVel, &pPhysics->velocity, &pThing->orient);
    if ( !pThing->attach.flags
        && (pPhysics->flags & SITH_PF_FLY) == 0
        && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0
        && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
    {
        *pAxis = 2;
        *pVelX = localVel.x;
        *pVelY = localVel.y;
        *pVelZ = pPhysics->velocity.z;
        return (pPhysics->velocity.z);
    }

    if ( (localVel.y < 0.0f ? -localVel.y : localVel.y) > (localVel.x < 0.0f ? -localVel.x : localVel.x) )
    {
        *pAxis = 1;
        *pVelX = localVel.x;
        *pVelY = localVel.y;
        *pVelZ = pPhysics->velocity.z;
        return (localVel.y);
    }

    *pAxis = 0;
    *pVelX = localVel.x;
    *pVelY = localVel.y;
    *pVelZ = pPhysics->velocity.z;
    return (localVel.x);
}

int J3DAPI sithPuppet_StopMode(SithThing* pThing, SithPuppetSubMode submode, float fadeOutTime)
{
    SithPuppetClassSubmode* pCurrentSubmode;
    rdKeyframe* pKeyframe;
    int track;

#line 4063
    SITH_ASSERTREL(pThing && pThing->pPuppetState && pThing->renderData.pPuppet);

    if ( !pThing->pPuppetClass || submode < 0 || submode >= SITH_PUPPET_NUMSUBMODES )
    {
        return 1;
    }

    pCurrentSubmode  = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][submode];
    pKeyframe = pCurrentSubmode->pKeyframe;
    if ( !pKeyframe )
    {
        return 1;
    }

    for ( track = 0; track < (int)RDPUPPET_MAX_TRACKS; ++track )
    {
        if ( pThing->renderData.pPuppet->aTracks[track].pKFTrack == pKeyframe )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, track, fadeOutTime);
        }
    }

    return 0;
}

int J3DAPI sithPuppet_SynchMode(SithThing* pThing, SithPuppetSubMode oldMode, SithPuppetSubMode newMode, float a4, int bReverse)
{
    SithPuppetTrack* pTrack;
    int trackNum = -1;
    int newTrackIdx = -1;
    float srcFramePos;
    float trackProgress;
    rdPuppet* pPuppet;
    float frame;
    rdKeyframe* pOldKF;
    rdKeyframe* pNewKeyframe;

#line 4121
    SITH_ASSERTREL(pThing);

    if ( !J3DINRANGE(oldMode, SITHPUPPETSUBMODE_STAND, SITHPUPPETSUBMODE_FALLFORWARD) )
    {
#line 4125
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SynchMode(): Old mode (%d) out of range.\n", oldMode, 0, 0, 0);
        return -1;
    }

    if ( !J3DINRANGE(newMode, SITHPUPPETSUBMODE_STAND, SITHPUPPETSUBMODE_FALLFORWARD) )
    {
#line 4130
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SynchMode(): New mode (%d) out of range.\n", newMode, 0, 0, 0);
        return -1;
    }

    pOldKF = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][oldMode].pKeyframe;
    pNewKeyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][newMode].pKeyframe;
    if ( !pOldKF || !pNewKeyframe )
    {
#line 4139
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SynchMode(): THING '%s' doesn't possess mode '%s' or '%s'.\n", pThing->aName, sithPuppet_aStrSubModes[oldMode], sithPuppet_aStrSubModes[newMode], 0);
        return -1;
    }

    pTrack = sithPuppet_GetModeTrackImpl(pThing, oldMode);
    if ( !pTrack )
    {
#line 4147
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SynchMode(): Old Mode '%s' not playing for THING '%s'.\n", sithPuppet_aStrSubModes[oldMode], pThing->aName, 0, 0);
        return -1;
    }

    trackNum    = pTrack->trackNum;
    newTrackIdx = sithPuppet_PlayMode(pThing, newMode, NULL);
    if ( newTrackIdx == -1 )
    {
#line 4156
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SynchMode(): Failed to play new mode '%s' for THING '%s'.\n", sithPuppet_aStrSubModes[newMode], pThing->aName, 0, 0);
        return -1;
    }

    if ( newTrackIdx == SITHPUPPET_PLAYERROR_ALREADYPLAYING )
    {
        return -1;
    }

    pPuppet = pThing->renderData.pPuppet;
    if ( (pPuppet->aTracks[trackNum].status & RDPUPPET_TRACK_PAUSED) != 0 )
    {
        return -1;
    }

    srcFramePos = pPuppet->aTracks[trackNum].curFrame;
    trackProgress = D3DVAL(srcFramePos / pPuppet->aTracks[trackNum].pKFTrack->numFrames);
    if ( bReverse )
    {
        frame = D3DMultiply(pPuppet->aTracks[newTrackIdx].pKFTrack->numFrames, D3DVAL(1.0f - trackProgress));
    }
    else
    {
        frame = D3DMultiply(pPuppet->aTracks[newTrackIdx].pKFTrack->numFrames, trackProgress);
    }

    pPuppet->aTracks[newTrackIdx].curFrame  = frame;
    pPuppet->aTracks[newTrackIdx].prevFrame = frame;

    if ( newMode < SITHPUPPETSUBMODE_JUMPROLLFWD || newMode > SITHPUPPETSUBMODE_GRABARMS )
    {
        pThing->pPuppetState->submode = newMode;
    }

    sithPuppet_StopKey(pPuppet, trackNum, 0.0f);
    return newTrackIdx;
}

void J3DAPI sithPuppet_ClearMode(SithThing* pThing, SithPuppetSubMode mode)
{
    SithPuppetTrack* pTrack;

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack->submode == mode )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.0f);
            sithPuppet_FreeTrack(pThing, pTrack);
            return;
        }

        pTrack = pTrack->pNextTrack;
    }
}

void J3DAPI sithPuppet_RemoveTrackByIndex(SithThing* pThing, int trackNum)
{
    SithPuppetTrack* pTrack;

    pTrack = pThing->pPuppetState->pFirstTrack;
    while ( pTrack )
    {
        if ( pTrack->trackNum == trackNum )
        {
            sithPuppet_FreeTrack(pThing, pTrack);
            return;
        }

        pTrack = pTrack->pNextTrack;
    }
}

int J3DAPI sithPuppet_NewTrack(SithThing* pThing, SithPuppetClassSubmode* pSubmode, int trackNum, SithPuppetSubMode submode)
{
    SithPuppetTrack* pNewTrack;

#line 4243
    pNewTrack = (SithPuppetTrack*)STDMALLOC(sizeof(SithPuppetTrack));
    if ( !pNewTrack )
    {
        return 1;
    }

    pNewTrack->pSubmode   = pSubmode;
    pNewTrack->trackNum   = trackNum;
    pNewTrack->submode    = submode;
    pNewTrack->pNextTrack = NULL;
    sithPuppet_AddTrack(pThing, pNewTrack);
    return 0;
}

void J3DAPI sithPuppet_AddTrack(SithThing* pThing, SithPuppetTrack* pNewTrack)
{
    SithPuppetTrack* pCurTrack;
    int numTracks = 0;

#line 4261
    SITH_ASSERTREL(pNewTrack);

    pCurTrack = pThing->pPuppetState->pFirstTrack;
    if ( !pCurTrack )
    {
        pThing->pPuppetState->pFirstTrack = pNewTrack;
        return;
    }

    numTracks = 1;
    while ( pCurTrack->pNextTrack )
    {
        pCurTrack = pCurTrack->pNextTrack;
        ++numTracks;
    }

    pCurTrack->pNextTrack = pNewTrack;
}

void J3DAPI sithPuppet_RemoveTrack(SithThing* pThing, SithPuppetTrack* pTrack)
{
    // Note, could actually be sithPuppet_FreeTrack;
    sithPuppet_FreeTrack(pThing, pTrack);
}

void J3DAPI sithPuppet_FreeTrack(SithThing* pThing, SithPuppetTrack* pTrack)
{
    SithPuppetTrack* pCurTrack;
    SithPuppetTrack* pPrevTrack;
    SithPuppetTrack* pNextTrack;

    pCurTrack = pThing->pPuppetState->pFirstTrack;
    if ( pCurTrack )
    {
        if ( pCurTrack == pTrack )
        {
            pThing->pPuppetState->pFirstTrack = pCurTrack->pNextTrack;
            stdMemory_Free(pCurTrack);
        }
        else
        {
            pPrevTrack = pCurTrack;
            pCurTrack = pPrevTrack->pNextTrack;
            while ( pCurTrack )
            {
                if ( pCurTrack == pTrack )
                {
                    pNextTrack = pCurTrack->pNextTrack;
                    stdMemory_Free(pCurTrack);
                    pPrevTrack->pNextTrack = pNextTrack;
                    return;
                }

                pPrevTrack = pCurTrack;
                pCurTrack = pCurTrack->pNextTrack;
            }
        }
    }
}

void J3DAPI sithPuppet_FreeTrackByIndex(SithThing* pThing, int trackNum)
{
    SithPuppetTrack* pCurTrack;
    SithPuppetTrack* pPrevTrack;
    SithPuppetTrack* pNextTrack;

    pCurTrack = pThing->pPuppetState->pFirstTrack;
    if ( pCurTrack && pCurTrack != STDMEMORY_FREEDPTR )
    {
        if ( pCurTrack->trackNum == trackNum )
        {
            pThing->pPuppetState->pFirstTrack = pCurTrack->pNextTrack;
            stdMemory_Free(pCurTrack);
        }
        else
        {
            pPrevTrack = pCurTrack;
            pCurTrack = pPrevTrack->pNextTrack;
            while ( pCurTrack && pCurTrack != STDMEMORY_FREEDPTR )
            {
                if ( pCurTrack->trackNum == trackNum )
                {
                    pNextTrack = pCurTrack->pNextTrack;
                    stdMemory_Free(pCurTrack);
                    pPrevTrack->pNextTrack = pNextTrack;
                    return;
                }

                pPrevTrack = pCurTrack;
                pCurTrack = pCurTrack->pNextTrack;
            }
        }
    }
}

SithPuppetTrack* J3DAPI sithPuppet_GetModeTrack(const SithThing* pThing, SithPuppetSubMode mode)
{
    return sithPuppet_GetModeTrackImpl(pThing, mode);
}

SithPuppetTrack* J3DAPI sithPuppet_GetModeTrackImpl(const SithThing* pThing, SithPuppetSubMode submode)
{
    SithPuppetTrack* pCurTrack = pThing->pPuppetState->pFirstTrack;
    while ( pCurTrack )
    {
        if ( pCurTrack->submode == submode )
        {
            return pCurTrack;
        }

        pCurTrack = pCurTrack->pNextTrack;
        if ( pCurTrack == STDMEMORY_FREEDPTR )
        {
            pCurTrack = NULL;
        }
    }

    return NULL;
}

void J3DAPI sithPuppet_ClearTrackList(SithThing* pThing)
{
    SithPuppetTrack* pNextTrack;
    SithPuppetTrack* pCurTrack;

    pCurTrack = pThing->pPuppetState->pFirstTrack;
    while ( pCurTrack )
    {
        pNextTrack = pCurTrack->pNextTrack;
        stdMemory_Free(pCurTrack);
        pCurTrack = pNextTrack;
    }

    pThing->pPuppetState->pFirstTrack = NULL;
}