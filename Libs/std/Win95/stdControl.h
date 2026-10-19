#ifndef STD_STDCONTROL_H
#define STD_STDCONTROL_H
#include <j3dcore/j3d.h>
#include <std/types.h>


J3D_EXTERN_C_START

// Mouse axis control ids
#define STDCONTROL_AID_MOUSE_X    STDCONTROL_MAX_JOYSTICK_AXES // 48
#define STDCONTROL_AID_MOUSE_Y    STDCONTROL_AID_MOUSE_X + 1
#define STDCONTROL_AID_MOUSE_Z    STDCONTROL_AID_MOUSE_X + 2

// Axis aid flags
#define STDCONTROL_AID_LOW_SENSITIVITY 0x20000000
#define STDCONTROL_AID_POSITIVE_AXIS   0x80000000
#define STDCONTROL_AID_NEGATIVE_AXIS   0xC0000000

#define STDCONTROL_GETAID(axis) ((axis) & ~(STDCONTROL_AID_POSITIVE_AXIS | STDCONTROL_AID_NEGATIVE_AXIS | STDCONTROL_AID_LOW_SENSITIVITY))

// Button key ids
#define STDCONTROL_KID_MOUSE_LBUTTON   (STDCONTROL_JOYSTICK_FIRSTKID + STDCONTROL_JOYSTICK_TOTALCIDS ) // 640
#define STDCONTROL_KID_MOUSE_RBUTTON   STDCONTROL_KID_MOUSE_LBUTTON + 1
#define STDCONTROL_KID_MOUSE_MBUTTON   STDCONTROL_KID_MOUSE_LBUTTON + 2
#define STDCONTROL_KID_MOUSE_XBUTTON   STDCONTROL_KID_MOUSE_LBUTTON + 3

// Joystick 
#define STDCONTROL_JOYSTICK_FIRSTCID   STDCONTROL_MAX_KEYBOARD_BUTTONS 

#define STDCONTROL_JOYSTICK_FIRSTKID   STDCONTROL_JOYSTICK_FIRSTCID // First button CID
#define STDCONTROL_JOYSTICK_LASTKID    STDCONTROL_JOYSTICK_GETBUTTON(STDCONTROL_MAX_JOYSTICK_DEVICES - 1, STDCONTROL_NUM_JOYSTICK_BUTTONS -1) // Last button CID

#define STDCONTROL_JOYSTICK_FIRTPOVCID  STDCONTROL_JOYSTICK_FIRSTKID + STDCONTROL_NUM_JOYSTICK_BUTTONS // First POV controller direction CID
#define STDCONTROL_JOYSTICK_LASTPOVCID  STDCONTROL_JOYSTICK_GETPOV(STDCONTROL_MAX_JOYSTICK_DEVICES - 1, STDCONTROL_MAX_JOYSTICK_POVCONTROLERS -1, STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS - 1)

#define STDCONTROL_JOYSTICK_TOTALCIDS   (STDCONTROL_MAX_JOYSTICK_DEVICES * STDCONTROL_TOTAL_JOYSTICK_CONTROLS)

// Joystick button ID or POV controller direction control ID
#define STDCONTROL_JOYSTICK_GETBUTTON(joyNum, btnNum)  ((STDCONTROL_TOTAL_JOYSTICK_CONTROLS) * (joyNum) + (btnNum) + STDCONTROL_JOYSTICK_FIRSTKID)
#define STDCONTROL_JOYSTICK_GETBUTTONINDEX(cid)        (((cid) - STDCONTROL_JOYSTICK_FIRSTKID) % STDCONTROL_TOTAL_JOYSTICK_CONTROLS ) // Get button index number from CID

#define STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, dirNum) ((STDCONTROL_TOTAL_JOYSTICK_CONTROLS) * (joyNum) + ((povNum) * STDCONTROL_MAX_JOYSTICK_POVCONTROLERS) + (STDCONTROL_JOYSTICK_FIRTPOVCID + (dirNum)))
#define STDCONTROL_JOYSTICK_GETPOVINDEX(cid)               (((cid) - STDCONTROL_JOYSTICK_FIRSTCID) % STDCONTROL_TOTAL_JOYSTICK_CONTROLS % STDCONTROL_NUM_JOYSTICK_BUTTONS / STDCONTROL_MAX_JOYSTICK_POVCONTROLERS) // Get POV controller index number from CID, i.e. control num
#define STDCONTROL_JOYSTICK_GETPOVDIRECTIONINDEX(cid)      (((cid) - STDCONTROL_JOYSTICK_FIRSTCID) % STDCONTROL_NUM_JOYSTICK_BUTTONS % STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS) // Get POV direction index number from CID

