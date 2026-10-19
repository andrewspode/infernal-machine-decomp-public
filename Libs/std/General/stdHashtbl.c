#include "std.h"
#include "stdHashtbl.h"
#include "stdLinklist.h"
#include "stdMemory.h"


static const signed int stdHashtbl_aPrimeTable[32] = {
    23u,   53u,   79u,   101u,  151u,  211u,  251u,  307u,
    353u,  401u,  457u,  503u,  557u,  601u,  653u,  701u,
    751u,  809u,  853u,  907u,  953u,  1009u, 1103u, 1201u,
    1301u, 1409u, 1511u, 1601u, 1709u, 1801u, 1901u, 1999u
};

static unsigned int J3DAPI CalculateHash(const char* pData, signed int hashSize);
static signed int J3DAPI GetNextPrime(signed int size);
static int J3DAPI isPrime(signed int val);

unsigned int J3DAPI CalculateHash(const char* pData, signed int hashSize)
{
    signed int hashValue = 0;
    for ( ; *pData; ++pData ) {
        hashValue = (65599 * hashValue) + *pData;
    }

    hashValue = hashValue % hashSize;
    hashValue = abs(hashValue);
#line 57
    STD_ASSERTREL(hashValue >= 0);
    STD_ASSERTREL(hashValue < hashSize);
    return hashValue;
}

int J3DAPI isPrime(signed int val)
{
    signed int i;

    for ( i = 2; i < val - 1; ++i )
    {
        if ( !(val % i) ) {
            return 0;
        }
    }

    return 1;
}

static size_t J3DAPI stdHashtbl_nextPrime(size_t size)
{
    size_t candidate = size;
    while ( !isPrime(candidate) ) {
        ++candidate;
    }

#line 111
    STD_ASSERTREL(isPrime(candidate));
    return candidate;
}

signed int J3DAPI GetNextPrime(signed int size)
{
    /* Original bug: nextPrime is uninitialized when the table scan is exhausted. */
    signed int nextPrime;
    signed int i;
    for ( i = 0; i < 32; ++i )
    {
        if ( size < stdHashtbl_aPrimeTable[i] )
        {
            nextPrime = stdHashtbl_aPrimeTable[i];
            break;
        }
    }

    if ( size > 1999 ) {
        nextPrime = stdHashtbl_nextPrime(size);
    }

#line 149
    STD_ASSERTREL(isPrime(nextPrime));
    return nextPrime;
}

tHashTable* J3DAPI stdHashtbl_New(size_t size)
{
#line 174
    tHashTable* pTable = (tHashTable*)STDMALLOC(sizeof(tHashTable));
    if ( !pTable ) {
        return NULL;
    }

    memset(pTable, 0, sizeof(tHashTable));

    pTable->numNodes = GetNextPrime(size);
#line 184
    pTable->paNodes = (tLinkListNode*)STDMALLOC(sizeof(tLinkListNode) * pTable->numNodes);
    if ( !pTable->paNodes ) {
        return NULL;
    }

    memset(pTable->paNodes, 0, sizeof(tLinkListNode) * pTable->numNodes);
    pTable->pfHashFunc = CalculateHash;
    return pTable;
}

static tLinkListNode* J3DAPI stdHashtbl_GetTailNode(const tLinkListNode* pCur)
{
#line 209
    STD_ASSERTREL(pCur != NULL);
    while ( pCur->next ) {
        pCur = pCur->next;
    }

    return (tLinkListNode*)pCur;
}

void J3DAPI stdHashtbl_Free(tHashTable* pTable)
{
    signed int i;

#line 260
    STD_ASSERTREL(pTable != NULL);
    for ( i = 0; i < (signed int)pTable->numNodes; ++i ) {
        stdHashtbl_FreeListNodes(&pTable->paNodes[i]);
    }

    stdMemory_Free(pTable->paNodes);
    stdMemory_Free(pTable);
}

void J3DAPI stdHashtbl_FreeListNodes(tLinkListNode* pNode)
{
    tLinkListNode* pNextNode;
    pNode = pNode->next;
    while ( pNode )
    {
        pNextNode = pNode->next;
        stdMemory_Free(pNode);
        pNode = pNextNode;
    }
}

int J3DAPI stdHashtbl_Add(tHashTable* pTable, const char* pName, void* pData)
{
    unsigned int curnodeIdx;
    tLinkListNode* cur;
    tLinkListNode* pNode;

#line 295
    STD_ASSERTREL(pTable != NULL);
    STD_ASSERTREL(pData != NULL);

    if ( stdHashtbl_Find(pTable, pName) ) {
        return 0;
    }

    curnodeIdx = pTable->pfHashFunc(pName, pTable->numNodes);
    cur = stdHashtbl_GetTailNode(&pTable->paNodes[curnodeIdx]);
    if ( !cur->name )
    {
        memset(&pTable->paNodes[curnodeIdx], 0, sizeof(pTable->paNodes[curnodeIdx]));
        pTable->paNodes[curnodeIdx].name = pName;
        pTable->paNodes[curnodeIdx].data = pData;
    }
    else
    {
#line 315
        pNode = (tLinkListNode*)STDMALLOC(sizeof(tLinkListNode));
        if ( !pNode ) {
            return 0;
        }

        memset(pNode, 0, sizeof(tLinkListNode));
        pNode->name = pName;
        pNode->data = pData;
        stdLinkList_AddNode(cur, pNode);
    }

    return 1;
}

