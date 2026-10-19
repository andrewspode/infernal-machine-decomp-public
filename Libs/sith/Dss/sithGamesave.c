#include "sithGamesave.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdConffile.h>
#include <std/General/stdFnames.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include <w32util/wuRegistry.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithGamesave_countfit_1, 13)
/* end count-fit */

#define SITHSAVEGAME_THUMBBPP        24
#define SITHSAVEGAME_THUMBSIZE       ((SITHSAVEGAME_THUMB_WIDTH * SITHSAVEGAME_THUMB_HEIGHT * SITHSAVEGAME_THUMBBPP) / 8) // 0x9000

#define SITHSAVEGAME_FILEVERSION  13
#define SITHSAVEGAME_ENDFILE      0x1000 

static int sithGamesave_state = 0;
static char sithGamesave_aLastFilename[128] = { 0 };
static int sithGamesave_aNdsSaveSectionSizes[15] = { 0 };
static SithGameStatistics sithGamesave_gameStatistics = { 0 };
static char sithGamesave_aPrevLevelFilename[64] = { 0 };
static char sithGamesave_aCurFilename[128] = { 0 };
static int sithGamesave_bStatisticsLocked = 0;
static tVBuffer* sithGamesave_pThumbnailImage = NULL;
static int sithGamesave_bThumbnail = 0;
static HBITMAP sithGamesave_hBmpThumbnail = NULL;
static SithGameSaveCallback sithGamesave_pfSaveGameCallback = NULL;

int J3DAPI sithGamesave_SaveCurrentWorld(SithMessageStream outstream);

int J3DAPI sithGamesave_RestoreFile(const char* pFilename, int bNotify);
int J3DAPI sithGamesave_ReadBlockTypeLength(uint16_t* pType, unsigned int* pLength);
int J3DAPI sithGamesave_SaveFile(const char* pFilename);

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh);
int J3DAPI sithGamesave_SeekThumbnail(tFileHandle fh);

SithGameStatistics* J3DAPI sithGamesave_GetGameStatistics(void)
{
    if ( !sithGamesave_bStatisticsLocked )
    {
        return &sithGamesave_gameStatistics;
    }

    return NULL;
}

int J3DAPI sithGamesave_LockGameStatistics(void)
{
    if ( !sithGamesave_bStatisticsLocked )
    {
        sithGamesave_bStatisticsLocked = 1;
        return 1;
    }

    return 0;
}

int J3DAPI sithGamesave_UnlockGameStatistics(void)
{
    sithGamesave_bStatisticsLocked = 0;
    return 1;
}

/* Linker-discarded (no caller in either build): debug 0x004507AC, right after UnlockGameStatistics. */
int J3DAPI sithGamesave_sub_4507AC(void)
{
    return sithGamesave_bStatisticsLocked;
}

int J3DAPI sithGamesave_Save(const char* pFilename, int bOverWrite)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;

#line 185
    SITH_ASSERTREL(pFilename);
    SITH_ASSERTREL(pWorld);

    // Multiplayer game?
    if ( (sith_mode.subModeFlags & 1) != 0 )
    {
        return 1;
    }

    // Don't save game when player is dead/dying
    if ( (sithPlayer_pLocalPlayerThing->flags & SITH_TF_DYING) != 0 )
    {
        return 1;
    }

    // Ensure folder structure exists
    sithMakeDirs();

    if ( !bOverWrite && stdConffile_Open(pFilename) )
    {
#line 201
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cannot overwrite existing file without bOverWrite specified.\n", 0, 0, 0, 0);
        stdConffile_Close();
        return 1;
    }

    // Deferred save game
    sithGamesave_state = SITHGAMESAVE_SAVE;
    strncpy(sithGamesave_aCurFilename, pFilename, STD_ARRAYLEN(sithGamesave_aCurFilename) - 1);
    sithGamesave_aCurFilename[STD_ARRAYLEN(sithGamesave_aCurFilename) - 1] = 0;
    return 0;
}

int J3DAPI sithGamesave_Restore(const char* pFilename, int bNotifyCog)
{
    // Check restoring savegame file exists and can be opened
#line 227
    SITH_ASSERTREL(pFilename);
    if ( !stdConffile_OpenMode(pFilename, "rb") )
    {
#line 231
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not open savegame file %s.\n", pFilename, 0, 0, 0);
        return 1;
    }

    stdConffile_Close();

    // If restoring from active game, defer restoring game from savegame file until current game is closed
    if ( sithWorld_pCurrentWorld )
    {
        sithGamesave_state = SITHGAMESAVE_RESTORE;
        strncpy(sithGamesave_aCurFilename, pFilename, STD_ARRAYLEN(sithGamesave_aCurFilename) - 1);
        sithGamesave_aCurFilename[STD_ARRAYLEN(sithGamesave_aCurFilename) - 1] = 0;
        return 0;
    }

    return sithGamesave_RestoreFile(pFilename, bNotifyCog);
}

