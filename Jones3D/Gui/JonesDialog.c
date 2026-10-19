#include "JonesDialog.h"

#include <Jones3D/Main/JonesMain.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

// Retail compiled the whole TU without optimization (EBP frames, stack homes,
// 0xCC fill; intrinsics stay enabled), like JonesConsole.c and JonesHud.c.
#pragma optimize("", off)

static int JonesDialog_bWindowResized = 1;
static int JonesDialog_cursorPointIdx = 1;

static POINT JonesDialog_mousePos = { 0 };
static JonesDialogGetDrawBufferDCFunc JonesDialog_pfGetDrawBufferDC = NULL;
static CHAR JonesDialog_szErrorBuffer[256] = { 0 };
static GDIDIBSectionInfo JonesDialog_GDIDIBSectionInfoOffScreen = { 0 };
static JonesDialogReleaseDCFunc JonesDialog_pfReleaseDC = NULL;
static RECT JonesDialog_windowRect = { 0 };
static RECT JonesDialog_windowPos[2] = { 0 };
static RECT JonesDialog_stru_553210 = { 0 };
static JonesDialogCursorInfo JonesDialog_cursorInfo = { 0 };
static GDIDIBSectionInfo JonesDialog_GDIDIBSectionInfo = { 0 };
static RECT JonesDialog_stru_5532D0 = { 0 };
static JonesDialogFlipPageFunc JonesDialog_pfFlipPage = NULL;
static int JonesDialog_dword_5532E4 = 0;
static int JonesDialog_bCursorTrackingInited = 0;
static int JonesDialog_dword_5532EC = 0;

static int J3DAPI JonesDialog_DummyFlipGDI(void);
static int J3DAPI JonesDialog_AllocScreenDBISection(int* pbSkippedAllocation);
static int J3DAPI JonesDialog_FreeScreenDIBSection(void);
static int J3DAPI JonesDialog_AllocOffScreenDIBSection(int* pBSkipedAllocation);
static int J3DAPI JonesDialog_FreeOffScreenDIBSection(void);

static void J3DAPI JonesDialog_SetGameState(JonesDialogGameState* pState, int bWindowModeSupported)
{
    RECT rcBuffer;
    unsigned int height;
    POINT sourceOrigin;
    unsigned int width;

    sourceOrigin.x = 0;
    sourceOrigin.y = 0;
    rcBuffer.left = 0;
    rcBuffer.top = 0;
    rcBuffer.right = 0;
    rcBuffer.bottom = 0;

    stdDisplay_GetBackBufferSize(&width, &height);
    rcBuffer.right = width;
    rcBuffer.bottom = height;

    if ( bWindowModeSupported )
    {
        JonesDialog_pfGetDrawBufferDC = stdDisplay_GetFrontBufferDC;
        JonesDialog_pfReleaseDC = stdDisplay_ReleaseFrontBufferDC;
        JonesDialog_cursorPointIdx = 1;
        if ( !stdDisplay_IsFullscreen() )
        {
            JonesDialog_pfFlipPage = JonesDialog_DummyFlipGDI;
        }
        else
        {
            JonesDialog_pfFlipPage = stdDisplay_FlipToGDISurface;
            stdDisplay_FlipToGDISurface();
        }
    }
    else
    {
        JonesDialog_pfGetDrawBufferDC = stdDisplay_GetBackBufferDC;
        JonesDialog_pfReleaseDC = stdDisplay_ReleaseBackBufferDC;
        JonesDialog_cursorPointIdx = 0;
        JonesDialog_pfFlipPage = stdDisplay_Update;
    }

    if ( JonesDialog_AllocOffScreenDIBSection(&pState->bOffScreenDBIAllocSkipped) )
    {
        JonesDialog_FreeOffScreenDIBSection();
        return;
    }

    if ( JonesDialog_AllocScreenDBISection(&pState->bScreenDBIAllocSkipped) )
    {
        JonesDialog_FreeScreenDIBSection();
        return;
    }

    pState->unknown7 = JonesDialog_dword_5532E4;
    if ( !JonesDialog_dword_5532E4 && !bWindowModeSupported )
    {
        JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &sourceOrigin, &rcBuffer);
        JonesDialog_pfFlipPage();
        JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &sourceOrigin, &rcBuffer);
        JonesDialog_dword_5532E4 = 1;
    }

    if ( !pState->bScreenDBIAllocSkipped )
    {
        HDC hDC;
        if ( !BitBlt(JonesDialog_GDIDIBSectionInfo.hScreenDC, 0, 0, width, height, JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC, 0, 0, SRCCOPY) )
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 150
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SetGameState: BitBlt of background to virtual desktop failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
        }

        hDC = GetDC(NULL);
        if ( hDC != JonesDialog_GDIDIBSectionInfo.hScreenDC && !BitBlt(hDC, 0, 0, width, height, JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC, 0, 0, SRCCOPY) )
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 161
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SetGameState: BitBlt of background to actual desktop failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
        }

        ReleaseDC(NULL, hDC);
    }

    pState->bGamePaused = JonesMain_IsGamePaused();
    if ( !pState->bGamePaused )
    {
        JonesMain_PauseGame();
    }

    pState->bControlsActive = stdControl_ControlsActive();
    if ( pState->bControlsActive )
    {
        int bOk = stdControl_SetActivation(0);
        if ( !bOk )
        {
            stdControl_ShowMouseCursor(1);
        }
    }

    JonesDialog_InitCursorTracking(&pState->unknown4);
    pState->bFrontBufferClipper = stdDisplay_SetBufferClipper(1);
    if ( !bWindowModeSupported )
    {
        pState->bBackBufferClipper = stdDisplay_SetBufferClipper(0);
    }
}

