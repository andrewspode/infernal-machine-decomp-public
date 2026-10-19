/* Retail calls rdVector_Add3Acc and rdVector_InvScale3 out of line in GetCenterPoint. */
#define RDVECTOR_NO_INLINE
#include <rdroid/Math/rdVector.h>
#undef RDVECTOR_NO_INLINE

#include "sithSurface.h"
#include "sithWorld.h"

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithIntersect.h>

#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/Raster/rdFace.h>

#include <sound/Sound.h>
#include <sound/types.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/types.h>
#include <std/Win95/stdComm.h>

#include <j3dcore/types.h>
#include <stdio.h>
#include <stdlib.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithSurface_countfit_1, 15)
CF_PROTO(sithSurface_countfit_2, 15)
CF_PROTO(sithSurface_countfit_3, 15)
CF_PROTO(sithSurface_countfit_4, 15)
CF_PROTO(sithSurface_countfit_5, 15)
CF_PROTO(sithSurface_countfit_6, 15)
CF_PROTO(sithSurface_countfit_7, 15)
CF_PROTO(sithSurface_countfit_8, 10)

/* end count-fit */

static SithSurface* sithSurface_apUnsyncedSurfaces[32];
// External: sithMulti_OpenGame clears it (debug build 0x004694D0); `= 0` keeps it in this block at the same offset.
size_t sithSurface_numUnsyncedSurfaces = 0;

// Unreferenced original storage at 0x0056FAA0 (the 4 bytes at 0x0056FA9C are VC5's 8-byte alignment
// padding for it): no surviving code reaches it. The zero-initialised group follows declaration
// order, so this position gives the same layout as any other.
static uint8_t sithSurface_aUnused_56FAA0[64] = { 0 };

void J3DAPI sithSurface_HideSectorAdjoin(SithSurfaceAdjoin* pAdjoin)
{
#line 106
    SITH_ASSERTREL(pAdjoin);
    if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
    {
        pAdjoin->flags |= SITH_ADJOIN_SECTORSET;
        pAdjoin->flags &= ~SITH_ADJOIN_VISIBLE;
    }
}

void J3DAPI sithSurface_ShowSectorAdjoin(SithSurfaceAdjoin* pAdjoin)
{
#line 136
    SITH_ASSERTREL(pAdjoin);
    if ( (pAdjoin->flags & SITH_ADJOIN_SECTORSET) != 0 )
    {
        pAdjoin->flags |= SITH_ADJOIN_VISIBLE;
        pAdjoin->flags &= ~SITH_ADJOIN_SECTORSET;
    }
}

int J3DAPI sithSurface_AllocWorldSurfaces(SithWorld* pWorld, size_t numSurfaces)
{
    size_t i;
    SithSurface* pCurf;

#line 169
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aSurfaces == NULL);

    pWorld->aSurfaces = (SithSurface*)STDMALLOC(sizeof(SithSurface) * numSurfaces);
    if ( !pWorld->aSurfaces )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate mem for %d surfaces.\n", numSurfaces, 0, 0, 0);
        return 1;
    }

    memset(pWorld->aSurfaces, 0, sizeof(SithSurface) * numSurfaces);
    pWorld->numSurfaces = numSurfaces;

    pCurf = pWorld->aSurfaces;
    for ( i = 0; i < numSurfaces; ++i )
    {
        rdFace_NewEntry(&pCurf->face);
        ++pCurf;
    }

    return 0;
}
static int J3DAPI sithSurface_AllocWorldAdjoins(SithWorld* pWorld, size_t numAdjoins)
{
#line 211
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(!pWorld->aAdjoins);

    if ( !numAdjoins )
    {
        pWorld->aAdjoins = NULL;
        return 0;
    }

#line 221
    pWorld->aAdjoins = (SithSurfaceAdjoin*)STDMALLOC(sizeof(SithSurfaceAdjoin) * numAdjoins);
    if ( !pWorld->aAdjoins )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate mem for %d adjoins.\n", numAdjoins, 0, 0, 0);
        return 1;
    }

    memset(pWorld->aAdjoins, 0, sizeof(SithSurfaceAdjoin) * numAdjoins);
    pWorld->numAdjoins = numAdjoins;
    return 0;
}

