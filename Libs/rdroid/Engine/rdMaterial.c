#include "rdMaterial.h"

#include <rdroid/Main/rdroid.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/std3D.h>
#include <std/Win95/stdDisplay.h>

#define RDMAT_MAGIC        "MAT "
#define RDMAT_VERSION      50
#define RDMAT_TYPE_TEXTURE 2

rdMaterialLoaderFunc rdMaterial_pfMaterialLoader = 0;
rdMaterialUnloaderFunc rdMaterial_pfMaterialUnloader = 0;

rdMaterialLoaderFunc J3DAPI rdMaterial_RegisterLoader(rdMaterialLoaderFunc pFunc)
{
    rdMaterialLoaderFunc pPrevFunc = rdMaterial_pfMaterialLoader;
    rdMaterial_pfMaterialLoader = pFunc;
    return pPrevFunc;
}

/* Linker-discarded: retail reads and calls rdMaterial_pfMaterialUnloader (rdMaterial_Free) but no retail code writes
   it, so its setter existed and was discarded. Jedi Knight's retail has rdMaterial_RegisterUnloader (0x0044A120) right
   after RegisterLoader (0x0044A110); Indy's debug build keeps it there (0x0050BBAC). */
rdMaterialUnloaderFunc J3DAPI rdMaterial_RegisterUnloader(rdMaterialUnloaderFunc pFunc)
{
    rdMaterialUnloaderFunc pPrevFunc = rdMaterial_pfMaterialUnloader;
    rdMaterial_pfMaterialUnloader = pFunc;
    return pPrevFunc;
}

rdMaterial* J3DAPI rdMaterial_Load(const char* pFilename)
{
    rdMaterial* pMaterial;

    if ( rdMaterial_pfMaterialLoader )
    {
        return rdMaterial_pfMaterialLoader(pFilename);
    }

#line 127
    pMaterial = (rdMaterial*)STDMALLOC(sizeof(rdMaterial));
    if ( !pMaterial )
    {
        goto alloc_error;
    }

    if ( !rdMaterial_LoadEntry(pFilename, pMaterial) )
    {
        return pMaterial;
    }

    // Failed to load material
    goto error;

alloc_error:
#line 139
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s'\n", pFilename, 0, 0, 0);

error:
    rdMaterial_Free(pMaterial);
    return NULL;
}

