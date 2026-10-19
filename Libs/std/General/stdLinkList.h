#ifndef STD_STDLINKLIST_H
#define STD_STDLINKLIST_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

void J3DAPI stdLinkList_AddNode(tLinkListNode* pCur, tLinkListNode* pNodeToAdd);
void J3DAPI stdLinkList_RemoveNode(tLinkListNode* pCur);
int J3DAPI stdLinkList_GetCount(tLinkListNode* pCur); // linker-discarded; the debug build's stdHashtbl calls it


J3D_EXTERN_C_END
#endif // STD_STDLINKLIST_H
