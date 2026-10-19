#include "JonesMain.h"
#include <Jones3D/Display/JonesHudConstants.h>
#include <Jones3D/Display/jonesConfig.h>

#include <rdroid/Primitives/rdModel3K.h>

#include <sith/Engine/sithRender.h>
#include <sith/World/sithModel.h>

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>

#include <w32util/wuRegistry.h>

#include <windowsx.h> // helper macros for win32 controls (combo-box, list-view ...)

extern StdVideoMode JonesMain_curVideoMode;
extern StdDisplayEnvironment* JonesMain_pStartupDisplayEnv;

void J3DAPI JonesDevDialog_LoadSettings(StdDisplayEnvironment* pDisplayEnv, JonesState* pConfig);

BOOL CALLBACK JonesDevDialog_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI JonesDevDialog_InitDevDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
void J3DAPI JonesDevDialog_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int notifyCode);
void J3DAPI JonesDevDialog_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig);
void J3DAPI JonesDevDialog_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState);
int J3DAPI JonesDevDialog_CurDisplaySupportsBPP(const JonesDisplaySettings* pSettings, size_t bpp);

#pragma optimize("", off)

void J3DAPI JonesDevDialog_LoadSettings(StdDisplayEnvironment* pDisplayEnv, JonesState* pConfig)
{
    size_t i;

    // Set local player name to computer name
    DWORD nSize     = STD_ARRAYLEN(pConfig->waPlayerName);
    CHAR aText[128];
    if ( !GetComputerName(aText, &nSize) )
    {
        STD_TOWSTR(pConfig->waPlayerName, "NoName");
    }
    else
    {
        STD_TOWSTR(pConfig->waPlayerName, aText);
    }

    if ( wuRegistry_GetIntEx("Katmai", 1) && rdModel3K_IsAvailable() )
    {
        rdModel3K_SetEnabled(1);
        rdModel3K_Startup();
        rdModel3K_SetEnabled(0);
    }

    // Make sure sith has all required dirs set
    sithMakeDirs();

    wuRegistry_GetStr("StartLevel", pConfig->aCurLevelFilename, STD_ARRAYLEN(pConfig->aCurLevelFilename), "");

    pConfig->displaySettings.bWindowMode  = wuRegistry_GetIntEx("InWindow", 0);
    pConfig->displaySettings.bDualMonitor = wuRegistry_GetIntEx("Dual Monitor", 0);
    pConfig->displaySettings.bBuffering   = wuRegistry_GetIntEx("Buffering", 0);
    pConfig->displaySettings.bFog       = wuRegistry_GetIntEx("Fog", 1);
    pConfig->displaySettings.fogDensity = wuRegistry_GetFloat("Fog Density", 1.0f);
    std3D_EnableFog(pConfig->displaySettings.bFog, pConfig->displaySettings.fogDensity);

    sithRender_fogDensity = pConfig->displaySettings.fogDensity * 100.0f;

    pConfig->displaySettings.filter = wuRegistry_GetInt("Filter", STD3D_MIPMAPFILTER_BILINEAR); // bilinear

    pConfig->bDevMode  = wuRegistry_GetIntEx("DevMode", 0);
    pConfig->startMode = wuRegistry_GetInt("Start Mode", JONES_STARTMODE_DEVELOPERDIALOG);
    pConfig->startMode = STDMATH_CLAMP(pConfig->startMode, JONES_STARTMODE_STARTGAME, JONES_STARTMODE_DISPLAYSETTINGS);

    pConfig->outputMode = wuRegistry_GetInt("Debug Mode", JONES_OUTPUTMODE_NONE);
    pConfig->logLevel   = wuRegistry_GetInt("Verbosity", JONES_LOGLEVEL_NORMAL);
    pConfig->performanceLevel = wuRegistry_GetInt("Performance Level", 4);

    pConfig->displaySettings.geoMode   = wuRegistry_GetInt("Geometry Mode", RD_GEOMETRY_FULL);
    pConfig->displaySettings.lightMode = wuRegistry_GetInt("Lighting Mode", RD_LIGHTING_GOURAUD);

    sithModel_EnableHiPoly(wuRegistry_GetIntEx("HiPoly", 0));

    JonesMain_pStartupDisplayEnv = pDisplayEnv;

    wuRegistry_GetStr("Display", aText, STD_ARRAYLEN(aText), "");

    for ( i = 0; i < JonesMain_pStartupDisplayEnv->numInfos; ++i )
    {
        if ( JonesMain_pStartupDisplayEnv->aDisplayInfos[i].displayDevice.bHAL )
        {
            pConfig->displaySettings.displayDeviceNum = i;
        }

        if ( streq(JonesMain_pStartupDisplayEnv->aDisplayInfos[i].displayDevice.aDriverName, aText) )
        {
            pConfig->displaySettings.displayDeviceNum = i;
            break;
        }
    }

    {
    StdDisplayInfo* pDisplay = &JonesMain_pStartupDisplayEnv->aDisplayInfos[pConfig->displaySettings.displayDeviceNum];
    for ( i = 0; i < pDisplay->numDevices; ++i )
    {
        if ( strstr(pDisplay->aDevices[i].deviceDescription, "HAL") ) // contains "HAL" word
        {
            pConfig->displaySettings.device3DNum = i;
            break;
        }
    }

    wuRegistry_GetStr("3D Device", aText, STD_ARRAYLEN(aText), "");
    for ( i = 0; i < pDisplay->numDevices; ++i )
    {
        if ( streq(pDisplay->aDevices[i].deviceDescription, aText) )
        {
            pConfig->displaySettings.device3DNum = i;
            break;
        }
    }

    JonesMain_curVideoMode.aspectRatio                    = 1.0f;
    JonesMain_curVideoMode.rasterInfo.width               = wuRegistry_GetInt("Width", 640);
    JonesMain_curVideoMode.rasterInfo.height              = wuRegistry_GetInt("Height", 480);
    JonesMain_curVideoMode.rasterInfo.colorInfo.bpp       = wuRegistry_GetInt("BPP", 16);
    JonesMain_curVideoMode.rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;

    pConfig->displaySettings.videoModeNum = JonesDevDialog_FindClosestVideoMode(JonesMain_pStartupDisplayEnv, &JonesMain_curVideoMode, pConfig->displaySettings.displayDeviceNum);

    JonesMain_curVideoMode = pDisplay->aModes[pConfig->displaySettings.videoModeNum];

    pConfig->displaySettings.width  = JonesMain_curVideoMode.rasterInfo.width;
    pConfig->displaySettings.height = JonesMain_curVideoMode.rasterInfo.height;
    pConfig->displaySettings.bClearBackBuffer = 0;

    pConfig->soundSettings.b3DHWSupport  = wuRegistry_GetIntEx("Sound 3D", 0);
    pConfig->soundSettings.bReverseSound = wuRegistry_GetIntEx("ReverseSound", 0);
    }
}


