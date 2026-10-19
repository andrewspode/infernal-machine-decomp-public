#ifndef JONES3D_JONESMAIN_H
#define JONES3D_JONESMAIN_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

extern char JonesMain_aErrorBuffer[1024];

extern HANDLE JonesMain_mainMutex;

int J3DAPI JonesMain_Startup(const char* lpCmdLine);
void J3DAPI JonesMain_Shutdown(void);

int J3DAPI JonesMain_Open(void);
int J3DAPI JonesMain_Close(void);
int J3DAPI JonesMain_HasStarted(void);

void J3DAPI JonesMain_PauseGame(void);
void J3DAPI JonesMain_ResumeGame(void);
int J3DAPI JonesMain_IsGamePaused(void);

int J3DAPI JonesMain_Process(void);
int J3DAPI JonesMain_ProcessGamesaveState(void);
void J3DAPI JonesMain_UpdateLevelNum(void);

void J3DAPI JonesMain_TogglePrintFramerate(void);

void J3DAPI JonesMain_NextLevel(void);
size_t J3DAPI JonesMain_GetCurrentLevelNum(void);

void J3DAPI JonesMain_SetBonusLevel(void);
void J3DAPI JonesMain_JumpLevel(int levelNum);
void J3DAPI JonesMain_RestartGame(void);
void J3DAPI JonesMain_RestartLevel(void);
void J3DAPI JonesMain_ShowEndCredits(void);

StdDisplayEnvironment* J3DAPI JonesMain_GetDisplayEnvironment(void);
JonesDisplaySettings* J3DAPI JonesMain_GetDisplaySettings(void);

int J3DAPI JonesMain_CloseWindow(void);

void J3DAPI JonesMain_RefreshDisplayDevice(void);

int J3DAPI JonesMain_PlayIntroMovie(void);

void J3DAPI JonesMain_BindToggleMenuControlKeys(const size_t* paKeyIds, int numKeys);

void J3DAPI JonesMain_LogErrorToFile(const char* pErrorText);

int J3DAPI JonesDevDialog_ShowDevDialog(HWND hWnd, JonesState* pConfig);

size_t J3DAPI JonesDevDialog_FindClosestVideoMode(const StdDisplayEnvironment* pList, const StdVideoMode* pVideoMode, size_t deviceNum);

J3D_EXTERN_C_END
#endif // JONES3D_JONESMAIN_H
