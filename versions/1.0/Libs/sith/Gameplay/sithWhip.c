#include "sithWhip.h"

#include <rdroid/Math/rdMatrix.h>
/* Retail calls rdVector_Set3 out of line in StartWhipSwing. */
#define RDVECTOR_NO_INLINE_SET3
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithWhip_countfit_1, 0)
CF_PROTO(sithWhip_countfit_2, 0)
/* end count-fit */

// Camera offsets for whip aim at target
static rdVector3 sithWhip_vecCameraOffsetWhipClimbAim     = { 0.0f , -0.1f , 0.02f };
static rdVector3 sithWhip_vecCameraLookOffsetWhipClimbAim = { 0.0f , 0.04f , 0.15f };

static rdVector3 sithWhip_vecCameraOffsetWhipSwingAim     = { 0.0f , -0.2f , 0.035f };
static rdVector3 sithWhip_vecCameraLookOffsetWhipSwingAim = { 0.0f , 0.02f , 0.02f };

static SithThing* sithWhip_pWhipThing      = NULL; // Whip actor thing, created for whip swing or whip climb move. Added: Init. to nil
static SithThing* sithWhip_pWhipSwingThing = NULL; // A thing to which whip mounts to do whip swing move. Added: Init. to nil
static SithThing* sithWhip_pWhipClimbThing = NULL; // A thing to which whip mounts to do whip climbing. Added: Init. to nil

static int sithWhip_bClimbDismountFinish = 0; // Added: Init. to false
static int sithWhip_bFoundWhipClimbThing = 0; // Unused. Added: Init. to false

void J3DAPI sithWhip_WhipClimbDismount(SithThing* pThing);
void J3DAPI sithWhip_CreatePlayerWhip(SithThing* pThing);
void J3DAPI sithWhip_RemoveWhip(void);

int J3DAPI sithWhip_SearchWhipSwingThing(SithThing* pThing);
int J3DAPI sithWhip_SearchWhipClimbThing(SithThing* pThing);

void J3DAPI sithWhip_UpdateWhipAim(SithThing* pThing)
{
#line 85
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));
    if ( sithWhip_SearchWhipSwingThing(pThing) )
    {
        sithCamera_SetExtCameraOffset(&sithWhip_vecCameraOffsetWhipSwingAim);
        sithCamera_SetExtCameraLookOffset(&sithWhip_vecCameraLookOffsetWhipSwingAim);
        sithWeapon_SendMessageAim(pThing, /*bAim=*/1);
    }

    else if ( sithWhip_SearchWhipClimbThing(pThing) )
    {
        sithCamera_SetExtCameraOffset(&sithWhip_vecCameraOffsetWhipClimbAim);
        sithCamera_SetExtCameraLookOffset(&sithWhip_vecCameraLookOffsetWhipClimbAim);
        sithWeapon_SendMessageAim(pThing, /*bAim=*/1);
    }
    else
    {
        sithWhip_Reset();
        sithWeapon_SendMessageAim(pThing, /*bAim=*/0);
    }
}

int J3DAPI sithWhip_StartWhipSwing(SithThing* pThing)
{
    float dot;
    SithCog* pCog;
    rdVector3 newLVec;
    rdVector3 pyr;

#line 112
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    dot = rdVector_Dot3(&pThing->orient.rvec, &sithWhip_pWhipSwingThing->orient.lvec);
    if ( dot != 0.0f )
    {
        rdVector_Copy3(&newLVec, &sithWhip_pWhipSwingThing->orient.lvec);

        if ( dot < 0.0f )
        {
            rdVector_Set3(&pyr, 0.0f, -90.0f, 0.0f);
        }
        else
        {
            rdVector_Set3(&pyr, 0.0f, 90.0f, 0.0f);
        }

        rdVector_Rotate3Acc(&newLVec, &pyr);

        // Set new LVec
        rdVector_Copy3(&pThing->orient.lvec, &newLVec);

        // Calculate RVec
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_zVector3);
        rdVector_Normalize3Acc(&pThing->orient.rvec);

        // Calculate UVec
        rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);
    }

    sithWhip_CreatePlayerWhip(pThing);
    if ( sithWhip_pWhipThing )
    {
        pThing->moveStatus = SITHPLAYERMOVE_WHIPSWINGING;
        sithWhip_pWhipSwingThing = NULL; // Note, no need to free as it's only reference pointer to world thing
        pThing->thingInfo.actorInfo.bControlsDisabled = 1;

        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

        pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(8)); // weap_whip.cog
        if ( pCog )
        {
            // Send whip thing start notification via User0 message
            sithWhip_pWhipThing->pCaptureCog = pCog;
            sithCog_SendMessage(pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_THING, sithWhip_pWhipThing->idx, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }

        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPSWING, sithPlayerControls_PuppetCallback);
        sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPSWING, NULL);
        return 1;
    }

    return 0;
}