static void J3DAPI JonesDialog_ResetGameState(JonesDialogGameState* pState, int bWindowModeSupported)
{
    int bActivated;

    if ( !pState->bFrontBufferClipper )
    {
        stdDisplay_RemoveBufferClipper(1);
    }

    if ( !bWindowModeSupported && !pState->bBackBufferClipper )
    {
        stdDisplay_RemoveBufferClipper(0);
    }

    if ( !pState->unknown4 )
    {
        JonesDialog_ResetCursorTracking();
    }

    if ( !pState->bOffScreenDBIAllocSkipped )
    {
        JonesDialog_FreeOffScreenDIBSection();
    }

    if ( !pState->bScreenDBIAllocSkipped )
    {
        JonesDialog_FreeScreenDIBSection();
    }

    stdControl_SetActivation(pState->bControlsActive);
    bActivated = stdControl_SetActivation(pState->bControlsActive);
    if ( !bActivated )
    {
        if ( pState->bControlsActive )
        {
            stdControl_ShowMouseCursor(0);
        }
        else
        {
            stdControl_ShowMouseCursor(1);
        }
    }

    JonesDialog_dword_5532E4 = pState->unknown7;
    if ( !pState->bGamePaused )
    {
        JonesMain_ResumeGame();
    }
}
/* retail bug: no return value; the debug build is the same */
static int J3DAPI JonesDialog_DummyFlipGDI(void)
{
}

int J3DAPI JonesDialog_InitCursorTracking(int* pAlreadyInited)
{
    HDC hDrawBufferDC;
    RECT recz;
    int bufWidth;
    int bufHeight;
    int canRenderWindowed;

    if ( JonesDialog_bCursorTrackingInited )
    {
        *pAlreadyInited = 1;
        return 0;
    }

    *pAlreadyInited = 0;
    memset(&JonesDialog_cursorInfo, 0, sizeof(JonesDialog_cursorInfo));

    canRenderWindowed = stdDisplay_CanRenderWindowed();
    if ( canRenderWindowed == -1 )
    {
        return 1;
    }

    if ( !canRenderWindowed )
    {
        JonesDialog_cursorInfo.hCursor = LoadCursor(NULL, IDC_ARROW);
        if ( !GetIconInfo(JonesDialog_cursorInfo.hCursor, &JonesDialog_cursorInfo.info) )
        {
#line 263
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error calling GetIconInfo().\n", 0, 0, 0, 0);
            JonesDialog_ResetCursorTracking();
            return 1;
        }

        hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
        JonesDialog_cursorInfo.hDrawDC = CreateCompatibleDC(hDrawBufferDC);
        JonesDialog_pfReleaseDC(hDrawBufferDC);

        if ( !JonesDialog_cursorInfo.hDrawDC )
        {
#line 274
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error creating memory DC.\n", 0, 0, 0, 0);
            JonesDialog_ResetCursorTracking();
            return 1;
        }

        if ( JonesDialog_cursorInfo.info.hbmColor )
        {
            if ( !GetBitmapDimensionEx(JonesDialog_cursorInfo.info.hbmColor, &JonesDialog_cursorInfo.size) )
            {
#line 284
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error calling GetBitmapDimensionEx() for color bitmap.\n", 0, 0, 0, 0);
                JonesDialog_ResetCursorTracking();
                return 1;
            }

            if ( SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmColor) == HGDI_ERROR )
            {
#line 290
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error calling SelectObject().\n", 0, 0, 0, 0);
                JonesDialog_ResetCursorTracking();
                return 1;
            }
        }
        else
        {
            if ( !JonesDialog_cursorInfo.info.hbmMask )
            {
#line 299
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: What the heck kinda cursor is this?.\n", 0, 0, 0, 0);
                JonesDialog_ResetCursorTracking();
                return 1;
            }

            if ( !GetBitmapDimensionEx(JonesDialog_cursorInfo.info.hbmMask, &JonesDialog_cursorInfo.size) )
            {
#line 306
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error calling GetBitmapDimensionEx() for color bitmap.\n", 0, 0, 0, 0);
                JonesDialog_ResetCursorTracking();
                return 1;
            }

            if ( JonesDialog_cursorInfo.size.cy )
            {
                JonesDialog_cursorInfo.size.cy /= 2;
            }

            if ( SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmMask) == HGDI_ERROR )
            {
#line 316
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error calling SelectObject().\n", 0, 0, 0, 0);
                JonesDialog_ResetCursorTracking();
                return 1;
            }
        }

        GetClipCursor(&JonesDialog_cursorInfo.rect);

        recz.left = 0;
        recz.top = 0;
        stdDisplay_GetBackBufferSize(&bufWidth, &bufHeight);
        recz.right = bufWidth;
        recz.bottom = bufHeight;
        recz.right -= 1;
        recz.bottom -= 1;

        if ( !ClipCursor(&recz) )
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 336
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_InitCursorTracking: Error calling ClipCursor().  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
            JonesDialog_ResetCursorTracking();
            return 1;
        }

        JonesDialog_cursorInfo.size.cx = 32;
        JonesDialog_cursorInfo.size.cy = 32;

        JonesDialog_cursorInfo.point[0].x = -1;
        JonesDialog_cursorInfo.point[0].y = -1;
        JonesDialog_cursorInfo.point[1].x = -1;
        JonesDialog_cursorInfo.point[1].y = -1;

        JonesDialog_bCursorTrackingInited = 1;
    }

    return 0;
}

