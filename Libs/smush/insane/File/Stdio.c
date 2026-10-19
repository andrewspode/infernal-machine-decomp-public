/*
 * insane_stdio: INSANE's `FILIO=insane_stdio` file-I/O module, a thin layer over the C runtime's stdio
 * (retail 0x004EA020-0x004EA0F0). Its registration, open, close and flush wrappers are here; its read, write,
 * tell and seek wrappers are byte-identical to the game's std file wrappers, which LINK folded together
 *
 */
#include <stdio.h>
#include "../lec.h"
#include "../filio.h"

int stdio_Register(LecShare* pShare, void* pParam);
static void stdio_Unregister(void);
static int stdio_Startup(void);
static void stdio_Shutdown(void);
static int stdio_open(const char* pPath, const char* pMode);
static void stdio_close(int fh);
static int stdio_read(int fh, void* pBuf, int size);
static int stdio_write(int fh, const void* pBuf, int size);
static int stdio_tell(int fh);
static int stdio_seek(int fh, int offset, int whence);
static void stdio_flush(int fh);

static LecModule stdio_module = { stdio_Register, stdio_Unregister, stdio_Startup, stdio_Shutdown };
static FilioInterface stdio_filio = { stdio_open, stdio_close, stdio_read, stdio_write, stdio_tell, stdio_seek, stdio_flush };

static void* stdio_pParam;
static LecShare* stdio_pShare;

int stdio_Register(LecShare* pShare, void* pParam)
{
    stdio_pShare = pShare;
    stdio_pParam = pParam;
    pShare->Publish("LECMODULE=insane_stdio", &stdio_module);
    stdio_pShare->Publish("FILIO=insane_stdio", &stdio_filio);
    return 0;
}

static void stdio_Unregister(void)
{
    stdio_pShare->Unpublish("FILIO=insane_stdio");
    stdio_pShare->Unpublish("LECMODULE=insane_stdio");
}

static int stdio_Startup(void)
{
    return 0;
}

static void stdio_Shutdown(void)
{
}

static int stdio_open(const char* pPath, const char* pMode)
{
    if (*pMode == 'w') {
        remove(pPath);
    }
    return (int)fopen(pPath, pMode);
}

static void stdio_close(int fh)
{
    fclose((FILE*)fh);
}

static int stdio_read(int fh, void* pBuf, int size)
{
    return (int)fread(pBuf, 1, size, (FILE*)fh);
}

static int stdio_write(int fh, const void* pBuf, int size)
{
    return (int)fwrite(pBuf, 1, size, (FILE*)fh);
}

static int stdio_tell(int fh)
{
    return ftell((FILE*)fh);
}

static int stdio_seek(int fh, int offset, int whence)
{
    return fseek((FILE*)fh, offset, whence);
}

static void stdio_flush(int fh)
{
    fflush((FILE*)fh);
}