// Joystick macro for getting control ID (CID) for POV direction button 
#define STDCONTROL_JOYSTICK_GETPOVLEFT(joyNum, povNum)  STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRLEFT)
#define STDCONTROL_JOYSTICK_GETPOVUP(joyNum, povNum)    STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRUP)
#define STDCONTROL_JOYSTICK_GETPOVRIGHT(joyNum, povNum) STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRRIGHT)
#define STDCONTROL_JOYSTICK_GETPOVDOWN(joyNum, povNum)  STDCONTROL_JOYSTICK_GETPOV(joyNum, povNum, STDCONTROL_JOYSTICK_POVDIRDOWN)

// Joystick POV direction indices
#define STDCONTROL_JOYSTICK_POVDIRLEFT  0
#define STDCONTROL_JOYSTICK_POVDIRUP    1
#define STDCONTROL_JOYSTICK_POVDIRRIGHT 2
#define STDCONTROL_JOYSTICK_POVDIRDOWN  3

// Joystick/controller axes related macros
#define STDCONTROL_JOYSTICK_NUMPOSAXES  3u // number of positional axes
#define STDCONTROL_JOYSTICK_NUMROTAXES  3u // number of rotation axes
#define STDCONTROL_JOYSTICK_NUMAXES     6 // STDCONTROL_JOYSTICK_NUMPOSAXES + STDCONTROL_JOYSTICK_NUMROTAXES; retail assertion text spells the literal 6

// Joystick positional & rotation axis indices
// Note, in directX 6 thes constants are defined in dinput.h as
// DIJOFS_X, DIJOFS_Y, DIJOFS_Z, DIJOFS_RX, DIJOFS_RY, DIJOFS_RZ
#define STDCONTROL_JOYSTICK_AXIS_X   0u
#define STDCONTROL_JOYSTICK_AXIS_Y   1u
#define STDCONTROL_JOYSTICK_AXIS_Z   2u
#define STDCONTROL_JOYSTICK_AXIS_RX  3u
#define STDCONTROL_JOYSTICK_AXIS_RY  4u
#define STDCONTROL_JOYSTICK_AXIS_RZ  5u

#define STDCONTROL_GET_JOYSTICK_AXIS(joyNum, axisIdx) (STDCONTROL_JOYSTICK_NUMAXES * (joyNum) + (axisIdx))
#define STDCONTROL_GET_JOYSTICK_AXIS_X(joyNum)        STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_X)
#define STDCONTROL_GET_JOYSTICK_AXIS_Y(joyNum)        STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_Y)
#define STDCONTROL_GET_JOYSTICK_AXIS_Z(joyNum)        STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_Z)
#define STDCONTROL_GET_JOYSTICK_AXIS_RX(joyNum)       STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RX)
#define STDCONTROL_GET_JOYSTICK_AXIS_RY(joyNum)       STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RY)
#define STDCONTROL_GET_JOYSTICK_AXIS_RZ(joyNum)       STDCONTROL_GET_JOYSTICK_AXIS(joyNum, STDCONTROL_JOYSTICK_AXIS_RZ)

#define STDCONTROL_MAX_JOYSTICK_DEVICES       8
#define STDCONTROL_MAX_JOYSTICK_POVCONTROLERS 4  // max number of point-of-view controllers e.g.: Hat switch
#define STDCONTROL_MAX_JOYSTICK_AXES          (STDCONTROL_MAX_JOYSTICK_DEVICES * STDCONTROL_JOYSTICK_NUMAXES) // 48, total number of axes from all devices

// Miscellaneous constants
#define STDCONTROL_MAX_AXES                   (STDCONTROL_MAX_JOYSTICK_AXES + 3) // 51; 3 - mouse axes 
#define STDCONTROL_MAX_KEYBOARD_BUTTONS       256 // aka max keyboard kids
#define STDCONTROL_MAX_MOUSE_BUTTONS            4
#define STDCONTROL_NUM_JOYSTICK_BUTTONS        32 // number of joy buttons per device
#define STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS   4 // number of directions per point-of-view controller
#define STDCONTROL_TOTAL_JOYSTICK_CONTROLS     (STDCONTROL_NUM_JOYSTICK_BUTTONS + (STDCONTROL_NUM_JOYSTICK_POVDIRECTIONS * STDCONTROL_MAX_JOYSTICK_POVCONTROLERS))// max number of joy buttons + POV controllers directions per device
#define STDCONTROL_MAX_KEYID                   (STDCONTROL_MAX_KEYBOARD_BUTTONS + STDCONTROL_JOYSTICK_TOTALCIDS + STDCONTROL_MAX_MOUSE_BUTTONS) // 644

// Helper macros to test if specific keyid or aid is either button or axis of specific device

#define STDCONTROL_ISKEYBOARDBUTTON(kid) ((kid) < STDCONTROL_MAX_KEYBOARD_BUTTONS)