int J3DAPI JonesDialog_TrackCursor(int bReDraw)
{
    int canRenderWindowed;

    if ( !JonesDialog_bCursorTrackingInited )
    {
        return 1;
    }

    if ( !JonesDialog_bWindowResized )
    {
        return 0;
    }

    canRenderWindowed = stdDisplay_CanRenderWindowed();
    if ( canRenderWindowed == -1 )
    {
        return 1;
    }

    if ( !canRenderWindowed && JonesDialog_cursorInfo.hDrawDC )
    {
        int bDraw;
        POINT origin;
        POINT mousePos;
        RECT drawRect;
        HDC hDrawBufferDC;

        origin.x = 0;
        origin.y = 0;
        bDraw = 1;

        GetCursorPos(&mousePos);
        if ( mousePos.x != JonesDialog_cursorInfo.point[1].x || mousePos.y != JonesDialog_cursorInfo.point[1].y || bReDraw )
        {
            drawRect.left = JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].x;
            drawRect.top = JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].y;
            drawRect.right = JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].x + JonesDialog_cursorInfo.size.cx;
            drawRect.bottom = JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx].y + JonesDialog_cursorInfo.size.cy;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &JonesDialog_cursorInfo.point[JonesDialog_cursorPointIdx], &drawRect);

            if ( JonesDialog_cursorInfo.numHandels )
            {
                RECT wndRect;
                int i;

                for ( i = 0; i < JonesDialog_cursorInfo.numHandels; i++ )
                {
                    GetWindowRect(JonesDialog_cursorInfo.aHwnd[i], &wndRect);
                    origin.x = wndRect.left;
                    origin.y = wndRect.top;
                    JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &origin, &wndRect);
                }
            }

            memcpy(&JonesDialog_cursorInfo.point[0], &JonesDialog_cursorInfo.point[1], sizeof(POINT));
            memcpy(&JonesDialog_cursorInfo.point[1], &mousePos, sizeof(POINT) * 2); // Retail copies mousePos and the last origin over point[1] and hCursor/fIcon

            drawRect.left = JonesDialog_cursorInfo.point[1].x;
            drawRect.top = JonesDialog_cursorInfo.point[1].y;
            drawRect.right = JonesDialog_cursorInfo.point[1].x + JonesDialog_cursorInfo.size.cx;
            drawRect.bottom = JonesDialog_cursorInfo.point[1].y + JonesDialog_cursorInfo.size.cy;

            origin.x = 0;
            origin.y = 0;
            ClientToScreen(stdWin95_GetWindow(), &origin);

            hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
            if ( !BitBlt(hDrawBufferDC, drawRect.left + origin.x, drawRect.top + origin.y, JonesDialog_cursorInfo.size.cx, JonesDialog_cursorInfo.size.cy, JonesDialog_cursorInfo.hDrawDC, 0, 0, SRCAND) )
            {
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 436
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_TrackCursor: BitBlt of cursor, SRCAND failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
            }

            if ( JonesDialog_cursorInfo.info.hbmColor )
            {
                SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmColor);
                if ( !BitBlt(hDrawBufferDC, drawRect.left + origin.x, drawRect.top + origin.y, JonesDialog_cursorInfo.size.cx, JonesDialog_cursorInfo.size.cy, JonesDialog_cursorInfo.hDrawDC, 0, 0, SRCPAINT) )
                {
                    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 449
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_TrackCursor: BitBlt of cursor, SRCPAINT failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
                }

                SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmMask);
            }
            else if ( !BitBlt(hDrawBufferDC, drawRect.left + origin.x, drawRect.top + origin.y, JonesDialog_cursorInfo.size.cx, JonesDialog_cursorInfo.size.cy, JonesDialog_cursorInfo.hDrawDC, 0, JonesDialog_cursorInfo.size.cy, SRCPAINT) )
            {
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 462
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_TrackCursor: BitBlt of cursor, SRCPAINT failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
            }

            JonesDialog_pfReleaseDC(hDrawBufferDC);
            JonesDialog_pfFlipPage();
        }
    }

    return 0;
}

int J3DAPI JonesDialog_ResetCursorTracking(void)
{
    int bCanRenderWindowed;

    bCanRenderWindowed = stdDisplay_CanRenderWindowed();
    if ( bCanRenderWindowed == -1 )
    {
        return 1;
    }

    if ( !bCanRenderWindowed )
    {
        if ( JonesDialog_cursorInfo.info.hbmColor )
        {
            DeleteObject(JonesDialog_cursorInfo.info.hbmColor);
        }


        if ( JonesDialog_cursorInfo.info.hbmMask )
        {
            DeleteObject(JonesDialog_cursorInfo.info.hbmMask);
        }


        if ( JonesDialog_cursorInfo.hDrawDC )
        {
            DeleteDC(JonesDialog_cursorInfo.hDrawDC);
        }


        ClipCursor(&JonesDialog_cursorInfo.rect);
    }

    memset(&JonesDialog_cursorInfo, 0, sizeof(JonesDialog_cursorInfo));
    JonesDialog_bCursorTrackingInited = 0;
    return 0;
}

int J3DAPI JonesDialog_CopyRectToDrawBuffer(GDIDIBSectionInfo* pInfo, const PPOINT lpSourceOrigin, LPRECT pDstRect)
{
    HDC hDrawBufferDC;

    if ( !lpSourceOrigin )
    {
#line 506
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_CopyRectToDrawBuffer: lpSourceOrigin is NULL.\n", 0, 0, 0, 0);
        return 0;
    }

    hDrawBufferDC = JonesDialog_pfGetDrawBufferDC();
    if ( !hDrawBufferDC )
    {
#line 514
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_CopyRectToDrawBuffer: hDrawBufferDC is NULL.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( pDstRect->left >= pDstRect->right || pDstRect->top >= pDstRect->bottom )
    {
        JonesDialog_pfReleaseDC(hDrawBufferDC);
        return 0;
    }

    if ( pInfo == &JonesDialog_GDIDIBSectionInfo && pInfo->bHasDC )
    {
        HDC hDC = GetDC(NULL);
        if ( !BitBlt(JonesDialog_GDIDIBSectionInfo.hScreenDC, pDstRect->left, pDstRect->top, pDstRect->right - pDstRect->left, pDstRect->bottom - pDstRect->top, hDC, lpSourceOrigin->x, lpSourceOrigin->y, SRCCOPY) )
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 536
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_CopyRectToDrawBuffer: BitBlt to virtual desktop failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
        }

        ReleaseDC(NULL, hDC);
    }

    if ( !BitBlt(hDrawBufferDC, pDstRect->left, pDstRect->top, pDstRect->right - pDstRect->left, pDstRect->bottom - pDstRect->top, pInfo->hScreenDC, lpSourceOrigin->x, lpSourceOrigin->y, SRCCOPY) )
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 551
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_CopyRectToDrawBuffer: BitBlt failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
    }

    JonesDialog_pfReleaseDC(hDrawBufferDC);
    return 1;
}

