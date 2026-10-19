#ifndef STD_TYPES_H
#define STD_TYPES_H
#include <j3dcore/types.h>
#include <d3d.h>
#include <ddraw.h>
#include <dinput.h>
#include <dplay.h>
#include <j3dcore/j3d.h>

J3D_EXTERN_C_START

typedef enum eStdColorFormatType
{
    STDCOLOR_FORMAT_RGB             = 0,
    STDCOLOR_FORMAT_RGBA_1BIT_ALPHA = 1,
    STDCOLOR_FORMAT_RGBA            = 2,
} StdColorFormatType;

typedef enum eColorMode J3D_ENUM_TYPE(int32_t)
{
    STDCOLOR_PAL  = 0,
        STDCOLOR_RGB  = 1,
        STDCOLOR_RGBA = 2,
} tColorMode;

typedef enum eStd3DMipmapFilterType
{
    STD3D_MIPMAPFILTER_NONE      = 0,
    STD3D_MIPMAPFILTER_BILINEAR  = 1,
    STD3D_MIPMAPFILTER_TRILINEAR = 2,
} Std3DMipmapFilterType;

typedef enum eStdControlAxisFlag
{
    STDCONTROL_AXIS_REGISTERED  = 0x1,
    STDCONTROL_AXIS_ENABLED     = 0x2,
    STDCONTROL_AXIS_HASDEADZONE = 0x8,
    STDCONTROL_AXIS_POSITIVE    = 0x10,
    STDCONTROL_AXIS_NEGATIVE    = 0x20,
    STDCONTROL_AXIS_GAMEPAD     = 0x80,
} StdControlAxisFlag;

typedef enum eStd3DRenderState
{
    STD3D_RS_UNKNOWN_1             = 0x1,
    STD3D_RS_UNKNOWN_2             = 0x2,
    STD3D_RS_ALPHAREF_SET          = 0x4,
    STD3D_RS_SUBPIXEL_CORRECTION   = 0x10,
    STD3D_RS_TEXFILTER_BILINEAR    = 0x80,
    STD3D_RS_UNKNOWN_200           = 0x200,
    STD3D_RS_UNKNOWN_400           = 0x400,
    STD3D_RS_TEX_CPAMP_U           = 0x800,
    STD3D_RS_TEX_CPAMP_V           = 0x1000,
    STD3D_RS_ZWRITE_DISABLED       = 0x2000,
    STD3D_RS_FOG_ENABLED           = 0x8000
} Std3DRenderState;

typedef uintptr_t tFileHandle;

typedef struct sLinkListNode tLinkListNode;
typedef struct sMemoryHeader tMemoryHeader;
typedef struct sSystemTexture tSystemTexture;
typedef struct sGob Gob;

typedef unsigned int (J3DAPI* tHashFunc)(const char*, signed int);
typedef int (J3DAPI* tPrintfFunc)(const char* pFormat, ...);
typedef void (J3DAPI* tAssertFunc)(const char*, const char*, int);
typedef int (J3DAPI* tAtExitFunc)(void (*func)(void));
typedef void* (J3DAPI* tMallocFunc)(size_t size);
typedef void (J3DAPI* tFreeFunc)(void* pData);
typedef void* (J3DAPI* tReallocFunc)(void* pData, size_t newSize);
typedef unsigned int (J3DAPI* tGetTimeMsecFunc)();
typedef tFileHandle(J3DAPI* tFileOpenFunc)(const char* pFilename, const char* mode);
typedef int (J3DAPI* tFileCloseFunc)(tFileHandle fh);
typedef size_t(J3DAPI* tFileReadFunc)(tFileHandle, void*, size_t);
typedef char* (J3DAPI* tFileGetsFunc)(tFileHandle fh, char* pStr, size_t size);
typedef size_t(J3DAPI* tFileWriteFunc)(tFileHandle fh, const void* pData, size_t size);
typedef int (J3DAPI* tFileEOFFunc)(tFileHandle fh);
typedef int (J3DAPI* tFileTellFunc)(tFileHandle fh);
typedef int (J3DAPI* tFileSeekFunc)(tFileHandle fh, int offset, int origin);
typedef size_t(J3DAPI* tFileSizeFunc)(const char* pFilename);
typedef int (*tFilePrintfFunc)(tFileHandle fh, const char* pFormat, ...);
typedef wchar_t* (J3DAPI* tFileGetwsFunc)(tFileHandle fh, wchar_t* pOutStr, size_t size);
typedef void* (J3DAPI* tAllocHandleFunc)(size_t);
typedef void (J3DAPI* tFreeHandleFunc)(void*);
typedef void* (J3DAPI* tReallocHandleFunc)(void* ptr, size_t newSize);
typedef int (J3DAPI* tLockHandleFunc)(int);
typedef void (J3DAPI* tUnlockHandleFunc)();

