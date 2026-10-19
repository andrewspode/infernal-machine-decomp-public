#include "sithAIInstinct.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#undef rdroid_yVector3
extern const rdVector3 rdroid_yVector3;
#undef rdroid_zVector3
extern const rdVector3 rdroid_zVector3;
#undef sithAIAwareness_aSectors
extern SithAIAwarenessSector* sithAIAwareness_aSectors;
#undef sithAIMove_flt_585464
extern float sithAIMove_flt_585464;
#undef sithAIMove_flt_585468
extern float sithAIMove_flt_585468;
#undef sithMain_frameNumber
extern size_t sithMain_frameNumber;
#undef sith_mode
extern SithMode sith_mode;
#undef sithPlayerControls_bCutsceneMode
extern int sithPlayerControls_bCutsceneMode;
#undef sithPlayer_pLocalPlayerThing
extern SithThing* sithPlayer_pLocalPlayerThing;
#undef sithTime_frameTimeFlex
extern float sithTime_frameTimeFlex;
#undef sithTime_msecGameTime
extern uint32_t sithTime_msecGameTime;
#undef sithTime_secGameTime
extern float sithTime_secGameTime;
#undef sith_pHS
extern tHostServices* sith_pHS;


// Instinct argument and state accessors. The parenthesised expansion matters:
// VC5 keeps the parentheses as an expression node, so a read through these
// macros is loaded onto the x87 stack (fld/fstp copies, fld; fld; fcompp and
// fld; fcomp constant compares) instead of being folded into a memory operand
// or an integer mov/cmp.
#define SITHAI_FLTARG(pInstinct, n) ((pInstinct)->fltArg[n])
#define SITHAI_PARAM(pState, n) ((pState)->aParams[n])

// A random scale of 1 +- 20% (PrimaryFire/AlternateFire fire delays).
#define SITHAIINSTINCT_RANDSCALE20 ((SITH_RAND() * 0.4f) - 0.2f + 1.0f)
#define SITHAIINSTINCT_RANDRANGE(min, max) (rand() % ((max) - (min)) + (min))

// A random fraction of `scale`. A macro, not an `__inline` function: the debug build (/Od /Ob0) spills the
// operand to a temporary around each `rand()` (HoverDrift), and retail multiplies by the operand after the call.
#define SITHAIINSTINCT_RAND(scale) ((scale) * SITH_RAND())

// True once `duration` seconds have passed since game time `secStart` (the HUNT and FLEE time-outs).
// The expansion's brackets around the sum are what retail shows: release adds the duration to the start
// time before it loads sithTime_secGameTime (fadd st(1); fld secGameTime; fcompp at LookForTarget 0x48F976,
// Flee 0x493316); the bare sum loads it first. Under /Od both spellings are the same (debug 0x4B89E7).
#define SITHAIINSTINCT_TIMEDOUT(secStart, duration) (sithTime_secGameTime > ((secStart) + (duration)))

void J3DAPI sithAIInstinct_InitInstincts(void)
{
    sithAI_RegisterInstinct("listen", (SithAIInstinctFunc)sithAIInstinct_Listen, 0, 0, 7);
    sithAI_RegisterInstinct("lookfortarget", (SithAIInstinctFunc)sithAIInstinct_LookForTarget, 0x204, 0x800, 0x100);
    sithAI_RegisterInstinct("sensedanger", (SithAIInstinctFunc)sithAIInstinct_SenseDanger, 0x804, 0, 7);
    sithAI_RegisterInstinct("primaryfire", (SithAIInstinctFunc)sithAIInstinct_PrimaryFire, 2, 0x800, 0);
    sithAI_RegisterInstinct("alternatefire", (SithAIInstinctFunc)sithAIInstinct_AlternateFire, 2, 0x800, 0);
    sithAI_RegisterInstinct("lobfire", (SithAIInstinctFunc)sithAIInstinct_LobFire, 2, 0x800, 0);
    sithAI_RegisterInstinct("opendoors", (SithAIInstinctFunc)sithAIInstinct_OpenDoors, 2, 0, 0);
    sithAI_RegisterInstinct("jump", (SithAIInstinctFunc)sithAIInstinct_Jump, 0, 0, 0x604);
    sithAI_RegisterInstinct("randomturn", (SithAIInstinctFunc)sithAIInstinct_RandomTurn, 4, 0, 0);
    sithAI_RegisterInstinct("roam", (SithAIInstinctFunc)sithAIInstinct_Roam, 4, 0xA12, 0xF04);
    sithAI_RegisterInstinct("flee", (SithAIInstinctFunc)sithAIInstinct_Flee, 0x800, 0, 0xF04);
    sithAI_RegisterInstinct("retreat", (SithAIInstinctFunc)sithAIInstinct_Retreat, 2, 0x1000800, 1);
    sithAI_RegisterInstinct("circlestrafe", (SithAIInstinctFunc)sithAIInstinct_CircleStrafe, 2, 0x1000800, 0x88E04);
    sithAI_RegisterInstinct("returnhome", (SithAIInstinctFunc)sithAIInstinct_ReturnHome, 0, 0, 0x900);
    sithAI_RegisterInstinct("talk", (SithAIInstinctFunc)sithAIInstinct_Talk, -1, 0, 0x400104);
    sithAI_RegisterInstinct("hitandrun", (SithAIInstinctFunc)sithAIInstinct_HitAndRun, 0xC00, 4, 0x4000);
    sithAI_RegisterInstinct("dodge", (SithAIInstinctFunc)sithAIInstinct_Dodge, 0, 0x100000, 0x81803);
    sithAI_RegisterInstinct("hop", (SithAIInstinctFunc)sithAIInstinct_Hop, 1, 0, 0);
    sithAI_RegisterInstinct("hoverdrift", (SithAIInstinctFunc)sithAIInstinct_HoverDrift, 0x6FE, 0xF001, 0);
    sithAI_RegisterInstinct("basicfollow", (SithAIInstinctFunc)sithAIInstinct_BasicFallow, 0x202, 0x1004814, 0x8CE04);
    sithAI_RegisterInstinct("wallcrawl", (SithAIInstinctFunc)sithAIInstinct_WallCrawl, 0x202, 0x3004014, 0x10A604);
    sithAI_RegisterInstinct("snakefollow", (SithAIInstinctFunc)sithAIInstinct_SnakeFollow, 6, 0, 0x8E04);
    sithAI_RegisterInstinct("randommove", (SithAIInstinctFunc)sithAIInstinct_RandomMove, 2, 0x104805, 0x4800);
    sithAI_RegisterInstinct("humancombatmove", (SithAIInstinctFunc)sithAIInstinct_HumanCombatMove, 2, 0x1104805, 0x80901);
    sithAI_RegisterInstinct("feargunshot", (SithAIInstinctFunc)sithAIInstinct_FearGunshot, 6, 0x800, 2);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIInstinct_countfit_1, 28) /* end count-fit */
int J3DAPI sithAIInstinct_Listen(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    SithThing* pListener;
    rdVector3 thingVec;
    SithThing* pSource;
    SithThing* pHitThing;
    SithThing* pShooter;
    int sightVal;
    int thingVis;
    float loudness;
    float distance;
    SithAIAwarenessSector* pSectorInfo;
    float hitDist;
    rdVector3 toDest;
    rdVector3 srcPos;

#line 215
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    if ( pThing && (pThing->flags & SITH_TF_DISABLED) != 0 )
    {
        return 0;
    }

    pListener = pLocal->pOwner;
    pSectorInfo = &sithAIAwareness_aSectors[sithSector_GetSectorIndex(pListener->pInSector)];
    if ( !pSectorInfo )
    {
#line 229
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Listen: Bad sector data passed!\n", 0, 0, 0, 0);
        return 0;
    }

    switch ( event )
    {
        case SITHAI_EVENT_HIT_SECTOR:
        case SITHAI_EVENT_TOUCHED:
            if ( SITHAI_PARAM(pState, 0) == 0.0f )
            {
                if ( event == SITHAI_EVENT_HIT_SECTOR )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_SURPRISE);
                }
                else
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_CURIOUS);
                }
                SITHAI_PARAM(pState, 0) = 0.1f;
            }

            if ( pThing )
            {
                pHitThing = pThing;
                rdVector_Copy3(&srcPos, &pHitThing->pos);
                pShooter = sithThing_GetThingParent(pHitThing);
                if ( event == SITHAI_EVENT_HIT_SECTOR )
                {
                    if ( SITH_RAND() > SITHAI_FLTARG(pInstinct, 1) )
                    {
                        sithAIMove_AISetLookThing(pLocal, pShooter);
                        return 0;
                    }

                    if ( (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
                    {
                        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);
                        pLocal->pTargetThing = sithPlayer_pLocalPlayerThing;
                        if ( sithAIUtil_AIMoveToNextWpnt(pLocal, 1.7f, 0.0f, 0) )
                        {
                            return 1;
                        }
                    }
                    else
                    {
                        sithAIMove_AISetLookThing(pLocal, pShooter);
                        sithAIMove_AISetMovePos(pLocal, &pShooter->pos, 1.7f);
                        return 1;
                    }
                }
                else if ( event == SITHAI_EVENT_TOUCHED )
                {
                    if ( pShooter->type == SITH_THING_PLAYER )
                    {
                        sithAIMove_StopAIMovement(pLocal);
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &pShooter->pos);
                        return 0;
                    }

                    if ( SITH_RAND() > SITHAI_FLTARG(pInstinct, 1) )
                    {
                        sithAIMove_AISetLookThing(pLocal, pShooter);
                        return 0;
                    }

                    if ( (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
                    {
                        if ( sithAIUtil_AIMoveToNextWpnt(pLocal, pLocal->moveSpeed, 0.0f, 0) )
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        rdVector3 pos;

                        rdVector_Sub3(&pos, &pLocal->pOwner->pos, &pShooter->pos);
                        rdVector_Normalize3Acc(&pos);
                        if ( rdVector_Dot3(&pos, &pLocal->moveDirection) < 0.0f )
                        {
                            rdVector_Scale3Acc(&pos, SITHAI_FLTARG(pInstinct, 2));
                            rdVector_Add3Acc(&pos, &pLocal->pOwner->pos);
                            sithAIMove_StopAIMovement(pLocal);
                            sithAIMove_AISetLookPosEyeLevel(pLocal, &pos);
                            sithAIMove_AISetMoveTargetPos(pLocal, &pos, 0.75f);
                            return 0;
                        }
                    }
                }
            }
            return 0;

        case SITHAI_EVENT_SOUND:
            if ( (pListener->thingInfo.actorInfo.flags & SITH_AF_DEAF) != 0 )
            {
                return 0;
            }
            break;

        default:
            return 0;
    }

    if ( pSectorInfo->aLevelAtTransmittingPos[0] != 0.0f && SITH_RAND() < SITHAI_FLTARG(pInstinct, 0) )
    {
        int bStop;
        int bMove;
        int bLook;
        int bHunt;

        bMove = 0;
        bLook = 0;
        bStop = 0;
        bHunt = 0;
        pSource = pSectorInfo->aTransmittingThing[0];
        if ( pSource && (pSource->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return 0;
        }

        if ( !sithAIAwareness_IsInTransmittingRange(pSectorInfo, 0, pListener) )
        {
            return 0;
        }

        sightVal = sithAIUtil_GetDistanceToTargetPos(pListener, &pListener->pos, &pSectorInfo->aStartPos[0], -1.0f, pLocal->pClass->sightDistance, 0.0f, &toDest, &distance);
        if ( distance <= pLocal->pClass->heardDistance )
        {
            if ( pSource && pSource->type )
            {
                thingVis = sithAIUtil_GetDistanceToTargetPos(pListener, &pListener->pos, &pSource->pos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &thingVec, &hitDist);
            }
            else
            {
                thingVis = 3;
            }

            if ( pSource && pSource->type && (pLocal->mode & (SITHAI_MODE_HUNTING | SITHAI_MODE_INSTINCTUSEWPNTS)) != 0 )
            {
                bHunt = 1;
            }
            else if ( !thingVis )
            {
                rdVector_Copy3(&pLocal->alertPos, &pSource->pos);
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bMove = 1;
                bLook = 1;
            }
            else if ( !sightVal )
            {
                rdVector_Copy3(&pLocal->alertPos, &pSectorInfo->aStartPos[0]);
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bMove = 1;
                bLook = 1;
            }
            else if ( pSource && (pLocal->mode & SITHAI_MODE_HUNTING) == 0 )
            {
                loudness = pSectorInfo->aLevelAtTransmittingPos[0];
                bStop = 1;
                bLook = 1;
            }

            if ( bStop || bMove || bLook || bHunt )
            {
                if ( pSource && pSource->type )
                {
                    int bSrcUnderwater;
                    int underwater;

                    bSrcUnderwater = (pSource->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
                    underwater = (pLocal->pOwner->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0;
                    if ( (bSrcUnderwater && !underwater) || (!bSrcUnderwater && underwater) )
                    {
                        return 0;
                    }
                }

                if ( loudness > SITHAI_PARAM(pState, 0) )
                {
                    SITHAI_PARAM(pState, 0) = loudness;
                }

                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_CURIOUS);
                if ( bStop )
                {
                    sithAIMove_StopAIMovement(pLocal);
                }

                if ( bMove && SITH_RAND() <= SITHAI_FLTARG(pInstinct, 1) )
                {
                    sithAIMove_AISetMovePos(pLocal, &pLocal->alertPos, 0.75f);
                }

                if ( bLook )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->alertPos);
                }

                if ( bHunt && SITH_RAND() <= SITHAI_FLTARG(pInstinct, 1) )
                {
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);
                    pLocal->pTargetThing = sithPlayer_pLocalPlayerThing;
                    sithAIUtil_AIMoveToNextWpnt(pLocal, 1.7f, 0.0f, 0);
                }
            }
        }
    }
    return 0;
}

int J3DAPI sithAIInstinct_LookForTarget(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithAIMode newMode;
    float curhuntTime;

    if ( event == SITHAI_EVENT_MODECHANGED && pObject )
    {
        newMode = (SithAIMode)pObject;
        if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 && (newMode & SITHAI_MODE_HUNTING) == 0 )
        {
            SITHAI_PARAM(pState, 0) = sithTime_secGameTime;
        }
    }

    if ( event )
    {
        return 0;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_AINOTARGET) != 0 )
    {
        return 0;
    }

    if ( sithPlayerControls_bCutsceneMode == 1 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 )
    {
        curhuntTime = SITHAI_FLTARG(pInstinct, 3);
        if ( !pLocal->pTargetThing )
        {
#line 567
            stdPrintf(sith_pHS->pDebugPrint, J3D_FILE, 567, "LookForTarget: AI '%s' lost track of its target.  Ending HUNT mode.\n", pLocal->pOwner->aName, 0, 0, 0);
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_HUNTING);
        }
        else
        {
            if ( curhuntTime == 0.0f )
            {
                curhuntTime = 20.0f;
            }

            if ( SITHAIINSTINCT_TIMEDOUT(SITHAI_PARAM(pState, 0), curhuntTime) )
            {
                sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_HUNTING);
            }
            else if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 && (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
            {
                sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_HUNTING);
            }
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
    {
        if ( !pInstinct->intArg[1] )
        {
            return 0;
        }

        if ( pLocal->msecAttackStart + pInstinct->intArg[1] < sithTime_msecGameTime )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
            if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 )
            {
                if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 && (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
                {
                    return 0;
                }

                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);
                sithAIUtil_AIMoveToNextWpnt(pLocal, 1.7f, 0.0f, 0);
            }
            return 1;
        }
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
    {
        return 0;
    }

    pLocal->pTargetThing = sithPlayer_pLocalPlayerThing;
