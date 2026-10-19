#include "sithAIUtil.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdLight.h>
#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float sithAIUtil_thingsInViewFovY = 0;
size_t sithAIUtil_numVisitedSectors = 0;
int sithAIUtil_bRenderAIWpnts = 0;
SithAIWaypointLayerFlag sithAIUtil_activeWpntLayer = 0;
size_t sithAIUtil_numThingsInView = 0;
rdVector3 sithAIUtil_mkPointCurDir = { 0 };
SithAIControlBlock* sithAIUtil_pMkPointCurLocal = NULL;
rdVector3 sithAIUtil_mkPointCurPYR = { 0 };
float sithAIUtil_thingsInViewFovX = 0;
SithAIWaypointOwner sithAIUtil_aWpntOwners[10] = { 0 };
SithThing** sithAIUtil_aThingsInView = NULL;
SithAIWaypointDistance sithAIUtil_aWpntDistances[60] = { 0 };
size_t sithAIUtil_sizeThingsInView = 0;
rdVector3 sithAIUtil_mkPointCurRVec = { 0 };
float sithAIUtil_maxDistanceToThingsInView = 0;
int sithAIUtil_thingInViewTypeMask = 0;
SithAIWaypoint sithAIUtil_aAIWpnts[60] = { 0 };
float sithAIUtil_secLastUpdate = 0;
int sithAIUtil_mkPoinCurFlags = 0;

float sithAIUtil_flt_539F4C = 0.923f;
float sithAIUtil_flt_539F50 = 0.984f;
float sithAIUtil_flt_539F54 = 0.5f;
float sithAIUtil_flt_539F58 = 1.5f;
float sithAIUtil_flt_539F5C = 1.0f;

#define sithAIUtil_Abs(x) ((x) < 0.0f ? -(x) : (x))

void J3DAPI sithAIUtil_UpdateTargetSight(SithAIControlBlock* pLocal, SithAIUtilFireFlags flags)
{
    SithThing* pOwner;
    SithThing* pTarget;
    int sightState;
    float curcollideSize;
    rdVector3 curstartPos;

    sightState  = pLocal->targetSightState;
    pOwner      = pLocal->pOwner;
    curcollideSize = 0.0f;
    if ( (flags & (SITHAIUTIL_FIRE_LOB | SITHAIUTIL_FIRE_PRIMARY | SITHAIUTIL_FIRE_ALT)) == 0 && pLocal->unknown124 == sithMain_frameNumber )
    {
        return;
    }

    pLocal->unknown124 = sithMain_frameNumber;
    if ( sithWeapon_HasWeaponSelected(pOwner) )
    {
        curcollideSize = sithWeapon_GetWeaponCollideSize(pOwner->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }
    else if ( pOwner->thingInfo.actorInfo.pWeaponTemplate )
    {
        curcollideSize = pOwner->thingInfo.actorInfo.pWeaponTemplate->collide.movesize;
    }

    rdMatrix_TransformVector34(&pLocal->weaponFirePos, &pOwner->thingInfo.actorInfo.fireOffset, &pOwner->orient);
    rdVector_Add3Acc(&pLocal->weaponFirePos, &pOwner->pos);
    rdVector_Copy3(&curstartPos, &pLocal->weaponFirePos);

    if ( (pLocal->submode & SITHAI_SUBMODE_FIREADDEYEOFFSET) != 0
        && (pOwner->thingInfo.actorInfo.eyeOffset.x != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.y != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.z != 0.0f) )
    {
        curstartPos.z = RD_MULTIPLY(pOwner->thingInfo.actorInfo.eyeOffset.z, 0.5f) + curstartPos.z;
    }

    pLocal->mode &= ~SITHAI_MODE_TARGETVISIBLE;
    pTarget = pLocal->pTargetThing;
    if ( pTarget )
    {
        if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
        {
            if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
            {
                sightState = 3;
            }

            if ( (pLocal->pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
            {
                sightState = 3;
            }
        }

        rdVector_Copy3(&pLocal->targetPos, &pTarget->pos);
        pLocal->targetSightState = sithAIUtil_GetDistanceToTarget(pOwner, &curstartPos, pTarget, pLocal->pClass->fov, pLocal->pClass->sightDistance, curcollideSize, &pLocal->toTarget, &pLocal->distance);
        if ( pLocal->targetSightState == 0 )
        {
            if ( sightState && !sithAIUtil_CanSeeTarget(pLocal, pTarget, pLocal->distance) )
            {
                pLocal->targetSightState = 3;
                return;
            }

            rdVector_Copy3(&pLocal->vecUnknown122, &pTarget->pos);
            pLocal->msecAttackStart = sithTime_msecGameTime;
            pLocal->mode |= SITHAI_MODE_TARGETVISIBLE;
        }
    }
    else
    {
        pLocal->targetSightState = sithAIUtil_GetDistanceToTargetPos(pOwner, &curstartPos, &pLocal->targetPos, pLocal->pClass->fov, pLocal->pClass->sightDistance, curcollideSize, &pLocal->toTarget, &pLocal->distance);
        if ( pLocal->targetSightState == 0 )
        {
            rdVector_Copy3(&pLocal->vecUnknown122, &pLocal->targetPos);
            pLocal->msecAttackStart = sithTime_msecGameTime;
            pLocal->mode |= SITHAI_MODE_TARGETVISIBLE;
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_FIREADDEYEOFFSET) != 0
        && (pOwner->thingInfo.actorInfo.eyeOffset.x != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.y != 0.0f || pOwner->thingInfo.actorInfo.eyeOffset.z != 0.0f) )
    {
        rdVector_Sub3(&pLocal->toTarget, &pLocal->targetPos, &pLocal->weaponFirePos);
        if ( pTarget )
        {
            pLocal->distance = rdVector_Normalize3Acc(&pLocal->toTarget) - pTarget->collide.size;
        }
        else
        {
            pLocal->distance = rdVector_Normalize3Acc(&pLocal->toTarget);
        }

        pLocal->distance = J3DMAX(pLocal->distance, 0.0f);
    }
}

void J3DAPI sithAIUtil_UpdateGoalSight(SithAIControlBlock* pLocal)
{
    SithThing* pOwner;
    SithThing* pTarget;
    int curSightState;

    curSightState = pLocal->targetSightState2;
    pOwner     = pLocal->pOwner;
    if ( pLocal->unknown141 == sithMain_frameNumber )
    {
        return;
    }

    pLocal->unknown141 = sithMain_frameNumber;
    pTarget = pLocal->goalThing;
    if ( pTarget )
    {
        if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
        {
            if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
            {
                curSightState = 3;
            }

            if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
            {
                curSightState = 3;
            }
        }

        pLocal->targetSightState2 = sithAIUtil_GetDistanceToTarget(pOwner, &pOwner->pos, pTarget, -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->vecUnknown0, &pLocal->targetDistance);
        if ( pLocal->targetSightState2 == 0 )
        {
            if ( curSightState && !sithAIUtil_CanSeeTarget(pLocal, pTarget, pLocal->targetDistance) )
            {
                pLocal->targetSightState2 = 3;
                return;
            }

            rdVector_Copy3(&pLocal->vecUnknown5, &pTarget->pos);
            pLocal->unknown150 = sithTime_msecGameTime;
        }
    }
    else
    {
        rdVector_Copy3(&pLocal->vecUnknown6, &pLocal->movePos);
        pLocal->targetSightState2 = sithAIUtil_GetDistanceToTargetPos(pOwner, &pOwner->pos, &pLocal->vecUnknown6, -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->vecUnknown0, &pLocal->targetDistance);
        if ( pLocal->targetSightState2 == 0 )
        {
            rdVector_Copy3(&pLocal->vecUnknown5, &pLocal->vecUnknown6);
            pLocal->unknown150 = sithTime_msecGameTime;
        }
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIUtil_countfit_1, 7) /* end count-fit */
// returns target sight state:
// 0 - target in sight
// 1 - target too far away (dist > sightDistance)
// 2 - target out of FOV view
// 3 - target not in sight
int J3DAPI sithAIUtil_GetDistanceToTarget(SithThing* pViewer, rdVector3* startPos, const SithThing* pTarget, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    int sightState;
    float curdotL;
    float dotR;
    SithSector* pSector;

    sightState = 2;

#line 423
    SITH_ASSERTREL(toTarget && pDist);
    SITH_ASSERTREL(pViewer && pTarget && startPos);

    rdVector_Sub3(toTarget, &pTarget->pos, startPos);
    *pDist = rdVector_Normalize3Acc(toTarget) - pTarget->collide.size;
    *pDist = *pDist > 0.0f ? *pDist : 0.0f;

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            return 3;
        }

        if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            return 3;
        }
    }

    if ( (pViewer->flags & SITH_TF_SUBMERGED) == 0 && (pTarget->flags & SITH_TF_SUBMERGED) != 0 && (pTarget->moveType != SITH_MT_PHYSICS || (pTarget->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0) )
    {
        return 3;
    }

    if ( (pViewer->flags & SITH_TF_SUBMERGED) != 0 && (pTarget->flags & SITH_TF_SUBMERGED) == 0 )
    {
        return 3;
    }

    if ( *pDist - pTarget->collide.size > sightDistance )
    {
        return 1;
    }

    if ( fov > -1.0f )
    {
        curdotL = rdVector_Dot3(toTarget, &pViewer->orient.lvec);
        dotR = sithAIUtil_Abs(rdVector_Dot3(toTarget, &pViewer->orient.rvec));

        if ( fov >= 0.0f && (curdotL < 0.0f || dotR > D3DVAL(1.0f - fov)) )
        {
            return 2;
        }

        if ( fov < 0.0f && curdotL < 0.0f && dotR < D3DVAL(fov + 1.0f) )
        {
            return 2;
        }
    }

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) != 0 )
    {
        return 0;
    }

    pSector = sithCollision_FindSectorInRadius(pViewer->pInSector, &pViewer->pos, startPos, 0.0f);
    if ( sithCollision_CanMoveToPos(pSector, startPos, &pTarget->pos, 0.0f) )
    {
        return 0;
    }

    return 3;
}

int J3DAPI sithAIUtil_GetDistanceToTargetPos(SithThing* pViewer, rdVector3* startPos, rdVector3* endPos, float fov, float sightDistance, float collisionSize, rdVector3* toTarget, float* pDist)
{
    float curdotL;
    float dotR;
    SithSector* pSector;

#line 513
    SITH_ASSERTREL(toTarget && pDist);
    SITH_ASSERTREL(pViewer && startPos);

    rdVector_Sub3(toTarget, endPos, startPos);
    *pDist = rdVector_Normalize3Acc(toTarget);
    if ( *pDist > sightDistance )
    {
        return 1;
    }

    if ( fov > -1.0f )
    {
        curdotL = rdVector_Dot3(toTarget, &pViewer->orient.lvec);
        dotR = sithAIUtil_Abs(rdVector_Dot3(toTarget, &pViewer->orient.rvec));

        if ( fov >= 0.0f && (curdotL < 0.0f || dotR > D3DVAL(1.0f - fov)) )
        {
            return 2;
        }

        if ( fov < 0.0f && curdotL < 0.0f && dotR < D3DVAL(fov + 1.0f) )
        {
            return 2;
        }
    }

    if ( (pViewer->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) != 0 )
    {
        return 0;
    }

    pSector = sithCollision_FindSectorInRadius(pViewer->pInSector, &pViewer->pos, startPos, 0.0f);
    if ( sithCollision_CanMoveToPos(pSector, startPos, endPos, 0.0f) )
    {
        return 0;
    }

    return 3;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIUtil_countfit_2, 2) /* end count-fit */
