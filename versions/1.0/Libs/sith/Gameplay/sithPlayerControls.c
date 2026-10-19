// rdVector_Dot3 is the inline header macro here; GetPushPullMoveNorm names the function, (rdVector_Dot3)(...), because retail calls it out of line there (0x004311E3, 0x0043124F -> 0x004CBC30).

#include "sithPlayerControls.h"

#include <rdroid/Engine/rdPuppet.h>
#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>
#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
rdVector3 sithPlayerControls_vecDebugCamDir = { { 0.0f }, { -1.0f }, { 0.0f } };
float sithPlayerControls_cameraDistance     = 0.2f;

static float sithPlayerControls_unknown_528CE8   = -0.06971f;
static float sithPlayerControls_aimInterpDistance = 0.3f;
static float sithPlayerControls_flt_528CF0        = 20.5f;
static float sithPlayerControls_flt_528CF4        = 22.5f;
static float sithPlayerControls_flt_528CF8        = 87.0f;
static float sithPlayerControls_flt_528CFC        = 89.0f;
static float sithPlayerControls_flt_528D00        = 50.0f;
static float sithPlayerControls_flt_528D04        = 3.0f;
static float sithPlayerControls_flt_528D08        = 5.0f;
static float sithPlayerControls_flt_528D0C        = 14.0f;
static float sithPlayerControls_flt_528D10        = 16.0f;
static float sithPlayerControls_flt_528D14        = 5.0f;
static float sithPlayerControls_flt_528D18        = 0.08f;
static float sithPlayerControls_flt_528D1C        = 0.12f;
static float sithPlayerControls_pistolAimYawOffset = 3.0f;
static float sithPlayerControls_rifleAimYawOffset  = 1.0f;

// Zero-initialised: retail keeps this TU's storage as one private .bss block in declaration order
// (0x0055EDD0-0x0055EE18) right after sithAI's, not as COMMON requests in the image's common tail.
static int sithPlayerControls_unusedState0 = 0; // 0x0055EDD0: no reference anywhere in the image
static int sithPlayerControls_climbPupTrackNum = 0; // only ProcessClimbMove reads/writes it

int sithPlayerControls_bActionActivated = 0;
int sithPlayerControls_bCutsceneMode = 0;
SithThing* sithPlayerControls_pCollidedThing = NULL;
unsigned int sithPlayerControls_curJumpDirection = 0;
SithThing* sithPlayerControls_pCurActivatedThing = NULL;
int sithPlayerControls_bJumpActivated = 0;
int sithPlayerControls_bLookMode = 0;
int sithPlayerControls_bHealthActivated = 0;
SithThing* sithPlayerControls_pVehicleBoardedThing = NULL;
SithThing* sithPlayerControls_pTargetThing = NULL;
int sithPlayerControls_dword_55EE00 = 0;
int sithPlayerControls_curMoveStatus = 0;
int sithPlayerControls_bTurnRightActivated = 0;
int sithPlayerControls_bTurnLeftActivated = 0;
float sithPlayerControls_secSwimmingTime = 0.0f;
float sithPlayerControls_secCommentWait = 0.0f;

// Tentative definitions (COMMON requests). Retail's common tail places these four between sithAI's
// and sithCamera's commons, i.e. at this object's pull position (LINK: first encounter as a common,
// reversed per batch), not at sithControl's or sithPlayerActions's, so
// this object emitted them first.
SithSurface* sithPlayerActions_pChalkWriteSurf;
rdVector3 sithPlayerActions_posChalkMarkWrite;
SithSector* sithPlayerActions_pChalkWriteSector;
int sithControl_controlOptions;

void J3DAPI sithPlayerControls_PuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;  // unreferenced; the debug build's frame keeps its slot
    SithActorInfo* pCurActorInfo = &pThing->thingInfo.actorInfo;  // unreferenced; the debug build's frame keeps its slot

    if ( !pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        return;
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_PUSHPULL_READY && markerType == RDKEYMARKER_CRAWL )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_WALK2STAND && markerType == 0 )
    {
        pThing->moveStatus                            = SITHPLAYERMOVE_STILL;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_STAND2WALK && markerType == 0 )
    {
        pThing->moveStatus                            = SITHPLAYERMOVE_WALKING;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_STAND2RUN && markerType == 0 )
    {
        pThing->moveStatus                            = SITHPLAYERMOVE_RUNNING;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_STILL || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_IDLE )
    {
        sithPlayerControls_ExitVehicle(pThing);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_LAND )
    {
        if ( markerType == 0 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithPuppet_FreeTrackByIndex(pThing, track);
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
        }
        else
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_ACTIVATING )
    {
        if ( markerType == RDKEYMARKER_ACTIVATERIGHTARM )
        {
            if ( sithPlayerControls_pCurActivatedThing )
            {
                sithCog_ThingSendMessage(sithPlayerControls_pCurActivatedThing, pThing, SITHCOG_MSG_TOUCHED);
            }
            else
            {
                sithFX_CreateChalkMark(&sithPlayerActions_posChalkMarkWrite, sithPlayerActions_pChalkWriteSurf, sithPlayerActions_pChalkWriteSector);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CALM);
            }

            return;
        }
        else if ( markerType == RDKEYMARKER_ACTIVATERIGHTARMREST )
        {
            sithCamera_RestoreExtCamera();
            return;
        }
        else if ( markerType == 0 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithPuppet_RemoveTrackByIndex(pThing, track);
            pThing->moveStatus = sithPlayerControls_curMoveStatus;
            if ( !sithPlayerControls_pCurActivatedThing )
            {
                sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_USER7);
            }

            sithPlayerControls_pCurActivatedThing = NULL;
            pThing->thingInfo.actorInfo.flags &= ~SITH_AF_CONTROLSDISABLED;
            return;
        }
        else
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_MOUNTING_WALL )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->moveStatus                            = SITHPLAYERMOVE_CLIMBIDLE;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLIDLE, NULL);
        sithPuppet_FreeTrackByIndex(pThing, track);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_CLIMBIDLE )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_PULLINGUP_1M )
    {
        if ( markerType == 0 )
        {
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
            if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_NORMAL )
            {
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
            }

            return;
        }
        else
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_JUMP_READY )
    {
        if ( markerType == 0 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithPuppet_DefaultCallback(pThing, track, markerType);
            if ( sithPlayerControls_curJumpDirection == 99 )
            {
                sithPlayerActions_ClimbOn2m(pThing);
            }
            else if ( sithPlayerControls_curJumpDirection > 0 )
            {
                sithPlayerActions_Jump(pThing, 0.77f, sithPlayerControls_curJumpDirection);
            }
            else if ( sithPlayerControls_curJumpDirection == 0 )
            {
                sithPlayerActions_Jump(pThing, 0.6f, sithPlayerControls_curJumpDirection);
            }
            else if ( sithPlayerControls_curJumpDirection == -1 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_JUMPBACK;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPBACK, NULL);
                sithPlayerActions_HopBack(pThing);
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                return;
            }

            sithPlayerControls_curJumpDirection = 0;
        }
        else
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
    {
        if ( markerType == 0 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
                }

                return;
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_NORMAL )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                }

                return;
            }
        }
        else
        {
            sithPuppet_DefaultCallback(pThing, track, markerType);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_WHIPSWINGING )
    {
        if ( markerType == RDKEYMARKER_ATTACK )
        {
            tSoundHandle hSnd = NULL;
            hSnd = Sound_GetSoundHandle(0x8003);
            if ( hSnd )
            {
                sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, 0);
            }
        }
        else if ( markerType == RDKEYMARKER_SWING )
        {
            tSoundHandle hSnd = NULL;
            hSnd = Sound_GetSoundHandle(0x8064);
            if ( hSnd )
            {
                sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, 0);
            }
        }
        else if ( markerType == RDKEYMARKER_ACTIVATERIGHTARMREST )
        {
            tSoundHandle hSnd = NULL;
            hSnd = Sound_GetSoundHandle(0x8018);
            if ( hSnd )
            {
                sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, 0);
            }

            sithCamera_RestoreExtCamera();
        }
        else if ( markerType == 0 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            sithWhip_DeactivateWhip(pThing);
        }
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_WHIP_CLIMB_START )
    {
        sithWhip_SetActorWhipClimbIdle(pThing);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_WHIPCLIMBIDLE )
    {
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_HANGING )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_STILL )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    }
}

int J3DAPI sithPlayerControls_Process(SithThing* pPlayerThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pPlayerThing->moveInfo.physics;
    SithActorInfo* pActInfo = &pPlayerThing->thingInfo.actorInfo;
    rdVector3 delta;
    int weaponIdx;

    if ( sithPlayerControls_bCutsceneMode )
    {
        return 0;
    }

    if ( sithPlayerControls_dword_55EE00 > 0 )
    {
        sithPlayerControls_dword_55EE00 -= (int)RD_MULTIPLY(secDeltaTime, 1000.0f);
        if ( sithPlayerControls_dword_55EE00 < 0 )
        {
            sithPlayerControls_dword_55EE00 = 0;
        }
    }

    if ( pPlayerThing->moveType != SITH_MT_PHYSICS )
    {
        return 0;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 && sithPlayerControls_ProcessEditorDebugControls(pPlayerThing, secDeltaTime) )
    {
        return 1;
    }

    if ( (pPlayerThing->flags & SITH_TF_DYING) != 0 )
    {
        sithPlayerControls_ProcessDeadPlayer(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( sithPlayerControls_secCommentWait != 0.0f )
    {
        sithPlayerControls_secCommentWait = sithPlayerControls_secCommentWait - secDeltaTime;
        if ( sithPlayerControls_secCommentWait <= 0.0f )
        {
            sithPlayerControls_secCommentWait = 0.0f;
        }
    }

    if ( sithTime_bPaused == 1 )
    {
        return 1;
    }

    if ( pPlayerThing->moveStatus != SITHPLAYERMOVE_SWIMIDLE )
    {
        sithPlayerControls_secSwimmingTime = 0.0f;
    }
    else
    {
        sithPlayerControls_secSwimmingTime += secDeltaTime;
    }

    if ( sithPuppet_bPlayerLeapForward && pPlayerThing->moveStatus != SITHPLAYERMOVE_RUNNING )
    {
        sithPuppet_bPlayerLeapForward = 0;
    }

    if ( sithPlayerControls_ProcessPlayerDebugControls(pPlayerThing, secDeltaTime) )
    {
        return 1;
    }

    if ( sithPlayerControls_bActionActivated && !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
    {
        sithPlayerControls_bActionActivated = 0;
    }

    if ( sithPlayerControls_bTurnLeftActivated && !sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        sithPlayerControls_bTurnLeftActivated = 0;
    }

    if ( sithPlayerControls_bTurnRightActivated && !sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        sithPlayerControls_bTurnRightActivated = 0;
    }

    if ( sithPlayerControls_bJumpActivated && !sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
    {
        sithPlayerControls_bJumpActivated = 0;
    }

    if ( sithPlayerControls_bHealthActivated && !sithControl_GetKey(SITHCONTROL_HEALTH, NULL) )
    {
        sithPlayerControls_bHealthActivated = 0;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        if ( sithControl_GetKey(SITHCONTROL_LOOK, NULL) )
        {
            if ( !sithPlayerControls_bCutsceneMode
                && !sithPlayerControls_bLookMode
                && pPlayerThing->moveInfo.physics.velocity.x == 0.0f
                && pPlayerThing->moveInfo.physics.velocity.y == 0.0f
                && pPlayerThing->moveInfo.physics.velocity.z == 0.0f
                && pPlayerThing->moveInfo.physics.angularVelocity.x == 0.0f
                && pPlayerThing->moveInfo.physics.angularVelocity.y == 0.0f
                && pPlayerThing->moveInfo.physics.angularVelocity.z == 0.0f )
            {
                weaponIdx = sithInventory_GetCurrentWeapon(pPlayerThing);
                if ( weaponIdx == SITHWEAPON_WHIP && sithWeapon_IsAiming(pPlayerThing) )
                {
                }
                else
                {
                    pActInfo->flags |= SITH_AF_VIEWCENTRING;
                    memset(&pActInfo->headPYR, 0, sizeof(rdVector3));
                    sithActor_SetHeadPYR(pPlayerThing, &pActInfo->headPYR);

                    sithPlayerControls_bLookMode  = 1;
                    sithCamera_bExtCameraLookMode = 1;

                    if ( pPlayerThing->moveStatus == SITHPLAYERMOVE_CRAWL_STILL )
                    {
                        rdVector_Set3(&delta, 0.0f, 0.0f, 0.025f);
                    }
                    else
                    {
                        rdVector_Set3(&delta, 0.0f, 0.0f, 0.06f);
                    }

                    sithCamera_SetExtCameraOffset(&delta);

                    if ( pPlayerThing->moveStatus == SITHPLAYERMOVE_CRAWL_STILL )
                    {
                        rdVector_Set3(&delta, 0.0f, 0.025f, 0.025f);
                    }
                    else
                    {
                        rdVector_Set3(&delta, 0.0f, 0.04f, 0.06f);
                    }

                    sithCamera_SetExtCameraLookOffset(&delta);
                }
            }
        }
        else if ( sithPlayerControls_bLookMode == 1 )
        {
            pActInfo->flags |= SITH_AF_VIEWCENTRING;
            memset(&pActInfo->headPYR, 0, sizeof(rdVector3));
            sithActor_SetHeadPYR(pPlayerThing, &pActInfo->headPYR);
            sithPlayerControls_bLookMode = 0;
            sithCamera_RestoreExtCamera();
            sithCamera_bExtCameraLookMode = 0;
        }
    }

    sithPlayerControls_ProcessWeaponAim(pPlayerThing, secDeltaTime);
    if ( pPlayerThing->type == SITH_THING_PLAYER && (pPlayerThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
    {
        return 0;
    }

    if ( sithPlayerControls_bLookMode == 1 && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        sithPlayerControls_ProcessLookControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_FLY) != 0 && pPlayerThing->moveStatus != SITHPLAYERMOVE_JEWELFLYING )
    {
        sithPlayerControls_ProcessFlyAndUnderwaterMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessGeneralControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_JEEP) != 0 )
    {
        sithVehicleControls_ProcessJeepPlayerMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessGeneralControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_RAFT) != 0 )
    {
        sithVehicleControls_ProcessRaftPlayerMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessGeneralControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & SITH_PF_MINECAR) != 0 )
    {
        sithVehicleControls_ProcessMineCarPlayerMove(pPlayerThing, secDeltaTime);
        sithPlayerControls_ProcessGeneralControls(pPlayerThing, secDeltaTime);
        return 0;
    }

    if ( (pPhysics->flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP)) == 0 && !pPlayerThing->thingInfo.actorInfo.bForceMovePlay )
    {
        switch ( pPlayerThing->moveStatus )
        {
            case SITHPLAYERMOVE_CRAWL_STILL:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
            case SITHPLAYERMOVE_CLIMBIDLE:
            case SITHPLAYERMOVE_SWIMIDLE:
            case SITHPLAYERMOVE_MOUNTING_WALL:
            case SITHPLAYERMOVE_CLIMBING_UP:
            case SITHPLAYERMOVE_CLIMBING_DOWN:
            case SITHPLAYERMOVE_CLIMBING_LEFT:
            case SITHPLAYERMOVE_CLIMBING_RIGHT:
            case SITHPLAYERMOVE_STAND_TO_CRAWL:
            case SITHPLAYERMOVE_CRAWL_TO_STAND:
                break;

            default:
                sithPhysics_SetThingLook(pPlayerThing, &rdroid_zVector3, 0.0f);
                break;
        }
    }

    switch ( pPlayerThing->moveStatus )
    {
        case SITHPLAYERMOVE_STILL:
        case SITHPLAYERMOVE_WALKING:
        case SITHPLAYERMOVE_RUNNING:
        case SITHPLAYERMOVE_CRAWL_STILL:
        case SITHPLAYERMOVE_JUMP_READY:
        case SITHPLAYERMOVE_PUSHPULL_READY:
        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
        case SITHPLAYERMOVE_SLIDEDOWNBACK:
            if ( (pPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pPlayerThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                pPlayerThing->moveStatus           = SITHPLAYERMOVE_SWIMIDLE;
                sithPlayerControls_secSwimmingTime = 0.0f;
                sithPlayerControls_ProcessSwimMove(pPlayerThing, secDeltaTime);
            }
            else
            {
                sithPlayerControls_ProcessGeneralMove(pPlayerThing, secDeltaTime);
            }
            break;

        case SITHPLAYERMOVE_CLIMBIDLE:
            sithPlayerControls_ProcessClimbMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_JUMPFWD:
        case SITHPLAYERMOVE_JUMPUP:
        case SITHPLAYERMOVE_FALLING:
        case SITHPLAYERMOVE_LEAPFWD:
            if ( (pPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pPlayerThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                pPlayerThing->moveStatus           = SITHPLAYERMOVE_SWIMIDLE;
                sithPlayerControls_secSwimmingTime = 0.0f;
                sithPlayerControls_ProcessSwimMove(pPlayerThing, secDeltaTime);
            }
            else
            {
                sithPlayerControls_ProcessJumpForwardMove(pPlayerThing, secDeltaTime);
            }
            break;

        case SITHPLAYERMOVE_SWIMIDLE:
            sithPlayerControls_ProcessSwimMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_HANGING:
            sithPlayerControls_ProcessHangMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_MINECAR_IDLE:
            sithVehicleControls_ProcessMineCarPlayerMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_JEEP_STILL:
            sithVehicleControls_ProcessJeepPlayerMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_STILL:
            sithVehicleControls_ProcessRaftPlayerMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_WHIPCLIMBIDLE:
        case SITHPLAYERMOVE_WHIP_CLIMB_START:
        case SITHPLAYERMOVE_WHIP_UNK2:
            sithWhip_ProcessWhipClimbMove(pPlayerThing, secDeltaTime);
            break;

        case SITHPLAYERMOVE_JEWELFLYING:
        case SITHPLAYERMOVE_JEWELFLYING_UNKN1:
        case SITHPLAYERMOVE_JEWELFLYING_UNKN2:
            sithPlayerControls_ProcessJewelFlyMove(pPlayerThing, secDeltaTime);
            break;
    }

    sithPlayerControls_ProcessGeneralControls(pPlayerThing, secDeltaTime);
    return 0;
}

int J3DAPI sithPlayerControls_ProcessPlayerDebugControls(SithThing* pThing, float secDeltaTime)
{
    int state;
    int keyNum;

    if ( !sithControl_GetKey(SITHCONTROL_DEBUG, 0) )
    {
        return 0;
    }

    sithControl_GetKey(SITHCONTROL_ACT2, &state);
    if ( state )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize = 0.04f;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->thingInfo.actorInfo.bForceMovePlay    = 0;
        sithPuppet_RemoveAllTracks(pThing);
        pThing->renderData.bSkipBuildingJoints = 0;
        rdVector_Copy3(&pThing->orient.dvec, &pThing->pos);
        rdPuppet_BuildJointMatrices(&pThing->renderData, &pThing->orient);
        memset(&pThing->orient.dvec, 0, sizeof(rdVector3));
        return 1;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
    }

    for ( keyNum = SITHCONTROL_WSELECT0; keyNum <= SITHCONTROL_WSELECT9; ++keyNum )
    {
        sithControl_GetKey(keyNum, &state);
        if ( state )
        {
            sithPlayerActions_MoveToPlayerPosition(pThing, keyNum - SITHCONTROL_WSELECT0);
            return 1;
        }
    }
}

void J3DAPI sithPlayerControls_ProcessLookControls(SithThing* pThing, float secDeltaTime)
{
    float moveScale = 1.0f;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pInfo = &pThing->thingInfo.actorInfo;
    float mouseFactor = 0.05f;  // unreferenced; the debug build's frame keeps its slot
    int bApplyOffset;
    float diff;
    float yawDelta;
    float pitchDelta;
    rdVector3 headPYR;

    bApplyOffset = 0;
    diff         = 0.0f;
    yawDelta     = 0.0f;
    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            diff         = -0.001f;
            bApplyOffset = 1;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        diff         = 0.001f;
        bApplyOffset = 1;
    }

    if ( diff != 0.0f )
    {
        yawDelta = D3DMultiply(RD_MULTIPLY(diff, 90.0f), secDeltaTime);
    }

    diff       = 0.0f;
    pitchDelta = 0.0f;
    rdVector_Copy3(&headPYR, &pInfo->headPYR);
    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        diff = 0.001f;
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        diff = -0.001f;
    }

    if ( diff != 0.0f )
    {
        bApplyOffset = 1;
        pitchDelta   = D3DMultiply(RD_MULTIPLY(diff, 90.0f), secDeltaTime);
    }

    if ( bApplyOffset )
    {
        float minOffset = -0.07f;
        float maxOffset = 0.15f;
        rdVector3 disp;

        rdVector_Copy3(&disp, &sithCamera_pCurCamera->offset);

        disp.z += pitchDelta;
        if ( disp.z < minOffset )
        {
            disp.z = minOffset;
        }
        else if ( disp.z > maxOffset )
        {
            disp.z = maxOffset;
        }

        if ( yawDelta != 0.0f )
        {
            disp.x += yawDelta;
            if ( disp.x < minOffset )
            {
                disp.x = minOffset;
            }
            else if ( disp.x > maxOffset )
            {
                disp.x = maxOffset;
            }
        }

        sithCamera_SetExtCameraOffset(&disp);
    }
    else if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        float mouseTurn;
        mouseTurn = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
        mouseTurn = D3DMultiply(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pInfo->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
        pPhysics->angularVelocity.y += mouseTurn;
    }
}

