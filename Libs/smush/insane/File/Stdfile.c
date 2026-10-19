/*
 * insane_stdfile: INSANE's `FILE=insane_stdfile` service (retail 0x004E9810-0x004E9B30). It subscribes to the next file
 * layer (`FILIO.chainto`) and forwards the seven basic operations to it, adding size, gets, eof, printf, getc and putc.
 *
 */
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "../lec.h"
#include "../filio.h"

typedef struct StdFileInterface
{
    int   (*Open)(const char* pPath, const char* pMode);
    void  (*Close)(int fh);
    int   (*Read)(int fh, void* pBuf, int size);
    int   (*Write)(int fh, const void* pBuf, int size);
    int   (*Tell)(int fh);
    int   (*Seek)(int fh, int offset, int whence);
    void  (*Flush)(int fh);
    int   (*Size)(int fh);
    char* (*Gets)(int fh, char* pBuf, int size);
    int   (*Eof)(int fh);
    int   (*Printf)(int fh, const char* pFormat, ...);
    int   (*Getc)(int fh);
    int   (*Putc)(int fh, int c);
} StdFileInterface;

int stdfile_Register(LecShare* pShare, void* pParam);
static void stdfile_Unregister(void);
static int stdfile_Startup(void);
static void stdfile_Shutdown(void);
static int stdfile_open(const char* pPath, const char* pMode);
static void stdfile_close(int fh);
static int stdfile_read(int fh, void* pBuf, int size);
static int stdfile_write(int fh, const void* pBuf, int size);
static int stdfile_tell(int fh);
static int stdfile_seek(int fh, int offset, int whence);
static void stdfile_flush(int fh);
static int stdfile_size(int fh);
static char* stdfile_gets(int fh, char* pBuf, int size);
static int stdfile_eof(int fh);
static int stdfile_printf(int fh, const char* pFormat, ...);
static int stdfile_getc(int fh);
static int stdfile_putc(int fh, int c);

static LecModule stdfile_module = { stdfile_Register, stdfile_Unregister, stdfile_Startup, stdfile_Shutdown };
static StdFileInterface stdfile_interface = {
    stdfile_open, stdfile_close, stdfile_read, stdfile_write, stdfile_tell, stdfile_seek, stdfile_flush,
    stdfile_size, stdfile_gets, stdfile_eof, stdfile_printf, stdfile_getc, stdfile_putc
};

static LecShare* stdfile_pShare;
static FilioInterface* stdfile_pChained;
static void* stdfile_pParam;

int stdfile_Register(LecShare* pShare, void* pParam)
{
    stdfile_pShare = pShare;
    stdfile_pParam = pParam;
    pShare->Publish("LECMODULE=insane_stdfile", &stdfile_module);
    stdfile_pShare->Publish("FILE=insane_stdfile", &stdfile_interface);
    return 0;
}

static void stdfile_Unregister(void)
{
    stdfile_pShare->Unpublish("FILE=insane_stdfile");
    stdfile_pShare->Unpublish("LECMODULE=insane_stdfile");
}

static int stdfile_Startup(void)
{
    stdfile_pChained = (FilioInterface*)stdfile_pShare->Subscribe("FILIO.chainto", stdfile_pParam);
    if (!stdfile_pChained) {
        stdfile_pShare->ReportError("StdFile Module: Cannot allocate filio services.");
        return -1;
    }
    return 0;
}

static void stdfile_Shutdown(void)
{
    stdfile_pShare->Unsubscribe(stdfile_pChained);
}

static int stdfile_open(const char* pPath, const char* pMode)
{
    return stdfile_pChained->Open(pPath, pMode);
}

static void stdfile_close(int fh)
{
    stdfile_pChained->Close(fh);
}

static int stdfile_read(int fh, void* pBuf, int size)
{
    return stdfile_pChained->Read(fh, pBuf, size);
}

static int stdfile_write(int fh, const void* pBuf, int size)
{
    return stdfile_pChained->Write(fh, pBuf, size);
}

static int stdfile_tell(int fh)
{
    return stdfile_pChained->Tell(fh);
}

static int stdfile_seek(int fh, int offset, int whence)
{
    return stdfile_pChained->Seek(fh, offset, whence);
}

static void stdfile_flush(int fh)
{
    stdfile_pChained->Flush(fh);
}

static int stdfile_size(int fh)
{
    int pos = stdfile_pChained->Tell(fh);
    int size;
    stdfile_pChained->Seek(fh, 0, 2);
    size = stdfile_pChained->Tell(fh);
    stdfile_pChained->Seek(fh, pos, 0);
    return size;
}

static char* stdfile_gets(int fh, char* pBuf, int size)
{
    int i;
    int c;
    for (i = 0; i < size; ++i) {
        c = stdfile_getc(fh);
        if (c == -1) {
            if (i == 0) {
                return NULL;
            }
            pBuf[i] = 0;
            break;
        }
        pBuf[i] = (char)c;
        if (c == '\n' && i + 1 < size) {
            pBuf[i + 1] = 0;
            break;
        }
    }
    return pBuf;
}

static int stdfile_eof(int fh)
{
    return stdfile_tell(fh) >= stdfile_size(fh);
}

static int stdfile_printf(int fh, const char* pFormat, ...)
{
    char buf[500];
    va_list args;
    va_start(args, pFormat);
    vsprintf(buf, pFormat, args);
    return stdfile_pChained->Write(fh, buf, strlen(buf));
}

static int stdfile_getc(int fh)
{
    if (stdfile_pChained->Read(fh, &fh, 1)) {
        return fh & 0xFF;
    }
    return -1;
}

static int stdfile_putc(int fh, int c)
{
    if (stdfile_pChained->Write(fh, &c, 1)) {
        return c & 0xFF;
    }
    return -1;
}
