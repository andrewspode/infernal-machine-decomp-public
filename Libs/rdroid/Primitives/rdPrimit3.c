#include "rdPrimit3.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPrimit2.h>


// Linker-discarded (no retail symbol, no caller even in the debug image); the debug build's text (0x514700).
// Jedi Knight's name.
int J3DAPI rdPrimit3_GetScreenCoord(const rdVector3* pPoint, rdScreenPoint* pScreenPt)
{
    rdVector3 vertex;
    rdVector3 projPoint;

    rdMatrix_TransformPoint34(&vertex, pPoint, &rdCamera_pCurCamera->orient);
    if ( rdClip_Point3(rdCamera_pCurCamera->pFrustum, &vertex) )
    {
        rdCamera_pCurCamera->pfProject(&projPoint, &vertex);
        if ( pScreenPt )
        {
            pScreenPt->x = (int32_t)projPoint.x;
            pScreenPt->y = (int32_t)projPoint.y;
            pScreenPt->z = projPoint.z;
        }

        return 1;
    }

    return 0;
}

// Linker-discarded; the debug build's text (0x514784). Jedi Knight's name.
int J3DAPI rdPrimit3_DrawPoint(const rdVector3* pPoint, rdScreenPoint* pScreenPt, uint32_t color)
{
    rdVector3 vertex;
    rdVector3 projPoint;
    int32_t x;
    int32_t y;

    rdMatrix_TransformPoint34(&vertex, pPoint, &rdCamera_pCurCamera->orient);
    if ( rdClip_Point3(rdCamera_pCurCamera->pFrustum, &vertex) )
    {
        rdCamera_pCurCamera->pfProject(&projPoint, &vertex);
        x = (int32_t)projPoint.x;
        y = (int32_t)projPoint.y;
        if ( pScreenPt )
        {
            pScreenPt->x = x;
            pScreenPt->y = y;
            pScreenPt->z = projPoint.z;
        }

        if ( rdCamera_pCurCamera->pCanvas->pVBuffer->rasterInfo.colorInfo.bpp == 8 )
        {
            *(rdCamera_pCurCamera->pCanvas->pVBuffer->pPixels + y * rdCamera_pCurCamera->pCanvas->pVBuffer->rasterInfo.rowSize + x) = (uint8_t)color;
        }
        else
        {
            *((uint16_t*)rdCamera_pCurCamera->pCanvas->pVBuffer->pPixels + y * rdCamera_pCurCamera->pCanvas->pVBuffer->rasterInfo.rowWidth + x) = (uint16_t)color;
        }

        return 1;
    }

    return 0;
}

// Linker-discarded; the debug build's text (0x514886). Jedi Knight's name.
int J3DAPI rdPrimit3_DrawClippedLine(const rdVector3* pPoint1, const rdVector3* pPoint2, rdScreenPoint* pScreenPt1, rdScreenPoint* pScreenPt2, int* pOut1, int* pOut2, uint32_t color, uint32_t pattern)
{
    rdVector3 vertex1;
    rdVector3 vertex2;
    rdVector3 projPoint1;
    rdVector3 projPoint2;
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;

    rdMatrix_TransformPoint34(&vertex1, pPoint1, &rdCamera_pCurCamera->orient);
    rdMatrix_TransformPoint34(&vertex2, pPoint2, &rdCamera_pCurCamera->orient);
    if ( rdClip_Line3Project(rdCamera_pCurCamera->pFrustum, &vertex1, &vertex2, pOut1, pOut2) )
    {
        rdCamera_pCurCamera->pfProject(&projPoint1, &vertex1);
        rdCamera_pCurCamera->pfProject(&projPoint2, &vertex2);

        x1 = (int32_t)projPoint1.x;
        y1 = (int32_t)projPoint1.y;
        if ( pScreenPt1 )
        {
            pScreenPt1->x = x1;
            pScreenPt1->y = y1;
            pScreenPt1->z = projPoint1.z;
        }

        x2 = (int32_t)projPoint2.x;
        y2 = (int32_t)projPoint2.y;
        if ( pScreenPt2 )
        {
            pScreenPt2->x = x2;
            pScreenPt2->y = y2;
            pScreenPt2->z = projPoint2.z;
        }

        rdPrimit2_DrawLine(rdCamera_pCurCamera->pCanvas, x1, y1, x2, y2, color, pattern);
        return 1;
    }

    return 0;
}

