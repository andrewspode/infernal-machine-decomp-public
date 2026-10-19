#include "sithAI.h"

#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAIInstinct.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWeapon.h>

#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <stdlib.h>
#include <string.h>

static tHashTable* pRegisteredInstinctHashtbl = NULL;
static size_t numFreeAIs = 0;
static size_t aFreeAIIndices[SITHAI_MAXAIS] = { 0 };
int sithAI_bOpen = 0;
static int bStartup = 0;
static SithAIRegisteredInstinct* aRegisteredInstincts = NULL;
static size_t numRegisteredInstincts = 0;

SithAIControlBlock sithAI_aControlBlocks[SITHAI_MAXAIS];
int sithAI_lastUsedAIIndex;

void J3DAPI sithAI_ProcessAIState(SithAIControlBlock* pLocal);
void J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal);
int J3DAPI sithAI_ProcessBlockedEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject);
int J3DAPI sithAI_ProcessUnhandledEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject);
int J3DAPI sithAI_ProcessUnhandledWpntEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject);
int J3DAPI sithAI_CreateInstinctRegistry(size_t maxInstincts);
void J3DAPI sithAI_FreeAI(size_t aiNum);
int J3DAPI sithAI_CreateAI(void);
void J3DAPI sithAI_ResetAllAIs(void);

int J3DAPI sithAI_Startup(void)
{
    if ( !bStartup )
    {
        if ( sithAI_CreateInstinctRegistry(SITHAI_MAXREGISTEREDINSTINCTS) )
        {
            return 1;
        }

        sithAIInstinct_InitInstincts();
        sithAI_ResetAllAIs();
        bStartup = 1;
    }
    else
    {
#line 135
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already initialized!\n", 0, 0, 0, 0);
    }

    return 0;
}

void J3DAPI sithAI_Shutdown(void)
{
    if ( bStartup )
    {
#line 155
        SITH_ASSERTREL(aRegisteredInstincts && pRegisteredInstinctHashtbl);
        stdMemory_Free(aRegisteredInstincts);
        stdHashtbl_Free(pRegisteredInstinctHashtbl);
        bStartup = 0;
        return;
    }

#line 164
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already shutdown!\n", 0, 0, 0, 0);
}

void J3DAPI sithAI_Open(void)
{
    if ( !bStartup )
    {
#line 185
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Attempt to open uninitialized system!\n", 0, 0, 0, 0);
    }

    if ( sithAI_bOpen )
    {
#line 190
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
    }

    sithAI_bOpen = 1;
}

void J3DAPI sithAI_Close(void)
{
    if ( sithAI_bOpen )
    {
        sithAI_ResetAllAIs();
        sithAI_bOpen = 0;
        return;
    }

#line 218
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
}

void J3DAPI sithAI_Create(SithThing* pThing)
{
    SithAIClass* pClass;
    SithAIControlBlock* pLocal;
    int aiIdx;

#line 243
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);

    pClass = pThing->controlInfo.aiControl.pClass;
    if ( !pClass )
    {
        pThing->controlType = SITH_CT_PLOT;
        return;
    }

    aiIdx = sithAI_CreateAI();
    if ( aiIdx < 0 )
    {
#line 259
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many AI things, not enough local control blocks.\n", 0, 0, 0, 0);
        pThing->controlType = SITH_CT_PLOT;
        return;
    }

    pThing->controlInfo.aiControl.pLocal = &sithAI_aControlBlocks[aiIdx];
    pLocal = pThing->controlInfo.aiControl.pLocal;

    rdVector_Copy3(&pLocal->homePos, &pThing->pos);
    rdVector_Copy3(&pLocal->homeOrient, &pThing->orient.lvec);

    pLocal->pClass       = pClass;
    pLocal->pOwner       = pThing;
    pLocal->numInstincts = pClass->numInstincts;
    pLocal->mode         = SITHAI_MODE_SLEEPING | SITHAI_MODE_SEARCHING;
    pLocal->moveSpeed    = 1.5f;

    if ( pThing->pPuppetClass && pThing->pPuppetState )
    {
        pThing->pPuppetState->armedMode = pClass->armedMode;
        pThing->pPuppetState->majorMode = pClass->armedMode + (SITH_PUPPET_NUMARMEDMODES * pThing->pPuppetState->moveMode);
    }

    pLocal->allowedSurfaceTypes = 0;
}

