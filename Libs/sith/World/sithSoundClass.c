#include "sithSoundClass.h"

#include <sith/AI/sithAIAwareness.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithVoice.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

static tHashTable* sithSoundClass_pHashTable;
static tHashTable* sithSoundClass_pHashtblModes;

static const char* sithSoundClass_aSoundModeNames[SITHSOUNDCLASS_MAXMODES] =
{
    "--reserved--", // used so the modeNum can start at 1, required due to stdHashtbl_Find returns 0 when mode is not found
    "create",
    "activate",
    "startmove",
    "stopmove",
    "moving",
    "lwalkhard",
    "rwalkhard",
    "lrunhard",
    "rrunhard",
    "lwalkmetal",
    "rwalkmetal",
    "lrunmetal",
    "rrunmetal",
    "lwalkwater",
    "rwalkwater",
    "lrunwater",
    "rrunwater",
    "lwalkpuddle",
    "rwalkpuddle",
    "lrunpuddle",
    "rrunpuddle",
    "lwalkearth",
    "rwalkearth",
    "lrunearth",
    "rrunearth",
    "lwalksnow",
    "rwalksnow",
    "lrunsnow",
    "rrunsnow",
    "lwalkwood",
    "rwalkwood",
    "lrunwood",
    "rrunwood",
    "lwalkhardecho",
    "rwalkhardecho",
    "lrunhardecho",
    "rrunhardecho",
    "lwalkwoodecho",
    "rwalkwoodecho",
    "lrunwoodecho",
    "rrunwoodecho",
    "lwalkearthecho",
    "rwalkearthecho",
    "lrunearthecho",
    "rrunearthecho",
    "lwalkaet",
    "rwalkaet",
    "lrunaet",
    "rrunaet",
    "enterwater",
    "enterwaterslow",
    "exitwater",
    "exitwaterslow",
    "lswimsurface",
    "rswimsurface",
    "treadsurface",
    "lswimunder",
    "rswimunder",
    "treadunder",
    "jump",
    "jumpmetal",
    "jumpwater",
    "jumpearth",
    "jumpsnow",
    "jumpwood",
    "jumphardecho",
    "jumpwoodecho",
    "jumpearthecho",
    "jumpaet",
    "landhard",
    "landmetal",
    "landwater",
    "landpuddle",
    "landearth",
    "landsnow",
    "landwood",
    "landhardecho",
    "landwoodecho",
    "landearthecho",
    "landaet",
    "landhurt",
    "hithard",
    "hitmetal",
    "hitearth",
    "deflected",
    "scrapehard",
    "scrapemetal",
    "scrapeearth",
    "hitdamaged",
    "falling",
    "corpsehit",
    "hurtimpact",
    "hurtenergy",
    "hurtfire",
    "hurtfists",
    "hurtmachete",
    "drowning",
    "death1",
    "death2",
    "deathunder",
    "drowned",
    "splattered",
    "pant",
    "breath",
    "gasp",
    "fire1",
    "fire2",
    "fire3",
    "fire4",
    "curious",
    "alert",
    "idle",
    "gloat",
    "fear",
    "boast",
    "slide1",
    "victory",
    "help",
    "flee",
    "search",
    "calm",
    "surprise",
    "reserved1",
    "reserved2",
    "reserved3",
    "reserved4",
    "reserved5",
    "reserved6",
    "reserved7",
    "reserved8",
    "stand2walk",
    "walk2stand",
    "climbhandleft",
    "climbhandright",
    "climbhoriz",
    "climbonto",
    "climbdownonto",
    "lswimaet",
    "rswimaet",
    "treadaet"
};

static size_t sithSoundClass_lastVoiceEntryNum = 1000u;

void J3DAPI sithSoundClass_Free(SithSoundClass* pClass);
tSoundChannelHandle J3DAPI sithSoundClass_PlayModeEntry(SithThing* pThing, SithSoundClassMode mode, SithSoundClassEntry* pEntry, float volume);
SithSoundClass* J3DAPI sithSoundClass_CacheFind(const char* pName);
int J3DAPI sithSoundClass_CacheAdd(SithSoundClass* pClass);
int J3DAPI sithSoundClass_CacheRemove(const SithSoundClass* pClass);