void J3DAPI sithSurface_FreeWorldSurfaces(SithWorld* pWorld)
{
    size_t i;
    SithSurface* pSurf;

#line 251
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL((pWorld->aSurfaces != NULL));

    pSurf = pWorld->aSurfaces;
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pSurf->face.numVertices = 0;
        if ( pSurf->face.aVertices )
        {
            stdMemory_Free(pSurf->face.aVertices);
        }

        if ( pSurf->face.aTexVertices )
        {
            stdMemory_Free(pSurf->face.aTexVertices);
        }

        if ( pSurf->aIntensities )
        {
            stdMemory_Free(pSurf->aIntensities);
        }

        pSurf->flags = 0;
        ++pSurf;
    }

    stdMemory_Free(pWorld->aSurfaces);

    pWorld->aSurfaces   = NULL;
    pWorld->numSurfaces = 0;
    sithSurface_numUnsyncedSurfaces = 0;

    if ( pWorld->aAdjoins )
    {
        stdMemory_Free(pWorld->aAdjoins);
    }

    pWorld->aAdjoins   = NULL;
    pWorld->numAdjoins = 0;
}

int J3DAPI sithSurface_WriteAdjoinsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    size_t written;
    size_t cursizeAdjoins;
    CndSurfaceAdjoin* aCndAdjoins;
    SithSurfaceAdjoin* pAdjoin;
    CndSurfaceAdjoin* pCndAdjoin;

    cursizeAdjoins = sizeof(CndSurfaceAdjoin) * pWorld->numAdjoins;
#line 303
    aCndAdjoins = (CndSurfaceAdjoin*)STDMALLOC(cursizeAdjoins);
    if ( !aCndAdjoins )
    {
        return 1;
    }

    for ( i = 0; i < pWorld->numAdjoins; ++i )
    {
        pAdjoin    = &pWorld->aAdjoins[i];
        pCndAdjoin = &aCndAdjoins[i];

        pCndAdjoin->flags     = pAdjoin->flags;
        pCndAdjoin->mirrorIdx = pAdjoin->pMirrorAdjoin ? pAdjoin->pMirrorAdjoin - pWorld->aAdjoins : -1;
        pCndAdjoin->distance  = pAdjoin->distance;
    }

    written = sith_pHS->pFileWrite(fh, aCndAdjoins, cursizeAdjoins);
    stdMemory_Free(aCndAdjoins);
    if ( written != cursizeAdjoins )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithSurface_ReadAdjoinsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    size_t nRead;
    size_t cursizeAdjoins;
    CndSurfaceAdjoin* aCndAdjoins;
    SithSurfaceAdjoin* pAdjoin;
    CndSurfaceAdjoin* pCndAdjoin;

    cursizeAdjoins = sizeof(CndSurfaceAdjoin) * pWorld->numAdjoins;
#line 332
    aCndAdjoins = (CndSurfaceAdjoin*)STDMALLOC(cursizeAdjoins);
    if ( !aCndAdjoins )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aCndAdjoins, cursizeAdjoins);
    if ( nRead != cursizeAdjoins )
    {
        goto error;
    }

    if ( sithSurface_AllocWorldAdjoins(pWorld, pWorld->numAdjoins) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numAdjoins; ++i )
    {
        pAdjoin    = &pWorld->aAdjoins[i];
        pCndAdjoin = &aCndAdjoins[i];

        pAdjoin->flags = pCndAdjoin->flags;
        if ( pCndAdjoin->mirrorIdx == -1 )
        {
            pAdjoin->pMirrorAdjoin = NULL;
        }
        else
        {
            pAdjoin->pMirrorAdjoin = &pWorld->aAdjoins[pCndAdjoin->mirrorIdx];
        }

        pAdjoin->distance = pCndAdjoin->distance;
    }

    stdMemory_Free(aCndAdjoins);
    return 0;

error:
    if ( aCndAdjoins )
    {
        stdMemory_Free(aCndAdjoins);
    }

    return 1;
}

