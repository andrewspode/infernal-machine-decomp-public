#ifndef RDROID_RDVECTOR_H
#define RDROID_RDVECTOR_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>
#include <rdroid/Main/rdroid.h>

#include <math.h>
#include <std/General/stdMath.h>

J3D_EXTERN_C_START

/*
 * Retail has out-of-line rdVector_Set3, Set4, Neg3, Add3Acc, Dot3, Len3 and
 * InvScale3 definitions in rdVector.c, and a few sith callers call them, but
 * most callers expand the header inline versions. rdVector.c defines
 * RDVECTOR_NO_INLINE so it can provide the external definitions.
 * RDVECTOR_NO_INLINE_LEN3 selects only the external length helper for callers
 * such as sithAnimate that use its retail out-of-line definition.
 * rdVector_Dot3's only out-of-line caller, sithPlayerControls_GetPushPullMoveNorm,
 * names the function as (rdVector_Dot3)(a, b) with the macro in scope; its TU's
 * other dot products expand the macro.
 * RDVECTOR_NO_INLINE_SET3 and RDVECTOR_NO_INLINE_SET4 likewise select only the
 * external rdVector_Set3 / rdVector_Set4
 * (sithWhip_StartWhipSwing calls it at 0x004CBBA0).
 *
 * This opt-in set is complete. Retail's whole out-of-line call inventory for the
 * optionally-inlined helpers is 12 functions / 18 call sites, censused from the
 * image's own call rel32 targets, and every one is covered by a define above;
 * Neg3, Add3Acc and InvScale3 are reached only through RDVECTOR_NO_INLINE, from
 * TUs whose retail bytes call them everywhere, so they need no per-helper
 * selector. Do not add another selector without a retail call to justify it:
 * for the census and its reproduction.
 */

/*
 * Every component macro below (and rdMath.h's DistancePointToPlane and
 * ClampVector3Acc) reads and writes a component by index from the first
 * member, (&(v)->x)[0], [1], [2] (and [3] for w), not by member name. This is
 * a release-chosen header form, not recovered text: /Od code cannot tell it
 * from (v)->y (constant offsets either way). Retail's bytes need it at
 * sithAIUtil_MakeRandPoint's two newDir copies, where VC5 reuses the record a
 * member select of the last component creates for memcpy's block move.
 *
 */

// Retail global (rdroid.c, 0x00509A00); Jedi Knight's rdVector.h declares it first of these.
extern const rdVector2 rdroid_zeroVector2;

extern const rdVector3 rdroid_zeroVector3;

extern const rdVector3 rdroid_xVector3;

extern const rdVector3 rdroid_yVector3;

extern const rdVector3 rdroid_zVector3;

void J3DAPI rdVector_Set2(rdVector2* dest, float x, float y);
void J3DAPI rdVector_Set3(rdVector3* dest, float x, float y, float z);
void J3DAPI rdVector_Set4(rdVector4* dest, float x, float y, float z, float w);

void J3DAPI rdVector_Copy2(rdVector2* dest, const rdVector2* src); // Added
void J3DAPI rdVector_Copy3(rdVector3* dest, const rdVector3* src); // Added
void J3DAPI rdVector_Copy4(rdVector4* dest, const rdVector4* src); // Added

void J3DAPI rdVector_Neg2(rdVector2* dest, const rdVector2* src);
void J3DAPI rdVector_Neg3(rdVector3* dest, const rdVector3* src);
void J3DAPI rdVector_Neg4(rdVector4* dest, const rdVector4* src);

void J3DAPI rdVector_Neg2Acc(rdVector2* a);
void J3DAPI rdVector_Neg3Acc(rdVector3* a); // Added
void J3DAPI rdVector_Neg4Acc(rdVector4* a);

void J3DAPI rdVector_Add2(rdVector2* c, const rdVector2* a, const rdVector2* b);
void J3DAPI rdVector_Add3(rdVector3* c, const rdVector3* a, const rdVector3* b); // Added
void J3DAPI rdVector_Add4(rdVector4* c, const rdVector4* a, const rdVector4* b);

void J3DAPI rdVector_Add2Acc(rdVector2* a, const rdVector2* b);
void J3DAPI rdVector_Add3Acc(rdVector3* a, const rdVector3* b);
void J3DAPI rdVector_Add4Acc(rdVector4* a, const rdVector4* b); // Added