void J3DAPI sithWhip_DeactivateWhip(SithThing* pThing)
{
    SithCog* pCog;

    if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        sithPuppet_StopForceMove(pThing, 0);
    }

    pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(8)); // weap_whip.cog
    if ( pCog )
    {
        sithCog_SendMessage(pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_THING, -99, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        sithWhip_pWhipThing = NULL; // TODO: Not freed
    }
}

int J3DAPI sithWhip_StartWhipClimb(SithThing* pThing, SithThing* pWhippedThing)
{
#line 184
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    sithCamera_RestoreExtCamera();
    sithInventory_SetSwimmingInventory(pThing, 0);
    sithWeapon_SetLastWeapon(SITHWEAPON_PISTOL);

    pThing->moveStatus = SITHPLAYERMOVE_WHIP_CLIMB_START;
    pThing->thingInfo.actorInfo.bControlsDisabled = 1;

    sithThing_DetachThing(pThing);
    pThing->moveInfo.physics.flags &= ~SITH_PF_USEGRAVITY;

    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_DRAWWEAPON, 0.0f);
    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_AIMWEAPON, 0.0f);

    pThing->pos.x = pWhippedThing->pos.x;
    pThing->pos.y = pWhippedThing->pos.y;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

    pThing->collide.movesize = 0.055f;

    sithWhip_CreatePlayerWhip(pThing);
    if ( sithWhip_pWhipThing )
    {
        // Why the 2 calls?
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, sithWhip_WhipClimbPuppetCallback);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, NULL);

        sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPCLIMBMOUNT, sithWhip_WhipFirePuppetCallback);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
        return 1;
    }

    return 0;
}

void J3DAPI sithWhip_SetActorWhipClimbIdle(SithThing* pThing)
{
    sithWhip_pWhipClimbThing = NULL;
    sithPhysics_ResetThingMovement(pThing);

    pThing->moveStatus = SITHPLAYERMOVE_WHIPCLIMBIDLE;
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;

    pThing->attach.flags = SITH_ATTACH_THINGCLIMBWHIP;
    pThing->attach.posOffset.z = 0.0f;

    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBIDLE, NULL);
    sithThing_RemoveSwapEntry(pThing, pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum);
}

void J3DAPI sithWhip_FinishWhipClimbDismount(SithThing* pThing)
{
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    if ( sithWhip_bClimbDismountFinish )
    {
        sithWhip_bClimbDismountFinish = 0;
        sithWhip_WhipClimbDismount(pThing);
    }
}

void J3DAPI sithWhip_WhipClimbDismount(SithThing* pThing)
{
    pThing->collide.movesize = 0.04f;

    if ( sithWhip_pWhipThing )
    {
        sithThing_RemoveThing(sithWorld_pCurrentWorld, sithWhip_pWhipThing);
        sithWhip_pWhipThing = NULL;
    }

    sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBIDLE, 0.0f);

    sithInventory_SetCurrentWeapon(pThing, SITHWEAPON_WHIP);
    pThing->thingInfo.actorInfo.weaponInfo.curWeaponID = SITHWEAPON_WHIP;
    sithWeapon_SetWeaponModel(pThing, SITHWEAPON_WHIP);

    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DRAWWEAPON, NULL);

    pThing->moveStatus = SITHPLAYERMOVE_FALLING;
    pThing->attach.flags = 0;
    pThing->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
    sithInventory_SetSwimmingInventory(pThing, 1);
}