int J3DAPI sithSurface_WriteAdjoinsListText(const SithWorld* pWorld)
{
    size_t i;
    SithSurfaceAdjoin* pAdjoin;

    if ( stdConffile_WriteLine("#------ Adjoins Subsection -----\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World adjoins %d\n", pWorld->numAdjoins) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#num:\tflags:\tmirror:\tdist:\n") )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numAdjoins; ++i )
    {
        pAdjoin = &pWorld->aAdjoins[i];
        if ( stdConffile_Printf("%d:\t0x%x\t%d\t%.2f\n", i, pAdjoin->flags, pAdjoin->pMirrorAdjoin ? pAdjoin->pMirrorAdjoin - pWorld->aAdjoins : -1, pAdjoin->distance) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithSurface_WriteSurfacesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    size_t j;
    size_t totalVerts;
    size_t nWritten;
    size_t size;
    int matIdx;
    int numMaterials;
    SithSurface* pSurf;
    CndSurfaceInfo* aCndSurfaces;
    CndSurfaceInfo* pCndSurf;
    CndSurfaceVertInfo* aCndSurfVerts;
    CndSurfaceVertInfo* pCndSurfVert;

    size = sizeof(CndSurfaceInfo) * pWorld->numSurfaces;
#line 401
    aCndSurfaces = (CndSurfaceInfo*)STDMALLOC(size);
    if ( !aCndSurfaces )
    {
        return 1;
    }

    totalVerts = 0;
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pSurf    = &pWorld->aSurfaces[i];
        pCndSurf = &aCndSurfaces[i];

        matIdx = pSurf->face.pMaterial ? pSurf->face.pMaterial->num : -1;
        if ( SITHWORLD_IS_STATICINDEX(matIdx) )
        {
            numMaterials = sithWorld_pStaticWorld->numMaterials;
            numMaterials = SITHWORLD_STATICINDEX(numMaterials);
        }
        else
        {
            numMaterials = pWorld->numMaterials;
        }

        if ( matIdx > numMaterials )
        {
            matIdx = -1;
#line 421
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Surface Material %s not in world.\n", pSurf->face.pMaterial->aName, 0, 0, 0);
        }

        if ( matIdx == -1 )
        {
            pSurf->face.geometryMode = pSurf->face.geometryMode > RD_GEOMETRY_SOLID ? RD_GEOMETRY_SOLID : pSurf->face.geometryMode;
        }

        pCndSurf->matNum    = matIdx;
        pCndSurf->surfflags = pSurf->flags;
        pCndSurf->faceflags = pSurf->face.flags;
        pCndSurf->geoMode   = pSurf->face.geometryMode;
        pCndSurf->lightMode = pSurf->face.lightingMode;

        pCndSurf->adjoinNum = pSurf->pAdjoin ? pSurf->pAdjoin - pWorld->aAdjoins : -1;

        rdVector_Copy4(&pCndSurf->color, &pSurf->face.extraLight);
        rdVector_Copy3(&pCndSurf->normal, &pSurf->face.normal);
        pCndSurf->numVerts = pSurf->face.numVertices;

        totalVerts += pSurf->face.numVertices;
    }

    nWritten = sith_pHS->pFileWrite(fh, aCndSurfaces, size);
    stdMemory_Free(aCndSurfaces);
    if ( nWritten != size )
    {
        return 1;
    }

    nWritten = sith_pHS->pFileWrite(fh, &totalVerts, sizeof(totalVerts));
    if ( nWritten != sizeof(totalVerts) )
    {
        return 1;
    }

    size = sizeof(CndSurfaceVertInfo) * totalVerts;
#line 447
    aCndSurfVerts = (CndSurfaceVertInfo*)STDMALLOC(size);
    if ( !aCndSurfVerts )
    {
        return 1;
    }

    pCndSurfVert = aCndSurfVerts;
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pSurf = &pWorld->aSurfaces[i];
        for ( j = 0; j < pSurf->face.numVertices; ++j )
        {
            pCndSurfVert->vertIdx = pSurf->face.aVertices[j];
            if ( pSurf->face.aTexVertices )
            {
                pCndSurfVert->texIdx = pSurf->face.aTexVertices[j];
            }
            else
            {
                pCndSurfVert->texIdx = 0;
            }

            rdVector_Copy4(&pCndSurfVert->color, &pSurf->aIntensities[j]);
            ++pCndSurfVert;
        }
    }

    nWritten = sith_pHS->pFileWrite(fh, aCndSurfVerts, size);
    stdMemory_Free(aCndSurfVerts);
    if ( nWritten != size )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithSurface_ReadSurfacesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    size_t j;
    size_t size;
    size_t nRead;
    size_t totalVerts;
    SithSurface* pSurf;
    CndSurfaceInfo* aCndSurfaces = NULL;
    CndSurfaceInfo* pCndSurf;
    CndSurfaceVertInfo* aCndSurfVerts;
    const CndSurfaceVertInfo* pCndSurfVert;

    size = sizeof(CndSurfaceInfo) * pWorld->numSurfaces;
#line 490
    aCndSurfaces = (CndSurfaceInfo*)STDMALLOC(size);
    if ( !aCndSurfaces )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aCndSurfaces, size);
    if ( nRead != size )
    {
        goto error;
    }

    if ( sithSurface_AllocWorldSurfaces(pWorld, pWorld->numSurfaces) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pSurf    = &pWorld->aSurfaces[i];
        pCndSurf = &aCndSurfaces[i];

        if ( pCndSurf->matNum != -1 )
        {
            if ( SITHWORLD_IS_STATICINDEX(pCndSurf->matNum) )
            {
                pCndSurf->matNum = SITHWORLD_FROM_STATICINDEX(pCndSurf->matNum);
                pSurf->face.pMaterial = &sithWorld_pStaticWorld->aMaterials[pCndSurf->matNum];
            }
            else
            {
                pSurf->face.pMaterial = pWorld->apMatArray[pCndSurf->matNum];
            }
        }
        else
        {
            pSurf->face.pMaterial = NULL;
        }

        pSurf->flags             = pCndSurf->surfflags;
        pSurf->face.flags        = pCndSurf->faceflags;
        pSurf->face.geometryMode = pCndSurf->geoMode;
        pSurf->face.lightingMode = pCndSurf->lightMode;

        if ( pCndSurf->adjoinNum != -1 )
        {
            pSurf->pAdjoin = &pWorld->aAdjoins[pCndSurf->adjoinNum];
            pSurf->pAdjoin->pAdjoinSurface = pSurf;
        }
        else
        {
            pSurf->pAdjoin = NULL;
        }

        rdVector_Copy4(&pSurf->face.extraLight, &pCndSurf->color);
        rdVector_Copy3(&pSurf->face.normal, &pCndSurf->normal);
        pSurf->face.numVertices = pCndSurf->numVerts;
    }

    if ( aCndSurfaces )
    {
        stdMemory_Free(aCndSurfaces);
    }

    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }

    pWorld->apMatArray = NULL;

    nRead = sith_pHS->pFileRead(fh, &totalVerts, sizeof(totalVerts));
    if ( nRead != sizeof(totalVerts) )
    {
        return 1;
    }

    size = sizeof(CndSurfaceVertInfo) * totalVerts;