void J3DAPI sithPlayerControls_ProcessGeneralMove(SithThing* pThing, float secDeltaTime)
{
    int bMoved;
    int bRun;
    int bClimbing;
    int bCanCrawl;
    int bClear;
    SithActorInfo* pActorInfo;
    int bHasMoveNorm;
    float moveScale;
    rdVector3 moveNorm;
    SithCollision* pCollision;
    SithThing* pCollided;
    SithPhysicsInfo* pPhysics;
    int searchFlags;
    float direction;

    moveScale  = 1.0f;
    pPhysics   = &pThing->moveInfo.physics;
    pActorInfo = &pThing->thingInfo.actorInfo;
    bRun       = 0;
    bMoved     = 0;

    sithPlayerActions_pCurCollidedModelFace = NULL;
    sithPlayerActions_pCurCollidedModel     = NULL;
    sithPlayerActions_pCurCollidedSurface   = NULL;

    if ( pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_JUMP_READY )
        {
            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
            {
                sithPlayerControls_curJumpDirection = 1;
                return;
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                sithPlayerControls_curJumpDirection = -1;
                return;
            }
            else
            {
                sithPlayerControls_curJumpDirection = 0;
                return;
            }
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_PUSHPULL_READY )
        {
        }
        else
        {
            return;
        }
    }

    moveScale = 1.0f;
    if ( (sithControl_controlOptions & 2) != 0 || sithControl_GetKey(SITHCONTROL_RUNFWD, NULL) || sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        moveScale *= 2.5f;
        bRun = 1;
    }

    if ( (sithControl_controlOptions & 2) != 0 && sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        moveScale = 1.0f;
        bRun      = 0;
    }

    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & (SITH_SURFACE_WEB | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER)) != 0 )
    {
        moveScale *= 0.8f;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_STILL:
            if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
            {
                moveScale = 0.1f;
            }
            else
            {
                if ( pThing->moveInfo.physics.height != 0.09f )
                {
                    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
                    pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
                    pThing->moveInfo.physics.height = 0.09f;
                    sithPhysics_FindFloor(pThing, 1);
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                }

                if ( sithControl_GetKey(SITHCONTROL_CAMERAZOOMOUT, NULL) )
                {
                    sithPlayerActions_Jump(pThing, 1.0f, 99);
                    return;
                }

                if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) && !sithPlayerControls_bJumpActivated )
                {
                    if ( sithPlayerActions_CanClimbOn2m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon(pThing) )
                    {
                        sithPlayerActions_ClimbOn2m(pThing);
                        return;
                    }

                    if ( sithPlayerActions_CanClimbOn1m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon(pThing) )
                    {
                        sithPlayerActions_ClimbOn1m(pThing);
                        return;
                    }

                    if ( !sithInventory_GetCurrentWeapon(pThing) && !sithWeapon_IsMountingWeapon(pThing) )
                    {
                        bClimbing   = 0;
                        searchFlags = 0xA00;
                        sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pThing->orient.lvec, pThing->collide.movesize * 2.0f, pThing->collide.movesize, searchFlags);
                        while ( (pCollision = sithCollision_PopStack()) != NULL )
                        {
                            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 || (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0 )
                            {
                                if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
                                {
                                    rdVector3 dir;
                                    rdVector3 lvec;
                                    float dot;
                                    rdVector_Neg3(&dir, &pCollision->pSurfaceCollided->face.normal);
                                    dir.z = 0.0f;
                                    rdVector_Normalize3Acc(&dir);

                                    rdVector_Copy3(&lvec, &pThing->orient.lvec);
                                    (&lvec.x)[2] = 0.0f; /* spelling-fit */
                                    rdVector_Normalize3Acc(&lvec);

                                    dot = rdVector_Dot3(&lvec, &dir);
                                    if ( dot > 0.8f )
                                    {
                                        pThing->moveStatus = SITHPLAYERMOVE_MOUNTING_WALL;
                                        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                                        sithPhysics_ResetThingMovement(pThing);
                                        sithPlayerActions_MoveToClimbSector(pThing, pCollision->pSurfaceCollided);
                                        sithThing_AttachThingToClimbSurface(pThing, pCollision->pSurfaceCollided, 1);
                                        sithPuppet_RemoveAllTracks(pThing);
                                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_MOUNTWALL, sithPlayerControls_PuppetCallback);
                                        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
                                        pThing->moveInfo.physics.flags &= ~SITH_PF_FLOORSTICK;
                                        bClimbing = 1;
                                        sithPlayerControls_bJumpActivated = 1;
                                        sithInventory_SetSwimmingInventory(pThing, 0);
                                        break;
                                    }
                                    else
                                    {
                                        continue;
                                    }
                                }
                            }
                            else
                            {
                                continue;
                            }
                        }

                        sithCollision_DecreaseStackLevel();
                        if ( bClimbing )
                        {
                            return;
                        }
                    }

                    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                    {
                        if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                        {
                            float dot;
                            dot = rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_zVector3);
                            if ( dot > 0.7f && dot < 0.75f )
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                                pThing->thingInfo.actorInfo.bForceMovePlay = 0;
                                return;
                            }
                        }

                        sithPlayerControls_curJumpDirection = 0;
                        sithPlayerActions_JumpStart(pThing);
                        return;
                    }
                }

                if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay && !sithPlayerActions_HasActiveWeapon(pThing) )
                {
                    bCanCrawl = 0;
                    if ( pThing->attach.flags )
                    {
                        if ( (pThing->attach.flags & SITH_ATTACH_THING) != 0 || (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
                        {
                            if ( !pThing->attach.attachedToStructure.pThingAttached )
                            {
                                bCanCrawl = 0;
                            }
                            else
                            {
                                SithThing* pAttachedThing;
                                pAttachedThing = pThing->attach.attachedToStructure.pThingAttached;
                                switch ( pAttachedThing->moveType )
                                {
                                    case SITH_MT_NONE:
                                        bCanCrawl = 1;
                                        break;

                                    case SITH_MT_PHYSICS:
                                        if ( pAttachedThing->moveInfo.physics.velocity.x != 0.0f
                                            || pAttachedThing->moveInfo.physics.velocity.y != 0.0f
                                            || pAttachedThing->moveInfo.physics.velocity.z != 0.0f )
                                        {
                                            bCanCrawl = 0;
                                        }
                                        else
                                        {
                                            bCanCrawl = 1;
                                        }

                                        break;

                                    case SITH_MT_PATH:
                                        if ( (pAttachedThing->moveInfo.pathMovement.mode & 1) != 0 )
                                        {
                                            bCanCrawl = 0;
                                        }
                                        else
                                        {
                                            bCanCrawl = 1;
                                        }

                                        break;
                                }

                                if ( bCanCrawl && pThing->attach.pFace )
                                {
                                    rdVector3 normal;
                                    float zdot;
                                    rdMatrix_TransformVector34(&normal, &pThing->attach.pFace->normal, &pAttachedThing->orient);
                                    zdot = (float)fabs(rdVector_Dot3(&normal, &rdroid_zVector3));
                                    if ( zdot < 0.98f )
                                    {
                                        bCanCrawl = 0;
                                    }
                                    else
                                    {
                                        bCanCrawl = 1;
                                    }
                                }
                            }
                        }
                        else if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && pThing->attach.attachedToStructure.pSurfaceAttached )
                        {
                            if ( (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_SHALLOWWATER) != 0
                                || (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_WATER) != 0 )
                            {
                                bCanCrawl = 0;
                            }
                            else
                            {
                                float zdot;
                                zdot = (float)fabs(rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_zVector3));
                                if ( zdot < 0.98f )
                                {
                                    bCanCrawl = 0;
                                }
                                else
                                {
                                    bCanCrawl = 1;
                                }
                            }
                        }
                    }

                    if ( bCanCrawl == 1 )
                    {
                        sithPlayerActions_Stand2Crawl(pThing);
                        return;
                    }

                    if ( sithPlayerControls_secCommentWait == 0.0f )
                    {
                        sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_SPLATTERED);
                        sithPlayerControls_secCommentWait = 3.0f;
                    }
                }

                if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
                {
                    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                    {
                        if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) )
                        {
                            if ( pThing->attach.flags )
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLFWD;
                                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLFWD, NULL);
                                return;
                            }
                        }
                        else if ( !pThing->thingInfo.actorInfo.bControlsDisabled )
                        {
                            rdVector3 pos;
                            int moveFlags;
                            rdVector_Copy3(&pos, &pThing->pos);
                            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.lvec, pThing->collide.movesize);
                            if ( bRun || sithPlayerActions_CanMoveToPos(pThing, &pos, &moveFlags) )
                            {
                                rdKeyframe* pKeyframe = NULL;
                                pKeyframe = sithPuppet_GetKeyframe("in_stand_bd_walk.key");
                                if ( pKeyframe )
                                {
                                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                                    if ( bRun )
                                    {
                                        pThing->moveStatus = SITHPLAYERMOVE_STAND2RUN;
                                    }
                                    else
                                    {
                                        pThing->moveStatus = SITHPLAYERMOVE_STAND2WALK;
                                    }

                                    sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, 1, 2, RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_NOLOOP, sithPlayerControls_PuppetCallback);
                                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_STAND);
                                    if ( bRun )
                                    {
                                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);
                                    }
                                    else
                                    {
                                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WALK, NULL);
                                    }

                                    return;
                                }
                            }
                            else
                            {
                                bMoved = 0;
                                return;
                            }
                        }
                    }
                }
                else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
                {
                    int climbType;
                    direction = -1.0f;
                    climbType = sithPlayerActions_SearchClimbSurface(pThing);
                    switch ( climbType )
                    {
                        case 0:
                            break;

                        case 1:
                            sithPlayerActions_ClimbToClimb(pThing, 0);
                            return;

                        case 2:
                            sithPlayerActions_ClimbToHang(pThing);
                            return;

                        case 3:
                            sithPlayerActions_ClimbToClimb(pThing, 1);
                            return;
                    }

                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) )
                    {
                        if ( pThing->attach.flags )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLBACK;
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLBACK, NULL);
                            return;
                        }
                    }
                    else
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                        pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, direction), 0.5f);
                        bMoved = 1;
                    }
                }
                else
                {
                    pPhysics->thrust.y = 0.0f;
                }

                if ( sithControl_GetKey(SITHCONTROL_STPLEFT, NULL) )
                {
                    if ( !pThing->thingInfo.actorInfo.bForceMovePlay && sithPlayerControls_CanStrafeMove(pThing, 0) )
                    {
                        sithPlayerActions_StrafeLeft(pThing);
                        sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
                    }

                    return;
                }

                if ( sithControl_GetKey(SITHCONTROL_STPRIGHT, NULL) )
                {
                    if ( !pThing->thingInfo.actorInfo.bForceMovePlay && sithPlayerControls_CanStrafeMove(pThing, 1) )
                    {
                        sithPlayerActions_StrafeRight(pThing);
                        sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
                    }

                    return;
                }

                pPhysics->thrust.x          = 0.0f;
                pPhysics->angularVelocity.y = 0.0f;
            }

            if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0 )
                    {
                        if ( pThing->attach.flags )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_JUMPRIGHT;
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPRIGHT, NULL);
                            return;
                        }
                    }
                    else
                    {
                        pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, -1.0f);
                        pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, -1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
                        if ( sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
                        {
                            pPhysics->angularVelocity.y *= 2.5f;
                        }
                        else
                        {
                            pPhysics->angularVelocity.y /= 1.4f;
                        }

                        bMoved = 1;
                    }
                }
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) == 0 )
                    {
                        if ( pThing->attach.flags )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_JUMPLEFT;
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPLEFT, NULL);
                            return;
                        }
                    }
                    else
                    {
                        pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, 1.0f);
                        pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, 1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
                        if ( sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
                        {
                            pPhysics->angularVelocity.y *= 2.5f;
                        }
                        else
                        {
                            pPhysics->angularVelocity.y /= 1.4f;
                        }

                        bMoved = 1;
                    }
                }
            }
            else
            {
                pPhysics->angularVelocity.y = 0.0f;
            }

            if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
            {
                return;
            }

            if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
            {
                float mouseTurn;
                mouseTurn = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
                mouseTurn = D3DMultiply(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pActorInfo->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
                pPhysics->angularVelocity.y += mouseTurn;
            }

            if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !sithPlayerControls_bActionActivated )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay && !pThing->thingInfo.actorInfo.bControlsDisabled && !sithWeapon_IsMountingWeapon(pThing) )
                {
                    int bFoundFloorItem;
                    int flags;
                    rdKeyframe* pKeyframe;
                    flags     = RDKEYFRAME_NOLOOP;
                    pCollided = sithCollision_FindActivatedThing(pThing, &bFoundFloorItem);
                    if ( pCollided )
                    {
                        pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
                        sithPlayerControls_pCurActivatedThing = pCollided;
                        sithCog_ThingSendMessage(sithPlayerControls_pCurActivatedThing, pThing, SITHCOG_MSG_ACTIVATE);
                        sithPhysics_ResetThingMovement(pThing);
                        sithPlayerControls_curMoveStatus              = pThing->moveStatus;
                        pThing->moveStatus                            = SITHPLAYERMOVE_ACTIVATING;
                        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                        if ( bFoundFloorItem )
                        {
                            pKeyframe = sithPuppet_GetKeyframeByIndex(0x808A);
                        }
                        else if ( sithInventory_GetCurrentWeapon(pThing) == SITHWEAPON_ZIPPO )
                        {
                            pKeyframe = sithPuppet_GetKeyframeByIndex(0x8091);
                            flags     = RDKEYFRAME_FADEOUT_NOLOOP;
                        }
                        else
                        {
                            pKeyframe = sithPuppet_GetKeyframeByIndex(0x808B);
                        }

                        if ( pKeyframe )
                        {
                            sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, 5, 8, flags, sithPlayerControls_PuppetCallback);
                        }

                        return;
                    }

                    if ( sithPlayerActions_Activate(pThing) == 1 )
                    {
                        return;
                    }

                    pCollided    = NULL;
                    bHasMoveNorm = sithPlayerControls_GetPushPullMoveNorm(&moveNorm, &pThing->orient.lvec, 15.0f);
                    if ( bHasMoveNorm && pThing->attach.flags && !sithInventory_GetCurrentWeapon(pThing) && !sithWeapon_IsMountingWeapon(pThing) )
                    {
                        flags     = 0xA00;
                        pCollided = NULL;
                        sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, pThing->collide.movesize, pThing->collide.movesize, flags);
                        while ( (pCollision = sithCollision_PopStack()) != NULL )
                        {
                            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                            {
                                if ( pCollision->pThingCollided == pThing )
                                {
                                    continue;
                                }

                                if ( (pCollision->pThingCollided->flags & SITH_TF_MOVABLE) != 0 && pCollision->distance < 0.009f )
                                {
                                    pCollided = pCollision->pThingCollided;
                                    break;
                                }
                            }
                        }

                        sithCollision_DecreaseStackLevel();
                        if ( pCollided )
                        {
                            rdVector3 lvec;
                            rdVector3 dir;
                            rdVector3 up;
                            float cosAngle;
                            rdVector_Copy3(&lvec, &pThing->orient.lvec);
                            (&lvec.x)[2] = 0.0f; /* spelling-fit */
                            rdVector_Normalize3Acc(&lvec);

                            rdVector_Sub3(&dir, &pThing->pos, &pCollided->pos);
                            dir.z = 0.0f;
                            rdVector_Normalize3Acc(&dir);

                            cosAngle = (float)fabs(rdVector_Dot3(&lvec, &dir));
                            if ( cosAngle > 0.94f )
                            {
                                rdVector_Set3(&up, 0.0f, 0.0f, 1.0f);
                                sithPlayerControls_pCollidedThing = pCollided;
                                sithPhysics_ResetThingMovement(pThing);

                                rdVector_Copy3(&pThing->orient.lvec, &moveNorm);
                                rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &up);
                                rdVector_Normalize3Acc(&pThing->orient.rvec);
                                rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);

                                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY, sithPlayerControls_PuppetCallback);
                                pThing->moveStatus                  = SITHPLAYERMOVE_PUSHPULL_READY;
                                sithPlayerControls_bActionActivated = 1;
                                return;
                            }
                            else
                            {
                                pCollided = NULL;
                            }
                        }
                    }

                    if ( !pCollided && !sithPlayerControls_bActionActivated && sithPlayerControls_TryBoardVehicle(pThing, 0) )
                    {
                        sithPlayerControls_bActionActivated = 1;
                        return;
                    }
                }

                if ( !bMoved && !pThing->thingInfo.actorInfo.bControlsDisabled )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_STILL;
                }
            }
            break;

        case SITHPLAYERMOVE_WALKING:
            if ( pThing->moveInfo.physics.height != 0.09f )
            {
                pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
                pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
                pThing->moveInfo.physics.height = 0.09f;
                sithPhysics_FindFloor(pThing, 1);
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
            }

            if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) && !sithPlayerControls_bJumpActivated && !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                if ( sithPlayerActions_CanClimbOn2m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon(pThing) )
                {
                    sithPuppet_RemoveAllTracks(pThing);
                    sithPhysics_ResetThingMovement(pThing);
                    sithPlayerActions_ClimbOn2m(pThing);
                    return;
                }

                if ( sithPlayerActions_CanClimbOn1m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon(pThing) )
                {
                    sithPuppet_RemoveAllTracks(pThing);
                    sithPhysics_ResetThingMovement(pThing);
                    sithPlayerActions_ClimbOn1m(pThing);
                    return;
                }

                if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                {
                    float dot;
                    dot = rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_zVector3);
                    if ( dot > 0.7f && dot < 0.75f )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        pThing->thingInfo.actorInfo.bForceMovePlay = 0;
                        return;
                    }
                }

                sithPlayerControls_curJumpDirection = 0;
                sithPlayerActions_JumpStart(pThing);
                return;
            }

            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && pThing->attach.flags )
                    {
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
                        pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLFWD;
                        sithPhysics_ResetThingMovement(pThing);
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLFWD, NULL);
                        return;
                    }

                    direction = 1.0f;
                    pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, direction), 0.9f);
                    if ( bRun )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
                        if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_WALK) )
                        {
                            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
                        }

                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_RUN, NULL);
                    }
                    else
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    }

                    if ( moveScale != 1.0f )
                    {
                        pPhysics->thrust.y = RD_MULTIPLY(pPhysics->thrust.y, moveScale);
                    }

                    bMoved = 1;
                }
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                direction = -1.0f;
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && pThing->attach.flags )
                    {
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALKBACK);
                        pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLBACK;
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLBACK, NULL);
                        return;
                    }

                    pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, direction), 0.5f);
                    bMoved = 1;
                }
            }
            else
            {
                pPhysics->thrust.y = 0.0f;
            }

            if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, -1.0f);
                    pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, -1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
                    bMoved = 1;
                }
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, 1.0f);
                    pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, 1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
                    bMoved = 1;
                }
            }
            else
            {
                pPhysics->angularVelocity.y = 0.0f;
            }

            if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
            {
                float mouseTurn;
                mouseTurn = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
                mouseTurn = D3DMultiply(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pActorInfo->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
                pPhysics->angularVelocity.y += mouseTurn;
            }

            if ( !pThing->thingInfo.actorInfo.bForceMovePlay && !bMoved && !pThing->thingInfo.actorInfo.bControlsDisabled )
            {
                rdKeyframe* pKeyframe = NULL;
                pKeyframe = sithPuppet_GetKeyframe("in_walk_bd_stand.key");
                if ( pKeyframe )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_WALK2STAND;
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                    if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_WALK) )
                    {
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
                    }
                    else if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_WALKBACK) )
                    {
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALKBACK);
                    }

                    sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, 1, 2, RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, sithPlayerControls_PuppetCallback);
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
                    sithPhysics_ResetThingMovement(pThing);
                    return;
                }
            }
            break;

        case SITHPLAYERMOVE_RUNNING:
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) && !sithPlayerControls_bJumpActivated && !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( sithPlayerActions_CanClimbOn2m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon(pThing) )
                    {
                        sithPuppet_RemoveAllTracks(pThing);
                        sithPhysics_ResetThingMovement(pThing);
                        sithPlayerActions_ClimbOn2m(pThing);
                        return;
                    }

                    if ( sithPlayerActions_CanClimbOn1m(pThing) == 1 && !sithPlayerActions_HasActiveWeapon(pThing) )
                    {
                        sithPuppet_RemoveAllTracks(pThing);
                        sithPhysics_ResetThingMovement(pThing);
                        sithPlayerActions_ClimbOn1m(pThing);
                        return;
                    }

                    sithPuppet_bPlayerLeapForward = 1;
                    return;
                }

                if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
                {
                    direction = 1.0f;
                    if ( sithControl_GetKey(SITHCONTROL_ACT3, NULL) && pThing->attach.flags )
                    {
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_RUN);
                        sithPhysics_ResetThingMovement(pThing);
                        pThing->moveStatus = SITHPLAYERMOVE_JUMPROLLFWD;
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPROLLFWD, NULL);
                        return;
                    }

                    pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, direction), 0.9f);
                    if ( bRun )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_RUNNING;
                    }
                    else
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                        if ( sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_RUN) )
                        {
                            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_RUN);
                        }

                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WALK, NULL);
                    }

                    if ( moveScale != 1.0f )
                    {
                        pPhysics->thrust.y = RD_MULTIPLY(pPhysics->thrust.y, moveScale);
                    }

                    bMoved = 1;
                }
                else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_WALKING;
                    direction = -1.0f;
                    pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, direction), 0.5f);
                    bMoved = 1;
                }
                else
                {
                    pPhysics->thrust.y = 0.0f;
                }

                if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
                {
                    pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, -1.0f);
                    pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, -1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
                    bMoved = 1;
                }
                else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
                {
                    pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, 1.0f);
                    pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, 1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
                    bMoved = 1;
                }
                else
                {
                    pPhysics->angularVelocity.y = 0.0f;
                }

                if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
                {
                    float mouseTurn;
                    mouseTurn = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
                    mouseTurn = D3DMultiply(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pActorInfo->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
                    pPhysics->angularVelocity.y += mouseTurn;
                }

                if ( !bMoved && !pThing->thingInfo.actorInfo.bControlsDisabled )
                {
                    rdKeyframe* pKeyframe = NULL;
                    pKeyframe = sithPuppet_GetKeyframe("in_walk_bd_stand.key");
                    if ( pKeyframe )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_WALK2STAND;
                        pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_RUN);
                        sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, 1, 2, RDKEYFRAME_FADEOUT_NOLOOP | RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, sithPlayerControls_PuppetCallback);
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
                        sithPhysics_ResetThingMovement(pThing);
                        return;
                    }
                }
            }
            break;

        case SITHPLAYERMOVE_CRAWL_STILL:
            if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & (SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER)) != 0 )
            {
                sithPlayerActions_Crawl2Stand(pThing);
                return;
            }

            if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                sithPlayerActions_Crawl2Stand(pThing);
                return;
            }

            if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    pPhysics->thrust.y = RD_MULTIPLY(1.0f, secDeltaTime);
                    pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust, 1.0f), 0.5f) + pPhysics->thrust.y;
                }
            }
            else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
            {
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    pPhysics->thrust.y = RD_MULTIPLY(-1.0f, secDeltaTime);
                    pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust, -1.0f), 0.5f) + pPhysics->thrust.y;
                }
            }
            else
            {
                pPhysics->thrust.y = 0.0f;
            }

            if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
            {
                pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, -0.5f);
                pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, -0.5f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
            }
            else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
            {
                pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, 0.5f);
                pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, 0.5f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
            }
            else
            {
                pPhysics->angularVelocity.y = 0.0f;
            }

            if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                int bFoundFloorItem;

                if ( sithPlayerActions_Activate(pThing) == 1 )
                {
                    return;
                }

                pCollided = sithCollision_FindActivatedThing(pThing, &bFoundFloorItem);
                if ( pCollided )
                {
                    pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
                    sithPlayerControls_pCurActivatedThing = pCollided;
                    sithCog_ThingSendMessage(sithPlayerControls_pCurActivatedThing, pThing, SITHCOG_MSG_ACTIVATE);
                    sithPhysics_ResetThingMovement(pThing);
                    sithPlayerControls_curMoveStatus              = pThing->moveStatus;
                    pThing->moveStatus                            = SITHPLAYERMOVE_ACTIVATING;
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_ACTIVATE, sithPlayerControls_PuppetCallback);
                    return;
                }
            }

            if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
            {
                float mouseTurn;
                mouseTurn = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
                mouseTurn = D3DMultiply(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pActorInfo->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
                pPhysics->angularVelocity.y += mouseTurn;
            }
            break;

        case SITHPLAYERMOVE_PUSHPULL_READY:
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                if ( !sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
                {
                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                    pThing->moveStatus                = SITHPLAYERMOVE_STILL;
                    sithPlayerControls_pCollidedThing = NULL;
                    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                    return;
                }

                if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
                {
                    if ( sithPlayerControls_pCollidedThing )
                    {
                        bHasMoveNorm = sithPlayerControls_GetPushPullMoveNorm(&moveNorm, &pThing->orient.lvec, 15.0f);
                        if ( bHasMoveNorm )
                        {
                            rdVector3 pos;
                            rdVector3 dir;
                            SithSector* pSector;
                            int bAligned;

                            bClear      = 1;
                            searchFlags = 0xA00;
                            sithCollision_SearchForCollisions(sithPlayerControls_pCollidedThing->pInSector, sithPlayerControls_pCollidedThing, &sithPlayerControls_pCollidedThing->pos, &moveNorm, 0.19f, sithPlayerControls_pCollidedThing->collide.movesize >= 0.1f ? 0.095f : 0.05f, searchFlags);
                            while ( (pCollision = sithCollision_PopStack()) != NULL )
                            {
                                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                                {
                                    if ( pCollision->pThingCollided == pThing )
                                    {
                                        continue;
                                    }

                                    bClear = 0;
                                }

                                if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                                {
                                    bClear = 0;
                                }
                            }

                            sithCollision_DecreaseStackLevel();
                            if ( !bClear )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_ALERT);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus                = SITHPLAYERMOVE_STILL;
                                sithPlayerControls_pCollidedThing = NULL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                return;
                            }

                            rdVector_ScaleAdd3(&pos, &moveNorm, 0.2f, &sithPlayerControls_pCollidedThing->pos);
                            rdVector_Neg3(&dir, &rdroid_zVector3);
                            pSector = sithCollision_FindSectorInRadius(sithPlayerControls_pCollidedThing->pInSector, &sithPlayerControls_pCollidedThing->pos, &pos, 0.0f);
                            if ( !pSector )
                            {
                                break;
                            }

                            bClear = 0;
                            sithCollision_SearchForCollisions(pSector, NULL, &pos, &dir, 0.16f, 0.01f, searchFlags);
                            while ( (pCollision = sithCollision_PopStack()) != NULL )
                            {
                                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                                {
                                    if ( pCollision->pThingCollided == sithPlayerControls_pCollidedThing )
                                    {
                                        continue;
                                    }

                                    bClear = 1;
                                    break;
                                }
                                else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
                                {
                                    float dot;
                                    dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_zVector3);
                                    if ( dot > 0.999f )
                                    {
                                        if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                                        {
                                            rdFace* pSurfFace;
                                            rdFace* pFace;
                                            float surfZ;
                                            float faceZ;
                                            float dz;
                                            pSurfFace = &pCollision->pSurfaceCollided->face;
                                            pFace     = pThing->attach.pFace;
                                            surfZ     = sithWorld_pCurrentWorld->aVertices[pSurfFace->aVertices[0]].z;
                                            faceZ     = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]].z;
                                            dz        = (float)fabs(surfZ - faceZ);
                                            if ( dz < 0.005f )
                                            {
                                                bClear = 1;
                                                break;
                                            }
                                            else
                                            {
                                                bClear = 0;
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            bClear = 1;
                                        }

                                        break;
                                    }
                                }
                            }

                            sithCollision_DecreaseStackLevel();
                            if ( !bClear )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_ALERT);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                sithPlayerControls_pCollidedThing = NULL;
                                return;
                            }

                            if ( !strncmp(sithPlayerControls_pCollidedThing->aName, "pushgear", 8) )
                            {
                                rdVector3 lvec;
                                float cosAngle;
                                rdVector_Copy3(&lvec, &sithPlayerControls_pCollidedThing->orient.lvec);
                                cosAngle = (float)fabs(rdVector_Dot3(&lvec, &moveNorm));
                                if ( cosAngle < 0.98f )
                                {
                                    bAligned = 0;
                                }
                                else
                                {
                                    bAligned = 1;
                                }

                                if ( !bAligned )
                                {
                                    if ( sithPlayerControls_secCommentWait == 0.0f )
                                    {
                                        sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_ALERT);
                                        sithPlayerControls_secCommentWait = 3.0f;
                                    }

                                    sithPlayerControls_pCollidedThing = NULL;
                                    return;
                                }
                            }

                            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                            sithPlayerActions_PushItem(pThing, sithPlayerControls_pCollidedThing, &moveNorm);
                            return;
                        }
                    }
                }
                else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
                {
                    if ( sithPlayerControls_pCollidedThing )
                    {
                        bHasMoveNorm = sithPlayerControls_GetPushPullMoveNorm(&moveNorm, &pThing->orient.lvec, 15.0f);
                        if ( bHasMoveNorm )
                        {
                            rdVector3 dir;
                            rdVector3 pos;
                            SithSector* pSector;
                            float radius;
                            int bAligned;

                            bClear = 1;
                            rdVector_Neg3(&dir, &moveNorm);
                            searchFlags = 0xA00;
                            if ( sithPlayerControls_pCollidedThing->collide.movesize >= 0.1f )
                            {
                                radius = 0.095f;
                                if ( pThing->pos.z > sithPlayerControls_pCollidedThing->pos.z )
                                {
                                    float dz;
                                    dz = D3DVAL(pThing->pos.z - sithPlayerControls_pCollidedThing->pos.z); /* spelling-fit */
                                    if ( dz > 0.02 && strncmp(sithPlayerControls_pCollidedThing->aName, "pushgear", 8) )
                                    {
                                        bClear = 0;
                                        if ( sithPlayerControls_secCommentWait == 0.0f )
                                        {
                                            sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                            sithPlayerControls_secCommentWait = 3.0f;
                                        }

                                        sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                        pThing->moveStatus                = SITHPLAYERMOVE_STILL;
                                        sithPlayerControls_pCollidedThing = NULL;
                                        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                radius = 0.05f;
                            }

                            sithCollision_SearchForCollisions(sithPlayerControls_pCollidedThing->pInSector, sithPlayerControls_pCollidedThing, &sithPlayerControls_pCollidedThing->pos, &dir, 0.19f + pThing->collide.movesize, radius, searchFlags);
                            while ( (pCollision = sithCollision_PopStack()) != NULL )
                            {
                                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                                {
                                    if ( pCollision->pThingCollided == pThing )
                                    {
                                        continue;
                                    }

                                    bClear = 0;
                                }

                                if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                                {
                                    bClear = 0;
                                    break;
                                }
                            }

                            sithCollision_DecreaseStackLevel();
                            if ( !bClear )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus                = SITHPLAYERMOVE_STILL;
                                sithPlayerControls_pCollidedThing = NULL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                return;
                            }

                            sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &dir, 0.19f + pThing->collide.movesize, pThing->collide.movesize, searchFlags);
                            while ( (pCollision = sithCollision_PopStack()) != NULL )
                            {
                                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                                {
                                    if ( pCollision->pThingCollided == pThing )
                                    {
                                        continue;
                                    }

                                    bClear = 0;
                                }

                                if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                                {
                                    bClear = 0;
                                    break;
                                }
                            }

                            sithCollision_DecreaseStackLevel();
                            if ( !bClear )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus                = SITHPLAYERMOVE_STILL;
                                sithPlayerControls_pCollidedThing = NULL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                return;
                            }

                            rdVector_Neg3Acc(&moveNorm);
                            rdVector_ScaleAdd3(&pos, &moveNorm, 0.2f, &sithPlayerControls_pCollidedThing->pos);
                            rdVector_Neg3(&dir, &rdroid_zVector3);
                            pSector = sithCollision_FindSectorInRadius(sithPlayerControls_pCollidedThing->pInSector, &sithPlayerControls_pCollidedThing->pos, &pos, 0.0f);
                            if ( !pSector )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                sithPlayerControls_pCollidedThing = NULL;
                                break;
                            }

                            bClear = 0;
                            sithCollision_SearchForCollisions(pSector, NULL, &pos, &dir, 0.17f, 0.04f, searchFlags);
                            while ( (pCollision = sithCollision_PopStack()) != NULL )
                            {
                                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                                {
                                    if ( pCollision->pThingCollided != pThing )
                                    {
                                        bClear = 1;
                                        break;
                                    }
                                }
                                else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
                                {
                                    float dot;
                                    dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_zVector3);
                                    if ( dot > 0.999f )
                                    {
                                        bClear = 1;
                                        break;
                                    }
                                }
                            }

                            sithCollision_DecreaseStackLevel();
                            if ( !bClear )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                sithPlayerControls_pCollidedThing = NULL;
                                return;
                            }

                            rdVector_ScaleAdd3(&pos, &moveNorm, 0.2f, &pThing->pos);
                            rdVector_Neg3(&dir, &rdroid_zVector3);
                            pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &pos, 0.0f);
                            if ( !pSector )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                sithPlayerControls_pCollidedThing = NULL;
                                return;
                            }

                            bClear = 0;
                            sithCollision_SearchForCollisions(pSector, NULL, &pos, &dir, 0.063f, 0.04f, searchFlags);
                            while ( (pCollision = sithCollision_PopStack()) != NULL )
                            {
                                if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                                {
                                    if ( pCollision->pThingCollided != pThing )
                                    {
                                        if ( pCollision->distance < 0.02f || pCollision->distance > 0.07f )
                                        {
                                            bClear = 0;
                                            break;
                                        }
                                        else
                                        {
                                            bClear = 1;
                                            break;
                                        }
                                    }
                                }
                                else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && pCollision->pSurfaceCollided )
                                {
                                    float dot;
                                    if ( pCollision->distance < 0.02f || pCollision->distance > 0.07f )
                                    {
                                        bClear = 0;
                                        break;
                                    }

                                    dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_zVector3);
                                    if ( dot > 0.999f )
                                    {
                                        if ( (sithPlayerControls_pCollidedThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                                        {
                                            rdFace* pSurfFace;
                                            rdFace* pFace;
                                            float surfZ;
                                            float faceZ;
                                            float dz;
                                            pSurfFace = &pCollision->pSurfaceCollided->face;
                                            pFace     = sithPlayerControls_pCollidedThing->attach.pFace;
                                            surfZ     = sithWorld_pCurrentWorld->aVertices[pSurfFace->aVertices[0]].z;
                                            faceZ     = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]].z;
                                            dz        = (float)fabs(surfZ - faceZ);
                                            if ( dz < 0.005f )
                                            {
                                                bClear = 1;
                                                break;
                                            }
                                            else
                                            {
                                                bClear = 0;
                                                break;
                                            }
                                        }
                                        else
                                        {
                                            bClear = 1;
                                        }

                                        break;
                                    }
                                }
                            }

                            sithCollision_DecreaseStackLevel();
                            if ( !bClear )
                            {
                                if ( sithPlayerControls_secCommentWait == 0.0f )
                                {
                                    sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                    sithPlayerControls_secCommentWait = 3.0f;
                                }

                                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                                pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                                pThing->moveStatus = SITHPLAYERMOVE_STILL;
                                sithPlayerControls_pCollidedThing = NULL;
                                return;
                            }

                            rdVector_Copy3(&dir, &moveNorm);
                            if ( !strncmp(sithPlayerControls_pCollidedThing->aName, "pushgear", 8) )
                            {
                                rdVector3 lvec;
                                float cosAngle;
                                rdVector_Copy3(&lvec, &sithPlayerControls_pCollidedThing->orient.lvec);
                                cosAngle = (float)fabs(rdVector_Dot3(&lvec, &moveNorm));
                                if ( cosAngle < 0.98f )
                                {
                                    bAligned = 0;
                                }
                                else
                                {
                                    bAligned = 1;
                                }

                                if ( !bAligned )
                                {
                                    if ( sithPlayerControls_secCommentWait == 0.0f )
                                    {
                                        sithSoundClass_PlayVoiceModeRandom(pThing, SITHSOUNDCLASS_BOAST);
                                        sithPlayerControls_secCommentWait = 3.0f;
                                    }

                                    sithPlayerControls_pCollidedThing = NULL;
                                    return;
                                }
                            }

                            sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_PUSHPULLREADY);
                            rdVector_Neg3Acc(&moveNorm);
                            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
                            sithPlayerActions_PullItem(pThing, sithPlayerControls_pCollidedThing, &moveNorm);
                            return;
                        }
                    }
                }
            }
            break;

        case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
            if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) && !sithPlayerControls_bJumpActivated )
            {
                SithPuppetTrack* pTrack = NULL;
                pTrack = sithPuppet_GetModeTrack(pThing, SITHPUPPETSUBMODE_SLIDEDOWNFWD);
                if ( pTrack )
                {
                    sithPuppet_StopKey(pThing->renderData.pPuppet, pTrack->trackNum, 0.0f);
                    sithPuppet_RemoveTrack(pThing, pTrack);
                }

                sithPhysics_ResetThingMovement(pThing);
                pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPLEFT, NULL);
                sithPlayerControls_bJumpActivated = 1;
            }
            break;
    }
}

