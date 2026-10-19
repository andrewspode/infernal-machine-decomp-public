#include "rdWallpaper.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/stdMemory.h>
#include <std/Win95/stdDisplay.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define RDWALLPAPER_NUMMATS        6u
#define RDWALLPAPER_LINE_THICKNESS 6.0f

static const rdVector4 rdWallpaper_extraLight = { 1.0f, 1.0f, 1.0f, 1.0f };

rdWallpaper* J3DAPI rdWallpaper_New(const char* pName)
{
    char aMatName[64];
    size_t i;
    rdWallpaper* wallpaper;

#line 51
    wallpaper = (rdWallpaper*)STDMALLOC(sizeof(rdWallpaper));
    if ( !wallpaper )
    {
#line 54
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for wallpaper.\n", 0, 0, 0, 0);
        goto error;
    }

    memset(wallpaper, 0, sizeof(rdWallpaper));
    strncpy(wallpaper->aName, pName, 63);
    wallpaper->aName[63] = 0;
    wallpaper->pCamera = rdCamera_New(90.0f, 0, 0.001f, 100.0f, 1.0f);
    if ( !wallpaper ) // Note, the debug build tests pWallpaper here (not pCamera)
    {
        goto error;
    }

    for ( i = 0; i < RDWALLPAPER_NUMMATS; ++i )
    {
        sprintf(aMatName, "%s_%1d.mat", wallpaper->aName, i + 1);
        if ( rdMaterial_LoadEntry(aMatName, &wallpaper->aMaterials[i]) )
        {
            goto error;
        }
    }

    return wallpaper;

error:
    if ( wallpaper )
    {
        rdWallpaper_Free(wallpaper);
    }

    return NULL;
}

void J3DAPI rdWallpaper_Free(rdWallpaper* pWallpaper)
{
    size_t i;

    if ( pWallpaper )
    {
        rdCamera_Free(pWallpaper->pCamera);
        for ( i = 0; i < RDWALLPAPER_NUMMATS; ++i ) {
            rdMaterial_FreeEntry(&pWallpaper->aMaterials[i]);
        }
        stdMemory_Free(pWallpaper);
    }
}

