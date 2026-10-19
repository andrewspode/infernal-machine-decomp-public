#include "sithMulti.h"

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithMulti_countfit_1, 15)
CF_PROTO(sithMulti_countfit_2, 15)
CF_PROTO(sithMulti_countfit_3, 15)
CF_PROTO(sithMulti_countfit_4, 15)
CF_PROTO(sithMulti_countfit_5, 15)
CF_PROTO(sithMulti_countfit_6, 15)
CF_PROTO(sithMulti_countfit_7, 15)
CF_PROTO(sithMulti_countfit_8, 4)
/* end count-fit */

// Written by the discarded OpenGame (debug 0x004694D0 stores 0 to both directly); their modules' headers do not
// declare them (a sithThing.h/sithSurface.h extern changes other TUs' code away from retail's),
// so the declarations stand here (placement inferred).
extern size_t sithThing_numUnsyncedThings;
extern size_t sithSurface_numUnsyncedSurfaces;

size_t sithMulti_tickRate = 70;

int sithMulti_numUpdatedSurfaces = 0;
int sithMulti_lastUpdateIdx = 0;
int sithMulti_playerWelcomeState = 0;
int sithMulti_numUpdatedSectors = 0;
int sithMulti_aRemovedStaticThings[256] = { 0 };
unsigned int sithMulti_curWelcomePlayerNum = 0;
int sithMulti_numUpdatedThings = 0;
int sithMulti_quitGameState = 0;
int sithMulti_checksum = 0;
int sithMulti_dword_56F980 = 0;
int bWelcomingPlayer = 0;
DPID sithMulti_newPlayerId = 0;
int sithMulti_bSyncScores = 0;
SithMultiNewPlayerJoinedCallback sithMulti_pfNewPlayerJoinedCallback = NULL;
unsigned int sithMulti_msecPingStartTime = 0;
unsigned int sithMulti_msecQuitGameTime = 0;
size_t sithMulti_numRemovedStaticThings = 0;
unsigned int sithMulti_msecLastSyncScoreTime = 0;
unsigned int sithMulti_msecWelcomeUpdateInterval = 0;

SithMultiServer sithMulti_server;
SithMessage sithMulti_message;

void J3DAPI sithMulti_FinishWelcomingPlayer(void);
void J3DAPI sithMulti_StartupServer(void);
void J3DAPI sithMulti_StartupClient(void);
void J3DAPI sithMulti_FreeThings(SithWorld* pWorld);

/* Linker-discarded in retail. Body from the Indy debug build (0x004694D0, assert at line 139): the multiplayer
   level start (sithMain's discarded multiplayer level open calls it). The session seed (sithMulti_dword_56F980) is set
   by CreatePlayer/InitTick below. */
void J3DAPI sithMulti_OpenGame(void)
{
    SithWorld* pWorld;

    pWorld = sithWorld_pCurrentWorld;
#line 139
    SITH_ASSERTREL(pWorld != NULL);

    sithMulti_quitGameState = 0;
    sith_mode.subModeFlags |= 1;
    sithMessage_outputstream |= SITHMESSAGE_STREAM_NET;
    sithMessage_inputstream |= SITHMESSAGE_STREAM_NET;
    sithMulti_FreeThings(pWorld);
    sithMulti_checksum = sithWorld_CalcWorldChecksum(sithWorld_pCurrentWorld, sithMulti_dword_56F980);

    sithThing_numUnsyncedThings     = 0;
    sithSurface_numUnsyncedSurfaces = 0;
    sithSector_numModifiedSectors   = 0;
    bWelcomingPlayer = 0;

    if ( stdComm_IsGameHost() )
    {
        sithMulti_StartupServer();
    }
    else
    {
        sithMulti_StartupClient();
    }
}

void J3DAPI sithMulti_CloseGame(void)
{
    if ( stdComm_IsGameHost() && bWelcomingPlayer )
    {
        sithMulti_StopWelcomingPlayer(1);
    }

    sithMessage_outputstream &= ~SITHMESSAGE_STREAM_NET;
    sithMessage_inputstream &= ~SITHMESSAGE_STREAM_NET;
    sithEvent_RegisterTask(2u, NULL, 0, SITHEVENT_TASKDISABLED);
    sithMessage_CloseGame();

    memset(&sithMulti_server, 0, sizeof(sithMulti_server));
}

