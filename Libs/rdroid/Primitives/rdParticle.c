#include "rdParticle.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdQClip.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>

#include <stdio.h>
#include <string.h>

#define RDPARTICLE_MAJVER   1
#define RDPARTICLE_MINVER   0


rdVector2 rdParticle_aFaceTexVerts[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
rdVector3 rdParticle_aFaceVerts[4] = { 0 };
uint8_t rdParticle_aUnused_14DFC28[912] = { 0 };
rdVector3 rdParticle_aTransformedVerts[RDPARTICLE_MAXVERTS] = { 0 };
// The loader hook (debug build 0x0156F900, right after aTransformedVerts) is retail's unreferenced 4 bytes at 0x014E0BB8:
// only the discarded RegisterLoader and Load use it (name inferred).
rdParticle* (J3DAPI* rdParticle_pParticleLoader)(const char* pFilename) = NULL;

/* Retail's linker discarded this function (no retail symbol, address or caller). Its evidence is the
   /Gf literal order: rdParticle's run opens with J3D_FILE and then New's "Error allocating memory for
   particle.\n", while New alone emits them the other way round (its STDMALLOC creates J3D_FILE first;
   VC5 emits a function's new literals in reverse creation order). So an earlier function used
   J3D_FILE with a message that did not survive, and the retail __LINE__ anchors leave +67 lines ahead
   of New (100). Jedi Knight's rdParticle_RegisterLoader stands immediately before rdParticle_New; the
   debug copy asserts its argument. Name from JK; the parameter type is spelled out here because the
   loader typedef is not retail's. Body from the Indy debug build (0x00528F20, assert at line 72). */
void J3DAPI rdParticle_RegisterLoader(rdParticle* (J3DAPI* pFunc)(const char*))
{
#line 72
    RD_ASSERTREL(pFunc != NULL);
    rdParticle_pParticleLoader = pFunc;
}

rdParticle* J3DAPI rdParticle_New(size_t num, float size, rdMaterial* pMat, rdLightMode lightMode)
{
    rdParticle* pParticle;

#line 100
    pParticle = (rdParticle*)STDMALLOC(sizeof(rdParticle));
    if ( !pParticle )
    {
#line 103
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for particle.\n", 0, 0, 0, 0);
        return NULL;
    }

    memset(pParticle, 0, sizeof(rdParticle));

    if ( rdParticle_NewEntry(pParticle, num, size, pMat, lightMode) )
    {
        stdMemory_Free(pParticle);
        return NULL;
    }

    return pParticle;
}

int J3DAPI rdParticle_NewEntry(rdParticle* pParticle, size_t num, float size, rdMaterial* pMaterial, rdLightMode lightMode)
{
#line 139
    RD_ASSERTREL(num<=256);

    pParticle->numVertices   = num;
    pParticle->pMaterial     = pMaterial;
    pParticle->lightningMode = lightMode;
    pParticle->size          = size;
    pParticle->sizeHalf      = D3DVAL(size / 2.0f);
    pParticle->radius        = 0.0f;

#line 149
    pParticle->aVerticies      = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pParticle->numVertices);
    pParticle->aVertMatCelNums = (int*)STDMALLOC(sizeof(int) * pParticle->numVertices);
    pParticle->aExtraLights    = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pParticle->numVertices);

    if ( !pParticle->aVerticies || !pParticle->aVertMatCelNums || !pParticle->aExtraLights )
    {
    }
    else
    {
        memset(pParticle->aVerticies, 0, sizeof(rdVector3) * pParticle->numVertices);
        memset(pParticle->aVertMatCelNums, -1, sizeof(int) * pParticle->numVertices);
        memset(pParticle->aExtraLights, 0xFF, sizeof(rdVector4) * pParticle->numVertices);
        return 0;
    }

    rdParticle_FreeEntry(pParticle);
#line 168
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for particle.\n", 0, 0, 0, 0);
    return 1;
}

rdParticle* J3DAPI rdParticle_Duplicate(const rdParticle* pOriginal)
{
    rdParticle* pParticle;

#line 192
    RD_ASSERTREL(pOriginal);

#line 195
    pParticle = (rdParticle*)STDMALLOC(sizeof(rdParticle));
    if ( !pParticle )
    {
#line 198
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for particle.\n", 0, 0, 0, 0);
        return NULL;
    }

    memset(pParticle, 0, sizeof(rdParticle));

    if ( rdParticle_NewEntry(pParticle, pOriginal->numVertices, pOriginal->size, pOriginal->pMaterial, pOriginal->lightningMode) )
    {
        stdMemory_Free(pParticle);
        return NULL;
    }

    memcpy(pParticle->aVerticies, pOriginal->aVerticies, sizeof(rdVector3) * pOriginal->numVertices);
    memcpy(pParticle->aVertMatCelNums, pOriginal->aVertMatCelNums, sizeof(int) * pOriginal->numVertices);
    memcpy(pParticle->aExtraLights, pOriginal->aExtraLights, sizeof(rdVector4) * pOriginal->numVertices);
    return pParticle;
}

