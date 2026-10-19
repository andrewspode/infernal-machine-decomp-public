#include "stdConsole.h"
#include <malloc.h>

HANDLE stdConsole_hOutput    = 0;
static WORD stdConsole_textAttribute = 0;
static WORD stdConsole_wAttributes  = 0;

static int stdConsole_dword_5DCCCC  = 0;
static HANDLE stdConsole_hInput     = 0;
// An 8-byte aggregate: VC5 aligns BSS objects of 8+ bytes to 8, which
// reproduces retail's 4-byte gap before 0x005DCCD8.
static CONSOLE_CURSOR_INFO stdConsole_aUnknown5DCCD8 = { 0 };

int J3DAPI stdConsole_Startup(const char* pTitleText, int attributes, int bShowMinimized)
{
    HWND hwnd;

    AllocConsole();
    SetConsoleTitle(pTitleText);

    stdConsole_hInput  = GetStdHandle(STD_INPUT_HANDLE);
    stdConsole_hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    stdConsole_aUnknown5DCCD8.dwSize = 8;
    stdConsole_aUnknown5DCCD8.bVisible = 1;
    stdConsole_dword_5DCCCC = 0;

    stdConsole_SetAttributes(attributes);
    stdConsole_InitOutputConsole();

    hwnd = FindWindow(0, pTitleText);
    if ( hwnd )
    {
        RECT curwRect;
        int curx;
        RECT sRect;
        int y;
        GetWindowRect(hwnd, &curwRect);

        GetWindowRect(GetDesktopWindow(), &sRect);

        curx = sRect.right - (curwRect.right - curwRect.left) - 10;
        y = sRect.bottom - (curwRect.bottom - curwRect.top) - 25;
        SetWindowPos(hwnd, NULL, curx, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
    }
    if ( bShowMinimized ) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }

    return 1;
}

void J3DAPI stdConsole_Shutdown(void)
{
    FreeConsole();
}

// Text window drawn into the console (the debug build's window module). The character buffer holds the
// interior only: (width - 2) x (height - 2) cells.
typedef struct StdConsoleWindow
{
    int bDrawn;
    int x;
    int y;
    int right;
    int bottom;
    int width;
    int height;
    char aTitle[80];
    uint8_t topStyle;     // 0 single line, 1 double line
    uint8_t bottomStyle;
    uint8_t leftStyle;
    uint8_t rightStyle;
    WORD attr;
    WORD borderAttr;
    WORD titleAttr;
    uint8_t titleAlign;   // 0 centered, 1 left, 2 right
    char* pBuffer;
    int unused7C;
    int bufferSize;
    int size;
    int curX;
    int curY;
} StdConsoleWindow;

void J3DAPI stdConsole_sub_4FFE92(StdConsoleWindow* pWnd);
void J3DAPI stdConsole_sub_50058F(StdConsoleWindow* pWnd);

StdConsoleWindow* J3DAPI stdConsole_sub_4FFA86(int x, int y, int width, int height, const char* pTitle, uint8_t topStyle, uint8_t bottomStyle, uint8_t leftStyle, uint8_t rightStyle, WORD attr, uint8_t borderAttr, uint8_t titleAttr, uint8_t titleAlign)
{
    StdConsoleWindow* pWnd;

    pWnd = (StdConsoleWindow*)malloc(sizeof(StdConsoleWindow));
    if ( pWnd == NULL ) {
        return NULL;
    }

    if ( x < 0 )
    {
        width += x;
        x = 0;
    }

    if ( y < 0 )
    {
        height += x;
        y = 0;
    }

    if ( x + width - 1 > 79 ) {
        width = 79 - x + 1;
    }

    if ( y + height - 1 > 24 ) {
        height = 24 - y + 1;
    }

    pWnd->x      = x;
    pWnd->y      = y;
    pWnd->width  = width;
    pWnd->height = height;
    pWnd->right  = x + width - 1;
    pWnd->bottom = y + height - 1;
    strncpy(pWnd->aTitle, pTitle, sizeof(pWnd->aTitle));
    pWnd->topStyle    = topStyle;
    pWnd->bottomStyle = bottomStyle;
    pWnd->leftStyle   = leftStyle;
    pWnd->rightStyle  = rightStyle;
    pWnd->attr        = attr;
    pWnd->borderAttr  = borderAttr;
    pWnd->titleAttr   = titleAttr;
    pWnd->titleAlign  = titleAlign;
    pWnd->size        = width * height;
    pWnd->bufferSize  = (width - 2) * (height - 2);
    pWnd->curX = 0;
    pWnd->curY = 0;
    pWnd->pBuffer = (char*)malloc(pWnd->bufferSize);
    stdConsole_sub_4FFE92(pWnd);
    pWnd->bDrawn = 0;
    return pWnd;
}

