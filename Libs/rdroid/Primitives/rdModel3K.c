/*
 * rdModel3K: the retail "Katmai" (Pentium III SSE) mesh renderer, retail 0x004E1DA0-0x004E2D80.
 *
 * NOT MSVC output: this TU is compiled with Intel C/C++ 4.0 (build 99100, `-O2 -QxM`);
 * retail's bytes carry ICL's aligned-stack
 * dual-entry prologue and `lea` fill. Startup, DrawMesh, Flush, TransformVerts and
 * Project are byte-identical to retail after relocation (DrawMesh since the camera matrix and local camera
 * are declared as float arrays). The CPU probes and the
 * availability/enable functions (0x004E2D80-0x004E2F30) are a second Intel object, rdModel3K_cpu.c.
 *
 * Source file name per the retail assertion text: C:\Jones3D\Libs\Katmai\source\rdModel3K.c (asserts at
 * lines 253/254 of DrawMesh).
 */
#include <math.h>
#include <xmmintrin.h>

#include "rdModel3K.h"

#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Engine/rdLight.h>

#include <std/types.h>
#include <std/General/stdEffect.h>
#include <std/General/stdMath.h>

typedef unsigned int u32;

// Table headers (invented names, layout from Startup/DrawMesh): seven float pools of 0x800 bytes follow
// header A, three follow header B. Each pool pointer is 16-byte aligned.
typedef struct
{
    float* v[7];
    int flags;
    rdMaterial* pMaterial;
    int unk24;
    int unk28;
    size_t numVerts;
    size_t numFaces;
    size_t numIndices;
} rdModel3KTableA;

typedef struct
{
    float* v[3];
} rdModel3KTableB;

// 32-byte D3DTLVERTEX-shaped record (Flush's scatter targets: 0x00 sx, 0x04 sy, 0x08 sz, 0x0C rhw, 0x10 color, 0x14 specular).
typedef struct
{
    float sx, sy, sz, rhw;
    unsigned int color, specular;
    float u, v;
} rdModel3KVert;

// Retail 0x004E2700 (Flush), 0x004E2B60 (TransformVerts) and 0x004E2BE0 (Project) are defined below DrawMesh.
void J3DAPI rdModel3K_Flush(const rdModel3Mesh* pMesh, rdModel3KTableA* pTable);
void J3DAPI rdModel3K_TransformVerts(const rdModel3Mesh* m, rdModel3KTableA* pTable);
void J3DAPI rdModel3K_Project(rdMatrix34* pMat, float* px, float* py, float* pz, rdModel3KTableB* pTable, int num);

// Defined by rdModel3.c (not declared in its header).
extern const rdModel3Mesh* rdModel3_pCurMesh;
extern rdLightMode rdModel3_curLightingMode;
extern rdLightMode rdModel3_lightingMode;
extern size_t rdModel3_numGeoLights;
extern size_t rdModel3_numMeshLights;
extern rdLight* rdModel3_apGeoLights[RDCAMERA_MAX_LIGHTS];
extern rdLight* rdModel3_apMeshLights[RDCAMERA_MAX_LIGHTS];
extern rdVector3 rdModel3_aLocalLightPos[RDCAMERA_MAX_LIGHTS];
extern float rdModel3_localCamera[3];

// rdCamera as this TU saw it (not rdCamera.h): the camera matrix as a float array, rows rvec/lvec/uvec/dvec. Retail's
// DrawMesh schedule proves both globals were float arrays here: ICL 4.0 orders a store to element 0 of a local float array
// after later loads of elements 1-2 of a global float array (both references indirect, no constant offset on element 0)
// and lets elements 1-2 pass each other (constant offsets into different objects), which is retail's camPos copy and dir
// block; a struct-typed global is a direct reference and never orders against the local.
//
extern rdCamera* rdCamera_pCurCamera;
extern float rdCamera_camMatrix[4][3];
extern int rdModel3_numDrawnFaces;
extern rdFaceFlags rdModel3_extraFaceFlags;

// std3D as this TU saw it (not std3D.h: retail's Flush pushes four arguments to 0x004BEB20, whose body and other caller
// take three).
int J3DAPI std3D_SetProjection(float fov, float aspectRatio, float nearPlane, float farPlane);
void J3DAPI std3D_DrawRenderList(LPDIRECT3DTEXTURE2 pTex, Std3DRenderState rdflags, LPD3DTLVERTEX aVerts, size_t numVerts, LPWORD aIndices, size_t numIndices);
void J3DAPI std3D_AddToTextureCache(tSystemTexture* pCacheTexture, StdColorFormatType format);
void J3DAPI std3D_UpdateFrameCount(tSystemTexture* pTexture);