int J3DAPI sithSoundClass_Startup(void)
{
    size_t modeNum;

    sithSoundClass_pHashTable    = stdHashtbl_New(64u);
    sithSoundClass_pHashtblModes = stdHashtbl_New(282u);
    if ( !sithSoundClass_pHashTable || !sithSoundClass_pHashtblModes )
    {
#line 268
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not allocate hashtable for soundclasses.\n", 0, 0, 0, 0);
        return 1;
    }

    for ( modeNum = 1; modeNum < STD_ARRAYLEN(sithSoundClass_aSoundModeNames); ++modeNum )
    {
        stdHashtbl_Add(sithSoundClass_pHashtblModes, sithSoundClass_aSoundModeNames[modeNum], (void*)modeNum);
    }

    return 0;
}

void J3DAPI sithSoundClass_Shutdown(void)
{
    if ( sithSoundClass_pHashTable )
    {
        stdHashtbl_Free(sithSoundClass_pHashTable);
    }

    sithSoundClass_pHashTable = NULL;

    if ( sithSoundClass_pHashtblModes )
    {
        stdHashtbl_Free(sithSoundClass_pHashtblModes);
    }

    sithSoundClass_pHashtblModes = NULL;
}

/* Linker-discarded: retail __LINE__ gap 268..353 (sithSoundClass_Startup -> sithSoundClass_ReadSoundClassesListText), where it
   was defined from the debug build. Body from the debug code (0x004B47AE): every failing write jumps to one
   `return 1` laid after the `return 0`. */
