#ifndef SITH_SITHPLAYER_H
#define SITH_SITHPLAYER_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

#define SITHPLAYER_MAX_PLAYERS 24

extern int sithPlayer_bPlayerInPor;

extern int sithPlayer_impFireType;

extern size_t sithPlayer_playerNum;

extern size_t sithPlayer_numPlayers;

extern SithThing* sithPlayer_pLocalPlayerThing;

extern SithPlayer* sithPlayer_pLocalPlayer;

extern int sithPlayer_bPlayerInvulnerable;

extern int sithPlayer_bInAetheriumSector;

extern int sithPlayer_bGuybrush;

extern unsigned int sithPlayer_curLevelNum;

extern int sithPlayer_bBonusMapBought;

extern SithPlayer sithPlayer_aPlayers[SITHPLAYER_MAX_PLAYERS];

extern float sithPlayer_impState;

void J3DAPI sithPlayer_Open(const wchar_t* awName);
void J3DAPI sithPlayer_Close(void);

void J3DAPI sithPlayer_NewPlayer(SithThing* pPlayer);
void J3DAPI sithPlayer_Reset(size_t playerNum);

void J3DAPI sithPlayer_PlacePlayers(SithWorld* pWorld);
void J3DAPI sithPlayer_HidePlayer(size_t playerNum);
int J3DAPI sithPlayer_ShowPlayer(size_t playerNum, DPID id);

void J3DAPI sithPlayer_SetLocalPlayer(size_t playerNum);
const wchar_t* J3DAPI sithPlayer_GetLocalPlayerName(void);

void J3DAPI sithPlayer_Update(SithPlayer* pPlayer, float secDetaTime);
int J3DAPI sithPlayer_GetPlayerNumByName(wchar_t* pwName);

int J3DAPI sithPlayer_GetPlayerNum(DPID playerId);
int J3DAPI sithPlayer_GetThingPlayerNum(const SithThing* pThing);

void J3DAPI sithPlayer_PlayerKilledAction(SithThing* pPlayerThing, const SithThing* pSrcThing);
void J3DAPI sithPlayer_KillPlayer(SithThing* pPlayerThing);

int J3DAPI sithPlayer_ToggleGuybrush(void);

void J3DAPI sithPlayer_IMPStartFiring(int fireType);
void J3DAPI sithPlayer_IMPEndFiring(int fireType);

J3D_EXTERN_C_END
#endif // SITH_SITHPLAYER_H
