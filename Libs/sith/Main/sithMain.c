#include "sithMain.h"
#include "sithString.h"

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIAwareness.h>
#include <sith/AI/sithAIClass.h>
#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Primitives/rdPrimit3.h>

#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <w32util/wuRegistry.h>

#define SITH_PATHSIZE 128
#define SITH_DEFAULTGAMEDIFFICULTY 5
#define SITH_NUMLEVELS 17

// Private BSS block 0x00563CF8-0x00564398, in retail order (zero-initialized definitions are declaration-ordered)
static char sith_aLevelNdsFileNameBuf[512]   = { 0 };
static char sith_aScreenShotsFileName[SITH_PATHSIZE] = { 0 };
static char sith_aNdsLevelFilenameBuf[512]   = { 0 };
static char sith_aTmpQuickSavePrefix[SITH_PATHSIZE]     = { 0 };
static char sith_aTmpSaveGameDirPath[SITH_PATHSIZE]     = { 0 };
static char sith_aTmpAutoSaveFilePrefix[SITH_PATHSIZE]  = { 0 };
static char sith_aTmpScreenShotDirPath[SITH_PATHSIZE]   = { 0 };

tHostServices* sith_pHS = NULL;

static int sith_bDrawUnknown          = 0;
int sith_bDrawPlayerMoveBounds        = 0;
static int sith_bDrawThingMoveBounds  = 0;
static int sith_bStartup              = 0;
static int sith_bOpen                 = 0;
static int sith_performanceLevel      = 0;
static SithOpenCallback sith_pfOpenCallback = NULL;

// Common storage
size_t sithMain_frameNumber;
SithMode sith_mode;
size_t sithMain_curRenderTick;

rdVector3 sith_unknownPos;
rdVector3 sith_unknownPos2;
float sith_unknownRadius;
float sith_unknownRadius2;

static const SithMainStartLevelNdsInfo sithMain_aLevelNdsInfos[SITH_NUMLEVELS] =
{
    // Note all level file names must be of 6 chars; see sithGetCurrentWorldSaveName
    { "SITHSTRING_STARTCYN", "00_CYN" },
    { "SITHSTRING_STARTBAB", "01_BAB" },
    { "SITHSTRING_STARTRIV", "02_RIV" },
    { "SITHSTRING_STARTSHS", "03_SHS" },
    { "SITHSTRING_STARTLAG", "05_LAG" },
    { "SITHSTRING_STARTVOL", "06_VOL" },
    { "SITHSTRING_STARTTEM", "07_TEM" },
    { "SITHSTRING_STARTJEP", "16_JEP" },
    { "SITHSTRING_STARTTEO", "08_TEO" },
    { "SITHSTRING_STARTOLV", "09_OLV" },
    { "SITHSTRING_STARTSEA", "10_SEA" },
    { "SITHSTRING_STARTPYR", "11_PYR" },
    { "SITHSTRING_STARTSOL", "12_SOL" },
    { "SITHSTRING_STARTNUB", "13_NUB" },
    { "SITHSTRING_STARTINF", "14_INF" },
    { "SITHSTRING_STARTAET", "15_AET" },
    { "SITHSTRING_STARTPRU", "17_PRU" }
};

const char* J3DAPI sithGetPath(char* aOutPath, const char* pSithStrName, int bPrependInstallDirPath);

void J3DAPI sithSetServices(tHostServices* pHS)
{
    sith_pHS = pHS;
}

void J3DAPI sithClearServices(void)
{
    sith_pHS = NULL;
}

