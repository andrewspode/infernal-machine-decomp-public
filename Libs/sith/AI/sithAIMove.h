#ifndef SITH_SITHAIMOVE_H
#define SITH_SITHAIMOVE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

extern float sithAIMove_flt_585464;

extern float sithAIMove_flt_585468;

void J3DAPI sithAIMove_Update(SithThing* pThing, float secDeltatTime);
void J3DAPI sithAIMove_UpdateMineCar(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateTurn(SithAIControlBlock* pLocal, float secDeltatTime);
void J3DAPI sithAIMove_UpdateMove(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_CheckMoveAlign(SithAIControlBlock* pLocal);
double J3DAPI sithAIMove_GetGoalTolerance(SithAIControlBlock* pLocal, rdVector3* a2, rdVector3* a3);
int J3DAPI sithAIMove_CheckWalkMove(SithAIControlBlock* pLocal, float secDeltaTime);
int J3DAPI sithAIMove_CheckFlyMove(SithAIControlBlock* pLocal, float secDeltaTime);
int J3DAPI sithAIMove_CheckSwimMove(SithAIControlBlock* pLocal, float secDeltaTime);
int J3DAPI sithAIMove_AIGetMoveState(const SithAIControlBlock* pLocal);
int J3DAPI sithAIMove_TrySpecialTurn(SithAIControlBlock* pLocal, float angle);
float J3DAPI sithAIMove_UpdateAIMove(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_GetAIMoveModes(const SithThing* pThing, float moveSpeed, int moveDirection, SithThingMoveStatus* pOutMoveStatus, SithPuppetSubMode* pOutSubmode);
void J3DAPI sithAIMove_SetSubMode(SithAIControlBlock* pLocal, SithThingMoveStatus moveStatus, size_t submode);
void J3DAPI sithAIMove_PuppetCallback(SithThing* pThing, int trackNum, rdKeyMarkerType marker);
void J3DAPI sithAIMove_AISetLookThing(SithAIControlBlock* pLocal, const SithThing* pThing);
void J3DAPI sithAIMove_AISetLookPos(SithAIControlBlock* pLocal, const rdVector3* targetPos);
void J3DAPI sithAIMove_AISetLookPosEyeLevel(SithAIControlBlock* pLocal, const rdVector3* targetPos);
int J3DAPI sithAIMove_AISetMovePos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed);
int J3DAPI sithAIMove_AISetMoveTargetPos(SithAIControlBlock* pLocal, const rdVector3* moveToPos, float moveSpeed);
int J3DAPI sithAIMove_StartSpecialMove(SithAIControlBlock* pLocal, int armedModeState);
void J3DAPI sithAIMove_FinishSpecialMove(SithAIControlBlock* pLocal, int type);
void J3DAPI sithAIMove_AIJump(SithAIControlBlock* pLocal, rdVector3* movePos, float a3);
void J3DAPI sithAIMove_AIStop(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_SetGoalReached(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_Unreachable(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_StopAIMovement(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_ResetAILook(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_UpdateBoss(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateHeadTrackTarget(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateHeadTrackLookPos(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_GetQuetzStrikeAngles(SithAIControlBlock* pLocal, float* pYaw, float* pPitch, float* pHeadYaw, float* pHeadPitch);
void J3DAPI sithAIMove_UpdateQuetzStrike(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_GetQuetzAnim2Angles(SithAIControlBlock* pLocal, float* angle, float* a3, float* a4, float* a5);
void J3DAPI sithAIMove_UpdateQuetzAnim2(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateQuetzAnim3(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateQuetzAnim5(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateQuetzAnim4(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_UpdateQuetzTail(SithAIControlBlock* pLocal, float secDeltaTime);
void J3DAPI sithAIMove_StartQuetzStrike(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_StartQuetzAnim2(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_StartQuetzAnim3(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_StartQuetzAnim5(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_StartQuetzAnim4(SithAIControlBlock* pLocal);
void J3DAPI sithAIMove_UpdateMardukTail(SithThing* pThing, float secDeltaTime);

J3D_EXTERN_C_END
#endif // SITH_SITHAIMOVE_H