#line 541
    aCndSurfVerts = (CndSurfaceVertInfo*)STDMALLOC(size);
    if ( !aCndSurfVerts )
    {
        return 1;
    }

    nRead = sith_pHS->pFileRead(fh, aCndSurfVerts, size);
    if ( nRead != size )
    {
        goto error;
    }

    pCndSurfVert = aCndSurfVerts;
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pSurf = &pWorld->aSurfaces[i];

#line 553
        pSurf->face.aVertices = (size_t*)STDMALLOC(sizeof(int) * pSurf->face.numVertices);
        if ( !pSurf->face.aVertices )
        {
            goto error;
        }

#line 556
        pSurf->face.aTexVertices = (size_t*)STDMALLOC(sizeof(int) * pSurf->face.numVertices);
        if ( !pSurf->face.aTexVertices )
        {
            goto error;
        }

#line 560
        pSurf->aIntensities = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pSurf->face.numVertices);
        if ( !pSurf->aIntensities )
        {
            goto error;
        }

        for ( j = 0; j < pSurf->face.numVertices; ++j )
        {
            pSurf->face.aVertices[j]    = pCndSurfVert->vertIdx;
            pSurf->face.aTexVertices[j] = pCndSurfVert->texIdx;
            rdVector_Copy4(&pSurf->aIntensities[j], &pCndSurfVert->color);
            ++pCndSurfVert;
        }
    }

    stdMemory_Free(aCndSurfVerts);
    return 0;

error:
    if ( aCndSurfaces )
    {
        stdMemory_Free(aCndSurfaces);
    }

    if ( aCndSurfVerts )
    {
        stdMemory_Free(aCndSurfVerts);
    }

    return 1;
}

