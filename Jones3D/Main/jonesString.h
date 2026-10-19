#ifndef JONES3D_JONESSTRING_H
#define JONES3D_JONESSTRING_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

const char* J3DAPI jonesString_GetString(const char* pKey);
int J3DAPI jonesString_Startup(void);
void J3DAPI jonesString_Shutdown(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESSTRING_H
