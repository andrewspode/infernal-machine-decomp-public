#include "JonesHud.h"
#include "JonesHudConstants.h"


#include <Jones3D/Display/jonesConfig.h>
#include <Jones3D/Main/jonesLevel.h>
#include <Jones3D/Main/jonesMain.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/jonesCog.h>
#include <Jones3D/Play/jonesInventory.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdCanvas.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Primitives/rdFont.h>
#include <rdroid/Primitives/rdModel3.h>
#include <rdroid/Raster/rdCache.h>

#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <w32util/wuRegistry.h>

#include <math.h>
#include <stdio.h>

// HUD tables used only by this TU: retail has one copy of each, in JonesHud.c's own
// .rdata/.data (jonesConfig.c includes JonesHudConstants.h only for the store table).
// Color constants
static const rdVector4 JonesHud_colorGreen   = { 0.14f, 0.69f, 0.0f, 0.0f };
static const rdVector4 JonesHud_colorYellow  = { 1.0f , 0.77f, 0.0f, 0.0f };
static const rdVector4 JonesHud_colorRed     = { 0.83f , 0.055f , 0.0f , 0.0f };
static const rdVector4 JonesHud_colorBlack   = { 0.01f , 0.01f , 0.01f , 0.0f };
static const rdVector4 JonesHud_colorWhite   = { 1.0f , 1.0f , 1.0f , 0.0f };
static const rdVector4 JonesHud_skyBlue      = { 0.4f ,  0.7f , 0.9f ,  0.0f };
static const rdVector4 JonesHud_colorPink    = { 1.0f , 0.0f , 1.0f, 0.0f };

// HUD camera constants
static const rdVector3 JonesHud_camViewPos = { 0.0f , -2.0f , 0.65f };
static const rdVector3 JonesHud_camViewPyr = { -17.700001f , 0.0f ,  0.0f };

// Menu item placement constants
static const rdVector3 JonesHud_invItemStartPos          = { -0.167f, 0.01f, -0.102f };
static const rdVector3 JonesHud_vecSelectedMenuItemScale = { 1.25f , 1.25f , 1.25f };
static const rdVector3 JonesHud_vecUnused_506500         = { 0.75f , 0.75f , 0.75f }; // Unreferenced; retail .rdata layout only

// Build option: when non-zero, ToggleMenu and Process treat the player as dead and
// run the game-over path. Retail tests a compile-time 0 as the third operand of both
// death checks (xor ecx,ecx; test ecx,ecx at 0x0041440C and 0x00414559); the name is
// inferred.
#define JONESHUD_DEBUG_GAMEOVER 0

// Inventory menu item constants
#define JONESHUD_MAX_MENU_ITEMS 134

