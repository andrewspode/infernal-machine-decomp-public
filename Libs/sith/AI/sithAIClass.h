#ifndef SITH_SITHAICLASS_H
#define SITH_SITHAICLASS_H
#include <j3dcore/j3d.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

int J3DAPI sithAIClass_Startup(void);
void J3DAPI sithAIClass_Shutdown(void);

int J3DAPI sithAIClass_WriteStaticAIClassesListText(const SithWorld* pWorld); // linker-discarded; the debug build's sithWrite.c calls it
int J3DAPI sithAIClass_ReadStaticAIClassesListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithAIClass_WriteStaticAIClassesListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithAIClass_ReadStaticAIClassesListBinary(tFileHandle fh, SithWorld* pWorld);

int J3DAPI sithAIClass_AllocWorldAIClasses(SithWorld* pWorld, size_t numClasses);
void J3DAPI sithAIClass_FreeWorldAIClasses(SithWorld* pWorld);

SithAIClass* J3DAPI sithAIClass_Load(SithWorld* pWorld, const char* pName);
int J3DAPI sithAIClass_LoadEntry(const char* pPath, SithAIClass* pClass);

J3D_EXTERN_C_END
#endif // SITH_SITHAICLASS_H
