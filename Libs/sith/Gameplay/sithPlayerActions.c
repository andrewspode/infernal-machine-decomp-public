#include "sithPlayerActions.h"
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPlayerActions_countfit_7, 30) CF_PROTO(sithPlayerActions_countfit_1, 5) /* end count-fit */
#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <string.h>

int sithPlayerActions_jewelFlyingPuppetTrackNum = -1;

static float sithPlayerActions_raftInflateDistance    = 0.1f;
static float sithPlayerActions_raftActivateSearchRadius = 0.07f;
static float sithPlayerActions_raftFloorNormalZ        = 0.9996f;

/* Zero-initialized definitions in retail's .bss order (0x0056B768-0x0056B784):
   these addresses lie inside the TU's own private block, so they cannot have
   been tentative definitions - those become COMMON requests the linker
   satisfies in the image's far common tail, like the three chalk-write globals
   (declared in sithPlayerActions.h): their COMMON requests come from
   sithPlayerControls.c, whose pull position the retail tail puts them at. */
SithSurface* sithPlayerActions_pCurCollidedSurface = NULL;
rdFace* sithPlayerActions_pCurCollidedModelFace = NULL;
rdModel3* sithPlayerActions_pCurCollidedModel = NULL;
SithThing* sithPlayerActions_pPlasma = NULL;
int sithPlayerActions_bJewelFlying = 0;
int sithPlayerActions_bPlayerInvisible = 0;
unsigned int sithPlayerActions_msecCurActivateTime = 0;

/* The jump-sound selection is written out at all eight of its sites in the Indy3D debug build (/Ob0, so an `__inline`
 * function would be a call there), each into a `mode` the function initialises to -1: a statement macro (name and
 * form inferred). Its two pointer locals are the callers': every debug frame has them among the function-scope slots. */
#define SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pAttachedSurf) \
    if ( ((pThing)->attach.flags & (SITH_ATTACH_THINGFACE | SITH_ATTACH_THING)) != 0 ) \
    { \
        pAttachedThing = (pThing)->attach.attachedToStructure.pThingAttached; \
        if ( (pAttachedThing->flags & SITH_TF_METAL) != 0 ) \
        { \
            mode = SITHSOUNDCLASS_JUMPMETAL; \
        } \
        else if ( (pAttachedThing->flags & SITH_TF_EARTH) != 0 ) \
        { \
            mode = SITHSOUNDCLASS_JUMPEARTH; \
        } \
        else if ( (pAttachedThing->flags & SITH_TF_SNOW) != 0 ) \
        { \
            mode = SITHSOUNDCLASS_JUMPSNOW; \
        } \
        else if ( (pAttachedThing->flags & SITH_TF_WOOD) != 0 ) \
        { \
            mode = SITHSOUNDCLASS_JUMPWOOD; \
        } \
        else \
        { \
            mode = SITHSOUNDCLASS_JUMP; \
        } \
    } \
    else \
    { \
        pAttachedSurf = (pThing)->attach.attachedToStructure.pSurfaceAttached; \
        if ( (pAttachedSurf->flags & (SITH_SURFACE_METAL | SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_EARTH | SITH_SURFACE_WEB | SITH_SURFACE_SNOW | SITH_SURFACE_WOOD | SITH_SURFACE_ECHO | SITH_SURFACE_WOODECHO | SITH_SURFACE_EARTHECHO | SITH_SURFACE_AETHERIUM)) != 0 ) \
        { \
            if ( (pAttachedSurf->flags & SITH_SURFACE_METAL) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPMETAL; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_WATER) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPWATER; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_SHALLOWWATER) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPWATER; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_EARTH) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPEARTH; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_SNOW) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPSNOW; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_WOOD) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPWOOD; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_ECHO) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPHARDECHO; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_WOODECHO) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPWOODECHO; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_EARTHECHO) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPEARTHECHO; \
            } \
            else if ( (pAttachedSurf->flags & SITH_SURFACE_AETHERIUM) != 0 ) \
            { \
                mode = SITHSOUNDCLASS_JUMPAET; \
            } \
            else \
            { \
                mode = SITHSOUNDCLASS_JUMP; \
            } \
        } \
        else \
        { \
            mode = SITHSOUNDCLASS_JUMP; \
        } \
    }

