#include "std.h"
#include "stdMemory.h"

#define STDMEMORY_HEADERMAGIC   0x12345678
#define STD_MEMORY_FREE_PATTERN 0xDD

#define STDMEMORY_ZONECOUNT 8456
#define STDMEMORY_ZONESIZE 0x7C00
#define STDMEMORY_BLOCKUSED 0x8000
#define STDMEMORY_BLOCKSIZE 0x7FFF

static tMemoryBlock stdMemory_aZoneData[STDMEMORY_ZONECOUNT];
tMemoryState stdMemory_curState;

// Allocation-number breakpoint: Malloc and Free set a local flag when the
// allocation being made or freed has this number, for a debugger breakpoint.
static int breakAllocNum = 0;
static int bStartup = 0;
static int bOpen = 0;

int J3DAPI stdMemory_Startup(void)
{
#line 126
    STD_ASSERTREL(bStartup == 0);
    memset(&stdMemory_curState, 0, sizeof(stdMemory_curState));
    bStartup = 1;
    return 1;
}

void J3DAPI stdMemory_Shutdown(void)
{
#line 149
    STD_ASSERTREL(bStartup == 1);
    bStartup = 0;
}

int J3DAPI stdMemory_Open(void)
{
#line 172
    STD_ASSERTREL(bStartup == 1);
    if ( bOpen )
    {
#line 176
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 0;
    }
    else
    {
        bOpen = 1;
        return 1;
    }
}

void J3DAPI stdMemory_Close(void)
{
    size_t count = 0;
    tMemoryHeader* cur;
    tMemoryHeader* pNext;

    if ( !bOpen )
    {
#line 203
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
        return;
    }

#line 208
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Maximum dynamic memory used: %d bytes.\n", stdMemory_curState.maxBytes, 0, 0, 0);
    if ( stdMemory_curState.totalAllocs > 0 || stdMemory_curState.totalBytes > 0 )
    {
#line 214
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "UNFREED MEMORY ALERT!\n\n", 0, 0, 0, 0);
        std_pHS->pErrorPrint("File\tLine\tSize\tNumber\n\n", 0, 0, 0, 0);

        cur = stdMemory_curState.header.pNext;
        while ( cur && count < 100 )
        {
            pNext = cur->pNext;
            std_pHS->pErrorPrint("%s\t%d\t%d\t%d\n", cur->pFilename, cur->line, cur->size, cur->number);
            cur = pNext;
            ++count;
        }
    }

    bOpen = 0;
}

void* J3DAPI stdMemory_Malloc(size_t size, const char* pFilename, size_t line)
{
    size_t allocSize;
    tMemoryHeader* pHeader;
    void* pInfo;
    int bBreak;

    bBreak = 0;
    if ( stdMemory_curState.totalAllocs == breakAllocNum )
    {
        bBreak = 1;
    }

    allocSize = size + sizeof(tMemoryHeap);
    pInfo = std_pHS->pMalloc(allocSize);
    if ( !pInfo )
    {
        return 0;
    }

    pHeader = (tMemoryHeader*)pInfo;
    pHeader->number = stdMemory_curState.totalAllocs;
    pHeader->id = (uint32_t)pHeader;
    pHeader->size = size;
    pHeader->pFilename = pFilename;
    pHeader->line = line;
    pHeader->pNext = stdMemory_curState.header.pNext;
    if ( pHeader->pNext )
    {
        pHeader->pNext->pPrev = pHeader;
    }

    pHeader->pPrev = &stdMemory_curState.header;
    stdMemory_curState.header.pNext = pHeader;

    memset((char*)pInfo + sizeof(tMemoryHeader), 0xCC, size);

    pHeader->magic = STDMEMORY_HEADERMAGIC;
    *(DWORD*)((char*)pInfo + size + sizeof(tMemoryHeader)) = STDMEMORY_HEADERMAGIC;
    stdMemory_curState.totalBytes += size;
    stdMemory_curState.maxBytes = (stdMemory_curState.maxBytes > stdMemory_curState.totalBytes) ? stdMemory_curState.maxBytes : stdMemory_curState.totalBytes;
    ++stdMemory_curState.totalAllocs;
    return (char*)pInfo + sizeof(tMemoryHeader);
}

void J3DAPI stdMemory_Free(void* pBytes)
{
    int bBreak = 0;
    tMemoryHeader* pHeader = (tMemoryHeader*)pBytes - 1;
#line 325
    STD_ASSERTREL((pHeader != NULL) && ((DWORD)pHeader == pHeader->id));
    if ( pHeader->number == breakAllocNum )
    {
        bBreak = 1;
    }

#line 335
    STD_ASSERTREL(pHeader->magic==0x12345678);
    STD_ASSERTREL(*(DWORD *)((BYTE *)pBytes+pHeader->size)==0x12345678);

    memset(pBytes, STD_MEMORY_FREE_PATTERN, pHeader->size);
    if ( pHeader->pNext )
    {
        pHeader->pNext->pPrev = pHeader->pPrev;
    }

#line 345
    STD_ASSERTREL(pHeader->pPrev);
    pHeader->pPrev->pNext = pHeader->pNext;
    stdMemory_curState.totalBytes -= pHeader->size;
    --stdMemory_curState.totalAllocs;
    std_pHS->pFree(pHeader);
}

