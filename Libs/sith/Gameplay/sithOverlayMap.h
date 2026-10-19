#ifndef SITH_SITHOVERLAYMAP_H
#define SITH_SITHOVERLAYMAP_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

extern int sithOverlayMap_bMapVisible;

int J3DAPI sithOverlayMap_Open(SithOverlayMapConfig* pConfig);
void J3DAPI sithOverlayMap_Close(void);

int J3DAPI sithOverlayMap_IsMapVisible(void);
int J3DAPI sithOverlayMap_ToggleMap(void);
void J3DAPI sithOverlayMap_ZoomIn(void);
void J3DAPI sithOverlayMap_ZoomOut(void);

void J3DAPI sithOverlayMap_Draw(void);

void J3DAPI sithOverlayMap_SetShowHints(int bShow);
int J3DAPI sithOverlayMap_GetShowHints(void);
int J3DAPI sithOverlayMap_UpdateHints(void);

void J3DAPI sithOverlayMap_EnableMapRotation(int bEnable);
int J3DAPI sithOverlayMap_GetMapRotation(void);

int J3DAPI sithOverlayMap_SetHintSolved(SithThing* pThing);
int J3DAPI sithOverlayMap_SetHintUnsolved(SithThing* pThing);

void J3DAPI sithOverlayMap_GetNumSeenHints(int* pOutNumSeenHints, int* pOutNumHints);


J3D_EXTERN_C_END
#endif // SITH_SITHOVERLAYMAP_H
