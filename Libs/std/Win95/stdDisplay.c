#include "stdDisplay.h"
#include "stdWin95.h"

#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdColorApi.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

static int stdDisplay_coopLevelFlags = DDSCL_NORMAL;

static HFONT stdDisplay_hFont;

tVBuffer stdDisplay_frontBuffer = { 0 };

static size_t stdDisplay_curDevice = 0;

static StdVideoMode stdDisplay_primaryVideoMode = { 0 };

static int stdDisplay_dword_5D73D8  = 0;
static int stdDisplay_dword_5D73DC  = 0;
static int stdDisplay_backbufWidth  = 0;
static int stdDisplay_backbufHeight = 0;

static tVSurface stdDisplay_zBuffer = { 0 };

static StdVideoMode stdDisplay_aVideoModes[64] = { 0 };

StdDisplayDevice stdDisplay_aDisplayDevices[16] = { 0 };

static int stdDisplay_bStartup = 0;
int stdDisplay_bOpen    = 0;
int stdDisplay_bModeSet = 0;

static size_t stdDisplay_numDevices            = 0;
StdDisplayDevice* stdDisplay_pCurDevice = NULL;

size_t stdDisplay_numVideoModes        = 0;
StdVideoMode* stdDisplay_pCurVideoMode = NULL;

static LPDIRECTDRAW4 stdDisplay_lpDD = NULL;

static int stdDisplay_bFullscreen = 0;

tVBuffer stdDisplay_backBuffer;

static const DXStatus stdDisplay_aDDStatusTbl[111] =
{
    { DD_OK,                                      "DD_OK"                                    },
    { DDERR_ALREADYINITIALIZED,                   "DDERR_ALREADYINITIALIZED"                 },
    { DDERR_CANNOTATTACHSURFACE,                  "DDERR_CANNOTATTACHSURFACE"                },
    { DDERR_CANNOTDETACHSURFACE,                  "DDERR_CANNOTDETACHSURFACE"                },
    { DDERR_CURRENTLYNOTAVAIL,                    "DDERR_CURRENTLYNOTAVAIL"                  },
    { DDERR_EXCEPTION,                            "DDERR_EXCEPTION"                          },
    { DDERR_GENERIC,                              "DDERR_GENERIC"                            },
    { DDERR_HEIGHTALIGN,                          "DDERR_HEIGHTALIGN"                        },
    { DDERR_INCOMPATIBLEPRIMARY,                  "DDERR_INCOMPATIBLEPRIMARY"                },
    { DDERR_INVALIDCAPS,                          "DDERR_INVALIDCAPS"                        },
    { DDERR_INVALIDCLIPLIST,                      "DDERR_INVALIDCLIPLIST"                    },
    { DDERR_INVALIDMODE,                          "DDERR_INVALIDMODE"                        },
    { DDERR_INVALIDOBJECT,                        "DDERR_INVALIDOBJECT"                      },
    { DDERR_INVALIDPARAMS,                        "DDERR_INVALIDPARAMS"                      },
    { DDERR_INVALIDPIXELFORMAT,                   "DDERR_INVALIDPIXELFORMAT"                 },
    { DDERR_INVALIDRECT,                          "DDERR_INVALIDRECT"                        },
    { DDERR_LOCKEDSURFACES,                       "DDERR_LOCKEDSURFACES"                     },
    { DDERR_NO3D,                                 "DDERR_NO3D"                               },
    { DDERR_NOALPHAHW,                            "DDERR_NOALPHAHW"                          },
    { DDERR_NOCLIPLIST,                           "DDERR_NOCLIPLIST"                         },
    { DDERR_NOCOLORCONVHW,                        "DDERR_NOCOLORCONVHW"                      },
    { DDERR_NOCOOPERATIVELEVELSET,                "DDERR_NOCOOPERATIVELEVELSET"              },
    { DDERR_NOCOLORKEY,                           "DDERR_NOCOLORKEY"                         },
    { DDERR_NOCOLORKEYHW,                         "DDERR_NOCOLORKEYHW"                       },
    { DDERR_NODIRECTDRAWSUPPORT,                  "DDERR_NODIRECTDRAWSUPPORT"                },
    { DDERR_NOEXCLUSIVEMODE,                      "DDERR_NOEXCLUSIVEMODE"                    },
    { DDERR_NOFLIPHW,                             "DDERR_NOFLIPHW"                           },
    { DDERR_NOGDI,                                "DDERR_NOGDI"                              },
    { DDERR_NOMIRRORHW,                           "DDERR_NOMIRRORHW"                         },
    { DDERR_NOTFOUND,                             "DDERR_NOTFOUND"                           },
    { DDERR_NOOVERLAYHW,                          "DDERR_NOOVERLAYHW"                        },
    { DDERR_OVERLAPPINGRECTS,                     "DDERR_OVERLAPPINGRECTS"                   },
    { DDERR_NORASTEROPHW,                         "DDERR_NORASTEROPHW"                       },
    { DDERR_NOROTATIONHW,                         "DDERR_NOROTATIONHW"                       },
    { DDERR_NOSTRETCHHW,                          "DDERR_NOSTRETCHHW"                        },
    { DDERR_NOT4BITCOLOR,                         "DDERR_NOT4BITCOLOR"                       },
    { DDERR_NOT4BITCOLORINDEX,                    "DDERR_NOT4BITCOLORINDEX"                  },
    { DDERR_NOT8BITCOLOR,                         "DDERR_NOT8BITCOLOR"                       },
    { DDERR_NOTEXTUREHW,                          "DDERR_NOTEXTUREHW"                        },
    { DDERR_NOVSYNCHW,                            "DDERR_NOVSYNCHW"                          },
    { DDERR_NOZBUFFERHW,                          "DDERR_NOZBUFFERHW"                        },
    { DDERR_NOZOVERLAYHW,                         "DDERR_NOZOVERLAYHW"                       },
    { DDERR_OUTOFCAPS,                            "DDERR_OUTOFCAPS"                          },
    { DDERR_OUTOFMEMORY,                          "DDERR_OUTOFMEMORY"                        },
    { DDERR_OUTOFVIDEOMEMORY,                     "DDERR_OUTOFVIDEOMEMORY"                   },
    { DDERR_OVERLAYCANTCLIP,                      "DDERR_OVERLAYCANTCLIP"                    },
    { DDERR_OVERLAYCOLORKEYONLYONEACTIVE,         "DDERR_OVERLAYCOLORKEYONLYONEACTIVE"       },
    { DDERR_PALETTEBUSY,                          "DDERR_PALETTEBUSY"                        },
    { DDERR_COLORKEYNOTSET,                       "DDERR_COLORKEYNOTSET"                     },
    { DDERR_SURFACEALREADYATTACHED,               "DDERR_SURFACEALREADYATTACHED"             },
    { DDERR_SURFACEALREADYDEPENDENT,              "DDERR_SURFACEALREADYDEPENDENT"            },
    { DDERR_SURFACEBUSY,                          "DDERR_SURFACEBUSY"                        },
    { DDERR_CANTLOCKSURFACE,                      "DDERR_CANTLOCKSURFACE"                    },
    { DDERR_SURFACEISOBSCURED,                    "DDERR_SURFACEISOBSCURED"                  },
    { DDERR_SURFACELOST,                          "DDERR_SURFACELOST"                        },
    { DDERR_SURFACENOTATTACHED,                   "DDERR_SURFACENOTATTACHED"                 },
    { DDERR_TOOBIGHEIGHT,                         "DDERR_TOOBIGHEIGHT"                       },
    { DDERR_TOOBIGSIZE,                           "DDERR_TOOBIGSIZE"                         },
    { DDERR_TOOBIGWIDTH,                          "DDERR_TOOBIGWIDTH"                        },
    { DDERR_UNSUPPORTED,                          "DDERR_UNSUPPORTED"                        },
    { DDERR_UNSUPPORTEDFORMAT,                    "DDERR_UNSUPPORTEDFORMAT"                  },
    { DDERR_UNSUPPORTEDMASK,                      "DDERR_UNSUPPORTEDMASK"                    },
    { DDERR_INVALIDSTREAM,                        "DDERR_INVALIDSTREAM"                      },
    { DDERR_VERTICALBLANKINPROGRESS,              "DDERR_VERTICALBLANKINPROGRESS"            },
    { DDERR_WASSTILLDRAWING,                      "DDERR_WASSTILLDRAWING"                    },
    { DDERR_XALIGN,                               "DDERR_XALIGN"                             },
    { DDERR_INVALIDDIRECTDRAWGUID,                "DDERR_INVALIDDIRECTDRAWGUID"              },
    { DDERR_DIRECTDRAWALREADYCREATED,             "DDERR_DIRECTDRAWALREADYCREATED"           },
    { DDERR_NODIRECTDRAWHW,                       "DDERR_NODIRECTDRAWHW"                     },
    { DDERR_PRIMARYSURFACEALREADYEXISTS,          "DDERR_PRIMARYSURFACEALREADYEXISTS"        },
    { DDERR_NOEMULATION,                          "DDERR_NOEMULATION"                        },
    { DDERR_REGIONTOOSMALL,                       "DDERR_REGIONTOOSMALL"                     },
    { DDERR_CLIPPERISUSINGHWND,                   "DDERR_CLIPPERISUSINGHWND"                 },
    { DDERR_NOCLIPPERATTACHED,                    "DDERR_NOCLIPPERATTACHED"                  },
    { DDERR_NOHWND,                               "DDERR_NOHWND"                             },
    { DDERR_HWNDSUBCLASSED,                       "DDERR_HWNDSUBCLASSED"                     },
    { DDERR_HWNDALREADYSET,                       "DDERR_HWNDALREADYSET"                     },
    { DDERR_NOPALETTEATTACHED,                    "DDERR_NOPALETTEATTACHED"                  },
    { DDERR_NOPALETTEHW,                          "DDERR_NOPALETTEHW"                        },
    { DDERR_BLTFASTCANTCLIP,                      "DDERR_BLTFASTCANTCLIP"                    },
    { DDERR_NOBLTHW,                              "DDERR_NOBLTHW"                            },
    { DDERR_NODDROPSHW,                           "DDERR_NODDROPSHW"                         },
    { DDERR_OVERLAYNOTVISIBLE,                    "DDERR_OVERLAYNOTVISIBLE"                  },
    { DDERR_NOOVERLAYDEST,                        "DDERR_NOOVERLAYDEST"                      },
    { DDERR_INVALIDPOSITION,                      "DDERR_INVALIDPOSITION"                    },
    { DDERR_NOTAOVERLAYSURFACE,                   "DDERR_NOTAOVERLAYSURFACE"                 },
    { DDERR_EXCLUSIVEMODEALREADYSET,              "DDERR_EXCLUSIVEMODEALREADYSET"            },
    { DDERR_NOTFLIPPABLE,                         "DDERR_NOTFLIPPABLE"                       },
    { DDERR_CANTDUPLICATE,                        "DDERR_CANTDUPLICATE"                      },
    { DDERR_NOTLOCKED,                            "DDERR_NOTLOCKED"                          },
    { DDERR_CANTCREATEDC,                         "DDERR_CANTCREATEDC"                       },
    { DDERR_NODC,                                 "DDERR_NODC"                               },
    { DDERR_WRONGMODE,                            "DDERR_WRONGMODE"                          },
    { DDERR_IMPLICITLYCREATED,                    "DDERR_IMPLICITLYCREATED"                  },
    { DDERR_NOTPALETTIZED,                        "DDERR_NOTPALETTIZED"                      },
    { DDERR_UNSUPPORTEDMODE,                      "DDERR_UNSUPPORTEDMODE"                    },
    { DDERR_NOMIPMAPHW,                           "DDERR_NOMIPMAPHW"                         },
    { DDERR_INVALIDSURFACETYPE,                   "DDERR_INVALIDSURFACETYPE"                 },
    { DDERR_NOOPTIMIZEHW,                         "DDERR_NOOPTIMIZEHW"                       },
    { DDERR_NOTLOADED,                            "DDERR_NOTLOADED"                          },
    { DDERR_NOFOCUSWINDOW,                        "DDERR_NOFOCUSWINDOW"                      },
    { DDERR_DCALREADYCREATED,                     "DDERR_DCALREADYCREATED"                   },
    { DDERR_NONONLOCALVIDMEM,                     "DDERR_NONONLOCALVIDMEM"                   },
    { DDERR_CANTPAGELOCK,                         "DDERR_CANTPAGELOCK"                       },
    { DDERR_CANTPAGEUNLOCK,                       "DDERR_CANTPAGEUNLOCK"                     },
    { DDERR_NOTPAGELOCKED,                        "DDERR_NOTPAGELOCKED"                      },
    { DDERR_MOREDATA,                             "DDERR_MOREDATA"                           },
    { DDERR_EXPIRED,                              "DDERR_EXPIRED"                            },
    { DDERR_VIDEONOTACTIVE,                       "DDERR_VIDEONOTACTIVE"                     },
    { DDERR_DEVICEDOESNTOWNSURFACE,               "DDERR_DEVICEDOESNTOWNSURFACE"             },
    { DDERR_NOTINITIALIZED,                       "DDERR_NOTINITIALIZED"                     }
};

static int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2);
static const char* J3DAPI stdDisplay_DDGetStatus(HRESULT status);

static int J3DAPI stdDisplay_InitDirectDraw(HWND hwnd);
static void J3DAPI stdDisplay_ReleaseDirectDraw(void);
static BOOL PASCAL stdDisplay_DDEnumCallback(GUID* lpGUID, LPSTR szDriverName, LPSTR szDriverDescription, LPVOID lpContext);
static HRESULT PASCAL stdDisplay_EnumVideoModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);

static void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode);
static int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode);
static int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers); // numBackBuffers - Specifies the number of back buffers associated with the surface

