#include "stdGob.h"
#include <std/General/std.h>
#include <std/General/stdHashTbl.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

static char stdGob_aEntryNameBuff[128] = { 0 };
static tHostServices stdGob_hs   = { 0 };
static int bStartup = 0;
static tHostServices* stdGob_pHS = NULL;

// TODO: The file read functions don't have implemented interacting with memory mapped file.

void J3DAPI stdGob_Startup(tHostServices* pHS)
{
#line 76
    STD_ASSERTREL(bStartup == 0);
    stdGob_hs  = *pHS;
    stdGob_pHS = &stdGob_hs;
    bStartup   = 1;
}

void J3DAPI stdGob_Shutdown(void)
{
    if ( bStartup )
    {
        return;
    }

#line 99
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "System shutdown.\n", 0, 0, 0, 0);
    bStartup   = 0;
    stdGob_pHS = NULL;
}

Gob* J3DAPI stdGob_Load(const char* pFilename, int numFileHandles, int bMMapFile)
{
#line 125
    Gob* pGob = (Gob*)STDMALLOC(sizeof(Gob));
    if ( !pGob )
    {
#line 128
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for Gob file.\n", 0, 0, 0, 0);
        return NULL;
    }

    memset(pGob, 0, sizeof(Gob));
    if ( stdGob_LoadEntry(pGob, pFilename, numFileHandles, bMMapFile) )
    {
#line 136
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Gob %s opened.\n", pFilename, 0, 0, 0);
        return pGob;
    }

    return NULL;
}

