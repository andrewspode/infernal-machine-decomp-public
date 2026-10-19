/*
 * insane_memory: INSANE's `MEMORY=insane` (plain malloc/free/realloc) and `MEMORY=insane_debug` (tracing allocator)
 * services (retail 0x004EAE10-0x004EB210). The debug allocator wraps every block in a 4-byte size header and a trailing
 * guard word, logs each request and keeps a per-address record (0x1C bytes: a list link, the address, size, file,
 * line and a reference count) that Unregister prints as a request summary.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../lec.h"
#include "../Util/List.h"
#include "Memory.h"

typedef struct MemoryRecord
{
    LecNode link;
    void* pBlock;
    int size;
    int line;
    const char* pFile;
    int refs;
} MemoryRecord;

int memory_Register(LecShare* pShare, void* pParam);
static void memory_Unregister(void);
static void* memory_debugAlloc(int size, const char* pFile, int line);
static void memory_debugFree(void* pBlock, const char* pFile, int line);
static void* memory_debugRealloc(void* pBlock, int size, const char* pFile, int line);
static int memory_Track(char* pRaw, int size, const char* pFile, int line, const char* pOp);
static int memory_Check(char* pRaw, int size, const char* pFile, int line, const char* pOp);

static LecModule memory_module = { memory_Register, memory_Unregister, NULL, NULL };
static MemoryInterface memory_plain = { malloc, free, realloc, NULL };
static MemoryDebugInterface memory_debug = { memory_debugAlloc, memory_debugFree, memory_debugRealloc };

static LecList memory_list;
static LecShare* memory_pServices;
static void* memory_pData;
static int memory_total;
static int memory_maxTotal;

int memory_Register(LecShare* pShare, void* pParam)
{
    memory_pServices = pShare;
    memory_pData = pParam;
    pShare->Publish("LECMODULE=insane_memory", &memory_module);
    memory_pServices->Publish("MEMORY=insane", &memory_plain);
    memory_pServices->Publish("MEMORY=insane_debug", &memory_debug);
    memory_total = 0;
    LecList_Init(&memory_list);
    return 0;
}

static void memory_Unregister(void)
{
    MemoryRecord* pRecord;

    memory_pServices->Unpublish("MEMORY=insane_debug");
    memory_pServices->Unpublish("MEMORY=insane");
    memory_pServices->Unpublish("LECMODULE=insane_memory");
    if (memory_list.head.pNext != &memory_list.tail) {
        printf("\nMaximum Usage = %ld\n", memory_maxTotal);
        printf("\nRequest Summary\n\n");
        while (memory_list.head.pNext != &memory_list.tail) {
            pRecord = (MemoryRecord*)memory_list.head.pNext;
            printf("%-16s,\tline %4d:\t%8ld bytes, [%08lx]\tstatus = %s\n", pRecord->pFile, pRecord->line, pRecord->size, pRecord->pBlock,
                pRecord->refs > 0 ? "Unfreed" : pRecord->refs == 0 ? "Freed" : "Freed Twice");
            LecList_Remove(&pRecord->link);
            free(pRecord);
        }
    }
}

static void* memory_debugAlloc(int size, const char* pFile, int line)
{
    char* pRaw = malloc(size + 8);
    if (!pRaw) {
        return NULL;
    }
    memory_Track(pRaw, size, pFile, line, "Alloc");
    *(int*)pRaw = size;
    memory_total += size + 4;
    return pRaw + 4;
}

static int memory_Track(char* pRaw, int size, const char* pFile, int line, const char* pOp)
{
    MemoryRecord* pRecord;

    *(char**)(pRaw + size + 4) = pRaw;
    if (memory_total > memory_maxTotal) {
        memory_maxTotal = memory_total;
    }
    printf("%-16s,\tline %4d:\t%s %8ld [%08lx] (usage = %ld)\n", pFile, line, pOp, size, pRaw, memory_total);
    for (pRecord = (MemoryRecord*)memory_list.head.pNext; &pRecord->link != &memory_list.tail; pRecord = (MemoryRecord*)pRecord->link.pNext) {
        if (pRaw == pRecord->pBlock) {
            pRecord->refs++;
            return 0;
        }
    }
    pRecord = malloc(sizeof(MemoryRecord));
    if (pRecord) {
        pRecord->pBlock = pRaw;
        pRecord->size = size;
        pRecord->line = line;
        pRecord->pFile = pFile;
        pRecord->refs = 1;
        LecList_Insert(&pRecord->link, &memory_list.head, 0);
    }
    return 0;
}

static void memory_debugFree(void* pBlock, const char* pFile, int line)
{
    char* pRaw = (char*)pBlock - 4;

    memory_total -= *(int*)pRaw + 4;
    if (!memory_Check(pRaw, *(int*)pRaw, pFile, line, "Free")) {
        free(pRaw);
    }
}

static int memory_Check(char* pRaw, int size, const char* pFile, int line, const char* pOp)
{
    MemoryRecord* pRecord;
    int bad = *(char**)(pRaw + size + 4) != pRaw;

    for (pRecord = (MemoryRecord*)memory_list.head.pNext; &pRecord->link != &memory_list.tail; pRecord = (MemoryRecord*)pRecord->link.pNext) {
        if (pRaw == pRecord->pBlock) {
            if (--pRecord->refs < 0) {
                printf("%-16s,\tline %4d:\t%s  %8ld [%08lx] (usage = %ld)  %s FREE TWICE!!\n", pFile, line, pOp, size, pRaw, memory_total,
                    bad ? "OVERWRITTEN!!" : "");
                return -1;
            }
            printf("%-16s,\tline %4d:\t%s  %8ld [%08lx] (usage = %ld)  %s \n", pFile, line, pOp, size, pRaw, memory_total,
                bad ? "OVERWRITTEN!!" : "");
            return 0;
        }
    }
    printf("%-16s,\tline %4d:\t%s  %8ld [%08lx] (usage = %ld)  FREEING INVALID CHUNK!\n", pFile, line, pOp, size, pRaw, memory_total);
    return -2;
}

static void* memory_debugRealloc(void* pBlock, int size, const char* pFile, int line)
{
    char* pRaw = pBlock;
    char* pNew;
    int base;

    if (pRaw) {
        pRaw -= 4;
        base = memory_total - *(int*)pRaw - 4;
    }
    if (memory_Check(pRaw, *(int*)pRaw, pFile, line, "Re-Free")) {
        return NULL;
    }
    pNew = realloc(pRaw, size + 8);
    if (!pNew) {
        memory_Track(pRaw, *(int*)pRaw, pFile, line, "Re-UnAlloc");
        return NULL;
    }
    memory_Track(pNew, size, pFile, line, "Re-Alloc");
    *(int*)pNew = size;
    memory_total = base + size + 4;
    return pNew + 4;
}
