#include "sithInventory.h"

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithInventory_countfit_1, 15)
CF_PROTO(sithInventory_countfit_2, 15)
CF_PROTO(sithInventory_countfit_3, 19)

/* end count-fit */

int sithInventory_bInitInventory = 1;

int sithInventory_bSendDeactivateMessage = 0;
int sithInventory_dword_56B750 = 0;
int sithInventory_dword_56B754 = 0;

SithPowerKeybind sithInventory_aPowerKeybinds[20];
SithInventoryType sithInventory_aTypes[SITHINVENTORY_MAXTYPES];

void J3DAPI sithInventory_RegisterType(size_t typeId, const char* pName, float min, float max, SithInventoryTypeFlag flags, SithCog* pItemCog, rdModel3* pInvModel, rdModel3* pItemModel)
{
    SithInventoryType* pType;

#line 85
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pName);

    pType = &sithInventory_aTypes[typeId];
    strncpy(pType->aName, pName, STD_ARRAYLEN(pType->aName) - 1);
    pType->aName[STD_ARRAYLEN(pType->aName) - 1] = '\0';

    pType->min   = min;
    pType->max   = max;
    pType->flags = flags | SITHINVENTORY_TYPE_REGISTERED;
    pType->pCog  = pItemCog;

    if ( pItemCog && ((pType->flags & SITHINVENTORY_TYPE_ITEM) != 0 || (pType->flags & SITHINVENTORY_TYPE_WEAPON) != 0) )
    {
        pItemCog->flags |= SITHCOG_LOCAL;
    }

    pType->pInvIconModel = pInvModel;
    pType->pModel        = pItemModel;
    pType->pHolsterModel = NULL;
}

