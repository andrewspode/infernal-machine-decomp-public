#include "jonesCog.h"
#include <Jones3D/Display/JonesHudConstants.h>
#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Main/JonesLevel.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Play/jonesInventory.h>

#include <sith/AI/sithAI.h>
#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>

static int jonesCog_curCutsceneType     = -1;
static int jonesCog_bInterfaceEnabled   = -1;
static int jonesCog_bOverlayMapVisible = 0;
static int jonesCog_bStartup           = 0;

int jonesCog_bShowHealthHUD;
int jonesCog_bMenuVisible;
int jonesCog_bEnableGamesave;
int jonesCog_dword_17EFFCC;

int J3DAPI jonesCog_RegisterFunctions(void);
void J3DAPI jonesCog_HideHealthDisplay(SithCog* pCog);
void J3DAPI jonesCog_ShowHealthDisplay(SithCog* pCog);
void J3DAPI jonesCog_EnableInterfaceFunc(SithCog* pCog);
void J3DAPI jonesCog_EndLevel(void);
void J3DAPI jonesCog_EndLevelFunc(SithCog* pCog);
void J3DAPI jonesCog_ExitToShell(SithCog* pCog);
void J3DAPI jonesCog_StartCutsceneFunc(SithCog* pCog);
void J3DAPI jonesCog_EndCutscene(SithCog* pCog);
void J3DAPI jonesCog_UpdateDifficulty(SithCog* pCog);
void J3DAPI jonesCog_InventoryItemChanged(SithCog* pCog);

int J3DAPI jonesCog_Startup(void)
{
    if ( jonesCog_bStartup ) {
        return 1;
    }

    if ( jonesCog_RegisterFunctions() ) {
        return 1;
    }

    JonesMain_UpdateLevelNum();
    jonesCog_bOverlayMapVisible = 0;
    jonesCog_curCutsceneType    = -1;
    jonesCog_bStartup           = 1;
    return 0;
}

void J3DAPI jonesCog_Shutdown(void)
{
    jonesCog_bStartup = 0;
}

int J3DAPI jonesCog_RegisterFunctions(void)
{
    int bFailed;
    SithCogSymbolTable* pSymbolTable = sithCog_pSymbolTable;

    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_HideHealthDisplay, "healthdisplayoff");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_ShowHealthDisplay, "healthdisplayon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_InventoryItemChanged, "jonesinvitemchanged");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_EndLevelFunc, "jonesendlevel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_ExitToShell, "exittoshell");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_StartCutsceneFunc, "startcutscene");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_EndCutscene, "endcutscene");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_UpdateDifficulty, "updatedifficulty");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pSymbolTable, jonesCog_EnableInterfaceFunc, "enableinterface");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI jonesCog_HideHealthDisplay(SithCog* pCog)
{
    jonesCog_bShowHealthHUD = 0;
    JonesHud_SetFadeHealthHUD(jonesCog_bShowHealthHUD, 1);
}

void J3DAPI jonesCog_ShowHealthDisplay(SithCog* pCog)
{
    jonesCog_bShowHealthHUD = 1;
    JonesHud_SetFadeHealthHUD(jonesCog_bShowHealthHUD, 1);
}

void J3DAPI jonesCog_InventoryItemChanged(SithCog* pCog)
{
    int bin = sithCogExec_PopInt(pCog);
    JonesHud_InventoryItemChanged(bin);
}

void J3DAPI jonesCog_EndLevelFunc(SithCog* pCog)
{
    jonesCog_EndLevel();
}

void J3DAPI jonesCog_EndLevel(void)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    if ( jonesConsole_bVisible ) {
        JonesConsole_HideConsole();
    }

    jonesInventory_AdvanceStatistics();

    ++sithPlayer_curLevelNum;
    jonesCog_bShowHealthHUD = 0;

    if ( sithOverlayMap_IsMapVisible() ) {
        sithOverlayMap_ToggleMap();
    }

    sithSoundMixer_StopAll();

    if ( sithPlayer_curLevelNum < JONESLEVEL_BONUSLEVELNUM ) // If cur level is below Peru
    {
        if ( JonesHud_ShowLevelCompleted() )
        {
            JonesMain_SetBonusLevel();
            sithPlayer_bBonusMapBought = 1;
        }
        else if ( sithPlayer_curLevelNum + 1 == JONESLEVEL_BONUSLEVELNUM && !sithPlayer_bBonusMapBought )
        {
            sithSoundMixer_StopAll();
            JonesMain_ShowEndCredits();
        }
        else
        {
            JonesMain_NextLevel();
        }
    }
    else
    {
        JonesMain_ShowEndCredits();
    }
}

void J3DAPI jonesCog_ExitToShell(SithCog* pCog)
{
    JonesMain_ShowEndCredits();
}

