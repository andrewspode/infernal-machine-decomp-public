#ifndef SITH_SITHCAMERA_H
#define SITH_SITHCAMERA_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define SITHCAMERA_INTCAMERANUM     0 // internal camera aka 1st person
#define SITHCAMERA_EXTCAMERANUM     1 // external camera aka 3rd person
#define SITHCAMERA_CINEMACAMERANUM  2 // cinematic camera. Default camera to be used in cutscenes
#define SITHCAMERA_IDLECAMERANUM    4 // idle camera
#define SITHCAMERA_UNKNONWCAMERANUM 5 // unknown
#define SITHCAMERA_ORBCAMERANUM     6 // orbital camera

// Min/Max camera FOV
#define SITHCAMERA_FOVMIN      20.0f
#define SITHCAMERA_FOVMAX     150.0f
#define SITHCAMERA_FOVDEFAULT  90.0f // default camera fov

// Default camera attenuation
#define SITHCAMERA_ATTENUATIONMIN 0.4f
#define SITHCAMERA_ATTENUATIONMAX 0.8f

extern SithCamera* sithCamera_pCurCamera;

extern int sithCamera_bExtCameraLookMode;

extern rdVector3 sithCamera_camSpot;

extern SithCamera sithCamera_aCameras[7];

/* Retail allocates one 44-byte COMMON at 0x018012C0 rather than seven objects: a
   separate 12-byte common is 16-aligned under the measured LINK 5.00 rule, and
   0x018012CC and 0x018012D8 - both pinned by sithCamera_SetPOVShake's store run and
   pushed as addresses by sithCamera_Update - are not. A whole-image census of the 98
   references fixes the member layout. The tags, the variable name, the member names
   and the POV-shake grouping are inferred. */
typedef struct sSithCameraPOVShake
{
    rdVector3 vecPosOffset;
    rdVector3 vecAngleOffset;
    float posDelta;
    float angleDelta;
} SithCameraPOVShake;

typedef struct sSithCameraInfo
{
    int bCurCameraSet;
    SithCameraState stateFlags;
    size_t curCycleCamNum;
    SithCameraPOVShake povShake;
} SithCameraInfo;

extern SithCameraInfo sithCamera_camInfo;

extern rdMatrix34 sithCamera_orbCamOrient;

extern rdVector3 sithCamera_camLookSpot;

int J3DAPI sithCamera_Startup(void);
void J3DAPI sithCamera_Shutdown(void);

int J3DAPI sithCamera_Open(rdCanvas* pCanvas, float aspect);
void J3DAPI sithCamera_Close(void);

void J3DAPI sithCamera_ResetAllCameras(void);

SithCamera* J3DAPI sithCamera_New(SithCameraType type, uint32_t a2, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus);
int J3DAPI sithCamera_NewEntry(SithCamera* pCamera, uint32_t a2, SithCameraType type, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus);

void J3DAPI sithCamera_Free(SithCamera* pCamera);
void J3DAPI sithCamera_FreeEntry(SithCamera* pCamera);

void J3DAPI sithCamera_SetCameraCanvas(SithCamera* pCamera, float aspect, rdCanvas* pCanvas);
int J3DAPI sithCamera_SetCurrentCamera(SithCamera* pCamera);

void J3DAPI sithCamera_CycleCamera(void);
int J3DAPI sithCamera_SetCurrentToCycleCamera(void);
void J3DAPI sithCamera_SetCameraFocus(SithCamera* pCamera, SithThing* pPrimaryFocusThing, SithThing* pSecondaryFocusThing);
void J3DAPI sithCamera_DecrementFOV(SithCamera* pCamera);

void J3DAPI sithCamera_SetCameraFOV(SithCamera* pCamera, float fov);

void J3DAPI sithCamera_SetCameraPosition(SithCamera* pCamera, const rdVector3* pPos);
void J3DAPI sithCamera_GetCameraPosition(const SithCamera* pCamera, rdVector3* pDstPos);

void J3DAPI sithCamera_SetExtCameraOffset(const rdVector3* pOffset);
void J3DAPI sithCamera_SetExtCameraLookOffset(const rdVector3* pOffset);
void J3DAPI sithCamera_RestoreExtCamera(void);

SithThing* J3DAPI sithCamera_GetPrimaryFocus(const SithCamera* pCamera);
SithThing* J3DAPI sithCamera_GetSecondaryFocus(const SithCamera* pCamera);

void J3DAPI sithCamera_Update(SithCamera* pCamera);
void J3DAPI sithCamera_RenderScene(void);

void J3DAPI sithCamera_SetPOVShake(const rdVector3* posOffset, const rdVector3* angleOffset, float posDelta, float angleDelta);

void J3DAPI sithCamera_SetCameraStateFlags(SithCameraState flags);
SithCameraState J3DAPI sithCamera_GetCameraStateFlags(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCAMERA_H