int J3DAPI sithMulti_CheckPlayers(int msecTime, SithEventParams* pParam)
{
    size_t i;

    if ( sithWorld_pCurrentWorld && sithPlayer_pLocalPlayerThing && (sith_mode.subModeFlags & 8) == 0 )
    {
        sithDSSThing_Pos(sithPlayer_pLocalPlayerThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, 0);
    }

    if ( stdComm_IsGameHost() )
    {
        for ( i = 0; i < sithPlayer_numPlayers; ++i )
        {
            if ( sithPlayer_aPlayers[i].playerNetId != sithMessage_localPlayerId
                && (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0
                && sithTime_msecGameTime > sithPlayer_aPlayers[i].msecLastCommTime + 45000 )
            {
#line 240
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Haven't heard from player %d, kicking them out of the game.\n", i, 0, 0, 0);
                sithMulti_QuitPlayer(sithPlayer_aPlayers[i].playerNetId);
                sithMulti_RemovePlayer(i);
            }
        }

        if ( sithMulti_msecLastSyncScoreTime + 10000 < sithTime_clockTime )
        {
            sithMulti_msecLastSyncScoreTime = sithTime_clockTime;
            sithMulti_SyncScores();
        }
    }
    else if ( sithTime_msecGameTime > sithPlayer_aPlayers[0].msecLastCommTime + 45000 )
    {
        sithPlayer_aPlayers[0].msecLastCommTime = sithTime_msecGameTime;
#line 262
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Haven't heard from server, exiting the game.\n", 0, 0, 0, 0);
        sithMulti_RemovePlayer(0);
    }

    return 1;
}

/* Linker-discarded in retail: the eight functions below, from the Indy debug build in its file order (0x00469780..0x00469A3A,
   between CheckPlayers and ProcessPlayerLost; the retail __LINE__ span 262..518 holds them). Names: Jedi Knight's
   sithMulti_CreatePlayer/InitTick (same roles: host and join set-up of the session settings and tick rate),
   OpenJones3D's GetTickRate/SetTickRate (sithCommand's "tick" verb calls them), JK's FreeThings; StartupServer/
   StartupClient are inferred. The debug build's sithMulti_EndLevel (0x00469935, "Multi_EndLevel called." at line
   373) is debug-only: it tests stdComm's lobby flag, which has no storage in retail. */
HRESULT J3DAPI sithMulti_CreatePlayer(const wchar_t* pSessionName, const wchar_t* pPassword, const char* pLevelName, size_t tickRate)
{
    HRESULT result;
    StdCommGame settings;

    memset(&settings, 0, sizeof(settings));
    wcsncpy(settings.aSessionName, pSessionName, STD_ARRAYLEN(settings.aSessionName) - 1);
    settings.aSessionName[STD_ARRAYLEN(settings.aSessionName) - 1] = 0;
    strncpy(settings.aSomething, pLevelName, STD_ARRAYLEN(settings.aSomething) - 1);
    settings.aSomething[STD_ARRAYLEN(settings.aSomething) - 1] = 0;
    wcsncpy(settings.aPassword, pPassword, STD_ARRAYLEN(settings.aPassword) - 1);
    settings.aPassword[STD_ARRAYLEN(settings.aPassword) - 1] = 0;

    settings.opt3       = tickRate;
    settings.maxPlayers = STD_ARRAYLEN(sithPlayer_aPlayers);
    sithMulti_dword_56F980 = SITH_RAND() * 4294967296.0f;
    settings.opt1 = sithMulti_dword_56F980;
    settings.opt2 = 10;

    result = sithMessage_StartGame(&settings);
    if ( result )
    {
        return result;
    }

    memset(&sithMulti_server, 0, sizeof(sithMulti_server));
    sithMulti_server.idServer = sithMessage_localPlayerId;
    sithMulti_tickRate = tickRate;
    sithEvent_RegisterTask(2u, sithMulti_CheckPlayers, tickRate, SITHEVENT_TASKINTERVAL);
    return 0;
}

void J3DAPI sithMulti_InitTick(const StdCommGame* pSettings)
{
    sithMulti_server.idServer = 0;
    sithMulti_tickRate = STDMATH_CLAMP(pSettings->opt3, 50, 300);
    memset(&sithMulti_server, 0, sizeof(sithMulti_server));
    sithMulti_dword_56F980 = pSettings->opt1;
}

size_t J3DAPI sithMulti_GetTickRate(void)
{
    return sithMulti_tickRate;
}

void J3DAPI sithMulti_SetTickRate(size_t tickRate)
{
    sithMulti_tickRate = tickRate;
}

#ifdef J3D_DEBUG
/* Debug-only: reads stdComm's debug-only lobby flag. */
#line 371
void J3DAPI sithMulti_EndLevel(void)
{
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Multi_EndLevel called.\n", 0, 0, 0, 0);
    if ( stdComm_IsGameHost() )
    {
        if ( bWelcomingPlayer )
        {
            sithMulti_StopWelcomingPlayer(1);
        }

        if ( stdComm_IsLobbied() )
        {
            sithMulti_LobbyMessage();
        }
    }

    sithMessage_Flush();
}
#endif

void J3DAPI sithMulti_StartupServer(void)
{
    size_t i;
    for ( i = 0; i < STD_ARRAYLEN(sithPlayer_aPlayers); ++i )
    {
        sithPlayer_Reset(i);
        sithPlayer_HidePlayer(i);
    }

    sithPlayer_ShowPlayer(0, sithMessage_localPlayerId);
    sithPlayer_SetLocalPlayer(0);
    sithMessage_Flush();
}

void J3DAPI sithMulti_StartupClient(void)
{
    size_t i;
    for ( i = 0; i < STD_ARRAYLEN(sithPlayer_aPlayers); ++i )
    {
        sithPlayer_Reset(i);
        sithPlayer_HidePlayer(i);
    }

    sithMessage_Flush();
    memset(&sithMulti_server, 0, sizeof(sithMulti_server));
}

void J3DAPI sithMulti_FreeThings(SithWorld* pWorld)
{
    int bHost;
    int i;

    bHost = stdComm_IsGameHost();
    sithMulti_numRemovedStaticThings = 0;
    for ( i = 0; i <= pWorld->lastThingIdx; ++i )
    {
        if ( pWorld->aThings[i].type == SITH_THING_ACTOR )
        {
            sithThing_RemoveThing(pWorld, &pWorld->aThings[i]);
        }
        else if ( !bHost )
        {
            pWorld->aThings[i].flags |= SITH_TF_REMOTE;
        }
    }
}

void J3DAPI sithMulti_ProcessPlayerLost(DPID idPlayer)
{
    int playerNum;
    if ( bWelcomingPlayer && idPlayer == sithMulti_newPlayerId )
    {
        sithMulti_StopWelcomingPlayer(1);
    }

    playerNum = sithMulti_GetPlayerIndexByID(idPlayer);
#line 518
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Player Lost: id=%x, playerNum=%d.\n", idPlayer, playerNum, 0, 0);
    if ( playerNum < 0 )
    {
        return;
    }

    sithMulti_RemovePlayer(playerNum);
}

void J3DAPI sithMulti_RemovePlayer(unsigned int playerNum)
{
    wchar_t awText[128];

#line 548
    SITH_ASSERTREL(playerNum < sithPlayer_numPlayers);
#line 550
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Removing player %d.\n", playerNum, 0, 0, 0);

    _snwprintf(awText, 128, sithString_GetString("%s_HAS_LEFT_THE_GAME"), sithPlayer_aPlayers[playerNum].awName);
    sithConsole_PrintWString(awText);

    if ( sithPlayer_aPlayers[playerNum].playerNetId == sithMulti_server.idServer )
    {
        sithConsole_PrintWString(sithString_GetString("SERVER_LEFT_GAME"));
        sithMulti_server.idServer = 0;
    }

    sithSoundMixer_StopSoundThing(sithPlayer_aPlayers[playerNum].pThing, 0);
    sithPlayer_HidePlayer(playerNum);

    if ( stdComm_IsGameHost() )
    {
        sithCog_BroadcastMessage(SITHCOG_MSG_LEAVE, SITHCOG_SYM_REF_THING, sithPlayer_aPlayers[playerNum].pThing->idx, SITHCOG_SYM_REF_NONE, playerNum);
    }
}

int J3DAPI sithMulti_SendWelcome(DPID idPlayer, int playerNum, DPID idTo)
{
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHINT32(playerNum);
    SITHDSS_PUSHINT32(idPlayer);

#line 596
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Sending game welcome of player %d to playerID %x.\n", playerNum, idTo, 0, 0);

    wcsncpy((wchar_t*)pCurDSSMsgData_, sithPlayer_aPlayers[playerNum].awName, 15);
    ((wchar_t*)pCurDSSMsgData_)[15] = 0;
    pCurDSSMsgData_ += 32;

    sithMulti_message.type = SITHDSS_WELCOME;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_NET, 1);
}

