#include "std3D.h"
#include "stdDisplay.h"

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <math.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(std3D_countfit_1, 15)
CF_PROTO(std3D_countfit_2, 15)
CF_PROTO(std3D_countfit_3, 15)
CF_PROTO(std3D_countfit_4, 15)
CF_PROTO(std3D_countfit_5, 15)
CF_PROTO(std3D_countfit_6, 13)
/* end count-fit */

#define STD3D_DEFAULT_MAX_VERTICES 512

float std3D_fogDensity = 1.0f;
static Std3DMipmapFilterType std3D_mipmapFilter = -1;
static int std3D_bRenderFog    = 1;
static size_t std3D_frameCount = 1;

static size_t std3D_curDevice                    = 0;
static size_t std3D_RGBATextureFormat            = 0;
static float std3D_zDepth                        = 0.0f;
static size_t std3D_RGBTextureFormat             = 0;
static size_t std3D_RGBAKeyTextureFormat         = 0;
static float std3D_fogStartDepth                 = 0.0f;
static size_t std3D_numDevices                   = 0;
static StdTextureFormat std3D_aTextureFormats[8] = { 0 };
static float std3D_fogDepthFactor                = 0.0f;
static Device3D std3D_aDevices[4]                = { 0 };
static Std3DRenderState std3D_renderState        = 0;
static D3DRECT std3D_activeRect                  = { 0 };
static float std3D_fogEndDepth                   = 0.0f;
static Device3D* std3D_pCurDevice                = NULL;
static size_t std3D_numTextureFormats            = 0;
static int std3D_bHasRGBTextureFormat            = 0;
static int std3D_bFogTable                       = 0;
size_t std3D_maxVertices                         = 0;
static LPDIRECT3DTEXTURE2 std3D_pD3DTex          = NULL;
static int std3D_bFindAllD3Devices               = 0;
static size_t std3D_numCachedTextures            = 0;
static tSystemTexture* std3D_pFirstTexCache      = NULL;
static tSystemTexture* std3D_pLastTexCache       = NULL;
static LPDIRECTDRAW4 std3D_lpDD                  = NULL;
static LPDIRECTDRAWPALETTE std3D_pDDPalette      = NULL;
static LPDIRECT3D3 std3D_pDirect3D               = NULL;
static LPDIRECT3DDEVICE3 std3D_pD3Device         = NULL;
static LPDIRECT3DVIEWPORT3 std3D_lpD3DViewPort   = NULL;
static int bStartup                              = 0;
static int std3D_bOpen                           = 0;

static const ColorInfo std3D_cfRGB565   = { STDCOLOR_RGB,  16, 5, 6, 5, 11,  5, 0, 3, 2, 3, 0, 0, 0 };
static const ColorInfo std3D_cfRGB5551  = { STDCOLOR_RGBA, 16, 5, 5, 5, 11,  6, 1, 3, 3, 3, 1, 0, 7 };
static const ColorInfo std3D_cfRGB4444  = { STDCOLOR_RGBA, 16, 4, 4, 4, 12,  8, 4, 4, 4, 4, 4, 0, 4 };

