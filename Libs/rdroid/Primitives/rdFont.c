#include "rdFont.h"

#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/stdMemory.h>
#include <std/Win95/stdDisplay.h>

#include <stdio.h>


/* One contiguous private .bss block at 0x005E1080: uninitialized statics, which VC5 emits in
   its symbol-hash order. These five spellings come out hash-ordered exactly as
   retail's addresses (buckets 333, 393, 549, 584, 850; a 1-in-120 coincidence otherwise), with
   the 4-byte hole at offset 4 being 8-byte alignment in front of the 64-byte array.
   */
static rdLightMode rdFont_lightingMode;
static rdFontColor rdFont_aFontColors;
static int rdFont_bStartup;
static int rdFont_bOpen;
static int rdFont_bFontColorSet;

void J3DAPI rdFont_DrawChar(uint8_t chr, float x, float y, float z, const rdFont* pFont);

/* rdFont's writer trio: retail's linker discarded all three (no retail symbol, address or caller), restored from the
   debug build: the glyph writer 0x50461a, the material writer 0x50475e and the public writer 0x504430,
   which the debug image emits in the order Write, glyph writer, material writer (a deferred static is emitted after its
   caller, as LoadEntry/LoadMaterial follow Load). Names and parameter names inferred (the counterparts of LoadEntry,
   LoadMaterial and Load). The glyph writer's first J3D_FILE use prints "Unable to write file" in retail's /Gf run
   (its open-failure message left with the function). */
static int J3DAPI rdFont_WriteEntry(const char* pFilename, const rdFont* pFont)
{
    size_t nWritten;
    tFileHandle fh;

    fh = rdroid_pHS->pFileOpen(pFilename, "wb+");
    if ( !fh )
    {
#line 45
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to open file '%s' for writing.\n", pFilename, 0, 0, 0);
        return -4;
    }

    nWritten = rdroid_pHS->pFileWrite(fh, &pFont->lineSpacing, sizeof(pFont->lineSpacing));
    if ( nWritten != sizeof(pFont->lineSpacing) )
    {
        goto write_error;
    }

    nWritten = rdroid_pHS->pFileWrite(fh, pFont->pName, 256);
    if ( nWritten != 256 )
    {
        goto write_error;
    }

    nWritten = rdroid_pHS->pFileWrite(fh, &pFont->fontSize, sizeof(pFont->fontSize));
    if ( nWritten != sizeof(pFont->fontSize) )
    {
        goto write_error;
    }

    nWritten = rdroid_pHS->pFileWrite(fh, pFont->aGlyphs, sizeof(rdGlyphMetrics) * 256);
    if ( nWritten != sizeof(rdGlyphMetrics) * 256 )
    {
        goto write_error;
    }

    rdroid_pHS->pFileClose(fh);
    return 0;

write_error:
    rdroid_pHS->pFileClose(fh);
#line 70
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to write file '%s'.\n", pFilename, 0, 0, 0);
    return -1;
}

static int J3DAPI rdFont_WriteMaterial(const char* pFilename, const rdMaterial* pMaterial, tVBuffer*** paTextures, size_t numMipLevels)
{
    if ( !rdMaterial_Write(pFilename, pMaterial, paTextures, 1) )
    {
        return 0;
    }

    return -1;
}

int J3DAPI rdFont_Write(const char* pFilename, const rdFont* pFont, tVBuffer*** paTextures, size_t numMipLevels)
{
    int bNotGcf;
    int bNotMat;
    char aGcfFilename[256];
    char aMatFilename[256];

    if ( !pFilename || !pFont->pName )
    {
        return -3;
    }

    bNotGcf = strcmp(&pFilename[strlen(pFilename) - 4], ".gcf");
    bNotMat = strcmp(&pFilename[strlen(pFilename) - 4], ".mat");
    if ( bNotGcf && bNotMat )
    {
        return -5;
    }

    memset(aGcfFilename, 0, sizeof(aGcfFilename));
    memset(aMatFilename, 0, sizeof(aMatFilename));

    if ( !bNotGcf )
    {
        strcpy(aGcfFilename, pFilename);
        strncpy(aMatFilename, pFilename, strlen(pFilename) - 3);
        strcat(aMatFilename, "mat");
    }
    else if ( !bNotMat )
    {
        strcpy(aMatFilename, pFilename);
        strncpy(aGcfFilename, pFilename, strlen(pFilename) - 3);
        strcat(aGcfFilename, "gcf");
    }

    if ( !rdFont_WriteEntry(aGcfFilename, pFont) && !rdFont_WriteMaterial(aMatFilename, pFont->pMaterial, paTextures, numMipLevels) )
    {
        return 0;
    }

    remove(aGcfFilename);
    remove(aMatFilename);
#line 125
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to write file '%s'.\n", pFilename, 0, 0, 0);
    return -1;
}

