#include "rdPrimit2.h"

#include <rdroid/Engine/rdClip.h>
#include <rdroid/Main/rdroid.h>
#include <std/General/stdMath.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

static D3DTLVERTEX rdPrimit2_aD3DVertices[2];

// Linker-discarded: the debug build's text (0x00511EB0, before DrawClippedLine), called only by the discarded
// rdPrimit3_DrawClippedLine: DrawClippedLine's raster loop without the clip. Name from Jones3DRE.
void J3DAPI rdPrimit2_DrawLine(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern)
{
    int x;
    int y;
    int dx;
    int dy;
    int xIncrement;
    int yIncrement;
    int error;
    uint32_t mask;
    int xError;
    int yError;
    int bVBLocked = 0;

    if ( pCanvas->pVBuffer->rasterInfo.colorInfo.bpp == 8 )
    {

        mask = 0x80000000;
        dx = x2 - x1;
        dy = y2 - y1;
        x = x1;
        y = y1;
        if ( dx > 0 )
        {
            xIncrement = 1;
        }
        else
        {
            xIncrement = -1;
        }

        if ( dy > 0 )
        {
            yIncrement = 1;
        }
        else
        {
            yIncrement = -1;
        }

        // Adjust for Bresenham algorithm
        if ( xIncrement < 0 )
        {
            dy = -dy;
        }

        if ( yIncrement > 0 )
        {
            dx = -dx;
        }

        // Draw first pixel if needed
        if ( (pattern & mask) != 0 )
        {
            *(pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowSize) + x) = color;
        }

        // Draw the line
        error = 0;
        while ( x != x2 || y != y2 )
        {

            // Rotate pattern mask
            mask >>= 1;
            if ( !mask ) mask = 0x80000000;

            // Determine next pixel
            xError = error + dy;
            yError = error + dx;
            if ( abs(xError) < abs(yError) )
            {
                x += xIncrement;
                error = xError;
            }
            else
            {
                y += yIncrement;
                error = yError;
            }

            // Draw if pattern bit is set
            if ( (pattern & mask) != 0 )
            {
                *(pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowSize) + x) = color;
            }
        }
    }
    else
    {

        // Lock buffer if needed
        if ( pCanvas->pVBuffer->lockRefCount == 1 )
        {
            bVBLocked = 1;
            stdDisplay_VBufferLock(pCanvas->pVBuffer);
        }

        mask = 0x80000000;
        dx = x2 - x1;
        dy = y2 - y1;
        x = x1;
        y = y1;
        if ( dx > 0 )
        {
            xIncrement = 1;
        }
        else
        {
            xIncrement = -1;
        }

        if ( dy > 0 )
        {
            yIncrement = 1;
        }
        else
        {
            yIncrement = -1;
        }

        // Adjust for Bresenham algorithm
        if ( xIncrement < 0 )
        {
            dy = -dy;
        }

        if ( yIncrement > 0 )
        {
            dx = -dx;
        }

        // Draw first pixel if needed
        if ( (pattern & mask) != 0 )
        {
            *((uint16_t*)pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowWidth) + x) = (uint16_t)color;
        }

        // Draw the line
        error = 0;
        while ( x != x2 || y != y2 )
        {

            // Rotate pattern mask
            mask >>= 1;
            if ( mask == 0 ) mask = 0x80000000;

            // Determine next pixel
            xError = error + dy;
            yError = error + dx;
            if ( abs(xError) < abs(yError) )
            {
                x += xIncrement;
                error = xError;
            }
            else
            {
                y += yIncrement;
                error = yError;
            }

            // Draw if pattern bit is set
            if ( (pattern & mask) != 0 )
            {
                *((uint16_t*)pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowWidth) + x) = (uint16_t)color;
            }
        }

        // Unlock buffer if we locked it
        if ( bVBLocked )
        {
            stdDisplay_VBufferUnlock(pCanvas->pVBuffer);
        }
    }
}