int J3DAPI sithMulti_ProcessWelcome(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    int playerNum;
    DPID playerId;
    size_t i;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    playerNum = SITHDSS_POPINT32();
    playerId  = SITHDSS_POPUINT32();

#line 632
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Processing welcome from player %d (id=%x).\n", playerNum, playerId, 0, 0);

    wcsncpy(sithPlayer_aPlayers[playerNum].awName, (const wchar_t*)pCurDSSMsgData_, 15);
    sithPlayer_aPlayers[playerNum].awName[15] = 0;
    pCurDSSMsgData_ += 32;

    if ( playerId == sithMessage_localPlayerId )
    {
#line 639
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "DSS::welcome packet addressed to me!.\n", 0, 0, 0, 0);
        if ( (sith_mode.subModeFlags & 8) != 0 )
        {
#line 642
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Joining the game loop.\n", 0, 0, 0, 0);

            for ( i = 0; i < sithPlayer_numPlayers; ++i )
            {
                sithPlayer_aPlayers[i].msecLastCommTime = sithTime_msecGameTime;
            }

            sith_mode.subModeFlags &= ~8;
            sithThing_LoadPostProcess(sithWorld_pCurrentWorld);
            sithPlayer_ShowPlayer(playerNum, playerId);
            sithPlayer_SetLocalPlayer(playerNum);

#line 660
            SITH_ASSERTREL(sithMulti_tickRate);
            sithEvent_RegisterTask(2u, sithMulti_CheckPlayers, sithMulti_tickRate, SITHEVENT_TASKINTERVAL);
            sithMessage_StopProcessMessages();
        }
        else
        {
#line 670
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Got duplicate Welcome packet from host, ignored.\n", 0, 0, 0, 0);
        }
    }
    else
    {
        if ( (sithPlayer_aPlayers[playerNum].flags & SITH_PLAYER_JOINEDGAME) != 0 )
        {
#line 677
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Received duplicate welcome packet from player %d (id=%x).\n", playerNum, playerId, 0, 0);
            return 1;
        }

        sithPlayer_ShowPlayer(playerNum, playerId);
        sithMulti_ProcessPlayerJoin(playerNum);
        if ( stdComm_IsGameHost() )
        {
            sithMulti_SyncScores();
        }
    }

    return 1;
}

void J3DAPI sithMulti_ProcessPlayerJoin(int playerNum)
{
    wchar_t awText[128];

    _snwprintf(awText, 128, sithString_GetString("%s_HAS_JOINED_THE_GAME"), sithPlayer_aPlayers[playerNum].awName);
    sithConsole_PrintWString(awText);

    sithPlayer_aPlayers[playerNum].msecLastCommTime = sithTime_msecGameTime;
    if ( stdComm_IsGameHost() )
    {
        sithCog_BroadcastMessage(SITHCOG_MSG_JOIN, SITHCOG_SYM_REF_THING, sithPlayer_aPlayers[playerNum].pThing->idx, SITHCOG_SYM_REF_NONE, playerNum);
    }

    if ( sithMulti_pfNewPlayerJoinedCallback )
    {
        sithMulti_pfNewPlayerJoinedCallback();
    }

    sithDSSThing_UpdateState(sithPlayer_pLocalPlayerThing, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
}

void J3DAPI sithMulti_SyncPlayers(DPID idTo, uint32_t dpFlags)
{
    uint8_t* pCurDSSMsgData_;
    size_t i;
    rdModel3* pMdl;
    char paStr[64];

        pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHUINT32(sithMulti_server.idServer);
    SITHDSS_PUSHUINT16((uint16_t)sithPlayer_numPlayers);

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        SITHDSS_PUSHUINT32(sithPlayer_aPlayers[i].flags);
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_UNKNOWN_04) != 0 )
        {
            SITHDSS_PUSHUINT32(sithPlayer_aPlayers[i].playerNetId);

            stdUtil_ToAStringEx(paStr, sithPlayer_aPlayers[i].awName, 15);
            paStr[15] = 0;

            strncpy((char*)pCurDSSMsgData_, paStr, 15);
            pCurDSSMsgData_[15] = 0;
            pCurDSSMsgData_ += 16;
        }
    }

    sithMulti_message.type = SITHDSS_SYNCPLAYERS;
    SITHDSS_ENDOUT;
    sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_NET, dpFlags);

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( sithPlayer_aPlayers[i].playerNetId && sithMessage_localPlayerId && sithPlayer_aPlayers[i].playerNetId != sithMessage_localPlayerId )
        {
            pMdl = sithModel_Load("gen_vi.3do", 1);
            if ( !pMdl )
            {
#line 773
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Model %s not found, using default.\n", "gen_vi.3do", 0, 0, 0);
            }

            sithThing_SetThingModel(sithPlayer_aPlayers[i].pThing, pMdl);
        }
    }
}