void J3DAPI sithPlayerControls_ProcessClimbMove(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;  // unreferenced; the debug build's frame keeps its slot
    SithActorInfo* pInfo = &pThing->thingInfo.actorInfo;  // unreferenced; the debug build's frame keeps its slot
    int retVal = 0;  // unreferenced; the debug build's frame keeps its slot
    float moveScale = 1.0f;  // unreferenced; the debug build's frame keeps its slot
    rdVector3 moveNorm;
    SithSurface* pSurf;
    int bHitNonClimbSurface;

    if ( pThing->thingInfo.actorInfo.bControlsDisabled )
    {
        return;
    }

    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        return;
    }

    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        rdVector_Set3(&pThing->moveInfo.physics.velocity, 0.0f, 0.0f, 0.0f);

        if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector_Set3(&moveNorm, 0.0f, 0.0f, 1.0f);
            rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
            rdVector_Normalize3Acc(&moveNorm);
            pSurf = sithPlayerControls_SearchForClimbSurface(pThing, &moveNorm, 1, &bHitNonClimbSurface);
            if ( !pSurf && !bHitNonClimbSurface && sithPlayerActions_CanPullUp(pThing) == 1 )
            {
                sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerControls_climbPupTrackNum, 0.0f);
                sithPlayerControls_climbPupTrackNum = -1;
                sithPlayerActions_ClimbPullUp(pThing);
                return;
            }
        }

        if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) && !sithPlayerControls_bJumpActivated && !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            sithPlayerActions_UnmountWall(pThing, sithPlayerControls_climbPupTrackNum);
            sithPlayerControls_bJumpActivated  = 1;
            sithPlayerControls_climbPupTrackNum = -1;
            return;
        }

        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                rdVector_Set3(&moveNorm, 0.0f, 0.0f, 1.0f);
                rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
                rdVector_Normalize3Acc(&moveNorm);
                pSurf = sithPlayerControls_SearchForClimbSurface(pThing, &moveNorm, 1, &bHitNonClimbSurface);
                if ( pSurf && !bHitNonClimbSurface )
                {
                    sithPlayerActions_ClimbMove(pThing, pSurf, 1);
                }
                else if ( !bHitNonClimbSurface )
                {
                    sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerControls_climbPupTrackNum, 0.0f);
                    sithPlayerControls_climbPupTrackNum = -1;
                    sithPlayerActions_ClimbPullUp(pThing);
                    return;
                }

                return;
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                rdVector_Set3(&moveNorm, 0.0f, 0.0f, -1.0f);
                rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
                rdVector_Normalize3Acc(&moveNorm);
                pSurf = sithPlayerControls_SearchForClimbSurface(pThing, &moveNorm, 2, &bHitNonClimbSurface);
                if ( pSurf )
                {
                    sithPlayerActions_ClimbMove(pThing, pSurf, 2);
                }

                return;
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
            {
                rdVector_Set3(&moveNorm, 1.0f, 0.0f, 0.0f);
                rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
                rdVector_Normalize3Acc(&moveNorm);
                pSurf = sithPlayerControls_SearchForClimbSurface(pThing, &moveNorm, 3, &bHitNonClimbSurface);
                if ( pSurf )
                {
                    sithPlayerActions_ClimbMove(pThing, pSurf, 3);
                }

                return;
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) && !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector_Set3(&moveNorm, -1.0f, 0.0f, 0.0f);
            rdMatrix_TransformVector34Acc(&moveNorm, &pThing->orient);
            rdVector_Normalize3Acc(&moveNorm);
            pSurf = sithPlayerControls_SearchForClimbSurface(pThing, &moveNorm, 4, &bHitNonClimbSurface);
            if ( pSurf )
            {
                sithPlayerActions_ClimbMove(pThing, pSurf, 4);
            }
        }

    }
}

