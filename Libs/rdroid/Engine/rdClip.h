#ifndef RDROID_RDCLIP_H
#define RDROID_RDCLIP_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define MAX_CLIP_VERTICIES 80u

/*
 * Fraction of the edge from `from` to `to` at which it crosses the near or far clip plane
 * `plane` (every near/far site of rdClip.c's clippers and rdQClip.c). Name, home and
 * parameter order inferred. The debug build (/Od) shows the operands unparenthesised at
 * every site (`mov; mov; fld; fsub`, where a parenthesised member read loads first), and
 * retail's clippers need the quotient's own pair plus a value node (Face3TOrtho's near and
 * far loops, the IL budget of its `clipindex++` hoist; no other function moves).
 *
 */
#define RDCLIP_LERPFACTOR(plane, from, to) D3DVAL(((plane - from) / (to - from)))

// Screen-space point written by rdClip_Line3: integer x/y, projected z. Jedi Knight's
// rdClip_Line3 (JK.EXE 0x0046E9D0, MotS 0x00485770, identical) stores __ftol'd x/y and the raw
// z; layout and name follow OpenJKDF2's rdScreenPoint.
typedef struct srdScreenPoint
{
    uint32_t x;
    uint32_t y;
    float z;
} rdScreenPoint;

extern int rdClip_faceStatus;

int J3DAPI rdClip_Line2(const rdCanvas* pCanvas, int* x1, int* y1, int* x2, int* y2);
rdClipOutcode J3DAPI rdClip_CalcOutcode2(const rdCanvas* pCanvas, int x, int y);


int J3DAPI rdClip_Line2Ex(float* x1, float* y1, float* x2, float* y2); // overload of rdClip_Line2
int J3DAPI rdClip_CalcOutcode2Ex(float x, float y, float width, float height); // overload of rdClip_CalcOutcode2
int J3DAPI rdClip_Point3(const rdClipFrustum* pFrustum, const rdVector3* pPoint); // discarded (line gap)
int J3DAPI rdClip_Line3Project(const rdClipFrustum* pClipFrustum, rdVector3* point1, rdVector3* point2, int* out1, int* out2); // discarded (line gap)
int J3DAPI rdClip_Line3Ortho(const rdClipFrustum* pClipFrustum, rdVector3* point1, rdVector3* point2, int* out1, int* out2); // discarded (line gap)
int J3DAPI rdClip_Line3(const rdClipFrustum* pClipFrustum, rdVector3* point1, rdVector3* point2, rdScreenPoint* pScreenPt1, rdScreenPoint* pScreenPt2, int* out1, int* out2); // discarded (line gap)

void J3DAPI rdClip_ClipFacePVS(rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDest);

int J3DAPI rdClip_Face3WPVS(rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices);
int J3DAPI rdClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices);
int J3DAPI rdClip_Face3WOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices);
int J3DAPI rdClip_Face3S(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices);
int J3DAPI rdClip_Face3SOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices);
int J3DAPI rdClip_Face3GS(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector4* aIntensities, int numVertices);
int J3DAPI rdClip_Face3GSOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector4* aIntensities, int numVertices);
int J3DAPI rdClip_Face3GT(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, int numVertices);
int J3DAPI rdClip_Face3GTOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, int numVertices);
int J3DAPI rdClip_Face3T(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, int numVertices);
int J3DAPI rdClip_Face3TOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, int numVertices);

// Checks if sphere is in frustum.
// pCenter should be in camera space.
// Returns RdFrustumCull:
//  0 - inside frustum
//  1 - intersects frustum
//  2 - outside frustum 
RdFrustumCull J3DAPI rdClip_SphereInFrustrum(const rdClipFrustum* pFrustrum, const rdVector3* pCenter, float radius);

// Function clips vertices to clipfrustum and output them to pDst
// Function does quick clipiping by getting number of vertices in laying in frustum and than  only near plane clipping is performed
void J3DAPI rdClip_QFace3W(const rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDst);

// Function clips and transforms face vertices to screen space then assigns them to out pProcFace 
// aVerts here should be transformed to camera space.
int J3DAPI rdClip_FaceToPlane(const rdClipFrustum* pFrustrum, rdCacheProcEntry* pProcFace, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTexVerts, const rdVector4* aLightColors, const rdVector4* aVertColors);

// Function transforms vertices to screen space and assigns them to out pProcFace 
// aVerts here should be transformed to camera space.
void J3DAPI rdClip_VerticesToPlane(rdCacheProcEntry* pProcFace, const rdVector3* aVerts, const rdVector2* aTexVerts, size_t numVerts);

J3D_EXTERN_C_END
#endif // RDROID_RDCLIP_H
