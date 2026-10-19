#include "std.h"
#include "stdConffile.h"
#include "stdMemory.h"


StdConffileEntry stdConffile_entry;
char* stdConffile_aLine;

#define STDCONFFILE_STACKSIZE 20
#define STDCONFFILE_FILENAMESIZE 128

static int stdConffile_bOpen = 0;

static tFileHandle openFile  = 0;
static tFileHandle writeFile = 0;

static size_t stackLevel = 0;
static char* apBufferStack[STDCONFFILE_STACKSIZE];
static tFileHandle openFileStack[STDCONFFILE_STACKSIZE];
static StdConffileEntry aEntryStack[STDCONFFILE_STACKSIZE];
static char aFilenameStack[STDCONFFILE_STACKSIZE][STDCONFFILE_FILENAMESIZE];

static char stdConffile_pFilename[STDCONFFILE_FILENAMESIZE];
static char stdConffile_aWriteFilename[STDCONFFILE_FILENAMESIZE];

static size_t stdConffile_linenum;
static size_t linenumStack[STDCONFFILE_STACKSIZE];

/* The format buffer remains in retail BSS even though its only user
   (stdConffile_Printf, below) was linker-discarded; The
   storage is retail's 0x005D6288. */
static char printBuffer[4096];

void J3DAPI stdConffile_PushStack(void);
void J3DAPI stdConffile_PopStack(void);

int J3DAPI stdConffile_Open(const char* pFilename)
{
    return stdConffile_OpenMode(pFilename, "r");
}

int J3DAPI stdConffile_OpenWrite(const char* pFilename)
{
    if ( writeFile ) {
        return 0;
    }

    writeFile = std_pHS->pFileOpen(pFilename, "wb");
    if ( writeFile <= 0 )
    {
        writeFile = 0;
        return 0;
    }

    strncpy(stdConffile_aWriteFilename, pFilename, sizeof(stdConffile_aWriteFilename) - 1);
    stdConffile_aWriteFilename[sizeof(stdConffile_aWriteFilename) - 1] = 0;
    return 1;
}

int J3DAPI stdConffile_OpenMode(const char* pFilename, const char* openMode)
{
    if ( stdConffile_bOpen ) {
        stdConffile_PushStack();
    }

#line 151
    STD_ASSERTREL(openFile == 0);
    if ( strcmp(pFilename, "none") != 0 )
    {
        openFile = std_pHS->pFileOpen(pFilename, openMode);
        if ( openFile <= 0 )
        {
#ifdef J3D_DEBUG
#line 159
            stdPrintf(std_pHS->pDebugPrint, J3D_FILE, __LINE__, "Conffile '%s' could not be opened (handle=%d)\n", pFilename, openFile, 0, 0);
#endif
            openFile = 0;
            if ( stdConffile_bOpen ) {
                stdConffile_PopStack();
            }
            return 0;
        }
    }
    else {
        openFile = 0;
    }

#line 174
    stdConffile_aLine = (char*)STDMALLOC(STDCONFFILE_LINESIZE);
    strncpy(stdConffile_pFilename, pFilename, sizeof(stdConffile_pFilename) - 1);
    stdConffile_pFilename[sizeof(stdConffile_pFilename) - 1] = 0;

    stdConffile_linenum = 0;
    stdConffile_bOpen   = 1;
    return 1;
}

void J3DAPI stdConffile_Close(void)
{
    if ( stdConffile_bOpen )
    {
#line 201
        STD_ASSERTREL(stdConffile_aLine != NULL);
        if ( openFile ) {
            std_pHS->pFileClose(openFile);
        }
        openFile = 0;
        stdMemory_Free(stdConffile_aLine);
        if ( !stackLevel ) {
            stdConffile_bOpen = 0;
        }
        else {
            stdConffile_PopStack();
        }
    }
    else
    {
#line 224
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
    }
}