void J3DAPI sithPlayerControls_ProcessJewelFlyMove(SithThing* pThing, float secDeltaTime)
{
    float moveScale = 1.0f;
    SithPhysicsInfo* pPhysData = &pThing->moveInfo.physics;
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    float thrustStep = 0.0002f;
    rdVector3 moveDir;
    int bCanMoveDown;
    int status;
    rdVector3 leapVel;
    int bCanRise;
    SithCollision* pCurCollision;

    bCanRise = bCanMoveDown = 1;
    if ( pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
    {
        return;
    }

    pThing->collide.type = SITH_COLLIDE_SPHERE;
    if ( (pPhysData->flags & SITH_PF_FLY) == 0 || pThing->attach.flags )
    {
        if ( pThing->attach.flags )
        {
            pThing->moveStatus = SITHPLAYERMOVE_STILL;
        }
        else
        {
            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        }

        if ( sithPlayerActions_jewelFlyingPuppetTrackNum >= 0 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerActions_jewelFlyingPuppetTrackNum, 0.0f);
        }

        sithPlayerActions_jewelFlyingPuppetTrackNum = -1;
        sithPlayerActions_DisableJewelFlying();
        return;
    }

    pPhysData->thrust.y = 0.0f;
    pPhysData->thrust.x = 0.0f;

    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_zVector3, 0.12f, 0.05f, 0xA00);
    while ( (pCurCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCurCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            bCanRise = 0;
            break;
        }
        else if ( (pCurCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanRise = 0;
            break;
        }
        else if ( (pCurCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && (pCurCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bCanRise = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    rdVector_Neg3(&moveDir, &rdroid_zVector3);
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveDir, 0.1f, 0.05f, 0x200);
    while ( (pCurCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCurCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            bCanMoveDown = 0;
            break;
        }
        else if ( (pCurCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanMoveDown = 0;
            break;
        }
        else if ( (pCurCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && (pCurCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bCanMoveDown = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( sithControl_GetKey(SITHCONTROL_FORWARD, &status) )
    {
        if ( bCanRise == 1 )
        {
            pPhysData->thrust.z += (float)RD_MULTIPLY(thrustStep, sithTime_fps);
            if ( pPhysData->thrust.z > 1.0f )
            {
                pPhysData->thrust.z = 1.0f;
            }
        }
        else
        {
            pPhysData->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, &status) )
    {
        if ( bCanMoveDown == 1 )
        {
            pPhysData->thrust.z -= (float)RD_MULTIPLY(thrustStep, sithTime_fps);
            if ( pPhysData->thrust.z < -1.0f )
            {
                pPhysData->thrust.z = -1.0f;
            }
        }
        else
        {
            pPhysData->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }
    }
    else
    {
        pPhysData->thrust.z = 0.0f;
        if ( !bCanRise )
        {
            pPhysData->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }

        if ( !bCanMoveDown )
        {
            pPhysData->velocity.z = 0.0f;
            sithPhysics_ResetThingMovement(pThing);
        }
    }

    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, &status) )
    {
        pPhysData->angularVelocity.y = RD_MULTIPLY(-1.0f, sithTime_fps);
        pPhysData->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, -1.0f), J3DMIN(moveScale, 1.0f)) + pPhysData->angularVelocity.y;
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, &status) )
    {
        pPhysData->angularVelocity.y = RD_MULTIPLY(1.0f, sithTime_fps);
        pPhysData->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, 1.0f), J3DMIN(moveScale, 1.0f)) + pPhysData->angularVelocity.y;
    }
    else
    {
        pPhysData->angularVelocity.y = 0.0f;
    }

    if ( sithControl_GetKey(SITHCONTROL_JUMP, &status) )
    {
        rdVector_Set3(&leapVel, 0.0f, 0.5f, 0.0f);
        rdMatrix_TransformVector34Acc(&leapVel, &pThing->orient);
        rdVector_Copy3(&pPhysData->velocity, &leapVel);

        pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_FALL, NULL);
        if ( sithPlayerActions_jewelFlyingPuppetTrackNum >= 0 )
        {
            sithPuppet_StopKey(pThing->renderData.pPuppet, sithPlayerActions_jewelFlyingPuppetTrackNum, 0.0f);
        }

        sithPlayerActions_jewelFlyingPuppetTrackNum = -1;
        pPhysData->flags &= ~SITH_PF_FLY;
        pPhysData->flags |= SITH_PF_USEGRAVITY;
        sithPlayer_impFireType = -1;
        sithWeapon_DeselectWeapon(pThing);
        sithPlayerActions_DisableJewelFlying();
        sithInventory_SetSwimmingInventory(pThing, 1);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(sithPlayerControls_countfit_1, 30, 15) CF_PROTO(sithPlayerControls_countfit_17, 6) /* end count-fit */
void J3DAPI sithPlayerControls_ProcessFlyAndUnderwaterMove(SithThing* pThing, float secDeltaTime)
{
    float turnScale = 1.0f;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pInfo = &pThing->thingInfo.actorInfo;
    int bFlightState = 0;
    int bInWater = 0;
    float moveRatio = 0.0f;
    int state;
    int ledgeKind;

    sithPlayerControls_curJumpDirection = 0;

#line 2673
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld,pThing));

    if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        bInWater = 1;
        bFlightState     = 0;
    }

    if ( (pPhysics->flags & SITH_PF_FLY) != 0 )
    {
        bFlightState     = 1;
        bInWater = 0;
    }

    if ( bInWater )
    {
        if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) )
        {
            if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
            {
                ledgeKind = sithPlayerControls_CheckWaterLedge(pThing);
                if ( ledgeKind == 1 )
                {
                    sithPhysics_ResetThingMovement(pThing);
                    pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP;
                    pThing->collide.movesize /= 4.0f;
                    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNTFROMWATER, NULL);
                    return;
                }

                if ( ledgeKind == 2 )
                {
                    sithPlayerActions_ClimbOn1m(pThing);
                    return;
                }
            }
            else
            {
                moveRatio = 1.0f;
                pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pInfo->maxThrust + pInfo->extraSpeed, moveRatio), 0.9f);
            }
        }
        else
        {
            pPhysics->thrust.y = 0.0f;
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
        }
        else
        {
            pPhysics->thrust.z = 0.0f;
        }

        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            pPhysics->angularVelocity.x = RD_MULTIPLY(-1.0f, sithTime_fps);
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            pPhysics->angularVelocity.x = RD_MULTIPLY(1.0f, sithTime_fps);
        }
        else
        {
            pPhysics->angularVelocity.x = 0.0f;
        }

        if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(-1.0f, sithTime_fps);
            pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pInfo->maxRotVelocity, -1.0f), J3DMIN(turnScale, 1.0f)) + pPhysics->angularVelocity.y;
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(1.0f, sithTime_fps);
            pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pInfo->maxRotVelocity, 1.0f), J3DMIN(turnScale, 1.0f)) + pPhysics->angularVelocity.y;
        }
        else
        {
            pPhysics->angularVelocity.y = 0.0f;
        }

        if ( (pPhysics->thrust.x != 0.0f || pPhysics->thrust.y != 0.0f || pPhysics->thrust.z != 0.0f)
            && (pInfo->headPYR.x != 0.0f || pInfo->headPYR.y != 0.0f || pInfo->headPYR.z != 0.0f)
            && bFlightState
            && (pPhysics->flags & SITH_PF_ONWATERSURFACE) == 0 )
        {
            rdMatrix34 orient;
            rdMatrix_BuildRotate34(&orient, &pInfo->headPYR);
            rdMatrix_TransformVector34Acc(&pPhysics->thrust, &orient);
        }

        return;
    }

    if ( (pPhysics->flags & SITH_PF_FLY) != 0 || pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        bFlightState = 1;
    }

    if ( (sithControl_controlOptions & 2) != 0 || sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
    {
        turnScale *= 2.5f;
    }

    if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER )
    {
        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
        {
            if ( bInWater )
            {
                pPhysics->angularVelocity.x = RD_MULTIPLY(-1.0f, sithTime_fps);
            }
            else
            {
                moveRatio = 1.0f;
                pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pInfo->maxThrust + pInfo->extraSpeed, moveRatio), 0.9f);
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
        {
            if ( bInWater )
            {
                pPhysics->angularVelocity.x = RD_MULTIPLY(1.0f, sithTime_fps);
            }
            else
            {
                moveRatio = -1.0f;
                pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pInfo->maxThrust + pInfo->extraSpeed, moveRatio), 0.5f);
            }
        }
        else
        {
            pPhysics->thrust.y = 0.0f;
        }

        if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(-1.0f, sithTime_fps);
            pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pInfo->maxRotVelocity, -1.0f), J3DMIN(turnScale, 1.0f)) + pPhysics->angularVelocity.y;
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(1.0f, sithTime_fps);
            pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pInfo->maxRotVelocity, 1.0f), J3DMIN(turnScale, 1.0f)) + pPhysics->angularVelocity.y;
        }
        else
        {
            pPhysics->angularVelocity.y = 0.0f;
        }

        pPhysics->thrust.z = 0.0f;

        if ( (pPhysics->thrust.x != 0.0f || pPhysics->thrust.y != 0.0f || pPhysics->thrust.z != 0.0f)
            && (pInfo->headPYR.x != 0.0f || pInfo->headPYR.y != 0.0f || pInfo->headPYR.z != 0.0f)
            && bFlightState
            && (pPhysics->flags & SITH_PF_ONWATERSURFACE) == 0 )
        {
            rdMatrix34 orient;
            rdMatrix_BuildRotate34(&orient, &pInfo->headPYR);
            rdMatrix_TransformVector34Acc(&pPhysics->thrust, &orient);
        }

        if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 && pPhysics->angularVelocity.y == 0.0f )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
            pPhysics->angularVelocity.y = RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pInfo->maxRotVelocity) + pPhysics->angularVelocity.y;
        }

        if ( bFlightState || bInWater )
        {
            if ( sithControl_GetKey(SITHCONTROL_JUMP, &state) )
            {
                if ( pThing->attach.flags )
                {
                    sithThing_DetachThing(pThing);
                    pPhysics->thrust.z += 2.0f;
                }
                else
                {
                    pPhysics->thrust.z += RD_MULTIPLY(pInfo->maxThrust, 0.5f);
                }
            }

            if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &state) )
            {
                pPhysics->thrust.z -= RD_MULTIPLY(pInfo->maxThrust, 0.5f);
            }
        }
    }
}

