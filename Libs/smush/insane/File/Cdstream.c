/*
 * insane_streamer: INSANE's `STREAMER=insane_streamer` service (retail 0x004E4D50-0x004E5470): a ring-buffer file streamer that
 * reads a file in 2 KB sectors ahead of the consumer.
 */
#include <stddef.h>
#include <string.h>
#include "../lec.h"
#include "../filio.h"
#include "../Memory/Memory.h"
#include "Framestream.h"
#include "Cdstream.h"

typedef struct Stream
{
    char* pBuf;                                 /* 0x00 ring buffer base */
    char* pEnd;                                 /* 0x04 base + ringSize */
    int ringSize;                               /* 0x08 (multiple of 0x800) */
    int maxFrame;                               /* 0x0C */
    int field10;                                /* 0x10 busy count: Prime raises it around its state update, Advance refuses while set */
    int active;                                 /* 0x14 */
    int field18;                                /* 0x18 */
    char* pStart;                               /* 0x1C */
    int field20;                                /* 0x20 */
    char* pRead;                                /* 0x24 */
    char* pWrite;                               /* 0x28 */
    int field2C;                                /* 0x2C */
    int hFile;                                  /* 0x30 */
    int filePos;                                /* 0x34 */
    int fileSize;                               /* 0x38 */
    char name[0xC8];                            /* 0x3C */
    int startOffset;                            /* 0x104 */
    int reopen;                                 /* 0x108 */
    int (*pfnRetry)(int code, const char* pName); /* 0x10C */
} Stream;

int streamer_Register(LecShare* pShare, void* pParam);
static void streamer_Unregister(void);
static int streamer_Startup(void);
static void streamer_Shutdown(void);
static int streamer_Create(Stream** ppStream, int ringSize, int frameSize);
static void streamer_Destroy(Stream** ppStream);
static void streamer_Reset(Stream* pStream);
static void streamer_Fill(Stream* pStream, int limit);
static int streamer_SetFile(Stream* pStream, const char* pName, int startOffset);
static int streamer_Prime(Stream* pStream);
static int streamer_IsActive(Stream* pStream);
static int streamer_Advance(Stream* pStream, int n);
static int streamer_Cancel(Stream* pStream);
static int streamer_InRange(Stream* pStream, int n);
static int streamer_Percent(Stream* pStream);

static LecModule streamer_module = { streamer_Register, streamer_Unregister, streamer_Startup, streamer_Shutdown };
static StreamerInterface streamer_interface = {
    streamer_Create, streamer_Destroy, streamer_SetFile, streamer_IsActive, streamer_Fill, streamer_Advance, streamer_Reset, streamer_Percent
};

static MemoryDebugInterface* streamer_pAlloc;
static LecShare* streamer_pShare;
static FilioInterface* streamer_pFile;
static void* streamer_pParam;
static int streamer_wrap;

int streamer_Register(LecShare* pShare, void* pParam)
{
    streamer_pShare = pShare;
    streamer_pParam = pParam;
    pShare->Publish("LECMODULE=insane_streamer", &streamer_module);
    streamer_pShare->Publish("STREAMER=insane_streamer", &streamer_interface);
    return 0;
}

static void streamer_Unregister(void)
{
    streamer_pShare->Unpublish("STREAMER=insane_streamer");
    streamer_pShare->Unpublish("LECMODULE=insane_streamer");
}

static int streamer_Startup(void)
{
    streamer_pAlloc = (MemoryDebugInterface*)streamer_pShare->Subscribe("MEMORY.workspace", streamer_pParam);
    streamer_pFile = (FilioInterface*)streamer_pShare->Subscribe("FILIO.io", streamer_pParam);
    if (!streamer_pAlloc) {
        streamer_pShare->ReportError("Streamer Module: Cannot allocate memory services.");
        return -1;
    }
    if (!streamer_pFile) {
        streamer_pShare->ReportError("Streamer Module: Cannot allocate filio services.");
        return -1;
    }
    return 0;
}

static void streamer_Shutdown(void)
{
    streamer_pShare->Unsubscribe(streamer_pAlloc);
    streamer_pShare->Unsubscribe(streamer_pFile);
}