static const DXStatus std3D_aD3DStatusTbl[186] =
{
  { D3D_OK,                                "D3D_OK" },
  { D3DERR_BADMAJORVERSION,                 "D3DERR_BADMAJORVERSION" },
  { D3DERR_BADMINORVERSION,                 "D3DERR_BADMINORVERSION" },
  { D3DERR_INVALID_DEVICE,                  "D3DERR_INVALID_DEVICE" },
  { D3DERR_INITFAILED,                      "D3DERR_INITFAILED" },
  { D3DERR_DEVICEAGGREGATED,                "D3DERR_DEVICEAGGREGATED" },
  { D3DERR_EXECUTE_CREATE_FAILED,           "D3DERR_EXECUTE_CREATE_FAILED" },
  { D3DERR_EXECUTE_DESTROY_FAILED,          "D3DERR_EXECUTE_DESTROY_FAILED" },
  { D3DERR_EXECUTE_LOCK_FAILED,             "D3DERR_EXECUTE_LOCK_FAILED" },
  { D3DERR_EXECUTE_UNLOCK_FAILED,           "D3DERR_EXECUTE_UNLOCK_FAILED" },
  { D3DERR_EXECUTE_LOCKED,                  "D3DERR_EXECUTE_LOCKED" },
  { D3DERR_EXECUTE_NOT_LOCKED,              "D3DERR_EXECUTE_NOT_LOCKED" },
  { D3DERR_EXECUTE_FAILED,                  "D3DERR_EXECUTE_FAILED" },
  { D3DERR_EXECUTE_CLIPPED_FAILED,          "D3DERR_EXECUTE_CLIPPED_FAILED" },
  { D3DERR_TEXTURE_NO_SUPPORT,              "D3DERR_TEXTURE_NO_SUPPORT" },
  { D3DERR_TEXTURE_CREATE_FAILED,           "D3DERR_TEXTURE_CREATE_FAILED" },
  { D3DERR_TEXTURE_DESTROY_FAILED,          "D3DERR_TEXTURE_DESTROY_FAILED" },
  { D3DERR_TEXTURE_LOCK_FAILED,             "D3DERR_TEXTURE_LOCK_FAILED" },
  { D3DERR_TEXTURE_UNLOCK_FAILED,           "D3DERR_TEXTURE_UNLOCK_FAILED" },
  { D3DERR_TEXTURE_LOAD_FAILED,             "D3DERR_TEXTURE_LOAD_FAILED" },
  { D3DERR_TEXTURE_SWAP_FAILED,             "D3DERR_TEXTURE_SWAP_FAILED" },
  { D3DERR_TEXTURE_LOCKED,                  "D3DERR_TEXTURE_LOCKED" },
  { D3DERR_TEXTURE_NOT_LOCKED,              "D3DERR_TEXTURE_NOT_LOCKED" },
  { D3DERR_TEXTURE_GETSURF_FAILED,          "D3DERR_TEXTURE_GETSURF_FAILED" },
  { D3DERR_MATRIX_CREATE_FAILED,            "D3DERR_MATRIX_CREATE_FAILED" },
  { D3DERR_MATRIX_DESTROY_FAILED,           "D3DERR_MATRIX_DESTROY_FAILED" },
  { D3DERR_MATRIX_SETDATA_FAILED,           "D3DERR_MATRIX_SETDATA_FAILED" },
  { D3DERR_MATRIX_GETDATA_FAILED,           "D3DERR_MATRIX_GETDATA_FAILED" },
  { D3DERR_SETVIEWPORTDATA_FAILED,          "D3DERR_SETVIEWPORTDATA_FAILED" },
  { D3DERR_INVALIDCURRENTVIEWPORT,          "D3DERR_INVALIDCURRENTVIEWPORT" },
  { D3DERR_INVALIDPRIMITIVETYPE,            "D3DERR_INVALIDPRIMITIVETYPE" },
  { D3DERR_INVALIDVERTEXTYPE,               "D3DERR_INVALIDVERTEXTYPE" },
  { D3DERR_TEXTURE_BADSIZE,                 "D3DERR_TEXTURE_BADSIZE" },
  { D3DERR_INVALIDRAMPTEXTURE,              "D3DERR_INVALIDRAMPTEXTURE" },
  { D3DERR_MATERIAL_CREATE_FAILED,          "D3DERR_MATERIAL_CREATE_FAILED" },
  { D3DERR_MATERIAL_DESTROY_FAILED,         "D3DERR_MATERIAL_DESTROY_FAILED" },
  { D3DERR_MATERIAL_SETDATA_FAILED,         "D3DERR_MATERIAL_SETDATA_FAILED" },
  { D3DERR_MATERIAL_GETDATA_FAILED,         "D3DERR_MATERIAL_GETDATA_FAILED" },
  { D3DERR_INVALIDPALETTE,                  "D3DERR_INVALIDPALETTE" },
  { D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY,        "D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY" },
  { D3DERR_ZBUFF_NEEDS_VIDEOMEMORY,         "D3DERR_ZBUFF_NEEDS_VIDEOMEMORY" },
  { D3DERR_SURFACENOTINVIDMEM,              "D3DERR_SURFACENOTINVIDMEM" },
  { D3DERR_LIGHT_SET_FAILED,                "D3DERR_LIGHT_SET_FAILED" },
  { D3DERR_LIGHTHASVIEWPORT,                "D3DERR_LIGHTHASVIEWPORT" },
  { D3DERR_LIGHTNOTINTHISVIEWPORT,          "D3DERR_LIGHTNOTINTHISVIEWPORT" },
  { D3DERR_SCENE_IN_SCENE,                  "D3DERR_SCENE_IN_SCENE" },
  { D3DERR_SCENE_NOT_IN_SCENE,              "D3DERR_SCENE_NOT_IN_SCENE" },
  { D3DERR_SCENE_BEGIN_FAILED,              "D3DERR_SCENE_BEGIN_FAILED" },
  { D3DERR_SCENE_END_FAILED,                "D3DERR_SCENE_END_FAILED" },
  { D3DERR_INBEGIN,                         "D3DERR_INBEGIN" },
  { D3DERR_NOTINBEGIN,                      "D3DERR_NOTINBEGIN" },
  { D3DERR_NOVIEWPORTS,                     "D3DERR_NOVIEWPORTS" },
  { D3DERR_VIEWPORTDATANOTSET,              "D3DERR_VIEWPORTDATANOTSET" },
  { D3DERR_VIEWPORTHASNODEVICE,             "D3DERR_VIEWPORTHASNODEVICE" },
  { D3DERR_NOCURRENTVIEWPORT,               "D3DERR_NOCURRENTVIEWPORT" },
  { D3DERR_INVALIDVERTEXFORMAT,             "D3DERR_INVALIDVERTEXFORMAT" },
  { D3DERR_COLORKEYATTACHED,                "D3DERR_COLORKEYATTACHED" },
  { D3DERR_VERTEXBUFFEROPTIMIZED,           "D3DERR_VERTEXBUFFEROPTIMIZED" },
  { D3DERR_VBUF_CREATE_FAILED,              "D3DERR_VBUF_CREATE_FAILED" },
  { D3DERR_VERTEXBUFFERLOCKED,              "D3DERR_VERTEXBUFFERLOCKED" },
  { D3DERR_ZBUFFER_NOTPRESENT,              "D3DERR_ZBUFFER_NOTPRESENT" },
  { D3DERR_STENCILBUFFER_NOTPRESENT,        "D3DERR_STENCILBUFFER_NOTPRESENT" },
  { D3DERR_WRONGTEXTUREFORMAT,              "D3DERR_WRONGTEXTUREFORMAT" },
  { D3DERR_UNSUPPORTEDCOLOROPERATION,       "D3DERR_UNSUPPORTEDCOLOROPERATION" },
  { D3DERR_UNSUPPORTEDCOLORARG,             "D3DERR_UNSUPPORTEDCOLORARG" },
  { D3DERR_UNSUPPORTEDALPHAOPERATION,       "D3DERR_UNSUPPORTEDALPHAOPERATION" },
  { D3DERR_UNSUPPORTEDALPHAARG,             "D3DERR_UNSUPPORTEDALPHAARG" },
  { D3DERR_TOOMANYOPERATIONS,               "D3DERR_TOOMANYOPERATIONS" },
  { D3DERR_CONFLICTINGTEXTUREFILTER,        "D3DERR_CONFLICTINGTEXTUREFILTER" },
  { D3DERR_UNSUPPORTEDFACTORVALUE,          "D3DERR_UNSUPPORTEDFACTORVALUE" },
  { D3DERR_CONFLICTINGRENDERSTATE,          "D3DERR_CONFLICTINGRENDERSTATE" },
  { D3DERR_UNSUPPORTEDTEXTUREFILTER,        "D3DERR_UNSUPPORTEDTEXTUREFILTER" },
  { D3DERR_TOOMANYPRIMITIVES,               "D3DERR_TOOMANYPRIMITIVES" },
  { D3DERR_INVALIDMATRIX,                   "D3DERR_INVALIDMATRIX" },
  { D3DERR_TOOMANYVERTICES,                 "D3DERR_TOOMANYVERTICES" },
  { D3DERR_CONFLICTINGTEXTUREPALETTE,       "D3DERR_CONFLICTINGTEXTUREPALETTE" },
  { DDERR_ALREADYINITIALIZED,               "DDERR_ALREADYINITIALIZED" },
  { DDERR_CANNOTATTACHSURFACE,              "DDERR_CANNOTATTACHSURFACE" },
  { DDERR_CANNOTDETACHSURFACE,              "DDERR_CANNOTDETACHSURFACE" },
  { DDERR_CURRENTLYNOTAVAIL,                "DDERR_CURRENTLYNOTAVAIL" },
  { DDERR_EXCEPTION,                        "DDERR_EXCEPTION" },
  { DDERR_GENERIC,                          "DDERR_GENERIC" },
  { DDERR_HEIGHTALIGN,                      "DDERR_HEIGHTALIGN" },
  { DDERR_INCOMPATIBLEPRIMARY,              "DDERR_INCOMPATIBLEPRIMARY" },
  { DDERR_INVALIDCAPS,                      "DDERR_INVALIDCAPS" },
  { DDERR_INVALIDCLIPLIST,                  "DDERR_INVALIDCLIPLIST" },
  { DDERR_INVALIDMODE,                      "DDERR_INVALIDMODE" },
  { DDERR_INVALIDOBJECT,                    "DDERR_INVALIDOBJECT" },
  { DDERR_INVALIDPARAMS,                    "DDERR_INVALIDPARAMS" },
  { DDERR_INVALIDPIXELFORMAT,               "DDERR_INVALIDPIXELFORMAT" },
  { DDERR_INVALIDRECT,                      "DDERR_INVALIDRECT" },
  { DDERR_LOCKEDSURFACES,                   "DDERR_LOCKEDSURFACES" },
  { DDERR_NO3D,                             "DDERR_NO3D" },
  { DDERR_NOALPHAHW,                        "DDERR_NOALPHAHW" },
  { DDERR_NOCLIPLIST,                       "DDERR_NOCLIPLIST" },
  { DDERR_NOCOLORCONVHW,                    "DDERR_NOCOLORCONVHW" },
  { DDERR_NOCOOPERATIVELEVELSET,            "DDERR_NOCOOPERATIVELEVELSET" },
  { DDERR_NOCOLORKEY,                       "DDERR_NOCOLORKEY" },
  { DDERR_NOCOLORKEYHW,                     "DDERR_NOCOLORKEYHW" },
  { DDERR_NODIRECTDRAWSUPPORT,              "DDERR_NODIRECTDRAWSUPPORT" },
  { DDERR_NOEXCLUSIVEMODE,                  "DDERR_NOEXCLUSIVEMODE" },
  { DDERR_NOFLIPHW,                         "DDERR_NOFLIPHW" },
  { DDERR_NOGDI,                            "DDERR_NOGDI" },
  { DDERR_NOMIRRORHW,                       "DDERR_NOMIRRORHW" },
  { DDERR_NOTFOUND,                         "DDERR_NOTFOUND" },
  { DDERR_NOOVERLAYHW,                      "DDERR_NOOVERLAYHW" },
  { DDERR_OVERLAPPINGRECTS,                 "DDERR_OVERLAPPINGRECTS" },
  { DDERR_NORASTEROPHW,                     "DDERR_NORASTEROPHW" },
  { DDERR_NOROTATIONHW,                     "DDERR_NOROTATIONHW" },
  { DDERR_NOSTRETCHHW,                      "DDERR_NOSTRETCHHW" },
  { DDERR_NOT4BITCOLOR,                     "DDERR_NOT4BITCOLOR" },
  { DDERR_NOT4BITCOLORINDEX,                "DDERR_NOT4BITCOLORINDEX" },
  { DDERR_NOT8BITCOLOR,                     "DDERR_NOT8BITCOLOR" },
  { DDERR_NOTEXTUREHW,                      "DDERR_NOTEXTUREHW" },
  { DDERR_NOVSYNCHW,                        "DDERR_NOVSYNCHW" },
  { DDERR_NOZBUFFERHW,                      "DDERR_NOZBUFFERHW" },
  { DDERR_NOZOVERLAYHW,                     "DDERR_NOZOVERLAYHW" },
  { DDERR_OUTOFCAPS,                        "DDERR_OUTOFCAPS" },
  { DDERR_OUTOFMEMORY,                      "DDERR_OUTOFMEMORY" },
  { DDERR_OUTOFVIDEOMEMORY,                 "DDERR_OUTOFVIDEOMEMORY" },
  { DDERR_OVERLAYCANTCLIP,                  "DDERR_OVERLAYCANTCLIP" },
  { DDERR_OVERLAYCOLORKEYONLYONEACTIVE,     "DDERR_OVERLAYCOLORKEYONLYONEACTIVE" },
  { DDERR_PALETTEBUSY,                      "DDERR_PALETTEBUSY" },
  { DDERR_COLORKEYNOTSET,                   "DDERR_COLORKEYNOTSET" },
  { DDERR_SURFACEALREADYATTACHED,           "DDERR_SURFACEALREADYATTACHED" },
  { DDERR_SURFACEALREADYDEPENDENT,          "DDERR_SURFACEALREADYDEPENDENT" },
  { DDERR_SURFACEBUSY,                      "DDERR_SURFACEBUSY" },
  { DDERR_CANTLOCKSURFACE,                  "DDERR_CANTLOCKSURFACE" },
  { DDERR_SURFACEISOBSCURED,                "DDERR_SURFACEISOBSCURED" },
  { DDERR_SURFACELOST,                      "DDERR_SURFACELOST" },
  { DDERR_SURFACENOTATTACHED,               "DDERR_SURFACENOTATTACHED" },
  { DDERR_TOOBIGHEIGHT,                     "DDERR_TOOBIGHEIGHT" },
  { DDERR_TOOBIGSIZE,                       "DDERR_TOOBIGSIZE" },
  { DDERR_TOOBIGWIDTH,                      "DDERR_TOOBIGWIDTH" },
  { DDERR_UNSUPPORTED,                      "DDERR_UNSUPPORTED" },
  { DDERR_UNSUPPORTEDFORMAT,                "DDERR_UNSUPPORTEDFORMAT" },
  { DDERR_UNSUPPORTEDMASK,                  "DDERR_UNSUPPORTEDMASK" },
  { DDERR_INVALIDSTREAM,                    "DDERR_INVALIDSTREAM" },
  { DDERR_VERTICALBLANKINPROGRESS,          "DDERR_VERTICALBLANKINPROGRESS" },
  { DDERR_WASSTILLDRAWING,                  "DDERR_WASSTILLDRAWING" },
  { DDERR_XALIGN,                           "DDERR_XALIGN" },
  { DDERR_INVALIDDIRECTDRAWGUID,            "DDERR_INVALIDDIRECTDRAWGUID" },
  { DDERR_DIRECTDRAWALREADYCREATED,         "DDERR_DIRECTDRAWALREADYCREATED" },
  { DDERR_NODIRECTDRAWHW,                   "DDERR_NODIRECTDRAWHW" },
  { DDERR_PRIMARYSURFACEALREADYEXISTS,      "DDERR_PRIMARYSURFACEALREADYEXISTS" },
  { DDERR_NOEMULATION,                      "DDERR_NOEMULATION" },
  { DDERR_REGIONTOOSMALL,                   "DDERR_REGIONTOOSMALL" },
  { DDERR_CLIPPERISUSINGHWND,               "DDERR_CLIPPERISUSINGHWND" },
  { DDERR_NOCLIPPERATTACHED,                "DDERR_NOCLIPPERATTACHED" },
  { DDERR_NOHWND,                           "DDERR_NOHWND" },
  { DDERR_HWNDSUBCLASSED,                   "DDERR_HWNDSUBCLASSED" },
  { DDERR_HWNDALREADYSET,                   "DDERR_HWNDALREADYSET" },
  { DDERR_NOPALETTEATTACHED,                "DDERR_NOPALETTEATTACHED" },
  { DDERR_NOPALETTEHW,                      "DDERR_NOPALETTEHW" },
  { DDERR_BLTFASTCANTCLIP,                  "DDERR_BLTFASTCANTCLIP" },
  { DDERR_NOBLTHW,                          "DDERR_NOBLTHW" },
  { DDERR_NODDROPSHW,                       "DDERR_NODDROPSHW" },
  { DDERR_OVERLAYNOTVISIBLE,                "DDERR_OVERLAYNOTVISIBLE" },
  { DDERR_NOOVERLAYDEST,                    "DDERR_NOOVERLAYDEST" },
  { DDERR_INVALIDPOSITION,                  "DDERR_INVALIDPOSITION" },
  { DDERR_NOTAOVERLAYSURFACE,               "DDERR_NOTAOVERLAYSURFACE" },
  { DDERR_EXCLUSIVEMODEALREADYSET,          "DDERR_EXCLUSIVEMODEALREADYSET" },
  { DDERR_NOTFLIPPABLE,                     "DDERR_NOTFLIPPABLE" },
  { DDERR_CANTDUPLICATE,                    "DDERR_CANTDUPLICATE" },
  { DDERR_NOTLOCKED,                        "DDERR_NOTLOCKED" },
  { DDERR_CANTCREATEDC,                     "DDERR_CANTCREATEDC" },
  { DDERR_NODC,                             "DDERR_NODC" },
  { DDERR_WRONGMODE,                        "DDERR_WRONGMODE" },
  { DDERR_IMPLICITLYCREATED,                "DDERR_IMPLICITLYCREATED" },
  { DDERR_NOTPALETTIZED,                    "DDERR_NOTPALETTIZED" },
  { DDERR_UNSUPPORTEDMODE,                  "DDERR_UNSUPPORTEDMODE" },
  { DDERR_NOMIPMAPHW,                       "DDERR_NOMIPMAPHW" },
  { DDERR_INVALIDSURFACETYPE,               "DDERR_INVALIDSURFACETYPE" },
  { DDERR_NOOPTIMIZEHW,                     "DDERR_NOOPTIMIZEHW" },
  { DDERR_NOTLOADED,                        "DDERR_NOTLOADED" },
  { DDERR_NOFOCUSWINDOW,                    "DDERR_NOFOCUSWINDOW" },
  { DDERR_DCALREADYCREATED,                 "DDERR_DCALREADYCREATED" },
  { DDERR_NONONLOCALVIDMEM,                 "DDERR_NONONLOCALVIDMEM" },
  { DDERR_CANTPAGELOCK,                     "DDERR_CANTPAGELOCK" },
  { DDERR_CANTPAGEUNLOCK,                   "DDERR_CANTPAGEUNLOCK" },
  { DDERR_NOTPAGELOCKED,                    "DDERR_NOTPAGELOCKED" },
  { DDERR_MOREDATA,                         "DDERR_MOREDATA" },
  { DDERR_EXPIRED,                          "DDERR_EXPIRED" },
  { DDERR_VIDEONOTACTIVE,                   "DDERR_VIDEONOTACTIVE" },
  { DDERR_DEVICEDOESNTOWNSURFACE,           "DDERR_DEVICEDOESNTOWNSURFACE" },
  { DDERR_NOTINITIALIZED,                   "DDERR_NOTINITIALIZED" }
};