int J3DAPI JonesDialog_RestoreBackground(HDC hdc, HWND hwnd, LPPOINT a3, LPRECT a4)
{
    int bSuccess = 1;

    if ( !JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC || !a3 || !a4 )
    {
        return 0;
    }

    if ( !BitBlt(hdc, a4->left, a4->top, a4->right - a4->left + 1, a4->bottom - a4->top + 1, JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC, a4->left + a3->x, a4->top + a3->y, SRCCOPY) )
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 575
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_RestoreBackground: BitBlt failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
    }

    return bSuccess;
}

static void J3DAPI JonesDialog_HandleWM_ERASEBKGND(HWND hwnd, WPARAM wParam, int bIconEraseBkgnd, JonesDialogData* pData, int unused)
{
    POINT SourceOrigin;
    WNDPROC lpfnWndProc;

    lpfnWndProc = NULL;
    if ( pData && pData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = pData->lpfnWndProc;
        if ( lpfnWndProc )
        {
            if ( bIconEraseBkgnd )
            {
                lpfnWndProc(hwnd, WM_ICONERASEBKGND, wParam, 0);
            }
            else
            {
                lpfnWndProc(hwnd, WM_ERASEBKGND, wParam, 0);
            }
        }

        SourceOrigin.x = pData->rcWindow.left;
        SourceOrigin.y = pData->rcWindow.top;

        if ( !JonesDialog_GDIDIBSectionInfo.hScreenDC )
        {
#line 606
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_HandleWM_ERASEBKGND: GDIDIBSectionInfo.hDC is NULL.\n", 0, 0, 0, 0);
        }
        else
        {
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &SourceOrigin, &pData->rcWindow);
        }
    }
}

static void J3DAPI JonesDialog_HandleWM_PAINT(HWND hWnd, JonesDialogData* lpData, int unused)
{
    POINT origin;
    WNDPROC lpfnWndProc;

    lpfnWndProc = NULL;
    if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = lpData->lpfnWndProc;
        if ( lpfnWndProc )
        {
            lpfnWndProc(hWnd, WM_PAINT, 0, 0);
        }

        if ( !JonesDialog_GDIDIBSectionInfo.hScreenDC )
        {
#line 630
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_HandleWM_PAINT: hScreenDC is NULL.\n", 0, 0, 0, 0);
        }
        else
        {
            origin.x = lpData->rcWindow.left;
            origin.y = lpData->rcWindow.top;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &origin, &lpData->rcWindow);
            JonesDialog_TrackCursor(1);
        }
    }
}

static void J3DAPI JonesDialog_HandleWM_NCPAINT(HWND hwnd, WPARAM wParam, JonesDialogData* lpData, int unused)
{
    POINT SourceOrigin;
    WNDPROC lpfnWndProc;

    lpfnWndProc = NULL;
    if ( lpData && lpData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = lpData->lpfnWndProc;
        if ( lpfnWndProc )
        {
            lpfnWndProc(hwnd, WM_NCPAINT, wParam, 0);
        }

        if ( !JonesDialog_GDIDIBSectionInfo.hScreenDC )
        {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 662
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_HandleWM_NCPAINT: GDIDIBSectionInfo.hDC is NULL.  Reason: %s\n", JonesDialog_szErrorBuffer, 0, 0, 0);
        }
        else
        {
            SourceOrigin.x = lpData->rcWindow.left;
            SourceOrigin.y = lpData->rcWindow.top;
            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &SourceOrigin, &lpData->rcWindow);
            JonesDialog_TrackCursor(1);
        }
    }
}

