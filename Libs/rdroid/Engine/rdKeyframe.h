#ifndef RDROID_RDKEYFRAME_H
#define RDROID_RDKEYFRAME_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

rdKeyframeLoadFunc J3DAPI rdKeyframe_RegisterLoader(rdKeyframeLoadFunc pFunc);

void J3DAPI rdKeyframe_NewEntry(rdKeyframe* pKeyframe);
rdKeyframe* J3DAPI rdKeyframe_Load(const char* pFilename);
int J3DAPI rdKeyframe_LoadEntry(const char* pFilename, rdKeyframe* pKeyframe);
void J3DAPI rdKeyframe_FreeEntry(rdKeyframe* pKeyfreame);

J3D_EXTERN_C_END
#endif // RDROID_RDKEYFRAME_H