static int J3DAPI std3D_InitRenderState(void);
static HRESULT CALLBACK std3D_EnumZBufferFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, DDPIXELFORMAT* lpContext);
static HRESULT CALLBACK std3D_D3DEnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpD3DHalDDesc, LPD3DDEVICEDESC lpD3DHelDDesc, LPVOID lpContext);
static HRESULT CALLBACK std3D_EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext);

static int J3DAPI std3D_CreateViewport(void);
static void J3DAPI std3D_GetZBufferFormat(DDPIXELFORMAT* pPixelFormat);
static void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture);
static void J3DAPI std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture);
static const char* J3DAPI std3D_D3DGetStatus(HRESULT res);

int J3DAPI std3D_Startup(void)
{
    HRESULT ddres;

#line 154
    STD_ASSERTREL(bStartup == 0);
    memset(std3D_aTextureFormats, 0, sizeof(std3D_aTextureFormats));
    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    std3D_lpDD = stdDisplay_GetDirectDraw();
    if ( !std3D_lpDD )
    {
#line 163
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DDraw device not created yet!\n", 0, 0, 0, 0);
        return 0;
    }

    ddres = IDirectDraw4_QueryInterface(std3D_lpDD, &IID_IDirect3D3, &std3D_pDirect3D);
    if ( ddres != DD_OK )
    {
#line 173
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating Direct3D interface object.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    std3D_numDevices = 0;
    ddres = IDirect3D3_EnumDevices(std3D_pDirect3D, std3D_D3DEnumDevicesCallback, 0);
    if ( ddres != DD_OK )
    {
#line 184
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when enumerating D3D devices.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
        return 0;
    }

    if ( std3D_numDevices == 0 ) {
        return 0;
    }

    bStartup = 1;
    return 1;
}

void J3DAPI std3D_Shutdown(void)
{
    if ( std3D_bOpen ) {
        std3D_Close();
    }

    if ( std3D_pDirect3D ) {
        IDirect3D3_Release(std3D_pDirect3D);
    }
    std3D_pD3Device  = NULL;
    std3D_pDirect3D  = NULL;
    std3D_numDevices = 0;

    memset(std3D_aTextureFormats, 0, sizeof(std3D_aTextureFormats));
    memset(std3D_aDevices, 0, sizeof(std3D_aDevices));

    bStartup = 0;
}

size_t J3DAPI std3D_GetNumDevices(void)
{
    return std3D_numDevices;
}

const Device3D* J3DAPI std3D_GetAllDevices(void)
{
    return std3D_aDevices;
}

int J3DAPI std3D_Open(size_t deviceNum)
{
    DDPIXELFORMAT pixelFormat; // Retail leaves this uninitialised; std3D_GetZBufferFormat writes every field it uses
    PALETTEENTRY aPalette[256];
    HRESULT hr;
    size_t memFree;
    size_t totalVideoMem;
    int i;

#line 276
    STD_ASSERTREL(bStartup == 1);
    if ( std3D_bOpen )
    {
#line 280
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 0;
    }

    if ( deviceNum >= std3D_numDevices ) {
        return 0;
    }

    std3D_curDevice  = deviceNum;
    std3D_pCurDevice = &std3D_aDevices[std3D_curDevice];
    if ( !std3D_pCurDevice->caps.hasZBuffer ) {
        return 0;
    }

    std3D_GetZBufferFormat(&pixelFormat);
    if ( stdDisplay_CreateZBuffer(&pixelFormat, std3D_pCurDevice->caps.bHAL == 0) )
    {
#line 295
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error creating Z buffer.\n", 0, 0, 0, 0);
        return 0;
    }

    // Init palette
    
    for ( i = 0; i < 256; ++i )
    {
        aPalette[i].peFlags = D3DPAL_RESERVED;
        aPalette[i].peBlue  = 254;
        aPalette[i].peGreen = 254;
        aPalette[i].peRed   = 254;
    }

    hr = IDirectDraw4_CreatePalette(
        std3D_lpDD,
        DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE | DDPCAPS_8BIT,
        (LPPALETTEENTRY)aPalette,
        &std3D_pDDPalette,
        NULL
    );

    if ( hr != DD_OK )
    {
#line 313
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating palette.\n", std3D_D3DGetStatus(hr), 0, 0, 0);
        return 0;
    }

    hr = IDirect3D3_CreateDevice(
        std3D_pDirect3D,
        &std3D_pCurDevice->duid,
        stdDisplay_backBuffer.surface.pDDSurf,
        &std3D_pD3Device,
        NULL
    );

    if ( hr != D3D_OK )
    {
#line 322
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating Direct3D device.\n", std3D_D3DGetStatus(hr), 0, 0, 0);
        return 0;
    }

    std3D_numTextureFormats    = 0;
    std3D_bHasRGBTextureFormat = 0;
    hr = IDirect3DDevice3_EnumTextureFormats(std3D_pD3Device, std3D_EnumTextureFormatsCallback, 0);
    if ( hr != D3D_OK )
    {
#line 334
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when enumerating D3D device texture formats.\n", std3D_D3DGetStatus(hr), 0, 0, 0);
        return 0;
    }

    if ( !std3D_numTextureFormats || !std3D_bHasRGBTextureFormat )
    {
#line 341
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: no 16 texture formats found.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( !std3D_CreateViewport() )
    {
#line 352
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error creating viewport.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( !std3D_pCurDevice->caps.maxVertexCount )
    {
        std3D_maxVertices = STD3D_DEFAULT_MAX_VERTICES;
    }
    else
    {
        std3D_maxVertices = std3D_pCurDevice->caps.maxVertexCount;
    }
#line 363
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Max vertices: %d.\n", std3D_maxVertices, 0, 0, 0);

    std3D_frameCount        = 1;
    std3D_numCachedTextures = 0;
    std3D_pFirstTexCache    = NULL;
    std3D_pLastTexCache     = NULL;

    // Get color formats for RGB, RGBA and RGBA key formats
    std3D_RGBTextureFormat     = std3D_FindClosestFormat(&std3D_cfRGB565);
    std3D_RGBAKeyTextureFormat = std3D_FindClosestFormat(&std3D_cfRGB5551);
    if ( std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ci.alphaBPP == 0
        && std3D_pCurDevice->caps.bColorkeyTextureSupported )
    {
        std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].bColorKey = 1;
        std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].colorKey = 0;
    }

    std3D_RGBATextureFormat = std3D_FindClosestFormat(&std3D_cfRGB4444);
    if ( std3D_aTextureFormats[std3D_RGBATextureFormat].ci.alphaBPP == 0
        && std3D_pCurDevice->caps.bColorkeyTextureSupported )
    {
        std3D_aTextureFormats[std3D_RGBATextureFormat].bColorKey = 1;
        std3D_aTextureFormats[std3D_RGBATextureFormat].colorKey = 0;
    }

    if ( !std3D_InitRenderState() )
    {
#line 399
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error initializing render state.\n", 0, 0, 0, 0);
        return 0;
    }

    if ( stdDisplay_GetTextureMemory(&std3D_pCurDevice->totalMemory, &std3D_pCurDevice->availableMemory) )
    {
        // Since we failed to get texture memory info indicate that opening failed
        return 0;
    }

#line 410
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Texture Ram  Total: %d bytes  Free: %d bytes.\n", std3D_pCurDevice->totalMemory, std3D_pCurDevice->availableMemory, 0, 0);

    totalVideoMem = memFree = 0;
    stdDisplay_GetTotalMemory(&totalVideoMem, &memFree);
#line 420
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Video Ram Total: %d bytes  Free: %d bytes.\n", totalVideoMem, memFree, 0, 0);

    std3D_bOpen = 1;
    return 1;
}

void J3DAPI std3D_Close(void)
{
    std3D_ResetTextureCache();
    if ( std3D_pDDPalette )
    {
        IDirectDrawPalette_Release(std3D_pDDPalette);
        std3D_pDDPalette = NULL;
    }

    if ( std3D_lpD3DViewPort )
    {
        IDirect3DViewport3_Release(std3D_lpD3DViewPort);
        std3D_lpD3DViewPort = NULL;
    }

    if ( std3D_pD3Device )
    {
        IDirect3DDevice3_Release(std3D_pD3Device);
        std3D_pD3Device = NULL;
    }

    std3D_numTextureFormats    = 0;
    std3D_curDevice            = 0;
    std3D_pCurDevice           = NULL;
    std3D_bHasRGBTextureFormat = 0;
    std3D_bOpen                = 0;
}

void J3DAPI std3D_GetTextureFormat(StdColorFormatType type, ColorInfo* pDest, int* pbColorKeySet, uint32_t* pColorKey)
{
    switch ( type )
    {
        case STDCOLOR_FORMAT_RGBA_1BIT_ALPHA:
            *pbColorKeySet = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].bColorKey;
            *pColorKey     = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].colorKey;
            memcpy(pDest, &std3D_aTextureFormats[std3D_RGBAKeyTextureFormat], sizeof(ColorInfo));
            break;

        case STDCOLOR_FORMAT_RGBA:
            *pbColorKeySet = std3D_aTextureFormats[std3D_RGBATextureFormat].bColorKey;
            *pColorKey     = std3D_aTextureFormats[std3D_RGBATextureFormat].colorKey;
            memcpy(pDest, &std3D_aTextureFormats[std3D_RGBATextureFormat], sizeof(ColorInfo));
            break;

        default:
            *pbColorKeySet = 0;
            memcpy(pDest, &std3D_aTextureFormats[std3D_RGBTextureFormat], sizeof(ColorInfo));
            break;
    }
}

StdColorFormatType J3DAPI std3D_GetColorFormat(const ColorInfo* pCi)
{
    switch ( pCi->alphaBPP )
    {
        case 0:
            return STDCOLOR_FORMAT_RGB;

        case 1:
            return STDCOLOR_FORMAT_RGBA_1BIT_ALPHA;

        default:
            return STDCOLOR_FORMAT_RGBA;
    }
}

size_t J3DAPI std3D_GetNumTextureFormats(void)
{
    return std3D_numTextureFormats;
}

int J3DAPI std3D_StartScene(void)
{
    HRESULT d3dres;

    ++std3D_frameCount;

    d3dres = IDirect3DDevice3_BeginScene(std3D_pD3Device);
    if ( d3dres != D3D_OK ) {
#line 530
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s beginning scene.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
    }

    std3D_pD3DTex = NULL;
    return d3dres;
}

void J3DAPI std3D_EndScene(void)
{
    HRESULT d3dres = IDirect3DDevice3_EndScene(std3D_pD3Device);
    if ( d3dres != D3D_OK ) {
#line 556
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s ending scene.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
    }
    std3D_pD3DTex = NULL;
}

/* Discarded by the retail linker (no caller in either build): the debug build has it here (0x004F80C2, lines 567/578/587),
   the non-indexed twin of DrawRenderList's head (max-vertex check, SetRenderState, SetTexture) ending in DrawPrimitive.
   Body from the debug code; name from Jones3DRE. */
void J3DAPI std3D_sub_4F80C2(LPDIRECT3DTEXTURE2 pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts)
{
    LPDIRECT3DTEXTURE2 pTexture = pTex;
    HRESULT d3dres;

    if ( numVerts > std3D_maxVertices )
    {
#line 567
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %d > %d maxVertices.\n", numVerts, std3D_maxVertices, 0, 0);
        return;
    }

    std3D_SetRenderState(rdflags);

    if ( pTex != std3D_pD3DTex )
    {
        d3dres = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, pTexture);
        if ( d3dres != D3D_OK ) {
#line 578
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s SetRenderState.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        }
        else {
            std3D_pD3DTex = pTex;
        }
    }

    d3dres = IDirect3DDevice3_DrawPrimitive(std3D_pD3Device, D3DPT_TRIANGLELIST, D3DFVF_TLVERTEX, aVerts, numVerts, D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS);
    if ( d3dres != D3D_OK )
    {
#line 587
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s DrawPrim.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
    }
}

