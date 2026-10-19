#include "sithSector.h"

#include <rdroid/Math/rdVector.h>

#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSound.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

static SithSector* sithSector_aModifiedSectors[16] = { 0 }; // zero initializers give the retail declaration-ordered BSS layout
static int sithSector_aSyncFlags[16] = { 0 };
size_t sithSector_numModifiedSectors = 0;

#ifdef J3D_DEBUG
// Debug-only text writer (debug 0x477EC0), called only by the debug build's sithWrite_Write. Its THRUST test needs a
// float 0.0 constant, and retail's sithSector object has no .rdata at all, so release never compiled it.
int J3DAPI sithSector_WriteSectorsListText(const SithWorld* pWorld)
{
    size_t i;
    const SithSector* pSec;
    size_t j;

    if ( stdConffile_WriteLine("###### Sector information ######\n") ) goto error;
    if ( stdConffile_WriteLine("Section: SECTORS\n\n") ) goto error;
    if ( stdConffile_Printf("World sectors %d\n\n", pWorld->numSectors) ) goto error;

    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        pSec = &pWorld->aSectors[i];
        if ( stdConffile_Printf("\nSECTOR\t%d\nFLAGS\t0x%x\nAMBIENT LIGHT\t%.2f %.2f %.2f\nEXTRA LIGHT\t%.2f %.2f %.2f\n", i, pSec->flags, pSec->ambientLight.red, pSec->ambientLight.green, pSec->ambientLight.blue, pSec->extraLight.red, pSec->extraLight.green, pSec->extraLight.blue) ) goto error;
        if ( stdConffile_Printf("TINT\t%.2f\t%.2f\t%.2f\n", pSec->tint.red, pSec->tint.green, pSec->tint.blue) ) goto error;
        if ( stdConffile_Printf("AVERAGE LIGHT INTENSITY\t%.4f\t%.4f\t%.4f\n", pSec->light.color.red, pSec->light.color.green, pSec->light.color.blue) ) goto error;
        if ( stdConffile_Printf("AVERAGE LIGHT POSITION\t%f\t%f\t%f\n", pSec->light.pos.x, pSec->light.pos.y, pSec->light.pos.z) ) goto error;
        if ( stdConffile_Printf("AVERAGE LIGHT FALLOFF\t%f\t%f\n", pSec->light.minRadius, pSec->light.maxRadius) ) goto error;
        if ( stdConffile_Printf("BOUNDBOX\t%f %f %f %f %f %f\n", pSec->boundBox.v0.x, pSec->boundBox.v0.y, pSec->boundBox.v0.z, pSec->boundBox.v1.x, pSec->boundBox.v1.y, pSec->boundBox.v1.z) ) goto error;
        if ( (pSec->flags & SITH_SECTOR_HASCOLLIDEBOX) && stdConffile_Printf("COLLIDEBOX\t%f %f %f %f %f %f\n", pSec->collideBox.v0.x, pSec->collideBox.v0.y, pSec->collideBox.v0.z, pSec->collideBox.v1.x, pSec->collideBox.v1.y, pSec->collideBox.v1.z) ) goto error;
        if ( pSec->hAmbientSound && stdConffile_Printf("SOUND\t%s %f\n", Sound_GetSoundFilename(pSec->hAmbientSound), pSec->ambientSoundVolume) ) goto error;
        if ( (pSec->thrust.x != 0.0f || pSec->thrust.y != 0.0f || pSec->thrust.z != 0.0f) && stdConffile_Printf("THRUST\t%f %f %f\n", pSec->thrust.x, pSec->thrust.y, pSec->thrust.z) ) goto error;
        if ( stdConffile_Printf("CENTER\t%f %f %f\n", pSec->center.x, pSec->center.y, pSec->center.z) ) goto error;
        if ( stdConffile_Printf("RADIUS\t%f\n", pSec->radius) ) goto error;
        if ( stdConffile_Printf("VERTICES\t%d\n", pSec->numVertices) ) goto error;

        for ( j = 0; j < pSec->numVertices; ++j )
        {
            if ( stdConffile_Printf("%d:\t%d\n", j, pSec->aVertIdxs[j]) ) goto error;
        }

        if ( stdConffile_Printf("SURFACES\t%d\t%d\n\n", sithSurface_GetSurfaceIndexEx(pWorld, pSec->pFirstSurface), pSec->numSurfaces) ) goto error;
    }

    return 0;