static void J3DAPI stdDisplay_ReleaseBuffers(void);
static uint8_t* J3DAPI stdDisplay_LockSurface(tVSurface* pVSurf);
static int J3DAPI stdDisplay_UnlockSurface(tVSurface* pSurf);
static int J3DAPI stdDisplay_ColorFillSurface(tVSurface* pSurf, uint32_t dwFillColor, const StdRect* lpRect);

// What FindClosestDevice compares: the leading three flags of a StdDisplayDevice's tail (debug build 0x004F2F37).
typedef struct
{
    int bHAL;
    int bGuidNotSet;
    int bWindowRenderNotSupported;
} StdDisplayDeviceMatch;

static int J3DAPI stdDisplay_BlitSysToSys(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha);
static int J3DAPI stdDisplay_BlitSurfaceToSurface(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha);
static int J3DAPI stdDisplay_BlitSysToSurface(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha);
static int J3DAPI stdDisplay_BlitSurfaceToSys(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha);

int J3DAPI stdDisplay_Startup(void)
{
    HRESULT ddres;

    if ( stdDisplay_bStartup ) {
        return 1;
    }

    stdDisplay_bStartup   = 1;
    stdDisplay_numDevices = 0;

    memset(&stdDisplay_frontBuffer, 0, sizeof(stdDisplay_frontBuffer));
    memset(&stdDisplay_backBuffer, 0, sizeof(stdDisplay_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));

    ddres = DirectDrawEnumerate(stdDisplay_DDEnumCallback, NULL);
    if ( ddres != DD_OK )
    {
#line 149
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when enumerating DDraw devices.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    // TODO: Change default res
    stdDisplay_primaryVideoMode.rasterInfo.width  = 640;
    stdDisplay_primaryVideoMode.rasterInfo.height = 480;

#line 156
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found %d Display Devices.\n", stdDisplay_numDevices, 0, 0, 0);
    return 1;
}

void J3DAPI stdDisplay_Shutdown(void)
{
    if ( stdDisplay_bOpen ) {
        stdDisplay_Close();
    }

    stdDisplay_numVideoModes = 0;
    stdDisplay_pCurVideoMode = NULL;
    stdDisplay_numDevices    = 0;

    memset(stdDisplay_aDisplayDevices, 0, sizeof(stdDisplay_aDisplayDevices));
    memset(&stdDisplay_frontBuffer, 0, sizeof(stdDisplay_frontBuffer));
    memset(&stdDisplay_backBuffer, 0, sizeof(stdDisplay_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));
    stdDisplay_bStartup      = 0;
}

int J3DAPI stdDisplay_Open(size_t deviceNum)
{
#line 215
    STD_ASSERTREL(stdDisplay_bStartup == 1);
    if ( stdDisplay_bOpen )
    {
#line 219
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        stdDisplay_Close();
    }

    if ( deviceNum > stdDisplay_numDevices ) {
        return 0;
    }

    stdDisplay_curDevice  = deviceNum;
    stdDisplay_pCurDevice = &stdDisplay_aDisplayDevices[stdDisplay_curDevice];
    if ( !stdDisplay_InitDirectDraw(stdWin95_GetWindow()) ) {
        return 0;
    }

    qsort(
        stdDisplay_aVideoModes,
        stdDisplay_numVideoModes,
        sizeof(StdVideoMode),
        (int(__cdecl*)(const void*, const void*))stdDisplay_VideoModeCompare
    );

    stdDisplay_bOpen = 1;
    return 1;
}

void J3DAPI stdDisplay_Close(void)
{
    if ( stdDisplay_bStartup != 1 )
    {
#line 256
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "stdDisplay_Close when not started.", 0, 0, 0, 0);
        return;
    }

    if ( !stdDisplay_bOpen )
    {
#line 262
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
        return;
    }

    if ( stdDisplay_bModeSet ) {
        stdDisplay_ClearMode();
    }

    stdDisplay_ReleaseDirectDraw();

    stdDisplay_curDevice  = 0;
    stdDisplay_pCurDevice = NULL;
    memset(&stdDisplay_frontBuffer, 0, sizeof(stdDisplay_frontBuffer));
    memset(&stdDisplay_backBuffer, 0, sizeof(stdDisplay_backBuffer));
    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));

    stdDisplay_bOpen      = 0;
}

int J3DAPI stdDisplay_SetMode(size_t modeNum, int bFullscreen, size_t numBackBuffers)
{
    if ( bFullscreen && modeNum >= stdDisplay_numVideoModes ) {
        return 1;
    }

    if ( stdDisplay_bModeSet ) {
        stdDisplay_ClearMode();
    }

    if ( bFullscreen )
    {
        stdDisplay_pCurVideoMode = &stdDisplay_aVideoModes[modeNum];
        if ( stdDisplay_SetFullscreenMode(stdWin95_GetWindow(), stdDisplay_pCurVideoMode, numBackBuffers) ) {
            return 1;
        }
    }
    else
    {
        stdDisplay_pCurVideoMode = &stdDisplay_primaryVideoMode;
        if ( !stdDisplay_SetWindowMode(stdWin95_GetWindow(), stdDisplay_pCurVideoMode) ) {
            return 1;
        }
    }

    stdDisplay_hFont = CreateFontA(stdDisplay_pCurVideoMode->rasterInfo.width < 640 ? 12 : 24, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH, "Arial");

    stdDisplay_dword_5D73D8  = 0;
    stdDisplay_dword_5D73DC  = 0;
    stdDisplay_backbufWidth  = stdDisplay_backBuffer.rasterInfo.width;
    stdDisplay_backbufHeight = stdDisplay_backBuffer.rasterInfo.height;
    stdDisplay_bModeSet      = 1;
    stdDisplay_bFullscreen   = bFullscreen;

    stdDisplay_VBufferFill(&stdDisplay_backBuffer, 0, NULL); // 0 - BLACK
    stdDisplay_Update();

    if ( bFullscreen )
    {
        // TODO: Why filling back buffer again? Maybe front buffer should be filled
        stdDisplay_VBufferFill(&stdDisplay_backBuffer, 0, NULL); // 0 - BLACK
    }

    return 0;
}

void J3DAPI stdDisplay_ClearMode(void)
{
    if ( stdDisplay_bModeSet ) {
        stdDisplay_ReleaseBuffers();
    }

    if ( stdDisplay_hFont )
    {
        DeleteObject(stdDisplay_hFont);
        stdDisplay_hFont = NULL;
    }

    stdDisplay_bModeSet = 0;
}

size_t J3DAPI stdDisplay_GetNumDevices(void)
{
    return stdDisplay_numDevices;
}

int J3DAPI stdDisplay_GetDevice(size_t deviceNum, StdDisplayDevice* pDest)
{
    if ( deviceNum < stdDisplay_numDevices )
    {
        memcpy(pDest, &stdDisplay_aDisplayDevices[deviceNum], sizeof(StdDisplayDevice));
        return 0;
    }

    return 1;
}

int J3DAPI stdDisplay_GetCurrentDevice(StdDisplayDevice* pDevice)
{
    if ( stdDisplay_numDevices )
    {
        memcpy(pDevice, &stdDisplay_aDisplayDevices[stdDisplay_curDevice], sizeof(StdDisplayDevice));
        return 0;
    }

    return 1;
}

void J3DAPI stdDisplay_Refresh(int bReload)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet && bReload )
    {
        HRESULT ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, stdWin95_GetWindow(), stdDisplay_coopLevelFlags);
        if ( ddres != DD_OK )
        {
#line 436
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting cooperative level.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return;
        }

        if ( stdDisplay_bFullscreen )
        {
            if ( stdDisplay_lpDD )
            {
                ddres = IDirectDraw4_SetDisplayMode(
                    stdDisplay_lpDD,
                    stdDisplay_pCurVideoMode->rasterInfo.width,
                    stdDisplay_pCurVideoMode->rasterInfo.height,
                    stdDisplay_pCurVideoMode->rasterInfo.colorInfo.bpp,
                    0,
                    0
                );
                if ( ddres != DD_OK )
                {
#line 447
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s on set display Mode front buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                    return;
                }
            }

            if ( stdDisplay_frontBuffer.surface.pDDSurf )
            {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
                if ( ddres != DD_OK )
                {
#line 456
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s on restore front buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                    return;
                }
            }
        }
        else if ( stdDisplay_backBuffer.surface.pDDSurf )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_backBuffer.surface.pDDSurf);
            if ( ddres != DD_OK )
            {
#line 466
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s on restore front buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                return;
            }
        }

        if ( stdDisplay_zBuffer.pDDSurf )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_zBuffer.pDDSurf);
            if ( ddres != DD_OK )
            {
#line 476
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s on restore zBuffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            }
        }
    }
}

tVBuffer* J3DAPI stdDisplay_VBufferNew(const tRasterInfo* pRasterInfo, int bUseVSurface, int bUseVideoMemory)
{
    tVBuffer* curvbuffer;
    unsigned int bbp;
    tVSurface* pVSurf;
    HRESULT ddres;

#line 530
    STD_ASSERTREL((pRasterInfo->colorInfo.bpp % 8) == 0);
#line 533
    curvbuffer = (tVBuffer*)STDMALLOC(sizeof(tVBuffer));
    if ( !curvbuffer )
    {
#line 536
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating vbuffer.\n", 0, 0, 0, 0);
        return NULL;
    }

    curvbuffer->pPixels          = NULL;
    curvbuffer->lockSurfRefCount = 0;
    memcpy(&curvbuffer->rasterInfo, pRasterInfo, sizeof(curvbuffer->rasterInfo));

    curvbuffer->colorKey = 0;

    bbp = (unsigned int)curvbuffer->rasterInfo.colorInfo.bpp / 8;
    curvbuffer->rasterInfo.rowSize  = curvbuffer->rasterInfo.width * bbp;
    curvbuffer->rasterInfo.rowWidth = curvbuffer->rasterInfo.rowSize / bbp;
    curvbuffer->rasterInfo.size     = curvbuffer->rasterInfo.width * curvbuffer->rasterInfo.height * bbp;

    if ( !bUseVSurface || !stdDisplay_bOpen )
    {
        curvbuffer->lockRefCount = 0;
        curvbuffer->bVideoMemory = 0;
#line 561
        curvbuffer->pPixels = (uint8_t*)STDMALLOC(curvbuffer->rasterInfo.size);
        if ( !curvbuffer->pPixels ) {
            return NULL;
        }

        curvbuffer->lockSurfRefCount = 1;
    }
    else
    {
        curvbuffer->lockRefCount = 1;
        curvbuffer->bVideoMemory = 0;

        pVSurf = &curvbuffer->surface;
        memset(&pVSurf->ddSurfDesc, 0, sizeof(pVSurf->ddSurfDesc));
        pVSurf->ddSurfDesc.dwSize = sizeof(DDSURFACEDESC2);
        pVSurf->ddSurfDesc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        pVSurf->ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        if ( !bUseVideoMemory ) {
            pVSurf->ddSurfDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        }

        pVSurf->ddSurfDesc.dwWidth  = curvbuffer->rasterInfo.width;
        pVSurf->ddSurfDesc.dwHeight = curvbuffer->rasterInfo.height;
        ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &pVSurf->ddSurfDesc, &pVSurf->pDDSurf, NULL);
        if ( ddres != DD_OK )
        {
#line 611
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating a DirectDraw vbuffer surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return NULL;
        }

        ddres = IDirectDrawSurface4_GetSurfaceDesc(pVSurf->pDDSurf, &pVSurf->ddSurfDesc);
        if ( ddres != DD_OK )
        {
#line 619
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting desc of surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return NULL;
        }

        if ( (pVSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
            curvbuffer->bVideoMemory = 1;
        }

        curvbuffer->rasterInfo.rowSize  = pVSurf->ddSurfDesc.dwLinearSize;
        curvbuffer->rasterInfo.rowWidth = curvbuffer->rasterInfo.rowSize / bbp;
    }

    return curvbuffer;
}

void J3DAPI stdDisplay_VBufferFree(tVBuffer* pVBuffer)
{
    tVSurface* pSurf;

#line 653
    STD_ASSERTREL(pVBuffer!=NULL);
    switch ( pVBuffer->lockRefCount )
    {
        case 0:
            if ( pVBuffer->pPixels )
            {
                stdMemory_Free(pVBuffer->pPixels);
                pVBuffer->pPixels = 0;
            }
            break;

        case 1:
            pSurf = &pVBuffer->surface;
            if ( pSurf->pDDSurf )
            {
                IDirectDrawSurface4_Release(pSurf->pDDSurf);
                pSurf->pDDSurf = NULL;
            }
            break;
    }

    stdMemory_Free(pVBuffer);
}

int J3DAPI stdDisplay_VBufferLock(tVBuffer* pVBuffer)
{
#line 711
    STD_ASSERTREL(pVBuffer!=NULL);
    switch ( pVBuffer->lockRefCount )
    {
        case 0:
            ++pVBuffer->lockSurfRefCount;
            break;

        case 1:
            if ( (pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) != 0
                && (pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_MODEX) != 0 ) {
                return 0;
            }

            pVBuffer->pPixels = stdDisplay_LockSurface(&pVBuffer->surface);
            if ( !pVBuffer->pPixels ) {
                return 0;
            }

            ++pVBuffer->lockSurfRefCount;
            break;
    }

    return 1;
}

int J3DAPI stdDisplay_VBufferUnlock(tVBuffer* pVBuffer)
{
    int result;

#line 764
    STD_ASSERTREL(pVBuffer!=NULL);
    switch ( pVBuffer->lockRefCount )
    {
        case 0:
            if ( pVBuffer->lockSurfRefCount ) {
                --pVBuffer->lockSurfRefCount;
            }
            return 1;

        case 1:
            if ( pVBuffer->lockSurfRefCount )
            {
                result = stdDisplay_UnlockSurface(&pVBuffer->surface);
                if ( !result ) {
                    --pVBuffer->lockSurfRefCount;
                }
            }
            else
            {
                result = 0;
            }

            return result;
    }

    return 1;
}

