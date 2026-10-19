#include "rdQClip.h"

#include <rdroid/Engine/rdClip.h>
#include <rdroid/Main/rdroid.h>


// One declaration-ordered private block (0x014E0BB8-0x014E1718), laid out like rdClip.c's
// storage block: the slots no surviving function references are the work storage of the
// rdClip-style variants this file did not keep.
static rdVector2* rdQClip_pSourceTVert = NULL;
static rdVector2* rdQClip_pDestTVert = NULL;
static rdVector4* rdQClip_pDestVertIntensity = NULL;
rdVector3* rdQClip_pDestVert = NULL;
static rdVector2 rdQClip_aWorkTVerts[RDQCLIP_MAXWORKVERTS] = { 0 };
rdVector3* rdQClip_pSourceVert = NULL;
static rdVector4* rdQClip_pSourceVertIntensity = NULL;
rdVector3 rdQClip_aWorkVerts[RDQCLIP_MAXWORKVERTS] = { 0 };
static rdVector4 rdQClip_aWorkVertIntensities[RDQCLIP_MAXWORKVERTS] = { 0 };

size_t J3DAPI rdQClip_VerticesInFrustrum(const rdClipFrustum* pFrustrum, const rdVector3* aVertices, size_t numVertices)
{
    const rdVector3* pCurVert;

    if ( aVertices->y < pFrustrum->nearPlane )
    {
        for ( pCurVert = aVertices + numVertices - 1; pCurVert > aVertices; --pCurVert )
        {
            if ( pCurVert->y >= pFrustrum->nearPlane )
            {
                break;
            }
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }
    else if ( pFrustrum->bClipFar && aVertices->y > pFrustrum->farPlane )
    {
        for ( pCurVert = aVertices + numVertices - 1; pCurVert > aVertices; --pCurVert )
        {
            if ( pCurVert->y <= pFrustrum->farPlane )
            {
                break;
            }
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }

    if ( aVertices->x < RD_MULTIPLY(aVertices->y, pFrustrum->leftPlane) )
    {
        for ( pCurVert = aVertices + numVertices - 1; pCurVert > aVertices; --pCurVert )
        {
            if ( pCurVert->x >= RD_MULTIPLY(pCurVert->y, pFrustrum->leftPlane) )
            {
                break;
            }
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }
    else if ( aVertices->x > RD_MULTIPLY(aVertices->y, pFrustrum->rightPlane) )
    {
        for ( pCurVert = aVertices + numVertices - 1; pCurVert > aVertices; --pCurVert )
        {
            if ( pCurVert->x <= RD_MULTIPLY(pCurVert->y, pFrustrum->rightPlane) )
            {
                break;
            }
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }

    if ( aVertices->z > RD_MULTIPLY(aVertices->y, pFrustrum->topPlane) )
    {
        for ( pCurVert = aVertices + numVertices - 1; pCurVert > aVertices; --pCurVert )
        {
            if ( pCurVert->z <= RD_MULTIPLY(pCurVert->y, pFrustrum->topPlane) )
            {
                break;
            }
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }
    else if ( aVertices->z < RD_MULTIPLY(aVertices->y, pFrustrum->bottomPlane) )
    {
        for ( pCurVert = aVertices + numVertices - 1; pCurVert > aVertices; --pCurVert )
        {
            if ( pCurVert->z >= RD_MULTIPLY(pCurVert->y, pFrustrum->bottomPlane) )
            {
                break;
            }
        }

        if ( pCurVert == aVertices )
        {
            return 0;
        }
    }

    return numVertices;
}

// Discarded by retail's linker; restored from the debug build (0x0052A1A7, between
// VerticesInFrustrum and Face3W, assert line 226). Its only caller is rdClip.c's discarded
// rdClip_QFace3T. It is the user of this file's texture/intensity work storage (debug
// 0x0156F908..0x0156FF60 = retail 0x014E0BC0..0x014E1218).
int J3DAPI rdQClip_Face3T(const rdClipFrustum* pFrustrum, rdVector3* aVertices, rdVector2* aTexVertices, rdVector4* aIntensities, int numVertices)
{
    int clipindex;
    rdVector3* pPrevVert;
    rdVector3* pCurVert;
    rdVector2* pPrevTVert;
    rdVector2* pCurTVert;
    rdVector4* pPrevIntens;
    rdVector4* pCurIntens;
    float clipFactor;
    int i;

    rdClip_faceStatus = 0;
    rdQClip_pSourceVert = aVertices;
    rdQClip_pDestVert = rdQClip_aWorkVerts;
    rdQClip_pSourceTVert = aTexVertices;
    rdQClip_pDestTVert = rdQClip_aWorkTVerts;
    rdQClip_pSourceVertIntensity = aIntensities;
    rdQClip_pDestVertIntensity = rdQClip_aWorkVertIntensities;

    clipindex = 0;
    pPrevVert = &rdQClip_pSourceVert[numVertices - 1];
    pPrevTVert = &rdQClip_pSourceTVert[numVertices - 1];
    pPrevIntens = &rdQClip_pSourceVertIntensity[numVertices - 1];
    pCurVert = rdQClip_pSourceVert;
    pCurTVert = rdQClip_pSourceTVert;
    pCurIntens = rdQClip_pSourceVertIntensity;

    for ( i = 0; i < numVertices; ++i )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurVert->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurVert->y == pFrustrum->nearPlane )
            {
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurVert->y < pFrustrum->nearPlane )
            {
                clipFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurVert->y);
                rdQClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdQClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurVert->z - pPrevVert->z), clipFactor) + pPrevVert->z;
                rdQClip_pDestVert[clipindex].x = RD_MULTIPLY((pCurVert->x - pPrevVert->x), clipFactor) + pPrevVert->x;
                rdQClip_pDestTVert[clipindex].x = RD_MULTIPLY((pCurTVert->x - pPrevTVert->x), clipFactor) + pPrevTVert->x;
                rdQClip_pDestTVert[clipindex].y = RD_MULTIPLY((pCurTVert->y - pPrevTVert->y), clipFactor) + pPrevTVert->y;
                rdQClip_pDestVertIntensity[clipindex].x = RD_MULTIPLY((pCurIntens->x - pPrevIntens->x), clipFactor) + pPrevIntens->x;
                rdQClip_pDestVertIntensity[clipindex].y = RD_MULTIPLY((pCurIntens->y - pPrevIntens->y), clipFactor) + pPrevIntens->y;
                rdQClip_pDestVertIntensity[clipindex].z = RD_MULTIPLY((pCurIntens->z - pPrevIntens->z), clipFactor) + pPrevIntens->z;
                rdQClip_pDestVertIntensity[clipindex++].w = RD_MULTIPLY((pCurIntens->w - pPrevIntens->w), clipFactor) + pPrevIntens->w;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurVert->y >= pFrustrum->nearPlane )
            {
                memcpy(&rdQClip_pDestVert[clipindex], pCurVert, sizeof(rdVector3));
                memcpy(&rdQClip_pDestTVert[clipindex], pCurTVert, sizeof(rdVector2));
                memcpy(&rdQClip_pDestVertIntensity[clipindex], pCurIntens, sizeof(rdVector4));
                clipindex++;
            }
        }

#line 226
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurVert;
        pCurVert++;
        pPrevTVert = pCurTVert;
        pCurTVert++;
        pPrevIntens = pCurIntens;
        pCurIntens++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40u;
        return clipindex;
    }
    else
    {
        if ( rdQClip_pDestVert != aVertices )
        {
            memcpy(aVertices, rdQClip_pDestVert, sizeof(aVertices[0]) * clipindex);
            memcpy(aTexVertices, rdQClip_pDestTVert, sizeof(aTexVertices[0]) * clipindex);
            memcpy(aIntensities, rdQClip_pDestVertIntensity, sizeof(aIntensities[0]) * clipindex);
        }

        return clipindex;
    }
}

