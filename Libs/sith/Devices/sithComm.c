#include "sithComm.h"

#include <sith/AI/sithAIUtil.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/World/sithVoice.h>

#include <std/General/stdConffile.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

static SithMessageProcessFunc sithMessage_aTypeFuncs[65] = { 0 };
static SithMessage outmsg                    = { 0 };

DPID sithMessage_localPlayerId                = 0;
SithMessageStream sithMessage_outputstream    = SITHMESSAGE_STREAM_NONE;
SithMessageStream sithMessage_inputstream     = SITHMESSAGE_STREAM_NONE;

static int sithMessage_bSturtup             = 0;
static size_t sithMessage_totalSentDataSize  = 0;
static size_t sithMessage_totalNetSent       = 0;
static DPID sithMessage_senderId             = 0;
static int sithMessage_bStopProcessMessages = 0;

int J3DAPI sithMessage_NetWrite(const SithMessage* pMsg, DPID idTo, uint32_t flags);
int J3DAPI sithMessage_ReceiveMessage(SithMessage* pMsg, DPID* pSender);
int J3DAPI sithMessage_ProcessCheckMembershipList(const SithMessage* pMsg);
void J3DAPI sithMessage_InitMessageHandlers(void);
int J3DAPI sithMessage_FileWrite(const SithMessage* pMessage);

void J3DAPI sithMessage_Startup(void)
{
    if ( sithMessage_bSturtup )
    {
#line 84
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Multiple startup on message system.\n", 0, 0, 0, 0);
        return;
    }

    sithMessage_totalSentDataSize    = 0;
    sithMessage_totalNetSent         = 0;
    sithMessage_senderId             = 0;
    sithMessage_bStopProcessMessages = 0;

    memset(sithMessage_aTypeFuncs, 0, sizeof(sithMessage_aTypeFuncs));
    sithMessage_InitMessageHandlers();

    sithMessage_bSturtup = 1;
}

void J3DAPI sithMessage_Shutdown(void)
{
    if ( !sithMessage_bSturtup )
    {
#line 108
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Shutdown of message system w/o startup.\n", 0, 0, 0, 0);
    }

    sithMessage_totalSentDataSize    = 0;
    sithMessage_totalNetSent         = 0;
    sithMessage_senderId             = 0;
    sithMessage_bStopProcessMessages = 0;
    memset(sithMessage_aTypeFuncs, 0, sizeof(sithMessage_aTypeFuncs));

    sithMessage_bSturtup = 0;
}

int J3DAPI sithMessage_SendMessage(SithMessage* pMessage, DPID idTo, SithMessageStream outstream, uint32_t dwDPFlags)
{
    int result;
    uint32_t stream = outstream & sithMessage_outputstream;

#line 129
    SITH_ASSERTREL(pMessage != NULL);
    SITH_ASSERTREL((pMessage->type >= 0) && (pMessage->type < 65));
    SITH_ASSERTREL(pMessage->length <= 3072);

    if ( stream == SITHMESSAGE_STREAM_NONE )
    {
        return 1;
    }

    pMessage->msecTime = sithTime_msecGameTime;
    if ( (stream & SITHMESSAGE_STREAM_NET) != 0 )
    {
        result = sithMessage_NetWrite(pMessage, idTo, dwDPFlags);
    }

    if ( (stream & SITHMESSAGE_STREAM_FILE) != 0 )
    {
        result = sithMessage_FileWrite(pMessage);
    }

    // Retail bug: result is uninitialized when neither stream bit is set (VC5 loads the outstream argument slot)
    return result;
}

void J3DAPI sithMessage_ProcessMessages(void)
{
    sithMessage_bStopProcessMessages = 0;
    if ( sithMessage_inputstream == SITHMESSAGE_STREAM_NONE )
    {
#line 164
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Called ProcessMessages without any input stream to read from.\n", 0, 0, 0, 0);
    }

    while ( !sithMessage_bStopProcessMessages && sithMessage_ReceiveMessage(&sithMulti_message, &sithMessage_senderId) == 1 )
    {
        int playerNum = sithPlayer_GetPlayerNum(sithMessage_senderId);
        if ( playerNum < 0 )
        {
            if ( sithMulti_message.type != SITHDSS_WELCOME
                && sithMulti_message.type != SITHDSS_JOIN
                && sithMulti_message.type != SITHDSS_SYNCPLAYERS
                && (sith_mode.subModeFlags & 8) == 0 )
            {
#line 181
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Ignored message type %d from non-active sender %x.\n", sithMulti_message.type, sithMessage_senderId, 0, 0);
                if ( stdComm_IsGameHost() )
                {
                    sithMulti_QuitPlayer(sithMessage_senderId);
                }

                continue;
            }
        }
        else
        {
            sithPlayer_aPlayers[playerNum].msecLastCommTime = sithTime_msecGameTime;
        }

        sithMessage_Process(&sithMulti_message);
    }
}

