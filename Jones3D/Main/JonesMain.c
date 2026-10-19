/* The C runtime headers come first, ahead of JonesMain.h (and the <Windows.h>
   it opens): the unit calls sprintf, fopen, exit and the string functions.
   This include order is release-chosen by measurement, not read from an
   image: it fixes the declaration ordinals the retail object shows. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>





#include "JonesMain.h"
#include <pbt.h>
#include <Jones3D/Display/JonesHudConstants.h>
#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Display/JonesDisplay.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Gui/JonesDialog.h>
#include <Jones3D/Main/JonesFile.h>
#include <Jones3D/Main/JonesLevel.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesCog.h>
#include <Jones3D/Play/JonesControl.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/DSS/sithDSS.h>
#include <sith/DSS/sithGamesave.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include <smush/SmushPlay.h>

#include <sound/Driver.h>
#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdCircBuf.h>
#include <std/General/stdEffect.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdConsole.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <w32util/wuRegistry.h>
#include <wkernel/wkernel.h>

#include <j3dcore/types.h>






#define JONES_QUICKSAVE_TEXTSHOWTIME    1000u // 1 sec
#define JONES_QUICKSAVE_TEXTSHOWTIMERID 1u 

#define JONES_FPSPRINT_INTERVAL  2000u // 2 sec


const JonesLevelInfo JonesMain_aCndLevelLoadInfos[JONESLEVEL_NUMLEVELS] =
{
    {
        NULL,
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 366.0f, 221.0f },
        0,
        NULL
    },
    {
        "00_CYN.cnd",
        "mat\\cyn_amap",
        { -5.0f, 187.0f },
        { 345.0f, 344.0f },
        1,
        "CANYONLANDS"
    },
    {
        "01_BAB.cnd",
        "mat\\bab_amap",
        { -5.0f, 163.0f },
        { 419.0f, 251.0f },
        1,
        "BABYLON"
    },
    {
        "02_RIV.cnd",
        "mat\\riv_amap",
        { -5.0f, 344.0f },
        { 423.0f, 130.0f },
        0,
        "TIAN SHAN RIVER"
    },
    {
        "03_SHS.cnd",
        "mat\\shs_amap",
        { 150.0f, 512.0f },
        { 397.0f, 274.0f },
        0,
        "SHAMBALA SANCTUARY"
    },
    {
        "05_LAG.cnd",
        "mat\\lag_amap",
        { -5.0f, 147.0f },
        { 435.0f, 380.0f },
        1,
        "PALAWAN LAGOON"
    },
    {
        "06_VOL.cnd",
        "mat\\vol_amap",
        { -5.0f, 184.0f },
        { 330.0f, 269.0f },
        0,
        "PALAWAN VOLCANO"
    },
    {
        "07_TEM.cnd",
        "mat\\tem_amap",
        { -5.0f, 127.0f },
        { 349.0f, 312.0f },
        0,
        "PALAWAN TEMPLE"
    },
    {
        "16_JEP.cnd",
        "mat\\jep_amap",
        { -5.0f, 223.0f },
        { 370.0f, 309.0f },
        1,
        "JEEP"
    },
    {
        "08_TEO.cnd",
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 365.0f, 221.0f },
        1,
        "TEOTIAACAN"
    },
    {
        "09_OLV.cnd",
        "mat\\olv_amap",
        { -5.0f, 277.0f },
        { 413.0f, 257.0f },
        0,
        "OLMEC VALLEY"
    },
    {
        "10_SEA.cnd",
        "mat\\sea_amap",
        { -5.0f, 252.0f },
        { 380.0f, 199.0f },
        0,
        "PUDOVKIN SHIP"
    },
    {
        "11_PYR.cnd",
        "mat\\pyr_amap",
        { -5.0f, 243.0f },
        { 421.0f, 391.0f },
        1,
        "MEROE PYRAMIDS"
    },
    {
        "12_SOL.cnd",
        "mat\\sol_amap",
        { -5.0f, 208.0f },
        { 348.0f, 266.0f },
        1,
        "SOLOMON'S MINES"
    },
    {
        "13_NUB.cnd",
        "mat\\nub_amap",
        { -5.0f, 237.0f },
        { 383.0f, 245.0f },
        0,
        "NUB'S TOMB"
    },
    {
        "14_INF.cnd",
        "mat\\inf_amap",
        { 85.0f, 485.0f },
        { 395.0f, 224.0f },
        0,
        "INFERNAL MACHINE"
    },
    {
        "15_AET.cnd",
        "mat\\aet_amap",
        { 329.0f, -5.0f },
        { 329.0f, 316.0f },
        1,
        "AETHERIUM"
    },
    {
        "17_PRU.cnd",
        "mat\\pru_amap",
        { 565.0f, -5.0f },
        { 176.0f, 341.0f },
        1,
        "RETURN TO PERU"
    }
};

const JonesLevelInfo JonesMain_aNdyLevelLoadInfos[JONESLEVEL_NUMLEVELS] =
{
    {
        NULL,
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 366.0f, 221.0f },
        1,
        NULL
    },
    {
        "00_CYN.ndy",
        "mat\\cyn_amap",
        { -5.0f, 187.0f },
        { 345.0f, 344.0f },
        1,
        "CANYONLANDS"
    },
    {
        "01_BAB.ndy",
        "mat\\bab_amap",
        { -5.0f, 163.0f },
        { 419.0f, 251.0f },
        1,
        "BABYLON"
    },
    {
        "02_RIV.ndy",
        "mat\\riv_amap",
        { -5.0f, 344.0f },
        { 423.0f, 130.0f },
        1,
        "TIAN SHAN RIVER"
    },
    {
        "03_SHS.ndy",
        "mat\\shs_amap",
        { 150.0f, 512.0f },
        { 397.0f, 274.0f },
        1,
        "SHAMBALA SANCTUARY"
    },
    {
        "05_LAG.ndy",
        "mat\\lag_amap",
        { -5.0f, 147.0f },
        { 435.0f, 380.0f },
        1,
        "PALAWAN LAGOON"
    },
    {
        "06_VOL.ndy",
        "mat\\vol_amap",
        { -5.0f, 184.0f },
        { 330.0f, 267.0f },
        1,
        "PALAWAN VOLCANO"
    },
    {
        "07_TEM.ndy",
        "mat\\tem_amap",
        { -5.0f, 127.0f },
        { 349.0f, 312.0f },
        1,
        "PALAWAN TEMPLE"
    },
    {
        "16_JEP.ndy",
        "mat\\jep_amap",
        { -5.0f, 223.0f },
        { 370.0f, 308.0f },
        1,
        "JEEP"
    },
    {
        "08_TEO.ndy",
        "mat\\teo_amap",
        { -5.0f, 358.0f },
        { 365.0f, 221.0f },
        1,
        "TEOTIAACAN"
    },
    {
        "09_OLV.ndy",
        "mat\\olv_amap",
        { -5.0f, 277.0f },
        { 413.0f, 257.0f },
        1,
        "OLMEC VALLEY"
    },
    {
        "10_SEA.ndy",
        "mat\\sea_amap",
        { -5.0f, 252.0f },
        { 380.0f, 199.0f },
        1,
        "PUDOVKIN SHIP"
    },
    {
        "11_PYR.ndy",
        "mat\\pyr_amap",
        { -5.0f, 243.0f },
        { 421.0f, 391.0f },
        1,
        "MEROE PYRAMIDS"
    },
    {
        "12_SOL.ndy",
        "mat\\sol_amap",
        { -5.0f, 208.0f },
        { 348.0f, 266.0f },
        1,
        "SOLOMON'S MINES"
    },
    {
        "13_NUB.ndy",
        "mat\\nub_amap",
        { -5.0f, 237.0f },
        { 383.0f, 245.0f },
        1,
        "NUB'S TOMB"
    },
    {
        "14_INF.ndy",
        "mat\\inf_amap",
        { 85.0f, 485.0f },
        { 395.0f, 224.0f },
        1,
        "INFERNAL MACHINE"
    },
    {
        "15_AET.ndy",
        "mat\\aet_amap",
        { 327.0f, -5.0f },
        { 327.0f, 316.0f },
        1,
        "AETHERIUM"
    },
    {
        "17_PRU.ndy",
        "mat\\pru_amap",
        { 565.0f, -5.0f },
        { 176.0f, 340.0f },
        1,
        "RETURN TO PERU"
    }
};

int J3DAPI JonesMain_ProcessGame(void);
static JonesMainProcessFunc JonesMain_pfProcess = JonesMain_ProcessGame;

char JonesMain_aErrorBuffer[1024] = "Unknown error";

// .bss in retail address order (0x005551B8-0x005555BC): zero-initialised
// definitions are laid out in definition order.
static size_t JonesMain_curLevelNum = 0;
static int JonesMain_curGamesaveState = 0;
static size_t JonesMain_aToggleMenuKeyIds[JONESCONTROL_ACTION_MAXBINDS] = { 0 };
static int JonesMain_introVideoMode = 0;
static JonesState JonesMain_state = { 0 };
static int JonesMain_aMessageData[2] = { 0 }; // Referenced only by the discarded SetMessageData (debug 0x5E40F8); name and type inferred
static tHostServices JonesMain_hs = { 0 };
static int JonesMain_bNoProcessWndEvents = 0;
static tCircularBuffer JonesMain_circBuf = { 0 };
static FILE* JonesMain_pLogFile = NULL;
static int JonesMain_unused_5555B0 = 0; // Unreferenced in retail; layout only
static int JonesMain_unused_5555B4 = 0; // Unreferenced in retail; layout only
HANDLE JonesMain_mainMutex = NULL;
static int JonesMain_bPrintFramerate = 0;
static int JonesMain_bWndMsgProcessed = 0;
static int JonesMain_bGamePaused = 0;
static int JonesMain_bStartup = 0;
static char JonesMain_aNdsFilename[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
static StdDisplayEnvironment* JonesMain_pDisplayEnv = NULL;
static int JonesMain_bRefreshDisplayDevice = 0;
static int JonesMain_bSystemSuspended = 0;
static int JonesMain_bSkipIntro = 0;
static uint8_t* JonesMain_aIntroMovieColorTable = NULL;
static int JonesMain_bPrintQuickSave = 0;
static int JonesMain_bAppActivated = 0;
static JonesMainProcessFunc JonesMain_pfCurProcess = NULL;
static int JonesMain_unused_555670 = 0; // Unreferenced in retail; layout only
static int JonesMain_bEndCredits = 0;
static tSoundHandle JonesMain_hSndCredits = 0;
static tSoundChannelHandle JonesMain_hCreditsMusic = 0;
static int JonesMain_bMenuToggled = 0;
static int JonesMain_bVideoModeError = 0;
static float JonesMain_frameRate = 0.0f;
static int JonesMain_frameCount = 0;
static int JonesMain_prevFrameCount = 0;
static uint32_t JonesMain_frameTime = 0;
static uint32_t JonesMain_prevFrameTime = 0;
static int JonesMain_unused_555560 = 0; // Unreferenced in retail; layout only
static int JonesMain_bAssertTriggered = 0;
StdVideoMode JonesMain_curVideoMode = { 0 };
StdDisplayEnvironment* JonesMain_pStartupDisplayEnv = NULL;

static void J3DAPI JonesMain_PrintQuickSave(void);
static void J3DAPI JonesMain_OnAppActivate(HWND hWnd, int bActivated);

int J3DAPI JonesMain_ProcessWindowEvents(void);
int J3DAPI JonesMain_ProcessGame(void);
void J3DAPI JonesMain_ProcessMenu(void);

void J3DAPI JonesMain_PrintFramerate(void);

int J3DAPI JonesMain_EnsureLevelFile(const char* pFilename);
int J3DAPI JonesMain_Restore(const char* pNdsFilePath);

int J3DAPI JonesMain_ProcessStartGame(void);
int J3DAPI JonesMain_ProcessCredits(void);

int J3DAPI JonesMain_LogError(const char* pFormat, ...);
int J3DAPI JonesMain_NoLog(const char* pFormat, ...);
int J3DAPI JonesMain_FilePrintf(const char* pFormat, ...);

J3DNORETURN void J3DAPI JonesMain_Assert(const char* pErrorText, const char* pSrcFile, int line);

int J3DAPI JonesMain_IntroWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pRetValue);
void J3DAPI JonesMain_IntroHandleWMLButtonUp(HWND hwnd, int curPosX, int curPosY, WPARAM mvk);
void J3DAPI JonesMain_IntroHandleWMKeydown(HWND hwnd, WPARAM vk, int a3, int repreatCount, int exkeyflags);
int J3DAPI JonesMain_IntroMovieBlt565(const SmushBitmap* pBitmap, int a2);
int J3DAPI JonesMain_IntroMovieBlt555(const SmushBitmap* pBitmap, int a2);
int J3DAPI JonesMain_IntroMovieBlt32(const SmushBitmap* pBitmap, int a2);

void J3DAPI JonesMain_InitializeHUD(void);
int J3DAPI JonesMain_SaveHUD(DPID idTo, unsigned int outstream);
int J3DAPI JonesMain_RestoreHUD(const SithMessage* pMsg);

void J3DAPI JonesDevDialog_LoadSettings(StdDisplayEnvironment* pDisplayEnv, JonesState* pConfig);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    JonesMain_mainMutex = CreateMutex(NULL, TRUE, "INDY3D");
    if ( GetLastError() == ERROR_ALREADY_EXISTS )
    {
        CloseHandle(JonesMain_mainMutex);
        return -1;
    }

    return wkernel_Run(hInstance, hPrevInstance, lpCmdLine, nShowCmd, "Jones 3D");
}

static void J3DAPI JonesMain_HandleWMGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pMinMaxInfo)
{
    uint32_t bufWidth, height;
    int cxFrame;
    int frameH;
    RECT rectWnd;

    GetWindowRect(hwnd, &rectWnd);

    cxFrame = 2 * GetSystemMetrics(SM_CXFRAME);
    frameH = 2 * GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CYMENU);

    stdDisplay_GetBackBufferSize(&bufWidth, &height);
    if ( bufWidth && height )
    {
        rectWnd.top = rectWnd.left = 0;
        rectWnd.right  = cxFrame + bufWidth;
        rectWnd.bottom = frameH + height;
    }

    pMinMaxInfo->ptMaxSize.x = rectWnd.right - rectWnd.left;
    pMinMaxInfo->ptMaxSize.y = rectWnd.bottom - rectWnd.top;

    pMinMaxInfo->ptMinTrackSize.x = rectWnd.right - rectWnd.left;
    pMinMaxInfo->ptMinTrackSize.y = rectWnd.bottom - rectWnd.top;
    pMinMaxInfo->ptMaxTrackSize.x = rectWnd.right - rectWnd.left;
    pMinMaxInfo->ptMaxTrackSize.y = rectWnd.bottom - rectWnd.top;

    JonesMain_bWndMsgProcessed = 1;
}

static void J3DAPI JonesMain_HandleWMPaint(HWND hWnd)
{
    HDC hdc;
    POINT pt;

    if ( JonesMain_pfProcess == JonesMain_ProcessWindowEvents )
    {
        PAINTSTRUCT ps;
        pt.x = 0;
        pt.y = 0;
        hdc = BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);
        if ( hdc )
        {
            ClientToScreen(hWnd, &pt);
            JonesDialog_RestoreBackground(hdc, hWnd, &pt, &ps.rcPaint);
            EndPaint(hWnd, &ps);
        }
    }
}

static void J3DAPI JonesMain_HandleWMTimer(HWND hWnd, WPARAM timerID)
{
    // TODO: If JonesMain_bWndMsgProcessed will be used by main wnd proc than it should be set to true here
    JonesMain_bPrintQuickSave = 0;
    KillTimer(hWnd, JONES_QUICKSAVE_TEXTSHOWTIMERID);
}

static void J3DAPI JonesMain_HandleWMKeydown(HWND hWnd, WPARAM vk, int a3, int repreatCount, int exkeyflags)
{
    switch ( vk )
    {
        case VK_F5:// Quick save
        {
            char aFilename[128];
            char aPath[128];
            tSoundHandle hSnd;

            if ( repreatCount > 1 || (exkeyflags & KF_REPEAT) != 0 )
            {
                return;
            }

            if ( jonesCog_bEnableGamesave && !jonesCog_bMenuVisible && !sithWorld_bLoading )
            {
                stdUtil_Format(aFilename, STD_ARRAYLEN(aFilename), "%s.%s", sithGetQuickSaveFilePrefix(), "nds");
                STD_MAKEPATH(aPath, sithGetSaveGamesDir(), aFilename);

                JonesHud_RestoreTreasuresStatistics();

                // Note, JonesMain_ProcessGamesaveState shows error message box in case of game save error
                if ( !sithGamesave_Save(aPath, 1) )
                {
                    // Success
                    hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(79));  // 79 - inv_quicksave.wav
                    sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, (SoundPlayFlag)0);

                    // Print quicksave text to screen and set text hide timer
                    JonesMain_bPrintQuickSave = 1;
                    JonesMain_PrintQuickSave();
                    SetTimer(hWnd, JONES_QUICKSAVE_TEXTSHOWTIMERID, JONES_QUICKSAVE_TEXTSHOWTIME, NULL); // 1 sec
                }
            }
        } break;

        case VK_F8: // Quick load
        {
            char aFilename[128];
            char aPath[128];
            FILE* pFile;
            int result;

            if ( repreatCount > 1 || (exkeyflags & KF_REPEAT) != 0 )
            {
                return;
            }

            if ( jonesCog_bEnableGamesave && !jonesCog_bMenuVisible && !sithWorld_bLoading )
            {
                pFile = NULL;
                jonesCog_bEnableGamesave = 0;

                stdUtil_Format(aFilename, STD_ARRAYLEN(aFilename), "%s.%s", sithGetQuickSaveFilePrefix(), "nds");
                STD_MAKEPATH(aPath, sithGetSaveGamesDir(), aFilename);

                pFile = fopen(aPath, "r");
                if ( !pFile )
                {
                    jonesCog_bEnableGamesave = 1;
                    return;
                }

                fclose(pFile); // Looks like the file exists, so we can close it here

                result = sithGamesave_LoadLevelFilename(aPath, NULL);
                if ( result )
                {
                    // Error loading, try load last save file
                    strncpy(aPath, sithGamesave_GetLastFilename(), STD_ARRAYLEN(aPath) - 1);
                    aPath[STD_ARRAYLEN(aPath) - 1] = 0;

                    result = sithGamesave_LoadLevelFilename(aPath, NULL);
                    if ( result )
                    {
                        const char* pErrorFormat = jonesString_GetString("JONES_STR_LOADERROR");
                        if ( pErrorFormat )
                        {
                            char aErrorText[128];

                            memset(aErrorText, 0, sizeof(aErrorText));
                            sprintf(aErrorText, pErrorFormat, aPath);
                            JonesMain_LogErrorToFile(aErrorText);
                            jonesCog_bEnableGamesave = 1;
                        }

                        return;
                    }
                }

                if ( sithCamera_pCurCamera )
                {
                    sithCamera_ResetAllCameras();
                }

                result = sithGamesave_Restore(aPath, /*bNotifyCog*/1);
                if ( result )
                {
                    // Error restoring
                    const char* pErrorFormat = jonesString_GetString("JONES_STR_LOADERROR");
                    if ( pErrorFormat )
                    {
                        char aErrorText[128];

                        memset(aErrorText, 0, sizeof(aErrorText));
                        sprintf(aErrorText, pErrorFormat, aPath);
                        JonesMain_LogErrorToFile(aErrorText);
                    }

                    JonesMain_CloseWindow();
                }
                else
                {
                    // Success
                    JonesHud_RestoreGameStatistics();
                }
            }
        } break;

        case VK_F12: // screenshot
        {
            if ( repreatCount > 1 || (exkeyflags & KF_REPEAT) != 0 )
            {
                return;
            }

            sithMakeDirs();
            sithRender_MakeScreenShot();
        } break;
    }
}

