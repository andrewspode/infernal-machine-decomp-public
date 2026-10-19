#include "sithPlayer.h"

#include <rdroid/Math/rdVector.h>
#include <rdroid/Math/rdMatrix.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <string.h>

static wchar_t sithPlayer_awLocalPlayerName[64];

size_t sithPlayer_playerNum = 0;
size_t sithPlayer_numPlayers = 0;
SithThing* sithPlayer_pLocalPlayerThing = NULL;
SithPlayer* sithPlayer_pLocalPlayer = NULL;
int sithPlayer_bPlayerInvulnerable = 0;
int sithPlayer_bInAetheriumSector = 0;
int sithPlayer_bGuybrush = 0;
unsigned int sithPlayer_curLevelNum = 0;
int sithPlayer_bBonusMapBought = 0;

int sithPlayer_bPlayerInPor = 1;
int sithPlayer_impFireType = -1;

SithPlayer sithPlayer_aPlayers[SITHPLAYER_MAX_PLAYERS];
float sithPlayer_impState;

void J3DAPI sithPlayer_Open(const wchar_t* awName)
{
    wcsncpy(sithPlayer_awLocalPlayerName, awName, STD_ARRAYLEN(sithPlayer_awLocalPlayerName) - 1);
    sithPlayer_awLocalPlayerName[STD_ARRAYLEN(sithPlayer_awLocalPlayerName) - 1] = 0;
    sithPlayer_impState = 0.0f;
}

void J3DAPI sithPlayer_Close(void)
{
    sithPlayer_pLocalPlayerThing = NULL;
    sithPlayer_pLocalPlayer        = NULL;
}

const wchar_t* J3DAPI sithPlayer_GetLocalPlayerName(void)
{
    return sithPlayer_awLocalPlayerName;
}

/* PLACEHOLDER, linker-discarded: the pooled /Gf cipher-key literal "\"" (0x00528674) is laid out
   first in this object's string run, ahead of PlacePlayers' literals, so a function defined before
   PlacePlayers used it (retail __LINE__ head excess +16 lines). Name inferred from the key's only
   surviving use (sithCommand_CipherText); body cut to that use. */
char J3DAPI sithPlayer_CipherChar(char ch)
{
    return ch ^ "\""[0];
}

void J3DAPI sithPlayer_PlacePlayers(SithWorld* pWorld)
{
    SithPlayer* pPlayer = sithPlayer_aPlayers;
    size_t numPlayers = 0;
    int thNum;
    SithThing* pThing;

#line 148
    SITH_ASSERTREL(pWorld != NULL);

    for ( thNum = 0, pThing = pWorld->aThings; thNum <= pWorld->lastThingIdx; ++thNum, ++pThing )
    {
        if ( pThing->type == SITH_THING_PLAYER )
        {
            if ( numPlayers >= STD_ARRAYLEN(sithPlayer_aPlayers) )
            {
#line 157
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many player objects placed.\n", 0, 0, 0, 0);
                continue;
            }

            pPlayer->pThing = pThing;
            pThing->flags |= SITH_TF_REMOTE;
            pThing->thingInfo.actorInfo.pPlayer = pPlayer;
            pPlayer->flags |= SITH_PLAYER_PLACED;

            rdMatrix_Copy34(&pPlayer->orient, &pThing->orient);
            pPlayer->orient.dvec.x = pThing->pos.x;
            pPlayer->orient.dvec.y = pThing->pos.y;
            pPlayer->orient.dvec.z = pThing->pos.z;
            pPlayer->pInSector = pThing->pInSector;

            ++pPlayer;
            ++numPlayers;
        }
    }

    sithPlayer_numPlayers = numPlayers;

    for ( thNum = numPlayers; thNum < SITHPLAYER_MAX_PLAYERS; ++thNum )
    {
        sithPlayer_aPlayers[thNum].pThing    = NULL;
        sithPlayer_aPlayers[thNum].pInSector = NULL;
    }
}

void J3DAPI sithPlayer_HidePlayer(size_t playerNum)
{
    SithPlayer* pPlayer = &sithPlayer_aPlayers[playerNum];
#line 216
    SITH_ASSERTREL(playerNum < SITHPLAYER_MAX_PLAYERS);

    pPlayer->flags &= ~SITH_PLAYER_JOINEDGAME;
    pPlayer->playerNetId = 0;

    if ( pPlayer->pThing && sithWorld_pCurrentWorld )
    {
        sithThing_SetThingModel(pPlayer->pThing, pPlayer->pThing->pTemplate->renderData.data.pModel3);
        pPlayer->pThing->flags |= SITH_TF_DISABLED;
    }
}

