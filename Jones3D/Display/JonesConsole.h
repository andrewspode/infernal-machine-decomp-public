#ifndef JONES3D_JONESCONSOLE_H
#define JONES3D_JONESCONSOLE_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>


J3D_EXTERN_C_START

#define JONESCONSOLE_FRAMERATEID 102u

extern int jonesConsole_bVisible;

int J3DAPI JonesConsole_Startup(void);
void J3DAPI JonesConsole_Shutdown(void);

int J3DAPI JonesConsole_Open(void);
void J3DAPI JonesConsole_Close(void);

int J3DAPI JonesConsole_PrintText(const char* pText);
void J3DAPI JonesConsole_PrintTextWithID(int ID, const char* pText);
void J3DAPI JonesConsole_ClearText(int yourID);

void J3DAPI JonesConsole_ShowConsole(void);
void J3DAPI JonesConsole_HandelChar(char chr);
void J3DAPI JonesConsole_HideConsole(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCONSOLE_H