void J3DAPI sithAI_Free(SithThing* pThing)
{
    size_t index;

#line 314
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);

    if ( pThing->controlInfo.aiControl.pLocal )
    {
        index = pThing->controlInfo.aiControl.pLocal - sithAI_aControlBlocks;
#line 320
        SITH_ASSERTREL((index >= 0) && (index < SITHAI_MAXAIS));

        if ( pThing->controlInfo.aiControl.pLocal->aFrames )
        {
            stdMemory_Free(pThing->controlInfo.aiControl.pLocal->aFrames);
            pThing->controlInfo.aiControl.pLocal->aFrames = NULL;
        }

        sithAI_FreeAI(index);
        pThing->controlInfo.aiControl.pLocal = NULL;
    }
}

void J3DAPI sithAI_Process(float secDeltaTime)
{
    int index;
    SithAIControlBlock* pLocal;
    for ( index = 0, pLocal = sithAI_aControlBlocks; index <= sithAI_lastUsedAIIndex; ++index, ++pLocal )
    {
        if ( !pLocal->pClass || (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) )
        {
            continue;
        }

        if ( pLocal->pOwner->thingInfo.actorInfo.health <= 0.0f )
        {
            continue;
        }

        if ( pLocal->pOwner->thingInfo.actorInfo.bForceMovePlay == 1 )
        {
            continue;
        }

        if ( (pLocal->mode & SITHAI_MODE_BLOCK) )
        {
            continue;
        }

        if ( (pLocal->mode & (SITHAI_MODE_DISABLED | SITHAI_MODE_SLEEPING)) )
        {
            continue;
        }

        sithAI_ProcessAIState(pLocal);
        sithWeapon_UpdateActorWeaponState(pLocal->pOwner, secDeltaTime);

        if ( pLocal->msecNextUpdate > sithTime_msecGameTime )
        {
            continue;
        }

        if ( sithAIMove_AIGetMoveState(pLocal) > 0 )
        {
            continue;
        }

        sithAI_InstinctUpdate(pLocal);
    }
}

void J3DAPI sithAI_ProcessAIState(SithAIControlBlock* pLocal)
{
    switch ( pLocal->pOwner->thingInfo.actorInfo.aiState )
    {
        case 0:
            if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_STAND2WALK, SITHPUPPETSUBMODE_WALK2ATTACK) )
            {
                return;
            }

            sithPuppet_RemoveAllTracks(pLocal->pOwner);
            sithPuppet_SetArmedMode(pLocal->pOwner, pLocal->pOwner->thingInfo.actorInfo.aiArmedModeState);

            pLocal->pOwner->pPuppetState->submode = 0;
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_STILL;
            sithAIMove_UpdateAIMove(pLocal);
            break;

        case 1:
            if ( sithTime_msecGameTime < pLocal->msecPauseMoveUntil )
            {
                return;
            }

            sithAIMove_StartSpecialMove(pLocal, pLocal->pOwner->thingInfo.actorInfo.aiArmedModeState);
            break;

        default:
            return;
    }

    sithCog_ThingSendMessageEx(pLocal->pOwner, 0, SITHCOG_MSG_STATECHANGE, pLocal->pOwner->thingInfo.actorInfo.aiState, pLocal->pOwner->thingInfo.actorInfo.aiArmedModeState, 0, 0);
    pLocal->pOwner->thingInfo.actorInfo.aiState = -1;
    pLocal->pOwner->thingInfo.actorInfo.aiArmedModeState = -1;
}