int J3DAPI sithAIUtil_AIFire(SithAIControlBlock* pLocal, float minDist, float maxDist, float fireDot, float aimError, int weaponNum, SithAIUtilFireFlags flags, int burstCount)
{
    SithThing* pShotTemplate;
    SithThing* pBullet;
    SithThing* pAttacker;
    SithInventoryType* pActiveWeaponType;
    SithPuppetSubMode curSubMode;
    int bHasFired;
    rdVector3 targetDir;
    SithWeaponId weaponIdx;
    SithFireProjectileFlags projFlags;
    float lobFactor;
    float accuracy;
    float leadFlightTime;
    float distAimScale;
    rdVector3 predictVec;
    float speed;

    lobFactor           = 1.0f;
    pShotTemplate       = NULL;
    curSubMode         = 0;
    projFlags = 0;
    bHasFired          = 0;

#line 602
    SITH_ASSERTREL(pLocal->pOwner && pLocal->pClass);

    pAttacker = pLocal->pOwner;
    if ( (sith_mode.debugModeFlags & SITHDEBUG_AIDISABLED) != 0 )
    {
        return 0;
    }

    if ( (pAttacker->flags & (SITH_TF_DESTROYED | SITH_TF_DYING)) != 0 )
    {
        return 0;
    }

    if ( !pLocal->pTargetThing )
    {
        return 0;
    }

    if ( (pAttacker->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pAttacker->thingInfo.actorInfo.flags & SITH_AF_NOUNDERWATERFIRE) != 0 )
    {
        return 0;
    }

    sithAI_EmitEvent(pLocal, SITHAI_EVENT_PREFIRE, (void*)flags);

    pShotTemplate   = pAttacker->thingInfo.actorInfo.pWeaponTemplate;
    weaponIdx = pAttacker->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( !pShotTemplate )
    {
        if ( !((unsigned int)weaponIdx >= SITHWEAPON_COMFISTS ? (unsigned int)weaponIdx <= SITHWEAPON_COMSHOTGUN : 0) )
        {
            goto postfire;
        }

        speed = 5.0f;
    }
    else
    {
        speed = pShotTemplate->moveInfo.physics.velocity.y;
    }

    sithAIUtil_UpdateTargetSight(pLocal, flags);
    rdVector_Copy3(&targetDir, &pLocal->toTarget);

    if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 )
    {
        if ( pLocal->msecFireWaitTime > sithTime_msecGameTime )
        {
            goto postfire;
        }

        if ( pLocal->targetSightState || !J3DINRANGE(pLocal->distance, minDist, maxDist) )
        {
            goto postfire;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_SKIPCHECKFIREFOV) == 0 && rdVector_Dot3(&pAttacker->orient.lvec, &targetDir) < fireDot )
        {
            goto postfire;
        }

        if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_40) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE;
        }
        else if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_80) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE2;
        }
        else if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_100) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE3;
        }
        else if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_200) != 0 )
        {
            curSubMode = SITHPUPPETSUBMODE_FIRE4;
        }

        if ( (flags & SITHAIUTIL_FIRE_STOP_ON_FIRE) != 0 )
        {
            sithAIMove_AIStop(pLocal);
        }
    }

    if ( weaponNum >= 0 && (pLocal->fireDot = fireDot, pLocal->minFireDist = minDist, pLocal->maxFireDist = maxDist, (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0) && (pAttacker->thingInfo.actorInfo.flags & SITH_AF_DELAYFIRE) != 0 )
    {
        pLocal->fireFlags     = flags | SITHAIUTIL_FIRE_UNKNOWN_8;
        pLocal->aimError      = aimError;
        pLocal->fireWeaponNum = weaponNum;
        sithPuppet_PlayMode(pAttacker, curSubMode, NULL);
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
        bHasFired = 1;
    }
    else
    {
        if ( (flags & SITHAIUTIL_FIRE_LEAD) != 0 && pLocal->pTargetThing->moveType == SITH_MT_PHYSICS
            && (pLocal->pTargetThing->moveInfo.physics.velocity.x != 0.0f || pLocal->pTargetThing->moveInfo.physics.velocity.y != 0.0f || pLocal->pTargetThing->moveInfo.physics.velocity.z != 0.0f) )
        {
            rdVector_Scale3(&predictVec, &targetDir, speed);
            rdVector_Add3Acc(&predictVec, &pLocal->pTargetThing->moveInfo.physics.velocity);
            rdVector_Normalize3Acc(&predictVec);
            if ( rdVector_Dot3(&predictVec, &targetDir) > 0.5f )
            {
                rdVector_Copy3(&targetDir, &predictVec);
            }
        }

        if ( (flags & SITHAIUTIL_FIRE_LOB) != 0 )
        {
            rdVector_Scale3(&predictVec, &pLocal->toTarget, speed);
            leadFlightTime = pLocal->distance / speed;
            predictVec.z += RD_MULTIPLY(RD_MULTIPLY(leadFlightTime, 0.5f), sithWorld_pCurrentWorld->gravity);
            lobFactor = rdVector_Normalize3(&targetDir, &predictVec) / speed;
            projFlags |= 1;
        }

        accuracy = RD_MULTIPLY(pLocal->pClass->accurancy, sithGetHitAccuarancyScalar());
        if ( aimError != 0.0f && pLocal->distance != 0.0f && SITH_RAND() > accuracy )
        {
            if ( pLocal->distance > 0.5f )
            {
                distAimScale = RD_MULTIPLY(pLocal->distance - 0.5f, 0.8f) + 1.0f;
            }

            aimError = RD_MULTIPLY(sithGetCombatDamageScalar(), aimError);
            sithAIUtil_RandomRotate(&targetDir, aimError);
        }

        if ( pShotTemplate )
        {
            if ( pAttacker->pAttachedThing && (pAttacker->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                sithAIMove_StartQuetzStrike(pLocal);
            }

            if ( weaponNum < 0 )
            {
                weaponNum = 0;
            }

            sithSoundClass_PlayModeRandom(pAttacker, SITHSOUNDCLASS_FIRE1 + weaponNum);
            rdVector_Copy3(&pAttacker->thingInfo.actorInfo.weaponInfo.vecUnknown0, &targetDir);
            pBullet = sithWeapon_WeaponFire(pAttacker, pShotTemplate, &targetDir, &pLocal->weaponFirePos, 0, curSubMode, lobFactor, projFlags, 0.0f);
            if ( pBullet )
            {
                sithCog_ThingSendMessageEx(pAttacker, pBullet, SITHCOG_MSG_FIRE, (int)pShotTemplate->thingInfo.weaponInfo.damage, pShotTemplate->thingInfo.weaponInfo.damageType, weaponNum, 0);
                if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 )
                {
                    sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
                }

                bHasFired = 1;
                goto postfire;
            }
        }
        else
        {
            pActiveWeaponType = sithInventory_GetType(pAttacker->thingInfo.actorInfo.weaponInfo.curWeaponID);
            if ( pActiveWeaponType && pActiveWeaponType->pCog )
            {
                rdVector_Sub3(&pAttacker->thingInfo.actorInfo.weaponInfo.vecUnknown0, &targetDir, &pLocal->toTarget);
                if ( sithWeapon_HasWeaponSelected(pAttacker) && !sithWeapon_IsAiming(pAttacker) )
                {
                    goto postfire;
                }

                if ( !sithWeapon_IsMountingWeapon(pAttacker) )
                {
                    sithCog_SendMessageEx(pActiveWeaponType->pCog, SITHCOG_MSG_ACTIVATE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pAttacker->idx, 0, burstCount, 0, 0, 0);
                    if ( (flags & SITHAIUTIL_FIRE_UNKNOWN_8) == 0 )
                    {
                        sithAI_EmitEvent(pLocal, SITHAI_EVENT_FIRE, pLocal->pTargetThing);
                    }

                    bHasFired = 1;
                }
            }

            goto postfire;
        }

        return 0;
    }

postfire:
    sithAI_EmitEvent(pLocal, SITHAI_EVENT_POSTFIRE, (void*)flags);
    return bHasFired;
}

void J3DAPI sithAIUtil_AIPauseMove(SithAIControlBlock* pLocal, int msecPause)
{
    pLocal->msecPauseMoveUntil = sithTime_msecGameTime + msecPause;
}

void J3DAPI sithAIUtil_AIResetWaypoints(void)
{
    int i;

    for ( i = 0; i < 60; ++i )
    {
        sithAIUtil_aAIWpnts[i].thingNum     = -1;
        sithAIUtil_aAIWpnts[i].flags        = SITH_AIWPNT_LAYER0;
        sithAIUtil_aAIWpnts[i].numUsedLinks = 0;
        sithAIUtil_aAIWpnts[i].rank    = 0;
    }

    for ( i = 0; i < 10; ++i )
    {
        sithAIUtil_aWpntOwners[i].thingNum          = -1;
        sithAIUtil_aWpntOwners[i].waypointNum       = 0;
        sithAIUtil_aWpntOwners[i].cosTurnAlignAngle = 0.0f;
    }

    sithAIUtil_activeWpntLayer   = SITH_AIWPNT_LAYER0;
    sithAIUtil_bRenderAIWpnts = 0;
}

int J3DAPI sithAIUtil_SyncWpnts(DPID idTo, unsigned int outstream)
{
    int wpntNum, j;
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

    for ( wpntNum = 0; wpntNum < 60; ++wpntNum )
    {
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].thingNum);
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].flags);
        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].numUsedLinks);
        for ( j = 0; j < 8; ++j )
        {
            SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].aWpntLinks[j]);
        }

        SITHDSS_PUSHINT32(sithAIUtil_aAIWpnts[wpntNum].rank);
    }

    for ( wpntNum = 0; wpntNum < 10; ++wpntNum )
    {
        SITHDSS_PUSHINT32(sithAIUtil_aWpntOwners[wpntNum].thingNum);
        SITHDSS_PUSHINT32(sithAIUtil_aWpntOwners[wpntNum].waypointNum);
        SITHDSS_PUSHFLOAT(sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle);
    }

    *(int*)pCurDSSMsgData_ = sithAIUtil_activeWpntLayer;
    pCurDSSMsgData_ += 4;

    sithMulti_message.type   = SITHDSS_SYNCWPNT;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 1u);
}

int J3DAPI sithAIUtil_ProcessSyncWpnts(const SithMessage* pMsg)
{
    int idx, j;
    const int* pData = (const int*)&pMsg->data[0];

#line 936
    SITH_ASSERTREL(pMsg);

    for ( idx = 0; idx < 60; ++idx )
    {
        sithAIUtil_aAIWpnts[idx].thingNum     = *pData++;
        sithAIUtil_aAIWpnts[idx].flags        = *pData++;
        sithAIUtil_aAIWpnts[idx].numUsedLinks = *pData++;
        for ( j = 0; j < 8; ++j )
        {
            sithAIUtil_aAIWpnts[idx].aWpntLinks[j] = *pData++;
        }

        sithAIUtil_aAIWpnts[idx].rank = *pData++;
    }

    for ( idx = 0; idx < 10; ++idx )
    {
        sithAIUtil_aWpntOwners[idx].thingNum          = *pData++;
        sithAIUtil_aWpntOwners[idx].waypointNum       = *pData++;
        sithAIUtil_aWpntOwners[idx].cosTurnAlignAngle = *(const float*)pData++;
    }

    sithAIUtil_activeWpntLayer = *pData++;
    return 1;
}

void J3DAPI sithAIUtil_AISetWpnt(SithThing* pGhost, unsigned int wpntIdx)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
#line 968
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts), 0, 0);
        return;
    }

    if ( !pGhost || pGhost->type != SITH_THING_GHOST )
    {
#line 973
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpnt: Non-GHOST type THING passed as waypoint.\n", 0, 0, 0, 0);
        return;
    }

    sithAIUtil_aAIWpnts[wpntIdx].thingNum     = pGhost->idx;
    sithAIUtil_aAIWpnts[wpntIdx].flags        = SITH_AIWPNT_LAYER0;
    sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks = 0;
}

SithThing* J3DAPI sithAIUtil_AIGetWpntThing(int wpntIdx)
{
    return &sithWorld_pCurrentWorld->aThings[sithAIUtil_aAIWpnts[wpntIdx].thingNum];
}

void J3DAPI sithAIUtil_AISetWpntRank(unsigned int wpntIdx, unsigned int rank)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
#line 992
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts), 0, 0);
        return;
    }

    rank = STDMATH_CLAMP(rank, 0, 0xFF);

    sithAIUtil_aAIWpnts[wpntIdx].flags &= ~0xFF;
    sithAIUtil_aAIWpnts[wpntIdx].flags |= rank;
}

void J3DAPI sithAIUtil_AISetWpntFlags(unsigned int wpntIdx, unsigned int flags)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
#line 1007
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts), 0, 0);
        return;
    }

    flags = STDMATH_CLAMP(flags, 0x100, 0xFFFFFF00);

    sithAIUtil_aAIWpnts[wpntIdx].flags |= flags;
}