// The fog-alpha conversion helper (inline assembly, like stdMath.h's). The debug build carries its
// out-of-line copy (0x4FBEC0, a pick-any COMDAT: `__inline` without `static`); release expands
// it once, in DrawRenderList. Name inferred.
__inline int J3DAPI std3D_FloatToInt(float value)
{
    int r;
    int* p = &r;
    __asm fld value
    __asm mov eax, p
    __asm fistp dword ptr [eax]
    return r;
}

void J3DAPI std3D_DrawRenderList(LPDIRECT3DTEXTURE2 pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices)
{
    LPDIRECT3DTEXTURE2 pTexture = pTex;
    HRESULT d3dres;

    if ( numVerts > (unsigned int)std3D_maxVertices )
    {
#line 598
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %d > %d maxVertices.\n", numVerts, std3D_maxVertices, 0, 0);
        return;
    }

    std3D_SetRenderState(rdflags);

    if ( pTex != std3D_pD3DTex )
    {
        d3dres = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, pTexture);
        if ( d3dres != D3D_OK ) {
#line 609
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s SetRenderState.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        }
        else {
            std3D_pD3DTex = pTex;
        }
    }

    if ( std3D_bFogTable )
    {
        size_t i;
        D3DTLVERTEX* pCurrentVert = aVerts;
        float depth;
        for ( i = 0; i < numVerts; ++i )
        {
            if ( pCurrentVert->rhw <= 0.0f )
            {
                pCurrentVert->specular = 0xFF000000;
            }
            else
            {
                depth = (std3D_fogEndDepth - pCurrentVert->rhw * std3D_zDepth) * std3D_fogDepthFactor;
                if ( depth >= 1.0f )
                {
                    pCurrentVert->specular = 0;
                }
                else if ( depth < 0.0f )
                {
                    pCurrentVert->specular = 0xFF000000;
                }
                else
                {
                    pCurrentVert->specular = std3D_FloatToInt((1.0f - depth) * 255.0f) << 24;
                }
            }
            ++pCurrentVert;
        }
    }

    d3dres = IDirect3DDevice3_DrawIndexedPrimitive(
        std3D_pD3Device,
        D3DPT_TRIANGLELIST,
        D3DFVF_TLVERTEX,
        aVerts,
        numVerts,
        aIndices,
        numIndices,
        D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS
    );

    if ( d3dres != D3D_OK )
    {
#line 643
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s DrawPrim.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
    }
}

void J3DAPI std3D_SetWireframeRenderState(void)
{
    HRESULT d3dres;

    std3D_SetRenderState(std3D_renderState & ~(STD3D_RS_FOG_ENABLED | STD3D_RS_UNKNOWN_400 | STD3D_RS_UNKNOWN_200));

    d3dres = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, NULL);
    if ( d3dres != D3D_OK )
    {
#line 655
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s SetRenderState.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        return;
    }

    std3D_pD3DTex = NULL;
}

void J3DAPI std3D_DrawLineStrip(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    HRESULT d3dres;

    if ( numVerts > std3D_maxVertices )
    {
#line 667
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %d > %d maxVertices.\n", numVerts, std3D_maxVertices, 0, 0);
        return;
    }

    d3dres = IDirect3DDevice3_DrawPrimitive(
        std3D_pD3Device,
        D3DPT_LINESTRIP,
        D3DFVF_TLVERTEX,
        aVerts,
        numVerts,
        D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS
    );

    if ( d3dres != D3D_OK ) {
#line 674
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s DrawPrim.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
    }
}

void J3DAPI std3D_DrawPointList(LPD3DTLVERTEX aVerts, size_t numVerts)
{
    HRESULT ddres;

    if ( numVerts > std3D_maxVertices )
    {
#line 684
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %d > %d maxVertices.\n", numVerts, std3D_maxVertices, 0, 0);
        return;
    }

    ddres = IDirect3DDevice3_DrawPrimitive(
        std3D_pD3Device,
        D3DPT_POINTLIST,
        D3DFVF_TLVERTEX,
        aVerts,
        numVerts,
        D3DDP_DONOTLIGHT | D3DDP_DONOTUPDATEEXTENTS
    );

    if ( ddres != D3D_OK ) {
#line 691
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s DrawPrim.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
    }
}

void J3DAPI std3D_SetRenderState(Std3DRenderState rdflags)
{
    int alphaRef;
    HRESULT hr;

    if ( std3D_renderState != rdflags )
    {
        if ( (std3D_renderState & STD3D_RS_ZWRITE_DISABLED) != (rdflags & STD3D_RS_ZWRITE_DISABLED) )
        {
            if ( (rdflags & STD3D_RS_ZWRITE_DISABLED) != 0 ) {
                hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZWRITEENABLE, FALSE);
            }
            else {
                hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZWRITEENABLE, TRUE);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_U) != (rdflags & STD3D_RS_TEX_CPAMP_U) )
        {
            if ( (rdflags & STD3D_RS_TEX_CPAMP_U) != 0 ) {
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
            }
            else {
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEX_CPAMP_V) != (rdflags & STD3D_RS_TEX_CPAMP_V) )
        {
            if ( (rdflags & STD3D_RS_TEX_CPAMP_V) != 0 ) {
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
            }
            else {
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
            }
        }

        if ( (std3D_renderState & STD3D_RS_FOG_ENABLED) != (rdflags & STD3D_RS_FOG_ENABLED) )
        {
            if ( (rdflags & STD3D_RS_FOG_ENABLED) != 0 && std3D_bRenderFog ) {
                hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGENABLE, TRUE);
            }
            else {
                hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGENABLE, FALSE);
            }
        }

        if ( (std3D_renderState & STD3D_RS_TEXFILTER_BILINEAR) != (rdflags & STD3D_RS_TEXFILTER_BILINEAR) )
        {
            if ( (rdflags & STD3D_RS_TEXFILTER_BILINEAR) != 0
                && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
            {
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
            }
            else if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
            {
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT);
                hr = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_POINT);
            }
        }

        if ( (std3D_renderState & STD3D_RS_ALPHAREF_SET) != (rdflags & STD3D_RS_ALPHAREF_SET) )
        {
            if ( (rdflags & STD3D_RS_ALPHAREF_SET) != 0 )
            {
                alphaRef = 0xA0;
                hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAREF, alphaRef);
            }
            else
            {
                hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAREF, 0);
            }
        }

        std3D_renderState = rdflags;
    }
}

void J3DAPI std3D_AllocSystemTexture(tSystemTexture* pTexture, tVBuffer** apVBuffers, size_t numMipLevels, StdColorFormatType formatType)
{
    size_t row;
    LPDIRECT3DTEXTURE2 pTmpTex;
    tVBuffer* pVBuffer;
    void* pDestPexels;
    void* pSrcPixels;
    uint32_t texWidth, texHeight;
    size_t texSize;
    DDPIXELFORMAT ddpixfmt;
    DDSURFACEDESC2 ddsdSrc;
    DDSURFACEDESC2 ddsdSrcTmp;
    DDSCAPS2 ddscaps;
    HRESULT ddres;
    LPDIRECT3DTEXTURE2 pTex;
    size_t mmNum;

    LPDIRECTDRAWSURFACE4 pSrcSurf    = NULL;
    LPDIRECTDRAWSURFACE4 pSrcSurfTmp = NULL;
    memset(pTexture, 0, sizeof(tSystemTexture));

    if ( !std3D_numTextureFormats ) {
        return;
    }

    // Get mipmap buffer at LOD 0
    pVBuffer = *apVBuffers;
    // Retail leaves texWidth/texHeight uninitialised; GetValidDimensions writes both
    std3D_GetValidDimensions(pVBuffer->rasterInfo.width, pVBuffer->rasterInfo.height, &texWidth, &texHeight);
    while ( numMipLevels > 1 && (pVBuffer->rasterInfo.width > texWidth || pVBuffer->rasterInfo.height > texHeight) )
    {
        --numMipLevels;
        pVBuffer = *++apVBuffers;
        std3D_GetValidDimensions(pVBuffer->rasterInfo.width, pVBuffer->rasterInfo.height, &texWidth, &texHeight);
    }

    texSize = texWidth * texHeight * pVBuffer->rasterInfo.colorInfo.bpp / 8;
    if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE ) {
        numMipLevels = 1;
    }

    switch ( formatType )
    {
        case STDCOLOR_FORMAT_RGBA_1BIT_ALPHA:
            ddpixfmt = std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].ddPixelFmt;
            break;

        case STDCOLOR_FORMAT_RGBA:
            ddpixfmt = std3D_aTextureFormats[std3D_RGBATextureFormat].ddPixelFmt;
            break;

        default:
            ddpixfmt = std3D_aTextureFormats[std3D_RGBTextureFormat].ddPixelFmt;
            break;
    }

    memset(&ddsdSrc, 0, sizeof(ddsdSrc));
    ddsdSrc.dwSize          = sizeof(DDSURFACEDESC2);
    ddsdSrc.dwFlags         = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsdSrc.ddsCaps.dwCaps  = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    ddsdSrc.dwWidth         = pVBuffer->rasterInfo.width;
    ddsdSrc.dwHeight        = pVBuffer->rasterInfo.height;
    memcpy(&ddsdSrc.ddpfPixelFormat, &ddpixfmt, sizeof(DDPIXELFORMAT));

    pSrcSurf = NULL;
    if ( std3D_mipmapFilter == STD3D_MIPMAPFILTER_NONE )
    {
        ddsdSrc.dwMipMapCount = 0;
    }
    else
    {
        ddsdSrc.dwFlags        |= DDSD_MIPMAPCOUNT;
        ddsdSrc.ddsCaps.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
        ddsdSrc.dwMipMapCount  = numMipLevels;
    }

    ddsdSrcTmp = ddsdSrc; // TODO: it might be copied within the scope where ddsdSrcTmp is used

    ddres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdSrc, &pSrcSurf, NULL);
    if ( ddres != DD_OK )
    {
#line 940
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating the DirectDraw source surface.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
        goto error;
    }
    else
    {
        // Retail does not pre-null pTex here; QueryInterface writes it
        ddres = IDirectDrawSurface4_QueryInterface(pSrcSurf, &IID_IDirect3DTexture2, &pTex);
        if ( ddres != DD_OK )
        {
#line 949
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating Direct3D source texture.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
            goto error;
        }
        else
        {
            for ( mmNum = 0; mmNum < numMipLevels; ++mmNum )
            {
                DDSURFACEDESC2 ddsdMipMap;

                memset(&ddsdMipMap, 0, sizeof(ddsdMipMap));
                ddsdMipMap.dwSize = sizeof(DDSURFACEDESC2);
                ddres = IDirectDrawSurface4_Lock(pSrcSurf, NULL, &ddsdMipMap, DDLOCK_WAIT, NULL);
                if ( ddres != DD_OK )
                {
#line 970
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when locking the DDSurface source buffer.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                    goto error;
                }

                // Copy pixels from input VBuffer mipmap to surface
                switch ( apVBuffers[mmNum]->rasterInfo.colorInfo.colorMode )
                {
                    case STDCOLOR_PAL:
#line 979
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't use paletized textures.\n", 0, 0, 0, 0);
                        break;

                    case STDCOLOR_RGB:
                    case STDCOLOR_RGBA:
                        stdDisplay_VBufferLock(apVBuffers[mmNum]);
                        for ( row = 0; row < ddsdMipMap.dwHeight; ++row )
                        {
                            pSrcPixels = &apVBuffers[mmNum]->pPixels[row * apVBuffers[mmNum]->rasterInfo.rowSize];
                            pDestPexels = (uint8_t*)ddsdMipMap.lpSurface + ddsdMipMap.lPitch * row;
                            memcpy(pDestPexels, pSrcPixels, ddsdMipMap.dwWidth * 2); // Retail copies a 16bpp row
                        }

                        stdDisplay_VBufferUnlock(apVBuffers[mmNum]);
                        break;
                }

                ddres = IDirectDrawSurface4_Unlock(pSrcSurf, NULL);
                if ( ddres != DD_OK )
                {
#line 1003
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when unlocking the DDSurface source buffer.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                    goto error;
                }

                if ( mmNum < numMipLevels - 1 )
                {
                    memset(&ddscaps, 0, sizeof(ddscaps));
                    ddscaps.dwCaps = DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;
                    ddres = IDirectDrawSurface4_GetAttachedSurface(pSrcSurf, &ddscaps, &pSrcSurf);
                    if ( ddres != DD_OK )
                    {
#line 1015
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting next DDSurface source buffer.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                        goto error;
                    }
                }
            }

            // If current texture size differs from input texture size
            // Create new texture with correct size and copy over current texture
            if ( texWidth != pVBuffer->rasterInfo.width || texHeight != pVBuffer->rasterInfo.height )
            {
                ddsdSrcTmp.dwWidth  = texWidth;
                ddsdSrcTmp.dwHeight = texHeight;
                ddres = IDirectDraw4_CreateSurface(std3D_lpDD, (LPDDSURFACEDESC2)&ddsdSrcTmp, &pSrcSurfTmp, NULL);
                if ( ddres != DD_OK )
                {
#line 1032
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating the DirectDraw source surface.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                    goto error;
                }
                else
                {
                    ddres = IDirectDrawSurface4_QueryInterface(pSrcSurfTmp, &IID_IDirect3DTexture2, &pTmpTex);
                    if ( ddres != DD_OK )
                    {
#line 1041
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating Direct3D source texture.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                        goto error;
                    }
                    else
                    {
                        for ( mmNum = 0; mmNum < numMipLevels; ++mmNum )
                        {
                            ddres = IDirectDrawSurface4_Blt(pSrcSurfTmp, NULL, pSrcSurf, NULL, DDBLT_WAIT, NULL);
                            if ( ddres != DD_OK )
                            {
#line 1050
                                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating the DirectDraw source surface.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                                goto error;
                            }

                            if ( mmNum < numMipLevels - 1 )
                            {
                                memset(&ddscaps, 0, sizeof(ddscaps));
                                ddscaps.dwCaps = DDSCAPS_MIPMAP | DDSCAPS_TEXTURE;
                                ddres = IDirectDrawSurface4_GetAttachedSurface(pSrcSurfTmp, &ddscaps, &pSrcSurfTmp);
                                if ( ddres != DD_OK )
                                {
#line 1062
                                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when getting next DDSurface source buffer.\n", std3D_D3DGetStatus(ddres), 0, 0, 0);
                                    goto error;
                                }
                            }
                        }

                        if ( pSrcSurfTmp ) {
                            IDirectDrawSurface4_Release(pSrcSurfTmp);
                        }

                        IDirect3DTexture2_Release(pTex);
                        ddsdSrc = ddsdSrcTmp;
                        pTex    = pTmpTex;
                    }
                }
            }
        }
    }

    if ( pSrcSurf ) {
        IDirectDrawSurface4_Release(pSrcSurf);
    }

    pTexture->ddsd           = ddsdSrc;
    pTexture->pD3DSrcTexture = pTex;
    pTexture->textureSize    = texSize;
    return;

