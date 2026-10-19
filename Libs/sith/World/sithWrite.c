#include "sithWrite.h"

#include <rdroid/Math/rdMatrix.h>

#include <sith/AI/sithAIClass.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithSound.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithPVS.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>

#include <string.h>

#line 75
int J3DAPI sithWrite_Write(const SithWorld* pWorld, const char* pFilename)
{
    char aDateTime[80];

    SITH_ASSERT(pWorld != NULL);

    if ( !stdConffile_OpenWrite(pFilename) )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not open file '%s' for writing.\n", pFilename, 0, 0, 0);
        return 1;
    }

    stdPlatform_GetDateTimeString(aDateTime, sizeof(aDateTime));
    if ( stdConffile_Printf("#********** World file %s ***** Created %s **********\n\n", pFilename, aDateTime) ) goto error;
    if ( sithWorld_WriteCopyrightText() ) goto error;
    if ( sithWorld_WriteHeaderText(pWorld) ) goto error;
    if ( sithSound_WriteSoundsListText(pWorld) ) goto error;
    if ( sithMaterial_WriteMaterialsListText(pWorld) ) goto error;
    if ( sithWorld_WriteGeoresourceText(pWorld) ) goto error;
    if ( sithSector_WriteSectorsListText(pWorld) ) goto error;
    if ( sithAIClass_WriteStaticAIClassesListText(pWorld) ) goto error;
    if ( sithModel_WriteStaticModelsListText(pWorld) ) goto error;
    if ( sithSprite_WriteStaticSpritesListText(pWorld) ) goto error;
    if ( sithPuppet_WriteStaticKeyframesListText(pWorld) ) goto error;
    if ( sithPuppet_WriteStaticPuppetsListText(pWorld) ) goto error;
    if ( sithSoundClass_WriteSoundClassesListText(pWorld) ) goto error;
    if ( sithCog_WriteCogScriptsListText(pWorld) ) goto error;
    if ( sithCog_WriteCogsListText(pWorld) ) goto error;
    if ( sithTemplate_WriteThingTemplatesListText(pWorld) ) goto error;
    if ( sithWrite_WriteStaticThingsListText(pWorld) ) goto error;
    if ( sithPVS_WritePVSText(pWorld) ) goto error;

    stdConffile_CloseWrite();
    return 0;

error:
    stdConffile_CloseWrite();
    return 1;
}

#line 123
int J3DAPI sithWrite_WriteStaticThingsListText(const SithWorld* pWorld)
{
    int count;
    int i;
    const SithThing* pThing;
    rdVector3 pyr;
    SithThing baseThing;
    const SithThing* pBasedOn;

    count = 0;
    SITH_ASSERT(pWorld != NULL);

    sithThing_Reset(&baseThing);
    if ( stdConffile_WriteLine("######## Thing placement #######\n") ) goto error;
    if ( stdConffile_WriteLine("Section: Things\n") ) goto error;
    if ( stdConffile_Printf("\nWorld things %d\n\n", pWorld->numThings) ) goto error;
    if ( stdConffile_WriteLine("#num template:       name:         \tX:\t\tY:\t\tZ:\t\tPitch:\t\tYaw:\t\tRoll:\t\tSector:\n") ) goto error;

    for ( i = 0; i < (int)pWorld->numThings; ++i )
    {
        pThing = &pWorld->aThings[i];
        if ( pThing->type )
        {
            rdMatrix_ExtractAngles34(&pThing->orient, &pyr);
            if ( stdConffile_Printf(
                "%3d: %-15s %-15s\t%f\t%f\t%f\t%f\t%f\t%f\t%d\t",
                i,
                pThing->pTemplate ? pThing->pTemplate->aName : "none",
                pThing->aName[0] ? pThing->aName : "none",
                pThing->pos.x,
                pThing->pos.y,
                pThing->pos.z,
                pyr.x,
                pyr.y,
                pyr.z,
                sithSector_GetSectorIndexEx(pWorld, pThing->pInSector)) )
            {
                goto error;
            }

            pBasedOn = pThing->pTemplate;
            if ( !pBasedOn )
            {
                pBasedOn = &baseThing;
            }

            if ( sithWrite_WriteThingArgs(pThing, pBasedOn, 0) ) goto error;
            if ( stdConffile_WriteLine("\n") ) goto error;
        }
    }

    if ( stdConffile_WriteLine("end\n") ) goto error;
    if ( stdConffile_WriteLine("################################\n\n\n") ) goto error;

    return 0;

error:
    return 1;
}

