#include <Jones3D/types.h> // Note, don't move as it requires including windows.h without WIN32_LEAN_AND_MEAN  being defined 

#include "JonesFile.h"
#include <stdio.h>

#include <std/General/std.h>
#include <std/General/stdFileUtil.h>
#include <std/General/stdFnames.h>
#include <std/Win95/stdGob.h>

#define JF_MAX_HANDLES 32

JonesResource JonesFile_aResources[3] = { 0 };
tHostServices JonesFile_sysHS = { 0 };
char JonesFile_aPathBuf[128] = { 0 };
JonesFileHandle JonesFile_aFileHandles[JF_MAX_HANDLES] = { 0 }; // Note: handles are 1-based, element fh - 1

int JonesFile_bStartup = 0;
int JonesFile_bOpened  = 0;

tHostServices* JonesFile_pHS     = NULL;
tHostServices* JonesFile_pSysEnv = NULL;
unsigned int JonesFile_hsInitCount = 0;

void J3DAPI JonesFile_InitServices(void);
void J3DAPI JonesFile_ResetServices(void);

void J3DAPI JonesFile_OpenResource(JonesResource* pRsource, const char* pPath);
void J3DAPI JonesFile_CloseResource(JonesResource* pResource);

tFileHandle J3DAPI JonesFile_OpenFile(const char* pFilename, const char* mode);
int J3DAPI JonesFile_CloseFile(tFileHandle fh);
size_t J3DAPI JonesFile_FileRead(tFileHandle fh, void* pDst, size_t size);
size_t J3DAPI JonesFile_FileWrite(tFileHandle fh, const void* pData, size_t size);
char* J3DAPI JonesFile_FileGets(tFileHandle fh, char* pStr, size_t size);
int J3DAPI JonesFile_FileEOF(tFileHandle fh);
int J3DAPI JonesFile_FileTell(tFileHandle fh);
int J3DAPI JonesFile_FileSeek(tFileHandle fh, int offset, int origin);
size_t J3DAPI JonesFile_FileSize(const char* pFilename);
int J3DAPI JonesFile_FilePrintf(tFileHandle fh, const char* aFormat, ...);

void J3DAPI JonesFile_Startup(tHostServices* pHS)
{
    if ( JonesFile_bStartup ) {
        return;
    }

    stdGob_Startup(pHS);
    JonesFile_hsInitCount = 0;
    JonesFile_bStartup    = 1;
}

void J3DAPI JonesFile_Shutdown(void)
{
    if ( !JonesFile_bStartup ) {
        return;
    }

    stdGob_Shutdown();
    JonesFile_hsInitCount = 0;
    JonesFile_bStartup    = 0;
}

int J3DAPI JonesFile_Open(tHostServices* pEnv, const char* pInstallPath, const char* pPathCD)
{
    if ( !JonesFile_bStartup ) {
        return 1;
    }

    if ( JonesFile_bOpened )
    {
#line 225
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 1;
    }

    JonesFile_sysHS = *pEnv;
    JonesFile_pHS = &JonesFile_sysHS;
    JonesFile_pSysEnv = pEnv;

    // Resource paths:
    // 0 - install path
    // 1 - current work dir
    // 2 - CD path
    memset(JonesFile_aResources, 0, sizeof(JonesFile_aResources));

    JonesFile_InitServices(); // This will override current pEnv

    if ( pInstallPath ) {
        JonesFile_SetInstallPath(pInstallPath);
    }

    if ( pPathCD ) {
        JonesFile_SetCDPath(pPathCD);
    }

    // Add path to current working dir
    JonesFile_OpenResource(&JonesFile_aResources[JF_RES_CWD], ".");

    memset(JonesFile_aFileHandles, 0, sizeof(JonesFile_aFileHandles));

    JonesFile_bOpened = 1;
    return 0;
}

void J3DAPI JonesFile_Close(void)
{
    if ( !JonesFile_bOpened )
    {
#line 276
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
        return;
    }

    {
        size_t i;
        for ( i = 0; i < JF_MAX_HANDLES; ++i )
        {
            if ( JonesFile_aFileHandles[i].bUsed )
            {
                JonesFile_CloseFile(i + 1);
#line 286
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "File left open %s.\n", JonesFile_aFileHandles[i].aName, 0, 0, 0);
            }
        }
    }

    JonesFile_ResetServices();
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_INSTALL]);
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_CD]);
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_CWD]);
    JonesFile_bOpened = 0;
}

