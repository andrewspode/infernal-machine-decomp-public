#include "sithTemplate.h"
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <rdroid/Engine/rdThing.h>

#ifdef J3D_DEBUG
#include <sith/World/sithWrite.h>
#endif

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithTemplate_countfit_1, 15)
CF_PROTO(sithTemplate_countfit_2, 15)
CF_PROTO(sithTemplate_countfit_3, 15)
CF_PROTO(sithTemplate_countfit_4, 15)
CF_PROTO(sithTemplate_countfit_5, 15)
CF_PROTO(sithTemplate_countfit_6, 15)
CF_PROTO(sithTemplate_countfit_7, 15)
CF_PROTO(sithTemplate_countfit_8, 15)
CF_PROTO(sithTemplate_countfit_9, 15)
CF_PROTO(sithTemplate_countfit_10, 15)
CF_PROTO(sithTemplate_countfit_11, 15)
CF_PROTO(sithTemplate_countfit_12, 15)
CF_PROTO(sithTemplate_countfit_13, 15)
CF_PROTO(sithTemplate_countfit_14, 15)
CF_PROTO(sithTemplate_countfit_15, 15)
CF_PROTO(sithTemplate_countfit_16, 15)
CF_PROTO(sithTemplate_countfit_17, 15)
CF_PROTO(sithTemplate_countfit_18, 15)
CF_PROTO(sithTemplate_countfit_19, 15)
CF_PROTO(sithTemplate_countfit_20, 15)
CF_PROTO(sithTemplate_countfit_21, 15)
CF_PROTO(sithTemplate_countfit_22, 3)
/* end count-fit */

#define SITHTEMPLAT_MAXCACHEDTEMPLATES 1024u
#define SITHTEMPLATE_MAXMASTERTEMPLATES 3072u

typedef struct sSithTemplate
{
    const char* pName;
    const char* pDescription;
    const char* pText;
    const char* pBasedOn;
} SithTemplate;

// Master-file state. The four statics tile retail's .bss 0x0056F9BC-0x0056F9CC in Jedi Knight's
// order (OpenJKDF2 symbols.syms: pMasterFile, pHashtable, masterFileCount, pMasterHashtable at
// 0x008BBC60..0x008BBC6C); the names are JK's. pMasterFile (0x0056F9BC) has no
// surviving reference: only the linker-discarded master-file loader used it. GetTemplate reads
// masterFileCount (0x0056F9C4) and pMasterHashtable (0x0056F9C8); no surviving code writes them.
static SithTemplate* sithTemplate_pMasterFile = NULL;
static tHashTable* sithTemplate_pHashtable = NULL;
static size_t sithTemplate_masterFileCount = 0;
static tHashTable* sithTemplate_pMasterHashtable = NULL;

void J3DAPI sithTemplate_AddTemplate(SithThing* pTemplate);
int J3DAPI sithTemplate_MasterFileAddDescription(const char* pDesc);
SithThing* J3DAPI sithTemplate_Parse(SithWorld* pWorld);
int J3DAPI sithTemplate_MasterFileAddTemplate(const char* pName, const char* pDesc, const char* pText, const char* pBasedOn);
SithThing* J3DAPI sithTemplate_CacheFind(const char* pName);
void J3DAPI sithTemplate_CacheAdd(SithThing* pThing);
void J3DAPI sithTemplate_CacheRemove(const SithThing* pThing);
int J3DAPI sithTemplate_ParseTemplateList(SithWorld* pWorld);

int J3DAPI sithTemplate_Startup(void)
{
    sithTemplate_pHashtable = stdHashtbl_New(SITHTEMPLAT_MAXCACHEDTEMPLATES);
    if ( !sithTemplate_pHashtable )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithTemplate_Shutdown(void)
{
    if ( sithTemplate_pHashtable )
    {
        stdHashtbl_Free(sithTemplate_pHashtable);
    }

    sithTemplate_pHashtable = NULL;
}

int J3DAPI sithTemplate_AllocWorldTemplates(SithWorld* pWorld, size_t size)
{
    size_t i;

#line 105
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aThingTemplates == NULL);
    SITH_ASSERTREL(sithTemplate_pHashtable);

    if ( !size )
    {
        return 0;
    }

#line 111
    pWorld->aThingTemplates = (SithThing*)STDMALLOC(sizeof(SithThing) * size);
    if ( !pWorld->aThingTemplates )
    {
        return 1;
    }

    memset(pWorld->aThingTemplates, 0, sizeof(SithThing) * size);

    for ( i = 0; i < size; ++i )
    {
        sithThing_Reset(&pWorld->aThingTemplates[i]);
        if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
        {
            pWorld->aThingTemplates[i].idx = SITHWORLD_STATICINDEX(i);
        }
        else
        {
            pWorld->aThingTemplates[i].idx = i;
        }
    }

    pWorld->sizeThingTemplates = size;
    pWorld->numThingTemplates  = 0;
    return 0;
}

