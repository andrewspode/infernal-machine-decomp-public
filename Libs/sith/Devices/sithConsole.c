#include "sithConsole.h"

#include <sith/Main/sithCommand.h>

#include <std/General/stdHashtbl.h>
#include <std/General/stdMemory.h>

#include <stdio.h>
#include <string.h>

#define SITHCONSOLE_MAXLINES 32
#define SITHCONSOLE_LINELEN  128

// Retail keeps this TU's zero-initialized storage in one declaration-ordered
// .bss block (0x0056A6B8-0x0056B6F8). The 4-byte hole before aBuffers is
// VC5's 8-byte alignment of that array. Two other 4-byte slots have no
// reference anywhere in the retail image and no alignment explanation; they
// are kept as unused statics so the block keeps its retail offsets. Jedi
// Knight's JK.EXE has the same unreferenced slot between its bInitted and
// maxCmds, so consoleFlags is original storage; neither name is evidenced

static int unusedState0 = 0;
static size_t curFlushIndex = 0;
static size_t endPrintIndex = 0;
static SithConsoleCommand* apCommands = NULL;
static size_t curPrintIndex = 0;
static char aBuffers[SITHCONSOLE_MAXLINES][SITHCONSOLE_LINELEN] = { 0 };
static tHashTable* pCommandTable = NULL;
static int bOpen = 0;
static int bStartup = 0;
static int consoleFlags = 0;
static size_t maxRegisteredCommands = 0;
static size_t numRegistredCommands = 0;
static size_t bufferSize = 0;
static SithConsoleWriteTextFunc pfPrintString = NULL;
static SithConsoleWriteWideTextFunc pfPrintWString = NULL;
static SithConsoleFlushFunc pfFlushFunc = NULL;

int J3DAPI sithConsole_CreateCommandTable(size_t size);
void J3DAPI sithConsole_FreeCommandTable(void);

int J3DAPI sithConsole_Startup(void)
{
#line 101
    SITH_ASSERTREL(!bStartup);

    bufferSize            = 0;
    maxRegisteredCommands = 0;
    numRegistredCommands  = 0;

    pfPrintString  = NULL;
    pfPrintWString = NULL;
    pfFlushFunc    = NULL;

    bStartup = 1;
    return 0;
}

void J3DAPI sithConsole_Shutdown(void)
{
    bufferSize            = 0;
    maxRegisteredCommands = 0;
    numRegistredCommands  = 0;

    pfPrintString  = NULL;
    pfPrintWString = NULL;
    pfFlushFunc    = NULL;

    if ( bOpen ) {
        sithConsole_Close();
    }

    bStartup = 0;
}

int J3DAPI sithConsole_Open(size_t numLines, size_t numCommands)
{
    if ( !bStartup ) {
        return 1;
    }

    if ( bOpen ) {
        return 1;
    }

    memset(aBuffers, 0, numLines * SITHCONSOLE_LINELEN);
    bufferSize = numLines;

    curPrintIndex = endPrintIndex = 0;
    curFlushIndex = 0;

    if ( !sithConsole_CreateCommandTable(numCommands) )
    {
#line 178
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to create console.\n", 0, 0, 0, 0);
        return 1;
    }

    sithCommand_RegisterCommands();

    bOpen = 1;
    return 0;
}

void J3DAPI sithConsole_Close(void)
{
    if ( !bOpen ) {
#line 208
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "sithConsole closed when not opened.\n", 0, 0, 0, 0);
    }

    bufferSize            = 0;
    maxRegisteredCommands = 0;
    numRegistredCommands  = 0;

    sithConsole_FreeCommandTable();
    bOpen = 0;
}

void J3DAPI sithConsole_PrintString(const char* pString)
{
    size_t len;

#line 243
    SITH_ASSERTREL(pString);

    if ( !bOpen ) {
        return;
    }

    if ( pfPrintString )
    {
        pfPrintString(pString);
        return;
    }

    len = strlen(pString);
    if ( len > SITHCONSOLE_LINELEN - 1 )
    {
#line 255
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Console string too long (%d).  Truncated.\n", len, 0, 0, 0);
    }

    curPrintIndex = (curPrintIndex + 1) % bufferSize;
    if ( curPrintIndex == endPrintIndex )
    {
        endPrintIndex = (endPrintIndex + 1) % bufferSize;
    }

    strncpy(aBuffers[curPrintIndex], pString, SITHCONSOLE_LINELEN - 1);
    aBuffers[curPrintIndex][SITHCONSOLE_LINELEN - 1] = '\0';
}

void J3DAPI sithConsole_PrintWString(const wchar_t* pwString)
{
#line 287
    SITH_ASSERTREL(pwString && bOpen);
    if ( pfPrintWString )
    {
        pfPrintWString(pwString);
    }
}

