
#include <stdio.h>

#include "std.h"
#include "stdHashtbl.h"
#include "stdMemory.h"
#include "stdStrTable.h"
#include "stdUtil.h"

static wchar_t stdStrTable_aBuffer[64] = { 0 };

int J3DAPI stdStrTable_Load(tStringTable* pStrTable, const char* pFilename)
{
    tFileHandle fh;
    int numMsgs;
    int bSuccess;
    int i;
    tStringTableNode* aData;
    char* pCur;
    char* pValBegin;
    char* pValEnd;
    char* pToken;
    char aLine[276];
    char aBuf[256];

    numMsgs = 0;
#line 117
    STD_ASSERTREL(pStrTable != NULL);
    STD_ASSERTREL(pStrTable->magic != 'sTbl');
    STD_ASSERTREL(pFilename != NULL);

    memset(pStrTable, 0, sizeof(tStringTable));
    fh = std_pHS->pFileOpen(pFilename, "rt");
    if ( !fh )
    {
        return 0;
    }

    if ( !stdStrTable_ReadLine(fh, aLine, sizeof(aLine) - 1) || sscanf(aLine, "MSGS %d", &numMsgs) != 1 )
    {
        std_pHS->pFileClose(fh);
        std_pHS->pErrorPrint("Bad 'MSG n' line in string table file '%s'\n", pFilename);
        return 0;
    }

    pStrTable->nMsgs = numMsgs;
#line 140
    aData = pStrTable->pData = (tStringTableNode*)STDMALLOC(sizeof(tStringTableNode) * numMsgs);
    if ( !aData )
        STDLOG_FATAL("Out of memory--cannot load string table");

    memset(pStrTable->pData, 0, sizeof(tStringTableNode) * numMsgs);

    pStrTable->pHashtbl = stdHashtbl_New((int)(numMsgs * 1.5));
    if ( !pStrTable->pHashtbl ) {
        STDLOG_FATAL("Out of memory--cannot load string table");
    }

    bSuccess = 1;
    for ( i = 0; bSuccess && i < numMsgs; i++ )
    {
        bSuccess = stdStrTable_ReadLine(fh, aLine, sizeof(aLine) - 1);
        if ( !strncmpi(aLine, "end", 3) )
        {
            bSuccess = 0;
            pStrTable->nMsgs = i;
            std_pHS->pErrorPrint("Premature 'END' found after only %d lines in '%s'.  Check number in 'MSG xxx' header.\n", i, pFilename);
        }

        if ( bSuccess )
        {
#line 174
            pCur = stdUtil_ParseLiteral(aLine, aBuf, sizeof(aBuf));
            if ( !pCur )
            {
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cannot understand this line in string table '%s'.\n   >>> %s\n", pFilename, aLine, 0, 0);
            }
            else
            {
#line 180
                aData[i].pKey = strcpy((char*)STDMALLOC(strlen(aBuf) + 1), aBuf);
                pCur = stdUtil_StringSplit(pCur, aBuf, sizeof(aBuf), " \t");
                if ( !pCur )
                {
#line 186
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cannot understand this line in string table '%s'.\n   >>> %s\n", pFilename, aLine, 0, 0);
                }
                else
                {
                    aData[i].unknown = atoi(aBuf);
                    stdStrTable_ParseLiteral(pCur, &pValBegin, &pValEnd);
                    memset(aBuf, 0, sizeof(aBuf));
                    strncpy(aBuf, pValBegin, pValEnd - pValBegin);
                    aData[i].value = stdUtil_ToWString(aBuf);
                    if ( !stdHashtbl_Add(pStrTable->pHashtbl, aData[i].pKey, &aData[i]) )
                    {
#line 208
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "The key '%s' is in the string table '%s' more than once.\n   >>>%s\n", aData[i].pKey, pFilename, aLine, 0);
                    }
                }
            }
        }
    }

    if ( bSuccess )
    {
        aLine[0] = 0;
        stdStrTable_ReadLine(fh, aLine, sizeof(aLine) - 1);
        pToken = strtok(aLine, " \t\n\r");
        if ( !streqi(pToken, "end") )
        {
            bSuccess = 0;
            std_pHS->pErrorPrint("'END' not found in '%s'.  Enlarge number in 'MSG xxx' header.\n", pFilename);
        }
    }

    pStrTable->magic = 'sTbl';
    std_pHS->pFileClose(fh);
    return bSuccess;
}