void J3DAPI rdWallpaper_Draw(const rdWallpaper* pWallpaper)
{
    float swidth;
    float sheight;
    size_t matNum;

    uint32_t height, width;
    stdDisplay_GetBackBufferSize(&width, &height);
    swidth = D3DVAL((float)width / RD_REF_WIDTH);
    sheight = D3DVAL(D3DVAL((float)height) / RD_REF_HEIGHT); /* spelling-fit */

    rdCamera_SetCurrent(pWallpaper->pCamera);

    for ( matNum = 0; matNum < RDWALLPAPER_NUMMATS; ++matNum )
    {
        LPD3DTLVERTEX pCurVert;

        rdCacheProcEntry* pPoly = rdCache_GetProcEntry();
        if ( !pPoly )
        {
            // Error getting proc entry
            return;
        }

        pPoly->flags        = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X;
        pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
        pPoly->numVertices  = 4;
        rdVector_Copy4(&pPoly->extraLight, &rdWallpaper_extraLight);

        {
            size_t i = 0;  // block scope: the debug build's frame slot order follows it
            for ( ; i < pPoly->numVertices; ++i ) {
            rdVector_Copy4(&pPoly->aVertIntensities[i], &rdWallpaper_extraLight);
        }
        }

        pCurVert = pPoly->aVertices;
        switch ( matNum )
        {
            case 0u: // left top
                pCurVert->sx  = D3DVAL(0.0f);
                pCurVert->sy  = D3DVAL(0.0f);
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5); // 0.00015259022
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(0.0f);
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(1.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(0.0f);
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(1.0f);
                break;

            case 1u: // top middle
                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(0.0f);
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(0.0f);
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(1.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(1.0f);
                break;

            case 2u: // top right
                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(0.0f);
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(640.0f, swidth));
                pCurVert->sy  = D3DVAL(0.0f);
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(640.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(1.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(1.0f);
                break;

            case 3u:// bottom left
                pCurVert->sx  = D3DVAL(0.0f);
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(480.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.875f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(0.0f);
                pCurVert->sy  = D3DVAL(D3DMultiply(480.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.875f); // (480 - 256) / 256
                break;

            case 4u: // bottom middle
                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(480.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.875f); // (480 - 256) / 256
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(256.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(480.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.875f); // (480 - 256) / 256
                break;

            case 5u: // bottom right
                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(640.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(256.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.0f);
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(640.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(480.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(1.0f);
                pCurVert->tv  = D3DVAL(0.875f); // (480 - 256) / 256
                ++pCurVert;

                pCurVert->sx  = D3DVAL(D3DMultiply(512.0f, swidth));
                pCurVert->sy  = D3DVAL(D3DMultiply(480.0f, sheight));
                pCurVert->sz  = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X5);
                pCurVert->tu  = D3DVAL(0.0f);
                pCurVert->tv  = D3DVAL(0.875f); // (480 - 256) / 256
                break;

            default:
                break;
        }

        pPoly->pMaterial = &pWallpaper->aMaterials[matNum];
        pPoly->matCelNum = pWallpaper->aMatCelNums[matNum];
        rdCache_AddProcFace(4u);
    }
}

rdWallLine* J3DAPI rdWallpaper_NewWallLine(float startX, float startY, float endX, float endY, const rdVector4* pColor)
{
    rdWallLine* pWallLine;

#line 317
    pWallLine = (rdWallLine*)STDMALLOC(sizeof(rdWallLine));
    if ( !pWallLine )
    {
#line 319
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for WallLine.\n", 0, 0, 0, 0);
    }
    else
    {
        memset(pWallLine, 0, sizeof(rdWallLine));
    }

    if ( !pWallLine->pMaterial )
    {
#line 325
        pWallLine->pMaterial = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
        if ( !pWallLine->pMaterial )
        {
            goto error;
        }

        memset(pWallLine->pMaterial, 0, sizeof(rdMaterial));
        if ( rdMaterial_LoadEntry("mat\\gen_4red.mat", pWallLine->pMaterial) )
        {
            goto error;
        }
    }

    pWallLine->startX = startX;
    pWallLine->startY = startY;
    pWallLine->deltaX = endX - startX;
    pWallLine->deltaY = endY - startY;
    rdVector_Copy4(&pWallLine->color, pColor);
    return pWallLine;

error:
    if ( pWallLine )
    {
        if ( pWallLine->pMaterial )
        {
            rdMaterial_FreeEntry(pWallLine->pMaterial);
            stdMemory_Free(pWallLine->pMaterial);
            pWallLine->pMaterial = NULL;
        }

        stdMemory_Free(pWallLine);
    }

    return NULL;
}

void J3DAPI rdWallpaper_FreeWallLine(rdWallLine* pWallLine)
{
    if ( pWallLine )
    {
        if ( pWallLine->pMaterial )
        {
            rdMaterial_FreeEntry(pWallLine->pMaterial);
            stdMemory_Free(pWallLine->pMaterial);
            pWallLine->pMaterial = NULL;
        }

        stdMemory_Free(pWallLine);
    }
}

void J3DAPI rdWallpaper_DrawWallLine(const rdWallLine* pLine, float progress)
{
    uint32_t width, height;
    float swidth;
    float sheight;
    float z;
    float thickness;
    float endX;
    float endY;
    float lineAspect;
    float lineOffset;
    float deltaRatio;
    float offsetY;
    float a1;  // quadratic terms a1..c2 and twoA each hold a slot in the debug frame
    float b1;
    float c1;
    float a2;
    float b2;
    float c2;
    float discriminant;
    float twoA;
    float root1;
    float root2;
    float err1;
    float err2;
    float a;
    float b;
    float c;
    float root;
    float y;
    float deltaX;
    float deltaY;
    size_t i;
    rdCacheProcEntry* pPoly;
    LPD3DTLVERTEX pCurVert;

    stdDisplay_GetBackBufferSize(&width, &height);
    swidth = D3DVAL((float)width / RD_REF_WIDTH);
    sheight = D3DVAL((float)height / RD_REF_HEIGHT);
    z = 0.0f;
    thickness = RDWALLPAPER_LINE_THICKNESS * swidth;

    pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return;
    }

    pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->numVertices  = 4;
    rdVector_Copy4(&pPoly->extraLight, &pLine->color);
    for ( i = 0; i < pPoly->numVertices; ++i )
    {
        rdVector_Copy4(&pPoly->aVertIntensities[i], &pLine->color);
    }

    endX = ceilf(pLine->deltaX * progress / 100.0f);
    if ( endX == 0.0f )
    {
        lineAspect = 1.0f;
        lineOffset = pLine->startY;
        endY = pLine->deltaY * progress / 100.0f;
    }
    else
    {
        lineAspect = pLine->deltaY / pLine->deltaX;
        lineOffset = pLine->startY - lineAspect * pLine->startX;
        endY = ((pLine->startX + endX) * lineAspect) + lineOffset - pLine->startY;
    }

    deltaRatio = pLine->deltaX / pLine->deltaY;
    offsetY = pLine->startY - deltaRatio * pLine->startX;

    a1 = 1.0f;
    b1 = 2.0f * a1 * -pLine->startX;
    c1 = pLine->startX * pLine->startX;

    a2 = deltaRatio * deltaRatio;
    b2 = 2.0f * deltaRatio * (offsetY - pLine->startY);
    c2 = (offsetY - pLine->startY) * (offsetY - pLine->startY);

    a = a1 + a2;
    b = b1 + b2;
    c = c1 + c2 - (thickness * thickness);

    discriminant = ((float)sqrt(b * b - 4.0f * a * c));
    if ( discriminant < 0.0f )
    {
        return;
    }

    twoA = 2.0f * a;
    root1 = (-b + discriminant) / twoA;
    root2 = (-b - discriminant) / twoA;
    err1 = (a * root1 * root1) + (b * root1) + c;
    err2 = (a * root2 * root2) + (b * root2) + c;
    if ( err1 < 0.001f )
    {
        root = root1;
    }
    else if ( err2 < 0.001f )
    {
        root = root2;
    }
    else
    {
        return;
    }

    y = (deltaRatio * root) + offsetY;
    deltaX = D3DVAL(root - pLine->startX);
    deltaY = y - pLine->startY;

    pCurVert = pPoly->aVertices;

    pCurVert->sx  = D3DVAL(pLine->startX * swidth + deltaX);
    pCurVert->sy  = D3DVAL(pLine->startY * sheight - deltaY);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(0.00001f);
    pCurVert->tu  = D3DVAL(1.0f);
    pCurVert->tv  = D3DVAL(0.0f);
    ++pCurVert;

    pCurVert->sx  = D3DVAL(D3DMultiply((pLine->startX + endX), swidth) + deltaX);
    pCurVert->sy  = D3DVAL(D3DMultiply((pLine->startY + endY), sheight) - deltaY);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(0.00001f);
    pCurVert->tu  = D3DVAL(1.0f);
    pCurVert->tv  = D3DVAL(1.0f);
    ++pCurVert;

    pCurVert->sx  = D3DVAL(D3DMultiply((pLine->startX + endX), swidth) - deltaX);
    pCurVert->sy  = D3DVAL(D3DMultiply((pLine->startY + endY), sheight) + deltaY);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(0.00001f);
    pCurVert->tu  = D3DVAL(0.0f);
    pCurVert->tv  = D3DVAL(1.0f);
    ++pCurVert;

    pCurVert->sx  = D3DVAL(pLine->startX * swidth - deltaX);
    pCurVert->sy  = D3DVAL(pLine->startY * sheight + deltaY);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(0.00001f);
    pCurVert->tu  = D3DVAL(0.0f);
    pCurVert->tv  = D3DVAL(0.0f);

    pPoly->pMaterial = pLine->pMaterial;
    pPoly->matCelNum = 0;
    rdCache_AddAlphaProcFace(4u);
}
