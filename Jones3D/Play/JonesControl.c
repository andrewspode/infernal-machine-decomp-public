#include "JonesControl.h"

#include <Jones3D/Display/JonesConsole.h>
#include <Jones3D/Main/JonesMain.h>

#include <sith/Devices/sithControl.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/World/sithWeapon.h>

#include <std/Win95/stdControl.h>
#include <w32util/wuRegistry.h>

static int JonesControl_bStartup = 0; // Added: Init to false

int J3DAPI JonesControl_Startup(void)
{
    int bJoystickControl;

    int bMouseControl = wuRegistry_GetIntEx("Mouse Control", 0);
    JonesDisplaySettings* pSettings = JonesMain_GetDisplaySettings();

    if ( JonesControl_bStartup )
    {
        return 1;
    }

    if ( sithControl_Startup(0) )
    {
        return 1;
    }

    // Note, instead of manually initializing the sithControl_DefaultInit could be called 
    // and unbind mouse controller afterwards
    sithControl_Reset();
    sithControl_controlOptions = 0x04;
    sithControl_RegisterKeyboardBindings();
    sithControl_RegisterJoystickBindings();
    // Note, sithControl_RegisterMouseBindings() is not called probably because jonesconfig would unbind it anyway

    if ( !pSettings->bWindowMode || (pSettings->bWindowMode && bMouseControl) )
    {
        stdControl_EnableMouse(1);
    }

    sithControl_RegisterControlCallback(sithPlayerControls_Process);
    sithControl_RegisterControlCallback(sithWeapon_ProcessWeaponControls);
    sithControl_RegisterControlCallback(JonesControl_ProcessControls);

    {
        int functionId = 0;  // block scope: the debug build's frame slot order follows it
        for ( ; functionId < SITHCONTROL_MAXFUNCTIONS; ++functionId )
    {
        sithControl_RegisterAxisFunction((SithControlFunction)functionId, SITHCONTROLFUNCTION_KEY);
    }
    }

    bJoystickControl = wuRegistry_GetIntEx("Joystick Control", 0);
    if ( bJoystickControl == 1 && stdControl_GetNumJoysticks() > 0 )
    {
        JonesControl_EnableJoystickAxes();
        sithControl_UnbindJoystickAxes();
    }

    wuRegistry_SaveIntEx("Joystick Control", bJoystickControl);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 && (!pSettings->bWindowMode || bMouseControl) )
    {
        sithControl_UnbindMouseAxes();
    }

    JonesControl_bStartup = 1;
    return 0;
}

void J3DAPI JonesControl_Shutdown(void)
{
    sithControl_Shutdown();
    JonesControl_bStartup = 0;
}

/* Linker-discarded (no caller in either build): debug 0x004135C6, an empty function between Shutdown and ProcessControls
   (a ResetGlobals-style function). */
void J3DAPI JonesControl_sub_4135C6(void)
{
}

int J3DAPI JonesControl_ProcessControls(SithThing* pPlayer, float secDeltaTime)
{
    int numPressed;


    if ( (pPlayer->thingInfo.actorInfo.flags & SITH_AF_CONTROLSDISABLED) != 0 )
    {
        return 0;
    }

    if ( !jonesConsole_bVisible )
    {
        sithControl_GetKey(SITHCONTROL_TALK, &numPressed); // F10 pressed
        if ( numPressed )
        {
            JonesConsole_ShowConsole();
        }
    }

    return 0;
}

void J3DAPI JonesControl_EnableJoystickAxes(void)
{
    int numJoysticks;
    int i;

    numJoysticks = stdControl_GetNumJoysticks();
    for ( i = 0; i < numJoysticks; ++i )
    {
        stdControl_EnableAxis(i * 6);
        stdControl_EnableAxis((i * 6) + 1);
        stdControl_EnableAxis((i * 6) + 2);
        stdControl_EnableAxis((i * 6) + 5);
    }
}
