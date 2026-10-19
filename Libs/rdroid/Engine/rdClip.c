#include "rdClip.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdQClip.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>
#include <std/Win95/stdDisplay.h>

#include<math.h>

#define RDCLIP_ABS(x) ((x) < 0.0f ? -(x) : (x))

// Retail's .bss block is 0x014DD998-0x014DE4F8 (0xB60 bytes, align 8).
// Every object below carries an explicit zero initializer, so the block has
// no uninitialized group and VC5 lays all eleven out in declaration order,
// which is retail's order. Do not
// drop an initializer: an uninitialized static jumps into the identifier-keyed
// group that comes first. The pinned retail addresses
// are aWorkTVerts 0x014DD9A8 (stored into the dest cursor at 0x004D4071),
// pSourceVert/pSourceVertIntensity 0x014DDC28/0x014DDC2C, aWorkVerts
// 0x014DDC30, aWorkVertIntensities 0x014DDFF0 and faceStatus 0x014DE4F4;
// rdClipFace.c's aWorkFaceVerts follows at 0x014DE4F8. rdClip_faceStatus is a
// public zero-initialized definition inside the private block, not a COMMON
// request: sithRender reads it (0x004444FA). The 4-byte slot at 0x014DE4F0 is
// referenced nowhere in the image and has no alignment explanation.
static rdVector2* rdClip_pSourceTVert = NULL;
static rdVector2* rdClip_pDestTVert = NULL;
static rdVector4* rdClip_pDestVertIntensity = NULL;
static rdVector3* rdClip_pDestVert = NULL;

static rdVector2 rdClip_aWorkTVerts[MAX_CLIP_VERTICIES] = { 0 };

static rdVector3* rdClip_pSourceVert = NULL;
static rdVector4* rdClip_pSourceVertIntensity = NULL;

static rdVector3 rdClip_aWorkVerts[MAX_CLIP_VERTICIES]  = { 0 };
static rdVector4 rdClip_aWorkVertIntensities[MAX_CLIP_VERTICIES] = { 0 };

static int rdClip_unusedState0 = 0;
int rdClip_faceStatus = 0;

int J3DAPI rdClip_Line2(const rdCanvas* pCanvas, int* x1, int* y1, int* x2, int* y2)
{
    float fy2;
    float fy1;
    rdClipOutcode outcode1;
    rdClipOutcode outcode2;
    rdClipOutcode outcode;
    float fx2;
    float fx1;
    float curX;
    float curY;
    BOOL yEq;
    BOOL xEq;

    outcode1 = rdClip_CalcOutcode2(pCanvas, *x1, *y1);
    outcode2 = rdClip_CalcOutcode2(pCanvas, *x2, *y2);
    if ( !outcode1 && !outcode2 )
    {
        return 1;
    }

    if ( (outcode1 & outcode2) != 0 )
    {
        return 0;
    }

    fx1 = (float)*x1;
    fy1 = (float)*y1;

    fx2 = (float)*x2;
    fy2 = (float)*y2;

    if ( fx1 == fx2 )
    {
        xEq = 1;
    }
    else
    {
        xEq = 0;
    }

    if ( fy1 == fy2 )
    {
        yEq = 1;
    }
    else
    {
        yEq = 0;
    }

    for ( ;; )
    {
        if ( outcode1 )
        {
            outcode = outcode1;
        }
        else
        {
            outcode = outcode2;
        }

        if ( (outcode & RDCLIP_OUTCODE_TOP) != 0 )
        {
            if ( xEq )
            {
                curX = fx1;
            }
            else
            {
                curX = RD_MULTIPLY((fx2 - fx1) / (fy2 - fy1), ((float)pCanvas->rect.top - fy1)) + fx1;
            }

            curY = (float)pCanvas->rect.top;
        }

        else if ( (outcode & RDCLIP_OUTCODE_BOTTOM) != 0 )
        {
            if ( xEq )
            {
                curX = fx1;
            }
            else
            {
                curX = RD_MULTIPLY((fx2 - fx1) / (fy2 - fy1), ((float)pCanvas->rect.bottom - fy1)) + fx1;
            }

            curY = (float)pCanvas->rect.bottom;
        }

        else if ( (outcode & RDCLIP_OUTCODE_RIGHT) != 0 )
        {
            if ( yEq )
            {
                curY = fy1;
            }
            else
            {
                curY = RD_MULTIPLY((fy2 - fy1) / (fx2 - fx1), ((float)pCanvas->rect.right - fx1)) + fy1;
            }

            curX = (float)pCanvas->rect.right;
        }
        else
        {
            if ( yEq )
            {
                curY = fy1;
            }
            else
            {
                curY = RD_MULTIPLY((fy2 - fy1) / (fx2 - fx1), ((float)pCanvas->rect.left - fx1)) + fy1;
            }

            curX = (float)pCanvas->rect.left;
        }

        if ( outcode == outcode1 )
        {
            fx1 = curX;
            fy1 = curY;
            outcode1 = rdClip_CalcOutcode2(pCanvas, (int32_t)fx1, (int32_t)fy1);
        }
        else
        {
            fx2 = curX;
            fy2 = curY;
            outcode2 = rdClip_CalcOutcode2(pCanvas, (int32_t)fx2, (int32_t)fy2);
        }

        if ( !outcode1 && !outcode2 )
        {
            *x1 = (int32_t)fx1;
            *y1 = (int32_t)fy1;
            *x2 = (int32_t)fx2;
            *y2 = (int32_t)fy2;
            return 1;
        }

        if ( (outcode1 & outcode2) != 0 )
        {
            return 0;
        }
    }
}

rdClipOutcode J3DAPI rdClip_CalcOutcode2(const rdCanvas* pCanvas, int x, int y)
{
    rdClipOutcode ccode;

    ccode = 0;
    if ( x < pCanvas->rect.left )
    {
        ccode |= RDCLIP_OUTCODE_LEFT;
    }
    else if ( x > pCanvas->rect.right )
    {
        ccode |= RDCLIP_OUTCODE_RIGHT;
    }

    if ( y < pCanvas->rect.top )
    {
        ccode |= RDCLIP_OUTCODE_TOP;                  // 0x1000 - RDCLIP_OUTCODE_TOP
    }
    else if ( y > pCanvas->rect.bottom )
    {
        ccode |= RDCLIP_OUTCODE_BOTTOM;                 // 0x100 - RDCLIP_OUTCODE_BOTTOM
    }

    return ccode;
}