#line 197
int J3DAPI sithWrite_WriteThingArgs(const SithThing* pThing, const SithThing* pBasedOn, int bWriteOrient)
{
    rdVector3 pyr;

    SITH_ASSERT(pThing != NULL);
    SITH_ASSERT(pBasedOn != NULL);

    if ( bWriteOrient && memcmp(&pThing->orient, &pBasedOn->orient, sizeof(rdMatrix34)) )
    {
        rdMatrix_ExtractAngles34(&pThing->orient, &pyr);
        if ( stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[68], pyr.x, pyr.y, pyr.z) )
        {
            goto error;
        }
    }

    if ( pThing->type != pBasedOn->type && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[1], sithThing_aStrThingTypes[pThing->type]) )
    {
        goto error;
    }

    if ( pThing->collide.type != pBasedOn->collide.type && stdConffile_Printf("%s=%d ", sithThing_aStrThingArgs[2], pThing->collide.type) )
    {
        goto error;
    }

    if ( pThing->moveType != pBasedOn->moveType )
    {
        switch ( pThing->moveType )
        {
            case SITH_MT_PATH:
                if ( stdConffile_Printf("%s=path ", sithThing_aStrThingArgs[3]) )
                {
                    goto error;
                }
                break;
            case SITH_MT_PHYSICS:
                if ( stdConffile_Printf("%s=physics ", sithThing_aStrThingArgs[3]) )
                {
                    goto error;
                }
                break;
            default:
                if ( stdConffile_Printf("%s=none ", sithThing_aStrThingArgs[3]) )
                {
                    goto error;
                }
                break;
        }
    }

    if ( pThing->flags != pBasedOn->flags && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[5], pThing->flags) )
    {
        goto error;
    }

    if ( pThing->light.color.red != pBasedOn->light.color.red
        || pThing->light.color.green != pBasedOn->light.color.green
        || pThing->light.color.blue != pBasedOn->light.color.blue
        || pThing->light.color.alpha != pBasedOn->light.color.alpha )
    {
        if ( pThing->light.color.alpha != 0.0f )
        {
            if ( stdConffile_Printf("%s=(%f/%f/%f/%f) ", sithThing_aStrThingArgs[7], pThing->light.color.red, pThing->light.color.green, pThing->light.color.blue, pThing->light.color.alpha) )
            {
                goto error;
            }
        }
        else if ( stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[7], pThing->light.color.red, pThing->light.color.green, pThing->light.color.blue) )
        {
            goto error;
        }
    }

    if ( pThing->msecLifeLeft != pBasedOn->msecLifeLeft && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[6], pThing->msecLifeLeft / 1000.0f) )
    {
        goto error;
    }

    if ( pThing->renderData.type == RD_THING_MODEL3
        && (pThing->renderData.type != pBasedOn->renderData.type || pThing->renderData.data.pModel3 != pBasedOn->renderData.data.pModel3)
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[10], pThing->renderData.data.pModel3->aName) )
    {
        goto error;
    }

    if ( pThing->renderData.type == RD_THING_SPRITE3
        && (pThing->renderData.type != pBasedOn->renderData.type || pThing->renderData.data.pSprite3 != pBasedOn->renderData.data.pSprite3) )
    {
        rdSprite3* pSprite = pThing->renderData.data.pSprite3;
        if ( stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[11], pSprite->aName) )
        {
            goto error;
        }
    }

    if ( pThing->renderData.type == RD_THING_PARTICLE
        && (pThing->renderData.type != pBasedOn->renderData.type || pThing->renderData.data.pParticle != pBasedOn->renderData.data.pParticle) )
    {
        rdParticle* pParticle = pThing->renderData.data.pParticle;
        if ( stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[52], pParticle->aName) )
        {
            goto error;
        }
    }

    if ( pThing->collide.size != pBasedOn->collide.size && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[4], pThing->collide.size) )
    {
        goto error;
    }

    if ( pThing->collide.movesize != pBasedOn->collide.movesize && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[54], pThing->collide.movesize) )
    {
        goto error;
    }

    if ( pThing->pPuppetClass != pBasedOn->pPuppetClass
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[36], pThing->pPuppetClass ? pThing->pPuppetClass->aName : "none") )
    {
        goto error;
    }

    if ( pThing->pSoundClass != pBasedOn->pSoundClass
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[9], pThing->pSoundClass ? pThing->pSoundClass->aName : "none") )
    {
        goto error;
    }

    if ( pThing->pCreateThingTemplate != pBasedOn->pCreateThingTemplate
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[64], pThing->pCreateThingTemplate ? pThing->pCreateThingTemplate->aName : "none") )
    {
        goto error;
    }

    if ( pThing->pCog != pBasedOn->pCog
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[46], pThing->pCog ? pThing->pCog->pScript->aName : "none") )
    {
        goto error;
    }

    if ( pThing->perfLevel != pBasedOn->perfLevel && stdConffile_Printf("%s=%d ", sithThing_aStrThingArgs[76], pThing->perfLevel) )
    {
        goto error;
    }

    switch ( pThing->moveType )
    {
        case SITH_MT_PHYSICS:
            if ( sithWrite_WriteThingPhysicsArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
        case SITH_MT_PATH:
            if ( sithWrite_WriteThingPathMovementArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
    }

    switch ( pThing->type )
    {
        case SITH_THING_ACTOR:
        case SITH_THING_PLAYER:
            if ( sithWrite_WriteActorThingArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
        case SITH_THING_WEAPON:
            if ( sithWrite_WriteWeaponThingArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
        case SITH_THING_EXPLOSION:
            if ( sithWrite_WriteExplosionThingArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
        case SITH_THING_ITEM:
            if ( sithWrite_WtiteItemThingArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
        case SITH_THING_PARTICLE:
            if ( sithWrite_WriteParticleThingArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
        case SITH_THING_HINT:
            if ( stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[69], pThing->userval) )
            {
                goto error;
            }
            break;
    }

    switch ( pThing->controlType )
    {
        case SITH_CT_AI:
            if ( sithWrite_WriteAIArgs(pThing, pBasedOn) )
            {
                goto error;
            }
            break;
    }
    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WriteThingPhysicsArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    if ( pThing->moveInfo.physics.surfDrag != pBasedOn->moveInfo.physics.surfDrag
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[12], pThing->moveInfo.physics.surfDrag) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.airDrag != pBasedOn->moveInfo.physics.airDrag
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[13], pThing->moveInfo.physics.airDrag) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.staticDrag != pBasedOn->moveInfo.physics.staticDrag
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[14], pThing->moveInfo.physics.staticDrag) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.mass != pBasedOn->moveInfo.physics.mass
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[15], pThing->moveInfo.physics.mass) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.height != pBasedOn->moveInfo.physics.height
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[16], pThing->moveInfo.physics.height) )
    {
        goto error;
    }

    if ( pThing->collide.unkWidth != pBasedOn->collide.unkWidth
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[74], pThing->collide.unkWidth) )
    {
        goto error;
    }

    if ( pThing->collide.unkHeight != pBasedOn->collide.unkHeight
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[73], pThing->collide.unkHeight) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.flags != pBasedOn->moveInfo.physics.flags
        && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[17], pThing->moveInfo.physics.flags) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.maxRotationVelocity != pBasedOn->moveInfo.physics.maxRotationVelocity
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[18], pThing->moveInfo.physics.maxRotationVelocity) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.maxVelocity != pBasedOn->moveInfo.physics.maxVelocity
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[19], pThing->moveInfo.physics.maxVelocity) )
    {
        goto error;
    }

    if ( memcmp(&pThing->moveInfo.physics.velocity, &pBasedOn->moveInfo.physics.velocity, sizeof(rdVector3))
        && stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[20], pThing->moveInfo.physics.velocity.x, pThing->moveInfo.physics.velocity.y, pThing->moveInfo.physics.velocity.z) )
    {
        goto error;
    }

    if ( memcmp(&pThing->moveInfo.physics.angularVelocity, &pBasedOn->moveInfo.physics.angularVelocity, sizeof(rdVector3))
        && stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[21], pThing->moveInfo.physics.angularVelocity.x, pThing->moveInfo.physics.angularVelocity.y, pThing->moveInfo.physics.angularVelocity.z) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.orientSpeed != pBasedOn->moveInfo.physics.orientSpeed
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[55], pThing->moveInfo.physics.orientSpeed) )
    {
        goto error;
    }

    if ( pThing->moveInfo.physics.buoyancy != pBasedOn->moveInfo.physics.buoyancy
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[56], pThing->moveInfo.physics.buoyancy) )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WriteThingPathMovementArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    size_t i;
    const SithPathMoveInfo* pPath;

    SITH_ASSERT(pThing && pThing->moveType == SITH_MT_PATH);

    pPath = &pThing->moveInfo.pathMovement;
    if ( pPath->numFrames )
    {
        stdConffile_Printf("%s=%d ", sithThing_aStrThingArgs[35], pPath->numFrames);
        for ( i = 0; i < pPath->numFrames; ++i )
        {
            if ( stdConffile_Printf("%s=(%f/%f/%f:%f/%f/%f) ", sithThing_aStrThingArgs[34], pPath->aFrames[i].pos.x, pPath->aFrames[i].pos.y, pPath->aFrames[i].pos.z, pPath->aFrames[i].pyr.x, pPath->aFrames[i].pyr.y, pPath->aFrames[i].pyr.z) ) goto error;
        }
    }

    return 0;

