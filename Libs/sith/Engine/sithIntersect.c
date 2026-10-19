#include "sithIntersect.h"

#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Main/sithMain.h>
#include <sith/World/sithSector.h>

#include <std/General/stdMath.h>
/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithIntersect_countfit_1, 15)
CF_PROTO(sithIntersect_countfit_2, 15)
CF_PROTO(sithIntersect_countfit_3, 15)
CF_PROTO(sithIntersect_countfit_4, 15)
CF_PROTO(sithIntersect_countfit_5, 15)
CF_PROTO(sithIntersect_countfit_6, 15)
CF_PROTO(sithIntersect_countfit_7, 15)
CF_PROTO(sithIntersect_countfit_8, 15)
CF_PROTO(sithIntersect_countfit_9, 15)
CF_PROTO(sithIntersect_countfit_10, 15)
CF_PROTO(sithIntersect_countfit_11, 15)
CF_PROTO(sithIntersect_countfit_12, 15)
CF_PROTO(sithIntersect_countfit_13, 15)
CF_PROTO(sithIntersect_countfit_14, 15)
CF_PROTO(sithIntersect_countfit_15, 15)
CF_PROTO(sithIntersect_countfit_16, 15)
CF_PROTO(sithIntersect_countfit_17, 12)

/* end count-fit */


#define SITHINTERSECT_ABS(x) ((x) < 0.0f ? -(x) : (x))

static size_t sithIntersect_aFaceProjectionAxes[3][2] = { { 2, 1 }, { 0, 2 }, { 1, 0 } };

// Linker-discarded (no retail caller): restored from the debug build (0x4DE7A0, the first function of the debug
// image's sithIntersect object). The name is inferred. Allowlisted under `discarded:` in relocations.yml.
float J3DAPI sithIntersect_CheckSphereSectorHit(const SithSector* pSector, const SithWorld* pWorld, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius)
{
    float hitDist;
    SithSurface* pSurface;
    size_t i;

    pSurface = pSector->pFirstSurface;
    for ( i = 0; i < pSector->numSurfaces; ++pSurface, ++i )
    {
        if ( sithIntersect_CheckSphereHit(startPos, moveNorm, moveDist, radius, &pSurface->face.normal, &pWorld->aVertices[*pSurface->face.aVertices], &hitDist, 0) )
        {
            moveDist = hitDist;
        }
    }

    return moveDist;
}

int J3DAPI sithIntersect_IsSphereInSector(const SithWorld* pWorld, const rdVector3* pos, float radius, const SithSector* pSector)
{
    SithSurface* pCurrentSurf;
    size_t i;

#line 133
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(pWorld, pSector));
    SITH_ASSERTREL(pos != NULL);
    SITH_ASSERTREL(radius >= ((float)0.0));

    if ( (pSector->flags & SITH_SECTOR_HASCOLLIDEBOX) != 0
        && pos->z - radius > pSector->collideBox.v0.z
        && pos->y - radius > pSector->collideBox.v0.y
        && pos->x - radius > pSector->collideBox.v0.x
        && pos->x + radius < pSector->collideBox.v1.x
        && pos->y + radius < pSector->collideBox.v1.y
        && pos->z + radius < pSector->collideBox.v1.z )
    {
        return 1;
    }

    pCurrentSurf = pSector->pFirstSurface;
    for ( i = 0; i < pSector->numSurfaces; ++pCurrentSurf, ++i )
    {
        SithSurfaceAdjoin* pAdjoin = pCurrentSurf->pAdjoin;
        if ( (pCurrentSurf->flags & SITH_SURFACE_COLLISION) != 0 || pAdjoin && (pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            float dist;
            const rdVector3* pVert = &pWorld->aVertices[pCurrentSurf->face.aVertices[0]];
            dist = rdMath_DistancePointToPlane(pos, &pCurrentSurf->face.normal, pVert);
            dist = stdMath_ClipNearZero(dist);
            if ( dist < radius )
            {
                return 0;
            }
        }
    }

    return 1;
}