int J3DAPI sithMulti_ProcessSyncPlayers(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    size_t i;
    rdModel3* pMdl;
    SithPlayerFlag prevFlags;
    char paStr[64];

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];

    memset(&sithMulti_server, 0, sizeof(sithMulti_server));
    sithMulti_server.msecSyncTime = stdPlatform_GetTimeMsec();

    sithMulti_server.idServer = SITHDSS_POPUINT32();
    sithPlayer_numPlayers = SITHDSS_POPUINT16();

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        prevFlags = sithPlayer_aPlayers[i].flags;
        sithPlayer_aPlayers[i].flags = SITHDSS_POPUINT32();
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_UNKNOWN_04) != 0 )
        {
            sithPlayer_aPlayers[i].playerNetId = SITHDSS_POPUINT32();

            if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && (prevFlags & SITH_PLAYER_JOINEDGAME) == 0 && (sith_mode.subModeFlags & 8) == 0 )
            {
#line 828
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Got a new player but we missed the welcome packet, apparently.\n", 0, 0, 0, 0);
                sithMulti_ProcessPlayerJoin(i);
            }
            else if ( !sithPlayer_aPlayers[i].playerNetId && (prevFlags & SITH_PLAYER_JOINEDGAME) != 0 && (sith_mode.subModeFlags & 8) == 0 )
            {
#line 833
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Noticed that player %d was removed by the server.  Removing locally.\n", i, 0, 0, 0);
                sithMulti_RemovePlayer(i);
            }

            strncpy(paStr, (const char*)pCurDSSMsgData_, 15);
            paStr[15] = 0;
            pCurDSSMsgData_ += 16;

            stdUtil_ToWStringEx(sithPlayer_aPlayers[i].awName, paStr, 15);
            sithPlayer_aPlayers[i].awName[15] = 0;
        }

        if ( sithPlayer_aPlayers[i].playerNetId && sithMessage_localPlayerId && sithPlayer_aPlayers[i].playerNetId != sithMessage_localPlayerId )
        {
            pMdl = sithModel_Load("gen_vi.3do", 1);
            if ( !pMdl )
            {
#line 850
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Model %s not found, using default.\n", "gen_vi.3do", 0, 0, 0);
                return 1;
            }

            sithThing_SetThingModel(sithPlayer_aPlayers[i].pThing, pMdl);
        }
    }

    return 1;
}

/* Linker-discarded in retail. Body from the Indy debug build (0x0046A4C7): the join request ProcessJoinRequest reads
   (level name, player name, world checksum). */
int J3DAPI sithMulti_SendJoinRequest(DPID idTo)
{
    uint8_t* pCurDSSMsgData_;
    int result;

    pCurDSSMsgData_ = &sithMulti_message.data[0];
    strncpy((char*)pCurDSSMsgData_, sithWorld_pCurrentWorld->aName, 63);
    pCurDSSMsgData_[63] = 0;
    pCurDSSMsgData_ += 64;

    wcsncpy((wchar_t*)pCurDSSMsgData_, sithPlayer_GetLocalPlayerName(), 15);
    ((wchar_t*)pCurDSSMsgData_)[15] = 0;
    pCurDSSMsgData_ += 32;

    SITHDSS_PUSHINT32(sithMulti_checksum);

    SITHDSS_ENDOUT;
    sithMulti_message.type = SITHDSS_JOIN;
    result = sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_NET, 0);
    return result;
}