void J3DAPI jonesCog_StartCutsceneFunc(SithCog* pCog)
{
    int type;
    SithThing* pCurrentThing;
    int thingNum;

    SithWorld* pWorld = sithWorld_pCurrentWorld;
#line 296
    STD_ASSERTREL(pWorld != ((void *)0));
#line 170

    if ( pCog )
    {
        type = sithCogExec_PopInt(pCog);
        if ( jonesCog_bMenuVisible )
        {
            JonesHud_CutsceneStart(1);
            jonesCog_curCutsceneType = type;
            return;
        }
    }
    else
    {
        if ( jonesCog_curCutsceneType > -1 )
        {
            type = jonesCog_curCutsceneType;
            jonesCog_curCutsceneType = -1;
        }
        else
        {
            return;
        }
    }

    jonesCog_bOverlayMapVisible = sithOverlayMap_IsMapVisible();
    if ( jonesCog_bOverlayMapVisible ) {
        sithOverlayMap_ToggleMap();
    }

    switch ( type )
    {
        case 0:
            jonesCog_bShowHealthHUD = 1;
            JonesHud_SetFadeHealthHUD(jonesCog_bShowHealthHUD, 0);
            break;

        case 1:
            jonesCog_bShowHealthHUD = 0;
            JonesHud_SetFadeHealthHUD(jonesCog_bShowHealthHUD, 1);
            break;

        case 2:
            jonesCog_bShowHealthHUD = 0;
            JonesHud_SetFadeHealthHUD(jonesCog_bShowHealthHUD, 0);
            break;
    }

    JonesHud_EnableMenu(0);

    jonesCog_bEnableGamesave = 0;
    sithCamera_camInfo.stateFlags |= SITHCAMERA_STATE_CUTSCENE;

    if ( sithPlayer_pLocalPlayerThing )
    {
        pCurrentThing = sithPlayer_pLocalPlayerThing;
        pCurrentThing->thingInfo.actorInfo.flags |= SITH_AF_INVULNERABLE;
        if ( pCurrentThing->pCog )
        {
            sithCog_SendMessage(pCurrentThing->pCog, SITHCOG_MSG_USER2, SITHCOG_SYM_REF_THING, pCurrentThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    // Destroy all flying bullets
    for ( thingNum = 0, pCurrentThing = pWorld->aThings; thingNum <= pWorld->lastThingIdx; ++thingNum, ++pCurrentThing )
    {
        if ( pCurrentThing->type == SITH_THING_WEAPON )
        {
            sithThing_DestroyThing(pCurrentThing);
        }
    }

    sithPlayerControls_bCutsceneMode = 1;
    sithAI_StopAllAttackingAIs();
}

void J3DAPI jonesCog_EndCutscene(SithCog* pCog)
{

    if ( jonesCog_bOverlayMapVisible ) {
        sithOverlayMap_ToggleMap();
    }

    jonesCog_bShowHealthHUD = 1;
    JonesHud_SetFadeHealthHUD(jonesCog_bShowHealthHUD, 1);
    JonesHud_EnableMenu(1);

    jonesCog_bEnableGamesave = 1;
    if ( sithPlayer_pLocalPlayerThing )
    {
        SithThing* pPlayerThing = sithPlayer_pLocalPlayerThing;

        if ( !sithPlayer_bPlayerInvulnerable ) {
            pPlayerThing->thingInfo.actorInfo.flags &= ~SITH_AF_INVULNERABLE;
        }

        if ( pPlayerThing->pCog ) {
            sithCog_SendMessage(pPlayerThing->pCog, SITHCOG_MSG_USER3, SITHCOG_SYM_REF_THING, pPlayerThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    sithCamera_camInfo.stateFlags &= ~SITHCAMERA_STATE_CUTSCENE;
    sithPlayerControls_bCutsceneMode = 0;
}

void J3DAPI jonesCog_UpdateDifficulty(SithCog* pCog)
{
    unsigned int difficulty = sithCogExec_PopInt(pCog);
    difficulty = STDMATH_CLAMP(difficulty, 0, 5);
    jonesInventory_UpdateIQPoints(difficulty);
}

void J3DAPI jonesCog_EnableInterfaceFunc(SithCog* pCog)
{
    int bEnable;
    if ( pCog )
    {
        bEnable = sithCogExec_PopInt(pCog);
        if ( jonesCog_bMenuVisible )
        {
            JonesHud_EnableInterface(1);
            jonesCog_bInterfaceEnabled = bEnable;
            return;
        }
    }
    else
    {
        if ( jonesCog_bInterfaceEnabled > -1 )
        {
            bEnable = jonesCog_bInterfaceEnabled;
            jonesCog_bInterfaceEnabled = -1;
        }
        else
        {
            return;
        }
    }

    JonesHud_EnableMenu(bEnable);
    jonesCog_bEnableGamesave = bEnable;
}

void J3DAPI jonesCog_StartCutscene(SithCog* pCog)
{
    jonesCog_StartCutsceneFunc(pCog);
}

void J3DAPI jonesCog_ToggleInterface(SithCog* pCog)
{
    jonesCog_EnableInterfaceFunc(pCog);
}
