#include "sithMaterial.h"

#include <rdroid/Engine/rdMaterial.h>

#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdFnames.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#include <stdio.h>
#include <string.h>

#define SITHMATERIAL_TABLESIZE       1024u
#define SITHMATERIAL_EXTRABUFFERSIZE 64u

// Retail spelling recovered from the assertion string "sithMaterial_pHashtable"
static tHashTable* sithMaterial_pHashtable;

rdMaterial* J3DAPI sithMaterial_CacheFind(const char* pName);
void J3DAPI sithMaterial_CacheAdd(rdMaterial* pMat);
int J3DAPI sithMaterial_CacheRemove(const rdMaterial* pMat);

int J3DAPI sithMaterial_Startup(void)
{
    sithMaterial_pHashtable = stdHashtbl_New(SITHMATERIAL_TABLESIZE);
    if ( !sithMaterial_pHashtable )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithMaterial_Shutdown(void)
{
    if ( sithMaterial_pHashtable )
    {
        stdHashtbl_Free(sithMaterial_pHashtable);
    }

    sithMaterial_pHashtable = NULL;
}

void J3DAPI sithMaterial_FreeWorldMaterials(SithWorld* pWorld)
{
    size_t i;

#line 106
    SITH_ASSERTREL(pWorld != NULL);

    if ( !pWorld->sizeMaterials )
    {
#line 110
        SITH_ASSERTREL(pWorld->aMaterials == NULL);
        return;
    }

    for ( i = 0; i < pWorld->numMaterials; ++i )
    {
        sithMaterial_CacheRemove(&pWorld->aMaterials[i]);
        rdMaterial_FreeEntry(&pWorld->aMaterials[i]);
    }

    if ( pWorld->aMaterials )
    {
        stdMemory_Free(pWorld->aMaterials);
    }

    pWorld->aMaterials   = NULL;
    pWorld->numMaterials = 0;

    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }

    pWorld->apMatArray = NULL;
}

/* Linker-discarded: retail __LINE__ gap 110..172 (sithMaterial_FreeWorldMaterials -> sithMaterial_ReadMaterialsListText);
   the debug build keeps it there (0x00474A32). */
