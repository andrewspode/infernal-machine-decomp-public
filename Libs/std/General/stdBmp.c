#include "std.h"
#include "stdBmp.h"
#include "stdColorApi.h"
#include "stdMemory.h"

#include <std/Win95/stdDisplay.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(stdBmp_countfit_1, 15)
/* end count-fit */

/* Retail diagnostics pass four variadic arguments, including unused zeros. */
#define BMP_TYPE 0x4D42

int J3DAPI stdBmp_WriteVBuffer(const char* pFilename, tVBuffer* pVBuffer)
{
    ColorInfo ciBmp = { STDCOLOR_RGB, 24, 8, 8, 8, 0, 8, 16, 0, 0, 0, 0, 0, 0 };
    DWORD imageSize;
    tFileHandle fh;
    size_t size;
    size_t nWritten;
    uint8_t* pRow;
    int rowIdx;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

#line 449
    STD_ASSERTREL(pFilename != NULL);
    STD_ASSERTREL(pVBuffer != NULL);

    if ( pVBuffer->rasterInfo.colorInfo.colorMode == STDCOLOR_PAL )
    {
        return 1;
    }

    /* Retail uses the source depth, even though the output rows are 24-bit. */
    imageSize = pVBuffer->rasterInfo.width * pVBuffer->rasterInfo.height * pVBuffer->rasterInfo.colorInfo.bpp / 8;

    memset(&fileHeader, 0, sizeof(fileHeader));

    fileHeader.bfType      = BMP_TYPE;
    fileHeader.bfOffBits   = sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfSize      = fileHeader.bfOffBits + imageSize;

    memset(&infoHeader, 0, sizeof(infoHeader));
    infoHeader.biSize          = sizeof(infoHeader);
    infoHeader.biWidth         = pVBuffer->rasterInfo.width;
    infoHeader.biHeight        = pVBuffer->rasterInfo.height;
    infoHeader.biPlanes        = 1;
    infoHeader.biBitCount      = 24;
    infoHeader.biCompression   = BI_RGB;
    infoHeader.biSizeImage     = imageSize;
    infoHeader.biXPelsPerMeter = 2834;
    infoHeader.biYPelsPerMeter = 2834;

    fh = std_pHS->pFileOpen(pFilename, "wb");
    if ( !fh )
    {
#line 510
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to open file '%s' for writing.\n", pFilename, 0, 0, 0);

        return 1;
    }

    size = sizeof(fileHeader);
    nWritten = std_pHS->pFileWrite(fh, &fileHeader, size);
    if ( nWritten != size )
    {
#line 519
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error attempting to write %d bytes to file '%s'.\n", size, pFilename, 0, 0);
        std_pHS->pFileClose(fh);
        return 1;
    }

    size = sizeof(infoHeader);
    nWritten = std_pHS->pFileWrite(fh, &infoHeader, size);
    if ( nWritten != size )
    {
#line 529
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error attempting to write %d bytes to file '%s'.\n", size, pFilename, 0, 0);

        std_pHS->pFileClose(fh);
        return 1;
    }

    size = 3 * pVBuffer->rasterInfo.width; // 3 - 24 bpp
#line 537
    pRow = (uint8_t*)STDMALLOC(size);
    if ( !pRow )
    {
        std_pHS->pFileClose(fh);
        return 1;
    }

    memset(pRow, 0, size);

    stdDisplay_VBufferLock(pVBuffer);
    for ( rowIdx = pVBuffer->rasterInfo.height - 1; rowIdx >= 0; rowIdx-- )
    {
        uint8_t* pSrc = &pVBuffer->pPixels[rowIdx * pVBuffer->rasterInfo.rowSize];
        stdColor_ColorConvertOneRow(
            pRow,
            &ciBmp,
            pSrc,
            &pVBuffer->rasterInfo.colorInfo,
            pVBuffer->rasterInfo.width,
            0,
            0
        );

        nWritten = std_pHS->pFileWrite(fh, pRow, size);
        if ( nWritten != 3 * pVBuffer->rasterInfo.width )
        {
#line 556
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error attempting to write %d bytes to file '%s'.\n", size, pFilename, 0, 0);
            /* Retail leaves the video buffer locked on a row-write failure. */
            stdMemory_Free(pRow);
            std_pHS->pFileClose(fh);
            return 1;
        }
    }

    stdMemory_Free(pRow);
    stdDisplay_VBufferUnlock(pVBuffer);
    std_pHS->pFileClose(fh);
    return 0;
}

HBITMAP J3DAPI stdBmp_Load(const char* pFilename)
{
    HBITMAP hbmp = NULL;
    HDC hdc;
    BITMAPFILEHEADER curBmpHeader;
    tFileHandle fh;
    void* pBits;
    BITMAPINFO bmi;
    size_t numRead;
    BITMAPINFOHEADER headerInfo;

    if ( !pFilename )
    {
        return NULL;
    }

    fh = std_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
#line 606
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid filename '%s'.\n", pFilename, 0, 0, 0);
        goto error;
    }

    numRead = std_pHS->pFileRead(fh, &curBmpHeader, sizeof(curBmpHeader));
    numRead += std_pHS->pFileRead(fh, &headerInfo, sizeof(headerInfo));
    if ( numRead != sizeof(curBmpHeader) + sizeof(headerInfo) )
    {
#line 633
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Cannot read File's header.\n", 0, 0, 0, 0);
        goto error;
    }

    if ( curBmpHeader.bfType != BMP_TYPE )
    {
#line 641
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: File's header does not contain 'BM' so its not a BMP file.\n", 0, 0, 0, 0);
        goto error;
    }

    if ( headerInfo.biSize != sizeof(BITMAPINFOHEADER) )
    {
#line 648
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: File's info header contains wrong header size.  The file may not be a BMP.\n", 0, 0, 0, 0);
        goto error;
    }

    if ( headerInfo.biBitCount != 24 )
    {
        switch ( headerInfo.biBitCount )
        {
            case 8:
#line 656
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "8 bpp BMP not yet supported.\n", 0, 0, 0, 0);
                break;
            case 16:
#line 657
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "16 bpp BMP not yet supported.\n", 0, 0, 0, 0);
                break;
            case 32:
#line 658
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "32 bpp BMP not yet supported.\n", 0, 0, 0, 0);
                break;
            default:
#line 661
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "This BMP file uses a bit-depth I do not support yet.\n", 0, 0, 0, 0);
                break;
        }

        goto error;
    }

    bmi.bmiHeader = headerInfo;
    bmi.bmiHeader.biSizeImage = bmi.bmiHeader.biWidth * bmi.bmiHeader.biHeight * 3;
    hdc = CreateCompatibleDC(NULL);
    if ( !hdc )
    {
        goto error;
    }

    hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if ( !hbmp )
    {
        goto error;
    }

    numRead = std_pHS->pFileRead(fh, pBits, bmi.bmiHeader.biSizeImage);
    if ( numRead != bmi.bmiHeader.biSizeImage )
    {
#line 681
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to read all the data from your BMP file.  It may be corrupted.\n", 0, 0, 0, 0);
        goto error;
    }

    DeleteDC(hdc);
    std_pHS->pFileClose(fh);
    return hbmp;

error:
    /* Retail failure cleanup closes only the file, including after GDI allocation. */
    if ( fh )
    {
        std_pHS->pFileClose(fh);
    }
    return NULL;
}