typedef struct sStdRect
{
    int left;
    int top;
    int right;
    int bottom;
} StdRect;

typedef struct sGobFileHeader
{
    char signature[4];
    int version;
    int dirOffset;
} tGobFileHeader;

typedef struct sGobFileEntry
{
    int offset;
    int size;
    char aName[128];
} tGobFileEntry;

typedef struct sGobFileDirectory
{
    uint32_t numEntries;
    tGobFileEntry* aEntries;
} GobFileDirectory;

struct sLinkListNode
{
    tLinkListNode* prev;
    tLinkListNode* next;
    const char* name;
    void* data;
};

typedef struct sHashTable
{
    int numNodes; // int: the debug build loads it with `fild dword` and compares it signed
    tLinkListNode* paNodes;
    tHashFunc pfHashFunc;
} tHashTable;

typedef struct sGobFileHandle
{
    int bUsed;
    Gob* pGob;
    tGobFileEntry* pEntry;
    int offset;
} GobFileHandle;

struct sGob
{
    char aFilePath[128];
    tFileHandle hGobFile;
    GobFileDirectory directory;
    tHashTable* pDirHash;
    int numHandles;
    GobFileHandle* aHandles;
    GobFileHandle* pCurHandle;
    int bFileMap;
    LPVOID pBase;
    HANDLE hFile;
    HANDLE hMapFile;
};

typedef struct sFindFileData
{
    float unknown;
    int nFoundFiles;
    char aSearchFilter[128];
    HANDLE handle;
} FindFileData;

typedef struct sHostServices
{
    float unknown1;
    tPrintfFunc pStatusPrint;
    tPrintfFunc pMessagePrint;
    tPrintfFunc pWarningPrint;
    tPrintfFunc pErrorPrint;
    tPrintfFunc pDebugPrint;
    tAssertFunc pAssert;
    tAtExitFunc pAtExit;
    tMallocFunc pMalloc;
    tFreeFunc pFree;
    tReallocFunc pRealloc;
    tGetTimeMsecFunc pGetTimeMsec;
    tFileOpenFunc pFileOpen;
    tFileCloseFunc pFileClose;
    tFileReadFunc pFileRead;
    tFileGetsFunc pFileGets;
    tFileWriteFunc pFileWrite;
    tFileEOFFunc pFileEOF;
    tFileTellFunc pFileTell;
    tFileSeekFunc pFileSeek;
    tFileSizeFunc pFileSize;
    tFilePrintfFunc pFilePrintf;
    tFileGetwsFunc pFileGetws;
    tAllocHandleFunc pAllocHandle;
    tFreeHandleFunc pFreeHandle;
    tReallocHandleFunc pReallocHandle;
    tLockHandleFunc pLockHandle;
    tUnlockHandleFunc pUnlockHandle;
} tHostServices;

struct sMemoryHeader
{
    int number;
    uint32_t id;
    size_t size;
    const char* pFilename;
    size_t line;
    tMemoryHeader* pPrev;
    tMemoryHeader* pNext;
    uint32_t magic;
};