error:
    return 1;
}
#endif

// Text of the debug build (debug 0x478330, /Od): the EOF (360), syntax (365) and
// allocation (370) prints are blocks after the success `return 0`, the first two
// ending `goto error` on one `return 1` after the third; no failure-kind flag (the
// debug frame holds exactly the other locals). Shared error-block labels plus
// the debug build's `error` label, not a `bEof` flag. The debug
// code lacks the NOGRAVITY clear (retail-only, the +1 line drift).
//
int J3DAPI sithSector_ReadSectorsListText(SithWorld* pWorld, int bSkip)
{
    size_t numSectors;
    size_t i;
    SithSector* pSec;
    int num;
    float red;
    float green;
    float blue;
    float minRadius;
    float maxRadius;
    char aSound[64];
    size_t numVerts;
    size_t j;
    int vertNum;
    int surfIdx;
    size_t numSurfs;
    SithSurface* pSurf;

    if ( bSkip )
    {
        return 1;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }
    if ( sscanf(stdConffile_aLine, " world sectors %d", &numSectors) != 1 )
    {
        goto syntax_error;
    }
    if ( sithSector_AllocWorldSectors(pWorld, numSectors) )
    {
        goto alloc_error;
    }

    for ( i = 0, pSec = pWorld->aSectors; i < numSectors; ++i, ++pSec )
    {
        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " sector %d", &num) != 1 )
        {
            goto syntax_error;
        }

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " flags %x", &pSec->flags) != 1 )
        {
            goto syntax_error;
        }
        pSec->flags &= ~SITH_SECTOR_NOGRAVITY;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " ambient light %f %f %f", &red, &green, &blue) != 3 )
        {
            goto syntax_error;
        }
        pSec->ambientLight.red   = red;
        pSec->ambientLight.green = green;
        pSec->ambientLight.blue  = blue;
        pSec->ambientLight.alpha = 0.0f;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " extra light %f %f %f", &red, &green, &blue) != 3 )
        {
            goto syntax_error;
        }
        pSec->extraLight.red   = red;
        pSec->extraLight.green = green;
        pSec->extraLight.blue  = blue;
        pSec->extraLight.alpha = 0.0f;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " tint %f %f %f", &pSec->tint.red, &pSec->tint.green, &pSec->tint.blue) == 3 && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " average light intensity %f %f %f", &pSec->light.color.red, &pSec->light.color.green, &pSec->light.color.blue) == 3 && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " average light position %f %f %f", &pSec->light.pos.x, &pSec->light.pos.y, &pSec->light.pos.z) == 3 && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " average light falloff %f %f", &minRadius, &maxRadius) == 2 )
        {
            pSec->light.minRadius = minRadius;
            pSec->light.maxRadius = maxRadius;
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
        }

        if ( sscanf(stdConffile_aLine, " boundbox %f %f %f %f %f %f ", &pSec->boundBox.v0.x, &pSec->boundBox.v0.y, &pSec->boundBox.v0.z, &pSec->boundBox.v1.x, &pSec->boundBox.v1.y, &pSec->boundBox.v1.z) != 6 )
        {
            goto syntax_error;
        }
        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, " collidebox %f %f %f %f %f %f ", &pSec->collideBox.v0.x, &pSec->collideBox.v0.y, &pSec->collideBox.v0.z, &pSec->collideBox.v1.x, &pSec->collideBox.v1.y, &pSec->collideBox.v1.z) == 6 )
        {
            pSec->flags |= SITH_SECTOR_HASCOLLIDEBOX;
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
        }

        if ( sscanf(stdConffile_aLine, "sound %s %f", aSound, &pSec->ambientSoundVolume) == 2 )
        {
            pSec->hAmbientSound = sithSound_Load(pWorld, aSound);
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
        }

        if ( sscanf(stdConffile_aLine, "thrust %f %f %f", &pSec->thrust.x, &pSec->thrust.y, &pSec->thrust.z) == 3 && !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, " center %f %f %f", &pSec->center.x, &pSec->center.y, &pSec->center.z) != 3 )
        {
            goto syntax_error;
        }

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " radius %f", &pSec->radius) != 1 )
        {
            goto syntax_error;
        }

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " vertices %d", &numVerts) != 1 )
        {
            goto syntax_error;
        }

