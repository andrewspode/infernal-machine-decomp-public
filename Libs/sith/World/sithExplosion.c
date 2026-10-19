#include "sithExplosion.h"

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAIAwareness.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>

#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

static int sithExplosion_bExpandFadeSet = 0;

void J3DAPI sithExplosion_Initialize(SithThing* pThing)
{
    SithExplosionInfo* pExplode;
#line 77
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pThing->type == SITH_THING_EXPLOSION);

    pExplode = &pThing->thingInfo.explosionInfo;
    pExplode->msecLifeLeft    = pThing->msecLifeLeft;
    pExplode->msecBabyTime   += sithTime_msecGameTime;
    pExplode->msecExpandTime += sithTime_msecGameTime;
    pExplode->msecFadeTime   += sithTime_msecGameTime;
    pExplode->pHitSurfaceMat  = NULL;
}


void J3DAPI sithExplosion_CreateSpriteThing(SithThing* pThing)
{
    SithThing* pThingSprite;
    float secLifeTime;
    const SithThing* pSrcTemplate;
    SithExplosionInfo* pExplode;
    unsigned int numCels;
    rdMaterial* pCelMaterial;
    float frameRate;

    pExplode = &pThing->thingInfo.explosionInfo;
    if ( strlen(pExplode->aSpriteTemplateName) > 0 )
    {
        pSrcTemplate = sithTemplate_GetTemplate(pExplode->aSpriteTemplateName);
        if ( pSrcTemplate )
        {
            pThingSprite = sithThing_CreateThingAtPos(pSrcTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, NULL);
            if ( !pThingSprite )
            {
#line 197
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "could not create thingsprite for explosion", 0, 0, 0, 0);
                pExplode->pSprite = NULL;
            }
            else
            {
                pExplode->pSprite = pThingSprite;
                if ( (pExplode->flags & SITH_EF_EXPAND) == 0 && (pExplode->flags & SITH_EF_FADE) == 0 )
                {
                    secLifeTime = (float)pThingSprite->msecLifeLeft / 1000.0f;
                    sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &pExplode->spriteStart, &pExplode->spriteEnd, secLifeTime);
                }
                else
                {
                    sithExplosion_UpdateExpandFade(pExplode);
                }
            }
        }
    }

    if ( (pExplode->flags & SITH_EF_ANIMATESPRITE) != 0 )
    {
        if ( pThing->renderData.type == RD_THING_SPRITE3 )
        {
            pCelMaterial = pThing->renderData.data.pSprite3->face.pMaterial;
            if ( !pCelMaterial || (numCels = pCelMaterial->numCels) <= 1 )
            {
                pExplode->flags &= ~SITH_EF_ANIMATESPRITE;
            }
            else
            {
                frameRate = RD_MULTIPLY((D3DVAL(numCels)) / D3DVAL(pThing->msecLifeLeft), 1000.0f); /* spelling-fit */
                sithAnimate_StartSpriteAnim(pThing, frameRate, SITHANIMATE_PAGEFLIP);
            }
        }
        else if ( pExplode->pSprite )
        {
            pCelMaterial = pExplode->pSprite->renderData.data.pSprite3->face.pMaterial;
            if ( !pCelMaterial || (numCels = pCelMaterial->numCels) <= 1 || (float)pExplode->pSprite->msecLifeLeft == 0.0f )
            {
                pExplode->flags &= ~SITH_EF_ANIMATESPRITE;
            }
            else
            {
                frameRate = RD_MULTIPLY(D3DVAL(numCels) / D3DVAL(pExplode->pSprite->msecLifeLeft), 1000.0f);
                sithAnimate_StartSpriteAnim(pExplode->pSprite, frameRate, SITHANIMATE_PAGEFLIP);
            }
        }
    }

    if ( pExplode && pExplode->pSprite && pExplode->pSprite->renderData.data.pSprite3 )
    {
        if ( (pExplode->flags & SITH_EF_RANDOMROLL) != 0 )
        {
            pExplode->pSprite->thingInfo.spriteInfo.rollAngle = RD_MULTIPLY(SITH_RAND(), 360.0f);
        }
        else
        {
            pExplode->pSprite->thingInfo.spriteInfo.rollAngle = 0.0f;
        }
    }
}

