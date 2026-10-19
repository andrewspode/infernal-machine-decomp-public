#include "sithControl.h"

#include <sith/Engine/sithCamera.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdControl.h>

#define STDCONTROL_MAXBINDINGS 8

typedef struct sSithControlFunctionBinding
{
    size_t numBindings;
    SithControlBinding aBindings[STDCONTROL_MAXBINDINGS];
} SithControlFunctionBinding;

static uint32_t msecPlayerIdleTime = 0;
static int bControlStartup = 0;
static int bControlOpen = 0;

static int numControlCallbacks;
static SithControlCallback apControlCallbacks[32];

static SithControlFunctionFlag aControlFlags[SITHCONTROL_MAXFUNCTIONS];
static SithControlFunctionBinding aControlBindings[SITHCONTROL_MAXFUNCTIONS];

const char* aFunctionNames[SITHCONTROL_MAXFUNCTIONS + 1] =
{
    "FORWARD",
    "BACK",
    "LEFT",
    "RIGHT",
    "TURNLEFT",
    "TURNRIGHT",
    "JUMP",
    "CRAWLTOGGLE",
    "ACT1",
    "FIRE1",
    "ACTIVATE",
    "ACT2",
    "ACT3",
    "NEXTWEAP",
    "PREVWEAP",
    "LIGHTERTOG",
    "LOOK",
    "STPLEFT",
    "STPRIGHT",
    "WEAPONTOG",
    "WSELECT0",
    "WSELECT1",
    "WSELECT2",
    "WSELECT3",
    "WSELECT4",
    "WSELECT5",
    "WSELECT6",
    "WSELECT7",
    "WSELECT8",
    "WSELECT9",
    "WSELECT10",
    "WSELECT11",
    "WSELECT12",
    "WSELECT13",
    "WSELECT14",
    "WSELECT15",
    "WSELECT16",
    "USEINV",
    "GAMESAVE",
    "DEBUG",
    "NEXTWEAPON",
    "PREVWEAPON",
    "MAP",
    "INCREASE",
    "DECREASE",
    "CAMERAMODE",
    "TALK",
    "SCREENSHOT",
    "CAMERAZOOMIN",
    "CAMERAZOOMOUT",
    "PITCH",
    "CENTER",
    "MLOOK",
    "MOUSETURN",
    "GAMMA",
    "RUNFWD",
    "HEALTH",
    "CHALK",
    "MAXFUNCTIONS"
};

void J3DAPI sithControl_FinishRead(void);

int J3DAPI sithControl_Startup(int bKeyboardForeground)
{
#line 103
    SITH_ASSERTREL(strcmp( aFunctionNames[SITHCONTROL_MAXFUNCTIONS], "MAXFUNCTIONS" ) == 0);
    if ( bControlStartup )
    {
        return 1;
    }

    if ( stdControl_Startup(bKeyboardForeground) )
    {
#line 113
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Control system platform startuped failed.\n", 0, 0, 0, 0);
        return 1;
    }

    sithControl_RegisterControlFunctions();
    sithControl_Reset();
    bControlStartup = 1;
    return 0;
}

void J3DAPI sithControl_Shutdown(void)
{
    stdControl_Shutdown();
    bControlStartup = 0;
}

int J3DAPI sithControl_Open(void)
{
#line 176
    SITH_ASSERTREL(bControlStartup);
    if ( stdControl_Open() ) {
        return 0;
    }

    msecPlayerIdleTime = 0;
    bControlOpen = 1;
    return 1;
}

int J3DAPI sithControl_Close(void)
{
    if ( !bControlOpen )
    {
#line 211
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Control system illegal close.\n", 0, 0, 0, 0);
        return 0;
    }

    stdControl_Close();
    bControlOpen = 0;
    return 1;
}

// Discarded by the retail linker (no caller in either build): debug 0x44d837. Name inferred.
int J3DAPI sithControl_IsOpen(void)
{
    return bControlOpen;
}

void J3DAPI sithControl_RegisterAxisFunction(size_t functionId, SithControlFunctionFlag flag)
{
#line 259
    SITH_ASSERTREL((functionId >= 0) && (functionId < SITHCONTROL_MAXFUNCTIONS));
    SITH_ASSERTREL(!bControlOpen);
    aControlFlags[functionId] = flag | (SITHCONTROLFUNCTION_AXIS | SITHCONTROLFUNCTION_REGISTERED);
}

void J3DAPI sithControl_RegisterKeyFunction(size_t functionId, SithControlFunctionFlag flags)
{
#line 289
    SITH_ASSERTREL((functionId >= 0) && (functionId < SITHCONTROL_MAXFUNCTIONS));
    SITH_ASSERTREL(!bControlOpen);

    // Note, unlike sithControl_RegisterAxisFunction the flags argument is not used here.
    //       Retail stores the constant KEY | REGISTERED, so the caller's flags are dropped.
    aControlFlags[functionId] = SITHCONTROLFUNCTION_KEY | SITHCONTROLFUNCTION_REGISTERED;
}