#line 320
        pSec->aVertIdxs = (size_t*)STDMALLOC(sizeof(int) * numVerts);
        if ( !pSec->aVertIdxs )
        {
            goto alloc_error;
        }

        for ( j = 0; j < numVerts; ++j )
        {
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }
            if ( sscanf(stdConffile_aLine, " %d: %d", &num, &vertNum) != 2 )
            {
                goto syntax_error;
            }
            pSec->aVertIdxs[j] = vertNum;
        }

        pSec->numVertices = numVerts;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }
        if ( sscanf(stdConffile_aLine, " surfaces %d %d", &surfIdx, &numSurfs) != 2 )
        {
            goto syntax_error;
        }

        pSec->numSurfaces   = numSurfs;
        pSec->pFirstSurface = sithSurface_GetSurfaceEx(pWorld, surfIdx);

        pSurf = pSec->pFirstSurface;
        for ( j = 0; j < numSurfs; ++j )
        {
            pSurf->pSector = pSec;
            ++pSurf;
        }

    }

    return 0;

eof_error:
#line 360
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", stdConffile_GetFilename(), 0, 0, 0);
    goto error;

syntax_error:
#line 365
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);
    goto error;

alloc_error:
#line 370
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);

error:
    return 1;
}

int J3DAPI sithSector_WriteSectorsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t numVertices;
    size_t nWritten;
    int32_t* aVerts;
    int32_t* pVertNum;
    const SithSector* pSec;
    CndSectorInfo* pSecInfo;
    const char* pSecSound;

    size_t sizes = sizeof(CndSectorInfo) * pWorld->numSectors;
#line 388
    CndSectorInfo* aSectorInfos = (CndSectorInfo*)STDMALLOC(sizes);
    size_t i;

    if ( !aSectorInfos )
    {
        return 1;
    }

    // Collect sector infos
    numVertices = 0;
    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        pSec = &pWorld->aSectors[i];
        pSecInfo = &aSectorInfos[i];

        pSecInfo->flags = pSec->flags;
        rdVector_Copy4(&pSecInfo->ambientLight, &pSec->ambientLight);
        rdVector_Copy4(&pSecInfo->extraLight, &pSec->extraLight);
        rdVector_Copy3(&pSecInfo->tint, &pSec->tint);

        rdVector_Copy3(&pSecInfo->light.pos, &pSec->light.pos);
        rdVector_Copy4(&pSecInfo->light.color, &pSec->light.color);
        pSecInfo->light.minRadius = pSec->light.minRadius;
        pSecInfo->light.maxRadius = pSec->light.maxRadius;

        pSecInfo->collideBox = pSec->collideBox;
        pSecInfo->boundBox   = pSec->boundBox;

        if ( pSec->hAmbientSound )
        {
            pSecSound = Sound_GetSoundFilename(pSec->hAmbientSound);
            if ( pSecSound )
            {
                memcpy(pSecInfo->aAmbientSound, pSecSound, sizeof(pSecInfo->aAmbientSound));
            }
            else
            {
                memset(pSecInfo->aAmbientSound, 0, sizeof(pSecInfo->aAmbientSound));
            }
        }
        else
        {
            memset(pSecInfo->aAmbientSound, 0, sizeof(pSecInfo->aAmbientSound));
        }

        pSecInfo->ambientSoundVolume = pSec->ambientSoundVolume;

        rdVector_Copy3(&pSecInfo->center, &pSec->center);

        pSecInfo->radius = pSec->radius;

        pSecInfo->firstSurfaceIdx = sithSurface_GetSurfaceIndexEx(pWorld, pSec->pFirstSurface);
        pSecInfo->numSurfaces     = pSec->numSurfaces;
        pSecInfo->pvsIdx          = pSec->pvsIdx;
        pSecInfo->numVertices     = pSec->numVertices;

        rdVector_Copy3(&pSecInfo->thrust, &pSec->thrust);

        numVertices += pSec->numVertices;
    }

    // Now write infos to file
    nWritten = sith_pHS->pFileWrite(fh, aSectorInfos, sizes);
    stdMemory_Free(aSectorInfos);

    if ( nWritten != sizes )
    {
        return 1;
    }

    // TODO: Check that numVertices > max uint32_t
    nWritten = sith_pHS->pFileWrite(fh, &numVertices, sizeof(uint32_t));
    if ( nWritten != sizeof(uint32_t) )
    {
        return 1;
    }

    // Now write verts indices of all sectors
    sizes = sizeof(int32_t) * numVertices;