int J3DAPI sithPlayerActions_Activate(SithThing* pPlayer)
{
    SithSector* pSector;
    unsigned int typeId;
    rdVector3 eyePos;
    SithSurface* pHitSurf;
    float distance;
    rdMatrix34 eyeOrient;
    SithCollision* pCollision;
    int bActivated;

    bActivated = 0;
    pHitSurf   = NULL;
    if ( sithPlayerActions_msecCurActivateTime + 500 > sithTime_clockTime )
    {
        return 0;
    }

    typeId = pPlayer->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( typeId > SITHWEAPON_NO_WEAPON && typeId < SITHWEAPON_ZIPPO )
    {
        return 0;
    }

    sithPlayerActions_msecCurActivateTime = sithTime_clockTime;

    rdMatrix_Copy34(&eyeOrient, &pPlayer->orient);
    rdVector_Copy3(&eyePos, &pPlayer->pos);
    pSector = sithCollision_FindSectorInRadius(pPlayer->pInSector, &pPlayer->pos, &eyePos, 0.0f);
    if ( !pSector )
    {
#line 139
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Activate: eyepoint outside of sector?\n", 0, 0, 0, 0);
        return 0;
    }

    if ( pPlayer == sithPlayer_pLocalPlayerThing
        && sithInventory_GetCurrentItem(pPlayer) == SITHWEAPON_CHALK
        && pPlayer->pInSector
        && (pPlayer->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0
        && !sithInventory_GetCurrentWeapon(pPlayer)
        && !sithWeapon_IsMountingWeapon(pPlayer)
        && !pPlayer->moveStatus )
    {
        SithSector* pWriteSector;
        rdVector3 writePos;

        eyePos.z += 0.04f;
        pSector = sithCollision_FindSectorInRadius(pPlayer->pInSector, &pPlayer->pos, &eyePos, 0.0f);
        if ( !pSector )
        {
            return 0;
        }

        sithCollision_SearchForCollisions(pSector, pPlayer, &eyePos, &pPlayer->orient.lvec, pPlayer->collide.movesize + 0.04f + 0.04f, 0.01f, 3);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) == 0 )
                {
                    if ( pCollision->pSurfaceCollided->pAdjoin && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
                    {
                        continue;
                    }

                    pHitSurf = pCollision->pSurfaceCollided;
                    distance = pCollision->distance;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                break;
            }
        }

        sithCollision_DecreaseStackLevel();

        if ( !pHitSurf )
        {
            sithSoundClass_PlayVoiceModeRandom(pPlayer, SITHSOUNDCLASS_SPLATTERED);
            return 0;
        }

        if ( pHitSurf->pAdjoin
            && ((pHitSurf->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 || (pHitSurf->pAdjoin->flags & SITH_ADJOIN_MOVE) == 0) )
        {
            sithSoundClass_PlayVoiceModeRandom(pPlayer, SITHSOUNDCLASS_SPLATTERED);
            return 0;
        }

        if ( (pHitSurf->flags & SITH_SURFACE_WATER) != 0 )
        {
            sithSoundClass_PlayVoiceModeRandom(pPlayer, SITHSOUNDCLASS_SPLATTERED);
            return 0;
        }

        rdVector_Scale3(&writePos, &pPlayer->orient.lvec, distance + pPlayer->collide.movesize);
        rdVector_Add3Acc(&writePos, &pPlayer->pos);
        writePos.z += pPlayer->collide.movesize;

        pWriteSector = sithCollision_FindSectorInRadius(pPlayer->pInSector, &pPlayer->pos, &writePos, 0.0f);
        if ( !pWriteSector )
        {
            return 0;
        }

        pPlayer->moveStatus = SITHPLAYERMOVE_ACTIVATING;
        pPlayer->thingInfo.actorInfo.bControlsDisabled = 1;

        sithPlayerActions_pChalkWriteSurf = pHitSurf;
        rdVector_Copy3(&sithPlayerActions_posChalkMarkWrite, &writePos);
        sithPlayerActions_pChalkWriteSector = pWriteSector;

        sithCog_ThingSendMessage(pPlayer, pPlayer, SITHCOG_MSG_USER6);
        sithPhysics_ResetThingMovement(pPlayer);
        sithPuppet_PlayMode(pPlayer, SITHPUPPETSUBMODE_ACTIVATE, sithPlayerControls_PuppetCallback);
        return 1;
    }

    if ( sithInventory_GetCurrentItem(pPlayer) != 56 )
    {
        SithThing* pHitThing;
        SithInventoryType* pType;
        SithSector* pDownSector;
        rdVector3 downPos;

        rdVector_Copy3(&eyePos, &pPlayer->pos);
        pSector = sithCollision_FindSectorInRadius(pPlayer->pInSector, &pPlayer->pos, &eyePos, 0.0f);

        pHitSurf = NULL;
        sithCollision_SearchForCollisions(pSector, pPlayer, &eyePos, &eyeOrient.lvec, pPlayer->collide.movesize + 0.04f, 0.05f, 2);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( (sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                {
#line 230
                    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Activate hit surface %d.\n", sithSurface_GetSurfaceIndex(pCollision->pSurfaceCollided), 0, 0, 0);
                }

                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) != 0 && typeId < SITHWEAPON_IMP1 )
                {
                    sithCog_SurfaceSendMessage(pCollision->pSurfaceCollided, pPlayer, SITHCOG_MSG_ACTIVATE);
                    bActivated = 1;
                    break;
                }

                if ( !pHitSurf )
                {
                    pHitSurf = pCollision->pSurfaceCollided;
                    distance = pCollision->distance;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                pHitThing = pCollision->pThingCollided;
                if ( pHitThing->type == SITH_THING_ITEM || pHitThing->type == SITH_THING_WEAPON )
                {
                    continue;
                }

                if ( pPlayer->controlType == SITH_CT_AI && pHitThing->type == SITH_THING_PLAYER )
                {
                    continue;
                }

                if ( (sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                {
#line 264
                    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Activate hit thing %s.\n", pHitThing->aName, 0, 0, 0);
                }

                if ( (pHitThing->flags & SITH_TF_COGLINKED) != 0 && (pHitThing->flags & SITH_TF_MOVABLE) == 0 )
                {
                    sithCog_ThingSendMessage(pHitThing, pPlayer, SITHCOG_MSG_ACTIVATE);
                    bActivated = 1;
                    if ( typeId > SITHWEAPON_ZIPPO )
                    {
                        if ( pHitThing->renderData.type != RD_THING_MODEL3 || !pHitThing->renderData.data.pModel3 )
                        {
                            continue;
                        }

                        if ( streqi(pHitThing->renderData.data.pModel3->aName, "imp_stands.3do") )
                        {
                            pType = sithInventory_GetType(typeId);
                            if ( pType->pCog )
                            {
                                sithCog_SendMessage(pType->pCog, SITHCOG_MSG_CHANGED, SITHCOG_SYM_REF_THING, pPlayer->idx, SITHCOG_SYM_REF_THING, pHitThing->idx, 0);
                            }
                        }
                    }

                    break;
                }
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( !bActivated )
        {
            rdVector_ScaleAdd3(&downPos, &pPlayer->orient.lvec, 0.035f, &pPlayer->pos);
            pDownSector = sithCollision_FindSectorInRadius(pPlayer->pInSector, &pPlayer->pos, &downPos, 0.0f);
            if ( !pDownSector )
            {
                // Empty arm: the debug build jumps from this test straight past the downward search (jne; jmp).
            }
            else
            {
                rdVector_Neg3(&eyePos, &rdroid_zVector3);
                sithCollision_SearchForCollisions(pDownSector, pPlayer, &downPos, &eyePos, 0.11f, 0.04f, 2);
                while ( (pCollision = sithCollision_PopStack()) != NULL )
                {
                    if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                    {
                        if ( (sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                        {
#line 322
                            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Activate down hit surface %d.\n", sithSurface_GetSurfaceIndex(pCollision->pSurfaceCollided), 0, 0, 0);
                        }

                        if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) != 0 )
                        {
                            sithCog_SurfaceSendMessage(pCollision->pSurfaceCollided, pPlayer, SITHCOG_MSG_ACTIVATE);
                            bActivated = 1;
                            break;
                        }
                    }
                    else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                    {
                        if ( pCollision->pThingCollided->type == SITH_THING_ITEM || pCollision->pThingCollided->type == SITH_THING_WEAPON )
                        {
                            continue;
                        }

                        if ( pPlayer->controlType == SITH_CT_AI && pCollision->pThingCollided->type == SITH_THING_PLAYER )
                        {
                            continue;
                        }

                        if ( (sith_mode.debugModeFlags & SITHDEBUG_TRACKSHOTS) != 0 )
                        {
#line 344
                            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Activate down hit thing %s.\n", pCollision->pThingCollided->aName, 0, 0, 0);
                        }

                        if ( (pCollision->pThingCollided->flags & SITH_TF_COGLINKED) != 0 && (pCollision->pThingCollided->flags & SITH_TF_MOVABLE) == 0 )
                        {
                            sithCog_ThingSendMessage(pCollision->pThingCollided, pPlayer, SITHCOG_MSG_ACTIVATE);
                            bActivated = 1;
                            break;
                        }
                    }
                }

                sithCollision_DecreaseStackLevel();
            }
        }
    }

    if ( !bActivated && pPlayer == sithPlayer_pLocalPlayerThing && pPlayer->pInSector && (pPlayer->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        int curItem;
        SithThing* pRaftThing;
        float searchDist;
        rdVector3 startPos;
        float boardDist;
        rdVector3 moveNorm;
        int bBoardRaft;
        int playerNum;
        SithSector* pStartSector;
        /* Original bug: bExitedSector is never initialised; the debug code tests whatever its slot holds. */
        int bExitedSector;
        rdVector3 raftPos;
        rdMatrix34 raftOrient;

        curItem = sithInventory_GetCurrentItem(pPlayer);
        if ( curItem == 57
            && !sithInventory_GetCurrentWeapon(pPlayer)
            && !sithWeapon_IsMountingWeapon(pPlayer)
            && (pPlayer->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0 )
        {
            pRaftThing = NULL;
            bBoardRaft = 0;
            if ( (pPlayer->attach.flags & SITH_ATTACH_SURFACE) != 0
                && (pPlayer->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_ISFLOOR) != 0
                && pPlayer->attach.attachedToStructure.pSurfaceAttached->face.normal.z > sithPlayerActions_raftFloorNormalZ )
            {
                rdVector_ScaleAdd3(&startPos, &pPlayer->orient.lvec, sithPlayerActions_raftInflateDistance, &pPlayer->pos);
                pStartSector = sithCollision_FindSectorInRadius(pPlayer->pInSector, &pPlayer->pos, &startPos, 0.001f);
                if ( !pStartSector )
                {
                    return 0;
                }

                rdVector_Neg3(&moveNorm, &rdroid_zVector3);
                searchDist = sithPhysics_GetThingHeight(pPlayer) + 0.06f;
                sithCollision_SearchForCollisions(pStartSector, NULL, &startPos, &moveNorm, searchDist, sithPlayerActions_raftActivateSearchRadius, 1);
                while ( (pCollision = sithCollision_PopStack()) != NULL )
                {
                    if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
                    {
                        if ( pCollision->pSurfaceCollided
                            && pCollision->pSurfaceCollided->pAdjoin
                            && (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
                        {
                            boardDist  = pCollision->distance;
                            bBoardRaft = 1;
                            break;
                        }
                    }
                    else
                    {
                        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
                            && (pCollision->type & SITHCOLLISION_FACE) != 0
                            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
                            && sithPlayerActions_raftActivateSearchRadius + pCollision->distance - sithPhysics_GetThingHeight(pPlayer) >= 0.04f
                            && sithPlayerActions_raftActivateSearchRadius + pCollision->distance - sithPhysics_GetThingHeight(pPlayer) <= 0.06f
                            && pCollision->pSurfaceCollided->face.normal.z > 0.99984f )
                        {
                            boardDist  = pCollision->distance + sithPlayerActions_raftActivateSearchRadius;
                            bBoardRaft = 1;
                            break;
                        }

                        break;
                    }
                }

                sithCollision_DecreaseStackLevel();

                if ( bBoardRaft )
                {
                    for ( playerNum = 0; playerNum < (int)sithPlayer_numPlayers; ++playerNum )
                    {
                        if ( !strcmp(sithPlayer_aPlayers[playerNum].pThing->aName, "raftplayer")
                            || !strcmp(sithPlayer_aPlayers[playerNum].pThing->aName, "shirtraftplayer") )
                        {
                            pRaftThing = sithPlayer_aPlayers[playerNum].pThing;
                            break;
                        }
                    }

                    if ( pRaftThing )
                    {
                        rdVector_Copy3(&raftPos, &startPos);
                        raftPos.z = raftPos.z - (boardDist - sithPhysics_GetThingHeight(pRaftThing));
                        pStartSector = sithCollision_FindSectorInRadius(pStartSector, &startPos, &raftPos, 0.0f);
                        if ( !pStartSector )
                        {
#line 473
                            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "This is really bad, trying to board raft in non-existent sector..\n", 0, 0, 0, 0);
                        }

                        rdVector_Copy3(&raftOrient.uvec, &pPlayer->orient.uvec);
                        rdVector_Copy3(&raftOrient.lvec, &pPlayer->orient.rvec);
                        rdVector_Neg3(&raftOrient.rvec, &pPlayer->orient.lvec);

                        if ( pStartSector != pPlayer->pInSector )
                        {
                            bExitedSector = 0;
                        }

                        if ( bExitedSector )
                        {
                            sithCog_SectorSendMessage(pPlayer->pInSector, pPlayer, SITHCOG_MSG_EXITED);
                        }

                        if ( sithPlayer_bGuybrush )
                        {
                            sithPlayer_ToggleGuybrush();
                        }

                        sithPlayer_HidePlayer(sithPlayer_playerNum);
                        sithThing_ExitSector(pRaftThing);

                        *pRaftThing->thingInfo.actorInfo.pPlayer = *pPlayer->thingInfo.actorInfo.pPlayer;
                        pRaftThing->thingInfo.actorInfo.pPlayer->pThing = pRaftThing;
                        pRaftThing->thingInfo.actorInfo.health = pPlayer->thingInfo.actorInfo.health;

                        sithPlayer_SetLocalPlayer(playerNum);
                        sithPlayer_ShowPlayer(playerNum, 0);

                        sithThing_SetPositionAndOrient(pRaftThing, &raftPos, &raftOrient);
                        sithThing_EnterSector(pRaftThing, pStartSector, 1, bExitedSector == 0);
                        sithPuppet_SetMoveMode(pRaftThing, SITHPUPPET_MOVEMODE_NORMAL);
                        sithPhysics_FindFloor(pRaftThing, 1);
                        sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], pRaftThing, NULL);

                        pRaftThing->moveStatus = SITHPLAYERMOVE_RAFT_BOARDING;
                        sithPuppet_PlayMode(pRaftThing, SITHPUPPETSUBMODE_HOPBACK, sithVehicleControls_PuppetCallback);
                        pRaftThing->thingInfo.actorInfo.bControlsDisabled = 1;

                        sithInventory_SetSwimmingInventory(pRaftThing, 0);
                        pRaftThing->alpha = 1.0f;
                        sithInventory_SetInventoryDisabled(pRaftThing, 56, 0);

                        sithVehicleControls_SendMessageUser0(pRaftThing);
                        if ( pRaftThing->pCog )
                        {
                            sithCog_SendMessage(pRaftThing->pCog, SITHCOG_MSG_BOARDED, SITHCOG_SYM_REF_THING, pRaftThing->idx, SITHCOG_SYM_REF_THING, pPlayer->idx, 0);
                        }

                        if ( (pPlayer->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                        {
                            pRaftThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
                        }

                        if ( pRaftThing->thingInfo.actorInfo.endurance.raftLeakDamage )
                        {
                            pRaftThing->thingInfo.actorInfo.endurance.raftLeakDamage = 1;
                        }

                        sithCog_ThingSendMessageEx(pRaftThing, NULL, SITHCOG_MSG_CALLBACK, 0, 16, 0, 0);
                        return 1;
                    }
                }
            }
        }
        else if ( curItem == 56 && (pPlayer->moveInfo.physics.flags & SITH_PF_RAFT) != 0 && pPlayer->thingInfo.actorInfo.endurance.raftLeakDamage )
        {
            pPlayer->thingInfo.actorInfo.endurance.raftLeakDamage = 0;
            sithInventory_SetInventory(pPlayer, 56, sithInventory_GetInventory(pPlayer, 56) - 1.0f);
            sithSoundClass_PlayModeFirst(pPlayer, SITHSOUNDCLASS_ACTIVATE);
            sithSoundClass_StopMode(pPlayer, SITHSOUNDCLASS_JUMPWOOD);
            return 1;
        }
    }

    return bActivated;
}

void J3DAPI sithPlayerActions_Jump(SithThing* pThing, float a2, unsigned int jumpDirection)
{
    rdVector3 jumpVector;
    rdVector3 velocity;
    SithSoundClassMode soundMode;
    tSoundChannelHandle hChannel;
    int id;
    SithPhysicsInfo* pPhysics;
    float jumpSpeed;
    rdVector3 dummyVec;
    SithThing* pBaseThing;
    SithSurface* pAttachedSurf;

    soundMode     = -1;
    pPhysics = &pThing->moveInfo.physics;
    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        return;
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        return;
    }

    jumpSpeed = RD_MULTIPLY(pThing->thingInfo.actorInfo.jumpSpeed, a2);
    switch ( jumpDirection )
    {
        case -1:
            rdVector_Set3(&jumpVector, 0.0f, -1.0f, 0.0f);
            // Falls through (no break in the debug build's text)

        case 0:
            rdVector_Set3(&jumpVector, 0.0f, 0.0f, 0.0f);
            break;

        case 1:
            rdVector_Set3(&jumpVector, 0.0f, 1.0f, 0.0f);
            break;

        case 2:
            rdVector_Set3(&jumpVector, 0.0f, -1.0f, 0.0f);
            break;

        case 3:
            rdVector_Set3(&jumpVector, -1.0f, 0.0f, 0.0f);
            break;

        case 4:
            rdVector_Set3(&jumpVector, 1.0f, 0.0f, 0.0f);
            break;

        case 99:
            rdVector_Set3(&jumpVector, 0.0f, 1.0f, 0.0f);
            break;
    }

    rdVector_Set3(&velocity, 0.0f, 0.0f, 2.4f);
    if ( jumpDirection )
    {
        rdMatrix_TransformVector34(&dummyVec, &jumpVector, &pThing->orient);
        rdVector_Add3Acc(&velocity, &dummyVec);
    }

    rdVector_Normalize3Acc(&velocity);

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, soundMode, pBaseThing, pAttachedSurf);
        hChannel = sithSoundClass_PlayModeRandom(pThing, soundMode);
        id = hChannel ? Sound_GetChannelGUID(hChannel) : -1;
        if ( hChannel && sithMessage_outputstream )
        {
            sithDSSThing_PlaySoundMode(pThing, soundMode, id, -1.0f);
        }

        sithThing_DetachThing(pThing);
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
        pPhysics->angularVelocity.y = 0.0f;

        switch ( jumpDirection )
        {
            case -1:
                pThing->moveStatus = SITHPLAYERMOVE_JUMPBACK;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HOPBACK, NULL);
                sithPlayerActions_HopBack(pThing);
                break;

            case 0:
                pThing->moveStatus = SITHPLAYERMOVE_JUMPUP;
                sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_JUMPUP, NULL);
                break;

            case 1:
                pThing->moveStatus = SITHPLAYERMOVE_JUMPFWD;
                pThing->collide.movesize = 0.06f;
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPFWD, NULL);
                break;

            default:
                rdVector_Copy3(&pPhysics->velocity, &velocity);
                break;
        }
    }
    else
    {
        return;
    }

    if ( sithMessage_outputstream )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_POS);
    }
}