/* VBufferFill's two fill helpers (inline assembly). Retail has no
   out-of-line body for either: both are __inline and expanded at two sites each (Fill32
   0x004BA685/0x004BA6C5, Fill16 0x004BA72F/0x004BA78B), every site storing its arguments into dead
   parameter homes and reloading them, the fingerprint of an inlined helper call.
   Retail's bodies are `cld; rep stosw/stosd`, an
   instruction no compiler emits (the only `cld` in the image), so they are __asm here, at these four
   sites only. Placed here by the retail __LINE__ gap 764..825 (VBufferUnlock -> VBufferFill, +20
   lines). Names are inferred; the third parameter is `count` because `size` is a MASM
   keyword. Not `static`: the debug build carries both as pick-any COMDATs (0x4F7670, 0x4F76B0), i.e.
   external `__inline` (the release object is identical either way). */
__inline void J3DAPI stdDisplay_SetPixels16(uint16_t* pPixels16, uint16_t pixel, size_t count)
{
    __asm
    {
        mov     edi, pPixels16
        mov     ecx, count
        test    ecx, 1
        jnz     odd_count
        mov     ax, pixel
        shl     eax, 16
        mov     ax, pixel
        shr     ecx, 1
        cld
        rep stosd
        jmp     done
odd_count:
        mov     ax, pixel
        cld
        rep stosw
done:
    }
}

__inline void J3DAPI stdDisplay_SetPixels32(uint32_t* pPixels32, uint32_t pixel, size_t count)
{
    __asm
    {
        mov     edi, pPixels32
        mov     eax, pixel
        mov     ecx, count
        cld
        rep stosd
    }
}

int J3DAPI stdDisplay_VBufferFill(tVBuffer* pVBuffer, uint32_t color, const StdRect* pRect)
{
    uint8_t* pPixels;
    int32_t i;

#line 825
    STD_ASSERTREL(pVBuffer!=NULL);

    switch ( pVBuffer->lockRefCount )
    {
        case 0:
            switch ( pVBuffer->rasterInfo.colorInfo.bpp )
            {
                case 32:
                    if ( !pRect )
                    {
                        stdDisplay_SetPixels32((uint32_t*)pVBuffer->pPixels, color, (size_t)(pVBuffer->rasterInfo.size / 4));
                    }
                    else
                    {
                        pPixels = (uint8_t*)((uint32_t*)pVBuffer->pPixels + pRect->top * pVBuffer->rasterInfo.rowWidth + pRect->left);
                        for ( i = 0; i < pRect->bottom; ++i )
                        {
                            stdDisplay_SetPixels32((uint32_t*)pPixels, color, pRect->right);
                            pPixels += pVBuffer->rasterInfo.rowSize;
                        }
                    }

                    break;

                case 24:
#line 852
                    STD_ASSERTREL(0);
                    break;

                case 16:
                    if ( !pRect )
                    {
                        stdDisplay_SetPixels16((uint16_t*)pVBuffer->pPixels, color, (size_t)(pVBuffer->rasterInfo.size / 2));
                    }
                    else
                    {
                        pPixels = (uint8_t*)((uint16_t*)pVBuffer->pPixels + pRect->top * pVBuffer->rasterInfo.rowWidth + pRect->left);
                        for ( i = 0; i < pRect->bottom; ++i )
                        {
                            stdDisplay_SetPixels16((uint16_t*)pPixels, color, pRect->right);
                            pPixels += pVBuffer->rasterInfo.rowSize;
                        }
                    }

                    break;

                case 8:
                    if ( !pRect )
                    {
                        memset(pVBuffer->pPixels, (uint8_t)color, pVBuffer->rasterInfo.size);
                    }
                    else
                    {
                        pPixels = pVBuffer->pPixels + pRect->top * pVBuffer->rasterInfo.rowWidth + pRect->left;
                        for ( i = 0; i < pRect->bottom; ++i )
                        {
                            memset(pPixels, (uint8_t)color, pRect->right);
                            pPixels += pVBuffer->rasterInfo.rowSize;
                        }
                    }

                    break;
            }

            break;

        case 1:
            if ( (pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) != 0
                && (pVBuffer->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_MODEX) != 0 ) {
                return 0;
            }

            return stdDisplay_ColorFillSurface(&pVBuffer->surface, color, pRect) == 0;
    }

    return 1;
}

/* ClearRect, VBufferCopy and VBufferSetColorKey: full bodies from the debug build (0x004F2835, lines 949/956 (the two
   `Error %s when waiting for blt.` prints); 0x004F2927, lines 994/995; 0x004F2A10, line 1043),
   confirmed by recompiling them under /Od. Still linker-discarded in retail; JK names
   and signatures. ClearRect fills through VBufferFill, then waits on the front buffer until its blits are done
   (GetBltStatus), restoring a lost surface. VBufferCopy dispatches on the two buffers' kind (`lockRefCount`: 0 = system
   memory, 1 = DirectDraw surface) to the four Blit* helpers further down (debug 0x004F33D8, 0x004F3A13, 0x004F3F46,
   0x004F4569); the debug code passes its six arguments through unchanged and returns 1. */
int J3DAPI stdDisplay_ClearRect(tVBuffer* pVBuffer, int fillColor, const StdRect* pRect)
{
    HRESULT ddres;

    stdDisplay_VBufferFill(pVBuffer, fillColor, pRect);
    while ( 1 )
    {
        ddres = IDirectDrawSurface4_GetBltStatus(stdDisplay_frontBuffer.surface.pDDSurf, DDGBS_ISBLTDONE);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            if ( ddres != DD_OK )
            {
#line 949
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when waiting for blt.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                return 0;
            }
        }

        if ( ddres != DDERR_WASSTILLDRAWING )
        {
#line 956
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when waiting for blt.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 0;
        }
    }

    return 1;
}

int J3DAPI stdDisplay_VBufferCopy(tVBuffer* pDst, tVBuffer* pSrc, unsigned int blitX, int blitY, const StdRect* pRect, int bAlpha)
{
#line 994
    STD_ASSERT(pDst!=NULL);
    STD_ASSERT(pSrc!=NULL);

    if ( pSrc->lockRefCount == 0 )
    {
        if ( pDst->lockRefCount == 0 ) {
            stdDisplay_BlitSysToSys(pDst, pSrc, blitX, blitY, pRect, bAlpha);
        }
        else {
            stdDisplay_BlitSysToSurface(pDst, pSrc, blitX, blitY, pRect, bAlpha);
        }
    }
    else if ( pDst->lockRefCount == 1 ) {
        stdDisplay_BlitSurfaceToSurface(pDst, pSrc, blitX, blitY, pRect, bAlpha);
    }
    else {
        stdDisplay_BlitSurfaceToSys(pDst, pSrc, blitX, blitY, pRect, bAlpha);
    }

    return 1;
}

int J3DAPI stdDisplay_VBufferSetColorKey(tVBuffer* pVBuffer, int color)
{
    DDCOLORKEY ddck;
    DWORD dwFlags;

#line 1043
    STD_ASSERT(pVBuffer!=NULL);
    pVBuffer->colorKey = color;
    switch ( pVBuffer->lockRefCount )
    {
        case 0:
            break;

        case 1:
            dwFlags = DDCKEY_SRCBLT;
            ddck.dwColorSpaceLowValue  = color;
            ddck.dwColorSpaceHighValue = color;
            IDirectDrawSurface4_SetColorKey(pVBuffer->surface.pDDSurf, dwFlags, &ddck);
            break;
    }

    return 1;
}

tVBuffer* J3DAPI stdDisplay_VBufferConvertColorFormat(const ColorInfo* pDesiredColorFormat, tVBuffer* pSrc, int bColorKey, uint32_t colorKey)
{
    tVBuffer* pDest;
    const ColorInfo* pSrcColorFormat;
    const uint8_t* pSrcRow;
    uint8_t* pDestRow;
    int y;
    tRasterInfo destRaster;

    pSrcRow  = NULL;
    pDestRow = NULL;

#line 1101
    STD_ASSERTREL(pSrc != NULL);
    if ( memcmp(pDesiredColorFormat, &pSrc->rasterInfo.colorInfo, sizeof(ColorInfo)) == 0 ) {
        return pSrc;
    }

    if ( pSrc->rasterInfo.colorInfo.colorMode == STDCOLOR_PAL && pDesiredColorFormat->colorMode == STDCOLOR_PAL ) {
        return pSrc;
    }

#line 1112
    STD_ASSERTREL(pSrc->rasterInfo.colorInfo.colorMode != STDCOLOR_PAL);
    STD_ASSERTREL(pDesiredColorFormat->colorMode != STDCOLOR_PAL);
    pSrcColorFormat = &pSrc->rasterInfo.colorInfo;
#line 1118
    STD_ASSERTREL(pSrcColorFormat->redBPP != 0);
    STD_ASSERTREL(pSrcColorFormat->greenBPP != 0);
    STD_ASSERTREL(pSrcColorFormat->blueBPP != 0);
    STD_ASSERTREL(pDesiredColorFormat->redBPP != 0);
    STD_ASSERTREL(pDesiredColorFormat->greenBPP != 0);
    STD_ASSERTREL(pDesiredColorFormat->blueBPP != 0);
#line 1128
    STD_ASSERTREL(pSrcColorFormat->redPosShift != 0 || pSrcColorFormat->greenPosShift != 0 || pSrcColorFormat->bluePosShift != 0);
#line 1131
    STD_ASSERTREL(pDesiredColorFormat->redPosShift != 0 || pDesiredColorFormat->greenPosShift != 0 || pDesiredColorFormat->bluePosShift != 0);

    if ( pSrc->rasterInfo.colorInfo.bpp == pDesiredColorFormat->bpp )
    {
        pDest = pSrc;
    }
    else
    {
        destRaster = pSrc->rasterInfo;
        destRaster.colorInfo = *pDesiredColorFormat;

        pDest = stdDisplay_VBufferNew(&destRaster, 0, 0);
        if ( !pDest )
        {
#line 1144
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to allocate memory for new tVBuffer", 0, 0, 0, 0);
            return NULL;
        }
    }

    stdDisplay_VBufferLock(pSrc);
    stdDisplay_VBufferLock(pDest);

#line 1151
    STD_ASSERTREL(pSrc->pPixels != NULL);
    STD_ASSERTREL(pDest->pPixels != NULL);

    for ( y = 0; y < (int)pDest->rasterInfo.height; ++y )
    {
        pSrcRow  = &pSrc->pPixels[y * pSrc->rasterInfo.rowSize];
        pDestRow = &pDest->pPixels[y * pDest->rasterInfo.rowSize];
        stdColor_ColorConvertOneRow(pDestRow, pDesiredColorFormat, pSrcRow, pSrcColorFormat, pDest->rasterInfo.width, bColorKey, colorKey);
    }

#line 1163
    STD_ASSERTREL(pDestRow <= pDest->pPixels + pDest->rasterInfo.size);
    STD_ASSERTREL(pSrcRow <= pSrc->pPixels + pSrc->rasterInfo.size);

    stdDisplay_VBufferUnlock(pSrc);
    stdDisplay_VBufferUnlock(pDest);

    pDest->rasterInfo.colorInfo = *pDesiredColorFormat;
    if ( pDest != pSrc ) {
        stdDisplay_VBufferFree(pSrc);
    }

    return pDest;
}

/* PLACEHOLDERS, linker-discarded: the Indy debug build has these three functions between
   VBufferConvertColorFormat and VideoModeCompare, in this order (debug 0x004F2E76, 0x004F2EA8, 0x004F2F37); retail's linker
   dropped them (no caller, no symbol). Bodies read from the unoptimised code and checked by recompiling under /Od
   (checked by recompiling). Names are inferred (JK has a TextOut and a FindClosestDevice at the same place); SetViewRect is
   named for what it stores: the four statics SetMode resets to 0, 0, width, height, written by nothing else and read by nothing. */
int J3DAPI stdDisplay_SetViewRect(int left, int top, int right, int bottom)
{
    stdDisplay_dword_5D73D8  = left;
    stdDisplay_dword_5D73DC  = top;
    stdDisplay_backbufWidth  = right + 1;
    stdDisplay_backbufHeight = bottom + 1;
    return 1;
}

int J3DAPI stdDisplay_TextOut(const char* pText, int x, int y)
{
    HDC hdc;

    if ( IDirectDrawSurface4_GetDC(stdDisplay_backBuffer.surface.pDDSurf, &hdc) == DD_OK )
    {
        SetBkMode(hdc, TRANSPARENT);
        SelectObject(hdc, stdDisplay_hFont);
        SetTextColor(hdc, RGB(255, 255, 0));
        TextOutA(hdc, x, y, pText, strlen(pText));
        IDirectDrawSurface4_ReleaseDC(stdDisplay_backBuffer.surface.pDDSurf, hdc);
        return 1;
    }

    return 0;
}

size_t J3DAPI stdDisplay_FindClosestDevice(const StdDisplayDeviceMatch* pMatch)
{
    int bestScore;
    int curScore;
    size_t bestDevice;
    size_t i;
    StdDisplayDevice* pDevice;

    if ( !stdDisplay_numDevices ) {
        return 0;
    }

    bestScore  = 0;
    bestDevice = 0;
    pDevice    = stdDisplay_aDisplayDevices;
    for ( i = 0; i < stdDisplay_numDevices; ++i )
    {
        curScore = 0;
        if ( pDevice->bWindowRenderNotSupported == pMatch->bWindowRenderNotSupported )
        {
            ++curScore;
            if ( pDevice->bGuidNotSet == pMatch->bGuidNotSet )
            {
                ++curScore;
                if ( pDevice->bHAL == pMatch->bHAL )
                {
                    ++curScore;
#line 1288
                    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found a perfect device match #%d!\n", i, 0, 0, 0);
                    return i;
                }
            }
        }

        if ( curScore > bestScore )
        {
            bestDevice = i;
            bestScore  = curScore;
        }

        ++pDevice;
    }

#line 1306
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Settling for a closest match #%d..\n", bestDevice, 0, 0, 0);
    return bestDevice;
}