int J3DAPI rdClip_Line2Ex(float* pX1, float* pY1, float* pX2, float* pY2)
{
    float x1;
    float y1;
    float x;
    float y;
    float x2;
    float y2;
    float width;
    float height;
    BOOL bXEqual;
    BOOL bYEqual;
    int ccode;
    int ccode1;
    int ccode2;
    uint32_t bwidth;
    uint32_t bheight;

    stdDisplay_GetBackBufferSize(&bwidth, &bheight);
    width = (float)bwidth;
    height = (float)bheight;

    ccode1 = rdClip_CalcOutcode2Ex(*pX1, *pY1, width, height);
    ccode2 = rdClip_CalcOutcode2Ex(*pX2, *pY2, width, height);

    if ( !ccode1 && !ccode2 )
    {
        return 1;
    }

    if ( (ccode1 & ccode2) != 0 )
    {
        return 0;
    }

    x1 = *pX1;
    y1 = *pY1;

    x2 = *pX2;
    y2 = *pY2;

    if ( x1 == x2 )
    {
        bXEqual = 1;
    }
    else
    {
        bXEqual = 0;
    }

    if ( y1 == y2 )
    {
        bYEqual = 1;
    }
    else
    {
        bYEqual = 0;
    }

    for ( ;; )
    {
        if ( ccode1 )
        {
            ccode = ccode1;
        }
        else
        {
            ccode = ccode2;
        }

        if ( (ccode & RDCLIP_OUTCODE_TOP) != 0 )
        {
            if ( bXEqual )
            {
                x = x1;
            }
            else
            {
                x = RD_MULTIPLY((x2 - x1) / (y2 - y1), (0.0f - y1)) + x1;
            }

            y = 0.0f;
        }

        else if ( (ccode & RDCLIP_OUTCODE_BOTTOM) != 0 )
        {
            if ( bXEqual )
            {
                x = x1;
            }
            else
            {
                x = RD_MULTIPLY((x2 - x1) / (y2 - y1), (height - y1)) + x1;
            }

            y = height;
        }

        else if ( (ccode & RDCLIP_OUTCODE_RIGHT) != 0 )
        {
            if ( bYEqual )
            {
                y = y1;
            }
            else
            {
                y = RD_MULTIPLY((y2 - y1) / (x2 - x1), (width - x1)) + y1;
            }

            x = width;
        }
        else
        {
            if ( bYEqual )
            {
                y = y1;
            }
            else
            {
                y = RD_MULTIPLY((y2 - y1) / (x2 - x1), (0.0f - x1)) + y1;
            }

            x = 0.0f;
        }

        if ( ccode == ccode1 )
        {
            x1 = x;
            y1 = y;
            ccode1 = rdClip_CalcOutcode2Ex(x1, y1, width, height);
        }
        else
        {
            x2 = x;
            y2 = y;
            ccode2 = rdClip_CalcOutcode2Ex(x2, y2, width, height);
        }

        if ( !ccode1 && !ccode2 )
        {
            *pX1 = x1;
            *pY1 = y1;
            *pX2 = x2;
            *pY2 = y2;
            return 1;
        }

        if ( (ccode1 & ccode2) != 0 )
        {
            return 0;
        }
    }
}

int J3DAPI rdClip_CalcOutcode2Ex(float x, float y, float width, float height)
{
    int ccode;

    ccode = 0;
    if ( x < 0.0f )
    {
        ccode |= RDCLIP_OUTCODE_LEFT;
    }
    else if ( x > width )
    {
        ccode |= RDCLIP_OUTCODE_RIGHT;
    }

    if ( y < 0.0f )
    {
        ccode |= RDCLIP_OUTCODE_TOP;                  // 0x1000 - RDCLIP_OUTCODE_TOP
    }
    else if ( y > height )
    {
        ccode |= RDCLIP_OUTCODE_BOTTOM;                 // 0x100 - RDCLIP_OUTCODE_BOTTOM
    }

    return ccode;
}


// Linker-discarded (no retail symbol or caller): retail's first __LINE__ anchor in this file is
// 1270 against ~440 without them, and Jedi Knight's rdClip.c has exactly these four 3D clippers
// between CalcOutcode2 and the face clippers. Bodies restored from the JK -> MotS -> Indy lineage
// JK.EXE and JKM.EXE carry the same instruction streams (relocation-masked,
// call targets by role), and this text reproduces JK's bytes.
int J3DAPI rdClip_Point3(const rdClipFrustum* pFrustum, const rdVector3* pPoint)
{
    float xPlane;
    float zPlane;

    if ( pPoint->y < pFrustum->nearPlane )
    {
        return 0;
    }

    if ( pFrustum->bClipFar && pPoint->y > pFrustum->farPlane )
    {
        return 0;
    }

    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        xPlane = RD_MULTIPLY(pPoint->y, pFrustum->leftPlane);
    }
    else
    {
        xPlane = pFrustum->orthoLeftPlane;
    }

    if ( pPoint->x < xPlane )
    {
        return 0;
    }

    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        xPlane = RD_MULTIPLY(pPoint->y, pFrustum->rightPlane);
    }
    else
    {
        xPlane = pFrustum->orthoRightPlane;
    }

    if ( pPoint->x > xPlane )
    {
        return 0;
    }

    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        zPlane = RD_MULTIPLY(pPoint->y, pFrustum->topPlane);
    }
    else
    {
        zPlane = pFrustum->orthoTopPlane;
    }

    if ( pPoint->z > zPlane )
    {
        return 0;
    }

    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        zPlane = RD_MULTIPLY(pPoint->y, pFrustum->bottomPlane);
    }
    else
    {
        zPlane = pFrustum->orthoBottomPlane;
    }

    if ( pPoint->z < zPlane )
    {
        return 0;
    }

    return 1;
}