void J3DAPI rdVector_Sub2(rdVector2* c, const rdVector2* a, const rdVector2* b);
void J3DAPI rdVector_Sub3(rdVector3* c, const rdVector3* a, const rdVector3* b); // Added
void J3DAPI rdVector_Sub4(rdVector4* c, const rdVector4* a, const rdVector4* b); // Added from debug version

void J3DAPI rdVector_Sub2Acc(rdVector2* a, const rdVector2* b);
void J3DAPI rdVector_Sub3Acc(rdVector3* a, const rdVector3* b); // Added
void J3DAPI rdVector_Sub4Acc(rdVector4* a, const rdVector4* b);

void J3DAPI rdVector_Scale3(rdVector3* dest, const rdVector3* src, float scalar); // Added from debug version
void J3DAPI rdVector_Scale4(rdVector4* dest, const rdVector4* src, float scalar); // Added from debug version

void J3DAPI rdVector_Scale3Acc(rdVector3* v, float scalar); // Added from debug version

void J3DAPI rdVector_InvScale3(rdVector3* dest, const rdVector3* src, float scalar);




float J3DAPI rdVector_Len2(const rdVector2* vec);
float J3DAPI rdVector_Len3(const rdVector3* vec);
float J3DAPI rdVector_Len4(const rdVector4* vec);


float J3DAPI rdVector_Dot2(const rdVector2* a, const rdVector2* b);
float J3DAPI rdVector_Dot3(const rdVector3* a, const rdVector3* b);
float J3DAPI rdVector_Dot4(const rdVector4* a, const rdVector4* b);

void J3DAPI rdVector_Cross3(rdVector3* dest, const rdVector3* v1, const rdVector3* v2); // Added
void J3DAPI rdVector_Cross3Acc(rdVector3* dest, const rdVector3* v2);


float J3DAPI rdVector_Normalize2(rdVector2* dest, const rdVector2* src);
float J3DAPI rdVector_Normalize3(rdVector3* dest, const rdVector3* src);
float J3DAPI rdVector_Normalize3Quick(rdVector3* dest, const rdVector3* src);
float J3DAPI rdVector_Normalize4(rdVector4* dest, const rdVector4* src);

float J3DAPI rdVector_Normalize2Acc(rdVector2* vec);
float J3DAPI rdVector_Normalize3Acc(rdVector3* vec);
float J3DAPI rdVector_Normalize3QuickAcc(rdVector3* src);


void J3DAPI rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr);
void J3DAPI rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr);
void J3DAPI rdVector_ExtractAngle(const rdVector3* v1, rdVector3* pyr);



// rdVector.c defines the (LINK-discarded) function itself.
#ifndef RDVECTOR_NO_INLINE
/* A memcpy macro, not an __inline function: the debug build has no parameter home at its sites. */
#define rdVector_Copy2(dest, src) memcpy((dest), (src), sizeof(rdVector2))
#endif




// rdVector.c defines the (LINK-discarded) function itself.
#ifndef RDVECTOR_NO_INLINE
/* A macro, not an __inline function: the debug build (/Od /Ob0) writes the four differences out at its one site,
 * sithAnimate_StartThingFadeAnim (debug 0x47b298), with no call and no parameter homes. */
#define rdVector_Sub4(c, a, b) \
    ((&(c)->x)[0] = (&(a)->x)[0] - (&(b)->x)[0], (&(c)->x)[1] = (&(a)->x)[1] - (&(b)->x)[1], \
     (&(c)->x)[2] = (&(a)->x)[2] - (&(b)->x)[2], (&(c)->x)[3] = (&(a)->x)[3] - (&(b)->x)[3])
#endif

#ifndef RDVECTOR_NO_INLINE
__inline void J3DAPI rdVector_InvScale3(rdVector3* dest, const rdVector3* src, float scalar)
{
    float invScalar = 1.0f / scalar;
    dest->x = ((src->x) * invScalar);
    dest->y = ((src->y) * invScalar);
    dest->z = ((src->z) * invScalar);
}
#endif



__inline float J3DAPI rdVector_Dist2(const rdVector2* a, const rdVector2* b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    return sqrtf(dx * dx + dy * dy);
}

/* A macro, not an __inline function: the debug build computes every difference twice and has no dx/dy/dz slots
 * (sithCogFunction_VectorDist, debug 0x495EB5); rdVector.c has no Dist3 body. The former __inline
 * body's text with each dx/dy/dz written out (plain products, as that body had). */
