#include "sithPathMove.h"

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Engine/sithCollision.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithPathMove_countfit_1, 15)
CF_PROTO(sithPathMove_countfit_2, 15)
CF_PROTO(sithPathMove_countfit_3, 15)
CF_PROTO(sithPathMove_countfit_4, 15)
CF_PROTO(sithPathMove_countfit_5, 15)
CF_PROTO(sithPathMove_countfit_6, 15)
CF_PROTO(sithPathMove_countfit_7, 15)
CF_PROTO(sithPathMove_countfit_8, 15)
CF_PROTO(sithPathMove_countfit_9, 15)
CF_PROTO(sithPathMove_countfit_10, 15)
CF_PROTO(sithPathMove_countfit_11, 15)
CF_PROTO(sithPathMove_countfit_12, 15)
CF_PROTO(sithPathMove_countfit_13, 15)
CF_PROTO(sithPathMove_countfit_14, 15)
CF_PROTO(sithPathMove_countfit_15, 15)
CF_PROTO(sithPathMove_countfit_16, 7)
/* end count-fit */

// Retail's abs/clip is `fcomp 0; fld; fchs`, not `fabs` (same shape as sithPhysics.c)
#define SITHPATHMOVE_ABS(x)          ((x) < 0.0f ? -(x) : (x))

void J3DAPI sithPathMove_FollowNextPath(SithThing* pThing);
void J3DAPI sithPathMove_SendMessageBlocked(SithThing* pThing);
void J3DAPI sithPathMove_UpdatePath(SithThing* pThing);

void J3DAPI sithPathMove_SkipToFrame(SithThing* pThing, size_t frame, float velocity)
{
    SithPathMoveInfo* pPath;

#line 82
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(velocity > ((float)0.0));

    pPath = &pThing->moveInfo.pathMovement;
    if ( frame >= pPath->numFrames )
    {
#line 91
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SkipToFrame: illegal frame #%d.\n", frame, 0, 0, 0);
        return;
    }

    pPath->mode     &= ~SITH_PATHMOVE_FOLLOWPATH;
    pPath->goalFrame = frame;
    pPath->moveVel   = velocity;

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);

    sithPathMove_MoveToPos(pThing, &pPath->aFrames[pPath->goalFrame], velocity);
}

void J3DAPI sithPathMove_MoveToFrame(SithThing* pThing, size_t frame, float velocity)
{
    SithPathMoveInfo* pPath;

#line 134
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(velocity > ((float)0.0));

    pPath = &pThing->moveInfo.pathMovement;
    if ( frame >= pPath->numFrames )
    {
#line 143
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "MoveToFrame: illegal frame #%d.\n", frame, 0, 0, 0);
        return;
    }

    pPath->mode     |= SITH_PATHMOVE_FOLLOWPATH;
    pPath->moveVel   = velocity;
    pPath->goalFrame = frame;

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);

    sithPathMove_FollowNextPath(pThing);
}

void J3DAPI sithPathMove_RotatePivot(SithThing* pThing, const rdVector3* pOffset, const rdVector3* pRot, float rotTime)
{
    SithPathMoveInfo* pPath;

#line 189
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(rotTime > ((float)0.0));

    pPath = &pThing->moveInfo.pathMovement;
    pPath->mode |= SITH_PATHMOVE_ROTATE_PIVOT | SITH_PATHMOVE_ROTATE;

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);

    rdVector_Copy3(&pPath->rotOffset, pOffset);
    rdMatrix_Copy34(&pPath->curOrient, &pThing->orient);
    rdVector_Sub3(&pPath->curOrient.dvec, &pThing->pos, &pPath->rotOffset);

    pPath->rotDeltaTime = 1.0f / rotTime;
    rdVector_Copy3(&pPath->goalPYR, pRot);

    pPath->rotDelta        = 0.0f;
    pPath->numBlockedMoves = 0;
    pPath->nextFrame       = pPath->currentFrame;
    pPath->currentFrame    = -1;
}

