#include "rdPolyline.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>

#include <string.h>

rdVector3 rdPolyline_aVerticesCache[4] = { 0 };

rdPolyline* J3DAPI rdPolyline_New(const char* pName, const char* pMatFilename, const char* pMatFilename2, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor)
{
    rdPolyline* pLine;

#line 88
    pLine = (rdPolyline*)STDMALLOC(sizeof(rdPolyline));
    if ( !pLine )
    {
#line 91
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for polyline.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdPolyline_NewEntry(pLine, pName, pMatFilename, length, baseRadius, tipRadius, geoMode, lightMode, pColor);
    return pLine;
}

int J3DAPI rdPolyline_NewEntry(rdPolyline* pLine, const char* pName, const char* pMatFilename, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor)
{
    rdFace* pFace;
    size_t i;

    if ( pName )
    {
        strncpy(pLine->aName, pName, sizeof(pLine->aName) - 1);
        pLine->aName[sizeof(pLine->aName) - 1] = '\0';
    }

    pLine->length     = length;
    pLine->baseRadius = baseRadius;
    pLine->tipRadius  = tipRadius;

    pFace = &pLine->face;
    pFace->flags        = RD_FF_FOG_ENABLED | RD_FF_TEX_TRANSLUCENT | RD_FF_DOUBLE_SIDED;
    pFace->geometryMode = geoMode;
    pFace->lightingMode = lightMode;
    pLine->geoMode      = geoMode;
    pLine->lightMode    = lightMode;

    rdVector_Copy4(&pFace->extraLight, pColor);

    pFace->pMaterial = rdMaterial_Load(pMatFilename);
    if ( !pFace->pMaterial )
    {
    }
    else
    {
        pFace->numVertices = 4;
#line 168
        pFace->aVertices = (size_t*)STDMALLOC(sizeof(*pFace->aVertices) * pFace->numVertices);
        if ( !pFace->aVertices )
        {
            goto alloc_error;
        }

        for ( i = 0; i < pFace->numVertices; ++i )
        {
            pFace->aVertices[i] = i;
        }

        if ( pFace->geometryMode >= RD_GEOMETRY_FULL )
        {
#line 180
            pFace->aTexVertices = (size_t*)STDMALLOC(sizeof(*pFace->aTexVertices) * pFace->numVertices);
            if ( !pFace->aTexVertices )
            {
                goto alloc_error;
            }

            for ( i = 0; i < pFace->numVertices; ++i )
            {
                pFace->aTexVertices[i] = i;
            }

#line 188
            pLine->apUVs = (rdVector2*)STDMALLOC(sizeof(*pLine->apUVs) * pFace->numVertices);
            if ( !pLine->apUVs )
            {
                goto alloc_error;
            }

            pLine->apUVs[0].x = 1.0f;
            pLine->apUVs[0].y = 0.0f;

            pLine->apUVs[1].x = 1.0f;
            pLine->apUVs[1].y = 1.0f;

            pLine->apUVs[2].x = 0.0f;
            pLine->apUVs[2].y = 1.0f;

            pLine->apUVs[3].x = 0.0f;
            pLine->apUVs[3].y = 0.0f;
        }

        pFace->texVertOffset.y = 0.0f;
        pFace->texVertOffset.x = 0.0f;
        return 1;

        // One print block entered from the three allocation failures, laid after the success return.
alloc_error:
#line 211
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for polyline '%'.\n", pName, 0, 0, 0);
    }

    return 0;
}

void J3DAPI rdPolyline_Free(rdPolyline* pPolyline)
{
    if ( pPolyline )
    {
        rdPolyline_FreeEntry(pPolyline);
        stdMemory_Free(pPolyline);
        return;
    }

#line 242
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL polyline ptr.\n", 0, 0, 0, 0);
}

void J3DAPI rdPolyline_FreeEntry(rdPolyline* pPolyline)
{
    rdFace* pFace;

    pFace = &pPolyline->face;
    if ( pPolyline->apUVs )
    {
        stdMemory_Free(pPolyline->apUVs);
        pPolyline->apUVs = NULL;
    }

    if ( pFace->aVertices )
    {
        stdMemory_Free(pFace->aVertices);
        pFace->aVertices = NULL;
    }

    if ( pFace->aTexVertices )
    {
        stdMemory_Free(pFace->aTexVertices);
        pFace->aTexVertices = NULL;
    }
}

int J3DAPI rdPolyline_Draw(const rdThing* pLine, const rdMatrix34* pOrient)
{
    rdPolyline* pPolyline;
    rdMatrix34 mat;
    rdVector3 vecFwd;
    rdVector3 vecEnd;
    float deX;
    float deZ;
    rdVector2 delta;
    float angle;
    float sinv;
    float cosv;
    float xzLen;
    rdVector2 negTip;
    rdVector2 tip;
    rdVector2 negBase;
    rdVector2 base;

    pPolyline = pLine->data.pPolyline;
    rdMatrix_Multiply34(&mat, &rdCamera_pCurCamera->orient, pOrient);

    vecFwd.x = 0.0f;
    vecFwd.y = pPolyline->length;
    vecFwd.z = 0.0f;
    rdMatrix_TransformPoint34(&vecEnd, &vecFwd, &mat);

    deX = vecEnd.x - mat.dvec.x;
    deZ = vecEnd.z - mat.dvec.z;
    delta.x = vecEnd.x - mat.dvec.x;
    delta.y = vecEnd.z - mat.dvec.z;
    xzLen = sqrtf(D3DMultiply(delta.x, delta.x) + D3DMultiply(delta.y, delta.y));

    angle = stdMath_ArcSin3((-deX) / xzLen);
    if ( deZ < 0.0f )
    {
        if ( deX > 0.0f )
        {
            angle = 180.0f - angle;
        }
        else
        {
            angle = -(180.0f + angle);
        }
    }

    stdMath_SinCos(angle, &sinv, &cosv);

    negTip.x = -pPolyline->tipRadius;
    negTip.y = xzLen;
    tip.x = pPolyline->tipRadius;
    tip.y = xzLen;
    negBase.x = -pPolyline->baseRadius;
    negBase.y = 0.0f;
    base.x = pPolyline->baseRadius;
    base.y = 0.0f;

    rdPolyline_aVerticesCache[0].x = RDVECTOR_ROTATE2_X(tip.x, tip.y, sinv, cosv) + mat.dvec.x;
    rdPolyline_aVerticesCache[0].y = vecEnd.y;
    rdPolyline_aVerticesCache[0].z = RDVECTOR_ROTATE2_Y(tip.x, tip.y, sinv, cosv) + mat.dvec.z;

    rdPolyline_aVerticesCache[1].x = RDVECTOR_ROTATE2_X(negTip.x, negTip.y, sinv, cosv) + mat.dvec.x;
    rdPolyline_aVerticesCache[1].y = vecEnd.y;
    rdPolyline_aVerticesCache[1].z = RDVECTOR_ROTATE2_Y(negTip.x, negTip.y, sinv, cosv) + mat.dvec.z;

    rdPolyline_aVerticesCache[2].x = RDVECTOR_ROTATE2_X(negBase.x, negBase.y, sinv, cosv) + mat.dvec.x;
    rdPolyline_aVerticesCache[2].y = mat.dvec.y;
    rdPolyline_aVerticesCache[2].z = RDVECTOR_ROTATE2_Y(negBase.x, negBase.y, sinv, cosv) + mat.dvec.z;

    rdPolyline_aVerticesCache[3].x = RDVECTOR_ROTATE2_X(base.x, base.y, sinv, cosv) + mat.dvec.x;
    rdPolyline_aVerticesCache[3].y = mat.dvec.y;
    rdPolyline_aVerticesCache[3].z = RDVECTOR_ROTATE2_Y(base.x, base.y, sinv, cosv) + mat.dvec.z;

    rdPolyline_DrawFace(pLine, &pPolyline->face, rdPolyline_aVerticesCache, pPolyline->apUVs);
    return 1;
}

void J3DAPI rdPolyline_DrawFace(const rdThing* pLine, const rdFace* pFace, const rdVector3* aVertices, const rdVector2* aUVs)
{
    rdCacheProcEntry* pPoly;
    size_t numVertices;
    size_t i;

    pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return;
    }

    numVertices = pFace->numVertices;
    if ( rdClip_FaceToPlane(rdCamera_pCurCamera->pFrustum, pPoly, pFace, aVertices, aUVs, NULL, NULL) )
    {
        for ( i = 0; i < numVertices; ++i )
        {
            rdVector_Copy4(&pPoly->aVertIntensities[i], &pFace->extraLight);
        }

        if ( (rdroid_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
        {
            pPoly->extraLight.red   += rdCamera_pCurCamera->ambientLight.red;
            pPoly->extraLight.green += rdCamera_pCurCamera->ambientLight.green;
            pPoly->extraLight.blue  += rdCamera_pCurCamera->ambientLight.blue;
            pPoly->extraLight.alpha += rdCamera_pCurCamera->ambientLight.alpha;
        }

        pPoly->matCelNum    = pFace->matCelNum;
        pPoly->lightingMode = RD_LIGHTING_NONE;
        pPoly->flags        = pFace->flags;
        pPoly->flags       |= RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_TRANSLUCENT;
        pPoly->pMaterial    = pFace->pMaterial;
        rdCache_AddAlphaProcFace(numVertices);
    }
}
