#include <sith/Gameplay/sithVehicleControls.h>

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithControl.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>

/* The debug build copies a member operand of these absolute values with `fld; fstp` (a parenthesised ternary arm):
 * a macro; name inferred, as in the other sith files. */
#define SITHVEHICLECONTROLS_ABS(x) ((x) < 0.0f ? -(x) : (x))

SithMineCarControlState sithVehicleControls_curMineCarState = { 0.0f, 0, 0.0f, -1, 0, 0, -1, 0, 0.0f, 0.0f, 0 };
SithRaftControlState sithVehicleControls_curRaftState = {
    SITHPLAYERMOVE_RAFT_STILL,
    SITHPUPPETSUBMODE_STAND,
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    0.0f,
    0.0f,
    0,
    0.0f,
    0.0f
};


SithMineCarControlState* J3DAPI sithVehicleControls_GetMineCarControlState(void)
{
    return &sithVehicleControls_curMineCarState;
}

void J3DAPI sithVehicleControls_SetMineCarControlState(SithMineCarControlState* pState)
{
    sithVehicleControls_curMineCarState = *pState;
}

SithRaftControlState* J3DAPI sithVehicleControls_GetRaftControlState(void)
{
    return &sithVehicleControls_curRaftState;
}

void J3DAPI sithVehicleControls_SetRaftControlState(SithRaftControlState* pState)
{
    sithVehicleControls_curRaftState = *pState;
}

