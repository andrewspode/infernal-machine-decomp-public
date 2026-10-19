#include "stdEffect.h"

static tStdFadeFactor stdEffect_fadeFactor;

void J3DAPI stdEffect_Startup(void)
{
    stdEffect_Reset();
}

void J3DAPI stdEffect_Shutdown(void)
{
    stdEffect_Reset();
}

void J3DAPI stdEffect_Reset(void)
{
    stdEffect_fadeFactor.bEnabled = 0;
    stdEffect_fadeFactor.factor   = 1.0f;
}

void J3DAPI stdEffect_SetFadeFactor(int bEnabled, float factor)
{
    stdEffect_fadeFactor.bEnabled = bEnabled;
    stdEffect_fadeFactor.factor = factor;
}

const tStdFadeFactor* J3DAPI stdEffect_GetFadeFactor(void)
{
    return &stdEffect_fadeFactor;
}
