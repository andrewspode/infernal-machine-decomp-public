#ifndef STD_STDBMP_H
#define STD_STDBMP_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI stdBmp_WriteVBuffer(const char* pFilename, tVBuffer* pVBuffer);
HBITMAP J3DAPI stdBmp_Load(const char* pFilename);

J3D_EXTERN_C_END
#endif // STD_STDBMP_H