void J3DAPI sithWhip_ProcessWhipClimbMove(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithActorInfo* actorInfo = &pThing->thingInfo.actorInfo;
    int bCurJumpOff;
    int bUp;
    int bDown;

#line 274
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));
    if ( pThing->thingInfo.actorInfo.bControlsDisabled || pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        return;
    }

    bCurJumpOff = sithControl_GetKey(SITHCONTROL_JUMP, NULL);
    bUp = sithControl_GetKey(SITHCONTROL_FORWARD, NULL);
    bDown = sithControl_GetKey(SITHCONTROL_BACK, NULL);

    if ( bCurJumpOff )
    {
        if ( pThing->attach.posOffset.z > 3.0f )
        {
            rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT, sithWhip_ClimbDismountPuppetCallback);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);

            if ( sithWhip_pWhipThing )
            {
                sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPCLIMBDISMOUNT, NULL);
            }

            pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        }
        else
        {
            sithWhip_WhipClimbDismount(pThing);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
        }
    }

    if ( bUp && !bCurJumpOff )
    {
        // Move up
        if ( pThing->attach.posOffset.z <= 3.0f )
        {
            rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBUP, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDLEFT);

            pThing->thingInfo.actorInfo.bForceMovePlay = 1;
            pThing->attach.posOffset.z = pThing->attach.posOffset.z + 1.0f;
        }
    }
    else if ( bDown )
    {
        // Move down
        if ( pThing->attach.posOffset.z <= 0.0f )
        {
            sithWhip_WhipClimbDismount(pThing);
        }
        else
        {
            rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_WHIPCLIMBDOWN, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHANDRIGHT);

            pThing->thingInfo.actorInfo.bForceMovePlay = 1;
            pThing->attach.posOffset.z = pThing->attach.posOffset.z - 1.0f;
        }
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNLEFT, 0) )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
        pPhysics->angularVelocity.y = sithTime_fps;
        pPhysics->angularVelocity.y = pPhysics->angularVelocity.y + actorInfo->maxRotVelocity;
    }
    else if ( sithControl_GetKey(SITHCONTROL_TURNRIGHT, 0) )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_MOVEPOS);
        pPhysics->angularVelocity.y = RD_MULTIPLY(sithTime_fps, -1.0f);
        pPhysics->angularVelocity.y = RD_MULTIPLY(actorInfo->maxRotVelocity, -1.0f) + pPhysics->angularVelocity.y;
    }
    else
    {
        sithPhysics_ResetThingMovement(pThing);
    }
}

void J3DAPI sithWhip_WhipFirePuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    tSoundHandle hSnd = 0;

    if ( markerType == RDKEYMARKER_ATTACK )
    {
        hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(3)); // gen_whip_fire.wav
        if ( hSnd ) {
            sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);
        }

        hSnd = 0;
    }
}

void J3DAPI sithWhip_WhipClimbPuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType)
{
    switch ( markerType )
    {
        case 0:
            if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
            {
                sithPuppet_StopForceMove(pThing, /*bStopTracks=*/0);
            }

            sithPuppet_FreeTrackByIndex(pThing, track);
            break;

        case RDKEYMARKER_CLIMB:
        {
            rdVector3 pos;

            sithThing_RemoveSwapEntry(pThing, pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum);

            rdVector_Copy3(&pos, &sithWhip_pWhipThing->pos);

            sithWhip_RemoveWhip(); // Note, the function also frees sithWhip_pWhipThing

            sithWhip_pWhipThing = sithTemplate_GetTemplate("+whipc_actor");
            if ( sithWhip_pWhipThing )
            {
                pos.z += sithWhip_pWhipThing->renderData.data.pModel3->radius;
                sithWhip_pWhipThing = sithThing_CreateThingAtPos(sithWhip_pWhipThing, &pos, &rdroid_identMatrix34, pThing->pInSector, NULL);
                if ( sithWhip_pWhipThing ) {
                    sithPuppet_PlayMode(sithWhip_pWhipThing, SITHPUPPETSUBMODE_WHIPCLIMBIDLE, NULL);
                }
            }

            sithWhip_WhipFirePuppetCallback(pThing, track, RDKEYMARKER_ATTACK);
            break;
        }
    }
}

void J3DAPI sithWhip_ClimbDismountPuppetCallback(SithThing* pThing, int track, rdKeyMarkerType mt)
{
    switch ( mt )
    {
        case 0:
            sithWhip_bClimbDismountFinish = 1;
            if ( pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
            {
                sithPuppet_StopForceMove(pThing, /*bStopTracks=*/0);
            }

            sithPuppet_FreeTrackByIndex(pThing, track);
            break;
    }
}

SithThing* J3DAPI sithWhip_GetWhipSwingThing(void)
{
    return sithWhip_pWhipSwingThing;
}

SithThing* J3DAPI sithWhip_GetWhipClimbThing(void)
{
    return sithWhip_pWhipClimbThing;
}

void J3DAPI sithWhip_Reset(void)
{
    sithWhip_pWhipSwingThing = NULL;
    sithWhip_pWhipClimbThing = NULL;

    sithCamera_RestoreExtCamera();
}

int J3DAPI sithWhip_Save(DPID idTo, uint32_t outstream)
{
    uint8_t* pCurDSSMsgData_ = sithMulti_message.data;
    SITHDSS_PUSHINT16(sithThing_GetThingIndex(sithWhip_pWhipThing));
    SITHDSS_PUSHUINT8(sithWhip_bClimbDismountFinish);
    sithMulti_message.type = SITHDSS_WHIPSTATUS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0x1);
}

