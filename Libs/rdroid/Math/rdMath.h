#ifndef RDROID_RDMATH_H
#define RDROID_RDMATH_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

#include <std/General/stdMath.h>

#include <math.h>


J3D_EXTERN_C_START

float J3DAPI rdMath_DistancePointToPlane(const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane);
void J3DAPI rdMath_CalcSurfaceNormal(rdVector3* pDestNormal, rdVector3* pVert1, rdVector3* pVert2, rdVector3* pVert3);

/*
 * Retail has one out-of-line rdMath_DistancePointToPlane in rdMath.c, called
 * only by rdLight_CalcFaceIntensity; every other plane distance is an
 * expansion of the macro below (26 of them against &rdroid_zeroVector3, which
 * nobody writes by hand; retail really does `fsub [rdroid_zeroVector3]`).
 * Callers opt in with RDMATH_INLINE_DISTANCEPOINTTOPLANE before including
 * this header, so rdMath.c and rdLight.c keep the out-of-line function. The
 * macro keeps member paths (&pThing->pos folds to pThing+0x..), which a
 * pointer parameter cannot. Each product is parenthesised once: across the
 * project one level reproduces retail better than two, and an __inline body
 * as the caller form does not reproduce it (inferred).
 * Components are read by
 * index, as in rdVector.h's component macros.
 * No shadowed __inline copy:
 * it was never compiled into any code.
 */
#ifdef RDMATH_INLINE_DISTANCEPOINTTOPLANE
#define rdMath_DistancePointToPlane(pPoint, pPlaneNormal, pPointOnPlane) \
    (((&(pPlaneNormal)->x)[0] * ((&(pPoint)->x)[0] - (&(pPointOnPlane)->x)[0])) \
    + ((&(pPlaneNormal)->x)[1] * ((&(pPoint)->x)[1] - (&(pPointOnPlane)->x)[1])) \
    + ((&(pPlaneNormal)->x)[2] * ((&(pPoint)->x)[2] - (&(pPointOnPlane)->x)[2])))
#endif
float J3DAPI rdMath_DeltaAngleNormalizedAbs(const rdVector3* pVectorX, const rdVector3* pVectorY);
float J3DAPI rdMath_DeltaAngleNormalized(const rdVector3* pVectorX, const rdVector3* pVectorY, const rdVector3* pVectorZ);
void J3DAPI rdMath_ClipVector3Acc(rdVector3* pVector, float minVal);
int J3DAPI rdMath_PointsCollinear(const rdVector3* p1, const rdVector3* p2, const rdVector3* p3);

/*
 * Callers expand ClampVector3Acc as a macro whose parenthesised `((vect)->x)`
 * compare reads keep retail's x87 `fld; fld; fcompp` clamp compares
 * (sithPhysics_UpdateThingPhysics). The kept-value arm
 * `(vect)->x` is not parenthesised: the debug build copies it with integer
 * moves under /Od, which a parenthesised arm does not give.
 *
 */
#define rdMath_ClampVector3Acc(vect, minVal, maxVal) \
    ((&(vect)->x)[0] = (((&(vect)->x)[0]) >= (minVal) ? (((&(vect)->x)[0]) <= (maxVal) ? (&(vect)->x)[0] : maxVal) : minVal), \
     (&(vect)->x)[1] = (((&(vect)->x)[1]) >= (minVal) ? (((&(vect)->x)[1]) <= (maxVal) ? (&(vect)->x)[1] : maxVal) : minVal), \
     (&(vect)->x)[2] = (((&(vect)->x)[2]) >= (minVal) ? (((&(vect)->x)[2]) <= (maxVal) ? (&(vect)->x)[2] : maxVal) : minVal))


J3D_EXTERN_C_END
#endif // RDROID_RDMATH_H
