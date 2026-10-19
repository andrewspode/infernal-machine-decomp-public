#ifndef STD_STDFNAMES_H
#define STD_STDFNAMES_H
#include <j3dcore/j3d.h>
#include <std/types.h>

#include "stdUtil.h"

J3D_EXTERN_C_START

/* See Libs/std/General/stdUtil.h for why - compile-time-only safety check dropped. */
#define STD_MAKEPATH(outPath, path1, path2) \
    stdFnames_MakePath(outPath, STD_ARRAYLEN(outPath), path1, path2)


const char* J3DAPI stdFnames_FindMedName(const char* pFilePath);
const char* J3DAPI stdFnames_FindExt(const char* pFilePath);
void J3DAPI stdFnames_StripExtAndDot(char* pPath);
void J3DAPI stdFnames_ChangeExt(char* pPath, const char* pExt);
char* J3DAPI stdFnames_Concat(char* dst, const char* src, int size);
void J3DAPI stdFnames_MakePath(char* aOutPath, int size, const char* pPath1, const char* pPath2);

J3D_EXTERN_C_END
#endif // STD_STDFNAMES_H
