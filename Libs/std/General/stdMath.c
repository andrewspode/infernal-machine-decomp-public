#include "std.h"
#include "stdMath.h"
#include "stdMathTables.h"

// Tentative definitions (COMMON requests) of the x87 control words the stdMath.h rounding helpers
// load and std.c's stdStartup prepares. Retail's common tail places them as the last std-batch
// encounter, after stdDisplay's common: first emitted by a std object pulled after stdDisplay,
// and stdMath.c is the only such object that references them
// (the COMMON tail).
int16_t word_183962C;
int16_t word_183962E;
int16_t word_1839630;

// The floorf and sqrtf helpers are defined in stdMath.h. The rest of the file
// is unoptimized, so its calls use their out-of-line copies (0x004C1340 and
// 0x004C1AF0), which are compiled with the default /O2 flags (0x90 fill).

// Every stdMath function is unoptimized in retail (EBP frames, stack homes for
// all temporaries, 0xCC fill) while inline expansion stays enabled: each call
// to a helper above reserves a slot for the helper's local `r` without
// inlining it. A scoped optimize pragma reproduces both (jonesInventory and
// sithAnimate precedent); a whole-file /Od does not reserve the slots.
#pragma optimize("", off)

// Helper macros for table lookups
#define SIN_TABLE_SIZE STD_ARRAYLEN(stdMath_aSinTable)

#define SIN_TABLE_LAST_IDX (SIN_TABLE_SIZE - 1)
#define SIN_TABLE_GET(idx) stdMath_aSinTable[((idx) )]
#define SIN_TABLE_GET_REVERSE(idx) stdMath_aSinTable[SIN_TABLE_LAST_IDX - ((idx) % SIN_TABLE_SIZE)]

#define TAN_TABLE_SIZE STD_ARRAYLEN(stdMath_aTanTable)

#define TAN_TABLE_LAST_IDX (TAN_TABLE_SIZE - 1)
#define TAN_TABLE_GET(idx) stdMath_aTanTable[(idx) % TAN_TABLE_SIZE]
#define TAN_TABLE_GET_REVERSE(idx) stdMath_aTanTable[TAN_TABLE_LAST_IDX - ((idx) % TAN_TABLE_SIZE)]

float J3DAPI stdMath_FlexPower(float base, int exponent)
{
    float ret = base;
    int i;
    for ( i = 0; i < exponent - 1; ++i ) {
        ret = ret * base;
    }
    return ret;
}

float J3DAPI stdMath_NormalizeAngle(float angle)
{
    float absAngle;
    float normAngle;
    float rounded;

    if ( angle < 0.0f )
    {
        absAngle = -angle;
        if ( absAngle < 360.0f ) {
            normAngle = 360.0f - absAngle;
        }
        else {
            rounded = floorf(absAngle / 360.0f);
            normAngle = 360.0f - (absAngle - rounded * 360.0f);
        }
    }
    else
    {
        if ( angle < 360.0f ) {
            return angle;
        }
        rounded = floorf(angle / 360.0f);
        normAngle = angle - rounded * 360.0f;
    }

    if ( normAngle == 360.0f ) {
        normAngle = 0.0f;
    }

    return normAngle;
}

float J3DAPI stdMath_NormalizeAngleAcute(float angle)
{
    float normAngle = stdMath_NormalizeAngle(angle);
    if ( normAngle > 180.0f ) {
        normAngle = -(360.0f - normAngle);
    }
    return normAngle;
}

float J3DAPI stdMath_NormalizeDeltaAngle(float angle1, float angle2)
{
    float delta = stdMath_NormalizeAngle(angle2) - stdMath_NormalizeAngle(angle1);
    if ( delta < 0.0f )
    {
        if ( delta >= -180.0f ) {
            return delta;
        }
        return 360.0f + delta;
    }

    if ( delta <= 180.0f ) {
        return delta;
    }
    return -(360.0f - delta);
}