static __m128 rdModel3K_vZero, rdModel3K_vOne, rdModel3K_vNegOne, rdModel3K_v255, rdModel3K_vScale; // retail 0x014E7530..
static __m64 rdModel3K_mmMask;                                                                      // retail 0x014E7580
static char rdModel3K_bufA[sizeof(rdModel3KTableA) + 7 * 0x800];                             // retail 0x014E7588
// Retail 0x014EADC0/0x014EADC4. Startup reads each pointer once as an integer (the aligned address)
// and once as a pointer (the store): reading through two types keeps ICL's PRE from merging the two
// loads, as retail's two separate loads show. A union
// spelling gives identical bytes; the integer read is the cast form.
static rdModel3KTableA* rdModel3K_gA;
static rdModel3KTableB* rdModel3K_gB;
static char rdModel3K_bufB[sizeof(rdModel3KTableB) + 3 * 0x800]; // retail 0x014EADC8

static __m128 rdModel3K_vCenterX, rdModel3K_vCenterY, rdModel3K_vFocal, rdModel3K_vAmbR, rdModel3K_vAmbG, rdModel3K_vAmbB;
static rdModel3KVert rdModel3K_aVerts[81920];
static unsigned short rdModel3K_aIndices[245760];
static __m128 rdModel3K_vInvNear, rdModel3K_vInvFar, rdModel3K_vFade; // retail 0x017E4640/50/60

#define RDMODEL3K_ALIGN16(a) ((float*)(((a) + 15) & ~15u))

void J3DAPI rdModel3K_Startup(void)
{
    rdModel3K_gA = (rdModel3KTableA*)rdModel3K_bufA;
    rdModel3K_gB = (rdModel3KTableB*)rdModel3K_bufB;
    rdModel3K_gA->v[0] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x0000);
    rdModel3K_gA->v[1] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x0800);
    rdModel3K_gA->v[2] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x1000);
    rdModel3K_gA->v[3] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x1800);
    rdModel3K_gA->v[4] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x2000);
    rdModel3K_gA->v[5] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x2800);
    rdModel3K_gA->v[6] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gA) + sizeof(rdModel3KTableA) + 0x3000);
    rdModel3K_gB->v[0] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gB) + sizeof(rdModel3KTableB) + 0x0000);
    rdModel3K_gB->v[1] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gB) + sizeof(rdModel3KTableB) + 0x0800);
    rdModel3K_gB->v[2] = RDMODEL3K_ALIGN16((*(u32*)&rdModel3K_gB) + sizeof(rdModel3KTableB) + 0x1000);
    rdModel3K_vZero = _mm_set_ps1(0.0f);
    rdModel3K_vOne = _mm_set_ps1(1.0f);
    rdModel3K_vNegOne = _mm_set_ps1(-1.0f);
    rdModel3K_v255 = _mm_set_ps1(255.0f);
    rdModel3K_vScale = _mm_set_ps1(0.03125f);
    _mm_empty();
    {
        __m64 m = _mm_cvtsi32_si64(0xff000000);
        rdModel3K_mmMask = _mm_unpacklo_pi32(m, m);
    }
    _mm_empty();
}

