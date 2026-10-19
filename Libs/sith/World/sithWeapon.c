#define RDVECTOR_NO_INLINE_SET3 // Retail calls rdVector_Set3 out of line at 0x00449274 in sithWeapon_FireProjectile (target 0x004CBBA0)
#include "sithWeapon.h"

#include <stdio.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithWeapon_countfit_1, 30)
CF_PROTO(sithWeapon_countfit_2, 30)
CF_PROTO(sithWeapon_countfit_3, 30)
CF_PROTO(sithWeapon_countfit_4, 30)
CF_PROTO(sithWeapon_countfit_5, 30)
CF_PROTO(sithWeapon_countfit_6, 30)
CF_PROTO(sithWeapon_countfit_7, 30)
CF_PROTO(sithWeapon_countfit_8, 30)
CF_PROTO(sithWeapon_countfit_9, 30)
CF_PROTO(sithWeapon_countfit_10, 30)
CF_PROTO(sithWeapon_countfit_11, 30)
CF_PROTO(sithWeapon_countfit_12, 30)
CF_PROTO(sithWeapon_countfit_13, 30)
CF_PROTO(sithWeapon_countfit_14, 7)
/* end count-fit */
#define SITHWEAPON_MAX_PROJECTILE_RICOCHETS 6

static int sithWeapon_bGenBloodsplort = 1;

static float sithWeapon_aMaxAimDistances[25] = { // up to SITHWEAPON_COMSHOTGUN
    0.0f, 0.0f, 0.0f, 1.0f, 1.5f, 1.5f, 2.5f, 0.0f, 1.5f, 1.0f,
    3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.5f, 1.5f, 2.5f, 1.5f, 1.0f
};

// Marduk mirror rebound tuning constants; retail holds them in writable .data
// at 0x0052BDAC-0x0052BDBC and only sithWeapon_ThingCollisionHandler reads them.

// Retail's .bss block is 0x0056B6FC-0x0056B72C (0x30 bytes, twelve 4-byte
// scalars, align 4). Every object carries an explicit zero initializer, so the
// block has no uninitialized group and VC5 lays all twelve out in declaration
// order; the order below is retail's
// address order. Do not drop an initializer or reorder these: an uninitialized
// static jumps into the identifier-keyed group that comes first, and that is
// what left this block permuted. Each address is pinned by its consumers in
// the retail bytes: 0x0056B6FC by Get/SetLastWeapon, 0x0056B714 by IsAiming
// and SendMessageAim, 0x0056B720 by Get/SetMountWait and IsMountingWeapon,
// 0x0056B728 by SetActorKilledCallback, 0x0056B718 by Open (and no save or
// restore), and the three holster slots by sithWeapon_SetHolsterModel's own
// switch, whose case bodies store 0x0056B704, 0x0056B708 and 0x0056B71C in
// whip/pistol/back source order (0x00449920, 0x0044993C, 0x00449958).
static SithWeaponId sithWeapon_lastPlayerWeaponID = 0;
static int sithWeapon_bPlayerWeaponActivated = 0;
static int sithWeapon_playerWhipHolsterSwapRefNum = 0;
static int sithWeapon_playerPistolHolsterSwapRefNum = 0;
static int sithWeapon_deactivatedPlayerWeaponID = 0;
static float sithWeapon_secPlayerWeaponDeactivationTime = 0.0f;
static int sithWeapon_bLocalPlayerAiming = 0;
static float sithWeapon_secWeaponActivationWaitEndTime = 0.0f;
static int sithWeapon_playerBackHolsterSwapRefNum = 0;
float sithWeapon_secMountWait = 0.0f;
static SithControlFunction sithWeapon_bufferedWaponKeyId = 0;
static SithWeaponActorKilledCallback sithWeapon_pfActorKilledCallback = NULL;

void J3DAPI sithWeapon_HandleImpact(SithThing* pWeapon);
int J3DAPI sithWeapon_IsLocalPlayerUnableToUseWeapon(void);

// Linker-discarded: the debug build keeps this empty, uncalled function right before Open (0x457280); name inferred
// (Jedi Knight's sithWeapon_Startup).
void J3DAPI sithWeapon_Startup(void)
{}

void J3DAPI sithWeapon_Open(void)
{
    sithWeapon_secWeaponActivationWaitEndTime = 0.0f;
}

void J3DAPI sithWeapon_Close(void)
{}

void J3DAPI sithWeapon_InitalizeActor(SithThing* pThing)
{
#line 178
    SITH_ASSERTREL((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER));

    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationStartTime   = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired          = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime    = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secAimWaitEndTime              = 0.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secWeaponSwapTime              = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID               = -1;
    pThing->thingInfo.actorInfo.weaponInfo.curWeaponID                    = 0;
    pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID             = -1;
    pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum               = -1;

    memset(&pThing->thingInfo.actorInfo.weaponInfo.vecUnknown0, 0, sizeof(pThing->thingInfo.actorInfo.weaponInfo.vecUnknown0));

    sithWeapon_secWeaponActivationWaitEndTime = 0.0f;

    if ( pThing->type == SITH_THING_PLAYER )
    {
        sithWeapon_lastPlayerWeaponID              = SITHWEAPON_PISTOL;
        sithWeapon_bufferedWaponKeyId              = -1;
        sithWeapon_deactivatedPlayerWeaponID       = -1;
        sithWeapon_playerWhipHolsterSwapRefNum     = -1;
        sithWeapon_playerPistolHolsterSwapRefNum   = -1;
        sithWeapon_playerBackHolsterSwapRefNum     = -1;
        sithWeapon_secMountWait                    = 0.0f;
        sithWeapon_secPlayerWeaponDeactivationTime = 0.0f;
        sithWeapon_bPlayerWeaponActivated          = 0;
        sithWeapon_bLocalPlayerAiming              = 0;
    }
}

void J3DAPI sithWeapon_UpdateActorWeaponState(SithThing* pThing, float secDeltaTime)
{
    SithInventoryType* pItem;

    if ( pThing->thingInfo.actorInfo.weaponInfo.secWeaponSwapTime > 0.0f && sithTime_secGameTime >= pThing->thingInfo.actorInfo.weaponInfo.secWeaponSwapTime )
    {
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponSwapTime = -1.0f;
        sithWeapon_SetWeaponModel(pThing, (SithWeaponId)pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
    }

    if ( sithWeapon_deactivatedPlayerWeaponID != -1 && pThing->type == SITH_THING_PLAYER )
    {
        if ( sithTime_secGameTime < sithWeapon_secPlayerWeaponDeactivationTime )
        {
            return;
        }

        pItem = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
        if ( pItem->pCog )
        {
            sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }

        sithWeapon_deactivatedPlayerWeaponID = -1;
    }
    else if ( pThing->type == SITH_THING_PLAYER && sithWeapon_IsLocalPlayerUnableToUseWeapon() )
    {
        if ( sithWeapon_bPlayerWeaponActivated == 1 )
        {
            sithWeapon_bPlayerWeaponActivated    = 0;
            sithWeapon_deactivatedPlayerWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
            sithWeapon_secPlayerWeaponDeactivationTime = sithTime_secGameTime;
        }
    }
    else if ( pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID != -1 )
    {
        if ( sithTime_secGameTime < pThing->thingInfo.actorInfo.weaponInfo.secAimWaitEndTime )
        {
            return;
        }

        if ( pThing->type == SITH_THING_PLAYER && sithWeapon_bPlayerWeaponActivated == 1 )
        {
            sithWeapon_DeactivateCurrentWeapon(pThing);
            return;
        }
        else
        {
            pItem = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID);
            if ( pItem->pCog )
            {
                sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_DESELECTED, SITHCOG_SYM_REF_INT, pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID, SITHCOG_SYM_REF_THING, pThing->idx, 0);
            }

            if ( pThing->type == SITH_THING_PLAYER )
            {
                sithWeapon_bLocalPlayerAiming = 0;
                sithInventory_SetCurrentWeapon(pThing, SITHWEAPON_NO_WEAPON);
                if ( (pItem->flags & SITHINVENTORY_TYPE_PLAYERWEAPON) != 0 )
                {
                    sithWeapon_lastPlayerWeaponID = pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID;
                }
            }

            pThing->thingInfo.actorInfo.weaponInfo.curWeaponID        = SITHWEAPON_NO_WEAPON;
            pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID = -1;
        }
    }
    else if ( pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID != -1 )
    {
        if ( pThing->type == SITH_THING_PLAYER && sithTime_secGameTime < sithWeapon_secMountWait )
        {
            return;
        }

        if ( sithTime_secGameTime < pThing->thingInfo.actorInfo.weaponInfo.secAimWaitEndTime )
        {
            return;
        }

        pItem = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID);
        if ( (pItem->flags & SITHINVENTORY_TYPE_WEAPON) != 0 && pItem->pCog )
        {
            pThing->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired = -1.0f;
            sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_SELECTED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }

        if ( pThing->type == SITH_THING_PLAYER )
        {
            sithInventory_SetCurrentWeapon(pThing, (SithWeaponId)pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID);
        }

        pThing->thingInfo.actorInfo.weaponInfo.curWeaponID = pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID;
        pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID = -1;
    }
    else
    {
        if ( pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime > 0.0f )
        {
            if ( sithTime_secGameTime < pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime )
            {
                return;
            }

            pItem = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
            if ( pItem->pCog )
            {
                sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_FIRE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
                pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = sithTime_secGameTime
                    + pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime;
            }

            if ( pThing->type == SITH_THING_PLAYER && sithWeapon_bPlayerWeaponActivated != 1 )
            {
                sithWeapon_bPlayerWeaponActivated = 1;
                sithWeapon_DeactivateCurrentWeapon(pThing);
            }
        }
        else
        {
            if ( pThing->type == SITH_THING_PLAYER && pThing->thingInfo.actorInfo.weaponInfo.curWeaponID == SITHWEAPON_WHIP )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
                    && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_WHIPAIM) != 0
                    || (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 && (pThing->attach.pFace->flags & RD_FF_3DO_WHIP_AIM) != 0 )
                {
                    sithWhip_UpdateWhipAim(pThing);
                }
                else if ( sithWeapon_bLocalPlayerAiming )
                {
                    sithWeapon_SendMessageAim(pThing, 0);
                }
            }
        }
    }
}

void J3DAPI sithWeapon_Update(SithThing* pThing, float secDeltaTime)
{
    SithWeaponInfo* pWeaponInfo;

#line 391
    SITH_ASSERTREL(pThing);
    pWeaponInfo = &pThing->thingInfo.weaponInfo;

    if ( (pWeaponInfo->flags & SITH_WF_INSTANTIMPACT) != 0 )
    {
        sithWeapon_HandleImpact(pThing);
        return;
    }

    if ( (pWeaponInfo->flags & SITH_WF_DAMAGEDECAY) != 0 && pWeaponInfo->damage > pWeaponInfo->minDamage )
    {
        pWeaponInfo->damage -= pWeaponInfo->rate * secDeltaTime;
        pWeaponInfo->damage = J3DMAX(pWeaponInfo->damage, pWeaponInfo->minDamage);
    }

    if ( (pWeaponInfo->flags & SITH_WF_DECAYEMITSOUNDAWARENESSEVENT) != 0 && SITH_ISFRAMECYCLE(pThing->idx, 8) ) // on every 8th frame
    {
        sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 2, 0.5f, pThing);
    }
}