void J3DAPI sithPlayerControls_ProcessGeneralControls(SithThing* pThing, float secDeltaTime)
{
    int state;
    size_t typeId;
    SithInventoryType* pType;

    sithControl_GetKey(SITHCONTROL_MAP, &state);
    if ( (state & 1) != 0 )
    {
        sithOverlayMap_ToggleMap();
    }

    if ( sithControl_GetKey(SITHCONTROL_INCREASE, &state) )
    {
        sithOverlayMap_ZoomIn();
    }

    if ( sithControl_GetKey(SITHCONTROL_DECREASE, &state) )
    {
        sithOverlayMap_ZoomOut();
    }

    if ( !sithPlayerControls_bHealthActivated && sithControl_GetKey(SITHCONTROL_HEALTH, &state) )
    {
        typeId = 0;
        if ( sithInventory_GetInventory(pThing, 39) != 0.0 && sithInventory_IsInventoryAvailable(pThing, 39) )
        {
            typeId = 39;
        }
        else if ( sithInventory_GetInventory(pThing, 49) != 0.0 && sithInventory_IsInventoryAvailable(pThing, 49) )
        {
            typeId = 49;
        }
        else if ( sithInventory_GetInventory(pThing, 38) != 0.0 && sithInventory_IsInventoryAvailable(pThing, 38) )
        {
            typeId = 38;
        }
        else if ( sithInventory_GetInventory(pThing, 48) != 0.0 && sithInventory_IsInventoryAvailable(pThing, 48) )
        {
            typeId = 48;
        }

        if ( typeId )
        {
            pType = sithInventory_GetType(typeId);
            if ( pType->pCog )
            {
                sithCog_SendMessage(pType->pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_INT, typeId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
            }
        }

        sithPlayerControls_bHealthActivated = 1;
    }
}

int J3DAPI sithPlayerControls_ProcessEditorDebugControls(SithThing* pThing, float secDeltaTime)
{
    int mode;
    size_t keyId;
    float zoom;
    rdVector3 pyr;
    float turnRate;
    rdMatrix34 rot;

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 && !sithControl_GetKey(SITHCONTROL_DEBUG, NULL) )
    {
        return 0;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
    }

    for ( keyId = SITHCONTROL_WSELECT0; keyId <= SITHCONTROL_WSELECT9; ++keyId )
    {
        sithControl_GetKey(keyId, &mode);
        if ( mode )
        {
            sithPlayerActions_MoveToPlayerPosition(pThing, keyId - SITHCONTROL_WSELECT0);
            break;
        }
    }

    sithControl_GetKey(SITHCONTROL_JUMP, &mode);
    if ( mode )
    {
        sithActor_DamageActor(pThing, pThing, 200.0f, 1);
        return 1;
    }

    sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &mode);
    if ( mode && sithCamera_pCurCamera != &sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM] )
    {
        sithCamera_SetCurrentCamera(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM]);
        return 1;
    }

    sithControl_GetKey(SITHCONTROL_ACT2, &mode);
    if ( mode )
    {
        pThing->moveStatus                            = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize                      = 0.04f;
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->thingInfo.actorInfo.bForceMovePlay    = 0;
        pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
        pThing->moveInfo.physics.height               = 0.09f;
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        return 1;
    }

    sithControl_GetKey(SITHCONTROL_STPRIGHT, &mode);
    if ( mode && pThing->moveStatus != 10 )
    {
        pThing->moveStatus        = 10;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
        sithThing_DetachThing(pThing);
        sithPuppet_PlayForceMoveMode(pThing, 48, NULL);
    }

    sithControl_GetKey(SITHCONTROL_ACTIVATE, &mode);
    if ( mode )
    {
        sithCamera_CycleCamera();
    }

    if ( sithCamera_pCurCamera->type == SITHCAMERA_ORBITAL )
    {
        turnRate = RD_MULTIPLY(secDeltaTime, 90.0f);
        if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, &mode) )
        {
            pyr.y = 1.0f;
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, &mode) )
        {
            pyr.y = -1.0f;
        }
        else
        {
            pyr.y = 0.0f;
        }

        pyr.x = sithControl_GetKeyAsAxis(SITHCONTROL_PITCH);
        pyr.z = 0.0f;
        rdVector_Scale3Acc(&pyr, turnRate);
        if ( pyr.x != 0.0f || pyr.y != 0.0f || pyr.z != 0.0f )
        {
            rdMatrix_BuildRotate34(&rot, &pyr);
            rdMatrix_TransformVector34Acc(&sithPlayerControls_vecDebugCamDir, &rot);
            rdVector_Normalize3Acc(&sithPlayerControls_vecDebugCamDir);
        }

        turnRate = RD_MULTIPLY(secDeltaTime, 0.1f);
        if ( sithControl_GetKey(SITHCONTROL_FORWARD, &mode) )
        {
            zoom = RD_MULTIPLY(turnRate, -1.0f);
        }
        else if ( sithControl_GetKey(SITHCONTROL_BACK, &mode) )
        {
            zoom = RD_MULTIPLY(turnRate, 1.0f);
        }
        else
        {
            zoom = 0.0f;
        }

        if ( zoom != 0.0f )
        {
            sithPlayerControls_cameraDistance += zoom;
            sithPlayerControls_cameraDistance = STDMATH_CLAMP(sithPlayerControls_cameraDistance, pThing->collide.size, 3.0f);

#line 3056
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "New camera distance = %f.\n", sithPlayerControls_cameraDistance, 0, 0, 0);
        }

        rdVector_Neg3(&sithCamera_orbCamOrient.lvec, &sithPlayerControls_vecDebugCamDir);
        rdVector_Cross3(&sithCamera_orbCamOrient.rvec, &sithCamera_orbCamOrient.lvec, &rdroid_zVector3);
        rdVector_Cross3(&sithCamera_orbCamOrient.uvec, &sithCamera_orbCamOrient.rvec, &sithCamera_orbCamOrient.lvec);
        rdMatrix_Normalize34(&sithCamera_orbCamOrient);

        rdVector_Scale3(&sithCamera_orbCamOrient.dvec, &sithPlayerControls_vecDebugCamDir, sithPlayerControls_cameraDistance);
    }

    sithControl_GetKey(SITHCONTROL_MAP, &mode);
    if ( mode )
    {
        sith_mode.mapModeFlags ^= SITHMAPMODE_SHOWALLTHINGS | SITHMAPMODE_SHOWALLSECTORS;
    }

    return 1;
}

int J3DAPI sithPlayerControls_ProcessDeadPlayer(SithThing* pThing, float secDeltaTime)
{
    return 1;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPlayerControls_countfit_2, 7) /* end count-fit */
void J3DAPI sithPlayerControls_ProcessJumpForwardMove(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor = &pThing->thingInfo.actorInfo;
    float thrustFactor = 0.0f;
    float moveScale = 1.0f;
    SithSurface* pHitSurf;
    rdFace* pFace;
    rdModel3Mesh* pHitMesh;
    SithThing* pHitObject;
    float curSpeed;
    float curLen;

    thrustFactor = 1.0f;
    pHitSurf = NULL;
    pFace = NULL;
    if ( pThing->attach.flags && pThing->moveStatus == SITHPLAYERMOVE_FALLING )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPuppet_RemoveAllTracks(pThing);
        return;
    }

    curSpeed = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));
    if ( curSpeed < 1.1f )
    {
        curLen = (float)sithPlayerActions_SearchForCollision(pThing, &pThing->orient.lvec, &pHitSurf, &pFace, &pHitMesh, &pHitObject);
        if ( curLen >= 0.0f )
        {
            if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
            {
                sithPlayerControls_bJumpActivated = 1;
            }

            if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
            {
                sithPuppet_StopForceMove(pThing, 1);
            }

            sithPlayerActions_ActorGrabLedge(pThing, curLen, pHitSurf, pFace, pHitMesh, pHitObject);
            sithPlayerControls_bJumpActivated = 1;
            sithPhysics_ResetThingMovement(pThing);

            if ( pThing == sithPlayer_pLocalPlayerThing && (pThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0 )
            {
                rdKeyframe* pKeyframe = NULL;
                pKeyframe = sithPuppet_GetKeyframe("in_jump_up_grab.key");
                if ( pKeyframe && sithPuppet_PlayKey(pThing->renderData.pPuppet, pKeyframe, 3, 5, RDKEYFRAME_RESTART_ACTIVE | RDKEYFRAME_FADEOUT_NOLOOP, NULL) >= 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_HITHARD);
                }
            }

            return;
        }
    }

    if ( (pPhysics->flags & SITH_PF_FLY) == 0 )
    {
        memset(&pPhysics->thrust, 0, sizeof(rdVector3));
        memset(&pPhysics->angularVelocity, 0, sizeof(rdVector3));
        return;
    }

    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActor->maxThrust + pActor->extraSpeed, thrustFactor), 0.9f);
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        thrustFactor = -1.0f;
        pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActor->maxThrust + pActor->extraSpeed, thrustFactor), 0.5f);
    }
    else
    {
        pPhysics->thrust.y = 0.0f;
    }

    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        pPhysics->angularVelocity.y = RD_MULTIPLY(-1.0f, sithTime_fps);
        pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActor->maxRotVelocity, -1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
        pPhysics->angularVelocity.y /= 2.0f;
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        pPhysics->angularVelocity.y = RD_MULTIPLY(1.0f, sithTime_fps);
        pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActor->maxRotVelocity, 1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
        pPhysics->angularVelocity.y /= 2.0f;
    }
    else
    {
        pPhysics->angularVelocity.y = 0.0f;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        float mouseTurn;
        mouseTurn = RD_MULTIPLY(sithTime_fps, sithControl_GetAxis(SITHCONTROL_MOUSETURN));
        mouseTurn = D3DMultiply(RD_MULTIPLY(sithControl_GetKeyAsAxis(SITHCONTROL_MOUSETURN), pActor->maxRotVelocity), J3DMIN(moveScale, 1.0f)) + mouseTurn;
        pPhysics->angularVelocity.y += mouseTurn;
    }
    else if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
    {
        pPhysics->thrust.z += RD_MULTIPLY(pActor->maxThrust, 0.5f);
    }
    else
    {
        pPhysics->thrust.z = 0.0f;
    }

    if ( (pPhysics->thrust.x != 0.0f || pPhysics->thrust.y != 0.0f || pPhysics->thrust.z != 0.0f)
        && (pActor->headPYR.x != 0.0f || pActor->headPYR.y != 0.0f || pActor->headPYR.z != 0.0f) )
    {
        rdMatrix34 orient;
        rdMatrix_BuildRotate34(&orient, &pActor->headPYR);
        rdMatrix_TransformVector34Acc(&pPhysics->thrust, &orient);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPlayerControls_countfit_3, 6) /* end count-fit */
void J3DAPI sithPlayerControls_ProcessSwimMove(SithThing* pThing, float secDeltaTime)
{
    float moveScale = 1.0f;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    int bFlying = 0;
    int unusedFlag = 0;  // unreferenced; the debug build's frame keeps its slot
    float thrustScale = 0.0f;
    float pitchRate;
    float yawRate;
    float rotAngle;
    rdVector3 pyr;
    rdVector3 angles;
    int state;
    int bFoundFloorItem;
    SithThing* pActivatedThing;
    rdMatrix34 orient;

    sithPlayerControls_curJumpDirection = 0;
    if ( sithPlayerControls_secSwimmingTime < 1.0f )
    {
        pPhysics->thrust.x = 0.0f;
        memset(&pPhysics->angularVelocity, 0, sizeof(rdVector3));
        memset(&pyr, 0, sizeof(rdVector3));
        sithPhysics_SetThingLook(pThing, &rdroid_zVector3, 0.0f);
        return;
    }

    if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM && (pThing->pInSector->flags & (SITH_SECTOR_AETHERIUM | SITH_SECTOR_UNDERWATER)) != 0 )
    {
        sithPuppet_RemoveAllTracks(pThing);
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
    }

    pitchRate = 60.0f;
    yawRate   = 75.0f;
    rdVector_Set3(&pyr, 0.0f, 0.0f, 0.0f);

    if ( (pThing->flags & SITH_TF_SUBMERGED) == 0 && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        sithPhysics_FindFloor(pThing, 0);
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
        return;
    }

    if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 && pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    if ( sithControl_GetKey(SITHCONTROL_ACT2, NULL) && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 && !sithWeapon_IsMountingWeapon(pThing) )
    {
        pActivatedThing = sithCollision_FindActivatedThing(pThing, &bFoundFloorItem);
        if ( pActivatedThing )
        {
            pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
            sithPlayerControls_pCurActivatedThing = pActivatedThing;
            sithCog_ThingSendMessage(sithPlayerControls_pCurActivatedThing, pThing, SITHCOG_MSG_ACTIVATE);
            sithPhysics_ResetThingMovement(pThing);
            sithPlayerControls_curMoveStatus              = pThing->moveStatus;
            pThing->moveStatus                            = SITHPLAYERMOVE_ACTIVATING;
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_PICKUP, sithPlayerControls_PuppetCallback);
            return;
        }

        if ( !pThing->thingInfo.actorInfo.bForceMovePlay && sithPlayerActions_Activate(pThing) == 1 )
        {
            return;
        }
    }

    if ( sithControl_GetKey(SITHCONTROL_JUMP, NULL) )
    {
        if ( !sithInventory_GetCurrentWeapon(pThing) && !sithWeapon_IsMountingWeapon(pThing) && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 && !sithPlayerControls_bJumpActivated )
        {
            bFoundFloorItem = sithPlayerControls_CheckWaterLedge(pThing);
            if ( bFoundFloorItem == 1 )
            {
                sithFX_CreateWaterRipple(pThing);
                sithPhysics_ResetThingMovement(pThing);
                pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
                pThing->collide.movesize /= 4.0f;
                rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNTFROMWATER, NULL);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATER);
                sithFX_CreateWaterRipple(pThing);
                return;
            }

            if ( bFoundFloorItem == 2 )
            {
                sithFX_CreateWaterRipple(pThing);
                sithPhysics_ResetThingMovement(pThing);
                pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
                pThing->collide.movesize /= 4.0f;
                pThing->pos.z             = pThing->pos.z - 0.005f;
                rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNT1MSTEP, NULL);
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATER);
                sithFX_CreateWaterRipple(pThing);
                return;
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
        {
            thrustScale = 2.0f;
            pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, thrustScale), 0.9f);
            if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                sithPhysics_FindWaterSurface(pThing);
            }

            sithPlayerControls_bJumpActivated = 1;
        }
    }
    else
    {
        pPhysics->thrust.y = 0.0f;
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
    {
        pyr.z = 1.0f;
        if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
        {
            sithPhysics_SetThingLook(pThing, &pyr, 0.0f);
        }

        if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &state) )
        {
            sithPhysics_ResetThingMovement(pThing);
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_FIRE2, 0.0f);
            sithFX_CreateWaterRipple(pThing);
            rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_DIVEFROMSURFACE, NULL);
            if ( sithPlayerControls_secCommentWait == 0.0f )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATERSLOW);
                sithPlayerControls_secCommentWait = 3.0f;
            }

            sithFX_CreateWaterRipple(pThing);
            return;
        }

        if ( sithControl_GetKey(SITHCONTROL_CRAWLTOGGLE, &state) )
        {
            pPhysics->thrust.z -= RD_MULTIPLY(pActorInfo->maxThrust, 0.5f);
        }
    }
    else
    {
        pPhysics->thrust.z = 0.0f;
    }

    if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, 1.0f), 0.9f);
        }
        else
        {
            rdMatrix_ExtractAngles34(&pThing->orient, &angles);
            rotAngle = RD_MULTIPLY(-pitchRate, sithTime_frameTimeFlex);
            pyr.x    = rotAngle;
            if ( fabs(pyr.x + angles.x) > 75.0 )
            {
                return;
            }

            rdMatrix_PreRotate34(&pThing->orient, &pyr);
            return;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_BACK, NULL) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pPhysics->thrust.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxThrust + pActorInfo->extraSpeed, -1.0f), 0.9f);
        }
        else
        {
            rdMatrix_ExtractAngles34(&pThing->orient, &angles);
            rotAngle = RD_MULTIPLY(pitchRate, sithTime_frameTimeFlex);
            pyr.x    = rotAngle;
            if ( fabs(pyr.x + angles.x) > 75.0 )
            {
                return;
            }

            rdMatrix_PreRotate34(&pThing->orient, &pyr);
            return;
        }
    }
    else
    {
        pPhysics->angularVelocity.x = 0.0f;
    }

    if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(-1.0f, sithTime_fps);
            pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, -1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
        }
        else
        {
            rotAngle = RD_MULTIPLY(-yawRate, sithTime_frameTimeFlex);
            pyr.y    = rotAngle;
            rdMatrix_PostRotate34(&pThing->orient, &pyr);
            return;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
    {
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pPhysics->angularVelocity.y = RD_MULTIPLY(1.0f, sithTime_fps);
            pPhysics->angularVelocity.y = D3DMultiply(RD_MULTIPLY(pActorInfo->maxRotVelocity, 1.0f), J3DMIN(moveScale, 1.0f)) + pPhysics->angularVelocity.y;
        }
        else
        {
            rotAngle = RD_MULTIPLY(yawRate, sithTime_frameTimeFlex);
            pyr.y    = rotAngle;
            rdMatrix_PostRotate34(&pThing->orient, &pyr);
            return;
        }
    }
    else
    {
        pPhysics->angularVelocity.y = 0.0f;
    }

    if ( (pPhysics->thrust.x != 0.0f || pPhysics->thrust.y != 0.0f || pPhysics->thrust.z != 0.0f)
        && (pActorInfo->headPYR.x != 0.0f || pActorInfo->headPYR.y != 0.0f || pActorInfo->headPYR.z != 0.0f)
        && bFlying
        && (pPhysics->flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        rdMatrix_BuildRotate34(&orient, &pActorInfo->headPYR);
        rdMatrix_TransformVector34Acc(&pPhysics->thrust, &orient);
    }
}