// Retail 0x004E1F00 (0x800). Byte-identical after relocation.
// dist, camPos and dir are float arrays: ICL 4.0 keeps local arrays in memory and treats a store to one as possibly
// aliasing heap and array data, which gives retail's stores/reloads (light->num and pFace->aVertices reread
// after every component store, rdCamera_pCurCamera/rdModel3_pCurMesh loaded twice). rdVector3 locals are
// scalarised by ICL in every spelling tried. pLight: retail keeps the light pointer across the
// rdModel3_apMeshLights store, which ICL only does for a local. n: retail computes the fan bound once.
void J3DAPI rdModel3K_DrawMesh(const rdModel3Mesh* pMesh, const rdMatrix34* orient)
{
    rdVector3 tpos;
    rdMatrix34 tmat;
    rdMatrix34 orthOrient;
    float dist[3];
    float camPos[3];
    float dir[3];
    float dot;
    const rdFace* pFace;
    size_t i;
    size_t faceNum;
    size_t j;
    size_t base;
    rdLight* pLight;

    rdModel3K_gA->numVerts = 0;
    rdModel3K_gA->numFaces = 0;
    rdModel3K_gA->numIndices = 0;
#line 253
    RD_ASSERTREL(rdCamera_pCurCamera!=NULL);
    RD_ASSERTREL(pMesh!=NULL);
    rdModel3_pCurMesh = pMesh;
    if (rdModel3_pCurMesh->apVertices) {
        rdModel3K_vCenterX = _mm_set_ps1(rdCamera_pCurCamera->pCanvas->center.x);
        rdModel3K_vCenterY = _mm_set_ps1(rdCamera_pCurCamera->pCanvas->center.y);
        rdModel3K_vFocal = _mm_set_ps1(rdCamera_pCurCamera->focalLength);
        rdModel3K_vAmbR = _mm_set_ps1(rdCamera_pCurCamera->ambientLight.x);
        rdModel3K_vAmbG = _mm_set_ps1(rdCamera_pCurCamera->ambientLight.y);
        rdModel3K_vAmbB = _mm_set_ps1(rdCamera_pCurCamera->ambientLight.z);
        if (rdModel3_pCurMesh->geoMode) {
            rdMatrix_TransformPoint34(&tpos, &orient->dvec, &rdCamera_pCurCamera->orient);
            rdMatrix_Multiply34(&tmat, &rdCamera_pCurCamera->orient, orient);
            rdModel3K_TransformVerts(rdModel3_pCurMesh, rdModel3K_gA);
            rdModel3K_Project(&tmat, rdModel3K_gA->v[0], rdModel3K_gA->v[1], rdModel3K_gA->v[2], rdModel3K_gB, rdModel3_pCurMesh->numVertices);
            rdMatrix_InvertOrtho34(&orthOrient, orient);
            rdModel3_lightingMode = rdModel3_pCurMesh->lightMode < rdModel3_curLightingMode ? rdModel3_pCurMesh->lightMode : rdModel3_curLightingMode;
            rdModel3_numMeshLights = 0;
            for (i = 0; i < rdModel3_numGeoLights; i++) {
                pLight = rdModel3_apGeoLights[i];
                dist[0] = rdCamera_pCurCamera->aLightPositions[pLight->num].x - orient->dvec.x;
                dist[1] = rdCamera_pCurCamera->aLightPositions[pLight->num].y - orient->dvec.y;
                dist[2] = rdCamera_pCurCamera->aLightPositions[pLight->num].z - orient->dvec.z;

                if (sqrtf(dist[0] * dist[0] + dist[1] * dist[1] + dist[2] * dist[2]) < rdModel3_pCurMesh->radius + pLight->minRadius) {
                    rdModel3_apMeshLights[rdModel3_numMeshLights] = pLight;
                    rdMatrix_TransformPoint34(&rdModel3_aLocalLightPos[rdModel3_numMeshLights], &rdCamera_pCurCamera->aLightPositions[pLight->num], &orthOrient);
                    rdModel3_numMeshLights++;
                }
            }
            rdLight_CalcVertexIntensities(rdCamera_pCurCamera->aLights, rdModel3_aLocalLightPos, rdCamera_pCurCamera->numLights,
                rdModel3_pCurMesh->aVertNormals, rdModel3_pCurMesh->apVertices, rdModel3_pCurMesh->aVertColors,
                rdModel3_pCurMesh->aLightIntensities, rdModel3_pCurMesh->numVertices, rdCamera_pCurCamera->attenuationMin);
            camPos[0] = rdCamera_camMatrix[3][0];
            camPos[1] = rdCamera_camMatrix[3][1];
            camPos[2] = rdCamera_camMatrix[3][2];
            rdMatrix_TransformPoint34((rdVector3*)rdModel3_localCamera, (rdVector3*)camPos, &orthOrient);
            pFace = pMesh->aFaces;
            for (faceNum = 0; faceNum < pMesh->numFaces; faceNum++) {
                dir[0] = rdModel3_localCamera[0] - rdModel3_pCurMesh->apVertices[*pFace->aVertices].x;
                dir[1] = rdModel3_localCamera[1] - rdModel3_pCurMesh->apVertices[*pFace->aVertices].y;
                dir[2] = rdModel3_localCamera[2] - rdModel3_pCurMesh->apVertices[*pFace->aVertices].z;
                dot = dir[0] * pFace->normal.x + dir[1] * pFace->normal.y + dir[2] * pFace->normal.z;

                if (!((dot > 0.0f) ? 0 : 1) || (pFace->flags & 1) || !(rdroid_curRenderOptions & 1)) {
                    base = rdModel3K_gA->numVerts;
                    for (j = 0; j < pFace->numVertices; j++) {
                        ((int*)rdModel3K_gA->v[0])[base + j] = ((int*)rdModel3K_gB->v[0])[pFace->aVertices[j]];
                        ((int*)rdModel3K_gA->v[1])[base + j] = ((int*)rdModel3K_gB->v[1])[pFace->aVertices[j]];
                        ((int*)rdModel3K_gA->v[2])[base + j] = ((int*)rdModel3K_gB->v[2])[pFace->aVertices[j]];
                        rdModel3K_gA->v[3][base + j] = rdModel3_pCurMesh->aLightIntensities[pFace->aVertices[j]].x;
                        rdModel3K_gA->v[4][base + j] = rdModel3_pCurMesh->aLightIntensities[pFace->aVertices[j]].y;
                        rdModel3K_gA->v[5][base + j] = rdModel3_pCurMesh->aLightIntensities[pFace->aVertices[j]].z;
                        rdModel3K_aVerts[base + j].u = rdModel3_pCurMesh->apTexVertices[pFace->aTexVertices[j]].x;
                        rdModel3K_aVerts[base + j].v = rdModel3_pCurMesh->apTexVertices[pFace->aTexVertices[j]].y;
                        rdModel3K_gA->numVerts++;
                    }
                    if (pFace->numVertices == 3) {
                        rdModel3K_aIndices[rdModel3K_gA->numIndices++] = base;
                        rdModel3K_aIndices[rdModel3K_gA->numIndices++] = base + 1;
                        rdModel3K_aIndices[rdModel3K_gA->numIndices++] = base + 2;
                        rdModel3_numDrawnFaces++;
                    } else {
                        int flip, n, a, lo, hi, k;
                        flip = 0;
                        n = pFace->numVertices - 2;
                        a = base;
                        lo = base + 1;
                        hi = base + pFace->numVertices - 1;
                        for (k = 0; k < n; k++) {
                            rdModel3K_aIndices[rdModel3K_gA->numIndices++] = a;
                            rdModel3K_aIndices[rdModel3K_gA->numIndices++] = lo;
                            rdModel3K_aIndices[rdModel3K_gA->numIndices++] = hi;
                            rdModel3_numDrawnFaces++;
                            if ((k & 1) ? 0 : 1) {
                                a = lo;
                                lo++;
                            } else {
                                a = hi;
                                hi--;
                            }
                        }
                    }
                    rdModel3K_gA->numFaces++;
                }
                if (faceNum == pMesh->numFaces || pFace->pMaterial != pFace[1].pMaterial || pFace->flags != pFace[1].flags) {
                    if (rdModel3K_gA->numFaces > 0) {
                        rdModel3K_gA->pMaterial = pFace->pMaterial;
                        rdModel3K_gA->flags = pFace->flags;
                        rdModel3K_Flush(rdModel3_pCurMesh, rdModel3K_gA);
                    }
                    base = 0;
                    rdModel3K_gA->numFaces = 0;
                    rdModel3K_gA->numIndices = 0;
                    rdModel3K_gA->numVerts = 0;
                }
                pFace++;
            }
        }
    }
}