int J3DAPI sithWeapon_SelectWeapon(SithThing* pThing, SithWeaponId typeId)
{
    int bHasWeapon;

    if ( pThing->type == SITH_THING_PLAYER )
    {
        pThing->thingInfo.actorInfo.weaponInfo.curWeaponID = sithInventory_GetCurrentWeapon(pThing);
    }

#line 434
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR)));

    if ( typeId != pThing->thingInfo.actorInfo.weaponInfo.curWeaponID && pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID == -1 )
    {
        if ( pThing->type == SITH_THING_PLAYER )
        {
            bHasWeapon = sithInventory_GetInventory(pThing, typeId) != 0.0 && sithInventory_IsInventoryAvailable(pThing, typeId);
        }
        else
        {
            bHasWeapon = 1;
            sithPlayerControls_CenterActorOrientation(pThing);
        }

        if ( bHasWeapon )
        {
            if ( pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationStartTime != -1.0f && pThing->type == SITH_THING_PLAYER )
            {
                sithWeapon_deactivatedPlayerWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
                sithWeapon_secPlayerWeaponDeactivationTime = pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime;
            }

            pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
            pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID   = typeId;
            return 1;
        }
    }

    return 0;
}


void J3DAPI sithWeapon_HandleImpact(SithThing* pWeapon)
{
    rdVector3 moveNorm;
    SithWeaponInfo* pWeaponInfo = &pWeapon->thingInfo.weaponInfo;

    SithCollision* pCollision;
    float damage = pWeaponInfo->damage;

    int searchFlags = 0;

    rdVector_Copy3(&moveNorm, &pWeapon->orient.lvec);

    if ( (pWeaponInfo->damageType & SITH_DAMAGE_MACHETE) != 0 )
    {
        searchFlags = 0x200;
    }

    sithCollision_SearchForCollisions(pWeapon->pInSector, pWeapon, &pWeapon->pos, &moveNorm, pWeaponInfo->range, pWeapon->collide.movesize, searchFlags);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pWeaponInfo->flags & SITH_WF_DAMAGEDECAY) != 0 )
        {
            damage -= RD_MULTIPLY(pWeaponInfo->rate, pCollision->distance);
            damage = J3DMAX(pWeaponInfo->minDamage, damage);
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            SithThing* pVictim = pCollision->pThingCollided;
            sithThing_DamageThing(pVictim, pWeapon, damage, pWeaponInfo->damageType);

            if ( pVictim->type == SITH_THING_ACTOR )
            {
                // Call actor killed cb if actor is killed
                if ( sithThing_GetThingParent(pWeapon) == sithPlayer_pLocalPlayerThing
                    && sithWeapon_pfActorKilledCallback
                    && pVictim->thingInfo.actorInfo.health == 0.0f )
                {
                    sithWeapon_pfActorKilledCallback(sithGetGameDifficulty());
                }

                // Generate bloodsplort anf destroy projectile or explode projectile
                if ( pVictim->thingInfo.actorInfo.health == 0.0f
                    && (pWeaponInfo->flags & SITH_WF_ACTORKILLDESTROY) != 0
                    && (pVictim->thingInfo.actorInfo.flags & SITH_AF_DROID) == 0
                    && pVictim->type != SITH_THING_PLAYER
                    && sithWeapon_bGenBloodsplort == 1 )
                {
                    sithWeapon_GenBloodsplort(pVictim);
                    sithThing_DestroyThing(pWeapon);
                }
                else {
                    sithWeapon_CreateWeaponExplosion(pWeapon, pWeaponInfo->pExplosionTemplate, NULL, /*bRotate=*/0);
                }
            }
            else {
                sithWeapon_CreateWeaponExplosion(pWeapon, pWeaponInfo->pExplosionTemplate, NULL, /*bRotate=*/0);
            }

            if ( pWeaponInfo->force != 0.0f && pVictim->moveType == SITH_MT_PHYSICS )
            {
                rdVector3 force;
                rdVector_Scale3(&force, &moveNorm, pWeaponInfo->force);
                sithPhysics_ApplyForce(pVictim, &force);
            }

            break;
        }

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            sithWeapon_CreateWeaponExplosion(pWeapon, pWeaponInfo->pExplosionTemplate, NULL, /*bRotate=*/0);
            sithSurface_HandleThingImpact(pCollision->pSurfaceCollided, pWeapon, damage, pWeaponInfo->damageType);
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( (pWeapon->flags & SITH_TF_COGLINKED) != 0 && (pWeapon->flags & SITH_TF_REMOTE) == 0 )
    {
        sithCog_ThingSendMessage(pWeapon, NULL, SITHCOG_MSG_REMOVED);
    }

    sithThing_RemoveThing(sithWorld_pCurrentWorld, pWeapon);
}

int J3DAPI sithWeapon_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    int flags;

    switch ( adjNum )
    {
        case SITHTHING_ARG_EXPLODE:
            pThing->thingInfo.weaponInfo.pExplosionTemplate = sithTemplate_GetTemplate(pArg->argValue);
            break;

        case SITHTHING_ARG_DAMAGE:
        {
            pThing->thingInfo.weaponInfo.damage = D3DVAL(atof(pArg->argValue));
            break;
        }
        case SITHTHING_ARG_MINDAMAGE:
        {
            pThing->thingInfo.weaponInfo.minDamage = D3DVAL(atof(pArg->argValue));
            break;
        }
        case SITHTHING_ARG_TYPEFLAGS:
        {
            if ( sscanf(pArg->argValue, "%x", &flags) != 1 )
            {
                goto error;
            }

            pThing->thingInfo.weaponInfo.flags = flags;
            break;
        }
        case SITHTHING_ARG_DAMAGECLASS:
        {
            if ( sscanf(pArg->argValue, "%x", &flags) != 1 )
            {
                goto error;
            }

            pThing->thingInfo.weaponInfo.damageType = flags;
            break;
        }
        case SITHTHING_ARG_RANGE:
        {
            pThing->thingInfo.weaponInfo.range = D3DVAL(atof(pArg->argValue));
            break;
        }
        case SITHTHING_ARG_RATE:
        {
            pThing->thingInfo.weaponInfo.rate = D3DVAL(atof(pArg->argValue));
            break;
        }
        case SITHTHING_ARG_FORCE:
        {
            pThing->thingInfo.weaponInfo.force = D3DVAL(atof(pArg->argValue));
            break;
        }
        default:
            return 0;
    }

    return 1;

error:
#line 661
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad argument pair %s=%s line %d.  Skipped.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber(), 0);
    return 1;
}