void J3DAPI sithPlayerControls_ProcessHangMove(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* pActor = &pThing->thingInfo.actorInfo;  // unreferenced; the debug build's frame keeps its slot
    int unusedBool = 0;  // unreferenced; the debug build's frame keeps its slot
    float moveScale = 1.0f;  // unreferenced; the debug build's frame keeps its slot
    SithSurface* pWall;
    rdModel3* pThingModel;
    SithThing* pHit;
    rdVector3 normMove;
    rdFace* pPoly;
    rdModel3Mesh* pCurHitMesh;

    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        return;
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        if ( sithControl_GetKey(SITHCONTROL_FORWARD, NULL) && !sithPlayerControls_bJumpActivated )
        {
            if ( !sithPlayerActions_CanPullUp(pThing) )
            {
                return;
            }

            pThing->collide.movesize  = 0.001f;
            pThing->collide.type      = SITH_COLLIDE_NONE;
            rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
            pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
            pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
            pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
            sithThing_DetachThing(pThing);
            sithPuppet_RemoveAllTracks(pThing);
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNTLEDGE, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
            return;
        }

        if ( (sithControl_GetKey(SITHCONTROL_BACK, NULL) || sithControl_GetKey(SITHCONTROL_JUMP, NULL)) && !sithPlayerControls_bJumpActivated )
        {
            sithPlayerActions_UnmountWall(pThing, -1);
            sithPlayerControls_bJumpActivated = 1;
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, NULL) )
        {
            rdVector_Set3(&normMove, 1.0f, 0.0f, 0.0f);
            rdMatrix_TransformVector34Acc(&normMove, &pThing->orient);
            rdVector_Normalize3Acc(&normMove);

            pWall  = NULL;
            pThingModel = NULL;
            pPoly  = NULL;
            pCurHitMesh  = NULL;
            pHit = NULL;
            if ( sithPlayerControls_SearchForCollisions(pThing, &normMove, &pWall, &pHit, &pThingModel, &pPoly, &pCurHitMesh, 3) )
            {
                if ( pWall )
                {
                    sithPuppet_RemoveAllTracks(pThing);
                    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                    pThing->collide.movesize = 0.0f;
                    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_HANGSHIMRIGHT, NULL);
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);
                    sithThing_AttachThingToClimbSurface(pThing, pWall, 0);
                    sithPlayerActions_pCurCollidedSurface = pWall;
                }
                else
                {
                    sithPuppet_RemoveAllTracks(pThing);
                    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                    pThing->collide.movesize = 0.0f;
                    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_HANGSHIMRIGHT, NULL);
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);
                    sithThing_AttachThingToThingFace(pThing, pHit, pPoly, pCurHitMesh->apVertices, 1);
                    sithPlayerActions_pCurCollidedModelFace = pPoly;
                    sithPlayerActions_pCurCollidedModel     = pThingModel;
                }
            }
        }
        else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, NULL) )
        {
            rdVector_Set3(&normMove, -1.0f, 0.0f, 0.0f);
            rdMatrix_TransformVector34Acc(&normMove, &pThing->orient);
            rdVector_Normalize3Acc(&normMove);

            pWall = NULL;
            if ( sithPlayerControls_SearchForCollisions(pThing, &normMove, &pWall, &pHit, &pThingModel, &pPoly, &pCurHitMesh, 4) )
            {
                if ( pWall )
                {
                    sithPuppet_RemoveAllTracks(pThing);
                    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                    pThing->collide.movesize = 0.0f;
                    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_HANGSHIMLEFT, NULL);
                    sithThing_AttachThingToClimbSurface(pThing, pWall, 0);
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);
                    sithPlayerActions_pCurCollidedSurface = pWall;
                }
                else
                {
                    sithPuppet_RemoveAllTracks(pThing);
                    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                    pThing->collide.movesize = 0.0f;
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);
                    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_HANGSHIMLEFT, NULL);
                    sithThing_AttachThingToThingFace(pThing, pHit, pPoly, pCurHitMesh->apVertices, 1);
                    sithPlayerActions_pCurCollidedModelFace = pPoly;
                    sithPlayerActions_pCurCollidedModel     = pThingModel;
                }
            }
        }
        else
        {
            pPhysics->thrust.x = 0.0f;
        }
    }
}

void J3DAPI sithPlayerControls_ProcessWeaponAim(SithThing* pThing, float secDeltaTime)
{
    int bHeadMoved = 0;
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    int bAimable = 0;
    SithThing* pTarget;
    int weaponId;
    SithInventoryType* pWeaponType;
    SithSector* pSector;
    float sinAngle;
    float cosMaxAngle;
    float cosMinAngle;
    float cosMaxAimAngle;
    float cosMinAimAngle;
    float cosAimAngle;
    float maxAimDist;
    float aimCosMaxAngle;
    float aimCosAngle;
    float aimDist;
    float axis;
    rdVector3 startPos;
    rdVector3 headPYR;

#line 3672
    SITH_ASSERTREL(pThing);

    if ( (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) || secDeltaTime == 0.0f )
    {
        return;
    }

    weaponId = sithInventory_GetCurrentWeapon(pThing);
    if ( sithInventory_GetCurrentWeapon(pThing) != SITHWEAPON_MIRROR )
    {
        stdMath_SinCos(sithPlayerControls_flt_528CF0, &sinAngle, &cosMaxAngle);
        stdMath_SinCos(sithPlayerControls_flt_528CF4, &sinAngle, &cosMinAngle);
        stdMath_SinCos(sithPlayerControls_flt_528CF8, &sinAngle, &cosMaxAimAngle);
        stdMath_SinCos(sithPlayerControls_flt_528CFC, &sinAngle, &cosMinAimAngle);
        stdMath_SinCos(sithPlayerControls_flt_528D00, &sinAngle, &cosAimAngle);
    }
    else
    {
        stdMath_SinCos(sithPlayerControls_flt_528D04, &sinAngle, &cosMaxAngle);
        stdMath_SinCos(sithPlayerControls_flt_528D08, &sinAngle, &cosMinAngle);
        stdMath_SinCos(sithPlayerControls_flt_528D0C, &sinAngle, &cosMaxAimAngle);
        stdMath_SinCos(sithPlayerControls_flt_528D10, &sinAngle, &cosMinAimAngle);
        stdMath_SinCos(sithPlayerControls_flt_528D14, &sinAngle, &cosAimAngle);
    }

    if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
    {
        pWeaponType = sithInventory_GetType(weaponId);
        pTarget     = pThing->controlInfo.aiControl.pLocal->pTargetThing;
    }
    else if ( pThing->type == SITH_THING_PLAYER )
    {
        pWeaponType = sithInventory_GetInventoryType(pThing, weaponId);
        pTarget     = sithPlayerControls_pTargetThing;
        if ( sithWeapon_GetMountWait() > sithTime_secGameTime )
        {
            sithPlayerControls_pTargetThing = NULL;
            sithPlayerControls_RotateAimJoints(pThing, 0.0f, 0.0f);
            sithWeapon_SendMessageAim(pThing, 0);
            return;
        }

        switch ( pThing->moveStatus )
        {
            case SITHPLAYERMOVE_JUMPROLLBACK:
            case SITHPLAYERMOVE_JUMPROLLFWD:
            case SITHPLAYERMOVE_JUMPLEFT:
            case SITHPLAYERMOVE_JUMPRIGHT:
                pTarget = sithPlayerControls_pTargetThing = NULL;
                sithPlayerControls_RotateAimJoints(pThing, 0.0f, 0.0f);
                sithWeapon_SendMessageAim(pThing, 0);
                return;
        }
    }

    rdVector_Copy3(&startPos, &pThing->pos);
    startPos.z += 0.05f;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        pTarget = NULL;
        if ( pThing->type == SITH_THING_PLAYER )
        {
            sithPlayerControls_pTargetThing = NULL;
        }

        sithPlayerControls_RotateAimJoints(pThing, 0.0f, 0.0f);
        sithWeapon_SendMessageAim(pThing, 0);
        return;
    }

    if ( pWeaponType && (pWeaponType->flags & SITHINVENTORY_TYPE_AUTOAIM) != 0 )
    {
        maxAimDist = sithWeapon_GetWeaponMaxAimDistance(weaponId);
        aimDist    = maxAimDist + 0.1f;
        if ( pTarget )
        {
            if ( pTarget->type != SITH_THING_FREE && (pTarget->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 && (pTarget->thingInfo.actorInfo.flags & SITH_AF_NOTARGET) == 0 )
            {
                aimCosMaxAngle = cosMinAngle;
                aimCosAngle    = cosMinAimAngle;
                if ( pThing->controlInfo.aiControl.pLocal && !sithWeapon_IsMountingWeapon(pThing) && !sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_AIMWEAPON) )
                {
                    aimDist        = maxAimDist;
                    aimCosMaxAngle = cosMaxAngle;
                    aimCosAngle    = cosMaxAimAngle;
                }

                if ( sithPlayerControls_CanAimAtPos(pThing, &startPos, &pTarget->pos, aimCosMaxAngle, cosAimAngle, aimCosAngle, aimDist) )
                {
                    if ( pThing->type == SITH_THING_PLAYER )
                    {
                        bAimable = sithCollision_CanMoveToPos(pSector, &startPos, &pTarget->pos, 0.0f);
                    }
                    else if ( pThing->controlInfo.aiControl.pLocal )
                    {
                        bAimable = pThing->controlInfo.aiControl.pLocal->mode & SITHAI_MODE_TARGETVISIBLE;
                    }
                }
            }

            if ( !bAimable )
            {
                pTarget = NULL;
                if ( pThing->type == SITH_THING_PLAYER )
                {
                    sithPlayerControls_pTargetThing = NULL;
                }
            }
        }

        if ( !pTarget )
        {
            if ( pThing->type == SITH_THING_PLAYER )
            {
                int searchFlags = 0x284;
                float searchRadius = 1.0f;
                float searchDist;
                SithCollision* pCollision;

                if ( ((pThing->idx + sithMain_frameNumber) & 1) != 0 )
                {
                    return;
                }

                searchDist = maxAimDist;
                sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &pThing->orient.lvec, searchDist, searchRadius, searchFlags);
                while ( (pCollision = sithCollision_PopStack()) != NULL )
                {
                    if ( (pCollision->type & SITHCOLLISION_THING) != 0
                        && pCollision->pThingCollided->type == SITH_THING_ACTOR
                        && (pCollision->pThingCollided->flags & (SITH_TF_WHIPSWING | SITH_TF_WHIPCLIMB)) == 0
                        && sithInventory_GetCurrentWeapon(pThing) != SITHWEAPON_WHIP
                        && pCollision->pThingCollided->type != SITH_THING_FREE
                        && (pCollision->pThingCollided->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
                        && (pCollision->pThingCollided->thingInfo.actorInfo.flags & SITH_AF_NOTARGET) == 0
                        && sithPlayerControls_CanAimAtPos(pThing, &startPos, &pCollision->pThingCollided->pos, cosMaxAngle, cosAimAngle, cosMaxAimAngle, maxAimDist)
                        && sithCollision_CanMoveToPos(pSector, &startPos, &pCollision->pThingCollided->pos, 0.0f)
                        && (sithInventory_GetCurrentWeapon(pThing) != SITHWEAPON_MIRROR || !stdUtil_StrCmp(pCollision->pThingCollided->aName, "marduk")) )
                    {
                        pTarget = pCollision->pThingCollided;
                        if ( pThing->type == SITH_THING_PLAYER )
                        {
                            sithPlayerControls_pTargetThing = pTarget;
                        }

                        break;
                    }
                }

                sithCollision_DecreaseStackLevel();
            }

        }

        if ( pTarget )
        {
            float eyeZ;
            rdVector3 targetPos;
            rdMatrix34 aimOrient;
            rdVector3 thingPYR;
            rdVector3 aimPYR;
            rdVector_Copy3(&targetPos, &pTarget->pos);
            eyeZ = pTarget->thingInfo.actorInfo.eyeOffset.z;
            if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_HUMAN) != 0 )
            {
                eyeZ *= 0.25f;
            }

            targetPos.z = targetPos.z + eyeZ;
            rdMatrix_LookAt(&aimOrient, &startPos, &targetPos, 0.0f);
            rdMatrix_ExtractAngles34(&pThing->orient, &thingPYR);
            rdMatrix_ExtractAngles34(&aimOrient, &aimPYR);
            aimPYR.x = stdMath_NormalizeAngleAcute(thingPYR.x - aimPYR.x);
            aimPYR.y = stdMath_NormalizeAngleAcute(thingPYR.y - aimPYR.y);
            aimPYR.x = STDMATH_CLAMP(aimPYR.x, -80.0f, 80.0f);
            aimPYR.y = STDMATH_CLAMP(aimPYR.y, -45.0f, 45.0f);
            sithPlayerControls_RotateAimJoints(pThing, -aimPYR.x, -aimPYR.y);
            sithWeapon_SendMessageAim(pThing, 1);
        }
        else
        {
            if ( pThing->type == SITH_THING_PLAYER )
            {
                sithPlayerControls_pTargetThing = NULL;
            }

            sithPlayerControls_RotateAimJoints(pThing, 0.0f, 0.0f);
            sithWeapon_SendMessageAim(pThing, 0);
        }

        return;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 && sithCamera_pCurCamera->type != SITHCAMERA_UNKNOWN_100 )
    {
        sithPlayerControls_RotateAimJoints(pThing, 0.0f, 0.0f);
    }

    pTarget = NULL;
    if ( pThing->type == SITH_THING_PLAYER )
    {
        sithPlayerControls_pTargetThing = NULL;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 && (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
        if ( sithControl_GetKey(SITHCONTROL_CENTER, NULL) )
        {
            sithPhysics_SetThingLook(pThing, &rdroid_zVector3, secDeltaTime);
        }

        return;
    }

    rdVector_Copy3(&headPYR, &pActorInfo->headPYR);
    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        axis = sithControl_GetAxis(SITHCONTROL_PITCH);
        if ( axis != 0.0f )
        {
            bHeadMoved = 1;
            headPYR.x += axis;
        }

        axis = sithControl_GetKeyAsAxis(SITHCONTROL_PITCH);
        if ( axis != 0.0f )
        {
            bHeadMoved = 1;
            headPYR.x += D3DMultiply(RD_MULTIPLY(axis, 90.0f), secDeltaTime);
        }

        if ( bHeadMoved )
        {
            headPYR.x = STDMATH_CLAMP(headPYR.x, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);

            sithActor_SetHeadPYR(pThing, &headPYR);
            pActorInfo->flags &= ~SITH_AF_VIEWCENTRING;
            return;
        }

        if ( sithControl_GetKey(SITHCONTROL_CENTER, NULL) || (pActorInfo->flags & SITH_AF_VIEWCENTRING) != 0 )
        {
            float centerStep;
            float centerRate;
            pActorInfo->flags |= SITH_AF_VIEWCENTRING;
            centerStep = -pActorInfo->headPYR.x;
            centerRate = RD_MULTIPLY(180.0f, secDeltaTime);
            centerStep = STDMATH_CLAMP(centerStep, -centerRate, centerRate);
            centerStep = stdMath_ClipNearZero(centerStep);

            if ( centerStep != 0.0f )
            {
                pActorInfo->headPYR.x += centerStep;
                sithActor_SetHeadPYR(pThing, &pActorInfo->headPYR);
            }
            else
            {
                pActorInfo->flags &= ~SITH_AF_VIEWCENTRING;
                pActorInfo->flags |= SITH_AF_VIEWCENTRED;
            }
        }
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPlayerControls_countfit_4, 30) CF_PROTO(sithPlayerControls_countfit_5, 28) /* end count-fit */
int J3DAPI sithPlayerControls_CanAimAtPos(SithThing* pThing, const rdVector3* pStartPos, const rdVector3* pAimPos, float cosMaxAngle, float cosMinAngle, float cosAimAngle, float maxDist)
{
    float dist;
    float planeDistance;
    rdVector3 lookDir;
    rdVector3 dir;
    float curMinimumAngle;
    float curAimAngle;
    float ratio;
    float lvecDot;
    float vecDot;

    rdVector_Sub3(&dir, pAimPos, pStartPos);
    dist  = rdVector_Normalize3Acc(&dir);
    if ( dist < maxDist )
    {
        if ( dist > sithPlayerControls_aimInterpDistance )
        {
            curMinimumAngle = cosMaxAngle;
            curAimAngle = cosAimAngle;
        }
        else
        {
            ratio       = (cosMaxAngle - cosMinAngle) / sithPlayerControls_aimInterpDistance;
            curMinimumAngle = RD_MULTIPLY(dist, ratio) + cosMinAngle;
            curAimAngle = cosAimAngle;
        }

        planeDistance = rdMath_DistancePointToPlane(&dir, &pThing->orient.uvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&lookDir, &pThing->orient.uvec, -planeDistance, &dir);
        rdVector_Normalize3Acc(&lookDir);

        lvecDot = rdVector_Dot3(&pThing->orient.lvec, &lookDir);
        if ( lvecDot >= curMinimumAngle )
        {
            vecDot = rdVector_Dot3(&dir, &lookDir);
            if ( vecDot >= curAimAngle )
            {
                return 1;
            }
        }
    }

    return 0;
}

