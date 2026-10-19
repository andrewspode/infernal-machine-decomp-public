#include "sithRender.h"
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithRender_countfit_7, 5) CF_PROTO(sithRender_countfit_8, 0) /* end count-fit */
#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdKeyframe.h>
#include <rdroid/Engine/rdLight.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Primitives/rdPrimit3.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/AI/sithAIUtil.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Engine/sithRenderSky.h>
#include <sith/Engine/sithShadow.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithPVS.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include  <std/General/stdEffect.h>
#include  <std/General/stdFileUtil.h>
#include  <std/General/stdFnames.h>
#include  <std/General/stdMath.h>
#include  <std/General/stdMemory.h>
#include  <std/General/stdUtil.h>
#include  <std/Win95/std3D.h>
#include  <std/Win95/stdDisplay.h>

#include <math.h>
#include <j3dcore/types.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithRender_countfit_1, 11)
CF_PROTO(sithRender_countfit_2, 5)
CF_PROTO(sithRender_countfit_3, 0)

/* end count-fit */

#define SITHRENDER_MAXTHINGLIGHTS  RDCAMERA_MAX_LIGHTS / 2 // 64; note this var must not exceed RDCAMERA_MAX_LIGHTS-1
#define SITHRENDER_MAXSECTORLIGHTS (RDCAMERA_MAX_LIGHTS - SITHRENDER_MAXTHINGLIGHTS)

// There are 2 types of thing light the dynamic light that affect emitting thing and surrounding area,
// and there is flat light that lits only the emitting thing.
#define SITHRENDER_FLATLIGHRANGE          0.01f
#define SITHRENDER_ISDYNAMICLIGHT(range) ((range) > SITHRENDER_FLATLIGHRANGE)
#define SITHRENDER_ISFLATLIGHT(range )   ((range) <= SITHRENDER_FLATLIGHRANGE)

#define SITHRENDER_MAXSCREENSHOTS 100u

/* Retail's private .bss block is 0x005647C8-0x0056A6B8. Every object below carries
   an explicit zero initializer, so VC5 emits the whole block in declaration order
   (the order here is retail's address order). The public globals in it are real
   zero-initialized definitions, not tentative ones: a tentative definition becomes a
   COMMON request the linker satisfies in the image's far common tail, which is where
   sithRender_lightMode and sithRender_numVisibleSectors really sit. The four
   sithRender_aUnused and unusedState objects are unreferenced original storage.
 */

static SithSector* sithRender_aVisibleSectors[SITHRENDER_MAX_VISIBLE_SECTORS] = { 0 };

/* Unreferenced original storage: 8 bytes of 8-aligned .bss at 0x005649C8. */
static uint8_t sithRender_aUnused_5649C8[8] = { 0 };

static rdVector3 sithRender_aClipVertices[MAX_CLIP_VERTICIES] = { 0 };
static size_t sithRender_lastPVSIndex = 0;

/* 4-byte alignment hole at 0x00564D94 before the 8-aligned rdPrimit3. */
static rdPrimit3 sithRender_clipFaceView = { 0 };
static SithSurfaceAdjoin* sithRender_aVisibleAdjoins[SITHRENDER_MAX_VISIBLE_SECTORS] = { 0 };

/* Unreferenced original storage: one 4-byte slot at 0x00564FB8. */
static int sithRender_unusedState0 = 0;

static size_t sithRender_curPVSIndex = 0;
int sithRender_renderflags = 0;
static size_t sithRender_numSectorPointLights = 0;
static size_t sithRender_numSpritesToDraw = 0;
static size_t sithRender_numAlphaAdjoins = 0;

/* Unreferenced original storage: 1,024 bytes of 8-aligned .bss at 0x00564FD0. */
static uint8_t sithRender_aUnused_564FD0[1024] = { 0 };

static rdVector3 sithRender_aTransformedClipVertices[MAX_CLIP_VERTICIES] = { 0 };
static size_t sithRender_numThingSectors = 0;

/* 4-byte alignment hole at 0x00565794 before the 8-aligned rdPrimit3. */
static rdPrimit3 sithRender_faceView = { 0 };
static rdLight sithRender_aSectorPointLights[SITHRENDER_MAXSECTORLIGHTS] = { 0 };
static rdLight sithRender_aThingLights[SITHRENDER_MAXTHINGLIGHTS] = { 0 };
static SithSector* sithRender_aThingSectors[SITHRENDER_MAX_SECTORS_WITH_THINGS] = { 0 };
static size_t sithRender_numSecorFrustrums = 0;

/* 4-byte alignment hole at 0x005673BC before the 8-aligned frustum array. */
static rdClipFrustum sithRender_aSectorFrustrums[SITHRENDER_MAX_VISIBLE_SECTORS] = { 0 };

/* Unreferenced original storage: one 4-byte slot at 0x0056A1C0. */
static int sithRender_unusedState1 = 0;

static size_t sithRender_curCamSectorIdx = 0; // Not used for anything particular
static SithSurface* sithRender_aAlphaAdjoins[SITHRENDER_MAX_VISIBLE_SECTORS / 2] = { 0 };
static rdVector3 sithRender_aSurfaceTransformedVertices[MAX_CLIP_VERTICIES] = { 0 };
static size_t sithRender_numThingLights = 0;
size_t sithRender_numRenderedSectors = 0;
size_t sithRender_numDrawnThings = 0;
size_t sithRender_numArchPolys = 0;
size_t sithRender_numAlphaArchPolys = 0;
size_t sithRender_numThingPolys = 0;
size_t sithRender_numAlphaThingPoly = 0;
size_t sithRender_numVisibleAdjoins = 0;
static size_t sithRender_numVisibleThingSectors = 0;
int sithRender_bResetCameraAspect = 0;
static int (J3DAPI* sithRender_pExtraThingRenderFunc)(SithThing* pThing) = NULL;
static uint8_t* sithRender_aAdjoinTable = NULL;

/* Tentative definitions: the linker satisfies these two COMMON requests in the
   image's far common tail at 0x017F9800/0x017F9804, outside the block above. */
rdLightMode sithRender_lightMode;
size_t sithRender_numVisibleSectors;

float sithRender_fogDensity = 100.0f;
static int sithRender_bPVSClipEnabled = 1;

// Max distance from the camera sector at which a sector's things are still collected
static float sithRender_maxThingSectorDistance = 8.0f;

int J3DAPI sithRender_Startup(void)
{
    rdMaterial_RegisterLoader(sithMaterial_Load);
    rdModel3_RegisterLoader(sithModel_Load);
    rdKeyframe_RegisterLoader(sithPuppet_LoadKeyframe);

    sithRender_renderflags           = 0;
    sithRender_pExtraThingRenderFunc = NULL;
    sithRender_aAdjoinTable          = NULL;
    return 1;
}