static LRESULT CALLBACK JonesDialog_SubclassDialogWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int height;
    POINT SourceOrigin;
    JonesDialogData* lpSubclassData;
    WNDPROC lpfnWndProc;
    int width;
    int bRenderWindowed;

    lpSubclassData = NULL;
    lpfnWndProc = NULL;
    lpSubclassData = (JonesDialogData*)GetWindowLong(hwnd, GWL_USERDATA);
    if ( lpSubclassData && lpSubclassData->dwSize == sizeof(JonesDialogData) )
    {
        lpfnWndProc = lpSubclassData->lpfnWndProc;
        switch ( uMsg )
        {
            case WM_ICONERASEBKGND:
                JonesDialog_HandleWM_ERASEBKGND(hwnd, wParam, 1, lpSubclassData, 0);
                return 0;

            case WM_ERASEBKGND:
                JonesDialog_HandleWM_ERASEBKGND(hwnd, wParam, 0, lpSubclassData, 0);
                return 0;

            case WM_PAINT:
                JonesDialog_HandleWM_PAINT(hwnd, lpSubclassData, 0);
                return 0;

            case WM_NCPAINT:
                JonesDialog_HandleWM_NCPAINT(hwnd, wParam, lpSubclassData, 0);
                return 0;

            case WM_CREATE:
                GetCursorPos(&JonesDialog_mousePos);
                break;

            case WM_NCMOUSEMOVE:
            case WM_NCLBUTTONDOWN:
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
                JonesDialog_mousePos.x = LOWORD(lParam);
                JonesDialog_mousePos.y = HIWORD(lParam);
                break;

            case WM_ENTERSIZEMOVE:
                bRenderWindowed = stdDisplay_CanRenderWindowed();
                if ( bRenderWindowed == -1 )
                {
                    break;
                }

                if ( !bRenderWindowed )
                {
                    JonesDialog_bWindowResized = 0;
                    GetWindowRect(hwnd, &JonesDialog_stru_553210);
                    GetWindowRect(hwnd, &JonesDialog_windowPos[0]);
                    GetWindowRect(hwnd, &JonesDialog_windowPos[1]);

                    JonesDialog_stru_553210.right -= JonesDialog_stru_553210.left;
                    JonesDialog_stru_553210.left = 0;
                    JonesDialog_stru_553210.bottom -= JonesDialog_stru_553210.top;
                    JonesDialog_stru_553210.top = 0;

                    JonesDialog_stru_5532D0.left = 0;
                    JonesDialog_stru_5532D0.top = 0;
                    stdDisplay_GetBackBufferSize(&width, &height);
                    JonesDialog_stru_5532D0.right = width;
                    JonesDialog_stru_5532D0.bottom = height;
                    JonesDialog_stru_5532D0.right = (JonesDialog_stru_5532D0.right < GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, HORZRES) ? JonesDialog_stru_5532D0.right : GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, HORZRES)) - 1;
                    JonesDialog_stru_5532D0.bottom = (JonesDialog_stru_5532D0.bottom < GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, VERTRES) ? JonesDialog_stru_5532D0.bottom : GetDeviceCaps(JonesDialog_GDIDIBSectionInfo.hScreenDC, VERTRES)) - 1;

                    JonesDialog_stru_5532D0.left = JonesDialog_mousePos.x - JonesDialog_windowPos[0].left + 1;
                    JonesDialog_stru_5532D0.top = JonesDialog_mousePos.y - JonesDialog_windowPos[0].top + 1;
                    JonesDialog_stru_5532D0.right -= JonesDialog_windowPos[0].right - JonesDialog_mousePos.x - 1;
                    JonesDialog_stru_5532D0.bottom -= JonesDialog_windowPos[0].bottom - JonesDialog_mousePos.y - 1;

                    JonesDialog_mousePos.x = STDMATH_CLAMP(JonesDialog_mousePos.x, JonesDialog_stru_5532D0.left, JonesDialog_stru_5532D0.right);
                    JonesDialog_mousePos.y = STDMATH_CLAMP(JonesDialog_mousePos.y, JonesDialog_stru_5532D0.top, JonesDialog_stru_5532D0.bottom);
                    SetCursorPos(JonesDialog_mousePos.x, JonesDialog_mousePos.y);

                    if ( !ClipCursor(&JonesDialog_stru_5532D0) )
                    {
                        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 770
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SubclassDialogWindowProc: Error calling ClipCursor().  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
                        return 1;
                    }
                }
                break;

            case WM_MOVE:
                if ( lpSubclassData->pExtraData )
                {
                    GetWindowRect(GetParent(hwnd), &lpSubclassData->rcWindow);
                }
                else
                {
                    GetWindowRect(hwnd, &lpSubclassData->rcWindow);
                }
                break;

            case WM_MOVING:
                bRenderWindowed = stdDisplay_CanRenderWindowed();
                if ( bRenderWindowed == -1 )
                {
                    break;
                }

                if ( !bRenderWindowed && JonesDialog_bCursorTrackingInited )
                {
                    LPRECT pRect;
                    SIZE size;
                    HDC hdcDraw;

                    pRect = (LPRECT)lParam;
                    size.cx = pRect->right - pRect->left;
                    size.cy = pRect->bottom - pRect->top;

                    SourceOrigin.x = JonesDialog_windowPos[JonesDialog_cursorPointIdx].left;
                    SourceOrigin.y = JonesDialog_windowPos[JonesDialog_cursorPointIdx].top;
                    JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &JonesDialog_windowPos[JonesDialog_cursorPointIdx]);

                    if ( JonesDialog_cursorInfo.numHandels )
                    {
                        int i;
                        for ( i = 0; i < JonesDialog_cursorInfo.numHandels - 1; i++ )
                        {
                            GetWindowRect(JonesDialog_cursorInfo.aHwnd[i], &JonesDialog_windowRect);
                            SourceOrigin.x = JonesDialog_windowRect.left;
                            SourceOrigin.y = JonesDialog_windowRect.top;
                            JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &SourceOrigin, &JonesDialog_windowRect);
                        }
                    }

                    memcpy(&lpSubclassData->rcWindow, pRect, sizeof(RECT));

                    SourceOrigin.x = 0;
                    SourceOrigin.y = 0;
                    ClientToScreen(hwnd, &SourceOrigin);
                    GetWindowRect(hwnd, &JonesDialog_windowRect);
                    SourceOrigin.x = JonesDialog_windowRect.left;
                    SourceOrigin.y = JonesDialog_windowRect.top;
                    JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfo, &SourceOrigin, pRect);

                    GetCursorPos(&SourceOrigin);
                    hdcDraw = JonesDialog_pfGetDrawBufferDC();
                    BitBlt(hdcDraw, SourceOrigin.x, SourceOrigin.y, JonesDialog_cursorInfo.size.cx, JonesDialog_cursorInfo.size.cy, JonesDialog_cursorInfo.hDrawDC, 0, 0, SRCAND);
                    if ( JonesDialog_cursorInfo.info.hbmColor )
                    {
                        SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmColor);
                        BitBlt(hdcDraw, SourceOrigin.x, SourceOrigin.y, JonesDialog_cursorInfo.size.cx, JonesDialog_cursorInfo.size.cy, JonesDialog_cursorInfo.hDrawDC, 0, 0, SRCPAINT);
                        SelectObject(JonesDialog_cursorInfo.hDrawDC, JonesDialog_cursorInfo.info.hbmMask);
                    }
                    else
                    {
                        BitBlt(hdcDraw, SourceOrigin.x, SourceOrigin.y, JonesDialog_cursorInfo.size.cx, JonesDialog_cursorInfo.size.cy, JonesDialog_cursorInfo.hDrawDC, 0, JonesDialog_cursorInfo.size.cy, SRCPAINT);
                    }

                    JonesDialog_pfReleaseDC(hdcDraw);
                    memcpy(&JonesDialog_windowPos[0], &JonesDialog_windowPos[1], sizeof(RECT));
                    JonesDialog_windowPos[1] = *pRect;
                    JonesDialog_pfFlipPage();
                    return 1;
                }
                break;

            case WM_EXITSIZEMOVE:
                GetWindowRect(hwnd, &lpSubclassData->rcWindow);
                JonesDialog_bWindowResized = 1;
                return 1;

            case WM_DESTROY:
                lpSubclassData = NULL;
                JonesDialog_TrackCursor(1);
                break;

            default:
                if ( lpfnWndProc )
                {
                    return CallWindowProc(lpfnWndProc, hwnd, uMsg, wParam, lParam);
                }
                break;
        }

        return CallWindowProc(lpfnWndProc, hwnd, uMsg, wParam, lParam);
    }

