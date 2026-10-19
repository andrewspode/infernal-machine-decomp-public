#include "sithCommand.h"


#ifdef J3D_DEBUG
#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIUtil.h>
#endif
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Main/sithMain.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Math/rdMatrix.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct sSithCommandMatInfo
{
    size_t num;
    size_t surfCount;
    size_t memPerSurf;
    size_t memUsage;
} SithCommandMatInfo;


static char aCipherTextBuf[256] = { 0 }; // Added: Init to 0

// The three enciphered cheat names are initialised arrays, not /Gf literals: retail keeps them at
// the head of this TU's .data (0x0053CA60/70/80), in declaration order and 8-byte aligned as VC5
// aligns a .data object of 8 bytes or more, ahead of the literal run that VC5 emits in reverse
// creation order. The names are inferred.
static char aDebugModeText[] = "VCINKV}OCPKML"; // taklit_marion
static char aWussText[]      = "WPEML}GNQC";    // urgon_elsa
static char aOuchText[]      = "CXGPKO}QMRJKC"; // azerim_sophia

int J3DAPI sithCommand_DebugMode(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Tick(const SithConsoleCommand* pFunc, const char* pStrTickRate);
int J3DAPI sithCommand_Session(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Wuss(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Fixme(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Ouch(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_MakeMePirate(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Coords(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Warp(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Jump(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Memory(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_DynamicMemory(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_MemoryDump(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Players(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Ping(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Kick(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Fly(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Imp1(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Imp2(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Imp3(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Imp4(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Imp5(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Electroman(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_CamInfo(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Fov(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_TweakJoint(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Weapons(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_CogTrace(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_CogList(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_CogPause(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_PauseAllCogs(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_MatList(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_Activate(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithCommand_StopSounds(const SithConsoleCommand* pFunc, const char* pArg);
const char* J3DAPI sithCommand_CipherText(const char* pText);

void J3DAPI sithCommand_RegisterCommands(void)
{
    // taklit_marion
    sithConsole_RegisterCommand(sithCommand_DebugMode, sithCommand_CipherText(aDebugModeText), 4);

    // urgon_elsa
    sithConsole_RegisterCommand(sithCommand_Wuss, sithCommand_CipherText(aWussText), 0);

    // azerim_sophia
    sithConsole_RegisterCommand(sithCommand_Ouch, sithCommand_CipherText(aOuchText), 0);

    sithConsole_RegisterCommand(sithCommand_Fixme, "fixme", 0);
#ifdef J3D_DEBUG
    sithConsole_RegisterCommand(sithCommand_DebugMode, "invul", 4);
    sithConsole_RegisterCommand(sithCommand_Wuss, "wuss", 0);
    sithConsole_RegisterCommand(sithCommand_Ouch, "ouch", 0);
    sithConsole_RegisterCommand(sithCommand_Warp, "warp", 0);
    sithConsole_RegisterCommand(sithCommand_Jump, "jump", 0);
#endif

    sithConsole_RegisterCommand(sithCommand_MakeMePirate, "makemeapirate", 0);
    sithConsole_RegisterCommand(sithCommand_Coords, "coords", 0);

    sithConsole_RegisterCommand(sithCommand_Memory, "mem", 0);
    sithConsole_RegisterCommand(sithCommand_DynamicMemory, "dynamicmem", 0);
    sithConsole_RegisterCommand(sithCommand_MemoryDump, "memdump", 0);

#ifdef J3D_DEBUG
    sithConsole_RegisterCommand(sithCommand_Imp1, "imp1", 0);
    sithConsole_RegisterCommand(sithCommand_Imp2, "imp2", 0);
    sithConsole_RegisterCommand(sithCommand_Imp3, "imp3", 0);
    sithConsole_RegisterCommand(sithCommand_Imp4, "imp4", 0);
    sithConsole_RegisterCommand(sithCommand_Imp5, "imp5", 0);
    sithConsole_RegisterCommand(sithCommand_Fly, "fly", 0);
    sithConsole_RegisterCommand(sithCommand_Players, "players", 0);
    sithConsole_RegisterCommand(sithCommand_Ping, "ping", 0);
    sithConsole_RegisterCommand(sithCommand_Kick, "kick", 0);
    sithConsole_RegisterCommand(sithCommand_Tick, "tick", 0);
    sithConsole_RegisterCommand(sithCommand_Session, "session", 0);
    sithConsole_RegisterCommand(sithCommand_Electroman, "electroman", 0);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "trackshots", 2);
    sithConsole_RegisterCommand(sithCommand_CamInfo, "caminfo", 0);
    sithConsole_RegisterCommand(sithCommand_Fov, "fov", 0);
    sithConsole_RegisterCommand(sithCommand_TweakJoint, "tj", 0);
    sithConsole_RegisterCommand(sithCommand_Weapons, "weapons", 0);
    sithConsole_RegisterCommand(sithConsole_Help, "help", 0);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "disableai", 0);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "notarget", 5);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "disablepuppet", 1);
    sithConsole_RegisterCommand(sithCommand_CogTrace, "cogtrace", 0);
    sithConsole_RegisterCommand(sithCommand_CogList, "coglist", 0);
    sithConsole_RegisterCommand(sithCog_CogStatus, "cogstatus", 0);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "noaishots", 3);
    sithConsole_RegisterCommand(sithAI_AIStatus, "aistatus", 0);
    sithConsole_RegisterCommand(sithAI_AIList, "ailist", 0);
    sithConsole_RegisterCommand(sithCommand_CogPause, "cogpause", 0);
    sithConsole_RegisterCommand(sithCommand_PauseAllCogs, "pauseallcogs", 0);
    sithConsole_RegisterCommand(sithCommand_MatList, "matlist", 0);
    sithConsole_RegisterCommand(sithCommand_Activate, "activate", 0);
    sithConsole_RegisterCommand(sithCommand_DebugMode, "slowmo", 6);
    sithConsole_RegisterCommand(sithCommand_StopSounds, "stopsounds", 0);
    sithConsole_RegisterCommand(sithAIUtil_WpDraw, "wpdraw", 0);
#endif
}

int J3DAPI sithCommand_DebugMode(const SithConsoleCommand* pFunc, const char* pArg)
{
    int* pCurFlags;
    int newFlags;
    wchar_t awComName[32];
    wchar_t awTxt[128];

#line 212
    SITH_ASSERTREL(pFunc);

    switch ( pFunc->flags )
    {
        case 0:
            pCurFlags = &sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_AIEVENTS_DISABLED;
            break;

        case 1:
            pCurFlags = &sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_PUPPETSYSTEM_DISABLED;
            break;

        case 2:
            pCurFlags = &sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_TRACKSHOTS;
            break;

        case 3:
            pCurFlags = &sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_AIDISABLED;
            break;

        case 4:  // invul
            if ( !sithWorld_pCurrentWorld || !sithWorld_pCurrentWorld->pLocalPlayer || sithWorld_pCurrentWorld->pLocalPlayer->type != SITH_THING_PLAYER )
            {
                return 0;
            }

            pCurFlags = (int*)&sithWorld_pCurrentWorld->pLocalPlayer->thingInfo.actorInfo.flags;
            newFlags = SITH_AF_INVULNERABLE;
            break;

        case 5:
            pCurFlags = &sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_AINOTARGET;
            break;

        case 6:
            pCurFlags = &sith_mode.debugModeFlags;
            newFlags = SITHDEBUG_SLOWMODE;
            break;

        default:
            return 0;
    }

    // Set/Clear flag
    if ( !pArg )
    {
    }
    else
    {
        if ( streqi(pArg, "on") || streqi(pArg, "1") )
        {
            *pCurFlags |= newFlags;
            if ( pFunc->flags == 4 ) // invul
            {
                sithPlayer_bPlayerInvulnerable = 1;
            }
        }
        else if ( streqi(pArg, "off") || streqi(pArg, "0") )
        {
            *pCurFlags &= ~newFlags;
            if ( pFunc->flags == 4 ) // invul
            {
                sithPlayer_bPlayerInvulnerable = 0;
            }
        }
    }

    // Print command state to console
    
    STD_TOWSTR(awComName, pFunc->aName);

    // Note, bug calling sithString_GetString will override the state string due to stdStrTable internal cache
    _snwprintf(awTxt, STD_ARRAYLEN(awTxt), sithString_GetString("%s_IS_%s"), awComName, ((*pCurFlags & newFlags) != 0) ? sithString_GetString("ON") : sithString_GetString("OFF"));
    sithConsole_PrintWString(awTxt);

    return 1;
}

int J3DAPI sithCommand_Wuss(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithCog* pCog;
    SithThing* pPlayerThing;


    pPlayerThing = sithPlayer_pLocalPlayerThing;
    if ( pPlayerThing )
    {
        pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(31)); //0x801F
        if ( pCog )
        {
            sithCog_SendMessage(pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pPlayerThing->idx, 0);
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithCommand_Fixme(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;


    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        pThing->moveStatus        = SITHPLAYERMOVE_STILL;
        pThing->collide.movesize  = 0.04f;

        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        pThing->thingInfo.actorInfo.bForceMovePlay    = 0;

        pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;  // 0x800 - SITH_PF_ALIGNUP
        pThing->moveInfo.physics.height = 0.09f;

        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_Ouch(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithCog* pCog;
    SithThing* pPlayerThing;


    pPlayerThing = sithPlayer_pLocalPlayerThing;
    if ( pPlayerThing )
    {
        pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(32)); // 0x8020
        if ( pCog )
        {
            sithCog_SendMessage(pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pPlayerThing->idx, 0);
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithCommand_MakeMePirate(const SithConsoleCommand* pFunc, const char* pArg)
{

    return sithPlayer_ToggleGuybrush();
}

int J3DAPI sithCommand_Coords(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pPlayer;
    rdVector3 pyr;

    if ( !sithWorld_pCurrentWorld || !sithWorld_pCurrentWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    pPlayer = sithWorld_pCurrentWorld->pLocalPlayer;
    if ( !pPlayer->pInSector )
    {
        sithConsole_PrintString("Thing is not in world");
        return 1;
    }

    rdMatrix_ExtractAngles34(&pPlayer->orient, &pyr);
    sprintf(std_genBuffer, "Pos: (%.2f, %.2f, %.2f) PYR: (%.2f, %.2f, %.2f) Sector: %d.", pPlayer->pos.x, pPlayer->pos.y, pPlayer->pos.z, pyr.pitch, pyr.yaw, pyr.roll, sithSector_GetSectorIndex(pPlayer->pInSector));
    sithConsole_PrintString(std_genBuffer);
    return 1;
}

#ifdef J3D_DEBUG
int J3DAPI sithCommand_Warp(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithWorld* pWorld;
    SithSector* pSector;
    rdVector3 pyr;
    size_t i;
    rdMatrix34 orient;
    int numMatched;
    rdVector3 pos;
    SithThing* pThing;

    pWorld = sithWorld_pCurrentWorld;
    if ( !pWorld || !pWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("Format: WARP x y z");
        return 0;
    }

    pThing  = pWorld->pLocalPlayer;
    numMatched = sscanf(pArg, "%f %f %f %f %f %f", &pos.x, &pos.y, &pos.z, &pyr.x, &pyr.y, &pyr.z);
    if ( numMatched < 3 )
    {
        return 0;
    }

    if ( numMatched == 6 )
    {
        rdMatrix_BuildRotate34(&orient, &pyr);
    }
    else
    {
        rdMatrix_Copy34(&orient, &rdroid_identMatrix34);
    }

    for ( i = 0, pSector = pWorld->aSectors; i < pWorld->numSectors; ++i, ++pSector )
    {
        if ( sithIntersect_IsSphereInSector(pWorld, &pos, 0.0f, pSector) )
        {
            break;
        }
    }

    if ( i == pWorld->numSectors )
    {
        sithConsole_PrintString("Position not in world");
        return 0;
    }

    sithThing_DetachThing(pThing);
    sithThing_ExitSector(pThing);
    sithThing_SetPositionAndOrient(pThing, &pos, &orient);
    sithThing_EnterSector(pThing, pSector, 1, 0);
    return 1;
}

int J3DAPI sithCommand_Jump(const SithConsoleCommand* pFunc, const char* pArg)
{
    int playerNum;

    if ( !pArg )
    {
        return 0;
    }

    if ( !sscanf(pArg, "%d", &playerNum) )
    {
        return 0;
    }

    sithPlayerActions_MoveToPlayerPosition(sithPlayer_pLocalPlayerThing, playerNum - 1);
    return 1;
}
#endif

int J3DAPI sithCommand_Memory(const SithConsoleCommand* pFunc, const char* pArg)
{
    const SithWorld* pWorld;
    size_t aMemUsed[17];
    size_t paCnt[17];
    size_t totalUsed;


    pWorld = sithWorld_pCurrentWorld;
    if ( pArg ) {
        pWorld = sithWorld_pStaticWorld;
    }

    if ( !pWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    sithWorld_GetMemoryUsage(pWorld, &aMemUsed, &paCnt);

    totalUsed = 0;

    sprintf(std_genBuffer, "%5d Materials        %8d bytes.", paCnt[0], aMemUsed[0]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[0];

    sprintf(std_genBuffer, "%5d Models           %8d bytes.", paCnt[10], aMemUsed[10]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[10];

    sprintf(std_genBuffer, "%5d Keyframes        %8d bytes.", paCnt[11], aMemUsed[11]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[11];

    sprintf(std_genBuffer, "%5d World Vertices   %8d bytes.", paCnt[1], aMemUsed[1]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[1];

    sprintf(std_genBuffer, "%5d World TexVerts   %8d bytes.", paCnt[2], aMemUsed[2]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[2];

    sprintf(std_genBuffer, "%5d Surfaces         %8d bytes.", paCnt[3], aMemUsed[3]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[3];

    sprintf(std_genBuffer, "%5d Sectors          %8d bytes.", paCnt[5], aMemUsed[5]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[5];

    sprintf(std_genBuffer, "%5d Cog Scripts\t\t%8d bytes.", paCnt[8], aMemUsed[8]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[8];

    sprintf(std_genBuffer, "%5d Cogs             %8d bytes.", paCnt[7], aMemUsed[7]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[7];

    sprintf(std_genBuffer, "%5d Adjoins          %8d bytes.", paCnt[4], aMemUsed[4]);
    sithConsole_PrintString(std_genBuffer);
    totalUsed += aMemUsed[4];

    sprintf(std_genBuffer, "Total Memory Used:   %8d bytes.", totalUsed);
    sithConsole_PrintString(std_genBuffer);
    return 1;
}

int J3DAPI sithCommand_DynamicMemory(const SithConsoleCommand* pFunc, const char* pArg)
{

    sprintf(std_genBuffer, "NumAllocs: %d TotalMemAlloc: %d bytes MaxMemAlloc: %d bytes", stdMemory_curState.totalAllocs, stdMemory_curState.totalBytes, stdMemory_curState.maxBytes);
    sithConsole_PrintString(std_genBuffer);
    return 1;
}

int J3DAPI sithCommand_MemoryDump(const SithConsoleCommand* pFunc, const char* pArg)
{
    tFileHandle fh;
    tMemoryHeader* pCurMemHeader;


    fh = sith_pHS->pFileOpen("memdump.txt", "w+");
    if ( !fh ) {
        return 0;
    }

    sith_pHS->pFilePrintf(
        fh,
        "Total Memory allocated: %d bytes   # Allocations: %d  Max Memory used: %d bytes\n\n",
        stdMemory_curState.totalBytes,
        stdMemory_curState.totalAllocs,
        stdMemory_curState.maxBytes
    );

    pCurMemHeader = stdMemory_curState.header.pNext;
    while ( pCurMemHeader )
    {
        sith_pHS->pFilePrintf(fh, "%25s:  line %3d   %8d bytes   #%d\n", pCurMemHeader->pFilename, pCurMemHeader->line, pCurMemHeader->size, pCurMemHeader->number);
        pCurMemHeader = pCurMemHeader->pNext;
    }

    sith_pHS->pFileClose(fh);
    sithConsole_PrintString("Memory dump file 'memdump.txt' written.");
    return 1;
}

#ifdef J3D_DEBUG
/* Debug-build console commands: only the debug build's RegisterCommands registers them, and retail has none of them.
   Bodies and position from the Indy debug build (0x004E6BEA "players" .. 0x004E7C66 "stopsounds"; "tick" 0x004E6E4B and
   "session" 0x004E6F06 (line 717) follow "kick"). Jedi Knight's sithCommand has tick and session too. */
int J3DAPI sithCommand_Players(const SithConsoleCommand* pFunc, const char* pArg)
{
    size_t i;
    char aName[64];

    sprintf(std_genBuffer, "Maxplayers = %d", sithPlayer_numPlayers);
    sithConsole_PrintString(std_genBuffer);
    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        STD_TOSTR(aName, sithPlayer_aPlayers[i].awName);
        sprintf(std_genBuffer, "Player %d:  Name: %s  Flags: %x  ID: %x", i, aName, sithPlayer_aPlayers[i].flags, sithPlayer_aPlayers[i].playerNetId);
        sithConsole_PrintString(std_genBuffer);
    }

    return 1;
}

int J3DAPI sithCommand_Ping(const SithConsoleCommand* pFunc, const char* pArg)
{
    int playerNum;
    wchar_t awName[64];

    playerNum = 0;
    if ( !stdComm_IsGameActive() )
    {
        return 0;
    }

    if ( pArg )
    {
        STD_TOWSTR(awName, pArg);
        playerNum = sithPlayer_GetPlayerNumByName(awName);
        if ( playerNum < 0 )
        {
            sprintf(std_genBuffer, "Player %s not found", awName);
            sithConsole_PrintString(std_genBuffer);
        }
    }

    sithMulti_Ping(sithPlayer_aPlayers[playerNum].playerNetId);
    return 1;
}

int J3DAPI sithCommand_Kick(const SithConsoleCommand* pFunc, const char* pArg)
{
    size_t i;
    wchar_t awName[64];

    if ( !pArg || !stdComm_IsGameActive() || !stdComm_IsGameHost() )
    {
        return 0;
    }

    STD_TOWSTR(awName, pArg);
    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) == 0 )
        {
            continue;
        }

        if ( !wstrcmpi(sithPlayer_aPlayers[i].awName, awName) )
        {
            sprintf(std_genBuffer, "Kicked %S", sithPlayer_aPlayers[i].awName);
            sithConsole_PrintString(std_genBuffer);
            sithMulti_QuitPlayer(sithPlayer_aPlayers[i].playerNetId);
        }
    }

    return 1;
}

int J3DAPI sithCommand_Tick(const SithConsoleCommand* pFunc, const char* pStrTickRate)
{
    size_t tickRate;

    if ( !stdComm_IsGameActive() )
    {
        return 0;
    }

    if ( !pStrTickRate || (tickRate = atoi(pStrTickRate)) == 0 )
    {
        sprintf(std_genBuffer, "Current tick rate is %d msec", sithMulti_GetTickRate());
        sithConsole_PrintString(std_genBuffer);
        return 1;
    }

    if ( !J3DINRANGE(tickRate, 50, 300) )
    {
        sithConsole_PrintString("New tick is out of range");
        return 0;
    }

    sithMulti_SetTickRate(tickRate);
    sithEvent_RegisterTask(2u, sithMulti_CheckPlayers, tickRate, SITHEVENT_TASKINTERVAL);
    return 1;
}

int J3DAPI sithCommand_Session(const SithConsoleCommand* pFunc, const char* pArg)
{
    DPID playerId;
    size_t i;
    size_t numPlayers;
    size_t playerNum;

    if ( !stdComm_IsGameActive() )
    {
        return 0;
    }

    if ( stdComm_UpdatePlayers(0) )
    {
#line 717
        stdPrintf(sith_pHS->pWarningPrint, J3D_FILE, __LINE__, "Unable to UpdatePlayers List.\n", 0, 0, 0, 0);
        return 0;
    }

    numPlayers = stdComm_GetNumPlayers();
    sprintf(std_genBuffer, "%d players in session", numPlayers);
    sithConsole_PrintString(std_genBuffer);
    for ( i = 0; i < numPlayers; ++i )
    {
        playerId = stdComm_GetPlayerID(i);
        playerNum = sithMulti_GetPlayerIndexByID(playerId);
        if ( playerNum >= 0 )
        {
            sprintf(std_genBuffer, "Player %x (%S) is in the session", playerId, sithPlayer_aPlayers[playerNum].awName);
        }
        else
        {
            sprintf(std_genBuffer, "Unknown player %x is in the session", playerId);
        }

        sithConsole_PrintString(std_genBuffer);
    }

    return 1;
}

int J3DAPI sithCommand_Fly(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    if ( !sithWorld_pCurrentWorld || !sithWorld_pCurrentWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    pThing = sithWorld_pCurrentWorld->pLocalPlayer;
    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        sithConsole_PrintString("Not physics thing.");
        return 0;
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
    {
        pThing->moveInfo.physics.flags &= ~SITH_PF_FLY;
        pThing->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
        sithConsole_PrintWString(sithString_GetString("FLYING_OFF"));
    }
    else
    {
        pThing->moveInfo.physics.flags |= SITH_PF_FLY;
        pThing->moveInfo.physics.flags &= ~SITH_PF_USEGRAVITY;
        sithConsole_PrintWString(sithString_GetString("FLYING_ON"));
    }

    return 1;
}

int J3DAPI sithCommand_Imp1(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        sithInventory_SetInventory(pThing, 14, 1.0f);
        sithInventory_SetInventoryAvailable(pThing, 14, 1);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_Imp2(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        sithInventory_SetInventory(pThing, 15, 1.0f);
        sithInventory_SetInventoryAvailable(pThing, 15, 1);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_Imp3(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        sithInventory_SetInventory(pThing, 16, 1.0f);
        sithInventory_SetInventoryAvailable(pThing, 16, 1);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_Imp4(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        sithInventory_SetInventory(pThing, 17, 1.0f);
        sithInventory_SetInventoryAvailable(pThing, 17, 1);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_Imp5(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        sithInventory_SetInventory(pThing, 18, 1.0f);
        sithInventory_SetInventoryAvailable(pThing, 18, 1);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_Electroman(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;
    rdMaterial* pMaterial;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        pThing->thingInfo.actorInfo.flags |= SITH_AF_ELECTRICWHIP;
        sithInventory_SetInventory(pThing, 2, 25.0f);
        pMaterial = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(0x146));
        if ( pMaterial )
        {
            pMaterial->curCelNum = 1;
        }

        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_CamInfo(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithCamera* pCamera;

    if ( !sithCamera_pCurCamera )
    {
        return 0;
    }

    pCamera = sithCamera_pCurCamera;
    sprintf(std_genBuffer, "POS: X %2.4f Y %2.4f Z %2.4f FOV %2.2f", pCamera->orient.dvec.x, pCamera->orient.dvec.y, pCamera->orient.dvec.z, pCamera->fov);
    sithConsole_PrintString(std_genBuffer);
    sprintf(std_genBuffer, "ORIENT: P %2.4f Y %2.4f R %2.4f", pCamera->lookPYR.pitch, pCamera->lookPYR.yaw, pCamera->lookPYR.roll);
    sithConsole_PrintString(std_genBuffer);
}

int J3DAPI sithCommand_Fov(const SithConsoleCommand* pFunc, const char* pArg)
{
    float fov;
    int numMatched;

    if ( !pArg )
    {
        return 0;
    }

    numMatched = sscanf(pArg, "%f", &fov);
    if ( numMatched == 1 )
    {
        sithCamera_SetCameraFOV(sithCamera_pCurCamera, fov);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_TweakJoint(const SithConsoleCommand* pFunc, const char* pArg)
{
    int jointNum;
    int axis;
    float degrees;
    int numConverted;
    SithThing* pThing;

    if ( !pArg )
    {
        sithConsole_PrintString("Format: tj JOINTNUM AXIS DEGREES");
        return 0;
    }

    numConverted = sscanf(pArg, "%d %d %f", &jointNum, &axis, &degrees);
    if ( numConverted < 3 )
    {
        sithConsole_PrintString("Format: tj JOINTNUM AXIS DEGREES");
        return 0;
    }

    pThing = sithPlayer_pLocalPlayerThing;
    if ( !pThing )
    {
        return 0;
    }

    ((float*)&pThing->renderData.apTweakedAngles[jointNum])[axis] = degrees;
    return 1;
}

int J3DAPI sithCommand_Weapons(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithThing* pThing;

    pThing = sithPlayer_pLocalPlayerThing;
    if ( pThing )
    {
        sithInventory_SetSwimmingInventory(pThing, 1);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_CogTrace(const SithConsoleCommand* pFunc, const char* pArg)
{
    size_t cogNum;
    SithCog* pCog;

    if ( !sithWorld_pCurrentWorld )
    {
        sithConsole_PrintString("no world open.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("syntax error.");
        return 0;
    }

    cogNum = atoi(pArg);
    if ( cogNum >= sithWorld_pCurrentWorld->numCogs )
    {
        sithConsole_PrintString("cog id out of range.");
        return 0;
    }

    pCog = &sithWorld_pCurrentWorld->aCogs[cogNum];
    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sithConsole_PrintString("Cog trace disabled.");
        pCog->flags &= ~SITHCOG_DEBUG;
    }
    else
    {
        sithConsole_PrintString("Cog trace enabled.");
        pCog->flags |= SITHCOG_DEBUG;
    }

    return 1;
}

int J3DAPI sithCommand_CogList(const SithConsoleCommand* pFunc, const char* pArg)
{
    static char aLineEnd[] = "";
    size_t i;
    SithCog* pCog;

    if ( !sithWorld_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    sprintf(std_genBuffer, "World cogs = %d.", sithWorld_pCurrentWorld->numCogs);
    sithConsole_PrintString(std_genBuffer);
    for ( i = 0, pCog = sithWorld_pCurrentWorld->aCogs; i < sithWorld_pCurrentWorld->numCogs; ++i, ++pCog )
    {
        sprintf(std_genBuffer, "%d: %-16s %-16s ", i, pCog->aName, pCog->pScript->aName);
        if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
        {
            sprintf(std_genBuffer + strlen(std_genBuffer), "(paused) ");
        }

        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer + strlen(std_genBuffer), "(trace)  ");
        }

        sprintf(std_genBuffer + strlen(std_genBuffer), aLineEnd);
        sithConsole_PrintString(std_genBuffer);
    }

    return 1;
}

int J3DAPI sithCommand_CogPause(const SithConsoleCommand* pFunc, const char* pArg)
{
    size_t cogNum;
    SithCog* pCog;

    if ( !sithWorld_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("syntax error.");
        return 0;
    }

    cogNum = atoi(pArg);
    if ( cogNum >= sithWorld_pCurrentWorld->numCogs )
    {
        sithConsole_PrintString("cog id out of range.");
        return 0;
    }

    pCog = &sithWorld_pCurrentWorld->aCogs[cogNum];
    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        sithConsole_PrintString("Cog enabled.");
        pCog->flags &= ~SITHCOG_DISABLED;
    }
    else
    {
        sithConsole_PrintString("Cog disabled.");
        pCog->flags |= SITHCOG_DISABLED;
    }

    return 1;
}

int J3DAPI sithCommand_PauseAllCogs(const SithConsoleCommand* pFunc, const char* pArg)
{
    size_t i;

    if ( !sithWorld_pCurrentWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

    if ( !pArg )
    {
        sithConsole_PrintString("On or off?");
        return 0;
    }

    if ( !_strcmpi(pArg, "on") || !_strcmpi(pArg, "1") )
    {
        sithConsole_PrintString("Cogs disabled.");
        for ( i = 0; i < sithWorld_pCurrentWorld->numCogs; ++i )
        {
            sithWorld_pCurrentWorld->aCogs[i].flags |= SITHCOG_DISABLED;
        }
    }
    else if ( !_strcmpi(pArg, "off") || !_strcmpi(pArg, "0") )
    {
        sithConsole_PrintString("Cogs enabled.");
        for ( i = 0; i < sithWorld_pCurrentWorld->numCogs; ++i )
        {
            sithWorld_pCurrentWorld->aCogs[i].flags &= ~SITHCOG_DISABLED;
        }
    }

    return 1;
}

static int J3DAPI sithCommand_CompareMatInfo(const void* pElem1, const void* pElem2);

int J3DAPI sithCommand_MatList(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithWorld* pWorld;
    SithSurface* pSurface;
    int i;
    int numMaterials;
    SithCommandMatInfo* aMatInfo;
    int matNum;

    pWorld = sithWorld_pCurrentWorld;
    if ( !pWorld )
    {
        sithConsole_PrintString("No world.");
        return 0;
    }

#line 1135
    aMatInfo = (SithCommandMatInfo*)STDMALLOC(sizeof(SithCommandMatInfo) * pWorld->numMaterials);
    STD_ASSERT(aMatInfo);

    for ( i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        aMatInfo[i].num       = i;
        aMatInfo[i].surfCount = 0;
    }

    for ( i = 0, pSurface = pWorld->aSurfaces; i < (int)pWorld->numSurfaces; ++i, ++pSurface )
    {
        if ( !pSurface->face.pMaterial )
        {
            continue;
        }

        matNum = pSurface->face.pMaterial - pWorld->aMaterials;
        ++aMatInfo[matNum].surfCount;
    }

    for ( i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        if ( aMatInfo[i].surfCount > 0 )
        {
            aMatInfo[i].memUsage   = rdMaterial_GetMaterialMemUsage(&pWorld->aMaterials[aMatInfo[i].num]);
            aMatInfo[i].memPerSurf = aMatInfo[i].memUsage / aMatInfo[i].surfCount;
        }
        else
        {
            aMatInfo[i].memPerSurf = 0;
            aMatInfo[i].memUsage   = 0;
        }
    }

    qsort(aMatInfo, pWorld->numMaterials, sizeof(SithCommandMatInfo), sithCommand_CompareMatInfo);

    numMaterials = pWorld->numMaterials;
    for ( i = numMaterials - 1; i >= 0; --i )
    {
        if ( aMatInfo[i].memPerSurf )
        {
            sprintf(std_genBuffer, "%-16s: %d faces, %d bytes, %d bytes/face.", pWorld->aMaterials[aMatInfo[i].num].aName, aMatInfo[i].surfCount, aMatInfo[i].memUsage, aMatInfo[i].memPerSurf);
            sithConsole_PrintString(std_genBuffer);
        }
    }

    return 1;
}

int J3DAPI sithCommand_Activate(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithWorld* pWorld;
    SithInventoryType* pType;
    int bin;
    int numMatched;
    SithThing* pThing;

    pWorld = sithWorld_pCurrentWorld;
    if ( !pWorld || !pWorld->pLocalPlayer )
    {
        sithConsole_PrintString("No world");
        return 0;
    }

    pThing  = pWorld->pLocalPlayer;
    numMatched = sscanf(pArg, "%d", &bin);
    if ( numMatched < 1 )
    {
        return 0;
    }

    if ( bin < 0 )
    {
        return 0;
    }

    pType = &sithInventory_aTypes[bin];
    if ( pType->pCog && sithInventory_IsInventoryAvailable(pThing, bin) )
    {
        sithCog_SendMessage(sithInventory_aTypes[bin].pCog, SITHCOG_MSG_ACTIVATE, SITHCOG_SYM_REF_NONE, bin, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        return 1;
    }

    return 0;
}

int J3DAPI sithCommand_StopSounds(const SithConsoleCommand* pFunc, const char* pArg)
{
    if ( !sithWorld_pCurrentWorld )
    {
        sithConsole_PrintString("No world open.");
        return 0;
    }

    sithSoundMixer_StopAll();
    return 1;
}

static int J3DAPI sithCommand_CompareMatInfo(const void* pElem1, const void* pElem2)
{
    return ((const SithCommandMatInfo*)pElem2)->memPerSurf - ((const SithCommandMatInfo*)pElem1)->memPerSurf;
}
#endif

const char* J3DAPI sithCommand_CipherText(const char* pText)
{
    // Function goes over all characters of text and xors each char with the
    // cipher key '"' (34 / 0x22). Under /Gf the pooled literal is writable, so
    // retail hoists a loop-invariant byte load of it out of the loop
    // (mov cl, byte ptr [0x00528674] at 0x004B4DBA - the image's only reference
    // to that datum) instead of folding an immediate.
    // def decrypt_cheat_text(enc_cheat):
    //      cheat = ""
    //      for c in enc_cheat:
    //          cheat += chr(ord(c) ^ 34)
    //      return cheat

    int len = strlen(pText);
    int i;
    for ( i = 0; i < len; ++i )
    {
        aCipherTextBuf[i] = pText[i] ^ "\""[0];
    }

    aCipherTextBuf[i] = 0;

    return aCipherTextBuf;
}