SithThing* J3DAPI sithWeapon_WeaponFire(SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* pFireDir, rdVector3* pFirePos, tSoundHandle hFireSnd, SithPuppetSubMode submode, float extra, SithFireProjectileFlags projectileFlags, float secDeltaTime)
{
    SithThing* pProjectile;

#line 689
    SITH_ASSERTREL(pShooter);

    if ( hFireSnd )
    {
        sithAIAwareness_CreateTransmittingEvent(pShooter->pInSector, &pShooter->pos, 0, 3.0f, pShooter);
    }

    if ( pShooter->type == SITH_THING_PLAYER && (pProjectileTemplate->thingInfo.weaponInfo.damageType & (SITH_DAMAGE_ELECTROWHIP | SITH_DAMAGE_WHIP | SITH_DAMAGE_MACHETE | SITH_DAMAGE_FISTS)) == 0 )// 0x838 - SITH_DAMAGE_ELECTROWHIP | SITH_DAMAGE_WHIP | SITH_DAMAGE_MACHETE | SITH_DAMAGE_FISTS
    {
        sithAIAwareness_CreateTransmittingEvent(pShooter->pInSector, &pShooter->pos, 3, 1.5f, pShooter);
    }

    pProjectile = sithWeapon_WeaponFireProjectile(pShooter, pProjectileTemplate, pFireDir, pFirePos, hFireSnd, submode, extra, projectileFlags, secDeltaTime);
    if ( pProjectile && sithMessage_outputstream )
    {
        sithDSSThing_Fire(pShooter, pProjectileTemplate, pFireDir, pFirePos, hFireSnd, submode, extra, projectileFlags, secDeltaTime, pProjectile->guid, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }

    return pProjectile;
}

SithThing* J3DAPI sithWeapon_WeaponFireProjectile(SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* pFireDir, rdVector3* pFirePos, tSoundHandle hFireSnd, SithPuppetSubMode submode, float extra, SithFireProjectileFlags flags, float secDeltaTime)
{
    SithThing* pProjectile = NULL;

    if ( !pShooter || !pFireDir )
    {
#line 746
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad arguments passed.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( !pProjectileTemplate )
    {
    }
    else
    {
        rdVector3 offsetPos;
        SithSector* pSector;
        rdVector3 curShooterOffset;
        rdVector3 moveDir;
        float moveDist;
        float curSpeed;
        SithCollision* pCollision;

        rdMatrix34 projectileOrient;
        rdMatrix_BuildFromLook34(&projectileOrient, pFireDir);

        if ( (pShooter->thingInfo.actorInfo.fireOffset.x != 0.0f
            || pShooter->thingInfo.actorInfo.fireOffset.y != 0.0f
            || pShooter->thingInfo.actorInfo.fireOffset.z != 0.0f)
            && pShooter->controlType != SITH_CT_AI )
        {
            rdVector3 fireOffset;
            rdVector_Copy3(&fireOffset, &pShooter->thingInfo.actorInfo.fireOffset);
            if ( (flags & SITHFIREPROJECTILE_RIGHTHAND_FIRE) != 0 )
            {
                int jointIdx = sithThing_GetThingJointIndex(pShooter, "inrhand");
                if ( jointIdx != -1 )
                {
                    rdMatrix_TransformVector34Acc(&fireOffset, &pShooter->renderData.paJointMatrices[jointIdx]);
                }
            }

            else if ( (flags & SITHFIREPROJECTILE_TORSO_FIRE) != 0 )
            {
                int jointIdx = sithThing_GetThingJointIndex(pShooter, "intorso");
                if ( jointIdx != -1 )
                {
                    rdMatrix_TransformVector34Acc(&fireOffset, &pShooter->renderData.paJointMatrices[jointIdx]);
                }
            }
            else
            {
                rdMatrix_TransformVector34Acc(&fireOffset, &pShooter->orient);
            }

            rdVector_Add3Acc(pFirePos, &fireOffset);
        }

        
        rdVector_Copy3(&offsetPos, &pShooter->pos);

        // Find offset sector
        
        curShooterOffset.x = 0.022f;
        curShooterOffset.y = 0.0f;
        curShooterOffset.z = 0.05f;
        rdMatrix_TransformVector34Acc(&curShooterOffset, &pShooter->orient);

        rdVector_Add3Acc(&offsetPos, &curShooterOffset);

        pSector = sithCollision_FindSectorInRadius(pShooter->pInSector, &pShooter->pos, &offsetPos, 0.0f);
        if ( !pSector )
        {
#line 796
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Uh-oh.  Failed to find offset sector.  Weapon fire failed.\n", 0, 0, 0, 0);
            return NULL;
        }

        // Find Fire point sector
        pSector = sithCollision_FindSectorAtThing(pShooter, pSector, &offsetPos, pFirePos, 0.0f);
        if ( !pSector )
        {
#line 804
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Uh-oh.  Failed to find fire point sector.  Weapon fire failed.\n", 0, 0, 0, 0);
            return NULL;
        }

        pProjectile = sithThing_CreateThingAtPos(pProjectileTemplate, pFirePos, &projectileOrient, pSector, pShooter);
        if ( !pProjectile )
        {
            return NULL;
        }

        if ( pProjectileTemplate->pCog )
        {
            sithCog_SendMessage(pProjectileTemplate->pCog, SITHCOG_MSG_FIRE, SITHCOG_SYM_REF_THING, pProjectile->idx, SITHCOG_SYM_REF_THING, pShooter->idx, 0);
        }

        if ( (flags & SITHFIREPROJECTILE_SCALE_VELOCITY) != 0 )
        {
            rdVector_Scale3Acc(&pProjectile->moveInfo.physics.velocity, extra);
        }

        if ( (flags & SITHFIREPROJECTILE_SCALE_DAMAGE) != 0 )
        {
            pProjectile->thingInfo.weaponInfo.damage = RD_MULTIPLY(pProjectile->thingInfo.weaponInfo.damage, extra);
        }

        if ( (flags & SITHFIREPROJECTILE_UNKNOWN_4) != 0 )
        {
            pProjectile->thingInfo.weaponInfo.unknown8 = pProjectile->thingInfo.weaponInfo.unknown8 * extra;
        }

        if ( (flags & SITHFIREPROJECTILE_UNKNOWN_8) != 0 )
        {
            pProjectile->thingInfo.weaponInfo.unknown7 = pProjectile->thingInfo.weaponInfo.unknown7 * extra;
        }

        
        if ( secDeltaTime > 0.02 )
        {
            SithPhysicsInfo* pPhysics;

            sithPhysics_UpdateThing(pProjectile, secDeltaTime);
            pPhysics = &pProjectile->moveInfo.physics;
            moveDist = rdVector_Normalize3(&moveDir, &pPhysics->deltaVelocity);
            if ( moveDist > 0.0f )
            {
                sithCollision_MoveThing(pProjectile, &moveDir, moveDist, pPhysics->flags);// // TODO: [BUG] Using physics.flags here is probably wrong???
            }
        }

        // Now search for projectile target
        curSpeed = rdVector_Normalize3(&moveDir, &pProjectile->moveInfo.physics.velocity);
        moveDist = J3DMIN(curSpeed * 3.0f, 5.0f);

        sithCollision_SearchForCollisions(pProjectile->pInSector, pProjectile, &pProjectile->pos, &moveDir, moveDist, pProjectile->collide.movesize, 2);
        pCollision = sithCollision_PopStack();
        sithCollision_DecreaseStackLevel();

        if ( pCollision && (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( !stdComm_IsGameActive()
                && pShooter == sithPlayer_pLocalPlayerThing
                && (pProjectile->thingInfo.weaponInfo.flags & SITH_WF_EMITAITARGETEDEVENT) != 0
                && pCollision->pThingCollided->controlType == SITH_CT_AI )
            {
                sithAI_EmitEvent(pCollision->pThingCollided->controlInfo.aiControl.pLocal, SITHAI_EVENT_TARGETED, pProjectile);
            }
        }
        else
        {
            sithCog_ThingSendMessageEx(pShooter, NULL, SITHCOG_MSG_MISSED, (int32_t)pProjectile->thingInfo.weaponInfo.damage, pProjectile->thingInfo.weaponInfo.damageType, 0, 0);
        }
    }

    if ( (flags & SITHFIREPROJECTILE_FIRE_EFFECT) != 0 )
    {
        sithWeapon_CreateWeaponFireFx(pShooter, pFirePos);
    }

    if ( hFireSnd )
    {
        sithSoundMixer_PlaySoundThing(hFireSnd, pShooter, 1.0f, 1.0f, 10.0f, SOUNDPLAY_HIGH_PRIORITY | SOUNDPLAY_THING_POS);
    }

    if ( submode > 0 && pShooter->pPuppetClass )
    {
        sithPuppet_PlayMode(pShooter, submode, NULL);
    }

    return pProjectile;
}

void J3DAPI sithWeapon_DamageWeapon(SithThing* pThing, const SithThing* pPurpetrator, float damage)
{
    if ( (pThing->thingInfo.weaponInfo.flags & SITH_WF_DAMAGEDESTROY) != 0 && damage > 1.0f && (!pThing->msecLifeLeft || pThing->msecLifeLeft > 250) )
    {
        pThing->msecLifeLeft = 250;
    }
}

int J3DAPI sithWeapon_ThingCollisionHandler(SithThing* pWeapon, SithThing* pThing, SithCollision* pCollision, int bSecondThingIsSource)
{
    float damage;

    if ( (pThing->flags & SITH_TF_NOWEAPONCOLLIDE) != 0 )
    {
        return 0;
    }

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_MOPHIABOMB) != 0 && pThing->type == SITH_THING_PLAYER )
    {
        int bNoProjectile = 1;

        if ( pThing->thingInfo.actorInfo.weaponInfo.curWeaponID == SITHWEAPON_MIRROR )
        {
            SithThing* pTemplate = sithTemplate_GetTemplate("+mardukhit");
            static float sithWeapon_mirrorReboundSpeedScale = 0.75f;
            if ( pTemplate )
            {
                SithThing* pReboundProjectile;
                tSoundHandle hSnd;
                float wepspeed;
                rdVector3 projectDir;

                if ( sithWeapon_IsAiming(pThing) )
                {
                    pReboundProjectile = sithThing_CreateThing(pTemplate, pWeapon);
                    if ( pReboundProjectile )
                    {
                        wepspeed = sqrtf(RD_MULTIPLY(pWeapon->moveInfo.physics.velocity.x, pWeapon->moveInfo.physics.velocity.x) + RD_MULTIPLY(pWeapon->moveInfo.physics.velocity.y, pWeapon->moveInfo.physics.velocity.y) + RD_MULTIPLY(pWeapon->moveInfo.physics.velocity.z, pWeapon->moveInfo.physics.velocity.z)); /* spelling-fit */
                        rdVector_Sub3(&projectDir, &pWeapon->pParent->pos, &pWeapon->pos);
                        rdVector_Normalize3Acc(&projectDir);

                        // Set projectile LVect
                        rdVector_Copy3(&pReboundProjectile->orient.lvec, &projectDir);

                        // Set projectile RVect
                        rdVector_Cross3(&pReboundProjectile->orient.rvec, &pReboundProjectile->orient.lvec, &pReboundProjectile->orient.uvec);
                        rdVector_Normalize3Acc(&pReboundProjectile->orient.rvec);

                        // Set projectile UVect
                        rdVector_Cross3(&pReboundProjectile->orient.uvec, &pReboundProjectile->orient.lvec, &pReboundProjectile->orient.rvec);
                        rdVector_Normalize3Acc(&pReboundProjectile->orient.uvec);

                        rdVector_Scale3(&pReboundProjectile->moveInfo.physics.velocity, &projectDir, wepspeed * sithWeapon_mirrorReboundSpeedScale);
                        sithCollision_MoveThing(pReboundProjectile, &projectDir, 0.06f, 5);

                        bNoProjectile = 0;
                    }
                }
                else
                {
                    rdVector3 projectDir;
                    float dot;
                    static float sithWeapon_mirrorReboundMinDot = -0.737f;

                    rdVector3 wepdir;
                    float wepspeed = rdVector_Normalize3(&wepdir, &pWeapon->moveInfo.physics.velocity);

                    rdVector3 thdir;
                    static float sithWeapon_mirrorAimUpBias = 0.04f;
                    static float sithWeapon_mirrorReboundDeflectScale = 2.0f;
                    rdVector_Copy3(&thdir, &pThing->orient.lvec);

                    thdir.z = thdir.z + sithWeapon_mirrorAimUpBias;
                    rdVector_Normalize3Acc(&thdir);

                    dot = rdVector_Dot3(&thdir, &wepdir);
                    if ( dot <= sithWeapon_mirrorReboundMinDot )
                    {
                        pReboundProjectile = sithThing_CreateThing(pTemplate, pWeapon);
                        if ( pReboundProjectile )
                        {

                            float dist = sithWeapon_mirrorReboundDeflectScale * dot < 0.0f ? -(sithWeapon_mirrorReboundDeflectScale * dot) : sithWeapon_mirrorReboundDeflectScale * dot;

                            rdVector_ScaleAdd3(&projectDir, &thdir, dist, &wepdir);
                            rdVector_Normalize3Acc(&projectDir);

                            rdVector_Copy3(&pReboundProjectile->orient.lvec, &projectDir);

                            rdVector_Cross3(&pReboundProjectile->orient.rvec, &pReboundProjectile->orient.lvec, &pReboundProjectile->orient.uvec);
                            rdVector_Normalize3Acc(&pReboundProjectile->orient.rvec);

                            rdVector_Cross3(&pReboundProjectile->orient.uvec, &pReboundProjectile->orient.lvec, &pReboundProjectile->orient.rvec);
                            rdVector_Normalize3Acc(&pReboundProjectile->orient.uvec);

                            rdVector_Scale3(&pReboundProjectile->moveInfo.physics.velocity, &projectDir, RD_MULTIPLY(wepspeed, sithWeapon_mirrorReboundSpeedScale));
                            sithCollision_MoveThing(pReboundProjectile, &pThing->orient.lvec, 0.06f, 5);

                            bNoProjectile = 0;
                            sithThing_DamageThing(pThing, pWeapon, 25.0f, SITH_DAMAGE_LIGHTNING);
                        }
                    }
                }
                if ( !bNoProjectile )
                {
                    pReboundProjectile->pParent = pThing;
                    pReboundProjectile->parentSignature = pThing->signature;
                    hSnd = sithSound_Load(sithWorld_pCurrentWorld, "aet_mr_ball_rebound.wav");
                    if ( hSnd )
                    {
                        sithSoundMixer_PlaySoundThing(hSnd, pReboundProjectile, 1.0f, 0.5f, 2.0f, SOUNDPLAY_PLAYTHINGONCE | SOUNDPLAY_THING_POS);
                    }

                    memset(&pWeapon->moveDir, 0, sizeof(pWeapon->moveDir));
                    sithThing_DestroyThing(pWeapon);
                    return 1;
                }
            }
        }

        if ( bNoProjectile && sithGetPerformanceLevel() >= 2 )
        {
            tSoundHandle hSnd = sithSound_Load(sithWorld_pCurrentWorld, "aet_mr_hit_indy.wav");
            if ( hSnd )
            {
                sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, 0.5f, 2.0f, SOUNDPLAY_PLAYTHINGONCE | SOUNDPLAY_THING_POS);
            }
        }
    }

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_PROXIMITY) != 0 )
    {
        pWeapon->thingInfo.weaponInfo.flags &= ~SITH_WF_PROXIMITY;

        pWeapon->thingInfo.weaponInfo.flags |= SITH_WF_EXPLODE;

        pWeapon->collide.size = 0.0f;
        pWeapon->msecLifeLeft = 500;
        sithSoundClass_PlayModeFirst(pWeapon, SITHSOUNDCLASS_ACTIVATE);
        return 0;
    }

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_SURFACERICOCHET) != 0 && pThing->type == SITH_THING_COG )
    {
        if ( pWeapon->thingInfo.weaponInfo.numRicochets++ < SITHWEAPON_MAX_PROJECTILE_RICOCHETS )
        {
            sithSoundClass_PlayModeRandom(pWeapon, SITHSOUNDCLASS_DEFLECTED);
        }
        else
        {
            pWeapon->moveInfo.physics.flags &= ~SITH_PF_SURFACEBOUNCE;
            pWeapon->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
        }
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        if ( pWeapon->thingInfo.weaponInfo.damage != 0.0f )
        {
            sithThing_DamageThing(pThing, pWeapon, pWeapon->thingInfo.weaponInfo.damage, pWeapon->thingInfo.weaponInfo.damageType);
        }

        if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_FACEHITEXPLODE) != 0 )
        {
            rdVector3 expDir;

            if ( pCollision->pFaceCollided )
            {
                SithThing* pExplosion;

                rdMatrix_TransformVector34(&expDir, &pCollision->pFaceCollided->normal, &pThing->orient);
                pExplosion = sithWeapon_CreateWeaponExplosion(pWeapon, pWeapon->thingInfo.weaponInfo.pExplosionTemplate, &expDir, /*bRotate=*/1);
                if ( pExplosion && (pExplosion->thingInfo.weaponInfo.flags & SITH_WF_IMPACTSOUND) != 0 )
                {
                    pExplosion->thingInfo.explosionInfo.pHitSurfaceMat = pCollision->pFaceCollided->pMaterial;
                }
            }
            else
            {
                expDir.x = 1.0f;
                expDir.y = 0.0f;
                expDir.z = 0.0f;
                sithWeapon_CreateWeaponExplosion(pWeapon, pWeapon->thingInfo.weaponInfo.pExplosionTemplate, &expDir, /*bRotate=*/0);
            }

            return 1;
        }

        if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_ATTACHFACE) != 0 )
        {
            sithPhysics_ResetThingMovement(pWeapon);
            sithSoundClass_StopMode(pWeapon, SITHSOUNDCLASS_CREATE);
            sithSoundClass_PlayModeFirst(pWeapon, SITHSOUNDCLASS_HITHARD);
            pWeapon->collide.movesize = 0.0f;
            if ( pCollision->pFaceCollided && pCollision->pMeshCollided )
            {
                sithThing_AttachThingToThing(pWeapon, pThing);
                sithPhysics_SetThingLook(pWeapon, &pCollision->hitNorm, 0.0f);
                pWeapon->attach.flags |= SITH_ATTACH_NOMOVE;
                pWeapon->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
            }
            else
            {
                sithThing_AttachThingToThing(pWeapon, pThing);
                sithPhysics_SetThingLook(pWeapon, &pCollision->hitNorm, 0.0f);
                pWeapon->attach.flags |= SITH_ATTACH_NOMOVE;

                pWeapon->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
            }

            return 1;
        }

        // Fallback to default collision handler
        return sithCollision_ThingCollisionHandler(pWeapon, pThing, pCollision, bSecondThingIsSource);
    }

    if ( pWeapon->thingInfo.weaponInfo.damage == 0.0f && (pWeapon->thingInfo.weaponInfo.flags & (SITH_WF_ATTACHTHING | SITH_WF_THINGHITEXPLODE)) == 0 )
    {
        return 0;
    }

    if ( !sithCollision_ThingCollisionHandler(pWeapon, pThing, pCollision, bSecondThingIsSource) )
    {
        return 0;
    }

    damage = sithThing_ScaleCombatDamage(pWeapon, pThing);
    if ( damage != 0.0f )
    {
        sithThing_DamageThing(pThing, pWeapon, damage, pWeapon->thingInfo.weaponInfo.damageType);
    }

    if ( sithThing_GetThingParent(pWeapon) == sithPlayer_pLocalPlayerThing
        && sithWeapon_pfActorKilledCallback
        && pThing->thingInfo.actorInfo.health == 0.0f )
    {
        sithWeapon_pfActorKilledCallback(sithGetGameDifficulty());
    }

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_THINGHITEXPLODE) != 0 )
    {
        if ( pThing->thingInfo.actorInfo.health == 0.0f
            && (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_ACTORKILLDESTROY) != 0
            && (pThing->thingInfo.actorInfo.flags & SITH_AF_DROID) == 0
            && pThing->type != SITH_THING_PLAYER
            && sithWeapon_bGenBloodsplort == 1 )
        {
            sithWeapon_GenBloodsplort(pThing);
            sithThing_DestroyThing(pWeapon);
        }
        else
        {
            sithWeapon_CreateWeaponExplosion(pWeapon, pWeapon->thingInfo.weaponInfo.pExplosionTemplate, NULL, /*bRotate=*/0);
        }

        return 1;
    }

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_ATTACHTHING) != 0 )
    {
        sithPhysics_ResetThingMovement(pWeapon);
        sithThing_AttachThingToThing(pWeapon, pThing);
        pWeapon->attach.flags |= SITH_ATTACH_NOMOVE;
        pWeapon->moveInfo.physics.flags |= SITH_PF_USEGRAVITY;
    }

    return 1;
}

