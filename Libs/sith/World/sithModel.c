#include "sithModel.h"

#include <rdroid/Primitives/rdModel3.h>

#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdFnames.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/types.h>

#include <stdlib.h>
#include <string.h>

#define SITHMODEL_DEFAULTMODEL "dflt.3do"

// Retail spelling recovered from the assertion string "sithModel_pHashtable != ((void *)0)"
static tHashTable* sithModel_pHashtable;
int sithModel_bHiPoly = 0;

void J3DAPI sithModel_CacheRemove(const rdModel3* pModel);
void J3DAPI sithModel_CacheAdd(rdModel3* pModel);
rdModel3* J3DAPI sithModel_CacheFind(const char* pName);

int J3DAPI sithModel_Startup(void)
{
    sithModel_pHashtable = stdHashtbl_New(256u);
    if ( !sithModel_pHashtable )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithModel_Shutdown(void)
{
    if ( sithModel_pHashtable )
    {
        stdHashtbl_Free(sithModel_pHashtable);
        sithModel_pHashtable = NULL;
    }
}

/* Linker-discarded (called only by the debug-only world writer): debug 0x0045E37F, between Shutdown and
   ReadStaticModelsListText (retail __LINE__ head room). The model count is written with 32 added; every name goes
   through a 128-byte copy. Body from the debug code. */
int J3DAPI sithModel_WriteStaticModelsListText(const SithWorld* pWorld)
{
    int i;
    char aName[128];

    if ( stdConffile_WriteLine("###### Models information ######\n") ) {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: MODELS\n\n") ) {
        goto error;
    }

    if ( stdConffile_Printf("World models %d\n\n", pWorld->numModels + 32) ) {
        goto error;
    }

    for ( i = 0; i < (int)pWorld->numModels; ++i )
    {
        strncpy(aName, pWorld->aModels[i].aName, 127);
        aName[127] = 0;
        if ( stdConffile_Printf("%d:\t%s\n", i, aName) ) {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("end\n") ) {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") ) {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithModel_ReadStaticModelsListText(SithWorld* pWorld, int bSkip)
{
    int numModels;
    float progressDelta;
    float progress;

    if ( bSkip )
    {
        return 1;
    }

#line 128
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aModels == NULL);
    SITH_ASSERTREL(pWorld->numModels == 0);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "world") || strcmp(stdConffile_entry.aArgs[1].argValue, "models") )
    {
#line 136
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading static models list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    numModels = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !numModels )
    {
#line 145
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Models section not needed if numModels == 0.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( sithModel_AllocWorldModels(pWorld, numModels) )
    {
        goto alloc_error;
    }

    sithWorld_UpdateLoadProgress(60.0f);
    progressDelta = SITHWORLD_PROGRESSSTEP(10.0f, numModels);
    progress      = 60.0f;
    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        sithModel_Load(stdConffile_entry.aArgs[1].argValue, 0);
        progress += progressDelta;
        sithWorld_UpdateLoadProgress(progress);
    }

    sithWorld_UpdateLoadProgress(70.0f);
#line 173
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Models read.  Model array space %d, used %d.\n", pWorld->sizeModels, pWorld->numModels, 0, 0);
#line 175
    SITH_ASSERTREL(pWorld->numModels <= pWorld->sizeModels);
    return 0;

alloc_error:
#line 184
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory error while reading models, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;
}

int J3DAPI sithModel_WriteStaticModelsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    char aName[128];
    size_t written;

    for ( i = 0; i < pWorld->numModels; ++i )
    {
        strncpy(aName, pWorld->aModels[i].aName, 127);
        aName[127] = 0;
        written = sith_pHS->pFileWrite(fh, aName, 64);
        if ( written != 64 )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithModel_ReadStaticModelsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    char aName[64];
    size_t nRead;
    size_t numModels = pWorld->numModels;

    if ( sithModel_AllocWorldModels(pWorld, pWorld->numModels) )
    {
        goto error;
    }

    for ( i = 0; i < numModels; ++i )
    {
        nRead = sith_pHS->pFileRead(fh, aName, STD_ARRAYLEN(aName));
        if ( nRead != STD_ARRAYLEN(aName) )
        {
            goto error;
        }

        sithModel_Load(aName, 0);
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithModel_FreeWorldModels(SithWorld* pWorld)
{
    size_t i;

#line 247
    SITH_ASSERTREL(pWorld != NULL);

    if ( !pWorld->sizeModels )
    {
#line 251
        SITH_ASSERTREL(pWorld->aModels == NULL);
        return;
    }

#line 255
    SITH_ASSERTREL(pWorld->aModels != NULL);
    for ( i = 0; i < pWorld->numModels; ++i )
    {
        sithModel_CacheRemove(&pWorld->aModels[i]);
        rdModel3_FreeEntryGeometryOnly(&pWorld->aModels[i]);
    }

    stdMemory_Free(pWorld->aModels);
    pWorld->aModels    = NULL;
    pWorld->numModels  = 0;
    pWorld->sizeModels = 0;
}

rdModel3* J3DAPI sithModel_Load(const char* pName, int bSkipDefault)
{
    rdModel3* pModel;
    char aPath[128];
    SithWorld* pWorld = sithWorld_pLastLoadedWorld;

#line 308
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(strlen(pName) < 128);

    STD_MAKEPATH(aPath, "3do", pName);
    stdFnames_ChangeExt(aPath, "3do");

#line 327
    SITH_ASSERTREL(pWorld->aModels != NULL);

    pModel = sithModel_CacheFind(pName);
    if ( pModel )
    {
        return pModel;
    }

#line 335
    SITH_ASSERTREL(pWorld->numModels <= pWorld->sizeModels);
    if ( pWorld->numModels >= pWorld->sizeModels )
    {
#line 340
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to load new model '%s'.\n", pName, 0, 0, 0);
        return NULL;
    }

    pModel = &pWorld->aModels[pWorld->numModels];
    if ( rdModel3_LoadEntry(aPath, pModel) )
    {
        pModel->num = pWorld->numModels;
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pModel->num = SITHWORLD_STATICINDEX(pModel->num);
        }

        sithModel_CacheAdd(pModel);
        pWorld->numModels++;
        return pModel;
    }

    if ( !bSkipDefault )
    {
        pModel = sithModel_Load(SITHMODEL_DEFAULTMODEL, 1);
        if ( pModel )
        {
#line 365
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Model %s not found, using default.\n", pName, 0, 0, 0);
            return pModel;
        }

#line 370
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Model %s not found, no default loaded.\n", pName, 0, 0, 0);
        return NULL;
    }

    return NULL;
}

size_t J3DAPI sithModel_GetModelMemUsage(const rdModel3* pModel)
{
    size_t i;
    size_t j;
    size_t k;
    size_t memUsage = 0;
    memUsage += sizeof(rdModel3);
    memUsage += sizeof(rdMaterial*) * pModel->numMaterials;
    memUsage += sizeof(rdModel3HNode) * pModel->numHNodes;
    for ( i = 0; i < pModel->numGeos; ++i )
    {
        memUsage += sizeof(rdModel3GeoSet);
        for ( j = 0; j < pModel->aGeos[i].numMeshes; ++j )
        {
            memUsage += sizeof(rdModel3Mesh);
            memUsage += 32 * pModel->aGeos[i].aMeshes[j].numVertices;
            memUsage += sizeof(rdVector2) * pModel->aGeos[i].aMeshes[j].numTexVertices;
            memUsage += sizeof(rdFace) * pModel->aGeos[i].aMeshes[j].numFaces;

            for ( k = 0; k < pModel->aGeos[i].aMeshes[j].numFaces; ++k )
            {
                memUsage += 8 * pModel->aGeos[i].aMeshes[j].aFaces[k].numVertices;
            }
        }
    }

    return memUsage;
}

int J3DAPI sithModel_AllocWorldModels(SithWorld* pWorld, size_t size)
{
#line 450
    SITH_ASSERTREL(pWorld->aModels == NULL);

#line 453
    pWorld->aModels = (rdModel3*)STDMALLOC(sizeof(rdModel3) * size);
    if ( !pWorld->aModels )
    {
#line 457
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate models array.\n", 0, 0, 0, 0);
        return 1;
    }

    pWorld->sizeModels = size;
    pWorld->numModels  = 0;
    memset(pWorld->aModels, 0, sizeof(rdModel3) * pWorld->sizeModels);
    return 0;
}

rdModel3* J3DAPI sithModel_GetModelByIndex(int16_t modelIdx)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(modelIdx) )
    {
        pWorld   = sithWorld_pStaticWorld;
        modelIdx &= ~SITH_STATICRESOURCE_INDEXMASK;
    }

    if ( !pWorld || modelIdx < 0 || modelIdx >= (int)pWorld->numModels )
    {
        return NULL;
    }

    return &pWorld->aModels[modelIdx];
}

