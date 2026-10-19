#include "sithActor.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#ifdef J3D_DEBUG
#include <sith/Devices/sithConsole.h>
#endif
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <sound/Sound.h>

#include <std/General/stdMath.h>
#include <std/Win95/stdComm.h>

#include <stdlib.h>

void J3DAPI sithActor_PlayDamageSoundFx(SithThing* pThing, uint32_t damageType);

float sithActor_flt_53929C = 0.6f;
float sithActor_flt_5392A0 = 1.2f;

void J3DAPI sithActor_SetDifficulty(SithThing* pActor)
{
    if ( !pActor || pActor->type != SITH_THING_ACTOR )
    {
#line 97
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid ACTOR thing passed to sithActor_SetDifficulty().\n", 0, 0, 0, 0);
        return;
    }

    if ( pActor->controlType == SITH_CT_AI && pActor->controlInfo.aiControl.pClass )
    {
        float health;
        float maxHealth;
        if ( pActor->pTemplate )
        {
            float healthRatio = D3DDivide(pActor->thingInfo.actorInfo.health, pActor->thingInfo.actorInfo.maxHealth);
            maxHealth = RD_MULTIPLY(pActor->pTemplate->thingInfo.actorInfo.maxHealth, sithGetHitAccuarancyScalar());
            health = RD_MULTIPLY(healthRatio, maxHealth);
        }
        else
        {
            maxHealth = RD_MULTIPLY(pActor->thingInfo.actorInfo.maxHealth, sithGetHitAccuarancyScalar());
            health = RD_MULTIPLY(pActor->thingInfo.actorInfo.health, sithGetHitAccuarancyScalar());
        }

        pActor->thingInfo.actorInfo.maxHealth = STDMATH_CLAMP(maxHealth, 1.0f, 1000.0f);
        pActor->thingInfo.actorInfo.health = STDMATH_CLAMP(health, 1.0f, 1000.0f);
    }
}

void J3DAPI sithActor_Update(SithThing* pThing, unsigned int msecDeltaTime)
{
    unsigned int msecUnderwater;

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) != 0 || (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
#ifdef J3D_DEBUG
        if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            sithConsole_PrintString("Uh Oh. Raft underwater.");
        }
#endif
        msecUnderwater = pThing->thingInfo.actorInfo.endurance.msecUnderwater;
        msecUnderwater += msecDeltaTime;
        pThing->thingInfo.actorInfo.endurance.msecUnderwater += msecDeltaTime;
        if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater >= SITHACTOR_MAX_UNDERWATER_MSEC )
        {
            float damage = D3DVAL(msecDeltaTime) / 5.0f;
            sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_DROWN);
            pThing->thingInfo.actorInfo.endurance.msecUnderwater = SITHACTOR_MAX_UNDERWATER_MSEC;
        }
    }
    else if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) == 0 )
    {
        // actor is swimming afloat (on water surface) in water

        if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater > SITHACTOR_MAX_UNDERWATER_MSEC - 10000 )
        {
            if ( SITH_ISFRAMECYCLE(pThing->idx, 16) ) // On every 16th frame
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_GASP);
            }
            else if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater > SITHACTOR_MAX_UNDERWATER_MSEC / 2 // TODO: ???, verify this if statement
                && SITH_ISFRAMECYCLE(pThing->idx, 8) ) // On every 8th frame
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_BREATH);
            }
        }

        pThing->thingInfo.actorInfo.endurance.msecUnderwater -= 8 * msecDeltaTime;
        if ( pThing->thingInfo.actorInfo.endurance.msecUnderwater < 0 || pThing->thingInfo.actorInfo.endurance.msecUnderwater > SITHACTOR_MAX_UNDERWATER_MSEC ) // TODO: ???
        {
            pThing->thingInfo.actorInfo.endurance.msecUnderwater = 0;
        }
    }
    else if ( pThing->thingInfo.actorInfo.endurance.raftLeakDamage
        && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0
        && pThing->moveStatus != SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT
        && pThing->moveStatus != SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT
        && pThing->moveStatus != SITHPLAYERMOVE_RAFT_UNBOARD_START
        && pThing->moveStatus != SITHPLAYERMOVE_RAFT_BOARDING )
    {
        // Update Raft damage

        float rate   = (sithActor_flt_53929C - sithActor_flt_5392A0) / 5.0f;
        float damage = RD_MULTIPLY(D3DVAL(sithGetGameDifficulty()), rate) + sithActor_flt_5392A0;
        float leakDamage = D3DVAL(msecDeltaTime) / damage;
        sithThing_DamageThing(pThing, pThing, leakDamage, SITH_DAMAGE_RAFT_LEAK);
    }

    if ( pThing->thingInfo.actorInfo.pThingMeshAttached )
    {
        rdMatrix34 meshOrient;

        SithThing* pThingMeshAttached = pThing->thingInfo.actorInfo.pThingMeshAttached;
        int nodeNum = pThing->thingInfo.actorInfo.attachMeshNum;// TODO: [BUG] the meshNum and the actual model nodeNum can have different number

        int bSkipBuildingJoints = pThingMeshAttached->renderData.bSkipBuildingJoints;
        pThingMeshAttached->renderData.bSkipBuildingJoints = 1;

        
        rdModel3_GetMeshMatrix(&pThingMeshAttached->renderData, &pThingMeshAttached->orient, nodeNum, &meshOrient);
        pThingMeshAttached->renderData.bSkipBuildingJoints = bSkipBuildingJoints;

        rdVector_Copy3(&pThing->pos, &meshOrient.dvec);
        rdMatrix_Copy34(&pThing->orient, &meshOrient);
        sithThing_SetSector(pThing, pThingMeshAttached->pInSector, 1);
    }

    sithVoice_UpdateLipSync(pThing);
}

