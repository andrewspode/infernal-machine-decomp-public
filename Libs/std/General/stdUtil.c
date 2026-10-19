#include "stdUtil.h"
#include <std/General/std.h>
#include <std/General/stdMemory.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* Retail's linker discarded this function (no retail symbol, address or caller). Its evidence is the
   /Gf literal order: retail's run starts "pSource != ((void *)0)", J3D_FILE, "pSeparators != ...",
   while StringSplit alone emits pSeparators, pSource, J3D_FILE (VC5 emits a function's new literals
   in reverse creation order, an assert creating J3D_FILE before its condition). So an earlier
   function emitted [pSource, J3D_FILE]: a lone STD_ASSERTREL(pSource != NULL). Jedi Knight's
   stdString (this module's counterpart) opens with stdString_FastCopy(const char* pSource), ahead of
   snprintf (stdUtil_Format) and CopyBetweenDelimiter (StringSplit). Name from JK with the TU's
   prefix. Body: the debug-build
   stdUtil_StringDuplicate, which the debug build places first in this file and
   which opens with the same pSource assert; it is the same function as Jedi Knight's FastCopy
   (allocate strlen + 1, copy), with Indy's pHS parameter and second assert (the "pHS" literal's only
   user is discarded, so retail's run keeps just "pSource", J3D_FILE). A modern
   stdUtil_StringCopy(dst, len + 1, src) is written as strcpy, as elsewhere in the tree. */
char* J3DAPI stdUtil_FastCopy(const char* pSource, tHostServices* pHS)
{
    char* pDupStr;

#line 44
    STD_ASSERTREL(pSource != NULL);
    STD_ASSERTREL(pHS != NULL);

    pDupStr = (char*)STDMALLOC(strlen(pSource) + 1);
    if ( !pDupStr )
    {
        return NULL;
    }

    strcpy(pDupStr, pSource);
    return pDupStr;
}

void J3DAPI stdUtil_Format(char* pStr, size_t size, const char* format, ...)
{
    va_list args;

    va_start(args, format);
    _vsnprintf(pStr, size - 1, format, args);
    pStr[size - 1] = '\0';
    va_end(args);
}

char* J3DAPI stdUtil_StringSplit(const char* pSource, char* pFirstToken, size_t maxTokenLenght, const char* pSeparators)
{
    char* pTokenLast;
    size_t beginSeps;
    size_t len;

#line 124
    STD_ASSERTREL(pSource != NULL);
    STD_ASSERTREL(pSeparators != NULL);

    if ( pFirstToken )
    {
        *pFirstToken = 0;
    }

    beginSeps = strspn(pSource, pSeparators);   // Skip any leading separators.
    STD_ASSERT(beginSeps >= 0);
    pSource += beginSeps;
    pTokenLast = strpbrk(pSource, pSeparators); // Find the next occurrence of any separator.

    if ( !pTokenLast )
    {
        len = strlen(pSource);
    }
    else
    {
        len = pTokenLast - pSource;
    }

    len = len < maxTokenLenght - 1 ? len : maxTokenLenght - 1;
    if ( pFirstToken )
    {
        strncpy(pFirstToken, pSource, len);
        pFirstToken[len] = 0;
    }

    return pTokenLast;
}

char* J3DAPI stdUtil_ParseLiteral(const char* pSource, char* pDest, size_t destSize)
{
    char* pStrBegin;
    char* pStrEnd;
    size_t len;

#line 202
    STD_ASSERTREL(pSource != NULL);

    if ( pDest )
    {
        *pDest = 0;
    }

    pStrBegin = strchr(pSource, '"');
    if ( !pStrBegin )
    {
        return NULL;
    }

    ++pStrBegin;
    pStrEnd = strchr(pStrBegin, '"');
    if ( !pStrEnd )
    {
        return NULL;
    }

    if ( pDest )
    {
        len = pStrEnd - pStrBegin;
        len = J3DMIN(len, destSize - 1);
#line 226
        STD_ASSERT(len >= 0);
#line 228
        STD_ASSERTREL(destSize > 0);
        memcpy(pDest, pStrBegin, len);
        pDest[len] = 0;
    }

    return pStrEnd + 1;
}

int J3DAPI stdUtil_ToWStringEx(wchar_t* pwString, const char* pString, int maxChars)
{
    int numChars = 0;

#line 261
    STD_ASSERTREL(pwString != NULL);
    STD_ASSERTREL(pString != NULL);
    STD_ASSERTREL(maxChars>=0);

    while ( numChars < maxChars && *pString )
    {
        *pwString++ = (unsigned char)*pString++;
        ++numChars;
    }

    if ( numChars < maxChars )
    {
        *pwString = 0;
    }

    return numChars;
}

int J3DAPI stdUtil_ToAStringEx(char* pString, const wchar_t* pwString, int maxChars)
{
    int numChars = 0;

#line 305
    STD_ASSERTREL(pString != NULL);
    STD_ASSERTREL(pwString != NULL);
    STD_ASSERTREL(maxChars>=0);

    while ( numChars < maxChars && *pwString )
    {
        if ( *pwString > 255 )
        {
            *pString = '?';
        }
        else
        {
            *pString = (char)*pwString;
        }

        ++pwString;
        ++pString;
        ++numChars;
    }

    if ( numChars < maxChars )
    {
        *pString = 0;
    }

    return numChars;
}

/* Linker-discarded; names inferred. Jedi Knight's and MotS's stdString.c (Indy's
   stdUtil.c) have WstrRemoveCharsAt and wstrncat between WcharToChar and CstrCopy, and retail's __LINE__ anchors leave
   +80 excess lines exactly there (ToAStringEx 307 -> ToWString 439). Bodies: the
   Indy debug build has both at that position (0x004EDB0F, original lines 350..352; 0x004EDBEE, lines 390..392) and
   returns nothing from either (the JK `int` return is dropped); read from the unoptimised disassembly. The
   header prototypes follow (return type void, the assert texts' parameter names). */
void J3DAPI stdUtil_WstrRemoveCharsAt(wchar_t* pwStr, int index, int numToDel)
{
    int size;
    int len;

    len = wcslen(pwStr);
#line 350
    STD_ASSERT(pwStr != NULL);
#line 351
    STD_ASSERT(index >= 0);
#line 352
    STD_ASSERT(numToDel > 0);

    if ( index >= len )
    {
        return;
    }

    numToDel = J3DMIN(numToDel, len - index);
    size = (len - (index + numToDel)) * sizeof(wchar_t) + sizeof(wchar_t);
    memmove(&pwStr[index], pwStr + index + numToDel, size);
}

void J3DAPI stdUtil_wstrncat(wchar_t* pwSource, int sourceSize, int index, wchar_t* pwStrToInsert)
{
    int lenInsert;
    int i;
    int num;
    wchar_t* pInsertPos;
    int lenSource;
    wchar_t* pMoveTo;

#line 390
    STD_ASSERT(pwSource != NULL);
#line 391
    STD_ASSERT(sourceSize > 0);
#line 392
    STD_ASSERT(pwStrToInsert != NULL);

    lenSource = wcslen(pwSource);
    lenInsert = wcslen(pwStrToInsert);
    pInsertPos = pwSource + index;
    pMoveTo = pwSource + index + lenInsert;

    num = lenSource - index + 1;
    num = J3DMIN(num, sourceSize - (index + lenInsert));
    if ( num > 0 )
    {
        for ( i = num - 1; i >= 0; --i )
        {
            pMoveTo[i] = pInsertPos[i];
        }
    }

    num = J3DMIN(lenInsert, sourceSize - index - 1);
    memcpy(pInsertPos, pwStrToInsert, num * sizeof(wchar_t));
    pwSource[sourceSize - 1] = 0;
}

wchar_t* J3DAPI stdUtil_ToWString(const char* pString)
{
    wchar_t* pwString;

#line 439
    STD_ASSERTREL(pString!=NULL);

#line 441
    pwString = (wchar_t*)STDMALLOC((sizeof(wchar_t) * strlen(pString)) + sizeof(wchar_t));
    STD_ASSERTREL(pwString!=NULL);

    stdUtil_ToWStringEx(pwString, pString, strlen(pString));
    pwString[strlen(pString)] = 0;
    return pwString;
}

char* J3DAPI stdUtil_ToAString(const wchar_t* pwString)
{
    char* pString;

#line 469
    STD_ASSERTREL(pwString!=NULL);

#line 471
    pString = (char*)STDMALLOC(wcslen(pwString) + 1);
    STD_ASSERTREL(pString!=NULL);

    stdUtil_ToAStringEx(pString, pwString, wcslen(pwString));
    pString[wcslen(pwString)] = 0;
    return pString;
}

void J3DAPI stdUtil_ToLower(char* pStr)
{
    while ( *pStr )
    {
        *pStr = (char)tolower(*pStr);
        ++pStr;
    }
}

/* Linker-discarded (no retail caller; their /Gf literals are COMDATs and went with them): the Indy debug build's last six
   stdUtil functions, restored in full from its code: a file-exists helper (0x004EDEC3, the stdUtil_FileExists),
   a path-list search (0x004EDF01, asserts at original lines 568..571), three rectangle helpers (0x004EE128, 0x004EE18E,
   0x004EE1F4, asserts 735..737; StdRect's members used as x, y, width, height, as every StdRect user in this tree does) and
   the checksum. Names other than FileExists/CalcChecksum are inferred. */
int J3DAPI stdUtil_FileExists(const char* pFilename)
{
    tFileHandle fh;

    fh = std_pHS->pFileOpen(pFilename, "r");
    if ( !fh )
    {
        return 0;
    }

    std_pHS->pFileClose(fh);
    return 1;
}

int J3DAPI stdUtil_FindFile(char* pFullname, int fullnameSize, const char* pFilename, const char* pPath)
{
    char aToken[128] = "";
    int fileLen = strlen(pFilename);  // unreferenced; the debug build's frame keeps its slot
    int bDone = 0;  // unreferenced; the debug build's frame keeps its slot
    char aSlashStr[2] = { '\\', '\0' };
    char aFullPath[128];
    const char* pStr;
    int skip;
    int remain;
    int dirLen;

#line 568
    STD_ASSERT(pFullname != NULL);
    STD_ASSERT(fullnameSize > 0);
    STD_ASSERT(pFilename != NULL);
    STD_ASSERT(pPath != NULL);

    pStr = pPath;
    do
    {
        skip = strspn(pStr, ";");
        pStr += skip;
        while ( *pStr && *pStr == ' ' )
        {
            ++pStr;
        }

        pStr = stdUtil_StringSplit(pStr, aToken, sizeof(aToken), ";");
        if ( aToken[0] )
        {
            dirLen = strlen(aToken);
            strncpy(aFullPath, aToken, sizeof(aFullPath) - 1);
            aFullPath[sizeof(aFullPath) - 1] = '\0';
            remain = sizeof(aFullPath) - dirLen;
            if ( aToken[dirLen - 1] != '\\' )
            {
                strncat(aFullPath, aSlashStr, remain);
                --remain;
            }

            strncat(aFullPath, pFilename, remain);
            if ( stdUtil_FileExists(aFullPath) )
            {
                strncpy(pFullname, aFullPath, fullnameSize - 1);
                pFullname[fullnameSize - 1] = '\0';
                return 1;
            }
        }
    }
    while ( pStr );

    return 0;
}

int J3DAPI stdUtil_RectsTouch(const StdRect* pRectA, const StdRect* pRectB)
{
    if ( pRectA->left + pRectA->right < pRectB->left )
    {
        return 0;
    }

    if ( pRectB->left + pRectB->right < pRectA->left )
    {
        return 0;
    }

    if ( pRectA->top + pRectA->bottom < pRectB->top )
    {
        return 0;
    }

    if ( pRectB->top + pRectB->bottom < pRectA->top )
    {
        return 0;
    }

    return 1;
}

int J3DAPI stdUtil_RectsOverlap(const StdRect* pRectA, const StdRect* pRectB)
{
    if ( pRectA->left + pRectA->right <= pRectB->left )
    {
        return 0;
    }

    if ( pRectB->left + pRectB->right <= pRectA->left )
    {
        return 0;
    }

    if ( pRectA->top + pRectA->bottom <= pRectB->top )
    {
        return 0;
    }

    if ( pRectB->top + pRectB->bottom <= pRectA->top )
    {
        return 0;
    }

    return 1;
}

void J3DAPI stdUtil_UnionRect(StdRect* pRectA, const StdRect* pRectB)
{
#line 735
    STD_ASSERT(pRectA != NULL);
    STD_ASSERT(pRectB != NULL);
    STD_ASSERT(pRectA != pRectB);

    if ( pRectA->left > pRectB->left )
    {
        pRectA->right += pRectA->left - pRectB->left;
        pRectA->left = pRectB->left;
    }

    if ( pRectA->top > pRectB->top )
    {
        pRectA->bottom += pRectA->top - pRectB->top;
        pRectA->top = pRectB->top;
    }

    if ( pRectA->left + pRectA->right < pRectB->left + pRectB->right )
    {
        pRectA->right = pRectB->left + pRectB->right - pRectA->left;
    }

    if ( pRectA->top + pRectA->bottom < pRectB->top + pRectB->bottom )
    {
        pRectA->bottom = pRectB->top + pRectB->bottom - pRectA->top;
    }
}

/* Linker-discarded in release, but called across modules in the debug build (sithWorld_CalcWorldChecksum 0x0043293F,
   sithThing_CalcThingChecksum 0x00448B41): debug 0x004EE323, the last stdUtil function. Name and signature from the
   debug build's stdUtil_CalcChecksum (Jedi Knight's util_Weirdchecksum); the copies into locals are the debug text. */
uint32_t J3DAPI stdUtil_CalcChecksum(const uint8_t* pData, size_t dataSize, uint32_t seed)
{
    const uint8_t* pCur;
    uint32_t checksum;

    pCur = pData;
    checksum = seed;
    while ( dataSize > 0 )
    {
        checksum = ((checksum >> 31) + 2 * checksum) ^ *pCur;
        --dataSize;
        ++pCur;
    }

    return checksum;
}