void J3DAPI sithAI_InstinctUpdate(SithAIControlBlock* pLocal)
{
    uint32_t msecNextUpdate;
    SithAIMode curMode;
    size_t i;
    int bCurHandled;

    msecNextUpdate = sithTime_msecGameTime + 5000;
    curMode = pLocal->mode;

restart:
    for ( i = 0; i < pLocal->numInstincts; ++i )
    {
        if ( (pLocal->aInstinctStates[i].flags & SITHAI_INSTINCT_DISABLED) )
        {
            continue;
        }

        if ( !(pLocal->mode & pLocal->pClass->aInstincts[i].updateModes) || (pLocal->mode & pLocal->pClass->aInstincts[i].updateBlockModes) )
        {
            continue;
        }

        if ( pLocal->aInstinctStates[i].msecNextUpdate <= sithTime_msecGameTime )
        {
            pLocal->aInstinctStates[i].msecNextUpdate = sithTime_msecGameTime + 1000;
            bCurHandled = pLocal->pClass->aInstincts[i].pfInstinct(pLocal, &pLocal->pClass->aInstincts[i], &pLocal->aInstinctStates[i], (SithAIEventType)0, 0);

            if ( curMode != pLocal->mode )
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)curMode);
            }

            if ( bCurHandled )
            {
                curMode = pLocal->mode;
                goto restart;
            }
        }

        msecNextUpdate = J3DMIN(pLocal->aInstinctStates[i].msecNextUpdate, msecNextUpdate);
    }

    pLocal->msecNextUpdate = msecNextUpdate;
}

int J3DAPI sithAI_ForceInstinctUpdate(SithAIControlBlock* pLocal, const char* pInstinctName, int bSendModeChangeEvent)
{
    SithAIMode prevMode;
    SithAIRegisteredInstinct* pInstinct;
    int currentinstIdx;
    int bCurHandled;

#line 554
    SITH_ASSERTREL(pLocal);

    prevMode = pLocal->mode;
    pInstinct = sithAI_FindInstinct(pInstinctName);
    if ( !pInstinct )
    {
#line 561
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid instinct '%s' passed to sithAI_ForceInstinctUpdate().\n", pInstinctName, 0, 0, 0);
        return 0;
    }

    currentinstIdx = sithAI_GetInstinctIndex(pLocal, pInstinct);
    if ( currentinstIdx > -1 )
    {
        if ( (pLocal->aInstinctStates[currentinstIdx].flags & SITHAI_INSTINCT_DISABLED) )
        {
            return 0;
        }

        if ( !(pLocal->mode & pLocal->pClass->aInstincts[currentinstIdx].updateModes) || (pLocal->mode & pLocal->pClass->aInstincts[currentinstIdx].updateBlockModes) )
        {
            return 0;
        }

        pLocal->aInstinctStates[currentinstIdx].msecNextUpdate = sithTime_msecGameTime + 1000;
        bCurHandled = pLocal->pClass->aInstincts[currentinstIdx].pfInstinct(pLocal, &pLocal->pClass->aInstincts[currentinstIdx], &pLocal->aInstinctStates[currentinstIdx], (SithAIEventType)0, NULL);

        if ( bSendModeChangeEvent && prevMode != pLocal->mode )
        {
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)prevMode);
        }

        return 1;
    }

    return 0;
}