static int streamer_Create(Stream** ppStream, int ringSize, int frameSize)
{
    Stream* pStream;
    char* pBuf;
    int rc;

#line 195
    pStream = streamer_pAlloc->Alloc(sizeof(Stream), INSANE_FILE, __LINE__);
    rc = pStream ? 0 : -1;
    if (rc == 0) {
        pStream->pfnRetry = NULL;
#line 199
        pBuf = streamer_pAlloc->Alloc(ringSize + frameSize, INSANE_FILE, __LINE__);
        pStream->pBuf = pBuf;
        rc = pBuf ? 0 : -1;
        if (rc == 0) {
            pStream->ringSize = ringSize & ~0x7FF;
            pStream->maxFrame = frameSize;
            pStream->pEnd = pBuf + pStream->ringSize;
            pStream->hFile = 0;
            pStream->startOffset = 0;
            pStream->reopen = 0;
            memset(pBuf, 0, pStream->ringSize);
            *ppStream = pStream;
        }
        else if (pStream) {
#line 214
            streamer_pAlloc->Free(pStream, INSANE_FILE, __LINE__);
        }
    }
    return rc;
}

static void streamer_Destroy(Stream** ppStream)
{
    Stream* pStream;

    if (ppStream) {
        pStream = *ppStream;
        if (pStream->hFile) {
            streamer_pFile->Close(pStream->hFile);
            pStream->hFile = 0;
        }
        if (pStream->pBuf) {
#line 239
            streamer_pAlloc->Free(pStream->pBuf, INSANE_FILE, __LINE__);
        }
        pStream->pBuf = NULL;
        if (*ppStream) {
#line 240
            streamer_pAlloc->Free(*ppStream, INSANE_FILE, __LINE__);
        }
        *ppStream = NULL;
    }
}

static void streamer_Reset(Stream* pStream)
{
    pStream->active = 0;
    pStream->pRead = pStream->pBuf;
    pStream->pWrite = pStream->pBuf;
    pStream->field18 = 0;
    pStream->field2C = 0;
    pStream->field20 = 0;
    if (pStream->hFile) {
        streamer_pFile->Close(pStream->hFile);
        pStream->hFile = 0;
    }
    pStream->field10 = 0;
}

static void streamer_Fill(Stream* pStream, int limit)
{
    int ok;
    int avail;
    int n;
    char* p;
    char* pWrite;                               /* release-chosen local: no retail trace */
    int more;                                   /* release-chosen local: no retail trace */

    ok = !streamer_Prime(pStream);
    more = pStream->hFile && pStream->filePos < pStream->fileSize;
    if (more) {
        pWrite = pStream->pWrite;
        if (pWrite <= pStream->pRead && pWrite != pStream->pBuf) {
            avail = pStream->pEnd - pStream->pRead;
        }
        else {
            if (pWrite == pStream->pBuf && pStream->pEnd - pStream->pRead > 0x800) {
                avail = pStream->pEnd - pStream->pRead - 0x800;
            }
            else {
                avail = (pWrite - pStream->pRead - 0x800) & ~0x7FF;
                if (avail < 0x800) {
                    return;
                }
            }
        }
        if (!ok) {
            if (avail > limit) {
                avail = limit;
            }
        }
        else if (avail > pStream->maxFrame) {
            avail = pStream->maxFrame;
        }
        n = (pStream->fileSize - pStream->filePos < avail) ? pStream->fileSize - pStream->filePos : avail;
        if (streamer_pFile->Read(pStream->hFile, (void*)pStream->pRead, n) == n) {
            pStream->filePos += avail;
            p = pStream->pRead + avail;
            if (p >= pStream->pEnd) {
                p -= pStream->ringSize;
            }
            pStream->pRead = p;
            if (pStream->active) {
                pStream->field18 = 1;
            }
        }
        else {
            streamer_pFile->Seek(pStream->hFile, pStream->filePos, 0);
            if (pStream->pfnRetry) {
                pStream->pfnRetry(0, NULL);
            }
        }
    }
}

static int streamer_SetFile(Stream* pStream, const char* pName, int startOffset)
{
    int hFile;

    if (pName) {
        hFile = streamer_pFile->Open(pName, "rb");
        if (!hFile) {
            return -1;
        }
        streamer_pFile->Close(hFile);
        strcpy(pStream->name, pName);
    }
    else {
        pStream->name[0] = 0;
    }
    pStream->reopen = 1;
    pStream->startOffset = startOffset;
    return 0;
}

