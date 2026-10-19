#ifndef RDROID_RDQCLIP_H
#define RDROID_RDQCLIP_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

size_t J3DAPI rdQClip_VerticesInFrustrum(const rdClipFrustum* pFrustrum, const rdVector3* aVertices, size_t numVertices);

// Discarded by retail's linker (debug build 0x0052A1A7); called from rdClip.c's discarded rdClip_QFace3T.
int J3DAPI rdQClip_Face3T(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, int numVertices);

// Function clips face vertices (max RDQCLIP_MAXWORKVERTS) based on their Y coord (aka forward) and clip frustum nearPlane
int J3DAPI rdQClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices);

J3D_EXTERN_C_END
#endif // RDROID_RDQCLIP_H