int J3DAPI sithSurface_WriteSurfacesListText(const SithWorld* pWorld)
{
    int matIdx;
    int numMaterials;
    size_t i;
    SithSurface* pSurf;
    size_t j;

    if ( stdConffile_WriteLine("#----- Surfaces Subsection -----\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World surfaces %d\n", pWorld->numSurfaces) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#num:\tmat:\tsurfflags:\tfaceflags:\tgeo:\tlight:\ttex:\tadjoin:\tExLR:\tExLG:\tExLB:\tnverts:\tvertices:\t\t\tiR:\tiG\tiB\n") )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pSurf  = &pWorld->aSurfaces[i];
        matIdx = pSurf->face.pMaterial ? pSurf->face.pMaterial->num : -1;
        if ( SITHWORLD_IS_STATICINDEX(matIdx) )
        {
            numMaterials = sithWorld_pStaticWorld->numMaterials;
            numMaterials = SITHWORLD_STATICINDEX(numMaterials);
        }
        else
        {
            numMaterials = pWorld->numMaterials;
        }

        if ( matIdx > numMaterials )
        {
#line 605
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Surface Material %s not in world.\n", pSurf->face.pMaterial->aName, 0, 0, 0);
            pSurf->face.pMaterial = NULL;
            matIdx = -1;
        }

        if ( matIdx == -1 )
        {
            pSurf->face.geometryMode = pSurf->face.geometryMode > RD_GEOMETRY_SOLID ? RD_GEOMETRY_SOLID : pSurf->face.geometryMode;
        }

        if ( stdConffile_Printf("%d:\t%d\t0x%x\t\t0x%x\t\t%d\t%d\t%d\t%d\t%.2f\t%.2f\t%.2f\t%.2f\t\t%d\t", i, matIdx, pSurf->flags, pSurf->face.flags, pSurf->face.geometryMode, pSurf->face.lightingMode, 3, pSurf->pAdjoin ? pSurf->pAdjoin - pWorld->aAdjoins : -1, pSurf->face.extraLight.red, pSurf->face.extraLight.green, pSurf->face.extraLight.blue, pSurf->face.extraLight.alpha, pSurf->face.numVertices) )
        {
            goto error;
        }

        for ( j = 0; j < pSurf->face.numVertices; ++j )
        {
            if ( pSurf->face.geometryMode >= RD_GEOMETRY_FULL )
            {
#line 629
                SITH_ASSERTREL(pSurf->face.pMaterial != NULL);
                SITH_ASSERTREL(pSurf->face.aTexVertices != NULL);
                if ( stdConffile_Printf("%d,%d\t", pSurf->face.aVertices[j], pSurf->face.aTexVertices[j]) )
                {
                    goto error;
                }
            }
            else
            {
                if ( stdConffile_Printf("%d,-1\t", pSurf->face.aVertices[j]) )
                {
                    goto error;
                }
            }
        }

        for ( j = 0; j < pSurf->face.numVertices; ++j )
        {
            if ( stdConffile_Printf("%f\t%f\t%f\t", pSurf->aIntensities[j].red, pSurf->aIntensities[j].green, pSurf->aIntensities[j].blue) )
            {
                goto error;
            }
        }

        if ( stdConffile_WriteLine("\n") )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("\n#--- Surface normals ---\n") )
    {
        goto error;
    }

    for ( i = 0, pSurf = pWorld->aSurfaces; i < pWorld->numSurfaces; ++i, ++pSurf )
    {
        if ( stdConffile_Printf("%d:\t%f\t%f\t%f\n", i, pSurf->face.normal.x, pSurf->face.normal.y, pSurf->face.normal.z) )
        {
            goto error;
        }
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithSurface_ReadSurfacesListText(SithWorld* pWorld, int bSkip)
{
    size_t i;
    size_t numAdjoins;
    size_t numSurfaces;
    size_t curMirrorIdx;
    SithSurfaceAdjoin* pAdjoin;
    rdFace* pFace;
    SithSurface* pSurf;
    int adjArg;


#line 687
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->apMatArray != NULL);

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " world adjoins %d", &numAdjoins) != 1 )
    {
        goto syntax_error;
    }

    if ( sithSurface_AllocWorldAdjoins(pWorld, numAdjoins) )
    {
        goto alloc_error;
    }

    for ( i = 0, pAdjoin = pWorld->aAdjoins; i < numAdjoins; ++i, ++pAdjoin )
    {
        if ( !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }

        adjArg = 1;
        if ( sscanf(stdConffile_entry.aArgs[adjArg++].argValue, "%x", &pAdjoin->flags) != 1 )
        {
            goto syntax_error;
        }

        curMirrorIdx = atoi(stdConffile_entry.aArgs[adjArg++].argValue);
        if ( curMirrorIdx < 0 )
        {
            pAdjoin->pMirrorAdjoin = NULL;
        }
        else
        {
            if ( curMirrorIdx >= pWorld->numAdjoins )
            {
#line 725
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Adjoin mirror index out of range!\n", 0, 0, 0, 0);
                goto range_error;
            }

            pAdjoin->pMirrorAdjoin = &pWorld->aAdjoins[curMirrorIdx];
        }

        pAdjoin->distance      = D3DVAL(atof(stdConffile_entry.aArgs[adjArg++].argValue));
    }

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " world surfaces %d", &numSurfaces) != 1 )
    {
        goto syntax_error;
    }

    if ( sithSurface_AllocWorldSurfaces(pWorld, numSurfaces) )
    {
        goto alloc_error;
    }

    for ( i = 0, pSurf = pWorld->aSurfaces; i < numSurfaces; ++i, ++pSurf )
    {
        size_t j;
        int activeParam;
        size_t cntVerts;
        int matCnt;
        int adjIdx;

        pFace = &pSurf->face;
        if ( !stdConffile_ReadArgs() )
        {
            goto eof_error;
        }

        activeParam = 1;

        matCnt = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
        if ( matCnt < 0 )
        {
            pFace->pMaterial = NULL;
        }
        else
        {
            if ( SITHWORLD_IS_STATICINDEX(matCnt) )
            {
                matCnt = SITHWORLD_FROM_STATICINDEX(matCnt);
                if ( matCnt >= (int)sithWorld_pStaticWorld->numMaterials )
                {
#line 778
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Surface material index out of range!\n", 0, 0, 0, 0);
                    goto range_error;
                }

                pSurf->face.pMaterial = &sithWorld_pStaticWorld->aMaterials[matCnt];
            }
            else
            {
                if ( matCnt >= (int)pWorld->sizeMaterials )
                {
#line 785
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Surface material index out of range!\n", 0, 0, 0, 0);
                    goto range_error;
                }

                pFace->pMaterial = pWorld->apMatArray[matCnt];
            }

            if ( pFace->pMaterial && !pFace->pMaterial )
            {
                goto range_error;
            }
        }

        if ( sscanf(stdConffile_entry.aArgs[activeParam++].argValue, "%x", &pSurf->flags) != 1 )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_entry.aArgs[activeParam++].argValue, "%x", &pFace->flags) != 1 )
        {
            goto syntax_error;
        }

        pFace->geometryMode = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
        if ( !pFace->pMaterial )
        {
            pFace->geometryMode = pFace->geometryMode > RD_GEOMETRY_SOLID ? RD_GEOMETRY_SOLID : pFace->geometryMode;
        }

        pFace->lightingMode = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
        if ( (pSurf->flags & (SITH_SURFACE_CEILINGSKY | SITH_SURFACE_HORIZONSKY)) != 0 )
        {
            pFace->lightingMode = RD_LIGHTING_NONE;
        }

        // Texture mode, parsed but unused
        atoi(stdConffile_entry.aArgs[activeParam++].argValue);

        adjIdx = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
        if ( adjIdx != -1 )
        {
            pSurf->pAdjoin = &pWorld->aAdjoins[adjIdx];
            pSurf->pAdjoin->pAdjoinSurface = pSurf;

            // Retail tests the stale adjoin-loop pointer here, not pSurf->pAdjoin
            if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) == 0
                || pFace->pMaterial
                && pFace->geometryMode
                && (pFace->flags & RD_FF_TEX_TRANSLUCENT) == 0
                && pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGB )
            {
                pSurf->pAdjoin->flags |= SITH_ADJOIN_UNKNOWN_80;
            }
        }
        else
        {
            pSurf->pAdjoin = NULL;
        }

        pFace->extraLight.red   = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));
        pFace->extraLight.green = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));
        pFace->extraLight.blue  = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));
        pFace->extraLight.alpha = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));

        cntVerts = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
        if ( cntVerts < 3 || cntVerts > stdConffile_entry.numArgs - activeParam )
        {
            goto syntax_error;
        }

        if ( cntVerts > RDCACHE_MAXFACEVERTICES )
        {
            goto range_error;
        }