void J3DAPI sithVehicleControls_PuppetCallback(SithThing* pPuppetThing, int track, rdKeyMarkerType markerType)
{
    static float sithVehicleControls_raftBoardPushSpeed = 0.04f;
    static float sithVehicleControls_raftBoardRotVelocity = 67.0f;
    SithThing* pThing = pPuppetThing;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

    if ( !pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        return;
    }

    if ( markerType == RDKEYMARKER_REACHRIGHTARMREST )
    {
        sithFX_CreateRipple(pThing, 0.02f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_PICKUP )
    {
        sithFX_CreateRipple(pThing, 0.04f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_DROP )
    {
        sithFX_CreateRipple(pThing, 0.06f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_MOVE )
    {
        sithFX_CreateRipple(pThing, 0.08f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_INVENTORYPULL )
    {
        sithFX_CreateRipple(pThing, 0.1f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_INVENTORYPUT )
    {
        sithFX_CreateRipple(pThing, 0.12f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_ATTACKFINISH )
    {
        sithFX_CreateRipple(pThing, 0.14f);
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_ROW )
    {
        if ( (pThing->flags & SITH_TF_DYING) != 0 )
        {
            if ( (int)pThing->userval == 3 )
            {
                sithFX_CreateRaftRipple(pThing, 0);
            }
        }
        else
        {
            sithVehicleControls_curRaftState.bRowing = 1;
            sithVehicleControls_curRaftState.secRowStartTime = sithTime_secGameTime;
            sithFX_CreateRowWaterFx(pThing, sithTime_secGameTime);
            sithVehicleControls_UpdateRaftPaddleSoundFx(pThing);
        }

        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_ROWFINISH )
    {
        sithVehicleControls_curRaftState.bRowing = 0;
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( markerType == RDKEYMARKER_CLIMB )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_TURN_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_TURN_RIGHT )
        {
            if ( pThing->moveStatus != sithVehicleControls_curRaftState.moveStatus )
            {
                SithThingMoveStatus prevMoveStatus = pThing->moveStatus;
                pThing->moveStatus = sithVehicleControls_curRaftState.moveStatus;
                sithVehicleControls_PlayPuppetMode(pThing, sithVehicleControls_curRaftState.puppetSubmode);
                sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
            }
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT
            || pThing->moveStatus == SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT )
        {
            SithThingMoveStatus prevMoveStatus = pThing->moveStatus;
            pThing->moveStatus = sithVehicleControls_curRaftState.moveStatus;
            sithVehicleControls_PlayPuppetMode(pThing, sithVehicleControls_curRaftState.puppetSubmode);
            sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_45 )
        {
            sithVehicleControls_PlayPuppetMode(pThing, sithVehicleControls_curRaftState.puppetSubmode);
            sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
        }
        else
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, track, 0.0f);
        }

        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_45 && !markerType )
    {
        if ( sithVehicleControls_curRaftState.moveStatus == SITHPLAYERMOVE_RAFT_UNBOARD_START )
        {
            pThing->moveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
        }
        else
        {
            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
        }

        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT
        || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT
        || pThing->moveStatus == SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT
        || pThing->moveStatus == SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT
        || pThing->moveStatus == SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT
        || pThing->moveStatus == SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT )
    {
        if ( !markerType && pThing->moveStatus != sithVehicleControls_curRaftState.moveStatus )
        {
            SithThingMoveStatus prevMoveStatus = pThing->moveStatus;
            pThing->moveStatus = sithVehicleControls_curRaftState.moveStatus;
            sithVehicleControls_PlayPuppetMode(pThing, sithVehicleControls_curRaftState.puppetSubmode);
        }

        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_LAND )
    {
        if ( !markerType )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithPuppet_FreeTrackByIndex(pThing, track);
            if ( (pPhysics->flags & SITH_PF_RAFT) != 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STILL;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_STAND);
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }
        }
        else
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_BOARDING )
    {
        if ( !markerType )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, 22, 0, 0);
            sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, 30, 0, 0);
            pThing->moveStatus = SITHPLAYERMOVE_RAFT_STILL;
            sithVehicleControls_SendMessageUser1(pThing);
        }
        else if ( markerType == RDKEYMARKER_DUCK )
        {
            rdVector_Scale3(&pThing->moveInfo.physics.velocity, &pThing->orient.rvec, -sithVehicleControls_raftBoardPushSpeed);
            pThing->moveInfo.physics.angularVelocity.y = sithVehicleControls_raftBoardRotVelocity;
            if ( (int)pThing->userval == 3 )
            {
                sithFX_CreateRaftRipple(pThing, 0);
            }
        }

        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT || pThing->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT )
    {
        if ( !markerType )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithVehicleControls_ExitRaft(pThing);
        }
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING )
    {
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
        if ( !markerType )
        {
            sithVehicleControls_SendMessageUser1(pThing);
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            pThing->moveStatus = SITHPLAYERMOVE_MINECAR_IDLE;
            pThing->thingInfo.actorInfo.flags |= SITH_AF_HEADLIGHT;
            pThing->thingInfo.actorInfo.headLightIntensity.alpha = 2.0f;
        }
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING )
    {
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STARTMOVE);
        if ( !markerType )
        {
            sithVehicleControls_SendMessageUser1(pThing);
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            pThing->moveStatus = SITHPLAYERMOVE_JEEP_STILL;
        }
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING )
    {
        if ( !markerType )
        {
            sithPlayerControls_ExitVehicle(pThing);
        }
        sithPuppet_DefaultCallback(pThing, track, markerType);
    }
    else
    {
        if ( (pPhysics->flags & SITH_PF_JEEP) != 0 && markerType == RDKEYMARKER_DIED )
        {
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_DEATH1);
        }

        if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT )
        {
            if ( !markerType )
            {
                sithPlayerControls_ExitVehicle(pThing);
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKING )
        {
            if ( markerType == RDKEYMARKER_DUCK )
            {
                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_DUCKED;
                sithVehicleControls_curMineCarState.secDuckTime = sithTime_secGameTime;
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKED )
        {
            if ( !markerType )
            {
                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_GETTING_UP;
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_GETTING_UP )
        {
            if ( !markerType )
            {
                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                pThing->moveStatus = SITHPLAYERMOVE_MINECAR_IDLE;
                pThing->moveInfo.physics.flags &= ~SITH_PF_CROUCHING;
            }
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
}

void J3DAPI sithVehicleControls_ProcessMineCarPlayerMove(SithThing* pThing, float secDeltaTime)
{
    static float sithVehicleControls_mineCarLeanStartAngVel = 25.0f;
    static float sithVehicleControls_mineCarLeanStopAngVel = 20.0f;
    static float sithVehicleControls_mineCarLeanMinSpeed = 0.7f;
    static float sithVehicleControls_mineCarLeanStopSpeed = 0.6f;
    static float sithVehicleControls_mineCarBreakSoundDelay = 0.2f;
    static float sithVehicleControls_mineCarBreakSoundFade = 0.6f;
    SithMineCarState* pState;
    float maxThrust;
    float maxRotVel;
    float maxFwdVel;
    float maxBackVel;
    float backSpeed;
    float brakeFwdVel;
    float brakeBackVel;
    float secDuckDelay;
    float thrust;
    float thrustY;
    int bMove;
    int bForward;
    int bExit;
    int bExitLeft;
    int bCanMoveRight;
    int bCanMoveLeft;
    rdVector3 dir;
    float maxVel;
    float jumpSpeed;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;  // unreferenced; the debug build's frame keeps its slot

    if ( !sithPhysics_CreateMineCarUserBlock(pThing) )
    {
        return;
    }

    pState = &pThing->userblock.pMinecar->state;
    if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
        || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT )
    {
        sithVehicleControls_curMineCarState.accelerationState = 0;
        sithVehicleControls_curMineCarState.indyLeanState     = 0;
        sithVehicleControls_curMineCarState.unbaordState      = 0;
        return;
    }

    if ( (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f)
        && sithVehicleControls_curMineCarState.accelerationState == 0 )
    {
        sithVehicleControls_curMineCarState.accelerationState = 4;
    }

    if ( !sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL) && pThing->moveStatus == SITHPLAYERMOVE_MINECAR_IDLE )
    {
        sithVehicleControls_curMineCarState.bNotDucked = 1;
    }

    pPhysics->thrust.x = 0.0f;
    pPhysics->thrust.z = 0.0f;

    maxVel       = pPhysics->maxVelocity;
    jumpSpeed    = pThing->thingInfo.actorInfo.jumpSpeed;
    backSpeed    = pPhysics->orientSpeed;
    maxThrust    = pThing->thingInfo.actorInfo.maxThrust;
    maxRotVel    = pThing->thingInfo.actorInfo.maxRotVelocity;
    secDuckDelay = pPhysics->airDrag;
    maxFwdVel    = RD_MULTIPLY(pPhysics->surfDrag, maxVel);
    maxBackVel   = -(RD_MULTIPLY(pPhysics->surfDrag, backSpeed));
    brakeFwdVel  = RD_MULTIPLY(pPhysics->surfDrag + 1.0f, jumpSpeed);
    brakeBackVel = -(RD_MULTIPLY(pPhysics->surfDrag + 1.0f, backSpeed));
    thrust = 0.0f;
    bMove = 0;

    pPhysics->velocity.x = stdMath_ClipNearZero(pPhysics->velocity.x);
    pPhysics->velocity.y = stdMath_ClipNearZero(pPhysics->velocity.y);
    pPhysics->velocity.z = stdMath_ClipNearZero(pPhysics->velocity.z);
    if ( pPhysics->velocity.x == 0.0f
        && pPhysics->velocity.y == 0.0f
        && pPhysics->velocity.z == 0.0f
        && sithVehicleControls_curMineCarState.accelerationState != 1 )
    {
        sithVehicleControls_curMineCarState.accelerationState = 0;
    }

    if ( sithVehicleControls_curMineCarState.indyLeanState == 0
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED )
    {
        if ( sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) > sithVehicleControls_mineCarLeanMinSpeed )
        {
            if ( pPhysics->angularVelocity.y > sithVehicleControls_mineCarLeanStartAngVel )
            {
                sithVehicleControls_curMineCarState.curPuppetTrack = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND2WALK, NULL);
                sithVehicleControls_curMineCarState.indyLeanState  = 1;
            }
            else if ( -sithVehicleControls_mineCarLeanStartAngVel > pPhysics->angularVelocity.y )
            {
                sithVehicleControls_curMineCarState.curPuppetTrack = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WALK2STAND, NULL);
                sithVehicleControls_curMineCarState.indyLeanState  = 2;
            }
        }
    }
    else if ( sithVehicleControls_curMineCarState.indyLeanState == 1 )
    {
        if ( pPhysics->angularVelocity.y < sithVehicleControls_mineCarLeanStopAngVel
            || sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) < sithVehicleControls_mineCarLeanStopSpeed
            || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKING )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithVehicleControls_curMineCarState.curPuppetTrack, 0.15f);
            sithVehicleControls_curMineCarState.indyLeanState = 0;
        }
    }
    else if ( sithVehicleControls_curMineCarState.indyLeanState == 2 )
    {
        if ( -sithVehicleControls_mineCarLeanStopAngVel < pPhysics->angularVelocity.y
            || sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) < sithVehicleControls_mineCarLeanStopSpeed
            || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKING )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithVehicleControls_curMineCarState.curPuppetTrack, 0.15f);
            sithVehicleControls_curMineCarState.indyLeanState = 0;
        }
    }

    if ( sithVehicleControls_curMineCarState.accelerationState == 1 )
    {
        sithVehicleControls_curMineCarState.secStoppingElapsedTime += secDeltaTime;
        if ( sithVehicleControls_curMineCarState.secStoppingElapsedTime > 0.4f )
        {
            sithVehicleControls_curMineCarState.accelerationState = 0;
        }
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_DUCKED && sithTime_secGameTime - sithVehicleControls_curMineCarState.secDuckTime > secDuckDelay )
    {
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_SLIDEDOWNBACK, sithVehicleControls_PuppetCallback);
        if ( sithVehicleControls_curMineCarState.curDuckPuppetTrackNum != -1 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithVehicleControls_curMineCarState.curDuckPuppetTrackNum, 0.1f);
        }
        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        return;
    }

    if ( sithVehicleControls_curMineCarState.unbaordState == 0 )
    {
        if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
        {
            sithVehicleControls_curMineCarState.unbaordState = 1;
        }
    }
    else if ( sithVehicleControls_curMineCarState.unbaordState == 1 )
    {
        if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL)
            && (sithVehicleControls_curMineCarState.accelerationState == 0 || sithVehicleControls_curMineCarState.accelerationState == 1)
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED )
        {
            sithVehicleControls_curMineCarState.secUnboardingElapsedTime = 0.0f;
            sithVehicleControls_curMineCarState.unbaordState = 3;
            sithPlayerControls_bActionActivated = 1;
        }
    }
    else if ( sithVehicleControls_curMineCarState.unbaordState >= 3 )
    {
        sithVehicleControls_curMineCarState.secUnboardingElapsedTime += secDeltaTime;
        if ( sithVehicleControls_curMineCarState.secUnboardingElapsedTime >= 0.2f
            || sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL)
            || sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL)
            || !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
        {
            bExit = 0;
            rdVector_Copy3(&dir, &pThing->orient.rvec);
            bCanMoveRight = sithVehicleControls_CanMoveInDirection(pThing, &dir);
            rdVector_Neg3(&dir, &pThing->orient.rvec);
            bCanMoveLeft = sithVehicleControls_CanMoveInDirection(pThing, &dir);

            if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                if ( bCanMoveLeft )
                {
                    bExit     = 1;
                    bExitLeft = 1;
                }
                else
                {
                    sithVehicleControls_curMineCarState.unbaordState = 4;
                    sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
                }
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                if ( bCanMoveRight )
                {
                    bExit     = 1;
                    bExitLeft = 0;
                }
                else
                {
                    sithVehicleControls_curMineCarState.unbaordState = 4;
                    sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
                }
            }
            else if ( sithVehicleControls_curMineCarState.unbaordState == 3 )
            {
                if ( bCanMoveLeft )
                {
                    bExit     = 1;
                    bExitLeft = 1;
                }
                else if ( bCanMoveRight )
                {
                    bExit     = 1;
                    bExitLeft = 0;
                }
                else
                {
                    sithVehicleControls_curMineCarState.unbaordState = 0;
                    sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
                }
            }

            if ( bExit )
            {
                if ( bExitLeft )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STRAFELEFT, sithVehicleControls_PuppetCallback);
                }
                else
                {
                    pThing->moveStatus = SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STRAFERIGHT, sithVehicleControls_PuppetCallback);
                }

                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE);
                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                sithVehicleControls_curMineCarState.unbaordState = 0;
                sithVehicleControls_SendMessageUser0(pThing);
            }
            else if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
            {
                sithVehicleControls_curMineCarState.unbaordState = 1;
            }

            return;
        }
    }

    if ( sithVehicleControls_curMineCarState.unbaordState >= 3 )
    {
        return;
    }

    if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL) && sithVehicleControls_curMineCarState.bNotDucked )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_IDLE )
        {
            sithVehicleControls_curMineCarState.curDuckPuppetTrackNum = sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_SLIDEDOWNFWD, sithVehicleControls_PuppetCallback);
            pThing->moveStatus = SITHPLAYERMOVE_MINECAR_DUCKING;
            pThing->moveInfo.physics.flags |= SITH_PF_CROUCHING;
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            sithVehicleControls_curMineCarState.bNotDucked = 0;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        if ( pPhysics->thrust.y < maxFwdVel
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
            && sithVehicleControls_curMineCarState.indyLeanState == 0 )
        {
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATEHIGH, NULL);
        }

        bMove = 1;
        if ( pPhysics->thrust.y >= 0.0f )
        {
            bForward = 1;
            thrust   = maxThrust;
        }
        else
        {
            bForward = 0;
            thrust   = maxRotVel;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( pPhysics->thrust.y > maxBackVel
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKING
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_DUCKED
            && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_GETTING_UP
            && sithVehicleControls_curMineCarState.indyLeanState == 0 )
        {
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, NULL);
        }

        bMove = 1;
        if ( pPhysics->thrust.y <= 0.0f )
        {
            bForward = 1;
            thrust   = -maxThrust;
        }
        else
        {
            bForward = 0;
            thrust   = -maxRotVel;
        }
    }
    else if ( sithVehicleControls_curMineCarState.accelerationState == 3 )
    {
        bMove    = 1;
        bForward = 1;
        if ( pPhysics->thrust.y >= 0.0f )
        {
            thrust = maxThrust;
        }
        else
        {
            thrust = -maxThrust;
        }
    }
    else if ( sithVehicleControls_curMineCarState.accelerationState == 2 )
    {
        bMove    = 1;
        bForward = 0;
        if ( pPhysics->thrust.y >= 0.0f )
        {
            thrust = -(RD_MULTIPLY(maxRotVel, 0.5f));
        }
        else
        {
            thrust = RD_MULTIPLY(maxRotVel, 0.5f);
        }
    }

    if ( bMove )
    {
        thrust = RD_MULTIPLY(thrust, secDeltaTime);
        if ( bForward )
        {
            sithVehicleControls_curMineCarState.secBreakingElapsedTime = 0.0f;
            thrustY = pPhysics->thrust.y;
            if ( thrustY < brakeFwdVel && thrustY > brakeBackVel )
            {
                sithVehicleControls_curMineCarState.accelerationState = 3;
            }

            stdMath_ClipNearZero(thrustY);
            if ( thrustY == 0.0f )
            {
                pPhysics->thrust.y = thrust;
            }
            else
            {
                thrustY = thrustY + thrust;
                if ( thrustY > maxFwdVel )
                {
                    thrustY = maxFwdVel;
                }
                else if ( thrustY < maxBackVel )
                {
                    thrustY = maxBackVel;
                }
                pPhysics->thrust.y = thrustY;
            }

            if ( sithVehicleControls_curMineCarState.accelerationState == 3 && (thrustY >= brakeFwdVel || thrustY <= brakeBackVel) )
            {
                sithVehicleControls_curMineCarState.accelerationState = 4;
            }
        }
        else
        {
            sithVehicleControls_curMineCarState.secBreakingElapsedTime += secDeltaTime;
            if ( sithVehicleControls_curMineCarState.secBreakingElapsedTime > sithVehicleControls_mineCarBreakSoundDelay && !sithVehicleControls_curMineCarState.hBreakSnd )
            {
                sithVehicleControls_curMineCarState.hBreakSnd = sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_RRUNMETAL);
                pState->bUpdateSparks = 1;
            }

            thrustY = pPhysics->thrust.y;
            if ( (thrustY < 0.0f ? -thrustY : thrustY) < (thrust < 0.0f ? -thrust : thrust) )
            {
                memset(&pPhysics->thrust, 0, sizeof(rdVector3));
                memset(&pPhysics->velocity, 0, sizeof(rdVector3));
                sithVehicleControls_curMineCarState.accelerationState = 1;
                sithVehicleControls_curMineCarState.secStoppingElapsedTime = 0.0f;
                sithVehicleControls_curMineCarState.secBreakingElapsedTime = 0.0f;
            }
            else
            {
                thrustY = thrustY + thrust;
                pPhysics->thrust.y = thrustY;
                if ( thrustY < brakeFwdVel && thrustY > brakeBackVel )
                {
                    sithVehicleControls_curMineCarState.accelerationState = 2;
                }
            }
        }
    }
    else
    {
        sithVehicleControls_curMineCarState.secBreakingElapsedTime = 0.0f;
    }

    if ( sithVehicleControls_curMineCarState.secBreakingElapsedTime == 0.0f && sithVehicleControls_curMineCarState.hBreakSnd )
    {
        sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RRUNMETAL, 0.0f, sithVehicleControls_mineCarBreakSoundFade);
        sithVehicleControls_curMineCarState.hBreakSnd = 0;
        if ( pState->bUpdateSparks )
        {
            pState->bUpdateSparks      = 0;
            pState->bUpdateRightSparks = 0;
            pState->bUpdateLeftSparks  = 0;
        }
    }
}