static int J3DAPI rdFont_LoadEntry(const char* pFilename, rdFont* pFont)
{
    size_t numRead;
    tFileHandle fh;

#line 136
    pFont->aGlyphs = (rdGlyphMetrics*)STDMALLOC(sizeof(rdGlyphMetrics) * 256);
    pFont->pName   = (char*)STDMALLOC(256);
    if ( !pFont->aGlyphs && !pFont->pName )
    {
        return -1;
    }

    fh = rdroid_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
#line 145
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to open file '%s'.\n", pFilename, 0, 0, 0);
        return -4;
    }

    numRead = rdroid_pHS->pFileRead(fh, &pFont->lineSpacing, sizeof(pFont->lineSpacing));
    if ( numRead != sizeof(pFont->lineSpacing) )
    {
        goto read_error;
    }

    numRead = rdroid_pHS->pFileRead(fh, pFont->pName, 256);
    if ( numRead != 256 )
    {
        goto read_error;
    }

    numRead = rdroid_pHS->pFileRead(fh, &pFont->fontSize, sizeof(pFont->fontSize));
    if ( numRead != sizeof(pFont->fontSize) )
    {
        goto read_error;
    }

    numRead = rdroid_pHS->pFileRead(fh, pFont->aGlyphs, sizeof(rdGlyphMetrics) * 256);
    if ( numRead != sizeof(rdGlyphMetrics) * 256 )
    {
        goto read_error;
    }

    rdroid_pHS->pFileClose(fh);
    return 0;

read_error:
    rdroid_pHS->pFileClose(fh);
#line 170
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unable to write file '%s'.\n", pFilename, 0, 0, 0);
    return -1;
}

static int J3DAPI rdFont_LoadMaterial(const char* pFilename, rdFont* pFont)
{
#line 177
    pFont->pMaterial = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
    if ( !pFont->pMaterial )
    {
#line 181
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s'\n", pFilename, 0, 0, 0);
        return -1;
    }

    return rdMaterial_LoadEntry(pFilename, pFont->pMaterial);
}

rdFont* J3DAPI rdFont_Load(const char* pFilename)
{
    rdFont* pFont;
    int bNotGcf;
    int bCurNotMat;
    char szGcfFilename[256];
    char szMatFilename[256];

#line 223
    pFont = (rdFont*)STDMALLOC(sizeof(rdFont));
    if ( !pFont || !pFilename )
    {
        return NULL;
    }

    memset(pFont, 0, sizeof(rdFont));

    bNotGcf = strcmp(&pFilename[strlen(pFilename) - 4], ".gcf");
    bCurNotMat = strcmp(&pFilename[strlen(pFilename) - 4], ".mat");
    if ( bNotGcf && bCurNotMat )
    {
        return NULL;
    }

    memset(szGcfFilename, 0, sizeof(szGcfFilename));
    memset(szMatFilename, 0, sizeof(szMatFilename));

    if ( !bNotGcf )
    {
        strcpy(szGcfFilename, pFilename);
        strncpy(szMatFilename, pFilename, strlen(pFilename) - 3);
        strcat(szMatFilename, "mat");
    }
    else if ( !bCurNotMat )
    {
        strcpy(szMatFilename, pFilename);
        strncpy(szGcfFilename, pFilename, strlen(pFilename) - 3);
        strcat(szGcfFilename, "gcf");
    }

    if ( !rdFont_LoadEntry(szGcfFilename, pFont) && !rdFont_LoadMaterial(szMatFilename, pFont) )
    {
        return pFont;
    }

    if ( pFont )
    {
        if ( pFont->pMaterial )
        {
            stdMemory_Free(pFont->pMaterial);
        }

        stdMemory_Free(pFont->aGlyphs);
        stdMemory_Free(pFont->pName);
        stdMemory_Free(pFont);
    }

    return NULL;
}

void J3DAPI rdFont_Free(rdFont* pFont)
{
    rdMaterial_FreeEntry(pFont->pMaterial);
    stdMemory_Free(pFont->pMaterial);
    stdMemory_Free(pFont->aGlyphs);
    stdMemory_Free(pFont->pName);
    stdMemory_Free(pFont);
}