#line 645
    SITH_ASSERTREL(pLocal->pTargetThing);
    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        pLocal->pTargetThing = NULL;
        return 0;
    }

    if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0
        && (pLocal->pOwner->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0
        && (pLocal->pTargetThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) != 0 )
    {
        pLocal->pTargetThing = NULL;
        return 0;
    }

    sithAIUtil_UpdateTargetSight(pLocal, 0);
    if ( !pLocal->targetSightState )
    {
        if ( pLocal->goalThing == pLocal->pTargetThing )
        {
            pLocal->unknown141 = sithMain_frameNumber;
            pLocal->targetSightState2 = pLocal->targetSightState;
            rdVector_Copy3(&pLocal->vecUnknown0, &pLocal->toTarget);
            pLocal->targetDistance = pLocal->distance;
            rdVector_Copy3(&pLocal->vecUnknown5, &pLocal->pTargetThing->pos);
            rdVector_Copy3(&pLocal->vecUnknown6, &pLocal->pTargetThing->pos);
            pLocal->unknown150 = sithTime_msecGameTime;
        }

        if ( SITHAI_FLTARG(pInstinct, 2) != 0.0f )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            return 0;
        }

        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_ATTACKING);
        if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
        {
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
        }

        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_ALERT);
        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_ACTIVATE);
        sithAIAwareness_CreateTransmittingEvent(pLocal->pTargetThing->pInSector, &pLocal->pOwner->pos, 0, 1.5f, pLocal->pTargetThing);
        pLocal->goalThing = pLocal->pTargetThing;
        return 1;
    }

    if ( SITHAI_FLTARG(pInstinct, 0) == 0.0f )
    {
        SITHAI_FLTARG(pInstinct, 0) = 500.0f;
    }
    return 0;
}

int J3DAPI sithAIInstinct_SenseDanger(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    SithThing* pOwner;
    SithAIAwarenessSector* pAISector;
    SithThing* pDanger;
    int curSightState;
    float distance;
    rdVector3 toTarget;

    pOwner = pLocal->pOwner;
    pAISector = &sithAIAwareness_aSectors[sithSector_GetSectorIndex(pOwner->pInSector)];
    if ( !event && pInstinct->intArg[0] )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    switch ( event )
    {
        case 1:
            if ( SITH_RAND() > SITHAI_FLTARG(pInstinct, 3) )
            {
                return 0;
            }

            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_SURPRISE);
            if ( pThing )
            {
                pLocal->pFleeFromThing = sithThing_GetThingParent(pThing);
            }

            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            return 1;

        case 2:
            if ( pAISector->aLevelAtTransmittingPos[2] <= SITHAI_FLTARG(pInstinct, 1) )
            {
                return 0;
            }

            pDanger = pAISector->aTransmittingThing[2];
            if ( !pDanger )
            {
                return 0;
            }

            if ( pDanger == pLocal->pOwner )
            {
                return 0;
            }

            curSightState = sithAIUtil_GetDistanceToTarget(pOwner, &pOwner->pos, pDanger, -1.0f, pLocal->pClass->heardDistance, 0.0f, &toTarget, &distance);
            if ( curSightState )
            {
                return 0;
            }

            pLocal->pFleeFromThing = pDanger;
            if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
            {
                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                sithAIAwareness_CreateTransmittingEvent(pLocal->pOwner->pInSector, &pLocal->pOwner->pos, 2, 4.0f, pLocal->pOwner);
            }

            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            return 1;

        case 0:
            if ( SITH_RAND() > SITHAI_FLTARG(pInstinct, 2) )
            {
                return 0;
            }

            pLocal->pTargetThing = sithPlayer_pLocalPlayerThing;
            if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
            {
                pLocal->pTargetThing = NULL;
                return 0;
            }

            sithAIUtil_UpdateTargetSight(pLocal, 0);
            if ( !pLocal->targetSightState )
            {
                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                sithAIAwareness_CreateTransmittingEvent(pLocal->pOwner->pInSector, &pLocal->pOwner->pos, 1, 3.0f, pLocal->pOwner);
                pLocal->pFleeFromThing = pLocal->pTargetThing;
            }
            break;
    }

    return 0;
}

signed int J3DAPI sithAIInstinct_FearGunshot(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithAIAwarenessSector* pAISector;
    SithThing* owner;
    SithThing* pShooter;

    owner = pLocal->pOwner;
    pAISector = &sithAIAwareness_aSectors[sithSector_GetSectorIndex(owner->pInSector)];
    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        return 0;
    }

    switch ( event )
    {
        case SITHAI_EVENT_SOUND:
            if ( pAISector->aLevelAtTransmittingPos[3] > 0.0f )
            {
                pShooter = pAISector->aTransmittingThing[3];
                if ( pShooter && pShooter != pLocal->pOwner )
                {
                    rdVector3 curdir;
                    float dist;
                    float dot;

                    rdVector_Sub3(&curdir, &pLocal->pOwner->pos, &pShooter->pos);
                    dist = rdVector_Normalize3Acc(&curdir);
                    dot = rdVector_Dot3(&pShooter->orient.lvec, &curdir);
                    if ( dist < SITHAI_FLTARG(pInstinct, 0) || (dist < SITHAI_FLTARG(pInstinct, 1) && dot > SITHAI_FLTARG(pInstinct, 3) && SITH_RAND() < SITHAI_FLTARG(pInstinct, 2)) )
                    {
                        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                        pLocal->pFleeFromThing = pShooter;
                        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                        return 1;
                    }
                }
            }
            break;
    }

    return 0;
}

int J3DAPI sithAIInstinct_Hop(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    rdVector3 force;
    float hopForce;
    float hopDelay;

    hopDelay = (float)pInstinct->intArg[0];
    pState->msecNextUpdate = sithTime_msecGameTime + (uint32_t)SITHAIINSTINCT_RAND(hopDelay);
    if ( !pLocal->pOwner->attach.flags )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    sithThing_DetachThing(pLocal->pOwner);
    hopForce = SITHAI_FLTARG(pInstinct, 1);
    rdVector_Set3(&force, 0.0f, 0.0f, SITHAIINSTINCT_RAND(hopForce));
    sithPhysics_ApplyForce(pLocal->pOwner, &force);
    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_JUMP);
    return 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIInstinct_countfit_2, 0) /* end count-fit */
int J3DAPI sithAIInstinct_HoverDrift(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    rdVector3 direction;
    float curSpread;
    rdVector3 curForce;

#line 1004
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    if ( (pLocal->pOwner->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
    {
        return 0;
    }

    curSpread = SITHAI_FLTARG(pInstinct, 0);
    curSpread = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, curSpread) + curSpread;
    pState->msecNextUpdate = sithTime_msecGameTime + (uint32_t)curSpread;
    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 || (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return 0;
    }

    rdVector_Set3(&direction, RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, 2.0f), RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, 2.0f), RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, 2.0f));
    rdVector_Scale3(&curForce, &direction, SITHAIINSTINCT_RAND(SITHAI_FLTARG(pInstinct, 1)));
    sithPhysics_ApplyForce(pLocal->pOwner, &curForce);
    return 0;
}

int J3DAPI sithAIInstinct_Talk(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pOwner;
    SithThing* pCheckThing;
    float nextTalkTime;
    float delay;
    float hpFrac;
    int bOwnerWater;
    int bPlayerWater;
    float talkChance;
    int bSuccess;
    int hChannel;
    int arg;

    talkChance = SITHAI_FLTARG(pInstinct, 1);
    delay = SITHAI_FLTARG(pInstinct, 0);
    nextTalkTime = -1.0f;
    pOwner = pLocal->pOwner;
    bPlayerWater = 0;
    bOwnerWater = 0;
    bSuccess = 0;
    hChannel = 0;
    if ( SITHAI_FLTARG(pInstinct, 2) != 0.0f )
    {
        nextTalkTime = SITHAI_PARAM(pState, 0) + SITHAI_FLTARG(pInstinct, 2);
    }

    switch ( event )
    {
        case 0:
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
            {
                delay = D3DVAL(delay / 2.0f);
                talkChance = D3DVAL(talkChance * 2.0);
            }

            delay = delay * 1000.0f;
            pState->msecNextUpdate = sithTime_msecGameTime + (uint32_t)delay;
            if ( nextTalkTime > sithTime_secGameTime )
            {
                return 0;
            }

            if ( sithPlayer_pLocalPlayerThing )
            {
                bPlayerWater = (sithPlayer_pLocalPlayerThing->pInSector->flags & 2) != 0;
            }

            bOwnerWater = (pLocal->pOwner->pInSector->flags & 2) != 0;
            if ( (bPlayerWater && !bOwnerWater) || (!bPlayerWater && bOwnerWater) )
            {
                return 0;
            }

            if ( SITH_RAND() < talkChance )
            {
                hpFrac = D3DDivide(pOwner->thingInfo.actorInfo.health, pOwner->thingInfo.actorInfo.maxHealth);
                if ( (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_IDLE);
                }
                else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
                {
                    if ( hpFrac > 0.5f )
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_GLOAT);
                    }
                    else
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_FEAR);
                    }
                }
                else if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
                {
                    if ( SITH_RAND() < 0.6f )
                    {
                        hChannel = sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_RESERVED8);
                    }
                    else if ( hpFrac > 0.25f )
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, SITHSOUNDCLASS_GLOAT);
                    }
                    else
                    {
                        hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, SITHSOUNDCLASS_FEAR);
                    }
                }
                else if ( (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) == 0 )
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_SEARCH);
                }
                else if ( hpFrac < 0.25f )
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_FEAR);
                }
                else
                {
                    hChannel = sithSoundClass_PlayModeRandom(pOwner, SITHSOUNDCLASS_BOAST);
                }
            }
            break;

        case SITHAI_EVENT_MODECHANGED:
            arg = (int)pObject;
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 && (arg & SITHAI_MODE_UNKNOWN_100000) == 0 )
            {
                sithSoundMixer_StopAllSoundsThing(pLocal->pOwner);
                hChannel = sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_RESERVED8);
            }
            break;

        case SITHAI_EVENT_TOUCHED:
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
            {
                return 0;
            }

            if ( nextTalkTime > sithTime_secGameTime )
            {
                return 0;
            }

            pCheckThing = (SithThing*)pObject;
            if ( !pCheckThing )
            {
                return 0;
            }

            if ( (pCheckThing->type == SITH_THING_PLAYER || pCheckThing->type == SITH_THING_ACTOR) && SITH_RAND() < talkChance )
            {
                hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, SITHSOUNDCLASS_ALERT);
            }
            break;

        case SITHAI_EVENT_TALK:
            if ( nextTalkTime > sithTime_secGameTime )
            {
                return 0;
            }

            arg = (int)pObject;
            hChannel = sithSoundClass_PlayModeRandom(pLocal->pOwner, (SithSoundClassMode)arg);
            bSuccess = 1;
            break;

        default:
            return 0;
    }

    if ( hChannel > 0 )
    {
        SITHAI_PARAM(pState, 0) = sithTime_secGameTime;
    }

    return bSuccess;
}

int J3DAPI sithAIInstinct_RandomTurn(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    rdVector3 dir;
    rdVector3 randomPyr;
    rdVector3 lookPos;
    rdVector3 toTarget;
    float distToLook;

    if ( !pInstinct->intArg[0] )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 5000;
    }
    else
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    }

    if ( (pLocal->mode & SITHAI_MODE_SEARCHING) == 0 )
    {
        return 0;
    }

    memset(&randomPyr, 0, sizeof(randomPyr));
    randomPyr.yaw = RD_MULTIPLY(D3DVAL(SITH_RAND()), 360.0f);
    rdVector_Copy3(&dir, &rdroid_yVector3);
    rdVector_Rotate3Acc(&dir, &randomPyr);

    rdVector_ScaleAdd3(&lookPos, &dir, SITHAI_FLTARG(pInstinct, 1), &pLocal->pOwner->pos);
    if ( !sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &lookPos, -1.0f, SITHAI_FLTARG(pInstinct, 1), 0.0f, &toTarget, &distToLook) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);
    }
}

int J3DAPI sithAIInstinct_OpenDoors(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    sithPlayerActions_Activate(pLocal->pOwner);
    pState->msecNextUpdate = sithTime_msecGameTime + 1000;
    return 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIInstinct_countfit_3, 30) CF_PROTO(sithAIInstinct_countfit_4, 10) /* end count-fit */
int J3DAPI sithAIInstinct_Jump(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    rdVector3 upPos;
    int bNoCollision;
    rdVector3 curjumpPos;
    SithThing* pThing;
    SithSector* pSect;
    float curdot;
    float upZ;

#line 1325
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    pThing = pLocal->pOwner;
    pSect = pThing->pInSector;
    if ( !pThing->attach.flags )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        return 0;
    }

    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    curdot = rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pLocal->vecUnknown0);
    if ( curdot > 0.02f )
    {
        return 0;
    }

    if ( (double)sithTime_msecGameTime < SITHAI_PARAM(pState, 0) )
    {
        return 0;
    }

    SITHAI_PARAM(pState, 0) = (double)sithTime_msecGameTime + SITHAI_FLTARG(pInstinct, 0);
    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
            rdVector_ScaleAdd3(&upPos, &rdroid_zVector3, SITHAI_FLTARG(pInstinct, 1), &pThing->pos);
            pSect = sithCollision_FindSectorInRadius(pSect, &pThing->pos, &upPos, 0.0f);
            if ( !pSect )
            {
                return 0;
            }

            upZ = upPos.z;
            rdVector_ScaleAdd3(&curjumpPos, &pLocal->moveDirection, 0.1f, &upPos);
            curjumpPos.z = upZ;
            pSect = sithCollision_FindSectorInRadius(pSect, &upPos, &curjumpPos, 0.0f);
            if ( !pSect )
            {
                return 0;
            }

            if ( sithAIUtil_CanWalk_ExplicitSector(pLocal, &curjumpPos, pSect, &bNoCollision) == 1 && bNoCollision )
            {
                sithAIMove_AIJump(pLocal, &curjumpPos, 1.0f);
            }
            break;

        case SITHAI_EVENT_HIT_CLIFF:
            rdVector_ScaleAdd3(&curjumpPos, &pLocal->moveDirection, SITHAI_FLTARG(pInstinct, 2), &pThing->pos);
            if ( sithAIUtil_CanWalk(pLocal, &curjumpPos, NULL) )
            {
                rdVector_ScaleAdd3Acc(&pThing->moveInfo.physics.velocity, &pLocal->moveDirection, 0.1f);
                sithAIMove_AIJump(pLocal, &pLocal->movePos, 1.0f);
            }
            break;

        default:
            return 0;
    }
    return 1;
}

/* Retail's linker discarded this function (no retail symbol, caller or registration); restored in full
   from the debug build (0x004B9D1E, between Jump and LobFire, asserts at lines 1425/1430). Jedi Knight's
   sithAICmd_BlindFire is the same instinct (fires at vecUnknown122 from weaponFirePos); name is JK's with the
   TU's prefix. */
