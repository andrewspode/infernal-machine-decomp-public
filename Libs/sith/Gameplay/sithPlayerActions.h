#ifndef SITH_SITHPLAYERACTIONS_H
#define SITH_SITHPLAYERACTIONS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

extern int sithPlayerActions_jewelFlyingPuppetTrackNum;

extern SithSurface* sithPlayerActions_pCurCollidedSurface;

extern rdFace* sithPlayerActions_pCurCollidedModelFace;

extern rdModel3* sithPlayerActions_pCurCollidedModel;

extern SithThing* sithPlayerActions_pPlasma;

extern int sithPlayerActions_bJewelFlying;

extern int sithPlayerActions_bPlayerInvisible;

extern SithSurface* sithPlayerActions_pChalkWriteSurf;

extern rdVector3 sithPlayerActions_posChalkMarkWrite;

extern SithSector* sithPlayerActions_pChalkWriteSector;

int J3DAPI sithPlayerActions_Activate(SithThing* pPlayer);
void J3DAPI sithPlayerActions_Jump(SithThing* pThing, float a2, unsigned int jumpDirection);
void J3DAPI sithPlayerActions_MoveToPlayerPosition(SithThing* pThing, unsigned int playerNum);
void J3DAPI sithPlayerActions_PushItem(SithThing* pThing, SithThing* pItem, rdVector3* pDirection);
void J3DAPI sithPlayerActions_PullItem(SithThing* pThing, SithThing* pItem, rdVector3* pDirection);
void J3DAPI sithPlayerActions_ClimbPullUp(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbMove(SithThing* pThing, SithSurface* pSurf, int direction);
void J3DAPI sithPlayerActions_UnmountWall(SithThing* pThing, int trackNum);
void J3DAPI sithPlayerActions_ClimbOn1m(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbOn2m(SithThing* pThing);
void J3DAPI sithPlayerActions_LeapForward(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpForward(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpLeft(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpRight(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpRollBack(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpRollForward(SithThing* pThing);
void J3DAPI sithPlayerActions_StrafeLeft(SithThing* pThing);
void J3DAPI sithPlayerActions_StrafeRight(SithThing* pThing);
void J3DAPI sithPlayerActions_HopBack(SithThing* pThing);
void J3DAPI sithPlayerActions_Stand2Crawl(SithThing* pThing);
void J3DAPI sithPlayerActions_Crawl2Stand(SithThing* pThing);
void J3DAPI sithPlayerActions_JumpStart(SithThing* pThing);
int J3DAPI sithPlayerActions_HasActiveWeapon(SithThing* pThing);
void J3DAPI sithPlayerActions_ClimbToClimb(SithThing* pThing, int bLedge);
void J3DAPI sithPlayerActions_ClimbToHang(SithThing* pThing);
int J3DAPI sithPlayerActions_CanMoveToPos(SithThing* pThing, rdVector3* pPos, int* a3);
float J3DAPI sithPlayerActions_SearchForCollision(SithThing* pThing, const rdVector3* pPYR, SithSurface** pHitSurf, rdFace** pHitFace, rdModel3Mesh** pHitMesh, SithThing** pHitThing);
int J3DAPI sithPlayerActions_CanClimbOn1m(SithThing* pThing);
int J3DAPI sithPlayerActions_CanClimbOn2m(SithThing* pThing);
int J3DAPI sithPlayerActions_SearchClimbSurface(SithThing* pThing);
int J3DAPI sithPlayerActions_ActorGrabLedge(SithThing* pThing, float distance, SithSurface* pHitSurf, rdFace* pHitFace, const rdModel3Mesh* pHitMesh, SithThing* pHitThing);
int J3DAPI sithPlayerActions_ClimbDownToIdle(SithThing* pThing);
int J3DAPI sithPlayerActions_CanPullUp(SithThing* pThing);
float J3DAPI sithPlayerActions_GetFaceTopZ(const SithThing* pThing, const rdFace* pFace, const rdModel3Mesh* pMesh);
void J3DAPI sithPlayerActions_StartInvisibility(void);
void J3DAPI sithPlayerActions_EndInvisibility(void);
int J3DAPI sithPlayerActions_IsInvisible(void);
int J3DAPI sithPlayerActions_StartJewelFlying(void);
void J3DAPI sithPlayerActions_EnableJewelFlying(SithThing* pPlasma);
void J3DAPI sithPlayerActions_DisableJewelFlying(void);
void J3DAPI sithPlayerActions_MoveToClimbSector(SithThing* pThing, SithSurface* pSurface);
void J3DAPI sithPlayerActions_Crawl(SithThing* pThing);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYERACTIONS_H