int J3DAPI sithPlayerControls_GetPushPullMoveNorm(rdVector3* moveNorm, const rdVector3* pLVect, float angle)
{
    rdVector3 curlvec;
    float sinAngle;
    float cosAngle;
    rdVector3 curdir;
    float curdot;

    rdVector_Copy3(&curlvec, pLVect);
    curlvec.z = 0.0f;
    stdMath_SinCos(angle, &sinAngle, &cosAngle);
    if ( pLVect->x == 0.0f && pLVect->y == 0.0f )
    {
        return 0;
    }

    if ( fabs(pLVect->x) > fabs(pLVect->y) )
    {
        curdir.x = 1.0f;
        curdir.y = curdir.z = 0.0f;
        curdot = (rdVector_Dot3)(pLVect, &curdir);
        if ( curdot > 0.0f )
        {
            if ( curdot > cosAngle )
            {
                rdVector_Copy3(moveNorm, &curdir);
                return 1;
            }

            return 0;
        }

        if ( curdot < 0.0f )
        {
            if ( curdot < -cosAngle )
            {
                rdVector_Copy3(moveNorm, &curdir);
                rdVector_Neg3Acc(moveNorm);
                return 1;
            }

            return 0;
        }

        return 0;
    }
    else
    {
        curdir.y = 1.0f;
        curdir.x = curdir.z = 0.0f;
        curdot = (rdVector_Dot3)(pLVect, &curdir);
        if ( curdot > 0.0f )
        {
            if ( curdot > cosAngle )
            {
                rdVector_Copy3(moveNorm, &curdir);
                return 1;
            }

            return 0;
        }

        if ( curdot < 0.0f )
        {
            if ( curdot < -cosAngle )
            {
                rdVector_Copy3(moveNorm, &curdir);
                rdVector_Neg3Acc(moveNorm);
                return 1;
            }

            return 0;
        }

        return 0;
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPlayerControls_countfit_6, 6) /* end count-fit */
int J3DAPI sithPlayerControls_CheckWaterLedge(SithThing* pThing)
{
    int ledgeType;
    rdVector3 pos;
    rdVector3 lvec;
    SithSector* pSector;
    SithCollision* pCollision;
    float dot;
    float searchDist;
    float searchRadius;
    rdVector3 dir;
    int searchFlags;

    ledgeType = 0;
    if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        return 0;
    }

    rdVector_Copy3(&pos, &pThing->pos);
    rdVector_Copy3(&lvec, &pThing->orient.lvec);
    rdVector_Neg3(&dir, &lvec);
    if ( lvec.z != 0.0f )
    {
        return 0;
    }

    searchRadius = 0.051f;
    searchDist   = 0.01f;
    searchFlags  = 0x801;
    pos.z += 0.02f;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &pos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, pThing, &pos, &lvec, searchDist, searchRadius, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATERLEDGE) != 0
            && (dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &dir)) > 0.87f )
        {
            rdFace* pFace = &pCollision->pSurfaceCollided->face;
            float maxZ;
            size_t i;
            rdVector3 newPos;
            SithSector* pNewSector;

            maxZ = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]].z;
            for ( i = 0; i < pFace->numVertices; ++i )
            {
                float vertexZ = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].z;
                if ( vertexZ > maxZ )
                {
                    maxZ = vertexZ;
                }
            }

            rdVector_Copy3(&newPos, &pThing->pos);
            newPos.z = maxZ;
            newPos.z -= 0.095f;
            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);
            if ( !pNewSector )
            {
                return 0;
            }

            rdVector_Copy3(&pThing->pos, &newPos);
            if ( pNewSector != pThing->pInSector )
            {
                sithThing_ExitSector(pThing);
                sithThing_EnterSector(pThing, pNewSector, 1, 0);
            }

            ledgeType = 1;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( !ledgeType )
    {
        searchRadius = 0.05f;
        searchDist   = 0.1f;
        rdVector_Copy3(&pos, &pThing->pos);
        pos.z += 0.02f;
        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &pos, 0.0f);
        if ( !pSector )
        {
            ledgeType = 0;
        }
        else
        {
            sithCollision_SearchForCollisions(pSector, pThing, &pos, &lvec, searchDist, searchRadius, searchFlags);
            while ( (pCollision = sithCollision_PopStack()) != NULL )
            {
                if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
                    && pCollision->pSurfaceCollided
                    && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATERLEDGE) != 0
                    && (dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &dir)) > 0.87f )
                {
                    ledgeType = 2;
                    break;
                }
            }

            sithCollision_DecreaseStackLevel();
        }
    }

    return ledgeType;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(sithPlayerControls_countfit_7, 30, 7) CF_PROTO(sithPlayerControls_countfit_14, 21) /* end count-fit */
int J3DAPI sithPlayerControls_SearchForCollisions(SithThing* pThing, rdVector3* moveNorm, SithSurface** ppHitSurf, SithThing** ppHitThing, rdModel3** ppHitModel, rdFace** ppHitFace, rdModel3Mesh** ppHitMesh, int someType)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;  // unreferenced; the debug build's frame keeps its slot
    SithActorInfo* pInfo = &pThing->thingInfo.actorInfo;  // unreferenced; the debug build's frame keeps its slot
    float walkLen;
    rdVector3 curEndPos;
    SithSurface* pImpactSurf;
    rdVector3 offsetVec;
    rdVector3 normalVec;
    SithCollision* pCollision;
    SithSector* pLastSector;
    int bWasHit;
    int bBlocked;
    float dot;
    int searchFlags;

    pImpactSurf = NULL;
    if ( someType == 1 || someType == 2 )
    {
        walkLen = 0.2f;
    }
    else
    {
        walkLen = 0.1f;
    }

    rdVector_Scale3(&offsetVec, moveNorm, walkLen);
    rdVector_Add3(&curEndPos, &pThing->pos, &offsetVec);
    curEndPos.z += 0.12f;

    pLastSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &curEndPos, 0.0f);
    if ( !pLastSector )
    {
        return 0;
    }

    bWasHit      = 0;
    searchFlags = 0xA00;
    sithCollision_SearchForCollisions(pLastSector, pThing, &curEndPos, &pThing->orient.lvec, 0.1f, 0.005f, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 )
            {
                rdVector_Copy3(&normalVec, &pCollision->pSurfaceCollided->face.normal);
                dot = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &normalVec);
                if ( dot < 0.9f )
                {
                    continue;
                }

                pImpactSurf = pCollision->pSurfaceCollided;
                bWasHit   = 1;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0
            && (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0
            && pCollision->pFaceCollided
            && pCollision->pThingCollided == pThing->attach.attachedToStructure.pThingAttached
            && (pCollision->pFaceCollided->flags & RD_FF_3DO_LEDGE) != 0 )
        {
            pImpactSurf    = NULL;
            bWasHit      = 1;
            *ppHitModel = pCollision->pThingCollided->renderData.data.pModel3;
            *ppHitFace  = pCollision->pFaceCollided;
            *ppHitMesh  = pCollision->pMeshCollided;
            *ppHitThing = pCollision->pThingCollided;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bWasHit || someType == 2 )
    {
        bBlocked    = 0;
        searchFlags = 0xA00;
        sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, moveNorm, walkLen, D3DVAL(pThing->collide.movesize / 2.0f), searchFlags);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( someType == 2 && pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
                {
                    sithPlayerActions_UnmountWall(pThing, -1);
                    sithCollision_DecreaseStackLevel();
                    return 0;
                }

                if ( pCollision->pSurfaceCollided != pImpactSurf )
                {
                    bBlocked = 1;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                if ( pCollision->pThingCollided )
                {
                    if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
                    {
                        if ( pCollision->pThingCollided != pThing->attach.attachedToStructure.pThingAttached )
                        {
                            bBlocked = 1;
                            break;
                        }
                    }
                    else
                    {
                        bBlocked = 1;
                        break;
                    }
                }
            }
            else
            {
                continue;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( bBlocked )
        {
            *ppHitSurf  = NULL;
            *ppHitFace  = NULL;
            *ppHitModel = NULL;
            *ppHitMesh  = NULL;
            return 0;
        }
    }

    *ppHitSurf = pImpactSurf;
    return bWasHit;
}

SithSurface* J3DAPI sithPlayerControls_SearchForClimbSurface(SithThing* pThing, const rdVector3* moveNorm, int climbDir, int* pbHitNonClimbSurface)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;  // unreferenced; the debug build's frame keeps its slot
    SithActorInfo* pInfo = &pThing->thingInfo.actorInfo;  // unreferenced; the debug build's frame keeps its slot
    SithSurface* pClimbSurface;
    float walkLen;
    rdVector3 curEndPos;
    rdVector3 offsetVec;
    rdVector3 normalVec;
    SithCollision* pCollision;
    int bBlocked;
    float dot;
    SithSector* pOldSector;
    int searchFlags;

    pClimbSurface = NULL;
    if ( climbDir == 1 || climbDir == 2 )
    {
        walkLen = 0.2f;
    }
    else
    {
        walkLen = 0.1f;
    }

    *pbHitNonClimbSurface = 0;

    rdVector_Scale3(&offsetVec, moveNorm, walkLen);
    rdVector_Add3(&curEndPos, &pThing->pos, &offsetVec);

    pOldSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &curEndPos, 0.0f);
    if ( !pOldSector )
    {
        return NULL;
    }

    searchFlags = 1;
    sithCollision_SearchForCollisions(pOldSector, pThing, &curEndPos, &pThing->orient.lvec, pThing->collide.movesize + 0.01f, pThing->collide.movesize, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
            {
                rdVector_Copy3(&normalVec, &pCollision->pSurfaceCollided->face.normal);
                dot                   = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &normalVec);
                pClimbSurface            = pCollision->pSurfaceCollided;
                *pbHitNonClimbSurface = 0;
                break;
            }
            else
            {
                *pbHitNonClimbSurface = 1;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
            {
                rdVector_Copy3(&normalVec, &pCollision->pSurfaceCollided->face.normal);
                dot                   = rdVector_Dot3(&pThing->attach.attachedToStructure.pSurfaceAttached->face.normal, &normalVec);
                pClimbSurface            = pCollision->pSurfaceCollided;
                *pbHitNonClimbSurface = 0;
                break;
            }
            else
            {
                if ( pCollision->pSurfaceCollided )
                {
                    if ( pCollision->pSurfaceCollided->pAdjoin )
                    {
                        if ( (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
                        {
                            *pbHitNonClimbSurface = 1;
                        }
                    }
                    else
                    {
                        *pbHitNonClimbSurface = 0;
                    }
                }
                else
                {
                    *pbHitNonClimbSurface = 0;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( pClimbSurface || climbDir == 2 )
    {
        bBlocked    = 0;
        searchFlags = 0xA00;
        sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, moveNorm, walkLen, 0.02f, searchFlags);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( climbDir == 2 && pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
                {
                    sithPlayerControls_bJumpActivated = 1;
                    sithPlayerActions_UnmountWall(pThing, -1);
                    sithCollision_DecreaseStackLevel();
                    *pbHitNonClimbSurface = 0;
                    return NULL;
                }

                if ( pCollision->pSurfaceCollided != pClimbSurface )
                {
                    bBlocked              = 1;
                    *pbHitNonClimbSurface = 1;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 || (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0 )
            {
                if ( (pCollision->pSurfaceCollided->flags & (SITH_SURFACE_WATER | SITH_SURFACE_LAVA)) != 0 )
                {
                    *pbHitNonClimbSurface = 1;
                    bBlocked              = 1;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                bBlocked              = 1;
                *pbHitNonClimbSurface = 1;
                break;
            }
            else
            {
                continue;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( bBlocked )
        {
            pClimbSurface = NULL;
        }
    }

    return pClimbSurface;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPlayerControls_countfit_16, 2) /* end count-fit */
int J3DAPI sithPlayerControls_TryBoardVehicle(SithThing* pThing, int bBoard)
{
    SithThing* pVehicle;
    int vehicleType;
    int bFoundPlayer;
    rdVector3 lvec;
    SithCollision* pCollision;
    SithThing* pPlayerThing;
    int searchFlags;

    vehicleType = 0;
    if ( sithInventory_GetCurrentWeapon(pThing) || sithWeapon_IsMountingWeapon(pThing) )
    {
        return 0;
    }

    searchFlags = 0xA00;
    pVehicle    = NULL;
    rdVector_Copy3(&lvec, &pThing->orient.lvec);
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &lvec, pThing->collide.movesize, pThing->collide.movesize, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            pVehicle = pCollision->pThingCollided;
            if ( pCollision->pThingCollided == pThing )
            {
                continue;
            }

            if ( !strcmp(pCollision->pThingCollided->aName, "jeep") || !strcmp(pCollision->pThingCollided->aName, "jeep_pyr") )
            {
                vehicleType = 1;
                break;
            }

            if ( !strcmp(pCollision->pThingCollided->aName, "minecar") )
            {
                vehicleType = 3;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( vehicleType )
    {
        bFoundPlayer = 0;
        switch ( vehicleType )
        {
            case 1:
            {
                SithThing* pActor = pThing;
                SithThing* pCar   = pVehicle;
                int playerNum;
                int bSectorChanged;
                rdVector3 checkPos;
                float dot;
                float planeDist;
                SithThing* pAttachedThing;
                SithAttachFlag attachFlags;
                char aModelName[84];
                rdModel3* pModel;
                size_t swapIdx;

                for ( playerNum = 0; playerNum < (int)sithPlayer_numPlayers; ++playerNum )
                {
                    if ( !strcmp(sithPlayer_aPlayers[playerNum].pThing->aName, "jeepplayer") || !strcmp(sithPlayer_aPlayers[playerNum].pThing->aName, "jeepplayerpyr") )
                    {
                        pPlayerThing = sithPlayer_aPlayers[playerNum].pThing;
                        bFoundPlayer = 1;
                        break;
                    }
                }

                if ( (dot = rdVector_Dot3(&pActor->orient.lvec, &pCar->orient.rvec)) < 0.707f )
                {
                    break;
                }

                rdVector_ScaleAdd3(&checkPos, &pCar->orient.lvec, -0.1f, &pCar->pos);
                if ( (planeDist = rdMath_DistancePointToPlane(&pActor->pos, &pCar->orient.lvec, &checkPos)) < 0.0f )
                {
                    break;
                }

                rdVector_ScaleAdd3(&checkPos, &pCar->orient.lvec, 0.07f, &pCar->pos);
                if ( (planeDist = rdMath_DistancePointToPlane(&pActor->pos, &pCar->orient.lvec, &checkPos)) > 0.0f )
                {
                    break;
                }

                if ( bFoundPlayer )
                {
                    sithPlayerControls_pVehicleBoardedThing = pCar;
                    sithCamera_RestoreExtCamera();
                    pThing->alpha = 1.0f;
                    if ( pActor->pInSector != pCar->pInSector )
                    {
                        bSectorChanged = 1;
                    }
                    else
                    {
                        bSectorChanged = 0;
                    }

                    if ( bSectorChanged )
                    {
                        sithCog_SectorSendMessage(pActor->pInSector, pActor, SITHCOG_MSG_EXITED);
                    }

                    if ( sithPlayer_bGuybrush )
                    {
                        sithPlayer_ToggleGuybrush();
                    }

                    sithPlayer_HidePlayer(sithPlayer_playerNum);
                    pCar->flags |= SITH_TF_DISABLED;
                    sithThing_ExitSector(pPlayerThing);

                    *pPlayerThing->thingInfo.actorInfo.pPlayer        = *pActor->thingInfo.actorInfo.pPlayer;
                    pPlayerThing->thingInfo.actorInfo.health          = pActor->thingInfo.actorInfo.health;
                    pPlayerThing->thingInfo.actorInfo.pPlayer->pThing = pPlayerThing;

                    sithPlayer_SetLocalPlayer(playerNum);
                    sithPlayer_ShowPlayer(playerNum, 0);
                    sithThing_SetPositionAndOrient(pPlayerThing, &pCar->pos, &pCar->orient);
                    sithThing_EnterSector(pPlayerThing, pCar->pInSector, 1, bSectorChanged == 0);

                    while ( pCar->pAttachedThing )
                    {
                        pAttachedThing = pCar->pAttachedThing;
                        attachFlags = pAttachedThing->attach.flags;
                        sithThing_DetachThing(pAttachedThing);
                        sithThing_AttachThingToThing(pAttachedThing, pPlayerThing);
                        pAttachedThing->attach.flags |= attachFlags & SITH_ATTACH_NOMOVE;
                    }

                    sithPuppet_SetMoveMode(pPlayerThing, SITHPUPPET_MOVEMODE_NORMAL);
                    sithPhysics_FindFloor(pPlayerThing, 1);
                    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], pPlayerThing, NULL);
                    pPlayerThing->moveStatus = SITHPLAYERMOVE_JEEP_BOARDING;
                    pPlayerThing->alpha      = 1.0f;
                    sithPuppet_PlayMode(pPlayerThing, SITHPUPPETSUBMODE_PUSHITEM, sithVehicleControls_PuppetCallback);
                    pPlayerThing->thingInfo.actorInfo.bControlsDisabled = 1;
                    sithInventory_SetSwimmingInventory(pPlayerThing, 0);
                    sithCog_ThingSendMessage(pPlayerThing, pActor, SITHCOG_MSG_BOARDED);
                    sithCog_ThingSendMessage(pCar, pPlayerThing, SITHCOG_MSG_BOARDED);

                    if ( (int)pCar->userval != -1 )
                    {
                        sprintf(aModelName, "gascan_%s", pCar->renderData.data.pModel3->aName);
                        pModel   = sithModel_GetModel(aModelName);
                        swapIdx  = sithThing_AddSwapEntry(pPlayerThing, sithThing_GetThingMeshIndex(pPlayerThing, "jeep_body"), pModel, sithModel_GetMeshIndex(pModel, "jeep_body"));
                        pPlayerThing->userval = (float)(size_t)swapIdx;
                    }

                    sithVehicleControls_SendMessageUser0(pPlayerThing);

                    if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                    {
                        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
                    }
                    else
                    {
                        pPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
                    }

                    if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
                    {
                        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
                    }
                    else
                    {
                        pPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
                    }

                    return 1;
                }

                break;
            }

            case 3:
            {
                SithThing* pActor = pThing;
                SithThing* pCar   = pVehicle;
                int playerNum;
                int bSectorChanged;
                rdVector3 checkPos;
                float dot;
                float planeDist;

                for ( playerNum = 0; playerNum < (int)sithPlayer_numPlayers; ++playerNum )
                {
                    if ( !strcmp(sithPlayer_aPlayers[playerNum].pThing->aName, "mineplayer") )
                    {
                        pPlayerThing = sithPlayer_aPlayers[playerNum].pThing;
                        bFoundPlayer = 1;
                        break;
                    }
                }

                dot = rdVector_Dot3(&pActor->orient.lvec, &pCar->orient.rvec);
                if ( (dot < 0.0f ? -dot : dot) < 0.707f )
                {
                    break;
                }

                rdVector_ScaleAdd3(&checkPos, &pCar->orient.lvec, -0.07f, &pCar->pos);
                if ( (planeDist = rdMath_DistancePointToPlane(&pActor->pos, &pCar->orient.lvec, &checkPos)) < 0.0f )
                {
                    break;
                }

                rdVector_ScaleAdd3(&checkPos, &pCar->orient.lvec, 0.07f, &pCar->pos);
                if ( (planeDist = rdMath_DistancePointToPlane(&pActor->pos, &pCar->orient.lvec, &checkPos)) > 0.0f )
                {
                    break;
                }

                if ( bFoundPlayer )
                {
                    sithPlayerControls_pVehicleBoardedThing = pCar;
                    sithCamera_RestoreExtCamera();
                    pThing->alpha = 1.0f;
                    if ( pActor->pInSector != pCar->pInSector )
                    {
                        bSectorChanged = 1;
                    }
                    else
                    {
                        bSectorChanged = 0;
                    }

                    if ( bSectorChanged )
                    {
                        sithCog_SectorSendMessage(pActor->pInSector, pActor, SITHCOG_MSG_EXITED);
                    }

                    if ( sithPlayer_bGuybrush )
                    {
                        sithPlayer_ToggleGuybrush();
                    }

                    sithPlayer_HidePlayer(sithPlayer_playerNum);
                    pCar->flags |= SITH_TF_DISABLED;
                    sithThing_ExitSector(pPlayerThing);

                    *pPlayerThing->thingInfo.actorInfo.pPlayer        = *pActor->thingInfo.actorInfo.pPlayer;
                    pPlayerThing->thingInfo.actorInfo.pPlayer->pThing = pPlayerThing;
                    pPlayerThing->thingInfo.actorInfo.health          = pActor->thingInfo.actorInfo.health;

                    sithPlayer_SetLocalPlayer(playerNum);
                    sithPlayer_ShowPlayer(playerNum, 0);
                    sithThing_SetPositionAndOrient(pPlayerThing, &pCar->pos, &pCar->orient);
                    sithThing_EnterSector(pPlayerThing, pCar->pInSector, 1, bSectorChanged == 0);
                    sithPuppet_SetMoveMode(pPlayerThing, SITHPUPPET_MOVEMODE_NORMAL);
                    sithPhysics_FindFloor(pPlayerThing, 1);
                    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], pPlayerThing, NULL);
                    pPlayerThing->moveStatus = SITHPLAYERMOVE_MINECAR_BOARDING;
                    pPlayerThing->alpha      = 1.0f;
                    if ( bBoard )
                    {
                        pPlayerThing->moveStatus = SITHPLAYERMOVE_MINECAR_IDLE;
                        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_HEADLIGHT;
                        pPlayerThing->thingInfo.actorInfo.headLightIntensity.alpha = 2.0f;
                    }
                    else
                    {
                        if ( dot > 0.0f )
                        {
                            sithPuppet_PlayMode(pPlayerThing, SITHPUPPETSUBMODE_HOPLEFT, sithVehicleControls_PuppetCallback);
                        }
                        else
                        {
                            sithPuppet_PlayMode(pPlayerThing, SITHPUPPETSUBMODE_HOPRIGHT, sithVehicleControls_PuppetCallback);
                        }

                        pPlayerThing->thingInfo.actorInfo.bControlsDisabled = 1;
                        sithVehicleControls_SendMessageUser0(pPlayerThing);
                    }

                    sithInventory_SetSwimmingInventory(pPlayerThing, 0);
                    sithCog_ThingSendMessage(pPlayerThing, pActor, SITHCOG_MSG_BOARDED);
                    sithCog_ThingSendMessage(pCar, pPlayerThing, SITHCOG_MSG_BOARDED);

                    if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                    {
                        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
                    }
                    else
                    {
                        pPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
                    }

                    if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
                    {
                        pPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
                    }
                    else
                    {
                        pPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
                    }

                    return 1;
                }

                break;
            }

            default:
                break;
        }
    }
    else
    {
        return 0;
    }
}

void J3DAPI sithPlayerControls_ExitVehicle(SithThing* pThing)
{
    int bSectChanged;
    SithSector* pSector;
    SithSector* pPrevSect;
    rdVector3 newPos;
    rdMatrix34 newRot;
    float heightOffset;
    SithThing* pAttachedThing;
    SithAttachFlag curFlags;
    SithThing* pPassenger;
    size_t playerSlot;

    sithVehicleControls_SendMessageUser1(pThing);
    sithInventory_SetSwimmingInventory(pThing, 1);

    rdVector_Copy3(&newPos, &pThing->pos);
    if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT )
    {
        rdVector_ScaleAdd3Acc(&newPos, &pThing->orient.rvec, -sithPlayerControls_flt_528D18);

        rdVector_Copy3(&newRot.uvec, &pThing->orient.uvec);
        rdVector_Copy3(&newRot.lvec, &pThing->orient.rvec);
        rdVector_Neg3(&newRot.rvec, &pThing->orient.lvec);

        heightOffset = sithPlayerControls_pVehicleBoardedThing->renderData.data.pModel3->insertOffset.z - sithPhysics_GetThingHeight(pThing);
        sithPlayerControls_pVehicleBoardedThing->flags |= SITH_TF_EMITLIGHT;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT )
    {
        rdVector_ScaleAdd3Acc(&newPos, &pThing->orient.rvec, sithPlayerControls_flt_528D18);

        rdVector_Copy3(&newRot.uvec, &pThing->orient.uvec);
        rdVector_Neg3(&newRot.lvec, &pThing->orient.rvec);
        rdVector_Copy3(&newRot.rvec, &pThing->orient.lvec);

        heightOffset = sithPlayerControls_pVehicleBoardedThing->renderData.data.pModel3->insertOffset.z - sithPhysics_GetThingHeight(pThing);
        sithPlayerControls_pVehicleBoardedThing->flags |= SITH_TF_EMITLIGHT;
    }
    else
    {
        rdVector_ScaleAdd3Acc(&newPos, &pThing->orient.rvec, -sithPlayerControls_flt_528D1C);

        rdVector_Copy3(&newRot.uvec, &rdroid_zVector3);
        rdVector_Cross3(&newRot.lvec, &newRot.uvec, &pThing->orient.lvec);
        rdVector_Normalize3Acc(&newRot.lvec);
        rdVector_Cross3(&newRot.rvec, &newRot.lvec, &newRot.uvec);
        rdVector_Normalize3Acc(&newRot.rvec);

        heightOffset = 0.0f;
        while ( pThing->pAttachedThing )
        {
            pAttachedThing = pThing->pAttachedThing;
            curFlags = pAttachedThing->attach.flags;
            sithThing_DetachThing(pAttachedThing);
            sithThing_AttachThingToThing(pAttachedThing, sithPlayerControls_pVehicleBoardedThing);
            pAttachedThing->attach.flags |= curFlags & SITH_ATTACH_NOMOVE;
        }
    }

    pPassenger        = sithPlayer_aPlayers[0].pThing;
    pPassenger->alpha = 1.0f;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);
    if ( !pSector )
    {
#line 4908
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "This is really bad, trying to exit vehicle to non-existent sector..\n", 0, 0, 0, 0);
    }

    if ( pSector == pThing->pInSector )
    {
        bSectChanged = 0;
    }
    else
    {
        bSectChanged = 1;
    }

    playerSlot  = sithPlayer_playerNum;
    pPrevSect = pThing->pInSector;
    if ( bSectChanged )
    {
        sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_EXITED);
    }

    sithPlayer_HidePlayer(playerSlot);

    *pPassenger->thingInfo.actorInfo.pPlayer        = *pThing->thingInfo.actorInfo.pPlayer;
    pPassenger->thingInfo.actorInfo.pPlayer->pThing = pPassenger;
    pPassenger->thingInfo.actorInfo.health          = pThing->thingInfo.actorInfo.health;

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
    {
        pPassenger->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
    }
    else
    {
        pPassenger->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
    {
        pPassenger->thingInfo.actorInfo.flags |= SITH_AF_POISONED;
    }
    else
    {
        pPassenger->thingInfo.actorInfo.flags &= ~SITH_AF_POISONED;
    }

    sithThing_ExitSector(pPassenger);
    sithPlayer_SetLocalPlayer(0);
    sithPlayer_ShowPlayer(0, 0);
    sithThing_SetPositionAndOrient(pPassenger, &newPos, &newRot);
    sithCog_ThingSendMessage(sithPlayerControls_pVehicleBoardedThing, pThing, SITHCOG_MSG_UNBOARDED);
    sithThing_EnterSector(pPassenger, pSector, 1, bSectChanged == 0);

    pPassenger->moveStatus = SITHPLAYERMOVE_STILL;
    sithPuppet_SetMoveMode(pPassenger, SITHPUPPET_MOVEMODE_NORMAL);
    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], pPassenger, NULL);
    sithPhysics_FindFloor(pPassenger, 1);
    pPassenger->alpha = 1.0f;

    sithThing_ExitSector(sithPlayerControls_pVehicleBoardedThing);
    rdVector_Copy3(&newPos, &pThing->pos);
    newPos.z += heightOffset;
    sithThing_SetPositionAndOrient(sithPlayerControls_pVehicleBoardedThing, &newPos, &pThing->orient);
    sithThing_EnterSector(sithPlayerControls_pVehicleBoardedThing, pPrevSect, 1, 1);
    sithPlayerControls_pVehicleBoardedThing->flags &= ~SITH_TF_DISABLED;
    sithPlayerControls_pVehicleBoardedThing = NULL;
}