int J3DAPI JonesDevDialog_ShowDevDialog(HWND hWnd, JonesState* pConfig)
{
    HINSTANCE hInstance;
    int res;

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

    res = DialogBoxParam(hInstance, (LPCSTR)101, hWnd, JonesDevDialog_DevDialogProc, (LPARAM)pConfig);
    return res;
}

/* Linker-discarded (no caller in either build): debug 0x00424A55, between ShowDevDialog and DevDialogProc; the same dialog
   procedure on resource 106. Name from Jones3DRE. */
int J3DAPI JonesMain_ShowDisplayOptions(HWND hWnd, JonesState* pConfig)
{
    HINSTANCE hInstance;
    int res;

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

    res = DialogBoxParam(hInstance, (LPCSTR)106, hWnd, JonesDevDialog_DevDialogProc, (LPARAM)pConfig);
    return res;
}

BOOL CALLBACK JonesDevDialog_DevDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL res;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            res = JonesDevDialog_InitDevDialog(hDlg, wParam, lParam);
            break;

        case WM_COMMAND:
            JonesDevDialog_DevDialogHandleCommand(hDlg, (uint16_t)wParam, lParam, HIWORD(wParam));
            res = 1;
            break;

        default:
            res = 0;
            break;
    };

    return res;
}

int J3DAPI JonesDevDialog_InitDevDialog(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    char aListText[64];
    int itemIdx = 0;
    int selIdx;
    HWND hDlgItem;
    tFoundFileInfo fileInfo;
    int levelCount;
    FindFileData* pFileData;
    JonesState* pConfig;
    size_t infoNum;


    pConfig = (JonesState*)lParam;

    // Populate display driver combo box list
    hDlgItem = GetDlgItem(hDlg, 1030); // Display driver
    for ( infoNum = 0; infoNum < JonesMain_pStartupDisplayEnv->numInfos; ++infoNum )
    {
        if ( JonesMain_pStartupDisplayEnv->aDisplayInfos[infoNum].displayDevice.bHAL == 1 )
        {
            sprintf(std_genBuffer, "%s", JonesMain_pStartupDisplayEnv->aDisplayInfos[infoNum].displayDevice.aDriverName);
            itemIdx = ComboBox_AddString(hDlgItem, std_genBuffer);
            ComboBox_SetItemData(hDlgItem, itemIdx, infoNum);

            // Select diver if matches the one in settings
            if ( infoNum == pConfig->displaySettings.displayDeviceNum )
            {
                ComboBox_SetCurSel(hDlgItem, itemIdx);
            }
        }
    }

    // Init CB GeometryMOde
    hDlgItem = GetDlgItem(hDlg, 1008);
    itemIdx = ComboBox_AddString(hDlgItem, "Vertex Only");
    ComboBox_SetItemData(hDlgItem, itemIdx, 1);

    selIdx = itemIdx;
    itemIdx = ComboBox_AddString(hDlgItem, "Wire Frame");
    ComboBox_SetItemData(hDlgItem, itemIdx, 2);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_WIREFRAME )
    {
        selIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Solid");
    ComboBox_SetItemData(hDlgItem, itemIdx, 3);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_SOLID )
    {
        selIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Texture");
    ComboBox_SetItemData(hDlgItem, itemIdx, 4);
    if ( pConfig->displaySettings.geoMode == RD_GEOMETRY_FULL )
    {
        selIdx = itemIdx;
    }

    ComboBox_SetCurSel(hDlgItem, selIdx);

    // Init CB Lighting Mode
    hDlgItem = GetDlgItem(hDlg, 1009);
    itemIdx = ComboBox_AddString(hDlgItem, "None");
    ComboBox_SetItemData(hDlgItem, itemIdx, 1);

    selIdx = itemIdx;
    itemIdx = ComboBox_AddString(hDlgItem, "Lit");
    ComboBox_SetItemData(hDlgItem, itemIdx, 0);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_NONE )
    {
        selIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Diffuse");
    ComboBox_SetItemData(hDlgItem, itemIdx, 2);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_DIFFUSE )
    {
        selIdx = itemIdx;
    }

    itemIdx = ComboBox_AddString(hDlgItem, "Gouraud");
    ComboBox_SetItemData(hDlgItem, itemIdx, 3);
    if ( pConfig->displaySettings.lightMode == RD_LIGHTING_GOURAUD )
    {
        selIdx = itemIdx;
    }

    ComboBox_SetCurSel(hDlgItem, selIdx);

    // Init CB MimpMap Filter
    hDlgItem = GetDlgItem(hDlg, 1012);
    itemIdx = ComboBox_AddString(hDlgItem, "None");
    ComboBox_SetItemData(hDlgItem, itemIdx, 0);

    itemIdx = ComboBox_AddString(hDlgItem, "Bilinear");
    ComboBox_SetCurSel(hDlgItem, itemIdx);// Select bilinear as defult
    ComboBox_SetItemData(hDlgItem, itemIdx, 1);

    itemIdx = ComboBox_AddString(hDlgItem, "Trilinear");
    ComboBox_SetItemData(hDlgItem, itemIdx, 2);

    // Setup Display related controls
    JonesDevDialog_DevDialogInitDisplayDevices(hDlg, pConfig);

    // Populate level List box
    hDlgItem = GetDlgItem(hDlg, 1001);
    if ( hDlgItem )
    {
        pFileData = stdFileUtil_NewFind("ndy", 3, "ndy");
        while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
        {
            itemIdx = ListBox_AddString(hDlgItem, fileInfo.aName);
        }
        stdFileUtil_DisposeFind(pFileData);

        pFileData = stdFileUtil_NewFind("ndy", 3, "cnd");
        while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
        {
            itemIdx = ListBox_AddString(hDlgItem, fileInfo.aName);
        }

        stdFileUtil_DisposeFind(pFileData);

        // Select cur level from config
        levelCount = ListBox_GetCount(hDlgItem);
        for ( itemIdx = 0; itemIdx < levelCount; ++itemIdx )
        {
            ListBox_GetText(hDlgItem, itemIdx, aListText);
            if ( streq(aListText, pConfig->aCurLevelFilename) )
            {
                ListBox_SetCurSel(hDlgItem, itemIdx);
                break;
            }
        }

        if ( itemIdx == levelCount )
        {
            // None selected, select level in the middle of the list
            ListBox_SetCurSel(hDlgItem, levelCount / 2);
        }
    }

    SetWindowLong(hDlg, DWL_USER, lParam); // Set config to dialog handle

    CheckDlgButton(hDlg, 1007, pConfig->bDevMode);// Dev mode

    JonesDevDialog_DevDialogUpdateRadioButtons(hDlg, pConfig);
    return 1;
}


