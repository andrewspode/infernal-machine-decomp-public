#include "sithSprite.h"

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Primitives/rdSprite.h>

#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Retail spelling recovered from the assertion string "sithSprite_pHashtable != ((void *)0)"
static tHashTable* sithSprite_pHashtable;

rdSprite3* J3DAPI sithSprite_CacheFind(const char* pName);
void J3DAPI sithSprite_CacheAdd(rdSprite3* pSprite);
void J3DAPI sithSprite_CacheRemove(const rdSprite3* pSprite);

int J3DAPI sithSprite_Startup(void)
{
    sithSprite_pHashtable = stdHashtbl_New(128u);
    if ( !sithSprite_pHashtable )
    {
#line 62
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate memory for sprites.\n", 0, 0, 0, 0);
        return 1;
    }

    return 0;
}

void J3DAPI sithSprite_Shutdown(void)
{
    if ( sithSprite_pHashtable )
    {
        stdHashtbl_Free(sithSprite_pHashtable);
    }

    sithSprite_pHashtable = NULL;
}

void J3DAPI sithSprite_Update(SithThing* pThing, uint32_t secDeltaTime)
{
    SithThing* pThingMeshAttached;
    int meshNumber;
    int skipBuildingJoints;

#line 88
    SITH_ASSERTREL(pThing->renderData.data.pSprite3);

    pThingMeshAttached = pThing->thingInfo.spriteInfo.pThingMeshAttached;
    if ( pThingMeshAttached && pThingMeshAttached->type )
    {
        rdMatrix34 meshOrient;
        meshNumber = pThing->thingInfo.spriteInfo.attachMeshNum;
        skipBuildingJoints = pThingMeshAttached->renderData.bSkipBuildingJoints;
        pThingMeshAttached->renderData.bSkipBuildingJoints = 1;
        rdModel3_GetMeshMatrix(&pThingMeshAttached->renderData, &pThingMeshAttached->orient, meshNumber, &meshOrient);
        pThingMeshAttached->renderData.bSkipBuildingJoints = skipBuildingJoints;

        rdVector_Copy3(&pThing->pos, &meshOrient.dvec);
        rdMatrix_Copy34(&pThing->orient, &pThingMeshAttached->orient);
        sithThing_SetSector(pThing, pThingMeshAttached->pInSector, 1);
    }
}

int J3DAPI sithSprite_Draw(SithThing* pThing)
{
    rdSprite3* pSprite3;

    if ( !pThing->renderData.data.pSprite3 )
    {
#line 116
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No rdsprite data in sithSprite_Draw().\n", 0, 0, 0, 0);
        return 0;
    }

    if ( (pThing->flags & SITH_TF_AIRDESTROYED) != 0 && pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        sithThing_DestroyThing(pThing);
        return 0;
    }

    pSprite3 = pThing->renderData.data.pSprite3;
    pSprite3->width                 = pThing->thingInfo.spriteInfo.width;
    pSprite3->height                = pThing->thingInfo.spriteInfo.height;
    pSprite3->widthHalf             = pSprite3->width / 2.0f;
    pSprite3->heightHalf            = pSprite3->height / 2.0f;
    pSprite3->face.extraLight.alpha = pThing->thingInfo.spriteInfo.alpha;
    pSprite3->rollAngle             = pThing->thingInfo.spriteInfo.rollAngle;

    return rdThing_Draw(&pThing->renderData, &pThing->orient);
}

void J3DAPI sithSprite_Create(SithThing* pThing)
{
    if ( !pThing->renderData.data.pSprite3 )
    {
#line 144
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No rdsprite data in sithSprite_Draw().\n", 0, 0, 0, 0);
        return;
    }

    pThing->thingInfo.spriteInfo.width  = pThing->renderData.data.pSprite3->width;
    pThing->thingInfo.spriteInfo.height = pThing->renderData.data.pSprite3->height;
    pThing->thingInfo.spriteInfo.alpha  = pThing->renderData.data.pSprite3->face.extraLight.alpha;
}

/* Linker-discarded: retail __LINE__ gap 144..198 (sithSprite_Create -> sithSprite_ReadStaticSpritesListText), where it
   was defined from the debug build. Body from the debug code (0x004B627B): every failing write jumps to one
   `return 1` laid after the `return 0`. */