error:
    if ( pSrcSurf ) {
        IDirectDrawSurface4_Release(pSrcSurf);
    }

#line 1096
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Done error exit from std3D_AllocSystemTexture.\n", 0, 0, 0, 0);
    return;
}

void J3DAPI std3D_GetValidDimensions(uint32_t width, uint32_t height, uint32_t* pOutWidth, uint32_t* pOutHeight)
{
    uint32_t texWidth  = STDMATH_CLAMP(width, std3D_pCurDevice->caps.minTexWidth, std3D_pCurDevice->caps.maxTexWidth);
    uint32_t texHeight = STDMATH_CLAMP(height, std3D_pCurDevice->caps.minTexHeight, std3D_pCurDevice->caps.maxTexHeight);

    if ( std3D_pCurDevice->caps.bSqareOnlyTexture && texWidth != texHeight )
    {
        *pOutHeight = *pOutWidth = J3DMAX(texWidth, texHeight);
    }
    else
    {
        *pOutWidth  = texWidth;
        *pOutHeight = texHeight;
    }
}

void J3DAPI std3D_ClearSystemTexture(tSystemTexture* pTex)
{
    if ( pTex->pD3DSrcTexture ) {
        IDirect3DTexture2_Release(pTex->pD3DSrcTexture);
    }

    if ( pTex->pD3DCachedTex )
    {
        std3D_RemoveTextureFromCacheList(pTex);
        IDirect3DTexture2_Release(pTex->pD3DCachedTex);
    }

    memset(pTex, 0, sizeof(tSystemTexture));
}

void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format)
{
    LPDIRECTDRAWSURFACE4 pVideoSurface = NULL;
    LPDIRECT3DTEXTURE2 pD3DTex = NULL;
    DDSURFACEDESC2 ddsdSurf;
    DDCOLORKEY colorKey;
    HRESULT dres;

#line 1149
    STD_ASSERTREL(pCacheTexture);



    if ( !pCacheTexture->pD3DSrcTexture )
    {
#line 1153
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "No Source texture.\n", 0, 0, 0, 0);
        goto error;
    }


    if ( pCacheTexture->textureSize > std3D_pCurDevice->availableMemory ) {
        std3D_PurgeTextureCache(pCacheTexture->textureSize);
    }

    ddsdSurf = pCacheTexture->ddsd;
    ddsdSurf.ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
    ddsdSurf.ddsCaps.dwCaps |=  DDSCAPS_ALLOCONLOAD | DDSCAPS_VIDEOMEMORY; //DDSCAPS_ALLOCONLOAD requires the D3D texture to call load before it can be used

    dres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdSurf, &pVideoSurface, NULL);
    while ( dres == DDERR_OUTOFVIDEOMEMORY )
    {
        if ( !std3D_PurgeTextureCache(pCacheTexture->textureSize) )
        {
#line 1179
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to purge texture cache for %x bytes!!!.\n", pCacheTexture->textureSize, 0, 0, 0);
            pVideoSurface = NULL;
            goto error;
        }
        dres = IDirectDraw4_CreateSurface(std3D_lpDD, &ddsdSurf, &pVideoSurface, NULL);
    }

    if ( dres != DD_OK )
    {
#line 1189
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating DirectDraw surface for texture.\n", std3D_D3DGetStatus(dres), 0, 0, 0);
        goto error;
    }

    if ( std3D_aTextureFormats[format].bColorKey )
    {
        colorKey.dwColorSpaceLowValue = std3D_aTextureFormats[format].colorKey;
        colorKey.dwColorSpaceHighValue = std3D_aTextureFormats[format].colorKey;
        dres = IDirectDrawSurface4_SetColorKey(pVideoSurface, DDCKEY_SRCBLT, &colorKey);
        if ( dres != DD_OK ) {
#line 1201
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s setting ColorKey dest texture.\n", std3D_D3DGetStatus(dres), 0, 0, 0);
        }
    }

    dres = IDirectDrawSurface4_QueryInterface(pVideoSurface, &IID_IDirect3DTexture2, &pD3DTex);
    if ( dres != DD_OK )
    {
#line 1210
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s creating Direct3D dest texture.\n", std3D_D3DGetStatus(dres), 0, 0, 0);
        pD3DTex = NULL;
        goto error;
    }

    dres = IDirect3DTexture2_Load(pD3DTex, pCacheTexture->pD3DSrcTexture);
    while ( dres == DDERR_OUTOFVIDEOMEMORY )
    {
        if ( !std3D_PurgeTextureCache(pCacheTexture->textureSize) )
        {
#line 1230
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to purge texture cache for %x bytes!!!.\n", pCacheTexture->textureSize, 0, 0, 0);
            goto error;
        }
        dres = IDirect3DTexture2_Load(pD3DTex, pCacheTexture->pD3DSrcTexture);
    }

    if ( dres != D3D_OK )
    {
#line 1239
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s Loading texture.\n", std3D_D3DGetStatus(dres), 0, 0, 0);
    }
    else
    {
        // Success
        pCacheTexture->pD3DCachedTex = pD3DTex;
        IDirectDrawSurface4_Release(pVideoSurface);
        pCacheTexture->frameNum      = std3D_frameCount;
        std3D_AddTextureToCacheList(pCacheTexture);
        return;
    }

error:
    if ( pVideoSurface ) {
        IDirectDrawSurface4_Release(pVideoSurface);
    }

    if ( pD3DTex ) {
        IDirect3DTexture2_Release(pD3DTex);
    }

    pCacheTexture->pD3DCachedTex = NULL;
    pCacheTexture->frameNum = 0;
#line 1277
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Done error exit from std3D_AddToTextureCache.\n", 0, 0, 0, 0);
}

size_t J3DAPI std3D_GetMipMapCount(const tSystemTexture* pTexture)
{
    LPDIRECTDRAWSURFACE4 pDDSurf = NULL;
    DDSURFACEDESC2 ddsd;
    HRESULT hDdres;

    if ( !pTexture || !pTexture->pD3DSrcTexture ) {
        return 0;
    }

    hDdres = IDirect3DTexture2_QueryInterface(pTexture->pD3DSrcTexture, &IID_IDirectDrawSurface4, &pDDSurf);
    if ( hDdres != S_OK ) {
        return 0;
    }

    ddsd.dwSize  = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags = DDSD_MIPMAPCOUNT;

    hDdres = IDirectDrawSurface4_GetSurfaceDesc(pDDSurf, &ddsd);
    IDirectDrawSurface4_Release(pDDSurf);
    if ( hDdres == DD_OK ) {
        return ddsd.dwMipMapCount;
    }

    return 0;
}

void J3DAPI std3D_ResetTextureCache(void)
{
    tSystemTexture* pCurTex = std3D_pFirstTexCache;
    while ( pCurTex )
    {
        tSystemTexture* pTex;

        if ( pCurTex->pD3DCachedTex )
        {
            IDirect3DTexture2_Release(pCurTex->pD3DCachedTex);
            pCurTex->pD3DCachedTex = NULL;
        }

        pTex = pCurTex;
        pCurTex = pCurTex->pNextCachedTexture;
        pTex->frameNum = 0;
        pTex->pNextCachedTexture = NULL;
        pTex->pPrevCachedTexture = NULL;
    }

    std3D_pFirstTexCache    = NULL;
    std3D_pLastTexCache     = NULL;
    std3D_numCachedTextures = 0;

    if ( std3D_pCurDevice ) {
        std3D_pCurDevice->availableMemory = std3D_pCurDevice->totalMemory;
    }

    if ( std3D_pD3Device )
    {
        HRESULT derr = IDirect3DDevice3_SetTexture(std3D_pD3Device, 0, NULL);
        if ( derr != D3D_OK ) {
#line 1347
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s SetRenderState.\n", std3D_D3DGetStatus(derr), 0, 0, 0); // TODO: error text says SetRenderState, why is that??? Shall SetRenderState be called instead?
        }
    }

    std3D_frameCount = 1;
}