#line 441
    aVerts = (int32_t*)STDMALLOC(sizes);
    if ( !aVerts )
    {
        return 1;
    }

    pVertNum = aVerts;
    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        pSec = &pWorld->aSectors[i];
        memcpy(pVertNum, pSec->aVertIdxs, sizeof(int32_t) * pSec->numVertices);
        pVertNum += pSec->numVertices;
    }

    nWritten = sith_pHS->pFileWrite(fh, aVerts, sizes);
    stdMemory_Free(aVerts);
    if ( nWritten != sizes )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithSector_ReadSectorsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    size_t sizes;
    uint32_t numVerts;
    CndSectorInfo* aSecInfos;
    int32_t* aVerts;
    int32_t* pCurVertNum;
    SithSector* pSec;
    const CndSectorInfo* pSecInfo;
    SithSurface* pSurf;
    size_t j;
    size_t nRead;
    size_t numSurfaces;

    sizes = sizeof(CndSectorInfo) * pWorld->numSectors;
#line 473
    aSecInfos = (CndSectorInfo*)STDMALLOC(sizes);
    if ( !aSecInfos )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aSecInfos, sizes);
    if ( nRead != sizes )
    {
        goto error;
    }

    if ( sithSector_AllocWorldSectors(pWorld, pWorld->numSectors) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        pSec     = &pWorld->aSectors[i];
        pSecInfo = &aSecInfos[i];

        pSec->flags = pSecInfo->flags & ~SITH_SECTOR_NOGRAVITY;

        rdVector_Copy4(&pSec->ambientLight, &pSecInfo->ambientLight);
        rdVector_Copy4(&pSec->extraLight, &pSecInfo->extraLight);
        rdVector_Copy3(&pSec->tint, &pSecInfo->tint);

        rdVector_Copy3(&pSec->light.pos, &pSecInfo->light.pos);
        rdVector_Copy4(&pSec->light.color, &pSecInfo->light.color);
        pSec->light.minRadius = pSecInfo->light.minRadius;
        pSec->light.maxRadius = pSecInfo->light.maxRadius;

        pSec->collideBox = pSecInfo->collideBox;
        pSec->boundBox   = pSecInfo->boundBox;

        if ( pSecInfo->aAmbientSound[0] )
        {
            pSec->hAmbientSound = sithSound_Load(pWorld, pSecInfo->aAmbientSound);
        }
        else
        {
            pSec->hAmbientSound = 0;
        }

        pSec->ambientSoundVolume = pSecInfo->ambientSoundVolume;
        rdVector_Copy3(&pSec->center, &pSecInfo->center);
        pSec->radius = pSecInfo->radius;

        pSec->pFirstSurface = sithSurface_GetSurfaceEx(pWorld, pSecInfo->firstSurfaceIdx);
        pSec->numSurfaces   = pSecInfo->numSurfaces;
        pSec->numVertices   = pSecInfo->numVertices;
        pSec->pvsIdx        = pSecInfo->pvsIdx;

        rdVector_Copy3(&pSec->thrust, &pSecInfo->thrust);

        pSurf = pSec->pFirstSurface;
        numSurfaces = pSec->numSurfaces;
        for ( j = 0; j < numSurfaces; ++j )
        {
            pSurf->pSector = pSec;
            ++pSurf;
        }
    }

    if ( aSecInfos )
    {
        stdMemory_Free(aSecInfos);
    }

    nRead = sith_pHS->pFileRead(fh, &numVerts, sizeof(numVerts));
    if ( nRead != sizeof(numVerts) )
    {
        return 1;
    }

    sizes = sizeof(int32_t) * numVerts;