SithThing* J3DAPI sithTemplate_GetTemplateByIndex(int idx)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(idx) )
    {
        pWorld = sithWorld_pStaticWorld;
        idx = SITHWORLD_FROM_STATICINDEX(idx);
    }

    if ( !pWorld )
    {
        return NULL;
    }

    if ( idx < 0 || idx >= (int)pWorld->numThingTemplates )
    {
        return NULL;
    }

    return &pWorld->aThingTemplates[idx];
}

int J3DAPI sithTemplate_GetTemplateIndex(const SithThing* pTemplate)
{
    if ( !pTemplate )
    {
        return -1;
    }

    if ( pTemplate < sithWorld_pCurrentWorld->aThingTemplates
        || pTemplate >= &sithWorld_pCurrentWorld->aThingTemplates[sithWorld_pCurrentWorld->numThingTemplates] )
    {
        return -1;
    }

    if ( !pTemplate->type )
    {
        return -1;
    }

    return pTemplate->idx;
}

#ifdef J3D_DEBUG
// Debug-only text writer (debug 0x473929), called only by the debug build's sithWrite_Write; each template's
// parameters are written by sithWrite.c, which retail has not got.
#line 175
int J3DAPI sithTemplate_WriteThingTemplatesListText(const SithWorld* pWorld)
{
    const SithThing* pTemplate;
    const SithThing* pBasedOn;
    SithThing baseThing;
    size_t numTemplates;
    size_t i;

    baseThing.idx = 0;
    baseThing.signature = 0;
    sithThing_Reset(&baseThing);

    if ( stdConffile_WriteLine("##### Templates information ####\n") ) goto error;
    if ( stdConffile_WriteLine("Section: TEMPLATES\n") ) goto error;
    if ( stdConffile_Printf("\nWorld templates %d\n\n", pWorld->numThingTemplates) ) goto error;
    if ( stdConffile_WriteLine("#Name:           Based On:        Params:\n") ) goto error;

    numTemplates = pWorld->numThingTemplates;
    pTemplate = pWorld->aThingTemplates;
    for ( i = 0; i < numTemplates; ++pTemplate, ++i )
    {
        pBasedOn = pTemplate->pTemplate;
        SITH_ASSERT((pTemplate->type != SITH_THING_FREE) && (pTemplate->type < SITH_THING_NUMTYPES));

        if ( stdConffile_Printf("%-16s %-16s ", pTemplate->aName, pBasedOn ? pBasedOn->aName : "none") ) goto error;

        if ( !pBasedOn )
        {
            pBasedOn = &baseThing;
        }

        if ( sithWrite_WriteThingArgs(pTemplate, pBasedOn, 1) ) goto error;
        if ( stdConffile_WriteLine("\n") ) goto error;
    }

    if ( stdConffile_WriteLine("end\n") ) goto error;
    if ( stdConffile_WriteLine("################################\n\n\n") ) goto error;

    return 0;

error:
    return 1;
}
#endif

int J3DAPI sithTemplate_ReadThingTemplatesListText(SithWorld* pWorld, int bSkip)
{
    size_t numTemplates = 0;

    if ( bSkip )
    {
        return 1;
    }

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_entry.aArgs[0].argValue, "world") || !streq(stdConffile_entry.aArgs[1].argValue, "templates") )
    {
#line 250
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading thing template list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    numTemplates = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !numTemplates )
    {
        return 0;
    }

    else if ( sithTemplate_AllocWorldTemplates(pWorld, numTemplates) )
    {
#line 261
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory allocation failure reading thing templates.\n", 0, 0, 0, 0);
        return 1;
    }

    return sithTemplate_ParseTemplateList(pWorld);
}

int J3DAPI sithTemplate_WriteThingTemplatesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    return sithThing_WriteThingsListBinary(fh, pWorld, pWorld->numThingTemplates, pWorld->aThingTemplates);
}

