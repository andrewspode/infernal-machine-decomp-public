#include "rdroid.h"

#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Raster/rdCache.h>

#ifdef J3D_DEBUG
#include <stdio.h>
#endif


// The shared constant vectors and identity matrices. Retail keeps them as one const block at
// 0x00509A00-0x00509B18, between rdCamera's and rdCanvas's .rdata, i.e. in this object's slot
// of the link order (rdCamera -> rdroid -> rdCanvas). The 2D, 4D and 4x4 entries are never
// referenced; their names follow Jedi Knight's rdroid API where it has them, the unreferenced
// 4D vectors carry the project's placeholder names.
const rdVector2 rdroid_zeroVector2 = { 0.0f, 0.0f };
const rdVector2 rdroid_xVector2    = { 1.0f, 0.0f };
const rdVector2 rdroid_yVector2    = { 0.0f, 1.0f };

const rdVector3 rdroid_zeroVector3 = { 0.0f, 0.0f, 0.0f };
const rdVector3 rdroid_xVector3    = { 1.0f, 0.0f, 0.0f };
const rdVector3 rdroid_yVector3    = { 0.0f, 1.0f, 0.0f };
const rdVector3 rdroid_zVector3    = { 0.0f, 0.0f, 1.0f };

const rdVector4 rdroid_aUnused_509A58 = { 0.0f, 0.0f, 0.0f, 0.0f };
const rdVector4 rdroid_xVector4 = { 1.0f, 0.0f, 0.0f, 0.0f };
const rdVector4 rdroid_yVector4 = { 0.0f, 1.0f, 0.0f, 0.0f };
const rdVector4 rdroid_zVector4 = { 0.0f, 0.0f, 1.0f, 0.0f };
const rdVector4 rdroid_wVector4 = { 0.0f, 0.0f, 0.0f, 1.0f };

const rdMatrix34 rdroid_identMatrix34 = {
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f }
};

const rdMatrix44 rdroid_identMatrix44 = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

// Tentative definitions (COMMON requests). Retail's common tail places them between rdCamera's and
// rdModel3's commons, at this object's pull position, in ascending identifier-hash bucket (42, 61,
// 105, 767) as one object's commons are; Jedi Knight's tail has the same four names in the same
// order inside its run of rdroid_cur* commons.
rdLightMode rdroid_curLightingMode;
tHostServices* rdroid_pHS;
rdGeometryMode rdroid_curGeometryMode;
rdRoidFlags rdroid_curRenderOptions;

static int rdroid_bRDroidStartup;
static int rdroid_bRDroidOpen;

#ifdef J3D_DEBUG
/* Debug-only (debug 0x00506870, the object's first function; no caller in the debug image): appends a line to a
   debug log file. The name is inferred. */
void J3DAPI rdroid_DebugFilePrint(const char* pText)
{
    FILE* fp;

    fp = fopen("d:\\jdebug.txt", "a+");
    if ( fp )
    {
        fputs(pText, fp);
        fclose(fp);
    }
}
#endif

void J3DAPI rdSetServices(tHostServices* pHS)
{
    rdroid_pHS = pHS;
}

void J3DAPI rdClearServices(void)
{
    rdroid_pHS = NULL;
}

void J3DAPI rdStartup(void)
{
    if ( rdroid_bRDroidStartup ) {
#line 130
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: rdStartup() has already been called!\n", 0, 0, 0, 0);
        // TODO: should return with error
    }

    rdCache_Startup();
    rdFont_Startup();
    rdroid_bRDroidStartup = 1;
}

void J3DAPI rdShutdown(void)
{
    if ( rdroid_bRDroidStartup != 1 ) {
        return;
    }
    rdFont_Shutdown();
    rdroid_bRDroidStartup = 0;
}

int J3DAPI rdOpen(void)
{
    if ( !rdroid_bRDroidStartup ) {
        return 0;
    }

    if ( rdroid_bRDroidOpen )
    {
#line 191
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 1;
    }

    if ( rdFont_Open() ) {
        return 0;
    }

    rdroid_curGeometryMode  = RD_GEOMETRY_FULL;
    rdroid_curLightingMode  = RD_LIGHTING_GOURAUD;
    rdroid_curRenderOptions = RDROID_BACKFACE_CULLING_ENABLED;
    rdroid_bRDroidOpen        = 1;
    return 1;
}

void J3DAPI rdClose(void)
{
    if ( rdroid_bRDroidStartup != 1 ) {
        return;
    }
    if ( rdroid_bRDroidOpen ) {
        rdFont_Close();
    }
    rdroid_bRDroidOpen = 0;
}

void J3DAPI rdSetRenderOptions(rdRoidFlags options)
{
    rdroid_curRenderOptions = options;
}

void J3DAPI rdSetGeometryMode(rdGeometryMode mode)
{
    rdroid_curGeometryMode = mode;
}

void J3DAPI rdSetLightingMode(rdLightMode mode)
{
    rdroid_curLightingMode = mode;
}

rdRoidFlags J3DAPI rdGetRenterOptions(void)
{
    return rdroid_curRenderOptions;
}

/* Discarded by the retail linker (no callers in either build): the debug build has the two getters after
   rdGetRenterOptions (0x00506A11 geometry mode, 0x00506A1B lighting mode). Names from Jones3DRE. */
rdGeometryMode J3DAPI rdGetGeometryMode(void)
{
    return rdroid_curGeometryMode;
}

rdLightMode J3DAPI rdGetLightingMode(void)
{
    return rdroid_curLightingMode;
}
