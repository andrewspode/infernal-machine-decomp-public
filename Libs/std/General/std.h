#ifndef STD_STD_H
#define STD_STD_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START



#define STDLOG_FATAL(message) \
    J3DLOG_FATAL(std_pHS, message)

#define STD_ASSERT(condition) \
    J3D_ASSERT(condition, std_pHS)

#define STD_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, std_pHS)

// Inclusive range test; the asserts that use it print this expansion.
#define STD_INRANGE(x, lo, hi) (((x)>=(lo))?(((x)<=(hi))?1:0):0)


extern char std_genBuffer[2048];

extern tHostServices* std_pHS;

void J3DAPI stdStartup(tHostServices* pHS);
void J3DAPI stdShutdown(void);
int J3DAPI stdPrintf(tPrintfFunc fpPrint, const char* pFilePath, unsigned int linenum, const char* format, ...);
int J3DAPI stdConsolePrintf(const char* pFormat, ...);
const char* J3DAPI stdFileFromPath(const char* pPath);
int J3DAPI stdCalcBitPos(int);
tFileHandle J3DAPI stdFileOpen(const char* pFilename, const char* mode);
int J3DAPI stdFileClose(tFileHandle fh);
size_t J3DAPI stdFileRead(tFileHandle fh, void* pOutData, size_t size);
size_t J3DAPI stdFileWrite(tFileHandle fh, const void* pData, size_t size);
char* J3DAPI stdFileGets(tFileHandle fh, char* pStr, size_t size);
wchar_t* J3DAPI stdFileGetws(tFileHandle fh, wchar_t* pOutStr, size_t size);
int J3DAPI stdFileEof(tFileHandle fh);
int J3DAPI stdFileTell(tFileHandle fh);
size_t J3DAPI stdFileSize(const char* pFilePath);
int J3DAPI stdFilePrintf(tFileHandle fh, const char* pFormat, ...);
int J3DAPI stdFileSeek(tFileHandle pFile, int offset, int origin);

J3D_EXTERN_C_END
#endif // STD_STD_H
