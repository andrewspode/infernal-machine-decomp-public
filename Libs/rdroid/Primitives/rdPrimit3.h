#ifndef RDROID_RDPRIMIT3_H
#define RDROID_RDPRIMIT3_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

void J3DAPI rdPrimit3_DrawClippedCircle(const rdVector3* pPos, float radius, float step, uint32_t color, uint32_t pattern);
void J3DAPI rdPrimit3_ClipFace(const rdClipFrustum* pFrustrum, rdGeometryMode geoMode, rdLightMode lightMode, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTexVertOffset);

J3D_EXTERN_C_END
#endif // RDROID_RDPRIMIT3_H
