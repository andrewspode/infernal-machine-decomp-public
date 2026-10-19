#include "sithPVS.h"

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>

#define SITHPVS_SER_NUM_PERLINE 16
#define SITHPVS_SER_LINESIZE    (sizeof(uint32_t) * SITHPVS_SER_NUM_PERLINE) // 64

/* Discarded by the retail linker (its only caller is the debug-only sithWrite.c); restored in full from the
   debug build (0x004B4080, before ReadPVSText). Every failed write jumps to one `return 1` after the success
   return, from inside both loops too (debug jmp 0x004B41F2). Name
   inferred. */
int J3DAPI sithPVS_WritePVSText(const SithWorld* pWorld)
{
    size_t pvsSize;
    size_t curSize;
    size_t i;
    size_t numLines;
    size_t j;
    uint32_t* aPvs;

    if ( pWorld->aPVS )
    {
        pvsSize = pWorld->sizePVS;
        numLines = pvsSize / SITHPVS_SER_LINESIZE;
        if ( pvsSize % SITHPVS_SER_LINESIZE )
        {
            ++numLines;
        }

        if ( stdConffile_WriteLine("######### PVS information ######\n") )
        {
            goto error;
        }

        if ( stdConffile_WriteLine("SECTION: pvs\n") )
        {
            goto error;
        }

        if ( stdConffile_Printf("PVS Size: %d\n", pvsSize) )
        {
            goto error;
        }

        curSize = 0;
        for ( i = 0; i < numLines; ++i )
        {
            aPvs = (uint32_t*)&pWorld->aPVS[SITHPVS_SER_LINESIZE * i];
            for ( j = 0; j < SITHPVS_SER_NUM_PERLINE && curSize < pvsSize; ++j )
            {
                if ( stdConffile_Printf("%.8x ", aPvs[j]) )
                {
                    goto error;
                }

                curSize += sizeof(uint32_t);
            }

            if ( stdConffile_Printf("\n") )
            {
                goto error;
            }
        }

        for ( i = 0; i < pWorld->numSectors; ++i )
        {
            if ( stdConffile_Printf("%x \n", pWorld->aSectors[i].pvsIdx) )
            {
                goto error;
            }
        }
    }

    return 0;

error:
    return 1;
}

// Text of the debug build (debug 0x4b41fb, /Od): the EOF (110), syntax (115) and
// allocation (120) prints are blocks after the success `return 0`, the first two
// ending `goto error` on one `return 1` after the third. Exception (C) labels plus the
// debug-route `error`.
int J3DAPI sithPVS_ReadPVSText(SithWorld* pWorld, int bSkip)
{
    size_t num;
    size_t curSize;


    // The debug code's aPVS guard is `je; jmp` onto the final `return 1`: an empty
    // then-arm (a single site an empty arm explains).
    if ( pWorld->aPVS )
    {
    }
    else
    {
        size_t i;
        size_t j;
        uint32_t* aPvs;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, "pvs size: %d", &pWorld->sizePVS) != 1 )
        {
            goto syntax_error;
        }

        num = pWorld->sizePVS / SITHPVS_SER_LINESIZE;
        if ( pWorld->sizePVS % SITHPVS_SER_LINESIZE )
        {
            ++num;
        }

#line 83
        pWorld->aPVS = (uint8_t*)STDMALLOC(num * SITHPVS_SER_LINESIZE);
        if ( !pWorld->aPVS )
        {
            goto alloc_error;
        }

        curSize = 0;
        for ( i = 0; i < num; ++i )
        {
            size_t curArg;

            if ( !stdConffile_ReadArgs() )
            {
                goto eof_error;
            }

            curArg = 0;
            aPvs = (uint32_t*)&pWorld->aPVS[SITHPVS_SER_LINESIZE * i];
            for ( j = 0; j < SITHPVS_SER_NUM_PERLINE && curSize < pWorld->sizePVS; ++j )
            {
                if ( sscanf(stdConffile_entry.aArgs[curArg++].argValue, "%x", &aPvs[j]) != 1 )
                {
                    goto syntax_error;
                }

                curSize += sizeof(uint32_t);
            }
        }

        for ( i = 0; i < pWorld->numSectors; ++i )
        {
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, "%x", &pWorld->aSectors[i].pvsIdx) != 1 )
            {
                goto syntax_error;
            }
        }

        return 0;

eof_error:
#line 110
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", stdConffile_GetFilename(), 0, 0, 0);
        goto error;

syntax_error:
#line 115
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);
        goto error;

alloc_error:
#line 120
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);
    }

error:
    return 1;
}

int J3DAPI sithPVS_WritePVSBinary(tFileHandle fh, const SithWorld* pWorld)
{
    uint32_t sizePVS = pWorld->sizePVS;
    size_t nWritten;

    nWritten = sith_pHS->pFileWrite(fh, &sizePVS, sizeof(uint32_t));
    if ( nWritten != sizeof(uint32_t) )
    {
        return 1;
    }

    if ( sizePVS )
    {
        nWritten = sith_pHS->pFileWrite(fh, pWorld->aPVS, pWorld->sizePVS);
        if ( nWritten != pWorld->sizePVS )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithPVS_ReadPVSBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t numRead;

    numRead = sith_pHS->pFileRead(fh, &pWorld->sizePVS, sizeof(uint32_t));
    if ( numRead != sizeof(uint32_t) )
    {
        return 1;
    }

    if ( pWorld->sizePVS )
    {
#line 153
        pWorld->aPVS = (uint8_t*)STDMALLOC(pWorld->sizePVS);
        if ( !pWorld->aPVS )
        {
            return 1;
        }

        numRead = sith_pHS->pFileRead(fh, pWorld->aPVS, pWorld->sizePVS);
        if ( numRead != pWorld->sizePVS )
        {
            return 1;
        }
    }

    return 0;
}

void J3DAPI sithPVS_SetTable(uint8_t* paAdjoinTable, const uint8_t* aElements, int size)
{
    int i = 0;
    int j;
    int el;
    while ( i < size )
    {
        el = *aElements++;
        if ( (el & 0x80) != 0 )
        {
            if ( i + (el & 0x7F) + 1 > size )
            {
                el = size - i - 1;
            }

            for ( j = 0; j < (el & 0x7F) + 1; ++j, ++i )
            {
                *paAdjoinTable++ = *aElements++;
            }
        }
        else
        {
            if ( i + el + 3 > size )
            {
                el = size - i - 3;
            }

            for ( j = 0; j < el + 3; ++j, ++i )
            {
                *paAdjoinTable++ = 0;
            }
        }
    }

#line 193
    SITH_ASSERTREL(i <= size);
}