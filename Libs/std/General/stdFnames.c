#include "stdFnames.h"

const char* J3DAPI stdFnames_FindMedName(const char* pFilePath)
{
    const char* pName = strrchr(pFilePath, '\\');
    if ( !pName )
    {
        return pFilePath;
    }

    while ( *pName == '\\' )
    {
        pName++;
    }

    return pName;
}

const char* J3DAPI stdFnames_FindExt(const char* pFilePath)
{
    const char* pFile = stdFnames_FindMedName(pFilePath);
    const char* pExt = strrchr(pFile, '.');
    if ( !pExt )
    {
        return NULL;
    }

    pExt++;
    return pExt;
}

/* Linker-discarded (no callers in either build): the debug build has Jedi Knight's AddDefaultExt and StripExt here
   (0x004EC70D, 0x004EC754), in Jedi Knight's order (FindMedName, FindExt, AddDefaultExt, StripExt, StripExtAndDot).
   Bodies from the debug code; names from Jedi Knight's stdFnames. */
int J3DAPI stdFnames_AddDefaultExt(char* pPath, const char* pExt)
{
    const char* pCurExt = stdFnames_FindExt(pPath);
    if ( !pCurExt )
    {
        strcat(pPath, ".");
        strcat(pPath, pExt);
        return 1;
    }

    return 0;
}

void J3DAPI stdFnames_StripExt(char* pPath)
{
    char* pExt = (char*)stdFnames_FindExt(pPath);
    if ( pExt )
    {
        *pExt = 0;
    }
}

void J3DAPI stdFnames_StripExtAndDot(char* pPath)
{
    char* pExt = (char*)stdFnames_FindExt(pPath);
    if ( pExt )
    {
        *(pExt - 1) = 0;
    }
}

void J3DAPI stdFnames_ChangeExt(char* pPath, const char* pExt)
{
    stdFnames_StripExtAndDot(pPath);
    strcat(pPath, ".");
    strcat(pPath, pExt);
}

/* Linker-discarded (no callers in either build): debug 0x004EC7CD..0x004EC90C, between ChangeExt and Concat, in Jedi
   Knight's order (StripDirAndExt, CopyExt, CopyMedName, CopyDir, CopyShortName). Bodies from the debug code; names from
   Jedi Knight's stdFnames. */
void J3DAPI stdFnames_StripDirAndExt(char* pPath)
{
    char* pName;
    char* pFound;

    pName = pPath;
    pFound = strrchr(pName, ':');
    if ( pFound )
    {
        pName = pFound + 1;
    }

    pFound = strrchr(pName, '\\');
    if ( pFound )
    {
        pName = pFound + 1;
    }

    pFound = strrchr(pName, '.');
    if ( pFound )
    {
        *pFound = 0;
    }

    strcpy(pPath, pName);
}

int J3DAPI stdFnames_CopyExt(char* pOut, int size, const char* pPath)
{
    const char* pExt = stdFnames_FindExt(pPath);
    if ( !pExt )
    {
        *pOut = 0;
        return 0;
    }

    strncpy(pOut, pExt, size - 1);
    pOut[size - 1] = 0;
    return 1;
}

void J3DAPI stdFnames_CopyMedName(char* pOut, int size, const char* pPath)
{
    strncpy(pOut, stdFnames_FindMedName(pPath), size - 1);
    pOut[size - 1] = 0;
}

void J3DAPI stdFnames_CopyDir(char* pOut, int size, const char* pPath)
{
    char* pSlash;

    strncpy(pOut, pPath, size - 1);
    pOut[size - 1] = 0;
    pSlash = strrchr(pOut, '\\');
    if ( pSlash )
    {
        *pSlash = 0;
    }
}

void J3DAPI stdFnames_CopyShortName(char* pOut, int size, const char* pPath)
{
    const char* pName = stdFnames_FindMedName(pPath);
    strncpy(pOut, pName, size - 1);
    pOut[size - 1] = 0;
    stdFnames_StripExtAndDot(pOut);
}

char* J3DAPI stdFnames_Concat(char* path1, const char* path2, int size)
{
    int len = strlen(path1);

    if ( path1[len - 1] != '\\' && len < size - 1 && *path1 )
    {
        path1[len] = '\\';
        path1[++len]   = '\0';
    }

    if ( *path2 == '\\' ) {
        path2++;
    }

    strncat(path1, path2, size - len - 1);
    return path1;
}

void J3DAPI stdFnames_MakePath(char* aOutPath, int size, const char* pPath1, const char* pPath2)
{
    strncpy(aOutPath, pPath1, size - 1);
    aOutPath[size - 1] = '\0';
    stdFnames_Concat(aOutPath, pPath2, size);
}

/* Linker-discarded (no caller in either build): debug 0x004ECA11, after MakePath (Jedi Knight's MakePath3). */
void J3DAPI stdFnames_MakePath3(char* aOutPath, int size, const char* pPath1, const char* pPath2, const char* pPath3)
{
    strncpy(aOutPath, pPath1, size - 1);
    aOutPath[size - 1] = '\0';
    stdFnames_Concat(aOutPath, pPath2, size);
    stdFnames_Concat(aOutPath, pPath3, size);
}