#line 861
        pFace->aVertices = (size_t*)STDMALLOC(sizeof(int) * cntVerts);
        if ( !pFace->aVertices )
        {
            goto alloc_error;
        }

#line 867
        pSurf->aIntensities = (rdVector4*)STDMALLOC(sizeof(rdVector4) * cntVerts);
        if ( !pSurf->aIntensities )
        {
            goto alloc_error;
        }

#line 873
        pFace->aTexVertices = (size_t*)STDMALLOC(sizeof(int) * cntVerts);
        if ( !pFace->aTexVertices )
        {
            goto alloc_error;
        }

        if ( pFace->pMaterial )
        {
            for ( j = 0; j < cntVerts; ++j )
            {
                pFace->aVertices[j]    = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
                pFace->aTexVertices[j] = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
            }
        }
        else
        {
            for ( j = 0; j < cntVerts; ++j )
            {
                pFace->aVertices[j] = atoi(stdConffile_entry.aArgs[activeParam++].argValue);
                ++activeParam;
                pFace->aTexVertices[j] = 0;
            }
        }

        for ( j = 0; j < cntVerts; ++j )
        {
            pSurf->aIntensities[j].red   = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));
            pSurf->aIntensities[j].green = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));
            pSurf->aIntensities[j].blue  = D3DVAL(atof(stdConffile_entry.aArgs[activeParam++].argValue));
            pSurf->aIntensities[j].alpha = pFace->extraLight.alpha;
        }

        pFace->num         = i;
        pFace->numVertices = cntVerts;
    }

    for ( i = 0, pSurf = pWorld->aSurfaces; i < numSurfaces; ++i, ++pSurf )
    {
        float nx;
        int normNum;
        float ny;
        float nz;

        pFace = &pSurf->face;
        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, "%d: %f %f %f", &normNum, &nx, &ny, &nz) != 4 )
        {
            goto syntax_error;
        }

        pFace->normal.x = nx;
        pFace->normal.y = ny;
        pFace->normal.z = nz;
    }

    pWorld->numSurfaces = numSurfaces;
    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }

    pWorld->apMatArray = NULL;
    return 0;

