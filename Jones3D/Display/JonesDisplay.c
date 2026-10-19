#include "JonesDisplay.h"

#include <Jones3D/Main/JonesMain.h>

#include <rdroid/Engine/rdCanvas.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Primitives/rdWallpaper.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdPlatform.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>

#include <w32util/wuRegistry.h>
#include <wkernel/wkernel.h>

static size_t JonesDisplay_primaryDisplayNum  = 1;
static uint32_t JonesDisplay_backBufferWidth  = 0; // Added: init to 0
static uint32_t JonesDisplay_backBufferHeight = 0;
static rdCanvas* JonesDisplay_pCanvas         = NULL;
static int JonesDisplay_bDualMonitor         = 0; // Added: init to false

// Load screen vars
static rdWallpaper* JonesDisplay_pWallpaper = NULL;
static rdWallLine* JonesDisplay_pWallLine   = NULL;
static tSoundHandle JonesDisplay_hSndLoadMusic = 0;
static tSoundChannelHandle JonesDisplay_hSndChannelLoadMusic = 0;

static size_t JonesDisplay_openLoadScreenCounter = 0;
static int JonesDisplay_dword_553314 = 0;
static int JonesDisplay_bUpdateLoadScreen = 0;
static int JonesDisplay_loadScreenState   = 0;
static int JonesDisplay_dword_553320 = 0;
static int JonesDisplay_bPrimaryLoadMusic = 0;

const rdVector4 JonesDisplay_wallLineColor = { 1.0f , 0.0f , 0.0f , 1.0f };

// Not used vars
static int JonesDisplay_dword_553328 = 0;
static int JonesDisplay_dword_55332C = 0;

int J3DAPI JonesDisplay_Startup(JonesDisplaySettings* pSettings)
{
    int res;
    size_t numBackBuffers;

    if ( !stdDisplay_Startup() )
    {
        goto error;
    }

    JonesDisplay_UpdateDualScreenWindowSize(pSettings);
    if ( pSettings->bWindowMode )
    {
        JonesDisplay_bDualMonitor = 0;
    }

    res = stdDisplay_Open(pSettings->displayDeviceNum);
    if ( !res )
    {
#line 79
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening display device.\n", 0, 0, 0, 0);
        goto error;
    }

    if ( pSettings->bWindowMode ) {
        wkernel_SetWindowStyle(WS_VISIBLE | WS_OVERLAPPEDWINDOW); // 0x10CF0000
    }
    else {
        wkernel_SetWindowStyle(WS_POPUP | WS_VISIBLE);
    }

    stdDisplay_SetDefaultResolution(pSettings->width, pSettings->height);
    JonesDisplay_primaryDisplayNum = pSettings->displayDeviceNum;

    if ( pSettings->bBuffering )
    {
        numBackBuffers = 2;
    }
    else
    {
        numBackBuffers = 1;
    }

    res = stdDisplay_SetMode(pSettings->videoModeNum, pSettings->bWindowMode == 0, numBackBuffers);
    if ( res )
    {
#line 98
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error setting display mode.\n", 0, 0, 0, 0);
        goto error;
    }

    stdDisplay_GetBackBufferSize(&JonesDisplay_backBufferWidth, &JonesDisplay_backBufferHeight);

    if ( !JonesDisplay_UpdateDualScreenWindowSize(pSettings) )
    {
        wkernel_SetWindowSize(JonesDisplay_backBufferWidth, JonesDisplay_backBufferHeight);
    }

    if ( stdDisplay_BackBufferFill(/*color=*/0u, /*pRect=*/NULL) )
    {
        goto error;
    }

    if ( stdDisplay_Update() )
    {
        goto error;
    }

    res = std3D_Startup();
    if ( !res )
    {
#line 117
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "std3D startup failed -- no 3D device?", 0, 0, 0, 0);
        goto error;
    }

    res = sithCamera_Startup();
    if ( !res )
    {
#line 125
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error Camera startup.\n", 0, 0, 0, 0);
        goto error;
    }

    res = std3D_Open(pSettings->device3DNum);
    if ( !res )
    {
#line 133
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "std3D open failed!!!!.\n", 0, 0, 0, 0);
        goto error;
    }

    if ( std3D_SetMipmapFilter(pSettings->filter) )
    {
        goto error;
    }

    if ( !rdOpen() )
    {
        goto error;
    }

    rdSetGeometryMode(pSettings->geoMode);
    if ( sithWorld_pCurrentWorld )
    {
        sithWorld_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
    }

    JonesDisplay_bPrimaryLoadMusic    = 0;
    JonesDisplay_loadScreenState      = 0;
    JonesDisplay_hSndChannelLoadMusic = 0;
    JonesDisplay_hSndLoadMusic        = 0;
    JonesDisplay_dword_55332C         = 0;
    JonesDisplay_dword_553328         = 0;
    return 0;

