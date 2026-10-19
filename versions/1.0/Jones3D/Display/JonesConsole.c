#include "JonesConsole.h"
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Play/jonesCog.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Primitives/rdModel3K.h>

#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithTime.h>

#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdPlatform.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>

#include <stdio.h>
#include <string.h>

#define JONESCONSOLE_MAXLINES      15
#define JONESCONSOLE_COMMANDLINEID 103

// Retail compiled the whole TU without optimization: EBP frames, stack homes
// for every temporary and 0xCC alignment fill (intrinsics stay enabled).
// Assertions are STD_ASSERTREL, i.e. MSVC <assert.h>'s `(void)(cond || (assert, 0))`
// shape: only it reproduces retail's /Od register rotation after the assert call

#pragma optimize("", off)

// Build stamp printed by the "version" command (retail .rdata 0x00506398).
const char JonesConsole_aVersion[128] = "Oct 29 1999,12:09:14 Release";

char JonesConsole_aCmdLine[128] = { 0 };
JonesConsoleTextLine JonesConsole_aBuffers[JONESCONSOLE_MAXLINES] = { 0 };
int JonesConsole_nextIndex = 0;
static int JonesConsole_unusedState0 = 0; // Unreferenced retail slot
char JonesConsole_aUnused_552770[2304] = { 0 };
unsigned int JonesConsole_cursorPos = 0;
static int JonesConsole_unusedState1 = 0; // Unreferenced retail slot
rdFont* JonesConsole_pFont = NULL;
int jonesConsole_bVisible = 0;
int jonesConsole_bStarted = 0;
int JonesConsole_bOpened = 0;
tHashTable* pFuncHashtbl = NULL;

