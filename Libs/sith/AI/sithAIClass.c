#include "sithAIClass.h"

#include <sith/AI/sithAI.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

// Retail spelling recovered from the assertion string "sithAIClass_pHashtable != ((void *)0)"
static tHashTable* sithAIClass_pHashtable = NULL;

SithAIClass* J3DAPI sithAIClass_CacheFind(const char* pName);
void J3DAPI sithAIClass_CacheAdd(SithAIClass* pClass);
void J3DAPI sithAIClass_CacheRemove(const SithAIClass* pClass);

int J3DAPI sithAIClass_Startup(void)
{
    sithAIClass_pHashtable = stdHashtbl_New(64u);
    if ( !sithAIClass_pHashtable )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithAIClass_Shutdown(void)
{
    if ( sithAIClass_pHashtable )
    {
        stdHashtbl_Free(sithAIClass_pHashtable);
    }

    sithAIClass_pHashtable = NULL;
}

/* Discarded by the retail linker (no caller in either image); restored in full from the debug build
   (0x0046BBCC, between Shutdown and ReadStaticAIClassesListText). Every failed write jumps to one
   `return 1` after the success return, from inside the loop too (debug jmp
   0x0046BC96). Name inferred. */
int J3DAPI sithAIClass_WriteStaticAIClassesListText(const SithWorld* pWorld)
{
    size_t i;

    if ( stdConffile_WriteLine("######### AI Classes ###########\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: AICLASS\n\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World AIClasses %d\n", pWorld->numAIClasses) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numAIClasses; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aAIClasses[i].aName) )
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

int J3DAPI sithAIClass_ReadStaticAIClassesListText(SithWorld* pWorld, int bSkip)
{
    size_t numClasses;

    if ( bSkip )
    {
        return 1;
    }

#line 152
    SITH_ASSERTREL(pWorld != NULL);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "world") || strcmp(stdConffile_entry.aArgs[1].argValue, "aiclasses") )
    {
#line 158
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading static aiclasses list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    numClasses = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !numClasses )
    {
        return 0;
    }

    if ( sithAIClass_AllocWorldAIClasses(pWorld, numClasses) )
    {
        goto alloc_error;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        if ( !sithAIClass_Load(pWorld, stdConffile_entry.aArgs[1].argValue) )
        {
            goto parse_error;
        }
    }

#line 183
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "AI classes read.  AI array space %d, used %d.\n", pWorld->sizeAIClasses, pWorld->numAIClasses, 0, 0);
#line 185
    SITH_ASSERTREL(pWorld->numAIClasses <= pWorld->sizeAIClasses);
    return 0;

parse_error:
#line 190
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error while reading aiclasses, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;

alloc_error:
#line 194
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory error while reading aiclasses, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;
}