int J3DAPI sithRender_Open(void)
{
    int i;

    sithRender_lightMode = RD_LIGHTING_GOURAUD;

    for ( i = 0; i < SITHRENDER_MAXTHINGLIGHTS; ++i ) {
        rdLight_NewEntry(&sithRender_aThingLights[i]);
    }

    // TODO: Verify if sithRender_aSectorPointLights array also has to be initialized

    if ( sithRenderSky_Open(sithWorld_pCurrentWorld->horizonDistance, sithWorld_pCurrentWorld->ceilingSkyHeight) )
    {
#line 245
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to Open sithRenderSky.\n", 0, 0, 0, 0); // every sithRender diagnostic goes through rdroid_pHS (retail)
        goto error;
    }

    sithRender_aAdjoinTable = (uint8_t*)STDMALLOC(sithWorld_pCurrentWorld->numAdjoins + 1);
    if ( !sithRender_aAdjoinTable )
    {
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to Alloc Adjoin Table.\n", 0, 0, 0, 0); // every sithRender diagnostic goes through rdroid_pHS (retail)
        goto error;
    }

    sithRender_bResetCameraAspect = 0;
    return 0;

    // Debug 0x453AC5: one `return 1` block after the success return, reached from both prints (0x453A5A, 0x453AB5);
    // the debug build's label.
error:
    return 1;
}

void J3DAPI sithRender_Close(void)
{
    if ( sithRender_aAdjoinTable ) {
        stdMemory_Free(sithRender_aAdjoinTable);
    }

    sithRender_aAdjoinTable = NULL;
    sithRenderSky_Close();
}

void J3DAPI sithRender_Shutdown(void)
{
    sithRender_Close();
    sithRender_pExtraThingRenderFunc = NULL;
    sithRender_aAdjoinTable          = NULL;
}

void J3DAPI sithRender_SetRenderFlags(int flags)
{
    sithRender_renderflags = flags;
}

int J3DAPI sithRender_GetRenderFlags(void)
{
    return sithRender_renderflags;
}

void J3DAPI sithRender_SetLightingMode(rdLightMode mode)
{
    sithRender_lightMode = mode;
}

void J3DAPI sithRender_RenderScene(void)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    sithRenderSky_Update();

    if ( (pWorld->state & SITH_WORLD_STATE_UPDATE_FOG) != 0 )
    {
        std3D_fogDensity = sithRender_fogDensity / 100.0f;
        std3D_SetFog(pWorld->fog.color.red, pWorld->fog.color.green, pWorld->fog.color.blue, pWorld->fog.startDepth, pWorld->fog.endDepth);
        pWorld->state &= ~SITH_WORLD_STATE_UPDATE_FOG;
    }

    if ( sithTime_IsPaused() ) {
        stdEffect_SetFadeFactor(1, 0.5f);
    }

    sithRender_Draw();
    stdEffect_SetFadeFactor(0, 1.0f);
    sithVoice_Draw();
    sithConsole_Flush();
}

#ifdef J3D_DEBUG
/* Debug build only: retail reads sithRender_bPVSClipEnabled (Draw) but no retail code writes it. The debug build
   has this toggle here, between RenderScene and Draw (0x00453BF7), called only by JonesConsole's debug-only "pvs"
   console command (0x00413333), which is why sithRender.h declares it.
   Body from the debug code; name from the debug build's text and Jones3DRE
 */
void J3DAPI sithRender_TogglePVS(void)
{
    sithRender_bPVSClipEnabled = !sithRender_bPVSClipEnabled;
}
#endif

