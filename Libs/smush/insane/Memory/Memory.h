#ifndef SMUSH_INSANE_MEMORY_H
#define SMUSH_INSANE_MEMORY_H

/*
 * INSANE's memory services (retail 0x004EAE10-0x004EB210, insane_memory). `MEMORY=insane` is plain
 * malloc/free/realloc; `MEMORY=insane_debug` (also what modules receive when they subscribe to
 * `MEMORY.workspace`) records the caller's file and line for every request.
 */
#include <stddef.h>

typedef struct MemoryInterface
{
    void* (*Alloc)(size_t size);
    void  (*Free)(void* pBlock);
    void* (*Realloc)(void* pBlock, size_t size);
    void* pReserved;
} MemoryInterface;

typedef struct MemoryDebugInterface
{
    void* (*Alloc)(int size, const char* pFile, int line);
    void  (*Free)(void* pBlock, const char* pFile, int line);
    void* (*Realloc)(void* pBlock, int size, const char* pFile, int line);
} MemoryDebugInterface;

#endif
