#include "sithShadow.h"

#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/std.h>
#include <std/General/stdUtil.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithShadow_countfit_1, 17)
CF_PROTO(sithShadow_countfit_2, 17)
CF_PROTO(sithShadow_countfit_3, 17)
CF_PROTO(sithShadow_countfit_4, 9)

/* end count-fit */

static rdVector3 sithShadow_aVertices[4];
static rdVector3 sithShadow_aTransformedVertices[STD_ARRAYLEN(sithShadow_aVertices)];
static rdVector2 sithShadow_aShadowUVs[STD_ARRAYLEN(sithShadow_aVertices)] = { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };

void J3DAPI sithShadow_RenderThingShadow(SithThing* pThing)
{
    float scale;
    float distance;
    float radius;
    float size;
    int bModelShadow;
    rdVector4 pos; // a 4-component vector: the debug frame (0x4E50D0) and retail's both hold 16 bytes here; only x..z are written
    rdMatrix34 shoePos; // Only rvec (left shoe) and lvec (right shoe) are used; the debug frame (0x4E50D0) and retail both hold the whole 48-byte object
    rdMatrix34 orient;
    rdMatrix34 meshOrient;
    rdThing* prdThing;
    int lshoeIdx;
    int rshoeIdx;

    rdVector_Copy3(&pos, &pThing->pos); // rdVector_Copy3 in the debug build's memcpy form, which takes the rdVector4
    if ( !pThing->renderData.data.pModel3 )
    {
        return;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        return;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_WHIPSWINGING:
            if ( sithShadow_SearchForAttachDistance(pThing, &distance, &radius) )
            {
                return;
            }

            scale = 1.0f - (distance - pThing->renderData.data.pModel3->insertOffset.z) / radius;
            break;

        case SITHPLAYERMOVE_CRAWL_STILL:
            if ( sithShadow_SearchForAttachDistance(pThing, &distance, &radius) )
            {
                return;
            }

            scale = 1.0f;
            break;

        default:
            if ( pThing->attach.flags == SITH_ATTACH_SURFACE )
            {
                distance = sithShadow_DistanceThingToSurface(pThing, &pThing->attach.pFace->normal, pThing->attach.pFace);
                scale = 1.0f;
            }
            else
            {
                if ( sithShadow_SearchForAttachDistance(pThing, &distance, &radius) )
                {
                    return;
                }

                scale = 1.0f - (distance - pThing->renderData.data.pModel3->insertOffset.z) / radius;
            }
            break;
    }

    pos.z = pos.z - (distance - 0.002f);

    rdMatrix_Copy34(&orient, &pThing->orient);
    rdVector_Copy3(&orient.dvec, &pos);
    rdVector_Sub3Acc(&orient.dvec, &pThing->renderData.data.pModel3->aHierarchyNodes->pos);

    if ( strneq(pThing->aName, "mine", 4u) )
    {
        size = pThing->renderData.data.pModel3->size;
        bModelShadow = 1;
        sithShadow_DrawShadow(&orient, size, scale, bModelShadow);
        return;
    }

    if ( strneq(pThing->aName, "jeep", 4u) )
    {
        size = pThing->renderData.data.pModel3->size * 1.8f;
        bModelShadow = 1;
        sithShadow_DrawShadow(&orient, size, scale, bModelShadow);
        return;
    }

    prdThing     = &pThing->renderData;
    size         = prdThing->data.pModel3->size;
    bModelShadow = 0;

    lshoeIdx = sithThing_GetThingMeshIndex(pThing, "inlshoe");
    rshoeIdx = sithThing_GetThingMeshIndex(pThing, "inrshoe");
    if ( lshoeIdx != -1 && rshoeIdx != -1 )
    {
        rdModel3_GetMeshMatrix(prdThing, &rdroid_identMatrix34, lshoeIdx, &meshOrient);
        rdVector_Copy3(&shoePos.rvec, &meshOrient.dvec);

        rdModel3_GetMeshMatrix(prdThing, &rdroid_identMatrix34, rshoeIdx, &meshOrient);
        rdVector_Copy3(&shoePos.lvec, &meshOrient.dvec);

        shoePos.rvec.z = pos.z;
        shoePos.lvec.z = shoePos.rvec.z;

        if ( pThing->moveStatus == SITHPLAYERMOVE_CRAWL_STILL )
        {
            sithShadow_DrawShadow(&orient, size, scale, 0);
        }
        else
        {
            sithShadow_DrawWalkShadow(size, scale, &shoePos.rvec, &shoePos.lvec, &orient.lvec, &orient.rvec);
        }
    }
    else
    {
        sithShadow_DrawShadow(&orient, size, scale, bModelShadow);
    }
}

