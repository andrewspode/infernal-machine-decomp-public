#include "rdCache.h"

#include <rdroid/Main/rdroid.h>

#include <std/General/stdEffect.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>

#include <stdlib.h>
#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(rdCache_countfit_1, 15)
CF_PROTO(rdCache_countfit_2, 15)
CF_PROTO(rdCache_countfit_3, 15)
CF_PROTO(rdCache_countfit_4, 15)
CF_PROTO(rdCache_countfit_5, 15)
CF_PROTO(rdCache_countfit_6, 15)
CF_PROTO(rdCache_countfit_7, 15)
CF_PROTO(rdCache_countfit_8, 15)
CF_PROTO(rdCache_countfit_9, 15)
CF_PROTO(rdCache_countfit_10, 8)

/* end count-fit */

#define RDCACHE_VERTBUFFERSIZE (RDCACHE_MAXVERTICES*RDCACHE_MAXFACEVERTICES)

// Zero-initialized external definitions: VC5 emits them to .bss in declaration
// order, which reproduces the retail layout (0x005E10F8-0x014DA920).
// totalVerts keeps its retail spelling from the "totalVerts<(2048*64)" assertion.
rdCacheProcEntry rdCache_aProcFaces[128] = { 0 };
rdVector4 rdCache_aVertIntensities[8192] = { 0 }; // Retail capacity; smaller than rdCache_aVertices
rdCacheProcEntry rdCache_aAlphaProcFaces[2048] = { 0 };
size_t rdCache_numProcFaces = 0;
D3DTLVERTEX rdCache_aVertices[RDCACHE_VERTBUFFERSIZE] = { 0 };
rdVector4 rdCache_aAlphaVertIntensities[RDCACHE_VERTBUFFERSIZE] = { 0 };
D3DTLVERTEX rdCache_aAlphaVertices[RDCACHE_VERTBUFFERSIZE] = { 0 };
LPD3DTLVERTEX rdCache_pCurCacheVertex = NULL;
size_t totalVerts = 0;
size_t rdCache_numAlphaProcFaces = 0;
LPD3DTLVERTEX rdCache_pCurInVert = NULL;
uint16_t rdCache_aVertIndices[RDCACHE_VERTBUFFERSIZE * 3] = { 0 };
D3DTLVERTEX rdCache_aHWVertices[RDCACHE_VERTBUFFERSIZE] = { 0 };
size_t rdCache_drawnFaces = 0;
size_t rdCache_frameNum = 0;
size_t rdCache_numUsedVertices = 0;
size_t rdCache_numUsedAlphaVertices = 0;

int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2);
int J3DAPI rdCache_ProcFaceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2);

void J3DAPI rdCache_Startup(void)
{
    // Retail clears only STD_ARRAYLEN(rdCache_aHWVertices) bytes
    memset(rdCache_aHWVertices, 0, STD_ARRAYLEN(rdCache_aHWVertices));
    rdCache_frameNum = 0;
}

void J3DAPI rdCache_AdvanceFrame(void)
{
    rdCache_drawnFaces = 0;
    ++rdCache_frameNum;

    rdCache_numProcFaces         = 0;
    rdCache_numUsedVertices      = 0;
    rdCache_numAlphaProcFaces    = 0;
    rdCache_numUsedAlphaVertices = 0;
}

int J3DAPI rdCache_GetFrameNum(void)
{
    return rdCache_frameNum;
}

/* Discarded by the retail linker (no caller in either image); the debug build has it at 0x00507C51, right after
   GetFrameNum. Name inferred. */
size_t J3DAPI rdCache_GetDrawnFaces(void)
{
    return rdCache_drawnFaces;
}

rdCacheProcEntry* J3DAPI rdCache_GetProcEntry(void)
{
    rdCacheProcEntry* pProcEntry;

    if ( rdCache_numProcFaces >= STD_ARRAYLEN(rdCache_aProcFaces) )
    {
        rdCache_Flush();
    }

    pProcEntry                   = &rdCache_aProcFaces[rdCache_numProcFaces];
    pProcEntry->aVertices        = &rdCache_aVertices[rdCache_numUsedVertices];
    pProcEntry->aVertIntensities = &rdCache_aVertIntensities[rdCache_numUsedVertices];
    return pProcEntry;
}

