/*
 * insane_readauto: INSANE's automatic animation reader (retail 0x004E33F0-0x004E3810). It publishes `READANIM=insane_auto`
 * and, when asked to open a file, offers the file to every other published `READANIM` reader: first by file name
 * (pass 0), then by data (pass 1), and forwards the calls to the first that accepts it.
 *
 */
#include <stddef.h>
#include "../lec.h"
#include "../readanim.h"
#include "../Memory/Memory.h"

typedef struct ReadAutoHandle
{
    ReadAnimInterface* pReader;
    void* hReader;
} ReadAutoHandle;

int readauto_Register(LecShare* pShare, void* pParam);
static void readauto_Unregister(void);
static int readauto_Startup(void);
static void readauto_Shutdown(void);
static const char* readauto_Query(int code);
static int readauto_Open(void** ppHandle, const char* pFile);
static void readauto_Close(void** ppHandle);
static int readauto_Op3(void* pHandle, int a, int b, int c, int d, int e);
static int readauto_Op4(void* pHandle, int a, int b);
static int readauto_Read(int a, int b, void* pFile);
static int readauto_Recognize(const char* pFile, int pass);

static LecModule readauto_module = { readauto_Register, readauto_Unregister, readauto_Startup, readauto_Shutdown };
static ReadAnimInterface readauto_interface = {
    readauto_Query, readauto_Open, readauto_Close, readauto_Op3, readauto_Op4, readauto_Read, readauto_Recognize
};

static MemoryDebugInterface* readauto_pAlloc;
static LecShare* readauto_pFileService;
static LecShare* readauto_pShare;
static void* readauto_pParam;

int readauto_Register(LecShare* pShare, void* pParam)
{
    readauto_pShare = pShare;
    readauto_pParam = pParam;
    pShare->Publish("LECMODULE=insane_readauto", &readauto_module);
    readauto_pShare->Publish("READANIM=insane_auto", &readauto_interface);
    return 0;
}

static void readauto_Unregister(void)
{
    readauto_pShare->Unpublish("READANIM=insane_auto");
    readauto_pShare->Unpublish("LECMODULE=insane_readauto");
}

static int readauto_Startup(void)
{
    readauto_pAlloc = (MemoryDebugInterface*)readauto_pShare->Subscribe("MEMORY.workspace", readauto_pParam);
    readauto_pFileService = (LecShare*)readauto_pShare->Subscribe("FILE.io", readauto_pParam);
    if (!readauto_pAlloc) {
        readauto_pShare->ReportError("ReadAuto Module: Cannot allocate memory services.");
        return -1;
    }
    if (!readauto_pFileService) {
        readauto_pShare->ReportError("ReadAuto Module: Cannot allocate file services.");
        return -1;
    }
    return 0;
}

static void readauto_Shutdown(void)
{
    readauto_pShare->Unsubscribe(readauto_pAlloc);
    readauto_pShare->Unsubscribe(readauto_pFileService);
}

static const char* readauto_Query(int code)
{
    ReadAnimInterface* pReader;
    void* pEnum;
    const char* pText;
    int count;
    int i;
    int k;

    switch (code) {
        case 0:
            return "Auto Reader";
        case 1:
            return "Reads an animation file automatically using published modules.";
        case 2:
            return "This module searches the system for all published READANIM modules and queries them in sequence from earliest to latest "
                   "to see if any of them are compatible with a given file.  The check is first done using filenames only.  If none are "
                   "found to match, data-level comparisons are done.";
    }
    if (code < 100 || code >= 120) {
        return "";
    }
    count = readauto_pShare->Count("READANIM");
    for (i = 0; i < count; ++i) {
        if (code < 100) {
            break;
        }
        pEnum = readauto_pShare->Enumerate("READANIM", i);
        if (pEnum) {
            pReader = (ReadAnimInterface*)readauto_pShare->Subscribe("READANIM.chainto", pEnum);
            if (pReader) {
                if (pReader != &readauto_interface) {
                    k = 100;
                    do {
                        pText = pReader->Query(k);
                        ++k;
                        if (pText && *pText) {
                            --code;
                        }
                    } while (pText && *pText && code >= 100);
                }
                readauto_pShare->Unsubscribe(pReader);
            }
        }
    }
    if (code < 100) {
        return pText;
    }
    return "";
}

static int readauto_Read(int a, int b, void* pFile)
{
    int rc = readauto_Open(&pFile, pFile);

    if (rc) {
        return rc;
    }
    rc = readauto_Op4(pFile, a, b);
    readauto_Close(&pFile);
    return rc;
}

static int readauto_Open(void** ppHandle, const char* pFile)
{
    ReadAutoHandle* pHandle;
    ReadAnimInterface* pReader;
    int count;
    int pass;
    int i;
    int rc;

#line 251
    pHandle = readauto_pAlloc->Alloc(sizeof(ReadAutoHandle), INSANE_FILE, __LINE__);
    rc = pHandle ? 0 : -1;
    if (rc) {
        return rc;
    }
    count = readauto_pShare->Count("READANIM");
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < count; ++i) {
            pReader = (ReadAnimInterface*)readauto_pShare->Enumerate("READANIM", i);
            if (pReader) {
                pHandle->pReader = readauto_pShare->Subscribe("READANIM.chainto", pReader);
                if (pHandle->pReader) {
                    if (pHandle->pReader != &readauto_interface && pHandle->pReader->Recognize(pFile, pass)) {
                        rc = pHandle->pReader->Open(&pHandle->hReader, pFile);
                        if (rc) {
                            readauto_pShare->Unsubscribe(pHandle->pReader);
                            if (pHandle) {
#line 290
                                readauto_pAlloc->Free(pHandle, INSANE_FILE, __LINE__);
                            }
                            return rc;
                        }
                        *ppHandle = pHandle;
                        return 0;
                    }
                    readauto_pShare->Unsubscribe(pHandle->pReader);
                }
            }
        }
    }
    readauto_pShare->ReportError("ReadAuto Module: Cannot Recognize File Type");
    if (pHandle) {
#line 280
        readauto_pAlloc->Free(pHandle, INSANE_FILE, __LINE__);
    }
    return -1;
}

static void readauto_Close(void** ppHandle)
{
    ReadAutoHandle* pHandle = *ppHandle;

    pHandle->pReader->Close(&pHandle->hReader);
    readauto_pShare->Unsubscribe(pHandle->pReader);
    if (*ppHandle) {
#line 310
        readauto_pAlloc->Free(*ppHandle, INSANE_FILE, __LINE__);
    }
    *ppHandle = NULL;
}

static int readauto_Op3(void* pHandle, int a, int b, int c, int d, int e)
{
    ReadAutoHandle* p = pHandle;

    return p->pReader->Op3(p->hReader, a, b, c, d, e);
}

static int readauto_Op4(void* pHandle, int a, int b)
{
    ReadAutoHandle* p = pHandle;

    return p->pReader->Op4(p->hReader, a, b);
}

static int readauto_Recognize(const char* pFile, int pass)
{
    return 0;
}