#line 526
    aVerts = (int32_t*)STDMALLOC(sizes);
    if ( !aVerts )
    {
        return 1;
    }

    nRead = sith_pHS->pFileRead(fh, aVerts, sizes);
    if ( nRead != sizes )
    {
        goto error;
    }

    pCurVertNum = aVerts;
    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        pSec = &pWorld->aSectors[i];
#line 538
        pSec->aVertIdxs = (size_t*)STDMALLOC(sizeof(int32_t) * pSec->numVertices);
        if ( !pSec->aVertIdxs )
        {
            goto error;
        }

        memcpy(pSec->aVertIdxs, pCurVertNum, sizeof(int32_t) * pSec->numVertices);
        pCurVertNum += pSec->numVertices;
    }

    stdMemory_Free(aVerts);
    return 0;

error:
    if ( pWorld->aSectors )
    {
        for ( i = 0; i < pWorld->numSectors; ++i )
        {
            pSec = &pWorld->aSectors[i];
            if ( pSec->aVertIdxs )
            {
                stdMemory_Free(pSec->aVertIdxs);
            }
        }

        stdMemory_Free(pWorld->aSectors);
        pWorld->aSectors = NULL;
    }

    if ( aSecInfos )
    {
        stdMemory_Free(aSecInfos);
    }

    if ( aVerts )
    {
        stdMemory_Free(aVerts);
    }

    return 1;
}

int J3DAPI sithSector_ValidateSectorPointer(const SithWorld* pWorld, const SithSector* pSector)
{
    if ( !pWorld || !pSector || pSector < pWorld->aSectors || pSector >= &pWorld->aSectors[pWorld->numSectors] )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithSector_AllocWorldSectors(SithWorld* pWorld, size_t numSectors)
{
#line 620
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aSectors == NULL);

#line 623
    pWorld->aSectors = (SithSector*)STDMALLOC(sizeof(SithSector) * numSectors);
    if ( !pWorld->aSectors )
    {
#line 626
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate mem for %d sectors.\n", numSectors, 0, 0, 0);
        return 1;
    }

    memset(pWorld->aSectors, 0, sizeof(SithSector) * numSectors);
    pWorld->numSectors = numSectors;
    sithSector_ResetAllSectors(pWorld);
    return 0;
}

void J3DAPI sithSector_ResetAllSectors(SithWorld* pWorld)
{
    SithSector* pSec = pWorld->aSectors;
    size_t i;
    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        pSec->numVertices         = 0;
        pSec->aVertIdxs           = NULL;
        pSec->numSurfaces         = 0;
        pSec->pFirstSurface       = NULL;
        pSec->pFirstThingInSector = NULL;
        ++pSec;
    }
}

void J3DAPI sithSector_FreeWorldSectors(SithWorld* pWorld)
{
    size_t i;
    SithSector* pSec;

#line 690
    SITH_ASSERTREL(pWorld->aSectors);

    for ( i = 0, pSec = pWorld->aSectors; i < pWorld->numSectors; ++i, ++pSec )
    {
        if ( pSec->aVertIdxs )
        {
            stdMemory_Free(pSec->aVertIdxs);
        }
    }

    stdMemory_Free(pWorld->aSectors);
    pWorld->aSectors   = NULL;
    pWorld->numSectors = 0;
}