void J3DAPI JonesDevDialog_DevDialogHandleCommand(HWND hWnd, int controlId, LPARAM lParam, int notifyCode)
{
    JonesState* pState;
    HWND hCB;
    int curIdx;


    pState = (JonesState*)GetWindowLong(hWnd, DWL_USER);
    switch ( controlId )
    {
        case 1030: // Display settings
            if ( notifyCode == CBN_SELCHANGE )
            {
                hCB    = GetDlgItem(hWnd, 1030);
                curIdx = ComboBox_GetCurSel(hCB);
                pState->displaySettings.displayDeviceNum = ComboBox_GetItemData(hCB, curIdx);
                JonesDevDialog_DevDialogInitDisplayDevices(hWnd, pState);
            }
            break;

        case 1031: // 3D Driver
            if ( notifyCode == CBN_SELCHANGE )
            {
                hCB    = GetDlgItem(hWnd, 1031);
                curIdx = ComboBox_GetCurSel(hCB);
                pState->displaySettings.device3DNum = ComboBox_GetItemData(hCB, curIdx);
                JonesDevDialog_DevDialogInitDisplayDevices(hWnd, pState);
            }
            break;

        case 1029: // display mode
            if ( notifyCode == CBN_SELCHANGE )
            {
                hCB    = GetDlgItem(hWnd, 1029);
                curIdx = ComboBox_GetCurSel(hCB);
                pState->displaySettings.videoModeNum = ComboBox_GetItemData(hCB, curIdx);
            }
            break;

        case 1013: // error output normal
            pState->outputMode = JONES_OUTPUTMODE_NONE;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1014: // error output console
            pState->outputMode = JONES_OUTPUTMODE_CONSOLE;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1015: // error output file
            pState->outputMode = JONES_OUTPUTMODE_LOGFILE;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1016: // debug output normal
            pState->logLevel = JONES_LOGLEVEL_NORMAL;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1017: // debug output verbose
            pState->logLevel = JONES_LOGLEVEL_VERBOSE;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1018: // debug output quiet
            pState->logLevel = JONES_LOGLEVEL_ERROR;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1176: // performance level 0
            pState->performanceLevel = 0;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1177: // performance level 1
            pState->performanceLevel = 1;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1178: // performance level 2
            pState->performanceLevel = 2;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1179: // performance level 3
            pState->performanceLevel = 3;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1180: // performance level 4
            pState->performanceLevel = 4;
            JonesDevDialog_DevDialogUpdateRadioButtons(hWnd, pState);
            break;

        case 1001: // select level list
            if ( notifyCode != LBN_DBLCLK )
            {
                break;
            }

            controlId = 1; // start game
            // Fall through to start the game

        case 1: // start the game

            // Start game logic

            // Get 3D driver
            hCB    = GetDlgItem(hWnd, 1031);
            curIdx = ComboBox_GetCurSel(hCB);
            pState->displaySettings.device3DNum = ComboBox_GetItemData(hCB, curIdx);

            // Get Display mode
            hCB    = GetDlgItem(hWnd, 1029);
            curIdx = ComboBox_GetCurSel(hCB);
            pState->displaySettings.videoModeNum = ComboBox_GetItemData(hCB, curIdx);

            // Geometry mode
            hCB    = GetDlgItem(hWnd, 1008);
            curIdx = ComboBox_GetCurSel(hCB);
            pState->displaySettings.geoMode = ComboBox_GetItemData(hCB, curIdx);

            if ( pState->displaySettings.geoMode == RD_GEOMETRY_WIREFRAME || pState->displaySettings.geoMode == RD_GEOMETRY_VERTEX )
            {
                pState->displaySettings.bClearBackBuffer = 1;
            }

            // Light mode
            hCB    = GetDlgItem(hWnd, 1009);
            curIdx = ComboBox_GetCurSel(hCB);
            pState->displaySettings.lightMode = ComboBox_GetItemData(hCB, curIdx);

            // Filter mode
            hCB    = GetDlgItem(hWnd, 1012);
            curIdx = ComboBox_GetCurSel(hCB);
            pState->displaySettings.filter = ComboBox_GetItemData(hCB, curIdx);

            pState->displaySettings.bWindowMode = IsDlgButtonChecked(hWnd, 1002) == 1; // window mode
            pState->bDevMode = IsDlgButtonChecked(hWnd, 1007) == 1;// devmode

            pState->displaySettings.width = JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.width;
            pState->displaySettings.height = JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.height;

            // Get selected level & Save settings
            hCB = GetDlgItem(hWnd, 1001);
            if ( ListBox_GetText(hCB, ListBox_GetCurSel(hCB), pState->aCurLevelFilename) == -1 )
            {
                break;
            }

            wuRegistry_SaveStr("Display", JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].displayDevice.aDriverName);
            wuRegistry_SaveStr("3D Device", JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aDevices[pState->displaySettings.device3DNum].deviceDescription);
            wuRegistry_SaveInt("Width", pState->displaySettings.width);
            wuRegistry_SaveInt("Height", pState->displaySettings.height);
            wuRegistry_SaveInt("BPP", JonesMain_pStartupDisplayEnv->aDisplayInfos[pState->displaySettings.displayDeviceNum].aModes[pState->displaySettings.videoModeNum].rasterInfo.colorInfo.bpp);
            wuRegistry_SaveInt("Filter", pState->displaySettings.filter);

            wuRegistry_SaveStr("StartLevel", pState->aCurLevelFilename);

            wuRegistry_SaveIntEx("InWindow", pState->displaySettings.bWindowMode);
            wuRegistry_SaveIntEx("Dual Monitor", pState->displaySettings.bDualMonitor);

            wuRegistry_SaveIntEx("DevMode", pState->bDevMode);

            wuRegistry_SaveIntEx("Sound 3D", pState->soundSettings.b3DHWSupport);

            wuRegistry_SaveInt("Debug Mode", pState->outputMode);
            wuRegistry_SaveInt("Verbosity", pState->logLevel);

            wuRegistry_SaveStr("User Path", pState->aInstallPath);
            wuRegistry_SaveInt("Performance Level", pState->performanceLevel);

            wuRegistry_SaveInt("Geometry Mode", pState->displaySettings.geoMode);
            wuRegistry_SaveInt("Lighting Mode", pState->displaySettings.lightMode);

            // Fall through to close the dialog

        case 2: // exit
            EndDialog(hWnd, controlId); // Close dialog
            break;

        default:
            break;
    }
}