void J3DAPI stdConsole_sub_4FFC2C(StdConsoleWindow* pWnd)
{
    free(pWnd->pBuffer);
    free(pWnd);
}

int J3DAPI stdConsole_sub_4FFC4C(int x, int y)
{
    COORD pos;

    pos.X = x;
    pos.Y = y;
    return SetConsoleCursorPosition(stdConsole_hOutput, pos);
}

void J3DAPI stdConsole_sub_4FFC74(COORD* pPos)
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(stdConsole_hOutput, &info);
    pPos->X = info.dwCursorPosition.X;
    pPos->Y = info.dwCursorPosition.Y;
}

void J3DAPI stdConsole_sub_4FFCA4(int bShow)
{
    if ( bShow )
    {
        if ( stdConsole_dword_5DCCCC > 0 )
        {
            stdConsole_dword_5DCCCC--;
            if ( stdConsole_dword_5DCCCC == 0 )
            {
                stdConsole_aUnknown5DCCD8.bVisible = 1;
                SetConsoleCursorInfo(stdConsole_hOutput, &stdConsole_aUnknown5DCCD8);
            }
        }
    }
    else
    {
        if ( stdConsole_dword_5DCCCC == 0 )
        {
            stdConsole_aUnknown5DCCD8.bVisible = 0;
            SetConsoleCursorInfo(stdConsole_hOutput, &stdConsole_aUnknown5DCCD8);
        }

        stdConsole_dword_5DCCCC++;
    }
}

int J3DAPI stdConsole_SetAttributes(WORD wAttributes)
{
    stdConsole_wAttributes = wAttributes;
    return 1;
}

int J3DAPI stdConsole_SetConsoleTextAttribute(WORD wAttributes)
{
    stdConsole_textAttribute = wAttributes;
    return SetConsoleTextAttribute(stdConsole_hOutput, wAttributes);
}

int J3DAPI stdConsole_sub_4FFD53(void)
{
    return FlushConsoleInputBuffer(stdConsole_hInput);
}

int J3DAPI stdConsole_InitOutputConsole(void)
{
    stdConsole_SetConsoleTextAttribute(stdConsole_wAttributes & 0xF0);

    {
    struct { COORD startCoords; DWORD dwNumWritten; } out;
    out.startCoords.X = 0;  // Start at the top-left corner
    out.startCoords.Y = 0;
    FillConsoleOutputCharacter(stdConsole_hOutput, ' ', 2000u, out.startCoords, &out.dwNumWritten);
    return stdConsole_SetConsoleTextAttribute(stdConsole_wAttributes);
}
}

void J3DAPI stdConsole_sub_4FFDBB(int y)
{
    COORD startCoords;
    DWORD dwNumWritten;

    stdConsole_SetConsoleTextAttribute(stdConsole_wAttributes & 0xF0);
    startCoords.X = 0;
    startCoords.Y = y;
    FillConsoleOutputCharacter(stdConsole_hOutput, ' ', 80u, startCoords, &dwNumWritten);
}

int J3DAPI stdConsole_sub_4FFE02(char ch, WORD textAttribute)
{
    DWORD nWritten;

    if ( stdConsole_textAttribute != textAttribute ) {
        stdConsole_SetConsoleTextAttribute(textAttribute);
    }

    return WriteConsole(stdConsole_hOutput, &ch, 1, &nWritten, NULL);
}

int J3DAPI stdConsole_WriteConsole(const char* pText, WORD textAttribute)
{
    DWORD nWritten;

    if ( stdConsole_textAttribute != textAttribute )
    {
        stdConsole_SetConsoleTextAttribute(textAttribute);
    }

    return WriteConsole(stdConsole_hOutput, pText, strlen(pText), &nWritten, NULL);
}

void J3DAPI stdConsole_sub_4FFE92(StdConsoleWindow* pWnd)
{
    memset(pWnd->pBuffer, ' ', pWnd->bufferSize);
    pWnd->curX = 0;
    pWnd->curY = 0;
}

void J3DAPI stdConsole_sub_4FFECC(StdConsoleWindow* pWnd, int row)
{
    memset(pWnd->pBuffer + row * (pWnd->width - 2), ' ', pWnd->width - 2);
}

