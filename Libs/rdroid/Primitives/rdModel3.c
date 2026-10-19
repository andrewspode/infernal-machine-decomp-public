#define RDVECTOR_NO_INLINE_SET4 // Retail calls rdVector_Set4 out of line only at 0x004C9D90 in rdModel3_LoadEntry (target 0x004CBBC0); its four other colour sets are inlined field stores
#include "rdModel3.h"

#include <rdroid/Main/rdroid.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Engine/rdClip.h>
#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Engine/rdLight.h>
#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Primitives/rdModel3K.h>
#include <rdroid/Primitives/rdPrimit3.h>
#include <rdroid/Raster/rdCache.h>
#include <rdroid/Raster/rdFace.h>

#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(rdModel3_countfit_1, 15)
CF_PROTO(rdModel3_countfit_2, 15)
CF_PROTO(rdModel3_countfit_3, 4)
/* end count-fit */

#define RDMODEL3_3DO_MAJVER      2
#define RDMODEL3_3DO_MINVER_I    1
#define RDMODEL3_3DO_MINVER_RGB  2
#define RDMODEL3_3DO_MINVER_RGBA 3

// Vars used for drawing model 
static rdModel3DrawFaceFunc pfDrawFace = &rdModel3_DrawFace;

// Zero-initialised private statics: VC5 lays these out in declaration order, which is retail's
// private block 0x014DA928-0x014DD94C.
static rdVector3 aView[RDMODEL3_MAX_VERTICES] = { 0 };
static const rdThing* pCurThing              = NULL;
static int bDrawMeshBounds                   = 0;
static rdModel3LoaderFunc pModel3Loader      = NULL;
static rdModel3UnloaderFunc pModel3Unloader  = NULL;
size_t rdModel3_numGeoLights                   = 0;
size_t rdModel3_numMeshLights                  = 0;

// Vars used when loading 3DO model
static int bCalcGeoRadius = 0;
static float maxGeoRadius = 0.0;

rdFaceFlags rdModel3_extraFaceFlags = 0;

// Tentative definitions: retail allocates these in the image's common tail, interleaved with
// other TUs' commons (0x0183AF90..0x0183B9C8).
rdLightMode rdModel3_curLightingMode;
rdLight* rdModel3_apGeoLights[RDCAMERA_MAX_LIGHTS];
const rdModel3Mesh* rdModel3_pCurMesh;
rdLightMode rdModel3_lightingMode;
rdVector3 rdModel3_localCamera;
rdLight* rdModel3_apMeshLights[RDCAMERA_MAX_LIGHTS];
rdVector3 rdModel3_aLocalLightPos[RDCAMERA_MAX_LIGHTS];

// Declared in rdModel3.h and read by sithRender; retail allocates both in the image's
// common tail (0x0183AF84 and 0x0183AF9C), i.e. tentative definitions, not initialised statics.
int rdModel3_numDrawnFaces;
int rdModel3_numDrawnAlphaFaces;

rdModel3LoaderFunc J3DAPI rdModel3_RegisterLoader(rdModel3LoaderFunc pfFunc)
{
    rdModel3LoaderFunc pPrevFunc;

    pPrevFunc = pModel3Loader;
    pModel3Loader = pfFunc;
    return pPrevFunc;
}

// Discarded by the retail linker; debug 0x50cdec. Retail keeps its private slot (pModel3Unloader,
// 0x014DD934), read only by the discarded Free.
rdModel3UnloaderFunc J3DAPI rdModel3_RegisterUnloader(rdModel3UnloaderFunc pfFunc)
{
    rdModel3UnloaderFunc pPrevFunc;

    pPrevFunc = pModel3Unloader;
    pModel3Unloader = pfFunc;
    return pPrevFunc;
}

void J3DAPI rdModel3_NewEntry(rdModel3* pModel3)
{
    memset(pModel3, 0, sizeof(rdModel3));
    strncpy(pModel3->aName, "UNKNOWN", STD_ARRAYLEN(pModel3->aName) - 1);
    pModel3->aName[STD_ARRAYLEN(pModel3->aName) - 1] = 0;
    pModel3->curGeoNum = 0;
}

// Discarded by the retail linker; text of the debug build (debug 0x50ce44, /Od). Retail's /Gf string run
// holds this TU's filename literal (0x0054AB38) right after NewEntry's "UNKNOWN" and before every
// LoadEntry string: this function's STDMALLOC is its first reference. The allocation
// print is a block after the success return (debug `jne; jmp 0x50cea6`, then `jmp 0x50ced1` past it to
// the shared Free): error labels after the success return.
rdModel3* J3DAPI rdModel3_Load(const char* pName)
{
    rdModel3* pModel3 = NULL;

    if ( pModel3Loader )
    {
        return pModel3Loader(pName, 0);
    }

#line 215
    pModel3 = (rdModel3*)STDMALLOC(sizeof(rdModel3));
    if ( !pModel3 )
    {
        goto alloc_error;
    }

    if ( rdModel3_LoadEntry(pName, pModel3) )
    {
        return pModel3;
    }

    goto error;

alloc_error:
#line 227
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for model '%s'.\n", pName, 0, 0, 0);

error:
    rdModel3_Free(pModel3);
    return NULL;
}

// Text of the debug build (debug 0x50cee3, /Od): function-scope locals only, the open
// (881), EOF (886), syntax (891), allocation (896) and range (901) prints as blocks after
// the success return, reaching one shared FreeEntry + Close cleanup. Exception (C) labels
// plus the debug-route `open_error`.
int J3DAPI rdModel3_LoadEntry(const char* pFilename, rdModel3* pModel3)
{
    rdVector2* pTexVertex = NULL;
    rdVector3* pVertex = NULL;
    int texMode;
    int vmin, vmaj;
    int bRGB;
    size_t numMats;
    size_t numNodes;
    size_t numGeos;
    size_t numMeshes;
    size_t numVerts;
    size_t numTexVerts;
    size_t numFaces;
    size_t faceVerts;
    size_t i;
    size_t geoNum;
    size_t meshNum;
    size_t vertNum;
    size_t faceNum;
    size_t nodeNum;
    int matIdx;
    rdModel3GeoSet* pGeo;
    rdModel3Mesh* pMesh;
    rdFace* pFace;
    rdModel3HNode* pNode;
    const char* aRGB;
    float radius;
    float x, y, z;
    float red, green, blue, alpha;
    float intensity;
    int entryNum;
    float pitch, yaw, roll;
    float pivotX, pivotY, pivotZ;
    float u, v;
    float li;
    int parentIdx, childIdx, siblingIdx;

    rdModel3_NewEntry(pModel3);

    strncpy(pModel3->aName, stdFileFromPath(pFilename), STD_ARRAYLEN(pModel3->aName) - 1);
    pModel3->aName[STD_ARRAYLEN(pModel3->aName) - 1] = 0;

    if ( !stdConffile_Open(pFilename) )
    {
        goto open_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " 3do %d.%d", &vmaj, &vmin) != 2 )
    {
#line 302
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid 3DO version %d.%d for file '%s'.\n", vmaj, vmin, pFilename, 0);
        // TODO: Should return error?
    }

    // TODO: Add check for vmaj

    if ( vmin == RDMODEL3_3DO_MINVER_I )
    {
        bRGB = 0;
    }
    else if ( vmin == RDMODEL3_3DO_MINVER_RGB || vmin == RDMODEL3_3DO_MINVER_RGBA )
    {
        bRGB = 1;
    }
    else
    {
#line 310
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid 3DO version %d.%d for file '%s'.\n", vmaj, vmin, pFilename, 0);
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " materials %d", &numMats) != 1 )
    {
        goto syntax_error;
    }

    if ( numMats > 0 )
    {
#line 329
        pModel3->apMaterials = (rdMaterial**)STDMALLOC(sizeof(rdMaterial**) * numMats);
        if ( !pModel3->apMaterials )
        {
            goto alloc_error;
        }

        for ( i = 0; i < numMats; ++i )
        {
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " %d: %s", &entryNum, std_genBuffer) != 2 )
            {
                goto syntax_error;
            }

            pModel3->apMaterials[i] = rdMaterial_Load(std_genBuffer);
            if ( !pModel3->apMaterials[i] )
            {
                goto syntax_error;
            }
        }
    }

    pModel3->numMaterials = numMats; // TODO [BUG]: Note, assigning numMats here makes sure that materials are not getting freed in case of mat load error.
                                     //             But this might also be cause of bug because from here on in any case of an error the materials are getting freed.
                                     //             Since materials are cached in sithMaterial module all freed materials will be invalidated in cache.
                                     //             And any part of the code that is referencing cached materials will access freed pointer!

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " radius %f", &radius) != 1 )
    {
        goto syntax_error;
    }

    pModel3->radius = radius;

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " insert offset %f %f %f", &x, &y, &z) != 3 )
    {
        goto syntax_error;
    }

    pModel3->insertOffset.x = x;
    pModel3->insertOffset.y = y;
    pModel3->insertOffset.z = z;

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " geosets %d", &numGeos) != 1 )
    {
        goto syntax_error;
    }

    pModel3->numGeos = numGeos;

    for ( geoNum = 0; geoNum < numGeos; geoNum++ )
    {
        pGeo = &pModel3->aGeos[geoNum];
        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_aLine, " geoset %d", &entryNum) != 1 )
        {
            goto syntax_error;
        }

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, " meshes %d", &numMeshes) != 1 )
        {
            goto syntax_error;
        }

        // TODO: maybe check numMeshes range, if 0 should continue

