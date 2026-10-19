#include <Windows.h>
#include <dinput.h>

#include "stdControl.h"
#include "stdWin95.h"

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

#define STDCONTROL_MOUSE_BUFFERSIZE 32u

static int stdControl_bReadMouse = 1;
static float stdControl_updateKHz = 1.0f;
static float stdControl_updateHz  = 1.0f;
// Unreferenced in Indy; names and values from Jedi Knight's stdControl (OpenJKDF2 symbols.syms), which has the same five 1.0f floats after bReadMouse.
static float stdControl_mouseXSensitivity = 1.0f;
static float stdControl_mouseYSensitivity = 1.0f;
static float stdControl_mouseZSensitivity = 1.0f;

static StdControlJoystickDevice stdControl_aJoystickDevices[STDCONTROL_MAX_JOYSTICK_DEVICES] = { 0 };
static int stdControl_cursorDisplayCounter = 0;
static uint8_t stdControl_aKeyboardState[STDCONTROL_MAX_KEYBOARD_BUTTONS] = { 0 };
static char stdControl_aStrBuf[128] = { 0 }; // Unreferenced retail storage; used by the debug-build GetJoysticDescription
static int stdControl_aAxisStates[STDCONTROL_MAX_AXES] = { 0 };
static StdInputDevice stdControl_keyboard = { 0 };
static uint32_t stdControl_aKeyIdleTimes[STDCONTROL_MAX_KEYID] = { 0 };
static int stdControl_aKeyInfo[STDCONTROL_MAX_KEYID] = { 0 };
StdControlAxis stdControl_aAxes[STDCONTROL_MAX_AXES] = { 0 };
static StdInputDevice stdControl_mouse = { 0 };
static POINT stdControl_mousePos = { 0 };
static int stdControl_aKeyPressed[STDCONTROL_MAX_KEYID] = { 0 };

static int stdControl_bStartup = 0;
int stdControl_bOpen = 0;
int stdControl_bControlsIdle = 0;
int stdControl_bControlsActive = 0;
static LPDIRECTINPUTA stdControl_pDI = 0;
static int stdControl_bMouseSensitivityEnabled = 0;
static int stdControl_bReadJoysticks = 0;
static int stdControl_numJoystickDevices = 0;
static uint32_t stdControl_curReadTime = 0;
static uint32_t stdControl_lastReadTime = 0;
static uint32_t stdControl_unknown = 0; // Unreferenced retail slot; Jedi Knight has an unnamed slot at the same position
static uint32_t stdControl_readDeltaTime = 0;

static const DXStatus stdControl_aDIStatusTbl[34] = {
    { DI_OK,                        "DI_OK" },
    { DI_NOTATTACHED,               "DI_NOTATTACHED" },
    { DI_BUFFEROVERFLOW,            "DI_BUFFEROVERFLOW" },
    { DI_PROPNOEFFECT,              "DI_PROPNOEFFECT" },
    { DI_POLLEDDEVICE,              "DI_POLLEDDEVICE" },
    { DIERR_OLDDIRECTINPUTVERSION,  "DIERR_OLDDIRECTINPUTVERSION" },
    { DIERR_BETADIRECTINPUTVERSION, "DIERR_BETADIRECTINPUTVERSION" },
    { DIERR_BADDRIVERVER,           "DIERR_BADDRIVERVER" },
    { DIERR_DEVICENOTREG,           "DIERR_DEVICENOTREG" },
    { DIERR_NOTFOUND,               "DIERR_NOTFOUND" },
    { DIERR_OBJECTNOTFOUND,         "DIERR_OBJECTNOTFOUND" },
    { DIERR_INVALIDPARAM,           "DIERR_INVALIDPARAM" },
    { DIERR_NOINTERFACE,            "DIERR_NOINTERFACE" },
    { DIERR_GENERIC,                "DIERR_GENERIC" },
    { DIERR_OUTOFMEMORY,            "DIERR_OUTOFMEMORY" },
    { DIERR_UNSUPPORTED,            "DIERR_UNSUPPORTED" },
    { DIERR_NOTINITIALIZED,         "DIERR_NOTINITIALIZED" },
    { DIERR_ALREADYINITIALIZED,     "DIERR_ALREADYINITIALIZED" },
    { DIERR_NOAGGREGATION,          "DIERR_NOAGGREGATION" },
    { DIERR_OTHERAPPHASPRIO,        "DIERR_OTHERAPPHASPRIO" },
    { DIERR_INPUTLOST,              "DIERR_INPUTLOST" },
    { DIERR_ACQUIRED,               "DIERR_ACQUIRED" },
    { DIERR_NOTACQUIRED,            "DIERR_NOTACQUIRED" },
    { DIERR_READONLY,               "DIERR_READONLY" },
    { DIERR_HANDLEEXISTS,           "DIERR_HANDLEEXISTS" },
    { DIERR_INSUFFICIENTPRIVS,      "DIERR_INSUFFICIENTPRIVS" },
    { DIERR_DEVICEFULL,             "DIERR_DEVICEFULL" },
    { DIERR_MOREDATA,               "DIERR_MOREDATA" },
    { DIERR_NOTDOWNLOADED,          "DIERR_NOTDOWNLOADED" },
    { DIERR_HASEFFECTS,             "DIERR_HASEFFECTS" },
    { DIERR_NOTEXCLUSIVEACQUIRED,   "DIERR_NOTEXCLUSIVEACQUIRED" },
    { DIERR_INCOMPLETEEFFECT,       "DIERR_INCOMPLETEEFFECT" },
    { DIERR_NOTBUFFERED,            "DIERR_NOTBUFFERED" },
    { DIERR_EFFECTPLAYING,          "DIERR_EFFECTPLAYING" }
};

static __inline void J3DAPI stdControl_UpdateKeyState(int keyId, int bPressed, unsigned int tickTime);
void J3DAPI stdControl_InitJoysticks(void);
void J3DAPI stdControl_InitKeyboard(int bForeground);
void J3DAPI stdControl_InitMouse(void);

void J3DAPI stdControl_EnableAxisRead(size_t axis);