void J3DAPI sithPlayerActions_MoveToPlayerPosition(SithThing* pThing, unsigned int playerNum)
{
    SithPlayer* pPlayer;
    if ( playerNum >= sithPlayer_numPlayers )
    {
        return;
    }

    pPlayer = &sithPlayer_aPlayers[playerNum];
    if ( (pPlayer->flags & SITH_PLAYER_PLACED) != 0 )
    {
        rdMatrix_Copy34(&pThing->orient, &pPlayer->orient);
        rdVector_Copy3(&pThing->pos, &pThing->orient.dvec);
        pThing->orient.dvec.x = rdroid_zeroVector3.x;
        pThing->orient.dvec.y = rdroid_zeroVector3.y;
        pThing->orient.dvec.z = rdroid_zeroVector3.z;
        sithThing_SetSector(pThing, pPlayer->pInSector, 0);
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
        pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
        sithPhysics_FindFloor(pThing, 1);
    }
}

void J3DAPI sithPlayerActions_PushItem(SithThing* pThing, SithThing* pItem, rdVector3* pDirection)
{
    int trackNum;

    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->moveStatus        = SITHPLAYERMOVE_PUSHING;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    pThing->collide.movesize  = pThing->collide.movesize / 2.0f;

    trackNum = sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_PUSHITEM, NULL);
    if ( trackNum < 0 )
    {
        if ( trackNum == -1 )
        {
            pThing->thingInfo.actorInfo.bForceMovePlay = 0;
            pThing->moveStatus       = SITHPLAYERMOVE_STILL;
            pThing->collide.movesize = D3DMultiply(pThing->collide.movesize, 2.0f);
        }
    }
    else
    {
        sithAnimate_PushItem(pThing, pItem, pDirection, trackNum);
        sithPlayerControls_bCutsceneMode = 1;
    }
}

void J3DAPI sithPlayerActions_PullItem(SithThing* pThing, SithThing* pItem, rdVector3* pDirection)
{
    int trackNum;
    rdVector3 dir;

    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->moveStatus        = SITHPLAYERMOVE_PULLING;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    rdVector_Neg3(&dir, pDirection);
    pThing->collide.movesize = pThing->collide.movesize / 2.0f;

    trackNum = sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_PULLITEM, NULL);
    if ( trackNum < 0 )
    {
        if ( trackNum == -1 )
        {
            pThing->thingInfo.actorInfo.bForceMovePlay = 0;
            pThing->moveStatus       = SITHPLAYERMOVE_STILL;
            pThing->collide.movesize = D3DMultiply(pThing->collide.movesize, 2.0f);
        }
    }
    else
    {
        sithPlayerControls_bCutsceneMode = 1;
        sithAnimate_PullItem(pThing, pItem, &dir, trackNum);
    }
}

void J3DAPI sithPlayerActions_ClimbPullUp(SithThing* pThing)
{
    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    pThing->collide.movesize  = pThing->collide.movesize / 4.0f;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
    pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;

    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBPULLINGUP, NULL);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
    sithInventory_SetSwimmingInventory(pThing, 1);
}

void J3DAPI sithPlayerActions_ClimbMove(SithThing* pThing, SithSurface* pSurf, int direction)
{
    pThing->thingInfo.actorInfo.bForceMovePlay = 1;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    pThing->collide.movesize  = pThing->collide.movesize / 4.0f;

    switch ( direction )
    {
        case 1:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_UP;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLUP, NULL);
            break;

        case 2:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_DOWN;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLDOWN, NULL);
            break;

        case 3:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_RIGHT;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLRIGHT, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHORIZ);
            break;

        case 4:
            pThing->moveStatus = SITHPLAYERMOVE_CLIMBING_LEFT;
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLLEFT, NULL);
            sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBHORIZ);
            break;
    }

    sithThing_AttachThingToClimbSurface(pThing, pSurf, 0);
}

void J3DAPI sithPlayerActions_UnmountWall(SithThing* pThing, int trackNum)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    rdVector3 dir;

    pThing->moveStatus = SITHPLAYERMOVE_FALLING;
    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
    pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
    pThing->thingInfo.actorInfo.bControlsDisabled = 0;
    pThing->thingInfo.actorInfo.bForceMovePlay    = 0;
    sithPhysics_ResetThingMovement(pThing);
    rdVector_Neg3(&dir, &pThing->orient.lvec); // dir is never read (debug build text)
    sithPuppet_RemoveAllTracks(pThing);

    rdVector_Set3(&localVelocity, 0.0f, -0.075f, 0.01f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);

    sithThing_DetachThing(pThing);
    sithPhysics_FindFloor(pThing, 0);
    if ( pThing->attach.flags )
    {
        pThing->moveStatus = SITHPLAYERMOVE_STILL;
        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_STAND, NULL);
    }

    sithPhysics_SetThingLook(pThing, &rdroid_zVector3, 0.0f);
    sithInventory_SetSwimmingInventory(pThing, 1);
}