int J3DAPI sithPlayer_ShowPlayer(size_t playerNum, DPID id)
{
    SithPlayer* pPlayer;
    SithThing* pThing;

#line 254
    SITH_ASSERTREL(playerNum < sithPlayer_numPlayers);

    pPlayer = &sithPlayer_aPlayers[playerNum];
    pThing = pPlayer->pThing;
    if ( pThing )
    {
        pPlayer->flags |= SITH_PLAYER_UNKNOWN_04 | SITH_PLAYER_JOINEDGAME;
        pPlayer->playerNetId = id;
        pThing->flags &= ~SITH_TF_DISABLED;
        return 1;
    }

    return 0;
}

void J3DAPI sithPlayer_SetLocalPlayer(size_t playerNum)
{
    SithPlayer* pPlayer;
    size_t i;

#line 280
    SITH_ASSERTREL(playerNum < sithPlayer_numPlayers);
    SITH_ASSERTREL(sithWorld_pCurrentWorld);

    pPlayer = &sithPlayer_aPlayers[playerNum];
#line 285
    SITH_ASSERTREL(pPlayer->pThing);

    sithWorld_pCurrentWorld->pLocalPlayer = pPlayer->pThing;
    sithWorld_pCurrentWorld->pCameraFocusThing = sithWorld_pCurrentWorld->pLocalPlayer;
    sithPlayer_pLocalPlayer = pPlayer;

    sithPlayer_pLocalPlayerThing = pPlayer->pThing;
    sithPlayer_pLocalPlayerThing->flags &= ~SITH_TF_REMOTE;

    sithPlayer_playerNum = playerNum;

    wcsncpy(sithPlayer_pLocalPlayer->awName, sithPlayer_awLocalPlayerName, STD_ARRAYLEN(sithPlayer_pLocalPlayer->awName) - 1);
    sithPlayer_pLocalPlayer->awName[STD_ARRAYLEN(sithPlayer_pLocalPlayer->awName) - 1] = 0;

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( sithPlayer_aPlayers[i].pThing && i != playerNum )
        {
            sithPlayer_aPlayers[i].pThing->flags |= SITH_TF_REMOTE;
        }
    }
}

void J3DAPI sithPlayer_Reset(size_t playerNum)
{
    SithPlayer* pPlayer = &sithPlayer_aPlayers[playerNum];
    if ( playerNum >= SITHPLAYER_MAX_PLAYERS )
    {
#line 343
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid playerNum %d in sithPlayer_Reset.\n", playerNum, 0, 0, 0);
        return;
    }

    pPlayer->respawnMask = 0;
    pPlayer->playerNetId = 0;
    pPlayer->awName[0]   = 0;

    if ( pPlayer->pThing && sithWorld_pCurrentWorld )
    {
        sithInventory_InitInventory(pPlayer->pThing);
    }

    pPlayer->flags &= ~(SITH_PLAYER_UNKNOWN_04 | SITH_PLAYER_JOINEDGAME);
}

