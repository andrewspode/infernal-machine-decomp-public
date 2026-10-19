#ifndef SMUSH_INSANE_FILIO_H
#define SMUSH_INSANE_FILIO_H

/*
 * INSANE's file-I/O interface (`FILIO=<module>`): seven entry points, implemented by insane_stdio (C stdio),
 * insane_zfilio (its `.znm` layer) and reached through insane_stdfile (`FILE=insane_stdfile`), which adds the
 * derived operations. The handle is an opaque 32-bit value (retail's getc/putc take the address of the handle
 * parameter itself as the byte buffer, and mask the result with 0xFF).
 */
typedef struct FilioInterface
{
    int  (*Open)(const char* pPath, const char* pMode);
    void (*Close)(int fh);
    int  (*Read)(int fh, void* pBuf, int size);
    int  (*Write)(int fh, const void* pBuf, int size);
    int  (*Tell)(int fh);
    int  (*Seek)(int fh, int offset, int whence);
    void (*Flush)(int fh);
} FilioInterface;

#endif
