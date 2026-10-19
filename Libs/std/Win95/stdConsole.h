#ifndef STD_STDCONSOLE_H
#define STD_STDCONSOLE_H
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

extern HANDLE stdConsole_hOutput;

int J3DAPI stdConsole_Startup(const char* pTitleText, int attributes, int bShowMinimized);
void J3DAPI stdConsole_Shutdown(void);

int J3DAPI stdConsole_SetAttributes(WORD wAttributes);
int J3DAPI stdConsole_SetConsoleTextAttribute(WORD wAttributes);
int J3DAPI stdConsole_InitOutputConsole(void);
int J3DAPI stdConsole_WriteConsole(const char* pText, WORD textAttribute);

J3D_EXTERN_C_END
#endif // STD_STDCONSOLE_H