void J3DAPI sithExplosion_UpdateExpandFade(SithExplosionInfo* pExplode)
{
    rdVector3 startSize;
    rdVector3 end;
    float lifeRemainingMsec;
    float secAnimTime;
    float deltaMsec;
    float maxTime;

    if ( !pExplode || !pExplode->pSprite )
    {
        return;
    }

    rdVector_Copy3(&startSize, &pExplode->spriteStart);
    rdVector_Copy3(&end, &pExplode->spriteEnd);

    lifeRemainingMsec = (float)pExplode->pSprite->msecLifeLeft;
    secAnimTime  = lifeRemainingMsec / 1000.0f;

    if ( (float)pExplode->msecFadeTime == (float)sithTime_msecGameTime + lifeRemainingMsec )
    {
        pExplode->flags &= ~SITH_EF_FADE;
    }

    if ( (float)pExplode->msecExpandTime == (float)sithTime_msecGameTime + lifeRemainingMsec )
    {
        pExplode->flags &= ~SITH_EF_EXPAND;
    }

    if ( !sithExplosion_bExpandFadeSet )
    {
        if ( pExplode->msecFadeTime == sithTime_msecGameTime )
        {
            deltaMsec = (float)(pExplode->msecExpandTime - sithTime_msecGameTime);
            end.x = pExplode->spriteStart.x;
            end.y = pExplode->spriteStart.y;
            end.z = sithExplosion_Lerp(pExplode->spriteStart.z, pExplode->spriteEnd.z, lifeRemainingMsec, deltaMsec);
            secAnimTime = (float)(pExplode->msecExpandTime - sithTime_msecGameTime) / 1000.0f;
            sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &startSize, &end, secAnimTime);
            pExplode->flags &= ~SITH_EF_FADE;
        }
        else if ( pExplode->msecExpandTime == sithTime_msecGameTime )
        {
            deltaMsec = (float)(pExplode->msecFadeTime - sithTime_msecGameTime);
            end.x = sithExplosion_Lerp(pExplode->spriteStart.x, pExplode->spriteEnd.x, lifeRemainingMsec, deltaMsec);
            end.y = sithExplosion_Lerp(pExplode->spriteStart.y, pExplode->spriteEnd.y, lifeRemainingMsec, deltaMsec);
            end.z = pExplode->spriteStart.z;
            secAnimTime = (float)(pExplode->msecFadeTime - sithTime_msecGameTime) / 1000.0f;
            sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &startSize, &end, secAnimTime);
            pExplode->flags &= ~SITH_EF_EXPAND;
        }
        else
        {
            if ( pExplode->msecExpandTime > pExplode->msecFadeTime )
            {
                secAnimTime = (float)(pExplode->msecFadeTime - sithTime_msecGameTime) / 1000.0f;
            }
            else
            {
                secAnimTime = (float)(pExplode->msecExpandTime - sithTime_msecGameTime) / 1000.0f;
            }

            sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &startSize, &startSize, secAnimTime);
        }

        sithExplosion_bExpandFadeSet = 1;
    }

    else
    {
        if ( pExplode->msecExpandTime != pExplode->msecFadeTime )
        {
            if ( sithTime_msecGameTime >= pExplode->msecExpandTime && (pExplode->flags & SITH_EF_EXPAND) != 0 )
            {
                if ( pExplode->msecFadeTime > pExplode->msecExpandTime )
                {
                    deltaMsec = (float)(pExplode->msecFadeTime - sithTime_msecGameTime);
                    end.x = sithExplosion_Lerp(pExplode->spriteStart.x, pExplode->spriteEnd.x, lifeRemainingMsec, deltaMsec);
                    end.y = sithExplosion_Lerp(pExplode->spriteStart.y, pExplode->spriteEnd.y, lifeRemainingMsec, deltaMsec);
                    end.z = pExplode->spriteStart.z;
                    secAnimTime = (float)(pExplode->msecFadeTime - sithTime_msecGameTime) / 1000.0f;
                }
                else
                {
                    maxTime   = (float)sithTime_msecGameTime + lifeRemainingMsec - (float)pExplode->msecFadeTime;
                    deltaMsec = (float)(sithTime_msecGameTime - pExplode->msecFadeTime);
                    startSize.z = sithExplosion_Lerp(pExplode->spriteStart.z, pExplode->spriteEnd.z, maxTime, deltaMsec);
                }

                sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &startSize, &end, secAnimTime);
                pExplode->msecExpandTime = sithTime_msecGameTime;
                pExplode->flags &= ~SITH_EF_EXPAND;
            }
            else if ( sithTime_msecGameTime >= pExplode->msecFadeTime && (pExplode->flags & SITH_EF_FADE) != 0 )
            {
                if ( pExplode->msecExpandTime > pExplode->msecFadeTime )
                {
                    deltaMsec = (float)(pExplode->msecExpandTime - sithTime_msecGameTime);
                    end.x = pExplode->spriteStart.x;
                    end.y = pExplode->spriteStart.y;
                    end.z = sithExplosion_Lerp(pExplode->spriteStart.z, pExplode->spriteEnd.z, lifeRemainingMsec, deltaMsec);
                    secAnimTime = (float)(pExplode->msecExpandTime - sithTime_msecGameTime) / 1000.0f;
                }
                else
                {
                    maxTime   = (float)sithTime_msecGameTime + lifeRemainingMsec - (float)pExplode->msecExpandTime;
                    deltaMsec = (float)(sithTime_msecGameTime - pExplode->msecExpandTime);
                    startSize.x = sithExplosion_Lerp(pExplode->spriteStart.x, pExplode->spriteEnd.x, maxTime, deltaMsec);
                    startSize.y = sithExplosion_Lerp(pExplode->spriteStart.y, pExplode->spriteEnd.y, maxTime, deltaMsec);
                }

                sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &startSize, &end, secAnimTime);
                pExplode->msecFadeTime = sithTime_msecGameTime;
                pExplode->flags &= ~SITH_EF_FADE;
            }
        }
        else if ( sithTime_msecGameTime >= pExplode->msecExpandTime && (pExplode->flags & SITH_EF_EXPAND) != 0 && (pExplode->flags & SITH_EF_FADE) != 0 )
        {
            sithAnimate_StartSpriteSizeAnim(pExplode->pSprite, &startSize, &end, secAnimTime);
            pExplode->flags &= ~(SITH_EF_EXPAND | SITH_EF_FADE);
        }
    }
    if ( (pExplode->flags & SITH_EF_EXPAND) == 0 && (pExplode->flags & SITH_EF_FADE) == 0 )
    {
        sithExplosion_bExpandFadeSet = 0;
    }
}