int J3DAPI sithInventory_FindNextTypeID(SithThing* pThing, int startSearchId, SithInventoryTypeFlag flags)
{
    int typeId;

#line 128
    SITH_ASSERTREL(startSearchId >= 0);

    for ( typeId = startSearchId + 1; typeId < SITHINVENTORY_MAXTYPES; ++typeId )
    {
        if ( (sithInventory_aTypes[typeId].flags & flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    for ( typeId = 0; typeId < startSearchId; ++typeId )
    {
        if ( (sithInventory_aTypes[typeId].flags & flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    return -1;
}

int J3DAPI sithInventory_FindPreviousTypeID(SithThing* pThing, int startSearchId, SithInventoryTypeFlag flags)
{
    int typeId;

#line 167
    SITH_ASSERTREL(startSearchId >= 0);

    for ( typeId = startSearchId - 1; typeId >= 0; --typeId )
    {
        if ( (sithInventory_aTypes[typeId].flags & flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    for ( typeId = SITHINVENTORY_MAXTYPES - 1; typeId > startSearchId; --typeId )
    {
        if ( (sithInventory_aTypes[typeId].flags & flags) != 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
        {
            return typeId;
        }
    }

    return -1;
}

/* Linker-discarded: retail __LINE__ gap 167..364 (sithInventory_FindPreviousTypeID -> sithInventory_GetCurrentWeapon), where the original
   defined it. Body restored from the Indy debug build's text. */
int J3DAPI sithInventory_FindNextItemID(SithThing* pThing, int itemId)
{
#line 205
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    if ( itemId < 0 )
    {
        itemId = pThing->thingInfo.actorInfo.pPlayer->inventory.curItemID;
    }

    return sithInventory_FindNextTypeID(pThing, itemId, SITHINVENTORY_TYPE_ITEM);
}

/* Linker-discarded: retail __LINE__ gap 167..364 (sithInventory_FindPreviousTypeID -> sithInventory_GetCurrentWeapon), where the original
   defined it. Body restored from the Indy debug build's text. */
int J3DAPI sithInventory_FindPreviousItemID(SithThing* pThing, int itemId)
{
#line 231
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    if ( itemId < 0 )
    {
        itemId = pThing->thingInfo.actorInfo.pPlayer->inventory.curItemID;
    }

    return sithInventory_FindPreviousTypeID(pThing, itemId, SITHINVENTORY_TYPE_ITEM);
}

/* Linker-discarded: retail __LINE__ gap 167..364 (sithInventory_FindPreviousTypeID -> sithInventory_GetCurrentWeapon), where the original
   defined it. Body restored from the Indy debug build's text. */
void J3DAPI sithInventory_SelectItem(SithThing* pThing, int typeId)
{
    SithInventory* pInventory;
    int curItemId;
    SithInventoryType* pType;

#line 258
    SITH_ASSERTREL(pThing);

    if ( typeId >= 0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
    {
        pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
        curItemId  = pInventory->curItemID;

        pType = sithInventory_GetType(curItemId);
        if ( pType->pCog )
        {
            sithCog_SendMessage(pType->pCog, SITHCOG_MSG_DESELECTED, SITHCOG_SYM_REF_INT, curItemId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
            if ( sithInventory_bSendDeactivateMessage == 1 )
            {
                sithCog_SendMessage(pType->pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_INT, curItemId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
                sithInventory_bSendDeactivateMessage = 0;
            }
        }

        pType = sithInventory_GetType(typeId);
        if ( pType->pCog )
        {
            sithCog_SendMessage(pType->pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_INT, typeId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }

        pInventory->curItemID = typeId;
    }
}

/* Linker-discarded: retail __LINE__ gap 167..364 (sithInventory_FindPreviousTypeID -> sithInventory_GetCurrentWeapon), where the original
   defined it. Body restored from the Indy debug build's text. */
void J3DAPI sithInventory_SelectNextItem(SithThing* pThing)
{
    int itemId;

#line 300
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    itemId = sithInventory_FindNextItemID(pThing, -1);
    sithInventory_SelectItem(pThing, itemId);
}

/* Linker-discarded: retail __LINE__ gap 167..364 (sithInventory_FindPreviousTypeID -> sithInventory_GetCurrentWeapon), where the original
   defined it. Body restored from the Indy debug build's text. */
void J3DAPI sithInventory_SelectPreviousItem(SithThing* pThing)
{
    int itemId;

#line 323
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    itemId = sithInventory_FindPreviousItemID(pThing, -1);
    sithInventory_SelectItem(pThing, itemId);
}

SithInventoryType* J3DAPI sithInventory_GetType(size_t typeId)
{
    return &sithInventory_aTypes[typeId];
}

int J3DAPI sithInventory_GetCurrentWeapon(const SithThing* pThing)
{
    SithInventory* pInventory;
    int weaponID = SITHWEAPON_NO_WEAPON;

#line 364
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
    {
        weaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    }
    else if ( pThing->type == SITH_THING_PLAYER )
    {
#line 372
        SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
        pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
        weaponID = pInventory->curWeaponID;
    }

    return weaponID;
}

void J3DAPI sithInventory_SetCurrentWeapon(SithThing* pThing, SithWeaponId weaponID)
{
    SithInventory* pInventory;

#line 397
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pInventory->curWeaponID = weaponID;

    switch ( weaponID )
    {
        case SITHWEAPON_PISTOL:
        case SITHWEAPON_TOKAREV:
        case SITHWEAPON_MAUSER:
            pInventory->lastPistolID = weaponID;
            break;

        case SITHWEAPON_SIMONOV:
        case SITHWEAPON_MACHETE:
        case SITHWEAPON_SUBMACHINE:
        case SITHWEAPON_SHOTGUN:
        case SITHWEAPON_BAZOOKA:
            pInventory->lastRifleID = weaponID;
            break;
    }
}

int J3DAPI sithInventory_GetCurrentItem(SithThing* pThing)
{
    SithInventory* pInventory;

#line 438
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    return pInventory->curItemID;
}

void J3DAPI sithInventory_SetCurrentItem(SithThing* pThing, int typeId)
{
    SithInventory* pInventory;

#line 462
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);
    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pInventory->curItemID = typeId;
}

float J3DAPI sithInventory_ChangeInventory(SithThing* pThing, size_t typeId, float amount)
{
    SithInventory* pInventory;
    SithInventoryType* pType;
    SithInventoryItem* pItem;
    float lastAmt;

#line 489
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return 0.0f;
    }

    pItem = &pInventory->aItems[typeId];

    lastAmt = pItem->amount;
    pItem->amount += amount;
    pItem->amount = STDMATH_CLAMP(pItem->amount, pType->min, pType->max);

    if ( lastAmt != pItem->amount )
    {
        if ( pType->pCog )
        {
            sithCog_SendMessage(pType->pCog, SITHCOG_MSG_CHANGED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, typeId);
        }

        pItem->status |= SITHINVENTORY_ITEM_CHANGED;
        if ( (pItem->status & SITHINVENTORY_ITEM_FOUND) == 0 )
        {
            pItem->status |= SITHINVENTORY_ITEM_FOUND;
        }

        if ( pItem->amount == 0.0f )
        {
            pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
        }
    }

    return pItem->amount;
}

float J3DAPI sithInventory_GetInventory(SithThing* pThing, size_t typeId)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 547
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;

    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return 0.0f;
    }

    return pInventory->aItems[typeId].amount;
}

float J3DAPI sithInventory_SetInventory(SithThing* pThing, size_t typeId, float amount)
{
    SithInventory* pInventory;
    SithInventoryType* pType;
    SithInventoryItem* pItem;
    float lastAmt;

#line 582
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return 0.0f;
    }

    pItem = &pInventory->aItems[typeId];

    lastAmt = pItem->amount;
    pItem->amount = amount;
    pItem->amount = STDMATH_CLAMP(pItem->amount, pType->min, pType->max);

    if ( lastAmt != pItem->amount )
    {
        if ( pType->pCog )
        {
            sithCog_SendMessage(pType->pCog, SITHCOG_MSG_CHANGED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, typeId);
        }

        pItem->status |= SITHINVENTORY_ITEM_CHANGED;
        if ( (pItem->status & SITHINVENTORY_ITEM_FOUND) == 0 )
        {
            pItem->status |= SITHINVENTORY_ITEM_FOUND;
        }

        if ( pItem->amount == 0.0f )
        {
            pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
        }
    }

    return pItem->amount;
}

void J3DAPI sithInventory_SetInventoryActivated(SithThing* pThing, size_t typeId, int bActivated)
{
    SithInventoryType* pType;
    SithInventory* pInventory;
    SithInventoryItem* pItem;

#line 648
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;

    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return;
    }

    pItem = &pInventory->aItems[typeId];
    if ( bActivated )
    {
        pItem->status |= SITHINVENTORY_ITEM_ACTIVATED;
    }
    else
    {
        pItem->status &= ~SITHINVENTORY_ITEM_ACTIVATED;
    }
}

int J3DAPI sithInventory_IsInventoryActivated(SithThing* pThing, size_t typeId)
{
    SithInventoryType* pType;
    SithInventory* pInventory;
    SithInventoryItem* pItem;

#line 686
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;

    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return 0;
    }

    pItem = &pInventory->aItems[typeId];
    if ( (pItem->status & SITHINVENTORY_ITEM_ACTIVATED) != 0 )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithInventory_SetInventoryAvailable(SithThing* pThing, size_t typeId, int bAvailable)
{
    SithInventoryType* pType;
    SithInventory* pInventory;
    SithInventoryItem* pItem;

#line 728
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;

    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return;
    }

    pItem = &pInventory->aItems[typeId];
    if ( bAvailable )
    {
        pItem->status |= SITHINVENTORY_ITEM_AVAILABLE;
    }
    else
    {
        pItem->status &= ~SITHINVENTORY_ITEM_AVAILABLE;
    }
}

int J3DAPI sithInventory_IsInventoryAvailable(const SithThing* pThing, size_t typeId)
{
    SithInventoryType* pType;
    SithInventory* pInventory;
    SithInventoryItem* pItem;

#line 766
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;

    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return 0;
    }

    pItem = &pInventory->aItems[typeId];
    if ( (pItem->status & SITHINVENTORY_ITEM_AVAILABLE) != 0 && (pItem->status & SITHINVENTORY_ITEM_DISABLED) == 0 )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithInventory_SetInventoryDisabled(SithThing* pThing, size_t typeId, int bDisabled)
{
    SithInventoryType* pType;
    SithInventory* pInventory;
    SithInventoryItem* pItem;

#line 808
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);

    if ( !pThing->thingInfo.actorInfo.pPlayer )
    {
        return;
    }

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return;
    }

    pItem = &pInventory->aItems[typeId];
    if ( bDisabled )
    {
        pItem->status |= SITHINVENTORY_ITEM_DISABLED;
    }
    else
    {
        pItem->status &= ~SITHINVENTORY_ITEM_DISABLED;
    }
}

int J3DAPI sithInventory_IsBackpackItem(SithThing* pThing, size_t typeId)
{
    SithInventoryType* pType;

#line 841
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pType = &sithInventory_aTypes[typeId];
    if ( (pType->flags & SITHINVENTORY_TYPE_REGISTERED) != 0 && (pType->flags & SITHINVENTORY_TYPE_BACKPACKITEM) != 0 )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithInventory_IsWeapon(size_t typeID)
{
    SithInventoryType* pType;

#line 868
    SITH_ASSERTREL(typeID < SITHINVENTORY_MAXTYPES);
    pType = &sithInventory_aTypes[typeID];
    if ( (pType->flags & SITHINVENTORY_TYPE_WEAPON) != 0 )
    {
        return 1;
    }

    return 0;
}

float J3DAPI sithInventory_GetInventoryMinimum(SithThing* pThing, size_t id)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 893
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType      = &sithInventory_aTypes[id];
    return pType->min;
}

float J3DAPI sithInventory_GetInventoryMaximum(SithThing* pThing, size_t id)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 920
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType      = &sithInventory_aTypes[id];
    return pType->max;
}

void J3DAPI sithInventory_SetInventoryFlags(SithThing* pThing, size_t id, SithInventoryTypeFlag flags)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 947
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(id < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType      = &sithInventory_aTypes[id];
    pType->flags |= flags;
}

/* Linker-discarded, retail __LINE__ gap 949..1072 (sithInventory_SetInventoryFlags -> sithInventory_BroadcastMessage), where the
   original defined GetInventoryFlags, ClearInventoryFlags and BroadcastInventoryMessage. Bodies and lines from the Indy
   debug build (0x0045D5BE lines 974..976, 0x0045D65D 1001..1003, 0x0045D708 1032/1033),
   recompiled /Od and identical to it; the asserts are the compiled-out SITH_ASSERT family (no
   retail copy exists to say whether the original spelled them REL). */
SithInventoryTypeFlag J3DAPI sithInventory_GetInventoryFlags(const SithThing* pThing, size_t id)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 974
    SITH_ASSERT(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERT(id < SITHINVENTORY_MAXTYPES);
    SITH_ASSERT(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType      = &sithInventory_aTypes[id];
    return pType->flags;
}

void J3DAPI sithInventory_ClearInventoryFlags(const SithThing* pThing, size_t id, SithInventoryTypeFlag flags)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 1001
    SITH_ASSERT(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERT(id < SITHINVENTORY_MAXTYPES);
    SITH_ASSERT(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType      = &sithInventory_aTypes[id];
    pType->flags &= ~flags;
}

int J3DAPI sithInventory_BroadcastInventoryMessage(SithThing* pThing, SithCogSymbolRefType srcType, int srcIdx, SithCogMsgType msg, SithInventoryItemStatus status, int param0, int param1, int param2, int param3)
{
    SithInventoryType* pType;
    int retVal;
    int i;
    SithInventory* pInventory;

#line 1032
    SITH_ASSERT(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERT(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    retVal     = 0;
    for ( i = 0; i < SITHINVENTORY_MAXTYPES; ++i )
    {
        if ( (pInventory->aItems[i].status & status) != 0 && sithInventory_IsInventoryAvailable(pThing, i) )
        {
            pType = &sithInventory_aTypes[i];
            if ( pType->pCog ) {
                retVal += sithCog_SendMessageEx(pType->pCog, msg, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, -1, param0, param1, param2, param3);
            }
        }
    }

    return retVal;
}

float J3DAPI sithInventory_BroadcastMessage(SithThing* pThing, SithCogSymbolRefType srcType, int srcIdx, SithCogMsgType messageType, SithInventoryTypeFlag flags, float param0, int param1, int param2, int param3)
{
    SithInventoryType* pType;
    int i;

#line 1072
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    for ( i = 0; i < SITHINVENTORY_MAXTYPES; ++i )
    {
        if ( sithInventory_IsInventoryAvailable(pThing, i) )
        {
            pType = &sithInventory_aTypes[i];
            if ( (pType->flags & flags) != 0 && pType->pCog )
            {
                param0 = (float)(unsigned int)sithCog_SendMessageEx(pType->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcType, srcIdx, -1, (int)param0, param1, param2, param3);
            }
        }
    }

    return param0;
}

void J3DAPI sithInventory_ResetInventory(SithThing* pThing)
{
    SithInventoryType* pType;
    SithInventory* pInventory;
    int i;

#line 1113
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    if ( sithInventory_bInitInventory || stdComm_IsGameActive() )
    {
        sithInventory_InitInventory(pThing);
        sithInventory_bInitInventory = 0;
    }

    for ( i = 0; i < SITHINVENTORY_MAXTYPES; ++i )
    {
        pType = &sithInventory_aTypes[i];
        if ( (pType->flags & SITHINVENTORY_TYPE_NOT_CARRIED_BETWEEN_LEVELS) != 0 )
        {
            sithInventory_SetInventoryAvailable(pThing, i, 0);
            sithInventory_SetInventory(pThing, i, 0.0f);
        }
    }

    pInventory->curItemID   = 0;
    pInventory->curWeaponID = SITHWEAPON_NO_WEAPON;

    sithInventory_bSendDeactivateMessage = 0;
    sithInventory_dword_56B750 = 0;
    sithInventory_dword_56B754 = 0;
}

/* Linker-discarded, retail __LINE__ gap 1114..1205 (sithInventory_ResetInventory -> sithInventory_CreateBackpack), where the original
   defined it. Body from the Indy debug build (0x0045DA50, lines 1175/1176):
   zero every type that is not carried between levels. */
void J3DAPI sithInventory_ResetAllTypes(SithThing* pThing)
{
    SithInventoryType* pType;
    int i;

#line 1175
    SITH_ASSERT(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERT(pThing->thingInfo.actorInfo.pPlayer);

    for ( i = 0; i < SITHINVENTORY_MAXTYPES; ++i )
    {
        pType = &sithInventory_aTypes[i];
        if ( (pType->flags & SITHINVENTORY_TYPE_NOT_CARRIED_BETWEEN_LEVELS) != 0 ) {
            sithInventory_SetInventory(pThing, i, 0.0f);
        }
    }
}

SithThing* J3DAPI sithInventory_CreateBackpack(SithThing* pThing)
{
    SithThing* template;
    SithThing* backpack;
    int curtypeId;
    float amount;
    SithItemBackpack* pBackpackInfo;

#line 1205
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    if ( !stdComm_IsGameActive() )
    {
        return NULL;
    }

    template = sithTemplate_GetTemplate("+backpack");
    if ( !template )
    {
        return NULL;
    }

    backpack = sithThing_CreateThing(template, pThing);
    if ( !backpack )
    {
        return NULL;
    }

    backpack->thingInfo.itemInfo.flags |= SITH_ITEM_BACKPACK;
    pBackpackInfo = &backpack->thingInfo.itemInfo.backpack;
    pBackpackInfo->numItems = 0;

    for ( curtypeId = 0; curtypeId < SITHINVENTORY_MAXTYPES; ++curtypeId )
    {
        if ( sithInventory_IsBackpackItem(pThing, curtypeId) )
        {
            amount = sithInventory_GetInventory(pThing, curtypeId);
            if ( pBackpackInfo->numItems < 32 && amount > 0.0f )
            {
                pBackpackInfo->aItems[pBackpackInfo->numItems].typeId = curtypeId;
                pBackpackInfo->aItems[pBackpackInfo->numItems].amount = amount;
                pBackpackInfo->numItems++;
            }
        }
    }

    sithDSSThing_CreateThing(template, backpack, pThing, NULL, NULL, NULL, SITHMESSAGE_STREAM_ALL, DPSEND_GUARANTEED);
    sithDSSThing_UpdateState(backpack, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    return backpack;
}

void J3DAPI sithInventory_PickupBackpack(SithThing* pPlayerThing, SithThing* pBackpackThing)
{
    SithItemBackpack* pBackpackInfo;
    int i;

#line 1271
    SITH_ASSERTREL(pPlayerThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pPlayerThing->thingInfo.actorInfo.pPlayer);

#line 1274
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));

    pBackpackInfo = &pBackpackThing->thingInfo.itemInfo.backpack;
    for ( i = 0; i < pBackpackInfo->numItems; ++i )
    {
        if ( sithInventory_IsBackpackItem(pPlayerThing, pBackpackInfo->aItems[i].typeId) )
        {
            sithInventory_ChangeInventory(pPlayerThing, pBackpackInfo->aItems[i].typeId, pBackpackInfo->aItems[i].amount);
        }
    }
}

int J3DAPI sithInventory_GetBackpackItemID(SithThing* pBackpackThing, int itemNum)
{
    SithItemBackpack* pBackpackInfo;

#line 1303
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));
    pBackpackInfo = &pBackpackThing->thingInfo.itemInfo.backpack;
    if ( itemNum < pBackpackInfo->numItems )
    {
        return pBackpackInfo->aItems[itemNum].typeId;
    }

    return -1;
}

float J3DAPI sithInventory_GetBackpackItemValue(SithThing* pBackpackThing, int itemNum)
{
    SithItemBackpack* pBackpackInfo;

#line 1331
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));
    pBackpackInfo = &pBackpackThing->thingInfo.itemInfo.backpack;
    if ( itemNum < pBackpackInfo->numItems )
    {
        return pBackpackInfo->aItems[itemNum].amount;
    }

    return -1.0f;
}

int J3DAPI sithInventory_GetNumBackpackItems(SithThing* pBackpackThing)
{
    SithItemBackpack* pBackpackInfo;

#line 1359
    SITH_ASSERTREL(pBackpackThing->type == SITH_THING_ITEM);
    SITH_ASSERTREL(((pBackpackThing->thingInfo.itemInfo.flags) & (SITH_ITEM_BACKPACK)));
    pBackpackInfo = &pBackpackThing->thingInfo.itemInfo.backpack;
    return pBackpackInfo->numItems;
}

/* Linker-discarded (debug build 0x0045DF00, lines 1553..1555; between GetNumBackpackItems and
   GetInventoryType): the player's inventory slot of a registered type, else NULL. Name inferred from GetInventoryType. */
SithInventoryItem* J3DAPI sithInventory_GetInventoryItem(SithThing* pThing, size_t typeId)
{
    SithInventoryItem* pItem;
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 1553
    SITH_ASSERT(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERT(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERT(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;
    pType      = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 ) {
        return NULL;
    }

    pItem = &pInventory->aItems[typeId];
    return pItem;
}

SithInventoryType* J3DAPI sithInventory_GetInventoryType(SithThing* pThing, size_t typeId)
{
    SithInventoryType* pType;
    SithInventory* pInventory;

#line 1586
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(typeId < SITHINVENTORY_MAXTYPES);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    pInventory = &pThing->thingInfo.actorInfo.pPlayer->inventory;

    pType = &sithInventory_aTypes[typeId];
    if ( pInventory == NULL || (pType->flags & SITHINVENTORY_TYPE_REGISTERED) == 0 )
    {
        return NULL;
    }

    return pType;
}

/* Linker-discarded: retail __LINE__ gap 1588..1706 (sithInventory_GetInventoryType -> sithInventory_InitInventory), where the
   debug build has sub_45E072/sub_45E0B0/sub_45E0E4 over sithInventory_aPowerKeybinds. Names are Jedi Knight's
   (OpenJKDF2) for the same three bodies; body restored from the debug build's text. */
int J3DAPI sithInventory_KeybindInit(void)
{
    int result;
    int i;

    result = 0;
    for ( i = 0; i < 20; ++i ) {
        sithInventory_aPowerKeybinds[i].bEnabled = 0;
    }

    return result;
}

/* Linker-discarded: retail __LINE__ gap 1588..1706 (sithInventory_GetInventoryType -> sithInventory_InitInventory), where the
   debug build has sub_45E072/sub_45E0B0/sub_45E0E4 over sithInventory_aPowerKeybinds. Names are Jedi Knight's
   (OpenJKDF2) for the same three bodies; body restored from the debug build's text. */
void J3DAPI sithInventory_SetPowerKeybind(size_t idx, int binding)
{
    sithInventory_aPowerKeybinds[idx].bEnabled   = 1;
    sithInventory_aPowerKeybinds[idx].binding    = binding;
    sithInventory_aPowerKeybinds[idx].bActivated = 0;
}

/* Linker-discarded: retail __LINE__ gap 1588..1706 (sithInventory_GetInventoryType -> sithInventory_InitInventory), where the
   debug build has sub_45E072/sub_45E0B0/sub_45E0E4 over sithInventory_aPowerKeybinds. Names are Jedi Knight's
   (OpenJKDF2) for the same three bodies; body restored from the debug build's text. */
int J3DAPI sithInventory_GetPowerKeybind(size_t idx)
{
    if ( sithInventory_aPowerKeybinds[idx].bEnabled == 1 )
    {
        return sithInventory_aPowerKeybinds[idx].binding;
    }

    return -1;
}

void J3DAPI sithInventory_InitInventory(SithThing* pThing)
{
    SithInventoryType* pType;
    int i;

#line 1706
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(pThing->thingInfo.actorInfo.pPlayer);

    for ( i = 0; i < SITHINVENTORY_MAXTYPES; ++i )
    {
        sithInventory_SetInventory(pThing, i, 0.0f);
        pType = &sithInventory_aTypes[i];
        if ( (pType->flags & SITHINVENTORY_TYPE_DEFAULT) != 0 )
        {
            sithInventory_SetInventoryAvailable(pThing, i, 1);
        }
        else
        {
            sithInventory_SetInventoryAvailable(pThing, i, 0);
        }

        sithInventory_SetInventoryActivated(pThing, i, 0);
    }

    pThing->thingInfo.actorInfo.pPlayer->inventory.lastPistolID = SITHWEAPON_PISTOL;
    pThing->thingInfo.actorInfo.pPlayer->inventory.lastRifleID  = SITHWEAPON_NO_WEAPON;
}

void J3DAPI sithInventory_BroadcastKilledMessage(const SithThing* pSender, const SithThing* pKiller)
{
    int i;
    SithInventoryType* pType;

    for ( i = 0; i < SITHINVENTORY_MAXTYPES; ++i )
    {
        if ( sithInventory_IsInventoryAvailable(pSender, i) )
        {
            pType = &sithInventory_aTypes[i];
            if ( pType->pCog )
            {
                sithCog_SendMessage(pType->pCog, SITHCOG_MSG_KILLED, SITHCOG_SYM_REF_THING, pSender->idx, SITHCOG_SYM_REF_THING, pKiller ? pKiller->idx : -1, 0);
            }
        }
    }
}

void J3DAPI sithInventory_SetSwimmingInventory(SithThing* pThing, int bItemsAvailable)
{
    size_t typeId;

    for ( typeId = SITHWEAPON_FISTS; typeId <= SITHWEAPON_IMP5; ++typeId )
    {
        sithInventory_SetInventoryDisabled(pThing, typeId, bItemsAvailable == 0);
    }

    sithInventory_SetInventoryDisabled(pThing, SITHWEAPON_CHALK, bItemsAvailable == 0);

    for ( typeId = SITHWEAPON_MIRROR; typeId <= 118; ++typeId )
    {
        sithInventory_SetInventoryDisabled(pThing, typeId, bItemsAvailable == 0);
    }
}
