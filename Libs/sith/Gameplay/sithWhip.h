#ifndef SITH_SITHWHIP_H
#define SITH_SITHWHIP_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

void J3DAPI sithWhip_UpdateWhipAim(SithThing* pThing);

signed int J3DAPI sithWhip_StartWhipSwing(SithThing* pThing);
void J3DAPI sithWhip_DeactivateWhip(SithThing* pThing);

int J3DAPI sithWhip_StartWhipClimb(SithThing* pThing, SithThing* pWhippedThing);

void J3DAPI sithWhip_SetActorWhipClimbIdle(SithThing* pThing);

void J3DAPI sithWhip_FinishWhipClimbDismount(SithThing* pThing);

void J3DAPI sithWhip_ProcessWhipClimbMove(SithThing* pThing, float secDeltaTime);

SithThing* J3DAPI sithWhip_GetWhipSwingThing(void);
SithThing* J3DAPI sithWhip_GetWhipClimbThing(void);

void J3DAPI sithWhip_Reset(void);

int J3DAPI sithWhip_Save(DPID idTo, uint32_t outstream);
int J3DAPI sithWhip_Restore(const SithMessage* pMsg);

void J3DAPI sithWhip_WhipFirePuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType);
void J3DAPI sithWhip_WhipClimbPuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType);
void J3DAPI sithWhip_ClimbDismountPuppetCallback(SithThing* pThing, int track, rdKeyMarkerType mt);

J3D_EXTERN_C_END
#endif // SITH_SITHWHIP_H
