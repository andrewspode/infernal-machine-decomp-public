#include "sithSound.h"

#include <sith/Devices/sithSoundMixer.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/Win95/stdWin95.h>

#include <stdio.h>
#include <string.h>

#define SITHSOUND_FILENAMELEN 64

static int bStartup;
static int bSkipRestore; // Name unconfirmed; spelling chosen so VC5 places it after bStartup

int J3DAPI sithSound_Initialize(tHostServices* pHS)
{
    return Sound_Initialize(pHS);
}

void J3DAPI sithSound_Uninitialize(void)
{
    Sound_Uninitialize();
}

int J3DAPI sithSound_StartupSound(void)
{
    return Sound_Startup();
}

void J3DAPI sithSound_ShutdownSound(void)
{
    Sound_Shutdown();
}

int J3DAPI sithSound_Startup(int bSound3D)
{
    SoundOpenFlags flags;
    int bError;

#line 101
    SITH_ASSERTREL(!bStartup);

    flags = 0;
    if ( !bSound3D )
    {
        flags |= SOUNDOPEN_NO3DSOUND;
    }

    Sound_Open(flags, SITHSOUND_MAXCHANNELS, sithSoundMixer_GetThingInfo, sithSoundMixer_CalcCameraRelativeSoundMix, stdWin95_GetWindow());

    Sound_Set3DGlobals(10.0f, 0.5f, 2.5f, 1.0f, 1.0f);
    Sound_EnableLoad(1);

    bError = sithSoundMixer_Startup();
    if ( bError )
    {
        goto error;
    }

    bError = sithSoundClass_Startup();
    if ( bError )
    {
        goto error;
    }

    bSkipRestore = 0;
    bStartup = 1;
    return 0;

error:
#line 125
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "stdSound_Startup failed (no hardware?).\n", 0, 0, 0, 0);
    sithSound_Shutdown();
    return 1;
}

void J3DAPI sithSound_Shutdown(void)
{
    sithSoundClass_Shutdown();
    sithSoundMixer_Shutdown();
    Sound_Close();

    bSkipRestore = 0;
    bStartup = 0;
}

/* Discarded by the retail linker (its only caller is the debug-only sithWrite.c); restored in full from the
   debug build (0x0045F2D8, between Shutdown and ReadSoundsListText). Every failed write jumps to one
   `return 1` laid after the success return, from inside the loop too (debug
   jmp 0x0045F3AE). Name inferred. */
int J3DAPI sithSound_WriteSoundsListText(const SithWorld* pWorld)
{
    int i;
    const char* pName;

    if ( stdConffile_WriteLine("###### Sound information  ######\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("SECTION: SOUNDS\n\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World sounds %d\n\n", pWorld->numSounds + 80) )
    {
        goto error;
    }

    for ( i = 0; i < (int)pWorld->numSounds; i++ )
    {
        pName = Sound_GetSoundFilename(Sound_GetSoundHandle(i));
        if ( pName )
        {
            if ( stdConffile_Printf("%s\n", pName) )
            {
                goto error;
            }
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

int J3DAPI sithSound_ReadSoundsListText(SithWorld* pWorld, int bSkip)
{
    size_t sizeSounds;
    size_t sizeBuf;
    size_t nFiles;
    char* pFilenames = NULL;
    size_t currenti;
    char* name;

#line 205
    SITH_ASSERTREL(pWorld);

    if ( bSkip )
    {
        return 1;
    }

    sithWorld_UpdateLoadProgress(0.0f);

    if ( !stdConffile_ReadArgs() )
    {
        goto error;
    }

    if ( !streq(stdConffile_entry.aArgs[0].argValue, "world") || !streq(stdConffile_entry.aArgs[1].argValue, "sounds") )
    {
        goto error;
    }

    sizeSounds = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !sizeSounds )
    {
        return 0;
    }

    pWorld->numSounds = 0;
    nFiles = 0;
#line 222
    sizeBuf = sizeSounds * SITHSOUND_FILENAMELEN;
#line 223
    pFilenames = (char*)STDMALLOC(sizeBuf);
    if ( !pFilenames )
    {
        goto error;
    }

    memset(pFilenames, 0, sizeBuf);

    name = pFilenames;
    while ( stdConffile_ReadArgs() && !streq(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        ++nFiles;
        sprintf(name, "%s", stdConffile_entry.aArgs[0].argValue);
        name += SITHSOUND_FILENAMELEN;
    }

    if ( bSkipRestore )
    {
        bSkipRestore = 0;
        return 0;
    }

    name = pFilenames;
    for ( currenti = 0; currenti < nFiles; ++currenti )
    {
        sithSound_Load(pWorld, name);
        name += SITHSOUND_FILENAMELEN;
    }

#line 248
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "%d sounds loaded.\n", pWorld->numSounds, 0, 0, 0);
    if ( pFilenames )
    {
        stdMemory_Free(pFilenames);
    }

    return 0;

error:
    if ( pFilenames )
    {
        stdMemory_Free(pFilenames);
    }

    sithSound_FreeWorldSounds(pWorld);
    return 1;
}


int J3DAPI sithSound_WriteSoundsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        if ( !Sound_ExportStaticBank(fh) )
        {
            return 1;
        }
    }
    else if ( !Sound_ExportNormalBank(fh) )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithSound_ReadSoundsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    if ( bSkipRestore )
    {
        bSkipRestore = 0;
        if ( !Sound_SkipSoundFileSection(fh) )
        {
            return 1;
        }

        return 0;
    }

    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        if ( !Sound_ImportStaticBank(fh) )
        {
            return 1;
        }
    }
    else if ( !Sound_ImportNormalBank(fh) )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithSound_FreeWorldSounds(SithWorld* pWorld)
{
#line 302
    SITH_ASSERTREL(pWorld != NULL);

    if ( bSkipRestore )
    {
        Sound_StopAllSounds();
    }
    else
    {
        Sound_Reset(pWorld->state & SITH_WORLD_STATE_STATIC);
        pWorld->numSounds = 0;
    }
}

tSoundHandle J3DAPI sithSound_Load(SithWorld* pWorld, const char* filename)
{
    char aPath[128];
    tSoundHandle hSnd;
    uint32_t sndIdx;

#line 340
    SITH_ASSERTREL(filename != NULL);
#line 341
    SITH_ASSERTREL(pWorld);

    if ( !bStartup )
    {
        return 0;
    }

    if ( !filename )
    {
        return 0;
    }

    if ( streq(filename, "none") )
    {
        return 0;
    }

    
    sprintf(aPath, "%s%c%s", "sound", '\\', filename);

    sndIdx = pWorld->numSounds;
    
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        sndIdx = SITHWORLD_STATICINDEX(sndIdx);
        hSnd = Sound_LoadStatic(aPath, &sndIdx);
    }
    else
    {
        hSnd = Sound_Load(aPath, &sndIdx);
    }

    if ( !hSnd )
    {
#line 364
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not open sound file '%s'.\n", aPath, 0, 0, 0);
        return hSnd;
    }

    sndIdx = SITHWORLD_FROM_STATICINDEX(sndIdx);
    if ( sndIdx == pWorld->numSounds )
    {
        ++pWorld->numSounds;
    }

    return hSnd;
}

void J3DAPI sithSound_SetSkipRestoringSounds(int bSkip)
{
    bSkipRestore = bSkip;
}