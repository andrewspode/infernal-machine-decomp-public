#include "std.h"
#include "stdColorApi.h"

/* Linker-discarded (relocations.yml discarded:). Retail's .rdata 0x00509980-0x005099A8, between
   stdComm's aDPStatusTbl and wkernel's GUID (stdColor's link slot), is a 40-byte VC5 float pool
   that nothing in the image references: 1/255, 0.0, -2.0, -4.0, 60.0, -360.0, 255.0, 360.0,
   1/60, 1.0. It is the first-use pool of the debug build's RGBtoHSV followed by HSVtoRGB
   (the full bodies compile to the same 40 bytes; VC5 pools `x + 2.0f` as `fsub [-2.0]` and
   `x / 60.0f` as `fmul [1/60]`), and their lines fill the head gap before ColorConvertOneRow's
   first anchor (line 264). Name and signature inferred (Jedi Knight's stdColor.h declares
   both). Minimal placeholder bodies: only the pooled constants, in pool order, are evidenced.
   The full bodies (an OpenJKDF2-style reconstruction) are clues, not
   evidence; they compile to exactly retail's orphaned pool, and with them ColorConvertOneRow
   comes closer to retail. They are kept minimal because no retail evidence gives the
   body shape - bring them in only if evidence appears. Each statement below carries one pool
   constant and nothing else; the two comparisons are the only non-folded uses that pool 0.0
   and 360.0 (without the ifs, or with `x - 0.0f`, `x * 0.0f`, `0.0f - x` or an empty if body,
   both vanish and .rdata no longer matches), so the ifs stay.
   The locals (declarations only, no statements) are the minimal set Jedi
   Knight's retail bodies prove (JK.EXE 0x00433890 / 0x00433A50, the same 40-byte pool): values
   computed before a branch and used across it, which no CSE supplies. RGBtoHSV: max (a nested
   MAX used five times), delta (computed before the max == 0 test, used after the join), rc/gc/bc
   (homed in dead parameter slots, read in the if-chain arms). HSVtoRGB: the unsigned sector (one
   __ftol result feeding the switch and an unsigned conversion), p/q/t (homed before the switch,
   loaded by its cases). Names are OpenJKDF2's (inference). */
void J3DAPI stdColor_RGBtoHSV(float r, float g, float b, float* hue, float* saturation, float* value)
{
    float maxVal;
    float delta;
    float rc;
    float gc;
    float bc;

    *value = r * 0.003921569f; // PLACEHOLDER
    if ( *value == 0.0f )
    {
        *saturation = g + 2.0f;
    }

    *hue = b + 4.0f;
    *hue = *hue * 60.0f;
    *hue = *hue + 360.0f;
}

void J3DAPI stdColor_HSVtoRGB(float hue, float saturation, float value, float* r, float* g, float* b)
{
    unsigned int sector;
    float p;
    float q;
    float t;

    *r = value * 255.0f; // PLACEHOLDER
    if ( hue == 360.0f )
    {
        *g = saturation / 60.0f;
    }

    *b = 1.0f - saturation;
}

