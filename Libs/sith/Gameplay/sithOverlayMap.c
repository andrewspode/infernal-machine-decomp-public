#include "sithOverlayMap.h"

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPrimit2.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithConsole.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdColorApi.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>

#include <w32util/wuRegistry.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithOverlayMap_countfit_1, 30)
CF_PROTO(sithOverlayMap_countfit_2, 30)
CF_PROTO(sithOverlayMap_countfit_3, 30)
CF_PROTO(sithOverlayMap_countfit_4, 30)
CF_PROTO(sithOverlayMap_countfit_5, 30)
CF_PROTO(sithOverlayMap_countfit_6, 30)
CF_PROTO(sithOverlayMap_countfit_7, 30)
CF_PROTO(sithOverlayMap_countfit_8, 30)
CF_PROTO(sithOverlayMap_countfit_9, 30)
CF_PROTO(sithOverlayMap_countfit_10, 30)
CF_PROTO(sithOverlayMap_countfit_11, 30)
CF_PROTO(sithOverlayMap_countfit_12, 30)
CF_PROTO(sithOverlayMap_countfit_13, 30)
CF_PROTO(sithOverlayMap_countfit_14, 30)
CF_PROTO(sithOverlayMap_countfit_15, 30)
CF_PROTO(sithOverlayMap_countfit_16, 30)
CF_PROTO(sithOverlayMap_countfit_17, 30)
CF_PROTO(sithOverlayMap_countfit_18, 30)
CF_PROTO(sithOverlayMap_countfit_19, 18)
/* end count-fit */
#define SITHOVERLAYMAP_NUMZLEVELS 9u

// Retail's private .bss block is 0x0055E8A0-0x0055E9B4 in this declaration
// order. Every static is explicitly zero-initialized: VC5 emits uninitialized
// file statics first in identifier-keyed order and zero-initialized
// definitions after them in declaration order. The
// 4-byte hole at 0x0055E8D4 is 8-byte alignment before aIndyIcons.
static rdMatrix34 sithOverlayMap_mapOrient = { 0 };
static SithThing* sithOverlayMap_pLocalPlayer = NULL;
static rdMaterial* sithOverlayMap_aIndyIcons[10] = { 0 }; // Added: Init to 0
static rdMaterial* sithOverlayMap_aMarkIcons[10] = { 0 }; // Added: Init to 0
static rdVector2 sithOverlayMap_canvasPos = { 0 };
static SithOverlayMapConfig sithOverlayMap_config = { 0 };
static SithWorld* sithOverlayMap_pCurWorld = NULL;
static rdMaterial* sithOverlayMap_aHintIcons[10] = { 0 }; // Added: Init to 0
static float sithOverlayMap_vertexScale = 0.0f;
static int sithOverlayMap_curHintNum = 0;
int sithOverlayMap_bMapVisible = 0; // Added: Init to false
static int sithOverlayMap_bNoSeenHintsCount = 0; // Added: Init to false
static size_t sithOverlayMap_curHintIconNum = 0;   // Is inited in open
static size_t sithOverlayMap_curIndyIconNum = 0;
static size_t sithOverlayMap_curMarkIconNum = 0;
static int sithOverlayMap_bOpened = 0; // Added: Init to false
static int sithOverlayMap_bShowHints = 0; // Added: Init to false
static int sithOverlayMap_iconBlinkStep = 0;
static uint32_t sithOverlayMap_msecLastIconBlinkUpdate = 0;

// Easter egg console command - "06/15/97"
static char sithOverlayMap_aEncEasterEggCmd[8] = { '\0', '\x06', '\xFF', '\x01', '\x05', '\xFF', '\t', '\a' };

static float sithOverlayMap_curScale = 75.0f;

static const char* sithOverlayMap_aHintIconNames[1] = { "hinticon.mat" };

static const char* sithOverlayMap_aMapIconNames[1]  = { "indyicon.mat" };

// Chalk mark thing icons related vars
static const char* sithOverlayMap_aChalkMarkIconNames[1] = { "gen_4icon_chalk_diamond.mat" };

// Vars used by sithOverlayMap_DrawPlayerIcon; retail keeps it in writable .data at 0x00528850
static rdVector4 sithOverlayMap_playerLightColor = { { 1.0f }, { 1.0f }, { 1.0f }, { 1.0f } };

// Hint thing icon related vars
static const rdVector2 sithOverlayMap_aHintIconVerts[4] = { { -0.075f, -0.075f },  { 0.075f, -0.075f },  { 0.075f, 0.075f },  { -0.075f, 0.075f } };
static const rdVector2 sithOverlayMap_aMarkIconVerts[4] = { { -0.075f, -0.075f },  { 0.075f, -0.075f },  { 0.075f, 0.075f },  { -0.075f, 0.075f } };

// Local player icon related vars
static const rdVector2 sithOverlayMap_aPlayerIconVerts[4] = { { -0.1f, -0.1f }, { 0.1f, -0.1f }, { 0.1f, 0.1f }, { -0.1f, 0.1f } };
static const rdVector2 sithOverlayMap_aPlayerIconUVs[4]   = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

static const rdVector2 sithOverlayMap_aThingIconUVs[4]  = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

// Vars used by sithOverlayMap_DrawThingIcon
static int sithOverlayMap_aIconBlinkUpdateIntervals[10] = { 135, 55, 40, 20, 8, 4, 2, 1, 0, 75 };