int J3DAPI sithMulti_ProcessJoinRequest(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    DPID playerId;
    size_t i;
    int clientChecksum;
    char aLevelName[64];

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    playerId = sithMessage_GetSenderID();

    if ( !stdComm_IsGameHost() )
    {
#line 926
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Ignoring JoinRequest from player %d, ignoring.\n", playerId, 0, 0, 0);
        return 1;
    }

    strncpy(aLevelName, (const char*)pCurDSSMsgData_, 63);
    aLevelName[63] = 0;
    pCurDSSMsgData_ += 64;

    if ( _strcmpi(aLevelName, sithWorld_pCurrentWorld->aName) )
    {
#line 936
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got join request but its for the wrong level %s.\n", aLevelName, 0, 0, 0);
        sithMulti_FinishJoining(SITHMULTI_JOIN_WRONGLEVEL, 0.0f, playerId);
        return 1;
    }

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( sithPlayer_aPlayers[i].playerNetId == playerId )
        {
            break;
        }
    }

    if ( i < sithPlayer_numPlayers )
    {
#line 950
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Received knock from player %d, already in game.\n", playerId, 0, 0, 0);
        sithMulti_SendWelcome(playerId, i, playerId);
        return 1;
    }

    sithMessage_CheckMembershipList(playerId);
    if ( sithMulti_quitGameState )
    {
        sithMulti_FinishJoining(SITHMULTI_JOIN_CANCEL, 0.0f, playerId);
        return 1;
    }

    if ( bWelcomingPlayer )
    {
        if ( sithMulti_newPlayerId != playerId )
        {
#line 970
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Deferring join request from player %x, busy already.\n", playerId, 0, 0, 0);
            sithMulti_FinishJoining(SITHMULTI_JOIN_BUSY, 0.0f, playerId);
        }
        else
        {
            sithMulti_FinishJoining(SITHMULTI_JOIN_OK, 0.5f, playerId);
        }

        return 1;
    }

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_PLACED) != 0 && !sithPlayer_aPlayers[i].playerNetId )
        {
            break;
        }
    }

    if ( i == sithPlayer_numPlayers )
    {
#line 993
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Received join request from player %d, but the game is full.. sucks for him.\n", playerId, 0, 0, 0);
        sithMulti_FinishJoining(SITHMULTI_JOIN_GAMEFULL, 0.0f, playerId);
        return 1;
    }

    sithMulti_curWelcomePlayerNum = i;
    if ( stdComm_UpdatePlayers(0) )
    {
#line 1005
        stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Unable to UpdatePlayers List.\n", 0, 0, 0, 0);
        return 1;
    }

    if ( stdComm_VerifyPlayer(playerId) )
    {
#line 1010
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got a Knock message, but the dude isn't enumerating.\n", 0, 0, 0, 0);
        return 1;
    }

    sithPlayer_Reset(sithMulti_curWelcomePlayerNum);
    wcsncpy(sithPlayer_aPlayers[sithMulti_curWelcomePlayerNum].awName, (const wchar_t*)pCurDSSMsgData_, 15);
    sithPlayer_aPlayers[sithMulti_curWelcomePlayerNum].awName[15] = 0;
    pCurDSSMsgData_ += 32;

    clientChecksum = SITHDSS_POPINT32();
#line 1026
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "My checksum=%x, client=%x.\n", sithMulti_checksum, clientChecksum, 0, 0);
    if ( sithMulti_checksum != clientChecksum )
    {
#line 1030
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got join request with wrong checksum, %x != %x.\n", sithMulti_checksum, clientChecksum, 0, 0);
        sithMulti_FinishJoining(SITHMULTI_JOIN_WRONGCHECKSUM, 0.0f, playerId);
        return 1;
    }

    sithMulti_FinishJoining(SITHMULTI_JOIN_OK, 0.25f, playerId);
    sithMulti_SyncPlayers(playerId, 0);
    sithMulti_StartWelcomingPlayer(playerId);
#line 1050
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Starting welcome of player %d.\n", playerId, 0, 0, 0);
    return 1;
}

int J3DAPI sithMulti_FinishJoining(SithMultiJoinStatus code, float arg4, DPID playerId)
{
    uint8_t* pCurDSSMsgData_;

        pCurDSSMsgData_ = &sithMulti_message.data[0];

    SITHDSS_PUSHINT32(code);
    SITHDSS_PUSHFLOAT(arg4);

    SITHDSS_ENDOUT;
    sithMulti_message.type = SITHDSS_JOINSTATUS;
    return sithMessage_SendMessage(&sithMulti_message, playerId, SITHMESSAGE_STREAM_NET, 0);
}

/* Linker-discarded in retail. Body from the Indy debug build (0x0046AA27, assert at line 1110): the chat message
   ProcessChat reads. */
void J3DAPI sithMulti_SendChat(const char* pString, int arg0, int arg1)
{
    uint8_t* pCurDSSMsgData_;
    size_t len;

    pCurDSSMsgData_ = &sithMulti_message.data[0];
#line 1110
    SITH_ASSERTREL(pString);

    len = strlen(pString) + 1;
    len = len < 128 ? len : 128;

    SITHDSS_PUSHINT32(arg0);
    SITHDSS_PUSHINT32(arg1);
    SITHDSS_PUSHUINT32(len);
    strncpy((char*)pCurDSSMsgData_, pString, len - 1);
    pCurDSSMsgData_[len - 1] = 0;
    pCurDSSMsgData_ += len;

    SITHDSS_ENDOUT;
    sithMulti_message.type = SITHDSS_CHAT;
    sithMessage_SendMessage(&sithMulti_message, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_NET, 1);
}

int J3DAPI sithMulti_ProcessChat(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    size_t len;
    int playerCount;
    int senderCnt;
    char aText[132];

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    senderCnt = SITHDSS_POPINT32();
    playerCount = SITHDSS_POPINT32();
    len = SITHDSS_POPUINT32();
    len = len < 128 ? len : 128;

    strncpy(aText, (const char*)pCurDSSMsgData_, len - 1);
    aText[len - 1] = 0;
    pCurDSSMsgData_ += len;
    aText[len + 1] = 0;

    if ( playerCount >= 0 )
    {
        sprintf(std_genBuffer, "%S says '%s'", sithPlayer_aPlayers[playerCount].awName, aText);
    }
    else
    {
        sprintf(std_genBuffer, "%s", aText);
    }

    sithConsole_PrintString(std_genBuffer);
    return 1;
}

/* Linker-discarded in retail (ProcessPong reads sithMulti_msecPingStartTime, which no retail code writes). Body from the
   Indy debug build (0x0046AC15; sithCommand's debug-only "ping" verb calls it). */
int J3DAPI sithMulti_Ping(DPID idTo)
{
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = &sithMulti_message.data[0];
    sithMulti_msecPingStartTime = sithTime_msecGameTime;
    SITHDSS_PUSHUINT32(sithMulti_msecPingStartTime);

    SITHDSS_ENDOUT;
    sithMulti_message.type = SITHDSS_PING;
    return sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_NET, 0);
}