int J3DAPI sithAIInstinct_BlindFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pTarget;
    SithThing* pActor;
    SithThing* pWeaponTemplate;
    int sightState;
    float distance;
    rdVector3 fireDir;

#line 1425
    SITH_ASSERTREL(pLocal);
    pTarget = pLocal->pTargetThing;
    pActor = pLocal->pOwner;
#line 1430
    SITH_ASSERTREL(pActor);
    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        pLocal->pTargetThing = NULL;
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        return 1;
    }

    if ( SITH_RAND() > SITHAI_FLTARG(pInstinct, 1) || pLocal->msecFireWaitTime > sithTime_msecGameTime )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 1000;
        return 0;
    }

    pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    pWeaponTemplate = pActor->thingInfo.actorInfo.pWeaponTemplate;
    if ( !pTarget || !pWeaponTemplate )
    {
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        return 1;
    }

    sightState = sithAIUtil_GetDistanceToTargetPos(pActor, &pActor->pos, &pLocal->vecUnknown122, SITHAI_FLTARG(pInstinct, 3), 10.0f, pWeaponTemplate->collide.movesize, &fireDir, &distance);
    if ( sightState || distance < SITHAI_FLTARG(pInstinct, 4) )
    {
        return 0;
    }

    if ( pLocal->distance != 0.0f && SITHAI_FLTARG(pInstinct, 5) != 0.0f )
    {
        float aimError;

        aimError = SITHAI_FLTARG(pInstinct, 5) / distance;
        sithAIUtil_RandomRotate(&fireDir, aimError);
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_AIDISABLED) == 0 )
    {
        SithThing* pProjectile;

        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FIRE1);
        pProjectile = sithWeapon_WeaponFire(pActor, pWeaponTemplate, &fireDir, &pLocal->weaponFirePos, 0, SITHPUPPETSUBMODE_FIRE, 1.0f, 0, 0.0f);
        if ( pProjectile )
        {
            sithCog_ThingSendMessage(pActor, pProjectile, SITHCOG_MSG_FIRE);
        }
    }

    return 0;
}

int J3DAPI sithAIInstinct_LobFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pActor;
    SithThing* pTarget;
    int weaponNum;
    SithAIUtilFireFlags fireFlags;

    fireFlags = SITHAIUTIL_FIRE_LOB;
    weaponNum = 0;
#line 1526
    SITH_ASSERTREL(pLocal);
    pActor = pLocal->pOwner;
    pTarget = pLocal->pTargetThing;
#line 1531
    SITH_ASSERTREL(pActor);
    if ( pActor->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pTarget) )
    {
        if ( pTarget && (pTarget->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 && (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 )
        {
            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_VICTORY);
            sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_VICTORY, NULL);
        }

        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->pTargetThing = NULL;
        return 1;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, 0x41, 0x44) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 150;
        return 0;
    }

    weaponNum = pInstinct->intArg[6];
    if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 5) )
    {
        ++weaponNum;
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &fireFlags);
    if ( sithAIUtil_AIFire(pLocal, SITHAI_FLTARG(pInstinct, 2), SITHAI_FLTARG(pInstinct, 3), SITHAI_FLTARG(pInstinct, 1), SITHAI_FLTARG(pInstinct, 4), weaponNum, fireFlags, 1) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
        pLocal->msecFireWaitTime = sithTime_msecGameTime + pInstinct->intArg[0];
    }
    else
    {
        sithAIMove_AISetLookThing(pLocal, pTarget);
        pState->msecNextUpdate = sithTime_msecGameTime + 500;
    }
    return 0;
}

int J3DAPI sithAIInstinct_PrimaryFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithAIUtilFireFlags flags;
    int curBurstNum;
    SithThing* pTarget;
    int weaponNum;
    SithThing* pActor;
    float closestDist;
    float farDist;
    rdVector3 lookPosition;
    float fireDelay;
    float curFireDist;
    SithWeaponId curWeaponID;

    flags = SITHAIUTIL_FIRE_PRIMARY;
    weaponNum = 0;
#line 1640
    SITH_ASSERTREL(pLocal);
    pActor = pLocal->pOwner;
#line 1643
    SITH_ASSERTREL(pActor);

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        return 0;
    }

    if ( pActor->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    pTarget = pLocal->pTargetThing;
    if ( !sithAIUtil_CanAttackTarget(pTarget) )
    {
        if ( pTarget && (pTarget->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 && (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 )
        {
            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_VICTORY);
            sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_VICTORY, NULL);
        }

        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->pTargetThing = NULL;
        return 1;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 250;
        return 0;
    }

    if ( SITHAI_FLTARG(pInstinct, 17) != 0.0f && rdVector_Dot3(&pLocal->toTarget, &pActor->orient.lvec) > SITHAI_FLTARG(pInstinct, 17) )
    {
        if ( SITHAI_FLTARG(pInstinct, 16) == 0.0f )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
        }
        return 0;
    }

    if ( SITHAI_FLTARG(pInstinct, 6) != 0.0f && SITH_RAND() <= SITHAI_FLTARG(pInstinct, 6) )
    {
        flags |= SITHAIUTIL_FIRE_LEAD;
    }

    weaponNum = pInstinct->intArg[11];
    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        switch ( pInstinct->intArg[14] )
        {
            case 1:
                weaponNum = -1;
                break;

            case 2:
                sithAIUtil_UpdateTargetSight(pLocal, 0);
                return 0;
        }
    }
    else if ( SITHAI_FLTARG(pInstinct, 10) != 0.0f )
    {
        curFireDist = SITHAI_FLTARG(pInstinct, 15);
        if ( curFireDist != 0.0f )
        {
            sithAIUtil_UpdateTargetSight(pLocal, 0);
            if ( curFireDist < pLocal->distance )
            {
                weaponNum = pInstinct->intArg[11] + 1;
            }
        }
        else
        {
            weaponNum = pInstinct->intArg[11] + 1;
        }
    }

    if ( weaponNum == pInstinct->intArg[11] && SITHAI_FLTARG(pInstinct, 7) != 0.0f && SITH_RAND() <= SITHAI_FLTARG(pInstinct, 7) )
    {
        weaponNum = pInstinct->intArg[11] + 1;
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &flags);

    curBurstNum = pInstinct->intArg[8];
    if ( !curBurstNum )
    {
        curBurstNum = 1;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        curBurstNum *= SITHAIINSTINCT_RANDRANGE((curBurstNum < 0 ? -curBurstNum : curBurstNum), 6);
    }
    else if ( curBurstNum < 0 )
    {
        curBurstNum = SITHAIINSTINCT_RANDRANGE(1, (curBurstNum < 0 ? -curBurstNum : curBurstNum) + 1);
    }

    closestDist = SITHAI_FLTARG(pInstinct, 4);
    farDist = SITHAI_FLTARG(pInstinct, 2);
    if ( sithWeapon_HasWeaponSelected(pActor) )
    {
        farDist = sithWeapon_GetWeaponMaxAimDistance(pActor->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) == 0 && !sithAIUtil_IsLineOfFireClear(pLocal, &pLocal->pOwner->pos) )
    {
        // No clear line of fire: no shot (the debug build's code has this empty arm)
    }
    else if ( sithAIUtil_AIFire(pLocal, closestDist, farDist, SITHAI_FLTARG(pInstinct, 1), SITHAI_FLTARG(pInstinct, 3), weaponNum, flags, curBurstNum) )
    {
        if ( SITHAI_FLTARG(pInstinct, 16) == 0.0f && !sithWeapon_IsAiming(pLocal->pOwner) )
        {
            sithAIMove_AISetLookThing(pLocal, pTarget);
        }

        fireDelay = SITHAIINSTINCT_RANDSCALE20 * SITHAI_FLTARG(pInstinct, 0);
        pState->msecNextUpdate = sithTime_msecGameTime + (uint32_t)fireDelay;
        return 0;
    }

    if ( SITHAI_FLTARG(pInstinct, 16) == 0.0f && (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
    {
        curWeaponID = pLocal->pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID;
        switch ( pLocal->targetSightState )
        {
            case 2:
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                break;

            case 0:
                if ( pLocal->pTargetThing->moveInfo.physics.velocity.x != 0.0f
                  || pLocal->pTargetThing->moveInfo.physics.velocity.y != 0.0f
                  || pLocal->pTargetThing->moveInfo.physics.velocity.z != 0.0f )
                {
                    rdVector_ScaleAdd3(&lookPosition, &pLocal->pTargetThing->moveInfo.physics.velocity, 0.5f, &pLocal->pTargetThing->pos);
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPosition);
                }
                else if ( (unsigned int)curWeaponID >= SITHWEAPON_COMFISTS ? (unsigned int)curWeaponID <= SITHWEAPON_COMSHOTGUN : 0 )
                {
                    sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                }
                break;
        }
    }

    pState->msecNextUpdate = sithTime_msecGameTime + 750;
    return 0;
}

int J3DAPI sithAIInstinct_AlternateFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithAIUtilFireFlags flags;
    SithThing* pTarget;
    int weaponNum;
    int burstCount;
    float currentfireDelay;
    rdVector3 lookPos;

    flags = SITHAIUTIL_FIRE_ALT;
#line 1880
    SITH_ASSERTREL(pLocal);

    pTarget = pLocal->pTargetThing;
    if ( !sithAIUtil_CanAttackTarget(pTarget) )
    {
        return 1;
    }

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[1];
        return 0;
    }

    if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 150;
        return 0;
    }

    weaponNum = pInstinct->intArg[0];
    if ( weaponNum < 0 )
    {
        weaponNum = SITHAIINSTINCT_RANDRANGE(0, (weaponNum < 0 ? -weaponNum : weaponNum) + 1);
    }

    sithAIUtil_SetWeaponFireFlags(weaponNum, &flags);

    if ( SITHAI_FLTARG(pInstinct, 7) != 0.0f && SITH_RAND() <= SITHAI_FLTARG(pInstinct, 7) )
    {
        flags |= SITHAIUTIL_FIRE_LEAD;
    }

    burstCount = pInstinct->intArg[8];
    if ( !burstCount )
    {
        burstCount = 1;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        burstCount *= SITHAIINSTINCT_RANDRANGE(3, 11);
    }
    else if ( burstCount < 0 )
    {
        burstCount = SITHAIINSTINCT_RANDRANGE(1, (burstCount < 0 ? -burstCount : burstCount) + 1);
    }

    if ( pInstinct->intArg[9] )
    {
        flags |= 0x400;
    }

    if ( sithAIUtil_AIFire(pLocal, SITHAI_FLTARG(pInstinct, 4), SITHAI_FLTARG(pInstinct, 5), SITHAI_FLTARG(pInstinct, 3), SITHAI_FLTARG(pInstinct, 6), weaponNum, flags, burstCount) )
    {
        currentfireDelay = SITHAIINSTINCT_RANDSCALE20 * SITHAI_FLTARG(pInstinct, 2);
        pState->msecNextUpdate = sithTime_msecGameTime + (uint32_t)currentfireDelay;
    }
    else
    {
        if ( pLocal->targetSightState == 2 )
        {
            sithAIMove_AISetLookThing(pLocal, pTarget);
        }
        else if ( pLocal->targetSightState == 0
               && pTarget->moveType == SITH_MT_PHYSICS
               && (pTarget->moveInfo.physics.velocity.x != 0.0f || pTarget->moveInfo.physics.velocity.y != 0.0f || pTarget->moveInfo.physics.velocity.z != 0.0f) )
        {
            rdVector_ScaleAdd3(&lookPos, &pTarget->moveInfo.physics.velocity, 0.5f, &pTarget->pos);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);
        }

        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[1];
    }

    return 0;
}

// Linker-discarded (no retail symbol, caller or registration). The debug build has this function at
// Linker-discarded (no retail symbol, caller or registration; debug build 0x004BAAEB, assert line 2013): Jedi
// Knight's sithAICmd_TurretFire. Aims the head within fltArg[2] (yaw) / fltArg[3] (pitch) towards the target,
// leading a moving physics target with chance fltArg[8], and fires from the head matrix; with fltArg[5] set it
// self-destructs fltArg[7] ms after its health falls below that fraction, firing at random meanwhile. The body is
// the debug code's text (/Od-identical).
int J3DAPI sithAIInstinct_TurretFire(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pOwner;
    SithActorInfo* pActorInfo;
    SithThing* pTarget;  // never read (only assigned); the debug frame holds this slot
    SithThing* pWeaponTemplate;
    SithThing* pProjectile;
    float maxYaw;
    float maxPitch;
    float fireRate;  // never read; the debug frame holds this slot
    float yaw;
    float pitch;

#line 2013
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        pLocal->pTargetThing = NULL;
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        return 1;
    }

    pOwner = pLocal->pOwner;
    pActorInfo = &pOwner->thingInfo.actorInfo;
    pTarget = pLocal->pTargetThing;
    pWeaponTemplate = pActorInfo->pWeaponTemplate;
    if ( event )
    {
        return 0;
    }

    if ( !pWeaponTemplate )
    {
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        return 1;
    }

    sithPuppet_SetArmedMode(pLocal->pOwner, 2);
    maxYaw = SITHAI_FLTARG(pInstinct, 2);
    maxPitch = SITHAI_FLTARG(pInstinct, 3);
    fireRate = SITHAI_FLTARG(pInstinct, 0);
    if ( SITHAI_FLTARG(pInstinct, 5) != 0.0f && pActorInfo->health < RD_MULTIPLY(pActorInfo->maxHealth, SITHAI_FLTARG(pInstinct, 5)) )
    {
        if ( SITHAI_PARAM(pState, 1) == 0.0f )
        {
            SITHAI_PARAM(pState, 1) = RD_MULTIPLY(SITHAI_FLTARG(pInstinct, 7), 0.001f) + sithTime_secGameTime;
            SITHAI_PARAM(pState, 0) = sithTime_secGameTime - 1.0f;
        }
        else if ( SITHAI_PARAM(pState, 1) < sithTime_secGameTime )
        {
            sithActor_KillActor(pOwner, pOwner, SITH_DAMAGE_ENERGY);
            return 0;
        }

        pActorInfo->headPYR.yaw = -maxYaw + SITH_RAND() * (2.0f * maxYaw);
        pActorInfo->headPYR.pitch = -maxPitch + SITH_RAND() * (2.0f * maxPitch);
        sithActor_UpdateAimJoints(pOwner);
        if ( SITHAI_PARAM(pState, 0) < sithTime_secGameTime )
        {
            rdMatrix34 fireOrient;

            rdMatrix_Copy34(&fireOrient, &pOwner->orient);
            rdMatrix_PreRotate34(&fireOrient, &pActorInfo->headPYR);
            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FIRE1);
            pProjectile = sithWeapon_WeaponFire(pOwner, pWeaponTemplate, &fireOrient.lvec, &pOwner->pos, 0, SITHPUPPETSUBMODE_FIRE, 1.0f, 0, 0.0f);
            if ( pProjectile )
            {
                sithCog_ThingSendMessage(pOwner, pProjectile, SITHCOG_MSG_FIRE);
            }

            SITHAI_PARAM(pState, 0) = RD_MULTIPLY(SITHAI_FLTARG(pInstinct, 6), 0.001f) + sithTime_secGameTime;
        }

        pState->msecNextUpdate = sithTime_msecGameTime + 50;
        return 0;
    }

    sithAIUtil_UpdateTargetSight(pLocal, SITHAIUTIL_FIRE_PRIMARY);
    if ( pLocal->targetSightState == 0 )
    {
        float turnStep;
        rdVector3 pyr;
        rdVector3 aimDir;
        rdVector3 leadDir;

        turnStep = SITHAI_FLTARG(pInstinct, 1) * sithTime_frameTimeFlex;
        if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 8)
            && pLocal->pTargetThing
            && pLocal->pTargetThing->moveType == SITH_MT_PHYSICS
            && (pLocal->pTargetThing->moveInfo.physics.velocity.x != 0.0f || pLocal->pTargetThing->moveInfo.physics.velocity.y != 0.0f || pLocal->pTargetThing->moveInfo.physics.velocity.z != 0.0f) )
        {
            rdVector_Scale3(&leadDir, &pLocal->toTarget, pWeaponTemplate->moveInfo.physics.velocity.y);
            rdVector_Add3Acc(&leadDir, &pLocal->pTargetThing->moveInfo.physics.velocity);
            rdVector_Normalize3Acc(&leadDir);
            rdMatrix_TransformVectorOrtho34(&aimDir, &leadDir, &pOwner->orient);
        }
        else
        {
            rdMatrix_TransformVectorOrtho34(&aimDir, &pLocal->toTarget, &pOwner->orient);
        }

        rdVector_ExtractAngle(&aimDir, &pyr);
        pyr.yaw = STDMATH_CLAMP(pyr.yaw, -maxYaw, maxYaw);
        pyr.pitch = STDMATH_CLAMP(pyr.pitch, -maxPitch, maxPitch);
        yaw = pActorInfo->headPYR.yaw;
        pitch = pActorInfo->headPYR.pitch;
        if ( yaw != pyr.yaw || pitch != pyr.pitch )
        {
            if ( maxYaw >= 180.0f )
            {
                if ( yaw - pyr.yaw > 180.0f )
                {
                    pyr.yaw += 360.0f;
                }
                else if ( yaw - pyr.yaw < -180.0f )
                {
                    pyr.yaw -= 360.0f;
                }
            }

            yaw = STDMATH_CLAMP(pyr.yaw, yaw - turnStep, yaw + turnStep);
            pActorInfo->headPYR.yaw = stdMath_NormalizeAngleAcute(yaw);
            pitch = STDMATH_CLAMP(pyr.pitch, pitch - turnStep, pitch + turnStep);
            pActorInfo->headPYR.pitch = pitch;
            sithActor_UpdateAimJoints(pOwner);
        }

        if ( SITHAI_PARAM(pState, 0) < sithTime_secGameTime && (sith_mode.debugModeFlags & SITHDEBUG_AIDISABLED) == 0 )
        {
            SithThing* pFired;
            rdMatrix34 aimOrient;

            rdMatrix_Copy34(&aimOrient, &pOwner->orient);
            rdMatrix_PreRotate34(&aimOrient, &pActorInfo->headPYR);
            sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FIRE1);
            pFired = sithWeapon_WeaponFire(pOwner, pWeaponTemplate, &aimOrient.lvec, &pLocal->weaponFirePos, 0, SITHPUPPETSUBMODE_FIRE, 1.0f, 0, 0.0f);
            if ( pFired )
            {
                sithCog_ThingSendMessage(pOwner, pFired, SITHCOG_MSG_FIRE);
            }

            SITHAI_PARAM(pState, 0) = RD_MULTIPLY(SITHAI_FLTARG(pInstinct, 0), 0.001f) + sithTime_secGameTime;
        }

        pState->msecNextUpdate = sithTime_msecGameTime + 1;
    }
    else
    {
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        return 1;
    }
}