int J3DAPI sithMaterial_WriteMaterialsListText(const SithWorld* pWorld)
{
    int i;

    if ( stdConffile_WriteLine("##### Material information #####\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("SECTION: MATERIALS\n\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World materials %d\n\n", pWorld->numMaterials + 64) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#num:\tmat:\n") )
    {
        goto error;
    }

    for ( i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aMaterials[i].aName) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("end\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithMaterial_ReadMaterialsListText(SithWorld* pWorld, int bSkip)
{
    size_t numMaterials;
    float progressStep;
    size_t curMatNum;
    rdMaterial* curMaterial;

    curMatNum = 0;
#line 172
    SITH_ASSERTREL(pWorld != NULL);
    if ( bSkip )
    {
        return 1;
    }

    if ( !stdConffile_ReadLine() )
    {
#line 180
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Read error in materials section, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    sithWorld_UpdateLoadProgress(5.0f);
    if ( sscanf(stdConffile_aLine, " world materials %d", &numMaterials) != 1 )
    {
#line 189
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Syntax error - expected world materials XX on line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    progressStep = SITHWORLD_PROGRESSSTEP(45.0f, numMaterials);
#line 196
    SITH_ASSERTREL(numMaterials > 0);

    if ( sithMaterial_AllocWorldMaterials(pWorld, numMaterials) )
    {
        return 1;
    }

    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }

#line 203
    pWorld->apMatArray = (rdMaterial**)STDMALLOC(sizeof(rdMaterial*) * numMaterials);
    if ( !pWorld->apMatArray )
    {
        return 1;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        curMaterial = sithMaterial_Load(stdConffile_entry.aArgs[1].argValue);
        if ( !curMaterial )
        {
#line 214
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Material load failure %s.\n", stdConffile_entry.aArgs[1].argValue, 0, 0, 0);
        }

        pWorld->apMatArray[curMatNum++] = curMaterial;
        sithWorld_UpdateLoadProgress(((float)curMatNum * progressStep) + 5.0f);
    }

    if ( pWorld->sizeMaterials != curMatNum + SITHMATERIAL_EXTRABUFFERSIZE && pWorld->sizeMaterials != curMatNum )
    {
        return 1;
    }

    sithWorld_UpdateLoadProgress(50.0f);
#line 230
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Allocated %d materials, %d used.\n", pWorld->sizeMaterials, pWorld->numMaterials, 0, 0);
    return 0;
}

int J3DAPI sithMaterial_WriteMaterialsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    tFileHandle fhMat              = 0;
    int bError                     = 1;
    CndMaterialInfo* aMatInfos     = NULL;
    uint8_t* aPixelBuffers         = NULL;
    size_t i;
    size_t sizeInfos;
    size_t sizePixelBuffers;
    const rdMaterial* pMat;
    CndMaterialInfo* pCurInfo;
    size_t texSize;
    size_t j;
    size_t nWritten;
    uint8_t* pCurPixelBuffer;
    rdMatHeader matHeader;
    char aPath[128];

    sizeInfos = sizeof(CndMaterialInfo) * pWorld->numMaterials;
#line 252
    aMatInfos = (CndMaterialInfo*)STDMALLOC(sizeInfos);
    if ( !aMatInfos )
    {
        goto error;
    }

    memset(aMatInfos, 0, sizeInfos);

    pCurInfo = aMatInfos;
    memset(&sizePixelBuffers, 0, sizeof(sizePixelBuffers));
    for ( i = 0; i < pWorld->numMaterials; ++i )
    {
        pMat = &pWorld->aMaterials[i];
        pCurInfo->numMipLevels = std3D_GetMipMapCount(pMat->aTextures);
        texSize = rdMaterial_GetMipSize(pMat->width, pMat->height, pCurInfo->numMipLevels);
        texSize *= 2 * pMat->numCels;
        sizePixelBuffers += texSize;
        ++pCurInfo;
    }

#line 268
    aPixelBuffers = (uint8_t*)STDMALLOC(sizePixelBuffers);
    if ( !aPixelBuffers )
    {
        goto error;
    }

    memset(aPixelBuffers, 0, sizePixelBuffers);

    pCurPixelBuffer = aPixelBuffers;
    pCurInfo = aMatInfos;
    for ( i = 0; i < pWorld->numMaterials; ++i )
    {
        pMat = &pWorld->aMaterials[i];
        STD_MAKEPATH(aPath, "mat", pMat->aName);

        fhMat = sith_pHS->pFileOpen(aPath, "rb");
        if ( fhMat )
        {
            pCurInfo->numCels = pMat->numCels;
            pCurInfo->width   = pMat->width;
            pCurInfo->height  = pMat->height;
            strncpy(pCurInfo->aName, pMat->aName, STD_ARRAYLEN(pCurInfo->aName) - 1);
            pCurInfo->aName[STD_ARRAYLEN(pCurInfo->aName) - 1] = 0;

            if ( sith_pHS->pFileRead(fhMat, &matHeader, sizeof(matHeader)) != sizeof(matHeader) )
            {
                goto error;
            }

            memcpy(&pCurInfo->colorInfo, &matHeader.colorInfo, sizeof(pCurInfo->colorInfo));

            if ( sith_pHS->pFileSeek(fhMat, sizeof(rdMatRecordHeader) * pMat->numCels, 1) )
            {
                goto error;
            }

            texSize = 2 * rdMaterial_GetMipSize(pMat->width, pMat->height, pCurInfo->numMipLevels);
            for ( j = 0; j < pMat->numCels; ++j )
            {
                if ( sith_pHS->pFileSeek(fhMat, sizeof(rdMatTextureHeader), 1) )
                {
                    goto error;
                }

                if ( texSize != sith_pHS->pFileRead(fhMat, pCurPixelBuffer, texSize) )
                {
                    goto error;
                }

                pCurPixelBuffer += texSize;
            }

            sith_pHS->pFileClose(fhMat);
            fhMat = 0;
        }
        else
        {
#line 305
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to open material '%s'.\n", aPath, 0, 0, 0);
            goto error;
        }

        ++pCurInfo;
    }

    nWritten = sith_pHS->pFileWrite(fh, &sizePixelBuffers, sizeof(sizePixelBuffers));
    if ( nWritten != sizeof(sizePixelBuffers) )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aMatInfos, sizeInfos);
    if ( nWritten != sizeInfos )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aPixelBuffers, sizePixelBuffers);
    if ( nWritten != sizePixelBuffers )
    {
        goto error;
    }

    bError = 0;

error:
    if ( fhMat )
    {
        sith_pHS->pFileClose(fhMat);
    }

    if ( aMatInfos )
    {
        stdMemory_Free(aMatInfos);
    }

    if ( aPixelBuffers )
    {
        stdMemory_Free(aPixelBuffers);
    }

    return bError;
}

int J3DAPI sithMaterial_ReadMaterialsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    tRasterInfo rasterInfo;
    size_t numMaterials;
    size_t sizeInfos;
    size_t sizePixelBuffers;
    size_t nRead;
    uint8_t* pCurPixelBuffer;
    size_t matNum;
    size_t worldMatIdx;
    size_t i;
    size_t j;
    const CndMaterialInfo* pCurInfo;
    rdMaterial* pMat;
    ColorInfo desiredColorFormat;
    int bHasColorKey;
    uint32_t colorKey;
    size_t texSize;
    int bError                 = 1;
    CndMaterialInfo* aMatInfos = NULL;
    uint8_t* aPixelBuffers     = NULL;
    tVBuffer** apBuffers       = NULL;

    memset(&rasterInfo, 0, sizeof(rasterInfo));
    if ( pWorld->apMatArray )
    {
        stdMemory_Free(pWorld->apMatArray);
    }