/* PLACEHOLDER, linker-discarded; name inferred. Jedi Knight's
   sithTrackThing.c (Indy's sithPathMove.c: every other function corresponds one to one, in the same
   order) has sithTrackThing_Rotate(thing, rot) directly after RotatePivot (JK 0x004FA980, MotS
   0x0054BBE0), and retail's __LINE__ anchors leave the file's largest excess exactly here
   (RotatePivot 191 -> FollowNextPath 316, +76 lines at the unit's 21-line overhead). Indy's cog verb
   "rotate" calls RotatePivot instead, so nothing referenced it. JK's signature; body not restored
   (JK-only text; JK's frame is 0 bytes, so no stack locals are attested). JK declares it in its module
   header; the matching sithPathMove.h prototype is left out: it changes sithCogFunctionThing's code
   (SetThingHeadOrientation, by declaration count) away from retail's. */
void J3DAPI sithPathMove_Rotate(SithThing* pThing, rdVector3* pRot)
{
}

void J3DAPI sithPathMove_FollowNextPath(SithThing* pThing)
{
    SithPathMoveInfo* pPath;
    float curtime;
    size_t nextFrame;

#line 316
    SITH_ASSERTREL(pThing && (pThing->moveType == SITH_MT_PATH));

    pPath = &pThing->moveInfo.pathMovement;
#line 321
    SITH_ASSERTREL(((pPath->mode) & (SITH_PATHMOVE_FOLLOWPATH)));

    if ( pPath->currentFrame == pPath->goalFrame )
    {
        curtime = sithPathMove_MoveToPos(pThing, &pPath->aFrames[pPath->currentFrame], pPath->moveVel);
        if ( curtime == 0.0f )
        {
#line 330
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing %s (index %d) reached goal frame %d.\n", pThing->aName, pThing->idx, pPath->goalFrame, 0);
            sithPathMove_Finish(pThing);
            return;
        }

        nextFrame = pPath->goalFrame;
    }
    else
    {
        if ( pPath->currentFrame < 0 )
        {
            nextFrame = pPath->goalFrame;
        }
        else
        {
            nextFrame = pPath->currentFrame + ((int)pPath->goalFrame - pPath->currentFrame < 0 ? -1 : 1);
        }

#line 350
        SITH_ASSERTREL(nextFrame < pPath->sizeFrames);
        pPath->nextFrame = nextFrame;
        curtime = sithPathMove_MoveToPos(pThing, &pPath->aFrames[nextFrame], pPath->moveVel);
    }

    curtime = 1.0f > curtime ? 1.0f : curtime;

    sithPathMove_RotateToPYR(pThing, &pPath->aFrames[nextFrame].pyr, curtime);
}

float J3DAPI sithPathMove_MoveToPos(SithThing* pThing, SithPathFrame* pos, float moveVel)
{
    SithPathMoveInfo* pPath;
    float dist;
    float moveTime = 0.0f;  // unreferenced; the debug build's frame keeps its slot

#line 396
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(pos);

    pPath = &pThing->moveInfo.pathMovement;
#line 403
    SITH_ASSERTREL(pPath->moveVel > ((float)0.0));

    rdVector_Sub3(&pPath->vecDeltaPos, &pos->pos, &pThing->pos);
    dist = rdVector_Normalize3Acc(&pPath->vecDeltaPos);
    if ( dist != 0.0f )
    {
        pPath->mode |= SITH_PATHMOVE_MOVE;
        pPath->numBlockedMoves = 0;
        pPath->moveTimeRemaining = dist / (pPath->moveVel);
#line 418
        SITH_ASSERTREL(pPath->moveTimeRemaining > ((float)0.0));
    }

    return pPath->moveTimeRemaining;
}