int J3DAPI stdGob_LoadEntry(Gob* pGob, const char* pFilename, int numFileHandles, int bMMapFile)
{
    tGobFileHeader fileHeader;
    size_t bytesRead;
    uint32_t idx;
    int bNeedFileOpen = 0;
    tGobFileEntry* pCurrentEntry;

    strncpy(pGob->aFilePath, pFilename, sizeof(pGob->aFilePath) - 1);
    pGob->aFilePath[sizeof(pGob->aFilePath) - 1] = 0;
    pGob->numHandles = numFileHandles;
    pGob->pCurHandle = NULL;

    if ( bMMapFile )
    {
        pGob->hFile = CreateFileA(pGob->aFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
#line 182
        STD_ASSERTREL(pGob->hFile!=NULL);
#line 183
        STD_ASSERTREL(pGob->hFile!=(HANDLE)((HFILE)-1));

        pGob->hMapFile = CreateFileMappingA(pGob->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if ( pGob->hMapFile )
        {
            pGob->bFileMap = 1;
            pGob->aHandles = (GobFileHandle*)LocalAlloc(LMEM_ZEROINIT, sizeof(GobFileHandle) * pGob->numHandles);
            if ( pGob->aHandles )
            {
                pGob->pBase = MapViewOfFile(pGob->hMapFile, FILE_MAP_READ, 0, 0, 0);
#line 202
                STD_ASSERTREL(pGob->pBase!=NULL);
            }
            else
            {
                UnmapViewOfFile(pGob->pBase);
                CloseHandle(pGob->hMapFile);
                bNeedFileOpen = 1;
            }
        }
        else
        {
            CloseHandle(pGob->hFile);
            bNeedFileOpen = 1;
        }
    }
    else
    {
        bNeedFileOpen = 1;
    }

    if ( bNeedFileOpen )
    {
        pGob->bFileMap = 0;
        pGob->hGobFile = stdGob_pHS->pFileOpen(pGob->aFilePath, "rb");
        if ( !pGob->hGobFile )
        {
#line 243
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening Gob file.\n", 0, 0, 0, 0);
            return 0;
        }

#line 248
        pGob->aHandles = (GobFileHandle*)STDMALLOC(sizeof(GobFileHandle) * pGob->numHandles);
        if ( !pGob->aHandles )
        {
#line 251
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for file handles.\n", 0, 0, 0, 0);
            return 0;
        }

        memset(pGob->aHandles, 0, sizeof(GobFileHandle) * pGob->numHandles);

        bytesRead = stdGob_pHS->pFileRead(pGob->hGobFile, &fileHeader, sizeof(tGobFileHeader));
#line 258
        STD_ASSERTREL(bytesRead == sizeof(tGobFileHeader));

        if ( strncmp(fileHeader.signature, "GOB ", 4u) )
        {
#line 267
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad signature in header of gob file.\n", 0, 0, 0, 0);
            return 0;
        }

        if ( fileHeader.version != 20 )
        {
#line 274
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad version %d for gob file\n", fileHeader.version, 0, 0, 0);
            return 0;
        }

        stdGob_pHS->pFileSeek(pGob->hGobFile, fileHeader.dirOffset, 0);
        stdGob_pHS->pFileRead(pGob->hGobFile, &pGob->directory.numEntries, 4);

#line 285
        pGob->directory.aEntries = (tGobFileEntry*)STDMALLOC(sizeof(tGobFileEntry) * pGob->directory.numEntries);
        if ( !pGob->directory.aEntries )
        {
#line 288
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for directory.\n", 0, 0, 0, 0);
            return 0;
        }

        pGob->pDirHash = stdHashtbl_New(1024u);
#line 294
        STD_ASSERTREL(pGob->pDirHash);

        pCurrentEntry = pGob->directory.aEntries;
        for ( idx = 0; idx < pGob->directory.numEntries; ++idx )
        {
            bytesRead = stdGob_pHS->pFileRead(pGob->hGobFile, pCurrentEntry, sizeof(tGobFileEntry));
#line 301
            STD_ASSERTREL(bytesRead==sizeof(tGobFileEntry));
            stdHashtbl_Add(pGob->pDirHash, pCurrentEntry->aName, pCurrentEntry);
            ++pCurrentEntry;
        }
    }

    return 1;
}

void J3DAPI stdGob_Free(Gob* pGob)
{
    if ( pGob )
    {
        stdGob_FreeEntry(pGob);
        stdMemory_Free(pGob);
        return;
    }

#line 339
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: attempt to free NULL Gob file ptr.\n", 0, 0, 0, 0);
}

void J3DAPI stdGob_FreeEntry(Gob* pGob)
{
#line 355
    STD_ASSERTREL(pGob!=NULL);
    if ( pGob->bFileMap )
    {
        UnmapViewOfFile(pGob->pBase);
        CloseHandle(pGob->hMapFile);
        CloseHandle(pGob->hFile);
        return;
    }

    if ( pGob->directory.aEntries )
    {
        stdMemory_Free(pGob->directory.aEntries);
        pGob->directory.aEntries = 0;
    }

    if ( pGob->pDirHash )
    {
        stdHashtbl_Free(pGob->pDirHash);
        pGob->pDirHash = NULL;
    }

    if ( pGob->aHandles )
    {
        stdMemory_Free(pGob->aHandles);
        pGob->aHandles = NULL;
    }

    stdGob_pHS->pFileClose(pGob->hGobFile);
    pGob->hGobFile = 0;
}

GobFileHandle* J3DAPI stdGob_FileOpen(Gob* pGob, const char* aName)
{
    tGobFileEntry* pEntry;
    GobFileHandle* pCurHandle;
    size_t handleNum;

#line 412
    STD_ASSERTREL(pGob);
#line 413
    STD_ASSERTREL(aName);

    strncpy(stdGob_aEntryNameBuff, aName, sizeof(stdGob_aEntryNameBuff) - 1);
    stdGob_aEntryNameBuff[sizeof(stdGob_aEntryNameBuff) - 1] = 0;
    stdUtil_ToLower(stdGob_aEntryNameBuff);

    pEntry = (tGobFileEntry*)stdHashtbl_Find(pGob->pDirHash, stdGob_aEntryNameBuff);
    if ( !pEntry )
    {
        return NULL;
    }

    pCurHandle = pGob->aHandles;
    for ( handleNum = 0; handleNum < pGob->numHandles; ++handleNum )
    {
        if ( !pCurHandle->bUsed )
        {
            pCurHandle->bUsed  = 1;
            pCurHandle->pGob   = pGob;
            pCurHandle->pEntry = pEntry;
            pCurHandle->offset = 0;
            return pCurHandle;
        }

        ++pCurHandle;
    }

#line 446
    stdPrintf(std_pHS->pWarningPrint, J3D_FILE, __LINE__, "Warning: out of file handles for gob %s.\n", pGob->aFilePath, 0, 0, 0);
    return NULL;
}

void J3DAPI stdGob_FileClose(GobFileHandle* pHandle)
{
#line 465
    STD_ASSERTREL(pHandle->bUsed);
    pHandle->bUsed = 0;
    if ( pHandle == pHandle->pGob->pCurHandle )
    {
        pHandle->pGob->pCurHandle = NULL;
    }
}

int J3DAPI stdGob_FileSeek(GobFileHandle* pHandle, int offset, int origin)
{
#line 491
    STD_ASSERTREL(pHandle);

    switch ( origin )
    {
        case 0:
            pHandle->offset = offset;
            break;
        case 1:
            pHandle->offset = pHandle->offset + offset;
            break;
        case 2:
            pHandle->offset = pHandle->pEntry->size + offset;
            break;
        default:
            return 0;

    }

    if ( pHandle == pHandle->pGob->pCurHandle )
    {
        pHandle->pGob->pCurHandle = NULL;
    }

    return 1;
}

int J3DAPI stdGob_FileTell(GobFileHandle* pHandle)
{
#line 536
    STD_ASSERTREL(pHandle);
    return pHandle->offset;
}

int J3DAPI stdGob_FileEOF(GobFileHandle* pHandle)
{
#line 558
    STD_ASSERTREL(pHandle);
    if ( pHandle->offset >= pHandle->pEntry->size - 1 )
    {
        return 1;
    }

    return 0;
}

size_t J3DAPI stdGob_FileRead(GobFileHandle* pHandle, void* data, size_t size)
{
    size_t nRead;

    if ( pHandle->pGob->pCurHandle != pHandle )
    {
        stdGob_pHS->pFileSeek(pHandle->pGob->hGobFile, pHandle->pEntry->offset + pHandle->offset, 0);
        pHandle->pGob->pCurHandle = pHandle;
    }

    size = J3DMIN(pHandle->pEntry->size - pHandle->offset, size);

    nRead = stdGob_pHS->pFileRead(pHandle->pGob->hGobFile, data, size);
    pHandle->offset += nRead;
    return nRead;
}

const char* J3DAPI stdGob_FileGets(GobFileHandle* pGobFileHandle, char* pStr, size_t size)
{
    char* pResult;

    if ( pGobFileHandle->offset >= pGobFileHandle->pEntry->size - 1 )
    {
        return NULL;
    }

    if ( pGobFileHandle->pGob->pCurHandle != pGobFileHandle )
    {
        stdGob_pHS->pFileSeek(pGobFileHandle->pGob->hGobFile, pGobFileHandle->pEntry->offset + pGobFileHandle->offset, 0);
        pGobFileHandle->pGob->pCurHandle = pGobFileHandle;
    }

    size = J3DMIN(pGobFileHandle->pEntry->size - pGobFileHandle->offset + 1, size);

    pResult = stdGob_pHS->pFileGets(pGobFileHandle->pGob->hGobFile, pStr, size);
    if ( pResult )
    {
        pGobFileHandle->offset += strlen(pResult);
    }

    return pResult;
}

/* Linker-discarded (no caller in either build): the debug build has the wide-character twin of FileGets after it
   (0x004FD885; Jedi Knight's stdGob_FileGetws). Body from the debug code. */
const wchar_t* J3DAPI stdGob_FileGetws(GobFileHandle* pGobFileHandle, wchar_t* pStr, size_t size)
{
    wchar_t* pResult;

    if ( pGobFileHandle->offset >= pGobFileHandle->pEntry->size - 1 )
    {
        return NULL;
    }

    if ( pGobFileHandle->pGob->pCurHandle != pGobFileHandle )
    {
        stdGob_pHS->pFileSeek(pGobFileHandle->pGob->hGobFile, pGobFileHandle->pEntry->offset + pGobFileHandle->offset, 0);
        pGobFileHandle->pGob->pCurHandle = pGobFileHandle;
    }

    size = J3DMIN((pGobFileHandle->pEntry->size - pGobFileHandle->offset) / sizeof(wchar_t) + 1, size);

    pResult = stdGob_pHS->pFileGetws(pGobFileHandle->pGob->hGobFile, pStr, size);
    if ( pResult )
    {
        pGobFileHandle->offset += wcslen(pResult);
    }

    return pResult;
}