int J3DAPI sithMulti_ProcessPing(const SithMessage* pMsg)
{
    ((SithMessage*)pMsg)->type = SITHDSS_PONG;
    sithMessage_SendMessage(&sithMulti_message, sithMessage_GetSenderID(), SITHMESSAGE_STREAM_NET, 0);
    return 1;
}

int J3DAPI sithMulti_ProcessPong(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    DPID currentsenderId;
    size_t i;
    unsigned int msecPing;
    unsigned int currentmsecStart;

    currentsenderId = sithMessage_GetSenderID();
    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    currentmsecStart = SITHDSS_POPUINT32();
    if ( currentmsecStart == sithMulti_msecPingStartTime )
    {
        for ( i = 0; i < sithPlayer_numPlayers; ++i )
        {
            if ( sithPlayer_aPlayers[i].playerNetId == currentsenderId )
            {
                msecPing = sithTime_msecGameTime - sithMulti_msecPingStartTime;
                sprintf(std_genBuffer, "Ping time to %S is %d msec", sithPlayer_aPlayers[i].awName, msecPing);
                sithConsole_PrintString(std_genBuffer);
#line 1266
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Ping reply from player %d - %d msec.\n", i, msecPing, 0, 0);
                break;
            }
        }
    }

    return 1;
}

void J3DAPI sithMulti_QuitPlayer(DPID id)
{
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = &sithMulti_message.data[0];
    if ( !stdComm_IsGameHost() )
    {
        return;
    }

    SITHDSS_PUSHUINT32(id);

    sithMulti_message.length = sizeof(id);
    sithMulti_message.type   = SITHDSS_QUIT;
    sithMessage_SendMessage(&sithMulti_message, id, SITHMESSAGE_STREAM_NET, 1);
}

int J3DAPI sithMulti_ProcessQuit(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    DPID currentsenderId;
    DPID playerId;
    int playerNum;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    currentsenderId = sithMessage_GetSenderID();
    playerId = SITHDSS_POPUINT32();
    if ( currentsenderId != sithMulti_server.idServer )
    {
#line 1317
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Got QUIT message from non-server, ignored.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( playerId == sithMessage_localPlayerId )
    {
        if ( sithMulti_quitGameState == 2 )
        {
            return 1;
        }

        sithConsole_PrintWString(sithString_GetString("MULTI_EJECTED"));
        sithMulti_QuitGame(sithTime_msecGameTime + 5000, 2);
    }
    else
    {
        playerNum = sithPlayer_GetPlayerNum(playerId);
        if ( playerNum >= 0 )
        {
            sithMulti_RemovePlayer(playerNum);
        }
    }

    return 1;
}

void J3DAPI sithMulti_Update(unsigned int msecDeltaTime)
{
    int i;
    unsigned int numUpdates;

    if ( !stdComm_IsGameActive() )
    {
        return;
    }

    sithThing_SyncThings();
    sithSurface_SyncSurfaces();
    sithSector_SyncSectors();

    if ( sithMulti_quitGameState && sithTime_msecGameTime > sithMulti_msecQuitGameTime )
    {
        switch ( sithMulti_quitGameState )
        {
            case 2:
                sithMulti_quitGameState = 0;
                break;

            case 1:
                sithMulti_quitGameState = 0;
                break;
        }

        return;
    }

    if ( !stdComm_IsGameHost() )
    {
        return;
    }

    if ( sithMulti_bSyncScores )
    {
        sithMulti_bSyncScores = 0;
        sithMulti_SyncPlayers(SITHMESSAGE_SENDTOJOINEDPLAYERS, 0);
    }

    if ( !bWelcomingPlayer )
    {
        return;
    }

    if ( sithMulti_quitGameState )
    {
        sithMulti_StopWelcomingPlayer(1);
        return;
    }

    msecDeltaTime += sithMulti_msecWelcomeUpdateInterval;
    numUpdates = msecDeltaTime / 60;
    sithMulti_msecWelcomeUpdateInterval = msecDeltaTime - 60 * numUpdates;

    for ( i = 0; i < numUpdates; ++i )
    {
        switch ( sithMulti_playerWelcomeState )
        {
            case 2:
                sithMulti_UpdateSurfaces();
                ++sithMulti_numUpdatedSurfaces;
                break;

            case 1:
                sithMulti_UpdateSectors();
                ++sithMulti_numUpdatedSectors;
                break;

            case 3:
                sithMulti_UpdateThings();
                ++sithMulti_numUpdatedThings;
                break;

            case 4:
                sithMulti_UpdateRemovals();
                break;

            default:
                return;
        }
    }
}

void J3DAPI sithMulti_SyncScores(void)
{
    sithMulti_bSyncScores = 1;
}

int J3DAPI sithMulti_GetPlayerIndexByID(DPID playerID)
{
    size_t i;
    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( playerID == sithPlayer_aPlayers[i].playerNetId )
        {
            return i;
        }
    }

    return -1;
}

/* Linker-discarded in retail: ProcessPlayerJoin-side code calls sithMulti_pfNewPlayerJoinedCallback, but no retail
   instruction stores it. Body and position from the Indy debug build (0x0046B07F, after GetPlayerIndexByID); the name is
   invented (Jedi Knight's one-store setter is OpenJKDF2's SetHandleridk). */
void J3DAPI sithMulti_SetNewPlayerJoinedCallback(SithMultiNewPlayerJoinedCallback pfCallback)
{
    sithMulti_pfNewPlayerJoinedCallback = pfCallback;
}