SithSector* J3DAPI sithSector_FindSectorAtPos(const SithWorld* pWorld, const rdVector3* pos)
{
    size_t i;
    size_t numSectors;
    SithSector* pSec;

#line 734
    SITH_ASSERTREL(pos != NULL);
    SITH_ASSERTREL(pWorld != NULL);

    numSectors = pWorld->numSectors;
    pSec = pWorld->aSectors;
    for ( i = 0; i < numSectors; ++pSec, ++i )
    {
        if ( !J3DINRANGE(pos->x, pSec->boundBox.v0.x, pSec->boundBox.v1.x) )
        {
            continue;
        }

        if ( !J3DINRANGE(pos->y, pSec->boundBox.v0.y, pSec->boundBox.v1.y) )
        {
            continue;
        }

        if ( !J3DINRANGE(pos->z, pSec->boundBox.v0.z, pSec->boundBox.v1.z) )
        {
            continue;
        }

        if ( sithIntersect_IsSphereInSector(pWorld, pos, 0.0f, pSec) )
        {
            return pSec;
        }
    }

    return NULL;
}

void J3DAPI sithSector_HideSectorAdjoins(SithSector* pSector)
{
    SithSurfaceAdjoin* i;

#line 781
    SITH_ASSERTREL(pSector);
    for ( i = pSector->pFirstAdjoin; i; i = i->pNextAdjoin )
    {
        sithSurface_HideSectorAdjoin(i);
    }

    pSector->flags |= SITH_SECTOR_ADJOINSOFF;
}

void J3DAPI sithSector_ShowSectorAdjoins(SithSector* pSector)
{
    SithSurfaceAdjoin* i;

    for ( i = pSector->pFirstAdjoin; i; i = i->pNextAdjoin )
    {
        sithSurface_ShowSectorAdjoin(i);
    }

    pSector->flags &= ~SITH_SECTOR_ADJOINSOFF;
}

void J3DAPI sithSector_SetSectorFlags(SithSector* pSector, SithSectorFlag flags)
{
    SithSectorFlag prevflags;
    SithThing* pCurrentCheckThing;

#line 839
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));

    prevflags = pSector->flags;
    pSector->flags |= flags;

    for ( pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
#line 848
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pCurrentCheckThing));
        if ( (prevflags & SITH_SECTOR_UNDERWATER) == 0 && (pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            switch ( pCurrentCheckThing->type )
            {
                case SITH_THING_ACTOR:
                case SITH_THING_PLAYER:
                    sithThing_ExitSector(pCurrentCheckThing);
                    sithThing_EnterSector(pCurrentCheckThing, pSector, 0, 0);
                    break;

                case SITH_THING_PARTICLE:
                case SITH_THING_SPRITE:
                    sithThing_DestroyThing(pCurrentCheckThing);
                    break;
            }
        }
    }
}

void J3DAPI sithSector_ClearSectorFlags(SithSector* pSector, SithSectorFlag flags)
{
    SithSectorFlag prevflags;
    SithThing* pCurrentCheckThing;

#line 878
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));

    prevflags = pSector->flags;
    pSector->flags &= ~flags;

    for ( pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
#line 887
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pCurrentCheckThing));
        if ( (prevflags & SITH_SECTOR_UNDERWATER) != 0 && (pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            switch ( pCurrentCheckThing->type )
            {
                case SITH_THING_ACTOR:
                case SITH_THING_PLAYER:
                    sithThing_ExitSector(pCurrentCheckThing);
                    sithThing_EnterSector(pCurrentCheckThing, pSector, 0, 0);
                    break;

                case SITH_THING_PARTICLE:
                case SITH_THING_SPRITE:
                    sithThing_DestroyThing(pCurrentCheckThing);
                    break;
            }
        }
    }
}

size_t J3DAPI sithSector_GetSectorThingCount(const SithSector* pSector)
{
    size_t count = 0;
    SithThing* pCurrentCheckThing;

#line 929
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));

    for ( pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
#line 935
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pCurrentCheckThing));
        ++count;
    }

    return count;
}