// Linker-discarded; the debug build's text (0x5149b2). Jedi Knight's name.
void J3DAPI rdPrimit3_DrawLine(const rdVector3* pPoint1, const rdVector3* pPoint2, uint32_t color)
{
    rdVector3 vertex1;
    rdVector3 vertex2;
    rdVector3 projPoint1;
    rdVector3 projPoint2;

    rdMatrix_TransformPoint34(&vertex1, pPoint1, &rdCamera_pCurCamera->orient);
    rdMatrix_TransformPoint34(&vertex2, pPoint2, &rdCamera_pCurCamera->orient);
    rdCamera_pCurCamera->pfProject(&projPoint1, &vertex1);
    rdCamera_pCurCamera->pfProject(&projPoint2, &vertex2);
    rdPrimit2_DrawClippedLine2(projPoint1.x, projPoint1.y, projPoint2.x, projPoint2.y, color);
}

void J3DAPI rdPrimit3_DrawClippedCircle(const rdVector3* pPos, float radius, float step, uint32_t color, uint32_t pattern)
{
    rdVector3 center;
    rdVector3 offset;
    rdVector3 circlePoint;
    rdVector3 projCenter;
    rdVector3 projCirclePoint;
    int32_t x;
    int32_t y;
    float radius2D;

    rdMatrix_TransformPoint34(&center, pPos, &rdCamera_pCurCamera->orient);

    rdVector_Set3(&offset, radius, 0.0f, 0.0f);
    rdVector_Add3(&circlePoint, &offset, &center);

    if ( center.y > 0.0f )
    {
        rdCamera_pCurCamera->pfProject(&projCenter, &center);
        rdCamera_pCurCamera->pfProject(&projCirclePoint, &circlePoint);

        x = (int32_t)(projCenter.x + 0.5f);
        y = (int32_t)(projCenter.y + 0.5f);
        radius2D = projCirclePoint.x - projCenter.x;

        rdPrimit2_DrawClippedCircle(rdCamera_pCurCamera->pCanvas, x, y, radius2D, step, color, pattern);
    }
}

// Linker-discarded (no retail symbol, no caller even in the debug image); the debug build's text (0x514b0f):
// ClipFace's per-mode copies, taking the vertex/texture indices from an rdFace (no texture offset, no alpha
// clamp), then the face's vertex count. Name inferred.
void J3DAPI rdPrimit3_GetFaceVerts(const rdFace* pFace, rdGeometryMode geoMode, rdLightMode lightMode, const rdPrimit3* pSrc, rdPrimit3* pDest)
{
    size_t i;

    switch ( geoMode )
    {
        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            for ( i = 0; i < pSrc->numVertices; ++i )
            {
                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
            }

            pDest->numVertices = pFace->numVertices;
            break;

        case RD_GEOMETRY_NONE:
            for ( i = 0; i < pSrc->numVertices; ++i )
            {
                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
            }

            pDest->numVertices = pFace->numVertices;
            break;

        case RD_GEOMETRY_SOLID:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
                    }

                    pDest->numVertices = pFace->numVertices;
                    break;

                case RD_LIGHTING_DIFFUSE:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
                    }

                    pDest->numVertices = pFace->numVertices;
                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                            for ( i = 0; i < pSrc->numVertices; ++i )
                            {
                                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);

                                pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);
                                pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);
                                pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);
                            }
                    }
                    else
                    {
                            for ( i = 0; i < pSrc->numVertices; ++i )
                            {
                                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);

                                pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].red, 0.0f, 1.0f);
                                pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].green, 0.0f, 1.0f);
                                pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].blue, 0.0f, 1.0f);
                            }
                    }

                    pDest->numVertices = pFace->numVertices;
            }

            break;

        case RD_GEOMETRY_FULL:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                case RD_LIGHTING_DIFFUSE:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
                        rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pFace->aTexVertices[i]]);
                    }

                    pDest->numVertices = pFace->numVertices;
                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                            for ( i = 0; i < pSrc->numVertices; ++i )
                            {
                                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
                                rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pFace->aTexVertices[i]]);

                                pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);
                                pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);
                                pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);
                            }
                    }
                    else
                    {
                            for ( i = 0; i < pSrc->numVertices; ++i )
                            {
                                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pFace->aVertices[i]]);
                                rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pFace->aTexVertices[i]]);

                                pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].red, 0.0f, 1.0f);
                                pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].green, 0.0f, 1.0f);
                                pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pFace->aVertices[i]].blue, 0.0f, 1.0f);
                            }
                    }

                    pDest->numVertices = pFace->numVertices;
                    break;

                default:
                    return;
            }

            break;

        default:
            return;
    }
}