rdCacheProcEntry* J3DAPI rdCache_GetAlphaProcEntry(void)
{
    rdCacheProcEntry* pProcEntry;

    if ( rdCache_numAlphaProcFaces >= STD_ARRAYLEN(rdCache_aAlphaProcFaces) )
    {
        rdCache_Flush();
        rdCache_FlushAlpha();
    }

    pProcEntry                   = &rdCache_aAlphaProcFaces[rdCache_numAlphaProcFaces];
    pProcEntry->aVertices        = &rdCache_aAlphaVertices[rdCache_numUsedAlphaVertices];
    pProcEntry->aVertIntensities = &rdCache_aAlphaVertIntensities[rdCache_numUsedAlphaVertices];
    return pProcEntry;
}

void J3DAPI rdCache_Flush(void)
{
    if ( !rdCache_numProcFaces )
    {
        return;
    }

    switch ( rdroid_curGeometryMode )
    {
        case RD_GEOMETRY_NONE:
            break;

        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            rdCache_SendWireframeFaceListToHardware(rdCache_numProcFaces, rdCache_aProcFaces);
            break;

        case RD_GEOMETRY_SOLID:
        case RD_GEOMETRY_FULL:
        default:
            rdCache_SendFaceListToHardware(rdCache_numProcFaces, rdCache_aProcFaces, rdCache_ProcFaceCompare);
            break;
    }

    rdCache_drawnFaces      += rdCache_numProcFaces;
    rdCache_numProcFaces    = 0;
    rdCache_numUsedVertices = 0;
}

void J3DAPI rdCache_FlushAlpha(void)
{
    if ( !rdCache_numAlphaProcFaces )
    {
        return;
    }

    switch ( rdroid_curGeometryMode )
    {
        case RD_GEOMETRY_NONE:
            break;

        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            rdCache_SendWireframeFaceListToHardware(rdCache_numAlphaProcFaces, rdCache_aAlphaProcFaces);
            break;

        case RD_GEOMETRY_SOLID:
        case RD_GEOMETRY_FULL:
        default:
            rdCache_SendFaceListToHardware(rdCache_numAlphaProcFaces, rdCache_aAlphaProcFaces, rdCache_ProcFaceDistanceCompare);
            break;
    }

    rdCache_drawnFaces           += rdCache_numAlphaProcFaces;
    rdCache_numAlphaProcFaces    = 0;
    rdCache_numUsedAlphaVertices = 0;
}

void J3DAPI rdCache_AddProcFace(size_t numVerts)
{
    rdCacheProcEntry* pEntry;

    pEntry = &rdCache_aProcFaces[rdCache_numProcFaces];
    pEntry->numVertices = numVerts;
    rdCache_numUsedVertices += numVerts;
    ++rdCache_numProcFaces;
}

void J3DAPI rdCache_AddAlphaProcFace(size_t numVertices)
{
    float distance;
    float sz;
    rdCacheProcEntry* pItem;
    size_t i;

    pItem = &rdCache_aAlphaProcFaces[rdCache_numAlphaProcFaces];
    pItem->numVertices = numVertices;

    distance = 3.4028235e38f;
    for ( i = 0; i < numVertices; ++i )
    {
        sz = pItem->aVertices[i].sz;
        if ( sz < distance )
        {
            distance = sz;
        }
    }

    pItem->distance = distance;
    rdCache_numUsedAlphaVertices += numVertices;
    ++rdCache_numAlphaProcFaces;
}