void J3DAPI sithMessage_StopProcessMessages(void)
{
    sithMessage_bStopProcessMessages = 1;
}

DPID J3DAPI sithMessage_GetSenderID(void)
{
    return sithMessage_senderId;
}

int J3DAPI sithMessage_NetWrite(const SithMessage* pMsg, DPID idTo, uint32_t flags)
{
    uint32_t curdataSize = pMsg->length + 4; // Message type plus padding
    int hr;
    size_t i;
    int bCurFailed;

#line 247
    SITH_ASSERTREL(pMsg);

    if ( idTo == SITHMESSAGE_SENDTOJOINEDPLAYERS )
    {
        hr = 0;
        for ( i = 0; i < sithPlayer_numPlayers; ++i )
        {
            if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) == 0 || sithPlayer_aPlayers[i].playerNetId == sithMessage_localPlayerId )
            {
                continue;
            }

            hr = stdComm_Send(sithMessage_localPlayerId, sithPlayer_aPlayers[i].playerNetId, &pMsg->type, curdataSize, flags);
            if ( hr )
            {
                bCurFailed = hr;
            }

            ++sithMessage_totalNetSent;
            sithMessage_totalSentDataSize += curdataSize;
        }
    }
    else
    {
        bCurFailed = stdComm_Send(sithMessage_localPlayerId, idTo, &pMsg->type, curdataSize, flags);
        ++sithMessage_totalNetSent;
        sithMessage_totalSentDataSize += curdataSize;
    }

    // Retail bug: bFailed is never initialized, so a broadcast without errors returns the flags argument slot
    return bCurFailed;
}

int J3DAPI sithMessage_ReceiveMessage(SithMessage* pMsg, DPID* pSender)
{
    size_t dataLen = STD_ARRAYLEN(pMsg->data) + 4; // type plus padding, see NetWrite
    int res;

#line 303
    SITH_ASSERTREL(pMsg != NULL);

    res = stdComm_Receive(pSender, &pMsg->type, &dataLen);
    if ( res == -1 ) // no more messages
    {
        return 0;
    }

    if ( res == 0 ) // success
    {
        pMsg->length   = dataLen - 4;
        pMsg->msecTime = sithTime_msecGameTime;
        return 1;
    }

    if ( (sith_mode.subModeFlags & 8) != 0 )
    {
        return 0;
    }

    switch ( res )
    {
        case 2: // Destroy player or group
            sithMulti_ProcessPlayerLost(*pSender);
            break;

        case 5: // Create player or group
            if ( stdComm_IsGameHost() )
            {
                sithMulti_SyncPlayers(*pSender, 1u);
            }

            break;

        case 8: // host
            sithMulti_server.idServer = sithMessage_localPlayerId;
            break;
    }

    return 0;
}

/* Linker-discarded, retail __LINE__ gap 303..561 (sithMessage_ReceiveMessage -> sithMessage_CheckMembershipList), where the original
   defined it. Body from the Indy debug build (0x00465C3A; lines 367/375/379):
   join the session, create the local player, close the game again if that fails. */
int J3DAPI sithMessage_JoinGame(size_t gameNum, const wchar_t* pPassword)
{
    int result;

    result = stdComm_JoinGame(gameNum, pPassword);
    if ( result )
    {
#line 367
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Problem opening game session %d.\n", gameNum, 0, 0, 0);
        return result;
    }

    sithMessage_localPlayerId = stdComm_CreatePlayer(sithPlayer_GetLocalPlayerName());
    if ( !sithMessage_localPlayerId )
    {
#line 375
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Create Local Player failed.\n", 0, 0, 0, 0);
    }
    else
    {
#line 379
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Joined game %d as player %d.\n", gameNum, sithMessage_localPlayerId, 0, 0);
        return 0;
    }

    stdComm_CloseGame();
    sithMessage_localPlayerId = 0;
    return E_FAIL;
}