void J3DAPI stdStrTable_ParseLiteral(const char* pStr, char** ppBegin, char** ppEnd)
{
    char* pCur;

    *ppBegin = strchr(pStr, '"') + 1;
    pCur = *ppBegin;
    while ( pCur )
    {
        pCur = strchr(pCur, '"');
        if ( pCur )
        {
            *ppEnd = pCur;
            if ( *pCur == '"' )
            {
                ++pCur;
            }
        }
    }
}

void J3DAPI stdStrTable_Free(tStringTable* pStrTable)
{
    int nMsgs;
    int i;
    tStringTableNode* aData;

#line 404
    STD_ASSERTREL(pStrTable != NULL);

    if ( pStrTable->magic != 'sTbl' )
    {
        return;
    }

    pStrTable->magic = 0;
    nMsgs = pStrTable->nMsgs;
    pStrTable->nMsgs = 0;
    aData = pStrTable->pData;
    pStrTable->pData = NULL;
    stdHashtbl_Free(pStrTable->pHashtbl);

    if ( !aData )
    {
        return;
    }

    for ( i = 0; i < nMsgs; ++i )
    {
        if ( aData[i].value ) {
            stdMemory_Free(aData[i].value);
        }

        if ( aData[i].pKey ) {
            stdMemory_Free((void*)aData[i].pKey);
        }
    }

    stdMemory_Free(aData);
}

wchar_t* J3DAPI stdStrTable_GetValue(const tStringTable* pStrTable, const char* pKey)
{
    tStringTableNode* pStrNode;
#line 457
    STD_ASSERTREL(pStrTable != NULL);

    if ( !pStrTable->nMsgs )
    {
        return NULL;
    }

    pStrNode = (tStringTableNode*)stdHashtbl_Find(pStrTable->pHashtbl, pKey);
    if ( pStrNode )
    {
        return pStrNode->value;
    }

    return NULL;
}

wchar_t* J3DAPI stdStrTable_GetValueOrKey(const tStringTable* pStrTable, const char* pKey)
{
    wchar_t* pVal;

#line 492
    STD_ASSERTREL(pStrTable != NULL);
    pVal = stdStrTable_GetValue(pStrTable, pKey);
    if ( pVal )
    {
        return pVal;
    }

    STD_TOWSTR(stdStrTable_aBuffer, pKey);
    return stdStrTable_aBuffer;
}

int J3DAPI stdStrTable_ReadLine(tFileHandle fh, char* pStr, int size)
{
    int bFinish = 0;
    char* pch;
    char aBuf[64];

    do
    {
        std_pHS->pFileGets(fh, pStr, size);
        pch = strchr(pStr, '\n');
        while ( !pch )
        {
            std_pHS->pFileGets(fh, aBuf, sizeof(aBuf));
            pch = strchr(aBuf, '\n');
        }

        // Skip spaces
        pch = pStr;
        while ( isspace(*pch) )
        {
            ++pch;
        }

        if ( *pch != '#' && *pch && *pch != '\r' && *pch != '\n' )
        {
            bFinish = 1;
        }
    } while ( !bFinish );

    return 1;
}

/* Linker-discarded (no caller in either image): the debug build's last stdStrTable.c function (0x00500F8D, int3 fill
   after it), ReadLine's wide-character twin. Its code has no read call at all (neither line read of ReadLine is there).
   The name is inferred. */
int J3DAPI stdStrTable_ReadLineW(tFileHandle fh, wchar_t* pStr)
{
    int bFinish = 0;
    wchar_t* pch;
    wchar_t aBuf[64];

    do
    {
        pch = wcsrchr(pStr, L'\n');
        while ( !pch )
        {
            pch = wcsrchr(aBuf, L'\n');
        }

        // Skip spaces
        pch = pStr;
        while ( iswspace(*pch) )
        {
            ++pch;
        }

        if ( *pch != L'#' && *pch && *pch != L'\r' && *pch != L'\n' )
        {
            bFinish = 1;
        }
    } while ( !bFinish );

    return 1;
}