static tMenuItemOrientInfo JonesHud_aDfltMenuItemOrients[JONESHUD_MAX_MENU_ITEMS] =
{
    { 1.1f, { { 0.0f }, { 270.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.85f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.8f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.9f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.05f, { { 60.0f }, { 90.0f }, { 90.0f } } },
    { 0.6f, { { 135.0f }, { 90.0f }, { 135.0f } } },
    { 0.9f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.0f, { { 60.0f }, { 90.0f }, { 90.0f } } },
    { 0.8f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 1.0f, { { 90.0f }, { 180.0f }, { 0.0f } } },
    { 1.0f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.8f, { { 45.0f }, { 315.0f }, { 90.0f } } },
    { 0.8f, { { 225.0f }, { 270.0f }, { 0.0f } } },
    { 1.25f, { { 225.0f }, { 270.0f }, { 0.0f } } },
    { 1.25f, { { 225.0f }, { 270.0f }, { 0.0f } } },
    { 0.7f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.9f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 30.0f }, { 0.0f }, { 315.0f } } },
    { 1.1f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.65f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.95f, { { 270.0f }, { 180.0f }, { 0.0f } } },
    { 1.1f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.05f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.05f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.7f, { { 0.0f }, { 180.0f }, { 0.0f } } },
    { 1.25f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.25f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.6f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 0.6f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 0.6f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 0.6f, { { 30.0f }, { 270.0f }, { 0.0f } } },
    { 1.25f, { { 0.0f }, { 150.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.1f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 0.7f, { { 45.0f }, { 90.0f }, { 0.0f } } },
    { 0.8f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.8f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.8f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 1.0f, { { 45.0f }, { 200.0f }, { 120.0f } } },
    { 0.85f, { { 45.0f }, { 135.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 1.0f, { { 315.0f }, { 0.0f }, { 0.0f } } },
    { 0.95f, { { 315.0f }, { 0.0f }, { 0.0f } } },
    { 0.65f, { { 315.0f }, { 0.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.65f, { { 45.0f }, { 315.0f }, { 0.0f } } },
    { 0.85f, { { 270.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 300.0f }, { 0.0f } } },
    { 0.7f, { { 45.0f }, { 315.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 225.0f }, { 0.0f } } },
    { 0.55f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.8f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 270.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.85f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.8f, { { 240.0f }, { 315.0f }, { 0.0f } } },
    { 0.8f, { { 240.0f }, { 315.0f }, { 0.0f } } },
    { 0.8f, { { 240.0f }, { 315.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 90.0f }, { 90.0f } } },
    { 0.9f, { { 45.0f }, { 90.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 225.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 225.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 135.0f }, { 0.0f } } },
    { 0.85f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 270.0f }, { 90.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.8f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 45.0f }, { 45.0f } } },
    { 0.85f, { { 0.0f }, { 25.0f }, { 45.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.7f, { { 45.0f }, { 270.0f }, { 90.0f } } },
    { 0.8f, { { 45.0f }, { 270.0f }, { 0.0f } } },
    { 1.0f, { { 315.0f }, { 270.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 80.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 240.0f }, { 0.0f } } },
    { 0.9f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 225.0f }, { 45.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 135.0f }, { 0.0f } } },
    { 0.7f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 180.0f }, { 45.0f } } },
    { 0.85f, { { 90.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 45.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 135.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 45.0f }, { 0.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.65f, { { 45.0f }, { 90.0f }, { 0.0f } } },
    { 0.7f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.75f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 1.0f, { { 0.0f }, { 180.0f }, { 0.0f } } },
    { 0.85f, { { 315.0f }, { 270.0f }, { 90.0f } } },
    { 0.85f, { { 135.0f }, { 270.0f }, { 90.0f } } },
    { 0.8f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 225.0f }, { 135.0f } } },
    { 0.75f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.75f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 200.0f }, { 0.0f } } },
    { 0.85f, { { 225.0f }, { 225.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 225.0f }, { 45.0f } } },
    { 0.85f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.9f, { { 45.0f }, { 45.0f }, { 0.0f } } },
    { 0.8f, { { 45.0f }, { 270.0f }, { 0.0f } } },
    { 0.85f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.85f, { { 0.0f }, { 0.0f }, { 0.0f } } },
    { 1.1f, { { 0.0f }, { 150.0f }, { 0.0f } } },
    { 0.85f, { { 45.0f }, { 315.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 90.0f }, { 0.0f } } },
    { 0.8f, { { 90.0f }, { 150.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 0.95f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 0.95f, { { 90.0f }, { 330.0f }, { 0.0f } } },
    { 1.0f, { { 90.0f }, { 330.0f }, { 0.0f } } }
};

static char* JonesHud_apMenuIconModelNames[JONESHUD_MAX_MENU_ITEMS] = {

    // Weapon menu item 3D models
    "weap_gloves_inv.3do",
    "weap_whip_inv.3do",
    "weap_revolver_inv.3do",
    "weap_tokarev_inv.3do",
    "weap_mauser_inv.3do",
    "weap_simonov_inv.3do",
    "weap_machete_inv.3do",
    "weap_ppsh41_inv.3do",
    "weap_toz34_inv.3do",
    "weap_grenade_inv.3do",
    "weap_satchel_inv.3do",
    "weap_bazooka_inv.3do",

    "gen_ammo_mauser.3do",
    "gen_ammo_ppsh41.3do",
    "gen_ammo_simonov.3do",
    "gen_ammo_tokarev.3do",
    "gen_ammo_toz34.3do",
    "gen_rckt_bzooka.3do",
    "weap_rocket_pack.3do",
    "weap_grenade_pack.3do",
    "weap_satchel_pack.3do",

    // Medicine menu item 3D models
    "gen_icon_hlthsml.3do",
    "gen_icon_hlthbig.3do",
    "gen_herb_small.3do",
    "gen_herb_big.3do",
    "gen_icon_bite.3do",
    NULL,

    // Treasury menu item 3D models
    "gen_coins_gold.3do",
    "gen_coins_slvr.3do",
    "gen_idol_gold.3do",
    "gen_idol_slvr.3do",
    "pru_idol.3do",
    "gen_ingots_gold.3do",
    "gen_ingots_slvr.3do",
    "gen_gem_red.3do",
    "gen_gem_green.3do",
    "gen_gem_blue.3do",
    "aet_gem04.3do",
    "riv_box_cash.3do",
    "gen_icon_chest.3do",

    // Inventory item menu 3D models
    "gen_icon_zippo.3do",
    "gen_icon_chalk.3do",
    "imp1_vibrat_inv.3do",
    "imp2_invis_inv.3do",
    "imp3_levit_inv.3do",
    "imp4_energ_inv.3do",
    "imp5_dial_inv.3do",
    "cyn_potshard.3do",
    "bab_gear_elev.3do",
    "bab_tu_key.3do",
    "bab_tblt_a.3do",
    "bab_tblt_b.3do",
    "bab_tblt_c.3do",
    "bab_idol_mrdk.3do",
    "riv_raft_inflt1_inv.3do",
    "riv_kit_ptch.3do",
    "riv_cndl_awind.3do",
    "riv_cndl_bwind.3do",
    "riv_cndl_cwind.3do",
    "riv_cndl_dwind.3do",
    "riv_coin.3do",
    "shs_key_sealdoor.3do",
    "shs_bulb_pot.3do",
    "shs_bulb_leafstem.3do",
    "shs_bulb_bud.3do",
    "gen_bronz_key.3do",
    "shs_key_lady.3do",
    "shw_jar_oil.3do",
    "shw_divkeyblue.3do",
    "shw_divkeygold.3do",
    "shw_divkeyred.3do",
    "lag_key_pdlck.3do",
    "lag_shovel_fold.3do",
    "lag_plane_prop.3do",
    "lag_hammer_small.3do",
    "lag_dvice_armng.3do",
    "lag_crnk_rsty.3do",
    "vol_key_sharkgate.3do",
    NULL,
    NULL,
    "vol_tram_wheel.3do",
    "vol_key_cuffs.3do",
    "tem_key_statue.3do",
    "tem_key_statue_b.3do",
    "tem_key_mnky.3do",
    "tem_sharkey.3do",
    "teo_key_water.3do",
    "teo_idol_bird.3do",
    "teo_idol_fish.3do",
    "teo_idol_jag.3do",
    "teo_mirror.3do",
    NULL,
    "gen_icon_crnk.3do",
    "pyr_minecar_whl.3do",
    "pyr_gem_eye.3do",
    "pyr_watch.3do",
    NULL,
    "pyr_ngin_chain.3do",
    "bab_jcan_gas.3do",
    "pyr_kndlng_cone.3do",
    "gen_bucket_wood.3do",
    "sol_oilcan_oz.3do",
    NULL,
    NULL,
    "sol_fuse_rplc.3do",
    NULL,
    "sol_gem_a.3do",
    "sol_gem_b.3do",
    "sol_gem_c.3do",
    NULL,
    "sol_pymd_key.3do",
    "sol_key_oil.3do",
    "gen_lever_stick.3do",
    "nub_anubis_arm.3do",
    "nub_gear_turtle.3do",
    "nub_key_elvtr.3do",
    "nub_rb_eye.3do",
    "inf_robo_head.3do",
    "aet_marduk_key.3do",
    "aet_mirror.3do",
    "shs_plank_2m.3do",
    "tem_pikes_floor.3do",
    "pru_key_sqr.3do",
    "gen_icon_bonusmap.3do",

    // System menu 3D models
     NULL,
     "iq_icon.3do",
     "gen_icon_help.3do",
     "gen_icon_camera.3do",
     "gen_icon_film.3do",
     "gen_icon_telgrf.3do",
     "gen_icon_type.3do",
     "gen_icon_tv.3do",
     "gen_icon_radio.3do",
     "gen_icon_exit.3do"
};

static char* JonesHud_apItemNames[JONESHUD_MAX_MENU_ITEMS] =
{
    // Weapons menu items
    "JONES_STR_INV_FISTS",
    "JONES_STR_INV_WHIP",
    "JONES_STR_INV_REVOLVER",
    "JONES_STR_INV_TOKEREV",
    "JONES_STR_INV_MAUSER",
    "JONES_STR_INV_SIMONOV",
    "JONES_STR_INV_MACHETE",
    "JONES_STR_INV_PPSH41",
    "JONES_STR_INV_TOZ34",
    "JONES_STR_INV_GRENADE",
    "JONES_STR_INV_SATCHEL",
    "JONES_STR_INV_BAZOOKA",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // Medicine menu
    "JONES_STR_INV_MEDICINE",
    "JONES_STR_INV_1STAID",
    "JONES_STR_INV_HERBSML",
    "JONES_STR_INV_HERBLARGE",
    "JONES_STR_INV_VENOM",
    "JONES_STR_INV_TICKET",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    // Item inventory menu
    "JONES_STR_INV_TREASURE",
    "JONES_STR_INV_LIGHTER",
    "JONES_STR_INV_CHALK",
    "JONES_STR_INV_SEISMIC",
    "JONES_STR_INV_INVISO",
    "JONES_STR_INV_ANTIGRAV",
    "JONES_STR_INV_POWER",
    "JONES_STR_INV_DILATOR",
    "JONES_STR_INV_SHARD",
    "JONES_STR_INV_EGEAR",
    "JONES_STR_INV_TRNKEY",
    "JONES_STR_INV_TBLA",
    "JONES_STR_INV_TBLB",
    "JONES_STR_INV_TBLC",
    "JONES_STR_INV_MARDUK",
    "JONES_STR_INV_DFLRFT",
    "JONES_STR_INV_RFTKIT",
    "JONES_STR_INV_CDLA",
    "JONES_STR_INV_CDLB",
    "JONES_STR_INV_CDLC",
    "JONES_STR_INV_CDLD",
    "JONES_STR_INV_COIN",
    "JONES_STR_INV_SEALKEY",
    "JONES_STR_INV_BULBPOT",
    "JONES_STR_INV_LEAF",
    "JONES_STR_INV_FLOWER",
    "JONES_STR_INV_BRONZEKEY",
    "JONES_STR_INV_LADYKEY",
    "JONES_STR_INV_OILJAR",
    "JONES_STR_INV_BLUDVKEY",
    "JONES_STR_INV_GLDDVKEY",
    "JONES_STR_INV_GRNTDVKEY",
    "JONES_STR_INV_PDLCKKY",
    "JONES_STR_INV_ENTOOL",
    "JONES_STR_INV_ZPROP",
    "JONES_STR_INV_RSTYHMMR",
    "JONES_STR_INV_ARMDVCE",
    "JONES_STR_INV_CRANK",
    "JONES_STR_INV_SHKGTKEY",
    "JONES_STR_INV_BLCKTKL",
    "JONES_STR_INV_SHKLKEY",
    "JONES_STR_INV_TRAMWHEEL",
    "JONES_STR_INV_CUFFKEY",
    "JONES_STR_INV_STUKEYA",
    "JONES_STR_INV_STUKEYB",
    "JONES_STR_INV_MKYKEY",
    "JONES_STR_INV_SHKKEY",
    "JONES_STR_INV_WTRKEY",
    "JONES_STR_INV_BRDIDL",
    "JONES_STR_INV_FSHIDL",
    "JONES_STR_INV_JGRIDL",
    "JONES_STR_INV_MIRROR",
    "JONES_STR_INV_DCKKEY",
    "JONES_STR_INV_CRNK",
    "JONES_STR_INV_MCRWHL",
    "JONES_STR_INV_GEMEYE",
    "JONES_STR_INV_WTCH",
    "JONES_STR_INV_RAIL",
    "JONES_STR_INV_DRVCHAIN",
    "JONES_STR_INV_GASCAN",
    "JONES_STR_INV_KNDLNG",
    "JONES_STR_INV_BUCKET",
    "JONES_STR_INV_OILCAN",
    "JONES_STR_INV_TRACK",
    "JONES_STR_INV_TRCKTIE",
    "JONES_STR_INV_FUSE",
    "JONES_STR_INV_MCRWHL",
    "JONES_STR_INV_GEMA",
    "JONES_STR_INV_GEMB",
    "JONES_STR_INV_GEMC",
    "JONES_STR_INV_HORUSEYE",
    "JONES_STR_INV_PYRKEY",
    "JONES_STR_INV_OILKEY",
    "JONES_STR_INV_STICK",
    "JONES_STR_INV_ARM",
    "JONES_STR_INV_BRZEGEAR",
    "JONES_STR_INV_BRZEKEY",
    "JONES_STR_INV_BOSSEYE",
    "JONES_STR_INV_ROBOT",
    "JONES_STR_INV_AETKEY",
    "JONES_STR_INV_MIRROR",
    "JONES_STR_INV_PLANK",
    "JONES_STR_INV_SPIKE",
    "JONES_STR_INV_SQRKEY",

    // System menus
    "JONES_STR_INV_MAP",
    "JONES_STR_INV_STATS",
    "JONES_STR_INV_STATS",
    "JONES_STR_INV_HELP",
    "JONES_STR_INV_SAVE",
    "JONES_STR_INV_LOAD",
    "JONES_STR_INV_DFCTLY",
    "JONES_STR_INV_CONFIG",
    "JONES_STR_INV_DSPLY",
    "JONES_STR_INV_SND",
    "JONES_STR_INV_EXIT"
};


// End credits constants

// Sound fx indices to jones3Dstatic sound list
// TODO: Replace indices in the list with some constants
// Note, from index 7 to the end are indy voice lines for the item that can't be activated (e.g. whoops)
static int JonesHud_aSoundFxIdxs[13] = { -1, 52, 54, 53, -1, 57, 56, 59, 60, 61, 62, 136, 137 };

static rdVector4 JonesHud_aCreditFontColors[12] =
{
    { { 1.0f }, { 1.0f }, { 1.0f }, { 1.0f } },
    { { 0.956f }, { 0.043f }, { 0.89f }, { 1.0f } },
    { { 0.592f }, { 0.007f }, { 0.984f }, { 1.0f } },
    { { 0.011f }, { 0.89f }, { 0.988f }, { 1.0f } },
    { { 0.0f }, { 0.588f }, { 0.0f }, { 1.0f } },
    { { 0.89f }, { 0.976f }, { 0.011f }, { 1.0f } },
    { { 1.0f }, { 0.0f }, { 0.501f }, { 1.0f } },
    { { 1.0f }, { 0.0f }, { 0.0f }, { 1.0f } },
    { { 0.0f }, { 1.0f }, { 0.0f }, { 1.0f } },
    { { 0.0f }, { 0.0f }, { 1.0f }, { 1.0f } },
    { { 1.0f }, { 0.329f }, { 0.007f }, { 1.0f } },
    { { 0.945f }, { 0.709f }, { 0.086f }, { 1.0f } }
};

// TODO: Update flags to be represented as hex numbers
static tJonesCreditEntry JonesHud_aCredits[325] =
{
    { "gen_4_pyramid.mat", 4, 1 },
    { "JONES_STR_HGAME", 2, 1 },
    { "(in alphabetical order)", 16, 0 },
    { "     ", 1, 0 },
    { "JONES_STR_TPROJECT", 0x10, 1 },
    { "HAL BARWOOD", 1, 0 },
    { "gen_4_selfportrait.mat", 4, 2 },
    { "JONES_STR_HENGINE", 2, 2 },
    { "JONES_STR_TLEADPGR", 16, 2 },
    { "PAUL D. LeFEVRE", 1, 0 },
    { "JONES_STR_TPGRS", 16, 2 },
    { "GARY KEITH BRUBAKER", 1, 0 },
    { "PATRICK McCARTHY", 1, 0 },
    { "IFE OLOWE", 1, 0 },
    { "MATTHEW D. RUSSELL", 1, 0 },
    { "STEVE SCHOLL", 1, 0 },
    { "RANDY TUDOR", 1, 0 },
    { "gen_4_sunrise.mat", 4, 3 },
    { "JONES_STR_HWORLD", 2, 3 },
    { "JONES_STR_TLEADLD", 16, 3 },
    { "STEVEN CHEN", 1, 0 },
    { "JONES_STR_TLD", 16, 3 },
    { "JOSEPH CHIANG", 1, 0 },
    { "REED K. DERLETH", 1, 0 },
    { "GEOFF JONES", 1, 0 },
    { "TIM LONGO", 1, 0 },
    { "CHRISTOPHER W. McGEE", 1, 0 },
    { "TIMOTHY R. MILLER", 1, 0 },
    { "DONALD SIELKE", 1, 0 },
    { "PAUL \"Skitoman\" ZABIEREK", 1, 0 },
    { "JONES_STR_TPLCMNT", 16, 3 },
    { "JESSE MOORE", 1, 0 },
    { "gen_4_eye.mat", 4, 4 },
    { "JONES_STR_HART", 2, 4 },
    { "JONES_STR_TART", 16, 4 },
    { "WILLIAM TILLER", 1, 0 },
    { "JONES_STR_TTEXTURE", 16, 4 },
    { "KIM BALESTRERI", 1, 0 },
    { "KATHY HSIEH", 1, 0 },
    { "GREGORY A. KNIGHT", 1, 0 },
    { "KAREN PURDY", 1, 0 },
    { "NATHAN STAPLEY", 1, 0 },
    { "TIM Y. TAO", 1, 0 },
    { "gen_4x_indy2.mat", 4, 5 },
    { "JONES_STR_HMOTION", 2, 5 },
    { "JONES_STR_TLEADANIM", 16, 5 },
    { "DEREK SAKAI", 1, 0 },
    { "JONES_STR_TANIM", 16, 5 },
    { "YOKO (YANG-JA) BALLARD", 1, 0 },
    { "ANSON JEW", 1, 0 },
    { "gen_4_hawk.mat", 4, 6 },
    { "JONES_STR_HSHAPES", 2, 6 },
    { "JONES_STR_TCHAR", 16, 6 },
    { "LEA MAI NGUYEN", 1, 0 },
    { "JONES_STR_TMODEL", 16, 6 },
    { "MIKE DACKO", 1, 0 },
    { "gen_4x_x.mat", 4, 7 },
    { "JONES_STR_HFLASH", 2, 7 },
    { "JONES_STR_TVISUAL", 16, 7 },
    { "JOSEF RICHARDSON", 1, 0 },
    { "gen_4x_check.mat", 4, 8 },
    { "JONES_STR_HARTPATH", 2, 8 },
    { "JONES_STR_TLDARTTECH", 16, 8 },
    { "HARLEY BALDWIN", 1, 0 },
    { "JONES_STR_TARTTECH", 16, 8 },
    { "LISSA KLANOR", 1, 0 },
    { "gen_4x_happy.mat", 4, 9 },
    { "JONES_STR_HDIGGRS", 2, 9 },
    { "JONES_STR_TSNDPGR", 16, 9 },
    { "MIKE McMAHON", 1, 0 },
    { "JONES_STR_TVIDEO", 16, 9 },
    { "VINCE LEE", 1, 0 },
    { "JONES_STR_TVIDEOADPT", 16, 9 },
    { "ANDRE BREMER", 1, 0 },
    { "JONES_STR_TINSTALL", 16, 9 },
    { "DARREN JOHNSON", 1, 0 },
    { "JOE LIGMAN", 1, 0 },
    { "JONES_STR_TTOOL", 16, 9 },
    { "AMIT SHALEV", 1, 0 },
    { "JONES_STR_TSPPGR", 16, 9 },
    { "INGAR SHU", 1, 0 },
    { "HWEI-LI TSAO", 1, 0 },
    { "JONES_STR_TANIM", 16, 9 },
    { "JOHN McLAUGHLIN", 1, 0 },
    { "VAMBERTO MADURO", 1, 0 },
    { "JONES_STR_TEFFECTS", 16, 9 },
    { "C. ANDREW NELSON", 1, 0 },
    { "JONES_STR_TTEXTURE", 16, 9 },
    { "CATHERINE \"Cat\" TILLER", 1, 0 },
    { "gen_4_star.mat", 4, 10 },
    { "JONES_STR_HVOICE", 2, 10 },
    { "JONES_STR_TINDY", 16, 10 },
    { "DOUG LEE", 1, 0 },
    { "JONES_STR_TSOPHIA", 16, 10 },
    { "TASIA VALENZA", 1, 0 },
    { "JONES_STR_TVOL", 16, 10 },
    { "VICTOR RAIDER-WEXLER", 1, 0 },
    { "JONES_STR_TTURNER", 16, 10 },
    { "BRUCE McGILL", 1, 0 },
    { "JONES_STR_TPRIEST", 16, 10 },
    { "CAROLYN SEYMOUR", 1, 0 },
    { "JONES_STR_TBOY", 16, 10 },
    { "GREY DELISLE", 1, 0 },
    { "JONES_STR_TMILITARY", 16, 10 },
    { "ADAM GREGOR", 1, 0 },
    { "ANDREW KOCHERGIN", 1, 0 },
    { "PHIL PROCTOR", 1, 0 },
    { "VLADIMIR ROMANOV", 1, 0 },
    { "JONES_STR_TMARDUK", 16, 10 },
    { "ADAM GREGOR", 1, 0 },
    { "DOUG LEE", 1, 0 },
    { "gen_4_pound.mat", 4, 1 },
    { "JONES_STR_HVPROD", 2, 1 },
    { "JONES_STR_TVPROD", 16, 1 },
    { "DARRAGH O'FARRELL", 1, 0 },
    { "JONES_STR_TVEDIT", 16, 1 },
    { "COYA ELLIOTT", 1, 0 },
    { "CINDY WONG", 1, 0 },
    { "JONES_STR_TVMGR", 16, 1 },
    { "TAMLYNN NIGLIO", 1, 0 },
    { "JONES_STR_TVCOORD", 16, 1 },
    { "PEGGY BARTLETT", 1, 0 },
    { "JONES_STR_TVRECORD", 16, 1 },
    { "SCREENMUSIC STUDIOS", 1, 0 },
    { "Studio City, California", 1, 0 },
    { "LUCASARTS ENTERTAINMENT COMPANY, LLC", 1, 0 },
    { "San Rafael, California", 1, 0 },
    { "gen_4x_circle.mat", 4, 2 },
    { "JONES_STR_HSOUND", 2, 2 },
    { "JONES_STR_TMUSIC", 16, 2 },
    { "CLINT BAJAKIAN", 1, 0 },
    { "JONES_STR_TSNDDSN", 16, 2 },
    { "DAVID LEVISON", 1, 0 },
    { "JONES_STR_TCOMPOSE", 16, 2 },
    { "JOHN WILLIAMS", 1, 0 },
    { "JONES_STR_TSNDASST", 16, 2 },
    { "MICHAEL FRAYNE", 1, 0 },
    { "JORY PRUM", 1, 0 },
    { "JONES_STR_TSNDCOORD", 16, 2 },
    { "KRISTEN BECHT", 1, 0 },
    { "gen_4_five.mat", 4, 3 },
    { "JONES_STR_HPROD", 2, 3 },
    { "JONES_STR_TPRODMGR", 16, 3 },
    { "WAYNE CLINE", 1, 0 },
    { "JONES_STR_TPRODCOORD", 16, 3 },
    { "JENNIFER O'NEAL", 1, 0 },
    { "KELLIE TAUZIN", 1, 0 },
    { "gen_4_skull.mat", 4, 4 },
    { "JONES_STR_HQA", 2, 4 },
    { "JONES_STR_TLEADTEST", 16, 4 },
    { "JOHN \"Buzz\" BUZOLICH", 1, 0 },
    { "JOHN \"Grandpa\" HANNON", 1, 0 },
    { "JONES_STR_TLEADTESTASST", 16, 4 },
    { "BRENT JAY ANDAYA JALIPA II", 1, 0 },
    { "JONES_STR_TTEST", 16, 4 },
    { "PHILLIP \"Philthy\" BERRY", 1, 0 },
    { "GABRIELLE BROWN", 1, 0 },
    { "SHANG-JU CHEN", 1, 0 },
    { "G.W. CHILDS", 1, 0 },
    { "JEFFREY DAY", 1, 0 },
    { "JOHN DRAKE", 1, 0 },
    { "ERIK ELLICOCK", 1, 0 },
    { "BRYAN ERCK", 1, 0 },
    { "DEREK FLIPPO", 1, 0 },
    { "CATHERINE HAIGLER", 1, 0 },
    { "CHANE HOLLANDER", 1, 0 },
    { "HANS LARSON", 1, 0 },
    { "RICARDO LIU", 1, 0 },
    { "TROY MASHBURN", 1, 0 },
    { "CHUCK McFADDEN", 1, 0 },
    { "MATTHEW \"Cyrus\" McMANUS", 1, 0 },
    { "ORION NEMETH", 1, 0 },
    { "TODD STRITTER", 1, 0 },
    { "TORII SWADER", 1, 0 },
    { "JOSEPH TALAVERA II", 1, 0 },
    { "JULIO TORRES", 1, 0 },
    { "JONES_STR_TINTLTEST", 16, 4 },
    { "JEFF SANDERS", 1, 0 },
    { "JONES_STR_TCOMP", 16, 4 },
    { "CHIP HINNENBERG", 1, 0 },
    { "JONES_STR_TCOMPTEST", 16, 4 },
    { "KARSTEN AGLER", 1, 0 },
    { "DARREN BROWN", 1, 0 },
    { "JIM DAVISON", 1, 0 },
    { "MARCUS P. GAINES", 1, 0 },
    { "DOYLE GILSTRAP", 1, 0 },
    { "JASON LAUBOROUGH", 1, 0 },
    { "DAN MIHOERCK", 1, 0 },
    { "CHARLES W. SMITH", 1, 0 },
    { "SCOTT TINGLEY", 1, 0 },
    { "AARON YOUNG", 1, 0 },
    { "JONES_STR_TEBOOK", 16, 4 },
    { "LYNN TAYLOR", 1, 0 },
    { "JONES_STR_TPRODBURN", 16, 4 },
    { "WENDY \"Cupcake\" KAPLAN", 1, 0 },
    { "KELLIE \"Twinky\" WALKER", 1, 0 },
    { "gen_4_questionmark.mat", 4, 5 },
    { "JONES_STR_HSUP", 2, 5 },
    { "JONES_STR_TSUPMGR", 16, 5 },
    { "PAUL PURDY", 1, 0 },
    { "JONES_STR_THINT", 16, 5 },
    { "TABITHA TOSTI", 1, 0 },
    { "gen_4x_indy.mat", 4, 6 },
    { "JONES_STR_HSHOP", 2, 6 },
    { "JONES_STR_TSHOPMGR", 16, 6 },
    { "TOM BYRON", 1, 0 },
    { "JONES_STR_TPRMGR", 16, 6 },
    { "TOM SARRIS", 1, 0 },
    { "JONES_STR_TPACKDSN", 16, 6 },
    { "SOO HOO DESIGN", 1, 0 },
    { "JONES_STR_TPACKPICT", 16, 6 },
    { "DREW STRUZAN", 1, 0 },
    { "BRUCE WOLF", 1, 0 },
    { "JONES_STR_TBOOK", 16, 6 },
    { "JO \"Captain Tripps\" ASHBURN", 1, 0 },
    { "MOLLIE BOERO", 1, 0 },
    { "JONES_STR_TBOOKDSN", 16, 6 },
    { "PATTY HILL", 1, 0 },
    { "JONES_STR_TPRSP", 16, 6 },
    { "HEATHER TWIST", 1, 0 },
    { "JOSH MOORE", 1, 0 },
    { "JONES_STR_TSHOPCOORD", 16, 6 },
    { "ANNE BARSON", 1, 0 },
    { "gen_4_hammer&sickle.mat", 4, 7 },
    { "JONES_STR_HINTL", 2, 7 },
    { "JONES_STR_TLCLINTL", 16, 7 },
    { "ADAM PASZTORY", 1, 0 },
    { "JONES_STR_TINTLDEVSP", 16, 7 },
    { "THERESA O'CONNOR", 1, 0 },
    { "JONES_STR_TINTLPROD", 16, 7 },
    { "BENJAMIN BECKER", 1, 0 },
    { "JONES_STR_TINTLDEV", 16, 7 },
    { "ARIC WILMUNDER", 1, 0 },
    { "JONES_STR_INTLCOORD", 16, 7 },
    { "LEE SUSEN", 1, 0 },
    { "JONES_STR_TINTLPR", 16, 7 },
    { "KATHY APOSTOLI", 1, 0 },
    { "gen_4_alien.mat", 4, 8 },
    { "JONES_STR_HSTAFF", 2, 8 },
    { "JONES_STR_TPRES", 16, 8 },
    { "JACK SORENSEN", 1, 0 },
    { "JONES_STR_TDIRDEV", 16, 8 },
    { "STEVE DAUTERMAN", 1, 0 },
    { "SIMON JEFFERY", 1, 0 },
    { "JONES_STR_TSHOPDIR", 16, 8 },
    { "MARY BIHR", 1, 0 },
    { "JONES_STR_TTECHDIR", 16, 8 },
    { "MALCOLM JOHNSON", 1, 0 },
    { "JONES_STR_TARTDIR", 16, 8 },
    { "KAREN CHELINI", 1, 0 },
    { "JONES_STR_TPRODDIR", 16, 8 },
    { "CAMELA MARTIN", 1, 0 },
    { "JONES_STR_TPRODBIS", 16, 8 },
    { "JEREMY SALESIN", 1, 0 },
    { "JONES_STR_TCTRLR", 16, 8 },
    { "TOM McCARTHY", 1, 0 },
    { "JONES_STR_TQADIR", 16, 8 },
    { "MARK CARTWRIGHT", 1, 0 },
    { "JONES_STR_TQAMGR", 16, 8 },
    { "DAN PETTIT", 1, 0 },
    { "JONES_STR_TSNDMGR", 16, 8 },
    { "MICHAEL LAND", 1, 0 },
    { "JONES_STR_TSALEMGR", 16, 8 },
    { "MEREDITH CAHILL", 1, 0 },
    { "JONES_STR_TINTLMGR", 16, 8 },
    { "LISA STAR", 1, 0 },
    { "JONES_STR_TCHMGR", 16, 8 },
    { "TIM MOORE", 1, 0 },
    { "JONES_STR_TBIS", 16, 8 },
    { "JOCELYN LAMM", 1, 0 },
    { "JANNETT SHIRLEY-PAUL", 1, 0 },
    { "JONES_STR_TADMIN", 16, 8 },
    { "CATHERINE DURAND", 1, 0 },
    { "DENISE GOLLAHER", 1, 0 },
    { "JOHN KNOWLES", 1, 0 },
    { "SANDI MANESIS", 1, 0 },
    { "GABRIEL McDONALD", 1, 0 },
    { "LISA SWART", 1, 0 },
    { "gen_4_ankh.mat", 4, 9 },
    { "JONES_STR_HTHANKS", 2, 9 },
    { "BARBARA BARWOOD", 1, 0 },
    { "MICHELLE MORRIS BRUBAKER", 1, 0 },
    { "THE CHIANG GANG", 1, 0 },
    { "BETH CONNELL & the 008 posse", 1, 0 },
    { "Thanks to JENNIFER for the salsa", 1, 0 },
    { "AMY EWING", 1, 0 },
    { "DR. & MRS. JONES (Bill & Lida)", 1, 0 },
    { "JENNIFER LONGO", 1, 0 },
    { "LIVIA MACKIN", 1, 0 },
    { "RACHEL SAIDMAN", 1, 0 },
    { "SUZIE, HEATHER & KIMMY SIELKE", 1, 0 },
    { "AMY TILLER", 1, 0 },
    { "GINA VENTURI", 1, 0 },
    { "VENUS & MATTHEW", 1, 0 },
    { "ELISA, NICK, JUSTICE & LIBERTY TUDOR", 1, 0 },
    { "TAC", 1, 0 },
    { "BEAN", 1, 0 },
    { "EDISON & OTHELLO", 1, 0 },
    { "MILO & CECIL", 1, 0 },
    { "PIXEL", 1, 0 },
    { "gen_4x_guy.mat", 4, 10 },
    { "JONES_STR_HTIP", 2, 10 },
    { "JONES_STR_TNGINEDEV", 16, 10 },
    { "RAY GRESKO", 1, 0 },
    { "ROB HUEBNER", 1, 0 },
    { "CHE-YUAN WANG", 1, 0 },
    { "WINSTON WOLFF", 1, 0 },
    { "JONES_STR_TCONSOLE", 16, 10 },
    { "PATRICK COSTELLO", 1, 0 },
    { "JONATHAN MENZIES", 1, 0 },
    { "NICK PAVIS", 1, 0 },
    { "JONES_STR_TPGRSUP", 16, 10 },
    { "BOB I. JUDELSON", 1, 0 },
    { "CHRIS KASTENSMIDT", 1, 0 },
    { "THE INTEL CORPORATION", 1, 0 },
    { "JONES_STR_TBOW", 16, 10 },
    { "HARRISON FORD", 1, 0 },
    { "STEVEN SPIELBERG", 1, 0 },
    { "gen_4_r2d2.mat", 4, 1 },
    { "JONES_STR_HSPTHXS", 2, 1 },
    { "GEORGE LUCAS", 1, 0 },
    { "mat\\gg_a.mat", 0x0C, 11 },
    { "mat\\gg_b.mat", 0x0C, 11 },
    { "\xA9 LUCASARTS ENTERTAINMENT COMPANY LLC", 2, 0 },
    { "JONES_STR_HCOPY", 2, 0 }
};

// Retail compiled the whole TU without optimization: EBP frames, stack homes
// for every temporary and 0xCC alignment fill (intrinsics stay enabled).
#pragma optimize("", off)


// Initialized data (.data), in retail order after the JonesHudConstants.h tables
static size_t JonesHud_msecMenuItemMoveDuration = 250;
static int JonesHud_selectedWeaponID            = -1;
static float JonesHud_curEnduranceIndAlpha      = 0.2f;
static float JonesHud_curEnduranceState         = -1.0f;
static int JonesHud_curKeyId                    = -1;
static int JonesHud_curWhoopsSndFxIdx           = -1;

// Zero-initialized statics (.bss). The zero initializers give VC5's declaration-ordered
// layout, which is retail's order (uninitialized statics would be hash ordered).
static int JonesHud_bCutsceneStart = 0;
static rdVector4 JonesHud_healthIndBarPos = { 0 };
static size_t JonesHud_msecDeltaTime = 0;
static JonesHudMenuItem* JonesHud_apMenuItems[JONESHUD_MAX_MENU_ITEMS] = { 0 };
static int JonesHud_hudState = 0;
static size_t JonesHud_msecGameOverWaitDeltaTime = 0;
static rdVector4 JonesHud_enduranceIndBarPos = { 0 };
static int JonesHud_creditsCurMatIdx = 0;
static float JonesHud_screenHeightScalar = 0;
static float JonesHud_screenWidthScalar = 0;
static int JonesHud_selectedTreasuresMenuItemID = 0;
static JonesHudRect JonesHud_healthIndRect = { 0 };
static float JonesHud_indScale = 0;
static int JonesHud_curCelNum = 0;
static size_t JonesHud_msecTime = 0;
static rdMatrix34 JonesHud_camViewMatrix = { 0 };
static rdFont* JonesHud_pCreditsFont2 = 0;
static char JonesHud_aSlectedNdsFilePath[JONESCONFIG_GAMESAVE_FILEPATHSIZE] = { 0 };
static int JonesHud_rootMenuItemId = 0;
static float JonesHud_maxItemZ = 0;
static int JonesHud_bIMPState = 0;
static int JonesHud_bBonusMapBought = 0;
static rdMaterial* JonesHud_apCreditsMats[STD_ARRAYLEN(JonesHud_aCredits)] = { 0 };
static rdFont* JonesHud_pCreditsFont1 = 0;
static uint32_t JonesHud_msecLastKeyPressTime = 0;
static int JonesHud_selectedWeaponMenuItemID = 0;
static int JonesHud_selectedOptionsMenuItemID = 0;
static int JonesHud_msecRaftIndicatorFadeTime = 0;
static int JonesHud_foundTreasureValue = 0;
static float JonesHud_curHealth = 0;
static float JonesHud_creditTextHeight = 0;
static float JonesHud_nearClipPlane = 0;
static size_t JonesHud_aActivateKeyIds[JONESCONTROL_ACTION_MAXBINDS] = { 0 };
static float JonesHud_aCosSinTableScaled[24][2] = { 0 };
static int JonesHud_bInterfaceEnabled = 0;
static int JonesHud_creditsCanvasHeight = 0;
static float JonesHud_aCreditsCurPosY[STD_ARRAYLEN(JonesHud_aCredits)] = { 0 };
static int JonesHud_levelStartIQPoints = 0;
static JonesHudRect JonesHud_enduranceRect = { 0 };
static int JonesHud_unusedState0 = 0; // Unreferenced 4-byte slot (0x00554FD8); layout only
static int JonesHud_dword_554FDC = 0;  // Written by UpdateHUDLayout, never read
static float JonesHud_flt_554FE0 = 0;
static float JonesHud_flt_554FE4 = 0;
static int JonesHud_creditsCurIdx = 0;
static int JonesHud_selectedItemsMenuItemID = 0;
static int JonesHud_numFoundTreasures = 0;
static int JonesHud_creditsCurEndIdx = 0;
static tSoundHandle JonesHud_aSoundFxHandles[STD_ARRAYLEN(JonesHud_aSoundFxIdxs)] = { 0 };
static int JonesHud_unusedState2 = 0; // Unreferenced 4-byte slots (0x0055502C, 0x00555030); layout only
static int JonesHud_unusedState3 = 0;
static float JonesHud_flt_555034 = 0;
static float JonesHud_flt_555038 = 0;
static float JonesHud_flt_55503C = 0;
static int JonesHud_creditsCanvasWidth = 0;
static float JonesHud_healthIndAlpha = 0;
static int JonesHud_bFadeHealthHUD = 0;
static float JonesHud_aCosSinTable[24][2] = { 0 };
static size_t JonesHud_msecHealthIndLastFadeUpdate = 0;
static float JonesHud_curFadeHealthState = 0;
static int JonesHud_bStartup = 0;
static int JonesHud_bOpen = 0;
static rdMaterial* JonesHud_pArrowUp = 0;
static rdMaterial* JonesHud_pArrowDown = 0;
static int JonesHud_bHealthIndFadeSet = 0;
static rdMaterial* JonesHud_pHudBaseMat = 0;
static rdMaterial* JonesHud_pHealthOverlay = 0;
static rdMaterial* JonesHud_pHealthOverlayHitMat = 0;
static rdMaterial* JonesHud_pPoisonedOverlayMat = 0;
static rdMaterial* JonesHud_pEnduranceOverlayMat = 0;
static rdFont* JonesHud_pMenuFont = 0;
static JonesHudMenuItem* JonesHud_pMenuItemLinkedList = 0;
static JonesHudMenuItem* JonesHud_pCloseMenuItem = 0;
static uint8_t JonesHud_item_flag_state_55514C = 0;
static JonesHudMenuItem* JonesHud_pCurSelectedMenuItem = 0;
static JonesHudMenuItem* JonesHud_pCurInvChangedItem = 0;
static int JonesHud_bExitActivated = 0;
static int JonesHud_dword_55515C = 0; // Cleared by Close/Update, never read
static int JonesHud_bRestoreActivated = 0;
static int JonesHud_bRestoreGameStatistics = 0;
static int JonesHud_bItemActivated = 0;
static int JonesHud_bMenuEnabled = 0;
static rdCamera* JonesHud_pHudCamera = 0;
static rdCanvas* JonesHud_pHudCanvas = 0;
static int JonesHud_bMapOpen = 0;
static int JonesHud_bShowMenu = 0;
static int JonesHud_unusedState1 = 0; // Unreferenced 4-byte slot (0x00555180); layout only
static tSoundChannelHandle JonesHud_hCurSndChannel = 0;
static int JonesHud_bFadingHealthHUD = 0;
static float JonesHud_hitOverlayAlphaState = 0;
static float JonesHud_hitOverlayAlpha = 0;
static int JonesHud_bKeyStateUpdated = 0;
static HANDLE JonesHud_hProcessHelp = 0;
static int JonesHud_msecCreditsElapsedTime = 0;
static int JonesHud_msecCreditsFadeStart = 0;
static float JonesHud_creditsSomeHeightRatio = 0;
static int JonesHud_bEndingCredits = 0;
static int JonesHud_bSkipUpdateCredits = 0;

int J3DAPI JonesHud_Startup(void)
{
    float x;
    int i;
    float outSin;
    float outCos;

    if ( JonesHud_bStartup )
    {
#line 340
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already initialized!\n", 0, 0, 0, 0);
        return 0;
    }

    jonesCog_bMenuVisible         = 0;
    jonesCog_bEnableGamesave      = 1;
    JonesHud_bRestoreActivated      = 0;
    JonesHud_bRestoreGameStatistics = 0;

    memset(JonesHud_aActivateKeyIds, 0, 2 * sizeof(JonesHud_aActivateKeyIds[0]));
    memset(JonesHud_aCosSinTableScaled, 0, sizeof(JonesHud_aCosSinTableScaled));
    memset(JonesHud_aCosSinTable, 0, sizeof(JonesHud_aCosSinTable));

    // Init sin cos table
    for ( i = 0, x = 0.0f; x < 360.0f; ++i, x = x + 15.0f )
    {
        stdMath_SinCos(x, &outSin, &outCos);
        JonesHud_aCosSinTable[i][1] = outSin;
        JonesHud_aCosSinTable[i][0] = outCos;
    }

    if ( jonesInventory_ResetGameStatistics() )
    {
        return 1;
    }

    JonesHud_bCutsceneStart    = 0;
    JonesHud_bInterfaceEnabled = 0;
    JonesHud_bStartup          = 1;
    return 0;
}

void J3DAPI JonesHud_Shutdown(void)
{
    int i;

    if ( !JonesHud_bStartup )
    {
#line 393
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already shutdown!\n", 0, 0, 0, 0);
        return;
    }

    jonesInventory_Shutdown();
    JonesHud_pHudBaseMat          = NULL;
    JonesHud_pHealthOverlay       = NULL;
    JonesHud_pHealthOverlayHitMat = NULL;
    JonesHud_pPoisonedOverlayMat  = NULL;
    JonesHud_pEnduranceOverlayMat = NULL;
    JonesHud_pArrowUp             = NULL;
    JonesHud_pArrowDown           = NULL;
    JonesHud_pHudCanvas           = NULL;
    JonesHud_pHudCamera           = NULL;
    JonesHud_pMenuFont            = NULL;
    JonesHud_pMenuItemLinkedList  = NULL;

    for ( i = 0; i < JONESHUD_MAX_MENU_ITEMS; ++i )
    {
        JonesHud_apMenuItems[i] = NULL;
    }

    for ( i = 0; i < 13; ++i )
    {
        JonesHud_aSoundFxHandles[i] = 0;
    }

    JonesHud_hCurSndChannel       = 0;
    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_pCloseMenuItem       = NULL;
    JonesHud_pCurInvChangedItem   = NULL;
    JonesHud_screenWidthScalar  = 0.0f;
    JonesHud_screenHeightScalar = 0.0f;

    jonesConfig_Shutdown();
    JonesHud_bCutsceneStart    = 0;
    JonesHud_bInterfaceEnabled = 0;
    JonesHud_bStartup          = 0;
}

int J3DAPI JonesHud_Open(void)
{
    rdCamera* pCurCamera;
    uint32_t width;
    rdVector4 lightColor;
    uint32_t height;

    pCurCamera = rdCamera_pCurCamera;
    if ( JonesHud_bOpen )
    {
#line 455
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 0;
    }

    if ( jonesInventory_Open() )
    {
        return 1;
    }

    JonesHud_hudState       = 0;
    JonesHud_pCloseMenuItem = NULL;

    if ( (JonesHud_pHudBaseMat = rdMaterial_Load("hud_health_base.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_base.mat");
    }
    else if ( (JonesHud_pHealthOverlay = rdMaterial_Load("hud_health_overlay.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_overlay.mat");
    }
    else if ( (JonesHud_pHealthOverlayHitMat = rdMaterial_Load("hud_health_overlay_hit.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_overlay_hit.mat");
    }
    else if ( (JonesHud_pPoisonedOverlayMat = rdMaterial_Load("hud_health_overlay_skull.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_health_overlay_skull.mat");
    }
    else if ( (JonesHud_pArrowUp = rdMaterial_Load("gen_4arrow_up.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "gen_4arrow_up.mat");
    }
    else if ( (JonesHud_pArrowDown = rdMaterial_Load("gen_4arrow_down.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "gen_4arrow_down.mat");
    }
    else if ( (JonesHud_pEnduranceOverlayMat = rdMaterial_Load("hud_breath_overlay.mat")) == NULL )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "hud_breath_overlay.mat");
    }
    else if ( !JonesHud_InitializeMenu() )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load initialize menu items.\n");
    }
    else if ( !jonesConfig_Startup() )
    {
        sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't intialize dialog data.\n");
    }
    else
    {
        JonesHud_InitializeMenuSounds();

        // Setup HUD camera
        rdCamera_SetOverlayDepthRange(1);

        if ( (JonesHud_pHudCamera = rdCamera_New(10.0f, 0, 1.0f, 100.0f, 1.0f)) == NULL )
        {
            sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't create Hud camera.\n");
        }
        else if ( (JonesHud_pHudCanvas = rdCanvas_New(0, &stdDisplay_backBuffer, 64, (int)JonesHud_healthIndRect.y, 576, (int)(JonesHud_healthIndRect.y + 64.0f))) == NULL )
        {
            sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't create Hud canvas.\n");
        }
        else
        {
            rdCamera_SetCanvas(JonesHud_pHudCamera, JonesHud_pHudCanvas);
            stdDisplay_GetBackBufferSize(&width, &height);
            rdCamera_SetFrustrum(JonesHud_pHudCamera, JonesHud_pHudCamera->pFrustum, 0, 0, width - 1, height - 1);
            rdCamera_SetCurrent(JonesHud_pHudCamera);

            rdMatrix_Copy34(&JonesHud_camViewMatrix, &rdroid_identMatrix34);
            rdMatrix_PostRotate34(&JonesHud_camViewMatrix, &JonesHud_camViewPyr);
            JonesHud_camViewMatrix.dvec = JonesHud_camViewPos;
            rdCamera_Update(&JonesHud_camViewMatrix);
            JonesHud_nearClipPlane = 1.0f + JonesHud_pHudCamera->pFrustum->nearPlane;

            // Restore previous camera
            rdCamera_SetCurrent(pCurCamera);

            lightColor.red   = 1.0f;
            lightColor.green = 1.0f;
            lightColor.blue  = 1.0f;
            lightColor.alpha = 0.0f;
            rdCamera_SetAmbientLight(JonesHud_pHudCamera, &lightColor);
            rdCamera_SetOverlayDepthRange(0);

            JonesHud_bBonusMapBought = 0;
            JonesHud_bIMPState       = 0;
            JonesHud_hudState        = 0;

            if ( (JonesHud_pMenuFont = rdFont_Load("mat\\jonesComic Sans MS14.gcf")) == NULL )
            {
                sprintf(JonesMain_aErrorBuffer, "Unable to open Hud: Couldn't load %s\n", "mat\\jonesComic Sans MS14.gcf");
            }
            else
            {
                JonesHud_bOpen             = 1;
                JonesHud_bCutsceneStart    = 0;
                JonesHud_bInterfaceEnabled = 0;
                return 0;
            }
        }
    }

#line 587
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, JonesMain_aErrorBuffer, 0, 0, 0, 0);
    JonesHud_Close();
    JonesHud_bOpen = 0;
    return 1;
}

void J3DAPI JonesHud_Close(void)
{
    int i;

    JonesHud_pMenuItemLinkedList = 0;
    for ( i = 0; i < JONESHUD_MAX_MENU_ITEMS; ++i )
    {
        if ( JonesHud_apMenuItems[i] )
        {
            JonesHud_FreeMenuItem(JonesHud_apMenuItems[i]);
            JonesHud_apMenuItems[i] = NULL;
        }
    }

    if ( JonesHud_pHudCamera )
    {
        rdCamera_Free(JonesHud_pHudCamera);
        JonesHud_pHudCamera = NULL;
    }

    if ( JonesHud_pHudCanvas )
    {
        rdCanvas_Free(JonesHud_pHudCanvas);
        JonesHud_pHudCanvas = NULL;
    }

    if ( JonesHud_pMenuFont )
    {
        rdFont_Free(JonesHud_pMenuFont);
        JonesHud_pMenuFont = NULL;
    }

    if ( JonesHud_pCurInvChangedItem )
    {
        JonesHud_FreeMenuItem(JonesHud_pCurInvChangedItem);
        JonesHud_pCurInvChangedItem = NULL;
    }

    JonesHud_pCurSelectedMenuItem = NULL;
    JonesHud_pCloseMenuItem       = NULL;
    JonesHud_pCurInvChangedItem   = NULL;

    JonesHud_bExitActivated         = 0;
    JonesHud_dword_55515C           = 0;
    JonesHud_bRestoreActivated      = 0;
    JonesHud_bRestoreGameStatistics = 0;
    JonesHud_bItemActivated         = 0;
    JonesHud_bMenuEnabled           = 0;
    JonesHud_bHealthIndFadeSet      = 0;
    JonesHud_bCutsceneStart         = 0;
    JonesHud_bInterfaceEnabled      = 0;
    jonesInventory_Close();

    // TODO: Why is this check so far down here? Maybe should be moved to the start of function
    if ( !JonesHud_bOpen )
    {
#line 658
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
    }
    else
    {
        JonesHud_bOpen = 0;
    }
}

void J3DAPI JonesHud_ToggleMenu(void)
{
    if ( sithPlayer_pLocalPlayerThing && (sithPlayer_pLocalPlayerThing->flags & SITH_TF_DYING || sithPlayer_pLocalPlayerThing->flags & SITH_TF_DESTROYED || JONESHUD_DEBUG_GAMEOVER) )
    {
        // Retail zeroes the four adjacent jonesCog HUD flags with one 16-byte memset
        memset(&jonesCog_bShowHealthHUD, 0, 4 * sizeof(int));
        JonesHud_hudState &= ~0x10;
        JonesHud_ShowGameOverDialog(1);
    }
    else if ( JonesHud_bMenuEnabled )
    {
        if ( (JonesHud_hudState & 1) == 0 )
        {
            if ( !JonesHud_pCurInvChangedItem )
            {
                sithGamesave_SetThumbnailImage(&stdDisplay_backBuffer);
                jonesCog_bMenuVisible = 1;
                JonesHud_bShowMenu      = 0;
            }
            else
            {
                JonesHud_bShowMenu = 1;
            }

            JonesHud_healthIndAlpha = 1.0f;
        }
        else if ( (JonesHud_hudState & 4) == 0 && (JonesHud_hudState & 8) == 0 && (JonesHud_hudState & 1) != 0 )
        {
            jonesCog_bMenuVisible = 0;
            JonesHud_MenuClose();
        }
    }
}

void J3DAPI JonesHud_EnableMenu(int bEnable)
{
    JonesHud_bMenuEnabled = bEnable;
}

int J3DAPI JonesHud_IsMenuEnabled(void)
{
    return JonesHud_bMenuEnabled;
}

void J3DAPI JonesHud_Process(void)
{
    SithWorld* pWorld;
    rdCamera* pCurCam;
    int state;
    float maxHealth;
    float hp;
    float raftState;
    float impLevel;
    int curWeaponId;

    pWorld = sithWorld_pCurrentWorld;
    if ( !sithPlayer_pLocalPlayerThing )
    {
        return;
    }

    // Update HUD screen vars
    JonesHud_Update(pWorld);

    if ( sithPlayer_pLocalPlayerThing->type
        && (sithPlayer_pLocalPlayerThing->flags & SITH_TF_DYING || sithPlayer_pLocalPlayerThing->flags & SITH_TF_DESTROYED || JONESHUD_DEBUG_GAMEOVER) )
    {
        // Player died, wait 5 sec then show game over dialog
        sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health = 0.0f;
        memset(&jonesCog_bShowHealthHUD, 0, 4 * sizeof(int));

        if ( (JonesHud_hudState & 0x10) == 0 )
        {
            JonesHud_hudState |= 0x10;
            JonesHud_msecGameOverWaitDeltaTime = JonesHud_msecTime + 5000;
        }
        else if ( JonesHud_HasTimeElapsed(5000, JonesHud_msecGameOverWaitDeltaTime, JonesHud_msecTime) )
        {
            JonesHud_ShowGameOverDialog(1);
            return;
        }
    }

    if ( JonesHud_bCutsceneStart && (JonesHud_hudState & 1) == 0 && !jonesCog_bMenuVisible )
    {
        // Menu is closed, start previously started cutscene
        JonesHud_bCutsceneStart = 0;
        jonesCog_StartCutscene(NULL);
    }

    if ( JonesHud_bInterfaceEnabled && (JonesHud_hudState & 1) == 0 && !jonesCog_bMenuVisible )
    {
        // Menu is closed show in game HUD
        JonesHud_bInterfaceEnabled = 0;
        jonesCog_ToggleInterface(NULL);
    }

    if ( JonesHud_bExitActivated && !JonesHud_hudState )
    {
        JonesMain_CloseWindow();
        return;
    }

    if ( JonesHud_bRestoreActivated && !JonesHud_hudState && strlen(JonesHud_aSlectedNdsFilePath) > 0 )
    {
        sithSoundMixer_StopAll();
        if ( sithCamera_pCurCamera )
        {
            sithCamera_ResetAllCameras();
        }

        sithGamesave_Restore(JonesHud_aSlectedNdsFilePath, 1);
        memset(JonesHud_aSlectedNdsFilePath, 0, sizeof(JonesHud_aSlectedNdsFilePath));
        JonesHud_bRestoreActivated = 0;
        return;
    }

    if ( JonesHud_bMapOpen && !sithPlayerControls_bCutsceneMode && (JonesHud_hudState & 1) == 0 )
    {
        JonesHud_bMapOpen = 0;
        sithOverlayMap_ToggleMap();
    }

    // Clear render cache before switching to rendering the HUD
    rdCache_FlushAlpha();
    rdCache_Flush();

    // Change cur camera to HUD camera
    pCurCam = rdCamera_pCurCamera;
    rdCamera_SetCurrent(JonesHud_pHudCamera);
    rdCamera_Update(&JonesHud_camViewMatrix);

    if ( !JonesHud_RenderFadeHealthIndicator(JonesHud_bFadeHealthHUD) )
    {
        if ( !JonesHud_bHealthIndFadeSet || JonesHud_bHealthIndFadeSet && JonesHud_healthIndAlpha >= 0.2f )
        {
            hp        = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health;
            maxHealth = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.maxHealth;
            if ( maxHealth > 0.0f )
            {
                state = (int)(300.0f * hp / maxHealth);
            }
            else
            {
                state = 0;
            }

            JonesHud_RenderHealthIndicator((float)state);
        }

        // Draw endurance indicator
        if ( sithPlayer_pLocalPlayerThing->pInSector && (sithPlayer_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            JonesHud_RenderEnduranceIndicator((float)(100 * sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.endurance.msecUnderwater / SITHACTOR_MAX_UNDERWATER_MSEC));
        }
        else if ( (sithPlayer_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            raftState = (float)sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.endurance.raftLeakDamage * 0.001f;
            JonesHud_RenderEnduranceIndicator(raftState);
        }
        else
        {
            curWeaponId = sithInventory_GetCurrentWeapon(sithPlayer_pLocalPlayerThing);
            if ( (impLevel = (sithPlayer_impState * 1000.0f) / 1800.0f) > 0.0f && impLevel <= 100.0f )
            {
                JonesHud_bIMPState = 1;
                JonesHud_RenderEnduranceIndicator(impLevel);
            }
            else
            {
                JonesHud_bIMPState = 0;
            }
        }
    }

    if ( JonesHud_pCurInvChangedItem ) // Is item pickup animation playing?
    {
        JonesHud_RenderInventoryItemChange();
    }
    else
    {
        sithInventory_SetCurrentItem(sithPlayer_pLocalPlayerThing, 0);
        if ( (JonesHud_hudState & 1) != 0 ) // Is inventory menu opened?
        {
            if ( (Sound_GetChannelFlags(JonesHud_hCurSndChannel) & SOUND_CHANNEL_PLAYING) == 0 )
            {
                JonesHud_hCurSndChannel = 0;
            }

            JonesHud_ProcessMenuItems();
        }
        else if ( jonesCog_bMenuVisible )
        {
            if ( sithOverlayMap_IsMapVisible() )
            {
                JonesHud_bMapOpen = 1;
                sithOverlayMap_ToggleMap();
            }

            stdControl_ResetMousePos();
            JonesMain_PauseGame();
            JonesHud_MenuOpen();
        }
        else if ( JonesHud_bShowMenu )
        {
            sithGamesave_SetThumbnailImage(&stdDisplay_backBuffer);
            jonesCog_bMenuVisible = 1;
            JonesHud_healthIndAlpha = 1.0f;
            JonesHud_bShowMenu      = 0;
        }
    }

    if ( JonesHud_selectedWeaponID != -1 && !sithWeapon_IsMountingWeapon(sithPlayer_pLocalPlayerThing) )
    {
        sithWeapon_SelectWeapon(sithPlayer_pLocalPlayerThing, JonesHud_selectedWeaponID);
        JonesHud_selectedWeaponID = -1;
    }

    rdCache_FlushAlpha();
    rdCache_Flush();
    rdCamera_SetCurrent(pCurCam);
    sithWorld_pCurrentWorld->state &= ~SITH_WORLD_STATE_INIT_HUD;
}

void J3DAPI JonesHud_Update(const SithWorld* pWorld)
{
    size_t curTime;
    uint32_t width;
    float prevHeightScalar;
    float prevWidth;
    int bCanvasUpdated;
    uint32_t height;

    prevWidth        = JonesHud_screenWidthScalar;
    prevHeightScalar = JonesHud_screenHeightScalar;
    bCanvasUpdated   = 0;
    curTime          = stdPlatform_GetTimeMsec();

    stdDisplay_GetBackBufferSize(&width, &height);
    JonesHud_screenWidthScalar  = D3DVAL(width / 640.0);
    JonesHud_screenHeightScalar = D3DVAL(height / 480.0);

    if ( (pWorld->state & SITH_WORLD_STATE_INIT_HUD) != 0 )
    {
        JonesHud_msecMenuItemMoveDuration = 250;
        JonesHud_hudState                 = 0;
        JonesHud_bHealthIndFadeSet        = 0;
        JonesHud_bExitActivated           = 0;
        JonesHud_dword_55515C             = 0;
        JonesHud_bItemActivated           = 0;
        JonesHud_bShowMenu                = 0;
        JonesHud_msecDeltaTime            = 1;
        JonesHud_bCutsceneStart           = 0;
        JonesHud_bInterfaceEnabled        = 0;

        JonesHud_InitializeGameStatistics();
        bCanvasUpdated = JonesHud_SetCanvasSize(width, height);
    }
    else if ( curTime > JonesHud_msecTime )
    {
        JonesHud_msecDeltaTime = curTime - JonesHud_msecTime;
    }
    else
    {
        JonesHud_msecDeltaTime = 0xFFFFFFFF - JonesHud_msecTime + curTime;
    }

    JonesHud_msecTime = curTime;
    if ( prevWidth != JonesHud_screenWidthScalar || prevHeightScalar != JonesHud_screenHeightScalar )
    {
        JonesHud_UpdateHUDLayout(width, height);
        JonesHud_UpdateSinCosTable();
        if ( !bCanvasUpdated )
        {
            JonesHud_SetCanvasSize(width, height);
        }
    }
}

int J3DAPI JonesHud_SetCanvasSize(int width, int height)
{
    if ( JonesHud_pHudCamera && JonesHud_pHudCamera->pFrustum )
    {
        rdCamera_SetOverlayDepthRange(1);
        if ( JonesHud_pHudCanvas )
        {
            rdCanvas_Free(JonesHud_pHudCanvas);
            JonesHud_pHudCanvas = NULL;
            JonesHud_pHudCanvas = rdCanvas_New(0, &stdDisplay_backBuffer, 64, (int)JonesHud_healthIndRect.y, 576, (int)(JonesHud_healthIndRect.y + 64.0f));
            rdCamera_SetCanvas(JonesHud_pHudCamera, JonesHud_pHudCanvas);
        }

        rdCamera_SetFrustrum(JonesHud_pHudCamera, JonesHud_pHudCamera->pFrustum, 0, 0, width - 1, height - 1);
        rdCamera_SetOverlayDepthRange(0);
    }
    else
    {
        return 0;
    }

    return 1;
}

void J3DAPI JonesHud_UpdateSinCosTable(void)
{
    int i;
    for ( i = 0; i < 24; ++i )
    {
        JonesHud_aCosSinTableScaled[i][0] = JonesHud_aCosSinTable[i][0] * JonesHud_indScale; // cos
        JonesHud_aCosSinTableScaled[i][1] = JonesHud_aCosSinTable[i][1] * JonesHud_indScale; // sin
    }
}

void J3DAPI JonesHud_UpdateHUDLayout(uint32_t width, uint32_t height)
{
    // These coords are all in screen size
    JonesHud_healthIndRect.x      = JonesHud_screenWidthScalar * 24.0f;
    JonesHud_healthIndRect.y      = height - JonesHud_healthIndRect.x - JonesHud_screenHeightScalar * 60.0f;
    JonesHud_healthIndRect.width  = JonesHud_screenWidthScalar * 60.0f;
    JonesHud_healthIndRect.height = JonesHud_screenHeightScalar * 60.0f;

    // z & w set later when drawing hud indicator
    rdVector_Set4(&JonesHud_healthIndBarPos, (JonesHud_healthIndRect.width / 2.0f) + JonesHud_healthIndRect.x, (JonesHud_healthIndRect.height / 2.0f) + JonesHud_healthIndRect.y, 0.0f, 0.0f);

    JonesHud_enduranceRect.y      = JonesHud_healthIndRect.y;
    JonesHud_enduranceRect.height = JonesHud_healthIndRect.height;
    JonesHud_enduranceRect.width  = JonesHud_healthIndRect.width;
    JonesHud_enduranceRect.x      = width - JonesHud_healthIndRect.x - JonesHud_enduranceRect.width;

    // z & w set later when drawing hud indicator
    rdVector_Set4(&JonesHud_enduranceIndBarPos, (JonesHud_enduranceRect.width / 2.0f) + JonesHud_enduranceRect.x, (JonesHud_enduranceRect.height / 2.0f) + JonesHud_enduranceRect.y, 0.0f, 0.0f);

    JonesHud_indScale = D3DVAL((JonesHud_healthIndRect.height - 2.0) / 2.0);
    JonesHud_indScale = JonesHud_indScale - 7.0f * JonesHud_screenWidthScalar;

    // Unused stuff
    JonesHud_flt_55503C = 16.0f * JonesHud_screenHeightScalar;
    JonesHud_flt_555038 = 16.0f * JonesHud_screenWidthScalar;
    JonesHud_flt_555034 = -JonesHud_flt_55503C;

    JonesHud_dword_554FDC = 0;

    JonesHud_flt_554FE4 = 16.0f * JonesHud_screenHeightScalar;
    JonesHud_flt_554FE0 = 16.0f * JonesHud_screenWidthScalar;
}

void J3DAPI JonesHud_MenuOpen(void)
{
    int i;
    float moveRate;

    if ( !sithPlayer_pLocalPlayerThing || JonesHud_pMenuItemLinkedList && (JonesHud_hudState & 1) != 0 )
    {
        return;
    }

    JonesHud_selectedWeaponMenuItemID = 0;
    for ( i = 0; i <= 11; ++i ) // 11 - num player weapons
    {
        if ( JonesHud_apMenuItems[i]
            && JonesHud_apMenuItems[i]->inventoryID == sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.curWeaponID )
        {
            JonesHud_selectedWeaponMenuItemID = i;
            break;
        }
    }

    JonesHud_selectedTreasuresMenuItemID = JONESHUD_MENU_TREASURE_CHEST;
    JonesHud_selectedItemsMenuItemID     = JONESHUD_MENU_INVITEM_ZIPPO;
    JonesHud_selectedOptionsMenuItemID   = JONESHUD_MENU_IQ;
    JonesHud_pCurSelectedMenuItem        = NULL;
    JonesHud_pMenuItemLinkedList         = NULL;

    JonesHud_ResetMenuItems();
    if ( !JonesHud_pMenuItemLinkedList )
    {
        return;
    }

    if ( JonesHud_aSoundFxHandles[3] ) // inv_expand.wav
    {
        JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
    }

    JonesHud_rootMenuItemId = JonesHud_pMenuItemLinkedList->id;
    JonesHud_pMenuItemLinkedList->flags |= 0x83;
    JonesHud_hudState |= 1; // mark menu opened

    moveRate = JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x;
    JonesHud_StartItemTranslation(JonesHud_pMenuItemLinkedList, 100, (float)fabs(moveRate), 0);
}

void J3DAPI JonesHud_MenuClose(void)
{
    float moveRate;
    JonesHudMenuItem* pItem;
    int i;

    JonesHud_pCurSelectedMenuItem   = NULL;
    JonesHud_item_flag_state_55514C = 0;

    pItem = JonesHud_pMenuItemLinkedList;
    for ( i = 0; i < 4; ++i ) // collapses up to 4 items
    {
        if ( pItem->nextUpItemId != -1
            && JonesHud_apMenuItems[pItem->nextUpItemId]
            && ((JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 1) != 0 || (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x20) != 0) )
        {
            JonesHud_item_flag_state_55514C |= 1 << i;
            if ( JonesHud_aSoundFxHandles[3] ) // inv_expand.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }

            JonesHud_CollapseMenuColumn(pItem);
        }

        pItem->flags |= i << 10;
        if ( pItem->nextRightItemId != -1
            && JonesHud_apMenuItems[pItem->nextRightItemId]
            && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId
            && (JonesHud_apMenuItems[pItem->nextRightItemId]->flags & 1) != 0 )
        {
            pItem = JonesHud_apMenuItems[pItem->nextRightItemId];
        }
        else
        {
            break;
        }
    }

    JonesHud_pCloseMenuItem = pItem;
    if ( JonesHud_item_flag_state_55514C == 0 )
    {
        if ( JonesHud_aSoundFxHandles[3] ) // inv_expand.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_pCloseMenuItem->flags |= 0x40;
        JonesHud_pCloseMenuItem->flags &= ~0x80;
        rdVector_Copy3(&JonesHud_pCloseMenuItem->startMovePos, &JonesHud_pCloseMenuItem->pos);

        if ( JonesHud_pCloseMenuItem == JonesHud_pMenuItemLinkedList )
        {
            JonesHud_pCloseMenuItem->endMovePos.x = -0.158f;
            moveRate = JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x;
        }
        else
        {
            JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_apMenuItems[JonesHud_pCloseMenuItem->nextLeftItemId]->pos.x;
            if ( JonesHud_pCloseMenuItem->endMovePos.x == JonesHud_pCloseMenuItem->startMovePos.x )
            {
                if ( JonesHud_apMenuItems[JonesHud_pCloseMenuItem->nextLeftItemId] == JonesHud_pMenuItemLinkedList )
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = -0.158f;
                    moveRate = JonesHud_pMenuItemLinkedList->endMovePos.x - JonesHud_pMenuItemLinkedList->startMovePos.x;
                }
                else
                {
                    JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_pCloseMenuItem->endMovePos.x - 0.065f;
                    moveRate = 0.065f;
                }
            }
        }

        JonesHud_StartItemTranslation(JonesHud_pCloseMenuItem, 50, (float)fabs(moveRate), 0);
    }

    JonesHud_hudState |= 0x04; // Closing menu
}

void J3DAPI JonesHud_StartItemTranslation(JonesHudMenuItem* pItem, uint32_t msecDuration, float moveRate, int bMoveUp)
{
    float delta;

    if ( bMoveUp )
    {
        delta = D3DVAL(fabs(pItem->endMovePos.z - pItem->startMovePos.z) / moveRate);
    }
    else
    {
        delta = D3DVAL(fabs(pItem->endMovePos.x - pItem->startMovePos.x) / moveRate);
    }

    pItem->msecMoveDuration = (int)(msecDuration * delta);
    pItem->msecMoveEndTime  = JonesHud_msecTime + pItem->msecMoveDuration;
    pItem->flags |= 0x01; // enable item rotation anim
}

int J3DAPI JonesHud_HasTimeElapsed(unsigned int msecDuration, unsigned int msecDeltaTime, unsigned int msecTime)
{
    if ( msecDuration >= msecDeltaTime )
    {
        if ( msecTime > msecDuration && msecTime >= msecDeltaTime )
        {
            return 1;
        }

        return 0;
    }

    if ( msecTime > msecDeltaTime )
    {
        return 1;
    }

    return 0;
}

void J3DAPI JonesHud_RenderEnduranceIndicator(float enduranceState)
{
    rdVector4 color;

    if ( (sithPlayer_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
    {
        // Draw raft indicator
        if ( sithWorld_pCurrentWorld && (sithWorld_pCurrentWorld->state & SITH_WORLD_STATE_INIT_HUD) != 0 )
        {
            JonesHud_curEnduranceIndAlpha = 0.2f;
            JonesHud_curEnduranceState    = -1.0f;
        }

        if ( JonesHud_curEnduranceState != enduranceState )
        {
            JonesHud_curEnduranceIndAlpha      = 1.0f;
            JonesHud_msecRaftIndicatorFadeTime = JonesHud_msecTime;
        }
        else if ( JonesHud_curEnduranceIndAlpha > 0.2f
            && JonesHud_curEnduranceState == enduranceState
            && JonesHud_HasTimeElapsed(3000, JonesHud_msecRaftIndicatorFadeTime + 3000, JonesHud_msecTime)
            && (JonesHud_curEnduranceIndAlpha = JonesHud_curEnduranceIndAlpha - (JonesHud_msecDeltaTime * 0.8f) / 3000.0f) < 0.2f )
        {
            JonesHud_curEnduranceIndAlpha = 0.2f;
        }

        JonesHud_curEnduranceState = enduranceState;
        JonesHud_DrawEnduranceIndicator(enduranceState, JonesHud_curEnduranceIndAlpha);

        rdVector_Copy4(&color, &JonesHud_colorWhite);
        color.alpha = JonesHud_curEnduranceIndAlpha;
        JonesHud_Draw(JonesHud_pEnduranceOverlayMat, &JonesHud_enduranceRect, RD_FIXEDPOINT_RHW_SCALE_X1, RD_FIXEDPOINT_RHW_SCALE_X1, &color, 0, 1);
    }
    else
    {
        JonesHud_DrawEnduranceIndicator(enduranceState, 1.0f);

        rdVector_Copy4(&color, &JonesHud_colorWhite);
        color.alpha = 1.0f;
        JonesHud_Draw(JonesHud_pEnduranceOverlayMat, &JonesHud_enduranceRect, RD_FIXEDPOINT_RHW_SCALE_X1, RD_FIXEDPOINT_RHW_SCALE_X1, &color, 0, 1);
    }
}

void J3DAPI JonesHud_DrawEnduranceIndicator(float state, float alpha)
{
    JonesHudRect rect;
    rdVector4 barColor;
    rdVector4 arcBarColor;
    int pct;
    float fAngle;

    rect = JonesHud_enduranceRect;
    if ( state < 100.0f )
    {
        pct = (int)state % 100;
    }
    else
    {
        pct = 100;
    }

    fAngle = D3DVAL((pct * 0.01) * 360.0);
    rdVector_Copy4(&arcBarColor, &JonesHud_colorBlack);

    JonesHud_enduranceIndBarPos.z = RD_FIXEDPOINT_RHW_SCALE_X2; // draw above the base color (yellow, blue, pink)
    JonesHud_enduranceIndBarPos.w = RD_FIXEDPOINT_RHW_SCALE_X2;
    arcBarColor.alpha = alpha;

    if ( fAngle != 0.0f )
    {
        // Draws indicator bar arc
        JonesHud_DrawIndicatorBar(&JonesHud_enduranceIndBarPos, JonesHud_indScale, &arcBarColor, fAngle, 1);
    }

    if ( (sithPlayer_pLocalPlayerThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
    {
        rdVector_Copy4(&barColor, &JonesHud_colorYellow);
    }
    else if ( sithPlayer_pLocalPlayerThing->pInSector && (sithPlayer_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        rdVector_Copy4(&barColor, &JonesHud_skyBlue);
    }
    else if ( JonesHud_bIMPState )
    {
        // IM parts bar color
        rdVector_Copy4(&barColor, &JonesHud_colorPink);
    }

    barColor.alpha = alpha;

    rect.x      = JonesHud_enduranceRect.width / 2.0f - JonesHud_indScale + rect.x;
    rect.y      = JonesHud_enduranceRect.width / 2.0f - JonesHud_indScale + rect.y;
    rect.width  = JonesHud_indScale * 2.0f;
    rect.height = JonesHud_indScale * 2.0f;

    JonesHud_Draw(JonesHud_pHudBaseMat, &rect, RD_FIXEDPOINT_RHW_SCALE_X3, RD_FIXEDPOINT_RHW_SCALE_X3, &barColor, 0, 1);
}

void J3DAPI JonesHud_SetFadeHealthHUD(int bShow, int bFade)
{
    JonesHud_bFadeHealthHUD = bFade;
    if ( !bShow && !JonesHud_bFadeHealthHUD )
    {
        JonesHud_healthIndAlpha = 0.0f;
    }
}

int J3DAPI JonesHud_RenderFadeHealthIndicator(int bFade)
{
    float curHealth;
    float delta;
    int pct;
    int maxHp;

    if ( bFade )
    {
        JonesHud_bFadingHealthHUD = 1;
        JonesHud_bFadeHealthHUD   = 0;
    }

    if ( (JonesHud_hudState & 1) != 0 )
    {
        JonesHud_bFadingHealthHUD = 0;
    }

    if ( JonesHud_msecDeltaTime == 1 )
    {
        JonesHud_curFadeHealthState = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health;
    }
    else
    {
        if ( JonesHud_curFadeHealthState != sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health )
        {
            JonesHud_healthIndAlpha   = 1.0f;
            JonesHud_bFadingHealthHUD = 0;
        }

        JonesHud_curFadeHealthState = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health;
    }

    if ( !JonesHud_bFadingHealthHUD )
    {
        if ( !jonesCog_bShowHealthHUD )
        {
            return 1;
        }

        return 0;
    }

    // Fade in or fade out health HUD
    delta = (JonesHud_msecDeltaTime * 0.8f) / 3000.0f;
    if ( jonesCog_bShowHealthHUD )
    {
        if ( JonesHud_healthIndAlpha < 0.2f )
        {
            if ( (JonesHud_healthIndAlpha = JonesHud_healthIndAlpha + delta) > 0.2f )
            {
                JonesHud_healthIndAlpha = 0.2f;
            }
        }
        else
        {
            JonesHud_bFadingHealthHUD = 0;
        }
    }
    else if ( JonesHud_healthIndAlpha > 0.0f )
    {
        if ( (JonesHud_healthIndAlpha = JonesHud_healthIndAlpha - delta) < 0.0f )
        {
            JonesHud_healthIndAlpha = 0.0f;
        }
    }
    else
    {
        JonesHud_bFadingHealthHUD = 0;
    }

    if ( JonesHud_healthIndAlpha == 0.0f || JonesHud_healthIndAlpha == 0.2f )
    {
        JonesHud_bFadingHealthHUD = 0;
    }

    if ( JonesHud_healthIndAlpha > 0.0f )
    {
        curHealth = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health;
        maxHp     = (int)sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.maxHealth;
        pct       = 300 * (int)curHealth / maxHp;
        JonesHud_DrawHealthIndicator(0.0f, (float)pct, JonesHud_healthIndAlpha);
    }

    return 1;
}

void J3DAPI JonesHud_DrawHealthIndicator(float hitDelta, float health, float alpha)
{
    rdVector4 color;
    rdVector_Copy4(&color, &JonesHud_colorWhite);
    if ( (sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_POISONED) != 0 )
    {
        // Draws poisoned mat above all components in health indicator
        color.alpha = 0.25f;
        JonesHud_Draw(JonesHud_pPoisonedOverlayMat,
            &JonesHud_healthIndRect,
            RD_FIXEDPOINT_RHW_SCALE_X1, // sz
            RD_FIXEDPOINT_RHW_SCALE_X1, // rhw
            &color,
            /*celNum=*/0,
            /*bAlpha=*/1
        );
    }

    // Draw hit flash effect
    if ( hitDelta > 0.0f )
    {
        JonesHud_DrawHealthHitOverlay(hitDelta, RD_FIXEDPOINT_RHW_SCALE_X2, RD_FIXEDPOINT_RHW_SCALE_X2);// 0.000061036088f, will draw below poison overlay mat
    }

    color.alpha = alpha;
    JonesHud_Draw(JonesHud_pHealthOverlay, &JonesHud_healthIndRect, RD_FIXEDPOINT_RHW_SCALE_X3, RD_FIXEDPOINT_RHW_SCALE_X3, &color, 0, 1);// 0.000091554131f, draws overlay below previously drawn components

    // Draws health bar in a style of a "pie chart", in colors yellow, red or black
    // Then draw_icon base beneath underneath with health bar circle in color green, yellow, red  
    JonesHud_DrawHealthIndicatorBar(health, alpha, RD_FIXEDPOINT_RHW_SCALE_X4, RD_FIXEDPOINT_RHW_SCALE_X4);// 0.00012207218f, draws pie chart bar below previously drawn components and above base (color: yellow or red or black)
    JonesHud_DrawHealthIndicatorBase(health, alpha, RD_FIXEDPOINT_RHW_SCALE_X5, RD_FIXEDPOINT_RHW_SCALE_X5);// 0.00015259022f, draws base below all previous components (color: green, yellow or red)
}

void J3DAPI JonesHud_DrawHealthHitOverlay(float hitDelta, float z, float rhw)
{
    rdVector4 color;
    float delta;

    if ( JonesHud_hitOverlayAlphaState == 0.0f && JonesHud_hitOverlayAlpha == 0.0f )
    {
        JonesHud_hitOverlayAlphaState = ((delta = hitDelta - (int)hitDelta % 20) + 20.0f) / 300.0f;
    }

    if ( JonesHud_hitOverlayAlpha < JonesHud_hitOverlayAlphaState )
    {
        if ( (JonesHud_hitOverlayAlpha = JonesHud_hitOverlayAlpha + ((JonesHud_msecDeltaTime * JonesHud_hitOverlayAlphaState * 2.0f) / 250.0f)) > JonesHud_hitOverlayAlphaState )
        {
            JonesHud_hitOverlayAlpha = JonesHud_hitOverlayAlphaState;
        }
    }
    else if ( (JonesHud_hitOverlayAlpha = JonesHud_hitOverlayAlpha - (JonesHud_msecDeltaTime * JonesHud_hitOverlayAlphaState * 2.0f) / 250.0f) <= 0.0f )
    {
        JonesHud_hitOverlayAlpha      = 0.0f;
        JonesHud_hitOverlayAlphaState = 0.0f;
        return;
    }

    rdVector_Copy4(&color, &JonesHud_colorWhite);
    color.alpha = JonesHud_hitOverlayAlpha;
    JonesHud_Draw(JonesHud_pHealthOverlayHitMat, &JonesHud_healthIndRect, z, rhw, &color, 0, 1);
}

void J3DAPI JonesHud_RenderHealthIndicator(float healthState)
{
    float health;

    if ( !sithPlayer_pLocalPlayerThing )
    {
        return;
    }

    if ( !JonesHud_bHealthIndFadeSet )
    {
        JonesHud_curHealth                   = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health;
        JonesHud_msecHealthIndLastFadeUpdate = JonesHud_msecTime;
        JonesHud_bHealthIndFadeSet           = 1;
    }

    health = sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health;
    if ( JonesHud_pCurInvChangedItem && JonesHud_pCurInvChangedItem->pos.x <= -0.13f
        || (JonesHud_hudState & 1) != 0
        || health == 0.0f
        || JonesHud_curHealth != health )
    {
        JonesHud_healthIndAlpha              = 1.0f;
        JonesHud_msecHealthIndLastFadeUpdate = JonesHud_msecTime;
    }
    else if ( JonesHud_healthIndAlpha > 0.2f
        && JonesHud_curHealth == health
        && JonesHud_HasTimeElapsed(3000, JonesHud_msecHealthIndLastFadeUpdate + 3000, JonesHud_msecTime)
        && (JonesHud_healthIndAlpha = JonesHud_healthIndAlpha - (JonesHud_msecDeltaTime * 0.8f) / 3000.0f) < 0.2f )
    {
        JonesHud_healthIndAlpha = 0.2f;
    }

    JonesHud_DrawHealthIndicator(JonesHud_curHealth - health, healthState, JonesHud_healthIndAlpha);
    JonesHud_curHealth = health;
}

void J3DAPI JonesHud_DrawHealthIndicatorBase(float state, float alpha, float z, float w)
{
    JonesHudRect rect;
    rdVector4 color;

    rect = JonesHud_healthIndRect;
    if ( state < 100.01 )
    {
        rdVector_Copy4(&color, &JonesHud_colorRed);
    }
    else if ( state < 200.01 )
    {
        rdVector_Copy4(&color, &JonesHud_colorYellow);
    }
    else
    {
        rdVector_Copy4(&color, &JonesHud_colorGreen);
    }

    color.alpha = alpha;

    rect.x      = JonesHud_healthIndRect.width / 2.0f - JonesHud_indScale + rect.x;
    rect.y      = JonesHud_healthIndRect.width / 2.0f - JonesHud_indScale + rect.y;
    rect.width  = JonesHud_indScale * 2.0f;
    rect.height = JonesHud_indScale * 2.0f;

    JonesHud_Draw(JonesHud_pHudBaseMat, &rect, z, w, &color, 0, 1);
}

void J3DAPI JonesHud_DrawHealthIndicatorBar(float health, float alpha, float z, float w)
{
    int progress;
    int roundAngle;
    float angle;
    rdVector4 color;

    progress   = (int)health % 100;
    roundAngle = (int)(angle = D3DVAL(((100 - progress) * 0.01) * 360.0));
    if ( !(roundAngle % 360) && health > 0.0f )
    {
        return;
    }

    if ( health < 100.01 )
    {
        rdVector_Copy4(&color, &JonesHud_colorBlack);
    }
    else if ( health < 200.01 )
    {
        rdVector_Copy4(&color, &JonesHud_colorRed);
    }
    else
    {
        rdVector_Copy4(&color, &JonesHud_colorYellow);
    }

    JonesHud_healthIndBarPos.z = z;
    JonesHud_healthIndBarPos.w = w;
    color.alpha = alpha;
    JonesHud_DrawIndicatorBar(&JonesHud_healthIndBarPos, JonesHud_indScale, &color, angle, 0);
}

void J3DAPI JonesHud_DrawIndicatorBar(const rdVector4* pPos, float scale, const rdVector4* pColor, float angle, int bArc)
{
    float startAngle;
    float loopAngle;
    float deltaAngle;
    int vertNum;
    LPD3DTLVERTEX pVert;
    D3DTLVERTEX vert;
    float maxAngle;
    int bSplit;
    rdCacheProcEntry* pPoly;
    rdCacheProcEntry* pCurrentPoly;
    rdCacheProcEntry* pSplitPoly;

    pPoly      = NULL;
    pSplitPoly = NULL;
    pPoly      = rdCache_GetAlphaProcEntry();
    if ( !pPoly )
    {
        return;
    }

    pPoly->flags        = RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->pMaterial    = JonesHud_pHudBaseMat;
    pPoly->matCelNum    = 0;
    pPoly->numVertices  = 0;

    vert.sx  = D3DVAL(pPos->x);
    vert.sy  = D3DVAL(pPos->y);
    vert.sz  = D3DVAL(pPos->z);
    vert.rhw = D3DVAL(pPos->w);
    vert.tu  = D3DVAL(0.5f);
    vert.tv  = D3DVAL(0.5f);

    vertNum = 0;
    pVert   = pPoly->aVertices;
    *pVert  = vert;
    ++pPoly->numVertices;
    ++vertNum;

    pCurrentPoly = pPoly;
    if ( bArc )
    {
        startAngle = angle + 270.0f;
        maxAngle   = 270.0f;
    }
    else
    {
        startAngle = 450.0f;
        maxAngle   = startAngle - angle;
    }

    bSplit     = 0;
    loopAngle  = startAngle;
    deltaAngle = ((float)fmod(startAngle, 15.0));

    while ( loopAngle >= maxAngle )
    {
        pVert  = &pCurrentPoly->aVertices[vertNum];
        *pVert = vert;

        if ( fmod(fmod(loopAngle, 360.0), 30.0) == 0.0 )
        {
            int sinIdx;
            sinIdx    = (int)(fmod(loopAngle, 360.0) / 15.0);
            pVert->sx = D3DVAL(pVert->sx + (JonesHud_aCosSinTableScaled[sinIdx][0])); // cos
            pVert->sy = D3DVAL(pVert->sy - (JonesHud_aCosSinTableScaled[sinIdx][1])); // sin
        }
        else
        {
            float outSin;
            float outCos;
            stdMath_SinCos(loopAngle, &outSin, &outCos);
            pVert->sx = D3DVAL((outCos * scale) + pVert->sx);
            pVert->sy = D3DVAL((-outSin * scale) + pVert->sy);
        }

        ++pCurrentPoly->numVertices;

        if ( startAngle - loopAngle + 15.0f > 180.0 && loopAngle > maxAngle && bSplit == 0 )
        {
            rdVector_Copy4(&pCurrentPoly->extraLight, pColor);
            rdVector_Copy4(pCurrentPoly->aVertIntensities, pColor);
            rdCache_AddAlphaProcFace(pCurrentPoly->numVertices);

            pSplitPoly = rdCache_GetAlphaProcEntry();
            if ( !pSplitPoly )
            {
                return;
            }

            pSplitPoly->flags        = RD_FF_TEX_TRANSLUCENT;
            pSplitPoly->lightingMode = RD_LIGHTING_DIFFUSE;
            pSplitPoly->pMaterial    = JonesHud_pHudBaseMat;
            pSplitPoly->matCelNum    = 0;
            pSplitPoly->numVertices  = 0;

            pVert  = pSplitPoly->aVertices;
            *pVert = vert;
            pSplitPoly->aVertices[1] = pPoly->aVertices[vertNum];

            pCurrentPoly = pSplitPoly;
            vertNum      = 2;
            pCurrentPoly->numVertices = vertNum;
            bSplit       = 1;
        }
        else
        {
            ++vertNum;
        }

        if ( loopAngle != maxAngle )
        {
            if ( loopAngle == startAngle && deltaAngle != 0.0f )
            {
                loopAngle = loopAngle - deltaAngle;
            }
            else
            {
                loopAngle = loopAngle - 15.0f;
            }

            if ( loopAngle < maxAngle )
            {
                loopAngle = maxAngle;
            }
        }
        else
        {
            break;
        }
    }

    rdVector_Copy4(&pCurrentPoly->extraLight, pColor);
    rdVector_Copy4(pCurrentPoly->aVertIntensities, pColor);
    rdCache_AddAlphaProcFace(pCurrentPoly->numVertices);
}

void J3DAPI JonesHud_Draw(const rdMaterial* pMaterial, const JonesHudRect* rect, float z, float rhw, const rdVector4* pColor, int celNum, int bAlpha)
{
    rdCacheProcEntry* pPoly;
    LPD3DTLVERTEX pCurOutVert;

    if ( bAlpha )
    {
        pPoly = rdCache_GetAlphaProcEntry();
        pPoly->flags = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    }
    else
    {
        pPoly = rdCache_GetProcEntry();
        pPoly->flags = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X;
    }

    if ( !pPoly )
    {
        return;
    }

    pPoly->lightingMode = RD_LIGHTING_DIFFUSE;
    pPoly->numVertices  = 4;
    pPoly->pMaterial    = (rdMaterial*)pMaterial;
    pPoly->matCelNum    = celNum;
    rdVector_Copy4(&pPoly->extraLight, pColor);
    rdVector_Copy4(pPoly->aVertIntensities, pColor);

    pCurOutVert = pPoly->aVertices;
    pCurOutVert->sx  = D3DVAL(rect->x);
    pCurOutVert->sy  = D3DVAL(rect->y);
    pCurOutVert->sz  = D3DVAL(z);
    pCurOutVert->rhw = D3DVAL(rhw);
    pCurOutVert->tu  = D3DVAL(0.0f);
    pCurOutVert->tv  = D3DVAL(0.0f);
    ++pCurOutVert;

    pCurOutVert->sx  = D3DVAL(rect->x + rect->width);
    pCurOutVert->sy  = D3DVAL(rect->y);
    pCurOutVert->sz  = D3DVAL(z);
    pCurOutVert->rhw = D3DVAL(rhw);
    pCurOutVert->tu  = D3DVAL(1.0f);
    pCurOutVert->tv  = D3DVAL(0.0f);
    ++pCurOutVert;

    pCurOutVert->sx  = D3DVAL(rect->x + rect->width);
    pCurOutVert->sy  = D3DVAL(rect->y + rect->height);
    pCurOutVert->sz  = D3DVAL(z);
    pCurOutVert->rhw = D3DVAL(rhw);
    pCurOutVert->tu  = D3DVAL(1.0f);
    pCurOutVert->tv  = D3DVAL(1.0f);
    ++pCurOutVert;

    pCurOutVert->sx  = D3DVAL(rect->x);
    pCurOutVert->sy  = D3DVAL(rect->y + rect->height);
    pCurOutVert->sz  = D3DVAL(z);
    pCurOutVert->rhw = D3DVAL(rhw);
    pCurOutVert->tu  = D3DVAL(0.0f);
    pCurOutVert->tv  = D3DVAL(1.0f);

    if ( bAlpha )
    {
        rdCache_AddAlphaProcFace(4);
    }
    else
    {
        rdCache_AddProcFace(4);
    }
}

void J3DAPI JonesHud_ProcessMenuItems(void)
{
    int bActivateKeyPressed;
    int i;

    JonesHud_UpdateItem(JonesHud_pMenuItemLinkedList);

    if ( (JonesHud_hudState & 4) == 0 && (JonesHud_hudState & 8) == 0 )
    {
        if ( (JonesHud_hudState & 2) == 0 )
        {
            i = 0;
            bActivateKeyPressed = 0;
            while ( JonesHud_aActivateKeyIds[i] > 0 )
            {
                if ( JonesHud_GetKey(JonesHud_aActivateKeyIds[i]) )
                {
                    bActivateKeyPressed = 1;
                    break;
                }

                ++i;
            }

            bActivateKeyPressed |= JonesHud_GetKey(DIK_RETURN);
            bActivateKeyPressed |= JonesHud_GetKey(DIK_NUMPADENTER);
            if ( bActivateKeyPressed )
            {
                JonesHud_MenuActivateItem();
            }
            else if ( JonesHud_GetKey(DIK_UP) )
            {
                JonesHud_MenuMoveUp();
            }
            else if ( JonesHud_GetKey(DIK_DOWN) )
            {
                JonesHud_MenuMoveDown();
            }
        }

        if ( JonesHud_GetKey(DIK_RIGHT) )
        {
            JonesHud_MenuMoveRight();
        }
        else if ( JonesHud_GetKey(DIK_LEFT) )
        {
            JonesHud_MenuMoveLeft();
        }
    }

    JonesHud_RenderMenuItems(JonesHud_pMenuItemLinkedList);
}

void J3DAPI JonesHud_MenuMoveLeft(void)
{
    JonesHudMenuItem* pCur;
    JonesHudMenuItem* pLeftItem;

    if ( JonesHud_pCurSelectedMenuItem )
    {
        pCur = JonesHud_pCurSelectedMenuItem;
    }
    else
    {
        pCur = JonesHud_pMenuItemLinkedList;
    }

    if ( pCur->nextLeftItemId != -1 && JonesHud_apMenuItems[pCur->nextLeftItemId] )
    {
        pLeftItem = JonesHud_apMenuItems[pCur->nextLeftItemId];
        if ( (pLeftItem->flags & 0x01) != 0 && (pLeftItem->flags & 0x80) == 0 && pLeftItem->pos.x == pLeftItem->endMovePos.x )
        {
            JonesHud_pCurSelectedMenuItem = JonesHud_apMenuItems[pLeftItem->id];
            JonesHud_ExpandMenuColumn(JonesHud_apMenuItems[pLeftItem->id]);
            JonesHud_CollapseMenuColumn(pCur);

            if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }
        }
    }
}

void J3DAPI JonesHud_MenuMoveRight(void)
{
    JonesHudMenuItem* pCur;
    JonesHudMenuItem* pRightItem;

    if ( !JonesHud_pCurSelectedMenuItem )
    {
        if ( (JonesHud_pMenuItemLinkedList->flags & 0x01) != 0
            && (JonesHud_pMenuItemLinkedList->flags & 0x80) == 0
            && JonesHud_pMenuItemLinkedList->pos.x == JonesHud_pMenuItemLinkedList->endMovePos.x )
        {
            JonesHud_pCurSelectedMenuItem = JonesHud_pMenuItemLinkedList;
            JonesHud_ExpandMenuColumn(JonesHud_pMenuItemLinkedList);

            if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }
        }
    }
    else
    {
        pCur = JonesHud_pCurSelectedMenuItem;
        if ( pCur->nextRightItemId != -1 && JonesHud_apMenuItems[pCur->nextRightItemId] )
        {
            pRightItem = JonesHud_apMenuItems[pCur->nextRightItemId];
            if ( (pRightItem->flags & 0x01) != 0 && (pRightItem->flags & 0x80) == 0 && pRightItem->pos.x == pRightItem->endMovePos.x )
            {
                JonesHud_pCurSelectedMenuItem = JonesHud_apMenuItems[pRightItem->id];
                JonesHud_ExpandMenuColumn(JonesHud_apMenuItems[pRightItem->id]);
                JonesHud_CollapseMenuColumn(pCur);

                if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }
            }
        }
    }
}

void J3DAPI JonesHud_MenuMoveDown(void)
{
    JonesHudMenuItem* pDown;
    JonesHudMenuItem* pCurItem;
    int curItemId;

    if ( !JonesHud_pCurSelectedMenuItem )
    {
        return;
    }

    if ( JonesHud_pCurSelectedMenuItem->nextDownItemId != -1 && JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId] )
    {
        pCurItem  = JonesHud_pCurSelectedMenuItem;
        pDown     = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId];
        curItemId = pCurItem->id;
        JonesHud_pCurSelectedMenuItem = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextDownItemId];

        if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_SetSelectedMenuItem(pCurItem->id, pDown);

        pDown->flags |= 0x0A;
        pCurItem->flags &= ~0x02;

        pDown->nextLeftItemId  = pCurItem->nextLeftItemId;
        pDown->nextRightItemId = pCurItem->nextRightItemId;

        pCurItem->nextRightItemId = -1;
        pCurItem->nextLeftItemId  = -1;
        if ( pDown->nextLeftItemId != -1 && JonesHud_apMenuItems[pDown->nextLeftItemId] )
        {
            JonesHud_apMenuItems[pDown->nextLeftItemId]->nextRightItemId = pCurItem->nextDownItemId;
        }

        if ( pDown->nextRightItemId != -1 && JonesHud_apMenuItems[pDown->nextRightItemId] )
        {
            JonesHud_apMenuItems[pDown->nextRightItemId]->nextLeftItemId = pCurItem->nextDownItemId;
        }

        while ( JonesHud_apMenuItems[pDown->nextUpItemId]->pos.z <= JonesHud_maxItemZ
            && JonesHud_pCurSelectedMenuItem != JonesHud_apMenuItems[pDown->nextUpItemId] )
        {
            pDown = JonesHud_apMenuItems[pDown->nextUpItemId];
            rdVector_Copy3(&pDown->startMovePos, &pDown->pos);
            rdVector_Copy3(&pDown->endMovePos, &pDown->pos);
            pDown->endMovePos.z = pDown->endMovePos.z + 0.065f;
            pDown->flags |= 0x8;

            JonesHud_StartItemTranslation(pDown, JonesHud_msecMenuItemMoveDuration, 0.065f, 1);
            if ( (JonesHud_apMenuItems[pDown->nextUpItemId]->flags & 0x01) == 0
                || pDown->id == JonesHud_selectedWeaponMenuItemID
                || pDown->id == JonesHud_selectedItemsMenuItemID
                || pDown->id == JonesHud_selectedOptionsMenuItemID
                || pDown->id == JonesHud_selectedTreasuresMenuItemID )
            {
                pDown->flags |= 0x08;
                pDown->flags &= ~0x01;
                break;
            }
        }

        rdVector_Copy3(&JonesHud_pCurSelectedMenuItem->startMovePos, &pCurItem->pos);
        rdVector_Copy3(&JonesHud_pCurSelectedMenuItem->pos, &pCurItem->pos);
        rdVector_Copy3(&JonesHud_pCurSelectedMenuItem->endMovePos, &pCurItem->pos);
        JonesHud_pCurSelectedMenuItem->startMovePos.z = JonesHud_pCurSelectedMenuItem->startMovePos.z - 0.065f;
        JonesHud_pCurSelectedMenuItem->pos.z          = JonesHud_pCurSelectedMenuItem->pos.z - 0.065f;

        JonesHud_StartItemTranslation(JonesHud_pCurSelectedMenuItem, JonesHud_msecMenuItemMoveDuration, 0.065f, 1);
        JonesHud_pCurSelectedMenuItem->flags &= ~0x01; // dont rotate anim
        JonesHud_hudState |= 0x08;
    }
}

void J3DAPI JonesHud_SetSelectedMenuItem(int menuId, JonesHudMenuItem* pItem)
{
    if ( menuId == JonesHud_rootMenuItemId )
    {
        JonesHud_rootMenuItemId = pItem->id;
        JonesHud_pMenuItemLinkedList = pItem;
    }

    if ( menuId == JonesHud_selectedWeaponMenuItemID )
    {
        JonesHud_selectedWeaponMenuItemID = pItem->id;
    }

    else if ( menuId == JonesHud_selectedTreasuresMenuItemID )
    {
        JonesHud_selectedTreasuresMenuItemID = pItem->id;
    }

    else if ( menuId == JonesHud_selectedItemsMenuItemID )
    {
        JonesHud_selectedItemsMenuItemID = pItem->id;
    }

    else if ( menuId == JonesHud_selectedOptionsMenuItemID )
    {
        JonesHud_selectedOptionsMenuItemID = pItem->id;
    }
}

void J3DAPI JonesHud_MenuMoveUp(void)
{
    JonesHudMenuItem* pUp;
    JonesHudMenuItem* pCurItem;
    int curItemId;

    if ( !JonesHud_pCurSelectedMenuItem )
    {
        return;
    }

    if ( JonesHud_pCurSelectedMenuItem->nextUpItemId != -1 && JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextUpItemId] )
    {
        pUp       = JonesHud_apMenuItems[JonesHud_pCurSelectedMenuItem->nextUpItemId];
        pCurItem  = JonesHud_pCurSelectedMenuItem;
        curItemId = pCurItem->id;
        JonesHud_pCurSelectedMenuItem = pUp;

        if ( JonesHud_aSoundFxHandles[2] ) // inv_move.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[2], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_SetSelectedMenuItem(pCurItem->id, pUp);

        pUp->flags |= 0x2;
        pCurItem->flags &= ~0x2;

        pUp->nextLeftItemId       = pCurItem->nextLeftItemId;
        pUp->nextRightItemId      = pCurItem->nextRightItemId;
        pCurItem->nextRightItemId = -1;
        pCurItem->nextLeftItemId  = -1;

        if ( pUp->nextLeftItemId != -1 && JonesHud_apMenuItems[pUp->nextLeftItemId] )
        {
            JonesHud_apMenuItems[pUp->nextLeftItemId]->nextRightItemId = pCurItem->nextUpItemId;
        }

        if ( pUp->nextRightItemId != -1 && JonesHud_apMenuItems[pUp->nextRightItemId] )
        {
            JonesHud_apMenuItems[pUp->nextRightItemId]->nextLeftItemId = pCurItem->nextUpItemId;
        }

        while ( JonesHud_apMenuItems[pUp->nextDownItemId]->pos.z <= JonesHud_maxItemZ )
        {
            rdVector_Copy3(&pUp->startMovePos, &JonesHud_apMenuItems[pUp->nextDownItemId]->pos);
            rdVector_Copy3(&pUp->pos, &JonesHud_apMenuItems[pUp->nextDownItemId]->pos);
            rdVector_Copy3(&pUp->endMovePos, &JonesHud_apMenuItems[pUp->nextDownItemId]->pos);

            pUp->startMovePos.z = pUp->startMovePos.z + 0.065f;
            pUp->pos.z          = pUp->pos.z + 0.065f;
            pUp->flags |= 0x08;
            JonesHud_StartItemTranslation(pUp, JonesHud_msecMenuItemMoveDuration, 0.065f, 1);

            if ( pUp->pos.z > JonesHud_maxItemZ || pUp->nextUpItemId == curItemId )
            {
                break;
            }

            pUp = JonesHud_apMenuItems[pUp->nextUpItemId];
        }

        pCurItem->flags |= 0x8;

        if ( pUp->pos.z > JonesHud_maxItemZ )
        {
            rdVector_Copy3(&pCurItem->startMovePos, &pCurItem->pos);
            rdVector_Copy3(&pCurItem->endMovePos, &pCurItem->pos);
            pCurItem->endMovePos.z = pCurItem->endMovePos.z - 0.065f;

            pUp->pyr.pitch = (JonesHud_aDfltMenuItemOrients[pUp->id].pyr.pitch);
            pUp->pyr.roll  = (JonesHud_aDfltMenuItemOrients[pUp->id].pyr.roll);
            pUp->pyr.yaw   = (JonesHud_aDfltMenuItemOrients[pUp->id].pyr.yaw);
            pUp->flags &= ~0x01;
        }
        else
        {
            rdVector_Copy3(&pCurItem->startMovePos, &JonesHud_apMenuItems[pCurItem->nextDownItemId]->endMovePos);
            rdVector_Copy3(&pCurItem->pos, &JonesHud_apMenuItems[pCurItem->nextDownItemId]->endMovePos);
            rdVector_Copy3(&pCurItem->endMovePos, &JonesHud_apMenuItems[pCurItem->nextDownItemId]->pos);

            pCurItem->pyr.pitch = (JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr.pitch);
            pCurItem->pyr.roll  = (JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr.roll);
            pCurItem->pyr.yaw   = (JonesHud_aDfltMenuItemOrients[pCurItem->id].pyr.yaw);
        }

        JonesHud_StartItemTranslation(pCurItem, JonesHud_msecMenuItemMoveDuration, 0.065f, 1);
        pCurItem->flags &= ~0x01; // disable updating item rotation
        JonesHud_hudState |= 0x8;
    }
}

void J3DAPI JonesHud_BindActivateControlKeys(const size_t* aKeyIds, int numKeys)
{
    int i;

    memset(JonesHud_aActivateKeyIds, 0, 2 * sizeof(JonesHud_aActivateKeyIds[0]));
    for ( i = 0; i < numKeys; ++i )
    {
        JonesHud_aActivateKeyIds[i] = aKeyIds[i];
    }
}

int J3DAPI JonesHud_GetKey(unsigned int keyId)
{
    int pressCount;
    int bKeyPressed;

    pressCount = 0;
    if ( !JonesHud_bKeyStateUpdated
        || JonesHud_bKeyStateUpdated && JonesHud_HasTimeElapsed(JonesHud_msecMenuItemMoveDuration, JonesHud_msecLastKeyPressTime + JonesHud_msecMenuItemMoveDuration, JonesHud_msecTime) )
    {
        JonesHud_bKeyStateUpdated = 0;
        switch ( keyId )
        {
            case DIK_UP:
                bKeyPressed = sithControl_GetKey(SITHCONTROL_FORWARD, &pressCount);
                break;

            case DIK_DOWN:
                bKeyPressed = sithControl_GetKey(SITHCONTROL_BACK, &pressCount);
                break;

            case DIK_RIGHT:
                bKeyPressed = sithControl_GetKey(SITHCONTROL_TURNRIGHT, &pressCount);
                break;

            case DIK_LEFT:
                bKeyPressed = sithControl_GetKey(SITHCONTROL_TURNLEFT, &pressCount);
                break;

            default:
                bKeyPressed = stdControl_ReadKey(keyId, &pressCount);
                break;
        }

        if ( bKeyPressed )
        {
            if ( keyId == JonesHud_curKeyId )
            {
                if ( JonesHud_msecMenuItemMoveDuration < 250 && JonesHud_HasTimeElapsed(250, JonesHud_msecLastKeyPressTime + 250, JonesHud_msecTime) )
                {
                    JonesHud_msecMenuItemMoveDuration = 250;
                }
                else if ( JonesHud_msecMenuItemMoveDuration > 75 )
                {
                    JonesHud_msecMenuItemMoveDuration = (size_t)(JonesHud_msecMenuItemMoveDuration * 0.75);
                    if ( JonesHud_msecMenuItemMoveDuration < 75 )
                    {
                        JonesHud_msecMenuItemMoveDuration = 75;
                    }
                }
            }
            else
            {
                JonesHud_msecMenuItemMoveDuration = 250;
            }

            JonesHud_msecLastKeyPressTime = JonesHud_msecTime;
            JonesHud_bKeyStateUpdated     = 1;
            JonesHud_curKeyId             = keyId;
            return 1;
        }
        else
        {
            JonesHud_bKeyStateUpdated = 0;
            return 0;
        }
    }

    return 0;
}

int J3DAPI JonesHud_InitializeMenu(void)
{
    int i;
    rdModel3* pIcon;
    JonesHudMenuItem* pNewItem;
    int typeId;

    if ( JonesHud_pMenuItemLinkedList )
    {
#line 2586
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Menu already initialized.\n", 0, 0, 0, 0);
        return 0;
    }

    for ( i = 0; i < JONESHUD_MAX_MENU_ITEMS; ++i )
    {
        if ( !JonesHud_apMenuItems[i] && JonesHud_apMenuIconModelNames[i] )
        {
            pIcon = sithModel_GetModel(JonesHud_apMenuIconModelNames[i]);
            if ( pIcon )
            {
                JonesHud_apMenuItems[i] = JonesHud_NewMenuItem(pIcon);
                if ( !JonesHud_apMenuItems[i] )
                {
#line 2602
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't create menu item %s.\n", JonesHud_apMenuIconModelNames[i], 0, 0, 0);
                    return 0;
                }

                JonesHud_apMenuItems[i]->id = i;
            }
            else if ( i >= JONESHUD_MENU_IQ || i == JONESHUD_MENU_TREASURE_CHEST )
            {
#line 2612
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't find model %s.\n", JonesHud_apMenuIconModelNames[i], 0, 0, 0);
                return 0;
            }
        }
    }

    // Initialize inventory menu items
    for ( i = 0; i < JONESHUD_MAX_MENU_ITEMS; ++i )
    {
        pNewItem = JonesHud_apMenuItems[i];
        typeId   = 0;
        while ( pNewItem && pNewItem->inventoryID == -1 && typeId < 200 )
        {
            if ( sithInventory_aTypes[typeId].pInvIconModel == pNewItem->prdIcon->data.pModel3 )
            {
                pNewItem->inventoryID = typeId;
                break;
            }

            ++typeId;
        }
    }

    return 1;
}

void J3DAPI JonesHud_InitializeMenuSounds(void)
{
    int i;
    for ( i = 0; i < 13; ++i )
    {
        if ( JonesHud_aSoundFxIdxs[i] > -1 )
        {
            JonesHud_aSoundFxHandles[i] = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(JonesHud_aSoundFxIdxs[i]));
        }
    }
}

JonesHudMenuItem* J3DAPI JonesHud_NewMenuItem(rdModel3* pItemIcon3)
{
    JonesHudMenuItem* pMenuItem;

#line 2723
    pMenuItem = (JonesHudMenuItem*)STDMALLOC(sizeof(JonesHudMenuItem));
    if ( !pMenuItem )
    {
        return NULL;
    }

    pMenuItem->prdIcon = rdThing_New(NULL);
    rdThing_SetModel3(pMenuItem->prdIcon, pItemIcon3);

    pMenuItem->msecMoveDuration = 0;
    pMenuItem->msecMoveEndTime  = 0;
    pMenuItem->msecMoveDelta    = 0;

    pMenuItem->id              = -1;
    pMenuItem->inventoryID     = -1;
    pMenuItem->nextUpItemId    = -1;
    pMenuItem->nextDownItemId  = -1;
    pMenuItem->nextLeftItemId  = -1;
    pMenuItem->nextRightItemId = -1;
    pMenuItem->flags           = 0;

    memset(&pMenuItem->startMovePos, 0, sizeof(pMenuItem->startMovePos));
    memset(&pMenuItem->endMovePos, 0, sizeof(pMenuItem->endMovePos));
    memset(&pMenuItem->pos, 0, sizeof(pMenuItem->pos));
    memset(&pMenuItem->pyr, 0, sizeof(pMenuItem->pyr));
    return pMenuItem;
}

void J3DAPI JonesHud_FreeMenuItem(JonesHudMenuItem* pItem)
{
    rdThing_Free(pItem->prdIcon);
    stdMemory_Free(pItem);
}

void J3DAPI JonesHud_UpdateItem(JonesHudMenuItem* pItem)
{
    if ( !pItem )
    {
        return;
    }

    if ( (pItem->flags & 1) != 0 ) // Update rotation anim
    {
        if ( pItem == JonesHud_pCurSelectedMenuItem
            && !((sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED
                : 0) )
        {
            if ( JonesHud_bItemActivated )
            {
                JonesHud_bItemActivated = 0;
            }
            else
            {
                JonesHud_pCurSelectedMenuItem->pyr.yaw = (float)((JonesHud_msecDeltaTime * 16.0) / 50.0) + JonesHud_pCurSelectedMenuItem->pyr.yaw;
            }
        }
        else if ( (JonesHud_aDfltMenuItemOrients[pItem->id].pyr.yaw) != pItem->pyr.yaw )
        {
            float fyaw;
            float diffYaw;

            fyaw = ((float)fmod(pItem->pyr.yaw, 360.0));
            if ( (diffYaw = fyaw - (JonesHud_aDfltMenuItemOrients[pItem->id].pyr.yaw)) < 0.0f )
            {
                diffYaw = diffYaw + 360.0;
            }

            if ( diffYaw + 1.0f >= 360.0 )
            {
                pItem->pyr.yaw = (JonesHud_aDfltMenuItemOrients[pItem->id].pyr.yaw);
            }
            else
            {
                pItem->pyr.yaw = (float)((360.0 - diffYaw) / 2.0) + pItem->pyr.yaw;
            }
        }
    }

    if ( (pItem->flags & 8) != 0
        || (pItem->flags & 0x10) != 0
        || (pItem->flags & 0x20) != 0
        || (pItem->flags & 0x80) != 0
        || (pItem->flags & 0x40) != 0 )
    {
        if ( JonesHud_HasTimeElapsed(pItem->msecMoveDuration, pItem->msecMoveEndTime, JonesHud_msecTime) )
        {
            int flag100;  // the five flag locals are all in the debug frame
            int rotFlag;
            int colIdx;
            int f200;
            int flag2;

            flag2   = pItem->flags & 2;
            rotFlag = pItem->flags & 1;
            colIdx  = ((pItem->flags >> 10) & 3) << 10;
            flag100 = pItem->flags & 0x100;
            f200    = pItem->flags & 0x200;

            rdVector_Copy3(&pItem->startMovePos, &pItem->endMovePos);
            rdVector_Copy3(&pItem->pos, &pItem->endMovePos);
            pItem->msecMoveDelta = 0;

            switch ( pItem->flags ^ rotFlag ^ (flag2 | flag100 | f200 | colIdx) )
            {
                case 0x08:
                    if ( !flag100 && !f200 )
                    {
                        if ( pItem->id == JonesHud_selectedWeaponMenuItemID
                            || pItem->id == JonesHud_selectedItemsMenuItemID
                            || pItem->id == JonesHud_selectedOptionsMenuItemID
                            || pItem->id == JonesHud_selectedTreasuresMenuItemID
                            || pItem->pos.z == JonesHud_maxItemZ
                            || (JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedWeaponMenuItemID
                                || JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedItemsMenuItemID
                                || JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedOptionsMenuItemID
                                || JonesHud_apMenuItems[pItem->nextUpItemId]->id == JonesHud_selectedTreasuresMenuItemID)
                            && pItem->pos.z <= JonesHud_maxItemZ
                            && pItem->pos.z >= JonesHud_invItemStartPos.z )
                        {
                            pItem->flags |= 1;
                        }

                        pItem->flags &= ~0x08;
                        JonesHud_hudState &= ~0x08;
                    }
                    break;

                case 0x20:
                    pItem->flags &= ~0x20;
                    JonesHud_ExpandMenuColumn(pItem);
                    break;

                case 0x10:
                    pItem->flags &= ~0x11;
                    JonesHud_CollapseMenuColumn(JonesHud_apMenuItems[pItem->nextDownItemId]);
                    break;

                case 0x40:
                {
                    JonesHudMenuItem* pLeftItem;
                    float moveRate;

                    pItem->flags &= ~0x41;
                    if ( pItem->nextLeftItemId != -1 && JonesHud_apMenuItems[pItem->nextLeftItemId] && pItem->id != JonesHud_rootMenuItemId )
                    {
                        pLeftItem = JonesHud_apMenuItems[pItem->nextLeftItemId];
                        if ( pLeftItem == JonesHud_pMenuItemLinkedList )
                        {
                            pLeftItem->endMovePos.x = -0.158f;
                            moveRate = pLeftItem->endMovePos.x - pLeftItem->startMovePos.x;
                        }
                        else
                        {
                            pLeftItem->endMovePos.x = pLeftItem->endMovePos.x - 0.065f;
                            moveRate = 0.065f;
                        }

                        pLeftItem->flags |= 0x40;
                        pLeftItem->flags &= ~0x80;
                        JonesHud_StartItemTranslation(pLeftItem, 50, (float)fabs(moveRate), 0);
                    }
                    else
                    {
                        JonesHud_hudState       = 0;
                        jonesCog_bMenuVisible = 0;
                        JonesHud_pCloseMenuItem = NULL;
                        sithGamesave_CloseRestore();
                        JonesMain_ResumeGame();

                        if ( !sithPlayerControls_bCutsceneMode && JonesHud_bMapOpen )
                        {
                            JonesHud_bMapOpen = 0;
                            sithOverlayMap_ToggleMap();
                        }
                    }
                } break;

                case 0x80:
                {
                    JonesHudMenuItem* pRightItem;

                    pItem->flags &= ~0x80;
                    pItem->flags |= 2;
                    if ( (JonesHud_hudState & 4) == 0
                        && pItem->nextRightItemId != -1
                        && JonesHud_apMenuItems[pItem->nextRightItemId]
                        && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
                    {
                        pRightItem = JonesHud_apMenuItems[pItem->nextRightItemId];
                        rdVector_Copy3(&pRightItem->startMovePos, &pItem->endMovePos);
                        rdVector_Copy3(&pRightItem->pos, &pItem->endMovePos);
                        rdVector_Copy3(&pRightItem->endMovePos, &pItem->endMovePos);
                        pRightItem->endMovePos.x = pRightItem->endMovePos.x + 0.065f;
                        pRightItem->flags |= 0x81;
                        JonesHud_StartItemTranslation(pRightItem, 100, 0.065f, 0);
                    }
                }
            }
        }
        else
        {
            // Update item translation
            float dTransZ;
            float deltaY;
            float dx;

            dx      = pItem->endMovePos.x - pItem->startMovePos.x;
            deltaY  = pItem->endMovePos.y - pItem->startMovePos.y;
            dTransZ = pItem->endMovePos.z - pItem->startMovePos.z;
            pItem->msecMoveDelta = pItem->msecMoveDelta + JonesHud_msecDeltaTime;

            if ( pItem->msecMoveDelta >= pItem->msecMoveDuration )
            {
                rdVector_Copy3(&pItem->pos, &pItem->endMovePos);
            }
            else
            {
                pItem->pos.x = (float)pItem->msecMoveDelta / pItem->msecMoveDuration * dx + pItem->startMovePos.x;
                pItem->pos.y = (float)pItem->msecMoveDelta / pItem->msecMoveDuration * deltaY + pItem->startMovePos.y;
                pItem->pos.z = (float)pItem->msecMoveDelta / pItem->msecMoveDuration * dTransZ + pItem->startMovePos.z;
            }
        }
    }

    if ( pItem->nextRightItemId != -1
        && JonesHud_apMenuItems[pItem->nextRightItemId]
        && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
    {
        JonesHud_UpdateItem(JonesHud_apMenuItems[pItem->nextRightItemId]);
    }

    if ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedWeaponMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedItemsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedOptionsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedTreasuresMenuItemID )
    {
        JonesHud_UpdateItem(JonesHud_apMenuItems[pItem->nextUpItemId]);
    }
}

void J3DAPI JonesHud_RenderMenuItems(JonesHudMenuItem* pItem)
{
    if ( (pItem->flags & 1) != 0 )
    {
        JonesHud_RenderMenuItem(pItem);
    }

    if ( pItem->nextRightItemId != -1
        && JonesHud_apMenuItems[pItem->nextRightItemId]
        && JonesHud_apMenuItems[pItem->nextRightItemId]->id != JonesHud_rootMenuItemId )
    {
        JonesHud_RenderMenuItems(JonesHud_apMenuItems[pItem->nextRightItemId]);
    }

    if ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedWeaponMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedItemsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedOptionsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedTreasuresMenuItemID )
    {
        JonesHud_RenderMenuItems(JonesHud_apMenuItems[pItem->nextUpItemId]);
    }
}

void J3DAPI JonesHud_RenderMenuItem(JonesHudMenuItem* pItem)
{
    rdMatrix34 orient;
    rdVector3 pyrOrient = { 0.0f, 0.0f, 0.0f };
    rdVector3 vecScale;
    rdVector3 vecIconRadiusScale;
    float oalpha;
    rdVector4 modelColor;

    rdMatrix_Copy34(&orient, &rdroid_identMatrix34);
    if ( !pItem )
    {
        return;
    }

    vecIconRadiusScale.x = (vecIconRadiusScale.y = (vecIconRadiusScale.z = 0.01820415f / pItem->prdIcon->data.pModel3->size));
    rdMatrix_PostScale34(&orient, &vecIconRadiusScale);

    vecScale.x = (vecScale.y = (vecScale.z = (JonesHud_aDfltMenuItemOrients[pItem->id].scale)));
    rdMatrix_PostScale34(&orient, &vecScale);

    if ( pItem == JonesHud_pCurSelectedMenuItem )
    {
        if ( pItem->pos.z == pItem->endMovePos.z )
        {
            // Draw selected item text
            char aMenuName[512];
            float itemOffsetY;
            rdFontColor textFontColor;
            rdMatrix34 screenMat;
            int amount;
            const char* pString;
            rdMatrix34 itemOrient;
            int x;
            int y;
            rdVector3 itemPos;

            pString     = NULL;
            amount      = 0;
            itemOffsetY = 20.0f * JonesHud_screenHeightScalar; // Unused

            rdVector_Copy3(&itemPos, &pItem->pos);
            rdMatrix_Copy34(&itemOrient, &rdroid_identMatrix34);
            itemOrient.dvec = itemPos;
            rdMatrix_Multiply34(&screenMat, &itemOrient, &rdCamera_pCurCamera->orient);
            rdCamera_PerspProject(&itemPos, &screenMat.dvec);

            textFontColor[0].red   = 1.0f;
            textFontColor[0].green = 0.0f;
            textFontColor[0].blue  = 0.0f;
            textFontColor[0].alpha = 1.0f;
            textFontColor[1].red   = 1.0f;
            textFontColor[1].green = 0.0f;
            textFontColor[1].blue  = 0.0f;
            textFontColor[1].alpha = 1.0f;
            textFontColor[2].red   = 1.0f;
            textFontColor[2].green = 1.0f;
            textFontColor[2].blue  = 0.0f;
            textFontColor[2].alpha = 1.0f;
            textFontColor[3].red   = 1.0f;
            textFontColor[3].green = 1.0f;
            textFontColor[3].blue  = 0.0f;
            textFontColor[3].alpha = 1.0f;
            rdFont_SetFontColor(textFontColor);

            x = (int)itemPos.x;
            y = (int)itemPos.y - (int)(70.0f * JonesHud_screenHeightScalar);

            if ( (sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED : 0 )
            {
                pString = jonesString_GetString("JONES_STR_INV_NOUSE");
            }
            else
            {
                pString = jonesString_GetString(JonesHud_apItemNames[pItem->id]);
            }

            if ( pString )
            {
                char aItemText[256];

                strcpy(aMenuName, pString);
                amount = (int)((sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].amount : 0.0f);
                memset(aItemText, 0, sizeof(aItemText));
                if ( pItem->inventoryID != -1 && pItem->id <= JONESHUD_MENU_INVITEM_BONUSMAP )
                {
                    switch ( pItem->id )
                    {
                        case JONESHUD_MENU_TREASURE_CHEST:
                            sprintf(aItemText, "%s  %i ($%i)\n", aMenuName, JonesHud_numFoundTreasures, amount);
                            break;

                        case JONESHUD_MENU_WEAP_GLOVES:
                        case JONESHUD_MENU_WEAP_MACHETE:
                            sprintf(aItemText, "%s", aMenuName);
                            break;

                        case JONESHUD_MENU_INVITEM_PATCHKIT:
                            sprintf(aItemText, "%s  %i\n", aMenuName, amount);
                            break;

                        case JONESHUD_MENU_WEAP_WHIP:
                            if ( (sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_ELECTRICWHIP) != 0 )
                            {
                                sprintf(aItemText, "%s  %i\n", aMenuName, amount);
                            }
                            else
                            {
                                sprintf(aItemText, "%s", aMenuName);
                            }
                            break;

                        default:
                            if ( (unsigned int)(int)((sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].amount : 0.0f) > 1 )
                            {
                                sprintf(aItemText, "%s  %i\n", aMenuName, amount);
                            }
                            else
                            {
                                sprintf(aItemText, "%s", aMenuName);
                            }
                            break;
                    }
                }
                else
                {
                    sprintf(aItemText, "%s", aMenuName);
                }

                rdFont_DrawTextLineClipped(aItemText, (float)(x / (JonesHud_screenWidthScalar * 640.0)), (float)(y / (JonesHud_screenHeightScalar * 480.0)), RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pMenuFont, RDFONT_ALIGNCENTER);
            }
        }

        // Highlight selected item by scaling it up a little bit
        if ( !((sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED : 0) )
        {
            rdMatrix_PostScale34(&orient, &JonesHud_vecSelectedMenuItemScale);
        }
    }

    // Rotate item matrix by RPY
    memset(&pyrOrient, 0, sizeof(pyrOrient));
    pyrOrient.roll = pItem->pyr.roll;
    rdMatrix_PostRotate34(&orient, &pyrOrient);

    memset(&pyrOrient, 0, sizeof(pyrOrient));
    pyrOrient.pitch = pItem->pyr.pitch;
    rdMatrix_PostRotate34(&orient, &pyrOrient);

    memset(&pyrOrient, 0, sizeof(pyrOrient));
    pyrOrient.yaw = pItem->pyr.yaw;
    rdMatrix_PostRotate34(&orient, &pyrOrient);

    orient.dvec.x = pItem->pos.x;
    orient.dvec.y = pItem->pos.y;
    orient.dvec.z = pItem->pos.z;

    rdModel3_GetThingColor(pItem->prdIcon, &modelColor);
    oalpha           = modelColor.alpha;
    modelColor.alpha = pItem->alpha;

    if ( (pItem->flags & 8) != 0
        || !((pItem->flags & 8) != 0 || (pItem->flags & 0x10) != 0 || (pItem->flags & 0x20) != 0 || (pItem->flags & 0x80) != 0 || (pItem->flags & 0x40) != 0) )
    {
        if ( (sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED : 0 )
        {
            modelColor.alpha = 0.4f;
        }
        else
        {
            modelColor.alpha = 0.95f;
        }

        rdModel3_SetThingColor(pItem->prdIcon, &modelColor);
    }
    else
    {
        float alphaFraction;
        int curFlag100;
        float range;
        int itemFlag1;
        int colIdx;
        float itemMaxAlpha;
        int itemFlag200;
        float animCur;
        int flag2;

        flag2       = pItem->flags & 2;
        itemFlag1   = pItem->flags & 1;
        colIdx      = ((pItem->flags >> 10) & 3) << 10;
        curFlag100  = pItem->flags & 0x100;
        itemFlag200 = pItem->flags & 0x200;
        itemMaxAlpha = 0.6f;

        switch ( pItem->flags ^ itemFlag1 ^ (flag2 | curFlag100 | itemFlag200 | colIdx) )
        {
            case 0x20:
                range   = pItem->startMovePos.z - pItem->endMovePos.z;
                animCur = pItem->startMovePos.z - pItem->pos.z;
                break;

            case 0x80:
                range   = pItem->startMovePos.x - pItem->endMovePos.x;
                animCur = pItem->startMovePos.x - pItem->pos.x;
                break;

            case 0x40:
                range   = pItem->startMovePos.x - pItem->endMovePos.x;
                animCur = pItem->pos.x - pItem->endMovePos.x;
                break;

            case 0x10:
                range   = pItem->startMovePos.z - pItem->endMovePos.z;
                animCur = pItem->pos.z - pItem->endMovePos.z;
                break;
        }

        if ( (alphaFraction = animCur / range) <= 0.6666667f )
        {
            modelColor.alpha = itemMaxAlpha * alphaFraction;
        }
        else
        {
            modelColor.alpha = (modelColor.alpha = (alphaFraction - 0.6666667f) * itemMaxAlpha / 0.33333334f) + 0.4f;
        }

        if ( (sithInventory_aTypes[pItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[pItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED : 0 )
        {
            modelColor.alpha = modelColor.alpha < 0.4f ? modelColor.alpha : 0.4f;
        }
        else
        {
            modelColor.alpha = modelColor.alpha < 0.95f ? modelColor.alpha : 0.95f;
        }

        if ( (pItem->flags & 0x10) != 0 || (pItem->flags & 0x40) != 0 )
        {
            modelColor.alpha = modelColor.alpha > 0.0f ? modelColor.alpha : 0.0f;
        }

        rdModel3_SetThingColor(pItem->prdIcon, &modelColor);
    }

    rdModel3_Draw(pItem->prdIcon, &orient);
    pItem->alpha     = modelColor.alpha;
    modelColor.alpha = oalpha;
    rdModel3_SetThingColor(pItem->prdIcon, &modelColor);
}

void J3DAPI JonesHud_MenuActivateItem(void)
{
    if ( !JonesHud_pCurSelectedMenuItem || !sithPlayer_pLocalPlayerThing )
    {
        return;
    }

    if ( (sithInventory_aTypes[JonesHud_pCurSelectedMenuItem->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
        ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_pCurSelectedMenuItem->inventoryID].status & SITHINVENTORY_ITEM_DISABLED
        : 0 )
    {
        sithGamesave_CloseRestore();
        JonesHud_MenuClose();
        return;
    }

    // Note, the system menus don't have inventory ID (e.g. settings, load/save, exit etc..)
    if ( JonesHud_pCurSelectedMenuItem->inventoryID != -1 && JonesHud_pCurSelectedMenuItem->id != JONESHUD_MENU_TREASURE_CHEST )
    {
        if ( JonesHud_pCurSelectedMenuItem->id <= JONESHUD_MENU_WEAP_BAZOOKA
            || JonesHud_pCurSelectedMenuItem->id == JONESHUD_MENU_INVITEM_WEAP_MIRROR
            || JonesHud_pCurSelectedMenuItem->id == JONESHUD_MENU_INVITEM_ZIPPO
            || JonesHud_pCurSelectedMenuItem->id >= JONESHUD_MENU_INVITEM_IMP1 && JonesHud_pCurSelectedMenuItem->id <= JONESHUD_MENU_INVITEM_IMP5 )
        {
            if ( sithWeapon_IsMountingWeapon(sithPlayer_pLocalPlayerThing) )
            {
                JonesHud_selectedWeaponID = JonesHud_pCurSelectedMenuItem->inventoryID;
            }
            else
            {
                sithWeapon_SelectWeapon(sithPlayer_pLocalPlayerThing, (SithWeaponId)JonesHud_pCurSelectedMenuItem->inventoryID);
                JonesHud_selectedWeaponID = -1;
            }
        }
        else
        {
            sithInventory_SetCurrentItem(sithPlayer_pLocalPlayerThing, JonesHud_pCurSelectedMenuItem->inventoryID);
            if ( JonesHud_pCurSelectedMenuItem->id >= JONESHUD_MENU_HEALTH_SMALL
                && JonesHud_pCurSelectedMenuItem->id <= JONESHUD_MENU_HEALTH_POISONKIT
                && sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.health < sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.maxHealth )
            {
                // Sends cog msg user0 to health items and poison kit
                jonesInventory_UseItem(sithPlayer_pLocalPlayerThing, JonesHud_pCurSelectedMenuItem->inventoryID);
            }
            else if ( !jonesInventory_ActivateItem(sithPlayer_pLocalPlayerThing, JonesHud_pCurSelectedMenuItem->inventoryID)
                && sithPlayer_pLocalPlayerThing->pInSector
                && (sithPlayer_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
            {
                // Item couldn't be activated, play whoops indy voice
                int idx;
                int fxIdx;

                do
                {
                    idx = rand() % 4;
                    fxIdx  = idx + 7; // from idx 7 - 10
                } while ( !JonesHud_aSoundFxHandles[fxIdx] || idx == JonesHud_curWhoopsSndFxIdx );

                JonesHud_curWhoopsSndFxIdx = idx;
                sithVoice_PlayThingVoice(sithPlayer_pLocalPlayerThing, JonesHud_aSoundFxHandles[fxIdx], 1.0f);
            }
            else if ( JonesHud_pCurSelectedMenuItem->id == JONESHUD_MENU_INVITEM_PATCHKIT && sithWorld_pCurrentWorld )
            {
                tSoundHandle hSndInfo;

                hSndInfo = sithSound_Load(sithWorld_pCurrentWorld, "riv_raft_repair.wav"); // inflate sound fx
                if ( hSndInfo )
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(hSndInfo, 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }
            }
        }

        sithGamesave_CloseRestore();
        JonesHud_MenuClose();
        return;
    }

    {
        // Handle activation of system menu item
        int dlgResult;

        JonesHud_bItemActivated = 1;
        switch ( JonesHud_pCurSelectedMenuItem->id )
        {
            case JONESHUD_MENU_EXIT:
            {
                const char* pNoSaveFormat;
                char aSaveGameFilename[JONESCONFIG_GAMESAVE_FILEPATHSIZE];

                memset(aSaveGameFilename, 0, sizeof(aSaveGameFilename));
                dlgResult = jonesConfig_ShowExitGameDialog(stdWin95_GetWindow(), aSaveGameFilename);
                if ( dlgResult == 2 )
                {
                    JonesHud_bExitActivated = 0;
                }
                else
                {
                    JonesHud_bExitActivated = 1;
                    if ( dlgResult == 1187 )
                    {
                        sithGamesave_Save(aSaveGameFilename, 1);
                        if ( sithGamesave_Process() )
                        {
                            pNoSaveFormat = jonesString_GetString("JONES_STR_NOSAVE");
                            if ( pNoSaveFormat )
                            {
                                char aMsg[256];

                                memset(aMsg, 0, sizeof(aMsg));
                                sprintf(aMsg, pNoSaveFormat, aSaveGameFilename);
                                jonesConfig_ShowMessageDialog(stdWin95_GetWindow(), "JONES_STR_EXIT", aMsg, 141);
                            }

                            dlgResult = 2;
                            JonesHud_bExitActivated = 0;
                        }
                        else
                        {
                            dlgResult = 1;
                        }
                    }
                }
            } break;

            case JONESHUD_MENU_LOAD_GAME:
                if ( jonesCog_bEnableGamesave )
                {
                    memset(JonesHud_aSlectedNdsFilePath, 0, sizeof(JonesHud_aSlectedNdsFilePath));
                    dlgResult = jonesConfig_GetLoadGameFilePath(stdWin95_GetWindow(), JonesHud_aSlectedNdsFilePath);
                    if ( dlgResult == 1 && strlen(JonesHud_aSlectedNdsFilePath) > 0 )
                    {
                        if ( JonesMain_HasStarted() )
                        {
                            JonesHud_bRestoreActivated = 1;
                        }

                        JonesHud_bRestoreGameStatistics = 1;
                    }
                }
                break;

            case JONESHUD_MENU_SAVE_GAME:
                if ( jonesCog_bEnableGamesave )
                {
                    char aFilePath[JONESCONFIG_GAMESAVE_FILEPATHSIZE];

                    JonesHud_RestoreTreasuresStatistics();
                    memset(aFilePath, 0, sizeof(aFilePath));
                    dlgResult = jonesConfig_GetSaveGameFilePath(stdWin95_GetWindow(), aFilePath);
                    if ( dlgResult == 1 )
                    {
                        jonesInventory_UpdateGameTimeStatistics(sithTime_msecGameTime);
                        sithGamesave_Save(aFilePath, 1);
                    }
                }
                break;

            case JONESHUD_MENU_GAME_SETTINGS:
                dlgResult = jonesConfig_ShowGamePlayOptions(stdWin95_GetWindow());
                break;

            case JONESHUD_MENU_CONTROLS_SETTINGS:
                dlgResult = jonesConfig_ShowControlOptions(stdWin95_GetWindow());
                break;

            case JONESHUD_MENU_DISPLAY_SETTINGS:
            {
                JonesDisplaySettings* pDSettings;
                StdDisplayEnvironment* pDisplayEnv;

                pDSettings  = JonesMain_GetDisplaySettings();
                pDisplayEnv = JonesMain_GetDisplayEnvironment();
                dlgResult   = jonesConfig_ShowDisplaySettingsDialog(stdWin95_GetWindow(), pDisplayEnv, pDSettings);
                if ( dlgResult == 1 )
                {
                    JonesMain_RefreshDisplayDevice();
                }
            } break;

            case JONESHUD_MENU_SOUND_SETTINGS:
            {
                JonesSoundSettings sndSettings;

                sndSettings.maxSoundVolume = Sound_GetMaxVolume();
                sndSettings.b3DHWSupport   = Sound_Get3DHWState();
                sndSettings.bReverseSound  = wuRegistry_GetIntEx("ReverseSound", 0);

                dlgResult = jonesConfig_ShowSoundSettingsDialog(stdWin95_GetWindow(), &sndSettings);
                if ( dlgResult == 1 )
                {
                    Sound_SetMaxVolume(sndSettings.maxSoundVolume);
                    Sound_Set3DHWState(sndSettings.b3DHWSupport);
                    Sound_SetReverseSound(sndSettings.bReverseSound);
                }
            } break;

            case JONESHUD_MENU_HELP:
                if ( JonesHud_hProcessHelp )
                {
                    DWORD exitCode;

                    GetExitCodeProcess(JonesHud_hProcessHelp, &exitCode);
                    if ( exitCode != STILL_ACTIVE )
                    {
                        JonesHud_hProcessHelp = NULL;
                    }
                }

                JonesHud_hProcessHelp = JonesHud_OpenHelp(JonesHud_hProcessHelp);
                dlgResult = 2;
                break;

            case JONESHUD_MENU_UNKNOWN_124:
            case JONESHUD_MENU_IQ: // Show game statistics
            {
                SithGameStatistics* pStatistics;

                jonesInventory_UpdateSolvedHintsStatistics();
                jonesInventory_UpdateGameTimeStatistics(sithTime_msecGameTime);
                pStatistics = sithGamesave_GetGameStatistics();
                if ( pStatistics && sithGamesave_LockGameStatistics() )
                {
                    pStatistics->aLevelStatistic[pStatistics->curLevelNum].iqPoints = jonesInventory_GetTotalIQPoints() - JonesHud_levelStartIQPoints;
                    jonesConfig_ShowStatisticsDialog(stdWin95_GetWindow(), pStatistics);
                    sithGamesave_UnlockGameStatistics();
                }

                dlgResult = 2;
            }
        }

        if ( dlgResult == 1 )
        {
            if ( JonesHud_pCurSelectedMenuItem->id != JONESHUD_MENU_SAVE_GAME )
            {
                sithGamesave_CloseRestore();
            }

            JonesHud_MenuClose();
        }
    }
}

void J3DAPI JonesHud_ResetMenuItems(void)
{
    int idx;
    JonesHudMenuItem* pPrevItem;
    int menu;
    int curId;
    int lastItemId;
    int selId;
    int firstItemId;

    if ( !sithPlayer_pLocalPlayerThing )
    {
        return;
    }

    JonesHud_hudState = 0;
    for ( idx = 0; idx < JONESHUD_MAX_MENU_ITEMS; ++idx )
    {
        if ( JonesHud_apMenuItems[idx] )
        {
            JonesHud_apMenuItems[idx]->msecMoveDuration = 0;
            JonesHud_apMenuItems[idx]->msecMoveEndTime  = 0;
            JonesHud_apMenuItems[idx]->msecMoveDelta    = 0;
            JonesHud_apMenuItems[idx]->flags            = 0;
            JonesHud_apMenuItems[idx]->alpha            = 1.0f;
            JonesHud_apMenuItems[idx]->nextDownItemId   = -1;
            JonesHud_apMenuItems[idx]->nextRightItemId  = -1;
            JonesHud_apMenuItems[idx]->nextUpItemId     = -1;
            JonesHud_apMenuItems[idx]->nextLeftItemId   = -1;

            JonesHud_apMenuItems[idx]->pyr.x = (JonesHud_aDfltMenuItemOrients[JonesHud_apMenuItems[idx]->id].pyr.x);
            JonesHud_apMenuItems[idx]->pyr.y = (JonesHud_aDfltMenuItemOrients[JonesHud_apMenuItems[idx]->id].pyr.y);
            JonesHud_apMenuItems[idx]->pyr.z = (JonesHud_aDfltMenuItemOrients[JonesHud_apMenuItems[idx]->id].pyr.z);

            memset(&JonesHud_apMenuItems[idx]->startMovePos, 0, sizeof(JonesHud_apMenuItems[idx]->startMovePos));
            memset(&JonesHud_apMenuItems[idx]->endMovePos, 0, sizeof(JonesHud_apMenuItems[idx]->endMovePos));
            memset(&JonesHud_apMenuItems[idx]->pos, 0, sizeof(JonesHud_apMenuItems[idx]->pos));
        }
    }

    pPrevItem = NULL;
    if ( JonesHud_selectedOptionsMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID] )
    {
        pPrevItem = JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID];
    }

    if ( JonesHud_selectedItemsMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID] )
    {
        if ( pPrevItem )
        {
            JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID], pPrevItem);
        }

        pPrevItem = JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID];
    }

    if ( JonesHud_selectedTreasuresMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID] )
    {
        if ( pPrevItem )
        {
            JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID], pPrevItem);
        }

        pPrevItem = JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID];
    }

    if ( JonesHud_selectedWeaponMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID] )
    {
        if ( pPrevItem )
        {
            JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID], pPrevItem);
        }

        pPrevItem = JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID];
    }

    JonesHud_SetLeftRightItems(JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID], pPrevItem);
    JonesHud_pMenuItemLinkedList = pPrevItem;

    // Setup the 3 inventory menus
    for ( menu = 0; menu < 3; ++menu )
    {
        switch ( menu )
        {
            case 0: // Weapons menu
                if ( JonesHud_selectedWeaponMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedWeaponMenuItemID] )
                {
                    selId       = JonesHud_selectedWeaponMenuItemID;
                    firstItemId = JONESHUD_MENU_WEAP_GLOVES;
                    lastItemId  = JONESHUD_MENU_WEAP_BAZOOKA;
                    break;
                }
                continue;

            case 1: // Treasury menu
                if ( JonesHud_selectedTreasuresMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedTreasuresMenuItemID] )
                {
                    selId       = JonesHud_selectedTreasuresMenuItemID;
                    firstItemId = JONESHUD_MENU_HEALTH_SMALL;
                    lastItemId  = JONESHUD_MENU_TREASURE_CHEST;
                    break;
                }
                continue;

            case 2: // Inventory items menu
                if ( JonesHud_selectedItemsMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedItemsMenuItemID] )
                {
                    selId       = JonesHud_selectedItemsMenuItemID;
                    firstItemId = JONESHUD_MENU_INVITEM_ZIPPO;
                    lastItemId  = JONESHUD_MENU_INVITEM_BONUSMAP;
                    break;
                }
                continue;
        }

        curId     = selId + 1;
        pPrevItem = JonesHud_apMenuItems[selId];
        for ( idx = 0; idx < lastItemId - firstItemId; ++idx )
        {
            if ( curId > lastItemId )
            {
                curId = firstItemId;
            }

            if ( curId != -1
                && JonesHud_apMenuItems[curId]
                && JonesHud_apMenuItems[curId]->inventoryID != -1
                && ((unsigned int)((sithInventory_aTypes[JonesHud_apMenuItems[curId]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                        ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[curId]->inventoryID].amount
                        : 0.0f) > 0
                    || curId == JONESHUD_MENU_TREASURE_CHEST
                    || JonesHud_apMenuItems[curId]->id <= JONESHUD_MENU_WEAP_BAZOOKA
                    && ((sithInventory_aTypes[JonesHud_apMenuItems[curId]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                        ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[curId]->inventoryID].status & SITHINVENTORY_ITEM_FOUND
                        : 0)) )
            {
                JonesHud_SetDownUpItems(JonesHud_apMenuItems[curId], pPrevItem);
                pPrevItem = JonesHud_apMenuItems[curId];
            }

            ++curId;
        }

        if ( pPrevItem != JonesHud_apMenuItems[selId] )
        {
            JonesHud_SetDownUpItems(JonesHud_apMenuItems[selId], pPrevItem);
        }
    }

    // Setup the system menu items
    if ( JonesHud_selectedOptionsMenuItemID != -1 && JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID] )
    {
        curId     = JonesHud_selectedOptionsMenuItemID + 1;
        pPrevItem = JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID];
        for ( idx = 0; idx < 8; ++idx )
        {
            if ( curId > JONESHUD_MENU_EXIT )
            {
                curId = JONESHUD_MENU_IQ;
            }

            if ( pPrevItem != JonesHud_apMenuItems[curId] )
            {
                JonesHud_SetDownUpItems(JonesHud_apMenuItems[curId], pPrevItem);
                pPrevItem = JonesHud_apMenuItems[curId];
            }

            ++curId;
        }

        if ( pPrevItem != JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID] )
        {
            JonesHud_SetDownUpItems(JonesHud_apMenuItems[JonesHud_selectedOptionsMenuItemID], pPrevItem);
        }
    }

    rdVector_Copy3(&JonesHud_pMenuItemLinkedList->startMovePos, &JonesHud_invItemStartPos);
    JonesHud_pMenuItemLinkedList->startMovePos.x = -0.158f;
    rdVector_Copy3(&JonesHud_pMenuItemLinkedList->pos, &JonesHud_pMenuItemLinkedList->startMovePos);
    rdVector_Copy3(&JonesHud_pMenuItemLinkedList->endMovePos, &JonesHud_invItemStartPos);
    JonesHud_pMenuItemLinkedList->endMovePos.x   = JonesHud_pMenuItemLinkedList->endMovePos.x + 0.065f;
    JonesHud_maxItemZ = JonesHud_invItemStartPos.z + 0.195f;
}