int J3DAPI sithTemplate_ReadThingTemplatesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    int result;
    size_t numThingTemplates = pWorld->numThingTemplates;
    if ( sithTemplate_AllocWorldTemplates(pWorld, numThingTemplates) )
    {
        return 1;
    }

    pWorld->numThingTemplates = numThingTemplates;
    result = sithThing_ReadThingsListBinary(fh, pWorld, numThingTemplates, pWorld->aThingTemplates, sithTemplate_AddTemplate);
    return result;
}

void J3DAPI sithTemplate_AddTemplate(SithThing* pTemplate)
{
    sithTemplate_CacheAdd(pTemplate);
}

/* Linker-discarded (no caller in either image): the master-file loader. The debug build keeps it at 0x00473C8D; retail
   keeps only its orphaned " \t" literal (0x0052F014) in sithTemplate's string block. */
int J3DAPI sithTemplate_LoadMasterFile(const char* pFilename)
{
    int bDone = 0;
    char* pDesc = NULL;
    char aSecondToken[64];
    char* pNext;
    char aKey[64];

#line 297
    SITH_ASSERTREL(pFilename);
    if ( !stdConffile_Open(pFilename) )
    {
        return 1;
    }

#line 302
    sithTemplate_pMasterFile = (SithTemplate*)STDMALLOC(sizeof(SithTemplate) * SITHTEMPLATE_MAXMASTERTEMPLATES);
    sithTemplate_pMasterHashtable = stdHashtbl_New(2 * SITHTEMPLATE_MAXMASTERTEMPLATES);
    if ( !sithTemplate_pMasterFile || !sithTemplate_pMasterHashtable )
    {
        goto error;
    }

    memset(sithTemplate_pMasterFile, 0, sizeof(SithTemplate) * SITHTEMPLATE_MAXMASTERTEMPLATES);
    while ( !bDone && stdConffile_ReadLine() )
    {
        pNext = stdUtil_StringSplit(stdConffile_aLine, aKey, STD_ARRAYLEN(aKey), " \t");
        if ( !strcmp(aKey, "end") )
        {
            bDone = 1;
        }
        else if ( !strcmp(aKey, "desc:") )
        {
            if ( pDesc )
            {
                sithTemplate_MasterFileAddDescription(pDesc);
            }

            pDesc = stdUtil_FastCopy(pNext, sith_pHS);
        }
        else if ( aKey[0] )
        {
            if ( pNext )
            {
                stdUtil_StringSplit(pNext, aSecondToken, STD_ARRAYLEN(aSecondToken), " \t");
                sithTemplate_MasterFileAddTemplate(stdUtil_FastCopy(aKey, sith_pHS), pDesc, stdUtil_FastCopy(stdConffile_aLine, sith_pHS), stdUtil_FastCopy(aSecondToken, sith_pHS));
            }
            else
            {
#line 343
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Syntax Error in file %s on line %d\n", pFilename, stdConffile_GetLineNumber(), 0, 0);
            }

            pDesc = NULL;
        }
    }

    stdConffile_Close();
    return 0;

error:
    stdConffile_Close();
    return 1;
}

/* Linker-discarded: retail __LINE__ gap 261..459 (sithTemplate_ReadThingTemplatesListText -> sithTemplate_FreeWorldTemplates), where the original
   defined it. Body restored from the debug build's text. */
void J3DAPI sithTemplate_FreeMasterFile(void)
{
    size_t i;

    for ( i = 0; i < sithTemplate_masterFileCount; ++i )
    {
        if ( sithTemplate_pMasterFile[i].pName )
        {
            stdMemory_Free((void*)sithTemplate_pMasterFile[i].pName);
        }

        if ( sithTemplate_pMasterFile[i].pText )
        {
            stdMemory_Free((void*)sithTemplate_pMasterFile[i].pText);
        }

        if ( sithTemplate_pMasterFile[i].pBasedOn )
        {
            stdMemory_Free((void*)sithTemplate_pMasterFile[i].pBasedOn);
        }

        if ( sithTemplate_pMasterFile[i].pDescription )
        {
            stdMemory_Free((void*)sithTemplate_pMasterFile[i].pDescription);
        }
    }

    if ( sithTemplate_pMasterFile )
    {
        stdMemory_Free(sithTemplate_pMasterFile);
        sithTemplate_pMasterFile = NULL;
    }

    sithTemplate_masterFileCount = 0;
    if ( sithTemplate_pMasterHashtable )
    {
        stdHashtbl_Free(sithTemplate_pMasterHashtable);
        sithTemplate_pMasterHashtable = NULL;
    }
}

