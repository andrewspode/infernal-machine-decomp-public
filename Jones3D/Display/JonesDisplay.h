#ifndef JONES3D_JONESDISPLAY_H
#define JONES3D_JONESDISPLAY_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

// Returns 0 for success and 1 indicating error
int J3DAPI JonesDisplay_Startup(JonesDisplaySettings* pSettings);
void J3DAPI JonesDisplay_Shutdown(void);

int J3DAPI JonesDisplay_Open(JonesDisplaySettings* pSettings);
void J3DAPI JonesDisplay_Close(void);

int J3DAPI JonesDisplay_Restart(JonesDisplaySettings* pSettings);
void J3DAPI JonesDisplay_SetDefaultVideoMode(const StdDisplayEnvironment* pEnv, JonesDisplaySettings* pDisplaySettings);

void J3DAPI JonesDisplay_EnableDualMonitor(int bEnable);
int J3DAPI JonesDisplay_UpdateDualScreenWindowSize(const JonesDisplaySettings* pSettings);

void J3DAPI JonesDisplay_OpenLoadScreen(const char* pMatFilePath, float wlStartX, float wlStartY, float wlEndX, float wlEndY, int bPrimaryMusicTheme);
void J3DAPI JonesDisplay_CloseLoadScreen(void);
void J3DAPI JonesDisplay_UpdateLoadProgress(float progress);

J3D_EXTERN_C_END
#endif // JONES3D_JONESDISPLAY_H