int J3DAPI sithWeapon_SurfaceCollisionHandler(SithThing* pThing, SithSurface* pSurf, SithCollision* pStack)
{
    SithThing* pExplosion;
    rdVector3 surfNormal;

#line 1257
    SITH_ASSERTREL(pThing->type == SITH_THING_WEAPON);

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Non-physics weapon %s hit a wall.\n", pThing->aName, 0, 0, 0);
        return 0;
    }

    // Print debug info
    if ( (sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
    {
        const char* pMatName;

        if ( pSurf->face.pMaterial )
        {
            pMatName = pSurf->face.pMaterial->aName;
        }
        else
        {
            pMatName = "none";
        }

        sprintf(std_genBuffer, "Weapon hit surface %d, sector %d, material '%s'.\n", sithSurface_GetSurfaceIndex(pSurf), sithSector_GetSectorIndex(pSurf->pSector), pMatName);
        sithConsole_PrintString(std_genBuffer);
    }

    // If hit sky surface, destroy projectile
    if ( (pSurf->flags & (SITH_SURFACE_CEILINGSKY | SITH_SURFACE_HORIZONSKY)) != 0 )
    {
        sithThing_DestroyThing(pThing);
        return 1;
    }

    if ( (pThing->thingInfo.weaponInfo.flags & SITH_WF_SURFACERICOCHET) != 0 )
    {
        if ( pThing->thingInfo.weaponInfo.numRicochets++ < SITHWEAPON_MAX_PROJECTILE_RICOCHETS )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEFLECTED);
        }
        else
        {
            pThing->moveInfo.physics.flags &= ~SITH_PF_SURFACEBOUNCE;
            pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
        }
    }

    if ( pThing->thingInfo.weaponInfo.damage != 0.0f )
    {
        sithSurface_HandleThingImpact(pSurf, pThing, pThing->thingInfo.weaponInfo.damage, pThing->thingInfo.weaponInfo.damageType);
    }

    if ( (pThing->thingInfo.weaponInfo.flags & SITH_WF_FACEHITEXPLODE) != 0 )
    {
        rdVector_Copy3(&surfNormal, &pSurf->face.normal);
        pExplosion = sithWeapon_CreateWeaponExplosion(pThing, pThing->thingInfo.weaponInfo.pExplosionTemplate, &surfNormal, 1);
        if ( pExplosion && (pExplosion->thingInfo.explosionInfo.flags & SITH_EF_UPDATEDEBRISMATERIAL) != 0 )
        {
            pExplosion->thingInfo.explosionInfo.pHitSurfaceMat = pSurf->face.pMaterial;
        }

        return 1;
    }

    if ( (pThing->thingInfo.weaponInfo.flags & SITH_WF_ATTACHFACE) != 0 )
    {
        sithCollision_HandleThingHitSurface(pThing, pSurf, pStack);
        sithPhysics_ResetThingMovement(pThing);
        pThing->collide.movesize = 0.0f;

        sithThing_AttachThingToSurface(pThing, pSurf, 0);
        sithPhysics_SetThingLook(pThing, &pSurf->face.normal, 0.0f);
        pThing->moveInfo.physics.flags |= SITH_PF_NOTHRUST;
        return 1;
    }

    // Tallback to default collision handler
    return sithCollision_HandleThingHitSurface(pThing, pSurf, pStack);
}

void J3DAPI sithWeapon_DestroyWeapon(SithThing* pWeapon)
{
#line 1383
    SITH_ASSERTREL(pWeapon && (pWeapon->type == SITH_THING_WEAPON));

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_EXPLODE) != 0 )
    {
        SithThing* pExplosion;

        rdMaterial* pMaterial = NULL;
        if ( (pWeapon->attach.flags & SITH_ATTACH_SURFACE) != 0 )
        {
            pMaterial = pWeapon->attach.attachedToStructure.pSurfaceAttached->face.pMaterial;
        }

        pExplosion = sithWeapon_CreateWeaponExplosion(pWeapon, pWeapon->thingInfo.weaponInfo.pExplosionTemplate, 0, 0);
        if ( pExplosion && pMaterial && (pExplosion->thingInfo.explosionInfo.flags & SITH_EF_UPDATEDEBRISMATERIAL) != 0 )
        {
            pExplosion->thingInfo.explosionInfo.pHitSurfaceMat = pMaterial;
        }
    }
    else
    {
        sithThing_DestroyThing(pWeapon);
    }
}

SithThing* J3DAPI sithWeapon_CreateWeaponExplosion(SithThing* pWeapon, SithThing* pExplosionTemplate, const rdVector3* pDir, int bRotate)
{
    rdVector3 curpos;
    SithInventoryType* pItem;
    SithThing* currentParent;
    SithThing* pExplosion;
    rdMatrix34 orient;

#line 1432
    SITH_ASSERTREL(pWeapon->type == SITH_THING_WEAPON);
    SITH_ASSERTREL(pWeapon->moveType == SITH_MT_PHYSICS);

    if ( (pWeapon->thingInfo.weaponInfo.flags & SITH_WF_EXPLODE) != 0
        && (pWeapon->flags & SITH_TF_SUBMERGED) != 0
        && (pWeapon->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        pExplosionTemplate = sithTemplate_GetTemplate("+uw_blast");
    }

    if ( !pExplosionTemplate )
    {
        sithThing_DestroyThing(pWeapon);
        return NULL;
    }

    
    rdVector_Copy3(&curpos, &pWeapon->pos);
    rdMatrix_Copy34(&orient, &rdroid_identMatrix34);

    if ( bRotate )
    {
        rdVector3 offset;
        rdVector_Copy3(&offset, pDir);
        rdVector_Scale3Acc(&offset, 0.001f); // offset /1000

        rdVector_Add3Acc(&curpos, &offset);

        // SetUVec & LVec
        rdVector_Copy3(&orient.uvec, &rdroid_zVector3);
        rdVector_Copy3(&orient.lvec, pDir);

        // Calc RVect
        rdVector_Cross3(&orient.rvec, &orient.lvec, &orient.uvec);
    }

    currentParent = sithThing_GetThingParent(pWeapon);
    pExplosion = sithThing_CreateThingAtPos(pExplosionTemplate, &curpos, &orient, pWeapon->pInSector, currentParent);
    if ( !pExplosion )
    {
#line 1470
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No thing space to create explosion from weapon %d.\n", pWeapon->idx, 0, 0, 0);
        sithThing_DestroyThing(pWeapon);
        return NULL;
    }

    pItem = sithInventory_GetType(currentParent->thingInfo.actorInfo.weaponInfo.curWeaponID);
    if ( pItem->pCog ) {
        sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_CREATED, SITHCOG_SYM_REF_THING, pExplosion->idx, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    if ( currentParent == sithPlayer_pLocalPlayerThing ) {
        sithAIAwareness_CreateTransmittingEvent(pExplosion->pInSector, &pExplosion->pos, 0, 1.0f, currentParent);
    }

    if ( (pWeapon->flags & SITH_TF_REMOTE) != 0 ) {
        pExplosion->flags |= SITH_TF_REMOTE;
    }

    sithThing_DestroyThing(pWeapon);
    return pExplosion;
}

float J3DAPI sithWeapon_GetMountWait(void)
{
    return sithWeapon_secMountWait;
}

void J3DAPI sithWeapon_SetMountWait(float secWait)
{
    sithWeapon_secMountWait = sithTime_secGameTime + secWait;
}

void J3DAPI sithWeapon_SetFireWait(SithThing* pThing, float waitTime)
{
#line 1526
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR)));
    if ( waitTime == -1.0f )
    {
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = -1.0f;
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime    = -1.0f;
    }
    else
    {
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = sithTime_secGameTime + waitTime;
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime    = waitTime;
    }
}

void J3DAPI sithWeapon_SetAimWait(SithThing* pThing, float waitTime)
{
#line 1542
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR)));
    pThing->thingInfo.actorInfo.weaponInfo.secAimWaitEndTime = sithTime_secGameTime + waitTime;
}

void J3DAPI sithWeapon_ActivateWeapon(SithThing* pThing, SithCog* pCog, float waitTime)
{

    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime = waitTime;
    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationStartTime = sithTime_secGameTime;
    if ( pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime <= 0.0f )
    {
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = -1.0f;
    }
    else
    {
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = sithTime_secGameTime + pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime;
    }
}

float J3DAPI sithWeapon_DeactivateWeapon(SithThing* pThing, SithCog* pCog)
{
    float waitTime;
    if ( pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationStartTime == -1.0f )
    {
        waitTime = 0.0f;
    }
    else
    {
        waitTime = sithTime_secGameTime - pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationStartTime;
    }

    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationStartTime   = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired          = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime = -1.0f;
    pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime    = 0.0f;
    return waitTime;
}