void J3DAPI rdParticle_Free(rdParticle* pParticle)
{
    if ( pParticle )
    {
        rdParticle_FreeEntry(pParticle);
        stdMemory_Free(pParticle);
        return;
    }

#line 241
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL particle ptr.\n", 0, 0, 0, 0);
}

void J3DAPI rdParticle_FreeEntry(rdParticle* pParticle)
{
    if ( pParticle->aVerticies )
    {
        stdMemory_Free(pParticle->aVerticies);
        pParticle->aVerticies = NULL;
    }

    if ( pParticle->aVertMatCelNums )
    {
        stdMemory_Free(pParticle->aVertMatCelNums);
        pParticle->aVertMatCelNums = NULL;
    }

    if ( pParticle->aExtraLights )
    {
        stdMemory_Free(pParticle->aExtraLights);
        pParticle->aExtraLights = NULL;
    }
}

/* Linker-discarded in retail (__LINE__ gap 241..371). Body and position from the Indy debug build (0x00529330, after
   FreeEntry; STDMALLOC at line 304, print at 307). */
rdParticle* J3DAPI rdParticle_Load(char* pFilename)
{
    rdParticle* pParticle;

    pParticle = NULL;
    if ( rdParticle_pParticleLoader )
    {
        return rdParticle_pParticleLoader(pFilename);
    }

#line 304
    pParticle = (rdParticle*)STDMALLOC(sizeof(rdParticle));
    if ( !pParticle )
    {
#line 307
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for particle.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( rdParticle_LoadEntry(pFilename, pParticle) )
    {
        return pParticle;
    }

    rdParticle_Free(pParticle);
    return NULL;
}

// Text of the debug build (debug 0x5293c8, /Od): the open print (487) is a block after
// the success `return 1`; the EOF (492), syntax (497), allocation (502) and range (507)
// prints follow it, the first three ending `goto error` on the shared Close and
// `return 0` after the range print. Shared error-block labels plus the debug build's `error`
// label.
int J3DAPI rdParticle_LoadEntry(const char* pFilename, rdParticle* pParticle)
{
    float x;
    float y;
    float z;
    int vermaj;
    int vermin;
    float size;
    float radius;
    size_t numVerts;
    int numCels;
    int num;
    size_t i;
    rdVector3* pVert;
    int* pCelNum;

    strncpy(pParticle->aName, stdFileFromPath(pFilename), sizeof(pParticle->aName) - 1);
    pParticle->aName[sizeof(pParticle->aName) - 1] = 0;

    if ( !stdConffile_Open(pFilename) )
    {
        goto open_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " par %d.%d", &vermaj, &vermin) != 2 )
    {
#line 371
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid PAR version %d.%d for file '%s'.\n", vermaj, vermin, pFilename, 0);
    }

    if ( vermaj != RDPARTICLE_MAJVER || vermin != RDPARTICLE_MINVER )
    {
#line 374
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid PAR version %d.%d for file '%s'.\n", vermaj, vermin, pFilename, 0);
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " size %f", &size) != 1 )
    {
        goto syntax_error;
    }

    pParticle->size = size;
    pParticle->sizeHalf = D3DVAL(pParticle->size) / 2.0f;

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " material %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    pParticle->pMaterial = rdMaterial_Load(std_genBuffer);
    if ( !pParticle->pMaterial )
    {
        goto syntax_error;
    }

    numCels = pParticle->pMaterial->numCels;
    if ( !pParticle->pMaterial )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " lightingmode %d", &pParticle->lightningMode) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " radius %f", &radius) != 1 )
    {
        goto syntax_error;
    }

    pParticle->radius = radius;
    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " insert offset %f %f %f", &x, &y, &z) != 3 )
    {
        goto syntax_error;
    }

    pParticle->insertOffset.x = x;
    pParticle->insertOffset.y = y;
    pParticle->insertOffset.z = z;

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " vertices %d", &numVerts) != 1 )
    {
        goto syntax_error;
    }

    if ( numVerts > RDPARTICLE_MAXVERTS )
    {
        goto range_error;
    }

    pParticle->numVertices = numVerts;