float J3DAPI sithActor_DamageActor(SithThing* pActor, SithThing* pThing, float damage, SithDamageType damageType)
{
    int bPlayedDamageSound = 0;

#line 262
    SITH_ASSERTREL(pActor && (damage > ((float)0.0)));
    SITH_ASSERTREL(pActor->type == SITH_THING_ACTOR || pActor->type == SITH_THING_PLAYER);

    if ( pActor->type == SITH_THING_PLAYER
        && (pActor->moveStatus == SITHPLAYERMOVE_RAFT_BOARDING
            || pActor->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT
            || pActor->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT
            || pActor->moveStatus == SITHPLAYERMOVE_RAFT_UNBOARD_START
            || pActor->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING
            || pActor->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT
            || pActor->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
            || pActor->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING
            || pActor->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING) )
    {
        return 0.0f;
    }

    if ( !stdComm_IsGameActive() || (pActor->flags & SITH_TF_REMOTE) == 0 )
    {
        if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 && damageType != SITH_DAMAGE_CRUSH )
        {
            if ( !bPlayedDamageSound && pThing->pParent && pThing->pParent->type == SITH_THING_PLAYER )
            {
                sithActor_PlayDamageSoundFx(pActor, damageType);
            }

            return 0.0f;
        }

        if ( pActor->thingInfo.actorInfo.health <= 0.0f )
        {
            return damage;
        }

        if ( pActor->type == SITH_THING_PLAYER )
        {
            damage = sithInventory_BroadcastMessage(pActor, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_MSG_DAMAGED, SITHINVENTORY_TYPE_DAMAGEABLE, damage, damageType, 0, 0);
            if ( damage == 0.0f )
            {
                return 0.0f;
            }
        }

        pActor->thingInfo.actorInfo.health = pActor->thingInfo.actorInfo.health - damage;
        if ( !bPlayedDamageSound )
        {
            sithActor_PlayDamageSoundFx(pActor, damageType);
        }

        if ( pThing )
        {
            if ( pThing != pActor && pActor->controlType == SITH_CT_AI )
            {
                sithAI_EmitEvent(pActor->controlInfo.aiControl.pLocal, SITHAI_EVENT_HIT_SECTOR, (void*)pThing);
            }

            pThing = sithThing_GetThingParent(pThing);
        }

        if ( pActor->thingInfo.actorInfo.health < 1.0f )
        {
            // Actor died

            if ( sithMessage_outputstream )
            {
                sithDSSThing_Death(pActor, pThing, 0, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }

            sithActor_KillActor(pActor, pThing, damageType);
            return damage - pActor->thingInfo.actorInfo.health;
        }
    }

    if ( pActor->pPuppetClass && pActor != pThing && SITH_RAND() < RD_MULTIPLY(damage, 0.05f) )
    {
        sithPuppet_PlayMode(pActor, SITHPUPPETSUBMODE_HIT, NULL);
    }

    return damage;
}