void J3DAPI sithMessage_CloseGame(void)
{
    if ( sithMessage_localPlayerId )
    {
        stdComm_DestroyPlayer(sithMessage_localPlayerId);
    }

    stdComm_CloseGame();
    sithMessage_localPlayerId = 0;
}

// Not in retail .text: nothing references it, so LINK /OPT:REF discarded it with its unique strings.
// Its "pSettings" assert literal survives at 0x0052CC14 in this TU's string block because
// stdComm_GetSessionSettings references the same pooled string, so the assert stays the REL spelling.
// Body from the Indy debug build (0x00465D46; lines 432/441/449/456/460): start (or update)
// the hosted game and create the local player, closing the game again if that fails. The debug code reaches its close-game
// block from the failed CreatePlayer by a jump past `return 0`, written as `goto error;` onto that block after the
// success return.
int J3DAPI sithMessage_StartGame(StdCommGame* pSettings)
{
    HRESULT hres;

#line 432
    SITH_ASSERTREL(pSettings);
    if ( sithMessage_localPlayerId )
    {
        hres = stdComm_SetGameParams(pSettings);
        if ( hres )
        {
            stdComm_CloseGame();
#line 441
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not set session params for game %S.\n", pSettings->aSessionName, 0, 0, 0);
            return hres;
        }
    }
    else
    {
        hres = stdComm_CreateGame(pSettings);
        if ( hres )
        {
#line 449
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create new game %S.\n", pSettings->aSessionName, 0, 0, 0);
            return hres;
        }

        sithMessage_localPlayerId = stdComm_CreatePlayer(sithPlayer_GetLocalPlayerName());
        if ( !sithMessage_localPlayerId )
        {
#line 456
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Create Local Player failed.\n", 0, 0, 0, 0);
            goto error;
        }

#line 460
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Started game '%S' as player %d.\n", pSettings->aSessionName, sithMessage_localPlayerId, 0, 0);
    }

    return 0;

error:
    stdComm_CloseGame();
    sithMessage_localPlayerId = 0;
    return E_FAIL;
}

/* PLACEHOLDER, linker-discarded: retail __LINE__ gap 303..561 (sithMessage_ReceiveMessage -> sithMessage_CheckMembershipList), where the original
   defined its debug-build sub_465EA8 (a compiled-out close-game block, then return 1). Name is descriptive
   (inferred); only existence and position are evidenced: minimal body. */
int J3DAPI sithMessage_LeaveGame(void)
{
    return 1;
}

/* Linker-discarded, retail __LINE__ gap 303..561, where it was defined. Body from the Indy debug build
   (0x00465ECB; lines 546/549): drain the pending DirectPlay messages into the shared
   message buffer, logging each, then the count. */
void J3DAPI sithMessage_Flush(void)
{
    DPID senderId;
    int res;
    size_t numFlushed;
    size_t dataLen;

    dataLen    = STD_ARRAYLEN(sithMulti_message.data);
    numFlushed = 0;
    while ( 1 )
    {
        res = stdComm_Receive(&senderId, sithMulti_message.data, &dataLen);
        if ( res == -1 ) {
            break;
        }

        ++numFlushed;
#line 546
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Flushed message type %d from player %x.\n", sithMulti_message.type, senderId, 0, 0);
    }

#line 549
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Flushed %d pending messages from the queue.\n", numFlushed, 0, 0, 0);
}

void J3DAPI sithMessage_CheckMembershipList(DPID idTo)
{
    uint8_t* pCurDSSMsgData_ = &outmsg.data[0];
    size_t i;

    if ( stdComm_UpdatePlayers(0) )
    {
#line 561
        stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Unable to UpdatePlayers List.\n", 0, 0, 0, 0);
        return;
    }

    SITHDSS_PUSHUINT8(stdComm_GetNumPlayers());
    for ( i = 0; i < stdComm_GetNumPlayers(); ++i )
    {
        SITHDSS_PUSHUINT32(stdComm_GetPlayerID(i));
    }

    outmsg.type = SITHDSS_CHECKMEMBERSHIPLIST;
    outmsg.length = pCurDSSMsgData_ - outmsg.data;
    sithMessage_SendMessage(&outmsg, idTo, 1, DPSEND_GUARANTEED);
}