#line 450
    pParticle->aVerticies      = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
    pParticle->aVertMatCelNums = (int*)STDMALLOC(sizeof(int) * numVerts);
    pParticle->aExtraLights    = (rdVector4*)STDMALLOC(sizeof(rdVector4) * numVerts);

    if ( !pParticle->aVerticies || !pParticle->aVertMatCelNums || !pParticle->aExtraLights )
    {
        goto alloc_error;
    }

    pVert   = pParticle->aVerticies;
    pCelNum = pParticle->aVertMatCelNums;

    for ( i = 0; i < numVerts; ++i )
    {
        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, " %d: %f %f %f %d", &num, &x, &y, &z, pCelNum) != 5 )
        {
            goto syntax_error;
        }

        if ( *pCelNum >= numCels )
        {
            goto syntax_error;
        }

        pVert->x = x;
        pVert->y = y;
        pVert->z = z;
        ++pVert;
        ++pCelNum;
    }

    stdConffile_Close();
    return 1;

open_error:
#line 487
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid load filename '%s'.\n", pFilename, 0, 0, 0);
    return 0;

eof_error:
#line 492
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", pFilename, 0, 0, 0);
    goto error;

syntax_error:
#line 497
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);
    goto error;

alloc_error:
#line 502
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);
    goto error;

range_error:
#line 507
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Value out of range for '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);

error:
    stdConffile_Close();
    return 0;
}

/* Linker-discarded (no caller in either build): the Indy debug build has the PAR text writer here (0x005298F6), after
   LoadEntry; Jedi Knight's rdParticle.c has the same function in this slot. Body from the debug code. */
int J3DAPI rdParticle_Write(const char* pFilename, const rdParticle* pParticle, const char* pCreatedFrom)
{
    tFileHandle fh;
    size_t i;

    fh = rdroid_pHS->pFileOpen(pFilename, "wt+");
    if ( !fh )
    {
        return 0;
    }

    rdroid_pHS->pFilePrintf(fh, "# PAR '%s' created from '%s'\n\n", pParticle->aName, pCreatedFrom);
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: HEADER\n\n");
    rdroid_pHS->pFilePrintf(fh, "PAR %d.%d\n\n", 1, 0);
    rdroid_pHS->pFilePrintf(fh, "SIZE %.6f\n\n", pParticle->size);
    rdroid_pHS->pFilePrintf(fh, "MATERIAL %s\n\n", pParticle->pMaterial->aName);
    rdroid_pHS->pFilePrintf(fh, "LIGHTINGMODE %d\n\n", pParticle->lightningMode);
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: GEOMETRYDEF\n\n");
    rdroid_pHS->pFilePrintf(fh, "# Object radius\n");
    rdroid_pHS->pFilePrintf(fh, "RADIUS %10.6f\n\n", pParticle->radius);
    rdroid_pHS->pFilePrintf(fh, "# Insertion offset\n");
    rdroid_pHS->pFilePrintf(fh, "INSERT OFFSET %10.6f %10.6f %10.6f\n\n", pParticle->insertOffset.x, pParticle->insertOffset.y, pParticle->insertOffset.z);
    rdroid_pHS->pFilePrintf(fh, "VERTICES %d\n\n", pParticle->numVertices);
    rdroid_pHS->pFilePrintf(fh, "# num:     x:         y:         z:       cel:\n");

    for ( i = 0; i < pParticle->numVertices; ++i )
    {
        rdroid_pHS->pFilePrintf(fh, "  %3d: %10.6f %10.6f %10.6f %d\n", i, pParticle->aVerticies[i].x, pParticle->aVerticies[i].y, pParticle->aVerticies[i].z, pParticle->aVertMatCelNums[i]);
    }

    rdroid_pHS->pFilePrintf(fh, "\n\n");
    rdroid_pHS->pFileClose(fh);
    return 1;
}