void J3DAPI sithRender_Draw(void)
{
    float sin;
    float cos;

    rdSetLightingMode(sithRender_lightMode);
    rdSetRenderOptions(rdGetRenterOptions() | RDROID_USE_AMBIENT_CAMERA_LIGHT);

    // Set camera
    if ( (sithCamera_pCurCamera->pSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        // Little underwater effect
        stdMath_SinCos(sithTime_secGameTime * 70.0f, &sin, &cos);
        rdCamera_SetFOV(&sithCamera_pCurCamera->rdCamera, D3DVAL(sin * 1.0f) + sithCamera_pCurCamera->fov);

        stdMath_SinCos(sithTime_secGameTime * 100.0f, &sin, &cos);
        rdCamera_SetAspectRatio(&sithCamera_pCurCamera->rdCamera, D3DVAL(sin / 30.0f) + sithCamera_pCurCamera->aspectRatio);

        sithRender_bResetCameraAspect = 1;
    }
    else if ( sithRender_bResetCameraAspect )
    {
        rdCamera_SetFOV(&sithCamera_pCurCamera->rdCamera, sithCamera_pCurCamera->fov);
        rdCamera_SetAspectRatio(&sithCamera_pCurCamera->rdCamera, sithCamera_pCurCamera->aspectRatio);
        sithRender_bResetCameraAspect = 0;
    }

    sithRender_numVisibleSectors    = 0;
    sithRender_numThingSectors        = 0;
    sithRender_numThingLights         = 0;
    sithRender_numSecorFrustrums      = 0;
    sithRender_numAlphaAdjoins        = 0;
    sithRender_numSpritesToDraw       = 0;
    sithRender_numVisibleThingSectors = 0;
    sithRender_numRenderedSectors     = 0;
    sithRender_numDrawnThings       = 0;
    sithRender_numVisibleAdjoins    = 0;

    rdCamera_ClearLights(rdCamera_pCurCamera);

    sithRender_curCamSectorIdx = sithCamera_pCurCamera->pSector - sithWorld_pCurrentWorld->aSectors; // TODO: ?? redundant

    // Collect sector and things to draw
    if ( sithRender_bPVSClipEnabled && sithWorld_pCurrentWorld->aPVS )
    {
        sithRender_PVSBuildVisibleSectorList(sithCamera_pCurCamera->pSector, rdCamera_pCurCamera->pFrustum);
    }
    else
    {
        sithRender_BuildVisibleSectorList(sithCamera_pCurCamera->pSector, rdCamera_pCurCamera->pFrustum);
    }

    sithRender_BuildVisibleSectorsThingList();
    if ( sithRender_numVisibleThingSectors > STD_ARRAYLEN(sithRender_aThingSectors) )
    {
#line 511
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many sectors with things in view %d of %d\n", STD_ARRAYLEN(sithRender_aThingSectors), sithRender_numVisibleThingSectors, 0, 0); // every sithRender diagnostic goes through rdroid_pHS (retail)
    }

    if ( (sithRender_renderflags & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
    {
        sithRender_BuildDynamicLights();
    }

    // Set projection
    std3D_SetProjection(sithCamera_pCurCamera->rdCamera.fov, sithCamera_pCurCamera->rdCamera.pFrustum->nearPlane, sithCamera_pCurCamera->rdCamera.pFrustum->farPlane);

    // Now draw everything
    sithRender_RenderSectors();

    if ( sithRender_numThingSectors != 0 )
    {
        sithRender_RenderThings();
    }

    if ( sithRender_numAlphaAdjoins != 0 )
    {
        sithRender_RenderAlphaAdjoins();
    }

    sithAIUtil_RenderAIWaypoints();
}

void J3DAPI sithRender_BuildVisibleSectorList(SithSector* pSector, rdClipFrustum* pFrustrum)
{
    int bBuildingSector;
    SithSurfaceAdjoin* pAdjoin;

    if ( pSector->renderTick != sithMain_curRenderTick )
    {
        sithRender_BuildVisibleSector(pSector, pFrustrum);
    }
    else
    {
        pSector->pClipFrustum = rdCamera_pCurCamera->pFrustum;
    }

    bBuildingSector = pSector->bBuildingSector;
    pSector->bBuildingSector   = 1;
    sithRender_faceView.aVertices = sithWorld_pCurrentWorld->aTransformedVertices;

    pAdjoin = pSector->pFirstAdjoin;
    while ( pAdjoin )
    {
        if ( !pAdjoin->pAdjoinSector->bBuildingSector )
        {
            SithSurface* pSurface;
            rdFace* pFace;
            rdVector3 lookDir;
            float dot;
            size_t numVertices;

            ++sithRender_numVisibleAdjoins;

            pSurface = pAdjoin->pAdjoinSurface;
            pFace    = &pSurface->face;

            rdVector_Sub3(&lookDir, &sithCamera_pCurCamera->lookPos, &sithWorld_pCurrentWorld->aVertices[*pFace->aVertices]);
            dot = rdVector_Dot3(&pFace->normal, &lookDir);
            if ( dot > 0.0f || (dot == 0.0f && pSector == sithCamera_pCurCamera->pSector) )
            {
                sithRender_BuildVisibleSurface(pSurface);

                sithRender_clipFaceView.aVertices  = sithRender_aClipVertices;
                sithRender_faceView.numVertices    = pFace->numVertices;
                sithRender_faceView.aVertIdxs      = pFace->aVertices;
                rdPrimit3_ClipFace(pFrustrum, RD_GEOMETRY_WIREFRAME, RD_LIGHTING_LIT, &sithRender_faceView, &sithRender_clipFaceView, &pFace->texVertOffset);

                numVertices = sithRender_clipFaceView.numVertices;
                if ( (numVertices >= 3 || (rdClip_faceStatus & 0x40) != 0)
                    && ((rdClip_faceStatus & 0x41) != 0
                        || (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0
                        && (!pFace->pMaterial || pFace->geometryMode == RD_GEOMETRY_NONE
                            || (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0
                            || pFace->pMaterial->formatType != STDCOLOR_FORMAT_RGB)) )
                {
                    rdClipFrustum frustrum;
                    rdClipFrustum* pAdjoinFrustrum;

                    rdCamera_pCurCamera->pfProjectList(sithRender_aTransformedClipVertices, sithRender_aClipVertices, numVertices);
                    if ( (rdClip_faceStatus & 0x41) != 0 )
                    {
                        pAdjoinFrustrum = pFrustrum;
                    }
                    else
                    {
                        pAdjoinFrustrum = &frustrum;
                        sithRender_BuildClipFrustrum(pAdjoinFrustrum, numVertices, 3.4028235e38f, 3.4028235e38f, -3.4028235e38f, -3.4028235e38f);
                    }

                    sithRender_BuildVisibleSectorList(pAdjoin->pAdjoinSector, pAdjoinFrustrum);
                }
            }
        }

        pAdjoin = pAdjoin->pNextAdjoin;
    }

    pSector->bBuildingSector = bBuildingSector;
}

void J3DAPI sithRender_BuildVisibleSurface(SithSurface* pSurface)
{
    rdFace* pFace;
    size_t i;
    size_t vertIndex;

    pFace = &pSurface->face;
    if ( pSurface->renderTick != sithMain_curRenderTick )
    {
        for ( i = 0; i < pFace->numVertices; ++i )
        {
            vertIndex = pFace->aVertices[i];
            if ( sithWorld_pCurrentWorld->aVertexRenderTickIds[vertIndex] != sithMain_curRenderTick )
            {
                rdMatrix_TransformPoint34(
                    &sithWorld_pCurrentWorld->aTransformedVertices[vertIndex],
                    &sithWorld_pCurrentWorld->aVertices[vertIndex],
                    &rdCamera_pCurCamera->orient
                );
                sithWorld_pCurrentWorld->aVertexRenderTickIds[vertIndex] = sithMain_curRenderTick;
            }
        }

        pSurface->renderTick = sithMain_curRenderTick;
    }
}

void J3DAPI sithRender_BuildClipFrustrum(rdClipFrustum* pFrustrum, size_t numVertices, float orthLeft, float orthTop, float orthRight, float orthBottom)
{
    size_t idx;
    float x;
    float y;

    for ( idx = 0; idx < numVertices; ++idx )
    {
        x = sithRender_aTransformedClipVertices[idx].x;
        y = sithRender_aTransformedClipVertices[idx].y;
        orthLeft   = J3DMIN(orthLeft, x);
        orthRight  = J3DMAX(orthRight, x);
        orthTop    = J3DMIN(orthTop, y);
        orthBottom = J3DMAX(orthBottom, y);
    }

    pFrustrum->orthoLeftPlane   = orthLeft;
    pFrustrum->orthoTopPlane    = orthTop;
    pFrustrum->orthoRightPlane  = orthRight;
    pFrustrum->orthoBottomPlane = orthBottom;

    rdCamera_SetFrustrum(rdCamera_pCurCamera, pFrustrum, (int)(ceilf(orthLeft) + 0.5f), (int)(ceilf(orthTop) + 0.5f), (int)ceilf(orthRight), (int)ceilf(orthBottom));
}

void J3DAPI sithRender_PVSBuildVisibleSectorList(SithSector* pSector, rdClipFrustum* pClipFrustum)
{
    uint32_t width, height;

    sithPVS_SetTable(sithRender_aAdjoinTable, &sithWorld_pCurrentWorld->aPVS[pSector->pvsIdx], sithWorld_pCurrentWorld->numAdjoins);
    memset(sithRender_aVisibleAdjoins, 0, sizeof(sithRender_aVisibleAdjoins));

    sithRender_curPVSIndex = 1;

    stdDisplay_GetBackBufferSize(&width, &height);
    sithRender_faceView.aVertices = sithWorld_pCurrentWorld->aTransformedVertices;

    pClipFrustum->orthoLeftPlane   = 0.0f;
    pClipFrustum->orthoTopPlane    = 0.0f;
    pClipFrustum->orthoRightPlane  = (float)width;
    pClipFrustum->orthoBottomPlane = (float)height;

    pSector->pClipFrustum = pClipFrustum;
    sithRender_PVSBuildVisibleSector(pSector);

    while ( sithRender_curPVSIndex <= sithRender_lastPVSIndex )
    {
        SithSurfaceAdjoin* pAdjoin = sithRender_aVisibleAdjoins[sithRender_curPVSIndex];
        sithRender_aVisibleAdjoins[sithRender_curPVSIndex] = NULL;

        while ( pAdjoin )
        {
            rdClipFrustum* pFrustum;
            SithSurface* pSurface;
            rdFace* pFace;
            rdVector3 lookDir;
            float dot;
            size_t numVertices;

            ++sithRender_numVisibleAdjoins;

            if ( (sithRender_aAdjoinTable[pAdjoin - sithWorld_pCurrentWorld->aAdjoins] & 0x80) != 0 )
            {
                pFrustum = rdCamera_pCurCamera->pFrustum;
                pFrustum->orthoLeftPlane   = 0.0f;
                pFrustum->orthoTopPlane    = 0.0f;
                pFrustum->orthoRightPlane  = (float)width;
                pFrustum->orthoBottomPlane = (float)height;
            }
            else
            {
                pFrustum = pAdjoin->pAdjoinSurface->pSector->pClipFrustum;
            }

            pSurface = pAdjoin->pAdjoinSurface;
            pFace    = &pSurface->face;
            rdVector_Sub3(&lookDir, &sithCamera_pCurCamera->lookPos, &sithWorld_pCurrentWorld->aVertices[*pFace->aVertices]);
            dot = rdVector_Dot3(&pFace->normal, &lookDir);
            if ( dot > 0.0f ) // If surface facing camera
            {
                sithRender_BuildVisibleSurface(pSurface);

                sithRender_clipFaceView.aVertices  = sithRender_aClipVertices;
                sithRender_faceView.numVertices    = pFace->numVertices;
                sithRender_faceView.aVertIdxs      = pFace->aVertices;

                rdClip_ClipFacePVS(pFrustum, &sithRender_faceView, &sithRender_clipFaceView);
                numVertices = sithRender_clipFaceView.numVertices;

                if ( (numVertices > 0 || (rdClip_faceStatus & 1) != 0)
                    && (!pFace->pMaterial || pFace->pMaterial->formatType != STDCOLOR_FORMAT_RGB
                        || (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0
                        || pFace->geometryMode == RD_GEOMETRY_NONE)
                    && (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
                {
                    SithSector* pAdjoinSector;

                    pAdjoinSector = pAdjoin->pAdjoinSector;

                    // Project vertices to camera space
                    rdCamera_pCurCamera->pfProjectList(sithRender_aTransformedClipVertices, sithRender_aClipVertices, numVertices);

                    if ( pAdjoinSector->renderTick == sithMain_curRenderTick ) // If sector was already build at current frame then  build only it's clip frustum
                    {
                        if ( (rdClip_faceStatus & 1) != 0 )
                        {
                            sithRender_BuildClipFrustrum(
                                pAdjoinSector->pClipFrustum,
                                numVertices,
                                pFrustum->orthoLeftPlane,
                                pFrustum->orthoTopPlane,
                                pFrustum->orthoRightPlane,
                                pFrustum->orthoBottomPlane
                            );
                        }
                        else
                        {
                            sithRender_BuildClipFrustrum(
                                pAdjoinSector->pClipFrustum,
                                numVertices,
                                pAdjoinSector->pClipFrustum->orthoLeftPlane,
                                pAdjoinSector->pClipFrustum->orthoTopPlane,
                                pAdjoinSector->pClipFrustum->orthoRightPlane,
                                pAdjoinSector->pClipFrustum->orthoBottomPlane
                            );
                        }
                    }
                    else
                    {
                        rdClipFrustum frustum;
                        rdClipFrustum* pNewFrustum;
                        if ( (rdClip_faceStatus & 1) != 0 )
                        {
                            pAdjoinSector->pClipFrustum = pFrustum;
                        }
                        else
                        {
                            pNewFrustum = &frustum;
                            sithRender_BuildClipFrustrum(pNewFrustum, numVertices, 3.4028235e38f, 3.4028235e38f, -3.4028235e38f, -3.4028235e38f);
                            pAdjoinSector->pClipFrustum = pNewFrustum;
                        }

                        sithRender_PVSBuildVisibleSector(pAdjoinSector);
                    }
                }
            }

            pAdjoin = pAdjoin->pNextVisibleAdjoin;
        }

        if ( !sithRender_aVisibleAdjoins[sithRender_curPVSIndex] )
        {
            ++sithRender_curPVSIndex;
        }
    }
}

void J3DAPI sithRender_PVSBuildVisibleSector(SithSector* pSector)
{
    size_t adjIdx;
    size_t i;
    SithSurfaceAdjoin* pAdjoin;

    // Update PVS stuff
    pAdjoin = pSector->pFirstAdjoin;
    while ( pAdjoin )
    {
        adjIdx = pAdjoin - sithWorld_pCurrentWorld->aAdjoins;
        if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
        {
            i = sithRender_aAdjoinTable[adjIdx];
            if ( i != 0 )
            {
                i &= ~0xFFFFFF80;
                if ( i < sithRender_curPVSIndex )
                {
                    i = sithRender_curPVSIndex;
                }

                pAdjoin->pNextVisibleAdjoin = sithRender_aVisibleAdjoins[i];
                sithRender_aVisibleAdjoins[i] = pAdjoin;
                sithRender_lastPVSIndex = J3DMAX(sithRender_lastPVSIndex, i);
            }
        }

        pAdjoin = pAdjoin->pNextAdjoin;
    }

    // Now build sector
    sithRender_BuildVisibleSector(pSector, pSector->pClipFrustum);
}

void J3DAPI sithRender_BuildVisibleSector(SithSector* pSector, const rdClipFrustum* pFrustrum)
{
    SithThing* pThing;
    SithActorInfo* pActorInfo;
    rdVector3 lightPos;

    pSector->renderTick = sithMain_curRenderTick;
    if ( sithRender_numVisibleSectors >= STD_ARRAYLEN(sithRender_aVisibleSectors) )
    {
#line 932
        stdPrintf(rdroid_pHS->pWarningPrint, J3D_FILE, __LINE__, "Max visible sectors exceeded!\n", 0, 0, 0, 0);
        return;
    }

    sithRender_aVisibleSectors[sithRender_numVisibleSectors++] = pSector;

    if ( (pSector->flags & SITH_SECTOR_SEEN) == 0 )
    {
        pSector->flags |= SITH_SECTOR_SEEN;
        if ( (pSector->flags & SITH_SECTOR_COGLINKED) != 0 ) { // Notify sector's cog
            sithCog_SectorSendMessage(pSector, NULL, SITHCOG_MSG_SIGHTED);
        }
    }

    // Assign sector+s frustum
    memcpy(&sithRender_aSectorFrustrums[sithRender_numSecorFrustrums], pFrustrum, sizeof(rdClipFrustum));
    pSector->pClipFrustum = &sithRender_aSectorFrustrums[sithRender_numSecorFrustrums++];

    // Collect emitted thing lights (ambient spot light & actor head light)
    pThing = pSector->pFirstThingInSector;
    while ( pThing && sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights) )
    {
        if ( (pThing->flags & SITH_TF_EMITLIGHT) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0 )
        {
            // Collect Thing's light if light range is > 0.01f
            if ( (pThing->light.color.red > 0.0f || pThing->light.color.green > 0.0f || pThing->light.color.blue > 0.0f)
                && SITHRENDER_ISDYNAMICLIGHT(pThing->light.color.alpha) )
            {
                rdVector_Copy4(&sithRender_aThingLights[sithRender_numThingLights].color, &pThing->light.color);

                sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->light.minRadius;
                sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->light.maxRadius;

                rdCamera_AddLight(rdCamera_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &pThing->pos);
                ++sithRender_numThingLights;
            }

            // Collect actor's head light
            if ( (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER)
                && sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights) )
            {
                pActorInfo = &pThing->thingInfo.actorInfo;
                if ( (pActorInfo->flags & SITH_AF_HEADLIGHT) != 0
                    && (pActorInfo->headLightIntensity.red > 0.0f
                        || pActorInfo->headLightIntensity.green > 0.0f
                        || pActorInfo->headLightIntensity.blue > 0.0f) )
                {
                    rdMatrix_TransformPoint34(&lightPos, &pActorInfo->lightOffset, &pThing->orient);
                    rdVector_Add3Acc(&lightPos, &pThing->pos);

                    rdVector_Copy4(&sithRender_aThingLights[sithRender_numThingLights].color, &pActorInfo->headLightIntensity);

                    sithRender_aThingLights[sithRender_numThingLights].minRadius = pActorInfo->headLightIntensity.alpha;
                    sithRender_aThingLights[sithRender_numThingLights].maxRadius = pActorInfo->headLightIntensity.alpha;

                    rdCamera_AddLight(rdCamera_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &lightPos);
                    ++sithRender_numThingLights;
                }
            }
        }

        pThing = pThing->pNextThingInSector;
    }

    sithRender_aThingSectors[sithRender_numThingSectors++] = pSector;
}

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithRender_countfit_6, 15)
CF_PROTO(sithRender_countfit_10, 3)
/* end count-fit */

void J3DAPI sithRender_RenderSectors(void)
{
    SithSector* pSector;
    SithSurface* pSurf;
    size_t numVertices;
    rdFaceFlags extraFaceFlags;
    size_t surfNum;
    size_t secNum;

    extraFaceFlags = 0;
    sithRender_numArchPolys = 0;
    if ( sithWorld_pCurrentWorld->fog.bEnabled ) {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }

    sithRender_faceView.aVertices    = sithWorld_pCurrentWorld->aTransformedVertices;
    sithRender_faceView.aTexVertices = sithWorld_pCurrentWorld->aTexVerticies;
    sithRender_faceView.aVertLights  = sithWorld_pCurrentWorld->aVertDynamicLights;

    for ( secNum = 0; secNum < sithRender_numVisibleSectors; ++secNum )
    {
        pSector = sithRender_aVisibleSectors[secNum];
        pSurf   = pSector->pFirstSurface;
        for ( surfNum = 0; surfNum < pSector->numSurfaces; ++surfNum )
        {
            rdVector3 camDir;

            rdFace* pFace = &pSurf->face;
            if ( pFace->geometryMode != RD_GEOMETRY_NONE )
            {
                rdVector_Sub3(&camDir, &sithCamera_pCurCamera->lookPos, &sithWorld_pCurrentWorld->aVertices[*pFace->aVertices]);
                if ( rdVector_Dot3(&pFace->normal, &camDir) > 0.0f ) // If surface is facing camera
                {
                    if ( pSurf->pAdjoin && (pFace->flags & RD_FF_TEX_TRANSLUCENT) != 0 )
                    {
                        if ( sithRender_numAlphaAdjoins < STD_ARRAYLEN(sithRender_aAlphaAdjoins) )
                        {
                            sithRender_aAlphaAdjoins[sithRender_numAlphaAdjoins++] = pSurf;
                        }
                    }
                    else
                    {
                        rdCacheProcEntry* pPoly;

                        // Rotate surface to camera orientation
                        if ( pSurf->renderTick != sithMain_curRenderTick )
                        {
                            size_t i;
                            for ( i = 0; i < pFace->numVertices; ++i )
                            {
                                size_t vertIdx = pFace->aVertices[i];
                                if ( sithWorld_pCurrentWorld->aVertexRenderTickIds[vertIdx] != sithMain_curRenderTick )
                                {
                                    rdMatrix_TransformPoint34(
                                        &sithWorld_pCurrentWorld->aTransformedVertices[vertIdx],
                                        &sithWorld_pCurrentWorld->aVertices[vertIdx],
                                        &rdCamera_pCurCamera->orient
                                    );
                                    sithWorld_pCurrentWorld->aVertexRenderTickIds[vertIdx] = sithMain_curRenderTick;
                                }
                            }

                            pSurf->renderTick = sithMain_curRenderTick;
                        }

                        pPoly = rdCache_GetProcEntry();
                        if ( pPoly )
                        {
                            // Render sky surface
                            if ( (pSurf->flags & (SITH_SURFACE_CEILINGSKY | SITH_SURFACE_HORIZONSKY)) != 0 )
                            {
                                // Clip vertices and transform to camera space
                                sithRender_clipFaceView.aVertices  = sithRender_aClipVertices;
                                sithRender_faceView.numVertices    = pFace->numVertices;
                                sithRender_faceView.aVertIdxs      = pFace->aVertices;

                                rdClip_QFace3W(pSector->pClipFrustum, &sithRender_faceView, &sithRender_clipFaceView);
                                numVertices = sithRender_clipFaceView.numVertices;
                                if ( numVertices >= 3 )
                                {
                                    // Project the clipped vertices to NDC space
                                    rdCamera_pCurCamera->pfProjectList(sithRender_aSurfaceTransformedVertices, sithRender_aClipVertices, numVertices);

                                    pPoly->flags     = pFace->flags;
                                    pPoly->pMaterial = pFace->pMaterial;

                                    // Now make sky poly from transformed vertices
                                    if ( (pSurf->flags & SITH_SURFACE_HORIZONSKY) != 0 )
                                    {
                                        sithRenderSky_HorizonFaceToPlane(pPoly, pFace, sithRender_aSurfaceTransformedVertices, numVertices);
                                    }
                                    else if ( (pSurf->flags & SITH_SURFACE_CEILINGSKY) != 0 )
                                    {
                                        sithRenderSky_CeilingFaceToPlane(pPoly, pFace, sithRender_aClipVertices, sithRender_aSurfaceTransformedVertices, numVertices);
                                    }

                                    pPoly->matCelNum = pFace->matCelNum;
                                    rdCache_AddProcFace(numVertices);

                                    ++sithRender_numArchPolys;
                                }
                            }
                            else // Not a sky surface
                            {
                                // Clip vertices and transform to NDC screen space
                                numVertices = pFace->numVertices;
                                if ( rdClip_FaceToPlane(pSector->pClipFrustum, pPoly, pFace, sithWorld_pCurrentWorld->aTransformedVertices, sithWorld_pCurrentWorld->aTexVerticies, sithWorld_pCurrentWorld->aVertDynamicLights, pSurf->aIntensities) )
                                {
                                    pPoly->lightingMode = pFace->lightingMode < sithRender_lightMode ? pFace->lightingMode : sithRender_lightMode;

                                    rdVector_Add3(&pPoly->extraLight, &pFace->extraLight, &pSector->extraLight);
                                    pPoly->extraLight.alpha = pFace->extraLight.alpha + pSector->extraLight.alpha;

                                    if ( (pSurf->flags & SITH_SURFACE_CEILINGSKY) != 0 )
                                    {
                                        sithRenderSky_CeilingFaceToPlane(pPoly, pFace, sithRender_aClipVertices, sithRender_aSurfaceTransformedVertices, numVertices);
                                        pPoly->flags = pFace->flags;
                                    }
                                    else
                                    {
                                        pPoly->flags = pFace->flags | extraFaceFlags;
                                    }

                                    pPoly->pMaterial = pFace->pMaterial;
                                    pPoly->matCelNum = pFace->matCelNum;
                                    rdCache_AddProcFace(numVertices);

                                    ++sithRender_numArchPolys;
                                }
                            }
                        }
                    }
                }
            }

            ++pSurf;
        }

        ++sithRender_numRenderedSectors;
    }

    rdCache_Flush();
}

void J3DAPI sithRender_BuildVisibleSectorsThingList(void)
{
    size_t i;
    SithSector* pSector;
    SithSurfaceAdjoin* pAdjoin;

    for ( i = 0; i < sithRender_numVisibleSectors; ++i )
    {
        pSector = sithRender_aVisibleSectors[i];
        pAdjoin = pSector->pFirstAdjoin;
        while ( pAdjoin )
        {
            if ( pAdjoin->pAdjoinSector->renderTick != sithMain_curRenderTick && (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 )
            {
                pAdjoin->pAdjoinSector->pClipFrustum = pSector->pClipFrustum;
                sithRender_BuildSectorThingList(pAdjoin->pAdjoinSector, 0.0f, pAdjoin->distance + pAdjoin->pMirrorAdjoin->distance);
            }

            pAdjoin = pAdjoin->pNextAdjoin;
        }
    }
}

void J3DAPI sithRender_BuildSectorThingList(SithSector* pSector, float curDistance, float extraDistance)
{
    rdVector3 lightPos;
    SithActorInfo* pActorInfo;
    float distance;
    SithThing* pThing;
    SithSurfaceAdjoin* pAdjoin;
    SithSurfaceAdjoin* pMirrorAdjoin;

    if ( pSector->renderTick != sithMain_curRenderTick )
    {
        pSector->renderTick = sithMain_curRenderTick;

        pThing = pSector->pFirstThingInSector;
        if ( pThing )
        {
            // The debug build tests a constant here (mov eax, 1; test; je past the loop, 0x455297): release-invisible,
            // name unknown, not restored.
            while ( pThing )
            {
                // Collect emitted thing lights (ambient spot light & actor head light)
                if ( sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights)
                    && (pThing->flags & SITH_TF_EMITLIGHT) != 0
                    && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DESTROYED)) == 0 )
                {
                    // Collect thing light if range is > 0.01f
                    if ( pThing->light.color.red > 0.0f
                        || pThing->light.color.green > 0.0f
                        || pThing->light.color.blue > 0.0f
                        && SITHRENDER_ISDYNAMICLIGHT(pThing->light.color.alpha) )
                    {
                        rdVector_Copy4(&sithRender_aThingLights[sithRender_numThingLights].color, &pThing->light.color);

                        sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->light.minRadius;
                        sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->light.maxRadius;

                        rdCamera_AddLight(rdCamera_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &pThing->pos);
                        ++sithRender_numThingLights;
                    }

                    // Collect actor head light
                    if ( (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER)
                        && sithRender_numThingLights < STD_ARRAYLEN(sithRender_aThingLights) )
                    {
                        pActorInfo = &pThing->thingInfo.actorInfo;
                        if ( (pActorInfo->flags & SITH_AF_HEADLIGHT) != 0
                            && (pActorInfo->headLightIntensity.x > 0.0f
                                || pActorInfo->headLightIntensity.y > 0.0f
                                || pActorInfo->headLightIntensity.z > 0.0f) )
                        {
                            rdMatrix_TransformPoint34(&lightPos, &pActorInfo->lightOffset, &pThing->orient);
                            rdVector_Add3Acc(&lightPos, &pThing->pos);

                            rdVector_Copy4(&sithRender_aThingLights[sithRender_numThingLights].color, &pActorInfo->headLightIntensity);

                            sithRender_aThingLights[sithRender_numThingLights].minRadius = pThing->light.minRadius;
                            sithRender_aThingLights[sithRender_numThingLights].maxRadius = pThing->light.maxRadius;

                            rdCamera_AddLight(rdCamera_pCurCamera, &sithRender_aThingLights[sithRender_numThingLights], &lightPos);
                            ++sithRender_numThingLights;
                        }
                    }
                }

                pThing = pThing->pNextThingInSector;
            }

            if ( curDistance < sithRender_maxThingSectorDistance )
            {
                ++sithRender_numVisibleThingSectors;
                if ( sithRender_numThingSectors < SITHRENDER_MAX_SECTORS_WITH_THINGS )
                {
                    sithRender_aThingSectors[sithRender_numThingSectors++] = pSector;
                }
            }
        }

        pAdjoin = pSector->pFirstAdjoin;
        while ( pAdjoin )
        {
            if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) != 0 && pAdjoin->pAdjoinSector->renderTick != sithMain_curRenderTick )
            {
                pMirrorAdjoin = pAdjoin->pMirrorAdjoin;
                distance = extraDistance + curDistance + pAdjoin->distance + pMirrorAdjoin->distance;
                if ( distance < sithRender_maxThingSectorDistance || distance < 8.0f )
                {
                    pAdjoin->pAdjoinSector->pClipFrustum = pSector->pClipFrustum;
                    sithRender_BuildSectorThingList(pAdjoin->pAdjoinSector, distance, 0.0f);
                }
            }

            pAdjoin = pAdjoin->pNextAdjoin;
        }
    }
}

void J3DAPI sithRender_BuildDynamicLights(void)
{
    // Function calculates vertex intensities from collected dynamic lights for all visible sectors

    size_t secNum;
    size_t lightNum;
    size_t i;
    size_t vertIdx;
    SithSector* pSector;
    rdLight* pLight;

    rdVector3* aVertices             = sithWorld_pCurrentWorld->aVertices;
    rdVector4* aVertDynamicLights    = sithWorld_pCurrentWorld->aVertDynamicLights;
    uint32_t* aVertLightsRenderTicks = sithWorld_pCurrentWorld->aVertDynamicLightsRenderTicks;
    float attenuationMax             = rdCamera_pCurCamera->attenuationMax;
    size_t numLights                 = 0;

    rdLight* aLights[RDCAMERA_MAX_LIGHTS];

    for ( secNum = 0; secNum < sithRender_numVisibleSectors; ++secNum )
    {
        pSector   = sithRender_aVisibleSectors[secNum];
        numLights = 0;

        for ( lightNum = 0; lightNum < rdCamera_pCurCamera->numLights; ++lightNum )
        {
            rdVector3 lightDir;
            pLight = rdCamera_pCurCamera->aLights[lightNum];
            rdVector_Sub3(&lightDir, &rdCamera_pCurCamera->aLightPositions[lightNum], &pSector->center);

            if ( sqrtf(rdVector_Dot3(&lightDir, &lightDir)) < pSector->radius + pLight->minRadius )
            {
                aLights[numLights++] = pLight;
            }
        }

        for ( i = 0; i < pSector->numVertices; ++i )
        {
            vertIdx = pSector->aVertIdxs[i];
            if ( aVertLightsRenderTicks[vertIdx] != sithMain_curRenderTick )
            {
                // Set default light color to black
                rdVector_Set4(&aVertDynamicLights[vertIdx], 0.0f, 0.0f, 0.0f, 0.0f);

                for ( lightNum = 0; lightNum < numLights; ++lightNum )
                {
                    pLight = aLights[lightNum];

                    {
                        float dist;
                        rdVector3 lightDir;
                        rdVector_Sub3(&lightDir, &rdCamera_pCurCamera->aLightPositions[pLight->num], &aVertices[vertIdx]);

                        dist = rdVector_Len3(&lightDir);
                        if ( dist < pLight->maxRadius )
                        {
                            float att = dist * attenuationMax;
                            aVertDynamicLights[vertIdx].red   += pLight->color.red - att;
                            aVertDynamicLights[vertIdx].green += pLight->color.green - att;
                            aVertDynamicLights[vertIdx].blue  += pLight->color.blue - att;
                        }
                    }

                    rdMath_ClampVector3Acc((rdVector3*)&aVertDynamicLights[vertIdx], 0.0f, 1.0f);
                    // TODO: alpha value is not set
                }

                aVertLightsRenderTicks[vertIdx] = sithMain_curRenderTick;
            }
        }
    }
}

void J3DAPI sithRender_RenderThings(void)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int bLightSet = 0;
    SithThing* pCurThing;
    size_t i;
    sithRender_numThingPolys     = 0;
    sithRender_numAlphaThingPoly = 0;

    rdModel3_EnableFogRendering(pWorld->fog.bEnabled);

    for ( i = 0; i < sithRender_numThingSectors; ++i )
    {
        SithSector* pSector = sithRender_aThingSectors[i];

        rdVector4 sectorAmbientLight;
        sectorAmbientLight.x     = STDMATH_CLAMP(pSector->ambientLight.x + pSector->extraLight.x, 0.0f, 1.0f);
        sectorAmbientLight.y     = STDMATH_CLAMP(pSector->ambientLight.y + pSector->extraLight.y, 0.0f, 1.0f);
        sectorAmbientLight.z     = STDMATH_CLAMP(pSector->ambientLight.z + pSector->extraLight.z, 0.0f, 1.0f);
        sectorAmbientLight.alpha = 0.0f;

        pCurThing = pSector->pFirstThingInSector;
        while ( pCurThing )
        {
            if ( (pCurThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0
                && ((sithCamera_pCurCamera->type & (SITHCAMERA_ORBITAL | SITHCAMERA_UNKNOWN_40 | SITHCAMERA_IDLE | SITHCAMERA_UNKNOWN_10 | SITHCAMERA_CINEMATIC | SITHCAMERA_EXTERNAL)) != 0
                    || pCurThing != sithCamera_pCurCamera->pPrimaryFocusThing) )
            {
                rdThing* prdThing;
                float radius;
                int bRender;

                // Transform thing pos to camera orient
                rdMatrix_TransformPoint34(&pCurThing->transformedPos, &pCurThing->pos, &rdCamera_pCurCamera->orient);

                prdThing = &pCurThing->renderData;

                switch ( prdThing->type )
                {
                    case RD_THING_MODEL3:
                    {
                        radius = J3DMAX(pCurThing->collide.size, pCurThing->collide.movesize);
                        radius = J3DMAX(radius, prdThing->data.pModel3->size);

                        // Set sector point light 
                        // 
                        // TODO [bug]: sithRender_numSectorPointLights is never incremented, but changing this breaks thin illumination as the light will affect also things from other sectors
                        //             Maybe just a temp var can be used for light can be used instead and we can then increase limitation of max thing light?
                        rdVector_Copy4(&sithRender_aSectorPointLights[sithRender_numSectorPointLights].color, &pSector->light.color);

                        sithRender_aSectorPointLights[sithRender_numSectorPointLights].minRadius = pSector->light.minRadius;
                        sithRender_aSectorPointLights[sithRender_numSectorPointLights].maxRadius = pSector->light.maxRadius;
                        rdCamera_AddLight(rdCamera_pCurCamera, &sithRender_aSectorPointLights[sithRender_numSectorPointLights], &pSector->light.pos);

                        bLightSet = 1;
                    }break;

                    case RD_THING_SPRITE3:
                        radius = prdThing->data.pSprite3->radius;
                        ++sithRender_numSpritesToDraw;
                        break;

                    case RD_THING_PARTICLE:
                        radius = prdThing->data.pParticle->radius;
                        break;

                    case RD_THING_POLYLINE:
                        radius = prdThing->data.pPolyline->length;
                        break;

                    default:
                        break;
                }

                bRender = 0;
                if ( prdThing->type != RD_THING_POLYLINE )
                {
                    // Render only if thing is inside frustum or intersects with frustum
                    prdThing->frustumCull = rdClip_SphereInFrustrum(pSector->pClipFrustum, &pCurThing->transformedPos, radius);
                    if ( prdThing->frustumCull != RDFRUSTUMCULL_OUTSIDE )
                    {
                        bRender = 1;
                    }
                }
                else
                {
                    bRender = 1;
                }

                if ( bRender == 1 )
                {
                    // Collect thing flat light
                    if ( (pCurThing->flags & SITH_TF_EMITLIGHT) != 0
                        && (pCurThing->light.color.red > 0.0f || pCurThing->light.color.green > 0.0f || pCurThing->light.color.blue > 0.0f)
                        && SITHRENDER_ISFLATLIGHT(pCurThing->light.color.alpha) )// if light range is <= 0.01f
                    {
                        rdVector4 ambientLight;
                        ambientLight.x = sectorAmbientLight.x + pCurThing->light.color.red;
                        ambientLight.x = STDMATH_CLAMP(ambientLight.x, 0.0f, 1.0f);
                        ambientLight.y = sectorAmbientLight.y + pCurThing->light.color.green;
                        ambientLight.y = STDMATH_CLAMP(ambientLight.y, 0.0f, 1.0f);
                        ambientLight.z = sectorAmbientLight.z + pCurThing->light.color.blue;
                        ambientLight.z = STDMATH_CLAMP(ambientLight.z, 0.0f, 1.0f);
                        ambientLight.alpha = sectorAmbientLight.alpha;

                        rdCamera_SetAmbientLight(rdCamera_pCurCamera, &ambientLight);
                    }
                    else
                    {
                        rdCamera_SetAmbientLight(rdCamera_pCurCamera, &sectorAmbientLight);
                    }

                    // Now draw thing
                    if ( sithRender_RenderThing(pCurThing) )
                    {
                        ++sithRender_numDrawnThings;
                    }
                }

                if ( bLightSet == 1 )
                {
                    --rdCamera_pCurCamera->numLights;
                    // Unsigned: the debug build keeps this test (0x4560C4), release drops it.
                    if ( rdCamera_pCurCamera->numLights < 0 )
                    {
                        rdCamera_pCurCamera->numLights = 0;
                    }

                    bLightSet = 0;
                }
            }

            pCurThing = pCurThing->pNextThingInSector;
        }
    }

    rdModel3_EnableFogRendering(0);
    rdCache_Flush();
}

int J3DAPI sithRender_RenderThing(SithThing* pThing)
{
    int resultCode;
    SithExplosionInfo* pBlastInfo;
    float dist;
    float brightness;

#line 1628
    RD_ASSERTREL(pThing != NULL); // rdroid_pHS, as every sithRender diagnostic (retail)

    if ( (pThing->flags & SITH_TF_SEEN) == 0 )
    {
        if ( (pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pThing, 0, SITHCOG_MSG_SIGHTED);
        }

        if ( pThing->controlType == SITH_CT_AI && pThing->controlInfo.aiControl.pLocal )
        {
            pThing->controlInfo.aiControl.pLocal->mode &= ~SITHAI_MODE_SLEEPING;
        }

        pThing->flags |= SITH_TF_SEEN;
    }

    // Set thing cur render frame num
    pThing->renderFrame = sithMain_frameNumber;

    // Now draw thing
    rdVector_Copy3(&pThing->orient.dvec, &pThing->pos);
    resultCode = sithThing_Draw(pThing);
    if ( (pThing->flags & SITH_TF_SHADOW) != 0 ) // draw shadow
    {
        sithShadow_RenderThingShadow(pThing);
    }

    sithRender_numThingPolys     += rdModel3_numDrawnFaces;
    sithRender_numAlphaThingPoly += rdModel3_numDrawnAlphaFaces;

    // Clear cur dvec aka position
    memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));

    // Extra draw func
    if ( sithRender_pExtraThingRenderFunc && (pThing->flags & SITH_TF_UNKNOWN_200000) != 0 )
    {
        sithRender_pExtraThingRenderFunc(pThing);
    }

    if ( pThing->type == SITH_THING_EXPLOSION )
    {
        pBlastInfo = &pThing->thingInfo.explosionInfo;
        if ( (pBlastInfo->flags & SITH_EF_BLINDPLAYER) != 0 )
        {
            // The debug build computes the camera distance and the flash brightness (JK blinds the player with them)
            // and uses neither; release keeps only the Dist3D1 call.
            dist       = stdMath_Dist3D1(pThing->transformedPos.x, pThing->transformedPos.y, pThing->transformedPos.z);
            brightness = (float)(unsigned int)(pBlastInfo->flashColor.red + pBlastInfo->flashColor.green + pBlastInfo->flashColor.blue) * 0.013020833f;
            pBlastInfo->flags &= ~SITH_EF_BLINDPLAYER;
        }
    }

    return resultCode;
}

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithRender_countfit_5, 5)
/* end count-fit */