int J3DAPI JonesFile_GetCurrentCDNum(void)
{
    char aPath[128];
    stdFnames_MakePath(aPath, sizeof(aPath), JonesFile_aResources[JF_RES_CD].aPath, "CD1.GOB");
    if ( std_pHS->pFileSize(aPath) )
    {
        return 1;
    }

    stdFnames_MakePath(aPath, sizeof(aPath), JonesFile_aResources[JF_RES_CD].aPath, "CD2.GOB");
    if ( std_pHS->pFileSize(aPath) )
    {
        return 2;
    }

    return 0;
}

int J3DAPI JonesFile_FileExists(const char* pFilename)
{
    tFileHandle fh = std_pHS->pFileOpen(pFilename, "rb");
    if ( fh )
    {
        std_pHS->pFileClose(fh);
        return 1;
    }

    return 0;
}

void J3DAPI JonesFile_SetInstallPath(const char* pPath)
{
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_INSTALL]);
    if ( *pPath )
    {
        JonesFile_OpenResource(&JonesFile_aResources[JF_RES_INSTALL], pPath);
    }
}

void J3DAPI JonesFile_SetCDPath(const char* pPath)
{
    JonesFile_CloseResource(&JonesFile_aResources[JF_RES_CD]);
    if ( *pPath )
    {
        char aPath[128];
        _snprintf(aPath, STD_ARRAYLEN(aPath), "%sresource", pPath);
        JonesFile_OpenResource(&JonesFile_aResources[JF_RES_CD], aPath);
    }
    else
    {
#line 379
        stdPrintf(std_pHS->pWarningPrint, J3D_FILE, __LINE__, "Empty CD path specified.\n", 0, 0, 0, 0);
    }
}

void J3DAPI JonesFile_InitServices(void)
{
    if ( !JonesFile_hsInitCount )
    {
        JonesFile_pSysEnv->pFileOpen   = JonesFile_OpenFile;
        JonesFile_pSysEnv->pFileClose  = JonesFile_CloseFile;
        JonesFile_pSysEnv->pFileRead   = JonesFile_FileRead;
        JonesFile_pSysEnv->pFileGets   = JonesFile_FileGets;
        JonesFile_pSysEnv->pFileWrite  = JonesFile_FileWrite;
        JonesFile_pSysEnv->pFileEOF    = JonesFile_FileEOF;
        JonesFile_pSysEnv->pFileTell   = JonesFile_FileTell;
        JonesFile_pSysEnv->pFileSeek   = JonesFile_FileSeek;
        JonesFile_pSysEnv->pFileSize   = JonesFile_FileSize;
        JonesFile_pSysEnv->pFilePrintf = JonesFile_FilePrintf;
    }

    ++JonesFile_hsInitCount;
}

void J3DAPI JonesFile_ResetServices(void)
{
    if ( JonesFile_hsInitCount > 0 )
    {
        JonesFile_pSysEnv->pFileOpen   = JonesFile_sysHS.pFileOpen;
        JonesFile_pSysEnv->pFileClose  = JonesFile_sysHS.pFileClose;
        JonesFile_pSysEnv->pFileRead   = JonesFile_sysHS.pFileRead;
        JonesFile_pSysEnv->pFileGets   = JonesFile_sysHS.pFileGets;
        JonesFile_pSysEnv->pFileWrite  = JonesFile_sysHS.pFileWrite;
        JonesFile_pSysEnv->pFileEOF    = JonesFile_sysHS.pFileEOF;
        JonesFile_pSysEnv->pFileTell   = JonesFile_sysHS.pFileTell;
        JonesFile_pSysEnv->pFileSeek   = JonesFile_sysHS.pFileSeek;
        JonesFile_pSysEnv->pFileSize   = JonesFile_sysHS.pFileSize;
        JonesFile_pSysEnv->pFilePrintf = JonesFile_sysHS.pFilePrintf;
        --JonesFile_hsInitCount;
    }
}