void J3DAPI sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    SithAIMode prevMode;
    int cogRes = 0;
    int cogParam = 0;
    int instinctRes = 0;
    int bSendCogEvent = 0;

    if ( !pLocal->pClass || !pLocal->pOwner || (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_AIEVENTS_DISABLED) != 0 )
    {
        return;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 )
    {
        return;
    }

    if ( pLocal->pOwner->thingInfo.actorInfo.health <= 0.0f )
    {
        return;
    }

    prevMode = pLocal->mode;
    if ( (pLocal->mode & SITHAI_MODE_SLEEPING) != 0 )
    {
        if ( event == SITHAI_EVENT_SOUND )
        {
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
        }
        else
        {
            return;
        }
    }

    switch ( (unsigned int)event )
    {
        case 0:
            break;

        case SITHAI_EVENT_HIT_SECTOR:
            if ( pObject )
            {
                cogParam = ((const SithThing*)pObject)->idx;
            }

            bSendCogEvent = 1;
            break;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_TARGETED:
        case SITHAI_EVENT_FIRE:
            if ( pObject )
            {
                cogParam = ((const SithThing*)pObject)->idx;
                bSendCogEvent = 1;
            }
            break;

        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_FLOOR:
        case SITHAI_EVENT_LAND_FLOOR:
            if ( pObject )
            {
                int surfIdx = sithSurface_GetSurfaceIndex((const SithSurface*)pObject);
                if ( surfIdx > -1 )
                {
                    cogParam = surfIdx;
                    bSendCogEvent = 1;
                }
            }
            break;

        default:
            cogParam = (int)pObject;
            bSendCogEvent = 1;
            break;
    }

    if ( bSendCogEvent )
    {
        cogRes = sithCog_ThingSendMessageEx(pLocal->pOwner, 0, SITHCOG_MSG_AIEVENT, event, pLocal->mode, cogParam, 0);
    }

    if ( !cogRes && (pLocal->mode & SITHAI_MODE_BLOCK) == 0 && !sithAIMove_AIGetMoveState(pLocal) )
    {
        size_t i;
        for ( i = 0; i < pLocal->numInstincts; ++i )
        {
            if ( (pLocal->aInstinctStates[i].flags & SITHAI_INSTINCT_DISABLED) != 0 )
            {
                continue;
            }

            if ( (pLocal->pClass->aInstincts[i].triggerEvents & event) != 0 )
            {
                instinctRes = pLocal->pClass->aInstincts[i].pfInstinct(pLocal, &pLocal->pClass->aInstincts[i], &pLocal->aInstinctStates[i], event, pObject);
                if ( instinctRes )
                {
                    break;
                }
            }
        }
    }

    if ( !cogRes && !instinctRes )
    {
        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
        {
            instinctRes = sithAI_ProcessUnhandledWpntEvent(pLocal, event, pObject);
        }

        if ( !instinctRes )
        {
            if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 || (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0 )
            {
                sithAI_ProcessBlockedEvent(pLocal, event, pObject);
            }
            else
            {
                sithAI_ProcessUnhandledEvent(pLocal, event, pObject);
            }
        }
    }

    if ( pLocal->mode != prevMode )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)prevMode);
    }
}

int J3DAPI sithAI_ProcessBlockedEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{

#line 776
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    switch ( event )
    {
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
            sithAIMove_Unreachable(pLocal);
            return 1;

        case SITHAI_EVENT_HIT_FLOOR:
            return 1;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_THING:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
            {
                sithAIMove_Unreachable(pLocal);
                return 1;
            }
            break;
    }

    return 0;
}

int J3DAPI sithAI_ProcessUnhandledEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    float minDist;
    rdVector3 pos;
    rdVector3 moveDir;
    int pathFlags;
    float moveDist;
    SithThing* pThing;