float J3DAPI sithActor_DamageRaftActor(SithThing* pActor, SithThing* pPerpetrator, float damage, SithDamageType damageType)
{
#line 379
    SITH_ASSERTREL(pActor && (damage > ((float)0.0)));
    SITH_ASSERTREL(pActor->type == SITH_THING_ACTOR || pActor->type == SITH_THING_PLAYER);

    if ( !stdComm_IsGameActive() || (pActor->flags & SITH_TF_REMOTE) == 0 )
    {
        if ( (pActor->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 && damageType != SITH_DAMAGE_CRUSH )
        {
            return 0.0f;
        }

        if ( pActor->thingInfo.actorInfo.endurance.raftLeakDamage >= SITHACTOR_MAX_RAFT_DAMAGE )
        {
            return damage;
        }

        if ( pActor->type == SITH_THING_PLAYER )
        {
            damage = sithInventory_BroadcastMessage(pActor, SITHCOG_SYM_REF_THING, pPerpetrator->idx, SITHCOG_MSG_DAMAGED, SITHINVENTORY_TYPE_DAMAGEABLE, damage, damageType, 0, 0);
            if ( damage == 0.0f )
            {
                return 0.0f;
            }
        }

        pActor->thingInfo.actorInfo.endurance.raftLeakDamage += (uint32_t)damage;
        if ( (damageType & SITH_DAMAGE_RAFT_LEAK) != 0 )
        {
            sithSoundClass_PlayModeFirst(pActor, SITHSOUNDCLASS_JUMPWOOD);
        }
        else
        {
            sithSoundClass_PlayModeFirst(pActor, SITHSOUNDCLASS_HITDAMAGED);
        }

        if ( pActor->thingInfo.actorInfo.endurance.raftLeakDamage >= SITHACTOR_MAX_RAFT_DAMAGE )
        {
            // Raft actor died

            if ( sithMessage_outputstream )
            {
                sithDSSThing_Death(pActor, pPerpetrator, 0, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }

            sithActor_KillActor(pActor, pPerpetrator, damageType);
            return damage;
        }
    }

    if ( pActor->pPuppetClass && pActor != pPerpetrator && SITH_RAND() < RD_MULTIPLY(damage, 0.05f) )
    {
        sithPuppet_PlayMode(pActor, SITHPUPPETSUBMODE_HIT, NULL);
    }

    return damage;
}

void J3DAPI sithActor_PlayDamageSoundFx(SithThing* pThing, uint32_t damageType)
{
    switch ( damageType )
    {
        case SITH_DAMAGE_ENERGY:
        case SITH_DAMAGE_LIGHTNING:
        case SITH_DAMAGE_LASER:
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTENERGY);
            break;

        case SITH_DAMAGE_FIRE:
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTFIRE);
            break;

        case SITH_DAMAGE_FISTS:
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTFISTS);
            break;

        case SITH_DAMAGE_DROWN:
            if ( SITH_ISFRAMECYCLE(pThing->idx, 16) ) // On every 16th frame make drowning sound fx
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DROWNING);
            }
            break;

        case SITH_DAMAGE_BONK:
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTIMPACT);
            break;

        case SITH_DAMAGE_MACHETE:
            // Damage fx for machete
            if ( !sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTMACHETE) )
            {
                tSoundHandle hSnd;
                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_DROID) != 0 )
                {
                    hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(81)); // gen_machete_hit_stone.wav
                }
                else
                {
                    hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(80)); // gen_machete_hit_flesh.wav
                }

                sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
            }
            break;

        case SITH_DAMAGE_POISON:
        case SITH_DAMAGE_IMP1:
        case SITH_DAMAGE_2000:
        case (SITH_DAMAGE_2000 | SITH_DAMAGE_IMP1):
        case SITH_DAMAGE_IMP4:
        case SITH_DAMAGE_IMP5:
        case SITH_DAMAGE_IMP_BLAST:
            break;

        default:
            if ( pThing->type == SITH_THING_PLAYER && ((pThing->flags & SITH_TF_SUBMERGED) != 0 || SITH_RAND() > 0.3f) )
            {
                break;
            }

            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTIMPACT);
            break;
    }
}