// Retail 0x004E2700 (0x460): projection, splat constants, render flags, texture lookup, then the batch loop (4 vertices per
// iteration: screen position, depth, rhw and packed colour into the 32-byte records) and the draw call. Byte-identical after
// relocation: the loop index is a function-scope static (retail 0x017E4670, the last object of
// this object's .bss; the 12 bytes after it are ICL's 16-byte section padding). ICL's alias analysis proves that a static
// local whose address is never taken cannot be reached through pTable or pFade, which is what lets the scheduler load
// numVerts above the index store and place the index store after the fade splat, as retail does; a file-scope static is
// not proved and does not reproduce retail's schedule.
// Shapes that reproduced retail's code:
// - std3D_SetProjection gets four arguments (fov, aspectRatio, near, far): retail stores four values, its body and the other caller
//   take three, so this TU saw an older prototype (declared locally above; std3D.h is not included);
// - `switch` on rdroid_curGeometryMode puts the material block out of line after the ret, as retail does (if/else forms lay it out inline);
// - each material clamp is written inside its own branch (retail duplicates the clamp tails);
// - the texture is cached through the inline helper below; the text otherwise follows the debug build's code (0x536580),
//   including its never-read `width`/`height` and the unused locals that hold the rest of its 0x1F0-byte frame.
// The debug build has this helper out of line at 0x536540, between DrawMesh and Flush (the same body as
// rdCache_AddToTextureCache); retail has expanded it into Flush and kept no copy. Name inferred.
static __inline void J3DAPI rdModel3K_AddToTextureCache(tSystemTexture* pTex, StdColorFormatType format)
{
    if (!pTex->pD3DCachedTex)
        std3D_AddToTextureCache(pTex, format);
    else
        std3D_UpdateFrameCount(pTex);
}

