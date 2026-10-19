#ifndef STD_STDMATH_H
#define STD_STDMATH_H
#include <j3dcore/j3d.h>

#include <math.h>

J3D_EXTERN_C_START

// Clamp value between min and max
// Retail tests the in-range conditions (>= min, <= max); VC5 emits each true
// arm first, which gives retail's branch layout.
#define STDMATH_RAD2DEG(x) ((x) * 57.295784f)
// The in-range arm's `val` is bare: the debug build (/Od) copies a member value there with
// integer moves, where a parenthesised float member read loads through the FPU
// (rdPrimit3_ClipFace, debug 0x515a4e).
#define STDMATH_CLAMP(val, minv, maxv) ((val) >= (minv) ? ((val) <= (maxv) ? val : maxv) : minv)

// Convert radians to degrees
#define STDMATH_PI 3.14159265358979323846
#define STDMATH_TODEGREES(rad)  ((rad) * (180.0 / STDMATH_PI))
#define STDMATH_TODEGREESF(rad) (float)STDMATH_TODEGREES(rad)

// Convert degrees to radians
#define STDMATH_RADIANS(deg)  ((deg) * (STDMATH_PI / 180.0))
#define STDMATH_RADIANSF(deg) (float)STDMATH_RADIANS(deg)

float J3DAPI stdMath_FlexPower(float base, int exponent);

float J3DAPI stdMath_NormalizeAngle(float angle);
float J3DAPI stdMath_NormalizeAngleAcute(float angle);
float J3DAPI stdMath_NormalizeDeltaAngle(float angle1, float angle2);

// Calculate sinus and cousins of value angle
void J3DAPI stdMath_SinCos(float angle, float* pSinOut, float* pCosOut);
float J3DAPI stdMath_Tan(float angle);
float J3DAPI stdMath_ArcSin1(float angle);
float J3DAPI stdMath_ArcSin2(float num);
float J3DAPI stdMath_ArcSin3(float angle);
float J3DAPI stdMath_ArcTan1(float x, float y);
float J3DAPI stdMath_ArcTan2(float x, float y);
float J3DAPI stdMath_ArcTan3(float x, float y);

// Returns value of the arc tangent
float J3DAPI stdMath_ArcTan4(float x, float y);
float J3DAPI stdMath_Dist2D1(float x, float y);
float J3DAPI stdMath_Dist2D2(float x, float y);
float J3DAPI stdMath_Dist2D3(float x, float y);
float J3DAPI stdMath_Dist2D4(float x, float y);
float J3DAPI stdMath_Dist3D1(float x, float y, float z);
float J3DAPI stdMath_Dist3D2(float x, float y, float z);
float J3DAPI stdMath_Dist3D3(float x, float y, float z);

/*
 * Single-precision x87 helpers (B-ASM-SQRT, B-ASM-FPUCW).
 *
 * The project otherwise avoids inline assembly; these
 * helpers are the exception because they are retail's own text.
 * Retail's math header defined these helpers as
 * `__inline` functions with hand-written x87 bodies. VC5 expands them at every
 * optimized call site (the `fstp [x]; fld [x]; fsqrt; fstp [r]; fld [r]`
 * bounce in an EBP frame). No C form reproduces any of the shapes. Two evidence bases:
 *
 * floorf and sqrtf also have out-of-line copies called by the unoptimized
 * stdMath.c (0x004C1340, 0x004C1AF0); ceilf and lrintf have none, and are
 * proven by byte-identical inline expansions at 7 and 5 independent sites
 * while retail still uses __ftol elsewhere in the same TUs. The names and
 * exact statement split are inferred from retail bytes
 *
 *
 * Define NONMATCHING_MATH to compile behaviour-equivalent C bodies instead.
 */
#ifndef NONMATCHING_MATH
extern int16_t word_183962C; // saved x87 control word (stdMath.c; set by stdStartup)
extern int16_t word_183962E; // round-down control word (stdMath.c; set by stdStartup)
extern int16_t word_1839630; // round-up control word (stdMath.c; set by stdStartup)
#endif

__inline float __cdecl stdMath_floorf(float value)
{
    float r;
#ifdef NONMATCHING_MATH
    r = (float)floor(value);
#else
    __asm fldcw word ptr [word_183962E]
    __asm fld value
    __asm frndint
    __asm fstp r
    __asm fldcw word ptr [word_183962C]
#endif
    return r;
}

__inline float __cdecl stdMath_ceilf(float value)
{
    float r;
#ifdef NONMATCHING_MATH
    r = (float)ceil(value);
#else
    __asm fldcw word ptr [word_1839630]
    __asm fld value
    __asm frndint
    __asm fstp r
    __asm fldcw word ptr [word_183962C]
#endif
    return r;
}

// Float to int with round-down (floor) semantics, not a truncating cast or
// C99 lrintf's current rounding mode; the name is inferred.
__inline long __cdecl stdMath_lrintf(float value)
{
    long r;
#ifdef NONMATCHING_MATH
    r = (long)floor(value);
#else
    __asm fldcw word ptr [word_183962E]
    __asm fld value
    __asm fistp r
    __asm fldcw word ptr [word_183962C]
#endif
    return r;
}

// Under the Intel compiler (rdModel3K.c) the body is C: that unit calls this helper (the debug build's call at
// 0x00535E7F reaches the out-of-line copy) and retail inlines it as a plain `fsqrt` in the x87 stack.
__inline float __cdecl stdMath_sqrtf(float value)
{
    float r;
#if defined(NONMATCHING_MATH) || defined(__ICL)
    r = (float)sqrt(value);
#else
    __asm fld value
    __asm fsqrt
    __asm fstp r
#endif
    return r;
}

/*
 * Callers reach all four helpers through these macros: every call site's value
 * is one parenthesised node, which keeps retail's copy of the helper's `r` into
 * the assigned variable (`fld [r]; fst [v]`) instead of VC5 coalescing the two.
 * The form is uniform over all sites (with it
 * rdPuppet_AdvanceTrack reproduces retail and no other site
 * changes). The macro
 * and helper names are inferred; the out-of-line /Od copies in stdMath.c are
 * therefore _stdMath_floorf and _stdMath_sqrtf.
 */
#define sqrtf(x) (stdMath_sqrtf(x))
#define floorf(x) (stdMath_floorf(x))
#define ceilf(x) (stdMath_ceilf(x))
#define lrintf(x) (stdMath_lrintf(x))

// Retail expands this as a macro: every use of `val` re-evaluates the argument
// (sithAIAwareness_IsInTransmittingRange has four inlined sqrt bounces for one
// ClipNearZero(rdVector_Len3(...))), and it has no out-of-line copy.
#define stdMath_ClipNearZero(val) ((((val) < 0.0f ? -(val) : (val)) > 0.00001f) ? (val) : 0.0f)

J3D_EXTERN_C_END
#endif // STD_STDMATH_H
