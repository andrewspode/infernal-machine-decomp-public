
#define STRICT
#include <Windows.h> // timeGetTime, include first to avoid undefined warning due to WIN32_LEAN_AND_MEAN

#include "std.h"
#include "stdMemory.h"
#include "stdPlatform.h"
#include "stdUtil.h"

#include <stdio.h>

static char std_filePrintBuffer[2048] = { 0 };
static int stdPlatform_bAssert = 0;

void J3DAPI stdPlatform_UnlockHandle(void)
{

}

int J3DAPI stdPlatform_InitServices(tHostServices* pHS)
{
    pHS->unknown1 = 1000.0f; // sec to msec converter constant

    pHS->pMessagePrint = stdPlatform_Printf;
    pHS->pStatusPrint  = stdPlatform_Printf;
    pHS->pWarningPrint = stdPlatform_Printf;
    pHS->pErrorPrint   = stdPlatform_Printf;
    pHS->pDebugPrint   = stdPlatform_Printf;

    pHS->pAssert = stdPlatform_Assert;
    pHS->pAtExit = NULL;

    pHS->pMalloc  = stdMemory_BlockMalloc;
    pHS->pFree    = stdMemory_BlockFree;
    pHS->pRealloc = stdMemory_BlockRealloc;

    pHS->pGetTimeMsec = stdPlatform_GetTimeMsec;

    pHS->pFileOpen      = stdFileOpen;
    pHS->pFileClose     = stdFileClose;
    pHS->pFileRead      = stdFileRead;
    pHS->pFileGets      = stdFileGets;
    pHS->pFileWrite     = stdFileWrite;
    pHS->pFileEOF       = stdFileEof;
    pHS->pFileTell      = stdFileTell;
    pHS->pFileSeek      = stdFileSeek;
    pHS->pFileSize      = stdFileSize;
    pHS->pFilePrintf    = stdFilePrintf;
    pHS->pFileGetws     = stdFileGetws;

    pHS->pAllocHandle   = stdPlatform_AllocHandle;
    pHS->pFreeHandle    = stdPlatform_FreeHandle;
    pHS->pReallocHandle = stdPlatform_ReallocHandle;

    pHS->pLockHandle   = stdPlatform_LockHandle;
    pHS->pUnlockHandle = stdPlatform_UnlockHandle;

    return CoInitialize(0);
}

void J3DAPI stdPlatform_ClearServices(tHostServices* pHS)
{
    memset(pHS, 0, sizeof(tHostServices));
    CoUninitialize();
}

unsigned int J3DAPI stdPlatform_GetTimeMsec(void)
{
    return timeGetTime(); // TODO: Use more precise timer
}

/* Linker-discarded (no caller in either image): the Indy debug build's local date/time formatter between GetTimeMsec and
   Assert (0x004FC88A, assert at original line 200), restored from its code; name inferred. */
void J3DAPI stdPlatform_GetDateTimeString(char* pResult, size_t resultSize)
{
    SYSTEMTIME time;
    char aTime[80];

#line 200
    STD_ASSERT(pResult != NULL);

    GetLocalTime(&time);
    sprintf(aTime, "%d/%d/%02d %d:%02d%s", time.wMonth, time.wDay, time.wYear, time.wHour > 12 ? time.wHour - 12 : time.wHour, time.wMinute, time.wHour > 12 ? "pm" : "am");
    if ( strlen(aTime) > resultSize )
    {
        strncpy(pResult, aTime, resultSize - 1);
        pResult[resultSize - 1] = '\0';
    }
    else
    {
        strcpy(pResult, aTime);
    }
}

J3DNORETURN void J3DAPI stdPlatform_Assert(const char* pErrorStr, const char* pFilename, int linenum)
{
    size_t fnpos = 0;
    int bFoundSep = 0;
    size_t pos;
    char aText[512];

    if ( stdPlatform_bAssert )
    {
        DebugBreak();
        exit(1);
    }
    stdPlatform_bAssert = 1;

    pos = 0;
    while ( pFilename[pos] )
    {
        if ( pFilename[pos] == '\\' )
        {
            bFoundSep = 1;
            fnpos     = pos;
        }

        ++pos;
    }

    if ( bFoundSep )
    {
        ++fnpos;
    }

    
    _snprintf(aText, STD_ARRAYLEN(aText), "%s(%d):  %s\n", &pFilename[fnpos], linenum, pErrorStr);
    std_pHS->pErrorPrint("ASSERT: %s", aText);
    MessageBoxA(NULL, aText, "Assert Handler", MB_TASKMODAL);

    DebugBreak();
    exit(1);
}

int J3DAPI stdPlatform_Printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    _vsnprintf(std_genBuffer, sizeof(std_genBuffer), format, args);
    va_end(args);

    OutputDebugStringA(std_genBuffer); // TODO: write also to printf
    return 1;
}

void* J3DAPI stdPlatform_AllocHandle(size_t size)
{
    return malloc(size);
}

void J3DAPI stdPlatform_FreeHandle(void* pData)
{
    free(pData);
}

void* J3DAPI stdPlatform_ReallocHandle(void* pMemory, size_t newSize)
{
    return realloc(pMemory, newSize);
}

int J3DAPI stdPlatform_LockHandle(int a1)
{
    return a1;
}

tFileHandle J3DAPI stdFileOpen(const char* pFilename, const char* mode)
{
    FILE* fh;
    fh = fopen(pFilename, mode);
    return (tFileHandle)fh;
}

int J3DAPI stdFileClose(tFileHandle fh)
{
    fclose((FILE*)fh);
    return 0;
}

size_t J3DAPI stdFileRead(tFileHandle fh, void* pOutData, size_t size)
{
    size_t nRead = fread(pOutData, 1u, size, (FILE*)fh);
    return nRead;
}

size_t J3DAPI stdFileWrite(tFileHandle fh, const void* pData, size_t size)
{
    size_t nWritten = fwrite(pData, 1u, size, (FILE*)fh);
    return nWritten;
}

char* J3DAPI stdFileGets(tFileHandle fh, char* pStr, size_t size)
{
    char* pResult = fgets(pStr, size, (FILE*)fh);
    return pResult;
}

wchar_t* J3DAPI stdFileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size)
{
    wchar_t* pResult = fgetws(pOutStr, size, (FILE*)fh);
    return pResult;
}

int J3DAPI stdFileEof(tFileHandle fh)
{
    return feof((FILE*)fh);
}

int J3DAPI stdFileTell(tFileHandle fh)
{
    return ftell((FILE*)fh);
}

size_t J3DAPI stdFileSize(const char* pFilePath)
{
    int size;

    tFileHandle fh = stdFileOpen(pFilePath, "rb");
    if ( !fh ) {
        return 0;
    }

    stdFileSeek(fh, 0, 2);
    size = stdFileTell(fh);
    stdFileClose(fh);
    return size;
}

int J3DAPI stdFilePrintf(tFileHandle fh, const char* pFormat, ...)
{
    int size;

    va_list args;
    va_start(args, pFormat);
    size = _vsnprintf(std_filePrintBuffer, sizeof(std_filePrintBuffer), pFormat, args);
    stdFileWrite(fh, std_filePrintBuffer, size);
    va_end(args);
    return 0;
}

int J3DAPI stdFileSeek(tFileHandle fh, int offset, int origin)
{
    return fseek((FILE*)fh, offset, origin);
}