void J3DAPI stdConffile_CloseWrite(void)
{
    if ( writeFile )
    {
        std_pHS->pFileClose(writeFile);
        writeFile = 0;
        strncpy(stdConffile_aWriteFilename, "NOT_OPEN", sizeof(stdConffile_aWriteFilename) - 1);
        stdConffile_aWriteFilename[sizeof(stdConffile_aWriteFilename) - 1] = 0;
    }
}

/* Linker-discarded: Jedi Knight (JK.EXE 0x00431420) and MotS (0x004351F0) define it between CloseWrite and Write;
   Indy's debug build keeps it there (0x004F0DAA). Retail Indy has no caller, so LINK dropped it. */
int J3DAPI stdConffile_WriteLine(const char* pLine)
{
    size_t len;
    size_t nWrite;

    if ( !writeFile || !pLine ) {
        return 1;
    }
    len = strlen(pLine);
    nWrite = std_pHS->pFileWrite(writeFile, pLine, len);
    if ( len != nWrite ) {
        return 1;
    }
    return 0;
}

int J3DAPI stdConffile_Write(const void* pData, size_t size)
{
    size_t nWritten;
    if ( !writeFile || !pData ) {
        return 1;
    }
    nWritten = std_pHS->pFileWrite(writeFile, pData, size);
    if ( size != nWritten ) {
        return 1;
    }
    return 0;
}

/* Linker-discarded: the debug build keeps it after Write (0x004F0E53); retail has no caller, and keeps only its 4 KB
   printBuffer in private BSS. */
int J3DAPI stdConffile_Printf(const char* pFormat, ...)
{
    size_t nWrite;
    size_t len;
    va_list arglist;

    if ( !writeFile || !pFormat ) {
        return 1;
    }
    va_start(arglist, pFormat);
    len = _vsnprintf(printBuffer, sizeof(printBuffer), pFormat, arglist);
    nWrite = std_pHS->pFileWrite(writeFile, printBuffer, len);
    va_end(arglist);
    if ( nWrite != len ) {
        return 1;
    }
    return 0;
}

int J3DAPI stdConffile_Read(void* pData, size_t size)
{
#line 340
    STD_ASSERTREL(pData && (size > 0));
    if ( !stdConffile_bOpen || !openFile ) {
        return 0;
    }
    if ( std_pHS->pFileRead(openFile, pData, size) != size ) {
        return 0;
    }
    return 1;
}

int J3DAPI stdConffile_ReadArgsFromStr(char* pStr)
{
    char* ptok;
    char* pqstr;
    int argNum = 0;
    StdConffileEntry* pEntry = &stdConffile_entry;
    memset(&pEntry->numArgs, 0, sizeof(pEntry->numArgs));

    ptok = strtok(pStr, ", \t\n\r");
    while ( ptok )
    {
        if ( argNum > 512 ) /* Retail accepts index 512 before reporting overflow. */
        {
#line 385
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "conffile  %s line %d too many args.\n", stdConffile_pFilename, stdConffile_linenum, 0, 0);
            return 1;
        }

        pqstr = strchr(ptok, '"');
        if ( pqstr )
        {
            pEntry->aArgs[argNum].argName  = pqstr + 1;
            pEntry->aArgs[argNum].argValue = pqstr + 1;
            if ( ptok[strlen(ptok) - 1] == '"' )
            {
                ptok[strlen(ptok) - 1] = '\0';
            }
            else
            {
                ptok[strlen(ptok)] = ' ';
                ptok = strtok(NULL, ",\t\n\r\"");
            }
        }
        else
        {
            pqstr = strchr(ptok, '=');
            if ( pqstr )
            {
                *pqstr = '\0';
                pEntry->aArgs[argNum].argName = ptok;
                pEntry->aArgs[argNum].argValue = pqstr + 1;
            }
            else
            {
                pEntry->aArgs[argNum].argName = ptok;
                pEntry->aArgs[argNum].argValue = ptok;
            }
        }

        ++argNum;
        ptok = strtok(NULL, ", \t\n\r");
    }

    pEntry->numArgs = argNum;
    return 0;
}