static int streamer_Prime(Stream* pStream)
{
    int hFile;
    int rc = -1;

    if (pStream->reopen) {
        if (pStream->name[0]) {
            hFile = streamer_pFile->Open(pStream->name, "rb");
            while (!hFile) {
                if (!pStream->pfnRetry || pStream->pfnRetry(1, pStream->name)) {
                    pStream->reopen = 0;
                    return -2;
                }
                hFile = streamer_pFile->Open(pStream->name, "rb");
            }
            if (pStream->hFile) {
                streamer_pFile->Close(pStream->hFile);
                pStream->hFile = 0;
            }
            pStream->hFile = hFile;
            streamer_pFile->Seek(hFile, 0, 2);
            pStream->fileSize = streamer_pFile->Tell(pStream->hFile);
        }
        streamer_pFile->Seek(pStream->hFile, pStream->startOffset & ~0x7FF, 0);
        pStream->field10++;
        pStream->filePos = pStream->startOffset & ~0x7FF;
        pStream->field20 = pStream->startOffset & 0x7FF;
        pStream->pStart = pStream->pRead;
        pStream->active = 1;
        pStream->field18 = 0;
        pStream->reopen = 0;
        pStream->field10--;
        rc = 0;
    }
    return rc;
}

static int streamer_IsActive(Stream* pStream)
{
    if (pStream->field18 && pStream->active) {
        return 1;
    }
    return 0;
}

static int streamer_Advance(Stream* pStream, int n)
{
    int len;

    if (pStream->field10 || !pStream->hFile) {
        return -1;
    }
    if (n > pStream->maxFrame) {
        streamer_pShare->ReportError("File Streamer:  Invalid frame size %ld", n);
        return -2;
    }
    if (!streamer_wrap) {
        pStream->pWrite += pStream->field2C;
        pStream->field2C = 0;
        if (pStream->pWrite >= pStream->pEnd) {
            pStream->pWrite -= pStream->ringSize;
        }
        streamer_wrap = 1;
    }
    if (n < 0) {
        if (pStream->active) {
            pStream->field2C = 0;
            pStream->pWrite = pStream->pStart;
        }
        streamer_wrap = 0;
        return -1;
    }
    if (pStream->active) {
        streamer_Cancel(pStream);
        streamer_wrap = 0;
        return 0;
    }
    if (pStream->field20) {
        n += pStream->field20;
    }
    if (!streamer_InRange(pStream, n)) {
        return -1;
    }
    streamer_wrap = 0;
    pStream->field2C = n;
    len = pStream->pWrite - pStream->pEnd + n;
    if (len > 0) {
        framestream_Copy(pStream->pEnd, pStream->pBuf, len);
    }
    if (!pStream->active && pStream->field20) {
        int t = pStream->field20;
        pStream->field20 = 0;
        return (int)(pStream->pWrite + t);
    }
    return (int)pStream->pWrite;
}

static int streamer_Cancel(Stream* pStream)
{
    if (pStream->active) {
        pStream->active = 0;
        pStream->field18 = 0;
        pStream->pWrite = pStream->pStart;
        pStream->field2C = 0;
        return 0;
    }
    return -1;
}

static int streamer_InRange(Stream* pStream, int n)
{
    if (pStream->pWrite <= pStream->pRead) {
        if (pStream->pRead >= pStream->pWrite + n) {
            return 1;
        }
    }
    else if (pStream->pRead + pStream->ringSize >= pStream->pWrite + n) {
        return 1;
    }
    return 0;
}

static int streamer_Percent(Stream* pStream)
{
    if (!pStream->hFile) {
        return 100;
    }
    if (pStream->filePos >= pStream->fileSize - 0x800) {
        return 100;
    }
    if (pStream->pWrite <= pStream->pRead) {
        return (pStream->pRead - pStream->pWrite) * 100 / pStream->ringSize;
    }
    return (pStream->ringSize - (pStream->pWrite - pStream->pRead)) * 100 / pStream->ringSize;
}