/* Linker-discarded (debug 0x004F305D, lines 1377/1387/1408; the /Gf literal order and the retail line gap place it between
   FindClosestDevice and VideoModeCompare). Full body from the debug build: it scans the module's video-mode array
   (`stdDisplay_aVideoModes`, `stdDisplay_numVideoModes`), so the one parameter is the wanted mode (the earlier JK-shaped
   (pMatch, aModes, numModes) signature is not what the debug code takes). Scoring mirrors FindClosestDevice. */
size_t J3DAPI stdDisplay_FindClosestMode(const StdVideoMode* pMatch)
{
    int closestScore;
    StdVideoMode* pMode;
    int curScore;
    size_t bestMode;
    size_t i;

    if ( !stdDisplay_numVideoModes ) {
        return 0;
    }

    closestScore = 0;
    bestMode     = 0;
    pMode        = stdDisplay_aVideoModes;
    for ( i = 0; i < stdDisplay_numVideoModes; ++i )
    {
        curScore = 0;
        if ( pMode->rasterInfo.colorInfo.colorMode == pMatch->rasterInfo.colorInfo.colorMode )
        {
            ++curScore;
            if ( pMode->rasterInfo.colorInfo.bpp == pMatch->rasterInfo.colorInfo.bpp )
            {
                ++curScore;
                if ( pMode->rasterInfo.width == pMatch->rasterInfo.width )
                {
                    ++curScore;
                    if ( pMode->rasterInfo.height == pMatch->rasterInfo.height )
                    {
                        ++curScore;
                        if ( pMatch->rasterInfo.colorInfo.colorMode == STDCOLOR_RGB )
                        {
                            if ( pMode->rasterInfo.colorInfo.redBPP == pMatch->rasterInfo.colorInfo.redBPP
                                && pMode->rasterInfo.colorInfo.greenBPP == pMatch->rasterInfo.colorInfo.greenBPP
                                && pMode->rasterInfo.colorInfo.blueBPP == pMatch->rasterInfo.colorInfo.blueBPP )
                            {
                                ++curScore;
#line 1377
                                stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found a perfect mode match #%d!\n", i, 0, 0, 0);
                                return i;
                            }
                        }
                        else
                        {
                            ++curScore;
#line 1387
                            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found a perfect mode match #%d!\n", i, 0, 0, 0);
                            return i;
                        }
                    }
                }
            }
        }

        if ( curScore > closestScore )
        {
            bestMode     = i;
            closestScore = curScore;
        }

        ++pMode;
    }

#line 1408
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Settling for a closest match #%d..\n", bestMode, 0, 0, 0);
    return bestMode;
}

int J3DAPI stdDisplay_VideoModeCompare(const StdVideoMode* pMode1, const StdVideoMode* pMode2)
{
    unsigned int bpp1;
    unsigned int bpp2;

    bpp1 = pMode1->rasterInfo.colorInfo.bpp;
    bpp2 = pMode2->rasterInfo.colorInfo.bpp;
    if ( bpp1 != bpp2 )
    {
        return bpp1 - bpp2;
    }

    bpp1 = pMode1->rasterInfo.width;
    bpp2 = pMode2->rasterInfo.width;
    if ( bpp1 != bpp2 )
    {
        return bpp1 - bpp2;
    }

    bpp1 = pMode1->rasterInfo.height;
    bpp2 = pMode2->rasterInfo.height;
    return bpp1 - bpp2;
}

int J3DAPI stdDisplay_GetTextureMemory(size_t* pTotal, size_t* pFree)
{
    HRESULT hDdres;
    DDSCAPS2 ddsc;

    memset(&ddsc, 0, sizeof(ddsc));
    ddsc.dwCaps = DDSCAPS_TEXTURE;
    hDdres = IDirectDraw4_GetAvailableVidMem(stdDisplay_lpDD, &ddsc, (LPDWORD)pTotal, (LPDWORD)pFree);
    if ( hDdres != DD_OK )
    {
#line 1460
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s in stdDisplay_GetTextureMemory.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 1;
    }

    return 0;
}

int J3DAPI stdDisplay_GetTotalMemory(size_t* pTotal, size_t* pFree)
{
    HRESULT hDdres;
    DDSCAPS2 ddscaps;

    memset(&ddscaps, 0, sizeof(ddscaps));
    ddscaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    hDdres = IDirectDraw4_GetAvailableVidMem(stdDisplay_lpDD, &ddscaps, (LPDWORD)pTotal, (LPDWORD)pFree);
    if ( hDdres != DD_OK )
    {
#line 1477
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s in stdDisplay_GetTotalMemory.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 1;
    }

    return 0;
}

const char* J3DAPI stdDisplay_DDGetStatus(HRESULT status)
{
    const char* pErrorText = "Unknown Error";
    int i;
    int numStatus = STD_ARRAYLEN(stdDisplay_aDDStatusTbl);
    for ( i = 0; i < numStatus; ++i )
    {
        if ( stdDisplay_aDDStatusTbl[i].code == status )
        {
            pErrorText = stdDisplay_aDDStatusTbl[i].text;
            break;
        }
    }

    return pErrorText;
}

/* Linker-discarded (debug build 0x004F33D8, no diagnostic sites): VBufferCopy's system-memory to system-memory path. Clips the
   destination rectangle against the destination buffer and copies row by row (memcpy), or pixel by pixel skipping the source
   colour key when `bAlpha & 1`. The 24 bpp case has no colour-key variant. Names (and the locals') inferred; reconstructed from the unoptimised
   debug code (frame slots and instruction shapes checked by recompiling). */
static int J3DAPI stdDisplay_BlitSysToSys(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha)
{
    unsigned int width;
    unsigned int dstSkip;
    unsigned int copyWidth;
    unsigned int col;
    unsigned int rowBytes;
    unsigned int row;
    uint16_t* pSrc16;
    unsigned int height;
    uint16_t* pDst16;
    unsigned int srcX;
    unsigned int srcY;
    unsigned int dstRight;
    uint8_t* pSrc8;
    uint32_t* pSrc32;
    uint32_t* pDst32;
    uint8_t* pDst8;
    unsigned int rectX;
    unsigned int srcSkip;
    unsigned int dstBottom;
    unsigned int rectY;

    if ( pRect )
    {
        rectX  = pRect->left;
        rectY  = pRect->top;
        width  = pRect->right;
        height = pRect->bottom;
    }
    else
    {
        rectX  = 0;
        rectY  = 0;
        width  = pSrc->rasterInfo.width;
        height = pSrc->rasterInfo.height;
    }

    if ( x >= (int)pDst->rasterInfo.width || x + width <= 0 ) {
        return 0;
    }

    if ( y >= (int)pDst->rasterInfo.height || y + height <= 0 ) {
        return 0;
    }

    dstRight  = x + width - 1;
    dstBottom = y + height - 1;
    srcX      = rectX;
    srcY      = rectY;

    if ( y < 0 )
    {
        srcY += -y;
        y = 0;
    }

    if ( dstBottom >= pDst->rasterInfo.height ) {
        dstBottom = pDst->rasterInfo.height - 1;
    }

    if ( x < 0 )
    {
        srcX += -x;
        x = 0;
    }

    if ( dstRight >= pDst->rasterInfo.width ) {
        dstRight = pDst->rasterInfo.width - 1;
    }

    copyWidth = dstRight - x + 1;

    if ( copyWidth > 0 )
    {
        rowBytes = copyWidth * (pSrc->rasterInfo.colorInfo.bpp / 8);

        switch ( pSrc->rasterInfo.colorInfo.bpp )
        {
            case 32:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc32 = (uint32_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst32 = (uint32_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc32 != pSrc->colorKey ) {
                                *pDst32 = *pSrc32;
                            }

                            ++pSrc32;
                            ++pDst32;
                        }

                        pSrc32 += srcSkip;
                        pDst32 += dstSkip;
                    }
                }
                else
                {
                    pSrc32 = (uint32_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst32 = (uint32_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst32, pSrc32, rowBytes);
                        pSrc32 += pSrc->rasterInfo.rowWidth;
                        pDst32 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;

            case 24:
                pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowSize + srcX * 3;
                pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowSize + x * 3;
                for ( row = y; row <= dstBottom; ++row )
                {
                    memcpy(pDst8, pSrc8, rowBytes);
                    pSrc8 += pSrc->rasterInfo.rowSize;
                    pDst8 += pDst->rasterInfo.rowSize;
                }
                break;

            case 16:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc16 = (uint16_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst16 = (uint16_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc16 != (uint16_t)pSrc->colorKey ) {
                                *pDst16 = *pSrc16;
                            }

                            ++pSrc16;
                            ++pDst16;
                        }

                        pSrc16 += srcSkip;
                        pDst16 += dstSkip;
                    }
                }
                else
                {
                    pSrc16 = (uint16_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst16 = (uint16_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst16, pSrc16, rowBytes);
                        pSrc16 += pSrc->rasterInfo.rowWidth;
                        pDst16 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;

            case 8:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc8 != (uint8_t)pSrc->colorKey ) {
                                *pDst8 = *pSrc8;
                            }

                            ++pSrc8;
                            ++pDst8;
                        }

                        pSrc8 += srcSkip;
                        pDst8 += dstSkip;
                    }
                }
                else
                {
                    pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst8, pSrc8, rowBytes);
                        pSrc8 += pSrc->rasterInfo.rowWidth;
                        pDst8 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;
        }
    }

    return 1;
}

/* Linker-discarded (debug build 0x004F3A13, lines 2028/2039/2046): VBufferCopy's DirectDraw-surface to DirectDraw-surface
   path: IDirectDrawSurface4::Blt of the (clipped by DirectDraw) rectangle, restoring a lost surface and retrying. Names
   inferred. The debug function also contains an `if (0)` arm before this one (IDirectDrawSurface4::BltFast, prints at lines
   1942/1953/1960); a literal dead branch is not reproduced here (open). */
static int J3DAPI stdDisplay_BlitSurfaceToSurface(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha)
{
    RECT srcRect;
    DDBLTFX bltfx;
    HRESULT ddres;
    RECT dstRect;
    DWORD flags;

    if ( pDst->lockSurfRefCount || pSrc->lockSurfRefCount ) {
        return 0;
    }

    dstRect.left = x;
    dstRect.top  = y;
    if ( pRect )
    {
        if ( !pRect->right ) {
            return 0;
        }

        if ( !pRect->bottom ) {
            return 0;
        }

        srcRect.left   = pRect->left;
        srcRect.top    = pRect->top;
        srcRect.right  = pRect->left + pRect->right;
        srcRect.bottom = pRect->top + pRect->bottom;
        dstRect.right  = x + pRect->right;
        dstRect.bottom = y + pRect->bottom;
    }
    else
    {
        srcRect.left   = 0;
        srcRect.top    = 0;
        srcRect.right  = pSrc->rasterInfo.width;
        srcRect.bottom = pSrc->rasterInfo.height;
        dstRect.right  = x + pSrc->rasterInfo.width;
        dstRect.bottom = y + pSrc->rasterInfo.height;
    }

    memset(&bltfx, 0, sizeof(bltfx));
    bltfx.dwSize = sizeof(bltfx);

    flags = DDBLT_WAIT;
    if ( bAlpha & 1 ) {
        flags |= DDBLT_KEYSRC;
    }

    while ( 1 )
    {
        ddres = IDirectDrawSurface4_Blt(pDst->surface.pDDSurf, &dstRect, pSrc->surface.pDDSurf, &srcRect, flags, &bltfx);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            if ( (pDst->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pDst->surface.pDDSurf);
            }

            if ( ddres != DD_OK )
            {
#line 2028
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when restoring surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                return 0;
            }

            if ( (pSrc->surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pSrc->surface.pDDSurf);
            }

            if ( ddres != DD_OK )
            {
#line 2039
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when restoring surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                return 0;
            }
        }

        if ( ddres != DD_OK )
        {
#line 2046
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when copying a surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 0;
        }
    }

    return 1;
}

/* Linker-discarded (debug build 0x004F3F46): VBufferCopy's system-memory source to DirectDraw-surface destination path. Same
   clipping and copy loops as BlitSysToSys, but the destination surface is locked around it and the 24 bpp case is the
   debug-only `STD_ASSERT(0)` (original line 2192). Names, including the locals', inferred (the debug frame matches up to local-name hash order). */
