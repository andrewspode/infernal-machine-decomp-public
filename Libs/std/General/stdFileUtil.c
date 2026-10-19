#include "std.h"
#include "stdFileUtil.h"
#include "stdFnames.h"
#include "stdMemory.h"

#include <io.h>
#include <stdio.h>
FindFileData* J3DAPI stdFileUtil_NewFind(const char* path, int mode, const char* pFilter)
{
    const char* filter;
#line 84
    FindFileData* pData = (FindFileData*)STDMALLOC(sizeof(FindFileData));
    if ( !pData ) {
        return NULL;
    }

    memset(pData, 0, sizeof(FindFileData));
    filter = pFilter;

    switch ( mode )
    {
        case 0:
        case 1:
        case 2:
            stdFnames_MakePath(pData->aSearchFilter, sizeof(pData->aSearchFilter), path, "*.*");
            break;

        case 3:
            if ( *filter == '.' ) {
                filter++;
            }

            sprintf(std_genBuffer, "*.%s", filter);
            stdFnames_MakePath(pData->aSearchFilter, sizeof(pData->aSearchFilter), path, std_genBuffer);
            break;
    }

    return pData;
}

void J3DAPI stdFileUtil_DisposeFind(FindFileData* ffData)
{
    FindFileData* pFind = ffData;
    if ( pFind && pFind->nFoundFiles ) {
        _findclose((long)pFind->handle);
    }

    if ( pFind ) {
        stdMemory_Free(pFind);
    }
}

/* Linker-discarded (no caller in either build): debug 0x004FDA8B, between DisposeFind and FindNext; closes the search
   handle like DisposeFind but keeps the search and restarts it. Name from Jones3DRE. */
void J3DAPI stdFileUtil_FindReset(FindFileData* ffData)
{
    FindFileData* pFind = ffData;
    if ( pFind && pFind->nFoundFiles ) {
        _findclose((long)pFind->handle);
    }

    if ( pFind ) {
        pFind->nFoundFiles = 0;
    }
}

int J3DAPI stdFileUtil_FindNext(FindFileData* ffData, tFoundFileInfo* pFileInfo)
{
    FindFileData* pFind = ffData;
    int currentresult;
    struct _finddata_t findInfo;

    if ( !pFind ) {
        return 0;
    }

    if ( pFind->nFoundFiles++ )
    {
        currentresult = _findnext((long)pFind->handle, &findInfo);
    }
    else
    {
        pFind->handle = (HANDLE)_findfirst(pFind->aSearchFilter, &findInfo);
        currentresult = (int)pFind->handle;
    }

    if ( currentresult == -1 ) {
        return 0;
    }

    strcpy(pFileInfo->aName, findInfo.name);

    pFileInfo->bIsDirectory = findInfo.attrib & _A_SUBDIR;
    pFileInfo->lastChanged  = findInfo.time_write;
    return 1;
}

/* Linker-discarded (no callers in either build): debug 0x004FDB8F and 0x004FDBDC, between FindNext and MkDir (Jedi
   Knight's FindQuick and CountMatches). Bodies from the debug code. */
int J3DAPI stdFileUtil_FindQuick(const char* path, int mode, const char* pFilter, tFoundFileInfo* pFileInfo)
{
    int res;
    FindFileData* pFind;

    pFind = stdFileUtil_NewFind(path, mode, pFilter);
    if ( !pFind ) {
        return 0;
    }

    res = stdFileUtil_FindNext(pFind, pFileInfo);
    stdFileUtil_DisposeFind(pFind);
    return res;
}

int J3DAPI stdFileUtil_CountMatches(const char* path, int mode, const char* pFilter)
{
    int count;
    FindFileData* pFind;
    tFoundFileInfo fileInfo;

    count = 0;
    pFind = stdFileUtil_NewFind(path, mode, pFilter);
    if ( !pFind ) {
        return 0;
    }

    while ( stdFileUtil_FindNext(pFind, &fileInfo) ) {
        ++count;
    }

    stdFileUtil_DisposeFind(pFind);
    return count;
}

int J3DAPI stdFileUtil_MkDir(const char* pPath)
{
    return CreateDirectoryA(pPath, NULL);
}

int J3DAPI stdFileUtil_FileExists(const char* pFilename)
{
    WIN32_FIND_DATAA findFileData;
    HANDLE hwndFind;

    hwndFind = FindFirstFileA(pFilename, &findFileData);
    if ( hwndFind != INVALID_HANDLE_VALUE )
    {
        return 1;
    }

    return 0;
}

/* Linker-discarded (no callers in either build): debug 0x004FDCA1, 0x004FDCB0 and 0x004FDCBF, after FileExists (Jedi
   Knight's RmDir, DelFile and Deltree). Bodies from the debug code. */
int J3DAPI stdFileUtil_RmDir(const char* pPath)
{
    return RemoveDirectoryA(pPath);
}

int J3DAPI stdFileUtil_DelFile(const char* pFilename)
{
    return DeleteFileA(pFilename);
}

int J3DAPI stdFileUtil_Deltree(const char* pPath)
{
    int result;
    char aPath[260];
    WIN32_FIND_DATAA findData;
    HANDLE hFind;

    result = 1;
    strcpy(aPath, pPath);
    strcat(aPath, "\\*.*");

    hFind = FindFirstFileA(aPath, &findData);
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            if ( findData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY )
            {
                if ( strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..") )
                {
                    strcpy(aPath, pPath);
                    strcat(aPath, "\\");
                    strcat(aPath, findData.cFileName);
                    result = stdFileUtil_Deltree(aPath);
                }
            }
            else
            {
                strcpy(aPath, pPath);
                strcat(aPath, "\\");
                strcat(aPath, findData.cFileName);
                result = DeleteFileA(aPath);
            }
        } while ( FindNextFileA(hFind, &findData) && result == 1 );

        FindClose(hFind);
        if ( result ) {
            result = RemoveDirectoryA(pPath);
        }
    }
    else
    {
        result = 0;
    }

    return result;
}