#define rdVector_Dist3(a, b) \
    sqrtf(((&(a)->x)[0] - (&(b)->x)[0]) * ((&(a)->x)[0] - (&(b)->x)[0]) \
          + ((&(a)->x)[1] - (&(b)->x)[1]) * ((&(a)->x)[1] - (&(b)->x)[1]) \
          + ((&(a)->x)[2] - (&(b)->x)[2]) * ((&(a)->x)[2] - (&(b)->x)[2]))

#if !defined(RDVECTOR_NO_INLINE)
/*
 * Callers expand Dot3 as a hygienic macro, not an __inline function. VC5 keeps
 * the redundant parentheses around each component read as tree nodes, which
 * reproduces retail's x87 shapes: the three-load preload with memory-operand
 * multiplies for stack vectors (sithCogFunction_VectorDot) and the six-load
 * preload for heap vectors (sithAIMove_CheckMoveAlign). The __inline function form
 * gives neither. Each product is
 * RD_MULTIPLY (rdroid.h) like every other product macro below: each factor
 * wrapped once, the product twice (the adopted header fit).
 *
 */
#define rdVector_Dot3(a, b) \
    (RD_MULTIPLY((&(a)->x)[0], (&(b)->x)[0]) + RD_MULTIPLY((&(a)->x)[1], (&(b)->x)[1]) \
     + RD_MULTIPLY((&(a)->x)[2], (&(b)->x)[2]))
#endif
#if !defined(RDVECTOR_NO_INLINE) && !defined(RDVECTOR_NO_INLINE_LEN3)
/* A macro, not an __inline function: the debug build reads the operand straight into the products at every site, no
 * parameter home (e.g. sithAIAwareness_CheckAdjoinDistance, debug 0x4E0D8E). */
#define rdVector_Len3(v) sqrtf(rdVector_Dot3(v, v))
#endif

/* A macro, not an __inline function: the debug build writes it out at its one site (sithCogFunctionThing_InterpolatePYR,
 * debug 0x4922ED: each component written out, no parameter homes). The /Od code cannot tell the
 * addend order (`fadd a` last either way). The body is the former __inline body's text with hygienic argument
 * brackets (plain products, not RD_MULTIPLY; no paren text chosen from retail bytes alone). */
#define rdVector_Lerp3(dest, a, b, t) \
    ((&(dest)->x)[0] = (&(a)->x)[0] + ((&(b)->x)[0] - (&(a)->x)[0]) * (t), \
     (&(dest)->x)[1] = (&(a)->x)[1] + ((&(b)->x)[1] - (&(a)->x)[1]) * (t), \
     (&(dest)->x)[2] = (&(a)->x)[2] + ((&(b)->x)[2] - (&(a)->x)[2]) * (t))

/*
 * Retail callers expand the common 3D helpers as hygienic macros, not as the
 * __inline functions above (those stay as reference definitions; removing
 * them perturbs VC5's declaration-count-dependent operand/slot ordering in
 * unrelated functions). A macro argument is a different c1 tree from an
 * inlined parameter, and VC5 keeps redundant parentheses around a component
 * read as a tree node, so these spellings reproduce retail's x87 load order.
 * The consistent spelling: every product or scaling is RD_MULTIPLY (rdroid.h,
 * `(D3DMultiply(a, b))`), so every component read is wrapped once and every
 * product twice, `((((a)->x) * (s)))`; sums, differences and negations stay
 * plain. A paren around a computed x87 value is a c1 tree node that weighs in
 * VC5's x87 block schedule (a paren around a memory read does not), so the
 * product depth decides which blocks are scheduled as retail's. No original
 * header text survives: the depths are the joint project-wide fit, with the
 * sites it does not reproduce recorded,
 * and the
 * product macro that gives them is a reconstruction (name and body inferred:
 * extra product brackets are a macro's fingerprint).
 * The scaling macros also write
 * the ordinary hygienic paren around their scalar parameter,
 * RD_MULTIPLY((src)->x, (scalar)), so a scalar reaches the product as ((s))
 * (checked project-wide;
 *  Each helper was first checked
 * across every TU. rdVector_CrossDot3's
 * singly wrapped products are not this text and stay written out.
 */