void J3DAPI sithVehicleControls_ProcessJeepPlayerMove(SithThing* pThing, float secDeltaTime)
{
    static float sithVehicleControls_jeepSurfDrag = 2.0f;
    static float sithVehicleControls_jeepStaticDrag = 0.02f;
    static float sithVehicleControls_jeepForceStopSurfDrag = 2.0f;
    static float sithVehicleControls_jeepForceStopStaticDrag = 0.05f;
    static float sithVehicleControls_jeepMaxVelocity = 1.0f;
    static float sithVehicleControls_jeepAccelMaxVelocity = 1.98f;
    static float sithVehicleControls_jeepReverseThrustScale = 0.35f;
    static float sithVehicleControls_jeepReverseBrakeThrust = -2.0f;
    static float sithVehicleControls_jeepBrakeThrust = 2.0f;
    static float sithVehicleControls_jeepAccel = 0.35f;
    static float sithVehicleControls_jeepBoostAccel = 0.5f;
    static float sithVehicleControls_jeepMaxRotVelocity = 150.0f;
    static float sithVehicleControls_jeepRotDecel = 600.0f;
    static float sithVehicleControls_jeepRotThrustScale = 3.0f;
    static float sithVehicleControls_jeepFullTurnSpeed = 0.4f;
    static float sithVehicleControls_jeepHeight = 0.18f;
    static float sithVehicleControls_jeepMoveSize = 0.165f;
    static float sithVehicleControls_jeepAccelSndVolume = 0.5f;
    static float sithVehicleControls_jeepWheelRadius = 0.0375f;
    static float sithVehicleControls_jeepExitMaxSpeed = 0.2f;
    static float sithVehicleControls_jeepExitMaxAngVel = 3.0f;
    static float sithVehicleControls_jeepForceStopTime = 0;
    static int sithVehicleControls_bJeepForceStopped = 0;
    static int sithVehicleControls_bJeepAccMoveSndFxPlayed = 0;
    static int sithVehicleControls_bJeepStartMoveSndFxPlayed = 0;
    float maxRotVelocity;
    float accel;
    float moveScale = 1.8f;
    float reverseScale;
    float fwdSpeed;
    float rotVelocity;
    float maxThrust;
    SithActorInfo* pActorInfo;
    SithPhysicsInfo* pPhysics;
    float velScale;
    float maxVelocity;
    float brakeThrust;
    float reverseDelta;
    float rotThrust;
    float minThrust;
    float thrustDelta;
    float reverseBrakeThrust;
    float wheelCircumference;
    float dot;
    float speed;
    float wheelAngle;
    int jointIdx;
    rdVector3 exitDir;
    float exitSpeed;
    float mouseTurn;

    pPhysics   = &pThing->moveInfo.physics;
    pActorInfo = &pThing->thingInfo.actorInfo;
    if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING
        || pThing->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING
        || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_83
        || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_84
        || pThing->moveStatus == SITHPLAYERMOVE_JEEP_IMPACT )
    {
        return;
    }

    pPhysics->height              = sithVehicleControls_jeepHeight;
    pThing->collide.movesize      = sithVehicleControls_jeepMoveSize;
    pPhysics->surfDrag            = sithVehicleControls_jeepSurfDrag;
    pPhysics->staticDrag          = sithVehicleControls_jeepStaticDrag;
    pPhysics->maxVelocity         = sithVehicleControls_jeepMaxVelocity;
    pPhysics->maxRotationVelocity = sithVehicleControls_jeepMaxRotVelocity;
    pPhysics->flags |= SITH_PF_USEANGULARTHRUST;

    if ( sithVehicleControls_bJeepForceStopped )
    {
        pPhysics->surfDrag   = sithVehicleControls_jeepForceStopSurfDrag;
        pPhysics->staticDrag = sithVehicleControls_jeepForceStopStaticDrag;
        if ( pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f )
        {
            sithVehicleControls_jeepForceStopTime = 0.0f;
        }
        else
        {
            sithVehicleControls_jeepForceStopTime += secDeltaTime;
            if ( sithVehicleControls_jeepForceStopTime > 0.1f )
            {
                sithVehicleControls_bJeepForceStopped = 0;
            }
        }
    }
    else
    {
        sithVehicleControls_jeepForceStopTime = 0.0f;
    }

    // Rotate wheels
    wheelCircumference = RD_MULTIPLY(RD_MULTIPLY(3.14159f, sithVehicleControls_jeepWheelRadius), 2.0f);
    if ( (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f)
        && !sithVehicleControls_bJeepForceStopped )
    {
        speed = D3DVAL(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) / wheelCircumference);
        wheelAngle = RD_MULTIPLY(RD_MULTIPLY(360.0f, speed), secDeltaTime);
        wheelAngle = stdMath_NormalizeAngle(wheelAngle);
        dot = rdVector_Dot3(&pPhysics->velocity, &pThing->orient.lvec);
        if ( dot < 0.0f )
        {
            wheelAngle = RD_MULTIPLY(wheelAngle, -1.0f);
        }

        jointIdx = sithThing_GetThingJointIndex(pThing, "brwheel");
        if ( jointIdx != -1 )
        {
            pThing->renderData.apTweakedAngles[jointIdx].x = pThing->renderData.apTweakedAngles[jointIdx].x - wheelAngle;
        }
        else
        {
#line 1116
            stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Jeep wheel node missing.\n", 0, 0, 0, 0);
        }

        jointIdx = sithThing_GetThingJointIndex(pThing, "blwheel");
        if ( jointIdx != -1 )
        {
            pThing->renderData.apTweakedAngles[jointIdx].x = pThing->renderData.apTweakedAngles[jointIdx].x - wheelAngle;
        }
        else
        {
#line 1126
            stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Jeep wheel node missing.\n", 0, 0, 0, 0);
        }

        jointIdx = sithThing_GetThingJointIndex(pThing, "frwheel");
        if ( jointIdx != -1 )
        {
            pThing->renderData.apTweakedAngles[jointIdx].x = pThing->renderData.apTweakedAngles[jointIdx].x - wheelAngle;
        }
        else
        {
#line 1136
            stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Jeep wheel node missing.\n", 0, 0, 0, 0);
        }

        jointIdx = sithThing_GetThingJointIndex(pThing, "flwheel");
        if ( jointIdx != -1 )
        {
            pThing->renderData.apTweakedAngles[jointIdx].x = pThing->renderData.apTweakedAngles[jointIdx].x - wheelAngle;
        }
        else
        {
#line 1146
            stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Jeep wheel node missing.\n", 0, 0, 0, 0);
        }
    }

    if ( sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        accel       = sithVehicleControls_jeepBoostAccel;
        maxVelocity = sithVehicleControls_jeepAccelMaxVelocity;
        if ( !sithVehicleControls_bJeepAccMoveSndFxPlayed )
        {
            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKSNOW, sithVehicleControls_jeepAccelSndVolume);
            sithVehicleControls_bJeepAccMoveSndFxPlayed = 1;
        }
    }
    else
    {
        accel       = sithVehicleControls_jeepAccel;
        maxVelocity = pPhysics->maxVelocity;
        sithVehicleControls_bJeepAccMoveSndFxPlayed = 0;
    }

    reverseScale       = sithVehicleControls_jeepReverseThrustScale;
    maxThrust          = RD_MULTIPLY(pPhysics->surfDrag, maxVelocity);
    minThrust          = -(RD_MULTIPLY(pPhysics->surfDrag, reverseScale));
    reverseBrakeThrust = sithVehicleControls_jeepReverseBrakeThrust;
    brakeThrust        = sithVehicleControls_jeepBrakeThrust;
    maxRotVelocity     = pPhysics->maxRotationVelocity;

    velScale       = D3DVAL(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) / sithVehicleControls_jeepFullTurnSpeed);
    velScale       = STDMATH_CLAMP(velScale, 0.0f, 1.0f);
    maxRotVelocity = RD_MULTIPLY(maxRotVelocity, velScale);
    rotVelocity    = RD_MULTIPLY(pPhysics->surfDrag, maxRotVelocity);

    if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !sithPlayerControls_bActionActivated )
    {
        rdVector_Neg3(&exitDir, &pThing->orient.rvec);
        exitSpeed = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));
        if ( exitSpeed <= sithVehicleControls_jeepExitMaxSpeed )
        {
            memset(&pPhysics->velocity, 0, sizeof(rdVector3));
        }

        if ( SITHVEHICLECONTROLS_ABS(pPhysics->angularVelocity.x) <= sithVehicleControls_jeepExitMaxAngVel )
        {
            pPhysics->angularVelocity.x = 0.0f;
        }

        if ( SITHVEHICLECONTROLS_ABS(pPhysics->angularVelocity.y) <= sithVehicleControls_jeepExitMaxAngVel )
        {
            pPhysics->angularVelocity.y = 0.0f;
        }

        if ( SITHVEHICLECONTROLS_ABS(pPhysics->angularVelocity.z) <= sithVehicleControls_jeepExitMaxAngVel )
        {
            pPhysics->angularVelocity.z = 0.0f;
        }

        sithPlayerControls_bActionActivated = 1;
        if ( pPhysics->velocity.x == 0.0f && pPhysics->velocity.y == 0.0f && pPhysics->velocity.z == 0.0f
            && pPhysics->angularVelocity.x == 0.0f && pPhysics->angularVelocity.y == 0.0f && pPhysics->angularVelocity.z == 0.0f
            && sithVehicleControls_CanMoveInDirection(pThing, &exitDir) )
        {
            sithVehicleControls_FadeJeepMoveSounds(pThing);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_STOPMOVE);
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_PULLITEM, sithVehicleControls_PuppetCallback);
            pThing->moveStatus = SITHPLAYERMOVE_JEEP_UNBOARDING;
            sithVehicleControls_SendMessageUser0(pThing);
            return;
        }

        sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
    }

    if ( pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f )
    {
        fwdSpeed = rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity);
    }
    else
    {
        fwdSpeed = 0.0f;
    }

    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        if ( fwdSpeed >= 0.0f && !sithVehicleControls_bJeepForceStopped )
        {
            if ( !sithVehicleControls_bJeepStartMoveSndFxPlayed )
            {
                sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKSNOW, sithVehicleControls_jeepAccelSndVolume);
            }

            sithVehicleControls_bJeepStartMoveSndFxPlayed = 1;
            thrustDelta = RD_MULTIPLY(maxThrust, secDeltaTime);
            thrustDelta = RD_MULTIPLY(thrustDelta, accel);
            pPhysics->thrust.y += thrustDelta;
            if ( pPhysics->thrust.y > maxThrust )
            {
                pPhysics->thrust.y = maxThrust;
            }
        }
        else
        {
            sithVehicleControls_bJeepForceStopped = 1;
            pPhysics->thrust.y = 0.0f;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( fwdSpeed <= 0.0f && !sithVehicleControls_bJeepForceStopped )
        {
            reverseDelta = RD_MULTIPLY(minThrust, secDeltaTime);
            reverseDelta = RD_MULTIPLY(reverseDelta, accel);
            pPhysics->thrust.y += reverseDelta;
            if ( pPhysics->thrust.y < minThrust )
            {
                pPhysics->thrust.y = minThrust;
            }
        }
        else
        {
            sithVehicleControls_bJeepForceStopped = 1;
            pPhysics->thrust.y = 0.0f;
        }
    }
    else
    {
        if ( fwdSpeed < 0.0f )
        {
            thrustDelta = RD_MULTIPLY(RD_MULTIPLY(brakeThrust, 0.5f), secDeltaTime);
            if ( pPhysics->thrust.y < (-thrustDelta) )
            {
                pPhysics->thrust.y += thrustDelta;
            }
            else
            {
                pPhysics->thrust.y = 0.0f;
            }
        }
        else if ( fwdSpeed > 0.0f )
        {
            reverseDelta = RD_MULTIPLY(RD_MULTIPLY(reverseBrakeThrust, 0.5f), secDeltaTime);
            if ( pPhysics->thrust.y > (-reverseDelta) )
            {
                pPhysics->thrust.y += reverseDelta;
            }
            else
            {
                pPhysics->thrust.y = 0.0f;
            }
        }

        sithVehicleControls_bJeepForceStopped         = 0;
        sithVehicleControls_bJeepStartMoveSndFxPlayed = 0;
    }

    rotThrust = pPhysics->rotThrust.y;
    if ( pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_82
        && (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f) )
    {
        if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            if ( rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity) >= 0.0f )
            {
                if ( rotThrust > 0.0f )
                {
                    rotThrust = 0.0f;
                }

                rotThrust = RD_MULTIPLY(rotVelocity, secDeltaTime);
                rotThrust = RD_MULTIPLY(rotThrust, sithVehicleControls_jeepRotThrustScale);
                pPhysics->rotThrust.y -= rotThrust;
            }
            else
            {
                rotThrust = RD_MULTIPLY(rotVelocity, secDeltaTime);
                rotThrust = RD_MULTIPLY(rotThrust, sithVehicleControls_jeepRotThrustScale);
                pPhysics->rotThrust.y += rotThrust;
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            if ( rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity) >= 0.0f )
            {
                if ( rotThrust < 0.0f )
                {
                    rotThrust = 0.0f;
                }

                rotThrust = RD_MULTIPLY(rotVelocity, secDeltaTime);
                rotThrust = RD_MULTIPLY(rotThrust, sithVehicleControls_jeepRotThrustScale);
                pPhysics->rotThrust.y += rotThrust;
            }
            else
            {
                rotThrust = RD_MULTIPLY(rotVelocity, secDeltaTime);
                rotThrust = RD_MULTIPLY(rotThrust, sithVehicleControls_jeepRotThrustScale);
                pPhysics->rotThrust.y -= rotThrust;
            }
        }
        else
        {
            rotThrust = RD_MULTIPLY(sithVehicleControls_jeepRotDecel, secDeltaTime);
            pPhysics->rotThrust.y = 0.0f;
            if ( SITHVEHICLECONTROLS_ABS(rotThrust) >= SITHVEHICLECONTROLS_ABS(pPhysics->angularVelocity.y) )
            {
                pPhysics->angularVelocity.y = 0.0f;
            }
            else
            {
                pPhysics->angularVelocity.y -= RD_MULTIPLY((pPhysics->angularVelocity.y < 0.0f ? -1 : 1), rotThrust);
            }
        }
    }

    pPhysics->rotThrust.y = STDMATH_CLAMP(pPhysics->rotThrust.y, (-rotVelocity), rotVelocity);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR)
        && (pPhysics->velocity.x != 0.0f || pPhysics->velocity.y != 0.0f || pPhysics->velocity.z != 0.0f) )
    {
        mouseTurn = RD_MULTIPLY(sithControl_GetAxis(SITHCONTROL_MOUSETURN), sithTime_fps);
        mouseTurn = RD_MULTIPLY(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pActorInfo->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
        pPhysics->angularVelocity.y += mouseTurn;
    }
}

