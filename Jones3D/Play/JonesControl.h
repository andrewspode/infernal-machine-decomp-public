#ifndef JONES3D_JONESCONTROL_H
#define JONES3D_JONESCONTROL_H
#include <Jones3D/types.h>
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>

J3D_EXTERN_C_START

int J3DAPI JonesControl_Startup(void);
void J3DAPI JonesControl_Shutdown(void);
int J3DAPI JonesControl_ProcessControls(SithThing* pPlayer, float secDeltaTime);
void J3DAPI JonesControl_EnableJoystickAxes(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESCONTROL_H