void J3DAPI stdColor_ColorConvertOneRow(uint8_t* pDestRow, const ColorInfo* pDestCI, const uint8_t* pSrcRow, const ColorInfo* pSrcCI, int width, int bColorKey, uint32_t colorKey)
{
    unsigned int pixel;
    int redDelta;
    int greenDelta;
    int blueDelta;
    int alphaDelta;
    int i;
    uint32_t r;
    uint32_t b;
    uint32_t g;
    uint32_t a;

    unsigned int redMask       = 0xFFFFFFFF >> (32 - pSrcCI->redBPP);
    unsigned int greenMask     = 0xFFFFFFFF >> (32 - pSrcCI->greenBPP);
    unsigned int blueMask      = 0xFFFFFFFF >> (32 - pSrcCI->blueBPP);
    unsigned int alphaMask;
    unsigned int maxAlphaValue;

    if ( pSrcCI->alphaBPP )
    {
        alphaMask = 0xFFFFFFFF >> (32 - pSrcCI->alphaBPP);
        maxAlphaValue = (255 >> pSrcCI->alphaPosShiftRight) / 2 > 1 ? (255 >> pSrcCI->alphaPosShiftRight) / 2 : 1;
    }

    redDelta = pSrcCI->redBPP - pDestCI->redBPP;
    greenDelta = pSrcCI->greenBPP - pDestCI->greenBPP;
    blueDelta = pSrcCI->blueBPP - pDestCI->blueBPP;
    if ( pSrcCI->alphaBPP )
    {
        alphaDelta = pSrcCI->alphaBPP - pDestCI->alphaBPP;
    }

    for ( i = 0; i < width; ++i )
    {
        switch ( pSrcCI->bpp )
        {
            case 8:
                pixel = *pSrcRow;
                break;

            case 16:
                pixel = *(uint16_t*)pSrcRow;
                break;

            /* Retail stores 24-bit pixels with the most significant byte first. */
            case 24:
                pixel = (*pSrcRow << 16) | (pSrcRow[1] << 8) | pSrcRow[2];
                break;

            case 32:
                pixel = *(uint32_t*)pSrcRow;
                break;

            default:
#line 264
                STDLOG_FATAL("Unsupported pixel depth.  Only 8, 16, 24, & 32 bits per pixel supported at the moment.");
                break;
        }

        r = (pixel >> pSrcCI->redPosShift) & redMask;
        g = (pixel >> pSrcCI->greenPosShift) & greenMask;
        b = (pixel >> pSrcCI->bluePosShift) & blueMask;
        if ( pSrcCI->alphaBPP )
        {
            a = (pixel >> pSrcCI->alphaPosShift) & alphaMask;
        }

        if ( redDelta > 0 )
        {
            r >>= redDelta;
        }
        else
        {
            r <<= -redDelta;
        }
        if ( greenDelta > 0 )
        {
            g >>= greenDelta;
        }
        else
        {
            g <<= -greenDelta;
        }
        /* Retail tests the red delta when choosing the blue shift direction. */
        if ( redDelta > 0 )
        {
            b >>= blueDelta;
        }
        else
        {
            b <<= -blueDelta;
        }

        pixel = (r << pDestCI->redPosShift) | (g << pDestCI->greenPosShift) | (b << pDestCI->bluePosShift);
        if ( pSrcCI->alphaBPP )
        {
            if ( bColorKey )
            {
                if ( a < maxAlphaValue ) {
                    pixel = colorKey;
                }
            }
            else
            {
                if ( alphaDelta > 0 )
                {
                    a >>= alphaDelta;
                }
                else
                {
                    a <<= -alphaDelta;
                }
                pixel |= a << pDestCI->alphaPosShift;
            }
        }

        switch ( pDestCI->bpp )
        {
            case 8:
                *pDestRow = (uint8_t)pixel;
                break;

            case 16:
                *(uint16_t*)pDestRow = (uint16_t)pixel;
                break;

            case 32:
                *(uint32_t*)pDestRow = pixel;
                break;

            case 24:
                pDestRow[0] = (pixel >> 16) & 0xFF;
                pDestRow[1] = (pixel >> 8) & 0xFF;
                pDestRow[2] = pixel & 0xFF;
                break;

            default:
#line 326
                STD_ASSERTREL(0);
                break;
        }

        pSrcRow += (unsigned int)pSrcCI->bpp >> 3;
        pDestRow += (unsigned int)pDestCI->bpp >> 3;
    }
}

#ifdef J3D_DEBUG
/* Debug-only as far as the evidence goes (debug 0x00503133, the object's last function; no caller in the debug image,
   no trace in retail): one pixel through ColorConvertOneRow. Name from Jedi Knight's stdColor; signature inferred from
   the debug code. */
uint32_t J3DAPI stdColor_ColorConvertOnePixel(const ColorInfo* pDestCI, uint32_t pixel, const ColorInfo* pSrcCI, int bColorKey, uint32_t colorKey)
{
    uint32_t result;

    stdColor_ColorConvertOneRow((uint8_t*)&result, pDestCI, (const uint8_t*)&pixel, pSrcCI, 1, bColorKey, colorKey);
    return result;
}
#endif
