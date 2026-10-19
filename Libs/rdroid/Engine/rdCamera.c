#include "rdCamera.h"

#include <rdroid/Engine/rdLight.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>


rdVector3 rdCamera_camPYR     = { 0 };
rdCamera* rdCamera_pCurCamera = NULL;
int rdCamera_bOverlayDepthRange        = 0;
rdMatrix34 rdCamera_camMatrix;

int J3DAPI rdCamera_BuildFOV(rdCamera* pCamera);
int J3DAPI rdCamera_BuildClipFrustrum(const rdCamera* pCamera, rdClipFrustum* pFrustum, float width, float height);

rdCamera* J3DAPI rdCamera_New(float fov, int bFarClip, float nearPlane, float farPlane, float aspectRatio)
{
    rdCamera* pCamera;

#line 73
    pCamera = (rdCamera*)STDMALLOC(sizeof(rdCamera));
    if ( !pCamera )
    {
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for camera.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( rdCamera_NewEntry(pCamera, fov, bFarClip, nearPlane, farPlane, aspectRatio) )
    {
        return pCamera;
    }

    return NULL;
}

int J3DAPI rdCamera_NewEntry(rdCamera* pCamera, float fov, int bClipFar, float nearPlane, float farPlane, float aspectRatio)
{
#line 113
    pCamera->pFrustum = (rdClipFrustum*)STDMALLOC(sizeof(rdClipFrustum));
    if ( !pCamera->pFrustum )
    {
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for camera clip frustrum.\n", 0, 0, 0, 0);
        return 0;
    }

    pCamera->fov = STDMATH_CLAMP(fov, RDCAMERA_FOVMIN, RDCAMERA_FOVMAX);

    pCamera->pFrustum->bClipFar  = bClipFar;
    pCamera->pFrustum->nearPlane = nearPlane;
    pCamera->pFrustum->farPlane  = farPlane;

    pCamera->aspectRatio      = aspectRatio;
    pCamera->invNearClipPlane = 0.0f;
    pCamera->invFarClipPlane  = 0.0f;
    pCamera->orthoScale       = 1.0f;
    pCamera->pCanvas          = NULL;

    pCamera->numLights      = 0;
    pCamera->attenuationMin = 0.2f;
    pCamera->attenuationMax = 0.1f;

    memset(&pCamera->pFrustum->leftPlaneNormal, 0, sizeof(pCamera->pFrustum->leftPlaneNormal));
    memset(&pCamera->pFrustum->rightPlaneNormal, 0, sizeof(pCamera->pFrustum->rightPlaneNormal));
    memset(&pCamera->pFrustum->topPlaneNormal, 0, sizeof(pCamera->pFrustum->topPlaneNormal));
    memset(&pCamera->pFrustum->bottomPlaneNormal, 0, sizeof(pCamera->pFrustum->bottomPlaneNormal));

    rdCamera_SetProjectType(pCamera, RDCAMERA_PROJECT_PERSPECTIVE);
    return 1;
}

void J3DAPI rdCamera_Free(rdCamera* pCamera)
{
    if ( pCamera )
    {
        rdCamera_FreeEntry(pCamera);
        stdMemory_Free(pCamera);
    }
    else
    {
#line 174
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: attempt to free NULL camera ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdCamera_FreeEntry(rdCamera* pCamera)
{
    if ( pCamera->pFrustum )
    {
        stdMemory_Free(pCamera->pFrustum);
    }
}

int J3DAPI rdCamera_SetCanvas(rdCamera* pCamera, rdCanvas* pCanvas)
{
#line 223
    RD_ASSERTREL(pCamera!=NULL);
    RD_ASSERTREL(pCanvas!=NULL);

    pCamera->pCanvas = pCanvas;
    rdCamera_BuildFOV(pCamera);
    return 1;
}

void J3DAPI rdCamera_SetCurrent(rdCamera* pCamera)
{
    if ( pCamera )
    {
        if ( rdCamera_pCurCamera != pCamera )
        {
            rdCamera_pCurCamera = pCamera;
        }
    }
}

int J3DAPI rdCamera_SetFOV(rdCamera* pCamera, float fov)
{
    pCamera->fov = STDMATH_CLAMP(fov, RDCAMERA_FOVMIN, RDCAMERA_FOVMAX);
    rdCamera_BuildFOV(pCamera);
    return 1;
}

int J3DAPI rdCamera_SetProjectType(rdCamera* pCamera, rdCameraProjType type)
{
    pCamera->projectType = type;
    switch ( type )
    {
        case RDCAMERA_PROJECT_ORTHO:
            if ( pCamera->aspectRatio == 1.0f )
            {
                pCamera->pfProject     = rdCamera_OrthoProjectSquare;
                pCamera->pfProjectList = rdCamera_OrthoProjectSquareLst;
            }
            else
            {
                pCamera->pfProject     = rdCamera_OrthoProject;
                pCamera->pfProjectList = rdCamera_OrthoProjectLst;
            }
            break;

        case RDCAMERA_PROJECT_PERSPECTIVE:
            if ( pCamera->aspectRatio == 1.0f )
            {
                pCamera->pfProject     = rdCamera_PerspProjectSquare;
                pCamera->pfProjectList = rdCamera_PerspProjectSquareLst;
            }
            else
            {
                pCamera->pfProject     = rdCamera_PerspProject;
                pCamera->pfProjectList = rdCamera_PerspProjectLst;
            }
            break;
    }

    if ( pCamera->pCanvas )
    {
        rdCamera_BuildFOV(pCamera);
    }

    return 1;
}

// Linker-discarded (no retail symbol, no caller); the debug build's text (0x505cae). Jedi Knight's name.
int J3DAPI rdCamera_SetOrthoScale(rdCamera* pCamera, float scale)
{
    pCamera->orthoScale = scale;
    rdCamera_BuildFOV(pCamera);
    return 1;
}

int J3DAPI rdCamera_SetAspectRatio(rdCamera* pCamera, float ratio)
{
    pCamera->aspectRatio = ratio;
    switch ( pCamera->projectType )
    {
        case RDCAMERA_PROJECT_ORTHO:
            if ( pCamera->aspectRatio == 1.0f )
            {
                pCamera->pfProject     = rdCamera_OrthoProjectSquare;
                pCamera->pfProjectList = rdCamera_OrthoProjectSquareLst;
            }
            else
            {
                pCamera->pfProject     = rdCamera_OrthoProject;
                pCamera->pfProjectList = rdCamera_OrthoProjectLst;
            }
            break;

        case RDCAMERA_PROJECT_PERSPECTIVE:
            if ( pCamera->aspectRatio == 1.0f )
            {
                pCamera->pfProject     = rdCamera_PerspProjectSquare;
                pCamera->pfProjectList = rdCamera_PerspProjectSquareLst;
            }
            else
            {
                pCamera->pfProject     = rdCamera_PerspProject;
                pCamera->pfProjectList = rdCamera_PerspProjectLst;
            }
    }

    rdCamera_BuildFOV(pCamera);
    return 1;
}

int J3DAPI rdCamera_BuildFOV(rdCamera* pCamera)
{
    const rdCanvas* pCanvas;
    float t;
    float hheight;
    float hwidth;

    pCanvas = pCamera->pCanvas;
    if ( !pCanvas )
    {
        return 0;
    }

    switch ( pCamera->projectType )
    {
        case RDCAMERA_PROJECT_ORTHO:
            hwidth  = D3DVAL((float)(pCanvas->rect.right - pCanvas->rect.left) / 2.0f);
            hheight = D3DVAL((float)(pCanvas->rect.bottom - pCanvas->rect.top) / 2.0f);

            pCamera->pFrustum->orthoLeftPlane   = -(hwidth / D3DVAL(pCamera->orthoScale));
            pCamera->pFrustum->orthoTopPlane    = hheight / D3DVAL(pCamera->orthoScale) / pCamera->aspectRatio;
            pCamera->pFrustum->orthoRightPlane  = hwidth / D3DVAL(pCamera->orthoScale);
            pCamera->pFrustum->orthoBottomPlane = (-(hheight / D3DVAL(pCamera->orthoScale))) / pCamera->aspectRatio;

            pCamera->focalLength = 0.0f;

            pCamera->pFrustum->topPlane    = 0.0f;
            pCamera->pFrustum->bottomPlane = 0.0f;
            pCamera->pFrustum->leftPlane   = 0.0f;
            pCamera->pFrustum->rightPlane  = 0.0f;
            break;

        case RDCAMERA_PROJECT_PERSPECTIVE:
            hwidth  = D3DVAL((float)(pCanvas->rect.right - pCanvas->rect.left) / 2.0f);
            hheight = D3DVAL((float)(pCanvas->rect.bottom - pCanvas->rect.top) / 2.0f);

            t = stdMath_Tan(pCamera->fov / 2.0f);
            pCamera->focalLength = hwidth / t;

            if ( rdCamera_bOverlayDepthRange )
            {
                pCamera->invNearClipPlane = 1.0f / (pCamera->pFrustum->nearPlane);
                pCamera->invFarClipPlane  = RD_MULTIPLY(1.0f / (1.0f / (pCamera->pFrustum->farPlane) - pCamera->invNearClipPlane), 0.1f);
            }
            else
            {
                pCamera->invNearClipPlane = 1.0f / (pCamera->pFrustum->nearPlane);
                pCamera->invFarClipPlane  = RD_MULTIPLY(1.0f / (1.0f / (pCamera->pFrustum->farPlane) - pCamera->invNearClipPlane), 0.9f);
                pCamera->invNearClipPlane = 0.1f / (0.9f * pCamera->invFarClipPlane) + pCamera->invNearClipPlane;
            }

            // Setup frustum planes
            pCamera->pFrustum->topPlane    = hheight / D3DVAL(pCamera->focalLength) / pCamera->aspectRatio;
            pCamera->pFrustum->leftPlane   = (-hwidth) / D3DVAL(pCamera->focalLength);
            pCamera->pFrustum->bottomPlane = (-hheight) / D3DVAL(pCamera->focalLength) / pCamera->aspectRatio;
            pCamera->pFrustum->rightPlane  = hwidth / D3DVAL(pCamera->focalLength);

            rdCamera_BuildClipFrustrum(pCamera, pCamera->pFrustum, RD_MULTIPLY(hwidth, 2.0f), RD_MULTIPLY(hheight, 2.0f));
            break;
    }

    return 1;
}

int J3DAPI rdCamera_BuildClipFrustrum(const rdCamera* pCamera, rdClipFrustum* pFrustum, float width, float height)
{
    float hwidth  = RD_MULTIPLY(width, 0.5f);
    float hheight = RD_MULTIPLY(height, 0.5f);

    pFrustum->leftPlaneNormal.x = -pCamera->focalLength;
    pFrustum->leftPlaneNormal.y = -hwidth;
    pFrustum->leftPlaneNormal.z = 0.0f;
    rdVector_Normalize3Acc(&pFrustum->leftPlaneNormal);

    rdVector_Copy3(&pFrustum->rightPlaneNormal, &pFrustum->leftPlaneNormal);
    pFrustum->rightPlaneNormal.x = -pFrustum->rightPlaneNormal.x;

    pFrustum->topPlaneNormal.x = 0.0f;
    pFrustum->topPlaneNormal.y = -hheight;
    pFrustum->topPlaneNormal.z = pCamera->focalLength;
    rdVector_Normalize3Acc(&pFrustum->topPlaneNormal);

    rdVector_Copy3(&pFrustum->bottomPlaneNormal, &pFrustum->topPlaneNormal);
    pFrustum->bottomPlaneNormal.z = -pFrustum->bottomPlaneNormal.z;
    return 1;
}

int J3DAPI rdCamera_SetFrustrum(rdCamera* pCamera, rdClipFrustum* pFrustrum, int left, int top, int right, int bottom)
{
    float curlsize;
    float currentbsize;
    float tsize;
    float rsize;

    const rdCanvas* pCanvas = pCamera->pCanvas;
    if ( !pCanvas )
    {
        return 0;
    }

    curlsize = pCanvas->center.x - (D3DVAL(left) - 0.5f);
    rsize = (D3DVAL(right) - 0.5f) - pCanvas->center.x; /* spelling-fit */
    tsize = pCanvas->center.y - (D3DVAL(top) - 0.5f);
    currentbsize = (D3DVAL(bottom) - 0.5f) - pCanvas->center.y; /* spelling-fit */

    pFrustrum->topPlane    =  tsize / D3DVAL(pCamera->focalLength) / D3DVAL(pCamera->aspectRatio); /* spelling-fit */
    pFrustrum->leftPlane   = (-curlsize) / D3DVAL(pCamera->focalLength);
    pFrustrum->bottomPlane = (-currentbsize) / D3DVAL(pCamera->focalLength) / pCamera->aspectRatio;
    pFrustrum->rightPlane  =  rsize / D3DVAL(pCamera->focalLength);

    pFrustrum->bClipFar    = pCamera->pFrustum->bClipFar;
    pFrustrum->nearPlane   = pCamera->pFrustum->nearPlane;
    pFrustrum->farPlane    = pCamera->pFrustum->farPlane;

    if ( rdCamera_bOverlayDepthRange )
    {
        pCamera->invNearClipPlane = 1.0f / (pCamera->pFrustum->nearPlane);
        pCamera->invFarClipPlane = RD_MULTIPLY(1.0f / (1.0f / (pCamera->pFrustum->farPlane) - pCamera->invNearClipPlane), 0.1f);
    }
    else
    {
        pCamera->invNearClipPlane = 1.0f / (pCamera->pFrustum->nearPlane);
        pCamera->invFarClipPlane = RD_MULTIPLY(1.0f / (1.0f / (pCamera->pFrustum->farPlane) - pCamera->invNearClipPlane), 0.9f);
        pCamera->invNearClipPlane = 0.1f / (0.9f * pCamera->invFarClipPlane) + pCamera->invNearClipPlane;
    }

    rdCamera_BuildClipFrustrum(pCamera, pFrustrum, (float)(pCanvas->rect.right - pCanvas->rect.left), (float)(pCanvas->rect.bottom - pCanvas->rect.top));
    return 1;
}

void J3DAPI rdCamera_Update(const rdMatrix34* orient)
{
    rdMatrix_InvertOrtho34(&rdCamera_pCurCamera->orient, orient); // Transform to view matrix (i.e.: inverse of orient)
    rdMatrix_Copy34(&rdCamera_camMatrix, orient); // Copy camera transformation matrix
    rdMatrix_ExtractAngles34(&rdCamera_camMatrix, &rdCamera_camPYR);
}

void J3DAPI rdCamera_OrthoProject(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does orthogonal projection from view space (camera space) to screen space.
    const rdCanvas* pCanvas = rdCamera_pCurCamera->pCanvas;
    pDestVertex->x = RD_MULTIPLY(rdCamera_pCurCamera->orthoScale, pSrcVertex->x) + pCanvas->center.x;
    pDestVertex->y = RD_MULTIPLY(-RD_MULTIPLY(pSrcVertex->z, rdCamera_pCurCamera->orthoScale), rdCamera_pCurCamera->aspectRatio) + pCanvas->center.y;
    pDestVertex->z = pSrcVertex->y;
}

void J3DAPI rdCamera_OrthoProjectLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    float scale;
    float yscale;
    float xCenter;
    float yCenter;

    xCenter = rdCamera_pCurCamera->pCanvas->center.x;
    yCenter = rdCamera_pCurCamera->pCanvas->center.y;
    scale   = rdCamera_pCurCamera->orthoScale;
    yscale  = RD_MULTIPLY(rdCamera_pCurCamera->aspectRatio, scale);

    while ( numVerts-- )
    {
        pDestVerts->x = RD_MULTIPLY(pSrcVerts->x, scale) + xCenter;
        pDestVerts->y = -RD_MULTIPLY(pSrcVerts->z, yscale) + yCenter;
        pDestVerts->z = pSrcVerts->y;
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_OrthoProjectSquare(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does orthogonal projection from view space (camera space) to screen space.
    const rdCanvas* pCanvas = rdCamera_pCurCamera->pCanvas;
    pDestVertex->x = RD_MULTIPLY(rdCamera_pCurCamera->orthoScale, pSrcVertex->x) + pCanvas->center.x;
    pDestVertex->y = -RD_MULTIPLY(pSrcVertex->z, rdCamera_pCurCamera->orthoScale) + pCanvas->center.y;
    pDestVertex->z = pSrcVertex->y;
}

void J3DAPI rdCamera_OrthoProjectSquareLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    float scale;
    float xCenter;
    float yCenter;

    xCenter = rdCamera_pCurCamera->pCanvas->center.x;
    yCenter = rdCamera_pCurCamera->pCanvas->center.y;
    scale   = rdCamera_pCurCamera->orthoScale;

    while ( numVerts-- )
    {
        pDestVerts->x = RD_MULTIPLY(pSrcVerts->x, scale) + xCenter;
        pDestVerts->y = -RD_MULTIPLY(pSrcVerts->z, scale) + yCenter;
        pDestVerts->z = pSrcVerts->y;
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_PerspProject(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does perspective projection from view space (camera space) to screen space.
    const rdCanvas* pCanvas = rdCamera_pCurCamera->pCanvas;
    float tz = D3DDivide(rdCamera_pCurCamera->focalLength, pSrcVertex->y);
    float curscale = RD_MULTIPLY(rdCamera_pCurCamera->aspectRatio, tz);
    pDestVertex->x = RD_MULTIPLY(pSrcVertex->x, tz) + pCanvas->center.x;
    pDestVertex->y = -RD_MULTIPLY(pSrcVertex->z, curscale) + pCanvas->center.y;
    pDestVertex->z = pSrcVertex->y;
}

void J3DAPI rdCamera_PerspProjectLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    float tz;
    float curscale;
    float centerX;
    float currentcenterY;

    centerX = rdCamera_pCurCamera->pCanvas->center.x;
    currentcenterY = rdCamera_pCurCamera->pCanvas->center.y;

    while ( numVerts-- )
    {
        tz = D3DDivide(rdCamera_pCurCamera->focalLength, pSrcVerts->y);
        curscale = RD_MULTIPLY(rdCamera_pCurCamera->aspectRatio, tz);
        pDestVerts->x = RD_MULTIPLY(pSrcVerts->x, tz) + centerX;
        pDestVerts->y = -RD_MULTIPLY(pSrcVerts->z, curscale) + currentcenterY;
        pDestVerts->z = pSrcVerts->y;
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_PerspProjectSquare(rdVector3* pDestVertex, const rdVector3* pSrcVertex)
{
    // Function does perspective projection from view space (camera space) to screen space.
    const rdCanvas* pCanvas = rdCamera_pCurCamera->pCanvas;
    float tz = 1.0f / D3DVAL(pSrcVertex->y);
    float scaleFactor = RD_MULTIPLY(rdCamera_pCurCamera->focalLength, tz);
    pDestVertex->x = RD_MULTIPLY(pSrcVertex->x, scaleFactor) + pCanvas->center.x;
    pDestVertex->y = -RD_MULTIPLY(pSrcVertex->z, scaleFactor) + pCanvas->center.y;
    pDestVertex->z = tz;
}

void J3DAPI rdCamera_PerspProjectSquareLst(rdVector3* pDestVerts, const rdVector3* pSrcVerts, size_t numVerts)
{
    float tz;
    float scaleFactor;
    float centerX;
    float yCenter;

    centerX = rdCamera_pCurCamera->pCanvas->center.x;
    yCenter = rdCamera_pCurCamera->pCanvas->center.y;

    while ( numVerts-- )
    {
        tz = 1.0f / D3DVAL(pSrcVerts->y);
        scaleFactor = RD_MULTIPLY(rdCamera_pCurCamera->focalLength, tz);
        pDestVerts->x = RD_MULTIPLY(pSrcVerts->x, scaleFactor) + centerX;
        pDestVerts->y = -RD_MULTIPLY(pSrcVerts->z, scaleFactor) + yCenter;
        pDestVerts->z = tz;
        ++pSrcVerts;
        ++pDestVerts;
    }
}

void J3DAPI rdCamera_SetAmbientLight(rdCamera* pCamera, const rdVector4* pLight)
{
    rdVector_Copy4(&pCamera->ambientLight, pLight);
}

void J3DAPI rdCamera_SetAttenuation(rdCamera* pCamera, float min, float max)
{
    size_t i;
    rdLight* pLight;

    pCamera->attenuationMin = min;
    pCamera->attenuationMax = max;

    for ( i = 0; i < pCamera->numLights; ++i )
    {
        pLight = pCamera->aLights[i];
        if ( pLight->minRadius == 0.0f )
        {
            pLight->minRadius = RD_MULTIPLY(pCamera->attenuationMin, 40.0f);
        }

        if ( pLight->maxRadius == 0.0f )
        {
            pLight->maxRadius = RD_MULTIPLY(pCamera->attenuationMax, 50.0f);
        }
    }
}

int J3DAPI rdCamera_AddLight(rdCamera* pCamera, rdLight* pLight, const rdVector3* pPos)
{
    if ( pCamera->numLights > RDCAMERA_MAX_LIGHTS ) // Retail bug: allows numLights == RDCAMERA_MAX_LIGHTS
    {
        return 0;
    }

    pLight->num = pCamera->numLights;
    pCamera->aLights[pCamera->numLights] = pLight;
    rdVector_Copy3(&pCamera->aLightPositions[pCamera->numLights], pPos);

    if ( pLight->minRadius == 0.0f )
    {
        pLight->minRadius = rdLight_GetIntensity(&pLight->color) / pCamera->attenuationMin;
    }

    if ( pLight->maxRadius == 0.0f )
    {
        pLight->maxRadius = rdLight_GetIntensity(&pLight->color) / pCamera->attenuationMax;
    }

    ++pCamera->numLights;
    return 1;
}

int J3DAPI rdCamera_ClearLights(rdCamera* pCamera)
{
    pCamera->numLights = 0;
    return 1;
}

void J3DAPI rdCamera_SetOverlayDepthRange(int bEnable)
{
    rdCamera_bOverlayDepthRange = bEnable;
}

int J3DAPI rdCamera_GetOverlayDepthRange(void)
{
    return rdCamera_bOverlayDepthRange;
}