int J3DAPI sithStartup(void)
{
    int bSuccess;

    //TODO: Add check for system already being started

    bSuccess = 1;
    sithGamesave_Startup();
    bSuccess &= sithEvent_Startup();
    bSuccess &= sithWorld_Startup();
    bSuccess &= sithRender_Startup();
    sithCollision_Startup();
    bSuccess &= sithThing_Startup();
    sithMessage_Startup();

    if ( sithCog_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithAI_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithSprite_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithParticle_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithPuppet_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithAIClass_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithMaterial_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithTemplate_Startup() )
    {
        bSuccess = 0;
    }

    if ( sithModel_Startup() )
    {
        bSuccess = 0;
    }

    sithAnimate_Startup();
    sithFX_Reset();

    bSuccess &= sithVoice_Startup();

    memset(&sith_mode, 0, sizeof(sith_mode));

    sithSetGameDifficulty(wuRegistry_GetInt("Difficulty", SITH_DEFAULTGAMEDIFFICULTY));

    memset(sith_aTmpAutoSaveFilePrefix, 0, sizeof(sith_aTmpAutoSaveFilePrefix));
    memset(sith_aTmpSaveGameDirPath, 0, sizeof(sith_aTmpSaveGameDirPath));
    memset(sith_aTmpQuickSavePrefix, 0, sizeof(sith_aTmpQuickSavePrefix));
    memset(sith_aTmpScreenShotDirPath, 0, sizeof(sith_aTmpScreenShotDirPath));
    memset(sith_aScreenShotsFileName, 0, sizeof(sith_aScreenShotsFileName));

    if ( bSuccess )
    {
        sith_bStartup = 1;
        return 0;
    }

#line 203
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sith startup failed.\n", 0, 0, 0, 0);
    return 1;
}


void J3DAPI sithShutdown(void)
{
    if ( sith_bStartup != 1 )
    {
        return;
    }

    sithVoice_Shutdown();
    sithAnimate_Shutdown();
    sithModel_Shutdown();
    sithTemplate_Shutdown();
    sithMaterial_Shutdown();
    sithAIClass_Shutdown();
    sithPuppet_Shutdown();
    sithParticle_Shutdown();
    sithSprite_Shutdown();

    sithAI_Shutdown();
    sithCog_Shutdown();
    sithMessage_Shutdown();
    sithThing_Shutdown();
    sithCollision_Shutdown();
    sithRender_Shutdown();
    sithWorld_Shutdown();
    sithEvent_Shutdown();
    sithGamesave_Shutdown();

    memset(sith_aTmpAutoSaveFilePrefix, 0, sizeof(sith_aTmpAutoSaveFilePrefix));
    memset(sith_aTmpSaveGameDirPath, 0, sizeof(sith_aTmpSaveGameDirPath));
    memset(sith_aTmpQuickSavePrefix, 0, sizeof(sith_aTmpQuickSavePrefix));
    memset(sith_aTmpScreenShotDirPath, 0, sizeof(sith_aTmpScreenShotDirPath));
    memset(sith_aScreenShotsFileName, 0, sizeof(sith_aScreenShotsFileName));

    sith_bStartup = 0;
}

void J3DAPI sithSetPerformanceLevel(size_t level)
{
    sith_performanceLevel = level;
}

int J3DAPI sithGetPerformanceLevel(void)
{
    return sith_performanceLevel;
}

int J3DAPI sithOpenStatic(const char* pFilename)
{
    char szPath[128];
    const char* pResourceDir = "\\Jones3D\\Resource\\Ndy";

    sithCloseStatic();

    sithWorld_pStaticWorld = sithWorld_NewEntry();
    if ( !sithWorld_pStaticWorld )
    {
        return 1;
    }

    sithWorld_pStaticWorld->state |= SITH_WORLD_STATE_STATIC;

    // Load world from file
    STD_MAKEPATH(szPath, "ndy", pFilename);
    if ( sithWorld_Load(sithWorld_pStaticWorld, szPath) )
    {
        // Failed to load world from cwd path, try from another path
        sithWorld_Free(sithWorld_pStaticWorld);
        sithWorld_pStaticWorld = sithWorld_NewEntry();
        if ( !sithWorld_pStaticWorld )
        {
            return 1;
        }

        STD_MAKEPATH(szPath, pResourceDir, pFilename);
        if ( sithWorld_Load(sithWorld_pStaticWorld, szPath) )
        {
            sithWorld_Free(sithWorld_pStaticWorld);
            sithWorld_pStaticWorld = NULL;
            return 1;
        }
    }

    return 0;
}

void J3DAPI sithCloseStatic(void)
{
    if ( sithWorld_pStaticWorld )
    {
        sithWorld_Free(sithWorld_pStaticWorld);
        sithWorld_pStaticWorld = NULL;
    }
}