void J3DAPI sithMulti_ProcessKilledPlayer(const SithPlayer* pPlayer, const SithThing* pPlayerThing, const SithThing* pKiller)
{
    wchar_t awMessage[128];
    float rnd;

    if ( pKiller && pKiller->type == SITH_THING_PLAYER )
    {
#line 1512
        SITH_ASSERTREL(pKiller->thingInfo.actorInfo.pPlayer);
        if ( pKiller == pPlayerThing )
        {
            rnd = SITH_RAND();
            rnd = rnd * 4.0f;
            if ( rnd < 1.0f )
            {
                _snwprintf(awMessage, 128, sithString_GetString("%s_COMMITTED_SUICIDE0"), pPlayer->awName);
            }
            else if ( rnd < 2.0f )
            {
                _snwprintf(awMessage, 128, sithString_GetString("%s_COMMITTED_SUICIDE1"), pPlayer->awName);
            }
            else if ( rnd < 3.0f )
            {
                _snwprintf(awMessage, 128, sithString_GetString("%s_COMMITTED_SUICIDE2"), pPlayer->awName);
            }
            else
            {
                _snwprintf(awMessage, 128, sithString_GetString("%s_COMMITTED_SUICIDE3"), pPlayer->awName);
            }
            sithConsole_PrintWString(awMessage);
        }
        else
        {
            _snwprintf(awMessage, 128, sithString_GetString("%s_WAS_KILLED_BY_%s"), pPlayer->awName, pKiller->thingInfo.actorInfo.pPlayer->awName);
            sithConsole_PrintWString(awMessage);
        }
    }
    else
    {
        _snwprintf(awMessage, 128, sithString_GetString("%s_DIED"), pPlayer->awName);
        sithConsole_PrintWString(awMessage);
    }
}

void J3DAPI sithMulti_QuitGame(unsigned int msecTime, int state)
{
    if ( sithMulti_quitGameState == state && msecTime >= sithMulti_msecQuitGameTime )
    {
        return;
    }

    sithMulti_quitGameState = state;
    sithMulti_msecQuitGameTime = msecTime;
}

size_t J3DAPI sithMulti_Respawn(SithThing* pPlayer)
{
    size_t numTries = 0;
    size_t numFound = 0;
    size_t aPlayerNums[SITHPLAYER_MAX_PLAYERS];
    size_t i;
    size_t spawnIdx;
    size_t playerNum;
    SithCollision* pCollision;

#line 1579
    SITH_ASSERTREL(pPlayer && (pPlayer->type == SITH_THING_PLAYER));

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( (pPlayer->thingInfo.actorInfo.pPlayer->respawnMask & (1 << i)) == 0 )
        {
            aPlayerNums[numFound++] = i;
        }
    }

    if ( !numFound )
    {
#line 1594
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No respawn points unmasked for thing %s, using respawn 0.\n", pPlayer->aName, 0, 0, 0);
        return 0;
    }

    if ( numFound == 1 )
    {
        return aPlayerNums[0];
    }

    spawnIdx = (size_t)D3DMultiply(D3DVAL(SITH_RAND()), numFound);
    spawnIdx = STDMATH_CLAMP(spawnIdx, 0, numFound - 1);
    playerNum = aPlayerNums[spawnIdx];

    for ( ;; )
    {
        sithCollision_SearchForCollisions(sithPlayer_aPlayers[playerNum].pInSector, NULL, &sithPlayer_aPlayers[playerNum].orient.dvec, &rdroid_zeroVector3, 0.0f, pPlayer->collide.movesize, 0x482);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0
                && pCollision->pThingCollided->type == SITH_THING_PLAYER
                && (pCollision->pThingCollided->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0 )
            {
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( pCollision )
        {
#line 1627
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Tried to use respawn %d but was occupied by %s.\n", playerNum, pCollision->pThingCollided->aName, 0, 0);
            if ( numTries < numFound )
            {
                ++numTries;
                spawnIdx = (spawnIdx + 1) % numFound;
                playerNum = aPlayerNums[spawnIdx];
                continue;
            }

#line 1636
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "All available respawn positions occupied, using %d anyway.\n", playerNum, 0, 0, 0);
        }

        break;
    }

    return playerNum;
}

void J3DAPI sithMulti_RemoveStaticThing(int guid)
{
    if ( sithMulti_numRemovedStaticThings < 256 )
    {
        sithMulti_aRemovedStaticThings[sithMulti_numRemovedStaticThings++] = guid;
    }
    else
    {
#line 1654
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Limit %d on # of removed static things exceeded!", 256, 0, 0, 0);
    }
}