float J3DAPI sithExplosion_Lerp(float min, float max, float maxTime, float deltaTime)
{
    float range = max - min;
    if ( maxTime > 0.0f )
    {
        return RD_MULTIPLY(range, deltaTime) / maxTime + min;
    }

    return max;
}

void J3DAPI sithExplosion_Update(SithThing* pThing, float sedDeltaTime)
{
    SithExplosionInfo* pExplode;
    float delta;

    pExplode = &pThing->thingInfo.explosionInfo;
#line 477
    SITH_ASSERTREL(pThing && pExplode);

    if ( (pExplode->flags & SITH_EF_BLASTPHASE) != 0 && pThing->msecLifeLeft <= pExplode->msecBlastTime )
    {
        sithExplosion_MakeBlast(pThing);
        pExplode->flags &= ~SITH_EF_BLASTPHASE;
    }

    if ( (pExplode->flags & SITH_EF_CHILDEXPLOSION) != 0
        && pExplode->msecBabyTime <= sithTime_msecGameTime
        && pThing->pCreateThingTemplate )
    {
        SithThing* pChildExplosion = sithThing_CreateThingAtPos(pThing->pCreateThingTemplate, &pThing->pos, &pThing->orient, pThing->pInSector, pThing->pParent);
        if ( pChildExplosion && (pThing->flags & SITH_TF_REMOTE) != 0 )
        {
            pChildExplosion->flags |= SITH_TF_REMOTE;
        }

        pExplode->flags &= ~SITH_EF_CHILDEXPLOSION;
    }

    if ( (pExplode->flags & SITH_EF_EXPAND) != 0 || (pExplode->flags & SITH_EF_FADE) != 0 )
    {
        sithExplosion_UpdateExpandFade(pExplode);
    }

    if ( (pExplode->flags & SITH_EF_VARIABLELIGHT) != 0 && (pThing->flags & SITH_TF_EMITLIGHT) != 0 )
    {
        if ( pThing->msecLifeLeft > pExplode->msecBlastTime )
        {
            delta = D3DDivide((D3DVAL(pExplode->msecLifeLeft - pThing->msecLifeLeft)), D3DVAL(pExplode->msecLifeLeft - pExplode->msecBlastTime)); /* spelling-fit */
        }
        else
        {
            delta = D3DDivide(pThing->msecLifeLeft, pExplode->msecBlastTime);
        }
        pThing->light.color.red   = RD_MULTIPLY(pExplode->maxLight, delta) + pThing->light.emitColor.red;
        pThing->light.color.green = RD_MULTIPLY(pExplode->maxLight, delta) + pThing->light.emitColor.green;
        pThing->light.color.blue  = RD_MULTIPLY(pExplode->maxLight, delta) + pThing->light.emitColor.blue;
    }
}