int J3DAPI sithAIInstinct_CircleStrafe(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    rdVector3 nextPos;
    int result;
    int bRandom;
    float side;
    SithThing* pSelf;  // never read; the debug frame holds this slot
    float range;
    rdVector3 strafeDir;
    SithThing* pTouched;
    rdVector3 pyr;
    float rate;
    rdVector3 dirToTarget;

    rate = 1.25f;
    result = 0;
#line 2222
    SITH_ASSERTREL(pLocal);

    pSelf = pLocal->pOwner;
    pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_FLEEING | SITHAI_MODE_SEARCHING)) != 0 )
    {
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->goalThing) )
    {
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->goalThing = NULL;
        return 0;
    }

    if ( SITHAI_FLTARG(pInstinct, 7) != 0.0f && sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 500;
        return 0;
    }

    if ( SITHAI_FLTARG(pInstinct, 8) != 0.0f )
    {
        rate = SITHAI_FLTARG(pInstinct, 8);
    }

    switch ( event )
    {
        case SITHAI_EVENT_GOAL_REACHED:
            result = 1;
            break;

        case 0:
            break;

        case SITHAI_EVENT_TOUCHED:
            if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 )
            {
                pTouched = pThing;
                if ( pTouched && pTouched->type != SITH_THING_PLAYER )
                {
                    goto stop;
                }
            }

            return 0;

        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        case SITHAI_EVENT_HIT_THING:
        case SITHAI_EVENT_GOAL_UNREACHABLE:
            if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 )
            {
                goto stop;
            }

            return 0;

        default:
            return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 && !pInstinct->intArg[6] )
    {
        sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
    }

    bRandom = pInstinct->intArg[4];
    sithAIUtil_UpdateGoalSight(pLocal);
    if ( SITHAI_FLTARG(pInstinct, 2) < pLocal->targetDistance || pLocal->targetSightState2 )
    {
        goto stop;
    }

    rdVector_Scale3(&strafeDir, &pLocal->vecUnknown0, -pLocal->targetDistance);
    if ( !bRandom && RD_MULTIPLY(pLocal->goalThing->orient.lvec.x, strafeDir.x) + RD_MULTIPLY(pLocal->goalThing->orient.lvec.y, strafeDir.y) + RD_MULTIPLY(pLocal->goalThing->orient.lvec.z, strafeDir.z) < 0.0f )
    {
        goto stop;
    }

    if ( SITHAI_PARAM(pState, 1)++ > 4.0f )
    {
        goto stop;
    }

    if ( SITHAI_PARAM(pState, 0) == 0.0f || bRandom )
    {
        if ( SITH_RAND() < 0.5f )
        {
            SITHAI_PARAM(pState, 0) = -1.0f;
        }
        else
        {
            SITHAI_PARAM(pState, 0) = 1.0f;
        }
    }

    side = SITHAI_PARAM(pState, 0);
    memset(&pyr, 0, sizeof(pyr));
    if ( !bRandom )
    {
        pyr.y = RD_MULTIPLY(SITHAI_FLTARG(pInstinct, 1), side);
    }
    else
    {
        pyr.y = RD_MULTIPLY(RD_MULTIPLY(D3DVAL(SITH_RAND()) + 0.5f, side), SITHAI_FLTARG(pInstinct, 1));
    }

    {
        rdVector3 toGoal;

        rdVector_Sub3(&toGoal, &pLocal->goalThing->pos, &pLocal->pOwner->pos);
        rdVector_Normalize3Acc(&toGoal);
        rdVector_Rotate3(&strafeDir, &toGoal, &pyr);
    }

    rdVector_ScaleAdd3(&nextPos, &strafeDir, RD_MULTIPLY(pLocal->pOwner->collide.movesize, 3.0f), &pLocal->pOwner->pos);

    if ( SITHAI_FLTARG(pInstinct, 5) != 0.0f )
    {
        rdVector3 pushDir;
        float pushDist;

        rdVector_Sub3(&pushDir, &pLocal->pOwner->pos, &nextPos);
        pushDist = rdVector_Normalize3Acc(&pushDir);
        if ( pushDist < SITHAI_FLTARG(pInstinct, 5) )
        {
            pushDist = SITHAI_FLTARG(pInstinct, 5);
            rdVector_ScaleAdd3(&nextPos, &pushDir, pushDist, &nextPos);
        }
    }

    if ( !sithAIUtil_GetDistanceToTargetPos(pLocal->goalThing, &pLocal->goalThing->pos, &nextPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirToTarget, &range) )
    {
        pLocal->mode |= SITHAI_MODE_CIRCLESTRAFING;
        sithAIMove_AISetMovePos(pLocal, &nextPos, rate);
        if ( !pInstinct->intArg[6] )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
        }
        else
        {
            sithAIMove_AISetLookPosEyeLevel(pLocal, &nextPos);
        }

        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[3];
    }
    else
    {
        SITHAI_PARAM(pState, 0) = -side;
        SITHAI_PARAM(pState, 1) = 0.0f;
        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    }

    return result;

stop:
    sithAIMove_AISetLookThing(pLocal, pLocal->goalThing);
    pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    SITHAI_PARAM(pState, 0) = 0.0f;
    SITHAI_PARAM(pState, 1) = 0.0f;
    return result;
}



int J3DAPI sithAIInstinct_Dodge(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float moveSpeed;
    float nextCheck;
    rdVector3 toDest;
    int bEvaded;
    float moveDist;
    float maxStep;
    float minDodgeDist;
    float minTargetLen;
    float minDistVal;
    SithAIUtilFireFlags moveFlags;
    float tmpVal;
    float targetGap;
    SithThing* pFirer;
    SithThing* pSource;
    SithAIAwarenessSector* pCurSector;
    float yaw;
    rdVector3 targetPos;

    moveSpeed = 1.5f;
    nextCheck  = -1.0f;
    yaw     = 0.0f;
    bEvaded   = 0;

#line 2450
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    if ( !event )
    {
        return 0;
    }

    if ( (event & (SITHAI_EVENT_GOAL_REACHED | SITHAI_EVENT_GOAL_UNREACHABLE)) != 0 && (pLocal->mode & SITHAI_MODE_UNKNOWN_10) != 0 )
    {
        goto goal_reached;
    }

    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_100000 | SITHAI_MODE_UNKNOWN_1000000)) != 0 )
    {
        return 0;
    }

    minDodgeDist = SITHAI_FLTARG(pInstinct, 0) + 0.03f;
    maxStep  = SITHAI_FLTARG(pInstinct, 1) + 0.03f;
    moveDist = RD_MULTIPLY(D3DVAL(SITH_RAND()), maxStep - minDodgeDist) + minDodgeDist;
    minTargetLen = SITHAI_FLTARG(pInstinct, 4);
    minDistVal = SITHAI_FLTARG(pInstinct, 10);

    if ( SITHAI_FLTARG(pInstinct, 2) != 0.0f )
    {
        moveSpeed = SITHAI_FLTARG(pInstinct, 2);
    }

    if ( SITHAI_FLTARG(pInstinct, 3) != 0.0f )
    {
        nextCheck = SITHAI_PARAM(pState, 0) + SITHAI_FLTARG(pInstinct, 3);
    }

    if ( event == SITHAI_EVENT_TARGETED )
    {
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
        {
            return 0;
        }

        pSource = pThing;
        if ( !pSource )
        {
            return 0;
        }

        if ( (pSource->thingInfo.weaponInfo.damageType & 0x810) != 0 )
        {
            return 0;
        }

        if ( (pSource->thingInfo.weaponInfo.flags & SITH_WF_AITARGETNODODGE) == 0 )
        {
            if ( nextCheck > sithTime_secGameTime )
            {
                return 0;
            }

            if ( SITH_RAND() >= SITHAI_FLTARG(pInstinct, 5) )
            {
                return 0;
            }
        }

        bEvaded = 1;
        pFirer = sithThing_GetThingParent(pSource);
        if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, pLocal->pClass->sightDistance, 0.0f, &toDest, &targetGap) )
        {
            return 0;
        }

        if ( targetGap < minTargetLen )
        {
            return 0;
        }

        if ( SITHAI_FLTARG(pInstinct, 8) != 0.0f && SITH_RAND() < SITHAI_FLTARG(pInstinct, 8) && sithAIMove_StartSpecialMove(pLocal, 0x101) )
        {
            goto dodged;
        }

        moveFlags = 0x8001;
    }
    else if ( event == SITHAI_EVENT_HIT_SECTOR )
    {
        if ( nextCheck > sithTime_secGameTime )
        {
            return 0;
        }

        if ( SITH_RAND() >= SITHAI_FLTARG(pInstinct, 6) )
        {
            return 0;
        }

        pSource = pThing;
        if ( !pSource )
        {
            return 0;
        }

        if ( pSource->type != SITH_THING_WEAPON || pSource->moveType != SITH_MT_PHYSICS )
        {
            return 0;
        }

        pFirer = sithThing_GetThingParent(pSource);
        if ( pFirer == pLocal->pOwner )
        {
            return 0;
        }

        if ( pFirer->controlType == SITH_CT_AI && pFirer->controlInfo.aiControl.pLocal )
        {
            rdVector_Sub3(&toDest, &pFirer->controlInfo.aiControl.pLocal->pTargetThing->pos, &pFirer->pos);
            rdVector_Normalize3Acc(&toDest);
            pLocal->pFleeFromThing = pFirer;
            moveFlags = 0x9001;
            moveDist = 1.0f;
            goto make_path;
        }

        if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, pLocal->pClass->sightDistance, 0.0f, &toDest, &targetGap) )
        {
            return 0;
        }

        if ( targetGap < minTargetLen )
        {
            return 0;
        }

        yaw = 60.0f;
        moveFlags = 0x8002;
    }
    else if ( event == SITHAI_EVENT_SOUND )
    {
        pCurSector = &sithAIAwareness_aSectors[sithSector_GetSectorIndex(pLocal->pOwner->pInSector)];
        if ( pCurSector->aLevelAtTransmittingPos[2] == 0.0f )
        {
            return 0;
        }

        pSource = pCurSector->aTransmittingThing[2];
        if ( !pSource )
        {
            return 0;
        }

        if ( pSource->moveType != SITH_MT_PHYSICS )
        {
            return 0;
        }

        if ( pSource->type == SITH_THING_WEAPON && (pSource->thingInfo.weaponInfo.damageType & 0x10) != 0 )
        {
            return 0;
        }

        pFirer = sithThing_GetThingParent(pSource);
        if ( pFirer == pLocal->pOwner )
        {
            return 0;
        }

        if ( (pFirer->moveInfo.physics.flags & 0xF000000) == 0 )
        {
            if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, pLocal->pClass->fov, 1.0f, 0.0f, &toDest, &targetGap) )
            {
                return 0;
            }
        }
        else
        {
            tmpVal = sqrtf(rdVector_Dot3(&pFirer->moveInfo.physics.velocity, &pFirer->moveInfo.physics.velocity)) * 2.0f;
            tmpVal = J3DMIN(pLocal->pClass->sightDistance, tmpVal);
            tmpVal = J3DMAX(tmpVal, 1.0f);
            if ( sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pFirer, -1.0f, tmpVal, 0.0f, &toDest, &targetGap) )
            {
                return 0;
            }
        }

        bEvaded = 1;
        if ( (pFirer->moveInfo.physics.flags & 0xF000000) != 0 )
        {
            float chance;
            rdVector3 heading;

            sithAIUtil_GetXYHeadingVector(pSource, &heading);
            if ( rdVector_Dot3(&toDest, &heading) > -0.5f )
            {
                return 0;
            }

            moveDist = 0.5f;
            moveFlags = 0x8001;
            chance = SITHAI_FLTARG(pInstinct, 9);
            if ( chance == 0.0f )
            {
                chance = 0.98f;
            }

            if ( sqrtf(rdVector_Dot3(&pFirer->moveInfo.physics.velocity, &pFirer->moveInfo.physics.velocity)) > 1.4f )
            {
                chance -= 0.2f;
            }

            if ( SITH_RAND() < chance )
            {
                moveFlags |= 0x1000;
            }

            pLocal->pFleeFromThing = pFirer;
            goto make_path;
        }

        if ( nextCheck > sithTime_secGameTime )
        {
            return 0;
        }

        if ( SITH_RAND() >= SITHAI_FLTARG(pInstinct, 7) )
        {
            return 0;
        }

        sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
        pLocal->pFleeFromThing = pFirer;
        yaw = 45.0f;
        moveFlags = 0xC002;
    }
    else
    {
        return 0;
    }

