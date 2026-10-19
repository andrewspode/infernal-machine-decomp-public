#include "jonesConfig.h"

#include <Jones3D/Display/JonesHud.h>
#include <Jones3D/Display/JonesHudConstants.h>
#include <Jones3D/Gui/JonesDialog.h>
#include <Jones3D/Main/JonesFile.h>
#include <Jones3D/Main/JonesLevel.h>
#include <Jones3D/Main/JonesMain.h>
#include <Jones3D/Main/jonesString.h>
#include <Jones3D/Play/JonesControl.h>
#include <Jones3D/Play/jonesInventory.h>


#include <sith/Devices/sithControl.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/DSS/sithGamesave.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWorld.h>

#include <std/General/std.h>
#include <std/General/stdBmp.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdControl.h>
#include <std/Win95/stdDisplay.h>
#include <std/Win95/stdWin95.h>

#include <sound/Sound.h>

#include <w32util/wuRegistry.h>

#include <math.h>
#include <windowsx.h>

// Joystick AXIS key IDs
#define JONESCONFIG_JOYAXISKID_UP       0x8000
#define JONESCONFIG_JOYAXISKID_RIGHT    0x8001
#define JONESCONFIG_JOYAXISKID_DOWN     0x8002
#define JONESCONFIG_JOYAXISKID_LEFT     0x8003
#define JONESCONFIG_JOYAXISKID_TWISTL   0x8004
#define JONESCONFIG_JOYAXISKID_TWISTR   0x8005
#define JONESCONFIG_JOYAXISKID_AIR      0x8006
#define JONESCONFIG_JOYAXISKID_GROUND   0x8007
#define JONESCONFIG_NUM_JOYAXISKIDS     8

#define JONESCONFIG_CONTROLSTRING_MAXLEN 256u

#define JONESCONFIG_NOFONTSCALEMASK 1119
#define JONESCONFIG_NOFONTSCALE(dlgID) MAKELONG(dlgID, JONESCONFIG_NOFONTSCALEMASK)

typedef struct sJonesConfigControlAction
{
    const int numBinds;
    const size_t* aBindings;
} JonesConfigControlAction;

typedef struct sJonesMessageDialogData
{
    int iconID;
    char aTitle[256];
    char aText[256];
    HICON hPrevIcon;
} JonesMessageDialogData;

typedef struct sJonesCreateControlSchemeDialogData
{
    char aNewSchemeName[128];
    HWND hSchemeList;
} JonesCreateControlSchemeDialogData;

typedef struct sJonesEditControlSchemeDialogData
{
    JonesControlsScheme* aSchemes;
    int numSchemes;
    HWND hCurList;
    int curListID;
    JonesControlsScheme* pDefaultShceme;
} JonesEditControlSchemeDialogData;

typedef struct sJonesAssignKeyDialogData
{
    int actionId;
    JonesControlsScheme* pScheme;
    char aControlName[256];
    size_t controlId;
    int listID;
    int bLocked;
} JonesAssignKeyDialogData;

typedef struct sJonesReAssignKeyDialogData
{
    JonesAssignKeyDialogData* pAssignKeyData;
    int index;
    size_t offset;
    size_t actionId;
    HWND hAssignKeyDlg;
} JonesReAssignKeyDialogData;


typedef struct sJonesDisplaySettingsVideoMode
{
    char aResolutionText[256];
    uint8_t colorDepthFlags;
    int aColorDepthVideoModes[5];
} JonesDisplaySettingsVideoMode;

typedef struct sJonesDisplaySettingsDialogData
{
    JonesDisplaySettings* pSettings;
    StdDisplayEnvironment* pDisplayEnv;
    JonesDisplaySettingsVideoMode* aVideoModes;
    const JonesDisplaySettingsVideoMode** aCurColorDepthResolutions;
    int numCurColorDepthResolutions;
    int numVideoModes;
} JonesDisplaySettingsDialogData;

typedef struct sLoadGameDialogData
{
    HDC hdcThumbnail;
    char aFilePath[128];
    HBITMAP hThumbnail;
    int pfThubnailProc;
    int bFolderSel;
    int unknown36;
    int unknown37;
} LoadGameDialogData;

typedef struct sSaveGameDialogData
{
    HDC hdcThumbnail;
    char aFilePath[128];
    HBITMAP hThumbnail;
    int pfThumbnailProc;
    int bFolderSel;
} SaveGameDialogData;

typedef struct sJonesDialogFontScaleMetrics
{
    int dialogID;
    int refWidth;
    int refHeight;
} JonesDialogSize;



// Module vars
// Retail .bss (0x005510B8..0x00551E30): the uninitialized statics come first in VC5's
// symbol-hash order, then every zero-initialized definition in declaration order
// the definitions below follow retail's addresses.
static HBITMAP jonesConfig_apDlgIcons[6];
static char jonesConfig_aKeySetDirectory[128];
static ABC jonesConfig_aGlyphMetrics[256];
static TEXTMETRICA jonesConfig_textMetric;
static JonesControlsConfig jonesConfig_controlOptions_curControlConfig;

static int jonesConfig_gamePlayOptions_bDefaultRun = 0;
int jonesConfig_gameStatistics_curLevelNum = 0;
static int jonesConfig_bStartup = 0;
static int jonesConfig_bTextMetricInited = 0;

// Message dialog
HFONT jonesConfig_hFontMessageDialog = NULL;

// Game save dialog
static HFONT jonesConfig_hFontSaveGameDlg = NULL;

// Exit game dialog
static HFONT jonesConfig_hFontExitDlg = NULL;

// Override game save/load message box dialog
HFONT jonesConfig_hFontGameSaveMsgBox = NULL;

// Game load dialog
static HFONT jonesConfig_hFontLoadGameDlg = NULL;

// Game play options dialog
static HFONT jonesConfig_hFontGamePlayOptionsDlg = NULL;

// Control options dialog
static HFONT jonesConfig_hFontControlOptions = NULL;
static JonesControlsScheme* jonesConfig_controlOptions_pCurNewScheme = NULL;

// Create control scheme
HFONT jonesConfig_hFontCreateControlSchemeDlg = NULL;

// Edit control scheme dialog
HFONT jonesConfig_hFontEditControlShceme = NULL;

// Assign key control options dialog
HFONT jonesConfig_hFontAssignKeyDlg = NULL;
int jonesConfig_assignKey_bControlsActive = 0;

// Re-assign key control options dialog
HFONT jonesConfig_hFontReassignKeyDlg = NULL;

// Display settings dialog
static HFONT jonesConfig_hFontDisplaySettingsDlg = NULL;

// Advance display settings dialog
static HFONT jonesConfig_hFontAdvanceDisplaySettingsDialog = NULL;

// Sound settings dialog
static HFONT jonesConfig_hFontSoundSettings = NULL;
static tSoundChannelHandle jonesConfig_soundSettings_hSndChannel = 0;
static int jonesConfig_soundSettings_curSoundVolume = 0;

// Game overr dialog
static HFONT jonesConfig_hFontGameOverDialog = NULL;

// Game statistics dialog
static HFONT jonesConfig_hFontGameStatisticsDialog = NULL;

// Chapter completed dialog
static HFONT jonesConfig_hFontLevelCopletedDialog = NULL;

// Store dialog
static HFONT jonesConfig_hFontStoreDialog = NULL;
static HFONT jonesConfig_hFontPurchaseMessageBox = NULL;

// CD dialog
static HFONT jonesConfig_hFontDialogInsertCD = NULL;

// Control keyset scheme serialization vars
static int jonesConfig_largestActionStrLen;
static int jonesConfig_maxActionWords;
static int jonesConfig_maxActionPaddedStrLen;
static int jonesConfig_maxControlLen;
static int jonesConfig_maxControlKeyWordCount;
static int jonesConfig_maxControlPaddedStrLen;



// jonesConfig_DrawStatisticDialogIQPoints
static StdRect jonesConfig_aNumberGlyphMetrics[10] =
{
    { 11, 2, 44, 45 },
    { 49, 2, 69, 45 },
    { 78, 2, 106, 45 },
    { 110, 2, 138, 45 },
    { 11, 76, 44, 120 },
    { 45, 76, 75, 120 },
    { 77, 76, 107, 120 },
    { 11, 150, 45, 194 },
    { 45, 150, 75, 194 },
    { 76, 150, 107, 194 }
};

static const char* jonesConfig_apDialogIconFiles[6] =
{
    "iq.bmp",
    "iqMask.bmp",
    "iqOverlay.bmp",
    "numbers.bmp",
    "exit.bmp",
    "exitmask.bmp"
};

static const char* jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_NUMLEVELS] = // level names, then the summary page's title
{
    "JONES_STR_CYN",
    "JONES_STR_BAB",
    "JONES_STR_RIV",
    "JONES_STR_SHS",
    "JONES_STR_LAG",
    "JONES_STR_VOL",
    "JONES_STR_TEM",
    "JONES_STR_JEP",
    "JONES_STR_TEO",
    "JONES_STR_OLV",
    "JONES_STR_SEA",
    "JONES_STR_PYR",
    "JONES_STR_SOL",
    "JONES_STR_NUB",
    "JONES_STR_INF",
    "JONES_STR_AET",
    "JONES_STR_PRU",
    "JONES_STR_SUMMARY"
};

// The action string names
static const char* jonesConfig_aJonesControlActionNames[JONESCONTROL_ACTION_NUMACTIONS] =
{
    "JONES_STR_WLKFWD",
    "JONES_STR_WLKBK",
    "JONES_STR_TRNLFT",
    "JONES_STR_TRNRGHT",
    "JONES_STR_STPLFT",
    "JONES_STR_STPRGHT",
    "JONES_STR_CRAWL",
    "JONES_STR_RUN",
    "JONES_STR_ROLL",
    "JONES_STR_JUMP",
    "JONES_STR_LOOK",
    "JONES_STR_ACT",
    "JONES_STR_TGLWEAP",
    "JONES_STR_PREVWEAP",
    "JONES_STR_NEXTWEAP",
    "JONES_STR_TGLLGHT",
    "JONES_STR_FISTS",
    "JONES_STR_WHIP",
    "JONES_STR_MAUSER",
    "JONES_STR_PPSH41",
    "JONES_STR_PISTOL",
    "JONES_STR_SIMONOV",
    "JONES_STR_TOKEREV",
    "JONES_STR_TOZ34",
    "JONES_STR_BAZOOKA",
    "JONES_STR_MACHETE",
    "JONES_STR_SATCHEL",
    "JONES_STR_GRENADE",
    "JONES_STR_MAP",
    "JONES_STR_INTERFACE",
    "JONES_STR_HEALTH",
    "JONES_STR_IMP1",
    "JONES_STR_IMP2",
    "JONES_STR_IMP3",
    "JONES_STR_IMP4",
    "JONES_STR_IMP5",
    "JONES_STR_CHALK"
};

static const char* jonesConfig_aJonesCapControlActionNames[JONESCONTROL_ACTION_NUMACTIONS] =
{
    "JONES_STR_CAPS_WLKFWD",
    "JONES_STR_CAPS_WLKBK",
    "JONES_STR_CAPS_TRNLFT",
    "JONES_STR_CAPS_TRNRGHT",
    "JONES_STR_CAPS_STPLFT",
    "JONES_STR_CAPS_STPRGHT",
    "JONES_STR_CAPS_CRAWL",
    "JONES_STR_CAPS_RUN",
    "JONES_STR_CAPS_ROLL",
    "JONES_STR_CAPS_JUMP",
    "JONES_STR_CAPS_LOOK",
    "JONES_STR_CAPS_ACT",
    "JONES_STR_CAPS_TGLWEAP",
    "JONES_STR_CAPS_PREVWEAP",
    "JONES_STR_CAPS_NEXTWEAP",
    "JONES_STR_CAPS_TGLLGHT",
    "JONES_STR_CAPS_FISTS",
    "JONES_STR_CAPS_WHIP",
    "JONES_STR_CAPS_MAUSER",
    "JONES_STR_CAPS_PPSH41",
    "JONES_STR_CAPS_PISTOL",
    "JONES_STR_CAPS_SIMONOV",
    "JONES_STR_CAPS_TOKEREV",
    "JONES_STR_CAPS_TOZ34",
    "JONES_STR_CAPS_BAZOOKA",
    "JONES_STR_CAPS_MACHETE",
    "JONES_STR_CAPS_SATCHEL",
    "JONES_STR_CAPS_GRENADE",
    "JONES_STR_CAPS_MAP",
    "JONES_STR_CAPS_INTERFACE",
    "JONES_STR_CAPS_HEALTH",
    "JONES_STR_CAPS_IMP1",
    "JONES_STR_CAPS_IMP2",
    "JONES_STR_CAPS_IMP3",
    "JONES_STR_CAPS_IMP4",
    "JONES_STR_CAPS_IMP5",
    "JONES_STR_CAPS_CHALK"
};


// Keyboard default keysets

static size_t aActionWalkFwdKeys[2]     = { DIK_UP   , JONESCONFIG_JOYAXISKID_UP };
static size_t aActionWalkBackKeys[2]    = { DIK_DOWN , JONESCONFIG_JOYAXISKID_DOWN };
static size_t aActionTurnLeftKeys[2]    = { DIK_LEFT , JONESCONFIG_JOYAXISKID_LEFT };
static size_t aActionTurnRightKeys[2]   = { DIK_RIGHT, JONESCONFIG_JOYAXISKID_RIGHT };
static size_t aActionStepLeftKeys[1]    = { DIK_A };
static size_t aActionStepRightKeys[1]   = { DIK_S };
static size_t aActionCrawlKeys[1]       = { DIK_C };
static size_t aActionRunKeys[2]         = { DIK_LSHIFT, DIK_RSHIFT };
static size_t aActionRollKeys[1]        = { DIK_Z };
static size_t aActionJumpKeys[4]        = { DIK_X, DIK_LMENU, DIK_RMENU, STDCONTROL_JOYSTICK_GETBUTTON(0, 1) };
static size_t aActionLookKeys[2]        = { DIK_INSERT, DIK_NUMPAD0 };
static size_t aActionActivateKeys[3]    = { DIK_LCONTROL, DIK_RCONTROL, STDCONTROL_JOYSTICK_GETBUTTON(0, 0) };
static size_t aActionToggleMapKeys[1]   = { DIK_SPACE };
static size_t aActionPrevWeaponKeys[1]  = { DIK_COMMA };
static size_t aActionNextWeaponKeys[1]  = { DIK_PERIOD };
static size_t aActionToggleLightKeys[1] = { DIK_L };
static size_t aActionFistsKeys[1]       = { DIK_0 };
static size_t aActionWhipKeys[1]        = { DIK_1 };
static size_t aActionMauserKeys[1]      = { DIK_4 };
static size_t aActionPPSH41Keys[1]      = { DIK_7 };
static size_t aActionPistolKeys[1]      = { DIK_2 };
static size_t aActionSimonovKeys[1]     = { DIK_5 };
static size_t aActionTokarevKeys[1]     = { DIK_3 };
static size_t aActionToz34Keys[1]       = { DIK_8 };
static size_t aActionBazookaKeys[1]     = { DIK_9 };
static size_t aActionMacheteKeys[1]     = { DIK_6 };
static size_t aActionSatchelKeys[1]     = { DIK_P };
static size_t aActionGrenadeKeys[1]     = { DIK_G };
static size_t aActionMapKeys[1]         = { DIK_M };
static size_t aActionInterfaceKeys[1]   = { DIK_I };
static size_t aActionHealthKeys[1]      = { DIK_H };
static size_t aActionIMP1Keys[1]        = { DIK_Q };
static size_t aActionIMP2Keys[1]        = { DIK_W };
static size_t aActionIMP3Keys[1]        = { DIK_E };
static size_t aActionIMP4Keys[1]        = { DIK_R };
static size_t aActionIMP5Keys[1]        = { DIK_T };
static size_t aActionChalk[1]           = { DIK_K };

static JonesConfigControlAction jonesConfig_aDefaultControlSet[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)    , aActionWalkFwdKeys     },
    { STD_ARRAYLEN(aActionWalkBackKeys)   , aActionWalkBackKeys    },
    { STD_ARRAYLEN(aActionTurnLeftKeys)   , aActionTurnLeftKeys    },
    { STD_ARRAYLEN(aActionTurnRightKeys)  , aActionTurnRightKeys   },
    { STD_ARRAYLEN(aActionStepLeftKeys)   , aActionStepLeftKeys    },
    { STD_ARRAYLEN(aActionStepRightKeys)  , aActionStepRightKeys   },
    { STD_ARRAYLEN(aActionCrawlKeys)      , aActionCrawlKeys       },
    { STD_ARRAYLEN(aActionRunKeys)        , aActionRunKeys         },
    { STD_ARRAYLEN(aActionRollKeys)       , aActionRollKeys        },
    { STD_ARRAYLEN(aActionJumpKeys)       , aActionJumpKeys        },
    { STD_ARRAYLEN(aActionLookKeys)       , aActionLookKeys        },
    { STD_ARRAYLEN(aActionActivateKeys)   , aActionActivateKeys    },
    { STD_ARRAYLEN(aActionToggleMapKeys)  , aActionToggleMapKeys   },
    { STD_ARRAYLEN(aActionPrevWeaponKeys) , aActionPrevWeaponKeys  },
    { STD_ARRAYLEN(aActionNextWeaponKeys) , aActionNextWeaponKeys  },
    { STD_ARRAYLEN(aActionToggleLightKeys), aActionToggleLightKeys },
    { STD_ARRAYLEN(aActionFistsKeys)      , aActionFistsKeys       },
    { STD_ARRAYLEN(aActionWhipKeys)       , aActionWhipKeys        },
    { STD_ARRAYLEN(aActionMauserKeys)     , aActionMauserKeys      },
    { STD_ARRAYLEN(aActionPPSH41Keys)     , aActionPPSH41Keys      },
    { STD_ARRAYLEN(aActionPistolKeys)     , aActionPistolKeys      },
    { STD_ARRAYLEN(aActionSimonovKeys)    , aActionSimonovKeys     },
    { STD_ARRAYLEN(aActionTokarevKeys)    , aActionTokarevKeys     },
    { STD_ARRAYLEN(aActionToz34Keys)      , aActionToz34Keys       },
    { STD_ARRAYLEN(aActionBazookaKeys)    , aActionBazookaKeys     },
    { STD_ARRAYLEN(aActionMacheteKeys)    , aActionMacheteKeys     },
    { STD_ARRAYLEN(aActionSatchelKeys)    , aActionSatchelKeys     },
    { STD_ARRAYLEN(aActionGrenadeKeys)    , aActionGrenadeKeys     },
    { STD_ARRAYLEN(aActionMapKeys)        , aActionMapKeys         },
    { STD_ARRAYLEN(aActionInterfaceKeys)  , aActionInterfaceKeys   },
    { STD_ARRAYLEN(aActionHealthKeys)     , aActionHealthKeys      },
    { STD_ARRAYLEN(aActionIMP1Keys)       , aActionIMP1Keys        },
    { STD_ARRAYLEN(aActionIMP2Keys)       , aActionIMP2Keys        },
    { STD_ARRAYLEN(aActionIMP3Keys)       , aActionIMP3Keys        },
    { STD_ARRAYLEN(aActionIMP4Keys)       , aActionIMP4Keys        },
    { STD_ARRAYLEN(aActionIMP5Keys)       , aActionIMP5Keys        },
    { STD_ARRAYLEN(aActionChalk)          , aActionChalk           }
};

// Joystick 2 Axis 4 buttons default keysets

static size_t aActionCrawlJoyButtons[2]     = { DIK_C, STDCONTROL_JOYSTICK_GETBUTTON(0, 2) };
static size_t aActionInterfaceJoyButtons[2] = { DIK_I, STDCONTROL_JOYSTICK_GETBUTTON(0, 3) };

static JonesConfigControlAction jonesConfig_aDfault2Axis4Button[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)        , aActionWalkFwdKeys },
    { STD_ARRAYLEN(aActionWalkBackKeys)       , aActionWalkBackKeys },
    { STD_ARRAYLEN(aActionTurnLeftKeys)       , aActionTurnLeftKeys },
    { STD_ARRAYLEN(aActionTurnRightKeys)      , aActionTurnRightKeys },
    { STD_ARRAYLEN(aActionStepLeftKeys)       , aActionStepLeftKeys },
    { STD_ARRAYLEN(aActionStepRightKeys)      , aActionStepRightKeys },
    { STD_ARRAYLEN(aActionCrawlJoyButtons)    , aActionCrawlJoyButtons },
    { STD_ARRAYLEN(aActionRunKeys)            , aActionRunKeys },
    { STD_ARRAYLEN(aActionRollKeys)           , aActionRollKeys },
    { STD_ARRAYLEN(aActionJumpKeys)           , aActionJumpKeys },
    { STD_ARRAYLEN(aActionLookKeys)           , aActionLookKeys },
    { STD_ARRAYLEN(aActionActivateKeys)       , aActionActivateKeys },
    { STD_ARRAYLEN(aActionToggleMapKeys)      , aActionToggleMapKeys },
    { STD_ARRAYLEN(aActionPrevWeaponKeys)     , aActionPrevWeaponKeys },
    { STD_ARRAYLEN(aActionNextWeaponKeys)     , aActionNextWeaponKeys },
    { STD_ARRAYLEN(aActionToggleLightKeys)    , aActionToggleLightKeys },
    { STD_ARRAYLEN(aActionFistsKeys)          , aActionFistsKeys },
    { STD_ARRAYLEN(aActionWhipKeys)           , aActionWhipKeys },
    { STD_ARRAYLEN(aActionMauserKeys)         , aActionMauserKeys },
    { STD_ARRAYLEN(aActionPPSH41Keys)         , aActionPPSH41Keys },
    { STD_ARRAYLEN(aActionPistolKeys)         , aActionPistolKeys },
    { STD_ARRAYLEN(aActionSimonovKeys)        , aActionSimonovKeys },
    { STD_ARRAYLEN(aActionTokarevKeys)        , aActionTokarevKeys },
    { STD_ARRAYLEN(aActionToz34Keys)          , aActionToz34Keys },
    { STD_ARRAYLEN(aActionBazookaKeys)        , aActionBazookaKeys },
    { STD_ARRAYLEN(aActionMacheteKeys)        , aActionMacheteKeys },
    { STD_ARRAYLEN(aActionSatchelKeys)        , aActionSatchelKeys },
    { STD_ARRAYLEN(aActionGrenadeKeys)        , aActionGrenadeKeys },
    { STD_ARRAYLEN(aActionMapKeys)            , aActionMapKeys },
    { STD_ARRAYLEN(aActionInterfaceJoyButtons), aActionInterfaceJoyButtons },
    { STD_ARRAYLEN(aActionHealthKeys)         , aActionHealthKeys },
    { STD_ARRAYLEN(aActionIMP1Keys)           , aActionIMP1Keys },
    { STD_ARRAYLEN(aActionIMP2Keys)           , aActionIMP2Keys },
    { STD_ARRAYLEN(aActionIMP3Keys)           , aActionIMP3Keys },
    { STD_ARRAYLEN(aActionIMP4Keys)           , aActionIMP4Keys },
    { STD_ARRAYLEN(aActionIMP5Keys)           , aActionIMP5Keys },
    { STD_ARRAYLEN(aActionChalk)              , aActionChalk }
};

// Joystick 2 Axis 8 buttons default keysets

static size_t aActionRollJoyButtons[2]         = { DIK_CAPITAL, STDCONTROL_JOYSTICK_GETBUTTON(0, 4) };
static size_t aActionNextWeaponJoyButtons[2]   = { DIK_PERIOD, STDCONTROL_JOYSTICK_GETBUTTON(0, 5) };
static size_t aActionToggleWeaponJoyButtons[2] = { DIK_SPACE, STDCONTROL_JOYSTICK_GETBUTTON(0, 6) };
static size_t aActionLookJoyButtons[3]         = { DIK_INSERT, DIK_NUMPAD0, STDCONTROL_JOYSTICK_GETBUTTON(0, 7) };

static JonesConfigControlAction jonesConfig_aDefault2Axis8Button[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)           , aActionWalkFwdKeys            },
    { STD_ARRAYLEN(aActionWalkBackKeys)          , aActionWalkBackKeys           },
    { STD_ARRAYLEN(aActionTurnLeftKeys)          , aActionTurnLeftKeys           },
    { STD_ARRAYLEN(aActionTurnRightKeys)         , aActionTurnRightKeys          },
    { STD_ARRAYLEN(aActionStepLeftKeys)          , aActionStepLeftKeys           },
    { STD_ARRAYLEN(aActionStepRightKeys)         , aActionStepRightKeys          },
    { STD_ARRAYLEN(aActionCrawlJoyButtons)       , aActionCrawlJoyButtons        },
    { STD_ARRAYLEN(aActionRunKeys)               , aActionRunKeys                },
    { STD_ARRAYLEN(aActionRollJoyButtons)        , aActionRollJoyButtons         },
    { STD_ARRAYLEN(aActionJumpKeys)              , aActionJumpKeys               },
    { STD_ARRAYLEN(aActionLookJoyButtons)        , aActionLookJoyButtons         },
    { STD_ARRAYLEN(aActionActivateKeys)          , aActionActivateKeys           },
    { STD_ARRAYLEN(aActionToggleWeaponJoyButtons), aActionToggleWeaponJoyButtons },
    { STD_ARRAYLEN(aActionPrevWeaponKeys)        , aActionPrevWeaponKeys         },
    { STD_ARRAYLEN(aActionNextWeaponJoyButtons)  , aActionNextWeaponJoyButtons   },
    { STD_ARRAYLEN(aActionToggleLightKeys)       , aActionToggleLightKeys        },
    { STD_ARRAYLEN(aActionFistsKeys)             , aActionFistsKeys              },
    { STD_ARRAYLEN(aActionWhipKeys)              , aActionWhipKeys               },
    { STD_ARRAYLEN(aActionMauserKeys)            , aActionMauserKeys             },
    { STD_ARRAYLEN(aActionPPSH41Keys)            , aActionPPSH41Keys             },
    { STD_ARRAYLEN(aActionPistolKeys)            , aActionPistolKeys             },
    { STD_ARRAYLEN(aActionSimonovKeys)           , aActionSimonovKeys            },
    { STD_ARRAYLEN(aActionTokarevKeys)           , aActionTokarevKeys            },
    { STD_ARRAYLEN(aActionToz34Keys)             , aActionToz34Keys              },
    { STD_ARRAYLEN(aActionBazookaKeys)           , aActionBazookaKeys            },
    { STD_ARRAYLEN(aActionMacheteKeys)           , aActionMacheteKeys            },
    { STD_ARRAYLEN(aActionSatchelKeys)           , aActionSatchelKeys            },
    { STD_ARRAYLEN(aActionGrenadeKeys)           , aActionGrenadeKeys            },
    { STD_ARRAYLEN(aActionMapKeys)               , aActionMapKeys                },
    { STD_ARRAYLEN(aActionInterfaceJoyButtons)   , aActionInterfaceJoyButtons    },
    { STD_ARRAYLEN(aActionHealthKeys)            , aActionHealthKeys             },
    { STD_ARRAYLEN(aActionIMP1Keys)              , aActionIMP1Keys               },
    { STD_ARRAYLEN(aActionIMP2Keys)              , aActionIMP2Keys               },
    { STD_ARRAYLEN(aActionIMP3Keys)              , aActionIMP3Keys               },
    { STD_ARRAYLEN(aActionIMP4Keys)              , aActionIMP4Keys               },
    { STD_ARRAYLEN(aActionIMP5Keys)              , aActionIMP5Keys               },
    { STD_ARRAYLEN(aActionChalk)                 , aActionChalk                  }
};

// Side Winder Gamepad default keysets found in debug version

static size_t aActionToggleStepLeftSideWinderGamePadButtons[2]  = { DIK_LBRACKET, STDCONTROL_JOYSTICK_GETBUTTON(0, 6) };
static size_t aActionToggleStepRightSideWinderGamePadButtons[2] = { DIK_RBRACKET, STDCONTROL_JOYSTICK_GETBUTTON(0, 7) };
static size_t aActionToggleCrawlSideWinderGamePadButtons[2]     = { DIK_C       , STDCONTROL_JOYSTICK_GETBUTTON(0, 2) };
static size_t aActionToggleRunSideWinderGamePadButtons[2]       = { DIK_CAPITAL , STDCONTROL_JOYSTICK_GETBUTTON(0, 3) };
static size_t aActionLookSideWinderGamePadButtons[3]            = { DIK_INSERT  , DIK_NUMPAD0, STDCONTROL_JOYSTICK_GETBUTTON(0, 5) };
static size_t aActionToggleWeaponSideWinderGamePadButtons[2]    = { DIK_SPACE   , STDCONTROL_JOYSTICK_GETBUTTON(0, 9) };
static size_t aActionNextWeaponSideWinderGamePad[2]             = { DIK_PERIOD  , STDCONTROL_JOYSTICK_GETBUTTON(0, 4) };
static size_t aActionToggleInterfaceSideWinderGamePadButtons[2] = { DIK_I       , STDCONTROL_JOYSTICK_GETBUTTON(0, 8) };

static JonesConfigControlAction jonesConfig_aDefaultSideWinderGamepad[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys)                            , aActionWalkFwdKeys    },
    { STD_ARRAYLEN(aActionWalkBackKeys)                           , aActionWalkBackKeys  },
    { STD_ARRAYLEN(aActionTurnLeftKeys)                           , aActionTurnLeftKeys  },
    { STD_ARRAYLEN(aActionTurnRightKeys)                          , aActionTurnRightKeys },
    { STD_ARRAYLEN(aActionToggleStepLeftSideWinderGamePadButtons) , aActionToggleStepLeftSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionToggleStepRightSideWinderGamePadButtons), aActionToggleStepRightSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionToggleCrawlSideWinderGamePadButtons)    , aActionToggleCrawlSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionToggleRunSideWinderGamePadButtons)      , aActionToggleRunSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionRollKeys)                               , aActionRollKeys      },
    { STD_ARRAYLEN(aActionJumpKeys)                               , aActionJumpKeys      },
    { STD_ARRAYLEN(aActionLookSideWinderGamePadButtons)           , aActionLookSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionActivateKeys)                           , aActionActivateKeys  },
    { STD_ARRAYLEN(aActionToggleWeaponSideWinderGamePadButtons)   , aActionToggleWeaponSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionPrevWeaponKeys)                         , aActionPrevWeaponKeys},
    { STD_ARRAYLEN(aActionNextWeaponSideWinderGamePad)            , aActionNextWeaponSideWinderGamePad},
    { STD_ARRAYLEN(aActionToggleLightKeys)                        , aActionToggleLightKeys},
    { STD_ARRAYLEN(aActionFistsKeys)                              , aActionFistsKeys    },
    { STD_ARRAYLEN(aActionWhipKeys)                               , aActionWhipKeys     },
    { STD_ARRAYLEN(aActionMauserKeys)                             , aActionMauserKeys   },
    { STD_ARRAYLEN(aActionPPSH41Keys)                             , aActionPPSH41Keys   },
    { STD_ARRAYLEN(aActionPistolKeys)                             , aActionPistolKeys   },
    { STD_ARRAYLEN(aActionSimonovKeys)                            , aActionSimonovKeys  },
    { STD_ARRAYLEN(aActionTokarevKeys)                            , aActionTokarevKeys  },
    { STD_ARRAYLEN(aActionToz34Keys)                              , aActionToz34Keys    },
    { STD_ARRAYLEN(aActionBazookaKeys)                            , aActionBazookaKeys  },
    { STD_ARRAYLEN(aActionMacheteKeys)                            , aActionMacheteKeys  },
    { STD_ARRAYLEN(aActionSatchelKeys)                            , aActionSatchelKeys  },
    { STD_ARRAYLEN(aActionGrenadeKeys)                            , aActionGrenadeKeys  },
    { STD_ARRAYLEN(aActionMapKeys)                                , aActionMapKeys      },
    { STD_ARRAYLEN(aActionToggleInterfaceSideWinderGamePadButtons), aActionToggleInterfaceSideWinderGamePadButtons},
    { STD_ARRAYLEN(aActionHealthKeys)                             , aActionHealthKeys   },
    { STD_ARRAYLEN(aActionIMP1Keys)                               , aActionIMP1Keys     },
    { STD_ARRAYLEN(aActionIMP2Keys)                               , aActionIMP2Keys     },
    { STD_ARRAYLEN(aActionIMP3Keys)                               , aActionIMP3Keys     },
    { STD_ARRAYLEN(aActionIMP4Keys)                               , aActionIMP4Keys     },
    { STD_ARRAYLEN(aActionIMP5Keys)                               , aActionIMP5Keys     },
    { STD_ARRAYLEN(aActionChalk)                                  , aActionChalk        }
};

// A fifth default key set: retail keeps it (0x00511390) and its own arrays (0x005112E8..0x0051138F)
// in this position, but no code and no jonesConfig_aDfltKeySets entry references it. Placeholder
// names follow the project convention for unnamed retail data.

static size_t jonesConfig_aUnused_5112E8[2] = { DIK_LEFT, JONESCONFIG_JOYAXISKID_TWISTL };
static size_t jonesConfig_aUnused_5112F0[2] = { DIK_RIGHT, JONESCONFIG_JOYAXISKID_TWISTR };
static size_t jonesConfig_aUnused_5112F8[2] = { DIK_LBRACKET, JONESCONFIG_JOYAXISKID_LEFT };
static size_t jonesConfig_aUnused_511300[2] = { DIK_RBRACKET, JONESCONFIG_JOYAXISKID_RIGHT };
static size_t jonesConfig_aUnused_511308[2] = { DIK_C, STDCONTROL_JOYSTICK_GETBUTTON(0, 2) };
static size_t jonesConfig_aUnused_511310[3] = { DIK_LSHIFT, DIK_RSHIFT, STDCONTROL_JOYSTICK_GETBUTTON(0, 9) };
static size_t jonesConfig_aUnused_511320[2] = { DIK_CAPITAL, STDCONTROL_JOYSTICK_GETBUTTON(0, 3) };
static size_t jonesConfig_aUnused_511328[4] = { DIK_X, DIK_LMENU, DIK_RMENU, STDCONTROL_JOYSTICK_GETBUTTON(0, 1) };
static size_t jonesConfig_aUnused_511338[3] = { DIK_INSERT, DIK_NUMPAD0, STDCONTROL_JOYSTICK_GETBUTTON(0, 5) };
static size_t jonesConfig_aUnused_511348[3] = { DIK_LCONTROL, DIK_RCONTROL, STDCONTROL_JOYSTICK_GETBUTTON(0, 0) };
static size_t jonesConfig_aUnused_511358[2] = { DIK_SPACE, STDCONTROL_JOYSTICK_GETPOV(0, 0, 1) };
static size_t jonesConfig_aUnused_511360[2] = { DIK_COMMA, STDCONTROL_JOYSTICK_GETPOV(0, 0, 0) };
static size_t jonesConfig_aUnused_511368[2] = { DIK_PERIOD, STDCONTROL_JOYSTICK_GETPOV(0, 0, 2) };
static size_t jonesConfig_aUnused_511370[2] = { DIK_L, STDCONTROL_JOYSTICK_GETBUTTON(0, 6) };
static size_t jonesConfig_aUnused_511378[2] = { DIK_1, STDCONTROL_JOYSTICK_GETBUTTON(0, 4) };
static size_t jonesConfig_aUnused_511380[2] = { DIK_TAB, STDCONTROL_JOYSTICK_GETBUTTON(0, 7) };
static size_t jonesConfig_aUnused_511388[2] = { DIK_I, STDCONTROL_JOYSTICK_GETPOV(0, 0, 3) };

static JonesConfigControlAction jonesConfig_aUnused_511390[JONESCONTROL_ACTION_NUMACTIONS] =
{
    { STD_ARRAYLEN(aActionWalkFwdKeys), aActionWalkFwdKeys },
    { STD_ARRAYLEN(aActionWalkBackKeys), aActionWalkBackKeys },
    { STD_ARRAYLEN(jonesConfig_aUnused_5112E8), jonesConfig_aUnused_5112E8 },
    { STD_ARRAYLEN(jonesConfig_aUnused_5112F0), jonesConfig_aUnused_5112F0 },
    { STD_ARRAYLEN(jonesConfig_aUnused_5112F8), jonesConfig_aUnused_5112F8 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511300), jonesConfig_aUnused_511300 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511308), jonesConfig_aUnused_511308 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511310), jonesConfig_aUnused_511310 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511320), jonesConfig_aUnused_511320 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511328), jonesConfig_aUnused_511328 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511338), jonesConfig_aUnused_511338 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511348), jonesConfig_aUnused_511348 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511358), jonesConfig_aUnused_511358 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511360), jonesConfig_aUnused_511360 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511368), jonesConfig_aUnused_511368 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511370), jonesConfig_aUnused_511370 },
    { STD_ARRAYLEN(aActionFistsKeys), aActionFistsKeys },
    { STD_ARRAYLEN(jonesConfig_aUnused_511378), jonesConfig_aUnused_511378 },
    { STD_ARRAYLEN(aActionMauserKeys), aActionMauserKeys },
    { STD_ARRAYLEN(aActionPPSH41Keys), aActionPPSH41Keys },
    { STD_ARRAYLEN(aActionPistolKeys), aActionPistolKeys },
    { STD_ARRAYLEN(aActionSimonovKeys), aActionSimonovKeys },
    { STD_ARRAYLEN(aActionTokarevKeys), aActionTokarevKeys },
    { STD_ARRAYLEN(aActionToz34Keys), aActionToz34Keys },
    { STD_ARRAYLEN(aActionBazookaKeys), aActionBazookaKeys },
    { STD_ARRAYLEN(aActionMacheteKeys), aActionMacheteKeys },
    { STD_ARRAYLEN(aActionSatchelKeys), aActionSatchelKeys },
    { STD_ARRAYLEN(aActionGrenadeKeys), aActionGrenadeKeys },
    { STD_ARRAYLEN(jonesConfig_aUnused_511380), jonesConfig_aUnused_511380 },
    { STD_ARRAYLEN(jonesConfig_aUnused_511388), jonesConfig_aUnused_511388 },
    { STD_ARRAYLEN(aActionHealthKeys), aActionHealthKeys },
    { STD_ARRAYLEN(aActionIMP1Keys), aActionIMP1Keys },
    { STD_ARRAYLEN(aActionIMP2Keys), aActionIMP2Keys },
    { STD_ARRAYLEN(aActionIMP3Keys), aActionIMP3Keys },
    { STD_ARRAYLEN(aActionIMP4Keys), aActionIMP4Keys },
    { STD_ARRAYLEN(aActionIMP5Keys), aActionIMP5Keys },
    { STD_ARRAYLEN(aActionChalk), aActionChalk }
};

// Default key sets
static const JonesConfigControlAction* jonesConfig_aDfltKeySets[] = {
    jonesConfig_aDefaultControlSet,
    jonesConfig_aDfault2Axis4Button,
    jonesConfig_aDefault2Axis8Button
};

#define JONESCONFIG_NUMDEFAULTCONTROLSCHEMES STD_ARRAYLEN(jonesConfig_aDfltKeySets)

static const char* jonesConfig_aDfltKeySetNames[JONESCONFIG_NUMDEFAULTCONTROLSCHEMES] = {
    "JONES_STR_DFLTKEY",
    "JONES_STR_DFLT2A4B",
    "JONES_STR_DFLT2A8B"
};




// Key control sting names
static const char* jonesConfig_aControlKeyStrings[223] =
{
    NULL,
    "JONES_STR_ESCAPE",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "JONES_STR_MINUS",
    "JONES_STR_EQUAL",
    "JONES_STR_BACK",
    "JONES_STR_TAB",
    "Q",
    "W",
    "E",
    "R",
    "T",
    "Y",
    "U",
    "I",
    "O",
    "P",
    "JONES_STR_LEFT_BRCKT",
    "JONES_STR_RIGHT_BRCKT",
    "JONES_STR_ENTER",
    "JONES_STR_LEFT_CTRL",
    "A",
    "S",
    "D",
    "F",
    "G",
    "H",
    "J",
    "K",
    "L",
    ";",
    "'",
    "`",
    "JONES_STR_LEFT_SHIFT",
    "\\",
    "Z",
    "X",
    "C",
    "V",
    "B",
    "N",
    "M",
    "JONES_STR_COMMA",
    ".",
    "/",
    "JONES_STR_RIGHT_SHIFT",
    "JONES_STR_NUMPAD_MULT",
    "JONES_STR_LEFT_ALT",
    "JONES_STR_SPACE",
    "JONES_STR_CAPS",
    "JONES_STR_F1",
    "JONES_STR_F2",
    "JONES_STR_F3",
    "JONES_STR_F4",
    "JONES_STR_F5",
    "JONES_STR_F6",
    "JONES_STR_F7",
    "JONES_STR_F8",
    "JONES_STR_F9",
    "JONES_STR_F10",
    "JONES_STR_NUMLOCK",
    "JONES_STR_SCROLL",
    "JONES_STR_NUMPAD_7",
    "JONES_STR_NUMPAD_8",
    "JONES_STR_NUMPAD_9",
    "JONES_STR_NUMPAD_MINUS",
    "JONES_STR_NUMPAD_4",
    "JONES_STR_NUMPAD_5",
    "JONES_STR_NUMPAD_6",
    "JONES_STR_NUMPAD_PLUS",
    "JONES_STR_NUMPAD_1",
    "JONES_STR_NUMPAD_2",
    "JONES_STR_NUMPAD_3",
    "JONES_STR_NUMPAD_0",
    "JONES_STR_NUMPAD_DOT",
    NULL,
    NULL,
    NULL,
    "JONES_STR_F11",
    "JONES_STR_F12",
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
    "JONES_STR_F13",
    "JONES_STR_F14",
    "JONES_STR_F15",
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
    NULL,
    NULL,
    "JONES_STR_NUMPAD_EQL",
    NULL,
    NULL,
    NULL,
    "@",
    ":",
    "__",
    NULL,
    "STOP",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JONES_STR_NUMPAD_ENTER",
    "JONES_STR_RIGHT_CTRL",
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
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JONES_STR_NUMPAD_COMMA",
    NULL,
    "JONES_STR_NUMPAD_DIV",
    NULL,
    "JONES_STR_SYSRQ",
    "JONES_STR_RIGHT_ALT",
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
    NULL,
    NULL,
    "JONES_STR_HOME",
    "JONES_STR_UPARRW",
    "JONES_STR_PAGEUP",
    NULL,
    "JONES_STR_LEFTARRW",
    NULL,
    "JONES_STR_RGHTARRW",
    NULL,
    "JONES_STR_END",
    "JONES_STR_DNARRW",
    "JONES_STR_PGDN",
    "JONES_STR_INSERT",
    "JONES_STR_DELETE",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "JONES_STR_LEFT_WND",
    "JONES_STR_RIGHT_WND",
    "JONES_STR_APPS",
    NULL
};

static JonesDialogSize jonesConfig_aDialogSizes[21] =
{
    { 164, 368, 232 },
    { 112, 260, 224 },
    { 111, 408, 218 },
    { 116, 297, 95 },
    { 115, 549, 367 },
    { 117, 309, 103 },
    { 120, 282, 152 },
    { 114, 238, 285 },
    { 148, 440, 393 },
    { 113, 351, 206 },
    { 211, 266, 103 },
    { 163, 282, 90 },
    { 214, 282, 90 },
    { 121, 282, 90 },
    { 150, 249, 100 },
    { 233, 344, 180 },
    { 190, 538, 297 },
    { 212, 280, 74 },
    { 159, 583, 250 },
    { 154, 579, 252 },
    { 167, 279, 75 }
};

static int jonesConfig_displaySettings_bUpdateSliderText = 1;
static Std3DMipmapFilterType jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_BILINEAR;
static int jonesConfig_advanceDisplaySettings_perfLevel = 4;
static int jonesConfig_prevLevelNum = -1;


int J3DAPI jonesConfig_InitKeySetsPath(void);
int J3DAPI jonesConfig_InitKeyActions(void);

void J3DAPI jonesConfig_ControlToString(size_t controlId, char* pDest);

JonesControlsScheme* J3DAPI jonesConfig_LoadActiveControlScheme(void);
int J3DAPI jonesConfig_LoadControlScheme(const char* pFilePath, JonesControlsScheme* pConfig);
int J3DAPI jonesConfig_ParseArgumentIndex(size_t* argNum, const StdConffileEntry* pEntry, size_t a3, const char** apStringList, size_t size);
int J3DAPI jonesConfig_GetStringListIndex(char* pValue, const char** apStringList, int size);
int J3DAPI jonesConfig_ParseJoystickControlId(size_t* pArgNum, const StdConffileEntry* pEntry);
int J3DAPI jonesConfig_SetDefaultControlScheme(JonesControlsScheme* pScheme, size_t num);
void J3DAPI jonesConfig_BindControls(JonesControlsScheme* pConfig);
void J3DAPI jonesConfig_BindJoystickControl(SithControlFunction functionId, size_t controlId);
void J3DAPI jonesConfig_FreeControlScheme(JonesControlsScheme* pScheme);
void J3DAPI jonesConfig_FreeControlConfigEntry(JonesControlsConfig* pConfig);

HFONT J3DAPI jonesConfig_InitDialog(HWND hWnd, HFONT hFont, int dlgID);
HFONT J3DAPI jonesConfig_CreateDialogFont(HWND hWnd, int bWindowMode, int dlgResNum, float* pFontScale);
void J3DAPI jonesConfig_ResetDialogFont(HWND hWndParent, HFONT hFont);
BOOL CALLBACK jonesConfig_ResetWindowFontCallback(HWND hWnd, LPARAM lparam);
BOOL CALLBACK jonesConfig_SetPositionAndTextCallback(HWND hCtrl, LPARAM lparam);
void J3DAPI jonesConfig_SetWindowFontAndPosition(HWND hCtrl, JonesDialogFontInfo* pFontInfo);
void J3DAPI jonesConfig_GetWindowScreenRect(HWND hWnd, LPRECT lpRect);
void J3DAPI jonesConfig_SetDialogTitleAndPosition(HWND hWnd, JonesDialogFontInfo* pDlgFontInfo);

UINT CALLBACK jonesConfig_SaveGameDialogHookProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_SaveGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME lpOfn);
LRESULT CALLBACK jonesConfig_SaveGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_ShowOverwriteSaveGameDlg(HWND hWnd, const char* aFilePath);
BOOL CALLBACK jonesConfig_SaveGameMsgBoxProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_GameSaveSetData(HWND hDlg, int a2, LPARAM lParam);

void* J3DAPI jonesConfig_GameFileDialog_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode);
UINT CALLBACK jonesConfig_LoadGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_ShowLoadGameWarningMsgBox(HWND hWnd);
BOOL J3DAPI jonesConfig_LoadGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME pofn);
LRESULT CALLBACK jonesConfig_LoadGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


int J3DAPI jonesConfig_ShowEditControlShemeDialog(HWND hWnd, JonesEditControlSchemeDialogData* pData);
BOOL CALLBACK jonesConfig_EditControlSchemeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitEditControlSchemeDlg(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_EditControlScheme_SetListColumn(HWND hWnd, const char* pJSText);
void J3DAPI jonesConfig_EditControlScheme_SetListScheme(HWND hWnd, int a2, JonesControlsScheme* pScheme);
void J3DAPI jonesConfig_EditControlScheme_Handle_WM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode);
void J3DAPI jonesConfig_EditControlScheme_EditAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme);
int J3DAPI jonesConfig_EditControlScheme_GetSelectedListItemBinding(HWND hDlg, HWND hListCtrl, JonesControlsScheme* pScheme, size_t* pItemIdx, size_t* pBindIdx);
void J3DAPI jonesConfig_EditControlScheme_AddAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme);
void J3DAPI jonesConfig_EditControlScheme_DeleteAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme);


int J3DAPI jonesConfig_ShowReassignKeyMsgBox(HWND hWnd, JonesReAssignKeyDialogData* pData);
BOOL CALLBACK jonesConfig_ReassignKeyDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitReassignKeyDialog(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_ReassignKeyDlg_HandleWM_COMMAND(HWND hWnd, unsigned int cntrlId, LPARAM lParam, unsigned int notifyCode);

BOOL CALLBACK jonesConfig_MessageDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void J3DAPI jonesConfig_InitMessageDialogIcon(HWND hWnd, JonesMessageDialogData* pData);
int J3DAPI jonesConfig_InitMessageDialogData(HWND hDlg, int a2, LPARAM lParam);
int J3DAPI jonesConfig_SetTextControl(HWND hDlg, HWND hWnd, const char* aText);
void J3DAPI jonesConfig_MessageDialog_HandleWM_COMMAND(HWND hDlg, int nResult, LPARAM lParam, int notifyCode);

void J3DAPI jonesConfig_MsgBoxDlg_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode);

BOOL CALLBACK jonesConfig_GamePlayOptionsProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
void J3DAPI jonesConfig_HandleWM_HSCROLL(HWND hDlg, HWND hWnd, UINT sbValue, int nPos);
int J3DAPI jonesConfig_GamePlayOptionsInitDlg(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_GamePlayOptions_HandleWM_COMMAND(HWND hDlg, int controlID, LPARAM lParam, int notifyCode);

int J3DAPI jonesConfig_CopyControlConfig(JonesControlsConfig* pSrc, JonesControlsConfig* pDst);
JonesControlsScheme* J3DAPI jonesConfig_CloneControlSchemesList(JonesControlsScheme* aSchemes, size_t start, size_t numSchemes);

int J3DAPI jonesConfig_ControlOptions_InitControlsConfig(void);
static int J3DAPI jonesConfig_ControlOptions_LoadAllControlSchemes(JonesControlsConfig* pConfig);
static int J3DAPI jonesConfig_ControlOptions_SetAllDefaultControlSchemes(JonesControlsConfig* pConfig);
BOOL CALLBACK jonesConfig_ControlOptionsDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_InitControlOptionsDialog(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_ControlOptions_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, unsigned int notificationCode);
static int J3DAPI jonesConfig_ControlOptions_WriteScheme(JonesControlsScheme* pScheme);
int J3DAPI jonesConfig_ControlOptions_UpdateButtons(HWND hDlg);
void J3DAPI jonesConfig_ControlOptions_DeleteScheme(HWND hDlg, JonesControlsConfig* pConfig);
void J3DAPI jonesConfig_ControlOptions_EditSelectedScheme(HWND hDlg, JonesControlsConfig* a2);
JonesControlsScheme* J3DAPI jonesConfig_GetDefaultControlScheme(JonesControlsConfig* pConfig);
void J3DAPI jonesConfig_ControlOptions_CreateNewScheme(HWND hDlg, JonesControlsConfig* pConfig, JonesControlsScheme* pNewScheme);


int J3DAPI jonesConfig_ShowAdvanceDisplaySettings(HWND hWnd, JonesDisplaySettingsDialogData* pData);
BOOL CALLBACK jonesConfig_AdvanceDisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP(const StdDisplayInfo* pDisplayInfo, const JonesDisplaySettings* pSettings, int bpp);
int J3DAPI jonesConfig_InitAdvanceDisplaySettingsDialog(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND(HWND hDlg, int ctrlID, int a3, int notifyCode);


BOOL CALLBACK jonesConfig_GameOverDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void J3DAPI jonesConfig_LoadGameGetLastSavedGamePath(char* pPath, unsigned int size);
int J3DAPI jonesConfig_GameOverDialogInit(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_GameOverDialog_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode);

BOOL CALLBACK jonesConfig_StatisticsDialogProc(HWND hwnd, UINT uMsg, WPARAM wPAram, LPARAM lParam);
void J3DAPI jonesConfig_StatisticProc_HandleWM_HSCROLL(int a1, HWND hWnd, UINT sbCode, int nThumbPos);
int J3DAPI jonesConfig_DrawImageOnDialogItem(HWND hDlg, HDC hdcWnd, HDC hdcCtrl, int nIDDlgItem, HBITMAP hImage, HBITMAP hMask);
int J3DAPI jonesConfig_SetStatisticsDialogForLevel(HWND hDlg, int levelNum, int* a3);
void J3DAPI jonesConfig_DrawStatisticDialogIQPoints(HWND hwnd, JonesDialogImageInfo** ppImageInfo, int dlgID, int iqpoints);
int J3DAPI jonesConfig_InitStatisticDialog(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_StatisticProc_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode);



BOOL CALLBACK jonesConfig_DialogInsertCDProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitDialogInsertCD(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_InsertCD_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode);

BOOL CALLBACK jonesConfig_ExitGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam);

int J3DAPI jonesConfig_ShowAssignKeyDialog(HWND hWnd, JonesAssignKeyDialogData* pData);
int J3DAPI jonesConfig_AssignKey_CheckBindForKey(JonesControlsScheme* pScheme, size_t keyControlID, size_t* pFunctionId, int listID, size_t* pOffset);
BOOL CALLBACK jonesConfig_AssignKeyDlgProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_InitAssignKeyDlg(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_AssignControlKey_ReadKey(HWND hWnd, WPARAM wParam);
int J3DAPI jonesConfig_AssignControlKey_ReadJoyAxisKey(JonesAssignKeyDialogData* pData);
void J3DAPI jonesConfig_AssignKeyDlg_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode);

int J3DAPI jonesConfig_Startup(void)
{
    JonesControlsScheme* pCfg;
    int i;

    if ( jonesConfig_bStartup )
    {
        return 1;
    }

    for ( i = 0; i < (int)STD_ARRAYLEN(jonesConfig_apDlgIcons); ++i )
    {
        char aIconPath[128];
        stdFnames_MakePath(aIconPath, STD_ARRAYLEN(aIconPath), "misc", jonesConfig_apDialogIconFiles[i]);

        jonesConfig_apDlgIcons[i] = stdBmp_Load(aIconPath);
        if ( !jonesConfig_apDlgIcons[i] )
        {
            int j;

            for ( j = 0; j < i; ++j )
            {
                DeleteObject(jonesConfig_apDlgIcons[j]);
            }

#line 455
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't load bitmap %s.\n", aIconPath, 0, 0, 0);
            return 0;
        }
    }

    if ( !jonesConfig_InitKeySetsPath() )
    {
        return 0;
    }

    if ( !jonesConfig_InitKeyActions() )
    {
        return 0;
    }

    memset(jonesConfig_aGlyphMetrics, 0, sizeof(jonesConfig_aGlyphMetrics));
    memset(&jonesConfig_textMetric, 0, sizeof(jonesConfig_textMetric));

    jonesConfig_bTextMetricInited = 0;

    jonesConfig_gamePlayOptions_bDefaultRun = wuRegistry_GetIntEx("Default Run", 0);

    if ( jonesConfig_gamePlayOptions_bDefaultRun )
    {
        sithControl_controlOptions |= 2u; // run flag
    }
    else
    {
        sithControl_controlOptions &= ~2;
    }

    // Init cur level num
    jonesConfig_gameStatistics_curLevelNum = 0;

    pCfg = jonesConfig_LoadActiveControlScheme();
    if ( pCfg )
    {
        jonesConfig_BindControls(pCfg);
        jonesConfig_FreeControlScheme(pCfg);
    }

    jonesConfig_bStartup = 1;
    return 1;
}

int J3DAPI jonesConfig_InitKeySetsPath(void)
{
    char aCurInstallPath[128];
    char* pKeySets;
    wchar_t* pPwKeySets = NULL;
    size_t len;

    pPwKeySets = sithString_GetString("SITHSTRING_KEYSETS");
    if ( pPwKeySets )
    {
        pKeySets = stdUtil_ToAString(pPwKeySets);

        memset(aCurInstallPath, 0, sizeof(aCurInstallPath));

        wuRegistry_GetStr("Install Path", aCurInstallPath, STD_ARRAYLEN(aCurInstallPath), "");

        if ( !pKeySets )
        {
#line 510
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't find string %s in lookup table.\n", pKeySets, 0, 0, 0);
            stdMemory_Free(pPwKeySets); // pwKeySets is null value but is allocated on heap in sithString_GetString
            return 0;
        }

        len = strlen(aCurInstallPath);
        if ( len == 0 )
        {
            sprintf(jonesConfig_aKeySetDirectory, "..\\\\%s", pKeySets);
        }
        else
        {
            if ( aCurInstallPath[len - 1] != '\\' )
            {
                sprintf(jonesConfig_aKeySetDirectory, "%s\\%s", aCurInstallPath, pKeySets);
            }
            else
            {
                sprintf(jonesConfig_aKeySetDirectory, "%s%s", aCurInstallPath, pKeySets);
            }
        }

        stdMemory_Free(pKeySets);
    }
    else
    {
        return 0;
    }

    return 1;
}

int J3DAPI jonesConfig_InitKeyActions(void)
{
    char aActionName[512];
    int keyNum;
    int i;

    jonesConfig_maxActionWords  = 0;
    jonesConfig_largestActionStrLen = 0;

    for ( i = 0; i < (int)STD_ARRAYLEN(jonesConfig_aJonesCapControlActionNames); ++i )
    {
        int len;
        char* pCurActionWord;

        const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[i]);
        if ( !pActionName )
        {
            return 0;
        }

        strcpy(aActionName, pActionName);

        len = 0;
        pCurActionWord = aActionName;
        while ( pCurActionWord )
        {
            pCurActionWord = strchr(pCurActionWord, ' ');
            if ( pCurActionWord )
            {
                ++pCurActionWord;
            }

            ++len;
        }

        if ( len > jonesConfig_maxActionWords )
        {
            jonesConfig_maxActionWords = len;
        }

        len = strlen(aActionName);
        if ( len > jonesConfig_largestActionStrLen )
        {
            jonesConfig_largestActionStrLen = len;
        }
    }

    for ( keyNum = 0; keyNum < STDCONTROL_MAX_KEYID + JONESCONFIG_NUM_JOYAXISKIDS + 4; ++keyNum ) // 656 =  STDCONTROL_MAX_KEYID + JONESCONFIG_NUM_JOYAXISKIDS + 4
    {
        char aControlStr[JONESCONFIG_CONTROLSTRING_MAXLEN];
        char* pCurControlWord;
        int length;

        memset(aControlStr, 0, sizeof(aControlStr));

        switch ( keyNum - STDCONTROL_MAX_KEYID )
        {
            case 0:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_UP, aControlStr);
                break;

            case 1:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_DOWN, aControlStr);
                break;

            case 2:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_LEFT, aControlStr);
                break;

            case 3:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_RIGHT, aControlStr);
                break;

            case 4:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_AIR, aControlStr);
                break;

            case 5:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_GROUND, aControlStr);
                break;

            case 6:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_TWISTL, aControlStr);
                break;

            case 7:
                jonesConfig_ControlToString(JONESCONFIG_JOYAXISKID_TWISTR, aControlStr);
                break;

            default:
                jonesConfig_ControlToString(keyNum, aControlStr);
                break;
        }

        length = 0;
        pCurControlWord = aControlStr;
        while ( pCurControlWord )
        {
            pCurControlWord = strchr(pCurControlWord, ' ');
            if ( pCurControlWord )
            {
                ++pCurControlWord;
            }

            ++length;
        }

        if ( length > jonesConfig_maxControlKeyWordCount )
        {
            jonesConfig_maxControlKeyWordCount = length;
        }

        length = strlen(aControlStr);
        if ( length > jonesConfig_maxControlLen )
        {
            jonesConfig_maxControlLen = length;
        }
    }

    jonesConfig_maxActionPaddedStrLen  = jonesConfig_largestActionStrLen + (5 - jonesConfig_largestActionStrLen % 5);
    jonesConfig_maxControlPaddedStrLen = jonesConfig_maxControlLen + (5 - jonesConfig_maxControlLen % 5);
    return 1;
}

void J3DAPI jonesConfig_Shutdown(void)
{
    int i;

    jonesConfig_FreeControlConfigEntry(&jonesConfig_controlOptions_curControlConfig);

    for ( i = 0; i < 2; i++ ) // TODO: Bug? Why only 2?
    {
        DeleteObject(jonesConfig_apDlgIcons[i]);
        jonesConfig_apDlgIcons[i] = NULL;
    }

    if ( jonesConfig_aKeySetDirectory ) // TODO: [BUG] tests the array address, which is always non-null
    {
        memset(jonesConfig_aKeySetDirectory, 0, sizeof(jonesConfig_aKeySetDirectory));
    }
}

static int J3DAPI jonesConfig_ControlOptions_WriteScheme(JonesControlsScheme* pScheme)
{
    char* pPadding;
    int i;
    size_t numPad;
    FILE* pFile;
    const char* pJonesString;
    char* pTitleText;
    char aCurDir[128];
    char aKfgPath[128];

    // Change cur dir to control config dir
    if ( !SetCurrentDirectory(jonesConfig_aKeySetDirectory) )
    {
        SECURITY_ATTRIBUTES secattr;
        secattr.nLength              = sizeof(SECURITY_ATTRIBUTES);
        secattr.lpSecurityDescriptor = NULL;
        secattr.bInheritHandle       = 1;
        CreateDirectory(jonesConfig_aKeySetDirectory, &secattr);

        memset(aCurDir, 0, sizeof(aCurDir));
        GetCurrentDirectory(STD_ARRAYLEN(aCurDir), aCurDir);
        SetCurrentDirectory(jonesConfig_aKeySetDirectory);
    }

#line 702
    pPadding = (char*)STDMALLOC(max(jonesConfig_maxActionPaddedStrLen, jonesConfig_maxControlPaddedStrLen));
    if ( !pPadding )
    {
        return 0;
    }
    memset(pPadding, ' ', max(jonesConfig_maxActionPaddedStrLen, jonesConfig_maxControlPaddedStrLen));
    pTitleText = (char*)STDMALLOC(jonesConfig_largestActionStrLen + jonesConfig_maxControlLen);
    if ( !pTitleText )
    {
        stdMemory_Free(pPadding);
        return 0;
    }

    memset(pTitleText, 0, jonesConfig_largestActionStrLen + jonesConfig_maxControlLen);

    memset(aKfgPath, 0, sizeof(aKfgPath));
    sprintf(aKfgPath, "%s%s", pScheme->aName, ".kfg");

    pFile = fopen(aKfgPath, "wt+");
    if ( !pFile )
    {
        SetCurrentDirectory(aCurDir);
        stdMemory_Free(pTitleText);
        stdMemory_Free(pPadding);
        return 0;
    }

    // Write heading text
    pJonesString = jonesString_GetString("JONES_STR_HEADING");
    if ( pJonesString )
    {
        char aEnabledLine[12];

        fprintf(pFile, pJonesString);
        fprintf(pFile, "\n");

        // Write enabled line
        sprintf(aEnabledLine, "%s\n\n", "1");
        fprintf(pFile, aEnabledLine);
    }
    else
    {
        goto error;
    }

    // Write column text "Actions :"
    memset(pTitleText, 0, jonesConfig_largestActionStrLen + jonesConfig_maxControlLen);

    pJonesString = jonesString_GetString("JONES_STR_ACTIONS");
    if ( pJonesString )
    {
        strcpy(pTitleText, pJonesString);

        numPad = jonesConfig_maxActionPaddedStrLen - strlen(pTitleText);
        memset(pPadding, 0, max(jonesConfig_maxActionPaddedStrLen, jonesConfig_maxControlPaddedStrLen));
        memset(pPadding, ' ', numPad);
        strcat(pTitleText, pPadding);

        // Write column text "Keys:"
        pJonesString = jonesString_GetString("JONES_STR_KEYS");
        if ( pJonesString )
        {
            // Write column header line
            strcat(pTitleText, pJonesString);
            fprintf(pFile, pTitleText);
            fprintf(pFile, "\n\n");
        }
        else
        {
            goto error;
        }
    }
    else
    {
        goto error;
    }

    // Now go over actions and write the key config per action
    for ( i = 0; i < (int)STD_ARRAYLEN(pScheme->aActions); ++i )
    {
        int (*pBindings)[9];
        int j;
        char aActionControlLine[256];

        pBindings = (int (*)[9])pScheme->aActions[i];

        // Write action name
        pJonesString = jonesString_GetString(jonesConfig_aJonesControlActionNames[i]);
        if ( pBindings && pJonesString )
        {
            memset(aActionControlLine, 0, sizeof(aActionControlLine));
            _snprintf(aActionControlLine, STD_ARRAYLEN(aActionControlLine), "\"%s\"", pJonesString);
            fprintf(pFile, aActionControlLine);

            // Write padding
            numPad = jonesConfig_maxActionPaddedStrLen - strlen(aActionControlLine);
            memset(pPadding, 0, max(jonesConfig_maxActionPaddedStrLen, jonesConfig_maxControlPaddedStrLen));
            memset(pPadding, ' ', numPad);
            fprintf(pFile, pPadding);

            // Now go over action's bindings and write them to file
            for ( j = 1; j <= JONESCONTROL_ACTION_GETNUMBINDS(*pBindings); ++j )
            {
                char aControlText[JONESCONFIG_CONTROLSTRING_MAXLEN];
                memset(aControlText, 0, sizeof(aControlText));

                jonesConfig_ControlToString((*pBindings)[j], aControlText);

                _snprintf(aActionControlLine, STD_ARRAYLEN(aActionControlLine), "\"%s\"", aControlText);
                fprintf(pFile, aActionControlLine);

                // Write padding
                numPad = jonesConfig_maxControlPaddedStrLen - strlen(aControlText);
                memset(pPadding, 0, max(jonesConfig_maxActionPaddedStrLen, jonesConfig_maxControlPaddedStrLen));
                memset(pPadding, ' ', numPad);
                fprintf(pFile, pPadding);
            }

            fprintf(pFile, "\n");
        }
        else
        {
            goto error;
        }
    }

    // Close file and set cur dir to previous dir
    fclose(pFile);
    SetCurrentDirectory(aCurDir);

    stdMemory_Free(pPadding);
    stdMemory_Free(pTitleText);
    return 1;

error:
    if ( pFile )
    {
        fclose(pFile);
    }

    SetCurrentDirectory(aCurDir);
    stdMemory_Free(pPadding);
    stdMemory_Free(pTitleText);
    return 0;
}

static int J3DAPI jonesConfig_ControlOptions_LoadAllControlSchemes(JonesControlsConfig* pConfig)
{
    WIN32_FIND_DATAA fileData;
    HANDLE hFile;
    int next;
    JonesControlsScheme* pScheme;

    char aCurDir[128];
    memset(aCurDir, 0, sizeof(aCurDir));

    GetCurrentDirectory(STD_ARRAYLEN(aCurDir), aCurDir);
    if ( !SetCurrentDirectory(jonesConfig_aKeySetDirectory) )
    {
        return 1;
    }

    hFile = FindFirstFile("*.kfg", &fileData);
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        next = 1;
        while ( next )
        {
            if ( (pConfig->numSchemes % 10) == 0 )
            {
#line 870
                pConfig->aSchemes = (JonesControlsScheme*)STDREALLOC(pConfig->aSchemes, sizeof(JonesControlsScheme) * (pConfig->numSchemes + 10));
            }

            if ( !pConfig->aSchemes )
            {
                SetCurrentDirectory(aCurDir);
                return 0;
            }

            pScheme = &pConfig->aSchemes[pConfig->numSchemes];
            if ( jonesConfig_LoadControlScheme(fileData.cFileName, pScheme) )
            {
                ++pConfig->numSchemes;
            }

            next = FindNextFile(hFile, &fileData);
        }

        FindClose(hFile);
    }

    SetCurrentDirectory(aCurDir);
    return 1;
}

void J3DAPI jonesConfig_ControlToString(size_t controlId, char* pDest)
{
    const char* pControlStr = NULL;
    int povCount;
    int buttonNumber;
    char* pJonesString = NULL;
    char aKeyName[256];

    if ( !pDest )
    {
        // TODO: maybe log?
        return;
    }

    memset(pDest, 0, JONESCONFIG_CONTROLSTRING_MAXLEN);

    switch ( controlId )
    {
            case DIK_ESCAPE:
                pJonesString = "JONES_STR_ESCAPE";
                break;

            case DIK_MINUS:
                pJonesString = "JONES_STR_MINUS";
                break;

            case DIK_EQUALS:
                pJonesString = "JONES_STR_EQUAL";
                break;

            case DIK_LBRACKET:
                pJonesString = "JONES_STR_LEFT_BRCKT";
                break;

            case DIK_RBRACKET:
                pJonesString = "JONES_STR_RIGHT_BRCKT";
                break;

            case DIK_RETURN:
                pJonesString = "JONES_STR_ENTER";
                break;

            case DIK_SEMICOLON:
            case DIK_APOSTROPHE:
            case DIK_GRAVE:
            case DIK_BACKSLASH:
            case DIK_PERIOD:
            case DIK_SLASH:
            case DIK_1:
            case DIK_2:
            case DIK_3:
            case DIK_4:
            case DIK_5:
            case DIK_6:
            case DIK_7:
            case DIK_8:
            case DIK_9:
            case DIK_0:
            case DIK_A:
            case DIK_B:
            case DIK_C:
            case DIK_D:
            case DIK_E:
            case DIK_F:
            case DIK_G:
            case DIK_H:
            case DIK_I:
            case DIK_J:
            case DIK_K:
            case DIK_L:
            case DIK_M:
            case DIK_N:
            case DIK_O:
            case DIK_P:
            case DIK_Q:
            case DIK_R:
            case DIK_S:
            case DIK_T:
            case DIK_U:
            case DIK_V:
            case DIK_W:
            case DIK_X:
            case DIK_Y:
            case DIK_Z:
                GetKeyNameText(((controlId & 0x7F) | ((controlId & 0x80) << 1)) << 16, aKeyName, sizeof(aKeyName) - 1);
                strcpy(pDest, aKeyName);
                _strupr(pDest);
                return;

            case DIK_COMMA:
                pJonesString = "JONES_STR_COMMA";
                break;

            case DIK_SPACE:
                pJonesString = "JONES_STR_SPACE";
                break;

            case DIK_LWIN:
                pJonesString = "JONES_STR_LEFT_WND";
                break;

            case DIK_RWIN:
                pJonesString = "JONES_STR_RIGHT_WND";
                break;

            case DIK_CAPITAL:
                pJonesString = "JONES_STR_CAPS";
                break;

            case DIK_LMENU:
                pJonesString = "JONES_STR_LEFT_ALT";
                break;

            case DIK_RMENU:
                pJonesString = "JONES_STR_RIGHT_ALT";
                break;

            case DIK_LSHIFT:
                pJonesString = "JONES_STR_LEFT_SHIFT";
                break;

            case DIK_RSHIFT:
                pJonesString = "JONES_STR_RIGHT_SHIFT";
                break;

            case DIK_TAB:
                pJonesString = "JONES_STR_TAB";
                break;

            case DIK_LCONTROL:
                pJonesString = "JONES_STR_LEFT_CTRL";
                break;

            case DIK_RCONTROL:
                pJonesString = "JONES_STR_RIGHT_CTRL";
                break;

            case DIK_BACK:
                pJonesString = "JONES_STR_BACK";
                break;

            case DIK_INSERT:
                pJonesString = "JONES_STR_INSERT";
                break;

            case DIK_DELETE:
                pJonesString = "JONES_STR_DELETE";
                break;

            case DIK_HOME:
                pJonesString = "JONES_STR_HOME";
                break;

            case DIK_END:
                pJonesString = "JONES_STR_END";
                break;

            case DIK_PRIOR:
                pJonesString = "JONES_STR_PAGEUP";
                break;

            case DIK_NEXT:
                pJonesString = "JONES_STR_PGDN";
                break;

            case DIK_UP:
                pJonesString = "JONES_STR_UPARRW";
                break;

            case DIK_DOWN:
                pJonesString = "JONES_STR_DNARRW";
                break;

            case DIK_LEFT:
                pJonesString = "JONES_STR_LEFTARRW";
                break;

            case DIK_RIGHT:
                pJonesString = "JONES_STR_RGHTARRW";
                break;

            case DIK_NUMLOCK:
                pJonesString = "JONES_STR_NUMLOCK";
                break;

            case DIK_NUMPAD1:
                pJonesString = "JONES_STR_NUMPAD_1";
                break;

            case DIK_NUMPAD2:
                pJonesString = "JONES_STR_NUMPAD_2";
                break;

            case DIK_NUMPAD3:
                pJonesString = "JONES_STR_NUMPAD_3";
                break;

            case DIK_NUMPAD4:
                pJonesString = "JONES_STR_NUMPAD_4";
                break;

            case DIK_NUMPAD5:
                pJonesString = "JONES_STR_NUMPAD_5";
                break;

            case DIK_NUMPAD6:
                pJonesString = "JONES_STR_NUMPAD_6";
                break;

            case DIK_NUMPAD7:
                pJonesString = "JONES_STR_NUMPAD_7";
                break;

            case DIK_NUMPAD8:
                pJonesString = "JONES_STR_NUMPAD_8";
                break;

            case DIK_NUMPAD9:
                pJonesString = "JONES_STR_NUMPAD_9";
                break;

            case DIK_NUMPAD0:
                pJonesString = "JONES_STR_NUMPAD_0";
                break;

            case DIK_NUMPADENTER:
                pJonesString = "JONES_STR_NUMPAD_ENTER";
                break;

            case DIK_NUMPADCOMMA:
                pJonesString = "JONES_STR_NUMPAD_COMMA";
                break;

            case DIK_NUMPADEQUALS:
                pJonesString = "JONES_STR_NUMPAD_EQL";
                break;

            case DIK_MULTIPLY:
                pJonesString = "JONES_STR_NUMPAD_MULT";
                break;

            case DIK_SUBTRACT:
                pJonesString = "JONES_STR_NUMPAD_MINUS";
                break;

            case DIK_ADD:
                pJonesString = "JONES_STR_NUMPAD_PLUS";
                break;

            case DIK_DECIMAL:
                pJonesString = "JONES_STR_NUMPAD_DOT";
                break;

            case DIK_DIVIDE:
                pJonesString = "JONES_STR_NUMPAD_DIV";
                break;

            case DIK_F1:
                pJonesString = "JONES_STR_F1";
                break;

            case DIK_F2:
                pJonesString = "JONES_STR_F2";
                break;

            case DIK_F3:
                pJonesString = "JONES_STR_F3";
                break;

            case DIK_F4:
                pJonesString = "JONES_STR_F4";
                break;

            case DIK_F5:
                pJonesString = "JONES_STR_F5";
                break;

            case DIK_F6:
                pJonesString = "JONES_STR_F6";
                break;

            case DIK_F7:
                pJonesString = "JONES_STR_F7";
                break;

            case DIK_F8:
                pJonesString = "JONES_STR_F8";
                break;

            case DIK_F9:
                pJonesString = "JONES_STR_F9";
                break;

            case DIK_F10:
                pJonesString = "JONES_STR_F10";
                break;

            case DIK_F11:
                pJonesString = "JONES_STR_F11";
                break;

            case DIK_F12:
                pJonesString = "JONES_STR_F12";
                break;

            case DIK_F13:
                pJonesString = "JONES_STR_F13";
                break;

            case DIK_F14:
                pJonesString = "JONES_STR_F14";
                break;

            case DIK_F15:
                pJonesString = "JONES_STR_F15";
                break;

            case JONESCONFIG_JOYAXISKID_UP:
                pJonesString = "JONES_STR_JOYUP";
                break;

            case JONESCONFIG_JOYAXISKID_RIGHT:
                pJonesString = "JONES_STR_JOYRIGHT";
                break;

            case JONESCONFIG_JOYAXISKID_DOWN:
                pJonesString = "JONES_STR_JOYDOWN";
                break;

            case JONESCONFIG_JOYAXISKID_LEFT:
                pJonesString = "JONES_STR_JOYLEFT";
                break;

            case JONESCONFIG_JOYAXISKID_TWISTR:
                pJonesString = "JONES_STR_JOYTWISTR";
                break;

            case JONESCONFIG_JOYAXISKID_TWISTL:
                pJonesString = "JONES_STR_JOYTWISTL";
                break;

            // Note missing handling of JONESCONFIG_JOYAXISKID_AIR and JONESCONFIG_JOYAXISKID_GROUND

            default:
            {
                if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 0 )
                {
                    // Index 0
                    pJonesString = "JONES_STR_HATLEFT";
                }
                else if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 2 )
                {
                    pJonesString = "JONES_STR_HATRIGHT";
                }
                else if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 3 )
                {
                    pJonesString = "JONES_STR_HATDOWN";
                }
                else if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == 1 )
                {
                    pJonesString = "JONES_STR_HATUP";
                }
                else if ( controlId >= STDCONTROL_JOYSTICK_FIRSTKID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID )
                {
                    pJonesString = "JONES_STR_JOYSTICK";
                }
            } break;
    }

    if ( pJonesString )
    {
        pControlStr = jonesString_GetString(pJonesString);
        if ( pControlStr )
        {
            if ( controlId >= STDCONTROL_MAX_KEYBOARD_BUTTONS && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID
                && !(controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRLEFT)
                && !(controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRUP)
                && !(controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRRIGHT)
                && !(controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRDOWN) )
            {
                // Joystick button
                buttonNumber = STDCONTROL_JOYSTICK_GETBUTTONINDEX(controlId) + 1;
                sprintf(pDest, pControlStr, buttonNumber);
            }
            else if ( controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRLEFT
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRUP
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRRIGHT
                || controlId >= STDCONTROL_JOYSTICK_FIRTPOVCID && controlId <= STDCONTROL_JOYSTICK_LASTPOVCID && STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(controlId) == STDCONTROL_JOYSTICK_POVDIRDOWN )
            {
                // Joystick POV
                povCount = STDCONTROL_JOYSTICK_GETPOVINDEX(controlId) + 1;
                sprintf(pDest, pControlStr, povCount);
            }
            else
            {
                // Regular key or joystick stick move
                strcpy(pDest, pControlStr);
            }
        }
    }
}

static int J3DAPI jonesConfig_ControlOptions_SetAllDefaultControlSchemes(JonesControlsConfig* pConfig)
{
    int i;

    for ( i = 0; i < (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; i++ )
    {
        JonesControlsScheme* pScheme;

        if ( !(pConfig->numSchemes % 10) )
        {
#line 1563
            pConfig->aSchemes = (JonesControlsScheme*)STDREALLOC(pConfig->aSchemes, sizeof(JonesControlsScheme) * (pConfig->numSchemes + 10));
        }

        if ( !pConfig->aSchemes )
        {
            return 0;
        }

        pScheme = &pConfig->aSchemes[pConfig->numSchemes];
        if ( !pScheme )
        {
            return 0;
        }

        if ( !jonesConfig_SetDefaultControlScheme(pScheme, i) )
        {
            return 0;
        }

        ++pConfig->numSchemes;
    }

    return 1;
}

int J3DAPI jonesConfig_ShowMessageDialog(HWND hWnd, const char* pTitle, const char* pText, int iconID)
{
    JonesMessageDialogData data;
    HINSTANCE hInstance;
    int dlgResult;

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

    memset(&data, 0, sizeof(data));

    data.iconID = iconID;
    strcpy(data.aText, pText);
    strcpy(data.aTitle, pTitle);
    dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(121), hWnd, jonesConfig_MessageDialogProc, (LPARAM)&data);
    return dlgResult;
}

BOOL CALLBACK jonesConfig_MessageDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = TRUE;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_MessageDialog_HandleWM_COMMAND(hwnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return FALSE;

        case WM_INITDIALOG:
        {
            JonesMessageDialogData* pData = (JonesMessageDialogData*)lParam;
            SetWindowText(hwnd, pData->aTitle);
            jonesConfig_InitMessageDialogIcon(hwnd, pData);
            jonesConfig_hFontMessageDialog = (HGDIOBJ)jonesConfig_InitDialog(hwnd, 0, 121);
            bResult = jonesConfig_InitMessageDialogData(hwnd, wParam, lParam);

            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;
        }

        case WM_DESTROY:
        {
            JonesMessageDialogData* pData = (JonesMessageDialogData*)GetWindowLong(hwnd, DWL_USER);
            HWND hIconCtrl = GetDlgItem(hwnd, 1201);
            jonesConfig_ResetDialogFont(hwnd, jonesConfig_hFontMessageDialog);

            if ( pData )
            {
                if ( hIconCtrl )
                {
                    HICON hCurIcon = Static_GetIcon(hIconCtrl, NULL);
                    Static_SetIcon(hIconCtrl, pData->hPrevIcon);
                    pData = NULL;
                    DeleteObject(hCurIcon);
                }
            }

            break;
        }

        default:
            bResult = FALSE;
            break;
    }

    return bResult;
}

void J3DAPI jonesConfig_InitMessageDialogIcon(HWND hWnd, JonesMessageDialogData* pData)
{
    HINSTANCE hInstance;
    HWND hImgCtrl;
    HICON hIcon;
    HANDLE img;

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    if ( !pData )
    {
        return;
    }

    img = LoadImage(hInstance, MAKEINTRESOURCE(pData->iconID), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR);
    hImgCtrl = GetDlgItem(hWnd, 1201);
    if ( hImgCtrl && img )
    {
        hIcon = Static_GetIcon(hImgCtrl, 0);
        Static_SetIcon(hImgCtrl, img);
        pData->hPrevIcon = hIcon;
    }
}

int J3DAPI jonesConfig_InitMessageDialogData(HWND hDlg, int a2, LPARAM lParam)
{
    JonesMessageDialogData* pCurData;
    HWND hTextCtrl;
    HDC hdc;
    HICON hIcon;  // dead local: frame slot ebp-4 (zero-initialised in the debug build)
    HFONT curFont;  // dead local: frame slot ebp-0x14 (zero-initialised in the debug build)
    char* pText;

    hTextCtrl = GetDlgItem(hDlg, 1075);
    pCurData = (JonesMessageDialogData*)lParam;
    hdc = GetDC(hTextCtrl); // TODO: ??
    hIcon = NULL;
    curFont = NULL;
    if ( !pCurData )
    {
        return 0;
    }

    jonesConfig_InitMessageDialogIcon(hDlg, pCurData);
    pText = pCurData->aText;
    jonesConfig_SetTextControl(hDlg, hTextCtrl, pText);
    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
    return 1;
}

int J3DAPI jonesConfig_SetTextControl(HWND hDlg, HWND hTextCtrl, const char* aText)
{
    const char* pCurPos = NULL;
    const char* pStart  = NULL;
    HDC hdc;
    RECT rectWnd;
    LONG dlgX;
    LONG dlgY;
    LONG maxWidth;
    size_t numLines;
    LONG yOffset;

    hdc = GetDC(hTextCtrl);
    SelectObject(hdc, (HGDIOBJ)SendMessage(hTextCtrl, WM_GETFONT, 0, 0));

    if ( !jonesConfig_bTextMetricInited )
    {
        memset(jonesConfig_aGlyphMetrics, 0, sizeof(jonesConfig_aGlyphMetrics));
        memset(&jonesConfig_textMetric, 0, sizeof(jonesConfig_textMetric));
        GetTextMetrics(hdc, (LPTEXTMETRICA)&jonesConfig_textMetric);
        GetCharABCWidths(hdc, jonesConfig_textMetric.tmFirstChar, jonesConfig_textMetric.tmLastChar, jonesConfig_aGlyphMetrics);
        jonesConfig_bTextMetricInited = 1;
    }

    numLines = 0;
    GetWindowRect(hDlg, &rectWnd);
    dlgX = rectWnd.left;
    dlgY = rectWnd.top;

    GetWindowRect(hTextCtrl, &rectWnd);
    maxWidth = rectWnd.right - rectWnd.left;

    pCurPos = aText;
    do
    {
        char aLine[256];
        int lineWidth;
        int lineLen;
        int i;

        pStart  = pCurPos;
        pCurPos = strchr(pStart, '\n');

        memset(aLine, 0, sizeof(aLine));
        if ( pCurPos )
        {
            strncpy(aLine, pStart, pCurPos - pStart);
        }
        else
        {
            strcpy(aLine, pStart);
        }

        lineWidth = 0;
        lineLen   = strlen(aLine);
        for ( i = 0; i < lineLen; ++i )
        {
            const ABC* pCharMetric = &jonesConfig_aGlyphMetrics[(uint8_t)aLine[i] - jonesConfig_textMetric.tmFirstChar];
            lineWidth += abs(pCharMetric->abcA) + abs(pCharMetric->abcB) + abs(pCharMetric->abcC);
        }

        if ( !lineLen && pCurPos )
        {
            ++numLines;
        }
        else
        {
            numLines += (int32_t)ceil((double)lineWidth / maxWidth);
        }

        if ( pCurPos )
        {
            if ( strlen(++pCurPos) == 0 )
            {
                pCurPos = NULL;
            }
        }
    } while ( pCurPos );

    yOffset = (jonesConfig_textMetric.tmHeight + jonesConfig_textMetric.tmExternalLeading) * numLines - (rectWnd.bottom - rectWnd.top);
    if ( yOffset > 0 )
    {
        HWND hOkBtn;
        HWND hNoBtn;
        int heightCaption;
        int frameSize;
        RECT rectBtn;
        int btnX;
        int btnY;
        int textX;
        int textY;
        RECT rectDlg;

        hOkBtn        = GetDlgItem(hDlg, 1);
        hNoBtn        = GetDlgItem(hDlg, 2);
        heightCaption = GetSystemMetrics(SM_CYCAPTION);
        frameSize     = GetSystemMetrics(SM_CYDLGFRAME) * 2;
        GetWindowRect(hOkBtn, &rectBtn);
        rectBtn.top += yOffset;
        GetWindowRect(hDlg, &rectDlg);

        btnX  = rectBtn.left - rectDlg.left;
        btnY  = rectBtn.top - rectDlg.top - (heightCaption + frameSize);
        textX = rectWnd.left - rectDlg.left;
        textY = rectWnd.top - rectDlg.top - (heightCaption + frameSize);

        rectDlg.bottom += yOffset;
        rectBtn.bottom += yOffset;
        rectWnd.bottom += yOffset;

        MoveWindow(hDlg, dlgX, dlgY, rectDlg.right - rectDlg.left, rectDlg.bottom - rectDlg.top, 1);
        MoveWindow(hOkBtn, btnX, btnY, rectBtn.right - rectBtn.left, rectBtn.bottom - rectBtn.top, 1);
        MoveWindow(hTextCtrl, textX, textY, rectWnd.right - rectWnd.left, rectWnd.bottom - rectWnd.top, 1);

        if ( hNoBtn )
        {
            RECT rectNoBtn;
            int btnNoX;
            int btnNoY;
            GetWindowRect(hNoBtn, &rectNoBtn);
            rectNoBtn.top += yOffset;
            btnNoX = rectNoBtn.left - rectDlg.left;
            btnNoY = rectNoBtn.top - rectDlg.top - (heightCaption + frameSize);
            rectNoBtn.bottom += yOffset;
            MoveWindow(hNoBtn, btnNoX, btnNoY, rectNoBtn.right - rectNoBtn.left, rectNoBtn.bottom - rectNoBtn.top, 1);
        }
    }

    SetWindowText(hTextCtrl, aText);
    return ShowWindow(hDlg, 1);
}

void J3DAPI jonesConfig_MessageDialog_HandleWM_COMMAND(HWND hDlg, int nResult, LPARAM lParam, int notifyCode)
{
    switch ( nResult )
    {
        case 1:
        case 2:
            EndDialog(hDlg, nResult);
    }
}

JonesControlsScheme* J3DAPI jonesConfig_LoadActiveControlScheme(void)
{
    JonesControlsScheme* pScheme;
    size_t schemeIdx;
    const char* pDfltKeyset;
    LPSTR pFilePart;
    int i;
    char aFilename[128];
    char aPath[128];
    char aFilePath[128];

    memset(aFilename, 0, sizeof(aFilename));

#line 2350
    pScheme = (JonesControlsScheme*)STDMALLOC(sizeof(JonesControlsScheme));
    if ( !pScheme )
    {
        return NULL;
    }

    memset(aPath, 0, sizeof(aPath));
    memset(pScheme, 0, sizeof(JonesControlsScheme));

    // Find active control scheme in system
    wuRegistry_GetStr("Configuration", aFilename, STD_ARRAYLEN(aFilename), "");
    if ( strlen(aFilename) > 0 )
    {
        // Load config scheme to new scheme
        memset(aFilePath, 0, sizeof(aFilePath));
        sprintf(aFilePath, "%s\\%s%s", jonesConfig_aKeySetDirectory, aFilename, ".kfg");

        memset(aPath, 0, sizeof(aPath));
        SearchPath(jonesConfig_aKeySetDirectory, aFilename, ".kfg", STD_ARRAYLEN(aPath), aPath, &pFilePart);
        if ( strlen(aPath) > 0 && jonesConfig_LoadControlScheme(aFilePath, pScheme) )
        {
            return pScheme;
        }

        // Load failed, try to make new scheme to default scheme based on config scheme name
        for ( i = 0; i < (int)STD_ARRAYLEN(jonesConfig_aDfltKeySetNames); )
        {
            pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[i]);
            if ( pDfltKeyset )
            {
                strcpy(pScheme->aName, pDfltKeyset);

                if ( streq(aFilename, pDfltKeyset) )
                {
                    // Retail bug: loops forever when aPath is set or setting the default scheme fails
                    if ( !strlen(aPath) && jonesConfig_SetDefaultControlScheme(pScheme, i) )
                    {
                        return pScheme;
                    }
                }
                else
                {
                    ++i;
                }
            }
            else
            {
                goto error;
            }
        }

        // Clear keyset config
        wuRegistry_SaveStr("Configuration", "");
        goto error;
    }

    // TODO: Hmm why is this check here? Should be already handled in above scope.
    if ( !strlen(aFilename) || !strlen(aPath) )
    {
        // Ok we have no scheme configured, let's load default
        switch ( stdControl_GetMaxJoystickButtons() )
        {
            case 4:
                schemeIdx = 1;
                break;

            case 8:
                schemeIdx = 2;
                break;

            default:
                schemeIdx = 0;
                break;
        }

        pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[schemeIdx]);
        if ( pDfltKeyset )
        {
            strcpy(pScheme->aName, pDfltKeyset);
            if ( jonesConfig_SetDefaultControlScheme(pScheme, schemeIdx) )
            {
                wuRegistry_SaveStr("Configuration", pDfltKeyset);
            }
            else
            {
                goto error;
            }
        }
        else
        {
            goto error;
        }
    }

    return pScheme;

error:
    jonesConfig_FreeControlScheme(pScheme);
    return NULL;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(jonesConfig_countfit_30, 30, 112) CF_PROTO(jonesConfig_countfit_129, 14) /* end count-fit */
int J3DAPI jonesConfig_LoadControlScheme(const char* pFilePath, JonesControlsScheme* pConfig)
{
    int result = 0; // debug build: initialised, never read
    int i;
    int j;
    int bindNum;
    int argNum;
    size_t* paActionBindings;
    int actionIdx;
    int numControllerBinds;
    int prevArgNum;
    int numKeyboardBinds;
    const char* pFaultyActionName;
    char aErrorActionName[512];
    const char* pErrorText;
    char aErrorFormat[512];
    char aErrorStr[256];

    // Function loads kfg file to pData
    if ( !stdConffile_Open(pFilePath) || !pConfig || !pFilePath )
    {
        return 0;
    }

    memset(pConfig->aName, 0, sizeof(pConfig->aName));
    strncpy(pConfig->aName, pFilePath, strlen(pFilePath) - 4);// -4 removes .kfg extension

    bindNum = 0;
    stdConffile_ReadArgs(); // Skip first comment line
    stdConffile_ReadArgs();
    if ( !(stdConffile_entry.numArgs && strneqi(stdConffile_entry.aArgs[0].argName, "1", 128u)) ) // 1 mark scheme is enabled
    {
        goto error;
    }

    stdConffile_ReadArgs(); // Skip header line
    for ( i = 0; i < (int)STD_ARRAYLEN(pConfig->aActions); i++ )
    {
        argNum = 0;
        if ( !stdConffile_ReadArgs() )
        {
            goto error;
        }

        actionIdx = jonesConfig_ParseArgumentIndex((size_t*)&argNum, &stdConffile_entry, jonesConfig_maxActionWords, jonesConfig_aJonesControlActionNames, STD_ARRAYLEN(jonesConfig_aJonesControlActionNames));
        if ( actionIdx == -1 )
        {
            goto error;
        }

        bindNum = 1; // Note, at index 0 the num of bindings state is stored
        paActionBindings = pConfig->aActions[actionIdx];
        paActionBindings[0] = 0; // Reset number of bindings state
        while ( paActionBindings && argNum < (int)stdConffile_entry.numArgs )
        {
            numControllerBinds = 0;
            numKeyboardBinds   = 0;
            prevArgNum         = argNum;
            paActionBindings[bindNum] = jonesConfig_ParseArgumentIndex((size_t*)&argNum, &stdConffile_entry, jonesConfig_maxControlKeyWordCount, jonesConfig_aControlKeyStrings, STD_ARRAYLEN(jonesConfig_aControlKeyStrings));
            if ( paActionBindings[bindNum] == -1 )
            {
                // We couldn't find control key num, try searching by control mouse button num, gamepad & joystick button num
                argNum = prevArgNum;
                paActionBindings[bindNum] = jonesConfig_ParseJoystickControlId((size_t*)&argNum, &stdConffile_entry);
                if ( paActionBindings[bindNum] == -1 )
                {
                    goto error;
                }

                numControllerBinds++;
                argNum++;
            }
            else
            {
                numKeyboardBinds++;
            }

            bindNum++;
            JONESCONTROL_ACTION_UPDATENUMBINDINGS(paActionBindings, numKeyboardBinds, numControllerBinds);
            if ( paActionBindings && JONESCONTROL_ACTION_GETNUMBINDS(paActionBindings) > JONESCONTROL_ACTION_MAXBINDS ) // TODO: is this a bug? Shouldn't it be GT or EQ?
            {
                // We've exceeded the number of key mappings
                pFaultyActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[actionIdx]);
                if ( pFaultyActionName )
                {
                    strcpy(aErrorActionName, pFaultyActionName);
                }
                else
                {
                    memset(aErrorActionName, 0, sizeof(aErrorActionName));
                }

                pErrorText = jonesString_GetString("JONES_STR_EXCEED");
                if ( pErrorText )
                {
                    strcpy(aErrorFormat, pErrorText);
                }
                else
                {
                    memset(aErrorFormat, 0, sizeof(aErrorFormat));
                }

                memset(aErrorStr, 0, sizeof(aErrorStr));
                if ( aErrorFormat[0] && aErrorActionName[0] )
                {
                    sprintf(aErrorStr, aErrorFormat, aErrorActionName);
                    jonesConfig_ShowMessageDialog(stdWin95_GetWindow(), "JONES_STR_CTRL_OPTS", aErrorStr, 145);
                }

                goto error;
            }
        }
    }

    stdConffile_Close();
    return 1;

error:
    memset(pConfig->aName, 0, sizeof(pConfig->aName));
    for ( j = 0; j < bindNum; j++ )
    {
        if ( pConfig->aActions[j] ) // Retail bug: tests a row address, which is never null
        {
            memset(pConfig->aActions[j], 0, STD_ARRAYLEN(pConfig->aActions[j])); // Retail bug: clears 9 bytes, not 9 entries
        }
    }

    stdConffile_Close();
    return 0;
}

int J3DAPI jonesConfig_ParseArgumentIndex(size_t* argNum, const StdConffileEntry* pEntry, size_t a3, const char** apStringList, size_t size)
{
    int result;
    char aValue[256];



    strncpy(aValue, pEntry->aArgs[*argNum].argName, STD_ARRAYLEN(aValue)); // Retail: no terminator
    result = jonesConfig_GetStringListIndex(aValue, apStringList, (int)size);

    ++*argNum;
    return result;
}


int J3DAPI jonesConfig_GetStringListIndex(char* pValue, const char** aStringList, int size)
{
    int index = 0;

    if ( size == STD_ARRAYLEN(jonesConfig_aControlKeyStrings) ) // if the list is list of keys, TODO: kill this nonsense and make case insensitive comparison
    {
        pValue = _strupr(pValue);
    }

    while ( index < size )
    {
        if ( aStringList[index] )
        {
            const char* pStr = jonesString_GetString(aStringList[index]);
            if ( pStr )
            {
                if ( strneqi(pValue, pStr, strlen(pStr)) && strlen(pStr) == strlen(pValue) )
                {
                    return index;
                }

                index++;
            }
            else
            {
                if ( strneqi(pValue, aStringList[index], strlen(pValue)) && strlen(pValue) == strlen(aStringList[index]) )
                {
                    return index;
                }

                index++;
            }
        }
        else
        {
            index++;
        }
    }

    return -1;
}

int J3DAPI jonesConfig_ParseJoystickControlId(size_t* pArgNum, const StdConffileEntry* pEntry)
{
    char aForward[512];
    char aBackward[512];
    char aLeft[512];
    char aRight[512];
    int controlId = -1;
    const char* pButtonFormat = NULL;
    const char* pUpText = NULL;
    const char* pDownText = NULL;
    const char* pLeftText = NULL;
    const char* pRightText = NULL;

    pButtonFormat = jonesString_GetString("JONES_STR_CAPS_BUTTON");// "BUTTON"
    if ( pButtonFormat )
    {
        if ( strneqi(pEntry->aArgs[*pArgNum].argName, pButtonFormat, strlen(pButtonFormat)) )
        {
            char* pArg2 = strpbrk(pEntry->aArgs[*pArgNum].argName, "0123456789");
            if ( pArg2 )
            {
                int btnNum = atoi(pArg2) - 1;
                controlId = STDCONTROL_JOYSTICK_GETBUTTON(0, btnNum);
            }
        }
    }

    if ( controlId == -1 )
    {
        int btnNum = 0;
        char aArgText[256];
        char aHatFormat[256];
        char* pArg2;

        memset(aHatFormat, 0, sizeof(aHatFormat));
        memset(aArgText, 0, sizeof(aArgText));

        strcpy(aArgText, pEntry->aArgs[*pArgNum].argName);

        btnNum = strcspn(pEntry->aArgs[*pArgNum].argName, "0123456789");
        aArgText[btnNum] = '\0';

        if ( btnNum > 0 )
        {
            btnNum = atoi(&pEntry->aArgs[*pArgNum].argName[btnNum]) - 1;
        }

        pArg2 = strchr(pEntry->aArgs[*pArgNum].argName, ' ');
        if ( pArg2 )
        {
            sprintf(aHatFormat, "%s%%i %s", aArgText, pArg2 + 1);
        }

        pUpText = jonesString_GetString("JONES_STR_HATUP");// "HAT%i UP"
        if ( pUpText )
        {
            strcpy(aForward, pUpText);
        }
        else
        {
            memset(aForward, 0, sizeof(aForward));
        }

        pDownText = jonesString_GetString("JONES_STR_HATDOWN");// "HAT%i DOWN"
        if ( pDownText )
        {
            strcpy(aBackward, pDownText);
        }
        else
        {
            memset(aBackward, 0, sizeof(aBackward));
        }

        pLeftText = jonesString_GetString("JONES_STR_HATLEFT");// "HAT%i LEFT"
        if ( pLeftText )
        {
            strcpy(aLeft, pLeftText);
        }
        else
        {
            memset(aLeft, 0, sizeof(aLeft));
        }

        pRightText = jonesString_GetString("JONES_STR_HATRIGHT");// "HAT%i RIGHT"
        if ( pRightText )
        {
            strcpy(aRight, pRightText);
        }
        else
        {
            memset(aRight, 0, sizeof(aRight));
        }

        if ( aForward[0] && aBackward[0] && aLeft[0] && aRight[0] && strlen(aHatFormat) > 0 )
        {
            if ( strneqi(aHatFormat, aForward, strlen(aForward)) )
            {
                controlId = STDCONTROL_JOYSTICK_GETPOVUP(0, btnNum);
            }
            else
            {
                if ( strneqi(aHatFormat, aBackward, strlen(aBackward)) )
                {
                    controlId = STDCONTROL_JOYSTICK_GETPOVDOWN(0, btnNum);
                }
                else
                {
                    if ( strneqi(aHatFormat, aLeft, strlen(aLeft)) )
                    {
                        controlId = STDCONTROL_JOYSTICK_GETPOVLEFT(0, btnNum);
                    }
                    else
                    {
                        if ( strneqi(aHatFormat, aRight, strlen(aRight)) )
                        {
                            controlId = STDCONTROL_JOYSTICK_GETPOVRIGHT(0, btnNum);
                        }
                    }
                }
            }
        }
    }

    if ( controlId == -1 )
    {
        const char* pTwistLText;
        const char* pTwistRText;
        char aTwistLeft[512];
        char aTwistRight[512];
        char aArgText[256];

        pTwistLText = jonesString_GetString("JONES_STR_JOYTWISTL");// "TWISTL"
        if ( pTwistLText )
        {
            strcpy(aTwistLeft, pTwistLText);
        }
        else
        {
            memset(aTwistLeft, 0, sizeof(aTwistLeft));
        }

        pTwistRText = jonesString_GetString("JONES_STR_JOYTWISTR");// "TWISTR"
        if ( pTwistRText )
        {
            strcpy(aTwistRight, pTwistRText);
        }
        else
        {
            memset(aTwistRight, 0, sizeof(aTwistRight));
        }

        memset(aArgText, 0, sizeof(aArgText));
        sprintf(aArgText, "%s", pEntry->aArgs[*pArgNum].argName);

        if ( strneqi(aArgText, aTwistLeft, strlen(aTwistLeft)) )
        {
            controlId = JONESCONFIG_JOYAXISKID_TWISTL;
        }
        else
        {
            if ( strneqi(aArgText, aTwistRight, strlen(aTwistRight)) )
            {
                controlId = JONESCONFIG_JOYAXISKID_TWISTR;
            }
        }

    }

    if ( controlId == -1 )
    {

        pUpText = jonesString_GetString("JONES_STR_JOYUP");// "FORWARD"
        if ( pUpText )
        {
            strcpy(aForward, pUpText);
        }
        else
        {
            memset(aForward, 0, sizeof(aForward));
        }

        pDownText = jonesString_GetString("JONES_STR_JOYDOWN");// "BACKWARD"
        if ( pDownText )
        {
            strcpy(aBackward, pDownText);
        }
        else
        {
            memset(aBackward, 0, sizeof(aBackward));
        }

        pLeftText = jonesString_GetString("JONES_STR_JOYLEFT");// "LEFT"
        if ( pLeftText )
        {
            strcpy(aLeft, pLeftText);
        }
        else
        {
            memset(aLeft, 0, sizeof(aLeft));
        }

        pRightText = jonesString_GetString("JONES_STR_JOYRIGHT");// "RIGHT"
        if ( pRightText )
        {
            strcpy(aRight, pRightText);
        }
        else
        {
            memset(aRight, 0, sizeof(aRight));
        }

        if ( strneqi(pEntry->aArgs[*pArgNum].argName, aForward, strlen(aForward)) )
        {
            controlId = JONESCONFIG_JOYAXISKID_UP;
        }
        else
        {
            if ( strneqi(pEntry->aArgs[*pArgNum].argName, aBackward, strlen(aBackward)) )
            {
                controlId = JONESCONFIG_JOYAXISKID_DOWN;
            }
            else
            {
                if ( strneqi(pEntry->aArgs[*pArgNum].argName, aLeft, strlen(aLeft)) )
                {
                    controlId = JONESCONFIG_JOYAXISKID_LEFT;
                }
                else
                {
                    if ( strneqi(pEntry->aArgs[*pArgNum].argName, aRight, strlen(aRight)) )
                    {
                        controlId = JONESCONFIG_JOYAXISKID_RIGHT;
                    }
                }
            }
        }
    }


    if ( controlId > -1 )
    {
        ++*pArgNum;
    }

    return controlId;
}

int J3DAPI jonesConfig_SetDefaultControlScheme(JonesControlsScheme* pScheme, size_t num)
{
    const char* pDefaultKeySetName;
    int idx;
    int j;
    size_t* pAction;

    pDefaultKeySetName = jonesString_GetString(jonesConfig_aDfltKeySetNames[num]);
    if ( pDefaultKeySetName )
    {
        strcpy(pScheme->aName, pDefaultKeySetName);
    }
    else
    {
        return 0;
    }

    for ( idx = 0; idx < (int)JONESCONTROL_ACTION_NUMACTIONS; ++idx )
    {
        pAction = pScheme->aActions[idx];
        *pAction = 0;

        for ( j = 1; j <= jonesConfig_aDfltKeySets[num][idx].numBinds; ++j )
        {
            pAction[j] = jonesConfig_aDfltKeySets[num][idx].aBindings[j - 1];
            if ( (pAction[j] >= STDCONTROL_MAX_KEYBOARD_BUTTONS && pAction[j] <= STDCONTROL_JOYSTICK_LASTPOVCID)
                || (pAction[j] >= JONESCONFIG_JOYAXISKID_UP && pAction[j] <= JONESCONFIG_JOYAXISKID_GROUND) )
            {
                // joystick controller
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pAction, +1);
            }
            else
            {
                // keyboard
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pAction, +1);
            }
        }
    }

    return 1;
}

void J3DAPI jonesConfig_BindControls(JonesControlsScheme* pConfig)
{
    JonesControlActions actionId;
    int bMouse    = wuRegistry_GetIntEx("Mouse Control", 0);
    int bJoystick = wuRegistry_GetIntEx("Joystick Control", 0);
    int n;
    size_t* binds;
    SithControlFunction functionId = -1;

    if ( !pConfig )
    {
        return;
    }

    // Bind toggle menu keys
    binds = &pConfig->aActions[JONESCONTROL_ACTION_INTERFACE][1];
    JonesMain_BindToggleMenuControlKeys(binds, JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[JONESCONTROL_ACTION_INTERFACE]));

    // Bind toggle menu keys
    binds = &pConfig->aActions[JONESCONTROL_ACTION_ACTIVATE][1];
    JonesHud_BindActivateControlKeys(binds, JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[JONESCONTROL_ACTION_ACTIVATE]));

    for ( functionId = SITHCONTROL_WSELECT0; functionId <= SITHCONTROL_WSELECT16; ++functionId )
    {
        sithControl_UnbindFunction(functionId);
    }

    for ( actionId = JONESCONTROL_ACTION_WALKFORWARD; actionId < JONESCONTROL_ACTION_NUMACTIONS; ++actionId )
    {
        functionId = -1;

        if ( actionId == JONESCONTROL_ACTION_ACTIVATE || actionId == JONESCONTROL_ACTION_TURNLEFT || actionId == JONESCONTROL_ACTION_TURNRIGHT )
        {
            int curBindNum = 1;
            int numActionBinds; // TODO: [BUG] uninitialized when actionId < TURNLEFT (0x00403D08 skips both stores)
            switch ( actionId )
            {
                case JONESCONTROL_ACTION_ACTIVATE:
                    numActionBinds = 3;
                    break;

                case JONESCONTROL_ACTION_TURNLEFT:
                case JONESCONTROL_ACTION_TURNRIGHT:
                    numActionBinds = 2;
                    break;
            }

            while ( curBindNum <= numActionBinds )
            {
                switch ( actionId )
                {
                    case JONESCONTROL_ACTION_ACTIVATE:
                        switch ( curBindNum )
                        {
                            case 1:
                                functionId = SITHCONTROL_ACT2;
                                break;

                            case 2:
                                functionId = SITHCONTROL_FIRE1;
                                break;

                            case 3:
                                functionId = SITHCONTROL_ACTIVATE;
                                break;
                        }
                        break;

                    case JONESCONTROL_ACTION_TURNLEFT:
                        switch ( curBindNum )
                        {
                            case 1:
                                functionId = SITHCONTROL_LEFT;
                                break;

                            case 2:
                                functionId = SITHCONTROL_TURNLEFT;
                                break;
                        }
                        break;

                    case JONESCONTROL_ACTION_TURNRIGHT:
                        switch ( curBindNum )
                        {
                            case 1:
                                functionId = SITHCONTROL_RIGHT;
                                break;

                            case 2:
                                functionId = SITHCONTROL_TURNRIGHT;
                                break;
                        }
                        break;
                }

                if ( functionId != -1 )
                {
                    sithControl_UnbindFunction(functionId);

                    // TODO: This must be a bug checking i is LT or EQ, most likely should be LT
                    {
                        for ( n = 1; n <= (int)JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]); ++n )
                    {
                        if ( !(pConfig->aActions[actionId][n] >= STDCONTROL_JOYSTICK_FIRSTKID && pConfig->aActions[actionId][n] <= STDCONTROL_JOYSTICK_LASTPOVCID
                            || pConfig->aActions[actionId][n] >= STDCONTROL_JOYSTICK_FIRSTKID) )
                        {
                            // We have keyboard key bind
                            sithControl_BindControl(functionId, pConfig->aActions[actionId][n], (SithControlBindFlag)0);
                        }
                        else
                        {
                            // We have joystick key bind
                            if ( bJoystick )
                            {
                                jonesConfig_BindJoystickControl(functionId, pConfig->aActions[actionId][n]);
                            }
                        }
                    }
                    }
                }

                ++curBindNum;
            }
        }

        else if ( actionId >= JONESCONTROL_ACTION_FISTS && actionId <= JONESCONTROL_ACTION_CHALK && actionId != JONESCONTROL_ACTION_MAP && actionId != JONESCONTROL_ACTION_INTERFACE )
        {
            if ( JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]) > 0 ) // TODO: this is kinda useless
            {
                // TODO: This must be a bug checking i is LT or EQ, most likely should be LT
                {
                    for ( n = 1; n <= (int)JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]); ++n )
                {
                    switch ( actionId )
                    {
                        case JONESCONTROL_ACTION_FISTS:
                            functionId = SITHCONTROL_WSELECT0;
                            break;

                        case JONESCONTROL_ACTION_WHIP:
                            functionId = SITHCONTROL_WSELECT1;
                            break;

                        case JONESCONTROL_ACTION_MAUSER:
                            functionId = SITHCONTROL_WSELECT4;
                            break;

                        case JONESCONTROL_ACTION_PPSH41:
                            functionId = SITHCONTROL_WSELECT7;
                            break;

                        case JONESCONTROL_ACTION_PISTOL:
                            functionId = SITHCONTROL_WSELECT2;
                            break;

                        case JONESCONTROL_ACTION_SIMONOV:
                            functionId = SITHCONTROL_WSELECT5;
                            break;

                        case JONESCONTROL_ACTION_TOKEREV:
                            functionId = SITHCONTROL_WSELECT3;
                            break;

                        case JONESCONTROL_ACTION_TOZ34:
                            functionId = SITHCONTROL_WSELECT8;
                            break;

                        case JONESCONTROL_ACTION_BAZOOKA:
                            functionId = SITHCONTROL_WSELECT9;
                            break;

                        case JONESCONTROL_ACTION_MACHETE:
                            functionId = SITHCONTROL_WSELECT6;
                            break;

                        case JONESCONTROL_ACTION_SATCHEL:
                            functionId = SITHCONTROL_WSELECT11;
                            break;

                        case JONESCONTROL_ACTION_GRENADE:
                            functionId = SITHCONTROL_WSELECT10;
                            break;

                        case JONESCONTROL_ACTION_IMP1:
                            functionId = SITHCONTROL_WSELECT12;
                            break;

                        case JONESCONTROL_ACTION_IMP2:
                            functionId = SITHCONTROL_WSELECT13;
                            break;

                        case JONESCONTROL_ACTION_IMP3:
                            functionId = SITHCONTROL_WSELECT14;
                            break;

                        case JONESCONTROL_ACTION_IMP4:
                            functionId = SITHCONTROL_WSELECT15;
                            break;

                        case JONESCONTROL_ACTION_IMP5:
                            functionId = SITHCONTROL_WSELECT16;
                            break;

                        case JONESCONTROL_ACTION_HEALTH:
                            functionId = SITHCONTROL_HEALTH;
                            break;

                        case JONESCONTROL_ACTION_CHALK:
                            functionId = SITHCONTROL_CHALK;
                            break;

                        default:
                            break;
                    }

                    if ( functionId != -1 )
                    {
                        if ( !(pConfig->aActions[actionId][n] >= STDCONTROL_JOYSTICK_FIRSTKID && pConfig->aActions[actionId][n] <= STDCONTROL_JOYSTICK_LASTPOVCID
                            || pConfig->aActions[actionId][n] >= STDCONTROL_JOYSTICK_FIRSTKID) )
                        {
                            // We have keyboard key bind
                            sithControl_BindControl(functionId, pConfig->aActions[actionId][n], (SithControlBindFlag)0);
                        }
                        else
                        {
                            // We have joystick key bind
                            if ( bJoystick )
                            {
                                jonesConfig_BindJoystickControl(functionId, pConfig->aActions[actionId][n]);
                            }
                        }
                    }
                }
                }
            }
        }
        else
        {
            switch ( actionId )
            {
                case JONESCONTROL_ACTION_WALKFORWARD:
                    functionId = SITHCONTROL_FORWARD;
                    break;

                case JONESCONTROL_ACTION_WALKBACK:
                    functionId = SITHCONTROL_BACK;
                    break;

                case JONESCONTROL_ACTION_STEPLEFT:
                    functionId = SITHCONTROL_STPLEFT;
                    break;

                case JONESCONTROL_ACTION_STEPRIGHT:
                    functionId = SITHCONTROL_STPRIGHT;
                    break;

                case JONESCONTROL_ACTION_CRAWL:
                    functionId = SITHCONTROL_CRAWLTOGGLE;
                    break;

                case JONESCONTROL_ACTION_RUN:
                    functionId = SITHCONTROL_ACT1;
                    break;

                case JONESCONTROL_ACTION_ROLL:
                    functionId = SITHCONTROL_ACT3;
                    break;

                case JONESCONTROL_ACTION_JUMP:
                    functionId = SITHCONTROL_JUMP;
                    break;

                case JONESCONTROL_ACTION_LOOK:
                    functionId = SITHCONTROL_LOOK;
                    break;

                case JONESCONTROL_ACTION_TOGGLEWEAPON:
                    functionId = SITHCONTROL_WEAPONTOGGLE;
                    break;

                case JONESCONTROL_ACTION_PREVWEAPON:
                    functionId = SITHCONTROL_PREVWEAPON;
                    break;

                case JONESCONTROL_ACTION_NEXTWEAPON:
                    functionId = SITHCONTROL_NEXTWEAPON;
                    break;

                case JONESCONTROL_ACTION_TOGGLELIGHT:
                    functionId = SITHCONTROL_LIGHTERTOGGLE;
                    break;

                case JONESCONTROL_ACTION_MAP:
                    functionId = SITHCONTROL_MAP;
                    break;

                default:
                    break;
            }

            if ( functionId != -1 )
            {
                sithControl_UnbindFunction(functionId);
                if ( functionId == SITHCONTROL_FORWARD )
                {
                    sithControl_UnbindFunction(SITHCONTROL_RUNFWD);
                }

                // TODO: This must be a bug checking i is LT or EQ, most likely should be LT
                {
                    for ( n = 1; n <= (int)JONESCONTROL_ACTION_GETNUMBINDS(pConfig->aActions[actionId]); ++n )
                {
                    if ( !(pConfig->aActions[actionId][n] >= STDCONTROL_JOYSTICK_FIRSTKID && pConfig->aActions[actionId][n] <= STDCONTROL_JOYSTICK_LASTPOVCID
                        || pConfig->aActions[actionId][n] >= STDCONTROL_JOYSTICK_FIRSTKID) )
                    {
                        // We have keyboard key bind
                        sithControl_BindControl(functionId, pConfig->aActions[actionId][n], (SithControlBindFlag)0);
                    }
                    else
                    {
                        // We have joystick key bind
                        if ( bJoystick )
                        {
                            jonesConfig_BindJoystickControl(functionId, pConfig->aActions[actionId][n]);
                        }
                    }
                }
                }
            }
        }
    }

    jonesConfig_EnableMouseControl(bMouse);
    if ( !bJoystick )
    {
        sithControl_UnbindJoystickAxes();
    }
}

void J3DAPI jonesConfig_BindJoystickControl(SithControlFunction functionId, size_t controlId)
{
    int numJoysticks = stdControl_GetNumJoysticks();
    int axisIndex;
    int flags;
    size_t buttonIdx;
    int joyNum;

    switch ( controlId )
    {
        case JONESCONFIG_JOYAXISKID_UP:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_Y;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_DOWN:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_Y;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_RIGHT:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_X;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_LEFT:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_X;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_AIR:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_Z;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_GROUND:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_Z;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_TWISTL:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_RZ;
            flags = STDCONTROL_AID_NEGATIVE_AXIS;
            break;

        case JONESCONFIG_JOYAXISKID_TWISTR:
            axisIndex  = STDCONTROL_JOYSTICK_AXIS_RZ;
            flags = STDCONTROL_AID_POSITIVE_AXIS;
            break;
        default:
            axisIndex = -1;
            break;
    }

    if ( axisIndex == -1 )
    {
        // Bind joystick button
        // Note, controlID has to be unpacked to button number due to binding cid to specific joystick
        buttonIdx = STDCONTROL_JOYSTICK_GETBUTTONINDEX(controlId);
        for ( joyNum = 0; joyNum < numJoysticks; ++joyNum )
        {
            sithControl_BindControl(functionId, STDCONTROL_JOYSTICK_GETBUTTON(joyNum, buttonIdx), (SithControlBindFlag)0);
        }
    }
    else
    {
        for ( joyNum = 0; joyNum < numJoysticks; ++joyNum )
        {
            sithControl_BindAxis(functionId, STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisIndex) | flags, (SithControlBindFlag)0);
            if ( functionId == SITHCONTROL_FORWARD && !stdControl_IsGamePad(joyNum) )
            {
                flags |= STDCONTROL_AID_LOW_SENSITIVITY;
                functionId = SITHCONTROL_RUNFWD;
                sithControl_BindAxis(functionId, STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisIndex) | flags, (SithControlBindFlag)0);
            }
        }
    }
}

HFONT J3DAPI jonesConfig_InitDialog(HWND hWnd, HFONT hFont, int dlgID)
{
    JonesDialogFontInfo fontData;
    float curFontScale; // Retail bug: uninitialised when hFont is passed in

    JonesDisplaySettings* pCurSettings = JonesMain_GetDisplaySettings();
    if ( pCurSettings )
    {
        fontData.bWindowMode = pCurSettings->bWindowMode;
    }
    else
    {
        fontData.bWindowMode = 0;
    }

    if ( !hFont )
    {
        fontData.hFont = jonesConfig_CreateDialogFont(hWnd, fontData.bWindowMode, (uint16_t)dlgID, &curFontScale);
    }
    else
    {
        fontData.hFont = hFont;
    }

    fontData.dialogID = (dlgID & 0xFFFF0000) >> 16;
    if ( !fontData.dialogID )
    {
        fontData.dialogID = dlgID & 0xFFFF;
    }

    fontData.hControlFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
    fontData.fontScaleX = curFontScale;
    fontData.fontScaleY = curFontScale;
    jonesConfig_SetDialogTitleAndPosition(hWnd, &fontData);

    fontData.dialogID = dlgID; // Use the original dlgID with potential NOFONTSCALE mask
    EnumChildWindows(hWnd, jonesConfig_SetPositionAndTextCallback, (LPARAM)&fontData);
    return fontData.hFont;
}

HFONT J3DAPI jonesConfig_CreateDialogFont(HWND hWnd, int bWindowMode, int dlgID, float* pFontScale)
{
    float fontScale;
    const char* pFontStr = NULL;
    HFONT font;
    LOGFONT logFont;
    uint32_t screenHeight;
    RECT rcDlg;
    int fontHeight;
    int k;
    float fontHeightf;
    int baseHeight;
    float cHeightRound;
    uint32_t scrwidth;

    if ( !hWnd )
    {
        return NULL;
    }

    GetClientRect(hWnd, &rcDlg);
#line 2739
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "height: %i, width:%i\n", rcDlg.bottom, rcDlg.right, 0, 0);

    pFontStr = jonesString_GetString("JONES_STR_FONT");
    if ( pFontStr )
    {
    GetObject((HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0), sizeof(LOGFONT), &logFont);

    // Retail bug: scale stays uninitialised when the dialog has no reference height
    baseHeight = 0;
    for ( k = 0; k < (int)STD_ARRAYLEN(jonesConfig_aDialogSizes); ++k )
    {
        if ( jonesConfig_aDialogSizes[k].dialogID == dlgID )
        {
            baseHeight = jonesConfig_aDialogSizes[k].refHeight;
            break;
        }
    }

    if ( baseHeight > 0 )
    {
        fontScale = (float)baseHeight / rcDlg.bottom;
    }

    if ( !bWindowMode && JonesMain_HasStarted() )
    {
        stdDisplay_GetBackBufferSize(&scrwidth, &screenHeight);
        fontHeight = -MulDiv(8, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72);

        if ( screenHeight < 480 && scrwidth < 640 )
        {
            if ( fontScale <= 1.0f )
            {
                fontScale *= (float)((double)screenHeight / 480.0);
                fontHeightf = fontHeight * fontScale;
            }
            else
            {
                fontHeightf = fontHeight * ((double)screenHeight / 480.0);
            }
        }
        else
        {
            fontHeightf = fontHeight * fontScale;
        }
    }
    else
    {
        RECT rectDskt;

        fontHeight = logFont.lfHeight;
        GetWindowRect(GetDesktopWindow(), &rectDskt);
        screenHeight = rectDskt.bottom - rectDskt.top;
        if ( screenHeight < 480 )
        {
            SystemParametersInfo(SPI_GETWORKAREA, 0, &rectDskt, 0);
            screenHeight = screenHeight - (screenHeight - rectDskt.bottom);
        }

        if ( fontScale <= 1.0f )
        {
            // Retail bug: scrwidth is not set on this path
            if ( screenHeight < 480 && scrwidth < 640 )
            {
                fontScale *= (float)((double)screenHeight / 480.0);
            }

            fontHeightf = logFont.lfHeight * fontScale;
        }
        else
        {
            fontHeightf = (float)(logFont.lfHeight > (int)(logFont.lfHeight * ((double)screenHeight / 480.0)) ? logFont.lfHeight : (int)(logFont.lfHeight * ((double)screenHeight / 480.0)));
        }
    }

    cHeightRound = (float)floor(fontHeightf);
    if ( fabs(cHeightRound - fontHeightf) < 0.5 )
    {
        fontHeightf = cHeightRound;
    }

    fontHeight = fontHeight > (int)fontHeightf ? fontHeight : (int)fontHeightf;
    fontHeight = fontHeight > -9 ? -9 : fontHeight;

    if ( !bWindowMode && JonesMain_HasStarted() )
    {
        if ( fontScale <= 1.0f || screenHeight > 480 && scrwidth > 640 )
        {
            *pFontScale = fontScale;
        }
        else
        {
            *pFontScale = D3DVAL((double)screenHeight / 480.0);
        }
    }
    else
    {
        *pFontScale = fontScale;
    }

    font = CreateFont(fontHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY, FF_DONTCARE, pFontStr);
    }

    return font; // Retail bug: uninitialised when JONES_STR_FONT is missing
}

void J3DAPI jonesConfig_ResetDialogFont(HWND hWndParent, HFONT hFont)
{
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( !hFont || (pSettings && pSettings->bWindowMode) )
    {
        return;
    }

    EnumChildWindows(hWndParent, jonesConfig_ResetWindowFontCallback, 0);
    DeleteObject((HGDIOBJ)hFont);
    hFont = NULL;
}

BOOL CALLBACK jonesConfig_ResetWindowFontCallback(HWND hWnd, LPARAM lparam)
{
    HFONT hfont;

    hfont = (HFONT)GetWindowLong(hWnd, GWL_USERDATA);
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hfont, 0);  // 0 - don't repaint
    return TRUE;
}

BOOL CALLBACK jonesConfig_SetPositionAndTextCallback(HWND hCtrl, LPARAM lparam)
{
    LPCSTR pText = NULL;
    JonesDialogFontInfo* pFontInfo;
    CHAR aText[256];
    CHAR aClassName[256];

    // Function resize control by scale, sets font and 
    // replaces all text of control from JONES_STR_* to corresponding text

    pFontInfo = (JonesDialogFontInfo*)lparam;
    if ( pFontInfo && pFontInfo->hFont )
    {
        // Scale and set font
        jonesConfig_SetWindowFontAndPosition(hCtrl, pFontInfo);
    }

    if ( pFontInfo->dialogID == 154 || pFontInfo->dialogID == 159 ) // Load/Save dialogs
    {
        return TRUE;
    }

    // Replace JONES_STR_* strings to corresponding text

    memset(aClassName, 0, sizeof(aClassName));
    GetClassName(hCtrl, aClassName, 256);

    memset(aText, 0, sizeof(aText));
    if ( strneqi(aClassName, "BUTTON", STD_ARRAYLEN(aClassName)) )
    {
        GetWindowText(hCtrl, aText, STD_ARRAYLEN(aText));
        pText = jonesString_GetString(aText);
        if ( pText )
        {
            SetWindowText(hCtrl, pText);
        }

        return TRUE;
    }
    else if ( strneqi(aClassName, "STATIC", STD_ARRAYLEN(aClassName)) )
    {
        GetWindowText(hCtrl, aText, STD_ARRAYLEN(aText));
        if ( strchr(aText, '%') )
        {
            return TRUE;
        }

        pText = jonesString_GetString(aText);
        if ( pText )
        {
            SetWindowText(hCtrl, pText);
        }

        return TRUE;
    }

    return TRUE;
}

void J3DAPI jonesConfig_SetWindowFontAndPosition(HWND hCtrl, JonesDialogFontInfo* pFontInfo)
{
    RECT rectDesktop;
    RECT rectWindow;
    RECT rectClient;
    RECT rectDlgWindow;

    int heightDesktop;
    int widthDesktop;

    HWND hParent = GetParent(hCtrl);

    HICON hPrevIcon = NULL;
    HWND hwndResumeBtn = NULL;
    HWND hwndThumbnail = NULL;
    HWND dlgItem = NULL;
    int offsetY = 0;

    // Set font
    if ( pFontInfo->hFont )
    {
        SendMessage(hCtrl, WM_SETFONT, (WPARAM)pFontInfo->hFont, 1); // 1 - repaint
        if ( HIWORD(pFontInfo->dialogID) != JONESCONFIG_NOFONTSCALEMASK )
        {
            SetWindowLong(hCtrl, GWL_USERDATA, (LONG)pFontInfo->hControlFont);
        }
    }

    GetWindowRect(GetDesktopWindow(), &rectDesktop);
    heightDesktop = rectDesktop.bottom - rectDesktop.top;
    widthDesktop  = rectDesktop.right - rectDesktop.left;

    GetWindowRect(hCtrl, &rectWindow);
    GetClientRect(hCtrl, &rectClient);
    GetWindowRect(hParent, &rectDlgWindow);

    if ( pFontInfo->bWindowMode && pFontInfo->fontScaleX >= 1.0f && pFontInfo->fontScaleY >= 1.0f )
    {
        return;
    }

    if ( HIWORD(pFontInfo->dialogID) == JONESCONFIG_NOFONTSCALEMASK )
    {
        LONG dlgID = (LONG)GetWindowLong(hCtrl, GWL_USERDATA);
        HWND dlgItem1137 = GetDlgItem(hParent, 1137);
        if ( dlgID == 159 || dlgID == 154 )     // load/save dialogs
        {
            SendMessage(hCtrl, WM_SETFONT, (WPARAM)pFontInfo->hControlFont, 1);// 1 - marks redraw
            return;
        }

        if ( dlgItem1137 ) // note, no dlg item with this ID in resources
        {
            CHAR aClassName[256];
            memset(aClassName, 0, sizeof(aClassName));

            GetClassName(hCtrl, aClassName, 256);
            if ( dlgItem1137 != hCtrl && strcmp(aClassName, "ToolbarWindow32") )
            {
                RECT rectDlgItem1137Window;
                GetWindowRect(dlgItem1137, &rectDlgItem1137Window);
                offsetY = (int)D3DMultiply(rectDlgItem1137Window.bottom - rectDlgItem1137Window.top, 1.0f - pFontInfo->fontScaleX);
            }
        }
        else
        {
            return;
        }
    }

    hPrevIcon = (HICON)SendMessage(hCtrl, STM_GETICON, 0, 0);
    dlgItem = GetDlgItem(hParent, 1182);
    if ( dlgItem == hCtrl )
    {
        hPrevIcon = (HICON)dlgItem;
    }

    if ( pFontInfo->dialogID == 159 || pFontInfo->dialogID == 154 )// Load/Save dialog
    {
        HWND hwndParent;

        LONG dlgID =(LONG)GetWindowLong(hCtrl, GWL_USERDATA); // TODO: UNUSED

        hwndParent = GetParent(hParent);
        if ( hwndParent )
        {
            HWND dlgItem1137 = GetDlgItem(hwndParent, 1137);
            if ( dlgItem1137 && dlgItem1137 != hCtrl )
            {
                RECT rectDlgItem1137Window;
                GetWindowRect(dlgItem1137, &rectDlgItem1137Window);
                offsetY = (int)D3DMultiply(rectDlgItem1137Window.bottom - rectDlgItem1137Window.top, 1.0f - pFontInfo->fontScaleX);
            }
        }

        if ( hCtrl == GetDlgItem(hParent, 1120) )
        {
            hwndResumeBtn = hCtrl;
        }
        if ( hCtrl == GetDlgItem(hParent, 1163) || hCtrl == GetDlgItem(hParent, 1125) )
        {
            hwndThumbnail = hCtrl; // Load / Save thumbnail
            offsetY = 0;
        }
    }

    if ( (!hPrevIcon || hPrevIcon && heightDesktop < 480 && widthDesktop < 640) && !hwndThumbnail )
    {
        float fontScaleX = pFontInfo->fontScaleX;
        if ( hPrevIcon && heightDesktop < 480 && widthDesktop < 640 )
        {
            fontScaleX = D3DVAL(heightDesktop / 480.0);
        }

        rectWindow.bottom = rectWindow.top + (LONG)((rectWindow.bottom - rectWindow.top) * fontScaleX);
    }

    if ( (!hPrevIcon || hPrevIcon && heightDesktop < 480 && widthDesktop < 640) && !hwndThumbnail && !hwndResumeBtn )
    {
        float fontScaleY = pFontInfo->fontScaleY;
        if ( hPrevIcon && heightDesktop < 480 && widthDesktop < 640 )
        {
            fontScaleY = D3DVAL(widthDesktop / 640.0);
        }

        rectWindow.right = rectWindow.left + (LONG)((rectWindow.right - rectWindow.left) * fontScaleY);
    }

    if ( hParent )
    {
        int posX;
        int posY;
        int dDlgWinX;
        int dDlgWinY;
        int dlgWidth;
        int dlgHeight;

        GetWindowRect(hParent, &rectDlgWindow);
        jonesConfig_GetWindowScreenRect(hParent, &rectClient);

        posX = (int)((rectWindow.left - rectClient.left) * pFontInfo->fontScaleY);
        posY = (int)((rectWindow.top - rectClient.top) * pFontInfo->fontScaleX);
        if ( offsetY > 0 && !hwndThumbnail )
        {
            posY += offsetY;
        }

        // Resize and reposition control
        MoveWindow(hCtrl, posX, posY, rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, /*bRepaint=*/TRUE);

        GetWindowRect(hCtrl, &rectWindow);
        GetClientRect(hCtrl, &rectClient);
        if ( rectWindow.bottom > rectDlgWindow.bottom || rectWindow.right > rectDlgWindow.right )
        {
            dDlgWinX = rectWindow.right - rectDlgWindow.right;
            dDlgWinY = rectWindow.bottom - rectDlgWindow.bottom;

            dlgWidth = rectDlgWindow.right - rectDlgWindow.left;
            dlgHeight = rectDlgWindow.bottom - rectDlgWindow.top;

            // Retail is a flat else-if chain: the middle arm re-tests dDlgWinY (0x00404A22)
            if ( dDlgWinY > 0 && dDlgWinX > 0 )
            {
                MoveWindow(hParent, rectDlgWindow.left, rectDlgWindow.top, dlgWidth + dDlgWinX, dlgHeight + dDlgWinY, 1);
            }
            else if ( dDlgWinY > 0 )
            {
                MoveWindow(hParent, rectDlgWindow.left, rectDlgWindow.top, dlgWidth, dlgHeight + dDlgWinY, 1);
            }
            else
            {
                MoveWindow(hParent, rectDlgWindow.left, rectDlgWindow.top, dlgWidth + dDlgWinX, dlgHeight, 1);
            }
        }
    }
}

void J3DAPI jonesConfig_GetWindowScreenRect(HWND hWnd, LPRECT lpRect)
{
    GetClientRect(hWnd, lpRect);

    {
    POINT point;

    point.x = point.y = 0;
    ClientToScreen(hWnd, &point);
    lpRect->top  = point.y;
    lpRect->left = point.x;

    point.x = lpRect->right;
    point.y = lpRect->bottom;
    ClientToScreen(hWnd, &point);

    lpRect->right  = point.x;
    lpRect->bottom = point.y;
}
}

void J3DAPI jonesConfig_SetDialogTitleAndPosition(HWND hWnd, JonesDialogFontInfo* pDlgFontInfo)
{
    RECT rectDeskto;
    uint32_t width, height;
    RECT rectClient;
    RECT rectWindow;
    RECT rectParent;
    RECT rectClientScreen;
    int32_t scaledClientWidth;
    int32_t scaledClientHeight;
    LONG winWidth;
    LONG winHeight;
    CHAR aTitleJS[256];
    LPCSTR pTitle;

    if ( pDlgFontInfo->dialogID != 154 && pDlgFontInfo->dialogID != 159 && pDlgFontInfo->dialogID != JONESCONFIG_NOFONTSCALEMASK )// if not load/save dialogs
    {
        memset(aTitleJS, 0, sizeof(aTitleJS));
        GetWindowText(hWnd, aTitleJS, STD_ARRAYLEN(aTitleJS));
        if ( strlen(aTitleJS) > 0 )
        {
            pTitle = jonesString_GetString(aTitleJS);
            if ( pTitle )
            {
                SetWindowText(hWnd, pTitle);
            }
        }
    }

    GetWindowRect(GetDesktopWindow(), &rectDeskto);

    height = rectDeskto.bottom - rectDeskto.top;
    width  = rectDeskto.right - rectDeskto.left;

    if ( pDlgFontInfo->bWindowMode && pDlgFontInfo->fontScaleX >= 1.0f && pDlgFontInfo->fontScaleY >= 1.0f )
    {
        return;
    }

    GetWindowRect(hWnd, &rectWindow);
    GetClientRect(hWnd, &rectClient);

    if ( !pDlgFontInfo->bWindowMode && JonesMain_HasStarted() )
    {
        stdDisplay_GetBackBufferSize(&width, &height);
    }

    if ( pDlgFontInfo->dialogID == 154 || pDlgFontInfo->dialogID == 159 )// load/save dialogs
    {
        scaledClientWidth  = rectClient.right + (int32_t)((1.0 - pDlgFontInfo->fontScaleY) * SITHSAVEGAME_THUMB_WIDTH);
        scaledClientHeight = rectClient.bottom + (int32_t)((1.0 - pDlgFontInfo->fontScaleX) * SITHSAVEGAME_THUMB_HEIGHT);

        winWidth  = rectWindow.right - rectWindow.left - (rectClient.right - scaledClientWidth);
        winHeight = rectWindow.bottom - rectWindow.top - (rectClient.bottom - scaledClientHeight);

        jonesConfig_GetWindowScreenRect(GetParent(hWnd), &rectParent);

        jonesConfig_GetWindowScreenRect(hWnd, &rectClientScreen);

        rectWindow.top    = rectClientScreen.top - rectParent.top;
        rectWindow.left   = rectClientScreen.left - rectParent.left;
        rectWindow.bottom = rectWindow.top + winHeight;
        rectWindow.right  = rectWindow.left + winWidth;
    }
    else
    {
        scaledClientWidth  = (int32_t)((double)rectClient.right * pDlgFontInfo->fontScaleY);
        scaledClientHeight = (int32_t)((double)rectClient.bottom * pDlgFontInfo->fontScaleX);
        if ( pDlgFontInfo->dialogID == JONESCONFIG_NOFONTSCALEMASK )
        {
            scaledClientWidth += (int32_t)((1.0 - pDlgFontInfo->fontScaleY) * SITHSAVEGAME_THUMB_WIDTH);
            scaledClientHeight += (int32_t)((1.0 - pDlgFontInfo->fontScaleX) * SITHSAVEGAME_THUMB_HEIGHT);
        }

        winWidth = rectWindow.right - rectWindow.left - (rectClient.right - scaledClientWidth);
        winHeight = rectWindow.bottom - rectWindow.top - (rectClient.bottom - scaledClientHeight);

        rectWindow.top    = (LONG)(height - winHeight) / 2;
        rectWindow.left   = (LONG)(width - winWidth) / 2;
        rectWindow.bottom = rectWindow.top + winHeight;
        rectWindow.right  = rectWindow.left + winWidth;
    }

    MoveWindow(hWnd, rectWindow.left, rectWindow.top, rectWindow.right - rectWindow.left, rectWindow.bottom - rectWindow.top, 1);
}

int J3DAPI jonesConfig_GetSaveGameFilePath(HWND hWnd, char* pOutFilePath)
{
    uint32_t width, height;
    OPENFILENAME ofn;
    RECT rect;
    char aFilterStr[128];
    CHAR aCurDir[128];
    SaveGameDialogData dialogData;
    char aFileTitle[JONESCONFIG_GAMESAVE_FILEPATHSIZE];
    char aFilename[JONESCONFIG_GAMESAVE_FILEPATHSIZE];
    char aFileterStr[512];
    char aSaveGameStr[512];
    const char* pFilterStr;
    const char* pSaveGMStr;
    BOOL res;
    HINSTANCE hInstance;
    const char* pExt;

    // Retail calls this unconditionally before any string setup, homed on
    // the stack for the whole function.
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();

    pExt = "nds";
    pFilterStr = jonesString_GetString("JONES_STR_FILTER");
    if ( pFilterStr )
    {
        strcpy(aFileterStr, pFilterStr);
    }
    else
    {
        memset(aFileterStr, 0, sizeof(aFileterStr));
    }

    pSaveGMStr = jonesString_GetString("JONES_STR_SAVEGM");
    if ( pSaveGMStr )
    {
        strcpy(aSaveGameStr, pSaveGMStr);
    }
    else
    {
        memset(aSaveGameStr, 0, sizeof(aSaveGameStr));
    }

    if ( !aFileterStr[0] || !aSaveGameStr[0] )
    {
        return 2;
    }

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

    memset(aCurDir, 0, sizeof(aCurDir));
    GetCurrentDirectory(STD_ARRAYLEN(aCurDir), aCurDir);

    memset(&dialogData, 0, sizeof(dialogData));
    dialogData.hThumbnail = NULL;
    memset(&ofn, 0, sizeof(ofn));
    memset(aFileTitle, 0, sizeof(aFileTitle));
    memset(aFilterStr, 0, sizeof(aFilterStr));

    sprintf(aFilterStr, aFileterStr, "(.nds)\0*.nds");

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hWnd;
    ofn.hInstance         = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    ofn.lpstrFilter       = aFilterStr;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = aFilename;
    ofn.lpstrFile[0]      = 0;
    ofn.nMaxFile          = STD_ARRAYLEN(aFilename);
    ofn.lpstrFileTitle    = aFileTitle;
    ofn.nMaxFileTitle     = STD_ARRAYLEN(aFileTitle);
    ofn.lpstrInitialDir   = sithGetSaveGamesDir();
    ofn.lpstrTitle        = aSaveGameStr;
    ofn.Flags             = OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_NOVALIDATE | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_HIDEREADONLY;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = pExt;
    ofn.lCustData         = (LPARAM)&dialogData;
    ofn.lpfnHook          = jonesConfig_SaveGameDialogHookProc;
    ofn.lpTemplateName    = MAKEINTRESOURCE(154);

    if ( pSettings && pSettings->bWindowMode )
    {
        GetWindowRect(GetDesktopWindow(), &rect);
        height = rect.bottom - rect.top;
        width  = rect.right - rect.left;
    }
    else
    {
        stdDisplay_GetBackBufferSize(&width, &height);
    }

    res = JonesDialog_ShowFileSelectDialog(&ofn, /*bOpen=*/0);
    SetCurrentDirectory(aCurDir);

    if ( res && pOutFilePath )
    {
        stdFnames_ChangeExt(ofn.lpstrFile, pExt);
        strncpy(pOutFilePath, ofn.lpstrFile, JONESCONFIG_GAMESAVE_FILEPATHSIZE - 1);
        pOutFilePath[JONESCONFIG_GAMESAVE_FILEPATHSIZE - 1] = '\0';
        return 1;
    }

    return 2;
}

UINT CALLBACK jonesConfig_SaveGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT result = 0; // Retail homes the proc result at [esp+0x10] and stores 0 before the dispatch

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_GameFileDialog_HandleWM_COMMAND(hDlg, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            result = jonesConfig_SaveGameDialogInit(hDlg, wParam, (LPOPENFILENAME)lParam);
            break;

        case WM_CLOSE:
            DeleteObject((HGDIOBJ)jonesConfig_hFontSaveGameDlg);
            jonesConfig_hFontSaveGameDlg = NULL;
            result = 0;
            break;

        case WM_NOTIFY:
        {
            OFNOTIFY* pOfNotify;
            SaveGameDialogData* pData;

            pOfNotify = (OFNOTIFY*)lParam;
            pData     = (SaveGameDialogData*)GetWindowLong(hDlg, DWL_USER);
            switch ( pOfNotify->hdr.code )
            {
                case CDN_INITDONE:
                    SetWindowLong(GetDlgItem(hDlg, 1125), GWL_USERDATA, 154);
                    SetWindowLong(GetDlgItem(hDlg, 1115), GWL_USERDATA, 154);

                    jonesConfig_hFontSaveGameDlg = jonesConfig_InitDialog(GetParent(hDlg), NULL, JONESCONFIG_NOFONTSCALE(154));
                    jonesConfig_hFontSaveGameDlg = jonesConfig_InitDialog(hDlg, jonesConfig_hFontSaveGameDlg, 154);
                    result = 1;
                    SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
                    break;

                case CDN_FILEOK:
                {
                    int bValidFile = 0;
                    char aFilename[128];
                    CHAR aPath[128];

                    if ( (int16_t)pOfNotify->lpOFN->nFileOffset > 0 )
                    {
                        size_t nFileOffset;
                        size_t nameLen;
                        char* pFilename;
                        LPSTR pFilePart;
                        int levelNum;
                        CHAR aSearchPath[128];
                        CHAR aExtension[128];
                        CHAR aCurDir[128];
                        char aQuckSaveFilename[128];

                        memset(aPath, 0, sizeof(aPath));
                        memset(aSearchPath, 0, sizeof(aSearchPath));
                        memset(aExtension, 0, sizeof(aExtension));
                        memset(aFilename, 0, sizeof(aFilename));

                        sprintf(aExtension, ".%s", pOfNotify->lpOFN->lpstrDefExt);

                        CommDlg_OpenSave_GetSpec(GetParent(hDlg), aFilename, STD_ARRAYLEN(aFilename));

                        pFilename = strstr(pOfNotify->lpOFN->lpstrFile, aFilename);
                        if ( pFilename )
                        {
                            nFileOffset = pFilename - pOfNotify->lpOFN->lpstrFile;
                        }
                        else
                        {
                            nFileOffset = strlen(pOfNotify->lpOFN->lpstrFile);
                        }

                        strncpy(aSearchPath, pOfNotify->lpOFN->lpstrFile, nFileOffset);

                        // TODO: this might be used for file name instead of searching for it below?
                        SearchPath(aSearchPath, aFilename, aExtension, STD_ARRAYLEN(aPath), aPath, &pFilePart);

                        memset(aCurDir, 0, sizeof(aCurDir));
                        strcpy(aCurDir, aSearchPath);

                        pFilename = strrchr(pOfNotify->lpOFN->lpstrFile, '\\');
                        if ( pFilename )
                        {
                            nFileOffset = pFilename - pOfNotify->lpOFN->lpstrFile;
                        }
                        else
                        {
                            nFileOffset = strlen(pOfNotify->lpOFN->lpstrFile);
                        }

                        strncpy(aSearchPath, pOfNotify->lpOFN->lpstrFile, nFileOffset);

                        if ( !pOfNotify->lpOFN->nFileExtension )
                        {
                            pFilename = NULL;
                        }
                        else
                        {
                            pFilename = &pOfNotify->lpOFN->lpstrFile[pOfNotify->lpOFN->nFileExtension];
                            if ( !*pFilename )
                            {
                                pFilename = NULL;
                            }
                        }

                        if ( (SetCurrentDirectory(aSearchPath) || strlen(aPath) > 0) && (!pFilename || pFilename && streq(pFilename, pOfNotify->lpOFN->lpstrDefExt)) )
                        {
                            SetCurrentDirectory(aCurDir);
                            bValidFile = 1;
                        }

                        nameLen = strcspn(aFilename, "?*");
                        if ( nameLen != strlen(aFilename) )
                        {
                            bValidFile = 0;
                        }

                        for ( levelNum = 0; levelNum < JONESLEVEL_BONUSLEVELNUM; ++levelNum )
                        {
                            const char* pSaveName;
                            char aAutoSaveFilename[128];

                            pSaveName = sithGetLevelSaveFilename(levelNum);
                            if ( pSaveName )
                            {
                                memset(aAutoSaveFilename, 0, sizeof(aAutoSaveFilename));
                                sprintf(aAutoSaveFilename, "%s%s", sithGetAutoSaveFilePrefix(), pSaveName);
                                if ( !strcmpi(aFilename, aAutoSaveFilename) )
                                {
                                    bValidFile = 0;
                                    break;
                                }
                            }
                        }

                        memset(aQuckSaveFilename, 0, sizeof(aQuckSaveFilename));
                        sprintf(aQuckSaveFilename, "%s.%s", sithGetQuickSaveFilePrefix(), pOfNotify->lpOFN->lpstrDefExt);
                        if ( streqi(aFilename, aQuckSaveFilename) )
                        {
                            bValidFile = 0;
                        }
                    }

                    if ( (int16_t)pOfNotify->lpOFN->nFileOffset < 0 || !bValidFile )
                    {
                        char* pFormat = (char*)jonesString_GetString("JONES_STR_INVALIDFILE");
                        if ( pFormat )
                        {
                            char aErrorText[256];
                            memset(aErrorText, 0, sizeof(aErrorText));
                            memset(aFilename, 0, sizeof(aFilename));

                            CommDlg_OpenSave_GetSpec(GetParent(hDlg), aFilename, STD_ARRAYLEN(aFilename));
                            sprintf(aErrorText, pFormat, aFilename);

                            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_SAVEGM", aErrorText, 141);
                        }

                        SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                        result = 1;
                    }
                    else if ( strlen(aPath) > 0 )
                    {
                        strcpy(pData->aFilePath, aFilename);
                        stdFnames_ChangeExt(pData->aFilePath, pOfNotify->lpOFN->lpstrDefExt);

                        // Retail bug: passes the dialog data itself, not its aFilePath, as the file path
                        if ( jonesConfig_ShowOverwriteSaveGameDlg(hDlg, (const char*)pData) != 1 )
                        {
                            SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                            result = 1;
                        }
                        else
                        {
                            result = 0;
                        }
                    }

                    break;
                }
            }

            break;
        }

        case WM_PAINT:
        {
            SaveGameDialogData* pData = (SaveGameDialogData*)GetWindowLong(hDlg, DWL_USER); // debug build: read, never used
            UpdateWindow(GetDlgItem(hDlg, 1125));
            UpdateWindow(GetDlgItem(hDlg, 1115));
            result = 1;
            break;
        }

        case WM_DESTROY:
        {
            HDC* hdc = (HDC*)GetWindowLong(hDlg, DWL_USER);
            DeleteDC(*hdc);
            *hdc = NULL;
            break;
        }

        default:
            result = 0;
            break;
    }

    return result;
}

int J3DAPI jonesConfig_SaveGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME lpOfn)
{
    HWND hButton = NULL;
    LONG width   = SITHSAVEGAME_THUMB_WIDTH;
    LONG height  = SITHSAVEGAME_THUMB_HEIGHT;
    SaveGameDialogData* pData;
    HWND hThumbnail;
    RECT dlgWndRect;
    RECT thumbWndRect;
    RECT thumbRect;
    LONG thumbWidth;
    LONG thumbHeight;

    pData = (SaveGameDialogData*)lpOfn->lCustData;
    pData->hThumbnail = sithGamesave_CreateThumbnail();

    pData->hdcThumbnail = CreateCompatibleDC(GetDC(GetDlgItem(hDlg, 1125)));

    hThumbnail = GetDlgItem(hDlg, 1125); // ??
    pData->pfThumbnailProc = GetWindowLong(hThumbnail, GWL_WNDPROC);
    SetWindowLong(hThumbnail, GWL_WNDPROC, (LONG)jonesConfig_SaveGameThumbnailPaintProc);

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);

    GetWindowRect(hDlg, &dlgWndRect);
    GetWindowRect(hThumbnail, &thumbWndRect);
    GetClientRect(hThumbnail, &thumbRect);

    thumbWidth  = width - (thumbRect.right - thumbRect.left);
    thumbHeight = height - (thumbRect.bottom - thumbRect.top);
    if ( thumbWidth || thumbHeight )
    {
        MoveWindow(hDlg, dlgWndRect.left, dlgWndRect.top, dlgWndRect.right - dlgWndRect.left + thumbWidth, dlgWndRect.bottom - dlgWndRect.top, TRUE);
        MoveWindow(
            hThumbnail,
            thumbWndRect.left - dlgWndRect.left,
            thumbWndRect.top - dlgWndRect.top,
            thumbWndRect.right - thumbWndRect.left + thumbWidth,
            thumbWndRect.bottom - thumbWndRect.top + thumbHeight,
            TRUE
        );
    }

    hThumbnail = GetDlgItem(GetParent(hDlg), 1152);  // some other dlgItem
    SendMessage(hThumbnail, EM_LIMITTEXT, 60u, 0);
    return 1;
}

LRESULT CALLBACK jonesConfig_SaveGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent;
    SaveGameDialogData* pData;
    HDC hdc;
    int ctrlID;
    HBITMAP hThumbnail;
    PAINTSTRUCT ps;

    hwndParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);
    pData      = (SaveGameDialogData*)GetWindowLong(hwndParent, DWL_USER);
    switch ( uMsg )
    {
        case WM_PAINT:
            if ( pData )
            {
                hThumbnail = pData->hThumbnail;
                ctrlID     = 1125;
                if ( hThumbnail )
                {
                    hdc = BeginPaint(hWnd, &ps);
                    SetStretchBltMode(hdc, STRETCH_HALFTONE);
                    jonesConfig_DrawImageOnDialogItem(hwndParent, pData->hdcThumbnail, hdc, ctrlID, hThumbnail, NULL);
                    EndPaint(hWnd, &ps);
                }
            }

            return 0;
    }

    // Retail dereferences pData unguarded here
    return CallWindowProc((WNDPROC)pData->pfThumbnailProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_ShowOverwriteSaveGameDlg(HWND hWnd, const char* aFilePath)
{
    int result;
    GameSaveMsgBoxData data;

    memset(&data, 0, sizeof(data));
    data.dialogID     = 214;
    data.pNdsFilePath = aFilePath;
    result = JonesDialog_ShowDialog(MAKEINTRESOURCE(214), hWnd, jonesConfig_SaveGameMsgBoxProc, (LPARAM)&data);
    return result;
}

int J3DAPI jonesConfig_ShowExitGameDialog(HWND hWnd, char* pSaveGameFilePath)
{
    int result;
    GameSaveMsgBoxData data;

    memset(&data, 0, sizeof(data));
    data.dialogID = 211;
    data.pNdsFilePath = pSaveGameFilePath;
    result = JonesDialog_ShowDialog(MAKEINTRESOURCE(211), hWnd, jonesConfig_ExitGameDlgProc, (LPARAM)&data);
    return result;
}

BOOL CALLBACK jonesConfig_ExitGameDlgProc(HWND hWnd, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_MsgBoxDlg_HandleWM_COMMAND(hWnd, LOWORD(wparam), lparam, HIWORD(wparam));
            return 0;

        case WM_INITDIALOG:
        {
            RECT rectWndIcon;
            HWND hIcon;
            GameSaveMsgBoxData* pGSData = (GameSaveMsgBoxData*)lparam;

            RECT rectWnd;
            hIcon = GetDlgItem(hWnd, 1182);

            GetClientRect(hWnd, &rectWnd);
            jonesConfig_GetWindowScreenRect(hWnd, &rectWnd);

            GetWindowRect(hIcon, &rectWndIcon);
            MoveWindow(hIcon, rectWndIcon.top - rectWnd.top, rectWndIcon.left - rectWnd.left, 64, 64, TRUE);

            jonesConfig_hFontExitDlg = jonesConfig_InitDialog(hWnd, 0, pGSData->dialogID);

            bResult = jonesConfig_GameSaveSetData(hWnd, wparam, lparam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HWND hIcon;
            HDC hdc;
            HDC hmemdc;
            LONG userData;

            userData = GetWindowLong(hWnd, DWL_USER); // unused
            hIcon  = GetDlgItem(hWnd, 1182);
            hdc    = BeginPaint(hIcon, &ps);
            hmemdc = CreateCompatibleDC(hdc);
            SetStretchBltMode(hdc, STRETCH_HALFTONE);
            jonesConfig_DrawImageOnDialogItem(hWnd, hmemdc, hdc, 1182, jonesConfig_apDlgIcons[4], jonesConfig_apDlgIcons[5]); // exit and exitmask bmps
            DeleteObject(hmemdc); // retail and the debug build release the memory DC with DeleteObject
            EndPaint(hIcon, &ps);
            bResult = 0;
            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontExitDlg);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

BOOL CALLBACK jonesConfig_SaveGameMsgBoxProc(HWND hWnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( umsg )
    {
        case WM_COMMAND:
            jonesConfig_MsgBoxDlg_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
        {
            GameSaveMsgBoxData* pData = (GameSaveMsgBoxData*)lParam;
            jonesConfig_hFontGameSaveMsgBox = jonesConfig_InitDialog(hWnd, 0, pData->dialogID);
            bResult = jonesConfig_GameSaveSetData(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontGameSaveMsgBox);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_GameSaveSetData(HWND hDlg, int a2, LPARAM lParam)
{
    GameSaveMsgBoxData* pData = (GameSaveMsgBoxData*)lParam;

    {
    const char* pNdsFilePath;
    const char* pOverwriteText;
    const char* pCurrentProgressText;
    char aText[256];

    if ( !pData )
    {
        return 0;
    }

    switch ( pData->dialogID )
    {
        case 214: // if save game overwrite message box
            pNdsFilePath = pData->pNdsFilePath;
            pOverwriteText = jonesString_GetString("JONES_STR_OVERWRITE");
            if ( pOverwriteText )
            {
                memset(aText, 0, sizeof(aText));
                sprintf(aText, pOverwriteText, pNdsFilePath + 4);
                jonesConfig_SetTextControl(hDlg, GetDlgItem(hDlg, 1197), aText);
            }
            break;

        case 163: // if  load game message box
            pCurrentProgressText = jonesString_GetString("JONES_STR_PROGRESS1");
            if ( pCurrentProgressText )
            {
                jonesConfig_SetTextControl(hDlg, GetDlgItem(hDlg, 1126), pCurrentProgressText);
            }
            break;
    }

    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
    return 1;
}
}

void J3DAPI jonesConfig_MsgBoxDlg_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode)
{
    GameSaveMsgBoxData* pData = (GameSaveMsgBoxData*)GetWindowLong(hWnd, DWL_USER);

    switch ( nResult )
    {
        case 1187:
            if ( pData->dialogID == 211 && jonesConfig_GetSaveGameFilePath(hWnd, (char*)pData->pNdsFilePath) != 1 )// in exit dialoge save game button was clicked
            {
                EndDialog(hWnd, 2);
                break;
            }

        case 1:
        case 2:
            EndDialog(hWnd, nResult);
            break;
    }
}


int J3DAPI jonesConfig_GetLoadGameFilePath(HWND hWnd, char* pDestNdsPath)
{
    uint32_t width, height;
    OPENFILENAME ofn;
    RECT rect;
    char aLastFile[JONESCONFIG_GAMESAVE_FILEPATHSIZE];
    LoadGameDialogData data;
    char aFilterStr[128];
    char aCwd[128];
    char aFileTitle[JONESCONFIG_GAMESAVE_FILEPATHSIZE];
    char aFilterFormatStr[512];
    char aLoadGameStr[512];
    const char* pFilterStr;
    const char* pLoadGameStr;
    const char* pLastFile;
    BOOL bHasFilePath;
    const char* pExt;

    // Retail calls this unconditionally before any string setup and keeps
    // the result live (ebp) for the whole function.
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();

    pExt = "nds";
    pFilterStr = jonesString_GetString("JONES_STR_FILTER");
    if ( pFilterStr )
    {
        strcpy(aFilterFormatStr, pFilterStr);
    }
    else
    {
        memset(aFilterFormatStr, 0, sizeof(aFilterFormatStr));
    }

    pLoadGameStr = jonesString_GetString("JONES_STR_LOADGM");
    if ( pLoadGameStr )
    {
        strcpy(aLoadGameStr, pLoadGameStr);
    }
    else
    {
        memset(aLoadGameStr, 0, sizeof(aLoadGameStr));
    }

    if ( !aFilterFormatStr[0] || !aLoadGameStr[0] )
    {
        return 2;
    }

    memset(aCwd, 0, sizeof(aCwd));
    GetCurrentDirectory(STD_ARRAYLEN(aCwd), aCwd);

    memset(&data, 0, sizeof(data));
    data.hThumbnail = 0;
    memset(&ofn, 0, sizeof(ofn));
    memset(aFileTitle, 0, sizeof(aFileTitle));
    memset(aFilterStr, 0, sizeof(aFilterStr));

    sprintf(aFilterStr, aFilterFormatStr, " (.nds)\0*.nds");

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hWnd;
    ofn.hInstance         = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    ofn.lpstrFilter       = aFilterStr;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;

    pLastFile = sithGamesave_GetLastFilename();
    memset(aLastFile, 0, sizeof(aLastFile));
    if ( pLastFile )
    {
        strncpy(aLastFile, pLastFile, STD_ARRAYLEN(aLastFile) - 1);
        aLastFile[STD_ARRAYLEN(aLastFile) - 1] = '\0';
    }

    ofn.lpstrFile = aLastFile;
    if ( strlen(aLastFile) == 0 )
    {
        ofn.lpstrFile[0] = 0;
    }

    ofn.nMaxFile        = STD_ARRAYLEN(aLastFile);
    ofn.lpstrFileTitle  = aFileTitle;
    ofn.nMaxFileTitle   = STD_ARRAYLEN(aFileTitle);
    ofn.lpstrInitialDir = sithGetSaveGamesDir();
    ofn.lpstrTitle      = aLoadGameStr;
    ofn.Flags           = OFN_EXPLORER | OFN_NONETWORKBUTTON | OFN_NOVALIDATE | OFN_ENABLETEMPLATE | OFN_ENABLEHOOK | OFN_HIDEREADONLY | OFN_READONLY;
    ofn.nFileOffset     = 0;
    ofn.nFileExtension  = 0;
    ofn.lpstrDefExt     = pExt;
    ofn.lCustData       = (LPARAM)&data;
    ofn.lpfnHook        = jonesConfig_LoadGameDialogHookProc;
    ofn.lpTemplateName  = MAKEINTRESOURCE(159);

    if ( pSettings && pSettings->bWindowMode || !JonesMain_HasStarted() )
    {
        GetWindowRect(GetDesktopWindow(), &rect);
        height = rect.bottom - rect.top;
        width  = rect.right - rect.left;
    }
    else
    {
        stdDisplay_GetBackBufferSize(&width, &height);
    }

    bHasFilePath = JonesDialog_ShowFileSelectDialog((LPOPENFILENAME)&ofn, 1);
    SetCurrentDirectory(aCwd);
    if ( (bHasFilePath || data.unknown36) && pDestNdsPath )
    {
        if ( bHasFilePath )
        {
            stdFnames_ChangeExt(ofn.lpstrFile, pExt);
            strncpy(pDestNdsPath, ofn.lpstrFile, JONESCONFIG_GAMESAVE_FILEPATHSIZE - 1);
            pDestNdsPath[JONESCONFIG_GAMESAVE_FILEPATHSIZE - 1] = '\0';
        }
        else
        {
            stdFnames_ChangeExt(data.aFilePath, pExt);
            strncpy(pDestNdsPath, data.aFilePath, JONESCONFIG_GAMESAVE_FILEPATHSIZE - 1);
            pDestNdsPath[JONESCONFIG_GAMESAVE_FILEPATHSIZE - 1] = '\0';
        }

        return 1;
    }

    return 2;
}

void* J3DAPI jonesConfig_GameFileDialog_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode)
{
    // Retail pushes the full WM_COMMAND decode at both call sites (add esp,0x10)

    void* pData = (void*)GetWindowLong(hWnd, DWL_USER);
    switch ( nResult ) /* retail bug: no return value; the debug build is the same */
    {
    }
}

UINT CALLBACK jonesConfig_LoadGameDialogHookProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT result = 0; // Retail materialises the zero in a register before the dispatch (0x00405F7E)

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_GameFileDialog_HandleWM_COMMAND(hDlg, LOWORD(wParam), lParam, HIWORD(wParam));
            return FALSE;

        case WM_INITDIALOG:
            result = jonesConfig_LoadGameDialogInit(hDlg, wParam, (LPOPENFILENAME)lParam);
            break;

        case WM_CLOSE:
            DeleteObject(jonesConfig_hFontLoadGameDlg);
            jonesConfig_hFontLoadGameDlg = NULL;
            result = 0;
            break;

        case WM_NOTIFY:
        {
            OFNOTIFY* pOfNotify;
            LoadGameDialogData* pData;

            pOfNotify = (OFNOTIFY*)lParam;
            pData     = (LoadGameDialogData*)GetWindowLong(hDlg, DWL_USER);
#line 4118
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "code: %i\n", pOfNotify->hdr.code, 0, 0, 0);

            switch ( pOfNotify->hdr.code )
            {
                case CDN_INITDONE:
                    SetWindowLong(GetDlgItem(hDlg, 1120), GWL_USERDATA, 159);
                    SetWindowLong(GetDlgItem(hDlg, 1117), GWL_USERDATA, 159);
                    SetWindowLong(GetDlgItem(hDlg, 1163), GWL_USERDATA, 159);

                    jonesConfig_hFontLoadGameDlg = jonesConfig_InitDialog(GetParent(hDlg), NULL, JONESCONFIG_NOFONTSCALE(159));
                    jonesConfig_hFontLoadGameDlg = jonesConfig_InitDialog(hDlg, jonesConfig_hFontLoadGameDlg, 159);
                    result = 1;
                    SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
                    break;

                case CDN_FILEOK:
                {
                    int bValidFile = 0;
                    char aFilename[128];
                    CHAR aPath[128];

                    if ( (int16_t)pOfNotify->lpOFN->nFileOffset > 0 )
                    {
                        char* pStr;
                        size_t strLen;
                        size_t nameLen;
                        LPSTR lpFilePart;
                        CHAR aSearchPath[128];
                        char aExtension[128];

                        memset(aPath, 0, sizeof(aPath));
                        memset(aSearchPath, 0, sizeof(aSearchPath));
                        memset(aExtension, 0, sizeof(aExtension));
                        memset(aFilename, 0, sizeof(aFilename));

                        sprintf(aExtension, ".%s", pOfNotify->lpOFN->lpstrDefExt);

                        SendMessage(GetParent(hDlg), CDM_GETSPEC, STD_ARRAYLEN(aFilename), (LPARAM)aFilename); // get selected filename

                        pStr = strstr(pOfNotify->lpOFN->lpstrFile, aFilename);
                        if ( pStr )
                        {
                            strLen = pStr - pOfNotify->lpOFN->lpstrFile;
                        }
                        else
                        {
                            strLen = strlen(pOfNotify->lpOFN->lpstrFile);
                        }

                        strncpy(aSearchPath, pOfNotify->lpOFN->lpstrFile, strLen);

                        SearchPath(aSearchPath, aFilename, aExtension, STD_ARRAYLEN(aPath), aPath, &lpFilePart);
                        if ( strlen(aPath) > 0 )
                        {
                            bValidFile = 1;
                        }

                        nameLen = strcspn(aFilename, "?*");
                        if ( nameLen != strlen(aFilename) )
                        {
                            bValidFile = 0;
                        }
                    }

                    if ( (int16_t)pOfNotify->lpOFN->nFileOffset < 0 || !bValidFile )
                    {
                        const char* pErrorString = jonesString_GetString("JONES_STR_INVALIDFILE");
                        if ( pErrorString )
                        {
                            CHAR aMessage[256];
                            memset(aMessage, 0, sizeof(aMessage));

                            SendMessage(GetParent(hDlg), CDM_GETSPEC, STD_ARRAYLEN(aFilename), (LPARAM)aFilename);
                            sprintf(aMessage, pErrorString, aFilename);
                            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_LOADGM", aMessage, 139);
                        }

                        SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                        result = TRUE;
                        break;
                    }

                    if ( !strlen(aPath) )
                    {
                        const char* pErrorString = jonesString_GetString("JONES_STR_NOFILE");
                        if ( pErrorString )
                        {
                            CHAR aMessage[256];
                            memset(aMessage, 0, sizeof(aMessage));

                            SendMessage(GetParent(hDlg), CDM_GETSPEC, STD_ARRAYLEN(aFilename), (LPARAM)aFilename);
                            sprintf(aMessage, pErrorString, aFilename);
                            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_LOADGM", aMessage, 139);
                        }

                        SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                        result = TRUE;
                    }
                    else if ( JonesMain_HasStarted()
                        && (sithPlayer_pLocalPlayerThing->flags & SITH_TF_DYING) == 0
                        && (sithPlayer_pLocalPlayerThing->flags & SITH_TF_DESTROYED) == 0
                        && jonesConfig_ShowLoadGameWarningMsgBox(hDlg) != 1 )
                    {
                        SetWindowLong(hDlg, DWL_MSGRESULT, 1);
                        result = TRUE;
                    }
                    else
                    {
                        result = FALSE;
                    }

                    break;
                }

                case CDN_SELCHANGE:
                {
                    HWND hThumbnail = GetDlgItem(hDlg, 1163);
                    if ( !pData->bFolderSel )
                    {
                        const char* pExt = NULL;
                        SendMessage(GetParent(hDlg), CDM_GETFILEPATH, STD_ARRAYLEN(pData->aFilePath), (LPARAM)pData->aFilePath);
                        if ( strlen(pData->aFilePath) > 0 )
                        {
                            pExt = pData->aFilePath + strlen(pData->aFilePath) - 3;
                        }

                        if ( pExt && streq(pExt, "nds") )
                        {
                            // Load thumbnail of newly selected nds file
                            if ( pData->hThumbnail )
                            {
                                DeleteObject(pData->hThumbnail);
                                pData->hThumbnail = NULL;
                            }

                            pData->hThumbnail = sithGamesave_LoadThumbnail(pData->aFilePath);
                        }
                    }

                    InvalidateRect(hThumbnail, NULL, TRUE);
                    UpdateWindow(hDlg);
                    result = TRUE;
                    break;
                }

                case CDN_FOLDERCHANGE:
                    if ( pData->hThumbnail )
                    {
                        HWND hDlgItem = GetDlgItem(hDlg, 1163);
                        pData->bFolderSel = 1;

                        InvalidateRect(hDlgItem, NULL, TRUE);
                        UpdateWindow(hDlg);
                    }

                    result = TRUE;
                    break;
            }

            break;
        }

        case WM_PAINT:
        {
            LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLong(hDlg, DWL_USER);
            if ( !pData->bFolderSel )
            {
                UpdateWindow(GetDlgItem(hDlg, 1163));
            }

            UpdateWindow(GetDlgItem(hDlg, 1117));
            result = 1;
            break;
        }

        case WM_DESTROY:
        {
            LoadGameDialogData* pData = (LoadGameDialogData*)GetWindowLong(hDlg, DWL_USER);
            DeleteDC(pData->hdcThumbnail);
            DeleteObject(pData->hThumbnail);
            pData->hThumbnail   = NULL;
            pData->hdcThumbnail = NULL;
            break;
        }

        default:
            result = 0;
            break;
    }

    return result;
}

int J3DAPI jonesConfig_ShowLoadGameWarningMsgBox(HWND hWnd)
{
    int result;
    GameSaveMsgBoxData data;

    memset(&data, 0, sizeof(data));
    data.dialogID = 163;
    result = JonesDialog_ShowDialog(MAKEINTRESOURCE(163), hWnd, jonesConfig_SaveGameMsgBoxProc, (LPARAM)&data);
    return result;
}

BOOL J3DAPI jonesConfig_LoadGameDialogInit(HWND hDlg, int a2, LPOPENFILENAME pofn)
{
    HWND hButton = NULL;
    LONG width   = SITHSAVEGAME_THUMB_WIDTH;
    LONG height  = SITHSAVEGAME_THUMB_HEIGHT;
    LoadGameDialogData* pData;
    RECT btnRect;
    HWND hThumbnail;
    RECT dlgWinRect;
    RECT thumbWinRect;
    RECT thumbRect;
    LONG thmbWidth;
    LONG thmbHeight;
    char aPath[128];

    pData = (LoadGameDialogData*)pofn->lCustData;

    hButton = GetDlgItem(hDlg, 1120); // load last saved game button
    GetWindowRect(hButton, &btnRect);

    pData->hdcThumbnail = CreateCompatibleDC(GetDC(GetDlgItem(hDlg, 1163)));

    hThumbnail = GetDlgItem(hDlg, 1163);
    pData->pfThubnailProc = GetWindowLong(hThumbnail, GWL_WNDPROC);
    SetWindowLong(hThumbnail, GWL_WNDPROC, (LONG)jonesConfig_LoadGameThumbnailPaintProc);
    SetWindowLong(hDlg, DWL_USER, (LONG)pData);

    GetWindowRect(hDlg, &dlgWinRect);
    GetWindowRect(hThumbnail, &thumbWinRect);
    GetClientRect(hThumbnail, &thumbRect);

    thmbWidth  = width - (thumbRect.right - thumbRect.left);
    thmbHeight = height - (thumbRect.bottom - thumbRect.top);
    if ( thmbWidth || thmbHeight )
    {
        MoveWindow(hDlg, dlgWinRect.left, dlgWinRect.top, dlgWinRect.right - dlgWinRect.left + thmbWidth, dlgWinRect.bottom - dlgWinRect.top, TRUE);

        MoveWindow(
            hThumbnail,
            thumbWinRect.left - dlgWinRect.left,
            thumbWinRect.top - dlgWinRect.top,
            thumbWinRect.right - thumbWinRect.left + thmbWidth,
            thumbWinRect.bottom - thumbWinRect.top + thmbHeight,
            TRUE
        );

        MoveWindow(
            hButton,
            thumbWinRect.left - dlgWinRect.left,
            btnRect.top - dlgWinRect.top + thmbHeight,
            thumbWinRect.right - thumbWinRect.left + thmbWidth,
            btnRect.bottom - btnRect.top,
            TRUE
        );

        // TODO: dead code as the button is not hooked to any code
        memset(aPath, 0, sizeof(aPath));
        jonesConfig_LoadGameGetLastSavedGamePath(aPath, sizeof(aPath));
        if ( !strlen(aPath) )
        {
            EnableWindow(hButton, 0); // close window if no game was saved yet
        }
    }

    return TRUE;
}

LRESULT CALLBACK jonesConfig_LoadGameThumbnailPaintProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent;
    LoadGameDialogData* pData;
    HDC hdc;
    int ctrlID;
    HBITMAP hThumbnail;
    PAINTSTRUCT ps;

    hwndParent = (HWND)GetWindowLong(hWnd, GWL_HWNDPARENT);
    pData      = (LoadGameDialogData*)GetWindowLong(hwndParent, DWL_USER);
    switch ( uMsg )
    {
        case WM_PAINT:
            if ( pData )
            {
                hThumbnail = pData->hThumbnail;
                ctrlID     = 1163; // thumbnail picture edit control
                if ( hThumbnail )
                {
                    if ( pData->bFolderSel )
                    {
                        pData->bFolderSel = 0;
                        if ( pData->hThumbnail )
                        {
                            DeleteObject((HGDIOBJ)pData->hThumbnail);
                            pData->hThumbnail = NULL;
                        }

                        return 1;
                    }

                    hdc = BeginPaint(hWnd, &ps);
                    SetStretchBltMode(hdc, STRETCH_HALFTONE);
                    jonesConfig_DrawImageOnDialogItem(hwndParent, pData->hdcThumbnail, hdc, ctrlID, hThumbnail, NULL);
                    EndPaint(hWnd, &ps);
                }
            }

            return 0;
    }

    return CallWindowProc((WNDPROC)pData->pfThubnailProc, hWnd, uMsg, wParam, lParam);
}

int J3DAPI jonesConfig_ShowGamePlayOptions(HWND hWnd)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    int dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(112), hWnd, jonesConfig_GamePlayOptionsProc, 0);
    return dlgResult;
}

BOOL CALLBACK jonesConfig_GamePlayOptionsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_GamePlayOptions_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontGamePlayOptionsDlg = jonesConfig_InitDialog(hWnd, NULL, 112);
            bResult = jonesConfig_GamePlayOptionsInitDlg(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_HSCROLL:
        {
            HWND hCtrl;
            int difficulty;
            const char* pDifficultyStr = NULL;

            jonesConfig_HandleWM_HSCROLL(hWnd, (HWND)lParam, LOWORD(wParam), (short)HIWORD(wParam));

            hCtrl = GetDlgItem(hWnd, 1050);   // Difficulty slider control
            difficulty = SendMessage(hCtrl, TBM_GETPOS, 0, 0);
            hCtrl = GetDlgItem(hWnd, 1215); // Difficulty text control
            switch ( difficulty )
            {
                case 0:
                    pDifficultyStr = jonesString_GetString("JONES_STR_HARD0");
                    break;

                case 1:
                    pDifficultyStr = jonesString_GetString("JONES_STR_HARD1");
                    break;

                case 2:
                    pDifficultyStr = jonesString_GetString("JONES_STR_HARD2");
                    break;

                case 3:
                    pDifficultyStr = jonesString_GetString("JONES_STR_HARD3");
                    break;

                case 4:
                    pDifficultyStr = jonesString_GetString("JONES_STR_HARD4");
                    break;

                case 5:
                    pDifficultyStr = jonesString_GetString("JONES_STR_HARD5");
                    break;

                default:
                    break;
            }

            if ( pDifficultyStr )
            {
                SetWindowText(hCtrl, pDifficultyStr);
            }

            bResult = 1;
            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontGamePlayOptionsDlg);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

void J3DAPI jonesConfig_HandleWM_HSCROLL(HWND hDlg, HWND hWnd, UINT sbValue, int nPos)
{
    int posNew = -1;
    int pageSize;
    int maximumPos;
    int posMin;
    int currentPosition;

    maximumPos   = SendMessage(hWnd, TBM_GETRANGEMAX, 0, 0);
    posMin   = SendMessage(hWnd, TBM_GETRANGEMIN, 0, 0);
    currentPosition   = SendMessage(hWnd, TBM_GETPOS, 0, 0);
    pageSize = SendMessage(hWnd, TBM_GETPAGESIZE, 0, 0);  // ???

    switch ( sbValue )
    {
        case SB_RIGHT:
            posNew = maximumPos;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            posNew = currentPosition;
            break;

        case SB_LEFT:
            posNew = posMin;
            break;

        default:
            break;
    }

    if ( posNew >= 0 )
    {
        SendMessage(hWnd, TBM_SETPOS, /*redraw=*/1u, posNew);
    }
}

int J3DAPI jonesConfig_GamePlayOptionsInitDlg(HWND hDlg, int a2, LPARAM lParam)
{
    HWND hCtrl;
    const char* pDifficultyString = NULL;

    // Shot Text option
    hCtrl = GetDlgItem(hDlg, 1052);
    Button_SetCheck(hCtrl, sithVoice_GetShowText());

    // Show Hints option
    hCtrl = GetDlgItem(hDlg, 1051);
    Button_SetCheck(hCtrl, sithOverlayMap_GetShowHints());

    // RotateMap option
    hCtrl = GetDlgItem(hDlg, 1204);
    Button_SetCheck(hCtrl, sithOverlayMap_GetMapRotation());

    // Default to Run option
    CheckDlgButton(hDlg, 1202, jonesConfig_gamePlayOptions_bDefaultRun);// CB default run

    // Difficulty slider and text
    hCtrl = GetDlgItem(hDlg, 1050); // Difficulty slider control

    SendMessage(hCtrl, TBM_SETRANGE, 1u, (uint16_t)jonesInventory_GetMaxDifficultyLevel() << 16);// LWORD is min and HIWORD is max range

    SendMessage(hCtrl, TBM_SETPOS, 1u, sithGetGameDifficulty());

    SendMessage(hCtrl, TBM_SETTICFREQ, 1u, 0);
    SendMessage(hCtrl, TBM_SETPAGESIZE, 0, 1);

    // Difficulty text control
    hCtrl = GetDlgItem(hDlg, 1215);

    switch ( sithGetGameDifficulty() )
    {
        case 0:
            pDifficultyString = jonesString_GetString("JONES_STR_HARD0");
            break;

        case 1:
            pDifficultyString = jonesString_GetString("JONES_STR_HARD1");
            break;

        case 2:
            pDifficultyString = jonesString_GetString("JONES_STR_HARD2");
            break;

        case 3:
            pDifficultyString = jonesString_GetString("JONES_STR_HARD3");
            break;

        case 4:
            pDifficultyString = jonesString_GetString("JONES_STR_HARD4");
            break;

        case 5:
            pDifficultyString = jonesString_GetString("JONES_STR_HARD5");
            break;

        default:
            break;
    }

    if ( pDifficultyString )
    {
        SetWindowText(hCtrl, pDifficultyString);
    }

    return 1;
}

void J3DAPI jonesConfig_GamePlayOptions_HandleWM_COMMAND(HWND hDlg, int controlID, LPARAM lParam, int notifyCode)
{

    switch ( controlID )
    {
        case 1: // OK
            // Get & save show text option
            sithVoice_ShowText(Button_GetState(GetDlgItem(hDlg, 1052)));
            wuRegistry_SaveIntEx("Show Text", sithVoice_GetShowText());

            // Get & save map rotation option
            sithOverlayMap_EnableMapRotation(Button_GetState(GetDlgItem(hDlg, 1204)));
            wuRegistry_SaveIntEx("Map Rotation", sithOverlayMap_GetMapRotation());

            // Get & save show hint option
            sithOverlayMap_SetShowHints(Button_GetState(GetDlgItem(hDlg, 1051)));
            wuRegistry_SaveIntEx("Show Hints", sithOverlayMap_GetShowHints());

            // Get & save difficulty
            sithSetGameDifficulty(SendMessage(GetDlgItem(hDlg, 1050), TBM_GETPOS, 0, 0));
            wuRegistry_SaveInt("Difficulty", sithGetGameDifficulty());

            // Get & save default to run option
            jonesConfig_gamePlayOptions_bDefaultRun = Button_GetCheck(GetDlgItem(hDlg, 1202));
            wuRegistry_SaveIntEx("Default Run", jonesConfig_gamePlayOptions_bDefaultRun);

            if ( jonesConfig_gamePlayOptions_bDefaultRun )
            {
                sithControl_controlOptions |= 0x02;
            }
            else
            {
                sithControl_controlOptions &= ~0x02;
            }
            // Fall through: close the dialog like Cancel

        case 2: //  CANCEL
            EndDialog(hDlg, controlID);
            break;
    }
}

void J3DAPI jonesConfig_EnableMouseControl(int bEnable)
{
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();
    if ( bEnable )
    {
        sithControl_UnbindMouseAxes();
        sithControl_RegisterMouseBindings();
    }

    else if ( !pSettings->bWindowMode )
    {
        sithControl_UnbindMouseAxes();
        stdControl_EnableMouse(1);
    }
    else
    {
        stdControl_EnableMouse(0);
    }
}

void J3DAPI jonesConfig_FreeControlScheme(JonesControlsScheme* pScheme)
{
    int i;

    for ( i = 0; i < (int)STD_ARRAYLEN(pScheme->aActions); ++i )
    {
        if ( pScheme->aActions[i] )
        {
            memset(pScheme->aActions[i], 0, STD_ARRAYLEN(pScheme->aActions[i]));
        }
    }

    stdMemory_Free(pScheme);
    pScheme = NULL;
}

void J3DAPI jonesConfig_FreeControlConfigEntry(JonesControlsConfig* pConfig)
{
    int i;

    for ( i = 0; i < pConfig->numSchemes; ++i )
    {
        JonesControlsScheme* pScheme = &pConfig->aSchemes[i];
        int j;
        if ( pScheme )
        {
            for ( j = 0; j < (int)STD_ARRAYLEN(pScheme->aActions); ++j )
            {
                if ( pScheme->aActions[j] ) // TODO: [BUG] tests a row address, which is never null
                {
                    memset(pScheme->aActions[j], 0, STD_ARRAYLEN(pScheme->aActions[j]));
                }
            }
        }
    }

    if ( pConfig->paDeleteSchemes )
    {
        stdMemory_Free(pConfig->paDeleteSchemes);
        pConfig->paDeleteSchemes = NULL;
    }

    if ( pConfig->aSchemes )
    {
        stdMemory_Free(pConfig->aSchemes);
        pConfig->aSchemes = NULL;
    }
}

int J3DAPI jonesConfig_ShowControlOptions(HWND hWnd)
{
    JonesControlsConfig config;
    HINSTANCE hInstance;
    int curresult;

    if ( !jonesConfig_ControlOptions_InitControlsConfig() )
    {
        return 2;
    }

    memset(&config, 0, sizeof(config));
    if ( jonesConfig_CopyControlConfig(&jonesConfig_controlOptions_curControlConfig, &config) )
    {
        hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

        curresult = JonesDialog_ShowDialog(MAKEINTRESOURCE(111), hWnd, jonesConfig_ControlOptionsDialogProc, (LPARAM)&config);
        if ( curresult == 1 )
        {
            // Set and bind selected scheme as active game control set
            wuRegistry_SaveStr("Configuration", config.aSchemes[config.selectedShemeIdx].aName);
            jonesConfig_BindControls(&config.aSchemes[config.selectedShemeIdx]);

            jonesConfig_FreeControlConfigEntry(&config);
            jonesConfig_FreeControlConfigEntry(&jonesConfig_controlOptions_curControlConfig);
            jonesConfig_controlOptions_curControlConfig.numSchemes = 0;

            // Enable disable mouse
            jonesConfig_EnableMouseControl(wuRegistry_GetIntEx("Mouse Control", 0));
        }
        else
        {
            jonesConfig_FreeControlConfigEntry(&config);
        }

        return curresult;
    }

    return 2;
}

int J3DAPI jonesConfig_CopyControlConfig(JonesControlsConfig* pSrc, JonesControlsConfig* pDst)
{
    JonesControlsScheme* curapSchemes = NULL;
    int bufSz;

    if ( !pSrc )
    {
        return 0;
    }

    if ( !pDst )
    {
#line 4643
        pDst = (JonesControlsConfig*)STDMALLOC(sizeof(JonesControlsConfig));
        if ( !pDst )
        {
            return 0;
        }
    }

    pDst->numSchemes       = pSrc->numSchemes;
    pDst->selectedShemeIdx = pSrc->selectedShemeIdx;

    curapSchemes = jonesConfig_CloneControlSchemesList(pSrc->aSchemes, 0, pSrc->numSchemes);
    if ( !curapSchemes && pSrc->aSchemes )
    {
        return 0;
    }

    if ( pDst->aSchemes )
    {
        jonesConfig_FreeControlConfigEntry(pDst);
        pDst->aSchemes = NULL;
    }

    pDst->aSchemes = curapSchemes;

    bufSz = 10 - pSrc->numSchemes % 10;
    if ( bufSz > 0 && bufSz < 10 )
    {
#line 4672
        pDst->aSchemes = (JonesControlsScheme*)STDREALLOC(pDst->aSchemes, sizeof(JonesControlsScheme) * (pSrc->numSchemes + bufSz));
        memset(&pDst->aSchemes[pSrc->numSchemes], 0, sizeof(JonesControlsScheme) * bufSz);
    }

    return 1;
}

JonesControlsScheme* J3DAPI jonesConfig_CloneControlSchemesList(JonesControlsScheme* aSchemes, size_t start, size_t numSchemes)
{
    JonesControlsScheme* aNewSchemes = NULL;
    int i;
    size_t* pAction;
    int idx;
    JonesControlsScheme* scheme;
    int j;

#line 4703
    aNewSchemes = (JonesControlsScheme*)STDMALLOC(sizeof(JonesControlsScheme) * numSchemes);
    if ( !aNewSchemes || !aSchemes )
    {
        return 0;
    }

    memset(aNewSchemes, 0, sizeof(JonesControlsScheme) * numSchemes);

    for ( i = start; i < (int)(start + numSchemes); ++i )
    {
        scheme = &aSchemes[i];
        strcpy(aNewSchemes[i - start].aName, scheme->aName);

        for ( j = 0; j < (int)STD_ARRAYLEN(scheme->aActions); ++j )
        {
            pAction = scheme->aActions[j];
            memset(aNewSchemes[i - start].aActions[j], 0, STD_ARRAYLEN(aNewSchemes[i - start].aActions[j]));
            for ( idx = 0; idx <= (int)JONESCONTROL_ACTION_GETNUMBINDS(pAction); ++idx )
            {
                aNewSchemes[i - start].aActions[j][idx] = pAction[idx];
            }
        }
    }

    return aNewSchemes;
}

int J3DAPI jonesConfig_ControlOptions_InitControlsConfig(void)
{
    JonesControlsConfig config;
    memset(&config, 0, sizeof(config));

    config.selectedShemeIdx = -1;

    if ( !jonesConfig_ControlOptions_SetAllDefaultControlSchemes(&config) || !jonesConfig_ControlOptions_LoadAllControlSchemes(&config) )
    {
        jonesConfig_FreeControlConfigEntry(&config);
        return 0;
    }
    else if ( !jonesConfig_CopyControlConfig(&config, &jonesConfig_controlOptions_curControlConfig) )
    {
        jonesConfig_FreeControlConfigEntry(&jonesConfig_controlOptions_curControlConfig);
        jonesConfig_FreeControlConfigEntry(&config);
        return 0;
    }
    else
    {
        jonesConfig_FreeControlConfigEntry(&config);
        return 1;
    }
}

BOOL CALLBACK jonesConfig_ControlOptionsDialogProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{

    BOOL bResult = 1;

    switch ( umsg )
    {
        case WM_COMMAND:
            jonesConfig_ControlOptions_HandleWM_COMMAND(hwnd, LOWORD(wparam), lparam, HIWORD(wparam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontControlOptions = jonesConfig_InitDialog(hwnd, 0, 111);
            bResult = jonesConfig_InitControlOptionsDialog(hwnd, wparam, lparam);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hwnd, jonesConfig_hFontControlOptions);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(jonesConfig_countfit_1, 14) /* end count-fit */
int J3DAPI jonesConfig_InitControlOptionsDialog(HWND hDlg, int a2, LPARAM lParam)
{
    JonesControlsConfig* pConfig = (JonesControlsConfig*)lParam;
    HWND hDlgItem;
    HDC hCfgListDC;
    RECT rectList;
    int i;
    char aSelectedCfgName[128];


    if ( pConfig->selectedShemeIdx == -1 )
    {
        memset(aSelectedCfgName, 0, sizeof(aSelectedCfgName));
        wuRegistry_GetStr("Configuration", aSelectedCfgName, STD_ARRAYLEN(aSelectedCfgName), "");
        if ( strlen(aSelectedCfgName) == 0 )
        {
            // Load default config
            const char* pDfltKeyset = NULL;

            switch ( stdControl_GetMaxJoystickButtons() )
            {
                case 4:
                    pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[1]);
                    break;

                case 8:
                    pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[2]);
                    break;

                default:
                    pDfltKeyset = jonesString_GetString(jonesConfig_aDfltKeySetNames[0]);
                    break;
            }

            if ( pDfltKeyset )
            {
                strcpy(aSelectedCfgName, pDfltKeyset);
            }
        }
    }

    pConfig->selectedShemeIdx = 0;

    hDlgItem = GetDlgItem(hDlg, 1047);
    hCfgListDC = GetDC(GetDlgItem(hDlg, 1047));

    for ( i = 0; i < pConfig->numSchemes; ++i )
    {
        SIZE sz;
        int itemIdx;

        itemIdx = ListBox_AddString(hDlgItem, pConfig->aSchemes[i].aName);

        pConfig->aSchemes[i].bModified = 0;
        if ( streq(aSelectedCfgName, pConfig->aSchemes[i].aName) )
        {
            pConfig->selectedShemeIdx = i;
        }

        GetTextExtentPoint(hCfgListDC, pConfig->aSchemes[i].aName, strlen(pConfig->aSchemes[i].aName), &sz);
        if ( sz.cx > pConfig->maxNameLen )
        {
            pConfig->maxNameLen = sz.cx;
        }
    }

    GetClientRect(hDlgItem, &rectList);
    if ( rectList.right - rectList.left < pConfig->maxNameLen )
    {
        ListBox_SetHorizontalExtent(hDlgItem, pConfig->maxNameLen);
    }

    ListBox_SetCurSel(hDlgItem, pConfig->selectedShemeIdx);

    if ( pConfig->selectedShemeIdx < (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
    {
        // We have only default scheme, disable view scheme button
        const char* pButtonText = jonesString_GetString("JONES_STR_VIEW_SCHEME");
        hDlgItem = GetDlgItem(hDlg, 1022);
        EnableWindow(hDlgItem, 0);
        if ( pButtonText )
        {
            hDlgItem = GetDlgItem(hDlg, 1028);
            SetWindowText(hDlgItem, pButtonText);
        }
    }

    // Mouse checkbox
    hDlgItem = GetDlgItem(hDlg, 1053);
    Button_SetCheck(hDlgItem, wuRegistry_GetIntEx("Mouse Control", 0));

    // Joystick checkbox
    hDlgItem = GetDlgItem(hDlg, 1054);
    if ( stdControl_GetNumJoysticks() > 0 )
    {
        Button_SetCheck(hDlgItem, wuRegistry_GetIntEx("Joystick Control", 0));
    }
    else
    {
        EnableWindow(hDlgItem, 0);
    }

    SetWindowLong(hDlg, DWL_USER, (LONG)pConfig);
    return 1;
}

void J3DAPI jonesConfig_ControlOptions_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, unsigned int notificationCode)
{
    JonesControlsConfig* pConfig;


    pConfig = (JonesControlsConfig*)GetWindowLongA(hWnd, DWL_USER);

    switch ( ctrlID )
    {
        case 1047: // list control notification
        {
            HWND hList = GetDlgItem(hWnd, 1047);
            int selIdx = ListBox_GetCurSel(hList);

            jonesConfig_ControlOptions_UpdateButtons(hWnd);
            pConfig->selectedShemeIdx = selIdx;

            if ( notificationCode != LBN_DBLCLK )
            {
                return;
            }
        }
        // Fall through: a double click edits the scheme like the Edit button

        case 1028: // Edit Scheme button clicked
            jonesConfig_ControlOptions_EditSelectedScheme(hWnd, pConfig);
            break;

        case 1022: // delete scheme button
            jonesConfig_ControlOptions_DeleteScheme(hWnd, pConfig);
            return;


        case 1029: // New Scheme button clicked
            jonesConfig_ControlOptions_CreateNewScheme(hWnd, pConfig, jonesConfig_controlOptions_pCurNewScheme);
            return;

        case 1: // OK button
        {
            int bMouseEnabled = Button_GetCheck(GetDlgItem(hWnd, 1053));

            int bJoyEnabled = Button_GetCheck(GetDlgItem(hWnd, 1054));

            // Delete pending scheme files to be removed
            {
                int i = 0;  // block scope: the debug build's frame slot order follows it
                for ( ; i < pConfig->numDeleteSchemes; ++i )
            {
                remove(pConfig->paDeleteSchemes[i]);
            }
            }

            pConfig->numDeleteSchemes = 0;
            if ( pConfig->paDeleteSchemes )
            {
                stdMemory_Free(pConfig->paDeleteSchemes);
                pConfig->paDeleteSchemes = NULL;
            }

            // Set current selected scheme as current game control set
            if ( jonesConfig_CopyControlConfig(pConfig, &jonesConfig_controlOptions_curControlConfig) )
            {
                int i;

                // Write modified schemes to file
                for ( i = (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; i < pConfig->numSchemes; ++i )
                {
                    if ( pConfig->aSchemes[i].bModified == 1 && !jonesConfig_ControlOptions_WriteScheme(&pConfig->aSchemes[i]) )
                    {
                        const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
                        if ( pErrorText )
                        {
                            jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                            InvalidateRect(hWnd, 0, 1);
                        }
                    }
                }
            }
            else
            {
                const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
                if ( pErrorText )
                {
                    jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pErrorText, 145);
                    InvalidateRect(hWnd, 0, 1);
                }

                // Due to error cancel set to cancel action
                ctrlID = 2; // cancel
            }

            // Save mouse/joystick settings
            wuRegistry_SaveIntEx("Mouse Control", bMouseEnabled);
            wuRegistry_SaveIntEx("Joystick Control", bJoyEnabled);

            if ( bJoyEnabled )
            {
                // Load current active scheme from file
                JonesControlsScheme* pActiveScheme = jonesConfig_LoadActiveControlScheme();
                if ( pActiveScheme )
                {
                    JonesControl_EnableJoystickAxes();
                    jonesConfig_BindControls(pActiveScheme);// TODO: why this has to be done here? Note, same is done when this dialog closes
                    jonesConfig_FreeControlScheme(pActiveScheme);
                }
            }
            else
            {
                sithControl_UnbindJoystickAxes();
            }

        }
        // Fall through: case 2 owns the shared free-and-EndDialog tail

        case 2: // Cancel button
            // Delete any new scheme
            if ( jonesConfig_controlOptions_pCurNewScheme )
            {
                stdMemory_Free(jonesConfig_controlOptions_pCurNewScheme);
                jonesConfig_controlOptions_pCurNewScheme = NULL;
            }

            EndDialog(hWnd, ctrlID);
            break;

        default:
            return;
    }
}

int J3DAPI jonesConfig_ControlOptions_UpdateButtons(HWND hDlg)
{
    int selIdx = ListBox_GetCurSel(GetDlgItem(hDlg, 1047));
    const char* pText = NULL;
    HWND hEditBtnCtrl = GetDlgItem(hDlg, 1028);
    if ( selIdx < (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
    {
        // Only default schemes are available,
        // set edit button to view and disable delete button
        pText = jonesString_GetString("JONES_STR_VIEW_SCHEME");
        if ( pText )
        {
            SetWindowText(hEditBtnCtrl, pText);
        }

        return EnableWindow(GetDlgItem(hDlg, 1022), 0);
    }
    else
    {
        // Ok we have custom schemes enable delete button
        pText = jonesString_GetString("JONES_STR_EDIT_SCHEME");
        if ( pText )
        {
            SetWindowText(hEditBtnCtrl, pText);
        }

        return EnableWindow(GetDlgItem(hDlg, 1022), 1);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(jonesConfig_countfit_2, 29) /* end count-fit */
void J3DAPI jonesConfig_ControlOptions_DeleteScheme(HWND hDlg, JonesControlsConfig* pConfig)
{
    int schemeNum = 0;

    HWND hListCtrl = GetDlgItem(hDlg, 1047); // list of schemes in control options dialog

    char paSchemePath[128];
    int selIdx = ListBox_GetCurSel(hListCtrl);
    memset(paSchemePath, 0, sizeof(paSchemePath));

    ListBox_GetText(hListCtrl, selIdx, paSchemePath);
    ListBox_DeleteString(hListCtrl, selIdx);

    while ( schemeNum < pConfig->numSchemes )
    {
        if ( streq(pConfig->aSchemes[schemeNum].aName, paSchemePath) )
        {
            int curi;
            int j;

            if ( (pConfig->numDeleteSchemes % 10) == 0 )
            {
#line 5091
                pConfig->paDeleteSchemes = (char (*)[128])STDREALLOC(
                    pConfig->paDeleteSchemes,
                    (pConfig->numDeleteSchemes + 10) * 128
                );
            }

            if ( pConfig->aSchemes[schemeNum].bModified != 1 )
            {
                char aPath[128];
                memset(aPath, 0, sizeof(aPath));

                sprintf(aPath, "%s\\%s%s", jonesConfig_aKeySetDirectory, paSchemePath, ".kfg");
                strcpy(pConfig->paDeleteSchemes[pConfig->numDeleteSchemes], aPath);
                ++pConfig->numDeleteSchemes;
            }

            for ( curi = 0; curi < (int)STD_ARRAYLEN(pConfig->aSchemes[schemeNum].aActions); ++curi )
            {
                memset(pConfig->aSchemes[schemeNum].aActions[curi], 0, STD_ARRAYLEN(pConfig->aSchemes[schemeNum].aActions[curi]));
            }

            for ( j = schemeNum; j < (signed int)pConfig->numSchemes; ++j )
            {
                if ( j == pConfig->numSchemes - 1 )
                {
                    memset(&pConfig->aSchemes[j], 0, sizeof(pConfig->aSchemes[j]));
                }
                else
                {
                    pConfig->aSchemes[j] = pConfig->aSchemes[j + 1];
                }
            }

            if ( schemeNum == pConfig->selectedShemeIdx )
            {
                pConfig->selectedShemeIdx = pConfig->selectedShemeIdx - 1 < 0 ? 0 : pConfig->selectedShemeIdx - 1;

                ListBox_SetCurSel(hListCtrl, pConfig->selectedShemeIdx);

                if ( pConfig->selectedShemeIdx < (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )// Are we left only with default schemes?
                {
                    // Disable delete button
                    EnableWindow(GetDlgItem(hDlg, 1022), 0);
                }
            }

            --pConfig->numSchemes;
            return;
        }

        ++schemeNum;
    }
}

void J3DAPI jonesConfig_ControlOptions_EditSelectedScheme(HWND hDlg, JonesControlsConfig* pConfig)
{
    char paSchemePath[128];
    JonesEditControlSchemeDialogData editdata;

    HWND hListCtrl = GetDlgItem(hDlg, 1047); // list of scheme names in control options

    // Get selected scheme name
    int selIdx = ListBox_GetCurSel(hListCtrl);
    int start;
    start = 0;
    memset(paSchemePath, 0, sizeof(paSchemePath));
    ListBox_GetText(hListCtrl, selIdx, paSchemePath);

    while ( start < pConfig->numSchemes )
    {
        if ( streq(pConfig->aSchemes[start].aName, paSchemePath) )
        {
            memset(&editdata, 0, sizeof(editdata));
            editdata.aSchemes       = jonesConfig_CloneControlSchemesList(pConfig->aSchemes, start, 1);
            editdata.numSchemes     = start;
            editdata.pDefaultShceme = jonesConfig_GetDefaultControlScheme(pConfig);
            break;
        }

        ++start;
    }

    if ( start == pConfig->numSchemes )
    {
        return;
    }

    if ( jonesConfig_ShowEditControlShemeDialog(hDlg, &editdata) == 1 )
    {
        InvalidateRect(hDlg, 0, 1);
        pConfig->aSchemes[start].bModified = editdata.aSchemes->bModified;
        if ( pConfig->aSchemes[start].bModified )
        {
            int i;
            int j;

            for ( i = 0; i < (int)STD_ARRAYLEN(pConfig->aSchemes[start].aActions); ++i )
            {
                for ( j = 0; j < (int)STD_ARRAYLEN(pConfig->aSchemes[start].aActions[i]); ++j )
                {
                    pConfig->aSchemes[start].aActions[i][j] = editdata.aSchemes->aActions[i][j];
                }
            }
        }

        stdMemory_Free(editdata.aSchemes);
        editdata.aSchemes = NULL;
    }
    else
    {
        jonesConfig_FreeControlScheme(editdata.aSchemes);
    }
}

JonesControlsScheme* J3DAPI jonesConfig_GetDefaultControlScheme(JonesControlsConfig* pConfig)
{
    const char* pSchemeName = NULL;
    int i;

    switch ( stdControl_GetMaxJoystickButtons() )
    {
        case 4:
            pSchemeName = jonesString_GetString(jonesConfig_aDfltKeySetNames[1]);
            break;

        case 8:
            pSchemeName = jonesString_GetString(jonesConfig_aDfltKeySetNames[2]);
            break;

        default:
            // Keyboard
            pSchemeName = jonesString_GetString(jonesConfig_aDfltKeySetNames[0]);
            break;
    }

    if ( pSchemeName )
    {
        for ( i = 0; i < (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; ++i ) // 3 default schemes
        {
            if ( streq(pConfig->aSchemes[i].aName, pSchemeName) )
            {
                return &pConfig->aSchemes[i];
            }
        }
    }

    return NULL;
}

// Declared next to its first use
int J3DAPI jonesConfig_ShowCreateControlSchemeDialog(HWND hWnd, JonesCreateControlSchemeDialogData* pData);
BOOL CALLBACK jonesConfig_CreateControlSchemeProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_InitCreateControlScheme(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_CreateControlScheme_Handle_WM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode);

void J3DAPI jonesConfig_ControlOptions_CreateNewScheme(HWND hDlg, JonesControlsConfig* pConfig, JonesControlsScheme* pNewScheme)
{
    JonesCreateControlSchemeDialogData newSchemeDlgData;
    HWND hSchemeList;
    JonesEditControlSchemeDialogData data;
    int schemeIdx;
    const char* pErrorText;
    SIZE sz;
    HDC hdc;
    int i;
    int j;
    int k;

    // Show dialog for user to enter new scheme filename
    memset(newSchemeDlgData.aNewSchemeName, 0, sizeof(newSchemeDlgData.aNewSchemeName));

    newSchemeDlgData.hSchemeList = GetDlgItem(hDlg, 1047);
    if ( jonesConfig_ShowCreateControlSchemeDialog(hDlg, &newSchemeDlgData) == 1 )
    {
        InvalidateRect(hDlg, 0, 1);

        pNewScheme = jonesConfig_CloneControlSchemesList(pConfig->aSchemes, 0, 1);
        if ( !pNewScheme )
        {
            pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
            if ( pErrorText )
            {
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                InvalidateRect(hDlg, 0, 1);
            }
        }
        else
        {
            strcpy(pNewScheme->aName, newSchemeDlgData.aNewSchemeName);

            memset(&data, 0, sizeof(data));

            data.aSchemes       = pNewScheme;
            data.numSchemes     = pConfig->numSchemes;
            data.pDefaultShceme = jonesConfig_GetDefaultControlScheme(pConfig);

            if ( jonesConfig_ShowEditControlShemeDialog(hDlg, &data) == 1 )
            {
                InvalidateRect(hDlg, 0, 1);

                // Get list position for the new scheme based on alphabetical order

                schemeIdx = (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES; // 3 - first index after default schemes
                while ( schemeIdx < pConfig->numSchemes )
                {
                    if ( strcmp(newSchemeDlgData.aNewSchemeName, pConfig->aSchemes[schemeIdx].aName) < 0 )
                    {
                        break;
                    }

                    ++schemeIdx;
                }

                // Allocate additional scheme list buffer
                if ( (pConfig->numSchemes % 10) == 0 )
                {
#line 5289
                    pConfig->aSchemes = (JonesControlsScheme*)STDREALLOC(pConfig->aSchemes, sizeof(JonesControlsScheme) * (pConfig->numSchemes + 10));
                }

                if ( pConfig->aSchemes )
                {
                    // Make space in config list to insert new scheme by shifting existing schemes for 1 place
                    for ( i = pConfig->numSchemes - 1; i >= schemeIdx; --i )
                    {
                        strcpy(pConfig->aSchemes[i + 1].aName, pConfig->aSchemes[i].aName);
                        for ( j = 0; j < (int)STD_ARRAYLEN(pConfig->aSchemes[i + 1].aActions); ++j )
                        {
                            for ( k = 0; k < (int)STD_ARRAYLEN(pConfig->aSchemes[i + 1].aActions[j]); ++k )
                            {
                                pConfig->aSchemes[i + 1].aActions[j][k] = pConfig->aSchemes[i].aActions[j][k];
                            }
                        }
                    }

                    // Insert new scheme name in list control
                    {
                        hSchemeList = GetDlgItem(hDlg, 1047);
                        ListBox_InsertString(hSchemeList, schemeIdx, newSchemeDlgData.aNewSchemeName);
                        ListBox_SetCurSel(hSchemeList, schemeIdx);

                        pConfig->selectedShemeIdx = schemeIdx;

                        // Resize increase width of list for the new scheme name to fit in row

                        hdc = GetDC(hSchemeList);
                        GetTextExtentPoint(hdc, newSchemeDlgData.aNewSchemeName, strlen(newSchemeDlgData.aNewSchemeName), &sz);

                        if ( sz.cx > pConfig->maxNameLen )
                        {
                            RECT rectSchemeList;

                            pConfig->maxNameLen = sz.cx;

                            GetClientRect(hSchemeList, &rectSchemeList);
                            if ( rectSchemeList.right - rectSchemeList.left < sz.cx )
                            {
                                ListBox_SetHorizontalExtent(hSchemeList, sz.cx);
                            }
                        }

                        // Insert new scheme in config list
                        pConfig->aSchemes[schemeIdx].bModified = data.aSchemes->bModified;
                        strcpy(pConfig->aSchemes[schemeIdx].aName, data.aSchemes->aName);

                        for ( j = 0; j < (int)STD_ARRAYLEN(pConfig->aSchemes[schemeIdx].aActions); ++j )
                        {
                            int l;
                            for ( l = 0; l < (int)STD_ARRAYLEN(pConfig->aSchemes[schemeIdx].aActions[j]); ++l )
                            {
                                pConfig->aSchemes[schemeIdx].aActions[j][l] = data.aSchemes->aActions[j][l];
                            }
                        }

                        stdMemory_Free(data.aSchemes);
                        data.aSchemes = NULL;
                        ++pConfig->numSchemes;
                    }
                }
                else
                {
                    const char* pErrorText = jonesString_GetString("JONES_STR_NO_MEM_SCHEME");
                    if ( pErrorText )
                    {
                        jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                        InvalidateRect(hDlg, 0, 1);
                    }
                }
            }
            else
            {
                stdMemory_Free(pNewScheme);
            }

            pNewScheme = NULL;
        }
    }
}

int J3DAPI jonesConfig_ShowCreateControlSchemeDialog(HWND hWnd, JonesCreateControlSchemeDialogData* pData)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    int dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(116), hWnd, jonesConfig_CreateControlSchemeProc, (LPARAM)pData);
    return dlgResult;
}

BOOL CALLBACK jonesConfig_CreateControlSchemeProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    BOOL bResult = 1;

    switch ( msg )
    {
        case WM_COMMAND:
            jonesConfig_CreateControlScheme_Handle_WM_COMMAND(hWnd, LOWORD(wparam), lparam, HIWORD(wparam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontCreateControlSchemeDlg = jonesConfig_InitDialog(hWnd, 0, 116);
            bResult = jonesConfig_InitCreateControlScheme(hWnd, wparam, lparam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontCreateControlSchemeDlg);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitCreateControlScheme(HWND hDlg, int a2, LPARAM lParam)
{
    JonesCreateControlSchemeDialogData* pData = (JonesCreateControlSchemeDialogData*)lParam;
    HWND hwndEditCtrl;


    hwndEditCtrl = GetDlgItem(hDlg, 1052);
    SetFocus(hwndEditCtrl);
    Edit_LimitText(hwndEditCtrl, 60);

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 0; // should it return 1 ?
}

void J3DAPI jonesConfig_CreateControlScheme_Handle_WM_COMMAND(HWND hDlg, int ctrlID, LPARAM lParam, int notifyCode)
{
    HWND hwndEditCtrl;
    int numSchemes;
    int itemIdx;
    const char* pCurInvalidChars;
    CHAR paName[128];

    JonesCreateControlSchemeDialogData* pData = (JonesCreateControlSchemeDialogData*)GetWindowLongA(hDlg, DWL_USER);


    switch ( ctrlID )
    {
        case 1: // ok button
        {
            char aSchemeName[128];

            pCurInvalidChars = "/:\\\"?<>*|+";
            hwndEditCtrl = GetDlgItem(hDlg, 1052);

            memset(paName, 0, sizeof(paName));
            Edit_GetText(hwndEditCtrl, paName, STD_ARRAYLEN(paName));

            if ( strlen(paName) != strcspn(paName, pCurInvalidChars) )
            {
                // Bad file name format
                const char* pErrorText = jonesString_GetString("JONES_STR_INVALIDFILE");
                if ( pErrorText )
                {
                    char aErrorText[256];
                    memset(aErrorText, 0, sizeof(aErrorText));

                    sprintf(aErrorText, pErrorText, paName);
                    jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", aErrorText, 145); // 145 - typewriter icon
                    InvalidateRect(hDlg, 0, 1);
                }

                Edit_SetText(hwndEditCtrl, ""); // empty string
                return;
            }

            // Check if scheme with same name already exists
            memset(aSchemeName, 0, sizeof(aSchemeName));

            itemIdx = 0;
            numSchemes = ListBox_GetCount(pData->hSchemeList);
            while ( itemIdx < numSchemes )
            {
                ListBox_GetText(pData->hSchemeList, itemIdx, aSchemeName);
                if ( streq(paName, aSchemeName) )
                {
                    const char* pErrorText = jonesString_GetString("JONES_STR_DUP_SCHEME");
                    if ( pErrorText )
                    {
                        jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                        InvalidateRect(hDlg, 0, 1);
                    }

                    Edit_SetText(hwndEditCtrl, ""); // empty string
                    return;
                }

                // Check if name is empty. Retail tests it here, inside the loop (0x00408B63)
                if ( strlen(paName) == 0 )
                {
                    const char* pErrorText = jonesString_GetString("JONES_STR_NO_SCHEME");
                    if ( pErrorText )
                    {
                        jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typewriter icon
                        InvalidateRect(hDlg, 0, 1);
                    }

                    return;
                }

                ++itemIdx;
            }

            // Ok name check has passed, assign new name to dlg data and close dialog
            if ( itemIdx == numSchemes )
            {
                Edit_GetText(hwndEditCtrl, pData->aNewSchemeName, STD_ARRAYLEN(pData->aNewSchemeName));
            }
        }
        // Fall through: a valid name closes the dialog like Cancel

        case 2: // cancel button
            EndDialog(hDlg, ctrlID);
            break;
    }
}

int J3DAPI jonesConfig_ShowEditControlShemeDialog(HWND hWnd, JonesEditControlSchemeDialogData* pData)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    int dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(115), hWnd, jonesConfig_EditControlSchemeProc, (LPARAM)pData);
    return dlgResult;
}

BOOL CALLBACK jonesConfig_EditControlSchemeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_EditControlScheme_Handle_WM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontEditControlShceme = jonesConfig_InitDialog(hWnd, 0, 115);
            bResult = jonesConfig_InitEditControlSchemeDlg(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_NOTIFY:
        {
            // Update cur selected list state
            NMHDR* pNM = (NMHDR*)lParam;
            JonesEditControlSchemeDialogData* pData = (JonesEditControlSchemeDialogData*)GetWindowLong(hWnd, DWL_USER);

            switch ( pNM->code )
            {
                case NM_CLICK: // -2 NM_CLICK
                    if ( pNM->idFrom == 1005 )
                    {
                        pData->hCurList = GetDlgItem(hWnd, 1005);
                        pData->curListID = 1005;
                    }
                    else if ( pNM->idFrom == 1006 )
                    {
                        pData->hCurList = GetDlgItem(hWnd, 1006);
                        pData->curListID = 1006;
                    }

                    break;
            }

            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontEditControlShceme);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitEditControlSchemeDlg(HWND hDlg, int a2, LPARAM lParam)
{
    HWND hwndKeyListCtrl;
    HWND hControlListCtrl;
    JonesEditControlSchemeDialogData* pData;
    const char* pGBText;
    char paGBText[256];

    hwndKeyListCtrl = GetDlgItem(hDlg, 1005);
    hControlListCtrl = GetDlgItem(hDlg, 1006);
    pData = (JonesEditControlSchemeDialogData*)lParam;
    pGBText = NULL;

    // Set keyboard binding list
    jonesConfig_EditControlScheme_SetListColumn(hwndKeyListCtrl, "JONES_STR_MAPKEY");
    jonesConfig_EditControlScheme_SetListScheme(hwndKeyListCtrl, 1005, pData->aSchemes);

    // Set controller (joystick/gamepad) binding list
    jonesConfig_EditControlScheme_SetListColumn(hControlListCtrl, "JONES_STR_MAPBUTTON");
    jonesConfig_EditControlScheme_SetListScheme(hControlListCtrl, 1006, pData->aSchemes);

    memset(paGBText, 0, sizeof(paGBText));
    pGBText = jonesString_GetString("JONES_STR_SETTINGS");
    if ( pGBText )
    {
        sprintf(paGBText, pGBText, pData->aSchemes->aName);
        SetWindowText(GetDlgItem(hDlg, 1049), paGBText); // 1049 - settings group box
    }

    if ( pData->numSchemes < (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
    {
        // We have only default schemes, disable modify buttons

        EnableWindow(GetDlgItem(hDlg, 1056), 0);

        EnableWindow(GetDlgItem(hDlg, 1046), 0);

        EnableWindow(GetDlgItem(hDlg, 1038), 0);

        EnableWindow(GetDlgItem(hDlg, 1), 0);

        EnableWindow(GetDlgItem(hDlg, 1099), 0);
    }

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

void J3DAPI jonesConfig_EditControlScheme_SetListColumn(HWND hCtrl, const char* pJSText)
{
    int width;
    RECT rcClient;
    int keysWidth;
    LVCOLUMNA column;
    const char* pText;

    int gameWidth, gameHgt;
    GetClientRect(hCtrl, &rcClient);
    width = rcClient.right - rcClient.left - GetSystemMetrics(SM_CXVSCROLL);

    column.mask     = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    column.cchTextMax = 256;
    column.fmt        = LVCFMT_LEFT;

    stdDisplay_GetBackBufferSize((uint32_t*)&gameWidth, (uint32_t*)&gameHgt);

    keysWidth = (int32_t)(105.0 * (1.0 < gameHgt / 480.0 ? 1.0 : gameHgt / 480.0)); // TODO: maybe remove the clamp to also upscale

    pText = jonesString_GetString("JONES_STR_FCTN");
    column.pszText    = pText;
    column.iSubItem   = 0;
    column.cx         = width - keysWidth;
    ListView_InsertColumn(hCtrl, column.iSubItem, &column);

    if ( pJSText )
    {
        pText = jonesString_GetString(pJSText);
        column.pszText = pText;
    }

    column.iSubItem = 1;
    column.cx       = keysWidth;
    ListView_InsertColumn(hCtrl, column.iSubItem, &column);
}

void J3DAPI jonesConfig_EditControlScheme_SetListScheme(HWND hCtrl, int ctrlID, JonesControlsScheme* pScheme)
{
    size_t itemIndex;
    int i;
    char aCtrlText[256];

    LVITEM item;
    item.mask       = LVIF_STATE | LVIF_PARAM | LVIF_TEXT;
    item.cchTextMax = 256;
    item.state      = 0;
    item.stateMask  = 0;

    itemIndex = 0;
    for ( i = 0; i < (int)STD_ARRAYLEN(jonesConfig_aJonesControlActionNames); ++i )
    {
        size_t ctrlsCount = 0;
        const char* pActionName = jonesString_GetString(jonesConfig_aJonesControlActionNames[i]);

        {
            int bindNum = 1;  // block scope: the debug build's frame slot order follows it
            for ( ; bindNum <= (int)JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[i]); ++bindNum ) // Note, bindNum must start at 1 as 0 indes ix num bindings state
        {
            int keyid = pScheme->aActions[i][bindNum];
            item.iItem    = itemIndex;
            item.iSubItem = 0;
            item.pszText  = (LPSTR)pActionName;

            if ( ctrlID == 1005 // 1005 - keyboard list
                && (keyid < STDCONTROL_MAX_KEYBOARD_BUTTONS || keyid > STDCONTROL_JOYSTICK_LASTPOVCID) && (keyid < JONESCONFIG_JOYAXISKID_UP || keyid > JONESCONFIG_JOYAXISKID_GROUND)
                || ctrlID == 1006 // controller list
                && (keyid >= STDCONTROL_JOYSTICK_FIRSTKID && keyid <= STDCONTROL_JOYSTICK_LASTPOVCID || keyid >= JONESCONFIG_JOYAXISKID_UP && keyid <= JONESCONFIG_JOYAXISKID_GROUND) )
            {
                if ( ctrlID == 1006 ) // Control List
                {
                    ++ctrlsCount;
                }

                item.lParam = (keyid << 16) | i;
                ListView_InsertItem(hCtrl, &item);

                memset(aCtrlText, 0, sizeof(aCtrlText));

                jonesConfig_ControlToString(keyid, aCtrlText);
                ListView_SetItemText(hCtrl, itemIndex, 1, aCtrlText);
                ++itemIndex;
            }
        }
        }

        if ( !ctrlsCount && ctrlID == 1006 )   // Control List
        {
            item.lParam = i;
            ListView_InsertItem(hCtrl, &item);

            memset(aCtrlText, 0, sizeof(aCtrlText));

            ListView_SetItemText(hCtrl, itemIndex, 1, aCtrlText);
            ++itemIndex;
        }
    }
}
void J3DAPI jonesConfig_EditControlScheme_Handle_WM_COMMAND(HWND hDlg, int ctrlID, LPARAM lParam, int notifyCode)
{
    JonesEditControlSchemeDialogData* pData = (JonesEditControlSchemeDialogData*)GetWindowLong(hDlg, DWL_USER);

    switch ( ctrlID )
    {
        case 1056: // Edit button
            if ( pData->numSchemes >= (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
            {
                jonesConfig_EditControlScheme_EditAssignment(hDlg, pData->hCurList, pData->curListID, pData->aSchemes);
            }
            break;

        case 1046: // Add button
            if ( pData->numSchemes >= (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
            {
                jonesConfig_EditControlScheme_AddAssignment(hDlg, pData->hCurList, pData->curListID, pData->aSchemes);
            }
            break;

        case 1038: // Delete button
            if ( pData->numSchemes >= (int)JONESCONFIG_NUMDEFAULTCONTROLSCHEMES )
            {
                jonesConfig_EditControlScheme_DeleteAssignment(hDlg, pData->hCurList, pData->curListID, pData->aSchemes);
            }
            break;

        case 1099: // Default scheme button
        {
            size_t* pDefaultAction;
            int bindIdx;
            size_t* pAction;
            int i;
            HWND hKeyListCtrl     = GetDlgItem(hDlg, 1005);
            HWND hCtrlList = GetDlgItem(hDlg, 1006);

            // Copy default scheme
            for ( i = 0; i < (int)STD_ARRAYLEN(pData->aSchemes->aActions); ++i )
            {
                pAction        = pData->aSchemes->aActions[i];
                pDefaultAction = pData->pDefaultShceme->aActions[i];

                memset(pAction, 0, STD_ARRAYLEN(pData->aSchemes->aActions[i])); // Clear existing bindings
                for ( bindIdx = 0; bindIdx <= (int)JONESCONTROL_ACTION_GETNUMBINDS(pDefaultAction); ++bindIdx )
                {
                    pAction[bindIdx] = pDefaultAction[bindIdx];
                }
            }

            // Reset list controls
            ListView_DeleteAllItems(hKeyListCtrl);
            ListView_DeleteAllItems(hCtrlList);

            // Init list controls with changed scheme
            jonesConfig_EditControlScheme_SetListScheme(hKeyListCtrl, 1005, pData->aSchemes); // Key list
            jonesConfig_EditControlScheme_SetListScheme(hCtrlList, 1006, pData->aSchemes); // Controller  list
            break;
        }

        case 1: // Save button
        {
            // Check that all required actions have key bindings
            int i;
            char aActionName[512];

            memset(aActionName, 0, sizeof(aActionName));
            for ( i = 0; i < (int)STD_ARRAYLEN(pData->aSchemes->aActions); ++i )
            {
                if ( JONESCONTROL_ACTION_GETNUMBINDS(pData->aSchemes->aActions[i]) == 0 && (i < JONESCONTROL_ACTION_FISTS || i > JONESCONTROL_ACTION_BAZOOKA) ) // 16 - 24 are weapons keys
                {
                    strcat(aActionName, jonesString_GetString(jonesConfig_aJonesCapControlActionNames[i]));
                    strcat(aActionName, "\n");
                }
            }

            // If there was an error aActionName len != 0
            if ( strlen(aActionName) > 0 )
            {
                char aErrorText[512];
                memset(aErrorText, 0, sizeof(aErrorText));

                sprintf(aErrorText, "%s\n", jonesString_GetString("JONES_STR_NO_MAPNG"));

                strcat(aErrorText, aActionName);
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", aErrorText, 145);// 145 - typewrite icon
                InvalidateRect(hDlg, 0, 1);
                break;
            }

            pData->aSchemes->bModified = 1;
        }
        // Fall through: a valid scheme closes the dialog like Cancel

        case 2: // Cancel button
            EndDialog(hDlg, ctrlID);
            break;
    }
}

void J3DAPI jonesConfig_EditControlScheme_EditAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme)
{
    size_t bindingIdx, itemIdx;
    JonesAssignKeyDialogData assignkeyData;
    memset(&assignkeyData, 0, sizeof(assignkeyData));

    assignkeyData.pScheme  = pScheme;
    assignkeyData.actionId = jonesConfig_EditControlScheme_GetSelectedListItemBinding(hDlg, hListCtrl, pScheme, &itemIdx, &bindingIdx);
    assignkeyData.listID   = listID;

    if ( assignkeyData.actionId < 0 )
    {
        const char* pErrorText = jonesString_GetString("JONES_STR_NOKEYACTION");
        if ( pErrorText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145);// 145 - typewriter icon
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    if ( jonesConfig_ShowAssignKeyDialog(hDlg, &assignkeyData) == 1 )
    {
        size_t* pAction = pScheme->aActions[assignkeyData.actionId];

        InvalidateRect(hDlg, 0, 1);

        if ( pAction ) // TODO: [BUG] tests a row address, which is never null
        {
            LVITEM item;

            itemIdx = ListView_GetNextItem(hListCtrl, -1, LVNI_SELECTED); // TODO: why do another call, and not use value of itemIdx returned by above func?
            ListView_SetItemText(hListCtrl, itemIdx, 1, assignkeyData.aControlName);

            
            item.mask     = LVIF_PARAM;
            item.iItem    = itemIdx;
            item.iSubItem = 0;
            item.lParam   = (assignkeyData.controlId << 16) | assignkeyData.actionId;
            ListView_SetItem(hListCtrl, &item);

            // Assign new key biding
            pAction[bindingIdx + 1] = assignkeyData.controlId; // Note, bindIdx must be incremented as it starts with 0, and index 0 points to num binds state

            // Update num key binds only if 0
            if ( assignkeyData.listID == 1005 && !JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pAction) )
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pAction, +1);
            }
            else if ( assignkeyData.listID == 1006 && !JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pAction) )
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pAction, +1);
            }

        }
    }
}

int J3DAPI jonesConfig_EditControlScheme_GetSelectedListItemBinding(HWND hDlg, HWND hListCtrl, JonesControlsScheme* pScheme, size_t* pItemIdx, size_t* pBindIdx)
{
    size_t i;
    size_t bindIdx;
    size_t actionId;
    size_t controlId;
    LVITEM item;
    int itemIndex;
    size_t* pCurAction;

    if ( !hListCtrl || !hDlg || !pScheme )
    {
        return -1;
    }

    itemIndex = ListView_GetNextItem(hListCtrl, -1, LVNI_SELECTED);
    if ( itemIndex < 0 )
    {
        return -1;
    }

    
    item.mask     = LVIF_PARAM;
    item.iItem    = itemIndex;
    item.iSubItem = 0;
    ListView_GetItem(hListCtrl, &item);

    actionId = item.lParam & 0xFFFF;
    controlId = (item.lParam & 0xFFFF0000) >> 16;// key binding controlID (kid or cid)
    bindIdx = 0;

    pCurAction = pScheme->aActions[actionId];

    for ( i = 1; i <= JONESCONTROL_ACTION_GETNUMBINDS(pCurAction); ++i ) // TODO, should check be < rather than <=
    {
        if ( pCurAction[i] != controlId )
        {
            ++bindIdx;
        }
        else
        {
            break;
        }
    }

    if ( !controlId
        || !JONESCONTROL_ACTION_GETNUMBINDS(pCurAction)
        || i <= JONESCONTROL_ACTION_GETNUMBINDS(pCurAction) )
    {
        if ( pItemIdx )
        {
            *pItemIdx = itemIndex;
        }

        if ( pBindIdx )
        {
            *pBindIdx = bindIdx;
        }

        return actionId;
    }

    return -1;
}

void J3DAPI jonesConfig_EditControlScheme_AddAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme)
{
    LVITEM item;
    JonesAssignKeyDialogData curdata;
    size_t* pBinds;
    const char* pText;
    const char* pErrorText;

    memset(&curdata, 0, sizeof(curdata));

    item.mask     = LVIF_PARAM;
    item.iSubItem = 0;
    item.iItem    = ListView_GetNextItem(hListCtrl, -1, LVNI_SELECTED);
    if ( item.iItem < 0 )
    {
        pErrorText = (char*)jonesString_GetString("JONES_STR_NOKEYACTION");
        if ( pErrorText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pErrorText, 145); // 145 - typwrite icon
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    ListView_GetItem(hListCtrl, &item);

    curdata.actionId = item.lParam & 0xFFFF;
    curdata.pScheme  = pScheme;
    curdata.listID   = listID;
    if ( jonesConfig_ShowAssignKeyDialog(hDlg, &curdata) == 1 )
    {
        pBinds = pScheme->aActions[curdata.actionId];
        InvalidateRect(hDlg, 0, 1);

        if ( pBinds && JONESCONTROL_ACTION_GETNUMBINDS(pBinds) == JONESCONTROL_ACTION_MAXBINDS )
        {
            // We exceed max number of action key bindings
            char szMsgText[256];
            char aErrorText[512];
            char aActioName[512];
            const char* pActionName;

            memset(szMsgText, 0, sizeof(szMsgText));

            pText = jonesString_GetString("JONES_STR_EXCEED");
            if ( pText )
            {
                strcpy(aErrorText, pText);
            }
            else
            {
                memset(aErrorText, 0, sizeof(aErrorText));
            }

            pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[curdata.actionId]);
            if ( pActionName )
            {
                strcpy(aActioName, pActionName);
            }
            else
            {
                memset(aActioName, 0, sizeof(aActioName));
            }

            if ( aErrorText[0] && aActioName[0] )
            {
                sprintf(szMsgText, aErrorText, aActioName);
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", szMsgText, 145);
                InvalidateRect(hDlg, 0, 1);
            }
        }
        else if ( pBinds )
        {
            int numBinds;

            if ( listID == 1005 )
            {
                numBinds = JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pBinds);
            }
            else
            {
                numBinds = JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pBinds);
            }

            if ( numBinds > 0 )
            {
                int numItems = ListView_GetItemCount(hListCtrl);
                while ( curdata.actionId == (item.lParam & 0xFFFF) && item.iItem < numItems )
                {
                    ++item.iItem;
                    ListView_GetItem(hListCtrl, &item);
                }

                pText = jonesString_GetString(jonesConfig_aJonesControlActionNames[curdata.actionId]);
                if ( pText )
                {
                    item.mask       = LVIF_STATE | LVIF_PARAM | LVIF_TEXT;
                    item.state      = 0;
                    item.stateMask  = 0;
                    item.pszText    = (LPSTR)pText;
                    item.cchTextMax = 256;
                    item.lParam     = (curdata.controlId << 16) | curdata.actionId;
                    ListView_InsertItem(hListCtrl, &item);
                }
            }
            else
            {
                item.mask   = LVIF_PARAM;
                item.lParam = (curdata.controlId << 16) | curdata.actionId;
                ListView_SetItem(hListCtrl, &item);
            }

            ListView_SetItemState(hListCtrl, item.iItem, 3, 3);
            ListView_SetItemText(hListCtrl, item.iItem, 1, curdata.aControlName);

            // Assign next key binding
            pBinds[JONESCONTROL_ACTION_GETNUMBINDS(pBinds) + 1] = curdata.controlId;

            // Update num bindings state
            if ( listID == 1005 ) // key list
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pBinds, +1);
            }
            else // controller list
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pBinds, +1);
            }
        }
        else
        {
            pText = jonesString_GetString("JONES_STR_NO_MEM_ASGNMNT");
            if ( pText )
            {
                jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_CTRL_OPTS", pText, 145);
                InvalidateRect(hDlg, 0, 1);
            }
        }
    }
}

void J3DAPI jonesConfig_EditControlScheme_DeleteAssignment(HWND hDlg, HWND hListCtrl, int listID, JonesControlsScheme* pScheme)
{
    LVITEM listItem;
    int destIndex;
    int itemIdx, bndIdx;
    int actionIdx = jonesConfig_EditControlScheme_GetSelectedListItemBinding(hDlg, hListCtrl, pScheme, &itemIdx, &bndIdx);
    int numBinds;

    if ( actionIdx < 0 )
    {
        return;
    }

    if ( listID == 1005 ) // if keyboard list
    {
        numBinds = JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[actionIdx]);
    }
    else
    {
        numBinds = JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[actionIdx]);
    }

    if ( !numBinds )
    {
        return;
    }

    if ( numBinds > 1 )
    {
        ListView_DeleteItem(hListCtrl, itemIdx);
    }
    else
    {
        ListView_SetItemText(hListCtrl, itemIdx, 1, "");

        listItem.mask     = LVIF_PARAM;
        listItem.iItem    = itemIdx;
        listItem.iSubItem = 0;
        listItem.lParam   = actionIdx;
        ListView_SetItem(hListCtrl, &listItem);
    }

    for ( destIndex = bndIdx + 1; destIndex < (int)JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[actionIdx]); ++destIndex )
    {
        pScheme->aActions[actionIdx][destIndex] = pScheme->aActions[actionIdx][destIndex + 1];
    }

    if ( listID == 1005 ) // Keyboard list
    {
        JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[actionIdx], -1);
    }
    else // Controller list
    {
        JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[actionIdx], -1);
    }
}
int J3DAPI jonesConfig_ShowAssignKeyDialog(HWND hWnd, JonesAssignKeyDialogData* pData)
{
    int remapDlgResult = 1; // OK button
    HINSTANCE instance;
    int dlgResult, index;
    JonesReAssignKeyDialogData redata;
    size_t functionNum, offset;

    instance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

    do
    {
        memset(pData->aControlName, 0, sizeof(pData->aControlName));

        pData->controlId = 0;
        dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(117), hWnd, jonesConfig_AssignKeyDlgProc, (LPARAM)pData);
        if ( dlgResult == 2 ) // Cancel
        {
            break;
        }

        
        index = jonesConfig_AssignKey_CheckBindForKey(pData->pScheme, pData->controlId, &functionNum, pData->listID, &offset);
        if ( functionNum == pData->actionId )
        {
            char aErrorText[512];
            const char* pErrorText = jonesString_GetString("JONES_STR_SAMEMAP");

            if ( pErrorText )
            {
                strcpy(aErrorText, pErrorText);
            }
            else
            {
                memset(aErrorText, 0, sizeof(aErrorText));
            }

            {
            char aActionName[512];
            const char* pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->actionId]);

            if ( pActionName )
            {
                strcpy(aActionName, pActionName);
            }
            else
            {
                memset(aActionName, 0, sizeof(aActionName));
            }

            if ( aErrorText[0] && aActionName[0] )
            {
            char aControlName[256];
            char aMsgText[256];

            memset(aMsgText, 0, sizeof(aMsgText));
            memset(aControlName, 0, sizeof(aControlName));

            jonesConfig_ControlToString(pData->controlId, aControlName);

            sprintf(aMsgText, "\"%s\" %s %s.", aControlName, aErrorText, aActionName);
            jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", aMsgText, 145);
            }

            dlgResult = 2;
            break;
        }
        }

        // If index > -1 then there is already key assigned for some action.
        // Show reassign msg box
        if ( index > -1 )
        {
            redata.pAssignKeyData = pData;
            redata.index          = index;
            redata.actionId       = functionNum;
            redata.offset         = offset;
            redata.hAssignKeyDlg  = hWnd;
            remapDlgResult = jonesConfig_ShowReassignKeyMsgBox(hWnd, &redata);
        }
    } while ( dlgResult == 1 && index > -1 && remapDlgResult != 1 );

    return dlgResult;
}

int J3DAPI jonesConfig_AssignKey_CheckBindForKey(JonesControlsScheme* pScheme, size_t keyControlID, size_t* pFunctionId, int listID, size_t* pOffset)
{
    int i = -1;
    int function;
    size_t curoffset;
    size_t* pAction;
    size_t currentbindIndx;

    for ( function = 0; function < (int)STD_ARRAYLEN(pScheme->aActions); ++function )
    {
        pAction = pScheme->aActions[function];
        curoffset = 0;
        for ( currentbindIndx = 1; currentbindIndx <= JONESCONTROL_ACTION_GETNUMBINDS(pAction); ++currentbindIndx )
        {
            if ( listID == 1006 // controller list
                && (pAction[currentbindIndx] >= STDCONTROL_JOYSTICK_FIRSTKID && pAction[currentbindIndx] <= STDCONTROL_JOYSTICK_LASTPOVCID
                    || pAction[currentbindIndx] >= JONESCONFIG_JOYAXISKID_UP && pAction[currentbindIndx] <= JONESCONFIG_JOYAXISKID_GROUND) )
            {
                ++i;
            }
            else if ( listID == 1005 // key list
                && (pAction[currentbindIndx] < STDCONTROL_MAX_KEYBOARD_BUTTONS || pAction[currentbindIndx] > STDCONTROL_JOYSTICK_LASTPOVCID)
                && (pAction[currentbindIndx] < JONESCONFIG_JOYAXISKID_UP || pAction[currentbindIndx] > JONESCONFIG_JOYAXISKID_GROUND) )
            {
                ++i;
            }

            if ( keyControlID == pAction[currentbindIndx] )
            {
                // We found existing binding for keyID
                *pFunctionId = function;
                *pOffset = curoffset;
#line 6086
                stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "index: %i function: %i offset: %i\n", i, function, curoffset, 0);
                return i;
            }

            ++curoffset;
        }

        if ( !JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pAction) && listID == 1005 // key list
            || !JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pAction) && listID == 1006 ) // controller list
        {
            ++i;
        }
    }

    return -1;
}

BOOL CALLBACK jonesConfig_AssignKeyDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_AssignKeyDlg_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_TIMER:
        {
            JonesAssignKeyDialogData* pData = (JonesAssignKeyDialogData*)GetWindowLong(hWnd, DWL_USER);
            if ( pData && !pData->bLocked )
            {
                jonesConfig_AssignControlKey_ReadKey(hWnd, wParam);
                bResult = 0;
            }
            else
            {
                bResult = 1;
            }

            break;
        }

        case WM_INITDIALOG:
            jonesConfig_hFontAssignKeyDlg = jonesConfig_InitDialog(hWnd, 0, 117);
            bResult = jonesConfig_InitAssignKeyDlg(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontAssignKeyDlg);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitAssignKeyDlg(HWND hDlg, int a2, LPARAM lParam)
{
    JonesAssignKeyDialogData* pData = (JonesAssignKeyDialogData*)lParam;
    const char* pActionPath = NULL;
    const char* pText;

    if ( pData->listID == 1006 )
    {
        pText = jonesString_GetString("JONES_STR_NEXTBUTTON");
        if ( pText )
        {
            SetWindowText(GetDlgItem(hDlg, 1054), pText);
        }
    }

    pActionPath = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->actionId]);
    if ( pActionPath )
    {
        SetWindowText(GetDlgItem(hDlg, 1055), pActionPath);
    }

    SetTimer(hDlg, 1u, 100u, 0); // Set timer to periodically check for pressed key
    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

void J3DAPI jonesConfig_AssignControlKey_ReadKey(HWND hWnd, WPARAM wParam)
{
    // Function checks if any of the key was pressed, and that case assings kid to pData->mapKeyDlgResult  and ends dialog

    JonesAssignKeyDialogData* pData = (JonesAssignKeyDialogData*)GetWindowLong(hWnd, DWL_USER);

    int bJoyActive = wuRegistry_GetIntEx("Joystick Control", 0);
    int bPressedKey = 0;
    size_t buttonNum;
    int bJustPressd;

    if ( !bPressedKey )
    {
        // Read controls
        jonesConfig_assignKey_bControlsActive = stdControl_ControlsActive();
        stdControl_SetActivation(1);
        stdControl_ReadControls();

        // Check if any of the axis were moved
        if ( bJoyActive && pData->listID == 1006 && jonesConfig_AssignControlKey_ReadJoyAxisKey(pData) ) // 1006 - controller list
        {
            stdControl_SetActivation(jonesConfig_assignKey_bControlsActive);
            bPressedKey = 1;
            KillTimer(hWnd, 1u);
            EndDialog(hWnd, 1);
            return;
        }

        // Ok let's check if button was pressed
        buttonNum = 0;
        while ( buttonNum < STDCONTROL_MAX_KEYID )
        {
            if ( stdControl_ReadKey(buttonNum, &bJustPressd) )
            {
                // 1005 - key list
                if ( pData->listID == 1005
                    && (buttonNum >= STDCONTROL_JOYSTICK_FIRSTKID
                        && buttonNum <= STDCONTROL_JOYSTICK_LASTPOVCID
                        || buttonNum >= JONESCONFIG_JOYAXISKID_UP && buttonNum <= JONESCONFIG_JOYAXISKID_GROUND) )
                {
                    const char* pMsgText = jonesString_GetString("JONES_STR_KEYONLY");
                    if ( pMsgText )
                    {
                        pData->bLocked = 1;
                        jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pMsgText, 145); // 145 - typewriter icon
                        pData->bLocked = 0;
                        InvalidateRect(hWnd, 0, 1);
                    }
                    return;
                }
                else if ( pData->listID == 1006 // Controller list
                    && (buttonNum < STDCONTROL_JOYSTICK_FIRSTKID || buttonNum > STDCONTROL_JOYSTICK_LASTPOVCID)
                    && (buttonNum < JONESCONFIG_JOYAXISKID_UP || buttonNum > JONESCONFIG_JOYAXISKID_GROUND) )
                {
                    const char* pMsgText = jonesString_GetString("JONES_STR_BUTTONONLY");
                    if ( pMsgText )
                    {
                        pData->bLocked = 1;
                        jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", pMsgText, 145); // 145 - typewriter icon
                        pData->bLocked = 0;
                        InvalidateRect(hWnd, 0, 1);
                    }
                    return;
                }
                else
                {
                    jonesConfig_ControlToString(buttonNum, pData->aControlName);
                    pData->controlId = buttonNum;
                    switch ( buttonNum )
                    {
                        case DIK_ESCAPE:
                        case DIK_MINUS:
                        case DIK_EQUALS:
                        case DIK_F4:
                        {
                            // Non mappable key, skip
                            const char* pMsgText = jonesString_GetString("JONES_STR_NO_MAP");
                            char aMsg[256];
                            if ( pMsgText )
                            {
                                sprintf(aMsg, pMsgText, pData->aControlName);

                                pData->bLocked = 1;
                                jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", aMsg, 145); // 145 - typewriter icon
                                pData->bLocked = 0;
                            }

                            memset(pData->aControlName, 0, sizeof(pData->aControlName));
                            break;
                        }
                        case DIK_1:
                        case DIK_2:
                        case DIK_3:
                        case DIK_4:
                        case DIK_5:
                        case DIK_6:
                        case DIK_7:
                        case DIK_8:
                        case DIK_9:
                        case DIK_0:
                        {
                            if ( pData->actionId < JONESCONTROL_ACTION_FISTS || pData->actionId > JONESCONTROL_ACTION_GRENADE ) // 16 - fist; 27 - grenade
                            {
                                // Only weapon key and we have non-weapon action
                                const char* pMsgText = jonesString_GetString("JONES_STR_NO_KEYMAP_NUM");
                                char aMsg[256];
                                if ( pMsgText )
                                {
                                    sprintf(aMsg, pMsgText, pData->aControlName);

                                    pData->bLocked = 1;
                                    jonesConfig_ShowMessageDialog(hWnd, "JONES_STR_CTRL_OPTS", aMsg, 145); // 145 - typewriter icon
                                    pData->bLocked = 0;
                                    InvalidateRect(hWnd, 0, 1);
                                }

                                memset(pData->aControlName, 0, sizeof(pData->aControlName));
                                break;
                            }
                        }
                        default:
                        {
                            // Got Key close dialog
                            stdControl_SetActivation(jonesConfig_assignKey_bControlsActive);
                            bPressedKey = 1;
                            KillTimer(hWnd, 1u);
                            EndDialog(hWnd, 1);
                        }
                    }
                }

                return;
            }

            ++buttonNum;
        }
    }
}

int J3DAPI jonesConfig_AssignControlKey_ReadJoyAxisKey(JonesAssignKeyDialogData* pData)
{
    int bJustPressd;
    int axisNum;

    int numJoysticks = (int)stdControl_GetNumJoysticks();
    if ( !numJoysticks )
    {
        return 0;
    }

    {
        int joyNum = 0;  // block scope: the debug build's frame slot order follows it
        for ( ; joyNum < numJoysticks; ++joyNum )
    {
        for ( axisNum = 0; axisNum <= (int)STDCONTROL_JOYSTICK_AXIS_RZ; ++axisNum )
        {
            unsigned int aidPosition = STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisNum) | STDCONTROL_AID_POSITIVE_AXIS;
            unsigned int aidNeg = STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisNum) | STDCONTROL_AID_NEGATIVE_AXIS;
            if ( stdControl_ReadAxisAsKey(aidPosition, &bJustPressd) )
            {
                switch ( axisNum )
                {
                    case STDCONTROL_JOYSTICK_AXIS_X:
                        pData->controlId = JONESCONFIG_JOYAXISKID_RIGHT;
                        break;

                    case STDCONTROL_JOYSTICK_AXIS_Y:
                        pData->controlId = JONESCONFIG_JOYAXISKID_DOWN;
                        break;

                    case STDCONTROL_JOYSTICK_AXIS_RZ:
                        pData->controlId = JONESCONFIG_JOYAXISKID_TWISTR;
                        break;
                }

                if ( pData->controlId > 0 )
                {
                    jonesConfig_ControlToString(pData->controlId, pData->aControlName);
                    return 1;
                }
            }
            else if ( stdControl_ReadAxisAsKey(aidNeg, &bJustPressd) )
            {
                switch ( axisNum )
                {
                    case STDCONTROL_JOYSTICK_AXIS_X:
                        pData->controlId = JONESCONFIG_JOYAXISKID_LEFT;
                        break;

                    case STDCONTROL_JOYSTICK_AXIS_Y:
                        pData->controlId = JONESCONFIG_JOYAXISKID_UP;
                        break;

                    case STDCONTROL_JOYSTICK_AXIS_RZ:
                        pData->controlId = JONESCONFIG_JOYAXISKID_TWISTL;
                        break;
                }

                if ( pData->controlId > 0 )
                {
                    jonesConfig_ControlToString(pData->controlId, pData->aControlName);
                    return 1;
                }
            }
        }
    }
    }

    return 0;
}

void J3DAPI jonesConfig_AssignKeyDlg_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode)
{

    if ( ctrlID == 2 )
    {
        KillTimer(hWnd, 1u);
        EndDialog(hWnd, ctrlID);
    }
}

int J3DAPI jonesConfig_ShowReassignKeyMsgBox(HWND hWnd, JonesReAssignKeyDialogData* pData)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    int dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(120), hWnd, jonesConfig_ReassignKeyDialogProc, (LPARAM)pData);
    return dlgResult;
}

BOOL CALLBACK jonesConfig_ReassignKeyDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_ReassignKeyDlg_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontReassignKeyDlg = jonesConfig_InitDialog(hWnd, 0, 120);
            bResult = jonesConfig_InitReassignKeyDialog(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontReassignKeyDlg);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitReassignKeyDialog(HWND hDlg, int a2, LPARAM lParam)
{
    JonesReAssignKeyDialogData* pData = (JonesReAssignKeyDialogData*)lParam;
    JonesAssignKeyDialogData* pAssignKeyData = pData->pAssignKeyData;
    char aControlName[256];
    char aText[256];
    char aFormat[512];
    char aActionName[512];
    char aNewMapText[256];
    const char* pText = NULL;
    const char* pActionName = NULL;
    HWND hCtrl;

    memset(aText, 0, sizeof(aText));
    memset(aControlName, 0, sizeof(aControlName));

    jonesConfig_ControlToString(pAssignKeyData->controlId, aControlName);
    sprintf(aText, "\"%s\" ", aControlName);

    pText = jonesString_GetString("JONES_STR_NEWMAP");
    if ( pText )
    {
        hCtrl = GetDlgItem(hDlg, 1066);
        sprintf(aNewMapText, pText, aText);
        SetWindowText(hCtrl, aNewMapText);
    }

    memset(aText, 0, sizeof(aText));
    pText = jonesString_GetString("JONES_STR_FROM");
    if ( pText )
    {
        strcpy(aFormat, pText);
    }
    else
    {
        memset(aFormat, 0, sizeof(aFormat));
    }

    pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pData->actionId]);
    if ( pActionName )
    {
        strcpy(aActionName, pActionName);
    }
    else
    {
        memset(aActionName, 0, sizeof(aActionName));
    }

    if ( aFormat[0] && aActionName[0] )
    {
        hCtrl = GetDlgItem(hDlg, 1073);
        sprintf(aText, aFormat, aActionName);
        SetWindowText(hCtrl, aText);
    }

    memset(aControlName, 0, sizeof(aControlName));
    pText = jonesString_GetString("JONES_STR_TO");
    if ( pText )
    {
        strcpy(aFormat, pText);
    }
    else
    {
        memset(aFormat, 0, sizeof(aFormat));
    }

    pActionName = jonesString_GetString(jonesConfig_aJonesCapControlActionNames[pAssignKeyData->actionId]);
    if ( pActionName )
    {
        strcpy(aActionName, pActionName);
    }
    else
    {
        memset(aActionName, 0, sizeof(aActionName));
    }

    if ( aFormat[0] && aActionName[0] )
    {
        hCtrl = GetDlgItem(hDlg, 1072);
        sprintf(aControlName, aFormat, aActionName);
        SetWindowText(hCtrl, aControlName);
    }

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

void J3DAPI jonesConfig_ReassignKeyDlg_HandleWM_COMMAND(HWND hWnd, unsigned int cntrlId, LPARAM lParam, unsigned int notifyCode)
{
    JonesReAssignKeyDialogData* pInfo = (JonesReAssignKeyDialogData*)GetWindowLong(hWnd, DWL_USER);
    JonesControlsScheme* pScheme = pInfo->pAssignKeyData->pScheme;
    HWND hList;


    switch ( cntrlId )
    {
        case 1: // Yes button
        {
            int numBinds;
            int i;
            hList = GetDlgItem(pInfo->hAssignKeyDlg, pInfo->pAssignKeyData->listID);

            if ( pInfo->pAssignKeyData->listID == 1005 )// Key list
            {
                numBinds = JONESCONTROL_ACTION_GETNUMKEYBOARDBINDS(pScheme->aActions[pInfo->actionId]);
            }
            else
            {
                numBinds = JONESCONTROL_ACTION_GETNUMCONTROLLERBINDS(pScheme->aActions[pInfo->actionId]);
            }

            if ( numBinds > 1 )
            {
                ListView_DeleteItem(hList, pInfo->index);
            }
            else if ( numBinds == 1 )
            {
                LVITEM item;
                item.mask     = LVIF_PARAM;
                item.iItem    = pInfo->index;
                item.iSubItem = 0;
                item.lParam   = pInfo->actionId;
                ListView_SetItem(hList, &item);

                // Set Empty text
                ListView_SetItemText(hList, pInfo->index, 1, "");
            }

            for ( i = pInfo->offset + 1; i < (int)JONESCONTROL_ACTION_GETNUMBINDS(pScheme->aActions[pInfo->actionId]); ++i )
            {
                pScheme->aActions[pInfo->actionId][i] = pScheme->aActions[pInfo->actionId][i + 1];
            }

            if ( pInfo->pAssignKeyData->listID == 1005 ) // key list
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_KEYBOARD(pScheme->aActions[pInfo->actionId], -1);
            }
            else // controller list
            {
                JONESCONTROL_ACTION_UPDATENUMBINDINGS_CONTROLLER(pScheme->aActions[pInfo->actionId], -1);
            }
        }
        // Fall through: close the dialog like No

        case 2: // No button
            EndDialog(hWnd, cntrlId);
            break;
    }
}

// Declared next to its first use
BOOL CALLBACK jonesConfig_DisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitDisplaySettingsDialog(HWND hDlg, int a2, LPARAM lParam);
int J3DAPI jonesConfig_DisplaySettings_GetVideoModeNum(const JonesDisplaySettingsVideoMode* pVideoMode, const char* pText, int numVideoModes);
void J3DAPI jonesConfig_DisplaySettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM a3, int notifyCode);

int J3DAPI jonesConfig_ShowDisplaySettingsDialog(HWND hWnd, StdDisplayEnvironment* pDisplayEnv, JonesDisplaySettings* pDSettings)
{
    float curFogDensity;
    int perfLevel;
    JonesDisplaySettings curSettings;
    int dlgResult;
    HINSTANCE hModule;

    JonesDisplaySettingsDialogData dlgData;

    hModule = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);

    dlgData.pSettings   = pDSettings;
    dlgData.pDisplayEnv = pDisplayEnv;
    dlgData.aVideoModes = NULL;
    dlgData.aCurColorDepthResolutions = NULL;
    dlgData.numCurColorDepthResolutions = 0;
    dlgData.numVideoModes = 0;

    // Save current settings, to restore in case of canceling 
    curFogDensity = sithRender_fogDensity;
    perfLevel = wuRegistry_GetInt("Performance Level", 4);

    // Retail's restore path leaves fogDensity alone (the copy below is dead in release)
    curSettings.bWindowMode = pDSettings->bWindowMode;
    curSettings.bDualMonitor = pDSettings->bDualMonitor;
    curSettings.displayDeviceNum = pDSettings->displayDeviceNum;
    curSettings.videoModeNum = pDSettings->videoModeNum;
    curSettings.device3DNum = pDSettings->device3DNum;
    curSettings.filter = pDSettings->filter;
    curSettings.width = pDSettings->width;
    curSettings.height = pDSettings->height;
    curSettings.bBuffering = pDSettings->bBuffering;
    curSettings.bFog = pDSettings->bFog;
    curSettings.fogDensity = pDSettings->fogDensity;
    curSettings.geoMode = pDSettings->geoMode;
    curSettings.lightMode = pDSettings->lightMode;
    curSettings.bClearBackBuffer = pDSettings->bClearBackBuffer;

    dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(114), hWnd, jonesConfig_DisplaySettingsDialogProc, (LPARAM)&dlgData);
    if ( dlgData.aVideoModes )
    {
        stdMemory_Free(dlgData.aVideoModes);
    }

    if ( dlgData.aCurColorDepthResolutions )
    {
        stdMemory_Free(dlgData.aCurColorDepthResolutions);
    }

    if ( dlgResult == 1 ) // OK btn clicked
    {
        jonesConfig_EnableMouseControl(wuRegistry_GetIntEx("Mouse Control", 0));
    }
    else // Cancel clicked
    {
        //  Restore to previous settings
        sithRender_fogDensity = curFogDensity;
        wuRegistry_SaveInt("Performance Level", perfLevel);

        pDSettings->bWindowMode = curSettings.bWindowMode;
        pDSettings->bDualMonitor = curSettings.bDualMonitor;
        pDSettings->displayDeviceNum = curSettings.displayDeviceNum;
        pDSettings->videoModeNum = curSettings.videoModeNum;
        pDSettings->device3DNum = curSettings.device3DNum;
        pDSettings->filter = curSettings.filter;
        pDSettings->width = curSettings.width;
        pDSettings->height = curSettings.height;
        pDSettings->bBuffering = curSettings.bBuffering;
        pDSettings->bFog = curSettings.bFog;
        pDSettings->geoMode = curSettings.geoMode;
        pDSettings->lightMode = curSettings.lightMode;
        pDSettings->bClearBackBuffer = curSettings.bClearBackBuffer;
    }

    return dlgResult;
}

BOOL CALLBACK jonesConfig_DisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_DisplaySettings_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontDisplaySettingsDlg = jonesConfig_InitDialog(hWnd, 0, 114);
            bResult = jonesConfig_InitDisplaySettingsDialog(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_HSCROLL:
        {
            JonesDisplaySettingsDialogData* pData;
            HWND hCtrl;
            int sliderPos;

            if ( jonesConfig_displaySettings_bUpdateSliderText )
            {
                pData = (JonesDisplaySettingsDialogData*)GetWindowLong(hWnd, DWL_USER);
                jonesConfig_HandleWM_HSCROLL(hWnd, (HWND)lParam, LOWORD(wParam), (short)HIWORD(wParam));

                hCtrl = GetDlgItem(hWnd, 1050);
                sliderPos = (int)SendMessage(hCtrl, TBM_GETPOS, 0, 0);

                hCtrl = GetDlgItem(hWnd, 1097);
                SetWindowText(hCtrl, pData->aVideoModes[sliderPos].aResolutionText);
            }

            bResult = 1;
            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontDisplaySettingsDlg);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitDisplaySettingsDialog(HWND hDlg, int a2, LPARAM lParam)
{
    JonesDisplaySettingsDialogData* pData = (JonesDisplaySettingsDialogData*)lParam;
    JonesDisplaySettings* pSettings = pData->pSettings;
    StdDisplayEnvironment* pDisplayEnv = pData->pDisplayEnv;
    HWND hBtnOk;
    HWND hCBColorDepth;
    StdDisplayInfo* pDisplayInfo;
    StdVideoMode curVideoMode;
    int bHasCurVideoMode;
    int selVideoModeNum = -1;
    const char* pFormat = NULL;
    int selCBColorDepthData;
    int itemData;
    int index;
    int i;
    int j;
    char aResolutionText[256];
    char aResText[256];
    HWND hResText;
    HWND hCtrl;

    hBtnOk = GetDlgItem(hDlg, 1);

    // Init color depth combo-box
    hCBColorDepth = GetDlgItem(hDlg, 1094);
    ComboBox_ResetContent(hCBColorDepth);

    if ( pData->aVideoModes )
    {
        stdMemory_Free(pData->aVideoModes);
        pData->aVideoModes   = NULL;
        pData->numVideoModes = 0;
    }

#line 7376
    pData->aVideoModes = (JonesDisplaySettingsVideoMode*)STDMALLOC(
        sizeof(JonesDisplaySettingsVideoMode) * pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].numModes
    );
    if ( pData->aVideoModes )
    {
        memset(pData->aVideoModes, 0, sizeof(JonesDisplaySettingsVideoMode) * pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].numModes);
    }
    else
    {
        return 0;
    }

    if ( pData->aCurColorDepthResolutions )
    {
        stdMemory_Free(pData->aCurColorDepthResolutions);
        pData->aCurColorDepthResolutions   = NULL;
        pData->numCurColorDepthResolutions = 0;
    }

    pDisplayInfo = &pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum];
    if ( !pDisplayInfo->displayDevice.bHAL )
    {
        EnableWindow(hBtnOk, 0);
        EnableWindow(hCBColorDepth, 0);
    }
    else
    {
        EnableWindow(hBtnOk, 1);
        EnableWindow(hCBColorDepth, 1);
    }

    curVideoMode = pDisplayInfo->aModes[pSettings->videoModeNum];
    bHasCurVideoMode = 0;

    for ( i = 0; i < pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].numModes; ++i )
    {
        pFormat = jonesString_GetString("JONES_STR_BYPIXELS");
        memset(aResolutionText, 0, sizeof(aResolutionText));

        if ( pFormat )
        {
            sprintf(aResolutionText, pFormat, pDisplayInfo->aModes[i].rasterInfo.width, pDisplayInfo->aModes[i].rasterInfo.height);
        }

        if ( pDisplayInfo->aModes[i].aspectRatio == 1.0f && pDisplayInfo->aModes[i].rasterInfo.width >= 512 && pDisplayInfo->aModes[i].rasterInfo.height >= 384 )
        {
            if ( jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP(pDisplayInfo, pSettings, pDisplayInfo->aModes[i].rasterInfo.colorInfo.bpp) )
            {
                int videomodeNum = jonesConfig_DisplaySettings_GetVideoModeNum(pData->aVideoModes, aResolutionText, pData->numVideoModes);
                int colorDepthIdx;
                int colorDepthMask;

                if ( videomodeNum == -1 )
                {
                    continue;
                }

                // Check if mode for this resolution is not assigned yet
                if ( videomodeNum == pData->numVideoModes )
                {
                    strcpy(pData->aVideoModes[videomodeNum].aResolutionText, aResolutionText);
                    pData->numVideoModes++;
                }

                switch ( pDisplayInfo->aModes[i].rasterInfo.colorInfo.bpp )
                {
                    case 16:
                        colorDepthIdx = 2;
                        colorDepthMask = 4;
                        pFormat = jonesString_GetString("JONES_STR_16BBP");
                        if ( pFormat )
                        {
                            sprintf(aResolutionText, pFormat);
                        }

                        break;

                    case 24:
                        colorDepthIdx = 3;
                        colorDepthMask = 8;
                        pFormat = jonesString_GetString("JONES_STR_24BBP");
                        if ( pFormat )
                        {
                            sprintf(aResolutionText, pFormat);
                        }

                        break;

                    case 32:
                        colorDepthIdx   = 4;
                        colorDepthMask = 0x10;
                        pFormat = jonesString_GetString("JONES_STR_32BBP");
                        if ( pFormat )
                        {
                            sprintf(aResolutionText, pFormat);
                        }
                        break;
                }

                pData->aVideoModes[videomodeNum].colorDepthFlags |= colorDepthMask;
                pData->aVideoModes[videomodeNum].aColorDepthVideoModes[colorDepthIdx] = i;

                if ( ComboBox_FindString(hCBColorDepth, 0, aResolutionText) == -1 )
                {
                    itemData = (colorDepthMask << 16) | colorDepthIdx;// Packs pixel byte size and color depth mask
                    index = ComboBox_AddString(hCBColorDepth, aResolutionText);
                    ComboBox_SetItemData(hCBColorDepth, index, itemData);
                }

                if ( pDisplayInfo->aModes[i].rasterInfo.width == curVideoMode.rasterInfo.width
                    && pDisplayInfo->aModes[i].rasterInfo.height == curVideoMode.rasterInfo.height
                    && pDisplayInfo->aModes[i].rasterInfo.colorInfo.bpp == curVideoMode.rasterInfo.colorInfo.bpp )
                {
                    selVideoModeNum     = videomodeNum;
                    selCBColorDepthData = itemData;
                    ComboBox_SetCurSel(hCBColorDepth, index);
                    bHasCurVideoMode = 1;
                }
            }
        }
    }

    // Init screen resolutions for selected color depth

#line 7504
    pData->aCurColorDepthResolutions = (const JonesDisplaySettingsVideoMode**)STDMALLOC(sizeof(const JonesDisplaySettingsVideoMode**) * pData->numVideoModes);
    if ( !pData->aCurColorDepthResolutions )
    {
        return 0;
    }

    if ( !bHasCurVideoMode )
    {
        // No color depth selected, so we select first video mode
        selVideoModeNum     = 0;
        selCBColorDepthData = MAKELONG(0, pData->aVideoModes[0].colorDepthFlags);
    }

    hResText = GetDlgItem(hDlg, 1097);
    GetWindowText(hResText, aResText, STD_ARRAYLEN(aResText));
    memset(pData->aCurColorDepthResolutions, 0, sizeof(const JonesDisplaySettingsVideoMode**) * pData->numVideoModes);

    for ( j = 0; j < pData->numVideoModes; ++j )
    {
        if ( (pData->aVideoModes[j].colorDepthFlags & HIWORD(selCBColorDepthData)) != 0 )
        {
            if ( strlen(aResText) > 0 && streq(aResText, pData->aVideoModes[j].aResolutionText) )
            {
                selVideoModeNum = pData->numCurColorDepthResolutions;
            }
            else if ( selVideoModeNum == j )
            {
                selVideoModeNum = pData->numCurColorDepthResolutions;
            }

            pData->aCurColorDepthResolutions[pData->numCurColorDepthResolutions++] = &pData->aVideoModes[j];
        }
    }

    // Init resolution slider
    hCtrl = GetDlgItem(hDlg, 1050);
    SendMessage(hCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, pData->numCurColorDepthResolutions - 1));
    SendMessage(hCtrl, TBM_SETPOS, /*re-draw=*/TRUE, selVideoModeNum);
    SendMessage(hCtrl, TBM_SETTICFREQ, 1u, 0);

    // Init resolution text control
    hCtrl = GetDlgItem(hDlg, 1097);
    SetWindowText(hCtrl, pData->aCurColorDepthResolutions[selVideoModeNum]->aResolutionText);
    SendMessage(hCtrl, TBM_SETPAGESIZE, 0, 1); // Retail bug: sent to the text control, not the slider

    if ( !bHasCurVideoMode )
    {
        ComboBox_SetCurSel(hCBColorDepth, 0);
    }

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

int J3DAPI jonesConfig_DisplaySettings_GetVideoModeNum(const JonesDisplaySettingsVideoMode* pVideoMode, const char* pText, int numVideoModes)
{
    int i;

    if ( !pText )
    {
        return -1;
    }

    i = 0;
    while ( i < numVideoModes )
    {
        if ( streq(pVideoMode[i].aResolutionText, pText) )
        {
            return i;
        }

        ++i;
    }

    return i;
}

void J3DAPI jonesConfig_DisplaySettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM a3, int notifyCode)
{
    JonesDisplaySettingsDialogData* pData;
    JonesDisplaySettings* pSettings;
    StdDisplayEnvironment* pDisplayEnv;

    pData = (JonesDisplaySettingsDialogData*)GetWindowLong(hWnd, DWL_USER);

    pSettings = pData->pSettings;
    pDisplayEnv = pData->pDisplayEnv;

    switch ( ctrlID )
    {
        case 1099: // Default button
        {
            int i;
            const char* pResFormat;
            HWND hCBColorDepth = GetDlgItem(hWnd, 1094);
            int numColorDepths = ComboBox_GetCount(hCBColorDepth);
            int colorDepthMask;
            size_t selDefaultIdx;

            pResFormat = NULL;
            for ( i = 0; i < numColorDepths; i++ )
            {
                colorDepthMask = ComboBox_GetItemData(hCBColorDepth, i);
                if ( HIWORD(colorDepthMask) == 0x04 )  // 16 bpp
                {
                    ComboBox_SetCurSel(hCBColorDepth, i);
                    break;
                }
            }

            CheckDlgButton(hWnd, 1095, 0); // ??? unknown ctrl, in advance display settings this is very fast performance radio button

            pResFormat = jonesString_GetString("JONES_STR_BYPIXELS");
            if ( pResFormat )
            {
                char aResText[256];
                int j;

                HWND hResSliderCtrl = GetDlgItem(hWnd, 1050);
                HWND hResTextCtrl   = GetDlgItem(hWnd, 1097);

                memset(aResText, 0, sizeof(aResText));
                sprintf(aResText, pResFormat, 640, 480);
                SetWindowText(hResTextCtrl, aResText);

                // Clear cur color depth res array
                memset(pData->aCurColorDepthResolutions, 0, sizeof(pData->aCurColorDepthResolutions) * pData->numCurColorDepthResolutions);
                pData->numCurColorDepthResolutions = 0;

                for ( j = 0; j < pData->numVideoModes; ++j )
                {
                    if ( (pData->aVideoModes[j].colorDepthFlags & 0x04) != 0 )// 16 bpp ie 1 << 2 (bytes)
                    {
                        if ( streq(aResText, pData->aVideoModes[j].aResolutionText) )
                        {
                            selDefaultIdx = pData->numCurColorDepthResolutions;
                        }

                        pData->aCurColorDepthResolutions[pData->numCurColorDepthResolutions++] = &pData->aVideoModes[j];
                    }
                }

                SendMessage(hResSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, pData->numCurColorDepthResolutions - 1));
                SendMessage(hResSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, selDefaultIdx);
                SendMessage(hResSliderCtrl, TBM_SETTICFREQ, 1u, 0);
                SetWindowText(hResTextCtrl, pData->aCurColorDepthResolutions[selDefaultIdx]->aResolutionText);
            }
            break;
        }

        case 1094: // Color depth combo box
            if ( notifyCode == CBN_SELCHANGE )
            {
                // Selection of color depth combo box changed,
                // we update aCurColorDepthResolutions for new the color depth
                size_t newItemIdx;
                char aResText[256];
                int i;

                HWND hCBColorDepth = GetDlgItem(hWnd, 1094);
                HWND hResSliderCtrl = GetDlgItem(hWnd, 1050);
                HWND hResTextCtrl = GetDlgItem(hWnd, 1097);// resolution text control

                int colorDepthMask = ComboBox_GetItemData(hCBColorDepth, ComboBox_GetCurSel(hCBColorDepth));

                memset(aResText, 0, sizeof(aResText));

                // Clear cur color depth res array
                memset(pData->aCurColorDepthResolutions, 0, sizeof(pData->aCurColorDepthResolutions) * pData->numCurColorDepthResolutions);

                GetWindowText(hResTextCtrl, aResText, STD_ARRAYLEN(aResText));
                pData->numCurColorDepthResolutions = 0;

                for ( i = 0; i < pData->numVideoModes; ++i )
                {
                    if ( (pData->aVideoModes[i].colorDepthFlags & HIWORD(colorDepthMask)) != 0 )
                    {
                        if ( streq(aResText, pData->aVideoModes[i].aResolutionText) )
                        {
                            newItemIdx = pData->numCurColorDepthResolutions;
                        }

                        pData->aCurColorDepthResolutions[pData->numCurColorDepthResolutions++] = &pData->aVideoModes[i];
                    }
                }

                SendMessage(hResSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, pData->numCurColorDepthResolutions - 1));
                SendMessage(hResSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, newItemIdx);
                SendMessage(hResSliderCtrl, TBM_SETTICFREQ, 1u, 0);
                SetWindowText(hResTextCtrl, pData->aCurColorDepthResolutions[newItemIdx]->aResolutionText);
            }
            break;

        case 1093: // Advance settings clicked
            jonesConfig_ShowAdvanceDisplaySettings(hWnd, pData);
            InvalidateRect(hWnd, 0, 1);
            break;

        case 1: // OK btn
        {
            // Save new screen resolution data

            HWND hCBColorDepth = GetDlgItem(hWnd, 1094);// color depth combo box
            HWND hResSliderCtrl = GetDlgItem(hWnd, 1050);// resolution slider control
            int curSliderPos = (int)SendMessage(hResSliderCtrl, TBM_GETPOS, 0, 0);
            int itemData = ComboBox_GetItemData(hCBColorDepth, ComboBox_GetCurSel(hCBColorDepth));

            pSettings->videoModeNum = pData->aCurColorDepthResolutions[curSliderPos]->aColorDepthVideoModes[itemData & 0xFFFF];

            pSettings->width = pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].rasterInfo.width;
            pSettings->height = pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].rasterInfo.height;

            wuRegistry_SaveStr("Display", pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].displayDevice.aDriverName);
            wuRegistry_SaveStr("3D Device", pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aDevices[pSettings->device3DNum].deviceDescription);
            wuRegistry_SaveInt("Width", pSettings->width);
            wuRegistry_SaveInt("Height", pSettings->height);
            wuRegistry_SaveInt("BPP", pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum].aModes[pSettings->videoModeNum].rasterInfo.colorInfo.bpp);

            // Set new fog config

            wuRegistry_SaveFloat("Fog Density", sithRender_fogDensity / 100.0f);

            wuRegistry_SaveIntEx("Buffering", pSettings->bBuffering);
            wuRegistry_SaveIntEx("Fog", pSettings->bFog);
            std3D_EnableFog(pSettings->bFog, sithRender_fogDensity);

            if ( sithWorld_pCurrentWorld )
            {
                sithWorld_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
            }

            wuRegistry_SaveInt("Filter", pSettings->filter);
        }
        // Fall through: OK closes the dialog like Cancel

        case 2: // Cancel btn
            EndDialog(hWnd, ctrlID);
            break;
    }
}

int J3DAPI jonesConfig_ShowAdvanceDisplaySettings(HWND hWnd, JonesDisplaySettingsDialogData* pData)
{
    int result;

    result = JonesDialog_ShowDialog(MAKEINTRESOURCE(148), hWnd, jonesConfig_AdvanceDisplaySettingsDialogProc, (LPARAM)pData); /* retail bug: result is never returned; the debug build is the same */
}

BOOL CALLBACK jonesConfig_AdvanceDisplaySettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontAdvanceDisplaySettingsDialog = jonesConfig_InitDialog(hWnd, 0, 148);
            bResult = jonesConfig_InitAdvanceDisplaySettingsDialog(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontAdvanceDisplaySettingsDialog);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_DisplaySettings_Get3DDeviceSupportsBPP(const StdDisplayInfo* pDisplayInfo, const JonesDisplaySettings* pSettings, int bpp)
{
    if ( !pDisplayInfo )
    {
        return 0;
    }

    switch ( bpp )
    {
        case 32:
            return pDisplayInfo->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_32;

        case 24:
            return pDisplayInfo->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_24;

        case 16:
            return pDisplayInfo->aDevices[pSettings->device3DNum].d3dDesc.dwDeviceRenderBitDepth & DDBD_16;
    }

    return 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(jonesConfig_countfit_3, 30, 15) CF_PROTO(jonesConfig_countfit_18, 15) /* end count-fit */
int J3DAPI jonesConfig_InitAdvanceDisplaySettingsDialog(HWND hDlg, int a2, LPARAM lParam)
{
    JonesDisplaySettingsDialogData* pData = (JonesDisplaySettingsDialogData*)lParam;
    JonesDisplaySettings* pSettings;
    StdDisplayEnvironment* pDisplayEnv;
    char aSelDeviceDesc[256];
    int newItemIdx;
    int bCurSel;
    HWND hCB3DeviceCtrl;
    HWND hCB3DRasterCtrl;
    StdDisplayInfo* pDisplayInfo;
    HWND hFogSliderCtrl;
    size_t i;


    pSettings   = pData->pSettings;
    pDisplayEnv = pData->pDisplayEnv;

    // Init 3D dwevice combo box
    hCB3DeviceCtrl = GetDlgItem(hDlg, 1084);    // 3D Devices combo box
    ComboBox_ResetContent(hCB3DeviceCtrl);

    for ( i = 0; i < pDisplayEnv->numInfos; ++i )
    {
        char aDBEntryText[256];

        memset(aDBEntryText, 0, sizeof(aDBEntryText));

        if ( pDisplayEnv->aDisplayInfos[i].displayDevice.bHAL == 1 )
        {
            sprintf(aDBEntryText, "%s", pDisplayEnv->aDisplayInfos[i].displayDevice.aDriverName);
            newItemIdx = ComboBox_AddString(hCB3DeviceCtrl, aDBEntryText);

            ComboBox_SetItemData(hCB3DeviceCtrl, newItemIdx, i); // i is index in aDisplayInfos

            if ( i == pSettings->displayDeviceNum )
            {
                ComboBox_SetCurSel(hCB3DeviceCtrl, newItemIdx);
            }
        }
    }

    // Init 3D Raster combo box
    hCB3DRasterCtrl = GetDlgItem(hDlg, 1085);

    memset(aSelDeviceDesc, 0, sizeof(aSelDeviceDesc));
    GetWindowText(hCB3DRasterCtrl, aSelDeviceDesc, STD_ARRAYLEN(aSelDeviceDesc));
    ComboBox_ResetContent(hCB3DRasterCtrl);

    pDisplayInfo = &pDisplayEnv->aDisplayInfos[pSettings->displayDeviceNum];
    if ( !pDisplayInfo->displayDevice.bHAL )
    {
        EnableWindow(hCB3DRasterCtrl, 0);
    }
    else
    {
        EnableWindow(hCB3DRasterCtrl, 1);

        bCurSel = 0;
        for ( i = 0; i < pDisplayInfo->numDevices; ++i )
        {
            char aDBEntryText[256];

            memset(aDBEntryText, 0, sizeof(aDBEntryText));

            sprintf(aDBEntryText, "%s", pDisplayInfo->aDevices[i].deviceDescription);
            newItemIdx = ComboBox_AddString(hCB3DRasterCtrl, aDBEntryText);

            ComboBox_SetItemData(hCB3DRasterCtrl, newItemIdx, i); // i is index in aDevices

            if ( streq(aSelDeviceDesc, aDBEntryText) || pSettings->device3DNum == i )
            {
                ComboBox_SetCurSel(hCB3DRasterCtrl, newItemIdx);
                pSettings->device3DNum = i;
                bCurSel = 1;
            }
        }

        if ( !bCurSel )
        {
            // Set first device as selected
            ComboBox_SetCurSel(hCB3DRasterCtrl, 0);
            pSettings->device3DNum = 0;
        }
    }

    // Init dialog radio buttons

    if ( pSettings->bBuffering )
    {
        // Tripple buffering
        CheckRadioButton(hDlg, 1091, 1092, 1092);
    }
    else
    {
        // Double buffering
        CheckRadioButton(hDlg, 1091, 1092, 1091);
    }

    // Set checked mipmap radio button
    switch ( pSettings->filter )
    {
        case STD3D_MIPMAPFILTER_NONE:
            CheckRadioButton(hDlg, 1088, 1090, 1088);
            break;

        case STD3D_MIPMAPFILTER_BILINEAR:
            CheckRadioButton(hDlg, 1088, 1090, 1089);
            break;

        case STD3D_MIPMAPFILTER_TRILINEAR:
            CheckRadioButton(hDlg, 1088, 1090, 1090);
            break;
    }

    switch ( wuRegistry_GetInt("Performance Level", 4) )
    {
        case 0:
            CheckRadioButton(hDlg, 1093, 1095, 1093); // average
            break;

        case 2:
            CheckRadioButton(hDlg, 1093, 1095, 1094); // fast
            break;

        case 4:
            CheckRadioButton(hDlg, 1093, 1095, 1095); // very fast
            break;
    }

    // Set fog check box
    CheckDlgButton(hDlg, 1051, pSettings->bFog);

    // Set fog slider
    hFogSliderCtrl = GetDlgItem(hDlg, 1216);
    SendMessage(hFogSliderCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, 100));
    SendMessage(hFogSliderCtrl, TBM_SETTICFREQ, 5u, 0);
    SendMessage(hFogSliderCtrl, TBM_SETPOS, /*re-draw=*/TRUE, (LPARAM)sithRender_fogDensity);
    SendMessage(hFogSliderCtrl, TBM_SETPAGESIZE, 0, 5u);

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

void J3DAPI jonesConfig_AdvanceDisplaySettings_HandleWM_COMMAND(HWND hDlg, int ctrlID, int a3, int notifyCode)
{
    JonesDisplaySettingsDialogData* pData;
    StdDisplayEnvironment* pDisplayEnvironment;
    JonesDisplaySettings* pSettings;


    pData = (JonesDisplaySettingsDialogData*)GetWindowLong(hDlg, DWL_USER);
    pSettings = pData->pSettings;
    pDisplayEnvironment = pData->pDisplayEnv;

    switch ( ctrlID )
    {
        case 1099: // Default button
        {
            CheckDlgButton(hDlg, 1051, 1u); // Fog check box
            pSettings->bFog = 1;
            sithRender_fogDensity = 100.0f;

            // Fog slider
            SendMessage(GetDlgItem(hDlg, 1216), TBM_SETPOS, /*re-draw=*/TRUE, (LPARAM)sithRender_fogDensity);

            // Perf. level set to very fast
            CheckRadioButton(hDlg, 1093, 1095, 1095);
            jonesConfig_advanceDisplaySettings_perfLevel = 4;

            // Set default to no buffering
            CheckRadioButton(hDlg, 1091, 1092, 1091);
            pSettings->bBuffering = 0;

            // Set default to bilinear
            CheckRadioButton(hDlg, 1088, 1090, 1089);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_BILINEAR;
            break;
        }

        case 1084: // 3D Devices combo box
        {

            if ( notifyCode == CBN_SELCHANGE )
            {
                HWND hCB3DDevice = GetDlgItem(hDlg, 1084);
                HWND hDisplaySettingsDlg = GetParent(hDlg);

                int currentSelIdx = ComboBox_GetCurSel(hCB3DDevice);
                pSettings->displayDeviceNum = ComboBox_GetItemData(hCB3DDevice, currentSelIdx);

                jonesConfig_InitAdvanceDisplaySettingsDialog(hDlg, 0, (LPARAM)pData);
                jonesConfig_InitDisplaySettingsDialog(hDisplaySettingsDlg, 0, (LPARAM)pData);
            }
        } break;

        case 1085: // 3D Rasterizers combo box
        {
            if ( notifyCode == CBN_SELCHANGE )
            {
                HWND hCB3DDevice = GetDlgItem(hDlg, 1084);
                HWND hDisplaySettingsDlg = GetParent(hDlg);

                int currentSelIdx = ComboBox_GetCurSel(hCB3DDevice);
                pSettings->device3DNum = ComboBox_GetItemData(hCB3DDevice, currentSelIdx);

                jonesConfig_InitAdvanceDisplaySettingsDialog(hDlg, 0, (LPARAM)pData);
                jonesConfig_InitDisplaySettingsDialog(hDisplaySettingsDlg, 0, (LPARAM)pData);
            }
        } break;

        case 1088: // No Mipmap radio-button
            CheckRadioButton(hDlg, 1088, 1090, 1088);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_NONE;
            break;

        case 1090: // Trilinear radio-button
            CheckRadioButton(hDlg, 1088, 1090, 1090);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_TRILINEAR;
            break;

        case 1089: // Bilinear radio-button
            CheckRadioButton(hDlg, 1088, 1090, 1089);
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_BILINEAR;
            break;

        case 1091: // Dbl buffer radio-button
            CheckRadioButton(hDlg, 1091, 1092, 1091);
            pSettings->bBuffering = 0;
            break;

        case 1092: // Tripple buffer
            CheckRadioButton(hDlg, 1091, 1092, 1092);
            pSettings->bBuffering = 1;
            break;

        case 1093: // Average perf. level radio button
            CheckRadioButton(hDlg, 1093, 1095, 1093);
            jonesConfig_advanceDisplaySettings_perfLevel = 0;
            break;

        case 1094: // Fast perf. level radio button
            CheckRadioButton(hDlg, 1093, 1095, 1094);
            jonesConfig_advanceDisplaySettings_perfLevel = 2;
            break;

        case 1095: // Very fast perf. level radio button
            CheckRadioButton(hDlg, 1093, 1095, 1095);
            jonesConfig_advanceDisplaySettings_perfLevel = 4;
            break;

        case 1: // OK btn
        {
            pSettings->bFog =  Button_GetCheck(GetDlgItem(hDlg, 1051));

            sithRender_fogDensity = (float)SendMessage(GetDlgItem(hDlg, 1216), TBM_GETPOS, 0, 0);
            pSettings->fogDensity = sithRender_fogDensity / 100.0f;

            wuRegistry_SaveInt("Performance Level", jonesConfig_advanceDisplaySettings_perfLevel);
            jonesConfig_advanceDisplaySettings_perfLevel = 4;

            if ( JonesMain_HasStarted() && jonesConfig_advanceDisplaySettings_perfLevel != wuRegistry_GetInt("Performance Level", 4) )
            {
                // Performance changed, show msg box informing that change will take into affect after game restart
                const char* pMsgText = jonesString_GetString("JONES_STR_PERFORMANCE");
                if ( pMsgText )
                {
                    jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_DPLY_OPTS", pMsgText, 136); // 136 - tv icon
                    InvalidateRect(hDlg, 0, 1);
                }
            }

            pSettings->filter = jonesConfig_advanceDisplaySettings_curFilterMode;
            jonesConfig_advanceDisplaySettings_curFilterMode = STD3D_MIPMAPFILTER_BILINEAR;
        }

            // Note falls through to the cancel case's EndDialog (retail 0x0040C19F)

        case 2: // Cancel btn
            EndDialog(hDlg, ctrlID);
            break;

        default:
            return;
    }

}

// Declared next to its first use
BOOL CALLBACK jonesConfig_SoundSettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitSoundSettingsDialog(HWND hDlg, int a2, LPARAM lParam);
void J3DAPI jonesConfig_SoundSettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode);

int J3DAPI jonesConfig_ShowSoundSettingsDialog(HWND hWnd, JonesSoundSettings* pData)
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    int dlgResult = JonesDialog_ShowDialog(MAKEINTRESOURCE(113), hWnd, jonesConfig_SoundSettingsDialogProc, (LPARAM)pData);
    return dlgResult;
}

BOOL CALLBACK jonesConfig_SoundSettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_SoundSettings_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontSoundSettings = jonesConfig_InitDialog(hWnd, 0, 113);
            bResult = jonesConfig_InitSoundSettingsDialog(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_HSCROLL:
        {
            tSoundHandle hSnd;
            HWND hVolSliderCtrl;
            int curPos;

            jonesConfig_HandleWM_HSCROLL(hWnd, (HWND)lParam, LOWORD(wParam), (short)HIWORD(wParam));
            if ( LOWORD(wParam) == SB_ENDSCROLL )
            {
                // Play test sound with new volume
                hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(57)); // 57 - mus_find.wav
                if ( jonesConfig_soundSettings_hSndChannel && (Sound_GetChannelFlags(jonesConfig_soundSettings_hSndChannel) & SOUND_CHANNEL_PLAYING) != 0 )
                {
                    sithSoundMixer_StopSound(jonesConfig_soundSettings_hSndChannel);
                }

                jonesConfig_soundSettings_hSndChannel = 0;
                if ( hSnd )
                {
                    hVolSliderCtrl = GetDlgItem(hWnd, 1050);
                    curPos = SendMessage(hVolSliderCtrl, TBM_GETPOS, 0, 0);
                    if ( jonesConfig_soundSettings_curSoundVolume != curPos )
                    {
                        jonesConfig_soundSettings_hSndChannel = sithSoundMixer_PlaySound(hSnd, D3DVAL(curPos / 100.0), 0.0f, (SoundPlayFlag)0);
                        jonesConfig_soundSettings_curSoundVolume = curPos;
                    }
                }
            }

            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontSoundSettings);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitSoundSettingsDialog(HWND hDlg, int a2, LPARAM lParam)
{
    JonesSoundSettings* pData = (JonesSoundSettings*)lParam;
    HWND hCtrl;
    float curVolume;

    hCtrl = GetDlgItem(hDlg, 1050);
    SendMessage(hCtrl, TBM_SETRANGE, /*re-draw=*/TRUE, MAKELONG(0, 100));
    SendMessage(hCtrl, TBM_SETTICFREQ, 17u, 0);
    curVolume = pData->maxSoundVolume;
    SendMessage(hCtrl, TBM_SETPOS, /*re-draw=*/TRUE, (LPARAM)(curVolume * 100.0f));
    SendMessage(hCtrl, TBM_SETPAGESIZE, 0, 5);

    // Set 3D sound check box
    hCtrl = GetDlgItem(hDlg, 1118);
    if ( !Sound_Has3DHW() )
    {
        CheckDlgButton(hDlg, 1118, 0);
        EnableWindow(hCtrl, 0);
    }
    else
    {
        if ( pData->b3DHWSupport )
        {
            CheckDlgButton(hDlg, 1118, 1);
        }
        else
        {
            CheckDlgButton(hDlg, 1118, 0);
        }
    }

    // Set reverse sound check box
    hCtrl = GetDlgItem(hDlg, 1051);
    CheckDlgButton(hDlg, 1051, pData->bReverseSound);

    SetWindowLong(hDlg, DWL_USER, (LONG)pData);
    return 1;
}

void J3DAPI jonesConfig_SoundSettings_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode)
{
    JonesSoundSettings* pCurSettings;
    HWND hCtrl;
    int cursliderPos;

    pCurSettings = (JonesSoundSettings*)GetWindowLong(hWnd, DWL_USER);

    switch ( ctrlID )
    {
        case 1: // OK
            hCtrl = GetDlgItem(hWnd, 1050);
            cursliderPos = SendMessage(hCtrl, TBM_GETPOS, 0, 0);
            pCurSettings->maxSoundVolume = (float)(cursliderPos / 100.0);

            hCtrl = GetDlgItem(hWnd, 1118);
            pCurSettings->b3DHWSupport = Button_GetState(hCtrl);

            wuRegistry_SaveFloat("Sound Volume", (float)(cursliderPos / 100.0));
            wuRegistry_SaveIntEx("Sound 3D", pCurSettings->b3DHWSupport);

            hCtrl = GetDlgItem(hWnd, 1051);
            pCurSettings->bReverseSound = Button_GetState(hCtrl);
            wuRegistry_SaveIntEx("ReverseSound", pCurSettings->bReverseSound);

            // Note falls through to the cancel case's EndDialog

        case 2: // Cancel btn
            EndDialog(hWnd, ctrlID);
            break;
    }
}

int J3DAPI jonesConfig_ShowGameOverDialog(HWND hWnd, char* pRestoreFilename, tSoundHandle hSndGameOVerMus, tSoundChannelHandle* pSndChnlMus)
{
    GameOverDialogData data;
    int curresult;
    memset(&data, 0, sizeof(data));

    data.pRestoreFilename = pRestoreFilename;
    data.sndChnlMusic     = *pSndChnlMus;
    data.hSndMusic        = hSndGameOVerMus;

    curresult = JonesDialog_ShowDialog(MAKEINTRESOURCE(150), hWnd, jonesConfig_GameOverDialogProc, (LPARAM)&data);
    return data.result;
}

BOOL CALLBACK jonesConfig_GameOverDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_GameOverDialog_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontGameOverDialog = jonesConfig_InitDialog(hWnd, 0, 150);
            bResult = jonesConfig_GameOverDialogInit(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontGameOverDialog);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

void J3DAPI jonesConfig_LoadGameGetLastSavedGamePath(char* pPath, unsigned int size)
{
    const char* pLastFile;
    int filenameLength;
    size_t nameLen;
    const char* pChar;
    char* pFound;
    char aFileName[128];
    char aLastDir[128];

    pLastFile = sithGamesave_GetLastFilename();
    if ( pLastFile && strlen(pLastFile) > 0 )
    {
        nameLen = strlen(pLastFile);
        pChar = &pLastFile[nameLen - 1];
        for ( filenameLength = nameLen; filenameLength >= 0; --filenameLength )
        {
            if ( *pChar != '\\' )
            {
                --pChar;
                if ( filenameLength < 0 ) // Retail bug: never true inside the loop (retail still tests it, 0x00408BEE)
                {
                    pChar = NULL;
                }
            }
            else
            {
                ++pChar;
                break;
            }
        }

        if ( pChar )
        {
            memset(aLastDir, 0, sizeof(aLastDir));
            memset(aFileName, 0, sizeof(aFileName));
            strncpy(aLastDir, pLastFile, pChar - pLastFile);
            strncpy(aFileName, pChar, strlen(pChar));
            SearchPath(aLastDir, aFileName, 0, 128u, pPath, &pFound);
        }
        else
        {
            SearchPath(sithGetSaveGamesDir(), pLastFile, 0, 128u, pPath, &pFound);
        }
    }

    if ( strlen(pPath) == 0 )
    {
        memset(aFileName, 0, sizeof(aFileName));
        memset(pPath, 0, size);
        if ( sithWorld_pCurrentWorld )
        {
            sprintf(aFileName, "%s%s", sithGetAutoSaveFilePrefix(), sithGetCurrentWorldSaveName());
            stdFnames_ChangeExt(aFileName, "nds");
            SearchPath(sithGetSaveGamesDir(), aFileName, 0, 128u, pPath, &pFound);
        }
    }
}

int J3DAPI jonesConfig_GameOverDialogInit(HWND hDlg, int a2, LPARAM lParam)
{
    GameOverDialogData* pData = (GameOverDialogData*)lParam;
    char paFileName[128];
    char aFilename[128];
    LPSTR pFilePart;

    memset(paFileName, 0, sizeof(paFileName));
    memset(aFilename, 0, sizeof(aFilename));

    if ( sithWorld_pCurrentWorld )
    {
        sprintf(aFilename, "%s%s", sithGetAutoSaveFilePrefix(), sithGetCurrentWorldSaveName());
        stdFnames_ChangeExt(aFilename, "nds");
        SearchPath(sithGetSaveGamesDir(), aFilename, NULL, STD_ARRAYLEN(paFileName), paFileName, &pFilePart);
    }

    if ( strlen(paFileName) == 0 )
    {
        // Disable restart btn
        EnableWindow(GetDlgItem(hDlg, 1177), 0); // 1177 - Restart
    }

    if ( pData->hSndMusic )
    {
        pData->sndChnlMusic = sithSoundMixer_PlaySound(pData->hSndMusic, 1.0f, 0.0f, (SoundPlayFlag)0);
    }

    strcpy(pData->pRestoreFilename, paFileName);
    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
    return 1;
}

void J3DAPI jonesConfig_GameOverDialog_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode)
{
    GameOverDialogData* pData = (GameOverDialogData*)GetWindowLong(hWnd, DWL_USER);

    switch ( ctrlID )
    {
        case 1177u:
            pData->result = 1177; // restart
            EndDialog(hWnd, 1);
            break;

        case 1179u:
            pData->result = 1179;
            EndDialog(hWnd, 1);
            break;

        case 1178u:
            pData->result = 1178; // load game
            if ( jonesConfig_GetLoadGameFilePath(hWnd, pData->pRestoreFilename) == 1 )
            {
                InvalidateRect(hWnd, 0, 1);
                EndDialog(hWnd, 1);
            }

            break;
    }
}

int J3DAPI jonesConfig_ShowStatisticsDialog(HWND hWnd, SithGameStatistics* pStatistics)
{
    int dwInitArg[3];
    JonesDialogImageInfo imageInfo;
    int curresult;

    if ( !pStatistics )
    {
        return 2;
    }

    dwInitArg[0] = (int)&imageInfo;
    dwInitArg[1] = (int)pStatistics;
    memset((void*)dwInitArg[0], 0, sizeof(imageInfo));
    curresult = JonesDialog_ShowDialog(MAKEINTRESOURCE(164), hWnd, jonesConfig_StatisticsDialogProc, (LPARAM)dwInitArg);
    return curresult;
}
 
BOOL CALLBACK jonesConfig_StatisticsDialogProc(HWND hwnd, UINT uMsg, WPARAM wPAram, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_StatisticProc_HandleWM_COMMAND(hwnd, LOWORD(wPAram), lParam, HIWORD(wPAram));
            return 0;

        case WM_INITDIALOG:
        {
            const char* pTitleText = NULL;
            HWND hTextStatTitle    = GetDlgItem(hwnd, 1158);

            jonesConfig_hFontGameStatisticsDialog = jonesConfig_InitDialog(hwnd, 0, 164);
            pTitleText = jonesString_GetString(jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_BONUSLEVELNUM]);
            if ( pTitleText )
            {
                SetWindowText(hTextStatTitle, pTitleText);
            }

            bResult = jonesConfig_InitStatisticDialog(hwnd, wPAram, lParam);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;
        }

        case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* pDrawItem;
            int ctlID;
            int* pData;
            JonesDialogImageInfo* pImageInfo;
            RECT rect;

            pDrawItem  = (DRAWITEMSTRUCT*)lParam;
            ctlID      = wPAram;
            pData      = (int*)GetWindowLong(hwnd, DWL_USER);
            pImageInfo = *(JonesDialogImageInfo**)pData;
            if ( !pImageInfo->unknown4 )
            {
                pImageInfo->unknown4 = GetBkColor(pDrawItem->hDC);
            }

            GetClientRect(pDrawItem->hwndItem, &rect);
            FillRect(pDrawItem->hDC, &rect, CreateSolidBrush(pImageInfo->unknown4));
            SetStretchBltMode(pDrawItem->hDC, STRETCH_HALFTONE);
            jonesConfig_DrawImageOnDialogItem(
                hwnd,
                pImageInfo->hdcFront,
                pDrawItem->hDC,
                ctlID,
                jonesConfig_apDlgIcons[0],// iq.bmp
                jonesConfig_apDlgIcons[1]);// iq_mask.bmp
            break;
        }

        case WM_HSCROLL:
        {
            int* pData;
            HWND hScroll;
            HWND hTextStatTitle;
            int scrollPos;
            int minPos;
            int maxPos;

            pData          = (int*)GetWindowLong(hwnd, DWL_USER);
            hScroll        = GetDlgItem(hwnd, 1162);
            hTextStatTitle = GetDlgItem(hwnd, 1158);
            jonesConfig_StatisticProc_HandleWM_HSCROLL((int)hwnd, (HWND)lParam, LOWORD(wPAram), (short)HIWORD(wPAram));
            scrollPos = GetScrollPos(hScroll, SB_CTL);
            if ( scrollPos != pData[2] )
            {
                const char* pTitleText = NULL;
                int levelNum;
                WPARAM iqpoints;
                DRAWITEMSTRUCT drawItem;

                GetScrollRange(hScroll, SB_CTL, &minPos, &maxPos);
                if ( scrollPos == maxPos )
                {
                    pTitleText = jonesString_GetString(jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_BONUSLEVELNUM]);
                    levelNum   = 17;
                    iqpoints   = jonesInventory_GetTotalIQPoints();
                }
                else
                {
                    if ( scrollPos == jonesConfig_gameStatistics_curLevelNum && jonesConfig_gameStatistics_curLevelNum > 0 )
                    {
                        pTitleText = jonesString_GetString(jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_LASTLEVELNUM]);
                    }
                    else
                    {
                        pTitleText = jonesString_GetString(jonesConfig_gameStatistics__aLevelNames[scrollPos]);
                    }

                    levelNum = scrollPos;
                    iqpoints = ((SithGameStatistics*)pData[1])->aLevelStatistic[scrollPos].iqPoints;
                }

                if ( pTitleText )
                {
                    SetWindowText(hTextStatTitle, pTitleText);
                }
                else if ( scrollPos == jonesConfig_gameStatistics_curLevelNum && jonesConfig_gameStatistics_curLevelNum > 0 )
                {
                    SetWindowText(hTextStatTitle, jonesConfig_gameStatistics__aLevelNames[JONESLEVEL_LASTLEVELNUM]);
                }
                else
                {
                    SetWindowText(hTextStatTitle, jonesConfig_gameStatistics__aLevelNames[levelNum]);
                }

                jonesConfig_SetStatisticsDialogForLevel(hwnd, levelNum, pData);
                jonesConfig_DrawStatisticDialogIQPoints(hwnd, (JonesDialogImageInfo**)pData, 164, iqpoints);

                drawItem.CtlType    = ODT_BUTTON;
                drawItem.CtlID      = 1220;
                drawItem.itemID     = 0;
                drawItem.itemAction = ODA_DRAWENTIRE;
                drawItem.itemState  = 0;
                drawItem.hwndItem   = GetDlgItem(hwnd, 1220);
                drawItem.hDC        = GetDC(drawItem.hwndItem);
                GetClientRect(drawItem.hwndItem, &drawItem.rcItem);
                drawItem.itemData = 0;

                SendMessage(hwnd, WM_DRAWITEM, 1220u, (LPARAM)&drawItem);
                ReleaseDC(drawItem.hwndItem, drawItem.hDC);
                pData[2] = scrollPos;
            }

            bResult = 1;
            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hwnd, (HFONT)jonesConfig_hFontGameStatisticsDialog);
            break;

        case WM_CLOSE:
            EndDialog(hwnd, 2);
            bResult = 0;
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

void J3DAPI jonesConfig_StatisticProc_HandleWM_HSCROLL(int a1, HWND hWnd, UINT sbCode, int nThumbPos)
{
    int curPos;
    SCROLLINFO scrlInfo;
    int posMin;
    int maxPos;
    int posNew;
    int pageSize;

    posNew = -1;
    scrlInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS;
    SendMessage(hWnd, SBM_GETSCROLLINFO, 0, (LPARAM)&scrlInfo);

    posMin   = scrlInfo.nMin;
    maxPos   = scrlInfo.nMax;
    curPos   = scrlInfo.nPos;
    pageSize = scrlInfo.nPage;

    switch ( sbCode )
    {
        case SB_BOTTOM:
            posNew = maxPos;
            break;

        case SB_LINEUP:
            posNew = max(posMin, curPos - 1);
            break;

        case SB_LINEDOWN:
            posNew = min(maxPos, curPos + 1);
            break;

        case SB_PAGEUP:
            posNew = max(posMin, curPos - pageSize);
            break;

        case SB_PAGEDOWN:
            posNew = min(maxPos, curPos + pageSize);
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            posNew = nThumbPos;
            break;

        case SB_TOP:
            posNew = posMin;
            break;

        default:
            break;
    }

    if ( posNew >= 0 )
    {
        SetScrollPos(hWnd, SB_CTL, posNew, TRUE);
    }
}

void J3DAPI jonesConfig_UpdateCurrentLevelNum(void)
{
    jonesConfig_gameStatistics_curLevelNum = JonesMain_GetCurrentLevelNum();
}

int J3DAPI jonesConfig_DrawImageOnDialogItem(HWND hDlg, HDC hdcWnd, HDC hdcCtrl, int nIDDlgItem, HBITMAP hImage, HBITMAP hMask)
{
    BITMAP bmp;

    RECT rect;
    GetClientRect(GetDlgItem(hDlg, nIDDlgItem), &rect);

    
    GetObject(hImage, sizeof(BITMAP), &bmp);

    if ( !hMask )
    {
        SelectObject(hdcWnd, (HGDIOBJ)hImage);
        return StretchBlt(hdcCtrl, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcWnd, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }
    else
    {
        SelectObject(hdcWnd, (HGDIOBJ)hMask);
        StretchBlt(hdcCtrl, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcWnd, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCAND);

        SelectObject(hdcWnd, (HGDIOBJ)hImage);
        return StretchBlt(hdcCtrl, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hdcWnd, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCPAINT);
    }
}

int J3DAPI jonesConfig_SetStatisticsDialogForLevel(HWND hDlg, int levelNum, int* a3)
{
    int curv3;
    int curv4;
    SithLevelStatistic* pLevelStatistics;
    SithGameStatistics* pStatistics;
    char aText[256];

    pStatistics = (SithGameStatistics*)a3[1];
    pLevelStatistics = &pStatistics->aLevelStatistic[levelNum];
    curv3 = pLevelStatistics->elapsedTime >> 8;
    curv4 = pLevelStatistics->elapsedTime ^ (curv3 << 8);
    memset(aText, 0, sizeof(aText));
    if ( curv4 < 10 )
    {
        sprintf(aText, "%i : 0%i ", curv3, curv4);
    }
    else
    {
        sprintf(aText, "%i : %i ", curv3, curv4);
    }

    SetWindowText(GetDlgItem(hDlg, 1153), aText);

    memset(aText, 0, sizeof(aText));
    sprintf(aText, "%i ", pStatistics->curLevelNum);
    SetWindowText(GetDlgItem(hDlg, 1152), aText);

    memset(aText, 0, sizeof(aText));
    sprintf(aText, "%i ", pLevelStatistics->numFoundTreasures);
    SetWindowText(GetDlgItem(hDlg, 1154), aText);

    memset(aText, 0, sizeof(aText));
    sprintf(aText, "%i ", pLevelStatistics->numSeenHints);
    SetWindowText(GetDlgItem(hDlg, 1155), aText);

    memset(aText, 0, sizeof(aText));
    sprintf(aText, "%i ", pLevelStatistics->difficultyPenalty);
    return SetWindowText(GetDlgItem(hDlg, 1156), aText);
}


void J3DAPI jonesConfig_DrawStatisticDialogIQPoints(HWND hwnd, JonesDialogImageInfo** ppImageInfo, int dlgID, int iqpoints)
{
    SithGameStatistics* pStatistics;
    JonesDialogImageInfo* pImageInfo;
    int points;
    int width;
    int height;
    int glyphHeight;
    int digit;
    void* ppvBits;
    int x;
    int y;
    int dstX;
    BITMAPINFO bmi;
    RECT rc;

    pStatistics = (SithGameStatistics*)ppImageInfo[1]; // debug build reads the caller's statistics pointer; unused
    pImageInfo  = *ppImageInfo;
    if ( pImageInfo->hBmp )
    {
        DeleteObject((HGDIOBJ)pImageInfo->hBmp);
        pImageInfo->hBmp = 0;
    }

    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDlgIcons[0]);
    BitBlt(pImageInfo->hdcFront, 39, 207, 127, 46, NULL, 0, 0, BLACKNESS);

    // TODO: add iqoverlay.bmp (icons[2])

    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDlgIcons[1]);
    BitBlt(pImageInfo->hdcFront, 39, 207, 127, 46, NULL, 0, 0, WHITENESS);

    points = iqpoints;
    if ( iqpoints < 0 )
    {
        iqpoints = 0;
    }

    // Calculate required width/height
    width       = 0;
    height      = 0;
    glyphHeight = 0;
    while ( iqpoints >= 0 )
    {
        digit = iqpoints % 10;
        iqpoints /= 10;
        if ( !iqpoints )
        {
            iqpoints = -1;
        }

        width += jonesConfig_aNumberGlyphMetrics[digit].right - jonesConfig_aNumberGlyphMetrics[digit].left + 1;
        glyphHeight = jonesConfig_aNumberGlyphMetrics[digit].bottom - jonesConfig_aNumberGlyphMetrics[digit].top + 1;
        if ( glyphHeight > height )
        {
            height = glyphHeight;
        }
    }

    bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth         = width;
    bmi.bmiHeader.biHeight        = -height;
    bmi.bmiHeader.biPlanes        = 1;
    bmi.bmiHeader.biBitCount      = 24;
    bmi.bmiHeader.biCompression   = BI_RGB;
    bmi.bmiHeader.biSizeImage     = width * height * 3;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed       = 0;
    bmi.bmiHeader.biClrImportant  = 0;

    pImageInfo->hBmp = CreateDIBSection(pImageInfo->hdcBack, &bmi, DIB_RGB_COLORS, &ppvBits, NULL, 0);

    SelectObject(pImageInfo->hdcBack, pImageInfo->hBmp);
    SelectObject(pImageInfo->hdcFront, jonesConfig_apDlgIcons[3]); // 3 - numbers bmp

    rc.top    = rc.left = 0;
    rc.bottom = height;
    rc.right  = width;
    FillRect(pImageInfo->hdcBack, &rc, CreateSolidBrush(RGB(255, 255, 255)));

    iqpoints = points;
    if ( points < 0 )
    {
        iqpoints = 0;
    }

    x = width;
    while ( iqpoints >= 0 )
    {
        digit = iqpoints % 10;
        iqpoints /= 10;
        if ( !iqpoints )
        {
            iqpoints = -1;
        }

        dstX = x - (jonesConfig_aNumberGlyphMetrics[digit].right - jonesConfig_aNumberGlyphMetrics[digit].left + 1);
        BitBlt(
            pImageInfo->hdcBack,
            dstX,
            0,
            jonesConfig_aNumberGlyphMetrics[digit].right - jonesConfig_aNumberGlyphMetrics[digit].left + 1,
            jonesConfig_aNumberGlyphMetrics[digit].bottom - jonesConfig_aNumberGlyphMetrics[digit].top + 1,
            pImageInfo->hdcFront,
            jonesConfig_aNumberGlyphMetrics[digit].left,
            jonesConfig_aNumberGlyphMetrics[digit].top,
            SRCCOPY
        );
        x = dstX;
    }

    x = (127 - width) / 2 + 39;
    y = (46 - height) / 2 + 207;

    // Draw IQ bmp
    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDlgIcons[0]);
    BitBlt(pImageInfo->hdcFront, x, y, width, height, pImageInfo->hdcBack, 0, 0, SRCAND);

    // Draw IQ mask bmp
    SelectObject(pImageInfo->hdcFront, (HGDIOBJ)jonesConfig_apDlgIcons[1]);
    BitBlt(pImageInfo->hdcFront, x, y, width, height, pImageInfo->hdcBack, 0, 0, SRCCOPY);
}

int J3DAPI jonesConfig_InitStatisticDialog(HWND hDlg, int a2, LPARAM lParam)
{
    int* pData = (int*)lParam;
    HDC* v4;
    HWND hV10;
    SCROLLINFO scrlInfo;
    SithGameStatistics* pCurDataa;

    v4 = (HDC*)*pData;
    pCurDataa = (SithGameStatistics*)pData[1];
    *v4 = CreateCompatibleDC(GetDC(GetDlgItem(hDlg, 1135)));
    v4[1] = CreateCompatibleDC(GetDC(GetDlgItem(hDlg, 1135)));
    jonesConfig_DrawStatisticDialogIQPoints(hDlg, (JonesDialogImageInfo**)pData, 164, jonesInventory_GetTotalIQPoints());
    if ( !pCurDataa->curLevelNum )
    {
        hV10 = GetDlgItem(hDlg, 1133);
        EnableWindow(hV10, 0);
    }

    jonesConfig_SetStatisticsDialogForLevel(hDlg, JONESLEVEL_BONUSLEVELNUM, pData);

    scrlInfo.cbSize = sizeof(SCROLLINFO);
    scrlInfo.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    scrlInfo.nMin = 0;
    scrlInfo.nMax = pCurDataa->curLevelNum + 1;
    scrlInfo.nPage = 1;
    scrlInfo.nPos = scrlInfo.nMax;
    SendMessage(GetDlgItem(hDlg, 1162), SBM_SETSCROLLINFO, TRUE, (LPARAM)&scrlInfo);

    pData[2] = scrlInfo.nMax;
    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
    return 1;
}

void J3DAPI jonesConfig_StatisticProc_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode)
{
    JonesDialogImageInfo* pCurImageInfo;
    JonesDialogImageInfo** ppImageInfo;
    SithGameStatistics* pCurStatistics;

    // Retail pushes the full WM_COMMAND decode (add esp,0x10)

    ppImageInfo = (JonesDialogImageInfo**)GetWindowLong(hWnd, DWL_USER);
    pCurImageInfo = *ppImageInfo;
    pCurStatistics = (SithGameStatistics*)ppImageInfo[1]; // debug build reads the statistics pointer; unused
    switch ( nResult )
    {
        case 1:
        case 2:
            DeleteObject(pCurImageInfo->hBmp);
            pCurImageInfo->hBmp = NULL;
            DeleteDC(pCurImageInfo->hdcFront);
            DeleteDC(pCurImageInfo->hdcBack);
            EndDialog(hWnd, nResult);
            break;
    }
}

// Declared next to its first use
int CALLBACK jonesConfig_LevelCompletedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitLevelCompletedDialog(HWND hDlg, int wParam, LPARAM lParam);
void J3DAPI jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(HWND hWnd, int ctrlID, LPARAM lParam, int notifyCode);

// Declared next to its first use
int J3DAPI jonesConfig_ShowStoreDialog(HWND hWnd, int* pBalance, int* pItemsState, int a4);
int CALLBACK jonesConfig_StoreDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int J3DAPI jonesConfig_StoreHandleDragEvent(HWND hwnd, HWND hDlgCtrl);
void J3DAPI jonesConfig_StoreDialog_HandleWM_MBUTTONUP(HWND hWnd);
BOOL J3DAPI jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(HWND hWnd);
int J3DAPI jonesConfig_InitStoreDialog(HWND hDlg, int a2, LPARAM lParam);
int J3DAPI jonesConfig_StoreInitItemIcons(HWND hWnd, tStoreCartState* pCart);
void J3DAPI jonesConfig_StoreSetListColumns(HWND hList, const char* pColumnName);
void J3DAPI jonesConfig_StoreInitItemList(HWND hWnd, int* apItemsState, int listID, int a4);
void J3DAPI jonesConfig_UpdateBalances(HWND hDlg, tStoreCartState* pCart);
void J3DAPI jonesConfig_AddStoreCartItem(HWND hDlg, tStoreCartState* pCart);
void J3DAPI jonesConfig_RemoveStoreCartItem(HWND hDlg, tStoreCartState* pCart);
void J3DAPI jonesConfig_StoreDialog_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode);
void J3DAPI jonesConfig_ClearStoreCart(HWND hDlg, tStoreCartState* pCart);

int J3DAPI jonesConfig_ShowLevelCompletedDialog(HWND hWnd, int* pBalance, int* apItemsState, int a4, int elapsedTime, int qiPoints, int numFoundTrasures, int foundTrasureValue, int totalTreasureValue)
{
    int curresult;
    JonesDialogImageInfo imageInfo;

    tLevelCompletedDialogState state;
    memset(&state, 0, sizeof(state));

    state.balance            = pBalance;
    state.apItemsState       = apItemsState;
    state.unknown2           = a4;
    state.elapsedTime        = elapsedTime;
    state.iqPoints           = qiPoints;
    state.numFoundTrasures   = numFoundTrasures;
    state.foundTreasureValue = foundTrasureValue;
    state.totalTreasureValue = totalTreasureValue;
    state.pStatistics        = sithGamesave_GetGameStatistics();

    if ( state.pStatistics && sithGamesave_LockGameStatistics() )
    {
        memset(&imageInfo, 0, sizeof(imageInfo));

        state.pIQImageInfo = &imageInfo;

        curresult = JonesDialog_ShowDialog(MAKEINTRESOURCE(233), hWnd, jonesConfig_LevelCompletedDialogProc, (LPARAM)&state);
        *pBalance  = *state.balance;
        sithGamesave_UnlockGameStatistics();
    }
    else
    {
        curresult = jonesConfig_ShowStoreDialog(hWnd, pBalance, apItemsState, a4);
    }

    return curresult;
}

int CALLBACK jonesConfig_LevelCompletedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontLevelCopletedDialog = jonesConfig_InitDialog(hWnd, 0, 233);
            bResult = jonesConfig_InitLevelCompletedDialog(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            tLevelCompletedDialogState* pState = (tLevelCompletedDialogState*)GetWindowLong(hWnd, DWL_USER);
            JonesDialogImageInfo* pImageInfo = pState->pIQImageInfo;
            HWND hImgCtrl = GetDlgItem(hWnd, 1135);
            HDC hdcImg = BeginPaint(hImgCtrl, &paint);

            SetStretchBltMode(hdcImg, STRETCH_HALFTONE);
            jonesConfig_DrawImageOnDialogItem(hWnd, pImageInfo->hdcFront, hdcImg, 1135, jonesConfig_apDlgIcons[0], jonesConfig_apDlgIcons[1]);//   "iq.bmp" "iqMask.bmp"

            EndPaint(hImgCtrl, &paint);
            bResult = 0;
            break;
        }

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontLevelCopletedDialog);
            break;

        default:
            bResult = 0;
            break;
    };

    return bResult;
}

int J3DAPI jonesConfig_InitLevelCompletedDialog(HWND hDlg, int wParam, LPARAM lParam)
{
    tLevelCompletedDialogState* pState = (tLevelCompletedDialogState*)lParam;
    int iqPoints;
    int hours;
    int minutes;
    char szText[256];

    pState->pIQImageInfo->hdcFront = CreateCompatibleDC(GetDC(GetDlgItem(hDlg, 1135)));
    pState->pIQImageInfo->hdcBack  = CreateCompatibleDC(GetDC(GetDlgItem(hDlg, 1135)));

    jonesConfig_prevLevelNum = pState->pStatistics->curLevelNum - 1;
    iqPoints = pState->pStatistics->aLevelStatistic[jonesConfig_prevLevelNum].iqPoints;
    jonesConfig_DrawStatisticDialogIQPoints(hDlg, &pState->pIQImageInfo, 233, iqPoints);

    // Format game elapsed time
    hours = pState->elapsedTime >> 8;
    minutes = pState->elapsedTime ^ (hours << 8);

    memset(szText, 0, sizeof(szText));
    if ( minutes < 10 )
    {
        sprintf(szText, "%i : 0%i ", hours, minutes);
    }
    else
    {
        sprintf(szText, "%i : %i ", hours, minutes);
    }

    SetWindowText(GetDlgItem(hDlg, 1153), szText);

    memset(szText, 0, sizeof(szText));
    sprintf(szText, "%i ", pState->iqPoints);
    SetWindowText(GetDlgItem(hDlg, 1154), szText);

    memset(szText, 0, sizeof(szText));
    sprintf(szText, "%i ", pState->numFoundTrasures);
    SetWindowText(GetDlgItem(hDlg, 1155), szText);

    memset(szText, 0, sizeof(szText));
    sprintf(szText, "%i ", pState->foundTreasureValue);
    SetWindowText(GetDlgItem(hDlg, 1157), szText);

    memset(szText, 0, sizeof(szText));
    sprintf(szText, "%i ", pState->totalTreasureValue);
    SetWindowText(GetDlgItem(hDlg, 1156), szText);

    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
    return 1;
}

void J3DAPI jonesConfig_ChapterCompleteDialog_HandleWM_COMMAND(HWND hWnd, int wParam, LPARAM lParam, int notifyCode)
{
    tLevelCompletedDialogState* pState = (tLevelCompletedDialogState*)GetWindowLong(hWnd, DWL_USER);


    switch ( wParam )
    {
        case 1:
        case 2: // 1 or 2 - done
            DeleteObject((HGDIOBJ)pState->pIQImageInfo->hBmp);
            pState->pIQImageInfo->hBmp = NULL;

            DeleteDC(pState->pIQImageInfo->hdcFront);
            DeleteDC(pState->pIQImageInfo->hdcBack);

            // Show store dialog if not bonus level
            if ( pState->pStatistics->curLevelNum + 1 < JONESLEVEL_BONUSLEVELNUM )
            {
                jonesConfig_ShowStoreDialog(hWnd, pState->balance, pState->apItemsState, pState->unknown2);
            }

            InvalidateRect(hWnd, NULL, TRUE);
            EndDialog(hWnd, wParam);
            break;
    }
}

int J3DAPI jonesConfig_ShowStoreDialog(HWND hWnd, int* pBalance, int* pItemsState, int a4)
{
    int curresult;

    tStoreCartState cart;

    memset(&cart, 0, sizeof(cart));
    cart.balance         = *pBalance;
    cart.apItemsState    = pItemsState;
    cart.unknown20       = a4;

    curresult = JonesDialog_ShowDialog(MAKEINTRESOURCE(190), hWnd, jonesConfig_StoreDialogProc, (LPARAM)&cart);
    *pBalance  = cart.balance;
    return curresult;
}

int CALLBACK jonesConfig_StoreDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int result = 1;
    switch ( msg )
    {
        case WM_COMMAND:
            jonesConfig_StoreDialog_HandleWM_COMMAND(hwnd, LOWORD(wparam), lparam, HIWORD(wparam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontStoreDialog = jonesConfig_InitDialog(hwnd, 0, 190);
            result = jonesConfig_InitStoreDialog(hwnd, wparam, lparam);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pNMHdr = (LPNMHDR)lparam;
            tStoreCartState* pCart = (tStoreCartState*)GetWindowLong(hwnd, DWL_USER);
            switch ( pNMHdr->code )
            {
                case NM_DBLCLK:
                {
                    // If double mouse button click, add or remove item to cart based on which list item was clicked
                    if ( pNMHdr->idFrom == 1005 ) // available list editor
                    {
                        jonesConfig_AddStoreCartItem(hwnd, pCart);
                    }
                    else if ( pNMHdr->idFrom == 1006 ) // cart items list editor
                    {
                        jonesConfig_RemoveStoreCartItem(hwnd, pCart);
                    }
                } break;
                case LVN_BEGINDRAG:
                case LVN_BEGINRDRAG:
                {
                    // Note, lparam is NMLISTVIEW struct here
                    pCart->dragListId = pNMHdr->idFrom;
                    jonesConfig_StoreHandleDragEvent(hwnd, GetDlgItem(hwnd, pNMHdr->idFrom));
                } break;
            };
        } break;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            jonesConfig_StoreDialog_HandleWM_MBUTTONUP(hwnd);
            break;

        case WM_MOUSEFIRST:
            jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(hwnd);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hwnd, jonesConfig_hFontStoreDialog);
            break;

        default:
            result = 0;
            break;
    }

    return result;
}

int J3DAPI jonesConfig_StoreHandleDragEvent(HWND hwnd, HWND hDlgCtrl)
{
    POINT cursorPos;
    RECT dlgRect, wndRect;
    RECT itemRect;
    int itemIdx;
    int i;
    LONG dragX;
    LONG dragY;
    LVITEMA item;
    int imageIdx;
    HIMAGELIST hList;
    LONG dx;
    LONG dy;

    tStoreCartState* pCart = (tStoreCartState*)GetWindowLong(hwnd, DWL_USER);
    int numSelected = ListView_GetSelectedCount(hDlgCtrl);

    SetCapture(hwnd);

    GetCursorPos(&cursorPos);

    GetWindowRect(hDlgCtrl, &dlgRect);
    GetWindowRect(hwnd, &wndRect);

    itemIdx = -1;
    for ( i = 0; i < numSelected; ++i )
    {
        itemIdx = ListView_GetNextItem(hDlgCtrl, itemIdx, LVNI_SELECTED);
        ListView_GetItemRect(hDlgCtrl, itemIdx, &itemRect, LVIR_SELECTBOUNDS);

        dragX = cursorPos.x - (itemRect.left + dlgRect.left);
        dragY = cursorPos.y - (itemRect.top + dlgRect.top);

        item.mask     = LVIF_PARAM;
        item.iSubItem = 0;
        item.iItem    = itemIdx;
        ListView_GetItem(hDlgCtrl, &item);

        imageIdx = (item.lParam & 0xFFFF0000) >> 16;

        hList = ListView_GetImageList(hDlgCtrl, LVSIL_SMALL);
        if ( !ImageList_BeginDrag(hList, imageIdx, dragX, dragY) )
        {
            ImageList_EndDrag();
            ImageList_DragLeave(hwnd);
            ReleaseCapture();
            pCart->bDragging = 0;
            return; /* retail bug: no return value here or at the end; the debug build is the same */
        }
    }

    dx = cursorPos.x - wndRect.left;
    dy = cursorPos.y - wndRect.top;
    ImageList_DragEnter(hwnd, dx, dy);
    pCart->bDragging = 1;
}

void J3DAPI jonesConfig_StoreDialog_HandleWM_MBUTTONUP(HWND hWnd)
{
    tStoreCartState* pCart = (tStoreCartState*)GetWindowLong(hWnd, DWL_USER);
    RECT currect;
    POINT cursorPos;
    if ( pCart->bDragging )
    {
        ImageList_EndDrag();
        ImageList_DragLeave(hWnd);

        
        GetCursorPos(&cursorPos);

        
        GetWindowRect(GetDlgItem(hWnd, 1006), &currect);

        if ( PtInRect(&currect, cursorPos) && pCart->dragListId == 1005 )// available items list
        {
            jonesConfig_AddStoreCartItem(hWnd, pCart);
        }
        else
        {
            GetWindowRect(GetDlgItem(hWnd, 1005), &currect);
            if ( PtInRect(&currect, cursorPos) )
            {
                if ( pCart->dragListId == 1006 )// cart list
                {
                    jonesConfig_RemoveStoreCartItem(hWnd, pCart);
                }
            }
        }

        pCart->bDragging = 0;
        ReleaseCapture();
    }
}


BOOL J3DAPI jonesConfig_StoreDialog_HandleWM_MOUSEFIRST(HWND hWnd)
{
    RECT bounds;
    int dx, dy;

    POINT cursorPos;
    GetCursorPos(&cursorPos);

    
    GetWindowRect(hWnd, &bounds);
    dx = cursorPos.x - bounds.left;
    dy = cursorPos.y - bounds.top;
    return ImageList_DragMove(dx, dy);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(jonesConfig_countfit_20, 28) /* end count-fit */
int J3DAPI jonesConfig_InitStoreDialog(HWND hDlg, int a2, LPARAM lParam)
{
    HWND hwndAvailableList;
    tStoreCartState* pCurCart;
    HWND hCartList;
    HWND hCurList;

    pCurCart          = (tStoreCartState*)lParam;
    hwndAvailableList = GetDlgItem(hDlg, 1005);
    hCartList      = GetDlgItem(hDlg, 1006);
    hCurList   = GetDlgItem(hDlg, 1199);
    if ( !jonesConfig_StoreInitItemIcons(hDlg, pCurCart) )
    {
        return 0;
    }

    ListView_SetImageList(hwndAvailableList, pCurCart->hList, LVSIL_SMALL);
    ListView_SetImageList(hCartList, pCurCart->hList, LVSIL_SMALL);
    ListView_SetImageList(hCurList, pCurCart->hList, LVSIL_SMALL);

    jonesConfig_StoreSetListColumns(hwndAvailableList, "JONES_STR_COST");
    jonesConfig_StoreSetListColumns(hCartList, "JONES_STR_COST");
    jonesConfig_StoreSetListColumns(hCurList, "JONES_STR_COUNT");

    jonesConfig_StoreInitItemList(hCurList, pCurCart->apItemsState, 1199, pCurCart->unknown20);
    jonesConfig_StoreInitItemList(hwndAvailableList, pCurCart->apItemsState, 1005, pCurCart->unknown20);

    if ( ListView_GetItemCount(hCurList) > ListView_GetCountPerPage(hCurList) )
    {
        int colWidth = ListView_GetColumnWidth(hCurList, 0);
        ListView_SetColumnWidth(hCurList, 0, colWidth - GetSystemMetrics(SM_CXVSCROLL));
    }

    if ( ListView_GetItemCount(hwndAvailableList) > ListView_GetCountPerPage(hwndAvailableList) )
    {
        int colWidth = ListView_GetColumnWidth(hwndAvailableList, 0);
        ListView_SetColumnWidth(hwndAvailableList, 0, colWidth - GetSystemMetrics(SM_CXVSCROLL));
    }

    pCurCart->cartListWidth = ListView_GetColumnWidth(hCartList, 0);
    pCurCart->total         = 0;
    jonesConfig_UpdateBalances(hDlg, pCurCart);

    SetWindowLong(hDlg, DWL_USER, lParam);
    return 1;
}

int J3DAPI jonesConfig_StoreInitItemIcons(HWND hWnd, tStoreCartState* pCart)
{
    HINSTANCE hInstance;
    int i;

    pCart->hList = ImageList_Create(16, 16, ILC_MASK | ILC_COLOR8, STD_ARRAYLEN(JonesHud_aStoreItems), 0);
    if ( !pCart->hList )
    {
        return 0;
    }

    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    for ( i = 0; i < (int)STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        pCart->aItemIcons[i] = LoadIconA(hInstance, MAKEINTRESOURCE(JonesHud_aStoreItems[i].iconID));
        ImageList_ReplaceIcon(pCart->hList, -1, pCart->aItemIcons[i]);
    }
}

void J3DAPI jonesConfig_StoreSetListColumns(HWND hList, const char* pColumnName)
{
    int bufWidth;
    int bufHeight;
    double ratio;
    int curCol;
    int valueWidth;
    RECT rectList;
    LPSTR pString;
    LVCOLUMNA column;

    GetClientRect(hList, &rectList);

    column.mask       = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    column.cchTextMax = 256;

    stdDisplay_GetBackBufferSize((uint32_t*)&bufWidth, (uint32_t*)&bufHeight);
    if ( 1.0 < bufHeight / 480.0 )
    {
        ratio = 1.0;
    }
    else
    {
        ratio = bufHeight / 480.0;
    }

    valueWidth = (int)(40.0 * ratio);

    curCol       = 0;
    pString        = (LPSTR)jonesString_GetString("JONES_STR_ITEMNAME");
    column.fmt      = LVCFMT_LEFT;
    column.cx       = rectList.right - rectList.left - valueWidth;
    column.pszText  = pString;
    column.iSubItem = curCol;
    ListView_InsertColumn(hList, curCol, &column);
    curCol++;

    pString        = (LPSTR)jonesString_GetString(pColumnName);
    column.fmt      = LVCFMT_RIGHT;
    column.cx       = valueWidth;
    column.pszText  = pString;
    column.iSubItem = curCol;
    ListView_InsertColumn(hList, curCol, &column);
}

void J3DAPI jonesConfig_StoreInitItemList(HWND hWnd, int* apItemsState, int listID, int a4)
{
    int i;
    int imageIdx;

    LVITEM newItem;

    newItem.mask       = LVIF_STATE | LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
    newItem.cchTextMax = 256; // must match aCost
    newItem.state      = 0;
    newItem.stateMask  = 0;

    imageIdx = 0;
    for ( i = 0; i < (int)STD_ARRAYLEN(JonesHud_aStoreItems); ++i )
    {
        switch ( listID )
        {
            case 1005: //  available items list
                if ( (apItemsState[i] & 0xF) != 0 ) // if item is available
                {
                    char aCost[256];
                    LPSTR pName = (LPSTR)jonesString_GetString(JonesHud_aStoreItems[i].aClipName);

                    newItem.iItem    = imageIdx;
                    newItem.iSubItem = 0;
                    newItem.pszText  = pName;
                    newItem.iImage   = i;
                    newItem.lParam   = (i << 16) | JonesHud_aStoreItems[i].price;
                    ListView_InsertItem(hWnd, &newItem);

                    _itoa(JonesHud_aStoreItems[i].price, aCost, 10);
                    ListView_SetItemText(hWnd, imageIdx, 1, aCost);
                    ++imageIdx;
                }

                apItemsState[i] &= ~0xFFFFu;
                break;

            case 1199: // current inventory list
                {
                    char aAmount[256];
                    LPSTR pName;

                    if ( !((apItemsState[i] & 0xFFF0) >> 4) )
                    {
                        break;
                    }

                    pName = (LPSTR)jonesString_GetString(JonesHud_aStoreItems[i].aName);

                    newItem.iItem    = imageIdx;
                    newItem.iSubItem = 0;
                    newItem.pszText  = pName;
                    newItem.iImage   = i;
                    ListView_InsertItem(hWnd, &newItem);

                    _itoa((apItemsState[i] & 0xFFF0) >> 4, aAmount, 10);// extracts amount
                    ListView_SetItemText(hWnd, imageIdx, 1, aAmount);
                    ++imageIdx;
                }
                break;
        }
    }
}

void J3DAPI jonesConfig_UpdateBalances(HWND hDlg, tStoreCartState* pCart)
{
    CHAR aText[256];
    memset(aText, 0, sizeof(aText));

    sprintf(aText, "%i ", pCart->total);

    SetWindowText(GetDlgItem(hDlg, 1173), aText); // 1173 - total cost

    memset(aText, 0, sizeof(aText));
    sprintf(aText, "%i ", pCart->balance);

    SetWindowText(GetDlgItem(hDlg, 1174), aText); // 1174 - balance
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(jonesConfig_countfit_21, 21) /* end count-fit */
void J3DAPI jonesConfig_AddStoreCartItem(HWND hDlg, tStoreCartState* pCart)
{
    int cost = 0;
    HWND hItemList = GetDlgItem(hDlg, 1005);        // available items list
    int cntSelected = ListView_GetSelectedCount(hItemList);
    int curIdx;
    int bonusMapIndex;
    int cnt;
    LVITEM item;
    int storeIndex;

    if ( !cntSelected )
    {
        return;
    }

    curIdx = -1;
    bonusMapIndex = -1;
    for ( cnt = 0; cnt < cntSelected; ++cnt )
    {
        curIdx = ListView_GetNextItem(hItemList, curIdx, LVNI_SELECTED);

        item.mask     = LVIF_PARAM;
        item.iSubItem = 0;
        item.iItem    = curIdx;
        ListView_GetItem(hItemList, &item);

        storeIndex = (item.lParam & 0xFFFF0000) >> 16; //  HIWORD
        if ( JonesHud_aStoreItems[storeIndex].menuID == JONESHUD_MENU_INVITEM_BONUSMAP )
        {
            bonusMapIndex = storeIndex;
        }

        cost += JonesHud_aStoreItems[storeIndex].price;
    }

    // Retail writes this as a guard clause (insufficient-balance error,
    // then return) inline before the purchase logic, not an if/else -
    // its compiled error block ends in its own `ret` (0x0040__40)
    // rather than jumping back to rejoin a shared tail.
    if ( pCart->balance < cost )
    {
        const char* pDlgText;
        if ( bonusMapIndex > -1 && JonesHud_aStoreItems[bonusMapIndex].menuID == JONESHUD_MENU_INVITEM_BONUSMAP ) {
            pDlgText = jonesString_GetString("JONES_STR_NOPERU");
        }
        else if ( cntSelected == 1 ) {
            pDlgText = jonesString_GetString("JONES_STR_CANTBUY1");
        }
        else {
            pDlgText = jonesString_GetString("JONES_STR_CANTBUY");
        }

        if ( pDlgText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_STORE", pDlgText, 191);
            InvalidateRect(hDlg, 0, 1);
        }
        return;
    }

    if ( bonusMapIndex > -1 && JonesHud_aStoreItems[bonusMapIndex].menuID == JONESHUD_MENU_INVITEM_BONUSMAP )
    {
        const char* pDlgText = jonesString_GetString("JONES_STR_PERU");
        if ( pDlgText )
        {
            jonesConfig_ShowMessageDialog(hDlg, "JONES_STR_STORE", pDlgText, 191);
            InvalidateRect(hDlg, 0, 1);
        }
    }

    curIdx = -1;
    for ( cnt = 0; cnt < cntSelected; ++cnt )
    {
        int imgIdx;
        HWND hCartList;
        int numItems;
        char aBuf[256];

        hCartList = GetDlgItem(hDlg, 1006);// cart list

        // Get item from available item list
        curIdx = ListView_GetNextItem(hItemList, curIdx, LVNI_SELECTED);

        memset(aBuf, 0, sizeof(aBuf));

        item.mask       = LVIF_PARAM | LVIF_TEXT;
        item.iSubItem   = 0;
        item.iItem      = curIdx;
        item.pszText    = aBuf;
        item.cchTextMax = STD_ARRAYLEN(aBuf);
        ListView_GetItem(hItemList, &item);

        // Add item to purchase list
        imgIdx = (item.lParam & 0xFFFF0000) >> 16; // See jonesConfig_StoreInitItemList

        item.mask      = LVIF_STATE | LVIF_PARAM | LVIF_IMAGE | LVIF_TEXT;
        item.state     = 0;
        item.stateMask = 0;
        item.iImage    = imgIdx;
        item.iItem     = ListView_GetItemCount(hCartList);
        ListView_InsertItem(hCartList, &item);

        // Set item in cart
        memset(aBuf, 0, sizeof(aBuf));
        ListView_GetItemText(hItemList, curIdx, 1, aBuf, item.cchTextMax);
        ListView_SetItemText(hCartList, item.iItem, 1, aBuf);

        numItems = pCart->apItemsState[imgIdx];
        pCart->apItemsState[imgIdx] = (numItems & 0xFFFF0000) | ((numItems & 0xFFFF) + 1);

        if ( ListView_GetItemCount(hCartList) > ListView_GetCountPerPage(hCartList) )
        {
            ListView_SetColumnWidth(hCartList, 0, pCart->cartListWidth - GetSystemMetrics(SM_CXVSCROLL));
        }
    }

    pCart->total   += cost;
    pCart->balance -= cost;
    jonesConfig_UpdateBalances(hDlg, pCart);
}

void J3DAPI jonesConfig_RemoveStoreCartItem(HWND hDlg, tStoreCartState* pCart)
{
    int refundSum = 0;
    HWND hList = GetDlgItem(hDlg, 1006);
    int selected = ListView_GetSelectedCount(hList);
    int row;
    LVITEM lvi;
    int i;
    int storeIndex;
    int cartState;

    if ( !selected )
    {
        return;
    }

    row = -1;
    for ( i = 0; i < selected; ++i )
    {
        row = ListView_GetNextItem(hList, row, LVNI_SELECTED);

        lvi.mask     = LVIF_PARAM;
        lvi.iSubItem = 0;
        lvi.iItem    = row;
        ListView_GetItem(hList, &lvi);

        refundSum += lvi.lParam & 0xFFFF;

        ListView_DeleteItem(hList, row);

        storeIndex = (lvi.lParam & 0xFFFF0000) >> 16;
        cartState = pCart->apItemsState[storeIndex];
        pCart->apItemsState[storeIndex] = (cartState & 0xFFFF0000) | ((cartState & 0xFFFF) - 1);

        if ( ListView_GetItemCount(hList) <= ListView_GetCountPerPage(hList) )
        {
            ListView_SetColumnWidth(hList, 0, LOWORD(pCart->cartListWidth));
        }
    }

    pCart->total   -= refundSum;
    pCart->balance += refundSum;
    jonesConfig_UpdateBalances(hDlg, pCart);
}

// Declared next to its first use
int J3DAPI jonesConfig_ShowPurchaseMessageBox(HWND hWnd, tStoreCartState* dwInitParam);
BOOL CALLBACK jonesConfig_PurchaseMessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
int J3DAPI jonesConfig_InitPurchaseMessageBox(HWND hDlg, int a2, LPARAM lParam);

void J3DAPI jonesConfig_StoreDialog_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode)
{
    tStoreCartState* pCart = (tStoreCartState*)GetWindowLong(hWnd, DWL_USER);

    switch ( nResult )
    {
        case 1168: // Add item
            jonesConfig_AddStoreCartItem(hWnd, pCart);
            break;

        case 1169: // Return item
            jonesConfig_RemoveStoreCartItem(hWnd, pCart);
            break;

        case 1175: // Clear items
            jonesConfig_ClearStoreCart(hWnd, pCart);
            break;

        case 1:
        case 2:
            if ( pCart->balance >= 0 )
            {
                if ( jonesConfig_ShowPurchaseMessageBox(hWnd, pCart) == 1 )
                {
                    InvalidateRect(hWnd, 0, 1);
                    EndDialog(hWnd, nResult);
                }
            }
            else
            {
                EndDialog(hWnd, nResult);
            }

            break;
    }
}

void J3DAPI jonesConfig_ClearStoreCart(HWND hDlg, tStoreCartState* pCart)
{
    HWND hWnd = GetDlgItem(hDlg, 1006);
    int numRows = ListView_GetItemCount(hWnd);
    int i;
    unsigned int storeIndex;
    LVITEM itemInfo;
    int stateVal;

    for ( i = 0; i < numRows; ++i )
    {
        itemInfo.mask = LVIF_PARAM;
        itemInfo.iSubItem = 0;
        itemInfo.iItem = i;
        ListView_GetItem(hWnd, &itemInfo);

        storeIndex = (itemInfo.lParam & 0xFFFF0000) >> 16;
        stateVal = pCart->apItemsState[storeIndex];
        pCart->apItemsState[storeIndex] = (stateVal & 0xFFFF0000) | ((stateVal & 0xFFFF) - 1);
    }

    pCart->balance += pCart->total;
    pCart->total = 0;
    ListView_DeleteAllItems(hWnd);
    jonesConfig_UpdateBalances(hDlg, pCart);
}

int J3DAPI jonesConfig_ShowPurchaseMessageBox(HWND hWnd, tStoreCartState* dwInitParam)
{
    int result;

    result = JonesDialog_ShowDialog(MAKEINTRESOURCE(212), hWnd, jonesConfig_PurchaseMessageBoxProc, (LPARAM)dwInitParam);
    return result;
}

BOOL CALLBACK jonesConfig_PurchaseMessageBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult = 1;

    switch ( uMsg )
    {
        case WM_COMMAND:
            jonesConfig_MsgBoxDlg_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;

        case WM_INITDIALOG:
            jonesConfig_hFontPurchaseMessageBox = jonesConfig_InitDialog(hWnd, 0, 212);
            bResult = jonesConfig_InitPurchaseMessageBox(hWnd, wParam, lParam);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontPurchaseMessageBox);
            break;

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitPurchaseMessageBox(HWND hDlg, int a2, LPARAM lParam)
{
    const char* pCurrentFormat;
    char aText[512];
    tStoreCartState* pCart = (tStoreCartState*)lParam;
    char szCurTotalText[256];

    memset(aText, 0, sizeof(aText));

    if ( !pCart->total )
    {
        pCurrentFormat = jonesString_GetString("JONES_STR_NOCHANGE");
        if ( pCurrentFormat )
        {
            strcat(aText, pCurrentFormat);
            strcat(aText, "  ");
        }

        pCurrentFormat = jonesString_GetString("JONES_STR_ABANDON");
        if ( pCurrentFormat )
        {
            strcat(aText, pCurrentFormat);
        }
    }
    else
    {
        strcat(aText, "\n");
        pCurrentFormat = jonesString_GetString("JONES_STR_ADDINV");
        if ( pCurrentFormat )
        {
            strcat(aText, pCurrentFormat);
            strcat(aText, " ");
        }

        pCurrentFormat = jonesString_GetString("JONES_STR_SUBCHEST");
        if ( pCurrentFormat )
        {
            memset(szCurTotalText, 0, sizeof(szCurTotalText));

            sprintf(szCurTotalText, pCurrentFormat, pCart->total);
            strcat(aText, szCurTotalText);
            strcat(aText, "  ");
        }

        pCurrentFormat = jonesString_GetString("JONES_STR_YESNOBUY");
        if ( pCurrentFormat )
        {
            strcat(aText, pCurrentFormat);
        }
    }

    jonesConfig_SetTextControl(hDlg, GetDlgItem(hDlg, 1191), aText);
    return 1;
}

int J3DAPI jonesConfig_ShowDialogInsertCD(HWND hWnd, LPARAM dwInitParam)
{
    int btnNum;
    HINSTANCE hInstance;

    ShowCursor(1);
    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    btnNum = JonesDialog_ShowDialog(MAKEINTRESOURCE(167), hWnd, jonesConfig_DialogInsertCDProc, dwInitParam);
    return btnNum; // 1 - ok or 2 - quit 
}

BOOL CALLBACK jonesConfig_DialogInsertCDProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bResult;

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            jonesConfig_hFontDialogInsertCD = jonesConfig_InitDialog(hWnd, 0, 167);
            bResult = jonesConfig_InitDialogInsertCD(hWnd, wParam, lParam);
            break;

        case WM_COMMAND:
            jonesConfig_InsertCD_HandleWM_COMMAND(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            bResult = 1;
            break;

        case WM_DESTROY:
            jonesConfig_ResetDialogFont(hWnd, jonesConfig_hFontDialogInsertCD);
            break; // Retail bug: returns an uninitialised result

        default:
            bResult = 0;
            break;
    }

    return bResult;
}

int J3DAPI jonesConfig_InitDialogInsertCD(HWND hDlg, int a2, LPARAM lParam)
{
    const char* pText = NULL;
    int cdNum = (int)lParam;
    HWND hWnd;

    hWnd = GetDlgItem(hDlg, 1134);
    switch ( cdNum )
    {
        default:
            pText = jonesString_GetString("JONES_STR_INSERT1");
            if ( pText )
            {
                SetWindowText(hWnd, pText);
            }

            break;

        case 2:
            pText = jonesString_GetString("JONES_STR_INSERT2");
            if ( pText )
            {
                SetWindowText(hWnd, pText);
            }

            break;
    }

    SetWindowLong(hDlg, DWL_USER, (LONG)lParam);
    return 1;
}

void J3DAPI jonesConfig_InsertCD_HandleWM_COMMAND(HWND hWnd, int nResult, LPARAM lParam, int notifyCode)
{
    int cdNum = (int)GetWindowLong(hWnd, DWL_USER);


    switch ( nResult )
    {
        case 1: // ok
            if ( cdNum != JonesFile_GetCurrentCDNum() )
            {
                return;
            }
            // Fall through: the requested CD is in, close like quit

        case 2: // quit
            EndDialog(hWnd, nResult);
            ShowCursor(0);
            break;
    }
}
