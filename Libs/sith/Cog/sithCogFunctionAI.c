#include "sithCogFunctionAI.h"

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithSoundClass.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

void J3DAPI sithCogFunctionAI_AISetMoveSpeed(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetMovePos(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetHomePos(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetMaxHomeDist(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetGoalThing(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetGoalLVec(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIJump(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetMoveFrame(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetMoveThing(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetGoalThing(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIEnableHeadTracking(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIDisableHeadTracking(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIEnableBodyTracking(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIDisableBodyTracking(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetLookPos(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetLookFrame(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetMovePos(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIPauseMove(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetSubMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetSubMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearSubMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_FirstThingInView(SithCog* pCog);
void J3DAPI sithCogFunctionAI_NextThingInView(SithCog* pCog);
void J3DAPI sithCogFunctionAI_ThingViewDot(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetFireTarget(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIEnableInstinct(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetCutsceneMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearCutsceneMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_IsAITargetInSight(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIFlee(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIStopFlee(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetClass(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetLookThing(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetLookThingEyeLevel(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIWaitForStop(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIWaitForHeadTracking(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIGetArmedMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIKnockout(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIRunOver(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetAllowedSurfaceType(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearAllowedSurfaceType(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetWpnt(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetWpntRank(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetWpntFlags(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearWpntFlags(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetActiveWpntLayer(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIConnectWpnts(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIConnectWpntsOneWay(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AITraverseWpnts(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearTraverseWpnts(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIFindNearestWpnt(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIWpntHuntTarget(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISetInstinctWpntMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIClearInstinctWpntMode(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AIFleeToWpnt(SithCog* pCog);
void J3DAPI sithCogFunctionAI_AISpat(SithCog* pCog);

static SithThing* sithCogFunctionAI_aThingsInView[32] = { 0 };
static int sithCogFunctionAI_numThingsInView = 0;
static int sithCogFunctionAI_curThingInView = 0;

int J3DAPI sithCogFunctionAI_RegisterFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetMode, "aigetmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMode, "aisetmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearMode, "aiclearmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetSubMode, "aigetsubmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetSubMode, "aisetsubmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearSubMode, "aiclearsubmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetMovePos, "aigetmovepos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMovePos, "aisetmovepos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetHomePos, "aigethomepos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIPauseMove, "aipausemove");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMaxHomeDist, "aisetmaxhomedist");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetGoalThing, "aigetgoalthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetGoalLVec, "aigetgoallvec");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_FirstThingInView, "firstthinginview");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_NextThingInView, "nextthinginview");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_ThingViewDot, "thingviewdot");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetFireTarget, "aisetfiretarget");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMoveThing, "aisetmovething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookPos, "aisetlookpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMoveSpeed, "aisetmovespeed");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookFrame, "aisetlookframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetMoveFrame, "aisetmoveframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_IsAITargetInSight, "isaitargetinsight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIFlee, "aiflee");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIStopFlee, "aistopflee");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetClass, "aisetclass");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIJump, "aijump");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIEnableInstinct, "aienableinstinct");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookThing, "aisetlookthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetLookThingEyeLevel, "aisetlookthingeyelevel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIWaitForStop, "aiwaitforstop");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIWaitForHeadTracking, "aiwaitforheadtracking");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIGetArmedMode, "aigetarmedmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIKnockout, "aiknockout");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIRunOver, "airunover");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetAllowedSurfaceType, "aisetallowedsurfacetype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearAllowedSurfaceType, "aiclearallowedsurfacetype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetCutsceneMode, "aisetcutscenemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearCutsceneMode, "aiclearcutscenemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetGoalThing, "aisetgoalthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIEnableHeadTracking, "aienableheadtracking");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIDisableHeadTracking, "aidisableheadtracking");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIEnableBodyTracking, "aienablebodytracking");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIDisableBodyTracking, "aidisablebodytracking");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetWpnt, "aisetwpnt");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetWpntRank, "aisetwpntrank");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetWpntFlags, "aisetwpntflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearWpntFlags, "aiclearwpntflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetActiveWpntLayer, "aisetactivewpntlayer");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIConnectWpnts, "aiconnectwpnts");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIConnectWpntsOneWay, "aiconnectwpntsoneway");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AITraverseWpnts, "aitraversewpnts");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearTraverseWpnts, "aicleartraversewpnts");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIFindNearestWpnt, "aifindnearestwpnt");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIWpntHuntTarget, "aiwpnthunttarget");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISetInstinctWpntMode, "aisetinstinctwpntmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIClearInstinctWpntMode, "aiclearinstinctwpntmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AISpat, "aispat");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionAI_AIFleeToWpnt, "aifleetowpnt");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithCogFunctionAI_AISetMoveSpeed(SithCog* pCog)
{
    float speed;
    SithThing* pThing;

#line 286
    SITH_ASSERTREL(pCog);

    speed = sithCogExec_PopFlex(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 294
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AISetMoveSpeed()\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( speed < 0.0f || speed > 10.0f )
    {
#line 300
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad move speed in AISetMoveSpeed().  Capping at 10.\n", pCog->aName, 0, 0, 0);
        speed = STDMATH_CLAMP(speed, 0.0f, 10.0f);
    }

    pThing->controlInfo.aiControl.pLocal->moveSpeed = speed;
}

void J3DAPI sithCogFunctionAI_AISetMovePos(SithCog* pCog)
{
    int wait;
    SithThing* pThing;
    SithAIControlBlock* pLocal;
    rdVector3 position;

#line 331
    SITH_ASSERTREL(pCog);

    wait = sithCogExec_PopInt(pCog);
    if ( !sithCogExec_PopVector(pCog, &position) )
    {
        goto error;
    }

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
        goto error;
    }

    if ( (pLocal->submode & (SITHAI_SUBMODE_UNKNOWN_2 | SITHAI_SUBMODE_UNKNOWN_1)) != 0 )
    {
        sithAIMove_StopAIMovement(pLocal);
    }

    if ( !sithAIMove_AISetMovePos(pLocal, &position, pLocal->moveSpeed) )
    {
        sithCog_ThingSendMessage(pLocal->pOwner, NULL, SITHCOG_MSG_ARRIVED);
        return;
    }

    if ( wait )
    {
        pCog->status = SITHCOG_STATUS_WAITING_AI_TO_STOP;
        pCog->statusParams[0] = pThing->idx;
    }

    return;

error:
#line 363
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or vector for AISetMovePos\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIGetHomePos(SithCog* pCog)
{
    SithThing* pThing;
    int bPushZero;
    rdVector3 vec;

    bPushZero = 0;
#line 387
    SITH_ASSERTREL(pCog);

    memset(&vec, 0, sizeof(vec));
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 395
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AIGetHomePos\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 402
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: thing %s is not AI controlled in AIGetHomePos\n", pCog->aName, pThing->aName, 0, 0);
        bPushZero = 1;
    }

    if ( bPushZero )
    {
        sithCogExec_PushVector(pCog, &vec);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pThing->controlInfo.aiControl.pLocal->homePos);
    }
}

void J3DAPI sithCogFunctionAI_AISetMaxHomeDist(SithCog* pCog)
{
    float dist = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 439
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing passed to AISetMaxHomeDist()\n", pCog->aName, 0, 0, 0);
    }

    else if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 445
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: THING '%s' is not AI controlled in AISetMaxHomeDist\n", pCog->aName, pThing->aName, 0, 0);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->maxHomeDist = dist;
    }
}

void J3DAPI sithCogFunctionAI_AIGetGoalThing(SithCog* pCog)
{
    SithThing* pThing;

#line 471
    SITH_ASSERTREL(pCog);

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 477
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AIGetGoalThing\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 484
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: thing %s is not AI controlled in AIGetGoalThing\n", pCog->aName, pThing->aName, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->controlInfo.aiControl.pLocal->goalThing )
    {
#line 491
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NOTE: Cog %s: thing %s has no goal thing in AIGetGoalThing\n", pCog->aName, pThing->aName, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->controlInfo.aiControl.pLocal->goalThing->idx);
}

void J3DAPI sithCogFunctionAI_AIGetGoalLVec(SithCog* pCog)
{
    SithThing* pThing;

#line 517
    SITH_ASSERTREL(pCog);

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 523
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad THING passed to AIGetGoalLVec\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 530
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: THING %s is not AI controlled in AIGetGoalLVec\n", pCog->aName, pThing->aName, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->controlInfo.aiControl.pLocal->goalLVec);
}

void J3DAPI sithCogFunctionAI_AIJump(SithCog* pCog)
{
    rdVector3 movePos;
    float cura3;
    int bCurVector;
    SithThing* pThing;

#line 547
    SITH_ASSERTREL(pCog);

    
    cura3 = sithCogExec_PopFlex(pCog);
    bCurVector = sithCogExec_PopVector(pCog, &movePos);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bCurVector )
    {
        goto error;
    }

    if ( !pThing->attach.flags
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    sithAIMove_AIJump(pThing->controlInfo.aiControl.pLocal, &movePos, cura3);
    return;

error:
#line 564
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad params in aijump.\n", 0, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AISetMoveFrame(SithCog* pCog)
{
    SithAIControlBlock* local;
    size_t frame;
    SithThing* pThing;

#line 575
    SITH_ASSERTREL(pCog);
    frame = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->controlType != SITH_CT_AI || !(local = pThing->controlInfo.aiControl.pLocal) )
    {
#line 582
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AISetMoveFrame\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !local->aFrames || frame >= local->numFrames )
    {
#line 588
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad frame in AISetMoveFrame\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithAIMove_AISetMovePos(
        local,
        &local->aFrames[frame],
        local->moveSpeed
    );
}

void J3DAPI sithCogFunctionAI_AISetMoveThing(SithCog* pCog)
{
    int wait;
    SithThing* pGoalThing;
    SithThing* pThing;

#line 614
    SITH_ASSERTREL(pCog);
    wait = sithCogExec_PopInt(pCog);
    pGoalThing = sithCogExec_PopThing(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pGoalThing
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->goalThing = pGoalThing;
    sithAIMove_AISetMovePos(
        pThing->controlInfo.aiControl.pLocal,
        &pGoalThing->pos,
        pThing->controlInfo.aiControl.pLocal->moveSpeed
    );

    if ( wait )
    {
        pCog->status = SITHCOG_STATUS_WAITING_AI_TO_STOP;
        pCog->statusParams[0] = pThing->idx;
    }
    return;

error:
#line 635
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args for AISetMoveThing\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AISetGoalThing(SithCog* pCog)
{
    SithThing* pGoalThing;
    SithThing* pThing;

#line 658
    SITH_ASSERTREL(pCog);
    pGoalThing = sithCogExec_PopThing(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pGoalThing
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->goalThing = pGoalThing;
    return;

error:
#line 671
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args for AISetGoalThing\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIEnableHeadTracking(SithCog* pCog)
{
    SithThing* pTarget;
    SithThing* pThing;

#line 693
    SITH_ASSERTREL(pCog);
    pTarget = sithCogExec_PopThing(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pTarget
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->pTargetThing = pTarget;
    pThing->controlInfo.aiControl.pLocal->submode |= SITHAI_SUBMODE_HEADTRACKINGMOTION | SITHAI_SUBMODE_UNKNOWN_800 | SITHAI_SUBMODE_UNKNOWN_400;
    return;

error:
#line 707
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args for AIEnableHeadTracking\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIDisableHeadTracking(SithCog* pCog)
{
    SithThing* pThing;

#line 727
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->submode &= ~(SITHAI_SUBMODE_HEADTRACKINGMOTION | SITHAI_SUBMODE_UNKNOWN_400); // Note, SITHAI_SUBMODE_UNKNOWN_800 not being cleared
    return;

error:
#line 739
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args for AIDisableHeadTracking\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIEnableBodyTracking(SithCog* pCog)
{
    SithThing* pTarget;
    SithThing* pThing;

#line 762
    SITH_ASSERTREL(pCog);
    pTarget = sithCogExec_PopThing(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing
        || !pTarget
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->pTargetThing = pTarget;
    pThing->controlInfo.aiControl.pLocal->submode |= SITHAI_SUBMODE_BODYTRACKINGMOTION;
    return;

error:
#line 776
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args for AIEnableBodyTracking\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIDisableBodyTracking(SithCog* pCog)
{
    SithThing* pThing;

#line 796
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->submode &= ~SITHAI_SUBMODE_BODYTRACKINGMOTION;
    return;

error:
#line 808
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args for AIDisableBodyTracking\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AISetLookPos(SithCog* pCog)
{
    SithThing* pThing;
    rdVector3 targetPos;

#line 830
    SITH_ASSERTREL(pCog);
    if ( !sithCogExec_PopVector(pCog, &targetPos) )
    {
        goto error;
    }

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    sithAIMove_AISetLookPos(pThing->controlInfo.aiControl.pLocal, &targetPos);
    return;

error:
#line 843
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or vector for AISetLookPos\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AISetLookFrame(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    size_t frameCount;
    SithThing* pThing;

#line 854
    SITH_ASSERTREL(pCog);
    frameCount = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->controlType != SITH_CT_AI
        || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
        goto error;
    }

    if ( !pLocal->aFrames || frameCount >= pLocal->numFrames )
    {
        goto error;
    }

    sithAIMove_AISetLookPos(pLocal, &pLocal->aFrames[frameCount]);
    return;

error:
#line 869
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or frame for AISetLookFrame\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIGetMovePos(SithCog* pCog)
{
    SithThing* pThing;

#line 892
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    sithCogExec_PushVector(pCog, &pThing->controlInfo.aiControl.pLocal->movePos);
    return;

error:
#line 903
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid AI THING passed to AIGetMovePos.\n", pCog->aName, 0, 0, 0);
    // TODO: Shouldn't return zero vector?
}

void J3DAPI sithCogFunctionAI_AIPauseMove(SithCog* pCog)
{
    int msecPause;
    SithThing* pThing;

#line 924
    SITH_ASSERTREL(pCog);
    msecPause = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    sithAIUtil_AIPauseMove(pThing->controlInfo.aiControl.pLocal, msecPause);
    return;

error:
#line 937
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid AI THING passed to AIPauseMove.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AISetMode(SithCog* pCog)
{
    SithAIMode newMode;
    SithAIMode oldmode;
    SithThing* pThing;

#line 964
    SITH_ASSERTREL(pCog);
    newMode = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    oldmode = pThing->controlInfo.aiControl.pLocal->mode;
    pThing->controlInfo.aiControl.pLocal->mode |= newMode;
    if ( oldmode != pThing->controlInfo.aiControl.pLocal->mode )
    {
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
    }
    return;

error:
#line 981
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AISetMode.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIGetMode(SithCog* pCog)
{
    SithThing* pThing;

#line 1001
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    sithCogExec_PushInt(pCog, pThing->controlInfo.aiControl.pLocal->mode);
    return;

error:
#line 1011
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AIGetMode.\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionAI_AIClearMode(SithCog* pCog)
{
    SithThing* pThing;
    SithAIMode oldmode;
    SithAIMode mode;

#line 1039
    SITH_ASSERTREL(pCog);
    mode = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    oldmode = pThing->controlInfo.aiControl.pLocal->mode;
    pThing->controlInfo.aiControl.pLocal->mode &= ~mode;
    if ( oldmode != pThing->controlInfo.aiControl.pLocal->mode )
    {
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
    }
    return;

error:
#line 1056
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AIClearMode.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIGetSubMode(SithCog* pCog)
{
    SithThing* pThing;

#line 1076
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    sithCogExec_PushInt(pCog, pThing->controlInfo.aiControl.pLocal->submode);
    return;

error:
#line 1086
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AIGetSubMode.\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionAI_AISetSubMode(SithCog* pCog)
{
    SithAISubMode submode;
    SithThing* pThing;

#line 1107
    SITH_ASSERTREL(pCog);
    submode = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->submode |= submode;
    return;

error:
#line 1120
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AISetSubMode.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIClearSubMode(SithCog* pCog)
{
    SithAISubMode submode;
    SithThing* pThing;

#line 1129
    SITH_ASSERTREL(pCog);
    submode = sithCogExec_PopInt(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
        goto error;
    }

    pThing->controlInfo.aiControl.pLocal->submode &= ~submode;
    return;

error:
#line 1142
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to AIClearSubMode.\n", pCog->aName, 0, 0, 0);
}

// Gets a thing in the specified view from a thing. 
// Use : 
//     FirstThingInView(ThingThatFinds, FOV(in angle), Distance(jku), WhatToLookForFlag);
//     Flag is the thing type value. The Type is ThingTypeMask
//     For ie if flag is 0x4, then actor thing should be returned
//     Returns the thing found.
// 
// From:    
//     https://web.archive.org/web/20011212235446/http://www.sfc.keio.ac.jp:80/~t00214ho/millennium/cogverb/
// 
// Forum:
//     https://web.archive.org/web/20200114134644/https://forums.massassi.net/vb3/archive/index.php/t-14421.html
void J3DAPI sithCogFunctionAI_FirstThingInView(SithCog* pCog)
{
    rdMatrix34 orient;

    int thingTypeMask = sithCogExec_PopInt(pCog);
    float curLen    = sithCogExec_PopFlex(pCog);
    float fovX        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 1173
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing passed to FirstThingInView.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdMatrix_Copy34(&orient, &pThing->orient);
    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        rdMatrix_PreRotate34(&orient, &pThing->thingInfo.actorInfo.headPYR);
        rdMatrix_PostTranslate34(&orient, &pThing->pos);
        rdMatrix_PreTranslate34(&orient, &pThing->thingInfo.actorInfo.eyeOffset);
    }
    else
    {
        rdMatrix_PostTranslate34(&orient, &pThing->pos);
    }

    sithCogFunctionAI_numThingsInView = sithAIUtil_GetThingsInView(
        pThing->pInSector,
        &orient,
        fovX,
        fovX,
        STD_ARRAYLEN(sithCogFunctionAI_aThingsInView),
        sithCogFunctionAI_aThingsInView,
        thingTypeMask,
        curLen
    );

    sithCogFunctionAI_curThingInView = 0;
    if ( sithCogFunctionAI_numThingsInView > 0 && sithCogFunctionAI_aThingsInView[sithCogFunctionAI_curThingInView] )
    {
        sithCogExec_PushInt(pCog, sithCogFunctionAI_aThingsInView[sithCogFunctionAI_curThingInView]->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionAI_NextThingInView(SithCog* pCog)
{
    ++sithCogFunctionAI_curThingInView;
    if ( sithCogFunctionAI_curThingInView < sithCogFunctionAI_numThingsInView
        && sithCogFunctionAI_aThingsInView[sithCogFunctionAI_curThingInView] )
    {
        sithCogExec_PushInt(pCog, sithCogFunctionAI_aThingsInView[sithCogFunctionAI_curThingInView]->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionAI_ThingViewDot(SithCog* pCog)
{
    rdMatrix34 orient1;
    rdVector3 dir;
    rdVector3 lvec;
    float dot;

    SithThing* pThing2 = sithCogExec_PopThing(pCog);
    SithThing* pThing1 = sithCogExec_PopThing(pCog);
    if ( !pThing2 || !pThing1 )
    {
#line 1253
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or player passed to ThingViewDot.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1000.0f);
    }
    else
    {
        rdMatrix_Copy34(&orient1, &pThing1->orient);
        if ( pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER )
        {
            rdMatrix_PreRotate34(&orient1, &pThing1->thingInfo.actorInfo.headPYR);
        }

        memcpy(&lvec, &orient1.lvec, sizeof(lvec));
        rdVector_Sub3(&dir, &pThing2->pos, &pThing1->pos);
        rdVector_Normalize3Acc(&lvec);
        rdVector_Normalize3Acc(&dir);
        dot = rdVector_Dot3(&lvec, &dir);
        sithCogExec_PushFlex(pCog, dot);
    }
}

void J3DAPI sithCogFunctionAI_AISetFireTarget(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithAIMode curOldmode;
    SithThing* pCurTarget = sithCogExec_PopThing(pCog);
    SithThing* pObj = sithCogExec_PopThing(pCog);
    if ( !pObj || pObj->controlType != SITH_CT_AI || !(pLocal = pObj->controlInfo.aiControl.pLocal) )
    {
        goto error;
    }

    if ( !pCurTarget )
    {
        goto error;
    }

    pLocal->pTargetThing    = pCurTarget;
    pLocal->goalThing       = pCurTarget;
    pLocal->msecAttackStart = sithTime_msecGameTime;
    if ( pCurTarget )
    {
        curOldmode = pLocal->mode;
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_ATTACKING);

        if ( (pLocal->submode & SITHAI_SUBMODE_CONTINUOUSWPNTMOTION) == 0 )
        {
            sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
        }

        sithAI_ForceInstinctUpdate(pLocal, "primaryfire", 0);
        sithAI_ForceInstinctUpdate(pLocal, "lobfire", 0);
        sithAI_ForceInstinctUpdate(pLocal, "alternatefire", 0);

        if ( pLocal->mode != curOldmode )
        {
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_MODECHANGED, (void*)curOldmode);
        }
    }

    return;

error:
#line 1334
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad parameters to SetFireTarget.\n", 0, 0, 0, 0);
}

void J3DAPI sithCogFunctionAI_AIEnableInstinct(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    int bEnable = sithCogExec_PopInt(pCog);
    const char* pCurInstinctName = sithCogExec_PopString(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    int bSuccess;
    if ( !pThing
        || !pCurInstinctName
        || pThing->controlType != SITH_CT_AI
        || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 1368
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad AI THING in EnableInstinct().\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        bSuccess = sithAI_EnableInstinct(pLocal, pCurInstinctName, bEnable);
        sithCogExec_PushInt(pCog, bSuccess);
    }
}

void J3DAPI sithCogFunctionAI_AISetCutsceneMode(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1433
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad AI THING in AISetCutsceneMode().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 1439
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not AI controlled in AISetCutsceneMode().\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_BLOCK);
}

void J3DAPI sithCogFunctionAI_AIClearCutsceneMode(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1458
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad AI THING in AIClearCutsceneMode().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 1464
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not AI controlled in AIClearCutsceneMode().\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
}

void J3DAPI sithCogFunctionAI_IsAITargetInSight(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1492
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid AI THING in IsAITargetInSight.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
    }
    else if ( !pThing->controlInfo.aiControl.pLocal->pTargetThing || pThing->controlInfo.aiControl.pLocal->targetSightState != 0 )// 0 - target in sight
    {
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionAI_AIFlee(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithThing* pFleeThing = sithCogExec_PopThing(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pFleeThing
        || !pThing
        || pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1533
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid AI thing in AIFlee.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pLocal = pThing->controlInfo.aiControl.pLocal;
        pLocal->pFleeFromThing = pFleeThing;
        if ( (pLocal->mode & SITHAI_MODE_FLEEING) == 0 )
        {
            SithAIMode oldmode;

            oldmode = pLocal->mode;
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEING);
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
        }
    }
}

void J3DAPI sithCogFunctionAI_AIStopFlee(SithCog* pCog)
{
    SithThing* curThing = sithCogExec_PopThing(pCog);
    SithAIMode curoldmode;

    SithAIControlBlock* pLocal;
    if ( !curThing
        || curThing->type != SITH_THING_ACTOR
        || curThing->controlType != SITH_CT_AI
        || !curThing->controlInfo.aiControl.pLocal )
    {
        if ( !curThing )
        {
#line 1573
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in AIStopFlee.\n", pCog->aName, 0, 0, 0);
        }
        else
        {
#line 1575
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not AI controlled in AIStopFlee.\n", pCog->aName, curThing->aName, 0, 0);
        }
    }
    else
    {
        pLocal = curThing->controlInfo.aiControl.pLocal;
        if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
        {
            curoldmode = pLocal->mode;
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
            sithAI_EmitEvent(curThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)curoldmode);
            pLocal->pFleeFromThing = NULL;
        }
    }
}

void J3DAPI sithCogFunctionAI_AISetClass(SithCog* pCog)
{
    SithAIClass* pClass = sithCogExec_PopAIClass(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pClass || !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1605
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad arguments for AISetClass.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->controlInfo.aiControl.pClass = pClass;
    pThing->controlInfo.aiControl.pLocal->pClass = pClass;
    pThing->controlInfo.aiControl.pLocal->numInstincts = pClass->numInstincts;
}

void J3DAPI sithCogFunctionAI_AISetLookThing(SithCog* pCog)
{
    SithThing* pTarget;
    SithThing* pThing;

#line 1634
    SITH_ASSERTREL(pCog);
    pTarget = sithCogExec_PopThing(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1642
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AISetLookThing()\n", pCog->aName, 0, 0, 0);
    }
    else if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1649
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: thing %s is not AI controlled in AISetLookThing()\n", pCog->aName, pThing->aName, 0, 0);
    }
    else if ( !pTarget )
    {
#line 1656
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad target in AISetLookThing()\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        sithAIMove_AISetLookThing(pThing->controlInfo.aiControl.pLocal, pTarget);
    }
}

void J3DAPI sithCogFunctionAI_AISetLookThingEyeLevel(SithCog* pCog)
{
    SithThing* pTarget;
    SithThing* pThing;

#line 1684
    SITH_ASSERTREL(pCog);
    pTarget = sithCogExec_PopThing(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1692
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AISetLookThingEyeLevel()\n", pCog->aName, 0, 0, 0);
    }
    else if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1699
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: thing %s is not AI controlled in AISetLookThingEyeLevel()\n", pCog->aName, pThing->aName, 0, 0);
    }
    else if ( !pTarget )
    {
#line 1706
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad target in AISetLookThingEyeLevel()\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        sithAIMove_AISetLookPosEyeLevel(pThing->controlInfo.aiControl.pLocal, &pTarget->pos);
    }
}

void J3DAPI sithCogFunctionAI_AIWaitForStop(SithCog* pCog)
{
    SithThing* pThing;

#line 1732
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1738
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad AI THING in AIWaitForStop()\n", pCog->aName, 0, 0, 0);
    }
    else if ( (pThing->controlInfo.aiControl.pLocal->mode & (SITHAI_MODE_TURNING | SITHAI_MODE_MOVING)) == 0 )
    {
#line 1744
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: '%s' is neither moving nor turning in AIWaitForStop().  Execution continuing.\n", pCog->aName, pThing->aName, 0, 0);
    }
    else
    {
        pCog->status = SITHCOG_STATUS_WAITING_AI_TO_STOP;
        pCog->statusParams[0] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionAI_AIWaitForHeadTracking(SithCog* pCog)
{
    SithThing* pThing;

#line 1771
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1777
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad AI THING in AIWaitForHeadTracking()\n", pCog->aName, 0, 0, 0);
    }
    else if ( (pThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_HEADTRACKINGMOTION) == 0 )
    {
#line 1783
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: '%s' is not head tracking in AIWaitForHeadTracking().  Execution continuing.\n", pCog->aName, pThing->aName, 0, 0);
    }
    else
    {
        pCog->status = SITHCOG_STATUS_WAITING_AI_HEAD_TRACK;
        pCog->statusParams[0] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionAI_AIGetArmedMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1812
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AIGetArmedMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing->pPuppetState )
    {
#line 1819
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: AIGetArmedMode(): Puppet state not available.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->pPuppetState->armedMode);
    }
}

void J3DAPI sithCogFunctionAI_AIKnockout(SithCog* pCog)
{
    int curtimerId = sithCogExec_PopInt(pCog);
    float secDuration = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

#line 1847
    SITH_ASSERTREL(pThing);
#line 1848
    SITH_ASSERTREL(pThing->renderData.pPuppet);
#line 1849
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);
#line 1850
    SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal);

    if ( secDuration > 0.0f )
    {
        SithEventParams params;

        sithPuppet_RemoveAllTracks(pThing);

        pThing->thingInfo.actorInfo.flags |= SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, sithAIMove_PuppetCallback); // activate - knockout

        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        pThing->moveStatus = SITHPLAYERMOVE_KNOCKEDOUT;

        params.param3 = 0;
        params.param2 = pThing->idx;
        params.param1 = curtimerId;
        params.idx    = pCog->idx;
        sithEvent_CreateEvent(SITHCOG_TASKID, &params, (uint32_t)(secDuration * 1000.0f));

        pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_DISABLED;
        pThing->collide.type = SITH_COLLIDE_NONE;
        pThing->flags &= ~SITH_TF_SHADOW;
    }
    else
    {
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATEHIGH, sithAIMove_PuppetCallback);
        sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, 0.0f);
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DRAWWEAPON, NULL);

        pThing->flags |= SITH_TF_SHADOW;
    }
}

void J3DAPI sithCogFunctionAI_AIRunOver(SithCog* pCog)
{
    SithInventoryType* pItem;

    int curtimerId            = sithCogExec_PopInt(pCog);
    float duration    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

#line 1922
    SITH_ASSERTREL(pThing);
#line 1923
    SITH_ASSERTREL(pThing->renderData.pPuppet);
#line 1924
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);
#line 1925
    SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal);
#line 1926
    SITH_ASSERTREL(duration);

    sithPuppet_RemoveAllTracks(pThing);

    pThing->thingInfo.actorInfo.flags |= SITH_AF_IMMOBILE | SITH_AF_INVULNERABLE;
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WHIPSWING, sithAIMove_PuppetCallback); // whipswing - runover mode

    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
    pThing->moveStatus = SITHPLAYERMOVE_RUNOVER;

    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATH2);

    {
        SithEventParams params;

        params.param3 = 0;
        params.param2 = pThing->idx;
        params.param1 = curtimerId;
        params.idx    = pCog->idx;
        sithEvent_CreateEvent(SITHCOG_TASKID, &params, (uint32_t)(duration * 1000.0f));
    }

    pItem = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
    if ( pItem->pCog )
    {
        sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_DESELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
    }

    pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_DISABLED;
    pThing->controlInfo.aiControl.pLocal->mode &= ~(SITHAI_MODE_ACTIVE | SITHAI_MODE_UNKNOWN_80 | SITHAI_MODE_TURNING | SITHAI_MODE_MOVING);
}

void J3DAPI sithCogFunctionAI_AISetAllowedSurfaceType(SithCog* pCog)
{
    int surftypes = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 1987
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AISetAllowedSurfaceType()\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->allowedSurfaceTypes |= surftypes;
    }
}

void J3DAPI sithCogFunctionAI_AIClearAllowedSurfaceType(SithCog* pCog)
{
    int surftypes = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 2005
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AIClearAllowedSurfaceType()\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->allowedSurfaceTypes &= ~surftypes;
    }
}

void J3DAPI sithCogFunctionAI_AISetWpnt(SithCog* pCog)
{
    int wpntIdx = sithCogExec_PopInt(pCog);
    SithThing* pObj = sithCogExec_PopThing(pCog);
    if ( !pObj )
    {
#line 2024
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid GHOST THING in AISetWpnt().\n", 0, 0, 0, 0);
        return;
    }

    sithAIUtil_AISetWpnt(pObj, wpntIdx);
}

void J3DAPI sithCogFunctionAI_AISetWpntRank(SithCog* pCog)
{
    int rank    = sithCogExec_PopInt(pCog);
    int wpntNum = sithCogExec_PopInt(pCog);
    sithAIUtil_AISetWpntRank(wpntNum, rank);
}

void J3DAPI sithCogFunctionAI_AISetWpntFlags(SithCog* pCog)
{
    SithAIWaypointLayerFlag flags = sithCogExec_PopInt(pCog);
    int wpntNum = sithCogExec_PopInt(pCog);
    sithAIUtil_AISetWpntFlags(wpntNum, flags);
}

void J3DAPI sithCogFunctionAI_AIClearWpntFlags(SithCog* pCog)
{
    SithAIWaypointLayerFlag flags = sithCogExec_PopInt(pCog);
    int wpntIdx = sithCogExec_PopInt(pCog);
    sithAIUtil_AIClearWpntFlags(wpntIdx, flags);
}

void J3DAPI sithCogFunctionAI_AISetActiveWpntLayer(SithCog* pCog)
{
    int layer = sithCogExec_PopInt(pCog);
    sithAIUtil_AISetActiveWpntLayer(layer);
}

void J3DAPI sithCogFunctionAI_AIConnectWpnts(SithCog* pCog)
{
    int wpntIdx2 = sithCogExec_PopInt(pCog);
    int wpntIdx1 = sithCogExec_PopInt(pCog);
    sithAIUtil_AIConnectWpnts(wpntIdx1, wpntIdx2, /*bConnectWpnt1To2=*/1);
}

void J3DAPI sithCogFunctionAI_AIConnectWpntsOneWay(SithCog* pCog)
{
    int wpntIdx2 = sithCogExec_PopInt(pCog);
    int wpntIdx1 = sithCogExec_PopInt(pCog);
    sithAIUtil_AIConnectWpnts(wpntIdx1, wpntIdx2, /*bConnectWpnt1To2=*/0);
}

void J3DAPI sithCogFunctionAI_AITraverseWpnts(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    int mode          = sithCogExec_PopInt(pCog);
    float degTurn     = sithCogExec_PopFlex(pCog);
    float moveSpeed   = sithCogExec_PopFlex(pCog);
    int wpntIdx       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 2112
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI THING in AITraverseWpnts().\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithAIUtil_AITraverseWpnts(pLocal, wpntIdx, moveSpeed, degTurn, mode);
    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionAI_AIClearTraverseWpnts(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 2131
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI THING in AIClearTraverseWpnts().\n", 0, 0, 0, 0);
    }
    else
    {
        sithAIUtil_AIClearMode(pLocal, SITHAI_MODE_TRAVERSEWPNTS);
    }
}

void J3DAPI sithCogFunctionAI_AIFindNearestWpnt(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    int wpntCnt = -1;

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->type != SITH_THING_PLAYER
        && (pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal)) )
    {
#line 2149
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI THING in AIFindNearestWpnt().\n", 0, 0, 0, 0);
    }
    else
    {
        wpntCnt = sithAIUtil_AIFindNearestWpnt(pThing, 1);
    }

    sithCogExec_PushInt(pCog, wpntCnt);
}

void J3DAPI sithCogFunctionAI_AIWpntHuntTarget(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithAIMode oldmode;
    int wpntIdx;

    float degTurn = sithCogExec_PopFlex(pCog);
    float moveSpeed = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 2174
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI THING in AIWpntHuntTarget().\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pLocal->pTargetThing = sithPlayer_pLocalPlayerThing;
#line 2180
        SITH_ASSERTREL(pLocal->pTargetThing);

        oldmode = pLocal->mode;
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_HUNTING);

        wpntIdx = sithAIUtil_AIFindNearestWpnt(pThing, 1);
        if ( wpntIdx > -1 )
        {
            sithCogExec_PushInt(pCog, wpntIdx);
            sithAIUtil_AITraverseWpnts(pLocal, wpntIdx, moveSpeed, degTurn, 0);
        }
        else
        {
            sithAIUtil_AISetMode(pLocal, SITHAI_MODE_SEARCHING);
            sithCogExec_PushInt(pCog, -1);
#line 2195
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIWpntHuntTarget: Could not initiate hunt.\n", 0, 0, 0, 0);
        }

        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)oldmode);
    }
}

void J3DAPI sithCogFunctionAI_AISetInstinctWpntMode(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 2211
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI THING in AISetInstinctWpntMode().\n", 0, 0, 0, 0);
    }
    else
    {
        pLocal->mode |= SITHAI_MODE_INSTINCTUSEWPNTS;
    }
}

void J3DAPI sithCogFunctionAI_AIClearInstinctWpntMode(SithCog* pCog)
{
    SithAIControlBlock* pLocal;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->controlType != SITH_CT_AI || !(pLocal = pThing->controlInfo.aiControl.pLocal) )
    {
#line 2228
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI THING in AIClearInstinctWpntMode().\n", 0, 0, 0, 0);
    }
    else
    {
        pLocal->mode &= ~SITHAI_MODE_INSTINCTUSEWPNTS;
    }
}

void J3DAPI sithCogFunctionAI_AIFleeToWpnt(SithCog* pCog)
{
    SithAIMode curoldmode;
    SithAIControlBlock* pLocal;
    SithThing* pCurWpnt;

    int index = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing
        || pThing->type != SITH_THING_ACTOR
        || pThing->controlType != SITH_CT_AI
        || !pThing->controlInfo.aiControl.pLocal )
    {
#line 2262
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid AI thing in AIFlee.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pCurWpnt = sithAIUtil_AIGetWpntThing(index);
        pLocal = pThing->controlInfo.aiControl.pLocal;
        pLocal->pFleeFromThing = pCurWpnt;
        curoldmode = pLocal->mode;
        sithAIUtil_AISetMode(pLocal, SITHAI_MODE_FLEEINGTOWAYPOINT);
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_MODECHANGED, (void*)curoldmode);
    }
}

void J3DAPI sithCogFunctionAI_AISpat(SithCog* pCog)
{
    int curtimerId       = sithCogExec_PopInt(pCog);
    float duration    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

#line 2300
    SITH_ASSERTREL(pThing);
#line 2301
    SITH_ASSERTREL(pThing->renderData.pPuppet);
#line 2302
    SITH_ASSERTREL(pThing->controlType == SITH_CT_AI);
#line 2303
    SITH_ASSERTREL(pThing->controlInfo.aiControl.pLocal);

    if ( duration > 0.0f )
    {
        SithEventParams param;

        sithPuppet_RemoveAllTracks(pThing);
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);

        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        pThing->moveStatus = SITHPLAYERMOVE_RUNNING;

        param.param3 = 0;
        param.param2 = pThing->idx;
        param.param1 = curtimerId;
        param.idx = pCog->idx;
        sithEvent_CreateEvent(SITHCOG_TASKID, &param, (uint32_t)(duration * 1000.0f));

        pThing->controlInfo.aiControl.pLocal->mode |= SITHAI_MODE_DISABLED;
    }
    else
    {
        pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_DISABLED;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
}
