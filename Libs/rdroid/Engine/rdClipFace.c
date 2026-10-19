#include "rdClip.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdQClip.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>

// The primitive and face entry points of the clipper: copy an rdPrimit3 into
// clip space and run the quick clippers, or project a clipped face into a
// D3D vertex list. A separate object from rdClip.c: retail pools its own 0.0f
// (0x00509C10, after rdClip.c's 0x00509C0C) and the debug build starts its
// code on a fresh 16-byte boundary with 0xCC fill (0x00526B00).
// Retail's .bss: aWorkFaceVerts at 0x014DE4F8, right after rdClip.c's block.
static rdVector3 rdClip_aWorkFaceVerts[144] = { 0 };

// Discarded by retail's linker; restored from the debug build (0x00526B00, between
// SphereInFrustrum and QFace3W; no caller in either image). The textured gouraud sibling of
// rdClip_QFace3W: rdPrimit3_ClipFace's RD_GEOMETRY_FULL/RD_LIGHTING_GOURAUD copy (the alpha
// sums the extra intensity here), then the quick near-plane clip. Name inferred.
void J3DAPI rdClip_QFace3T(const rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDst, const rdVector2* pTexVertOffset)
{
    size_t i;

    if ( pSrc->aVertIntensities )
    {
        for ( i = 0; i < pSrc->numVertices; ++i )
        {
            rdVector_Copy3(&pDst->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
            rdVector_Copy2(&pDst->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

            pDst->aTexVertices[i].x = pDst->aTexVertices[i].x + pTexVertOffset->x;
            pDst->aTexVertices[i].y = pDst->aTexVertices[i].y + pTexVertOffset->y;

            pDst->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);
            pDst->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);
            pDst->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);
            pDst->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha + pSrc->aVertIntensities[i].alpha, 0.0f, 1.0f);
        }
    }
    else
    {
        for ( i = 0; i < pSrc->numVertices; ++i )
        {
            rdVector_Copy3(&pDst->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
            rdVector_Copy2(&pDst->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

            pDst->aTexVertices[i].x = pDst->aTexVertices[i].x + pTexVertOffset->x;
            pDst->aTexVertices[i].y = pDst->aTexVertices[i].y + pTexVertOffset->y;

            pDst->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red, 0.0f, 1.0f);
            pDst->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green, 0.0f, 1.0f);
            pDst->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue, 0.0f, 1.0f);
            pDst->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
        }
    }

    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        pDst->numVertices = rdQClip_VerticesInFrustrum(pFrustrum, pDst->aVertices, pSrc->numVertices);
        if ( pDst->numVertices > 0 )
        {
            pDst->numVertices = rdQClip_Face3T(pFrustrum, pDst->aVertices, pDst->aTexVertices, pDst->aVertLights, pSrc->numVertices);
        }
    }
    else
    {
        pDst->numVertices = rdClip_Face3TOrtho(pFrustrum, pDst->aVertices, pDst->aTexVertices, pDst->aVertLights, pSrc->numVertices);
    }
}