#line 410
        pGeo->aMeshes = (rdModel3Mesh*)STDMALLOC(sizeof(rdModel3Mesh) * numMeshes);
        if ( !pGeo->aMeshes )
        {
            goto alloc_error;
        }

        pGeo->numMeshes = numMeshes;
        for ( meshNum = 0; meshNum < numMeshes; meshNum++ )
        {
            pMesh = &pGeo->aMeshes[meshNum];

            rdVector_Set4(&pMesh->meshColor, 1.0f, 1.0f, 1.0f, 1.0f); // white
            pMesh->num = meshNum;

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " mesh %d", std_genBuffer) != 1 )
            {
                goto syntax_error;
            }

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " name %s", std_genBuffer) != 1 )
            {
                goto syntax_error;
            }

            strncpy(pMesh->name, std_genBuffer, STD_ARRAYLEN(pMesh->name) - 1);
            pMesh->name[STD_ARRAYLEN(pMesh->name) - 1] = 0;

            if ( !stdConffile_ReadLine() )
            {
                goto syntax_error;
            }

            if ( sscanf(stdConffile_aLine, " radius %f", &pMesh->radius) != 1 )
            {
                goto syntax_error;
            }

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " geometrymode %d", &pMesh->geoMode) != 1 )
            {
                goto syntax_error;
            }

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " lightingmode %d", &pMesh->lightMode) != 1 )
            {
                goto syntax_error;
            }

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " texturemode %d", &texMode) != 1 )
            {
                goto syntax_error;
            }

            // Read mesh vertices

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " vertices %d", &numVerts) != 1 )
            {
                goto syntax_error;
            }

            if ( numVerts > RDMODEL3_MAX_VERTICES )
            {
                goto range_error;
            }

            pMesh->apVertices        = NULL;
            pMesh->aVertColors       = NULL;
            pMesh->aLightIntensities = NULL;

            pMesh->someFaceFlags = 0;
            if ( pMesh->lightMode != RD_LIGHTING_GOURAUD )
            {
                pMesh->someFaceFlags = 1;
            }

            if ( numVerts > 0 )
            {
#line 503
                pMesh->apVertices = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
                if ( !pMesh->apVertices )
                {
                    goto alloc_error;
                }

#line 510
                pMesh->aVertColors = (rdVector4*)STDMALLOC(sizeof(rdVector4) * numVerts);
                if ( !pMesh->aVertColors )
                {
                    goto alloc_error;
                }

#line 516
                pMesh->aLightIntensities = (rdVector4*)STDMALLOC(sizeof(rdVector4) * numVerts);
                if ( !pMesh->aLightIntensities )
                {
                    goto alloc_error;
                }

                memset(pMesh->aLightIntensities, 0, sizeof(rdVector4) * numVerts);
            }

            pVertex = pMesh->apVertices;
            for ( vertNum = 0; vertNum < numVerts; ++vertNum )
            {
                if ( !stdConffile_ReadLine() )
                {
                    goto eof_error;
                }

                if ( !bRGB )
                {
                    if ( sscanf(stdConffile_aLine, " %d: %f %f %f %f", &entryNum, &x, &y, &z, &intensity) != 5 )
                    {
                        goto syntax_error;
                    }

                    pMesh->aVertColors[vertNum].red   = intensity;
                    pMesh->aVertColors[vertNum].green = intensity;
                    pMesh->aVertColors[vertNum].blue  = intensity;
                    pMesh->aVertColors[vertNum].alpha = 1.0f;
                }
                else
                {
                    alpha = 1.0f;
                    if ( sscanf(stdConffile_aLine, " %d: %f %f %f %f %f %f %f", &entryNum, &x, &y, &z, &red, &green, &blue, &alpha) != 8
                        && sscanf(stdConffile_aLine, " %d: %f %f %f %f %f %f", &entryNum, &x, &y, &z, &red, &green, &blue) != 7 )
                    {
                        goto syntax_error;
                    }

                    pMesh->aVertColors[vertNum].red   = red;
                    pMesh->aVertColors[vertNum].green = green;
                    pMesh->aVertColors[vertNum].blue  = blue;
                    pMesh->aVertColors[vertNum].alpha = alpha;
                }

                pVertex->x = x;
                pVertex->y = y;
                pVertex->z = z;
                ++pVertex;
            }

            pMesh->numVertices = numVerts;

            // Read UV vertices

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " texture vertices %d", &numTexVerts) != 1 )
            {
                goto syntax_error;
            }

            if ( numTexVerts > RDMODEL3_MAX_TEXVRTICES )
            {
                goto range_error;
            }

            pMesh->apTexVertices = NULL;
            if ( numTexVerts )
            {
#line 576
                pMesh->apTexVertices = (rdVector2*)STDMALLOC(sizeof(rdVector2) * numTexVerts);
                if ( !pMesh->apTexVertices )
                {
                    goto alloc_error;
                }

                pTexVertex = pMesh->apTexVertices;
                for ( vertNum = 0; vertNum < numTexVerts; ++vertNum )
                {
                    if ( !stdConffile_ReadLine() )
                    {
                        goto eof_error;
                    }

                    if ( sscanf(stdConffile_aLine, " %d: %f %f", &entryNum, &u, &v) != 3 )
                    {
                        goto syntax_error;
                    }

                    pTexVertex->x = u;
                    pTexVertex->y = v;
                    ++pTexVertex;
                }
            }

            pMesh->numTexVertices = numTexVerts;

            // Read vert normals

            // Skip section line "VERTEX NORMALS"
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            pMesh->aVertNormals = NULL;
            if ( numVerts > 0 )
            {
#line 608
                pMesh->aVertNormals = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
                if ( !pMesh->aVertNormals )
                {
                    goto alloc_error;
                }
            }

            for ( vertNum = 0; vertNum < numVerts; vertNum++ )
            {
                if ( !stdConffile_ReadLine() )
                {
                    goto eof_error;
                }

                if ( sscanf(stdConffile_aLine, " %d: %f %f %f", &entryNum, &x, &y, &z) != 4 )
                {
                    goto syntax_error;
                }

                pMesh->aVertNormals[vertNum].x = x;
                pMesh->aVertNormals[vertNum].y = y;
                pMesh->aVertNormals[vertNum].z = z;
            }

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            // Read mesh faces

            if ( sscanf(stdConffile_aLine, " faces %d", &numFaces) != 1 )
            {
                goto syntax_error;
            }

            if ( numFaces > RDMODEL3_MAX_FACES )
            {
                goto range_error;
            }

            pMesh->numFaces = numFaces;
            pMesh->aFaces   = NULL;

            if ( numFaces > 0 )
            {
#line 644
                pMesh->aFaces = (rdFace*)STDMALLOC(sizeof(rdFace) * numFaces);
                if ( !pMesh->aFaces )
                {
                    goto alloc_error;
                }
            }

            pFace = pMesh->aFaces;
            for ( faceNum = 0; faceNum < numFaces; ++faceNum )
            {
                if ( !stdConffile_ReadLine() )
                {
                    goto eof_error;
                }

                rdFace_NewEntry(pFace);

                // Skip face num
                strtok(stdConffile_aLine, " \t");

                // Parse mat idx
                matIdx = atoi(strtok(NULL, " \t"));
                pFace->num = faceNum;
                if ( matIdx == -1 )
                {
                    pFace->pMaterial = NULL;
                }
                else
                {
                    pFace->pMaterial = pModel3->apMaterials[matIdx]; // TODO: Add bounds check
                }

                // Parse faceflags
                if ( sscanf(strtok(NULL, " \t"), "%x", &pFace->flags) != 1 )
                {
                    goto syntax_error;
                }

                // Parse geo mode
                if ( sscanf(strtok(NULL, " \t"), "%d", &pFace->geometryMode) != 1 )
                {
                    goto syntax_error;
                }

                // Parse light mode
                if ( sscanf(strtok(NULL, " \t"), "%d", &pFace->lightingMode) != 1 )
                {
                    goto syntax_error;
                }

                if ( pFace->lightingMode != RD_LIGHTING_GOURAUD )
                {
                    pMesh->someFaceFlags = 1;
                }

                if ( sscanf(strtok(NULL, " \t"), "%d", &texMode) != 1 ) // textMode not used
                {
                    goto syntax_error;
                }

                // Parse extra light
                if ( !bRGB )
                {
                    if ( sscanf(strtok(NULL, " \t"), "%f", &li) != 1 )
                    {
                        goto syntax_error;
                    }

                    pFace->extraLight.red   = li;
                    pFace->extraLight.green = li;
                    pFace->extraLight.blue  = li;
                    pFace->extraLight.alpha = 1.0f;
                }
                else
                {
                    alpha = 1.0f;
                    aRGB = strtok(NULL, " \t");
                    if ( sscanf(aRGB, "(%f/%f/%f/%f)", &red, &green, &blue, &alpha) != 4
                        && sscanf(aRGB, "(%f/%f/%f)", &red, &green, &blue) != 3 )
                    {
                        goto syntax_error;
                    }

                    pFace->extraLight.red   = red;
                    pFace->extraLight.green = green;
                    pFace->extraLight.blue  = blue;
                    pFace->extraLight.alpha = alpha;
                }

                faceVerts = atoi(strtok(NULL, " \t"));
                if ( !faceVerts )
                {
                    goto syntax_error;
                }

                if ( faceVerts > RDMODEL3_MAX_FACE_VERTICES )
                {
                    goto range_error;
                }

                pFace->numVertices = faceVerts;
#line 726
                pFace->aVertices = (size_t*)STDMALLOC(sizeof(int*) * faceVerts);
                if ( !pFace->aVertices )
                {
                    goto alloc_error;
                }

                if ( pFace->pMaterial )
                {
#line 733
                    pFace->aTexVertices = (size_t*)STDMALLOC(sizeof(int*) * faceVerts);
                    if ( !pFace->aTexVertices )
                    {
                        goto alloc_error;
                    }

                    for ( vertNum = 0; vertNum < faceVerts; ++vertNum )
                    {
                        pFace->aVertices[vertNum] = atoi(strtok(NULL, " \t,")); // Parse vert index
                        if ( pFace->aVertices[vertNum] > pMesh->numVertices - 1 )
                        {
                            pFace->aVertices[vertNum] = 0;
#line 744
                            stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of range vertex on mesh %s of model %s!\n", pMesh->name, pModel3->aName, 0, 0);
                        }

                        pFace->aTexVertices[vertNum] = atoi(strtok(NULL, " \t,")); // Parse UV index;
                        if ( pFace->aTexVertices[vertNum] > pMesh->numTexVertices - 1 )
                        {
                            pFace->aTexVertices[vertNum] = 0;
#line 750
                            stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of range tex vertex on mesh %s of model %s!\n", pMesh->name, pModel3->aName, 0, 0);
                        }
                    }
                }
                else
                {
                    for ( vertNum = 0; vertNum < faceVerts; ++vertNum )
                    {
                        pFace->aVertices[vertNum] = atoi(strtok(NULL, " \t,")); // Parse vert index
                        strtok(NULL, " \t,");// Skip texVertIdx; retail's no-material loop (0x004CA6CE) has no range check
                    }
                }

                pMesh->someFaceFlags |= pFace->flags & RD_FF_TEX_TRANSLUCENT;
                if ( pFace->pMaterial )
                {
                    pMesh->someFaceFlags |= pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGBA;
                }
                ++pFace;
            }

            // Read Face normals

            // Skip section line "FACE NORMALS"
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            pFace = pMesh->aFaces;
            for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                if ( !stdConffile_ReadLine() )
                {
                    goto eof_error;
                }

                if ( sscanf(stdConffile_aLine, " %d: %f %f %f", &entryNum, &x, &y, &z) != 4 )
                {
                    goto syntax_error;
                }

                pFace->normal.x = x;
                pFace->normal.y = y;
                pFace->normal.z = z;
                ++pFace;
            }
        }
    }

    // Section HIERARCHYDEF
    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " hierarchy nodes %d", &numNodes) != 1 )
    {
        goto syntax_error;
    }

