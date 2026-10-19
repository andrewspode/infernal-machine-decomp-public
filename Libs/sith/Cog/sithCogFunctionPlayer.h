#ifndef SITH_SITHCOGFUNCTIONPLAYER_H
#define SITH_SITHCOGFUNCTIONPLAYER_H
#include <j3dcore/j3d.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

int J3DAPI sithCogFunctionPlayer_RegisterFunctions(SithCogSymbolTable* pTable);
void J3DAPI sithCogFunctionPlayer_GetLocalPlayerThing(SithCog* pCog);

J3D_EXTERN_C_END
#endif // SITH_SITHCOGFUNCTIONPLAYER_H