int J3DAPI rdPrimit2_DrawClippedLine(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern)
{
    int x;
    int y;
    int dx;
    int dy;
    int xIncrement;
    int yIncrement;
    int error;
    uint32_t mask;
    int xError;
    int yError;
    int bVBLocked = 0;

    if ( rdClip_Line2(pCanvas, &x1, &y1, &x2, &y2) )
    {
        if ( pCanvas->pVBuffer->rasterInfo.colorInfo.bpp == 8 )
        {

            mask = 0x80000000;
            dx = x2 - x1;
            dy = y2 - y1;
            x = x1;
            y = y1;
            if ( dx > 0 )
            {
                xIncrement = 1;
            }
            else
            {
                xIncrement = -1;
            }

            if ( dy > 0 )
            {
                yIncrement = 1;
            }
            else
            {
                yIncrement = -1;
            }

            // Adjust for Bresenham algorithm
            if ( xIncrement < 0 )
            {
                dy = -dy;
            }

            if ( yIncrement > 0 )
            {
                dx = -dx;
            }

            // Draw first pixel if needed
            if ( (pattern & mask) != 0 )
            {
                *(pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowSize) + x) = color;
            }

            // Draw the line
            error = 0;
            while ( x != x2 || y != y2 )
            {

                // Rotate pattern mask
                mask >>= 1;
                if ( !mask ) mask = 0x80000000;

                // Determine next pixel
                xError = error + dy;
                yError = error + dx;
                if ( abs(xError) < abs(yError) )
                {
                    x += xIncrement;
                    error = xError;
                }
                else
                {
                    y += yIncrement;
                    error = yError;
                }

                // Draw if pattern bit is set
                if ( (pattern & mask) != 0 )
                {
                    *(pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowSize) + x) = color;
                }
            }
        }
        else
        {

            // Lock buffer if needed
            if ( pCanvas->pVBuffer->lockRefCount == 1 )
            {
                bVBLocked = 1;
                stdDisplay_VBufferLock(pCanvas->pVBuffer);
            }

            mask = 0x80000000;
            dx = x2 - x1;
            dy = y2 - y1;
            x = x1;
            y = y1;
            if ( dx > 0 )
            {
                xIncrement = 1;
            }
            else
            {
                xIncrement = -1;
            }

            if ( dy > 0 )
            {
                yIncrement = 1;
            }
            else
            {
                yIncrement = -1;
            }

            // Adjust for Bresenham algorithm
            if ( xIncrement < 0 )
            {
                dy = -dy;
            }

            if ( yIncrement > 0 )
            {
                dx = -dx;
            }

            // Draw first pixel if needed
            if ( (pattern & mask) != 0 )
            {
                *((uint16_t*)pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowWidth) + x) = (uint16_t)color;
            }

            // Draw the line
            error = 0;
            while ( x != x2 || y != y2 )
            {

                // Rotate pattern mask
                mask >>= 1;
                if ( mask == 0 ) mask = 0x80000000;

                // Determine next pixel
                xError = error + dy;
                yError = error + dx;
                if ( abs(xError) < abs(yError) )
                {
                    x += xIncrement;
                    error = xError;
                }
                else
                {
                    y += yIncrement;
                    error = yError;
                }

                // Draw if pattern bit is set
                if ( (pattern & mask) != 0 )
                {
                    *((uint16_t*)pCanvas->pVBuffer->pPixels + (y * pCanvas->pVBuffer->rasterInfo.rowWidth) + x) = (uint16_t)color;
                }
            }

            // Unlock buffer if we locked it
            if ( bVBLocked )
            {
                stdDisplay_VBufferUnlock(pCanvas->pVBuffer);
            }
        }

        return 1;
    }

    return 0;
}

void J3DAPI rdPrimit2_Reset(void)
{
    std3D_SetWireframeRenderState();
    memset(rdPrimit2_aD3DVertices, 0, sizeof(rdPrimit2_aD3DVertices));
}