void J3DAPI JonesFile_OpenResource(JonesResource* pRsource, const char* pPath)
{
    tFoundFileInfo fileInfo;
    FindFileData* pFileData;

    strncpy(pRsource->aPath, pPath, sizeof(pRsource->aPath) - 1);
    pRsource->aPath[sizeof(pRsource->aPath) - 1] = '\0';
    JonesFile_ResetServices();

    // Now find all *.gob files in dir
    pRsource->numGobFiles = 0;

    
    pFileData = stdFileUtil_NewFind(pPath, /*mode=*/3, "gob");
    while ( stdFileUtil_FindNext(pFileData, &fileInfo) )
    {
        if ( fileInfo.aName[0] != '.' )
        {
            stdFnames_MakePath(JonesFile_aPathBuf, sizeof(JonesFile_aPathBuf), pPath, fileInfo.aName);
            pRsource->aGobFiles[pRsource->numGobFiles] = stdGob_Load(JonesFile_aPathBuf, /*numFileHandles*/16, /*bMapFile*/0); // TODO: After file mapping is fixed in stdGob enable file mapping here
            if ( !pRsource->aGobFiles[pRsource->numGobFiles] )
            {
#line 486
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening gob '%s'.\n", JonesFile_aPathBuf, 0, 0, 0);
                break;
            }

            ++pRsource->numGobFiles;
        }
    }

    stdFileUtil_DisposeFind(pFileData);
    JonesFile_InitServices();
}

void J3DAPI JonesFile_CloseResource(JonesResource* pResource)
{
    size_t i;

    for ( i = 0; i < pResource->numGobFiles; ++i )
    {
        stdGob_Free(pResource->aGobFiles[i]);
    }

    pResource->numGobFiles = 0;
}

tFileHandle J3DAPI JonesFile_OpenFile(const char* pFilename, const char* mode)
{
    GobFileHandle* pGobFileHandle;
    int bOpen;
    JonesResource* pCurRes;
    tFileHandle fh;
    size_t gobNum;
    size_t handle;
    size_t resNum;
    Gob* pGobEntry;

    for ( handle = 0; handle < JF_MAX_HANDLES; ++handle )
    {
        if ( !JonesFile_aFileHandles[handle].bUsed )
        {
            break;
        }
    }

    if ( handle >= JF_MAX_HANDLES )
    {
#line 571
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of file handles!\n", 0, 0, 0, 0);
        return 0;
    }

    // Try to open file by provided path
    bOpen = 0;
    fh = JonesFile_pHS->pFileOpen(pFilename, mode);
    if ( fh )
    {
        JonesFile_aFileHandles[handle].bFileHandle = 1;
        JonesFile_aFileHandles[handle].hFile       = fh;
        strncpy(JonesFile_aFileHandles[handle].aName, pFilename, sizeof(JonesFile_aFileHandles[handle].aName));
        JonesFile_aFileHandles[handle].aName[sizeof(JonesFile_aFileHandles[handle].aName)] = '\0'; // Note: retail writes one past aName, into struct padding
        JonesFile_aFileHandles[handle].bUsed       = 1;
        bOpen = 1;
    }
    else // No file was found, try to find it in stored resources
    {
        // Search the resources:
        // 0 - install path
        // 1 - current work dir path
        // 2 - CD path
        pCurRes = JonesFile_aResources;
        for ( resNum = 0; resNum < STD_ARRAYLEN(JonesFile_aResources) && !bOpen; ++resNum )
        {
            if ( pCurRes->aPath[0] )
            {
                if ( pCurRes != &JonesFile_aResources[JF_RES_CD] ) // If not CD resource
                {
                    stdFnames_MakePath(JonesFile_aPathBuf, sizeof(JonesFile_aPathBuf), pCurRes->aPath, pFilename);
                    fh = JonesFile_pHS->pFileOpen(JonesFile_aPathBuf, mode);
                    if ( fh )
                    {
                        JonesFile_aFileHandles[handle].bFileHandle = 1;
                        JonesFile_aFileHandles[handle].hFile       = fh;
                        strncpy(JonesFile_aFileHandles[handle].aName, JonesFile_aPathBuf, sizeof(JonesFile_aFileHandles[handle].aName));
                        JonesFile_aFileHandles[handle].aName[sizeof(JonesFile_aFileHandles[handle].aName)] = '\0';
                        JonesFile_aFileHandles[handle].bUsed       = 1;
                        bOpen = 1;
                    }
                }
                else
                {
                    bOpen = bOpen;
                }

                if ( !bOpen )
                {
                    // File was not found on disk try to search for file in GOB resources
                    for ( gobNum = 0; gobNum < pCurRes->numGobFiles && !bOpen; ++gobNum )
                    {
                        pGobEntry = pCurRes->aGobFiles[gobNum];
                        pGobFileHandle = stdGob_FileOpen(pGobEntry, pFilename);
                        if ( pGobFileHandle )
                        {
                            JonesFile_aFileHandles[handle].bFileHandle    = 0;
                            JonesFile_aFileHandles[handle].pGobFileHandle = pGobFileHandle;
                            strncpy(JonesFile_aFileHandles[handle].aName, pFilename, sizeof(JonesFile_aFileHandles[handle].aName));
                            JonesFile_aFileHandles[handle].aName[sizeof(JonesFile_aFileHandles[handle].aName)] = '\0';
                            JonesFile_aFileHandles[handle].bUsed          = 1;
                            bOpen = 1;
                        }
                    }
                }
            }

            ++pCurRes;
        }
    }

    if ( !bOpen )
    {
        return 0;
    }

    return handle + 1;
}