void J3DAPI sithPathMove_RotateToPYR(SithThing* pThing, rdVector3* pyr, float time)
{
    SithPathMoveInfo* pCurrentPath;
    rdVector3 curPYR;
    rdVector3 curnexGoalPYR;

#line 447
    SITH_ASSERTREL(pThing && (pThing->moveType == SITH_MT_PATH));
    SITH_ASSERTREL(pyr);
    SITH_ASSERTREL(time > ((float)0.0));

    pCurrentPath = &pThing->moveInfo.pathMovement;

    rdMatrix_ExtractAngles34(&pThing->orient, &curPYR);
    rdVector_Sub3(&curnexGoalPYR, pyr, &curPYR);

    curnexGoalPYR.pitch = stdMath_NormalizeAngleAcute(curnexGoalPYR.pitch);
    curnexGoalPYR.yaw   = stdMath_NormalizeAngleAcute(curnexGoalPYR.yaw);
    curnexGoalPYR.roll  = stdMath_NormalizeAngleAcute(curnexGoalPYR.roll);

    if ( SITHPATHMOVE_ABS(curnexGoalPYR.x) < 2.5f )
    {
        curnexGoalPYR.x = 0.0f;
    }

    if ( SITHPATHMOVE_ABS(curnexGoalPYR.y) < 2.5f )
    {
        curnexGoalPYR.y = 0.0f;
    }

    if ( SITHPATHMOVE_ABS(curnexGoalPYR.z) < 2.5f )
    {
        curnexGoalPYR.z = 0.0f;
    }

    if ( curnexGoalPYR.x != 0.0f || curnexGoalPYR.y != 0.0f || curnexGoalPYR.z != 0.0f )
    {
        pCurrentPath->rotDeltaTime = 1.0f / time;
        rdMatrix_Copy34(&pCurrentPath->curOrient, &pThing->orient);
        memset(&pCurrentPath->curOrient.dvec, 0, sizeof(rdVector3));
        rdVector_Copy3(&pCurrentPath->goalPYR, &curnexGoalPYR);
        rdVector_Copy3(&pCurrentPath->rotateToPYR, pyr);

        pCurrentPath->mode |= SITH_PATHMOVE_ROTATE;
        pCurrentPath->mode &= ~SITH_PATHMOVE_ROTATE_PIVOT;
        pCurrentPath->rotDelta = 0.0f;
    }
}

static __inline void J3DAPI sithPathMove_UpdateRotate(SithThing* pThing, float simTime)
{
    rdVector3 deltaPYR;
    rdMatrix34 curOrient;
    float deltaTime;
    float rotDelta;
    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

    deltaTime = pPath->rotDeltaTime * simTime;
    rotDelta  = pPath->rotDelta + deltaTime;
    if ( rotDelta > 1.0f )
    {
        deltaTime = 1.0f - pPath->rotDelta;
        rotDelta  = 1.0f;
    }
    else if ( rotDelta < -1.0f )
    {
        deltaTime = -1.0f - pPath->rotDelta;
        rotDelta  = -1.0f;
    }

    rdVector_Scale3(&deltaPYR, &pPath->goalPYR, rotDelta);
    rdMatrix_Copy34(&curOrient, &pPath->curOrient);
    rdMatrix_PostRotate34(&curOrient, &deltaPYR);

    if ( (pPath->mode & SITH_PATHMOVE_ROTATE_PIVOT) != 0 )
    {
        rdVector3 moveNorm;
        rdVector_Add3(&moveNorm, &pPath->rotOffset, &curOrient.dvec);
        rdVector_Sub3Acc(&moveNorm, &pThing->pos);

        if ( moveNorm.x != 0.0f || moveNorm.y != 0.0f || moveNorm.z != 0.0f )
        {
            float moveDist = rdVector_Normalize3Acc(&moveNorm);
            moveDist = stdMath_ClipNearZero(moveDist);
            if ( moveDist != 0.0f )
            {
                float movedDist = sithCollision_MoveThing(pThing, &moveNorm, moveDist, 0x44);
                if ( movedDist < moveDist )
                {
                    rotDelta = movedDist / moveDist * deltaTime + pPath->rotDelta;
                    rdVector_Scale3(&deltaPYR, &pPath->goalPYR, rotDelta);
                    rdMatrix_Copy34(&curOrient, &pPath->curOrient);
                    rdMatrix_PreRotate34(&curOrient, &deltaPYR);
                }
            }
        }
    }

    pPath->rotDelta = rotDelta;
    memset(&pThing->orient.dvec, 0, sizeof(rdVector3));
    sithCollision_ApplyThingOrient(pThing, &curOrient);

    if ( pPath->rotDelta >= 1.0f || pPath->rotDelta <= -1.0f )
    {
        if ( (pPath->mode & SITH_PATHMOVE_ROTATE_PIVOT) == 0 )
        {
            rdMatrix_BuildRotate34(&curOrient, &pPath->rotateToPYR);
            memset(&pThing->orient.dvec, 0, sizeof(rdVector3));
            sithCollision_ApplyThingOrient(pThing, &curOrient);
        }

        pPath->mode &= ~(SITH_PATHMOVE_ROTATE_PIVOT | SITH_PATHMOVE_ROTATE);
    }
}

