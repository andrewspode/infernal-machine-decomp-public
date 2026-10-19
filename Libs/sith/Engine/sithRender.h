#ifndef SITH_SITHRENDER_H
#define SITH_SITHRENDER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START
#define SITHRENDER_MAX_VISIBLE_SECTORS     128
#define SITHRENDER_MAX_SECTORS_WITH_THINGS SITHRENDER_MAX_VISIBLE_SECTORS * 2

extern float sithRender_fogDensity;

extern size_t sithRender_numDrawnThings;

extern size_t sithRender_numArchPolys;

extern size_t sithRender_numAlphaArchPolys;

extern size_t sithRender_numThingPolys;

extern size_t sithRender_numAlphaThingPoly;

extern size_t sithRender_numVisibleAdjoins;

extern rdLightMode sithRender_lightMode;

extern size_t sithRender_numVisibleSectors;

extern int sithRender_renderflags;

extern size_t sithRender_numRenderedSectors;

int J3DAPI sithRender_Startup(void);
int J3DAPI sithRender_Open(void);
void J3DAPI sithRender_Close(void);
void J3DAPI sithRender_Shutdown(void);
void J3DAPI sithRender_SetRenderFlags(int flags);
int J3DAPI sithRender_GetRenderFlags(void);
void J3DAPI sithRender_SetLightingMode(rdLightMode mode);

void J3DAPI sithRender_RenderScene(void);
void J3DAPI sithRender_TogglePVS(void);
void J3DAPI sithRender_Draw(void);

void J3DAPI sithRender_BuildVisibleSectorList(SithSector* pSector, rdClipFrustum* pFrustrum);
void J3DAPI sithRender_BuildVisibleSurface(SithSurface* pSurface);
void J3DAPI sithRender_BuildClipFrustrum(rdClipFrustum* pFrustrum, size_t numVertices, float orthLeft, float orthTop, float orthRight, float orthBottom);
void J3DAPI sithRender_PVSBuildVisibleSectorList(SithSector* pSector, rdClipFrustum* pClipFrustum);
void J3DAPI sithRender_PVSBuildVisibleSector(SithSector* pSector);
void J3DAPI sithRender_BuildVisibleSector(SithSector* pSector, const rdClipFrustum* pFrustrum);
void J3DAPI sithRender_RenderSectors(void);
void J3DAPI sithRender_BuildVisibleSectorsThingList(void);
void J3DAPI sithRender_BuildSectorThingList(SithSector* pSector, float curDistance, float extraDistance);
void J3DAPI sithRender_BuildDynamicLights(void);
void J3DAPI sithRender_RenderThings(void);
int J3DAPI sithRender_RenderThing(SithThing* pThing);
void J3DAPI sithRender_RenderAlphaAdjoins(void);
int J3DAPI sithRender_MakeScreenShot(void);

J3D_EXTERN_C_END
#endif // SITH_SITHRENDER_H
