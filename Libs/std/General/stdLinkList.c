#include "std.h"
#include "stdLinkList.h"


void J3DAPI stdLinkList_AddNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd)
{
    tLinkListNode* pNext;

#line 35
    STD_ASSERTREL(pCur != NULL);
    STD_ASSERTREL(pNodeToAdd != NULL);
    STD_ASSERTREL(pNodeToAdd->prev == NULL);
    STD_ASSERTREL(pNodeToAdd->next == NULL);

    pNext = pCur->next;
    pNodeToAdd->prev     = pCur;
    pNodeToAdd->next     = pNext;
    pCur->next           = pNodeToAdd;
    if ( pNext ) {
        pNext->prev = pNodeToAdd;
    }
}


/* The functions below up to GetFirstNode are linker-discarded in retail (relocations.yml `discarded:`); bodies read from the
   Indy debug build (recompiled under /Od: byte-identical modulo local slots), names and
   order are Jedi Knight's (its stdLinklist.c has the same ten functions in this order), parameter names are the debug
   assert texts'. Their asserts are the compiled-out STD_ASSERT family (unobservable whether the original used the REL
   variant: no retail copy). No header prototypes (declaration counts). */
void J3DAPI stdLinkList_InsertNode(tLinkListNode* pCur, tLinkListNode* pNodeToInsert)
{
    tLinkListNode* pPrev;

#line 70
    STD_ASSERT(pCur != NULL);
    STD_ASSERT(pNodeToInsert != NULL);
    STD_ASSERT(pNodeToInsert->prev == NULL);
    STD_ASSERT(pNodeToInsert->next == NULL);

    pPrev = pCur->prev;
    pNodeToInsert->next = pCur;
    pNodeToInsert->prev = pPrev;
    pCur->prev          = pNodeToInsert;
    if ( pPrev ) {
        pPrev->next = pNodeToInsert;
    }
}

void J3DAPI stdLinkList_AppendNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd)
{
    tLinkListNode* pLast;

#line 104
    STD_ASSERT(pCur != NULL);
    STD_ASSERT(pNodeToAdd != NULL);
    STD_ASSERT(pNodeToAdd->prev == NULL);
    STD_ASSERT(pNodeToAdd->next == NULL);

    pLast = pCur;
    while ( pLast->next ) {
        pLast = pLast->next;
    }

    pNodeToAdd->prev = pLast;
    pNodeToAdd->next = NULL;
    pLast->next      = pNodeToAdd;
}

void J3DAPI stdLinkList_RemoveNode(tLinkListNode* pCur)
{
#line 133
    STD_ASSERTREL(pCur != NULL);
    if ( pCur->prev ) {
        pCur->prev->next = pCur->next;
    }

    if ( pCur->next ) {
        pCur->next->prev = pCur->prev;
    }

    pCur->next = NULL;
    pCur->prev = NULL;
}

void J3DAPI stdLinkList_NewList(tLinkListNode* pFirstOfNewList)
{
#line 160
    STD_ASSERT(pFirstOfNewList != NULL);
    if ( pFirstOfNewList->prev ) {
        pFirstOfNewList->prev->next = NULL;
    }

    pFirstOfNewList->prev = NULL;
}

void J3DAPI stdLinkList_DetachNode(tLinkListNode* pCur)
{
    pCur->prev = NULL;
    pCur->next = NULL;
}

int J3DAPI stdLinkList_GetCount(tLinkListNode* pCur)
{
    int count = 0;
    while ( pCur )
    {
        ++count;
        pCur = pCur->next;
    }

    return count;
}

tLinkListNode* J3DAPI stdLinkList_GetNode(tLinkListNode* pFirstNode, int n)
{
#line 230
    STD_ASSERT(n >= 0);
    STD_ASSERT(pFirstNode != NULL);
    while ( pFirstNode && n > 0 )
    {
        --n;
        pFirstNode = pFirstNode->next;
    }

    return pFirstNode;
}

tLinkListNode* J3DAPI stdLinkList_GetLastNode(tLinkListNode* pCur)
{
    if ( !pCur ) {
        return NULL;
    }

    while ( pCur->next ) {
        pCur = pCur->next;
    }

    return pCur;
}

tLinkListNode* J3DAPI stdLinkList_GetFirstNode(tLinkListNode* pCur)
{
    if ( !pCur ) {
        return NULL;
    }

    while ( pCur->prev ) {
        pCur = pCur->prev;
    }

    return pCur;
}