void J3DAPI sithAIUtil_AIClearWpntFlags(unsigned int wpntIdx, unsigned int flags)
{
    if ( wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
#line 1021
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpnt: Index (%d) out of allowed range (0 - %d).\n", wpntIdx, STD_ARRAYLEN(sithAIUtil_aAIWpnts), 0, 0);
        return;
    }

    flags = STDMATH_CLAMP(flags, 0x100, 0xFFFFFF00);

    sithAIUtil_aAIWpnts[wpntIdx].flags &= ~flags;
}

void J3DAPI sithAIUtil_AISetActiveWpntLayer(SithAIWaypointLayerFlag layer)
{
    if ( (layer & 0x3F000) == 0 )
    {
#line 1035
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpntLayer: Invalid layer flag (%x).\n", layer, 0, 0, 0);
        return;
    }

    sithAIUtil_activeWpntLayer = layer;
}

void J3DAPI sithAIUtil_AIConnectWpnts(unsigned int wpntIdx1, unsigned int wpntIdx2, int bConnectWpnt1To2)
{
    if ( wpntIdx1 >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) || wpntIdx2 >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
#line 1047
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AISetWpnt: Index (%d or %d) out of allowed range (0 - %d).\n", wpntIdx1, wpntIdx2, STD_ARRAYLEN(sithAIUtil_aAIWpnts), 0);
        return;
    }

    if ( sithAIUtil_aAIWpnts[wpntIdx1].thingNum == -1 )
    {
#line 1052
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIConnectWpnts: Invalid waypoint (#%d).\n", wpntIdx1, 0, 0, 0);
        return;
    }

    if ( sithAIUtil_aAIWpnts[wpntIdx2].thingNum == -1 )
    {
#line 1057
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIConnectWpnts: Invalid waypoint (#%d).\n", wpntIdx2, 0, 0, 0);
        return;
    }

    if ( (unsigned int)sithAIUtil_aAIWpnts[wpntIdx1].numUsedLinks < STD_ARRAYLEN(sithAIUtil_aAIWpnts[wpntIdx1].aWpntLinks) )
    {
        sithAIUtil_aAIWpnts[wpntIdx1].aWpntLinks[sithAIUtil_aAIWpnts[wpntIdx1].numUsedLinks] = wpntIdx2;
        sithAIUtil_aAIWpnts[wpntIdx1].numUsedLinks++;
    }
    else
    {
#line 1070
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIConnectWpnts: Cant connect (#%d) to (#%d). Too many links\n", wpntIdx2, wpntIdx1, 0, 0);
    }

    if ( bConnectWpnt1To2 == 1 )
    {
        if ( (unsigned int)sithAIUtil_aAIWpnts[wpntIdx2].numUsedLinks < STD_ARRAYLEN(sithAIUtil_aAIWpnts[wpntIdx2].aWpntLinks) )
        {
            sithAIUtil_aAIWpnts[wpntIdx2].aWpntLinks[sithAIUtil_aAIWpnts[wpntIdx2].numUsedLinks] = wpntIdx1;
            sithAIUtil_aAIWpnts[wpntIdx2].numUsedLinks++;
        }
        else
        {
#line 1084
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIConnectWpnts: Cannot connect (#%d) to (#%d). Too many links.\n", wpntIdx1, wpntIdx2, 0, 0);
        }
    }
}

int J3DAPI sithAIUtil_AIMoveToNextWpnt(SithAIControlBlock* pLocal, float moveSpeed, float degTurn, int mode)
{
    int wpntIdx;

    wpntIdx = sithAIUtil_AIFindNearestWpnt(pLocal->pOwner, 1);
    if ( wpntIdx > -1 )
    {
        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
        {
            degTurn = 180.0f;
        }

        sithAIUtil_AITraverseWpnts(pLocal, wpntIdx, moveSpeed, degTurn, mode);
        return 1;
    }

    return 0;
}

void J3DAPI sithAIUtil_AITraverseWpnts(SithAIControlBlock* pLocal, unsigned int wpntIdx, float moveSpeed, float degTurn, unsigned int mode)
{
    SithWorld* pCurWorld;
    int thingNum;
    int ownerNum;

    pCurWorld = sithWorld_pCurrentWorld;

#line 1119
    SITH_ASSERTREL(pLocal);

    if ( !pCurWorld || wpntIdx >= STD_ARRAYLEN(sithAIUtil_aAIWpnts) )
    {
#line 1123
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AITraverseWpnts: Index out of allowed range.\n", 0, 0, 0, 0);
        return;
    }

    thingNum = sithAIUtil_aAIWpnts[wpntIdx].thingNum;
    if ( thingNum > pCurWorld->lastThingIdx || pCurWorld->aThings[thingNum].type != SITH_THING_GHOST )
    {
#line 1130
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AITraverseWpnts: Invalid waypoint (#%d).\n", wpntIdx, 0, 0, 0);
        return;
    }

    if ( degTurn == 0.0f )
    {
        degTurn = pLocal->pClass->degTurnAlign;
        if ( degTurn == 0.0f )
        {
            degTurn = 45.0f;
        }
    }

    ownerNum = sithAIUtil_CreateAIWaypoint(pLocal, degTurn);
    if ( ownerNum == -1 )
    {
#line 1145
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AITraverseWpnts: Too many AI's using waypoints (max = %d).\n", STD_ARRAYLEN(sithAIUtil_aWpntOwners), 0, 0, 0);
        sithAIMove_AIStop(pLocal);
        return;
    }

    pLocal->mode |= SITHAI_MODE_TRAVERSEWPNTS;
    if ( mode == 1 )
    {
        pLocal->submode |= SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION;
    }
    else if ( mode > 1 )
    {
        pLocal->submode |= SITHAI_SUBMODE_CONTINUOUSWPNTMOTION;
    }

    sithAIUtil_aWpntOwners[ownerNum].waypointNum = (sithAIUtil_aWpntOwners[ownerNum].waypointNum & ~0xFF00) | (wpntIdx << 8);

    if ( sithAIMove_AISetMovePos(pLocal, &pCurWorld->aThings[thingNum].pos, moveSpeed) )
    {
        sithAIMove_AISetLookPosEyeLevel(pLocal, &pCurWorld->aThings[thingNum].pos);
        sithAIUtil_CheckWpntMoveAlign(pLocal);
        return;
    }

    pLocal->moveSpeed = moveSpeed;
    sithAIUtil_AIAdvanceToNextWpnt(pLocal, 0);
}

static int J3DAPI sithAIUtil_SelectWpntLink(SithAIControlBlock* pLocal, unsigned int numLinks, int* aWypointLinks, int linkNum, int wpntOwnerNum)
{
    rdVector3 targetDir;
    rdVector3 wpntDir;
    float threshold;
    float playerDistance;
    int rankLead;
    float fleeFacing;
    int idxThing;
    float slopeStep;
    int bestPriority;
    int curRank;
    int nextWpnt;
    int leadWpnt;
    unsigned int idx;
    SithWorld* pWorld;

    pWorld = sithWorld_pCurrentWorld;
    if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 )
    {
        sithAIUtil_RankWpntsFromPlayer();
    }
    else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            sithAIUtil_RankWpntsFromThing(pLocal->pFleeFromThing);
        }
        else
        {
            sithAIUtil_RankWpntsFromPlayer();
        }
    }

    leadWpnt = aWypointLinks[linkNum];
    for ( idx = linkNum + 1; idx < numLinks; ++idx )
    {
        nextWpnt = aWypointLinks[idx];
        if ( (sithAIUtil_aAIWpnts[nextWpnt].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[nextWpnt].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        bestPriority = sithAIUtil_aAIWpnts[leadWpnt].flags & 0xFF;
        curRank  = sithAIUtil_aAIWpnts[nextWpnt].flags & 0xFF;

        if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && pLocal->pFleeFromThing && (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) == 0 )
        {
            if ( sithAIUtil_aAIWpnts[leadWpnt].rank == 0xFF )
            {
                bestPriority -= 14;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank == 0xFF )
            {
                curRank -= 14;
            }

            rdVector_Sub3(&targetDir, &pLocal->pFleeFromThing->pos, &pLocal->pOwner->pos);
            playerDistance = rdVector_Normalize3Acc(&targetDir);
            slopeStep = (sithAIUtil_flt_539F50 - sithAIUtil_flt_539F4C) / (sithAIUtil_flt_539F58 - sithAIUtil_flt_539F54);
            threshold = RD_MULTIPLY(playerDistance - sithAIUtil_flt_539F54, slopeStep) + sithAIUtil_flt_539F4C;
            threshold = STDMATH_CLAMP(threshold, sithAIUtil_flt_539F4C, sithAIUtil_flt_539F50);

            idxThing = sithAIUtil_aAIWpnts[leadWpnt].thingNum;
            rdVector_Sub3(&wpntDir, &pWorld->aThings[idxThing].pos, &pLocal->pOwner->pos);
            rdVector_Normalize3Acc(&wpntDir);
            fleeFacing = rdVector_Dot3(&targetDir, &wpntDir);
            if ( fleeFacing > threshold )
            {
                bestPriority -= 10;
            }

            idxThing = sithAIUtil_aAIWpnts[nextWpnt].thingNum;
            rdVector_Sub3(&wpntDir, &pWorld->aThings[idxThing].pos, &pLocal->pOwner->pos);
            rdVector_Normalize3Acc(&wpntDir);
            fleeFacing = rdVector_Dot3(&targetDir, &wpntDir);
            if ( fleeFacing > threshold )
            {
                curRank -= 10;
            }

            rankLead = sithAIUtil_aAIWpnts[nextWpnt].rank - sithAIUtil_aAIWpnts[leadWpnt].rank;
            if ( rankLead < 0 )
            {
                curRank += -rankLead * 3;
            }
            else if ( rankLead > 0 )
            {
                bestPriority += rankLead * 3;
            }

            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        --curRank;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        --bestPriority;
                    }
                }
            }
        }
        else if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 && pLocal->pFleeFromThing && (pLocal->mode & SITHAI_MODE_FLEEINGTOWAYPOINT) != 0 )
        {
            bestPriority = 0;
            curRank  = 0;
            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        curRank -= 6;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        bestPriority -= 6;
                    }
                }
            }

            if ( sithAIUtil_aAIWpnts[nextWpnt].rank > sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                curRank += 7;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank < sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                bestPriority += 7;
            }
        }
        else if ( (pLocal->mode & SITHAI_MODE_HUNTING) != 0 && pLocal->pTargetThing )
        {
            bestPriority = 0;
            curRank  = 0;
            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        curRank -= 6;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        bestPriority -= 6;
                    }
                }
            }

            if ( sithAIUtil_aAIWpnts[nextWpnt].rank > sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                curRank += 7;
            }
            else if ( sithAIUtil_aAIWpnts[nextWpnt].rank < sithAIUtil_aAIWpnts[leadWpnt].rank )
            {
                bestPriority += 7;
            }
        }
        else if ( (pLocal->mode & (SITHAI_MODE_ATTACKING | SITHAI_MODE_SEARCHING)) != 0 )
        {
            if ( wpntOwnerNum != -1 )
            {
                if ( (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) < 60 )
                {
                    if ( nextWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        curRank -= 6;
                    }

                    if ( leadWpnt == (sithAIUtil_aWpntOwners[wpntOwnerNum].waypointNum & 0xFF) )
                    {
                        bestPriority -= 6;
                    }
                }
            }

            if ( curRank < bestPriority && SITH_RAND() > RD_MULTIPLY(bestPriority - curRank, 0.1f) + 0.5f )
            {
                bestPriority = -bestPriority;
            }
        }

        if ( curRank > bestPriority || (curRank == bestPriority && SITH_RAND() > 0.5f) )
        {
            leadWpnt = nextWpnt;
        }
    }

    return leadWpnt;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIUtil_countfit_3, 2) /* end count-fit */