int J3DAPI sithWhip_Restore(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    int16_t thingIdx;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 460
    SITH_ASSERTREL(pMsg);

    thingIdx = SITHDSS_POPINT16();
    sithWhip_pWhipThing = sithThing_GetThingByIndex(thingIdx);
    sithWhip_bClimbDismountFinish = SITHDSS_POPUINT8();

    sithWhip_pWhipSwingThing = NULL;
    sithWhip_pWhipClimbThing = NULL;

    return 1;
}

int J3DAPI sithWhip_SearchWhipSwingThing(SithThing* pThing)
{
    rdVector3 moveNorm = { 0.0f, 1.0f, 1.0f };
    SithCollision* pCollision;
    int flags;
    float radius;
    float searchDist;

#line 486
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));
    sithWhip_pWhipSwingThing = NULL;
    flags = 0x122;

    // Search for whip swing thing in the forward,up direction at max distance 0.65 (6.5m)
    rdVector_Copy3(&moveNorm, &pThing->orient.lvec);
    moveNorm.z = sqrtf(RD_MULTIPLY(moveNorm.x, moveNorm.x) + RD_MULTIPLY(moveNorm.y, moveNorm.y));
    rdVector_Normalize3Acc(&moveNorm);

    radius  = 0.075f;
    searchDist = 0.65f;
    if ( sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, searchDist, radius, flags) <= searchDist )
    {
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                if ( pCollision->pThingCollided == pThing )
                {
                    continue;
                }

                if ( (pCollision->pThingCollided->flags & SITH_TF_WHIPSWING) != 0 )
                {
                    sithWhip_pWhipSwingThing = pCollision->pThingCollided;
                    break;
                }
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    if ( sithWhip_pWhipSwingThing )
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int J3DAPI sithWhip_SearchWhipClimbThing(SithThing* pThing)
{
    rdVector3 moveNorm = { 0.0f, 0.0f, 1.0f };
    SithCollision* pCollision;
    int flags;
    float radius;
    float searchDist;

#line 546
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    flags = 0x122;
    sithWhip_pWhipClimbThing = NULL; // No need to free as it's reference to world thing

    // Search for whip climb thing in the up direction at max distance 0.6 (6m)
    radius  = pThing->collide.movesize;
    radius  = 0.075f;
    searchDist = 0.6f;
    if ( sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &moveNorm, searchDist, radius, flags) <= searchDist )
    {
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                if ( pCollision->pThingCollided == pThing )
                {
                    continue;
                }

                if ( (pCollision->pThingCollided->flags & SITH_TF_WHIPCLIMB) != 0 )
                {
                    sithWhip_pWhipClimbThing = pCollision->pThingCollided;
                    if ( sithWhip_pWhipClimbThing->pos.z - pThing->pos.z < 0.3f )
                    {
                        sithWhip_pWhipClimbThing = NULL;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    // Note, the unused var sithWhip_bFoundWhipClimbThing
    if ( sithWhip_pWhipClimbThing )
    {
        sithWhip_bFoundWhipClimbThing = 1;
        return 1;
    }
    else
    {
        sithWhip_bFoundWhipClimbThing = 0;
        return 0;
    }
}

void J3DAPI sithWhip_CreatePlayerWhip(SithThing* pThing)
{
    int handMeshIndex;
    const SithThing* pTemplate;
    rdModel3* pWhipHandModel;

#line 603
    SITH_ASSERTREL((pThing!= NULL) && (pThing->type == SITH_THING_PLAYER) && (pThing->renderData.pPuppet != NULL));

    handMeshIndex = sithThing_GetThingMeshIndex(pThing, "inrhand");
    pTemplate = sithTemplate_GetTemplate("+whip_actor");
    if ( !pTemplate || handMeshIndex == -1 )
    {
        return;
    }

    sithWhip_pWhipThing = sithThing_CreateThingAtPos(pTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, 0);
    if ( sithWhip_pWhipThing )
    {
        pWhipHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(31)); // whip hand - weap_whip.3do
        if ( pWhipHandModel )
        {
            pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum = sithThing_AddSwapEntry(pThing, handMeshIndex, pWhipHandModel, /*meshNumSrc=*/0);
        }

        sithThing_AttachThingToThingMesh(sithWhip_pWhipThing, pThing, handMeshIndex);
    }
}

void J3DAPI sithWhip_RemoveWhip(void)
{
    if ( sithWhip_pWhipThing )
    {
        sithWhip_pWhipThing->thingInfo.actorInfo.pThingMeshAttached = NULL;
        sithWhip_pWhipThing->thingInfo.actorInfo.attachMeshNum = 0;

        sithThing_RemoveThing(sithWorld_pCurrentWorld, sithWhip_pWhipThing);
        sithWhip_pWhipThing = NULL;
    }
}