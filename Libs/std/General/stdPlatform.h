#ifndef STD_STDPLATFORM_H
#define STD_STDPLATFORM_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI stdPlatform_InitServices(tHostServices* pHS);
void J3DAPI stdPlatform_ClearServices(tHostServices* pHS);
unsigned int J3DAPI stdPlatform_GetTimeMsec(void);
void J3DAPI stdPlatform_GetDateTimeString(char* pResult, size_t resultSize); // linker-discarded; the debug build's sithWrite.c calls it

J3DNORETURN void J3DAPI stdPlatform_Assert(const char* pErrorStr, const char* pFilename, int linenum);
int J3DAPI stdPlatform_Printf(const char* format, ...);

void* J3DAPI stdPlatform_AllocHandle(size_t size);
void J3DAPI stdPlatform_FreeHandle(void* pData);
void* J3DAPI stdPlatform_ReallocHandle(void* pMemory, size_t newSize);

int J3DAPI stdPlatform_LockHandle(int);
void J3DAPI stdPlatform_UnlockHandle(void);

J3D_EXTERN_C_END
#endif // STD_STDPLATFORM_H
