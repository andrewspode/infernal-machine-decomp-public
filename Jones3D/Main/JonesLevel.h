#ifndef JONESLEVEL_H
#define JONESLEVEL_H
#include <Libs/j3dcore/j3d.h>
#include <Jones3D/types.h>


J3D_EXTERN_C_START

#define JONESLEVEL_NUMLEVELS       18
#define JONESLEVEL_GENERICLEVELNUM  0
#define JONESLEVEL_FIRSTLEVELNUM    1
#define JONESLEVEL_LASTLEVELNUM    16
#define JONESLEVEL_BONUSLEVELNUM   17

extern const JonesLevelInfo JonesMain_aCndLevelLoadInfos[JONESLEVEL_NUMLEVELS];
extern const JonesLevelInfo JonesMain_aNdyLevelLoadInfos[JONESLEVEL_NUMLEVELS];

J3D_EXTERN_C_END
#endif // JONESLEVEL_H