void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture)
{
    pTexture->frameNum = std3D_frameCount;
    std3D_RemoveTextureFromCacheList(pTexture);
    std3D_AddTextureToCacheList(pTexture);
}

/* Discarded by the retail linker (no caller in either build): the debug build has it here (0x004F988A), copying the
   device's leading 13-dword caps block with one struct assignment (rep movsd, ecx = 13). Body from the debug code;
   name from Jones3DRE. */
int J3DAPI std3D_GetDeviceInfo(size_t deviceNum, Device3DCaps* pCaps)
{
    if ( !std3D_numDevices || deviceNum >= std3D_numDevices )
    {
        return 0;
    }

    *pCaps = std3D_aDevices[deviceNum].caps;
    return 1;
}

/* Discarded by the retail linker (no caller in either build): the debug build has it after GetDeviceInfo (0x004F98C9,
   lines 1441/1460), scoring the devices by perspective-correct textures, Z buffer and HAL like stdDisplay's
   FindClosestDevice. Body from the debug code; name from Jones3DRE. */
size_t J3DAPI std3D_FindClosestDevice(const Device3DCaps* pMatch)
{
    int bestScore;
    int curScore;
    size_t bestDevice;
    size_t i;
    Device3D* pDevice;

    if ( !std3D_numDevices ) {
        return 0;
    }

    bestScore  = 0;
    bestDevice = 0;
    pDevice    = std3D_aDevices;
    for ( i = 0; i < std3D_numDevices; ++i )
    {
        curScore = 0;
        if ( !pMatch->bTexturePerspectiveSupported || pDevice->caps.bTexturePerspectiveSupported == pMatch->bTexturePerspectiveSupported )
        {
            ++curScore;
            if ( !pMatch->hasZBuffer || pDevice->caps.hasZBuffer == pMatch->hasZBuffer )
            {
                ++curScore;
                if ( pDevice->caps.bHAL == pMatch->bHAL )
                {
                    ++curScore;
#line 1441
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

#line 1460
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Settling for a closest match #%d..\n", bestDevice, 0, 0, 0);
    return bestDevice;
}

/* Discarded by the retail linker (no caller in either build): debug 0x004F99F7, between FindClosestDevice and
   FindClosestFormat. Returns 0 and the number of the first device that has perspective-correct textures, a Z buffer
   and is a HAL device, else 1. Name from Jones3DRE. */
int J3DAPI std3D_sub_4F99F7(size_t* pDeviceNum)
{
    Device3D* pDevice;
    size_t i;

    if ( !std3D_numDevices ) {
        return 1;
    }

    pDevice = std3D_aDevices;
    for ( i = 0; i < std3D_numDevices; ++i )
    {
        if ( pDevice->caps.bTexturePerspectiveSupported && pDevice->caps.hasZBuffer && pDevice->caps.bHAL )
        {
            *pDeviceNum = i;
            return 0;
        }

        ++pDevice;
    }

    return 1;
}

size_t J3DAPI std3D_FindClosestFormat(const ColorInfo* pMatch)
{
    int bestMatch;
    int matchScore;
    StdTextureFormat* pFormat;
    size_t i;
    size_t closestMatch;

    if ( !std3D_numTextureFormats ) {
        return 0;
    }

    bestMatch = 0;
    closestMatch = 0;
    pFormat = std3D_aTextureFormats;
    for ( i = 0; i < std3D_numTextureFormats; ++i )
    {
        matchScore = 0;
        if ( pFormat->ci.colorMode == pMatch->colorMode )
        {
            ++matchScore;
            if ( pFormat->ci.bpp == pMatch->bpp )
            {
                ++matchScore;
                switch ( pMatch->colorMode )
                {
                    case STDCOLOR_RGB:
                        if ( pFormat->ci.redBPP == pMatch->redBPP && pFormat->ci.greenBPP == pMatch->greenBPP && pFormat->ci.blueBPP == pMatch->blueBPP )
                        {
                            ++matchScore;
                            return i;
                        }
                        break;

                    case STDCOLOR_RGBA:
                        if ( pFormat->ci.colorMode == STDCOLOR_RGBA )
                        {
                            ++matchScore;
                        }

                        if ( pFormat->ci.redBPP == pMatch->redBPP
                            && pFormat->ci.greenBPP == pMatch->greenBPP
                            && pFormat->ci.blueBPP == pMatch->blueBPP
                            && pFormat->ci.alphaBPP == pMatch->alphaBPP )
                        {
                            ++matchScore;
                            return i;
                        }
                        break;

                    default:
                        // Index color mode
                        ++matchScore;
#line 1589
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found a perfect mode match #%d!\n", i, 0, 0, 0);
                        return i;
                }
            }
        }

        if ( matchScore > bestMatch )
        {
            closestMatch   = i;
            bestMatch = matchScore;
        }

        ++pFormat;
    }

#line 1609
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Settling for a closest match #%d..\n", closestMatch, 0, 0, 0);
    return closestMatch;
}

int J3DAPI std3D_InitRenderState(void)
{
    uint32_t rasterCaps;
    HRESULT d3dres;

    std3D_renderState = 0;
    rasterCaps = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwRasterCaps;

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZENABLE, TRUE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_UNKNOWN_1;

    d3dres = std3D_SetMipmapFilter(STD3D_MIPMAPFILTER_BILINEAR);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR) != 0 )
    {
        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        std3D_renderState |= STD3D_RS_TEXFILTER_BILINEAR;
    }

    else if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MAGFPOINT) != 0 )
    {
        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MAGFILTER, D3DTFG_POINT);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MINFILTER, D3DTFN_POINT);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SUBPIXEL, TRUE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    std3D_renderState |= STD3D_RS_SUBPIXEL_CORRECTION;

    d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    if ( (std3D_pCurDevice->d3dDesc.dwTextureOpCaps & D3DTEXOPCAPS_MODULATE) != 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwSrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) != 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwDestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA) != 0 )
    {
        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_COLORARG2, D3DTA_CURRENT);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }

        if ( (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwAlphaCmpCaps & D3DPCMPCAPS_GREATER) != 0 )
        {
            d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATER);
            if ( d3dres != D3D_OK )
            {
                return 0;
            }

            d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHAREF, 0);
            if ( d3dres != D3D_OK )
            {
                return 0;
            }
        }
    }
    else
    {
        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }
    }

    if ( std3D_aTextureFormats[std3D_RGBAKeyTextureFormat].bColorKey
        || std3D_aTextureFormats[std3D_RGBATextureFormat].bColorKey )
    {
        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }
    }

    if ( std3D_pCurDevice->caps.bStippledShadeSupported )
    {
        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_STIPPLEDALPHA, TRUE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }
    }

    else
    {
        d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_STIPPLEDALPHA, FALSE);
        if ( d3dres != D3D_OK )
        {
            return 0;
        }
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_MONOENABLE, FALSE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_SPECULARENABLE, FALSE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    if ( (rasterCaps & D3DPRASTERCAPS_FOGTABLE) == 0
        && (rasterCaps & D3DPRASTERCAPS_FOGVERTEX) == 0 )
    {
        std3D_bRenderFog = 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGENABLE, FALSE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_DITHERENABLE, TRUE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }


    std3D_renderState |= STD3D_RS_UNKNOWN_2;

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_ANTIALIAS, 0);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    d3dres = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    if ( d3dres != D3D_OK )
    {
        return 0;
    }

    return 1;
}

/* Discarded by the retail linker (no caller in either build): debug 0x004FA2CE, just before SetMipmapFilter, returns the
   current mipmap filter. Name from Jones3DRE. */
Std3DMipmapFilterType J3DAPI std3D_sub_4FA2CE(void)
{
    return std3D_mipmapFilter;
}

int J3DAPI std3D_SetMipmapFilter(Std3DMipmapFilterType filter)
{
    int d3dres = 0;
    if ( filter == STD3D_MIPMAPFILTER_TRILINEAR
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR) == 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR) == 0 )
    {
        filter = STD3D_MIPMAPFILTER_BILINEAR;
    }

    if ( filter == STD3D_MIPMAPFILTER_BILINEAR
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPFPOINT) == 0
        && (std3D_pCurDevice->d3dDesc.dpcTriCaps.dwTextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR) == 0 )
    {
        filter = STD3D_MIPMAPFILTER_NONE;
    }

    if ( filter != std3D_mipmapFilter )
    {
        switch ( filter )
        {
            default:
                d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MIPFILTER, D3DTFP_NONE);
                break;
            case STD3D_MIPMAPFILTER_BILINEAR:
                d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MIPFILTER, D3DTFP_POINT);
                break;
            case STD3D_MIPMAPFILTER_TRILINEAR:
                d3dres = IDirect3DDevice3_SetTextureStageState(std3D_pD3Device, 0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
                break;
        }
    }

    std3D_mipmapFilter = filter;
    return d3dres;
}

int J3DAPI std3D_SetProjection(float fov, float nearPlane, float farPlane)
{
    float w;
    float fAspect = 1.0f;
    D3DMATRIX currentproj;
    float h;
    float Q;

    if ( fabs(farPlane - nearPlane) < 0.01f )
    {
        return DDERR_INVALIDPARAMS;
    }

    if ( fabs(sin(fov / 2)) < 0.01f )
    {
        return DDERR_INVALIDPARAMS;
    }

    w = fAspect * (float)(cos(fov / 2) / sin(fov / 2));
    h = 1.0f * (float)(cos(fov / 2) / sin(fov / 2));
    Q = farPlane / (farPlane - nearPlane);

    memset(&currentproj, 0, sizeof(currentproj));
    currentproj._11 = w;
    currentproj._22 = h;
    currentproj._33 = Q;
    currentproj._34 = 1.0f;
    currentproj._43 = -Q * nearPlane;
    std3D_zDepth = (farPlane - nearPlane) * 10.0f;
    return IDirect3DDevice3_SetTransform(std3D_pD3Device, D3DTRANSFORMSTATE_PROJECTION, &currentproj);
}

void J3DAPI std3D_EnableFog(int bEnabled, float density)
{
    DWORD dwRasterCaps = 0;

    std3D_bRenderFog = bEnabled;
    if ( std3D_pCurDevice )
    {
        std3D_fogDensity = density;

        dwRasterCaps = std3D_pCurDevice->d3dDesc.dpcTriCaps.dwRasterCaps;
        if ( (dwRasterCaps & D3DPRASTERCAPS_FOGTABLE) == 0 && (dwRasterCaps & D3DPRASTERCAPS_FOGVERTEX) == 0 ) {
            std3D_bRenderFog = 0;
        }

        if ( (dwRasterCaps & D3DPRASTERCAPS_FOGTABLE) == 0 && std3D_bRenderFog ) {
            std3D_bFogTable = 1;
        }
        else {
            std3D_bFogTable = 0;
        }
    }
    else
    {
        std3D_bFogTable = 0;
    }
}

int J3DAPI std3D_SetFog(float red, float green, float blue, float startDepth, float endDepth)
{
    HRESULT hr;
    float fogEnd;

    std3D_EnableFog(std3D_bRenderFog, std3D_fogDensity);
    hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGCOLOR, D3DRGB(red, green, blue));
    if ( hr != D3D_OK )
    {
        return hr;
    }

    if ( std3D_fogDensity == 0.0f )
    {
        hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
        return hr;
    }

    hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
    if ( hr != D3D_OK )
    {
        return hr;
    }

    fogEnd = endDepth * (2.0 - std3D_fogDensity);
    hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLESTART, *(DWORD*)&startDepth);
    if ( hr != D3D_OK )
    {
        return hr;
    }

    hr = IDirect3DDevice3_SetRenderState(std3D_pD3Device, D3DRENDERSTATE_FOGTABLEEND, *(DWORD*)&fogEnd);
    if ( hr != D3D_OK )
    {
        return hr;
    }

    std3D_fogStartDepth  = startDepth;
    std3D_fogEndDepth    = fogEnd;
    std3D_fogDepthFactor = 1.0f / (fogEnd - startDepth);
}