static __inline void J3DAPI sithPathMove_UpdateMove(SithThing* pThing, float simTime)
{
    float moveDist;
    float timeDelta;
    SithPathMoveInfo* pPath = &pThing->moveInfo.pathMovement;

    timeDelta = pPath->moveTimeRemaining > simTime ? simTime : pPath->moveTimeRemaining;

    moveDist = pPath->moveVel * timeDelta;
    moveDist = stdMath_ClipNearZero(moveDist);
    if ( moveDist != 0.0f )
    {
        float movedDist = sithCollision_MoveThing(pThing, &pPath->vecDeltaPos, moveDist, 0x44);
        if ( movedDist < moveDist )
        {
            if ( stdMath_ClipNearZero(movedDist) > 0.0f )
            {
                timeDelta = movedDist / moveDist * timeDelta;
            }
            else
            {
                timeDelta = 0.0f;
            }

            ++pPath->numBlockedMoves;
        }
    }

    pPath->moveTimeRemaining -= timeDelta;
    if ( stdMath_ClipNearZero(pPath->moveTimeRemaining) == 0.0f )
    {
        pPath->mode &= ~SITH_PATHMOVE_MOVE;
    }
    else if ( pPath->numBlockedMoves > 2 )
    {
        sithPathMove_SendMessageBlocked(pThing);
    }
}

void J3DAPI sithPathMove_Update(SithThing* pThing, float simTime)
{
    SithPathMoveInfo* pPath;

#line 511
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    SITH_ASSERTREL(simTime > ((float)0.0));

    if ( simTime == 0.0f )
    {
        return;
    }

    pPath = &pThing->moveInfo.pathMovement;
    if ( !pPath->mode || (pPath->mode & SITH_PATHMOVE_PAUSED) != 0 )
    {
        return;
    }

    if ( (pPath->mode & SITH_PATHMOVE_ROTATE) != 0 )
    {
        sithPathMove_UpdateRotate(pThing, simTime);
    }

    if ( (pPath->mode & SITH_PATHMOVE_MOVE) != 0 )
    {
        sithPathMove_UpdateMove(pThing, simTime);
    }

    if ( (pPath->mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) == 0 )
    {
        sithPathMove_UpdatePath(pThing);
    }
}

int J3DAPI sithPathMove_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    size_t currentframeNumber;
    float curyaw;
    float pitch;
    float roll;
    size_t sizeFrames;
    rdVector3 pos;

#line 553
    SITH_ASSERTREL((pArg!=((void *)0)) && (pThing != ((void *)0)));

    switch ( adjNum )
    {
        case SITHTHING_ARG_NUMFRAMES:
            if ( pThing->moveInfo.pathMovement.sizeFrames )
            {
                goto error;
            }

            sizeFrames = atoi(pArg->argValue);
            if ( (int)sizeFrames < 1 )
            {
                goto error;
            }

#line 567
            SITH_ASSERTREL(pThing->moveInfo.pathMovement.aFrames == NULL);
            pThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * sizeFrames);
            if ( !pThing->moveInfo.pathMovement.aFrames )
            {
                goto alloc_error;
            }

            memset(pThing->moveInfo.pathMovement.aFrames, 0, sizeof(SithPathFrame) * sizeFrames);
            pThing->moveInfo.pathMovement.sizeFrames = sizeFrames;
            pThing->moveInfo.pathMovement.numFrames  = 0;
            break;

        case SITHTHING_ARG_FRAME:
            if ( pThing->moveInfo.pathMovement.numFrames < pThing->moveInfo.pathMovement.sizeFrames )
            {
                currentframeNumber = pThing->moveInfo.pathMovement.numFrames;
                if ( sscanf(pArg->argValue, "(%f/%f/%f:%f/%f/%f)", &pos.x, &pos.y, &pos.z, &pitch, &curyaw, &roll) != 6 )
                {
                    goto error;
                }

                rdVector_Copy3(&pThing->moveInfo.pathMovement.aFrames[currentframeNumber].pos, &pos);
                pThing->moveInfo.pathMovement.aFrames[currentframeNumber].pyr.pitch = pitch;
                pThing->moveInfo.pathMovement.aFrames[currentframeNumber].pyr.yaw   = curyaw;
                pThing->moveInfo.pathMovement.aFrames[currentframeNumber].pyr.roll  = roll;
                pThing->moveInfo.pathMovement.numFrames = ++currentframeNumber;
            }
            else
            {
#line 598
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many frames for thing %d.\n", pThing->idx, 0, 0, 0);
            }

            break;

        default:
            return 0;
    }

    return 1;