SithCollisionType J3DAPI sithIntersect_CheckFaceVerticesIntersection(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, rdVector3* pHitPos)
{
    int hitmask;
    if ( !sithIntersect_TestSphereFaceHit(startPos, radius, pFace, aVertices, &hitmask) )
    {
        return 0;
    }

    if ( !hitmask )
    {
        return SITHCOLLISION_FACE;
    }

#line 220
    SITH_ASSERTREL(radius > ((float)0.0));
    return sithIntersect_CheckSphereFaceHitVerticesIntersection(startPos, radius, pFace, aVertices, hitmask, pHitPos);
}


/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithIntersect_countfit_18, 15)
CF_PROTO(sithIntersect_countfit_19, 15)
CF_PROTO(sithIntersect_countfit_20, 15)
CF_PROTO(sithIntersect_countfit_21, 15)
CF_PROTO(sithIntersect_countfit_22, 15)
CF_PROTO(sithIntersect_countfit_23, 15)
CF_PROTO(sithIntersect_countfit_24, 15)
CF_PROTO(sithIntersect_countfit_25, 15)
CF_PROTO(sithIntersect_countfit_26, 15)
CF_PROTO(sithIntersect_countfit_27, 15)
CF_PROTO(sithIntersect_countfit_28, 15)
CF_PROTO(sithIntersect_countfit_29, 15)
CF_PROTO(sithIntersect_countfit_30, 15)
CF_PROTO(sithIntersect_countfit_31, 14)
/* end count-fit */

SithCollisionType J3DAPI sithIntersect_CheckSphereThingIntersection(SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, SithThing* pCheck, int colflags, float* pHitDistance, rdModel3Mesh** ppHitMesh, rdFace** ppHitFace, rdVector3* hitNorm)
{
    int bFaceCol = 0;
    float hitDistance;
    rdMatrix34 tmat;
    SithCollisionType hitType;

#line 283
    SITH_ASSERTREL(pCheck->collide.type != SITH_COLLIDE_NONE);
    SITH_ASSERTREL(pHitDistance != NULL);
    SITH_ASSERTREL(ppHitMesh && ppHitFace);

    if ( (pCheck->flags & SITH_TF_JEEPSTOP) != 0 )
    {
        if ( !pThing || pThing->type != SITH_THING_PLAYER )
        {
            return 0;
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            if ( rdVector_Dot3(&pCheck->orient.lvec, &pThing->moveInfo.physics.velocity) >= 0.0f ) // if not facing same direction
            {
                return 0;
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) == 0 )
        {
            return 0;
        }
    }

    if ( (colflags & 0x80) == 0 && (pCheck->collide.type == SITH_COLLIDE_FACE || pThing && pThing->collide.type == SITH_COLLIDE_FACE) )
    {
        bFaceCol = 1;
    }

    if ( !sithIntersect_CheckSphereIntersection(startPos, moveNorm, moveDist, radius, &pCheck->pos, pCheck->collide.size, &hitDistance, bFaceCol, colflags) )
    {
        return 0;
    }

    if ( !bFaceCol )
    {
        rdVector_ScaleAdd3(hitNorm, moveNorm, hitDistance, startPos);

        rdVector_Sub3Acc(hitNorm, &pCheck->pos);
        rdVector_Normalize3Acc(hitNorm);

        *pHitDistance = hitDistance;
        return SITHCOLLISION_THING;
    }


    {
    rdVector3 dir, pos;
    int bThingFaceCol = 0;
    SithThing* pComplex = NULL;

    if ( pCheck->collide.type == SITH_COLLIDE_FACE )
    {
        bThingFaceCol = 0;
        pComplex      = pCheck;

        rdVector_Copy3(&dir, moveNorm);
        rdVector_Copy3(&pos, startPos);
    }
    else
    {
#line 376
        SITH_ASSERTREL(pThing && (pThing->collide.type != SITH_COLLIDE_SPHERE));

        pComplex     = pThing;
        radius       = pCheck->collide.size;
        bThingFaceCol = 1;

        rdVector_Neg3(&dir, moveNorm);
        rdVector_Copy3(&pos, &pCheck->pos);
    }

#line 390
    SITH_ASSERTREL(pComplex->renderData.type == RD_THING_MODEL3);

    
    rdVector_Copy3(&pComplex->orient.dvec, &pComplex->pos); // TODO: Verify if this isn't a bug setting dvec here; might need tmp mat instead 
    rdMatrix_InvertOrtho34(&tmat, &pComplex->orient);
    rdMatrix_TransformPoint34Acc(&pos, &tmat);
    rdMatrix_TransformVector34Acc(&dir, &tmat);

    hitType = sithIntersect_CheckSphereThingModelIntersection(&pos, &dir, moveDist, radius, &pComplex->renderData, pHitDistance, ppHitFace, ppHitMesh, hitNorm);
    if ( hitType )
    {
        rdMatrix_TransformVector34Acc(hitNorm, &pComplex->orient);
        if ( bThingFaceCol )
        {
            rdVector_Neg3Acc(hitNorm);
        }

        hitType |= SITHCOLLISION_THING;
    }

    return hitType;
}
}