// Linker-discarded (no retail symbol, no caller even in the debug image); the debug build's text (0x51577f):
// ClipFace's mode switch clipping a face already built in place. Name inferred.
void J3DAPI rdPrimit3_ClipFaceVerts(const rdClipFrustum* pFrustrum, rdGeometryMode geoMode, rdLightMode lightMode, rdPrimit3* pFace)
{
    switch ( geoMode )
    {
        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
            {
                pFace->numVertices = rdClip_Face3W(pFrustrum, pFace->aVertices, pFace->numVertices);
            }
            else
            {
                pFace->numVertices = rdClip_Face3WOrtho(pFrustrum, pFace->aVertices, pFace->numVertices);
            }

            break;

        case RD_GEOMETRY_NONE:
            if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
            {
                pFace->numVertices = rdClip_Face3S(pFrustrum, pFace->aVertices, pFace->numVertices);
            }
            else
            {
                pFace->numVertices = rdClip_Face3SOrtho(pFrustrum, pFace->aVertices, pFace->numVertices);
            }

            break;

        case RD_GEOMETRY_SOLID:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pFace->numVertices = rdClip_Face3S(pFrustrum, pFace->aVertices, pFace->numVertices);
                    }
                    else
                    {
                        pFace->numVertices = rdClip_Face3SOrtho(pFrustrum, pFace->aVertices, pFace->numVertices);
                    }

                    break;

                case RD_LIGHTING_DIFFUSE:
                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pFace->numVertices = rdClip_Face3S(pFrustrum, pFace->aVertices, pFace->numVertices);
                    }
                    else
                    {
                        pFace->numVertices = rdClip_Face3SOrtho(pFrustrum, pFace->aVertices, pFace->numVertices);
                    }

                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pFace->numVertices = rdClip_Face3GS(pFrustrum, pFace->aVertices, pFace->aVertLights, pFace->numVertices);
                    }
                    else
                    {
                        pFace->numVertices = rdClip_Face3GSOrtho(pFrustrum, pFace->aVertices, pFace->aVertLights, pFace->numVertices);
                    }

            }

            break;

        case RD_GEOMETRY_FULL:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                case RD_LIGHTING_DIFFUSE:
                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pFace->numVertices = rdClip_Face3GT(pFrustrum, pFace->aVertices, pFace->aTexVertices, pFace->numVertices);
                    }
                    else
                    {
                        pFace->numVertices = rdClip_Face3GTOrtho(pFrustrum, pFace->aVertices, pFace->aTexVertices, pFace->numVertices);
                    }

                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pFace->numVertices = rdClip_Face3T(pFrustrum, pFace->aVertices, pFace->aTexVertices, pFace->aVertLights, pFace->numVertices);
                    }
                    else
                    {
                        pFace->numVertices = rdClip_Face3TOrtho(pFrustrum, pFace->aVertices, pFace->aTexVertices, pFace->aVertLights, pFace->numVertices);
                    }

                    break;

                default:
                    return;
            }

            break;

        default:
            return;
    }
}

void J3DAPI rdPrimit3_ClipFace(const rdClipFrustum* pFrustrum, rdGeometryMode geoMode, rdLightMode lightMode, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTexVertOffset)
{
    size_t i;

    switch ( geoMode )
    {
        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            for ( i = 0; i < pSrc->numVertices; ++i )
            {
                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
            }

            if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
            {
                pDest->numVertices = rdClip_Face3W(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }
            else
            {
                pDest->numVertices = rdClip_Face3WOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }

            break;

        case RD_GEOMETRY_NONE:
            for ( i = 0; i < pSrc->numVertices; ++i )
            {
                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
            }

            if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
            {
                pDest->numVertices = rdClip_Face3S(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }
            else
            {
                pDest->numVertices = rdClip_Face3SOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
            }

            break;

        case RD_GEOMETRY_SOLID:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                    }

                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3S(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3SOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }

                    break;

                case RD_LIGHTING_DIFFUSE:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                    }

                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3S(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3SOrtho(pFrustrum, pDest->aVertices, pSrc->numVertices);
                    }

                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);

                            // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }
                    else
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);

                            // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }

                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3GS(pFrustrum, pDest->aVertices, pDest->aVertLights, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3GSOrtho(pFrustrum, pDest->aVertices, pDest->aVertLights, pSrc->numVertices);
                    }
            }

            break;

        case RD_GEOMETRY_FULL:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                case RD_LIGHTING_DIFFUSE:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                        rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

                        pDest->aTexVertices[i].x = pDest->aTexVertices[i].x + pTexVertOffset->x;
                        pDest->aTexVertices[i].y = pDest->aTexVertices[i].y + pTexVertOffset->y;
                    }

                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3GT(pFrustrum, pDest->aVertices, pDest->aTexVertices, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3GTOrtho(pFrustrum, pDest->aVertices, pDest->aTexVertices, pSrc->numVertices);
                    }

                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                            rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

                            pDest->aTexVertices[i].x = pDest->aTexVertices[i].x + pTexVertOffset->x;
                            pDest->aTexVertices[i].y = pDest->aTexVertices[i].y + pTexVertOffset->y;

                            // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }
                    else
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                            rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

                            pDest->aTexVertices[i].x = pDest->aTexVertices[i].x + pTexVertOffset->x;
                            pDest->aTexVertices[i].y = pDest->aTexVertices[i].y + pTexVertOffset->y;

                            // TODO: verify if it's a bug assigning to pDest->aVertLights[i] instead of pSrc->aVertLights[pSrc->aVertIdxs[i]]

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }

                    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
                    {
                        pDest->numVertices = rdClip_Face3T(pFrustrum, pDest->aVertices, pDest->aTexVertices, pDest->aVertLights, pSrc->numVertices);
                    }
                    else
                    {
                        pDest->numVertices = rdClip_Face3TOrtho(pFrustrum, pDest->aVertices, pDest->aTexVertices, pDest->aVertLights, pSrc->numVertices);
                    }

                    break;

                default:
                    return;
            }

            break;

        default:
            return;
    }
}