alloc_error:
#line 609
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory allocation error, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 0;

error:
#line 613
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber(), 0);
    return 0;
}

void J3DAPI sithPathMove_SendMessageBlocked(SithThing* pThing)
{
#line 621
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);
    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
    {
        sithCog_ThingSendMessage(pThing, NULL, SITHCOG_MSG_BLOCKED);
    }
}

void J3DAPI sithPathMove_UpdatePath(SithThing* pThing)
{
    if ( (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_FOLLOWPATH) != 0 )
    {
        pThing->moveInfo.pathMovement.currentFrame = pThing->moveInfo.pathMovement.nextFrame;
        if ( pThing->moveInfo.pathMovement.currentFrame == pThing->moveInfo.pathMovement.goalFrame )
        {
            sithPathMove_Finish(pThing);
        }
        else
        {
            sithPathMove_FollowNextPath(pThing);
        }
    }
    else
    {
        pThing->moveInfo.pathMovement.currentFrame = pThing->moveInfo.pathMovement.nextFrame;
        sithPathMove_Finish(pThing);
    }
}

void J3DAPI sithPathMove_Finish(SithThing* pThing)
{
    SithPathMoveInfo* pPath;

#line 677
    SITH_ASSERTREL(pThing != NULL);

    pPath = &pThing->moveInfo.pathMovement;
    pPath->mode           &= ~(SITH_PATHMOVE_ROTATE_PIVOT | SITH_PATHMOVE_FOLLOWPATH | SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE);
    pPath->moveVel         = 0.0f;
    pPath->goalFrame       = 0;
    pPath->nextFrame       = 0;
    pPath->numBlockedMoves = 0;

    sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_MOVING);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE);

    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
    {
        sithCog_ThingSendMessage(pThing, NULL, SITHCOG_MSG_ARRIVED);
    }
}

int J3DAPI sithPathMove_PathMovePause(SithThing* pThing)
{
#line 725
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);

    if ( (pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) == 0 )
    {
        return 0;
    }

    sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_MOVING);
    pThing->moveInfo.pathMovement.mode |= SITH_PATHMOVE_PAUSED;
    return 1;
}

int J3DAPI sithPathMove_PathMoveResume(SithThing* pThing)
{
#line 762
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PATH);

    if ( (pThing->moveInfo.pathMovement.mode & SITH_PATHMOVE_PAUSED) == 0 )
    {
        return 0;
    }

    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_MOVING);
    pThing->moveInfo.pathMovement.mode &= ~SITH_PATHMOVE_PAUSED;
    return 1;
}

void J3DAPI sithPathMove_Create(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pOffset)
{
    size_t i;
    SithPathFrame* pFrame;
    rdVector3 offset;

#line 802
    SITH_ASSERTREL(pNewThing && pMarker);
    SITH_ASSERTREL((pNewThing->moveType == SITH_MT_PATH) && (pMarker->moveType == SITH_MT_PATH));
    SITH_ASSERTREL(!pNewThing->moveInfo.pathMovement.aFrames);

#line 807
    pNewThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * pMarker->moveInfo.pathMovement.sizeFrames);
    memcpy(pNewThing->moveInfo.pathMovement.aFrames, pMarker->moveInfo.pathMovement.aFrames, sizeof(SithPathFrame) * pMarker->moveInfo.pathMovement.sizeFrames);

    pNewThing->moveInfo.pathMovement.sizeFrames = pMarker->moveInfo.pathMovement.sizeFrames;
    pNewThing->moveInfo.pathMovement.numFrames  = pMarker->moveInfo.pathMovement.numFrames;

    for ( i = 0; i < pNewThing->moveInfo.pathMovement.numFrames; ++i )
    {
        pFrame = &pNewThing->moveInfo.pathMovement.aFrames[i];
        rdVector_Rotate3(&offset, pOffset, &pFrame->pyr);
        rdVector_Add3Acc(&pFrame->pos, &offset);
    }
}