error:
    JonesDisplay_Shutdown();
    return 1;
}

void J3DAPI JonesDisplay_Shutdown(void)
{
    rdClose();
    std3D_Close();
    stdDisplay_ClearMode();
    stdDisplay_Close();
    sithCamera_Shutdown();

    JonesDisplay_pCanvas = NULL;
    std3D_Shutdown();
    stdDisplay_Shutdown();
}

/* Discarded by the retail linker (no caller in either image); the debug build has it at 0x00415E39, between Shutdown and
   Restart (lines 210/216). Re-opens the display at the size SetBackBufferSize stored. Name inferred. */
int J3DAPI JonesDisplay_ApplyBackBufferSize(JonesDisplaySettings* pSettings)
{
    uint32_t width;
    int bOpened;
    uint32_t height;

    stdDisplay_GetBackBufferSize(&width, &height);
    if ( width == JonesDisplay_backBufferWidth && height == JonesDisplay_backBufferHeight )
    {
        return 0;
    }

    std3D_Close();
    stdDisplay_ClearMode();
    stdDisplay_SetDefaultResolution(JonesDisplay_backBufferWidth, JonesDisplay_backBufferHeight);
    if ( stdDisplay_SetMode(pSettings->videoModeNum, 0, 1) )
    {
#line 210
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error setting display mode.\n", 0, 0, 0, 0);
        return 1;
    }

    bOpened = std3D_Open(pSettings->device3DNum);
    if ( !bOpened )
    {
#line 216
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "std3D open failed!!!!.\n", 0, 0, 0, 0);
        return 1;
    }

    JonesDisplay_Close();
    JonesDisplay_Open(pSettings);
    stdDisplay_Refresh(1);
}

int J3DAPI JonesDisplay_Restart(JonesDisplaySettings* pSettings)
{
    JonesDisplay_Close();
    JonesDisplay_Shutdown();
    if ( JonesDisplay_Startup(pSettings) ) {
        return 1;
    }

    return JonesDisplay_Open(pSettings);
}

void J3DAPI JonesDisplay_SetDefaultVideoMode(const StdDisplayEnvironment* pEnv, JonesDisplaySettings* pDisplaySettings)
{
    size_t i;
    StdDisplayInfo* pInfo;
    StdVideoMode currentVideoMode;

    pDisplaySettings->bWindowMode  = 0;
    pDisplaySettings->bDualMonitor = 0;
    pDisplaySettings->bBuffering   = 0;
    pDisplaySettings->bFog         = 1;
    pDisplaySettings->filter       = STD3D_MIPMAPFILTER_BILINEAR;
    pDisplaySettings->geoMode      = RD_GEOMETRY_FULL;
    pDisplaySettings->lightMode    = RD_LIGHTING_GOURAUD;

    for ( i = 0; i < pEnv->numInfos; ++i )
    {
        if ( pEnv->aDisplayInfos[i].displayDevice.bHAL )
        {
            pDisplaySettings->displayDeviceNum = i;
        }
    }

    if ( i >= pEnv->numInfos )
    {
        pDisplaySettings->displayDeviceNum = -1;
        return;
    }

    pInfo = &pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum];
    if ( !pInfo->numDevices )
    {
        pDisplaySettings->displayDeviceNum = -1;
        return;
    }

    // Search for hardware 3D device
    for ( i = 0; i < pInfo->numDevices; ++i )
    {
        if ( strstr(pInfo->aDevices[i].deviceDescription, "HAL") )
        {
            pDisplaySettings->device3DNum = i;
            break;
        }
    }

    currentVideoMode.aspectRatio                    = 1.0;
    currentVideoMode.rasterInfo.width               = 640; // TODO: make 16:9 res
    currentVideoMode.rasterInfo.height              = 480;
    currentVideoMode.rasterInfo.colorInfo.bpp       = 16;  // TODO: make it 24/32 BPP
    currentVideoMode.rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;
    pDisplaySettings->videoModeNum = JonesDevDialog_FindClosestVideoMode(pEnv, &currentVideoMode, pDisplaySettings->displayDeviceNum);
    currentVideoMode = pInfo->aModes[pDisplaySettings->videoModeNum];

    pDisplaySettings->width = currentVideoMode.rasterInfo.width;
    pDisplaySettings->height = currentVideoMode.rasterInfo.height;
    pDisplaySettings->bClearBackBuffer = 0;

    wuRegistry_SaveStr("Display", pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].displayDevice.aDriverName);
    wuRegistry_SaveStr("3D Device", pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].aDevices[pDisplaySettings->device3DNum].deviceDescription);
    wuRegistry_SaveInt("Width", pDisplaySettings->width);
    wuRegistry_SaveInt("Height", pDisplaySettings->height);
    wuRegistry_SaveInt("BPP", pEnv->aDisplayInfos[pDisplaySettings->displayDeviceNum].aModes[pDisplaySettings->videoModeNum].rasterInfo.colorInfo.bpp);
    wuRegistry_SaveInt("Filter", pDisplaySettings->filter);
    wuRegistry_SaveIntEx("InWindow", pDisplaySettings->bWindowMode);
    wuRegistry_SaveIntEx("Dual Monitor", pDisplaySettings->bDualMonitor);
    wuRegistry_SaveInt("Geometry Mode", pDisplaySettings->geoMode);
    wuRegistry_SaveInt("Lighting Mode", pDisplaySettings->lightMode);
}

