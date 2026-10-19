#include "std.h"
#include "stdMemory.h"
#include "stdCircBuf.h"

/* Retail diagnostic locations are part of the executable data. */
#line 43

int J3DAPI stdCircBuf_New(tCircularBuffer* pCirc, int numElementsDesired, int sizeOfEachElement)
{
    STD_ASSERTREL(pCirc != NULL);
    STD_ASSERTREL(numElementsDesired > 0);
    STD_ASSERTREL(sizeOfEachElement > 0);

    memset(pCirc, 0, sizeof(tCircularBuffer));

#line 53
    pCirc->paElements = (uint8_t*)STDMALLOC(numElementsDesired * sizeOfEachElement);
    if ( !pCirc->paElements )
    {
        return 0;
    }

    memset(pCirc->paElements, 0, numElementsDesired * sizeOfEachElement);
    pCirc->numAllocated = numElementsDesired;
    pCirc->elementSize = sizeOfEachElement;
    return 1;
}

void J3DAPI stdCircBuf_Free(tCircularBuffer* pCirc)
{
    if ( pCirc->paElements )
    {
        stdMemory_Free(pCirc->paElements);
    }

    pCirc->numAllocated = 0;
    pCirc->paElements = 0;
    pCirc->numValidElements = 0;
}

void* J3DAPI stdCircBuf_Purge(tCircularBuffer* pCirc)
{
    uint8_t* pElement;

#line 103
    STD_ASSERTREL(pCirc != NULL);
    if ( !pCirc->paElements )
    {
        return NULL;
    }
    if ( !pCirc->numValidElements )
    {
        return NULL;
    }

    --pCirc->numValidElements;
#line 112
    STD_ASSERT(pCirc->paElements != NULL);
    pElement = &pCirc->paElements[pCirc->iFirst * pCirc->elementSize];
    pCirc->iFirst = (pCirc->iFirst + 1) % pCirc->numAllocated;
    return pElement;
}

void* J3DAPI stdCircBuf_GetNextElement(tCircularBuffer* pCirc)
{
    int idx;

#line 145
    STD_ASSERTREL(pCirc != NULL);
    if ( !pCirc->paElements )
    {
        return NULL;
    }

    if ( pCirc->numValidElements >= pCirc->numAllocated )
    {
        stdCircBuf_Purge(pCirc);
    }

#line 153
    STD_ASSERTREL(pCirc->numValidElements <= pCirc->numAllocated);
    STD_ASSERTREL(pCirc->iFirst < pCirc->numAllocated);

    idx = (pCirc->iFirst + pCirc->numValidElements) % pCirc->numAllocated;
    ++pCirc->numValidElements;
#line 158
    STD_ASSERTREL(pCirc->paElements != NULL);
    return &pCirc->paElements[idx * pCirc->elementSize];
}
