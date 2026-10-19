#ifndef SMUSHPLAY_H
#define SMUSHPLAY_H

#include <j3dcore/j3d.h>
#include <std/types.h>

J3D_EXTERN_C_START

/*
 * SmushPlay: the retail SMUSH movie player at 0x004E30B0-0x004E33F0 (Libs/smush/SmushPlay.c,
 * whose own prototypes use the library's plain types). The game's entry points are declared
 * here with the game's types, as JonesMain calls them. Declaring them as real prototypes
 * (rather than far-address trampolines) is what lets VC5 emit retail's direct
 * `call` instead of `mov reg, imm32; call reg`.
 */

typedef struct sSmushColorFormat
{
    int redBPP;
    int greenBPP;
    int blueBPP;
    int redPosShift;
    int greenPosShift;
    int bluePosShift;
    int redPosShiftRight;
    int greenPosShiftRight;
    int bluePosShiftRight;
    int alphaBPP;
    int alphaPosShift;
    int alphaPosShiftRight;
} SmushColorFormat;

typedef struct sSmushBitmap
{
    void* pPixels;
    uint32_t width;
    uint32_t height;
    int pixelSize;
    int bpp;
    size_t pitch;
    int unknown6;
    int unknown7;
    tColorMode colorMode;
    SmushColorFormat format;
} SmushBitmap;


int SmushPlay_SysStartup(void* hwnd, void* pDSound);
void SmushPlay_SysShutdown(void);
void SmushPlay_SetGlobalVolume(int volume);
int SmushPlay_PlayMovie(const char* pFilename, int a1, int a3, int a4, int a5, int width, int height, int a8, int a9, int(__cdecl* pBlit)(const SmushBitmap* pBitmap, int a2), int a11, int a12, int a13);

J3D_EXTERN_C_END


#endif // SMUSHPLAY_H