#include "rdSprite.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/stdMemory.h>

#include <math.h>
#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(rdSprite_countfit_1, 15)
CF_PROTO(rdSprite_countfit_2, 15)
CF_PROTO(rdSprite_countfit_3, 15)
CF_PROTO(rdSprite_countfit_4, 15)
CF_PROTO(rdSprite_countfit_5, 3)
/* end count-fit */

rdMatrix34 rdSprite_aView = { 0 };

/* Retail's linker discarded this function (no retail symbol, address or caller). Its evidence is the
   /Gf literal order: retail's run is J3D_FILE, then NewEntry's "Error: Bad memory allocation for
   sprite", while NewEntry alone emits them the other way round (its STDMALLOC creates J3D_FILE
   first; VC5 emits a function's new literals in reverse creation order). So an earlier function
   emitted J3D_FILE. Jedi Knight's rdSprite_New allocates the sprite and calls NewEntry. Body from the
   Indy debug build (0x00528350, lines 88..91); parameter order Jedi Knight's
   (type first; NewEntry takes the name first), which the debug call confirms. */
rdSprite3* J3DAPI rdSprite_New(int type, const char* pName, const char* pMatName, float width, float height, rdGeometryMode geomode, rdLightMode lightmode, const rdVector4* pExtraLight, const rdVector3* pOffset)
{
    rdSprite3* pSprite;

#line 88
    pSprite = (rdSprite3*)STDMALLOC(sizeof(rdSprite3));
    if ( !pSprite )
    {
#line 91
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for sprite.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdSprite_NewEntry(pSprite, pName, type, pMatName, width, height, geomode, lightmode, pExtraLight, pOffset);
    return pSprite;
}

int J3DAPI rdSprite_NewEntry(rdSprite3* pSprite, const char* pName, int type, const char* pMatName, float width, float height, rdGeometryMode geomode, rdLightMode lightmode, const rdVector4* pExtraLight, const rdVector3* pOffset)
{
    size_t idx;
    float zero = 0.0f;
    rdFace* pFace;
    float one  = 1.0f;

    if ( pName )
    {
        strncpy(pSprite->aName, pName, 63);
        pSprite->aName[63] = 0;
    }

    pSprite->type = type;
    pSprite->width = width;
    pSprite->height = height;
    rdVector_Copy3(&pSprite->offset, pOffset);

    pFace = &pSprite->face;
    pFace->flags = RD_FF_FOG_ENABLED | RD_FF_DOUBLE_SIDED;
    pFace->geometryMode = geomode;
    pFace->lightingMode = lightmode;
    rdVector_Copy4(&pFace->extraLight, pExtraLight);

    pFace->pMaterial = rdMaterial_Load(pMatName);
    if ( !pFace->pMaterial )
    {
        return 0;
    }

    pFace->numVertices = 4;
#line 172
    pFace->aVertices = (size_t*)STDMALLOC(sizeof(*pFace->aVertices) * pFace->numVertices);
    if ( !pFace->aVertices )
    {
        goto alloc_error;
    }
    if ( pFace->geometryMode > RD_GEOMETRY_SOLID )
    {
#line 179
        pFace->aTexVertices = (size_t*)STDMALLOC(sizeof(*pFace->aTexVertices) * pFace->numVertices);
        if ( !pFace->aTexVertices )
        {
            goto alloc_error;
        }

        for ( idx = 0; idx < pFace->numVertices; ++idx )
        {
            pFace->aVertices[idx]    = idx;
            pFace->aTexVertices[idx] = idx;
        }

#line 191
        pSprite->aTexVerts = (rdVector2*)STDMALLOC(sizeof(*pSprite->aTexVerts) * pFace->numVertices);
        if ( !pSprite->aTexVerts )
        {
            goto alloc_error;
        }

        pSprite->aTexVerts[0].x = zero;
        pSprite->aTexVerts[0].y = zero;
        pSprite->aTexVerts[1].x = one;
        pSprite->aTexVerts[1].y = zero;
        pSprite->aTexVerts[2].x = one;
        pSprite->aTexVerts[2].y = one;
        pSprite->aTexVerts[3].x = zero;
        pSprite->aTexVerts[3].y = one;
    }
    else
    {
        for ( idx = 0; idx < pFace->numVertices; ++idx )
        {
            pFace->aVertices[idx] = idx;
        }
    }

    pSprite->widthHalf  = pSprite->width / 2.0f;
    pSprite->heightHalf = pSprite->height / 2.0f;
    pSprite->radius     = sqrtf(D3DMultiply(pSprite->widthHalf, pSprite->widthHalf) + D3DMultiply(pSprite->heightHalf, pSprite->heightHalf));
    return 1;

    // Retail's one print block (push 232) is entered from three allocation
    // failures and laid out after the aTexVerts test; no structured form gives those edges

alloc_error:
#line 232
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for sprite '%'.\n", pName, 0, 0, 0);
    return 0;
}

/* Linker-discarded (no caller in either image): the debug build's rdSprite_Free (0x005286AC), restored from its code. */
void J3DAPI rdSprite_Free(rdSprite3* pSprite)
{
    if ( pSprite )
    {
        rdSprite_FreeEntry(pSprite);
        stdMemory_Free(pSprite);
    }
    else
    {
#line 266
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL sprite ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdSprite_FreeEntry(rdSprite3* pSprite)
{
    rdFace* pFace;

    pFace = &pSprite->face;

    if ( pSprite->aTexVerts )
    {
        stdMemory_Free(pSprite->aTexVerts);
        pSprite->aTexVerts = NULL;
    }

    if ( pFace->aVertices )
    {
        stdMemory_Free(pFace->aVertices);
        pFace->aVertices = NULL;
    }

    if ( pFace->aTexVertices )
    {
        stdMemory_Free(pFace->aTexVertices);
        pFace->aTexVertices = NULL;
    }
}

int J3DAPI rdSprite_Draw(rdThing* prdThing, const rdMatrix34* orient)
{
    rdCacheProcEntry* pPoly;
    rdSprite3* pSprite3;
    rdFace* pFace;
    rdVector3 tpos;
    rdVector3 lvec;
    size_t i;
    RdFrustumCull frustumCull;

    pSprite3 = prdThing->data.pSprite3;
    pFace    = &pSprite3->face;

    if ( pSprite3->type == 0 )
    {
        rdMatrix_TransformPoint34(&tpos, &orient->dvec, &rdCamera_pCurCamera->orient);
    }
    else if ( pSprite3->type == 2 )
    {
        rdVector_Copy3(&tpos, &orient->dvec);
        rdVector_Copy3(&lvec, &orient->lvec);
    }

    frustumCull = prdThing->frustumCull;
    if ( frustumCull != RDFRUSTUMCULL_OUTSIDE )
    {
        pPoly = rdCache_GetAlphaProcEntry();
        if ( !pPoly )
        {
            return 0;
        }

        if ( pSprite3->type == 0 )
        {
            rdSprite_aView.rvec.x = -pSprite3->widthHalf + pSprite3->offset.x;
            rdSprite_aView.rvec.y = pSprite3->offset.y;
            rdSprite_aView.rvec.z = pSprite3->heightHalf + pSprite3->offset.z;

            rdSprite_aView.lvec.x = pSprite3->widthHalf + pSprite3->offset.x;
            rdSprite_aView.lvec.y = pSprite3->offset.y;
            rdSprite_aView.lvec.z = pSprite3->heightHalf + pSprite3->offset.z;

            rdSprite_aView.uvec.x = pSprite3->widthHalf + pSprite3->offset.x;
            rdSprite_aView.uvec.y = pSprite3->offset.y;
            rdSprite_aView.uvec.z = -pSprite3->heightHalf + pSprite3->offset.z;

            rdSprite_aView.dvec.x = -pSprite3->widthHalf + pSprite3->offset.x;
            rdSprite_aView.dvec.y = pSprite3->offset.y;
            rdSprite_aView.dvec.z = -pSprite3->heightHalf + pSprite3->offset.z;

            if ( pSprite3->rollAngle != 0.0f )
            {
                rdVector3 vecPYR;
                memset(&vecPYR, 0, sizeof(rdVector3));
                vecPYR.z = pSprite3->rollAngle;
                rdMatrix_PostRotate34(&rdSprite_aView, &vecPYR);
                pSprite3->rollAngle = 0.0f;
            }

            rdVector_Add3Acc(&rdSprite_aView.rvec, &tpos);
            rdVector_Add3Acc(&rdSprite_aView.lvec, &tpos);
            rdVector_Add3Acc(&rdSprite_aView.uvec, &tpos);
            rdVector_Add3Acc(&rdSprite_aView.dvec, &tpos);
        }
        else if ( pSprite3->type == 2 )
        {
            rdVector3 down;
            rdVector3 left;
            rdVector3 right;
            rdVector3 prod;
            rdVector3 up;

            if ( fabs(lvec.z >= 0.9999f) != 0.0 )
            {
                rdVector_Cross3(&prod, &rdroid_xVector3, &lvec);
            }
            else
            {
                rdVector_Cross3(&prod, &rdroid_zVector3, &lvec);
            }

            rdVector_Cross3(&up, &prod, &lvec);
            rdVector_Scale3(&right, &prod, pSprite3->widthHalf);

            rdVector_Scale3(&down, &up, pSprite3->heightHalf);

            rdVector_Neg3(&left, &right);

            rdVector_Sub3(&rdSprite_aView.rvec, &left, &down);

            rdVector_Sub3(&rdSprite_aView.lvec, &right, &down);

            rdVector_Add3(&rdSprite_aView.uvec, &right, &down);

            rdVector_Add3(&rdSprite_aView.dvec, &left, &down);

            if ( pSprite3->rollAngle != 0.0f )
            {
                rdMatrix34 tmat;
                rdMatrix_BuildFromVectorAngle34(&tmat, &lvec, pSprite3->rollAngle);
                rdMatrix_PostMultiply34(&rdSprite_aView, &tmat);
                pSprite3->rollAngle = 0.0f;
            }

            rdVector_Add3Acc(&rdSprite_aView.rvec, &tpos);
            rdVector_Add3Acc(&rdSprite_aView.lvec, &tpos);
            rdVector_Add3Acc(&rdSprite_aView.uvec, &tpos);
            rdVector_Add3Acc(&rdSprite_aView.dvec, &tpos);
        }

        if ( pSprite3->type == 2 )
        {
            for ( i = 0; i < 4; ++i )
            {
                rdVector3 tvec;
                rdMatrix_TransformPoint34(&tvec, &(&rdSprite_aView.rvec)[i], &rdCamera_pCurCamera->orient);
                rdVector_Copy3(&(&rdSprite_aView.rvec)[i], &tvec);
            }
        }

        if ( rdClip_FaceToPlane(rdCamera_pCurCamera->pFrustum, pPoly, pFace, &rdSprite_aView.rvec, pSprite3->aTexVerts, NULL, NULL) )
        {
            pPoly->aVertIntensities->alpha = pFace->extraLight.alpha;
            rdVector_Copy4(&pPoly->extraLight, &pFace->extraLight);

            if ( (rdroid_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
            {
                pPoly->extraLight.red   += rdCamera_pCurCamera->ambientLight.red;
                pPoly->extraLight.green += rdCamera_pCurCamera->ambientLight.green;
                pPoly->extraLight.blue  += rdCamera_pCurCamera->ambientLight.blue;
                pPoly->extraLight.alpha += rdCamera_pCurCamera->ambientLight.alpha;
            }

            pPoly->matCelNum    = prdThing->matCelNum;
            pPoly->flags        = pFace->flags;
            pPoly->flags       |= RD_FF_FOG_ENABLED | RD_FF_ZWRITE_DISABLED | RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
            pPoly->pMaterial    = pFace->pMaterial;
            pPoly->lightingMode = J3DMIN(J3DMIN(rdroid_curLightingMode, pFace->lightingMode), RD_LIGHTING_DIFFUSE);
            rdCache_AddAlphaProcFace(pFace->numVertices);
        }
        else
        {
            return 0;
        }

        return 1;
    }

    return 0;
}