void J3DAPI sithPlayerActions_ClimbOn1m(SithThing* pThing)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus = SITHPLAYERMOVE_PULLINGUP_1M;
        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 )
        {
            pThing->moveInfo.physics.flags &= ~SITH_PF_ONWATERSURFACE;
        }

        pThing->collide.movesize  = pThing->collide.movesize / 2.0f;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

        sithThing_DetachThing(pThing);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNT1MSTEP, NULL);
        sithInventory_SetSwimmingInventory(pThing, 0);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
    }
}

void J3DAPI sithPlayerActions_ClimbOn2m(SithThing* pThing)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus        = SITHPLAYERMOVE_PULLINGUP;
        pThing->collide.movesize  = 0.01f;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

        sithThing_DetachThing(pThing);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_MOUNT2MLEDGE, NULL);
        sithInventory_SetSwimmingInventory(pThing, 0);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
    }
}

/* Discarded by the retail linker (no caller in either image), restored from the debug build (0x00461444,
   an empty EBP frame, between ClimbOn2m and LeapForward). Name and signature from OpenJones3D develop
 */
void J3DAPI sithPlayerActions_sub_461444(void)
{
}

void J3DAPI sithPlayerActions_LeapForward(SithThing* pThing)
{
    int bOnSlope;
    float dot;
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithSoundClassMode mode;
    tSoundChannelHandle hSndChannel;
    SithThing* pAttachedThing;
    SithSurface* pCurAttachedSurf;

    bOnSlope = 0;
    mode   = -1;
    if ( pThing->attach.flags )
    {
        dot = rdVector_Dot3(&pThing->attach.pFace->normal, &rdroid_zVector3);
        if ( dot < 0.8f && dot > 0.69f )
        {
            bOnSlope = 1;
        }
    }

    if ( bOnSlope )
    {
        rdVector_Set3(&localVelocity, 0.0f, 0.77f, 0.75f);
    }
    else
    {
        rdVector_Set3(&localVelocity, 0.0f, 0.77f, 0.75f);
    }

    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pCurAttachedSurf);
        hSndChannel = sithSoundClass_PlayModeFirst(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpForward(SithThing* pThing)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithSoundClassMode mode;
    SithSurface* pCurAttachedSurf;
    SithThing* pAttachedThing;
    tSoundChannelHandle snd;

    mode = -1;
    rdVector_Set3(&localVelocity, 0.0f, 0.4f, 0.75f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);
    pThing->collide.movesize = 0.08f;

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pCurAttachedSurf);
        snd = sithSoundClass_PlayModeFirst(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpLeft(SithThing* pThing)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithSoundClassMode mode;
    SithSurface* pCurAttachedSurf;
    SithThing* pAttachedThing;
    tSoundChannelHandle snd;

    mode = -1;
    rdVector_Set3(&localVelocity, -0.75f, 0.0f, 0.4f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pCurAttachedSurf);
        snd = sithSoundClass_PlayModeFirst(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpRight(SithThing* pThing)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithSoundClassMode mode;
    SithSurface* pCurAttachedSurf;
    SithThing* pAttachedThing;
    tSoundChannelHandle snd;

    mode = -1;
    rdVector_Set3(&localVelocity, 0.75f, 0.0f, 0.4f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pCurAttachedSurf);
        snd = sithSoundClass_PlayModeFirst(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpRollBack(SithThing* pThing)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithThing* pAttached;
    SithSurface* pAttachedSurf;
    rdVector3 unusedDir; // never used; the debug frame keeps its 12-byte slot
    SithSoundClassMode mode;
    tSoundChannelHandle hSound;

    mode = -1;
    rdVector_Set3(&localVelocity, 0.0f, -0.75f, 0.4f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttached, pAttachedSurf);
        hSound = sithSoundClass_PlayModeFirst(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_JumpRollForward(SithThing* pThing)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithSoundClassMode mode;
    SithSurface* pCurAttachedSurf;
    SithThing* pAttachedThing;
    tSoundChannelHandle snd;

    mode = -1;
    rdVector_Set3(&localVelocity, 0.0f, 0.75f, 0.4f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);
    pThing->moveInfo.physics.angularVelocity.y = 0.0f;

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pCurAttachedSurf);
        snd = sithSoundClass_PlayModeFirst(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_StrafeLeft(SithThing* pThing)
{
    rdVector3 dir;
    rdVector3 pos;
    int bSurfChange;

    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        rdVector_Neg3(&dir, &pThing->orient.rvec);
        rdVector_ScaleAdd3(&pos, &dir, 0.1f, &pThing->pos);
        if ( sithPlayerActions_CanMoveToPos(pThing, &pos, &bSurfChange) != 1 )
        {
            return;
        }

        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus = SITHPLAYERMOVE_STRAFING_LEFT;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_STRAFELEFT, NULL);
    }
}

void J3DAPI sithPlayerActions_StrafeRight(SithThing* pThing)
{
    rdVector3 dir;
    rdVector3 pos;
    int bSurfChange;

    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        rdVector_Copy3(&dir, &pThing->orient.rvec);
        rdVector_ScaleAdd3(&pos, &dir, 0.1f, &pThing->pos);
        if ( sithPlayerActions_CanMoveToPos(pThing, &pos, &bSurfChange) != 1 )
        {
            return;
        }

        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus = SITHPLAYERMOVE_STRAFING_RIGHT;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_STRAFERIGHT, NULL);
    }
}

void J3DAPI sithPlayerActions_HopBack(SithThing* pThing)
{
    rdVector3 localVelocity;
    rdVector3 velocity;
    SithSoundClassMode mode;
    SithSurface* pCurAttachedSurf;
    SithThing* pAttachedThing;
    tSoundChannelHandle snd;

    mode = -1;
    rdVector_Set3(&localVelocity, 0.0f, -0.35f, 0.6f);
    rdMatrix_TransformVector34(&velocity, &localVelocity, &pThing->orient);
    rdVector_Copy3(&pThing->moveInfo.physics.velocity, &velocity);

    if ( pThing->attach.flags )
    {
        SITHPLAYERACTIONS_GETJUMPSOUNDMODE(pThing, mode, pAttachedThing, pCurAttachedSurf);
        snd = sithSoundClass_PlayModeRandom(pThing, mode);
    }

    sithThing_DetachThing(pThing);
}

void J3DAPI sithPlayerActions_Stand2Crawl(SithThing* pThing)
{
    sithInventory_SetSwimmingInventory(pThing, 0);

    pThing->collide.movesize  = 0.02f;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    pThing->moveStatus        = SITHPLAYERMOVE_STAND_TO_CRAWL;

    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_CRAWL);
    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_STAND2CRAWL, NULL);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
}

void J3DAPI sithPlayerActions_Crawl2Stand(SithThing* pThing)
{
    rdVector3 curmoveNorm;
    int curCanStand;
    SithCollision* pCollision;

    curCanStand = 1;
    rdVector_Copy3(&curmoveNorm, &rdroid_zVector3);
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &curmoveNorm, 0.115f, pThing->collide.movesize, 0x200);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            curCanStand = 0;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            curCanStand = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !curCanStand )
    {
        return;
    }

    sithInventory_SetSwimmingInventory(pThing, 1);
    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALK);
    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_WALKBACK);

    pThing->collide.movesize = 0.04f;
    rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);
    pThing->moveStatus = SITHPLAYERMOVE_CRAWL_TO_STAND;

    sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CRAWL2STAND, NULL);
    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBONTO);
}

void J3DAPI sithPlayerActions_JumpStart(SithThing* pThing)
{
    pThing->moveStatus = SITHPLAYERMOVE_JUMP_READY;
    sithPhysics_ResetThingMovement(pThing);
    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_JUMPREADY, sithPlayerControls_PuppetCallback);
    sithPuppet_bPlayerLeapForward = 0;
    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
}

int J3DAPI sithPlayerActions_HasActiveWeapon(SithThing* pThing)
{
    int curWeapon;

    curWeapon = sithInventory_GetCurrentWeapon(pThing);
    if ( curWeapon )
    {
        return 1;
    }

    return sithWeapon_IsMountingWeapon(pThing);
}

void J3DAPI sithPlayerActions_ClimbToClimb(SithThing* pThing, int bLedge)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus        = SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT;
        pThing->collide.movesize  = 0.001f;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

        sithThing_DetachThing(pThing);
        if ( bLedge )
        {
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_GRABLEDGE, NULL);
        }
        else
        {
            sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBTOCLIMB, NULL);
        }

        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
        sithInventory_SetSwimmingInventory(pThing, 0);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
    }
}

void J3DAPI sithPlayerActions_ClimbToHang(SithThing* pThing)
{
    if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
    {
        pThing->thingInfo.actorInfo.bForceMovePlay = 1;
        pThing->moveStatus        = SITHPLAYERMOVE_CLIMB_TO_HANG;
        pThing->collide.movesize  = 0.001f;
        rdVector_Copy3(&pThing->forceMoveStartPos, &pThing->pos);

        sithThing_DetachThing(pThing);
        sithPuppet_PlayForceMoveMode(pThing, SITHPUPPETSUBMODE_CLIMBTOHANG, NULL);
        sithInventory_SetSwimmingInventory(pThing, 0);
        sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_CLIMBDOWNONTO);
    }
}