void J3DAPI std3D_ClearZBuffer(void)
{
    if ( std3D_lpD3DViewPort )
    {
        HRESULT d3dres = IDirect3DViewport3_Clear2(
            std3D_lpD3DViewPort,
            1,                   // dwCount
            &std3D_activeRect,
            D3DCLEAR_ZBUFFER,
            0,                   // color = black
            1.0f,                // dvZ = 1.0f
            0
        );
        if ( d3dres != D3D_OK )
        {
#line 1921
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when clearing Z.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        }
    }
}

int J3DAPI std3D_CreateViewport(void)
{
    HRESULT d3dres = IDirect3D3_CreateViewport(std3D_pDirect3D, &std3D_lpD3DViewPort, NULL);
    if ( d3dres != D3D_OK )
    {
#line 1949
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating D3D viewport.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        return 0;
    }

    d3dres = IDirect3DDevice3_AddViewport(std3D_pD3Device, std3D_lpD3DViewPort);
    if ( d3dres != D3D_OK )
    {
#line 1957
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when adding the D3D viewport to the device.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        return 0;
    }

    {
    D3DVIEWPORT2 d3dviewport;
    memset(&d3dviewport, 0, sizeof(d3dviewport)); // Retail zero-fills (rep stosd), not an addition
    d3dviewport.dwSize       = sizeof(D3DVIEWPORT2);
    d3dviewport.dwX          = 0;
    d3dviewport.dwY          = 0;
    d3dviewport.dwWidth      = stdDisplay_backBuffer.rasterInfo.width;
    d3dviewport.dwHeight     = stdDisplay_backBuffer.rasterInfo.height;
    d3dviewport.dvClipX      = 0.0f;
    d3dviewport.dvClipY      = 0.0f;
    d3dviewport.dvClipWidth  = (float)stdDisplay_backBuffer.rasterInfo.width;
    d3dviewport.dvClipHeight = (float)stdDisplay_backBuffer.rasterInfo.height;
    d3dviewport.dvMinZ       = 0.0f;
    d3dviewport.dvMaxZ       = 1.0f;

    d3dres = IDirect3DViewport3_SetViewport2(std3D_lpD3DViewPort, &d3dviewport);
    if ( d3dres != D3D_OK )
    {
#line 1979
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when creating D3D viewport.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        return 0;
    }

    d3dres = IDirect3DDevice3_SetCurrentViewport(std3D_pD3Device, std3D_lpD3DViewPort);
    if ( d3dres != D3D_OK )
    {
#line 1985
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when setting D3D viewport.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
        return 0;
    }

    std3D_activeRect.x1 = 0;
    std3D_activeRect.y1 = 0;
    std3D_activeRect.x2 = (int32_t)d3dviewport.dvClipWidth;
    std3D_activeRect.y2 = (int32_t)d3dviewport.dvClipHeight;

    d3dres = IDirect3DViewport3_Clear2(
        std3D_lpD3DViewPort,
        1,
        &std3D_activeRect,
        D3DCLEAR_ZBUFFER,
        0,                 // color = black
        1.0f,              // dvZ = 1.0f
        0
    );
    if ( d3dres != D3D_OK )
    {
#line 1997
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error %s when Clearing the Z.\n", std3D_D3DGetStatus(d3dres), 0, 0, 0);
    }


    // TODO: why is stencil buffer being cleared here?
    d3dres = IDirect3DViewport3_Clear2(
        std3D_lpD3DViewPort,
        1,
        &std3D_activeRect,
        D3DCLEAR_STENCIL,
        0,                // color = black
        1.0f,             // dvZ = 1.0f
        0
    );

    return 1;
}
}

void J3DAPI std3D_GetZBufferFormat(DDPIXELFORMAT* pPixelFormat)
{
    if ( !pPixelFormat || !std3D_pDirect3D || !std3D_pCurDevice )
    {
        return;
    }

    memset(pPixelFormat, 0, sizeof(DDPIXELFORMAT));

    pPixelFormat->dwZBufferBitDepth = 0xFFFFFFFFu;
    IDirect3D3_EnumZBufferFormats(std3D_pDirect3D, &std3D_pCurDevice->duid, std3D_EnumZBufferFormatsCallback, pPixelFormat);
}

HRESULT CALLBACK std3D_EnumZBufferFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, DDPIXELFORMAT* lpContext)
{
    if ( !lpDDPixFmt || !lpContext )
    {
        return 0;
    }

    if ( lpDDPixFmt->dwFlags == DDPF_ZBUFFER
        && lpDDPixFmt->dwZBufferBitDepth >= 16 // TODO: try make it 32 bit
        && lpContext->dwZBufferBitDepth > lpDDPixFmt->dwZBufferBitDepth )
    {
        memcpy(lpContext, lpDDPixFmt, sizeof(DDPIXELFORMAT));
    }

    return 1;
}

HRESULT CALLBACK std3D_D3DEnumDevicesCallback(GUID* lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC lpD3DHalDDesc, LPD3DDEVICEDESC lpD3DHelDDesc, LPVOID lpContext)
{
    Device3D* pD3DDriver;

    if ( !lpGuid || !lpD3DHalDDesc || !lpD3DHelDDesc )
    {
        return 0;
    }

    if ( !std3D_bFindAllD3Devices )
    {
        // Skip devices we don't want i.e.: software emulated devices

        if ( memcmp(lpGuid, &IID_IDirect3DNullDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DRGBDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DRampDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DMMXDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }

        if ( memcmp(lpGuid, &IID_IDirect3DRefDevice, sizeof(*lpGuid)) == 0 )
        {
            return 1;
        }
    }

    if ( std3D_numDevices >= STD_ARRAYLEN(std3D_aDevices) )
    {
        return 0;
    }

    pD3DDriver = &std3D_aDevices[std3D_numDevices];
    pD3DDriver->duid = *lpGuid;

    strncpy(pD3DDriver->deviceDescription, lpDeviceDescription, STD_ARRAYLEN(pD3DDriver->deviceDescription) - 1);
    pD3DDriver->deviceDescription[STD_ARRAYLEN(pD3DDriver->deviceDescription) - 1] = '\0';
    strncpy(pD3DDriver->deviceName, lpDeviceName, STD_ARRAYLEN(pD3DDriver->deviceName) - 1);
    pD3DDriver->deviceName[STD_ARRAYLEN(pD3DDriver->deviceName) - 1] = '\0';

    pD3DDriver->caps.bHAL = lpD3DHalDDesc->dwFlags != 0;
    if ( pD3DDriver->caps.bHAL ) {
        memcpy(&pD3DDriver->d3dDesc, lpD3DHalDDesc, sizeof(pD3DDriver->d3dDesc));
    }
    else {
        memcpy(&pD3DDriver->d3dDesc, lpD3DHelDDesc, sizeof(pD3DDriver->d3dDesc));
    }

    {
    StdDisplayDevice device;
    if ( stdDisplay_GetCurrentDevice(&device) )
    {
        return 0;
    }

    if ( !device.bGuidNotSet && !pD3DDriver->caps.bHAL )
    {
        return 1;
    }

    pD3DDriver->caps.bTexturePerspectiveSupported = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) != 0;
    pD3DDriver->caps.hasZBuffer                   = pD3DDriver->d3dDesc.dwDeviceZBufferBitDepth != 0;
    pD3DDriver->caps.bSqareOnlyTexture            = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0;
    pD3DDriver->caps.bAlphaTextureSupported       = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_ALPHA) != 0;

    if ( (pD3DDriver->d3dDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAFLATBLEND) != 0 )
    {
        pD3DDriver->caps.bStippledShadeSupported = 0;
    }
    else if ( (pD3DDriver->d3dDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAFLATSTIPPLED) != 0 )
    {
        pD3DDriver->caps.bStippledShadeSupported = 1;
    }
    else
    {
        pD3DDriver->caps.bStippledShadeSupported = 0;
    }

    pD3DDriver->caps.bAlphaBlendSupported =
        (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATEALPHA) != 0 &&
        (pD3DDriver->d3dDesc.dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_ALPHAGOURAUDBLEND) != 0 ||
        pD3DDriver->caps.bStippledShadeSupported;
    pD3DDriver->caps.bColorkeyTextureSupported    = (pD3DDriver->d3dDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_TRANSPARENCY) != 0;

    if ( !pD3DDriver->d3dDesc.dwMinTextureWidth )
    {
        pD3DDriver->caps.minTexWidth  = 16;
        pD3DDriver->caps.minTexHeight = 16;
        pD3DDriver->caps.maxTexWidth  = 256;
        pD3DDriver->caps.maxTexHeight = 256;
    }
    else
    {
        pD3DDriver->caps.minTexWidth  = pD3DDriver->d3dDesc.dwMinTextureWidth;
        pD3DDriver->caps.minTexHeight = pD3DDriver->d3dDesc.dwMinTextureHeight;
        pD3DDriver->caps.maxTexWidth  = pD3DDriver->d3dDesc.dwMaxTextureWidth;
        pD3DDriver->caps.maxTexHeight = pD3DDriver->d3dDesc.dwMaxTextureHeight;
    }

    pD3DDriver->caps.maxVertexCount = pD3DDriver->d3dDesc.dwMaxVertexCount;

#line 2169
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found |%s|%s|%s|%s| D3D Device\n", pD3DDriver->caps.hasZBuffer ? "Z" : "Non-Z", pD3DDriver->caps.bAlphaTextureSupported ? "Alpha" : "No Alpha", pD3DDriver->caps.bStippledShadeSupported ? "Stippled" : "Blend", pD3DDriver->caps.bColorkeyTextureSupported ? "Colorkey" : "No Colorkey");

#line 2170
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Description: %s [%s]\n", pD3DDriver->deviceName, pD3DDriver->deviceDescription, 0, 0);

    ++std3D_numDevices;
    return 1;
}
}