int16_t J3DAPI sithModel_GetModelIndex(const rdModel3* pModel)
{
    int index;
    if ( pModel >= sithWorld_pCurrentWorld->aModels && pModel < &sithWorld_pCurrentWorld->aModels[sithWorld_pCurrentWorld->numModels] )
    {
        index = pModel - sithWorld_pCurrentWorld->aModels;
    }
    else if ( pModel >= sithWorld_pStaticWorld->aModels && pModel < &sithWorld_pStaticWorld->aModels[sithWorld_pStaticWorld->numModels] )
    {
        index = pModel - sithWorld_pStaticWorld->aModels;
        index |= SITH_STATICRESOURCE_INDEXMASK;
    }
    else
    {
        index = -1;
    }

    return index;
}

rdModel3* J3DAPI sithModel_GetModel(const char* pName)
{
    SithWorld* pWorld;
    size_t worldNum;
    size_t i;

    pWorld = sithWorld_pCurrentWorld;
    if ( !pName )
    {
        return NULL;
    }

    for ( worldNum = 0; worldNum < 2; worldNum++ )
    {
        if ( worldNum == 1 )
        {
            pWorld = sithWorld_pStaticWorld;
        }

        if ( !pWorld )
        {
            continue;
        }

        for ( i = 0; i < pWorld->numModels; ++i )
        {
            if ( !strcmpi(pName, pWorld->aModels[i].aName) )
            {
                return &pWorld->aModels[i];
            }
        }
    }

    return NULL;
}

