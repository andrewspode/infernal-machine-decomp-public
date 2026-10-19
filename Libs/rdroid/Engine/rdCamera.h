#ifndef RDROID_RDCAMERA_H
#define RDROID_RDCAMERA_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define RDCAMERA_FOVMIN   5.0f
#define RDCAMERA_FOVMAX 179.0f

extern rdVector3 rdCamera_camPYR;

extern rdCamera* rdCamera_pCurCamera;

extern rdMatrix34 rdCamera_camMatrix;

rdCamera* J3DAPI rdCamera_New(float fov, int bFarClip, float nearPlane, float farPlane, float aspectRatio);
int J3DAPI rdCamera_NewEntry(rdCamera* pCamera, float fov, int bClipFar, float nearPlane, float farPlane, float aspectRatio);
void J3DAPI rdCamera_Free(rdCamera* pCamera);
void J3DAPI rdCamera_FreeEntry(rdCamera* pCamera);

int J3DAPI rdCamera_SetCanvas(rdCamera* pCamera, rdCanvas* pCanvas);
void J3DAPI rdCamera_SetCurrent(rdCamera* pCamera);

int J3DAPI rdCamera_SetFOV(rdCamera* pCamera, float fov);
int J3DAPI rdCamera_SetProjectType(rdCamera* pCamera, rdCameraProjType type);
int J3DAPI rdCamera_SetAspectRatio(rdCamera* pCamera, float ratio);
int J3DAPI rdCamera_SetFrustrum(rdCamera* pCamera, rdClipFrustum* pFrustrum, int left, int top, int right, int bottom);

void J3DAPI rdCamera_Update(const rdMatrix34* orient);

void J3DAPI rdCamera_OrthoProject(rdVector3* pDestVertex, const rdVector3* pSrcVertex);
void J3DAPI rdCamera_OrthoProjectLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts);
void J3DAPI rdCamera_OrthoProjectSquare(rdVector3* pDestVertex, const rdVector3* pSrcVertex);
void J3DAPI rdCamera_OrthoProjectSquareLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts);
void J3DAPI rdCamera_PerspProject(rdVector3* pDestVertex, const rdVector3* pSrcVertex);
void J3DAPI rdCamera_PerspProjectLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts);
void J3DAPI rdCamera_PerspProjectSquare(rdVector3* pDestVertex, const rdVector3* pSrcVertex);
void J3DAPI rdCamera_PerspProjectSquareLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts);

void J3DAPI rdCamera_SetAmbientLight(rdCamera* pCamera, const rdVector4* pLight);
void J3DAPI rdCamera_SetAttenuation(rdCamera* pCamera, float min, float max);
int J3DAPI rdCamera_AddLight(rdCamera* pCamera, rdLight* pLight, const rdVector3* pPos);
int J3DAPI rdCamera_ClearLights(rdCamera* pCamera);

void J3DAPI rdCamera_SetOverlayDepthRange(int bEnable);


J3D_EXTERN_C_END
#endif // RDROID_RDCAMERA_H
