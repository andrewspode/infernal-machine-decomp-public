#include "wkernel.h"

#include <commdlg.h>
#include <Jones3D/Main/JonesMain.h>
#include <std/Win95/stdWin95.h>

#include <CommCtrl.h> // InitCommonControls
#include <initguid.h> // DEFINE_GUID

DEFINE_GUID(wkernel_guid, 0x82CE4DA0, 0x9CBF, 0x11D1, 0x90, 0x85, 0x00, 0x60, 0x97, 0x76, 0x0EA, 0x02);
static char wkernel_aClassName[] = "wKernelJones3D";

HWND wkernel_hwnd = NULL;
int wkernel_nShowCmd = 0;
MSG wkernel_msg_1 = { 0 };
MSG wkernel_msg_2 = { 0 };
WKERNELWNDPROC wkernel_pfWndProc = NULL;
int wkernel_wndBorderWidth = 0;
int wkernel_wndMenuBarHeight = 0;

int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName);
LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int J3DAPI wkernel_Run(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd, LPCSTR lpWindowName)
{
    int result;

    result = 0;
    wkernel_nShowCmd = nShowCmd;
    if ( wkernel_CreateWindow(hInstance, wkernel_nShowCmd, lpWindowName) )
    {
        return -1;
    }

    stdWin95_SetWindow(wkernel_hwnd);
    stdWin95_SetInstance(hInstance);
    stdWin95_SetGuid(&wkernel_guid);

    InitCommonControls();

    wkernel_wndBorderWidth = 2 * GetSystemMetrics(SM_CXFRAME);
    wkernel_wndMenuBarHeight = (2 * GetSystemMetrics(SM_CXFRAME)) + GetSystemMetrics(SM_CYMENU);

    if ( JonesMain_Startup(lpCmdLine) )
    {
        JonesMain_Shutdown();
        return -1;
    }

    while ( 1 )
    {
        switch ( result )
        {
            case 0:
                result = JonesMain_Process();
                break;

            case -1:
            case 1:
                return result;
        }
    }
}

int J3DAPI wkernel_PeekProcessEvents(void)
{
    if ( PeekMessage(&wkernel_msg_1, NULL, 0, 0, PM_NOREMOVE) )
    {
        do
        {
            switch ( GetMessage(&wkernel_msg_1, NULL, 0, 0) )
            {
                case 0:
                    return 1;
                case -1:
                    return -1;
            }
            TranslateMessage(&wkernel_msg_1);
            DispatchMessage(&wkernel_msg_1);
        } while ( PeekMessage(&wkernel_msg_1, NULL, 0, 0, PM_NOREMOVE) );
    }

    return 0;
}

int J3DAPI wkernel_ProcessEvents(void)
{
    do
    {
        switch ( GetMessage(&wkernel_msg_2, NULL, 0, 0) )
        {
            case 0:
                return 1;
            case -1:
                return -1;
        }
        TranslateMessage(&wkernel_msg_2);
        DispatchMessage(&wkernel_msg_2);
    } while ( PeekMessage(&wkernel_msg_2, NULL, 0, 0, PM_NOREMOVE) );

    return 0;
}

void J3DAPI wkernel_SetWindowStyle(LONG dwNewLong)
{
    SetWindowLong(wkernel_hwnd, GWL_STYLE, dwNewLong);
    if ( (dwNewLong & WS_BORDER) != 0 )
    {
        wkernel_wndBorderWidth = 2 * GetSystemMetrics(SM_CXFRAME);
        wkernel_wndMenuBarHeight = 2 * GetSystemMetrics(SM_CXFRAME);
    }

    if ( (dwNewLong & WS_CAPTION) != 0 )
    {
        wkernel_wndMenuBarHeight += GetSystemMetrics(SM_CYMENU);
    }
}

BOOL J3DAPI wkernel_SetWindowSize(int width, int height)
{
    return SetWindowPos(
        wkernel_hwnd,
        NULL,
        0,
        0,
        width + wkernel_wndBorderWidth,
        height + wkernel_wndMenuBarHeight,
        SWP_NOZORDER | SWP_NOMOVE
    );
}

/* Discarded by the retail linker (no callers in either build): the debug build has two ShowWindow wrappers after
   SetWindowSize (0x005041E7 shows the window with the stored show command, 0x005041FF hides it). Names from Jones3DRE. */
BOOL J3DAPI wkernel_sub_5041E7(void)
{
    return ShowWindow(wkernel_hwnd, wkernel_nShowCmd);
}

BOOL J3DAPI wkernel_sub_5041FF(void)
{
    return ShowWindow(wkernel_hwnd, SW_HIDE);
}

void J3DAPI wkernel_SetWindowProc(WKERNELWNDPROC pfProc)
{
    wkernel_pfWndProc = pfProc;
}

int J3DAPI wkernel_CreateWindow(HINSTANCE hInstance, int nShowCmd, LPCSTR lpWindowName)
{
    WNDCLASSEXA wndcls;
    HWND hWnd;


    wndcls.cbSize        = sizeof(WNDCLASSEXA);
    wndcls.hInstance     = hInstance;
    wndcls.lpszClassName = wkernel_aClassName;
    wndcls.lpszMenuName  = NULL;
    wndcls.lpfnWndProc   = wkernel_MainWndProc;
    wndcls.style         = 0;

    wndcls.hIcon = LoadIcon(hInstance, "APPICON");
    if ( !wndcls.hIcon )
    {
        wndcls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    wndcls.hIconSm = LoadIcon(hInstance, "APPICON");
    if ( !wndcls.hIconSm )
    {
        wndcls.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }

    wndcls.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndcls.cbClsExtra    = 0;
    wndcls.cbWndExtra    = 0;
    wndcls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    if ( !RegisterClassEx(&wndcls) )
    {
        return 1;
    }

    // Make sure only one game window is live
    hWnd = FindWindow(wkernel_aClassName, lpWindowName);
    if ( hWnd )
    {
        return 1;
    }

    wkernel_hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        wkernel_aClassName,
        lpWindowName,
        WS_POPUP | WS_VISIBLE,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL,
        NULL,
        hInstance,
        NULL
    );

    wkernel_SetWindowSize(100, 25);
    if ( !wkernel_hwnd )
    {
        return 1;
    }

    ShowWindow(wkernel_hwnd, SW_SHOWNORMAL);
    UpdateWindow(wkernel_hwnd);
    return 0;
}

LRESULT CALLBACK wkernel_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int retValue;
    int bHandled = 0;
    int bProcess = 1;

    switch ( uMsg )
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            bProcess = 0;
            break;

        case WM_CLOSE:
            JonesMain_Shutdown();
            bProcess = 0;
            break;
    }

    if ( bProcess && wkernel_pfWndProc )
    {
        bHandled = wkernel_pfWndProc(hWnd, uMsg, wParam, lParam, &retValue);
        if ( bHandled )
        {
            return retValue;
        }
    }

    if ( !bHandled )
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 1;
}