int J3DAPI sithPlayerActions_CanMoveToPos(SithThing* pThing, rdVector3* pPos, int* a3)
{
    int result;
    rdVector3 moveNorm;
    float radius;
    SithCollision* pCollision;
    SithSector* pSector;

    result = 0;
    rdVector_Set3(&moveNorm, 0.0f, 0.0f, -1.0f);

#line 1636
    SITH_ASSERTREL(pThing);

    radius  = D3DVAL(pThing->collide.movesize / 2.0f);
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, pPos, 0.0f);
    if ( !pSector )
    {
        return 2;
    }

    sithCollision_SearchForCollisions(pSector, pThing, pPos, &moveNorm, sithPhysics_GetThingHeight(pThing) + 0.115f, radius, 0xA00);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
#line 1651
            SITH_ASSERTREL(pCollision->pSurfaceCollided);

            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 || (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) != 0 )
            {
                float dot;

                if ( (pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_4 || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_5)
                    && (pCollision->pSurfaceCollided->flags & (SITH_SURFACE_WATER | SITH_SURFACE_SHALLOWWATER)) != 0 )
                {
                    result = 0;
                    break;
                }

                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) != 0 )
                {
                    result = 0;
                    break;
                }

                dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_zVector3);
                if ( dot < 0.8f && dot > 0.69f )
                {
                    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
                    {
                        result = 1;
                    }
                    else if ( pThing->moveStatus == SITHPLAYERMOVE_RUNNING )
                    {
                        rdVector3 norm;
                        rdVector3 dir;
                        float runDot;

                        rdVector_Copy3(&norm, &pCollision->pSurfaceCollided->face.normal);
                        norm.z = 0.0f;
                        rdVector_Normalize3Acc(&norm);

                        rdVector_Copy3(&dir, &pThing->orient.lvec);
                        dir.z = 0.0f;
                        rdVector_Normalize3Acc(&dir);

                        runDot = rdVector_Dot3(&dir, &norm);
                        if ( runDot > 0.0f )
                        {
                            result = 0;
                        }
                        else
                        {
                            result = 3;
                        }
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else if ( dot < 0.69f )
                {
                    result = 0;
                }
                else
                {
                    result = 1;
                }
            }
            else
            {
                float dot;

                dot = rdVector_Dot3(&pCollision->pSurfaceCollided->face.normal, &rdroid_zVector3);
                if ( dot < 0.63f )
                {
                    if ( pThing->moveStatus == SITHPLAYERMOVE_RUNNING )
                    {
                        rdVector3 norm;
                        rdVector3 dir;
                        float runDot;

                        rdVector_Copy3(&norm, &pCollision->pSurfaceCollided->face.normal);
                        norm.z = 0.0f;
                        rdVector_Normalize3Acc(&norm);

                        rdVector_Copy3(&dir, &pThing->orient.lvec);
                        dir.z = 0.0f;
                        rdVector_Normalize3Acc(&dir);

                        runDot = rdVector_Dot3(&dir, &norm);
                        if ( runDot > 0.0f )
                        {
                            result = 0;
                        }
                        else
                        {
                            result = 3;
                        }
                    }
                    else
                    {
                        result = 0;
                    }
                }
                else
                {
                    result = 2;
                }
            }

            if ( a3 )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) == 0 || pThing->attach.attachedToStructure.pSurfaceAttached != pCollision->pSurfaceCollided )
                {
                    *a3 = 1;
                }
                else
                {
                    *a3 = 0;
                }
            }

            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
#line 1755
            SITH_ASSERTREL(pCollision->pThingCollided);

            if ( (pCollision->pThingCollided->flags & SITH_TF_STANDON) == 0 && pCollision->pThingCollided->type != SITH_THING_ITEM )
            {
                result = 0;
                break;
            }

            result = 1;
            if ( a3 )
            {
                if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) == 0 || pThing->attach.attachedToStructure.pThingAttached != pCollision->pThingCollided )
                {
                    *a3 = 1;
                }
                else
                {
                    *a3 = 0;
                }
            }

            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LAVA) != 0 )
        {
            *a3 = 0;
            result = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return result;
}

/* Discarded by the retail linker (no caller in either image), restored from the debug build (0x00462E59,
   `xor eax, eax` in an EBP frame, between CanMoveToPos and SearchForCollision). Name and signature from
   OpenJones3D develop. */
int J3DAPI sithPlayerActions_sub_462E59(void)
{
    return 0;
}