int J3DAPI JonesFile_CloseFile(tFileHandle fh)
{
    JonesFileHandle* pHandle;

#line 666
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bUsed )
    {
        if ( pHandle->bFileHandle )
        {
            JonesFile_pHS->pFileClose(pHandle->hFile);
            pHandle->hFile = 0;
        }
        else
        {
            stdGob_FileClose(pHandle->pGobFileHandle);
            pHandle->pGobFileHandle = NULL;
        }
    }

    memset(pHandle, 0, sizeof(JonesFileHandle));
    return 0;
}

size_t J3DAPI JonesFile_FileRead(tFileHandle fh, void* pDst, size_t size)
{
    size_t result;
    JonesFileHandle* pHandle;

#line 713
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        result = JonesFile_pHS->pFileRead(pHandle->hFile, pDst, size);
    }
    else
    {
        result = stdGob_FileRead(pHandle->pGobFileHandle, pDst, size);
    }

    return result;
}

size_t J3DAPI JonesFile_FileWrite(tFileHandle fh, const void* pData, size_t size)
{
    size_t result;
    JonesFileHandle* pHandle;

#line 746
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        result = JonesFile_pHS->pFileWrite(pHandle->hFile, pData, size);
    }
    else
    {
#line 753
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error tried to write to bundled file '%s'\n", pHandle->aName, 0, 0, 0);
        result = 0;
    }

    return result;
}

char* J3DAPI JonesFile_FileGets(tFileHandle fh, char* pStr, size_t size)
{
    char* result;
    JonesFileHandle* pHandle;

#line 779
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        result = JonesFile_pHS->pFileGets(pHandle->hFile, pStr, size);
    }
    else
    {
        result = (char*)stdGob_FileGets(pHandle->pGobFileHandle, pStr, size);
    }

    return result;
}

int J3DAPI JonesFile_FileEOF(tFileHandle fh)
{
    JonesFileHandle* pHandle;

#line 809
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileEOF(pHandle->hFile);
    }
    else
    {
        return stdGob_FileEOF(pHandle->pGobFileHandle);
    }
}

int J3DAPI JonesFile_FileTell(tFileHandle fh)
{
    JonesFileHandle* pHandle;

#line 837
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileTell(pHandle->hFile);
    }
    else
    {
        return stdGob_FileTell(pHandle->pGobFileHandle);
    }
}

int J3DAPI JonesFile_FileSeek(tFileHandle fh, int offset, int origin)
{
    JonesFileHandle* pHandle;

#line 867
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));
    pHandle = &JonesFile_aFileHandles[fh - 1];
    if ( pHandle->bFileHandle )
    {
        return JonesFile_pHS->pFileSeek(pHandle->hFile, offset, origin);
    }
    else
    {
        return stdGob_FileSeek(pHandle->pGobFileHandle, offset, origin);
    }
}

size_t J3DAPI JonesFile_FileSize(const char* pFilename)
{
    return JonesFile_pHS->pFileSize(pFilename);
}

int J3DAPI JonesFile_FilePrintf(tFileHandle fh, const char* aFormat, ...)
{
    va_list args;
    size_t size;
    JonesFileHandle* pHandle;

#line 917
    STD_ASSERTREL(STD_INRANGE(fh, 1, JF_MAX_HANDLES-1));

    pHandle = &JonesFile_aFileHandles[fh - 1];

    va_start(args, aFormat);
    size = _vsnprintf(std_genBuffer, sizeof(std_genBuffer), aFormat, args);

    if ( pHandle->bFileHandle )
    {
        JonesFile_pHS->pFilePrintf(pHandle->hFile, std_genBuffer, size);
    }
    else
    {
#line 928
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error tried to printf to bundled file '%s'\n", pHandle->aName, 0, 0, 0);
    }

    va_end(args);
    return 0;
}