SithCollisionType J3DAPI sithIntersect_CheckSphereThingModelIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdThing* prdThing, float* hitDistance, rdFace** ppFace, rdModel3Mesh** ppMesh, rdVector3* hitNorm)
{
    SithCollisionType hitType = 0;
    rdModel3GeoSet* pGeos;
    size_t i;
    SithCollisionType curHitType;

#line 499
    SITH_ASSERTREL(prdThing != NULL);
    SITH_ASSERTREL(prdThing->type == RD_THING_MODEL3);
    SITH_ASSERTREL(prdThing->data.pModel3 != NULL);
    SITH_ASSERTREL(ppMesh && ppFace);
    SITH_ASSERTREL(hitNorm);

    pGeos = prdThing->data.pModel3->aGeos;
    for ( i = 0; i < pGeos->numMeshes; ++i )
    {
        curHitType = sithIntersect_CheckSphereMeshIntersection(startPos, moveNorm, moveDist, radius, &pGeos->aMeshes[i], hitDistance, ppFace, hitNorm);
        if ( curHitType )
        {
            *ppMesh  = &pGeos->aMeshes[i];
            moveDist = *hitDistance;
            hitType = curHitType;
        }
    }

    return hitType;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereMeshIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdModel3Mesh* pMesh, float* hitDist, rdFace** ppHitFace, rdVector3* hitNorm)
{
    SithCollisionType currenthitType = 0;
    float maxHitDistance = 1.0f;
    size_t i;
    rdVector3 curHitNorm;
    SithCollisionType curHit;

#line 556
    SITH_ASSERTREL(hitNorm);

    for ( i = 0; i < pMesh->numFaces; ++i )
    {
        curHit = sithIntersect_CheckSphereFaceIntersectionEx(startPos, moveNorm, moveDistance, radius, &pMesh->aFaces[i], pMesh->apVertices, hitDist, &curHitNorm, 0);
        if ( curHit && (*hitDist < moveDistance
            || (currenthitType != SITHCOLLISION_FACE && curHit == SITHCOLLISION_FACE)
            || rdVector_Dot3(&pMesh->aFaces[i].normal, moveNorm) < maxHitDistance) )
        {
            *ppHitFace   = &pMesh->aFaces[i];
            moveDistance = *hitDist;
            currenthitType      = curHit;
            rdVector_Copy3(hitNorm, &curHitNorm);
            maxHitDistance   = rdVector_Dot3(&pMesh->aFaces[i].normal, moveNorm);
        }
    }

    return currenthitType;
}

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithIntersect_countfit_32, 20)
/* end count-fit */