make_path:
    pLocal->pFleeFromThing = NULL;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDistVal, moveDist, yaw, moveFlags, &toDest, &targetPos) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &targetPos);
        sithAIMove_AISetMoveTargetPos(pLocal, &targetPos, moveSpeed);
    }
    else
    {
        return 0;
    }

dodged:
    pLocal->mode |= SITHAI_MODE_UNKNOWN_10;
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    SITHAI_PARAM(pState, 0) = sithTime_secGameTime;
    return bEvaded;

goal_reached:
    pLocal->mode &= ~SITHAI_MODE_UNKNOWN_10;
    if ( pLocal->pTargetThing )
    {
        sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
    }

    return 0;
}




int J3DAPI sithAIInstinct_HitAndRun(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    float time;

    switch ( event )
    {
        case 0:
            if ( SITHAI_PARAM(pState, 1) == 1.0f )
            {
                if ( sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
                {
                    pLocal->mode &= ~SITHAI_MODE_ATTACKING;
                    pState->msecNextUpdate = sithTime_msecGameTime + 100;
                    return 0;
                }

                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                pLocal->pFleeFromThing = pLocal->pTargetThing;
                goto flee;
            }
            break;

        case SITHAI_EVENT_FIRE:
            SITHAI_PARAM(pState, 3) = 1.0f;
            if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 2) )
            {
                SITHAI_PARAM(pState, 1) = 1.0f;
                pState->msecNextUpdate = sithTime_msecGameTime + 500;
            }
            return 0;

        default:
            return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        if ( SITHAI_PARAM(pState, 2) == 1.0f )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_ATTACKING);
            SITHAI_PARAM(pState, 2) = 0.0f;
        }

        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 && SITHAI_PARAM(pState, 3) != 0.0f && sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        time = SITHAI_PARAM(pState, 0);
        time += 1000.0f;
        if ( time > SITHAI_FLTARG(pInstinct, 0) )
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            pLocal->pFleeFromThing = pLocal->pTargetThing;
            goto flee;
        }

        SITHAI_PARAM(pState, 0) = time;
        pState->msecNextUpdate = sithTime_msecGameTime + 1000;
    }
    else
    {
        SITHAI_PARAM(pState, 0) = 0.0f;
        pState->msecNextUpdate = sithTime_msecGameTime + 1000;
    }

    return 0;

flee:
    SITHAI_PARAM(pState, 2) = 1.0f;
    SITHAI_PARAM(pState, 0) = 0.0f;
    SITHAI_PARAM(pState, 1) = 0.0f;
    SITHAI_PARAM(pState, 3) = 0.0f;
    pState->msecNextUpdate = sithTime_msecGameTime + (int)SITHAI_FLTARG(pInstinct, 1);
    return 0;
}


int J3DAPI sithAIInstinct_Retreat(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    float healthRatio;

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        pState->flags |= SITHAI_INSTINCT_DISABLED;
        return 0;
    }

    healthRatio = D3DDivide(pLocal->pOwner->thingInfo.actorInfo.health, pLocal->pOwner->thingInfo.actorInfo.maxHealth);
    if ( SITHAI_FLTARG(pInstinct, 3) != 0.0f && SITHAI_PARAM(pState, 0) > SITHAI_FLTARG(pInstinct, 3) )
    {
        return 0;
    }

    if ( healthRatio < SITHAI_FLTARG(pInstinct, 1) )
    {
        switch ( event )
        {
            case 0:
            case SITHAI_EVENT_HIT_SECTOR:
                if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 2) )
                {
                    SITHAI_PARAM(pState, 0) = SITHAI_PARAM(pState, 0) + 1.0f;
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FLEE);
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                    pLocal->pFleeFromThing = pLocal->pTargetThing;
                }
                else if ( SITH_RAND() < 0.1f )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FEAR);
                }
                break;
        }
    }

    pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    return 0;
}

/* Retail's linker discarded this function (no retail symbol, caller or registration); restored in full
   from the debug build (0x004BC4B5, between Retreat and RandomMove, assert at line 3011). Jedi Knight's
   sithAICmd_Crouch is the same instinct; name is JK's with the TU's prefix. */
int J3DAPI sithAIInstinct_Crouch(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pThing;

#line 3011
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    pThing = pLocal->pOwner;
    if ( pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->goalThing) )
    {
        pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
        pLocal->goalThing = NULL;
        return 0;
    }

    pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 && (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 && (pLocal->mode & SITHAI_MODE_TARGETVISIBLE) != 0 )
    {
        pThing->moveInfo.physics.flags |= SITH_PF_CROUCHING;
    }
    else
    {
        pThing->moveInfo.physics.flags &= ~SITH_PF_CROUCHING;
    }

    return 0;
}

int J3DAPI sithAIInstinct_RandomMove(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    float minDist;
    rdVector3 direction;
    float moveDist;
    int pathOpts;
    float moveSpeed;
    float maxSpeed;
    float angleDeg;
    float lowSpeed;
    int delay;
    float maxLen;
    rdVector3 goalPos;

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        return 0;
    }

    if ( (pLocal->mode & (SITHAI_MODE_SEARCHING | SITHAI_MODE_FLEEING | SITHAI_MODE_CIRCLESTRAFING)) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        return 0;
    }

    if ( pInstinct->intArg[6] && sithPuppet_IsAnyModeOnTrack(pLocal->pOwner, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        return 0;
    }

    sithAIUtil_UpdateTargetSight(pLocal, 0);

    switch ( event )
    {
        case SITHAI_EVENT_FIRE:
            if ( !pInstinct->intArg[7] )
            {
                return 0;
            }
            break;

        case 0:
            delay = pInstinct->intArg[0];
            delay += (int)D3DMultiply(D3DVAL((uint32_t)delay), D3DVAL(SITH_RAND()) - 0.5f);
            pState->msecNextUpdate = sithTime_msecGameTime + delay;
            if ( pLocal->targetSightState )
            {
                return 0;
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( !pLocal->targetSightState && pLocal->pTargetThing )
            {
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            }
            else if ( SITHAI_PARAM(pState, 3) != 0.0f )
            {
                rdVector_Set3(&direction, SITHAI_PARAM(pState, 0), SITHAI_PARAM(pState, 1), SITHAI_PARAM(pState, 2));
                sithAIMove_AISetLookPosEyeLevel(pLocal, &direction);
            }

            SITHAI_PARAM(pState, 3) = 0.0f;
            return 0;

        default:
            return 0;
    }

    minDist   = SITHAI_FLTARG(pInstinct, 1) + 0.03f;
    maxLen   = SITHAI_FLTARG(pInstinct, 2) + 0.03f;
    moveDist  = RD_MULTIPLY(D3DVAL(SITH_RAND()), D3DVAL(maxLen - minDist)) + minDist;
    lowSpeed  = SITHAI_FLTARG(pInstinct, 3);
    maxSpeed  = SITHAI_FLTARG(pInstinct, 4);
    moveSpeed = RD_MULTIPLY(D3DVAL(SITH_RAND()), D3DVAL(maxSpeed - lowSpeed)) + lowSpeed;

    rdVector_Neg3(&direction, &pLocal->toTarget);

    pathOpts = 0x808002;
    if ( pInstinct->intArg[9] )
    {
        pathOpts |= 4;
    }

    angleDeg = SITHAI_FLTARG(pInstinct, 5);
    if ( angleDeg < 0.0f )
    {
        pathOpts |= 0x4000;
    }

    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, moveDist, angleDeg, pathOpts, &direction, &goalPos) )
    {
        if ( pLocal->pTargetThing )
        {
            SITHAI_PARAM(pState, 0) = pLocal->pTargetThing->pos.x;
            SITHAI_PARAM(pState, 1) = pLocal->pTargetThing->pos.y;
            SITHAI_PARAM(pState, 2) = pLocal->pTargetThing->pos.z;
            SITHAI_PARAM(pState, 3) = 1.0f;
        }
        else
        {
            SITHAI_PARAM(pState, 3) = 0.0f;
        }

        if ( pInstinct->intArg[8] || moveDist >= (maxLen - minDist) / 2.0f + minDist )
        {
            sithAIMove_AISetLookPosEyeLevel(pLocal, &goalPos);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &goalPos, moveSpeed);
    }

    return 0;
}

int J3DAPI sithAIInstinct_HumanCombatMove(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pOwner;
    SithAIMode mode;
    int bAimAtDest;
    int bCheckPath;
    unsigned int curMoveType;
    int bTargetVisible;
    float healthPct;
    float minSpeed;
    float angle;
    float minDist;
    float maxRange;
    int opts;
    float fastSpeed;
    float enemyRange;
    rdVector3 targetPos;
    float rndVal;
    float curMoveSpeed;
    float arc;
    unsigned int triedMoves;
    rdVector3 dirNorm;

    pOwner      = pLocal->pOwner;
    bAimAtDest = 1;
    bCheckPath  = 0;

#line 3248
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    if ( !sithAIUtil_CanAttackTarget(pLocal->pTargetThing) )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_ATTACKING) == 0 )
    {
        goto clear;
    }

    if ( (pLocal->mode & (SITHAI_MODE_SEARCHING | SITHAI_MODE_FLEEING | SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_UNKNOWN_100000)) != 0 )
    {
        goto clear;
    }

    sithAIUtil_UpdateTargetSight(pLocal, 0);
    healthPct = D3DDivide(pOwner->thingInfo.actorInfo.health, pOwner->thingInfo.actorInfo.maxHealth);

    switch ( event )
    {
        case SITHAI_EVENT_MODECHANGED:
            mode = (SithAIMode)pObject;
            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) != 0 && (mode & SITHAI_MODE_CHASE_GOAL) == 0 )
            {
                if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_1000000) != 0 )
                {
                    return 0;
                }

                sithAIMove_StopAIMovement(pLocal);
                bTargetVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &pLocal->vecUnknown5, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirNorm, &enemyRange);
                if ( healthPct <= SITHAI_FLTARG(pInstinct, 6) && SITH_RAND() < SITHAI_FLTARG(pInstinct, 9) && bTargetVisible )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown);
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown, SITHAI_FLTARG(pInstinct, 5));
                    pLocal->mode |= SITHAI_MODE_UNKNOWN_1000000;
                    pLocal->mode &= ~SITHAI_MODE_CHASE_GOAL;
                    SITHAI_PARAM(pState, 3) = 3.0f;
                    return 1;
                }

                if ( bTargetVisible )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown);
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown, SITHAI_FLTARG(pInstinct, 5));
                    return 1;
                }
            }
            break;

        case SITHAI_EVENT_HIT_SECTOR:
            if ( healthPct <= SITHAI_FLTARG(pInstinct, 6) )
            {
                if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 7) )
                {
                    sithAIUtil_AIPlaySoundMode(pLocal, SITHSOUNDCLASS_FLEE);
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
                    pLocal->pFleeFromThing = pLocal->pTargetThing;
                    return 0;
                }

                if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 8) && sithAI_HasInstinct(pLocal, "circlestrafe") )
                {
                    sithAI_EnableInstinct(pLocal, "circlestrafe", 1);
                    pLocal->submode |= 0x400000;
                }

                if ( SITH_RAND() < SITHAI_FLTARG(pInstinct, 10) )
                {
                    sithAIMove_StopAIMovement(pLocal);
                    pLocal->mode |= SITHAI_MODE_UNKNOWN_100000;
                    sithAI_ForceInstinctUpdate(pLocal, "primaryfire", 0);
                    sithAI_ForceInstinctUpdate(pLocal, "lobfire", 0);
                    sithAI_ForceInstinctUpdate(pLocal, "alternatefire", 0);
                }
            }
            break;

        case 0:
            if ( pLocal->targetSightState != 0 )
            {
                return 0;
            }

            if ( sithAIUtil_IsLineOfFireClear(pLocal, &pLocal->pOwner->pos) )
            {
                bCheckPath = 1;
                if ( (pLocal->pTargetThing->moveInfo.physics.flags & 0xF000000) == 0 && sithAIUtil_IsThingMoving(pLocal->pTargetThing) )
                {
                    SITHAI_PARAM(pState, 1) += 0.2f;
                }

                if ( SITHAI_PARAM(pState, 0) > (float)sithTime_msecGameTime )
                {
                    return 0;
                }

                arc = SITHAI_FLTARG(pInstinct, 0);
                arc = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, arc) + arc;
                SITHAI_PARAM(pState, 0) = sithTime_msecGameTime + arc;
                if ( SITH_RAND() > SITHAI_PARAM(pState, 1) )
                {
                    SITHAI_PARAM(pState, 1) += 0.05f;
                    return 0;
                }
            }

            minDist    = SITHAI_FLTARG(pInstinct, 2) + 0.03f;
            maxRange    = SITHAI_FLTARG(pInstinct, 3) + 0.03f;
            enemyRange       = RD_MULTIPLY(D3DVAL(SITH_RAND()), D3DVAL(maxRange - minDist)) + minDist;
            minSpeed   = SITHAI_FLTARG(pInstinct, 4);
            fastSpeed   = SITHAI_FLTARG(pInstinct, 5);
            triedMoves = (unsigned int)SITHAI_PARAM(pState, 2);
            if ( triedMoves )
            {
                if ( (triedMoves & 1) == 0 )
                {
                    goto move_toward;
                }

                if ( (triedMoves & 2) == 0 )
                {
                    goto move_back;
                }

                if ( (triedMoves & 4) == 0 )
                {
                    goto move_side;
                }

                SITHAI_PARAM(pState, 2) = 0.0f;
                return 0;
            }

            rndVal = SITH_RAND();
            if ( J3DINRANGE(rndVal, 0.0f, 0.4f) )
            {
                goto move_toward;
            }

            if ( J3DINRANGE(rndVal, 0.5f, 0.8f) )
            {
                goto move_back;
            }

            goto move_side;

        move_toward:
            curMoveType  = 1;
            curMoveSpeed = fastSpeed;
            angle     = 0.0f;
            opts     = 0x8001;
            rdVector_Copy3(&dirNorm, &pLocal->toTarget);
            goto make_path;

        move_back:
            curMoveType = 2;
            if ( !pInstinct->intArg[1] && pLocal->distance > D3DVAL(pOwner->collide.movesize * 4.0) )
            {
                return 1;
            }

            bAimAtDest = 0;
            curMoveSpeed   = minSpeed;
            angle       = 30.0f;
            opts       = 0xC002;
            rdVector_Neg3(&dirNorm, &pLocal->toTarget);
            goto make_path;

        move_side:
            curMoveType = 4;
            if ( !pInstinct->intArg[1] )
            {
                return 1;
            }

            curMoveSpeed = RD_MULTIPLY(D3DVAL(SITH_RAND()), D3DVAL(fastSpeed - minSpeed)) + minSpeed;
            angle     = 30.0f;
            opts     = 0xC002;
            rdVector_Copy3(&dirNorm, &pLocal->toTarget);

        make_path:
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minDist, enemyRange, angle, opts, &dirNorm, &targetPos) )
            {
                if ( bCheckPath && !sithAIUtil_IsLineOfFireClear(pLocal, &targetPos) )
                {
                }
                else
                {
                    SITHAI_PARAM(pState, 1) = 0.0f;
                    SITHAI_PARAM(pState, 2) = 0.0f;
                    if ( bAimAtDest )
                    {
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &targetPos);
                    }
                    else
                    {
                        rdVector_Sub3(&dirNorm, &pOwner->pos, &targetPos);
                        rdVector_Add3(&dirNorm, &pOwner->pos, &dirNorm);
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &dirNorm);
                    }

                    sithAIMove_AISetMoveTargetPos(pLocal, &targetPos, curMoveSpeed);
                    return 0;
                }
            }

            triedMoves |= curMoveType;
            SITHAI_PARAM(pState, 2) = (float)triedMoves;
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_1000000) != 0 )
            {
                if ( SITHAI_PARAM(pState, 3) == 3.0f )
                {
                    if ( pLocal->targetSightState != 0 )
                    {
                        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_1000000;
                        return 0;
                    }

                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown122);
                    sithAIUtil_AIPauseMove(pLocal, rand() % 1001 + 2000);
                    SITHAI_PARAM(pState, 3) = 2.0f;
                }
                else if ( SITHAI_PARAM(pState, 3) == 2.0f )
                {
                    if ( pLocal->targetSightState == 0 )
                    {
                        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_1000000;
                        return 0;
                    }

                    sithAIUtil_AIPauseMove(pLocal, rand() % 2001 + 1000);
                    SITHAI_PARAM(pState, 3) = 3.0f;
                }

                rdVector_Sub3(&targetPos, &pLocal->vecUnknown122, &pLocal->pOwner->pos);
                rdVector_Normalize3Acc(&targetPos);
                rdVector_Sub3(&dirNorm, &pLocal->vecUnknown, &pLocal->pOwner->pos);
                rdVector_Normalize3Acc(&dirNorm);

                pOwner->thingInfo.actorInfo.aiState = 1;
                angle = rdMath_DeltaAngleNormalized(&dirNorm, &targetPos, &pOwner->orient.uvec);
                if ( angle < 0.0f )
                {
                    pOwner->thingInfo.actorInfo.aiArmedModeState = 0x201;
                }
                else
                {
                    pOwner->thingInfo.actorInfo.aiArmedModeState = 0x401;
                }
                return 1;
            }

            if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) != 0 )
            {
                bTargetVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->pOwner, &pLocal->pOwner->pos, &pLocal->vecUnknown5, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dirNorm, &enemyRange);
                if ( bTargetVisible )
                {
                    sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown5, 1.7f);
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->vecUnknown5);
                    return 1;
                }
            }
            else if ( pLocal->pTargetThing )
            {
                sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
            }

            goto clear;

        case SITHAI_EVENT_GOAL_UNREACHABLE:
            goto clear;
    }

    return 0;

