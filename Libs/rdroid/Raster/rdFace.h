#ifndef RDROID_RDFACE_H
#define RDROID_RDFACE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

int J3DAPI rdFace_NewEntry(rdFace* pFace);
void J3DAPI rdFace_FreeEntry(rdFace* pFace);

J3D_EXTERN_C_END
#endif // RDROID_RDFACE_H
