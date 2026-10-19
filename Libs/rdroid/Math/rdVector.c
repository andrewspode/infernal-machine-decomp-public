#define RDVECTOR_NO_INLINE

#include "rdMatrix.h"
#include <std/General/stdMath.h>
#include "rdVector.h"

#include <rdroid/Main/rdroid.h>





// rdVector.h expands these helpers as macros for its callers; this file defines
// the functions themselves (most LINK-discarded).
#undef rdVector_Copy3
#undef rdVector_Copy4
#undef rdVector_Neg3Acc
#undef rdVector_Add3
#undef rdVector_Add4Acc
#undef rdVector_Sub3
#undef rdVector_Sub3Acc
#undef rdVector_Cross3

// Absolute value as a macro (the /Od Normalize3QuickAcc expands it inline, so
// it is not a call): the same text as rdClip.c's RDCLIP_ABS. Name inferred.
#define RDVECTOR_ABS(x) ((x) < 0.0f ? -(x) : (x))

/* Every function below is the debug build's rdVector.c text (0x510DF0..0x511EA1, the same order,
   /Od byte-identical when recompiled; the debug build calls the sqrtf helper out of
   line). The ones with no retail symbol are LINK-discarded. Names and
   parameter names follow Jedi Knight and rdVector.h. */
void J3DAPI rdVector_Set2(rdVector2* dest, float x, float y)
{
    dest->x = x;
    dest->y = y;
}

void J3DAPI rdVector_Set3(rdVector3* dest, float x, float y, float z)
{
    dest->x = x;
    dest->y = y;
    dest->z = z;
}

void J3DAPI rdVector_Set4(rdVector4* dest, float x, float y, float z, float w)
{
    dest->x = x;
    dest->y = y;
    dest->z = z;
    dest->w = w;
}

void J3DAPI rdVector_Copy2(rdVector2* dest, const rdVector2* src)
{
    memcpy(dest, src, sizeof(rdVector2));
}

void J3DAPI rdVector_Copy3(rdVector3* dest, const rdVector3* src)
{
    memcpy(dest, src, sizeof(rdVector3));
}

void J3DAPI rdVector_Copy4(rdVector4* dest, const rdVector4* src)
{
    memcpy(dest, src, sizeof(rdVector4));
}

void J3DAPI rdVector_Neg2(rdVector2* dest, const rdVector2* src)
{
    dest->x = -src->x;
    dest->y = -src->y;
}

void J3DAPI rdVector_Neg3(rdVector3* dest, const rdVector3* src)
{
    dest->x = -src->x;
    dest->y = -src->y;
    dest->z = -src->z;
}

void J3DAPI rdVector_Neg4(rdVector4* dest, const rdVector4* src)
{
    dest->x = -src->x;
    dest->y = -src->y;
    dest->z = -src->z;
    dest->w = -src->w;
}

void J3DAPI rdVector_Neg2Acc(rdVector2* a)
{
    a->x = -a->x;
    a->y = -a->y;
}

void J3DAPI rdVector_Neg3Acc(rdVector3* a)
{
    a->x = -a->x;
    a->y = -a->y;
    a->z = -a->z;
}

void J3DAPI rdVector_Neg4Acc(rdVector4* a)
{
    a->x = -a->x;
    a->y = -a->y;
    a->z = -a->z;
    a->w = -a->w;
}

void J3DAPI rdVector_Add2(rdVector2* c, const rdVector2* a, const rdVector2* b)
{
    c->x = a->x + b->x;
    c->y = a->y + b->y;
}

void J3DAPI rdVector_Add3(rdVector3* c, const rdVector3* a, const rdVector3* b)
{
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
}

void J3DAPI rdVector_Add4(rdVector4* c, const rdVector4* a, const rdVector4* b)
{
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
    c->w = a->w + b->w;
}

void J3DAPI rdVector_Add2Acc(rdVector2* a, const rdVector2* b)
{
    a->x += b->x;
    a->y += b->y;
}

