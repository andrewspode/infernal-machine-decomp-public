#ifndef SITH_SITHPHYSICS_H
#define SITH_SITHPHYSICS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

// Track face geometry gathered by sithPhysics_ProcessTrackFace for mine car track following
typedef struct sSithPhysicsTrackFaceData SithPhysicsTrackFaceData;

void J3DAPI sithPhysics_FindFloor(SithThing* pThing, int bNoThingStateUpdate);
void J3DAPI sithPhysics_FindWaterSurface(SithThing* pThing);
void J3DAPI sithPhysics_UpdateThing(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force);
void J3DAPI sithPhysics_SetThingLook(SithThing* pThing, const rdVector3* pNormal, float secDeltaTime);
void J3DAPI sithPhysics_ApplyDrag(rdVector3* pVelocity, float drag, float mag, float secDeltaTime);
int J3DAPI sithPhysics_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);
void J3DAPI sithPhysics_ResetThingMovement(SithThing* pThing);
float J3DAPI sithPhysics_GetThingHeight(const SithThing* pThing);
// Updates physics include gravity effect on thing
// update player physics in the air or water/aeterium
// local variable allocation has failed, the output may be wrong!
void J3DAPI sithPhysics_UpdatePlayerPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateUnderwaterThingPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateClimbingThingPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_UpdateAttachedThingPhysics(SithThing* pThing, float secDeltaTime);
int J3DAPI sithPhysics_CreateMineCarUserBlock(SithThing* pMineCar);
void J3DAPI sithPhysics_InitMineCarFxState(SithThing* pThing, SithMineCarFxState* pFxState);
void J3DAPI sithPhysics_InitVehicleFxState(SithThing* pThing, SithVehicleEngineFxState* pFxState);
void J3DAPI sithPhysics_InitMineCarChassisDefault(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitMineCarChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitTrackTruckChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitTrackJeepChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo);
void J3DAPI sithPhysics_InitMineCarExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);
void J3DAPI sithPhysics_InitJeepExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);
void J3DAPI sithPhysics_InitTrackTruckExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);
void J3DAPI sithPhysics_InitMineCarState(SithThing* pThing, SithMineCarState* pState);
void J3DAPI sithPhysics_InitTrackTruckState(SithThing* pThing, SithMineCarState* pState);
void J3DAPI sithPhysics_UpdateMineCarPhysics(SithThing* pThing, float secDeltaTime);
// update physics
void J3DAPI sithPhysics_UpdateRaftPhysics(SithThing* pThing, float secDeltaTime);
void J3DAPI sithPhysics_ProcessMineCarTrackMove(SithThing* pThing, float secDeltaTime, float speed, rdVector3* pOutMoveDelta);
int J3DAPI sithPhysics_ProcessTrackFace(SithThing* pThing, rdFace* pFace, const rdVector3* pMoveDir, const rdVector3* pRightDir, const rdVector3* pPos, SithPhysicsTrackFaceData* pData);
int J3DAPI sithPhysics_CheckForPointOnTrack(SithThing* pThing, const rdVector3* pPos, rdFace** ppOutFoundFace, int bUpdateState, rdFace* pPrevFace);
void J3DAPI sithPhysics_MoveToTrackFaceFront(SithThing* pThing, SithPhysicsTrackFaceData* pData, float* pSecDeltaTime, rdVector3* pStartPos, rdVector3* pOutPos, float speed);
void J3DAPI sithPhysics_UpdateJeepPhysics(SithThing* pThing, float secDeltaTime);
// function might check for jeep surface and thing face collision


J3D_EXTERN_C_END
#endif // SITH_SITHPHYSICS_H