void J3DAPI stdControl_ReadKeyboard(void);
void J3DAPI stdControl_ReadJoysticks(void);
void J3DAPI stdControl_ReadMouse(void);

const char* J3DAPI stdControl_DIGetStatus(int HRESULT);
BOOL CALLBACK stdControl_EnumDevicesCallback(LPCDIDEVICEINSTANCEA pdidInstance, LPVOID pContext);

int J3DAPI stdControl_Startup(int bKeyboardForeground)
{
    HRESULT hres;

    if ( stdControl_bStartup )
    {
        return 1;
    }

    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyInfo, 0, sizeof(stdControl_aKeyInfo));
    memset(stdControl_aAxes, 0, sizeof(stdControl_aAxes));
    memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));

    stdControl_mousePos.x = 0;
    stdControl_mousePos.y = 0;
    memset(&stdControl_keyboard, 0, sizeof(stdControl_keyboard));
    stdControl_numJoystickDevices = 0;
    memset(stdControl_aJoystickDevices, 0, sizeof(stdControl_aJoystickDevices));
    memset(&stdControl_mouse, 0, sizeof(stdControl_mouse));
    stdControl_cursorDisplayCounter = 0;

    hres = DirectInputCreate(stdWin95_GetInstance(), DIRECTINPUT_VERSION, &stdControl_pDI, NULL);
    if ( hres != DI_OK )
    {
#line 225
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectInputCreate returned %s.\n", stdControl_DIGetStatus(hres), 0, 0, 0);
        return 1;
    }

    hres = IDirectInput_EnumDevices(stdControl_pDI, 0, stdControl_EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
    if ( hres != DI_OK )
    {
#line 233
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create DInput Joystick device.\n", 0, 0, 0, 0);
        return 1;
    }

    stdControl_InitKeyboard(bKeyboardForeground);
    stdControl_InitJoysticks();
    stdControl_InitMouse();
    stdControl_Reset();

    stdControl_bStartup = 1;
    return 0;
}

void J3DAPI stdControl_Shutdown(void)
{
    if ( !stdControl_bStartup )
    {
        return;
    }

    stdControl_bStartup = 0;
    if ( stdControl_mouse.pDIDevice )
    {
        IDirectInputDevice_Unacquire(stdControl_mouse.pDIDevice);
        IDirectInputDevice_Release(stdControl_mouse.pDIDevice);
    }

    memset(&stdControl_mouse, 0, sizeof(stdControl_mouse));

    if ( stdControl_keyboard.pDIDevice )
    {
        IDirectInputDevice_Unacquire(stdControl_keyboard.pDIDevice);
        IDirectInputDevice_Release(stdControl_keyboard.pDIDevice);
    }

    memset(&stdControl_keyboard, 0, sizeof(stdControl_keyboard));

    {
        size_t i;
        for ( i = 0; i < stdControl_numJoystickDevices; i++ )
        {
            if ( stdControl_aJoystickDevices[i].pDIDevice )
            {
                IDirectInputDevice2_Unacquire(stdControl_aJoystickDevices[i].pDIDevice);
                IDirectInputDevice2_Release(stdControl_aJoystickDevices[i].pDIDevice);
            }
        }
    }

    stdControl_numJoystickDevices = 0;
    memset(stdControl_aJoystickDevices, 0, sizeof(stdControl_aJoystickDevices));
    if ( stdControl_pDI )
    {
        IDirectInput_Release(stdControl_pDI);
        stdControl_pDI = 0;
    }
}

int J3DAPI stdControl_Open(void)
{
    if ( !stdControl_bStartup )
    {
        return 1;
    }

    stdControl_bOpen = 1;
    stdControl_SetActivation(1);
    return 0;
}

void J3DAPI stdControl_Close(void)
{
    if ( !stdControl_bOpen )
    {
        return;
    }

    stdControl_SetActivation(0);
    stdControl_bOpen = 0;
}

void J3DAPI stdControl_Reset(void)
{
    stdControl_bReadMouse     = 0;
    stdControl_bReadJoysticks = 0;
    stdControl_bMouseSensitivityEnabled = 0;
    {
        size_t i;
        for ( i = 0; i < STDCONTROL_MAX_AXES; i++ )
        {
            stdControl_aAxes[i].flags &= ~STDCONTROL_AXIS_ENABLED;
        }
    }
}

void J3DAPI stdControl_DisableReadJoysticks(void)
{
    stdControl_bReadJoysticks = 0;
}

int J3DAPI stdControl_EnableAxis(size_t axis)
{
    axis = STDCONTROL_GETAID(axis);
    if ( axis >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_REGISTERED) == 0 )
    {
        return 0;
    }

    stdControl_aAxes[axis].flags |= STDCONTROL_AXIS_ENABLED;
    stdControl_EnableAxisRead(axis);
    return 1;
}

void J3DAPI stdControl_ReadControls(void)
{
    size_t first;
    size_t last;

#line 441
    STD_ASSERTREL(stdControl_bStartup && stdControl_bOpen);
    if ( !stdControl_bControlsActive )
    {
        return;
    }

    stdControl_bControlsIdle = 1;
    memset(stdControl_aKeyIdleTimes, 0, sizeof(stdControl_aKeyIdleTimes));
    memset(stdControl_aKeyPressed, 0, sizeof(stdControl_aKeyPressed));

    stdControl_curReadTime   = timeGetTime();
    stdControl_readDeltaTime = stdControl_curReadTime - stdControl_lastReadTime;

    stdControl_updateKHz = 1.0f / D3DVAL(stdControl_readDeltaTime);
    stdControl_updateHz  = D3DMultiply(1000.0f, stdControl_updateKHz);

    if ( stdControl_bMouseSensitivityEnabled )
    {
        first = 0;
        last  = 7;
        memset(&stdControl_aAxisStates[first], 0, last - first);
    }
    else
    {
        memset(stdControl_aAxisStates, 0, sizeof(stdControl_aAxisStates));
    }

    stdControl_ReadKeyboard();

    if ( stdControl_bReadJoysticks )
    {
        stdControl_ReadJoysticks();
    }

    stdControl_ReadMouse();
    stdControl_lastReadTime = stdControl_curReadTime;
}