struct sSystemTexture
{
    DDSURFACEDESC2 ddsd;
    LPDIRECT3DTEXTURE2 pD3DSrcTexture;
    LPDIRECT3DTEXTURE2 pD3DCachedTex;
    size_t textureSize;
    size_t frameNum;
    tSystemTexture* pPrevCachedTexture;
    tSystemTexture* pNextCachedTexture;
};

typedef struct sColorInfo
{
    tColorMode colorMode;
    uint32_t bpp;
    uint32_t redBPP;
    uint32_t greenBPP;
    uint32_t blueBPP;
    int32_t redPosShift;
    int32_t greenPosShift;
    int32_t bluePosShift;
    int32_t redPosShiftRight;
    int32_t greenPosShiftRight;
    int32_t bluePosShiftRight;
    uint32_t alphaBPP;
    int32_t alphaPosShift;
    int32_t alphaPosShiftRight;
} ColorInfo;

typedef struct sFoundFileInfo
{
    char aName[260];
    int bIsDirectory;
    uint32_t lastChanged;
} tFoundFileInfo;

typedef struct sStringTableNode
{
    const char* pKey;
    wchar_t* value;
    int unknown;
} tStringTableNode;

typedef struct sStringTable
{
    uint32_t nMsgs;
    tStringTableNode* pData;
    tHashTable* pHashtbl;
    int magic;
} tStringTable;

typedef struct sRasterInfo
{
    uint32_t width;
    uint32_t height;
    size_t size;
    size_t rowSize;
    size_t rowWidth;
    ColorInfo colorInfo;
} tRasterInfo;

typedef struct sVSurface
{
    LPDIRECTDRAWSURFACE4 pDDSurf;
    DDSURFACEDESC2 ddSurfDesc;
} tVSurface;

typedef struct sVBuffer
{
    size_t lockRefCount;
    size_t lockSurfRefCount;
    int bVideoMemory;
    tRasterInfo rasterInfo;
    uint8_t* pPixels;
    int colorKey;
    tVSurface surface;
} tVBuffer;

typedef struct sCircularBuffer
{
    int numAllocated;
    uint8_t* paElements;
    int iFirst;
    int numValidElements;
    int elementSize;
} tCircularBuffer;

typedef struct sStdDisplayDevice
{
    char aDeviceName[128];
    char aDriverName[128];
    int bHAL;
    int bGuidNotSet;
    int bWindowRenderNotSupported;
    size_t totalVideoMemory;
    size_t freeVideoMemory;
    DDCAPS_DX6 ddcaps;
    GUID guid;
} StdDisplayDevice;

typedef struct sStdVideoMode
{
    float aspectRatio;
    tRasterInfo rasterInfo;
} StdVideoMode;

typedef struct sDevice3DCaps
{
    int bHAL;
    int bTexturePerspectiveSupported;
    int hasZBuffer;
    int bColorkeyTextureSupported;
    int bAlphaTextureSupported;
    int bStippledShadeSupported;
    int bAlphaBlendSupported;
    int bSqareOnlyTexture;
    size_t minTexWidth;
    size_t minTexHeight;
    size_t maxTexWidth;
    size_t maxTexHeight;
    size_t maxVertexCount;
} Device3DCaps;