void J3DAPI rdVector_Add3Acc(rdVector3* a, const rdVector3* b)
{
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
}

void J3DAPI rdVector_Add4Acc(rdVector4* a, const rdVector4* b)
{
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
    a->w += b->w;
}

void J3DAPI rdVector_Sub2(rdVector2* c, const rdVector2* a, const rdVector2* b)
{
    c->x = a->x - b->x;
    c->y = a->y - b->y;
}

void J3DAPI rdVector_Sub3(rdVector3* c, const rdVector3* a, const rdVector3* b)
{
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
}

void J3DAPI rdVector_Sub4(rdVector4* c, const rdVector4* a, const rdVector4* b)
{
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
    c->w = a->w - b->w;
}

void J3DAPI rdVector_Sub2Acc(rdVector2* a, const rdVector2* b)
{
    a->x -= b->x;
    a->y -= b->y;
}

void J3DAPI rdVector_Sub3Acc(rdVector3* a, const rdVector3* b)
{
    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;
}

void J3DAPI rdVector_Sub4Acc(rdVector4* a, const rdVector4* b)
{
    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;
    a->w -= b->w;
}

float J3DAPI rdVector_Dot2(const rdVector2* a, const rdVector2* b)
{
    return RD_MULTIPLY((a)->x, (b)->x) + RD_MULTIPLY((a)->y, (b)->y);
}

// The out-of-line helpers are written with the header macros' own text (rdVector.h
// rdVector_Dot3; Scale3/Scale3Acc are called below): kind-6 shared source.
float J3DAPI rdVector_Dot3(const rdVector3* a, const rdVector3* b)
{
    return RD_MULTIPLY((a)->x, (b)->x) + RD_MULTIPLY((a)->y, (b)->y) + RD_MULTIPLY((a)->z, (b)->z);
}

float J3DAPI rdVector_Dot4(const rdVector4* a, const rdVector4* b)
{
    return RD_MULTIPLY((a)->x, (b)->x) + RD_MULTIPLY((a)->y, (b)->y) + RD_MULTIPLY((a)->z, (b)->z) + RD_MULTIPLY((a)->w, (b)->w);
}

void J3DAPI rdVector_Cross3(rdVector3* dest, const rdVector3* v1, const rdVector3* v2)
{
#line 660
    RD_ASSERT((dest!=v1)&&(dest!=v2));
    dest->x = RD_MULTIPLY(v1->y, v2->z) - RD_MULTIPLY(v1->z, v2->y);
    dest->y = RD_MULTIPLY(v1->z, v2->x) - RD_MULTIPLY(v2->z, v1->x);
    dest->z = RD_MULTIPLY(v2->y, v1->x) - RD_MULTIPLY(v1->y, v2->x);
}

void J3DAPI rdVector_Cross3Acc(rdVector3* dest, const rdVector3* v2)
{
    rdVector3 tmp;

    tmp.x = RD_MULTIPLY(dest->y, v2->z) - RD_MULTIPLY(dest->z, v2->y);
    tmp.y = RD_MULTIPLY(dest->z, v2->x) - RD_MULTIPLY(v2->z, dest->x);
    tmp.z = RD_MULTIPLY(v2->y, dest->x) - RD_MULTIPLY(dest->y, v2->x);
    memcpy(dest, &tmp, sizeof(rdVector3));
}

float J3DAPI rdVector_Len2(const rdVector2* vec)
{
    return sqrtf(RD_MULTIPLY((vec)->x, (vec)->x) + RD_MULTIPLY((vec)->y, (vec)->y));
}

float J3DAPI rdVector_Len3(const rdVector3* vec)
{
    return sqrtf(RD_MULTIPLY((vec)->x, (vec)->x) + RD_MULTIPLY((vec)->y, (vec)->y) + RD_MULTIPLY((vec)->z, (vec)->z));
}

