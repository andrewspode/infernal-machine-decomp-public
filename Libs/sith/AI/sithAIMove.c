#include "sithAIMove.h"
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(sithAIMove_countfit_1, 30, 4) CF_PROTO(sithAIMove_countfit_5, 3) /* end count-fit */
#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithSoundClass.h>
#ifdef J3D_DEBUG
#include <sith/World/sithTemplate.h>
#endif
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#ifdef J3D_DEBUG
#include <sith/World/sithWorld.h>
#endif
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_6, 30) CF_PROTO(sithAIMove_countfit_7, 0) /* end count-fit */
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

float sithAIMove_flt_585464 = 0;
float sithAIMove_flt_585468 = 0;
#ifdef J3D_DEBUG
static SithThing* sithAIMove_pMoveToDebugMarkThing = NULL;
static SithThing* sithAIMove_pMoveToPosGroundMarkThing = NULL;
static int sithAIMove_bDebugMoveToPos = 0;
#endif


#define sithAIMove_Abs(x) ((x) < 0.0f ? -(x) : (x))

void J3DAPI sithAIMove_Update(SithThing* pThing, float secDeltatTime)
{
    SithAIControlBlock* pLocal;
    static float sithAIMove_flt_539D18 = 20.0f;

#line 147
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);

    if ( pThing->type != SITH_THING_ACTOR )
    {
        return;
    }

    if ( pThing->thingInfo.actorInfo.health <= 0.0f )
    {
        return;
    }

    pLocal = pThing->controlInfo.aiControl.pLocal;
#line 158
    SITH_ASSERTREL(pLocal);

    if ( (pLocal->mode & SITHAI_MODE_SLEEPING) != 0 && (pLocal->mode & SITHAI_MODE_BLOCK) == 0 && (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) == 0 )
    {
        return;
    }

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 && (pLocal->mode & SITHAI_MODE_MOVING) != 0 && (pLocal->mode & SITHAI_MODE_DISABLED) == 0 )
    {
        rdVector3 lookPos;
        rdVector_Copy3(&lookPos, &pLocal->movePos);
        lookPos.z += pLocal->pOwner->thingInfo.actorInfo.eyeOffset.z;
        sithAIMove_AISetLookPos(pLocal, &lookPos);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 || (pLocal->submode & SITHAI_SUBMODE_BODYTRACKINGMOTION) != 0 )
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_BOSS) != 0 )
        {
            sithAIMove_UpdateBoss(pLocal, secDeltatTime);
        }
        else if ( pLocal->pOwner->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
        {
            sithAIMove_UpdateMineCar(pLocal, secDeltatTime);
        }
        else
        {
            if ( (pLocal->submode & SITHAI_SUBMODE_BODYTRACKINGMOTION) != 0 && pLocal->pTargetThing )
            {
                if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 || (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400) == 0 )
                {
                    sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                }
                else if ( sithAIMove_Abs(pThing->thingInfo.actorInfo.headPYR.y) > sithAIMove_flt_539D18 )
                {
                    sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
                }
            }

            sithAIMove_UpdateTurn(pLocal, secDeltatTime);
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_10) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
        {
            sithAIUtil_CheckWpntMoveAlign(pLocal);
        }
        else
        {
            sithAIMove_CheckMoveAlign(pLocal);
        }
    }

    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 && (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_10) == 0 )
    {
        sithAIMove_UpdateMove(pLocal, secDeltatTime);
    }

    if ( sithWeapon_HasWeaponSelected(pLocal->pOwner) )
    {
        sithPlayerControls_ProcessWeaponAim(pLocal->pOwner, secDeltatTime);
    }

    if ( !sithWeapon_IsAiming(pLocal->pOwner)
        && ((pLocal->mode & (SITHAI_MODE_UNKNOWN_80 | SITHAI_MODE_TURNING)) != 0 || (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400) != 0) )
    {
        if ( ((pLocal->mode & SITHAI_MODE_BLOCK) != 0 || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0)
            && (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_400) == 0 )
        {
            sithAIMove_UpdateHeadTrackLookPos(pLocal, secDeltatTime);
        }
        else
        {
            sithAIMove_UpdateHeadTrackTarget(pLocal, secDeltatTime);
        }
    }

    if ( pLocal->pOwner->pAttachedThing && (pLocal->pOwner->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
    {
        sithAIMove_UpdateQuetzTail(pLocal, secDeltatTime);
    }
    else if ( !strncmp(pThing->aName, "marduk", 6) )
    {
        sithAIMove_UpdateMardukTail(pThing, secDeltatTime);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_8, 7) /* end count-fit */
void J3DAPI sithAIMove_UpdateMineCar(SithAIControlBlock* pLocal, float secDeltaTime)
{
    static float sithAIMove_flt_539D1C = 0.999f;
    static float sithAIMove_flt_539D20 = 1.05f;
    float angle;
    SithThing* pThing;
    rdVector3 aimDir;
    static float sithAIMove_flt_539D24 = 1.0f;
    rdVector3 lvec;
    SithActorInfo* pActorInfo;
    int meshIdx;
    int moveState;
    float dot;
    float upDot;
    float fwdDot;
    float rightDot;
    float turnRate;
    float maxStep;
    float turnLimit;
    rdVector3 toLook;
    rdVector3 jointPos;
    rdVector3 lookDir;
    rdVector3 rvec;
    rdVector3 pyr;

    pThing = pLocal->pOwner;
    pActorInfo = &pThing->thingInfo.actorInfo;
    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    meshIdx = sithThing_GetThingMeshIndex(pThing, "redarmy");
    rdVector_Copy3(&jointPos, &pThing->renderData.paJointMatrices[meshIdx].dvec);
    rdVector_Sub3(&toLook, &pLocal->lookPos, &jointPos);

    rdVector_Rotate3(&lvec, &pThing->orient.lvec, &pThing->renderData.apTweakedAngles[meshIdx]);
    rdVector_Normalize3Acc(&lvec);

    dot = rdMath_DistancePointToPlane(&lvec, &pThing->orient.uvec, &rdroid_zeroVector3);
    rdVector_ScaleAdd3(&aimDir, &pThing->orient.uvec, -dot, &lvec);
    rdVector_Normalize3Acc(&aimDir);

    rdVector_Cross3(&rvec, &aimDir, &pThing->orient.uvec);
    rdVector_Normalize3Acc(&rvec);

    upDot = sithAIMove_Abs(rdVector_Dot3(&pThing->orient.uvec, &toLook));
    if ( upDot > sithAIMove_flt_539D1C )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
        {
#line 346
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Strange... AI '%s' is looking straight up or down.\n", pThing->aName, 0, 0, 0);
        }
    }
    else
    {
        dot = rdMath_DistancePointToPlane(&toLook, &pThing->orient.uvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&lookDir, &pThing->orient.uvec, -dot, &toLook);
        rdVector_Normalize3Acc(&lookDir);

        angle = rdMath_DeltaAngleNormalized(&lookDir, &aimDir, &pThing->orient.uvec);

        turnRate = RD_MULTIPLY(pActorInfo->maxRotVelocity, secDeltaTime);
        maxStep = RD_MULTIPLY(pActorInfo->maxRotVelocity, 0.07f);
        if ( sithAIMove_Abs(angle) <= maxStep && sithAIMove_Abs(angle) >= sithAIMove_flt_539D24 )
        {
            turnRate = RD_MULTIPLY(sithAIMove_Abs(angle) / maxStep, turnRate);
        }

        turnLimit = RD_MULTIPLY(turnRate, sithAIMove_flt_539D20);
        if ( sithAIMove_Abs(angle) <= turnLimit )
        {
            fwdDot = rdVector_Dot3(&pThing->orient.lvec, &lookDir);
            rightDot = rdVector_Dot3(&pThing->orient.rvec, &lookDir);
            pThing->renderData.apTweakedAngles[meshIdx].y = 90.0f - stdMath_ArcSin3(fwdDot);
            if ( rightDot > 0.0f )
            {
                pThing->renderData.apTweakedAngles[meshIdx].y = -pThing->renderData.apTweakedAngles[meshIdx].y;
            }
        }
        else
        {
            pyr.x = 0.0f;
            pyr.y = -RD_MULTIPLY(D3DVAL(angle < 0.0f ? -1 : 1), turnRate);
            pyr.z = 0.0f;
            rdVector_Rotate3Acc(&aimDir, &pyr);

            fwdDot = rdVector_Dot3(&pThing->orient.lvec, &aimDir);
            rightDot = rdVector_Dot3(&pThing->orient.rvec, &aimDir);
            pThing->renderData.apTweakedAngles[meshIdx].y = 90.0f - stdMath_ArcSin3(fwdDot);
            if ( rightDot > 0.0f )
            {
                pThing->renderData.apTweakedAngles[meshIdx].y = -pThing->renderData.apTweakedAngles[meshIdx].y;
            }

            if ( moveState == 0 )
            {
                if ( angle < 0.0f )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                    if ( angle < -45.0f )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_105;
                    }
                }
                else
                {
                    pThing->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                    if ( angle > 45.0f )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_106;
                    }
                }
            }
            return;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_TURNING;
    if ( moveState == 0 )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_9, 12) /* end count-fit */