void J3DAPI sithVehicleControls_PlayPuppetMode(SithThing* pThing, SithPuppetSubMode submode)
{
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;

    sithPuppet_PlayMode(pThing, submode, sithVehicleControls_PuppetCallback);
    if ( submode != SITHPUPPETSUBMODE_STAND )
    {
        pActorInfo->bControlsDisabled = 1;
    }
    else
    {
        pActorInfo->bControlsDisabled = 0;
    }

    sithVehicleControls_curRaftState.bRowing = 0;
}

static void J3DAPI sithVehicleControls_SearchWallSurface(SithThing* pThing, SithVehicleSurfaceSearch* pSearch, float moveDist)
{
    static float sithVehicleControls_surfaceSearchMaxNormalZ = 0.02f;
    static float sithVehicleControls_surfaceSearchMaxDot = -0.906f;
    SithCollision* pCollision;
    float curdot;

    pSearch->pSurface = NULL;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pSearch->dir, moveDist, 0.001f, 2);
    pCollision = sithCollision_PopStack();
    if ( pCollision && (pCollision->type & SITHCOLLISION_WORLD) && (pCollision->type & SITHCOLLISION_FACE) )
    {
        if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) == 0 && pCollision->pSurfaceCollided->face.normal.z <= sithVehicleControls_surfaceSearchMaxNormalZ )
        {
            curdot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &pSearch->dir);
            if ( curdot < sithVehicleControls_surfaceSearchMaxDot )
            {
                pSearch->pSurface = pCollision->pSurfaceCollided;
                pSearch->dot      = curdot;
                pSearch->distance = pCollision->distance;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
}