void J3DAPI stdConsole_sub_4FFEFD(StdConsoleWindow* pWnd)
{
    char hLine;
    char teeRight;
    char teeLeft;
    char topLeft;
    char topRight;
    char botLeft;
    char botRight;
    char botLine;
    char leftSide;
    char rightSide;
    int titleX;
    int titleLen;
    int i;
    int j;

    switch ( pWnd->topStyle )
    {
        case 0:
            hLine    = (char)0xC4;
            teeLeft  = (char)0xC3;
            teeRight = (char)0xB4;
            switch ( pWnd->leftStyle )
            {
                case 0:
                    topLeft = (char)0xDA;
                    break;
                case 1:
                    topLeft = (char)0xD6;
                    break;
            }

            switch ( pWnd->rightStyle )
            {
                case 0:
                    topRight = (char)0xBF;
                    break;
                case 1:
                    topRight = (char)0xB7;
                    break;
            }
            break;

        case 1:
            hLine    = (char)0xCD;
            teeLeft  = (char)0xC6;
            teeRight = (char)0xB5;
            switch ( pWnd->rightStyle )
            {
                case 0:
                    topLeft = (char)0xD5;
                    break;
                case 1:
                    topLeft = (char)0xC9;
                    break;
            }

            switch ( pWnd->rightStyle )
            {
                case 0:
                    topRight = (char)0xB8;
                    break;
                case 1:
                    topRight = (char)0xBB;
                    break;
            }
            break;
    }

    switch ( pWnd->leftStyle )
    {
        case 0:
            leftSide = (char)0xB3;
            break;
        case 1:
            leftSide = (char)0xBA;
            break;
    }

    switch ( pWnd->rightStyle )
    {
        case 0:
            rightSide = (char)0xB3;
            break;
        case 1:
            rightSide = (char)0xBA;
            break;
    }

    switch ( pWnd->bottomStyle )
    {
        case 0:
            botLine = (char)0xC4;
            switch ( pWnd->leftStyle )
            {
                case 0:
                    botLeft = (char)0xC0;
                    break;
                case 1:
                    botLeft = (char)0xD3;
                    break;
            }

            switch ( pWnd->rightStyle )
            {
                case 0:
                    botRight = (char)0xD9;
                    break;
                case 1:
                    botRight = (char)0xBD;
                    break;
            }
            break;

        case 1:
            botLine = (char)0xCD;
            switch ( pWnd->leftStyle )
            {
                case 0:
                    botLeft = (char)0xD4;
                    break;
                case 1:
                    botLeft = (char)0xC8;
                    break;
            }

            switch ( pWnd->rightStyle )
            {
                case 0:
                    botRight = (char)0xBE;
                    break;
                case 1:
                    botRight = (char)0xBC;
                    break;
            }
            break;
    }

    stdConsole_sub_4FFCA4(0);
    stdConsole_sub_4FFC4C(pWnd->x, pWnd->y);
    stdConsole_sub_4FFE02(topLeft, pWnd->borderAttr);
    stdConsole_sub_4FFC4C(pWnd->x, pWnd->y + pWnd->height - 1);
    stdConsole_sub_4FFE02(botLeft, pWnd->borderAttr);

    for ( i = pWnd->x + 1; i < pWnd->x + pWnd->width - 1; ++i )
    {
        stdConsole_sub_4FFC4C(i, pWnd->y);
        stdConsole_sub_4FFE02(hLine, pWnd->borderAttr);
        stdConsole_sub_4FFC4C(i, pWnd->y + pWnd->height - 1);
        stdConsole_sub_4FFE02(botLine, pWnd->borderAttr);
    }

    stdConsole_sub_4FFC4C(i, pWnd->y);
    stdConsole_sub_4FFE02(topRight, pWnd->borderAttr);
    stdConsole_sub_4FFC4C(i, pWnd->y + pWnd->height - 1);
    stdConsole_sub_4FFE02(botRight, pWnd->borderAttr);

    for ( j = pWnd->y + 1; j < pWnd->y + pWnd->height - 1; ++j )
    {
        stdConsole_sub_4FFC4C(pWnd->x, j);
        stdConsole_sub_4FFE02(leftSide, pWnd->borderAttr);
        stdConsole_sub_4FFC4C(pWnd->x + pWnd->width - 1, j);
        stdConsole_sub_4FFE02(rightSide, pWnd->borderAttr);
    }

    titleLen = strlen(pWnd->aTitle);
    if ( titleLen )
    {
        switch ( pWnd->titleAlign )
        {
            case 0:
                titleX = pWnd->x + pWnd->width / 2 - titleLen / 2 - 1;
                break;
            case 1:
                titleX = pWnd->x + 3;
                break;
            case 2:
                titleX = pWnd->right - titleLen - 3;
                break;
        }

        stdConsole_sub_4FFC4C(titleX, pWnd->y);
        stdConsole_sub_4FFE02(teeRight, pWnd->borderAttr);
        stdConsole_sub_4FFC4C(titleX + 1, pWnd->y);
        stdConsole_WriteConsole(pWnd->aTitle, pWnd->titleAttr);
        stdConsole_sub_4FFC4C(titleX + titleLen + 1, pWnd->y);
        stdConsole_sub_4FFE02(teeLeft, pWnd->borderAttr);
    }

    stdConsole_sub_4FFE92(pWnd);
    stdConsole_sub_50058F(pWnd);
    stdConsole_sub_4FFCA4(1);
    pWnd->bDrawn = 1;
}