int J3DAPI sithAIUtil_AIAdvanceToNextWpnt(SithAIControlBlock* pLocal, int bNotify)
{
    int thingNum;
    SithWorld* pCurLevel;
    int wpntIdx;
    int* aLinks;
    int bLinkUsable;
    int i;
    int nextWpntNum;
    int ownerIdx;

    pCurLevel = sithWorld_pCurrentWorld;

#line 1420
    SITH_ASSERTREL(pLocal && pLocal->pOwner);

    ownerIdx = sithAIUtil_GetAIWaypointNum(pLocal);
    if ( ownerIdx == -1 )
    {
#line 1425
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIAdvanceToNextWpnt: AI '%s' not in waypoint tracking list.\n", pLocal->pOwner->aName, 0, 0, 0);
    }
    else
    {
        wpntIdx = (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & 0xFF00) >> 8;
        sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_UPDATEWPNTS, 0, 0, 0, 0);

        if ( wpntIdx < 60 )
        {
            if ( sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks > 0 )
            {
                aLinks = sithAIUtil_aAIWpnts[wpntIdx].aWpntLinks;
                bLinkUsable = 0;
                for ( i = 0; i < (int)sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks; ++i )
                {
                    nextWpntNum = aLinks[i];
                    if ( (sithAIUtil_aAIWpnts[nextWpntNum].flags & sithAIUtil_activeWpntLayer) != 0 && (sithAIUtil_aAIWpnts[nextWpntNum].flags & SITH_AIWPNT_DISABLED) == 0 )
                    {
                        bLinkUsable = 1;
                        break;
                    }
                }

                if ( !bLinkUsable )
                {
                }
                else if ( sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks == 1 && aLinks[0] == (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & 0xFF) && (pLocal->submode & (SITHAI_SUBMODE_CONTINUOUSWPNTMOTION | SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION)) == 0 )
                {
                }
                else
                {
                    nextWpntNum = sithAIUtil_SelectWpntLink(pLocal, sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks, aLinks, i, ownerIdx);
                    thingNum = sithAIUtil_aAIWpnts[nextWpntNum].thingNum;
                    if ( sithAIMove_AISetMovePos(pLocal, &pCurLevel->aThings[thingNum].pos, pLocal->moveSpeed) )
                    {
                        sithAIMove_AISetLookPosEyeLevel(pLocal, &pCurLevel->aThings[thingNum].pos);
                        sithAIUtil_CheckWpntMoveAlign(pLocal);

                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum |= 0xFF;
                        if ( wpntIdx != -1 )
                        {
                            sithAIUtil_aWpntOwners[ownerIdx].waypointNum = (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & ~0xFF) | wpntIdx;
                        }

                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum |= 0xFF00;
                        sithAIUtil_aWpntOwners[ownerIdx].waypointNum = (sithAIUtil_aWpntOwners[ownerIdx].waypointNum & ~0xFF00) | (nextWpntNum << 8);
                        if ( bNotify )
                        {
                            sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVEDWPNT, wpntIdx, nextWpntNum, 0, 0);
                        }

                        return 1;
                    }
                }
            }
        }

        sithCog_ThingSendMessageEx(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVEDWPNT, wpntIdx, -1, 0, 0);
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVED);
    }

    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    return 0;
}

 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIUtil_countfit_4, 0) /* end count-fit */
void J3DAPI sithAIUtil_CheckWpntMoveAlign(SithAIControlBlock* pLocal)
{
    int wpntNumber;
    rdVector3 dir;
    float dist;
    float dot;

    wpntNumber = sithAIUtil_GetAIWaypointNum(pLocal);
    if ( wpntNumber > -1 )
    {
        dist = rdMath_DistancePointToPlane(&pLocal->goalLVec, &pLocal->pOwner->orient.uvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&dir, &pLocal->pOwner->orient.uvec, -dist, &pLocal->goalLVec);
        rdVector_Normalize3Acc(&dir);

        if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
        {
            dot = RD_MULTIPLY(pLocal->pOwner->orient.lvec.x, dir.x) + RD_MULTIPLY(pLocal->pOwner->orient.lvec.y, dir.y) + RD_MULTIPLY(pLocal->pOwner->orient.lvec.z, dir.z);
        }
        else
        {
            dot = RD_MULTIPLY(pLocal->pOwner->orient.lvec.x, dir.x) + RD_MULTIPLY(pLocal->pOwner->orient.lvec.y, dir.y);
        }

        if ( dot >= sithAIUtil_aWpntOwners[wpntNumber].cosTurnAlignAngle || (pLocal->mode & SITHAI_MODE_TURNING) == 0 )
        {
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
        }
        else
        {
            memset(&pLocal->pOwner->moveInfo.physics.velocity, 0, sizeof(rdVector3));
        }
    }
    else
    {
        sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    }
}