float J3DAPI stdControl_ReadAxis(size_t axis)
{
    float curvalue;
    int pos;

    axis = STDCONTROL_GETAID(axis);
    if ( axis >= STDCONTROL_MAX_AXES )
    {
        return 0.0f;
    }

    if ( !stdControl_bControlsActive )
    {
        return 0.0f;
    }

    if ( (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_ENABLED) == 0 )
    {
        return 0.0f;
    }

    pos = stdControl_aAxisStates[axis] - stdControl_aAxes[axis].center;
    if ( !pos )
    {
        return 0.0f;
    }

    if ( (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_HASDEADZONE) == 0 && stdControl_aAxes[axis].deadzoneThreshold && (pos < 0 ? -pos : pos) < stdControl_aAxes[axis].deadzoneThreshold )
    {
        return 0.0f;
    }

    curvalue = D3DMultiply(D3DVAL(pos), stdControl_aAxes[axis].scale);
    curvalue = (curvalue < 0.0f ? -curvalue : curvalue) > 0.00001f ? curvalue : 0.0f;

    if ( stdControl_bControlsIdle && curvalue != 0.0f )
    {
        stdControl_bControlsIdle = 0;
    }

    return curvalue;
}

int J3DAPI stdControl_ReadAxisRaw(size_t axis)
{
    int pos;

    axis = STDCONTROL_GETAID(axis);
    if ( axis >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( !stdControl_bControlsActive )
    {
        return 0;
    }

    if ( (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_ENABLED) == 0 )
    {
        return 0;
    }

    pos = stdControl_aAxisStates[axis] - stdControl_aAxes[axis].center;
    if ( !pos )
    {
        return 0;
    }

    if ( stdControl_bControlsIdle && pos )
    {
        stdControl_bControlsIdle = 0;
    }

    return pos;
}

float J3DAPI stdControl_ReadKeyAsAxis(size_t keyId)
{
    DWORD time;
    float deltaTime;

#line 613
    STD_ASSERTREL((keyId < STDCONTROL_MAX_KEYID));
    if ( !stdControl_bControlsActive )
    {
        return 0.0f;
    }

    time = stdControl_aKeyIdleTimes[keyId];
    if ( !time )
    {
        if ( stdControl_aKeyInfo[keyId] )
        {
            time = stdControl_readDeltaTime;
        }
        else
        {
            return 0.0f;
        }
    }

    time = time < stdControl_readDeltaTime ? time : stdControl_readDeltaTime;
    deltaTime = D3DMultiply(D3DVAL(time), stdControl_updateKHz);
    if ( stdControl_bControlsIdle && deltaTime != 0.0f )
    {
        stdControl_bControlsIdle = 0;
    }

    return deltaTime;
}

int J3DAPI stdControl_ReadAxisAsKey(size_t axis, int* pbPressed)
{
    if ( (axis & STDCONTROL_AID_LOW_SENSITIVITY) != 0 && (stdControl_aAxes[axis].flags & STDCONTROL_AXIS_GAMEPAD) == 0 )
    {
        return stdControl_ReadAxisAsKeyEx(axis, pbPressed, 0.75f);
    }
    else
    {
        return stdControl_ReadAxisAsKeyEx(axis, pbPressed, 0.25f);
    }
}

int J3DAPI stdControl_ReadAxisAsKeyEx(size_t axis, int* pbPressed, float lowValue)
{
    float pos = stdControl_ReadAxis(axis);

    if ( (axis & (STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS)) != 0 )
    {
        axis &= ~STDCONTROL_AID_LOW_SENSITIVITY;
        axis &= (STDCONTROL_AID_LOW_SENSITIVITY | STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS);
        if ( axis == STDCONTROL_AID_POSITIVE_AXIS && pos > lowValue )
        {
            return 1;
        }

        if ( axis == STDCONTROL_AID_NEGATIVE_AXIS && pos < -lowValue )
        {
            return 1;
        }
    }
    else if ( (pos < 0.0f ? -pos : pos) > lowValue )
    {
        return 1;
    }

    return 0;
}

int J3DAPI stdControl_ReadKey(size_t keyNum, int* pbPressed)
{
#line 710
    STD_ASSERTREL(keyNum < STDCONTROL_MAX_KEYID);
    if ( !stdControl_bControlsActive )
    {
        if ( pbPressed )
        {
            *pbPressed = 0;
        }

        return 0;
    }

    if ( pbPressed )
    {
        *pbPressed += stdControl_aKeyPressed[keyNum];
    }

    if ( stdControl_bControlsIdle && stdControl_aKeyInfo[keyNum] )
    {
        stdControl_bControlsIdle = 0;
    }

    return stdControl_aKeyInfo[keyNum];
}

// Retail folded this empty body into the shared `ret` at 0x004EE700; sithControl_FinishRead tail-jumps there.
void J3DAPI stdControl_FinishRead(void)
{}

/* Discarded by the retail linker (no caller in either image); restored in full from the debug build
   (0x004EECDC, between FinishRead and ControlsActive). Its 250.0f/200.0f are the TU's last pool slots
   (0x005084D8), referenced by nothing in retail. Name and parameters are Jedi Knight's. */
void J3DAPI stdControl_SetMouseSensitivity(float xSensitivity, float ySensitivity)
{
    int xLimit;
    int yLimit;

    stdControl_mouseXSensitivity = xSensitivity;
    stdControl_mouseYSensitivity = ySensitivity;

    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_X].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        xLimit = (int)(stdControl_mouseXSensitivity * 250.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -xLimit, xLimit, 0.0f);
    }

    if ( (stdControl_aAxes[STDCONTROL_AID_MOUSE_Y].flags & STDCONTROL_AXIS_REGISTERED) != 0 )
    {
        yLimit = (int)(stdControl_mouseYSensitivity * 200.0f);
        stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -yLimit, yLimit, 0.0f);
    }
}

int J3DAPI stdControl_ControlsActive(void)
{
    return stdControl_bControlsActive;
}