void J3DAPI sithAIMove_UpdateTurn(SithAIControlBlock* pLocal, float secDeltatTime)
{
    static float sithAIMove_flt_539D28 = 0.999f;
    rdVector3 goalLVec;
    static float sithAIMove_flt_539D2C = 1.05f;
    static float sithAIMove_flt_539D30 = 1.0f;
    SithThing* pOwner;
    SithActorInfo* pActorInfo;
    int moveState;
    rdVector3 lvec;
    rdVector3 pyr;
    float dist;
    float angle;
    float maxStep;
    float turnRate;
    float turnLimit;
    float upDot;

    pOwner = pLocal->pOwner;
    pActorInfo = &pOwner->thingInfo.actorInfo;
    rdVector_Copy3(&goalLVec, &pLocal->goalLVec);
    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    moveState = sithAIMove_AIGetMoveState(pLocal);
    if ( moveState == 2 )
    {
        return;
    }

    upDot = sithAIMove_Abs(rdVector_Dot3(&pOwner->orient.uvec, &goalLVec));
    if ( upDot > sithAIMove_flt_539D28 )
    {
        if ( (pOwner->moveInfo.physics.flags & SITH_PF_FLY) == 0 )
        {
#line 491
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Strange... AI '%s' is looking straight up or down.\n", pOwner->aName, 0, 0, 0);
        }
    }
    else
    {
        dist = rdMath_DistancePointToPlane(&goalLVec, &pOwner->orient.uvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&lvec, &pOwner->orient.uvec, -dist, &goalLVec);
        rdVector_Normalize3Acc(&lvec);

        angle = rdMath_DeltaAngleNormalized(&lvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
        if ( sithAIMove_TrySpecialTurn(pLocal, angle) )
        {
            return;
        }

        turnRate = RD_MULTIPLY(pActorInfo->maxRotVelocity, secDeltatTime);
        maxStep = RD_MULTIPLY(pActorInfo->maxRotVelocity, 0.07f);
        if ( sithAIMove_Abs(angle) <= maxStep && sithAIMove_Abs(angle) >= sithAIMove_flt_539D30 )
        {
            turnRate = RD_MULTIPLY(sithAIMove_Abs(angle) / maxStep, turnRate);
        }

        turnLimit = RD_MULTIPLY(turnRate, sithAIMove_flt_539D2C);
        if ( sithAIMove_Abs(angle) <= turnLimit )
        {
            rdVector_Copy3(&pOwner->orient.lvec, &lvec);
            rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
            rdVector_Normalize3Acc(&pOwner->orient.rvec);
            rdVector_Cross3(&pOwner->orient.uvec, &pOwner->orient.rvec, &pOwner->orient.lvec);
        }
        else
        {
            pyr.x = 0.0f;
            pyr.y = -RD_MULTIPLY(D3DVAL(angle < 0.0f ? -1 : 1), turnRate);
            pyr.z = 0.0f;
            rdMatrix_PreRotate34(&pOwner->orient, &pyr);
            rdMatrix_Normalize34(&pOwner->orient);
            if ( moveState == 0 )
            {
                if ( angle < 0.0f )
                {
                    pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                    if ( angle < -45.0f )
                    {
                        pOwner->moveStatus = SITHPLAYERMOVE_UNKNOWN_105;
                    }
                }
                else
                {
                    pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                    if ( angle > 45.0f )
                    {
                        pOwner->moveStatus = SITHPLAYERMOVE_UNKNOWN_106;
                    }
                }
            }
            return;
        }
    }

    pLocal->mode &= ~SITHAI_MODE_TURNING;
    if ( moveState == 0 )
    {
        pOwner->moveStatus = SITHPLAYERMOVE_STILL;
    }
}


void J3DAPI sithAIMove_UpdateMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    int bGoalReached;
    SithThing* pThing;
    rdVector3 pos;
    rdVector3 movePos;
    rdVector3 vel;
    float moveDist;
    float goalDist;
    float dot;
    static float sithAIMove_flt_539D34 = 0.3f;

    bGoalReached = 0;
#line 607
    SITH_ASSERTREL(pLocal);

    pThing = pLocal->pOwner;
#line 610
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_ACTOR));

    if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    if ( sithTime_msecGameTime < pLocal->msecPauseMoveUntil )
    {
        return;
    }

    pLocal->submode &= ~(SITHAI_SUBMODE_UNKNOWN_4000 | SITHAI_SUBMODE_UNKNOWN_4);

    moveDist = RD_MULTIPLY(RD_MULTIPLY(pThing->thingInfo.actorInfo.maxThrust, secDeltaTime), pLocal->moveSpeed);
    rdVector_Copy3(&pos, &pThing->pos);
    rdVector_Copy3(&movePos, &pLocal->movePos);

    goalDist = sithAIMove_GetGoalTolerance(pLocal, &pos, &movePos);

    rdVector_Sub3(&pLocal->moveDirection, &movePos, &pos);
    pLocal->moveDistance = rdVector_Normalize3Acc(&pLocal->moveDirection);

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) == 0 )
    {
        rdVector_Scale3(&vel, &pLocal->moveDirection, moveDist);
    }
    else
    {
        dot = rdVector_Dot3(&pThing->orient.lvec, &pLocal->moveDirection);
        dot = STDMATH_CLAMP(dot, sithAIMove_flt_539D34, 1.0f);
        moveDist *= dot;
        rdVector_Scale3(&vel, &pThing->orient.lvec, moveDist);
    }

    vel.x = stdMath_ClipNearZero(vel.x);
    vel.y = stdMath_ClipNearZero(vel.y);
    vel.z = stdMath_ClipNearZero(vel.z);
    if ( (pLocal->mode & SITHAI_MODE_UNKNOWN_8000) != 0 && pLocal->goalThing )
    {
        rdVector3 goalDir;
        rdVector3 dir;
        float goalSpeed;

        rdVector_Copy3(&goalDir, &pLocal->goalThing->moveInfo.physics.velocity);
        goalSpeed = rdVector_Normalize3Acc(&goalDir);
        rdVector_Copy3(&dir, &pThing->moveInfo.physics.velocity);
        rdVector_Normalize3Acc(&dir);
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &dir, goalSpeed);
    }
    else
    {
        rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &vel);
    }

    switch ( sithAIMove_AIGetMoveState(pLocal) )
    {
        case 0:
            break;

        case 2:
            memset(&pLocal->pOwner->moveInfo.physics.velocity, 0, sizeof(rdVector3));
            break;
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
    {
        float speed;
        speed = rdVector_Len3(&pThing->moveInfo.physics.velocity);
        rdVector_Scale3(&pThing->moveInfo.physics.velocity, &pThing->orient.lvec, speed);
    }

    if ( (pLocal->mode & SITHAI_MODE_NOCHECKFORCLIFF) == 0
        && (pThing->moveInfo.physics.velocity.x != 0.0f || pThing->moveInfo.physics.velocity.y != 0.0f || pThing->moveInfo.physics.velocity.z != 0.0f) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) == 0 && pThing->attach.flags )
        {
            bGoalReached = sithAIMove_CheckWalkMove(pLocal, secDeltaTime);
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 && (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) == 0 )
        {
            bGoalReached = sithAIMove_CheckFlyMove(pLocal, secDeltaTime);
        }
        else if ( (pThing->flags & (SITH_TF_AIRDESTROYED | SITH_TF_SUBMERGED)) != 0 )
        {
            bGoalReached = sithAIMove_CheckSwimMove(pLocal, secDeltaTime);
        }

        if ( (pLocal->mode & SITHAI_MODE_MOVING) == 0 )
        {
            return;
        }
    }

    if ( RD_MULTIPLY(pThing->collide.movesize, 3.0f) >= pLocal->moveDistance )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_SET, NULL);
    }

    if ( !bGoalReached && pLocal->moveDistance <= goalDist )
    {
        bGoalReached = 1;
    }

    if ( bGoalReached )
    {
        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_2) != 0 )
        {
            sithAIMove_StopAIMovement(pLocal);
            sithAIMove_AISetMovePos(pLocal, &pLocal->vecUnknown3, pLocal->moveSpeed);
            return;
        }

        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_8) != 0 )
        {
            int flags;
            flags = 0;
            if ( (pLocal->mode & SITHAI_MODE_ACTIVE) != 0 )
            {
                flags |= 0x100;
            }

            if ( sithAIUtil_SweepMoveToPathPos(pLocal, &pLocal->vecUnknown3, 45.0f, 90.0f, 45.0f, flags) )
            {
                return;
            }
        }

        if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 && sithAIUtil_AIAdvanceToNextWpnt(pLocal, 1) )
        {
            return;
        }

        sithAIMove_SetGoalReached(pLocal);
    }
}



void J3DAPI sithAIMove_CheckMoveAlign(SithAIControlBlock* pLocal)
{
    if ( (pLocal->submode & SITHAI_SUBMODE_NOMOVEBACKWARDS) != 0 )
    {
        if ( rdVector_Dot3(&pLocal->goalLVec, &pLocal->pOwner->orient.lvec) >= 0.0f )
        {
            pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
        }
    }
    else
    {
        pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_10;
    }
}

double J3DAPI sithAIMove_GetGoalTolerance(SithAIControlBlock* pLocal, rdVector3* a2, rdVector3* a3)
{
    float minDist;
    int bOnSurface;
    float distance;
    SithThing* pThing;
    int numAligned;
    float dist1;
    float dist2;
    rdVector3 delta;
    float dot;
    rdVector3 pos;
    rdVector3 pos2;
    static float sithAIMove_flt_539D38 = 0.05f;

    pThing = pLocal->pOwner;
#line 859
    SITH_ASSERTREL(pThing);

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
    {
        return J3DMIN(pThing->collide.movesize, 0.05f);
    }

    if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
    {
        numAligned = 0;
        bOnSurface = 0;
        distance = 0.0f;
        minDist = J3DMIN(pThing->collide.movesize, 0.03f);
        if ( pThing->moveStatus == SITHPLAYERMOVE_MOUNTING_WALL )
        {
            return minDist;
        }

        rdVector_Sub3(&delta, a3, a2);
        if ( sithAIMove_Abs(delta.x) <= minDist )
        {
            ++numAligned;
            a3->x = a2->x;
            delta.x = 0.0f;
        }

        if ( sithAIMove_Abs(delta.y) <= minDist )
        {
            ++numAligned;
            a3->y = a2->y;
            delta.y = 0.0f;
        }

        if ( sithAIMove_Abs(delta.z) <= minDist )
        {
            ++numAligned;
            a3->z = a2->z;
            delta.z = 0.0f;
        }

        if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
        {
            distance = rdVector_Normalize3Acc(&delta);
            dot = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &delta);
            if ( sithAIMove_Abs(dot) < 0.996f )
            {
                bOnSurface = 1;
            }
        }

        if ( !bOnSurface )
        {
            if ( numAligned == 2 || distance <= RD_MULTIPLY(pThing->collide.movesize, 2.0f) )
            {
                rdVector_Copy3(a3, a2);
            }
            else
            {
                sithAIMove_Unreachable(pLocal);
            }
        }

        return minDist;
    }

    minDist = J3DMIN(pThing->collide.movesize, 0.03f);
    a2->z = pThing->pos.z;
    a3->z = pThing->pos.z;
    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_100) != 0 )
    {
        rdVector_ScaleAdd3(&pos, &pThing->orient.rvec, sithAIMove_flt_539D38, a2);
        dist1 = stdMath_Dist2D1(pos.x - a3->x, pos.y - a3->y);
        rdVector_ScaleAdd3(&pos2, &pThing->orient.rvec, -sithAIMove_flt_539D38, a2);
        dist2 = stdMath_Dist2D1(pos2.x - a3->x, pos2.y - a3->y);
        if ( dist1 < sithAIMove_flt_539D38 || dist2 < sithAIMove_flt_539D38 )
        {
            rdVector_Copy3(a3, a2);
        }
    }

    return minDist;
}

int J3DAPI sithAIMove_CheckWalkMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    rdVector3 endPos;
    rdVector3 direction;
    float offset;

    rdVector_ScaleAdd3(&endPos, &pLocal->pOwner->moveInfo.physics.velocity, secDeltaTime, &pLocal->pOwner->pos);
    if ( ((pLocal->pOwner->idx + sithMain_frameNumber) & 1) != 0 )
    {
        if ( (pLocal->mode & SITHAI_MODE_WALLCRAWLING) != 0 )
        {
            sithAIUtil_GetXYZHeadingVector(pLocal->pOwner, &direction);
            offset = pLocal->pOwner->collide.movesize;
            if ( offset - 0.03f > 0.0f )
            {
                offset -= 0.03f;
            }
        }
        else
        {
            sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &direction);
            offset = pLocal->pOwner->collide.movesize + 0.03f;
        }

        rdVector_ScaleAdd3(&endPos, &direction, offset, &endPos);
    }

    switch ( sithAIUtil_CanWalk(pLocal, &endPos, NULL) )
    {
        case 1:
            break;

        case 0:
        case 8:
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_CLIFF, NULL);
            break;

        case 2:
            if ( !sithAI_HasInstinct(pLocal, "wallcrawl") )
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_CLIFF, NULL);
            }

            break;

        case 4:
            if ( (pLocal->submode & SITHAI_SUBMODE_ALLOWSTEPTHING) == 0 )
            {
                sithAI_EmitEvent(pLocal, SITHAI_EVENT_HIT_THING, NULL);
            }

            break;

        default:
            sithAIMove_Unreachable(pLocal);
            break;
    }

    return 0;
}