SithControlBinding* J3DAPI sithControl_BindControl(size_t functionId, size_t controlId, SithControlBindFlag flags)
{
    SithControlBinding* pBinding;
    size_t numBindings;

#line 325
    SITH_ASSERTREL(functionId < SITHCONTROL_MAXFUNCTIONS);
    SITH_ASSERTREL(controlId < STDCONTROL_MAX_KEYID);

    flags = flags & ~(SITHCONTROLBIND_AXIS_ENABLED | SITHCONTROLBIND_AXISCONTROL);
    flags = flags | SITHCONTROLBIND_KEYCONTROL;
    if ( (aControlFlags[functionId] & SITHCONTROLFUNCTION_REGISTERED) == 0 )
    {
        return NULL;
    }

    if ( aControlBindings[functionId].numBindings == STDCONTROL_MAXBINDINGS )
    {
#line 339
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to bind more than %d controls to function %d.\n", STDCONTROL_MAXBINDINGS, functionId, 0, 0);
        return NULL;
    }

    numBindings = aControlBindings[functionId].numBindings;
    pBinding = &aControlBindings[functionId].aBindings[numBindings];
    pBinding->flags     = flags;
    pBinding->controlId = controlId;
    aControlBindings[functionId].numBindings = numBindings + 1;

    return pBinding;
}

SithControlBinding* J3DAPI sithControl_BindAxis(size_t functionId, size_t axis, SithControlBindFlag flags)
{
    SithControlBinding* pBinding;
    size_t numBindings;

#line 388
    SITH_ASSERTREL(functionId < SITHCONTROL_MAXFUNCTIONS);

    flags = flags & ~SITHCONTROLBIND_KEYCONTROL;
    flags = flags | SITHCONTROLBIND_AXISCONTROL;
    if ( !stdControl_TestAxisFlag(axis, STDCONTROL_AXIS_REGISTERED) )
    {
#line 396
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Tried to bind unavailable axis %d to function %d.\n", axis, functionId, 0, 0);
        return NULL;
    }

    if ( (aControlFlags[functionId] & SITHCONTROLFUNCTION_REGISTERED) == 0 )
    {
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Tried to bind axis %d to unavailable function %d.\n", axis, functionId, 0, 0);
        return NULL;
    }

    if ( aControlBindings[functionId].numBindings == STDCONTROL_MAXBINDINGS )
    {
#line 409
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to bind more than %d controls to function %d.\n", STDCONTROL_MAXBINDINGS, functionId, 0, 0);
        return NULL;
    }

    if ( stdControl_TestAxisFlag(axis, STDCONTROL_AXIS_ENABLED) )
    {
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Control %x rebound to function %d.\n", axis, functionId, 0, 0);
        sithControl_UnbindControl(SITHCONTROLBIND_AXISCONTROL, axis);
    }

    if ( !stdControl_EnableAxis(axis) )
    {
#line 422
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Platform could not enable axis %d.\n", axis, 0, 0, 0);
        return NULL;
    }

    if ( (flags & SITHCONTROLBIND_AXIS_HASDEADZONE) != 0 )
    {
        stdControl_SetAxisFlags(axis, STDCONTROL_AXIS_HASDEADZONE);
    }

    numBindings = aControlBindings[functionId].numBindings;
    pBinding = &aControlBindings[functionId].aBindings[numBindings];
    pBinding->flags     = flags;
    pBinding->controlId = axis;
    aControlBindings[functionId].numBindings = numBindings + 1;

    return pBinding;
}

void J3DAPI sithControl_UnbindFunction(SithControlFunction fctnID)
{
#line 458
    SITH_ASSERTREL(STD_INRANGE(fctnID, 0, SITHCONTROL_MAXFUNCTIONS-1));
    memset(aControlBindings[fctnID].aBindings, 0, STDCONTROL_MAXBINDINGS); // Retail bug: clears only 8 bytes, not the whole array
    aControlBindings[fctnID].numBindings = 0;
}

void J3DAPI sithControl_UnbindFunctionIndex(int funcId, size_t bindIndex)
{
    size_t i;

#line 480
    SITH_ASSERTREL(STD_INRANGE(funcId, 0, SITHCONTROL_MAXFUNCTIONS-1));
    SITH_ASSERTREL(STD_INRANGE(bindIndex, 0, aControlBindings[funcId].numBindings-1));

    --aControlBindings[funcId].numBindings;
    for ( i = bindIndex; i < aControlBindings[funcId].numBindings; ++i )
    {
        aControlBindings[funcId].aBindings[i] = aControlBindings[funcId].aBindings[i + 1];
    }
}

void J3DAPI sithControl_UnbindControl(SithControlBindFlag flags, size_t controlId)
{
    size_t funcId;
    size_t bindIndex;

    for ( funcId = 0; funcId < SITHCONTROL_MAXFUNCTIONS; funcId++ )
    {
        for ( bindIndex = 0; bindIndex < aControlBindings[funcId].numBindings; ++bindIndex )
        {
            if ( (aControlBindings[funcId].aBindings[bindIndex].flags & flags) != 0
                && aControlBindings[funcId].aBindings[bindIndex].controlId == controlId )
            {
                goto found;
            }
        }
    }

    return;

found:
    sithControl_UnbindFunctionIndex(funcId, bindIndex);
}