int J3DAPI stdControl_SetActivation(int bActive)
{
    HRESULT hres = 1;
    HRESULT hr;
    size_t i;

    if ( !stdControl_bOpen )
    {
        return 1;
    }

    for ( i = 0; i < STDCONTROL_MAX_KEYBOARD_BUTTONS; ++i )
    {
        stdControl_UpdateKeyState(i, 0, stdControl_curReadTime);
    }

    if ( stdControl_bReadMouse && stdControl_mouse.pDIDevice )
    {
        for ( i = 0; i < STDCONTROL_MAX_MOUSE_BUTTONS; ++i )
        {
            stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + i, 0, stdControl_curReadTime);
        }
    }

    if ( bActive )
    {
        if ( stdControl_bReadMouse && stdControl_mouse.pDIDevice )
        {
            hres = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
        }

        if ( stdControl_keyboard.pDIDevice )
        {
            hr = IDirectInputDevice_Acquire(stdControl_keyboard.pDIDevice);
        }

        for ( i = 0; i < stdControl_numJoystickDevices; ++i )
        {
            if ( stdControl_aJoystickDevices[i].pDIDevice )
            {
                hr = IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[i].pDIDevice);
            }
        }

        stdControl_bControlsActive = 1;
    }
    else
    {
        if ( stdControl_mouse.pDIDevice )
        {
            hres = IDirectInputDevice_Unacquire(stdControl_mouse.pDIDevice);
        }

        if ( stdControl_keyboard.pDIDevice )
        {
            hr = IDirectInputDevice_Unacquire(stdControl_keyboard.pDIDevice);
        }

        for ( i = 0; i < stdControl_numJoystickDevices; ++i )
        {
            if ( stdControl_aJoystickDevices[i].pDIDevice )
            {
                hr = IDirectInputDevice2_Unacquire(stdControl_aJoystickDevices[i].pDIDevice);
            }
        }

        stdControl_bControlsActive = 0;
    }

    return hres;
}

int J3DAPI stdControl_ToggleMouse(void)
{
    HRESULT hres;

    if ( stdControl_bReadMouse )
    {
        stdControl_bReadMouse = 0;
        if ( stdControl_mouse.pDIDevice )
        {
            hres = IDirectInputDevice_Unacquire(stdControl_mouse.pDIDevice);
            if ( hres == DI_OK )
            {
                stdControl_ShowMouseCursor(1);
            }
        }
    }
    else
    {
        stdControl_bReadMouse = 1;
        if ( stdControl_mouse.pDIDevice )
        {
            hres = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
            if ( hres == DI_OK )
            {
                stdControl_ShowMouseCursor(0);
            }
        }
    }

    return stdControl_bReadMouse;
}


int J3DAPI stdControl_EnableMouse(int bEnable)
{
    if ( bEnable != stdControl_bReadMouse )
    {
        stdControl_ToggleMouse();
    }

    return 1;
}

int J3DAPI stdControl_ControlsIdle(void)
{
    return stdControl_bControlsIdle;
}

int J3DAPI stdControl_TestAxisFlag(size_t axis, StdControlAxisFlag flags)
{
    uint32_t bPositiveAxis = axis & STDCONTROL_AID_POSITIVE_AXIS;
    uint32_t bNegativeAxis = axis & STDCONTROL_AID_NEGATIVE_AXIS;
    axis = STDCONTROL_GETAID(axis);
    if ( axis >= STDCONTROL_MAX_AXES )
    {
        return 0;
    }

    if ( (flags & STDCONTROL_AXIS_ENABLED) != 0 )
    {
        if ( bPositiveAxis )
        {
            flags &= ~STDCONTROL_AXIS_ENABLED;
            flags |= STDCONTROL_AXIS_POSITIVE;
        }

        if ( bNegativeAxis )
        {
            flags &= ~STDCONTROL_AXIS_ENABLED;
            flags |= STDCONTROL_AXIS_NEGATIVE;
        }
    }

    return stdControl_aAxes[axis].flags & flags;
}

void J3DAPI stdControl_SetAxisFlags(size_t axis, StdControlAxisFlag flags)
{
    uint32_t bPositiveAxis = axis & STDCONTROL_AID_POSITIVE_AXIS;
    uint32_t bNegativeAxis = axis & STDCONTROL_AID_NEGATIVE_AXIS;
    axis = STDCONTROL_GETAID(axis);
    if ( axis >= STDCONTROL_MAX_AXES )
    {
        return;
    }

    if ( (flags & STDCONTROL_AXIS_ENABLED) != 0 )
    {
        if ( bPositiveAxis )
        {
            flags |= STDCONTROL_AXIS_POSITIVE;
        }

        if ( bNegativeAxis )
        {
            flags |= STDCONTROL_AXIS_NEGATIVE;
        }
    }

    stdControl_aAxes[axis].flags |= flags;
}

static __inline void J3DAPI stdControl_UpdateKeyState(int keyId, int bPressed, unsigned int tickTime)
{
    if ( bPressed && !stdControl_aKeyInfo[keyId] )
    {
        stdControl_aKeyInfo[keyId] = 1;
        stdControl_aKeyIdleTimes[keyId] = stdControl_curReadTime - tickTime;
        ++stdControl_aKeyPressed[keyId];
    }
    else if ( !bPressed && stdControl_aKeyInfo[keyId] )
    {
        stdControl_aKeyInfo[keyId] = 0;
        if ( !stdControl_aKeyIdleTimes[keyId] )
        {
            stdControl_aKeyIdleTimes[keyId] = stdControl_readDeltaTime;
        }

        stdControl_aKeyIdleTimes[keyId] -= stdControl_curReadTime - tickTime;
    }
}