int J3DAPI rdPrimit2_DrawClippedLine2(float x1, float y1, float x2, float y2, uint32_t color)
{
    if ( rdClip_Line2Ex(&x1, &y1, &x2, &y2) )
    {
        rdPrimit2_aD3DVertices[0].sx    = D3DVAL(x1);
        rdPrimit2_aD3DVertices[0].sy    = D3DVAL(y1);
        rdPrimit2_aD3DVertices[0].color = color;

        rdPrimit2_aD3DVertices[1].sx    = D3DVAL(x2);
        rdPrimit2_aD3DVertices[1].sy    = D3DVAL(y2);
        rdPrimit2_aD3DVertices[1].color = color;

        std3D_DrawLineStrip(rdPrimit2_aD3DVertices, 2u);
        return 1;
    }

    return 0;
}

void J3DAPI rdPrimit2_DrawClippedCircle(rdCanvas* pCanvas, int x, int y, float radius, float step, uint32_t color, uint32_t pattern)
{
    float arcDeg;
    int iRadius;
    int y2;
    int pointX;
    float cosTheta;
    int y1;
    int x1;
    float sinval;

    iRadius = (int32_t)(radius + 0.5f);
    if ( x + iRadius < pCanvas->rect.left || x - iRadius > pCanvas->rect.right )
    {
        return;
    }

    if ( y + iRadius < pCanvas->rect.top || y - iRadius > pCanvas->rect.bottom )
    {
        return;
    }

    arcDeg = 0.0f;
    stdMath_SinCos(arcDeg, &sinval, &cosTheta);
    x1 = (int32_t)(RD_MULTIPLY(radius, cosTheta) + 0.5f) + x;
    y1 = (int32_t)(RD_MULTIPLY(radius, sinval) + 0.5f) + y;
    arcDeg = arcDeg + step;
    while ( arcDeg <= 360.0f )
    {
        stdMath_SinCos(arcDeg, &sinval, &cosTheta);
        pointX = (int32_t)(RD_MULTIPLY(radius, cosTheta) + 0.5f) + x;
        y2 = (int32_t)(RD_MULTIPLY(radius, sinval) + 0.5f) + y;

        rdPrimit2_DrawClippedLine(pCanvas, x1, y1, pointX, y2, color, pattern);

        x1 = pointX;
        y1 = y2;
        arcDeg = arcDeg + step;
    }
}

void J3DAPI rdPrimit2_DrawClippedCircle2(float centerX, float centerY, float radius, float degrees, uint32_t color)
{
    float arcDeg = 0.0f;
    float pointX;
    float y2;
    float x1;
    float cosTheta, sinval;

    float y1;
    stdMath_SinCos(arcDeg, &sinval, &cosTheta);

    x1 = RD_MULTIPLY(radius, cosTheta) + centerX;
    y1 = RD_MULTIPLY(radius, sinval) + centerY;

    arcDeg = arcDeg + degrees;
    while ( arcDeg <= 360.0f )
    {
        stdMath_SinCos(arcDeg, &sinval, &cosTheta);

        pointX = RD_MULTIPLY(radius, cosTheta) + centerX;
        y2 = RD_MULTIPLY(radius, sinval) + centerY;
        rdPrimit2_DrawClippedLine2(x1, y1, pointX, y2, color);

        x1 = pointX;
        y1 = y2;
        arcDeg = arcDeg + degrees;
    }
}


/* Linker-discarded (no callers in either build): the debug build has them after DrawClippedCircle2 (0x00512745, 0x005127DA),
   an outline of four resp. three DrawClippedLine calls. Names from Jones3DRE. */
void J3DAPI rdPrimit2_DrawRectangle(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, uint32_t color, uint32_t pattern)
{
    rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x2, y1, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x2, y1, x2, y2, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x1, y2, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x1, y2, x2, y2, color, pattern);
}

void J3DAPI rdPrimit2_DrawTriangle(rdCanvas* pCanvas, int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color, uint32_t pattern)
{
    rdPrimit2_DrawClippedLine(pCanvas, x1, y1, x2, y2, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x2, y2, x3, y3, color, pattern);
    rdPrimit2_DrawClippedLine(pCanvas, x3, y3, x1, y1, color, pattern);
}