float J3DAPI rdVector_Len4(const rdVector4* vec)
{
    return sqrtf(RD_MULTIPLY((vec)->x, (vec)->x) + RD_MULTIPLY((vec)->y, (vec)->y) + RD_MULTIPLY((vec)->z, (vec)->z) + RD_MULTIPLY((vec)->w, (vec)->w));
}

float J3DAPI rdVector_Normalize2(rdVector2* dest, const rdVector2* src)
{
    float len;

#line 781
    RD_ASSERTREL(dest!=src);
    len = sqrtf(RD_MULTIPLY((src)->x, (src)->x) + RD_MULTIPLY((src)->y, (src)->y));
    if ( len != 0.0f )
    {
        dest->x = src->x / len;
        dest->y = src->y / len;
    }
    else
    {
        memcpy(dest, src, sizeof(rdVector2));
    }

    return len;
}

float J3DAPI rdVector_Normalize3(rdVector3* dest, const rdVector3* src)
{
    float len;

#line 817
    RD_ASSERTREL(dest!=src);
    len = sqrtf(RD_MULTIPLY((src)->x, (src)->x) + RD_MULTIPLY((src)->y, (src)->y) + RD_MULTIPLY((src)->z, (src)->z));
    if ( len != 0.0f )
    {
        dest->x = src->x / len;
        dest->y = src->y / len;
        dest->z = src->z / len;
    }
    else
    {
        memcpy(dest, src, sizeof(rdVector3));
    }

    return len;
}

/* LINK-discarded; the debug build's 0x5115A1. Its constants fix the TU's .rdata pool order
   (0.34375f and 0.25f at 0x00509BC0/C4 ahead of Normalize3Acc's 1.0f). Retail has no copy of it,
   so nothing shows how it was optimized. The line-909 assert's text is debug-only (not in
   retail's .data), so it is RD_ASSERT. */

float J3DAPI rdVector_Normalize3Quick(rdVector3* dest, const rdVector3* src)
{
    float min;
    float len;
    float temp;
    float inv;
    float max;
    float med;

    max = RDVECTOR_ABS(src->x);
    med = RDVECTOR_ABS(src->y);
    min = RDVECTOR_ABS(src->z);

    if ( min > med )
    {
        if ( min > max )
        {
            temp = min;
            min = max;
            max = temp;
            if ( med < min )
            {
                temp = min;
                min = med;
                med = temp;
            }
        }
        else
        {
            temp = min;
            min = med;
            med = temp;
        }
    }
    else if ( max < med )
    {
        temp = med;
        med = max;
        max = temp;
        if ( min > med )
        {
            temp = min;
            min = med;
            med = temp;
        }
    }

#line 903
    RD_ASSERTREL((min <= med) && (med <= max));

    len = (0.34375f * med) + (0.25f * min) + max;
#line 909
    RD_ASSERT(len > ((float)0.0));

    inv = 1.0f / len;
    rdVector_Scale3(dest, src, inv);
    return len;
}


float J3DAPI rdVector_Normalize4(rdVector4* dest, const rdVector4* src)
{
    float len;

#line 941
    RD_ASSERTREL(dest!=src);
    len = sqrtf(RD_MULTIPLY((src)->x, (src)->x) + RD_MULTIPLY((src)->y, (src)->y) + RD_MULTIPLY((src)->z, (src)->z) + RD_MULTIPLY((src)->w, (src)->w));
    if ( len != 0.0f )
    {
        dest->x = src->x / len;
        dest->y = src->y / len;
        dest->z = src->z / len;
        dest->w = src->w / len;
    }
    else
    {
        memcpy(dest, src, sizeof(rdVector4));
    }

    return len;
}

float J3DAPI rdVector_Normalize2Acc(rdVector2* vec)
{
    float len;

    len = sqrtf(RD_MULTIPLY((vec)->x, (vec)->x) + RD_MULTIPLY((vec)->y, (vec)->y));
    if ( len != 0.0f )
    {
        vec->x = vec->x / len;
        vec->y = vec->y / len;
    }

    return len;
}