int J3DAPI rdClip_Line3Project(const rdClipFrustum* pClipFrustum, rdVector3* point1, rdVector3* point2, int* out1, int* out2)
{
    float t;
    float xplane1;
    float xplane2;
    float zplane1;
    float zplane2;
    float numer;
    float planeProj;
    float clipX;
    float clipY;
    float clipZ;

    if ( point1->y < pClipFrustum->nearPlane && point2->y < pClipFrustum->nearPlane )
    {
        return 0;
    }

    if ( point1->y < pClipFrustum->nearPlane )
    {
        t = (pClipFrustum->nearPlane - point1->y) / (point2->y - point1->y);
        point1->y = pClipFrustum->nearPlane;
        point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->y < pClipFrustum->nearPlane )
    {
        t = (pClipFrustum->nearPlane - point1->y) / (point2->y - point1->y);
        point2->y = pClipFrustum->nearPlane;
        point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    if ( pClipFrustum->bClipFar )
    {
        if ( point1->y > pClipFrustum->farPlane && point2->y > pClipFrustum->farPlane )
        {
            return 0;
        }

        if ( point1->y > pClipFrustum->farPlane )
        {
            t = (pClipFrustum->farPlane - point1->y) / (point2->y - point1->y);
            point1->y = pClipFrustum->farPlane;
            point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
            point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
            if ( out1 )
            {
                *out1 = 1;
            }
        }
        else if ( point2->y > pClipFrustum->farPlane )
        {
            t = (pClipFrustum->farPlane - point1->y) / (point2->y - point1->y);
            point2->y = pClipFrustum->farPlane;
            point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
            point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
            if ( out2 )
            {
                *out2 = 1;
            }
        }
    }

    xplane1 = RD_MULTIPLY(point1->y, pClipFrustum->leftPlane);
    xplane2 = RD_MULTIPLY(point2->y, pClipFrustum->leftPlane);
    if ( point1->x < xplane1 && point2->x < xplane2 )
    {
        return 0;
    }

    if ( point1->x < xplane1 )
    {
        numer = RD_MULTIPLY(point2->y, point1->x) - RD_MULTIPLY(point1->y, point2->x);
        planeProj = RD_MULTIPLY(pClipFrustum->leftPlane, point2->y - point1->y) - (point2->x - point1->x);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipX = RD_MULTIPLY(pClipFrustum->leftPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->x - point1->x) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipX - point1->x) / (point2->x - point1->x);
        }

        point1->x = clipX;
        point1->y = clipY;
        point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->x < xplane2 )
    {
        numer = RD_MULTIPLY(point2->y, point1->x) - RD_MULTIPLY(point1->y, point2->x);
        planeProj = RD_MULTIPLY(pClipFrustum->leftPlane, point2->y - point1->y) - (point2->x - point1->x);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipX = RD_MULTIPLY(pClipFrustum->leftPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->x - point1->x) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipX - point1->x) / (point2->x - point1->x);
        }

        point2->x = clipX;
        point2->y = clipY;
        point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    xplane1 = RD_MULTIPLY(point1->y, pClipFrustum->rightPlane);
    xplane2 = RD_MULTIPLY(point2->y, pClipFrustum->rightPlane);
    if ( point1->x > xplane1 && point2->x > xplane2 )
    {
        return 0;
    }

    if ( point1->x > xplane1 )
    {
        numer = RD_MULTIPLY(point2->y, point1->x) - RD_MULTIPLY(point1->y, point2->x);
        planeProj = RD_MULTIPLY(pClipFrustum->rightPlane, point2->y - point1->y) - (point2->x - point1->x);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipX = RD_MULTIPLY(pClipFrustum->rightPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->x - point1->x) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipX - point1->x) / (point2->x - point1->x);
        }

        point1->x = clipX;
        point1->y = clipY;
        point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->x > xplane2 )
    {
        numer = RD_MULTIPLY(point2->y, point1->x) - RD_MULTIPLY(point1->y, point2->x);
        planeProj = RD_MULTIPLY(pClipFrustum->rightPlane, point2->y - point1->y) - (point2->x - point1->x);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipX = RD_MULTIPLY(pClipFrustum->rightPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->x - point1->x) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipX - point1->x) / (point2->x - point1->x);
        }

        point2->x = clipX;
        point2->y = clipY;
        point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    zplane1 = RD_MULTIPLY(point1->y, pClipFrustum->topPlane);
    zplane2 = RD_MULTIPLY(point2->y, pClipFrustum->topPlane);
    if ( point1->z > zplane1 && point2->z > zplane2 )
    {
        return 0;
    }

    if ( point1->z > zplane1 )
    {
        numer = RD_MULTIPLY(point1->z, point2->y) - RD_MULTIPLY(point2->z, point1->y);
        planeProj = RD_MULTIPLY(pClipFrustum->topPlane, point2->y - point1->y) - (point2->z - point1->z);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipZ = RD_MULTIPLY(pClipFrustum->topPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->z - point1->z) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipZ - point1->z) / (point2->z - point1->z);
        }

        point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        point1->y = clipY;
        point1->z = clipZ;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->z > zplane2 )
    {
        numer = RD_MULTIPLY(point1->z, point2->y) - RD_MULTIPLY(point2->z, point1->y);
        planeProj = RD_MULTIPLY(pClipFrustum->topPlane, point2->y - point1->y) - (point2->z - point1->z);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipZ = RD_MULTIPLY(pClipFrustum->topPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->z - point1->z) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipZ - point1->z) / (point2->z - point1->z);
        }

        point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        point2->y = clipY;
        point2->z = clipZ;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    zplane1 = RD_MULTIPLY(point1->y, pClipFrustum->bottomPlane);
    zplane2 = RD_MULTIPLY(point2->y, pClipFrustum->bottomPlane);
    if ( point1->z < zplane1 && point2->z < zplane2 )
    {
        return 0;
    }

    if ( point1->z < zplane1 )
    {
        numer = RD_MULTIPLY(point1->z, point2->y) - RD_MULTIPLY(point2->z, point1->y);
        planeProj = RD_MULTIPLY(pClipFrustum->bottomPlane, point2->y - point1->y) - (point2->z - point1->z);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipZ = RD_MULTIPLY(pClipFrustum->bottomPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->z - point1->z) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipZ - point1->z) / (point2->z - point1->z);
        }

        point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        point1->y = clipY;
        point1->z = clipZ;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->z < zplane2 )
    {
        numer = RD_MULTIPLY(point1->z, point2->y) - RD_MULTIPLY(point2->z, point1->y);
        planeProj = RD_MULTIPLY(pClipFrustum->bottomPlane, point2->y - point1->y) - (point2->z - point1->z);
        if ( planeProj != 0.0f )
        {
            clipY = numer / planeProj;
        }
        else
        {
            clipY = numer;
        }

        clipZ = RD_MULTIPLY(pClipFrustum->bottomPlane, clipY);
        if ( RDCLIP_ABS(point2->y - point1->y) > RDCLIP_ABS(point2->z - point1->z) )
        {
            t = (clipY - point1->y) / (point2->y - point1->y);
        }
        else
        {
            t = (clipZ - point1->z) / (point2->z - point1->z);
        }

        point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        point2->y = clipY;
        point2->z = clipZ;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    return 1;
}