int J3DAPI sithAIMove_CheckFlyMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    SithSector* pSector;
    rdVector3 endPosition;
    rdVector3 curdir;

    pThing = pLocal->pOwner;
    if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        pThing->moveInfo.physics.velocity.z += 0.5f;
        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
    }
    else
    {
        rdVector_ScaleAdd3(&endPosition, &pThing->moveInfo.physics.velocity, secDeltaTime, &pThing->pos);
        sithAIUtil_GetXYHeadingVector(pThing, &curdir);
        rdVector_ScaleAdd3(&endPosition, &curdir, pThing->collide.movesize + 0.05f, &endPosition);

        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPosition, 0.0f);
        if ( !pSector )
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }

        if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 )
        {
            sithAIMove_Unreachable(pLocal);
            return 1;
        }

        if ( pSector && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            sithAIMove_StopAIMovement(pLocal);
            pThing->moveStatus = SITHPLAYERMOVE_WALKING;
            pThing->moveInfo.physics.velocity.z += 0.5f;
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithAIMove_CheckSwimMove(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    SithSector* pSector;
    rdVector3 endPosition;
    rdVector3 curdir;

    pThing = pLocal->pOwner;
    if ( pThing->moveInfo.physics.velocity.z != 0.0f )
    {
        rdVector_Copy3(&endPosition, &pThing->pos);
        if ( (pLocal->submode & SITHAI_SUBMODE_SWIMNEARSURFACE) != 0 )
        {
            endPosition.z = RD_MULTIPLY(pThing->collide.movesize, 0.2f) + endPosition.z;
        }
        else
        {
            endPosition.z = RD_MULTIPLY(pThing->collide.movesize, 0.5f) + endPosition.z;
        }

        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPosition, 0.0f);
        if ( pSector && (pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            pThing->moveInfo.physics.velocity.z = -0.05f;
            if ( pLocal->movePos.z > pThing->pos.z )
            {
                pLocal->movePos.z = pThing->pos.z - 0.05f;
            }
        }
    }

    rdVector_ScaleAdd3(&endPosition, &pThing->moveInfo.physics.velocity, secDeltaTime, &pThing->pos);
    sithAIUtil_GetXYHeadingVector(pLocal->pOwner, &curdir);
    rdVector_ScaleAdd3(&endPosition, &curdir, pLocal->pOwner->collide.movesize + 0.03f, &endPosition);

    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPosition, 0.0f);
    if ( !pSector )
    {
        sithAIMove_Unreachable(pLocal);
        return 0;
    }

    if ( (pSector->flags & SITH_SECTOR_NOACTORENTER) != 0 || (pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        sithAIMove_Unreachable(pLocal);
    }

    return 0;
}

int J3DAPI sithAIMove_AIGetMoveState(const SithAIControlBlock* pLocal)
{
    switch ( pLocal->pOwner->moveStatus )
    {
        case SITHPLAYERMOVE_ROLLING_LEFT:
        case SITHPLAYERMOVE_ROLLING_RIGHT:
        case SITHPLAYERMOVE_STRAFING_LEFT:
        case SITHPLAYERMOVE_STRAFING_RIGHT:
        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
        case SITHPLAYERMOVE_STAND_TO_CRAWL:
        case SITHPLAYERMOVE_CRAWL_TO_STAND:
        case SITHPLAYERMOVE_WALK2STAND:
        case SITHPLAYERMOVE_STAND2WALK:
        case SITHPLAYERMOVE_UNKNOWN_78:
        case SITHPLAYERMOVE_STAND2RUN:
        case SITHPLAYERMOVE_UNKNOWN_80:
        case SITHPLAYERMOVE_UNKNOWN_81:
        case SITHPLAYERMOVE_SLIDEDOWNBACK:
        case SITHPLAYERMOVE_KNOCKEDOUT:
        case SITHPLAYERMOVE_RUNOVER:
            return 1;

        case SITHPLAYERMOVE_MOUNTING_WALL:
        case SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES:
        case SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES:
        case SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES:
        case SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES:
        case SITHPLAYERMOVE_TURNING_180_DEGREES:
            return 2;

        default:
            return 0;
    }
}

int J3DAPI sithAIMove_TrySpecialTurn(SithAIControlBlock* pLocal, float angle)
{
    int turned;
    float absAngle;
    int currentdirFlags;
    int turnFlags;

    turned = 0;
    if ( (pLocal->submode & SITHAI_SUBMODE_SPECIALTURNS) == 0 )
    {
        return 0;
    }

    if ( sithAIMove_AIGetMoveState(pLocal) )
    {
        return 1;
    }

    absAngle = sithAIMove_Abs(angle);
    if ( absAngle < 22.5f )
    {
        return 0;
    }

    currentdirFlags = angle < 0.0f ? 0x200 : 0x400;
    if ( J3DINRANGE(absAngle, 112.5f, 180.0f) )
    {
        turnFlags = currentdirFlags | 0x10;
        if ( J3DINRANGE(absAngle, 112.5f, 157.5f) && sithAIMove_StartSpecialMove(pLocal, turnFlags) )
        {
            turned = 1;
        }

        if ( !turned && sithAIMove_StartSpecialMove(pLocal, 0x20) )
        {
            turned = 1;
        }
    }
    else if ( J3DINRANGE(absAngle, 67.5f, 112.5f) )
    {
        turnFlags = currentdirFlags | 0x08;
        if ( sithAIMove_StartSpecialMove(pLocal, turnFlags) )
        {
            turned = 1;
        }
    }

    if ( !turned )
    {
        turnFlags = currentdirFlags | 0x04;
        if ( sithAIMove_StartSpecialMove(pLocal, turnFlags) )
        {
            turned = 1;
        }
    }

    return turned;
}

float J3DAPI sithAIMove_UpdateAIMove(SithAIControlBlock* pLocal)
{
    float moveSpeed;
    SithThing* pThing;
    SithThingMoveStatus moveStatus;
    rdVector3 localVelocity;
    SithPhysicsInfo* pPhysics;
    SithPuppetSubMode cursubmode;
    int moveDirection;
    float curthreshold;

    pPhysics = &pLocal->pOwner->moveInfo.physics;
    pThing = pLocal->pOwner;
    moveSpeed  = 0.0f;
    cursubmode    = SITHPUPPETSUBMODE_STAND;
    moveStatus = SITHPLAYERMOVE_STILL;
    curthreshold  = 0.01f;
#line 1306
    SITH_ASSERTREL(pThing);

    if ( !pThing->pInSector )
    {
    }
    else if ( pPhysics->velocity.x == 0.0f && pPhysics->velocity.y == 0.0f && pPhysics->velocity.z == 0.0f && (pLocal->mode & SITHAI_MODE_TURNING) == 0 )
    {
    }
    else if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
    }
    else
    {
        rdMatrix_TransformVectorOrtho34(&localVelocity, &pPhysics->velocity, &pThing->orient);
        if ( (pPhysics->flags & SITH_PF_FLY) != 0
            && sithAIMove_Abs(localVelocity.z) > sithAIMove_Abs(localVelocity.y)
            && sithAIMove_Abs(localVelocity.z) > sithAIMove_Abs(localVelocity.x) )
        {
            moveSpeed = localVelocity.z;
            if ( moveSpeed > curthreshold )
            {
                moveStatus = SITHPLAYERMOVE_UNKNOWN_103;
                cursubmode    = SITHPUPPETSUBMODE_RISING;
            }
            else if ( moveSpeed < -curthreshold )
            {
                moveStatus = SITHPLAYERMOVE_UNKNOWN_104;
                cursubmode    = SITHPUPPETSUBMODE_FALLFORWARD;
            }
        }
        else
        {
            if ( sithAIMove_Abs(localVelocity.z) > sithAIMove_Abs(localVelocity.y) && sithAIMove_Abs(localVelocity.z) > sithAIMove_Abs(localVelocity.x) )
            {
                moveSpeed     = localVelocity.z;
                moveDirection = 2;
            }
            else if ( sithAIMove_Abs(localVelocity.y) > sithAIMove_Abs(localVelocity.x) )
            {
                moveSpeed     = localVelocity.y;
                moveDirection = 1;
            }
            else
            {
                moveSpeed     = localVelocity.x;
                moveDirection = 0;
            }

            sithAIMove_GetAIMoveModes(pThing, moveSpeed, moveDirection, &moveStatus, &cursubmode);
        }
    }

    if ( pThing->thingInfo.actorInfo.bControlsDisabled || pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return moveSpeed;
    }

    switch ( sithAIMove_AIGetMoveState(pLocal) )
    {
        case 0:
            break;

        case 1:
            return moveSpeed;

        case 2:
            return 0.0f;
    }

    if ( pThing->pPuppetState->submode != cursubmode )
    {
        sithAIMove_SetSubMode(pLocal, moveStatus, cursubmode);
    }

    return moveSpeed;
}

void J3DAPI sithAIMove_GetAIMoveModes(const SithThing* pThing, float moveSpeed, int moveDirection, SithThingMoveStatus* pOutMoveStatus, SithPuppetSubMode* pOutSubmode)
{
    float walkSpeed;
    float runSpeed;
    float turnSpeed;
    SithAIControlBlock* pLocal;
    static float sithAIMove_flt_539D3C = 0.005f;
    static float sithAIMove_flt_539D40 = 0.27f;
    static float sithAIMove_flt_539D44 = 0.001f;

    pLocal = pThing->controlInfo.aiControl.pLocal;
    walkSpeed = 0.001f;
    runSpeed  = 0.27f;
    turnSpeed = 0.005f;
#line 1428
    SITH_ASSERTREL(pLocal);

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE15) != 0 )
    {
        walkSpeed = RD_MULTIPLY(sithAIMove_flt_539D44, 1.15f);
        runSpeed  = RD_MULTIPLY(sithAIMove_flt_539D40, 1.15f);
    }
    else if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE10) != 0 )
    {
        walkSpeed = RD_MULTIPLY(sithAIMove_flt_539D44, 1.1f);
        runSpeed  = RD_MULTIPLY(sithAIMove_flt_539D40, 1.1f);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 )
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FLYERMOVE) != 0 )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_105 )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNLEFT;
                return;
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_106 )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNRIGHT;
                return;
            }
        }

        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE15) != 0 )
        {
            turnSpeed = RD_MULTIPLY(sithAIMove_flt_539D3C, 1.15f);
        }
        else if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FASTMOVE10) != 0 )
        {
            turnSpeed = RD_MULTIPLY(sithAIMove_flt_539D3C, 1.1f);
        }

        if ( sithAIMove_Abs(moveSpeed) < turnSpeed )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_TURNING_LEFT || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_105 )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_LEFT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNLEFT;
            }
            else
            {
                *pOutMoveStatus = SITHPLAYERMOVE_TURNING_RIGHT;
                *pOutSubmode    = SITHPUPPETSUBMODE_TURNRIGHT;
            }

            return;
        }
    }

    switch ( moveDirection )
    {
        case 0:
            if ( sithAIMove_Abs(moveSpeed) >= runSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_RUNNING;
                *pOutSubmode    = SITHPUPPETSUBMODE_RUN;
            }
            else if ( sithAIMove_Abs(moveSpeed) >= walkSpeed && sithAIMove_Abs(moveSpeed) < runSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                *pOutSubmode    = SITHPUPPETSUBMODE_WALK;
            }
            else
            {
                *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
            }

            break;

        case 2:
            if ( !pThing->attach.flags )
            {
                if ( moveSpeed >= -walkSpeed && moveSpeed <= walkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                    *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
                }
                else if ( sithAIMove_Abs(moveSpeed) > walkSpeed )
                {
                    *pOutMoveStatus = SITHPLAYERMOVE_FALLING;
                    *pOutSubmode    = SITHPUPPETSUBMODE_FALL;
                }

                break;
            }

            // Fall through

        case 1:
            if ( moveSpeed >= runSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_RUNNING;
                *pOutSubmode    = SITHPUPPETSUBMODE_RUN;
            }
            else if ( moveSpeed >= walkSpeed && moveSpeed < runSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                *pOutSubmode    = SITHPUPPETSUBMODE_WALK;
            }
            else if ( moveSpeed > -walkSpeed && moveSpeed < walkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_STILL;
                *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
            }
            else if ( moveSpeed <= -walkSpeed )
            {
                *pOutMoveStatus = SITHPLAYERMOVE_WALKING;
                *pOutSubmode    = SITHPUPPETSUBMODE_WALKBACK;
            }

            break;

        default:
            *pOutMoveStatus = SITHPLAYERMOVE_STILL;
            *pOutSubmode    = SITHPUPPETSUBMODE_STAND;
            break;
    }
}

void J3DAPI sithAIMove_SetSubMode(SithAIControlBlock* pLocal, SithThingMoveStatus moveStatus, size_t submode)
{
    SithThing* pThing;
    int majorMode;
    SithPuppetSubMode curSubmode;
    size_t transitionMode;
    SithThingMoveStatus transitionStatus;
    rdPuppetTrackCallback pfCallback;
    int bTransition;
    rdKeyframe* pKeyframe;
    SithPuppetTrack* pTrack;
    int trackNum;

    pfCallback = NULL;
    pThing = pLocal->pOwner;
    transitionMode = 0;
    bTransition = 0;
    trackNum = -1;
    if ( !pThing || !pThing->pPuppetClass || !pThing->pPuppetState )
    {
#line 1588
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithAIMove_SetSubMode: Bad puppet data for AI '%s'.\n", pThing->aName, 0, 0, 0);
        return;
    }

    majorMode = pThing->pPuppetState->majorMode;
    curSubmode = pThing->pPuppetState->submode;
    switch ( curSubmode )
    {
        case SITHPUPPETSUBMODE_STAND:
        case SITHPUPPETSUBMODE_TURNLEFT:
        case SITHPUPPETSUBMODE_TURNRIGHT:
            switch ( submode )
            {
                case SITHPUPPETSUBMODE_WALK:
                case SITHPUPPETSUBMODE_RUN:
                case SITHPUPPETSUBMODE_WALKBACK:
                    transitionMode = SITHPUPPETSUBMODE_STAND2WALK;
                    transitionStatus = SITHPLAYERMOVE_STAND2WALK;
                    break;
            }
            break;

        case SITHPUPPETSUBMODE_WALK:
            switch ( submode )
            {
                case SITHPUPPETSUBMODE_STAND:
                case SITHPUPPETSUBMODE_TURNLEFT:
                case SITHPUPPETSUBMODE_TURNRIGHT:
                    transitionMode = SITHPUPPETSUBMODE_WALK2STAND;
                    transitionStatus = SITHPLAYERMOVE_WALK2STAND;
                    break;
            }
            break;

        case SITHPUPPETSUBMODE_RUN:
            switch ( submode )
            {
                case SITHPUPPETSUBMODE_STAND:
                case SITHPUPPETSUBMODE_TURNLEFT:
                case SITHPUPPETSUBMODE_TURNRIGHT:
                    transitionMode = SITHPUPPETSUBMODE_WALK2STAND;
                    transitionStatus = SITHPLAYERMOVE_WALK2STAND;
                    break;
            }
            break;

        case SITHPUPPETSUBMODE_FALL:
            switch ( submode )
            {
                case SITHPUPPETSUBMODE_STAND:
                    if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithAIMove_PuppetCallback) > -1 )
                    {
                        moveStatus = SITHPLAYERMOVE_LAND;
                    }
                    break;
            }
            break;
    }

    if ( transitionMode > 0 )
    {
        if ( pThing->pPuppetClass->aModes[majorMode][transitionMode].pKeyframe )
        {
            bTransition = 1;
        }

        if ( bTransition )
        {
            sithPuppet_ClearMode(pThing, curSubmode);
            moveStatus = transitionStatus;
            submode = transitionMode;
            pfCallback = sithAIMove_PuppetCallback;
            switch ( transitionMode )
            {
                case SITHPUPPETSUBMODE_STAND2WALK:
                    memset(&pThing->moveInfo.physics.velocity, 0, sizeof(rdVector3));
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STAND2WALK);
                    break;

                case SITHPUPPETSUBMODE_WALK2STAND:
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_WALK2STAND);
                    break;
            }
        }
    }

    pKeyframe = pThing->pPuppetClass->aModes[majorMode][submode].pKeyframe;
    if ( !pKeyframe )
    {
        // no keyframe for this submode
    }
    else
    {
        pTrack = sithPuppet_FindActiveTrack(pThing, pKeyframe);
        if ( pTrack )
        {
            sithPuppet_SynchMode(pThing, pTrack->submode, submode, -1.0f, 0);
        }
        else
        {
            sithPuppet_SetSubMode(pThing, submode, pfCallback);
        }
    }

    pThing->pPuppetState->submode = submode;
    pThing->moveStatus = moveStatus;
}


