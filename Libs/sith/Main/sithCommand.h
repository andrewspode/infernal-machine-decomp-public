#ifndef SITH_SITHCOMMAND_H
#define SITH_SITHCOMMAND_H
#include <j3dcore/j3d.h>
#include <sith/Main/sithMain.h>


J3D_EXTERN_C_START

void J3DAPI sithCommand_RegisterCommands(void);
int J3DAPI sithCommand_DebugMode(const SithConsoleCommand* pFunc, const char* pArg);


J3D_EXTERN_C_END
#endif // SITH_SITHCOMMAND_H