int J3DAPI sithSoundClass_WriteSoundClassesListText(const SithWorld* pWorld)
{
    size_t i;

    if ( stdConffile_WriteLine("#### Sound (foley) Classes #####\n") ) {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: Soundclass\n\n") ) {
        goto error;
    }

    if ( stdConffile_Printf("World soundclasses %d\n", pWorld->numSoundClasses) ) {
        goto error;
    }

    for ( i = 0; i < pWorld->numSoundClasses; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aSoundClasses[i].aName) ) {
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

int J3DAPI sithSoundClass_ReadSoundClassesListText(SithWorld* pWorld, int bSkip)
{
    int numClasses;

    if ( bSkip )
    {
        return 1;
    }

#line 353
    SITH_ASSERTREL(pWorld && !pWorld->aSoundClasses);
    SITH_ASSERTREL(!pWorld->sizeSoundClasses && !pWorld->numSoundClasses);

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_entry.aArgs[0].argValue, "world") || !streq(stdConffile_entry.aArgs[1].argValue, "soundclasses") )
    {
#line 359
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading soundclass list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    numClasses = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( numClasses <= 0 )
    {
        return 0;
    }

    if ( stdComm_IsGameActive() )
    {
        // Add additional buffer size
        numClasses += 32;
    }

    if ( sithSoundClass_AllocSoundClasses(pWorld, numClasses) )
    {
        goto alloc_error;
    }

    while ( stdConffile_ReadArgs() && !streq(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        sithSoundClass_Load(pWorld, stdConffile_entry.aArgs[1].argValue);
    }

#line 386
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Soundclasses read.  Soundclass array space %d, used %d.\n", pWorld->sizeSoundClasses, pWorld->numSoundClasses, 0, 0);
#line 388
    SITH_ASSERTREL(pWorld->numSoundClasses <= pWorld->sizeSoundClasses);
    return 0;

alloc_error:
#line 393
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory error while reading soundclasses, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    return 1;
}

int J3DAPI sithSoundClass_WriteSoundClassesListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t i;
    size_t written;

    for ( i = 0; i < pWorld->numSoundClasses; ++i )
    {
        written = sith_pHS->pFileWrite(fh, &pWorld->aSoundClasses[i].aName, sizeof(pWorld->aSoundClasses[i].aName));
        if ( written != sizeof(pWorld->aSoundClasses[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithSoundClass_ReadSoundClassesListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t i;
    char aFilename[64];
    size_t nRead;
    size_t nClasses = pWorld->numSoundClasses;

    if ( !nClasses )
    {
        return 0;
    }

    if ( sithSoundClass_AllocSoundClasses(pWorld, pWorld->sizeSoundClasses) )
    {
        goto error;
    }

    for ( i = 0; i < nClasses; ++i )
    {
        nRead = sith_pHS->pFileRead(fh, aFilename, sizeof(aFilename));
        if ( nRead != sizeof(aFilename) )
        {
            goto error;
        }

        sithSoundClass_Load(pWorld, aFilename);
    }

    return 0;

error:
    return 1;
}

SithSoundClass* J3DAPI sithSoundClass_Load(SithWorld* pWorld, const char* pName)
{
    SithSoundClass* pClass;
    char aPath[128];

#line 457
    SITH_ASSERTREL(pWorld && pName);
    SITH_ASSERTREL(strlen(pName) < 64);

    if ( !sithSoundClass_pHashTable )
    {
        return NULL;
    }

    if ( streq(pName, "none") || !*pName )
    {
        return NULL;
    }

    if ( !pWorld->aSoundClasses )
    {
#line 469
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Soundclasses array not allocated for this world.\n", 0, 0, 0, 0);
        return NULL;
    }

    sprintf(aPath, "%s%c%s", "misc\\snd", '\\', pName);

    pClass = sithSoundClass_CacheFind(pName);
    if ( pClass )
    {
        return pClass;
    }

    if ( pWorld->numSoundClasses == pWorld->sizeSoundClasses )
    {
#line 485
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many sound classes loaded, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return NULL;
    }

    pClass = &pWorld->aSoundClasses[pWorld->numSoundClasses];
    strncpy(pClass->aName, pName, STD_ARRAYLEN(pClass->aName) - 1);
    pClass->aName[STD_ARRAYLEN(pClass->aName) - 1] = 0;

    if ( !sithSoundClass_LoadEntry(pWorld, pClass, aPath) )
    {
        ++pWorld->numSoundClasses;
        sithSoundClass_CacheAdd(pClass);
        return pClass;
    }

#line 504
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to load soundclass '%s'.\n", aPath, 0, 0, 0);
    return NULL;
}

int J3DAPI sithSoundClass_LoadEntry(SithWorld* pWorld, SithSoundClass* pClass, const char* pPath)
{
    SoundPlayFlag flags = SOUNDPLAY_ABSOLUTE_POS;
    SithSoundClassEntry* pPrevEntry;

#line 533
    SITH_ASSERTREL(pClass && pPath);

    if ( !stdConffile_Open(pPath) )
    {
        return 1;
    }

    while ( stdConffile_ReadArgs() )
    {
        size_t modeNum;
        tSoundHandle hSnd;
        SithSoundClassEntry* pEntry;

        if ( stdConffile_entry.numArgs < 2 )
        {
#line 542
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Short line in file %s, line %d.\n", pPath, stdConffile_GetLineNumber(), 0, 0);
            continue;
        }

        modeNum = (size_t)stdHashtbl_Find(sithSoundClass_pHashtblModes, stdConffile_entry.aArgs[0].argValue);
        if ( modeNum <= 0 || modeNum >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
        {
#line 549
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Mode %s not recognized in file %s.\n", stdConffile_entry.aArgs[0].argValue, pPath, 0, 0);
            continue;
        }

        if ( stdConffile_entry.numArgs > 2 )
        {
            sscanf(stdConffile_entry.aArgs[2].argValue, "%x", &flags);
        }

        if ( streq(stdConffile_entry.aArgs[1].argValue, "none") )
        {
            hSnd = 0;
        }
        else
        {
            hSnd = sithSound_Load(pWorld, stdConffile_entry.aArgs[1].argValue);
            if ( !hSnd )
            {
#line 563
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Mode %d sound %s could not be loaded.\n", modeNum, stdConffile_entry.aArgs[1].argValue, 0, 0);
                continue;
            }
        }

#line 568
        pEntry = (SithSoundClassEntry*)STDMALLOC(sizeof(SithSoundClassEntry));
        if ( !pEntry )
        {
#line 571
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Malloc error loading soundclass entry in %s.\n", pPath, 0, 0, 0);
            continue;
        }

        memset(pEntry, 0, sizeof(SithSoundClassEntry));
        pEntry->hSnd      = hSnd;
        pEntry->playflags = flags;
        pEntry->minRadius = 0.5f;
        pEntry->maxRadius = 2.5f;
        pEntry->maxVolume = 1.0f;

        if ( stdConffile_entry.numArgs > 3 )
        {
            pEntry->minRadius = D3DVAL(atof(stdConffile_entry.aArgs[3].argValue));
        }

        if ( stdConffile_entry.numArgs > 4 )
        {
            pEntry->maxRadius = D3DVAL(atof(stdConffile_entry.aArgs[4].argValue));
        }

        if ( stdConffile_entry.numArgs > 5 )
        {
            pEntry->maxVolume = D3DVAL(atof(stdConffile_entry.aArgs[5].argValue));
        }

        if ( !pClass->aEntries[modeNum] )
        {
            pClass->aEntries[modeNum] = pEntry;
            pEntry->numEntries = 1;
        }
        else
        {
            size_t count;

            count = 1;
            pPrevEntry = pClass->aEntries[modeNum];
            while ( pPrevEntry->pNextMode )
            {
                pPrevEntry = pPrevEntry->pNextMode;
                ++count;
            }

            pPrevEntry->pNextMode = pEntry;
            pClass->aEntries[modeNum]->numEntries = count + 1;
        }
    }

    stdConffile_Close();
    return 0;
}

int J3DAPI sithSoundClass_AllocSoundClasses(SithWorld* pWorld, int size)
{
#line 640
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(size > 0);

#line 643
    pWorld->aSoundClasses = (SithSoundClass*)STDMALLOC(sizeof(SithSoundClass) * size);
    if ( !pWorld->aSoundClasses )
    {
#line 645
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory failure allocating %d soundclasses.\n", size, 0, 0, 0);
        return 1;
    }

    pWorld->sizeSoundClasses = size;
    pWorld->numSoundClasses  = 0;
    memset(pWorld->aSoundClasses, 0, sizeof(SithSoundClass) * size);
    return 0;
}

void J3DAPI sithSoundClass_FreeWorldSoundClasses(SithWorld* pWorld)
{
    size_t i;

#line 706
    SITH_ASSERTREL(pWorld);
    if ( !pWorld->sizeSoundClasses )
    {
#line 710
        SITH_ASSERTREL(!pWorld->aSoundClasses);
        return;
    }

#line 714
    SITH_ASSERTREL(pWorld->aSoundClasses);
    for ( i = 0; i < pWorld->numSoundClasses; ++i )
    {
        sithSoundClass_Free(&pWorld->aSoundClasses[i]);
    }

    stdMemory_Free(pWorld->aSoundClasses);
    pWorld->aSoundClasses    = NULL;
    pWorld->sizeSoundClasses = 0;
    pWorld->numSoundClasses  = 0;
}

void J3DAPI sithSoundClass_Free(SithSoundClass* pClass)
{
    int i;
    SithSoundClassEntry* pCurMode;
    SithSoundClassEntry* pNextMode;

    sithSoundClass_CacheRemove(pClass);
    for ( i = 0; i < SITHSOUNDCLASS_MAXMODES; ++i )
    {
        pCurMode = pClass->aEntries[i];
        while ( pCurMode )
        {
            pNextMode = pCurMode->pNextMode;
            stdMemory_Free(pCurMode);
            pCurMode = pNextMode;
        }
    }
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeFirstEx(SithThing* pThing, SithSoundClassMode mode, float volume)
{
#line 742
    SITH_ASSERTREL(pThing);
    if ( !pThing->pSoundClass )
    {
        return 0;
    }

    if ( mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
#line 749
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound mode %d out of range.\n", mode, 0, 0, 0);
        return 0;
    }

    if ( !pThing->pSoundClass->aEntries[mode] )
    {
        return 0;
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pThing->pSoundClass->aEntries[mode], volume);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeFirst(SithThing* pThing, SithSoundClassMode mode)
{
#line 774
    SITH_ASSERTREL(pThing);
    if ( !pThing->pSoundClass )
    {
        return 0;
    }

    if ( mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
#line 781
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound mode %d out of range.\n", mode, 0, 0, 0);
        return 0;
    }

    if ( !pThing->pSoundClass->aEntries[mode] )
    {
        return 0;
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pThing->pSoundClass->aEntries[mode], 1.0f);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    SithSoundClassEntry* pEntry;
    size_t entryCount;
    size_t i;

    if ( !pThing )
    {
#line 812
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NULL thing in sithSoundClass_PlayModeRandom()", 0, 0, 0, 0);
        return 0;
    }

    if ( !pThing->pSoundClass )
    {
        return 0;
    }

    if ( mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
#line 821
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound modes out of range.\n", 0, 0, 0, 0);
        return 0;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return 0;
    }

    if ( pEntry->numEntries > 1 )
    {
        entryCount = (size_t)D3DMultiply(D3DVAL(SITH_RANDF()), pEntry->numEntries);
        entryCount = STDMATH_CLAMP(entryCount, 0, pEntry->numEntries - 1);

        for ( i = 0; i < entryCount; ++i )
        {
            pEntry = pEntry->pNextMode;
        }
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pEntry, 1.0f);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayVoiceModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    SithSoundClassEntry* pModeEntry;
    size_t entryCount;
    size_t i;
    tSoundHandle hSnd;

    if ( !pThing )
    {
#line 849
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NULL thing in sithSoundClass_PlayModeRandom()", 0, 0, 0, 0);
        return 0;
    }

    if ( !pThing->pSoundClass )
    {
        return 0;
    }

    if ( mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
#line 858
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound modes out of range.\n", 0, 0, 0, 0);
        return 0;
    }

    pModeEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pModeEntry )
    {
        return 0;
    }

    if ( pModeEntry->numEntries > 1 )
    {
        entryCount = (size_t)D3DMultiply(D3DVAL(SITH_RANDF()), pModeEntry->numEntries);
        entryCount = STDMATH_CLAMP(entryCount, 0, pModeEntry->numEntries - 1);

        for ( i = 0; i < entryCount; ++i )
        {
            pModeEntry = pModeEntry->pNextMode;
        }
    }

    hSnd = pModeEntry->hSnd;
    return sithVoice_PlayThingVoice(pThing, hSnd, pModeEntry->maxVolume);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayPlayerVoiceModeRandom(SithThing* pThing, SithSoundClassMode mode)
{
    SithSoundClassEntry* pModeEntry;
    size_t entryCount;
    size_t i;
    tSoundHandle hSnd;

#line 889
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    if ( !pThing )
    {
#line 893
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NULL thing in sithSoundClass_PlayModeRandom()", 0, 0, 0, 0);
        return 0;
    }

    if ( !pThing->pSoundClass )
    {
        return 0;
    }

    if ( mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
#line 902
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound modes out of range.\n", 0, 0, 0, 0);
        return 0;
    }

    pModeEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pModeEntry )
    {
        return 0;
    }

    if ( pModeEntry->numEntries > 1 )
    {
        entryCount = (size_t)D3DMultiply(D3DVAL(SITH_RANDF()), pModeEntry->numEntries);
        entryCount = STDMATH_CLAMP(entryCount, 0, pModeEntry->numEntries - 1);

        if ( entryCount == sithSoundClass_lastVoiceEntryNum && ++entryCount >= pModeEntry->numEntries )
        {
            entryCount = 0;
        }

#line 921
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "%d %d\n", entryCount, sithSoundClass_lastVoiceEntryNum, 0, 0);
        sithSoundClass_lastVoiceEntryNum = entryCount;

        for ( i = 0; i < entryCount; ++i )
        {
            pModeEntry = pModeEntry->pNextMode;
        }
    }

    hSnd = pModeEntry->hSnd;
    return sithVoice_PlayThingVoice(pThing, hSnd, pModeEntry->maxVolume);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayMode(SithThing* pThing, SithSoundClassMode mode, float selectionRatio)
{
    SithSoundClassEntry* pEntry;
    size_t entryCount;
    size_t i;

#line 957
    SITH_ASSERTREL(pThing);
    if ( !pThing->pSoundClass )
    {
        return 0;
    }

    if ( mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
#line 964
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sound modes out of range.\n", 0, 0, 0, 0);
        return 0;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return 0;
    }

    if ( pEntry->numEntries > 1 )
    {
        entryCount = (size_t)D3DMultiply(pEntry->numEntries, selectionRatio);
        entryCount = STDMATH_CLAMP(entryCount, 0, pEntry->numEntries - 1);

        for ( i = 0; i < entryCount; ++i )
        {
            pEntry = pEntry->pNextMode;
        }
    }

    return sithSoundClass_PlayModeEntry(pThing, mode, pEntry, 1.0f);
}

void J3DAPI sithSoundClass_StopMode(SithThing* pThing, SithSoundClassMode mode)
{
    tSoundHandle hSnd;

    if ( !pThing->pSoundClass || mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return;
    }

    if ( !pThing->pSoundClass->aEntries[mode] )
    {
        return;
    }

    hSnd = pThing->pSoundClass->aEntries[mode]->hSnd;
    sithSoundMixer_StopSoundThing(pThing, hSnd);
}

void J3DAPI sithSoundClass_SetModePitch(SithThing* pThing, SithSoundClassMode mode, float pitch)
{
    SithSoundClassEntry* pEntry;
    size_t numEntries;
    size_t i;
    tSoundHandle hSnd;

    if ( !pThing->pSoundClass || mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return;
    }

    numEntries = pEntry->numEntries;
    for ( i = 0; i < numEntries; ++i )
    {
        hSnd = pEntry->hSnd;
        sithSoundMixer_SetPitchThing(pThing, hSnd, pitch);
        pEntry = pEntry->pNextMode;
    }
}

void J3DAPI sithSoundClass_SetModeVolume(SithThing* pThing, SithSoundClassMode mode, float volume)
{
    SithSoundClassEntry* pEntry;
    size_t numEntries;
    size_t i;
    tSoundHandle hSnd;

    if ( !pThing->pSoundClass || mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return;
    }

    numEntries = pEntry->numEntries;
    for ( i = 0; i < numEntries; ++i )
    {
        hSnd = pEntry->hSnd;
        sithSoundMixer_SetVolumeThing(pThing, hSnd, volume);
        pEntry = pEntry->pNextMode;
    }
}

void J3DAPI sithSoundClass_FadeModeVolume(SithThing* pThing, SithSoundClassMode mode, float startVolume, float endVolume)
{
    SithSoundClassEntry* pEntry;
    size_t numEntries;
    size_t i;
    tSoundHandle hSnd;

    if ( !pThing->pSoundClass || mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return;
    }

    numEntries = pEntry->numEntries;
    for ( i = 0; i < numEntries; ++i )
    {
        hSnd = pEntry->hSnd;
        sithSoundMixer_FadeVolumeThing(pThing, hSnd, startVolume, endVolume);
        pEntry = pEntry->pNextMode;
    }
}

int J3DAPI sithSoundClass_IsModeFadingVol(SithThing* pThing, SithSoundClassMode mode)
{
    SithSoundClassEntry* pEntry;

    if ( !pThing->pSoundClass || mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return 0;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return 0;
    }

    return sithSoundMixer_IsThingFadingVol(pThing, pEntry->hSnd);
}

/* IsModePitchFading and SetThingClass (end of file): discarded by the retail linker (no caller in either
   image), restored in full from the debug build (0x004B5C16, 0x004B5F3A; __LINE__ 1222). Names from
   OpenJones3D develop / the debug build's text. */
int J3DAPI sithSoundClass_IsModePitchFading(SithThing* pThing, SithSoundClassMode mode)
{
    SithSoundClassEntry* pEntry;

    if ( !pThing->pSoundClass || mode >= STD_ARRAYLEN(sithSoundClass_aSoundModeNames) )
    {
        return 0;
    }

    pEntry = pThing->pSoundClass->aEntries[mode];
    if ( !pEntry )
    {
        return 0;
    }

    return sithSoundMixer_IsThingFadingPitch(pThing, pEntry->hSnd);
}

tSoundChannelHandle J3DAPI sithSoundClass_PlayModeEntry(SithThing* pThing, SithSoundClassMode mode, SithSoundClassEntry* pEntry, float volume)
{
    int bMakeAIEvent;
    tSoundHandle hwndSnd;
    SoundPlayFlag flags;

    hwndSnd = pEntry->hSnd;
    if ( !hwndSnd )
    {
        return 0;
    }

    flags  = pEntry->playflags;
    volume = D3DMultiply(pEntry->maxVolume, volume);
    if ( (flags & SOUNDPLAY_ABSOLUTE_POS) != 0 )
    {
        return sithSoundMixer_PlaySoundPos(hwndSnd, &pThing->pos, pThing->pInSector, volume, pEntry->minRadius, pEntry->maxRadius, flags);
    }

    bMakeAIEvent = 0;
    if ( pThing == sithPlayer_pLocalPlayerThing )
    {
        switch ( mode )
        {
            // Debug build: the walk and hurt modes share one empty arm (a single break)
            case SITHSOUNDCLASS_LWALKHARD:
            case SITHSOUNDCLASS_RWALKHARD:
            case SITHSOUNDCLASS_LWALKMETAL:
            case SITHSOUNDCLASS_RWALKMETAL:
            case SITHSOUNDCLASS_LWALKWATER:
            case SITHSOUNDCLASS_RWALKWATER:
            case SITHSOUNDCLASS_LWALKPUDDLE:
            case SITHSOUNDCLASS_RWALKPUDDLE:
            case SITHSOUNDCLASS_LWALKEARTH:
            case SITHSOUNDCLASS_RWALKEARTH:
            case SITHSOUNDCLASS_LWALKSNOW:
            case SITHSOUNDCLASS_RWALKSNOW:
            case SITHSOUNDCLASS_LWALKWOOD:
            case SITHSOUNDCLASS_RWALKWOOD:
            case SITHSOUNDCLASS_LWALKHARDECHO:
            case SITHSOUNDCLASS_RWALKHARDECHO:
            case SITHSOUNDCLASS_LWALKWOODECHO:
            case SITHSOUNDCLASS_RWALKWOODECHO:
            case SITHSOUNDCLASS_LWALKEARTHECHO:
            case SITHSOUNDCLASS_RWALKEARTHECHO:
            case SITHSOUNDCLASS_LWALKAET:
            case SITHSOUNDCLASS_RWALKAET:
            case SITHSOUNDCLASS_HURTIMPACT:
            case SITHSOUNDCLASS_HURTENERGY:
            case SITHSOUNDCLASS_HURTFIRE:
                break;

            default:
                bMakeAIEvent = 1;
                break;
        }
    }

    if ( pThing->moveType == SITH_MT_PATH )
    {
        bMakeAIEvent = 1;
    }

    if ( bMakeAIEvent )
    {
        sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 0, D3DMultiply(pEntry->maxRadius, 0.6f), pThing);
    }

    return sithSoundMixer_PlaySoundThing(hwndSnd, pThing, volume, pEntry->minRadius, pEntry->maxRadius, flags);
}

SithSoundClass* J3DAPI sithSoundClass_CacheFind(const char* pName)
{
#line 1186
    SITH_ASSERTREL(pName != NULL);
    if ( !sithSoundClass_pHashTable )
    {
        return NULL;
    }

    return (SithSoundClass*)stdHashtbl_Find(sithSoundClass_pHashTable, pName);
}

int J3DAPI sithSoundClass_CacheAdd(SithSoundClass* pClass)
{
#line 1194
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);

    if ( !sithSoundClass_pHashTable )
    {
        return 0;
    }

    return stdHashtbl_Add(sithSoundClass_pHashTable, pClass->aName, pClass);
}

int J3DAPI sithSoundClass_CacheRemove(const SithSoundClass* pClass)
{
#line 1203
    SITH_ASSERTREL(pClass != NULL);
    SITH_ASSERTREL(strlen(pClass->aName) > 0);

    if ( !sithSoundClass_pHashTable )
    {
        return 0;
    }

    return stdHashtbl_Remove(sithSoundClass_pHashTable, pClass->aName);
}

int J3DAPI sithSoundClass_SetThingClass(SithThing* pThing, SithSoundClass* pSoundClass)
{
#line 1222
    SITH_ASSERT(pThing && pSoundClass);

    if ( pThing->pSoundClass != pSoundClass )
    {
        pThing->pSoundClass = pSoundClass;
        return 1;
    }

    return 0;
}