void J3DAPI sithAIMove_PuppetCallback(SithThing* pCallbackThing, int trackNum, rdKeyMarkerType marker)
{
    SithThing* pThing;
    SithPhysicsInfo* pPhysics;
    SithAIControlBlock* pLocal;

    pThing = pCallbackThing;
    pPhysics = &pThing->moveInfo.physics;
#line 1742
    SITH_ASSERTREL(pThing);
    pLocal = pThing->controlInfo.aiControl.pLocal;
    if ( !pLocal )
    {
        sithPuppet_DefaultCallback(pThing, trackNum, marker);
        return;
    }

    switch ( marker )
    {
        case RDKEYMARKER_LEFTFOOT:
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_STRAFING_RIGHT:
                    sithAIMove_FinishSpecialMove(pLocal, 2);
                    break;

                default:
                    sithPuppet_DefaultCallback(pThing, trackNum, marker);
                    break;
            }
            break;

        case RDKEYMARKER_RIGHTFOOT:
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_STRAFING_LEFT:
                    sithAIMove_FinishSpecialMove(pLocal, 2);
                    break;

                default:
                    sithPuppet_DefaultCallback(pThing, trackNum, marker);
                    break;
            }
            break;

        case RDKEYMARKER_DIED:
            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_KNOCKEDOUT:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED1);
                    break;

                case SITHPLAYERMOVE_RUNOVER:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED2);
                    break;

                case SITHPLAYERMOVE_ROLLING_LEFT:
                case SITHPLAYERMOVE_ROLLING_RIGHT:
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RESERVED3);
                    break;

                default:
                    sithPuppet_DefaultCallback(pThing, trackNum, marker);
                    break;
            }
            break;

        case (rdKeyMarkerType)0:
            sithPuppet_FreeTrackByIndex(pThing, trackNum);
            if ( (pThing->flags & (SITH_TF_DESTROYED | SITH_TF_DYING)) != 0 )
            {
                return;
            }

            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES:
                case SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES:
                    sithAIMove_FinishSpecialMove(pLocal, 4);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES:
                case SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES:
                    sithAIMove_FinishSpecialMove(pLocal, 8);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES:
                case SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES:
                    sithAIMove_FinishSpecialMove(pLocal, 16);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_TURNING_180_DEGREES:
                    sithAIMove_FinishSpecialMove(pLocal, 32);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_STRAFING_LEFT:
                case SITHPLAYERMOVE_STRAFING_RIGHT:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    sithAIMove_SetGoalReached(pLocal);
                    break;

                case SITHPLAYERMOVE_ROLLING_LEFT:
                case SITHPLAYERMOVE_ROLLING_RIGHT:
                    sithAIMove_FinishSpecialMove(pLocal, 1);
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    sithAIMove_SetGoalReached(pLocal);
                    break;

                case SITHPLAYERMOVE_MOUNTING_WALL:
                    sithAIMove_FinishSpecialMove(pLocal, 64);
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    break;

                case SITHPLAYERMOVE_STAND2WALK:
                case SITHPLAYERMOVE_STAND2RUN:
                    if ( pThing->moveStatus == SITHPLAYERMOVE_STAND2WALK )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    }
                    else
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
                    }
                    break;

                case SITHPLAYERMOVE_WALK2STAND:
                case SITHPLAYERMOVE_UNKNOWN_78:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;

                case SITHPLAYERMOVE_KNOCKEDOUT:
                    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    pThing->collide.type = SITH_COLLIDE_FACE;
                    pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_DISABLED;
                    pThing->thingInfo.actorInfo.flags &= ~(SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE);
                    break;

                case SITHPLAYERMOVE_LAND:
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                    break;

                default:
                    return;
            }

            switch ( pThing->moveStatus )
            {
                case SITHPLAYERMOVE_STILL:
                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
                    break;

                case SITHPLAYERMOVE_WALKING:
                    if ( pPhysics->velocity.x == 0.0f && pPhysics->velocity.y == 0.0f && pPhysics->velocity.z == 0.0f )
                    {
                        pPhysics->velocity.y = 0.06f;
                    }
                    else if ( rdVector_Len3(&pPhysics->velocity) < 0.06f )
                    {
                        pPhysics->velocity.x = RD_MULTIPLY(pPhysics->velocity.x, 1.06f);
                        pPhysics->velocity.y = RD_MULTIPLY(pPhysics->velocity.y, 1.06f);
                        pPhysics->velocity.z = RD_MULTIPLY(pPhysics->velocity.z, 1.06f);
                    }

                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_WALK, NULL);
                    break;

                case SITHPLAYERMOVE_RUNNING:
                    if ( pPhysics->velocity.x == 0.0f && pPhysics->velocity.y == 0.0f && pPhysics->velocity.z == 0.0f )
                    {
                        pPhysics->velocity.y = 0.27f;
                    }

                    if ( rdVector_Len3(&pPhysics->velocity) < 0.27f )
                    {
                        pPhysics->velocity.x = RD_MULTIPLY(pPhysics->velocity.x, 1.27f);
                        pPhysics->velocity.y = RD_MULTIPLY(pPhysics->velocity.y, 1.27f);
                        pPhysics->velocity.z = RD_MULTIPLY(pPhysics->velocity.z, 1.27f);
                    }

                    sithPuppet_SetSubMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);
                    break;
            }
            break;

        default:
            sithPuppet_DefaultCallback(pThing, trackNum, marker);
            break;
    }
}



void J3DAPI sithAIMove_AISetLookThing(SithAIControlBlock* pLocal, const SithThing* pThing)
{
    rdVector3 lookPos;
    rdVector3 eyeOffset;

#line 1947
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(pThing);

    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        rdMatrix_TransformVector34(&eyeOffset, &pThing->thingInfo.actorInfo.eyeOffset, &pThing->orient);
        rdVector_Add3(&lookPos, &pThing->pos, &eyeOffset);
    }
    else
    {
        rdVector_Copy3(&lookPos, &pThing->pos);
    }

    sithAIMove_AISetLookPos(pLocal, &lookPos);
}

void J3DAPI sithAIMove_AISetLookPos(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    SithThing* pOwner;
    SithActorInfo* pCurActorInfo;
    rdVector3 eyePos;
    float curdist;

#line 1989
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(targetPos);

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) != 0 )
    {
        return;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
#line 1999
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "WARNING!  Look target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName, 0, 0, 0);
    }

    pOwner = pLocal->pOwner;
    pCurActorInfo = &pOwner->thingInfo.actorInfo;
    rdVector_Copy3(&eyePos, &pOwner->pos);
    if ( pOwner->type == SITH_THING_ACTOR || pOwner->type == SITH_THING_PLAYER )
    {
        eyePos.z += pCurActorInfo->eyeOffset.z;
    }

    rdVector_Sub3(&pLocal->goalLVec, targetPos, &eyePos);
    curdist = rdVector_Normalize3Acc(&pLocal->goalLVec);
    if ( _isnan(curdist) || curdist < 0.001f )
    {
        rdVector_Copy3(&pLocal->goalLVec, &pOwner->orient.lvec);
        curdist = 0.0f;
    }

    rdVector_Copy3(&pLocal->lookPos, targetPos);
    if ( curdist != 0.0f )
    {
        pLocal->mode |= SITHAI_MODE_TURNING;
        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_10;
    }

    pLocal->mode |= SITHAI_MODE_UNKNOWN_80;
    if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_UNKNOWN_40000, NULL);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_10, 3) /* end count-fit */
void J3DAPI sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos)
{
    rdVector3 eyePos;
    SithThing* curOwner;
    rdVector3 curlookPos;
    float dist;

    curOwner = pLocal->pOwner;
    rdVector_Copy3(&curlookPos, targetPos);
    eyePos.x = 0.0f;
    eyePos.y = 0.0f;
    eyePos.z = curOwner->thingInfo.actorInfo.eyeOffset.z;
    rdMatrix_TransformVector34Acc(&eyePos, &curOwner->orient);
    rdVector_Add3Acc(&eyePos, &curOwner->pos);

    dist = rdMath_DistancePointToPlane(&curlookPos, &curOwner->orient.uvec, &eyePos);
    rdVector_ScaleAdd3Acc(&curlookPos, &curOwner->orient.uvec, -dist);
    sithAIMove_AISetLookPos(pLocal, &curlookPos);
}


int J3DAPI sithAIMove_AISetMovePos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
    rdVector3 startPos;
    rdVector3 homeDir;
    rdVector3 endPos;
    float dist;
#ifdef J3D_DEBUG
    SithThing* pTemplate;
    rdVector3 pos;
    rdMatrix34 orient;
    rdVector3 look;
#endif

#line 2103
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);
    SITH_ASSERTREL(moveToPos);

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
#line 2113
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "WARNING!  Move target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName, 0, 0, 0);
    }

    rdVector_Copy3(&endPos, moveToPos);
    rdVector_Copy3(&startPos, &pLocal->pOwner->pos);
    if ( pLocal->maxHomeDist > 0.0f )
    {
        rdVector_Sub3(&homeDir, moveToPos, &pLocal->homePos);
        dist = rdVector_Normalize3Acc(&homeDir);
        if ( dist > pLocal->maxHomeDist )
        {
            rdVector_ScaleAdd3(&endPos, &homeDir, pLocal->maxHomeDist, &pLocal->homePos);
        }
    }

    dist = (float)sithAIMove_GetGoalTolerance(pLocal, &startPos, &endPos);
    if ( sqrtf((moveToPos->x - pLocal->pOwner->pos.x) * (moveToPos->x - pLocal->pOwner->pos.x) + (moveToPos->y - pLocal->pOwner->pos.y) * (moveToPos->y - pLocal->pOwner->pos.y) + (moveToPos->z - pLocal->pOwner->pos.z) * (moveToPos->z - pLocal->pOwner->pos.z)) <= dist )
    {
        return 0;
    }

    pLocal->moveSpeed = moveSpeed;
    rdVector_Copy3(&pLocal->movePos, moveToPos);
    rdVector_Copy3(&pLocal->vecUnknown, &pLocal->pOwner->pos);
    sithSoundClass_PlayModeFirst(pLocal->pOwner, SITHSOUNDCLASS_MOVING);
    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_MOVING);
    if ( (pLocal->mode & SITHAI_MODE_WANTALLEVENTS) != 0 )
    {
        sithAI_EmitEvent(pLocal, SITHAI_EVENT_MOVE_TARGET_CHANGED, NULL);
    }

#ifdef J3D_DEBUG
    if ( sithAIMove_pMoveToDebugMarkThing )
    {
        sithThing_RemoveThing(sithWorld_pCurrentWorld, sithAIMove_pMoveToDebugMarkThing);
        sithAIMove_pMoveToDebugMarkThing = NULL;
    }

    if ( sithAIMove_pMoveToPosGroundMarkThing )
    {
        sithThing_RemoveThing(sithWorld_pCurrentWorld, sithAIMove_pMoveToPosGroundMarkThing);
        sithAIMove_pMoveToPosGroundMarkThing = NULL;
    }

    if ( sithAIMove_bDebugMoveToPos )
    {
        pTemplate = sithTemplate_GetTemplate("+x_mark");
        rdVector_Copy3(&pos, moveToPos);
        if ( pTemplate )
        {
            look.x = 0.0f;
            look.y = 0.0f;
            look.z = 1.0f;
            rdMatrix_BuildFromLook34(&orient, &look);
            sithAIMove_pMoveToDebugMarkThing = sithThing_CreateThingAtPos(pTemplate, &pos, &orient, pLocal->pOwner->pInSector, NULL);

            pos.z -= sithPhysics_GetThingHeight(pLocal->pOwner);
            pos.z += 0.001f;
            sithAIMove_pMoveToPosGroundMarkThing = sithThing_CreateThingAtPos(pTemplate, &pos, &orient, pLocal->pOwner->pInSector, NULL);
        }
    }
#endif

    return 1;
}


int J3DAPI sithAIMove_AISetMoveTargetPos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
#line 2220
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(moveToPos);

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
#line 2226
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "WARNING!  Move target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName, 0, 0, 0);
    }

    pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_1;
    if ( sithAIMove_AISetMovePos(pLocal, moveToPos, moveSpeed) )
    {
        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_1;
        return 1;
    }

    return 0;
}

/* Linker-discarded (no retail symbol or caller; debug build 0x004C4173, asserts at lines 2264/2265, print at 2274):
   moves to moveToPos, saving the current move target in vecUnknown3 for UpdateMove's SUBMODE_UNKNOWN_2 return.
   Body is the debug code's text. */
int J3DAPI sithAIMove_AISetMoveTargetPos2(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed)
{
#line 2264
    SITH_ASSERTREL(pLocal);
    SITH_ASSERTREL(moveToPos);

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_1) != 0 )
    {
        return 0;
    }

    if ( (pLocal->mode & SITHAI_MODE_DISABLED) != 0 && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
#line 2274
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "WARNING!  Move target issued to '%s'. AI is DISABLED.\n", pLocal->pOwner->aName, 0, 0, 0);
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_2) == 0 )
    {
        rdVector_Copy3(&pLocal->vecUnknown3, &pLocal->movePos);
        if ( sithAIMove_AISetMoveTargetPos(pLocal, moveToPos, moveSpeed) )
        {
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_2;
            return 1;
        }
    }

    return 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_11, 10) /* end count-fit */