void J3DAPI sithRender_RenderAlphaAdjoins(void)
{
    int bFlush = 0;
    int numDrawn = 0; // debug frame: a slot set to 0 here and never read again (name invented)
    rdFaceFlags extraFaceFlags = 0;
    size_t surfNum;
    size_t vertNum;
    size_t vertIdx;
    rdCacheProcEntry* pPoly;
    size_t numVertices;
    SithSurface* pSurf;
    SithSector* pSector;
    rdFace* pFace;
    rdVector4 ambientLight;

    sithRender_numAlphaArchPolys = 0;
    if ( sithWorld_pCurrentWorld->fog.bEnabled )
    {
        extraFaceFlags = RD_FF_FOG_ENABLED;
    }

    for ( surfNum = 0; surfNum < sithRender_numAlphaAdjoins; ++surfNum )
    {
        pSurf   = sithRender_aAlphaAdjoins[surfNum];
        pFace   = &pSurf->face;
        pSector = pSurf->pSector;

        rdVector_Copy4(&ambientLight, &pSector->ambientLight); // TODO: Why the ambient light is applied? For normal surfs that is not the case
        rdVector_Add3Acc((rdVector3*)&ambientLight, (const rdVector3*)&pSector->extraLight);
        ambientLight.alpha = 0.0f;

        rdCamera_SetAmbientLight(rdCamera_pCurCamera, &ambientLight);

        if ( pSurf->renderTick != sithMain_curRenderTick ) // transform surface verts to camera space if not done for current frame yet
        {
            for ( vertNum = 0; vertNum < pFace->numVertices; ++vertNum )
            {
                vertIdx = pFace->aVertices[vertNum];
                if ( sithWorld_pCurrentWorld->aVertexRenderTickIds[vertIdx] != sithMain_curRenderTick )
                {
                    rdMatrix_TransformPoint34(
                        &sithWorld_pCurrentWorld->aTransformedVertices[vertIdx],
                        &sithWorld_pCurrentWorld->aVertices[vertIdx],
                        &rdCamera_pCurCamera->orient
                    );
                    sithWorld_pCurrentWorld->aVertexRenderTickIds[vertIdx] = sithMain_curRenderTick;
                }
            }

            pSurf->renderTick = sithMain_curRenderTick;
        }

        pPoly = rdCache_GetAlphaProcEntry();
        if ( pPoly )
        {
            pPoly->lightingMode = pFace->lightingMode < sithRender_lightMode ? pFace->lightingMode : sithRender_lightMode;

            numVertices = pFace->numVertices;
            if ( rdClip_FaceToPlane(pSector->pClipFrustum, pPoly, pFace, sithWorld_pCurrentWorld->aTransformedVertices, sithWorld_pCurrentWorld->aTexVerticies, sithWorld_pCurrentWorld->aVertDynamicLights, pSurf->aIntensities) )
            {
                rdVector_Add3(&pPoly->extraLight, &pFace->extraLight, &pSector->extraLight);
                pPoly->extraLight.alpha = pFace->extraLight.alpha + pSector->extraLight.alpha;

                pPoly->matCelNum = pFace->matCelNum;
                pPoly->flags     = pFace->flags | extraFaceFlags;
                pPoly->pMaterial = pFace->pMaterial;

                ++sithRender_numAlphaArchPolys;
                rdCache_AddAlphaProcFace(numVertices);

                bFlush = 1;
            }
        }
    }

    if ( bFlush )
    {
        rdCache_FlushAlpha();
    }
}