void J3DAPI stdControl_InitJoysticks(void)
{
    HRESULT hres;
    size_t curjoyNumber;
    DIPROPRANGE curdirange;
    DIPROPDWORD dipdw;
    LPDIRECTINPUTDEVICEA curlpd = NULL;

    for ( curjoyNumber = 0; curjoyNumber < stdControl_numJoystickDevices; ++curjoyNumber )
    {
        hres = IDirectInput_CreateDevice(stdControl_pDI, &stdControl_aJoystickDevices[curjoyNumber].dinstance.guidInstance, &curlpd, NULL);
        if ( FAILED(hres) )
        {
#line 1064
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create DInput Joystick device.\n", 0, 0, 0, 0);
        }

        hres = IDirectInputDevice_QueryInterface(curlpd, &IID_IDirectInputDevice2A, (LPVOID*)&stdControl_aJoystickDevices[curjoyNumber].pDIDevice);
        IDirectInputDevice_Release(curlpd);
        curlpd = NULL;

        if ( FAILED(hres) )
        {
            goto error;
        }

        stdControl_aJoystickDevices[curjoyNumber].caps.dwSize = sizeof(DIDEVCAPS);
        hres = IDirectInputDevice2_GetCapabilities(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, &stdControl_aJoystickDevices[curjoyNumber].caps);
        if ( FAILED(hres) )
        {
            goto error;
        }

        hres = IDirectInputDevice2_SetDataFormat(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, &c_dfDIJoystick);
        if ( FAILED(hres) )
        {
            goto error;
        }

        hres = IDirectInputDevice2_SetCooperativeLevel(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, stdWin95_GetWindow(), DISCL_BACKGROUND | DISCL_EXCLUSIVE);
        if ( FAILED(hres) )
        {
            goto error;
        }

        curdirange.diph.dwSize       = sizeof(DIPROPRANGE);
        curdirange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        curdirange.diph.dwObj        = DIJOFS_X;
        curdirange.diph.dwHow        = DIPH_BYOFFSET;

        hres = IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, DIPROP_RANGE, &curdirange.diph);
        if ( SUCCEEDED(hres) )
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_X(curjoyNumber), curdirange.lMin, curdirange.lMax, 0.2f);
        }

        curdirange.diph.dwObj = DIJOFS_Y;
        hres = IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, DIPROP_RANGE, &curdirange.diph);
        if ( SUCCEEDED(hres) )
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_Y(curjoyNumber), curdirange.lMin, curdirange.lMax, 0.2f);
        }

        curdirange.diph.dwObj = DIJOFS_Z;
        hres = IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, DIPROP_RANGE, &curdirange.diph);
        if ( SUCCEEDED(hres) )
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_Z(curjoyNumber), curdirange.lMin, curdirange.lMax, 0.2f);
        }

        curdirange.diph.dwObj = DIJOFS_RX;
        hres = IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, DIPROP_RANGE, &curdirange.diph);
        if ( SUCCEEDED(hres) )
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RX(curjoyNumber), curdirange.lMin, curdirange.lMax, 0.2f);
        }

        curdirange.diph.dwObj = DIJOFS_RY;
        hres = IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, DIPROP_RANGE, &curdirange.diph);
        if ( SUCCEEDED(hres) )
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RY(curjoyNumber), curdirange.lMin, curdirange.lMax, 0.2f);
        }

        curdirange.diph.dwObj = DIJOFS_RZ;
        hres = IDirectInputDevice2_GetProperty(stdControl_aJoystickDevices[curjoyNumber].pDIDevice, DIPROP_RANGE, &curdirange.diph);
        if ( SUCCEEDED(hres) )
        {
            stdControl_RegisterAxis(STDCONTROL_GET_JOYSTICK_AXIS_RZ(curjoyNumber), curdirange.lMin, curdirange.lMax, 0.2f);
        }

        if ( GET_DIDEVICE_SUBTYPE(stdControl_aJoystickDevices[curjoyNumber].dinstance.dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD )
        {
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Y(curjoyNumber)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_X(curjoyNumber)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Z(curjoyNumber)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RX(curjoyNumber)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RY(curjoyNumber)].flags |= STDCONTROL_AXIS_GAMEPAD;
            stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RZ(curjoyNumber)].flags |= STDCONTROL_AXIS_GAMEPAD;
        }

        if ( (stdControl_aJoystickDevices[curjoyNumber].caps.dwFlags & DIDC_FORCEFEEDBACK) != 0 )
        {
            dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
            dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
            dipdw.diph.dwObj        = 0;
            dipdw.diph.dwHow        = DIPH_DEVICE;
            dipdw.dwData            = 0;
        }

        hres = IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[curjoyNumber].pDIDevice);
        if ( FAILED(hres) )
        {
            goto error;
        }

        continue;

    error:
#line 1141
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s error Acquiring Joystick.\n", stdControl_DIGetStatus(hres), 0, 0, 0);
        if ( stdControl_aJoystickDevices[curjoyNumber].pDIDevice )
        {
            IDirectInputDevice2_Release(stdControl_aJoystickDevices[curjoyNumber].pDIDevice);
        }

        stdControl_aJoystickDevices[curjoyNumber].pDIDevice = NULL;
        stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_X(curjoyNumber)].flags &= ~STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Y(curjoyNumber)].flags &= ~STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_Z(curjoyNumber)].flags &= ~STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RX(curjoyNumber)].flags &= ~STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RY(curjoyNumber)].flags &= ~STDCONTROL_AXIS_REGISTERED;
        stdControl_aAxes[STDCONTROL_GET_JOYSTICK_AXIS_RZ(curjoyNumber)].flags &= ~STDCONTROL_AXIS_REGISTERED;
    }
}