void* J3DAPI stdHashtbl_Find(const tHashTable* pTable, const char* pName)
{
    int pNodeHash;
    tLinkListNode* pNode = stdHashtbl_FindNode(pTable, pName, &pNodeHash);
    if ( pNode ) {
        return pNode->data;
    }
    return NULL;
}

tLinkListNode* J3DAPI stdHashtbl_FindNode(const tHashTable* pTable, const char* pName, int* pNodeIdx)
{
    tLinkListNode* pCurNode;
    if ( !pTable ) {
        return NULL;
    }

    *pNodeIdx = pTable->pfHashFunc(pName, pTable->numNodes);
    pCurNode = &pTable->paNodes[*pNodeIdx];
    while ( pCurNode && pCurNode->name )
    {
        if ( streq(pCurNode->name, pName) ) {
            return pCurNode;
        }

        pCurNode = pCurNode->next;
    }

    return NULL;
}

int J3DAPI stdHashtbl_Remove(tHashTable* pTable, const char* pName)
{
    int curnodeIdx;
    tLinkListNode* pNode;
    tLinkListNode* nodeNext;

#line 415
    STD_ASSERTREL(pTable != NULL);

    pNode = stdHashtbl_FindNode(pTable, pName, &curnodeIdx);
    if ( pNode )
    {
        nodeNext = pNode->next;
        stdLinkList_RemoveNode(pNode);

        if ( &pTable->paNodes[curnodeIdx] == pNode )
        {
            tLinkListNode* pNext;

            if ( !nodeNext )
            {
                memset(&pTable->paNodes[curnodeIdx], 0, sizeof(pTable->paNodes[curnodeIdx]));
            }
            else
            {
                memcpy(&pTable->paNodes[curnodeIdx], nodeNext, sizeof(pTable->paNodes[curnodeIdx]));
                pNext = pTable->paNodes[curnodeIdx].next;
                if ( pNext ) {
                    pNext->prev = &pTable->paNodes[curnodeIdx];
                }
                stdMemory_Free(nodeNext);
            }
        }
        else
        {
            stdMemory_Free(pNode);
        }

        return 1;
    }

    return 0;
}

#ifdef J3D_DEBUG
/* Debug-only (debug 0x004FC504, assert line 533): its 100.0f has no slot in retail's stdHashtbl pool, so the
   release build did not compile it. */
#line 525
void J3DAPI stdHashtbl_PrintDiagnostics(const tHashTable* pTable)
{
    int total;
    int i;
    int filled;
    int len;
    int maxLookups;

    STD_ASSERT(pTable != NULL);

    std_pHS->pDebugPrint("\nHASHTABLE Diagnostics\n");
    std_pHS->pDebugPrint("---------------------\n");

    filled = 0;
    total = 0;
    maxLookups = 0;
    for ( i = 0; i < pTable->numNodes; ++i )
    {
        if ( pTable->paNodes[i].name )
        {
            ++filled;
            len = stdLinkList_GetCount(&pTable->paNodes[i]);
            total += len;
            if ( len > maxLookups )
            {
                maxLookups = len;
            }
        }
    }

    std_pHS->pDebugPrint(" Maximum Lookups = %d\n", maxLookups);
    std_pHS->pDebugPrint(" Filled Indices = %d/%d (%2.2f%%)\n", filled, pTable->numNodes, (float)filled * 100.0f / ((float)pTable->numNodes));
    std_pHS->pDebugPrint(" Average Lookup = %2.2f\n", (float)total / ((float)filled));
    std_pHS->pDebugPrint(" Weighted Lookup = %2.2f\n", (float)total / ((float)pTable->numNodes));
    std_pHS->pDebugPrint("---------------------\n");
}
#endif

/* Dump: discarded by the retail linker (no caller in either image), restored in full from the debug build
   (0x004FC663, after PrintDiagnostics; assert line 578). Jedi Knight's stdHashTable_Dump. */
void J3DAPI stdHashtbl_Dump(const tHashTable* pTable)
{
    int i;
    tLinkListNode* pNode;

#line 578
    STD_ASSERT(pTable != NULL);

    std_pHS->pDebugPrint("\nHASHTABLE\n---------\n");
    for ( i = 0; i < pTable->numNodes; ++i )
    {
        std_pHS->pDebugPrint("Index: %d\t", i);
        pNode = &pTable->paNodes[i];
        std_pHS->pDebugPrint("Strings:", i);
        while ( pNode )
        {
            std_pHS->pDebugPrint(" '%s'", pNode->name);
            pNode = pNode->next;
        }

        std_pHS->pDebugPrint("\n");
    }
}