#define STDCONTROL_ISMOUSEAXIS(aid) ((aid) >= STDCONTROL_AID_MOUSE_X && (aid) < STDCONTROL_MAX_AXES)
#define STDCONTROL_ISMOUSEBUTTON(kid) ((kid) >= STDCONTROL_KID_MOUSE_LBUTTON && (kid) < STDCONTROL_MAX_KEYID)

// Check for joystick controller ids
#define STDCONTROL_ISJOYSTICKAXIS(aid) ((aid) >= 0 && (aid) < STDCONTROL_AID_MOUSE_X)
#define STDCONTROL_ISJOYSTICKBUTTON(kid) ((kid) >= STDCONTROL_JOYSTICK_FIRSTKID && (kid) <= STDCONTROL_JOYSTICK_LASTPOVCID)
#define STDCONTROL_ISJOYSTICKPOV(cid) ((STDCONTROL_GETAID(cid) >= STDCONTROL_JOYSTICK_FIRTPOVCID) && (STDCONTROL_GETAID(cid) <= STDCONTROL_JOYSTICK_LASTPOVCID))


extern StdControlAxis stdControl_aAxes[STDCONTROL_MAX_AXES];

extern int stdControl_bOpen;

extern int stdControl_bControlsIdle;

extern int stdControl_bControlsActive;

int J3DAPI stdControl_Startup(int bKeyboardForeground);
void J3DAPI stdControl_Shutdown(void);

int J3DAPI stdControl_Open(void);
void J3DAPI stdControl_Close(void);
void J3DAPI stdControl_Reset(void);

void J3DAPI stdControl_DisableReadJoysticks(void);
int J3DAPI stdControl_EnableAxis(size_t axis);

void J3DAPI stdControl_ReadControls(void);
void J3DAPI stdControl_FinishRead(void);

float J3DAPI stdControl_ReadAxis(size_t axis);
int J3DAPI stdControl_ReadAxisRaw(size_t axis);
float J3DAPI stdControl_ReadKeyAsAxis(size_t keyId);

/**
 * @brief Emulates a key press from a joystick/gamepad axis.
 *
 * Returns non-zero if the axis is "pressed". If pbPressed is not NULL,
 * it is set to non-zero if the axis was just pressed - always true if non-zero value is returned.
 *
 * @param axis - The axis index.
 * @param pbPressed - Pointer to an int for the "just pressed" state.
 * @return A non-zero value if the key is currently pressed down; zero otherwise.
 */
int J3DAPI stdControl_ReadAxisAsKey(size_t axis, int* pbPressed);

/**
 * @brief Emulates a key press from a joystick/gamepad axis with a threshold.
 *
 * Returns non-zero if the axis value exceeds the specified threshold. If pbPressed is not NULL,
 * it is set to non-zero if the axis was just pressed - always true if non-zero value is returned.
 *
 * @param axis - The axis index.
 * @param pbPressed - Pointer to an int for the "just pressed" state.
 * @param lowValue - The threshold value for detecting a press.
 * @return A non-zero value if the key is currently pressed down; zero otherwise.
 */
int J3DAPI stdControl_ReadAxisAsKeyEx(size_t axis, int* pbPressed, float lowValue);

/**
 * @brief Reads the current and just pressed state of a specified key.
 *
 * Returns non-zero if the key is currently pressed. If pbPressed is not NULL,
 * it is set to non-zero if the key was just pressed.
 *
 * @param keyNum - The key identifier.
 * @param pbPressed - Pointer to an int for the "just pressed" state.
 *
 * @return A non-zero value if the key is currently pressed down; zero otherwise.
 */
int J3DAPI stdControl_ReadKey(size_t keyNum, int* pbPressed);

int J3DAPI stdControl_ControlsActive(void);
int J3DAPI stdControl_SetActivation(int bActive); // returns 0 if mouse control was successfully processed 

int J3DAPI stdControl_ToggleMouse(void);
int J3DAPI stdControl_EnableMouse(int bEnable);

void J3DAPI stdControl_ResetMousePos(void);
void J3DAPI stdControl_EnableMouseSensitivity(int bEnable);
void J3DAPI stdControl_SetMouseSensitivity(float xSensitivity, float ySensitivity);
void J3DAPI stdControl_ShowMouseCursor(int bShow);

int J3DAPI stdControl_ControlsIdle(void);

int J3DAPI stdControl_TestAxisFlag(size_t axis, StdControlAxisFlag flags);
void J3DAPI stdControl_SetAxisFlags(size_t axis, int flags);

void J3DAPI stdControl_RegisterAxis(size_t axisID, int min, int max, float deadzoneScale);
size_t J3DAPI stdControl_GetMaxJoystickButtons(void);
size_t J3DAPI stdControl_GetNumJoysticks(void);

int J3DAPI stdControl_IsGamePad(int joyNum);


J3D_EXTERN_C_END
#endif // STD_STDCONTROL_H