int J3DAPI sithOpenNormal(const char* pWorldName, const wchar_t* pwPlayerName)
{
    char aPath[128];

    sithWorld_pCurrentWorld = sithWorld_NewEntry();
    if ( !sithWorld_pCurrentWorld )
    {
        return 1;
    }

    sithAIUtil_AIResetWaypoints();

    STD_MAKEPATH(aPath, "ndy", pWorldName);
    if ( sithWorld_Load(sithWorld_pCurrentWorld, aPath) ) {
        goto error;
    }

    sithWorld_InitPlayers(sithWorld_pCurrentWorld);
    if ( sithOpen(pwPlayerName) ) {
        goto error;
    }

    // Success
    sith_mode.masterMode = SITH_MODE_OPENED;
    sithSetGameDifficulty(sith_mode.difficulty);
    return 0;

error:
#line 351
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening world: %s.\n", pWorldName, 0, 0, 0);
    if ( sithWorld_pCurrentWorld )
    {
        sithWorld_Free(sithWorld_pCurrentWorld);
    }

    sithWorld_pCurrentWorld = NULL;
    return 1;
}

void J3DAPI sithOpenPostProcess(void)
{
    size_t i;

    sithTime_Reset();
    sithInventory_ResetInventory(sithPlayer_pLocalPlayerThing);
    sithCog_BroadcastMessage(SITHCOG_MSG_STARTUP, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0);

    // Send initialized & created message to every thing COG
    for ( i = 0; i < sithWorld_pCurrentWorld->numThings; ++i )
    {
        SithThing* pThing = &sithWorld_pCurrentWorld->aThings[i];
        if ( pThing->pCog )
        {
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_INITIALIZED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_CREATED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    if ( stdComm_IsGameActive() )
    {
        sithPlayer_NewPlayer(sithPlayer_pLocalPlayerThing);
        sithMulti_SendWelcome(sithMessage_localPlayerId, sithPlayer_playerNum, SITHMESSAGE_SENDTOJOINEDPLAYERS);
        sithMulti_SendWelcome(sithMessage_localPlayerId, sithPlayer_playerNum, SITHMESSAGE_SENDTOJOINEDPLAYERS); // TODO: Why same message sent twice
    }
    else if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        // Make savegame at the start of the level
        char aFilename[128];
        char aFilepath[128];

        stdUtil_Format(aFilename, STD_ARRAYLEN(aFilename), "%s%s", sithGetAutoSaveFilePrefix(), sithGetCurrentWorldSaveName());
        stdFnames_ChangeExt(aFilename, "nds");
        STD_MAKEPATH(aFilepath, sithGetSaveGamesDir(), aFilename);
        sithGamesave_Save(aFilepath, 1);
    }

    sithTime_Reset(); // TODO: Why time is reset 2nd time?
    sithFX_Reset();
}

/* Linker-discarded in retail (__LINE__ gap 351..653, sithOpenNormal -> sithUpdate). Body from the Indy debug build
   (0x0044FCCE, print at line 508): the multiplayer level open, sithOpenNormal's twin that starts the multiplayer game.
   The debug code reaches the print block laid after the success `return 0` from both failures (`je next; jmp 0x0044FD4D`
   at 0x0044FD1F and 0x0044FD31), as in sithOpenNormal. */
int J3DAPI sithOpenMulti(const char* pFilename, const wchar_t* pwPlayerName)
{
    char aPath[128];

    sithWorld_pCurrentWorld = sithWorld_NewEntry();
    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    STD_MAKEPATH(aPath, "ndy", pFilename);
    if ( sithWorld_Load(sithWorld_pCurrentWorld, aPath) ) {
        goto error;
    }

    if ( sithOpen(pwPlayerName) ) {
        goto error;
    }

    sithTime_Reset();
    sithMulti_OpenGame();
    sith_mode.masterMode = SITH_MODE_OPENED;
    return 0;

error:
#line 508
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening multiplayer level %s.\n", pFilename, 0, 0, 0);
    if ( sithWorld_pCurrentWorld )
    {
        sithWorld_Free(sithWorld_pCurrentWorld);
    }

    sithWorld_pCurrentWorld = NULL;
    return 1;
}

int J3DAPI sithOpen(const wchar_t* pwPlayerName)
{
    // TODO: Add check for the system already being open

    if ( sith_pfOpenCallback )
    {
        sith_pfOpenCallback();
    }

    sithMain_frameNumber   = 0;
    sithMain_curRenderTick = 1;
    sithWorld_ResetRenderState(sithWorld_pCurrentWorld);

    sithEvent_Open();

    if ( sithAnimate_Open() )
    {
        goto error;
    }

    if ( !sithVoice_Open() )
    {
        goto error;
    }

    sithAI_Open();
    if ( sithCog_Open(sithWorld_pCurrentWorld) )
    {
        goto error;
    }

    sithControl_Open();
    if ( sithAIAwareness_Open() )
    {
        goto error;
    }

    if ( sithRender_Open() )
    {
        goto error;
    }

    sithPlayer_Open(pwPlayerName);
    sithWeapon_Open();
    if ( sithOverlayMap_Open(0) )
    {
        goto error;
    }

    sithGamesave_Open();

    sith_bOpen = 1;
    return 0;

error:
    return 1;
}

void J3DAPI sithClose(void)
{
    if ( !sith_bStartup )
    {
        return;
    }

    if ( sith_bOpen )
    {
        if ( (sith_mode.subModeFlags & 1) != 0 )
        {
            sithMulti_CloseGame();
        }

        sithGamesave_Close();
        sithOverlayMap_Close();
        sithWeapon_Close();
        sithPlayer_Close();
        sithRender_Close();
        sithAIAwareness_Close();

        sithControl_Close();
        sithCog_Close();

        sithWorld_Close(sithWorld_pCurrentWorld);
        sithWorld_pCurrentWorld = NULL;

        sithAI_Close();
        sithVoice_Close();
        sithAnimate_Close();
        sithEvent_Close();

        if ( sithCamera_pCurCamera )
        {
            sithCamera_pCurCamera->pSector = NULL;
            sithCamera_pCurCamera->pPrimaryFocusThing = NULL;
            sithCamera_pCurCamera->pSecondaryFocusThing = NULL;
        }

        sith_mode.masterMode   = SITH_MODE_CLOSED;
        sith_mode.subModeFlags = 0;

        sith_bOpen = 0;
    }
}

void J3DAPI sithUpdate(void)
{
    // Retail assertion text names the mode global sith_mode
#line 653
    SITH_ASSERTREL(sith_mode.masterMode != SITH_MODE_CLOSED);
    if ( (sith_mode.subModeFlags & 8) != 0 ) // Game host?
    {
        sithTime_Advance();
        sithMessage_ProcessMessages();
        sithAnimate_Update(sithTime_frameTimeFlex);
        sithThing_Update(sithTime_frameTimeFlex, sithTime_frameTime);
    }
    else
    {
        sithSoundMixer_Update();

        if ( sithTime_IsPaused() )
        {
            if ( sith_mode.masterMode != SITH_MODE_UNKNOWN_2 )
            {
                sithControl_Update(sithTime_frameTimeFlex, sithTime_frameTime);
            }
        }
        else
        {
            ++sithMain_frameNumber;

            sithAdvanceRenderTick();
            sithTime_Advance();
            sithEvent_Process();

            if ( sithMessage_inputstream )
            {
                sithMessage_ProcessMessages();
            }

            if ( (sith_mode.debugModeFlags & SITHDEBUG_AIEVENTS_DISABLED) == 0 )
            {
                sithAI_Process(sithTime_frameTimeFlex);
            }

            sithAnimate_Update(sithTime_frameTimeFlex);

            if ( sith_mode.masterMode != SITH_MODE_UNKNOWN_2 )
            {
                sithControl_Update(sithTime_frameTimeFlex, sithTime_frameTime);
            }

            sithThing_Update(sithTime_frameTimeFlex, sithTime_frameTime);
            sithCog_ProcessCogs();
            sithMulti_Update(sithTime_frameTime);
        }
    }
}

void J3DAPI sithDrawScene(void)
{
    if ( (sith_mode.subModeFlags & 8) != 0 )
    {
        return;
    }

    // Render scene
    sithAdvanceRenderTick();
    sithCamera_Update(sithCamera_pCurCamera);
    sithCamera_RenderScene();

    // Draw debug stuff

    if ( sith_bDrawPlayerMoveBounds && sithWorld_pCurrentWorld && sithPlayer_pLocalPlayerThing )
    {
        rdPrimit3_DrawClippedCircle(&sithPlayer_pLocalPlayerThing->pos, sithPlayer_pLocalPlayerThing->collide.movesize, 20.0f, 0xFFFF3F2F, 0xFFFFFFFF); // RGB565
    }

    if ( sith_bDrawUnknown )
    {
        rdPrimit3_DrawClippedCircle(&sith_unknownPos, sith_unknownRadius, 20.0f, 0xFFFF3F2F, 0xFFFFFFFF); // RGB565
        rdPrimit3_DrawClippedCircle(&sith_unknownPos2, sith_unknownRadius2, 20.0f, 0xFFFF3F2F, 0xFFFFFFFF); // RGB565
    }

    if ( sith_bDrawThingMoveBounds && sithWorld_pCurrentWorld )
    {
        int thNum;
        SithThing* pThing;
        for ( thNum = 0, pThing = sithWorld_pCurrentWorld->aThings; thNum <= sithWorld_pCurrentWorld->lastThingIdx; ++thNum, ++pThing )
        {
            if ( pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_WEAPON )
            {
                rdPrimit3_DrawClippedCircle(&pThing->pos, pThing->collide.movesize, 20.0f, 0xFFFF3F2F, 0xFFFFFFFF); // RGB565
            }
        }
    }
}

void J3DAPI sithSetGameDifficulty(int difficulty)
{
    size_t i;

    sith_mode.difficulty = difficulty;
    if ( !sithWorld_pCurrentWorld )
    {
        return;
    }

    for ( i = 0; i < sithWorld_pCurrentWorld->numThings; ++i )
    {
        SithThing* pActor = &sithWorld_pCurrentWorld->aThings[i];
        if ( pActor->type == SITH_THING_ACTOR )
        {
            sithActor_SetDifficulty(pActor);
        }
    }
}

int J3DAPI sithGetGameDifficulty(void)
{
    return sith_mode.difficulty;
}

float J3DAPI sithGetHitAccuarancyScalar(void)
{
    float scalar = 1.0f;
    switch ( sith_mode.difficulty )
    {
        case 0:
            scalar = 0.5f;
            break;

        case 1:
            scalar = 0.67f;
            break;

        case 2:
            scalar = 0.83f;
            break;

        case 3:
            scalar = 1.0f;
            break;

        case 4:
            scalar = 1.17f;
            break;

        case 5:
            scalar = 1.33f;
            break;
    }

    return scalar;
}

float J3DAPI sithGetCombatDamageScalar(void)
{
    float scalar = 1.0f;
    switch ( sith_mode.difficulty )
    {
        case 0:
            scalar = 1.5f;
            break;

        case 1:
            scalar = 1.33f;
            break;

        case 2:
            scalar = 1.17f;
            break;

        case 3:
            scalar = 1.0f;
            break;

        case 4:
            scalar = 0.83f;
            break;

        case 5:
            scalar = 0.67f;
            break;
    }

    return scalar;
}

float J3DAPI sithGetIMPDamageScalar(void)
{
    float scalar = 1.0f;
    switch ( sith_mode.difficulty )
    {
        case 0:
            scalar = 0.5f;
            break;

        case 1:
            scalar = 0.6f;
            break;

        case 2:
            scalar = 0.7f;
            break;

        case 3:
            scalar = 0.8f;
            break;

        case 4:
            scalar = 0.9f;
            break;

        case 5:
            scalar = 1.0f;
            break;
    }

    return scalar;
}

void J3DAPI sithAdvanceRenderTick(void)
{
    if ( !++sithMain_curRenderTick )
    {
        sithWorld_ResetRenderState(sithWorld_pCurrentWorld);
        sithMain_curRenderTick = 1;
    }
}

void J3DAPI sithMakeDirs(void)
{
    stdFileUtil_MkDir(sithGetSaveGamesDir());
    stdFileUtil_MkDir(sithGetScreenShotsDir());
}

const char* J3DAPI sithGetAutoSaveFilePrefix(void)
{
    return sithGetPath(sith_aTmpAutoSaveFilePrefix, "SITHSTRING_AUTO", 0);
}

const char* J3DAPI sithGetPath(char* aOutPath, const char* pSithStrName, int bPrependInstallDirPath)
{
    const wchar_t* pwName;
    char* name;
    char aInstallPath[SITH_PATHSIZE];

    // The functions calling this cache the path in a static buffer, so a non-empty buffer is returned as is
    if ( strlen(aOutPath) == 0 )
    {
        pwName = sithString_GetString(pSithStrName);
        if ( pwName )
        {
            name = stdUtil_ToAString(pwName);

            memset(aInstallPath, 0, sizeof(aInstallPath));
            wuRegistry_GetStr("Install Path", aInstallPath, STD_ARRAYLEN(aInstallPath), "");

            if ( strlen(name) > 0 )
            {
                if ( strlen(aInstallPath) > 0 && bPrependInstallDirPath )
                {
                    char* pFilename = name;
                    char* pStripName = strchr(pFilename, '\\');
                    if ( pStripName )
                    {
                        pFilename = pStripName + 1;
                    }

                    while ( pFilename && *pFilename == '\\' )
                    {
                        ++pFilename;
                    }

                    stdFnames_MakePath(aOutPath, SITH_PATHSIZE, aInstallPath, pFilename);
                }
                else
                {
                    strncpy(aOutPath, name, SITH_PATHSIZE - 1);
                    aOutPath[SITH_PATHSIZE - 1] = 0;
                }
            }
            else
            {
                strncpy(aOutPath, pSithStrName, SITH_PATHSIZE - 1);
                aOutPath[SITH_PATHSIZE - 1] = 0;
            }

            if ( name )
            {
                stdMemory_Free(name);
            }
        }
    }

    return aOutPath;
}

const char* J3DAPI sithGetSaveGamesDir(void)
{
    return sithGetPath(sith_aTmpSaveGameDirPath, "SITHSTRING_SAVEGAMES", 1);
}

const char* J3DAPI sithGetQuickSaveFilePrefix(void)
{
    return sithGetPath(sith_aTmpQuickSavePrefix, "SITHSTRING_QUICK", 0);
}

const char* J3DAPI sithGetScreenShotsDir(void)
{
    return sithGetPath(sith_aTmpScreenShotDirPath, "SITHSTRING_SHOTS", 1);
}

const char* J3DAPI sithGetScreenShotFileTemplate(void)
{
    return sithGetPath(sith_aScreenShotsFileName, "SITHSTRING_JONES_SHOTS", 0);
}

void J3DAPI sithSetOpenCallback(SithOpenCallback pfCallback)
{
    sith_pfOpenCallback = pfCallback;
}

const char* J3DAPI sithGetCurrentWorldSaveName(void)
{
    int i;
    const wchar_t* curpwName;
    char* pName;

    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    for ( i = 0; i < SITH_NUMLEVELS; ++i )
    {
        if ( strneqi(sithWorld_pCurrentWorld->aName, sithMain_aLevelNdsInfos[i].aFilename, 6u) )
        {
            curpwName = sithString_GetString(sithMain_aLevelNdsInfos[i].aName);
            if ( curpwName )
            {
                pName = stdUtil_ToAString(curpwName);
                if ( pName )
                {
                    strcpy(sith_aNdsLevelFilenameBuf, pName);
                    stdMemory_Free(pName);
                    return sith_aNdsLevelFilenameBuf;
                }

                return sithWorld_pCurrentWorld->aName;
            }

            return sithWorld_pCurrentWorld->aName;
        }
    }

    return sithWorld_pCurrentWorld->aName;
}

const char* J3DAPI sithGetLevelSaveFilename(int levelNum)
{
    const wchar_t* pwLevelName = NULL;
    char* pLevelStr;

    if ( levelNum >= SITH_NUMLEVELS || levelNum < 0 )
    {
        return 0;
    }

    pwLevelName = sithString_GetString(sithMain_aLevelNdsInfos[levelNum].aName);
    if ( pwLevelName )
    {
        pLevelStr = stdUtil_ToAString(pwLevelName);
        if ( pLevelStr )
        {
            strcpy(sith_aLevelNdsFileNameBuf, pLevelStr);
            stdMemory_Free(pLevelStr);
            return sith_aLevelNdsFileNameBuf;
        }

        return 0;
    }

    return 0;
}