void J3DAPI JonesHud_SetLeftRightItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2)
{
    pItem1->nextRightItemId = pItem2->id;
    pItem2->nextLeftItemId = pItem1->id;
}

void J3DAPI JonesHud_SetDownUpItems(JonesHudMenuItem* pItem1, JonesHudMenuItem* pItem2)
{
    pItem1->nextDownItemId = pItem2->id;
    pItem2->nextUpItemId = pItem1->id;
}

void J3DAPI JonesHud_ExpandMenuColumn(JonesHudMenuItem* pItem)
{
    // Shows up to 4 items in the column of the currently selected item
    JonesHudMenuItem* pUpItem;
    JonesHudMenuItem* pFirstItem;

    if ( pItem->nextUpItemId == -1 || !JonesHud_apMenuItems[pItem->nextUpItemId] || JonesHud_apMenuItems[pItem->nextUpItemId] == pItem )
    {
        return;
    }

    JonesHud_hudState |= 2;
    pFirstItem = pItem;
    while ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && pItem->pos.z < JonesHud_maxItemZ
        && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0
        && pItem->pos.z < JonesHud_apMenuItems[pItem->nextUpItemId]->pos.z )
    {
        pItem = JonesHud_apMenuItems[pItem->nextUpItemId];
    }

    if ( (pItem->flags & 8) != 0
        || (pItem->flags & 0x10) != 0
        || (pItem->flags & 0x20) != 0
        || (pItem->flags & 0x80) != 0
        || (pItem->flags & 0x40) != 0 )
    {
        rdVector_Copy3(&pItem->startMovePos, &pItem->pos);
        pItem->endMovePos.z = pItem->endMovePos.z + 0.065f;
        pItem->flags &= ~0x10;
        pItem->flags |= 0x20;
        JonesHud_StartItemTranslation(pItem, 100, 0.065f, 1);
    }
    else if ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && pItem->pos.z < JonesHud_maxItemZ
        && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) == 0 )
    {
        pUpItem = JonesHud_apMenuItems[pItem->nextUpItemId];
        rdVector_Copy3(&pUpItem->startMovePos, &pItem->pos);
        rdVector_Copy3(&pUpItem->endMovePos, &pItem->pos);
        rdVector_Copy3(&pUpItem->pos, &pItem->pos);
        pUpItem->endMovePos.z = pUpItem->endMovePos.z + 0.065f;
        pUpItem->flags |= 0x20;
        JonesHud_StartItemTranslation(pUpItem, 100, 0.065f, 1);
    }
    else
    {
        JonesHud_hudState &= ~0x02;
    }
}

