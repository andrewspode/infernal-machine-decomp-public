#ifndef RDROID_RDCACHE_H
#define RDROID_RDCACHE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define RDCACHE_MAXFACEVERTICES 64 // Max vertices per face
                                   // Note, some clipping functions, such as rdClip_FaceToPlane can handle at max 144 vertices. So be conservative when raising this number

#define RDCACHE_MAXVERTICES 2048

extern rdCacheProcEntry rdCache_aProcFaces[128];
extern size_t rdCache_numProcFaces;
extern size_t rdCache_drawnFaces;

void J3DAPI rdCache_Startup(void);
void J3DAPI rdCache_AdvanceFrame(void);
int  J3DAPI rdCache_GetFrameNum(void);

rdCacheProcEntry* J3DAPI rdCache_GetProcEntry(void);
rdCacheProcEntry* J3DAPI rdCache_GetAlphaProcEntry(void);

void  J3DAPI rdCache_Flush(void);
void  J3DAPI rdCache_FlushAlpha(void);

void J3DAPI rdCache_AddProcFace(size_t numVerts);
void J3DAPI rdCache_AddAlphaProcFace(size_t numVertices);

void J3DAPI rdCache_SendFaceListToHardware(size_t numPolys, rdCacheProcEntry* aPolys, rdCacheSortFunc pfSort);
void J3DAPI rdCache_SendWireframeFaceListToHardware(size_t numPolys, rdCacheProcEntry* aPolys);

void J3DAPI rdCache_AddToTextureCache(tSystemTexture* pTexture, StdColorFormatType format);

J3D_EXTERN_C_END
#endif // RDROID_RDCACHE_H
