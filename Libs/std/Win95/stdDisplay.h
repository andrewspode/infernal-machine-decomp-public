#ifndef STD_STDDISPLAY_H
#define STD_STDDISPLAY_H
#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

extern tVBuffer stdDisplay_frontBuffer;

extern tVBuffer stdDisplay_backBuffer;

extern StdDisplayDevice stdDisplay_aDisplayDevices[16];

extern int stdDisplay_bOpen;

extern int stdDisplay_bModeSet;

extern StdDisplayDevice* stdDisplay_pCurDevice;

extern size_t stdDisplay_numVideoModes;

extern StdVideoMode* stdDisplay_pCurVideoMode;

int J3DAPI stdDisplay_Startup(void);
void J3DAPI stdDisplay_Shutdown(void);

int J3DAPI stdDisplay_Open(size_t deviceNum);
void J3DAPI stdDisplay_Close(void);

LPDIRECTDRAW4 stdDisplay_GetDirectDraw(void); // Can be retrieved after display system is opened
int J3DAPI stdDisplay_CreateZBuffer(LPDDPIXELFORMAT pPixelFormat, int bSystemMemory);


int stdDisplay_Update(void); // Flips back & front buffers
void J3DAPI stdDisplay_Refresh(int bReload); // If called with bReload = true it will re-set the current video mode

size_t J3DAPI stdDisplay_GetNumDevices(void);
int J3DAPI stdDisplay_GetDevice(size_t deviceNum, StdDisplayDevice* pDest); // Copies display  device at deviceNum to pDest
int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice); // Copies current display device to pDest

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers); // Sets current video mode
size_t J3DAPI stdDisplay_FindClosestMode(const StdVideoMode* pMatch);
void stdDisplay_ClearMode(void); // Clears current video mode

size_t J3DAPI stdDisplay_GetNumVideoModes(void);
int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode); // Copies video mode at modeNum to pDestMode
int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode);  // Copies current video mode to pDestMode

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory);
void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer);
int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer);
int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer);
int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t dwFillColor, const StdRect* pRect);
int J3DAPI stdDisplay_ClearRect(tVBuffer* pVBuffer, int fillColor, const StdRect* pRect);
int J3DAPI stdDisplay_VBufferCopy(tVBuffer* pDest, tVBuffer* pSrc, unsigned int blitX, int blitY, const StdRect* pRect, int bAlpha);
int J3DAPI stdDisplay_VBufferSetColorKey(tVBuffer* pVBuffer, int color);
tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, uint32_t colorKey);

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree);
int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree);

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight);
int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* lpRect);
int J3DAPI stdDisplay_SaveScreen(const char* pFilename);

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height);

HDC J3DAPI stdDisplay_GetFrontBufferDC(void);
void J3DAPI stdDisplay_ReleaseFrontBufferDC(HDC hdc);
HDC J3DAPI stdDisplay_GetBackBufferDC(void);
void J3DAPI stdDisplay_ReleaseBackBufferDC(HDC hdc);
int J3DAPI stdDisplay_FlipToGDISurface(void);

int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer);
HRESULT J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer);

int J3DAPI stdDisplay_CanRenderWindowed(void);
int J3DAPI stdDisplay_IsFullscreen(void);

int J3DAPI stdDisplay_LockBackBuffer(void** ppSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch);
void J3DAPI stdDisplay_UnlockBackBuffer(void);

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel);

J3D_EXTERN_C_END
#endif // STD_STDDISPLAY_H