int J3DAPI sithAIMove_StartSpecialMove(SithAIControlBlock* pLocal, int armedModeState)
{
    SithThing* pOwner;
    rdVector3 dir;
    int bHaveSecond;
    SithPuppetSubMode submode;
    SithPuppetSubMode submode2;
    int bHaveFirst;
    int bRight;
    int numTries;
    float angle;
    float dot;
    int bBlocked;
    int bWalk;
    float moveDist;
    rdVector3 halfPos;
    rdVector3 movePos;
    rdVector3 hitNorm;

    bHaveFirst = 0;
    bHaveSecond = 0;
#line 2324
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    pOwner = pLocal->pOwner;

    if ( (armedModeState & 1) != 0 )
    {
        if ( rdVector_Dot3(&pOwner->orient.lvec, &pLocal->toTarget) <= 0.4f )
        {
            return 0;
        }

        submode  = SITHPUPPETSUBMODE_HOPLEFT;
        submode2 = SITHPUPPETSUBMODE_HOPRIGHT;
    }
    else if ( (armedModeState & 2) != 0 )
    {
        if ( rdVector_Dot3(&pOwner->orient.lvec, &pLocal->toTarget) <= 0.4f )
        {
            return 0;
        }

        submode  = SITHPUPPETSUBMODE_STRAFELEFT;
        submode2 = SITHPUPPETSUBMODE_STRAFERIGHT;
    }
    else if ( (armedModeState & 4) != 0 )
    {
        submode  = SITHPUPPETSUBMODE_WHIPCLIMBUP;
        submode2 = SITHPUPPETSUBMODE_WHIPCLIMBDOWN;
    }
    else if ( (armedModeState & 8) != 0 )
    {
        submode  = SITHPUPPETSUBMODE_WHIPCLIMBLEFT;
        submode2 = SITHPUPPETSUBMODE_WHIPCLIMBRIGHT;
    }
    else if ( (armedModeState & 0x10) != 0 )
    {
        submode  = SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT;
        submode2 = SITHPUPPETSUBMODE_WHIPSWINGMOUNT;
    }
    else if ( (armedModeState & 0x20) != 0 )
    {
        submode     = SITHPUPPETSUBMODE_WHIPCLIMBMOUNT;
        bHaveSecond = 1;
    }
    else if ( (armedModeState & 0x40) != 0 )
    {
        submode     = SITHPUPPETSUBMODE_MOUNTWALL;
        bHaveSecond = 1;
    }
    else
    {
        return 0;
    }

    if ( !bHaveFirst )
    {
        bHaveFirst = pOwner->pPuppetClass->aModes[pOwner->pPuppetState->majorMode][submode].pKeyframe != NULL;
    }

    if ( !bHaveSecond )
    {
        bHaveSecond = pOwner->pPuppetClass->aModes[pOwner->pPuppetState->majorMode][submode2].pKeyframe != NULL;
    }

    if ( !bHaveFirst || !bHaveSecond )
    {
        return 0;
    }

    if ( (armedModeState & 3) != 0 )
    {
        moveDist = (armedModeState & 1) != 0 ? 0.21000001f : 0.08f;

        bRight = 1;
        if ( (armedModeState & 0x100) != 0 )
        {
            bRight = SITH_RAND() < 0.5f ? 0 : 1;
        }
        else if ( (armedModeState & 0x200) != 0 )
        {
            bRight = 0;
        }

        if ( !bRight )
        {
            rdVector_Neg3(&dir, &pOwner->orient.rvec);
        }
        else
        {
            rdVector_Copy3(&dir, &pOwner->orient.rvec);
        }

        rdVector_ScaleAdd3(&halfPos, &dir, moveDist * 0.5f, &pOwner->pos);
        rdVector_ScaleAdd3(&movePos, &dir, moveDist, &pOwner->pos);
    }
    else if ( (armedModeState & 0x1C) != 0 )
    {
        bRight = (armedModeState & 0x200) == 0;
        goto start_move;
    }
    else if ( (armedModeState & 0x20) != 0 )
    {
        goto start_move;
    }
    else
    {
        goto start_move;
    }

    numTries = 0;
    while ( (bBlocked = sithAIUtil_CheckPathToPoint(pOwner, &movePos, pOwner->collide.movesize, &moveDist, &hitNorm, 1, 0)) != 0 && (armedModeState & 3) != 0 )
    {
        if ( ++numTries < 2 )
        {
            // Note: the direction is flipped but movePos is not recomputed
            bRight = 1 - bRight;
            rdVector_Neg3Acc(&dir);
            continue;
        }

        return 0;
    }

    if ( (armedModeState & 1) != 0 && (bWalk = sithAIUtil_CanWalk(pLocal, &halfPos, NULL)) != 1 )
    {
        return 0;
    }

    if ( (armedModeState & 3) != 0 && (bWalk = sithAIUtil_CanWalk(pLocal, &movePos, NULL)) != 1 )
    {
        return 0;
    }

start_move:
    if ( (armedModeState & 2) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        if ( !bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_STRAFING_LEFT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_STRAFELEFT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_STRAFING_RIGHT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_STRAFERIGHT, sithAIMove_PuppetCallback);
        }

        if ( pLocal->pTargetThing )
        {
            sithAIMove_AISetLookThing(pLocal, pLocal->pTargetThing);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &movePos, 1.0f);
        return 1;
    }

    if ( (armedModeState & 1) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
        sithAIMove_ResetAILook(pLocal);
        if ( !bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_ROLLING_LEFT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_HOPLEFT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_ROLLING_RIGHT;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_HOPRIGHT, sithAIMove_PuppetCallback);
        }

        sithAIMove_AISetMoveTargetPos(pLocal, &movePos, 2.0f);
        return 1;
    }

    if ( (armedModeState & 4) != 0 )
    {
        if ( !bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBUP, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBDOWN, sithAIMove_PuppetCallback);
        }

        return 1;
    }

    if ( (armedModeState & 8) != 0 )
    {
        if ( !bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBLEFT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBRIGHT, sithAIMove_PuppetCallback);
        }

        return 1;
    }

    if ( (armedModeState & 0x10) != 0 )
    {
        if ( !bRight )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT, sithAIMove_PuppetCallback);
        }
        else
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPSWINGMOUNT, sithAIMove_PuppetCallback);
        }

        return 1;
    }

    if ( (armedModeState & 0x20) != 0 )
    {
        pLocal->pOwner->moveStatus = SITHPLAYERMOVE_TURNING_180_DEGREES;
        sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, sithAIMove_PuppetCallback);
        return 1;
    }

    if ( (armedModeState & 0x40) != 0 )
    {
        angle = rdMath_DeltaAngleNormalized(&pLocal->vecUnknown3, &pOwner->orient.uvec, &pOwner->orient.lvec);
        if ( J3DINRANGE(sithAIMove_Abs(angle), 80.0f, 100.0f) )
        {
            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_MOUNTWALL, sithAIMove_PuppetCallback);
            return 1;
        }

        if ( J3DINRANGE(sithAIMove_Abs(angle), 60.0f, 79.0f) )
        {
            rdVector_ScaleAdd3(&dir, &pLocal->vecUnknown3, -0.2f, &pOwner->orient.uvec);
            rdVector_Normalize3Acc(&dir);

            dot = rdMath_DistancePointToPlane(&pOwner->orient.lvec, &dir, &rdroid_zeroVector3);

            rdVector_ScaleAdd3(&movePos, &dir, -dot, &pOwner->orient.lvec);
            rdVector_Normalize3Acc(&movePos);

            // Note: the projected lvec is never used
            rdVector_Copy3(&pOwner->orient.uvec, &dir);
            rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
            rdVector_Normalize3Acc(&pOwner->orient.rvec);
            rdVector_Cross3(&pOwner->orient.lvec, &pOwner->orient.uvec, &pOwner->orient.rvec);

            pLocal->pOwner->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
            sithPuppet_SetSubMode(pLocal->pOwner, SITHPUPPETSUBMODE_MOUNTWALL, sithAIMove_PuppetCallback);
            return 1;
        }

        if ( sithAIMove_Abs(angle) > 40.0f )
        {
            sithAIMove_FinishSpecialMove(pLocal, 0x40);
            return 1;
        }
    }

    return 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_12, 1) /* end count-fit */
void J3DAPI sithAIMove_FinishSpecialMove(SithAIControlBlock* pLocal, int type)
{
    SithThing* pOwner;
    rdVector3 pyr;

    pOwner = pLocal->pOwner;
    switch ( type )
    {
        case 2:
            sithAIMove_StopAIMovement(pLocal);
            sithAIUtil_ApplyForce(pLocal, &pLocal->moveDirection, -0.05f);
            break;

        case 1:
            break;

        case 4:
        case 8:
        case 0x10:
        case 0x20:
            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_45_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 45.0f, 0.0f);
            }

            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_45_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, -45.0f, 0.0f);
            }

            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 90.0f, 0.0f);
            }

            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, -90.0f, 0.0f);
            }

            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_LEFT_135_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 135.0f, 0.0f);
            }

            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_RIGHT_135_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, -135.0f, 0.0f);
            }

            if ( pOwner->moveStatus == SITHPLAYERMOVE_TURNING_180_DEGREES )
            {
                rdVector_Set3(&pyr, 0.0f, 180.0f, 0.0f);
            }

            rdMatrix_PreRotate34(&pOwner->orient, &pyr);
            rdMatrix_Normalize34(&pOwner->orient);
            break;

        case 0x40:
            sithThing_DetachThing(pOwner);
            rdVector_Copy3(&pOwner->orient.uvec, &pLocal->vecUnknown3);
            pOwner->orient.lvec.x = D3DMultiply(pOwner->orient.uvec.y, pOwner->orient.rvec.z) - D3DMultiply(pOwner->orient.uvec.z, pOwner->orient.rvec.y);
            pOwner->orient.lvec.y = D3DMultiply(pOwner->orient.uvec.z, pOwner->orient.rvec.x) - D3DMultiply(pOwner->orient.uvec.x, pOwner->orient.rvec.z);
            pOwner->orient.lvec.z = D3DMultiply(pOwner->orient.uvec.x, pOwner->orient.rvec.y) - D3DMultiply(pOwner->orient.uvec.y, pOwner->orient.rvec.x);
            rdVector_Normalize3Acc(&pOwner->orient.lvec);
            sithPhysics_FindFloor(pOwner, 1);
            if ( pOwner->moveStatus == SITHPLAYERMOVE_MOUNTING_WALL )
            {
                sithCollision_MoveThing(pOwner, &pOwner->orient.lvec, pOwner->collide.movesize, 0);
            }
            break;
    }
}

void J3DAPI sithAIMove_AIJump(SithAIControlBlock* pLocal, rdVector3* movePos, float a3)
{
    int trackNum;

    rdVector_Copy3(&pLocal->movePos, movePos);
    pLocal->moveSpeed = 2.0f;
    trackNum = sithPuppet_PlayMode(pLocal->pOwner, SITHPUPPETSUBMODE_JUMPUP, NULL);
    if ( trackNum < 0 )
    {
        sithPlayerActions_Jump(pLocal->pOwner, a3, 0);
    }

    pLocal->msecPauseMoveUntil = sithTime_msecGameTime + 2000;
    pLocal->mode |= SITHAI_MODE_MOVING;
}

void J3DAPI sithAIMove_AIStop(SithAIControlBlock* pLocal)
{
#line 2724
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    sithAIMove_StopAIMovement(pLocal);
    sithAIMove_ResetAILook(pLocal);
    rdVector_Copy3(&pLocal->movePos, &pLocal->pOwner->pos);
    sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, NULL);
    sithAIMove_UpdateAIMove(pLocal);
}

void J3DAPI sithAIMove_SetGoalReached(SithAIControlBlock* pLocal)
{
#line 2744
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_REACHED, NULL);
    if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVED);
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_4000) != 0 )
    {
        return;
    }

    if ( (pLocal->mode & SITHAI_MODE_TRAVERSEWPNTS) != 0 )
    {
        return;
    }

    sithAIMove_StopAIMovement(pLocal);
    if ( !sithAIMove_AIGetMoveState(pLocal) )
    {
        sithAIMove_UpdateAIMove(pLocal);
    }
}

void J3DAPI sithAIMove_Unreachable(SithAIControlBlock* pLocal)
{
#line 2784
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    if ( (pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_BLOCKED);
        return;
    }

    sithAIMove_StopAIMovement(pLocal);
    sithAI_EmitEvent(pLocal, SITHAI_EVENT_GOAL_UNREACHABLE, NULL);
    sithAIMove_UpdateAIMove(pLocal);
}

void J3DAPI sithAIMove_StopAIMovement(SithAIControlBlock* pLocal)
{
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_MOVING);
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSMOTION) != 0 )
    {
        pLocal->mode |= SITHAI_MODE_MOVING;
        return;
    }

    sithSoundClass_StopMode(pLocal->pOwner, SITHSOUNDCLASS_MOVING);
    memset(&pLocal->pOwner->moveInfo.physics.velocity, 0, sizeof(rdVector3));
}

void J3DAPI sithAIMove_ResetAILook(SithAIControlBlock* pLocal)
{
    sithActor_SetHeadPYR(pLocal->pOwner, &rdroid_zeroVector3);
    rdVector_Copy3(&pLocal->goalLVec, &pLocal->pOwner->orient.lvec);
    sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TURNING);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_13, 24) /* end count-fit */