void J3DAPI rdCache_SendFaceListToHardware(size_t numPolys, rdCacheProcEntry* pCurPoly, rdCacheSortFunc pfSort)
{
    size_t polyNum;
    rdFaceFlags fflags;
    Std3DRenderState rdflags;
    rdMaterial* pCurMat;
    int curMatCelNum;
    StdColorFormatType format;
    tSystemTexture* pTex;
    LPDIRECT3DTEXTURE2 pD3DCachedTex;
    size_t totalIndices;
    int curVertIdx;
    rdLightMode lightingMode;
    size_t i;
    float red;
    float green;
    float blue;
    float alpha;
    int triCount;
    int triPoint1Num;
    int triPoint2Num;
    int triPoint3Num;
    int triNum;
    const tStdFadeFactor* pFadeFactor;
    float matWidth;
    float matHeight;

    pFadeFactor = stdEffect_GetFadeFactor();
    if ( pfSort == rdCache_ProcFaceDistanceCompare )
    {
        qsort(pCurPoly, numPolys, sizeof(rdCacheProcEntry), pfSort);
    }

    for ( polyNum = 0; polyNum < numPolys; )
    {
        rdflags = STD3D_RS_SUBPIXEL_CORRECTION | STD3D_RS_UNKNOWN_2 | STD3D_RS_UNKNOWN_1;
        fflags  = pCurPoly->flags;
        if ( (fflags & RD_FF_TEX_CLAMP_X) != 0 )
        {
            rdflags |= STD3D_RS_TEX_CPAMP_U;
        }

        if ( (fflags & RD_FF_TEX_CLAMP_Y) != 0 )
        {
            rdflags |= STD3D_RS_TEX_CPAMP_V;
        }

        if ( (fflags & RD_FF_TEX_FILTER_NEAREST) == 0 )
        {
            rdflags |= STD3D_RS_TEXFILTER_BILINEAR;
        }

        if ( (fflags & RD_FF_ZWRITE_DISABLED) != 0 )
        {
            rdflags |= STD3D_RS_ZWRITE_DISABLED;
        }

        if ( (fflags & RD_FF_FOG_ENABLED) != 0 )
        {
            rdflags |= STD3D_RS_FOG_ENABLED;
        }

        switch ( rdroid_curGeometryMode )
        {
            case RD_GEOMETRY_SOLID:
                pCurMat = NULL;
                break;

            default:
                pCurMat = pCurPoly->pMaterial;
                break;
        }

        if ( pCurMat )
        {
            format = pCurMat->formatType;
            if ( format == STDCOLOR_FORMAT_RGBA_1BIT_ALPHA )
            {
                rdflags |= STD3D_RS_ALPHAREF_SET;
            }

            curMatCelNum = pCurPoly->matCelNum;
            if ( curMatCelNum == -1 )
            {
                pTex = &pCurPoly->pMaterial->aTextures[STDMATH_CLAMP((int)pCurPoly->pMaterial->curCelNum, 0, (int)pCurPoly->pMaterial->numCels - 1)];
            }
            else
            {
                pTex = &pCurPoly->pMaterial->aTextures[STDMATH_CLAMP(curMatCelNum, 0, (int)pCurPoly->pMaterial->numCels - 1)];
            }

            rdCache_AddToTextureCache(pTex, format);
            pD3DCachedTex = pTex->pD3DCachedTex;
            matWidth  = (float)pCurMat->width;
            matHeight = (float)pCurMat->height;
        }
        else
        {
            pD3DCachedTex = NULL;
        }

        totalIndices       = 0;
        totalVerts = 0;
        do
        {
            curVertIdx = totalVerts;
            lightingMode = pCurPoly->lightingMode < rdroid_curLightingMode ? pCurPoly->lightingMode : rdroid_curLightingMode;

            rdCache_pCurInVert      = pCurPoly->aVertices;
            rdCache_pCurCacheVertex = &rdCache_aHWVertices[totalVerts];
            for ( i = 0; i < pCurPoly->numVertices; ++i )
            {
                *rdCache_pCurCacheVertex = *rdCache_pCurInVert;
                ++rdCache_pCurInVert;

                switch ( lightingMode )
                {
                    case RD_LIGHTING_GOURAUD:
                        red   = pCurPoly->aVertIntensities[i].red + pCurPoly->extraLight.red;
                        green = pCurPoly->aVertIntensities[i].green + pCurPoly->extraLight.green;
                        blue  = pCurPoly->aVertIntensities[i].blue + pCurPoly->extraLight.blue;
                        if ( pCurPoly->extraLight.alpha < 1.0f )
                        {
                            alpha = pCurPoly->aVertIntensities[i].alpha + pCurPoly->extraLight.alpha;
                        }
                        else
                        {
                            alpha = pCurPoly->aVertIntensities[i].alpha;
                        }
                        break;

                    case RD_LIGHTING_LIT:
                    case RD_LIGHTING_DIFFUSE:
                        red   = pCurPoly->extraLight.red;
                        green = pCurPoly->extraLight.green;
                        blue  = pCurPoly->extraLight.blue;
                        alpha = pCurPoly->aVertIntensities->alpha;
                        break;

                    default:
                        red = green = blue = 1.0f;
                        alpha = pCurPoly->aVertIntensities->alpha;
                        break;
                }

                if ( pFadeFactor->bEnabled )
                {
                    red   = D3DMultiply(pFadeFactor->factor, red);
                    green = D3DMultiply(pFadeFactor->factor, green);
                    blue  = D3DMultiply(pFadeFactor->factor, blue);
                }

                red   = STDMATH_CLAMP(red, 0.0f, 1.0f);
                green = STDMATH_CLAMP(green, 0.0f, 1.0f);
                blue  = STDMATH_CLAMP(blue, 0.0f, 1.0f);
                alpha = STDMATH_CLAMP(alpha, 0.0f, 1.0f);

                if ( (fflags & RD_FF_TEX_TRANSLUCENT) != 0 )
                {
                    rdCache_pCurCacheVertex->color = D3DRGBA(red, green, blue, alpha);
                }
                else
                {
                    rdCache_pCurCacheVertex->color = D3DRGBA(red, green, blue, 1.0f);
                }

                ++rdCache_pCurCacheVertex;
                ++totalVerts;
            }

            // Triangulation
            if ( pCurPoly->numVertices > 3 )
            {
                triCount     = pCurPoly->numVertices - 2;
                triPoint1Num = 0;
                triPoint2Num = 1;
                triPoint3Num = pCurPoly->numVertices - 1;
                for ( triNum = 0; triNum < triCount; ++triNum )
                {
                    rdCache_aVertIndices[totalIndices++] = curVertIdx + triPoint1Num;
                    rdCache_aVertIndices[totalIndices++] = curVertIdx + triPoint2Num;
                    rdCache_aVertIndices[totalIndices++] = curVertIdx + triPoint3Num;
                    if ( !(triNum & 1) != 0 )
                    {
                        triPoint1Num = triPoint2Num++;
                    }
                    else
                    {
                        triPoint1Num = triPoint3Num--;
                    }
                }
            }
            else
            {
                rdCache_aVertIndices[totalIndices++] = curVertIdx;
                rdCache_aVertIndices[totalIndices++] = curVertIdx + 1;
                rdCache_aVertIndices[totalIndices++] = curVertIdx + 2;
            }

            ++pCurPoly;
            ++polyNum;
            if ( totalIndices + 3 * (pCurPoly->numVertices - 2) >= std3D_maxVertices )
            {
                break;
            }
        }
        while ( polyNum < numPolys
            && pCurMat == pCurPoly->pMaterial
            && curMatCelNum == pCurPoly->matCelNum
            && fflags == pCurPoly->flags );

#line 517
        RD_ASSERTREL(totalVerts<RDCACHE_VERTBUFFERSIZE);
        std3D_DrawRenderList(pD3DCachedTex, rdflags, rdCache_aHWVertices, totalVerts, rdCache_aVertIndices, totalIndices);
    }
}