int J3DAPI rdClip_Line3Ortho(const rdClipFrustum* pClipFrustum, rdVector3* point1, rdVector3* point2, int* out1, int* out2)
{
    float t;
    float xplane1;
    float xplane2;
    float vertPlane1;
    float vertPlane2;

    if ( point1->y < pClipFrustum->nearPlane && point2->y < pClipFrustum->nearPlane )
    {
        return 0;
    }

    if ( point1->y < pClipFrustum->nearPlane )
    {
        t = (pClipFrustum->nearPlane - point1->y) / (point2->y - point1->y);
        point1->y = pClipFrustum->nearPlane;
        point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->y < pClipFrustum->nearPlane )
    {
        t = (pClipFrustum->nearPlane - point1->y) / (point2->y - point1->y);
        point2->y = pClipFrustum->nearPlane;
        point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    if ( pClipFrustum->bClipFar )
    {
        if ( point1->y > pClipFrustum->farPlane && point2->y > pClipFrustum->farPlane )
        {
            return 0;
        }

        if ( point1->y > pClipFrustum->farPlane )
        {
            t = (pClipFrustum->farPlane - point1->y) / (point2->y - point1->y);
            point1->y = pClipFrustum->farPlane;
            point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
            point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
            if ( out1 )
            {
                *out1 = 1;
            }
        }
        else if ( point2->y > pClipFrustum->farPlane )
        {
            t = (pClipFrustum->farPlane - point1->y) / (point2->y - point1->y);
            point2->y = pClipFrustum->farPlane;
            point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
            point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
            if ( out2 )
            {
                *out2 = 1;
            }
        }
    }

    xplane1 = pClipFrustum->orthoLeftPlane;
    xplane2 = pClipFrustum->orthoLeftPlane;
    if ( point1->x < xplane1 && point2->x < xplane2 )
    {
        return 0;
    }

    if ( point1->x < xplane1 )
    {
        t = (xplane1 - point1->x) / (point2->x - point1->x);
        point1->x = xplane1;
        point1->y = RD_MULTIPLY(point2->y - point1->y, t) + point1->y;
        point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->x < xplane2 )
    {
        t = (xplane2 - point2->x) / (point2->x - point1->x);
        point2->x = xplane2;
        point2->y = RD_MULTIPLY(point2->y - point1->y, t) + point2->y;
        point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point2->z;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    xplane1 = pClipFrustum->orthoRightPlane;
    xplane2 = pClipFrustum->orthoRightPlane;
    if ( point1->x > xplane1 && point2->x > xplane2 )
    {
        return 0;
    }

    if ( point1->x > xplane1 )
    {
        t = (xplane1 - point1->x) / (point2->x - point1->x);
        point1->x = xplane1;
        point1->y = RD_MULTIPLY(point2->y - point1->y, t) + point1->y;
        point1->z = RD_MULTIPLY(point2->z - point1->z, t) + point1->z;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->x > xplane2 )
    {
        t = (xplane2 - point2->x) / (point2->x - point1->x);
        point2->x = xplane2;
        point2->y = RD_MULTIPLY(point2->y - point1->y, t) + point2->y;
        point2->z = RD_MULTIPLY(point2->z - point1->z, t) + point2->z;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    vertPlane1 = pClipFrustum->orthoTopPlane;
    vertPlane2 = pClipFrustum->orthoTopPlane;
    if ( point1->z > vertPlane1 && point2->z > vertPlane2 )
    {
        return 0;
    }

    if ( point1->z > vertPlane1 )
    {
        t = (vertPlane1 - point1->z) / (point2->z - point1->z);
        point1->z = vertPlane1;
        point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        point1->y = RD_MULTIPLY(point2->y - point1->y, t) + point1->y;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->z > vertPlane2 )
    {
        t = (vertPlane2 - point2->z) / (point2->z - point1->z);
        point2->z = vertPlane2;
        point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point2->x;
        point2->y = RD_MULTIPLY(point2->y - point1->y, t) + point2->y;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    vertPlane1 = pClipFrustum->orthoBottomPlane;
    vertPlane2 = pClipFrustum->orthoBottomPlane;
    if ( point1->z < vertPlane1 && point2->z < vertPlane2 )
    {
        return 0;
    }

    if ( point1->z < vertPlane1 )
    {
        t = (vertPlane1 - point1->z) / (point2->z - point1->z);
        point1->z = vertPlane1;
        point1->x = RD_MULTIPLY(point2->x - point1->x, t) + point1->x;
        point1->y = RD_MULTIPLY(point2->y - point1->y, t) + point1->y;
        if ( out1 )
        {
            *out1 = 1;
        }
    }
    else if ( point2->z < vertPlane2 )
    {
        t = (vertPlane2 - point2->z) / (point2->z - point1->z);
        point2->z = vertPlane2;
        point2->x = RD_MULTIPLY(point2->x - point1->x, t) + point2->x;
        point2->y = RD_MULTIPLY(point2->y - point1->y, t) + point2->y;
        if ( out2 )
        {
            *out2 = 1;
        }
    }

    return 1;
}

int J3DAPI rdClip_Line3(const rdClipFrustum* pClipFrustum, rdVector3* point1, rdVector3* point2, rdScreenPoint* pScreenPt1, rdScreenPoint* pScreenPt2, int* out1, int* out2)
{
    int bVisible;
    int x1;
    int y1;
    int x2;
    int y2;
    rdVector3 vertex1;
    rdVector3 vertex2;
    rdVector3 project1;
    rdVector3 project2;

    rdMatrix_TransformPoint34(&vertex1, point1, &rdCamera_pCurCamera->orient);
    rdMatrix_TransformPoint34(&vertex2, point2, &rdCamera_pCurCamera->orient);
    if ( rdCamera_pCurCamera->projectType == RDCAMERA_PROJECT_PERSPECTIVE )
    {
        bVisible = rdClip_Line3Project(pClipFrustum, &vertex1, &vertex2, out1, out2);
    }
    else
    {
        bVisible = rdClip_Line3Ortho(pClipFrustum, &vertex1, &vertex2, out1, out2);
    }

    if ( bVisible )
    {
        rdCamera_pCurCamera->pfProject(&project1, &vertex1);
        rdCamera_pCurCamera->pfProject(&project2, &vertex2);

        x1 = (int)project1.x;
        y1 = (int)project1.y;
        if ( pScreenPt1 )
        {
            pScreenPt1->x = x1;
            pScreenPt1->y = y1;
            pScreenPt1->z = project1.z;
        }

        x2 = (int)project2.x;
        y2 = (int)project2.y;
        if ( pScreenPt2 )
        {
            pScreenPt2->x = x2;
            pScreenPt2->y = y2;
            pScreenPt2->z = project2.z;
        }

        return 1;
    }

    return 0;
}

void J3DAPI rdClip_ClipFacePVS(rdClipFrustum* pFrustrum, const rdPrimit3* pSrc, rdPrimit3* pDest)
{
    size_t i;
    for ( i = 0; i < pSrc->numVertices; ++i )
    {
        rdVector_Copy3(&pDest->aVertices[i], &pSrc->aVertices[pSrc->aVertIdxs[i]]);
    }

    pDest->numVertices = rdClip_Face3WPVS(pFrustrum, pDest->aVertices, pSrc->numVertices);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(rdClip_countfit_1, 13) /* end count-fit */
int J3DAPI rdClip_Face3WPVS(rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    rdVector3* pCurVertex;
    float numer;
    float planeProj;
    float outX;
    float outY;
    float outZ;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    // Do left plane clipping
    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex  = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->leftPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->leftPlane);
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->leftPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->leftPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x   = outX;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1270
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do right plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->rightPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->rightPlane);
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->rightPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->rightPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x   = outX;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1339
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do top plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->topPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->topPlane);
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->topPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->topPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = outZ;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1408
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do bottom plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->bottomPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->bottomPlane);
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->bottomPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->bottomPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = outZ;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1476
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do near plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y   = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1528
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x41;
        return clipindex;
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
    }

    return clipindex;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(rdClip_countfit_2, 26) /* end count-fit */
int J3DAPI rdClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    rdVector3* pCurVertex;
    float numer;
    float planeProj;
    float outX;
    float outY;
    float outZ;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    // Do left plane clipping
    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex  = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->leftPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->leftPlane);
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->leftPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->leftPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x   = outX;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1632
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do right plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->rightPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->rightPlane);
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->rightPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->rightPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x   = outX;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1701
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do top plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->topPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->topPlane);
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->topPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->topPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = outZ;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1770
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do bottom plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->bottomPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->bottomPlane);
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->bottomPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->bottomPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = outZ;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1838
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do near plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y   = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 1890
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y   = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    clipindex++;
                }
            }