int J3DAPI sithMessage_ProcessCheckMembershipList(const SithMessage* pMsg)
{
    size_t numPlayers;
    size_t i;
    DPID idPlayer;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    if ( stdComm_IsGameHost() || (sith_mode.subModeFlags & 8) == 0 )
    {
#line 591
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Ignoring MembershipList check from %x.\n", sithMessage_senderId, 0, 0, 0);
        return 1;
    }

    if ( stdComm_UpdatePlayers(0) )
    {
#line 598
        stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Unable to UpdatePlayers List.\n", 0, 0, 0, 0);
        return 0;
    }

    numPlayers = SITHDSS_POPUINT8();
    for ( i = 0; i < numPlayers; ++i )
    {
        idPlayer = SITHDSS_POPUINT32();
        if ( !stdComm_VerifyPlayer(idPlayer) )
        {
            continue;
        }

        stdComm_RejoinSession(&sithMessage_localPlayerId, sithPlayer_GetLocalPlayerName());
#line 625
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "--- Session memebership problem detected --- rejoining session.\n", 0, 0, 0, 0);
        return 1;
    }

#line 631
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "We verified %d players in our session successfully.\n", numPlayers, 0, 0, 0);
    return 1;
}

void J3DAPI sithMessage_RegisterFunction(int type, SithMessageProcessFunc pFunc)
{
#line 654
    SITH_ASSERTREL((type >= 0) && (type < 65));
    sithMessage_aTypeFuncs[type] = pFunc;
}

int J3DAPI sithMessage_Process(const SithMessage* pMessage)
{
    int type;
    int result;

#line 683
    SITH_ASSERTREL(pMessage != NULL);

    type = pMessage->type;
    if ( type < 0 || type >= 65 )
    {
#line 689
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Message contained illegal type field %d.\n", type, 0, 0, 0);
        return 1;
    }

    if ( !sithMessage_aTypeFuncs[type] )
    {
#line 695
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: message type %d has no registered function to process.  Discarded\n", type, 0, 0, 0);
        return 1;
    }

    result = sithMessage_aTypeFuncs[type](pMessage);
    return result;
}