// Linker-discarded (no retail symbol, no caller even in the debug image); the debug build's text (0x516bd7):
// ClipFace's copies without the clip calls. Only the textured gouraud case sets the vertex count. Jedi Knight's name.
void J3DAPI rdPrimit3_NoClipFace(rdGeometryMode geoMode, rdLightMode lightMode, const rdPrimit3* pSrc, rdPrimit3* pDest, const rdVector2* pTexVertOffset)
{
    size_t i;

    switch ( geoMode )
    {
        case RD_GEOMETRY_VERTEX:
        case RD_GEOMETRY_WIREFRAME:
            for ( i = 0; i < pSrc->numVertices; ++i )
            {
                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
            }

            pDest->numVertices = pSrc->numVertices;
            break;

        case RD_GEOMETRY_NONE:
            for ( i = 0; i < pSrc->numVertices; ++i )
            {
                rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
            }

            pDest->numVertices = pSrc->numVertices;
            break;

        case RD_GEOMETRY_SOLID:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                    }

                    pDest->numVertices = pSrc->numVertices;
                    break;

                case RD_LIGHTING_DIFFUSE:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                    }

                    pDest->numVertices = pSrc->numVertices;
                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }
                    else
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }

                    pDest->numVertices = pSrc->numVertices;
            }

            break;

        case RD_GEOMETRY_FULL:
            switch ( lightMode )
            {
                case RD_LIGHTING_NONE:
                case RD_LIGHTING_LIT:
                case RD_LIGHTING_DIFFUSE:
                    for ( i = 0; i < pSrc->numVertices; ++i )
                    {
                        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                        rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

                        pDest->aTexVertices[i].x = pDest->aTexVertices[i].x + pTexVertOffset->x;
                        pDest->aTexVertices[i].y = pDest->aTexVertices[i].y + pTexVertOffset->y;
                    }

                    pDest->numVertices = pSrc->numVertices;
                    break;

                case RD_LIGHTING_GOURAUD:
                    if ( pSrc->aVertIntensities )
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                            rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

                            pDest->aTexVertices[i].x = pDest->aTexVertices[i].x + pTexVertOffset->x;
                            pDest->aTexVertices[i].y = pDest->aTexVertices[i].y + pTexVertOffset->y;

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red + pSrc->aVertIntensities[i].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green + pSrc->aVertIntensities[i].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue + pSrc->aVertIntensities[i].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }
                    else
                    {
                        for ( i = 0; i < pSrc->numVertices; ++i )
                        {
                            rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
                            rdVector_Copy2(&pDest->aTexVertices[i], &pSrc->aTexVertices[pSrc->aTexVertIdxs[i]]);

                            pDest->aTexVertices[i].x = pDest->aTexVertices[i].x + pTexVertOffset->x;
                            pDest->aTexVertices[i].y = pDest->aTexVertices[i].y + pTexVertOffset->y;

                            pDest->aVertLights[i].red = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].red, 0.0f, 1.0f);

                            pDest->aVertLights[i].green = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].green, 0.0f, 1.0f);

                            pDest->aVertLights[i].blue = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].blue, 0.0f, 1.0f);

                            pDest->aVertLights[i].alpha = STDMATH_CLAMP(pSrc->aVertLights[pSrc->aVertIdxs[i]].alpha, 0.0f, 1.0f);
                        }
                    }

                    pDest->numVertices = pSrc->numVertices;
                    break;

                default:
                    return;
            }

            break;

        default:
            return;
    }
}