static int J3DAPI sithVehicleControls_FindRaftExitSurfacePos(SithThing* pThing, float moveDist, rdVector3* pOutPos, rdVector3* pOutPYR)
{
    static float sithVehicleControls_raftExitMinUpZ = 0.9986f;
    static float sithVehicleControls_raftExitMinFloorNormalZ = 0.9998f;
    static float sithVehicleControls_raftExitProbeRadius = 0.05f;
    static float sithVehicleControls_raftExitSurfaceOffset = 0.05f;
    int result;
    int bestIdx;
    int bFound;
    float searchDist;
    SithVehicleSurfaceSearch aSearch[7];
    float minDot;
    int i;
    SithSector* pSector;
    SithSector* pEndSector;
    rdVector3 startPos;
    float heightOffset;
    float downDist;
    rdVector3 moveNorm;
    rdVector3 endPos;
    rdVector3 downDir;
    SithCollision* pCollision;
    float dot;
    size_t vertNum;

    result = 8;
    if ( pThing->orient.uvec.z < sithVehicleControls_raftExitMinUpZ )
    {
        return 8;
    }

    searchDist = moveDist;

    rdVector_Copy3(&aSearch[2].dir, &pThing->orient.lvec);
    aSearch[2].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[2].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[2], searchDist);

    rdVector_Neg3(&aSearch[3].dir, &pThing->orient.rvec);
    rdVector_Add3Acc(&aSearch[3].dir, &pThing->orient.lvec);
    aSearch[3].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[3].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[3], searchDist);

    rdVector_Copy3(&aSearch[4].dir, &pThing->orient.rvec);
    rdVector_Add3Acc(&aSearch[4].dir, &pThing->orient.lvec);
    aSearch[4].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[4].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[4], searchDist);

    rdVector_Neg3(&aSearch[0].dir, &pThing->orient.rvec);
    aSearch[0].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[0].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[0], searchDist);

    rdVector_Copy3(&aSearch[1].dir, &pThing->orient.rvec);
    aSearch[1].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[1].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[1], searchDist);

    rdVector_Neg3(&aSearch[5].dir, &pThing->orient.rvec);
    rdVector_ScaleAdd3Acc(&aSearch[5].dir, &pThing->orient.lvec, -1.0f);
    aSearch[5].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[5].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[5], searchDist);

    rdVector_Copy3(&aSearch[6].dir, &pThing->orient.rvec);
    rdVector_ScaleAdd3Acc(&aSearch[6].dir, &pThing->orient.lvec, -1.0f);
    aSearch[6].dir.z = 0.0f;
    rdVector_Normalize3Acc(&aSearch[6].dir);
    sithVehicleControls_SearchWallSurface(pThing, &aSearch[6], searchDist);

    bFound = 0;
    while ( !bFound )
    {
        bestIdx = -1;
        minDot = 1.0f;
        for ( i = 0; i < 7; i++ )
        {
            if ( aSearch[i].pSurface && aSearch[i].dot < minDot )
            {
                bestIdx = i;
            }
        }

        if ( bestIdx == -1 )
        {
            break;
        }

        heightOffset = -sithPhysics_GetThingHeight(pThing) + sithVehicleControls_raftExitProbeRadius + 0.06f;
        rdVector_Copy3(&startPos, &pThing->pos);
        startPos.z += heightOffset;
        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.001f);
        if ( pSector )
        {
            rdVector_Neg3(&moveNorm, &aSearch[bestIdx].pSurface->face.normal);
            sithCollision_SearchForCollisions(pSector, pThing, &startPos, &moveNorm, searchDist, sithVehicleControls_raftExitProbeRadius, 2);
            pCollision = sithCollision_PopStack();
            sithCollision_DecreaseStackLevel();
            if ( !pCollision )
            {
                rdVector_Neg3(&downDir, &rdroid_zVector3);
                rdVector_ScaleAdd3(&endPos, &moveNorm, searchDist + 0.05f, &startPos);
                pEndSector = sithCollision_FindSectorInRadius(pSector, &startPos, &endPos, 0.001f);
                if ( pEndSector )
                {
                    downDist = 0.02f + sithVehicleControls_raftExitProbeRadius;
                    sithCollision_SearchForCollisions(pEndSector, pThing, &endPos, &downDir, downDist, 0.001f, 2);
                    while ( (pCollision = sithCollision_PopStack()) != NULL )
                    {
                        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
                            && (pCollision->type & SITHCOLLISION_FACE) != 0
                            && pCollision->pSurfaceCollided
                            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0
                            && pCollision->pSurfaceCollided->face.normal.z >= sithVehicleControls_raftExitMinFloorNormalZ )
                        {
                            bFound = 1;
                            break;
                        }
                    }

                    sithCollision_DecreaseStackLevel();
                }
            }
        }

        if ( !bFound )
        {
            aSearch[bestIdx].pSurface = NULL;
        }
    }

    if ( bFound )
    {
        dot = rdVector_Dot3(&aSearch[bestIdx].pSurface->face.normal, &pThing->orient.rvec);
        if ( dot <= 0.0f )
        {
            result = 1;
        }
        else
        {
            result = 0;
        }

        rdVector_Copy3(pOutPYR, &aSearch[bestIdx].pSurface->face.normal);
        memset(pOutPos, 0, sizeof(rdVector3));
        for ( vertNum = 0; vertNum < aSearch[bestIdx].pSurface->face.numVertices; vertNum++ )
        {
            rdVector_Add3Acc(pOutPos, &sithWorld_pCurrentWorld->aVertices[aSearch[bestIdx].pSurface->face.aVertices[vertNum]]);
        }

        rdVector_Scale3Acc(pOutPos, 1.0f / (float)vertNum);
        rdVector_ScaleAdd3Acc(pOutPos, pOutPYR, sithVehicleControls_raftExitSurfaceOffset);
    }

    return result;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithVehicleControls_countfit_1, 15) /* end count-fit */
