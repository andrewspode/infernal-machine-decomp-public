#include "sithCogFunctionThing.h"

#include <sith/AI/sithAIMove.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <std/General/stdMath.h>
#include <std/Win95/stdComm.h>

/* count-fit: placeholder declarations, not reconstructed source */


CF_PROTO(sithCogFunctionThing_countfit_1, 2)
/* end count-fit */
/* Forward prototypes of the cog functions; the parameter is left unnamed (release-chosen prototype style). */
void J3DAPI sithCogFunctionThing_IsGhostVisible(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingAlpha(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingAlpha(SithCog*);
void J3DAPI sithCogFunctionThing_CreatePolylineThing(SithCog*);
void J3DAPI sithCogFunctionThing_GetMoveStatus(SithCog*);
void J3DAPI sithCogFunctionThing_CopyOrient(SithCog*);
void J3DAPI sithCogFunctionThing_CopyOrientAndPos(SithCog*);
void J3DAPI sithCogFunctionThing_CreateLaser(SithCog*);
void J3DAPI sithCogFunctionThing_CreateLightning(SithCog*);
void J3DAPI sithCogFunctionThing_AnimateSpriteSize(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingType(SithCog*);
void J3DAPI sithCogFunctionThing_IsThingAutoAiming(SithCog*);
void J3DAPI sithCogFunctionThing_PlayForceMoveMode(SithCog*);
void J3DAPI sithCogFunctionThing_SetMoveMode(SithCog*);
void J3DAPI sithCogFunctionThing_CreateThing(SithCog*);
void J3DAPI sithCogFunctionThing_CreateThingAtPos(SithCog*);
void J3DAPI sithCogFunctionThing_DamageThing(SithCog*);
void J3DAPI sithCogFunctionThing_HealThing(SithCog*);
void J3DAPI sithCogFunctionThing_GetHealth(SithCog*);
void J3DAPI sithCogFunctionThing_GetMaxHealth(SithCog*);
void J3DAPI sithCogFunctionThing_SetHealth(SithCog*);
void J3DAPI sithCogFunctionThing_DestroyThing(SithCog*);
void J3DAPI sithCogFunctionThing_JumpToFrame(SithCog*);
void J3DAPI sithCogFunctionThing_MoveToFrame(SithCog*);
void J3DAPI sithCogFunctionThing_SkipToFrame(SithCog*);
void J3DAPI sithCogFunctionThing_RotatePivot(SithCog*);
void J3DAPI sithCogFunctionThing_Rotate(SithCog*);
void J3DAPI sithCogFunctionThing_RotateToPYR(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingLight(SithCog*);
void J3DAPI sithCogFunctionThing_SetExtCamOffset(SithCog*);
void J3DAPI sithCogFunctionThing_SetExtCamOffsetToThing(SithCog*);
void J3DAPI sithCogFunctionThing_SetExtCamLookOffset(SithCog*);
void J3DAPI sithCogFunctionThing_SetExtCamLookOffsetToThing(SithCog*);
void J3DAPI sithCogFunctionThing_MakeCamera2LikeCamera1(SithCog*);
void J3DAPI sithCogFunctionThing_RestoreExtCam(SithCog*);
void J3DAPI sithCogFunctionThing_SetCameraPosition(SithCog*);
void J3DAPI sithCogFunctionThing_SetCameraLookInterp(SithCog*);
void J3DAPI sithCogFunctionThing_SetCameraPosInterp(SithCog*);
void J3DAPI sithCogFunctionThing_GetCameraFOV(SithCog*);
void J3DAPI sithCogFunctionThing_GetCameraPosition(SithCog*);
void J3DAPI sithCogFunctionThing_SetCameraInterpSpeed(SithCog*);
void J3DAPI sithCogFunctionThing_SetCameraFOV(SithCog*);
void J3DAPI sithCogFunctionThing_ResetCameraFOV(SithCog*);
void J3DAPI sithCogFunctionThing_SetCameraFadeThing(SithCog*);
void J3DAPI sithCogFunctionThing_ThingLight(SithCog*);
void J3DAPI sithCogFunctionThing_ThingLightAnim(SithCog*);
void J3DAPI sithCogFunctionThing_ThingFadeAnim(SithCog*);
void J3DAPI sithCogFunctionThing_WaitForStop(SithCog*);
void J3DAPI sithCogFunctionThing_WaitForAnimStop(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingSector(SithCog*);
void J3DAPI sithCogFunctionThing_GetCurFrame(SithCog*);
void J3DAPI sithCogFunctionThing_GetGoalFrame(SithCog*);
void J3DAPI sithCogFunctionThing_StopThing(SithCog*);
void J3DAPI sithCogFunctionThing_IsThingMoving(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingPulse(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingTimer(SithCog*);
void J3DAPI sithCogFunctionThing_CaptureThing(SithCog*);
void J3DAPI sithCogFunctionThing_ReleaseThing(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingParent(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingPos(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingPos(SithCog*);
void J3DAPI sithCogFunctionThing_GetInventory(SithCog*);
void J3DAPI sithCogFunctionThing_SetInventory(SithCog*);
void J3DAPI sithCogFunctionThing_ChangeInventory(SithCog*);
void J3DAPI sithCogFunctionThing_GetInventoryCog(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingVelocity(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingVel(SithCog*);
void J3DAPI sithCogFunctionThing_ApplyForce(SithCog*);
void J3DAPI sithCogFunctionThing_AddThingVel(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingLVec(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingUVec(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingRVec(SithCog*);
void J3DAPI sithCogFunctionThing_GetActorHeadPYR(SithCog*);
void J3DAPI sithCogFunctionThing_DetachThing(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingAttachFlags(SithCog*);
void J3DAPI sithCogFunctionThing_AttachThingToSurf(SithCog*);
void J3DAPI sithCogFunctionThing_AttachThingToThing(SithCog*);
void J3DAPI sithCogFunctionThing_AttachThingToThingEx(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingAttachedThing(SithCog*);
void J3DAPI sithCogFunctionThing_WaitMode(SithCog*);
void J3DAPI sithCogFunctionThing_PlayMode(SithCog*);
void J3DAPI sithCogFunctionThing_StopMode(SithCog*);
void J3DAPI sithCogFunctionThing_SynchMode(SithCog*);
void J3DAPI sithCogFunctionThing_IsModePlaying(SithCog*);
void J3DAPI sithCogFunctionThing_PauseMode(SithCog*);
void J3DAPI sithCogFunctionThing_ResumeMode(SithCog*);
void J3DAPI sithCogFunctionThing_TrackToMode(SithCog*);
void J3DAPI sithCogFunctionThing_PlayKey(SithCog*);
void J3DAPI sithCogFunctionThing_PlayKeyEx(SithCog*);
void J3DAPI sithCogFunctionThing_PauseKey(SithCog*);
void J3DAPI sithCogFunctionThing_ResumeKey(SithCog*);
void J3DAPI sithCogFunctionThing_StopKey(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMesh(SithCog*);
void J3DAPI sithCogFunctionThing_RestoreThingMesh(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingModel(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingModel(SithCog*);
void J3DAPI sithCogFunctionThing_SetArmedMode(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingFlags(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingFlags(SithCog*);
void J3DAPI sithCogFunctionThing_ClearThingFlags(SithCog*);
void J3DAPI sithCogFunctionThing_TeleportThing(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingType(SithCog*);
void J3DAPI sithCogFunctionThing_GetCollideType(SithCog*);
void J3DAPI sithCogFunctionThing_SetCollideType(SithCog*);
void J3DAPI sithCogFunctionThing_FirstThingInSector(SithCog*);
void J3DAPI sithCogFunctionThing_NextThingInSector(SithCog*);
void J3DAPI sithCogFunctionThing_PrevThingInSector(SithCog*);
void J3DAPI sithCogFunctionThing_GetInventoryMinimum(SithCog*);
void J3DAPI sithCogFunctionThing_GetInventoryMaximum(SithCog*);
void J3DAPI sithCogFunctionThing_GetLoadedFrames(SithCog*);
void J3DAPI sithCogFunctionThing_GetFramePos(SithCog*);
void J3DAPI sithCogFunctionThing_GetFrameRot(SithCog*);
void J3DAPI sithCogFunctionThing_PathMovePause(SithCog*);
void J3DAPI sithCogFunctionThing_SetHeadLightIntensity(SithCog*);
void J3DAPI sithCogFunctionThing_GetHeadLightIntensity(SithCog*);
void J3DAPI sithCogFunctionThing_IsThingVisible(SithCog*);
void J3DAPI sithCogFunctionThing_PathMoveResume(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingCurLightMode(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingCurLightMode(SithCog*);
void J3DAPI sithCogFunctionThing_SetActorExtraSpeed(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingTemplate(SithCog*);
void J3DAPI sithCogFunctionThing_SetLifeleft(SithCog*);
void J3DAPI sithCogFunctionThing_GetLifeleft(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingThrust(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingThrust(SithCog*);
void J3DAPI sithCogFunctionThing_AmputateJoint(SithCog*);
void J3DAPI sithCogFunctionThing_SetActorWeapon(SithCog*);
void J3DAPI sithCogFunctionThing_GetActorWeapon(SithCog*);
void J3DAPI sithCogFunctionThing_GetPhysicsFlags(SithCog*);
void J3DAPI sithCogFunctionThing_SetPhysicsFlags(SithCog*);
void J3DAPI sithCogFunctionThing_ClearPhysicsFlags(SithCog*);
void J3DAPI sithCogFunctionThing_ParseArg(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingRotVel(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingRotVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetRotThrust(SithCog*);
void J3DAPI sithCogFunctionThing_GetRotThrust(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingLook(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingHeadLookPos(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingHeadLookThing(SithCog*);
void J3DAPI sithCogFunctionThing_IsThingCrouching(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingClassCog(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingClassCog(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingCaptureCog(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingCaptureCog(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingRespawn(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingSignature(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingAttachFlags(SithCog*);
void J3DAPI sithCogFunctionThing_ClearThingAttachFlags(SithCog*);
void J3DAPI sithCogFunctionThing_GetParticleSize(SithCog*);
void J3DAPI sithCogFunctionThing_SetParticleSize(SithCog*);
void J3DAPI sithCogFunctionThing_GetParticleGrowthSpeed(SithCog*);
void J3DAPI sithCogFunctionThing_SetParticleGrowthSpeed(SithCog*);
void J3DAPI sithCogFunctionThing_GetParticleTimeoutRate(SithCog*);
void J3DAPI sithCogFunctionThing_SetParticleTimeoutRate(SithCog*);
void J3DAPI sithCogFunctionThing_GetTypeFlags(SithCog*);
void J3DAPI sithCogFunctionThing_SetTypeFlags(SithCog*);
void J3DAPI sithCogFunctionThing_ClearTypeFlags(SithCog*);
void J3DAPI sithCogFunctionThing_TakeItem(SithCog*);
void J3DAPI sithCogFunctionThing_HasLOS(SithCog*);
void J3DAPI sithCogFunctionThing_GetFireOffset(SithCog*);
void J3DAPI sithCogFunctionThing_SetFireOffset(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingUserData(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingUserData(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingCollideSize(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingCollideSize(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMoveSize(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMoveSize(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMass(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMass(SithCog*);
void J3DAPI sithCogFunctionThing_SyncThingPos(SithCog*);
void J3DAPI sithCogFunctionThing_SyncThingAttachment(SithCog*);
void J3DAPI sithCogFunctionThing_SyncThingState(SithCog*);
void J3DAPI sithCogFunctionThing_GetMajorMode(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingPosEx(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingGuid(SithCog*);
void J3DAPI sithCogFunctionThing_GetGuidThing(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMaxVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMaxVel(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMaxAngVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMaxAngVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetJointAngle(SithCog*);
void J3DAPI sithCogFunctionThing_GetJointAngle(SithCog*);
void J3DAPI sithCogFunctionThing_SetMaxHeadPitch(SithCog*);
void J3DAPI sithCogFunctionThing_SetMinHeadPitch(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMaxHeadPitch(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMinHeadPitch(SithCog*);
void J3DAPI sithCogFunctionThing_SetMaxHeadYaw(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMaxHeadYaw(SithCog*);
void J3DAPI sithCogFunctionThing_InterpolatePYR(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingLVecPYR(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingLVec(SithCog*);
void J3DAPI sithCogFunctionThing_SetHeadPYR(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingAirDrag(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingJointPos(SithCog*);
void J3DAPI sithCogFunctionThing_IsThingModelName(SithCog*);
void J3DAPI sithCogFunctionThing_GetMeshByName(SithCog*);
void J3DAPI sithCogFunctionThing_GetNodeByName(SithCog*);
void J3DAPI sithCogFunctionThing_AttachThingToThingMesh(SithCog*);
void J3DAPI sithCogFunctionThing_DetachThingMesh(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMaxRotVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMaxRotVel(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingMaxHeadVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingMaxHeadVel(SithCog*);
void J3DAPI sithCogFunctionThing_SetPuppetModeFPS(SithCog*);
void J3DAPI sithCogFunctionThing_ResetThing(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingInsertOffset(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingInsertOffset(SithCog*);
void J3DAPI sithCogFunctionThing_GetThingEyeOffset(SithCog*);
void J3DAPI sithCogFunctionThing_CheckFloorDistance(SithCog*);
void J3DAPI sithCogFunctionThing_CheckPathToPoint(SithCog*);
void J3DAPI sithCogFunctionThing_MoveThing(SithCog*);
void J3DAPI sithCogFunctionThing_MoveThingToPos(SithCog*);
void J3DAPI sithCogFunctionThing_SetThingStateChange(SithCog*);
void J3DAPI sithCogFunctionThing_StartQuetzAnim(SithCog*);
void J3DAPI sithCogFunctionThing_BoardVehicle(SithCog*);
void J3DAPI sithCogFunctionThing_FadeInTrack(SithCog*);
void J3DAPI sithCogFunctionThing_MakeFairyDust(SithCog*);
void J3DAPI sithCogFunctionThing_MakeFairyDustDeluxe(SithCog*);

int J3DAPI sithCogFunctionThing_RegisterFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_WaitForStop, "waitforstop");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_WaitForAnimStop, "waitforanimstop");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_StopThing, "stopthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_DestroyThing, "destroything");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetHealth, "getthinghealth");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMaxHealth, "getthingmaxhealth");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetHealth, "gethealth");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_HealThing, "healthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingLight, "getthinglight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingLight, "setthinglight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingLight, "thinglight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingLightAnim, "thinglightanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ThingFadeAnim, "thingfadeanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateThing, "creatething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateThingAtPos, "createthingatpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CaptureThing, "capturething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ReleaseThing, "releasething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingVel, "setthingvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AddThingVel, "addthingvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ApplyForce, "applyforce");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_DetachThing, "detachthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAttachFlags, "getattachflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAttachFlags, "getthingattachflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToSurf, "attachthingtosurf");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToThing, "attachthingtothing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetArmedMode, "setarmedmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingFlags, "setthingflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearThingFlags, "clearthingflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_TeleportThing, "teleportthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingType, "setthingtype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCollideType, "setcollidetype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHeadLightIntensity, "setheadlightintensity");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingCurLightMode, "getthingcurlightmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingCurLightMode, "setthingcurlightmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetActorExtraSpeed, "setactorextraspeed");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingPosEx, "setthingposex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxVel, "setthingmaxvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxAngVel, "setthingmaxangvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetJointAngle, "setthingjointangle");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMaxHeadPitch, "setthingmaxheadpitch");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMinHeadPitch, "setthingminheadpitch");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxHeadPitch, "getthingmaxheadpitch");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMinHeadPitch, "getthingminheadpitch");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMaxHeadYaw, "setthingmaxheadyaw");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxHeadYaw, "getthingmaxheadyaw");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingLVec, "setthinglvecpyr");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHeadPYR, "setactorheadpyr");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingAirDrag, "setthingairdrag");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxRotVel, "setthingmaxrotvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMaxHeadVel, "setthingmaxheadvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResetThing, "resetthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_MoveThing, "movething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_MoveThingToPos, "movethingtopos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingType, "getthingtype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingMoving, "isthingmoving");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingMoving, "ismoving");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCurFrame, "getcurframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetGoalFrame, "getgoalframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingParent, "getthingparent");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingSector, "getthingsector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingPos, "getthingpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingPos, "setthingpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingVelocity, "getthingvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingUVec, "getthinguvec");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingLVec, "getthinglvec");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingRVec, "getthingrvec");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingFlags, "getthingflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCollideType, "getcollidetype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetHeadLightIntensity, "getheadlightintensity");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingVisible, "isthingvisible");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingGuid, "getthingguid");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetGuidThing, "getguidthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxVel, "getthingmaxvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxAngVel, "getthingmaxangvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetJointAngle, "getthingjointangle");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_InterpolatePYR, "interpolatepyr");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingLVecPYR, "getthinglvecpyr");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetActorHeadPYR, "getactorheadpyr");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingJointPos, "getthingjointpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingModelName, "isthingmodelname");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxRotVel, "getthingmaxrotvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMaxHeadVel, "getthingmaxheadvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CopyOrient, "copyorient");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CopyOrientAndPos, "copyorientandpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingInsertOffset, "getthinginsertoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingInsertOffset, "setthinginsertoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingEyeOffset, "getthingeyeoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingPulse, "setthingpulse");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingTimer, "setthingtimer");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventory, "getinv");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetInventory, "setinv");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ChangeInventory, "changeinv");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventoryCog, "getinvcog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventoryMinimum, "getinvmin");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetInventoryMaximum, "getinvmax");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayKey, "playkey");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayKeyEx, "playkeyex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_StopKey, "stopkey");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PauseKey, "pausekey");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResumeKey, "resumekey");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingModel, "setthingmodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingModel, "getthingmodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayMode, "playmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_StopMode, "stopmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SynchMode, "synchmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsModePlaying, "ismodeplaying");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PauseMode, "pausemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResumeMode, "resumemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_TrackToMode, "tracktomode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_WaitMode, "waitmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMajorMode, "getmajormode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_FirstThingInSector, "firstthinginsector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_NextThingInSector, "nextthinginsector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PrevThingInSector, "prevthinginsector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_MoveToFrame, "movetoframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SkipToFrame, "skiptoframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_JumpToFrame, "jumptoframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PathMovePause, "pathmovepause");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PathMoveResume, "pathmoveresume");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_Rotate, "rotate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_RotatePivot, "rotatepivot");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_RotateToPYR, "rotatetopyr");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingTemplate, "getthingtemplate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_DamageThing, "damagething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetLifeleft, "setlifeleft");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetLifeleft, "getlifeleft");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingThrust, "setthingthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingThrust, "getthingthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHealth, "setthinghealth");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetHealth, "sethealth");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AmputateJoint, "amputatejoint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetActorWeapon, "setactorweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetActorWeapon, "getactorweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetPhysicsFlags, "getphysicsflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetPhysicsFlags, "setphysicsflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearPhysicsFlags, "clearphysicsflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ParseArg, "parsearg");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingRotVel, "getthingrotvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingRotVel, "setthingrotvel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetRotThrust, "getthingrotthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetRotThrust, "setthingrotthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingLook, "setthinglook");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingHeadLookPos, "setthingheadlookpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingHeadLookThing, "setthingheadlookthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingCrouching, "isthingcrouching");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingCrouching, "iscrouching");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingClassCog, "getthingclasscog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingClassCog, "setthingclasscog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingCaptureCog, "getthingcapturecog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingCaptureCog, "setthingcapturecog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingRespawn, "getthingrespawn");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingSignature, "getthingsignature");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingAttachFlags, "setthingattachflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearThingAttachFlags, "clearthingattachflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetParticleSize, "getparticlesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetParticleSize, "setparticlesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetParticleGrowthSpeed, "getparticlegrowthspeed");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetParticleGrowthSpeed, "setparticlegrowthspeed");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetParticleTimeoutRate, "getparticletimeoutrate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetParticleTimeoutRate, "setparticletimeoutrate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "gettypeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "settypeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "cleartypeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getactorflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setactorflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearactorflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getweaponflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setweaponflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearweaponflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getexplosionflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setexplosionflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearexplosionflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getitemflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setitemflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearitemflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetTypeFlags, "getparticleflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetTypeFlags, "setparticleflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ClearTypeFlags, "clearparticleflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_TakeItem, "takeitem");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_HasLOS, "haslos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetFireOffset, "getthingfireoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetFireOffset, "setthingfireoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingUserData, "getthinguserdata");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingUserData, "setthinguserdata");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingCollideSize, "getthingcollidesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingCollideSize, "setthingcollidesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMoveSize, "getthingmovesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMoveSize, "setthingmovesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingMass, "getthingmass");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMass, "setthingmass");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SyncThingPos, "syncthingpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SyncThingAttachment, "syncthingattachment");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SyncThingState, "syncthingstate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToThingEx, "attachthingtothingex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAttachedThing, "getthingattachedthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMeshByName, "getmeshbyname");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetNodeByName, "getnodebyname");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AttachThingToThingMesh, "attachthingtothingmesh");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_DetachThingMesh, "detachthingmesh");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingMesh, "setthingmesh");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_RestoreThingMesh, "restorethingmesh");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetThingAlpha, "getthingalpha");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingAlpha, "setthingalpha");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCameraFOV, "getcamerafov");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraFOV, "setcamerafov");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_ResetCameraFOV, "resetcamerafov");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraLookInterp, "setcameralookinterp");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraPosInterp, "setcameraposinterp");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraInterpSpeed, "setcamerainterpspeed");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraPosition, "setcameraposition");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_AnimateSpriteSize, "animatespritesize");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetCameraPosition, "getcameraposition");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetCameraFadeThing, "setcamerafadething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamOffset, "setextcamoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamOffsetToThing, "setextcamoffsettothing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamLookOffsetToThing, "setextcamlookoffsettothing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetExtCamLookOffset, "setextcamlookoffset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_RestoreExtCam, "restoreextcam");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_PlayForceMoveMode, "playforcemovemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsThingAutoAiming, "isthingautoaiming"); // TODO: redundant function
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_GetMoveStatus, "getmovestatus");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateLaser, "createlaser");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreateLightning, "createlightning");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_MakeFairyDust, "makefairydust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_MakeFairyDustDeluxe, "makefairydustdeluxe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetPuppetModeFPS, "setpuppetmodefps");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetMoveMode, "setmovemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CheckFloorDistance, "checkfloordistance");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CheckPathToPoint, "checkpathtopoint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_SetThingStateChange, "setthingstatechange");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_CreatePolylineThing, "createpolylinething");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_StartQuetzAnim, "startquetzanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_BoardVehicle, "boardvehicle");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_FadeInTrack, "fadeintrack");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_IsGhostVisible, "isghostvisible");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionThing_MakeCamera2LikeCamera1, "makecamera2likecamera1");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithCogFunctionThing_IsGhostVisible(SithCog* pCog)
{
    float retSin, retCos;
    rdVector3 delta;
    float ghostDot;
    float coneAngle            = sithCogExec_PopFlex(pCog);
    SithThing* pGhostThing = sithCogExec_PopThing(pCog);
    SithThing* pViewer      = sithCogExec_PopThing(pCog);
    if ( !pGhostThing || !pViewer )
    {
#line 844
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing ref in IsGhostVisible()!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( coneAngle < 1.0f || coneAngle > 90.0f )
    {
#line 851
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid range angle in IsGhostVisible()!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    stdMath_SinCos(coneAngle, &retSin, &retCos);
    rdVector_Sub3(&delta, &pGhostThing->pos, &pViewer->pos);
    rdVector_Normalize3Acc(&delta);

    ghostDot = rdVector_Dot3(&pViewer->orient.lvec, &delta);
    if ( ghostDot >= retCos )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionThing_SetThingAlpha(SithCog* pCog)
{
    rdVector4 color;
    float alpha       = sithCogExec_PopFlex(pCog);
    SithThing* pObj = sithCogExec_PopThing(pCog);
    if ( !pObj )
    {
#line 878
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing ref in SetThingAlpha()!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    alpha = STDMATH_CLAMP(alpha, 0.0f, 1.0f);

    if ( pObj->type == SITH_THING_POLYLINE )
    {
        memcpy(&color, &pObj->renderData.data.pPolyline->face.extraLight, sizeof(color));
        color.alpha = alpha;
        memcpy(&pObj->renderData.data.pPolyline->face.extraLight, &color, sizeof(color));
    }

    pObj->alpha = alpha;
    sithCogExec_PushFlex(pCog, alpha);
}

void J3DAPI sithCogFunctionThing_GetThingAlpha(SithCog* pCog)
{
    float alpha;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 906
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing ref in SetThingAlpha()!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }

    alpha = pThing->alpha;
    sithCogExec_PushFlex(pCog, alpha);
}

void J3DAPI sithCogFunctionThing_CreatePolylineThing(SithCog* pCog)
{
    SithThing* pLine;
    rdVector3 endPos;
    float beamTime        = sithCogExec_PopFlex(pCog);
    float tipRadius       = sithCogExec_PopFlex(pCog);
    float baseRadius      = sithCogExec_PopFlex(pCog);
    rdMaterial* pLineMaterial = sithCogExec_PopMaterial(pCog);
    int bValid              = sithCogExec_PopVector(pCog, &endPos);
    SithThing* pDestThing = sithCogExec_TryPopThing(pCog);
    SithThing* pSrcThing  = sithCogExec_PopThing(pCog);
    if ( !bValid )
    {
#line 936
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid vector in CreatePolyLineThing!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pSrcThing )
    {
#line 943
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid source thing in CreatePolyLineThing!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pLine = sithFX_CreatePolylineThing(pSrcThing, pDestThing, &endPos, pLineMaterial, baseRadius, tipRadius, beamTime);
        if ( pLine )
        {
            sithCogExec_PushInt(pCog, pLine->idx);
        }
        else
        {
#line 957
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create poly line from thing %s to thing %s CreatePolyLineThing!\n", pCog->aName, pSrcThing->aName, pDestThing->aName, 0);
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetMoveStatus(SithCog* pCog)
{
    SithThingMoveStatus status;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 972
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing ref if GetMoveStatus()!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    status = pThing->moveStatus;
    sithCogExec_PushInt(pCog, status);
}

void J3DAPI sithCogFunctionThing_CopyOrient(SithCog* pCog)
{
    SithThing* pSrcThing;
    SithThing* pDestThing = sithCogExec_PopThing(pCog);
    if ( !pDestThing )
    {
#line 990
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid destination thing in CopyOrient!\n", pCog->aName, 0, 0, 0);
        return;
    }

    pSrcThing = sithCogExec_PopThing(pCog);
    if ( !pSrcThing )
    {
#line 997
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid source thing in CopyOrient!\n", pCog->aName, 0, 0, 0);
        return;
    }

    memcpy(&pDestThing->orient, &pSrcThing->orient, sizeof(pDestThing->orient));
}

void J3DAPI sithCogFunctionThing_CopyOrientAndPos(SithCog* pCog)
{
    SithThing* pDstThing = sithCogExec_PopThing(pCog);
    SithThing* pSrcThing = sithCogExec_PopThing(pCog);

    if ( !pDstThing )
    {
#line 1016
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid destination thing in CopyOrientAndPos!\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pSrcThing )
    {
#line 1022
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid source thing in CopyOrientAndPos!\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pSrcThing->pInSector )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Source thing %s isn't in any sector in CopyOrient!\n", pCog->aName, pSrcThing->aName, 0, 0); // Retail text
        return;
    }

    memcpy(&pDstThing->orient, &pSrcThing->orient, sizeof(pDstThing->orient));
    memcpy(&pDstThing->pos, &pSrcThing->pos, sizeof(pDstThing->pos));

    sithThing_ExitSector(pDstThing);
    sithThing_EnterSector(pDstThing, pSrcThing->pInSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/1);
}

void J3DAPI sithCogFunctionThing_CreateLaser(SithCog* pCog)
{
    rdVector3 startOffset;
    SithThing* pSrcThing;
    SithThing* pBeam;
    rdVector3 endPoint;
    int bVec;
    int bArgError    = 0;
    float beamTime   = sithCogExec_PopFlex(pCog);
    float endRadius  = sithCogExec_PopFlex(pCog);
    float baseRadius = sithCogExec_PopFlex(pCog);

    bVec = sithCogExec_PopVector(pCog, &endPoint);
    if ( !bVec )
    {
#line 1056
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid end pos vector in CreateLaser!\n", pCog->aName, 0, 0, 0);
        bArgError = 1;
    }

    bVec = sithCogExec_PopVector(pCog, &startOffset);
    if ( !bVec )
    {
#line 1063
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid offset vector in CreateLaser!\n", pCog->aName, 0, 0, 0);
        bArgError = 1;
    }

    pSrcThing = sithCogExec_PopThing(pCog);
    if ( !pSrcThing )
    {
#line 1070
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid source thing in CreateLaser!\n", pCog->aName, 0, 0, 0);
        bArgError = 1;
    }

    if ( bArgError == 1 )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pBeam = sithFX_CreateLaserThing(pSrcThing, &startOffset, &endPoint, baseRadius, endRadius, beamTime);
        if ( pBeam )
        {
            sithCogExec_PushInt(pCog, pBeam->idx);
        }
        else
        {
#line 1088
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create laser from thing %s in CreateLaser!\n", pCog->aName, pSrcThing->aName, 0, 0);
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_CreateLightning(SithCog* pCog)
{
    rdVector3 startOffset;
    SithThing* pSrcThing;
    SithThing* pBeam;
    rdVector3 endPoint;
    int bVec;
    int bArgError    = 0;
    float beamTime   = sithCogExec_PopFlex(pCog);
    float endRadius  = sithCogExec_PopFlex(pCog);
    float baseRadius = sithCogExec_PopFlex(pCog);

    bVec = sithCogExec_PopVector(pCog, &endPoint);
    if ( !bVec )
    {
#line 1109
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid end pos vector in CreateLightning!\n", pCog->aName, 0, 0, 0);
        bArgError = 1;
    }

    bVec = sithCogExec_PopVector(pCog, &startOffset);
    if ( !bVec )
    {
#line 1116
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid offset vector in CreateLightning!\n", pCog->aName, 0, 0, 0);
        bArgError = 1;
    }

    pSrcThing = sithCogExec_PopThing(pCog);
    if ( !pSrcThing )
    {
#line 1123
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid source thing in CreateLightning!\n", pCog->aName, 0, 0, 0);
        bArgError = 1;
    }

    if ( bArgError == 1 )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pBeam = sithFX_CreateLightningThing(pSrcThing, &startOffset, &endPoint, baseRadius, endRadius, beamTime);
        if ( pBeam )
        {
            sithCogExec_PushInt(pCog, pBeam->idx);
        }
        else
        {
#line 1141
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create lightning from thing %s in CreateLightning!\n", pCog->aName, pSrcThing->aName, 0, 0);
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_AnimateSpriteSize(SithCog* pCog)
{
    SithAnimationSlot* pAnimNew;
    rdVector3 vecEnd;
    rdVector3 startVec;
    float deltaTime    = sithCogExec_PopFlex(pCog);
    int bVecEnd        = sithCogExec_PopVector(pCog, &vecEnd);
    int bGotStart      = sithCogExec_PopVector(pCog, &startVec);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !bGotStart )
    {
#line 1163
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid start vector in AnimateSpriteSize!\n", pCog->aName, 0, 0, 0);
    }
    else if ( !bVecEnd )
    {
#line 1169
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid end vector in AnimateSpriteSize!\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pThing )
    {
#line 1175
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in AnimateSpriteSize!\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pAnimNew = sithAnimate_StartSpriteSizeAnim(pThing, &startVec, &vecEnd, deltaTime);
        if ( pAnimNew )
        {
            sithCogExec_PushInt(pCog, pAnimNew->animID);
            return;
        }
    }

#line 1188
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create animation for AnimateSpriteSize().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_GetThingType(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->type);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_IsThingAutoAiming(SithCog* pCog)
{
    // TODO: redundant function
}

void J3DAPI sithCogFunctionThing_PlayForceMoveMode(SithCog* pCog)
{
    int trackNum;
    int bPlaying = 0;
    SithPuppetSubMode submode = sithCogExec_PopInt(pCog);
    SithThing* pThing         = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1272
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing in PlayForceMoveMode()\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    switch ( submode )
    {
        case SITHPUPPETSUBMODE_HOPBACK:
            sithPlayerActions_HopBack(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_JUMPUP:
            sithPlayerActions_Jump(pThing, 0.6f, 0);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_JUMPFWD:
            sithPlayerActions_Jump(pThing, 0.77f, 1);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_PUSHITEM:
            pThing->moveStatus = SITHPLAYERMOVE_PUSHING;
            break;

        case SITHPUPPETSUBMODE_PULLITEM:
            pThing->moveStatus = SITHPLAYERMOVE_PULLING;
            break;

        case SITHPUPPETSUBMODE_HANGSHIMLEFT:
        case SITHPUPPETSUBMODE_HANGSHIMRIGHT:
            pThing->moveStatus = SITHPLAYERMOVE_HANGING;
            break;

        case SITHPUPPETSUBMODE_CLIMBWALLUP:
        case SITHPUPPETSUBMODE_CLIMBWALLDOWN:
        case SITHPUPPETSUBMODE_CLIMBWALLLEFT:
        case SITHPUPPETSUBMODE_CLIMBWALLRIGHT:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBIDLE;
            break;

        case SITHPUPPETSUBMODE_CLIMBPULLINGUP:
        case SITHPUPPETSUBMODE_MOUNTFROMWATER:
            pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP;
            break;

        case SITHPUPPETSUBMODE_DIVEFROMSURFACE:
            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
            break;

        case SITHPUPPETSUBMODE_MOUNT1MSTEP:
            sithPlayerActions_ClimbOn1m(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_MOUNT2MLEDGE:
            sithPlayerActions_ClimbOn2m(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_JUMPROLLBACK:
            sithPlayerActions_JumpRollBack(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_JUMPROLLFWD:
            sithPlayerActions_JumpRollForward(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_LEAP:
            pThing->moveStatus = SITHPLAYERMOVE_JUMPFWD;
            rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
            sithThing_DetachThing(pThing);
            trackNum     = sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_LEAP, NULL);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_HOPLEFT:
            sithPlayerActions_JumpLeft(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_HOPRIGHT:
            sithPlayerActions_JumpRight(pThing);
            bPlaying = 1;
            break;

        case SITHPUPPETSUBMODE_PUSHPULLREADY:
        case SITHPUPPETSUBMODE_WHIPSWING:
            if ( pThing->type != SITH_THING_PLAYER )
            {
                rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
                break;
            }

            // fall through
        default:
            sithCogExec_PushInt(pCog, -1);
            return;
    }

    if ( !bPlaying )
    {
        trackNum = sithPuppet_PlayForceMoveMode(pThing, submode, NULL);
    }

    sithCogExec_PushInt(pCog, trackNum);
}

void J3DAPI sithCogFunctionThing_SetMoveMode(SithCog* pCog)
{
    int newMode       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 1378
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing in SetMoveMode()\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( newMode < 0 || newMode > 107 )
    {
#line 1384
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad move mode for thing %s in SetMoveMode()\n", pThing->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithPuppet_SetMoveMode(pThing, (SithPuppetMoveMode)newMode);
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionThing_CreateThing(SithCog* pCog)
{
    SithThing* pNewThing;
    SithThing* pMarker   = sithCogExec_PopThing(pCog);
    SithThing* pTemplate = sithCogExec_PopTemplate(pCog);
    if ( !pMarker )
    {
#line 1405
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing reference in CreateThing.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( pMarker->type == SITH_THING_FREE )
    {
#line 1412
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s has been deleted from the world in CreateThing.\n", pCog->aName, pMarker->aName, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pMarker->pInSector )
    {
#line 1419
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not in any sector in CreateThing.\n", pCog->aName, pMarker->aName, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pTemplate )
    {
#line 1426
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad template reference in CreateThing.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pNewThing = sithThing_CreateThing(pTemplate, pMarker);
        if ( pNewThing )
        {
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithDSSThing_CreateThing(pTemplate, pNewThing, pMarker, NULL, NULL, NULL, 0xFFu, DPSEND_GUARANTEED);
                }
            }

            pNewThing->moveStatus = SITHPLAYERMOVE_STILL;
            sithCogExec_PushInt(pCog, pNewThing->idx);
        }
        else
        {
#line 1445
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: CreateThing failed.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_CreateThingAtPos(SithCog* pCog)
{
    rdMatrix34 orient;
    rdVector3 off;
    SithThing* pResult;
    rdVector3 pos, pyr;
    int bPyrValid          = sithCogExec_PopVector(pCog, &pyr);
    int bPositionValid          = sithCogExec_PopVector(pCog, &pos);
    SithSector* curSector  = sithCogExec_PopSector(pCog);
    SithThing* pTemplate = sithCogExec_PopTemplate(pCog);
    if ( !bPyrValid || !bPositionValid )
    {
#line 1493
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad vector in CreateThingAtPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pTemplate )
    {
#line 1500
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad template in CreateThingAtPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !curSector )
    {
#line 1507
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector in CreateThingAtPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        switch ( pTemplate->renderData.type )
        {
            case RD_THING_MODEL3:
                memcpy(&off, &pTemplate->renderData.data.pModel3->insertOffset, sizeof(off));
                break;

            case RD_THING_SPRITE3:
                memcpy(&off, &pTemplate->renderData.data.pSprite3->offset, sizeof(off));
                break;

            default:
                memset(&off, 0, sizeof(off));
                break;
        }

        rdMatrix_BuildRotate34(&orient, &pyr);
        rdMatrix_TransformVector34Acc(&off, &orient);
        rdVector_Add3Acc(&pos, &off);

        pResult = sithThing_CreateThingAtPos(pTemplate, &pos, &orient, curSector, NULL);
        if ( pResult )
        {
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithDSSThing_CreateThing(pTemplate, pResult, NULL, curSector, &pos, &pyr, 0xFFu, DPSEND_GUARANTEED);
                }
            }

            sithCogExec_PushInt(pCog, pResult->idx);
        }
        else
        {
#line 1547
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create thing in CreateThingAtPos.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_DamageThing(SithCog* pCog)
{
    SithThing* pCurDamageThing    = sithCogExec_PopThing(pCog);
    SithDamageType curdamageclass = sithCogExec_PopInt(pCog);
    float damage               = sithCogExec_PopFlex(pCog);
    SithThing* pThing          = sithCogExec_PopThing(pCog);

    if ( damage <= 0.0f || !pThing )
    {
    }
    else
    {
        if ( !pCurDamageThing )
        {
            pCurDamageThing = pThing;
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                if ( stdComm_IsGameHost() )
                {
                    sithDSSThing_DamageThing(pThing, pCurDamageThing, damage, curdamageclass, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_NET);
                }
            }
        }

        damage = sithThing_DamageThing(pThing, pCurDamageThing, damage, curdamageclass);
        sithCogExec_PushFlex(pCog, damage);
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_HealThing(SithCog* pCog)
{
    float health      = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( health <= 0.0f || !pThing || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) )
    {
        return;
    }

    pThing->thingInfo.actorInfo.health += health;
    if ( pThing->thingInfo.actorInfo.health > pThing->thingInfo.actorInfo.maxHealth )
    {
        pThing->thingInfo.actorInfo.health = pThing->thingInfo.actorInfo.maxHealth;
    }
}

void J3DAPI sithCogFunctionThing_GetHealth(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) )
    {
        if ( !pThing )
        {
#line 1648
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetHealth.\n", pCog->aName, 0, 0, 0);
        }
        else
        {
#line 1650
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not an actor or player in GetHealth.\n", pCog->aName, pThing->aName, 0, 0);
        }

        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.health);
}

void J3DAPI sithCogFunctionThing_GetMaxHealth(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) )
    {
        if ( !pThing )
        {
#line 1681
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetMaxHealth.\n", pCog->aName, 0, 0, 0);
        }
        else
        {
#line 1683
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not an actor or player in GetMaxHealth.\n", pCog->aName, pThing->aName, 0, 0);
        }

        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHealth);
}

void J3DAPI sithCogFunctionThing_SetHealth(SithCog* pCog)
{
    float health      = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) )
    {
        return;
    }

    pThing->thingInfo.actorInfo.health = health;
}

void J3DAPI sithCogFunctionThing_DestroyThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        return;
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_DestroyThing(pThing->guid, SITHMESSAGE_SENDTOJOINEDPLAYERS);
        }
    }

    sithThing_DestroyThing(pThing);
}

void J3DAPI sithCogFunctionThing_JumpToFrame(SithCog* pCog)
{
    SithSector* pNewSector = sithCogExec_PopSector(pCog);
    size_t frame           = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || !pNewSector || pThing->moveType != SITH_MT_PATH || frame >= pThing->moveInfo.pathMovement.numFrames )
    {
#line 1792
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal params in JumpToFrame.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->pInSector && pNewSector != pThing->pInSector )
    {
        sithThing_ExitSector(pThing);
    }

    if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    rdMatrix_BuildRotate34(&pThing->orient, &pThing->moveInfo.pathMovement.aFrames[frame].pyr);
    memcpy(&pThing->pos, &pThing->moveInfo.pathMovement.aFrames[frame].pos, sizeof(pThing->pos));

    if ( !pThing->pInSector )
    {
        sithThing_EnterSector(pThing, pNewSector, /*bNoWaterSplash=*/1, /*bNoNotify=*/0);
    }
}

void J3DAPI sithCogFunctionThing_MoveToFrame(SithCog* pCog)
{
    size_t frame;
    SithThing* pThing;
    float curSpeed = sithCogExec_PopFlex(pCog);
    curSpeed       = RD_MULTIPLY(curSpeed, 0.1f);
    frame       = sithCogExec_PopInt(pCog);
    pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 1853
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing pointer is NULL in MoveToFrame\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
#line 1858
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not a path move thing in MoveToFrame\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( pThing->moveInfo.pathMovement.numFrames <= frame )
    {
#line 1863
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s, moveFrame (%d) > path's frames (%d)\n", pCog->aName, pThing->aName, frame, pThing->moveInfo.pathMovement.numFrames);
        return;
    }

    if ( curSpeed == 0.0f )
    {
#line 1875
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad movement speed set for movetoframe.  Set=0.5\n", pCog->aName, 0, 0, 0);
        curSpeed = 0.5f;
    }

    sithPathMove_MoveToFrame(pThing, frame, curSpeed);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PathMove(pThing, frame, curSpeed, 0, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

void J3DAPI sithCogFunctionThing_SkipToFrame(SithCog* pCog)
{
    size_t frame;
    SithThing* pThing;
    float curSpeed = sithCogExec_PopFlex(pCog);
    curSpeed       = RD_MULTIPLY(curSpeed, 0.1f);
    frame       = sithCogExec_PopInt(pCog);
    pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PATH || pThing->moveInfo.pathMovement.numFrames <= frame )
    {

#line 1925
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal params in MoveToFrame call from COG.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( curSpeed == 0.0f )
    {
#line 1931
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad movement speed set for movetoframe.  Set=0.5\n", pCog->aName, 0, 0, 0);
        curSpeed = 0.5f;
    }

    sithPathMove_SkipToFrame(pThing, frame, curSpeed);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_PathMove(pThing, frame, curSpeed, 1, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

void J3DAPI sithCogFunctionThing_RotatePivot(SithCog* pCog)
{
    float rotTime     = sithCogExec_PopFlex(pCog);
    size_t frame      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( rotTime == 0.0f )
    {
        rotTime = 1.0f;
    }

    if ( !pThing || pThing->moveType != SITH_MT_PATH || pThing->moveInfo.pathMovement.numFrames <= frame )
    {
#line 1982
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal parameters in Rotate call from COG.\n", pCog->aName, 0, 0, 0);
    }
    else if ( rotTime > 0.0f )
    {
        sithPathMove_RotatePivot(pThing, &pThing->moveInfo.pathMovement.aFrames[frame].pos, &pThing->moveInfo.pathMovement.aFrames[frame].pyr, rotTime);
    }
    else
    {
        rdVector3 rot;
        rdVector_Neg3(&rot, &pThing->moveInfo.pathMovement.aFrames[frame].pyr);
        sithPathMove_RotatePivot(pThing, &pThing->moveInfo.pathMovement.aFrames[frame].pos, &rot, -rotTime);
    }
}

void J3DAPI sithCogFunctionThing_Rotate(SithCog* pCog)
{
    rdVector3 offset;
    rdVector3 pyrOrient;
    rdVector3 pivot;
    float rotTime     = sithCogExec_PopFlex(pCog);
    size_t axis       = sithCogExec_PopInt(pCog);
    float degrees     = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 2037
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in Rotate.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
#line 2043
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not a pathmove object in Rotate.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( axis >= 3 )
    {
#line 2049
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s invalid rotation axis %d in Rotate.\n", pCog->aName, pThing->aName, axis, 0);
        return;
    }

    if ( degrees == 0.0f )
    {
#line 2055
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NOTE: Cog %s: Thing %s being rotated 0 degrees in Rotate.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( rotTime == 0.0f )
    {
#line 2061
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s given rotation time of 0 in Rotate.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( pThing->renderData.data.pModel3 )
    {
        memcpy(&offset, &pThing->renderData.data.pModel3->insertOffset, sizeof(offset));
    }
    else
    {
        memset(&offset, 0, sizeof(offset));
    }

    rdMatrix_TransformVector34Acc(&offset, &pThing->orient);
    rdVector_Sub3(&pivot, &pThing->pos, &offset);

    switch ( axis )
    {
        case 0:
            rdVector_Set3(&pyrOrient, -degrees, 0.0f, 0.0f);
            break;

        default:
            rdVector_Set3(&pyrOrient, 0.0f, -degrees, 0.0f);
            break;

        case 2:
            rdVector_Set3(&pyrOrient, 0.0f, 0.0f, -degrees);
            break;
    }

    sithPathMove_RotatePivot(pThing, &pivot, &pyrOrient, rotTime);
}

void J3DAPI sithCogFunctionThing_RotateToPYR(SithCog* pCog)
{
    rdVector3 curpyr;
    float time        = sithCogExec_PopFlex(pCog);
    int bPyr          = sithCogExec_PopVector(pCog, &curpyr);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 2131
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in RotateToPYR.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
#line 2137
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not a pathmove object in RotateToPYR.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( !bPyr )
    {
#line 2143
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid vector applied to thing %s in RotateToPYR call from COG.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( time <= 0.0f )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Time must be greater than zero for thing %s in RotateToPYR call from COG.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    sithPathMove_RotateToPYR(pThing, &curpyr, time);
}

void J3DAPI sithCogFunctionThing_GetThingLight(SithCog* pCog)
{
    rdVector3 color;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        goto error;
    }

    color.red   = pThing->light.color.red;
    color.green = pThing->light.color.green;
    color.blue  = pThing->light.color.blue;
    sithCogExec_PushVector(pCog, &color);
    return;

error:
#line 2189
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to GetThingLight().\n", pCog->aName, 0, 0, 0);
    color.red = color.green = color.blue = -1.0f;
    sithCogExec_PushVector(pCog, &color);
}

void J3DAPI sithCogFunctionThing_SetExtCamOffset(SithCog* pCog)
{
    rdVector3 vec;
    sithCogExec_PopVector(pCog, &vec);

    sithCamera_SetExtCameraOffset(&vec);
}

void J3DAPI sithCogFunctionThing_SetExtCamOffsetToThing(SithCog* pCog)
{
    rdVector3 offset;
    rdVector3 pyr;

    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 2213
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad THING in SetExtCamOffsetToThing() call.\n", pCog->aName, 0, 0, 0);
        return;
    }


    rdVector_Sub3(&offset, &pThing->pos, &sithPlayer_pLocalPlayerThing->pos);


    rdMatrix_ExtractAngles34(&sithPlayer_pLocalPlayerThing->orient, &pyr);

    pyr.pitch = 0.0f;
    pyr.yaw   = -pyr.y;
    pyr.roll  = 0.0f;
    rdVector_Rotate3Acc(&offset, &pyr);

    sithCamera_SetExtCameraOffset(&offset);
}

void J3DAPI sithCogFunctionThing_SetExtCamLookOffset(SithCog* pCog)
{
    rdVector3 vec;
    sithCogExec_PopVector(pCog, &vec);

    sithCamera_SetExtCameraLookOffset(&vec);
}

void J3DAPI sithCogFunctionThing_SetExtCamLookOffsetToThing(SithCog* pCog)
{
    rdVector3 offset;
    rdVector3 pyr;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 2248
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad THING in SetExtCamLookOffsetToThing() call.\n", pCog->aName, 0, 0, 0);
        return;
    }


    rdVector_Sub3(&offset, &pThing->pos, &sithPlayer_pLocalPlayerThing->pos);


    rdMatrix_ExtractAngles34(&sithPlayer_pLocalPlayerThing->orient, &pyr);

    pyr.pitch = 0.0f;
    pyr.yaw   = -pyr.y;
    pyr.roll  = 0.0f;
    rdVector_Rotate3Acc(&offset, &pyr);

    sithCamera_SetExtCameraLookOffset(&offset);
}

void J3DAPI sithCogFunctionThing_MakeCamera2LikeCamera1(SithCog* pCog)
{
    SithSector* pLookRoom;
    SithSector* pPosRoom;
    rdVector3 lookVec;
    rdVector3 posVec;
    SithThing* pCam1Look = sithCogExec_PopThing(pCog);
    SithThing* pPosThing  = sithCogExec_PopThing(pCog);
    if ( !sithPlayer_pLocalPlayerThing || !sithPlayer_pLocalPlayerThing->pInSector )
    {
#line 2276
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: no local player or player not in valid sector!\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
    }
    else if ( !pCam1Look || !pPosThing )
    {
#line 2282
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog &s: Bad THING in MakeCamera2LikeCamera1() call.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
    }
    else
    {
        memcpy(&posVec, &sithCamera_camSpot, sizeof(posVec));
        memcpy(&lookVec, &sithCamera_camLookSpot, sizeof(lookVec));
        pPosRoom = sithCollision_FindSectorInRadius(sithPlayer_pLocalPlayerThing->pInSector, &sithPlayer_pLocalPlayerThing->pos, &posVec, 0.0f);
        if ( !pPosRoom )
        {
#line 2293
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog &s: pos thing %s endpoint not in valid sector in MakeCamera2LikeCamera1() call.\n", pCog->aName, pPosThing->aName, 0, 0);
            sithCogExec_PushInt(pCog, 0);
        }
        else
        {
            pLookRoom = sithCollision_FindSectorInRadius(sithPlayer_pLocalPlayerThing->pInSector, &sithPlayer_pLocalPlayerThing->pos, &lookVec, 0.0f);
            if ( !pLookRoom )
            {
#line 2302
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog &s: look thing %s endpoint not in valid sector in MakeCamera2LikeCamera1() call.\n", pCog->aName, pCam1Look->aName, 0, 0);
                sithCogExec_PushInt(pCog, 0);
            }
            else
            {
                memcpy(&pPosThing->pos, &posVec, sizeof(pPosThing->pos));
                memcpy(&pCam1Look->pos, &lookVec, sizeof(pCam1Look->pos));
                if ( pPosRoom != pPosThing->pInSector )
                {
                    if ( pPosThing->pInSector )
                    {
                        sithThing_ExitSector(pPosThing);
                    }

                    sithThing_EnterSector(pPosThing, pPosRoom, 1, 1);
                }

                if ( pLookRoom != pCam1Look->pInSector )
                {
                    if ( pCam1Look->pInSector )
                    {
                        sithThing_ExitSector(pCam1Look);
                    }

                    sithThing_EnterSector(pCam1Look, pLookRoom, 1, 1);
                }

                sithCogExec_PushInt(pCog, 1);
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_RestoreExtCam(SithCog* pCog)
{
    sithCamera_RestoreExtCamera();
}

void J3DAPI sithCogFunctionThing_SetCameraPosition(SithCog* pCog)
{
    SithCamera* pCamera;
    rdVector3 vec;
    int camNum;
    sithCogExec_PopVector(pCog, &vec);
    camNum = sithCogExec_PopInt(pCog);

    if ( camNum > -1 && camNum < 7 )
    {
        pCamera = &sithCamera_aCameras[camNum];
        sithCamera_SetCameraPosition(pCamera, &vec);
    }
}

void J3DAPI sithCogFunctionThing_SetCameraLookInterp(SithCog* pCog)
{
    SithCamera* pCamera;
    int bEnable = sithCogExec_PopInt(pCog);
    int camNum  = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < 7 )
    {
        pCamera = &sithCamera_aCameras[camNum];
        if ( bEnable != pCamera->bLookInterp )
        {
            pCamera->lookInterpState = -1;
        }

        pCamera->bLookInterp = bEnable;
    }
    else
    {
#line 2373
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera in SetCameraLookInterp().\n", pCog->aName, 0, 0, 0);
    }
}

// Function enables/disables dolly mode for specified camera.
//
// Dolly mode: when camera primary focus is changed the camera will interpolate move to the new focused thing instead of teleport to it.
void J3DAPI sithCogFunctionThing_SetCameraPosInterp(SithCog* pCog)
{
    SithCamera* pCam;
    int bDollyMode = sithCogExec_PopInt(pCog);
    int camNum     = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < 7 )
    {
        pCam = &sithCamera_aCameras[camNum];
        if ( bDollyMode != pCam->bPosInterp )
        {
            pCam->posInterpState = -1;
        }

        pCam->bPosInterp = bDollyMode;
    }
    else
    {
#line 2397
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera in SetCameraPosInterp().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetCameraFOV(SithCog* pCog)
{
    SithCamera* pCamera = sithCamera_pCurCamera;
    sithCogExec_PushFlex(pCog, pCamera->fov);
}

void J3DAPI sithCogFunctionThing_GetCameraPosition(SithCog* pCog)
{
    rdVector3 pos;
    int camNum = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < 7 )
    {
        SithCamera* pCamera;

        pCamera = &sithCamera_aCameras[camNum];
        sithCamera_GetCameraPosition(pCamera, &pos);
    }
    else
    {
#line 2436
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera number %d in GetCameraPosition().\n", pCog->aName, camNum, 0, 0);
        rdVector_Set3(&pos, 0.0f, 0.0f, 0.0f);
    }

    sithCogExec_PushVector(pCog, &pos);
}

void J3DAPI sithCogFunctionThing_SetCameraInterpSpeed(SithCog* pCog)
{
    float speed = sithCogExec_PopFlex(pCog);
    SithCamera* currentCamera;
    int camNum  = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < 7 )
    {
        currentCamera = &sithCamera_aCameras[camNum];
        currentCamera->interpSpeed = speed;
    }
    else
    {
#line 2459
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera in SetCameraInterpSpeed().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_SetCameraFOV(SithCog* pCog)
{
    float timeDelta     = sithCogExec_PopFlex(pCog);
    int bCurrentInterp         = sithCogExec_PopInt(pCog);
    float fov           = sithCogExec_PopFlex(pCog);
    SithCamera* pCamera = sithCamera_pCurCamera;
    if ( !bCurrentInterp || timeDelta <= 0.0f )
    {
        if ( bCurrentInterp && timeDelta <= 0.0f )
        {
#line 2489
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid interp time in SetCameraFOV().\n", pCog->aName, 0, 0, 0);
        }

        sithCamera_SetCameraFOV(pCamera, fov);
    }
    else
    {
        sithAnimate_CameraZoom(pCamera, fov, timeDelta);
    }
}

void J3DAPI sithCogFunctionThing_ResetCameraFOV(SithCog* pCog)
{
    float fov             = SITHCAMERA_FOVDEFAULT;
    float time            = sithCogExec_PopFlex(pCog);
    int bAnimate           = sithCogExec_PopInt(pCog);
    SithCamera* pCam   = sithCamera_pCurCamera;
    if ( !bAnimate || time <= 0.0f )
    {
        if ( bAnimate && time <= 0.0f )
        {
#line 2526
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid interp time in ResetCameraFOV().\n", pCog->aName, 0, 0, 0);
        }

        sithCamera_SetCameraFOV(pCam, fov);
    }
    else
    {
        sithAnimate_CameraZoom(pCam, fov, time);
    }
}

void J3DAPI sithCogFunctionThing_SetCameraFadeThing(SithCog* pCog)
{
    rdVector3 pyr;
    SithCamera* pCamera;
    SithSector* pDstSector;
    int bBackside                   = sithCogExec_PopInt(pCog);
    SithThing* pPrimaryThing   = sithCogExec_PopThing(pCog);
    SithThing* pSecondaryFocusThing = sithCogExec_PopThing(pCog);
    int camNum                      = sithCogExec_PopInt(pCog);
    if ( !pSecondaryFocusThing )
    {
#line 2556
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad fade thing in SetCameraFadeThing() call.\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pSecondaryFocusThing )
    {
#line 2562
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad ghost thing in SetCameraFadeThing() call.\n", pCog->aName, 0, 0, 0);
    }
    else if ( camNum < 0 || camNum >= 7 )
    {
#line 2568
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad camera number in SetCameraFadeThing() call.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pCamera = &sithCamera_aCameras[camNum];
        if ( pCamera->bPosInterp )
        {
            pCamera->posInterpState = -1;
        }

        pCamera->bPosInterp = 0;
        if ( pCamera->bLookInterp )
        {
            pCamera->lookInterpState = -1;
        }

        pCamera->bLookInterp = 0;
        sithCamera_SetCameraPosition(pCamera, &sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos);
        sithCamera_SetCameraFOV(pCamera, sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].fov);
        memcpy(&pCamera->orient, &sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].orient, sizeof(pCamera->orient));
        memcpy(&pSecondaryFocusThing->orient, &pCamera->orient, sizeof(pSecondaryFocusThing->orient));
        pSecondaryFocusThing->pos.x = sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos.x + D3DVAL(RD_MULTIPLY(pSecondaryFocusThing->orient.lvec.x, 0.02));
        pSecondaryFocusThing->pos.y = sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos.y + D3DVAL(RD_MULTIPLY(pSecondaryFocusThing->orient.lvec.y, 0.02));
        pSecondaryFocusThing->pos.z = sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos.z + D3DVAL(RD_MULTIPLY(pSecondaryFocusThing->orient.lvec.z, 0.02));
        pDstSector = sithCollision_FindSectorInRadius(sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].pSector, &sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos, &pSecondaryFocusThing->pos, 0.0f);
        sithThing_SetSector(pSecondaryFocusThing, pDstSector, 0);
        memcpy(&pPrimaryThing->orient, &pSecondaryFocusThing->orient, sizeof(pPrimaryThing->orient));
        memcpy(&pPrimaryThing->pos, &sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].lookPos, sizeof(pPrimaryThing->pos));
        sithThing_SetSector(pPrimaryThing, sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].pSector, 0);
        if ( !bBackside )
        {
            rdVector_Set3(&pyr, 0.0f, 180.0f, 0.0f);
            rdMatrix_PreRotate34(&pSecondaryFocusThing->orient, &pyr);
        }

        sithCamera_SetCameraFocus(pCamera, pPrimaryThing, pSecondaryFocusThing);
    }
}

void J3DAPI sithCogFunctionThing_ThingLight(SithCog* pCog)
{
    rdVector4 color;
    float litupTime   = sithCogExec_PopFlex(pCog);
    float range       = sithCogExec_PopFlex(pCog);
    int bCurVector          = sithCogExec_PopVector(pCog, (rdVector3*)&color);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !bCurVector )
    {
        goto error;
    }

    if ( !pThing )
    {
        goto error;
    }

    if ( litupTime == 0.0f )
    {
        pThing->light.color.red   = color.red;
        pThing->light.color.green = color.green;
        pThing->light.color.blue  = color.blue;
        pThing->light.minRadius   = range;
        pThing->light.maxRadius   = 1.1f * range;
        if ( color.red > 0.0f || color.green > 0.0f || color.blue > 0.0f )
        {
            pThing->flags |= SITH_TF_EMITLIGHT;
        }
    }
    else
    {
        color.alpha = range;
        sithAnimate_StartThingLightAnim(pThing, &color, litupTime, (SithAnimateFlags)0);
    }

    return;

error:
#line 2664
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to ThingLight().\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionThing_ThingLightAnim(SithCog* pCog)
{
    SithAnimationSlot* pAnim;
    rdVector4 colorStart, targetColor;
    float speed       = sithCogExec_PopFlex(pCog);
    float targetRadius   = sithCogExec_PopFlex(pCog);
    int bEndColor     = sithCogExec_PopVector(pCog, (rdVector3*)&targetColor);
    float startRange = sithCogExec_PopFlex(pCog);
    int bColor        = sithCogExec_PopVector(pCog, (rdVector3*)&colorStart);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !bEndColor || !bColor || speed <= 0.0f )
    {
#line 2698
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad parameters to ThingLightAnim.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        colorStart.alpha          = startRange;
        targetColor.alpha            = targetRadius;
        pThing->light.color.red   = colorStart.red;
        pThing->light.color.green = colorStart.green;
        pThing->light.color.blue  = colorStart.blue;
        pThing->light.color.alpha = colorStart.alpha;
        pThing->light.minRadius   = startRange;
        pThing->light.maxRadius   = RD_MULTIPLY(1.1f, startRange);
        pAnim = sithAnimate_StartThingLightAnim(pThing, &targetColor, RD_MULTIPLY(speed, 0.5f), SITHANIMATE_LOOP);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_ThingFadeAnim(SithCog* pCog)
{
    SithAnimationSlot* pAnim;
    rdVector4 curendColor;
    rdVector4 startColor;
    int bLoop;
    float timeDelta;
    float endAlpha;
    float startAlpha;
    SithThing* pThing;
    rdVector_Set4(&curendColor, 1.0f, 1.0f, 1.0f, 1.0f);
    rdVector_Set4(&startColor, 1.0f, 1.0f, 1.0f, 1.0f);
    bLoop      = sithCogExec_PopInt(pCog);
    timeDelta  = sithCogExec_PopFlex(pCog);
    endAlpha   = sithCogExec_PopFlex(pCog);
    startAlpha = sithCogExec_PopFlex(pCog);
    pThing     = sithCogExec_PopThing(pCog);
    if ( !pThing || timeDelta <= 0.0f )
    {
#line 2756
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad parameters to sithCogFunctionThing_FadeThingAnim.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        if ( pThing->type == SITH_THING_POLYLINE )
        {
            memcpy(&startColor, &pThing->renderData.data.pPolyline->face.extraLight, sizeof(startColor));
            memcpy(&curendColor, &pThing->renderData.data.pPolyline->face.extraLight, sizeof(curendColor));
            startColor.alpha = startAlpha;
            curendColor.alpha   = endAlpha;
        }
        else
        {
            startColor.alpha = startAlpha;
            rdModel3_SetThingColor(&pThing->renderData, &startColor);
            curendColor.alpha = endAlpha;
        }

        if ( bLoop )
        {
            pAnim = sithAnimate_StartThingFadeAnim(pThing, &startColor, &curendColor, RD_MULTIPLY(timeDelta, 0.5f), SITHANIMATE_LOOP);
        }
        else
        {
            pAnim = sithAnimate_StartThingFadeAnim(pThing, &startColor, &curendColor, timeDelta, (SithAnimateFlags)0);
        }

        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
            return;
        }
    }

#line 2793
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create animation for sithCogFunctionThing_FadeThingAnim().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_WaitForStop(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 2823
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal thingref in WaitForStop call.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->moveType != SITH_MT_PATH )
    {
#line 2829
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: thing %s is not a path move thing in WaitForStop call.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    if ( (pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_MOVE | SITH_PATHMOVE_ROTATE)) == 0 )
    {
        return;
    }

    pCog->status          = SITHCOG_STATUS_WAITING_THING_TO_STOP;
    pCog->statusParams[0] = pThing->idx;
    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: Waiting for stop on object %d.\n", pCog->aName, pThing->idx);
        sithConsole_PrintString(std_genBuffer);
    }
}

void J3DAPI sithCogFunctionThing_WaitForAnimStop(SithCog* pCog)
{
    SithAnimationSlot* pAnim;
    int animID = sithCogExec_PopInt(pCog);
    pAnim = sithAnimate_GetAnim(animID);
    if ( !pAnim )
    {
#line 2880
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Anim ID %d is not a valid ID in WaitForAnimStop call.\n", pCog->aName, animID, 0, 0);
        return;
    }

    pCog->status          = SITHCOG_STATUS_WAITING_ANIMATION_TO_STOP;
    pCog->statusParams[0] = animID;

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: Waiting for anim %d to stop.\n", pCog->aName, animID);
        sithConsole_PrintString(std_genBuffer);
    }
}

void J3DAPI sithCogFunctionThing_GetThingSector(SithCog* pCog)
{
    SithThing* pThing;

#line 2913
    SITH_ASSERTREL(pCog);

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !pThing->pInSector )
    {
        goto error;
    }

    sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pThing->pInSector));
    return;

error:
#line 2924
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing ref in GetThingSector().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_GetCurFrame(SithCog* pCog)
{
    SithThing* pThing;

#line 2946
    SITH_ASSERTREL(pCog);

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->moveType != SITH_MT_PATH )
    {
        goto error;
    }

    sithCogExec_PushInt(pCog, pThing->moveInfo.pathMovement.currentFrame);
    return;

error:
    if ( !pThing )
    {
#line 2958
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thingRef in GetCurFrame.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
#line 2960
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not a path move type thing.\n", pCog->aName, pThing->aName, 0, 0);
    }

    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionThing_GetGoalFrame(SithCog* pCog)
{
    SithThing* pThing;

#line 2983
    SITH_ASSERTREL(pCog);

    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->moveType != SITH_MT_PATH )
    {
        goto error;
    }

    sithCogExec_PushInt(pCog, pThing->moveInfo.pathMovement.goalFrame);
    return;

error:
#line 2994
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thingRef in GetGoalFrame.\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionThing_StopThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        // TODO: Maybe add error log?
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
        {
            sithPhysics_ResetThingMovement(pThing);
            if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
            {
                sithAIMove_AIStop(pThing->controlInfo.aiControl.pLocal);
            }
        } break;
        case SITH_MT_PATH:
        {
            sithPathMove_Finish(pThing);
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithDSSThing_PathMove(pThing, 0, 0.0f, 2, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL); // 2 - finish moving
                }
            }
        } break;
    };
}

void J3DAPI sithCogFunctionThing_IsThingMoving(SithCog* pCog)
{
    SithThing* pThing;
    int bMoving;
#line 3070
    SITH_ASSERTREL(pCog);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        goto error;
    }

    if ( pThing->type == SITH_THING_FREE )
    {
        goto error;
    }

    if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
    {
        bMoving = pThing->controlInfo.aiControl.pLocal->mode & (SITHAI_MODE_TURNING | SITHAI_MODE_MOVING);
    }
    else if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        bMoving = (pThing->moveInfo.physics.velocity.x != 0.0f
                   || pThing->moveInfo.physics.velocity.y != 0.0f
                   || pThing->moveInfo.physics.velocity.z != 0.0f)
            | (pThing->moveInfo.physics.angularVelocity.pitch != 0.0f
               || pThing->moveInfo.physics.angularVelocity.yaw != 0.0f
               || pThing->moveInfo.physics.angularVelocity.roll != 0.0f);
    }
    else if ( pThing->moveType == SITH_MT_PATH )
    {
        bMoving = pThing->moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE);
    }
    else
    {
#line 3103
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unknown movement type for thingRef, assuming not moving.\n", pCog->aName, 0, 0, 0);
        bMoving = 0;
    }

    sithCogExec_PushInt(pCog, bMoving);
    return;

error:
#line 3112
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad parameters in IsMoving()\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionThing_SetThingPulse(SithCog* pCog)
{
    float secPulse    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        goto error;
    }

    if ( secPulse != 0.0f )
    {
        pThing->flags            |= SITH_TF_PULSESET;
        pThing->msecPulseInterval = (uint32_t)RD_MULTIPLY(secPulse, 1000.0f);
        pThing->msecNextPulseTime = sithTime_msecGameTime + pThing->msecPulseInterval;
    }
    else
    {
        pThing->flags &= ~SITH_TF_PULSESET;
        pThing->msecNextPulseTime = 0;
        pThing->msecPulseInterval = 0;
    }

    return;

error:
#line 3154
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal parameters to SetThingPulse.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionThing_SetThingTimer(SithCog* pCog)
{
    uint32_t msecTimer;
    float secTimer    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        goto error;
    }

    if ( secTimer != 0.0f )
    {
        msecTimer = (uint32_t)D3DVAL(RD_MULTIPLY(secTimer, 1000.0f));
        pThing->flags |= SITH_TF_TIMERSET;
        pThing->msecTimerTime = sithTime_msecGameTime + msecTimer;
    }
    else
    {
        pThing->flags &= ~SITH_TF_TIMERSET;
        pThing->msecTimerTime = 0;
    }

    return;

error:
#line 3196
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal parameters to SetThingTimer.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionThing_CaptureThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        return;
    }

    pThing->pCaptureCog = pCog;
    pThing->flags      |= SITH_TF_COGLINKED;
}

void J3DAPI sithCogFunctionThing_ReleaseThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        return;
    }

    pThing->pCaptureCog = NULL;
    if ( !pThing->pCog && !sithCog_IsThingLinked(pThing) )
    {
        pThing->flags &= ~SITH_TF_COGLINKED;
    }
}

void J3DAPI sithCogFunctionThing_GetThingParent(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SithThing* pParent;
    if ( !pThing )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pParent = sithThing_GetThingParent(pThing);
    if ( pParent )
    {
        sithCogExec_PushInt(pCog, pParent->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetThingPos(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->pos);
}

void J3DAPI sithCogFunctionThing_SetThingPos(SithCog* pCog)
{
    rdVector3 newPos;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &newPos);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    rdVector_Copy3(&pThing->pos, &newPos);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Pos(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionThing_GetInventory(SithCog* pCog)
{
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 3388
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called GetInventory for illegal thing.\n", pCog->aName, 0, 0, 0);
    }
    else if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 3394
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called GetInventory for illegal type %d.\n", pCog->aName, typeId, 0, 0);
    }
    else
    {
        sithCogExec_PushFlex(pCog, sithInventory_GetInventory(pThing, typeId));
        return;
    }

    sithCogExec_PushFlex(pCog, 0.0f);
}

void J3DAPI sithCogFunctionThing_SetInventory(SithCog* pCog)
{
    float amount      = sithCogExec_PopFlex(pCog);
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 3431
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called SetInventory for illegal thing.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 3437
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called SetInventory for illegal type %d.\n", pCog->aName, typeId, 0, 0);
        return;
    }

    sithInventory_SetInventory(pThing, typeId, amount);
}

void J3DAPI sithCogFunctionThing_ChangeInventory(SithCog* pCog)
{
    float amount      = sithCogExec_PopFlex(pCog);
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 3475
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called ChangeInventory for illegal thing.\n", pCog->aName, 0, 0, 0);
    }
    else if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 3481
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called ChangeInventory for illegal type %d.\n", pCog->aName, typeId, 0, 0);
    }
    else
    {
        sithCogExec_PushFlex(pCog, sithInventory_ChangeInventory(pThing, typeId, amount));
        return;
    }

    sithCogExec_PushFlex(pCog, 0.0f);
}