int J3DAPI rdQClip_Face3W(const rdClipFrustum* pFrustrum, rdVector3* aVertices, int numVertices)
{
    int clipindex;
    rdVector3* pPrevVert;
    rdVector3* pCurrentVert;
    int curi;
    float clipFactor;

    rdClip_faceStatus = 0;
    rdQClip_pSourceVert = aVertices;
    rdQClip_pDestVert   = rdQClip_aWorkVerts;

    clipindex = 0;
    pPrevVert = &rdQClip_pSourceVert[numVertices - 1];
    pCurrentVert = rdQClip_pSourceVert;

    for ( curi = 0; curi < numVertices; ++curi )
    {
        if ( pPrevVert->y >= pFrustrum->nearPlane || pCurrentVert->y >= pFrustrum->nearPlane )
        {
            if ( pPrevVert->y == pFrustrum->nearPlane || pCurrentVert->y == pFrustrum->nearPlane )
            {
            }
            else if ( pPrevVert->y < pFrustrum->nearPlane || pCurrentVert->y < pFrustrum->nearPlane )
            {
                clipFactor = RDCLIP_LERPFACTOR(pFrustrum->nearPlane, pPrevVert->y, pCurrentVert->y);
                rdQClip_pDestVert[clipindex].y = pFrustrum->nearPlane;
                rdQClip_pDestVert[clipindex].z = RD_MULTIPLY((pCurrentVert->z - pPrevVert->z), clipFactor) + pPrevVert->z;
                rdQClip_pDestVert[clipindex++].x = RD_MULTIPLY((pCurrentVert->x - pPrevVert->x), clipFactor) + pPrevVert->x;

                rdClip_faceStatus |= 0x01;
            }

            if ( pCurrentVert->y >= pFrustrum->nearPlane )
            {
                memcpy(&rdQClip_pDestVert[clipindex], pCurrentVert, sizeof(rdVector3));
                clipindex++;
            }
        }

#line 311
        RD_ASSERTREL(clipindex<80);
        pPrevVert = pCurrentVert++;
    }

    if ( clipindex < 3 )
    {
        rdClip_faceStatus |= 0x40u;
        return clipindex;
    }
    else
    {
        if ( rdQClip_pDestVert != aVertices )
        {
            memcpy(aVertices, rdQClip_pDestVert, sizeof(aVertices[0]) * clipindex);
        }

        return clipindex;
    }
}