void J3DAPI sithActor_KillActor(SithThing* pThing, SithThing* pSrcThing, SithDamageType damageType)
{
    float curHealth;
    rdPuppetTrackCallback pfCallback = NULL;

    if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    curHealth = pThing->thingInfo.actorInfo.health;
    pThing->thingInfo.actorInfo.health = 0.0f;

    sithCog_ThingSendMessageEx(pThing, pSrcThing, SITHCOG_MSG_KILLED, 0, damageType, 0, 0);

    if ( (pThing->flags & SITH_TF_DESTROYED) != 0 )
    {
        return;
    }

    sithSoundMixer_StopSoundThing(pThing, 0);

    if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) == 0 && pThing->moveStatus != SITHPLAYERMOVE_RUNOVER )
    {
        if ( damageType == SITH_DAMAGE_DROWN )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DROWNED);
        }

        else if ( damageType == SITH_DAMAGE_CRUSH )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_SPLATTERED);
        }

        else if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATHUNDER);
        }

        else if ( curHealth < -10.0f )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATH2);
        }
        else
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_DEATH1);
        }
    }

    sithActor_SetHeadPYR(pThing, &rdroid_zeroVector3);
    sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 0, 2.0f, pSrcThing);

    if ( pThing->type == SITH_THING_PLAYER )
    {
        sithPlayer_PlayerKilledAction(pThing, pSrcThing);
    }

    pThing->moveInfo.physics.flags &= ~SITH_PF_CROUCHING;

    pThing->flags |= SITH_TF_DYING;

    pThing->flags &= ~SITH_TF_SHADOW;

    if ( pThing->type == SITH_THING_PLAYER )
    {
        switch ( pThing->moveStatus )
        {
            case SITHPLAYERMOVE_HANGING:
            case SITHPLAYERMOVE_CLIMBIDLE:
            case SITHPLAYERMOVE_CLIMBING_UP:
            case SITHPLAYERMOVE_CLIMBING_DOWN:
            case SITHPLAYERMOVE_CLIMBING_LEFT:
            case SITHPLAYERMOVE_CLIMBING_RIGHT:
                sithThing_DetachThing(pThing);
                pThing->moveInfo.physics.flags |= SITH_PF_FLOORSTICK;
                pThing->collide.movesize = 0.04f;
                break;

            case SITHPLAYERMOVE_CRAWL_STILL:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                sithPuppet_RemoveAllTracks(pThing);
                pThing->collide.movesize = 0.04f;
                break;

            default:
                break;
        }
    }

    if ( pThing->type == SITH_THING_ACTOR && pThing->controlType == SITH_CT_AI )
    {
        pfCallback = sithAIMove_PuppetCallback;
        switch ( pThing->moveStatus )
        {
            case SITHPLAYERMOVE_ROLLING_LEFT:
            case SITHPLAYERMOVE_ROLLING_RIGHT:
                sithPuppet_SetArmedMode(pThing, 1u);
                break;

            default:
                break;
        }
    }

    if ( pThing->pPuppetClass && pThing->moveStatus != SITHPLAYERMOVE_RUNOVER )
    {
        sithPuppet_RemoveAllTracks(pThing);
        if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            if ( pThing->thingInfo.actorInfo.endurance.raftLeakDamage >= SITHACTOR_MAX_RAFT_DAMAGE )
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH2, sithVehicleControls_PuppetCallback);
                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
            }
            else
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH, NULL);
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH, sithVehicleControls_PuppetCallback);
            pThing->thingInfo.actorInfo.bControlsDisabled = 1;
        }
        else
        {
            if ( damageType == SITH_DAMAGE_COLD_WATER )
            {
                pThing->pPuppetState->majorMode = pThing->pPuppetState->armedMode + 8; // Unarmed swimming
                curHealth = -31.0f;
            }

            if ( curHealth < -30.0f && pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][SITHPUPPETSUBMODE_DEATH2].pKeyframe )
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH2, pfCallback);
            }
            else
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_DEATH, pfCallback);
            }
        }
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        return;
    }

    if ( pThing->controlType == SITH_CT_AI )
    {
        sithAI_Stop(pThing);
    }

    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_EXPLODE_WHEN_KILLED) != 0 && pThing->thingInfo.actorInfo.pExplodeTemplate )
    {
        sithThing_CreateThingAtPos(pThing->thingInfo.actorInfo.pExplodeTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, NULL);
        sithThing_DestroyThing(pThing);
    }
    else
    {
        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_BREATHEUNDERWATER) != 0 )
        {
            pThing->moveInfo.physics.buoyancy = 0.3f;
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_FLY) != 0 )
        {
            sithActor_DestroyActor(pThing);
        }
        else
        {
            pThing->msecLifeLeft = 3000;
        }
    }
}

