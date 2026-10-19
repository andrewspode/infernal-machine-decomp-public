#include "sithTime.h"

#include <std/General/stdMath.h>
#include <std/General/stdPlatform.h>

#define SITHTIME_MAXFRAMETIME 200 // 200ms

uint32_t sithTime_frameTime = 0;
float sithTime_frameTimeFlex = 0;
float sithTime_fps = 0;
uint32_t sithTime_msecGameTime = 0;
float sithTime_secGameTime = 0;
uint32_t sithTime_clockTime = 0;
int sithTime_bPaused = 0;
static uint32_t sithTime_msecPauseStartTime = 0;

void J3DAPI sithTime_Advance(void)
{
    uint32_t curTime;

    if ( !sithTime_clockTime )
    {
        return;
    }

    if ( sithTime_bPaused )
    {
        return;
    }

    curTime = stdPlatform_GetTimeMsec();
    if ( curTime < sithTime_clockTime )
    {
#line 116
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Clock looped around.  Handling it.\n", 0, 0, 0, 0);
        sithTime_frameTime = (uint32_t)-1 - sithTime_clockTime + curTime;
    }
    else
    {
        sithTime_frameTime = curTime - sithTime_clockTime;
    }

    sithTime_frameTime = STDMATH_CLAMP(sithTime_frameTime, 1, SITHTIME_MAXFRAMETIME);

    sithTime_clockTime = curTime;

    if ( (sith_mode.debugModeFlags & SITHDEBUG_SLOWMODE) != 0 )
    {
        sithTime_frameTime = lrintf((float)sithTime_frameTime * 0.2f); // frameTime / 5.0f
    }

    sithTime_frameTimeFlex = (float)sithTime_frameTime * 0.001f;
    if ( sithTime_frameTimeFlex <= 0.0f )
    {
        sithTime_frameTimeFlex = 1.0f / 10000.0f; // 0.000099999997f
    }

    sithTime_fps           = 1.0f / sithTime_frameTimeFlex;
    sithTime_msecGameTime += sithTime_frameTime;
    sithTime_secGameTime   = (float)sithTime_msecGameTime * 0.001f;
}

void J3DAPI sithTime_Pause(void)
{
    if ( !sithTime_bPaused )
    {
        sithTime_msecPauseStartTime = stdPlatform_GetTimeMsec();
        sithTime_bPaused = 1;
    }
}

void J3DAPI sithTime_Resume(void)
{
    if ( sithTime_bPaused )
    {
        sithTime_clockTime += stdPlatform_GetTimeMsec() - sithTime_msecPauseStartTime;
        sithTime_bPaused = 0;
    }
}

/* Discarded by the retail linker (no caller in either image); restored in full from the debug build
   (0x004570CB, between Resume and Reset; assert lines 224/237). Jedi Knight's sithTime_SetDelta; name
   from OpenJones3D develop. */
void J3DAPI sithTime_SetFrameTime(uint32_t frameTime)
{
#line 224
    SITH_ASSERTREL(sithTime_clockTime != 0);

    sithTime_frameTime = frameTime;
    sithTime_clockTime = stdPlatform_GetTimeMsec();
    sithTime_frameTime = STDMATH_CLAMP(sithTime_frameTime, 10, SITHTIME_MAXFRAMETIME);

    sithTime_frameTimeFlex = (float)sithTime_frameTime * 0.001f;
#line 237
    SITH_ASSERTREL(sithTime_frameTimeFlex > 0.0f);

    sithTime_fps           = 1.0f / sithTime_frameTimeFlex;
    sithTime_msecGameTime += sithTime_frameTime;
    sithTime_secGameTime   = (float)sithTime_msecGameTime * 0.001f;
}

void J3DAPI sithTime_Reset(void)
{
    sithTime_msecGameTime  = 0;
    sithTime_secGameTime   = 0.0f;
    sithTime_frameTime     = 0;
    sithTime_frameTimeFlex = 0.0f;
    sithTime_fps           = 0.0f;
    sithTime_clockTime     = stdPlatform_GetTimeMsec();
}

void J3DAPI sithTime_SetGameTime(uint32_t msecTime)
{
    sithTime_msecGameTime  = msecTime;
    sithTime_secGameTime   = (float)sithTime_msecGameTime * 0.001f;
    sithTime_frameTime     = 0;
    sithTime_frameTimeFlex = 0.0f;
    sithTime_fps           = 0.0f;
    sithTime_clockTime     = stdPlatform_GetTimeMsec();
}

int J3DAPI sithTime_IsPaused(void)
{
    return sithTime_bPaused;
}