#line 894
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SubclassDialogWindowProc: Invalid lpSubclassData.\n", 0, 0, 0, 0);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static BOOL CALLBACK JonesDialog_SubclassDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    uint32_t height;
    int posY;
    POINT SourceOrigin;
    int posX;
    JonesDialogData* pData;
    WNDPROC pfPrevHook;
    RECT rc;
    uint32_t width;
    BOOL result;

    pData = NULL;
    pfPrevHook = NULL;
    pData = (JonesDialogData*)GetWindowLong(hwnd, GWL_USERDATA);
    if ( pData && pData->dwSize == sizeof(JonesDialogData) )
    {
        pfPrevHook = (WNDPROC)pData->lpfnPrevHook;
    }

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            SetWindowLong(hwnd, GWL_USERDATA, lParam);
            pData = (JonesDialogData*)lParam;
            if ( pData && pData->dwSize == sizeof(JonesDialogData) )
            {
                pfPrevHook = (WNDPROC)pData->lpfnPrevHook;
                pData->lpfnWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
                pData->lpUserData = (LPVOID)GetWindowLong(hwnd, GWL_USERDATA);
                SetWindowLong(hwnd, GWL_WNDPROC, (LONG)JonesDialog_SubclassDialogWindowProc);
                lParam = pData->lPrevCustData;
            }

            GetWindowRect(hwnd, &rc);
            stdDisplay_GetBackBufferSize(&width, &height);
            posX = (width >> 1) - (rc.right - rc.left + 1) / 2;
            posY = (height >> 1) - (rc.bottom - rc.top + 1) / 2;
            MoveWindow(hwnd, posX, posY, rc.right - rc.left + 1, rc.bottom - rc.top + 1, TRUE);

            if ( pData && pData->dwSize == sizeof(JonesDialogData) )
            {
                GetWindowRect(hwnd, &pData->rcWindow);
            }

            if ( JonesDialog_cursorInfo.numHandels < 10 )
            {
                JonesDialog_cursorInfo.aHwnd[JonesDialog_cursorInfo.numHandels++] = hwnd;
            }
            else
            {
#line 948
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SubclassDialogProc: Too many dialogs displayed!\n", 0, 0, 0, 0);
            }

            SetTimer(hwnd, 1, 33, NULL);
            break;

        case WM_TIMER:
            JonesDialog_TrackCursor(1);
            break;

        case WM_MOVE:
            if ( pData && pData->dwSize == sizeof(JonesDialogData) )
            {
                GetWindowRect(hwnd, &pData->rcWindow);
            }
            break;

        case WM_DESTROY:
            if ( !stdDisplay_CanRenderWindowed() )
            {
                GetWindowRect(hwnd, &rc);
                SourceOrigin.x = rc.left;
                SourceOrigin.y = rc.top;
                JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &rc);
                JonesDialog_pfFlipPage();
                JonesDialog_CopyRectToDrawBuffer(&JonesDialog_GDIDIBSectionInfoOffScreen, &SourceOrigin, &rc);
            }

            if ( JonesDialog_cursorInfo.numHandels )
            {
                JonesDialog_cursorInfo.aHwnd[--JonesDialog_cursorInfo.numHandels] = NULL;
            }

            pData = NULL;
            JonesDialog_TrackCursor(1);
            if ( !JonesDialog_cursorInfo.numHandels )
            {
                KillTimer(hwnd, 1);
            }
            break;
    }

    if ( pfPrevHook )
    {
        result = CallWindowProc(pfPrevHook, hwnd, uMsg, wParam, lParam);
    }
    else
    {
        result = 0;
    }

    return result;
}