int J3DAPI sithActor_SurfaceCollisionHandler(SithThing* pThing, SithSurface* pSurface, SithCollision* pHitStack)
{
    int bCollided;

    if ( pThing->controlType != SITH_CT_AI )
    {
        return 0;
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 && pThing->collide.movesize > 0.3f )
    {
        return 0;
    }

    bCollided = sithCollision_HandleThingHitSurface(pThing, pSurface, pHitStack);
    if ( bCollided )
    {
        if ( (pSurface->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_HIT_FLOOR, pSurface);
        }
        else
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_HIT_WALL, pSurface);
        }
    }

    return bCollided;
}

int J3DAPI sithActor_ActorCollisionHandler(SithThing* pSrcThing, SithThing* pThing, SithCollision* pCollision, int bSecondThingIsSource)
{
    int bCollision = sithCollision_ThingCollisionHandler(pSrcThing, pThing, pCollision, bSecondThingIsSource);
    if ( !bCollision )
    {
        return 0;
    }

    if ( pSrcThing->controlType == SITH_CT_AI && pSrcThing->controlInfo.aiControl.pLocal )
    {
        sithAI_EmitEvent(pSrcThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_TOUCHED, pThing);
    }

    if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
    {
        sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_TOUCHED, pSrcThing);
    }

    return bCollision;
}

void J3DAPI sithActor_SetHeadPYR(SithThing* pThing, const rdVector3* headAngles)
{
#line 835
    SITH_ASSERTREL(pThing && headAngles && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));

    pThing->thingInfo.actorInfo.flags &= ~SITH_AF_VIEWCENTRED;

    rdVector_Copy3(&pThing->thingInfo.actorInfo.headPYR, headAngles);

    if ( pThing->pPuppetClass && pThing->renderData.type == RD_THING_MODEL3 && pThing->renderData.apTweakedAngles )
    {
        int lastNodeNum = pThing->renderData.data.pModel3->numHNodes - 1;

        int neckCount = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_NECK];
        int hipNum  = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_HIP];
        int aim1Num = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIM1];
        int aim2Count = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIM2];

        if ( J3DINRANGE(neckCount, 0, lastNodeNum) )
        {
            pThing->renderData.apTweakedAngles[neckCount].pitch = RD_MULTIPLY(headAngles->pitch, 0.5f);
            pThing->renderData.apTweakedAngles[neckCount].yaw = headAngles->yaw;
        }

        if ( J3DINRANGE(hipNum, 0, lastNodeNum) )
        {
            pThing->renderData.apTweakedAngles[hipNum].pitch = RD_MULTIPLY(headAngles->pitch, 0.5f);
        }

        if ( J3DINRANGE(aim1Num, 0, lastNodeNum) )
        {
            pThing->renderData.apTweakedAngles[aim1Num].pitch = RD_MULTIPLY(headAngles->pitch, 0.3f);
        }

        if ( J3DINRANGE(aim2Count, 0, lastNodeNum) )
        {
            pThing->renderData.apTweakedAngles[aim2Count].pitch = RD_MULTIPLY(headAngles->pitch, 0.3f);
        }
    }
}