void J3DAPI sithPlayer_Update(SithPlayer* pPlayer, float secDeltaTime)
{
    SithThing* pThing;
    int bImpStateUpdated;

    bImpStateUpdated = 0;
    if ( pPlayer != sithPlayer_pLocalPlayer )
    {
        return;
    }

#line 387
    SITH_ASSERTREL(pPlayer);

    pThing = pPlayer->pThing;
#line 389
    SITH_ASSERTREL(pThing);

    sithWeapon_UpdateActorWeaponState(pThing, secDeltaTime);

    // Did thing fall into falldeth sector?
    if ( !pThing->attach.flags
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_FALLKILLED) == 0
        && pThing->moveType == SITH_MT_PHYSICS
        && pThing->pInSector
        && (pThing->pInSector->flags & SITH_SECTOR_FALLDEATH) != 0
        && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0
        && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        pThing->thingInfo.actorInfo.flags |= SITH_AF_FALLKILLED;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_FALLKILLED) != 0 && (pThing->flags & SITH_TF_DYING) == 0 )
    {
        // Player fell into pit, kill him
        pThing->flags |= SITH_TF_DYING;
        sithPlayer_KillPlayer(pThing);
    }

    // Update IMP fire state
    if ( sithPlayer_impFireType != -1 )
    {
        switch ( sithPlayer_impFireType )
        {
            case SITHWEAPON_IMP1:
            case SITHWEAPON_IMP4:
                sithPlayer_impState = (sithGetIMPDamageScalar() * 160.0f) + sithPlayer_impState;
                bImpStateUpdated = 1;
                sithPlayer_impFireType = -1;
                if ( sithPlayer_impState > 180.0f )
                {
                    float excess;

                    excess = sithPlayer_impState - 180.0f;
                    sithPlayer_impState = 180.0f;
                    sithThing_DamageThing(pThing, pThing, (excess * 2.0f) * sithGetIMPDamageScalar(), SITH_DAMAGE_IMP_BLAST);
                }
                break;

            case SITHWEAPON_IMP2:
            case SITHWEAPON_IMP3:
            case SITHWEAPON_IMP5:
                sithPlayer_impState = ((sithGetIMPDamageScalar() * 10.0f) * secDeltaTime) + sithPlayer_impState;
                bImpStateUpdated = 1;
                if ( sithPlayer_impState > 180.0f )
                {
                    sithPlayer_impState = 180.0f;
                    sithThing_DamageThing(pThing, pThing, (secDeltaTime * 40.0f) * sithGetIMPDamageScalar(), SITH_DAMAGE_IMP_BLAST);
                }
                break;
        }
    }

    // Update state in Aetherium or POR
    if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        sithPlayer_impState = (sithGetIMPDamageScalar() * secDeltaTime) + sithPlayer_impState;
        if ( sithPlayer_impState >= 180.0f )
        {
            float damage;

            sithPlayer_impState = 180.0f;
            damage = (secDeltaTime * 180.0f) * sithGetIMPDamageScalar();
            sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_IMP_BLAST);
        }
    }
    else if ( sithPlayer_impState != 0.0f
        && (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
        && sithPlayer_bPlayerInPor
        && !bImpStateUpdated )
    {
        // Player is safe In POR, increase aetherium stamina
        sithPlayer_impState = sithPlayer_impState - secDeltaTime * 8.0f;
        if ( sithPlayer_impState < 0.0f )
        {
            sithPlayer_impState = 0.0f;
        }
    }
}

void J3DAPI sithPlayer_NewPlayer(SithThing* pPlayer)
{
    int result = 0;  // unreferenced; the debug build's frame keeps its slot
    size_t spawnPlayerNum;

#line 493
    SITH_ASSERTREL(pPlayer);
    SITH_ASSERTREL((pPlayer->type == SITH_THING_PLAYER) || (pPlayer->type == SITH_THING_CORPSE));
    SITH_ASSERTREL(pPlayer->thingInfo.actorInfo.pPlayer);

    if ( pPlayer->renderData.pPuppet && pPlayer->pPuppetState )
    {
        sithPuppet_RemoveAllTracks(pPlayer);
    }

    if ( stdComm_IsGameActive() && (pPlayer->flags & SITH_TF_REMOTE) != 0 )
    {
        return;
    }

#line 506
    SITH_ASSERTREL(pPlayer->pTemplate);
    pPlayer->thingInfo.actorInfo.health = pPlayer->pTemplate->thingInfo.actorInfo.health;

    if ( (pPlayer->pTemplate->moveInfo.physics.flags & SITH_PF_ALIGNUP) != 0 )
    {
        pPlayer->moveInfo.physics.flags|= SITH_PF_ALIGNUP;
        pPlayer->moveInfo.physics.flags &= ~(SITH_PF_ALIGNED | SITH_PF_ALIGNSURFACE);
    }

    sithActor_SetHeadPYR(pPlayer, &rdroid_zeroVector3);

    if ( pPlayer == sithPlayer_pLocalPlayerThing )
    {
        sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_INTCAMERANUM], pPlayer, NULL);
        sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], pPlayer, NULL);
        sithCamera_RestoreExtCamera();
        sithCamera_SetCurrentToCycleCamera();
    }

    pPlayer->flags &= ~(SITH_TF_DYING | SITH_TF_DESTROYED);
    pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_FALLKILLED;
    pPlayer->msecLifeLeft = 0;

    if ( stdComm_IsGameActive() && pPlayer == sithPlayer_pLocalPlayerThing )
    {
        spawnPlayerNum = sithMulti_Respawn(pPlayer);
        sithThing_ExitSector(pPlayer);
        sithThing_SetPositionAndOrient(pPlayer, &sithPlayer_aPlayers[spawnPlayerNum].orient.dvec, &sithPlayer_aPlayers[spawnPlayerNum].orient);
        sithThing_EnterSector(pPlayer, sithPlayer_aPlayers[spawnPlayerNum].pInSector, 1, 0);

        sithCamera_Update(sithCamera_pCurCamera);

        pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_CONTROLSDISABLED;

        sithPhysics_ResetThingMovement(pPlayer);
        sithWeapon_DeactivateCurrentWeapon(pPlayer);
        sithCog_BroadcastMessage(SITHCOG_MSG_NEWPLAYER, SITHCOG_SYM_REF_THING, pPlayer->idx, SITHCOG_SYM_REF_THING, pPlayer->idx);
        if ( sithMessage_outputstream )
        {
            sithDSSThing_UpdateState(pPlayer, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

/* Linker-discarded: retail __LINE__ gap 506..658 (sithPlayer_NewPlayer -> sithPlayer_PlayerKilledAction), where the original
   defined it. Body from the debug build (0x0042DCF4): the first joined player whose
   name matches case-insensitively (the debug code calls `_wcsicmp`), else -1. */
int J3DAPI sithPlayer_GetPlayerNumByName(wchar_t* pwName)
{
    size_t i;

    if ( !pwName ) {
        return -1;
    }

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) == 0 ) {
            continue;
        }

        if ( !wstrcmpi(sithPlayer_aPlayers[i].awName, pwName) ) {
            return i;
        }
    }

    return -1;
}