static UINT CALLBACK JonesDialog_SubclassFileDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hParent;
    JonesDialogData* pData;
    LPOFNHOOKPROC pfPrevHook;
    LPOPENFILENAMEA lpOpenFileName;
    UINT result;

    pfPrevHook = NULL;
    hParent = GetParent(hDlg);
    if ( hParent == stdWin95_GetWindow() || hParent == GetParent(stdWin95_GetWindow()) )
    {
        hParent = hDlg;
    }

    pData = (JonesDialogData*)GetWindowLong(hDlg, GWL_USERDATA);
    if ( pData && pData->dwSize == sizeof(JonesDialogData) )
    {
        pfPrevHook = pData->lpfnPrevHook;
    }

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            lpOpenFileName = (LPOPENFILENAMEA)lParam;
            pData = (JonesDialogData*)lpOpenFileName->lCustData;
            SetWindowLong(hDlg, GWL_USERDATA, (LONG)pData);
            JonesDialog_dword_5532EC = 0;
            if ( pData && pData->dwSize == sizeof(JonesDialogData) )
            {
#line 1029
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SubclassFileDialogProc: hDlg == %08lx, hParent == %08lx\n", hDlg, hParent, 0, 0);

                lpOpenFileName->lCustData = pData->lPrevCustData;
                pfPrevHook = pData->lpfnPrevHook;
                pData->lpfnWndProc = (WNDPROC)GetWindowLong(hDlg, GWL_WNDPROC);
                pData->lpUserData = (LPVOID)GetWindowLong(hDlg, GWL_USERDATA);
                SetWindowLong(hDlg, GWL_WNDPROC, (LONG)JonesDialog_SubclassDialogWindowProc);
                lParam = (LPARAM)lpOpenFileName;

                if ( JonesDialog_cursorInfo.numHandels < 10 )
                {
                    JonesDialog_cursorInfo.aHwnd[JonesDialog_cursorInfo.numHandels++] = hParent;
                }
                else
                {
#line 1041
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_SubclassFileDialogProc: Too many dialogs displayed!\n", 0, 0, 0, 0);
                }

                if ( pData->pExtraData )
                {
                    pData = pData->pExtraData;
                    pData->lpfnWndProc = (WNDPROC)GetWindowLong(hParent, GWL_WNDPROC);
                    pData->lpUserData = (LPVOID)GetWindowLong(hParent, GWL_USERDATA);
                    GetWindowRect(hParent, &pData->rcWindow);
                    SetWindowLong(hParent, GWL_WNDPROC, (LONG)JonesDialog_SubclassDialogWindowProc);
                    SetWindowLong(hParent, GWL_USERDATA, (LONG)pData);
                }

                result = CallWindowProc((WNDPROC)pfPrevHook, hDlg, uMsg, wParam, lParam);
                if ( pData && pData->dwSize == sizeof(JonesDialogData) )
                {
                    GetWindowRect(hParent, &pData->rcWindow);
                }

                SetTimer(hDlg, 1, 33, NULL);
                return result;
            }
            break;

        case WM_TIMER:
            JonesDialog_TrackCursor(1);
            break;

        case WM_DESTROY:
            if ( JonesDialog_cursorInfo.numHandels )
            {
                JonesDialog_cursorInfo.aHwnd[--JonesDialog_cursorInfo.numHandels] = NULL;
            }

            KillTimer(hDlg, 1);
            break;
    }

    if ( pfPrevHook )
    {
        result = CallWindowProc((WNDPROC)pfPrevHook, hDlg, uMsg, wParam, lParam);
    }
    else
    {
        result = 0;
    }

    return result;
}

static int J3DAPI JonesDialog_AllocOffScreenGDISection(GDIDIBSectionInfo* pInfo, HDC hdc, int b24bpp, int* pbSkipedAllocation)
{
    StdVideoMode displayMode;

    stdDisplay_GetCurrentVideoMode(&displayMode);
    if ( pInfo->hBitmap )
    {
        if ( pbSkipedAllocation )
        {
            *pbSkipedAllocation = 1;
        }

        return 0;
    }

    *pbSkipedAllocation = 0;
    if ( stdDisplay_GetCurrentVideoMode(&displayMode) )
    {
        return 1;
    }

    memset(&pInfo->bmpInfo, 0, sizeof(pInfo->bmpInfo));
    pInfo->bmpInfo.bmiHeader.biSize   = sizeof(pInfo->bmpInfo.bmiHeader);
    pInfo->bmpInfo.bmiHeader.biWidth  = displayMode.rasterInfo.width;
    pInfo->bmpInfo.bmiHeader.biHeight = displayMode.rasterInfo.height;
    pInfo->bmpInfo.bmiHeader.biPlanes = 1;
    if ( b24bpp )
    {
        pInfo->bmpInfo.bmiHeader.biBitCount = 24;
    }
    else
    {
        pInfo->bmpInfo.bmiHeader.biBitCount = displayMode.rasterInfo.colorInfo.bpp;
    }

    pInfo->bmpInfo.bmiHeader.biCompression = BI_RGB;
    pInfo->bmpInfo.bmiHeader.biSizeImage = displayMode.rasterInfo.height * ((displayMode.rasterInfo.width + 3) >> 2) * 4 * (displayMode.rasterInfo.colorInfo.bpp >> 3);

    pInfo->hScreenDC = hdc;
    pInfo->hBitmap   = CreateDIBSection(pInfo->hScreenDC, &pInfo->bmpInfo, DIB_RGB_COLORS, (void**)&pInfo->pPixels, NULL, 0);
    if ( !pInfo->hBitmap )
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 1139
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_AllocOffScreenGDISection: CreateDIBSection failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
    }

    SelectObject(pInfo->hScreenDC, pInfo->hBitmap);
    return pInfo->hBitmap == NULL;
}

static int J3DAPI JonesDialog_AllocScreenDBISection(int* pbSkippedAllocation)
{
    HDC hdcFront;
    int bHasDC;
    HDC hdc;
    StdVideoMode displayMode;

    bHasDC = 0;
    if ( stdDisplay_GetCurrentVideoMode(&displayMode) )
    {
        return 1;
    }

    hdc = GetDC(NULL);
    bHasDC = !stdDisplay_CanRenderWindowed() && displayMode.rasterInfo.colorInfo.bpp == 16 && GetDeviceCaps(hdc, BITSPIXEL) == 16;
    if ( bHasDC )
    {
        hdcFront = stdDisplay_GetFrontBufferDC();
        ReleaseDC(NULL, hdc);
        hdc = CreateCompatibleDC(hdcFront);
        stdDisplay_ReleaseFrontBufferDC(hdcFront);
        if ( JonesDialog_AllocOffScreenGDISection(&JonesDialog_GDIDIBSectionInfo, hdc, bHasDC, pbSkippedAllocation) )
        {
            return 1;
        }
    }
    else
    {
        memset(&JonesDialog_GDIDIBSectionInfo, 0, sizeof(JonesDialog_GDIDIBSectionInfo));
        JonesDialog_GDIDIBSectionInfo.hScreenDC = hdc;
    }

    JonesDialog_GDIDIBSectionInfo.bHasDC = bHasDC;
    if ( bHasDC )
    {
        return JonesDialog_GDIDIBSectionInfo.hBitmap == NULL;
    }
    else
    {
        return JonesDialog_GDIDIBSectionInfo.hScreenDC == NULL;
    }
}

