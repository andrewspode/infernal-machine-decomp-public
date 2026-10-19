#ifndef STD_STDHASHTBL_H
#define STD_STDHASHTBL_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

tHashTable* J3DAPI stdHashtbl_New(size_t size);
void J3DAPI stdHashtbl_Free(tHashTable* pTable);
void J3DAPI stdHashtbl_FreeListNodes(tLinkListNode* pNode);
int J3DAPI stdHashtbl_Add(tHashTable* pTable, const char* pName, void* pData);
void* J3DAPI stdHashtbl_Find(const tHashTable* pTable, const char* pName);
tLinkListNode* J3DAPI stdHashtbl_FindNode(const tHashTable* pTable, const char* pName, int* pNodeHash);
int J3DAPI stdHashtbl_Remove(tHashTable* pTable, const char* pName);
J3D_EXTERN_C_END
#endif // STD_STDHASHTBL_H
