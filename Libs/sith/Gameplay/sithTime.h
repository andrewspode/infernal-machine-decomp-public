#ifndef SITH_SITHTIME_H
#define SITH_SITHTIME_H
#include <j3dcore/j3d.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

extern uint32_t sithTime_frameTime;

extern float sithTime_frameTimeFlex;

extern float sithTime_fps;

extern uint32_t sithTime_msecGameTime;

extern float sithTime_secGameTime;

extern uint32_t sithTime_clockTime;

extern int sithTime_bPaused;

void J3DAPI sithTime_Advance(void);
void J3DAPI sithTime_Pause(void);
void J3DAPI sithTime_Resume(void);
void J3DAPI sithTime_Reset(void);
void J3DAPI sithTime_SetGameTime(uint32_t msecTime);
int J3DAPI sithTime_IsPaused(void);

J3D_EXTERN_C_END
#endif // SITH_SITHTIME_H