void J3DAPI stdConsole_sub_50039D(StdConsoleWindow* pWnd, const char* pText, int bMoveCursor)
{
    char* pDest;
    int ch;

    if ( pWnd->width <= 2 ) {
        return;
    }

    if ( pWnd->height <= 2 ) {
        return;
    }

    pDest = pWnd->pBuffer + pWnd->curY * (pWnd->width - 2) + pWnd->curX;
    while ( *pText )
    {
        if ( pWnd->curX >= pWnd->width - 2 )
        {
            pWnd->curY++;
            pWnd->curX = 0;
        }

        if ( pWnd->curY >= pWnd->height - 2 )
        {
            memmove(pWnd->pBuffer, &pWnd->pBuffer[pWnd->width - 2], pWnd->bufferSize - (pWnd->width - 2));
            pWnd->curY--;
            pDest -= pWnd->width - 2;
            stdConsole_sub_4FFECC(pWnd, pWnd->curY);
        }

        ch = *pText++;
        switch ( ch )
        {
            case '\n':
                pWnd->curY++;
                pWnd->curX = 0;
                pDest = pWnd->pBuffer + pWnd->curY * (pWnd->width - 2) + pWnd->curX;
                break;

            default:
                *pDest = ch;
                pDest++;
                pWnd->curX++;
                break;
        }
    }

    stdConsole_sub_50058F(pWnd);
    if ( bMoveCursor ) {
        stdConsole_sub_4FFC4C(pWnd->curX + pWnd->x + 1, pWnd->curY + pWnd->y + 1);
    }
}

void J3DAPI stdConsole_sub_50058F(StdConsoleWindow* pWnd)
{
    char* pSrc;
    int rowLen;
    int line;
    int i;
    COORD cursorPos;
    char aRow[81];

    stdConsole_sub_4FFC74(&cursorPos);
    stdConsole_sub_4FFCA4(0);
    if ( stdConsole_textAttribute != pWnd->attr ) {
        stdConsole_SetConsoleTextAttribute(pWnd->attr);
    }

    pSrc   = pWnd->pBuffer;
    rowLen = pWnd->width - 2;
    line   = pWnd->y + 1;
    aRow[rowLen] = 0;
    for ( i = 0; i < pWnd->height - 2; ++i )
    {
        memcpy(aRow, pSrc, rowLen);
        stdConsole_sub_4FFC4C(pWnd->x + 1, line);
        stdConsole_WriteConsole(aRow, pWnd->attr);
        pSrc += rowLen;
        ++line;
    }

    stdConsole_sub_4FFC4C(cursorPos.X, cursorPos.Y);
    stdConsole_sub_4FFCA4(1);
}

void J3DAPI stdConsole_sub_50068C(int y, const char* pText, WORD textAttr, WORD fillAttr)
{
    int i;
    int len;
    int pad;

    stdConsole_sub_4FFCA4(0);
    len = strlen(pText);
    pad = 40 - len / 2;
    stdConsole_sub_4FFC4C(0, y);
    for ( i = 0; i < pad; ++i ) {
        stdConsole_sub_4FFE02(' ', fillAttr);
    }

    stdConsole_sub_4FFC4C(i, y);
    stdConsole_WriteConsole(pText, textAttr);
    for ( i += len; i < 80; ++i ) {
        stdConsole_sub_4FFE02(' ', fillAttr);
    }

    stdConsole_sub_4FFCA4(1);
}