int J3DAPI sithGamesave_SaveCurrentWorld(SithMessageStream outstream)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    SithThing* pThing;
    SithEvent* pPendingEvent;
    int bFailed;
    size_t i;

#line 280
    SITH_ASSERTREL(pWorld);

    // If outstream is not set to required stream, return error
    if ( (sithMessage_outputstream & outstream) == 0 )
    {
        return 1;
    }

    if ( sithGamesave_pfSaveGameCallback )
    {
        bFailed = sithGamesave_pfSaveGameCallback(DPID_ALLPLAYERS, outstream);
        if ( bFailed )
        {
            return bFailed;
        }
    }

    // Write things state
    for ( i = 0, pThing = pWorld->aThings; i < pWorld->numThings; ++i, ++pThing )
    {
        if ( sithThing_CanSync(pThing) )
        {
            bFailed = sithDSSThing_FullDescription(pThing, DPID_ALLPLAYERS, outstream);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[1] += sithMulti_message.length;

            if ( pThing->renderData.pPuppet )
            {
                bFailed = sithDSS_PuppetStatus(pThing, DPID_ALLPLAYERS, outstream);
                if ( bFailed )
                {
                    return bFailed;
                }

                sithGamesave_aNdsSaveSectionSizes[2] += sithMulti_message.length;
            }
        }
    }

    // Write attachment things state
    for ( i = 0; i < pWorld->numThings; ++i )
    {
        if ( sithThing_CanSync(&pWorld->aThings[i]) && pWorld->aThings[i].attach.flags )
        {
            bFailed = sithDSSThing_Attachment(&pWorld->aThings[i], DPID_ALLPLAYERS, outstream, 1u);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[3] += sithMulti_message.length;
        }
    }

    // Write AIs state
    for ( i = 0; i < STD_ARRAYLEN(sithAI_aControlBlocks); ++i )
    {
        if ( sithAI_aControlBlocks[i].pClass )
        {
            bFailed = sithDSS_AIStatus(&sithAI_aControlBlocks[i], DPID_ALLPLAYERS, outstream);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[4] += sithMulti_message.length;
        }
    }

    // Write AI waypoints
    sithAIUtil_SyncWpnts(DPID_ALLPLAYERS, outstream);

    // Write level cogs state
    for ( i = 0; i < pWorld->numCogs; ++i )
    {
        bFailed = sithDSSCog_SyncCogState(&pWorld->aCogs[i], DPID_ALLPLAYERS, outstream);
        if ( bFailed )
        {
            return bFailed;
        }

        sithGamesave_aNdsSaveSectionSizes[7] += sithMulti_message.length;
    }

    // Write static cogs state
    if ( sithWorld_pStaticWorld )
    {
        for ( i = 0; i < sithWorld_pStaticWorld->numCogs; ++i )
        {
            bFailed = sithDSSCog_SyncCogState(&sithWorld_pStaticWorld->aCogs[i], DPID_ALLPLAYERS, outstream);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[8] += sithMulti_message.length;
        }
    }

    // Write surfaces state
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        if ( (pWorld->aSurfaces[i].flags & SITH_SURFACE_SYNC) != 0 )
        {
            bFailed = sithDSS_SurfaceStatus(&pWorld->aSurfaces[i], DPID_ALLPLAYERS, outstream);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[5] += sithMulti_message.length;
        }
    }

    // Write sectors state
    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        if ( (pWorld->aSectors[i].flags & SITH_SECTOR_SYNC) != 0 || (pWorld->aSectors[i].flags & SITH_SECTOR_ADJOINSOFF) != 0 )
        {
            bFailed = sithDSS_SectorStatus(&pWorld->aSectors[i], DPID_ALLPLAYERS, outstream);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[6] += sithMulti_message.length;
        }
    }

    // Write inventory state
    for ( i = 0; i < STD_ARRAYLEN(sithInventory_aTypes); ++i )
    {
        if ( (sithInventory_aTypes[i].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 )
        {
            bFailed = sithDSS_Inventory(sithPlayer_pLocalPlayerThing, i, DPID_ALLPLAYERS, outstream);
            if ( bFailed )
            {
                return bFailed;
            }

            sithGamesave_aNdsSaveSectionSizes[9] += sithMulti_message.length;
        }
    }

    // Write state of animate module
    bFailed = sithAnimate_Save(outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[10] += sithMulti_message.length;

    // Write event system state
    for ( pPendingEvent = sithEvent_pFirstQueuedEvent; pPendingEvent; pPendingEvent = pPendingEvent->pNextEvent )
    {
        bFailed = sithDSS_SyncTaskEvents(pPendingEvent, DPID_ALLPLAYERS, outstream);
        if ( bFailed )
        {
            return bFailed;
        }

        sithGamesave_aNdsSaveSectionSizes[11] += sithMulti_message.length;
    }

    // Write state of all cameras
    bFailed = sithDSS_SyncCameras(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[12] += sithMulti_message.length;

    // Write game state (player flying state, player cur weapon num, drawn chalk marks, current cel num of all world materials  etc...)
    bFailed = sithDSS_SyncGameState(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_message.length;

    // Write voice system state
    bFailed = sithVoice_SyncVoiceState(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_message.length;

    // Write vhicle control system state
    bFailed = sithDSS_SyncVehicleControlState(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_message.length;

    // Write whip system state
    bFailed = sithWhip_Save(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_message.length;

    // Write state of seen sectors
    bFailed = sithDSS_SyncSeenSectors(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_message.length;

    // Write unknown section
    bFailed = sithDSS_SendEndSection(DPID_ALLPLAYERS, outstream);
    if ( bFailed )
    {
        return bFailed;
    }

    sithGamesave_aNdsSaveSectionSizes[14] += sithMulti_message.length;
    return 0;
}

int J3DAPI sithGamesave_Process(void)
{
    int bError;
    switch ( sithGamesave_state )
    {
        case SITHGAMESAVE_RESTORE:
            sithGamesave_Close();

            // Restore game from savegame file, and notify master cog
            bError = sithGamesave_RestoreFile(sithGamesave_aCurFilename, /*bNotify*/0);
            if ( sithCog_pMasterCog )
            {
                sithCog_SendMessage(sithCog_pMasterCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
            }

            sithGamesave_state = SITHGAMESAVE_NONE;
            sithGamesave_Close();
            break;

        case SITHGAMESAVE_SAVE:
            if ( !sithGamesave_bThumbnail )
            {
                sithGamesave_SetThumbnailImage(&stdDisplay_backBuffer);
                sithGamesave_hBmpThumbnail = sithGamesave_CreateThumbnail();
            }

            bError = sithGamesave_SaveFile(sithGamesave_aCurFilename);
            if ( !bError )
            {
                wuRegistry_SaveStr("Last Save Game", sithGamesave_aCurFilename);
            }

            // If savegame is autosave i.e.: savegame at the start of a level (start_xxx.nds), notify master cog
            if ( strneq(stdFnames_FindMedName(sithGamesave_aCurFilename), sithGetAutoSaveFilePrefix(), strlen(sithGetAutoSaveFilePrefix())) && sithCog_pMasterCog )
            {
                sithCog_SendMessage(sithCog_pMasterCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
            }

            sithGamesave_state = SITHGAMESAVE_NONE;
            sithGamesave_Close();
            break;

        default:
            bError = 0;
            break;
    }

    return bError;
}

// Reads the savegame file header and sections. Returns 1 on failure.
int J3DAPI sithGamesave_RestoreFile(const char* pFilename, int bNotify)
{
    int bError = 0;  // unreferenced; the debug build's frame keeps its slot
    tFileHandle fh;
    int hresult;
    NdsHeader hdr;
    char aText[64];
    size_t n;
    SithCogSymbol* pSymbol;

    // Note, the file handle is set to the status returned by stdConffile_OpenMode and not to the opened file handle
    fh = stdConffile_OpenMode(pFilename, "rb");
    if ( !fh )
    {
#line 550
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: could not open restore file!\n", 0, 0, 0, 0);
        goto error;
    }

    // Read nds file hdr
    if ( !stdConffile_Read(&hdr, sizeof(NdsHeader)) )
    {
#line 557
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: header read failed!\n", 0, 0, 0, 0);
        goto error;
    }

    // Check hdr version
    if ( hdr.version != SITHSAVEGAME_FILEVERSION )
    {
#line 569
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: version numbers don't match!\n", 0, 0, 0, 0);
        goto error;
    }

    sithGamesave_bThumbnail = hdr.bThumbnail;
    sithSetPerformanceLevel(hdr.perflevel);

    strncpy(sithGamesave_aPrevLevelFilename, hdr.aPreviousLevelFilename, STD_ARRAYLEN(sithGamesave_aPrevLevelFilename) - 1);
    sithGamesave_aPrevLevelFilename[STD_ARRAYLEN(sithGamesave_aPrevLevelFilename) - 1] = 0;

    // Skip thumbnail section
    hresult = sithGamesave_SeekThumbnail(fh);
    if ( hresult )
    {
#line 582
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: seekThumbNail failed!\n", 0, 0, 0, 0);
        goto error;
    }

    // Close current level
    if ( sithWorld_pCurrentWorld && streqi(sithWorld_pCurrentWorld->aName, hdr.aLevelFilename) )
    {
        // Loading the same level, sithSound_FreeWorldSounds won't free sounds when set here to skip restoring sounds.
        sithSound_SetSkipRestoringSounds(1);
    }

    if ( sithWorld_pCurrentWorld )
    {
        sithClose();
    }

    // Load level specified in hdr
    if ( sithOpenNormal(hdr.aLevelFilename, L"Indiana Jones") )
    {
#line 599
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Restore: sithOpenNormal failed!\n", 0, 0, 0, 0);
        goto error;
    }

    // Copy game statistics from hdr
    sithGamesave_gameStatistics = hdr.gameStatistics;

    // Copy the state of COG global symbols from hdr
    for ( n = 0; n < STD_ARRAYLEN(hdr.aCogGlobalValues); ++n )
    {
        sprintf(aText, "%s%d", "global", n);
        pSymbol = sithCogParse_GetSymbol(sithCog_pSymbolTable, aText);
        if ( pSymbol )
        {
            memcpy(&pSymbol->value, &hdr.aCogGlobalValues[n], sizeof(hdr.aCogGlobalValues[n]));
        }
    }

    // Set local player number
    sithPlayer_playerNum = hdr.localPlayerNum;

    // Reset system and clear all things
    sithAnimate_Reset();
    sithEvent_Reset();
    sithFX_ClearChalkMarks();
    sithThing_RemoveWorldThings(sithWorld_pCurrentWorld);

    // Parse & restore gamesave file sections
    hresult = sithGamesave_ReadBlockTypeLength(&sithMulti_message.type, &sithMulti_message.length);
    while ( !hresult )
    {
        if ( sithMulti_message.length && !stdConffile_Read(sithMulti_message.data, sithMulti_message.length) )
        {
#line 647
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "stdConfile_Read failed, last type %d.\n", sithMulti_message.type, 0, 0, 0);
            goto error;
        }

        if ( !sithMessage_Process(&sithMulti_message) )
        {
#line 655
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithMessage_Process() failed for DSS message type %d.\n", sithMulti_message.type, 0, 0, 0);
            goto error;
        }

        hresult = sithGamesave_ReadBlockTypeLength(&sithMulti_message.type, &sithMulti_message.length);
        if ( hresult && hresult != SITHSAVEGAME_ENDFILE ) // Reading error
        {
#line 661
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "ReadBlockTypeLength failed, type %d.\n", sithMulti_message.type, 0, 0, 0);
        }
    }

    if ( hresult != SITHSAVEGAME_ENDFILE )
    {
#line 666
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "hresult != ENDFILE, last type %d\n", sithMulti_message.type, 0, 0, 0);
        goto error;
    }

    // Restore sound mixer state from savegame file
    hresult = sithSoundMixer_GameRestore(fh);
    if ( hresult )
    {
#line 674
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: sithSoundMixer_GameRestore failed!\n", 0, 0, 0, 0);
        goto error;
    }

    // Restore weapon system state from savegame file
    hresult = sithWeapon_Restore(fh);
    if ( hresult )
    {
#line 681
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: sithWeapon_Restore failed!\n", 0, 0, 0, 0);
        return 1;
    }

    // Load post process
    sithThing_LoadPostProcess(sithWorld_pCurrentWorld);
    sithPlayer_SetLocalPlayer(hdr.localPlayerNum);

    // Close savegame file
    stdConffile_Close();
    fh = 0;

    strncpy(sithGamesave_aLastFilename, pFilename, STD_ARRAYLEN(sithGamesave_aLastFilename) - 1);
    sithGamesave_aLastFilename[STD_ARRAYLEN(sithGamesave_aLastFilename) - 1] = 0;

    // Fix COG links to things
#line 702
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: calling sithCog_FixupLinksToThings\n", 0, 0, 0, 0);
    sithCog_FixupLinksToThings();

    // Set game time from savegame file
    sithTime_SetGameTime(hdr.msecGameTime);
#line 710
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "RESTORE: returning SUCCESS!\n", 0, 0, 0, 0);

    if ( bNotify == 1 )
    {
        sithGamesave_NotifyRestored(pFilename);
    }

    return 0; // Restore succeeded

error:
    stdConffile_Close();
    sithClose();
#line 743
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Savegame parsing failed for %s.\n", pFilename, 0, 0, 0);
    return 1;
}

void J3DAPI sithGamesave_NotifyRestored(const char* pFilePath)
{
    // If restoring from autosave i.e.: savegame at the start of a level (start_xxx.nds), notify master cog
    if ( strneq(stdFnames_FindMedName(pFilePath), sithGetAutoSaveFilePrefix(), strlen(sithGetAutoSaveFilePrefix())) )
    {
        if ( sithCog_pMasterCog )
        {
            sithCog_SendMessage(sithCog_pMasterCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }
}

int J3DAPI sithGamesave_ReadBlockTypeLength(uint16_t* pType, uint32_t* pLength)
{
    if ( !stdConffile_Read(pType, sizeof(uint16_t)) )
    {
        goto error;
    }

    if ( !stdConffile_Read(pLength, sizeof(uint32_t)) )
    {
        goto error;
    }

    if ( *pType == SITHDSS_ENDSECTION )
    {
        return SITHSAVEGAME_ENDFILE; // end of savegame file section
    }

    return 0; // success

error:
    *pType   = SITHDSS_ENDSECTION;
    *pLength = 0;
    return 1;
}

int J3DAPI sithGamesave_SaveFile(const char* pFilename)
{
    tFileHandle fh;
    SithMessageStream curStream;
    NdsHeader header;
    char aText[64];
    size_t n;
    SithCogSymbol* pSymbol;
    int hresult;

#line 797
    SITH_ASSERTREL(sithPlayer_pLocalPlayerThing);

    // Don't make savegame file when player is dying
    if ( (sithPlayer_pLocalPlayerThing->flags & SITH_TF_DYING) != 0 )
    {
        return 1;
    }

    memset(&header, 0, sizeof(header));
    memset(sithGamesave_aNdsSaveSectionSizes, 0, sizeof(sithGamesave_aNdsSaveSectionSizes));

    // Note, the file handle is set to the status returned by stdConffile_OpenWrite and not to the opened write file handle
    fh = stdConffile_OpenWrite(pFilename);
    if ( !fh )
    {
#line 810
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening file %s for writing.\n", pFilename, 0, 0, 0);
        return 1;
    }

    // Change output stream to file
    curStream = sithMessage_outputstream;
    sithMessage_outputstream = SITHMESSAGE_STREAM_FILE;

    // Init NDS file header
    header.version = SITHSAVEGAME_FILEVERSION;
    strncpy(header.aDate, "Jan 28 2000", STD_ARRAYLEN(header.aDate) - 1); // __DATE__ of the retail build
    header.aDate[STD_ARRAYLEN(header.aDate) - 1] = 0;

    strncpy(header.aLevelFilename, sithWorld_pCurrentWorld->aName, STD_ARRAYLEN(header.aLevelFilename) - 1);
    header.aLevelFilename[STD_ARRAYLEN(header.aLevelFilename) - 1] = 0;

    strncpy(header.aPreviousLevelFilename, sithGamesave_aPrevLevelFilename, STD_ARRAYLEN(sithGamesave_aPrevLevelFilename) - 1);
    header.aPreviousLevelFilename[STD_ARRAYLEN(sithGamesave_aPrevLevelFilename) - 1] = 0;

    header.msecGameTime   = sithTime_msecGameTime;
    header.gameStatistics = sithGamesave_gameStatistics;
    header.bThumbnail     = sithGamesave_bThumbnail;
    header.perflevel      = sithGetPerformanceLevel();

    // Copy COG global symbols to header
    for ( n = 0; n < STD_ARRAYLEN(header.aCogGlobalValues); ++n )
    {
        sprintf(aText, "%s%d", "global", n);

        pSymbol = sithCogParse_GetSymbol(sithCog_pSymbolTable, aText);
        if ( !pSymbol )
        {
            memset(&header.aCogGlobalValues[n], 0, sizeof(header.aCogGlobalValues[n]));
        }
        else
        {
            memcpy(&header.aCogGlobalValues[n], &pSymbol->value, sizeof(header.aCogGlobalValues[n]));
            switch ( pSymbol->value.type )
            {
                case SITHCOG_VALUE_POINTER:
                case SITHCOG_VALUE_SYMBOLID:
                case SITHCOG_VALUE_STRING:
                    // Reset symbol value if it is a pointer, symbol id or string
                    memset(&header.aCogGlobalValues[n].val, 0, sizeof(header.aCogGlobalValues[n].val));
                    break;
            }
        }
    }

    header.localPlayerNum = sithPlayer_playerNum;

    // Write header to file
    hresult = stdConffile_Write(&header, sizeof(NdsHeader));
    if ( hresult )
    {
        return 1;
    }

    sithGamesave_aNdsSaveSectionSizes[0] = sizeof(NdsHeader);

    // Write thumbnail
    hresult = sithGamesave_WriteThumbnail(fh);
    if ( hresult )
    {
        return 1;
    }

    // Save world state to file
    hresult = sithGamesave_SaveCurrentWorld(SITHMESSAGE_STREAM_FILE);
    if ( hresult )
    {
        return 1;
    }

    // Write sound mixer state
    hresult = sithSoundMixer_GameSave(fh);
    if ( hresult )
    {
        return 1;
    }

    // Write weapon system state to file
    hresult = sithWeapon_Save(fh);
    if ( hresult )
    {
        return 1;
    }

    // Writing savegame succeeded
    stdConffile_CloseWrite();
    fh = 0;

    strncpy(sithGamesave_aLastFilename, pFilename, STD_ARRAYLEN(sithGamesave_aLastFilename) - 1);
    sithGamesave_aLastFilename[STD_ARRAYLEN(sithGamesave_aLastFilename) - 1] = 0;

    // Print game saved to console
    sithConsole_PrintWString(sithString_GetString("GAME_SAVED"));

    sithMessage_outputstream = curStream;
    return 0; // success
}

int J3DAPI sithGamesave_SetThumbnailImage(tVBuffer* pVBuffer)
{
    int index;
    size_t pixelSize;
    tRasterInfo rasterInfo;
    if ( !pVBuffer )
    {
        return 0;
    }

    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }

    if ( sithGamesave_hBmpThumbnail )
    {
        DeleteObject(sithGamesave_hBmpThumbnail);
        sithGamesave_hBmpThumbnail = NULL;
    }


    // Clone the thumbnail image to sithGamesave_pThumbnailImage
    memset(&rasterInfo, 0, sizeof(rasterInfo));
    rasterInfo = pVBuffer->rasterInfo;
    sithGamesave_pThumbnailImage = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
    if ( !sithGamesave_pThumbnailImage )
    {
        return 0;
    }

    pixelSize = sithGamesave_pThumbnailImage->rasterInfo.rowSize / sithGamesave_pThumbnailImage->rasterInfo.rowWidth;
    stdDisplay_VBufferLock(pVBuffer);
    for ( index = 0; index < (int)rasterInfo.height; ++index )
    {
        uint8_t* pDestRow = &sithGamesave_pThumbnailImage->pPixels[index * sithGamesave_pThumbnailImage->rasterInfo.rowSize];
        uint8_t* pCurSourceRow  = &pVBuffer->pPixels[index * pVBuffer->rasterInfo.rowSize];
        memcpy(pDestRow, pCurSourceRow, pVBuffer->rasterInfo.width * pixelSize);
    }

    stdDisplay_VBufferUnlock(pVBuffer);
    return 1;
}

HBITMAP J3DAPI sithGamesave_CreateThumbnail(void)
{
    ColorInfo currentthumbColorInfo = { STDCOLOR_RGB, 24, 8, 8, 8, 0, 8, 16, 0, 0, 0, 0, 0, 0 };
    BITMAPINFO bmpInfo;
    LONG imageSize;
    HDC hHdc;
    HDC hdcThumb;
    HBITMAP hBmp;
    void* pPixmap;

    if ( sithGamesave_hBmpThumbnail )
    {
        return sithGamesave_hBmpThumbnail;
    }

    if ( !sithGamesave_pThumbnailImage )
    {
        goto error;
    }

    sithGamesave_pThumbnailImage = stdDisplay_VBufferConvertColorFormat(&currentthumbColorInfo, sithGamesave_pThumbnailImage, 0, 0);
    imageSize = sithGamesave_pThumbnailImage->rasterInfo.width * sithGamesave_pThumbnailImage->rasterInfo.height * 3;

    bmpInfo.bmiHeader.biSize          = sizeof(bmpInfo.bmiHeader);
    bmpInfo.bmiHeader.biWidth         = sithGamesave_pThumbnailImage->rasterInfo.width;
    bmpInfo.bmiHeader.biHeight        = -(int32_t)sithGamesave_pThumbnailImage->rasterInfo.height;
    bmpInfo.bmiHeader.biPlanes        = 1;
    bmpInfo.bmiHeader.biBitCount      = SITHSAVEGAME_THUMBBPP;
    bmpInfo.bmiHeader.biCompression   = BI_RGB;
    bmpInfo.bmiHeader.biSizeImage     = imageSize;
    bmpInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpInfo.bmiHeader.biClrUsed       = 0;
    bmpInfo.bmiHeader.biClrImportant  = 0;

    hHdc = CreateCompatibleDC(NULL);
    if ( !hHdc )
    {
        goto error;
    }

    hBmp = CreateDIBSection(hHdc, &bmpInfo, DIB_RGB_COLORS, &pPixmap, NULL, 0);
    if ( !hBmp )
    {
        goto error;
    }

    memcpy(pPixmap, sithGamesave_pThumbnailImage->pPixels, bmpInfo.bmiHeader.biSizeImage);

    bmpInfo.bmiHeader.biSize          = sizeof(bmpInfo.bmiHeader);
    bmpInfo.bmiHeader.biWidth         = SITHSAVEGAME_THUMB_WIDTH;
    bmpInfo.bmiHeader.biHeight        = -SITHSAVEGAME_THUMB_HEIGHT;
    bmpInfo.bmiHeader.biPlanes        = 1;
    bmpInfo.bmiHeader.biBitCount      = SITHSAVEGAME_THUMBBPP;
    bmpInfo.bmiHeader.biCompression   = BI_RGB;
    bmpInfo.bmiHeader.biSizeImage     = SITHSAVEGAME_THUMBSIZE;
    bmpInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpInfo.bmiHeader.biClrUsed       = 0;
    bmpInfo.bmiHeader.biClrImportant  = 0;

    hdcThumb = CreateCompatibleDC(NULL);
    if ( !hdcThumb )
    {
        goto error;
    }

    sithGamesave_hBmpThumbnail = CreateDIBSection(hdcThumb, &bmpInfo, DIB_RGB_COLORS, &pPixmap, NULL, 0);
    if ( !sithGamesave_hBmpThumbnail )
    {
        goto error;
    }

    // Resize bmp
    SelectObject(hHdc, hBmp);
    SelectObject(hdcThumb, sithGamesave_hBmpThumbnail);
    SetStretchBltMode(hdcThumb, STRETCH_HALFTONE);
    StretchBlt(hdcThumb, 0, 0, SITHSAVEGAME_THUMB_WIDTH, SITHSAVEGAME_THUMB_HEIGHT, hHdc, 0, 0, sithGamesave_pThumbnailImage->rasterInfo.width, sithGamesave_pThumbnailImage->rasterInfo.height, SRCCOPY);

    stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
    sithGamesave_pThumbnailImage = NULL;

    DeleteDC(hHdc);
    DeleteObject(hBmp);
    DeleteDC(hdcThumb);

    sithGamesave_bThumbnail = 1;
    return sithGamesave_hBmpThumbnail;

error:
    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }

    if ( hBmp )
    {
        DeleteObject(hBmp);
    }

    if ( hHdc )
    {
        DeleteDC(hHdc);
    }

    if ( hdcThumb )
    {
        DeleteDC(hdcThumb);
    }

    sithGamesave_bThumbnail = 0;
    return NULL;
}

int J3DAPI sithGamesave_WriteThumbnail(tFileHandle fh)
{
    BITMAP bmp;
    size_t imgSize;

    if ( sithGamesave_bThumbnail )
    {
        if ( !GetObject(sithGamesave_hBmpThumbnail, sizeof(BITMAP), &bmp) )
        {
            return 1;
        }

        imgSize = bmp.bmWidth * bmp.bmHeight * 3;
        if ( imgSize != sith_pHS->pFileWrite(fh, bmp.bmBits, bmp.bmWidth * bmp.bmHeight * 3) )
        {
            return 1;
        }

        // Success
        sithGamesave_bThumbnail = 0;
    }

    return 0;
}

int J3DAPI sithGamesave_SeekThumbnail(tFileHandle fh)
{
    // Skips thumbnail section
    if ( sithGamesave_bThumbnail && sith_pHS->pFileSeek(fh, SITHSAVEGAME_THUMBSIZE, 1) ) // 0x9000 - 128 * 96 * 3
    {
        return 1;
    }

    return 0;
}

HBITMAP J3DAPI sithGamesave_LoadThumbnail(const char* pFilename)
{
    NdsHeader currentheader;
    BITMAPINFO bmi;
    void* pPixels;
    HBITMAP hBitmap;
    HDC hdc = NULL;
    int bCurOpened;
    size_t imageSize;

    bCurOpened = stdConffile_OpenMode(pFilename, "rb");
    if ( !bCurOpened )
    {
        return NULL;
    }

    // Read nds file header
    if ( !stdConffile_Read(&currentheader, sizeof(NdsHeader)) )
    {
        goto error;
    }

    if ( currentheader.version != SITHSAVEGAME_FILEVERSION )
    {
        goto error;
    }

    if ( !currentheader.bThumbnail )
    {
        goto error;
    }

    bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth         = SITHSAVEGAME_THUMB_WIDTH;
    bmi.bmiHeader.biHeight        = -SITHSAVEGAME_THUMB_HEIGHT;
    imageSize                     = SITHSAVEGAME_THUMBSIZE;
    bmi.bmiHeader.biPlanes        = 1;
    bmi.bmiHeader.biBitCount      = SITHSAVEGAME_THUMBBPP;
    bmi.bmiHeader.biCompression   = BI_RGB;
    bmi.bmiHeader.biSizeImage     = imageSize;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed       = 0;
    bmi.bmiHeader.biClrImportant  = 0;

    hdc = CreateCompatibleDC(NULL);
    if ( !hdc )
    {
        goto error;
    }

    hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pPixels, NULL, 0);
    if ( !hBitmap )
    {
        goto error;
    }

    if ( !stdConffile_Read(pPixels, imageSize) )
    {
        goto error;
    }

    DeleteDC(hdc);
    stdConffile_Close();
    return hBitmap;

error:
    if ( hdc )
    {
        DeleteDC(hdc);
    }

    stdConffile_Close();
    return NULL;
}

void J3DAPI sithGamesave_Startup(void)
{
    sithGamesave_hBmpThumbnail = NULL;
    sithGamesave_bThumbnail    = 0;
    memset(sithGamesave_aPrevLevelFilename, 0, sizeof(sithGamesave_aPrevLevelFilename));
}

void J3DAPI sithGamesave_Shutdown(void)
{
    sithGamesave_Close();
    memset(sithGamesave_aPrevLevelFilename, 0, sizeof(sithGamesave_aPrevLevelFilename));
}

void J3DAPI sithGamesave_Open(void)
{
    sithGamesave_hBmpThumbnail = NULL;
    sithGamesave_bThumbnail = 0;
}

void J3DAPI sithGamesave_CloseRestore(void)
{
    if ( sithGamesave_state == SITHGAMESAVE_SAVE )
    {
        return;
    }

    sithGamesave_Close();
}

void J3DAPI sithGamesave_Close(void)
{
    if ( sithGamesave_hBmpThumbnail )
    {
        DeleteObject((HGDIOBJ)sithGamesave_hBmpThumbnail);
        sithGamesave_hBmpThumbnail = NULL;
        sithGamesave_bThumbnail = 0;
    }

    if ( sithGamesave_pThumbnailImage )
    {
        stdDisplay_VBufferFree(sithGamesave_pThumbnailImage);
        sithGamesave_pThumbnailImage = NULL;
    }
}

void J3DAPI sithGamesave_SetPreviousLevelFilename(const char* pFilename)
{
    strncpy(sithGamesave_aPrevLevelFilename, pFilename, STD_ARRAYLEN(sithGamesave_aPrevLevelFilename) - 1);
    sithGamesave_aPrevLevelFilename[STD_ARRAYLEN(sithGamesave_aPrevLevelFilename) - 1] = 0;
}

const char* J3DAPI sithGamesave_GetPreviousLevelFilename(void)
{
    return sithGamesave_aPrevLevelFilename;
}

/* Linker-discarded (no caller in either build): debug 0x004521E2, between GetPreviousLevelFilename and GetLastFilename
   (a 23-byte `return !sithGamesave_state`). */
int J3DAPI sithGamesave_sub_4521E2(void)
{
    if ( !sithGamesave_state ) {
        return 1;
    }

    return 0;
}

const char* J3DAPI sithGamesave_GetLastFilename(void)
{
    return sithGamesave_aLastFilename;
}

int J3DAPI sithGamesave_LoadLevelFilename(const char* pNdsFilePath, char* pDestFilename)
{
    NdsHeader header;
    int bOpened;

    bOpened = stdConffile_OpenMode(pNdsFilePath, "rb");
    if ( !bOpened )
    {
        goto error;
    }

    if ( !stdConffile_Read(&header, sizeof(NdsHeader)) )
    {
        goto error;
    }

    stdConffile_Close();
    if ( header.version != SITHSAVEGAME_FILEVERSION )
    {
        goto error;
    }

    if ( pDestFilename )
    {
        strncpy(pDestFilename, header.aLevelFilename, 64 - 1); // dest buffer is 64 chars
        pDestFilename[64 - 1] = 0;
    }

    return 0;

error:
    stdConffile_Close();
    return 1;
}

int J3DAPI sithGamesave_GetState(const char** ppCurFilename)
{
    *ppCurFilename = sithGamesave_aCurFilename;
    return sithGamesave_state;
}

void J3DAPI sithGamesave_SetSaveGameCallback(SithGameSaveCallback pfCallback)
{
    sithGamesave_pfSaveGameCallback = pfCallback;
}