int J3DAPI JonesDisplay_Open(JonesDisplaySettings* pSettings)
{
    uint32_t screenH, screenW;
    int x0, y0;
    int flags;
    StdVideoMode displayMode;

    if ( JonesDisplay_pCanvas )
    {
        JonesDisplay_Close();
    }

    x0 = 0;
    y0  = 0;
    stdDisplay_GetBackBufferSize(&screenW, &screenH);
    flags = 0;
    JonesDisplay_pCanvas = rdCanvas_New(flags, &stdDisplay_backBuffer, x0, y0, screenW, screenH);
    if ( !JonesDisplay_pCanvas )
    {
        return 1;
    }

    sithRender_SetRenderFlags(sithRender_GetRenderFlags() | RDROID_USE_AMBIENT_CAMERA_LIGHT);

    rdSetGeometryMode(pSettings->geoMode);
    sithRender_SetLightingMode(pSettings->lightMode);

    
    if ( stdDisplay_GetCurrentVideoMode(&displayMode) )
    {
        return 1;
    }

    sithCamera_Open(JonesDisplay_pCanvas, displayMode.aspectRatio);
    return 0;
}

void J3DAPI JonesDisplay_Close(void)
{
    sithCamera_Close();
    if ( JonesDisplay_pCanvas )
    {
        rdCanvas_Free(JonesDisplay_pCanvas);
    }

    JonesDisplay_pCanvas = NULL;
}

void J3DAPI JonesDisplay_EnableDualMonitor(int bEnable)
{
    JonesDisplay_bDualMonitor = bEnable;
}

int J3DAPI JonesDisplay_UpdateDualScreenWindowSize(const JonesDisplaySettings* pSettings)
{
    if ( pSettings->bWindowMode && JonesDisplay_bDualMonitor && JonesDisplay_primaryDisplayNum > 0 )
    {
        wkernel_SetWindowSize(200, 20);
        return 1;
    }

    return 0;
}

/* Discarded by the retail linker (no caller in either image); the debug build has it at 0x00416377, between
   UpdateDualScreenWindowSize and OpenLoadScreen. Name inferred. */
void J3DAPI JonesDisplay_SetBackBufferSize(uint32_t width, uint32_t height)
{
    JonesDisplay_backBufferWidth  = width;
    JonesDisplay_backBufferHeight = height;
}

void J3DAPI JonesDisplay_OpenLoadScreen(const char* pMatFilePath, float wlStartX, float wlStartY, float wlEndX, float wlEndY, int bPrimaryMusicTheme)
{
    switch ( JonesDisplay_openLoadScreenCounter )
    {
        case 0:
        case 1:
        case 2:
            ++JonesDisplay_openLoadScreenCounter;
            break;

        default: // Greater than 2 is when loading saved game within existing game
            sithSoundMixer_StopAll();
            break;
    }

    JonesDisplay_pWallpaper = rdWallpaper_New(pMatFilePath);
    JonesDisplay_pWallLine  = rdWallpaper_NewWallLine(wlStartX, wlStartY, wlEndX, wlEndY, &JonesDisplay_wallLineColor);

    sithWorld_SetLoadProgressCallback(JonesDisplay_UpdateLoadProgress);
    JonesDisplay_bPrimaryLoadMusic = bPrimaryMusicTheme;
    JonesDisplay_bUpdateLoadScreen = 0;
    sithTime_Reset();
}