void J3DAPI JonesConsole_FlushToDisplay(void);
int J3DAPI JonesConsole_ExeCommand(const char* pLine);
void J3DAPI JonesConsole_FlushCommandLine(void);
int J3DAPI JonesConsole_PrintVersion(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_PrintFramerate(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_PrintPolys(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_JumpLevel(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_Radius(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_Sounds(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_KatOn(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_KatOff(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_PVS(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_NextLevel(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_EndLevel(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_Restart(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_ShowEndCredits(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI JonesConsole_RemoveExpired(void);

int J3DAPI JonesConsole_Startup(void)
{
#line 141
    STD_ASSERTREL(!jonesConsole_bStarted);

    if ( sithConsole_Startup() )
    {
        return 1;
    }

    sithConsole_RegisterPrintFunctions((SithConsoleWriteTextFunc)JonesConsole_PrintText, NULL, JonesConsole_FlushToDisplay);
    memset(JonesConsole_aUnused_552770, 0, sizeof(JonesConsole_aUnused_552770));
    JonesConsole_pFont    = NULL;
    jonesConsole_bStarted = 1;
    JonesConsole_bOpened  = 0;
    return 0;
}

void J3DAPI JonesConsole_Shutdown(void)
{
    if ( JonesConsole_bOpened )
    {
        JonesConsole_Close();
    }

    sithConsole_Shutdown();
    jonesConsole_bStarted = 0;
}

int J3DAPI JonesConsole_Open(void)
{
    if ( JonesConsole_bOpened )
    {
        return 0;
    }

    if ( sithConsole_Open(16, 64) )
    {
        return 1;
    }

    pFuncHashtbl = stdHashtbl_New(64);
#line 196
    STD_ASSERTREL(pFuncHashtbl);

    sithConsole_RegisterCommand(JonesConsole_PrintVersion, "version", 0);
    sithConsole_RegisterCommand(JonesConsole_PrintFramerate, "framerate", 0);
    sithConsole_RegisterCommand(JonesConsole_PrintPolys, "polys", 0);
    sithConsole_RegisterCommand(JonesConsole_ShowEndCredits, "endcredit", 0);
#ifdef J3D_DEBUG
    sithConsole_RegisterCommand(JonesConsole_JumpLevel, "jumplevel", 0);
    sithConsole_RegisterCommand(JonesConsole_Radius, "radius", 0);
    sithConsole_RegisterCommand(JonesConsole_KatOn, "katon", 0);
    sithConsole_RegisterCommand(JonesConsole_KatOff, "katoff", 0);
    sithConsole_RegisterCommand(JonesConsole_PVS, "pvs", 0);
    sithConsole_RegisterCommand(JonesConsole_NextLevel, "nextlevel", 0);
    sithConsole_RegisterCommand(JonesConsole_EndLevel, "endlevel", 0);
    sithConsole_RegisterCommand(JonesConsole_Restart, "restart", 0);
    sithConsole_RegisterCommand(JonesConsole_Sounds, "sounds", 0);
#endif

    JonesConsole_nextIndex = 0;
    JonesConsole_pFont = rdFont_Load("mat\\jonesBook Antiqua24.gcf");
    if ( !JonesConsole_pFont )
    {
        return 0;
    }

    JonesConsole_bOpened = 1;
    return 0;
}

void J3DAPI JonesConsole_Close(void)
{
    if ( !JonesConsole_bOpened )
    {
        return;
    }

    sithConsole_Close();
    if ( JonesConsole_pFont )
    {
        rdFont_Free(JonesConsole_pFont);
        JonesConsole_pFont = NULL;
    }

    if ( pFuncHashtbl )
    {
        stdHashtbl_Free(pFuncHashtbl);
        pFuncHashtbl = NULL;
    }

    JonesConsole_bOpened = 0;
}

void J3DAPI JonesConsole_FlushToDisplay(void)
{
    float xPos;
    int line;
    rdFontColor textColor;
    uint32_t bufWidth;
    float yPos;
    uint32_t bufHeight;
    const char* pWrapEnd;
    const char* pText;
    char aText[256];

    if ( !JonesConsole_pFont )
    {
        return;
    }

    JonesConsole_RemoveExpired();
    stdDisplay_GetBackBufferSize(&bufWidth, &bufHeight);

    xPos = D3DVAL(4.0 / bufWidth);
    yPos = 0.0f;

    textColor[0].red   = 1.0f;
    textColor[0].green = 0.0f;
    textColor[0].blue  = 0.0f;
    textColor[0].alpha = 1.0f;
    textColor[1].red   = 1.0f;
    textColor[1].green = 0.0f;
    textColor[1].blue  = 0.0f;
    textColor[1].alpha = 1.0f;
    textColor[2].red   = 1.0f;
    textColor[2].green = 1.0f;
    textColor[2].blue  = 0.0f;
    textColor[2].alpha = 1.0f;
    textColor[3].red   = 1.0f;
    textColor[3].green = 1.0f;
    textColor[3].blue  = 0.0f;
    textColor[3].alpha = 1.0f;
    rdFont_SetFontColor(textColor);

    line = 0;
    while ( line < JonesConsole_nextIndex && line < JONESCONSOLE_MAXLINES )
    {
        pText = JonesConsole_aBuffers[line].aLine;
        while ( pText )
        {
            pWrapEnd = rdFont_GetWrapLine(pText, JonesConsole_pFont, 1.0f);
            if ( pWrapEnd == pText )
            {
                pWrapEnd = NULL;
            }

            if ( !pWrapEnd )
            {
                rdFont_DrawTextLine(pText, xPos, yPos, rdCamera_pCurCamera->pFrustum->nearPlane, JonesConsole_pFont, RDFONT_ALIGNLEFT);
                pText = NULL;
            }
            else
            {
                memset(aText, 0, sizeof(aText));
                strncpy(aText, pText, pWrapEnd - pText);
                rdFont_DrawTextLine(aText, xPos, yPos, rdCamera_pCurCamera->pFrustum->nearPlane, JonesConsole_pFont, RDFONT_ALIGNLEFT);
                pText = pWrapEnd;
            }

            yPos = (float)JonesConsole_pFont->lineSpacing / RD_REF_HEIGHT + yPos;
        }

        ++line;
    }
}

int J3DAPI JonesConsole_PrintText(const char* pText)
{
    int nextIndex;
    int i;
    int index;

    if ( JonesConsole_nextIndex >= JONESCONSOLE_MAXLINES )
    {
        index = 0;
        for ( i = 0; i < JONESCONSOLE_MAXLINES; ++i )
        {
            if ( JonesConsole_aBuffers[i].msecExpireTime != -1 )
            {
                index = i;
                break;
            }
        }

        JonesConsole_aBuffers[index].msecExpireTime = 0;
        JonesConsole_RemoveExpired();
    }

    nextIndex = JonesConsole_nextIndex;
#line 397
    STD_ASSERTREL(STD_INRANGE(nextIndex, 0, JONESCONSOLE_MAXLINES-1));

    strncpy(JonesConsole_aBuffers[nextIndex].aLine, pText, sizeof(JonesConsole_aBuffers[nextIndex].aLine));
    JonesConsole_aBuffers[nextIndex].ID = 0;
    JonesConsole_aBuffers[nextIndex].msecExpireTime = stdPlatform_GetTimeMsec() + 10000;
    JonesConsole_nextIndex++;
    return nextIndex;
}

void J3DAPI JonesConsole_PrintTextWithID(int ID, const char* pText)
{
    int bExists;
    int i;
    int index;

    bExists = 0;
    for ( i = 0; i < JonesConsole_nextIndex && !bExists; ++i )
    {
        if ( JonesConsole_aBuffers[i].ID == ID )
        {
            if ( strcmp(JonesConsole_aBuffers[i].aLine, pText) )
            {
                strncpy(JonesConsole_aBuffers[i].aLine, pText, sizeof(JonesConsole_aBuffers[i].aLine));
            }

            bExists = 1;
        }
    }

    if ( !bExists )
    {
        index = JonesConsole_PrintText(pText);
        JonesConsole_aBuffers[index].ID = ID;
        JonesConsole_aBuffers[index].msecExpireTime = (unsigned int)-1;
    }
}

void J3DAPI JonesConsole_ClearText(int yourID)
{
    int i;

#line 478
    STD_ASSERTREL(yourID != 0);

    for ( i = 0; i < JonesConsole_nextIndex; ++i )
    {
        if ( JonesConsole_aBuffers[i].ID == yourID )
        {
            break;
        }
    }

    if ( i < JonesConsole_nextIndex )
    {
        JonesConsole_aBuffers[i].msecExpireTime = 0;
    }
}

int J3DAPI JonesConsole_ExeCommand(const char* pLine)
{
    char aLine[128];
    SithConsoleCommand* pFunc;
    char* pToken;

#line 519
    STD_ASSERTREL(pLine);

    strncpy(aLine, pLine, sizeof(aLine) - 1);
    aLine[sizeof(aLine) - 1] = 0;
    _strlwr(aLine);

    pToken = strtok(aLine, ", \t\n\r");
    if ( !pToken )
    {
        return 0;
    }

    pFunc = (SithConsoleCommand*)stdHashtbl_Find(pFuncHashtbl, pToken);
    if ( pFunc )
    {
#line 543
        STD_ASSERTREL(pFunc->pfFunc);

        pToken = strtok(NULL, "\n\r");
        pFunc->pfFunc(pFunc, pToken);
        return 1;
    }

    return 0;
}

void J3DAPI JonesConsole_ShowConsole(void)
{
    jonesConsole_bVisible = 1;
    strncpy(JonesConsole_aCmdLine, "] ", 3);
    JonesConsole_cursorPos = 2;
    stdControl_SetActivation(0);
    JonesConsole_FlushCommandLine();
}

void J3DAPI JonesConsole_HandelChar(char chr)
{
    if ( chr == '\r' )
    {
        if ( JonesConsole_cursorPos > 2 && !JonesConsole_ExeCommand(&JonesConsole_aCmdLine[2]) )
        {
            sithConsole_ExeCommand(&JonesConsole_aCmdLine[2]);
        }

        JonesConsole_HideConsole();
        return;
    }

    if ( chr == '\b' )
    {
        if ( JonesConsole_cursorPos > 2 )
        {
            JonesConsole_aCmdLine[--JonesConsole_cursorPos] = 0;
        }
    }
    else if ( JonesConsole_cursorPos < 127 )
    {
        JonesConsole_aCmdLine[JonesConsole_cursorPos]     = chr;
        JonesConsole_aCmdLine[JonesConsole_cursorPos + 1] = 0;
        JonesConsole_cursorPos++;
    }

    JonesConsole_FlushCommandLine();
}

void J3DAPI JonesConsole_FlushCommandLine(void)
{
    JonesConsole_PrintTextWithID(JONESCONSOLE_COMMANDLINEID, JonesConsole_aCmdLine);
}

void J3DAPI JonesConsole_HideConsole(void)
{
    JonesConsole_cursorPos  = 0;
    jonesConsole_bVisible = 0;
    JonesConsole_ClearText(JONESCONSOLE_COMMANDLINEID);
    stdControl_SetActivation(1);
}

int J3DAPI JonesConsole_PrintVersion(const SithConsoleCommand* pFunc, const char* pArg)
{
    sithConsole_PrintString(JonesConsole_aVersion);
    return 1;
}

int J3DAPI JonesConsole_PrintFramerate(const SithConsoleCommand* pFunc, const char* pArg)
{
    JonesConsole_ClearText(JONESCONSOLE_FRAMERATEID);
    JonesMain_TogglePrintFramerate();
    return 1;
}

int J3DAPI JonesConsole_PrintPolys(const SithConsoleCommand* pFunc, const char* pArg)
{
    sprintf(std_genBuffer, "Arch polys: %d", sithRender_numArchPolys);
    sithConsole_PrintString(std_genBuffer);
    sprintf(std_genBuffer, "Alpha Arch polys: %d", sithRender_numAlphaArchPolys);
    sithConsole_PrintString(std_genBuffer);
    sprintf(std_genBuffer, "Thing polys: %d", sithRender_numThingPolys);
    sithConsole_PrintString(std_genBuffer);
    sprintf(std_genBuffer, "Alpha Thing polys: %d", sithRender_numAlphaThingPoly);
    sithConsole_PrintString(std_genBuffer);
    return 1;
}

#ifdef J3D_DEBUG
/* Debug-build console commands: only the debug build's Open registers them ("jumplevel", "radius", "katon", "katoff",
   "pvs", "nextlevel", "endlevel", "restart", "sounds"; 0x00413256, 0x004132AC, 0x00413308, 0x0041331C, 0x00413330,
   0x0041333F, 0x0041334E, 0x0041335D, 0x004132C7), and retail has none of them. Bodies from the debug code; names from
   Jones3DRE. */
int J3DAPI JonesConsole_JumpLevel(const SithConsoleCommand* pFunc, const char* pArg)
{
    int levelNum;

    if ( !pArg )
    {
        sithConsole_PrintString("Level Number?");
        return 0;
    }

    if ( !sscanf(pArg, "%d", &levelNum) )
    {
        return 0;
    }

    ++levelNum;
    JonesMain_JumpLevel(levelNum);
    return 1;
}

int J3DAPI JonesConsole_Radius(const SithConsoleCommand* pFunc, const char* pArg)
{
    sith_bDrawPlayerMoveBounds = !sith_bDrawPlayerMoveBounds;
    return 1;
}

int J3DAPI JonesConsole_Sounds(const SithConsoleCommand* pFunc, const char* pArg)
{
#line 741
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Game time %f\n", sithTime_secGameTime, 0, 0, 0);
    Sound_SoundDump();
    return 1;
}

int J3DAPI JonesConsole_KatOn(const SithConsoleCommand* pFunc, const char* pArg)
{
    rdModel3K_SetEnabled(1);
    return 1;
}

int J3DAPI JonesConsole_KatOff(const SithConsoleCommand* pFunc, const char* pArg)
{
    rdModel3K_SetEnabled(0);
    return 1;
}

int J3DAPI JonesConsole_PVS(const SithConsoleCommand* pFunc, const char* pArg)
{
    sithRender_TogglePVS();
    return 1;
}

int J3DAPI JonesConsole_NextLevel(const SithConsoleCommand* pFunc, const char* pArg)
{
    JonesMain_NextLevel();
    return 1;
}

int J3DAPI JonesConsole_EndLevel(const SithConsoleCommand* pFunc, const char* pArg)
{
    jonesCog_EndLevel();
    return 1;
}

int J3DAPI JonesConsole_Restart(const SithConsoleCommand* pFunc, const char* pArg)
{
    JonesMain_RestartLevel();
    return 1;
}
#endif

int J3DAPI JonesConsole_ShowEndCredits(const SithConsoleCommand* pFunc, const char* pArg)
{
    JonesMain_ShowEndCredits();
    return 1;
}

int J3DAPI JonesConsole_RemoveExpired(void)
{
    int writeIdx;
    unsigned int msecCurTime;
    int bFound;
    int i;

    writeIdx = 0;
    msecCurTime    = stdPlatform_GetTimeMsec();
    bFound  = 0;
    for ( i = 0; i < JonesConsole_nextIndex; ++i )
    {
        if ( JonesConsole_aBuffers[i].msecExpireTime > msecCurTime )
        {
            if ( writeIdx != i )
            {
                strncpy(JonesConsole_aBuffers[writeIdx].aLine, JonesConsole_aBuffers[i].aLine, sizeof(JonesConsole_aBuffers[writeIdx].aLine));
                JonesConsole_aBuffers[writeIdx].msecExpireTime = JonesConsole_aBuffers[i].msecExpireTime;
                JonesConsole_aBuffers[writeIdx].ID             = JonesConsole_aBuffers[i].ID;
                bFound = 1;
            }

            ++writeIdx;
        }
    }

    JonesConsole_nextIndex = writeIdx;
    return bFound;
}