float J3DAPI rdVector_Normalize3Acc(rdVector3* vec)
{
    float invLen;
    float len;

    len = sqrtf(RD_MULTIPLY((vec)->x, (vec)->x) + RD_MULTIPLY((vec)->y, (vec)->y) + RD_MULTIPLY((vec)->z, (vec)->z));
    if ( len != 0.0f )
    {
        invLen = 1.0f / len;
        rdVector_Scale3Acc(vec, invLen);
    }

    return len;
}

// Retail compiled this function without optimization.
#pragma optimize("", off)
float J3DAPI rdVector_Normalize3QuickAcc(rdVector3* src)
{
    float min;
    float len;
    float temp;
    float inv;
    float max;
    float med;

    max = RDVECTOR_ABS(src->x);
    med = RDVECTOR_ABS(src->y);
    min = RDVECTOR_ABS(src->z);

    if ( min > med )
    {
        if ( min > max )
        {
            temp = min;
            min = max;
            max = temp;
            if ( med < min )
            {
                temp = min;
                min = med;
                med = temp;
            }
        }
        else
        {
            temp = min;
            min = med;
            med = temp;
        }
    }
    else if ( max < med )
    {
        temp = med;
        med = max;
        max = temp;
        if ( min > med )
        {
            temp = min;
            min = med;
            med = temp;
        }
    }

    // Retail's assertion branches straight to the handler call: J3D_ASSERTREL's
    // `||` form; the old conditional-expression macro left an /Od temporary.
#line 1091
    RD_ASSERTREL((min <= med) && (med <= max));

    len = (0.34375f * med) + (0.25f * min) + max;
    inv = 1.0f / len;
    rdVector_Scale3Acc(src, inv);
    return len;
}
#pragma optimize("", on)

// The debug build's rdVector.c continues after Normalize3QuickAcc with Normalize4Acc, the Scale and InvScale
// families (0x511AED..0x511E0B; InvScale3 is the one retail kept), then Rotate3, Rotate3Acc and ExtractAngle.
// The others are LINK-discarded (relocations.yml discarded:); bodies from the debug code.
#undef rdVector_Scale2
#undef rdVector_Scale3
#undef rdVector_Scale4
#undef rdVector_Scale3Acc

float J3DAPI rdVector_Normalize4Acc(rdVector4* vec)
{
    float invLen;
    float len;

    len = sqrtf(RD_MULTIPLY((vec)->x, (vec)->x) + RD_MULTIPLY((vec)->y, (vec)->y) + RD_MULTIPLY((vec)->z, (vec)->z) + RD_MULTIPLY((vec)->w, (vec)->w));
    if ( len != 0.0f )
    {
        invLen = 1.0f / len;
        vec->x = RD_MULTIPLY((vec)->x, (invLen));
        vec->y = RD_MULTIPLY((vec)->y, (invLen));
        vec->z = RD_MULTIPLY((vec)->z, (invLen));
        vec->w = RD_MULTIPLY((vec)->w, (invLen));
    }

    return len;
}

void J3DAPI rdVector_Scale2(rdVector2* dest, const rdVector2* src, float scalar)
{
    dest->x = RD_MULTIPLY((src)->x, (scalar));
    dest->y = RD_MULTIPLY((src)->y, (scalar));
}

void J3DAPI rdVector_Scale3(rdVector3* dest, const rdVector3* src, float scalar)
{
    dest->x = RD_MULTIPLY((src)->x, (scalar));
    dest->y = RD_MULTIPLY((src)->y, (scalar));
    dest->z = RD_MULTIPLY((src)->z, (scalar));
}

void J3DAPI rdVector_Scale4(rdVector4* dest, const rdVector4* src, float scalar)
{
    dest->x = RD_MULTIPLY((src)->x, (scalar));
    dest->y = RD_MULTIPLY((src)->y, (scalar));
    dest->z = RD_MULTIPLY((src)->z, (scalar));
    dest->w = RD_MULTIPLY((src)->w, (scalar));
}

