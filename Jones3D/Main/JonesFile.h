#ifndef JONES3D_JONESFILE_H
#define JONES3D_JONESFILE_H
#include <j3dcore/j3d.h>
#include <Jones3D/types.h>

J3D_EXTERN_C_START

#define JF_RES_INSTALL  0
#define JF_RES_CWD      1 // current working dir
#define JF_RES_CD       2

void J3DAPI JonesFile_Startup(tHostServices* pHS);
void J3DAPI JonesFile_Shutdown(void);

int J3DAPI JonesFile_Open(tHostServices* pEnv, const char* pInstallPath, const char* pPathCD);
void J3DAPI JonesFile_Close(void);

int J3DAPI JonesFile_GetCurrentCDNum(void);
int J3DAPI JonesFile_FileExists(const char* pFilename);

void J3DAPI JonesFile_SetInstallPath(const char* pPath);
void J3DAPI JonesFile_SetCDPath(const char* pPath);

extern JonesResource JonesFile_aResources[3];

J3D_EXTERN_C_END
#endif // JONES3D_JONESFILE_H
