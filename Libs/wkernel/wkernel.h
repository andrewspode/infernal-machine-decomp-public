#ifndef WKERNEL_WKERNEL_H
#define WKERNEL_WKERNEL_H
#include <j3dcore/j3d.h>
#include <wkernel/types.h>

// TODO: move module to std

J3D_EXTERN_C_START

int J3DAPI wkernel_Run(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd, LPCSTR lpWindowName);
void J3DAPI wkernel_SetWindowStyle(LONG dwNewLong);
BOOL J3DAPI wkernel_SetWindowSize(int width, int height);
void J3DAPI wkernel_SetWindowProc(WKERNELWNDPROC pfProc); // Window process
int J3DAPI wkernel_PeekProcessEvents(void);
int J3DAPI wkernel_ProcessEvents(void);

J3D_EXTERN_C_END
#endif // WKERNEL_WKERNEL_H
