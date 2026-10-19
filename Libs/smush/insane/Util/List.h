#ifndef SMUSH_INSANE_LIST_H
#define SMUSH_INSANE_LIST_H

/*
 * INSANE's doubly linked list (retail 0x004F03E0-0x004F0460): a list is two linked nodes, a head and a tail
 * sentinel; iteration runs from head.pNext to the tail's address. Names are inferred from the pointer shapes
 *
 */
typedef struct LecNode LecNode;
struct LecNode
{
    LecNode* pPrev;
    LecNode* pNext;
};

typedef struct LecList
{
    LecNode head;
    LecNode tail;
} LecList;

void LecList_Init(LecList* pList);
void LecList_Insert(LecNode* pNode, LecNode* pRef, int before);
void LecList_Remove(LecNode* pNode);

#endif
