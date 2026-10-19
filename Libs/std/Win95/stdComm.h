#ifndef STD_STDCOMM_H
#define STD_STDCOMM_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

#define STDCOMM_MAX_PLAYERS 24U
#define STDCOMM_MAX_GAMES   32U

int J3DAPI stdComm_Startup(void);
void J3DAPI stdComm_Shutdown(void);
int J3DAPI stdComm_InitConnection(int connNum);
int32_t J3DAPI stdComm_Send(DPID idFrom, DPID idTo, const void* pData, uint32_t size, uint32_t flags);
int32_t J3DAPI stdComm_Receive(DPID* pSender, void* pData, size_t* pLength);
int J3DAPI stdComm_GetGame(size_t gameNum, StdCommGame* pGame);

// Linker-discarded in retail; only referenced by sithComm.c's discarded sithMessage_StartGame.
HRESULT J3DAPI stdComm_CreateGame(const StdCommGame* pSettings);
HRESULT J3DAPI stdComm_SetGameParams(StdCommGame* pSettings);
int J3DAPI stdComm_JoinGame(size_t gameNum, const wchar_t* pPassword);
void J3DAPI stdComm_CloseGame(void);

int J3DAPI stdComm_RejoinSession(DPID* pPlayerId, const wchar_t* pPlayerName);
int J3DAPI stdComm_IsGameActive(void);
int J3DAPI stdComm_IsGameHost(void);
#ifdef J3D_DEBUG
int J3DAPI stdComm_IsLobbied(void);
uint8_t* J3DAPI stdComm_GetLobbyData(void);
#endif
int J3DAPI stdComm_UpdatePlayers(size_t gameNum);
int J3DAPI stdComm_EnumSessions(int bAsync, const wchar_t* pPassword);
void J3DAPI stdComm_EnumSessions2(void);
DPID J3DAPI stdComm_CreatePlayer(const wchar_t* pPlayerName);
void J3DAPI stdComm_DestroyPlayer(DPID playerId);
size_t J3DAPI stdComm_GetNumPlayers(void);
int J3DAPI stdComm_VerifyPlayer(DPID id);
DPID J3DAPI stdComm_GetPlayerID(size_t playerNum);
void J3DAPI stdComm_SendLobbyMessage(void* pData, uint32_t size);

J3D_EXTERN_C_END
#endif // STD_STDCOMM_H
