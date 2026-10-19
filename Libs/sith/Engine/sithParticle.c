#include "sithParticle.h"

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdParticle.h>

#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>

#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SITHPARTICLE_MAXWORLDPARTICLES 64u // load particel when now particles allocated

// Retail spelling recovered from the assertion string "sithParticle_pHashtable != ((void *)0)"
static tHashTable* sithParticle_pHashtable;

rdParticle* J3DAPI sithParticle_CacheFind(const char* pName);
void J3DAPI sithParticle_CacheAdd(rdParticle* pParticle);
void J3DAPI sithParticle_CacheRemove(const rdParticle* pParticle);

int J3DAPI sithParticle_Startup(void)
{
    sithParticle_pHashtable = stdHashtbl_New(128u);
    if ( !sithParticle_pHashtable )
    {
#line 69
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate memory for particless.\n", 0, 0, 0, 0);
        return 1;
    }

    return 0;
}

void J3DAPI sithParticle_Shutdown(void)
{
    if ( sithParticle_pHashtable )
    {
        stdHashtbl_Free(sithParticle_pHashtable);
        sithParticle_pHashtable = NULL;
    }
}

rdParticle* J3DAPI sithParticle_Load(SithWorld* pWorld, const char* pName)
{
    rdParticle* pParticle;
    char aPath[128];

#line 105
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pName != NULL);

    if ( !pWorld->aParticles && sithParticle_AllocWorldParticles(pWorld, SITHPARTICLE_MAXWORLDPARTICLES) )
    {
        return NULL;
    }

    pParticle = sithParticle_CacheFind(pName);
    if ( pParticle )
    {
        return pParticle;
    }

    if ( pWorld->numParticles >= pWorld->sizeParticles )
    {
#line 124
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to define new particle '%s'.\n", pName, 0, 0, 0);
        return NULL;
    }

    pParticle = &pWorld->aParticles[pWorld->numParticles];
    sprintf(aPath, "%s%c%s", "misc\\par", '\\', pName);

    if ( !rdParticle_LoadEntry(aPath, pParticle) )
    {
#line 137
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Load failed for particle file %s.\n", aPath, 0, 0, 0);
        return NULL;
    }

    sithParticle_CacheAdd(pParticle);
    ++pWorld->numParticles;
    return pParticle;
}

int J3DAPI sithParticle_AllocWorldParticles(SithWorld* pWorld, size_t size)
{
#line 150
    SITH_ASSERTREL(pWorld->aParticles == NULL);
    SITH_ASSERTREL(pWorld->sizeParticles == 0);

#line 154
    pWorld->aParticles = (rdParticle*)STDMALLOC(sizeof(rdParticle) * size);
    if ( !pWorld->aParticles )
    {
#line 158
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate particles array.\n", 0, 0, 0, 0);
        return 1;
    }

    pWorld->sizeParticles = size;
    pWorld->numParticles  = 0;
    memset(pWorld->aParticles, 0, sizeof(rdParticle) * pWorld->sizeParticles);
    return 0;
}

int J3DAPI sithParticle_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPEFLAGS:
            if ( sscanf(pArg->argValue, "%x", &pThing->thingInfo.particleInfo.flags) != 1 )
            {
                goto error;
            }
            break;

        case SITHTHING_ARG_MATERIAL:
            pThing->thingInfo.particleInfo.pMaterial = sithMaterial_Load(pArg->argValue);
            break;

        case SITHTHING_ARG_RANGE:
            pThing->thingInfo.particleInfo.maxRadius = D3DVAL(atof(pArg->argValue));
#line 195
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.maxRadius >= ((float)0.0));
            break;

        case SITHTHING_ARG_MINSIZE:
            pThing->thingInfo.particleInfo.minRadius = D3DVAL(atof(pArg->argValue));
#line 200
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.minRadius >= ((float)0.0));
            break;

        case SITHTHING_ARG_RATE:
            pThing->thingInfo.particleInfo.timeoutRate = D3DVAL(atof(pArg->argValue));
#line 205
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.timeoutRate >= ((float)0.0));
            break;

        case SITHTHING_ARG_MAXTHRUST:
            pThing->thingInfo.particleInfo.growthSpeed = D3DVAL(atof(pArg->argValue));
#line 210
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.growthSpeed >= ((float)0.0));
            break;

        case SITHTHING_ARG_PITCHRANGE:
            pThing->thingInfo.particleInfo.pitchRange = D3DVAL(atof(pArg->argValue));
#line 215
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.pitchRange >= ((float)0.0));
            break;

        case SITHTHING_ARG_YAWRANGE:
            pThing->thingInfo.particleInfo.yawRange = D3DVAL(atof(pArg->argValue));
#line 220
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.yawRange >= ((float)0.0));
            break;

        case SITHTHING_ARG_ELEMENTSIZE:
            pThing->thingInfo.particleInfo.size = D3DVAL(atof(pArg->argValue));