void J3DAPI sithVehicleControls_ProcessRaftPlayerMove(SithThing* pThing, float secDeltaTime)
{
    float fwdRotThrust;
    float backRotThrust;
    float fwdThrust;
    float backThrust;
    float rvecDot;
    float dot;
    float dist;
    static float sithVehicleControls_raftWakeInterval = 0.05f;
    static float sithVehicleControls_raftWakeMinSpeed = 0.05f;
    static float sithVehicleControls_raftExitReachDist = 0.04f;
    static float sithVehicleControls_raftExitAlignDot = 0.98f;
    static float sithVehicleControls_raftHeight = 0.03f;
    static float sithVehicleControls_raftSurfDrag = 0.4f;
    static float sithVehicleControls_raftMaxThrust = 0.2f;
    static float sithVehicleControls_raftRotThrust = 60.0f;
    static float sithVehicleControls_raftTurnRotScale = 1.5f;
    static float sithVehicleControls_raftMaxRotVelocity = 135.0f;
    static float sithVehicleControls_raftExitSearchDist = 0.2f;
    static float sithVehicleControls_raftUnboardTimeout = 5.0f;
    static float sithVehicleControls_raftUnboardThrustScale = 0.15f;
    static float sithVehicleControls_raftUnboardRotScale = 120.0f;
    static float sithVehicleControls_raftUnboardMinRotVel = 5.0f;
    static float sithVehicleControls_raftStaticDrag = 0;
    static float sithVehicleControls_raftRotSpeed = 0;
    SithPhysicsInfo* pPhysics;
    SithActorInfo* pActorInfo;
    int unusedFlag;  // unreferenced; the debug build's frame keeps its slot
    int exitResult;
    float exitDist;

    pPhysics   = &pThing->moveInfo.physics;
    pActorInfo = &pThing->thingInfo.actorInfo;
    unusedFlag = 0;
    pPhysics->height                      = sithVehicleControls_raftHeight;
    pPhysics->surfDrag                    = sithVehicleControls_raftSurfDrag;
    pPhysics->staticDrag                  = sithVehicleControls_raftStaticDrag;
    pActorInfo->maxThrust = sithVehicleControls_raftMaxThrust;
    pPhysics->orientSpeed                 = sithVehicleControls_raftRotSpeed;
    pPhysics->maxRotationVelocity         = sithVehicleControls_raftMaxRotVelocity;

    fwdThrust     = pActorInfo->maxThrust;
    backThrust    = -(RD_MULTIPLY(fwdThrust, 0.5f));
    fwdRotThrust  = sithVehicleControls_raftRotThrust;
    backRotThrust = fwdRotThrust;

    if ( sithVehicleControls_curRaftState.velocity <= 0.0f
        && sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) >= sithVehicleControls_raftWakeMinSpeed
        && (int)pThing->userval == 3 )
    {
        sithFX_CreateRaftWake(pThing);
        sithVehicleControls_curRaftState.velocity = sithVehicleControls_raftWakeInterval;
    }
    else if ( sithVehicleControls_curRaftState.velocity > 0.0f )
    {
        sithVehicleControls_curRaftState.velocity = sithVehicleControls_curRaftState.velocity - RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), secDeltaTime);
    }
    else if ( sithVehicleControls_curRaftState.velocity < 0.0f )
    {
        sithVehicleControls_curRaftState.velocity = 0.0f;
    }

    if ( sithVehicleControls_curRaftState.bRowing )
    {
        sithFX_CreateRowWaterFx(pThing, sithVehicleControls_curRaftState.secRowStartTime);
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_FALLING:
        case SITHPLAYERMOVE_LAND:
        case SITHPLAYERMOVE_UNKNOWN_45:
        case SITHPLAYERMOVE_RAFT_BOARDING:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            break;

        default:
            if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !sithPlayerControls_bActionActivated )
            {
                sithPlayerControls_bActionActivated = 1;
                exitResult = sithVehicleControls_FindRaftExitSurfacePos(pThing, sithVehicleControls_raftExitSearchDist, &sithVehicleControls_curRaftState.exitSurfPos, &sithVehicleControls_curRaftState.exitSurfPYR);
                if ( exitResult != 8 )
                {
                    sithVehicleControls_curRaftState.moveSize = pThing->collide.movesize;
                    sithVehicleControls_curRaftState.secUnboardTime = 0.0f;
                    pThing->collide.movesize = RD_MULTIPLY(sithVehicleControls_curRaftState.moveSize, 0.5f);
                    sithPhysics_ResetThingMovement(pThing);
                    sithVehicleControls_SendMessageUser0(pThing);

                    switch ( pThing->moveStatus )
                    {
                        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
                        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
                        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
                        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
                        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
                            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
                            pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_45;
                            break;

                        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
                        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
                        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
                        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
                        case SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT:
                            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
                            pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_45;
                            break;

                        case SITHPLAYERMOVE_RAFT_STILL:
                            pThing->moveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
                            break;

                        default:
                            pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_45;
                            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_UNBOARD_START;
                            break;
                    }
                }
                else
                {
                    sithSoundClass_PlayPlayerVoiceModeRandom(pThing, SITHSOUNDCLASS_LRUNSNOW);
                }
            }
            break;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_RAFT_BOARDING:
            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_STILL;
            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            break;

        case SITHPLAYERMOVE_UNKNOWN_45:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            sithVehicleControls_curRaftState.secUnboardTime += secDeltaTime;
            if ( sithVehicleControls_curRaftState.secUnboardTime >= sithVehicleControls_raftUnboardTimeout )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STILL;
                pThing->collide.movesize = sithVehicleControls_curRaftState.moveSize;
                sithVehicleControls_SendMessageUser1(pThing);
                break;
            }
            else if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARD_START )
            {
                rvecDot = rdVector_Dot3(&pThing->orient.rvec, &sithVehicleControls_curRaftState.exitSurfPYR);
                exitDist = rdVector_Dist3(&sithVehicleControls_curRaftState.exitSurfPos, &pThing->pos);
                if ( exitDist <= sithVehicleControls_raftExitReachDist && SITHVEHICLECONTROLS_ABS(rvecDot) > sithVehicleControls_raftExitAlignDot )
                {
                    pThing->collide.movesize = sithVehicleControls_curRaftState.moveSize;
                    sithPhysics_ResetThingMovement(pThing);
                    if ( rvecDot < 0.0f )
                    {
                        sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_HOPRIGHT);
                        pThing->moveStatus = SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT;
                        rdVector_Neg3(&pThing->orient.rvec, &sithVehicleControls_curRaftState.exitSurfPYR);
                        rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);
                        rdVector_Normalize3Acc(&pThing->orient.lvec);
                    }
                    else
                    {
                        sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_HOPLEFT);
                        pThing->moveStatus = SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT;
                        rdVector_Copy3(&pThing->orient.rvec, &sithVehicleControls_curRaftState.exitSurfPYR);
                        rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);
                        rdVector_Normalize3Acc(&pThing->orient.lvec);
                    }

                    sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, 21, 0, 0);
                    sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, 30, 0, 0);
                }
            }
            break;

        case SITHPLAYERMOVE_RAFT_STILL:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT);
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT;
                sithVehicleControls_PlayPuppetMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT);
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_STILL;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
            sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_STILL;
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT;
            }
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_TURN_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) && !sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) && sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT;
            }
            else
            {
                sithVehicleControls_curRaftState.moveStatus = SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT;
            }
            break;

        case SITHPLAYERMOVE_FALLING:
            sithCog_ThingSendMessageEx(pThing, NULL, SITHCOG_MSG_CALLBACK, 0, 22, 0, 0);
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithVehicleControls_curRaftState.moveStatus = pThing->moveStatus;
            break;

        case SITHPLAYERMOVE_LAND:
            sithVehicleControls_curRaftState.moveStatus = pThing->moveStatus;
            break;

        default:
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithVehicleControls_curRaftState.moveStatus = pThing->moveStatus;
            break;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.y = 0.0f;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.y = -fwdRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.y = -(RD_MULTIPLY(fwdRotThrust, 0.5f));
                }
                pPhysics->thrust.y = fwdThrust;
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.y = 0.0f;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.y = fwdRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.y = RD_MULTIPLY(fwdRotThrust, 0.5f);
                }
                pPhysics->thrust.y = fwdThrust;
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.y = 0.0f;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.y = backRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.y = RD_MULTIPLY(backRotThrust, 0.5f);
                }
                pPhysics->thrust.y = backThrust;
            }
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.y = 0.0f;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.y = -backRotThrust;
                }
                else
                {
                    pPhysics->rotThrust.y = -(RD_MULTIPLY(backRotThrust, 0.5f));
                }
                pPhysics->thrust.y = backThrust;
            }
            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->rotThrust.y = RD_MULTIPLY(fwdRotThrust, sithVehicleControls_raftTurnRotScale);
                }
                else
                {
                    pPhysics->rotThrust.y = 0.0f;
                }

                pPhysics->thrust.y = 0.0f;
            }
            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            if ( sithVehicleControls_curRaftState.bRowing )
            {
                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->rotThrust.y = -(RD_MULTIPLY(fwdRotThrust, sithVehicleControls_raftTurnRotScale));
                }
                else
                {
                    pPhysics->rotThrust.y = 0.0f;
                }

                pPhysics->thrust.y = 0.0f;
            }
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
            pPhysics->rotThrust.y = 0.0f;
            pPhysics->thrust.y    = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
            pPhysics->rotThrust.y = 0.0f;
            pPhysics->thrust.y    = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
            pPhysics->rotThrust.y = 0.0f;
            pPhysics->thrust.y    = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
            pPhysics->rotThrust.y = 0.0f;
            pPhysics->thrust.y    = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
            pPhysics->rotThrust.y = 0.0f;
            pPhysics->thrust.y    = 0.0f;
            break;

        case SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT:
            pPhysics->rotThrust.y = 0.0f;
            pPhysics->thrust.y    = 0.0f;
            break;

        case SITHPLAYERMOVE_UNKNOWN_45:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            dist = rdVector_Dist3(&sithVehicleControls_curRaftState.exitSurfPos, &pThing->pos);
            if ( dist > sithVehicleControls_raftExitReachDist )
            {
                /* count-fit: placeholder declarations, not reconstructed source */ CF_EXTERNS(sithVehicleControls_countfit_2, 11) /* end count-fit */ rdVector3 dir;

                rdVector_Sub3(&dir, &sithVehicleControls_curRaftState.exitSurfPos, &pThing->pos);
                rdVector_Normalize3Acc(&dir);
                dot = rdVector_Dot3(&pThing->orient.lvec, &dir);
                pPhysics->thrust.y = RD_MULTIPLY(dot, sithVehicleControls_raftUnboardThrustScale);
                dot = rdVector_Dot3(&pThing->orient.rvec, &dir);
                pPhysics->thrust.x = RD_MULTIPLY(dot, sithVehicleControls_raftUnboardThrustScale);
            }
            else
            {
                memset(&pPhysics->thrust, 0, sizeof(rdVector3));
                memset(&pPhysics->velocity, 0, sizeof(rdVector3));
            }

            dot = rdVector_Dot3(&pThing->orient.lvec, &sithVehicleControls_curRaftState.exitSurfPYR);
            if ( rdVector_Dot3(&pThing->orient.rvec, &sithVehicleControls_curRaftState.exitSurfPYR) >= 0.0f )
            {
                pPhysics->angularVelocity.y = RD_MULTIPLY(dot, sithVehicleControls_raftUnboardRotScale);
            }
            else
            {
                pPhysics->angularVelocity.y = -(RD_MULTIPLY(dot, sithVehicleControls_raftUnboardRotScale));
            }

            if ( pPhysics->angularVelocity.y < sithVehicleControls_raftUnboardMinRotVel && -sithVehicleControls_raftUnboardMinRotVel < pPhysics->angularVelocity.y )
            {
                pPhysics->angularVelocity.y = RD_MULTIPLY((float)(pPhysics->angularVelocity.y < 0.0f ? -1 : 1), sithVehicleControls_raftUnboardMinRotVel);
            }
            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
            break;

        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            break;

        default:
            memset(&pPhysics->rotThrust, 0, sizeof(rdVector3));
            memset(&pPhysics->thrust, 0, sizeof(rdVector3));
            break;
    }

    switch ( sithVehicleControls_curRaftState.moveStatus )
    {
        case SITHPLAYERMOVE_RAFT_STILL:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_STAND;
            break;
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_STRAFELEFT;
            break;
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_STRAFERIGHT;
            break;
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_HANGSHIMLEFT;
            break;
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_HANGSHIMRIGHT;
            break;
        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_TURNLEFT;
            break;
        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_TURNRIGHT;
            break;
        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_CLIMBWALLLEFT;
            break;
        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_WHIPCLIMBLEFT;
            break;
        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_CLIMBWALLRIGHT;
            break;
        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_WHIPCLIMBRIGHT;
            break;
        case SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_WHIPCLIMBUP;
            break;
        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_WHIPCLIMBDOWN;
            break;
        case SITHPLAYERMOVE_UNKNOWN_45:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_STAND;
            break;
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_HOPRIGHT;
            break;
        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_HOPLEFT;
            break;
        default:
            sithVehicleControls_curRaftState.puppetSubmode = SITHPUPPETSUBMODE_STAND;
            break;
    }
}

