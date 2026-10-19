#ifndef JONES3D_JONESDIALOG_H
#define JONES3D_JONESDIALOG_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

int J3DAPI JonesDialog_InitCursorTracking(int* pAlreadyInited);
int J3DAPI JonesDialog_TrackCursor(int bReDraw);
int J3DAPI JonesDialog_ResetCursorTracking(void);
int J3DAPI JonesDialog_CopyRectToDrawBuffer(GDIDIBSectionInfo* pInfo, const PPOINT lpSourceOrigin, LPRECT pDstRect);
int J3DAPI JonesDialog_RestoreBackground(HDC hdc, HWND hwnd, LPPOINT a3, LPRECT a4);
int J3DAPI JonesDialog_ShowDialog(LPCSTR lpTemplateName, HWND hWnd, DLGPROC lpDialogFunc, LPARAM dwInitParam);
BOOL J3DAPI JonesDialog_ShowFileSelectDialog(LPOPENFILENAMEA pofn, int bOpen);

J3D_EXTERN_C_END
#endif // JONES3D_JONESDIALOG_H