eof_error:
#line 945
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", stdConffile_GetFilename(), 0, 0, 0);
    goto error;

syntax_error:
#line 950
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);
    goto error;

alloc_error:
#line 955
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);
    goto error;

range_error:
#line 960
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Value out of range for '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);

error:
    return 1;
}

int J3DAPI sithSurface_ValidateSurfacePointer(const SithSurface* pSurf)
{
    if ( !pSurf )
    {
        return 0;
    }

    if ( !pSurf->pSector )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithSurface_ValidateWorldSurfaces(const SithWorld* pWorld)
{
    size_t i;
    const SithSurface* pSurf;

#line 1021
    SITH_ASSERTREL(pWorld != NULL);

    for ( i = 0, pSurf = pWorld->aSurfaces; i < pWorld->numSurfaces; ++i, ++pSurf )
    {
        if ( !sithSurface_ValidateSurfacePointer(pSurf) )
        {
            return 0;
        }
    }

    return 1;
}

void J3DAPI sithSurface_HandleThingImpact(SithSurface* pSurf, SithThing* pThing, float damage, int damageType)
{
    if ( stdComm_IsGameActive() && pThing && (pThing->flags & SITH_TF_REMOTE) != 0 )
    {
        return;
    }

    if ( pThing->type == SITH_THING_WEAPON && (pThing->thingInfo.weaponInfo.flags & SITH_WF_IMPACTSOUND) != 0 )
    {
        sithSurface_PlaySurfaceHitSound(pSurf, pThing, damageType);
    }

    if ( (pSurf->flags & SITH_SURFACE_COGLINKED) != 0 )
    {
        sithCog_SurfaceSendMessageEx(pSurf, pThing, SITHCOG_MSG_DAMAGED, (int32_t)damage, damageType, 0, 0);
    }
}

void J3DAPI sithSurface_PlaySurfaceHitSound(const SithSurface* pSurf, SithThing* pThing, int damageType)
{
    tSoundHandle hSnd;
    if ( (damageType & SITH_DAMAGE_MACHETE) != 0 )
    {
        if ( (pSurf->flags & SITH_SURFACE_METAL) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(82)); // 0x8052
        }
        else if ( (pSurf->flags & (SITH_SURFACE_SNOW | SITH_SURFACE_EARTH | 0x80000000)) != 0 ) // 0x80000000 - EARTH_ECHO
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(84)); //0x8054
        }
        else if ( (pSurf->flags & SITH_SURFACE_WEB) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(85)); // 0x8055
        }
        else if ( (pSurf->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_WOOD)) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(83)); // 0x8053
        }
        else
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(81)); // 0x8051
        }

        sithSoundMixer_PlaySoundPos(hSnd, &pThing->pos, pThing->pInSector, 1.0f, 0.5f, 2.5f, SOUNDPLAY_ABSOLUTE_POS);
    }
}