#if !defined(RDVECTOR_NO_INLINE) && !defined(RDVECTOR_NO_INLINE_SET3)
#define rdVector_Set3(dest, x_, y_, z_) \
    ((&(dest)->x)[0] = (x_), (&(dest)->x)[1] = (y_), (&(dest)->x)[2] = (z_))
#endif

#if !defined(RDVECTOR_NO_INLINE) && !defined(RDVECTOR_NO_INLINE_SET4)
#define rdVector_Set4(dest, x_, y_, z_, w_) \
    ((&(dest)->x)[0] = (x_), (&(dest)->x)[1] = (y_), (&(dest)->x)[2] = (z_), (&(dest)->x)[3] = (w_))
#endif

/*
 * Copy3/Copy4 are memcpy calls: the Indy3D debug build (/Od) has `push 0Ch; push src; push dest;
 * call memcpy` at every Copy3 site and rdVector.c's own Copy2/3/4 are 23-byte memcpy bodies; with this
 * form 31 more debug functions are /Od-identical and none is lost.
 * ScaleAdd3 writes the addend first, as its debug sites load it first.
 */
#define rdVector_Copy3(dest, src) memcpy((dest), (src), sizeof(rdVector3))
#define rdVector_Copy4(dest, src) memcpy((dest), (src), sizeof(rdVector4))

#if !defined(RDVECTOR_NO_INLINE) && !defined(RDVECTOR_NO_INLINE_NEG3)
#define rdVector_Neg3(dest, src) \
    ((&(dest)->x)[0] = -(&(src)->x)[0], (&(dest)->x)[1] = -(&(src)->x)[1], (&(dest)->x)[2] = -(&(src)->x)[2])
#endif

#define rdVector_Neg3Acc(v) \
    ((&(v)->x)[0] = -(&(v)->x)[0], (&(v)->x)[1] = -(&(v)->x)[1], (&(v)->x)[2] = -(&(v)->x)[2])

#ifndef RDVECTOR_NO_INLINE
#define rdVector_Add3Acc(a, b) \
    ((&(a)->x)[0] += (&(b)->x)[0], (&(a)->x)[1] += (&(b)->x)[1], (&(a)->x)[2] += (&(b)->x)[2])
#endif

#define rdVector_Add4Acc(a, b) \
    ((&(a)->x)[0] += (&(b)->x)[0], (&(a)->x)[1] += (&(b)->x)[1], (&(a)->x)[2] += (&(b)->x)[2], (&(a)->x)[3] += (&(b)->x)[3])

#define rdVector_Add3(c, a, b) \
    ((&(c)->x)[0] = (&(a)->x)[0] + (&(b)->x)[0], \
     (&(c)->x)[1] = (&(a)->x)[1] + (&(b)->x)[1], \
     (&(c)->x)[2] = (&(a)->x)[2] + (&(b)->x)[2])

#define rdVector_Sub3(c, a, b) \
    ((&(c)->x)[0] = (&(a)->x)[0] - (&(b)->x)[0], \
     (&(c)->x)[1] = (&(a)->x)[1] - (&(b)->x)[1], \
     (&(c)->x)[2] = (&(a)->x)[2] - (&(b)->x)[2])

#define rdVector_Sub3Acc(a, b) \
    ((&(a)->x)[0] -= (&(b)->x)[0], (&(a)->x)[1] -= (&(b)->x)[1], (&(a)->x)[2] -= (&(b)->x)[2])

/*
 * 2-D rotation of (x, y) by an angle given as its sine and cosine (stdMath_SinCos
 * order): x' = x*c - y*s, y' = x*s + y*c. Reconstructed (name inferred): the same
 * rotation text is written at every 2-D rotate in the image (rdPolyline_Draw's four
 * vertex pairs, sithRenderSky_HorizonFaceToPlane's tu/tv), and only this header's
 * product style (each product parenthesised twice, the difference/sum as one node)
 * reproduces retail there (HorizonFaceToPlane exact).
 * The products are RD_MULTIPLY (rdroid.h).
 */
#define RDVECTOR_ROTATE2_X(x, y, s, c) (RD_MULTIPLY(x, c) - RD_MULTIPLY(y, s))
#define RDVECTOR_ROTATE2_Y(x, y, s, c) (RD_MULTIPLY(x, s) + RD_MULTIPLY(y, c))

