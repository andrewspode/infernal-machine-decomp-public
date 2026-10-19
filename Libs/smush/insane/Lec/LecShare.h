#ifndef SMUSH_INSANE_LECSHARE_H
#define SMUSH_INSANE_LECSHARE_H

/*
 * The LEC framework's start-up interface (LecShare.c), called by the host (SmushPlay.c): the module list LecShare_Init registers,
 * the batch-subscription table the share table's BatchSubscribe/BatchUnsubscribe slots take, and the log every module can print to.
 */
#include "../lec.h"

typedef struct LecModuleEntry
{
    int (*pfnRegister)(LecShare* pShare, void* pParam);
    void* pParam;
} LecModuleEntry;

typedef struct LecBatch
{
    void** ppOut;
    const char* pName;
    const char* pPreferred;
} LecBatch;

typedef void (*LecErrorFn)(const char* pFormat, ...);

LecShare* LecShare_Init(LecErrorFn pError, LecModuleEntry* pModules, const char* pA, const char* pB);
int LecShare_UnregisterAll(void);
int LecShare_Startup(void);
int LecShare_Shutdown(void);
void lec_Log(const char* pFormat, ...);

#endif
