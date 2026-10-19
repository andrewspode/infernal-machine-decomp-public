#include "sithCogFunctionPlayer.h"

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/Win95/stdComm.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithCogFunctionPlayer_countfit_1, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_2, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_3, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_4, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_5, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_6, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_7, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_8, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_9, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_10, 15)
CF_PROTO(sithCogFunctionPlayer_countfit_11, 5)
/* end count-fit */

void J3DAPI sithCogFunctionPlayer_IMPStartFiring(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_IMPEndFiring(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetPlayerInPor(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_StartInvisibility(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_EndInvisibility(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_JewelFlyingStatus(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_StartJewelFlying(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_MakeMeStop(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetInvActivated(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetInvAvailable(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetInvDisabled(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_IsInvActivated(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_IsInvAvailable(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetGoalFlags(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_ClearGoalFlags(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetNumPlayers(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetMaxPlayers(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetAbsoluteMaxPlayers(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetLocalPlayerThing(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetPlayerThing(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetPlayerNum(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_PickupBackpack(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_NthBackpackBin(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_NthBackpackValue(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetNumBackbackItems(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_CreateBackpack(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetRespawnMask(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetRespawnMask(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SyncScores(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetCurItem(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_IsAming(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetSwimmingInventory(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_IsInvisible(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_ResetInventory(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_IsItemFound(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_SetElectricWhip(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetCutsceneMode(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_MakeMePirate(SithCog* pCog);
void J3DAPI sithCogFunctionPlayer_GetLastWeapon(SithCog* pCog);

int J3DAPI sithCogFunctionPlayer_RegisterFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetInvActivated, "setinvactivated");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetInvAvailable, "setinvavailable");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetInvDisabled, "setinvdisabled");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IsInvActivated, "isinvactivated");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IsInvAvailable, "isinvavailable");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetGoalFlags, "setgoalflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_ClearGoalFlags, "cleargoalflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetNumPlayers, "getnumplayers");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetMaxPlayers, "getmaxplayers");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetAbsoluteMaxPlayers, "getabsolutemaxplayers");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetLocalPlayerThing, "getlocalplayerthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetPlayerThing, "getplayerthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetPlayerNum, "getplayernum");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_PickupBackpack, "pickupbackpack");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_CreateBackpack, "createbackpack");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_NthBackpackBin, "nthbackpackbin");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_NthBackpackValue, "nthbackpackvalue");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetNumBackbackItems, "numbackpackitems");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetRespawnMask, "getrespawnmask");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetRespawnMask, "setrespawnmask");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SyncScores, "syncscores");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetCurItem, "getcuritem");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_JewelFlyingStatus, "jewelflyingstatus");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_StartJewelFlying, "startjewelflying");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IsAming, "isaiming");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetSwimmingInventory, "setswimminginventory");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_StartInvisibility, "startinvisibility");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_EndInvisibility, "endinvisibility");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IsInvisible, "isinvisible");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_ResetInventory, "resetinventory");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_MakeMeStop, "makemestop");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IsItemFound, "isitemfound");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetElectricWhip, "setwhipelectric");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_SetPlayerInPor, "playerinpor");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetCutsceneMode, "getcutscenemode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_GetLastWeapon, "getlastweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_MakeMePirate, "makemeapirate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IMPStartFiring, "impstartfiring");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionPlayer_IMPEndFiring, "impendfiring");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithCogFunctionPlayer_IMPStartFiring(SithCog* pCog)
{
    int fireType = sithCogExec_PopInt(pCog);
    sithPlayer_IMPStartFiring(fireType);
}

void J3DAPI sithCogFunctionPlayer_IMPEndFiring(SithCog* pCog)
{
    int fireType = sithCogExec_PopInt(pCog);
    sithPlayer_IMPEndFiring(fireType);
}

void J3DAPI sithCogFunctionPlayer_SetPlayerInPor(SithCog* pCog)
{
    int bInPor = sithCogExec_PopInt(pCog);
    sithPlayer_bPlayerInPor = bInPor;
}

void J3DAPI sithCogFunctionPlayer_StartInvisibility(SithCog* pCog)
{
    sithPlayerActions_StartInvisibility();
}

void J3DAPI sithCogFunctionPlayer_EndInvisibility(SithCog* pCog)
{
    sithPlayerActions_EndInvisibility();
}

void J3DAPI sithCogFunctionPlayer_JewelFlyingStatus(SithCog* pCog)
{
    SithThing* pPlasma  = sithCogExec_PopThing(pCog);
    int bFlying;
    bFlying = sithCogExec_PopInt(pCog);
    if ( bFlying )
    {
        sithPlayerActions_EnableJewelFlying(pPlasma);
    }
    else if ( sithPlayer_pLocalPlayerThing )  // end flaying
    {
        if ( sithPlayer_pLocalPlayerThing->moveStatus != SITHPLAYERMOVE_JEWELFLYING )
        {
            sithPlayerActions_DisableJewelFlying();
        }
    }
}

void J3DAPI sithCogFunctionPlayer_StartJewelFlying(SithCog* pCog)
{
    int started = sithPlayerActions_StartJewelFlying();
    sithCogExec_PushInt(pCog, started);
}

void J3DAPI sithCogFunctionPlayer_MakeMeStop(SithCog* pCog)
{
    SithThing* pPlayer;
    int guid;

    pPlayer = sithPlayer_pLocalPlayerThing;
    if ( !pPlayer )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( (pPlayer->moveInfo.physics.flags & SITH_PF_MINECAR) != 0
        || (pPlayer->moveInfo.physics.flags & SITH_PF_RAFT) != 0
        || (pPlayer->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    guid = pPlayer->guid;
    switch ( pPlayer->moveStatus )
    {
        case SITHPLAYERMOVE_HANGING:
        case SITHPLAYERMOVE_CLIMBIDLE:
        case SITHPLAYERMOVE_SWIMIDLE:
        case SITHPLAYERMOVE_JEEP_IMPACT:
        case SITHPLAYERMOVE_CLIMBING_UP:
        case SITHPLAYERMOVE_CLIMBING_DOWN:
        case SITHPLAYERMOVE_CLIMBING_LEFT:
        case SITHPLAYERMOVE_CLIMBING_RIGHT:
            sithCogExec_PushInt(pCog, -1);
            return;

        case SITHPLAYERMOVE_CRAWL_STILL:
        case SITHPLAYERMOVE_UNKNOWN_4:
        case SITHPLAYERMOVE_UNKNOWN_5:
            sithPlayerActions_Crawl2Stand(pPlayer);
            pPlayer->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
            pCog->status          = SITHCOG_STATUS_WAITING_PLAYER_TO_STOP;
            pCog->statusParams[0] = guid;
            break;

        case SITHPLAYERMOVE_STILL:
        case SITHPLAYERMOVE_WALKING:
        case SITHPLAYERMOVE_RUNNING:
            sithPhysics_ResetThingMovement(pPlayer);
            pPlayer->moveStatus = SITHPLAYERMOVE_STILL;
            // fall through

        default:
            pPlayer->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
            pCog->status          = SITHCOG_STATUS_WAITING_PLAYER_TO_STOP;
            pCog->statusParams[0] = guid;
            break;
    }

    sithCogExec_PushInt(pCog, guid);
}

void J3DAPI sithCogFunctionPlayer_SetInvActivated(SithCog* pCog)
{
    int bActivated    = sithCogExec_PopInt(pCog);
    int typeId        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 365
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in SetInvActivated.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 371
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory id in SetInvActivated.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( bActivated )
    {
        sithInventory_SetInventoryActivated(pThing, typeId, 1);
    }
    else
    {
        sithInventory_SetInventoryActivated(pThing, typeId, 0);
    }
}

void J3DAPI sithCogFunctionPlayer_SetInvAvailable(SithCog* pCog)
{
    int bCurAvailable    = sithCogExec_PopInt(pCog);
    int typeId        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 404
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in SetInvAvailable.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 410
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory id in SetInvAvailable.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( bCurAvailable )
    {
        sithInventory_SetInventoryAvailable(pThing, typeId, 1);
    }
    else
    {
        sithInventory_SetInventoryAvailable(pThing, typeId, 0);
    }
}

void J3DAPI sithCogFunctionPlayer_SetInvDisabled(SithCog* pCog)
{
    int bDisabled     = sithCogExec_PopInt(pCog);
    int typeId        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 444
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetInvDisabled().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 450
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad inventory ID in SetInvDisabled().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithInventory_SetInventoryDisabled(pThing, typeId, bDisabled);
}

void J3DAPI sithCogFunctionPlayer_IsInvActivated(SithCog* pCog)
{
    int bin           = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 478
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in IsInvActivated.\n", pCog->aName, 0, 0, 0);

        // TODO: BUG missing return value 0
        return;
    }

    if ( bin >= SITHINVENTORY_MAXTYPES )
    {
#line 484
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory id in IsInvActivated.\n", pCog->aName, 0, 0, 0);

         // TODO: BUG missing return value 0
        return;
    }

    if ( sithInventory_IsInventoryActivated(pThing, bin) )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionPlayer_IsInvAvailable(SithCog* pCog)
{
    int typeId        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 516
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in IsInvAvailable.\n", pCog->aName, 0, 0, 0);

         // TODO: BUG missing return value 0
        return;
    }

    if ( typeId >= SITHINVENTORY_MAXTYPES )
    {
#line 522
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory id in IsInvAvailable.\n", pCog->aName, 0, 0, 0);

        // TODO: BUG missing return value 0
        return;
    }

    if ( sithInventory_IsInventoryAvailable(pThing, typeId) )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionPlayer_SetGoalFlags(SithCog* pCog)
{
    int curFlags;
    int flags         = sithCogExec_PopInt(pCog);
    int bin           = sithCogExec_PopInt(pCog) + 100; // 100 is watch item which doesn't make sense, so it must be leftover function from sith engine
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 556
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in SetGoalFlags.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( bin >= SITHINVENTORY_MAXTYPES )
    {
#line 562
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory bin in SetGoalFlags.\n", pCog->aName, 0, 0, 0);
        return;
    }

    curFlags = (int)sithInventory_GetInventory(pThing, bin);
    curFlags |= flags;
    sithInventory_SetInventory(pThing, bin, (float)curFlags);
}

void J3DAPI sithCogFunctionPlayer_ClearGoalFlags(SithCog* pCog)
{
    int curFlags;
    int flags         = sithCogExec_PopInt(pCog);
    int bin           = sithCogExec_PopInt(pCog) + 100; // 100 is watch item which doesn't make sense, so it must be leftover function from sith engine
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 596
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in ClearGoalFlags.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( bin >= SITHINVENTORY_MAXTYPES )
    {
#line 602
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory bin in ClearGoalFlags.\n", pCog->aName, 0, 0, 0);
        return;
    }

    curFlags = (int)sithInventory_GetInventory(pThing, bin);
    flags = curFlags & ~flags;
    sithInventory_SetInventory(pThing, bin, (float)flags);
}

void J3DAPI sithCogFunctionPlayer_GetNumPlayers(SithCog* pCog)
{
    int nPlayers = 0;
    size_t i;
    for ( i = 0; i < sithPlayer_numPlayers; ++i )
    {
        if ( (sithPlayer_aPlayers[i].flags & SITH_PLAYER_JOINEDGAME) != 0 )
        {
            ++nPlayers;
        }
    }

    sithCogExec_PushInt(pCog, nPlayers);
}

void J3DAPI sithCogFunctionPlayer_GetMaxPlayers(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithPlayer_numPlayers);
}

void J3DAPI sithCogFunctionPlayer_GetAbsoluteMaxPlayers(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, SITHPLAYER_MAX_PLAYERS);
}

void J3DAPI sithCogFunctionPlayer_GetLocalPlayerThing(SithCog* pCog)
{
    SithThing* pPlayer = sithPlayer_pLocalPlayerThing;
    if ( pPlayer )
    {
        sithCogExec_PushInt(pCog, pPlayer->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionPlayer_GetPlayerThing(SithCog* pCog)
{
    unsigned int playerNum = sithCogExec_PopInt(pCog);

    if ( playerNum >= sithPlayer_numPlayers )
    {
#line 723
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player num in GetPlayerThing.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, sithPlayer_aPlayers[playerNum].pThing->idx);
    }
}

void J3DAPI sithCogFunctionPlayer_GetPlayerNum(SithCog* pCog)
{
    int playerNum;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 753
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in GetPlayerNum.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    playerNum = sithPlayer_GetThingPlayerNum(pThing);
    if ( playerNum == -1 )
    {
#line 762
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player in GetPlayerNum.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;

    }

    sithCogExec_PushInt(pCog, playerNum);
}

void J3DAPI sithCogFunctionPlayer_PickupBackpack(SithCog* pCog)
{
    SithThing* pBackpack = sithCogExec_PopThing(pCog);
    SithThing* pThing    = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 789
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player in PickupBackpack.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !pBackpack
        || pBackpack->type != SITH_THING_ITEM
        || (pBackpack->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) == 0 )
    {
#line 795
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid backpack in PickupBackpack.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithInventory_PickupBackpack(pThing, pBackpack);
}

void J3DAPI sithCogFunctionPlayer_NthBackpackBin(SithCog* pCog)
{
    int itemNum       = sithCogExec_PopInt(pCog);

    int itemID;
    SithThing* pObj = sithCogExec_PopThing(pCog);

    if ( !pObj || pObj->type != SITH_THING_ITEM || (pObj->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) == 0 )
    {
#line 822
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid backpack in NthBackpackBin.\n", pCog->aName, 0, 0, 0);

        // TODO: BUG, missing return value
        return;
    }

    itemID = sithInventory_GetBackpackItemID(pObj, itemNum);
    sithCogExec_PushInt(pCog, itemID);
}

void J3DAPI sithCogFunctionPlayer_NthBackpackValue(SithCog* pCog)
{
    int itemId        = sithCogExec_PopInt(pCog);

    SithThing* curThing = sithCogExec_PopThing(pCog);
    float itemID;

    if ( !curThing || curThing->type != SITH_THING_ITEM || (curThing->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) == 0 )
    {
#line 852
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid backpack in NthBackpackBin.\n", pCog->aName, 0, 0, 0);

        // TODO: BUG, missing return value
        return;
    }

    itemID = sithInventory_GetBackpackItemValue(curThing, itemId);
    sithCogExec_PushFlex(pCog, itemID);
}

void J3DAPI sithCogFunctionPlayer_GetNumBackbackItems(SithCog* pCog)
{
    int numItems;

    SithThing* thing = sithCogExec_PopThing(pCog);

    if ( !thing || thing->type != SITH_THING_ITEM || (thing->thingInfo.itemInfo.flags & SITH_ITEM_BACKPACK) == 0 )
    {
#line 880
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid backpack in NthBackpackBin.\n", pCog->aName, 0, 0, 0);

        // TODO: BUG, missing return value
        return;
    }

    numItems = sithInventory_GetNumBackpackItems(thing);
    sithCogExec_PushInt(pCog, numItems);
}

void J3DAPI sithCogFunctionPlayer_CreateBackpack(SithCog* pCog)
{
    SithThing* pBackpack;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 907
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player in CreateBackpack.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pBackpack = sithInventory_CreateBackpack(pThing);
    if ( !pBackpack )
    {
#line 915
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid backpack in CreateBackpack.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pBackpack->idx);
    }
}

void J3DAPI sithCogFunctionPlayer_GetRespawnMask(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SithPlayer* pPlayer;
    if ( !pThing || pThing->type != SITH_THING_PLAYER || (pPlayer = pThing->thingInfo.actorInfo.pPlayer) == 0 )
    {
#line 945
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player thing in GetRespawnMask.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pPlayer->respawnMask);
    }
}

void J3DAPI sithCogFunctionPlayer_SetRespawnMask(SithCog* pCog)
{
    int mask          = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    SithPlayer* pPlayer;
    if ( !pThing || pThing->type != SITH_THING_PLAYER || (pPlayer = pThing->thingInfo.actorInfo.pPlayer) == NULL )
    {
#line 977
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player thing in SetRespawnMask.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        pPlayer->respawnMask = mask;
    }
}

void J3DAPI sithCogFunctionPlayer_SyncScores(SithCog* pCog)
{
    if ( stdComm_IsGameActive() )
    {
        sithMulti_SyncScores();
    }
}

void J3DAPI sithCogFunctionPlayer_GetCurItem(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    int itemID;
    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 1021
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player THING in GetCurItem().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        itemID = sithInventory_GetCurrentItem(pThing);
        sithCogExec_PushInt(pCog, itemID);
    }
}

void J3DAPI sithCogFunctionPlayer_IsAming(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || (pThing->type != SITH_THING_ACTOR) && pThing->type != SITH_THING_PLAYER )
    {
#line 1040
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid THING passed to IsAiming().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( !sithWeapon_IsAiming(pThing) )
    {
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunctionPlayer_SetSwimmingInventory(SithCog* pCog)
{
    int bItemsAvailable = sithCogExec_PopInt(pCog);
    SithThing* pThing   = sithCogExec_PopThing(pCog);
#line 1074
    SITH_ASSERTREL((pThing != NULL) && (pThing->type == SITH_THING_PLAYER));

    sithInventory_SetSwimmingInventory(pThing, bItemsAvailable);

    if ( !bItemsAvailable )
    {
        sithInventory_SetInventoryDisabled(pThing, SITHWEAPON_MACHETE, 0);
        sithInventory_SetInventoryDisabled(pThing, 93u, 0); // 93 - hammer
        sithInventory_SetInventoryDisabled(pThing, 94u, 0); // 94 - propellor
    }
}

void J3DAPI sithCogFunctionPlayer_IsInvisible(SithCog* pCog)
{
    if ( sithPlayerActions_IsInvisible() )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionPlayer_ResetInventory(SithCog* pCog)
{
    SithInventoryItem* pItem;
    SithInventoryItem* szItems;
    SithWeaponId i;

    SithThing* pThing  = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 1106
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid player thing in ResetInventory().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithInventory_InitInventory(pThing);
    sithThing_ResetSwapList(pThing);

    szItems = pThing->thingInfo.actorInfo.pPlayer->inventory.aItems;
    if ( szItems != NULL )
    {
        for ( i = SITHWEAPON_WHIP; i < SITHWEAPON_ZIPPO; ++i )
        {
            pItem = &szItems[i];
            if ( pItem )
            {
                pItem->status &= ~SITHINVENTORY_ITEM_FOUND;
            }
        }
    }
}

void J3DAPI sithCogFunctionPlayer_IsItemFound(SithCog* pCog)
{
    SithInventoryItem* szItems;
    SithInventoryItem* pItem;

    int itemID = sithCogExec_PopInt(pCog);
    if ( sithPlayer_pLocalPlayerThing )
    {
        szItems = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems;
        if ( szItems != NULL )
        {
            pItem = &szItems[itemID];
            if ( pItem )
            {
                if ( (pItem->status & SITHINVENTORY_ITEM_FOUND) != 0 )
                {
                    sithCogExec_PushInt(pCog, 1);
                }
                else
                {
                    sithCogExec_PushInt(pCog, 0);
                }
            }
        }
    }
}

void J3DAPI sithCogFunctionPlayer_SetElectricWhip(SithCog* pCog)
{
    int bElectrict = sithCogExec_PopInt(pCog);

    rdMaterial* pMaterial;
    if ( sithPlayer_pLocalPlayerThing )
    {
        if ( bElectrict )
        {
            sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.flags |= SITH_AF_ELECTRICWHIP;
        }
        else
        {
            sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_ELECTRICWHIP;
        }
    }

    pMaterial = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(326));// 0x8146
    if ( pMaterial )
    {
        pMaterial->curCelNum = bElectrict; // TODO: 0 - regular, 1 - elect , what happens if the num is out of bound ? :)
    }
}

void J3DAPI sithCogFunctionPlayer_GetCutsceneMode(SithCog* pCog)
{
    if ( sithPlayerControls_bCutsceneMode == 1 )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunctionPlayer_GetLastWeapon(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithWeapon_GetLastWeapon());
}

void J3DAPI sithCogFunctionPlayer_MakeMePirate(SithCog* pCog)
{
    sithPlayer_ToggleGuybrush();
}