void J3DAPI sithControl_RegisterControlCallback(SithControlCallback pfCallback)
{
    if ( numControlCallbacks > (int)STD_ARRAYLEN(apControlCallbacks) ) // Retail bug: off-by-one bound check
    {
#line 558
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to register too many control callbacks.\n", 0, 0, 0, 0);
        return;
    }
    apControlCallbacks[numControlCallbacks++] = pfCallback;
}

void J3DAPI sithControl_Update(float secDeltaTime, uint32_t msecDeltaTime)
{
    int bNoIdleCam = 0;
    int i;

    if ( !bControlOpen )
    {
        return;
    }

    if ( sithPlayer_pLocalPlayerThing
        && (sithPlayer_pLocalPlayerThing->thingInfo.actorInfo.flags & SITH_AF_NOIDLECAMERA) == 0
        && (sithPlayer_pLocalPlayerThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) == 0
        && (sithCamera_camInfo.stateFlags & SITHCAMERA_STATE_CUTSCENE) == 0
        && sithPlayer_pLocalPlayerThing->moveInfo.physics.velocity.x == 0.0f
        && sithPlayer_pLocalPlayerThing->moveInfo.physics.velocity.y == 0.0f
        && sithPlayer_pLocalPlayerThing->moveInfo.physics.velocity.z == 0.0f )
    {
        switch ( sithPlayer_pLocalPlayerThing->moveStatus )
        {
            case SITHPLAYERMOVE_HANGING:
            case SITHPLAYERMOVE_CLIMBIDLE:
                bNoIdleCam = 1;
                break;

            default:
                break;
        }

        if ( (sithPlayer_pLocalPlayerThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
        {
            bNoIdleCam = 1;
        }

        if ( sithTime_IsPaused() )
        {
            bNoIdleCam = 1;
        }

        if ( bNoIdleCam == 1 )
        {
            msecPlayerIdleTime = 0;
        }

        if ( stdControl_ControlsIdle() && !bNoIdleCam )
        {
            msecPlayerIdleTime += msecDeltaTime;
            if ( msecPlayerIdleTime > STDCONTROL_IDLECAM_TIMEOUT && sithCamera_pCurCamera != &sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM] )
            {
#line 627
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Switch to idle camera.\n", 0, 0, 0, 0);
                sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM], sithPlayer_pLocalPlayerThing, 0);
                sithCamera_SetCurrentCamera(&sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM]);
            }
        }
        else
        {
            msecPlayerIdleTime = 0;
            if ( sithCamera_pCurCamera == &sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM] )
            {
#line 638
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Switch out of idle camera.\n", 0, 0, 0, 0);
                sithCamera_SetCurrentCamera(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM]);
            }
        }
    }
    else if ( sithCamera_pCurCamera == &sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM] )
    {
        msecPlayerIdleTime = 0;
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Switch out of idle camera.\n", 0, 0, 0, 0);
        sithCamera_SetCurrentCamera(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM]);
    }

    // Process callbacks
    if ( sithWorld_pCurrentWorld->pLocalPlayer && numControlCallbacks > 0 )
    {
        sithControl_ReadControls();
        for ( i = 0; i < numControlCallbacks; ++i )
        {
            if ( apControlCallbacks[i] && apControlCallbacks[i](sithWorld_pCurrentWorld->pLocalPlayer, secDeltaTime) ) {
                break;
            }
        }

        sithControl_FinishRead();
    }


}

/* PLACEHOLDER for a function the retail linker discarded (/OPT:REF). Its only
   surviving trace is this TU's two orphaned read-only pool constants -1.0f
   (0x00507290) and 1.0f (0x00507294): a whole-image dword scan finds no
   reference to either, they sit inside the TU's reviewed .rdata block directly
   after the 0.0f (0x0050728C) that sithControl_Update/GetKeyAsAxis/GetAxis use,
   and no surviving function in the TU emits any read-only constant but that
   0.0f (the compiled pool is 4 bytes against retail's 12). In VC5's first-use
   pool order that position is the first function after sithControl_Update, so
   the original defined one there that clamped to [-1, 1]. The name is
   inferred. Body from the Indy debug build
   (0x0044E0F5, asserts at 697..698): every enabled binding read as
   an axis or a key, scaled by sithTime_fps when AXIS_ENABLED, inverted, scaled by
   its sensitivity, clamped, summed and clamped again. */