void J3DAPI JonesDevDialog_DevDialogInitDisplayDevices(HWND hDlg, JonesState* pConfig)
{
    HWND hBTNStart;
    StdDisplayInfo* pDisplayInfo;
    int itemNum;
    CHAR aDriverName[128];
    HWND hCBDispMode;
    HWND h3DDriver;
    int bDone;
    size_t modeNumber;
    size_t iDevice;
    HWND hChkWindowMode;

    hBTNStart      = GetDlgItem(hDlg, 1);
    hChkWindowMode = GetDlgItem(hDlg, 1002);
    hCBDispMode    = GetDlgItem(hDlg, 1029);
    h3DDriver      = GetDlgItem(hDlg, 1031);

    ComboBox_ResetContent(hCBDispMode);

    pDisplayInfo = &JonesMain_pStartupDisplayEnv->aDisplayInfos[pConfig->displaySettings.displayDeviceNum];
    if ( !pDisplayInfo->displayDevice.bHAL )
    {
        EnableWindow(hBTNStart, 0);
        EnableWindow(hChkWindowMode, 0);
        EnableWindow(hCBDispMode, 0);
        EnableWindow(h3DDriver, 0);
        return;
    }

    EnableWindow(hBTNStart, 1);
    EnableWindow(hChkWindowMode, pDisplayInfo->displayDevice.bWindowRenderNotSupported == 0);

    if ( pDisplayInfo->displayDevice.bWindowRenderNotSupported )
    {
        pConfig->displaySettings.bWindowMode = 0;
    }

    EnableWindow(hCBDispMode, 1);
    EnableWindow(h3DDriver, 1);
    CheckDlgButton(hDlg, 1002, pConfig->displaySettings.bWindowMode);// CB window mode

    // Init 3D driver combo box list
    bDone = 0;

    memset(aDriverName, 0, sizeof(aDriverName));
    GetWindowText(h3DDriver, aDriverName, STD_ARRAYLEN(aDriverName));

    ComboBox_ResetContent(h3DDriver);
    for ( iDevice = 0; iDevice < pDisplayInfo->numDevices; ++iDevice )
    {
        sprintf(std_genBuffer, "%s", pDisplayInfo->aDevices[iDevice].deviceDescription);

        itemNum = ComboBox_AddString(h3DDriver, std_genBuffer);
        ComboBox_SetItemData(h3DDriver, itemNum, iDevice);

        // Select item in cb list
        if ( streq(aDriverName, std_genBuffer) || pConfig->displaySettings.device3DNum == iDevice )
        {
            ComboBox_SetCurSel(h3DDriver, itemNum);
            pConfig->displaySettings.device3DNum = iDevice;
            bDone = 1;
        }
    }

    if ( !bDone )
    {
        // Select first element in list
        pConfig->displaySettings.device3DNum = 0;
        ComboBox_SetCurSel(h3DDriver, 0);
    }

    // Populate display mode combo box list (resolutions) and select

    JonesMain_curVideoMode.rasterInfo.colorInfo.bpp = 16;

    bDone = 0;
    for ( modeNumber = 0; modeNumber < JonesMain_pStartupDisplayEnv->aDisplayInfos[pConfig->displaySettings.displayDeviceNum].numModes; ++modeNumber )
    {
        sprintf(std_genBuffer, "%dx%d %dbpp", pDisplayInfo->aModes[modeNumber].rasterInfo.width, pDisplayInfo->aModes[modeNumber].rasterInfo.height, pDisplayInfo->aModes[modeNumber].rasterInfo.colorInfo.bpp);

        if ( pDisplayInfo->aModes[modeNumber].aspectRatio == 1.0f
            && pDisplayInfo->aModes[modeNumber].rasterInfo.width >= 512
            && pDisplayInfo->aModes[modeNumber].rasterInfo.height >= 384 )
        {
            if ( JonesDevDialog_CurDisplaySupportsBPP(&pConfig->displaySettings, pDisplayInfo->aModes[modeNumber].rasterInfo.colorInfo.bpp) )
            {
                itemNum = ComboBox_AddString(hCBDispMode, std_genBuffer);
                ComboBox_SetItemData(hCBDispMode, itemNum, modeNumber);

                // Select mode
                if ( pDisplayInfo->aModes[modeNumber].rasterInfo.width == JonesMain_curVideoMode.rasterInfo.width
                    && pDisplayInfo->aModes[modeNumber].rasterInfo.height == JonesMain_curVideoMode.rasterInfo.height
                    && pDisplayInfo->aModes[modeNumber].rasterInfo.colorInfo.bpp == JonesMain_curVideoMode.rasterInfo.colorInfo.bpp )
                {
                    ComboBox_SetCurSel(hCBDispMode, itemNum);
                    bDone = 1;
                }
            }
        }
    }

    if ( !bDone )
    {
        ComboBox_SetCurSel(hCBDispMode, 0);
    }
}