int J3DAPI sithShadow_SearchForAttachDistance(SithThing* pThing, float* distance, float* radius)
{
    float castRadius = 0.0f;
    int physicsFlags = pThing->moveInfo.physics.flags;  // unreferenced; the debug build's frame keeps its slot
    int bCheckFloor = 0;
    rdVector3 moveNorm;
    float castDist;
    SithCollision* pCollision;

#line 168
    SITH_ASSERTREL(pThing);
    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        return 1;
    }

    if ( !pThing->renderData.data.pModel3 )
    {
        return 1;
    }

    if ( !pThing->pInSector )
    {
        return 1;
    }

    rdVector_Neg3(&moveNorm, &rdroid_zVector3);

    castDist = sithPhysics_GetThingHeight(pThing);
    if ( castDist == 0.0f )
    {
        castDist = pThing->collide.movesize + 0.005f;
    }

    castDist = castDist * 4.0f;
    *radius = castDist;
    if ( castDist <= 0.0f )
    {
        goto not_found;
    }

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, castDist, castRadius, 0x2812);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            *distance = sithShadow_DistanceThingToSurface(pThing, &pCollision->pSurfaceCollided->face.normal, &pCollision->pSurfaceCollided->face);
            sithCollision_DecreaseStackLevel();
            return 0;
        }

        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            rdVector3 hitPoint;
            rdVector3 hitfaceVert;
            rdVector3 hitDir;
            SithThing* pHitThing;

            pHitThing = pCollision->pThingCollided;
#line 216
            SITH_ASSERTREL(pHitThing);
            if ( pHitThing == pThing )
            {
                continue;
            }

            if ( !pCollision->pFaceCollided || !pCollision->pMeshCollided )
            {
#line 223
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Hit standon object but not enough collide info to attach.\n", 0, 0, 0, 0);
                sithCollision_DecreaseStackLevel();
                return 1;
            }

            rdMatrix_TransformVector34(&hitDir, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);
            if ( (bCheckFloor & 0x10) != 0 && rdVector_Dot3(&hitDir, &rdroid_zVector3) < 0.6f )
            {
                continue;
            }

            rdVector_Copy3(&hitfaceVert, &pCollision->pMeshCollided->apVertices[*pCollision->pFaceCollided->aVertices]);
            rdMatrix_TransformVector34(&hitDir, &pCollision->pFaceCollided->normal, &pHitThing->orient);
            rdMatrix_TransformVector34(&hitPoint, &hitfaceVert, &pHitThing->orient);
            rdVector_Add3Acc(&hitPoint, &pHitThing->pos);

            *distance = rdMath_DistancePointToPlane(&pThing->pos, &hitDir, &hitPoint);
            sithCollision_DecreaseStackLevel();
            return 0;
        }
    }

    sithCollision_DecreaseStackLevel();
    return 1;

not_found:
    return 1;
}

float J3DAPI sithShadow_DistanceThingToSurface(const SithThing* pThing, const rdVector3* normal, const rdFace* pFace)
{
    return rdMath_DistancePointToPlane(&pThing->pos, normal, &sithWorld_pCurrentWorld->aVertices[*pFace->aVertices]);
}

static const rdVector3 sithShadow_yVector3 = { 0.0f, 1.0f, 0.0f };
static const rdVector3 sithShadow_xVector3 = { 1.0f, 0.0f, 0.0f };