int J3DAPI rdMaterial_LoadEntry(const char* pFilename, rdMaterial* pMat)
{
    tFileHandle fh = 0;
    rdMatHeader header;
    ColorInfo ci;
    int bColorKey;
    uint32_t colorKey;
    rdMatCelInfo celInfo;
    rdMatTextureCelInfo texcelInfo;
    rdMatTextureHeader texHeader;
    tRasterInfo rasterInfo;
    tVBuffer** apVBuffers;
    size_t i;
    int j;

    // Clear input mat
    memset(pMat, 0, sizeof(rdMaterial));

    fh = rdroid_pHS->pFileOpen(pFilename, "rb");
    if ( !fh )
    {
        goto open_error;
    }

    if ( rdroid_pHS->pFileRead(fh, &header, sizeof(rdMatHeader)) != sizeof(rdMatHeader) )
    {
        goto error;
    }

    if ( strncmp(header.magic, RDMAT_MAGIC, 4) )
    {
#line 213
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad header for '%s'\n", pFilename, 0, 0, 0);
        goto format_error;
    }

    if ( header.version != RDMAT_VERSION )
    {
#line 220
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad version %d for '%s'\n", header.version, pFilename, 0, 0);
        goto format_error;
    }

    if ( header.numCels != header.numTextures )
    {
#line 225
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Old style mat for '%s'\n", pFilename, 0, 0, 0);
        goto format_error;
    }

    pMat->numCels    = header.numCels;
    pMat->formatType = std3D_GetColorFormat(&header.colorInfo);
    std3D_GetTextureFormat(pMat->formatType, &ci, &bColorKey, &colorKey);

    for ( i = 0; i < pMat->numCels; ++i )
    {
        if ( rdroid_pHS->pFileRead(fh, &celInfo, sizeof(rdMatCelInfo)) != sizeof(rdMatCelInfo) )
        {
            goto error;
        }

        if ( (celInfo.type & 0x8) && rdroid_pHS->pFileRead(fh, &texcelInfo, sizeof(rdMatTextureCelInfo)) != sizeof(rdMatTextureCelInfo) )
        {
            goto error;
        }
    }

    pMat->aTextures = NULL;
    if ( pMat->numCels )
    {
#line 257
        pMat->aTextures = (tSystemTexture*)STDMALLOC(sizeof(tSystemTexture) * pMat->numCels);
        if ( !pMat->aTextures )
        {
            goto alloc_error;
        }

        memset(pMat->aTextures, 0, sizeof(tSystemTexture) * pMat->numCels);
    }

    // Read in cel textures
    for ( i = 0; i < pMat->numCels; ++i )
    {
        if ( rdroid_pHS->pFileRead(fh, &texHeader, sizeof(rdMatTextureHeader)) != sizeof(rdMatTextureHeader) )
        {
            goto error;
        }

        pMat->width  = texHeader.width;
        pMat->height = texHeader.height;

        rasterInfo.width     = texHeader.width;
        rasterInfo.height    = texHeader.height;
        rasterInfo.colorInfo = header.colorInfo;

        apVBuffers = NULL;
        if ( texHeader.numMipLevels )
        {
#line 287
            apVBuffers = (tVBuffer**)STDMALLOC(sizeof(tVBuffer*) * texHeader.numMipLevels);
            if ( !apVBuffers )
            {
                goto alloc_error;
            }

            memset(apVBuffers, 0, sizeof(tVBuffer*) * texHeader.numMipLevels);
        }

        // Read in cel mipmaps
        for ( j = 0; j < texHeader.numMipLevels; ++j )
        {
            apVBuffers[j] = stdDisplay_VBufferNew(&rasterInfo, 0, 0);
            if ( !apVBuffers[j] )
            {
                goto alloc_error;
            }

            stdDisplay_VBufferLock(apVBuffers[j]);
            if ( apVBuffers[j]->rasterInfo.size != rdroid_pHS->pFileRead(fh, apVBuffers[j]->pPixels, apVBuffers[j]->rasterInfo.size) )
            {
                goto error;
            }

            stdDisplay_VBufferUnlock(apVBuffers[j]);
            if ( apVBuffers[j]->rasterInfo.colorInfo.colorMode && std3D_GetNumTextureFormats() > 0 )
            {
                apVBuffers[j] = stdDisplay_VBufferConvertColorFormat(&ci, apVBuffers[j], bColorKey, colorKey);
            }

            rasterInfo.width  >>= 1;
            rasterInfo.height >>= 1;
        }

        std3D_AllocSystemTexture(&pMat->aTextures[i], apVBuffers, texHeader.numMipLevels, pMat->formatType);
        for ( j = 0; j < texHeader.numMipLevels; ++j )
        {
            stdDisplay_VBufferFree(apVBuffers[j]);
        }

        stdMemory_Free(apVBuffers);
    }

    if ( header.type & 1 )
    {
        goto error;
    }

    strncpy(pMat->aName, stdFileFromPath(pFilename), STD_ARRAYLEN(pMat->aName) - 1);
    pMat->aName[STD_ARRAYLEN(pMat->aName) - 1] = 0;
    rdroid_pHS->pFileClose(fh);
    return 0;

open_error:
#line 341
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid load filename '%s'.\n", pFilename, 0, 0, 0);
    goto error;

format_error:
#line 346
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid format for '%s'\n", pFilename, 0, 0, 0);
    goto error;

alloc_error:
#line 351
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s'\n", pFilename, 0, 0, 0);

error:
    if ( fh )
    {
        rdroid_pHS->pFileClose(fh);
    }

    return 1;
}

void J3DAPI rdMaterial_Free(rdMaterial* pMaterial)
{
#line 373
    RD_ASSERTREL(pMaterial!=NULL);
    if ( rdMaterial_pfMaterialUnloader )
    {
        rdMaterial_pfMaterialUnloader(pMaterial);
    }
    else
    {
        rdMaterial_FreeEntry(pMaterial);
        stdMemory_Free(pMaterial);
    }
}

void J3DAPI rdMaterial_FreeEntry(rdMaterial* pMaterial)
{
    tSystemTexture* aTextures;
    size_t idx;

#line 409
    RD_ASSERTREL(pMaterial!=NULL);

    aTextures = pMaterial->aTextures;
    if ( aTextures )
    {
        for ( idx = 0; idx < pMaterial->numCels; ++idx ) {
            std3D_ClearSystemTexture(&aTextures[idx]);
        }
    }

    if ( pMaterial->aTextures )
    {
        stdMemory_Free(pMaterial->aTextures);
    }

    memset(pMaterial, 0, sizeof(rdMaterial));
}