/* Linker-discarded master-file helpers (no caller in retail; the debug build keeps them after FreeMasterFile, at
   0x00473FC3, 0x00474021, 0x004740E8, 0x004740FB and 0x00474105). */
int J3DAPI sithTemplate_MasterFileAddDescription(const char* pDesc)
{
#line 390
    SITH_ASSERTREL(pDesc);
    if ( sithTemplate_masterFileCount >= SITHTEMPLATE_MAXMASTERTEMPLATES )
    {
        return 0;
    }

    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pDescription = pDesc;
    sithTemplate_masterFileCount++;
    return 1;
}

int J3DAPI sithTemplate_MasterFileAddTemplate(const char* pName, const char* pDesc, const char* pText, const char* pBasedOn)
{
#line 404
    SITH_ASSERTREL(pName && pText);
    if ( sithTemplate_masterFileCount >= SITHTEMPLATE_MAXMASTERTEMPLATES )
    {
        return 0;
    }

    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pName        = pName;
    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pDescription = pDesc;
    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pText        = pText;
    sithTemplate_pMasterFile[sithTemplate_masterFileCount].pBasedOn     = pBasedOn;
    stdHashtbl_Add(sithTemplate_pMasterHashtable, pName, &sithTemplate_pMasterFile[sithTemplate_masterFileCount]);
    sithTemplate_masterFileCount++;
    return 1;
}

SithTemplate* J3DAPI sithTemplate_MasterFileGetTemplateByIndex(size_t index)
{
    return &sithTemplate_pMasterFile[index];
}

size_t J3DAPI sithTemplate_MasterFileCount(void)
{
    return sithTemplate_masterFileCount;
}

SithTemplate* J3DAPI sithTemplate_MasterFileGetTemplate(const char* pName)
{
#line 436
    SITH_ASSERTREL(pName != NULL);
    if ( !sithTemplate_pMasterHashtable )
    {
        return NULL;
    }

    return (SithTemplate*)stdHashtbl_Find(sithTemplate_pMasterHashtable, pName);
}

void J3DAPI sithTemplate_FreeWorldTemplates(SithWorld* pWorld)
{
    size_t i;

#line 459
    SITH_ASSERTREL(pWorld != NULL);

    for ( i = 0; i < pWorld->numThingTemplates; ++i )
    {
        rdThing_FreeEntry(&pWorld->aThingTemplates[i].renderData);
        sithTemplate_CacheRemove(&pWorld->aThingTemplates[i]);
    }

    if ( pWorld->aThingTemplates )
    {
#line 473
        SITH_ASSERTREL(pWorld->sizeThingTemplates > 0);

        stdMemory_Free(pWorld->aThingTemplates);
        pWorld->aThingTemplates    = NULL;
        pWorld->sizeThingTemplates = 0;
        pWorld->numThingTemplates  = 0;
    }
    else
    {
#line 481
        SITH_ASSERTREL(pWorld->sizeThingTemplates == 0);
    }
}

SithThing* J3DAPI sithTemplate_GetTemplate(const char* pName)
{
    SithThing* pTemplate;
    char szTemplateText[2048];

#line 492
    SITH_ASSERTREL(pName);

    if ( streq(pName, "none") || !*pName )
    {
        return NULL;
    }

    pTemplate = sithTemplate_CacheFind(pName);
    if ( pTemplate )
    {
        return pTemplate;
    }

    {
    const SithTemplate* pMFTemplate;
    if ( sithTemplate_masterFileCount && (pMFTemplate = (const SithTemplate*)stdHashtbl_Find(sithTemplate_pMasterHashtable, pName)) != 0 )
    {
        if ( pMFTemplate->pBasedOn )
        {
            sithTemplate_GetTemplate(pMFTemplate->pBasedOn);
        }

        stdConffile_Open("none");

        strncpy(szTemplateText, pMFTemplate->pText, STD_ARRAYLEN(szTemplateText) - 1);
        szTemplateText[STD_ARRAYLEN(szTemplateText) - 1] = '\0';

        if ( !stdConffile_ReadArgsFromStr(szTemplateText) )
        {
            pTemplate = sithTemplate_Parse(sithWorld_pLastLoadedWorld); // first arg is base
        }

        pTemplate = sithTemplate_Parse(sithWorld_pLastLoadedWorld);
        stdConffile_Close();

        if ( !pTemplate )
        {
#line 522
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Template %s found in master file, but could not be loaded.\n", pName, 0, 0, 0);
            return NULL;
        }

        return pTemplate;
    }
    }

#line 532
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Template %s could not be found.\n", pName, 0, 0, 0);
    return NULL;
}