void J3DAPI rdModel3K_Flush(const rdModel3Mesh* pMesh, rdModel3KTableA* pTable)
{
    float aBuf[20];
    float* pBuf;
    const tStdFadeFactor* pFade;
    Std3DRenderState rdflags;
    int flags;
    rdMaterial* pMat;
    LPDIRECT3DTEXTURE2 pD3DTex;
    StdColorFormatType format;
    int celNum;
    tSystemTexture* pTex;
    float width, height;
    int unused1, unused2, unused3, unused4, unused5, unused6;
    __m128 y, recip, scale, sx, sy, rhw, r, g, b;
    __m128 unusedV1, unusedV2, unusedV3, unusedV4, unusedV5;
    __m64 ir, ig, ib;
    __m64 unusedM;
    static size_t i; // retail 0x017E4670

    pBuf = (float*)(((u32)aBuf + 15) & ~15u);
    std3D_SetProjection(rdCamera_pCurCamera->fov, rdCamera_pCurCamera->aspectRatio, rdCamera_pCurCamera->pFrustum->nearPlane, rdCamera_pCurCamera->pFrustum->farPlane);
    pFade = stdEffect_GetFadeFactor();
    rdModel3K_vInvNear = _mm_set_ps1(rdCamera_pCurCamera->invNearClipPlane);
    rdModel3K_vInvFar = _mm_set_ps1(rdCamera_pCurCamera->invFarClipPlane);

    rdflags = STD3D_RS_SUBPIXEL_CORRECTION | STD3D_RS_UNKNOWN_2 | STD3D_RS_UNKNOWN_1;
    flags = rdModel3K_gA->flags;
    if (flags & RD_FF_TEX_CLAMP_X)
        rdflags |= STD3D_RS_TEX_CPAMP_U;
    if (flags & RD_FF_TEX_CLAMP_Y)
        rdflags |= STD3D_RS_TEX_CPAMP_V;
    if (!(flags & RD_FF_TEX_FILTER_NEAREST))
        rdflags |= STD3D_RS_TEXFILTER_BILINEAR;
    if (flags & RD_FF_ZWRITE_DISABLED)
        rdflags |= STD3D_RS_ZWRITE_DISABLED;
    if (rdModel3_extraFaceFlags & RD_FF_FOG_ENABLED)
        rdflags |= STD3D_RS_FOG_ENABLED;

    switch (rdroid_curGeometryMode) {
    case RD_GEOMETRY_SOLID:
        pMat = NULL;
        break;
    default:
        pMat = rdModel3K_gA->pMaterial;
        break;
    }
    if (pMat) {
        format = pMat->formatType;
        celNum = pMesh->aFaces->matCelNum;
        if (celNum == -1)
            pTex = &rdModel3K_gA->pMaterial->aTextures[STDMATH_CLAMP((int)rdModel3K_gA->pMaterial->curCelNum, 0, (int)rdModel3K_gA->pMaterial->numCels - 1)];
        else
            pTex = &rdModel3K_gA->pMaterial->aTextures[STDMATH_CLAMP(celNum, 0, (int)rdModel3K_gA->pMaterial->numCels - 1)];
        rdModel3K_AddToTextureCache(pTex, format);
        pD3DTex = pTex->pD3DCachedTex;
        width = (float)pMat->width;
        height = (float)pMat->height;
    } else {
        pD3DTex = NULL;
    }
    rdModel3K_vFade = _mm_set_ps1(pFade->factor);
    for (i = 0; i < pTable->numVerts; i += 4) {
        y = _mm_load_ps(pTable->v[1] + i);
        recip = _mm_rcp_ps(y);
        scale = _mm_mul_ps(rdModel3K_vFocal, recip);
        sx = _mm_add_ps(_mm_mul_ps(_mm_load_ps(pTable->v[0] + i), scale), rdModel3K_vCenterX);
        sy = _mm_mul_ps(rdModel3K_vNegOne, _mm_mul_ps(_mm_load_ps(pTable->v[2] + i), scale));
        sy = _mm_add_ps(sy, rdModel3K_vCenterY);
        _mm_store_ps(pBuf, sx);
        rdModel3K_aVerts[i].sx = pBuf[0];
        rdModel3K_aVerts[i + 1].sx = pBuf[1];
        rdModel3K_aVerts[i + 2].sx = pBuf[2];
        rdModel3K_aVerts[i + 3].sx = pBuf[3];
        _mm_store_ps(pBuf + 4, sy);
        rdModel3K_aVerts[i].sy = pBuf[4];
        rdModel3K_aVerts[i + 1].sy = pBuf[5];
        rdModel3K_aVerts[i + 2].sy = pBuf[6];
        rdModel3K_aVerts[i + 3].sy = pBuf[7];
        y = _mm_mul_ps(rdModel3K_vInvFar, _mm_sub_ps(recip, rdModel3K_vInvNear));
        _mm_store_ps(pBuf + 8, y);
        rdModel3K_aVerts[i].sz = pBuf[8];
        rdModel3K_aVerts[i + 1].sz = pBuf[9];
        rdModel3K_aVerts[i + 2].sz = pBuf[10];
        rdModel3K_aVerts[i + 3].sz = pBuf[11];
        rhw = _mm_mul_ps(recip, rdModel3K_vScale);
        _mm_store_ps(pBuf + 12, rhw);
        rdModel3K_aVerts[i].rhw = pBuf[12];
        rdModel3K_aVerts[i + 1].rhw = pBuf[13];
        rdModel3K_aVerts[i + 2].rhw = pBuf[14];
        rdModel3K_aVerts[i + 3].rhw = pBuf[15];

        r = _mm_load_ps(pTable->v[3] + i);
        g = _mm_load_ps(pTable->v[4] + i);
        b = _mm_load_ps(pTable->v[5] + i);
        r = _mm_max_ps(r, rdModel3K_vZero);
        r = _mm_min_ps(r, rdModel3K_vOne);
        g = _mm_max_ps(g, rdModel3K_vZero);
        g = _mm_min_ps(g, rdModel3K_vOne);
        b = _mm_max_ps(b, rdModel3K_vZero);
        b = _mm_min_ps(b, rdModel3K_vOne);
        r = _mm_add_ps(r, rdModel3K_vAmbR);
        g = _mm_add_ps(g, rdModel3K_vAmbG);
        b = _mm_add_ps(b, rdModel3K_vAmbB);
        if (pFade->bEnabled) {
            r = _mm_mul_ps(r, rdModel3K_vFade);
            g = _mm_mul_ps(g, rdModel3K_vFade);
            b = _mm_mul_ps(b, rdModel3K_vFade);
        }
        r = _mm_max_ps(r, rdModel3K_vZero);
        r = _mm_min_ps(r, rdModel3K_vOne);
        g = _mm_max_ps(g, rdModel3K_vZero);
        g = _mm_min_ps(g, rdModel3K_vOne);
        b = _mm_max_ps(b, rdModel3K_vZero);
        b = _mm_min_ps(b, rdModel3K_vOne);
        _mm_empty();
        r = _mm_mul_ps(r, rdModel3K_v255);
        g = _mm_mul_ps(g, rdModel3K_v255);
        b = _mm_mul_ps(b, rdModel3K_v255);

        ir = _mm_cvtps_pi32(r);
        ig = _mm_cvtps_pi32(g);
        ib = _mm_cvtps_pi32(b);
        ir = _mm_slli_pi32(ir, 16);
        ig = _mm_slli_pi32(ig, 8);
        ib = _mm_or_si64(rdModel3K_mmMask, _mm_or_si64(ir, _mm_or_si64(ig, ib)));
        rdModel3K_aVerts[i].color = _mm_cvtsi64_si32(ib);
        ib = _mm_srli_si64(ib, 32);
        rdModel3K_aVerts[i + 1].color = _mm_cvtsi64_si32(ib);

        r = _mm_shuffle_ps(r, r, 0x0E);
        g = _mm_shuffle_ps(g, g, 0x0E);
        b = _mm_shuffle_ps(b, b, 0x0E);
        ir = _mm_cvtps_pi32(r);
        ig = _mm_cvtps_pi32(g);
        ib = _mm_cvtps_pi32(b);
        ir = _mm_slli_pi32(ir, 16);
        ig = _mm_slli_pi32(ig, 8);
        ib = _mm_or_si64(rdModel3K_mmMask, _mm_or_si64(ir, _mm_or_si64(ig, ib)));
        rdModel3K_aVerts[i + 2].color = _mm_cvtsi64_si32(ib);
        ib = _mm_srli_si64(ib, 32);
        rdModel3K_aVerts[i + 3].color = _mm_cvtsi64_si32(ib);
        _mm_empty();
    }
    std3D_DrawRenderList(pD3DTex, rdflags, (LPD3DTLVERTEX)rdModel3K_aVerts, pTable->numVerts, rdModel3K_aIndices, pTable->numIndices);
}