clear:
    SITHAI_PARAM(pState, 1) = 0.0f;
    SITHAI_PARAM(pState, 2) = 0.0f;
    return 0;
}



int J3DAPI sithAIInstinct_Roam(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    float moveSpeed;
    SithAIMode mode;
    rdVector3 movePos;
    rdVector3 direction;
    float roamDist;
    float closestDist;
    int bEnabled;
    float homeRadius;
    rdVector3 startPos;
    int pathOpts;
    int bMoved;  // never read; the debug frame holds this slot

    bMoved = 0;
    moveSpeed = 0.75f;
    bEnabled = 0;

    if ( (pLocal->mode & (SITHAI_MODE_ATTACKING | SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_ACTIVE | SITHAI_MODE_FLEEING)) != 0 )
    {
        return 0;
    }

    if ( event == SITHAI_EVENT_MODECHANGED && pThing )
    {
        mode = (SithAIMode)pThing;
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 && (mode & SITHAI_MODE_SEARCHING) == 0 )
        {
            pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
            return 0;
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
    {
        return 0;
    }

    closestDist  = pLocal->pOwner->collide.movesize + 0.03f;
    homeRadius = SITHAI_FLTARG(pInstinct, 1);
    roamDist = RD_MULTIPLY(D3DVAL(SITH_RAND()), (homeRadius < 0.0f ? -homeRadius : homeRadius) - closestDist) + closestDist;

    if ( SITHAI_FLTARG(pInstinct, 3) != 0.0f )
    {
        moveSpeed = SITHAI_FLTARG(pInstinct, 3);
    }

    if ( homeRadius > 0.0f )
    {
        rdVector_Copy3(&startPos, &pLocal->homePos);
    }
    else
    {
        rdVector_Copy3(&startPos, &pLocal->pOwner->pos);
    }

    if ( SITHAI_FLTARG(pInstinct, 2) != 0.0f )
    {
        bEnabled = 1;
    }

    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
            pathOpts = 0x20C002;
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
            rdVector_Neg3Acc(&direction);
            if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, closestDist, roamDist, 45.0f, pathOpts, &direction, &movePos) )
            {
                sithAIMove_StopAIMovement(pLocal);
                if ( sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed) )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                    return 1;
                }
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->submode & 0x20200) == 0 )
            {
                return 0;
            }
            // fallthrough

        case 0:
            pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
            if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 && sithAIUtil_AIMoveToNextWpnt(pLocal, moveSpeed, 0.0f, 1) )
            {
                return 0;
            }

            if ( homeRadius > 0.0f
                && sqrtf(RD_MULTIPLY(pLocal->pOwner->pos.x - pLocal->homePos.x, pLocal->pOwner->pos.x - pLocal->homePos.x) + RD_MULTIPLY(pLocal->pOwner->pos.y - pLocal->homePos.y, pLocal->pOwner->pos.y - pLocal->homePos.y) + RD_MULTIPLY(pLocal->pOwner->pos.z - pLocal->homePos.z, pLocal->pOwner->pos.z - pLocal->homePos.z)) > homeRadius )
            {
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
                sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, moveSpeed);
                return 0;
            }
            break;

        default:
            return 0;
    }

    if ( sithAIUtil_MakeRoamPos(pLocal, &startPos, closestDist, roamDist, bEnabled, 2, &movePos) && sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
        return 1;
    }

    if ( sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, moveSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
        return 1;
    }

    return 0;
}


int J3DAPI sithAIInstinct_ReturnHome(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithAIMode prevMode;
    rdVector3 lookPos;

    switch ( event )
    {
        case SITHAI_EVENT_MODECHANGED:
            prevMode = (SithAIMode)pObject;
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 && (prevMode & SITHAI_MODE_SEARCHING) == 0 )
            {
                sithAIMove_AISetMovePos(pLocal, &pLocal->homePos, 1.2f);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->homePos);
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
            {
                rdVector_ScaleAdd3(&lookPos, &pLocal->homeOrient, 1.0f, &pLocal->pOwner->pos);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &lookPos);
            }
            break;
    }

    return 0;
}

int J3DAPI sithAIInstinct_Flee(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithThing* pThing)
{
    int ret;  // never read; the debug frame holds this slot
    rdVector3 endPos;
    SithThing* pTouched;
    SithAIMode mode;
    int bPanic;
    rdVector3 direction;
    float threatRange;
    float minStep;
    float fleeingSpeed;
    int pathOpts;
    float angle;
    float panicTime;
    float fleeRange;
    SithThing* pSelf;  // never read; the debug frame holds this slot

    ret     = 0;
    pSelf     = pLocal->pOwner;
    panicTime   = SITHAI_FLTARG(pInstinct, 2);
    fleeRange   = SITHAI_FLTARG(pInstinct, 0);
    minStep    = pLocal->pOwner->collide.movesize + 0.03f;
    fleeingSpeed  = 1.5f;
    bPanic = 0;

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
    {
        if ( SITHAI_PARAM(pState, 0) > 0.0f )
        {
            goto stop;
        }

        return 0;
    }

    pLocal->goalThing    = NULL;
    pLocal->pTargetThing = NULL;
    if ( SITHAI_PARAM(pState, 0) == 0.0f )
    {
        SITHAI_PARAM(pState, 0) = sithTime_secGameTime;
        bPanic = 1;
    }

    if ( panicTime == 0.0f )
    {
        panicTime = 10.0f;
        if ( (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            panicTime += 15.1f;
        }
    }

    if ( !pLocal->pFleeFromThing )
    {
        goto stop;
    }

    if ( SITHAIINSTINCT_TIMEDOUT(SITHAI_PARAM(pState, 0), panicTime) )
    {
        goto stop;
    }

    if ( SITHAI_FLTARG(pInstinct, 4) != 0.0f )
    {
        fleeingSpeed = SITHAI_FLTARG(pInstinct, 4);
    }

    if ( pInstinct->intArg[1] )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[1];
    }
    else
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 5000;
    }

    if ( !sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pLocal->pFleeFromThing, -1.0f, fleeRange, 0.0f, &direction, &threatRange) || bPanic )
    {
        switch ( event )
        {
            case SITHAI_EVENT_MODECHANGED:
                if ( pThing )
                {
                    mode = (SithAIMode)pThing;
                    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && (mode & SITHAI_MODE_FLEEING) == 0 )
                    {
                        rdVector_Neg3Acc(&direction);
                        goto flee;
                    }
                }

                return 0;

            case 0:
            case SITHAI_EVENT_GOAL_REACHED:
                rdVector_Neg3Acc(&direction);
                goto flee;

            case SITHAI_EVENT_TOUCHED:
                pTouched = pThing;
                if ( pTouched && pTouched->type == SITH_THING_PLAYER )
                {
                    return 1;
                }
                break;
        }

        sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
        pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
        goto flee;
    }
    else if ( (pLocal->submode & 0x8000) != 0 )
    {
        return 0;
    }

stop:
    if ( pLocal->pFleeFromThing )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &pLocal->pFleeFromThing->pos);
    }

    sithAIMove_StopAIMovement(pLocal);
    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
    SITHAI_PARAM(pState, 0) = 0.0f;
    pLocal->pFleeFromThing  = NULL;
    return 0;

flee:
    if ( (pLocal->mode & SITHAI_MODE_INSTINCTUSEWPNTS) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
        {
            return 1;
        }

        if ( sithAIUtil_AIMoveToNextWpnt(pLocal, fleeingSpeed, 0.0f, 0) )
        {
            return 1;
        }
    }

    sithAIMove_StopAIMovement(pLocal);
    pathOpts = 0x21002;
    if ( SITHAI_FLTARG(pInstinct, 5) > 0.0f )
    {
        minStep = SITHAI_FLTARG(pInstinct, 5);
    }

    angle = SITHAI_FLTARG(pInstinct, 3);
    if ( angle != 0.0f )
    {
        pathOpts |= 0x8000;
        if ( angle < 0.0f )
        {
            pathOpts |= 0x4000;
        }
    }

    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minStep, fleeRange, angle, pathOpts, &direction, &endPos) && sithAIMove_AISetMovePos(pLocal, &endPos, fleeingSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &endPos);
        return 1;
    }

    pathOpts = 0x8001;
    pathOpts |= 0x21000;
    if ( sithAIUtil_MakePathPos(pLocal, &pLocal->pOwner->pos, minStep, fleeRange, angle, pathOpts, &direction, &endPos) && sithAIMove_AISetMovePos(pLocal, &endPos, fleeingSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &endPos);
        return 1;
    }

    goto stop;
}


