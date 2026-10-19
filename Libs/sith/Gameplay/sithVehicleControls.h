#ifndef SITH_SITHVEHICLECONTROLS_H
#define SITH_SITHVEHICLECONTROLS_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

// Result of a wall surface probe along dir (see sithVehicleControls_SearchWallSurface)
typedef struct sSithVehicleSurfaceSearch
{
    SithSurface* pSurface;
    float dot;
    rdVector3 dir;
    float distance;
} SithVehicleSurfaceSearch;


SithMineCarControlState* J3DAPI sithVehicleControls_GetMineCarControlState(void);
void J3DAPI sithVehicleControls_SetMineCarControlState(SithMineCarControlState* pState);
SithRaftControlState* J3DAPI sithVehicleControls_GetRaftControlState(void);
void J3DAPI sithVehicleControls_SetRaftControlState(SithRaftControlState* pState);
void J3DAPI sithVehicleControls_PuppetCallback(SithThing* pThing, int track, rdKeyMarkerType markerType);
void J3DAPI sithVehicleControls_ProcessMineCarPlayerMove(SithThing* pThing, float secDeltaTime);
void J3DAPI sithVehicleControls_ProcessJeepPlayerMove(SithThing* pThing, float secDeltaTime);
void J3DAPI sithVehicleControls_PlayPuppetMode(SithThing* pThing, SithPuppetSubMode submode);
void J3DAPI sithVehicleControls_ProcessRaftPlayerMove(SithThing* pThing, float secDeltaTime);
void J3DAPI sithVehicleControls_FadeJeepMoveSounds(SithThing* pThing);
int J3DAPI sithVehicleControls_CanMoveInDirection(SithThing* pThing, rdVector3* pDir);
void J3DAPI sithVehicleControls_ExitRaft(SithThing* pRaftPlayer);
void J3DAPI sithVehicleControls_UpdateRaftPaddleSoundFx(SithThing* pThing);
void J3DAPI sithVehicleControls_SendMessageUser0(SithThing* pThing);
void J3DAPI sithVehicleControls_SendMessageUser1(SithThing* pThing);

J3D_EXTERN_C_END
#endif // SITH_SITHVEHICLECONTROLS_H