int J3DAPI rdParticle_Draw(const rdThing* pThing, const rdMatrix34* pOrient)
{
    rdParticle* pParticle;
    int bAlpha;
    RdFrustumCull frustumCull;
    rdLightMode lightMode;
    rdVector4 ambientLight;
    rdMatrix34 tmat;
    size_t i;
    int numInFrustum;
    int bAlphaPoly;
    rdCacheProcEntry* pPoly;

    pParticle = pThing->data.pParticle;
    frustumCull = pThing->frustumCull;
    if ( pParticle->pMaterial )
    {
        bAlpha = pParticle->pMaterial->formatType == STDCOLOR_FORMAT_RGBA;
    }
    else
    {
        bAlpha = 0;
    }

    if ( frustumCull != RDFRUSTUMCULL_OUTSIDE )
    {
        rdMatrix_Multiply34(&tmat, &rdCamera_pCurCamera->orient, pOrient);

        if ( (rdroid_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
        {
            rdVector_Copy3(&ambientLight, &rdCamera_pCurCamera->ambientLight);
            ambientLight.alpha = 1.0f;
        }
        else
        {
            ambientLight.red   = 0.0f;
            ambientLight.green = 0.0f;
            ambientLight.blue  = 0.0f;
            ambientLight.alpha = 1.0f;
        }

        lightMode = pParticle->lightningMode;
        rdMatrix_TransformPointList34(&tmat, pParticle->aVerticies, rdParticle_aTransformedVerts, pParticle->numVertices);

        for ( i = 0; i < pParticle->numVertices; ++i )
        {
            rdParticle_aFaceVerts[0].x = rdParticle_aTransformedVerts[i].x - pParticle->sizeHalf;
            rdParticle_aFaceVerts[0].y = rdParticle_aTransformedVerts[i].y;
            rdParticle_aFaceVerts[0].z = rdParticle_aTransformedVerts[i].z - pParticle->sizeHalf;

            rdParticle_aFaceVerts[1].x = rdParticle_aTransformedVerts[i].x + pParticle->sizeHalf;
            rdParticle_aFaceVerts[1].y = rdParticle_aTransformedVerts[i].y;
            rdParticle_aFaceVerts[1].z = rdParticle_aTransformedVerts[i].z - pParticle->sizeHalf;

            rdParticle_aFaceVerts[2].x = rdParticle_aTransformedVerts[i].x + pParticle->sizeHalf;
            rdParticle_aFaceVerts[2].y = rdParticle_aTransformedVerts[i].y;
            rdParticle_aFaceVerts[2].z = rdParticle_aTransformedVerts[i].z + pParticle->sizeHalf;

            rdParticle_aFaceVerts[3].x = rdParticle_aTransformedVerts[i].x - pParticle->sizeHalf;
            rdParticle_aFaceVerts[3].y = rdParticle_aTransformedVerts[i].y;
            rdParticle_aFaceVerts[3].z = rdParticle_aTransformedVerts[i].z + pParticle->sizeHalf;

            numInFrustum = rdQClip_VerticesInFrustrum(rdCamera_pCurCamera->pFrustum, rdParticle_aFaceVerts, 4);
            if ( numInFrustum == 4 )
            {
                bAlphaPoly = rdParticle_aFaceVerts[0].y < 1.5f;
                bAlphaPoly &= bAlpha;
                if ( bAlphaPoly )
                {
                    pPoly = rdCache_GetAlphaProcEntry();
                }
                else
                {
                    pPoly = rdCache_GetProcEntry();
                }

                if ( !pPoly )
                {
                    return 0;
                }

                rdClip_VerticesToPlane(pPoly, rdParticle_aFaceVerts, rdParticle_aFaceTexVerts, 4);

                rdVector_Copy4(&pPoly->aVertIntensities[0], &ambientLight);
                rdVector_Copy4(&pPoly->aVertIntensities[1], &ambientLight);
                rdVector_Copy4(&pPoly->aVertIntensities[2], &ambientLight);
                rdVector_Copy4(&pPoly->aVertIntensities[3], &ambientLight);

                pPoly->extraLight.red   = 0.0f;
                pPoly->extraLight.green = 0.0f;
                pPoly->extraLight.blue  = 0.0f;
                pPoly->extraLight.alpha = 1.0f;

                pPoly->lightingMode = lightMode;
                pPoly->pMaterial    = pParticle->pMaterial;
                pPoly->matCelNum    = pParticle->aVertMatCelNums[i];
                pPoly->extraLight.red   = pParticle->aExtraLights[i].red;
                pPoly->extraLight.green = pParticle->aExtraLights[i].green;
                pPoly->extraLight.blue  = pParticle->aExtraLights[i].blue;
                pPoly->extraLight.alpha = pParticle->aExtraLights[i].alpha;

                if ( bAlphaPoly )
                {
                    pPoly->flags = RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
                    rdCache_AddAlphaProcFace(4);
                }
                else
                {
                    pPoly->flags = RD_FF_FOG_ENABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
                    rdCache_AddProcFace(4);
                }
            }

        }

        return 1;
    }

    return 0;
}