void J3DAPI stdControl_InitKeyboard(int bForeground)
{
    HRESULT hres;

    if ( !stdControl_pDI )
    {
        return;
    }

    hres = IDirectInput_CreateDevice(stdControl_pDI, &GUID_SysKeyboard, &stdControl_keyboard.pDIDevice, NULL);
    if ( FAILED(hres) )
    {
        goto error;
    }

    stdControl_keyboard.diDevCaps.dwSize = sizeof(DIDEVCAPS);
    hres = IDirectInputDevice_GetCapabilities(stdControl_keyboard.pDIDevice, &stdControl_keyboard.diDevCaps);
    if ( FAILED(hres) )
    {
        goto error;
    }

    hres = IDirectInputDevice_SetDataFormat(stdControl_keyboard.pDIDevice, &c_dfDIKeyboard);
    if ( FAILED(hres) )
    {
        goto error;
    }

    if ( bForeground )
    {
        hres = IDirectInputDevice_SetCooperativeLevel(stdControl_keyboard.pDIDevice, stdWin95_GetWindow(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
    }
    else
    {
        hres = IDirectInputDevice_SetCooperativeLevel(stdControl_keyboard.pDIDevice, stdWin95_GetWindow(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
    }

    if ( FAILED(hres) )
    {
        goto error;
    }

    {
        DIPROPDWORD didpw;
        didpw.diph.dwSize       = sizeof(DIPROPDWORD);
        didpw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        didpw.diph.dwObj        = 0;
        didpw.diph.dwHow        = DIPH_DEVICE;
        didpw.dwData            = STD_ARRAYLEN(stdControl_aKeyboardState);

        hres = IDirectInputDevice_SetProperty(stdControl_keyboard.pDIDevice, DIPROP_BUFFERSIZE, &didpw.diph);
        if ( FAILED(hres) )
        {
            goto error;
        }
    }

    return;

error:
#line 1211
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s error Acquiring Keyboard.\n", stdControl_DIGetStatus(hres), 0, 0, 0);
    if ( stdControl_keyboard.pDIDevice )
    {
        IDirectInputDevice_Release(stdControl_keyboard.pDIDevice);
    }

    stdControl_keyboard.pDIDevice = NULL;
}

void J3DAPI stdControl_InitMouse(void)
{
    int xLimit;
    int yLimit;
    int zLimit;
    HRESULT hres;

    if ( !stdControl_pDI )
    {
        return;
    }

    hres = IDirectInput_CreateDevice(stdControl_pDI, &GUID_SysMouse, &stdControl_mouse.pDIDevice, NULL);
    if ( FAILED(hres) )
    {
        goto error;
    }

    stdControl_mouse.diDevCaps.dwSize = sizeof(DIDEVCAPS);
    hres = IDirectInputDevice_GetCapabilities(stdControl_mouse.pDIDevice, &stdControl_mouse.diDevCaps);
    if ( FAILED(hres) )
    {
        goto error;
    }

    hres = IDirectInputDevice_SetDataFormat(stdControl_mouse.pDIDevice, &c_dfDIMouse);
    if ( FAILED(hres) )
    {
        goto error;
    }

    hres = IDirectInputDevice_SetCooperativeLevel(stdControl_mouse.pDIDevice, stdWin95_GetWindow(), DISCL_FOREGROUND | DISCL_EXCLUSIVE);
    if ( FAILED(hres) )
    {
        goto error;
    }

    {
        DIPROPDWORD didpw;
        didpw.diph.dwSize       = sizeof(DIPROPDWORD);
        didpw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        didpw.diph.dwObj        = 0;
        didpw.diph.dwHow        = DIPH_DEVICE;
        didpw.dwData            = STDCONTROL_MOUSE_BUFFERSIZE;

        hres = IDirectInputDevice_SetProperty(stdControl_mouse.pDIDevice, DIPROP_BUFFERSIZE, &didpw.diph);
        if ( FAILED(hres) )
        {
            goto error;
        }
    }

    xLimit = 250;
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_X, -xLimit, xLimit, 0.0f);
    yLimit = 200;
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Y, -yLimit, yLimit, 0.0f);
    zLimit = 20;
    stdControl_RegisterAxis(STDCONTROL_AID_MOUSE_Z, -zLimit, zLimit, 0.0f);
    return;

error:
#line 1278
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s error Acquiring Mouse.\n", stdControl_DIGetStatus(hres), 0, 0, 0);
    if ( stdControl_mouse.pDIDevice )
    {
        IDirectInputDevice_Release(stdControl_mouse.pDIDevice);
    }

    stdControl_mouse.pDIDevice = NULL;
}

void J3DAPI stdControl_EnableAxisRead(size_t axis)
{
    if ( STDCONTROL_ISMOUSEAXIS(axis) )
    {
        stdControl_bReadMouse = 1;
    }
    else if ( STDCONTROL_ISJOYSTICKAXIS(axis) )
    {
        stdControl_bReadJoysticks = 1;
    }
}

void J3DAPI stdControl_ReadKeyboard(void)
{
    HRESULT hr;
    size_t curkeyId;

    hr = IDirectInputDevice_GetDeviceState(stdControl_keyboard.pDIDevice, STD_ARRAYLEN(stdControl_aKeyboardState), stdControl_aKeyboardState);
    switch ( hr )
    {
        case DI_OK:
            for ( curkeyId = 0; curkeyId < STDCONTROL_MAX_KEYBOARD_BUTTONS; ++curkeyId )
            {
                stdControl_UpdateKeyState(curkeyId, stdControl_aKeyboardState[curkeyId] & 0x80, stdControl_curReadTime);
            }

            return;

        default:
#line 1353
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetDeviceState from keyboard returned %s.\n", stdControl_DIGetStatus(hr), 0, 0, 0);

        case DIERR_NOTACQUIRED:
        case DIERR_INPUTLOST:
            break;
    }

    hr = IDirectInputDevice_Acquire(stdControl_keyboard.pDIDevice);
    if ( hr != DI_OK && hr != DIERR_OTHERAPPHASPRIO )
    {
#line 1360
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Acquire keyboard returned %s.\n", stdControl_DIGetStatus(hr), 0, 0, 0);
    }
}

void J3DAPI stdControl_ReadJoysticks(void)
{
    size_t joyNum;
    size_t i;
    HRESULT hResult;
    int kid;
    int bCentred;
    DWORD pov;
    DIJOYSTATE jstate;

    for ( joyNum = 0; joyNum < stdControl_numJoystickDevices; joyNum++ )
    {
        kid = STDCONTROL_JOYSTICK_GETBUTTON(joyNum, 0);
        hResult = IDirectInputDevice2_Poll(stdControl_aJoystickDevices[joyNum].pDIDevice);
        if ( FAILED(hResult) )
        {
#line 1420
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s error Poll Joystick.\n", stdControl_DIGetStatus(hResult), 0, 0, 0);
            hResult = IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            return;
        }

        hResult = IDirectInputDevice2_GetDeviceState(stdControl_aJoystickDevices[joyNum].pDIDevice, sizeof(DIJOYSTATE), &jstate);
        if ( FAILED(hResult) )
        {
#line 1426
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "%s error GetDeviceState Joystick.\n", stdControl_DIGetStatus(hResult), 0, 0, 0);
            hResult = IDirectInputDevice2_Acquire(stdControl_aJoystickDevices[joyNum].pDIDevice);
            return;
        }

        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)]  = jstate.lX;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)]  = jstate.lY;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)]  = jstate.lZ;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)] = jstate.lRx;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)] = jstate.lRy;
        stdControl_aAxisStates[STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)] = jstate.lRz;

        for ( i = 0; i < STDCONTROL_NUM_JOYSTICK_BUTTONS; ++i )
        {
            stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETBUTTON(joyNum, i), jstate.rgbButtons[i], stdControl_curReadTime);
        }

        for ( i = 0; i < stdControl_aJoystickDevices[joyNum].caps.dwPOVs && i < STDCONTROL_MAX_JOYSTICK_POVCONTROLERS; ++i )
        {
            pov      = jstate.rgdwPOV[i];
            bCentred = LOWORD(pov) == 0xFFFF;

            stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVLEFT(joyNum, i), pov >= 225 * DI_DEGREES && pov <= 315 * DI_DEGREES, stdControl_curReadTime);
            stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVUP(joyNum, i), (pov >= 315 * DI_DEGREES || pov <= 45 * DI_DEGREES) && !bCentred, stdControl_curReadTime);
            stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVRIGHT(joyNum, i), pov >= 45 * DI_DEGREES && pov <= 135 * DI_DEGREES, stdControl_curReadTime);
            stdControl_UpdateKeyState(STDCONTROL_JOYSTICK_GETPOVDOWN(joyNum, i), pov >= 135 * DI_DEGREES && pov <= 225 * DI_DEGREES, stdControl_curReadTime);
        }
    }
}