void J3DAPI sithOverlayMap_DrawSectors(SithSector* pSector);
int J3DAPI sithOverlayMap_DrawSector(SithSector* pSector);
void J3DAPI sithOverlayMap_DrawPlayerIcon(void);
void J3DAPI sithOverlayMap_DrawThingIcon(const SithThing* pThing, const rdMaterial* pIcon, const rdVector2* aVertices, int bZLevelColor, int bBlink);

int J3DAPI sithOverlayMap_EasterEggConsoleCallback(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithOverlayMap_CanDrawSurfaceEdge(SithSurface* pSurface, int vertIdx, int nextVertIdx);

int J3DAPI sithOverlayMap_EasterEggConsoleCallback(const SithConsoleCommand* pFunc, const char* pArg)
{
    size_t hintCelNum = 0;
    size_t indyCelNum = 0;
    size_t markCelNum = 0;

    hintCelNum = (sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum + 1) % sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->numCels;
    indyCelNum = (sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->curCelNum + 1) % sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->numCels;
    markCelNum = (sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->curCelNum + 1) % sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->numCels;

    sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum = hintCelNum;
    sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->curCelNum = indyCelNum;
    sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->curCelNum = markCelNum;
    return 1;
}

int J3DAPI sithOverlayMap_Open(SithOverlayMapConfig* pConfig)
{
    char aEasterEggCmd[STD_ARRAYLEN(sithOverlayMap_aEncEasterEggCmd) + 4];
    size_t i;
    float scalePercent;

    if ( sithOverlayMap_bOpened )
    {
#line 238
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 0;
    }

    sithOverlayMap_bShowHints          = wuRegistry_GetIntEx("Show Hints", 0);
    sithOverlayMap_config.bMapRotation = wuRegistry_GetIntEx("Map Rotation", 0);

    scalePercent               = D3DVAL((sithOverlayMap_curScale - 10.0f) / 290.0f);
    sithOverlayMap_vertexScale = D3DMultiply(scalePercent, 200.0f) + 110.0f;

    sithOverlayMap_config.aZLevelBounds = NULL;
    sithOverlayMap_config.aZLevelColors = NULL;

    if ( pConfig )
    {
        memcpy(&sithOverlayMap_config, pConfig, sizeof(sithOverlayMap_config));
    }
    else
    {
        // Init map geometry Z-levels

        sithOverlayMap_config.numZLevels    = SITHOVERLAYMAP_NUMZLEVELS;
#line 260
        sithOverlayMap_config.aZLevelBounds = (float*)STDMALLOC(sizeof(float) * sithOverlayMap_config.numZLevels);
        if ( !sithOverlayMap_config.aZLevelBounds )
        {
            goto error;
        }

        sithOverlayMap_config.aZLevelBounds[0] = -1.0f;
        sithOverlayMap_config.aZLevelBounds[1] = -0.8f;
        sithOverlayMap_config.aZLevelBounds[2] = -0.4f;
        sithOverlayMap_config.aZLevelBounds[3] = -0.2f;
        sithOverlayMap_config.aZLevelBounds[4] = -0.1f;
        sithOverlayMap_config.aZLevelBounds[5] =  0.2f;
        sithOverlayMap_config.aZLevelBounds[6] =  0.4f;
        sithOverlayMap_config.aZLevelBounds[7] =  0.8f;
        sithOverlayMap_config.aZLevelBounds[8] =  1.0f;

#line 274
        sithOverlayMap_config.aZLevelColors = (D3DCOLOR*)STDMALLOC(sizeof(*sithOverlayMap_config.aZLevelColors) * sithOverlayMap_config.numZLevels);
        if ( !sithOverlayMap_config.aZLevelColors )
        {
            goto error;
        }

        // Init geometry Z-level colors
        sithOverlayMap_config.aZLevelColors[0] = STD_RGB(0, 38, 176);    // 0xFF0026B0
        sithOverlayMap_config.aZLevelColors[1] = STD_RGB(18, 92, 170);   // 0xFF125CAA
        sithOverlayMap_config.aZLevelColors[2] = STD_RGB(51, 194, 158);  // 0xFF33C29E
        sithOverlayMap_config.aZLevelColors[3] = STD_RGB(145, 225, 92);  // 0xFF91E15C
        sithOverlayMap_config.aZLevelColors[4] = STD_RGB(255, 255, 30);  // 0xFFFFFF1E
        sithOverlayMap_config.aZLevelColors[5] = STD_RGB(232, 180, 21);  // 0xFFE8B415
        sithOverlayMap_config.aZLevelColors[6] = STD_RGB(210, 142, 24);  // 0xFFD28E18
        sithOverlayMap_config.aZLevelColors[7] = STD_RGB(201, 80, 15);   // 0xFFC9500F
        sithOverlayMap_config.aZLevelColors[8] = STD_RGB(194, 0, 0);     // 0xFFC20000

        // Init thing map color
        sithOverlayMap_config.playerBoundsColor    = STD_RGB(255, 15, 0);   // 0xFFFF0F00
        sithOverlayMap_config.playerDirectionColor = STD_RGB(0, 255, 0);    // 0xFF00FF00
        sithOverlayMap_config.actorBoundsColor     = STD_RGB(127, 127, 15); // 0xFF7F7F0F
        sithOverlayMap_config.actorDirectionColor  = STD_RGB(15, 127, 15);  // 0xFF0F7F0F
        sithOverlayMap_config.itemBoundsColor      = STD_RGB(0, 0, 255);    // 0xFF0000FF
        sithOverlayMap_config.weaponBoundsColor    = STD_RGB(0, 0, 119);    // 0xFF000077
        sithOverlayMap_config.defaultBoundsColor   = STD_RGB(51, 51, 255);  // 0xFF3333FF
    }

    memset(sithOverlayMap_aHintIcons, 0, sizeof(sithOverlayMap_aHintIcons));
    memset(sithOverlayMap_aIndyIcons, 0, sizeof(sithOverlayMap_aIndyIcons));

    for ( i = 0; i < STD_ARRAYLEN(sithOverlayMap_aHintIconNames); i++ )
    {
        sithOverlayMap_aHintIcons[i] = sithMaterial_Load(sithOverlayMap_aHintIconNames[i]);
    }

    // Init map icons
    for ( i = 0; i < STD_ARRAYLEN(sithOverlayMap_aMapIconNames); i++ )
    {
        sithOverlayMap_aIndyIcons[i] = sithMaterial_Load(sithOverlayMap_aMapIconNames[i]);
    }

    for ( i = 0; i < STD_ARRAYLEN(sithOverlayMap_aChalkMarkIconNames); i++ )
    {
        sithOverlayMap_aMarkIcons[i] = sithMaterial_Load(sithOverlayMap_aChalkMarkIconNames[i]);
    }

    sithOverlayMap_curHintIconNum = 0;
    sithOverlayMap_curIndyIconNum = 0;
    sithOverlayMap_curMarkIconNum = 0;

    sithOverlayMap_UpdateHints();

    sithOverlayMap_bOpened = 1;

    // Init Easter egg command
    for ( i = 0; i < STD_ARRAYLEN(sithOverlayMap_aEncEasterEggCmd); ++i )
    {
        aEasterEggCmd[i] = sithOverlayMap_aEncEasterEggCmd[i] + '0';
    }

    aEasterEggCmd[STD_ARRAYLEN(sithOverlayMap_aEncEasterEggCmd)] = 0;

    sithConsole_RegisterCommand(sithOverlayMap_EasterEggConsoleCallback, aEasterEggCmd, 0);
    return 0;

error:
    sithOverlayMap_Close();
    return 1;
}

void J3DAPI sithOverlayMap_Close(void)
{
    if ( !sithOverlayMap_bOpened )
    {
#line 366
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
        return;
    }

    sithOverlayMap_bMapVisible = 0;

    if ( sithOverlayMap_config.aZLevelBounds )
    {
        stdMemory_Free(sithOverlayMap_config.aZLevelBounds);
    }
    sithOverlayMap_config.aZLevelBounds = NULL;

    if ( sithOverlayMap_config.aZLevelColors )
    {
        stdMemory_Free(sithOverlayMap_config.aZLevelColors);
    }
    sithOverlayMap_config.aZLevelColors = NULL;

    sithOverlayMap_bOpened = 0;
}

int J3DAPI sithOverlayMap_IsMapVisible(void)
{
    return sithOverlayMap_bMapVisible;
}

int J3DAPI sithOverlayMap_ToggleMap(void)
{
    sithOverlayMap_bMapVisible = !sithOverlayMap_bMapVisible; /* retail bug: no return value; the debug build is the same */
}

void J3DAPI sithOverlayMap_ZoomIn(void)
{
    float scalePercent;

    if ( sithOverlayMap_bMapVisible )
    {
        sithOverlayMap_curScale += J3DMAX(D3DMultiply((sithOverlayMap_curScale - 10.0f) / 290.0f, 20.0f), 1.0f);
        sithOverlayMap_curScale = J3DMIN(sithOverlayMap_curScale, 300.0f);

        scalePercent = D3DVAL((sithOverlayMap_curScale - 10.0f) / 290.0f);
        sithOverlayMap_vertexScale = D3DMultiply(scalePercent, 200.0f) + 110.0f;
    }
}

void J3DAPI sithOverlayMap_ZoomOut(void)
{
    float scalePercent;

    if ( sithOverlayMap_bMapVisible )
    {
        sithOverlayMap_curScale -= J3DMAX(D3DMultiply((sithOverlayMap_curScale - 10.0f) / 290.0f, 20.0f), 1.0f);
        sithOverlayMap_curScale = J3DMAX(sithOverlayMap_curScale, 10.0f);

        scalePercent = D3DVAL((sithOverlayMap_curScale - 10.0f) / 290.0f);
        sithOverlayMap_vertexScale = D3DMultiply(scalePercent, 200.0f) + 110.0f;
    }
}

void J3DAPI sithOverlayMap_Draw(void)
{
    SithThing* pCurrentHintThing;
    int i;
    rdVector3 pyr;
    uint32_t hintval;

    if ( !sithOverlayMap_bMapVisible )
    {
        return;
    }

    // Prepare render state
    sithAdvanceRenderTick();
    rdPrimit2_Reset(); // Will switch to wireframe mode

    sithOverlayMap_pCurWorld    = sithWorld_pCurrentWorld;
    sithOverlayMap_pLocalPlayer = sithOverlayMap_pCurWorld->pLocalPlayer;
    sithOverlayMap_canvasPos.x  = sithCamera_pCurCamera->rdCamera.pCanvas->center.x;
    sithOverlayMap_canvasPos.y  = sithCamera_pCurCamera->rdCamera.pCanvas->center.y;

    // Update map orientation
    pyr.pitch = 0.0f;
    pyr.yaw   = -stdMath_ArcTan4(sithPlayer_pLocalPlayerThing->orient.lvec.y, sithPlayer_pLocalPlayerThing->orient.lvec.x);
    pyr.roll  = 0.0f;
    rdMatrix_BuildRotate34(&sithOverlayMap_mapOrient, &pyr);

    // Draw sectors
    sithOverlayMap_DrawSectors(sithOverlayMap_pLocalPlayer->pInSector);

    // Draw map hints
    for ( i = 0; i < sithFX_numChalkMarks; ++i )
    {
        sithOverlayMap_DrawThingIcon(
            sithFX_aChalkMarks[i],
            sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum],
            sithOverlayMap_aMarkIconVerts,
            sithOverlayMap_aMarkIcons[sithOverlayMap_curMarkIconNum]->curCelNum == 0, // bZLevelColor
            /*bBlink=*/0
        );
    }

    // Set cur hint thing
    if ( sithOverlayMap_curHintNum != -1 )
    {
        pCurrentHintThing = &sithWorld_pCurrentWorld->aThings[sithOverlayMap_curHintNum];
    }
    else
    {
        pCurrentHintThing = NULL;
    }

    // Update & draw cur hint thing
    if ( sithOverlayMap_bShowHints && sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum] && pCurrentHintThing )
    {
        hintval = (int32_t)pCurrentHintThing->userval | SITH_HINT_SEEN;
        if ( (hintval & SITH_HINT_SOLVED) != 0 )
        {
            sithOverlayMap_UpdateHints();
            if ( pCurrentHintThing )
            {
                hintval = (int32_t)pCurrentHintThing->userval | SITH_HINT_SEEN;
            }
        }

        if ( pCurrentHintThing )
        {
            pCurrentHintThing->userval = D3DVAL(hintval);
            sithThing_SyncThing(pCurrentHintThing, SITHTHING_SYNC_STATE);

            sithOverlayMap_DrawThingIcon(
                pCurrentHintThing,
                sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum],
                sithOverlayMap_aHintIconVerts,
                sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum == 0, // bZLevelColor
                sithOverlayMap_aHintIcons[sithOverlayMap_curHintIconNum]->curCelNum == 0  // bBlink
            );
        }
    }

    // Now draw player on top
    sithOverlayMap_DrawPlayerIcon();
}