#line 1947
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3WOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    rdVector3* pCurVertex;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    // Do left plane clipping
    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex  = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoLeftPlane;
        xCurProj = pFrustrum->orthoLeftPlane;
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x   = prevProjX;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2040
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do right plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoRightPlane;
        xCurProj = pFrustrum->orthoRightPlane;
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x   = prevProjX;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2097
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do top plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoTopPlane;
        zCurrentPos = pFrustrum->orthoTopPlane;
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = zPrevPos;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2154
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do bottom plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoBottomPlane;
        zCurrentPos = pFrustrum->orthoBottomPlane;
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = zPrevPos;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2210
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do near plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y   = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2262
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y   = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    clipindex++;
                }
            }

#line 2319
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3S(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    rdVector3* pCurVertex;
    float numer;
    float planeProj;
    float outX;
    float outY;
    float outZ;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    // Do left plane clipping
    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex  = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->leftPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->leftPlane);
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->leftPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->leftPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x   = outX;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2425
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do right plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->rightPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->rightPlane);
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->rightPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->rightPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x   = outX;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2494
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do top plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->topPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->topPlane);
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->topPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->topPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = outZ;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2563
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do bottom plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->bottomPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->bottomPlane);
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->bottomPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->bottomPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = outY;
                rdClip_pDestVert[clipindex++].z = outZ;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2632
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do near plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y   = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2684
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y   = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    clipindex++;
                }
            }

#line 2741
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3SOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    rdVector3* pCurVertex;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    // Do left plane clipping
    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert   = rdClip_aWorkVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex  = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoLeftPlane;
        xCurProj = pFrustrum->orthoLeftPlane;
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x   = prevProjX;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2834
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do right plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoRightPlane;
        xCurProj = pFrustrum->orthoRightPlane;
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x   = prevProjX;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2891
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do top plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoTopPlane;
        zCurrentPos = pFrustrum->orthoTopPlane;
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = zPrevPos;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 2948
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do bottom plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoBottomPlane;
        zCurrentPos = pFrustrum->orthoBottomPlane;
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x   = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y   = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex++].z = zPrevPos;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 3005
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    // Do near plane clipping
    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y   = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                clipindex++;
            }
        }

#line 3057
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y   = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z   = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    clipindex++;
                }
            }

#line 3114
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
    }

    return clipindex;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(rdClip_countfit_3, 30) CF_PROTO(rdClip_countfit_4, 6) /* end count-fit */
int J3DAPI rdClip_Face3GS(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector4* aIntensities, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    int i;
    rdVector4* pPrevRgb;
    rdVector3* pCurVertex;
    float numer;
    float planeProj;
    float outX;
    float outY;
    float outZ;
    rdVector4* pCurRgb;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVert = rdClip_aWorkVerts;
    rdClip_pDestVertIntensity = rdClip_aWorkVertIntensities;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->leftPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->leftPlane);
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->leftPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->leftPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x = outX;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3236
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->rightPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->rightPlane);
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->rightPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->rightPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x = outX;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3318
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->topPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->topPlane);
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->topPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->topPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = outZ;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3402
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->bottomPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->bottomPlane);
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->bottomPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->bottomPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = outZ;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3487
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3552
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        {
            rdVector4* pTemp = rdClip_pDestVertIntensity;
            rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
            rdClip_pSourceVertIntensity = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;
        pCurRgb = rdClip_pSourceVertIntensity;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                    rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                    rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                    rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                    clipindex++;
                }
            }

#line 3623
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
            pPrevRgb = pCurRgb;
            pCurRgb++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(*aVertices) * clipindex);
        memcpy(aIntensities, rdClip_pDestVertIntensity, sizeof(*aIntensities) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3GSOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector4* aIntensities, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector3* pPrevVert;
    float lerpFactor;
    int i;
    rdVector4* pPrevRgb;
    rdVector3* pCurVertex;
    rdVector4* pCurRgb;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVert = rdClip_aWorkVerts;
    rdClip_pDestVertIntensity = rdClip_aWorkVertIntensities;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoLeftPlane;
        xCurProj = pFrustrum->orthoLeftPlane;
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x = prevProjX;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3739
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoRightPlane;
        xCurProj = pFrustrum->orthoRightPlane;
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x = prevProjX;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3808
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoTopPlane;
        zCurrentPos = pFrustrum->orthoTopPlane;
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = zPrevPos;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3877
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoBottomPlane;
        zCurrentPos = pFrustrum->orthoBottomPlane;
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = zPrevPos;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 3946
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 4011
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        {
            rdVector4* pTemp = rdClip_pDestVertIntensity;
            rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
            rdClip_pSourceVertIntensity = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;
        pCurRgb = rdClip_pSourceVertIntensity;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                    rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                    rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                    rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                    clipindex++;
                }
            }

