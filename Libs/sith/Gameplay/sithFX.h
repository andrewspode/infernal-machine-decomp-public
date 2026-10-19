#ifndef SITH_SITHFX_H
#define SITH_SITHFX_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithThing.h>

#include <std/types.h>

J3D_EXTERN_C_START

extern int sithFX_lastChalkMarkNum;

extern SithThing* sithFX_aChalkMarks[10];

extern int sithFX_numChalkMarks;

void J3DAPI sithFX_Reset(void);
void J3DAPI sithFX_ClearChalkMarks(void);

void J3DAPI sithFX_DestroyFairyDustDeluxe(SithThing* pThing);
void J3DAPI sithFX_CreateFairyDust(const rdVector3* pos, SithSector* pSector);
int J3DAPI sithFX_SetFairyDustSize(rdParticle* pParticle, int sizeFactor);
void J3DAPI sithFX_UpdateFairyDustUserBlock(SithThing* pThing);
void J3DAPI sithFX_CreateFairyDustDelux(SithThing* pThing, const rdVector3* pPos);
int J3DAPI sithFX_CreateFairyDustDeluxDusts(SithThing* pThing, SithFairyDustUserBlock* pUserBlock);

void J3DAPI sithFX_CreateBubble(SithThing* pThing);
void J3DAPI sithFX_CreateWaterRipple(SithThing* pThing);
void J3DAPI sithFX_CreateRaftRipple(SithThing* pThing, int bCreateSplash);
void J3DAPI sithFX_CreateRaftWake(SithThing* pThing);
void J3DAPI sithFX_CreatePaddleSplash(SithThing* pThing, const rdVector3* pos);
void J3DAPI sithFX_CreateRowWaterFx(SithThing* pThing, float secTime);
void J3DAPI sithFX_CreateRipple(SithThing* pThing, float size);
void J3DAPI sithFx_CreateTireFx(SithThing* pThing, const rdVector3* pPosLeft, rdMaterial* pMatLeft, const rdVector3* pPosRigth, rdMaterial* pMatRight);
void J3DAPI sithFX_CreateMineCarSparks(SithThing* pThing, rdVector3* pSparkPos);

void J3DAPI sithFX_CreateChalkMark(const rdVector3* pPos, const SithSurface* pAttSurf, SithSector* pSector);
SithThing* J3DAPI sithFX_CreateThingOnSurface(const SithThing* pTemplate, const rdVector3* pPos, const SithSurface* pAttSurf, SithSector* pSector);

void J3DAPI sithFX_UpdatePolyline(SithThing* pThing);
void J3DAPI sithFX_ResetPolylineTexVertOffset(SithThing* pThing);

SithThing* J3DAPI sithFX_CreateLaserThing(const SithThing* pSourceThing, const rdVector3* vecOffset, const rdVector3* vecEnd, float baseRadius, float tipRadius, float duration);
SithThing* J3DAPI sithFX_CreateLightningThing(const SithThing* pSourceThing, const rdVector3* offset, const rdVector3* endPos, float baseRadius, float tipRadius, float duration);

// Crates polyline from pStartThing to pEndThing or endPos if pEndThing is null
SithThing* J3DAPI sithFX_CreatePolylineThing(const SithThing* pSourceThing, SithThing* pEndThing, const rdVector3* endPos, const rdMaterial* pMaterial, float baseRadius, float tipRadius, float duration);

J3D_EXTERN_C_END
#endif // SITH_SITHFX_H