typedef struct sDevice3D
{
    Device3DCaps caps;
    char deviceName[128];
    char deviceDescription[128];
    size_t totalMemory;
    size_t availableMemory;
    D3DDEVICEDESC d3dDesc;
    GUID duid;
    int unknown146;
    int unknown147;
    int unknown148;
    int unknown149;
    int unknown150;
    int unknown151;
    int unknown152;
    int unknown153;
    int unknown154;
    int unknown155;
    int unknown156;
    int unknown157;
    int unknown158;
    int unknown159;
    int unknown160;
    int unknown161;
    int unknown162;
    int unknown163;
    int unknown164;
    int unknown165;
    int unknown166;
    int unknown167;
    int unknown168;
    int unknown169;
    int unknown170;
    int unknown171;
    int unknown172;
    int unknown173;
    int unknown174;
    int unknown175;
    int unknown176;
    int unknown177;
    int unknown178;
    int unknown179;
    int unknown180;
    int unknown181;
    int unknown182;
    int unknown183;
    int unknown184;
    int unknown185;
    int unknown186;
    int unknown187;
    int unknown188;
    int unknown189;
    int unknown190;
    int unknown191;
    int unknown192;
    int unknown193;
    int unknown194;
    int unknown195;
    int unknown196;
    int unknown197;
    int unknown198;
    int unknown199;
    int unknown200;
    int unknown201;
    int unknown202;
    int unknown203;
    int unknown204;
    int unknown205;
    int unknown206;
    int unknown207;
    int unknown208;
    int unknown209;
    int unknown210;
    int unknown211;
    int unknown212;
    int unknown213;
    int unknown214;
    int unknown215;
    int unknown216;
    int unknown217;
} Device3D;

typedef struct sDXStatus
{
    HRESULT code;
    const char* text;
} DXStatus;

typedef struct sStdDisplayInfo
{
    StdDisplayDevice displayDevice;
    size_t numModes;
    StdVideoMode* aModes;
    size_t numDevices;
    Device3D* aDevices;
} StdDisplayInfo;

typedef struct sStdTextureFormat
{
    ColorInfo ci;
    int bColorKey;
    uint32_t colorKey;
    DDPIXELFORMAT ddPixelFmt;
} StdTextureFormat;

typedef struct sStdDisplayEnvironment
{
    size_t numInfos;
    StdDisplayInfo* aDisplayInfos;
} StdDisplayEnvironment;

typedef struct sStdInputDevice
{
    LPDIRECTINPUTDEVICEA pDIDevice;
    DIDEVCAPS diDevCaps;
} StdInputDevice;

typedef struct sStdControlAxis
{
    StdControlAxisFlag flags;
    int min;
    int max;
    int center;
    int deadzoneThreshold;
    float scale;
} StdControlAxis;

typedef struct sStdControlJoystickDevice
{
    DIDEVICEINSTANCEA dinstance;
    LPDIRECTINPUTDEVICE2A pDIDevice;
    DIDEVCAPS caps;
} StdControlJoystickDevice;

typedef struct sStdFadeFactor
{
    int bEnabled;
    float factor;
} tStdFadeFactor;

typedef struct sMemoryHeap
{
    tMemoryHeader header;
    void* pMemory;
} tMemoryHeap;

typedef struct sMemoryState
{
    size_t totalBytes;
    size_t totalAllocs;
    size_t maxBytes;
    tMemoryHeader header;
} tMemoryState;

typedef struct sMemoryBlockHeader
{
    size_t size;
    void* pHeap;
} tMemoryBlockHeader;

typedef struct sMemoryZoneHeader
{
    uint16_t size;
    uint16_t prevSize;
    struct sMemoryBlock* pZone;
} tMemoryZoneHeader;

typedef struct sMemoryBlock
{
    tMemoryZoneHeader* pFirst;
    tMemoryZoneHeader* pLargest;
    size_t largestSize;
    int bAlloced;
} tMemoryBlock;

typedef struct sStdCommPlayerInfo
{
    wchar_t aName[20];
    DPID id;
} StdCommPlayerInfo;

typedef struct sStdCommSessionSettings
{
    GUID guid;
    int maxPlayers;
    int numCurrentPlayers;
    wchar_t aSessionName[64];
    char aSomething[64];
    wchar_t aPassword[64];
    uint32_t opt1;
    uint32_t opt2;
    uint32_t opt3;
} StdCommGame;

J3D_EXTERN_C_END
#endif //STD_TYPES_H
