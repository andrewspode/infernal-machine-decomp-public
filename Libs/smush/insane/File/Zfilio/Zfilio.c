/*
 * insane_zfilio: INSANE's `FILIO=insane_zfilio` service (retail 0x004E9B30-0x004EA020). It sits on top of the next file layer
 * (`FILIO.chainto`, kept in the global zfilio_pFilio that the game's zlib gzio.c calls through) and opens files whose extension is
 * in its list (".gz", plus whatever the host adds with zfilio_Configure, e.g. SmushPlay's ".znm") with zlib's gz functions,
 * tracking a logical position because gz streams seek lazily.
 */
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <zlib/zlib.h>
#include "../../lec.h"
#include "../../filio.h"
#include "../../Memory/Memory.h"

#define ZFILIO_MAXEXTENSIONS 50

typedef struct ZfileHandle
{
    void* hFile;
    int isGz;
    int gzSize;
    int gzPos;
    int pos;
} ZfileHandle;

typedef struct ZfilioInterface
{
    ZfileHandle* (*Open)(const char* pPath, const char* pMode);
    void (*Close)(ZfileHandle* pFile);
    int  (*Read)(ZfileHandle* pFile, void* pBuf, int size);
    int  (*Write)(ZfileHandle* pFile, const void* pBuf, int size);
    int  (*Tell)(ZfileHandle* pFile);
    int  (*Seek)(ZfileHandle* pFile, int offset, int whence);
    void (*Flush)(ZfileHandle* pFile);
} ZfilioInterface;

int stdUtil_StrCmp(const char* pStr1, const char* pStr2);

int zfilio_Register(LecShare* pShare, void* pParam);
int zfilio_Configure(int tag, ...);
static void zfilio_Unregister(void);
static int zfilio_Startup(void);
static void zfilio_Shutdown(void);
static ZfileHandle* zfilio_open(const char* pPath, const char* pMode);
static void zfilio_close(ZfileHandle* pFile);
static int zfilio_read(ZfileHandle* pFile, void* pBuf, int size);
static int zfilio_write(ZfileHandle* pFile, const void* pBuf, int size);
static int zfilio_tell(ZfileHandle* pFile);
static int zfilio_seek(ZfileHandle* pFile, int offset, int whence);
static void zfilio_flush(ZfileHandle* pFile);

static int zfilio_extCount = 1;
static const char* zfilio_extensions[ZFILIO_MAXEXTENSIONS] = { ".gz" };
static int zfilio_flags = 1;
static LecModule zfilio_module = { zfilio_Register, zfilio_Unregister, zfilio_Startup, zfilio_Shutdown };
static ZfilioInterface zfilio_interface = { zfilio_open, zfilio_close, zfilio_read, zfilio_write, zfilio_tell, zfilio_seek, zfilio_flush };

FilioInterface* zfilio_pFilio;
static MemoryDebugInterface* zfilio_pMemory;
static LecShare* zfilio_pServices;
static LecDebug* zfilio_pDebug;
static void* zfilio_pParam;

int zfilio_Register(LecShare* pShare, void* pParam)
{
    zfilio_pServices = pShare;
    zfilio_pParam = pParam;
    pShare->Publish("LECMODULE=insane_zfilio", &zfilio_module);
    zfilio_pServices->Publish("FILIO=insane_zfilio", &zfilio_interface);
    return 0;
}

int zfilio_Configure(int tag, ...)
{
    va_list args;

    va_start(args, tag);
    while (tag) {
        switch (tag) {
            case 1:
                if (zfilio_extCount < ZFILIO_MAXEXTENSIONS) {
                    zfilio_extensions[zfilio_extCount++] = va_arg(args, const char*);
                }
                else if (zfilio_pDebug) {
                    zfilio_pDebug->Print("Too many zfilio extension set");
                }
                break;
            default:
                return -1;
        }
        tag = va_arg(args, int);
    }
    return 0;
}

static void zfilio_Unregister(void)
{
    zfilio_pServices->Unpublish("FILIO=insane_zfilio");
    zfilio_pServices->Unpublish("LECMODULE=insane_zfilio");
}

static int zfilio_Startup(void)
{
    zfilio_pFilio = (FilioInterface*)zfilio_pServices->Subscribe("FILIO.chainto", zfilio_pParam);
    zfilio_pMemory = (MemoryDebugInterface*)zfilio_pServices->Subscribe("MEMORY.buffers", zfilio_pParam);
    zfilio_pDebug = (LecDebug*)zfilio_pServices->Subscribe("VIRTHERC.debug", zfilio_pParam);
    if (!zfilio_pFilio) {
        zfilio_pServices->ReportError("Zfile Module: Cannot allocate filio services.");
        return -1;
    }
    if (!zfilio_pMemory) {
        zfilio_pServices->ReportError("Zfile Module: Cannot allocate memory services.");
        return -1;
    }
    return 0;
}