int J3DAPI sithWeapon_ProcessWeaponControls(SithThing* pThing, float secDeltaTime)
{
    SithWeaponId curWpnID;
    SithInventoryType* pItem;
    int keyMode;
    unsigned int idKey;
    int bMountingWpn;
    SithWeaponId curWeaponId;

#line 1637
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);

    if ( sithWeapon_IsLocalPlayerUnableToUseWeapon() )
    {
        sithWeapon_bufferedWaponKeyId = -1;
        return 0;
    }

    if ( sithPlayerControls_bCutsceneMode == 1 || (pThing->flags & SITH_TF_DYING) != 0 || (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
    {
        if ( sithWeapon_bPlayerWeaponActivated == 1 )
        {
            sithWeapon_DeactivateCurrentWeapon(pThing);
        }

        sithWeapon_bufferedWaponKeyId = -1;
        return 0;
    }

    curWpnID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    pItem = sithInventory_GetType(curWpnID);
    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 && (pItem->flags & SITHINVENTORY_TYPE_UNKNOWN_200) == 0 )
    {
        if ( sithWeapon_bPlayerWeaponActivated == 1 )
        {
            sithWeapon_DeactivateCurrentWeapon(pThing);
        }

        return 0;
    }

    bMountingWpn = sithWeapon_IsMountingWeapon(pThing);
    if ( bMountingWpn )
    {
        for ( idKey = SITHCONTROL_WSELECT0; idKey <= SITHCONTROL_WSELECT16; ++idKey )
        {
            sithControl_GetKey(idKey, &keyMode);
            if ( keyMode )
            {
                sithWeapon_bufferedWaponKeyId = idKey;
                return 0;
            }
        }

        if ( curWpnID > SITHWEAPON_NO_WEAPON
            && sithTime_secGameTime >= sithWeapon_secWeaponActivationWaitEndTime
            && (sithControl_GetKey(SITHCONTROL_ACT2, &keyMode), keyMode)
            && !sithWeapon_bPlayerWeaponActivated )
        {
            sithWeapon_secWeaponActivationWaitEndTime = sithTime_secGameTime + 0.5f;
            return 0;
        }


        sithControl_GetKey(SITHCONTROL_WEAPONTOGGLE, &keyMode);
        if ( keyMode )
        {
            sithWeapon_bufferedWaponKeyId = SITHCONTROL_WEAPONTOGGLE;
            return 0;
        }

        sithControl_GetKey(SITHCONTROL_LIGHTERTOGGLE, &keyMode);
        if ( keyMode )
        {
            sithWeapon_bufferedWaponKeyId = SITHCONTROL_LIGHTERTOGGLE;
            return 0;
        }

        sithControl_GetKey(SITHCONTROL_NEXTWEAPON, &keyMode);
        if ( keyMode )
        {
            sithWeapon_bufferedWaponKeyId = SITHCONTROL_NEXTWEAPON;
            return 0;
        }

        sithControl_GetKey(SITHCONTROL_PREVWEAPON, &keyMode);
        if ( keyMode )
        {
            sithWeapon_bufferedWaponKeyId = SITHCONTROL_PREVWEAPON;
            return 0;
        }

        return 0;
    }

    for ( idKey = SITHCONTROL_WSELECT0; idKey <= SITHCONTROL_WSELECT16; ++idKey )
    {
        sithControl_GetKey(idKey, &keyMode);
        if ( keyMode )
        {
            curWeaponId = sithWeapon_GetWeaponForControlKey(idKey);
            sithWeapon_SelectWeapon(pThing, curWeaponId);
            sithWeapon_bufferedWaponKeyId = -1;
            return 0;
        }
    }

    if ( curWpnID > SITHWEAPON_NO_WEAPON && sithTime_secGameTime >= sithWeapon_secWeaponActivationWaitEndTime )
    {
        if ( sithControl_GetKey(SITHCONTROL_ACT2, &keyMode) )
        {
            if ( !sithWeapon_bPlayerWeaponActivated )
            {
                if ( (pItem->flags & SITHINVENTORY_TYPE_UNKNOWN_200) != 0 )
                {
                    if ( pThing->moveStatus >= SITHPLAYERMOVE_STILL
                        && pThing->moveStatus <= SITHPLAYERMOVE_RUNNING
                        && pThing->thingInfo.actorInfo.bControlsDisabled != 1 )
                    {
                        sithPhysics_ResetThingMovement(pThing);
                    }
                    else
                    {
                        sithWeapon_bufferedWaponKeyId = SITHCONTROL_ACT2;
                        return 0;
                    }

                    if ( curWpnID == SITHWEAPON_WHIP && sithWeapon_ActivateWhip(pThing, pItem->pCog) == 1 )
                    {
                        sithWeapon_bLocalPlayerAiming = 0;
                        sithWeapon_deactivatedPlayerWeaponID = -1;
                        sithWeapon_bufferedWaponKeyId = -1;
                        return 0;
                    }
                }

                sithWeapon_bPlayerWeaponActivated = 1;
                if ( pItem->pCog )
                {
                    sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_ACTIVATE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
                }

                sithWeapon_bufferedWaponKeyId = -1;
                sithWeapon_secWeaponActivationWaitEndTime = pThing->thingInfo.actorInfo.weaponInfo.secAimWaitEndTime
                    + pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime;
            }
        }
        else if ( sithWeapon_bPlayerWeaponActivated == 1 )
        {
            sithWeapon_DeactivateCurrentWeapon(pThing);
        }
    }

    sithControl_GetKey(SITHCONTROL_WEAPONTOGGLE, &keyMode);
    if ( keyMode )
    {
        if ( !curWpnID )
        {
            if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
            {
                sithWeapon_SelectWeapon(pThing, SITHWEAPON_MACHETE);
            }
            else
            {
                sithWeapon_SelectWeapon(pThing, sithWeapon_lastPlayerWeaponID);
            }
        }
        else
        {
            sithWeapon_DeselectWeapon(pThing);
        }

        sithWeapon_bufferedWaponKeyId = -1;
    }

    sithControl_GetKey(SITHCONTROL_LIGHTERTOGGLE, &keyMode);
    if ( keyMode )
    {
        if ( curWpnID != SITHWEAPON_ZIPPO )
        {
            sithWeapon_SelectWeapon(pThing, SITHWEAPON_ZIPPO);
        }
        else
        {
            sithWeapon_DeselectWeapon(pThing);
        }

        sithWeapon_bufferedWaponKeyId = -1;
    }

    sithControl_GetKey(SITHCONTROL_NEXTWEAPON, &keyMode);
    if ( keyMode )
    {
        sithWeapon_SelectNextWeapon(pThing);
        sithWeapon_bufferedWaponKeyId = -1;
    }

    sithControl_GetKey(SITHCONTROL_PREVWEAPON, &keyMode);
    if ( keyMode )
    {
        sithWeapon_SelectPreviousWeapon(pThing);
        sithWeapon_bufferedWaponKeyId = -1;
    }

    if ( sithControl_GetKey(SITHCONTROL_CHALK, &keyMode) )
    {
        sithInventory_SetCurrentItem(pThing, SITHWEAPON_CHALK);
        sithPlayerActions_Activate(pThing);
        sithInventory_SetCurrentItem(pThing, SITHWEAPON_NO_WEAPON);
        sithWeapon_bufferedWaponKeyId = -1;
    }

    if ( sithWeapon_bufferedWaponKeyId != -1 && !sithWeapon_bPlayerWeaponActivated )
    {
        switch ( sithWeapon_bufferedWaponKeyId )
        {
            case SITHCONTROL_WSELECT0:
            case SITHCONTROL_WSELECT1:
            case SITHCONTROL_WSELECT2:
            case SITHCONTROL_WSELECT3:
            case SITHCONTROL_WSELECT4:
            case SITHCONTROL_WSELECT5:
            case SITHCONTROL_WSELECT6:
            case SITHCONTROL_WSELECT7:
            case SITHCONTROL_WSELECT8:
            case SITHCONTROL_WSELECT9:
            case SITHCONTROL_WSELECT10:
            case SITHCONTROL_WSELECT11:
            case SITHCONTROL_WSELECT12:
            case SITHCONTROL_WSELECT13:
            case SITHCONTROL_WSELECT14:
            case SITHCONTROL_WSELECT15:
            case SITHCONTROL_WSELECT16:
                curWeaponId = sithWeapon_GetWeaponForControlKey(sithWeapon_bufferedWaponKeyId);
                sithWeapon_SelectWeapon(pThing, curWeaponId);
                break;

            case SITHCONTROL_WEAPONTOGGLE:
                if ( curWpnID > SITHWEAPON_NO_WEAPON && curWpnID < SITHWEAPON_ZIPPO )
                {
                    sithWeapon_DeselectWeapon(pThing);
                }
                else
                {
                    sithWeapon_SelectWeapon(pThing, sithWeapon_lastPlayerWeaponID);
                }

                break;

            case SITHCONTROL_LIGHTERTOGGLE:
                if ( curWpnID != SITHWEAPON_ZIPPO )
                {
                    sithWeapon_SelectWeapon(pThing, SITHWEAPON_ZIPPO);
                }
                else
                {
                    sithWeapon_DeselectWeapon(pThing);
                }

                break;

            case SITHCONTROL_NEXTWEAPON:
                sithWeapon_SelectNextWeapon(pThing);
                break;

            case SITHCONTROL_PREVWEAPON:
                sithWeapon_SelectPreviousWeapon(pThing);
                break;

            default:
                break;
        }

#line 1962
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Processed buffered weapon key %d.\n", sithWeapon_bufferedWaponKeyId, 0, 0, 0);
        sithWeapon_bufferedWaponKeyId = -1;
        return 0;
    }

    // Original bug: the function falls off the end without a return value.
    // Retail's epilogue at this path leaves EAX holding whatever the last
    // comparison loaded, and emits no store or load of a result.
}

void J3DAPI sithWeapon_DeselectWeapon(SithThing* pThing)
{
    SithInventoryType* pItemInfo;
    int typeId = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( typeId )
    {
        pItemInfo = sithInventory_GetType(typeId);
        if ( (pItemInfo->flags & SITHINVENTORY_TYPE_WEAPON) != 0 )
        {
            pThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
            pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID   = SITHWEAPON_NO_WEAPON;
        }
    }
}