/* Linker-discarded (no retail symbol; debug build 0x004B0EF5, assert line 889): applies the actor's head
   pitch and yaw to the puppet's aim joints. Called by sithAIInstinct_TurretFire (itself discarded). The name is
   OpenJones3D develop's (inferred). */
void J3DAPI sithActor_UpdateAimJoints(SithThing* pThing)
{
    int pitchNum;
    int yawNum;

#line 889
    SITH_ASSERT(pThing && ((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_CORPSE)));
    if ( pThing->pPuppetClass )
    {
        pitchNum = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIMPITCH];
        yawNum = pThing->pPuppetClass->aJoints[SITHPUPPET_JOINTINDEX_AIMYAW];
        if ( pitchNum >= 0 )
        {
            pThing->renderData.apTweakedAngles[pitchNum].pitch = pThing->thingInfo.actorInfo.headPYR.pitch;
        }

        if ( yawNum >= 0 )
        {
            pThing->renderData.apTweakedAngles[yawNum].yaw = pThing->thingInfo.actorInfo.headPYR.yaw;
        }
    }
}

void J3DAPI sithActor_DestroyActor(SithThing* pActor)
{

    pActor->flags |= SITH_TF_DYING;

    sithThing_DetachAttachedThings(pActor);

    pActor->moveInfo.physics.flags |= SITH_PF_FLOORSTICK | SITH_PF_ALIGNSURFACE | SITH_PF_USEGRAVITY;
    pActor->moveInfo.physics.flags &= ~(SITH_PF_FLY | SITH_PF_ALIGNUP | SITH_PF_ALIGNED | SITH_PF_WALLSTICK);

    pActor->type = SITH_THING_CORPSE;

    pActor->msecLifeLeft = 30000; // corpse will be visible for 30 sec
    sithPhysics_FindFloor(pActor, 0);
}

void J3DAPI sithActor_DestroyCorpse(SithThing* pThing)
{
    // Thing should be of a type SITH_THING_CORPSE, set in sithActor_DestroyActor
    // Fyi, here we can make the actor corpse to not disappear
    if ( pThing->renderFrame + 1 == sithMain_frameNumber )
    {
        // In camera view, extend life for 3 sec
        pThing->msecLifeLeft = 3000;
    }
    else
    {
        sithThing_DestroyThing(pThing);
    }
}

