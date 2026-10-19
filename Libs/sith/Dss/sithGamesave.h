#ifndef SITH_SITHGAMESAVE_H
#define SITH_SITHGAMESAVE_H
#include <j3dcore/j3d.h>


#include <sith/types.h>
#include <sith/Main/sithMain.h>


J3D_EXTERN_C_START

#define SITHSAVEGAME_THUMB_WIDTH  128
#define SITHSAVEGAME_THUMB_HEIGHT 96

void J3DAPI sithGamesave_Startup(void);
void J3DAPI sithGamesave_Shutdown(void);

void J3DAPI sithGamesave_Open(void);
void J3DAPI sithGamesave_CloseRestore(void);
void J3DAPI sithGamesave_Close(void);

SithGameStatistics* J3DAPI sithGamesave_GetGameStatistics(void);
int J3DAPI sithGamesave_LockGameStatistics(void);
int J3DAPI sithGamesave_UnlockGameStatistics(void);

int J3DAPI sithGamesave_Save(const char* pFilename, int bOverwrite);
int J3DAPI sithGamesave_Restore(const char* pFilename, int bNotifyCog);
int J3DAPI sithGamesave_Process(void);

void J3DAPI sithGamesave_NotifyRestored(const char* pFilePath); // Notify master cog the game has been restored from a save file

int J3DAPI sithGamesave_SetThumbnailImage(tVBuffer* pVBuffer);
HBITMAP J3DAPI sithGamesave_CreateThumbnail(void);
HBITMAP J3DAPI sithGamesave_LoadThumbnail(const char* pFilename); // Function reads the thumbnail from the nds save file

void J3DAPI sithGamesave_SetPreviousLevelFilename(const char* pFilename);
const char* J3DAPI sithGamesave_GetPreviousLevelFilename(void);

const char* J3DAPI sithGamesave_GetLastFilename(void);
int J3DAPI sithGamesave_LoadLevelFilename(const char* pNdsFilePath, char* pDestFilename); // Function reads the level filename from the nds save file

int J3DAPI sithGamesave_GetState(const char** ppCurFilename);
void J3DAPI sithGamesave_SetSaveGameCallback(SithGameSaveCallback pfCallback);

J3D_EXTERN_C_END
#endif // SITH_SITHGAMESAVE_H