void J3DAPI sithShadow_DrawShadow(const rdMatrix34* orient, float size, float scale, int bCar)
{
    rdMaterial* pMat;
    rdVector3 localShadowOffset;
    rdMatrix34 tmat;
    rdCacheProcEntry* pAlphaProc;
    float curShadowAlpha;
    float shsize;

    shsize = D3DVAL(RD_MULTIPLY(size, scale) / 2.0f);
    pMat   = NULL;
    localShadowOffset.x = 0.0f;
    localShadowOffset.y = 0.0f;
    localShadowOffset.z = 0.0f;
    if ( scale <= 0.0f )
    {
        return;
    }

    if ( bCar )
    {
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(2)); // jep_4shadow.mat
    }
    else
    {
        pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(0)); // gen_4indy_shadow.mat
    }

    if ( !pMat )
    {
        return;
    }

    pAlphaProc = rdCache_GetAlphaProcEntry();
    if ( !pAlphaProc )
    {
        return;
    }

    rdMatrix_Multiply34(&tmat, &rdCamera_pCurCamera->orient, orient);

    rdVector_Set3(&sithShadow_aVertices[0], -shsize, -shsize, 0.0f);

    rdVector_Set3(&sithShadow_aVertices[1], shsize, -shsize, 0.0f);

    rdVector_Set3(&sithShadow_aVertices[2], shsize, shsize, 0.0f);

    rdVector_Set3(&sithShadow_aVertices[3], -shsize, shsize, 0.0f);

    rdMatrix_TransformPointList34(&tmat, sithShadow_aVertices, sithShadow_aTransformedVertices, STD_ARRAYLEN(sithShadow_aTransformedVertices));
    rdClip_VerticesToPlane(pAlphaProc, sithShadow_aTransformedVertices, sithShadow_aShadowUVs, STD_ARRAYLEN(sithShadow_aShadowUVs));

    pAlphaProc->lightingMode = RD_LIGHTING_GOURAUD;
    pAlphaProc->flags        = RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pAlphaProc->pMaterial    = pMat;
    pAlphaProc->matCelNum    = -1;

    curShadowAlpha = RD_MULTIPLY(scale, 0.8f) + 0.2f;
    rdVector_Set4(&pAlphaProc->aVertIntensities[0], 1.0f, 1.0f, 1.0f, curShadowAlpha);
    rdVector_Set4(&pAlphaProc->aVertIntensities[1], 1.0f, 1.0f, 1.0f, curShadowAlpha);
    rdVector_Set4(&pAlphaProc->aVertIntensities[2], 1.0f, 1.0f, 1.0f, curShadowAlpha);
    rdVector_Set4(&pAlphaProc->aVertIntensities[3], 1.0f, 1.0f, 1.0f, curShadowAlpha);
    rdVector_Set4(&pAlphaProc->extraLight, 1.0f, 1.0f, 1.0f, 1.0f);

    rdCache_AddAlphaProcFace(4);
}

void J3DAPI sithShadow_DrawWalkShadow(float size, float scale, const rdVector3* leg, const rdVector3* rleg, const rdVector3* lvec, const rdVector3* rvec)
{
    rdMaterial* pMat;
    rdCacheProcEntry* pPoly;
    rdVector3 ldir;
    rdVector3 rdir;
    float legSize;
    float escale;
    float sx;
    float sy;
    float ex;
    float ey;
    float alpha;

    legSize = D3DVAL(RD_MULTIPLY(size, scale)) / 8.0f;
    escale  = RD_MULTIPLY(0.0075f, scale);
    pMat    = NULL;
    if ( scale <= 0.0f )
    {
        return;
    }

    rdVector_Normalize3(&ldir, lvec);
    rdVector_Normalize3(&rdir, rvec);

    pMat = sithMaterial_GetMaterialByIndex(SITHWORLD_STATICINDEX(1));
    if ( !pMat )
    {
        return;
    }

    pPoly = rdCache_GetAlphaProcEntry();

    sx = rdVector_Dot3(&sithShadow_xVector3, &ldir) * legSize;
    sy = rdVector_Dot3(&sithShadow_yVector3, &ldir) * legSize;
    ex = rdVector_Dot3(&sithShadow_xVector3, &rdir) * escale;
    ey = rdVector_Dot3(&sithShadow_yVector3, &rdir) * escale;

    sithShadow_aVertices[0].x = leg->x - sx - ex;
    sithShadow_aVertices[0].y = leg->y - sy - ey;
    sithShadow_aVertices[0].z = leg->z;

    sithShadow_aVertices[1].x = leg->x + sx - ex;
    sithShadow_aVertices[1].y = leg->y + sy - ey;
    sithShadow_aVertices[1].z = leg->z;

    sithShadow_aVertices[2].x = rleg->x + sx + ex;
    sithShadow_aVertices[2].y = rleg->y + sy + ey;
    sithShadow_aVertices[2].z = rleg->z;

    sithShadow_aVertices[3].x = rleg->x - sx + ex;
    sithShadow_aVertices[3].y = rleg->y - sy + ey;
    sithShadow_aVertices[3].z = rleg->z;

    rdMatrix_TransformPointList34(&rdCamera_pCurCamera->orient, sithShadow_aVertices, sithShadow_aTransformedVertices, STD_ARRAYLEN(sithShadow_aTransformedVertices));
    rdClip_VerticesToPlane(pPoly, sithShadow_aTransformedVertices, sithShadow_aShadowUVs, STD_ARRAYLEN(sithShadow_aShadowUVs));

    pPoly->lightingMode = RD_LIGHTING_GOURAUD;
    pPoly->flags        = RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pPoly->pMaterial    = pMat;
    pPoly->matCelNum    = -1;

    alpha = RD_MULTIPLY(scale, 0.8f) + 0.2f;
    rdVector_Set4(&pPoly->aVertIntensities[0], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[1], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[2], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->aVertIntensities[3], 1.0f, 1.0f, 1.0f, alpha);
    rdVector_Set4(&pPoly->extraLight, 1.0f, 1.0f, 1.0f, 1.0f);

    rdCache_AddAlphaProcFace(4);
}