float J3DAPI sithControl_GetKeyAsAxisNormalized(size_t axisId)
{
    float val;
    float result = 0.0f;
    SithControlBindFlag bindflags;
    size_t i;
    SithControlBinding* pBinding;

#line 697
    SITH_ASSERTREL((axisId >= 0) && (axisId < SITHCONTROL_MAXFUNCTIONS));
    SITH_ASSERTREL(((aControlFlags[axisId]) & (SITHCONTROLFUNCTION_AXIS)));

    for ( i = 0; i < aControlBindings[axisId].numBindings; ++i )
    {
        pBinding  = &aControlBindings[axisId].aBindings[i];
        bindflags = pBinding->flags;
        if ( (bindflags & SITHCONTROLBIND_AXISCONTROL) != 0 )
        {
            if ( (sithControl_controlOptions & 0x20) != 0
                && pBinding->controlId >= 0
                && pBinding->controlId < STDCONTROL_MAX_JOYSTICK_AXES )
            {
                continue;
            }

            val = stdControl_ReadAxis(pBinding->controlId);
        }
        else
        {
            if ( (sithControl_controlOptions & 0x20) != 0
                && pBinding->controlId >= STDCONTROL_JOYSTICK_FIRSTCID
                && pBinding->controlId <= STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS - 1 )
            {
                continue;
            }

            val = stdControl_ReadKeyAsAxis(pBinding->controlId);
        }

        if ( (aControlBindings[axisId].aBindings[i].flags & SITHCONTROLBIND_AXIS_ENABLED) != 0 )
        {
            val = val * sithTime_fps;
        }

        if ( val == 0.0f )
        {
            continue;
        }

        if ( (bindflags & SITHCONTROLBIND_INVERT_AXIS) != 0 )
        {
            val = -val;
        }

        if ( pBinding->sensitivity != 0.0f )
        {
            val = pBinding->sensitivity * val;
        }

        val = STDMATH_CLAMP(val, -1.0f, 1.0f);
        result += val;
    }

    result = STDMATH_CLAMP(result, -1.0f, 1.0f);
    return result;
}

float J3DAPI sithControl_GetKeyAsAxis(size_t axisId)
{
    float axisValue;
    float value = 0.0f;

#line 760
    SITH_ASSERTREL((axisId >= 0) && (axisId < SITHCONTROL_MAXFUNCTIONS));
    SITH_ASSERTREL(((aControlFlags[axisId]) & (SITHCONTROLFUNCTION_AXIS)));

    {
        size_t i = 0;  // block scope: the debug build's frame slot order follows it
        for ( ; i < aControlBindings[axisId].numBindings; ++i )
    {
        SithControlBinding* pBinding  = &aControlBindings[axisId].aBindings[i];
        SithControlBindFlag bindflags = pBinding->flags;

        if ( (bindflags & SITHCONTROLBIND_AXIS_ENABLED) != 0 )
        {
            continue;
        }

        if ( (bindflags & SITHCONTROLBIND_AXISCONTROL) != 0 )
        {
            // Note, joystick axis bindings are read by sithControl_GetAxis
            if ( (sithControl_controlOptions & 0x20) != 0
                && pBinding->controlId >= 0
                && pBinding->controlId < STDCONTROL_MAX_JOYSTICK_AXES )
            {
                continue;
            }

            continue;
        }

        if ( (sithControl_controlOptions & 0x20) != 0
            && pBinding->controlId >= STDCONTROL_JOYSTICK_FIRSTCID
            && pBinding->controlId <= STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS - 1 )
        {
            continue;
        }

        axisValue = stdControl_ReadKeyAsAxis(pBinding->controlId);
        if ( axisValue == 0.0f )
        {
            continue;
        }

        if ( (bindflags & SITHCONTROLBIND_INVERT_AXIS) != 0 )
        {
            axisValue = -axisValue;
        }

        if ( pBinding->sensitivity != 0.0f )
        {
            axisValue = D3DMultiply(pBinding->sensitivity, axisValue);
        }

        value += axisValue;
    }
    }

    return value;
}

float J3DAPI sithControl_GetAxis(size_t axisId)
{
    float value = 0.0f;
    float axisVal;

#line 834
    SITH_ASSERTREL((axisId >= 0) && (axisId < SITHCONTROL_MAXFUNCTIONS));
    SITH_ASSERTREL(((aControlFlags[axisId]) & (SITHCONTROLFUNCTION_AXIS)));

    {
        size_t numBindings = aControlBindings[axisId].numBindings;
        size_t i = 0;  // block scope: the debug build's frame slot order follows it
        SithControlBinding* pBinding = aControlBindings[axisId].aBindings;
        for ( ; i < numBindings; i++, pBinding++ )
    {
        SithControlBindFlag bindflags = aControlBindings[axisId].aBindings[i].flags;

        if ( (bindflags & SITHCONTROLBIND_AXIS_ENABLED) == 0 )
        {
            continue;
        }

        if ( (sithControl_controlOptions & 0x20) != 0
            && pBinding->controlId >= 0
            && pBinding->controlId < STDCONTROL_AID_MOUSE_X )
        {
            continue;
        }

        axisVal = D3DVAL(stdControl_ReadAxisRaw(aControlBindings[axisId].aBindings[i].controlId));

        if ( (bindflags & SITHCONTROLBIND_INVERT_AXIS) != 0 )
        {
            axisVal = -axisVal;
        }

        if ( aControlBindings[axisId].aBindings[i].sensitivity != 0.0f )
        {
            axisVal = D3DMultiply(aControlBindings[axisId].aBindings[i].sensitivity, axisVal);
        }

        value += axisVal;
    }
    }

    return value;
}