void J3DAPI sithPlayerControls_RotateAimJoints(SithThing* pThing, float pitch, float yaw)
{
    rdVector3 curheadPYR;
    int armJoint;
    int jointCount;
    int weaponId;

    weaponId = sithInventory_GetCurrentWeapon(pThing);
    if ( pitch == 0.0f && yaw == 0.0f )
    {
        sithPlayerControls_CenterActorOrientation(pThing);
        return;
    }

    switch ( weaponId )
    {
        case SITHWEAPON_PISTOL:
        case SITHWEAPON_TOKAREV:
        case SITHWEAPON_MAUSER:
        case SITHWEAPON_COMTOKAREV:
        case SITHWEAPON_COMMAUSER:
            yaw += sithPlayerControls_pistolAimYawOffset;

            armJoint = sithThing_GetThingJointIndex(pThing, "inrarm");
            if ( armJoint >= 0 )
            {
                if ( pitch > 80.0f )
                {
                    pitch = 80.0f;
                }

                if ( pitch < -80.0f )
                {
                    pitch = -80.0f;
                }

                pThing->renderData.apTweakedAngles[armJoint].yaw  = pitch;
                pThing->renderData.apTweakedAngles[armJoint].roll = RD_MULTIPLY(pitch, -0.5f);
            }

            jointCount = sithThing_GetThingJointIndex(pThing, "intorso");
            if ( jointCount >= 0 )
            {
                if ( yaw > 45.0f )
                {
                    yaw = 45.0f;
                }

                if ( yaw < -45.0f )
                {
                    yaw = -45.0f;
                }

                pThing->renderData.apTweakedAngles[jointCount].yaw   = yaw;
                pThing->renderData.apTweakedAngles[jointCount].pitch = 0.0f;
            }
            break;

        case SITHWEAPON_SUBMACHINE:
        case SITHWEAPON_SHOTGUN:
        case SITHWEAPON_BAZOOKA:
        case SITHWEAPON_COMSUBMACHINE:
        case SITHWEAPON_COMSHOTGUN:
            yaw += sithPlayerControls_rifleAimYawOffset;
            // fallthrough

        case SITHWEAPON_SIMONOV:
        case SITHWEAPON_COMSIMONOV:
            jointCount = sithThing_GetThingJointIndex(pThing, "intorso");
            if ( jointCount >= 0 )
            {
                if ( pitch > 45.0f )
                {
                    pitch = 45.0f;
                }

                if ( pitch < -45.0f )
                {
                    pitch = -45.0f;
                }

                pThing->renderData.apTweakedAngles[jointCount].pitch = pitch;

                jointCount = sithThing_GetThingJointIndex(pThing, "intorso");
                if ( yaw > 45.0f )
                {
                    yaw = 45.0f;
                }

                if ( yaw < -45.0f )
                {
                    yaw = -45.0f;
                }

                pThing->renderData.apTweakedAngles[jointCount].yaw  = yaw;
                pThing->renderData.apTweakedAngles[jointCount].roll = 0.0f;
            }
            break;

        case SITHWEAPON_MIRROR:
            jointCount = sithThing_GetThingJointIndex(pThing, "intorso");
            if ( jointCount >= 0 )
            {
                if ( pitch > 45.0f )
                {
                    pitch = 45.0f;
                }

                if ( pitch < -45.0f )
                {
                    pitch = -45.0f;
                }

                pThing->renderData.apTweakedAngles[jointCount].pitch = pitch;

                jointCount = sithThing_GetThingJointIndex(pThing, "intorso");
                if ( yaw > 45.0f )
                {
                    yaw = 45.0f;
                }

                if ( yaw < -45.0f )
                {
                    yaw = -45.0f;
                }

                pThing->renderData.apTweakedAngles[jointCount].yaw  = yaw;
                pThing->renderData.apTweakedAngles[jointCount].roll = 0.0f;
            }
            break;

        default:
            sithPlayerControls_CenterActorOrientation(pThing);
            break;
    }

    if ( pitch != 0.0f || yaw != 0.0f )
    {
        curheadPYR.pitch = pitch;
        curheadPYR.yaw   = 0.0f;
        curheadPYR.roll  = 0.0f;
        sithActor_SetHeadPYR(pThing, &curheadPYR);
    }
}

void J3DAPI sithPlayerControls_CenterActorOrientation(SithThing* pThing)
{
    rdVector3 pyr;
    int jointNum;

    rdVector_Set3(&pyr, 0.0f, 0.0f, 0.0f);
    rdVector_Copy3(&pThing->thingInfo.actorInfo.headPYR, &pyr);

    jointNum = sithThing_GetThingJointIndex(pThing, "inrarm");
    rdVector_Copy3(&pThing->renderData.apTweakedAngles[jointNum], &pyr);

    jointNum = sithThing_GetThingJointIndex(pThing, "intorso");
    rdVector_Copy3(&pThing->renderData.apTweakedAngles[jointNum], &pyr);

    sithActor_SetHeadPYR(pThing, &pyr);
}

SithThing* J3DAPI sithPlayerControls_GetTargetThing(void)
{
    return sithPlayerControls_pTargetThing;
}

SithThing* J3DAPI sithPlayerControls_GetVehicleBoardedThing(void)
{
    return sithPlayerControls_pVehicleBoardedThing;
}

void J3DAPI sithPlayerControls_SetVehicleBoardedThing(SithThing* pThing)
{
    sithPlayerControls_pVehicleBoardedThing = pThing;
}
 
signed int J3DAPI sithPlayerControls_CanStrafeMove(SithThing* pThing, int bMoveRight)
{
    int bCanMove;
    int bNoCollision;
    int sFlags;
    rdVector3 loc;
    rdVector3 moveNorm;
    rdVector3 curDownNorm;
    SithSector* pCurSector;
    SithCollision* pCollision;
    float distance;

    if ( bMoveRight == 0 )
    {
        rdVector_Neg3(&moveNorm, &pThing->orient.rvec);
    }
    else if ( bMoveRight == 1 )
    {
        rdVector_Copy3(&moveNorm, &pThing->orient.rvec);
    }
    else
    {
        return 0;
    }

    bNoCollision = 1;
    sFlags  = 0xA00;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, 0.1f, 0.089f, sFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            bNoCollision = 0;
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 || (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0 )
            {
                bNoCollision = 0;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bNoCollision = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( !bNoCollision )
    {
        return 0;
    }

    rdVector_Scale3(&loc, &pThing->orient.lvec, 0.1f);
    rdVector_Add3Acc(&loc, &pThing->pos);
    pCurSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &loc, 0.0f);
    if ( !pCurSector )
    {
        return 0;
    }

    rdVector_Neg3(&curDownNorm, &rdroid_zVector3);
    bCanMove = 0;
    sithCollision_SearchForCollisions(pCurSector, NULL, &loc, &curDownNorm, 0.11f, 0.01f, sFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            rdFace* pFace = &pCollision->pSurfaceCollided->face;
            float dotVal;

            distance = rdMath_DistancePointToPlane(&loc, &pFace->normal, &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]]);
            if ( distance < 0.088f || distance > 0.092f )
            {
                break;
            }

            if ( (dotVal = rdVector_Dot3(&pFace->normal, &rdroid_zVector3)) < 0.979f )
            {
                break;
            }

            bCanMove = 1;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pFaceCollided )
        {
            rdFace* pWall = pCollision->pFaceCollided;
            rdVector3 thingNormal;
            float curThingDot;
            rdVector3 vertex;
            rdVector3* aVertices;
            rdMatrix34 orient;
            rdVector3 normal;
            rdModel3Mesh* mesh;

            rdMatrix_TransformVector34(&thingNormal, &pWall->normal, &pCollision->pThingCollided->orient);
            if ( (curThingDot = rdVector_Dot3(&thingNormal, &rdroid_zVector3)) > 0.979f )
            {
                rdMatrix_Copy34(&orient, &pCollision->pThingCollided->orient);
                mesh     = pCollision->pMeshCollided;
                aVertices = mesh->apVertices;
                rdMatrix_TransformVector34(&normal, &pWall->normal, &orient);
                rdMatrix_TransformVector34(&vertex, &aVertices[pWall->aVertices[0]], &orient);
                rdVector_Add3Acc(&vertex, &pCollision->pThingCollided->pos);

                distance = rdMath_DistancePointToPlane(&loc, &normal, &vertex);
                if ( distance < 0.088f || distance > 0.092f )
                {
                    break;
                }

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