int J3DAPI sithPlayer_GetPlayerNum(DPID playerId)
{
    size_t i;

    if ( !playerId )
    {
        return -1;
    }

    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( playerId == sithPlayer_aPlayers[i].playerNetId )
        {
            return i;
        }
    }

    return -1;
}

void J3DAPI sithPlayer_PlayerKilledAction(SithThing* pPlayerThing, const SithThing* pSrcThing)
{
    SithPlayer* pPlayer = pPlayerThing->thingInfo.actorInfo.pPlayer;
#line 658
    SITH_ASSERTREL(pPlayer);

    pPlayerThing->moveInfo.physics.flags |= (SITH_PF_ALIGNSURFACE | SITH_PF_USEGRAVITY);
    pPlayerThing->moveInfo.physics.flags &= ~(SITH_PF_ALIGNED | SITH_PF_ALIGNUP);

    pPlayerThing->flags |= SITH_TF_DYING;

    if ( (pPlayerThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) == 0 )
    {
        sithPhysics_ResetThingMovement(pPlayerThing);
    }
    else
    {
        memset(&pPlayerThing->moveInfo.physics.rotThrust, 0, sizeof(pPlayerThing->moveInfo.physics.rotThrust));
        memset(&pPlayerThing->moveInfo.physics.thrust, 0, sizeof(pPlayerThing->moveInfo.physics.thrust));
    }

    sithWeapon_DeactivateCurrentWeapon(pPlayerThing);
    sithInventory_BroadcastKilledMessage(pPlayerThing, pSrcThing);

    if ( stdComm_IsGameActive() )
    {
        sithMulti_ProcessKilledPlayer(pPlayer, pPlayerThing, pSrcThing);
    }
}