void J3DAPI stdControl_ReadMouse(void)
{
    DWORD bufferSize = STDCONTROL_MOUSE_BUFFERSIZE;
    DIMOUSESTATE mouseState;
    DIDEVICEOBJECTDATA aMouseBuffer[STDCONTROL_MOUSE_BUFFERSIZE];
    HRESULT hr;
    size_t i;
    size_t axis;
    DIDEVICEOBJECTDATA* pData;

    if ( !stdControl_bReadMouse )
    {
        return;
    }

    if ( stdControl_mouse.pDIDevice )
    {
        hr = IDirectInputDevice_GetDeviceState(stdControl_mouse.pDIDevice, sizeof(DIMOUSESTATE), &mouseState);
        switch ( hr )
        {
            case DI_OK:
                if ( stdControl_bMouseSensitivityEnabled )
                {
                    axis = STDCONTROL_AID_MOUSE_X;
                    stdControl_aAxisStates[axis] += mouseState.lX;
                    stdControl_aAxisStates[axis] = STDMATH_CLAMP(stdControl_aAxisStates[axis], stdControl_aAxes[axis].min, stdControl_aAxes[axis].max);

                    axis = STDCONTROL_AID_MOUSE_Y;
                    stdControl_aAxisStates[axis] += mouseState.lY;
                    stdControl_aAxisStates[axis] = STDMATH_CLAMP(stdControl_aAxisStates[axis], stdControl_aAxes[axis].min, stdControl_aAxes[axis].max);

                    axis = STDCONTROL_AID_MOUSE_Z;
                    stdControl_aAxisStates[axis] += mouseState.lZ;
                    stdControl_aAxisStates[axis] = STDMATH_CLAMP(stdControl_aAxisStates[axis], stdControl_aAxes[axis].min, stdControl_aAxes[axis].max);
                }
                else
                {
                    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = mouseState.lX;
                    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = mouseState.lY;
                    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] = mouseState.lZ;
                    if ( stdControl_readDeltaTime < 25 )
                    {
                        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = (mouseState.lX + stdControl_mousePos.x) >> 1;
                        stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = (mouseState.lY + stdControl_mousePos.y) >> 1;
                    }

                    stdControl_mousePos.x = mouseState.lX;
                    stdControl_mousePos.y = mouseState.lY;
                }
                break;

            default:
#line 1523
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetDeviceState(mouse) returned %s.\n", stdControl_DIGetStatus(hr), 0, 0, 0);
                // Fallthrough

            case DIERR_NOTACQUIRED:
            case DIERR_INPUTLOST:
                hr = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
                if ( hr != DI_OK && hr != DIERR_OTHERAPPHASPRIO )
                {
#line 1529
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Acquire mouse returned %s.\n", stdControl_DIGetStatus(hr), 0, 0, 0);
                }
                break;
        }

        hr = IDirectInputDevice_GetDeviceData(stdControl_mouse.pDIDevice, sizeof(DIDEVICEOBJECTDATA), aMouseBuffer, &bufferSize, 0);
        switch ( hr )
        {
            case DI_OK:
                for ( i = 0, pData = aMouseBuffer; i < bufferSize; i++, pData++ )
                {
                    if ( pData->dwOfs < DIMOFS_BUTTON0 || pData->dwOfs > DIMOFS_BUTTON3 )
                    {
                        continue;
                    }

                    stdControl_UpdateKeyState(pData->dwOfs + (STDCONTROL_KID_MOUSE_LBUTTON - DIMOFS_BUTTON0), pData->dwData & 0x80, pData->dwTimeStamp);
                }
                break;

            case DI_BUFFEROVERFLOW:
                for ( i = 0; i < STDCONTROL_MAX_MOUSE_BUTTONS; i++ )
                {
                    stdControl_UpdateKeyState(STDCONTROL_KID_MOUSE_LBUTTON + i, mouseState.rgbButtons[i], stdControl_curReadTime);
                }
                break;

            default:
                hr = IDirectInputDevice_Acquire(stdControl_mouse.pDIDevice);
                if ( hr != DIERR_OTHERAPPHASPRIO )
                {
#line 1558
                    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetDeviceData from mouse returned %s.\n", stdControl_DIGetStatus(hr), 0, 0, 0);
                }
                break;
        }
    }
}