void J3DAPI sithCogFunctionThing_GetInventoryCog(SithCog* pCog)
{
    SithInventoryType* pCurInventory;
    size_t typeId     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 3515
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called GetInventoryCog for illegal thing.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pCurInventory = sithInventory_GetInventoryType(pThing, typeId);
        if ( typeId >= SITHINVENTORY_MAXTYPES || !pCurInventory )
        {
#line 3523
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Called GetInventoryCog for illegal type %d.\n", pCog->aName, typeId, 0, 0);
        }
        else
        {
            if ( !pCurInventory->pCog )
            {
                sithCogExec_PushInt(pCog, -1);
            }
            else
            {
                sithCogExec_PushInt(pCog, pCurInventory->pCog->idx);
            }

            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_GetThingVelocity(SithCog* pCog)
{
    rdVector3 vec;
    SithThing* pThing;
    rdVector_Copy3(&vec, &rdroid_zeroVector3);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            rdVector_Copy3(&vec, &pThing->moveInfo.physics.velocity);
            break;

        case SITH_MT_PATH:
            rdVector_Scale3(&vec, &pThing->moveInfo.pathMovement.vecDeltaPos, pThing->moveInfo.pathMovement.moveVel);
            break;
    }

    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunctionThing_SetThingVel(SithCog* pCog)
{
    rdVector3 vel;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &vel);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            memcpy(&pThing->moveInfo.physics.velocity, &vel, sizeof(pThing->moveInfo.physics.velocity));
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
                }
            }
            break;

        default:
#line 3611
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set velocity on a movepath thing.", pCog->aName, 0, 0, 0);
            break;
    }
}

void J3DAPI sithCogFunctionThing_ApplyForce(SithCog* pCog)
{
    rdVector3 force;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &force);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        return;
    }

    sithPhysics_ApplyForce(pThing, &force);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
        }
    }
}

void J3DAPI sithCogFunctionThing_AddThingVel(SithCog* pCog)
{
    rdVector3 vel;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &vel);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &vel);
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
                }
            }
            break;

        default:
#line 3681
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot add velocity on a non-physics thing.", pCog->aName, 0, 0, 0);
            break;
    }
}

void J3DAPI sithCogFunctionThing_GetThingLVec(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->orient.lvec);
}

void J3DAPI sithCogFunctionThing_GetThingUVec(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->orient.uvec);
}

void J3DAPI sithCogFunctionThing_GetThingRVec(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->orient.rvec);
}

void J3DAPI sithCogFunctionThing_GetActorHeadPYR(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || (pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER) )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pThing->thingInfo.actorInfo.headPYR);
    }
}

void J3DAPI sithCogFunctionThing_DetachThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
        return;
    }

    sithThing_DetachThing(pThing);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingAttachFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->attach.flags);
}

void J3DAPI sithCogFunctionThing_AttachThingToSurf(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 3859
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AttachThingToSurf().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pSurf )
    {
#line 3865
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector in AttachThingToSurf().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithThing_AttachThingToSurface(pThing, pSurf, /*bNoImpactUpdate=*/1);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_AttachThingToThing(SithCog* pCog)
{
    SithThing* pAttachThing = sithCogExec_PopThing(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 3895
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AttachThingToThing().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pAttachThing )
    {
#line 3901
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad attach thing in AttachThingToThing().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithThing_AttachThingToThing(pThing, pAttachThing);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_AttachThingToThingEx(SithCog* pCog)
{
    SithAttachFlag attflags = sithCogExec_PopInt(pCog);
    SithThing* pAttachObj = sithCogExec_PopThing(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 3932
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in AttachThingToThingEx.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pAttachObj )
    {
#line 3938
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid attach thing in AttachThingToThingEx.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithThing_AttachThingToThing(pThing, pAttachObj);
    pThing->attach.flags |= attflags;

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingAttachedThing(SithCog* pCog)
{
    SithThing* pAttachedThing;
    int thingNum      = -1;
    int attype        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 3976
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid THING in GetThingAttachedThing().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( attype == -1 )
    {
        thingNum = sithThing_GetThingIndex(pThing->pAttachedThing);
    }
    else
    {
        pAttachedThing = pThing->pAttachedThing;
        while ( pAttachedThing )
        {
            if ( pAttachedThing->type == attype )
            {
                thingNum = sithThing_GetThingIndex(pAttachedThing);
                break;
            }

            pAttachedThing = pAttachedThing->pNextAttachedThing;
        }
    }

    sithCogExec_PushInt(pCog, thingNum);
}

void J3DAPI sithCogFunctionThing_WaitMode(SithCog* pCog)
{
    SithPuppetTrack* pTrack;
    SithPuppetSubMode currentMode   = sithCogExec_PopInt(pCog);
    SithThing* pThing        = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4029
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in WaitMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pTrack = sithPuppet_GetModeTrack(pThing, currentMode);
    if ( !pTrack )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
    pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[pTrack->trackNum].guid;
    pCog->statusParams[1] = pThing->idx;
    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionThing_PlayMode(SithCog* pCog)
{
    int trackNum;

    int wait                = sithCogExec_PopInt(pCog);
    SithPuppetSubMode subMode   = sithCogExec_PopInt(pCog);
    SithThing* pThing        = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 4077
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in PlayMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    trackNum = sithPuppet_PlayMode(pThing, subMode, /*pCallback=*/NULL);
    if ( trackNum == -1 )
    {
#line 4086
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No space to add keyframe mode %d to thing %s in PlayMode().\n", pCog->aName, subMode, pThing->aName, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, trackNum);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN && trackNum >= 0 )
        {
            sithDSSThing_PlayKeyMode(pThing, subMode, pThing->renderData.pPuppet->aTracks[trackNum].guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    if ( wait )
    {
        if ( trackNum >= 0 )
        {
            pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
            pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[trackNum].guid;
            pCog->statusParams[1] = pThing->idx;
        }
    }
}

void J3DAPI sithCogFunctionThing_StopMode(SithCog* pCog)
{
    float fade         = sithCogExec_PopFlex(pCog);
    SithPuppetSubMode subMode = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 4131
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in StopMode().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithPuppet_StopMode(pThing, subMode, fade);
}

void J3DAPI sithCogFunctionThing_SynchMode(SithCog* pCog)
{
    int result;
    int bReverse              = sithCogExec_PopInt(pCog);
    float unusedParam                 = sithCogExec_PopFlex(pCog);
    SithPuppetSubMode newMode = sithCogExec_PopInt(pCog);
    SithPuppetSubMode oldMode = sithCogExec_PopInt(pCog);
    SithThing* pThing         = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 4171
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SynchMode().\n", pCog->aName, 0, 0, 0);
        return;
    }

    result = sithPuppet_SynchMode(pThing, oldMode, newMode, unusedParam, bReverse);
    if ( result < 0 )
    {
#line 4188
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: SynchMode() - Unable to sync mode %d to mode %d.\n", pCog->aName, oldMode, newMode, 0);
    }
}

void J3DAPI sithCogFunctionThing_IsModePlaying(SithCog* pCog)
{
    rdKeyframe* keyframe;
    int trackNum;
    int subMode          = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4218
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in IsModePlaying().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    keyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][subMode].pKeyframe;
    if ( !keyframe )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    for ( trackNum = 0; trackNum < (int)RDPUPPET_MAX_TRACKS; ++trackNum )
    {
        if ( pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack == keyframe )
        {
            break;
        }
    }

    if ( trackNum == RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( (pThing->renderData.pPuppet->aTracks[trackNum].status & RDPUPPET_TRACK_PAUSED) != 0 )
    {
        sithCogExec_PushInt(pCog, 2);
    }
    else
    {
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_PauseMode(SithCog* pCog)
{
    rdKeyframe* keyframe;
    int trackNum;
    int subMode          = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4276
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in PauseMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    keyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][subMode].pKeyframe;
    if ( !keyframe )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    for ( trackNum = 0; trackNum < (int)RDPUPPET_MAX_TRACKS; ++trackNum )
    {
        if ( pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack == keyframe )
        {
            break;
        }
    }

    if ( trackNum == RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( (pThing->renderData.pPuppet->aTracks[trackNum].status & RDPUPPET_TRACK_PAUSED) != 0 )
    {
        sithCogExec_PushInt(pCog, 2);
    }
    else
    {
        pThing->renderData.pPuppet->aTracks[trackNum].status |= RDPUPPET_TRACK_PAUSED;
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_ResumeMode(SithCog* pCog)
{
    rdKeyframe* keyframe;
    int trackNum;
    int subMode          = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4335
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in ResumeMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    keyframe = pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][subMode].pKeyframe;
    if ( !keyframe )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    for ( trackNum = 0; trackNum < (int)RDPUPPET_MAX_TRACKS; ++trackNum )
    {
        if ( pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack == keyframe )
        {
            break;
        }
    }

    if ( trackNum == RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( (pThing->renderData.pPuppet->aTracks[trackNum].status & RDPUPPET_TRACK_PAUSED) == 0 )
    {
        sithCogExec_PushInt(pCog, 2);
    }
    else
    {
        pThing->renderData.pPuppet->aTracks[trackNum].status &= ~RDPUPPET_TRACK_PAUSED;
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_TrackToMode(SithCog* pCog)
{
    rdKeyframe* pKFTrack;
    int subMode;
    int trackNum      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4394
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in TrackToMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( trackNum < 0 || trackNum >= (int)RDPUPPET_MAX_TRACKS )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pKFTrack = pThing->renderData.pPuppet->aTracks[trackNum].pKFTrack;
        if ( !pKFTrack )
        {
            sithCogExec_PushInt(pCog, -1);
        }
        else
        {
            for ( subMode = 0; subMode < (int)SITH_PUPPET_NUMSUBMODES; ++subMode )
            {
                if ( pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][subMode].pKeyframe == pKFTrack )
                {
                    break;
                }
            }

            if ( subMode == SITH_PUPPET_NUMSUBMODES )
            {
                sithCogExec_PushInt(pCog, -1);
            }
            else
            {
                sithCogExec_PushInt(pCog, subMode);
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_PlayKey(SithCog* pCog)
{
    int trackNum;
    int bWaiting               = sithCogExec_PopInt(pCog);
    rdKeyframeFlags kfflags = sithCogExec_PopInt(pCog);
    int bLowPriority         = sithCogExec_PopInt(pCog);
    rdKeyframe* pKfTrack    = sithCogExec_PopKeyframe(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4446
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing to play keyframe anim on in PlayKey().\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pThing->renderData.pPuppet )
    {
#line 4452
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: tried to play keyframe anim %s on thing %s - no puppet in PlayKey().\n", pCog->aName, pKfTrack->aName, pThing->aName, 0);
    }
    else if ( !pKfTrack )
    {
#line 4458
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad keyframe reference in PlayKey().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        trackNum = sithPuppet_PlayKey(pThing->renderData.pPuppet, pKfTrack, bLowPriority, bLowPriority + 2, kfflags, NULL);
        if ( trackNum < 0 )
        {
#line 4466
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No space to add keyframe %s to thing %s.\n", pCog->aName, pKfTrack->aName, pThing->aName, 0);
            sithCogExec_PushInt(pCog, -1);
            return;
        }

        sithCogExec_PushInt(pCog, trackNum);
        if ( pThing->moveType == SITH_MT_PATH )
        {
            if ( pThing->moveInfo.pathMovement.mode )
            {
                sithPathMove_Finish(pThing);
            }

            memcpy(&pThing->moveInfo.pathMovement.curOrient.dvec, &pThing->pos, sizeof(pThing->pos));
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_PlayKey(pThing, pKfTrack, kfflags, bLowPriority, pThing->renderData.pPuppet->aTracks[trackNum].guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }

        if ( bWaiting )
        {
            pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
            pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[trackNum].guid;
            pCog->statusParams[1] = pThing->idx;
        }

        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_PlayKeyEx(SithCog* pCog)
{
    int trackNum;
    int bWaiting               = sithCogExec_PopInt(pCog);
    rdKeyframeFlags kfflags = sithCogExec_PopInt(pCog);
    int highPrio        = sithCogExec_PopInt(pCog);
    int lowPriority         = sithCogExec_PopInt(pCog);
    rdKeyframe* pKfTrack    = sithCogExec_PopKeyframe(pCog);
    SithThing* pThing       = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4530
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing to play keyframe anim on in PlayKeyEx().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pKfTrack )
    {
#line 4537
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad keyframe reference in PlayKeyEx().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    trackNum = sithPuppet_PlayKey(pThing->renderData.pPuppet, pKfTrack, lowPriority, highPrio, kfflags, NULL);
    if ( trackNum < 0 )
    {
#line 4545
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No space to add keyframe %s to thing %s in PLayKeyEx().\n", pCog->aName, pKfTrack->aName, pThing->aName, 0);
        sithCogExec_PushInt(pCog, trackNum);
        return;
    }

    sithCogExec_PushInt(pCog, trackNum);
    if ( pThing->moveType == SITH_MT_PATH )
    {
        if ( pThing->moveInfo.pathMovement.mode )
        {
            sithPathMove_Finish(pThing);
        }

        memcpy(&pThing->moveInfo.pathMovement.curOrient.dvec, &pThing->pos, sizeof(pThing->pos));
    }

    if ( bWaiting )
    {
        pCog->status          = SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP;
        pCog->statusParams[0] = pThing->renderData.pPuppet->aTracks[trackNum].guid;
        pCog->statusParams[1] = pThing->idx;
    }
}

void J3DAPI sithCogFunctionThing_PauseKey(SithCog* pCog)
{
    int trackNum      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4585
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in PauseKey().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pThing->pPuppetClass )
    {
#line 4592
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s has no puppet in PauseKey().\n", pCog->aName, pThing->aName, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( trackNum < 0 || trackNum >= (int)RDPUPPET_MAX_TRACKS )
    {
#line 4599
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid track number %d for thing %s in PauseKey().\n", pCog->aName, trackNum, pThing->aName, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pThing->renderData.pPuppet->aTracks[trackNum].status )
    {
#line 4606
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Track number %d does not exist for thing %s in PauseKey().\n", pCog->aName, trackNum, pThing->aName, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pThing->renderData.pPuppet->aTracks[trackNum].status |= RDPUPPET_TRACK_PAUSED;
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_ResumeKey(SithCog* pCog)
{
    int trackNum      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4626
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in ResumeKey().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pThing->pPuppetClass )
    {
#line 4633
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s has no puppet in ResumeKey().\n", pCog->aName, pThing->aName, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( trackNum < 0 || trackNum >= (int)RDPUPPET_MAX_TRACKS )
    {
#line 4640
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid track number %d for thing %s in ResumeKey().\n", pCog->aName, trackNum, pThing->aName, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( !pThing->renderData.pPuppet->aTracks[trackNum].status )
    {
#line 4647
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Track number %d does not exist for thing %s in ResumeKey().\n", pCog->aName, trackNum, pThing->aName, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pThing->renderData.pPuppet->aTracks[trackNum].status &= ~RDPUPPET_TRACK_PAUSED;
        sithCogExec_PushInt(pCog, 1);
    }
}

void J3DAPI sithCogFunctionThing_StopKey(SithCog* pCog)
{
    int result;
    int trackGuid;
    float duration    = sithCogExec_PopFlex(pCog);
    int track      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4671
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing to stop keyframe anim on.\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pThing->renderData.pPuppet )
    {
#line 4677
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: cannot stop puppet track %d, not puppet.\n", pCog->aName, track, 0, 0);
    }
    else if ( track < 0 || track >= (int)RDPUPPET_MAX_TRACKS )
    {
#line 4683
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: can't stop track %d, out of range.\n", pCog->aName, track, 0, 0);
    }
    else if ( duration < 0.0f )
    {
#line 4689
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: fadeTime %g, is illegal.\n", pCog->aName, duration, 0, 0);
    }
    else
    {
        trackGuid = pThing->renderData.pPuppet->aTracks[track].guid;
        result = sithPuppet_StopKey(pThing->renderData.pPuppet, track, duration);
        if ( result )
        {
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithDSSThing_StopKey(pThing, trackGuid, duration, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
                }
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingMesh(SithCog* pCog)
{
    int result;
    int meshNumSrc    = sithCogExec_PopInt(pCog);
    rdModel3* pModel  = sithCogExec_PopModel3(pCog);
    int meshNumber       = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4720
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingMesh.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pModel )
    {
#line 4726
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad model in SetThingMesh.\n", pCog->aName, 0, 0, 0);
        return;
    }

    result = sithThing_AddSwapEntry(pThing, meshNumber, pModel, meshNumSrc);
    sithCogExec_PushInt(pCog, result);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_RestoreThingMesh(SithCog* pCog)
{
    const int  refnum = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 4747
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in RestoreThingMesh.\n", pCog->aName, 0, 0, 0);
        return;
    }
    if ( refnum == -99 )
    {
        sithThing_ResetSwapList(pThing);
    }
    else
    {
        sithThing_RemoveSwapEntry(pThing, refnum);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingModel(SithCog* pCog)
{
    rdModel3* pModel  = sithCogExec_PopModel3(pCog);
    rdModel3* pPrevModel;
    int result;
    int curprevModelIdx;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4788
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: SetThingModel - No thing to set.\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pModel )
    {
#line 4794
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: SetThingModel - No model to set.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pPrevModel = pThing->renderData.data.pModel3;
        if ( pPrevModel )
        {
            curprevModelIdx = pPrevModel->num;
        }
        else
        {
#line 4806
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: SetThingModel - Thing had no model (warning).\n", pCog->aName, 0, 0, 0);
            curprevModelIdx = -1;
        }

        result = sithThing_SetThingModel(pThing, pModel);
        if ( !result )
        {
#line 4814
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: SetThingModel - Failed to set model.\n", pCog->aName, 0, 0, 0);
        }
        else
        {
            sithCogExec_PushInt(pCog, curprevModelIdx);
            if ( result && (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithDSSThing_SetModel(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS);
                }
            }

            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_GetThingModel(SithCog* pCog)
{
    rdModel3* pModel;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4853
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: GetThingModel - No thing to set.", pCog->aName, 0, 0, 0);
    }
    else if ( pThing->renderData.type != RD_THING_MODEL3 )
    {
#line 4859
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: GetThingModel - Thing is not model.", pCog->aName, 0, 0, 0);
    }
    else
    {
        pModel = pThing->renderData.data.pModel3;
        if ( !pModel )
        {
#line 4866
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: GetThingModel - No model on thing.", pCog->aName, 0, 0, 0);
        }
        else
        {
            sithCogExec_PushInt(pCog, pModel->num);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_SetArmedMode(SithCog* pCog)
{
    int armedMode     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 4901
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to set armed mode on illegal thing.\n", pCog->aName, 0, 0, 0);
    }
    else if ( armedMode < 0 || armedMode >= (int)SITH_PUPPET_NUMARMEDMODES )
    {
#line 4907
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to set invalid armed mode '%d' on thing '%s'.\n", pCog->aName, armedMode, pThing->aName, 0);
    }
    else
    {
        sithPuppet_SetArmedMode(pThing, armedMode);
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithDSSThing_UpdateState(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
    }
}

void J3DAPI sithCogFunctionThing_GetThingFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetThingFlags(SithCog* pCog)
{
    const int  flags  = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && flags != 0 )
    {
        pThing->flags |= flags;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
            }
        }
    }
    else
    {
#line 4976
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_ClearThingFlags(SithCog* pCog)
{
    const int  flags  = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && flags )
    {
        pThing->flags &= ~flags;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
            }
        }
    }
    else
    {
#line 5010
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear thing flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_TeleportThing(SithCog* pCog)
{
    SithThing* pSourceThing  = sithCogExec_PopThing(pCog);
    SithThing* pDestThing = sithCogExec_PopThing(pCog);
    if ( !pDestThing || !pSourceThing )
    {
#line 5036
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s:Illegal params in TeleportThing.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pDestThing->attach.flags )
    {
        sithThing_DetachThing(pDestThing);
    }

    memcpy(&pDestThing->orient, &pSourceThing->orient, sizeof(pDestThing->orient));
    memcpy(&pDestThing->pos, &pSourceThing->pos, sizeof(pDestThing->pos));
    sithThing_SetSector(pDestThing, pSourceThing->pInSector, 0);

    if ( pDestThing->moveType == SITH_MT_PHYSICS && (pDestThing->moveInfo.physics.flags & SITH_PF_FLOORSTICK) != 0 )
    {
        sithPhysics_FindFloor(pDestThing, 1);
    }

    if ( pDestThing == sithPlayer_pLocalPlayerThing )
    {
        sithCamera_Update(sithCamera_pCurCamera);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Pos(pDestThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingType(SithCog* pCog)
{
    int type          = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && type >= 0 && type < SITH_THING_NUMTYPES )
    {
        pThing->type = type;
    }
}

void J3DAPI sithCogFunctionThing_GetCollideType(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->collide.type);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetCollideType(SithCog* pCog)
{
    unsigned int type     = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( pThing && type >= SITH_COLLIDE_NONE && type < SITH_COLLIDE_NUMTYPES )
    {
        pThing->collide.type = type;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
            }
        }
    }
    else
    {
#line 5150
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing collide type.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_FirstThingInSector(SithCog* pCog)
{
    SithThing* pFirstThing;
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
#line 5175
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot get first thing in sector.  Invalid sector.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pFirstThing = pSector->pFirstThingInSector;
        if ( pFirstThing )
        {
            sithCogExec_PushInt(pCog, pFirstThing->idx);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}


void J3DAPI sithCogFunctionThing_NextThingInSector(SithCog* pCog)
{
    SithThing* pNextThing;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 5212
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing passed to NextThingInSector.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pNextThing = pThing->pNextThingInSector;
    if ( pNextThing )
    {
        sithCogExec_PushInt(pCog, pNextThing->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_PrevThingInSector(SithCog* pCog)
{
    SithThing* pPrevThing;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 5248
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing passed to PrevThingInSector()\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pPrevThing = pThing->pPrevThingInSector;
    if ( pPrevThing )
    {
        sithCogExec_PushInt(pCog, pPrevThing->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetInventoryMinimum(SithCog* pCog)
{
    float minVal;

    const int  bin    = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 5288
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing passed to GetInventoryMinimum.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    minVal = sithInventory_GetInventoryMinimum(pThing, bin);
    sithCogExec_PushFlex(pCog, minVal);
}

void J3DAPI sithCogFunctionThing_GetInventoryMaximum(SithCog* pCog)
{
    float maxVal;

    const int  bin    = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 5321
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing passed to GetInventoryMaximum.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    maxVal = sithInventory_GetInventoryMaximum(pThing, bin);
    sithCogExec_PushFlex(pCog, maxVal);
}

// Linker-discarded (no retail symbol, caller or registration): retail's __LINE__ anchors put 141 lines
// between GetInventoryMaximum (5321) and PathMovePause (5462) against 16 here, and Jedi Knight's
// sithCogFunctionThing.c has GetLoadedFrames, GetFramePos and GetFrameRot (none registered) exactly
// between those two. Minimal placeholder bodies with
// their locals only: JK.EXE keeps pThing in esi (GetLoadedFrames, 0x00503EA0) and
// frame/pThing in esi/edi (GetFramePos 0x00503EE0, GetFrameRot 0x00503F40); MotS has the same saves.
void J3DAPI sithCogFunctionThing_GetLoadedFrames(SithCog* pCog)
{
    SithThing* pThing;
}

void J3DAPI sithCogFunctionThing_GetFramePos(SithCog* pCog)
{
    size_t frame;
    SithThing* pThing;
}

void J3DAPI sithCogFunctionThing_GetFrameRot(SithCog* pCog)
{
    size_t frame;
    SithThing* pThing;
}

void J3DAPI sithCogFunctionThing_PathMovePause(SithCog* pCog)
{
    int paused       = 0;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PATH )
    {
#line 5462
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
        paused = sithPathMove_PathMovePause(pThing);
    }

    if ( paused == 1 )
    {
        sithCogExec_PushInt(pCog, pThing->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetHeadLightIntensity(SithCog* pCog)
{
    int bColor;  // result never read; the debug frame holds this slot
    rdVector3 color;
    SithThing* pThing;
    bColor = sithCogExec_PopVector(pCog, &color);
    pThing = sithCogExec_PopThing(pCog);
    if ( pThing && (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER) )
    {
        pThing->thingInfo.actorInfo.headLightIntensity.red   = color.red;
        pThing->thingInfo.actorInfo.headLightIntensity.green = color.green;
        pThing->thingInfo.actorInfo.headLightIntensity.blue  = color.blue;
        pThing->thingInfo.actorInfo.headLightIntensity.alpha = 0.0f;
        sithCogExec_PushVector(pCog, &color);
    }
    else
    {
        color.red = color.green = color.blue = -1.0f;
        sithCogExec_PushVector(pCog, &color);
    }
}

void J3DAPI sithCogFunctionThing_GetHeadLightIntensity(SithCog* pCog)
{
    SithThing* pThing;
    int bColor;  // result never read; the debug frame holds this slot
    rdVector3 color;
    bColor = sithCogExec_PopVector(pCog, &color);
    pThing = sithCogExec_PopThing(pCog);
    if ( pThing && (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER) )
    {
        color.red   = pThing->thingInfo.actorInfo.headLightIntensity.red;
        color.green = pThing->thingInfo.actorInfo.headLightIntensity.green;
        color.blue  = pThing->thingInfo.actorInfo.headLightIntensity.blue;
        sithCogExec_PushVector(pCog, &color);
    }
    else
    {
        color.red = color.green = color.blue = -1.0f;
        sithCogExec_PushVector(pCog, &color);
    }
}

void J3DAPI sithCogFunctionThing_IsThingVisible(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        goto error;
    }

    sithCogExec_PushInt(pCog, pThing->renderFrame + 1 >= sithMain_frameNumber);
    return;

error:
#line 5576
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad parameters in IsThingVisible()\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionThing_PathMoveResume(SithCog* pCog)
{
    int bResumed      = 0;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PATH )
    {
#line 5601
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
        bResumed = sithPathMove_PathMoveResume(pThing);
    }

    if ( bResumed == 1 )
    {
        sithCogExec_PushInt(pCog, pThing->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetThingCurLightMode(SithCog* pCog)
{
    rdLightMode subMode  = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        pThing->renderData.lightMode = subMode;
    }
    else
    {
#line 5638
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingCurLightMode().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetThingCurLightMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->renderData.lightMode);
    }
    else
    {
#line 5663
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingCurLightMode().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_SetActorExtraSpeed(SithCog* pCog)
{
    float speed       = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing )
    {
        pThing->thingInfo.actorInfo.extraSpeed = speed;
    }
    else
    {
#line 5692
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetActorExtraSpeed().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetThingTemplate(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 5716
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingTemplate().\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pThing->pTemplate )
    {
#line 5722
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No template for thing in GetThingTemplate().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->pTemplate->idx);
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_SetLifeleft(SithCog* pCog)
{
    const float secLeft = sithCogExec_PopFlex(pCog);
    SithThing* pThing   = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 5756
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetLifeleft().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( secLeft < 0.0f )
    {
#line 5762
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Seconds must be positive for SetLifeleft().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->msecLifeLeft = (uint32_t)RD_MULTIPLY(secLeft, 1000.0f);

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
            sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
        }
    }
}

void J3DAPI sithCogFunctionThing_GetLifeleft(SithCog* pCog)
{
    float secLeft;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 5794
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetLifeleft().\n", pCog->aName, 0, 0, 0);
        return;
    }

    secLeft = (float)pThing->msecLifeLeft / 1000.0f;
    sithCogExec_PushFlex(pCog, secLeft);
}

void J3DAPI sithCogFunctionThing_SetThingThrust(SithCog* pCog)
{
    SithPhysicsInfo* pPhysics;
    rdVector3  thrust;
    int bValid  = sithCogExec_PopVector(pCog, &thrust);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
#line 5828
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or no physics in SetThingThrust().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !bValid )
    {
#line 5834
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad vector in SetThingThrust().\n", pCog->aName, 0, 0, 0);
        return;
    }

    // Return previous newThrust
    pPhysics = &pThing->moveInfo.physics;
    sithCogExec_PushVector(pCog, &pPhysics->thrust);

    // Now set new newThrust
    memcpy(&pPhysics->thrust, &thrust, sizeof(pPhysics->thrust));
}

void J3DAPI sithCogFunctionThing_GetThingThrust(SithCog* pCog)
{
    SithPhysicsInfo* pPhysics;
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
#line 5867
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or no physics in GetThingThrust().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pPhysics = &pThing->moveInfo.physics;
    sithCogExec_PushVector(pCog, &pPhysics->thrust);
}

void J3DAPI sithCogFunctionThing_AmputateJoint(SithCog* pCog)
{
    size_t joint      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 5900
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AmputateJoint().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
#line 5904
        SITH_ASSERTREL(pThing->renderData.data.pModel3 != NULL);
        if ( joint < 0 || joint > pThing->renderData.data.pModel3->numHNodes )
        {
#line 5908
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad joint passed to AmputateJoint().\n", pCog->aName, 0, 0, 0);
        }
        else
        {
            pThing->renderData.paJointAmputationFlags[joint] = 1;
        }
    }
}

void J3DAPI sithCogFunctionThing_SetActorWeapon(SithCog* pCog)
{
    SithThing* pCurWeapon = sithCogExec_PopTemplate(pCog);
    SithThing* pActor  = sithCogExec_PopThing(pCog);

    if ( !pActor || ((pActor->type != SITH_THING_ACTOR) && (pActor->type != SITH_THING_PLAYER)) )
    {
#line 5934
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or weapon in SetActorWeapon().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pActor->thingInfo.actorInfo.pWeaponTemplate = pCurWeapon;
}

void J3DAPI sithCogFunctionThing_GetActorWeapon(SithCog* pCog)
{
    SithThing* pWeaponTemplate;

    SithThing* actor = sithCogExec_PopThing(pCog);

    if ( !actor || ((actor->type != SITH_THING_ACTOR) && (actor->type != SITH_THING_PLAYER)) )
    {
#line 5957
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing or weapon in GetActorWeapon().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pWeaponTemplate = actor->thingInfo.actorInfo.pWeaponTemplate;
    if ( pWeaponTemplate )
    {
        sithCogExec_PushInt(pCog, pWeaponTemplate->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetPhysicsFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PHYSICS )
    {
        sithCogExec_PushInt(pCog, pThing->moveInfo.physics.flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetPhysicsFlags(SithCog* pCog)
{
    const int  physflags = sithCogExec_PopInt(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( pThing && physflags != 0 && pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.flags |= physflags;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
            }
        }
    }
    else
    {
#line 6029
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set Physics flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_ClearPhysicsFlags(SithCog* pCog)
{
    const int  physflags = sithCogExec_PopInt(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( pThing && physflags != 0 && pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.flags &= ~physflags;
    }
    else
    {
#line 6059
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear Physics flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_ParseArg(SithCog* pCog)
{
    size_t i;
    const char* pArgs = sithCogExec_PopString(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pArgs || !pThing )
    {
        goto error;
    }

    strncpy(std_genBuffer, pArgs, STD_ARRAYLEN(std_genBuffer) - 1);
    std_genBuffer[STD_ARRAYLEN(std_genBuffer) - 1] = 0;
    if ( stdConffile_ReadArgsFromStr(std_genBuffer) )
    {
        goto error;
    }

    for ( i = 0; i < stdConffile_entry.numArgs; ++i )
    {
        if ( sithThing_ParseArg(&stdConffile_entry.aArgs[i], sithWorld_pCurrentWorld, pThing) )
        {
#line 6102
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Argument %s=%s parse failed.\n", pCog->aName, stdConffile_entry.aArgs[i].argName, stdConffile_entry.aArgs[i].argValue, 0);
        }
    }

    return;

error:
#line 6109
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad arguments to ParseArg verb.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionThing_SetThingRotVel(SithCog* pCog)
{
    rdVector3 vel;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &vel);
    pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PHYSICS )
    {
        memcpy(&pThing->moveInfo.physics.angularVelocity, &vel, sizeof(vel));
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
            }
        }
    }
    else
    {
#line 6141
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing, or non-physics thing in SetRotVel.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetThingRotVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PHYSICS )
    {
        sithCogExec_PushVector(pCog, &pThing->moveInfo.physics.angularVelocity);
    }
    else
    {
#line 6169
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing, or non-physics thing in SetRotVel.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_SetRotThrust(SithCog* pCog)
{
    rdVector3 thrust;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &thrust);
    pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PHYSICS )
    {
        memcpy(&pThing->moveInfo.physics.rotThrust, &thrust, sizeof(thrust));
        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
            }
        }
    }
    else
    {
#line 6204
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing, or non-physics thing in SetRotThrust.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetRotThrust(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->moveType == SITH_MT_PHYSICS )
    {
        sithCogExec_PushVector(pCog, &pThing->moveInfo.physics.rotThrust);
    }
    else
    {
#line 6231
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing, or non-physics thing in SetRotThrust.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_SetThingLook(SithCog* pCog)
{
    rdVector3 look;
    int bValidVec     = sithCogExec_PopVector(pCog, &look);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && bValidVec == 1 )
    {
        rdVector_Normalize3Acc(&look);
        rdMatrix_BuildFromLook34(&pThing->orient, &look);

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
            }
        }
    }
    else
    {
#line 6267
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Could not set thing orientation from look vector for SetThingLook().\n", pCog->aName, 0, 0, 0);
    }
}

int J3DAPI sithCogFunctionThing_SetThingHeadOrientation(SithThing* pThing, const rdVector3* pLook)
{
    SithActorInfo* pActorInfo;
    rdVector3 dir;

    if ( pThing && pLook )
    {
        if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
        {
            return 0;
        }

        pActorInfo = &pThing->thingInfo.actorInfo;
        if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) != 0 )
        {
            float rdot;
            float ldot;
            float yaw;
            float pitch;

            rdVector_Sub3(&dir, pLook, &pThing->pos);
            rdVector_Normalize3Acc(&dir);

            pitch = stdMath_ArcSin3(dir.z);
            pitch = STDMATH_CLAMP(pitch, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);

            dir.z = 0.0f;
            rdVector_Normalize3Acc(&dir);
            ldot = rdVector_Dot3(&pThing->orient.lvec, &dir);
            rdot = rdVector_Dot3(&pThing->orient.rvec, &dir);
            yaw  = 90.0f - stdMath_ArcSin3(ldot);
            if ( rdot > 0.0f )
            {
                yaw = -yaw;
            }

            yaw = STDMATH_CLAMP(yaw, -35.0f, 35.0f);
            if ( pitch != pActorInfo->headPYR.pitch || yaw != pActorInfo->headPYR.yaw )
            {
                pActorInfo->headPYR.pitch = pitch;
                pActorInfo->headPYR.yaw   = yaw;
                sithActor_SetHeadPYR(pThing, &pActorInfo->headPYR);
            }
        }
        else
        {
#line 6320
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: illegal attempt to set thing %s head orientation: thing is not an actor.\n", pThing->aName, 0, 0, 0);
            return 0;
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingHeadLookPos(SithCog* pCog)
{
    rdVector3 look;
    int bValidVec     = sithCogExec_PopVector(pCog, &look);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && bValidVec == 1 )
    {
        if ( !sithCogFunctionThing_SetThingHeadOrientation(pThing, &look) )
        {
#line 6349
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: illegal attempt to set thing %s head orientation SetThingHeadLook().\n", pCog->aName, pThing->aName, 0, 0);
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
            }
        }
    }
    else if ( pThing )
    {
#line 6357
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad look vector passed for thing %s in SetThingHeadLook().\n", pCog->aName, pThing->aName, 0, 0);
    }
    else
    {
#line 6359
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing reference passed to SetThingHeadLook().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_SetThingHeadLookThing(SithCog* pCog)
{
    SithThing* pLookThing = sithCogExec_PopThing(pCog);
    SithThing* pThing     = sithCogExec_PopThing(pCog);
    if ( pThing && pLookThing )
    {
        if ( !sithCogFunctionThing_SetThingHeadOrientation(pThing, &pLookThing->pos) )
        {
#line 6386
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: illegal attempt to set thing %s head orientation SetThingHeadLook().\n", pCog->aName, pThing->aName, 0, 0);
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
            }
        }
    }
    else if ( pThing )
    {
#line 6394
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad look vector passed for thing %s in SetThingHeadLook().\n", pCog->aName, pThing->aName, 0, 0);
    }
    else
    {
#line 6396
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing reference passed to SetThingHeadLook().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_IsThingCrouching(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
        sithCogExec_PushInt(pCog, -1);
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_CROUCHING) != 0 )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetThingClassCog(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !pThing->pCog )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->pCog->idx);
    }
}

void J3DAPI sithCogFunctionThing_SetThingClassCog(SithCog* pCog)
{
    SithCog* pThingCog = sithCogExec_PopCog(pCog);
    SithThing* pThing  = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 6476
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing, in SetThingClassCog.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pThingCog )
    {
#line 6482
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No cog, in SetThingClassCog.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->pCog = pThingCog;
}

void J3DAPI sithCogFunctionThing_GetThingCaptureCog(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !pThing->pCaptureCog )
    {
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->pCaptureCog->idx);
    }
}

void J3DAPI sithCogFunctionThing_SetThingCaptureCog(SithCog* pCog)
{
    SithCog* pCaptureCog = sithCogExec_PopCog(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 6537
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing, in SetThingCaptureCog.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pCaptureCog )
    {
#line 6543
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No cog, in SetThingCaptureCog.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->pCaptureCog = pCaptureCog;
}

void J3DAPI sithCogFunctionThing_GetThingRespawn(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_ITEM )
    {
#line 6570
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal thing, in GetThingRespawn.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        sithCogExec_PushFlex(pCog, pThing->thingInfo.itemInfo.secRespawnInterval);
    }
}

void J3DAPI sithCogFunctionThing_GetThingSignature(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->signature);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetThingAttachFlags(SithCog* pCog)
{
    int attflags      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && attflags != 0 )
    {
        pThing->attach.flags |= attflags;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
            }
        }
    }
    else
    {
#line 6637
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing attach flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_ClearThingAttachFlags(SithCog* pCog)
{
    int attflags      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && attflags != 0 )
    {
        pThing->attach.flags &= ~attflags;

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
            }
        }
    }
    else
    {
#line 6671
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear thing attach flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetParticleSize(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PARTICLE )
        {
            sithCogExec_PushFlex(pCog, pThing->thingInfo.particleInfo.size);
        }
        else
        {
#line 6703
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot get thing Particle size for non particle things.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushFlex(pCog, -1.0f);
        }
    }
    else
    {
#line 6710
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetParticleSize.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
}

void J3DAPI sithCogFunctionThing_SetParticleSize(SithCog* pCog)
{
    float size        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PARTICLE )
        {
            pThing->thingInfo.particleInfo.size = size;
        }
        else
        {
#line 6745
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing particle size for non particle things.\n", pCog->aName, 0, 0, 0);
        }
    }
    else
    {
#line 6750
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetParticleSize.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetParticleGrowthSpeed(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PARTICLE )
        {
            sithCogExec_PushFlex(pCog, pThing->thingInfo.particleInfo.growthSpeed);
        }
        else
        {
#line 6782
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot get thing particle GrowthSpeed for non particle things.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushFlex(pCog, -1.0f);
        }
    }
    else
    {
#line 6789
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetParticleGrowthSpeed.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
}

void J3DAPI sithCogFunctionThing_SetParticleGrowthSpeed(SithCog* pCog)
{
    float speed       = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PARTICLE )
        {
            pThing->thingInfo.particleInfo.growthSpeed = speed;
        }
        else
        {
#line 6824
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing particle GrowthSpeed for non particle things.\n", pCog->aName, 0, 0, 0);
        }
    }
    else
    {
#line 6829
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetParticleGrowthSpeed.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetParticleTimeoutRate(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PARTICLE )
        {
            sithCogExec_PushFlex(pCog, pThing->thingInfo.particleInfo.timeoutRate);
        }
        else
        {
#line 6861
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot get thing particle TimeoutRate for non particle things.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushFlex(pCog, -1.0f);
        }
    }
    else
    {
#line 6868
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetParticleTimeoutRate.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
}

void J3DAPI sithCogFunctionThing_SetParticleTimeoutRate(SithCog* pCog)
{
    float timeoutRate = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        if ( pThing->type == SITH_THING_PARTICLE )
        {
            pThing->thingInfo.particleInfo.timeoutRate = timeoutRate;
        }
        else
        {
#line 6903
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing particle TimeoutRate for non particle things.\n", pCog->aName, 0, 0, 0);
        }
    }
    else
    {
#line 6908
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetParticleTimeoutRate.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_GetTypeFlags(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.flags);
                break;

            case SITH_THING_WEAPON:
                sithCogExec_PushInt(pCog, pThing->thingInfo.weaponInfo.flags);
                break;

            case SITH_THING_EXPLOSION:
                sithCogExec_PushInt(pCog, pThing->thingInfo.explosionInfo.flags);
                break;

            case SITH_THING_ITEM:
                sithCogExec_PushInt(pCog, pThing->thingInfo.itemInfo.flags);
                break;

            case SITH_THING_PARTICLE:
                sithCogExec_PushInt(pCog, pThing->thingInfo.particleInfo.flags);
                break;

            default:
#line 6952
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot get thing type flags for this type of thing.\n", pCog->aName, 0, 0, 0);
                sithCogExec_PushInt(pCog, -1);
                break;
        }
    }
    else
    {
#line 6959
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetTypeFlags.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_SetTypeFlags(SithCog* pCog)
{
    int flags     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && flags )
    {
        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                pThing->thingInfo.actorInfo.flags |= flags;
                break;

            case SITH_THING_WEAPON:
                pThing->thingInfo.weaponInfo.flags |= flags;
                break;

            case SITH_THING_ITEM:
                pThing->thingInfo.itemInfo.flags |= flags;
                break;

            case SITH_THING_EXPLOSION:
                pThing->thingInfo.explosionInfo.flags |= flags;
                break;

            case SITH_THING_PARTICLE:
                pThing->thingInfo.particleInfo.flags |= flags;
                break;

            default:
#line 7006
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set thing type flags for this type of thing.\n", pCog->aName, 0, 0, 0);
                break;
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
            }
        }
    }
    else
    {
#line 7016
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetTypeFlags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_ClearTypeFlags(SithCog* pCog)
{
    int flags     = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && flags )
    {
        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                pThing->thingInfo.actorInfo.flags &= ~flags;
                break;

            case SITH_THING_WEAPON:
                pThing->thingInfo.weaponInfo.flags &= ~flags;
                break;

            case SITH_THING_ITEM:
                pThing->thingInfo.itemInfo.flags &= ~flags;
                break;

            case SITH_THING_EXPLOSION:
                pThing->thingInfo.explosionInfo.flags &= ~flags;
                break;

            case SITH_THING_PARTICLE:
                pThing->thingInfo.particleInfo.flags &= ~flags;
                break;

            default:
#line 7062
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear thing type flags for this type of thing.\n", pCog->aName, 0, 0, 0);
                break;
        }

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
            }
        }
    }
    else
    {
#line 7072
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in ClearTypeFlags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_TakeItem(SithCog* pCog)
{
    SithThing* pObj = sithCogExec_PopThing(pCog);
    SithThing* pItem  = sithCogExec_PopThing(pCog);
    if ( pItem && pObj && pItem->type == SITH_THING_ITEM )
    {
        sithItem_SetItemTaken(pItem, pObj, 0);
    }
}

void J3DAPI sithCogFunctionThing_HasLOS(SithCog* pCog)
{
    SithThing* pTarget = sithCogExec_PopThing(pCog);
    SithThing* pViewer = sithCogExec_PopThing(pCog);

    if ( pViewer && pTarget )
    {
        if ( sithCollision_HasLOS(pViewer, pTarget, 0) )
        {
            sithCogExec_PushInt(pCog, 1);
        }
        else
        {
            sithCogExec_PushInt(pCog, 0);
        }
    }
    else
    {
#line 7121
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in HasLOS.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetFireOffset(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7145
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetFireOffset.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->thingInfo.actorInfo.fireOffset);
}

void J3DAPI sithCogFunctionThing_SetFireOffset(SithCog* pCog)
{
    rdVector3 offset;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &offset);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 7174
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetFireOffset.\n", pCog->aName, 0, 0, 0);
        return;
    }

    memcpy(&pThing->thingInfo.actorInfo.fireOffset, &offset, sizeof(offset));
}

void J3DAPI sithCogFunctionThing_GetThingUserData(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7200
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingUserData.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->userval);
}

void J3DAPI sithCogFunctionThing_SetThingUserData(SithCog* pCog)
{
    float userValue     = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7229
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingUserData.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->userval = userValue;
}

void J3DAPI sithCogFunctionThing_GetThingCollideSize(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7255
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingCollideSize.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->collide.size);
}

void J3DAPI sithCogFunctionThing_SetThingCollideSize(SithCog* pCog)
{
    float size        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7284
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingCollideSize.\n", pCog->aName, 0, 0, 0);
        return;
    }
    pThing->collide.size = size;
}

void J3DAPI sithCogFunctionThing_GetThingMoveSize(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7310
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingMoveSize.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->collide.movesize);
}

void J3DAPI sithCogFunctionThing_SetThingMoveSize(SithCog* pCog)
{
    float movesize      = sithCogExec_PopFlex(pCog);
    SithThing* pThing   = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7339
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingMoveSize.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->collide.movesize = movesize;
}

void J3DAPI sithCogFunctionThing_GetThingMass(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            sithCogExec_PushFlex(pCog, pThing->moveInfo.physics.mass);
            break;

        default:
#line 7373
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Non physics thing in GetThingMass.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushFlex(pCog, 0.0f);
            break;
    }
}

void J3DAPI sithCogFunctionThing_SetThingMass(SithCog* pCog)
{
    float mass        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            pThing->moveInfo.physics.mass = mass;
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
            {
                if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
                {
                    sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
                }
            }
            break;

        default:
#line 7413
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Non physics thing in SetThingMass.\n", pCog->aName, 0, 0, 0);
            break;
    }
}

void J3DAPI sithCogFunctionThing_SyncThingPos(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7439
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SyncThingPos().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
}

void J3DAPI sithCogFunctionThing_SyncThingAttachment(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7469
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SyncThingAttachment().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithDSSThing_Attachment(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL, 0x00);
}

void J3DAPI sithCogFunctionThing_SyncThingState(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7499
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SyncThingState().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
}

void J3DAPI sithCogFunctionThing_GetMajorMode(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 7527
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing in GetMajorMode().\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pThing->pPuppetClass || !pThing->renderData.pPuppet )
    {
#line 7533
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No puppet on thing %s for GetMajorMode().\n", pCog->aName, pThing->aName, 0, 0);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->pPuppetState->majorMode);
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionThing_SetThingPosEx(SithCog* pCog)
{
    rdVector3 newPos;
    SithThing* pThing;
    SithSector* pSector = sithCogExec_PopSector(pCog);
    sithCogExec_PopVector(pCog, &newPos);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pSector || pSector == (SithSector*)-1 )
    {
        pSector = sithSector_FindSectorAtPos(sithWorld_pCurrentWorld, &newPos);
    }

    if ( !pThing )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    rdVector_Copy3(&pThing->pos, &newPos);
    sithThing_SetSector(pThing, pSector, 0);
    if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_FLOORSTICK) != 0 )
    {
        sithPhysics_FindFloor(pThing, 1);
    }

    if ( pThing == sithPlayer_pLocalPlayerThing )
    {
        sithCamera_Update(sithCamera_pCurCamera);
    }

    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithDSSThing_Pos(pThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionThing_GetThingGuid(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->guid);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionThing_GetGuidThing(SithCog* pCog)
{
    int guid = sithCogExec_PopInt(pCog);

    SithThing* pThing = sithThing_GetGuidThing(guid);
    if ( !pThing )
    {
#line 7649
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid GUID in GetGuidThing.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, pThing->idx);
}

void J3DAPI sithCogFunctionThing_GetThingMaxVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            sithCogExec_PushFlex(pCog, pThing->moveInfo.physics.maxVelocity);
            break;

        default:
#line 7685
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Non physics thing in GetThingMaxVel.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushFlex(pCog, 0.0f);
            break;
    }
}

void J3DAPI sithCogFunctionThing_SetThingMaxVel(SithCog* pCog)
{
    float maxSpeed    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            pThing->moveInfo.physics.maxVelocity = maxSpeed;
            break;

        default:
#line 7720
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Non physics thing in SetThingMaxVel.\n", pCog->aName, 0, 0, 0);
            break;
    }
}

void J3DAPI sithCogFunctionThing_GetThingMaxAngVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            sithCogExec_PushFlex(pCog, pThing->moveInfo.physics.maxRotationVelocity);
            break;

        default:
#line 7752
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Non physics thing in GetThingMaxAngVel.\n", pCog->aName, 0, 0, 0);
            sithCogExec_PushFlex(pCog, 0.0f);
            break;
    }
}

void J3DAPI sithCogFunctionThing_SetThingMaxAngVel(SithCog* pCog)
{
    float maxRotSpeed = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            pThing->moveInfo.physics.maxRotationVelocity = maxRotSpeed;
            break;

        default:
#line 7787
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Non physics thing in SetThingMaxAngVel.\n", pCog->aName, 0, 0, 0);
            break;
    }
}

void J3DAPI sithCogFunctionThing_SetJointAngle(SithCog* pCog)
{
    float angle       = sithCogExec_PopFlex(pCog);
    int jointNum      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 7815
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetJointAngle.\n", pCog->aName, 0, 0, 0);
        return;
    }

#line 7819
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));
    if ( pThing->pPuppetClass && pThing->renderData.type == RD_THING_MODEL3 && pThing->renderData.apTweakedAngles )
    {
        int nodeNum;
        int maxNode;

        maxNode = pThing->renderData.data.pModel3->numHNodes - 1;
        nodeNum = pThing->pPuppetClass->aJoints[jointNum];
        if ( J3DINRANGE(nodeNum, 0, maxNode) )
        {
            pThing->renderData.apTweakedAngles[nodeNum].pitch = angle;
        }
    }
}

void J3DAPI sithCogFunctionThing_GetJointAngle(SithCog* pCog)
{
    float angle       = -1.0f;
    int jointNum      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 7857
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetJointAngle.\n", pCog->aName, 0, 0, 0);
        return;
    }

#line 7861
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));
    if ( pThing->pPuppetClass && pThing->renderData.type == RD_THING_MODEL3 && pThing->renderData.apTweakedAngles )
    {
        int nodeNum;
        int maxNode;

        maxNode = pThing->renderData.data.pModel3->numHNodes - 1;
        nodeNum = pThing->pPuppetClass->aJoints[jointNum];
        if ( J3DINRANGE(nodeNum, 0, maxNode) )
        {
            angle = pThing->renderData.apTweakedAngles[nodeNum].pitch;
        }
    }

    sithCogExec_PushFlex(pCog, angle);
}

void J3DAPI sithCogFunctionThing_SetMaxHeadPitch(SithCog* pCog)
{
    float maxPitch    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 7900
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetMaxHeadPitch.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadPitch);
    pThing->thingInfo.actorInfo.maxHeadPitch = maxPitch;
}

void J3DAPI sithCogFunctionThing_SetMinHeadPitch(SithCog* pCog)
{
    float minPitch    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 7931
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetMinHeadPitch.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.minHeadPitch);
    pThing->thingInfo.actorInfo.minHeadPitch = minPitch;
}

void J3DAPI sithCogFunctionThing_GetThingMaxHeadPitch(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7960
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in GetThingMaxHeadPitch().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 7967
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in GetThingMaxHeadPitch().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadPitch);
}

void J3DAPI sithCogFunctionThing_GetThingMinHeadPitch(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 7992
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in GetThingMinHeadPitch().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 7999
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in GetThingMinHeadPitch().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.minHeadPitch);
}

void J3DAPI sithCogFunctionThing_SetMaxHeadYaw(SithCog* pCog)
{
    float maxYaw      = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8026
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetMaxHeadYaw.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadYaw);
    pThing->thingInfo.actorInfo.maxHeadYaw = maxYaw;
}

void J3DAPI sithCogFunctionThing_GetThingMaxHeadYaw(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8055
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in GetThingMaxHeadYaw().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 8062
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in GetThingMaxHeadYaw().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadYaw);
}

void J3DAPI sithCogFunctionThing_InterpolatePYR(SithCog* pCog)
{
    rdVector3 pyr1, pyrTo;
    rdMatrix34 orient;
    rdVector3 fwd;
    rdVector3 lookTarget2;

    rdVector3 fromPos;
    rdVector3 lookTarget1;

    float angle = sithCogExec_PopFlex(pCog);
    sithCogExec_PopVector(pCog, &lookTarget2);
    sithCogExec_PopVector(pCog, &lookTarget1);
    sithCogExec_PopVector(pCog, &fromPos);

    // TODO: Check for errors

    // Get pyr1

    rdVector_Sub3(&fwd, &lookTarget1, &fromPos);
    rdVector_Normalize3Acc(&fwd);


    rdMatrix_BuildFromLook34(&orient, &fwd);
    rdMatrix_ExtractAngles34(&orient, &pyr1);

    // Get pyr2

    rdVector_Sub3(&fwd, &lookTarget2, &fromPos);
    rdVector_Normalize3Acc(&fwd);

    rdMatrix_BuildFromLook34(&orient, &fwd);
    rdMatrix_ExtractAngles34(&orient, &pyrTo);

    rdVector_Lerp3(&fwd, &pyr1, &pyrTo, angle);
    sithCogExec_PushVector(pCog, &fwd);
}

void J3DAPI sithCogFunctionThing_GetThingLVecPYR(SithCog* pCog)
{
    rdMatrix34 orient;
    rdVector3 pyr;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    rdMatrix_Copy34(&orient, &pThing->orient);
    rdMatrix_ExtractAngles34(&orient, &pyr);
    sithCogExec_PushVector(pCog, &pyr);
}

void J3DAPI sithCogFunctionThing_SetThingLVec(SithCog* pCog)
{
    rdMatrix34 orient;

    rdVector3 lvec;
    int bVector          = sithCogExec_PopVector(pCog, &lvec);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && bVector == 1 )
    {
        rdMatrix_BuildRotate34(&orient, &lvec);
        rdVector_Normalize3Acc(&orient.lvec);
        rdMatrix_BuildFromLook34(&pThing->orient, &orient.lvec);

        if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
        {
            if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN ) {
                sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
            }
        }
    }
    else
    {
#line 8178
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Could not set thing orientation from look vector for SetThingLVec().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionThing_SetHeadPYR(SithCog* pCog)
{
    SithActorInfo* pActorInfo;
    rdVector3 pyr;
    SithThing* pThing;
    sithCogExec_PopVector(pCog, &pyr);
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8205
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetHeadPYR.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
#line 8211
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not an actor or player in SetHeadPYR.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    pActorInfo = &pThing->thingInfo.actorInfo;
    if ( (pActorInfo->flags & SITH_AF_CANROTATEHEAD) == 0 )
    {
#line 8218
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s cannot change head orientation in SetHeadPYR.\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    pyr.pitch = STDMATH_CLAMP(pyr.pitch, pActorInfo->minHeadPitch, pActorInfo->maxHeadPitch);
    pyr.yaw   = STDMATH_CLAMP(pyr.yaw, -pActorInfo->maxHeadYaw, pActorInfo->maxHeadYaw);

    sithActor_SetHeadPYR(pThing, &pyr);
}

void J3DAPI sithCogFunctionThing_SetThingAirDrag(SithCog* pCog)
{
    float drag        = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->moveType != SITH_MT_PHYSICS )
    {
#line 8252
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingAirDrag.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->moveInfo.physics.airDrag = drag;
}

void J3DAPI sithCogFunctionThing_GetThingJointPos(SithCog* pCog)
{
    size_t jointNum   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->renderData.pPuppet )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
#line 8281
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingJointPos.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( jointNum >= pThing->renderData.data.pModel3->numHNodes )
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
#line 8288
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid joint number in GetThingJointPos.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->renderData.paJointMatrices[jointNum].dvec);
}

void J3DAPI sithCogFunctionThing_IsThingModelName(SithCog* pCog)
{
    const char* pModelName = sithCogExec_PopString(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->renderData.data.pModel3 )
    {
#line 8317
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad Thing in IsThingModelName().", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( !pModelName )
    {
#line 8323
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args in IsThingModelName().", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( streqi(pThing->renderData.data.pModel3->aName, pModelName) )
    {
        sithCogExec_PushInt(pCog, 1);
        return;
    }

    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunctionThing_GetMeshByName(SithCog* pCog)
{
    const char* pMeshName = sithCogExec_PopString(pCog);
    SithThing* pThing     = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8353
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad Thing in GetMeshByName().", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, sithThing_GetThingMeshIndex(pThing, pMeshName));
}

void J3DAPI sithCogFunctionThing_GetNodeByName(SithCog* pCog)
{
    const char* pNodeName = sithCogExec_PopString(pCog);
    SithThing* pThing     = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8380
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad Thing in GetNodeByName().", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    sithCogExec_PushInt(pCog, sithThing_GetThingJointIndex(pThing, pNodeName));
}

void J3DAPI sithCogFunctionThing_AttachThingToThingMesh(SithCog* pCog)
{
    SithThing* pCurThing;
    int meshNum             = sithCogExec_PopInt(pCog);
    SithThing* pTemplate    = sithCogExec_PopTemplate(pCog);
    SithThing* pCurAttachThing = sithCogExec_PopThing(pCog);
    if ( !pCurAttachThing || !pCurAttachThing->renderData.pPuppet || !pCurAttachThing->renderData.data.pModel3 )
    {
        sithCogExec_PushInt(pCog, -1);
#line 8412
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in AttachThingToThingMesh().\n", pCog->aName, 0, 0, 0);
    }
    else if ( !pTemplate )
    {
        sithCogExec_PushInt(pCog, -1);
#line 8419
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad template in AttachThingToThingMesh().\n", pCog->aName, 0, 0, 0);
    }
    else if ( meshNum < 0 )
    {
        sithCogExec_PushInt(pCog, -1);
#line 8426
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad mesh number in AttachThingToThingMesh().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pCurThing = sithThing_CreateThingAtPos(pTemplate, &pCurAttachThing->pos, &pCurAttachThing->orient, pCurAttachThing->pInSector, NULL);
        if ( !pCurThing )
        {
            sithCogExec_PushInt(pCog, -1);
#line 8435
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unable to create attached thing in AttachThingToThingMesh().\n", pCog->aName, 0, 0, 0);
        }
        else
        {
            if ( pCurThing->type == SITH_THING_EXPLOSION )
            {
                pCurThing->pParent = pCurAttachThing;
            }

            sithThing_AttachThingToThingMesh(pCurThing, pCurAttachThing, meshNum);
            sithCogExec_PushInt(pCog, pCurThing->idx);
        }
    }
}

void J3DAPI sithCogFunctionThing_DetachThingMesh(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8467
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing reference in DetachThingMesh().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithThing_DestroyThing(pThing);
}

void J3DAPI sithCogFunctionThing_GetThingMaxRotVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8491
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in GetThingMaxRotVel().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 8498
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in GetThingMaxRotVel().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxRotVelocity);
}

void J3DAPI sithCogFunctionThing_SetThingMaxRotVel(SithCog* pCog)
{
    float vel         = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8525
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in SetThingMaxRotVel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 8531
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in SetThingMaxRotVel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->thingInfo.actorInfo.maxRotVelocity = vel;
}

void J3DAPI sithCogFunctionThing_GetThingMaxHeadVel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8555
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in GetThingMaxHeadVel().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 8562
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in GetThingMaxHeadVel().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, pThing->thingInfo.actorInfo.maxHeadVelocity);
}

void J3DAPI sithCogFunctionThing_SetThingMaxHeadVel(SithCog* pCog)
{
    float vel         = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8590
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Ain't no thing in SetThingMaxHeadVel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
#line 8596
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing type in SetThingMaxHeadVel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pThing->thingInfo.actorInfo.maxHeadVelocity = vel;
}

void J3DAPI sithCogFunctionThing_SetPuppetModeFPS(SithCog* pCog)
{
    float fps              = sithCogExec_PopFlex(pCog);
    unsigned int subModeNum   = sithCogExec_PopInt(pCog);
    unsigned int majorMode = sithCogExec_PopInt(pCog);
    SithThing* pThing      = sithCogExec_PopThing(pCog);
    if ( !pThing || !pThing->pPuppetClass )
    {
#line 8616
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetPuppetModeFPS().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
    else if ( majorMode < 0 || majorMode >= SITH_PUPPET_NUMARMEDMODES * SITH_PUPPET_NUMMOVEMODES )
    {
#line 8623
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad major mode number in SetPuppetModeFPS().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
    else if ( subModeNum < 0 || subModeNum >= SITH_PUPPET_NUMSUBMODES )
    {
#line 8630
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad mode number in SetPuppetModeFPS().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
    else if ( !pThing->pPuppetClass->aModes[majorMode][subModeNum].pKeyframe )
    {
#line 8637
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: THING %s doesnt possess Submode %d for SetPuppetModeFPS().\n", pCog->aName, pThing->aName, subModeNum, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
    else
    {
        sithCogExec_PushFlex(pCog, pThing->pPuppetClass->aModes[majorMode][subModeNum].pKeyframe->fps);
        pThing->pPuppetClass->aModes[majorMode][subModeNum].pKeyframe->fps = fps;
    }
}

void J3DAPI sithCogFunctionThing_ResetThing(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->renderData.pPuppet )
    {
#line 8665
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in ResetThing().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithPhysics_ResetThingMovement(pThing);
    sithPuppet_RemoveAllTracks(pThing);
    pThing->moveStatus = SITHPLAYERMOVE_STILL;
}

void J3DAPI sithCogFunctionThing_GetThingInsertOffset(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8691
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetThingInsertOffset().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pThing->renderData.data.pModel3 )
    {
#line 8697
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing model in GetThingInsertOffset().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithCogExec_PushVector(pCog, &pThing->renderData.data.pModel3->insertOffset);
}

void J3DAPI sithCogFunctionThing_SetThingInsertOffset(SithCog* pCog)
{
    rdVector3 offset;
    int bVec          = sithCogExec_PopVector(pCog, &offset);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8725
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingInsertOffset().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !bVec )
    {
#line 8730
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad vector in SetThingInsertOffset().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pThing->renderData.data.pModel3 )
    {
#line 8736
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No thing model in SetThingInsertOffset().\n", pCog->aName, 0, 0, 0);
        return;
    }

    memcpy(&pThing->renderData.data.pModel3->insertOffset, &offset, sizeof(offset));
    sithCogExec_PushVector(pCog, &pThing->renderData.data.pModel3->insertOffset);
}

void J3DAPI sithCogFunctionThing_GetThingEyeOffset(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing ) {
#line 8763
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in GetThingEyeOffset().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
    {
        sithCogExec_PushVector(pCog, &pThing->thingInfo.actorInfo.eyeOffset);
    }
    else
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
}

void J3DAPI sithCogFunctionThing_CheckFloorDistance(SithCog* pCog)
{
    rdVector3 moveNorm;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8786
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in CheckFloorDistance().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
        return;
    }

    memset(&moveNorm, 0, sizeof(moveNorm));
    moveNorm.z = -1.0f;
    sithCogExec_PushFlex(pCog, sithCollision_CheckDistance(pThing, &moveNorm));
}

void J3DAPI sithCogFunctionThing_CheckPathToPoint(SithCog* pCog)
{
    rdVector3 normal;
    float distanceTo;

    rdVector3 vecTarget;
    const int bSkipFloor    = sithCogExec_PopInt(pCog) == 1;
    const int bDetect = sithCogExec_PopInt(pCog) == 1;
    const int bGotVec          = sithCogExec_PopVector(pCog, &vecTarget);
    SithThing* pThing      = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8811
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad THING in CheckPathToPoint() call.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    if ( !bGotVec )
    {
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }



    sithAIUtil_CheckPathToPoint(pThing, &vecTarget, pThing->collide.movesize, &distanceTo, &normal, bDetect, bSkipFloor);
    sithCogExec_PushFlex(pCog, distanceTo);
}

void J3DAPI sithCogFunctionThing_MoveThing(SithCog* pCog)
{
    SithAnimationSlot* pAnim;
    rdVector3 vecDir;
    float timeDelta   = sithCogExec_PopFlex(pCog);
    float travelDist    = sithCogExec_PopFlex(pCog);
    int bVector          = sithCogExec_PopVector(pCog, &vecDir);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8854
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in MoveThing().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !bVector )
    {
#line 8860
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad vector in MoveThing().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( travelDist <= 0.0f )
    {
#line 8866
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad move distance in MoveThing().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( timeDelta <= 0.0f )
    {
        sithCollision_MoveThing(pThing, &vecDir, travelDist, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pAnim = sithAnimate_StartThingMove(pThing, &vecDir, travelDist, timeDelta);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_MoveThingToPos(SithCog* pCog)
{
    SithAnimationSlot* pAnim;
    float distance;
    rdVector3 moveNorm;
    rdVector3 position;
    float time        = sithCogExec_PopFlex(pCog);
    int bVector          = sithCogExec_PopVector(pCog, &position);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8912
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in MoveThingToPos().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !bVector )
    {
#line 8918
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad vector in MoveThingToPos().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( time <= 0.0f )
    {
        rdVector_Sub3(&moveNorm, &position, &pThing->pos);
        distance = rdVector_Normalize3Acc(&moveNorm);
        sithCollision_MoveThing(pThing, &moveNorm, distance, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pAnim = sithAnimate_StartThingMoveToPos(pThing, &position, time);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

void J3DAPI sithCogFunctionThing_SetThingStateChange(SithCog* pCog)
{
    int armModeState  = sithCogExec_PopInt(pCog);
    int state         = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 8951
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetThingStateChange.\n", pCog->aName, 0, 0, 0);
        return;
    }

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_PLAYER:
            pThing->thingInfo.actorInfo.aiState = state;
            pThing->thingInfo.actorInfo.aiArmedModeState = armModeState;
            return;
    }

#line 8964
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not an actor or player in SetThingStateChange.\n", pCog->aName, pThing->aName, 0, 0);
}

void J3DAPI sithCogFunctionThing_StartQuetzAnim(SithCog* pCog)
{
    int subMode          = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 8980
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing in StartQuetzAnim()\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else if ( subMode < 0 || subMode > 6 )
    {
#line 8986
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad mode for thing %s in StartQuetzAnim()\n", pThing->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        switch ( subMode )
        {
            case 2:
                sithAIMove_StartQuetzAnim2(pThing->controlInfo.aiControl.pLocal);
                break;

            case 3:
                sithAIMove_StartQuetzAnim3(pThing->controlInfo.aiControl.pLocal);
                break;

            case 5:
                sithAIMove_StartQuetzAnim5(pThing->controlInfo.aiControl.pLocal);
                break;

            case 4:
                sithAIMove_StartQuetzAnim4(pThing->controlInfo.aiControl.pLocal);
                break;
        }
    }
}

void J3DAPI sithCogFunctionThing_BoardVehicle(SithCog* pCog)
{
    int result;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        if ( !pThing )
        {
#line 9036
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in Board Vehicle.\n", pCog->aName, 0, 0, 0);
        }
        else
        {
#line 9040
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not player in Board Vehicle.\n", pCog->aName, pThing->aName, 0, 0);
        }
    }
    else
    {
        result = sithPlayerControls_TryBoardVehicle(pThing, 1);
        sithCogExec_PushInt(pCog, result);
    }
}

void J3DAPI sithCogFunctionThing_FadeInTrack(SithCog* pCog)
{
    const float curSpeed = sithCogExec_PopFlex(pCog);
    const int track   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    int bSuccess;

    if ( !pThing )
    {
#line 9076
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in FadeInMode().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    bSuccess = rdPuppet_FadeInTrack(pThing->renderData.pPuppet, track, curSpeed);
    sithCogExec_PushInt(pCog, bSuccess);
}

void J3DAPI sithCogFunctionThing_MakeFairyDust(SithCog* pCog)
{
    rdVector3 pos;
    int bPos          = sithCogExec_PopVector(pCog, &pos);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !pThing->pInSector || !bPos )
    {
#line 9106
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args in MakeFairyDust().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithFX_CreateFairyDust(&pos, pThing->pInSector);
}

void J3DAPI sithCogFunctionThing_MakeFairyDustDeluxe(SithCog* pCog)
{
    rdVector3 pos;
    int bPos          = sithCogExec_PopVector(pCog, &pos);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !bPos )
    {
#line 9135
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad args in MakeFairyDustDeluxe().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f )
    {
        sithFX_DestroyFairyDustDeluxe(pThing);
    }
    else
    {
        sithFX_CreateFairyDustDelux(pThing, &pos);
    }
}