void J3DAPI rdVector_Scale2Acc(rdVector2* v, float scalar)
{
    v->x = RD_MULTIPLY((v)->x, (scalar));
    v->y = RD_MULTIPLY((v)->y, (scalar));
}

void J3DAPI rdVector_Scale3Acc(rdVector3* v, float scalar)
{
    v->x = RD_MULTIPLY((v)->x, (scalar));
    v->y = RD_MULTIPLY((v)->y, (scalar));
    v->z = RD_MULTIPLY((v)->z, (scalar));
}

void J3DAPI rdVector_Scale4Acc(rdVector4* v, float scalar)
{
    v->x = RD_MULTIPLY((v)->x, (scalar));
    v->y = RD_MULTIPLY((v)->y, (scalar));
    v->z = RD_MULTIPLY((v)->z, (scalar));
    v->w = RD_MULTIPLY((v)->w, (scalar));
}

void J3DAPI rdVector_InvScale2(rdVector2* dest, const rdVector2* src, float scalar)
{
    dest->x = src->x / scalar;
    dest->y = src->y / scalar;
}

void J3DAPI rdVector_InvScale3(rdVector3* dest, const rdVector3* src, float scalar)
{
    float invScalar = 1.0f / scalar;
    dest->x = RD_MULTIPLY((src)->x, (invScalar));
    dest->y = RD_MULTIPLY((src)->y, (invScalar));
    dest->z = RD_MULTIPLY((src)->z, (invScalar));
}

void J3DAPI rdVector_InvScale4(rdVector4* dest, const rdVector4* src, float scalar)
{
    float invScalar = 1.0f / scalar;
    dest->x = RD_MULTIPLY((src)->x, (invScalar));
    dest->y = RD_MULTIPLY((src)->y, (invScalar));
    dest->z = RD_MULTIPLY((src)->z, (invScalar));
    dest->w = RD_MULTIPLY((src)->w, (invScalar));
}

void J3DAPI rdVector_InvScale2Acc(rdVector2* v, float scalar)
{
    v->x = v->x / scalar;
    v->y = v->y / scalar;
}

void J3DAPI rdVector_InvScale3Acc(rdVector3* v, float scalar)
{
    float invScalar = 1.0f / scalar;
    v->x = RD_MULTIPLY((v)->x, (invScalar));
    v->y = RD_MULTIPLY((v)->y, (invScalar));
    v->z = RD_MULTIPLY((v)->z, (invScalar));
}

void J3DAPI rdVector_InvScale4Acc(rdVector4* v, float scalar)
{
    float invScalar = 1.0f / scalar;
    v->x = RD_MULTIPLY((v)->x, (invScalar));
    v->y = RD_MULTIPLY((v)->y, (invScalar));
    v->z = RD_MULTIPLY((v)->z, (invScalar));
    v->w = RD_MULTIPLY((v)->w, (invScalar));
}

void J3DAPI rdVector_Rotate3(rdVector3* vec, const rdVector3* pivot, const rdVector3* pyr)
{
    rdMatrix34 mat;
    rdMatrix_BuildRotate34(&mat, pyr);
    rdMatrix_TransformVector34(vec, pivot, &mat);
}

void J3DAPI rdVector_Rotate3Acc(rdVector3* vec, const rdVector3* pyr)
{
    rdMatrix34 mat;
    rdMatrix_BuildRotate34(&mat, pyr);
    rdMatrix_TransformVector34Acc(vec, &mat);
}

void J3DAPI rdVector_ExtractAngle(const rdVector3* v1, rdVector3* pyr)
{
    pyr->z = 0.0f;
    pyr->x = stdMath_ArcSin3(v1->z);
    pyr->y = stdMath_ArcTan4(v1->y, v1->x);
}