int J3DAPI sithConsole_ExeCommand(const char* pLine)
{
    SithConsoleCommand* pFunc;
    const char* pCmdName;

#line 321
    SITH_ASSERTREL(bOpen);
#line 322
    SITH_ASSERTREL(pLine);

    _strlwr((char*)pLine);

    pCmdName = strtok((char*)pLine, ", \t\n\r");
    if ( !pCmdName ) {
        return 0;
    }

    pFunc = (SithConsoleCommand*)stdHashtbl_Find(pCommandTable, pCmdName);
    if ( pFunc )
    {
#line 340
        SITH_ASSERTREL(pFunc->pfFunc);
        pCmdName = strtok(NULL, "\n\r");
        pFunc->pfFunc(pFunc, pCmdName);
        return 1;
    }

    sprintf(std_genBuffer, "Console command %s not recognized.", pCmdName);
    sithConsole_PrintString(std_genBuffer);
    return 0;
}

void J3DAPI sithConsole_Flush(void)
{
    if ( pfFlushFunc )
    {
        pfFlushFunc();
    }
    else
    {
        while ( curFlushIndex != curPrintIndex )
        {
            curFlushIndex = (curFlushIndex + 1) % bufferSize;
#line 385
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s", aBuffers[curFlushIndex], 0, 0, 0);
        }
    }
}

int J3DAPI sithConsole_RegisterCommand(SithConsoleFunction pfFunc, const char* pName, int flags)
{
    SithConsoleCommand* pCommand;

#line 415
    SITH_ASSERTREL(pfFunc && pName);
    if ( numRegistredCommands == maxRegisteredCommands )
    {
#line 419
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to register > %d console funcs.\n", maxRegisteredCommands, 0, 0, 0);
        return 0;
    }

    pCommand = (SithConsoleCommand*)stdHashtbl_Find(pCommandTable, pName);
    if ( pCommand )
    {
        // TODO: Add log for func getting overridden
        pCommand->pfFunc = pfFunc;
        pCommand->flags  = flags;
        return 1;
    }

    strncpy(apCommands[numRegistredCommands].aName, pName, sizeof(apCommands[numRegistredCommands].aName) - 1);
    apCommands[numRegistredCommands].aName[sizeof(apCommands[numRegistredCommands].aName) - 1] = '\0';

    apCommands[numRegistredCommands].pfFunc = pfFunc;
    apCommands[numRegistredCommands].flags  = flags;

    stdHashtbl_Add(
        pCommandTable,
        apCommands[numRegistredCommands].aName,
        &apCommands[numRegistredCommands]
    );
    ++numRegistredCommands;
    return 1;
}

void J3DAPI sithConsole_RegisterPrintFunctions(SithConsoleWriteTextFunc pfWriteText, SithConsoleWriteWideTextFunc pfWriteWText, SithConsoleFlushFunc pfFlush)
{
    pfPrintString  = pfWriteText;
    pfPrintWString = pfWriteWText;
    pfFlushFunc    = pfFlush;
}

#ifdef J3D_DEBUG
/* Debug-only: Jedi Knight's sithConsole.c (0x004DA1D0) has sithConsole_Help right after RegisterPrintFunctions, and
   Indy's debug build keeps it there (0x00456D03); only the debug build's sithCommand_RegisterCommands registers "help". */
int J3DAPI sithConsole_Help(const SithConsoleCommand* pFunc, const char* pArg)
{
    char aLine[56] = "\0";
    size_t nameLen;
    size_t pos = 0;
    size_t i;

    sithConsole_PrintString("The following commands are available:");
    for ( i = 0; i < numRegistredCommands; ++i )
    {
        nameLen = strlen(apCommands[i].aName);
        if ( pos + nameLen + 5 >= sizeof(aLine) )
        {
            sithConsole_PrintString(aLine);
            pos = 0;
        }

        pos += sprintf(&aLine[pos], "    %s", apCommands[i].aName);
    }

    sithConsole_PrintString(aLine);
    return 1;
}
#endif

int J3DAPI sithConsole_CreateCommandTable(size_t size)
{
#line 525
    SITH_ASSERTREL(size > 0);
#line 528
    apCommands = (SithConsoleCommand*)STDMALLOC(sizeof(SithConsoleCommand) * size);
    pCommandTable = stdHashtbl_New(2 * size);

    if ( !apCommands || !pCommandTable )
    {
    }
    else
    {
        memset(apCommands, 0, sizeof(SithConsoleCommand) * size);
        maxRegisteredCommands = size;
        return 1;
    }

    if ( apCommands )
    {
        stdMemory_Free(apCommands);
        apCommands = NULL;
    }

    if ( pCommandTable )
    {
        stdHashtbl_Free(pCommandTable);
        pCommandTable = NULL;
    }

    return 0;
}

void J3DAPI sithConsole_FreeCommandTable(void)
{
    if ( apCommands )
    {
        stdMemory_Free(apCommands);
        apCommands = NULL;
    }

    if ( pCommandTable )
    {
        stdHashtbl_Free(pCommandTable);
        pCommandTable = NULL;
    }
}