void J3DAPI sithMulti_UpdateSurfaces(void)
{
    SithSurface* pSurf;

    while ( (size_t)sithMulti_lastUpdateIdx < sithWorld_pCurrentWorld->numSurfaces )
    {
        pSurf = &sithWorld_pCurrentWorld->aSurfaces[sithMulti_lastUpdateIdx++];
        if ( (pSurf->flags & SITH_SURFACE_SYNC) != 0 )
        {
            sithDSS_SurfaceStatus(pSurf, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            break;
        }
    }

    if ( (size_t)sithMulti_lastUpdateIdx >= sithWorld_pCurrentWorld->numSurfaces )
    {
#line 1696
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Welcome of player %d switching to SECTORS.\n", sithMulti_newPlayerId, 0, 0, 0);
        sithMulti_playerWelcomeState = 1;
        sithMulti_lastUpdateIdx = 0;
    }
}

void J3DAPI sithMulti_UpdateSectors(void)
{
    SithSector* pSector;

    while ( (size_t)sithMulti_lastUpdateIdx < sithWorld_pCurrentWorld->numSectors )
    {
        pSector = &sithWorld_pCurrentWorld->aSectors[sithMulti_lastUpdateIdx++];
        if ( (pSector->flags & SITH_SECTOR_SYNC) != 0 )
        {
            sithDSS_SectorStatus(pSector, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            break;
        }

        if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 )
        {
            sithDSS_SectorFlags(pSector, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            break;
        }
    }

    if ( (size_t)sithMulti_lastUpdateIdx >= sithWorld_pCurrentWorld->numSectors )
    {
#line 1742
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Welcome of player %d switching to THINGS.\n", sithMulti_newPlayerId, 0, 0, 0);
        sithMulti_playerWelcomeState = 3;
        sithMulti_lastUpdateIdx = 0;
    }
}

void J3DAPI sithMulti_UpdateThings(void)
{
    SithThing* pThing;

    while ( sithMulti_lastUpdateIdx <= sithWorld_pCurrentWorld->lastThingIdx )
    {
        pThing = &sithWorld_pCurrentWorld->aThings[sithMulti_lastUpdateIdx++];
        if ( sithThing_CanSync(pThing) && pThing->type != SITH_THING_WEAPON && pThing->type != SITH_THING_EXPLOSION )
        {
            if ( (pThing->guid & 0xFFFF0000) == 0 )
            {
                sithDSSThing_UpdateState(pThing, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            }
            else
            {
                sithDSSThing_FullDescription(pThing, sithMulti_newPlayerId, SITHMESSAGE_STREAM_NET);
            }

            sithDSSThing_Pos(pThing, sithMulti_newPlayerId, 0);
            break;
        }
    }

    if ( sithMulti_lastUpdateIdx > sithWorld_pCurrentWorld->lastThingIdx )
    {
#line 1791
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Welcome of player %d switching to REMOVALS.\n", sithMulti_newPlayerId, 0, 0, 0);
        sithMulti_playerWelcomeState = 4;
        sithMulti_lastUpdateIdx = 0;
    }
}

void J3DAPI sithMulti_UpdateRemovals(void)
{
    if ( (size_t)sithMulti_lastUpdateIdx < sithMulti_numRemovedStaticThings )
    {
        sithDSSThing_DestroyThing(sithMulti_aRemovedStaticThings[sithMulti_lastUpdateIdx], sithMulti_newPlayerId);
        ++sithMulti_lastUpdateIdx;
    }

    if ( (size_t)sithMulti_lastUpdateIdx >= sithMulti_numRemovedStaticThings )
    {
        sithMulti_FinishWelcomingPlayer();
    }
}

void J3DAPI sithMulti_FinishWelcomingPlayer(void)
{
#line 1830
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Finished welcoming player %d, sending the welcome packet.\n", sithMulti_newPlayerId, 0, 0, 0);
#line 1833
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Update totals: %d surfaces, %d sectors, %d things.\n", sithMulti_numUpdatedSurfaces, sithMulti_numUpdatedSectors, sithMulti_numUpdatedThings, 0);

    sithPlayer_aPlayers[sithMulti_curWelcomePlayerNum].playerNetId = sithMulti_newPlayerId;
    sithMulti_SyncPlayers(sithMulti_newPlayerId, 1);
    sithMulti_SendWelcome(sithMulti_newPlayerId, sithMulti_curWelcomePlayerNum, sithMulti_newPlayerId);
    sithMulti_StopWelcomingPlayer(0);
    sithMulti_SyncScores();
}

void J3DAPI sithMulti_StartWelcomingPlayer(DPID playerId)
{
#line 1869
    SITH_ASSERTREL(!bWelcomingPlayer);
    SITH_ASSERTREL(playerId);
#line 1872
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Starting to welcome player %d.\n", playerId, 0, 0, 0);

    bWelcomingPlayer = 1;
    sithMulti_newPlayerId = playerId;
    sithMulti_playerWelcomeState = 2;
    sithMulti_lastUpdateIdx = 0;
    sithMulti_numUpdatedThings = sithMulti_numUpdatedSectors = sithMulti_numUpdatedSurfaces = 0;
}

void J3DAPI sithMulti_StopWelcomingPlayer(int bError)
{
#line 1901
    SITH_ASSERTREL(bWelcomingPlayer);
#line 1903
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Stopped welcoming player %d.\n", sithMulti_newPlayerId, 0, 0, 0);
    if ( bError && sithMulti_newPlayerId )
    {
        sithMulti_FinishJoining(SITHMULTI_JOIN_CANCEL, 0.0f, sithMulti_newPlayerId);
    }

    bWelcomingPlayer = 0;
    sithMulti_newPlayerId = 0;
    sithMulti_playerWelcomeState = 2;
    sithMulti_lastUpdateIdx = 0;
}

/* Linker-discarded in retail. Body and position from the Indy debug build (0x0046BA8D, the file's last function; called
   by EndLevel): the joined players' names, sent to the lobby. Jedi Knight's name for the same job. */
void J3DAPI sithMulti_LobbyMessage(void)
{
    uint8_t* pCurDSSMsgData_;
    size_t dataSize;
    size_t i;
    int numPlayers;
    SithPlayer* pPlayer;

    pCurDSSMsgData_ = &sithMulti_message.data[0];
    numPlayers = 0;
    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 )
        {
            ++numPlayers;
        }
    }

    SITHDSS_PUSHINT16(numPlayers);
    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        pPlayer = &sithPlayer_aPlayers[i];
        if ( (pPlayer->flags & SITH_PLAYER_JOINEDGAME) == 0 )
        {
            continue;
        }

        wcsncpy((wchar_t*)pCurDSSMsgData_, pPlayer->awName, STD_ARRAYLEN(pPlayer->awName) - 1);
        ((wchar_t*)pCurDSSMsgData_)[STD_ARRAYLEN(pPlayer->awName) - 1] = 0;
        pCurDSSMsgData_ += sizeof(pPlayer->awName);
    }

    dataSize = pCurDSSMsgData_ - &sithMulti_message.data[0];
    stdComm_SendLobbyMessage(&sithMulti_message.data[0], dataSize);
}