void J3DAPI stdMath_SinCos(float angle, float* pSinOut, float* pCosOut)
{
    float scaledAngle;
    int32_t quadrant;
    float baseLookup;
    int32_t index;
    float blend;
    int32_t bWrapAround;
    int32_t nextIndex;
    float normalized;
    float nextLookup;

    normalized = stdMath_NormalizeAngle(angle);
    if ( normalized < 90.0f ) {
        quadrant = 0;
    }
    else if ( normalized < 180.0f ) {
        quadrant = 1;
    }
    else if ( normalized < 270.0f ) {
        quadrant = 2;
    }
    else {
        quadrant = 3;
    }

    scaledAngle = normalized * 45.511112f;
    blend = scaledAngle - floorf(scaledAngle);
    index = (int32_t)scaledAngle;
    nextIndex = index + 1;

    switch ( quadrant )
    {
        case 0:
            if ( nextIndex >= 0x1000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = stdMath_aSinTable[index];
            if ( bWrapAround )
                nextLookup = stdMath_aSinTable[0xFFF - (nextIndex - 0x1000)];
            else
                nextLookup = stdMath_aSinTable[nextIndex];
            *pSinOut = (nextLookup - baseLookup) * blend + baseLookup;

            baseLookup = stdMath_aSinTable[0xFFF - index];
            if ( bWrapAround )
                nextLookup = -stdMath_aSinTable[nextIndex - 0x1000];
            else
                nextLookup = stdMath_aSinTable[0xFFF - nextIndex];
            *pCosOut = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        case 1:
            if ( nextIndex >= 0x2000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = stdMath_aSinTable[0xFFF - (index - 0x1000)];
            if ( bWrapAround )
                nextLookup = -stdMath_aSinTable[nextIndex - 0x2000];
            else
                nextLookup = stdMath_aSinTable[0xFFF - (nextIndex - 0x1000)];
            *pSinOut = (nextLookup - baseLookup) * blend + baseLookup;

            baseLookup = -stdMath_aSinTable[index - 0x1000];
            if ( bWrapAround )
                nextLookup = -stdMath_aSinTable[0xFFF - (nextIndex - 0x2000)];
            else
                nextLookup = -stdMath_aSinTable[nextIndex - 0x1000];
            *pCosOut = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        case 2:
            if ( nextIndex >= 0x3000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = -stdMath_aSinTable[index - 0x2000];
            if ( bWrapAround )
                nextLookup = -stdMath_aSinTable[0xFFF - (nextIndex - 0x3000)];
            else
                nextLookup = -stdMath_aSinTable[nextIndex - 0x2000];
            *pSinOut = (nextLookup - baseLookup) * blend + baseLookup;

            baseLookup = -stdMath_aSinTable[0xFFF - (index - 0x2000)];
            if ( bWrapAround )
                nextLookup = stdMath_aSinTable[nextIndex - 0x3000];
            else
                nextLookup = -stdMath_aSinTable[0xFFF - (nextIndex - 0x2000)];
            *pCosOut = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        case 3:
            if ( nextIndex >= 0x4000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = -stdMath_aSinTable[0xFFF - (index - 0x3000)];
            if ( bWrapAround )
                nextLookup = stdMath_aSinTable[nextIndex - 0x4000];
            else
                nextLookup = -stdMath_aSinTable[0xFFF - (nextIndex - 0x3000)];
            *pSinOut = (nextLookup - baseLookup) * blend + baseLookup;

            baseLookup = stdMath_aSinTable[index - 0x3000];
            if ( bWrapAround )
                nextLookup = stdMath_aSinTable[0xFFF - (nextIndex - 0x4000)];
            else
                nextLookup = stdMath_aSinTable[nextIndex - 0x3000];
            *pCosOut = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        default:
            return;
    }
}

float J3DAPI stdMath_Tan(float angle)
{
    float scaledAngle;
    int32_t quadrant;
    float baseLookup;
    int32_t index;
    float blend;
    int32_t bWrapAround;
    float tanValue;
    int32_t nextIndex;
    float nextLookup;

    angle = stdMath_NormalizeAngle(angle);
    if ( angle < 90.0f ) {
        quadrant = 0;
    }
    else if ( angle < 180.0f ) {
        quadrant = 1;
    }
    else if ( angle < 270.0f ) {
        quadrant = 2;
    }
    else {
        quadrant = 3;
    }

    scaledAngle = (angle / 360.0f) * 16384.0f;
    blend = scaledAngle - floorf(scaledAngle);
    index = (int32_t)scaledAngle;
    nextIndex = index + 1;

    // Retail has no default case: an out-of-range quadrant returns the
    // uninitialized tanValue (quadrant is always 0..3 here).
    switch ( quadrant )
    {
        case 0:
            if ( nextIndex >= 0x1000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = stdMath_aTanTable[index];
            if ( bWrapAround )
                nextLookup = -stdMath_aTanTable[0xFFF - (nextIndex - 0x1000)];
            else
                nextLookup = stdMath_aTanTable[nextIndex];
            tanValue = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        case 1:
            if ( nextIndex >= 0x2000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = -stdMath_aTanTable[0xFFF - (index - 0x1000)];
            if ( bWrapAround )
                nextLookup = stdMath_aTanTable[nextIndex - 0x2000];
            else
                nextLookup = -stdMath_aTanTable[0xFFF - (nextIndex - 0x1000)];
            tanValue = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        case 2:
            if ( nextIndex >= 0x3000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = stdMath_aTanTable[index - 0x2000];
            if ( bWrapAround )
                nextLookup = -stdMath_aTanTable[0xFFF - (nextIndex - 0x3000)];
            else
                nextLookup = stdMath_aTanTable[nextIndex - 0x2000];
            tanValue = (nextLookup - baseLookup) * blend + baseLookup;
            break;

        case 3:
            if ( nextIndex >= 0x4000 )
                bWrapAround = 1;
            else
                bWrapAround = 0;
            baseLookup = -stdMath_aTanTable[0xFFF - (index - 0x3000)];
            if ( bWrapAround )
                nextLookup = stdMath_aTanTable[nextIndex - 0x4000];
            else
                nextLookup = -stdMath_aTanTable[0xFFF - (nextIndex - 0x3000)];
            tanValue = (nextLookup - baseLookup) * blend + baseLookup;
            break;
    }

    return tanValue;
}

float J3DAPI stdMath_ArcSin1(float num)
{
    int numNegative;
    float asinval;

    numNegative = num < 0.0f;
    num = num < 0.0f ? -num : num;
    if ( num > 0.70710677f )
    {
        num = sqrtf(1.0f - num * num);
        asinval = 90.0f - STDMATH_RAD2DEG(num + stdMath_FlexPower(num, 3) * 0.212749f);
    }
    else
    {
        asinval = STDMATH_RAD2DEG(num + stdMath_FlexPower(num, 3) * 0.212749f);
    }

    if ( numNegative ) {
        return -asinval;
    }
    return asinval;
}

float J3DAPI stdMath_ArcSin2(float num)
{
    int numNegative;
    float asinval;

    numNegative = num < 0.0f;
    num = num < 0.0f ? -num : num;
    if ( num > 0.70710677f )
    {
        num = sqrtf(1.0f - num * num);
        asinval = 90.0f - STDMATH_RAD2DEG(num + stdMath_FlexPower(num, 3) / 6.0f + stdMath_FlexPower(num, 5) * 0.105502f);
    }
    else
    {
        asinval = STDMATH_RAD2DEG(num + stdMath_FlexPower(num, 3) / 6.0f + stdMath_FlexPower(num, 5) * 0.105502f);
    }

    if ( numNegative ) {
        return -asinval;
    }
    return asinval;
}

float J3DAPI stdMath_ArcSin3(float num)
{
    int numNegative;
    float asinval;

    numNegative = num < 0.0f;
    num = num < 0.0f ? -num : num;
    if ( num > 0.70710677f )
    {
        num = sqrtf(1.0f - num * num);
        asinval = 90.0f
                - STDMATH_RAD2DEG(num + stdMath_FlexPower(num, 3) / 6.0f + stdMath_FlexPower(num, 5) * 0.075f
                   + stdMath_FlexPower(num, 7) * 0.066797f);
    }
    else
    {
        asinval = STDMATH_RAD2DEG(num + stdMath_FlexPower(num, 3) / 6.0f + stdMath_FlexPower(num, 5) * 0.075f
                   + stdMath_FlexPower(num, 7) * 0.066797f);
    }

    if ( numNegative ) {
        return -asinval;
    }
    return asinval;
}

float J3DAPI stdMath_ArcTan1(float x, float y)
{
    float tanRatio;
    float angle;
    int yNegative;
    int xNegative;

    if ( y == 0.0f && x == 0.0f )
        return 0.0f;

    xNegative = x < 0.0f;
    x = x < 0.0f ? -x : x;
    yNegative = y < 0.0f;
    y = y < 0.0f ? -y : y;

    if ( y > x ) {
        tanRatio = x / y;
    }
    else {
        tanRatio = y / x;
    }
    tanRatio = tanRatio < 0.0f ? -tanRatio : tanRatio;

    angle = STDMATH_RAD2DEG(tanRatio - stdMath_FlexPower(tanRatio, 3) * 0.22629f);

    if ( x >= y ) {
        angle = 90.0f - angle;
    }

    angle = 90.0f - angle;
    if ( xNegative ) {
        angle = 180.0f - angle;
    }

    if ( !yNegative ) {
        angle = -angle;
    }

    return angle;
}

float J3DAPI stdMath_ArcTan2(float x, float y)
{
    float tanRatio;
    float angle;
    int yNegative;
    int xNegative;

    if ( y == 0.0f && x == 0.0f )
        return 0.0f;

    xNegative = x < 0.0f;
    x = x < 0.0f ? -x : x;
    yNegative = y < 0.0f;
    y = y < 0.0f ? -y : y;

    if ( y > x ) {
        tanRatio = x / y;
    }
    else {
        tanRatio = y / x;
    }
    tanRatio = tanRatio < 0.0f ? -tanRatio : tanRatio;

    angle = STDMATH_RAD2DEG(tanRatio - stdMath_FlexPower(tanRatio, 3) / 3.0f + stdMath_FlexPower(tanRatio, 5) * 0.12366f);

    if ( x >= y ) {
        angle = 90.0f - angle;
    }

    angle = 90.0f - angle;
    if ( xNegative ) {
        angle = 180.0f - angle;
    }

    if ( !yNegative ) {
        angle = -angle;
    }

    return angle;
}

float J3DAPI stdMath_ArcTan3(float x, float y)
{
    float tanRatio;
    float angle;
    int yNegative;
    int xNegative;

    if ( y == 0.0f && x == 0.0f )
        return 0.0f;

    xNegative = x < 0.0f;
    x = x < 0.0f ? -x : x;
    yNegative = y < 0.0f;
    y = y < 0.0f ? -y : y;

    if ( y > x ) {
        tanRatio = x / y;
    }
    else {
        tanRatio = y / x;
    }
    tanRatio = tanRatio < 0.0f ? -tanRatio : tanRatio;

    angle = STDMATH_RAD2DEG(tanRatio - stdMath_FlexPower(tanRatio, 3) / 3.0f + stdMath_FlexPower(tanRatio, 5) / 5.0f
             - stdMath_FlexPower(tanRatio, 7) * 0.08392f);

    if ( x >= y ) {
        angle = 90.0f - angle;
    }

    angle = 90.0f - angle;
    if ( xNegative ) {
        angle = 180.0f - angle;
    }

    if ( !yNegative ) {
        angle = -angle;
    }

    return angle;
}

float J3DAPI stdMath_ArcTan4(float x, float y)
{
    float tanRatio;
    float angle;
    int yNegative;
    int xNegative;

    if ( y == 0.0f && x == 0.0f )
        return 0.0f;

    xNegative = x < 0.0f;
    x = x < 0.0f ? -x : x;
    yNegative = y < 0.0f;
    y = y < 0.0f ? -y : y;

    if ( y > x ) {
        tanRatio = x / y;
    }
    else {
        tanRatio = y / x;
    }
    tanRatio = tanRatio < 0.0f ? -tanRatio : tanRatio;

    angle = STDMATH_RAD2DEG(tanRatio - stdMath_FlexPower(tanRatio, 3) / 3.0f + stdMath_FlexPower(tanRatio, 5) / 5.0f
             - stdMath_FlexPower(tanRatio, 7) / 7.0f + stdMath_FlexPower(tanRatio, 9) * 0.063235f);

    if ( x >= y ) {
        angle = 90.0f - angle;
    }

    angle = 90.0f - angle;
    if ( xNegative ) {
        angle = 180.0f - angle;
    }

    if ( !yNegative ) {
        angle = -angle;
    }

    return angle;
}

void J3DAPI stdMath_FloorDivMod(int dividend, int divisor, int* pOutDiv, int* pOutMod)
{
    int div;
    int mod;

    if ( dividend >= 0 )
    {
        div = dividend / divisor;
        mod = dividend % divisor;
    }
    else
    {
        div = -(-dividend / divisor);
        mod = -dividend % divisor;
        if ( mod )
        {
            --div;
            mod = divisor - mod;
        }
    }

    *pOutDiv = div;
    *pOutMod = mod;
}

float J3DAPI stdMath_Dist2D1(float x, float y)
{
    float max;
    float min;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    max = x > y ? x : y;
    min = x < y ? x : y;
    return min / 2.0f + max;
}

float J3DAPI stdMath_Dist2D2(float x, float y)
{
    float max;
    float min;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    max = x > y ? x : y;
    min = x < y ? x : y;
    return min / 4.0f + max;
}

float J3DAPI stdMath_Dist2D3(float x, float y)
{
    float max;
    float min;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    max = x > y ? x : y;
    min = x < y ? x : y;
    return min * 0.375f + max;
}

float J3DAPI stdMath_Dist2D4(float x, float y)
{
    float max;
    float min;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    max = x > y ? x : y;
    min = x < y ? x : y;
    return max * 0.875f + min / 2.0f < max ? max : max * 0.875f + min / 2.0f;
}

float J3DAPI stdMath_Dist3D1(float x, float y, float z)
{
    float min;
    float max;
    float mid;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    z = z < 0.0f ? -z : z;

    if ( x > y )
    {
        if ( x > z )
        {
            max = x;
            if ( y > z )
            {
                mid = y;
                min = z;
            }
            else
            {
                mid = z;
                min = y;
            }
        }
    }
    else if ( y > z )
    {
        max = y;
        if ( x > z )
        {
            mid = x;
            min = z;
        }
        else
        {
            mid = z;
            min = x;
        }
    }

    return mid / 2.0f + max + min / 2.0f;
}

float J3DAPI stdMath_Dist3D2(float x, float y, float z)
{
    float min;
    float max;
    float mid;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    z = z < 0.0f ? -z : z;

    if ( x > y )
    {
        if ( x > z )
        {
            max = x;
            if ( y > z )
            {
                mid = y;
                min = z;
            }
            else
            {
                mid = z;
                min = y;
            }
        }
    }
    else if ( y > z )
    {
        max = y;
        if ( x > z )
        {
            mid = x;
            min = z;
        }
        else
        {
            mid = z;
            min = x;
        }
    }

    return 0.3125f * mid + max + min / 2.0f;
}

float J3DAPI stdMath_Dist3D3(float x, float y, float z)
{
    float min;
    float max;
    float mid;

    x = x < 0.0f ? -x : x;
    y = y < 0.0f ? -y : y;
    z = z < 0.0f ? -z : z;

    if ( x > y )
    {
        if ( x > z )
        {
            max = x;
            if ( y > z )
            {
                mid = y;
                min = z;
            }
            else
            {
                mid = z;
                min = y;
            }
        }
    }
    else if ( y > z )
    {
        max = y;
        if ( x > z )
        {
            mid = x;
            min = z;
        }
        else
        {
            mid = z;
            min = x;
        }
    }

    return 0.34375f * mid + max + min / 2.0f;
}

#pragma optimize("", on)