void* J3DAPI stdMemory_Realloc(void* pBytes, size_t size, const char* pFilename, size_t line)
{
    void* pInfo;
    tMemoryHeader* pHeader;
    size_t allocationSz;
    size_t oldSize;

    if ( !pBytes ) {
        return stdMemory_Malloc(size, pFilename, line);
    }
    if ( !size )
    {
        stdMemory_Free(pBytes);
        return NULL;
    }

    allocationSz = size + sizeof(tMemoryHeap);
    pHeader = (tMemoryHeader*)pBytes - 1;
#line 399
    STD_ASSERTREL((pHeader != NULL) && ((DWORD)pHeader == pHeader->id));
    oldSize = pHeader->size;
    pInfo = std_pHS->pRealloc(pHeader, allocationSz);
    if ( !pInfo ) {
        return NULL;
    }
    pHeader = (tMemoryHeader*)pInfo;
    pHeader->id = (DWORD)pHeader;
    pHeader->size = size;
    pHeader->pFilename = pFilename;
    pHeader->line = line;
    if ( pHeader->pNext ) {
        pHeader->pNext->pPrev = pHeader;
    }
    if ( pHeader->pPrev ) {
        pHeader->pPrev->pNext = pHeader;
    }
    pHeader->magic = STDMEMORY_HEADERMAGIC;
    *(DWORD*)((char*)pInfo + size + sizeof(tMemoryHeader)) = STDMEMORY_HEADERMAGIC;
    stdMemory_curState.totalBytes += size - oldSize;
    stdMemory_curState.maxBytes = (stdMemory_curState.maxBytes > stdMemory_curState.totalBytes) ? stdMemory_curState.maxBytes : stdMemory_curState.totalBytes;
    return (char*)pInfo + sizeof(tMemoryHeader);
}

void* J3DAPI stdMemory_BlockMalloc(size_t size)
{
    tMemoryZoneHeader* pBlock;
    tMemoryZoneHeader* pLargest;
    int i;
    tMemoryZoneHeader* pBest;
    tMemoryBlock* pZone;
    uint16_t currentblockSize;
    uint16_t bestSize;
    uint16_t currentremainder;

    if ( size > 4096 ) {
        return stdMemory_BlockAlloc(size);
    }
    size = (size + 3) & ~3;
    for ( i = 0, pZone = stdMemory_aZoneData; i < STDMEMORY_ZONECOUNT && pZone->bAlloced; ++i, ++pZone )
    {
        if ( pZone->largestSize >= size ) {
            break;
        }
    }
    if ( i == STDMEMORY_ZONECOUNT || !pZone->bAlloced )
    {
        for ( i = 0, pZone = stdMemory_aZoneData; i < STDMEMORY_ZONECOUNT && pZone->bAlloced; ++i, ++pZone )
        {
            if ( !pZone->pFirst ) {
                break;
            }
        }
        if ( i == STDMEMORY_ZONECOUNT ) {
            return stdMemory_BlockAlloc(size);
        }
        pBlock = (tMemoryZoneHeader*)malloc(STDMEMORY_ZONESIZE);
        if ( !pBlock ) {
            return stdMemory_BlockAlloc(size);
        }
        pZone->pLargest = pBlock;
        pZone->pFirst = pBlock;
        pZone->largestSize = STDMEMORY_ZONESIZE - 2 * sizeof(tMemoryZoneHeader);
        pZone->bAlloced = 1;
        pBlock->size = STDMEMORY_ZONESIZE - sizeof(tMemoryZoneHeader);
        pBlock->prevSize = 0;
        pBlock->pZone = pZone;
        /* A zero-sized terminal header ends the zone walk. */
        pBlock = (tMemoryZoneHeader*)((char*)pBlock + STDMEMORY_ZONESIZE - sizeof(tMemoryZoneHeader));
        pBlock->size = 0;
        pBlock->prevSize = STDMEMORY_ZONESIZE - sizeof(tMemoryZoneHeader);
        pBlock->pZone = pZone;
    }

    bestSize = STDMEMORY_ZONESIZE;
    pBest = NULL;
    pBlock = pZone->pFirst;
    currentblockSize = pBlock->size;
    size += sizeof(tMemoryZoneHeader);
    while ( currentblockSize )
    {
        if ( !(currentblockSize & STDMEMORY_BLOCKUSED) )
        {
            currentremainder = currentblockSize - size;
            if ( currentremainder >= 0 && currentremainder < bestSize )
            {
                bestSize = currentremainder;
                pBest = pBlock;
            }
        }
        else {
            currentblockSize &= STDMEMORY_BLOCKSIZE;
        }

        pBlock = (tMemoryZoneHeader*)((char*)pBlock + currentblockSize);
        currentblockSize = pBlock->size;
    }
#line 529
    STD_ASSERTREL(pBest);
    currentblockSize = pBest->size;
    pBest->size = size | STDMEMORY_BLOCKUSED;
    if ( currentblockSize != size )
    {
        pBlock = (tMemoryZoneHeader*)((char*)pBest + size);
        pBlock->size = currentblockSize - size;
        pBlock->prevSize = size;
        pBlock = (tMemoryZoneHeader*)((char*)pBest + currentblockSize);
        pBlock->prevSize = currentblockSize - size;
    }
    pBest->pZone = pZone;
    if ( pBest == pZone->pLargest )
    {
        bestSize = sizeof(tMemoryZoneHeader);
        pLargest = NULL;
        pBlock = pZone->pFirst;
        currentblockSize = pBlock->size;
        while ( currentblockSize )
        {
            if ( !(currentblockSize & STDMEMORY_BLOCKUSED) )
            {
                if ( currentblockSize > bestSize )
                {
                    bestSize = currentblockSize;
                    pLargest = pBlock;
                }
            }
            else {
                currentblockSize &= STDMEMORY_BLOCKSIZE;
            }

            pBlock = (tMemoryZoneHeader*)((char*)pBlock + currentblockSize);
            currentblockSize = pBlock->size;
        }
        if ( bestSize > sizeof(tMemoryZoneHeader) ) {
            bestSize -= sizeof(tMemoryZoneHeader);
        }
        else {
            bestSize = 0;
        }
        pZone->largestSize = bestSize;
        pZone->pLargest = pLargest;
    }
    return pBest + 1;
}