int J3DAPI sithSprite_WriteStaticSpritesListText(const SithWorld* pWorld)
{
    int i;

    if ( stdConffile_WriteLine("###### Sprite information ######\n") ) {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: SPRITES\n\n") ) {
        goto error;
    }

    if ( stdConffile_Printf("World sprites %d\n\n", pWorld->numSprites) ) {
        goto error;
    }

    for ( i = 0; i < (int)pWorld->numSprites; ++i )
    {
        if ( stdConffile_Printf("%d: %s\n", i, pWorld->aSprites[i].aName) ) {
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

int J3DAPI sithSprite_ReadStaticSpritesListText(SithWorld* pWorld, int bSkip)
{
    int staticSpritesCount;
    float progressDelta;
    float progress;

    if ( bSkip )
    {
        return 1;
    }

#line 198
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aSprites == NULL);
    SITH_ASSERTREL(pWorld->numSprites == 0);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "world") || strcmp(stdConffile_entry.aArgs[1].argValue, "sprites") )
    {
#line 206
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading static sprites list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    staticSpritesCount = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !staticSpritesCount )
    {
        return 0;
    }

    if ( sithSprite_AllocWorldSprites(pWorld, staticSpritesCount) )
    {
        goto alloc_error;
    }

    sithWorld_UpdateLoadProgress(70.0f);
    progressDelta = SITHWORLD_PROGRESSSTEP(10.0f, staticSpritesCount);
    progress      = 70.0f;
    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        if ( !sithSprite_Load(pWorld, stdConffile_entry.aArgs[1].argValue) )
        {
            goto parse_error;
        }

        progress += progressDelta;
        sithWorld_UpdateLoadProgress(progress);
    }

    sithWorld_UpdateLoadProgress(80.0f);
#line 244
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Sprites read.  Sprite array space %d, used %d.\n", pWorld->sizeSprites, pWorld->numSprites, 0, 0);
#line 246
    SITH_ASSERTREL(pWorld->numSprites <= pWorld->sizeSprites);
    return 0;

parse_error:
#line 251
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error while reading sprites, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;

alloc_error:
#line 255
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory error while reading sprites, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;
}