int J3DAPI sithControl_GetKey(size_t keyId, int* pState)
{
    int value = 0;
    size_t i;

#line 893
    SITH_ASSERTREL((keyId >= 0) && (keyId < SITHCONTROL_MAXFUNCTIONS));
    SITH_ASSERTREL(((aControlFlags[keyId]) & (SITHCONTROLFUNCTION_KEY)));

    if ( pState )
    {
        *pState = 0;
    }

    for ( i = 0; i < aControlBindings[keyId].numBindings; ++i )
    {
        size_t keyNum = aControlBindings[keyId].aBindings[i].controlId;
        if ( (sithControl_controlOptions & 0x20) != 0
            && keyNum >= STDCONTROL_JOYSTICK_FIRSTCID
            && keyNum <= STDCONTROL_JOYSTICK_FIRSTCID + STDCONTROL_JOYSTICK_TOTALCIDS - 1 )
        {
            continue;
        }

        if ( (aControlBindings[keyId].aBindings[i].flags & SITHCONTROLBIND_AXISCONTROL) != 0 )
        {
            value |= stdControl_ReadAxisAsKey(keyNum, pState);
        }

        if ( (aControlBindings[keyId].aBindings[i].flags & SITHCONTROLBIND_KEYCONTROL) != 0 )
        {
            value |= stdControl_ReadKey(keyNum, pState);
        }
    }

    return value;
}

void J3DAPI sithControl_ReadControls(void)
{
#line 948
    SITH_ASSERTREL(bControlStartup && bControlOpen);
    stdControl_ReadControls();
}

void J3DAPI sithControl_FinishRead(void)
{
    stdControl_FinishRead();
}

// Discarded by the retail linker (no caller in either build): debug 0x44e76c, the default-bindings reset.
// Name inferred; assert line 994.
void J3DAPI sithControl_DefaultInit(void)
{
#line 994
    SITH_ASSERTREL(bControlStartup && !bControlOpen);
    sithControl_Reset();
    sithControl_controlOptions = 4;
    sithControl_RegisterKeyboardBindings();
    sithControl_RegisterJoystickBindings();
    sithControl_RegisterMouseBindings();
}

// Discarded by the retail linker (no caller in either build): debug 0x44e7bb. Unbinds every keyboard key
// (the control id unmasked here) and registers the default keyboard bindings again. Name inferred.
void J3DAPI sithControl_RebindKeyboard(void)
{
    size_t funcId;
    size_t bindIndex;
    int bFound;
    SithControlBinding* pBinding;
    int bAxis;
    int controlId;

    for ( funcId = 0; funcId < SITHCONTROL_MAXFUNCTIONS; ++funcId )
    {
        do
        {
            for ( bindIndex = 0, bFound = 0; bindIndex < aControlBindings[funcId].numBindings && !bFound; ++bindIndex )
            {
                pBinding = &aControlBindings[funcId].aBindings[bindIndex];
                bAxis = pBinding->flags & SITHCONTROLBIND_AXISCONTROL;
                controlId = pBinding->controlId;
                if ( !bAxis && STDCONTROL_ISKEYBOARDBUTTON(controlId) )
                {
                    sithControl_UnbindFunctionIndex(funcId, bindIndex);
                    bFound = 1;
                }
            }
        } while ( bFound );
    }

    sithControl_RegisterKeyboardBindings();
}

void J3DAPI sithControl_UnbindJoystickAxes(void)
{
    size_t funcId;
    SithControlBinding* pBinding;
    int bCurFound;
    size_t bindIndex;
    int bAxis;
    int currentcontrolId;

    for ( funcId = 0; funcId < SITHCONTROL_MAXFUNCTIONS; ++funcId )
    {
        do
        {
            for ( bindIndex = 0, bCurFound = 0; bindIndex < aControlBindings[funcId].numBindings && !bCurFound; ++bindIndex )
            {
                pBinding = &aControlBindings[funcId].aBindings[bindIndex];
                bAxis = pBinding->flags & SITHCONTROLBIND_AXISCONTROL;
                currentcontrolId = pBinding->controlId & 0x0FFFFFFF;
                if ( !bAxis && STDCONTROL_ISJOYSTICKBUTTON(currentcontrolId) || bAxis && STDCONTROL_ISJOYSTICKAXIS(currentcontrolId) )
                {
                    sithControl_UnbindFunctionIndex(funcId, bindIndex);
                    bCurFound = 1;
                }
            }
        } while ( bCurFound );
    }

    stdControl_DisableReadJoysticks();
}

// Discarded by the retail linker (no caller in either build): debug 0x44e957. Name inferred.
void J3DAPI sithControl_RebindJoystick(void)
{
    sithControl_UnbindJoystickAxes();
    sithControl_RegisterJoystickBindings();
}

