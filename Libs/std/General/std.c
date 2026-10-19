#include <stdio.h>

#include "std.h"
#include "stdMemory.h"
#include "stdUtil.h"

#include <std/Win95/stdConsole.h>

static char std_printBuffer[2048] = { 0 };
static int std_bStdStartup = 0;
tHostServices* std_pHS;
char std_genBuffer[2048];

// The x87 control words stdStartup prepares; stdMath.c's tentative definitions own the storage
// (retail's common tail places them after stdDisplay's common, at a std object pulled after rank
// 88 - std.c, rank 80, would have placed them first).
extern int16_t word_183962C;
extern int16_t word_183962E;
extern int16_t word_1839630;

void J3DAPI stdStartup(tHostServices* pHS)
{
    std_pHS = pHS;

    if ( std_bStdStartup )
    {
        return;
    }

    stdMemory_Startup();
    stdMemory_Open();

    // Original hand-written FPU control-word setup. The saved
    // word's low byte is kept and its
    // high byte is replaced outright: 0x0B gives RC=10 (round up) with PC=11 and
    // 0x07 gives RC=01 (round down) with PC=11. The stdMath.h rounding helpers
    // load word_1839630 for ceilf, word_183962E for floorf and lrintf, and
    // word_183962C to restore the startup mode. The trailing reload of the saved
    // word has no consumer: an original oversight with no C expression.
    __asm fnstcw word ptr[word_183962C]
    __asm mov ax, word ptr[word_183962C]
    __asm mov ah, 0Bh
    __asm mov word ptr[word_1839630], ax
    __asm mov ax, word ptr[word_183962C]
    __asm mov ah, 07h
    __asm mov word ptr[word_183962E], ax
    __asm mov ax, word ptr[word_183962C]

    std_bStdStartup = 1;
}

void J3DAPI stdShutdown(void)
{
    if ( !std_bStdStartup )
    {
        return;
    }

    __asm fldcw word ptr[word_183962C]

    stdMemory_Close();
    stdMemory_Shutdown();
    std_bStdStartup = 0;
}

int J3DAPI stdPrintf(tPrintfFunc pfPrint, const char* pFilePath, unsigned int linenum, const char* format, ...)
{
    size_t i;
    size_t fnIdx = 0;
    int bFnFound = 0;
    int nTotalWritten;
    size_t remain;
    int nWritten;
    va_list args;

    if ( pfPrint )
    {
        va_start(args, format);

        i = 0;
        while ( pFilePath[i] )
        {
            if ( pFilePath[i] == '\\' )
            {
                bFnFound = 1;
                fnIdx = i;
            }

            ++i;
        }

        if ( bFnFound )
        {
            ++fnIdx;
        }

        // Format string: <filename>(linenum): format
        nTotalWritten = 0;
        remain = sizeof(std_printBuffer);
        nWritten = _snprintf(&std_printBuffer[nTotalWritten], remain, &pFilePath[fnIdx]);
        remain -= nWritten;
        nTotalWritten += nWritten;
        nWritten = _snprintf(&std_printBuffer[nTotalWritten], remain, "(%d): ", linenum);
        remain -= nWritten;
        nTotalWritten += nWritten;
        _vsnprintf(&std_printBuffer[nTotalWritten], remain, format, args);
        va_end(args);

        // Write to output function
        pfPrint(std_printBuffer);
    }

    return 0;
}

int J3DAPI stdConsolePrintf(const char* pFormat, ...)
{
    va_list args;
    va_start(args, pFormat);
    _vsnprintf(std_genBuffer, STD_ARRAYLEN(std_genBuffer), pFormat, args);
    va_end(args);

    stdConsole_WriteConsole(std_genBuffer, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    return sizeof(std_genBuffer);
}

const char* J3DAPI stdFileFromPath(const char* pPath)
{
    const char* pFilename = pPath;
    pFilename = strrchr(pPath, '\\');
    if ( pFilename )
    {
        return pFilename + 1;
    }

    return pPath;
}

int J3DAPI stdCalcBitPos(int bit)
{
    int pos = 0;
    while ( bit > 1 )
    {
        bit >>= 1;
        pos++;
    }

    return pos;
}