int J3DAPI sithAIUtil_AIFindNearestWpnt(SithThing* pThing, int a2)
{
    int idWpnt;
    int found;
    int i;
    unsigned int linkId;
    int numDist;
    SithWorld* pWorld;
    int idxThing;
    unsigned int nLinks;
    float pathDist;
    rdVector3 hitNorm;
    int aLinks[9];

    pWorld       = sithWorld_pCurrentWorld;
    idWpnt      = 0;
    linkId            = 0;
    numDist = 0;
    found       = 0;
    if ( pThing != sithPlayer_pLocalPlayerThing )
    {
        sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_UPDATEWPNTS, 0, 0, 0, 0);
    }

    numDist = 0;
    for ( i = 0; i < 60; ++i )
    {
        if ( (sithAIUtil_aAIWpnts[i].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[i].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        idxThing = sithAIUtil_aAIWpnts[i].thingNum;
        if ( idxThing == -1 )
        {
            continue;
        }

        if ( idxThing > pWorld->lastThingIdx || pWorld->aThings[idxThing].type != SITH_THING_GHOST )
        {
#line 1603
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIFindNearestWpnt: Invalid waypoint (#%d).\n", i, 0, 0, 0);
            continue;
        }

        found = 1;
        sithAIUtil_aWpntDistances[numDist].num = i;
        sithAIUtil_aWpntDistances[numDist].distance = RD_MULTIPLY(pThing->pos.x - pWorld->aThings[idxThing].pos.x, pThing->pos.x - pWorld->aThings[idxThing].pos.x) + RD_MULTIPLY(pThing->pos.y - pWorld->aThings[idxThing].pos.y, pThing->pos.y - pWorld->aThings[idxThing].pos.y) + RD_MULTIPLY(pThing->pos.z - pWorld->aThings[idxThing].pos.z, pThing->pos.z - pWorld->aThings[idxThing].pos.z);
        if ( sithAIUtil_aWpntDistances[numDist].distance <= RD_MULTIPLY(pThing->collide.movesize + 0.03f, pThing->collide.movesize + 0.03f) )
        {
            return i;
        }

        ++numDist;
    }

    if ( !found )
    {
        return -1;
    }

    qsort(sithAIUtil_aWpntDistances, numDist, sizeof(SithAIWaypointDistance), sithAIUtil_CompareAIWpntDistances);
    idWpnt = sithAIUtil_aWpntDistances[0].num;
    if ( a2 )
    {
        found = 0;
        for ( i = 0; i < numDist; ++i )
        {
            idWpnt  = sithAIUtil_aWpntDistances[i].num;
            idxThing = sithAIUtil_aAIWpnts[idWpnt].thingNum;
            if ( !sithAIUtil_CheckPathToPoint(pThing, &pWorld->aThings[idxThing].pos, pThing->collide.movesize, &pathDist, &hitNorm, 1, 0) )
            {
                found = 1;
                break;
            }
        }

        if ( !found )
        {
            return -1;
        }
    }

    if ( pThing == sithPlayer_pLocalPlayerThing )
    {
        return idWpnt;
    }

    aLinks[0] = idWpnt;
    nLinks  = 1;
    for ( ++i; i < numDist; ++i )
    {
        for ( linkId = 0; linkId < sithAIUtil_aAIWpnts[idWpnt].numUsedLinks; ++linkId )
        {
            if ( sithAIUtil_aAIWpnts[idWpnt].aWpntLinks[linkId] == sithAIUtil_aWpntDistances[i].num )
            {
                idxThing = sithAIUtil_aAIWpnts[sithAIUtil_aWpntDistances[i].num].thingNum;
                if ( !sithAIUtil_CheckPathToPoint(pThing, &pWorld->aThings[idxThing].pos, pThing->collide.movesize, &pathDist, &hitNorm, 1, 0) )
                {
                    aLinks[nLinks++] = sithAIUtil_aWpntDistances[i].num;
                    break;
                }
            }
        }

        if ( nLinks >= sithAIUtil_aAIWpnts[idWpnt].numUsedLinks + 1 )
        {
            break;
        }
    }

    idWpnt = sithAIUtil_SelectWpntLink(pThing->controlInfo.aiControl.pLocal, nLinks, aLinks, 0, -1);
    return idWpnt;
}


void J3DAPI sithAIUtil_RankWpntsFromPlayer(void)
{
    int wpntNum;
    int top;
    unsigned int idx;
    unsigned int rankNum;
    int aStack[60];
    int curWpnt;
    int linkCount;
    SithWorld* pWorld;

    pWorld = sithWorld_pCurrentWorld;
    if ( !sithPlayer_pLocalPlayerThing )
    {
        return;
    }

    if ( (sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
    {
        return;
    }

    for ( idx = 0; idx < 60; ++idx )
    {
        sithAIUtil_aAIWpnts[idx].rank = 0;
    }

    wpntNum = sithAIUtil_AIFindNearestWpnt(sithPlayer_pLocalPlayerThing, 0);
    if ( wpntNum == -1 )
    {
        return;
    }

    rankNum = 255;
    top = 0;
    aStack[0] = wpntNum;
    sithAIUtil_aAIWpnts[wpntNum].rank = rankNum;
    while ( top >= 0 )
    {
        curWpnt = aStack[top--];
        rankNum = sithAIUtil_aAIWpnts[curWpnt].rank - 1;
        for ( idx = 0; idx < sithAIUtil_aAIWpnts[curWpnt].numUsedLinks; ++idx )
        {
            linkCount = sithAIUtil_aAIWpnts[curWpnt].aWpntLinks[idx];
            if ( (sithAIUtil_aAIWpnts[linkCount].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[linkCount].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            if ( sithAIUtil_aAIWpnts[linkCount].rank < rankNum )
            {
                sithAIUtil_aAIWpnts[linkCount].rank = rankNum;
                if ( sithAIUtil_aAIWpnts[linkCount].numUsedLinks > 1 )
                {
                    aStack[++top] = linkCount;
                }
            }
        }
    }
}


void J3DAPI sithAIUtil_RankWpntsFromThing(SithThing* pThing)
{
    int wpntNum;
    int top;
    unsigned int idx;
    unsigned int rankNum;
    int aStack[60];
    int curWpnt;
    int linkCount;
    int bFound;
    SithWorld* pWorld;

    pWorld = sithWorld_pCurrentWorld;
    bFound = 0;
    for ( idx = 0; idx < 60; ++idx )
    {
        sithAIUtil_aAIWpnts[idx].rank = 0;
    }

    for ( idx = 0; idx < 60; ++idx )
    {
        if ( sithAIUtil_aAIWpnts[idx].thingNum == pThing->idx )
        {
            bFound = 1;
            wpntNum = idx;
            break;
        }
    }

    if ( !bFound )
    {
        return;
    }

    rankNum = 255;
    top = 0;
    aStack[0] = wpntNum;
    sithAIUtil_aAIWpnts[wpntNum].rank = rankNum;
    while ( top >= 0 )
    {
        curWpnt = aStack[top--];
        rankNum = sithAIUtil_aAIWpnts[curWpnt].rank - 1;
        for ( idx = 0; idx < sithAIUtil_aAIWpnts[curWpnt].numUsedLinks; ++idx )
        {
            linkCount = sithAIUtil_aAIWpnts[curWpnt].aWpntLinks[idx];
            if ( (sithAIUtil_aAIWpnts[linkCount].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[linkCount].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            if ( sithAIUtil_aAIWpnts[linkCount].rank < rankNum )
            {
                sithAIUtil_aAIWpnts[linkCount].rank = rankNum;
                if ( sithAIUtil_aAIWpnts[linkCount].numUsedLinks > 1 )
                {
                    aStack[++top] = linkCount;
                }
            }
        }
    }
}


int J3DAPI sithAIUtil_CreateAIWaypoint(SithAIControlBlock* pLocal, float degTurn)
{
    int ownerNum;
    int wpntNum;
    float sinAngle;
    float cosAngle;

    wpntNum = -1;
    for ( ownerNum = 0; ownerNum < 10; ++ownerNum )
    {
        if ( sithAIUtil_aWpntOwners[ownerNum].thingNum == -1 && wpntNum == -1 )
        {
            wpntNum = ownerNum;
        }

        if ( sithAIUtil_aWpntOwners[ownerNum].thingNum == pLocal->pOwner->idx )
        {
            wpntNum = ownerNum;
            break;
        }
    }

    if ( wpntNum > -1 )
    {
        degTurn = STDMATH_CLAMP(degTurn, 5.0f, 180.0f);
        stdMath_SinCos(degTurn, &sinAngle, &cosAngle);
        sithAIUtil_aWpntOwners[wpntNum].thingNum = pLocal->pOwner->idx;
        sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle = cosAngle;
    }

    return wpntNum;
}


void J3DAPI sithAIUtil_ClearAIWaypoint(SithAIControlBlock* pLocal)
{
    int wpntNum;

    wpntNum = sithAIUtil_GetAIWaypointNum(pLocal);
    if ( wpntNum > -1 )
    {
        sithAIUtil_aWpntOwners[wpntNum].thingNum = -1;
        sithAIUtil_aWpntOwners[wpntNum].waypointNum = 0;
        sithAIUtil_aWpntOwners[wpntNum].cosTurnAlignAngle = 0.0f;
    }
}

int J3DAPI sithAIUtil_GetAIWaypointNum(SithAIControlBlock* pLocal)
{
    int i;

    for ( i = 0; i < 10; ++i )
    {
        if ( sithAIUtil_aWpntOwners[i].thingNum == pLocal->pOwner->idx )
        {
            break;
        }
    }

    if ( i < 10 )
    {
        return i;
    }

    return -1;
}

int J3DAPI sithAIUtil_CompareAIWpntDistances(const void* pElem1, const void* pElem2)
{
    const SithAIWaypointDistance* pDist1;
    const SithAIWaypointDistance* pDist2;

    pDist1 = (const SithAIWaypointDistance*)pElem1;
    pDist2 = (const SithAIWaypointDistance*)pElem2;
    if ( pDist1 && pDist2 )
    {
        if ( pDist1->distance < pDist2->distance )
        {
            return -1;
        }

        if ( pDist1->distance > pDist2->distance )
        {
            return 1;
        }
    }

    return 0;
}

/* Debug-only console command "wpdraw" (debug 0x004CBA5F, between CompareAIWpntDistances and RenderAIWaypoints;
   registered by the debug sithCommand.c at 0x004E61EA with the name "wpdraw", a registration and string retail has
   not got), restored in full and /Od-identical; retail's linker discarded it. Name:
   sithCommand_WpDraw with this TU's prefix. */
int J3DAPI sithAIUtil_WpDraw(const SithConsoleCommand* pFunc, const char* pArg)
{
    if ( sithAIUtil_bRenderAIWpnts )
    {
        sithAIUtil_bRenderAIWpnts = 0;
    }
    else
    {
        sithAIUtil_bRenderAIWpnts = 1;
    }

    return 1;
}

void J3DAPI sithAIUtil_RenderAIWaypoints(void)
{
    SithWorld* pWorld;
    unsigned int wpntIdx;
    unsigned int j;
    int thingNum;
    int linkGhostIdx;
    rdVector3 laserEnd;
    rdVector3 offset;
    int toWpntIdx;

    pWorld = sithWorld_pCurrentWorld;
    if ( !sithAIUtil_bRenderAIWpnts )
    {
        sithAIUtil_secLastUpdate = 0.0f;
        return;
    }

    if ( sithTime_secGameTime - sithAIUtil_secLastUpdate < sithAIUtil_flt_539F5C )
    {
        return;
    }

    for ( wpntIdx = 0; wpntIdx < 60; ++wpntIdx )
    {
        thingNum = sithAIUtil_aAIWpnts[wpntIdx].thingNum;
        if ( (sithAIUtil_aAIWpnts[wpntIdx].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[wpntIdx].flags & sithAIUtil_activeWpntLayer) == 0 )
        {
            continue;
        }

        for ( j = 0; j < sithAIUtil_aAIWpnts[wpntIdx].numUsedLinks; ++j )
        {
            toWpntIdx = sithAIUtil_aAIWpnts[wpntIdx].aWpntLinks[j];
            if ( toWpntIdx < wpntIdx )
            {
                continue;
            }

            if ( (sithAIUtil_aAIWpnts[toWpntIdx].flags & SITH_AIWPNT_DISABLED) != 0 || (sithAIUtil_aAIWpnts[toWpntIdx].flags & sithAIUtil_activeWpntLayer) == 0 )
            {
                continue;
            }

            linkGhostIdx = sithAIUtil_aAIWpnts[toWpntIdx].thingNum;
            offset.x = 0.0f;
            offset.y = 0.0f;
            offset.z = 0.1f;
            rdVector_Copy3(&laserEnd, &pWorld->aThings[linkGhostIdx].pos);
            laserEnd.z += 0.1f;
            sithFX_CreateLaserThing(&pWorld->aThings[thingNum], &offset, &laserEnd, 0.05f, 0.05f, sithAIUtil_flt_539F5C + 0.1f);
            sithAIUtil_secLastUpdate = sithTime_secGameTime;
        }
    }
}


int J3DAPI sithAIUtil_CheckPathToPoint(SithThing* pViewer, const rdVector3* pTarget, float radius, float* pDistance, rdVector3* pHitNorm, int bDetectThings, int bSkipFloor)
{
    int bBlocked;
    rdVector3 curdir;
    float distance;
    SithCollision* pCurCollision;
    int flags;

    bBlocked = 0;
#line 2060
    SITH_ASSERTREL(pViewer && pTarget);

    rdVector_Sub3(&curdir, pTarget, &pViewer->pos);
    distance = rdVector_Normalize3Acc(&curdir);
    *pDistance = distance;

    flags = 0x102;
    if ( !bDetectThings )
    {
        flags |= 0x1;
    }

    sithCollision_SearchForCollisions(pViewer->pInSector, pViewer, &pViewer->pos, &curdir, distance, radius, flags);
    while ( (pCurCollision = sithCollision_PopStack()) != NULL )
    {
        if ( !bSkipFloor && (pCurCollision->type & SITHCOLLISION_WORLD) != 0 && (pCurCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            continue;
        }

        if ( (pCurCollision->type & (SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)) != 0 && (pCurCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            continue;
        }

        bBlocked = 1;
        *pDistance = pCurCollision->distance;
        rdVector_Copy3(pHitNorm, &pCurCollision->hitNorm);
        break;
    }

    sithCollision_DecreaseStackLevel();
    return bBlocked;
}

int J3DAPI sithAIUtil_CanWalk(const SithAIControlBlock* pLocal, rdVector3* endPos, int* pbNoCollision)
{
    SithThing* pThing;
    SithSector* pSector;

    pThing = pLocal->pOwner;
#line 2133
    SITH_ASSERTREL(pThing);

    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, endPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
    {
        return 0;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
    {
        return 0;
    }

    return sithAIUtil_CanWalk_ExplicitSector(pLocal, endPos, pSector, pbNoCollision);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIUtil_countfit_5, 7) /* end count-fit */
int J3DAPI sithAIUtil_CanWalk_ExplicitSector(const SithAIControlBlock* pLocal, const rdVector3* startPos, SithSector* pStartSector, int* pbNoCollision)
{
    SithThing* pThing;
    SithCollision* pCollision;
    rdVector3 downDir;
    float radius;
    int result;

    result = 8;
    pThing = pLocal->pOwner;
    radius = 0.001f;
#line 2167
    SITH_ASSERTREL(pThing);

    if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
    {
        rdVector_Neg3(&downDir, &pThing->orient.uvec);
    }
    else if ( RD_MULTIPLY(pThing->orient.uvec.x, rdroid_zVector3.x) + RD_MULTIPLY(pThing->orient.uvec.y, rdroid_zVector3.y) + RD_MULTIPLY(pThing->orient.uvec.z, rdroid_zVector3.z) > -0.707f )
    {
        rdVector_Neg3(&downDir, &rdroid_zVector3);
    }
    else
    {
        rdVector_Copy3(&downDir, &rdroid_zVector3);
    }

    sithCollision_SearchForCollisions(pStartSector, pThing, startPos, &downDir, sithPhysics_GetThingHeight(pThing) + pLocal->pClass->maxStep, radius, 0);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        float dot;

        if ( (pCollision->type & (SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
            {
                result = 0;
                break;
            }

            if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 && (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
            {
                result = 0;
                break;
            }
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            SithSurface* pAttachedSurface;
            SithSurface* pSurface;

#line 2209
            SITH_ASSERTREL(pCollision->pSurfaceCollided);
            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_NOAIMOVE) != 0 )
            {
                result = 0;
                break;
            }

            if ( pLocal->allowedSurfaceTypes )
            {
                if ( (pCollision->pSurfaceCollided->flags & pLocal->allowedSurfaceTypes) == 0 )
                {
                    result = 0;
                    break;
                }
            }
            else if ( (pCollision->pSurfaceCollided->flags & 0x200000) != 0 )
            {
                result = 0;
                break;
            }

            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                result = 1;
                if ( (pThing->moveInfo.physics.flags & SITH_PF_ALIGNUP) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                {
                    pAttachedSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
                    pSurface = pCollision->pSurfaceCollided;
                    if ( pAttachedSurface != pSurface )
                    {
                        dot = rdVector_Dot3(&pSurface->face.normal, &rdroid_zVector3);
                        if ( dot < 0.788f )
                        {
                            result = 2;
                        }
                    }
                }
            }
            else
            {
                result = 2;
            }

            if ( pbNoCollision )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) == 0 || pThing->attach.attachedToStructure.pSurfaceAttached != pCollision->pSurfaceCollided )
                {
                    *pbNoCollision = 1;
                }
                else
                {
                    *pbNoCollision = 0;
                }
            }

            break;
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
#line 2277
            SITH_ASSERTREL(pCollision->pThingCollided);
            if ( (pCollision->pThingCollided->attach.flags & SITH_ATTACH_THING) != 0 && pCollision->pThingCollided->attach.attachedToStructure.pThingAttached == pThing )
            {
                result = 1;
            }
            else if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                result = 1;
            }
            else if ( (pCollision->pThingCollided->flags & SITH_TF_STANDON) == 0 && (pCollision->pThingCollided->type & 0xA) == 0 )
            {
                result = 4;
                break;
            }

            result = 1;
            if ( pbNoCollision )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) == 0 || pThing->attach.attachedToStructure.pThingAttached != pCollision->pThingCollided )
                {
                    *pbNoCollision = 1;
                }
                else
                {
                    *pbNoCollision = 0;
                }
            }

            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return result;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIUtil_countfit_15, 131) /* end count-fit */
int J3DAPI sithAIUtil_MakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, int flags, float angle, float* pDownCos, rdVector3* pDir, rdVector3* pDestPoint)
{
    rdVector3 newDir;
    int bValid;
    SithThing* pActor;
    float pitch;
    float yaw;
    int isInFront;
    rdVector3 rotPyr;
    rdVector3 toMovePos;

    bValid = 0;
#line 2356
    SITH_ASSERTREL(pLocal && pLocal->pClass && pLocal->pOwner);
    pActor = pLocal->pOwner;

    if ( !pPos || !pDir || !pDestPoint )
    {
#line 2361
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithAIUtil_MakeRandPoint: Bad parameters.\n", 0, 0, 0, 0);
        return 0;
    }

    sithAIUtil_mkPoinCurFlags = 0;
    sithAIUtil_pMkPointCurLocal = NULL;

    if ( (flags & 4) == 0 )
    {
        pDir->z = 0.0f;
        rdVector_Normalize3Acc(pDir);
    }

    if ( (flags & 6) != 0 )
    {
        pitch = (flags & 4) != 0 ? angle : 0.0f;
        yaw = (flags & 2) != 0 ? angle : 0.0f;

        if ( (flags & 0x4000) != 0 )
        {
            if ( pitch != 0.0f )
            {
                pitch = RD_MULTIPLY(D3DVAL(SITH_RAND()), pitch);
            }

            if ( yaw != 0.0f )
            {
                yaw = RD_MULTIPLY(D3DVAL(SITH_RAND()), yaw);
            }
        }

        rotPyr.x = pitch;
        rotPyr.y = yaw;
        rotPyr.z = 0.0f;
        if ( (flags & 0x8000) != 0 && SITH_RAND() > 0.5f )
        {
            rdVector_Neg3Acc(&rotPyr);
        }

        rdVector_Rotate3(&newDir, pDir, &rotPyr);
        bValid = 1;
    }
    else if ( (flags & 1) != 0 )
    {
        rdVector_Cross3(&newDir, &pActor->orient.uvec, pDir);

        if ( (flags & 0x800) != 0 )
        {
            if ( rdVector_Dot3(&pLocal->moveDirection, &newDir) < 0.0f )
            {
                rdVector_Neg3Acc(&newDir);
            }
        }
        else
        {
            if ( (flags & 0x8000) != 0 && SITH_RAND() > 0.5f )
            {
                rdVector_Neg3Acc(&newDir);
            }

            if ( (flags & 0x4000) != 0 )
            {
                rotPyr.x = 0.0f;
                rotPyr.y = RD_MULTIPLY(D3DVAL(SITH_RAND()), angle);
                rotPyr.z = 0.0f;
                if ( SITH_RAND() > 0.5f )
                {
                    rdVector_Neg3Acc(&rotPyr);
                }

                rdVector_Rotate3Acc(&newDir, &rotPyr);
            }
        }

        bValid = 1;
    }

    if ( bValid )
    {
        if ( (flags & 0x101700) != 0 )
        {
            rdVector3 movePos;
            sithAIUtil_AIGetMovePos(pLocal, flags, &movePos);
            rdVector_Sub3(&toMovePos, &movePos, &pActor->pos);
            rdVector_Normalize3Acc(&toMovePos);

            isInFront = rdVector_Dot3(&newDir, &toMovePos) > 0.0f;
            if ( ((flags & 0x100500) != 0 && !isInFront) || ((flags & 0x1200) != 0 && isInFront) )
            {
                if ( (flags & 2) != 0 )
                {
                    rdVector_Neg3Acc(&rotPyr);
                    rdVector_Rotate3(&newDir, pDir, &rotPyr);
                }
                else
                {
                    rdVector_Neg3Acc(&newDir);
                }
            }

            if ( (flags & 0x10000) != 0 )
            {
                *pDownCos = -rdMath_DistancePointToPlane(pPos, &newDir, &movePos);
            }
        }

        if ( stdMath_ClipNearZero(*pDownCos) != 0.0f )
        {
            sithAIUtil_mkPoinCurFlags = flags;
            sithAIUtil_pMkPointCurLocal = pLocal;
            rdVector_Copy3(&sithAIUtil_mkPointCurDir, pDir);
            if ( (flags & 2) != 0 )
            {
                rdVector_Copy3(&sithAIUtil_mkPointCurPYR, &rotPyr);
            }
            else
            {
                rdVector_Copy3(&sithAIUtil_mkPointCurRVec, &newDir);
            }

            rdVector_ScaleAdd3(pDestPoint, &newDir, *pDownCos, pPos);
            rdVector_Copy3(pDir, &newDir);
            *pDownCos = sithAIUtil_Abs(*pDownCos);
        }
        else
        {
            bValid = 0;
        }
    }

    return bValid;
}

int J3DAPI sithAIUtil_RetryMakeRandPoint(SithAIControlBlock* pLocal, rdVector3* pPos, float a3, rdVector3* pDirection, rdVector3* pDestPoint)
{
    rdVector3 dir;

    if ( !sithAIUtil_mkPoinCurFlags || !sithAIUtil_pMkPointCurLocal || !pLocal )
    {
        return 0;
    }

    if ( pLocal != sithAIUtil_pMkPointCurLocal || !pPos || !pDirection || !pDestPoint )
    {
#line 2532
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithAIUtil_RetryMakeRandPoint: Bad parameters.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( (sithAIUtil_mkPoinCurFlags & 0x10400) != 0 )
    {
        sithAIUtil_mkPoinCurFlags = 0;
        sithAIUtil_pMkPointCurLocal = NULL;
        return 0;
    }

    if ( (sithAIUtil_mkPoinCurFlags & 6) != 0 )
    {
        rdVector_Neg3Acc(&sithAIUtil_mkPointCurPYR);
        rdVector_Rotate3(&dir, &sithAIUtil_mkPointCurDir, &sithAIUtil_mkPointCurPYR);
        sithAIUtil_mkPoinCurFlags &= ~6;
    }
    else if ( (sithAIUtil_mkPoinCurFlags & 1) != 0 )
    {
        rdVector_Neg3(&dir, &sithAIUtil_mkPointCurRVec);
        sithAIUtil_mkPoinCurFlags &= ~1;
    }
    else
    {
        sithAIUtil_mkPoinCurFlags = 0;
        sithAIUtil_pMkPointCurLocal = NULL;
        return 0;
    }

    rdVector_ScaleAdd3(pDestPoint, &dir, a3, pPos);
    rdVector_Copy3(pDirection, &dir);
    return 1;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(sithAIUtil_countfit_6, 30, 8) CF_PROTO(sithAIUtil_countfit_14, 12) /* end count-fit */

int J3DAPI sithAIUtil_CheckPathToPos(SithAIControlBlock* pLocal, SithSector* pStartSector, rdVector3* startPos, int flags, float minMoveDist, float moveDist, float radius, rdVector3* pDirection, rdVector3* a9)
{
    SithThing* pOwner;
    SithCollision* pCollision;
    SithSector* pSector;

#line 2606
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    pOwner = pLocal->pOwner;

    if ( !pDirection )
    {
#line 2611
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithAIUtil_CheckPathToPos: Bad direction vector.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( radius == 0.0f )
    {
        radius = pOwner->collide.movesize;
    }

    if ( (flags & 0x10) != 0 )
    {
        int result;
        rdVector3 endPos;

        result = 1;
        rdVector_ScaleAdd3(&endPos, pDirection, moveDist, startPos);

        sithCollision_SearchForCollisions(pStartSector, pOwner, startPos, pDirection, moveDist, radius, 0x102);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (flags & 0x20000) != 0 && (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided->type == SITH_THING_PLAYER )
            {
                continue;
            }

            if ( (flags & 0x40000) != 0 && (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                continue;
            }

            if ( (flags & 0x80000) != 0 && (pCollision->type & SITHCOLLISION_WORLD) != 0 && (pCollision->pSurfaceCollided->flags & 1) != 0 )
            {
                continue;
            }

            if ( pCollision->distance >= minMoveDist )
            {
                rdVector_ScaleAdd3(&endPos, pDirection, pCollision->distance, startPos);
            }
            else
            {
                result = 0;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                result = 4;
                break;
            }

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                result = 2;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( result )
        {
            SithThing* pTarget;
            float targetDist;
            rdVector3 toTarget;

            pTarget = NULL;
            if ( (flags & 0x2000) != 0 && pLocal->goalThing )
            {
                pTarget = pLocal->goalThing;
            }

            if ( (flags & 0x800000) != 0 && pLocal->pTargetThing )
            {
                pTarget = pLocal->pTargetThing;
            }

            if ( pTarget && sithAIUtil_GetDistanceToTargetPos(pTarget, &pTarget->pos, &endPos, -1.0f, pLocal->pClass->sightDistance, 0.0f, &toTarget, &targetDist) == 3 )
            {
                return 0;
            }

            if ( a9 )
            {
                rdVector_Copy3(a9, &endPos);
            }
        }

        return result;
    }

    if ( (flags & 0x20) != 0 )
    {
        int side;
        int bSideDone;
        int i;
        int bVisible;
        float step;
        rdVector3 perp;
        float distance;
        int aScores[2];
        int aResults[2];
        rdVector3 aEndPos[2];

        aScores[0] = 0;
        aScores[1] = 0;
        aResults[0] = 0;
        aResults[1] = 0;
        side = 1;
        bSideDone = 0;
        rdVector_Cross3(&perp, &pLocal->pOwner->orient.uvec, pDirection);
        perp.x = stdMath_ClipNearZero(perp.x);
        perp.y = stdMath_ClipNearZero(perp.y);
        perp.z = stdMath_ClipNearZero(perp.z);
        if ( rdVector_Normalize3Acc(&perp) == 0.0f )
        {
            rdVector_Cross3(&perp, pDirection, &pLocal->pOwner->orient.rvec);
            perp.x = stdMath_ClipNearZero(perp.x);
            perp.y = stdMath_ClipNearZero(perp.y);
            perp.z = stdMath_ClipNearZero(perp.z);
            rdVector_Normalize3Acc(&perp);
        }

        step = 0.03f;
        distance = minMoveDist;
        while ( side >= 0 )
        {
            if ( bSideDone )
            {
                if ( --side < 0 )
                {
                    break;
                }

                bSideDone = 0;
                distance = minMoveDist;
                rdVector_Neg3Acc(&perp);
            }

            distance += step;
            if ( distance > moveDist )
            {
                bSideDone = 1;
                continue;
            }

            rdVector_ScaleAdd3(&aEndPos[side], &perp, distance, startPos);
            pSector = sithCollision_FindSectorInRadius(pStartSector, startPos, &aEndPos[side], radius);
            if ( !pSector )
            {
                bSideDone = 1;
                continue;
            }

            sithCollision_SearchForCollisions(pStartSector, pOwner, startPos, &perp, distance, radius, 0x102);
            pCollision = sithCollision_PopStack();
            sithCollision_DecreaseStackLevel();
            if ( pCollision )
            {
                rdVector_ScaleAdd3(&aEndPos[side], &perp, pCollision->distance, startPos);
                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                {
                    aResults[side] = 4;
                }
                else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                {
                    aResults[side] = 2;
                }

                bSideDone = 1;
            }
            else
            {
                aResults[side] = sithAIUtil_CheckPathToPos(pLocal, pSector, &aEndPos[side], 0x80010, pOwner->collide.movesize + 0.03f, radius, 0.0f, pDirection, NULL);
                if ( aResults[side] == 1 )
                {
                    bSideDone = 1;
                }
            }
        }

        if ( pLocal->goalThing )
        {
            float goalDist;

            for ( i = 0; i < 2; ++i )
            {
                bVisible = sithAIUtil_GetDistanceToTargetPos(pLocal->goalThing, &pLocal->goalThing->pos, &aEndPos[i], -1.0f, pLocal->pClass->sightDistance, 0.0f, &pLocal->vecUnknown0, &pLocal->targetDistance) != 3;
                if ( bVisible )
                {
                    ++aScores[i];
                }
                else if ( (flags & 0x2000) != 0 )
                {
                    --aScores[i];
                }
            }

            goalDist = sqrtf(RD_MULTIPLY(aEndPos[0].x - pLocal->goalThing->pos.x, aEndPos[0].x - pLocal->goalThing->pos.x) + RD_MULTIPLY(aEndPos[0].y - pLocal->goalThing->pos.y, aEndPos[0].y - pLocal->goalThing->pos.y));
            if ( goalDist < sqrtf(RD_MULTIPLY(aEndPos[1].x - pLocal->goalThing->pos.x, aEndPos[1].x - pLocal->goalThing->pos.x) + RD_MULTIPLY(aEndPos[1].y - pLocal->goalThing->pos.y, aEndPos[1].y - pLocal->goalThing->pos.y)) )
            {
                if ( (flags & 0x100) != 0 )
                {
                    ++aScores[0];
                }
                else if ( (flags & 0x200) != 0 )
                {
                    --aScores[0];
                }
            }
            else
            {
                if ( (flags & 0x100) != 0 )
                {
                    ++aScores[1];
                }
                else if ( (flags & 0x200) != 0 )
                {
                    --aScores[1];
                }
            }
        }

        i = aScores[0] < aScores[1];
        if ( a9 )
        {
            rdVector_Copy3(a9, &aEndPos[i]);
        }

        return aResults[i];
    }

    return 0;
}


int J3DAPI sithAIUtil_SweepMoveToPathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float angle, float maxAngle, float deltaAngle, int flags)
{
    int bFound;
    int randFlags;
    float pathDist;
    float curAngle;
    float hitDist;
    rdVector3 toTargetDir;
    rdVector3 surfaceNorm;
    rdVector3 randDir;
    rdVector3 destPos;

    rdVector_Sub3(&toTargetDir, pPos, &pLocal->pOwner->pos);
    toTargetDir.z = 0.0f;
    pathDist = rdVector_Normalize3Acc(&toTargetDir);

    flags &= 0x0FFFFF00;
    randFlags = flags | 0x8002;
    curAngle = angle;
    while ( curAngle <= maxAngle )
    {
        rdVector_Copy3(&randDir, &toTargetDir);
        if ( !sithAIUtil_MakeRandPoint(pLocal, &pLocal->pOwner->pos, randFlags, curAngle, &pathDist, &randDir, &destPos) )
        {
            return 0;
        }

        while ( (bFound = sithAIUtil_CheckPathToPoint(pLocal->pOwner, &destPos, pLocal->pOwner->collide.movesize, &hitDist, &surfaceNorm, 1, 0)) == 0
            && curAngle != 0.0f
            && curAngle != 180.0f
            && sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, pathDist, &toTargetDir, &destPos) )
        {
        }

        if ( bFound )
        {
            sithAIMove_StopAIMovement(pLocal);
            sithAIMove_AISetLookPosEyeLevel(pLocal, &destPos);
            sithAIMove_AISetMoveTargetPos(pLocal, &destPos, pLocal->moveSpeed);
            return 1;
        }

        curAngle += deltaAngle;
    }

    return 0;
}

int J3DAPI sithAIUtil_MakeRoamPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, int bSomeBool, int maxRetries, rdVector3* pDestPos)
{
    int curresult;
    size_t numRetries;
    int currentflags;
    rdVector3 dir;

    curresult     = 0;
    numRetries = 0;
    rdVector_Copy3(&dir, &pLocal->pOwner->orient.lvec);

retry:
    currentflags = 0xC002;
    if ( bSomeBool )
    {
        currentflags |= 4;
    }

    if ( !sithAIUtil_MakeRandPoint(pLocal, pPos, currentflags, 360.0f, &moveDist, &dir, pDestPos) )
    {
        return 0;
    }

check:
    if ( bSomeBool )
    {
        currentflags = 0x10;
    }
    else
    {
        currentflags = 0x80010;
    }

    curresult = sithAIUtil_CheckPathToPos(pLocal, pLocal->pOwner->pInSector, &pLocal->pOwner->pos, currentflags, minDist, moveDist, 0.0f, &dir, pDestPos);
    if ( !curresult )
    {
        if ( sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, moveDist, &dir, pDestPos) )
        {
            goto check;
        }

        if ( ++numRetries < maxRetries )
        {
            goto retry;
        }
    }

    if ( curresult )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithAIUtil_MakePathPos(SithAIControlBlock* pLocal, rdVector3* pPos, float minDist, float moveDist, float angle, int flags, rdVector3* pDirection, rdVector3* pDestPos)
{
    int checkRes;
    int pathFlags;

    checkRes = 0;
    if ( !sithAIUtil_MakeRandPoint(pLocal, pPos, flags, angle, &moveDist, pDirection, pDestPos) )
    {
        return 0;
    }

    pathFlags = flags & 0x8E2000;
    if ( (flags & 4) != 0 )
    {
        pathFlags |= 0x10;
    }
    else
    {
        pathFlags |= 0x80010;
    }

    while ( (checkRes = sithAIUtil_CheckPathToPos(pLocal, pLocal->pOwner->pInSector, &pLocal->pOwner->pos, pathFlags, minDist, moveDist, 0.0f, pDirection, pDestPos)) == 0 )
    {
        if ( sithAIUtil_RetryMakeRandPoint(pLocal, &pLocal->pOwner->pos, moveDist, pDirection, pDestPos) )
        {
            continue;
        }

        return 0;
    }

    return 1;
}


int J3DAPI sithAIUtil_IsLineOfFireClear(const SithAIControlBlock* pLocal, const rdVector3* pPos)
{
    rdVector3 curdir;
    SithAIControlBlock* cur;
    float dist;
    float dot;
    int idx;

    if ( !pLocal || !pLocal->pOwner || pLocal->pOwner->controlType != SITH_CT_AI )
    {
        return 1;
    }

    if ( !sithWeapon_HasWeaponSelected(pLocal->pOwner) )
    {
        return 1;
    }

    if ( !pLocal->pTargetThing )
    {
        return 1;
    }

    rdVector_Sub3(&curdir, &pLocal->pTargetThing->pos, pPos);
    dist = rdVector_Normalize3Acc(&curdir);

    for ( idx = 0, cur = sithAI_aControlBlocks; idx <= sithAI_lastUsedAIIndex; ++idx, ++cur )
    {
        if ( !cur || !cur->pOwner || cur->pOwner->controlType != SITH_CT_AI )
        {
            continue;
        }

        if ( (cur->pOwner->thingInfo.actorInfo.flags & SITH_AF_HUMAN) == 0 )
        {
            continue;
        }

        if ( (cur->mode & SITHAI_MODE_TARGETVISIBLE) == 0 )
        {
            continue;
        }

        if ( cur == pLocal )
        {
            continue;
        }

        dot = rdVector_Dot3(&cur->toTarget, &curdir);
        if ( dot >= 0.966f && dist > cur->distance )
        {
            return 0;
        }
    }

    return 1;
}


void J3DAPI sithAIUtil_AIPlaySoundMode(SithAIControlBlock* pLocal, unsigned int mode)
{
    if ( !pLocal || !pLocal->pOwner )
    {
        return;
    }

    if ( J3DINRANGE(mode, SITHSOUNDCLASS_CURIOUS, SITHSOUNDCLASS_RESERVED8) && sithAI_HasInstinct(pLocal, "talk") )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_TALK, (void*)mode);
    }
    else
    {
        sithSoundClass_PlayModeRandom(pLocal->pOwner, mode);
    }
}


void J3DAPI sithAIUtil_AISetMode(SithAIControlBlock* pLocal, unsigned int mode)
{
    switch ( mode )
    {
        case SITHAI_MODE_MOVING:
            pLocal->mode |= SITHAI_MODE_MOVING;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_4000;
            break;

        case SITHAI_MODE_SEARCHING:
            pLocal->mode |= SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            pLocal->mode &= ~(SITHAI_MODE_FLEEINGTOWAYPOINT | SITHAI_MODE_FLEEING);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            sithActor_SetHeadPYR(pLocal->pOwner, &rdroid_zeroVector3);
            break;

        case SITHAI_MODE_ATTACKING:
            pLocal->mode |= (SITHAI_MODE_ACTIVE | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_SEARCHING);
            pLocal->mode &= ~(SITHAI_MODE_FLEEINGTOWAYPOINT | SITHAI_MODE_FLEEING);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            break;

        case SITHAI_MODE_HUNTING:
            pLocal->mode |= SITHAI_MODE_HUNTING;
            pLocal->mode &= ~(SITHAI_MODE_FLEEINGTOWAYPOINT | SITHAI_MODE_FLEEING);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
            break;

        case SITHAI_MODE_FLEEINGTOWAYPOINT:
            pLocal->mode |= SITHAI_MODE_FLEEING;
            pLocal->mode |= SITHAI_MODE_FLEEINGTOWAYPOINT;
            pLocal->mode &= ~SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
            pLocal->mode &= ~SITHAI_MODE_HUNTING;
            break;

        case SITHAI_MODE_FLEEING:
            pLocal->mode |= SITHAI_MODE_FLEEING;
            pLocal->mode &= ~SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            pLocal->mode &= ~(SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode &= ~SITHAI_MODE_SLEEPING;
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_1;
            break;

        case SITHAI_MODE_BLOCK:
            pLocal->mode |= (SITHAI_MODE_BLOCK | SITHAI_MODE_NOCHECKFORCLIFF);
            pLocal->mode |= SITHAI_MODE_SEARCHING;
            pLocal->mode &= ~(SITHAI_MODE_CHASE_GOAL | SITHAI_MODE_ACTIVE);
            pLocal->mode &= ~(SITHAI_MODE_CIRCLESTRAFING | SITHAI_MODE_ATTACKING);
            pLocal->mode &= ~(SITHAI_MODE_UNKNOWN_1000000 | SITHAI_MODE_TARGETVISIBLE);
            break;

        case SITHAI_MODE_WALLCRAWLING:
            pLocal->mode |= SITHAI_MODE_WALLCRAWLING;
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_100;
            break;
    }
}


void J3DAPI sithAIUtil_AIClearMode(SithAIControlBlock* pLocal, unsigned int mode)
{
    switch ( mode )
    {
        case SITHAI_MODE_MOVING:
            pLocal->mode &= ~SITHAI_MODE_MOVING;
            pLocal->mode &= ~SITHAI_MODE_UNKNOWN_10;
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_1 | SITHAI_SUBMODE_UNKNOWN_2);
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_4000 | SITHAI_SUBMODE_UNKNOWN_4);
            pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_8 | SITHAI_SUBMODE_UNKNOWN_10);
            break;

        case SITHAI_MODE_TURNING:
            pLocal->mode &= ~SITHAI_MODE_TURNING;
            pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
            break;

        case SITHAI_MODE_TRAVERSEWPNTS:
            pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
            pLocal->mode &= ~SITHAI_MODE_HUNTING;
            pLocal->mode &= ~SITHAI_MODE_FLEEINGTOWAYPOINT;
            pLocal->submode &= ~SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION;
            sithAIUtil_ClearAIWaypoint(pLocal);
            break;

        case SITHAI_MODE_HUNTING:
            pLocal->mode &= ~SITHAI_MODE_HUNTING;
            if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
            {
                pLocal->mode &= ~SITHAI_MODE_TRAVERSEWPNTS;
                pLocal->mode &= ~SITHAI_MODE_FLEEINGTOWAYPOINT;
                pLocal->submode &= ~SITHAI_SUBMODE_SEMICONTINUOUSWPNTMOTION;
                sithAIUtil_ClearAIWaypoint(pLocal);
            }
            break;

        case SITHAI_MODE_WALLCRAWLING:
            pLocal->mode &= ~SITHAI_MODE_WALLCRAWLING;
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_100;
            break;
    }
}


void J3DAPI sithAIUtil_SetWeaponFireFlags(unsigned int weaponNum, SithAIUtilFireFlags* pFlags)
{
    switch ( weaponNum )
    {
        case 0:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_40;
            break;

        case 1:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_80;
            break;

        case 2:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_100;
            break;

        case 3:
            *pFlags |= SITHAIUTIL_FIRE_UNKNOWN_200;
            break;
    }
}

int J3DAPI sithAIUtil_AIGetMovePos(const SithAIControlBlock* pLocal, int flags, rdVector3* pDestPos)
{
    if ( (flags & 0x100000) != 0 )
    {
        rdVector_Copy3(pDestPos, &pLocal->pOwner->pos);
        return 1;
    }

    if ( (flags & 0x400) != 0 )
    {
        rdVector_Copy3(pDestPos, &pLocal->movePos);
        return 1;
    }

    if ( (flags & 0x200000) != 0 )
    {
        rdVector_Copy3(pDestPos, &pLocal->homePos);
        return 1;
    }

    if ( (flags & 0x300) != 0 )
    {
        if ( pLocal->goalThing )
        {
            rdVector_Copy3(pDestPos, &pLocal->goalThing->pos);
        }
        else
        {
            rdVector_Copy3(pDestPos, &pLocal->vecUnknown6);
        }

        return 1;
    }

    if ( (flags & 0x1000) != 0 )
    {
        if ( pLocal->pFleeFromThing )
        {
            rdVector_ScaleAdd3(pDestPos, &pLocal->pFleeFromThing->moveInfo.physics.velocity, sithTime_frameTimeFlex, &pLocal->pFleeFromThing->pos);
        }
    }

    return 0;
}

int J3DAPI sithAIUtil_GetXYHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    if ( !pThing || !pDest )
    {
#line 3437
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad args passed to sithAIUtil_GetXYHeadingVector().\n", 0, 0, 0, 0);
        return 0;
    }

    if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector_Copy3(pDest, &pThing->moveInfo.physics.velocity);
    }
    else
    {
        rdVector_Copy3(pDest, &pThing->orient.lvec);
    }

    pDest->z = 0.0f;
    pDest->x = stdMath_ClipNearZero(pDest->x);
    pDest->y = stdMath_ClipNearZero(pDest->y);
    pDest->z = stdMath_ClipNearZero(pDest->z);
    rdVector_Normalize3Acc(pDest);
    return 1;
}