void J3DAPI rdClip_QFace3W(const rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDst)
{
    size_t i;

    for ( i = 0; i < pSrc->numVertices; ++i )
    {
        rdVector_Copy3(&pDst->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
    }

    pDst->numVertices = rdQClip_VerticesInFrustrum(pFrustrum, pDst->aVertices, pSrc->numVertices);
    if ( pDst->numVertices > 0 )
    {
        pDst->numVertices = rdQClip_Face3W(pFrustrum, pDst->aVertices, pSrc->numVertices);
    }
}

int J3DAPI rdClip_FaceToPlane(const rdClipFrustum* pFrustrum, rdCacheProcEntry* pProcFace, const rdFace* pFace, const rdVector3* aVerts, const rdVector2* aTexVerts, const rdVector4* aLightColors, const rdVector4* aVertColors)
{
    size_t i;
    float invY;
    float ccenterX;
    float ccenterY;
    float invNearPlane;
    float invFarPlane;
    float scale;
    LPD3DTLVERTEX pOutVert;
    size_t vertIdx;

    // Clip and transform face vertices to screen space then assigns pProcFace

    for ( i = 0; i < pFace->numVertices; ++i )
    {
        vertIdx = pFace->aVertices[i];
        rdVector_Copy3(&rdClip_aWorkFaceVerts[i], &aVerts[vertIdx]);
    }

    if ( !rdQClip_VerticesInFrustrum(pFrustrum, rdClip_aWorkFaceVerts, pFace->numVertices) )
    {
        return 0;
    }

    ccenterX = rdCamera_pCurCamera->pCanvas->center.x;
    ccenterY = rdCamera_pCurCamera->pCanvas->center.y;

    invNearPlane = rdCamera_pCurCamera->invNearClipPlane;
    invFarPlane = rdCamera_pCurCamera->invFarClipPlane;

    if ( aLightColors )
    {
        if ( aVertColors )
        {
            for ( i = 0; i < pFace->numVertices; ++i )
            {
                invY = 1.0f / (rdClip_aWorkFaceVerts[i].y);
                scale = RD_MULTIPLY(rdCamera_pCurCamera->focalLength, invY);

                pOutVert = &pProcFace->aVertices[i];
                pOutVert->sx = D3DVAL(D3DMultiply(D3DVAL(rdClip_aWorkFaceVerts[i].x), scale) + ccenterX);
                pOutVert->sy = D3DVAL(-D3DMultiply(D3DVAL(rdClip_aWorkFaceVerts[i].z), scale) + ccenterY);
                pOutVert->sz = D3DVAL(D3DMultiply(invY - invNearPlane, invFarPlane));

                pOutVert->rhw = D3DVAL(D3DMultiply(invY, 0.03125f)); // 0.03125f - 1/ 32

                pOutVert->tu = D3DVAL(aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x);
                pOutVert->tv = D3DVAL(aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y);

                vertIdx = pFace->aVertices[i];
                rdVector_Set4(&pProcFace->aVertIntensities[i],
                    STDMATH_CLAMP(aLightColors[vertIdx].x + aVertColors[i].x, 0.0f, 1.0f),
                    STDMATH_CLAMP(aLightColors[vertIdx].y + aVertColors[i].y, 0.0f, 1.0f),
                    STDMATH_CLAMP(aLightColors[vertIdx].z + aVertColors[i].z, 0.0f, 1.0f),
                    STDMATH_CLAMP(aLightColors[vertIdx].w + aVertColors[i].w, 0.0f, 1.0f));
            }
        }
        else
        {
            for ( i = 0; i < pFace->numVertices; ++i )
            {
                invY = 1.0f / (rdClip_aWorkFaceVerts[i].y);
                scale = RD_MULTIPLY(rdCamera_pCurCamera->focalLength, invY);

                pOutVert = &pProcFace->aVertices[i];
                pOutVert->sx = D3DVAL(D3DMultiply(D3DVAL(rdClip_aWorkFaceVerts[i].x), scale) + ccenterX);
                pOutVert->sy = D3DVAL(-D3DMultiply(D3DVAL(rdClip_aWorkFaceVerts[i].z), scale) + ccenterY);
                pOutVert->sz = D3DVAL(D3DMultiply(invY - invNearPlane, invFarPlane));

                pOutVert->rhw = D3DVAL(D3DMultiply(invY, 0.03125f)); // 0.03125f - 1/ 32

                pOutVert->tu = D3DVAL(aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x);
                pOutVert->tv = D3DVAL(aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y);

                vertIdx = pFace->aVertices[i];
                rdVector_Copy4(&pProcFace->aVertIntensities[i], &aLightColors[vertIdx]);
            }
        }
    }
    else
    {
        for ( i = 0; i < pFace->numVertices; ++i )
        {
            invY = 1.0f / (rdClip_aWorkFaceVerts[i].y);
            scale = RD_MULTIPLY(rdCamera_pCurCamera->focalLength, invY);

            pOutVert = &pProcFace->aVertices[i];
            pOutVert->sx = D3DVAL(D3DMultiply(D3DVAL(rdClip_aWorkFaceVerts[i].x), scale) + ccenterX);
            pOutVert->sy = D3DVAL(-D3DMultiply(D3DVAL(rdClip_aWorkFaceVerts[i].z), scale) + ccenterY);
            pOutVert->sz = D3DVAL(D3DMultiply(invY - invNearPlane, invFarPlane));

            pOutVert->rhw = D3DVAL(D3DMultiply(invY, 0.03125f)); // 0.03125f - 1/ 32

            pOutVert->tu = D3DVAL(aTexVerts[pFace->aTexVertices[i]].x + pFace->texVertOffset.x);
            pOutVert->tv = D3DVAL(aTexVerts[pFace->aTexVertices[i]].y + pFace->texVertOffset.y);
        }
    }
}

void J3DAPI rdClip_VerticesToPlane(rdCacheProcEntry* pProcFace, const rdVector3* aVerts, const rdVector2* aTexVerts, size_t numVerts)
{
    size_t i;
    float scale;
    LPD3DTLVERTEX pOutVert;
    // Transform vertices to screen space then assigns pProcFace

    float ccenterX = rdCamera_pCurCamera->pCanvas->center.x;
    float ccenterY = rdCamera_pCurCamera->pCanvas->center.y;

    float invNearClipPlane = rdCamera_pCurCamera->invNearClipPlane;
    float invFarClipPlane  = rdCamera_pCurCamera->invFarClipPlane;
    float invY;

    for ( i = 0; i < numVerts; ++i )
    {
        invY = 1.0f / (aVerts[i].y);
        scale = RD_MULTIPLY(rdCamera_pCurCamera->focalLength, invY);

        pOutVert = &pProcFace->aVertices[i];
        pOutVert->sx = D3DVAL(D3DMultiply(D3DVAL(aVerts[i].x), scale) + ccenterX);
        pOutVert->sy = D3DVAL(-D3DMultiply(D3DVAL(aVerts[i].z), scale) + ccenterY);
        pOutVert->sz = D3DVAL(D3DMultiply(invY - invNearClipPlane, invFarClipPlane));

        pOutVert->rhw = D3DVAL(D3DMultiply(invY, 0.03125f)); // 0.03125f - 1/ 32

        pOutVert->tu = D3DVAL(aTexVerts[i].x);
        pOutVert->tv = D3DVAL(aTexVerts[i].y);
    }
}