void J3DAPI sithMessage_InitMessageHandlers(void)
{
    sithMessage_RegisterFunction(SITHDSS_POS, sithDSSThing_ProcessPos);
    sithMessage_RegisterFunction(SITHDSS_FIRE, sithDSSThing_ProcessFire);
    sithMessage_RegisterFunction(SITHDSS_JOIN, sithMulti_ProcessJoinRequest);
    sithMessage_RegisterFunction(SITHDSS_WELCOME, sithMulti_ProcessWelcome);
    sithMessage_RegisterFunction(SITHDSS_DEATH, sithDSSThing_ProcessDeath);
    sithMessage_RegisterFunction(SITHDSS_DAMAGE, sithDSSThing_ProcessDamage);
    sithMessage_RegisterFunction(SITHDSS_COGMESSAGE, sithDSSCog_ProcessMessage);
    sithMessage_RegisterFunction(SITHDSS_THINGSTATE, sithDSSThing_ProcessStateUpdate);
    sithMessage_RegisterFunction(SITHDSS_PLAYSOUND, sithDSSThing_ProcessPlaySound);
    sithMessage_RegisterFunction(SITHDSS_PLAYKEY, sithDSSThing_ProcessPlayKey);
    sithMessage_RegisterFunction(SITHDSS_THINGFULLDESC, sithDSSThing_ProcessFullDescription);
    sithMessage_RegisterFunction(SITHDSS_COGSTATE, sithDSSCog_ProcessCogState);
    sithMessage_RegisterFunction(SITHDSS_SURFACESTATUS, sithDSS_ProcessSurfaceStatus);
    sithMessage_RegisterFunction(SITHDSS_AISTATUS, sithDSS_ProcessAIStatus);
    sithMessage_RegisterFunction(SITHDSS_INVENTORY, sithDSS_ProcessInventory);
    sithMessage_RegisterFunction(SITHDSS_ANIMSTATUS, sithDSS_ProcessAnimStatus);
    sithMessage_RegisterFunction(SITHDSS_SECTORSTATUS, sithDSS_ProcessSectorStatus);
    sithMessage_RegisterFunction(SITHDSS_SYNCSEENSECTORS, sithDSS_ProcessSyncSeenSecors);
    sithMessage_RegisterFunction(SITHDSS_PATHMOVE, sithDSSThing_ProcessPathMove);
    sithMessage_RegisterFunction(SITHDSS_PUPPETSTATUS, sithDSS_ProcessPuppetStatus);
    sithMessage_RegisterFunction(SITHDSS_SYNCPLAYERS, sithMulti_ProcessSyncPlayers);
    sithMessage_RegisterFunction(SITHDSS_ATTACHMENT, sithDSSThing_ProcessAttachment);
    sithMessage_RegisterFunction(SITHDSS_SYNCTASKEVENTS, sithDSS_ProcessSyncTaskEvents);
    sithMessage_RegisterFunction(SITHDSS_SYNCCAMERAS, sithDSS_ProcessSyncCameras);
    sithMessage_RegisterFunction(SITHDSS_TAKE, sithDSSThing_ProcessTake);
    sithMessage_RegisterFunction(SITHDSS_TAKE2, sithDSSThing_ProcessTake);
    sithMessage_RegisterFunction(SITHDSS_STOPKEY, sithDSSThing_ProcessStopKey);
    sithMessage_RegisterFunction(SITHDSS_STOPSOUND, sithDSSThing_ProcessStopSound);
    sithMessage_RegisterFunction(SITHDSS_CREATETHING, sithDSSThing_ProcessCreateThing);
    sithMessage_RegisterFunction(SITHDSS_SYNCGAMESTATE, sithDSS_ProcessSyncGameState);
    sithMessage_RegisterFunction(SITHDSS_CHAT, sithMulti_ProcessChat);
    sithMessage_RegisterFunction(SITHDSS_DESTROYTHING, sithDSSThing_ProcessDestroyThing);
    sithMessage_RegisterFunction(SITHDSS_SECTORFLAGS, sithDSS_ProcessSectorFlags);
    sithMessage_RegisterFunction(SITHDSS_PLAYSOUNDMODE, sithDSSThing_ProcessPlaySoundMode);
    sithMessage_RegisterFunction(SITHDSS_PLAYKEYMODE, sithDSSThing_ProcessPlayKeyMode);
    sithMessage_RegisterFunction(SITHDSS_SETMODEL, sithDSSThing_ProcessSetModel);
    sithMessage_RegisterFunction(SITHDSS_PING, sithMulti_ProcessPing);
    sithMessage_RegisterFunction(SITHDSS_PONG, sithMulti_ProcessPong);
    sithMessage_RegisterFunction(SITHDSS_CHECKMEMBERSHIPLIST, sithMessage_ProcessCheckMembershipList);
    sithMessage_RegisterFunction(SITHDSS_QUIT, sithMulti_ProcessQuit);
    sithMessage_RegisterFunction(SITHDSS_MOVEPOS, sithDSSThing_ProcessMovePos);
    sithMessage_RegisterFunction(SITHDSS_ENDSECTION, sithDSS_ProcessEndSection);
    sithMessage_RegisterFunction(SITHDSS_VOICESTATE, sithVoice_ProcessVoiceState);
    sithMessage_RegisterFunction(SITHDSS_VEHICLECONTROLSTATE, sithDSS_ProcessVehicleControlsState);
    sithMessage_RegisterFunction(SITHDSS_WHIPSTATUS, sithWhip_Restore);
    sithMessage_RegisterFunction(SITHDSS_SYNCWPNT, sithAIUtil_ProcessSyncWpnts);
}

int J3DAPI sithMessage_FileWrite(const SithMessage* pMessage)
{
    int result;

    result = stdConffile_Write(&pMessage->type, sizeof(pMessage->type));
    if ( result )
    {
        return 1;
    }

    result = stdConffile_Write(&pMessage->length, sizeof(pMessage->length));
    if ( result )
    {
        return 1;
    }

    result = stdConffile_Write(pMessage->data, pMessage->length);
    if ( result )
    {
        return 1;
    }
}