SithThing* J3DAPI sithTemplate_Parse(SithWorld* pWorld)
{
    SithThing* pResult;
    SithThing tempTemplate;
    size_t i;
    const SithThing* pBaseTemplate;

#line 568
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(strlen(stdConffile_entry.aArgs[0].argValue) > 0);

    pResult = sithTemplate_CacheFind(stdConffile_entry.aArgs[0].argValue);
    if ( pResult )
    {
        return pResult;
    }

    
    sithThing_Reset(&tempTemplate);
    pResult = &tempTemplate;

    // Get base template thing
    pBaseTemplate = sithTemplate_CacheFind(stdConffile_entry.aArgs[1].argValue);
    if ( !pBaseTemplate && !streq(stdConffile_entry.aArgs[1].argValue, "none") )
    {
#line 583
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Based-on template %s not found, using blank.\n", stdConffile_entry.aArgs[1].argValue, 0, 0, 0);
    }

    // Assign base to template
    sithThing_SetThingBasedOn(pResult, pBaseTemplate);

    strncpy(pResult->aName, stdConffile_entry.aArgs[0].argValue, STD_ARRAYLEN(pResult->aName) - 1);
    pResult->aName[STD_ARRAYLEN(pResult->aName) - 1] = '\0';

    // Parse template params
    for ( i = 2; i < stdConffile_entry.numArgs; ++i )
    {
        if ( sithThing_ParseArg(&stdConffile_entry.aArgs[i], pWorld, pResult) )
        {
#line 597
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unrecognized argument '%s=%s' line %d.\n", stdConffile_entry.aArgs[i].argName, stdConffile_entry.aArgs[i].argValue, stdConffile_GetLineNumber(), 0);
        }
    }

    if ( pResult->type == SITH_THING_FREE )
    {
#line 604
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Template %s has no type.  Discarded.\n", stdConffile_entry.aArgs[0].argValue, 0, 0, 0);
        return NULL;
    }

    if ( pWorld->numThingTemplates >= pWorld->sizeThingTemplates )
    {
#line 612
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of space for thing templates, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return NULL;
    }

    // Copy template to word template array
    pResult = &pWorld->aThingTemplates[pWorld->numThingTemplates++];
    tempTemplate.idx = pResult->idx;
    memcpy(pResult, &tempTemplate, sizeof(SithThing));

    sithTemplate_CacheAdd(pResult);
    return pResult;
}

SithThing* J3DAPI sithTemplate_CacheFind(const char* pName)
{
#line 636
    SITH_ASSERTREL(pName != NULL);

    if ( !sithTemplate_pHashtable )
    {
        return NULL;
    }

    return (SithThing*)stdHashtbl_Find(sithTemplate_pHashtable, pName);
}

void J3DAPI sithTemplate_CacheAdd(SithThing* pThing)
{
#line 645
    SITH_ASSERTREL(strlen(pThing->aName) > 0);
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL(sithTemplate_pHashtable);
    stdHashtbl_Add(sithTemplate_pHashtable, pThing->aName, pThing);
}

void J3DAPI sithTemplate_CacheRemove(const SithThing* pThing)
{
#line 655
    SITH_ASSERTREL(strlen(pThing->aName) > 0);
    SITH_ASSERTREL(pThing != NULL);
    if ( !sithTemplate_pHashtable )
    {
        return;
    }

    stdHashtbl_Remove(sithTemplate_pHashtable, pThing->aName);
}

int J3DAPI sithTemplate_ParseTemplateList(SithWorld* pWorld)
{
    int unusedFlag = 0; // Debug build initialises this local and never reads it.

#line 667
    SITH_ASSERTREL(pWorld != NULL);

    while ( stdConffile_ReadArgs() && !streq(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        if ( !sithTemplate_Parse(pWorld) )
        {
#line 675
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Template parse failed for template on line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
            continue;
        }
    }

    return 0;
}