static int J3DAPI stdDisplay_BlitSysToSurface(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha)
{
    unsigned int width;
    unsigned int dstSkip;
    unsigned int copyWidth;
    unsigned int col;
    unsigned int rowBytes;
    unsigned int row;
    uint16_t* pSrc16;
    unsigned int height;
    uint16_t* pDst16;
    unsigned int srcX;
    unsigned int srcY;
    unsigned int dstRight;
    uint8_t* pSrc8;
    uint32_t* pSrc32;
    uint32_t* pDst32;
    uint8_t* pDst8;
    unsigned int rectX;
    unsigned int srcSkip;
    unsigned int dstBottom;
    unsigned int rectY;

    if ( pDst->lockSurfRefCount ) {
        return 0;
    }

    if ( pRect )
    {
        if ( !pRect->right ) {
            return 0;
        }

        if ( !pRect->bottom ) {
            return 0;
        }

        rectX  = pRect->left;
        rectY  = pRect->top;
        width  = pRect->right;
        height = pRect->bottom;
    }
    else
    {
        rectX  = 0;
        rectY  = 0;
        width  = pSrc->rasterInfo.width;
        height = pSrc->rasterInfo.height;
    }

    if ( x >= (int)pDst->rasterInfo.width || x + width <= 0 ) {
        return 0;
    }

    if ( y >= (int)pDst->rasterInfo.height || y + height <= 0 ) {
        return 0;
    }

    dstRight  = x + width - 1;
    dstBottom = y + height - 1;
    srcX      = rectX;
    srcY      = rectY;

    if ( y < 0 )
    {
        srcY += -y;
        y = 0;
    }

    if ( dstBottom >= pDst->rasterInfo.height ) {
        dstBottom = pDst->rasterInfo.height - 1;
    }

    if ( x < 0 )
    {
        srcX += -x;
        x = 0;
    }

    if ( dstRight >= pDst->rasterInfo.width ) {
        dstRight = pDst->rasterInfo.width - 1;
    }

    copyWidth = dstRight - x + 1;

    if ( copyWidth > 0 )
    {
        rowBytes = copyWidth * (pSrc->rasterInfo.colorInfo.bpp / 8);
        stdDisplay_VBufferLock(pDst);

        switch ( pSrc->rasterInfo.colorInfo.bpp )
        {
            case 32:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc32 = (uint32_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst32 = (uint32_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc32 != pSrc->colorKey ) {
                                *pDst32 = *pSrc32;
                            }

                            ++pSrc32;
                            ++pDst32;
                        }

                        pSrc32 += srcSkip;
                        pDst32 += dstSkip;
                    }
                }
                else
                {
                    pSrc32 = (uint32_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst32 = (uint32_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst32, pSrc32, rowBytes);
                        pSrc32 += pSrc->rasterInfo.rowWidth;
                        pDst32 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;

            case 24:
#line 2192
                STD_ASSERT(0);
                break;

            case 16:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc16 = (uint16_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst16 = (uint16_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc16 != (uint16_t)pSrc->colorKey ) {
                                *pDst16 = *pSrc16;
                            }

                            ++pSrc16;
                            ++pDst16;
                        }

                        pSrc16 += srcSkip;
                        pDst16 += dstSkip;
                    }
                }
                else
                {
                    pSrc16 = (uint16_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst16 = (uint16_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst16, pSrc16, rowBytes);
                        pSrc16 += pSrc->rasterInfo.rowWidth;
                        pDst16 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;

            case 8:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc8 != (uint8_t)pSrc->colorKey ) {
                                *pDst8 = *pSrc8;
                            }

                            ++pSrc8;
                            ++pDst8;
                        }

                        pSrc8 += srcSkip;
                        pDst8 += dstSkip;
                    }
                }
                else
                {
                    pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst8, pSrc8, rowBytes);
                        pSrc8 += pSrc->rasterInfo.rowWidth;
                        pDst8 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;
        }

        stdDisplay_VBufferUnlock(pDst);
    }

    return 1;
}

/* Linker-discarded (debug build 0x004F4569): VBufferCopy's DirectDraw-surface source to system-memory destination path: the
   mirror of BlitSysToSurface (the source surface is locked; the 24 bpp case asserts at original line 2418). Names, including the locals', inferred (the debug frame matches up to local-name hash order). */
static int J3DAPI stdDisplay_BlitSurfaceToSys(tVBuffer* pDst, tVBuffer* pSrc, int x, int y, const StdRect* pRect, int bAlpha)
{
    unsigned int width;
    unsigned int dstSkip;
    unsigned int copyWidth;
    unsigned int col;
    unsigned int rowBytes;
    unsigned int row;
    uint16_t* pSrc16;
    unsigned int height;
    uint16_t* pDst16;
    unsigned int srcX;
    unsigned int srcY;
    unsigned int dstRight;
    uint8_t* pSrc8;
    uint32_t* pSrc32;
    uint32_t* pDst32;
    uint8_t* pDst8;
    unsigned int rectX;
    unsigned int srcSkip;
    unsigned int dstBottom;
    unsigned int rectY;

    if ( pSrc->lockSurfRefCount ) {
        return 0;
    }

    if ( pRect )
    {
        if ( !pRect->right ) {
            return 0;
        }

        if ( !pRect->bottom ) {
            return 0;
        }

        rectX  = pRect->left;
        rectY  = pRect->top;
        width  = pRect->right;
        height = pRect->bottom;
    }
    else
    {
        rectX  = 0;
        rectY  = 0;
        width  = pSrc->rasterInfo.width;
        height = pSrc->rasterInfo.height;
    }

    if ( x >= (int)pDst->rasterInfo.width || x + width <= 0 ) {
        return 0;
    }

    if ( y >= (int)pDst->rasterInfo.height || y + height <= 0 ) {
        return 0;
    }

    dstRight  = x + width - 1;
    dstBottom = y + height - 1;
    srcX      = rectX;
    srcY      = rectY;

    if ( y < 0 )
    {
        srcY += -y;
        y = 0;
    }

    if ( dstBottom >= pDst->rasterInfo.height ) {
        dstBottom = pDst->rasterInfo.height - 1;
    }

    if ( x < 0 )
    {
        srcX += -x;
        x = 0;
    }

    if ( dstRight >= pDst->rasterInfo.width ) {
        dstRight = pDst->rasterInfo.width - 1;
    }

    copyWidth = dstRight - x + 1;

    if ( copyWidth > 0 )
    {
        rowBytes = copyWidth * (pSrc->rasterInfo.colorInfo.bpp / 8);
        stdDisplay_VBufferLock(pSrc);

        switch ( pSrc->rasterInfo.colorInfo.bpp )
        {
            case 32:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc32 = (uint32_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst32 = (uint32_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc32 != pSrc->colorKey ) {
                                *pDst32 = *pSrc32;
                            }

                            ++pSrc32;
                            ++pDst32;
                        }

                        pSrc32 += srcSkip;
                        pDst32 += dstSkip;
                    }
                }
                else
                {
                    pSrc32 = (uint32_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst32 = (uint32_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst32, pSrc32, rowBytes);
                        pSrc32 += pSrc->rasterInfo.rowWidth;
                        pDst32 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;

            case 24:
#line 2418
                STD_ASSERT(0);
                break;

            case 16:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc16 = (uint16_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst16 = (uint16_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc16 != (uint16_t)pSrc->colorKey ) {
                                *pDst16 = *pSrc16;
                            }

                            ++pSrc16;
                            ++pDst16;
                        }

                        pSrc16 += srcSkip;
                        pDst16 += dstSkip;
                    }
                }
                else
                {
                    pSrc16 = (uint16_t*)pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst16 = (uint16_t*)pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst16, pSrc16, rowBytes);
                        pSrc16 += pSrc->rasterInfo.rowWidth;
                        pDst16 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;

            case 8:
                if ( bAlpha & 1 )
                {
                    srcSkip = pSrc->rasterInfo.rowWidth - copyWidth;
                    dstSkip = pDst->rasterInfo.rowWidth - copyWidth;
                    pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        for ( col = x; col <= dstRight; ++col )
                        {
                            if ( *pSrc8 != (uint8_t)pSrc->colorKey ) {
                                *pDst8 = *pSrc8;
                            }

                            ++pSrc8;
                            ++pDst8;
                        }

                        pSrc8 += srcSkip;
                        pDst8 += dstSkip;
                    }
                }
                else
                {
                    pSrc8 = pSrc->pPixels + srcY * pSrc->rasterInfo.rowWidth + srcX;
                    pDst8 = pDst->pPixels + y * pDst->rasterInfo.rowWidth + x;
                    for ( row = y; row <= dstBottom; ++row )
                    {
                        memcpy(pDst8, pSrc8, rowBytes);
                        pSrc8 += pSrc->rasterInfo.rowWidth;
                        pDst8 += pDst->rasterInfo.rowWidth;
                    }
                }
                break;
        }

        stdDisplay_VBufferUnlock(pSrc);
    }

    return 1;
}

int J3DAPI stdDisplay_CreateZBuffer(LPDDPIXELFORMAT pPixelFormat, int bSystemMemory)
{
    HRESULT ddres;

    memset(&stdDisplay_zBuffer.ddSurfDesc, 0, sizeof(stdDisplay_zBuffer.ddSurfDesc));
    stdDisplay_zBuffer.ddSurfDesc.dwSize         = sizeof(DDSURFACEDESC2);
    stdDisplay_zBuffer.ddSurfDesc.dwFlags        = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    stdDisplay_zBuffer.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    stdDisplay_zBuffer.ddSurfDesc.dwWidth        = stdDisplay_backBuffer.rasterInfo.width;
    stdDisplay_zBuffer.ddSurfDesc.dwHeight       = stdDisplay_backBuffer.rasterInfo.height;
    if ( bSystemMemory )
    {
        stdDisplay_zBuffer.ddSurfDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }
    else
    {
        stdDisplay_zBuffer.ddSurfDesc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    }

    memcpy(&stdDisplay_zBuffer.ddSurfDesc.ddpfPixelFormat, pPixelFormat, sizeof(stdDisplay_zBuffer.ddSurfDesc.ddpfPixelFormat));
    ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_zBuffer.ddSurfDesc, &stdDisplay_zBuffer.pDDSurf, NULL);
    if ( ddres != DD_OK )
    {
#line 2535
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating zBuffer DDraw surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 1;
    }

    ddres = IDirectDrawSurface4_AddAttachedSurface(stdDisplay_backBuffer.surface.pDDSurf, stdDisplay_zBuffer.pDDSurf);
    if ( ddres != DD_OK )
    {
#line 2543
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when attaching zbuffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 1;
    }

    ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_zBuffer.pDDSurf, &stdDisplay_zBuffer.ddSurfDesc);
    if ( ddres != DD_OK )
    {
#line 2551
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting zbuffer surface description.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 1;
    }

    return 0;
}

int J3DAPI stdDisplay_InitDirectDraw(HWND hwnd)
{
    HRESULT hDdres;
    LPDIRECTDRAW lpDD;

    
    lpDD = NULL;
    if ( stdDisplay_pCurDevice->bGuidNotSet ) {
        hDdres = DirectDrawCreate(NULL, &lpDD, NULL);
    }
    else {
        hDdres = DirectDrawCreate(&stdDisplay_pCurDevice->guid, &lpDD, NULL);
    }

    if ( hDdres != DD_OK )
    {
#line 2615
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating DirectDraw object.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }

    hDdres = IDirectDraw_QueryInterface(lpDD, &IID_IDirectDraw4, &stdDisplay_lpDD);
    if ( hDdres != DD_OK )
    {
#line 2622
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error getting DirectDraw4 %s.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }

    hDdres = IDirectDraw_Release(lpDD);
    if ( hDdres != DD_OK )
    {
#line 2629
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error Release %s.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }
    lpDD = NULL;

    stdDisplay_numVideoModes = 0;
    hDdres = IDirectDraw4_EnumDisplayModes(stdDisplay_lpDD, 0, NULL, NULL, stdDisplay_EnumVideoModesCallback);
    if ( hDdres != DD_OK )
    {
#line 2649
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when enumerating video modes.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }

    return 1;
}

void J3DAPI stdDisplay_ReleaseDirectDraw(void)
{
    if ( stdDisplay_lpDD )
    {
        HRESULT ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, 0, DDSCL_NORMAL);
        if ( ddres != DD_OK ) {
#line 2680
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting coop level.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        }

        ddres = IDirectDraw4_RestoreDisplayMode(stdDisplay_lpDD);
        if ( ddres != DD_OK ) {
#line 2684
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when restoring the video mode.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        }

        IDirectDraw4_Release(stdDisplay_lpDD);
        stdDisplay_lpDD = NULL;
    }

    stdDisplay_coopLevelFlags = DDSCL_NORMAL;
    stdDisplay_numVideoModes = 0;
}

BOOL PASCAL stdDisplay_DDEnumCallback(GUID* lpGUID, LPSTR szDriverName, LPSTR szDriverDescription, LPVOID lpContext)
{
    StdDisplayDevice* pDevice;
    LPDIRECTDRAW lpDD;
    HRESULT ddres;


    lpDD = NULL;
    if ( stdDisplay_numDevices < STD_ARRAYLEN(stdDisplay_aDisplayDevices) )
    {
        pDevice = &stdDisplay_aDisplayDevices[stdDisplay_numDevices];
        if ( lpGUID )
        {
            pDevice->guid = *lpGUID;
            pDevice->bGuidNotSet = 0;
        }
        else
        {
            memset(&pDevice->guid, 0, sizeof(pDevice->guid));
            pDevice->bGuidNotSet = 1;
        }

        strncpy(pDevice->aDriverName, szDriverName, STD_ARRAYLEN(pDevice->aDriverName) - 1);
        pDevice->aDriverName[STD_ARRAYLEN(pDevice->aDriverName) - 1] = 0;
        strncpy(pDevice->aDeviceName, szDriverDescription, STD_ARRAYLEN(pDevice->aDeviceName) - 1);
        pDevice->aDeviceName[STD_ARRAYLEN(pDevice->aDeviceName) - 1] = 0;

        ddres = DirectDrawCreate(lpGUID, &lpDD, 0);
        if ( ddres != DD_OK )
        {
#line 2743
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating scratch DirectDraw object.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 0;
        }

        memset(&pDevice->ddcaps, 0, sizeof(pDevice->ddcaps));
        pDevice->ddcaps.dwSize = sizeof(DDCAPS_DX6);
        ddres = IDirectDraw_GetCaps(lpDD, (LPDDCAPS)&pDevice->ddcaps, 0);
        if ( ddres != DD_OK )
        {
#line 2764
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when reading DDraw CAPS.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            IDirectDraw_Release(lpDD);
            return 0;
        }

        if ( (pDevice->ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED) != 0 ) {
            pDevice->bWindowRenderNotSupported = 0;
        }
        else {
            pDevice->bWindowRenderNotSupported = 1;
        }

        if ( (pDevice->ddcaps.dwCaps & DDCAPS_3D) != 0 ) {
            pDevice->bHAL = 1;
        }
        else {
            pDevice->bHAL = 0;
        }

        pDevice->totalVideoMemory = pDevice->ddcaps.dwVidMemTotal;
        pDevice->freeVideoMemory  = pDevice->ddcaps.dwVidMemFree;

#line 2800
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found %s DD Device: %s [%s]\n", pDevice->bHAL ? "3D" : "Non-3D", pDevice->aDeviceName, pDevice->aDriverName, 0);
#line 2802
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Memory: 0x%x out of 0x%x free\n", pDevice->freeVideoMemory, pDevice->totalVideoMemory, 0, 0);

        IDirectDraw_Release(lpDD);
        ++stdDisplay_numDevices;
        return 1;
    }

    return 0;
}