static void J3DAPI JonesMain_PrintQuickSave(void)
{
    const char* pText = NULL;
    const rdFont* pFont;

    pFont = sithVoice_GetTextFont();
    if ( !JonesMain_bPrintQuickSave || !pFont )
    {
        return;
    }

    pText = jonesString_GetString("JONES_STR_QUICKSAVE");
    if ( pText )
    {
        // Retail divides by a `double` 480.0 here rather than calling
        // rdFont_GetNormY (whose RD_REF_HEIGHT is a float).
        float posY = D3DVAL(0.5 - pFont->fontSize / 480.0);

        rdFontColor fontColor;
        rdVector_Set4(&fontColor[0], 1.0f, 0.0f, 0.0f, 1.0f);
        rdVector_Set4(&fontColor[1], 1.0f, 0.0f, 0.0f, 1.0f);
        rdVector_Set4(&fontColor[2], 1.0f, 1.0f, 0.0f, 1.0f);
        rdVector_Set4(&fontColor[3], 1.0f, 1.0f, 0.0f, 1.0f);

        rdFont_SetFontColor(fontColor);
        rdFont_DrawTextLine(pText, 0.5f, posY, rdCamera_pCurCamera->pFrustum->nearPlane, pFont, 1);
    }
}

static int J3DAPI JonesMain_HandleWMActivateApp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    // TODO: If JonesMain_bWndMsgProcessed will be used by main wnd proc than it should be set to true here
    JonesMain_OnAppActivate(hWnd, wParam);
    return DefWindowProc(hWnd, WM_ACTIVATEAPP, wParam, lParam);
}