#line 225
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.size > ((float)0.0));
            break;

        case SITHTHING_ARG_COUNT:
            pThing->thingInfo.particleInfo.numParticles = atoi(pArg->argValue);
#line 230
            SITH_ASSERTREL(pThing->thingInfo.particleInfo.numParticles <= RDPARTICLE_MAXVERTS);
            break;

        default:
            return 0;
    }

    return 1;

error:
    return 0;
}

void J3DAPI sithParticle_Update(SithThing* pThing, float secDeltaTime)
{
    SithParticleInfo* pInfo;
    size_t i;
    size_t numCels;
    size_t dcel;
    int dverts;
    float dtime;

#line 264
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_PARTICLE));
    SITH_ASSERTREL(pThing->renderData.data.pParticle);
    pInfo = &pThing->thingInfo.particleInfo;

    pInfo->secElapsed += secDeltaTime;
    if ( (pInfo->flags & SITHPARTICLE_FLAG_RANDOM_CEL_CHANGE) != 0 )
    {
        dtime   = D3DDivide(RD_MULTIPLY(pInfo->secElapsed, 1000.0f), pThing->msecLifeLeft);
        numCels = pInfo->pMaterial->numCels;
        dtime   = RD_MULTIPLY(dtime, secDeltaTime);
        for ( i = 0; i < pThing->renderData.data.pParticle->numVertices; ++i )
        {
            dcel = numCels - pThing->renderData.data.pParticle->aVertMatCelNums[i] - 1;
            if ( dcel && SITH_RAND() < D3DMultiply(dcel, dtime) )
            {
                ++pThing->renderData.data.pParticle->aVertMatCelNums[i];
            }
        }
    }

    if ( (pInfo->flags & SITHPARTICLE_FLAG_TIMEOUT_RATE) != 0 )
    {
        dverts = (int)RD_MULTIPLY(pThing->thingInfo.particleInfo.timeoutRate, 0.15f);
        dverts = (dverts < 1) ? 1 : dverts;

        pThing->renderData.data.pParticle->numVertices += dverts;
        if ( pThing->renderData.data.pParticle->numVertices > pThing->thingInfo.particleInfo.numParticles )
        {
            pThing->renderData.data.pParticle->numVertices = pThing->thingInfo.particleInfo.numParticles;
        }
    }

    if ( sithPlayerActions_IsInvisible() )
    {
        sithFX_UpdateFairyDustUserBlock(pThing);
    }

    if ( (pInfo->flags & SITHPARTICLE_FLAG_OUTWARD_EXPANDING) != 0 )
    {
        float growDelta;
        float scale;
        rdVector3 vecScale;
        rdMatrix34 scaleMat;

        growDelta = RD_MULTIPLY(pInfo->growthSpeed, secDeltaTime);
        if ( pInfo->curGrowthSize < 0.01f )
        {
            scale = 0.01f / D3DVAL(pInfo->curGrowthSize);
            pInfo->curGrowthSize = 0.01f;
        }
        else
        {
            scale = (pInfo->curGrowthSize + growDelta) / D3DVAL(pInfo->curGrowthSize);
            pInfo->curGrowthSize = pInfo->curGrowthSize + growDelta;
        }

        vecScale.x = vecScale.y = vecScale.z = scale;
        rdMatrix_BuildScale34(&scaleMat, &vecScale);
        rdMatrix_PostMultiply34(&pThing->orient, &scaleMat);
        pThing->renderData.data.pParticle->radius = RD_MULTIPLY(pThing->renderData.data.pParticle->radius, scale);
    }
}