#line 4080
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
            pPrevRgb = pCurRgb;
            pCurRgb++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
        memcpy(aIntensities, rdClip_pDestVertIntensity, sizeof(rdVector4) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3GT(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector2* pUVLastTexVerts;
    float lerpFactor;
    rdVector2* pUVCurrentTexVerts;
    rdVector3* pPrevVert;
    rdVector3* pCurVertex;
    float numer;
    float planeProj;
    float outX;
    float outY;
    float outZ;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert = rdClip_aWorkVerts;
    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert = rdClip_aWorkTVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->leftPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->leftPlane);
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->leftPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->leftPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x = outX;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4203
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->rightPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->rightPlane);
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->rightPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->rightPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x = outX;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4283
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->topPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->topPlane);
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->topPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->topPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = outZ;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4363
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->bottomPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->bottomPlane);
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->bottomPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->bottomPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = outZ;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4444
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4507
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        {
            rdVector2* pTemp = rdClip_pDestTVert;
            rdClip_pDestTVert = rdClip_pSourceTVert;
            rdClip_pSourceTVert = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;
        pUVCurrentTexVerts = rdClip_pSourceTVert;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                    rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                    rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                    clipindex++;
                }
            }

#line 4575
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
            pUVLastTexVerts = pUVCurrentTexVerts;
            pUVCurrentTexVerts++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(*aVertices) * clipindex);
        memcpy(aTexVertices, rdClip_pDestTVert, sizeof(*aTexVertices) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3GTOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector2* pUVLastTexVerts;
    float lerpFactor;
    rdVector2* pUVCurrentTexVerts;
    rdVector3* pPrevVert;
    rdVector3* pCurVertex;
    int i;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert = rdClip_aWorkVerts;
    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert = rdClip_aWorkTVerts;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoLeftPlane;
        xCurProj = pFrustrum->orthoLeftPlane;
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x = prevProjX;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4686
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoRightPlane;
        xCurProj = pFrustrum->orthoRightPlane;
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x = prevProjX;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4754
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoTopPlane;
        zCurrentPos = pFrustrum->orthoTopPlane;
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = zPrevPos;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4822
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoBottomPlane;
        zCurrentPos = pFrustrum->orthoBottomPlane;
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = zPrevPos;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4891
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                clipindex++;
            }
        }

#line 4954
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        {
            rdVector2* pTemp = rdClip_pDestTVert;
            rdClip_pDestTVert = rdClip_pSourceTVert;
            rdClip_pSourceTVert = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;
        pUVCurrentTexVerts = rdClip_pSourceTVert;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                    rdClip_pDestTVert[clipindex].x   = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                    rdClip_pDestTVert[clipindex++].y   = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                    clipindex++;
                }
            }

#line 5022
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
            pUVLastTexVerts = pUVCurrentTexVerts;
            pUVCurrentTexVerts++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
        memcpy(aTexVertices, rdClip_pDestTVert, sizeof(*aTexVertices) * clipindex);
    }

    return clipindex;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(rdClip_countfit_5, 5) /* end count-fit */
int J3DAPI rdClip_Face3T(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector2* pUVLastTexVerts;
    float lerpFactor;
    rdVector2* pUVCurrentTexVerts;
    rdVector3* pPrevVert;
    int i;
    rdVector4* pPrevRgb;
    rdVector3* pCurVertex;
    float numer;
    float planeProj;
    float outX;
    float outY;
    float outZ;
    rdVector4* pCurRgb;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert = rdClip_aWorkVerts;
    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert = rdClip_aWorkTVerts;
    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVertIntensity = rdClip_aWorkVertIntensities;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->leftPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->leftPlane);
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->leftPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->leftPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x = outX;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY(pCurRgb->x - pPrevRgb->x, lerpFactor) + pPrevRgb->x; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY(pCurRgb->y - pPrevRgb->y, lerpFactor) + pPrevRgb->y; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY(pCurRgb->z - pPrevRgb->z, lerpFactor) + pPrevRgb->z; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY(pCurRgb->w - pPrevRgb->w, lerpFactor) + pPrevRgb->w; /* spelling-fit */

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5159
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = RD_MULTIPLY(pPrevVert->y, pFrustrum->rightPlane);
        xCurProj = RD_MULTIPLY(pCurVertex->y, pFrustrum->rightPlane);
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                numer = RD_MULTIPLY(pCurVertex->y, pPrevVert->x) - RD_MULTIPLY(pPrevVert->y, pCurVertex->x);
                planeProj = RD_MULTIPLY(pFrustrum->rightPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->x - pPrevVert->x);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outX = RD_MULTIPLY(pFrustrum->rightPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->x - pPrevVert->x) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                }

                rdClip_pDestVert[clipindex].x = outX;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY(pCurRgb->x - pPrevRgb->x, lerpFactor) + pPrevRgb->x; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY(pCurRgb->y - pPrevRgb->y, lerpFactor) + pPrevRgb->y; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY(pCurRgb->z - pPrevRgb->z, lerpFactor) + pPrevRgb->z; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY(pCurRgb->w - pPrevRgb->w, lerpFactor) + pPrevRgb->w; /* spelling-fit */

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5251
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->topPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->topPlane);
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->topPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->topPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = outZ;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY(pCurRgb->x - pPrevRgb->x, lerpFactor) + pPrevRgb->x; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY(pCurRgb->y - pPrevRgb->y, lerpFactor) + pPrevRgb->y; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY(pCurRgb->z - pPrevRgb->z, lerpFactor) + pPrevRgb->z; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY(pCurRgb->w - pPrevRgb->w, lerpFactor) + pPrevRgb->w; /* spelling-fit */

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5343
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = RD_MULTIPLY(pPrevVert->y, pFrustrum->bottomPlane);
        zCurrentPos = RD_MULTIPLY(pCurVertex->y, pFrustrum->bottomPlane);
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                numer = RD_MULTIPLY(pPrevVert->z, pCurVertex->y) - RD_MULTIPLY(pCurVertex->z, pPrevVert->y);
                planeProj = RD_MULTIPLY(pFrustrum->bottomPlane, pCurVertex->y - pPrevVert->y) - (pCurVertex->z - pPrevVert->z);
                if ( planeProj != 0.0f )
                {
                    outY = numer / planeProj;
                }
                else
                {
                    outY = numer;
                }

                outZ = RD_MULTIPLY(pFrustrum->bottomPlane, outY);
                if ( RDCLIP_ABS(pCurVertex->y - pPrevVert->y) > RDCLIP_ABS(pCurVertex->z - pPrevVert->z) )
                {
                    lerpFactor = D3DVAL((outY - pPrevVert->y) / (pCurVertex->y - pPrevVert->y));
                }
                else
                {
                    lerpFactor = D3DVAL((outZ - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                }

                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = outY;
                rdClip_pDestVert[clipindex].z = outZ;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY(pCurRgb->x - pPrevRgb->x, lerpFactor) + pPrevRgb->x; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY(pCurRgb->y - pPrevRgb->y, lerpFactor) + pPrevRgb->y; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY(pCurRgb->z - pPrevRgb->z, lerpFactor) + pPrevRgb->z; /* spelling-fit */
                rdClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY(pCurRgb->w - pPrevRgb->w, lerpFactor) + pPrevRgb->w; /* spelling-fit */

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5435
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;
                rdClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY((pCurRgb->w - pPrevRgb->w), lerpFactor) + pPrevRgb->w;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5510
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        {
            rdVector2* pTemp = rdClip_pDestTVert;
            rdClip_pDestTVert = rdClip_pSourceTVert;
            rdClip_pSourceTVert = pTemp;
        }

        {
            rdVector4* pTemp = rdClip_pDestVertIntensity;
            rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
            rdClip_pSourceVertIntensity = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
        pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;
        pUVCurrentTexVerts = rdClip_pSourceTVert;
        pCurRgb = rdClip_pSourceVertIntensity;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                    rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                    rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                    rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                    rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                    rdClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;
                    rdClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY((pCurRgb->w - pPrevRgb->w), lerpFactor) + pPrevRgb->w;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                    rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                    clipindex++;
                }
            }

#line 5590
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
            pUVLastTexVerts = pUVCurrentTexVerts;
            pUVCurrentTexVerts++;
            pPrevRgb = pCurRgb;
            pCurRgb++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(*aVertices) * clipindex);
        memcpy(aTexVertices, rdClip_pDestTVert, sizeof(*aTexVertices) * clipindex);
        memcpy(aIntensities, rdClip_pDestVertIntensity, sizeof(*aIntensities) * clipindex);
    }

    return clipindex;
}