void J3DAPI sithSurface_GetCenterPoint(const SithSurface* pSurface, rdVector3* centerpoint)
{
    size_t i;
    rdVector3 offset;
    rdVector3 curvertSum;

    memset(&curvertSum, 0, sizeof(curvertSum));
    for ( i = 0; i < pSurface->face.numVertices; ++i )
    {
        rdVector_Add3Acc(&curvertSum, &sithWorld_pCurrentWorld->aVertices[pSurface->face.aVertices[i]]);
    }

    rdVector_InvScale3(centerpoint, &curvertSum, (float)pSurface->face.numVertices);

    if ( !sithIntersect_IsSphereInSector(sithWorld_pCurrentWorld, centerpoint, 0.0f, pSurface->pSector) )
    {
        rdVector_Scale3(&offset, &pSurface->face.normal, 0.0002f);
        rdVector_Add3Acc(centerpoint, &offset);
    }

    if ( !sithIntersect_IsSphereInSector(sithWorld_pCurrentWorld, centerpoint, 0.0f, pSurface->pSector) )
    {
#line 1136
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cannot place surface centerpoint in parent sector.\n", 0, 0, 0, 0);
    }
}

void J3DAPI sithSurface_SyncSurface(SithSurface* pSurface)
{
#line 1157
    SITH_ASSERTREL(pSurface);

    pSurface->flags |= SITH_SURFACE_SYNC;
    if ( sithSurface_numUnsyncedSurfaces >= STD_ARRAYLEN(sithSurface_apUnsyncedSurfaces) )
    {
        return;
    }

    sithSurface_apUnsyncedSurfaces[sithSurface_numUnsyncedSurfaces++] = pSurface;
}

void J3DAPI sithSurface_SyncSurfaces(void)
{
    size_t i;

    if ( !sithMessage_outputstream || !sithSurface_numUnsyncedSurfaces )
    {
        return;
    }

    for ( i = 0; i < sithSurface_numUnsyncedSurfaces; ++i )
    {
        sithDSS_SurfaceStatus(sithSurface_apUnsyncedSurfaces[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
    }

    sithSurface_numUnsyncedSurfaces = 0;
}

SithSurface* J3DAPI sithSurface_GetSurfaceEx(const SithWorld* pWorld, int surfIdx)
{
    if ( !pWorld || surfIdx < 0 || surfIdx >= (int)pWorld->numSurfaces )
    {
        return NULL;
    }

    return &pWorld->aSurfaces[surfIdx];
}

int J3DAPI sithSurface_GetSurfaceIndex(const SithSurface* pSurf)
{
    if ( pSurf >= sithWorld_pCurrentWorld->aSurfaces && pSurf <= &sithWorld_pCurrentWorld->aSurfaces[sithWorld_pCurrentWorld->numSurfaces] )
    {
        return pSurf - sithWorld_pCurrentWorld->aSurfaces;
    }

    return -1;
}

int J3DAPI sithSurface_GetSurfaceIndexEx(const SithWorld* pWorld, const SithSurface* pSurface)
{
    if ( pSurface >= pWorld->aSurfaces && pSurface <= &pWorld->aSurfaces[pWorld->numSurfaces] )
    {
        return pSurface - pWorld->aSurfaces;
    }

    return -1;
}