void J3DAPI stdMemory_BlockFree(void* pMemory)
{
    tMemoryZoneHeader* pBlock = (tMemoryZoneHeader*)pMemory - 1;
    tMemoryZoneHeader* pCurOther;
    tMemoryBlock* pZone;
    uint16_t blockLength;
    uint16_t curZoneIdx;
    uint16_t nextLen;

#line 600
    STD_ASSERTREL(pMemory);
    pZone = pBlock->pZone;
    if ( !pZone )
    {
        free(pBlock);
        return;
    }
    curZoneIdx = pZone - stdMemory_aZoneData;
    blockLength = pBlock->size;
    if ( !((blockLength & STDMEMORY_BLOCKUSED) && curZoneIdx >= 0 && curZoneIdx < STDMEMORY_ZONECOUNT) )
    {
        std_pHS->pWarningPrint("Attempting to dispose a bogus or already-disposed-of block!");
        return;
    }

    blockLength &= STDMEMORY_BLOCKSIZE;
    pCurOther = (tMemoryZoneHeader*)((char*)pBlock + blockLength);
    nextLen = pCurOther->size;
    if ( !(nextLen & STDMEMORY_BLOCKUSED) ) {
        blockLength += nextLen;
    }
    pCurOther = (tMemoryZoneHeader*)((char*)pBlock - pBlock->prevSize);
    curZoneIdx = pCurOther->size;
    if ( !(curZoneIdx & STDMEMORY_BLOCKUSED) )
    {
        blockLength += curZoneIdx;
        pBlock = pCurOther;
    }
    pBlock->size = blockLength;
    pCurOther = (tMemoryZoneHeader*)((char*)pBlock + blockLength);
    pCurOther->prevSize = blockLength;
    if ( blockLength - sizeof(tMemoryZoneHeader) > pZone->largestSize )
    {
        if ( blockLength == STDMEMORY_ZONESIZE - sizeof(tMemoryZoneHeader) )
        {
            /* Retail keeps bAlloced set; the next search can reuse this slot. */
            free(pZone->pFirst);
            pZone->pFirst = NULL;
            pZone->largestSize = 0;
            pZone->pLargest = NULL;
        }
        else
        {
            pZone->largestSize = blockLength - sizeof(tMemoryZoneHeader);
            pZone->pLargest = pBlock;
        }
    }
}

void* J3DAPI stdMemory_BlockRealloc(void* pData, size_t size)
{
    void* pNew;
    tMemoryZoneHeader* pBlock = (tMemoryZoneHeader*)pData - 1;
    size_t oldSize;
    if ( !pData ) {
        return stdMemory_BlockMalloc(size);
    }
    if ( !size )
    {
        stdMemory_BlockFree(pData);
        return NULL;
    }
    pNew = stdMemory_BlockMalloc(size);
    if ( pBlock->pZone ) {
        oldSize = (pBlock->size & ~STDMEMORY_BLOCKUSED) - sizeof(tMemoryZoneHeader);
    }
    else {
        oldSize = ((tMemoryBlockHeader*)pBlock)->size - sizeof(tMemoryBlockHeader);
    }
    if ( pNew )
    {
        size = size < oldSize ? size : oldSize;
        memcpy(pNew, pData, size);
        stdMemory_BlockFree(pData);
        return pNew;
    }
    return NULL;
}

void* J3DAPI stdMemory_BlockAlloc(size_t size)
{
    tMemoryBlockHeader* pBlock;
    pBlock = (tMemoryBlockHeader*)malloc(size + sizeof(tMemoryBlockHeader));
    if ( !pBlock ) {
        return NULL;
    }
    pBlock->size = size + sizeof(tMemoryBlockHeader);
    pBlock->pHeap = NULL;
    return pBlock + 1;
}