#line 816
    pModel3->aHierarchyNodes = (rdModel3HNode*)STDMALLOC(sizeof(rdModel3HNode) * numNodes);
    if ( !pModel3->aHierarchyNodes )
    {
        goto alloc_error;
    }

    pModel3->numHNodes = numNodes;

    for ( nodeNum = 0; nodeNum < numNodes; ++nodeNum )
    {
        pNode = &pModel3->aHierarchyNodes[nodeNum];
        pNode->num = nodeNum;

        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        // The node flags go into texMode: retail reads both into one never-read slot (3 address sites)
        if ( sscanf(stdConffile_aLine, " %d: %x %x %d %d %d %d %d %f %f %f %f %f %f %f %f %f %s", &entryNum, &texMode, &pNode->type, &pNode->meshIdx, &parentIdx, &childIdx, &siblingIdx, &pNode->numChildren, &x, &y, &z, &pitch, &yaw, &roll, &pivotX, &pivotY, &pivotZ, pNode->aName) != 18 )
        {
            goto syntax_error;
        }

        // TODO: Add bounds check
        if ( parentIdx != -1 )
        {
            pNode->pParent = &pModel3->aHierarchyNodes[parentIdx];
        }
        else
        {
            pNode->pParent = NULL;
        }

        if ( childIdx != -1 )
        {
            pNode->pChild = &pModel3->aHierarchyNodes[childIdx];
        }
        else
        {
            pNode->pChild = NULL;
        }

        if ( siblingIdx != -1 )
        {
            pNode->pSibling = &pModel3->aHierarchyNodes[siblingIdx];
        }
        else
        {
            pNode->pSibling = NULL;
        }

        pNode->pos.x = x;
        pNode->pos.y = y;
        pNode->pos.z = z;

        pNode->pyr.pitch = pitch;
        pNode->pyr.yaw   = yaw;
        pNode->pyr.roll  = roll;

        pNode->pivot.x = pivotX;
        pNode->pivot.y = pivotY;
        pNode->pivot.z = pivotZ;
    }

    stdConffile_Close();

    bCalcGeoRadius = 1;
    maxGeoRadius = 0.0f;
    rdModel3_BuildExpandedRadius(pModel3, pModel3->aHierarchyNodes, &rdroid_identMatrix34);
    pModel3->size = maxGeoRadius;
    bCalcGeoRadius = 0;

    return 1; // Success

open_error:
#line 881
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid load filename '%s'.\n", pFilename, 0, 0, 0);
    return 0;

eof_error:
#line 886
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", pFilename, 0, 0, 0);
    goto error;

syntax_error:
#line 891
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);
    goto error;

alloc_error:
#line 896
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);
    goto error;

range_error:
#line 901
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Value out of range for '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);

error:
    rdModel3_FreeEntry(pModel3);
    stdConffile_Close();
    return 0;
}

// Discarded by the retail linker (no caller in either build): debug 0x50e487, the post-load pass of
// the loader family (name inferred). Assert line 937.
void J3DAPI rdModel3_LoadPostProcess(rdModel3* pModel3)
{
    rdModel3_CalcRadii(pModel3);
    rdModel3_CalcFaceNormals(pModel3);
    rdModel3_CalcVertexNormals(pModel3);
#line 937
    RD_ASSERT(rdModel3_Validate(pModel3));
}

