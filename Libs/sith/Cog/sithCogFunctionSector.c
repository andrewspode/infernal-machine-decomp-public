#include "sithCogFunctionSector.h"

#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogexec.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithWorld.h>






void J3DAPI sithCogFunctionSector_GetSectorTint(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorTint(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorLight(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorLight(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorAdjoins(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorSurfflags(SithCog* pCog);
void J3DAPI sithCogFunctionSector_ClearSectorSurfflags(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorColormap(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorColormap(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorThrust(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorThrust(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SetSectorFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSector_ClearSectorFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorFlags(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorThingCount(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorPlayerCount(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorCount(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorCenter(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetNumSectorVertices(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetNumSectorSurfaces(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorVertexPos(SithCog* pCog);
void J3DAPI sithCogFunctionSector_GetSectorSurfaceRef(SithCog* pCog);
void J3DAPI sithCogFunctionSector_SyncSector(SithCog* pCog);
void J3DAPI sithCogFunctionSector_FindSectorAtPos(SithCog* pCog);

int J3DAPI sithCogFunctionSector_RegisterFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorTint, "getsectortint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorTint, "setsectortint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorAdjoins, "setsectoradjoins");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorAdjoins, "sectoradjoins");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorLight, "getsectorlight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorLight, "setsectorlight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorLight, "sectorlight");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorColormap, "getcolormap");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorColormap, "getsectorcolormap");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorColormap, "setcolormap");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorColormap, "setsectorcolormap");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorThrust, "getsectorthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorThrust, "setsectorthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorThrust, "sectorthrust");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorFlags, "getsectorflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorFlags, "setsectorflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_ClearSectorFlags, "clearsectorflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorThingCount, "getsectorthingcount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorThingCount, "sectorthingcount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorPlayerCount, "getsectorplayercount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorPlayerCount, "sectorplayercount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorCount, "getsectorcount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorCenter, "getsectorcenter");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetNumSectorVertices, "getnumsectorvertices");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorVertexPos, "getsectorvertexpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetNumSectorSurfaces, "getnumsectorsurfaces");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_GetSectorSurfaceRef, "getsectorsurfaceref");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SyncSector, "syncsector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_FindSectorAtPos, "findsectoratpos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_SetSectorSurfflags, "setsectorsurfflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunctionSector_ClearSectorSurfflags, "clearsectorsurfflags");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithCogFunctionSector_GetSectorTint(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        goto error;
    }

    sithCogExec_PushVector(pCog, &pSector->tint);
    return;

error:
#line 182
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to GetSectorTint().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
}

void J3DAPI sithCogFunctionSector_SetSectorTint(SithCog* pCog)
{
    SithSector* pSector;

    rdVector3 color;
    sithCogExec_PopVector(pCog, &color);

    pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        #line 209
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector in SetSectorTint().\n", pCog->aName, 0, 0, 0);
        return;
    }

    pSector->tint.x = STDMATH_CLAMP(color.x, 0.0f, 1.0f);
    pSector->tint.y = STDMATH_CLAMP(color.y, 0.0f, 1.0f);
    pSector->tint.z = STDMATH_CLAMP(color.z, 0.0f, 1.0f);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 )
    {
        if ( pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
        {
            sithSector_SyncSector(pSector, 1);
        }
    }
}

void J3DAPI sithCogFunctionSector_GetSectorLight(SithCog* pCog)
{
    rdVector4 color;

    SithSector* pCurSector = sithCogExec_PopSector(pCog);
    if ( !pCurSector )
    {
        goto error;
    }

    color.x = pCurSector->extraLight.x;
    color.y = pCurSector->extraLight.y;
    color.z = pCurSector->extraLight.z;
    sithCogExec_PushVector(pCog, (rdVector3*)&color);
    return;

error:
#line 253
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to GetSectorLight().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushFlex(pCog, 0.0f);
}

void J3DAPI sithCogFunctionSector_SetSectorLight(SithCog* pCog)
{
    rdVector3 color;
    float curtimeDelta     = sithCogExec_PopFlex(pCog);
    int bCurVector            = sithCogExec_PopVector(pCog, &color);
    SithSector* pCurSector = sithCogExec_PopSector(pCog);
    if ( !pCurSector || !bCurVector )
    {
        goto error;
    }

    if ( curtimeDelta == 0.0f )
    {
        pCurSector->extraLight.red   =  color.red;
        pCurSector->extraLight.green = color.green;
        pCurSector->extraLight.blue  = color.blue;
        sithSector_SyncSector(pCurSector, 1);
    }
    else
    {
        sithAnimate_StartSectorLightAnim(pCurSector, &color, curtimeDelta, (SithAnimateFlags)0);
    }

    return;

error:
#line 301
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to SectorLight().\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionSector_SetSectorAdjoins(SithCog* pCog)
{
    int bOn = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        goto error;
    }

    if ( bOn && (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 )
    {
        sithSector_ShowSectorAdjoins(pSector);
    }
    else if ( !bOn && (pSector->flags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_HideSectorAdjoins(pSector);
    }
    else
    {
        return;
    }

    sithSector_SyncSector(pSector, 0);
    return;

error:
#line 347
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal params in SetSectorAdjoins().\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionSector_SetSectorSurfflags(SithCog* pCog)
{
    int cursurfflags = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    int i;
    SithSurface* pSurf;
    if ( pSector && cursurfflags )
    {
        for ( i = 0; i < (signed int)pSector->numSurfaces; ++i )
        {
            pSurf = &pSector->pFirstSurface[i];
            pSurf->flags |= cursurfflags;
            sithSurface_SyncSurface(pSurf);
        }
    }
    else
    {
#line 375
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set sector surfaces flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSector_ClearSectorSurfflags(SithCog* pCog)
{
    int cursurfflags = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    int i;
    SithSurface* pSurf;
    if ( pSector && cursurfflags )
    {
        for ( i = 0; i < (signed int)pSector->numSurfaces; ++i )
        {
            pSurf = &pSector->pFirstSurface[i];
            pSurf->flags &= ~cursurfflags;
            sithSurface_SyncSurface(pSurf);
        }
    }
    else
    {
#line 404
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Cannot set sector surfaces flags.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorColormap(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);

    #line 425
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Sector colormaps NOT SUPPORTED.\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunctionSector_SetSectorColormap(SithCog* pCog)
{
    int colormap = sithCogExec_PopInt(pCog);
    #line 448
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: SetSectorColormap NOT SUPPORTED.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogFunctionSector_GetSectorThrust(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        goto error;
    }

    sithCogExec_PushVector(pCog, &pSector->thrust);
    return;

error:
#line 477
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal call to GetSectorThrust().\n", pCog->aName, 0, 0, 0);
    sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
}

void J3DAPI sithCogFunctionSector_SetSectorThrust(SithCog* pCog)
{
    rdVector3 dir;
    float speed         = sithCogExec_PopFlex(pCog);
    int bVector            = sithCogExec_PopVector(pCog, &dir);
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( !pSector || !bVector )
    {
        return;
    }

    if ( dir.x != 0.0f || dir.y != 0.0f || dir.z != 0.0f )
    {
        pSector->flags |= SITH_SECTOR_USETHRUST;
        rdVector_Scale3(&pSector->thrust, &dir, speed);
    }
    else
    {
        pSector->flags &= ~SITH_SECTOR_USETHRUST;
        memset(&pSector->thrust, 0, sizeof(pSector->thrust));
    }

    sithSector_SyncSector(pSector, 1);
}

void J3DAPI sithCogFunctionSector_SetSectorFlags(SithCog* pCog)
{
    SithSectorFlag flags = sithCogExec_PopInt(pCog);
    SithSector* pSector  = sithCogExec_PopSector(pCog);
    if ( pSector && flags )
    {
        sithSector_SetSectorFlags(pSector, flags);
        sithSector_SyncSector(pSector, 0);
    }
    else
    {
#line 554
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad Args in SetSectorFlags().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSector_ClearSectorFlags(SithCog* pCog)
{
    SithSectorFlag flags = sithCogExec_PopInt(pCog);
    SithSector* pSector  = sithCogExec_PopSector(pCog);
    if ( pSector && flags )
    {
        sithSector_ClearSectorFlags(pSector, flags);
        sithSector_SyncSector(pSector, 0);
    }
    else
    {
#line 586
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad Args in ClearSectorFlags().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorFlags(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( pSector )
    {
        sithCogExec_PushInt(pCog, pSector->flags);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorThingCount(SithCog* pCog)
{
    size_t nThings = 0;
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( pSector )
    {
        nThings = sithSector_GetSectorThingCount(pSector);
        sithCogExec_PushInt(pCog, nThings);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorPlayerCount(SithCog* pCog)
{
    size_t count = 0;
    SithSector* pCurSector = sithCogExec_PopSector(pCog);
    if ( pCurSector )
    {
        count = sithSector_GetSectorPlayerCount(pCurSector);
        sithCogExec_PushInt(pCog, count);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorCount(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithWorld_pCurrentWorld->numSectors);
}

void J3DAPI sithCogFunctionSector_GetSectorCenter(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
#line 718
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector passed to GetSectorCenter.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pSector->center);
    }
}

void J3DAPI sithCogFunctionSector_GetNumSectorVertices(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
#line 747
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector passed to GetNumSectorVertices.\n", pCog->aName, 0, 0, 0);

        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSector->numVertices);
    }
}

void J3DAPI sithCogFunctionSector_GetNumSectorSurfaces(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
#line 776
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector passed to GetNumSectorSurfaces.\n", pCog->aName, 0, 0, 0);

        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, pSector->numSurfaces);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorVertexPos(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int vertNum          = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);

    if ( !pSector )
    {
        #line 809
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector passed to GetSectorVertexPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    if ( vertNum >= pSector->numVertices || vertNum < 0 )
    {
        #line 816
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad index passed to GetSectorVertexPos.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
    }
    else
    {
        sithCogExec_PushVector(pCog, &pWorld->aVertices[pSector->aVertIdxs[vertNum]]);
    }
}

void J3DAPI sithCogFunctionSector_GetSectorSurfaceRef(SithCog* pCog)
{
    int surfNum         = sithCogExec_PopInt(pCog);
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        #line 849
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector passed to GetSectorSurfaceRef.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( surfNum >= pSector->numSurfaces || surfNum < 0 )
    {
        #line 856
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad index passed to GetSectorSurfaceRef.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        sithCogExec_PushInt(pCog, sithSurface_GetSurfaceIndex(&pSector->pFirstSurface[surfNum]));
    }
}

void J3DAPI sithCogFunctionSector_SyncSector(SithCog* pCog)
{
    SithSector* pSector = sithCogExec_PopSector(pCog);
    if ( !pSector )
    {
        #line 883
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector in SyncSector().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithSector_SyncSector(pSector, 1);
}

void J3DAPI sithCogFunctionSector_FindSectorAtPos(SithCog* pCog)
{
    SithSector* pFoundSector;

    rdVector3 pos;
    sithCogExec_PopVector(pCog, &pos);
    pFoundSector = sithSector_FindSectorAtPos(sithWorld_pCurrentWorld, &pos);

    if ( pFoundSector )
    {
        sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pFoundSector));
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}