size_t J3DAPI sithSector_GetSectorPlayerCount(const SithSector* pSector)
{
    size_t count = 0;
    SithThing* pCurrentCheckThing;

#line 960
    SITH_ASSERTREL(pSector != NULL);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));

    for ( pCurrentCheckThing = pSector->pFirstThingInSector; pCurrentCheckThing; pCurrentCheckThing = pCurrentCheckThing->pNextThingInSector )
    {
#line 966
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pCurrentCheckThing));
        if ( pCurrentCheckThing->type == SITH_THING_PLAYER )
        {
            ++count;
        }
    }

    return count;
}

/* PLACEHOLDER, linker-discarded: retail __LINE__ gap 966..1012 (sithSector_GetSectorPlayerCount -> sithSector_SyncSector), where the original
   defined it. Body from the debug build (0x00479AC8, debug lines 979/980 = retail 981/982): the two
   sector asserts (compiled out) and a count over the pFirstAdjoin/pNextAdjoin list. */
size_t J3DAPI sithSector_GetAdjoinCount(const SithSector* pSector)
{
    size_t count = 0;
    SithSurfaceAdjoin* pCurrentAdjoin;

#line 981
    SITH_ASSERT(pSector != NULL);
    SITH_ASSERT(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));

    pCurrentAdjoin = pSector->pFirstAdjoin;
    while ( pCurrentAdjoin )
    {
        ++count;
        pCurrentAdjoin = pCurrentAdjoin->pNextAdjoin;
    }

    return count;
}

void J3DAPI sithSector_SyncSector(SithSector* pSector, int flags)
{
    size_t i;

#line 1012
    SITH_ASSERTREL(pSector);

    pSector->flags |= SITH_SECTOR_SYNC;
    if ( !sithMessage_outputstream )
    {
        return;
    }

    // TODO: Wouldn't it make more sense for this check to move after for loop
    if ( sithSector_numModifiedSectors >= STD_ARRAYLEN(sithSector_aModifiedSectors) )
    {
#line 1022
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many modified sectors.\n", 0, 0, 0, 0);
        return;
    }

    for ( i = 0; i < sithSector_numModifiedSectors; ++i )
    {
        if ( sithSector_aModifiedSectors[i] == pSector )
        {
            sithSector_aSyncFlags[i] |= flags;
            return;
        }
    }

    sithSector_aModifiedSectors[sithSector_numModifiedSectors] = pSector;
    sithSector_aSyncFlags[sithSector_numModifiedSectors++] = flags;
}

void J3DAPI sithSector_SyncSectors(void)
{
    size_t i;
    for ( i = 0; i < sithSector_numModifiedSectors; ++i )
    {
        if ( (sithSector_aSyncFlags[i] & 1) != 0 )
        {
            sithDSS_SectorStatus(sithSector_aModifiedSectors[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
        else
        {
            sithDSS_SectorFlags(sithSector_aModifiedSectors[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }

    sithSector_numModifiedSectors = 0;
}

SithSector* J3DAPI sithSector_GetSectorEx(const SithWorld* pWorld, int idx)
{
    if ( !pWorld || idx < 0 || idx >= (int)pWorld->numSectors )
    {
        return NULL;
    }

    return &pWorld->aSectors[idx];
}

int J3DAPI sithSector_GetSectorIndexEx(const SithWorld* pWorld, const SithSector* pSector)
{
    if ( pSector >= pWorld->aSectors && pSector <= &pWorld->aSectors[pWorld->numSectors] )
    {
        return pSector - pWorld->aSectors;
    }

    return -1;
}

int J3DAPI sithSector_GetSectorIndex(const SithSector* pSector)
{
    if ( pSector >= sithWorld_pCurrentWorld->aSectors && pSector <= &sithWorld_pCurrentWorld->aSectors[sithWorld_pCurrentWorld->numSectors] )
    {
        return pSector - sithWorld_pCurrentWorld->aSectors;
    }

    return -1;
}
