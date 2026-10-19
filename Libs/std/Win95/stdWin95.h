#ifndef STD_STDWIN95_H
#define STD_STDWIN95_H
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START


void J3DAPI stdWin95_SetWindow(HWND hwnd);
HWND J3DAPI stdWin95_GetWindow(void);
void J3DAPI stdWin95_SetInstance(HINSTANCE hinstance);
HINSTANCE J3DAPI stdWin95_GetInstance(void);
void J3DAPI stdWin95_SetGuid(const GUID* pGuid);
const GUID* stdWin95_GetGuid(void); // linker-discarded; the debug build's stdComm calls it

J3D_EXTERN_C_END
#endif // STD_STDWIN95_H