static LRESULT J3DAPI JonesMain_HandleWMActivate(HWND hWnd, WPARAM activateState, LPARAM hwndActivate, int minimizedState)
{
    // TODO: If JonesMain_bWndMsgProcessed will be used by main wnd proc than it should be set to true here
    JonesMain_OnAppActivate(hWnd, activateState != WA_INACTIVE);
    return DefWindowProc(hWnd, WM_ACTIVATE, MAKEWPARAM(activateState, minimizedState), hwndActivate);
}

static void J3DAPI JonesMain_OnAppActivate(HWND hWnd, int bActivated)
{
    int result;
    if ( !JonesMain_pfCurProcess )
    {
        JonesMain_pfCurProcess = JonesMain_pfProcess;
    }

    if ( bActivated )
    {
        if ( !JonesMain_bAppActivated )
        {
            if ( !JonesMain_state.displaySettings.bWindowMode )
            {
                ShowWindow(hWnd, SW_SHOW);
            }

            JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);
            stdDisplay_Refresh(1);
            std3D_ResetTextureCache();

            if ( sithWorld_pCurrentWorld )
            {
                sithWorld_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
            }

            JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);
        }

        if ( JonesMain_pfCurProcess )
        {
            JonesMain_pfProcess = JonesMain_pfCurProcess;
            JonesMain_pfCurProcess = 0;
        }

        JonesMain_bAppActivated = 1;
    }
    else
    {
        JonesMain_pfProcess = JonesMain_ProcessWindowEvents;
        stdDisplay_Refresh(0);
        JonesMain_bAppActivated = 0;
    }

    result = stdControl_SetActivation(bActivated);
    if ( !result )
    {
        if ( bActivated )
        {
            stdControl_ShowMouseCursor(0);
        }
        else
        {
            stdControl_ShowMouseCursor(1);
        }
    }
}

static void J3DAPI JonesMain_HandleWMChar(HWND hwnd, char chr, int repeatCount)
{

    if ( jonesConsole_bVisible )
    {
        JonesConsole_HandelChar(chr);
    }
}

static void J3DAPI JonesMain_HandleWMSysCommand(HWND hWnd, WPARAM wParam, int curPosX, int curPosY)
{

    switch ( wParam )
    {
        case SC_KEYMENU:
        case SC_SCREENSAVE:
            JonesMain_bWndMsgProcessed = 1;
            break;

        default:
            break;
    }
}

static void J3DAPI JonesMain_HandleWMPowerBroadcast(HWND hWnd, WPARAM wParam)
{
    switch ( wParam )
    {
        case PBT_APMQUERYSUSPEND:
#line 688
            stdPrintf(std_pHS->pDebugPrint, J3D_FILE, __LINE__, "WM_POWERBROADCAST Query Suspend.\n", 0, 0, 0, 0);
            break;

        case PBT_APMSUSPEND:
            JonesMain_bSystemSuspended = 1;
            JonesMain_bSkipIntro       = 1;
            JonesMain_OnAppActivate(hWnd, 0);
#line 694
            stdPrintf(std_pHS->pDebugPrint, J3D_FILE, __LINE__, "WM_POWERBROADCAST Suspend.\n", 0, 0, 0, 0);
            break;

        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
            JonesMain_bSystemSuspended = 0;
            JonesMain_bSkipIntro       = 1;
            JonesMain_OnAppActivate(hWnd, 1);
            Sleep(100u);

#line 702
            stdPrintf(std_pHS->pDebugPrint, J3D_FILE, __LINE__, "WM_POWERBROADCAST Resume.\n", 0, 0, 0, 0);
            break;
    }
}

static int J3DAPI JonesMain_GameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pReturnVal)
{
    JonesMain_bWndMsgProcessed = 0;
    switch ( uMsg )
    {
        case WM_CHAR:
            JonesMain_HandleWMChar(hWnd, (char)wParam, (int16_t)lParam);
            return 0;

        case WM_KEYDOWN:
            JonesMain_HandleWMKeydown(hWnd, wParam, 1, (int16_t)lParam, HIWORD(lParam));
            return 0;

        case WM_TIMER:
            JonesMain_HandleWMTimer(hWnd, wParam);
            return 0;

        case WM_ACTIVATE:
            JonesMain_HandleWMActivate(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_ACTIVATEAPP:
            JonesMain_HandleWMActivateApp(hWnd, wParam, lParam);
            return 0;

        case WM_POWERBROADCAST:
            JonesMain_HandleWMPowerBroadcast(hWnd, wParam);
            return lParam;

        case WM_SYSCOMMAND:
            JonesMain_HandleWMSysCommand(hWnd, wParam, (int16_t)lParam, (int16_t)HIWORD(lParam));
            return 0;

        case WM_GETMINMAXINFO:
            JonesMain_HandleWMGetMinMaxInfo(hWnd, (LPMINMAXINFO)lParam);
            return 0;

        case WM_PAINT:
            JonesMain_HandleWMPaint(hWnd);
            return 0;
    }

    if ( JonesMain_bWndMsgProcessed )
    {
        *pReturnVal = 0;
    }

    return JonesMain_bWndMsgProcessed;
}

int J3DAPI JonesMain_Startup(const char* lpCmdLine)
{
    int bSuccess;
    int bPlayIntro;
    int result;

    bPlayIntro = 1;

    JonesMain_bPrintQuickSave = 0;
    JonesMain_curLevelNum     = 0;

    // Setup process routines
    JonesMain_pfProcess = JonesMain_ProcessGame;

    memset(&JonesMain_circBuf, 0, sizeof(JonesMain_circBuf));
    memset(JonesMain_aToggleMenuKeyIds, 0, JONESCONTROL_ACTION_MAXBINDS); // Retail bug: clears 8 bytes, not the whole array

    wkernel_SetWindowProc(JonesMain_GameWndProc);

    // Initialize Host services
    stdPlatform_InitServices(&JonesMain_hs);

    stdStartup(&JonesMain_hs);
    rdSetServices(&JonesMain_hs);
    sithSetServices(&JonesMain_hs);
    sithSound_Initialize(&JonesMain_hs);

    JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);
    JonesFile_Startup(&JonesMain_hs);

    result = wuRegistry_Startup(HKEY_LOCAL_MACHINE, "Software\\LucasArts Entertainment Company LLC\\Indiana Jones and the Infernal Machine\\v1.0");
    if ( result )
    {
        JonesMain_LogErrorToFile("Couldn't open the registry.");
        JonesMain_CloseWindow();
        return 1;
    }

    wuRegistry_GetStr("User Path", JonesMain_state.aInstallPath, STD_ARRAYLEN(JonesMain_state.aInstallPath), "");
    wuRegistry_GetStr("Source Dir", JonesMain_state.aCDPath, STD_ARRAYLEN(JonesMain_state.aCDPath), "");

    std3D_SetFindAllDevices(wuRegistry_GetIntEx("AllDevices", 0));

    JonesFile_Open(&JonesMain_hs, JonesMain_state.aInstallPath, JonesMain_state.aCDPath);

    if ( jonesString_Startup() )
    {
        JonesMain_LogErrorToFile("Could not open game text.");
        JonesMain_CloseWindow();
        return 1;
    }

    result = sithString_Startup();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_SITHERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    JonesMain_pDisplayEnv = std3D_BuildDisplayEnvironment();
    if ( !JonesMain_pDisplayEnv )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_DISPLAYERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    JonesDisplay_EnableDualMonitor(1);
    JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);

    JonesDevDialog_LoadSettings(JonesMain_pDisplayEnv, &JonesMain_state);

    sithSound_StartupSound();
    result = sithSound_Startup(JonesMain_state.soundSettings.b3DHWSupport);
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_SOUNDERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }
    }

    JonesMain_state.soundSettings.maxSoundVolume = wuRegistry_GetFloat("Sound Volume", Sound_GetMaxVolume());
    wuRegistry_SaveFloat("Sound Volume", JonesMain_state.soundSettings.maxSoundVolume);

    Sound_SetMaxVolume(JonesMain_state.soundSettings.maxSoundVolume);
    SmushPlay_SetGlobalVolume((size_t)(JonesMain_state.soundSettings.maxSoundVolume * 127.0f)); // TODO: Can be removed as it's being set in PlayIntroMovie

    // Handle start mode
    switch ( JonesMain_state.startMode )
    {
        case JONES_STARTMODE_STARTGAME:
        {
            sscanf(lpCmdLine, "%d", &JonesMain_curLevelNum);
            if ( JonesMain_curLevelNum < JONESLEVEL_FIRSTLEVELNUM || JonesMain_curLevelNum > JONESLEVEL_NUMLEVELS ) // Retail bug: allows the out-of-range level 18
            {
                JonesMain_curLevelNum = JONESLEVEL_FIRSTLEVELNUM;
            }

            strncpy(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pFilename, sizeof(JonesMain_state.aCurLevelFilename) - 1);
            JonesMain_state.aCurLevelFilename[sizeof(JonesMain_state.aCurLevelFilename) - 1] = 0;
            bSuccess = 1;

        } break; // start game

        case JONES_STARTMODE_LOADGAME:
        {
            bSuccess = jonesConfig_GetLoadGameFilePath(stdWin95_GetWindow(), JonesMain_aNdsFilename);
            if ( bSuccess != 1 )
            {
                return 1;
            }

            // Skip intro video
            bPlayIntro = 0;

        } break; // Start game

        default: // Developer dialog
        {
            bSuccess = JonesDevDialog_ShowDevDialog(stdWin95_GetWindow(), &JonesMain_state);
            if ( bSuccess != 1 )
            {
                return 1; // exit
            }
        } break;

        case JONES_STARTMODE_SOUNDSETTINGS:
        {
            bSuccess = jonesConfig_ShowSoundSettingsDialog(stdWin95_GetWindow(), &JonesMain_state.soundSettings);
            return 1; // exit
        }

        case JONES_STARTMODE_DISPLAYSETTINGS:
        {
            bSuccess = jonesConfig_ShowDisplaySettingsDialog(stdWin95_GetWindow(), JonesMain_pDisplayEnv, &JonesMain_state.displaySettings);
            return 1; // exit
        }
    }

    // Start game

    // Init jones vars

    JonesDisplay_EnableDualMonitor(JonesMain_state.displaySettings.bDualMonitor);

    // Init output mode
    switch ( JonesMain_state.outputMode )
    {
        case JONES_OUTPUTMODE_CONSOLE:
        {
            switch ( JonesMain_state.logLevel )
            {
                case JONES_LOGLEVEL_VERBOSE:
                    JonesMain_hs.pDebugPrint  = stdConsolePrintf;
                    // Fallthrough
                case JONES_LOGLEVEL_NORMAL:
                    JonesMain_hs.pStatusPrint = stdConsolePrintf;
                    // Fallthrough
                case JONES_LOGLEVEL_ERROR:
                    JonesMain_hs.pErrorPrint  = JonesMain_LogError;
                    break;

                default:
                    break; // No logging
            };

            // Startup console
            stdConsole_Startup("Debug", FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN, /*bShowMinimized=*/0); // White text

        } break;

        case JONES_OUTPUTMODE_LOGFILE:
        {
            JonesMain_pLogFile = fopen("JonesLog.txt", "w+"); // TODO: Add check for case when fopen fails
            switch ( JonesMain_state.logLevel )
            {
                case JONES_LOGLEVEL_VERBOSE:
                    JonesMain_hs.pDebugPrint  = JonesMain_FilePrintf;
                    // Fallthrough
                case JONES_LOGLEVEL_NORMAL:
                    JonesMain_hs.pStatusPrint = JonesMain_FilePrintf;
                    // Fallthrough
                case JONES_LOGLEVEL_ERROR:
                    JonesMain_hs.pErrorPrint  = JonesMain_LogError;
                    break;

                default:
                    break; // No logging
            };
        } break;

        default: // JONES_OUTPUTMODE_NONE
        {
            switch ( JonesMain_state.logLevel )
            {
                case JONES_LOGLEVEL_VERBOSE:
                    JonesMain_hs.pDebugPrint = JonesMain_hs.pStatusPrint;
                    // Fallthrough
                case JONES_LOGLEVEL_ERROR:
                    JonesMain_hs.pMessagePrint = JonesMain_NoLog;
                    JonesMain_hs.pStatusPrint  = JonesMain_NoLog;
                    JonesMain_hs.pWarningPrint = JonesMain_NoLog;
                    JonesMain_hs.pDebugPrint   = JonesMain_NoLog;
                    JonesMain_hs.pErrorPrint   = JonesMain_LogError;
                    break;

                default:
                    break; // No logging
            };
        } break;
    }

    // Set assert handler
    JonesMain_hs.pAssert = JonesMain_Assert;
    stdCircBuf_New(&JonesMain_circBuf, 4, 128);

    // Startup modules and load level