// Linker-discarded (no retail symbol, no caller); the debug build's text (0x504bf9). Name inferred.
void J3DAPI rdFont_SetFontColorUniform(const rdVector4* pColor)
{
    int i;
    for ( i = 0; i < 4; ++i )
    {
        rdVector_Copy4(&rdFont_aFontColors[i], pColor);
    }

    rdFont_bFontColorSet = 1;
    rdFont_lightingMode  = RD_LIGHTING_DIFFUSE;
}

void J3DAPI rdFont_SetFontColor(const rdFontColor apColor)
{
    int i;
    for ( i = 0; i < 4; ++i )
    {
        rdVector_Copy4(&rdFont_aFontColors[i], &apColor[i]);
    }

    rdFont_bFontColorSet = 1;
    rdFont_lightingMode  = RD_LIGHTING_GOURAUD;
}

// Linker-discarded (no retail symbol, no caller); the debug build's text (0x504ca2, `__LINE__` 308). Name inferred.
rdVector4* J3DAPI rdFont_GetFontColor(void)
{
    rdVector4* aColors;
    int i;

#line 308
    aColors = (rdVector4*)STDMALLOC(sizeof(rdVector4) * 4);
    if ( !aColors )
    {
        return NULL;
    }

    for ( i = 0; i < 4; ++i )
    {
        rdVector_Copy4(&aColors[i], &rdFont_aFontColors[i]);
    }

    return aColors;
}

// Linker-discarded (no retail symbol, no caller); an empty function in the debug build (0x504d0d). Name inferred.
void J3DAPI rdFont_Update(void)
{
}

const char* J3DAPI rdFont_GetWrapLine(const char* pText, const rdFont* pFont, float widthScale)
{
    float width = 0.0f;
    const char* pCurrentLineEnd = NULL;
    int textLength = strlen(pText);
    int idx;
    uint8_t chr;

    if ( !pFont || !pText )
    {
        return NULL;
    }

    for ( idx = 0; idx < textLength; ++idx )
    {
        chr = pText[idx];
        if ( pFont->aGlyphs[chr].baselineOriginY == -pFont->fontSize )
        {
            chr = '?';
        }

        width += pFont->aGlyphs[chr].baselineOriginX;
        if ( width > (RD_REF_WIDTH * widthScale) )
        {
            if ( !pCurrentLineEnd )
            {
                return pText;
            }

            return pCurrentLineEnd + 1;
        }

        if ( chr == ' ' )
        {
            pCurrentLineEnd = &pText[idx];
        }
    }

    return NULL;
}

size_t J3DAPI rdFont_GetTextWidth(const char* pText, const rdFont* pFont)
{
    float width = 0.0f;
    const char* pCurrentLineEnd = NULL; // never read (the debug build stores it)
    int textLength = strlen(pText);
    int idx;
    int chr;

    if ( !pFont || !pText )
    {
        return 0;
    }

    for ( idx = 0; idx < textLength; ++idx )
    {
        chr = pText[idx];
        if ( pFont->aGlyphs[chr].baselineOriginY == -pFont->fontSize )
        {
            chr = '?';
        }

        width += pFont->aGlyphs[chr].baselineOriginX;
    }

    return (size_t)width;
}

void J3DAPI rdFont_DrawTextLine(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags)
{
    int textLen;
    int bAlignLeft;
    int bAlignRight;
    int bAlignCenter;
    uint32_t width;
    uint32_t height;
    float swidth;
    float sheight;
    float curPosX;
    float curPosY;
    float textWidth;
    float offsetX;
    float nextX;
    uint8_t chr;
    int i;

    textLen      = strlen(pText);
    bAlignLeft   = alignFlags & RDFONT_ALIGNLEFT;
    bAlignRight  = alignFlags & RDFONT_ALIGNRIGHT;
    bAlignCenter = alignFlags & RDFONT_ALIGNCENTER;
    if ( !pFont )
    {
        return;
    }

    if ( !(bAlignLeft ^ bAlignRight ^ bAlignCenter) )
    {
        return;
    }

    stdDisplay_GetBackBufferSize(&width, &height);
    swidth  = D3DVAL((float)width / RD_REF_WIDTH);
    sheight = D3DVAL((float)height / RD_REF_HEIGHT);
    curPosX = (float)width * x;
    curPosY = (float)height * y;
    curPosY = ((float)pFont->fontSize * sheight) + curPosY;

    if ( bAlignCenter || bAlignRight )
    {
        textWidth = 0.0f;
        for ( i = 0; i < textLen; ++i )
        {
            chr = pText[i];
            if ( pFont->aGlyphs[chr].baselineOriginY == -pFont->fontSize )
            {
                chr = '?';
            }

            textWidth += (float)pFont->aGlyphs[chr].baselineOriginX * swidth;
        }

        if ( bAlignCenter )
        {
            offsetX = textWidth / 2.0f;
        }

        if ( bAlignRight )
        {
            offsetX = textWidth;
        }

        curPosX = curPosX - (float)(int)offsetX;
    }

    for ( i = 0; i < textLen; ++i )
    {
        chr = pText[i];
        if ( chr == 160 ) // no-break space
        {
            chr = ' ';
        }
        else if ( pFont->aGlyphs[chr].baselineOriginY == -pFont->fontSize )
        {
            chr = '?';
        }

        if ( pFont->aGlyphs[chr].baselineOriginY != -1 )
        {
            nextX = ((float)pFont->aGlyphs[chr].baselineOriginX * swidth) + curPosX;
            if ( nextX >= 0.0f
                && nextX <= (RD_REF_WIDTH * swidth)
                && curPosX >= 0.0f
                && curPosX <= (RD_REF_WIDTH * swidth) )
            {
                rdFont_DrawChar(chr, curPosX, curPosY, z, pFont);
            }

            curPosX = ((float)pFont->aGlyphs[chr].baselineOriginX * swidth) + curPosX;
        }
    }
}