int J3DAPI rdClip_Face3TOrtho(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, int numVertices)
{
    float prevProjX;
    int clipindex;
    rdVector2* pUVLastTexVerts;
    float lerpFactor;
    rdVector2* pUVCurrentTexVerts;
    rdVector3* pPrevVert;
    int i;
    rdVector4* pPrevRgb;
    rdVector3* pCurVertex;
    rdVector4* pCurRgb;
    float xCurProj;
    float zPrevPos;
    float zCurrentPos;

    rdClip_faceStatus = 0;
    rdClip_pSourceVert = aVertices;
    rdClip_pDestVert = rdClip_aWorkVerts;
    rdClip_pSourceTVert = aTexVertices;
    rdClip_pDestTVert = rdClip_aWorkTVerts;
    rdClip_pSourceVertIntensity = aIntensities;
    rdClip_pDestVertIntensity = rdClip_aWorkVertIntensities;

    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoLeftPlane;
        xCurProj = pFrustrum->orthoLeftPlane;
        if ( pPrevVert->x >= prevProjX || pCurVertex->x >= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x < prevProjX || pCurVertex->x < xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x = prevProjX;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x10;
            }

            if ( pCurVertex->x >= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5717
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        prevProjX = pFrustrum->orthoRightPlane;
        xCurProj = pFrustrum->orthoRightPlane;
        if ( pPrevVert->x <= prevProjX || pCurVertex->x <= xCurProj )
        {
            if ( pPrevVert->x == prevProjX || pCurVertex->x == xCurProj )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->x > prevProjX || pCurVertex->x > xCurProj )
            {
                lerpFactor = D3DVAL((prevProjX - pPrevVert->x) / (pCurVertex->x - pPrevVert->x));
                rdClip_pDestVert[clipindex].x = prevProjX;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x20;
            }

            if ( pCurVertex->x <= xCurProj )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5796
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoTopPlane;
        zCurrentPos = pFrustrum->orthoTopPlane;
        if ( pPrevVert->z <= zPrevPos || pCurVertex->z <= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z > zPrevPos || pCurVertex->z > zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = zPrevPos;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x04;
            }

            if ( pCurVertex->z <= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5875
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        zPrevPos = pFrustrum->orthoBottomPlane;
        zCurrentPos = pFrustrum->orthoBottomPlane;
        if ( pPrevVert->z >= zPrevPos || pCurVertex->z >= zCurrentPos )
        {
            if ( pPrevVert->z == zPrevPos || pCurVertex->z == zCurrentPos )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->z < zPrevPos || pCurVertex->z < zCurrentPos )
            {
                lerpFactor = D3DVAL((zPrevPos - pPrevVert->z) / (pCurVertex->z - pPrevVert->z));
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestVert[clipindex].y = RD_MULTIPLY((pCurVertex->y - pPrevVert->y), lerpFactor) + pPrevVert->y;
                rdClip_pDestVert[clipindex].z = zPrevPos;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x08;
            }

            if ( pCurVertex->z >= zCurrentPos )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 5954
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        return clipindex;
    }

    {
        rdVector3* pTemp = rdClip_pDestVert;
        rdClip_pDestVert = rdClip_pSourceVert;
        rdClip_pSourceVert = pTemp;
    }

    {
        rdVector2* pTemp = rdClip_pDestTVert;
        rdClip_pDestTVert = rdClip_pSourceTVert;
        rdClip_pSourceTVert = pTemp;
    }

    {
        rdVector4* pTemp = rdClip_pDestVertIntensity;
        rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
        rdClip_pSourceVertIntensity = pTemp;
    }

    numVertices = clipindex;
    clipindex = 0;
    pPrevVert = &rdClip_pSourceVert[numVertices - 1];
    pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
    pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
    pCurVertex = rdClip_pSourceVert;
    pUVCurrentTexVerts = rdClip_pSourceTVert;
    pCurRgb = rdClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVertex->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVertex->y == pFrustrum->nearPlane )
            {
                // An endpoint on the plane: nothing to clip.
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVertex->y < pFrustrum->nearPlane )
            {
                lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVertex->y);
                rdClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdClip_pDestVert[clipindex].z = D3DVAL(RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z); /* spelling-fit */
                rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->x), lerpFactor) + pPrevRgb->y;
                rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->x), lerpFactor) + pPrevRgb->z;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVertex->y >= pFrustrum->nearPlane )
            {
                rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                clipindex++;
            }
        }