#line 998
    stdPrintf(std_pHS->pDebugPrint, J3D_FILE, __LINE__, "Starting up Indiana Jones and the Infernal Machine.\n", 0, 0, 0, 0);

    stdEffect_Startup();
    rdStartup();

    result = sithStartup();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_SITHSTARTERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }


    // Init save game callbacks
    sithSetOpenCallback(JonesMain_InitializeHUD);
    sithGamesave_SetSaveGameCallback(JonesMain_SaveHUD);
    sithMessage_RegisterFunction(SITHDSS_HUDSTATE, JonesMain_RestoreHUD);

    result = JonesConsole_Startup();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CONSOLEERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    result = JonesHud_Startup();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CTRLERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Set performance level
    sithSetPerformanceLevel(JonesMain_state.performanceLevel);

    // Init debug flags
    sith_mode.debugModeFlags = 0;
    if ( JonesMain_state.bDevMode )
    {
        sith_mode.debugModeFlags |= SITHDEBUG_INEDITOR;
    }

    result = JonesDisplay_Startup(&JonesMain_state.displaySettings);
    if ( result )
    {
        // Error starting display module with set video mode, try setting default video mode

        JonesDisplay_SetDefaultVideoMode(JonesMain_pDisplayEnv, &JonesMain_state.displaySettings);
        if ( JonesMain_state.displaySettings.displayDeviceNum == -1 )
        {
            // Error setting default video mode
            char aErrorText[128];
            const char* pErrorText = jonesString_GetString("JONES_STR_NO3DCARD");

            memset(aErrorText, 0, sizeof(aErrorText));
            stdUtil_Format(aErrorText, STD_ARRAYLEN(aErrorText), "%s", pErrorText);

            jonesConfig_ShowMessageDialog(stdWin95_GetWindow(), "JONES_STR_3D_DEVICE", aErrorText, 136);
            JonesMain_CloseWindow();
            return 1;
        }

        // There was no error in setting default mode, lets try starting default module
        result = JonesDisplay_Startup(&JonesMain_state.displaySettings);
        if ( result )
        {
            // Nope it won't budge, we have no option but to quit the game
            const char* pErrorText = jonesString_GetString("JONES_STR_DISPLAYERROR");
            if ( pErrorText )
            {
                JonesMain_LogErrorToFile(pErrorText);
            }

            JonesMain_CloseWindow(); // Quit game
            return 1;
        }
    }

    result = JonesControl_Startup();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CTRLERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    result = JonesConsole_Open();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_CONSOLEERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    result = jonesCog_Startup();
    if ( result )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_COGERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Play intro video
    if ( bPlayIntro && JonesMain_PlayIntroMovie() )
    {
        // Error playing intro movie
        return 1;
    }

    // Finish playing intro video, now load static resource level

    // Set load screen for loading static world
    if ( JonesMain_state.startMode == JONES_STARTMODE_LOADGAME )
    {
        JonesMain_Restore(JonesMain_aNdsFilename);
    }
    else
    {
        JonesMain_UpdateLevelNum();
        JonesDisplay_OpenLoadScreen( // TODO: what if ndy file?
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pMatFilename,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].bPrimaryMusicTheme
        );
    }

    // Load Static level
    {
    char aStaticFilename[64];

    strncpy(aStaticFilename, "Jones3DSTATIC", sizeof(aStaticFilename) - 1);
    aStaticFilename[sizeof(aStaticFilename) - 1] = 0;
    stdFnames_ChangeExt(aStaticFilename, "cnd");

    result = sithOpenStatic(aStaticFilename);
    if ( result )
    {
        stdFnames_ChangeExt(aStaticFilename, "ndy");
        result = sithOpenStatic(aStaticFilename);
        if ( result )
        {
            // Error loading static level
            const char* pErrorText = jonesString_GetString("JONES_STR_STATICERROR");
            if ( pErrorText )
            {
                JonesMain_LogErrorToFile(pErrorText);
            }

            JonesMain_CloseWindow(); // quit game
            return 1;
        }
    }
    }

    // Finished loading static resource level
    JonesDisplay_CloseLoadScreen();

    // Open HUD
    result = JonesHud_Open();
    if ( result )
    {
        // Error opening HUD
        const char* pErrorText = jonesString_GetString("JONES_STR_HUDERROR");
        if ( pErrorText )
        {
            JonesMain_LogErrorToFile(pErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Load level
    result = JonesMain_Open();
    if ( result )
    {
        // Error opening level
        const char* pFormat = jonesString_GetString("JONES_STR_OPENERROR");
        if ( pFormat )
        {
            char aErrorText[128];

            memset(aErrorText, 0, sizeof(aErrorText));
            sprintf(aErrorText, pFormat, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
            JonesMain_LogErrorToFile(aErrorText);
        }

        JonesMain_CloseWindow();
        return 1;
    }

    // Success starting game
    JonesMain_bStartup = 1;
    return 0;
}

void J3DAPI JonesMain_Shutdown(void)
{
    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = NULL;

    wkernel_SetWindowProc(NULL);

    if ( JonesMain_pDisplayEnv )
    {
        std3D_FreeDisplayEnvironment(JonesMain_pDisplayEnv);
        JonesMain_pDisplayEnv = NULL;
    }

    JonesMain_Close();
    JonesHud_Close();
    sithCloseStatic();

    jonesCog_Shutdown();
    JonesControl_Shutdown();
    JonesConsole_Close();

    JonesDisplay_Shutdown();
    JonesDisplay_CloseLoadScreen();

    JonesHud_Shutdown();
    JonesConsole_Shutdown();

    sithString_Shutdown();
    jonesString_Shutdown();

    sithShutdown();
    rdShutdown();

    stdEffect_Shutdown(); // LINK folded its body onto stdEffect_Startup's (both call stdEffect_Reset)
    stdCircBuf_Free(&JonesMain_circBuf);

    switch ( JonesMain_state.outputMode )
    {
        case JONES_OUTPUTMODE_CONSOLE:
            stdConsole_Shutdown();
            break;

        case JONES_OUTPUTMODE_LOGFILE:
            fclose(JonesMain_pLogFile);
            break;

        default:
            break;
    };

    sithSound_Shutdown();
    sithSound_ShutdownSound();

    JonesFile_Close();
    wuRegistry_Shutdown();
    JonesFile_Shutdown();

    stdShutdown();
    sithSound_Uninitialize();
    sithClearServices();
    rdClearServices();
    stdPlatform_ClearServices(&JonesMain_hs);

    JonesMain_bStartup = 0;

    if ( JonesMain_mainMutex )
    {
        CloseHandle(JonesMain_mainMutex);
    }
}

void J3DAPI JonesMain_PauseGame(void)
{
    if ( JonesMain_bGamePaused == 1 )
    {
        return;
    }

    sithTime_Pause();
    Sound_Pause();
    JonesMain_bGamePaused = 1;
}

void J3DAPI JonesMain_ResumeGame(void)
{
    if ( !JonesMain_bGamePaused )
    {
        return;
    }

    sithTime_Resume();
    Sound_Resume();
    JonesMain_bGamePaused = 0;
}

int J3DAPI JonesMain_IsGamePaused(void)
{
    return JonesMain_bGamePaused;
}

int J3DAPI JonesMain_Process(void)
{
    return JonesMain_pfProcess();
}

int J3DAPI JonesMain_ProcessWindowEvents(void)
{
    return wkernel_ProcessEvents();
}

int J3DAPI JonesMain_ProcessGame(void)
{
    int result;

    if ( JonesMain_bSystemSuspended )
    {
        return 0;
    }

    if ( sith_mode.masterMode == SITH_MODE_OPENED )
    {
        JonesMain_ProcessMenu();
        sithUpdate();

        if ( JonesMain_ProcessGamesaveState() )
        {
            return -1;
        }

        // Note functions from ClearZBuffer to and including std3D_StartScene could be part of rdAdvanceFrame
        std3D_ClearZBuffer();
        rdCache_AdvanceFrame();

        // TODO: Refactor and enable viewport clearing in std3D instead
        if ( JonesMain_state.displaySettings.bClearBackBuffer )
        {
            stdDisplay_BackBufferFill(0, 0);
        }

        if ( !std3D_StartScene() )
        {
            sithDrawScene();
            sithOverlayMap_Draw();
            JonesMain_PrintQuickSave();
            JonesHud_Process();

            // below 3 function calls could be part of rdFinishFrame
            rdCache_Flush();
            rdCache_FlushAlpha();
            std3D_EndScene();
        }

        JonesMain_PrintFramerate();
        stdDisplay_Update();
    }

    if ( JonesMain_bRefreshDisplayDevice )
    {
        result = JonesDisplay_Restart(&JonesMain_state.displaySettings);
        if ( result )
        {
            JonesDisplay_SetDefaultVideoMode(JonesMain_pDisplayEnv, &JonesMain_state.displaySettings);
            result = JonesDisplay_Restart(&JonesMain_state.displaySettings);
            if ( result )
            {
                // All attempt to set video mode failed, exit game
                const char* pFormat = jonesString_GetString("JONES_STR_VIDEOERROR");
                if ( pFormat )
                {
                    JonesMain_LogErrorToFile(pFormat);
                }

                JonesMain_CloseWindow(); // exits game
                return -1;
            }

            // Success with default video mode
            JonesMain_bVideoModeError = 1;
        }

        // Success
        JonesMain_bRefreshDisplayDevice = 0;
    }
    else if ( JonesMain_bVideoModeError )
    {
        // Shows message box that there was an error setting video mode in previous frame and
        // was set to default video mode (640 x 480)

        char aNoDisplyError[512];
        char aNoDisply2Error[512];
        const char* curErrorStr2;
        const char* pCurErrorStr = jonesString_GetString("JONES_STR_NODISPLAY");

        if ( pCurErrorStr )
        {
            strcpy(aNoDisplyError, pCurErrorStr);
        }
        else
        {
            memset(aNoDisplyError, 0, sizeof(aNoDisplyError));
        }

        curErrorStr2 = jonesString_GetString("JONES_STR_NODISPLAY2");
        if ( curErrorStr2 )
        {
            strcpy(aNoDisply2Error, curErrorStr2);
        }
        else
        {
            memset(aNoDisply2Error, 0, sizeof(aNoDisply2Error));
        }

        if ( aNoDisplyError[0] && aNoDisply2Error[0] )
        {
            char aErrorText[128];

            memset(aErrorText, 0, sizeof(aErrorText));
            stdUtil_Format(aErrorText, STD_ARRAYLEN(aErrorText), "%s \n %s", aNoDisplyError, aNoDisply2Error);

            jonesConfig_ShowMessageDialog(stdWin95_GetWindow(), "JONES_STR_DPLY_OPTS", aErrorText, 136);
        }

        JonesMain_bVideoModeError = 0;
    }

    return wkernel_PeekProcessEvents();
}

void J3DAPI JonesMain_ProcessMenu(void)
{
    int numPressed;
    int bMenuKeyPressed;
    size_t curi;

    bMenuKeyPressed = 0;
    curi = 0;
    while ( JonesMain_aToggleMenuKeyIds[curi] > 0 )
    {
        if ( stdControl_ReadKey(JonesMain_aToggleMenuKeyIds[curi], &numPressed) )
        {
            bMenuKeyPressed = 1;
            break;
        }

        ++curi;
    }

    bMenuKeyPressed |= stdControl_ReadKey(DIK_ESCAPE, &numPressed);
    if ( bMenuKeyPressed && !JonesMain_bMenuToggled && !stdControl_ReadKey(DIK_RCONTROL, &numPressed) && !stdControl_ReadKey(DIK_LCONTROL, &numPressed) )
    {
        if ( jonesConsole_bVisible )
        {
            JonesConsole_HideConsole();
        }
        else
        {
            JonesHud_ToggleMenu();
            JonesMain_bMenuToggled = 1;
        }
    }
    else if ( !bMenuKeyPressed )
    {
        JonesMain_bMenuToggled = 0;
    }
}

void J3DAPI JonesMain_PrintFramerate(void)
{
    int numPolys;

    ++JonesMain_frameCount;
    JonesMain_frameTime = stdPlatform_GetTimeMsec();
    numPolys = sithRender_numArchPolys + sithRender_numAlphaArchPolys + sithRender_numThingPolys + sithRender_numAlphaThingPoly;

    if ( JonesMain_bPrintFramerate )
    {
        if ( JonesMain_frameTime - JonesMain_prevFrameTime > JONES_FPSPRINT_INTERVAL ) // 2 secs
        {
            JonesMain_frameRate      = D3DDivide((JonesMain_frameCount - JonesMain_prevFrameCount) * 1000.0, JonesMain_frameTime - JonesMain_prevFrameTime);
            JonesMain_prevFrameTime  = JonesMain_frameTime;
            JonesMain_prevFrameCount = JonesMain_frameCount;

            sprintf(std_genBuffer, "%02.2fHz A:%d S:%d P:%d Z:%d",
                JonesMain_frameRate, sithRender_numVisibleAdjoins, sithRender_numVisibleSectors, numPolys, sithRender_numDrawnThings);

            JonesConsole_PrintTextWithID(JONESCONSOLE_FRAMERATEID, std_genBuffer);
#line 1728
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s\n", std_genBuffer, 0, 0, 0);
        }
    }
}

#ifdef J3D_DEBUG
/* Debug-only as far as the evidence goes (debug 0x00422402, between PrintFramerate and TogglePrintFramerate; no caller
   in the debug image, no trace in retail): reports the error buffer and quits. Name from Jones3DRE. */
void J3DAPI JonesMain_FatalErrorUnknown(void)
{
    MessageBox(NULL, JonesMain_aErrorBuffer, "Unable to continue Error", MB_TASKMODAL | MB_ICONHAND);
    JonesMain_Shutdown();
    exit(1);
}
#endif

void J3DAPI JonesMain_TogglePrintFramerate(void)
{
    JonesMain_bPrintFramerate = JonesMain_bPrintFramerate == 0;
}

/* Retail's linker discarded this function (no retail symbol, address or caller). Its evidence is the
   /Gf literal order: retail creates "pMsg" (0x00525060) between PrintFramerate's format and Open's
   "start", although this TU's only surviving user is RestoreHUD, emitted much later. Body from the debug
   build (0x422443, /Od-identical): it stores the message's first int in the 8-byte object
   after JonesMain_state (debug 0x5E40F8, retail 0x005553E8) and reads a second one into a dead local.
   The name is descriptive. */
int J3DAPI JonesMain_SetMessageData(SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    int value;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1764
    STD_ASSERTREL(pMsg);
    if ( (sith_mode.subModeFlags & 8) == 0 )
    {
        return 1;
    }

    JonesMain_aMessageData[0] = SITHDSS_POPINT32();
    value = SITHDSS_POPINT32();
    SITHDSS_ENDIN;
    return 1;
}

int J3DAPI JonesMain_Open(void)
{
    int bNotStartFile = 0;
    int result;

    if ( JonesMain_EnsureLevelFile(JonesMain_state.aCurLevelFilename) )
    {
        return 1;
    }

    // Open load screen
    // Note that internal state of loadscreen should be here > 1, meaning the static world has been loaded
    // so the progress bar starts at 50%
    if ( JonesMain_state.startMode == JONES_STARTMODE_LOADGAME )
    {
        JonesMain_Restore(JonesMain_aNdsFilename);
    }
    else
    {
        JonesMain_UpdateLevelNum();
        JonesDisplay_OpenLoadScreen(
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pMatFilename,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].bPrimaryMusicTheme
        );
    }

    JonesDisplay_UpdateDualScreenWindowSize(&JonesMain_state.displaySettings);

    // Load savegame
    if ( strlen(JonesMain_aNdsFilename) > 0 )
    {
        if ( strncmp("start", JonesMain_aNdsFilename, 5u) == 0 )
        {
            bNotStartFile = 0;
        }
        else
        {
            bNotStartFile = 1;
        }

        result = sithGamesave_Restore(JonesMain_aNdsFilename, 0);
        if ( result )
        {
            // Error
            const  char* pFormat = jonesString_GetString("JONES_STR_LOADERROR");
            if ( pFormat )
            {
                char aErrorText[128];

                memset(aErrorText, 0, sizeof(aErrorText));
                sprintf(aErrorText, pFormat, JonesMain_aNdsFilename);
                JonesMain_LogErrorToFile(aErrorText);
            }

            JonesMain_CloseWindow(); // Note this will exit process
        }
        else
        {
            JonesHud_RestoreGameStatistics();
        }

        if ( JonesDisplay_Open(&JonesMain_state.displaySettings) )
        {
            return 1;
        }

        if ( JonesMain_state.startMode == JONES_STARTMODE_LOADGAME )
        {
            sithGamesave_NotifyRestored(JonesMain_aNdsFilename);
        }

        memset(JonesMain_aNdsFilename, 0, sizeof(JonesMain_aNdsFilename));
    }
    else
    {
        // Load level & open level
        result = sithOpenNormal(JonesMain_state.aCurLevelFilename, JonesMain_state.waPlayerName);
        if ( result )
        {
            // Error opening level
            const  char* pFormat = jonesString_GetString("JONES_STR_OPENERROR");
            if ( pFormat )
            {
                char aErrorText[128];

                memset(aErrorText, 0, sizeof(aErrorText));
                sprintf(aErrorText, pFormat, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
                JonesMain_LogErrorToFile(aErrorText);
            }

            JonesMain_CloseWindow(); // Note this will exit process
        }

        if ( JonesDisplay_Open(&JonesMain_state.displaySettings) )
        {
            return 1;
        }

        sithOpenPostProcess();
    }

    JonesDisplay_CloseLoadScreen();
    return 0;
}

int J3DAPI JonesMain_EnsureLevelFile(const char* pFilename)
{
    size_t cdNum;

    char aPath[128];
    STD_MAKEPATH(aPath, "ndy", pFilename);
    if ( JonesFile_FileExists(aPath) )
    {
        // File found
        return 0;
    }

    // No level file was found

    JonesFile_Close();

    cdNum = JonesFile_GetCurrentCDNum() == 1 ? 2 : 1;
    if ( jonesConfig_ShowDialogInsertCD(stdWin95_GetWindow(), cdNum) != 1 )
    {
        return 1; // exit
    }

    return JonesFile_Open(&JonesMain_hs, JonesMain_state.aInstallPath, JonesMain_state.aCDPath);
}

int J3DAPI JonesMain_Close(void)
{
    JonesDisplay_Close();
    sithClose();
    return 0;
}

int J3DAPI JonesMain_Restore(const char* pNdsFilePath)
{
    if ( !pNdsFilePath )
    {
        return 1;
    }

    {
    int result = sithGamesave_LoadLevelFilename(pNdsFilePath, JonesMain_state.aCurLevelFilename);
    if ( result )
    {
        return result;
    }
    }

    if ( JonesMain_EnsureLevelFile(JonesMain_state.aCurLevelFilename) )
    {
        return 1;
    }

    {
    const char* pNdsFilename = strrchr(pNdsFilePath, '\\');
    if ( !pNdsFilename )
    {
        pNdsFilename = pNdsFilePath;
    }
    else
    {
        ++pNdsFilename;
    }

    {
    char aQSaveFilename[128];

    memset(aQSaveFilename, 0, sizeof(aQSaveFilename));

    JonesMain_UpdateLevelNum();

    sprintf(aQSaveFilename, "%s%s", sithGetAutoSaveFilePrefix(), sithGetCurrentWorldSaveName());

    stdFnames_ChangeExt(aQSaveFilename, "nds");

    // Open load screen
    if ( JonesMain_curLevelNum == 1 && !streq(pNdsFilename, aQSaveFilename) )
    {
        // Note, using cyb_amap instead of cyn_amap
        JonesDisplay_OpenLoadScreen("mat\\cyb_amap", -5.0f, 187.0f, 315.0f, 297.0f, JonesMain_aCndLevelLoadInfos[1].bPrimaryMusicTheme);
    }
    else
    {
        JonesDisplay_OpenLoadScreen(
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pMatFilename,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineStart.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.x,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].wallineEnd.y,
            JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].bPrimaryMusicTheme
        );
    }

    return 0;
}
}
}

int J3DAPI JonesMain_ProcessGamesaveState(void)
{
    const char* pNdsFilename;
    int result;
    char aLoadLerrorStr[512];
    const char* pLoadErrStr;
    const char* pErrorFormat;

    pErrorFormat = pLoadErrStr = jonesString_GetString("JONES_STR_LOADERROR");
    if ( pLoadErrStr )
    {
        strcpy(aLoadLerrorStr, pLoadErrStr);
    }
    else
    {
        memset(aLoadLerrorStr, 0, sizeof(aLoadLerrorStr));
    }

    JonesMain_curGamesaveState = sithGamesave_GetState(&pNdsFilename);
    switch ( JonesMain_curGamesaveState )
    {
        case SITHGAMESAVE_SAVE:
        {
            result = sithGamesave_Process();
            if ( result )
            {
                // Error
                const char* pFormat = jonesString_GetString("JONES_STR_SAVEERROR");
                if ( pFormat )
                {
                    char aErrorText[128];

                    memset(aErrorText, 0, sizeof(aErrorText));
                    sprintf(aErrorText, pFormat, pNdsFilename);
                    JonesMain_LogErrorToFile(aErrorText);
                }
            }

            break;
        }

        case SITHGAMESAVE_RESTORE:
        {
            result = JonesMain_Restore(pNdsFilename);
            if ( result )
            {
                goto restore_error;
            }

            result = sithGamesave_Process();
            if ( result )
            {
                goto restore_error;
            }

            // Success
            JonesDisplay_CloseLoadScreen();
            break;
        }

        default:
            result = 0;
            break;
    }

    return result;

restore_error:
    if ( aLoadLerrorStr[0] )
    {
        char aErrorText[128];

        memset(aErrorText, 0, sizeof(aErrorText));
        sprintf(aErrorText, aLoadLerrorStr, pNdsFilename);
        JonesMain_LogErrorToFile(aErrorText);
    }

    JonesMain_CloseWindow();
#line 2047
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error in Restore: %s\n", pNdsFilename, 0, 0, 0);
    return 1;
}

void J3DAPI JonesMain_UpdateLevelNum(void)
{
    size_t i;

    JonesMain_curLevelNum = 0;
    for ( i = 1; i < JONESLEVEL_NUMLEVELS; ++i )
    {
        if ( streqi(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[i].pFilename) )
        {
            JonesMain_curLevelNum = i;
            break;
        }
    }

    if ( JonesMain_curLevelNum == 0 )
    {
        for ( i = 1; i < JONESLEVEL_NUMLEVELS; ++i )
        {
            if ( streqi(JonesMain_state.aCurLevelFilename, JonesMain_aNdyLevelLoadInfos[i].pFilename) )
            {
                JonesMain_curLevelNum = i;
                return;
            }
        }
    }
}

void J3DAPI JonesMain_NextLevel(void)
{
    if ( streqi(sithWorld_pCurrentWorld->aName, "17_PRU.cnd") )
    {
        strncpy(JonesMain_state.aCurLevelFilename, sithGamesave_GetPreviousLevelFilename(), sizeof(JonesMain_state.aCurLevelFilename) - 1);
        JonesMain_state.aCurLevelFilename[sizeof(JonesMain_state.aCurLevelFilename) - 1] = 0;
    }
    else
    {
        strncpy(JonesMain_state.aCurLevelFilename, sithWorld_pCurrentWorld->aName, sizeof(JonesMain_state.aCurLevelFilename) - 1);
        JonesMain_state.aCurLevelFilename[sizeof(JonesMain_state.aCurLevelFilename) - 1] = 0;
    }

    JonesMain_state.startMode = JONES_STARTMODE_STARTGAME;

    // Advance level
    JonesMain_UpdateLevelNum();
    ++JonesMain_curLevelNum;
    strncpy(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pFilename, sizeof(JonesMain_state.aCurLevelFilename) - 1);
    JonesMain_state.aCurLevelFilename[sizeof(JonesMain_state.aCurLevelFilename) - 1] = 0;

    // Set process
    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

int J3DAPI JonesMain_ProcessStartGame(void)
{
    int result;

    result = JonesMain_Close();
    if ( result )
    {
        const char* pErrorStr = jonesString_GetString("JONES_STR_CLOSEERROR");
        if ( pErrorStr )
        {
            char aErrorText[128];

            memset(aErrorText, 0, sizeof(aErrorText));
            sprintf(aErrorText, pErrorStr, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
            JonesMain_LogErrorToFile(aErrorText);
        }

        JonesMain_CloseWindow();
    }

    result = JonesMain_Open();
    if ( result )
    {
        const char* pErrorStr = jonesString_GetString("JONES_STR_OPENERROR");
        if ( pErrorStr )
        {
            char aErrorText[128];

            memset(aErrorText, 0, sizeof(aErrorText));
            sprintf(aErrorText, pErrorStr, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pName);
            JonesMain_LogErrorToFile(aErrorText);
        }

        JonesMain_CloseWindow();
    }

    JonesMain_pfProcess = JonesMain_ProcessGame;
    return 0;
}

void J3DAPI JonesMain_SetBonusLevel(void)
{
    JonesMain_state.startMode = JONES_STARTMODE_STARTGAME;

    sithGamesave_SetPreviousLevelFilename(JonesMain_state.aCurLevelFilename);
    strncpy(JonesMain_state.aCurLevelFilename, "17_PRU.cnd", sizeof(JonesMain_state.aCurLevelFilename) - 1);
    JonesMain_state.aCurLevelFilename[sizeof(JonesMain_state.aCurLevelFilename) - 1] = 0;

    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

#ifdef J3D_DEBUG
/* Debug build only: its one caller is the debug build's console command "jumplevel" (0x0041329B), which only the
   debug build registers; retail has no copy. Body from the debug code (0x00422D91); name from Jones3DRE. */
void J3DAPI JonesMain_JumpLevel(int levelNum)
{
    if ( levelNum >= STD_ARRAYLEN(JonesMain_aCndLevelLoadInfos) || levelNum < JONESLEVEL_FIRSTLEVELNUM )
    {
        sithConsole_PrintString("Invalid Level number!");
        return;
    }

    JonesMain_state.startMode = JONES_STARTMODE_STARTGAME;
    strcpy(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[levelNum].pFilename);
    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}
#endif

/* Discarded by the retail linker: the debug build's demo splash screen calls it (JonesDisplay, 0x00416A3B); retail
   keeps no copy. Body from the debug code (0x00422DDF); name from Jones3DRE. */
void J3DAPI JonesMain_RestartGame(void)
{
    JonesMain_curLevelNum = JONESLEVEL_FIRSTLEVELNUM;
    strcpy(JonesMain_state.aCurLevelFilename, JonesMain_aCndLevelLoadInfos[JonesMain_curLevelNum].pFilename);
    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}

#ifdef J3D_DEBUG
/* Debug build only: its one caller is the debug build's console command "restart" (0x00413360), which only the
   debug build registers; retail has no copy. Body from the debug code (0x00422E14); name from Jones3DRE. */
void J3DAPI JonesMain_RestartLevel(void)
{
    JonesMain_pfProcess = JonesMain_ProcessStartGame;
}
#endif

void J3DAPI JonesMain_ShowEndCredits(void)
{
    JonesMain_pfProcess = JonesMain_ProcessCredits;
}

int J3DAPI JonesMain_ProcessCredits(void)
{
    int bFinished; // Retail bug: uninitialised when std3D_StartScene fails

    if ( !JonesMain_bEndCredits )
    {
        int numEscPressed;

        JonesMain_hSndCredits = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(2)); // mus_enddemo.wav
        if ( JonesMain_hSndCredits && !JonesMain_hCreditsMusic )
        {
            JonesMain_hCreditsMusic = sithSoundMixer_PlaySound(JonesMain_hSndCredits, 1.0, 0.0, SOUNDPLAY_PLAYONCE | SOUNDPLAY_LOOP);
        }

        stdControl_ReadControls();
        
        if ( !JonesMain_bEndCredits && stdControl_ReadKey(DIK_ESCAPE, &numEscPressed) )
        {
            JonesMain_bEndCredits = 1;
        }
    }

    std3D_ClearZBuffer();
    rdCache_AdvanceFrame();

    // TODO: Refactor and enable viewport clearing in std3D instead
    if ( JonesMain_state.displaySettings.bClearBackBuffer )
    {
        stdDisplay_BackBufferFill(0, NULL);
    }

    if ( !std3D_StartScene() )
    {
        sithDrawScene();
        if ( !JonesMain_bEndCredits )
        {
            bFinished = JonesHud_DrawCredits(0, JonesMain_hCreditsMusic);
        }
        else
        {
            bFinished = JonesHud_DrawCredits(1, JonesMain_hCreditsMusic);
        }

        rdCache_Flush();
        rdCache_FlushAlpha();
        std3D_EndScene();
        stdDisplay_Update();
    }

    if ( bFinished )
    {
        if ( JonesMain_hCreditsMusic )
        {
            sithSoundMixer_StopSound(JonesMain_hCreditsMusic);
        }

        JonesMain_pfProcess     = JonesMain_ProcessGame;
        JonesMain_hSndCredits   = 0;
        JonesMain_hCreditsMusic = 0;
        JonesMain_bEndCredits   = 0;
        JonesHud_ShowGameOverDialog(0);
    }

    return wkernel_PeekProcessEvents();
}

int J3DAPI JonesMain_HasStarted(void)
{
    return JonesMain_bStartup;
}

StdDisplayEnvironment* J3DAPI JonesMain_GetDisplayEnvironment(void)
{
    return JonesMain_pDisplayEnv;
}

JonesDisplaySettings* J3DAPI JonesMain_GetDisplaySettings(void)
{
    return &JonesMain_state.displaySettings;
}

int J3DAPI JonesMain_CloseWindow(void)
{
    return PostMessage(stdWin95_GetWindow(), WM_CLOSE, 0, 0);
}

int J3DAPI JonesMain_FilePrintf(const char* pFormat, ...)
{
    // TODO: Check if file is open? Tho, the startup should take care of that
    va_list args;
    va_start(args, pFormat);
    _vsnprintf(std_genBuffer, STD_ARRAYLEN(std_genBuffer), pFormat, args);
    va_end(args);

    fprintf(JonesMain_pLogFile, std_genBuffer); /* retail bug: the message is used as the format string */
    fflush(JonesMain_pLogFile);
    return STD_ARRAYLEN(std_genBuffer); /* retail bug: the buffer size, not the length written */
}

int J3DAPI JonesMain_LogError(const char* pFormat, ...)
{
    char* pString;

    va_list curargs;
    va_start(curargs, pFormat);
    _vsnprintf(std_genBuffer, STD_ARRAYLEN(std_genBuffer), pFormat, curargs);
    va_end(curargs);

    // TODO: What's the point of using circbuf??
    pString = (char*)stdCircBuf_GetNextElement(&JonesMain_circBuf);
    if ( pString )
    {
        strncpy(pString, std_genBuffer, 127);
        pString[127] = 0;
    }

    switch ( JonesMain_state.outputMode )
    {
        case JONES_OUTPUTMODE_CONSOLE:
            stdConsole_WriteConsole(std_genBuffer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            break;

        case JONES_OUTPUTMODE_LOGFILE:
            JonesMain_FilePrintf(std_genBuffer); /* retail bug: FilePrintf formats std_genBuffer into itself */
            break;
    }

    return strlen(std_genBuffer);
}

int J3DAPI JonesMain_NoLog(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    va_end(args);
    return 0;
}

void J3DAPI JonesMain_RefreshDisplayDevice(void)
{
    JonesMain_bRefreshDisplayDevice = 1;
}

int J3DAPI JonesMain_PlayIntroMovie(void)
{
    const char aJonesopn_320x240[] = "jonesopn_3.snm";
    const char aJonesopn_400x300[] = "jonesopn_4.snm";
    const char aJonesopn_512x384[] = "jonesopn_5.snm";
    const char aJonesopn_640x480[] = "jonesopn.snm";
    const char aJonesopn_800x600[] = "jonesopn_8.snm";

    const ColorInfo cfRGB555  = { STDCOLOR_RGB, 16, 5, 5, 5, 10, 5,  0, 3, 3, 3, 0,  0, 0 };
    const ColorInfo cfRGB565  = { STDCOLOR_RGB, 16, 5, 6, 5, 11, 5,  0, 3, 2, 3, 0,  0, 0 };
    const ColorInfo cfBGR888  = { STDCOLOR_RGB, 24, 8, 8, 8,  0, 8, 16, 0, 0, 0, 0,  0, 0 };
    const ColorInfo cfRGB888  = { STDCOLOR_RGB, 24, 8, 8, 8, 16, 8,  0, 0, 0, 0, 0,  0, 0 };
    const ColorInfo cfBGR8888 = { STDCOLOR_RGB, 32, 8, 8, 8,  0, 8, 16, 0, 0, 0, 8, 24, 0 };
    const ColorInfo cfRGB8888 = { STDCOLOR_RGB, 32, 8, 8, 8, 16, 8,  0, 0, 0, 0, 8, 24, 0 };
    size_t colorInfoSize = 44;

    int result;
    StdVideoMode videoMode;
    char aPath[128];
    char aFilename[256];
    int pixel;

    wkernel_SetWindowProc(JonesMain_IntroWndProc);
    JonesMain_bNoProcessWndEvents = 0;

    result = wkernel_PeekProcessEvents();
    if ( result )
    {
        return result;
    }

    SmushPlay_SysStartup(stdWin95_GetWindow(), SoundDriver_GetDSound());
    SmushPlay_SetGlobalVolume((size_t)(JonesMain_state.soundSettings.maxSoundVolume * 127.0f));

    if ( stdDisplay_GetCurrentVideoMode(&videoMode) )
    {
        return result;
    }

    // Set video filename base on screen resolution
    switch ( videoMode.rasterInfo.width )
    {
        case 512:
            strcpy(aFilename, aJonesopn_512x384);
            break;

        case 640:
            if ( JonesMain_state.performanceLevel >= 3 )
            {
                strcpy(aFilename, aJonesopn_640x480);
            }
            else
            {
                strcpy(aFilename, aJonesopn_320x240);
            }

            break;

        case 800:
            if ( JonesMain_state.performanceLevel >= 3 )
            {
                strcpy(aFilename, aJonesopn_800x600);
            }
            else
            {
                strcpy(aFilename, aJonesopn_400x300);
            }

            break;

        default:
            strcpy(aFilename, aJonesopn_640x480);
            break;
    }

    if ( stdFileUtil_FileExists(aFilename) )
    {
        strncpy(aPath, aFilename, STD_ARRAYLEN(aPath) - 1);
        aPath[STD_ARRAYLEN(aPath) - 1] = 0;
    }
    else
    {
        // Fallback to CD
        char aResDir[128];

        wuRegistry_GetStr("Source Dir", aPath, STD_ARRAYLEN(aPath), "");
        _snprintf(aResDir, STD_ARRAYLEN(aResDir), "%sresource", aPath);
        STD_MAKEPATH(aPath, aResDir, aFilename);
    }

    if ( !stdFileUtil_FileExists(aPath) )
    {
        // Failed to locate game videos, return
        wkernel_SetWindowProc(JonesMain_GameWndProc);
        return wkernel_PeekProcessEvents();
    }

    Sleep(100u);

    if ( 0 == memcmp(&cfRGB555, &videoMode.rasterInfo.colorInfo, colorInfoSize) )
    {
        JonesMain_introVideoMode = 2;
#line 2606
        JonesMain_aIntroMovieColorTable = (uint8_t*)STDMALLOC(((int)UINT16_MAX + 1) * sizeof(int16_t));
        if ( JonesMain_aIntroMovieColorTable )
        {
            // Init pixel conversion table
            for ( pixel = 0; pixel < (int)UINT16_MAX + 1; ++pixel )
            {
                ((uint16_t*)JonesMain_aIntroMovieColorTable)[pixel] = stdDisplay_EncodeFromRGB565((uint16_t)pixel);
            }

            // Play intro movie
            SmushPlay_PlayMovie(aPath, 15, 0, 0, 0, 640, 480, -1, 0, JonesMain_IntroMovieBlt555, 1, 1000000, 1000000);

            if ( JonesMain_aIntroMovieColorTable )
            {
                stdMemory_Free(JonesMain_aIntroMovieColorTable);
            }

            JonesMain_aIntroMovieColorTable = NULL;
        }
    }
    else if ( 0 == memcmp(&cfRGB565, &videoMode.rasterInfo.colorInfo, colorInfoSize) )
    {
        // Play intro movie
        JonesMain_introVideoMode = 1;
        SmushPlay_PlayMovie(aPath, 15, 0, 0, 0, 640, 480, -1, 0, JonesMain_IntroMovieBlt565, 1, 1000000, 1000000);
    }
    else if ( 0 == memcmp(&cfBGR888, &videoMode.rasterInfo.colorInfo, colorInfoSize) || 0 == memcmp(&cfRGB888, &videoMode.rasterInfo.colorInfo, colorInfoSize) )
    {
        JonesMain_introVideoMode = 3;
    }
    else if ( 0 == memcmp(&cfBGR8888, &videoMode.rasterInfo.colorInfo, colorInfoSize) || 0 == memcmp(&cfRGB8888, &videoMode.rasterInfo.colorInfo, colorInfoSize) )
    {
        JonesMain_introVideoMode = 4;
#line 2632
        JonesMain_aIntroMovieColorTable = (uint8_t*)STDMALLOC(((int)UINT16_MAX + 1) * sizeof(uint32_t));
        if ( JonesMain_aIntroMovieColorTable )
        {
            // Init pixel conversion table
            for ( pixel = 0; pixel < (int)UINT16_MAX + 1; ++pixel )
            {
                ((uint32_t*)JonesMain_aIntroMovieColorTable)[pixel] = stdDisplay_EncodeFromRGB565((uint16_t)pixel);
            }

            // Play intro movie
            SmushPlay_PlayMovie(aPath, 15, 0, 0, 0, 640, 480, -1, 0, JonesMain_IntroMovieBlt32, 1, 1000000, 1000000);

            if ( JonesMain_aIntroMovieColorTable )
            {
                stdMemory_Free(JonesMain_aIntroMovieColorTable);
            }

            JonesMain_aIntroMovieColorTable = NULL;
        }
    }
    else
    {
        JonesMain_introVideoMode = 0;
    }

    SmushPlay_SysShutdown();
    JonesMain_bSkipIntro = 0;

    // Set window process to game process
    wkernel_SetWindowProc(JonesMain_GameWndProc);

    if ( JonesMain_bNoProcessWndEvents == 1 )
    {
        return 1;
    }

    result = wkernel_PeekProcessEvents();
    return result;
}

int J3DAPI JonesMain_IntroWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int* pRetValue)
{
    JonesMain_bWndMsgProcessed = 0;

    switch ( uMsg )
    {
        case WM_KEYDOWN:
            JonesMain_IntroHandleWMKeydown(hWnd, wParam, 1, (int16_t)lParam, HIWORD(lParam));
            return 0;

        case WM_LBUTTONUP:
            JonesMain_IntroHandleWMLButtonUp(hWnd, (int16_t)lParam, (int16_t)HIWORD(lParam), wParam);
            return 0;

        case WM_ACTIVATE:
            JonesMain_HandleWMActivate(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_ACTIVATEAPP:
            JonesMain_HandleWMActivateApp(hWnd, wParam, lParam);
            return 0;

        case WM_GETMINMAXINFO:
            JonesMain_HandleWMGetMinMaxInfo(hWnd, (LPMINMAXINFO)lParam);
            return 0;

        case WM_POWERBROADCAST:
            JonesMain_HandleWMPowerBroadcast(hWnd, wParam);
            return lParam;
    }

    if ( JonesMain_bWndMsgProcessed )
    {
        *pRetValue = 0;
    }

    return JonesMain_bWndMsgProcessed;
}

void J3DAPI JonesMain_IntroHandleWMLButtonUp(HWND hwnd, int curPosX, int curPosY, WPARAM mvk)
{
    JonesMain_bSkipIntro = 1;
}

void J3DAPI JonesMain_IntroHandleWMKeydown(HWND hwnd, WPARAM vk, int a3, int repreatCount, int exkeyflags)
{

    switch ( vk )
    {
        case VK_RETURN:
        case VK_ESCAPE:
            JonesMain_bSkipIntro = 1;
            break;

        case VK_F4:
            JonesMain_CloseWindow();
            break;

        case VK_F12:
            sithMakeDirs();
            sithRender_MakeScreenShot();
            break;

        default:
            return;
    }
}

int J3DAPI JonesMain_IntroMovieBlt565(const SmushBitmap* pBitmap, int a2)
{
    int result;
    int lockResult;
    uint32_t i;
    uint32_t j;
    int curHeight;
    int curWidth;
    uint8_t* pPixels;
    uint32_t height;
    uint32_t width;
    void* pSurface;
    uint8_t* pCurPixelIn;
    uint16_t* pCurPixelOut;
    uint8_t* pRow;
    int pitch;


    
    
    
    
    
    
    
    
    
    
    
    

    lockResult = stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch);

    pRow = (uint8_t*)pSurface;
    pPixels = (uint8_t*)pBitmap->pPixels;
    curHeight = 0;

    for ( i = 0; i < height; ++i )
    {
        curWidth = 0;
        pCurPixelIn = pPixels;
        pCurPixelOut = (uint16_t*)pRow;

        for ( j = 0; j < width; ++j )
        {
            *pCurPixelOut++ = *(uint16_t*)pCurPixelIn;

            curWidth += pBitmap->width;
            while ( curWidth >= (int)width )
            {
                curWidth -= width;
                pCurPixelIn += pBitmap->pixelSize;
            }
        }

        pRow += pitch;

        curHeight += pBitmap->height;
        while ( curHeight >= (int)height )
        {
            curHeight -= height;
            pPixels += pBitmap->pitch;
        }
    }

    stdDisplay_UnlockBackBuffer();

    stdDisplay_Update();

    if ( JonesMain_bSkipIntro )
    {
    }
    else
    {
        result = wkernel_PeekProcessEvents();
        if ( result )
        {
            JonesMain_bNoProcessWndEvents = 1;
        }
        else
        {
            return 0;
        }
    }

    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = 0;
    return 1;
}

int J3DAPI JonesMain_IntroMovieBlt555(const SmushBitmap* pBitmap, int a2)
{
    int result;
    int lockResult;
    uint32_t i;
    uint32_t j;
    int curHeight;
    int curWidth;
    uint8_t* pPixels;
    uint32_t height;
    uint32_t width;
    void* pSurface;
    uint8_t* pCurPixelIn;
    uint16_t* pCurPixelOut;
    uint8_t* pRow;
    int pitch;


    
    
    
    
    
    
    
    
    
    
    
    

    lockResult = stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch);

    pRow = (uint8_t*)pSurface;
    pPixels = (uint8_t*)pBitmap->pPixels;
    curHeight = 0;

    for ( i = 0; i < height; ++i )
    {
        curWidth = 0;
        pCurPixelIn = pPixels;
        pCurPixelOut = (uint16_t*)pRow;

        for ( j = 0; j < width; ++j )
        {
            *pCurPixelOut++ = *(uint16_t*)&JonesMain_aIntroMovieColorTable[2 * *(uint16_t*)pCurPixelIn];

            curWidth += pBitmap->width;
            while ( curWidth >= (int)width )
            {
                curWidth -= width;
                pCurPixelIn += pBitmap->pixelSize;
            }
        }

        pRow += pitch;

        curHeight += pBitmap->height;
        while ( curHeight >= (int)height )
        {
            curHeight -= height;
            pPixels += pBitmap->pitch;
        }
    }

    stdDisplay_UnlockBackBuffer();

    stdDisplay_Update();

    if ( JonesMain_bSkipIntro )
    {
    }
    else
    {
        result = wkernel_PeekProcessEvents();
        if ( result )
        {
            JonesMain_bNoProcessWndEvents = 1;
        }
        else
        {
            return 0;
        }
    }

    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = 0;
    return 1;
}

int J3DAPI JonesMain_IntroMovieBlt32(const SmushBitmap* pBitmap, int a2)
{
    int result;
    int lockResult;
    uint32_t i;
    uint32_t j;
    int curHeight;
    int curWidth;
    uint8_t* pPixels;
    uint32_t height;
    uint32_t width;
    void* pSurface;
    uint8_t* pCurPixelIn;
    uint32_t* pCurPixelOut;
    uint8_t* pRow;
    int pitch;


    
    
    
    
    
    
    
    
    
    
    
    

    lockResult = stdDisplay_LockBackBuffer(&pSurface, &width, &height, &pitch);

    pRow = (uint8_t*)pSurface;
    pPixels = (uint8_t*)pBitmap->pPixels;
    curHeight = 0;

    for ( i = 0; i < height; ++i )
    {
        curWidth = 0;
        pCurPixelIn = pPixels;
        pCurPixelOut = (uint32_t*)pRow;

        for ( j = 0; j < width; ++j )
        {
            *pCurPixelOut++ = *(uint32_t*)&JonesMain_aIntroMovieColorTable[4 * *(uint16_t*)pCurPixelIn];

            curWidth += pBitmap->width;
            while ( curWidth >= (int)width )
            {
                curWidth -= width;
                pCurPixelIn += pBitmap->pixelSize;
            }
        }

        pRow += pitch;

        curHeight += pBitmap->height;
        while ( curHeight >= (int)height )
        {
            curHeight -= height;
            pPixels += pBitmap->pitch;
        }
    }

    stdDisplay_UnlockBackBuffer();

    stdDisplay_Update();

    if ( JonesMain_bSkipIntro )
    {
    }
    else
    {
        result = wkernel_PeekProcessEvents();
        if ( result )
        {
            JonesMain_bNoProcessWndEvents = 1;
        }
        else
        {
            return 0;
        }
    }

    if ( JonesMain_aIntroMovieColorTable )
    {
        stdMemory_Free(JonesMain_aIntroMovieColorTable);
    }

    JonesMain_aIntroMovieColorTable = NULL;
    return 1;
}

J3DNORETURN void J3DAPI JonesMain_Assert(const char* pErrorText, const char* pSrcFile, int line)
{
    size_t filenamePos;
    int bFoundFilename;
    size_t i;

    filenamePos = 0;
    bFoundFilename = 0;

    if ( JonesMain_bAssertTriggered )
    {
        DebugBreak();
        exit(1);
    }

    JonesMain_bAssertTriggered = 1;

    i = 0;
    while ( pSrcFile[i] )
    {
        if ( pSrcFile[i] == '\\' )
        {
            bFoundFilename = 1;
            filenamePos    = i;
        }

        ++i;
    }

    if ( bFoundFilename )
    {
        ++filenamePos;
    }

    stdUtil_Format(JonesMain_aErrorBuffer, STD_ARRAYLEN(JonesMain_aErrorBuffer), "%s(%d):  %s\n", &pSrcFile[filenamePos], line, pErrorText);
    std_pHS->pErrorPrint("ASSERT: %s", JonesMain_aErrorBuffer);

    MessageBox(NULL, JonesMain_aErrorBuffer, "Assert Handler", MB_TASKMODAL);

    DebugBreak();
    JonesMain_Shutdown();
    exit(1);
}

void J3DAPI JonesMain_BindToggleMenuControlKeys(const size_t* paKeyIds, int numKeys)
{
    int i;

    memset(JonesMain_aToggleMenuKeyIds, 0, JONESCONTROL_ACTION_MAXBINDS); // Retail bug: clears 8 bytes, not the whole array
    for ( i = 0; i < numKeys; ++i )
    {
        JonesMain_aToggleMenuKeyIds[i] = paKeyIds[i];
    }
}

void J3DAPI JonesMain_InitializeHUD(void)
{
    JonesHud_EnableMenu(0);
    JonesHud_SetHealthBarAlpha(0.0f);
    jonesCog_bShowHealthHUD  = 0;
    jonesCog_bEnableGamesave = 1;
}

int J3DAPI JonesMain_SaveHUD(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = sithMulti_message.data;
    SITHDSS_PUSHUINT8(JonesHud_IsMenuEnabled());
    SITHDSS_PUSHUINT8(jonesCog_bShowHealthHUD);
    SITHDSS_PUSHFLOAT(JonesHud_GetHealthBarAlpha());
    sithMulti_message.type = SITHDSS_HUDSTATE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0x01);
}

int J3DAPI JonesMain_RestoreHUD(const SithMessage* pMsg)
{
    float alpha;
    uint8_t* pCurDSSMsgData_;
    int value;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 2773
    STD_ASSERTREL(pMsg);

    value = SITHDSS_POPUINT8();
    JonesHud_EnableMenu(value);
    value = SITHDSS_POPUINT8();
    jonesCog_bShowHealthHUD = value;
    alpha = SITHDSS_POPFLOAT();
    JonesHud_SetHealthBarAlpha(alpha);
    SITHDSS_ENDIN;

    return 1;
}

size_t J3DAPI JonesMain_GetCurrentLevelNum(void)
{
    return JonesMain_curLevelNum;
}

void J3DAPI JonesMain_LogErrorToFile(const char* pErrorText)
{
    char curFilePath[128];
    const char* pFilename = jonesString_GetString("JONES_STR_ERRORFILE");
    size_t pathLen;
    char aInstallPath[128];

    memset(curFilePath, 0, sizeof(curFilePath));
    if ( !pFilename )
    {
        sprintf(curFilePath, ".\\\\%s", "JonesError.txt");
    }
    else
    {
        memset(aInstallPath, 0, sizeof(aInstallPath));
        wuRegistry_GetStr("Install Path", aInstallPath, STD_ARRAYLEN(aInstallPath), "");
        pathLen = strlen(aInstallPath);
        if ( !pathLen )
        {
            sprintf(curFilePath, ".\\\\%s", pFilename);
        }
        else if ( aInstallPath[pathLen - 1] != '\\' )
        {
            sprintf(curFilePath, "%s\\%s", aInstallPath, pFilename);
        }
        else
        {
            sprintf(curFilePath, "%s%s", aInstallPath, pFilename);
        }
    }

    if ( pErrorText && strlen(pErrorText) > 0 )
    {
        FILE* fp = fopen(curFilePath, "wt+");
        if ( !fp )
        {
            return;
        }

        fprintf(fp, pErrorText);
        fprintf(fp, "\n");
        fclose(fp);
    }
}