// Linker-discarded (no retail symbol, caller or registration); restored in full from the debug build
// (0x004BDE4C, between Flee and BasicFallow). Jedi Knight's sithAICmd_Withdraw is the same
// instinct (the name was first inferred from that and from the __LINE__ gap).
int J3DAPI sithAIInstinct_Withdraw(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pOwner;
    float moveDist;
    float distance;
    rdVector3 pyr;
    rdVector3 dir;
    rdVector3 destPos;

    pOwner = pLocal->pOwner;
    moveDist = SITHAI_FLTARG(pInstinct, 1);
    if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
    {
        return 0;
    }

    if ( !sithAIUtil_CanAttackTarget(pLocal->pFleeFromThing) )
    {
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
        pLocal->pFleeFromThing = NULL;
        return 0;
    }

    if ( pInstinct->intArg[0] )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
    }
    else
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 5000;
    }

    if ( !sithAIUtil_GetDistanceToTarget(pLocal->pOwner, &pLocal->pOwner->pos, pLocal->pFleeFromThing, -1.0f, pLocal->pClass->sightDistance, 0.0f, &dir, &distance) )
    {
        memset(&pyr, 0, sizeof(pyr));
        switch ( event )
        {
            case 0:
            case SITHAI_EVENT_MODECHANGED:
            case SITHAI_EVENT_GOAL_REACHED:
                if ( distance >= moveDist )
                {
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
                    return 1;
                }

                rdVector_Neg3Acc(&dir);
                pyr.y = RD_MULTIPLY(SITH_RAND() - 0.5f, 180.0f);
                if ( (pLocal->pOwner->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
                {
                    pyr.x = RD_MULTIPLY(SITH_RAND() - 0.5f, 90.0f);
                }

                rdVector_Rotate3Acc(&dir, &pyr);
                rdVector_ScaleAdd3(&destPos, &dir, moveDist, &pLocal->pOwner->pos);
                break;

            default:
                if ( pInstinct->intArg[2] && pInstinct->intArg[2] > SITHAI_PARAM(pState, 0)++ )
                {
                    if ( SITH_RAND() < 0.5f )
                    {
                        pyr.y = 90.0f;
                    }
                    else
                    {
                        pyr.y = -90.0f;
                    }

                    rdVector_Rotate3(&dir, &pLocal->moveDirection, &pyr);
                    rdVector_ScaleAdd3(&destPos, &dir, moveDist, &pLocal->pOwner->pos);
                }
                else
                {
                    SITHAI_PARAM(pState, 0) = 0.0f;
                    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
                    return 1;
                }
                break;
        }

        sithAIMove_AISetMovePos(pLocal, &destPos, 1.3f);
        sithAIMove_AISetLookPosEyeLevel(pLocal, &destPos);
        return 0;
    }

    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
    return 1;
}

int J3DAPI sithAIInstinct_BasicFallow(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    SithThing* pFoe;
    SithSurface* pSurface;
    int pathCheck;
    int pathMask;
    float moveSpeed;
    float pathDistance;
    float minDist;
    float gap;
    float idealDistance;
    float length;
    int losCheck;
    float reach;
    rdVector3 toDir;
    rdVector3 movePos;
    rdVector3 targetVec;
    rdVector3 heading;
    SithThing* pGoal;

    pathCheck = 0;
    moveSpeed = 1.0f;
    length      = 0.0f;

#line 4255
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    pFoe = pLocal->pOwner;

    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_10)) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_CIRCLESTRAFING) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
        {
            return 0;
        }

        pLocal->mode &= ~SITHAI_MODE_CIRCLESTRAFING;
    }

    if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
    {
        return 0;
    }

    if ( (event & (SITHAI_EVENT_HIT_THING | SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_TOUCHED)) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
        {
            return 0;
        }

        if ( (pLocal->mode & (SITHAI_MODE_ACTIVE | SITHAI_MODE_MOVING)) == 0 )
        {
            return 0;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4) != 0 )
        {
            return 1;
        }

        sithAIUtil_GetXYHeadingVector(pFoe, &heading);
        if ( SITHAI_PARAM(pState, 0) > 3.0f )
        {
            SITHAI_PARAM(pState, 0) = 0.0f;
            SITHAI_PARAM(pState, 1) = 0.0f;
            rdVector_Copy3(&pLocal->vecUnknown3, &pLocal->movePos);
            rdVector_Neg3Acc(&heading);
            gap = RD_MULTIPLY(pFoe->collide.movesize, 4.0f);
            rdVector_ScaleAdd3(&movePos, &heading, gap, &pFoe->pos);
            if ( sithAIUtil_SweepMoveToPathPos(pLocal, &movePos, 0.0f, 60.0f, 30.0f, 0x100) )
            {
                pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_8;
                return 1;
            }
            return 0;
        }
    }

    if ( (event & (SITHAI_EVENT_HIT_THING | SITHAI_EVENT_HIT_CLIFF | SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_TOUCHED)) != 0 )
    {
        SITHAI_PARAM(pState, 1) = 1.0f;
    }

    if ( pInstinct->intArg[5] && sithPuppet_IsAnyModeOnTrack(pFoe, SITHPUPPETSUBMODE_FIRE, SITHPUPPETSUBMODE_FIRE4) )
    {
        pState->msecNextUpdate = sithTime_msecGameTime + 500;
        return 0;
    }

    if ( !pLocal->goalThing )
    {
        return 0;
    }

    pGoal = pLocal->goalThing;
    sithAIUtil_UpdateGoalSight(pLocal);
    switch ( event )
    {
        case SITHAI_EVENT_HIT_CLIFF:
            return 0;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_THING:
        {
            SithThing* pHitThing;

            pHitThing = (SithThing*)pObject;
            if ( pHitThing )
            {
                if ( pHitThing->type == SITH_THING_PLAYER )
                {
                    return 0;
                }

                rdVector_Sub3(&toDir, &pFoe->pos, &pHitThing->pos);
                toDir.z = 0.0f;
                rdVector_Normalize3Acc(&toDir);
                if ( rdVector_Dot3(&heading, &toDir) > 0.0f )
                {
                    return 1;
                }
            }

            SITHAI_PARAM(pState, 0) += 1.0f;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
            if ( sithAIUtil_SweepMoveToPathPos(pLocal, &pLocal->movePos, 45.0f, 135.0f, 45.0f, 0x100) )
            {
                return 1;
            }
            return 0;
        }

        case SITHAI_EVENT_HIT_WALL:
            pSurface = (SithSurface*)pObject;
            if ( !pSurface )
            {
                return 0;
            }

            rdVector_Copy3(&toDir, &pSurface->face.normal);
            toDir.z = 0.0f;
            toDir.x = stdMath_ClipNearZero(toDir.x);
            toDir.y = stdMath_ClipNearZero(toDir.y);
            toDir.z = stdMath_ClipNearZero(toDir.z);
            rdVector_Normalize3Acc(&toDir);

            if ( !pLocal->targetSightState2 )
            {
                float goalDot;
                float wallDot;

                targetVec.x = pLocal->vecUnknown0.x;
                targetVec.y = pLocal->vecUnknown0.y;
                targetVec.z = 0.0f;
                targetVec.x = stdMath_ClipNearZero(targetVec.x);
                targetVec.y = stdMath_ClipNearZero(targetVec.y);
                targetVec.z = stdMath_ClipNearZero(targetVec.z);
                rdVector_Normalize3Acc(&targetVec);

                goalDot = D3DVAL(RD_MULTIPLY(heading.x, targetVec.x) + RD_MULTIPLY(heading.y, targetVec.y));
                wallDot = D3DVAL(RD_MULTIPLY(heading.x, toDir.x) + RD_MULTIPLY(heading.y, toDir.y));
                if ( goalDot > 0.701f && wallDot > -0.966f )
                {
                    return 1;
                }
            }

            if ( pLocal->pClass->rank < 0.5f && SITH_RAND() > pLocal->pClass->rank )
            {
                return 0;
            }

            SITHAI_PARAM(pState, 0) += 1.0f;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
            if ( !pLocal->targetSightState2 )
            {
                pathDistance = pFoe->collide.movesize;
                gap = RD_MULTIPLY(pFoe->collide.movesize, 2.0f);
                pathCheck = sithAIUtil_CheckPathToPos(pLocal, pFoe->pInSector, &pFoe->pos, 0x2120, pathDistance, gap, 0.0f, &toDir, &movePos);
                if ( (pathCheck & 2) == 0 )
                {
                    sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                    sithAIMove_AISetMoveTargetPos(pLocal, &movePos, 1.0f);
                    return 1;
                }
            }

            pathDistance = J3DMAX(pFoe->collide.movesize, 0.089999996f);
            gap = RD_MULTIPLY(D3DVAL(SITH_RAND()), 0.1f) + pathDistance;
            pathMask = 0x8102;
            if ( sithAIUtil_MakePathPos(pLocal, &pFoe->pos, pathDistance, gap, 45.0f, pathMask, &toDir, &movePos) )
            {
                sithAIMove_StopAIMovement(pLocal);
                sithAIMove_AISetLookPosEyeLevel(pLocal, &movePos);
                sithAIMove_AISetMoveTargetPos(pLocal, &movePos, moveSpeed);
                return 1;
            }
            return 0;

        case 0:
            if ( pInstinct->intArg[0] )
            {
                pState->msecNextUpdate = sithTime_msecGameTime + pInstinct->intArg[0];
            }
            break;

        case SITHAI_EVENT_GOAL_UNREACHABLE:
            return 0;

        case SITHAI_EVENT_GOAL_REACHED:
            SITHAI_PARAM(pState, 0) = 0.0f;
            if ( SITHAI_PARAM(pState, 1) == 1.0f )
            {
                SITHAI_PARAM(pState, 1) = 0.0f;
                sithAIMove_AISetLookPosEyeLevel(pLocal, &pGoal->pos);
            }

            if ( (pLocal->mode & SITHAI_MODE_SEARCHING) != 0 )
            {
                return 0;
            }

            if ( (pLocal->submode & (SITHAI_SUBMODE_SEMICONTINUOUSMOTION | SITHAI_SUBMODE_CONTINUOUSMOTION)) == 0 )
            {
                return 0;
            }
            break;

        default:
            return 0;
    }

    pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_2000000;
    if ( (pFoe->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 && (pGoal->moveInfo.physics.flags & 0xF000000) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_100000) != 0 )
    {
        moveSpeed = 2.0f;
        minDist   = SITHAI_FLTARG(pInstinct, 1);
        reach   = J3DMIN(RD_MULTIPLY(pFoe->collide.movesize, 3.0f), minDist);
        idealDistance = 0.0f;
    }
    else
    {
        minDist   = SITHAI_FLTARG(pInstinct, 1);
        reach   = SITHAI_FLTARG(pInstinct, 2);
        idealDistance = SITHAI_FLTARG(pInstinct, 3);
        if ( sithWeapon_HasWeaponSelected(pFoe) )
        {
            reach   = sithWeapon_GetWeaponMaxAimDistance(pFoe->thingInfo.actorInfo.weaponInfo.curWeaponID);
            reach  -= 0.1f;
            idealDistance = 0.0f;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400000) != 0 )
        {
            minDist   = D3DVAL(RD_MULTIPLY(minDist, 0.5));
            reach   = D3DVAL(RD_MULTIPLY(reach, 0.5));
            idealDistance = D3DVAL(RD_MULTIPLY(idealDistance, 0.5));
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_USEMATCHVELOCITY) != 0
        && (pLocal->goalThing->moveInfo.physics.velocity.x != 0.0f || pLocal->goalThing->moveInfo.physics.velocity.y != 0.0f || pLocal->goalThing->moveInfo.physics.velocity.z != 0.0f) )
    {
        if ( pLocal->targetDistance >= 0.0f && pLocal->targetDistance <= RD_MULTIPLY(pFoe->collide.movesize, 2.0f) )
        {
            pLocal->mode |= SITHAI_MODE_UNKNOWN_8000;
        }
        else
        {
            pLocal->mode &= ~SITHAI_MODE_UNKNOWN_8000;
        }
    }
    else
    {
        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_8000;
    }

    if ( !pLocal->targetSightState2 || pLocal->targetSightState2 == 2 )
    {
        pLocal->mode &= ~SITHAI_MODE_CHASE_GOAL;
        if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
        {
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
        }

        sithAIMove_AISetLookPosEyeLevel(pLocal, &pGoal->pos);
        if ( pLocal->targetDistance > reach )
        {
            length = pLocal->targetDistance - reach;
        }
        else if ( pLocal->targetDistance < minDist )
        {
            if ( idealDistance != 0.0f )
            {
                length = pLocal->targetDistance - idealDistance;
            }
            else
            {
                length = pLocal->targetDistance - minDist;
            }
        }

        if ( length != 0.0f )
        {
            if ( length < 0.0f )
            {
                pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2000000;
            }

            rdVector_ScaleAdd3(&movePos, &pLocal->vecUnknown0, length, &pFoe->pos);
            if ( (pFoe->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
            {
                movePos.z = pGoal->pos.z + pFoe->collide.movesize;
            }
            else if ( (pFoe->flags & SITH_TF_SUBMERGED) != 0 )
            {
                movePos.z = pGoal->pos.z;
            }
        }
        else
        {
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2000000;
            if ( (pFoe->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
            {
                return 0;
            }

            length = D3DVAL(pFoe->collide.movesize / 2.0f) + pGoal->pos.z;
            if ( (pFoe->pos.z - length < 0.0f ? -(pFoe->pos.z - length) : pFoe->pos.z - length) <= 0.05f )
            {
                return 0;
            }

            movePos.x = pFoe->pos.x;
            movePos.y = pFoe->pos.y;
            movePos.z = length;
        }

        losCheck = sithAIUtil_GetDistanceToTargetPos(pGoal, &pGoal->pos, &movePos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &toDir, &gap);
        if ( SITHAI_FLTARG(pInstinct, 4) != 0.0f || !losCheck )
        {
            sithAIMove_AISetMovePos(pLocal, &movePos, moveSpeed);
            return 1;
        }

        if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
        {
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, NULL);
        }
    }
    else if ( (pLocal->mode & SITHAI_MODE_CHASE_GOAL) == 0 )
    {
        if ( (pGoal->type == SITH_THING_PLAYER || pGoal->type == SITH_THING_ACTOR) && (pGoal->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return 0;
        }

        if ( (pFoe->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            return 0;
        }

        pLocal->mode |= SITHAI_MODE_CHASE_GOAL;
        return 1;
    }

    return 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIInstinct_countfit_5, 9) /* end count-fit */
int J3DAPI sithAIInstinct_WallCrawl(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, SithSurface* pSurface)
{
    SithThing* pThing;
    SithSurface* pSurf;
    rdVector3 movePos;
    float heightDiff;
    float size;
    float dot;
    int bCurTargetVisible;  // never read; the debug frame holds this slot
    int curresult;

    curresult = 0;
    bCurTargetVisible = 0;
#line 4713
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    pThing = pLocal->pOwner;

    if ( (event & (SITHAI_EVENT_HIT_WALL | SITHAI_EVENT_HIT_FLOOR)) != 0 && (pLocal->mode & (SITHAI_MODE_ACTIVE | SITHAI_MODE_MOVING)) == 0 )
    {
        return 0;
    }

    size = pThing->collide.movesize + 0.03f;
    if ( (pLocal->mode & (SITHAI_MODE_ACTIVE | SITHAI_MODE_ATTACKING)) != 0 )
    {
        sithAIUtil_UpdateGoalSight(pLocal);
        bCurTargetVisible = pLocal->targetSightState2 == 0;
        sithAIUtil_AIGetMovePos(pLocal, 0x100, &movePos);
    }
    else
    {
        sithAIUtil_AIGetMovePos(pLocal, 0x400, &movePos);
    }

    heightDiff = movePos.z - pThing->pos.z;
    switch ( event )
    {
        case SITHAI_EVENT_HIT_CLIFF:
            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                sithAIMove_Unreachable(pLocal);
                return 1;
            }

            return 0;

        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_THING:
            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                return 1;
            }
            break;

        case SITHAI_EVENT_HIT_FLOOR:
        case SITHAI_EVENT_LAND_FLOOR:
            pSurf = pSurface;
            if ( !pSurf )
            {
                break;
            }

            if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_400000) != 0 )
            {
                pLocal->mode &= ~SITHAI_MODE_UNKNOWN_400000;
                return 1;
            }

            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_WALLCRAWLING);
                goto crawl;
            }
            break;

        case SITHAI_EVENT_HIT_WALL:
            if ( (pLocal->submode & (SITHAI_SUBMODE_WALLCRAWLLOCKED | SITHAI_SUBMODE_UNKNOWN_4)) != 0 )
            {
                return 1;
            }

            pSurf = pSurface;
            if ( !pSurf )
            {
                break;
            }

            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                goto crawl;
            }

            if ( heightDiff > size )
            {
                sithAIUtil_AISetMode(pLocal, SITHAI_MODE_WALLCRAWLING);
                goto crawl;
            }
            break;

        case 0:
            if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_ISFLOOR) != 0 )
                {
                    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_WALLCRAWLING);
                }

                pState->msecNextUpdate = sithTime_msecGameTime + 500;
                return 1;
            }
            break;

        default:
            return 0;
    }

    if ( (pLocal->mode & (SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_10 | SITHAI_MODE_SEARCHING)) != 0 )
    {
        return 0;
    }

    curresult = sithAIInstinct_BasicFallow(pLocal, pInstinct, pState, event, pSurface);
    return curresult;

crawl:
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4) == 0 && pSurf )
    {
        dot = rdVector_Dot3(&pThing->orient.lvec, &pSurf->face.normal);
        if ( dot >= 0.0f )
        {
            return 1;
        }

        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4;
        rdVector_Copy3(&pLocal->vecUnknown3, &pSurf->face.normal);
        if ( sithAIMove_StartSpecialMove(pLocal, 0x40) )
        {
            return 1;
        }

        sithThing_AttachThingToSurface(pThing, pSurf, 1);
    }

    return 1;
}





