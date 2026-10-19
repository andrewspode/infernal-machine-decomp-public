#ifndef SITH_SITHSTRING_H
#define SITH_SITHSTRING_H
#include <j3dcore/j3d.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

int J3DAPI sithString_Startup(void);
void J3DAPI sithString_Shutdown(void);
wchar_t* J3DAPI sithString_GetString(const char* pStr);

J3D_EXTERN_C_END
#endif // SITH_SITHSTRING_H