HRESULT PASCAL stdDisplay_EnumVideoModesCallback(LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext)
{
    StdVideoMode* pMode;
    uint32_t pfflags;
    uint32_t curBitmask;
    int bits;


    if ( stdDisplay_numVideoModes < STD_ARRAYLEN(stdDisplay_aVideoModes) )
    {
        pMode = &stdDisplay_aVideoModes[stdDisplay_numVideoModes];
        pMode->rasterInfo.width   = lpDDSurfaceDesc->dwWidth;
        pMode->rasterInfo.height  = lpDDSurfaceDesc->dwHeight;
        pMode->rasterInfo.rowSize = lpDDSurfaceDesc->lPitch;
        stdDisplay_SetAspectRatio(pMode);

        pfflags = lpDDSurfaceDesc->ddpfPixelFormat.dwFlags;
        if ( (pfflags & DDPF_PALETTEINDEXED8) != 0 )
        {
            pMode->rasterInfo.colorInfo.colorMode = STDCOLOR_PAL;
            pMode->rasterInfo.colorInfo.bpp       = 8;
            pMode->rasterInfo.colorInfo.redBPP    = 0;
            pMode->rasterInfo.colorInfo.greenBPP  = 0;
            pMode->rasterInfo.colorInfo.blueBPP   = 0;
        }
        else if ( (pfflags & DDPF_RGB) != 0 )
        {
            pMode->rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;
            pMode->rasterInfo.colorInfo.bpp       = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

            bits = 0;
            for ( curBitmask = lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
            {
                ++bits;
            }

            pMode->rasterInfo.colorInfo.redPosShift = bits;
            pMode->rasterInfo.colorInfo.redPosShiftRight = stdCalcBitPos(255 / (lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask >> bits));

            bits = 0;
            while ( (curBitmask & 1) != 0 )
            {
                ++bits;
                curBitmask >>= 1;
            }

            pMode->rasterInfo.colorInfo.redBPP = bits;

            bits = 0;
            for ( curBitmask = lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
            {
                ++bits;
            }

            pMode->rasterInfo.colorInfo.greenPosShift = bits;
            pMode->rasterInfo.colorInfo.greenPosShiftRight = stdCalcBitPos(255 / (lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask >> bits));

            bits = 0;
            while ( (curBitmask & 1) != 0 )
            {
                ++bits;
                curBitmask >>= 1;
            }

            pMode->rasterInfo.colorInfo.greenBPP = bits;

            bits = 0;
            for ( curBitmask = lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
            {
                ++bits;
            }

            pMode->rasterInfo.colorInfo.bluePosShift = bits;
            pMode->rasterInfo.colorInfo.bluePosShiftRight = stdCalcBitPos(255 / (lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask >> bits));

            bits = 0;
            while ( (curBitmask & 1) != 0 )
            {
                ++bits;
                curBitmask >>= 1;
            }

            pMode->rasterInfo.colorInfo.blueBPP = bits;

        }

        switch ( pMode->rasterInfo.colorInfo.bpp )
        {
            case 8u:
                pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize;
                break;
            case 16u:
                pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize / 2u;
                break;
            case 24u:
                pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize / 3u;
                break;
            case 32u:
                pMode->rasterInfo.rowWidth = pMode->rasterInfo.rowSize / 4u;
                break;
        }

        pMode->rasterInfo.size = pMode->rasterInfo.width * pMode->rasterInfo.height * (pMode->rasterInfo.colorInfo.bpp / 8);

        {
            size_t freeVRam, totalVRam, freeTexMem, totalTexMem;
            size_t requiredVRam;

            freeVRam = 0;
            totalVRam = 0;
            stdDisplay_GetTotalMemory(&totalVRam, &freeVRam);
            stdDisplay_GetTextureMemory(&totalTexMem, &freeTexMem);

            requiredVRam = 3 * pMode->rasterInfo.size;
#line 2939
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Video Ram Total: %d bytes  Free: %d bytes Required: %d bytes.\n", totalVRam, freeVRam, requiredVRam, 0);
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Texture Ram Total: %d bytes  Free: %d bytes.\n", totalTexMem, freeTexMem, 0, 0);

            if ( requiredVRam > totalVRam )
            {
                if ( pMode->rasterInfo.width > 640 || pMode->rasterInfo.height > 480 || pMode->rasterInfo.colorInfo.bpp != 16 )
                {
#line 2960
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Not enough video RAM for mode %d by %d by %d!\n", pMode->rasterInfo.width, pMode->rasterInfo.height, pMode->rasterInfo.colorInfo.bpp, 0);
                    return 1;
                }

                if ( totalTexMem >= 0x200000 ) {
#line 2950
                    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Allowing <= 640x480x16 with 2MB texture mem.\n", 0, 0, 0, 0);
                }
                else {
#line 2955
                    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Not enough video memory, allowing <= 640x480x16 anyway...\n", 0, 0, 0, 0);
                }
            }
        }

        ++stdDisplay_numVideoModes;
        return 1;
    }

    return 0;
}

void J3DAPI stdDisplay_SetAspectRatio(StdVideoMode* pMode)
{
    if ( pMode->rasterInfo.width == 320 && pMode->rasterInfo.height == 200 )
    {
        pMode->aspectRatio = 0.75f; // 1/1.333f aka 1/ 4:3
    }

    else if ( pMode->rasterInfo.width == 320 && pMode->rasterInfo.height == 400 )
    {
        pMode->aspectRatio = 0.75f;
    }

    else if ( pMode->rasterInfo.width == 640 && pMode->rasterInfo.height == 400 )
    {
        pMode->aspectRatio = 0.75f;
    }
    else
    {
        pMode->aspectRatio = 1.0f;
    }
}

LPDIRECTDRAW4 J3DAPI stdDisplay_GetDirectDraw(void)
{
    return stdDisplay_lpDD;
}

HRESULT J3DAPI stdDisplay_GetDDSurface1(LPDIRECTDRAWSURFACE4 pDDSurf, LPDIRECTDRAWSURFACE* ppDDSurf1)
{
    return IDirectDrawSurface4_QueryInterface(pDDSurf, &IID_IDirectDrawSurface, (LPVOID*)ppDDSurf1);
}

int J3DAPI stdDisplay_SetWindowMode(HWND hWnd, StdVideoMode* pDisplayMode)
{
    HRESULT ddres;
    unsigned int bbpp;
    unsigned int width;
    unsigned int height;
    LPDDSURFACEDESC2 pDesc;
    LPDIRECTDRAWCLIPPER pDDClipper;
    uint32_t mask;
    int rbits;
    int gbits;
    int bbits;

    width  = pDisplayMode->rasterInfo.width;
    height = pDisplayMode->rasterInfo.height;
    if ( !SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE) ) {
        return 0;
    }

    stdDisplay_coopLevelFlags = DDSCL_NORMAL;
    ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, hWnd, stdDisplay_coopLevelFlags);
    if ( ddres != DD_OK )
    {
#line 3047
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting cooperative level.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

    // Front buffer
    stdDisplay_frontBuffer.lockRefCount     = 1;
    stdDisplay_frontBuffer.lockSurfRefCount = 0;
    stdDisplay_frontBuffer.bVideoMemory     = 0;
    stdDisplay_frontBuffer.pPixels          = NULL;

    memset(&stdDisplay_frontBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_frontBuffer.surface.ddSurfDesc));
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwSize         = sizeof(DDSURFACEDESC2);
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwFlags        = DDSD_CAPS;
    stdDisplay_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_frontBuffer.surface.ddSurfDesc, &stdDisplay_frontBuffer.surface.pDDSurf, NULL);
    if ( ddres != DD_OK )
    {
#line 3074
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating the DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    ddres = IDirectDraw4_CreateClipper(stdDisplay_lpDD, 0, &pDDClipper, NULL);
    if ( ddres != DD_OK )
    {
#line 3081
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating DirectDraw clipper.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    ddres = IDirectDrawClipper_SetHWnd(pDDClipper, 0, hWnd);
    if ( ddres != DD_OK )
    {
        IDirectDrawClipper_Release(pDDClipper);
#line 3088
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting clipper hwnd.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    ddres = IDirectDrawSurface4_SetClipper(stdDisplay_frontBuffer.surface.pDDSurf, pDDClipper);
    if ( ddres != DD_OK )
    {
        IDirectDrawClipper_Release(pDDClipper);
#line 3095
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when attaching clipper to primary surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    IDirectDrawClipper_Release(pDDClipper);

    ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_frontBuffer.surface.pDDSurf, &stdDisplay_frontBuffer.surface.ddSurfDesc);
    if ( ddres != DD_OK )
    {
#line 3112
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting desc of DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    pDesc = &stdDisplay_frontBuffer.surface.ddSurfDesc;
    pDisplayMode->rasterInfo.width   = pDesc->dwWidth;
    pDisplayMode->rasterInfo.height  = pDesc->dwHeight;
    pDisplayMode->rasterInfo.rowSize = pDesc->lPitch;
    stdDisplay_SetAspectRatio(pDisplayMode);

    if ( ((pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) | (pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) | (pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED2) | (pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED1)) != 0 ) {
        return 0;
    }

    if ( (pDesc->ddpfPixelFormat.dwFlags & DDPF_RGB) != 0 )
    {
        pDisplayMode->rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;
        pDisplayMode->rasterInfo.colorInfo.bpp       = pDesc->ddpfPixelFormat.dwRGBBitCount;
        for ( rbits = 0, mask = pDesc->ddpfPixelFormat.dwRBitMask; (mask & 1) == 0; ++rbits, mask >>= 1 )
        {
            ;
        }

        pDisplayMode->rasterInfo.colorInfo.redPosShift = rbits;
        pDisplayMode->rasterInfo.colorInfo.redPosShiftRight = stdCalcBitPos(255 / (pDesc->ddpfPixelFormat.dwRBitMask >> rbits));

        for ( rbits = 0; (mask & 1) != 0; ++rbits, mask >>= 1 )
        {
            ;
        }

        pDisplayMode->rasterInfo.colorInfo.redBPP = rbits;

        for ( gbits = 0, mask = pDesc->ddpfPixelFormat.dwGBitMask; (mask & 1) == 0; ++gbits, mask >>= 1 )
        {
            ;
        }

        pDisplayMode->rasterInfo.colorInfo.greenPosShift = gbits;
        pDisplayMode->rasterInfo.colorInfo.greenPosShiftRight = stdCalcBitPos(255 / (pDesc->ddpfPixelFormat.dwGBitMask >> gbits));

        for ( gbits = 0; (mask & 1) != 0; ++gbits, mask >>= 1 )
        {
            ;
        }

        pDisplayMode->rasterInfo.colorInfo.greenBPP = gbits;

        for ( bbits = 0, mask = pDesc->ddpfPixelFormat.dwBBitMask; (mask & 1) == 0; ++bbits, mask >>= 1 )
        {
            ;
        }

        pDisplayMode->rasterInfo.colorInfo.bluePosShift = bbits;
        pDisplayMode->rasterInfo.colorInfo.bluePosShiftRight = stdCalcBitPos(255 / (pDesc->ddpfPixelFormat.dwBBitMask >> bbits));

        for ( bbits = 0; (mask & 1) != 0; ++bbits, mask >>= 1 )
        {
            ;
        }

        pDisplayMode->rasterInfo.colorInfo.blueBPP = bbits;
    }

    switch ( pDisplayMode->rasterInfo.colorInfo.bpp )
    {
        case 8u:
            pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize;
            break;

        case 16u:
            pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize / 2u;
            break;

        case 24u:
            pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize / 3u;
            break;

        case 32u:
            pDisplayMode->rasterInfo.rowWidth = pDisplayMode->rasterInfo.rowSize / 4u;
            break;
    }

    pDisplayMode->rasterInfo.size = pDisplayMode->rasterInfo.width * pDisplayMode->rasterInfo.height * (pDisplayMode->rasterInfo.colorInfo.bpp / 8);
    bbpp = pDisplayMode->rasterInfo.colorInfo.bpp / 8;

    stdDisplay_frontBuffer.rasterInfo = pDisplayMode->rasterInfo;
    stdDisplay_frontBuffer.rasterInfo.rowSize  = stdDisplay_frontBuffer.surface.ddSurfDesc.lPitch;
    stdDisplay_frontBuffer.rasterInfo.rowWidth = stdDisplay_frontBuffer.rasterInfo.rowSize / bbpp;

    if ( (stdDisplay_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
        stdDisplay_frontBuffer.bVideoMemory = 1;
    }

    // Back buffer
    memset(&stdDisplay_backBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_backBuffer.surface.ddSurfDesc));
    stdDisplay_backBuffer.surface.ddSurfDesc.dwSize         = sizeof(DDSURFACEDESC2);
    stdDisplay_backBuffer.surface.ddSurfDesc.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    stdDisplay_backBuffer.surface.ddSurfDesc.dwWidth        = width;
    stdDisplay_backBuffer.surface.ddSurfDesc.dwHeight       = height;
    stdDisplay_backBuffer.surface.ddSurfDesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN;

    ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_backBuffer.surface.ddSurfDesc, &stdDisplay_backBuffer.surface.pDDSurf, NULL);
    if ( ddres != DD_OK )
    {
#line 3204
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating off-screen back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    stdDisplay_backBuffer.lockRefCount     = 1;
    stdDisplay_backBuffer.lockSurfRefCount = 0;
    stdDisplay_backBuffer.bVideoMemory     = 1;

    ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_backBuffer.surface.pDDSurf, &stdDisplay_backBuffer.surface.ddSurfDesc);
    if ( ddres != DD_OK )
    {
#line 3214
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting desc of back buffer rendering surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    pDesc = &stdDisplay_backBuffer.surface.ddSurfDesc;
    stdDisplay_backBuffer.rasterInfo.width   = pDesc->dwWidth;
    stdDisplay_backBuffer.rasterInfo.height  = pDesc->dwHeight;
    stdDisplay_backBuffer.rasterInfo.rowSize = pDesc->lPitch;

    if ( ((pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) | (pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) | (pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED2) | (pDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED1)) != 0 ) {
        return 0;
    }

    if ( (pDesc->ddpfPixelFormat.dwFlags & DDPF_RGB) != 0 )
    {
        stdDisplay_backBuffer.rasterInfo.colorInfo.colorMode = STDCOLOR_RGB;
        stdDisplay_backBuffer.rasterInfo.colorInfo.bpp       = pDesc->ddpfPixelFormat.dwRGBBitCount;
        for ( rbits = 0, mask = pDesc->ddpfPixelFormat.dwRBitMask; (mask & 1) == 0; ++rbits, mask >>= 1 )
        {
            ;
        }

        stdDisplay_backBuffer.rasterInfo.colorInfo.redPosShift = rbits;
        stdDisplay_backBuffer.rasterInfo.colorInfo.redPosShiftRight = stdCalcBitPos(255 / (pDesc->ddpfPixelFormat.dwRBitMask >> rbits));

        for ( rbits = 0; (mask & 1) != 0; ++rbits, mask >>= 1 )
        {
            ;
        }

        stdDisplay_backBuffer.rasterInfo.colorInfo.redBPP = rbits;

        for ( gbits = 0, mask = pDesc->ddpfPixelFormat.dwGBitMask; (mask & 1) == 0; ++gbits, mask >>= 1 )
        {
            ;
        }

        stdDisplay_backBuffer.rasterInfo.colorInfo.greenPosShift = gbits;
        stdDisplay_backBuffer.rasterInfo.colorInfo.greenPosShiftRight = stdCalcBitPos(255 / (pDesc->ddpfPixelFormat.dwGBitMask >> gbits));

        for ( gbits = 0; (mask & 1) != 0; ++gbits, mask >>= 1 )
        {
            ;
        }

        stdDisplay_backBuffer.rasterInfo.colorInfo.greenBPP = gbits;

        for ( bbits = 0, mask = pDesc->ddpfPixelFormat.dwBBitMask; (mask & 1) == 0; ++bbits, mask >>= 1 )
        {
            ;
        }

        stdDisplay_backBuffer.rasterInfo.colorInfo.bluePosShift = bbits;
        stdDisplay_backBuffer.rasterInfo.colorInfo.bluePosShiftRight = stdCalcBitPos(255 / (pDesc->ddpfPixelFormat.dwBBitMask >> bbits));

        for ( bbits = 0; (mask & 1) != 0; ++bbits, mask >>= 1 )
        {
            ;
        }

        stdDisplay_backBuffer.rasterInfo.colorInfo.blueBPP = bbits;
    }

    stdDisplay_backBuffer.rasterInfo.rowSize  = stdDisplay_backBuffer.surface.ddSurfDesc.lPitch;
    stdDisplay_backBuffer.rasterInfo.rowWidth = stdDisplay_backBuffer.rasterInfo.rowSize / bbpp;
    stdDisplay_coopLevelFlags = DDSCL_NORMAL;

#line 3259
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Primary buffer in %s memory.\n", stdDisplay_frontBuffer.bVideoMemory ? "VIDEO" : "SYSTEM", 0, 0, 0);
#line 3260
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Back buffer in %s memory.\n", stdDisplay_backBuffer.bVideoMemory ? "VIDEO" : "SYSTEM", 0, 0, 0);
    return 1;
}

int J3DAPI stdDisplay_SetFullscreenMode(HWND hwnd, const StdVideoMode* pDisplayMode, size_t numBackBuffers)
{
    unsigned int bbpp;
    HRESULT ddres;

    stdDisplay_coopLevelFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
    ddres = IDirectDraw4_SetCooperativeLevel(stdDisplay_lpDD, hwnd, stdDisplay_coopLevelFlags);
    if ( ddres != DD_OK )
    {
#line 3289
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting cooperative level.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

#line 3294
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Set video mode %d %d %d.\n", pDisplayMode->rasterInfo.width, pDisplayMode->rasterInfo.height, pDisplayMode->rasterInfo.colorInfo.bpp, 0);

    ddres = IDirectDraw4_SetDisplayMode(stdDisplay_lpDD, pDisplayMode->rasterInfo.width, pDisplayMode->rasterInfo.height, pDisplayMode->rasterInfo.colorInfo.bpp, 0, 0);
    if ( ddres != DD_OK )
    {
#line 3299
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting video mode.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

    // Setup front buffer
    stdDisplay_frontBuffer.lockRefCount     = 1;
    stdDisplay_frontBuffer.lockSurfRefCount = 0;
    stdDisplay_frontBuffer.bVideoMemory     = 0;
    memcpy(&stdDisplay_frontBuffer.rasterInfo, &pDisplayMode->rasterInfo, sizeof(stdDisplay_frontBuffer.rasterInfo));
    stdDisplay_frontBuffer.pPixels          = NULL;

    memset(&stdDisplay_frontBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_frontBuffer.surface.ddSurfDesc));
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwSize            = sizeof(DDSURFACEDESC2);
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwWidth           = pDisplayMode->rasterInfo.width;
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwHeight          = pDisplayMode->rasterInfo.height;
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwBackBufferCount = numBackBuffers;
    stdDisplay_frontBuffer.surface.ddSurfDesc.dwFlags           = DDSD_BACKBUFFERCOUNT | DDSD_CAPS;
    stdDisplay_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps    = DDSCAPS_3DDEVICE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;

    ddres = IDirectDraw4_CreateSurface(stdDisplay_lpDD, &stdDisplay_frontBuffer.surface.ddSurfDesc, &stdDisplay_frontBuffer.surface.pDDSurf, NULL);
    if ( ddres != DD_OK )
    {
#line 3327
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating the DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

    ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_frontBuffer.surface.pDDSurf, &stdDisplay_frontBuffer.surface.ddSurfDesc);
    if ( ddres != DD_OK )
    {
#line 3335
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting desc of DirectDraw primary surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

    bbpp = (unsigned int)pDisplayMode->rasterInfo.colorInfo.bpp / 8;
    stdDisplay_frontBuffer.rasterInfo.rowSize  = stdDisplay_frontBuffer.surface.ddSurfDesc.lPitch;
    stdDisplay_frontBuffer.rasterInfo.rowWidth = stdDisplay_frontBuffer.surface.ddSurfDesc.lPitch / bbpp;

    if ( (stdDisplay_frontBuffer.surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
        stdDisplay_frontBuffer.bVideoMemory = 1;
    }

    // Setup back buffer
    stdDisplay_backBuffer.lockRefCount     = 1;
    stdDisplay_backBuffer.lockSurfRefCount = 0;
    stdDisplay_backBuffer.bVideoMemory     = 0;
    memcpy(&stdDisplay_backBuffer.rasterInfo, &pDisplayMode->rasterInfo, sizeof(stdDisplay_backBuffer.rasterInfo));
    stdDisplay_backBuffer.pPixels          = NULL;

    memset(&stdDisplay_backBuffer.surface.ddSurfDesc, 0, sizeof(stdDisplay_backBuffer.surface.ddSurfDesc));
    stdDisplay_backBuffer.surface.ddSurfDesc.dwSize            = sizeof(DDSURFACEDESC2);
    stdDisplay_backBuffer.surface.ddSurfDesc.dwWidth           = pDisplayMode->rasterInfo.width;
    stdDisplay_backBuffer.surface.ddSurfDesc.dwHeight          = pDisplayMode->rasterInfo.height;
    stdDisplay_backBuffer.surface.ddSurfDesc.dwFlags           = DDSD_BACKBUFFERCOUNT | DDSD_CAPS;
    stdDisplay_backBuffer.surface.ddSurfDesc.ddsCaps.dwCaps    = DDSCAPS_3DDEVICE | DDSCAPS_BACKBUFFER;
    stdDisplay_backBuffer.surface.ddSurfDesc.dwBackBufferCount = numBackBuffers;

    ddres = IDirectDrawSurface4_GetAttachedSurface(
        stdDisplay_frontBuffer.surface.pDDSurf,
        &stdDisplay_backBuffer.surface.ddSurfDesc.ddsCaps,
        &stdDisplay_backBuffer.surface.pDDSurf
    );
    if ( ddres != DD_OK )
    {
#line 3366
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating the DirectDraw backbuffer surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

    ddres = IDirectDrawSurface4_GetSurfaceDesc(stdDisplay_backBuffer.surface.pDDSurf, &stdDisplay_backBuffer.surface.ddSurfDesc);
    if ( ddres != DD_OK )
    {
#line 3374
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting desc of DirectDraw back surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        return ddres;
    }

    if ( (stdDisplay_backBuffer.surface.ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) != 0 ) {
        stdDisplay_backBuffer.bVideoMemory = 1;
    }

    stdDisplay_backBuffer.rasterInfo.rowSize  = stdDisplay_backBuffer.surface.ddSurfDesc.lPitch;
    stdDisplay_backBuffer.rasterInfo.rowWidth = stdDisplay_backBuffer.surface.ddSurfDesc.lPitch / bbpp;

#line 3384
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Primary buffer in %s memory.\n", stdDisplay_frontBuffer.bVideoMemory ? "VIDEO" : "SYSTEM", 0, 0, 0);
#line 3385
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Back buffer in %s memory.\n", stdDisplay_backBuffer.bVideoMemory ? "VIDEO" : "SYSTEM", 0, 0, 0);
    return 0;
}

void J3DAPI stdDisplay_ReleaseBuffers(void)
{
    if ( stdDisplay_zBuffer.pDDSurf )
    {
        IDirectDrawSurface4_DeleteAttachedSurface(stdDisplay_backBuffer.surface.pDDSurf, 0, stdDisplay_zBuffer.pDDSurf);
        IDirectDrawSurface4_Release(stdDisplay_zBuffer.pDDSurf);
    }

    if ( stdDisplay_frontBuffer.surface.pDDSurf ) {
        IDirectDrawSurface4_Release(stdDisplay_frontBuffer.surface.pDDSurf);
    }

    memset(&stdDisplay_zBuffer, 0, sizeof(stdDisplay_zBuffer));
    memset(&stdDisplay_backBuffer, 0, sizeof(stdDisplay_backBuffer));
    memset(&stdDisplay_frontBuffer, 0, sizeof(stdDisplay_frontBuffer));
}

uint8_t* J3DAPI stdDisplay_LockSurface(tVSurface* pVSurf)
{
    while ( 1 )
    {
        HRESULT ddres = IDirectDrawSurface4_Lock(pVSurf->pDDSurf, NULL, &pVSurf->ddSurfDesc, DDLOCK_WAIT, NULL);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            if ( (pVSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pVSurf->pDDSurf);
            }
        }

        if ( ddres != DD_OK )
        {
#line 3456
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when locking the DD buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 0;
        }
    }

    return (uint8_t*)pVSurf->ddSurfDesc.lpSurface;
}

int J3DAPI stdDisplay_UnlockSurface(tVSurface* pSurf)
{
    while ( 1 )
    {
        HRESULT ddres = IDirectDrawSurface4_Unlock(pSurf->pDDSurf, 0);
        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            if ( (pSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            }
            else {
                ddres = IDirectDrawSurface4_Restore(pSurf->pDDSurf);
            }
        }

        if ( ddres != DD_OK )
        {
#line 3508
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when unlocking the display buffer surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdDisplay_Update(void)
{
    HRESULT ddres;
    HWND hwnd;
    RECT srcRect;

    while ( 1 )
    {
        if ( stdDisplay_bFullscreen )
        {
            ddres = IDirectDrawSurface4_Flip(stdDisplay_frontBuffer.surface.pDDSurf, NULL, 0);
        }
        else
        {
            RECT dstRect;
            POINT screenPos;

            hwnd = stdWin95_GetWindow();
            screenPos.x = screenPos.y = 0;
            ClientToScreen(hwnd, &screenPos);
            dstRect.top = screenPos.y;
            dstRect.left = screenPos.x;
            dstRect.right = screenPos.x + stdDisplay_backBuffer.rasterInfo.width;
            dstRect.bottom = screenPos.y + stdDisplay_backBuffer.rasterInfo.height;
            srcRect.top = srcRect.left = 0;
            srcRect.bottom = stdDisplay_backBuffer.rasterInfo.height;
            srcRect.right = stdDisplay_backBuffer.rasterInfo.width;

            ddres = IDirectDrawSurface4_Blt(stdDisplay_frontBuffer.surface.pDDSurf, &dstRect, stdDisplay_backBuffer.surface.pDDSurf, &srcRect, DDBLT_WAIT, NULL);
        }

        if ( ddres == DD_OK ) {
            break;
        }

        if ( ddres == DDERR_SURFACELOST )
        {
            ddres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            if ( ddres != DD_OK )
            {
#line 3556
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when flipping the page.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                return 1;
            }

            if ( !stdDisplay_bFullscreen )
            {
                ddres = IDirectDrawSurface4_Restore(stdDisplay_backBuffer.surface.pDDSurf);
                if ( ddres != DD_OK )
                {
#line 3565
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when flipping the page.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
                    return 1;
                }
            }
        }

        if ( ddres != DDERR_WASSTILLDRAWING && ddres != DD_OK )
        {
#line 3573
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when flipping the page.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdDisplay_ColorFillSurface(tVSurface* pSurf, uint32_t dwFillColor, const StdRect* pRect)
{
    HRESULT hDdres;
    DDBLTFX ddbltfx;
    RECT currentrect;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize      = sizeof(DDBLTFX);
    ddbltfx.dwFillColor = dwFillColor;

    if ( pRect )
    {
        if ( pRect->right == 0 ) {
            return 1;
        }

        if ( pRect->bottom == 0 ) {
            return 1;
        }

        currentrect.left   = pRect->left;
        currentrect.top    = pRect->top;
        currentrect.right  = pRect->left + pRect->right;
        currentrect.bottom = pRect->top + pRect->bottom;
    }
    else
    {
        currentrect.left   = 0;
        currentrect.top    = 0;
        currentrect.right  = pSurf->ddSurfDesc.dwWidth;
        currentrect.bottom = pSurf->ddSurfDesc.dwHeight;
    }

    while ( 1 )
    {
        hDdres = IDirectDrawSurface4_Blt(pSurf->pDDSurf, &currentrect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &ddbltfx);
        if ( hDdres == DD_OK ) {
            break;
        }

        if ( hDdres == DDERR_SURFACELOST )
        {
            if ( (pSurf->ddSurfDesc.ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) != 0 ) {
                hDdres = IDirectDrawSurface4_Restore(stdDisplay_frontBuffer.surface.pDDSurf);
            }
            else {
                hDdres = IDirectDrawSurface4_Restore(pSurf->pDDSurf);
            }
        }

        if ( hDdres != DD_OK )
        {
#line 3650
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when color filling the surface.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
            return 1;
        }
    }

    return 0;
}

int J3DAPI stdDisplay_BackBufferFill(uint32_t color, const StdRect* pRect)
{
    return stdDisplay_ColorFillSurface(&stdDisplay_backBuffer.surface, color, pRect);
}

int J3DAPI stdDisplay_SaveScreen(const char* pFilename)
{
    return stdBmp_WriteVBuffer(pFilename, &stdDisplay_backBuffer);
}

void J3DAPI stdDisplay_SetDefaultResolution(uint32_t width, uint32_t height)
{
    stdDisplay_primaryVideoMode.rasterInfo.width  = width;
    stdDisplay_primaryVideoMode.rasterInfo.height = height;
}

void J3DAPI stdDisplay_GetBackBufferSize(uint32_t* pWidth, uint32_t* pHeight)
{
    *pWidth = stdDisplay_backBuffer.rasterInfo.width;
    *pHeight = stdDisplay_backBuffer.rasterInfo.height;
}

size_t J3DAPI stdDisplay_GetNumVideoModes(void)
{
    return stdDisplay_numVideoModes;
}

int J3DAPI stdDisplay_GetVideoMode(size_t modeNum, StdVideoMode* pDestMode)
{
    if ( modeNum < stdDisplay_numVideoModes )
    {
        memcpy(pDestMode, &stdDisplay_aVideoModes[modeNum], sizeof(StdVideoMode));
        return 0;
    }

    return 1;
}

int J3DAPI stdDisplay_GetCurrentVideoMode(StdVideoMode* pDisplayMode)
{
    if ( stdDisplay_pCurVideoMode )
    {
        memcpy(pDisplayMode, stdDisplay_pCurVideoMode, sizeof(StdVideoMode));
        return 0;
    }

    return 1;
}

HDC J3DAPI stdDisplay_GetFrontBufferDC(void)
{
    HDC hdc;
    HRESULT ddres;

    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        ddres = IDirectDrawSurface4_GetDC(stdDisplay_frontBuffer.surface.pDDSurf, &hdc);
        if ( ddres != DD_OK )
        {
#line 3721
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting DC of back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 0;
        }

        return hdc;
    }

    return 0;
}

void J3DAPI stdDisplay_ReleaseFrontBufferDC(HDC hdc)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        HRESULT ddres = IDirectDrawSurface4_ReleaseDC(stdDisplay_frontBuffer.surface.pDDSurf, hdc);
        if ( ddres != DD_OK ) {
#line 3739
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when releasing DC of back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        }
    }
}

HDC J3DAPI stdDisplay_GetBackBufferDC(void)
{
    HDC hdc;
    HRESULT ddres;

    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        ddres = IDirectDrawSurface4_GetDC(stdDisplay_backBuffer.surface.pDDSurf, &hdc);
        if ( ddres != DD_OK )
        {
#line 3756
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting DC of back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
            return 0;
        }

        return hdc;
    }

    return 0;
}

void J3DAPI stdDisplay_ReleaseBackBufferDC(HDC hdc)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        HRESULT ddres = IDirectDrawSurface4_ReleaseDC(stdDisplay_backBuffer.surface.pDDSurf, hdc);
        if ( ddres != DD_OK ) {
#line 3774
#line 3774
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when releasing DC of back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        }
    }
}

int J3DAPI stdDisplay_FlipToGDISurface(void)
{
    HRESULT ddres = IDirectDraw4_FlipToGDISurface(stdDisplay_lpDD); /* retail bug: no return value; the debug build is the same */
    if ( ddres != DD_OK )
    {
#line 3788
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when flipping to GDI surface.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
    }
}

int J3DAPI stdDisplay_CanRenderWindowed(void)
{
    HRESULT ddres;
    DDCAPS ddcaps;

    if ( !stdDisplay_lpDD ) {
        return -1;
    }

    memset(&ddcaps, 0, sizeof(ddcaps));
    ddcaps.dwSize = sizeof(DDCAPS);
    ddres = IDirectDraw4_GetCaps(stdDisplay_lpDD, &ddcaps, 0);
    if ( ddres == DD_OK ) {
        return ddcaps.dwCaps2 & DDCAPS2_CANRENDERWINDOWED;
    }

    return -1;
}


int J3DAPI stdDisplay_SetBufferClipper(int bFrontBuffer)
{
    HRESULT hDdres;
    IDirectDrawClipper* pClipper;

    if ( !stdDisplay_bFullscreen ) {
        return 0;
    }

    if ( bFrontBuffer )
    {
        IDirectDrawSurface4_GetClipper(stdDisplay_frontBuffer.surface.pDDSurf, &pClipper);
        if ( pClipper ) {
            return 1; // Clipper already set for buffer
        }
    }
    else
    {
        IDirectDrawSurface4_GetClipper(stdDisplay_backBuffer.surface.pDDSurf, &pClipper);
        if ( pClipper ) {
            return 1; // Clipper already set for buffer
        }
    }

    hDdres = IDirectDraw4_CreateClipper(stdDisplay_lpDD, 0, &pClipper, 0);
    if ( hDdres != DD_OK )
    {
#line 3838
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating DirectDraw clipper.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }

    hDdres = IDirectDrawClipper_SetHWnd(pClipper, 0, stdWin95_GetWindow());
    if ( hDdres != DD_OK )
    {
        IDirectDrawClipper_Release(pClipper);
#line 3846
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting clipper hwnd.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }

    if ( bFrontBuffer ) {
        hDdres = IDirectDrawSurface4_SetClipper(stdDisplay_frontBuffer.surface.pDDSurf, pClipper);
    }
    else {
        hDdres = IDirectDrawSurface4_SetClipper(stdDisplay_backBuffer.surface.pDDSurf, pClipper);
    }

    if ( hDdres != DD_OK )
    {
        IDirectDrawClipper_Release(pClipper);
#line 3858
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when attaching clipper to primary surface.\n", stdDisplay_DDGetStatus(hDdres), 0, 0, 0);
        return 0;
    }

    IDirectDrawClipper_Release(pClipper);
    return 0;
}

HRESULT J3DAPI stdDisplay_RemoveBufferClipper(int bFrontBuffer)
{
    if ( !stdDisplay_bFullscreen )
    {
        return; /* retail bug: no return value here or at the end; the debug build is the same */
    }

    if ( bFrontBuffer ) {
        IDirectDrawSurface4_SetClipper(stdDisplay_frontBuffer.surface.pDDSurf, NULL);
    }
    else {
        IDirectDrawSurface4_SetClipper(stdDisplay_backBuffer.surface.pDDSurf, NULL);
    }
}

int J3DAPI stdDisplay_CopyFrontToBack(const POINT* pOffset, StdRect* pRect)
{
    StdRect rect;

    if ( !pRect || !stdDisplay_frontBuffer.surface.pDDSurf || !stdDisplay_backBuffer.surface.pDDSurf ) {
        return 0;
    }

    pRect->left   = pRect->left > 0 ? pRect->left : 0;
    pRect->right  = pRect->right < (int)stdDisplay_backBuffer.rasterInfo.width - 1 ? pRect->right : (int)stdDisplay_backBuffer.rasterInfo.width - 1;
    pRect->top    = pRect->top > 0 ? pRect->top : 0;
    pRect->bottom = pRect->bottom < (int)stdDisplay_backBuffer.rasterInfo.height - 1 ? pRect->bottom : (int)stdDisplay_backBuffer.rasterInfo.height - 1;

    rect.left   = pRect->left;
    rect.top    = pRect->top;
    rect.right  = pRect->right - pRect->left + 1;
    rect.bottom = pRect->bottom - pRect->top + 1;
    return stdDisplay_BlitSurfaceToSurface(&stdDisplay_backBuffer, &stdDisplay_frontBuffer, pOffset->x + rect.left, pOffset->y + rect.top, &rect, 0);
}

int J3DAPI stdDisplay_CopyBackToFront(const POINT* pOffset, StdRect* pRect)
{
    StdRect rect;

    if ( !pRect || !stdDisplay_frontBuffer.surface.pDDSurf || !stdDisplay_backBuffer.surface.pDDSurf ) {
        return 0;
    }

    pRect->left   = pRect->left > 0 ? pRect->left : 0;
    pRect->right  = pRect->right < (int)stdDisplay_backBuffer.rasterInfo.width - 1 ? pRect->right : (int)stdDisplay_backBuffer.rasterInfo.width - 1;
    pRect->top    = pRect->top > 0 ? pRect->top : 0;
    pRect->bottom = pRect->bottom < (int)stdDisplay_backBuffer.rasterInfo.height - 1 ? pRect->bottom : (int)stdDisplay_backBuffer.rasterInfo.height - 1;

    rect.left   = pRect->left;
    rect.top    = pRect->top;
    rect.right  = pRect->right - pRect->left + 1;
    rect.bottom = pRect->bottom - pRect->top + 1;
    return stdDisplay_BlitSurfaceToSurface(&stdDisplay_frontBuffer, &stdDisplay_backBuffer, pRect->left + pOffset->x, pRect->top + pOffset->y, &rect, 0);
}

int J3DAPI stdDisplay_IsFullscreen(void)
{
    return stdDisplay_bFullscreen;
}

float J3DAPI stdDisplay_GetAspectRatio(void)
{
    return stdDisplay_pCurVideoMode->aspectRatio;
}

int J3DAPI stdDisplay_LockBackBuffer(void** pSurface, uint32_t* pWidth, uint32_t* pHeight, int32_t* pPitch)
{
    HRESULT ddres;
    DDSURFACEDESC2 ddsd;

    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ddres = IDirectDrawSurface4_Lock(stdDisplay_backBuffer.surface.pDDSurf, NULL, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WRITEONLY | DDLOCK_WAIT, NULL);
        if ( ddres == DD_OK )
        {
            *pWidth   = ddsd.dwWidth;
            *pHeight  = ddsd.dwHeight;
            *pPitch   = ddsd.lPitch;
            *pSurface = ddsd.lpSurface;
            return 0;
        }

#line 3958
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when locking back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
    }

    return 1;
}

void J3DAPI stdDisplay_UnlockBackBuffer(void)
{
    if ( stdDisplay_bOpen && stdDisplay_bModeSet )
    {
        HRESULT ddres = IDirectDrawSurface4_Unlock(stdDisplay_backBuffer.surface.pDDSurf, 0);
        if ( ddres != DD_OK ) {
#line 3973
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when unlocking back buffer.\n", stdDisplay_DDGetStatus(ddres), 0, 0, 0);
        }
    }
}

uint32_t J3DAPI stdDisplay_EncodeFromRGB565(uint16_t pixel)
{
    ColorInfo ci565 = { STDCOLOR_RGB, 16, 5, 6, 5, 11, 5, 0, 3, 2, 3, 0, 0, 0 };
    ColorInfo colorInfo;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint8_t blue;
    uint8_t red;
    uint8_t green;
    uint32_t color;

    colorInfo = stdDisplay_pCurVideoMode->rasterInfo.colorInfo;
    redMask   = 0xFFFFFFFF >> (32 - ci565.redBPP);
    greenMask = 0xFFFFFFFF >> (32 - ci565.greenBPP);
    blueMask  = 0xFFFFFFFF >> (32 - ci565.blueBPP);

    red = ((pixel >> ci565.redPosShift) & redMask) << 3;
    if ( (red & 8) != 0 )
    {
        red |= 7;
    }

    green = ((pixel >> ci565.greenPosShift) & greenMask) << 2;
    if ( (green & 4) != 0 )
    {
        green |= 3;
    }

    blue = ((pixel >> ci565.bluePosShift) & blueMask) << 3;
    if ( (blue & 8) != 0 )
    {
        blue |= 7;
    }

    color = (red >> colorInfo.redPosShiftRight << colorInfo.redPosShift)
        | (green >> colorInfo.greenPosShiftRight << colorInfo.greenPosShift)
        | (blue >> colorInfo.bluePosShiftRight << colorInfo.bluePosShift);
    return color;
}
