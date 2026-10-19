#ifndef JONES3D_JONESCOG_H
#define JONES3D_JONESCOG_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>
#include <sith/types.h>

J3D_EXTERN_C_START

extern int jonesCog_bShowHealthHUD;

extern int jonesCog_bMenuVisible;

extern int jonesCog_bEnableGamesave;

extern int jonesCog_dword_17EFFCC;

int J3DAPI jonesCog_Startup(void);
void J3DAPI jonesCog_Shutdown(void);

void J3DAPI jonesCog_EndLevel(void);
void J3DAPI jonesCog_StartCutscene(SithCog* pCog);// TODO: this should actually be restore cutscene if NULL is passed

void J3DAPI jonesCog_ToggleInterface(SithCog* pCog); // TODO: this should actually be restore interface if NULL is passed

J3D_EXTERN_C_END
#endif // JONES3D_JONESCOG_H