int J3DAPI sithWeapon_IsMountingWeapon(const SithThing* pThing)
{
    if ( pThing->type == SITH_THING_PLAYER && sithTime_secGameTime < sithWeapon_secMountWait )
    {
        return 1;
    }

    if ( pThing->thingInfo.actorInfo.weaponInfo.selectedWeaponID != -1 )
    {
        return 1;
    }

    if ( sithTime_secGameTime < pThing->thingInfo.actorInfo.weaponInfo.secAimWaitEndTime )
    {
        return 1;
    }

    if ( sithTime_secGameTime < pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithWeapon_HasWeaponSelected(const SithThing* pThing)
{
#line 2007
    SITH_ASSERTREL(pThing);
    if ( pThing->type == SITH_THING_PLAYER && J3DINRANGE(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID, SITHWEAPON_FISTS, SITHWEAPON_BAZOOKA) )
    {
        return 1;
    }

    if ( pThing->type == SITH_THING_ACTOR )
    {
        if ( (pThing->moveInfo.physics.flags & (SITH_PF_UNKNOWN_8000000 | SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
        {
            return 0;
        }

        if ( J3DINRANGE(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID, SITHWEAPON_COMTOKAREV, SITHWEAPON_COMSHOTGUN) )
        {
            return 1;
        }
    }

    return 0;
}


int J3DAPI sithWeapon_GetAimOrient(rdMatrix34* pOutOrient, SithThing* pShooter, const rdMatrix34* pStartOrient, const rdVector3* pFireOffset, float autoAimFovX, float autoAimFovZ)
{
    int curWeaponID;

#line 2053
    SITH_ASSERTREL(pShooter);

    rdMatrix_Copy34(pOutOrient, pStartOrient);
    rdVector_Copy3(&pOutOrient->dvec, pFireOffset);

    curWeaponID = pShooter->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( curWeaponID > SITHWEAPON_WHIP && curWeaponID < SITHWEAPON_GRENADE && curWeaponID != SITHWEAPON_MACHETE )
    {
        SithThing* pTarget;

        pTarget = sithPlayerControls_GetTargetThing();
        if ( pTarget )
        {
            float zOffset;
            rdVector3 targetPos;
            rdVector3 vecTargetFire;

            if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_NOTARGET) != 0 )
            {
                return 0;
            }

            zOffset = pTarget->thingInfo.actorInfo.eyeOffset.z;
            rdVector_Copy3(&targetPos, &pTarget->pos);
            if ( (pTarget->thingInfo.actorInfo.flags & SITH_AF_HUMAN) != 0 )
            {
                zOffset = zOffset * 0.25f; // don't aim head
            }
            else if ( curWeaponID == SITHWEAPON_SUBMACHINE || curWeaponID == SITHWEAPON_SHOTGUN )
            {
                zOffset = zOffset * 0.5f; // aim at torso
            }

            targetPos.z = targetPos.z + zOffset;
            rdVector_Sub3(&vecTargetFire, &targetPos, pFireOffset);
            if ( rdVector_Len3(&vecTargetFire) > 0.0f )
            {
                rdMatrix_LookAt(pOutOrient, pFireOffset, &targetPos, 0.0f);
                return 1;
            }
        }

        return 0;
    }
    else
    {
        size_t numThings;
        size_t i;
        SithThing* aTarget[16];
        int targetNum;
        float curDot;
        rdVector3 curDir;
        rdVector3 toDargetDir;
        float dot;

        if ( autoAimFovX == 0.0f || autoAimFovZ == 0.0f )
        {
            return 0;
        }

        if ( sithCamera_pCurCamera - sithCamera_aCameras == 1 )
        {
            autoAimFovX = autoAimFovX * 2.0f;
            autoAimFovZ = autoAimFovZ * 2.0f;
        }

        numThings = sithAIUtil_GetThingsInView(pShooter->pInSector, pOutOrient, autoAimFovX, autoAimFovZ, STD_ARRAYLEN(aTarget), aTarget, (SITHTHING_TYPE2MASK(SITH_THING_ACTOR) | SITHTHING_TYPE2MASK(SITH_THING_PLAYER)), 5.0f);// 0x404 - player | actor
        if ( numThings > 0 )
        {
            targetNum = -1;
            curDot = -1.0f;
            for ( i = 0; i < numThings; ++i )
            {
                if ( aTarget[i] == pShooter || (aTarget[i]->thingInfo.actorInfo.flags & SITH_AF_NOTARGET) != 0 )
                {
                    continue;
                }

                if ( !sithCollision_HasLOS(pShooter, aTarget[i], 0) )
                {
                    continue;
                }

                rdVector_Sub3(&toDargetDir, &aTarget[i]->pos, &pShooter->pos);
                if ( rdVector_Len3(&toDargetDir) > 0.0f )
                {
                    rdVector_Copy3(&curDir, &pOutOrient->lvec);
                    rdVector_Normalize3Acc(&toDargetDir);
                    rdVector_Normalize3Acc(&curDir);

                    dot = rdVector_Dot3(&curDir, &toDargetDir) < 0.0f ? -rdVector_Dot3(&curDir, &toDargetDir) : rdVector_Dot3(&curDir, &toDargetDir);
                    if ( targetNum < 0 || dot > curDot )
                    {
                        curDot    = dot;
                        targetNum = i;
                    }
                }
            }

            if ( targetNum >= 0 )
            {
                rdMatrix_LookAt(pOutOrient, pFireOffset, &aTarget[targetNum]->pos, 0.0f);
                return 1;
            }
        }

        return 0;
    }
}


SithThing* J3DAPI sithWeapon_FireProjectile(SithThing* pShooter, const SithThing* pProjectileTemplate, tSoundHandle hFireSnd, SithPuppetSubMode submode, rdVector3* pFireOffset, rdVector3* pAimError, float extra, SithFireProjectileFlags flags, float autoAimFovX, float autoAimFovZ)
{
    rdVector3 fireOffset;
    SithThing* pProjectileThing;
    rdMatrix34 muzzleOrient;
    rdMatrix34 aimMat;
    float secDeltaTime;
    float numExpectedFires;

#line 2195
    SITH_ASSERTREL(pShooter);
    if ( pShooter->type != SITH_THING_ACTOR && pShooter->type != SITH_THING_PLAYER )
    {
#line 2199
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing in sithWeapon_FireProjectile.\n", 0, 0, 0, 0);
        return 0;
    }

    
    if ( pShooter->type == SITH_THING_ACTOR )
    {
        rdVector3 targetPos;
        rdVector_Copy3(&fireOffset, &pShooter->thingInfo.actorInfo.weaponInfo.vecUnknown0);
        rdVector_Copy3(&targetPos, &pShooter->controlInfo.aiControl.pLocal->targetPos);
        return sithWeapon_FireProjectileEx(pShooter, pProjectileTemplate, hFireSnd, submode, pFireOffset, &targetPos, extra, flags, autoAimFovX, autoAimFovZ, &fireOffset, /*bUseFireOffset*/1);
    }

    
    if ( (flags & SITHFIREPROJECTILE_RIGHTHAND_FIRE) != 0 )
    {
        int jointIdx = sithThing_GetThingJointIndex(pShooter, "inrhand");
        if ( jointIdx == -1 )
        {
#line 2222
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad hand node in sithWeapon_FireProjectile.\n", 0, 0, 0, 0);
            return 0;
        }

        rdMatrix_Copy34(&muzzleOrient, &pShooter->renderData.paJointMatrices[jointIdx]);
        rdVector_Set3(&fireOffset, 0.0f, -90.0f, 0.0f);
        rdMatrix_PreRotate34(&muzzleOrient, &fireOffset);
    }
    else if ( (flags & SITHFIREPROJECTILE_TORSO_FIRE) != 0 )
    {
        int jointIdx = sithThing_GetThingJointIndex(pShooter, "intorso");
        if ( jointIdx == -1 )
        {
#line 2236
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad hand node in sithWeapon_FireProjectile.\n", 0, 0, 0, 0);
            return 0;
        }

        rdMatrix_Copy34(&muzzleOrient, &pShooter->renderData.paJointMatrices[jointIdx]);
        rdMatrix_PreRotate34(&muzzleOrient, &fireOffset);
    }
    else
    {
        rdMatrix_Copy34(&muzzleOrient, &pShooter->orient);
    }

    if ( pFireOffset->x == 0.0f && pFireOffset->y == 0.0f && pFireOffset->z == 0.0f )
    {
        rdVector_Copy3(pFireOffset, &pShooter->pos);
    }

    
    if ( (flags & SITHFIREPROJECTILE_AIM_ERROR) != 0 )
    {
        if ( !sithWeapon_GetAimOrient(&aimMat, pShooter, &muzzleOrient, pFireOffset, autoAimFovX, autoAimFovZ)
            && (pAimError->x != 0.0f || pAimError->y != 0.0f || pAimError->z != 0.0f) )
        {
            rdMatrix_PreRotate34(&aimMat, pAimError);
        }
    }
    else
    {
        rdMatrix_Copy34(&aimMat, &muzzleOrient);
        if ( pAimError->x != 0.0f || pAimError->y != 0.0f || pAimError->z != 0.0f )
        {
            rdMatrix_PreRotate34(&aimMat, pAimError);
        }
    }

    rdVector_Copy3(&fireOffset, &aimMat.lvec);
    secDeltaTime = 0.0f;
    if ( (flags & SITHFIREPROJECTILE_RAPID_FIRE) != 0 )
    {
        numExpectedFires = 1.0f;
        if ( pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired != -1.0f && pShooter->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime > 0.0f )
        {
            numExpectedFires = (sithTime_secGameTime - pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired)
                / pShooter->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime
                - 1.0f;
        }

        pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired = sithTime_secGameTime;
#line 2287
        SITH_ASSERTREL(numExpectedFires < 10);
        while ( numExpectedFires > 1.0f )
        {
            numExpectedFires = numExpectedFires - 1.0f;
            secDeltaTime = RD_MULTIPLY(pShooter->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime, numExpectedFires);
            sithWeapon_WeaponFire(pShooter, pProjectileTemplate, &fireOffset, pFireOffset, 0, submode, extra, flags, secDeltaTime);
        }
    }
    else
    {
        pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired = -1.0f;
    }

    pProjectileThing = sithWeapon_WeaponFire(pShooter, pProjectileTemplate, &fireOffset, pFireOffset, hFireSnd, submode, extra, flags, secDeltaTime);
    return pProjectileThing;
}

void J3DAPI sithWeapon_DeactivateCurrentWeapon(SithThing* pThing)
{
#line 2316
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_PLAYER) || (pThing ->type == SITH_THING_ACTOR)));

    if ( pThing->type == SITH_THING_PLAYER )
    {
        if ( sithWeapon_bPlayerWeaponActivated == 1 )
        {
            sithWeapon_bPlayerWeaponActivated = 0;
            sithWeapon_deactivatedPlayerWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
            sithWeapon_secPlayerWeaponDeactivationTime = pThing->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitEndTime;
        }
    }
    else // Actor
    {
        SithInventoryType* pItem = sithInventory_GetType(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
        if ( pItem->pCog ) {
            sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }
    }
}

int J3DAPI sithWeapon_SelectNextWeapon(SithThing* pThing)
{
    int curcurWeaponID;
    int weaponID;

#line 2353
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);

    if ( pThing->thingInfo.actorInfo.weaponInfo.curWeaponID == SITHWEAPON_NO_WEAPON )
    {
        curcurWeaponID = sithWeapon_lastPlayerWeaponID;
    }
    else
    {
        curcurWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
        if ( curcurWeaponID > sithInventory_FindNextTypeID(pThing, curcurWeaponID, SITHINVENTORY_TYPE_PLAYERWEAPON) )
        {
            sithWeapon_DeselectWeapon(pThing);
            return 0;
        }
    }

    weaponID = sithInventory_FindNextTypeID(pThing, curcurWeaponID, SITHINVENTORY_TYPE_PLAYERWEAPON);
    if ( weaponID != -1 )
    {
        return sithWeapon_SelectWeapon(pThing, (SithWeaponId)weaponID);
    }

    return -1;
}

int J3DAPI sithWeapon_SelectPreviousWeapon(SithThing* pThing)
{
    int curcurWeaponID;
    int weaponId;

#line 2394
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);

    if ( pThing->thingInfo.actorInfo.weaponInfo.curWeaponID == SITHWEAPON_NO_WEAPON )
    {
        curcurWeaponID = sithWeapon_lastPlayerWeaponID;
    }
    else
    {
        curcurWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
        if ( curcurWeaponID < sithInventory_FindPreviousTypeID(pThing, curcurWeaponID, SITHINVENTORY_TYPE_PLAYERWEAPON) )
        {
            sithWeapon_DeselectWeapon(pThing);
            return 0;
        }
    }


    weaponId = sithInventory_FindPreviousTypeID(pThing, curcurWeaponID, SITHINVENTORY_TYPE_PLAYERWEAPON);
    if ( weaponId != -1 )
    {
        return sithWeapon_SelectWeapon(pThing, (SithWeaponId)weaponId);
    }

    return -1;
}

void J3DAPI sithWeapon_SetWeaponModel(SithThing* pThing, SithWeaponId weaponID)
{
    int meshIdx;

#line 2434
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    SITH_ASSERTREL(sithInventory_aTypes[weaponID].pModel);

    meshIdx = sithThing_GetThingMeshIndex(pThing, "inrhand");
    if ( meshIdx == -1 )
    {
        return;
    }

    pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum = sithThing_AddSwapEntry(pThing, meshIdx, sithInventory_aTypes[weaponID].pModel, 0);
}