void J3DAPI JonesDevDialog_DevDialogUpdateRadioButtons(HWND hDlg, const JonesState* pState)
{
    switch ( pState->outputMode )
    {
        case JONES_OUTPUTMODE_NONE:
            CheckRadioButton(hDlg, 1013, 1015, 1013); // Normal
            break;
        case JONES_OUTPUTMODE_CONSOLE:
            CheckRadioButton(hDlg, 1013, 1015, 1014); // Console
            break;
        case JONES_OUTPUTMODE_LOGFILE:
            CheckRadioButton(hDlg, 1013, 1015, 1015); // Log file
            break;
    };

    switch ( pState->logLevel )
    {
        case JONES_LOGLEVEL_NORMAL:
            CheckRadioButton(hDlg, 1016, 1018, 1016); // Normal
            break;
        case JONES_LOGLEVEL_VERBOSE:
            CheckRadioButton(hDlg, 1016, 1018, 1017); // Verbose
            break;
        case JONES_LOGLEVEL_ERROR:
            CheckRadioButton(hDlg, 1016, 1018, 1018); // Quite
            break;
    };

    switch ( pState->performanceLevel )
    {
        case 0:
            CheckRadioButton(hDlg, 1176, 1180, 1176);
            break;

        case 1:
            CheckRadioButton(hDlg, 1176, 1180, 1177);
            break;

        case 2:
            CheckRadioButton(hDlg, 1176, 1180, 1178);
            break;

        case 3:
            CheckRadioButton(hDlg, 1176, 1180, 1179);
            break;

        case 4:
            CheckRadioButton(hDlg, 1176, 1180, 1180);
            break;

        default:
            return;
    }
}