void J3DAPI rdCache_SendWireframeFaceListToHardware(size_t numPolys, rdCacheProcEntry* pCurPoly)
{
    size_t i;
    size_t idx;
    D3DTLVERTEX aVerts[2];

    std3D_SetWireframeRenderState();

    for ( i = 0; i < numPolys; ++i )
    {
        for ( idx = 0; idx < pCurPoly->numVertices; ++idx )
        {
            pCurPoly->aVertices[idx].color = RGBA_MAKE(255, 255, 255, 255);
        }

        switch ( rdroid_curGeometryMode )
        {
            case RD_GEOMETRY_VERTEX:
                // Retail draws the hardware vertex buffer here
                std3D_DrawPointList(rdCache_aHWVertices, pCurPoly->numVertices);
                break;

            case RD_GEOMETRY_WIREFRAME:
                std3D_DrawLineStrip(pCurPoly->aVertices, pCurPoly->numVertices);

                // Draw end vert twice to complete the line
                aVerts[0] = pCurPoly->aVertices[pCurPoly->numVertices - 1];
                aVerts[1] = pCurPoly->aVertices[0];
                std3D_DrawLineStrip(aVerts, 2);
                break;
        }

        ++pCurPoly;
    }
}

void J3DAPI rdCache_AddToTextureCache(tSystemTexture* pTexture, StdColorFormatType format)
{
    if ( !pTexture->pD3DCachedTex )
    {
        std3D_AddToTextureCache(pTexture, format);
    }
    else
    {
        std3D_UpdateFrameCount(pTexture);
    }
}

int J3DAPI rdCache_ProcFaceDistanceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2)
{
    const rdCacheProcEntry* pFace1 = pEntry1;
    const rdCacheProcEntry* pCurFace2 = pEntry2;
    if ( pCurFace2->distance > pFace1->distance )
    {
        return 1;
    }

    return -1;
}

int J3DAPI rdCache_ProcFaceCompare(const rdCacheProcEntry* pEntry1, const rdCacheProcEntry* pEntry2)
{
    const rdCacheProcEntry* pCacheFaceLhs = pEntry1;
    const rdCacheProcEntry* pCacheFaceRhs = pEntry2;
    rdMaterial* pMat1 = pCacheFaceLhs->pMaterial;
    rdMaterial* pMat2 = pCacheFaceRhs->pMaterial;
    if ( pMat1 == pMat2 ) {
        return pCacheFaceRhs->matCelNum - pCacheFaceLhs->matCelNum;
    }

    return pMat2 - pMat1;
}