void J3DAPI JonesHud_CollapseMenuColumn(JonesHudMenuItem* pItem)
{
    JonesHudMenuItem* pDown;
    float moveRate;

    while ( pItem->nextUpItemId != -1
        && JonesHud_apMenuItems[pItem->nextUpItemId]
        && pItem->pos.z < JonesHud_maxItemZ
        && (JonesHud_apMenuItems[pItem->nextUpItemId]->flags & 0x01) != 0 // update rotation anim
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedWeaponMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedItemsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedOptionsMenuItemID
        && JonesHud_apMenuItems[pItem->nextUpItemId]->id != JonesHud_selectedTreasuresMenuItemID
        && pItem->pos.z <= JonesHud_apMenuItems[pItem->nextUpItemId]->pos.z )
    {
        pItem = JonesHud_apMenuItems[pItem->nextUpItemId];
    }

    if ( (pItem->flags & 0x02) == 0 && (pItem->flags & 0x10) == 0 )
    {
        pDown = JonesHud_apMenuItems[pItem->nextDownItemId];
        rdVector_Copy3(&pItem->startMovePos, &pItem->pos);
        rdVector_Copy3(&pItem->endMovePos, &pDown->pos);
        pItem->flags &= ~0x20;
        pItem->flags |= 0x10;
        JonesHud_StartItemTranslation(pItem, 100, 0.065f, 1);
    }
    else if ( JonesHud_pCloseMenuItem )
    {
        JonesHud_item_flag_state_55514C &= ~(1 << ((pItem->flags >> 10) & 0x03));
        if ( JonesHud_item_flag_state_55514C == 0 )
        {
            if ( JonesHud_aSoundFxHandles[3] ) // inv_expand.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[3], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }

            JonesHud_pCloseMenuItem->flags |= 0x40;
            JonesHud_pCloseMenuItem->flags &= ~0x80;
            if ( JonesHud_pCloseMenuItem == JonesHud_pMenuItemLinkedList )
            {
                JonesHud_pCloseMenuItem->endMovePos.x = -0.158f;
                moveRate = JonesHud_pCloseMenuItem->endMovePos.x - JonesHud_pCloseMenuItem->startMovePos.x;
            }
            else
            {
                JonesHud_pCloseMenuItem->endMovePos.x = JonesHud_pCloseMenuItem->endMovePos.x - 0.065f;
                moveRate = 0.065f;
            }

            JonesHud_StartItemTranslation(JonesHud_pCloseMenuItem, 50, (float)fabs(moveRate), 0);
        }
    }
}