static void zfilio_Shutdown(void)
{
    zfilio_pServices->Unsubscribe(zfilio_pFilio);
    zfilio_pServices->Unsubscribe(zfilio_pMemory);
    if (zfilio_pDebug) {
        zfilio_pServices->Unsubscribe(zfilio_pDebug);
    }
}

static ZfileHandle* zfilio_open(const char* pPath, const char* pMode)
{
    ZfileHandle* pFile;
    const char* pExt;
    int size;
    int fh;
    int i;

#line 211
    pFile = zfilio_pMemory->Alloc(sizeof(ZfileHandle), INSANE_FILE, __LINE__);
    if (!pFile) {
        if (zfilio_pDebug && (zfilio_flags & 1)) {
            zfilio_pDebug->Print("Zfile Memory Alloc fail");
        }
        return NULL;
    }
    pFile->hFile = NULL;
    pFile->isGz = 0;
    if ((*pMode == 'r' || *pMode == 'R') && pMode[1] != '+' && (pMode[1] == 0 || pMode[2] != '+')) {
        for (i = 0; i < zfilio_extCount; ++i) {
            pExt = strrchr(pPath, '.');
            if (pExt && stdUtil_StrCmp(pExt, zfilio_extensions[i]) == 0) {
                pFile->hFile = gzopen(pPath, pMode);
                if (pFile->hFile) {
                    pFile->isGz = 1;
                    pFile->gzPos = 0;
                    pFile->pos = 0;
                    fh = zfilio_pFilio->Open(pPath, pMode);
                    zfilio_pFilio->Seek(fh, -4, 2);
                    zfilio_pFilio->Read(fh, &size, 4);
                    pFile->gzSize = size;
                    zfilio_pFilio->Close(fh);
                    return pFile;
                }
                break;
            }
        }
    }
    pFile->hFile = (void*)zfilio_pFilio->Open(pPath, pMode);
    if (pFile->hFile) {
        return pFile;
    }
#line 266
    zfilio_pMemory->Free(pFile, INSANE_FILE, __LINE__);
    return NULL;
}

static void zfilio_close(ZfileHandle* pFile)
{
    if (pFile->hFile) {
        if (pFile->isGz) {
            gzclose(pFile->hFile);
        }
        else {
            zfilio_pFilio->Close((int)pFile->hFile);
        }
    }
    if (pFile) {
#line 292
        zfilio_pMemory->Free(pFile, INSANE_FILE, __LINE__);
    }
}

static int zfilio_read(ZfileHandle* pFile, void* pBuf, int size)
{
    int rc = 0;

    if (pFile->hFile) {
        if (pFile->isGz) {
            if (pFile->gzPos != pFile->pos) {
                gzseek(pFile->hFile, pFile->pos, 0);
                pFile->gzPos = pFile->pos;
            }
            rc = gzread(pFile->hFile, pBuf, size);
            pFile->gzPos += size;
            pFile->pos += size;
            return rc;
        }
        rc = zfilio_pFilio->Read((int)pFile->hFile, pBuf, size);
    }
    return rc;
}

static int zfilio_write(ZfileHandle* pFile, const void* pBuf, int size)
{
    int rc = 0;

    if (pFile->hFile) {
        if (pFile->isGz) {
            rc = gzwrite(pFile->hFile, (void*)pBuf, size);
            pFile->gzPos += size;
            pFile->pos += size;
            return rc;
        }
        rc = zfilio_pFilio->Write((int)pFile->hFile, pBuf, size);
    }
    return rc;
}

static int zfilio_tell(ZfileHandle* pFile)
{
    int pos = 0;

    if (pFile->hFile) {
        if (pFile->isGz) {
            return pFile->pos;
        }
        pos = zfilio_pFilio->Tell((int)pFile->hFile);
    }
    return pos;
}

static int zfilio_seek(ZfileHandle* pFile, int offset, int whence)
{
    if (pFile->hFile) {
        if (pFile->isGz) {
            switch (whence) {
                case 0:
                    pFile->pos = offset;
                    break;
                case 1:
                    pFile->pos += offset;
                    break;
                case 2:
                    pFile->pos = pFile->gzSize + offset;
                    break;
            }
        }
        else {
            return zfilio_pFilio->Seek((int)pFile->hFile, offset, whence);
        }
    }
}

static void zfilio_flush(ZfileHandle* pFile)
{
    if (pFile->hFile) {
        if (pFile->isGz) {
            gzflush(pFile->hFile, 4);
        }
        else {
            zfilio_pFilio->Flush((int)pFile->hFile);
        }
    }
}
