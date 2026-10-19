#ifndef STD_STDCONFFILE_H
#define STD_STDCONFFILE_H
#include <j3dcore/j3d.h>

#include <std/types.h>

#include <stdio.h>

J3D_EXTERN_C_START

#define STDCONFFILE_LINESIZE 4096u
#define STDCONFILE_MAXARGS   512u

typedef struct sStdConffileArg
{
    const char* argName;
    const char* argValue;
} StdConffileArg;

typedef struct sStdConffileEntry
{
    size_t numArgs;
    StdConffileArg aArgs[STDCONFILE_MAXARGS];
} StdConffileEntry;


extern StdConffileEntry stdConffile_entry;
extern char* stdConffile_aLine;

int J3DAPI stdConffile_Open(const char* pFilename);
int J3DAPI stdConffile_OpenWrite(const char* pFilename);
int J3DAPI stdConffile_OpenMode(const char* pFilename, const char* openMode);

void J3DAPI stdConffile_Close(void);
void J3DAPI stdConffile_CloseWrite(void);

int J3DAPI stdConffile_WriteLine(const char* pLine);
int J3DAPI stdConffile_Write(const void* pData, size_t size);
int J3DAPI stdConffile_Printf(const char* pFormat, ...);

int J3DAPI stdConffile_Read(void* pData, size_t size);
int J3DAPI stdConffile_ReadArgsFromStr(char* pStr);
int J3DAPI stdConffile_ReadArgs(void);
int J3DAPI stdConffile_ReadLine(void);

tFileHandle J3DAPI stdConffile_GetFileHandle(void);
size_t J3DAPI stdConffile_GetLineNumber(void);

const char* J3DAPI stdConffile_GetFilename(void);

J3D_EXTERN_C_END
#endif // STD_STDCONFFILE_H