int J3DAPI sithSprite_WriteStaticSpritesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    size_t written;
    for ( i = 0; i < pWorld->numSprites; ++i )
    {
        written = sith_pHS->pFileWrite(fh, pWorld->aSprites[i].aName, STD_ARRAYLEN(pWorld->aSprites[i].aName));
        if ( written != STD_ARRAYLEN(pWorld->aSprites[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithSprite_ReadStaticSpritesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    char aName[64];
    size_t nRead;
    size_t numCurrentSprites = pWorld->numSprites;

    if ( sithSprite_AllocWorldSprites(pWorld, pWorld->numSprites) )
    {
        goto error;
    }

    for ( i = 0; i < numCurrentSprites; ++i )
    {
        nRead = sith_pHS->pFileRead(fh, aName, STD_ARRAYLEN(aName));
        if ( nRead != STD_ARRAYLEN(aName) )
        {
            goto error;
        }

        if ( !sithSprite_Load(pWorld, aName) )
        {
            goto error;
        }
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithSprite_FreeWorldSprites(SithWorld* pWorld)
{
    size_t i;

#line 312
    SITH_ASSERTREL(pWorld != NULL);

    if ( !pWorld->sizeSprites )
    {
#line 316
        SITH_ASSERTREL(pWorld->aSprites == NULL);
        return;
    }

#line 320
    SITH_ASSERTREL(pWorld->aSprites != NULL);
    for ( i = 0; i < pWorld->numSprites; ++i )
    {
        sithSprite_CacheRemove(&pWorld->aSprites[i]);
        rdSprite_FreeEntry(&pWorld->aSprites[i]);
    }

    stdMemory_Free(pWorld->aSprites);
    pWorld->aSprites    = NULL;
    pWorld->numSprites  = 0;
    pWorld->sizeSprites = 0;
}

rdSprite3* J3DAPI sithSprite_Load(SithWorld* pWorld, const char* pName)
{
    rdSprite3* pSprite3;
    char aPath[128];
    char aMatFilename[64];
    unsigned int type;
    float width;
    float height;
    rdGeometryMode geoMode;
    rdLightMode lightMode;
    rdVector4 extraLight;
    rdVector3 offset;

#line 378
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aSprites != NULL);

    pSprite3 = sithSprite_CacheFind(pName);
    if ( pSprite3 )
    {
        return pSprite3;
    }

#line 387
    SITH_ASSERTREL(pWorld->numSprites <= pWorld->sizeSprites);
    if ( pWorld->numSprites >= pWorld->sizeSprites )
    {
#line 392
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to define new sprite '%s'.\n", pName, 0, 0, 0);
        return NULL;
    }

    pSprite3 = &pWorld->aSprites[pWorld->numSprites];
    sprintf(aPath, "%s%c%s", "misc\\spr", '\\', pName);
    if ( !stdConffile_Open(aPath) )
    {
        if ( strcmp(pName, "default.spr") )
        {
            pSprite3 = sithSprite_Load(pWorld, "default.spr");
            if ( pSprite3 )
            {
#line 409
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sprite %s not found, using default.\n", pName, 0, 0, 0);
                return pSprite3;
            }

#line 414
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sprite %s not found, no default found.\n", pName, 0, 0, 0);
            return NULL;
        }

        return NULL;
    }

    if ( !stdConffile_ReadArgs() || stdConffile_entry.numArgs != 14 )
    {
#line 426
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad data in sprite file %s.\n", pName, 0, 0, 0);
        stdConffile_Close();
        return NULL;
    }

    strncpy(aMatFilename, stdConffile_entry.aArgs[0].argValue, 63);
    aMatFilename[63] = 0;

    type   = atoi(stdConffile_entry.aArgs[1].argValue);
    width  = D3DVAL(atof(stdConffile_entry.aArgs[2].argValue));
    height = D3DVAL(atof(stdConffile_entry.aArgs[3].argValue));

    geoMode   = atoi(stdConffile_entry.aArgs[4].argValue);
    lightMode = atoi(stdConffile_entry.aArgs[5].argValue);
    atoi(stdConffile_entry.aArgs[6].argValue); // Texture mode, unused

    extraLight.red   = D3DVAL(atof(stdConffile_entry.aArgs[7].argValue));
    extraLight.green = D3DVAL(atof(stdConffile_entry.aArgs[8].argValue));
    extraLight.blue  = D3DVAL(atof(stdConffile_entry.aArgs[9].argValue));
    extraLight.alpha = D3DVAL(atof(stdConffile_entry.aArgs[10].argValue));

    offset.x = D3DVAL(atof(stdConffile_entry.aArgs[11].argValue));
    offset.y = D3DVAL(atof(stdConffile_entry.aArgs[12].argValue));
    offset.z = D3DVAL(atof(stdConffile_entry.aArgs[13].argValue));

    stdConffile_Close();

    if ( type > 2 || width <= 0.0f || height <= 0.0f )
    {
#line 453
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad sprite params.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( rdSprite_NewEntry(pSprite3, pName, type, aMatFilename, width, height, geoMode, lightMode, &extraLight, &offset) )
    {
        sithSprite_CacheAdd(pSprite3);
        pWorld->numSprites++;
        return pSprite3;
    }

#line 466
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to load sprite '%s'.\n", pName, 0, 0, 0);
    return NULL;
}

int J3DAPI sithSprite_AllocWorldSprites(SithWorld* pWorld, size_t size)
{
#line 491
    SITH_ASSERTREL(pWorld->aSprites == NULL);

#line 494
    pWorld->aSprites = (rdSprite3*)STDMALLOC(sizeof(rdSprite3) * size);
    if ( !pWorld->aSprites )
    {
#line 498
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocate sprites array.\n", 0, 0, 0, 0);
        return 1;
    }

    pWorld->sizeSprites = size;
    pWorld->numSprites  = 0;
    memset(pWorld->aSprites, 0, sizeof(rdSprite3) * pWorld->sizeSprites);
    return 0;
}

void J3DAPI sithSprite_Initialize(SithThing* pThing)
{
    rdVector3 pyr;
    rdMatrix_ExtractAngles34(&pThing->orient, &pyr);
    if ( pyr.roll == 0.0f )
    {
        return;
    }

    if ( pyr.roll < 0.0f )
    {
        pyr.roll = pyr.roll + 360.0f;
    }

    pThing->thingInfo.spriteInfo.rollAngle = pyr.roll;
}

rdSprite3* J3DAPI sithSprite_CacheFind(const char* pName)
{
#line 535
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithSprite_pHashtable != NULL);

    return (rdSprite3*)stdHashtbl_Find(sithSprite_pHashtable, pName);
}

void J3DAPI sithSprite_CacheAdd(rdSprite3* pSprite)
{
#line 544
    SITH_ASSERTREL(pSprite != NULL);
    SITH_ASSERTREL(strlen(pSprite->aName) > 0);
    SITH_ASSERTREL(sithSprite_pHashtable != NULL);

    stdHashtbl_Add(sithSprite_pHashtable, pSprite->aName, pSprite);
}

void J3DAPI sithSprite_CacheRemove(const rdSprite3* pSprite)
{
#line 555
    SITH_ASSERTREL(pSprite != NULL);
    SITH_ASSERTREL(strlen(pSprite->aName) > 0);
    SITH_ASSERTREL(sithSprite_pHashtable != NULL);

    stdHashtbl_Remove(sithSprite_pHashtable, pSprite->aName);
}