int J3DAPI stdConffile_ReadArgs(void)
{
#line 459
    STD_ASSERTREL((stdConffile_bOpen) && (openFile != 0));
    for ( ;; )
    {
        if ( !stdConffile_ReadLine() )
        {
            return 0;
        }

        if ( stdConffile_ReadArgsFromStr(stdConffile_aLine) )
        {
            return 0;
        }

        if ( !stdConffile_entry.numArgs )
        {
            continue;
        }

        break;
    }

    return 1;
}

int J3DAPI stdConffile_ReadLine(void)
{
    int bEnd = 0;
    int unusedFlag = 0; // Debug build initialises this local and never reads it.
    char* pLine  = stdConffile_aLine;
    size_t nRead = STDCONFFILE_LINESIZE - 1;
    int bRead;
    size_t len;
    char* pCommentLine;

    while ( !bEnd && nRead > 0 )
    {
        bRead = std_pHS->pFileGets(openFile, pLine, nRead);
        if ( !bRead )
        {
            return 0;
        }

        ++stdConffile_linenum;
        if ( *pLine == ';' || *pLine == '#' || *pLine == '\n' || *pLine == '\r' )
        {
            continue;
        }

        pCommentLine = strchr(pLine, '#');
        if ( pCommentLine )
        {
            *pCommentLine = '\0';
        }

        // Convert line to lower case
        _strlwr(pLine);

        len = strlen(stdConffile_aLine);
        if ( stdConffile_aLine[len - 2] == '\\' )
        {
            nRead = STDCONFFILE_LINESIZE - len;
            if ( !nRead )
            {
#line 531
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "conffile line %d too long.  Truncated.\n", stdConffile_linenum, 0, 0, 0);
            }

            pLine = &stdConffile_aLine[len - 2];
        }
        else
        {
            bEnd = 1;
            if ( stdConffile_aLine[len - 1] == '\r' || stdConffile_aLine[len - 1] == '\n' )
            {
                stdConffile_aLine[len - 1] = '\0';
            }
        }
    }

    return 1;
}

tFileHandle J3DAPI stdConffile_GetFileHandle(void)
{
#line 569
    STD_ASSERTREL(openFile != 0);
    return openFile;
}

size_t J3DAPI stdConffile_GetLineNumber(void)
{
    return stdConffile_linenum;
}

const char* J3DAPI stdConffile_GetFilename(void)
{
    return stdConffile_pFilename;
}

/* Linker-discarded (no caller in either image); the debug build keeps it after GetFilename (0x004F12F2). */
const char* J3DAPI stdConffile_GetWriteFilename(void)
{
    return stdConffile_aWriteFilename;
}

void J3DAPI stdConffile_PushStack(void)
{
#line 612
    STD_ASSERTREL(stackLevel < 20);
    STD_ASSERTREL(strlen(stdConffile_pFilename) < 128);
    strcpy(aFilenameStack[stackLevel], stdConffile_pFilename);

    linenumStack[stackLevel] = stdConffile_linenum;
    stdConffile_linenum = 0;

    openFileStack[stackLevel] = openFile;
    openFile = 0;

    apBufferStack[stackLevel] = stdConffile_aLine;

    memcpy(&aEntryStack[stackLevel], &stdConffile_entry, sizeof(StdConffileEntry));
    ++stackLevel;
}

void J3DAPI stdConffile_PopStack(void)
{
    if ( stackLevel < 1 )
    {
        return;
    }

    strcpy(stdConffile_pFilename, aFilenameStack[--stackLevel]);

    stdConffile_linenum = linenumStack[stackLevel];
    openFile = openFileStack[stackLevel];

    stdConffile_aLine = apBufferStack[stackLevel];
    memcpy(&stdConffile_entry, &aEntryStack[stackLevel], sizeof(stdConffile_entry));
}