void J3DAPI sithWeapon_ResetWeaponModel(SithThing* pThing)
{
#line 2455
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    sithThing_RemoveSwapEntry(pThing, pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum);
}

void J3DAPI sithWeapon_SetHolsterModel(SithThing* pThing, SithWeaponId weaponID, signed int meshNum)
{
#line 2471
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(sithInventory_aTypes[weaponID].pHolsterModel);
    if ( pThing->type == SITH_THING_PLAYER )
    {
        switch ( weaponID )
        {
            case SITHWEAPON_WHIP:
                sithWeapon_playerWhipHolsterSwapRefNum = sithThing_AddSwapEntry(pThing, meshNum, sithInventory_aTypes[weaponID].pHolsterModel, /*meshNumSrc=*/0);
                break;

            case SITHWEAPON_PISTOL:
            case SITHWEAPON_TOKAREV:
            case SITHWEAPON_MAUSER:
                sithWeapon_playerPistolHolsterSwapRefNum = sithThing_AddSwapEntry(pThing, meshNum, sithInventory_aTypes[weaponID].pHolsterModel, /*meshNumSrc=*/0);
                break;

            case SITHWEAPON_SIMONOV:
            case SITHWEAPON_MACHETE:
            case SITHWEAPON_SUBMACHINE:
            case SITHWEAPON_SHOTGUN:
            case SITHWEAPON_BAZOOKA:
                sithWeapon_playerBackHolsterSwapRefNum = sithThing_AddSwapEntry(pThing, meshNum, sithInventory_aTypes[weaponID].pHolsterModel, /*meshNumSrc=*/0);
                break;
        }
    }
    else if ( pThing->type == SITH_THING_ACTOR )
    {
        sithThing_AddSwapEntry(pThing, meshNum, sithInventory_aTypes[weaponID].pHolsterModel, /*meshNumSrc=*/0);
    }
}

void J3DAPI sithWeapon_ResetHolsterModel(SithThing* pThing, int holsterNum)
{
#line 2514
    SITH_ASSERTREL(pThing);

    switch ( holsterNum )
    {
        case SITHWEAPON_HOLSTWEWHIP: // whipholster
            if ( sithWeapon_playerWhipHolsterSwapRefNum != -1 )
            {
                sithThing_RemoveSwapEntry(pThing, sithWeapon_playerWhipHolsterSwapRefNum);
            }

            return;

        case SITHWEAPON_HOLSTERPISTOL: // pistol holster
            if ( sithWeapon_playerPistolHolsterSwapRefNum != -1 )
            {
                sithThing_RemoveSwapEntry(pThing, sithWeapon_playerPistolHolsterSwapRefNum);
            }

            return;

        case SITHWEAPON_HOLSTERBACK: // back holster
            if ( sithWeapon_playerBackHolsterSwapRefNum != -1 )
            {
                sithThing_RemoveSwapEntry(pThing, sithWeapon_playerBackHolsterSwapRefNum);
            }

            return;

        default:
            return;
    }
}

/* Linker-discarded writer (relocations.yml `discarded:`): retail reads sithWeapon_bGenBloodsplort
   (HandleImpact, ThingCollisionHandler) and no code stores it; the debug build has
   this setter here, between ResetHolsterModel and SendMessageAim. Name and signature are
   inferred (the debug header spelled the parameter `bool`); body cut to the store. */
void J3DAPI sithWeapon_EnablBloodsplort(int bEnable)
{
    sithWeapon_bGenBloodsplort = bEnable;
}

void J3DAPI sithWeapon_SendMessageAim(SithThing* pThing, int bAim)
{
    // TODO: COG messages could be sent via thing cog overloads

    unsigned int curcurWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    SithInventoryType* pWaponItem = sithInventory_GetType(curcurWeaponID);
    if ( !pWaponItem || !pWaponItem->pCog )
    {
        return;
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        if ( sithWeapon_IsMountingWeapon(pThing) )
        {
            return;
        }

        if ( sithWeapon_IsLocalPlayerUnableToUseWeapon() )
        {
            return;
        }

        if ( bAim )
        {
            if ( !sithWeapon_bLocalPlayerAiming )
            {
                sithCog_SendMessageEx(pWaponItem->pCog, SITHCOG_MSG_AIM, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0, 1, 0, 0, 0);
                sithWeapon_bLocalPlayerAiming = 1;
            }
        }
        else
        {
            if ( pThing->thingInfo.actorInfo.weaponInfo.curWeaponID == SITHWEAPON_WHIP
                && (sithWhip_GetWhipSwingThing() || sithWhip_GetWhipClimbThing()) )
            {
                return;
            }

            if ( sithWeapon_bLocalPlayerAiming )
            {
                sithWeapon_bLocalPlayerAiming = 0;
            }

            if ( sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_AIMWEAPON) )
            {
                sithCog_SendMessageEx(pWaponItem->pCog, SITHCOG_MSG_AIM, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0, 0, 0, 0, 0);
            }
        }
    }
    else if ( pThing->type == SITH_THING_ACTOR )
    {
        if ( bAim )
        {
            if ( sithWeapon_IsMountingWeapon(pThing) )
            {
                return;
            }

            sithCog_SendMessageEx(pWaponItem->pCog, SITHCOG_MSG_AIM, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0, 1, 0, 0, 0);
        }
        else
        {
            if ( sithWeapon_IsMountingWeapon(pThing) )
            {
                if ( J3DINRANGE(curcurWeaponID, SITHWEAPON_COMFISTS, SITHWEAPON_COMSHOTGUN) )
                {
                    sithWeapon_DeactivateCurrentWeapon(pThing);
                }
                else
                {
                    sithWeapon_DeactivateWeapon(pThing, pWaponItem->pCog);
                }
            }

            sithCog_SendMessageEx(pWaponItem->pCog, SITHCOG_MSG_AIM, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0, 0, 0, 0, 0);
        }
    }
}


void J3DAPI sithWeapon_SetActorKilledCallback(SithWeaponActorKilledCallback pfCallback)
{
    sithWeapon_pfActorKilledCallback = pfCallback;
}

int J3DAPI sithWeapon_IsAiming(SithThing* pThing)
{
#line 2652
    SITH_ASSERTREL(pThing);
    if ( pThing->type == SITH_THING_PLAYER )
    {
        return sithWeapon_bLocalPlayerAiming;
    }

    if ( pThing->type == SITH_THING_ACTOR
        && J3DINRANGE(pThing->thingInfo.actorInfo.weaponInfo.curWeaponID, SITHWEAPON_COMTOKAREV, SITHWEAPON_COMSHOTGUN)
        && sithPuppet_IsModeOnTrack(pThing, SITHPUPPETSUBMODE_AIMWEAPON) )
    {
        return 1;
    }

    return 0;
}


void J3DAPI sithWeapon_GenBloodsplort(SithThing* pHitThing)
{
    rdMaterial* pMat;
    SithThing* pCurTemplate;
    rdVector3 end;

#line 2691
    SITH_ASSERTREL(pHitThing != NULL);
    SITH_ASSERTREL(pHitThing->renderData.data.pModel3 != NULL);
    SITH_ASSERTREL((pHitThing->type == SITH_THING_ACTOR) || (pHitThing->type == SITH_THING_CORPSE));

    if ( (pHitThing->thingInfo.actorInfo.flags & SITH_AF_ARACHNID) != 0 )
    {
        pCurTemplate = sithTemplate_GetTemplate("+vulcansplort");
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(420)); // gen_a4sprite_blood_grn.mat
    }
    else
    {
        pCurTemplate = sithTemplate_GetTemplate("+bloodsplort");
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(179)); // gen_a4sprite_blood.mat
    }

    if ( pCurTemplate )
    {
        SithThing* pSprite = sithThing_CreateThingAtPos(pCurTemplate, &pHitThing->pos, &pHitThing->orient, pHitThing->pInSector, NULL);
        if ( pSprite )
        {
            float cursize;
            rdVector3 start;

            int meshIndex = sithThing_GetThingMeshIndex(pHitThing, "intorso");
            if ( meshIndex == -1 )
            {
                if ( !strncmp(pHitThing->aName, "snak", 4u) )
                {
                    meshIndex = sithThing_GetThingMeshIndex(pHitThing, "snhead");
                }
                else if ( !strncmp(pHitThing->aName, "spid", 4u) )
                {
                    meshIndex = sithThing_GetThingMeshIndex(pHitThing, "spbelly");
                }
                else if ( !strncmp(pHitThing->aName, "scor", 4u) )
                {
                    meshIndex = sithThing_GetThingMeshIndex(pHitThing, "scsect1");
                }
                else if ( !strncmp(pHitThing->aName, "komo", 4u) )
                {
                    meshIndex = sithThing_GetThingMeshIndex(pHitThing, "lineck");
                }
                else if ( !strncmp(pHitThing->aName, "cent", 4u) )
                {
                    meshIndex = sithThing_GetThingMeshIndex(pHitThing, "cphead");
                }

                if ( meshIndex == -1 )
                {
                    meshIndex = 0;
                }

#line 2730
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Blood splort attached to mesh number %d.\n", meshIndex, 0, 0, 0);
            }

            sithThing_AttachThingToThingMesh(pSprite, pHitThing, meshIndex);
            if ( pMat )
            {
                sithAnimate_StartMaterialAnim(pMat, 16.0f, (SithAnimateFlags)0);
            }

            cursize = pHitThing->renderData.data.pModel3->size;
            cursize = cursize / 2.0f;
            cursize = STDMATH_CLAMP(cursize, 0.01f, 0.1f);

            
            start.x = cursize;
            start.y = cursize;
            start.z = 1.0f;


            cursize = cursize * 2.0f;
            end.x = cursize;
            end.y = cursize;
            end.z = 1.0f;
            sithAnimate_StartSpriteSizeAnim(pSprite, &start, &end, 0.5f);
        }
    }
}

SithWeaponId J3DAPI sithWeapon_GetLastWeapon(void)
{
    return sithWeapon_lastPlayerWeaponID;
}

void J3DAPI sithWeapon_SetLastWeapon(SithWeaponId weaponNum)
{
#line 2762
    SITH_ASSERTREL((weaponNum >= SITHWEAPON_NO_WEAPON) && (weaponNum <= SITHWEAPON_IMP5));
    sithWeapon_lastPlayerWeaponID = weaponNum;
}

float J3DAPI sithWeapon_GetWeaponMaxAimDistance(SithWeaponId weaponId)
{
    if ( J3DINRANGE(weaponId, SITHWEAPON_NO_WEAPON, SITHWEAPON_BAZOOKA) )
    {
        return sithWeapon_aMaxAimDistances[weaponId];
    }

    if ( J3DINRANGE(weaponId, SITHWEAPON_COMFISTS, SITHWEAPON_COMSHOTGUN) )
    {
        return sithWeapon_aMaxAimDistances[weaponId];
    }

    if ( weaponId == SITHWEAPON_MIRROR )
    {
        return 4.0f;
    }

    return 0.0f;
}


float J3DAPI sithWeapon_GetWeaponCollideSize(SithWeaponId typeId)
{
    switch ( typeId )
    {
        case SITHWEAPON_COMTOKAREV:
        case SITHWEAPON_COMMAUSER:
        case SITHWEAPON_COMSIMONOV:
        case SITHWEAPON_COMSUBMACHINE:
            return 0.02f;

        case SITHWEAPON_COMSHOTGUN:
            return 0.065f;
    }

    return 0.0f;
}