void J3DAPI JonesHud_InventoryItemChanged(int typeId)
{
    int itemID;
    JonesHudMenuItem* pItem;

    pItem = NULL;
    if ( !sithPlayer_pLocalPlayerThing || !sithPlayer_pLocalPlayerThing->pInSector )
    {
        return;
    }

    if ( JonesHud_pCurInvChangedItem )
    {
        JonesHud_FreeMenuItem(JonesHud_pCurInvChangedItem);
        JonesHud_pCurInvChangedItem = NULL;
    }

    for ( itemID = 0; itemID < JONESHUD_MAX_MENU_ITEMS; ++itemID )
    {
        if ( itemID != -1 && JonesHud_apMenuItems[itemID] && JonesHud_apMenuItems[itemID]->inventoryID == typeId )
        {
            if ( (sithInventory_aTypes[JonesHud_apMenuItems[itemID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[itemID]->inventoryID].status & SITHINVENTORY_ITEM_FOUND
                : 0 )
            {
                switch ( itemID )
                {
                    case JONESHUD_MENU_WEAP_MAUSER:
                        itemID = JONESHUD_MENU_AMMO_MAUSER;
                        break;

                    case JONESHUD_MENU_WEAP_SUBMACHINE:
                        itemID = JONESHUD_MENU_AMMO_SUBMACHINE;
                        break;

                    case JONESHUD_MENU_WEAP_SIMONOV:
                        itemID = JONESHUD_MENU_AMMO_SIMONOV;
                        break;

                    case JONESHUD_MENU_WEAP_TOKAREV:
                        itemID = JONESHUD_MENU_AMMO_TOKAREV;
                        break;

                    case JONESHUD_MENU_WEAP_SHOTGUN:
                        itemID = JONESHUD_MENU_AMMO_SHOTGUN;
                        break;

                    case JONESHUD_MENU_WEAP_BAZOOKA:
                        itemID = JONESHUD_MENU_AMMO_BAZOOKA;
                        break;

                    default:
                        break;
                }
            }

            pItem = JonesHud_apMenuItems[itemID];
            break;
        }
    }

    if ( pItem )
    {
        JonesHud_pCurInvChangedItem = JonesHud_NewMenuItem(JonesHud_apMenuItems[itemID]->prdIcon->data.pModel3);
        JonesHud_pCurInvChangedItem->id          = pItem->id;
        JonesHud_pCurInvChangedItem->inventoryID = pItem->inventoryID;

        rdVector_Copy3(&JonesHud_pCurInvChangedItem->endMovePos, &JonesHud_invItemStartPos);
        JonesHud_pCurInvChangedItem->endMovePos.x = -0.158f;
        rdVector_Copy3(&JonesHud_pCurInvChangedItem->pos, &JonesHud_invItemStartPos);
        rdVector_Copy3(&JonesHud_pCurInvChangedItem->startMovePos, &JonesHud_invItemStartPos);

        JonesHud_pCurInvChangedItem->pyr.x = (JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.x);
        JonesHud_pCurInvChangedItem->pyr.y = (JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.y);
        JonesHud_pCurInvChangedItem->pyr.z = (JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.z);

        if ( JonesHud_pCurInvChangedItem->id >= JONESHUD_MENU_TREASURE_COINS_GOLD && JonesHud_pCurInvChangedItem->id <= JONESHUD_MENU_TREASURE_CASHBOX )
        {
            // Found Treasure
            jonesInventory_AdvanceFoundTreasuresStatistics();
            ++JonesHud_numFoundTreasures;
        }

        if ( sithPlayer_pLocalPlayerThing->pInSector && (sithPlayer_pLocalPlayerThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0
            || JonesHud_bIMPState )
        {
            JonesHud_pCurInvChangedItem->pos.x          = 0.1026f;
            JonesHud_pCurInvChangedItem->startMovePos.x = 0.1026f;
        }
        else
        {
            JonesHud_pCurInvChangedItem->pos.x          = 0.1526f;
            JonesHud_pCurInvChangedItem->startMovePos.x = 0.1526f;
        }

        if ( JonesHud_pCurInvChangedItem->id >= JONESHUD_MENU_TREASURE_COINS_GOLD && JonesHud_pCurInvChangedItem->id <= JONESHUD_MENU_TREASURE_CASHBOX )
        {
            if ( JonesHud_aSoundFxHandles[12] ) // inv_treasure.wav
            {
                JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[12], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
            }
        }
        else if ( JonesHud_aSoundFxHandles[5] ) // mus_find.wav
        {
            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[5], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
        }

        JonesHud_StartItemTranslation(JonesHud_pCurInvChangedItem, 2000, (float)fabs(JonesHud_pCurInvChangedItem->pos.x - -0.158f), 0);
    }
    else
    {
        JonesHud_pCurInvChangedItem = NULL;
    }
}

void J3DAPI JonesHud_RenderInventoryItemChange(void)
{
    float scale;
    float normYaw;
    float dyaw;
    float dTransZ;
    float deltaX;
    float distY;
    float distTotal;
    float remX;

    scale = 1.0f;
    if ( JonesHud_HasTimeElapsed(JonesHud_pCurInvChangedItem->msecMoveDuration, JonesHud_pCurInvChangedItem->msecMoveEndTime, JonesHud_msecTime) )
    {
        if ( (JonesHud_pCurInvChangedItem->flags & 0x40) == 0 )
        {
            // Rotate item to final orientation
            if ( (JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.yaw) != JonesHud_pCurInvChangedItem->pyr.yaw )
            {
                normYaw = ((float)fmod(JonesHud_pCurInvChangedItem->pyr.yaw, 360.0));
                if ( (dyaw = normYaw - (JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.yaw)) < 0.0f )
                {
                    dyaw = dyaw + 360.0;
                }

                if ( dyaw + 20.0f >= 360.0 )
                {
                    JonesHud_pCurInvChangedItem->pyr.yaw       = (JonesHud_aDfltMenuItemOrients[JonesHud_pCurInvChangedItem->id].pyr.yaw);
                    JonesHud_pCurInvChangedItem->msecMoveDelta = 0;
                    JonesHud_pCurInvChangedItem->flags |= 0x40;
                }
                else if ( dyaw > 0.0f )
                {
                    JonesHud_pCurInvChangedItem->pyr.yaw = (float)((360.0 - dyaw) / 2.0) + JonesHud_pCurInvChangedItem->pyr.yaw;
                }
            }
            else
            {
                JonesHud_pCurInvChangedItem->flags |= 0x40;
                JonesHud_pCurInvChangedItem->msecMoveDelta = 0;
            }
        }
        else
        {
            // Item finished rotating, now translate item
            if ( JonesHud_pCurInvChangedItem->endMovePos.x == JonesHud_pCurInvChangedItem->pos.x )
            {
                // Item translated to the end position
                if ( !JonesHud_bMenuEnabled )
                {
                    JonesHud_healthIndAlpha = 1.0f;
                    JonesHud_bFadeHealthHUD = 1;
                }

                JonesHud_FreeMenuItem(JonesHud_pCurInvChangedItem);
                JonesHud_pCurInvChangedItem = NULL;
                return;
            }

            JonesHud_pCurInvChangedItem->msecMoveDelta = JonesHud_pCurInvChangedItem->msecMoveDelta + JonesHud_msecDeltaTime;
            JonesHud_pCurInvChangedItem->msecMoveDelta = JonesHud_pCurInvChangedItem->msecMoveDelta * 3;

            if ( JonesHud_pCurInvChangedItem->startMovePos.x == JonesHud_pCurInvChangedItem->pos.x )
            {
                // Begin translation
                if ( JonesHud_aSoundFxHandles[6] ) // inv_slide_to.wav
                {
                    JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(JonesHud_aSoundFxHandles[6], 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                }

                deltaX  = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->startMovePos.x;
                distY   = JonesHud_pCurInvChangedItem->endMovePos.y - JonesHud_pCurInvChangedItem->startMovePos.y;
                dTransZ = JonesHud_pCurInvChangedItem->endMovePos.z - JonesHud_pCurInvChangedItem->startMovePos.z;
                JonesHud_pCurInvChangedItem->pos.x = ((JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f) * deltaX) + JonesHud_pCurInvChangedItem->startMovePos.x;
                JonesHud_pCurInvChangedItem->pos.y = ((JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f) * distY) + JonesHud_pCurInvChangedItem->startMovePos.y;
                JonesHud_pCurInvChangedItem->pos.z = ((JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f) * dTransZ) + JonesHud_pCurInvChangedItem->startMovePos.z;
            }
            else
            {
                deltaX  = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->startMovePos.x;
                distY   = JonesHud_pCurInvChangedItem->endMovePos.y - JonesHud_pCurInvChangedItem->startMovePos.y;
                dTransZ = JonesHud_pCurInvChangedItem->endMovePos.z - JonesHud_pCurInvChangedItem->startMovePos.z;
                JonesHud_pCurInvChangedItem->pos.x = ((JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f) * deltaX) + JonesHud_pCurInvChangedItem->startMovePos.x;
                JonesHud_pCurInvChangedItem->pos.y = ((JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f) * distY) + JonesHud_pCurInvChangedItem->startMovePos.y;
                JonesHud_pCurInvChangedItem->pos.z = ((JonesHud_pCurInvChangedItem->msecMoveDelta / 4000.0f) * dTransZ) + JonesHud_pCurInvChangedItem->startMovePos.z;
                if ( JonesHud_pCurInvChangedItem->pos.x < JonesHud_pCurInvChangedItem->endMovePos.x )
                {
                    JonesHud_pCurInvChangedItem->pos.x = JonesHud_pCurInvChangedItem->endMovePos.x;
                }
            }

            if ( JonesHud_pCurInvChangedItem->pos.x == JonesHud_pCurInvChangedItem->endMovePos.x )
            {
                scale = 0.0f;
            }
            else
            {
                remX      = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->pos.x;
                distTotal = JonesHud_pCurInvChangedItem->endMovePos.x - JonesHud_pCurInvChangedItem->startMovePos.x;
                scale     = remX / distTotal;
            }
        }
    }
    else
    {
        // Rotate icon for the duration time
        JonesHud_pCurInvChangedItem->pyr.yaw = (float)((JonesHud_msecDeltaTime * 16.0) / 50.0) + JonesHud_pCurInvChangedItem->pyr.yaw;
    }

    JonesHud_RenderChangedItem(JonesHud_pCurInvChangedItem, scale);
}

void J3DAPI JonesHud_RenderChangedItem(const JonesHudMenuItem* pItem, float scale)
{
    rdVector3 tvec = { 0.0f, 0.0f, 0.0f };
    rdMatrix34 placement;
    rdVector3 itemScale;
    rdVector3 svec;
    float curScale;

    rdMatrix_Copy34(&placement, &rdroid_identMatrix34);
    if ( !pItem )
    {
        return;
    }

    svec.x = (svec.y = (svec.z = 0.01820415f / pItem->prdIcon->data.pModel3->size)); // 0.018204151f - 1 / 54.9325261f
    rdMatrix_PostScale34(&placement, &svec);
    rdMatrix_PostScale34(&placement, &JonesHud_vecSelectedMenuItemScale);

    itemScale.x = (itemScale.y = (itemScale.z = (JonesHud_aDfltMenuItemOrients[pItem->id].scale)));
    rdMatrix_PostScale34(&placement, &itemScale);

    if ( (curScale = 0.01820415f * JonesHud_vecSelectedMenuItemScale.x * itemScale.x) > 0.0154452f )
    {
        float factor;
        float extra;
        rdVector3 vec;

        extra  = (curScale - 0.0154452f) * scale;
        factor = (0.0154452f + extra) / curScale;
        vec.x = (vec.y = (vec.z = factor));
        curScale = curScale * factor;
        rdMatrix_PostScale34(&placement, &vec);
    }

    // RPY rotate
    memset(&tvec, 0, sizeof(tvec));
    tvec.roll = pItem->pyr.roll;
    rdMatrix_PostRotate34(&placement, &tvec);

    memset(&tvec, 0, sizeof(tvec));
    tvec.pitch = pItem->pyr.pitch;
    rdMatrix_PostRotate34(&placement, &tvec);

    memset(&tvec, 0, sizeof(tvec));
    tvec.yaw = pItem->pyr.yaw;
    rdMatrix_PostRotate34(&placement, &tvec);

    memset(&tvec, 0, sizeof(tvec));
    rdVector_Copy3(&tvec, &pItem->pos);

    tvec.x = tvec.x - ((itemScale.x - 1.0f) * 0.75f) * 0.0242722f;
    placement.dvec = tvec;
    rdModel3_Draw(pItem->prdIcon, &placement);
}

int J3DAPI JonesHud_ShowLevelCompleted(void)
{
    int nFound;
    int treasureValue;
    int elapsedTime;
    int balance;
    int i;
    int valueFound;
    int aStoreState[14];
    SithGameStatistics* pStatistics;
    int newIqPoints;
    int bNewLevel;

    bNewLevel   = 0;
    pStatistics = sithGamesave_GetGameStatistics();
    if ( pStatistics && sithGamesave_LockGameStatistics() )
    {
        elapsedTime   = pStatistics->aLevelStatistic[pStatistics->curLevelNum - 1].elapsedTime;
        newIqPoints   = pStatistics->totalIQPoints - JonesHud_levelStartIQPoints;
        nFound        = pStatistics->aLevelStatistic[pStatistics->curLevelNum - 1].numFoundTreasures;
        treasureValue = (int)((sithInventory_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount : 0.0f);
        valueFound    = treasureValue - JonesHud_foundTreasureValue;
        sithGamesave_UnlockGameStatistics();
    }

    balance = (int)((sithInventory_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount : 0.0f);
    for ( i = 0; i < 14; ++i )
    {
        if ( JonesHud_aStoreItems[i].menuID == JONESHUD_MENU_INVITEM_BONUSMAP
            && !sithPlayer_bBonusMapBought
            && JonesMain_GetCurrentLevelNum() != JONESLEVEL_BONUSLEVELNUM
            && JonesMain_GetCurrentLevelNum() != 10
            && JonesMain_GetCurrentLevelNum() != 15
            || JonesHud_aStoreItems[i].menuID >= JONESHUD_MENU_HEALTH_SMALL && JonesHud_aStoreItems[i].menuID <= JONESHUD_MENU_HEALTH_POISONKIT )
        {
            aStoreState[i] = (JonesHud_aStoreItems[i].menuID << 16) | ((int)((sithInventory_aTypes[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].amount : 0.0f) << 4) | 1;
        }
        else
        {
            int menuID;
            int bItemAvailable;

            menuID = JonesHud_aStoreItems[i].menuID;
            bItemAvailable = JonesHud_apMenuItems[menuID]
                && ((sithInventory_aTypes[JonesHud_apMenuItems[menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
                    ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[menuID]->inventoryID].status & SITHINVENTORY_ITEM_FOUND
                    : 0);
            if ( !bItemAvailable )
            {
                aStoreState[i] = menuID << 16;
            }
            else
            {
                aStoreState[i] = (menuID << 16) | ((int)((sithInventory_aTypes[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[JonesHud_aStoreItems[i].menuID]->inventoryID].amount : 0.0f) << 4) | bItemAvailable;
            }
        }
    }

    if ( jonesConfig_ShowLevelCompletedDialog(stdWin95_GetWindow(), &balance, aStoreState, 11, elapsedTime, newIqPoints, nFound, valueFound, treasureValue) == 1 )
    {
        for ( i = 0; i < 14; ++i )
        {
            int numBought;
            int id;

            id        = (aStoreState[i] & 0xFFFF0000) >> 16;
            numBought = aStoreState[i] & 0xFFFF;
            if ( numBought > 0 )
            {
                if ( id == JONESHUD_MENU_INVITEM_BONUSMAP )
                {
                    jonesConfig_UpdateCurrentLevelNum();
                    bNewLevel = 1;
                    JonesHud_bBonusMapBought = 1;
                }
                else if ( JonesHud_apMenuItems[id] )
                {
                    sithInventory_ChangeInventory(sithPlayer_pLocalPlayerThing, JonesHud_apMenuItems[id]->inventoryID, (float)(numBought * JonesHud_aStoreItems[i].unitsPerItem));
                    sithInventory_SetInventoryAvailable(sithPlayer_pLocalPlayerThing, JonesHud_apMenuItems[id]->inventoryID, 1);
                }
            }
        }

        sithInventory_SetInventory(sithPlayer_pLocalPlayerThing, JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID, (float)balance);
    }

    pStatistics = sithGamesave_GetGameStatistics();
    if ( pStatistics && sithGamesave_LockGameStatistics() )
    {
        pStatistics->foundTreasureValue = (int)((sithInventory_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0 ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount : 0.0f);
        pStatistics->numFoundTreasures  = 0;
        sithGamesave_UnlockGameStatistics();
    }

    return bNewLevel;
}

HANDLE J3DAPI JonesHud_OpenHelp(HANDLE process)
{
    HANDLE hProc;
    wchar_t* pwHelpFile;
    char aPath[128];
    const char* pFilename;
    SHELLEXECUTEINFOA sei;


    pwHelpFile = sithString_GetString("SITHSTRING_HELPFILE");
    hProc      = 0;
    if ( pwHelpFile )
    {
        pFilename = stdUtil_ToAString(pwHelpFile);
        memset(aPath, 0, sizeof(aPath));
        wuRegistry_GetStr("Install Path", aPath, sizeof(aPath), "");
        if ( pFilename && strlen(aPath) > 0 )
        {
            memset(&sei, 0, sizeof(sei));
            sei.cbSize      = 60;
            sei.fMask       = 80;
            sei.lpVerb      = "open";
            sei.lpFile      = pFilename;
            sei.lpDirectory = aPath;
            sei.nShow       = 1;
            ShellExecuteExA(&sei);
            hProc = sei.hProcess;
        }

        if ( pFilename )
        {
            stdMemory_Free((void*)pFilename);
        }
    }

    return hProc;
}

void J3DAPI JonesHud_ShowGameOverDialog(int bPlayDiedMusic)
{
    int bError;
    char filename[JONESCONFIG_GAMESAVE_FILEPATHSIZE];
    int hSound;
    tSoundChannelHandle hSndChan;
    const char* pErrorText;

    hSndChan = 0;
    hSound   = 0;
    sithSoundMixer_StopAll();
    memset(filename, 0, sizeof(filename));
    if ( bPlayDiedMusic )
    {
        hSound = JonesHud_aSoundFxHandles[11]; // mus_gen_indydies1.wav
    }

    switch ( jonesConfig_ShowGameOverDialog(stdWin95_GetWindow(), filename, hSound, &hSndChan) )
    {
        case 1177: // restart
        case 1178: // load game
            if ( (int)hSndChan > 0 )
            {
                sithSoundMixer_StopSound(JonesHud_hCurSndChannel);
            }

            if ( strlen(filename) > 0 )
            {
                JonesHud_bRestoreGameStatistics = 1;
                sithSoundMixer_StopAll();
                if ( sithCamera_pCurCamera )
                {
                    sithCamera_ResetAllCameras();
                }

                sithGamesave_Restore(filename, 1);
                bError = JonesMain_ProcessGamesaveState();
                if ( bError )
                {
                    pErrorText = jonesString_GetString("JONES_STR_CTRLERROR");
                    if ( pErrorText )
                    {
                        JonesMain_LogErrorToFile(pErrorText);
                    }

                    JonesMain_CloseWindow();
                }
                else
                {
                    JonesHud_hudState &= ~0x10;
                    JonesHud_hudState       = 0;
                    jonesCog_bMenuVisible = 0;
                    sithGamesave_CloseRestore();
                    JonesMain_ResumeGame();
                    return;
                }
            }
            else
            {
                JonesMain_CloseWindow();
            }
            break;

        default:
            if ( (int)hSndChan > 0 )
            {
                sithSoundMixer_StopSound(hSndChan);
            }

            JonesMain_CloseWindow();
    }
}

float J3DAPI JonesHud_GetHealthBarAlpha(void)
{
    return JonesHud_healthIndAlpha;
}

void J3DAPI JonesHud_SetHealthBarAlpha(float a1)
{
    JonesHud_healthIndAlpha = a1;
}

void J3DAPI JonesHud_CutsceneStart(int a1)
{
    JonesHud_bCutsceneStart = a1;
}

void J3DAPI JonesHud_EnableInterface(int bEnable)
{
    JonesHud_bInterfaceEnabled = bEnable;
}

int J3DAPI JonesHud_DrawCredits(int bEndCredits, tSoundChannelHandle hSndChannel)
{
    float alpha;
    float lineScalar;
    int i;
    uint32_t msecCurTime;

    msecCurTime = stdPlatform_GetTimeMsec();
    lineScalar  = D3DVAL(JonesHud_creditsCanvasHeight / 480.0 < 1.0 ? 1.0 : JonesHud_creditsCanvasHeight / 480.0);

    // Update credits
    if ( !bEndCredits && !JonesHud_bSkipUpdateCredits )
    {
        if ( JonesHud_msecCreditsElapsedTime == 0 ) // initial init.
        {
            JonesHud_bEndingCredits       = 0;
            JonesHud_msecCreditsFadeStart = 0;
            JonesHud_creditTextHeight     = 0.0f;
            JonesHud_creditsCurMatIdx     = 0;
            stdDisplay_GetBackBufferSize((uint32_t*)&JonesHud_creditsCanvasWidth, (uint32_t*)&JonesHud_creditsCanvasHeight);

            JonesHud_creditsSomeHeightRatio = D3DVAL(JonesHud_creditsCanvasHeight / 8000.0);

            JonesHud_pCreditsFont1 = rdFont_Load("mat\\jonesComic Sans MS14.gcf");
            JonesHud_pCreditsFont2 = rdFont_Load("mat\\jonesCalisto MT20.gcf");
            JonesHud_curCelNum     = 0;

            memset(JonesHud_apCreditsMats, 0, sizeof(JonesHud_apCreditsMats));
            for ( i = 0; i < 325; ++i )
            {
                int bIcon;

                bIcon = JonesHud_aCredits[i].flags & 0x08;
                if ( (JonesHud_aCredits[i].flags ^ bIcon) == 0x04 )
                {
                    if ( !bIcon )
                    {
                        JonesHud_apCreditsMats[i] = rdMaterial_Load(JonesHud_aCredits[i].aText);
                    }
                    else
                    {
                        rdMaterial* pMat;

                        if ( !JonesHud_creditsCurMatIdx )
                        {
                            JonesHud_creditsCurMatIdx = i;
                            JonesHud_creditTextHeight = D3DVAL(JonesHud_pCreditsFont2->lineSpacing + 48.0 + JonesHud_pCreditsFont1->lineSpacing + (2 * JonesHud_pCreditsFont1->fontSize));
                        }

#line 4415
                        JonesHud_apCreditsMats[i] = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
                        pMat = JonesHud_apCreditsMats[i];
                        if ( pMat && rdMaterial_LoadEntry(JonesHud_aCredits[i].aText, pMat) )
                        {
                            JonesHud_aCredits[i].flags &= ~0x08;
                            rdMaterial_FreeEntry(pMat);
                            pMat = NULL;
                        }
                    }
                }
            }

            memset(JonesHud_aCreditsCurPosY, 0, sizeof(JonesHud_aCreditsCurPosY));
            JonesHud_creditsCurIdx    = 0;
            JonesHud_creditsCurEndIdx = 0;
            JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] = JonesHud_creditsCanvasHeight + 64;

            if ( !JonesHud_pCreditsFont1 || !JonesHud_pCreditsFont2 )
            {
                JonesHud_msecCreditsElapsedTime = 0;
                JonesHud_bSkipUpdateCredits     = 0;
                alpha = 0.0f;
                if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] )
                {
                    rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                    stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                    JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] = NULL;
                }

                if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] )
                {
                    rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
                    stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
                    JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] = NULL;
                }

                if ( JonesHud_pCreditsFont1 )
                {
                    rdFont_Free(JonesHud_pCreditsFont1);
                    JonesHud_pCreditsFont1 = NULL;
                }

                if ( JonesHud_pCreditsFont2 )
                {
                    rdFont_Free(JonesHud_pCreditsFont2);
                    JonesHud_pCreditsFont2 = NULL;
                }

                return 1;
            }

            JonesHud_msecCreditsElapsedTime = stdPlatform_GetTimeMsec();
            return 0;
        }
        else
        {
            float v31;
            int j;

            v31 = (msecCurTime - JonesHud_msecCreditsElapsedTime) * JonesHud_creditsSomeHeightRatio; // never read; the debug build computes and stores it

            // Update credits movement progress and sound fx
            j = JonesHud_creditsCurIdx;
            while ( j < 325 && j <= JonesHud_creditsCurEndIdx )
            {
                int bIcon;

                bIcon = JonesHud_aCredits[JonesHud_creditsCurIdx].flags & 0x08;
                JonesHud_aCreditsCurPosY[j] = JonesHud_aCreditsCurPosY[j] - (float)((JonesHud_creditsCanvasHeight / 480.0) * 1.5); // Here we move text up

                if ( j >= JonesHud_creditsCurMatIdx
                    && JonesHud_creditsCurMatIdx > 0
                    && JonesHud_aCreditsCurPosY[JonesHud_creditsCurMatIdx] > 0.0f
                    && JonesHud_aCreditsCurPosY[JonesHud_creditsCurMatIdx + 1] > 0.0f
                    && (JonesHud_creditsCanvasHeight - JonesHud_creditTextHeight) / 2.0f >= JonesHud_aCreditsCurPosY[JonesHud_creditsCurMatIdx] )
                {
                    // At the end credits, stop LEC logo and copyright at the center
                    switch ( j - JonesHud_creditsCurMatIdx )
                    {
                        case 0:
                        case 1: // LEC icon
                            JonesHud_aCreditsCurPosY[j] = (JonesHud_creditsCanvasHeight - JonesHud_creditTextHeight) / 2.0f;
                            break;

                        case 2: // LEC text
                            JonesHud_aCreditsCurPosY[j] = (float)((JonesHud_pCreditsFont1->lineSpacing + 48.0) * lineScalar) + JonesHud_aCreditsCurPosY[j - 1];
                            break;

                        case 3: // Copyright text
                            JonesHud_aCreditsCurPosY[j] = (JonesHud_pCreditsFont1->lineSpacing * lineScalar) + JonesHud_aCreditsCurPosY[j - 1];
                            break;
                    }
                }

                if ( j == JonesHud_creditsCurIdx )
                {
                    switch ( JonesHud_aCredits[JonesHud_creditsCurIdx].flags ^ bIcon )
                    {
                        case 0x04: // icon image
                            if ( JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] < -64.0f )
                            {
                                ++JonesHud_creditsCurIdx;
                            }

                            if ( JonesHud_creditsCurIdx == JonesHud_creditsCurMatIdx && JonesHud_creditsCurMatIdx > 0 )
                            {
                                if ( !JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] || !JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] )
                                {
                                    JonesHud_bSkipUpdateCredits = 1;
                                }
                                else
                                {
                                    int prevCelNum;
                                    tSoundHandle hSnd;

                                    prevCelNum = JonesHud_curCelNum;
                                    if ( !JonesHud_msecCreditsFadeStart )
                                    {
                                        JonesHud_msecCreditsFadeStart = msecCurTime;
                                    }

                                    JonesHud_curCelNum = JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]->numCels * (msecCurTime - JonesHud_msecCreditsFadeStart) / 1000;
                                    if ( prevCelNum == 12 && JonesHud_curCelNum == 13 ) // gg_a.mat whip fire cel 12 & 13
                                    {
                                        hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(3)); // gen_whip_fire.wav
                                        if ( hSnd )
                                        {
                                            JonesHud_hCurSndChannel = sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, SOUNDPLAY_PLAYONCE);
                                        }
                                    }

                                    if ( JonesHud_curCelNum == JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]->numCels && !JonesHud_bSkipUpdateCredits )
                                    {
                                        JonesHud_bSkipUpdateCredits   = 1;
                                        JonesHud_msecCreditsFadeStart = msecCurTime;
                                    }
                                }
                            }
                            break;

                        case 0x02:
                        case 0x10:
                            if ( -JonesHud_pCreditsFont1->fontSize > JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] )
                            {
                                ++JonesHud_creditsCurIdx;
                            }
                            break;

                        case 0x01:
                            if ( -JonesHud_pCreditsFont2->fontSize > JonesHud_aCreditsCurPosY[JonesHud_creditsCurIdx] )
                            {
                                ++JonesHud_creditsCurIdx;
                            }
                            break;
                    }
                }

                ++j;
            }
        }
    }

    // End credits?
    if ( bEndCredits && !JonesHud_bEndingCredits )
    {
        if ( hSndChannel )
        {
            sithSoundMixer_FadeVolume(hSndChannel, 0.0f, 500.0f);
        }

        JonesHud_msecCreditsFadeStart = msecCurTime;
        JonesHud_bEndingCredits       = 1;
    }
    else if ( JonesHud_bSkipUpdateCredits
        && !JonesHud_bEndingCredits
        && (!JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] || !JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] || JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] && JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] && msecCurTime - JonesHud_msecCreditsFadeStart >= 1000.0) )
    {
        if ( hSndChannel )
        {
            sithSoundMixer_FadeVolume(hSndChannel, 0.0f, 500.0f);
        }

        JonesHud_msecCreditsFadeStart = msecCurTime;
        JonesHud_bEndingCredits       = 1;
    }

    if ( JonesHud_bEndingCredits )
    {
        if ( (alpha = 1.0f - (float)((msecCurTime - JonesHud_msecCreditsFadeStart) / 1000.0)) <= 0.0f )
        {
            // Finished fade out
            JonesHud_msecCreditsElapsedTime = 0;
            JonesHud_bSkipUpdateCredits     = 0;
            alpha = 0.0f;

            rdFont_Free(JonesHud_pCreditsFont1);
            rdFont_Free(JonesHud_pCreditsFont2);
            JonesHud_pCreditsFont1 = NULL;
            JonesHud_pCreditsFont2 = NULL;

            if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] )
            {
                rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx]);
                JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx] = NULL;
            }

            if ( JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] )
            {
                rdMaterial_FreeEntry(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
                stdMemory_Free(JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1]);
                JonesHud_apCreditsMats[JonesHud_creditsCurMatIdx + 1] = NULL;
            }

            return 1;
        }
    }
    else
    {
        alpha = 1.0f;
    }

    // Draw credits
    i = JonesHud_creditsCurIdx;
    while ( JonesHud_creditsCanvasHeight + 64 >= JonesHud_aCreditsCurPosY[i] && i < 325 && i <= JonesHud_creditsCurEndIdx )
    {
        int bIcon;

        bIcon = JonesHud_aCredits[i].flags & 0x08;
        switch ( JonesHud_aCredits[i].flags ^ bIcon )
        {
            case 0x04: // mat icon
            {
                const rdMaterial* pIconMat;

                pIconMat = JonesHud_apCreditsMats[i];
                if ( pIconMat )
                {
                    JonesHudRect rect;
                    int colorId;
                    rdVector4 fontColor;

                    colorId     = JonesHud_aCredits[i].fontColorNum;
                    rect.y      = (JonesHud_aCreditsCurPosY[i]);
                    rect.height = 64.0f;
                    rect.width  = 64.0f;
                    if ( i == JonesHud_creditsCurMatIdx + 1 && JonesHud_creditsCurMatIdx > 0 )
                    {
                        rect.x = (JonesHud_creditsCanvasWidth - 64) / 2 + 64;
                    }
                    else
                    {
                        rect.x = (JonesHud_creditsCanvasWidth - 64) / 2;
                    }

                    rdVector_Copy4(&fontColor, &JonesHud_aCreditFontColors[colorId]);
                    fontColor.alpha = alpha;
                    JonesHud_Draw(pIconMat, &rect, RD_FIXEDPOINT_RHW_SCALE_X1, RD_FIXEDPOINT_RHW_SCALE_X1, &fontColor, JonesHud_curCelNum, 1);
                }
                else
                {
                    JonesHud_aCredits[i].flags &= ~0x08;
                }

                if ( i + 1 < 325 && JonesHud_aCreditsCurPosY[i + 1] == 0.0f && i + 1 > JonesHud_creditsCurEndIdx && !bEndCredits )
                {
                    if ( i == JonesHud_creditsCurMatIdx && JonesHud_creditsCurMatIdx > 0 )
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = JonesHud_aCreditsCurPosY[i];
                    }
                    else
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = (float)((JonesHud_pCreditsFont1->lineSpacing + 48.0) * lineScalar) + JonesHud_aCreditsCurPosY[i];
                    }

                    JonesHud_creditsCurEndIdx = i + 1;
                }
            } break;

            case 0x02:  // colored
            case 0x10: // white color
            {
                rdFontColor textColor;
                int k;
                int colorIdx;
                const char* pText;

                pText    = NULL;
                colorIdx = JonesHud_aCredits[i].fontColorNum;
                for ( k = 0; k < 4; ++k )
                {
                    if ( JonesHud_aCredits[i].flags == 0x10 )
                    {
                        rdVector_Copy4(&textColor[k], &JonesHud_colorWhite);
                    }
                    else
                    {
                        rdVector_Copy4(&textColor[k], &JonesHud_aCreditFontColors[colorIdx]);
                    }

                    textColor[k].alpha = alpha;
                }

                rdFont_SetFontColor(textColor);
                pText = jonesString_GetString(JonesHud_aCredits[i].aText);
                if ( pText )
                {
                    rdFont_DrawTextLine(pText, 0.5f, JonesHud_aCreditsCurPosY[i] / JonesHud_creditsCanvasHeight, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pCreditsFont1, RDFONT_ALIGNCENTER);
                }
                else
                {
                    rdFont_DrawTextLine(JonesHud_aCredits[i].aText, 0.5f, JonesHud_aCreditsCurPosY[i] / JonesHud_creditsCanvasHeight, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pCreditsFont1, RDFONT_ALIGNCENTER);
                }

                if ( i + 1 < 325 && JonesHud_aCreditsCurPosY[i + 1] == 0.0f && i + 1 > JonesHud_creditsCurEndIdx && !bEndCredits )
                {
                    if ( JonesHud_aCredits[i].flags == 0x10 )
                    {
                        JonesHud_aCreditsCurPosY[i + 1] = (JonesHud_pCreditsFont2->lineSpacing * lineScalar) + JonesHud_aCreditsCurPosY[i];
                    }
                    else
                    {
                        int bNextIcon;

                        bNextIcon = JonesHud_aCredits[i + 1].flags & 0x08;
                        switch ( JonesHud_aCredits[i + 1].flags ^ bNextIcon )
                        {
                            case 0x01:
                            case 0x04:
                            case 0x10:
                                if ( i + 1 != 318 && i + 1 != 315 )
                                {
                                    JonesHud_aCreditsCurPosY[i + 1] = (float)((JonesHud_pCreditsFont1->lineSpacing + 24.0) * lineScalar) + JonesHud_aCreditsCurPosY[i];
                                }
                                else
                                {
                                    JonesHud_aCreditsCurPosY[i + 1] = (JonesHud_pCreditsFont2->lineSpacing * lineScalar) + JonesHud_aCreditsCurPosY[i];
                                }
                                break;

                            case 0x02:
                                JonesHud_aCreditsCurPosY[i + 1] = (JonesHud_pCreditsFont1->lineSpacing * lineScalar) + JonesHud_aCreditsCurPosY[i];
                                break;
                        }
                    }

                    JonesHud_creditsCurEndIdx = i + 1;
                }
            } break;

            case 0x01: // Ascii text
            {
                rdFontColor fontColor;
                int k;
                const char* pText;

                pText = NULL;
                for ( k = 0; k < 4; ++k )
                {
                    rdVector_Copy4(&fontColor[k], &JonesHud_colorWhite);
                    fontColor[k].alpha = alpha;
                }

                rdFont_SetFontColor(fontColor);
                rdFont_DrawTextLine(JonesHud_aCredits[i].aText, 0.5f, JonesHud_aCreditsCurPosY[i] / JonesHud_creditsCanvasHeight, RD_FIXEDPOINT_RHW_SCALE_X1, JonesHud_pCreditsFont2, RDFONT_ALIGNCENTER);

                if ( i + 1 < 325 && JonesHud_aCreditsCurPosY[i + 1] == 0.0f && i + 1 > JonesHud_creditsCurEndIdx && !bEndCredits )
                {
                    int bNextIcon;

                    bNextIcon = JonesHud_aCredits[i + 1].flags & 0x08;
                    switch ( JonesHud_aCredits[i + 1].flags ^ bNextIcon )
                    {
                        case 0x04:
                            JonesHud_aCreditsCurPosY[i + 1] = (float)((JonesHud_pCreditsFont1->lineSpacing + 48.0) * lineScalar) + JonesHud_aCreditsCurPosY[i];
                            break;

                        case 0x10:
                            JonesHud_aCreditsCurPosY[i + 1] = (float)((JonesHud_pCreditsFont2->lineSpacing * 2.25) * lineScalar) + JonesHud_aCreditsCurPosY[i];
                            break;

                        default:
                            JonesHud_aCreditsCurPosY[i + 1] = (float)((JonesHud_pCreditsFont2->lineSpacing * 1.25) * lineScalar) + JonesHud_aCreditsCurPosY[i];
                            break;
                    }

                    JonesHud_creditsCurEndIdx = i + 1;
                }
            } break;
        }

        ++i;
    }

    JonesHud_msecCreditsElapsedTime = msecCurTime;
    return 0;
}