// Discarded by the retail linker: the debug build's model text writer (debug 0x50e4db, /Od), restored
// in full from its code; frame 0x3c = these 15 function-scope locals (one vertNum for every vertex
// loop, one i for the material loops). The vertex-normal loop advances pFace (debug 0x50ead0) and the
// /Gf literals are COMDATs that went with the function. Name and signature inferred (JK
// rdModel3_WriteText).
int J3DAPI rdModel3_Write(const char* pFilename, const rdModel3* pModel, const char* pCratedName)
{
    size_t vertNum;
    tFileHandle fh;
    size_t i;
    const rdFace* pFace;
    size_t nodeNum;
    int siblingNum;
    size_t faceNum;
    int childNum;
    int parentNum;
    const rdModel3HNode* pNode;
    const rdModel3Mesh* pMesh;
    size_t geoNum;
    const rdModel3GeoSet* pGeo;
    int matNum;
    size_t meshNum;

    fh = rdroid_pHS->pFileOpen(pFilename, "wt+");
    if ( !fh )
    {
        return 0;
    }

    rdroid_pHS->pFilePrintf(fh, "# MODEL '%s' created from '%s'\n\n", pModel->aName, pCratedName);
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: HEADER\n\n");
    rdroid_pHS->pFilePrintf(fh, "3DO %d.%d\n\n", RDMODEL3_3DO_MAJVER, RDMODEL3_3DO_MINVER_RGBA);
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: MODELRESOURCE\n\n");
    rdroid_pHS->pFilePrintf(fh, "# Materials list\n");
    rdroid_pHS->pFilePrintf(fh, "MATERIALS %d\n\n", pModel->numMaterials);

    for ( i = 0; i < pModel->numMaterials; ++i )
    {
        rdroid_pHS->pFilePrintf(fh, "%10d:%15s\n", i, pModel->apMaterials[i]);
    }

    rdroid_pHS->pFilePrintf(fh, "\n\n");
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: GEOMETRYDEF\n\n");
    rdroid_pHS->pFilePrintf(fh, "# Object radius\n");
    rdroid_pHS->pFilePrintf(fh, "RADIUS %10.6f\n\n", pModel->radius);
    rdroid_pHS->pFilePrintf(fh, "# Insertion offset\n");
    rdroid_pHS->pFilePrintf(fh, "INSERT OFFSET %10.6f %10.6f %10.6f\n\n", pModel->insertOffset.x, pModel->insertOffset.y, pModel->insertOffset.z);
    rdroid_pHS->pFilePrintf(fh, "# Number of Geometry Sets\n");
    rdroid_pHS->pFilePrintf(fh, "GEOSETS %d\n\n", pModel->numGeos);

    for ( geoNum = 0; geoNum < pModel->numGeos; ++geoNum )
    {
        pGeo = &pModel->aGeos[geoNum];
        rdroid_pHS->pFilePrintf(fh, "# Geometry Set definition\n");
        rdroid_pHS->pFilePrintf(fh, "GEOSET %d\n\n", geoNum);
        rdroid_pHS->pFilePrintf(fh, "# Number of Meshes\n");
        rdroid_pHS->pFilePrintf(fh, "MESHES %d\n\n\n", pGeo->numMeshes);

        for ( meshNum = 0; meshNum < pGeo->numMeshes; ++meshNum )
        {
            pMesh = &pGeo->aMeshes[meshNum];
            rdroid_pHS->pFilePrintf(fh, "# Mesh definition\n");
            rdroid_pHS->pFilePrintf(fh, "MESH %d\n\n", meshNum);
            rdroid_pHS->pFilePrintf(fh, "NAME %s\n\n", pMesh->name);
            rdroid_pHS->pFilePrintf(fh, "RADIUS %10.6f\n\n", pMesh->radius);
            rdroid_pHS->pFilePrintf(fh, "GEOMETRYMODE\t%d\n", pMesh->geoMode);
            rdroid_pHS->pFilePrintf(fh, "LIGHTINGMODE\t%d\n", pMesh->lightMode);
            rdroid_pHS->pFilePrintf(fh, "TEXTUREMODE\t%d\n", 3);
            rdroid_pHS->pFilePrintf(fh, "\n\n");

            rdroid_pHS->pFilePrintf(fh, "VERTICES %d\n\n", pMesh->numVertices);
            rdroid_pHS->pFilePrintf(fh, "# num:     x:         y:         z:         i: \n");
            for ( vertNum = 0; vertNum < pMesh->numVertices; ++vertNum )
            {
                rdroid_pHS->pFilePrintf(
                    fh,
                    "  %3d: %10.6f %10.6f %10.6f %8.6f %8.6f %8.6f %8.6f\n",
                    vertNum,
                    pMesh->apVertices[vertNum].x,
                    pMesh->apVertices[vertNum].y,
                    pMesh->apVertices[vertNum].z,
                    pMesh->aVertColors[vertNum].red,
                    pMesh->aVertColors[vertNum].green,
                    pMesh->aVertColors[vertNum].blue,
                    pMesh->aVertColors[vertNum].alpha
                );
            }

            rdroid_pHS->pFilePrintf(fh, "\n\n");
            rdroid_pHS->pFilePrintf(fh, "TEXTURE VERTICES %d\n\n", pMesh->numTexVertices);
            for ( vertNum = 0; vertNum < pMesh->numTexVertices; ++vertNum )
            {
                rdroid_pHS->pFilePrintf(fh, "  %3d: %10.6f %10.6f\n", vertNum, pMesh->apTexVertices[vertNum].x, pMesh->apTexVertices[vertNum].y);
            }

            rdroid_pHS->pFilePrintf(fh, "\n\n");
            rdroid_pHS->pFilePrintf(fh, "VERTEX NORMALS\n\n");
            rdroid_pHS->pFilePrintf(fh, "# num:     x:         y:         z:\n");
            for ( vertNum = 0; vertNum < pMesh->numVertices; ++vertNum )
            {
                rdroid_pHS->pFilePrintf(fh, "  %3d: %10.6f %10.6f %10.6f\n", vertNum, pMesh->aVertNormals[vertNum].x, pMesh->aVertNormals[vertNum].y, pMesh->aVertNormals[vertNum].z);
                ++pFace;
            }

            rdroid_pHS->pFilePrintf(fh, "\n\n");
            rdroid_pHS->pFilePrintf(fh, "FACES %d\n\n", pMesh->numFaces);
            pFace = pMesh->aFaces;
            rdroid_pHS->pFilePrintf(fh, "#  num:  material:   type:  geo:  light:   tex:  R:  G:  B:  A:  verts:\n");
            for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                if ( pFace->pMaterial )
                {
                    matNum = -1;
                    for ( i = 0; i < pModel->numMaterials; ++i )
                    {
                        if ( pFace->pMaterial == pModel->apMaterials[i] )
                        {
                            matNum = i;
                        }
                    }

#line 1097
                    RD_ASSERT(matNum!=-1);
                }
                else
                {
                    matNum = -1;
                }

                rdroid_pHS->pFilePrintf(
                    fh,
                    "   %3d: %9d  0x%04x  %4d %7d %6d (%f/%f/%f/%f) %7d  ",
                    faceNum,
                    matNum,
                    pFace->flags,
                    pFace->geometryMode,
                    pFace->lightingMode,
                    3,
                    pFace->extraLight.red,
                    pFace->extraLight.green,
                    pFace->extraLight.blue,
                    pFace->extraLight.alpha,
                    pFace->numVertices
                );

                if ( pFace->aTexVertices )
                {
                    for ( vertNum = 0; vertNum < pFace->numVertices; ++vertNum )
                    {
                        rdroid_pHS->pFilePrintf(fh, "%3d,%3d ", pFace->aVertices[vertNum], pFace->aTexVertices[vertNum]);
                    }
                }
                else
                {
                    for ( vertNum = 0; vertNum < pFace->numVertices; ++vertNum )
                    {
                        rdroid_pHS->pFilePrintf(fh, "%3d, 0 ", pFace->aVertices[vertNum]);
                    }
                }

                rdroid_pHS->pFilePrintf(fh, "\n");
                ++pFace;
            }

            rdroid_pHS->pFilePrintf(fh, "\n\n");
            rdroid_pHS->pFilePrintf(fh, "FACE NORMALS\n\n");
            pFace = pMesh->aFaces;
            rdroid_pHS->pFilePrintf(fh, "# num:     x:         y:         z:\n");
            for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                rdroid_pHS->pFilePrintf(fh, "  %3d: %10.6f %10.6f %10.6f\n", faceNum, pFace->normal.x, pFace->normal.y, pFace->normal.z);
                ++pFace;
            }

            rdroid_pHS->pFilePrintf(fh, "\n\n");
        }
    }

    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: HIERARCHYDEF\n\n");
    rdroid_pHS->pFilePrintf(fh, "# Hierarchy node list\n");
    rdroid_pHS->pFilePrintf(fh, "HIERARCHY NODES %d\n\n", pModel->numHNodes);
    rdroid_pHS->pFilePrintf(
        fh,
        "#  num:   flags:   type:    mesh:  parent:  child:  sibling:  numChildren:        x:         y:         z:     pitch:       yaw:      roll:    pivotx:  "
        "  pivoty:    pivotz:  hnodename:\n");

    for ( nodeNum = 0; nodeNum < pModel->numHNodes; ++nodeNum )
    {
        pNode = &pModel->aHierarchyNodes[nodeNum];
        if ( pNode->pParent )
        {
            parentNum = pNode->pParent->num;
        }
        else
        {
            parentNum = -1;
        }

        if ( pNode->pChild )
        {
            childNum = pNode->pChild->num;
        }
        else
        {
            childNum = -1;
        }

        if ( pNode->pSibling )
        {
            siblingNum = pNode->pSibling->num;
        }
        else
        {
            siblingNum = -1;
        }

        rdroid_pHS->pFilePrintf(
            fh,
            "   %3d:  0x%04x 0x%05X %8d %8d %7d %9d %13d %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f %10.6f  %s\n",
            nodeNum,
            0,
            pNode->type,
            pNode->meshIdx,
            parentNum,
            childNum,
            siblingNum,
            pNode->numChildren,
            pNode->pos.x,
            pNode->pos.y,
            pNode->pos.z,
            pNode->pyr.pitch,
            pNode->pyr.yaw,
            pNode->pyr.roll,
            pNode->pivot.x,
            pNode->pivot.y,
            pNode->pivot.z,
            pNode->aName
        );
    }

    rdroid_pHS->pFileClose(fh);
    return 1;
}