#line 352
    pWorld->apMatArray = (rdMaterial**)STDMALLOC(sizeof(rdMaterial*) * pWorld->sizeMaterials);
    if ( !pWorld->apMatArray )
    {
        return 1;
    }

    numMaterials = pWorld->numMaterials;
    if ( sithMaterial_AllocWorldMaterials(pWorld, pWorld->sizeMaterials) )
    {
        goto error;
    }

    sizeInfos = sizeof(CndMaterialInfo) * numMaterials;
#line 361
    aMatInfos = (CndMaterialInfo*)STDMALLOC(sizeInfos);
    if ( !aMatInfos )
    {
        goto error;
    }

    memset(aMatInfos, 0, sizeInfos);
    pCurInfo = aMatInfos;

    nRead = sith_pHS->pFileRead(fh, &sizePixelBuffers, sizeof(sizePixelBuffers));
    if ( nRead != sizeof(sizePixelBuffers) )
    {
        goto error;
    }

#line 369
    aPixelBuffers = (uint8_t*)STDMALLOC(sizePixelBuffers);
    if ( !aPixelBuffers )
    {
        goto error;
    }

    memset(aPixelBuffers, 0, sizePixelBuffers);
    pCurPixelBuffer = aPixelBuffers;

    nRead = sith_pHS->pFileRead(fh, aMatInfos, sizeInfos);
    if ( nRead != sizeInfos )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aPixelBuffers, sizePixelBuffers);
    if ( nRead != sizePixelBuffers )
    {
        goto error;
    }

    pCurPixelBuffer = aPixelBuffers;
    for ( matNum = 0, worldMatIdx = 0; matNum < numMaterials; ++matNum )
    {
        pMat = sithMaterial_CacheFind(pCurInfo->aName);
        if ( pMat )
        {
            ++pCurInfo;
            texSize = rdMaterial_GetMipSize(pMat->width, pMat->height, pCurInfo->numMipLevels);
            texSize *= 2 * pMat->numCels;
            pCurPixelBuffer += texSize;
            pWorld->apMatArray[matNum] = pMat;
            continue;
        }

        pMat = &pWorld->aMaterials[worldMatIdx++];
        pWorld->apMatArray[matNum] = pMat;

        strncpy(pMat->aName, pCurInfo->aName, STD_ARRAYLEN(pMat->aName) - 1);
        pMat->aName[STD_ARRAYLEN(pMat->aName) - 1] = 0;

        pMat->numCels    = pCurInfo->numCels;
        pMat->formatType = std3D_GetColorFormat(&pCurInfo->colorInfo);
        std3D_GetTextureFormat(pMat->formatType, &desiredColorFormat, &bHasColorKey, &colorKey);

        pMat->width     = pCurInfo->width;
        pMat->height    = pCurInfo->height;
        pMat->aTextures = NULL;

        if ( pMat->numCels )
        {
#line 408
            pMat->aTextures = (tSystemTexture*)STDMALLOC(sizeof(tSystemTexture) * pMat->numCels);
            if ( !pMat->aTextures )
            {
                goto error;
            }

            memset(pMat->aTextures, 0, sizeof(tSystemTexture) * pMat->numCels);
        }

#line 414
        apBuffers = (tVBuffer**)STDMALLOC(sizeof(tVBuffer*) * pCurInfo->numMipLevels);
        if ( !apBuffers )
        {
            goto error;
        }

        memset(apBuffers, 0, sizeof(tVBuffer*) * pCurInfo->numMipLevels);

        rasterInfo.colorInfo = pCurInfo->colorInfo;
        for ( j = 0; j < pCurInfo->numMipLevels; ++j )
        {
            rasterInfo.width  = pMat->width >> j;
            rasterInfo.height = pMat->height >> j;
            apBuffers[j] = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
            if ( !apBuffers[j] )
            {
                goto error;
            }
        }

        for ( i = 0; i < pMat->numCels; ++i )
        {
            for ( j = 0; j < pCurInfo->numMipLevels; ++j )
            {
                apBuffers[j]->rasterInfo.colorInfo = rasterInfo.colorInfo;
                stdDisplay_VBufferLock(apBuffers[j]);
                memcpy(apBuffers[j]->pPixels, pCurPixelBuffer, apBuffers[j]->rasterInfo.size);
                stdDisplay_VBufferUnlock(apBuffers[j]);

                apBuffers[j] = stdDisplay_VBufferConvertColorFormat(&desiredColorFormat, apBuffers[j], bHasColorKey, colorKey);
                pCurPixelBuffer += apBuffers[j]->rasterInfo.size;
            }

            std3D_AllocSystemTexture(&pMat->aTextures[i], apBuffers, pCurInfo->numMipLevels, pMat->formatType);
        }

        for ( j = 0; j < pCurInfo->numMipLevels; ++j )
        {
            stdDisplay_VBufferFree(apBuffers[j]);
            apBuffers[j] = NULL;
        }

        stdMemory_Free(apBuffers);
        apBuffers = NULL;
        ++pCurInfo;

        sithMaterial_CacheAdd(pMat);

        pMat->num = pWorld->numMaterials;
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pMat->num = SITHWORLD_STATICINDEX(pMat->num);
        }

        ++pWorld->numMaterials;
    }

    bError = 0;

