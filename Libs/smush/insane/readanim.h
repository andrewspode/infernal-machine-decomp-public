#ifndef SMUSH_INSANE_READANIM_H
#define SMUSH_INSANE_READANIM_H

/*
 * INSANE's `READANIM` interface: one animation-file reader (retail: insane_readauto's own table at 0x0054BF18 and the
 * per-format readers it enumerates). Slot roles are inferred from readauto's calls: Query
 * answers text codes (0 name, 1 short description, 2 long description, 100.. the file extensions the reader takes),
 * Recognize says whether the reader takes a file (pass 0 by file name, pass 1 by data).
 */
typedef struct ReadAnimInterface
{
    const char* (*Query)(int code);
    int  (*Open)(void** ppHandle, const char* pFile);
    void (*Close)(void** ppHandle);
    int  (*Op3)(void* hReader, int a, int b, int c, int d, int e);
    int  (*Op4)(void* hReader, int a, int b);
    int  (*Read)(int a, int b, void* pFile);
    int  (*Recognize)(const char* pFile, int pass);
} ReadAnimInterface;

#endif