// Discarded by the retail linker (called only by the discarded Load): debug 0x50efb6; the NULL
// warning is line 1225.
void J3DAPI rdModel3_Free(rdModel3* pModel3)
{
    if ( pModel3 )
    {
        if ( pModel3Unloader )
        {
            pModel3Unloader(pModel3);
            return;
        }

        rdModel3_FreeEntry(pModel3);
        stdMemory_Free(pModel3);
    }
    else
    {
#line 1225
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL model ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdModel3_FreeEntry(rdModel3* pModel3)
{
    size_t idx;
    rdFace* pFace;
    size_t k;
    rdModel3Mesh* pMesh;

    if ( pModel3 )
    {
        size_t i;
        rdModel3GeoSet* pGeoSet;

        pGeoSet = pModel3->aGeos;
        for ( i = 0; i < pModel3->numGeos; ++i )
        {
            size_t j;

            pMesh = pGeoSet->aMeshes;
            for ( j = 0; j < pGeoSet->numMeshes; ++j )
            {
                if ( pMesh->apVertices )
                {
                    stdMemory_Free(pMesh->apVertices);
                }

                if ( pMesh->apTexVertices )
                {
                    stdMemory_Free(pMesh->apTexVertices);
                }

                if ( pMesh->aFaces )
                {
                    pFace = pMesh->aFaces;
                    for ( k = 0; k < pMesh->numFaces; ++k )
                    {
                        rdFace_FreeEntry(pFace);
                        ++pFace;
                    }

                    stdMemory_Free(pMesh->aFaces);
                }

                if ( pMesh->aVertColors )
                {
                    stdMemory_Free(pMesh->aVertColors);
                }

                if ( pMesh->aLightIntensities )
                {
                    stdMemory_Free(pMesh->aLightIntensities);
                }

                if ( pMesh->aVertNormals )
                {
                    stdMemory_Free(pMesh->aVertNormals);
                }

                ++pMesh;
            }

            if ( pGeoSet->aMeshes )
            {
                stdMemory_Free(pGeoSet->aMeshes);
            }

            ++pGeoSet;
        }

        if ( pModel3->aHierarchyNodes )
        {
            stdMemory_Free(pModel3->aHierarchyNodes);
        }

        for ( idx = 0; idx < pModel3->numMaterials; ++idx )
        {
            // TODO: add check for null material
            rdMaterial_Free(pModel3->apMaterials[idx]);
        }

        if ( pModel3->apMaterials )
        {
            stdMemory_Free(pModel3->apMaterials);
        }
    }
    else
    {
#line 1304
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL model ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdModel3_FreeEntryGeometryOnly(rdModel3* pModel3)
{
    rdFace* pFace;
    size_t k;
    rdModel3Mesh* pMesh;

    if ( pModel3 )
    {
        size_t i;
        rdModel3GeoSet* pGeoSet;

        pGeoSet = pModel3->aGeos;
        for ( i = 0; i < pModel3->numGeos; ++i )
        {
            size_t j;

            pMesh = pGeoSet->aMeshes;
            for ( j = 0; j < pGeoSet->numMeshes; ++j )
            {
                if ( pMesh->apVertices )
                {
                    stdMemory_Free(pMesh->apVertices);
                }

                if ( pMesh->apTexVertices )
                {
                    stdMemory_Free(pMesh->apTexVertices);
                }

                if ( pMesh->aFaces )
                {
                    pFace = pMesh->aFaces;
                    for ( k = 0; k < pMesh->numFaces; ++k )
                    {
                        rdFace_FreeEntry(pFace);
                        ++pFace;
                    }

                    stdMemory_Free(pMesh->aFaces);
                }

                if ( pMesh->aVertColors )
                {
                    stdMemory_Free(pMesh->aVertColors);
                }

                if ( pMesh->aLightIntensities )
                {
                    stdMemory_Free(pMesh->aLightIntensities);
                }

                if ( pMesh->aVertNormals )
                {
                    stdMemory_Free(pMesh->aVertNormals);
                }

                ++pMesh;
            }

            if ( pGeoSet->aMeshes )
            {
                stdMemory_Free(pGeoSet->aMeshes);
            }

            ++pGeoSet;
        }

        if ( pModel3->aHierarchyNodes )
        {
            stdMemory_Free(pModel3->aHierarchyNodes);
        }

        if ( pModel3->apMaterials )
        {
            stdMemory_Free(pModel3->apMaterials);
        }
    }
    else
    {
#line 1382
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL model ptr.\n", 0, 0, 0, 0);
    }
}

// Discarded by the retail linker (called only by the discarded LoadPostProcess): debug 0x50f3da. No
// final return: the debug code falls off the end after the geoset loop.
int J3DAPI rdModel3_Validate(const rdModel3* pModel3)
{
    const rdModel3GeoSet* pGeo;
    size_t geoNum;
    const rdModel3Mesh* pMesh;
    size_t meshNum;
    const rdFace* pFace;
    size_t faceNum;

    pGeo = pModel3->aGeos;
    for ( geoNum = 0; geoNum < pModel3->numGeos; ++geoNum )
    {
        pMesh = pGeo->aMeshes;
        for ( meshNum = 0; meshNum < pGeo->numMeshes; ++meshNum )
        {
            pFace = pMesh->aFaces;
            for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
                if ( pFace->pMaterial )
                {
                    if ( pFace->geometryMode < RD_GEOMETRY_FULL )
                    {
                        return 0;
                    }
                }
                else if ( pFace->geometryMode > RD_GEOMETRY_NONE )
                {
                    return 0;
                }

                ++pFace;
            }

            ++pMesh;
        }

        ++pGeo;
    }
}

// Discarded by the retail linker (called only by the discarded LoadPostProcess): debug 0x50f4a4.
void J3DAPI rdModel3_CalcRadii(rdModel3* pModel3)
{
    float maxRadius;
    rdModel3Mesh* pMesh;
    size_t meshNum;
    const rdVector3* pVertex;
    size_t vertNum;
    float radius;

    maxRadius = 0.0f;
    pMesh = pModel3->aGeos[0].aMeshes;
    for ( meshNum = 0; meshNum < pModel3->aGeos[0].numMeshes; ++meshNum )
    {
        maxRadius = 0.0f;
        pVertex = pMesh->apVertices;
        for ( vertNum = 0; vertNum < pMesh->numVertices; ++vertNum )
        {
            radius = rdVector_Len3(pVertex);
            if ( radius > maxRadius )
            {
                maxRadius = radius;
            }

            ++pVertex;
        }

        pMesh->radius = maxRadius;
        ++pMesh;
    }

    maxGeoRadius = 0.0f;
    rdModel3_BuildExpandedRadius(pModel3, pModel3->aHierarchyNodes, &rdroid_identMatrix34);
    pModel3->radius = maxGeoRadius;
}

void J3DAPI rdModel3_BuildExpandedRadius(const rdModel3* pModel, const rdModel3HNode* pNode, const rdMatrix34* orient)
{
    rdMatrix34 tmat;

    rdMatrix34 nmat, norient;
    rdVector3 pivot;
    rdVector3 tvert;

    rdMatrix_Build34(&nmat, &pNode->pyr, &pNode->pos);
    rdMatrix_BuildTranslate34(&norient, &pNode->pivot);
    rdMatrix_PostMultiply34(&norient, &nmat);

    if ( pNode->pParent )
    {
        rdVector_Neg3(&pivot, &pNode->pParent->pivot);
        rdMatrix_PostTranslate34(&norient, &pivot);
    }

    
    rdMatrix_Multiply34(&tmat, orient, &norient);

    if ( pNode->meshIdx != -1 )
    {
        const rdModel3Mesh* pMesh = &pModel->aGeos[0].aMeshes[pNode->meshIdx];
        const rdVector3* pVertex   = pMesh->apVertices;
        size_t i;

        for ( i = 0; i < pMesh->numVertices; ++i )
        {
            float radius;

            rdMatrix_TransformPoint34(&tvert, pVertex, &tmat);

            if ( bCalcGeoRadius )
            {
                radius = rdVector_Len3(&tvert);
            }
            else
            {
                radius = tvert.z;
            }

            if ( radius > maxGeoRadius )
            {
                maxGeoRadius = radius;
            }

            ++pVertex;
        }
    }

    if ( pNode->numChildren > 0 )
    {
        const rdModel3HNode* pChild = pNode->pChild;
        size_t i;
        for ( i = 0; i < pNode->numChildren; ++i )
        {
            rdModel3_BuildExpandedRadius(pModel, pChild, &tmat);
            pChild = pChild->pSibling;
        }
    }
}

// Discarded by the retail linker (called only by the discarded LoadPostProcess): debug 0x50f7a8.
// The vertex counters are int, compared with the face's vertex count as int (debug `jge`/`jl`).
void J3DAPI rdModel3_CalcFaceNormals(rdModel3* pModel3)
{
    rdModel3GeoSet* pGeo;
    size_t geoNum;
    rdModel3Mesh* pMesh;
    size_t meshNum;
    rdFace* pFace;
    size_t faceNum;
    int vertNum;
    int prevVertNum;
    int nextVertNum;

    pGeo = pModel3->aGeos;
    for ( geoNum = 0; geoNum < pModel3->numGeos; ++geoNum )
    {
        pMesh = pGeo->aMeshes;
        for ( meshNum = 0; meshNum < pGeo->numMeshes; ++meshNum )
        {
            pFace = pMesh->aFaces;
            for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
            {
#line 1608
                RD_ASSERT(pFace->numVertices > 2);

                for ( vertNum = 0; vertNum < (int)pFace->numVertices; ++vertNum )
                {
                    prevVertNum = vertNum - 1;
                    if ( prevVertNum < 0 )
                    {
                        prevVertNum = pFace->numVertices - 1;
                    }

                    nextVertNum = (vertNum + 1) % pFace->numVertices;
                    if ( !rdMath_PointsCollinear(
                        &pMesh->apVertices[pFace->aVertices[vertNum]],
                        &pMesh->apVertices[pFace->aVertices[nextVertNum]],
                        &pMesh->apVertices[pFace->aVertices[prevVertNum]]) )
                    {
                        break;
                    }
                }

                if ( vertNum >= (int)pFace->numVertices )
                {
#line 1625
                    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid Face encountered while calculating normals in model: '%s'\n", pModel3->aName, 0, 0, 0);
                }
                else
                {
                    rdMath_CalcSurfaceNormal(
                        &pFace->normal,
                        &pMesh->apVertices[pFace->aVertices[vertNum]],
                        &pMesh->apVertices[pFace->aVertices[nextVertNum]],
                        &pMesh->apVertices[pFace->aVertices[prevVertNum]]);
                }

                ++pFace;
            }

            ++pMesh;
        }

        ++pGeo;
    }
}

// Discarded by the retail linker (called only by the discarded LoadPostProcess): debug 0x50f9b5, at its
// debug position before FindNamedNode. Its `numFaces + 1.0f` is the orphaned -1.0f of retail's pool
// (0x00509BB8; VC5 /O2 compiles `+ 1.0f` as `fsub [-1.0f]`). The one-face arm reads vertex 0 of the
// mesh, not this vertex (debug 0x50fb39).
void J3DAPI rdModel3_CalcVertexNormals(rdModel3* pModel)
{
    rdModel3GeoSet* pGeo;
    size_t geoNum;
    rdModel3Mesh* pMesh;
    size_t meshNum;
    const rdVector3* pVertex;
    rdVector3* pVertNormal;
    size_t vertNum;
    rdVector3 normal;
    float numFaces;
    const rdFace* pFace;
    size_t faceNum;
    size_t i;
    size_t prevVertNum;
    size_t nextVertNum;
    rdVector3 dprev;
    rdVector3 dnext;

    pGeo = pModel->aGeos;
    for ( geoNum = 0; geoNum < pModel->numGeos; ++geoNum )
    {
        pMesh = pGeo->aMeshes;
        for ( meshNum = 0; meshNum < pGeo->numMeshes; ++meshNum )
        {
            pVertex = pMesh->apVertices;
            pVertNormal = pMesh->aVertNormals;
            for ( vertNum = 0; vertNum < pMesh->numVertices; ++vertNum )
            {
                rdVector_Set3(&normal, 0.0f, 0.0f, 0.0f);
                numFaces = 0.0f;
                pFace = pMesh->aFaces;
                for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
                {
                    for ( i = 0; i < pFace->numVertices; ++i )
                    {
                        if ( pFace->aVertices[i] == vertNum )
                        {
                            rdVector_Add3Acc(&normal, &pFace->normal);
                            numFaces = numFaces + 1.0f;
                            break;
                        }
                    }

                    ++pFace;
                }

                if ( numFaces != 0.0f )
                {
                    if ( numFaces == 1.0f )
                    {
                        pFace = pMesh->aFaces;
                        prevVertNum = pFace->numVertices - 1;
                        nextVertNum = 1 % pFace->numVertices;
                        rdVector_Sub3(&dprev, pMesh->apVertices, &pMesh->apVertices[pFace->aVertices[prevVertNum]]);
                        rdVector_Sub3(&dnext, pMesh->apVertices, &pMesh->apVertices[pFace->aVertices[nextVertNum]]);
                        rdVector_Add3(pVertNormal, &dprev, &dnext);
                    }
                    else
                    {
                        pVertNormal->x = normal.x / numFaces;
                        pVertNormal->y = normal.y / numFaces;
                        pVertNormal->z = normal.z / numFaces;
                    }

                    rdVector_Normalize3Acc(pVertNormal);
                }
                else
                {
                    rdVector_Set3(pVertNormal, 1.0f, 0.0f, 0.0f);
#line 1735
                    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Unused vertex found while calculating vert normals.\n", 0, 0, 0, 0);
                }

                ++pVertex;
                ++pVertNormal;
            }

            ++pMesh;
        }

        ++pGeo;
    }
}

// Discarded by the retail linker (no caller in either build): debug 0x50fcea.
rdModel3HNode* J3DAPI rdModel3_FindNamedNode(const char* pName, rdModel3* pModel3)
{
    rdModel3HNode* pNode;
    size_t nodeNum;

    pNode = pModel3->aHierarchyNodes;
    for ( nodeNum = 0; nodeNum < pModel3->numHNodes; ++nodeNum )
    {
        if ( streqi(pNode->aName, pName) )
        {
            return pNode;
        }

        ++pNode;
    }

    return NULL;
}

int J3DAPI rdModel3_GetMeshMatrix(rdThing* pThing, const rdMatrix34* orient, unsigned int nodeNum, rdMatrix34* meshOrient)
{
#line 1808
    RD_ASSERTREL(pThing && meshOrient);
    RD_ASSERTREL(orient);
    RD_ASSERTREL(pThing->type == RD_THING_MODEL3);
    RD_ASSERTREL(pThing->data.pModel3);

    if ( nodeNum >= pThing->data.pModel3->numHNodes )
    {
        return 0;
    }

    if ( pThing->rdFrameNum != rdCache_GetFrameNum() )
    {
        rdPuppet_BuildJointMatrices(pThing, orient);
    }

    rdMatrix_Copy34(meshOrient, &pThing->paJointMatrices[nodeNum]);
    return 1;
}

int J3DAPI rdModel3_ReplaceMesh(rdModel3* pModel, unsigned int geosetNum, unsigned int meshNum, const rdModel3Mesh* pSrcMesh)
{
    rdModel3Mesh* pMesh;
    rdModel3GeoSet* pGeo;

    if ( geosetNum >= pModel->numGeos )
    {
        return 1;
    }

    pGeo = &pModel->aGeos[geosetNum];
    if ( meshNum >= pGeo->numMeshes )
    {
        return 1;
    }

    pMesh = &pGeo->aMeshes[meshNum];
    memcpy(pMesh, pSrcMesh, sizeof(rdModel3Mesh));
    return 0;
}

int J3DAPI rdModel3_SwapMesh(rdModel3* pModel1, unsigned int meshNum1, rdModel3* pModel2, int meshNum2)
{
    rdModel3GeoSet* pGeo1;
    rdModel3GeoSet* pGeo2;
    rdModel3Mesh* pMesh1;
    rdModel3Mesh* pMesh2;
    int bFailed1;
    int bFailed2;
    rdModel3Mesh mesh1;

    if ( !pModel1 || !pModel2 )
    {
        return 1;
    }

    pGeo1 = &pModel1->aGeos[0];
    pGeo2 = &pModel2->aGeos[0];
    pMesh1 = &pGeo1->aMeshes[meshNum1];
    pMesh2 = &pGeo2->aMeshes[meshNum2];

    memcpy(&mesh1, pMesh1, sizeof(mesh1));

    bFailed1 = rdModel3_ReplaceMesh(pModel1, 0, meshNum1, pMesh2);
    bFailed2 = rdModel3_ReplaceMesh(pModel2, 0, meshNum2, &mesh1);
    if ( !bFailed1 && !bFailed2 )
    {
        return 0;
    }

#line 1886
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: SwapMesh for models '%s and %s'.\n", pModel1->aName, pModel2->aName, 0, 0);
    return 1;
}

void J3DAPI rdModel3_GetThingColor(const rdThing* pThing, rdVector4* pDestColor)
{
    const rdModel3* pModel3;
    const rdModel3HNode* pNode;
    const rdModel3GeoSet* pGeo;
    size_t geoNum;

    pModel3 = pThing->data.pModel3;
    pNode = pModel3->aHierarchyNodes;
    if ( pThing->geosetNum != -1 )
    {
        geoNum = pThing->geosetNum;
    }
    else
    {
        geoNum = pModel3->curGeoNum;
    }

    pGeo = &pModel3->aGeos[geoNum];
    if ( pNode->meshIdx != -1 )
    {
        rdVector_Copy4(pDestColor, &pGeo->aMeshes[pNode->meshIdx].meshColor);
    }
    else if ( pNode->numChildren > 0 )
    {
        const rdModel3HNode* pChild;

        pChild = pNode->pChild;
        if ( pChild->meshIdx != -1 )
        {
            rdVector_Copy4(pDestColor, &pGeo->aMeshes[pChild->meshIdx].meshColor);
        }
        else
        {
            pDestColor->red   = 0.0f;
            pDestColor->green = 0.0f;
            pDestColor->blue  = 0.0f;
            pDestColor->alpha = 1.0f;
        }
    }
}

void J3DAPI rdModel3_SetModelColor(rdModel3* pModel, const rdVector4* pColor)
{
    const rdModel3HNode* pNode;
    rdModel3GeoSet* pGeo;
    size_t geoNum;

    pNode = pModel->aHierarchyNodes;
    geoNum = 0;
    pGeo = &pModel->aGeos[geoNum];
    rdModel3_SetNodeColor(pModel, pGeo, pNode, pColor);
}

void J3DAPI rdModel3_SetThingColor(rdThing* prdThing, const rdVector4* pColor)
{
    rdModel3* pModel3;
    const rdModel3HNode* pNode;
    rdModel3GeoSet* pGeo;
    size_t geoNum;

    if ( prdThing->type != RD_THING_MODEL3 )
    {
        return;
    }

    pModel3 = prdThing->data.pModel3;
    pNode = pModel3->aHierarchyNodes;
    if ( prdThing->geosetNum != -1 )
    {
        geoNum = prdThing->geosetNum;
    }
    else
    {
        geoNum = pModel3->curGeoNum;
    }

    pGeo = &pModel3->aGeos[geoNum];
    rdModel3_SetNodeColorForNonAmputatedJoints(prdThing, pGeo, pNode, pColor);
}

void J3DAPI rdModel3_SetNodeColor(const rdModel3* pModel, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor)
{
    size_t i;

    if ( pNode->meshIdx != -1 )
    {
        rdVector_Copy4(&pGeoSet->aMeshes[pNode->meshIdx].meshColor, pColor);
    }

    if ( pNode->numChildren > 0 )
    {
        rdModel3HNode* pChild;

        pChild = pNode->pChild;
        for ( i = 0; i < pNode->numChildren; ++i )
        {
            rdModel3_SetNodeColor(pModel, pGeoSet, pChild, pColor);
            pChild = pChild->pSibling;
        }
    }
}

void J3DAPI rdModel3_SetNodeColorForNonAmputatedJoints(const rdThing* prdThing, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor)
{
    size_t i;

    if ( pNode->meshIdx != -1 )
    {
        rdVector_Copy4(&pGeoSet->aMeshes[pNode->meshIdx].meshColor, pColor);
    }

    if ( pNode->numChildren > 0 )
    {
        rdModel3HNode* pChild;

        pChild = pNode->pChild;
        for ( i = 0; i < pNode->numChildren; ++i )
        {
            if ( !prdThing->paJointAmputationFlags[pChild->num] )
            {
                rdModel3_SetNodeColorForNonAmputatedJoints(prdThing, pGeoSet, pChild, pColor);
            }

            pChild = pChild->pSibling;
        }
    }
}

int J3DAPI rdModel3_Draw(rdThing* pThing, const rdMatrix34* pPlacement)
{
    rdModel3* pModel3;
    RdFrustumCull cull;
    size_t i;
    rdLight* pLight;
    rdModel3HNode* pRootNode;
    rdModel3GeoSet* pGeo;
    rdVector3 dist;
    size_t geoNum;

#line 2042
    RD_ASSERTREL(rdCamera_pCurCamera!=NULL);
    RD_ASSERTREL(pThing!=NULL);
    RD_ASSERTREL(pThing->data.pModel3!=NULL);

    rdModel3_numDrawnFaces = rdModel3_numDrawnAlphaFaces = 0;

    pCurThing = pThing;

    pModel3 = pThing->data.pModel3;
    cull = pThing->frustumCull;
    if ( cull != RDFRUSTUMCULL_OUTSIDE )
    {
        if ( pThing->geosetNum != -1 ) {
            geoNum = pThing->geosetNum;
        }
        else {
            geoNum = pModel3->curGeoNum;
        }

        if ( pThing->rdFrameNum != rdCache_GetFrameNum() || rdCamera_pCurCamera->pCanvas->pVBuffer->rasterInfo.colorInfo.bpp == 8 )
        {
            rdPuppet_BuildJointMatrices(pThing, pPlacement);
        }

        rdModel3_curLightingMode = rdroid_curLightingMode;
        if ( rdModel3_curLightingMode > RD_LIGHTING_LIT )
        {
            rdModel3_numGeoLights = 0;
            for ( i = 0; i < rdCamera_pCurCamera->numLights; ++i )
            {
                pLight = rdCamera_pCurCamera->aLights[i];
                // The light-to-placement vector is computed but never read: the range test after it is constant-true
                // in the debug build (0x510415) and the store is unconditional in release.
                rdVector_Sub3(&dist, &rdCamera_pCurCamera->aLightPositions[i], &pPlacement->dvec);
                rdModel3_apGeoLights[rdModel3_numGeoLights++] = pLight;
            }
        }

        pRootNode = pModel3->aHierarchyNodes;
        pGeo = &pModel3->aGeos[geoNum];
        rdModel3_DrawHNode(pGeo, pRootNode);
        return 1;
    }

    return 0;
}

void J3DAPI rdModel3_DrawHNode(const rdModel3GeoSet* prdGeo, const rdModel3HNode* pNode)
{
    if ( pNode->meshIdx != -1 )
    {
        rdModel3Mesh* pMesh = &prdGeo->aMeshes[pNode->meshIdx];
        if ( rdModel3K_IsEnabled() & (pMesh->someFaceFlags == 0) & (pMesh->meshColor.alpha >= 1.0f) )
        {
            rdModel3K_DrawMesh(pMesh, &pCurThing->paJointMatrices[pNode->num]);
        }
        else
        {
            rdModel3_DrawMesh(pMesh, &pCurThing->paJointMatrices[pNode->num]);
        }
    }

    if ( pNode->numChildren > 0 )
    {
        size_t i;
        {
            rdModel3HNode* pChild = pNode->pChild;
            for ( i = 0; i < pNode->numChildren; ++i )
            {
                if ( !pCurThing->paJointAmputationFlags[pChild->num] )
                {
                    rdModel3_DrawHNode(prdGeo, pChild);
                }

                pChild = pChild->pSibling;
            }
        }
    }
}

void J3DAPI rdModel3_DrawMesh(const rdModel3Mesh* pMesh, const rdMatrix34* orient)
{
    rdMatrix34 tmat;
    rdLight* pLight;
    rdVector3 dist;
    rdVector3 camPos;
    size_t i;
    float dot;
    rdVector3 dir;
    const rdFace* pFace;
    size_t faceNum;
    int bBackFace;
    rdVector3 tpos;
    rdMatrix34 orthOrient;

#line 2179
    RD_ASSERTREL(rdCamera_pCurCamera!=NULL);
    RD_ASSERTREL(pMesh!=NULL);

    rdModel3_pCurMesh = pMesh;

    if ( rdModel3_pCurMesh->geoMode )
    {
        rdMatrix_TransformPoint34(&tpos, &orient->dvec, &rdCamera_pCurCamera->orient);

        // Transform mesh vertices to camera space, oriented to orient
        rdMatrix_Multiply34(&tmat, &rdCamera_pCurCamera->orient, orient);
        rdMatrix_TransformPointList34(&tmat, rdModel3_pCurMesh->apVertices, aView, rdModel3_pCurMesh->numVertices);

        rdMatrix_InvertOrtho34(&orthOrient, orient);

        rdModel3_lightingMode = rdModel3_pCurMesh->lightMode < rdModel3_curLightingMode ? rdModel3_pCurMesh->lightMode : rdModel3_curLightingMode;

        switch ( rdModel3_lightingMode )
        {
            case RD_LIGHTING_DIFFUSE:
                // `dist` as in rdModel3_Draw: computed, never read (debug 0x510762, 0x510857: constant-true range test).
                rdModel3_numMeshLights = 0;
                for ( i = 0; i < rdModel3_numGeoLights; ++i )
                {
                    pLight = rdModel3_apGeoLights[i];
                    rdVector_Sub3(&dist, &rdCamera_pCurCamera->aLightPositions[pLight->num], &orient->dvec);
                    rdModel3_apMeshLights[rdModel3_numMeshLights] = pLight;
                    rdMatrix_TransformPoint34(&rdModel3_aLocalLightPos[rdModel3_numMeshLights], &rdCamera_pCurCamera->aLightPositions[pLight->num], &orthOrient);
                    ++rdModel3_numMeshLights;
                }
                break;

            case RD_LIGHTING_GOURAUD:
                rdModel3_numMeshLights = 0;
                for ( i = 0; i < rdModel3_numGeoLights; ++i )
                {
                    pLight = rdModel3_apGeoLights[i];
                    rdVector_Sub3(&dist, &rdCamera_pCurCamera->aLightPositions[pLight->num], &orient->dvec);
                    rdModel3_apMeshLights[rdModel3_numMeshLights] = pLight;
                    rdMatrix_TransformPoint34(&rdModel3_aLocalLightPos[rdModel3_numMeshLights], &rdCamera_pCurCamera->aLightPositions[pLight->num], &orthOrient);
                    ++rdModel3_numMeshLights;
                }

                rdLight_CalcVertexIntensities(
                    rdCamera_pCurCamera->aLights,
                    rdModel3_aLocalLightPos,
                    rdCamera_pCurCamera->numLights,
                    rdModel3_pCurMesh->aVertNormals,
                    rdModel3_pCurMesh->apVertices,
                    rdModel3_pCurMesh->aVertColors,
                    rdModel3_pCurMesh->aLightIntensities,
                    rdModel3_pCurMesh->numVertices,
                    rdCamera_pCurCamera->attenuationMin
                );

                if ( bDrawMeshBounds )
                {
                    // Draw mesh bounds
                    rdPrimit3_DrawClippedCircle(&orient->dvec, rdModel3_pCurMesh->radius, 60.0f, 0xFFFF3F2F, 0xFFFFFFFF);
                }
                break;
        }

        camPos = rdCamera_camMatrix.dvec;
        rdMatrix_TransformPoint34(&rdModel3_localCamera, &camPos, &orthOrient);

        pFace = pMesh->aFaces;
        for ( faceNum = 0; faceNum < pMesh->numFaces; ++faceNum )
        {
            rdVector_Sub3(&dir, &rdModel3_localCamera, &rdModel3_pCurMesh->apVertices[*pFace->aVertices]);

            dot = rdVector_Dot3(&pFace->normal, &dir);
            if ( dot > 0.0f )
            {
                bBackFace = 0;
            }
            else
            {
                bBackFace = 1;
            }

            if ( !bBackFace || (pFace->flags & RD_FF_DOUBLE_SIDED) != 0 || (rdroid_curRenderOptions & RDROID_BACKFACE_CULLING_ENABLED) == 0 )
            {
                pfDrawFace(pFace, aView, bBackFace, &pMesh->meshColor);
            }

            ++pFace;
        }
    }
}

void J3DAPI rdModel3_DrawFace(const rdFace* pFace, const rdVector3* aTransformedVertices, int bIsBackFace, const rdVector4* pMeshColor)
{
    rdCacheProcEntry* pPoly;
    rdVector4 faceIntensity;

    int bTranslucent = pFace->flags & RD_FF_TEX_TRANSLUCENT;
    size_t i;
    if ( pFace->pMaterial )
    {
        bTranslucent |= pFace->pMaterial->formatType == STDCOLOR_FORMAT_RGBA;
    }

    if ( pMeshColor->alpha != 1.0f )
    {
        bTranslucent = 1;
    }

    
    if ( bTranslucent )
    {
        pPoly = rdCache_GetAlphaProcEntry();
    }
    else
    {
        pPoly = rdCache_GetProcEntry();
    }

    pPoly->lightingMode = rdModel3_lightingMode < pFace->lightingMode ? rdModel3_lightingMode : pFace->lightingMode;

    // Clip/transform to clip space
    // Fyi, grimengine uses either rdPrim3_ClipFace or rdPrim3_NoClipFace because it additionally clips away meshes polys that are not in clip frustum 
    if ( rdClip_FaceToPlane(rdCamera_pCurCamera->pFrustum, pPoly, pFace, aTransformedVertices, rdModel3_pCurMesh->apTexVertices, rdModel3_pCurMesh->aLightIntensities, NULL) )
    {
        size_t numVertices = pFace->numVertices;

        rdVector_Copy4(&pPoly->extraLight, &pFace->extraLight);

        switch ( pPoly->lightingMode )
        {
        case RD_LIGHTING_DIFFUSE:
        {
            // Note: grim engine also uses pre-calculated light directions (aLocalLightDir)

            rdVector3 faceNormal;

            // TODO: use rdModel3_apMeshLights and rdModel3_numMeshLights instead of the camera lights
            if ( bIsBackFace )
            {
                rdVector_Neg3(&faceNormal, &pFace->normal);
                rdLight_CalcFaceIntensity(rdCamera_pCurCamera->aLights, rdModel3_aLocalLightPos, rdCamera_pCurCamera->numLights, pFace, &faceNormal, rdModel3_pCurMesh->apVertices, rdCamera_pCurCamera->attenuationMin, &faceIntensity);
            }
            else
            {
                rdLight_CalcFaceIntensity(rdCamera_pCurCamera->aLights, rdModel3_aLocalLightPos, rdCamera_pCurCamera->numLights, pFace, &pFace->normal, rdModel3_pCurMesh->apVertices, rdCamera_pCurCamera->attenuationMin, &faceIntensity);
            }

            rdVector_Add4Acc(&pPoly->extraLight, &faceIntensity);
        }
        break;
        }

        if ( (rdroid_curRenderOptions & RDROID_USE_AMBIENT_CAMERA_LIGHT) != 0 )
        {
            rdVector_Add4Acc(&pPoly->extraLight, &rdCamera_pCurCamera->ambientLight);
        }

        if ( pMeshColor->alpha != 1.0f )
        {
            pPoly->extraLight.alpha = 1.0f;
            for ( i = 0; i < numVertices; ++i )
            {
                pPoly->aVertIntensities[i].alpha = pMeshColor->alpha;
            }
        }

        pPoly->matCelNum = pFace->matCelNum;
        pPoly->flags     = pFace->flags;
        pPoly->flags    |= rdModel3_extraFaceFlags;

        if ( pMeshColor->alpha != 1.0f ) {
            pPoly->flags |= RD_FF_TEX_TRANSLUCENT;
        }

        pPoly->pMaterial = pFace->pMaterial;

        if ( bTranslucent )
        {
            ++rdModel3_numDrawnAlphaFaces;
            rdCache_AddAlphaProcFace(numVertices);
        }
        else
        {
            ++rdModel3_numDrawnFaces;
            rdCache_AddProcFace(numVertices);
        }
    }
}

void J3DAPI rdModel3_EnableFogRendering(int bEnabled)
{
    if ( bEnabled )
    {
        rdModel3_extraFaceFlags = RD_FF_FOG_ENABLED;
    }
    else
    {
        rdModel3_extraFaceFlags = 0;
    }
}

// Discarded by the retail linker (no caller in either build): debug 0x510ddb, the file's last function,
// the setter of pfDrawFace (retail .data, called by DrawMesh, never written in retail). No return value.
void J3DAPI rdModel3_RegisterFaceDraw(rdModel3DrawFaceFunc pfFunc)
{
    pfDrawFace = pfFunc;
}