float J3DAPI sithPlayerActions_SearchForCollision(SithThing* pThing, const rdVector3* pPYR, SithSurface** pHitSurf, rdFace** pHitFace, rdModel3Mesh** pHitMesh, SithThing** pHitThing)
{
    int bHitSurface;
    rdVector3 offset;
    SithSurface* pSurf;
    int faceHit;
    float traceRadius;
    SithCollision* pContact;
    float scanRange;
    rdVector3 lookDir;
    int sFlags;
    rdVector3 posStart;
    SithSector* pSector;
    rdFace* pLedge;
    float distance;

    bHitSurface = 0;
    faceHit    = 0;
    pSurf  = NULL;
    pLedge  = NULL;
    if ( sithPlayerActions_HasActiveWeapon(pThing) )
    {
        return -1.0f;
    }

    sFlags = 0xA00;
    rdVector_Copy3(&posStart, &pThing->pos);
    rdVector_Set3(&offset, 0.0f, 0.0f, 0.09f);
    rdVector_Add3Acc(&posStart, &offset);
    rdVector_Copy3(&lookDir, &pThing->orient.lvec);
    if ( pThing->moveStatus == SITHPLAYERMOVE_CLIMB_TO_HANG )
    {
        traceRadius = 0.005f;
    }
    else
    {
        traceRadius = 0.005f;
    }

    scanRange = 0.09f;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &posStart, 0.0f);
    if ( !pSector )
    {
        return -1.0f;
    }

    sithCollision_SearchForCollisions(pSector, pThing, &posStart, &lookDir, scanRange, traceRadius, sFlags);
    while ( (pContact = sithCollision_PopStack()) != NULL )
    {
        if ( (pContact->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pContact->pSurfaceCollided
                && (pContact->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 )
            {
                rdVector3 hitNormal;
                float dot;
                rdVector_Neg3(&hitNormal, &pContact->pSurfaceCollided->face.normal);
                dot = rdVector_Dot3(&pThing->orient.lvec, &hitNormal);
                if ( dot >= 0.949f )
                {
                    if ( sithPlayerActions_pCurCollidedSurface == pContact->pSurfaceCollided )
                    {
                        break;
                    }

                    pSurf  = sithPlayerActions_pCurCollidedSurface = pContact->pSurfaceCollided;
                    bHitSurface = 1;
                    distance    = pContact->distance;
                    if ( distance < 0.0f )
                    {
                        pSurf  = NULL;
                        bHitSurface = 0;
                    }

                    break;
                }
            }
        }
        else if ( (pContact->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pContact->pFaceCollided
                && pContact->pFaceCollided != sithPlayerActions_pCurCollidedModelFace
                && (pContact->pFaceCollided->flags & RD_FF_3DO_LEDGE) != 0 )
            {
                rdVector3 negDir;
                float faceDot;
                rdVector3 norm;

                pLedge = sithPlayerActions_pCurCollidedModelFace = pContact->pFaceCollided;
                rdMatrix_TransformVector34(&norm, &pContact->pFaceCollided->normal, &pContact->pThingCollided->orient);
                rdVector_Neg3(&negDir, &pThing->orient.lvec);
                faceDot = rdVector_Dot3(&negDir, &norm);
                if ( faceDot > 0.949f )
                {
                    faceHit = 1;
                    distance = pContact->distance;
                    if ( distance < 0.0f )
                    {
                        distance = 0.0f;
                    }

                    *pHitMesh  = pContact->pMeshCollided;
                    *pHitThing = pContact->pThingCollided;
                    sithPlayerActions_pCurCollidedModel     = pContact->pThingCollided->renderData.data.pModel3;
                    sithPlayerActions_pCurCollidedModelFace = pContact->pFaceCollided;
                    break;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bHitSurface || faceHit )
    {
        float movesize;
        int bFloorFound;

        movesize = pThing->collide.movesize;
        pThing->collide.movesize = 0.02f;
        sithPhysics_ResetThingMovement(pThing);

        bFloorFound = 0;
        rdVector_Neg3(&lookDir, &rdroid_zVector3);
        sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &lookDir, 0.13f, pThing->collide.movesize, 0xA00);
        while ( (pContact = sithCollision_PopStack()) != NULL )
        {
            if ( (pContact->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( (pContact->pSurfaceCollided->flags & SITH_SURFACE_LAVA) == 0 )
                {
                    bFloorFound = 1;
                    break;
                }
            }
            else if ( (pContact->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
            {
                if ( (pContact->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0
                    && (pContact->pSurfaceCollided->flags & SITH_SURFACE_LAVA) == 0 )
                {
                    bFloorFound = 1;
                    break;
                }
            }
            else if ( (pContact->type & SITHCOLLISION_THING) != 0
                && (pContact->pThingCollided->flags & SITH_TF_STANDON) != 0
                && pContact->pThingCollided != *pHitThing )
            {
                bFloorFound = 1;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();

        pThing->collide.movesize = movesize;
        if ( bFloorFound )
        {
            return -1.0f;
        }
    }

    if ( bHitSurface )
    {
        *pHitSurf = pSurf;
        return distance;
    }

    if ( faceHit )
    {
        *pHitFace = pLedge;
        return distance;
    }

    return -1.0f;
}

int J3DAPI sithPlayerActions_CanClimbOn1m(SithThing* pThing)
{
    float dot;
    rdVector3 startPos;
    int bDone;
    int bCanClimb;
    float hitDist;
    SithCollision* pHit;
    int sFlags;
    SithSector* pSector;
    float searchRadius;
    rdVector3 moveNorm;
    char name[256];

    bDone    = 0;
    bCanClimb = 1;
    rdVector_Copy3(&startPos, &pThing->pos);
    startPos.z = startPos.z - sithPhysics_GetThingHeight(pThing);
    startPos.z = startPos.z + 0.098f;
    rdVector_Copy3(&moveNorm, &pThing->orient.lvec);
    searchRadius = 0.01f;
    hitDist   = pThing->collide.movesize + 0.01f;
    sFlags  = 0xA00;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, pThing, &startPos, &moveNorm, hitDist, searchRadius, sFlags);
    while ( (pHit = sithCollision_PopStack()) != NULL )
    {
        if ( (pHit->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pHit->pSurfaceCollided )
            {
                dot = rdVector_Dot3(&pHit->pSurfaceCollided->face.normal, &rdroid_zVector3);
                if ( dot < 0.1f && dot > -0.1f )
                {
                    bDone = 1;
                    break;
                }
            }
        }
        else if ( (pHit->type & SITHCOLLISION_THING) != 0 )
        {
            if ( ((pHit->pFaceCollided && (pHit->pThingCollided->flags & SITH_TF_STANDON) != 0 && (pHit->pThingCollided->flags & SITH_TF_MOUNTABLE) != 0)
                || (pHit->pThingCollided->flags & SITH_TF_UNKNOWN_1000) != 0)
                && pHit->pFaceCollided->normal.z == 0.0f )
            {
                strcpy(name, pHit->pThingCollided->aName);
                if ( !strcmp(name, "20mropebridge")
                    || !strcmp(name, "10mbrokebridge")
                    || !strcmp(name, "20mholesbridge")
                    || !strcmp(name, "20mholebridge")
                    || !strcmp(name, "25mholebridge") )
                {
                    bDone = 0;
                    break;
                }
                else
                {
                    bDone = 1;
                    break;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( bDone )
    {
        searchRadius = 0.045f;
        startPos.z += 0.115f;
        hitDist = 0.071f;
        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
        if ( !pSector )
        {
            return 0;
        }

        sithCollision_SearchForCollisions(pSector, pThing, &startPos, &moveNorm, hitDist, searchRadius, sFlags);
        while ( (pHit = sithCollision_PopStack()) != NULL )
        {
            if ( (pHit->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( pHit->pSurfaceCollided )
                {
                    bCanClimb = 0;
                    break;
                }
            }
            else if ( (pHit->type & SITHCOLLISION_THING) != 0 && pHit->pFaceCollided )
            {
                bCanClimb = 0;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
    }
    else
    {
        return 0;
    }

    if ( bDone && bCanClimb )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithPlayerActions_CanClimbOn2m(SithThing* pThing)
{
    int bFound;
    int bBlocked;
    rdVector3 startPos;
    rdVector3 moveNorm;
    float searchRadius;
    float searchDist;
    int searchFlags;
    SithSector* pSector;
    SithSector* pCheckSector;
    rdVector3 checkPos;
    SithCollision* pCollision;
    float hitDist;
    int bCanClimb;

    bFound   = 0;
    bBlocked = 0;
    rdVector_Copy3(&startPos, &pThing->pos);
    startPos.z = startPos.z - sithPhysics_GetThingHeight(pThing);
    startPos.z = startPos.z + 0.15f;
    rdVector_Copy3(&moveNorm, &pThing->orient.lvec);
    searchRadius = 0.02f;
    searchDist   = pThing->collide.movesize + 0.01f;
    searchFlags  = 0xA00;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, pThing, &startPos, &moveNorm, searchDist, searchRadius, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided
                && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0
                && pCollision->distance < 0.04f )
            {
                bFound = 1;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0
            && pCollision->pThingCollided
            && (pCollision->pThingCollided->flags & SITH_TF_CLIMBCRATE) != 0
            && pCollision->distance < 0.04f )
        {
            bFound = 1;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bFound )
    {
        rdVector_Copy3(&checkPos, &startPos);
        checkPos.z += 0.15f;
        pCheckSector = sithCollision_FindSectorInRadius(pSector, &startPos, &checkPos, 0.0f);
        if ( !pCheckSector )
        {
            return 0;
        }

        searchDist   = 0.05f + pThing->collide.movesize;
        searchRadius = 0.045f;
        hitDist = sithCollision_SearchForCollisions(pCheckSector, pThing, &checkPos, &moveNorm, searchDist, searchRadius, searchFlags);
        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                if ( pCollision->pSurfaceCollided )
                {
                    bBlocked = 1;
                    break;
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                bBlocked = 1;
                break;
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    bCanClimb    = 1;
    searchDist   = 0.2f;
    searchRadius = 0.03f;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_zVector3, searchDist, searchRadius, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanClimb = 0;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pCollision->pThingCollided == pThing )
            {
                continue;
            }

            bCanClimb = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( !bCanClimb )
    {
        return 0;
    }

    if ( bFound && bBlocked )
    {
        return 0;
    }

    if ( bFound && !bBlocked )
    {
        return 1;
    }

    return bFound;
}

int J3DAPI sithPlayerActions_SearchClimbSurface(SithThing* pThing)
{
    int bAngled;
    int bBlocked;
    int bClimbable;
    int bLedge;
    int searchFlags;
    rdVector3 pos;
    rdVector3 lookDir;
    rdVector3 backDir;
    rdVector3 downPos;
    rdVector3 downDir;
    rdVector3 prevPos;
    rdVector3 negLook;
    rdVector3 normal;
    rdVector3 thingNormal;
    float normalZ;
    float dot;
    SithSector* pSector;
    SithCollision* pCollision;

    if ( sithInventory_GetCurrentWeapon(pThing) || sithWeapon_IsMountingWeapon(pThing) )
    {
        return 0;
    }

    bAngled     = 0;
    searchFlags = 0xA00;
    rdVector_Copy3(&pos, &pThing->pos);
    rdVector_Copy3(&lookDir, &pThing->orient.lvec);
    rdVector_Neg3(&backDir, &lookDir);

    bBlocked = 0;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &backDir, 0.07f, pThing->collide.movesize, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            bBlocked = 1;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bBlocked = 1;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( bBlocked )
    {
        return 0;
    }

    rdVector_Scale3(&downPos, &backDir, 0.07f);
    rdVector_Add3Acc(&downPos, &pos);
    rdVector_Neg3(&downDir, &rdroid_zVector3);
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &downPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, pThing, &downPos, &downDir, 0.25f, pThing->collide.movesize, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) == 0 )
            {
                bBlocked = 1;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bBlocked = 1;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( bBlocked )
    {
        return 0;
    }

    rdVector_Copy3(&prevPos, &downPos);
    rdVector_Scale3Acc(&downDir, 0.1f);
    rdVector_Add3Acc(&downPos, &downDir);
    pSector = sithCollision_FindSectorInRadius(pSector, &prevPos, &downPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    bClimbable = bLedge = 0;
    rdVector_Neg3(&negLook, &pThing->orient.lvec);
    sithCollision_SearchForCollisions(pSector, NULL, &downPos, &lookDir, 0.02f, 0.04f, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                rdVector_Copy3(&normal, &pCollision->pSurfaceCollided->face.normal);
                normalZ = normal.z;
                if ( normalZ > 0.1f )
                {
                    bAngled = 1;
                }

                normal.z = 0.0f;
                rdVector_Normalize3Acc(&normal);
                dot = rdVector_Dot3(&negLook, &normal);
                if ( dot > 0.949f )
                {
                    if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_LEDGE) != 0 )
                    {
                        bLedge = 1;
                        break;
                    }

                    if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
                    {
                        bClimbable = 1;
                        break;
                    }
                }
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pCollision->pFaceCollided && (pCollision->pFaceCollided->flags & RD_FF_3DO_LEDGE) != 0 )
            {
                rdMatrix_TransformVector34(&thingNormal, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);
                dot = rdVector_Dot3(&thingNormal, &negLook);
                if ( dot > 0.949f )
                {
                    bLedge = 1;
                    break;
                }
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bClimbable )
    {
        if ( bAngled )
        {
            return 3;
        }

        return 1;
    }

    if ( bLedge )
    {
        return 2;
    }

    return 0;
}

int J3DAPI sithPlayerActions_ActorGrabLedge(SithThing* pThing, float distance, SithSurface* pHitSurf, rdFace* pHitFace, const rdModel3Mesh* pHitMesh, SithThing* pHitThing)
{
    rdFace* pFace;
    rdVector3 dir;
    rdVector3 lookDir;
    rdVector3 faceNormal;
    float topZ;
    rdVector3 curPos;
    size_t i;
    float z;
    rdVector3 pos;
    float dist;
    SithSector* pSector;

    if ( pHitSurf )
    {
        rdVector_Set3(&dir, 0.0f, 0.0f, 1.0f);
        rdVector_Neg3(&lookDir, &pHitSurf->face.normal);
        rdVector_Copy3(&pThing->orient.lvec, &lookDir);
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &dir);
        rdVector_Normalize3Acc(&pThing->orient.rvec);
        rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);
    }
    else if ( pHitFace )
    {
        rdVector_Set3(&dir, 0.0f, 0.0f, 1.0f);
        rdMatrix_TransformVector34(&faceNormal, &pHitFace->normal, &pHitThing->orient);
        rdVector_Neg3(&lookDir, &faceNormal);
        rdVector_Copy3(&pThing->orient.lvec, &lookDir);
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &dir);
        rdVector_Normalize3Acc(&pThing->orient.rvec);
        rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);
    }

    pThing->moveStatus = SITHPLAYERMOVE_HANGING;
    if ( pHitSurf )
    {
        rdVector_Copy3(&curPos, &pThing->pos);
        rdVector_Copy3(&pos, &curPos);
        pFace = &pHitSurf->face;
        topZ  = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]].z;
        for ( i = 0; i < pFace->numVertices; ++i )
        {
            z = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].z;
            if ( z > topZ )
            {
                topZ = z;
            }
        }

        pos.z = topZ;
        dist = rdMath_DistancePointToPlane(&pos, &pFace->normal, &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]]);
        rdVector_ScaleAdd3(&pThing->pos, &pThing->orient.lvec, dist, &pThing->pos);
        rdVector_Neg3(&dir, &pThing->orient.lvec);
        rdVector_ScaleAdd3(&pThing->pos, &dir, 0.024f, &pThing->pos);
        pThing->pos.z = topZ - 0.118f;

        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &curPos, &pThing->pos, 0.0f);
        if ( pSector != pThing->pInSector )
        {
            sithThing_ExitSector(pThing);
            sithThing_EnterSector(pThing, pSector, 1, 0);
        }

        sithThing_AttachThingToClimbSurface(pThing, pHitSurf, 1);
        sithPlayerActions_pCurCollidedSurface = pHitSurf;
    }
    else if ( pHitFace )
    {
        rdVector3 pos;
        rdMatrix34 orient;
        rdVector3* aVertices;
        rdVector3 normal;
        rdVector3 vert;
        float dist;
        SithSector* pSector;
        rdVector3 curPos2;

        rdVector_Copy3(&curPos2, &pThing->pos);
        rdVector_Copy3(&pos, &curPos2);
        topZ = sithPlayerActions_GetFaceTopZ(pHitThing, pHitFace, pHitMesh);
        topZ = topZ + pHitThing->pos.z;
        pos.z = topZ;
        rdMatrix_Copy34(&orient, &pHitThing->orient);
        aVertices = pHitMesh->apVertices;
        rdMatrix_TransformVector34(&normal, &pHitFace->normal, &orient);
        rdMatrix_TransformVector34(&vert, &aVertices[pHitFace->aVertices[0]], &orient);
        rdVector_Add3Acc(&vert, &pHitThing->pos);

        dist = rdMath_DistancePointToPlane(&pos, &normal, &vert);
        rdVector_ScaleAdd3(&pThing->pos, &pThing->orient.lvec, dist, &pThing->pos);
        rdVector_Neg3(&dir, &pThing->orient.lvec);
        rdVector_ScaleAdd3(&pThing->pos, &dir, 0.024f, &pThing->pos);
        pThing->pos.z = topZ - 0.118f;

        pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &curPos2, &pThing->pos, 0.0f);
        if ( pSector != pThing->pInSector )
        {
            sithThing_ExitSector(pThing);
            sithThing_EnterSector(pThing, pSector, 1, 0);
        }

        sithThing_AttachThingToThingFace(pThing, pHitThing, pHitFace, pHitMesh->apVertices, 1);
        sithPlayerActions_pCurCollidedModelFace = pHitFace;
        sithPlayerActions_pCurCollidedModel     = pHitThing->renderData.data.pModel3;
    }

    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_HANGLEDGE, NULL);
    pThing->moveInfo.physics.flags &= ~SITH_PF_FLOORSTICK;
    sithInventory_SetSwimmingInventory(pThing, 0);
    return 1;
}

int J3DAPI sithPlayerActions_ClimbDownToIdle(SithThing* pThing)
{
    int searchFlags;
    SithCollision* pCollision;

    searchFlags = 0xA00;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &pThing->orient.lvec, pThing->collide.movesize, pThing->collide.movesize, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 || (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_CLIMBABLE) != 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_CLIMBIDLE;
                sithPlayerActions_MoveToClimbSector(pThing, pCollision->pSurfaceCollided);
                sithThing_AttachThingToClimbSurface(pThing, pCollision->pSurfaceCollided, 1);
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_CLIMBWALLIDLE, NULL);
                pThing->moveInfo.physics.flags &= ~SITH_PF_FLOORSTICK;
                sithInventory_SetSwimmingInventory(pThing, 1);
                break;
            }
        }
        else
        {
            continue;
        }
    }

    sithCollision_DecreaseStackLevel();
    return 1;
}