#line 6028
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVertex;
        pCurVertex++;
        pUVLastTexVerts = pUVCurrentTexVerts;
        pUVCurrentTexVerts++;
        pPrevRgb = pCurRgb;
        pCurRgb++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40;
        return clipindex;
    }

    if ( pFrustrum->bClipFar )
    {
        {
            rdVector3* pTemp = rdClip_pDestVert;
            rdClip_pDestVert = rdClip_pSourceVert;
            rdClip_pSourceVert = pTemp;
        }

        {
            rdVector2* pTemp = rdClip_pDestTVert;
            rdClip_pDestTVert = rdClip_pSourceTVert;
            rdClip_pSourceTVert = pTemp;
        }

        {
            rdVector4* pTemp = rdClip_pDestVertIntensity;
            rdClip_pDestVertIntensity = rdClip_pSourceVertIntensity;
            rdClip_pSourceVertIntensity = pTemp;
        }

        numVertices = clipindex;
        clipindex = 0;
        pPrevVert = &rdClip_pSourceVert[numVertices - 1];
        pUVLastTexVerts = &rdClip_pSourceTVert[numVertices - 1];
        pPrevRgb = &rdClip_pSourceVertIntensity[numVertices - 1];
        pCurVertex = rdClip_pSourceVert;
        pUVCurrentTexVerts = rdClip_pSourceTVert;
        pCurRgb = rdClip_pSourceVertIntensity;

        for ( i = 0; i < numVertices; ++i )
        {
            if ( pPrevVert->y <= pFrustrum->farPlane || pCurVertex->y <= pFrustrum->farPlane )
            {
                if ( pPrevVert->y == pFrustrum->farPlane || pCurVertex->y == pFrustrum->farPlane )
                {
                    // An endpoint on the plane: nothing to clip.
                }
                else if ( pPrevVert->y > pFrustrum->farPlane || pCurVertex->y > pFrustrum->farPlane )
                {
                    lerpFactor = RDCLIP_LERPFACTOR(pFrustrum->farPlane, pPrevVert->y, pCurVertex->y);
                    rdClip_pDestVert[clipindex].y = pFrustrum->farPlane;
                    rdClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVertex->z - pPrevVert->z), lerpFactor) + pPrevVert->z;
                    rdClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVertex->x - pPrevVert->x), lerpFactor) + pPrevVert->x;
                    rdClip_pDestTVert[clipindex].x = RD_MULTIPLY((pUVCurrentTexVerts->x - pUVLastTexVerts->x), lerpFactor) + pUVLastTexVerts->x;
                    rdClip_pDestTVert[clipindex].y = RD_MULTIPLY((pUVCurrentTexVerts->y - pUVLastTexVerts->y), lerpFactor) + pUVLastTexVerts->y;
                    rdClip_pDestVertIntensity[clipindex].x   = RD_MULTIPLY((pCurRgb->x - pPrevRgb->x), lerpFactor) + pPrevRgb->x;
                    rdClip_pDestVertIntensity[clipindex].y   = RD_MULTIPLY((pCurRgb->y - pPrevRgb->y), lerpFactor) + pPrevRgb->y;
                    rdClip_pDestVertIntensity[clipindex++].z = RD_MULTIPLY((pCurRgb->z - pPrevRgb->z), lerpFactor) + pPrevRgb->z;

                    rdClip_faceStatus |= 0x02;
                }

                if ( pCurVertex->y <= pFrustrum->farPlane )
                {
                    rdVector_Copy3(&rdClip_pDestVert[clipindex], pCurVertex);
                    rdVector_Copy2(&rdClip_pDestTVert[clipindex], pUVCurrentTexVerts);
                    rdVector_Copy4(&rdClip_pDestVertIntensity[clipindex], pCurRgb);
                    clipindex++;
                }
            }

#line 6107
            RD_ASSERTREL(clipindex<80);
            pPrevVert = pCurVertex;
            pCurVertex++;
            pUVLastTexVerts = pUVCurrentTexVerts;
            pUVCurrentTexVerts++;
            pPrevRgb = pCurRgb;
            pCurRgb++;
        }

        if ( clipindex < 3 )
        {
            return clipindex;
        }
    }

    if ( rdClip_pDestVert != aVertices )
    {
        memcpy(aVertices, rdClip_pDestVert, sizeof(rdVector3) * clipindex);
        memcpy(aTexVertices, rdClip_pDestTVert, sizeof(*aTexVertices) * clipindex);
        memcpy(aIntensities, rdClip_pDestVertIntensity, sizeof(*aIntensities) * clipindex);
    }

    return clipindex;
}

RdFrustumCull J3DAPI rdClip_SphereInFrustrum(const rdClipFrustum* pFrustrum, const rdVector3* pCenter, float radius)
{
    rdVector3 tmpNormal;
    float planeDist;

    int bInside = 1;
    float yHigh;
    float yLow;

    // Retail zeroes a 12-byte stack aggregate here (three `mov [esp+n],eax` stores
    // and the `sub esp,0xc` frame at 0x004D7E60) and never reads it again.
    // The debug build calls memset first, before both distances (0x526946).
    memset(&tmpNormal, 0, sizeof(tmpNormal));

    yHigh = pCenter->y + radius;
    yLow = pCenter->y - radius;

    if ( yHigh < pFrustrum->nearPlane )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( yLow < pFrustrum->nearPlane )
    {
        bInside = 0;
    }

    if ( pFrustrum->bClipFar )
    {
        if ( yLow > pFrustrum->farPlane )
        {
            return RDFRUSTUMCULL_OUTSIDE;
        }

        if ( yHigh > pFrustrum->farPlane )
        {
            bInside = 0;
        }
    }

    planeDist = RDVECTOR_ROTATE2_Y(pFrustrum->topPlaneNormal.y, pFrustrum->topPlaneNormal.z, pCenter->y, pCenter->z);
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( planeDist > -radius )
    {
        bInside = 0;
    }

    planeDist = RDVECTOR_ROTATE2_Y(pFrustrum->bottomPlaneNormal.y, pFrustrum->bottomPlaneNormal.z, pCenter->y, pCenter->z);
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( planeDist > -radius )
    {
        bInside = 0;
    }

    planeDist = RDVECTOR_ROTATE2_Y(pFrustrum->leftPlaneNormal.y, pFrustrum->leftPlaneNormal.x, pCenter->y, pCenter->x);
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( planeDist > -radius )
    {
        bInside = 0;
    }

    planeDist = RDVECTOR_ROTATE2_Y(pFrustrum->rightPlaneNormal.y, pFrustrum->rightPlaneNormal.x, pCenter->y, pCenter->x);
    if ( planeDist > radius )
    {
        return RDFRUSTUMCULL_OUTSIDE;
    }

    if ( planeDist > -radius )
    {
        bInside = 0;
    }

    if ( bInside )
    {
        return RDFRUSTUMCULL_INSIDE;
    }

    return RDFRUSTUMCULL_INTERSECT;
}