size_t J3DAPI JonesDevDialog_FindClosestVideoMode(const StdDisplayEnvironment* pList, const StdVideoMode* pVideoMode, size_t deviceNum)
{
    size_t modeNum;
    StdDisplayInfo* pDisplay;
    size_t i;

    modeNum = 0;
    pDisplay = &pList->aDisplayInfos[deviceNum];

    for ( i = 0; i < pList->aDisplayInfos[deviceNum].numModes; ++i )
    {
        if ( pDisplay->aModes[i].rasterInfo.colorInfo.bpp == 16 )
        {
            if ( pDisplay->aModes[i].rasterInfo.width == pVideoMode->rasterInfo.width && pDisplay->aModes[i].rasterInfo.height == pVideoMode->rasterInfo.height )
            {
                modeNum = i;
                break;
            }

            modeNum = i;
        }
    }

    return modeNum;
}

int J3DAPI JonesDevDialog_CurDisplaySupportsBPP(const JonesDisplaySettings* pSettings, size_t bpp)
{
    StdDisplayInfo* pDisplay = &JonesMain_pStartupDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum];
    switch ( bpp )
    {
        case 32:
            return pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_32;
        case 24:
            return pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_24;
        case 16:
            return pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_16;
        case 8:
            return pDisplay->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_8;
    }

    return 0;
}