void J3DAPI sithOverlayMap_SetShowHints(int bShow)
{
    sithOverlayMap_bShowHints = bShow;
}

void J3DAPI sithOverlayMap_EnableMapRotation(int bEnable)
{
    sithOverlayMap_config.bMapRotation = bEnable;
}

int J3DAPI sithOverlayMap_GetMapRotation(void)
{
    return sithOverlayMap_config.bMapRotation;
}

int J3DAPI sithOverlayMap_GetShowHints(void)
{
    return sithOverlayMap_bShowHints;
}

int J3DAPI sithOverlayMap_UpdateHints(void)
{
    int maxHintValue = -1;
    int32_t hintval;
    int i;
    int curHintNum = -1;

    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    for ( i = 0; i <= sithWorld_pCurrentWorld->lastThingIdx; ++i )
    {
        if ( sithWorld_pCurrentWorld->aThings[i].type == SITH_THING_HINT )
        {
            hintval = (int32_t)sithWorld_pCurrentWorld->aThings[i].userval;
            if ( (hintval & SITH_HINT_SOLVED) != 0 )
            {
                continue;
            }

            hintval &= ~(SITH_HINT_SOLVED | SITH_HINT_SEEN); // Remove flags to extract hint value
            if ( (size_t)hintval < (size_t)maxHintValue ) // Note: Don't remove casting
            {
                maxHintValue = hintval;
                curHintNum = i;
            }
        }
    }

    sithOverlayMap_curHintNum = curHintNum;
    if ( curHintNum == -1 )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithOverlayMap_SetHintSolved(SithThing* pThing)
{
    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    if ( pThing )
    {
        pThing->userval = D3DVAL((int32_t)pThing->userval | SITH_HINT_SOLVED);
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }

    return sithOverlayMap_UpdateHints();
}

int J3DAPI sithOverlayMap_SetHintUnsolved(SithThing* pThing)
{
    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    if ( pThing )
    {
        pThing->userval = D3DVAL((int32_t)pThing->userval & ~SITH_HINT_SOLVED);
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }

    return sithOverlayMap_UpdateHints();
}

/* Retail's linker discarded this function (no retail symbol, address or caller). Its evidence is the
   /Gf literal order: retail's only "on" (0x005288F4) is the last literal of this object's run, after
   its J3D_FILE, although its only surviving user is sithCommand_DebugMode in a later object; and
   bNoSeenHintsCount, which GetNumSeenHints reads, is written by no surviving code. This console
   callback is in the debug build here; the body is the debug build's (0x0042F9C4, byte-identical
   under /Od). Name and parameters inferred. */
int J3DAPI sithOverlayMap_NoSeenHintsCountConsoleCallback(const SithConsoleCommand* pFunc, const char* pArg)
{
    if ( !pArg )
    {
        return 0;
    }

    if ( streqi(pArg, "on") )
    {
        sithOverlayMap_bNoSeenHintsCount = 1;
    }
    else
    {
        sithOverlayMap_bNoSeenHintsCount = 0;
    }

    return 1;
}

void J3DAPI sithOverlayMap_GetNumSeenHints(int* pOutNumSeenHints, int* pOutNumHints)
{
    int numHintsSeenCnt = 0;
    int totalNumHints = 0;
    int i;
    int32_t hintval;

    if ( !sithWorld_pCurrentWorld )
    {
        return;
    }

    for ( i = 0; i <= sithWorld_pCurrentWorld->lastThingIdx; ++i )
    {
        if ( sithWorld_pCurrentWorld->aThings[i].type == SITH_THING_HINT )
        {
            hintval = (int32_t)sithWorld_pCurrentWorld->aThings[i].userval;
            if ( (hintval & SITH_HINT_SEEN) != 0 )
            {
                ++numHintsSeenCnt;
            }

            ++totalNumHints;
        }
    }

    if ( sithOverlayMap_bNoSeenHintsCount )
    {
        numHintsSeenCnt = 0;
    }

    *pOutNumSeenHints = numHintsSeenCnt;
    *pOutNumHints     = totalNumHints;
}

/* Retail's linker discarded this function (no retail symbol, address or caller). The debug build has it here,
   between GetNumSeenHints and DrawSectors (0x0042FACC): the hint thing when a hint is current, and no return value
   on the other path (the /Od code falls through to the epilogue with eax unset). Name inferred
   text; nothing in the debug image calls it. */
SithThing* J3DAPI sithOverlayMap_GetCurrentHintThing(void)
{
    if ( sithOverlayMap_curHintNum != -1 )
    {
        return &sithWorld_pCurrentWorld->aThings[sithOverlayMap_curHintNum];
    }
}

void J3DAPI sithOverlayMap_DrawSectors(SithSector* pSector)
{
    int bRenderAdjoinSectors;
    SithSurfaceAdjoin* pCurAdjoin;

    if ( pSector->renderTick != sithMain_curRenderTick )
    {
        if ( (pSector->flags & SITH_SECTOR_SEEN) == 0 && (sith_mode.mapModeFlags & SITHMAPMODE_SHOWALLSECTORS) == 0 )
        {
            return;
        }

        pSector->renderTick = sithMain_curRenderTick;

        if ( (pSector->flags & SITH_SECTOR_HIDEONMAP) == 0 )
        {
            bRenderAdjoinSectors = sithOverlayMap_DrawSector(pSector);
        }
        else
        {
            bRenderAdjoinSectors = 1;
        }

        if ( bRenderAdjoinSectors )
        {
            pCurAdjoin = pSector->pFirstAdjoin;
            while ( pCurAdjoin )
            {
                sithOverlayMap_DrawSectors(pCurAdjoin->pAdjoinSector);
                pCurAdjoin = pCurAdjoin->pNextAdjoin;
            }
        }
    }
}

int J3DAPI sithOverlayMap_DrawSector(SithSector* pSector)
{
    int bSectorDrawn = 0;
    int bDrawing = 0;

    SithSurface* pSurf;
    size_t surfNum;
    size_t vecNum;
    size_t vertIdx;
    size_t nextVertIdx;
    int edgeClipState;
    rdVector3 vert;
    rdVector3 nextVert;
    int bVertInBounds;
    size_t layer;
    uint32_t color;
    uint32_t linePattern;
    rdVector2 pos1;
    rdVector2 pos2;
    rdVector3 secCenter;
    float radius;
    SithThing* pCurThing;
    int bDrawThing;
    uint32_t dirLineColor;

    // Draw level sectors
    pSurf = pSector->pFirstSurface;
    for ( surfNum = 0; surfNum < pSector->numSurfaces; ++surfNum )
    {
        if ( (pSurf->flags & SITH_SURFACE_ISFLOOR) != 0 )
        {
            for ( vecNum = 0; vecNum < pSurf->face.numVertices; ++vecNum )
            {
                vertIdx     = pSurf->face.aVertices[vecNum];
                nextVertIdx = pSurf->face.aVertices[(vecNum + 1) % pSurf->face.numVertices];

                edgeClipState = sithOverlayMap_CanDrawSurfaceEdge(pSurf, vertIdx, nextVertIdx);
                if ( edgeClipState )
                {
                    bDrawing = 1;

                    rdVector_Copy3(&vert, &sithOverlayMap_pCurWorld->aVertices[vertIdx]);
                    rdVector_Copy3(&nextVert, &sithOverlayMap_pCurWorld->aVertices[nextVertIdx]);

                    rdVector_Sub3Acc(&vert, &sithOverlayMap_pLocalPlayer->pos);
                    rdVector_Sub3Acc(&nextVert, &sithOverlayMap_pLocalPlayer->pos);

                    if ( sithOverlayMap_config.bMapRotation )
                    {
                        rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                        rdMatrix_TransformVector34Acc(&nextVert, &sithOverlayMap_mapOrient);
                    }

                    bVertInBounds = 0;
                    for ( layer = 0; layer < sithOverlayMap_config.numZLevels - 1; ++layer )
                    {
                        if ( vert.z >= sithOverlayMap_config.aZLevelBounds[layer]
                            && vert.z < sithOverlayMap_config.aZLevelBounds[layer + 1] )
                        {
                            bVertInBounds = 1;
                            break;
                        }
                    }

                    if ( bVertInBounds )
                    {
                        color       = sithOverlayMap_config.aZLevelColors[layer];
                        linePattern = 0xFFFFFFFF;
                    }
                    else
                    {
                        if ( edgeClipState == 2 )
                        {
                            continue;
                        }

                        if ( vert.z < sithOverlayMap_config.aZLevelBounds[0] )
                        {
                            layer = 0;
                        }
                        else
                        {
                            layer = sithOverlayMap_config.numZLevels - 1;
                        }

                        color       = sithOverlayMap_config.aZLevelColors[layer];
                        linePattern = 0xCCCCCCCC;
                    }

                    rdVector_Scale3Acc(&vert, sithOverlayMap_curScale);
                    rdVector_Scale3Acc(&nextVert, sithOverlayMap_curScale);

                    pos1.x = vert.x;
                    pos1.y = -vert.y;
                    pos2.x = nextVert.x;
                    pos2.y = -nextVert.y;

                    pos1.x += sithOverlayMap_canvasPos.x;
                    pos1.y += sithOverlayMap_canvasPos.y;
                    pos2.x += sithOverlayMap_canvasPos.x;
                    pos2.y += sithOverlayMap_canvasPos.y;

                    if ( rdPrimit2_DrawClippedLine2(pos1.x, pos1.y, pos2.x, pos2.y, color) )
                    {
                        bSectorDrawn = 1;
                    }
                }
            }
        }

        pSurf->renderTick = sithMain_curRenderTick;
        ++pSurf;
    }

    if ( !bSectorDrawn )
    {
        rdVector_Copy3(&secCenter, &pSector->center);
        rdVector_Scale3Acc(&secCenter, sithOverlayMap_curScale);

        pos1.x = secCenter.x;
        pos1.y = -secCenter.y;
        pos1.x += sithOverlayMap_canvasPos.x;
        pos1.y += sithOverlayMap_canvasPos.y;

        radius = D3DMultiply(pSector->radius, sithOverlayMap_curScale);

        if ( J3DINRANGE(pos1.x, sithCamera_pCurCamera->rdCamera.pCanvas->rect.left - radius, sithCamera_pCurCamera->rdCamera.pCanvas->rect.right + radius)
            || J3DINRANGE(pos1.y, sithCamera_pCurCamera->rdCamera.pCanvas->rect.top - radius, sithCamera_pCurCamera->rdCamera.pCanvas->rect.bottom + radius) )
        {
            bSectorDrawn = 1;
        }
    }

    // Draw Things
    if ( (sith_mode.mapModeFlags & (SITHMAPMODE_SHOWALLTHINGS | SITHMAPMODE_SHOWACTORS | SITHMAPMODE_SHOWPLAYERS)) != 0 )
    {
        pCurThing = pSector->pFirstThingInSector;
        while ( pCurThing )
        {
            if ( pCurThing != sithWorld_pCurrentWorld->pCameraFocusThing && (pCurThing->flags & (SITH_TF_DISABLED | SITH_TF_INVISIBLE | SITH_TF_DESTROYED)) == 0 )
            {
                bDrawThing = 0;

                if ( (sith_mode.mapModeFlags & SITHMAPMODE_SHOWALLTHINGS) != 0 )
                {
                    bDrawThing = 1;
                    color = sithOverlayMap_config.defaultBoundsColor;
                }

                if ( pCurThing->type == SITH_THING_PLAYER )
                {
                    if ( (sith_mode.mapModeFlags & (SITHMAPMODE_SHOWACTORS | SITHMAPMODE_SHOWPLAYERS)) != 0 )
                    {
                        bDrawThing = 1;
                    }

                    color = sithOverlayMap_config.playerBoundsColor;
                    dirLineColor = sithOverlayMap_config.playerDirectionColor;
                }
                else if ( pCurThing->type == SITH_THING_ACTOR )
                {
                    if ( (sith_mode.mapModeFlags & SITHMAPMODE_SHOWACTORS) != 0 )
                    {
                        bDrawThing = 1;
                    }

                    color = sithOverlayMap_config.actorBoundsColor;
                    dirLineColor = sithOverlayMap_config.actorDirectionColor;
                }
                else if ( pCurThing->type == SITH_THING_ITEM )
                {
                    if ( (sith_mode.mapModeFlags & SITHMAPMODE_SHOWITEMS) != 0 )
                    {
                        bDrawThing = 1;
                    }

                    color = sithOverlayMap_config.itemBoundsColor;
                }
                else if ( pCurThing->type == SITH_THING_WEAPON )
                {
                    if ( (sith_mode.mapModeFlags & SITHMAPMODE_SHOWWEAPONS) != 0 )
                    {
                        bDrawThing = 1;
                    }

                    color = sithOverlayMap_config.weaponBoundsColor;
                }
                else
                {
                    color = sithOverlayMap_config.defaultBoundsColor;
                }

                if ( bDrawThing )
                {
                    rdVector_Sub3(&vert, &pCurThing->pos, &sithOverlayMap_pLocalPlayer->pos);
                    if ( sithOverlayMap_config.bMapRotation )
                    {
                        rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                    }

                    rdVector_Scale3Acc(&vert, sithOverlayMap_curScale);

                    // Draw thing on map
                    pos1.x = vert.x;
                    pos1.y = -vert.y;
                    pos1.x += sithOverlayMap_canvasPos.x;
                    pos1.y += sithOverlayMap_canvasPos.y;

                    radius = D3DMultiply(pCurThing->collide.movesize, sithOverlayMap_curScale);
                    rdPrimit2_DrawClippedCircle2(pos1.x, pos1.y, radius, 20.0f, color);

                    // Draw actor / player direction line
                    if ( pCurThing->type == SITH_THING_ACTOR || pCurThing->type == SITH_THING_PLAYER )
                    {
                        rdVector_Scale3(&nextVert, &pCurThing->orient.lvec, D3DMultiply(pCurThing->collide.movesize, 2.0f));
                        rdVector_Add3Acc(&nextVert, &pCurThing->pos);

                        rdVector_Sub3(&vert, &nextVert, &sithOverlayMap_pLocalPlayer->pos);
                        if ( sithOverlayMap_config.bMapRotation )
                        {
                            rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
                        }

                        rdVector_Scale3Acc(&vert, sithOverlayMap_curScale);

                        pos2.x = vert.x;
                        pos2.y = -vert.y;
                        pos2.x += sithOverlayMap_canvasPos.x;
                        pos2.y += sithOverlayMap_canvasPos.y;

                        rdPrimit2_DrawClippedLine2(pos1.x, pos1.y, pos2.x, pos2.y, dirLineColor);
                    }
                }
            }

            pCurThing = pCurThing->pNextThingInSector;
        }
    }

    return bDrawing ? bSectorDrawn : 1;
}

void J3DAPI sithOverlayMap_DrawPlayerIcon(void)
{
    size_t curi;
    rdCacheProcEntry* pPoly;
    LPD3DTLVERTEX pVert;

    if ( !sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum] )
    {
        return;
    }

    rdCache_FlushAlpha();
    pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return;
    }

    pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->pMaterial    = sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum];
    pPoly->matCelNum    = sithOverlayMap_aIndyIcons[sithOverlayMap_curIndyIconNum]->curCelNum;
    pPoly->numVertices  = STD_ARRAYLEN(sithOverlayMap_aPlayerIconVerts);

    for ( curi = 0; curi < pPoly->numVertices; ++curi )
    {
        rdVector3 vert;
        pVert = &pPoly->aVertices[curi];

        vert.x = sithOverlayMap_aPlayerIconVerts[curi].x;
        vert.y = sithOverlayMap_aPlayerIconVerts[curi].y;
        rdVector_Scale2(&vert, &vert, sithOverlayMap_vertexScale);

        if ( !sithOverlayMap_config.bMapRotation )
        {
            rdMatrix_TransformVector34Acc(&vert, &sithOverlayMap_mapOrient);
        }

        vert.x = vert.x + sithOverlayMap_canvasPos.x;
        vert.y = vert.y + sithOverlayMap_canvasPos.y;

        pVert->sx  = vert.x;
        pVert->sy  = vert.y;
        pVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
        pVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;

        pVert->tu = D3DVAL(sithOverlayMap_aPlayerIconUVs[curi].x);
        pVert->tv = D3DVAL(sithOverlayMap_aPlayerIconUVs[curi].y);

        rdVector_Copy4(&pPoly->aVertIntensities[curi], &sithOverlayMap_playerLightColor);
    }

    rdVector_Copy4(&pPoly->extraLight, &sithOverlayMap_playerLightColor);
    rdCache_AddAlphaProcFace(pPoly->numVertices);
    rdCache_FlushAlpha();
}