void J3DAPI JonesHud_InitializeGameStatistics(void)
{
    SithGameStatistics* pStatistics;

    pStatistics = sithGamesave_GetGameStatistics();
    if ( !JonesHud_bRestoreGameStatistics )
    {
        JonesHud_numFoundTreasures  = 0;
        JonesHud_foundTreasureValue = (int)((sithInventory_aTypes[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].flags & SITHINVENTORY_TYPE_REGISTERED) != 0
            ? sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.pPlayer->inventory.aItems[JonesHud_apMenuItems[JONESHUD_MENU_TREASURE_CHEST]->inventoryID].amount
            : 0.0f);

        if ( pStatistics && sithGamesave_LockGameStatistics() )
        {
            if ( !pStatistics->curLevelNum )
            {
                JonesHud_levelStartIQPoints = 150;
            }
            else
            {
                JonesHud_levelStartIQPoints = pStatistics->totalIQPoints;
            }

            sithGamesave_UnlockGameStatistics();
        }
    }
    else
    {
        if ( pStatistics && sithGamesave_LockGameStatistics() )
        {
            JonesHud_levelStartIQPoints = pStatistics->aLevelStatistic[pStatistics->curLevelNum].levelStartIQPoints;
            JonesHud_foundTreasureValue = pStatistics->foundTreasureValue;
            JonesHud_numFoundTreasures  = pStatistics->numFoundTreasures;
            sithGamesave_UnlockGameStatistics();
        }

        JonesHud_bRestoreGameStatistics = 0;
    }

    jonesInventory_ResetStatisticsGameTime();
}

void J3DAPI JonesHud_RestoreGameStatistics(void)
{
    JonesHud_bRestoreGameStatistics = 1;
}

void J3DAPI JonesHud_RestoreTreasuresStatistics(void)
{
    SithGameStatistics* pStatistics;

    pStatistics = sithGamesave_GetGameStatistics();
    if ( pStatistics )
    {
        if ( sithGamesave_LockGameStatistics() )
        {
            pStatistics->foundTreasureValue = JonesHud_foundTreasureValue;
            pStatistics->numFoundTreasures = JonesHud_numFoundTreasures;
            sithGamesave_UnlockGameStatistics();
        }
    }
}
