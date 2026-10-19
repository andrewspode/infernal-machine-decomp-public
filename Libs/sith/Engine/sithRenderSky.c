#include "sithRenderSky.h"

#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithRenderSky_countfit_1, 12)

/* end count-fit */

// Ceiling sky globals
static float ceilingSkyElevation;
static rdVector3 ceilingSkyLow;
static rdVector3 ceilingSkyPoint;
static rdVector3 ceilingSkyNormal = { 0.0f ,  0.0f ,  -1.0f };
static rdVector4 ceilingSkyPlane = { 0.0f, 0.0f, 1.0f, -0.1f }; // unreferenced in retail (.data 0x0053C9E8)

// Horizon sky globals
static float horizonRange;
static float pixelsPerRev;
static float horizonScale;

static float lookPitchOffset;
static float lookYawOffset;

static float sinRoll;
static float cosRoll;

int J3DAPI sithRenderSky_Open(float horizonDistance, float ceilingHeight)
{
    pixelsPerRev = 0.008333334f; // (768.0f / 360.0f) / 256.0f
    horizonRange  = horizonDistance / 256.0f;

    ceilingSkyElevation = ceilingHeight;

    ceilingSkyPoint.x = 0.0f;
    ceilingSkyPoint.y = 0.0f;
    ceilingSkyPoint.z = ceilingSkyElevation;

    ceilingSkyLow.x = 0.0f;
    ceilingSkyLow.y = 0.0f;
    ceilingSkyLow.z = -ceilingSkyElevation;

    return 0;
}

void J3DAPI sithRenderSky_Close(void)
{}

void J3DAPI sithRenderSky_Update(void)
{
    float focalLength = rdCamera_pCurCamera->focalLength;
    horizonScale = horizonRange / focalLength;
    stdMath_SinCos(sithCamera_pCurCamera->lookPYR.roll, &sinRoll, &cosRoll);

    lookYawOffset   = -(sithCamera_pCurCamera->lookPYR.yaw * pixelsPerRev);
    lookPitchOffset = -(sithCamera_pCurCamera->lookPYR.pitch * pixelsPerRev);
}

void J3DAPI sithRenderSky_HorizonFaceToPlane(rdCacheProcEntry* pPoly, const rdFace* pFace, rdVector3* aVerts, size_t numVerts)
{
    LPD3DTLVERTEX pOutVert;
    size_t i;

    {
        float xFactor;
        float yFactor;

        pPoly->lightingMode = RD_LIGHTING_NONE;
        for ( i = 0; i < numVerts; ++i )
        {
            pOutVert = &pPoly->aVertices[i];
            pOutVert->sx = aVerts[i].x;
            pOutVert->sy = aVerts[i].y;
            pOutVert->sz = aVerts[i].z;

            pOutVert->rhw = horizonRange;
            pOutVert->sz = 0.9999695f; // almost at cam far plane dist

            xFactor = (pOutVert->sx - rdCamera_pCurCamera->pCanvas->center.x) * horizonScale;
            yFactor = (pOutVert->sy - rdCamera_pCurCamera->pCanvas->center.y) * horizonScale;

            pOutVert->tu = RDVECTOR_ROTATE2_X(xFactor, yFactor, sinRoll, cosRoll) + lookYawOffset;
            pOutVert->tv = RDVECTOR_ROTATE2_Y(xFactor, yFactor, sinRoll, cosRoll) + lookPitchOffset;
            pOutVert->tu += sithWorld_pCurrentWorld->horizonSkyOffset.x + pFace->texVertOffset.x;
            pOutVert->tv += sithWorld_pCurrentWorld->horizonSkyOffset.y + pFace->texVertOffset.y;
        }
    }
}

void J3DAPI sithRenderSky_CeilingFaceToPlane(rdCacheProcEntry* pPoly, const rdFace* pFace, const rdVector3* aVerts, const rdVector3* aTransformedVerts, size_t numVerts)
{
    rdVector3 dir;
    float invMatWidth;
    float invMatHeight;
    float skyDist;
    size_t i;

    if ( !pFace->pMaterial )
    {
        return;
    }

    invMatWidth = 1.0f / (float)pFace->pMaterial->width;
    invMatHeight = 1.0f / (float)pFace->pMaterial->height;
    pPoly->lightingMode = RD_LIGHTING_NONE;

    for ( i = 0; i < numVerts; ++i )
    {
        rdVector2 uv;
        LPD3DTLVERTEX pOutVert;
        rdVector3 skyVert;

        rdMatrix_TransformPoint34(&skyVert, &aVerts[i], &rdCamera_camMatrix);
        rdVector_Sub3Acc(&skyVert, &sithCamera_pCurCamera->lookPos);
        rdVector_Normalize3(&dir, &skyVert);

        if ( !sithIntersect_CheckSphereHit(&sithCamera_pCurCamera->lookPos, &dir, 1000.0f, 0.0f, &ceilingSkyNormal, &ceilingSkyPoint, &skyDist, 0) )
        {
            skyDist = 1000.0f;
        }

        rdVector_Scale3Acc(&dir, skyDist);

        rdVector_Add3Acc(&dir, &sithCamera_pCurCamera->lookPos);

        uv.x = dir.x * 16.0f * invMatWidth;
        uv.y = dir.y * 16.0f * invMatHeight;
        uv.x += sithWorld_pCurrentWorld->ceilingSkyOffset.x;
        uv.y += sithWorld_pCurrentWorld->ceilingSkyOffset.y;
        uv.x += pFace->texVertOffset.x;
        uv.y += pFace->texVertOffset.y;

        pOutVert = &pPoly->aVertices[i];
        pOutVert->tu  = uv.x;
        pOutVert->tv  = uv.y;
        pOutVert->sx  = aTransformedVerts[i].x;
        pOutVert->sy  = aTransformedVerts[i].y;
        pOutVert->sz  = 0.9999695f;
        pOutVert->rhw = aTransformedVerts[i].z / 32.0f;
    }
}