void J3DAPI JonesDisplay_CloseLoadScreen(void)
{
    if ( JonesDisplay_pWallpaper ) {
        rdWallpaper_Free(JonesDisplay_pWallpaper);
    }
    JonesDisplay_pWallpaper = NULL;

    if ( JonesDisplay_pWallLine ) {
        rdWallpaper_FreeWallLine(JonesDisplay_pWallLine);
    }
    JonesDisplay_pWallLine = NULL;
}

void J3DAPI JonesDisplay_UpdateLoadProgress(float progress)
{
    size_t msecCurTime;
    size_t msecEndUpdate;
    int i;

    std3D_ClearZBuffer();
    rdCache_AdvanceFrame();
    sithTime_Advance();
    sithSoundMixer_Update();

    switch ( JonesDisplay_openLoadScreenCounter )
    {
    case 1: // loading static world
        progress = progress / 2.0f;
        break;
    case 2: // loading normal world
        progress = D3DVAL(progress / 2.0f) + 50.0f;
        break;
    }

    if ( !std3D_StartScene() )
    {
        // Play loading music
        if ( !JonesDisplay_hSndLoadMusic && progress < 50.0f && !JonesDisplay_hSndChannelLoadMusic )
        {
            if ( JonesDisplay_bPrimaryLoadMusic )
            {
                JonesDisplay_hSndLoadMusic = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(0));
            }
            else
            {
                JonesDisplay_hSndLoadMusic = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(1));
            }

            if ( JonesDisplay_hSndLoadMusic )
            {
                sithSoundMixer_StopAll();
                JonesDisplay_hSndChannelLoadMusic = sithSoundMixer_PlaySound(JonesDisplay_hSndLoadMusic, 1.0f, 0.0, SOUNDPLAY_LOOP);
            }
        }

        // Draw background images
        if ( JonesDisplay_pWallpaper )
        {
            if ( progress == 100.0f && (!JonesDisplay_loadScreenState || JonesDisplay_loadScreenState > 0 && JonesDisplay_bUpdateLoadScreen) )
            {
                // Progress completed, increment background image mats cells to show load completed state
                for ( i = 0; i < 6; ++i )
                {
                    if ( JonesDisplay_pWallpaper->aMatCelNums[i] < (int)JonesDisplay_pWallpaper->aMaterials[i].numCels - 1 )
                    {
                        ++JonesDisplay_pWallpaper->aMatCelNums[i];
                    }
                }
            }

            rdWallpaper_Draw(JonesDisplay_pWallpaper);
        }

        // Draw progress bar line
        if ( JonesDisplay_pWallLine )
        {
            rdWallpaper_DrawWallLine(JonesDisplay_pWallLine, progress);

            if ( progress == 100.0f && (!JonesDisplay_loadScreenState || JonesDisplay_loadScreenState > 0 && JonesDisplay_bUpdateLoadScreen) )
            {
                // Loading completed
                msecCurTime = stdPlatform_GetTimeMsec();
                msecEndUpdate = msecCurTime + 250;

                JonesDisplay_bUpdateLoadScreen = 0;

                if ( ++JonesDisplay_loadScreenState == 1 )
                {
                    if ( JonesDisplay_hSndChannelLoadMusic )
                    {
                        if ( (Sound_GetChannelFlags(JonesDisplay_hSndChannelLoadMusic) & SOUND_CHANNEL_PLAYING) != 0 )
                        {
                            sithSoundMixer_StopSound(JonesDisplay_hSndChannelLoadMusic);
                        }

                        JonesDisplay_hSndChannelLoadMusic = 0;
                        JonesDisplay_hSndLoadMusic = 0;
                    }

                    // Play load finish sound fx
                    JonesDisplay_hSndLoadMusic = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(55)); // 55 - alt_arrive.wav
                    if ( JonesDisplay_hSndLoadMusic )
                    {
                        sithSoundMixer_PlaySound(JonesDisplay_hSndLoadMusic, 1.0f, 0.0, SOUNDPLAY_PLAYONCE);
                    }

                    msecEndUpdate = msecCurTime + 250;
                }

                if ( JonesDisplay_loadScreenState == 2 )
                {
                    msecEndUpdate = msecCurTime + 1000;
                }

                rdCache_Flush();
                rdCache_FlushAlpha();
                std3D_EndScene();

                while ( stdPlatform_GetTimeMsec() < msecEndUpdate )
                {
                    stdDisplay_Update();
                    JonesDisplay_UpdateLoadProgress(progress);
                }

                if ( JonesDisplay_loadScreenState < 3 )
                {
                    JonesDisplay_bUpdateLoadScreen = 1;
                    rdCache_Flush();
                    rdCache_FlushAlpha();
                    stdDisplay_Update();
                    JonesDisplay_UpdateLoadProgress(progress); // Update to show background image in load completed state
                    return;
                }
                else
                {
                    JonesDisplay_loadScreenState = 0;
                    JonesDisplay_bPrimaryLoadMusic = 0;
                    JonesDisplay_dword_553328 = 0;          // ???
                    sithSoundMixer_StopAll();
                    JonesDisplay_hSndLoadMusic = 0;
                    JonesDisplay_hSndChannelLoadMusic = 0;
                    JonesDisplay_dword_55332C = 0;          // ???

                    rdCache_Flush();
                    rdCache_FlushAlpha();
                    stdDisplay_Update();
                    return;
                }
            }
        }

        rdCache_Flush();
        rdCache_FlushAlpha();
    }

    std3D_EndScene();
    stdDisplay_Update();
}

