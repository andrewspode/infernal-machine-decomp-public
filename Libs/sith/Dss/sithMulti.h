#ifndef SITH_SITHMULTI_H
#define SITH_SITHMULTI_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

typedef struct sSithMultiServer
{
    DPID idServer;
    unsigned int msecSyncTime;
} SithMultiServer;

extern SithMultiServer sithMulti_server;

extern SithMessage sithMulti_message;

extern int sithMulti_quitGameState;

void J3DAPI sithMulti_OpenGame(void);
void J3DAPI sithMulti_CloseGame(void);

int J3DAPI sithMulti_CheckPlayers(int msecTime, SithEventParams* pParam);
HRESULT J3DAPI sithMulti_CreatePlayer(const wchar_t* pSessionName, const wchar_t* pPassword, const char* pLevelName, size_t tickRate);
void J3DAPI sithMulti_InitTick(const StdCommGame* pSettings);
size_t J3DAPI sithMulti_GetTickRate(void);
void J3DAPI sithMulti_SetTickRate(size_t tickRate);


void J3DAPI sithMulti_ProcessPlayerLost(DPID idPlayer);
void J3DAPI sithMulti_RemovePlayer(unsigned int playerNum);
int J3DAPI sithMulti_SendWelcome(DPID idPlayer, int playerNum, DPID idTo);
int J3DAPI sithMulti_ProcessWelcome(const SithMessage* pMsg);
void J3DAPI sithMulti_ProcessPlayerJoin(int playerNum);
void J3DAPI sithMulti_SyncPlayers(DPID idTo, uint32_t dpFlags);
int J3DAPI sithMulti_ProcessSyncPlayers(const SithMessage* pMsg);
int J3DAPI sithMulti_SendJoinRequest(DPID idTo);
int J3DAPI sithMulti_ProcessJoinRequest(const SithMessage* pMsg);
int J3DAPI sithMulti_FinishJoining(SithMultiJoinStatus code, float arg4, DPID playerId);
void J3DAPI sithMulti_SendChat(const char* pString, int arg0, int arg1);
int J3DAPI sithMulti_ProcessChat(const SithMessage* pMsg);

int J3DAPI sithMulti_Ping(DPID idTo);
int J3DAPI sithMulti_ProcessPing(const SithMessage* pMsg);
int J3DAPI sithMulti_ProcessPong(const SithMessage* pMsg);

void J3DAPI sithMulti_QuitPlayer(DPID id);
int J3DAPI sithMulti_ProcessQuit(const SithMessage* pMsg);

void J3DAPI sithMulti_Update(unsigned int msecDeltaTime);
void J3DAPI sithMulti_SyncScores(void);
int J3DAPI sithMulti_GetPlayerIndexByID(DPID idPlayer);
void J3DAPI sithMulti_SetNewPlayerJoinedCallback(SithMultiNewPlayerJoinedCallback pfCallback);
void J3DAPI sithMulti_ProcessKilledPlayer(const SithPlayer* pPlayer, const SithThing* pPlayerThing, const SithThing* pKiller);
void J3DAPI sithMulti_QuitGame(unsigned int msecTime, int state);
size_t J3DAPI sithMulti_Respawn(SithThing* pPlayer);
void J3DAPI sithMulti_RemoveStaticThing(int guid);
void J3DAPI sithMulti_UpdateSurfaces(void);
void J3DAPI sithMulti_UpdateSectors(void);
void J3DAPI sithMulti_UpdateThings(void);
void J3DAPI sithMulti_UpdateRemovals(void);
void J3DAPI sithMulti_StartWelcomingPlayer(DPID playerId);
void J3DAPI sithMulti_StopWelcomingPlayer(int bError);
void J3DAPI sithMulti_LobbyMessage(void);

J3D_EXTERN_C_END
#endif // SITH_SITHMULTI_H