int J3DAPI sithModel_GetMeshIndex(const rdModel3* pModel, const char* pMeshName)
{
    size_t i;
    int bFound = 0;

    if ( !pModel || !pMeshName )
    {
        return -1;
    }

    for ( i = 0; i < pModel->aGeos[0].numMeshes; ++i )
    {
        if ( !strcmp(pMeshName, pModel->aGeos[0].aMeshes[i].name) )
        {
            bFound = 1;
            break;
        }
    }

    if ( bFound )
    {
        return i;
    }

    return -1;
}

void J3DAPI sithModel_EnableHiPoly(int bEnabled)
{
    sithModel_bHiPoly = bEnabled;
}

/* Linker-discarded (no caller in either build): debug 0x0045F016, right after EnableHiPoly. */
int J3DAPI sithModel_IsHiPolyEnabled(void)
{
    return sithModel_bHiPoly;
}

void J3DAPI sithModel_CacheRemove(const rdModel3* pModel)
{
#line 592
    SITH_ASSERTREL(pModel != NULL);
    SITH_ASSERTREL(strlen(pModel->aName) > 0);
    SITH_ASSERTREL(sithModel_pHashtable != NULL);
    stdHashtbl_Remove(sithModel_pHashtable, pModel->aName);
}

void J3DAPI sithModel_CacheAdd(rdModel3* pModel)
{
#line 601
    SITH_ASSERTREL(pModel != NULL);
    SITH_ASSERTREL(strlen(pModel->aName) > 0);
    SITH_ASSERTREL(sithModel_pHashtable != NULL);
    stdHashtbl_Add(sithModel_pHashtable, pModel->aName, pModel);
}

rdModel3* J3DAPI sithModel_CacheFind(const char* pName)
{
#line 610
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithModel_pHashtable != NULL);
    return stdHashtbl_Find(sithModel_pHashtable, pName);
}