void J3DAPI sithAIMove_UpdateBoss(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pOwner;
    SithActorInfo* pActorInfo;
    rdVector3 dir;
    rdVector3 pyr;
    float dot;
    float dist;
    float angle;
    float rotStep;
    float rotLimit;
    rdVector3 goal;
    static float sithAIMove_flt_539D48 = 0.99f;
    static float sithAIMove_flt_539D4C = 1.05f;

    pOwner = pLocal->pOwner;
    pActorInfo = &pOwner->thingInfo.actorInfo;
    rdVector_Copy3(&goal, &pLocal->goalLVec);
    dot = sithAIMove_Abs(rdVector_Dot3(&pOwner->orient.uvec, &goal));
    if ( dot > sithAIMove_flt_539D48 )
    {
        pLocal->mode &= ~SITHAI_MODE_TURNING;
#line 2882
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "WARNING! AI '%s' looking straight up or down.\n", pOwner->aName, 0, 0, 0);
        return;
    }

    dist = rdMath_DistancePointToPlane(&goal, &pOwner->orient.uvec, &rdroid_zeroVector3);
    rdVector_ScaleAdd3(&dir, &pOwner->orient.uvec, -dist, &goal);
    rdVector_Normalize3Acc(&dir);

    angle = rdMath_DeltaAngleNormalized(&dir, &pOwner->orient.lvec, &pOwner->orient.uvec);
    rotStep = RD_MULTIPLY(RD_MULTIPLY(pActorInfo->maxRotVelocity, 1.0f), secDeltaTime);
    rotLimit = RD_MULTIPLY(rotStep, sithAIMove_flt_539D4C);
    if ( sithAIMove_Abs(angle) <= rotLimit )
    {
        rdVector_Copy3(&pOwner->orient.lvec, &dir);
        rdVector_Cross3(&pOwner->orient.rvec, &pOwner->orient.lvec, &pOwner->orient.uvec);
        rdVector_Normalize3Acc(&pOwner->orient.rvec);
        rdVector_Cross3(&pOwner->orient.uvec, &pOwner->orient.rvec, &pOwner->orient.lvec);
        pLocal->mode &= ~SITHAI_MODE_TURNING;
    }
    else
    {
        pyr.x = 0.0f;
        pyr.y = -RD_MULTIPLY(D3DVAL(angle < 0.0f ? -1 : 1), rotStep);
        pyr.z = 0.0f;
        rdMatrix_PreRotate34(&pOwner->orient, &pyr);
        rdMatrix_Normalize34(&pOwner->orient);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_14, 3) /* end count-fit */
void J3DAPI sithAIMove_UpdateHeadTrackTarget(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithActorInfo* pActorInfo;
    SithThing* pTarget;
    rdVector3 headPYR;
    rdVector3 eyePos;
    rdVector3 targetPos;
    rdVector3 aimDir;
    rdVector3 targetDir;
    rdVector3 projLookDir;
    rdVector3 pitchDir;
    float dist;
    rdVector3 lookDir;
    float dot;
    float lowThreshold;
    SithThing* pOwner;
    float highThreshold;
    rdVector3 projTargetDir;
    float yaw;
    float deltaYaw;
    float pitch;
    float deltaPitch;
    float yawRate;
    float pitchRate;
    static float sithAIMove_flt_539D50 = -0.2f;
    static float sithAIMove_flt_58546C = 0;

#line 2945
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    pOwner = pLocal->pOwner;
    pActorInfo = &pOwner->thingInfo.actorInfo;
    pTarget = pLocal->pTargetThing;
    rdVector_Copy3(&headPYR, &pActorInfo->headPYR);
    if ( (pActorInfo->flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return;
    }

    eyePos.x = 0.0f;
    eyePos.y = 0.0f;
    eyePos.z = pOwner->thingInfo.actorInfo.eyeOffset.z;
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    if ( pTarget && (pActorInfo->flags & SITH_AF_SEEINVISIBLE) == 0 )
    {
        if ( (pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER) && (pTarget->thingInfo.actorInfo.flags & SITH_AF_INVISIBLE) != 0 )
        {
            pTarget = NULL;
        }

        if ( (pActorInfo->flags & SITH_AF_BLIND) != 0 )
        {
            pTarget = NULL;
        }
    }

    if ( pTarget )
    {
        if ( pTarget->type == SITH_THING_ACTOR || pTarget->type == SITH_THING_PLAYER )
        {
            rdMatrix_TransformVector34(&targetPos, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
            rdVector_Add3Acc(&targetPos, &pTarget->pos);
        }
        else
        {
            rdVector_Copy3(&targetPos, &pTarget->pos);
        }

        rdVector_Sub3(&targetDir, &targetPos, &eyePos);
        rdVector_Normalize3Acc(&targetDir);
        dist = rdMath_DistancePointToPlane(&targetDir, &pOwner->orient.uvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&projTargetDir, &pOwner->orient.uvec, -dist, &targetDir);
        rdVector_Normalize3Acc(&projTargetDir);
    }

    rdVector_Copy3(&targetPos, &pLocal->lookPos);
    rdVector_Sub3(&lookDir, &targetPos, &eyePos);
    rdVector_Normalize3Acc(&lookDir);
    dist = rdMath_DistancePointToPlane(&lookDir, &pOwner->orient.uvec, &rdroid_zeroVector3);
    rdVector_ScaleAdd3(&projLookDir, &pOwner->orient.uvec, -dist, &lookDir);
    rdVector_Normalize3Acc(&projLookDir);

    lowThreshold = sithAIMove_flt_539D50;
    highThreshold = sithAIMove_flt_58546C;
    if ( !pTarget )
    {
        pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_800;
    }
    else if ( (pActorInfo->flags & SITH_AF_NOSLOPEMOVE) != 0 || (pLocal->mode & SITHAI_MODE_BLOCK) != 0 )
    {
        pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_800;
        lowThreshold = -1.0f;
        highThreshold = 1.0f;
    }
    else
    {
        dot = rdVector_Dot3(&pOwner->orient.lvec, &projTargetDir);
        if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_800) != 0 )
        {
            if ( dot < lowThreshold )
            {
                pLocal->submode &= ~SITHAI_SUBMODE_UNKNOWN_800;
            }
        }
        else if ( dot > highThreshold )
        {
            pLocal->submode |= SITHAI_SUBMODE_UNKNOWN_800;
        }
    }

    if ( (pLocal->submode & SITHAI_SUBMODE_UNKNOWN_800) != 0 )
    {
        rdVector_Copy3(&aimDir, &projTargetDir);
        rdVector_Copy3(&pitchDir, &targetDir);
    }
    else
    {
        rdVector_Copy3(&aimDir, &projLookDir);
        rdVector_Copy3(&pitchDir, &lookDir);
    }

    yawRate = RD_MULTIPLY(pActorInfo->maxHeadVelocity, secDeltaTime);
    pitchRate = RD_MULTIPLY(RD_MULTIPLY(pActorInfo->maxHeadVelocity, 0.5f), secDeltaTime);
    dot = rdVector_Dot3(&pOwner->orient.lvec, &aimDir);
    if ( dot >= lowThreshold )
    {
        yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &aimDir, &pOwner->orient.uvec);
    }
    else
    {
        yaw = 0.0f;
    }

    deltaYaw = yaw - pActorInfo->headPYR.y;
    deltaYaw = STDMATH_CLAMP(deltaYaw, -yawRate, yawRate);
    headPYR.y += deltaYaw;
    if ( headPYR.y <= -pActorInfo->maxHeadYaw || headPYR.y >= pActorInfo->maxHeadYaw )
    {
        headPYR.y = RD_MULTIPLY(pActorInfo->maxHeadYaw, D3DVAL(headPYR.y < 0.0f ? -1 : 1));
        pLocal->submode &= ~SITHAI_SUBMODE_HEADTRACKINGMOTION;
    }
    else
    {
        pLocal->submode |= SITHAI_SUBMODE_HEADTRACKINGMOTION;
    }

    if ( dot > 0.0f )
    {
        pitch = rdMath_DeltaAngleNormalized(&aimDir, &pitchDir, &pOwner->orient.rvec);
    }
    else
    {
        pitch = 0.0f;
    }

    deltaPitch = pitch - pActorInfo->headPYR.x;
    deltaPitch = STDMATH_CLAMP(deltaPitch, -pitchRate, pitchRate);
    headPYR.x = STDMATH_CLAMP(headPYR.x + deltaPitch, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);

    sithActor_SetHeadPYR(pLocal->pOwner, &headPYR);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_15, 21) /* end count-fit */
void J3DAPI sithAIMove_UpdateHeadTrackLookPos(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithActorInfo* pActorInfo;
    SithThing* pTarget;
    rdVector3 headPYR;
    rdVector3 lookPos;
    rdVector3 projDir;
    rdVector3 eyePos;
    float dist;
    float dot;
    float yaw;
    float deltaYaw;
    float pitch;
    float deltaPitch;
    SithThing* pOwner;
    float yawRate;
    float pitchRate;
    static float sithAIMove_flt_539D54 = 0.5f;
    rdVector3 lookDir;

#line 3161
    SITH_ASSERTREL(pLocal && pLocal->pOwner);
    pOwner = pLocal->pOwner;
    pActorInfo = &pOwner->thingInfo.actorInfo;
    pTarget = pLocal->pTargetThing;
    rdVector_Copy3(&headPYR, &pActorInfo->headPYR);
    rdVector_Copy3(&lookPos, &pLocal->lookPos);
    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        return;
    }

    if ( (pOwner->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    eyePos.x = 0.0f;
    eyePos.y = 0.0f;
    eyePos.z = pOwner->thingInfo.actorInfo.eyeOffset.z;
    rdMatrix_TransformVector34Acc(&eyePos, &pOwner->orient);
    rdVector_Add3Acc(&eyePos, &pOwner->pos);

    rdVector_Sub3(&lookDir, &lookPos, &eyePos);
    rdVector_Normalize3Acc(&lookDir);
    dist = rdMath_DistancePointToPlane(&lookDir, &pOwner->orient.uvec, &rdroid_zeroVector3);
    rdVector_ScaleAdd3(&projDir, &pOwner->orient.uvec, -dist, &lookDir);
    rdVector_Normalize3Acc(&projDir);

    yawRate = RD_MULTIPLY(pActorInfo->maxHeadVelocity, secDeltaTime);
    pitchRate = RD_MULTIPLY(RD_MULTIPLY(pActorInfo->maxHeadVelocity, 0.5f), secDeltaTime);

    yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &projDir, &pOwner->orient.uvec);
    deltaYaw = yaw - pActorInfo->headPYR.y;
    deltaYaw = STDMATH_CLAMP(deltaYaw, -yawRate, yawRate);
    headPYR.y = STDMATH_CLAMP(headPYR.y + deltaYaw, -pActorInfo->maxHeadYaw, pActorInfo->maxHeadYaw);

    dot = rdVector_Dot3(&pOwner->orient.lvec, &projDir);
    if ( dot > 0.0f )
    {
        pitch = rdMath_DeltaAngleNormalized(&projDir, &lookDir, &pOwner->orient.rvec);
    }
    else
    {
        pitch = 0.0f;
    }

    deltaPitch = pitch - pActorInfo->headPYR.x;
    deltaPitch = STDMATH_CLAMP(deltaPitch, -pitchRate, pitchRate);
    headPYR.x = STDMATH_CLAMP(headPYR.x + deltaPitch, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);

    sithActor_SetHeadPYR(pLocal->pOwner, &headPYR);

    if ( sithAIMove_Abs(deltaYaw) <= sithAIMove_flt_539D54 && sithAIMove_Abs(deltaPitch) <= sithAIMove_flt_539D54 )
    {
        pLocal->mode &= ~SITHAI_MODE_UNKNOWN_80;
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_16, 14) /* end count-fit */

void J3DAPI sithAIMove_GetQuetzStrikeAngles(SithAIControlBlock* pLocal, float* pYaw, float* pPitch, float* pHeadYaw, float* pHeadPitch)
{
    SithThing* pOwner;
    SithThing* pTarget;
    rdVector3 ownerEye;
    rdVector3 targetEye;
    rdVector3 lookDir;
    float dist;
    float yaw;
    float distance;
    float pitchScale;
    float headPitchScale;
    static float sithAIMove_flt_539D58 = 0.3f;
    static float sithAIMove_flt_539D5C = 0.5f;
    static float sithAIMove_flt_539D60 = -15.0f;
    static float sithAIMove_flt_539D64 = -60.0f;
    static float sithAIMove_flt_539D68 = 60.0f;
    static float sithAIMove_flt_585470 = 0;
    rdVector3 dir;

    pOwner = pLocal->pOwner;
    pTarget = pLocal->pTargetThing;
    rdVector_Copy3(&ownerEye, &pOwner->pos);
    ownerEye.z += pOwner->thingInfo.actorInfo.eyeOffset.z;

    rdMatrix_TransformVector34(&targetEye, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
    rdVector_Add3Acc(&targetEye, &pTarget->pos);
    if ( (pTarget->moveInfo.physics.flags & SITH_PF_CROUCHING) != 0 )
    {
        targetEye.z -= 0.1f;
    }

    rdVector_Sub3(&dir, &targetEye, &ownerEye);
    rdVector_Normalize3Acc(&dir);

    dist = rdMath_DistancePointToPlane(&dir, &pOwner->orient.uvec, &rdroid_zeroVector3);
    rdVector_ScaleAdd3(&lookDir, &pOwner->orient.uvec, -dist, &dir);
    rdVector_Normalize3Acc(&lookDir);

    yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &lookDir, &pOwner->orient.uvec);
    *pYaw = yaw;
    *pHeadYaw = -pOwner->renderData.apTweakedAngles[2].y;

    distance = sqrtf((pOwner->pos.x - pTarget->pos.x) * (pOwner->pos.x - pTarget->pos.x) + (pOwner->pos.y - pTarget->pos.y) * (pOwner->pos.y - pTarget->pos.y));
    distance = STDMATH_CLAMP(distance, sithAIMove_flt_539D58, sithAIMove_flt_539D5C);

    pitchScale = (sithAIMove_flt_539D64 - sithAIMove_flt_539D60) / (sithAIMove_flt_539D5C - sithAIMove_flt_539D58);
    *pPitch = RD_MULTIPLY(distance - sithAIMove_flt_539D58, pitchScale) + sithAIMove_flt_539D60;
    headPitchScale = D3DVAL((sithAIMove_flt_539D68 - sithAIMove_flt_585470) / (sithAIMove_flt_539D5C - sithAIMove_flt_539D58));
    *pHeadPitch = RD_MULTIPLY(distance - sithAIMove_flt_539D58, headPitchScale) + sithAIMove_flt_585470 - pOwner->renderData.apTweakedAngles[2].x;
}


void J3DAPI sithAIMove_UpdateQuetzStrike(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    float timeElapsed;
    SithQuetzSnapShot* pSnapShot;
    SithQuetzSnapShot* pPrevSnapShot;
    float angleHeadPitch;
    float remainder;
    SithQuetzStrike* pStrike;
    float ratio;
    float headYaw;
    float pitchAngle;
    float angle;
    float length;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->mode != 1 )
    {
        return;
    }

    pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    remainder     = pSnapShot->time - pStrike->curTime;
    if ( pStrike->snapShot == 1 )
    {
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[1].angle1 = 20.0f;
        pStrike->aSnapShots[1].angle2 = RD_MULTIPLY(angle, 0.5f);
        pStrike->aSnapShots[1].angle3 = pThing->renderData.apTweakedAngles[2].pitch - 20.0f;
        pStrike->aSnapShots[1].angle4 = pThing->renderData.apTweakedAngles[2].yaw + RD_MULTIPLY(headYaw, 0.5f);
        pStrike->aSnapShots[1].angle5 = pStrike->aSnapShots[0].angle5;
    }
    else if ( pStrike->snapShot == 2 )
    {
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[2].angle1 = RD_MULTIPLY(pitchAngle, 0.9f);
        pStrike->aSnapShots[2].angle2 = RD_MULTIPLY(angle, 0.9f);
        pStrike->aSnapShots[2].angle3 = pThing->renderData.apTweakedAngles[2].pitch + RD_MULTIPLY(angleHeadPitch, 0.9f);
        pStrike->aSnapShots[2].angle4 = pThing->renderData.apTweakedAngles[2].yaw + RD_MULTIPLY(headYaw, 0.9f);
        pStrike->aSnapShots[2].angle5 = -30.0f;
    }
    else if ( pStrike->snapShot == 3 )
    {
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[3].angle1 = pitchAngle;
        pStrike->aSnapShots[3].angle2 = angle;
        pStrike->aSnapShots[3].angle3 = pThing->renderData.apTweakedAngles[2].pitch + angleHeadPitch;
        pStrike->aSnapShots[3].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw;
        pStrike->aSnapShots[3].angle5 = 20.0f;
    }
    else if ( pStrike->snapShot == 4 )
    {
        sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[4].angle1 = 10.0f;
        pStrike->aSnapShots[4].angle2 = RD_MULTIPLY(angle, 0.4f);
        pStrike->aSnapShots[4].angle3 = pThing->renderData.apTweakedAngles[2].pitch - 10.0f;
        pStrike->aSnapShots[4].angle4 = pThing->renderData.apTweakedAngles[2].yaw + RD_MULTIPLY(headYaw, 0.4f);
        pStrike->aSnapShots[4].angle5 = pStrike->aSnapShots[0].angle5;
    }
    else if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( remainder >= secDeltaTime )
    {
#line 3390
        SITH_ASSERTREL(pStrike->snapShot != 0);
        pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;
        length = pSnapShot->time - pPrevSnapShot->time;
        timeElapsed = pStrike->curTime - pPrevSnapShot->time;
        ratio = timeElapsed / length;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = RD_MULTIPLY(pSnapShot->angle1 - pPrevSnapShot->angle1, ratio) + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = RD_MULTIPLY(pSnapShot->angle2 - pPrevSnapShot->angle2, ratio) + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = RD_MULTIPLY(pSnapShot->angle3 - pPrevSnapShot->angle3, ratio) + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = RD_MULTIPLY(pSnapShot->angle4 - pPrevSnapShot->angle4, ratio) + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = RD_MULTIPLY(pSnapShot->angle5 - pPrevSnapShot->angle5, ratio) + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzStrike(pLocal, secDeltaTime - remainder);
        }
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithAIMove_countfit_17, 17) /* end count-fit */
void J3DAPI sithAIMove_GetQuetzAnim2Angles(SithAIControlBlock* pLocal, float* angle, float* a3, float* a4, float* a5)
{
    rdVector3 targetPos;
    SithThing* pOwner;
    SithThing* pTarget;
    rdVector3 pos;
    rdVector3 planeDir;
    float dist;
    float yaw;
    rdVector3 dir;

    pOwner  = pLocal->pOwner;
    pTarget = pLocal->pTargetThing;
    rdVector_Copy3(&pos, &pOwner->pos);
    pos.z += pOwner->thingInfo.actorInfo.eyeOffset.z;

    rdMatrix_TransformVector34(&targetPos, &pTarget->thingInfo.actorInfo.eyeOffset, &pTarget->orient);
    rdVector_Add3Acc(&targetPos, &pTarget->pos);

    rdVector_Sub3(&dir, &targetPos, &pos);
    rdVector_Normalize3Acc(&dir);

    dist = rdMath_DistancePointToPlane(&dir, &pOwner->orient.uvec, &rdroid_zeroVector3);
    rdVector_ScaleAdd3(&planeDir, &pOwner->orient.uvec, -dist, &dir);
    rdVector_Normalize3Acc(&planeDir);

    yaw = rdMath_DeltaAngleNormalized(&pOwner->orient.lvec, &planeDir, &pOwner->orient.uvec);
    *angle = yaw;
    *a4    = -pOwner->renderData.apTweakedAngles[2].yaw;
    *a5    = 50.0f;
    *a3    = -30.0f;
}