error:
    if ( aMatInfos )
    {
        stdMemory_Free(aMatInfos);
    }

    if ( aPixelBuffers )
    {
        stdMemory_Free(aPixelBuffers);
    }

    if ( apBuffers )
    {
        stdMemory_Free(apBuffers);
    }

    return bError;
}

rdMaterial* J3DAPI sithMaterial_Load(const char* pName)
{
    rdMaterial* pMat;
    char aPath[128];
    int bLoaded;

    SithWorld* pWorld = sithWorld_pLastLoadedWorld;
    bLoaded = 0;

#line 513
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(strlen(pName) < 128);
    SITH_ASSERTREL(pWorld->aMaterials != NULL);

    pMat = sithMaterial_CacheFind(pName);
    if ( pMat )
    {
        return pMat;
    }

    if ( !pName[0] )
    {
        return NULL;
    }

#line 530
    SITH_ASSERTREL(pWorld->numMaterials <= pWorld->sizeMaterials);
    if ( pWorld->numMaterials >= pWorld->sizeMaterials )
    {
#line 535
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space left to load new material '%s'.\n", pName, 0, 0, 0);
        return NULL;
    }

    pMat = &pWorld->aMaterials[pWorld->numMaterials];
    STD_MAKEPATH(aPath, "mat", pName);
    if ( !rdMaterial_LoadEntry(aPath, pMat) )
    {
        bLoaded = 1;
    }

    if ( !bLoaded )
    {
#line 558
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Material %s not found.\n", pName, 0, 0, 0);
        return NULL;
    }

    sithMaterial_CacheAdd(pMat);

    pMat->num = pWorld->numMaterials;
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        pMat->num = SITHWORLD_STATICINDEX(pMat->num);
    }

    ++pWorld->numMaterials;
    return pMat;
}

rdMaterial* J3DAPI sithMaterial_GetMaterialByIndex(int index)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        pWorld = sithWorld_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( !pWorld || index < 0 || index >= (int)pWorld->numMaterials )
    {
        return NULL;
    }

    return &pWorld->aMaterials[index];
}

int J3DAPI sithMaterial_AllocWorldMaterials(SithWorld* pWorld, size_t numMaterials)
{
    size_t size;

#line 634
    SITH_ASSERTREL(pWorld->aMaterials == NULL);

    size = sizeof(rdMaterial) * numMaterials;
#line 637
    pWorld->aMaterials = (rdMaterial*)STDMALLOC(size);
    if ( !pWorld->aMaterials )
    {
        return 1;
    }

    memset(pWorld->aMaterials, 0, size);
    pWorld->sizeMaterials = numMaterials;
    pWorld->numMaterials  = 0;

    if ( !sithMaterial_pHashtable )
    {
        sithMaterial_pHashtable = stdHashtbl_New(SITHMATERIAL_TABLESIZE);
        if ( !sithMaterial_pHashtable )
        {
            stdMemory_Free(pWorld->aMaterials);
            return 1;
        }
    }

    return 0;
}

rdMaterial* J3DAPI sithMaterial_CacheFind(const char* pName)
{
#line 664
    SITH_ASSERTREL(sithMaterial_pHashtable);
    return (rdMaterial*)stdHashtbl_Find(sithMaterial_pHashtable, pName);
}

void J3DAPI sithMaterial_CacheAdd(rdMaterial* pMat)
{
#line 670
    SITH_ASSERTREL(sithMaterial_pHashtable);
    stdHashtbl_Add(sithMaterial_pHashtable, pMat->aName, pMat);
}

int J3DAPI sithMaterial_CacheRemove(const rdMaterial* pMat)
{
#line 676
    SITH_ASSERTREL(sithMaterial_pHashtable);
    SITH_ASSERTREL(pMat->aName);
    return stdHashtbl_Remove(sithMaterial_pHashtable, pMat->aName);
}