/* PLACEHOLDER, linker-discarded: retail calls sithRender_pExtraThingRenderFunc (RenderThing) but only ever stores NULL
   into it (Startup, Close), so its setter existed and was discarded. Jedi Knight's retail has
   sithRender_SetExtraThingRenderFunc (0x004C8600) right after RenderAlphaAdjoins, in the same place.
   Signature is inferred; body cut to the store. No header prototype:
   nothing calls it and sithRender.h has none. */
void J3DAPI sithRender_SetExtraThingRenderFunc(int (J3DAPI* pfFunc)(SithThing* pThing))
{
    sithRender_pExtraThingRenderFunc = pfFunc;
}

int J3DAPI sithRender_MakeScreenShot(void)
{
    char aCurFilename[64];
    char aFilePath[128];

    size_t ssnum = 0;
    stdUtil_Format(aCurFilename, STD_ARRAYLEN(aCurFilename), sithGetScreenShotFileTemplate(), ssnum);
    stdFnames_MakePath(aFilePath, sizeof(aFilePath), sithGetScreenShotsDir(), aCurFilename);

    while ( stdFileUtil_FileExists(aFilePath) && ssnum < SITHRENDER_MAXSCREENSHOTS )
    {
        ++ssnum;
        stdUtil_Format(aCurFilename, STD_ARRAYLEN(aCurFilename), sithGetScreenShotFileTemplate(), ssnum);
        stdFnames_MakePath(aFilePath, sizeof(aFilePath), sithGetScreenShotsDir(), aCurFilename);
    }

    if ( ssnum > SITHRENDER_MAXSCREENSHOTS )
    {
        ssnum = 0;
        stdUtil_Format(aCurFilename, STD_ARRAYLEN(aCurFilename), sithGetScreenShotFileTemplate(), ssnum);
        stdFnames_MakePath(aFilePath, sizeof(aFilePath), sithGetScreenShotsDir(), aCurFilename);
    }

    return stdDisplay_SaveScreen(aFilePath);
}