void J3DAPI stdControl_RegisterAxis(size_t axisID, int min, int max, float deadzoneScale)
{
    int center;

#line 1619
    STD_ASSERTREL(max > min);
    STD_ASSERTREL(max-min > 0);
    STD_ASSERTREL(axisID < STDCONTROL_MAX_AXES);

    center = min + (max - min + 1) / 2;

    stdControl_aAxes[axisID].flags |= STDCONTROL_AXIS_REGISTERED;
    stdControl_aAxes[axisID].min    = min;
    stdControl_aAxes[axisID].max    = max;
    stdControl_aAxes[axisID].center = center;

    stdControl_aAxes[axisID].scale = 1.0f / D3DVAL(max - center);

    if ( deadzoneScale != 0.0f )
    {
        stdControl_aAxes[axisID].deadzoneThreshold = (int)D3DMultiply(deadzoneScale, D3DVAL(max - center));
    }
    else
    {
        stdControl_aAxes[axisID].deadzoneThreshold = 0;
    }
}

const char* J3DAPI stdControl_DIGetStatus(int HRESULT)
{
    const char* error = "Unknown Error";
    int i;
    int numStatus = STD_ARRAYLEN(stdControl_aDIStatusTbl);
    for ( i = 0; i < numStatus; i++ )
    {
        if ( stdControl_aDIStatusTbl[i].code == HRESULT )
        {
            error = stdControl_aDIStatusTbl[i].text;
            break;
        }
    }

    return error;
}

BOOL CALLBACK stdControl_EnumDevicesCallback(LPCDIDEVICEINSTANCEA pdidInstance, LPVOID pContext)
{

    switch ( GET_DIDEVICE_TYPE(pdidInstance->dwDevType) )
    {
        case DIDEVTYPE_KEYBOARD:
#line 1703
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Keyboard:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
            break;

        case DIDEVTYPE_MOUSE:
#line 1706
            stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Mouse:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
            break;

        case DIDEVTYPE_JOYSTICK:
            if ( stdControl_numJoystickDevices < STD_ARRAYLEN(stdControl_aJoystickDevices) )
            {
                memcpy(&stdControl_aJoystickDevices[stdControl_numJoystickDevices].dinstance, pdidInstance, sizeof(DIDEVICEINSTANCEA));
                stdControl_numJoystickDevices++;
                switch ( GET_DIDEVICE_SUBTYPE(pdidInstance->dwDevType) )
                {
                    case DIDEVTYPEJOYSTICK_GAMEPAD:
#line 1716
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Gamepad:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;

                    case DIDEVTYPEJOYSTICK_TRADITIONAL:
#line 1719
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Joystick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;

                    case DIDEVTYPEJOYSTICK_FLIGHTSTICK:
#line 1722
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Flightstick:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;

                    case DIDEVTYPEJOYSTICK_RUDDER:
#line 1725
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Rudder:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;

                    case DIDEVTYPEJOYSTICK_WHEEL:
#line 1728
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Wheel:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;

                    case DIDEVTYPEJOYSTICK_HEADTRACKER:
#line 1731
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "HeadTracker:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;

                    default:
#line 1735
                        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Unknown:%s:%s\n", pdidInstance->tszProductName, pdidInstance->tszInstanceName, 0, 0);
                        break;
                }
            }
            break;
    }

    return DIENUM_CONTINUE;
}

void J3DAPI stdControl_ResetMousePos(void)
{
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_X] = 0;
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Y] = 0;
    stdControl_aAxisStates[STDCONTROL_AID_MOUSE_Z] = 0;
}

size_t J3DAPI stdControl_GetMaxJoystickButtons(void)
{
    int index;
    size_t maxButtons = 0;
    for ( index = 0; index < stdControl_numJoystickDevices; index++ )
    {
        if ( stdControl_aJoystickDevices[index].caps.dwButtons > maxButtons )
        {
            maxButtons = stdControl_aJoystickDevices[index].caps.dwButtons;
        }
    }

    return maxButtons;
}

size_t J3DAPI stdControl_GetNumJoysticks(void)
{
    return stdControl_numJoystickDevices;
}

/* GetJoysticDescription/MouseSensitivityEnabled: discarded by the retail linker (no caller in either image),
   restored in full from the debug build (0x004F0A6C, 0x004F0AAC, between GetNumJoysticks and
   EnableMouseSensitivity); the description fills stdControl_aStrBuf, retail's unreferenced 128 bytes. Names
   from the debug build's text / OpenJones3D. */
const char* J3DAPI stdControl_GetJoysticDescription(int joyNum)
{
    stdUtil_Format(stdControl_aStrBuf, sizeof(stdControl_aStrBuf), "%s:%s", stdControl_aJoystickDevices[joyNum].dinstance.tszProductName, stdControl_aJoystickDevices[joyNum].dinstance.tszInstanceName);
    return stdControl_aStrBuf;
}

int J3DAPI stdControl_MouseSensitivityEnabled(void)
{
    return stdControl_bMouseSensitivityEnabled;
}

void J3DAPI stdControl_EnableMouseSensitivity(int bEnable)
{
    stdControl_bMouseSensitivityEnabled = bEnable;
}

void J3DAPI stdControl_ShowMouseCursor(int bShow)
{
    stdControl_cursorDisplayCounter = ShowCursor(bShow);
    if ( stdControl_cursorDisplayCounter % 2 )
    {
        stdControl_cursorDisplayCounter = ShowCursor(bShow == 0);
    }
}

int J3DAPI stdControl_IsGamePad(int joyNum)
{
    return GET_DIDEVICE_SUBTYPE(stdControl_aJoystickDevices[joyNum].dinstance.dwDevType) == DIDEVTYPEJOYSTICK_GAMEPAD;
}