// Retail 0x004E2B60. The assertion's __LINE__ is 746 (retail push 0x2EA); #line pins it, as for DrawMesh.
#line 743
void J3DAPI rdModel3K_TransformVerts(const rdModel3Mesh* m, rdModel3KTableA* pTable)
{
    size_t i;
    RD_ASSERTREL(m->numVertices<500);
    for (i = 0; i < m->numVertices; i++) {
        pTable->v[0][i] = m->apVertices[i].x;
        pTable->v[1][i] = m->apVertices[i].y;
        pTable->v[2][i] = m->apVertices[i].z;
    }
}

// Discarded by the retail linker (no caller in either build): debug 0x537070, between TransformVerts and Project, the
// reverse copy of TransformVerts (table rows back into the mesh vertices). Name inferred.
void J3DAPI rdModel3K_WriteBackVerts(const rdModel3KTableA* pTable, const rdModel3Mesh* m)
{
    size_t i;
    for (i = 0; i < pTable->numVerts; i++) {
        m->apVertices[i].x = pTable->v[0][i];
        m->apVertices[i].y = pTable->v[1][i];
        m->apVertices[i].z = pTable->v[2][i];
    }
}

// Retail 0x004E2BE0: 4 vertices per iteration, x' = (m00*x + m10*y) + (m20*z + dx) and so on per output. The splats are
// declared column by column (the order that reproduces retail's schedule and spills); num is signed (jle/jl).
void J3DAPI rdModel3K_Project(rdMatrix34* pMat, float* px, float* py, float* pz, rdModel3KTableB* pTable, int num)
{
    __m128 m00 = _mm_set_ps1(pMat->rvec.x);
    __m128 m10 = _mm_set_ps1(pMat->lvec.x);
    __m128 m20 = _mm_set_ps1(pMat->uvec.x);
    __m128 dx = _mm_set_ps1(pMat->dvec.x);
    __m128 m01 = _mm_set_ps1(pMat->rvec.y);
    __m128 m11 = _mm_set_ps1(pMat->lvec.y);
    __m128 m21 = _mm_set_ps1(pMat->uvec.y);
    __m128 dy = _mm_set_ps1(pMat->dvec.y);
    __m128 m02 = _mm_set_ps1(pMat->rvec.z);
    __m128 m12 = _mm_set_ps1(pMat->lvec.z);
    __m128 m22 = _mm_set_ps1(pMat->uvec.z);
    __m128 dz = _mm_set_ps1(pMat->dvec.z);
    int i;
    for (i = 0; i < num; i += 4) {
        __m128 x = _mm_load_ps(px + i);
        __m128 y = _mm_load_ps(py + i);
        __m128 z = _mm_load_ps(pz + i);
        __m128 tx = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m00, x), _mm_mul_ps(m10, y)), _mm_add_ps(_mm_mul_ps(m20, z), dx));
        __m128 ty = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m01, x), _mm_mul_ps(m11, y)), _mm_add_ps(_mm_mul_ps(m21, z), dy));
        __m128 tz = _mm_add_ps(_mm_add_ps(_mm_mul_ps(m02, x), _mm_mul_ps(m12, y)), _mm_add_ps(_mm_mul_ps(m22, z), dz));
        _mm_store_ps(pTable->v[0] + i, tx);
        _mm_store_ps(pTable->v[1] + i, ty);
        _mm_store_ps(pTable->v[2] + i, tz);
    }
}