int J3DAPI sithPlayerActions_CanPullUp(SithThing* pThing)
{
    int searchFlags;
    SithSector* pSector;
    rdVector3 topPos;
    SithCollision* pCollision;
    float searchDist;
    int bNoWallAhead;
    float searchRadius;
    int bNoWallAbove;
    rdVector3 lookDir;
    int bCanPullUp;
    SithThing* pAttached;

    if ( pThing->attach.flags == SITH_ATTACH_THINGFACE )
    {
        pAttached = pThing->attach.attachedToStructure.pThingAttached;
        if ( pAttached )
        {
            if ( pAttached->moveType == SITH_MT_PHYSICS )
            {
                if ( pAttached->moveInfo.physics.velocity.x != 0.0f
                    || pAttached->moveInfo.physics.velocity.y != 0.0f
                    || pAttached->moveInfo.physics.velocity.z != 0.0f )
                {
                    return 0;
                }
            }
            else if ( pAttached->moveType == SITH_MT_PATH && (pAttached->moveInfo.pathMovement.mode & (SITH_PATHMOVE_MOVE | SITH_PATHMOVE_ROTATE)) != 0 )
            {
                return 0;
            }
        }
    }

    searchFlags  = 0xA00;
    bCanPullUp   = 1;
    searchDist   = 0.22f;
    searchRadius = 0.015f;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_zVector3, searchDist, searchRadius, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bCanPullUp = 0;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bCanPullUp = 0;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bCanPullUp = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( !bCanPullUp )
    {
        return 0;
    }

    bNoWallAhead = bNoWallAbove = 1;
    rdVector_Copy3(&topPos, &pThing->pos);
    topPos.z += 0.178f;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &topPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    searchDist   = 0.07f;
    searchRadius = 0.01f;
    rdVector_Copy3(&lookDir, &pThing->orient.lvec);
    sithCollision_SearchForCollisions(pSector, NULL, &topPos, &lookDir, searchDist, searchRadius, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bNoWallAhead = 0;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bNoWallAhead = 0;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bNoWallAhead = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( !bNoWallAhead )
    {
        return 0;
    }

    topPos.z += 0.069f;
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &topPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    searchDist   = 0.07f;
    searchRadius = 0.02f;
    rdVector_Copy3(&lookDir, &pThing->orient.lvec);
    sithCollision_SearchForCollisions(pSector, NULL, &topPos, &lookDir, searchDist, searchRadius, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided )
            {
                bNoWallAbove = 0;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            bNoWallAbove = 0;
            break;
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
        {
            bNoWallAbove = 0;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( !bNoWallAbove )
    {
        return 0;
    }

    return 1;
}

float J3DAPI sithPlayerActions_GetFaceTopZ(const SithThing* pThing, const rdFace* pFace, const rdModel3Mesh* pMesh)
{
    rdMatrix34 mat;
    int count;
    rdVector3* vertices;
    rdVector3 vert;
    float maxZ;
    float z;
    int idx;

    rdMatrix_Copy34(&mat, &pThing->orient);
    vertices   = pMesh->apVertices;
    count = pFace->numVertices;
    if ( count <= 0 )
    {
        return 0.0f;
    }

    rdMatrix_TransformVector34(&vert, &vertices[pFace->aVertices[0]], &mat);
    maxZ = vert.z;
    for ( idx = 1; idx < count; ++idx )
    {
        rdMatrix_TransformVector34(&vert, &vertices[pFace->aVertices[idx]], &mat);
        z = vert.z;
        if ( z > maxZ )
        {
            maxZ = z;
        }
    }

    return maxZ;
}

void J3DAPI sithPlayerActions_StartInvisibility(void)
{
    rdVector4 curstartColor;
    rdVector4 endColor;
    SithThing* pPlayer = sithPlayer_pLocalPlayerThing;
    if ( sithPlayerActions_bPlayerInvisible == 1 )
    {
        return;
    }

    pPlayer->flags &= ~SITH_TF_SHADOW;

    curstartColor.red   = 0.0f;
    curstartColor.green = 0.0f;
    curstartColor.blue  = 0.0f;
    curstartColor.alpha = 1.0f;
    endColor.red     = 0.0f;
    endColor.green   = 0.0f;
    endColor.blue    = 0.0f;
    endColor.alpha   = 0.1f;
    sithAnimate_StartThingFadeAnim(sithPlayer_pLocalPlayerThing, &curstartColor, &endColor, 1.0f, 0);

    sithPlayerActions_bPlayerInvisible = 1;
    if ( pPlayer )
    {
        pPlayer->thingInfo.actorInfo.flags |= SITH_AF_INVISIBLE;
    }
}

void J3DAPI sithPlayerActions_EndInvisibility(void)
{
    rdVector4 curstartColor;
    rdVector4 endColor;
    SithThing* pPlayer = sithPlayer_pLocalPlayerThing;
    if ( !sithPlayerActions_bPlayerInvisible )
    {
        return;
    }

    endColor.red     = 0.0f;
    endColor.green   = 0.0f;
    endColor.blue    = 0.0f;
    endColor.alpha   = 1.0f;
    curstartColor.red   = 0.0f;
    curstartColor.green = 0.0f;
    curstartColor.blue  = 0.0f;
    curstartColor.alpha = 0.1f;
    sithAnimate_StartThingFadeAnim(sithPlayer_pLocalPlayerThing, &curstartColor, &endColor, 1.0f, 0);

    sithPlayerActions_bPlayerInvisible = 0;
    sithFX_DestroyFairyDustDeluxe(pPlayer);
    if ( pPlayer )
    {
        pPlayer->thingInfo.actorInfo.flags &= ~SITH_AF_INVISIBLE;
    }

    pPlayer->flags |= SITH_TF_SHADOW;
}

int J3DAPI sithPlayerActions_IsInvisible(void)
{
    return sithPlayerActions_bPlayerInvisible;
}

int J3DAPI sithPlayerActions_StartJewelFlying(void)
{
    SithThing* pPlayer;
    rdKeyframe* pKey;
    int curWeapon;

    pKey    = NULL;
    pPlayer = sithPlayer_pLocalPlayerThing;
    if ( !pPlayer )
    {
        return 0;
    }

    curWeapon = sithInventory_GetCurrentWeapon(pPlayer);
    if ( curWeapon != SITHWEAPON_IMP3 )
    {
        return 0;
    }

    if ( !sithPlayerActions_bJewelFlying )
    {
        return 0;
    }

    sithPhysics_ResetThingMovement(pPlayer);
    pPlayer->moveStatus = SITHPLAYERMOVE_JEWELFLYING;
    sithThing_DetachThing(pPlayer);
    pPlayer->moveInfo.physics.flags |= SITH_PF_FLY;
    pPlayer->moveInfo.physics.flags &= ~SITH_PF_USEGRAVITY;

    if ( sithPlayerActions_pPlasma )
    {
        rdVector3 pos;
        sithPlayerActions_pPlasma->flags &= ~SITH_TF_DISABLED;
        rdVector_Copy3(&pos, &pPlayer->pos);
        pos.x = sithPlayerActions_pPlasma->pos.x;
        pos.y = sithPlayerActions_pPlasma->pos.y;
        pos.z += 0.05f;
        pPlayer->collide.type = SITH_COLLIDE_NONE;
        pPlayer->thingInfo.actorInfo.bControlsDisabled = 1;
        sithAnimate_StartThingMoveToPos(pPlayer, &pos, 1.0f);
    }

    sithInventory_SetSwimmingInventory(pPlayer, 0);

    pKey = sithPuppet_GetKeyframe("in_imp3_float.key");
    if ( pKey )
    {
        sithPlayerActions_jewelFlyingPuppetTrackNum = sithPuppet_PlayKey(pPlayer->renderData.pPuppet, pKey, 3, 4, 0, NULL);
        sithPuppet_ClearMode(pPlayer, SITHPUPPETSUBMODE_STAND);
    }

    return 1;
}

void J3DAPI sithPlayerActions_EnableJewelFlying(SithThing* pPlasma)
{
    sithPlayerActions_pPlasma      = pPlasma;
    sithPlayerActions_bJewelFlying = 1;
    if ( sithPlayer_impFireType == SITHWEAPON_IMP3 )
    {
        sithPlayerActions_StartJewelFlying();
    }
}

void J3DAPI sithPlayerActions_DisableJewelFlying(void)
{
    SithThing* pPlayer = sithPlayer_pLocalPlayerThing;
    if ( pPlayer )
    {
        switch ( pPlayer->moveStatus )
        {
            case SITHPLAYERMOVE_CRAWL_STILL:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
            case SITHPLAYERMOVE_PUSHING:
            case SITHPLAYERMOVE_PULLING:
            case SITHPLAYERMOVE_STAND_TO_CRAWL:
            case SITHPLAYERMOVE_CRAWL_TO_STAND:
                break;

            default:
                pPlayer->collide.movesize = 0.04f;
                break;
        }
    }

    if ( sithPlayerActions_pPlasma )
    {
        sithPlayerActions_pPlasma->flags |= SITH_TF_DISABLED;
    }

    sithPlayerActions_pPlasma      = NULL;
    sithPlayerActions_bJewelFlying = 0;
}

/* QuickTurnLeft/QuickTurnRight: discarded by the retail linker (no caller in either image), restored in
   full from the debug build (0x00465015, 0x004650F8; __LINE__ 2918/2925 and 2947/2954). Names from
   OpenJones3D develop. */
void J3DAPI sithPlayerActions_QuickTurnLeft(SithThing* pThing)
{
    SithAnimationSlot* pAnim;
    rdKeyframe* pKey = NULL;

    if ( !pThing )
    {
#line 2918
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing pointer to QuickTurnLeft()", 0, 0, 0, 0);
        return;
    }

    pAnim = sithAnimate_StartThingQuickTurnAnim(pThing, 0);
    if ( !pAnim )
    {
#line 2925
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not start quick-turn sith animation on %s!", pThing->aName, 0, 0, 0);
        return;
    }

    pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
    pThing->moveStatus = SITHPLAYERMOVE_TURNING_LEFT_90_DEGREES;

    pKey = sithPuppet_GetKeyframe("in_rotate90_left.key");
    if ( pKey )
    {
        sithPuppet_PlayKey(pThing->renderData.pPuppet, pKey, 1, 2, RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, NULL);
    }
}

void J3DAPI sithPlayerActions_QuickTurnRight(SithThing* pThing)
{
    SithAnimationSlot* pAnim;
    rdKeyframe* pKey = NULL;

    if ( !pThing )
    {
#line 2947
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad thing pointer to QuickTurnRight()", 0, 0, 0, 0);
        return;
    }

    pAnim = sithAnimate_StartThingQuickTurnAnim(pThing, 1);
    if ( !pAnim )
    {
#line 2954
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not start quick-turn sith animation on %s!", pThing->aName, 0, 0, 0);
        return;
    }

    pThing->thingInfo.actorInfo.flags |= SITH_AF_CONTROLSDISABLED;
    pThing->moveStatus = SITHPLAYERMOVE_TURNING_RIGHT_90_DEGREES;

    pKey = sithPuppet_GetKeyframe("in_rotate90_right.key");
    if ( pKey )
    {
        sithPuppet_PlayKey(pThing->renderData.pPuppet, pKey, 1, 2, RDKEYFRAME_DISABLE_FADEIN | RDKEYFRAME_NOLOOP, NULL);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(sithPlayerActions_countfit_2, 30, 3) CF_PROTO(sithPlayerActions_countfit_5, 16) /* end count-fit */
void J3DAPI sithPlayerActions_MoveToClimbSector(SithThing* pThing, SithSurface* pSurface)
{
    SithSector* pSector;
    rdVector3 newPos;
    rdVector3 pos;
    rdVector3 offset;  // unreferenced; the debug build's frame keeps its slot
    float dy;
    float dx;
    float centerY;
    float centerX;
    int i;
    float maxY;
    float minY;
    float maxX;
    float minX;
    rdFace* pFace;
    size_t numVertices;
    rdVector3 right;
    rdVector3 normal;

    memcpy(&normal, &pSurface->face.normal, sizeof(rdVector3));
    normal.z = 0.0f;
    rdVector_Normalize3Acc(&normal);
    rdVector_Cross3(&right, &normal, &rdroid_zVector3);

    numVertices = pSurface->face.numVertices;
    pFace = &pSurface->face;
    maxX = minX = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]].x;
    maxY = minY = sithWorld_pCurrentWorld->aVertices[pFace->aVertices[0]].y;
    for ( i = 0; i < (int)numVertices; ++i )
    {
        maxX = J3DMAX(maxX, sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].x);
        minX = J3DMIN(minX, sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].x);
        maxY = J3DMAX(maxY, sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].y);
        minY = J3DMIN(minY, sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].y);
    }

    centerX = D3DVAL((maxX - minX) / 2.0f) + minX; /* spelling-fit */
    centerY = D3DVAL((maxY - minY) / 2.0f) + minY; /* spelling-fit */
    dx = centerX - pThing->pos.x;
    dy = centerY - pThing->pos.y;

    pos.x = pThing->pos.x + dx;
    pos.y = pThing->pos.y + dy;
    pos.z = pThing->pos.z;
    rdVector_ScaleAdd3(&newPos, &normal, 0.05f, &pos);

    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);
    if ( !pSector )
    {
#line 3024
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "can't move to valid sector centering on climb face!", 0, 0, 0, 0);
        return;
    }

    if ( pSector == pThing->pInSector )
    {
        memcpy(&pThing->pos, &newPos, sizeof(rdVector3));
    }
    else
    {
        memcpy(&pThing->pos, &newPos, sizeof(rdVector3));
        sithThing_ExitSector(pThing);
        sithThing_EnterSector(pThing, pSector, 1, 1);
    }
}

void J3DAPI sithPlayerActions_Crawl(SithThing* pThing)
{
    rdVector3 moveDir;
    float dist;
    float moveDistance;
    SithCollision* pCollision;
    SithSector* pNewSector;
    rdVector3 point;

    rdVector_Neg3(&moveDir, &rdroid_zVector3);
    dist = 0.0f;
    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveDir, 0.2f, 0.0f, 0xA00);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && pCollision->pSurfaceCollided
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            dist = pCollision->distance;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    moveDistance = dist - 0.045f;
    if ( moveDistance <= 0.0f )
    {
        return;
    }

    rdVector_Scale3(&point, &moveDir, moveDistance);
    rdVector_Add3Acc(&point, &pThing->pos);
    pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &point, 0.0f);
    rdVector_Copy3(&pThing->pos, &point);
    if ( pNewSector != pThing->pInSector )
    {
        sithThing_ExitSector(pThing);
        sithThing_EnterSector(pThing, pNewSector, 0, 0);
    }
}
