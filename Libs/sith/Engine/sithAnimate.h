#ifndef SITH_SITHANIMATE_H
#define SITH_SITHANIMATE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

void J3DAPI sithAnimate_Startup(void);
void sithAnimate_Shutdown(void); // Retail; body folded into the shared ret at 0x004EE700

int J3DAPI sithAnimate_Open(void);
void J3DAPI sithAnimate_Close(void);
void J3DAPI sithAnimate_Reset(void);

void J3DAPI sithAnimate_Stop(SithAnimationSlot* pAnim);
int J3DAPI sithAnimate_GetSurfaceAnimIndex(const SithSurface* pSurf);

SithAnimationSlot* J3DAPI sithAnimate_StartScrollSurfaceAnim(SithSurface* pSurf, const rdVector3* velocity);
SithAnimationSlot* J3DAPI sithAnimate_StartSideSkyAnim(SithSurfaceFlag skyType, const  rdVector2* pDirection);
SithAnimationSlot* J3DAPI sithAnimate_StartSpriteAnim(SithThing* pSprite, float fps, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_StartSurfaceAnim(SithSurface* pSurf, float fps, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_StartMaterialAnim(rdMaterial* pMat, float fps, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_StartSurfaceLightAnim(SithSurface* pSurface, const rdVector3* light, float timeDelta, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_StartThingQuickTurnAnim(SithThing* pThing, int direction);
SithAnimationSlot* J3DAPI sithAnimate_StartSpriteSizeAnim(SithThing* pSprite, const rdVector3* start, const rdVector3* end, float timeDelta);
SithAnimationSlot* J3DAPI sithAnimate_StartSectorLightAnim(SithSector* pSector, const rdVector3* pNewLight, float timeDelta, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_StartThingLightAnim(SithThing* pThing, const rdVector4* color, float timeDelta, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_StartThingMove(SithThing* pThing, const rdVector3* pDirection, float distance, float timeDelta);
SithAnimationSlot* J3DAPI sithAnimate_StartThingMoveToPos(SithThing* pThing, const rdVector3* pPos, float time);
SithAnimationSlot* J3DAPI sithAnimate_StartThingFadeAnim(SithThing* pThing, const rdVector4* startColor, const rdVector4* endColor, float timeDelta, SithAnimateFlags flags);
SithAnimationSlot* J3DAPI sithAnimate_CameraZoom(SithCamera* pCamera, float fov, float timeDelta);
SithAnimationSlot* J3DAPI sithAnimate_PushItem(SithThing* pActor, SithThing* pThing, const rdVector3* pDirection, int trackNum);
SithAnimationSlot* J3DAPI sithAnimate_PullItem(SithThing* pActor, SithThing* pThing, const rdVector3* pDirection, int trackNum);

void J3DAPI sithAnimate_Update(float secTimeDelta);

SithAnimationSlot* J3DAPI sithAnimate_GetSurfaceAnim(const SithSurface* pSurf);
void J3DAPI sithAnimate_GetSurfaceScrollingDirection(const SithSurface* pSurf, rdVector3* result);
SithAnimationSlot* J3DAPI sithAnimate_GetAnim(int animID);
SithAnimationSlot* J3DAPI sithAnimate_GetMaterialAnim(const rdMaterial* pMaterial);

SithAnimationSlot* J3DAPI sithAnimate_Create(void);
int J3DAPI sithAnimate_Save(unsigned int outstream);


J3D_EXTERN_C_END
#endif // SITH_SITHANIMATE_H
