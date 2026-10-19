#include "rdMath.h"

#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(rdMath_countfit_1, 15)
CF_PROTO(rdMath_countfit_2, 9)
/* end count-fit */

/* The debug build's rdMath.c (0x512850..0x512D53, one /Od copy of every function below, all byte-identical to this text)
   takes absolute values with an inline `< 0.0f` ternary (fchs arm), not a fabs call: the same text as rdVector.c's
   RDVECTOR_ABS. Name inferred. CalcSurfaceNormal, ClipVector3Acc, DeltaAngleNormalizedAbs and PointsCollinear are
   LINK-discarded (no retail symbol, relocations.yml discarded:); their bodies are the debug build's. */
#define RDMATH_ABS(x) ((x) < 0.0f ? -(x) : (x))

// Debug 0x512850. Its call to ClipVector3Acc pushes the 1e-6 as a 4-byte float: a prototype is in scope (rdMath.h).
void J3DAPI rdMath_CalcSurfaceNormal(rdVector3* pDestNormal, rdVector3* pVert1, rdVector3* pVert2, rdVector3* pVert3)
{
    rdVector3 v1;
    rdVector3 v2;

    rdVector_Sub3(&v1, pVert2, pVert1);
    rdVector_Sub3(&v2, pVert3, pVert1);
    rdVector_Normalize3Acc(&v1);
    rdVector_Normalize3Acc(&v2);
    rdVector_Cross3(pDestNormal, &v1, &v2);
    rdVector_Normalize3Acc(pDestNormal);
    rdMath_ClipVector3Acc(pDestNormal, 0.000001f);
}

float J3DAPI rdMath_DistancePointToPlane(const rdVector3* pPoint, const rdVector3* pPlaneNormal, const rdVector3* pPointOnPlane)
{
    return ((pPoint->x - pPointOnPlane->x) * pPlaneNormal->x)
        + ((pPoint->y - pPointOnPlane->y) * pPlaneNormal->y)
        + ((pPoint->z - pPointOnPlane->z) * pPlaneNormal->z);
}

/* Retail's linker discarded this function (no retail symbol, address or caller). Its constants
   are the head of the TU's .rdata pool: retail pools 1.0f, 0.0f, 90.0f (0x00509BDC..E4) ahead of
   DeltaAngleNormalized's own -1.0f and 180.0f, and VC5 pools float literals in textual first-use
   order, so a function before DeltaAngleNormalized first-used exactly those three, in that order.
   The debug build's rdMath_DeltaAngleNormalizedAbs is that function: its body below
   (1.0f in place of the debug text's double 1.0, which would pool an 8-byte constant retail
   lacks) reproduces the pool byte for byte. It sits at Jedi Knight's position, after
   DistancePointToPlane (JK rdMath.c order: CalcSurfaceNormal, DistancePointToPlane,
   DeltaAngleNormalizedAbs, DeltaAngleNormalized, ClipVector3Acc, PointsCollinear), which
   DistancePointToPlane's lack of float literals leaves pool-neutral. */
float J3DAPI rdMath_DeltaAngleNormalizedAbs(const rdVector3* pVectorX, const rdVector3* pVectorY)
{
    float dot = rdVector_Dot3(pVectorX, pVectorY);
    if ( dot == 1.0f )
    {
        return 0.0f;
    }

    return 90.0f - stdMath_ArcSin1(dot);
}

float J3DAPI rdMath_DeltaAngleNormalized(const rdVector3* pVectorX, const rdVector3* pVectorY, const rdVector3* pVectorZ)
{
    float angle;
    float dot;
    rdVector3 cross;

    dot = STDMATH_CLAMP(rdVector_Dot3(pVectorX, pVectorY), -1.0f, 1.0f);
    if ( dot == 1.0f )
    {
        return 0.0f;
    }

    if ( dot == -1.0f )
    {
        return 180.0f;
    }

    angle = 90.0f - stdMath_ArcSin1(dot);
    rdVector_Cross3(&cross, pVectorX, pVectorY);
    if ( rdVector_Dot3(&cross, pVectorZ) > 0.0f )
    {
        return angle;
    }

    return (-angle);
}

// Debug 0x512B32 (called by CalcSurfaceNormal); Jedi Knight's rdMath.c has it here. Name inferred.
void J3DAPI rdMath_ClipVector3Acc(rdVector3* pVector, float minVal)
{
    pVector->x = RDMATH_ABS(pVector->x) < minVal ? 0.0f : pVector->x;
    pVector->y = RDMATH_ABS(pVector->y) < minVal ? 0.0f : pVector->y;
    pVector->z = RDMATH_ABS(pVector->z) < minVal ? 0.0f : pVector->z;
}

int J3DAPI rdMath_PointsCollinear(const rdVector3* p1, const rdVector3* p2, const rdVector3* p3)
{
    rdVector3 v1;
    rdVector3 v2;
    float dot;

    rdVector_Sub3(&v1, p2, p1);
    rdVector_Sub3(&v2, p3, p1);
    rdVector_Normalize3Acc(&v1);
    rdVector_Normalize3Acc(&v2);
    dot = RDMATH_ABS(rdVector_Dot3(&v1, &v2));
    // Debug 0x512D01..0x512D44: the inner compare's value and the outer conditional each take an /Od temporary, then
    // one test of the result; `&&` in either place gives one temporary fewer.
    if ( dot >= 0.999f ? dot <= 1.001f : 0 )
    {
        return 1;
    }

    return 0;
}
