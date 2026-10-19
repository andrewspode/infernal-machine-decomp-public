/*
 * insane_list: INSANE's intrusive doubly linked list helpers (retail 0x004F03E0-0x004F0460).
 *
 */
#include <stddef.h>
#include "List.h"

void LecList_Init(LecList* pList)
{
    pList->tail.pPrev = &pList->head;
    pList->head.pPrev = NULL;
    pList->head.pNext = &pList->tail;
    pList->tail.pNext = NULL;
}

void LecList_Insert(LecNode* pNode, LecNode* pRef, int before)
{
    if (before) {
        pNode->pNext = pRef;
        pNode->pPrev = pRef->pPrev;
        if (pRef->pPrev) {
            pRef->pPrev->pNext = pNode;
        }
        pRef->pPrev = pNode;
    }
    else {
        pNode->pPrev = pRef;
        pNode->pNext = pRef->pNext;
        if (pRef->pNext) {
            pRef->pNext->pPrev = pNode;
        }
        pRef->pNext = pNode;
    }
}

void LecList_Remove(LecNode* pNode)
{
    if (pNode->pPrev) {
        pNode->pPrev->pNext = pNode->pNext;
    }
    if (pNode->pNext) {
        pNode->pNext->pPrev = pNode->pPrev;
    }
}