void J3DAPI sithVehicleControls_FadeJeepMoveSounds(SithThing* pThing)
{
    static float sithVehicleControls_jeepMoveSndFadeTime = 0.1f;
    sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, 0.0f, sithVehicleControls_jeepMoveSndFadeTime);
    sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, 0.0f, sithVehicleControls_jeepMoveSndFadeTime);
}

int J3DAPI sithVehicleControls_CanMoveInDirection(SithThing* pThing, rdVector3* pDir)
{
    static float sithVehicleControls_canMoveProbeDist = 0.15f;
    static float sithVehicleControls_canMoveProbeRadius = 0.07f;
    int bCanMove;
    int searchMask;
    SithCollision* pImpact;
    SithSector* pCurSect;
    int bFoundFloor;
    rdVector3 endPos;
    rdVector3 bottomDirection;

    bCanMove = 1;
    searchMask = 0x200;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, pDir, sithVehicleControls_canMoveProbeDist, sithVehicleControls_canMoveProbeRadius, searchMask);
    while ( (pImpact = sithCollision_PopStack()) != NULL )
    {
        if ( (pImpact->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pThing == pImpact->pThingCollided )
            {
                continue;
            }

            bCanMove = 0;
        }
        else
        {
            if ( (pImpact->type & SITHCOLLISION_ADJOINTOUCH) != 0
                && (pImpact->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) == 0
                && (pImpact->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
            {
                continue;
            }

            if ( (pImpact->type & SITHCOLLISION_ADJOINCROSS) != 0 )
            {
                continue;
            }

            bCanMove = 0;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( !bCanMove )
    {
        return 0;
    }

    bottomDirection.x = 0.0f;
    bottomDirection.y = 0.0f;
    bottomDirection.z = -1.0f;
    rdVector_ScaleAdd3(&endPos, pDir, sithVehicleControls_canMoveProbeDist, &pThing->pos);

    pCurSect = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPos, 0.0f);
    if ( !pCurSect )
    {
        return 0;
    }

    bCanMove = 0;
    bFoundFloor = 0;
    searchMask = 0x200;
    sithCollision_SearchForCollisions(pCurSect, NULL, &endPos, &bottomDirection, sithPhysics_GetThingHeight(pThing) + 0.005f, 0.001f, searchMask);
    while ( (pImpact = sithCollision_PopStack()) != NULL )
    {
        if ( (pImpact->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pThing == pImpact->pThingCollided )
            {
                continue;
            }

            bCanMove = 0;
            break;
        }
        else if ( (pImpact->type & SITHCOLLISION_WORLD) != 0 && (pImpact->type & SITHCOLLISION_FACE) != 0 )
        {
            if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0
                && pImpact->pSurfaceCollided->pAdjoin
                && (pImpact->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0
                && (pImpact->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
            {
                bCanMove = 0;
                break;
            }

            if ( pImpact->pSurfaceCollided->face.normal.z >= 0.89f )
            {
                bCanMove = 1;
            }
            else
            {
                bCanMove = 0;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    return bCanMove;
}

void J3DAPI sithVehicleControls_ExitRaft(SithThing* pRaftPlayer)
{
    SithSector* pSector;
    rdVector3 pos;
    rdMatrix34 orient;
    SithThing* pPlayer;
    int bSectorChanged;
    size_t playerNum;
    float exitDist = 0.1f;
    float zOffset = 0.05f;

    sithVehicleControls_SendMessageUser1(pRaftPlayer);
    sithInventory_SetSwimmingInventory(pRaftPlayer, 1);
    sithInventory_SetInventoryDisabled(pRaftPlayer, 56, 1);

    if ( pRaftPlayer->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT )
    {
        rdVector_Copy3(&orient.uvec, &rdroid_zVector3);
        rdVector_Cross3(&orient.lvec, &pRaftPlayer->orient.lvec, &orient.uvec);
        rdVector_Normalize3Acc(&orient.lvec);
        rdVector_Cross3(&orient.rvec, &orient.lvec, &orient.uvec);
        rdVector_Normalize3Acc(&orient.rvec);
        rdVector_ScaleAdd3(&pos, &pRaftPlayer->orient.rvec, -exitDist, &pRaftPlayer->pos);
    }
    else
    {
        rdVector_Copy3(&orient.uvec, &rdroid_zVector3);
        rdVector_Cross3(&orient.lvec, &orient.uvec, &pRaftPlayer->orient.lvec);
        rdVector_Normalize3Acc(&orient.lvec);
        rdVector_Cross3(&orient.rvec, &orient.lvec, &orient.uvec);
        rdVector_Normalize3Acc(&orient.rvec);
        rdVector_ScaleAdd3(&pos, &pRaftPlayer->orient.rvec, exitDist, &pRaftPlayer->pos);
    }

    pPlayer = sithPlayer_aPlayers[0].pThing;
    zOffset += sithPhysics_GetThingHeight(pPlayer) - sithPhysics_GetThingHeight(pRaftPlayer);
    pos.z += zOffset;

    pSector = sithCollision_FindSectorInRadius(pRaftPlayer->pInSector, &pRaftPlayer->pos, &pos, 0.0f);
    if ( !pSector )
    {
        #line 3063
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "This is really bad, trying to exit vehicle to non-existent sector..\n", 0, 0, 0, 0);
    }

    if ( pSector == pRaftPlayer->pInSector )
    {
        bSectorChanged = 0;
    }
    else
    {
        bSectorChanged = 1;
    }

    playerNum = sithPlayer_playerNum;
    if ( bSectorChanged )
    {
        sithCog_SectorSendMessage(pRaftPlayer->pInSector, pRaftPlayer, SITHCOG_MSG_EXITED);
    }

    sithPlayer_HidePlayer(playerNum);

    *pPlayer->thingInfo.actorInfo.pPlayer = *pRaftPlayer->thingInfo.actorInfo.pPlayer;
    pPlayer->thingInfo.actorInfo.pPlayer->pThing = pPlayer;
    pPlayer->thingInfo.actorInfo.health = pRaftPlayer->thingInfo.actorInfo.health;

    if ( (pRaftPlayer->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
    {
        pPlayer->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
    }
    else
    {
        pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
    }

    if ( (pRaftPlayer->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
    {
        pPlayer->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
    }
    else
    {
        pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
    }

    sithThing_ExitSector(pPlayer);
    sithPlayer_SetLocalPlayer(0);
    sithPlayer_ShowPlayer(0, 0);
    sithThing_SetPositionAndOrient(pPlayer, &pos, &orient);
    sithThing_EnterSector(pPlayer, pSector, 1, !bSectorChanged);

    pPlayer->alpha = 1.0f;
    pPlayer->moveStatus = SITHPLAYERMOVE_STILL;
    sithPuppet_SetMoveMode(pPlayer, SITHPUPPET_MOVEMODE_NORMAL);
    sithPhysics_FindFloor(pPlayer, 1);
    sithCamera_SetCameraFocus(&sithCamera_aCameras[1], pPlayer, NULL);
    sithCog_ThingSendMessageEx(pRaftPlayer, NULL, SITHCOG_MSG_CALLBACK, 0, 16, 0, 0);
    sithSoundClass_StopMode(pRaftPlayer, 65);
}

void J3DAPI sithVehicleControls_UpdateRaftPaddleSoundFx(SithThing* pThing)
{
    static int sithVehicleControls_paddleSide = -1;
    static float sithVehicleControls_paddleFwdMinSpeed = 0.175f;
    static float sithVehicleControls_paddleBackMinSpeed = 0.1f;
    static float sithVehicleControls_unusedHalfSqrt2 = 0.707f;
    int bFwd;
    rdVector3 paddleVector;
    float velocity;
    float minRate;
    float forwardDotp;
    float curDot;
    rdVector3 vecValue;

    bFwd = 1;
    if ( pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT || pThing->moveStatus == SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT )
    {
        rdVector_Copy3(&paddleVector, &pThing->orient.lvec);
    }
    else
    {
        rdVector_Neg3(&paddleVector, &pThing->orient.lvec);
    }

    velocity = rdVector_Normalize3(&vecValue, &pThing->moveInfo.physics.velocity);
    forwardDotp = rdVector_Dot3(&pThing->orient.lvec, &vecValue);
    if ( forwardDotp >= 0.0f )
    {
        minRate = sithVehicleControls_paddleFwdMinSpeed;
    }
    else
    {
        minRate = sithVehicleControls_paddleBackMinSpeed;
    }

    if ( velocity > minRate )
    {
        curDot = rdVector_Dot3(&vecValue, &paddleVector);
        if ( curDot > minRate )
        {
            bFwd = 0;
        }
    }

    if ( sithVehicleControls_paddleSide > 0 )
    {
        if ( bFwd )
        {
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LWALKWATER);
        }
        else
        {
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LWALKPUDDLE);
        }
    }
    else if ( bFwd )
    {
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_RWALKWATER);
    }
    else
    {
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_RWALKPUDDLE);
    }

    sithVehicleControls_paddleSide *= -1;
}

void J3DAPI sithVehicleControls_SendMessageUser0(SithThing* pThing)
{
    sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER0);
    sithPlayerControls_bCutsceneMode = 0;
}

void J3DAPI sithVehicleControls_SendMessageUser1(SithThing* pThing)
{
    sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER1);
}