int J3DAPI sithIntersect_CheckSphereIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, const rdVector3* endPos, float size, float* hitDistance, int bCheckEndPos, int colflags)
{
    float distanceSq;
    rdVector3 pos;
    float dist;
    rdVector3 dir;
    float toClosest;
    float maxDistSq;
    float maxDist;

    maxDist = radius + size;
    rdVector_Sub3(&dir, endPos, startPos);

    if ( moveDist == 0.0f )
    {
        goto check_end;
    }

    toClosest = rdVector_Dot3(moveNorm, &dir);
    if ( toClosest < 0.0f || moveDist + maxDist < toClosest )
    {
        if ( bCheckEndPos )
        {
            goto check_end;
        }

        return 0;
    }

    rdVector_ScaleAdd3(&pos, moveNorm, toClosest, startPos);
    dist = sqrtf(((endPos->x - pos.x) * (endPos->x - pos.x)) + ((endPos->y - pos.y) * (endPos->y - pos.y)) + ((endPos->z - pos.z) * (endPos->z - pos.z)));
    if ( dist < maxDist )
    {
        distanceSq = dist * dist;
        dist = toClosest - stdMath_sqrtf((maxDistSq = maxDist * maxDist) - distanceSq);
        if ( dist > moveDist || dist < 0.0f )
        {
            *hitDistance = 0.0f;
            return 1;
        }

        *hitDistance = dist;
        return 1;
    }

    return 0;

check_end:
    dist = rdVector_Len3(&dir);
    if ( dist < maxDist )
    {
        if ( (colflags & 0x400) != 0 )
        {
            *hitDistance = dist;
        }
        else
        {
            *hitDistance = 0.0f;
        }

        return 1;
    }

    return 0;
}

static __inline void J3DAPI sithIntersect_GetProjectionAxes(const rdVector3* pNormal, int* pAxis1, int* pAxis2);