void J3DAPI sithExplosion_MakeBlast(SithThing* pThing)
{
    SithExplosionInfo* pExplode;
    float exprange;
    float expforce;
    float expdamage;
    SithCollision* pCollision;
    size_t i;

#line 565
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_EXPLOSION));

    pExplode  = &pThing->thingInfo.explosionInfo;
    exprange  = pExplode->range;
    expforce  = pExplode->force;
    expdamage = pExplode->damage;

    if ( !(exprange > 0.0f && (expdamage > 0.0f || expforce > 0.0f)) )
    {
    }
    else
    {
        sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 1, 3.0f, pThing);
        sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &rdroid_zeroVector3, 0.0f, exprange, 0x482);

        while ( (pCollision = sithCollision_PopStack()) != NULL )
        {
            float dist;
            float scale;
            SithThing* pVictim;

            dist  = pCollision->distance;
            scale = 1.0f - RD_MULTIPLY(dist / exprange, dist / exprange);
            scale = STDMATH_CLAMP(scale, 0.25f, 1.0f);

            if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
            {
                sithSurface_HandleThingImpact(pCollision->pSurfaceCollided, pThing, scale * expdamage, pExplode->damageType);
                continue;
            }

#line 601
            SITH_ASSERTREL(((pCollision->type) & (SITHCOLLISION_THING)));

            pVictim = pCollision->pThingCollided;
            if ( (pExplode->flags & SITH_EF_NOSHOOTERDAMAGE) != 0 && pVictim == pThing->pParent )
            {
                continue;
            }

            if ( !sithCollision_HasLOS(pThing, pVictim, 1) )
            {
                continue;
            }

            if ( expforce != 0.0f && pVictim->moveType == SITH_MT_PHYSICS && (pVictim->moveInfo.physics.flags & SITH_PF_USEBLASTFORCE) != 0 )
            {
                rdVector3 vecForce;
                rdVector_Scale3(&vecForce, &pCollision->hitNorm, -RD_MULTIPLY(expforce, scale));
                sithPhysics_ApplyForce(pVictim, &vecForce);
            }

            // Note: overwrites the explosion damage for the remaining collisions
            expdamage = sithThing_ScaleCombatDamage(pThing, pVictim);
            if ( expdamage != 0.0f )
            {
                sithThing_DamageThing(pVictim, pThing, scale * expdamage, pExplode->damageType);
            }
        }

        sithCollision_DecreaseStackLevel();
    }

    if ( (pExplode->flags & SITH_EF_UPDATEDEBRISMATERIAL) != 0 && !pExplode->pHitSurfaceMat )
    {
        return;
    }

    for ( i = 0; i < STD_ARRAYLEN(pExplode->apDebries); ++i )
    {
        if ( pExplode->apDebries[i] )
        {
            rdMatrix34 orient;
            SithThing* pDebris;
            rdVector3 pyr;

            pyr.pitch = RD_MULTIPLY(SITH_RAND(), 360.0f);
            pyr.yaw   = RD_MULTIPLY(SITH_RAND(), 360.0f);
            pyr.roll  = RD_MULTIPLY(SITH_RAND(), 360.0f);
            rdMatrix_BuildRotate34(&orient, &pyr);

            pDebris = sithThing_CreateThingAtPos(pExplode->apDebries[i], &pThing->pos, &orient, pThing->pInSector, NULL);
            if ( pDebris )
            {
                // Extend life left up to 1.5sec
                pDebris->msecLifeLeft += (uint32_t)RD_MULTIPLY(SITH_RAND(), 1500.0f);

                if ( (pExplode->flags & SITH_EF_UPDATEDEBRISMATERIAL) != 0 && pDebris->renderData.data.pModel3 )
                {
                    size_t j;
                    rdModel3* pModel = pDebris->renderData.data.pModel3;
                    for ( j = 0; j < pModel->aGeos[0].numMeshes; ++j )
                    {
                        size_t k;
                        for ( k = 0; k < pModel->aGeos[0].aMeshes[j].numFaces; ++k )
                        {
                            if ( pModel->aGeos[0].aMeshes[j].aFaces[k].pMaterial )
                            {
                                pModel->aGeos[0].aMeshes[j].aFaces[k].pMaterial = pExplode->pHitSurfaceMat;
                            }
                        }
                    }
                }
                else if ( (pExplode->flags & SITH_EF_ANIMATEDEBRISMATERIAL) != 0 )
                {
                    if ( pDebris && pDebris->renderData.data.pModel3 )
                    {
                        rdMaterial* pMat = pDebris->renderData.data.pModel3->apMaterials[i]; // Note: indexed by the debris slot
                        if ( pMat )
                        {
                            sithAnimate_StartMaterialAnim(pMat, 12.0f, (SithAnimateFlags)0);
                        }
                    }

                    pExplode->flags &= ~SITH_EF_ANIMATEDEBRISMATERIAL;
                }
            }
        }
    }
}