static int J3DAPI sithAIInstinct_SnakeMungeTestCheck(SithAIControlBlock* pLocal, rdVector3* pDirection, float distance, rdVector3* pOutPoint)
{
    static float sithAIInstinct_flt_539A60 = 0.3f;
    static float sithAIInstinct_flt_539A64 = 0.05f;
    static float sithAIInstinct_flt_539A68 = 0.2f;
    SithThing* pThing;
    rdVector3 delta;
    float len;
    rdVector3 curFarPos;
    int result;
    float fudge;
    SithQuetzUserBlock* pQuetz;
    rdVector3 beginPos;
    int success;  // never read; the debug frame holds this slot
    int mask;
    rdVector3 endPos;
    rdVector3 curHitPos;
    SithSector* pCurSector;

    success = 0;
    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return 0;
    }

    pQuetz = pThing->userblock.pQuetz;
    fudge = 0.0f;
    if ( pQuetz->bPosFudged )
    {
        fudge = pQuetz->unknown393;
    }

    rdVector_ScaleAdd3(&curFarPos, pDirection, distance + sithAIInstinct_flt_539A64, &pThing->pos);
    rdVector_ScaleAdd3(pOutPoint, pDirection, distance, &pThing->pos);

    rdVector_Copy3(&beginPos, &pThing->pos);
    beginPos.z = sithAIInstinct_flt_539A60 - sithPhysics_GetThingHeight(pThing) + 0.002f - fudge + beginPos.z;
    curFarPos.z = sithAIInstinct_flt_539A60 - sithPhysics_GetThingHeight(pThing) + 0.002f - fudge + curFarPos.z;
    rdVector_Copy3(&endPos, &beginPos);

    pCurSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &beginPos, 0.0f);
    if ( !pCurSector )
    {
#line 4932
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "_SnakeMungeTestCheck - couldn't find sector of start pointt", 0, 0, 0, 0);
        return 0;
    }

    rdVector_Sub3(&delta, &endPos, &beginPos);
    delta.x = stdMath_ClipNearZero(delta.x);
    delta.y = stdMath_ClipNearZero(delta.y);
    delta.z = stdMath_ClipNearZero(delta.z);
    if ( delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f )
    {
#line 4941
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "_SnakeMungeTestCheck - no LOS to start point", 0, 0, 0, 0);
        return 0;
    }

    mask |= 0x62010;
    result = sithAIUtil_CheckPathToPos(pLocal, pCurSector, &beginPos, mask, 0.0f, distance + sithAIInstinct_flt_539A64, sithAIInstinct_flt_539A60, pDirection, &curHitPos);
    switch ( result )
    {
        case 0:
            return 0;

        case 1:
            return 1;

        default:
            len = sqrtf(RD_MULTIPLY(beginPos.x - curHitPos.x, beginPos.x - curHitPos.x) + RD_MULTIPLY(beginPos.y - curHitPos.y, beginPos.y - curHitPos.y));
            if ( len - sithAIInstinct_flt_539A64 >= sithAIInstinct_flt_539A68 )
            {
                rdVector_ScaleAdd3(pOutPoint, pDirection, len - sithAIInstinct_flt_539A64, &pThing->pos);
                return 1;
            }

            return 0;
    }
}

void J3DAPI sithAIInstinct_SnakeFindMovePos(SithAIControlBlock* pLocal, rdVector3* pMovePos, const rdVector3* pAltPos)
{
    static float sithAIInstinct_flt_539A6C = 15.0f;
    static int sithAIInstinct_int_539A70 = 24;
    static float sithAIInstinct_flt_539A74 = 0.3f;
    float angleIncr;  // never read; the debug frame holds this slot
    int bFoundIt;
    int side;
    int retries;
    float altReach;
    SithThing* pThing;
    float range;
    float angle;
    rdVector3 probe;
    rdVector3 diff;
    rdVector3 angles;

    bFoundIt = 0;
    memset(&angles, 0, sizeof(angles));
    pThing = pLocal->pOwner;

    {
        rdVector3 dir;
        rdVector_Sub3(&dir, pMovePos, &pThing->pos);
        rdVector_Normalize3Acc(&dir);
        angleIncr = rdMath_DeltaAngleNormalized(&dir, &pThing->orient.lvec, &rdroid_zVector3);
        dir.z = 0.0f;
    }

    rdVector_Sub3(&diff, pMovePos, &pThing->pos);
    range = rdVector_Normalize3(&probe, &diff);
    if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, range, pMovePos) )
    {
        return;
    }

    rdVector_Sub3(&diff, pAltPos, &pThing->pos);
    altReach = rdVector_Normalize3(&probe, &diff);
    if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, altReach, pMovePos) )
    {
        return;
    }

    side = SITH_RAND() - 0.5f < 0.0f ? -1 : 1;
    rdVector_Copy3(&probe, &pThing->orient.lvec);
    angle = 0.0f;
    retries = sithAIInstinct_int_539A70;
    do
    {
        if ( sithAIInstinct_SnakeMungeTestCheck(pLocal, &probe, sithAIInstinct_flt_539A74, pMovePos) )
        {
            bFoundIt = 1;
        }

        if ( angle == 0.0f )
        {
            angle = RD_MULTIPLY(side, sithAIInstinct_flt_539A6C);
        }
        else if ( (angle < 0.0f ? -1 : 1) == side )
        {
            angle = -angle;
        }
        else
        {
            angle = -angle + RD_MULTIPLY(side, sithAIInstinct_flt_539A6C);
        }

        angles.y = angle;
        rdVector_Rotate3(&probe, &pThing->orient.lvec, &angles);
    }
    while ( !bFoundIt && retries-- );

    if ( bFoundIt )
    {
        char aText[256];
        sprintf(aText, "Retires %d", sithAIInstinct_int_539A70 - retries);
    }
    else
    {
        rdVector_Copy3(pMovePos, &pThing->pos);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIInstinct_countfit_6, 8) /* end count-fit */
signed int J3DAPI sithAIInstinct_SnakeFollow(SithAIControlBlock* pLocal, SithAIInstinct* pInstinct, SithAIInstinctState* pState, SithAIEventType event, void* pObject)
{
    static float sithAIInstinct_flt_539A78 = 2.0f;
    static float sithAIInstinct_flt_539A7C = 60.0f;
    static float sithAIInstinct_flt_539A80 = 10.0f;
    static float sithAIInstinct_flt_539A84 = 30.0f;
    static float sithAIInstinct_flt_539A88 = 45.0f;
    static float sithAIInstinct_flt_539A8C = 60.0f;
    static float sithAIInstinct_flt_539A90 = 0.2f;
    static float sithAIInstinct_flt_539A94 = 0.5f;
    static float sithAIInstinct_flt_539A98 = 0.25f;
    static float sithAIInstinct_flt_539A9C = 0.8f;
    static float sithAIInstinct_flt_539AA0 = 0.65f;
    static float sithAIInstinct_flt_539AA4 = 0.3f;
    static float sithAIInstinct_flt_539AA8 = 0.96f;
    static float sithAIInstinct_flt_539AAC = 0.5f;
    static float sithAIInstinct_flt_539AB0 = 0.5f;
    static float sithAIInstinct_flt_539AB4 = -0.98f;
    static float sithAIInstinct_flt_539AB8 = 1.0f;
    SithThing* pOwner;
    rdVector3 moveGoal;
    rdVector3 pyr2;
    SithThing* pGoalThing;  // never read; the debug frame holds this slot
    rdVector3 altPos;
    rdVector3 dirVec;
    int bAdvance;
    float arg0;  // never read; the debug frame holds this slot
    float fltArg1;  // never read; the debug frame holds this slot
    float fltArg2;  // never read; the debug frame holds this slot
    rdVector3 pyr1;
    float distance;
    float travelDist;
    float lookLen;

#line 5131
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    bAdvance = 1;
    pOwner = pLocal->pOwner;

    switch ( event )
    {
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        case SITHAI_EVENT_HIT_THING:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 || (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
            {
                return 1;
            }
            break;

        case SITHAI_EVENT_TOUCHED:
        {
            SithThing* pThing;

            pThing = (SithThing*)pObject;
            if ( pThing->type == SITH_THING_PLAYER )
            {
                bAdvance = 0;
            }
            break;
        }

        case 0:
            if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
            {
                bAdvance = 0;
            }
            break;

        case SITHAI_EVENT_GOAL_REACHED:
            break;

        default:
            return 0;
    }

    if ( !pLocal->goalThing )
    {
        pLocal->submode &= ~0x200;
        return 0;
    }

    pState->msecNextUpdate = sithTime_msecGameTime + 500;
    pGoalThing = pLocal->goalThing;
    arg0 = SITHAI_FLTARG(pInstinct, 0);
    fltArg1 = SITHAI_FLTARG(pInstinct, 1);
    fltArg2 = SITHAI_FLTARG(pInstinct, 2);
    sithAIUtil_UpdateGoalSight(pLocal);
    sithAIUtil_UpdateTargetSight(pLocal, 0);

    if ( !pLocal->targetSightState2 || pLocal->targetSightState2 == 2 )
    {
        SITHAI_PARAM(pState, 0) = 0.0f;
        rdVector_Copy3(&dirVec, &pLocal->vecUnknown0);
        distance = pLocal->targetDistance;
        pLocal->mode |= SITHAI_MODE_LOSTSIGHTOFGOAL;
    }
    else
    {
        SITHAI_PARAM(pState, 0) = 1.0f;
        rdVector_Sub3(&dirVec, &pLocal->vecUnknown5, &pLocal->pOwner->pos);
        distance = rdVector_Normalize3Acc(&dirVec);
        pLocal->mode &= ~SITHAI_MODE_LOSTSIGHTOFGOAL;
    }

    if ( bAdvance )
    {
        float dotRight;
        float dotFwd;
        float yaw;
        float turn1;
        float turnAngle2;
        float farDist;
        float nearRange;

        dotFwd = rdVector_Dot3(&pLocal->pOwner->orient.lvec, &dirVec);
        dotRight = rdVector_Dot3(&pLocal->pOwner->orient.rvec, &dirVec);
        yaw = rdMath_DeltaAngleNormalized(&pLocal->pOwner->orient.lvec, &dirVec, &rdroid_zVector3) < 0.0f
            ? -rdMath_DeltaAngleNormalized(&pLocal->pOwner->orient.lvec, &dirVec, &rdroid_zVector3)
            : rdMath_DeltaAngleNormalized(&pLocal->pOwner->orient.lvec, &dirVec, &rdroid_zVector3);

        if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
        {
            farDist = sithAIInstinct_flt_539A9C;
            nearRange = sithAIInstinct_flt_539AA4;
        }
        else
        {
            farDist = sithAIInstinct_flt_539AA0;
            nearRange = sithAIMove_flt_585468;
        }

        if ( dotFwd <= sithAIInstinct_flt_539AB4 )
        {
            turn1 = RD_MULTIPLY(SITH_RAND() - 0.5f < 0.0f ? -1 : 1, sithAIInstinct_flt_539A88);
            turnAngle2 = -turn1;
            travelDist = sithAIInstinct_flt_539A98;
            lookLen = sithAIInstinct_flt_539A98;
        }
        else if ( dotFwd < sithAIInstinct_flt_539AB0 )
        {
            turn1 = sithAIInstinct_flt_539A88;
            turnAngle2 = sithAIInstinct_flt_539A8C;
            travelDist = sithAIInstinct_flt_539A98;
            lookLen = sithAIInstinct_flt_539A98;
        }
        else if ( distance > farDist )
        {
            if ( sithAIInstinct_flt_539AB8 < sithAIInstinct_flt_539AB0 )
            {
                turn1 = yaw;
                turnAngle2 = 0.0f;
                travelDist = sithAIInstinct_flt_539A90;
                lookLen = sithAIInstinct_flt_539A90;
            }
            else
            {
                turn1 = yaw + sithAIInstinct_flt_539A84;
                turn1 = STDMATH_CLAMP(turn1, 0.0f, sithAIInstinct_flt_539A7C);
                turnAngle2 = -sithAIInstinct_flt_539A80 + yaw;
                travelDist = sithAIInstinct_flt_539A94;
                lookLen = sithAIInstinct_flt_539A94;
            }
        }
        else if ( distance > nearRange )
        {
            turn1 = yaw;
            turnAngle2 = yaw;
            if ( (pLocal->mode & SITHAI_MODE_ATTACKING) != 0 )
            {
                travelDist = distance - 0.1f;
                travelDist = STDMATH_CLAMP(travelDist, 0.1f, distance);
                lookLen = distance - 0.45f;
                lookLen = STDMATH_CLAMP(lookLen, 0.1f, distance);
            }
            else
            {
                travelDist = distance;
                lookLen = distance - 0.05f;
                lookLen = STDMATH_CLAMP(lookLen, 0.1f, distance);
            }
        }
        else
        {
            turn1 = -yaw;
            turnAngle2 = RD_MULTIPLY(yaw, 2.0f);
            travelDist = distance + 0.1f;
            lookLen = distance + 0.1f;
        }

        sithAIInstinct_flt_539AB8 = dotFwd;
        if ( dotRight > 0.0f )
        {
            rdVector_Set3(&pyr1, 0.0f, -turn1, 0.0f);
            rdVector_Set3(&pyr2, 0.0f, -turnAngle2, 0.0f);
        }
        else
        {
            rdVector_Set3(&pyr1, 0.0f, turn1, 0.0f);
            rdVector_Set3(&pyr2, 0.0f, turnAngle2, 0.0f);
        }

        rdVector_Rotate3(&dirVec, &pLocal->pOwner->orient.lvec, &pyr1);
        rdVector_ScaleAdd3(&moveGoal, &dirVec, travelDist, &pLocal->pOwner->pos);

        rdVector_Rotate3(&dirVec, &pLocal->pOwner->orient.lvec, &pyr2);
        rdVector_ScaleAdd3(&altPos, &dirVec, lookLen, &pLocal->pOwner->pos);

        moveGoal.z = pLocal->pOwner->pos.z;
        altPos.z = pLocal->pOwner->pos.z;
        sithAIInstinct_SnakeFindMovePos(pLocal, &moveGoal, &altPos);
        sithAIMove_AISetMovePos(pLocal, &moveGoal, sithAIInstinct_flt_539A78);

        rdVector_Copy3(&moveGoal, &pLocal->movePos);
        moveGoal.z += pLocal->pOwner->thingInfo.actorInfo.eyeOffset.z;
        sithAIMove_AISetLookPos(pLocal, &moveGoal);

        rdVector_Sub3(&dirVec, &pLocal->movePos, &pOwner->pos);
        dotRight = rdVector_Dot3(&pLocal->pOwner->orient.rvec, &dirVec);
        sithAIInstinct_SnakePlaySlitherSound(pOwner, dotRight);

        if ( pLocal->pOwner->userblock.pQuetz )
        {
            pLocal->pOwner->userblock.pQuetz->unknown391 = 1;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
    pLocal->submode |= 0x400;
    pLocal->submode |= 0x200;
    pLocal->submode |= 0x100;

    switch ( event )
    {
        case SITHAI_EVENT_TOUCHED:
        case SITHAI_EVENT_HIT_WALL:
        case SITHAI_EVENT_HIT_CLIFF:
        case SITHAI_EVENT_HIT_THING:
            return 1;

        default:
            return 0;
    }
}

void J3DAPI sithAIInstinct_SnakePlaySlitherSound(const SithThing* pThing, float a2)
{
    tSoundHandle hSnd;

    hSnd = 0;
    if ( (a2 < 0.0f ? -a2 : a2) < 0.1f )
    {
        a2 = 0.0f;
    }
    else if ( a2 > 0.0f )
    {
        a2 = 1.0f;
    }
    else
    {
        a2 = -1.0f;
    }

    if ( sithAIMove_flt_585464 != a2 )
    {
        if ( a2 < 0.0f )
        {
            hSnd = sithSound_Load(sithWorld_pCurrentWorld, "olv_boss_slither1.wav");
        }
        else
        {
            hSnd = sithSound_Load(sithWorld_pCurrentWorld, "olv_boss_slither2.wav");
        }

        if ( hSnd )
        {
            sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, 0.01f, 7.0f, SOUNDPLAY_THING_POS);
        }
        sithAIMove_flt_585464 = a2;
    }
}