void J3DAPI sithAIMove_UpdateQuetzAnim2(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    float timeElapsed;
    SithQuetzSnapShot* pSnapShot;
    SithQuetzSnapShot* pPrevSnapShot;
    float angleHeadPitch;
    float remainder;
    SithQuetzStrike* pStrike;
    float ratio;
    float headYaw;
    float pitchAngle;
    float angle;
    float length;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->mode != 2 )
    {
        return;
    }

    pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    remainder     = pSnapShot->time - pStrike->curTime;
    if ( pStrike->snapShot == 1 )
    {
        sithAIMove_GetQuetzAnim2Angles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[1].angle1 = 20.0f;
        pStrike->aSnapShots[1].angle2 = RD_MULTIPLY(angle, 0.5f);
        pStrike->aSnapShots[1].angle3 = pThing->renderData.apTweakedAngles[2].pitch + 10.0f;
        pStrike->aSnapShots[1].angle4 = pThing->renderData.apTweakedAngles[2].yaw + RD_MULTIPLY(headYaw, 0.5f);
        pStrike->aSnapShots[1].angle5 = pStrike->aSnapShots[0].angle5;
    }
    else if ( pStrike->snapShot == 2 )
    {
        sithAIMove_GetQuetzAnim2Angles(pLocal, &angle, &pitchAngle, &headYaw, &angleHeadPitch);
        pStrike->aSnapShots[2].angle1 = pitchAngle;
        pStrike->aSnapShots[2].angle2 = angle;
        pStrike->aSnapShots[2].angle3 = pThing->renderData.apTweakedAngles[2].pitch + angleHeadPitch;
        pStrike->aSnapShots[2].angle4 = pThing->renderData.apTweakedAngles[2].yaw + headYaw;
        pStrike->aSnapShots[2].angle5 = -70.0f;
    }
    else if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( remainder >= secDeltaTime )
    {
#line 3528
        SITH_ASSERTREL(pStrike->snapShot != 0);
        pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;
        length = pSnapShot->time - pPrevSnapShot->time;
        timeElapsed = pStrike->curTime - pPrevSnapShot->time;
        ratio = timeElapsed / length;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = RD_MULTIPLY(pSnapShot->angle1 - pPrevSnapShot->angle1, ratio) + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = RD_MULTIPLY(pSnapShot->angle2 - pPrevSnapShot->angle2, ratio) + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = RD_MULTIPLY(pSnapShot->angle3 - pPrevSnapShot->angle3, ratio) + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = RD_MULTIPLY(pSnapShot->angle4 - pPrevSnapShot->angle4, ratio) + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = RD_MULTIPLY(pSnapShot->angle5 - pPrevSnapShot->angle5, ratio) + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim2(pLocal, secDeltaTime - remainder);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzAnim3(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    float timeElapsed;
    SithQuetzSnapShot* pSnapShot;
    SithQuetzSnapShot* pPrevSnapShot;
    float timeLeft;
    float period;
    SithQuetzStrike* pStrike;
    float ratio;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->mode != 3 )
    {
        return;
    }

    pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    timeLeft     = pSnapShot->time - pStrike->curTime;
    if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( timeLeft >= secDeltaTime )
    {
#line 3602
        SITH_ASSERTREL(pStrike->snapShot != 0);
        pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;
        period = pSnapShot->time - pPrevSnapShot->time;
        timeElapsed = pStrike->curTime - pPrevSnapShot->time;
        ratio = timeElapsed / period;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = RD_MULTIPLY(pSnapShot->angle1 - pPrevSnapShot->angle1, ratio) + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = RD_MULTIPLY(pSnapShot->angle2 - pPrevSnapShot->angle2, ratio) + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = RD_MULTIPLY(pSnapShot->angle3 - pPrevSnapShot->angle3, ratio) + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = RD_MULTIPLY(pSnapShot->angle4 - pPrevSnapShot->angle4, ratio) + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = RD_MULTIPLY(pSnapShot->angle5 - pPrevSnapShot->angle5, ratio) + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim3(pLocal, secDeltaTime - timeLeft);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzAnim5(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    float timeElapsed;
    SithQuetzSnapShot* pSnapShot;
    SithQuetzSnapShot* pPrevSnapShot;
    float timeLeft;
    float period;
    SithQuetzStrike* pStrike;
    float ratio;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->mode != 5 )
    {
        return;
    }

    pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    timeLeft     = pSnapShot->time - pStrike->curTime;
    if ( pStrike->snapShot == pStrike->numSnapShots - 1 )
    {
        pSnapShot->angle3 = pThing->renderData.apTweakedAngles[2].pitch;
        pSnapShot->angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    }

    if ( timeLeft >= secDeltaTime )
    {
#line 3677
        SITH_ASSERTREL(pStrike->snapShot != 0);
        pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;
        period = pSnapShot->time - pPrevSnapShot->time;
        timeElapsed = pStrike->curTime - pPrevSnapShot->time;
        ratio = timeElapsed / period;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = RD_MULTIPLY(pSnapShot->angle1 - pPrevSnapShot->angle1, ratio) + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = RD_MULTIPLY(pSnapShot->angle2 - pPrevSnapShot->angle2, ratio) + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = RD_MULTIPLY(pSnapShot->angle3 - pPrevSnapShot->angle3, ratio) + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = RD_MULTIPLY(pSnapShot->angle4 - pPrevSnapShot->angle4, ratio) + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = RD_MULTIPLY(pSnapShot->angle5 - pPrevSnapShot->angle5, ratio) + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim5(pLocal, secDeltaTime - timeLeft);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzAnim4(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    float timeElapsed;
    SithQuetzSnapShot* pSnapShot;
    SithQuetzSnapShot* pPrevSnapShot;
    float timeLeft;
    float period;
    SithQuetzStrike* pStrike;
    float ratio;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    if ( pStrike->mode != 4 )
    {
        return;
    }

    pSnapShot = &pStrike->aSnapShots[pStrike->snapShot];
    timeLeft     = pSnapShot->time - pStrike->curTime;
    if ( timeLeft >= secDeltaTime )
    {
#line 3742
        SITH_ASSERTREL(pStrike->snapShot != 0);
        pPrevSnapShot = &pStrike->aSnapShots[pStrike->snapShot - 1];
        pStrike->curTime += secDeltaTime;
        period = pSnapShot->time - pPrevSnapShot->time;
        timeElapsed = pStrike->curTime - pPrevSnapShot->time;
        ratio = timeElapsed / period;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        pThing->renderData.apTweakedAngles[1].pitch  = RD_MULTIPLY(pSnapShot->angle1 - pPrevSnapShot->angle1, ratio) + pPrevSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = RD_MULTIPLY(pSnapShot->angle2 - pPrevSnapShot->angle2, ratio) + pPrevSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = RD_MULTIPLY(pSnapShot->angle3 - pPrevSnapShot->angle3, ratio) + pPrevSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = RD_MULTIPLY(pSnapShot->angle4 - pPrevSnapShot->angle4, ratio) + pPrevSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = RD_MULTIPLY(pSnapShot->angle5 - pPrevSnapShot->angle5, ratio) + pPrevSnapShot->angle5;
    }
    else
    {
        pStrike->snapShot++;
        pStrike->curTime = pSnapShot->time;
        pThing->renderData.apTweakedAngles[1].pitch  = pSnapShot->angle1;
        pThing->renderData.apTweakedAngles[1].yaw    = pSnapShot->angle2;
        pThing->renderData.apTweakedAngles[2].pitch  = pSnapShot->angle3;
        pThing->renderData.apTweakedAngles[2].yaw    = pSnapShot->angle4;
        pThing->renderData.apTweakedAngles[28].pitch = pSnapShot->angle5;
        if ( pStrike->snapShot == pStrike->numSnapShots )
        {
            pStrike->mode = 0;
        }
        else
        {
            sithAIMove_UpdateQuetzAnim4(pLocal, secDeltaTime - timeLeft);
        }
    }
}

void J3DAPI sithAIMove_UpdateQuetzTail(SithAIControlBlock* pLocal, float secDeltaTime)
{
    SithThing* pThing;
    SithQuetzStrike* pStrike;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    switch ( pStrike->mode )
    {
        case 1:
            sithAIMove_UpdateQuetzStrike(pLocal, secDeltaTime);
            break;

        case 2:
            sithAIMove_UpdateQuetzAnim2(pLocal, secDeltaTime);
            break;

        case 3:
            sithAIMove_UpdateQuetzAnim3(pLocal, secDeltaTime);
            break;

        case 5:
            sithAIMove_UpdateQuetzAnim5(pLocal, secDeltaTime);
            break;

        case 4:
            sithAIMove_UpdateQuetzAnim4(pLocal, secDeltaTime);
            break;
    }
}

void J3DAPI sithAIMove_StartQuetzStrike(SithAIControlBlock* pLocal)
{
    SithThing* pThing;
    SithQuetzStrike* pStrike;
    float neckPitch;
    SithThing* pTarget;
    float a3;
    float headYaw;
    float angle;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    pTarget = pLocal->pTargetThing;
    sithAIMove_GetQuetzStrikeAngles(pLocal, &angle, &a3, &headYaw, &neckPitch);

    pStrike->curTime = 0.0f;
    pStrike->snapShot = 0;
    pStrike->aSnapShots[0].time = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 0.11f;
    pStrike->aSnapShots[2].time = 0.24f;
    pStrike->aSnapShots[3].time = 0.3f;
    pStrike->aSnapShots[4].time = 0.5f;

    pStrike->aSnapShots[5].time = 0.7f;
    pStrike->aSnapShots[5].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[5].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[5].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[5].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[5].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode = 1;
    pStrike->numSnapShots = 6;
}

void J3DAPI sithAIMove_StartQuetzAnim2(SithAIControlBlock* pLocal)
{
    SithThing* pThing;
    SithQuetzStrike* pStrike;
    SithThing* pTarget;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    pTarget = pLocal->pTargetThing;

    pStrike->curTime = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 0.11f;

    pStrike->aSnapShots[2].time = 0.24f;

    pStrike->aSnapShots[3].time = 0.5f;
    pStrike->aSnapShots[3].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[3].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[3].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[3].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[3].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode = 2;
    pStrike->numSnapShots = 4;
}

void J3DAPI sithAIMove_StartQuetzAnim3(SithAIControlBlock* pLocal)
{
    SithThing* pThing;
    SithQuetzStrike* pStrike;
    SithThing* pTarget;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    pTarget = pLocal->pTargetThing;

    pStrike->curTime = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 0.1f;
    pStrike->aSnapShots[1].angle1 = -10.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 50.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -40.0f;

    pStrike->aSnapShots[2].time = 0.3f;
    pStrike->aSnapShots[2].angle1 = -15.0f;
    pStrike->aSnapShots[2].angle2 = 30.0f;
    pStrike->aSnapShots[2].angle3 = 60.0f;
    pStrike->aSnapShots[2].angle4 = 50.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->aSnapShots[3].time = 0.6f;
    pStrike->aSnapShots[3].angle1 = -12.0f;
    pStrike->aSnapShots[3].angle2 = -30.0f;
    pStrike->aSnapShots[3].angle3 = 55.0f;
    pStrike->aSnapShots[3].angle4 = -50.0f;
    pStrike->aSnapShots[3].angle5 = -50.0f;

    pStrike->aSnapShots[4].time = 0.9f;
    pStrike->aSnapShots[4].angle1 = -15.0f;
    pStrike->aSnapShots[4].angle2 = 30.0f;
    pStrike->aSnapShots[4].angle3 = 60.0f;
    pStrike->aSnapShots[4].angle4 = 50.0f;
    pStrike->aSnapShots[4].angle5 = -35.0f;

    pStrike->aSnapShots[5].time = 1.2f;
    pStrike->aSnapShots[5].angle1 = -12.0f;
    pStrike->aSnapShots[5].angle2 = -30.0f;
    pStrike->aSnapShots[5].angle3 = 55.0f;
    pStrike->aSnapShots[5].angle4 = -50.0f;
    pStrike->aSnapShots[5].angle5 = -50.0f;

    pStrike->aSnapShots[6].time = 1.5f;
    pStrike->aSnapShots[6].angle1 = -15.0f;
    pStrike->aSnapShots[6].angle2 = 30.0f;
    pStrike->aSnapShots[6].angle3 = 60.0f;
    pStrike->aSnapShots[6].angle4 = 50.0f;
    pStrike->aSnapShots[6].angle5 = -60.0f;

    pStrike->aSnapShots[7].time = 1.8f;
    pStrike->aSnapShots[7].angle1 = -12.0f;
    pStrike->aSnapShots[7].angle2 = -30.0f;
    pStrike->aSnapShots[7].angle3 = 55.0f;
    pStrike->aSnapShots[7].angle4 = -50.0f;
    pStrike->aSnapShots[7].angle5 = -50.0f;

    pStrike->aSnapShots[8].time = 2.1f;
    pStrike->aSnapShots[8].angle1 = -15.0f;
    pStrike->aSnapShots[8].angle2 = 30.0f;
    pStrike->aSnapShots[8].angle3 = 60.0f;
    pStrike->aSnapShots[8].angle4 = 50.0f;
    pStrike->aSnapShots[8].angle5 = -35.0f;

    pStrike->aSnapShots[9].time = 2.4f;
    pStrike->aSnapShots[9].angle1 = -12.0f;
    pStrike->aSnapShots[9].angle2 = -30.0f;
    pStrike->aSnapShots[9].angle3 = 55.0f;
    pStrike->aSnapShots[9].angle4 = -50.0f;
    pStrike->aSnapShots[9].angle5 = -50.0f;

    pStrike->aSnapShots[10].time = 2.7f;
    pStrike->aSnapShots[10].angle1 = -15.0f;
    pStrike->aSnapShots[10].angle2 = 30.0f;
    pStrike->aSnapShots[10].angle3 = 60.0f;
    pStrike->aSnapShots[10].angle4 = 50.0f;
    pStrike->aSnapShots[10].angle5 = -60.0f;

    pStrike->aSnapShots[11].time = 3.0f;
    pStrike->aSnapShots[11].angle1 = -12.0f;
    pStrike->aSnapShots[11].angle2 = -30.0f;
    pStrike->aSnapShots[11].angle3 = 55.0f;
    pStrike->aSnapShots[11].angle4 = -50.0f;
    pStrike->aSnapShots[11].angle5 = -50.0f;

    pStrike->aSnapShots[12].time = 3.3f;
    pStrike->aSnapShots[12].angle1 = -10.0f;
    pStrike->aSnapShots[12].angle2 = 0.0f;
    pStrike->aSnapShots[12].angle3 = 20.0f;
    pStrike->aSnapShots[12].angle4 = 0.0f;
    pStrike->aSnapShots[12].angle5 = -15.0f;

    pStrike->aSnapShots[13].time = 3.7f;
    pStrike->aSnapShots[13].angle1 = 0.0f;
    pStrike->aSnapShots[13].angle2 = 0.0f;
    pStrike->aSnapShots[13].angle3 = 5.0f;
    pStrike->aSnapShots[13].angle4 = 0.0f;
    pStrike->aSnapShots[13].angle5 = -10.0f;

    pStrike->aSnapShots[14].time = 5.0f;
    pStrike->aSnapShots[14].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[14].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[14].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[14].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[14].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode = 3;
    pStrike->numSnapShots = 15;
}

void J3DAPI sithAIMove_StartQuetzAnim5(SithAIControlBlock* pLocal)
{
    SithThing* pThing;
    SithQuetzStrike* pStrike;
    SithThing* pTarget;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    pTarget = pLocal->pTargetThing;

    pStrike->curTime = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 0.1f;
    pStrike->aSnapShots[1].angle1 = -10.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 50.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -40.0f;

    pStrike->aSnapShots[2].time = 0.3f;
    pStrike->aSnapShots[2].angle1 = -15.0f;
    pStrike->aSnapShots[2].angle2 = 30.0f;
    pStrike->aSnapShots[2].angle3 = 60.0f;
    pStrike->aSnapShots[2].angle4 = 50.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->aSnapShots[3].time = 0.6f;
    pStrike->aSnapShots[3].angle1 = -12.0f;
    pStrike->aSnapShots[3].angle2 = -30.0f;
    pStrike->aSnapShots[3].angle3 = 55.0f;
    pStrike->aSnapShots[3].angle4 = -50.0f;
    pStrike->aSnapShots[3].angle5 = -50.0f;

    pStrike->aSnapShots[4].time = 0.9f;
    pStrike->aSnapShots[4].angle1 = -15.0f;
    pStrike->aSnapShots[4].angle2 = 30.0f;
    pStrike->aSnapShots[4].angle3 = 60.0f;
    pStrike->aSnapShots[4].angle4 = 50.0f;
    pStrike->aSnapShots[4].angle5 = -35.0f;

    pStrike->aSnapShots[5].time = 1.2f;
    pStrike->aSnapShots[5].angle1 = -12.0f;
    pStrike->aSnapShots[5].angle2 = -30.0f;
    pStrike->aSnapShots[5].angle3 = 55.0f;
    pStrike->aSnapShots[5].angle4 = -50.0f;
    pStrike->aSnapShots[5].angle5 = -50.0f;

    pStrike->aSnapShots[6].time = 1.5f;
    pStrike->aSnapShots[6].angle1 = -15.0f;
    pStrike->aSnapShots[6].angle2 = 30.0f;
    pStrike->aSnapShots[6].angle3 = 60.0f;
    pStrike->aSnapShots[6].angle4 = 50.0f;
    pStrike->aSnapShots[6].angle5 = -60.0f;

    pStrike->aSnapShots[7].time = 1.8f;
    pStrike->aSnapShots[7].angle1 = -12.0f;
    pStrike->aSnapShots[7].angle2 = -30.0f;
    pStrike->aSnapShots[7].angle3 = 55.0f;
    pStrike->aSnapShots[7].angle4 = -50.0f;
    pStrike->aSnapShots[7].angle5 = -50.0f;

    pStrike->aSnapShots[8].time = 2.1f;
    pStrike->aSnapShots[8].angle1 = -15.0f;
    pStrike->aSnapShots[8].angle2 = 30.0f;
    pStrike->aSnapShots[8].angle3 = 60.0f;
    pStrike->aSnapShots[8].angle4 = 50.0f;
    pStrike->aSnapShots[8].angle5 = -35.0f;

    pStrike->aSnapShots[9].time = 3.0f;
    pStrike->aSnapShots[9].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[9].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[9].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[9].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[9].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->mode = 5;
    pStrike->numSnapShots = 10;
}

void J3DAPI sithAIMove_StartQuetzAnim4(SithAIControlBlock* pLocal)
{
    SithThing* pThing;
    SithQuetzStrike* pStrike;
    SithThing* pTarget;

    pThing = pLocal->pOwner;
    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    pStrike = &pThing->userblock.pQuetz->strike;
    pTarget = pLocal->pTargetThing;

    pStrike->curTime = 0.0f;
    pStrike->snapShot = 0;

    pStrike->aSnapShots[0].time = 0.0f;
    pStrike->aSnapShots[0].angle1 = pThing->renderData.apTweakedAngles[1].pitch;
    pStrike->aSnapShots[0].angle2 = pThing->renderData.apTweakedAngles[1].yaw;
    pStrike->aSnapShots[0].angle3 = pThing->renderData.apTweakedAngles[2].pitch;
    pStrike->aSnapShots[0].angle4 = pThing->renderData.apTweakedAngles[2].yaw;
    pStrike->aSnapShots[0].angle5 = pThing->renderData.apTweakedAngles[28].pitch;

    pStrike->aSnapShots[1].time = 1.6f;
    pStrike->aSnapShots[1].angle1 = 0.0f;
    pStrike->aSnapShots[1].angle2 = 0.0f;
    pStrike->aSnapShots[1].angle3 = 0.0f;
    pStrike->aSnapShots[1].angle4 = 0.0f;
    pStrike->aSnapShots[1].angle5 = -60.0f;

    pStrike->aSnapShots[2].time = 5.0f;
    pStrike->aSnapShots[2].angle1 = 0.0f;
    pStrike->aSnapShots[2].angle2 = 0.0f;
    pStrike->aSnapShots[2].angle3 = 0.0f;
    pStrike->aSnapShots[2].angle4 = 0.0f;
    pStrike->aSnapShots[2].angle5 = -60.0f;

    pStrike->mode = 4;
    pStrike->numSnapShots = 3;
}

void J3DAPI sithAIMove_UpdateMardukTail(SithThing* pThing, float secDeltaTime)
{
    int jointIndex;
    float angle;
    static float sithAIMove_flt_539D6C = 720.0f;

    if ( stdUtil_StrCmp(pThing->aName, "marduk") == 0 || stdUtil_StrCmp(pThing->aName, "marduk2") == 0 )
    {
        jointIndex = sithThing_GetThingJointIndex(pThing, "wmabdomen");
        if ( jointIndex != -1 )
        {
            angle = stdMath_NormalizeAngle(RD_MULTIPLY(sithAIMove_flt_539D6C, secDeltaTime));
            pThing->renderData.apTweakedAngles[jointIndex].yaw += angle;
        }
    }
}
