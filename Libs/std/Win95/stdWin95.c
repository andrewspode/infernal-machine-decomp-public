
#include "stdWin95.h"

#include <std/General/std.h>

static GUID stdWin95_guid;
static HWND stdWin95_hwnd = NULL;
static HINSTANCE stdWin95_hInstance = NULL;

/* Linker-discarded (no caller in either image): retail emits this file's J3D_FILE literal (0x0053EBB0) ahead of SetGuid's
   "pGuid", so a function defined before SetGuid used it, and the retail __LINE__ anchor (SetGuid's assert at 319) leaves a
   +286-line head. The Indy debug build has exactly three functions there (0x004F1490, 0x004F15BF, 0x004F168B), restored
   from its code: an 8-bit DIB section with an identity palette, a 24-bit one, and a window-frame size
   adjustment. Jedi Knight's module holds GDI helpers at the same place; their OpenJKDF2 names are kept for the two DIB
   helpers and the frame helper. JK's Create16bppPaletted/SetPalette/SetPalette2, placeholders here before, are not in
   the debug build and are gone. */
HBITMAP J3DAPI stdWin95_Create8bppPaletted(HDC hdc, int width, int height, void** ppvBits)
{
    BITMAPINFO* pbmi;
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD bmiColors[256];
    } info;
    WORD* pIndex;
    HBITMAP hBitmap;
    int n;

    pbmi = (BITMAPINFO*)&info;
    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = width;
    pbmi->bmiHeader.biHeight        = -height;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = 8;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 0;
    pbmi->bmiHeader.biClrImportant  = 0;

    pIndex = info.bmiColors;
    for ( n = 0; n < 256; ++n )
    {
        *pIndex = (WORD)n;
        ++pIndex;
    }

    hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&info, DIB_PAL_COLORS, ppvBits, NULL, 0);
    if ( !hBitmap )
    {
#line 109
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error during CreateDIBSection.\n", 0, 0, 0, 0);
        return NULL;
    }

    return hBitmap;
}

HBITMAP J3DAPI stdWin95_CreateRGB(HDC hdc, int width, int height, void** ppvBits)
{
    BITMAPINFO* pbmi;
    BITMAPINFO info;
    HBITMAP hbm;

    pbmi = &info;
    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = width;
    pbmi->bmiHeader.biHeight        = -height;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = 24;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 0;
    pbmi->bmiHeader.biClrImportant  = 0;

    hbm = CreateDIBSection(hdc, &info, DIB_RGB_COLORS, ppvBits, NULL, 0);
    if ( !hbm )
    {
#line 144
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error during CreateDIBSection.\n", 0, 0, 0, 0);
        return NULL;
    }

    return hbm;
}

void J3DAPI stdWin95_GetSystemInfo(int* pWidth, int* pHeight, DWORD style, int bMenu)
{
    int dx;
    int dy;

    dx = dy = 0;
    if ( style & WS_BORDER )
    {
        if ( style & WS_THICKFRAME )
        {
            dx += 2 * GetSystemMetrics(SM_CXFRAME);
            dy += 2 * GetSystemMetrics(SM_CYFRAME);
        }
        else
        {
            dx += 2 * GetSystemMetrics(SM_CXBORDER);
            dy += 2 * GetSystemMetrics(SM_CYBORDER);
        }
    }

    if ( style & WS_CAPTION )
    {
        dy += GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYBORDER);
    }

    if ( bMenu )
    {
        dy += GetSystemMetrics(SM_CYMENU);
    }

    if ( style & WS_VSCROLL )
    {
        dx += GetSystemMetrics(SM_CXVSCROLL);
    }

    if ( style & WS_HSCROLL )
    {
        dy += GetSystemMetrics(SM_CXHSCROLL);
    }

    *pWidth += dx;
    *pHeight += dy;
}

void J3DAPI stdWin95_SetWindow(HWND hwnd)
{
    stdWin95_hwnd = hwnd;
}

HWND J3DAPI stdWin95_GetWindow(void)
{
    return stdWin95_hwnd;
}

void J3DAPI stdWin95_SetInstance(HINSTANCE hinstance)
{
    stdWin95_hInstance = hinstance;
}

HINSTANCE J3DAPI stdWin95_GetInstance(void)
{
    return stdWin95_hInstance;
}

void J3DAPI stdWin95_SetGuid(const GUID* pGuid)
{
#line 319
    STD_ASSERTREL(pGuid);
    stdWin95_guid = *pGuid;
}

/* PLACEHOLDER, linker-discarded. Evidence: the debug build has this accessor
   (`mov eax, &stdWin95_guid; ret`) directly after SetGuid, called by the discarded stdComm CreateGame and
   EnumSessions*; retail loads nothing from stdWin95_guid. Name from stdComm.c's earlier comment (inferred). */
const GUID* J3DAPI stdWin95_GetGuid(void)
{
    return &stdWin95_guid;
}

/* Linker-discarded (no caller in either image): the debug build's two list-box helpers after GetGuid (0x004F1816,
   0x004F18EA), restored from its code; names inferred. AddListBoxLines adds each '\n'-separated line, widens the
   horizontal extent (passing the whole text, as the debug code does), selects the last line and trims to maxLines. */
void J3DAPI stdWin95_UpdateListBoxExtent(HWND hwnd, const char* pText); // debug build: called before its definition

LRESULT J3DAPI stdWin95_AddListBoxLines(HWND hwnd, char* pText, int maxLines)
{
    char* pLine;
    char* pEnd;
    int numLines;
    LRESULT lResult;
    char* pNext;

    pLine = pText;
    do
    {
        pEnd = strchr(pLine, '\n');
        if ( !pEnd )
        {
            pNext = NULL;
        }
        else
        {
            *pEnd = '\0';
            pNext = pEnd + 1;
        }

        lResult = SendMessageA(hwnd, LB_ADDSTRING, 0, (LPARAM)pLine);
        stdWin95_UpdateListBoxExtent(hwnd, pText);
        pLine = pNext;
    }
    while ( pLine && *pLine );

    numLines = SendMessageA(hwnd, LB_GETCOUNT, 0, 0);
    SendMessageA(hwnd, LB_SETCURSEL, numLines - 1, 0);
    while ( numLines > maxLines )
    {
        SendMessageA(hwnd, LB_DELETESTRING, 0, 0);
        --numLines;
    }

    return lResult;
}

void J3DAPI stdWin95_UpdateListBoxExtent(HWND hwnd, const char* pText)
{
    HDC hdc;
    SIZE size;
    int extent;

    hdc = GetDC(hwnd);
    GetTextExtentPoint32A(hdc, pText, strlen(pText), &size);
    extent = SendMessageA(hwnd, LB_GETHORIZONTALEXTENT, 0, 0);
    if ( size.cx > extent )
    {
        SendMessageA(hwnd, LB_SETHORIZONTALEXTENT, size.cx, 0);
    }

    ReleaseDC(hwnd, hdc);
}