#line 825
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(pLocal->pOwner && pLocal->pClass);

    switch ( event )
    {
        SithAIMode prevMode;

        case SITHAI_EVENT_HIT_CLIFF:
            if ( SITH_RAND() <= pLocal->pClass->rank )
            {
                sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &moveDir);
                rdVector_Neg3Acc(&moveDir);

                sithAIMove_StopAIMovement(pLocal);
                sithAIUtil_ApplyForce(pLocal, &moveDir, 0.05f);

                pathFlags = 0xC002; // 0x2 - yaw | 0x4000 - random | 0x8000 - rand neg rvec
                minDist = J3DMAX(pLocal->pOwner->collide.movesize, 0.089999996f);
                moveDist = (SITH_RAND() * 0.2f) + minDist;

                if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
                {
                    pathFlags |= 0x100;
                }

                if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, 60.0f, pathFlags, &moveDir, &pos) )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pos);
                    sithAIMove_AISetMoveTargetPos(pLocal, &pos, 1.0f);
                    return 1;
                }
            }

            sithAIMove_Unreachable(pLocal);
            break;

        case SITHAI_EVENT_HIT_WALL:
            sithAIMove_Unreachable(pLocal);
            return 1;

        case SITHAI_EVENT_HIT_FLOOR:
            return 1;

        case SITHAI_EVENT_TOUCHED:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
            {
                pThing = (SithThing*)pObject;
                if ( pThing )
                {
                    sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &pos);
                    rdVector_Sub3(&moveDir, &pLocal->pOwner->pos, &pThing->pos);
                    moveDir.z = 0.0f;
                    rdVector_Normalize3Acc(&moveDir);
                    if ( rdVector_Dot3(&pos, &moveDir) >= 0.0f )
                    {
                        return 1;
                    }
                }

                sithAIMove_Unreachable(pLocal);
                return 1;
            }
            break;

        case SITHAI_EVENT_HIT_THING:
            sithAIMove_Unreachable(pLocal);
            return 1;

        case SITHAI_EVENT_MODECHANGED:
            prevMode = (SithAIMode)pObject;
            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) != 0 && (prevMode & SITHAI_MODE_CHASE_GOAL) == 0 )
            {
                if ( (pLocal->mode & SITHAI_MODE_NOCHASING) != 0 && pLocal->targetDistance > pLocal->pClass->sightDistance )
                {
                    sithAIMove_StopAIMovement(pLocal);
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
                    return 1;
                }

                if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) != 0 )
                {
                    return 1;
                }

                sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown5, 1.7f);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown5);
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) != 0 )
            {
                sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &moveDir);

                pathFlags = 0xC102;
                minDist = J3DMAX(pLocal->pOwner->collide.movesize, 0.089999996f);
                moveDist = minDist + 0.2f;

                if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, 45.0f, pathFlags, &moveDir, &pos) )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pos);
                    sithAIMove_AISetMovePos(pLocal, &pos, 1.7f);
                    return 1;
                }
            }
    }

    return 0;
}

int J3DAPI sithAI_ProcessUnhandledWpntEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject)
{
    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, 0);
            if ( sithAIUtil_AIMoveToNextWpnt(pLocal, pLocal->moveSpeed, 0.0f, 0) )
            {
                return 1;
            }
    }

    return 0;
}

void J3DAPI sithAI_Stop(SithThing* pThing)
{
    SithAIControlBlock* pLocal;
    if ( !pThing || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
        return;
    }

    sithAIMove_StopAIMovement(pLocal);
    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
    sithAIUtil_ClearAIWaypoint(pLocal);
}

void J3DAPI sithAI_StopAllAttackingAIs(void)
{
    int aiIdx;
    SithAIControlBlock* pLocal;
    for ( aiIdx = 0, pLocal = sithAI_aControlBlocks; aiIdx <= sithAI_lastUsedAIIndex; ++aiIdx, ++pLocal )
    {
        if ( !pLocal || !pLocal->pOwner || pLocal->pOwner->controlType != SITH_CT_AI )
        {
            continue;
        }

        if ( (pLocal->pOwner->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) )
        {
            continue;
        }

        if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
        {
            sithAIMove_AIStop(pLocal);
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        }
    }
}

int J3DAPI sithAI_RegisterInstinct(const char* pName, SithAIInstinctFunc pfInstinct, SithAIMode updateModes, SithAIMode updateBlockModes, SithAIEventType triggerEvents)
{
#line 1103
    SITH_ASSERTREL(pName);
    SITH_ASSERTREL(pfInstinct);

    if ( numRegisteredInstincts >= SITHAI_MAXREGISTEREDINSTINCTS )
    {
        return 0;
    }

    aRegisteredInstincts[numRegisteredInstincts].pfInstinct = pfInstinct;
    strncpy(aRegisteredInstincts[numRegisteredInstincts].aName, pName, STD_ARRAYLEN(aRegisteredInstincts[numRegisteredInstincts].aName) - 1);
    aRegisteredInstincts[numRegisteredInstincts].aName[STD_ARRAYLEN(aRegisteredInstincts[numRegisteredInstincts].aName) - 1] = '\0';
    aRegisteredInstincts[numRegisteredInstincts].updateModes      = updateModes;
    aRegisteredInstincts[numRegisteredInstincts].updateBlockModes = updateBlockModes;
    aRegisteredInstincts[numRegisteredInstincts].triggerEvents    = triggerEvents;
    ++numRegisteredInstincts;
}