int J3DAPI sithIntersect_TestSphereFaceHit(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, int* pHitMask)
{
    int curEdgeMask;
    int vertNum;
    int axis1, axis2;
    rdVector2 curSphPos;
    int numVertices;
    int nextIdx;
    rdVector2 nedge;
    rdVector2 edgeVec;

    if ( pHitMask )
    {
        *pHitMask = 0;
    }

    sithIntersect_GetProjectionAxes(&pFace->normal, &axis1, &axis2);

    curSphPos.x = *(&startPos->x + axis1);
    curSphPos.y = *(&startPos->x + axis2);

    numVertices = pFace->numVertices;
    for ( vertNum = 0, curEdgeMask = 1; vertNum < numVertices; ++vertNum, curEdgeMask <<= 1 )
    {
        nedge.x = edgeVec.x = -*(&aVertices[pFace->aVertices[vertNum]].x + axis1); // Get n-th component of vertex, i.e. x, y or z
        nedge.y = edgeVec.y = -*(&aVertices[pFace->aVertices[vertNum]].x + axis2); // Get n-th component of vertex, i.e. x, y or z

        edgeVec.x += curSphPos.x;
        edgeVec.y += curSphPos.y;

        nextIdx = (vertNum + 1) % numVertices;
        nedge.x += *(&aVertices[pFace->aVertices[nextIdx]].x + axis1); // Get n-th component of next vertex, i.e. x, y or z
        nedge.y += *(&aVertices[pFace->aVertices[nextIdx]].x + axis2); // Get n-th component of next vertex, i.e. x, y or z

        if ( RD_MULTIPLY(edgeVec.x, nedge.y) - RD_MULTIPLY(edgeVec.y, nedge.x) < 0.0f )
        {
            if ( radius == 0.0f || !pHitMask )
            {
                return 0;
            }

            rdVector_Normalize2Acc(&nedge);
            if ( RD_MULTIPLY(edgeVec.x, nedge.y) - RD_MULTIPLY(edgeVec.y, nedge.x) < -radius )
            {
                return 0;
            }

            *pHitMask |= curEdgeMask;
        }
    }

    return 1;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereFaceHitVerticesIntersection(const rdVector3* startPos, float radius, const rdFace* pFace, const rdVector3* aVertices, unsigned int vertHitMask, rdVector3* pHitPos)
{
    SithCollisionType hitType = 0;
    float maxHitDist = radius + 1.0f;
    size_t vertNum;
    rdVector3 dVertStartPos;
    rdVector3 dverts;
    float vertdist;
    float dot;
    const rdVector3* pVert;
    const rdVector3* pNextVert;
    float hitDist;
    rdVector3 hitPos;
    SithCollisionType curHitType;

#line 833
    SITH_ASSERTREL(pFace != NULL);
    SITH_ASSERTREL(radius >= ((float)0.0));


    for ( vertNum = 0; vertNum < pFace->numVertices; ++vertNum, vertHitMask >>= 1 )
    {
        if ( !vertHitMask )
        {
            break;
        }

        if ( (vertHitMask & 1) == 0 )
        {
            continue;
        }

        pVert = &aVertices[pFace->aVertices[vertNum]];
        pNextVert = &aVertices[pFace->aVertices[(vertNum + 1) % pFace->numVertices]];

        rdVector_Sub3(&dVertStartPos, startPos, pVert);
        rdVector_Sub3(&dverts, pNextVert, pVert);

        vertdist = rdVector_Normalize3Acc(&dverts);
        dot = rdVector_Dot3(&dverts, &dVertStartPos);

        if ( -radius > dot || dot - radius > vertdist )
        {
            continue;
        }

        curHitType = SITHCOLLISION_FACEVERTEX;
        if ( dot < 0.0f )
        {
            rdVector_Copy3(&hitPos, pVert);
        }
        else if ( dot > vertdist )
        {
            rdVector_Copy3(&hitPos, pNextVert);
        }
        else
        {
            curHitType = SITHCOLLISION_FACEEDGE;
            rdVector_ScaleAdd3(&hitPos, &dverts, dot, pVert);
        }

        hitDist = sqrtf(RD_MULTIPLY(startPos->x - hitPos.x, startPos->x - hitPos.x)
            + RD_MULTIPLY(startPos->y - hitPos.y, startPos->y - hitPos.y)
            + RD_MULTIPLY(startPos->z - hitPos.z, startPos->z - hitPos.z));
        if ( hitDist <= radius && hitDist < maxHitDist )
        {
            hitType = curHitType;
            maxHitDist = hitDist;
            if ( pHitPos )
            {
                rdVector_Copy3(pHitPos, &hitPos);
            }
        }
    }

    return hitType;
}

int J3DAPI sithIntersect_CheckSphereHit(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdVector3* normal, const rdVector3* point, float* pSphereHitDist, int colflags)
{
    float curdot;
    float moveDot;

    float curdist = rdMath_DistancePointToPlane(startPos, normal, point);
    curdist = stdMath_ClipNearZero(curdist);
    if ( curdist < 0.0f )
    {
        return 0;
    }

    curdist -= radius;
    if ( curdist > moveDistance )
    {
        return 0;
    }

    curdot = -rdVector_Dot3(normal, moveNorm);
    if ( curdist < 0.0f )
    {
        if ( (colflags & 0x400) != 0 )
        {
            *pSphereHitDist += radius;
        }
        else
        {
            *pSphereHitDist = 0.0f;
        }

        return 1;
    }

    if ( curdot <= 0.0f )
    {
        return 0;
    }

    if ( (moveDot = RD_MULTIPLY(curdot, moveDistance)) < curdist )
    {
        return 0;
    }

    *pSphereHitDist = curdist / curdot;
    *pSphereHitDist = 0.0f > *pSphereHitDist ? 0.0f : *pSphereHitDist;

#line 976
    SITH_ASSERTREL((*pSphereHitDist <= moveDistance) && (*pSphereHitDist >= ((float)0.0)));
    return 1;
}





SithCollisionType J3DAPI sithIntersect_CheckSphereFaceIntersectionEx(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdFace* pFace, const rdVector3* aVertices, float* hitDist, rdVector3* hitNorm, int colflags)
{
    rdVector3 hitPos[1];
    rdVector3 vertexHitPos;
    float dist;
    SithCollisionType hitType;

#line 1018
    SITH_ASSERTREL((startPos != NULL) && (pFace != NULL) && (aVertices != NULL));
    SITH_ASSERTREL((hitPos != NULL) && (moveNorm != NULL));
    SITH_ASSERTREL(radius >= ((float)0.0));

    if ( !sithIntersect_CheckSphereHit(startPos, moveNorm, moveDistance, radius, &pFace->normal, &aVertices[*pFace->aVertices], hitDist, colflags) )
    {
        return 0;
    }

    if ( (colflags & 0x400) == 0 && rdVector_Dot3(moveNorm, &pFace->normal) >= 0.0f )
    {
        return 0;
    }

    
    if ( *hitDist != 0.0f )
    {
        rdVector_ScaleAdd3(hitPos, moveNorm, *hitDist, startPos);

        dist = rdMath_DistancePointToPlane(hitPos, &pFace->normal, &aVertices[*pFace->aVertices]);
        dist = stdMath_ClipNearZero(dist);
        if ( dist != 0.0f )
        {
            rdVector_ScaleAdd3(hitPos, &pFace->normal, -dist, hitPos);
        }
    }
    else
    {
        rdVector_Copy3(hitPos, startPos);

        dist = rdMath_DistancePointToPlane(hitPos, &pFace->normal, &aVertices[*pFace->aVertices]);
        dist = stdMath_ClipNearZero(dist);
        if ( dist != 0.0f )
        {
            rdVector_ScaleAdd3(hitPos, &pFace->normal, -dist, hitPos);
        }
    }

    if ( hitNorm )
    {
        hitType = sithIntersect_CheckFaceVerticesIntersection(hitPos, radius, pFace, aVertices, &vertexHitPos);
        if ( hitType == SITHCOLLISION_FACEEDGE || hitType == SITHCOLLISION_FACEVERTEX )
        {
            rdVector_Sub3(hitNorm, startPos, &vertexHitPos);
            rdVector_Normalize3Acc(hitNorm);
        }
        else
        {
            rdVector_Copy3(hitNorm, &pFace->normal);
        }
    }
    else
    {
        hitType = sithIntersect_CheckFaceVerticesIntersection(hitPos, radius, pFace, aVertices, NULL);
    }

    return hitType;
}

SithCollisionType J3DAPI sithIntersect_CheckSphereFaceIntersection(const rdVector3* startPos, const rdVector3* moveNorm, float moveDistance, float radius, const rdFace* pFace, const rdVector3* aVertices, float* hitDist, int flags)
{
    rdVector3 hitPos[1];
    SithCollisionType hitType;

#line 1097
    SITH_ASSERTREL((startPos != NULL) && (pFace != NULL) && (aVertices != NULL));
    SITH_ASSERTREL((hitPos != NULL) && (moveNorm != NULL));
    SITH_ASSERTREL(radius >= ((float)0.0));
    if ( !sithIntersect_CheckSphereHit(startPos, moveNorm, moveDistance, radius, &pFace->normal, &aVertices[*pFace->aVertices], hitDist, flags) )
    {
        return 0;
    }

    if ( radius != 0.0f )
    {
        return SITHCOLLISION_FACE;
    }

    
    rdVector_ScaleAdd3(hitPos, moveNorm, *hitDist, startPos);
    hitType = sithIntersect_CheckFaceVerticesIntersection(hitPos, radius, pFace, aVertices, 0);
    return hitType;
}

// Linker-discarded (in the debug build its only caller is CheckSphereThingIntersection's `if ( 0 )` arm, which this
// source does not reproduce): restored from the debug build (0x4E0140, after CheckSphereFaceIntersection). Name inferred.
// No other function calls it.
int J3DAPI sithIntersect_CheckThingBoundBoxIntersection(const SithThing* pThing1, const SithThing* pThing2, const rdVector3* moveNorm, float moveDist, float* hitDist)
{
    int bIntersectX = 0;
    int bIntersectY = 0;
    int bIntersectZ = 0;
    rdVector3 center;
    rdVector3 bbMin1;
    rdVector3 bbMax1;
    rdVector3 bbMin2;
    rdVector3 bbMax2;
    float dist;

    rdVector_Copy3(&center, &pThing1->pos);
    rdVector_ScaleAdd3Acc(&center, moveNorm, moveDist);

    bbMin1.x = center.x - pThing1->collide.width;
    bbMin1.y = center.y - pThing1->collide.width;
    bbMin1.z = center.z - pThing1->collide.height;

    bbMax1.x = center.x + pThing1->collide.width;
    bbMax1.y = center.y + pThing1->collide.width;
    bbMax1.z = center.z + pThing1->collide.height;

    bbMin2.x = pThing2->pos.x - pThing2->collide.width;
    bbMin2.y = pThing2->pos.y - pThing2->collide.width;
    bbMin2.z = pThing2->pos.z - pThing2->collide.height;

    bbMax2.x = pThing2->pos.x + pThing2->collide.width;
    bbMax2.y = pThing2->pos.y + pThing2->collide.width;
    bbMax2.z = pThing2->pos.z + pThing2->collide.height;

    *hitDist = 0.0f;

    if ( bbMax1.x > bbMin2.x && bbMax1.x < bbMax2.x )
    {
        bIntersectX = 1;
        *hitDist = bbMin2.x - pThing1->pos.x;
    }

    if ( !bIntersectX )
    {
        if ( bbMin1.x < bbMax2.x && bbMin1.x > bbMin2.x )
        {
            bIntersectX = 1;
            dist = pThing1->pos.x - bbMax2.x;
            if ( dist > *hitDist )
            {
                *hitDist = dist;
            }
        }

        if ( !bIntersectX )
        {
            if ( bbMin1.x > bbMin2.x && bbMax1.x < bbMax2.x )
            {
                bIntersectX = 1;
            }

            if ( !bIntersectX && bbMin2.x > bbMin1.x && bbMax2.x < bbMax1.x )
            {
                bIntersectX = 1;
            }
        }
    }

    if ( bIntersectX == 1 )
    {
        if ( bbMax1.y > bbMin2.y && bbMax1.y < bbMax2.y )
        {
            bIntersectY = 1;
            dist = bbMin2.y - pThing1->pos.y;
            if ( dist > *hitDist )
            {
                *hitDist = dist;
            }
        }

        if ( !bIntersectY && bbMin1.y < bbMax2.y && bbMin1.y > bbMin2.y )
        {
            bIntersectY = 1;
            dist = pThing1->pos.y - bbMax2.y;
            if ( dist > *hitDist )
            {
                *hitDist = dist;
            }
        }

        if ( !bIntersectY )
        {
            if ( bbMin1.y > bbMin2.y && bbMax1.y < bbMax2.y )
            {
                bIntersectY = 1;
            }

            if ( !bIntersectY && bbMin2.y > bbMin1.y && bbMax2.y < bbMax1.y )
            {
                bIntersectY = 1;
            }
        }
    }

    if ( bIntersectY == 1 )
    {
        if ( bbMax1.z > bbMin2.z && bbMax1.z < bbMax2.z )
        {
            bIntersectZ = 1;
            dist = bbMin2.z - pThing1->pos.z;
            if ( dist > *hitDist )
            {
                *hitDist = dist;
            }
        }

        if ( !bIntersectZ && bbMin1.z < bbMax2.z && bbMin1.z > bbMin2.z )
        {
            bIntersectZ = 1;
            dist = pThing1->pos.z - bbMax2.z;
            if ( dist > *hitDist )
            {
                *hitDist = dist;
            }
        }

        if ( !bIntersectZ )
        {
            if ( bbMin1.z > bbMin2.z && bbMax1.z < bbMax2.z )
            {
                bIntersectZ = 1;
            }

            if ( !bIntersectZ && bbMin2.z > bbMin1.z && bbMax2.z < bbMax1.z )
            {
                bIntersectZ = 1;
            }
        }
    }

    if ( bIntersectX && bIntersectY && bIntersectZ )
    {
        return 1;
    }

    return 0;
}

static __inline void J3DAPI sithIntersect_GetProjectionAxes(const rdVector3* pNormal, int* pAxis1, int* pAxis2)
{
    float x;
    float y;
    float z;
    size_t idx;

    x = pNormal->x < 0.0f ? -pNormal->x : pNormal->x;
    y = SITHINTERSECT_ABS(pNormal->y);
    z = SITHINTERSECT_ABS(pNormal->z);

    if ( x > y )
    {
        if ( x > z )
        {
            idx = 0;
        }
        else
        {
            idx = 2;
        }
    }
    else if ( y > z )
    {
        idx = 1;
    }
    else
    {
        idx = 2;
    }

    if ( *(&pNormal->x + idx) > 0.0f )
    {
        *pAxis1 = sithIntersect_aFaceProjectionAxes[idx][0];
        *pAxis2 = sithIntersect_aFaceProjectionAxes[idx][1];
    }
    else
    {
        *pAxis1 = sithIntersect_aFaceProjectionAxes[idx][1];
        *pAxis2 = sithIntersect_aFaceProjectionAxes[idx][0];
    }
}
