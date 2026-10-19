#ifndef STD_STDEFFECT_H
#define STD_STDEFFECT_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

void J3DAPI stdEffect_Startup(void);
void J3DAPI stdEffect_Shutdown(void);
void J3DAPI stdEffect_Reset(void);
void J3DAPI stdEffect_SetFadeFactor(int bEnabled, float factor);
const tStdFadeFactor* J3DAPI stdEffect_GetFadeFactor(void);

J3D_EXTERN_C_END
#endif // STD_STDEFFECT_H