HRESULT CALLBACK std3D_EnumTextureFormatsCallback(LPDDPIXELFORMAT lpDDPixFmt, LPVOID lpContext)
{
    StdTextureFormat* pTexFormat;
    uint32_t curBitmask;
    int bits;

    if ( std3D_numTextureFormats < STD_ARRAYLEN(std3D_aTextureFormats) )
    {


    pTexFormat = &std3D_aTextureFormats[std3D_numTextureFormats];
    memcpy(&pTexFormat->ddPixelFmt, lpDDPixFmt, sizeof(pTexFormat->ddPixelFmt));

    if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED8) != 0 )
    {
#line 2210
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found 8 bpp palettized tex format.\n", 0, 0, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED4) != 0 )
    {
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found 4 bpp palettized tex format.\n", 0, 0, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED2) != 0 )
    {
#line 2218
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found 2 bpp palettized tex format.\n", 0, 0, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_PALETTEINDEXED1) != 0 )
    {
#line 2222
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found 1 bpp palettized tex format.\n", 0, 0, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_LUMINANCE) != 0 )
    {
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found Luminance Alpha tex format (%d:%d).\n", lpDDPixFmt->dwRGBBitCount, lpDDPixFmt->dwRGBBitCount, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_ALPHA) != 0 )
    {
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found Alpha tex format. (%d)\n", lpDDPixFmt->dwRGBBitCount, 0, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_FOURCC) != 0 )
    {
        char aFourCC[8];
        memcpy(aFourCC, &lpDDPixFmt->dwFourCC, 4u);
        aFourCC[4] = 0;
#line 2238
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found FourCC tex format (%s)\n", aFourCC, 0, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_BUMPDUDV) != 0 )
    {
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found Luminance BumpMap UV tex format (%d:%d)\n", lpDDPixFmt->dwRGBBitCount, lpDDPixFmt->dwRGBBitCount, 0, 0);
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_ALPHAPIXELS) != 0 )
    {
        std3D_bHasRGBTextureFormat = 1;
        pTexFormat->ci.colorMode = STDCOLOR_RGBA;
        pTexFormat->ci.bpp = lpDDPixFmt->dwRGBBitCount;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwRBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.redPosShift = bits;
        pTexFormat->ci.redPosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwRBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.redBPP = bits;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwGBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.greenPosShift = bits;
        pTexFormat->ci.greenPosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwGBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.greenBPP = bits;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwBBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.bluePosShift = bits;
        pTexFormat->ci.bluePosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwBBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.blueBPP = bits;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwRGBAlphaBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.alphaPosShift = bits;
        pTexFormat->ci.alphaPosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwRGBAlphaBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.alphaBPP = bits;


#line 2281
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found RGBA tex format (%d:%d:%d:%d).\n", pTexFormat->ci.redBPP, pTexFormat->ci.greenBPP, pTexFormat->ci.blueBPP, pTexFormat->ci.alphaBPP);
        ++std3D_numTextureFormats;
    }
    else if ( (lpDDPixFmt->dwFlags & DDPF_RGB) != 0 )
    {
        std3D_bHasRGBTextureFormat = 1;
        pTexFormat->ci.colorMode = STDCOLOR_RGB;
        pTexFormat->ci.bpp = lpDDPixFmt->dwRGBBitCount;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwRBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.redPosShift = bits;
        pTexFormat->ci.redPosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwRBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.redBPP = bits;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwGBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.greenPosShift = bits;
        pTexFormat->ci.greenPosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwGBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.greenBPP = bits;

        bits = 0;
        for ( curBitmask = lpDDPixFmt->dwBBitMask; (curBitmask & 1) == 0; curBitmask >>= 1 )
        {
            ++bits;
        }

        pTexFormat->ci.bluePosShift = bits;
        pTexFormat->ci.bluePosShiftRight = stdCalcBitPos(255 / (lpDDPixFmt->dwBBitMask >> bits));

        bits = 0;
        while ( (curBitmask & 1) != 0 )
        {
            ++bits;
            curBitmask >>= 1;
        }

        pTexFormat->ci.blueBPP = bits;


        pTexFormat->ci.alphaPosShift      = 0;
        pTexFormat->ci.alphaPosShiftRight = 0;
        pTexFormat->ci.alphaBPP           = 0;

#line 2319
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found RGB tex format (%d:%d:%d).\n", pTexFormat->ci.redBPP, pTexFormat->ci.greenBPP, pTexFormat->ci.blueBPP, 0);
        ++std3D_numTextureFormats;
    }
    else
    {
#line 2323
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Found Unknown tex format.\n", 0, 0, 0, 0);
    }

    return 1;
    }

    return 0;
}

//
void J3DAPI std3D_AddTextureToCacheList(tSystemTexture* pTexture)
{
    if ( !std3D_pFirstTexCache )
    {
        std3D_pFirstTexCache         = std3D_pLastTexCache = pTexture;
        pTexture->pPrevCachedTexture = NULL;
        pTexture->pNextCachedTexture = NULL;
    }
    else
    {
        std3D_pLastTexCache->pNextCachedTexture = pTexture;
        pTexture->pPrevCachedTexture            = std3D_pLastTexCache;
        pTexture->pNextCachedTexture            = NULL;
        std3D_pLastTexCache                     = pTexture;
    }

    ++std3D_numCachedTextures;
    std3D_pCurDevice->availableMemory -= pTexture->textureSize;
}

void J3DAPI std3D_RemoveTextureFromCacheList(tSystemTexture* pCacheTexture)
{
    if ( pCacheTexture == std3D_pFirstTexCache )
    {
        std3D_pFirstTexCache = pCacheTexture->pNextCachedTexture;
        if ( std3D_pFirstTexCache )
        {
            std3D_pFirstTexCache->pPrevCachedTexture = NULL;
            if ( !std3D_pFirstTexCache->pNextCachedTexture ) {
                std3D_pLastTexCache = std3D_pFirstTexCache;
            }
        }
        else {
            std3D_pLastTexCache = NULL;
        }
    }
    else if ( pCacheTexture == std3D_pLastTexCache )
    {
        std3D_pLastTexCache = pCacheTexture->pPrevCachedTexture;
        std3D_pLastTexCache->pNextCachedTexture = NULL;
    }
    else
    {
        pCacheTexture->pPrevCachedTexture->pNextCachedTexture = pCacheTexture->pNextCachedTexture;
        pCacheTexture->pNextCachedTexture->pPrevCachedTexture = pCacheTexture->pPrevCachedTexture;
    }

    --std3D_numCachedTextures;
    std3D_pCurDevice->availableMemory += pCacheTexture->textureSize;

    pCacheTexture->pNextCachedTexture = NULL;
    pCacheTexture->pPrevCachedTexture = NULL;
    pCacheTexture->frameNum = 0;
}

int J3DAPI std3D_PurgeTextureCache(size_t size)
{
    tSystemTexture* pNextCachedTexture = NULL;
    size_t purgedBytes = 0;
    tSystemTexture* pCacheTexture;

    for ( pCacheTexture = std3D_pFirstTexCache; pCacheTexture && pCacheTexture->frameNum != std3D_frameCount; pCacheTexture = pNextCachedTexture )
    {
        pNextCachedTexture = pCacheTexture->pNextCachedTexture;
        if ( pCacheTexture->textureSize == size )
        {
            IDirect3DTexture2_Release(pCacheTexture->pD3DCachedTex);
            pCacheTexture->pD3DCachedTex = NULL;
            purgedBytes += pCacheTexture->textureSize;
            std3D_RemoveTextureFromCacheList(pCacheTexture);
            return 1;
        }
    }

    pCacheTexture = std3D_pFirstTexCache;
    while ( pCacheTexture && purgedBytes < size )
    {
        pNextCachedTexture = pCacheTexture->pNextCachedTexture;
        if ( pCacheTexture->frameNum != std3D_frameCount )
        {
            IDirect3DTexture2_Release(pCacheTexture->pD3DCachedTex);
            pCacheTexture->pD3DCachedTex = NULL;
            purgedBytes += pCacheTexture->textureSize;
            std3D_RemoveTextureFromCacheList(pCacheTexture);
        }

        pCacheTexture = pNextCachedTexture;
    }

    return purgedBytes > 0;
}

const char* J3DAPI std3D_D3DGetStatus(HRESULT res)
{
    const char* pText = "Unknown Error";
    int numCodes = STD_ARRAYLEN(std3D_aD3DStatusTbl);
    int i;

    for ( i = 0; i < numCodes; ++i )
    {
        if ( std3D_aD3DStatusTbl[i].code == res )
        {
            pText = std3D_aD3DStatusTbl[i].text;
            break;
        }
    }

    return pText;
}

StdDisplayEnvironment* J3DAPI std3D_BuildDisplayEnvironment(void)
{
    int bStarted;
    size_t deviceNum;
    StdVideoMode* pCurMode;
    size_t modeNum;
#line 2681
    StdDisplayEnvironment* pDeviceList = (StdDisplayEnvironment*)STDMALLOC(sizeof(StdDisplayEnvironment));
    memset(pDeviceList, 0, sizeof(StdDisplayEnvironment));

    bStarted = stdDisplay_Startup();
    if ( !bStarted )
    {
#line 2688
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error starting stdDisplay system.\n", 0, 0, 0, 0);
        std3D_FreeDisplayEnvironment(pDeviceList);
        return NULL;
    }

    pDeviceList->numInfos      = stdDisplay_GetNumDevices();
    pDeviceList->aDisplayInfos = NULL;

    if ( pDeviceList->numInfos )
    {
        StdDisplayInfo* pCurInfo;

#line 2699
        pDeviceList->aDisplayInfos = (StdDisplayInfo*)STDMALLOC(sizeof(StdDisplayInfo) * pDeviceList->numInfos);

        pCurInfo = pDeviceList->aDisplayInfos;
        for ( deviceNum = 0; deviceNum < pDeviceList->numInfos; ++deviceNum )
        {
            memset(pCurInfo, 0, sizeof(StdDisplayInfo));

            if ( stdDisplay_GetDevice(deviceNum, &pCurInfo->displayDevice) )
            {
#line 2709
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error getting stdDisplay device.\n", 0, 0, 0, 0);
                std3D_FreeDisplayEnvironment(pDeviceList);
                return NULL;
            }

            bStarted = stdDisplay_Open(deviceNum);
            if ( !bStarted )
            {
#line 2717
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error opening stdDisplay device.\n", 0, 0, 0, 0);
                std3D_FreeDisplayEnvironment(pDeviceList);
                return NULL;
            }

            pCurInfo->numModes = stdDisplay_GetNumVideoModes();
            pCurInfo->aModes   = NULL;

            if ( pCurInfo->numModes )
            {
#line 2728
                pCurInfo->aModes = (StdVideoMode*)STDMALLOC(sizeof(StdVideoMode) * pCurInfo->numModes);
                pCurMode = pCurInfo->aModes;
                for ( modeNum = 0; modeNum < pCurInfo->numModes; ++modeNum )
                {
                    if ( !stdDisplay_GetVideoMode(modeNum, pCurMode) )
                    {
                        ++pCurMode;
                    }
                }

                if ( pCurInfo->displayDevice.bHAL )
                {
                    bStarted = std3D_Startup();
                    if ( !bStarted )
                    {
#line 2742
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error starting std3D system.\n", 0, 0, 0, 0);
                        std3D_FreeDisplayEnvironment(pDeviceList);
                        return NULL;
                    }

                    pCurInfo->numDevices = std3D_GetNumDevices();
                    pCurInfo->aDevices   = NULL;
                    if ( pCurInfo->numDevices )
                    {
#line 2753
                        pCurInfo->aDevices = (Device3D*)STDMALLOC(sizeof(Device3D) * pCurInfo->numDevices);
                        memcpy(pCurInfo->aDevices, std3D_GetAllDevices(), sizeof(Device3D) * pCurInfo->numDevices);
                    }

                    std3D_Shutdown();
                }
                else
                {
                    pCurInfo->numDevices = 0;
                    pCurInfo->aDevices   = NULL;
                }
            }

            stdDisplay_Close();
            ++pCurInfo;
        }
    }

    stdDisplay_Shutdown();
    return pDeviceList;
}

void J3DAPI std3D_FreeDisplayEnvironment(StdDisplayEnvironment* pEnv)
{
    size_t i;

    if ( pEnv->aDisplayInfos )
    {
        StdDisplayInfo* pCurDevice = pEnv->aDisplayInfos;
        for ( i = 0; i < pEnv->numInfos; ++i )
        {
            if ( pCurDevice->aDevices )
            {
                stdMemory_Free(pCurDevice->aDevices);
                pCurDevice->aDevices = NULL;
            }

            if ( pCurDevice->aModes )
            {
                stdMemory_Free(pCurDevice->aModes);
                pCurDevice->aModes = NULL;
            }

            ++pCurDevice;
        }

        if ( pEnv->aDisplayInfos )
        {
            stdMemory_Free(pEnv->aDisplayInfos);
            pEnv->aDisplayInfos = NULL;
        }
    }

    stdMemory_Free(pEnv);
}

void J3DAPI std3D_SetFindAllDevices(int bFindAll)
{
    std3D_bFindAllD3Devices = bFindAll;
}