/* Discarded by the retail linker (no caller in either image); the debug build's last JonesDisplay function
   (0x004167CC): the demo build's splash screen, left on any of the listed keys, or restarting the game on R.
   Name inferred. */
int J3DAPI JonesDisplay_ShowSplashDemo(void)
{
    rdWallpaper* pWallpaper;
    int bExitLoop;
    int bKeyPressed;

    pWallpaper = rdWallpaper_New("mat\\splashdemo");
    bExitLoop      = 0;
    sithSoundMixer_StopAll();
    JonesDisplay_hSndLoadMusic = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(2));
    if ( JonesDisplay_hSndLoadMusic )
    {
        sithSoundMixer_PlaySound(JonesDisplay_hSndLoadMusic, 1.0f, 0.0f, 0);
    }

    while ( !bExitLoop && pWallpaper )
    {
        stdControl_ReadControls();
        if ( stdControl_ReadKey(DIK_ESCAPE, &bKeyPressed)
            || stdControl_ReadKey(DIK_SPACE, &bKeyPressed)
            || stdControl_ReadKey(DIK_F1, &bKeyPressed)
            || stdControl_ReadKey(DIK_F2, &bKeyPressed)
            || stdControl_ReadKey(DIK_F3, &bKeyPressed)
            || stdControl_ReadKey(DIK_F4, &bKeyPressed)
            || stdControl_ReadKey(DIK_F5, &bKeyPressed)
            || stdControl_ReadKey(DIK_F6, &bKeyPressed)
            || stdControl_ReadKey(DIK_F7, &bKeyPressed)
            || stdControl_ReadKey(DIK_F8, &bKeyPressed)
            || stdControl_ReadKey(DIK_F9, &bKeyPressed)
            || stdControl_ReadKey(DIK_F10, &bKeyPressed)
            || stdControl_ReadKey(DIK_F11, &bKeyPressed)
            || stdControl_ReadKey(DIK_F12, &bKeyPressed)
            || stdControl_ReadKey(DIK_RETURN, &bKeyPressed)
            || stdControl_ReadKey(DIK_NUMPADENTER, &bKeyPressed)
            || stdControl_ReadKey(DIK_RCONTROL, &bKeyPressed)
            || stdControl_ReadKey(DIK_LCONTROL, &bKeyPressed)
            || stdControl_ReadKey(DIK_RMENU, &bKeyPressed)
            || stdControl_ReadKey(DIK_LMENU, &bKeyPressed)
            || stdControl_ReadKey(DIK_Q, &bKeyPressed)
            || stdControl_ReadKey(DIK_X, &bKeyPressed)
            || stdControl_ReadKey(DIK_W, &bKeyPressed) )
        {
            bExitLoop = 1;
        }
        else if ( stdControl_ReadKey(DIK_R, &bKeyPressed) )
        {
            JonesMain_RestartGame();
            if ( pWallpaper )
            {
                rdWallpaper_Free(pWallpaper);
                pWallpaper = NULL;
            }

            return 0;
        }

        std3D_ClearZBuffer();
        rdCache_AdvanceFrame();
        if ( !std3D_StartScene() )
        {
            rdWallpaper_Draw(pWallpaper);
            rdCache_Flush();
            rdCache_FlushAlpha();
            std3D_EndScene();
            stdDisplay_Update();
        }
    }

    if ( pWallpaper )
    {
        rdWallpaper_Free(pWallpaper);
        pWallpaper = NULL;
    }

    return 1;
}