void J3DAPI sithControl_UnbindMouseAxes(void)
{
    size_t funcId;
    SithControlBinding* pBinding;
    int bCurFound;
    size_t bindIndex;
    int bAxis;
    int currentcontrolId;

    for ( funcId = 0; funcId < SITHCONTROL_MAXFUNCTIONS; ++funcId )
    {
        do
        {
            for ( bindIndex = 0, bCurFound = 0; bindIndex < aControlBindings[funcId].numBindings && !bCurFound; ++bindIndex )
            {
                pBinding = &aControlBindings[funcId].aBindings[bindIndex];
                bAxis = pBinding->flags & SITHCONTROLBIND_AXISCONTROL;
                currentcontrolId = pBinding->controlId & 0x0FFFFFFF;
                if ( !bAxis && STDCONTROL_ISMOUSEBUTTON(currentcontrolId) || bAxis && STDCONTROL_ISMOUSEAXIS(currentcontrolId) )
                {
                    sithControl_UnbindFunctionIndex(funcId, bindIndex);
                    bCurFound = 1;
                }
            }
        } while ( bCurFound );
    }
}

// Discarded by the retail linker (no caller in either build): debug 0x44ea41. Name inferred.
void J3DAPI sithControl_RebindMouse(void)
{
    sithControl_UnbindMouseAxes();
    sithControl_RegisterMouseBindings();
}

void J3DAPI sithControl_Reset(void)
{
    memset(aControlBindings, 0, sizeof(aControlBindings));
    stdControl_Reset();
}

void J3DAPI sithControl_RegisterControlFunctions(void)
{
    sithControl_RegisterKeyFunction(SITHCONTROL_FORWARD, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_BACK, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_LEFT, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_RIGHT, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_TURNLEFT, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_TURNRIGHT, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_CRAWLTOGGLE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_JUMP, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_ACT1, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_RUNFWD, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_FIRE1, 0x64);
    sithControl_RegisterKeyFunction(SITHCONTROL_ACTIVATE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_ACT2, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_NEXTWEAP, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_PREVWEAP, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_LIGHTERTOGGLE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_LOOK, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_STPLEFT, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_STPRIGHT, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WEAPONTOGGLE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT0, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT1, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT2, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT3, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT4, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT5, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT6, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT7, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT8, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT9, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT10, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT11, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT12, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT13, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT14, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT15, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_WSELECT16, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_PREVWEAPON, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_NEXTWEAPON, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_GAMESAVE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_MAP, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_INCREASE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_DECREASE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_CAMERAMODE, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_TALK, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_ACT3, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_HEALTH, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_CHALK, 0);
#ifdef J3D_DEBUG
    sithControl_RegisterKeyFunction(SITHCONTROL_DEBUG, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_GAMMA, 0);
#endif
    sithControl_RegisterKeyFunction(SITHCONTROL_SCREENSHOT, 0);
#ifdef J3D_DEBUG
    sithControl_RegisterKeyFunction(SITHCONTROL_CAMERAZOOMIN, 0);
    sithControl_RegisterKeyFunction(SITHCONTROL_CAMERAZOOMOUT, 0);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithControl_RegisterKeyFunction(SITHCONTROL_MLOOK, 0);
        sithControl_RegisterAxisFunction(SITHCONTROL_PITCH, 8);
        sithControl_RegisterKeyFunction(SITHCONTROL_CENTER, 0);
        sithControl_RegisterAxisFunction(SITHCONTROL_MOUSETURN, 8);
    }
#endif
}