int J3DAPI sithWeapon_IsLocalPlayerUnableToUseWeapon(void)
{
    // TODO: Why sithPlayer_pLocalPlayerThing is used? Verify if player thing could be passed instead to this function
#line 2804
    SITH_ASSERTREL(sithPlayer_pLocalPlayerThing);
    if ( sithPuppet_IsModeOnTrack(sithPlayer_pLocalPlayerThing, SITHPUPPETSUBMODE_PUSHPULLREADY) )
    {
        return 1;
    }

    switch ( sithPlayer_pLocalPlayerThing->moveStatus )
    {
        case SITHPLAYERMOVE_STILL:
        case SITHPLAYERMOVE_WALKING:
        case SITHPLAYERMOVE_RUNNING:
        case SITHPLAYERMOVE_SWIMIDLE:
        case SITHPLAYERMOVE_WALK2STAND:
        case SITHPLAYERMOVE_STAND2WALK:
        case SITHPLAYERMOVE_UNKNOWN_78:
        case SITHPLAYERMOVE_STAND2RUN:
        case SITHPLAYERMOVE_UNKNOWN_80:
        case SITHPLAYERMOVE_UNKNOWN_81:
            return 0;
        default:
            break;
    }

    return 1;
}

int J3DAPI sithWeapon_Save(tFileHandle fh)
{
    size_t size;

    size = sizeof(sithWeapon_bPlayerWeaponActivated);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_bPlayerWeaponActivated, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_bLocalPlayerAiming);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_bLocalPlayerAiming, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_lastPlayerWeaponID);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_lastPlayerWeaponID, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_bufferedWaponKeyId);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_bufferedWaponKeyId, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_deactivatedPlayerWeaponID);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_deactivatedPlayerWeaponID, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_playerWhipHolsterSwapRefNum);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_playerWhipHolsterSwapRefNum, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_playerPistolHolsterSwapRefNum);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_playerPistolHolsterSwapRefNum, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_playerBackHolsterSwapRefNum);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_playerBackHolsterSwapRefNum, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_secMountWait);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_secMountWait, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_secPlayerWeaponDeactivationTime);
    if ( size != sith_pHS->pFileWrite(fh, &sithWeapon_secPlayerWeaponDeactivationTime, size) )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}


int J3DAPI sithWeapon_Restore(tFileHandle fh)
{
    size_t size;

    size = sizeof(sithWeapon_bPlayerWeaponActivated);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_bPlayerWeaponActivated, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_bLocalPlayerAiming);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_bLocalPlayerAiming, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_lastPlayerWeaponID);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_lastPlayerWeaponID, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_bufferedWaponKeyId);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_bufferedWaponKeyId, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_deactivatedPlayerWeaponID);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_deactivatedPlayerWeaponID, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_playerWhipHolsterSwapRefNum);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_playerWhipHolsterSwapRefNum, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_playerPistolHolsterSwapRefNum);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_playerPistolHolsterSwapRefNum, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_playerBackHolsterSwapRefNum);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_playerBackHolsterSwapRefNum, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_secMountWait);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_secMountWait, size) )
    {
        goto error;
    }

    size = sizeof(sithWeapon_secPlayerWeaponDeactivationTime);
    if ( size != sith_pHS->pFileRead(fh, &sithWeapon_secPlayerWeaponDeactivationTime, size) )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}


SithWeaponId J3DAPI sithWeapon_GetWeaponForControlKey(SithControlFunction keyID)
{
    switch ( keyID )
    {
        case SITHCONTROL_WSELECT0:
            return SITHWEAPON_FISTS;

        case SITHCONTROL_WSELECT1:
            return SITHWEAPON_WHIP;

        case SITHCONTROL_WSELECT2:
            return SITHWEAPON_PISTOL;

        case SITHCONTROL_WSELECT3:
            return SITHWEAPON_TOKAREV;

        case SITHCONTROL_WSELECT4:
            return SITHWEAPON_MAUSER;

        case SITHCONTROL_WSELECT5:
            return SITHWEAPON_SIMONOV;

        case SITHCONTROL_WSELECT6:
            return SITHWEAPON_MACHETE;

        case SITHCONTROL_WSELECT7:
            return SITHWEAPON_SUBMACHINE;

        case SITHCONTROL_WSELECT8:
            return SITHWEAPON_SHOTGUN;

        case SITHCONTROL_WSELECT9:
            return SITHWEAPON_BAZOOKA;

        case SITHCONTROL_WSELECT10:
            return SITHWEAPON_GRENADE;

        case SITHCONTROL_WSELECT11:
            return SITHWEAPON_SATCHEL;

        case SITHCONTROL_WSELECT12:
            return SITHWEAPON_IMP1;

        case SITHCONTROL_WSELECT13:
            return SITHWEAPON_IMP2;

        case SITHCONTROL_WSELECT14:
            return SITHWEAPON_IMP3;

        case SITHCONTROL_WSELECT15:
            return SITHWEAPON_IMP4;

        case SITHCONTROL_WSELECT16:
            return SITHWEAPON_IMP5;

        default:
            break;
    }

    return -1;
}

SithThing* J3DAPI sithWeapon_FireProjectileEx(SithThing* pShooter, const SithThing* pProjectileTemplate, tSoundHandle hSndFire, SithPuppetSubMode submode, rdVector3* pFirePos, const rdVector3* pTargetPos, float extra, SithFireProjectileFlags flags, float autoAimFovX, float autoAimFovZ, const rdVector3* pFireOffset, int bUseFireOffset)
{
    rdVector3 dir;
    SithThing* pProjectileThing;
    float sedDeltaTime;
    float numExpectedFires;

#line 2987
    SITH_ASSERTREL(pShooter != NULL);

    if ( pShooter->type != SITH_THING_ACTOR )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing in sithWeapon_FireProjectileEx.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( pShooter->controlType == SITH_CT_AI )
    {
        SithAIControlBlock* pLocal = pShooter->controlInfo.aiControl.pLocal;
        if ( rdVector_Dot3(&pLocal->toTarget, &pShooter->orient.lvec) < pLocal->fireDot )
        {
            return 0;
        }
    }

    if ( pFirePos->x == 0.0f && pFirePos->y == 0.0f && pFirePos->z == 0.0f )
    {
        rdVector_Copy3(pFirePos, &pShooter->pos);
    }

    
    if ( bUseFireOffset == 1 )
    {
        if ( pShooter->thingInfo.actorInfo.fireOffset.x != 0.0f
            || pShooter->thingInfo.actorInfo.fireOffset.y != 0.0f
            || pShooter->thingInfo.actorInfo.fireOffset.z != 0.0f )
        {
            rdVector3 shooterFireOffset;
            rdVector_Copy3(&shooterFireOffset, &pShooter->thingInfo.actorInfo.fireOffset);
            rdMatrix_TransformVector34Acc(&shooterFireOffset, &pShooter->orient);

            rdVector_Add3Acc(pFirePos, &shooterFireOffset);
        }

        rdVector_Sub3(&dir, pTargetPos, pFirePos);

        rdVector_Add3Acc(&dir, pFireOffset);
        rdVector_Normalize3Acc(&dir);
    }
    else
    {
        rdVector_Copy3(&dir, &pShooter->orient.lvec);
    }

    sedDeltaTime = 0.0f;
    if ( (flags & SITHFIREPROJECTILE_RAPID_FIRE) != 0 )
    {
        numExpectedFires = 1.0f;
        if ( pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired != -1.0f && pShooter->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime > 0.0f )
        {
            numExpectedFires = (sithTime_secGameTime - pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired)
                / pShooter->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime
                - 1.0f;
        }

        pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired = sithTime_secGameTime;
#line 3060
        SITH_ASSERTREL(numExpectedFires < 10);
        while ( numExpectedFires > 1.0f )
        {
            numExpectedFires = numExpectedFires - 1.0f;
            sedDeltaTime = RD_MULTIPLY(pShooter->thingInfo.actorInfo.weaponInfo.secWeaponActivationWaitTime, numExpectedFires);
            sithWeapon_WeaponFire(pShooter, pProjectileTemplate, &dir, pFirePos, 0, submode, extra, flags, sedDeltaTime);
        }
    }
    else
    {
        pShooter->thingInfo.actorInfo.weaponInfo.secTimeLastRapidFired = -1.0f;
    }

    pProjectileThing = sithWeapon_WeaponFire(pShooter, pProjectileTemplate, &dir, pFirePos, hSndFire, submode, extra, flags, sedDeltaTime);
    return pProjectileThing;
}

void J3DAPI sithWeapon_CreateWeaponFireFx(SithThing* pThing, rdVector3* pos)
{
    int curweaponId;
    rdModel3* pHandModel;
    SithThing* flash;
    int meshIdx;

#line 3098
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    curweaponId = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    switch ( curweaponId )
    {
        case SITHWEAPON_PISTOL:
            pHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(83)); // weap_revolver_fire.3do
            break;

        case SITHWEAPON_TOKAREV:
        case SITHWEAPON_COMTOKAREV:
            pHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(85)); // weap_tokarev_fire.3do
            break;

        case SITHWEAPON_MAUSER:
        case SITHWEAPON_COMMAUSER:
            pHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(81)); // weap_mauser_fire.3do
            break;

        case SITHWEAPON_SIMONOV:
        case SITHWEAPON_COMSIMONOV:
            pHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(84)); // weap_simonov_fire.3do
            break;

        case SITHWEAPON_SUBMACHINE:
        case SITHWEAPON_COMSUBMACHINE:
            pHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(82)); // weap_ppsh41_fire.3do
            break;

        case SITHWEAPON_SHOTGUN:
        case SITHWEAPON_COMSHOTGUN:
            pHandModel = sithModel_GetModelByIndex(SITHWORLD_STATICINDEX(86)); // weap_toz34_fire.3do
            break;

        default:
            return;
    }

    meshIdx = sithThing_GetThingMeshIndex(pThing, "inrhand");
    if ( pHandModel && meshIdx != -1 )
    {
        pThing->thingInfo.actorInfo.weaponInfo.weaponSwapRefNum = sithThing_AddSwapEntry(pThing, meshIdx, pHandModel, 0);
        pThing->thingInfo.actorInfo.weaponInfo.secWeaponSwapTime = sithTime_secGameTime + 0.025f;
        flash = sithTemplate_GetTemplate("ghost");
        flash = sithThing_CreateThingAtPos(flash, pos, &pThing->orient, pThing->pInSector, pThing->pParent);
        if ( flash )
        {
            flash->light.color.red   = 0.5f;
            flash->light.color.green = 0.5f;
            flash->light.color.blue  = 0.5f;
            flash->light.minRadius   = 0.2f;
            flash->light.maxRadius   = 0.4f;
            flash->flags |= SITH_TF_EMITLIGHT;
            flash->msecLifeLeft      = 50;
        }
    }
}

int J3DAPI sithWeapon_ActivateWhip(SithThing* pThing, SithCog* pCog)
{
    int bValidTgt = 0;
    int success = 0;
    SithThing* pWhipTarget;

#line 3166
    SITH_ASSERTREL(pCog != NULL);

    pWhipTarget = sithWhip_GetWhipSwingThing();
    if ( pWhipTarget )
    {
        bValidTgt = (pWhipTarget->flags & SITH_TF_WHIPCLIMB) == 0;
        if ( bValidTgt )
        {
            success = sithWhip_StartWhipSwing(pThing);
        }
    }
    else
    {
        pWhipTarget = sithWhip_GetWhipClimbThing();
        if ( pWhipTarget )
        {
            bValidTgt = (pWhipTarget->flags & SITH_TF_WHIPSWING) == 0;
            if ( bValidTgt )
            {
                success = sithWhip_StartWhipClimb(pThing, pWhipTarget);
            }
        }
    }

    if ( pWhipTarget )
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_ELECTRICWHIP) != 0 )
        {
            sithCog_ThingSendMessageEx(pWhipTarget, pThing, SITHCOG_MSG_DAMAGED, 0, SITH_DAMAGE_ELECTROWHIP, 0, 0);
        }
        else
        {
            sithCog_ThingSendMessageEx(pWhipTarget, pThing, SITHCOG_MSG_DAMAGED, 0, SITH_DAMAGE_WHIP, 0, 0);
        }

        if ( !bValidTgt )
        {
            return 1;
        }
    }

    if ( bValidTgt && success )
    {
        sithCog_SendMessage(pCog, SITHCOG_MSG_ACTIVATE, SITHCOG_SYM_REF_THING, -99, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        return 1;
    }

    return 0;
}