void J3DAPI sithPlayer_KillPlayer(SithThing* pPlayerThing)
{
    SithPlayer* pPlayer = pPlayerThing->thingInfo.actorInfo.pPlayer;

#line 709
    SITH_ASSERTREL(pPlayer);

    if ( pPlayerThing == sithPlayer_pLocalPlayerThing )
    {
        sithDSSThing_Death(pPlayerThing, pPlayerThing, 1, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }

    if ( (pPlayerThing->flags & SITH_TF_COGLINKED) != 0 )
    {
        sithCog_ThingSendMessage(pPlayerThing, pPlayerThing, SITHCOG_MSG_KILLED);
        if ( (pPlayerThing->flags & SITH_TF_DESTROYED) != 0 )
        {
            return;
        }
    }

    sithSoundMixer_StopSoundThing(pPlayerThing, 0);
    sithThing_DetachAttachedThings(pPlayerThing);
    sithActor_SetHeadPYR(pPlayerThing, &rdroid_zeroVector3);

    pPlayerThing->moveInfo.physics.flags |= SITH_PF_ALIGNSURFACE | SITH_PF_USEGRAVITY;
    pPlayerThing->moveInfo.physics.flags &= ~(SITH_PF_ALIGNUP | SITH_PF_ALIGNED);

    sithActor_KillActor(pPlayerThing, NULL, SITH_DAMAGE_DROWN);
    sithWeapon_DeactivateCurrentWeapon(pPlayerThing);

    if ( stdComm_IsGameActive() )
    {
        sithMulti_ProcessKilledPlayer(pPlayer, pPlayerThing, pPlayerThing);
    }
}

/* Linker-discarded local-player inventory accessors (debug build 0x0042DFBF, 0x0042E047, 0x0042E0D2, 0x0042E185, directly
   after KillPlayer; asserts at lines 764/765, 786/787, 809/810, 836/837). They act on the
   local player's inventory slot `binIndex` (the same data sithInventory_* reaches through a thing). Names inferred. The
   range asserts use STD_INRANGE, whose expansion the debug build prints (as in sithControl.c). */
float J3DAPI sithPlayer_GetBinAmount(int binIndex)
{
#line 764
    SITH_ASSERT(STD_INRANGE(sithPlayer_playerNum, 0, sithPlayer_numPlayers-1));
    SITH_ASSERT(binIndex < SITHINVENTORY_MAXTYPES);
    return sithPlayer_aPlayers[sithPlayer_playerNum].inventory.aItems[binIndex].amount;
}

void J3DAPI sithPlayer_SetBinAmount(int binIndex, float amount)
{
#line 786
    SITH_ASSERT(STD_INRANGE(sithPlayer_playerNum, 0, sithPlayer_numPlayers-1));
    SITH_ASSERT(binIndex < SITHINVENTORY_MAXTYPES);
    sithPlayer_aPlayers[sithPlayer_playerNum].inventory.aItems[binIndex].amount = amount;
}

void J3DAPI sithPlayer_SetBinAvailable(int binIndex, int bAvailable)
{
    SithInventoryItem* pItem;

#line 809
    SITH_ASSERT(STD_INRANGE(sithPlayer_playerNum, 0, sithPlayer_numPlayers-1));
    SITH_ASSERT(binIndex < SITHINVENTORY_MAXTYPES);
    pItem = &sithPlayer_aPlayers[sithPlayer_playerNum].inventory.aItems[binIndex];
    if ( bAvailable ) {
        pItem->status |= SITHINVENTORY_ITEM_AVAILABLE;
    }
    else {
        pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
    }
}

int J3DAPI sithPlayer_IsBinAvailable(int binIndex)
{
    SithInventoryItem* pItem;

#line 836
    SITH_ASSERT(STD_INRANGE(sithPlayer_playerNum, 0, sithPlayer_numPlayers-1));
    SITH_ASSERT(binIndex < SITHINVENTORY_MAXTYPES);
    pItem = &sithPlayer_aPlayers[sithPlayer_playerNum].inventory.aItems[binIndex];
    if ( (pItem->status & SITHINVENTORY_ITEM_AVAILABLE) != 0 ) {
        return 1;
    }

    return 0;
}

int J3DAPI sithPlayer_GetThingPlayerNum(const SithThing* pThing)
{
    int i;

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
        return -1;
    }

    if ( !stdComm_IsGameActive() )
    {
        return 0;
    }

    for ( i = 0; i < (int)sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && sithPlayer_aPlayers[i].pThing == pThing )
        {
            return i;
        }
    }

    return -1;
}

/* Linker-discarded (debug build 0x0042E2A2, between GetThingPlayerNum and ToggleGuybrush; no assert sites): the joined player
   whose thing has the given thing index. Name inferred from its sibling. */
int J3DAPI sithPlayer_GetThingIdxPlayerNum(int thingIdx)
{
    int i;

    if ( !stdComm_IsGameActive() ) {
        return 0;
    }

    for ( i = 0; i < (int)sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 && sithPlayer_aPlayers[i].pThing->idx == thingIdx ) {
            return i;
        }
    }

    return -1;
}

int J3DAPI sithPlayer_ToggleGuybrush(void)
{
    SithWeaponId i;
    SithThing* pPlayer;
    SithCog* pCog;

    pPlayer = sithPlayer_pLocalPlayerThing;
    if ( pPlayer )
    {
        if ( (pPlayer->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
        {
            return 0;
        }

        pCog = pPlayer->pCog;
        if ( pCog )
        {
            if ( sithPlayer_bGuybrush )
            {
                sithCog_SendMessage(pCog, SITHCOG_MSG_USER1, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pPlayer->idx, 0);
                sithPlayer_bGuybrush = 0;
            }
            else
            {
                sithCog_SendMessage(pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pPlayer->idx, 0);
                sithPlayer_bGuybrush = 1;
            }
        }

        for ( i = SITHWEAPON_SATCHEL; i < SITHWEAPON_IMP4; ++i )
        {
            pCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(i));
            if ( pCog )
            {
                sithCog_SendMessage(pCog, SITHCOG_MSG_STARTUP, SITHCOG_SYM_REF_INT, 9999, SITHCOG_SYM_REF_NONE, 0, 0);
            }
        }

        return 1;
    }

    return 0;
}

void J3DAPI sithPlayer_IMPStartFiring(int fireType)
{
    sithPlayer_impFireType = fireType;
}

void J3DAPI sithPlayer_IMPEndFiring(int fireType)
{
    if ( fireType == sithPlayer_impFireType )
    {
        sithPlayer_impFireType = -1;
    }
}