void J3DAPI sithOverlayMap_DrawThingIcon(const SithThing* pThing, const rdMaterial* pIcon, const rdVector2* aVertices, int bZLevelColor, int bBlink)
{
    rdVector4 lightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    rdVector3 thingPos;
    int bFoundLayer;
    size_t layer;
    D3DCOLOR layerColor;
    rdVector3 pos;
    rdVector2 center;
    rdCacheProcEntry* pPoly;
    size_t i;
    LPD3DTLVERTEX pVert;

    if ( !pThing || !pIcon || !aVertices )
    {
        return;
    }

    rdVector_Copy3(&thingPos, &pThing->pos);
    rdVector_Sub3Acc(&thingPos, &sithOverlayMap_pLocalPlayer->pos);

    if ( bZLevelColor )
    {
        bFoundLayer = 0;
        for ( layer = 0; layer < sithOverlayMap_config.numZLevels - 1; ++layer )
        {
            if ( thingPos.z >= sithOverlayMap_config.aZLevelBounds[layer] && thingPos.z < sithOverlayMap_config.aZLevelBounds[layer + 1] )
            {
                bFoundLayer = 1;
                break;
            }
        }

        if ( !bFoundLayer )
        {
            if ( thingPos.z < sithOverlayMap_config.aZLevelBounds[0] )
            {
                layer = 0;
            }
            else
            {
                layer = sithOverlayMap_config.numZLevels - 1;
            }
        }

        layerColor = sithOverlayMap_config.aZLevelColors[layer];
        lightColor.red   = D3DVAL(D3DVAL(STD_GETRED(layerColor)) / 255.0f);
        lightColor.green = D3DVAL(D3DVAL(STD_GETGREEN(layerColor)) / 255.0f);
        lightColor.blue  = D3DVAL(D3DVAL(STD_GETBLUE(layerColor)) / 255.0f);
    }

    if ( bBlink )
    {
        if ( stdPlatform_GetTimeMsec() > sithOverlayMap_msecLastIconBlinkUpdate + sithOverlayMap_aIconBlinkUpdateIntervals[sithOverlayMap_iconBlinkStep] )
        {
            sithOverlayMap_msecLastIconBlinkUpdate = stdPlatform_GetTimeMsec();
            sithOverlayMap_iconBlinkStep = (sithOverlayMap_iconBlinkStep + 1) % 10;
        }

        lightColor.alpha = D3DVAL(1.0f - D3DVAL(RD_MULTIPLY(D3DVAL(sithOverlayMap_iconBlinkStep), 0.1f))); /* spelling-fit */
        if ( lightColor.alpha <= 0.1f )
        {
            lightColor.alpha = 0.0f;
        }
    }

    rdVector_Sub3(&pos, &pThing->pos, &sithOverlayMap_pLocalPlayer->pos);
    if ( sithOverlayMap_config.bMapRotation )
    {
        rdMatrix_TransformVector34Acc(&pos, &sithOverlayMap_mapOrient);
    }

    rdVector_Scale3Acc(&pos, sithOverlayMap_curScale);

    center.x = pos.x;
    center.y = (-pos.y);
    center.x += sithOverlayMap_canvasPos.x;
    center.y += sithOverlayMap_canvasPos.y;

    rdCache_FlushAlpha();
    pPoly = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return;
    }

    pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->pMaterial    = (rdMaterial*)pIcon;
    pPoly->matCelNum    = pIcon->curCelNum;
    pPoly->numVertices  = 4;

    for ( i = 0; i < pPoly->numVertices; ++i )
    {
        rdVector3 vert;
        pVert = &pPoly->aVertices[i];

        vert.x = aVertices[i].x;
        vert.y = aVertices[i].y;
        rdVector_Scale2(&vert, &vert, sithOverlayMap_vertexScale);
        vert.x += center.x;
        vert.y += center.y;

        pVert->sx  = vert.x;
        pVert->sy  = vert.y;
        pVert->sz  = RD_FIXEDPOINT_RHW_SCALE_X5;
        pVert->rhw = RD_FIXEDPOINT_RHW_SCALE_X5;

        pVert->tu = D3DVAL(sithOverlayMap_aThingIconUVs[i].x);
        pVert->tv = D3DVAL(sithOverlayMap_aThingIconUVs[i].y);

        rdVector_Copy4(&pPoly->aVertIntensities[i], &lightColor);
    }

    rdVector_Copy4(&pPoly->extraLight, &lightColor);

    rdCache_AddAlphaProcFace(pPoly->numVertices);
    rdCache_FlushAlpha();
}