#define rdVector_Scale2(dest, src, scalar) \
    ((&(dest)->x)[0] = RD_MULTIPLY((&(src)->x)[0], (scalar)), \
     (&(dest)->x)[1] = RD_MULTIPLY((&(src)->x)[1], (scalar)))

#define rdVector_Scale3(dest, src, scalar) \
    ((&(dest)->x)[0] = RD_MULTIPLY((&(src)->x)[0], (scalar)), \
     (&(dest)->x)[1] = RD_MULTIPLY((&(src)->x)[1], (scalar)), \
     (&(dest)->x)[2] = RD_MULTIPLY((&(src)->x)[2], (scalar)))

#define rdVector_Scale4(dest, src, scalar) \
    ((&(dest)->x)[0] = RD_MULTIPLY((&(src)->x)[0], (scalar)), \
     (&(dest)->x)[1] = RD_MULTIPLY((&(src)->x)[1], (scalar)), \
     (&(dest)->x)[2] = RD_MULTIPLY((&(src)->x)[2], (scalar)), \
     (&(dest)->x)[3] = RD_MULTIPLY((&(src)->x)[3], (scalar)))

#define rdVector_Scale3Acc(v, scalar) \
    ((&(v)->x)[0] = RD_MULTIPLY((&(v)->x)[0], (scalar)), \
     (&(v)->x)[1] = RD_MULTIPLY((&(v)->x)[1], (scalar)), \
     (&(v)->x)[2] = RD_MULTIPLY((&(v)->x)[2], (scalar)))

#define rdVector_ScaleAdd3Acc(dest, src, scalar) \
    ((&(dest)->x)[0] += RD_MULTIPLY((&(src)->x)[0], (scalar)), \
     (&(dest)->x)[1] += RD_MULTIPLY((&(src)->x)[1], (scalar)), \
     (&(dest)->x)[2] += RD_MULTIPLY((&(src)->x)[2], (scalar)))

#define rdVector_ScaleAdd3(dest, src, scalar, add) \
    ((&(dest)->x)[0] = (&(add)->x)[0] + RD_MULTIPLY((&(src)->x)[0], (scalar)), \
     (&(dest)->x)[1] = (&(add)->x)[1] + RD_MULTIPLY((&(src)->x)[1], (scalar)), \
     (&(dest)->x)[2] = (&(add)->x)[2] + RD_MULTIPLY((&(src)->x)[2], (scalar)))

#define rdVector_ScaleAdd4(dest, src, scalar, add) \
    ((&(dest)->x)[0] = (&(add)->x)[0] + RD_MULTIPLY((&(src)->x)[0], (scalar)), \
     (&(dest)->x)[1] = (&(add)->x)[1] + RD_MULTIPLY((&(src)->x)[1], (scalar)), \
     (&(dest)->x)[2] = (&(add)->x)[2] + RD_MULTIPLY((&(src)->x)[2], (scalar)), \
     (&(dest)->x)[3] = (&(add)->x)[3] + RD_MULTIPLY((&(src)->x)[3], (scalar)))

#define rdVector_Cross3(dest, v1, v2) \
    ((&(dest)->x)[0] = RD_MULTIPLY((&(v1)->x)[1], (&(v2)->x)[2]) - RD_MULTIPLY((&(v1)->x)[2], (&(v2)->x)[1]), \
     (&(dest)->x)[1] = RD_MULTIPLY((&(v1)->x)[2], (&(v2)->x)[0]) - RD_MULTIPLY((&(v1)->x)[0], (&(v2)->x)[2]), \
     (&(dest)->x)[2] = RD_MULTIPLY((&(v1)->x)[0], (&(v2)->x)[1]) - RD_MULTIPLY((&(v1)->x)[1], (&(v2)->x)[0]))

#define rdVector_CrossDot3(a, b, c) \
    (((((&(a)->x)[1]) * ((&(b)->x)[2])) - (((&(b)->x)[1]) * ((&(a)->x)[2]))) * ((&(c)->x)[0]) \
     + ((((&(a)->x)[2]) * ((&(b)->x)[0])) - (((&(b)->x)[2]) * ((&(a)->x)[0]))) * ((&(c)->x)[1]) \
     + ((((&(a)->x)[0]) * ((&(b)->x)[1])) - (((&(b)->x)[0]) * ((&(a)->x)[1]))) * ((&(c)->x)[2]))

J3D_EXTERN_C_END
#endif // RDROID_RDVECTOR_H
