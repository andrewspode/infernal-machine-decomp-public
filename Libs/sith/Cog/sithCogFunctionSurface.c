#include "sithCogFunctionSurface.h"

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogexec.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>

void J3DAPI sithCogFunctionSurface_GetAdjoinAlpha(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetAdjoinAlpha(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceAdjoin(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceSector(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetNumSurfaceVertices(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceVertexPos(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetHorizonSkyOffset(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetHorizonSkyOffset(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetCeilingSkyOffset(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetCeilingSkyOffset(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SlideHorizonSky(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SlideCeilingSky(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SurfaceLightAnim(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SlideWall(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetWallCel(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetWallCel(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceMaterial(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetSurfaceMaterial(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetSurfaceFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_ClearSurfaceFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetAdjoinFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_ClearAdjoinFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetAdjoinFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetFaceType(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_ClearFaceType(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetFaceType(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetFaceGeoMode(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetFaceGeoMode(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetFaceLightMode(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetFaceLightMode(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SetSurfaceLight(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceLight(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceCenter(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceCount(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_GetSurfaceNormal(SithCog* pCog);
void J3DAPI sithCogFunctionSurface_SyncSurface(SithCog* pCog);

void J3DAPI sithCogFunctionSurface_RegisterFunctions(SithCogSymbolTable* pTbl)
{
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceAdjoin, "getsurfaceadjoin");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceSector, "getsurfacesector");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetNumSurfaceVertices, "getnumsurfacevertices");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceVertexPos, "getsurfacevertexpos");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetHorizonSkyOffset, "sethorizonskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetHorizonSkyOffset, "gethorizonskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetCeilingSkyOffset, "setceilingskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetCeilingSkyOffset, "getceilingskyoffset");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideHorizonSky, "slidehorizonsky");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideCeilingSky, "slideceilingsky");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceCount, "getsurfacecount");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideWall, "slidewall");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SlideWall, "slidesurface");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetWallCel, "getwallcel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetWallCel, "setwallcel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetWallCel, "getsurfacecel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetWallCel, "setsurfacecel");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceMaterial, "getsurfacemat");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetSurfaceMaterial, "setsurfacemat");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceFlags, "getsurfaceflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetSurfaceFlags, "setsurfaceflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_ClearSurfaceFlags, "clearsurfaceflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetAdjoinFlags, "getadjoinflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetAdjoinFlags, "setadjoinflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_ClearAdjoinFlags, "clearadjoinflags");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetFaceType, "setfacetype");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_ClearFaceType, "clearfacetype");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetFaceType, "getfacetype");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetFaceGeoMode, "setfacegeomode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetFaceGeoMode, "getfacegeomode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetFaceLightMode, "setfacelightmode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetFaceLightMode, "getfacelightmode");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceLight, "getsurfacelight");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetSurfaceLight, "setsurfacelight");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceCenter, "getsurfacecenter");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SurfaceLightAnim, "surfacelightanim");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetSurfaceNormal, "getsurfacenormal");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SyncSurface, "syncsurface");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_GetAdjoinAlpha, "getadjoinalpha");
    sithCog_RegisterFunction(pTbl, sithCogFunctionSurface_SetAdjoinAlpha, "setadjoinalpha");
}

void J3DAPI sithCogFunctionSurface_GetAdjoinAlpha(SithCog* pCog)
{
    SithSurface* pSurf;
    float curalpha;

    pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
#line 145
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface passed to GetAdjoinAlpha.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    curalpha = pSurf->aIntensities->alpha * 255.0f; // TODO: Is this right? Shouldn't be extralight.alpha?
    sithCogExec_PushFlex(pCog, curalpha);
}

void J3DAPI sithCogFunctionSurface_SetAdjoinAlpha(SithCog* pCog)
{
    size_t i;
    rdFace* pSurfaceFace;
    float alpha = sithCogExec_PopFlex(pCog);
    SithSurface* pSurface = sithCogExec_PopSurface(pCog);

    if ( !pSurface )
    {
#line 167
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface passed to SetAdjoinAlpha.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pSurfaceFace = &pSurface->face;
    alpha = ((float)(alpha / 255.0));
    alpha = STDMATH_CLAMP(alpha, 0.0f, 1.0f);
    if ( alpha < 1.0f )
    {
        pSurface->face.flags |= RD_FF_TEX_TRANSLUCENT;
    }
    else
    {
        pSurface->face.flags &= ~RD_FF_TEX_TRANSLUCENT;
    }

    // Update alpha of all vertices
    for ( i = 0; i < pSurfaceFace->numVertices; ++i )
    {
        pSurface->aIntensities[i].alpha = alpha;
    }

    pSurface->flags |= SITH_SURFACE_SYNC;
}

void J3DAPI sithCogFunctionSurface_GetSurfaceAdjoin(SithCog* pCog)
{
    SithSurface* pAdjSurf;

    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !pSurf->pAdjoin || !pSurf->pAdjoin->pMirrorAdjoin )
    {
#line 207
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface passed to GetSurfaceAdjoin.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pAdjSurf = pSurf->pAdjoin->pMirrorAdjoin->pAdjoinSurface;
    if ( pAdjSurf )
    {
        sithCogExec_PushInt(pCog, sithSurface_GetSurfaceIndex(pAdjSurf));
    }
    else
    {
#line 220
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No adjoining surface in GetSurfaceAdjoin.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceSector(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
#line 245
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface passed to GetSurfaceSector.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( pSurf->pSector )
    {
        sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pSurf->pSector));
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_GetNumSurfaceVertices(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
#line 280
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface passed to GetNumSurfaceVertices.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSurf->face.numVertices);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceVertexPos(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;

    int vertCount = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);

    if ( !pSurf )
    {
#line 313
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface passed to GetSurfaceVertexPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    if ( vertCount >= pSurf->face.numVertices || vertCount < 0 )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad index passed to GetSurfaceVertexPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    sithCogExec_PushVector(pCog, &pWorld->aVertices[pSurf->face.aVertices[vertCount]]);
}


void J3DAPI sithCogFunctionSurface_SetHorizonSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    SithWorld* pWorld;

    pWorld = sithWorld_pCurrentWorld;
    sithCogExec_PopVector(pCog, &offset);
    pWorld->horizonSkyOffset.x = offset.x;
    pWorld->horizonSkyOffset.y = offset.y;
}

void J3DAPI sithCogFunctionSurface_GetHorizonSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    offset.x = pWorld->horizonSkyOffset.x;
    offset.y = pWorld->horizonSkyOffset.y;
    offset.z = 0.0f;
    sithCogExec_PushVector(pCog, &offset);
}

void J3DAPI sithCogFunctionSurface_SetCeilingSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    SithWorld* pWorld;

    pWorld = sithWorld_pCurrentWorld;
    sithCogExec_PopVector(pCog, &offset);
    pWorld->ceilingSkyOffset.x = offset.x;
    pWorld->ceilingSkyOffset.y = offset.y;
}

void J3DAPI sithCogFunctionSurface_GetCeilingSkyOffset(SithCog* pCog)
{
    rdVector3 offset;
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    offset.x = pWorld->ceilingSkyOffset.x;
    offset.y = pWorld->ceilingSkyOffset.y;
    offset.z = 0.0f;
    sithCogExec_PushVector(pCog, &offset);
}

void J3DAPI sithCogFunctionSurface_SlideHorizonSky(SithCog* pCog)
{
    rdVector2 curdir;

    SithAnimationSlot* pAnim;
    float x;
    float y;

    y = sithCogExec_PopFlex(pCog);
    x = sithCogExec_PopFlex(pCog);
    curdir.x = x;
    curdir.y = y;

    pAnim = sithAnimate_StartSideSkyAnim(SITH_SURFACE_HORIZONSKY, &curdir);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SlideCeilingSky(SithCog* pCog)
{
    rdVector2 curdir;

    SithAnimationSlot* pAnim;
    float x;
    float y;

    y = sithCogExec_PopFlex(pCog);
    x = sithCogExec_PopFlex(pCog);
    curdir.x = x;
    curdir.y = y;

    pAnim = sithAnimate_StartSideSkyAnim(SITH_SURFACE_CEILINGSKY, &curdir);
    if ( pAnim )
    {
        sithCogExec_PushInt(pCog, pAnim->animID);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SurfaceLightAnim(SithCog* pCog)
{
    rdVector4 curstartColor;
    SithSurface* pCurSurf;
    SithAnimationSlot* pCurrentAnim;

    float speed = sithCogExec_PopFlex(pCog);

    // TODO: [BUG] I think this is wrong, the order of the colors is wrong
    rdVector4 endColor; // Retail bug: the alpha component is never initialised
    endColor.red   = sithCogExec_PopFlex(pCog);
    endColor.green = sithCogExec_PopFlex(pCog);
    endColor.blue  = sithCogExec_PopFlex(pCog);

    // TODO: [BUG] I think this is wrong, the order of the colors is wrong
    
    curstartColor.red   = sithCogExec_PopFlex(pCog);
    curstartColor.green = sithCogExec_PopFlex(pCog);
    curstartColor.blue  = sithCogExec_PopFlex(pCog);

    pCurSurf = sithCogExec_PopSurface(pCog);
    if ( !pCurSurf )
    {
#line 538
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad parameters in SurfaceLightAnim().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        rdVector_Copy4(&pCurSurf->face.extraLight, &curstartColor); // Retail bug: startColor.alpha is uninitialised and copied with the rest

        pCurrentAnim = sithAnimate_StartSurfaceLightAnim(pCurSurf, (const rdVector3*)&endColor, speed * 0.5f, SITHANIMATE_LOOP);
        if ( pCurrentAnim )
        {
            sithCogExec_PushInt(pCog, pCurrentAnim->animID);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSurface_SlideWall(SithCog* pCog)
{
    SithAnimationSlot* pAnim;
    int bVector;
    SithSurface* pSurf;

    rdVector3 curdir;
    float speed = sithCogExec_PopFlex(pCog);
    speed = speed / 10.0f;
    bVector  = sithCogExec_PopVector(pCog, &curdir);
    pSurf = sithCogExec_PopSurface(pCog);

    if ( !pSurf || !pSurf->face.pMaterial )
    {
#line 593
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal surface index in SlideWall call.\n", pCog->aName, 0, 0, 0);
    }
    else if ( !bVector )
    {
#line 599
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal vector in SlideWall call.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        rdVector_Scale3Acc(&curdir, speed);
        pAnim = sithAnimate_StartScrollSurfaceAnim(pSurf, &curdir);
        if ( pAnim )
        {
            sithSurface_SyncSurface(pSurf);
            sithCogExec_PushInt(pCog, pAnim->animID);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSurface_GetWallCel(SithCog* pCog)
{
    int curCel;
    SithSurface* pSurf;

#line 642
    SITH_ASSERTREL(pCog);

    pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || !pSurf->face.pMaterial )
    {
#line 649
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal surface index in GetWallCel call.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        curCel = pSurf->face.matCelNum;
        sithCogExec_PushInt(pCog, curCel);
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSurface_SetWallCel(SithCog* pCog)
{
    int celNum;
    SithSurface* pSurf;
    int curcurCelNum;

#line 692
    SITH_ASSERTREL(pCog);
    celNum = sithCogExec_PopInt(pCog);
    pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf || pSurf->face.pMaterial == NULL )
    {
#line 700
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal surface index in SetWallCel call.\n", pCog->aName, 0, 0, 0);
    }
    // cell num can be -1 or < numCels
    else if ( celNum < -1 || celNum >= (int)pSurf->face.pMaterial->numCels )
    {
#line 707
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal cel number %d in SetWallCel call.\n", pCog->aName, celNum, 0, 0);
    }
    else
    {
        curcurCelNum = pSurf->face.matCelNum;
        pSurf->face.matCelNum = celNum;

        sithSurface_SyncSurface(pSurf);
        sithCogExec_PushInt(pCog, curcurCelNum);
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSurface_GetSurfaceMaterial(SithCog* pCog)
{
    rdMaterial* pMaterial;
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pMaterial = pSurf->face.pMaterial;
    if ( pMaterial )
    {
        sithCogExec_PushInt(pCog, pMaterial - sithWorld_pCurrentWorld->aMaterials);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetSurfaceMaterial(SithCog* pCog)
{
    rdMaterial* pCurrentMat;

    rdMaterial* pMaterial   = sithCogExec_PopMaterial(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    pCurrentMat = pSurf->face.pMaterial;
    pSurf->face.pMaterial = pMaterial;
    if ( pCurrentMat )
    {
        sithCogExec_PushInt(pCog, pCurrentMat - sithWorld_pCurrentWorld->aMaterials);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }

    sithSurface_SyncSurface(pSurf);
}

void J3DAPI sithCogFunctionSurface_SetSurfaceFlags(SithCog* pCog)
{
    int cursurfflags      = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf && cursurfflags )
    {
        pSurf->flags |= cursurfflags;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 828
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set surface flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_ClearSurfaceFlags(SithCog* pCog)
{
    int cursurfflags      = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf && cursurfflags )
    {
        pSurf->flags &= ~cursurfflags;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 858
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear surface flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceFlags(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetAdjoinFlags(SithCog* pCog)
{
    int currentadjflags       = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf && pSurf->pAdjoin != NULL && currentadjflags != 0 )
    {
        pSurf->pAdjoin->flags |= currentadjflags;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 919
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set adjoin flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_ClearAdjoinFlags(SithCog* pCog)
{
    int currentadjflags        = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf && pSurf->pAdjoin != NULL && currentadjflags != 0 )
    {
        pSurf->pAdjoin->flags &= ~currentadjflags;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 951
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear adjoin flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_GetAdjoinFlags(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        if ( pSurf->pAdjoin )
        {
            sithCogExec_PushInt(pCog, pSurf->pAdjoin->flags);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetFaceType(SithCog* pCog)
{
    rdFaceFlags curfaceflags = sithCogExec_PopInt(pCog);
    SithSurface* pSurf    = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        pSurf->face.flags |= curfaceflags;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 1015
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set face type.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_ClearFaceType(SithCog* pCog)
{
    rdFaceFlags curfaceflags = sithCogExec_PopInt(pCog);
    SithSurface* pSurf    = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        pSurf->face.flags &= ~curfaceflags;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 1047
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot clear face type.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_GetFaceType(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->face.flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetFaceGeoMode(SithCog* pCog)
{
    int geoMode        = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        if ( pSurf->face.pMaterial )
        {
            pSurf->face.geometryMode = geoMode;
            sithSurface_SyncSurface(pSurf);
        }
        else
        {
            pSurf->face.geometryMode = RD_GEOMETRY_NONE;
            if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
            {
                sithSurface_SyncSurface(pSurf);
            }
        }
    }
    else
    {
#line 1122
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set face geometryMode.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_GetFaceGeoMode(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->face.geometryMode);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetFaceLightMode(SithCog* pCog)
{
    int lightmode      = sithCogExec_PopInt(pCog);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        pSurf->face.lightingMode = lightmode;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 1185
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set face lightingMode.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSurface_GetFaceLightMode(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushInt(pCog, pSurf->face.lightingMode);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_SetSurfaceLight(SithCog* pCog)
{
    rdVector3 light;
    float fadeTime = sithCogExec_PopFlex(pCog);
    int bVecValid = sithCogExec_PopVector(pCog, &light);
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    SithAnimationSlot* pAnimation;
    if ( !pSurf || !bVecValid )
    {
        goto error;
    }

    if ( fadeTime == 0.0f )
    {

        pSurf->face.extraLight.red   = light.red;
        pSurf->face.extraLight.green = light.green;
        pSurf->face.extraLight.blue  = light.blue;
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
        pAnimation = sithAnimate_StartSurfaceLightAnim(pSurf, &light, fadeTime, 0);
        if ( pAnimation )
        {
            sithCogExec_PushInt(pCog, pAnimation->animID);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }

    return;

error:
#line 1269
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal params in SetSurfaceLight().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSurface_GetSurfaceLight(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( !pSurf )
    {
        goto error;
    }

    sithCogExec_PushVector(pCog, (const rdVector3*)&pSurf->face.extraLight);
    return;

error:
#line 1299
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal params in GetSurfaceLight().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
}

void J3DAPI sithCogFunctionSurface_GetSurfaceCenter(SithCog* pCog)
{
    rdVector3 curpoint;

    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithSurface_GetCenterPoint(pSurf, &curpoint);
        sithCogExec_PushVector(pCog, &curpoint);
    }
    else
    {
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
}


void J3DAPI sithCogFunctionSurface_GetSurfaceCount(SithCog* pCog)
{
    if ( sithWorld_pCurrentWorld )
    {
        sithCogExec_PushInt(pCog, sithWorld_pCurrentWorld->numSurfaces);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSurface_GetSurfaceNormal(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithCogExec_PushVector(pCog, &pSurf->face.normal);
    }
    else
    {
#line 1380
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to GetSurfaceNormal().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
}

void J3DAPI sithCogFunctionSurface_SyncSurface(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        sithSurface_SyncSurface(pSurf);
    }
    else
    {
#line 1409
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface in SyncSurface().\n", pCog->aName, 0, 0, 0);
    }
}