int J3DAPI sithOverlayMap_CanDrawSurfaceEdge(SithSurface* pSurface, int vertIdx, int nextVertIdx)
{
    rdFace* pFace;
    SithSurface* pSurf;
    size_t surfaceIdx;
    size_t j;
    size_t endVertIdx;
    SithSurfaceAdjoin* pAdjoin;
    size_t curVert;
    SithSector* pSec;
    SithSector* pOtherSec;

    pSec     = pSurface->pSector;
    pSurf = pSec->pFirstSurface;
    for ( surfaceIdx = 0; surfaceIdx < pSec->numSurfaces; ++surfaceIdx )
    {
        if ( pSurf != pSurface && pSurf->pAdjoin )
        {
            pAdjoin = pSurf->pAdjoin;
            pFace   = &pAdjoin->pAdjoinSurface->face;
            if ( pFace->pMaterial )
            {
                if ( (pAdjoin->flags & SITH_ADJOIN_VISIBLE) == 0
                    || pFace->geometryMode
                    && (pFace->flags & RD_FF_TEX_TRANSLUCENT) == 0
                    && pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGB )
                {
                    for ( j = 0; j < pFace->numVertices; ++j )
                    {
                        curVert  = pFace->aVertices[j];
                        endVertIdx = pFace->aVertices[(j + 1) % pFace->numVertices];
                        if ( curVert == nextVertIdx && endVertIdx == vertIdx )
                        {
                            if ( (pSurf->flags & SITH_SURFACE_ISFLOOR) != 0 )
                            {
                                return 0;
                            }

                            return 1;
                        }
                    }
                }
            }
        }

        if ( pSurf != pSurface && !pSurf->pAdjoin )
        {
            pFace = &pSurf->face;
            for ( j = 0; j < pFace->numVertices; ++j )
            {
                curVert  = pFace->aVertices[j];
                endVertIdx = pFace->aVertices[(j + 1) % pFace->numVertices];
                if ( curVert == nextVertIdx && endVertIdx == vertIdx )
                {
                    if ( (pSurf->flags & SITH_SURFACE_ISFLOOR) != 0 )
                    {
                        return 0;
                    }

                    return 1;
                }
            }
        }

        ++pSurf;
    }

    pAdjoin = pSec->pFirstAdjoin;
    while ( pAdjoin )
    {
        pOtherSec = pAdjoin->pAdjoinSector;
        pSurf   = pOtherSec->pFirstSurface;
        for ( surfaceIdx = 0; surfaceIdx < pOtherSec->numSurfaces; ++surfaceIdx )
        {
            if ( (pSurf->flags & SITH_SURFACE_ISFLOOR) != 0 )
            {
                pFace = &pSurf->face;
                for ( j = 0; j < pFace->numVertices; ++j )
                {
                    curVert  = pFace->aVertices[j];
                    endVertIdx = pFace->aVertices[(j + 1) % pFace->numVertices];
                    if ( curVert == nextVertIdx && endVertIdx == vertIdx )
                    {
                        return 0;
                    }
                }
            }

            ++pSurf;
        }

        pAdjoin = pAdjoin->pNextAdjoin;
    }

    return 2;
}