static int J3DAPI JonesDialog_FreeScreenDIBSection(void)
{
    if ( JonesDialog_GDIDIBSectionInfo.hScreenDC )
    {
        if ( JonesDialog_GDIDIBSectionInfo.bHasDC )
        {
            DeleteDC(JonesDialog_GDIDIBSectionInfo.hScreenDC);
        }
        else
        {
            ReleaseDC(0, JonesDialog_GDIDIBSectionInfo.hScreenDC);
        }
    }

    if ( JonesDialog_GDIDIBSectionInfo.hBitmap )
    {
        DeleteObject(JonesDialog_GDIDIBSectionInfo.hBitmap);
    }

    JonesDialog_GDIDIBSectionInfo.hScreenDC = NULL;
    JonesDialog_GDIDIBSectionInfo.hBitmap   = NULL;
    JonesDialog_GDIDIBSectionInfo.pPixels   = NULL;
    return 0;
}

static int J3DAPI JonesDialog_AllocOffScreenDIBSection(int* pBSkipedAllocation)
{
    HDC drawDC;
    StdVideoMode displayMode;
    HDC hScreenDC;

    if ( stdDisplay_GetCurrentVideoMode(&displayMode) )
    {
        return 1;
    }

    drawDC = stdDisplay_GetFrontBufferDC();
    hScreenDC = CreateCompatibleDC(drawDC);
    if ( JonesDialog_AllocOffScreenGDISection(&JonesDialog_GDIDIBSectionInfoOffScreen, hScreenDC, 0, pBSkipedAllocation) )
    {
        return 1;
    }

    JonesDialog_GDIDIBSectionInfoOffScreen.bHasDC = 1;
    if ( !*pBSkipedAllocation && !BitBlt(JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC, 0, 0, displayMode.rasterInfo.width, displayMode.rasterInfo.height, drawDC, 0, 0, SRCCOPY) )
    {
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, JonesDialog_szErrorBuffer, 255, NULL);
#line 1237
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "JonesDialog_AllocOffScreenDIBSection: BitBlt failed.  Reason: %s", JonesDialog_szErrorBuffer, 0, 0, 0);
    }

    stdDisplay_ReleaseFrontBufferDC(drawDC);
    return JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap == NULL;
}

static int J3DAPI JonesDialog_FreeOffScreenDIBSection(void)
{
    if ( JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC )
    {
        DeleteDC(JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC);
    }

    if ( JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap )
    {
        DeleteObject(JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap);
    }

    JonesDialog_GDIDIBSectionInfoOffScreen.hScreenDC = NULL;
    JonesDialog_GDIDIBSectionInfoOffScreen.hBitmap   = NULL;
    JonesDialog_GDIDIBSectionInfoOffScreen.pPixels   = NULL;
    return 0;
}

int J3DAPI JonesDialog_ShowDialog(LPCSTR lpTemplateName, HWND hWnd, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
    int windowed;
    JonesDialogData dlgData;
    HINSTANCE hInstance;
    JonesDialogGameState gameState;
    int result;

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    if ( !JonesMain_HasStarted() )
    {
        return DialogBoxParam(hInstance, lpTemplateName, hWnd, lpDialogFunc, dwInitParam);
    }

    memset(&dlgData, 0, sizeof(dlgData));
    dlgData.dwSize = sizeof(JonesDialogData);
    dlgData.lpfnPrevHook = (LPOFNHOOKPROC)lpDialogFunc;
    dlgData.lPrevCustData = dwInitParam;

    windowed = stdDisplay_CanRenderWindowed();
    if ( windowed == -1 )
    {
        return -1;
    }

    JonesDialog_SetGameState(&gameState, windowed);
    if ( !windowed )
    {
        result = DialogBoxParam(hInstance, lpTemplateName, hWnd, JonesDialog_SubclassDialogProc, (LPARAM)&dlgData);
    }
    else
    {
        result = DialogBoxParam(hInstance, lpTemplateName, hWnd, lpDialogFunc, dwInitParam);
    }

    JonesDialog_ResetGameState(&gameState, windowed);
    return result;
}

BOOL J3DAPI JonesDialog_ShowFileSelectDialog(LPOPENFILENAMEA pofn, int bOpen)
{
    int windowed;
    JonesDialogData dlgExtraData;
    JonesDialogData dlgData;
    JonesDialogGameState gameState;
    BOOL result;

    if ( !JonesMain_HasStarted() )
    {
        return GetOpenFileName(pofn);
    }

    if ( !pofn )
    {
        return 0;
    }

    windowed = stdDisplay_CanRenderWindowed();
    if ( windowed == -1 )
    {
        return -1;
    }

    if ( !windowed )
    {
        memset(&dlgData, 0, sizeof(dlgData));
        dlgData.dwSize = sizeof(JonesDialogData);
        dlgData.lpfnPrevHook = pofn->lpfnHook;
        pofn->lpfnHook = JonesDialog_SubclassFileDialogProc;
        dlgData.lPrevCustData = pofn->lCustData;
        pofn->lCustData = (LPARAM)&dlgData;

        memset(&dlgExtraData, 0, sizeof(dlgExtraData));
        dlgExtraData.dwSize = sizeof(JonesDialogData);
        dlgData.pExtraData = &dlgExtraData;
    }

    JonesDialog_SetGameState(&gameState, windowed);
    if ( bOpen )
    {
        result = GetOpenFileName(pofn);
    }
    else
    {
        result = GetSaveFileName(pofn);
    }

    JonesDialog_ResetGameState(&gameState, windowed);
    return result;
}