int J3DAPI rdMaterial_Write(const char* pFilename, const rdMaterial* pMaterial, tVBuffer*** paTextures, size_t numMipLevels)
{
    tFileHandle fh = 0;
    rdMatHeader header;
    rdMatCelInfo celInfo;
    rdMatTextureCelInfo texcelInfo;
    rdMatTextureHeader texHeader;
    size_t i;
    size_t j;
    size_t k;

    strncpy(header.magic, RDMAT_MAGIC, 4);
    header.version     = RDMAT_VERSION;
    header.type        = RDMAT_TYPE_TEXTURE;
    header.numCels     = pMaterial->numCels;
    header.numTextures = pMaterial->numCels;
    memcpy(&header.colorInfo, &(**paTextures)->rasterInfo.colorInfo, sizeof(ColorInfo));

    fh = rdroid_pHS->pFileOpen(pFilename, "wb+");
    if ( !fh )
    {
        goto open_error;
    }

    if ( rdroid_pHS->pFileWrite(fh, &header, sizeof(rdMatHeader)) != sizeof(rdMatHeader) )
    {
        goto error;
    }

    celInfo.type     = 8;
    celInfo.colorIdx = 0;
    celInfo.unknown1 = 0;
    celInfo.unknown2 = 0;
    celInfo.unknown3 = 0;
    celInfo.unknown4 = 1.0f;

    for ( i = 0; i < pMaterial->numCels; ++i )
    {
        if ( rdroid_pHS->pFileWrite(fh, &celInfo, sizeof(rdMatCelInfo)) != sizeof(rdMatCelInfo) )
        {
            goto error;
        }

        texcelInfo.unknown0 = 0;
        texcelInfo.celNum   = i;
        if ( rdroid_pHS->pFileWrite(fh, &texcelInfo, sizeof(rdMatTextureCelInfo)) != sizeof(rdMatTextureCelInfo) )
        {
            goto error;
        }
    }

    for ( j = 0; j < pMaterial->numCels; ++j )
    {
        texHeader.unknown1        = 0;
        texHeader.transparentBool = 0;
        texHeader.width           = pMaterial->width;
        texHeader.height          = pMaterial->height;
        texHeader.numMipLevels    = numMipLevels;
        texHeader.unknown2        = 0;
        if ( rdroid_pHS->pFileWrite(fh, &texHeader, sizeof(rdMatTextureHeader)) != sizeof(rdMatTextureHeader) )
        {
            goto error;
        }

        for ( k = 0; k < numMipLevels; ++k )
        {
            stdDisplay_VBufferLock(paTextures[j][k]);
            if ( paTextures[j][k]->rasterInfo.size != rdroid_pHS->pFileWrite(fh, paTextures[j][k]->pPixels, paTextures[j][k]->rasterInfo.size) )
            {
                goto error;
            }

            stdDisplay_VBufferUnlock(paTextures[j][k]);
        }
    }

    rdroid_pHS->pFileClose(fh);
    return 0;

open_error:
#line 562
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid save filename '%s'.\n", pFilename, 0, 0, 0);

error:
    if ( fh )
    {
        rdroid_pHS->pFileClose(fh);
    }

    return 1;
}

size_t J3DAPI rdMaterial_GetMaterialMemUsage(const rdMaterial* pMaterial)
{
    size_t mipLevels;
    size_t memSize;

    mipLevels = std3D_GetMipMapCount(pMaterial->aTextures);
    memSize = rdMaterial_GetMipSize(pMaterial->width, pMaterial->height, mipLevels);
    memSize *= pMaterial->numCels;
    memSize += sizeof(rdMaterial);
    memSize += pMaterial->numCels * sizeof(tSystemTexture);
    return memSize;
}

size_t J3DAPI rdMaterial_GetMipSize(int width, int height, size_t mipLevels)
{
    size_t total = 0;
    size_t curSize = width * height;
    size_t i;

    for ( i = 0; i < mipLevels; ++i )
    {
        total += curSize;
        curSize >>= 2;
    }

    return total;
}