void J3DAPI sithParticle_Initalize(SithThing* pThing)
{
    rdLightMode lightMode = RD_LIGHTING_DIFFUSE;
    SithParticleInfo* pInfo;
    rdParticle* pParticle;
    size_t numCels;
    size_t i;
    size_t celNum;
    float pitch;
    float yaw;
    float dradius;
    float scale;
    rdVector3 pyr;

#line 370
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_PARTICLE));
    pInfo = &pThing->thingInfo.particleInfo;

    if ( pThing->renderData.type == RD_THING_PARTICLE )
    {
        pParticle = rdParticle_Duplicate(pThing->renderData.data.pParticle);
        rdThing_SetParticleCloud(&pThing->renderData, pParticle);
        pInfo->pMaterial    = pParticle->pMaterial;
        pInfo->numParticles = pParticle->numVertices;
    }
    else
    {
        numCels = pInfo->pMaterial->numCels;
        if ( (pInfo->flags & SITHPARTICLE_FLAG_EMIT_LIGHT) != 0 )
        {
            lightMode = RD_LIGHTING_NONE;
        }

        pParticle = rdParticle_New(pInfo->numParticles, pInfo->size, pInfo->pMaterial, lightMode);
        if ( !pParticle )
        {
            return;
        }

        rdThing_SetParticleCloud(&pThing->renderData, pParticle);

        if ( pInfo->pitchRange != 0.0f )
        {
            pitch = RD_MULTIPLY(pInfo->pitchRange, 2.0f);
        }
        else
        {
            pitch = 720.0f;
        }

        if ( pInfo->yawRange != 0.0f )
        {
            yaw = RD_MULTIPLY(pInfo->yawRange, 2.0f);
        }
        else
        {
            yaw = 720.0f;
        }

        dradius = pInfo->maxRadius - pInfo->minRadius;
        dradius = J3DMAX(dradius, 0.0f);

        for ( i = 0; i < pThing->renderData.data.pParticle->numVertices; ++i )
        {
            rdVector_Set3(&pyr, RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pitch), RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, yaw), 0.0f);
            scale = RD_MULTIPLY(D3DVAL(SITH_RAND()), dradius) + pThing->thingInfo.particleInfo.minRadius;
            rdVector_Rotate3(&pThing->renderData.data.pParticle->aVerticies[i], &rdroid_yVector3, &pyr);

            rdVector_Scale3Acc(&pThing->renderData.data.pParticle->aVerticies[i], scale);

            if ( numCels > 1 && (pInfo->flags & SITHPARTICLE_FLAG_RANDOM_START_CEL) != 0 )
            {
                celNum = (size_t)D3DMultiply(D3DVAL(SITH_RAND()), numCels);
                celNum = J3DMIN(celNum, numCels - 1);

                pThing->renderData.data.pParticle->aVertMatCelNums[i] = celNum;
            }
            else
            {
                pThing->renderData.data.pParticle->aVertMatCelNums[i] = -1;
            }
        }

        pThing->renderData.data.pParticle->radius = pInfo->maxRadius;
    }

    pInfo->curGrowthSize = 1.0f;
    if ( (pThing->thingInfo.particleInfo.flags & SITHPARTICLE_FLAG_TIMEOUT_RATE) != 0 )
    {
        pThing->renderData.data.pParticle->numVertices = 1;
    }
}

void J3DAPI sithParticle_DestroyParticle(SithThing* pThing)
{
    size_t dverts;

#line 472
    SITH_ASSERTREL(pThing && (pThing->type == SITH_THING_PARTICLE));
    pThing->thingInfo.particleInfo.flags &= ~SITHPARTICLE_FLAG_TIMEOUT_RATE;

    if ( (pThing->thingInfo.particleInfo.flags & SITHPARTICLE_FLAG_FADE_OUT_OVER_TIME) != 0 )
    {
        dverts = (size_t)RD_MULTIPLY(pThing->thingInfo.particleInfo.timeoutRate, 0.1f);
        dverts = (dverts < 1) ? 1 : dverts;

        if ( dverts >= pThing->renderData.data.pParticle->numVertices )
        {
            sithThing_DestroyThing(pThing);
        }
        else
        {
            pThing->renderData.data.pParticle->numVertices -= dverts;
            pThing->msecLifeLeft = (uint32_t)D3DVAL(RD_MULTIPLY(D3DVAL(SITH_RAND()), 100.0f)) + 1;
        }
    }
    else
    {
        sithThing_DestroyThing(pThing);
    }
}

void J3DAPI sithParticle_Free(SithThing* pThing)
{
    if ( pThing->renderData.data.pParticle )
    {
        rdParticle_Free(pThing->renderData.data.pParticle);
        pThing->renderData.data.pParticle = NULL;
    }
}

void J3DAPI sithParticle_FreeWorldParticles(SithWorld* pWorld)
{
    size_t i;

    for ( i = 0; i < pWorld->numParticles; ++i )
    {
        sithParticle_CacheRemove(&pWorld->aParticles[i]);
        rdParticle_FreeEntry(&pWorld->aParticles[i]);
    }

    stdMemory_Free(pWorld->aParticles);
    pWorld->aParticles    = NULL;
    pWorld->sizeParticles = 0;
    pWorld->numParticles  = 0;
}

rdParticle* J3DAPI sithParticle_CacheFind(const char* pName)
{
#line 561
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithParticle_pHashtable != NULL);

    return (rdParticle*)stdHashtbl_Find(sithParticle_pHashtable, pName);
}

void J3DAPI sithParticle_CacheAdd(rdParticle* pParticle)
{
#line 571
    SITH_ASSERTREL(pParticle != NULL);
    SITH_ASSERTREL(strlen(pParticle->aName) > 0);
    SITH_ASSERTREL(sithParticle_pHashtable != NULL);

    stdHashtbl_Add(sithParticle_pHashtable, pParticle->aName, pParticle);
}

void J3DAPI sithParticle_CacheRemove(const rdParticle* pParticle)
{
#line 582
    SITH_ASSERTREL(pParticle != NULL);
    SITH_ASSERTREL(strlen(pParticle->aName) > 0);
    SITH_ASSERTREL(sithParticle_pHashtable != NULL);

    stdHashtbl_Remove(sithParticle_pHashtable, pParticle->aName);
}