int J3DAPI sithAIUtil_GetXYZHeadingVector(const SithThing* pThing, rdVector3* pDest)
{
    if ( !pThing || !pDest )
    {
#line 3462
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad args passed to sithAIUtil_GetXYHeadingVector().\n", 0, 0, 0, 0);
        return 0;
    }

    if ( pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector_Copy3(pDest, &pThing->moveInfo.physics.velocity);
        pDest->x = stdMath_ClipNearZero(pDest->x);
        pDest->y = stdMath_ClipNearZero(pDest->y);
        pDest->z = stdMath_ClipNearZero(pDest->z);
        rdVector_Normalize3Acc(pDest);
    }
    else
    {
        rdVector_Copy3(pDest, &pThing->orient.lvec);
    }

    return 1;
}

int J3DAPI sithAIUtil_CanAttackTarget(const SithThing* pThing)
{
    if ( !pThing )
    {
        return 0;
    }

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return 0;
    }

    if ( pThing->type == SITH_THING_PLAYER && (pThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithAIUtil_IsThingMoving(const SithThing* pThing)
{
    int bMoving;

    bMoving = 0;
    if ( !pThing )
    {
        return 0;
    }

    switch ( pThing->type )
    {
        case SITH_THING_FREE:
            break;

        case SITH_THING_ACTOR:
            if ( !pThing->controlInfo.aiControl.pLocal )
            {
                return 0;
            }

            bMoving = pThing->controlInfo.aiControl.pLocal->mode & SITHAI_MODE_MOVING;
            break;

        default:
            if ( pThing->moveType == SITH_MT_PHYSICS )
            {
                bMoving = (pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f)
                    | (pThing->moveInfo.physics.angularVelocity.x != 0.0f || pThing->moveInfo.physics.angularVelocity.y != 0.0f || pThing->moveInfo.physics.angularVelocity.z != 0.0f);
            }
            else if ( pThing->moveType == SITH_MT_PATH )
            {
                bMoving = pThing->moveInfo.pathMovement.mode & 1;
            }
            break;
    }

    return bMoving;
}


void J3DAPI sithAIUtil_ApplyForce(SithAIControlBlock* pLocal, const rdVector3* pDirection, float force)
{
    rdVector3 vecForce;

    rdVector_Scale3(&vecForce, pDirection, RD_MULTIPLY(pLocal->pOwner->moveInfo.physics.mass, force));
    sithPhysics_ApplyForce(pLocal->pOwner, &vecForce);
}

/* Retail's linker discarded this function (no retail symbol, address or caller); restored in full from the
   debug build (debug 0x004CE63C, /Od-identical). Retail still pools its 2.0f at 0x00507A14.
   Name and parameters are Jedi Knight's sithAI_SetRandomThingLook, with the TU's prefix. */
void J3DAPI sithAIUtil_SetRandomThingLook(rdMatrix34* orient, const SithThing* pThing, const rdVector3* pLookPos, float angle)
{
    float range;
    rdVector3 pyr;

    rdMatrix_LookAt(orient, &pThing->pos, pLookPos, 0.0f);
    if ( angle > 0.0f )
    {
        range = RD_MULTIPLY(angle, 2.0f);
        pyr.x = RD_MULTIPLY(D3DVAL(SITH_RAND()), range) - angle;
        pyr.y = RD_MULTIPLY(D3DVAL(SITH_RAND()), range) - angle;
        pyr.z = 0.0f;
        rdMatrix_PreRotate34(orient, &pyr);
    }
}

void J3DAPI sithAIUtil_RandomRotate(rdVector3* pOutPYR, float scalar)
{
    pOutPYR->x = RD_MULTIPLY(SITH_RAND() - 0.5f, scalar) + pOutPYR->x;
    pOutPYR->y = RD_MULTIPLY(SITH_RAND() - 0.5f, scalar) + pOutPYR->y;
    pOutPYR->z = RD_MULTIPLY(SITH_RAND() - 0.5f, scalar) + pOutPYR->z;
    rdVector_Normalize3Acc(pOutPYR);
}

/* Retail's linker discarded this function (no retail symbol, address or caller); restored in full from the
   debug build (debug 0x004CE751, assert line 3678, /Od-identical). Retail still pools its 360.0f at
   0x00507A18. Name and parameter are Jedi Knight's sithAI_RandomRotationVector, with the TU's prefix. */
void J3DAPI sithAIUtil_RandomRotationVector(rdVector3* randVec)
{
    rdVector3 pyr;

#line 3678
    SITH_ASSERTREL(randVec);

    pyr.x = RD_MULTIPLY(D3DVAL(SITH_RAND()), 360.0f);
    pyr.y = RD_MULTIPLY(D3DVAL(SITH_RAND()), 360.0f);
    pyr.z = 0.0f;
    rdVector_Rotate3(randVec, &rdroid_yVector3, &pyr);
}

size_t J3DAPI sithAIUtil_GetThingsInView(SithSector* pStartSector, const rdMatrix34* orient, float fovX, float fovY, int sizeResult, SithThing** pResult, int thingTypeMask, float maxDistance)
{
    float angleX;
    float curangleY;
    float sinFov;

#line 3730
    SITH_ASSERTREL(pStartSector!=NULL);

    if ( fovX < 0.0f || fovY < 0.0f )
    {
        return 0;
    }

    sithAIUtil_sizeThingsInView    = sizeResult;
    sithAIUtil_thingInViewTypeMask = thingTypeMask;
    sithAIUtil_maxDistanceToThingsInView = maxDistance;
    sithAIUtil_aThingsInView       = pResult;

    angleX = 90.0f - D3DVAL(fovX / 2.0f);
    curangleY = 90.0f - D3DVAL(fovY / 2.0f);
    stdMath_SinCos(angleX, &sinFov, &sithAIUtil_thingsInViewFovX);
    stdMath_SinCos(curangleY, &sinFov, &sithAIUtil_thingsInViewFovY);

    sithAdvanceRenderTick();
    sithAIUtil_numThingsInView   = 0;
    sithAIUtil_numVisitedSectors = 0;
    sithAIUtil_GetNextThingsInView(pStartSector, orient, 0.0f);
    return sithAIUtil_numThingsInView;
}

/* Debug-only stub (debug 0x004CE8BD, assert line 3788 `pStartSector!=NULL`, between GetThingsInView and
   GetNextThingsInView), restored in full and /Od-identical; retail's linker discarded it (no caller in either
   image). Name and the second parameter's meaning are inferred: it takes the
   start sector and a float range, returns 0 for a non-positive range and otherwise advances the render tick and
   returns an empty count. */
size_t J3DAPI sithAIUtil_GetThingsInRadius(SithSector* pStartSector, float radius)
{
    size_t numThings;

#line 3788
    SITH_ASSERTREL(pStartSector!=NULL);

    if ( radius <= 0.0f )
    {
        return 0;
    }

    numThings = 0;
    sithAdvanceRenderTick();
    return numThings;
}

void J3DAPI sithAIUtil_GetNextThingsInView(SithSector* pSector, const rdMatrix34* pOrient, float distance)
{
    SithThing* pCurThing;
    rdVector3 relPosDir;
    rdVector3 upDir;
    rdVector3 rightNorm;
    rdVector3 forwardVec;
    float dotUpDist;
    float dotRightDist;
    float dotAheadDist;
    float adjoinDist;
    SithSurfaceAdjoin* pAdjoin;
    SithSurface* pSurface;
    rdFace* pFace;

    if ( pSector->renderTick != sithMain_curRenderTick )
    {
        pSector->renderTick = sithMain_curRenderTick;
        if ( sithAIUtil_numVisitedSectors >= 128 )
        {
            return;
        }

        ++sithAIUtil_numVisitedSectors;

        pCurThing = pSector->pFirstThingInSector;
        while ( pCurThing && sithAIUtil_numThingsInView < sithAIUtil_sizeThingsInView )
        {
            if ( ((1 << pCurThing->type) & sithAIUtil_thingInViewTypeMask) != 0 && (pCurThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
            {
                rdVector_Sub3(&relPosDir, &pCurThing->pos, &pOrient->dvec);
                rdVector_Normalize3Acc(&relPosDir);

                rdVector_Normalize3(&upDir, &pOrient->uvec);
                rdVector_Normalize3(&rightNorm, &pOrient->rvec);
                rdVector_Normalize3(&forwardVec, &pOrient->lvec);

                dotUpDist    = rdVector_Dot3(&upDir, &relPosDir);
                dotRightDist = rdVector_Dot3(&rightNorm, &relPosDir);
                dotAheadDist  = rdVector_Dot3(&forwardVec, &relPosDir);

                if ( dotUpDist <= sithAIUtil_thingsInViewFovY && dotUpDist >= -sithAIUtil_thingsInViewFovY
                    && dotRightDist <= sithAIUtil_thingsInViewFovX && dotRightDist >= -sithAIUtil_thingsInViewFovX
                    && dotAheadDist >= 0.0f )
                {
                    if ( sithAIUtil_numThingsInView < sithAIUtil_sizeThingsInView )
                    {
                        sithAIUtil_aThingsInView[sithAIUtil_numThingsInView++] = pCurThing;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            pCurThing = pCurThing->pNextThingInSector;
        }

        if ( distance > sithAIUtil_maxDistanceToThingsInView )
        {
            return;
        }

        pAdjoin = pSector->pFirstAdjoin;
        while ( pAdjoin )
        {
            pSurface = pAdjoin->pAdjoinSurface;
            pFace    = &pSurface->face;
            if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0
                && (!pFace->pMaterial || !pFace->geometryMode || (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0 || pFace->pMaterial->formatType != STDCOLOR_FORMAT_RGB)
                && rdVector_Dot3(&pFace->normal, &pOrient->lvec) < 0.0f )
            {
                adjoinDist = distance + pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance;
                sithAIUtil_GetNextThingsInView(pAdjoin->pAdjoinSector, pOrient, adjoinDist);
            }

            pAdjoin = pAdjoin->pNextAdjoin;
        }
    }
}



int J3DAPI sithAIUtil_CanSeeTarget(const SithAIControlBlock* pLocal, SithThing* pTarget, float distance)
{
    SithThing* pOwner;
    float chance;
    float distFactor;

    chance = 1.0f;
    pOwner = pLocal->pOwner;

    if ( !pTarget || pTarget->type != SITH_THING_ACTOR && pTarget->type != SITH_THING_PLAYER )
    {
        return 1;
    }

    if ( distance < 2.0f )
    {
        goto check_invisible;
    }

    if ( (pLocal->mode & SITHAI_MODE_ACTIVE) == 0 )
    {
        chance *= 0.5f;
    }

    if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_HEADLIGHT) != 0 || (pTarget->unknownFlags & 1) != 0 )
    {
        goto check_invisible;
    }

    distFactor = RD_MULTIPLY(distance - 2.0f, 0.1f);
    distFactor = STDMATH_CLAMP(distFactor, 0.0f, 0.6f);
    chance = RD_MULTIPLY(1.0f - distFactor, chance);

    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINDARK) == 0 && rdLight_GetIntensity(&pTarget->pInSector->ambientLight) < 0.5f )
    {
        chance = RD_MULTIPLY(rdLight_GetIntensity(&pTarget->pInSector->ambientLight) + 0.2f, chance);
    }

    if ( pTarget->moveType == SITH_MT_PHYSICS )
    {
        if ( (pTarget->moveInfo.physics.flags & SITH_PF_CROUCHING) != 0 )
        {
            chance *= 0.75f;
        }

        if ( pTarget->moveInfo.physics.velocity.x == 0.0f && pTarget->moveInfo.physics.velocity.y == 0.0f && pTarget->moveInfo.physics.velocity.z == 0.0f )
        {
            chance *= 0.5f;
        }
    }

check_invisible:
    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            chance *= 0.05f;
        }

        if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_BLIND) != 0 )
        {
            chance *= 0.05f;
        }
    }

    chance = STDMATH_CLAMP(chance, 0.05f, 1.0f);
    if ( SITH_RAND() < chance )
    {
        return 1;
    }

    return 0;
}