SithAIRegisteredInstinct* J3DAPI sithAI_FindInstinct(const char* pInstinctName)
{
    size_t i;

    if ( !pInstinctName )
    {
#line 1142
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Null parameter passed to FindInstinct().\n", 0, 0, 0, 0);
        return 0;
    }

    for ( i = 0; i < numRegisteredInstincts; ++i )
    {
        if ( stdUtil_StrCmp(pInstinctName, aRegisteredInstincts[i].aName) == 0 )
        {
            return &aRegisteredInstincts[i];
        }
    }

    return 0;
}

int J3DAPI sithAI_GetInstinctIndex(const SithAIControlBlock* pLocal, const SithAIRegisteredInstinct* pRegInstinct)
{
    size_t i;
    for ( i = 0; i < pLocal->numInstincts; ++i )
    {
        if ( pLocal->pClass->aInstincts[i].pfInstinct == pRegInstinct->pfInstinct )
        {
            break;
        }
    }

    if ( i < pLocal->numInstincts )
    {
        return i;
    }

    return -1;
}

int J3DAPI sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName)
{
    SithAIRegisteredInstinct* pInstinct;

#line 1201
    SITH_ASSERTREL(pLocal);
    pInstinct = sithAI_FindInstinct(pName);
    if ( pInstinct && sithAI_GetInstinctIndex(pLocal, pInstinct) != -1 )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithAI_EnableInstinct(SithAIControlBlock* pLocal, const char* pInstinctName, int bEnable)
{
    SithAIRegisteredInstinct* pCurInstinct;
    int instinctIdx;

#line 1221
    SITH_ASSERTREL(pLocal);

    pCurInstinct = sithAI_FindInstinct(pInstinctName);
    if ( !pCurInstinct )
    {
#line 1227
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid instinct '%s' passed to EnableInstinct().\n", pInstinctName, 0, 0, 0);
        return 0;
    }

    instinctIdx = sithAI_GetInstinctIndex(pLocal, pCurInstinct);
    if ( instinctIdx == -1 )
    {
        if ( bEnable )
        {
#line 1237
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AI '%s' doesn't possess instinct '%s' for EnableInstinct() call.\n", pLocal->pOwner->aName, pInstinctName, 0, 0);
        }

        return 0;
    }

    if ( bEnable )
    {
        pLocal->aInstinctStates[instinctIdx].flags &= ~SITHAI_INSTINCT_DISABLED;
    }
    else
    {
        pLocal->aInstinctStates[instinctIdx].flags |= SITHAI_INSTINCT_DISABLED;
    }

    return 1;
}

#ifdef J3D_DEBUG
// Debug-build console commands (debug build 0x435146, 0x435213; retail __LINE__ gap 1237..1369), registered only by the
// debug build's sithCommand_RegisterCommands: restored from the debug code.
int J3DAPI sithAI_AIList(const SithConsoleCommand* pFunc, const char* pArg)
{
    int i;
    SithAIControlBlock* pLocal;

    if ( !sithAI_bOpen )
    {
        sithConsole_PrintString("AI system not open.\n");
        return 0;
    }

    sithConsole_PrintString("Active AI things:\n");
    for ( i = 0, pLocal = sithAI_aControlBlocks; i <= sithAI_lastUsedAIIndex; ++i, ++pLocal )
    {
        if ( !pLocal->pClass || !pLocal->pOwner )
        {
            continue;
        }

        sprintf(std_genBuffer, "Block %2d: Class '%s', Owner '%s' (%d), Flags 0x%x\n", i, pLocal->pClass->aName, pLocal->pOwner->aName, pLocal->pOwner->idx, pLocal->mode);
        sithConsole_PrintString(std_genBuffer);
    }

    return 1;
}

int J3DAPI sithAI_AIStatus(const SithConsoleCommand* pFunc, const char* pArg)
{
    int idx;
    SithAIControlBlock* pLocal;
    size_t i;

    if ( !pArg || !sithAI_bOpen || sscanf(pArg, "%d", &idx) != 1 || idx > sithAI_lastUsedAIIndex || idx < 0 )
    {
        sithConsole_PrintString("cannot process AIStatus command.\n");
        return 0;
    }

    pLocal = &sithAI_aControlBlocks[idx];
    if ( !pLocal->pOwner )
    {
        sithConsole_PrintString("That AI block is not currently active.\n");
        return 1;
    }

    sprintf(std_genBuffer, "AI Status dump for thing %d (%s).\n", pLocal->pOwner->idx, pLocal->pOwner->aName);
    sithConsole_PrintString(std_genBuffer);

    sprintf(std_genBuffer, "Class '%s', Flags=0x%x, Moods %d/%d/%d, NextUpdate=%d\n", pLocal->pClass->aName, pLocal->mode, pLocal->aMoods[0], pLocal->aMoods[1], pLocal->aMoods[2], pLocal->msecNextUpdate);
    sithConsole_PrintString(std_genBuffer);

    sithConsole_PrintString("Current instincts:\n");
    for ( i = 0; i < pLocal->numInstincts; ++i )
    {
        sprintf(std_genBuffer, "Instinct %d: Params: %f/%f/%f/%f, nextUpdate=%d, mask=0x%x, mode=0x%x.\n", i, pLocal->aInstinctStates[i].aParams[0], pLocal->aInstinctStates[i].aParams[1], pLocal->aInstinctStates[i].aParams[2], pLocal->aInstinctStates[i].aParams[3], pLocal->aInstinctStates[i].msecNextUpdate, pLocal->pClass->aInstincts[i].triggerEvents, pLocal->pClass->aInstincts[i].updateModes);
        sithConsole_PrintString(std_genBuffer);
    }

    return 1;
}
#endif

int J3DAPI sithAI_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    SithAIControlBlock* pLocal = pThing->controlInfo.aiControl.pLocal;
    rdVector3 framePosition;
    size_t frameNum;
    size_t size;

#line 1369
    SITH_ASSERTREL(pArg && pThing);

    if ( !pLocal )
    {
        goto error;
    }

    switch ( adjNum )
    {
        case SITHTHING_ARG_NUMFRAMES:
            if ( pLocal->sizeFrames )
            {
                goto error;
            }

#line 1380
            SITH_ASSERTREL(!pLocal->aFrames);
            size = atoi(pArg->argValue);
            if ( size < 1 )
            {
                goto error;
            }

            if ( sithAI_AllocAIFrames(pLocal, size) )
            {
                goto alloc_error;
            }
            break;

        case SITHTHING_ARG_FRAME:
            if ( pLocal->numFrames >= pLocal->sizeFrames )
            {
#line 1392
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many AI frames for thing %d.\n", pThing->idx, 0, 0, 0);
                goto error;
            }

            frameNum = pLocal->numFrames;
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &framePosition.x, &framePosition.y, &framePosition.z) != 3 )
            {
                goto error;
            }

            rdVector_Copy3(&pLocal->aFrames[frameNum], &framePosition);
            ++pLocal->numFrames;
            break;

        default:
            return 0;
    }

    return 1;