int J3DAPI sithExplosion_ParseArg(const StdConffileArg* pArg, SithThing* pThing, signed int adjNum)
{
    int value;
    size_t i;
    float currentfvalue;
    SithExplosionInfo* pExplode;
    char aTplName[256];

#line 721
    SITH_ASSERTREL(pThing && pArg && (adjNum < SITHTHING_NUMADJECTIVES));

    pExplode = &pThing->thingInfo.explosionInfo;
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPEFLAGS:
            if ( sscanf(pArg->argValue, "%x", &value) != 1 )
            {
                goto error;
            }

            pExplode->flags = value;
            break;

        case SITHTHING_ARG_DAMAGE:
            pExplode->damage = D3DVAL(atof(pArg->argValue));
            pExplode->flags |= SITH_EF_BLASTDAMAGE | SITH_EF_BLASTPHASE;
            break;

        case SITHTHING_ARG_DAMAGECLASS:
            if ( sscanf(pArg->argValue, "%x", &value) != 1 )
            {
                goto error;
            }

            pExplode->damageType = value;
            break;

        case SITHTHING_ARG_EXPANDTIME:
            currentfvalue = D3DVAL(atof(pArg->argValue));
            pExplode->msecExpandTime = (int)RD_MULTIPLY(1000.0f, currentfvalue);
            pExplode->flags |= SITH_EF_EXPAND;
            break;

        case SITHTHING_ARG_FADETIME:
            currentfvalue = D3DVAL(atof(pArg->argValue));
            pExplode->msecFadeTime = (int)RD_MULTIPLY(1000.0f, currentfvalue);
            pExplode->flags |= SITH_EF_FADE;
            break;

        case SITHTHING_ARG_BLASTTIME:
            currentfvalue = D3DVAL(atof(pArg->argValue));
            pExplode->msecBlastTime = (int)RD_MULTIPLY(1000.0f, currentfvalue);
            pExplode->flags |= SITH_EF_BLASTPHASE;
            break;

        case SITHTHING_ARG_BABYTIME:
            currentfvalue = D3DVAL(atof(pArg->argValue));
            pExplode->msecBabyTime = (int)RD_MULTIPLY(1000.0f, currentfvalue);
            pExplode->flags |= SITH_EF_CHILDEXPLOSION;
            break;

        case SITHTHING_ARG_FORCE:
            pExplode->force = D3DVAL(atof(pArg->argValue));
            pExplode->flags |= SITH_EF_BLASTPHASE;
            break;

        case SITHTHING_ARG_MAXLIGHT:
            pExplode->maxLight = D3DVAL(atof(pArg->argValue));
            pExplode->flags |= SITH_EF_VARIABLELIGHT;
            break;

        case SITHTHING_ARG_RANGE:
            pExplode->range = D3DVAL(atof(pArg->argValue));
            pExplode->flags |= SITH_EF_BLASTPHASE;
            break;

        case SITHTHING_ARG_DEBRIS:
            for ( i = 0; i < STD_ARRAYLEN(pExplode->apDebries); i++ )
            {
                if ( !pExplode->apDebries[i] )
                {
                    pExplode->apDebries[i] = sithTemplate_GetTemplate(pArg->argValue);
                    break;
                }
            }
            break;

        case SITHTHING_ARG_FLASHRGB:
            if ( sscanf(pArg->argValue, "(%d/%d/%d)", &pExplode->flashColor.red, &pExplode->flashColor.green, &pExplode->flashColor.blue) != 3 )
            {
                goto error;
            }
            break;

        case SITHTHING_ARG_SPRITETHING:
            if ( sscanf(pArg->argValue, "%s", aTplName) != 1 )
            {
                strcpy(pExplode->aSpriteTemplateName, "");
                goto error;
            }

            strcpy(pExplode->aSpriteTemplateName, aTplName);
            break;

        case SITHTHING_ARG_SPRITESTART:
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pExplode->spriteStart.x, &pExplode->spriteStart.y, &pExplode->spriteStart.z) != 3 )
            {
                goto error;
            }
            break;

        case SITHTHING_ARG_SPRITEEND:
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pExplode->spriteEnd.x, &pExplode->spriteEnd.y, &pExplode->spriteEnd.z) != 3 )
            {
                goto error;
            }
            break;

        default:
            return 0;
    }

    return 1;

error:
    return 0;
}