void J3DAPI rdFont_DrawChar(uint8_t chr, float x, float y, float z, const rdFont* pFont)
{
    rdCacheProcEntry* pPoly;
    float texHeight;
    float gheight;
    float texWidth;
    float gwidth;
    uint32_t width;
    uint32_t height;
    float swidth;
    float sheight;
    float ypos;
    LPD3DTLVERTEX pCurVert;
    size_t i;

    pPoly     = rdCache_GetAlphaProcEntry();
    texHeight = pFont->aGlyphs[chr].bottom - pFont->aGlyphs[chr].top;
    gheight   = texHeight * 256.0f;
    texWidth  = pFont->aGlyphs[chr].right - pFont->aGlyphs[chr].left;
    gwidth    = texWidth * 256.0f;
    if ( !pPoly )
    {
        return;
    }

    stdDisplay_GetBackBufferSize(&width, &height);
    swidth  = D3DVAL((float)width / RD_REF_WIDTH);
    sheight = D3DVAL((float)height / RD_REF_HEIGHT);

    ypos = y - D3DVAL(pFont->aGlyphs[chr].baselineOriginY) * sheight;

    pPoly->flags        = RD_FF_TEX_CLAMP_Y | RD_FF_TEX_CLAMP_X | RD_FF_TEX_TRANSLUCENT;
    pPoly->lightingMode = rdFont_lightingMode;
    pPoly->numVertices  = 4;
    pPoly->pMaterial    = pFont->pMaterial;
    pPoly->matCelNum    = 0;

    pCurVert = pPoly->aVertices;
    pCurVert->sx  = D3DVAL(x);
    pCurVert->sy  = D3DVAL(ypos);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X1);
    pCurVert->tu  = D3DVAL(pFont->aGlyphs[chr].left);
    pCurVert->tv  = D3DVAL(pFont->aGlyphs[chr].top);
    ++pCurVert;

    pCurVert->sx  = D3DVAL((gwidth * swidth) + x);
    pCurVert->sy  = D3DVAL(ypos);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X1);
    pCurVert->tu  = D3DVAL(pFont->aGlyphs[chr].right);
    pCurVert->tv  = D3DVAL(pFont->aGlyphs[chr].top);
    ++pCurVert;

    pCurVert->sx  = D3DVAL((gwidth * swidth) + x);
    pCurVert->sy  = D3DVAL((gheight * sheight) + ypos);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X1);
    pCurVert->tu  = D3DVAL(pFont->aGlyphs[chr].right);
    pCurVert->tv  = D3DVAL(pFont->aGlyphs[chr].bottom);
    ++pCurVert;

    pCurVert->sx  = D3DVAL(x);
    pCurVert->sy  = D3DVAL((gheight * sheight) + ypos);
    pCurVert->sz  = D3DVAL(z);
    pCurVert->rhw = D3DVAL(RD_FIXEDPOINT_RHW_SCALE_X1);
    pCurVert->tu  = D3DVAL(pFont->aGlyphs[chr].left);
    pCurVert->tv  = D3DVAL(pFont->aGlyphs[chr].bottom);

    if ( rdFont_bFontColorSet )
    {
        if ( rdFont_lightingMode == RD_LIGHTING_DIFFUSE )
        {
            rdVector_Copy4(&pPoly->extraLight, &rdFont_aFontColors[0]);
        }
        else
        {
            rdVector_Set4(&pPoly->extraLight, 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    for ( i = 0; i < pPoly->numVertices; ++i )
    {
        if ( rdFont_bFontColorSet )
        {
            if ( rdFont_lightingMode == RD_LIGHTING_GOURAUD )
            {
                rdVector_Copy4(&pPoly->aVertIntensities[i], &rdFont_aFontColors[i]);
            }
            else
            {
                rdVector_Set4(&pPoly->aVertIntensities[i], 0.0f, 0.0f, 0.0f, 1.0f);
            }
        }
    }

    rdCache_AddAlphaProcFace(4);
}

void J3DAPI rdFont_DrawTextLineClipped(const char* pText, float x, float y, float z, const rdFont* pFont, int alignFlags)
{
    int textLen;
    int bAlignLeft;
    int bAlignRight;
    int bAlignCenter;
    uint32_t width;
    uint32_t height;
    float swidth;
    float sheight;
    float curPosX;
    float curPosY;
    float textWidth;
    float offsetX;
    float nextX;
    uint8_t chr;
    int i;

    textLen      = strlen(pText);
    bAlignLeft   = alignFlags & RDFONT_ALIGNLEFT;
    bAlignRight  = alignFlags & RDFONT_ALIGNRIGHT;
    bAlignCenter = alignFlags & RDFONT_ALIGNCENTER;
    if ( !pFont )
    {
        return;
    }

    if ( !(bAlignLeft ^ bAlignRight ^ bAlignCenter) )
    {
        return;
    }

    stdDisplay_GetBackBufferSize(&width, &height);
    swidth  = D3DVAL((float)width / RD_REF_WIDTH);
    sheight = (float)height / RD_REF_HEIGHT;
    curPosX = (float)width * x;
    curPosY = (float)height * y;
    curPosY = ((float)pFont->fontSize * sheight) + curPosY;

    if ( bAlignCenter || bAlignRight )
    {
        textWidth = 0.0f;
        for ( i = 0; i < textLen; ++i )
        {
            chr = pText[i];
            if ( pFont->aGlyphs[chr].baselineOriginY == -pFont->fontSize )
            {
                chr = '?';
            }

            textWidth += (float)pFont->aGlyphs[chr].baselineOriginX * swidth;
        }

        if ( bAlignCenter )
        {
            offsetX = textWidth / 2.0f;
        }

        if ( bAlignRight )
        {
            offsetX = textWidth;
        }

        curPosX = curPosX - (float)(int)offsetX;
    }

    for ( i = 0; i < textLen; ++i )
    {
        chr = pText[i];
        if ( pFont->aGlyphs[chr].baselineOriginY == -pFont->fontSize )
        {
            chr = '?';
        }
        else
        {
            nextX = ((float)pFont->aGlyphs[chr].baselineOriginX * swidth) + curPosX;
            if ( nextX >= 0.0f
                && nextX <= (RD_REF_WIDTH * swidth)
                && curPosX >= 0.0f
                && curPosX <= (RD_REF_WIDTH * swidth)
                && curPosY >= 0.0f
                && curPosY <= (RD_REF_HEIGHT * sheight) )
            {
                rdFont_DrawChar(chr, curPosX, curPosY, z, pFont);
            }
        }

        curPosX = ((float)pFont->aGlyphs[chr].baselineOriginX * swidth) + curPosX;
    }
}

void J3DAPI rdFont_Startup(void)
{
    if ( rdFont_bStartup )
    {
#line 762
        stdPrintf(rdroid_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already initialized!\n", 0, 0, 0, 0);
    }

    rdFont_bStartup = 1;
}

void J3DAPI rdFont_Shutdown(void)
{
    if ( rdFont_bOpen )
    {
        rdFont_Close();
    }

    if ( !rdFont_bStartup )
    {
#line 776
        stdPrintf(rdroid_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already shutdown!\n", 0, 0, 0, 0);
        return;
    }

    rdFont_bStartup = 0;
}

int J3DAPI rdFont_Open(void)
{
    if ( rdFont_bOpen )
    {
#line 788
        stdPrintf(rdroid_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return -2;
    }

    rdFont_bOpen = 1;
    return 0;
}

void J3DAPI rdFont_Close(void)
{
    if ( !rdFont_bOpen )
    {
#line 801
        stdPrintf(rdroid_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: System already closed!\n", 0, 0, 0, 0);
        return;
    }

    rdFont_bOpen = 0;
}