error:
#line 1413
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber(), 0);
    return 0;

alloc_error:
#line 1417
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory allocation error, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 0;
}

int J3DAPI sithAI_AllocAIFrames(SithAIControlBlock* pLocal, size_t sizeFrames)
{
#line 1424
    pLocal->aFrames = (rdVector3*)STDMALLOC(sizeof(rdVector3) * sizeFrames);
    if ( !pLocal->aFrames )
    {
        return 1;
    }

    memset(pLocal->aFrames, 0, sizeof(rdVector3) * sizeFrames);
    pLocal->sizeFrames = sizeFrames;
    pLocal->numFrames  = 0;
    return 0;
}

// The retail linker discarded this function (/OPT:REF): no retail code calls it and it owns no
// retail bytes. Its first assertion literal survives at 0x00528BC8, inside sithAI's contiguous
// /Gf string run between sithAI_ParseArg's and sithAI_CreateInstinctRegistry's literals, because
// sithPathMove_Create (0x004ACE76) references the pooled copy. That position places the definition
// here in source order. The real algorithm left no retail bytes; the body and the `pivot` parameter are
// restored from the debug build's text. Allowlisted under
// `discarded:` in relocations.yml.
void J3DAPI sithAI_CreateAIFramesFomMarker(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pivot)
{
    SithAIControlBlock* pLocal = pNewThing->controlInfo.aiControl.pLocal;
    size_t i;
    SithPathFrame* pFrame;
    rdVector3 vec;

#line 1442
    SITH_ASSERTREL(pNewThing && pMarker);
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL((pNewThing->controlType == SITH_CT_AI) && (pMarker->moveType == SITH_MT_PATH));
    SITH_ASSERTREL(!pLocal->aFrames);

#line 1448
    pLocal->aFrames    = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pMarker->moveInfo.pathMovement.sizeFrames);
    pLocal->sizeFrames = pMarker->moveInfo.pathMovement.sizeFrames;
    pLocal->numFrames  = pMarker->moveInfo.pathMovement.numFrames;

    for ( i = 0; i < pLocal->numFrames; ++i )
    {
        pFrame = &pMarker->moveInfo.pathMovement.aFrames[i];

        rdVector_Rotate3(&vec, pivot, &pFrame->pyr);

        pLocal->aFrames[i].x = pFrame->pos.x + vec.x;
        pLocal->aFrames[i].y = pFrame->pos.y + vec.y;
        pLocal->aFrames[i].z = pFrame->pos.z + vec.z;
    }
}

