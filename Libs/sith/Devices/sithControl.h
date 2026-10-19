#ifndef SITH_SITHCONTROL_H
#define SITH_SITHCONTROL_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>
#include <std/types.h>

J3D_EXTERN_C_START

enum { SITHCONTROL_MAXFUNCTIONS = 58 };

#define STDCONTROL_IDLECAM_TIMEOUT 90000 // 90 sec, time in milliseconds before switching to idle camera due to control inactivity

extern int sithControl_controlOptions;

int J3DAPI sithControl_Startup(int bKeyboardForeground);
void J3DAPI sithControl_Shutdown(void);

int J3DAPI sithControl_Open(void);
int J3DAPI sithControl_Close(void);

void J3DAPI sithControl_RegisterAxisFunction(size_t functionId, SithControlFunctionFlag flag);
void J3DAPI sithControl_RegisterKeyFunction(size_t functionId, SithControlFunctionFlag);
SithControlBinding* J3DAPI sithControl_BindControl(size_t functionId, size_t controlId, SithControlBindFlag flags);
SithControlBinding* J3DAPI sithControl_BindAxis(size_t functionId, size_t controlId, SithControlBindFlag flags);

void J3DAPI sithControl_UnbindFunction(SithControlFunction fctnID);
void J3DAPI sithControl_UnbindFunctionIndex(int funcId, size_t bindIndex);
void J3DAPI sithControl_UnbindControl(SithControlBindFlag flags, size_t controlId);

void J3DAPI sithControl_RegisterControlCallback(SithControlCallback pfCallback);
void J3DAPI sithControl_Update(float secDeltaTime, uint32_t msecDeltaTime);

float J3DAPI sithControl_GetKeyAsAxisNormalized(size_t axisId);
float J3DAPI sithControl_GetKeyAsAxis(size_t axisId);
float J3DAPI sithControl_GetAxis(size_t axisId);
int J3DAPI sithControl_GetKey(size_t keyId, int* pState);

void J3DAPI sithControl_ReadControls(void);
void J3DAPI sithControl_FinishRead(void);

void J3DAPI sithControl_UnbindJoystickAxes(void);

void J3DAPI sithControl_UnbindMouseAxes(void);

void sithControl_Reset(void); // Clears all bindings

void J3DAPI sithControl_RegisterControlFunctions(void);
void J3DAPI sithControl_RegisterKeyboardBindings(void);
void J3DAPI sithControl_RegisterJoystickBindings(void);
void J3DAPI sithControl_RegisterMouseBindings(void);

J3D_EXTERN_C_END
#endif // SITH_SITHCONTROL_H