int J3DAPI sithAIClass_WriteStaticAIClassesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    size_t written;
    for ( i = 0; i < pWorld->numAIClasses; ++i )
    {
        written = sith_pHS->pFileWrite(fh, pWorld->aAIClasses[i].aName, STD_ARRAYLEN(pWorld->aAIClasses[i].aName));
        if ( written != STD_ARRAYLEN(pWorld->aAIClasses[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithAIClass_ReadStaticAIClassesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    char aName[64];
    size_t nRead;
    size_t nAIClasses = pWorld->numAIClasses;

    if ( sithAIClass_AllocWorldAIClasses(pWorld, pWorld->sizeAIClasses) )
    {
        goto error;
    }

    for ( i = 0; i < nAIClasses; ++i )
    {
        nRead = sith_pHS->pFileRead(fh, aName, STD_ARRAYLEN(aName));
        if ( nRead != STD_ARRAYLEN(aName) )
        {
            goto error;
        }

        if ( !sithAIClass_Load(pWorld, aName) )
        {
            goto error;
        }
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithAIClass_AllocWorldAIClasses(SithWorld* pWorld, size_t numClasses)
{
#line 253
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(pWorld->aAIClasses == NULL);

#line 257
    pWorld->aAIClasses = (SithAIClass*)STDMALLOC(sizeof(SithAIClass) * numClasses);
    if ( !pWorld->aAIClasses )
    {
        goto alloc_error;
    }

    memset(pWorld->aAIClasses, 0, sizeof(SithAIClass) * numClasses);
    pWorld->numAIClasses  = 0;
    pWorld->sizeAIClasses = numClasses;
    return 0;

alloc_error:
    sithAIClass_FreeWorldAIClasses(pWorld);
#line 271
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory allocation failure.\n", 0, 0, 0, 0);
    return 1;
}

void J3DAPI sithAIClass_FreeWorldAIClasses(SithWorld* pWorld)
{
    size_t i;

#line 294
    SITH_ASSERTREL(pWorld);
    if ( pWorld->aAIClasses )
    {
        for ( i = 0; i < pWorld->numAIClasses; ++i )
        {
            sithAIClass_CacheRemove(&pWorld->aAIClasses[i]);
        }

        stdMemory_Free(pWorld->aAIClasses);
        pWorld->aAIClasses = NULL;
    }

    pWorld->sizeAIClasses = 0;
    pWorld->numAIClasses  = 0;
}

SithAIClass* J3DAPI sithAIClass_Load(SithWorld* pWorld, const char* pName)
{
    SithAIClass* pClass;
    char aFilePath[128];

#line 335
    SITH_ASSERTREL(pWorld && pName);
    SITH_ASSERTREL(strlen(pName) < 64);

    if ( !pWorld->aAIClasses )
    {
#line 340
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to load AI class %s.\n", pName, 0, 0, 0);
        return NULL;
    }

    pClass = sithAIClass_CacheFind(pName);
    if ( pClass )
    {
        return pClass;
    }

    sprintf(aFilePath, "%s%c%s", "misc\\ai", '\\', pName);

    if ( pWorld->numAIClasses >= pWorld->sizeAIClasses )
    {
#line 353
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to load new AI class %s.\n", pName, 0, 0, 0);
        return NULL;
    }

    pClass = &pWorld->aAIClasses[pWorld->numAIClasses];
    memset(pClass, 0, sizeof(SithAIClass));

    strncpy(pClass->aName, pName, STD_ARRAYLEN(pClass->aName) - 1);
    pClass->aName[STD_ARRAYLEN(pClass->aName) - 1] = 0;

    if ( sithAIClass_LoadEntry(aFilePath, pClass) )
    {
        sithAIClass_CacheAdd(pClass);
        pClass->num = pWorld->numAIClasses++;
        return pClass;
    }

    if ( strcmp(pName, "default.ai") )
    {
        pClass = sithAIClass_Load(pWorld, "default.ai");
        if ( pClass )
        {
#line 379
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AI Class %s not found, using %s.\n", pName, "default.ai", 0, 0);
            return pClass;
        }

#line 384
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AI Class %s not found, no default found.\n", pName, 0, 0, 0);
        return pClass;
    }

    return NULL;
}

int J3DAPI sithAIClass_LoadEntry(const char* pPath, SithAIClass* pClass)
{
    size_t i;

#line 416
    SITH_ASSERTREL(pPath && pClass);
    if ( !stdConffile_Open(pPath) )
    {
        return 0;
    }

    pClass->maxStep       = 0.5f;
    pClass->sightDistance = 20.0f;
    pClass->heardDistance = 10.0f;
    pClass->fov           = 0.0f;
    pClass->accurancy     = 0.5f;
    pClass->armedMode     = 0;
    pClass->degTurnAlign  = 45.0f;

    if ( !stdConffile_ReadArgs() )
    {
        goto syntax_error;
    }

    for ( i = 0; i < stdConffile_entry.numArgs; ++i )
    {
        if ( !strcmp(stdConffile_entry.aArgs[i].argName, "alignment") )
        {
            pClass->alignment = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "rank") )
        {
            pClass->rank = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "maxstep") )
        {
            pClass->maxStep = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "sightdist") )
        {
            pClass->sightDistance = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "heardist") )
        {
            pClass->heardDistance = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "wakeupdist") )
        {
            pClass->weakupDistance = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "fov") )
        {
            float sinFov;
            float cosFov;
            stdMath_SinCos((float)atof(stdConffile_entry.aArgs[i].argValue) * 0.5f, &sinFov, &cosFov);
            pClass->fov = cosFov;
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "accuracy") )
        {
            pClass->accurancy = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "armedmode") )
        {
            pClass->armedMode = atoi(stdConffile_entry.aArgs[i].argValue);
            if ( !(pClass->armedMode >= 0 && pClass->armedMode < SITH_PUPPET_NUMARMEDMODES) )
            {
                pClass->armedMode = 0;
#line 480
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Illegal armed mode in class %s.  Setting to unarmed.\n", pClass->aName, 0, 0, 0);
            }
        }
        else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "degturnalign") )
        {
            pClass->degTurnAlign = D3DVAL(atof(stdConffile_entry.aArgs[i].argValue));
        }
    }

    while ( stdConffile_ReadArgs() )
    {
        SithAIInstinct* pInstinct;
        SithAIRegisteredInstinct* regInstinct;

        pInstinct = &pClass->aInstincts[pClass->numInstincts];
        if ( pClass->numInstincts >= STD_ARRAYLEN(pClass->aInstincts) )
        {
#line 499
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many instincts in class %s (max %d)\n", pClass->aName, STD_ARRAYLEN(pClass->aInstincts), 0, 0);
            continue;
        }

        regInstinct = sithAI_FindInstinct(stdConffile_entry.aArgs[0].argValue);
        if ( !regInstinct )
        {
#line 506
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not find registered instinct %s for class %s, not added.\n", stdConffile_entry.aArgs[0].argValue, pClass->aName, 0, 0);
            continue;
        }

        pInstinct->pfInstinct       = regInstinct->pfInstinct;
        pInstinct->updateModes      = regInstinct->updateModes;
        pInstinct->updateBlockModes = regInstinct->updateBlockModes;
        pInstinct->triggerEvents    = regInstinct->triggerEvents;

        for ( i = 0; i < STD_ARRAYLEN(pInstinct->fltArg); ++i )
        {
            if ( stdConffile_entry.numArgs > i + 1 )
            {
                pInstinct->fltArg[i] = D3DVAL(atof(stdConffile_entry.aArgs[i + 1].argValue));
                pInstinct->intArg[i] = lrintf(pInstinct->fltArg[i]);
            }
            else
            {
                pInstinct->fltArg[i] = 0.0f;
                pInstinct->intArg[i] = 0;
            }
        }

        ++pClass->numInstincts;
    }

    stdConffile_Close();
    return 1;

syntax_error:
#line 538
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Syntax error reading AI info.  File %s line %d.\n", stdConffile_GetFilename(), stdConffile_GetLineNumber(), 0, 0);
    stdConffile_Close();
    return 0;
}

SithAIClass* J3DAPI sithAIClass_CacheFind(const char* pName)
{
#line 549
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithAIClass_pHashtable != NULL);
    return (SithAIClass*)stdHashtbl_Find(sithAIClass_pHashtable, pName);
}

void J3DAPI sithAIClass_CacheAdd(SithAIClass* pClass)
{
#line 557
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);
    SITH_ASSERTREL(sithAIClass_pHashtable != NULL);
    stdHashtbl_Add(sithAIClass_pHashtable, pClass->aName, pClass);
}

void J3DAPI sithAIClass_CacheRemove(const SithAIClass* pClass)
{
#line 566
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);
    SITH_ASSERTREL(sithAIClass_pHashtable != NULL);
    stdHashtbl_Remove(sithAIClass_pHashtable, pClass->aName);
}