void J3DAPI sithControl_RegisterKeyboardBindings(void)
{
    if ( !bControlStartup )
    {
        return;
    }

    sithControl_BindControl(SITHCONTROL_FORWARD, DIK_UP, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_BACK, DIK_DOWN, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_LEFT, DIK_LEFT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_RIGHT, DIK_RIGHT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_TURNLEFT, DIK_LEFT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_TURNRIGHT, DIK_RIGHT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_LOOK, DIK_NUMPAD0, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_LOOK, DIK_INSERT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_STPLEFT, DIK_A, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_STPRIGHT, DIK_S, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_CRAWLTOGGLE, DIK_C, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WEAPONTOGGLE, DIK_SPACE, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_LIGHTERTOGGLE, DIK_L, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_JUMP, DIK_X, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_JUMP, DIK_LMENU, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_JUMP, DIK_RMENU, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_ACT1, DIK_LSHIFT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_ACT1, DIK_RSHIFT, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_FIRE1, DIK_RCONTROL, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_FIRE1, DIK_LCONTROL, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_ACT3, DIK_Z, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_ACTIVATE, DIK_GRAVE, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_ACT2, DIK_LCONTROL, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_ACT2, DIK_RCONTROL, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT0, DIK_0, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT1, DIK_1, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT2, DIK_2, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT3, DIK_3, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT4, DIK_4, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT5, DIK_5, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT6, DIK_6, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT7, DIK_7, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT8, DIK_8, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT9, DIK_9, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT10, DIK_G, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT11, DIK_P, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT12, DIK_Q, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT13, DIK_W, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT14, DIK_E, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT15, DIK_R, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_WSELECT16, DIK_T, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_PREVWEAPON, DIK_COMMA, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_NEXTWEAPON, DIK_PERIOD, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_GAMESAVE, DIK_F9, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_MAP, DIK_M, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_INCREASE, DIK_EQUALS, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_DECREASE, DIK_MINUS, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_TALK, DIK_F10, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_SCREENSHOT, DIK_F12, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_HEALTH, DIK_H, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_CHALK, DIK_K, (SithControlBindFlag)0);
#ifdef J3D_DEBUG
    sithControl_BindControl(SITHCONTROL_DEBUG, DIK_BACK, (SithControlBindFlag)0);
    sithControl_BindControl(SITHCONTROL_GAMMA, DIK_F5, (SithControlBindFlag)0);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithControl_BindControl(SITHCONTROL_MLOOK, DIK_V, (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_FORWARD, DIK_W, (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_PITCH, DIK_PRIOR, SITHCONTROLBIND_INVERT_AXIS);
        sithControl_BindControl(SITHCONTROL_PITCH, DIK_NEXT, (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_CENTER, DIK_HOME, (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_CENTER, DIK_NUMPAD5, (SithControlBindFlag)0);
    }
#endif
}

// Debug 0x44f0af: the function returns right after reading the joystick count; the binding loop after
// that `return` is unreachable (the debug code keeps its increment/test/body, the reached code jumps
// to the epilogue at 0x44f0cb), and retail compiles the body to a tail call of GetNumJoysticks.
void J3DAPI sithControl_RegisterJoystickBindings(void)
{
    size_t numJoysticks;
    size_t i;

    if ( !bControlStartup )
    {
        return;
    }

    numJoysticks = stdControl_GetNumJoysticks();
    return;

    for ( i = 0; i < numJoysticks; ++i )
    {
        sithControl_BindAxis(SITHCONTROL_TURNLEFT, STDCONTROL_GET_JOYSTICK_AXIS_X(i) | STDCONTROL_AID_NEGATIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_TURNRIGHT, STDCONTROL_GET_JOYSTICK_AXIS_X(i) | STDCONTROL_AID_POSITIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_FORWARD, STDCONTROL_GET_JOYSTICK_AXIS_Y(i) | STDCONTROL_AID_NEGATIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_RUNFWD, STDCONTROL_GET_JOYSTICK_AXIS_Y(i) | STDCONTROL_AID_NEGATIVE_AXIS | STDCONTROL_AID_LOW_SENSITIVITY, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_BACK, STDCONTROL_GET_JOYSTICK_AXIS_Y(i) | STDCONTROL_AID_POSITIVE_AXIS, (SithControlBindFlag)0);

        sithControl_BindControl(SITHCONTROL_ACT2, STDCONTROL_JOYSTICK_GETBUTTON(i, 0), (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_JUMP, STDCONTROL_JOYSTICK_GETBUTTON(i, 1), (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_ACT1, STDCONTROL_JOYSTICK_GETBUTTON(i, 2), (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_ACT3, STDCONTROL_JOYSTICK_GETBUTTON(i, 4), (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_CRAWLTOGGLE, STDCONTROL_JOYSTICK_GETBUTTON(i, 5), (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_WEAPONTOGGLE, STDCONTROL_JOYSTICK_GETBUTTON(i, 9), (SithControlBindFlag)0);
    }
}

void J3DAPI sithControl_RegisterMouseBindings(void)
{
    SithControlBinding* pBinding;

    if ( !bControlStartup )
    {
        return;
    }

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        pBinding = sithControl_BindAxis(SITHCONTROL_MOUSETURN, STDCONTROL_AID_MOUSE_X, SITHCONTROLBIND_AXIS_ENABLED | SITHCONTROLBIND_INVERT_AXIS);
        if ( pBinding )
        {
            pBinding->sensitivity = 0.4f;
        }

        pBinding = sithControl_BindAxis(SITHCONTROL_PITCH, STDCONTROL_AID_MOUSE_Y, SITHCONTROLBIND_AXIS_ENABLED);
        if ( pBinding )
        {
            pBinding->sensitivity = 0.3f;
        }

        pBinding = sithControl_BindAxis(SITHCONTROL_PITCH, STDCONTROL_AID_MOUSE_Z, (SithControlBindFlag)0);
        if ( pBinding )
        {
            pBinding->sensitivity = 4.0f;
        }

        sithControl_BindControl(SITHCONTROL_FIRE1, STDCONTROL_KID_MOUSE_LBUTTON, (SithControlBindFlag)0);
    }
    else
    {
        stdControl_EnableMouseSensitivity(1);
        sithControl_BindAxis(SITHCONTROL_TURNLEFT, STDCONTROL_AID_MOUSE_X | STDCONTROL_AID_NEGATIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_TURNRIGHT, STDCONTROL_AID_MOUSE_X | STDCONTROL_AID_POSITIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_FORWARD, STDCONTROL_AID_MOUSE_Y | STDCONTROL_AID_NEGATIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_RUNFWD, STDCONTROL_AID_MOUSE_Y | STDCONTROL_AID_NEGATIVE_AXIS | STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_LOW_SENSITIVITY, (SithControlBindFlag)0);
        sithControl_BindAxis(SITHCONTROL_BACK, STDCONTROL_AID_MOUSE_Y | STDCONTROL_AID_POSITIVE_AXIS, (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_ACT2, STDCONTROL_KID_MOUSE_LBUTTON, (SithControlBindFlag)0);
        sithControl_BindControl(SITHCONTROL_JUMP, STDCONTROL_KID_MOUSE_RBUTTON, (SithControlBindFlag)0);
    }

    stdControl_EnableMouse(1);
}

// Discarded by the retail linker (no caller in either build): debug 0x44f334, the file's last function. Calls
// pfFunc for every binding the three device filters admit, then once per function with bindIndex -1 for a
// missing axis, key and inverted-key binding. Name from Jedi Knight's sithControl_EnumBindings (same five
// arguments); filter names inferred.
void J3DAPI sithControl_EnumBindings(int (J3DAPI* pfFunc)(size_t, const char*, SithControlFunctionFlag, size_t, int, SithControlBindFlag, SithControlBinding*, int), int bKeyboard, int bJoystick, int bMouse, int userData)
{
    size_t bindIndex;
    int bContinue;
    size_t funcId;
    SithControlFunctionBinding* pBinding;
    int bAxisFunc;
    int bInvertedKeyBinding;
    int bKeyBinding;
    int bAxisBinding;
    int controlId;
    SithControlBindFlag flags;

    bContinue = 1;
    for ( funcId = 0; bContinue && funcId < SITHCONTROL_MAXFUNCTIONS; ++funcId )
    {
        bKeyBinding = 0;
        bInvertedKeyBinding = 0;
        bAxisBinding = 0;
        pBinding = &aControlBindings[funcId];
        bAxisFunc = aControlFlags[funcId] & SITHCONTROLFUNCTION_AXIS;
        for ( bindIndex = 0; bContinue && bindIndex < pBinding->numBindings; ++bindIndex )
        {
            controlId = pBinding->aBindings[bindIndex].controlId;
            flags = pBinding->aBindings[bindIndex].flags;
            if ( (flags & SITHCONTROLBIND_KEYCONTROL) != 0 && STDCONTROL_ISKEYBOARDBUTTON(controlId) && !bKeyboard )
            {
                continue;
            }

            if ( ((flags & SITHCONTROLBIND_AXISCONTROL) != 0 && STDCONTROL_ISMOUSEAXIS(controlId)
                    || (flags & SITHCONTROLBIND_KEYCONTROL) != 0 && STDCONTROL_ISMOUSEBUTTON(controlId))
                && !bMouse )
            {
                continue;
            }

            if ( ((flags & SITHCONTROLBIND_AXISCONTROL) != 0 && STDCONTROL_ISJOYSTICKAXIS(controlId)
                    || (flags & SITHCONTROLBIND_KEYCONTROL) != 0 && STDCONTROL_ISJOYSTICKBUTTON(controlId))
                && !bJoystick )
            {
                continue;
            }

            bContinue = pfFunc(funcId, aFunctionNames[funcId], aControlFlags[funcId], bindIndex, controlId, flags, &pBinding->aBindings[bindIndex], userData);
            bKeyBinding = bKeyBinding
                || (pBinding->aBindings[bindIndex].flags & SITHCONTROLBIND_KEYCONTROL) != 0
                && (pBinding->aBindings[bindIndex].flags & SITHCONTROLBIND_INVERT_AXIS) == 0;
            bInvertedKeyBinding = bInvertedKeyBinding
                || (pBinding->aBindings[bindIndex].flags & SITHCONTROLBIND_KEYCONTROL) != 0
                && (pBinding->aBindings[bindIndex].flags & SITHCONTROLBIND_INVERT_AXIS) != 0;
            bAxisBinding = bAxisBinding || (pBinding->aBindings[bindIndex].flags & SITHCONTROLBIND_AXISCONTROL) != 0;
        }

        if ( bContinue && bAxisFunc && !bAxisBinding )
        {
            bContinue = pfFunc(funcId, aFunctionNames[funcId], aControlFlags[funcId], (size_t)-1, 0, SITHCONTROLBIND_AXISCONTROL, NULL, userData);
        }

        if ( bContinue && !bKeyBinding || !pBinding->numBindings )
        {
            bContinue = pfFunc(funcId, aFunctionNames[funcId], aControlFlags[funcId], (size_t)-1, 0, SITHCONTROLBIND_KEYCONTROL, NULL, userData);
        }

        if ( bContinue && bAxisFunc && !bInvertedKeyBinding )
        {
            bContinue = pfFunc(funcId, aFunctionNames[funcId], aControlFlags[funcId], (size_t)-1, 0, SITHCONTROLBIND_INVERT_AXIS | SITHCONTROLBIND_KEYCONTROL, NULL, userData);
        }
    }
}