int J3DAPI sithAI_CreateInstinctRegistry(size_t maxInstincts)
{
#line 1488
    SITH_ASSERTREL(aRegisteredInstincts == NULL);
#line 1490
    aRegisteredInstincts = (SithAIRegisteredInstinct*)STDMALLOC(sizeof(SithAIRegisteredInstinct) * maxInstincts);
    if ( !aRegisteredInstincts )
    {
#line 1493
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Startup failure on AI - no mem for instinct registry.\n", 0, 0, 0, 0);
        return 1;
    }

    pRegisteredInstinctHashtbl = stdHashtbl_New(2 * maxInstincts);
    if ( !pRegisteredInstinctHashtbl )
    {
        stdMemory_Free(aRegisteredInstincts);
#line 1501
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Startup failure on AI - no mem for hashtable.\n", 0, 0, 0, 0);
        return 1;
    }

    return 0;
}

void J3DAPI sithAI_FreeAI(size_t aiNum)
{
#line 1526
    SITH_ASSERTREL((aiNum < SITHAI_MAXAIS));
    SITH_ASSERTREL(numFreeAIs < SITHAI_MAXAIS);

    memset(&sithAI_aControlBlocks[aiNum], 0, sizeof(SithAIControlBlock));

    if ( aiNum == sithAI_lastUsedAIIndex )
    {
        int i;
        for ( i = (int)aiNum - 1; i >= 0; --i )
        {
            if ( sithAI_aControlBlocks[i].pOwner )
            {
                break;
            }
        }

        sithAI_lastUsedAIIndex = i;
    }

    aFreeAIIndices[numFreeAIs++] = aiNum;
}

int J3DAPI sithAI_CreateAI(void)
{
    size_t freeAIIndex;

    if ( !numFreeAIs )
    {
#line 1567
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: out of object space - Create failure.\n", 0, 0, 0, 0);
        return -1;
    }

    freeAIIndex = aFreeAIIndices[--numFreeAIs];
#line 1573
    SITH_ASSERTREL(freeAIIndex < SITHAI_MAXAIS);

    if ( (int)freeAIIndex > sithAI_lastUsedAIIndex )
    {
        sithAI_lastUsedAIIndex = freeAIIndex;
    }

    return freeAIIndex;
}

void J3DAPI sithAI_ResetAllAIs(void)
{
    int aiNum;

    memset(sithAI_aControlBlocks, 0, sizeof(sithAI_aControlBlocks));
    numFreeAIs = 0;

    for ( aiNum = SITHAI_MAXAIS - 1; aiNum >= 0; --aiNum )
    {
        sithAI_FreeAI(aiNum);
    }
}