int J3DAPI sithActor_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    SithActorInfo* pActorInfo = &pThing->thingInfo.actorInfo;
    float curvalue;
    rdVector4 headLight;
    switch ( adjNum )
    {
        case SITHTHING_ARG_WEAPON:
        {
            pActorInfo->pWeaponTemplate = sithTemplate_GetTemplate(pArg->argValue);
            break;
        }
        case SITHTHING_ARG_HEALTH:
        {
            curvalue = D3DVAL(atof(pArg->argValue));
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->health = curvalue;
            pActorInfo->maxHealth = J3DMAX(pActorInfo->maxHealth, pActorInfo->health);
            break;
        }
        case SITHTHING_ARG_MAXHEALTH:
        {
            curvalue = D3DVAL(atof(pArg->argValue));
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->maxHealth = curvalue;
            break;
        }
        case SITHTHING_ARG_MAXTHRUST:
        {
            curvalue = D3DVAL(atof(pArg->argValue));
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->maxThrust = curvalue;
            break;
        }
        case SITHTHING_ARG_MAXROTTHRUST:
        {
            curvalue = atof(pArg->argValue);
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->maxRotVelocity = curvalue;
            break;
        }
        case SITHTHING_ARG_MAXHEADVEL:
        {
            curvalue = atof(pArg->argValue);
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->maxHeadVelocity = curvalue;
            break;
        }
        case SITHTHING_ARG_MAXHEADYAW:
        {
            curvalue = atof(pArg->argValue);
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->maxHeadYaw = curvalue;
            break;
        }
        case SITHTHING_ARG_JUMPSPEED:
        {
            curvalue = atof(pArg->argValue);
            if ( curvalue < 0.0f )
            {
                goto error;
            }

            pActorInfo->jumpSpeed = curvalue;
            break;
        }
        case SITHTHING_ARG_TYPEFLAGS:
        {
            SithActorFlag actorflags;
            if ( sscanf(pArg->argValue, "%x", &actorflags) != 1 )
            {
                goto error;
            }

            pActorInfo->flags = actorflags;
            break;
        }
        case SITHTHING_ARG_EYEOFFSET:
        {
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pActorInfo->eyeOffset.x, &pActorInfo->eyeOffset.y, &pActorInfo->eyeOffset.z) != 3 )
            {
                goto error;
            }
            break;
        }
        case SITHTHING_ARG_MINHEADPITCH:
        {
            if ( sscanf(pArg->argValue, "%f", &curvalue) != 1 )
            {
                goto error;
            }

            pActorInfo->minHeadPitch = curvalue;
            break;
        }
        case SITHTHING_ARG_MAXHEADPITCH:
        {
            if ( sscanf(pArg->argValue, "%f", &curvalue) != 1 )
            {
                goto error;
            }

            pActorInfo->maxHeadPitch = curvalue;
            break;
        }
        case SITHTHING_ARG_FIREOFFSET:
        {
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pActorInfo->fireOffset.x, &pActorInfo->fireOffset.y, &pActorInfo->fireOffset.z) != 3 )
            {
                goto error;
            }
            break;
        }
        case SITHTHING_ARG_LIGHTOFFSET:
        {
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pActorInfo->lightOffset.x, &pActorInfo->lightOffset.y, &pActorInfo->lightOffset.z) != 3 )
            {
                goto error;
            }

            pThing->flags |= SITH_TF_EMITLIGHT;
            break;
        }
        case SITHTHING_ARG_LIGHTINTENSITY:
        {
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &headLight.red, &headLight.green, &headLight.blue) != 3 )
            {
                goto error;
            }

            // TODO: Add additional format option with light range (see thing light param)
            rdVector_Copy4(&pActorInfo->headLightIntensity, &headLight);
            pThing->flags |= SITH_TF_EMITLIGHT;
            break;
        }
        case SITHTHING_ARG_EXPLODE:
        {
            pActorInfo->pExplodeTemplate = sithTemplate_GetTemplate(pArg->argValue);
            break;
        }
        case SITHTHING_ARG_VOICECOLOR:
        {
            if ( sscanf(pArg->argValue, "(%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f)", &pActorInfo->voiceInfo.voiceColor.top.red, &pActorInfo->voiceInfo.voiceColor.top.green, &pActorInfo->voiceInfo.voiceColor.top.blue, &pActorInfo->voiceInfo.voiceColor.top.alpha, &pActorInfo->voiceInfo.voiceColor.middle.red, &pActorInfo->voiceInfo.voiceColor.middle.green, &pActorInfo->voiceInfo.voiceColor.middle.blue, &pActorInfo->voiceInfo.voiceColor.middle.alpha, &pActorInfo->voiceInfo.voiceColor.bottomLeft.red, &pActorInfo->voiceInfo.voiceColor.bottomLeft.green, &pActorInfo->voiceInfo.voiceColor.bottomLeft.blue, &pActorInfo->voiceInfo.voiceColor.bottomLeft.alpha, &pActorInfo->voiceInfo.voiceColor.bottomRight.red, &pActorInfo->voiceInfo.voiceColor.bottomRight.green, &pActorInfo->voiceInfo.voiceColor.bottomRight.blue, &pActorInfo->voiceInfo.voiceColor.bottomRight.alpha) != 16 )
            {
                goto error;
            }
            break;
        }
        default:
            return 0;
    }

    return 1;

error:
#line 1093
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Invalid argument '%s=%s' line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber(), 0);
    return 0;
}