error:
    return 1;
}

#line 531
int J3DAPI sithWrite_WriteActorThingArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    SITH_ASSERT((pThing != NULL) && (pBasedOn != NULL));
    SITH_ASSERT((pThing->type == SITH_THING_ACTOR) || (pThing->type == SITH_THING_PLAYER));

    if ( pThing->thingInfo.actorInfo.pWeaponTemplate != pBasedOn->thingInfo.actorInfo.pWeaponTemplate
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[29], pThing->thingInfo.actorInfo.pWeaponTemplate->aName) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.health != pBasedOn->thingInfo.actorInfo.health
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[23], pThing->thingInfo.actorInfo.health) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.maxHealth != pBasedOn->thingInfo.actorInfo.maxHealth
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[53], pThing->thingInfo.actorInfo.maxHealth) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.maxThrust != pBasedOn->thingInfo.actorInfo.maxThrust
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[24], pThing->thingInfo.actorInfo.maxThrust) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.maxRotVelocity != pBasedOn->thingInfo.actorInfo.maxRotVelocity
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[25], pThing->thingInfo.actorInfo.maxRotVelocity) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.maxHeadVelocity != pBasedOn->thingInfo.actorInfo.maxHeadVelocity
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[26], pThing->thingInfo.actorInfo.maxHeadVelocity) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.maxHeadYaw != pBasedOn->thingInfo.actorInfo.maxHeadYaw
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[27], pThing->thingInfo.actorInfo.maxHeadYaw) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.jumpSpeed != pBasedOn->thingInfo.actorInfo.jumpSpeed
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[28], pThing->thingInfo.actorInfo.jumpSpeed) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.flags != pBasedOn->thingInfo.actorInfo.flags
        && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[22], pThing->thingInfo.actorInfo.flags) )
    {
        goto error;
    }

    if ( memcmp(&pThing->thingInfo.actorInfo.eyeOffset, &pBasedOn->thingInfo.actorInfo.eyeOffset, sizeof(rdVector3))
        && stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[57], pThing->thingInfo.actorInfo.eyeOffset.x, pThing->thingInfo.actorInfo.eyeOffset.y, pThing->thingInfo.actorInfo.eyeOffset.z) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.minHeadPitch != pBasedOn->thingInfo.actorInfo.minHeadPitch
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[58], pThing->thingInfo.actorInfo.minHeadPitch) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.maxHeadPitch != pBasedOn->thingInfo.actorInfo.maxHeadPitch
        && stdConffile_Printf("%s=%.2f ", sithThing_aStrThingArgs[59], pThing->thingInfo.actorInfo.maxHeadPitch) )
    {
        goto error;
    }

    if ( memcmp(&pThing->thingInfo.actorInfo.fireOffset, &pBasedOn->thingInfo.actorInfo.fireOffset, sizeof(rdVector3))
        && stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[60], pThing->thingInfo.actorInfo.fireOffset.x, pThing->thingInfo.actorInfo.fireOffset.y, pThing->thingInfo.actorInfo.fireOffset.z) )
    {
        goto error;
    }

    if ( memcmp(&pThing->thingInfo.actorInfo.lightOffset, &pBasedOn->thingInfo.actorInfo.lightOffset, sizeof(rdVector3))
        && stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[61], pThing->thingInfo.actorInfo.lightOffset.x, pThing->thingInfo.actorInfo.lightOffset.y, pThing->thingInfo.actorInfo.lightOffset.z) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.headLightIntensity.red != pBasedOn->thingInfo.actorInfo.headLightIntensity.red
        && pThing->thingInfo.actorInfo.headLightIntensity.green != pBasedOn->thingInfo.actorInfo.headLightIntensity.green
        && pThing->thingInfo.actorInfo.headLightIntensity.blue != pBasedOn->thingInfo.actorInfo.headLightIntensity.blue
        && stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[62], pThing->thingInfo.actorInfo.headLightIntensity.red, pThing->thingInfo.actorInfo.headLightIntensity.green, pThing->thingInfo.actorInfo.headLightIntensity.blue) )
    {
        goto error;
    }

    if ( memcmp(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor, &pBasedOn->thingInfo.actorInfo.voiceInfo.voiceColor, sizeof(VGradiantColor))
        && stdConffile_Printf(
            "%s=(%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f/%f) ",
            sithThing_aStrThingArgs[75],
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.red,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.green,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.blue,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.alpha,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.middle.red,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.middle.green,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.middle.blue,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.middle.alpha,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomLeft.red,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomLeft.green,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomLeft.blue,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomLeft.alpha,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomRight.red,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomRight.green,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomRight.blue,
            pThing->thingInfo.actorInfo.voiceInfo.voiceColor.bottomRight.alpha) )
    {
        goto error;
    }

    if ( pThing->thingInfo.actorInfo.pExplodeTemplate != pBasedOn->thingInfo.actorInfo.pExplodeTemplate
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[33], pThing->thingInfo.actorInfo.pExplodeTemplate ? pThing->thingInfo.actorInfo.pExplodeTemplate->aName : "none") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WriteWeaponThingArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    const SithWeaponInfo* pWeaponInfo;
    const SithWeaponInfo* pBasedOnWeaponInfo;

    pWeaponInfo = &pThing->thingInfo.weaponInfo;
    pBasedOnWeaponInfo = &pBasedOn->thingInfo.weaponInfo;

    if ( pWeaponInfo->pExplosionTemplate != pBasedOnWeaponInfo->pExplosionTemplate
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[33], pWeaponInfo->pExplosionTemplate ? pWeaponInfo->pExplosionTemplate->aName : "none") )
    {
        goto error;
    }

    if ( pWeaponInfo->damage != pBasedOnWeaponInfo->damage && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[30], pWeaponInfo->damage) ) goto error;
    if ( pWeaponInfo->minDamage != pBasedOnWeaponInfo->minDamage && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[31], pWeaponInfo->minDamage) ) goto error;
    if ( pWeaponInfo->damageType != pBasedOnWeaponInfo->damageType && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[32], pWeaponInfo->damageType) ) goto error;
    if ( pWeaponInfo->flags != pBasedOnWeaponInfo->flags && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[22], pWeaponInfo->flags) ) goto error;
    if ( pWeaponInfo->range != pBasedOnWeaponInfo->range && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[41], pWeaponInfo->range) ) goto error;
    if ( pWeaponInfo->rate != pBasedOnWeaponInfo->rate && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[49], pWeaponInfo->rate) ) goto error;
    if ( pWeaponInfo->force != pBasedOnWeaponInfo->force && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[39], pWeaponInfo->force) ) goto error;

    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WriteExplosionThingArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    const SithExplosionInfo* pInfo;
    size_t i;
    const SithExplosionInfo* pBasedOnInfo;

    pInfo = &pThing->thingInfo.explosionInfo;
    pBasedOnInfo = &pBasedOn->thingInfo.explosionInfo;

    if ( pInfo->flags != pBasedOnInfo->flags && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[22], pInfo->flags) ) goto error;
    if ( pInfo->damage != pBasedOnInfo->damage && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[30], pInfo->damage) ) goto error;
    if ( pInfo->damageType != pBasedOnInfo->damageType && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[32], pInfo->damageType) ) goto error;

    if ( pInfo->msecBlastTime != pBasedOnInfo->msecBlastTime
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[37], pInfo->msecBlastTime / 1000.0f) )
    {
        goto error;
    }

    if ( pInfo->msecBabyTime != pBasedOnInfo->msecBabyTime
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[38], pInfo->msecBabyTime / 1000.0f) )
    {
        goto error;
    }

    if ( pInfo->msecExpandTime != pBasedOnInfo->msecExpandTime
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[43], pInfo->msecExpandTime / 1000.0f) )
    {
        goto error;
    }

    if ( pInfo->msecFadeTime != pBasedOnInfo->msecFadeTime
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[44], pInfo->msecFadeTime / 1000.0f) )
    {
        goto error;
    }

    if ( pInfo->force != pBasedOnInfo->force && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[39], pInfo->force) ) goto error;
    if ( pInfo->maxLight != pBasedOnInfo->maxLight && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[40], pInfo->maxLight) ) goto error;
    if ( pInfo->range != pBasedOnInfo->range && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[41], pInfo->range) ) goto error;

    if ( strlen(pInfo->aSpriteTemplateName) > 0 )
    {
        if ( stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[70], pInfo->aSpriteTemplateName) ) goto error;
        if ( stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[71], pInfo->spriteStart.x, pInfo->spriteStart.y, pInfo->spriteStart.z) ) goto error;
        if ( stdConffile_Printf("%s=(%f/%f/%f) ", sithThing_aStrThingArgs[72], pInfo->spriteEnd.x, pInfo->spriteEnd.y, pInfo->spriteEnd.z) ) goto error;
    }

    for ( i = 0; i < STD_ARRAYLEN(pInfo->apDebries); ++i )
    {
        if ( pInfo->apDebries[i] && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[63], pInfo->apDebries[i]->aName) ) goto error;
    }

    if ( memcmp(&pInfo->flashColor, &pBasedOnInfo->flashColor, sizeof(rdRGB))
        && stdConffile_Printf("%s=(%d/%d/%d) ", sithThing_aStrThingArgs[42], pInfo->flashColor.red, pInfo->flashColor.green, pInfo->flashColor.blue) )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WtiteItemThingArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    if ( pThing->thingInfo.itemInfo.flags != pBasedOn->thingInfo.itemInfo.flags
        && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[22], pThing->thingInfo.itemInfo.flags) )
    {
        goto error;
    }

    if ( pThing->thingInfo.itemInfo.secRespawnInterval != pBasedOn->thingInfo.itemInfo.secRespawnInterval
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[47], pThing->thingInfo.itemInfo.secRespawnInterval) )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WriteParticleThingArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    if ( pThing->thingInfo.particleInfo.flags != pBasedOn->thingInfo.particleInfo.flags
        && stdConffile_Printf("%s=0x%x ", sithThing_aStrThingArgs[22], pThing->thingInfo.particleInfo.flags) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.pMaterial != pBasedOn->thingInfo.particleInfo.pMaterial
        && stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[48], pThing->thingInfo.particleInfo.pMaterial ? pThing->thingInfo.particleInfo.pMaterial->aName : "none") )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.maxRadius != pBasedOn->thingInfo.particleInfo.maxRadius
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[41], pThing->thingInfo.particleInfo.maxRadius) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.timeoutRate != pBasedOn->thingInfo.particleInfo.timeoutRate
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[49], pThing->thingInfo.particleInfo.timeoutRate) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.growthSpeed != pBasedOn->thingInfo.particleInfo.growthSpeed
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[24], pThing->thingInfo.particleInfo.growthSpeed) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.size != pBasedOn->thingInfo.particleInfo.size
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[51], pThing->thingInfo.particleInfo.size) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.numParticles != pBasedOn->thingInfo.particleInfo.numParticles
        && stdConffile_Printf("%s=%d ", sithThing_aStrThingArgs[50], pThing->thingInfo.particleInfo.numParticles) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.minRadius != pBasedOn->thingInfo.particleInfo.minRadius
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[65], pThing->thingInfo.particleInfo.minRadius) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.pitchRange != pBasedOn->thingInfo.particleInfo.pitchRange
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[66], pThing->thingInfo.particleInfo.pitchRange) )
    {
        goto error;
    }

    if ( pThing->thingInfo.particleInfo.yawRange != pBasedOn->thingInfo.particleInfo.yawRange
        && stdConffile_Printf("%s=%f ", sithThing_aStrThingArgs[67], pThing->thingInfo.particleInfo.yawRange) )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWrite_WriteAIArgs(const SithThing* pThing, const SithThing* pBasedOn)
{
    const SithAIControlBlock* pLocal;
    size_t i;
    const SithAIClass* pClassInfo;

    pLocal = pThing->controlInfo.aiControl.pLocal;
    if ( pThing->controlInfo.aiControl.pClass != pBasedOn->controlInfo.aiControl.pClass )
    {
        pClassInfo = pThing->controlInfo.aiControl.pClass;
        if ( stdConffile_Printf("%s=%s ", sithThing_aStrThingArgs[45], pClassInfo ? pClassInfo->aName : "none") ) goto error;
    }

    if ( pLocal && pLocal->numFrames )
    {
        if ( stdConffile_Printf("%s=%d ", sithThing_aStrThingArgs[35], pLocal->numFrames) ) goto error;

        for ( i = 0; i < pLocal->numFrames; ++i )
        {
            if ( stdConffile_Printf("%s=(%.3f/%.3f/%.3f) ", sithThing_aStrThingArgs[34], pLocal->aFrames[i].x, pLocal->aFrames[i].y, pLocal->aFrames[i].z) ) goto error;
        }
    }

    return 0;

error:
    return 1;
}
