#ifndef RDROID_RDROID_H
#define RDROID_RDROID_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <std/General/std.h>

J3D_EXTERN_C_START

#define RD_REF_WIDTH       640.0f
#define RD_REF_HEIGHT      480.0f
#define RD_REF_APECTRATIO (RD_REF_WIDTH / RD_REF_HEIGHT) // Original aspect ratio (4:3)

// Guessed macro: Older GPUs, particularly those in the DirectX 6 era, often used fixed - point math instead of floating - point for performance reasons.
// The rhw value was frequently packed into a 16 - bit or 32 - bit fixed - point format, and 32767.5f is the middle value for a signed 16 - bit integer range(-32768 to 32767).
// Using 1.0f / 32767.5f normalizes the W component, keeping it within a more precise range for the hardware.
#define RD_FIXEDPOINT_RHW_SCALE_BASE (1.0f / 32767.5f) // 0.000030518044f

// Predefined RHW scale factors.
// Higher multipliers yield a larger RHW value, which moves vertices further from the camera.
// These are useful for quickly selecting standard scaling values.
#define RD_FIXEDPOINT_RHW_SCALE_X1   RD_FIXEDPOINT_RHW_SCALE(1) // same as RD_FIXEDPOINT_RHW_SCALE_BASE
#define RD_FIXEDPOINT_RHW_SCALE_X2   RD_FIXEDPOINT_RHW_SCALE(2) // 0.000061036088
#define RD_FIXEDPOINT_RHW_SCALE_X3   RD_FIXEDPOINT_RHW_SCALE(3) // 0.000091554131
#define RD_FIXEDPOINT_RHW_SCALE_X4   RD_FIXEDPOINT_RHW_SCALE(4) // 0.00012207218
#define RD_FIXEDPOINT_RHW_SCALE_X5   RD_FIXEDPOINT_RHW_SCALE(5) // 0.00015259022

// Macro to compute an RHW scale factor for an arbitrary multiplier.
// Use this macro when you need a non-standard scale factor instead of the predefined ones.
#define RD_FIXEDPOINT_RHW_SCALE(multiplier) ((multiplier) * RD_FIXEDPOINT_RHW_SCALE_BASE)

/*
 * rdroid's scalar product: the D3D SDK value macro (d3dtypes.h, via rdroid/types.h)
 * wrapped once more, expanding to `(((a) * (b)))`. Reconstructed (name and body
 * inferred): no original header text survives, but
 * retail's x87 shapes need exactly this depth - one wrap more than
 * D3DMultiply(a, b) - at every product of rdClip.c's clippers, rdQClip.c,
 * rdMatrix.c and rdVector.h's product macros, and no SDK macro gives it.
 *
 */
#define RD_MULTIPLY(a, b) (D3DMultiply(a, b))



#define RDLOG_FATAL(message) \
    J3DLOG_FATAL(rdroid_pHS, message)

#define RD_ASSERT(condition) \
    J3D_ASSERTREL(condition, rdroid_pHS )

#define RD_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, rdroid_pHS )

extern rdLightMode rdroid_curLightingMode;

extern tHostServices *rdroid_pHS;

extern rdGeometryMode rdroid_curGeometryMode;

extern rdRoidFlags rdroid_curRenderOptions;

void J3DAPI rdStartup(void);
void J3DAPI rdShutdown(void);

int J3DAPI rdOpen(void);
void J3DAPI rdClose(void);

void J3DAPI rdSetServices(tHostServices* pHS);
void J3DAPI rdClearServices(void);

rdRoidFlags J3DAPI rdGetRenterOptions(void);
void J3DAPI rdSetRenderOptions(rdRoidFlags options);

void J3DAPI rdSetGeometryMode(rdGeometryMode mode);

void J3DAPI rdSetLightingMode(rdLightMode mode);

J3D_EXTERN_C_END
#endif // RDROID_RDROID_H
