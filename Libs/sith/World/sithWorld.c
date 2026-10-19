#include "sithWorld.h"

#include <rdroid/Engine/rdMaterial.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAIClass.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithSound.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithPVS.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdFnames.h>
#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>





#define SITHWORLD_COPYRIGHTLINELEN  32
#define SITHWORLD_NUMCOPYRIGHTLINES 38

#define SITHWORLD_MAXTEXTSECTIONPARSERS 32

#define SITHWORLD_NDYMINVERSION 2
#define SITHWORLD_NDYMAXVERSION 3
#define SITHWORLD_CNDVERSION    3

// A "mocking" world struct which is stored to cnd files.
// Stores all pointers as uint32_t type
typedef struct sCndWorld
{
    char aCopyright[1216];
    char aName[64];
    SithWorldState state;
    int version;
    float gravity;
    float ceilingSkyHeight;
    float horizonDistance;
    rdVector2 horizonSkyOffset;
    rdVector2 ceilingSkyOffset;
    float distancesLOD[4];
    SithWorldFog fog;
    uint32_t numSounds;
    uint32_t numMaterials;
    uint32_t sizeMaterials;
    uint32_t aMaterials;
    uint32_t apMatArray;
    uint32_t numVertices;
    uint32_t aVertices;
    uint32_t aTransformedVertices;
    uint32_t aVertexRenderTickIds;
    uint32_t numTexVertices;
    uint32_t aTexVerticies;
    uint32_t aVertDynamicLights;
    uint32_t aVertDynamicLightsRenderTicks;
    uint32_t numAdjoins;
    uint32_t aAdjoins;
    uint32_t numSurfaces;
    uint32_t aSurfaces;
    uint32_t numSectors;
    uint32_t aSectors;
    size_t numAIClasses;
    size_t sizeAIClasses;
    uint32_t aAIClasses;
    uint32_t numModels;
    uint32_t sizeModels;
    uint32_t aModels;
    size_t numSprites;
    size_t sizeSprites;
    uint32_t aSprites;
    uint32_t numKeyframes;
    uint32_t sizeKeyframes;
    uint32_t aKeyframes;
    uint32_t numPuppetClasses;
    uint32_t sizePuppetClasses;
    uint32_t aPuppetClasses;
    uint32_t numSoundClasses;
    uint32_t sizeSoundClasses;
    uint32_t aSoundClasses;
    uint32_t numCogScripts;
    uint32_t sizeCogScripts;
    uint32_t aCogScripts;
    uint32_t numCogs;
    uint32_t sizeCogs;
    uint32_t aCogs;
    uint32_t numThingTemplates;
    uint32_t sizeThingTemplates;
    uint32_t aThingTemplates;
    uint32_t numThings;
    int32_t lastThingIdx;
    uint32_t aThings;
    uint32_t sizePVS;
    uint32_t aPVS;
} CndWorld;

static const char aCopyrightSymbol[1216] =
{
    "................................"
    "................@...@...@...@..."
    ".............@...@..@..@...@...."
    "................@.@.@.@.@.@....."
    "@@@@@@@@......@...........@....."
    "@@@@@@@@....@@......@@@....@...."
    "@@.....@.....@......@@@.....@@.."
    "@@.@@@@@......@.....@@@......@@."
    "@@@@@@@@.......@....@@.....@@..."
    "@@@@@@@@.........@@@@@@@@@@....."
    "@@@@@@@@..........@@@@@@........"
    "@@.....@..........@@@@@........."
    "@@.@@@@@.........@@@@@@........."
    "@@.....@.........@@@@@@........."
    "@@@@@@@@.........@@@@@@........."
    "@@@@@@@@.........@@@@@@@........"
    "@@@...@@.........@@@@@@@........"
    "@@.@@@.@.........@.....@........"
    "@@..@..@........@.......@......."
    "@@@@@@@@........@.......@......."
    "@@@@@@@@.......@........@......."
    "@@..@@@@.......@........@......."
    "@@@@..@@......@.........@......."
    "@@@@.@.@......@.........@......."
    "@@....@@........................"
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
    "@@@@@@@@@@@@@.@@@@@@@@@@@@@@@@@@"
    "@@.@@..@@@@@..@@@@@@@@@@.@@@@@@@"
    "@@.@.@.@@@@.@.@@@.@..@@...@@@..@"
    "@@..@@@@@@....@@@..@@@@@.@@@@.@@"
    "@@@@@@@@...@@.@@@.@@@@@..@@...@@"
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
    "@.(c).lucasarts.entertainment..@"
    "@.........company.llc..........@"
    "@....(c).lucasfilm.ltd.&.tm....@"
    "@.....all.rights.reserved......@"
    "@...used.under.authorization...@"
    "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
};

// Retail's BSS block 0x00561340-0x005624A8 is one contiguous run in this
// declaration order, with the four public globals defined inside it. Every
// definition is explicitly zero-initialized: VC5 emits uninitialized file
// statics first in identifier-hash order and zero-initialized definitions
// after them in declaration order. The 4-byte
// holes at 0x00561344 and 0x00562094 are 8-byte alignment before the two
// large char arrays.
static SithWorldLoadProgressCallback pfLoadProgressCallback = NULL;
static char aCopyrightBuf[1216] = { 0 };
static SithWorldTextSectionParseHandler aTextSectioParseHandlers[SITHWORLD_MAXTEXTSECTIONPARSERS] = { 0 };

SithWorld* sithWorld_pCurrentWorld = NULL;
SithWorld* sithWorld_pStaticWorld = NULL;
SithWorld* sithWorld_pLastLoadedWorld = NULL;

static char aCurSection[1024] = { 0 };

int sithWorld_bLoading = 0;

static int numTextSectionParseHandlers = 0;
static int bWorldStartup = 0;
static int bValidCopyright = 0;

static SithWorldBinarySectionParser aBinarySectionParsers[17] =
{
    {
        "sounds",
        &sithSound_ReadSoundsListBinary,
        &sithSound_WriteSoundsListBinary
    },
    {
        "materials",
        &sithMaterial_ReadMaterialsListBinary,
        &sithMaterial_WriteMaterialsListBinary
    },
    {
        "georesource",
        &sithWorld_ReadGeoresourceBinary,
        &sithWorld_WriteGeoresourceBinary
    },
    {
        "adjoins",
        &sithSurface_ReadAdjoinsListBinary,
        &sithSurface_WriteAdjoinsListBinary
    },
    {
        "surfaces",
        &sithSurface_ReadSurfacesListBinary,
        &sithSurface_WriteSurfacesListBinary
    },
    {
        "sectors",
        &sithSector_ReadSectorsListBinary,
        &sithSector_WriteSectorsListBinary
    },
    {
        "aiclasses",
        &sithAIClass_ReadStaticAIClassesListBinary,
        &sithAIClass_WriteStaticAIClassesListBinary
    },
    {
        "models",
        &sithModel_ReadStaticModelsListBinary,
        &sithModel_WriteStaticModelsListBinary
    },
    {
        "sprites",
        &sithSprite_ReadStaticSpritesListBinary,
        &sithSprite_WriteStaticSpritesListBinary
    },
    {
        "keyframes",
        &sithPuppet_ReadStaticKeyframesListBinary,
        &sithPuppet_WriteStaticKeyframesListBinary
    },
    {
        "animclass",
        &sithPuppet_ReadStaticPuppetsListBinary,
        &sithPuppet_WriteStaticPuppetsListBinary
    },
    {
        "soundclasses",
        &sithSoundClass_ReadSoundClassesListBinary,
        &sithSoundClass_WriteSoundClassesListBinary
    },
    {
        "cogcripts",
        &sithCog_ReadCogScriptsListBinary,
        &sithCog_WriteCogScriptsListBinary
    },
    {
        "cogs",
        &sithCog_ReadCogsListBinary,
        &sithCog_WriteCogsListBinary
    },
    {
        "templates",
        &sithTemplate_ReadThingTemplatesListBinary,
        &sithTemplate_WriteThingTemplatesListBinary
    },
    {
        "things",
        &sithThing_ReadStaticThingsListBinary,
        &sithThing_WriteStaticThingsListBinary
    },
    {
        "pvs",
        &sithPVS_ReadPVSBinary,
        &sithPVS_WritePVSBinary
    }
};


int J3DAPI sithWorld_Startup(void)
{
#line 179
    SITH_ASSERTREL(bWorldStartup == 0);

    numTextSectionParseHandlers = 0;

    if ( sithWorld_RegisterTextSectionParser("copyright", sithWorld_ReadCopyrightText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("header", sithWorld_ReadHeaderText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("sounds", sithSound_ReadSoundsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("materials", sithMaterial_ReadMaterialsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("georesource", sithWorld_ReadGeoresourceText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("sectors", sithSector_ReadSectorsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("aiclass", sithAIClass_ReadStaticAIClassesListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("models", sithModel_ReadStaticModelsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("sprites", sithSprite_ReadStaticSpritesListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("keyframes", sithPuppet_ReadStaticKeyframesListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("animclass", sithPuppet_ReadStaticPuppetsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("soundclass", sithSoundClass_ReadSoundClassesListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("cogscripts", sithCog_ReadCogScriptsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("cogs", sithCog_ReadCogsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("templates", sithTemplate_ReadThingTemplatesListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("things", sithThing_ReadStaticThingsListText) )
    {
        goto error;
    }

    if ( sithWorld_RegisterTextSectionParser("pvs", sithPVS_ReadPVSText) )
    {
        goto error;
    }

    bWorldStartup = 1;
    return 1;

error:
    return 0;
}

void J3DAPI sithWorld_Shutdown(void)
{
#line 223
    SITH_ASSERTREL(bWorldStartup == 1);
    if ( sithWorld_pCurrentWorld )
    {
        stdMemory_Free(sithWorld_pCurrentWorld);
    }

    if ( sithWorld_pStaticWorld )
    {
        stdMemory_Free(sithWorld_pStaticWorld);
    }

    sithWorld_pCurrentWorld    = NULL;
    sithWorld_pStaticWorld     = NULL;
    sithWorld_pLastLoadedWorld = NULL;

    bWorldStartup = 0;
}

void J3DAPI sithWorld_SetLoadProgressCallback(SithWorldLoadProgressCallback pfProgressCallback)
{
    pfLoadProgressCallback = pfProgressCallback;
}

void J3DAPI sithWorld_UpdateLoadProgress(float progress)
{
    if ( pfLoadProgressCallback )
    {
        pfLoadProgressCallback(progress);
    }
}

int J3DAPI sithWorld_Load(SithWorld* pWorld, const char* pFilename)
{
    int result;

#line 292
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(bWorldStartup==1);

    if ( !pWorld )
    {
        return 1;
    }

    if ( !pFilename )
    {
    }
    else
    {
        sithWorld_pLastLoadedWorld = pWorld;
        sithWorld_bLoading         = 1;

        if ( !strcmp(stdFnames_FindExt(pFilename), "cnd") )
        {
            result = sithWorld_LoadEntryBinary(pWorld, pFilename);
        }
        else
        {
            result = sithWorld_LoadEntryText(pWorld, pFilename, /*bSkipParsing=*/0);
        }

        if ( result )
        {
#line 318
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse problem in file '%s'.\n", pFilename, 0, 0, 0);
            goto error;
        }
    }

    if ( sithWorld_LoadPostProcess(pWorld) )
    {
        // Post-process error
        if ( pFilename )
        {
#line 327
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Post-load processing failed for %s.\n", pFilename, 0, 0, 0);
        }
        else
        {
#line 329
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Post-load processing failed.\n", 0, 0, 0, 0);
        }
    }
    else
    {
        // Success
        sithWorld_bLoading = 0;
        return 0;
    }

error:
    sithWorld_bLoading = 0;
    return 1;
}

int J3DAPI sithWorld_LoadPostProcess(SithWorld* pWorld)
{
    SithSurfaceAdjoin* pPrevAdjoin = NULL;
    SithSector* pPrevSector = NULL;
    int bNoCurWorld = 0;
    size_t surfNumber;
    SithSurface* pSurf;
    SithSurfaceAdjoin* pAdjoin;
    size_t i;
    SithThing* pThing;

    if ( (pWorld->state & SITH_WORLD_STATE_INITIALIZED) != 0 )
    {
        return 0;
    }

    pWorld->state |= SITH_WORLD_STATE_INIT_HUD;
    pWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;

    if ( pWorld->numVertices )
    {
        // Alloc render arrays
#line 377
        pWorld->aTransformedVertices = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pWorld->numVertices);
        if ( !pWorld->aTransformedVertices )
        {
            goto alloc_error;
        }

#line 382
        pWorld->aVertDynamicLights = (rdVector4*)STDMALLOC(sizeof(rdVector4) * pWorld->numVertices);
        if ( !pWorld->aVertDynamicLights )
        {
            goto alloc_error;
        }

        memset(pWorld->aVertDynamicLights, 0, sizeof(rdVector4) * pWorld->numVertices);
#line 388
        pWorld->aVertexRenderTickIds = (uint32_t*)STDMALLOC(sizeof(uint32_t) * pWorld->numVertices);
        if ( !pWorld->aVertexRenderTickIds )
        {
            goto alloc_error;
        }

        memset(pWorld->aVertexRenderTickIds, 0, sizeof(uint32_t) * pWorld->numVertices);
#line 394
        pWorld->aVertDynamicLightsRenderTicks = (uint32_t*)STDMALLOC(sizeof(uint32_t) * pWorld->numVertices);
        if ( !pWorld->aVertDynamicLightsRenderTicks )
        {
            goto alloc_error;
        }

        memset(pWorld->aVertDynamicLightsRenderTicks, 0, sizeof(uint32_t) * pWorld->numVertices);

        // Init adjoins
        for ( surfNumber = 0, pSurf = pWorld->aSurfaces; surfNumber < pWorld->numSurfaces; ++surfNumber, ++pSurf )
        {
            if ( pSurf->pAdjoin )
            {
                pAdjoin = pSurf->pAdjoin;
                if ( pAdjoin->pMirrorAdjoin )
                {
                    pAdjoin->pAdjoinSector = pAdjoin->pMirrorAdjoin->pAdjoinSurface->pSector;
                }

                if ( pPrevAdjoin && pPrevSector == pSurf->pSector )
                {
                    pPrevAdjoin->pNextAdjoin = pAdjoin;
                }
                else
                {
                    pSurf->pSector->pFirstAdjoin = pAdjoin;
                }

                pPrevAdjoin = pAdjoin;
                pPrevSector = pSurf->pSector;
            }
        }

        // Init players
        sithPlayer_PlacePlayers(pWorld);

        // Find floor for things
        if ( !sithWorld_pCurrentWorld )
        {
            bNoCurWorld = 1;
            sithWorld_pCurrentWorld = pWorld;
        }

        for ( i = 0; i < pWorld->numThings; ++i )
        {
            pThing = &pWorld->aThings[i];
            if ( pThing->type && pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & (SITH_PF_WALLSTICK | SITH_PF_FLOORSTICK)) != 0 )
            {
                sithPhysics_FindFloor(pThing, 1);
            }
        }

        if ( bNoCurWorld )
        {
            sithWorld_pCurrentWorld = NULL;
        }

        // Validate world
        if ( sithWorld_ValidateWorld(pWorld) )
        {
#line 450
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Some required sections not found or loaded in %s.\n", pWorld->aName, 0, 0, 0);
            goto error;
        }
    }

    pWorld->state |= SITH_WORLD_STATE_INITIALIZED;
    return 0;

alloc_error:
#line 463
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, pWorld->aName, 0, 0);

error:
    return 1;
}

// Retail lays the open-failure (550), missing-copyright (544) and section-error (554)
// diagnostics out as blocks after the success print (538), numbered in that trailing
// order; written with one label per block under the text-loader shared-error-block goto
// style of the text loaders.
int J3DAPI sithWorld_LoadEntryText(SithWorld* pWorld, const char* pFilename, int bSkipParsing)
{
    uint32_t startTime;
    uint32_t endTime;
    char aSection[64];

    // Function reads NDY level file

    bValidCopyright = 0;

    // Set world name
    strncpy(pWorld->aName, stdFnames_FindMedName(pFilename), STD_ARRAYLEN(pWorld->aName) - 1);
    pWorld->aName[STD_ARRAYLEN(pWorld->aName) - 1] = 0;
    stdUtil_ToLower(pWorld->aName);

    startTime = stdPlatform_GetTimeMsec();
    if ( !stdConffile_Open(pFilename) )
    {
        goto open_error;
    }

    // Read NDY file sections
    while ( stdConffile_ReadLine() )
    {
        // Note: retail's format has no field width, so a section name longer than
        //       aSection overflows the buffer. Retail bug, kept.
        if ( sscanf(stdConffile_aLine, " section: %s", aSection) != 1 )
        {
            continue;
        }

        strcpy(aCurSection, aSection);
        if ( sithWorld_ReadTextSection(pWorld, aSection, bSkipParsing) )
        {
#line 522
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Section '%s' contained errors.\n", aSection, 0, 0, 0);
            goto syntax_error;
        }
    }

    sithWorld_UpdateLoadProgress(100.0f);

    if ( !bValidCopyright )
    {
        goto copyright_error;
    }

    stdConffile_Close();

    endTime = stdPlatform_GetTimeMsec();
#line 538
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "%f seconds to load %s.\n", (endTime - startTime) / 1000.0, pWorld->aName, 0, 0);

    return 0;

copyright_error:
#line 544
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Load of file '%s' failed due to missing copyright.\n", pFilename, 0, 0, 0);
    stdConffile_Close();
    return 1;

open_error:
#line 550
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid load filename '%s'.\n", pFilename, 0, 0, 0);
    return 1;

syntax_error:
#line 554
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Load of file %s failed due to syntax errors.\n", pFilename, 0, 0, 0);
    stdConffile_Close();
    return 1;
}

void J3DAPI sithWorld_Close(SithWorld* pWorld)
{
#line 573
    SITH_ASSERTREL(bWorldStartup == 1);
    if ( pWorld )
    {
        sithWorld_Free(pWorld);
    }
}

SithWorld* J3DAPI sithWorld_NewEntry(void)
{
#line 598
    SithWorld* pWorld = (SithWorld*)STDMALLOC(sizeof(SithWorld));
    if ( !pWorld )
    {
        return NULL;
    }

    memset(pWorld, 0, sizeof(SithWorld));
    return pWorld;
}

void J3DAPI sithWorld_Free(SithWorld* pWorld)
{
#line 623
    SITH_ASSERTREL(pWorld);

    if ( pWorld->aPVS )
    {
        stdMemory_Free(pWorld->aPVS);
        pWorld->aPVS = NULL;
    }

    if ( pWorld->aThings )
    {
        sithThing_FreeWorldThings(pWorld);
    }

    if ( pWorld->aSectors )
    {
        sithSector_FreeWorldSectors(pWorld);
    }

    if ( pWorld->aModels )
    {
        sithModel_FreeWorldModels(pWorld);
    }

    if ( pWorld->aSprites )
    {
        sithSprite_FreeWorldSprites(pWorld);
    }

    if ( pWorld->aParticles )
    {
        sithParticle_FreeWorldParticles(pWorld);
    }

    if ( pWorld->aKeyframes )
    {
        sithPuppet_FreeWorldKeyframes(pWorld);
    }

    if ( pWorld->aThingTemplates )
    {
        sithTemplate_FreeWorldTemplates(pWorld);
    }

    if ( pWorld->aVertices )
    {
        stdMemory_Free(pWorld->aVertices);
        pWorld->aVertices = NULL;
    }

    if ( pWorld->aTransformedVertices )
    {
        stdMemory_Free(pWorld->aTransformedVertices);
        pWorld->aTransformedVertices = NULL;
    }

    if ( pWorld->aVertDynamicLights )
    {
        stdMemory_Free(pWorld->aVertDynamicLights);
        pWorld->aVertDynamicLights = NULL;
    }

    if ( pWorld->aVertDynamicLightsRenderTicks )
    {
        stdMemory_Free(pWorld->aVertDynamicLightsRenderTicks);
        pWorld->aVertDynamicLightsRenderTicks = NULL;
    }

    if ( pWorld->aTexVerticies )
    {
        stdMemory_Free(pWorld->aTexVerticies);
        pWorld->aTexVerticies = NULL;
    }

    if ( pWorld->aSurfaces )
    {
        sithSurface_FreeWorldSurfaces(pWorld);
    }

    if ( pWorld->aVertexRenderTickIds )
    {
        stdMemory_Free(pWorld->aVertexRenderTickIds);
        pWorld->aVertexRenderTickIds = NULL;
    }

    if ( pWorld->aMaterials )
    {
        sithMaterial_FreeWorldMaterials(pWorld);
    }

    sithSound_FreeWorldSounds(pWorld);
    if ( pWorld->aCogs || pWorld->aCogScripts )
    {
        sithCog_FreeWorldCogs(pWorld);
    }

    if ( pWorld->aPuppetClasses )
    {
        sithPuppet_FreeWorldPuppets(pWorld);
    }

    if ( pWorld->aAIClasses )
    {
        sithAIClass_FreeWorldAIClasses(pWorld);
    }

    if ( pWorld->aSoundClasses )
    {
        sithSoundClass_FreeWorldSoundClasses(pWorld);
    }

    stdMemory_Free(pWorld);
}

int J3DAPI sithWorld_ReadHeaderText(SithWorld* pWorld, int bSkip)
{
    uint32_t version;
    const char* name;
    float pixperRev;

    if ( bSkip )
    {
        return 1;
    }

    name = pWorld->aName;

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "version %d", &version) == 1 )
    {
        switch ( version )
        {
            case SITHWORLD_NDYMINVERSION:
            case SITHWORLD_NDYMAXVERSION:
                break;

            default:
#line 777
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid version %.2f for file '%s'.\n", version, name, 0, 0); // Note: retail passes the int version to a %.2f conversion
                goto syntax_error;
        }
    }
    else
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "world gravity %f", &pWorld->gravity) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "ceiling sky z %f", &pWorld->ceilingSkyHeight) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "horizon distance %f", &pWorld->horizonDistance) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "horizon pixels per rev %f", &pixperRev) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "horizon sky offset %f %f", &pWorld->horizonSkyOffset, &pWorld->horizonSkyOffset.y) != 2 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "ceiling sky offset %f %f", &pWorld->ceilingSkyOffset, &pWorld->ceilingSkyOffset.y) != 2 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "lod distances %f %f %f %f", pWorld->distancesLOD, &pWorld->distancesLOD[1], &pWorld->distancesLOD[2], &pWorld->distancesLOD[3]) != 4 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, "fog %d %f %f %f %f %f %f\n", &pWorld->fog.bEnabled, &pWorld->fog.color.red, &pWorld->fog.color.green, &pWorld->fog.color.blue, &pWorld->fog.color.alpha, &pWorld->fog.startDepth, &pWorld->fog.endDepth) != 7 )
    {
        goto syntax_error;
    }

    return 0;

syntax_error:
#line 826
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, name, 0, 0);
    return 1;
}

/* Linker-discarded (no retail symbol); body from the Indy3D debug build (0x431b8f, called by the debug world writer at
   0x4e9b13). The debug code jumps from every write failure to one `return 1` after `return 0`, all top-level sites
   (an open question): written with structured returns here. */
int J3DAPI sithWorld_WriteHeaderText(const SithWorld* pWorld)
{
    if ( stdConffile_WriteLine("###### Header information ######\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("SECTION: HEADER\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("# version and global constant settings\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("Version          %d\n", SITHWORLD_CNDVERSION) )
    {
        goto error;
    }

    if ( stdConffile_Printf("World Gravity    %.2f\n", pWorld->gravity) )
    {
        goto error;
    }

    if ( stdConffile_Printf("Ceiling Sky Z    %.6f\n", pWorld->ceilingSkyHeight) )
    {
        goto error;
    }

    if ( stdConffile_Printf("Horizon Distance %.6f\n", pWorld->horizonDistance) )
    {
        goto error;
    }

    if ( stdConffile_Printf("Horizon Pixels per Rev %.6f\n", 768.0) )
    {
        goto error;
    }

    if ( stdConffile_Printf("Horizon Sky Offset   %.6f %.6f\n", pWorld->horizonSkyOffset.x, pWorld->horizonSkyOffset.y) )
    {
        goto error;
    }

    if ( stdConffile_Printf("Ceiling Sky Offset   %.6f %.6f\n", pWorld->ceilingSkyOffset.x, pWorld->ceilingSkyOffset.y) )
    {
        goto error;
    }

    if ( stdConffile_Printf("LOD Distances\t\t%.6f\t%.6f\t%.6f\t%.6f\n", pWorld->distancesLOD[0], pWorld->distancesLOD[1], pWorld->distancesLOD[2], pWorld->distancesLOD[3]) )
    {
        goto error;
    }

    if ( stdConffile_Printf("fog\t\t%d\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\n", pWorld->fog.bEnabled, pWorld->fog.color.red, pWorld->fog.color.green, pWorld->fog.color.blue, pWorld->fog.color.alpha, pWorld->fog.startDepth, pWorld->fog.endDepth) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWorld_ReadCopyrightText(SithWorld* pWorld, int bSkip)
{
    size_t i;
    const char* name;

    if ( bSkip )
    {
        return 1;
    }

    name = pWorld->aName;

    for ( i = 0; i < SITHWORLD_NUMCOPYRIGHTLINES; ++i )
    {
        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        memcpy(&aCopyrightBuf[SITHWORLD_COPYRIGHTLINELEN * i], stdConffile_aLine, SITHWORLD_COPYRIGHTLINELEN); // Note, must be exact size, no null term
    }

    if ( memcmp(aCopyrightBuf, aCopyrightSymbol, STD_ARRAYLEN(aCopyrightSymbol)) )
    {
    }
    else
    {
        // Success
        bValidCopyright = 1;
        return 0;
    }

#line 910
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid copyright symbol.\n", stdConffile_aLine, name, 0, 0);
    bValidCopyright = 0;
    goto error;

syntax_error:
#line 916
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, name, 0, 0);

error:
    return 1;
}

void J3DAPI sithWorld_GetMemoryUsage(const SithWorld* pWorld, size_t(*aMemUsed)[17], size_t(*aCount)[17])
{
    size_t i;
    size_t j;

#line 945
    SITH_ASSERTREL(aMemUsed && aCount);
    SITH_ASSERTREL(pWorld);

    memset(aMemUsed, 0, sizeof(*aMemUsed));
    memset(aCount, 0, sizeof(*aCount));

    (*aCount)[0] = pWorld->numMaterials;
    for ( i = 0; i < pWorld->numMaterials; ++i )
    {
        (*aMemUsed)[0] += rdMaterial_GetMaterialMemUsage(&pWorld->aMaterials[i]);
    }

    (*aCount)[1] = pWorld->numVertices;
    (*aMemUsed)[1] = ((3 * sizeof(rdVector3)) + sizeof(rdVector4)) * pWorld->numVertices;

    (*aCount)[2] = pWorld->numTexVertices;
    (*aMemUsed)[2] = sizeof(rdVector2) * pWorld->numTexVertices;

    (*aCount)[3] = pWorld->numSurfaces;
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        (*aMemUsed)[3] += (sizeof(rdVector3) * pWorld->aSurfaces[i].face.numVertices) + sizeof(SithSurface);// TODO: Shouldn't it be (sizeof(rdVector3) + sizeof(rdVector2)) * numVertices like for sprite?
    }

    (*aCount)[4] = pWorld->numAdjoins;
    (*aMemUsed)[4] = sizeof(SithSurfaceAdjoin) * pWorld->numAdjoins;

    (*aCount)[5] = pWorld->numSectors;
    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        (*aMemUsed)[5] += (sizeof(*pWorld->aSectors[i].aVertIdxs) * pWorld->aSectors[i].numVertices) + sizeof(SithSector);
    }

    (*aCount)[6] = pWorld->numSounds;
    (*aMemUsed)[6] = Sound_GetMemoryUsage();

    (*aCount)[8] = pWorld->numCogScripts;
    for ( i = 0; i < pWorld->numCogScripts; ++i )
    {
        (*aMemUsed)[8] += (sizeof(*pWorld->aCogScripts[i].pCode) * pWorld->aCogScripts[i].codeSize) + sizeof(SithCogScript);
        (*aMemUsed)[8] += (sizeof(SithCogSymbol) * pWorld->aCogScripts[i].pSymbolTable->numUsedSymbols) + sizeof(SithCogSymbolTable);
    }

    (*aCount)[7] = pWorld->numCogs;
    for ( i = 0; i < pWorld->numCogs; ++i )
    {
        (*aMemUsed)[7] += (sizeof(SithCogSymbol) * pWorld->aCogs[i].pSymbolTable->numUsedSymbols) + sizeof(SithCog) + sizeof(SithCogSymbolTable);
    }

    (*aCount)[10] = pWorld->numModels;
    for ( i = 0; i < pWorld->numModels; ++i )
    {
        (*aMemUsed)[10] += sithModel_GetModelMemUsage(&pWorld->aModels[i]);
    }

    (*aCount)[11] = pWorld->numKeyframes;
    for ( i = 0; i < pWorld->numKeyframes; ++i )
    {
        (*aMemUsed)[11] += sizeof(rdKeyframe);
        (*aMemUsed)[11] += sizeof(rdKeyframeNode) * pWorld->aKeyframes[i].numJoints;
        for ( j = 0; j < pWorld->aKeyframes[i].numJoints; ++j )
        {
            (*aMemUsed)[11] += sizeof(rdKeyframeNodeEntry) * pWorld->aKeyframes[i].aNodes[j].numEntries;
        }
    }

    (*aCount)[12] = pWorld->numPuppetClasses;
    (*aMemUsed)[12] = sizeof(SithPuppetClass) * pWorld->numPuppetClasses;

    (*aCount)[13] = pWorld->numSprites;
    (*aMemUsed)[13] = sizeof(rdSprite3) * pWorld->numSprites;
    for ( i = 0; i < pWorld->numSprites; ++i )
    {
        (*aMemUsed)[13] +=
            (sizeof(rdVector3) + sizeof(rdVector2)) * pWorld->aSprites[i].face.numVertices;
    }

    (*aCount)[14] = pWorld->numThingTemplates;
    (*aMemUsed)[14] = sizeof(SithThing) * pWorld->numThingTemplates;

    (*aCount)[15] = pWorld->numThings;
    (*aMemUsed)[15] = sizeof(SithThing) * pWorld->numThings;
}

int J3DAPI sithWorld_RegisterTextSectionParser(const char* aSectionName, SithWorldTextSectionParseFunc pfParseFunction)
{
    int idx = sithWorld_GetTextSectionParserIndex(aSectionName);
    if ( idx == -1 )
    {
        if ( numTextSectionParseHandlers >= SITHWORLD_MAXTEXTSECTIONPARSERS )
        {
            return 1;
        }

        idx = numTextSectionParseHandlers++;
    }

    strncpy(aTextSectioParseHandlers[idx].aName, aSectionName, STD_ARRAYLEN(aTextSectioParseHandlers[idx].aName) - 1);
    aTextSectioParseHandlers[idx].aName[STD_ARRAYLEN(aTextSectioParseHandlers[idx].aName) - 1] = 0;
    aTextSectioParseHandlers[idx].pfHandler = pfParseFunction;
    return 0;
}

/* Linker-discarded (no retail symbol); body from the Indy3D debug build (0x43242d, the function after
   RegisterTextSectionParser: there is no RemoveSectionParser in this file's debug code). */
void J3DAPI sithWorld_CalcSurfaceNormals(SithWorld* pWorld)
{
    rdFace* pFace;
    size_t i;
    SithSurface* pSurf;
    rdVector3* aVertices;
    int j;
    int prevVert;
    size_t nextVert;

    pSurf     = pWorld->aSurfaces;
    aVertices = pWorld->aVertices;
    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pFace = &pSurf->face;
#line 1146
        SITH_ASSERTREL(pFace->numVertices > 2);

        for ( j = 0; j < (int)pFace->numVertices; ++j )
        {
            prevVert = j - 1;
            if ( prevVert < 0 )
            {
                prevVert = pFace->numVertices - 1;
            }

            nextVert = (j + 1) % pFace->numVertices;
            if ( !rdMath_PointsCollinear(&aVertices[pFace->aVertices[j]], &aVertices[pFace->aVertices[nextVert]], &aVertices[pFace->aVertices[prevVert]]) )
            {
                break;
            }
        }

        if ( j >= (int)pFace->numVertices )
        {
#line 1162
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid Face encountered while calculating normals (surface #%d).\n", i, 0, 0, 0);
        }
        else
        {
            rdMath_CalcSurfaceNormal(&pFace->normal, &aVertices[pFace->aVertices[j]], &aVertices[pFace->aVertices[nextVert]], &aVertices[pFace->aVertices[prevVert]]);
        }

        ++pSurf;
    }
}

void J3DAPI sithWorld_ResetRenderState(SithWorld* pWorld)
{
    SithSector* pSec;
    size_t index;

#line 1198
    SITH_ASSERTREL((pWorld != NULL));
    SITH_ASSERTREL((pWorld->aSectors != NULL));

    memset(pWorld->aVertexRenderTickIds, 0, sizeof(*pWorld->aVertexRenderTickIds) * pWorld->numVertices);
    memset(pWorld->aVertDynamicLightsRenderTicks, 0, sizeof(*pWorld->aVertDynamicLightsRenderTicks) * pWorld->numVertices);

    pSec = pWorld->aSectors;
    for ( index = 0; index < pWorld->numSectors; ++index )
    {
        SithSurface* pCurSurf = pSec->pFirstSurface;
        size_t j;
        for ( j = 0; j < pSec->numSurfaces; ++j )
        {
            pCurSurf->renderTick = 0;
            ++pCurSurf;
        }

        pSec->renderTick = 0;
        ++pSec;
    }
}

/* Linker-discarded (no retail symbol); body from the Indy3D debug build (0x4326e1). */
void J3DAPI sithWorld_ResetGeoresource(SithWorld* pWorld)
{
    size_t i;

#line 1245
    SITH_ASSERTREL(pWorld);

    for ( i = 0; i < pWorld->numSurfaces; ++i )
    {
        pWorld->aSurfaces[i].face.matCelNum = 0;
    }

    for ( i = 0; i < pWorld->numSectors; ++i )
    {
        memset(&pWorld->aSectors[i].thrust, 0, sizeof(rdVector3));
        rdVector_Set3(&pWorld->aSectors[i].tint, 0.0f, 0.0f, 0.0f);
    }
}

int J3DAPI sithWorld_ValidateWorld(const SithWorld* pWorld)
{
    if ( !pWorld->aThings && pWorld->numThings )
    {
#line 1284
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Problem with things array, should not be NULL.\n", 0, 0, 0, 0);
    }
    else if ( !pWorld->aSprites && pWorld->numSprites )
    {
#line 1290
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Problem with spriates array, should not be NULL.\n", 0, 0, 0, 0);
    }
    else if ( !pWorld->aModels && pWorld->numModels )
    {
#line 1296
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Problem with models array, should not be NULL.\n", 0, 0, 0, 0);
    }
    else if ( !pWorld->aSectors || !pWorld->aSurfaces || !pWorld->aVertices )
    {
#line 1302
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "A required geometry section is missing from the level file.\n", 0, 0, 0, 0);
    }
    else if ( !sithSurface_ValidateWorldSurfaces(pWorld) )
    {
#line 1310
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Surface resources did not pass validation.\n", 0, 0, 0, 0);
    }
    else
    {
        // Success
        return 0;
    }

    return 1;
}

/* Linker-discarded: retail __LINE__ gap 1310..1404 (sithWorld_ValidateWorld -> sithWorld_ReadTextSection). Body from the Indy
   debug build (0x0043293F; caller the discarded sithMulti 0x004694D0). Its status print's literal is a /Gf COMDAT that
   went with the function, so retail's missing copy says nothing about the print. */
uint32_t J3DAPI sithWorld_CalcWorldChecksum(SithWorld* pWorld, uint32_t seed)
{
    uint32_t checksum;
    size_t i;

    checksum = seed;
    for ( i = 0; i < pWorld->numCogScripts; ++i )
    {
        checksum = stdUtil_CalcChecksum((const uint8_t*)pWorld->aCogScripts[i].pCode, pWorld->aCogScripts[i].codeSize, checksum);
    }

    checksum = stdUtil_CalcChecksum((const uint8_t*)pWorld->aVertices, pWorld->numVertices * sizeof(rdVector3), checksum);

    for ( i = 0; i < pWorld->numThingTemplates; ++i )
    {
        checksum = sithThing_CalcThingChecksum(&pWorld->aThingTemplates[i], checksum);
    }

    if ( sithWorld_pStaticWorld )
    {
        for ( i = 0; i < sithWorld_pStaticWorld->numCogScripts; ++i )
        {
            checksum = stdUtil_CalcChecksum((const uint8_t*)sithWorld_pStaticWorld->aCogScripts[i].pCode, sithWorld_pStaticWorld->aCogScripts[i].codeSize, checksum);
        }
    }

#line 1363
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Checksum computed:  Seed=%x, Result=%x.\n", seed, checksum, 0, 0);
    return checksum;
}

int J3DAPI sithWorld_InitPlayers(SithWorld* pWorld)
{
    size_t i;

    for ( i = 1; i < sithPlayer_numPlayers; ++i )
    {
        sithPlayer_HidePlayer(i);
    }

    sithPlayer_SetLocalPlayer(0);
    return 1;
}

int J3DAPI sithWorld_ReadTextSection(SithWorld* pWorld, const char* aSectionName, int bSkip)
{
    uint32_t endTime;
    int handlerIdx;
    uint32_t startTime;

#line 1404
    SITH_ASSERTREL(pWorld);

    handlerIdx = sithWorld_GetTextSectionParserIndex(aSectionName);
    if ( handlerIdx == -1 )
    {
#line 1412
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Unknown Section '%s'.\n", aSectionName, 0, 0, 0);
        return 0;
    }

    startTime = stdPlatform_GetTimeMsec();
    if ( aTextSectioParseHandlers[handlerIdx].pfHandler(pWorld, bSkip) )
    {
#line 1422
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error while parsing section %s.\n", aSectionName, 0, 0, 0);
        return 1;
    }

    // Success
    endTime = stdPlatform_GetTimeMsec();
    return 0;
}

/* PLACEHOLDER, linker-discarded: retail __LINE__ gap 1422..1565 (sithWorld_ReadTextSection -> sithWorld_ReadGeoresourceText), where the original
   defined it. Only existence and position are evidenced: minimal body. */
const char* J3DAPI sithWorld_GetCurReadSection(void)
{
    return aCurSection;
}

int J3DAPI sithWorld_GetTextSectionParserIndex(const char* aSectionName)
{
    int i;

    for ( i = 0; i < numTextSectionParseHandlers; ++i )
    {
        if ( strneqi(aTextSectioParseHandlers[i].aName, aSectionName, STD_ARRAYLEN(aTextSectioParseHandlers[i].aName)) )
        {
            return i;
        }
    }

    return -1;
}

/* Linker-discarded (retail __LINE__ gap 1422..1565, sithWorld_ReadTextSection -> sithWorld_ReadGeoresourceText). Body from the
   debug build (0x432C1F, /Od EXACT): every failed write, from inside the two loops too, jumps to one `return 1`
   after `return 0` (debug jmp 0x432E35 x15). */
int J3DAPI sithWorld_WriteGeoresourceText(const SithWorld* pWorld)
{
    size_t i;

    if ( stdConffile_WriteLine("#### Geomtry Resources Info ####\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("SECTION: GEORESOURCE\n\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#----- Vertices Subsection -----\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World vertices %d\n", pWorld->numVertices) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#num:\tvertex:\n") )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numVertices; ++i )
    {
        if ( stdConffile_Printf("%d:\t%f\t%f\t%f\n", i, pWorld->aVertices[i].x, pWorld->aVertices[i].y, pWorld->aVertices[i].z) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("\n\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#-- Texture Verts Subsection ---\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World texture vertices %d\n", pWorld->numTexVertices) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#num:\tu:\tv:\n") )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numTexVertices; ++i )
    {
        if ( stdConffile_Printf("%d:\t%f\t%f\n", i, pWorld->aTexVerticies[i].x, pWorld->aTexVerticies[i].y) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("\n\n") )
    {
        goto error;
    }

    if ( sithSurface_WriteAdjoinsListText(pWorld) )
    {
        goto error;
    }

    if ( sithSurface_WriteSurfacesListText(pWorld) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWorld_ReadGeoresourceText(SithWorld* pWorld, int bSkip)
{
    rdVector2* pTexVert = NULL;
    rdVector3* pVert = NULL;
    const char* pName = pWorld->aName;
    size_t numVerts;
    size_t numTexVerts;
    size_t i;
    int entryNum;
    float x;
    float y;
    float z;
    float u;
    float v;

    if ( bSkip )
    {
        // TODO: should move cur file offset to the end of this section?
        return 1;
    }

    sithWorld_UpdateLoadProgress(50.0f);

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " world vertices %d", &numVerts) != 1 )
    {
        goto syntax_error;
    }

#line 1565
    pWorld->aVertices = (rdVector3*)STDMALLOC(sizeof(rdVector3) * numVerts);
    if ( !pWorld->aVertices )
    {
        goto alloc_error;
    }

    for ( i = 0, pVert = pWorld->aVertices; i < numVerts; ++i, ++pVert )
    {
        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, " %d: %f %f %f", &entryNum, &x, &y, &z) != 4 )
        {
            goto syntax_error;
        }

        pVert->x = x;
        pVert->y = y;
        pVert->z = z;
    }

    pWorld->numVertices = numVerts;

    if ( !stdConffile_ReadLine() )
    {
        goto eof_error;
    }

    if ( sscanf(stdConffile_aLine, " world texture vertices %d", &numTexVerts) != 1 )
    {
        goto syntax_error;
    }

#line 1592
    pWorld->aTexVerticies = (rdVector2*)STDMALLOC(sizeof(rdVector2) * numTexVerts);
    if ( !pWorld->aTexVerticies )
    {
        goto alloc_error;
    }

    pTexVert = pWorld->aTexVerticies;
    for ( i = 0; i < numTexVerts; ++i )
    {
        if ( !stdConffile_ReadLine() )
        {
            goto eof_error;
        }

        if ( sscanf(stdConffile_aLine, " %d: %f %f", &entryNum, &u, &v) != 3 )
        {
            goto syntax_error;
        }

        pTexVert->x = u;
        pTexVert->y = v;
        ++pTexVert;
    }

    pWorld->numTexVertices = numTexVerts;

    // Read goeres succeed, now read surfaces
    return sithSurface_ReadSurfacesListText(pWorld, bSkip);

eof_error:
#line 1615
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", pName, 0, 0, 0);
    goto error;

syntax_error:
#line 1620
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, pName, 0, 0);
    goto error;

alloc_error:
#line 1625
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, pName, 0, 0);

error:
    return 1;
}

/* Linker-discarded (no retail symbol); body from the Indy3D debug build (0x4330fd, between ReadGeoresourceText and
   WriteGeoresourceBinary; called with no argument by the debug world writer at 0x4e9b01). Name inferred from
   ReadCopyrightText. The failed write inside the line loop jumps to the shared `return 1` after
   `return 0`; the other sites use the same label. */
int J3DAPI sithWorld_WriteCopyrightText(void)
{
    char aLine[SITHWORLD_COPYRIGHTLINELEN + 1];
    int i;

    if ( stdConffile_WriteLine("#### Copyright information #####\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("SECTION: COPYRIGHT\n") )
    {
        goto error;
    }

    aLine[SITHWORLD_COPYRIGHTLINELEN] = 0;
    for ( i = 0; i < SITHWORLD_NUMCOPYRIGHTLINES; ++i )
    {
        memcpy(aLine, &aCopyrightSymbol[SITHWORLD_COPYRIGHTLINELEN * i], SITHWORLD_COPYRIGHTLINELEN);
        if ( stdConffile_Printf("%s\n", aLine) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithWorld_WriteGeoresourceBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t size;
    size_t nWritten;

    size = sizeof(rdVector3) * pWorld->numVertices;
    nWritten = sith_pHS->pFileWrite(fh, pWorld->aVertices, size);
    if ( nWritten != size )
    {
        return 1;
    }

    size = sizeof(rdVector2) * pWorld->numTexVertices;
    nWritten = sith_pHS->pFileWrite(fh, pWorld->aTexVerticies, size);
    if ( nWritten != size )
    {
        return 1;
    }

    return 0;
}

int J3DAPI sithWorld_ReadGeoresourceBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t size;
    size_t nRead;

    size = sizeof(rdVector3) * pWorld->numVertices;
#line 1679
    pWorld->aVertices = (rdVector3*)STDMALLOC(size);
    if ( !pWorld->aVertices )
    {
        return 1;
    }

    nRead = sith_pHS->pFileRead(fh, pWorld->aVertices, size);
    if ( nRead != size )
    {
        return 1;
    }

    size = sizeof(rdVector2) * pWorld->numTexVertices;
#line 1687
    pWorld->aTexVerticies = (rdVector2*)STDMALLOC(size);
    if ( !pWorld->aTexVerticies )
    {
        return 1;
    }

    nRead = sith_pHS->pFileRead(fh, pWorld->aTexVerticies, size);
    if ( nRead != size )
    {
        return 1;
    }

    return 0;
}

/* Linker-discarded (no retail symbol); body from the Indy3D debug build (0x433313). A failed section writer inside the
   section loop jumps to the shared close-and-`return 1` block after `return 0` (as LoadEntryBinary);
   the other failure sites use the same label. The size word is written before it is measured (the debug code writes
   the uninitialised local, then seeks back and rewrites it). */
int J3DAPI sithWorld_WriteEntryBinary(SithWorld* pWorld, const char* pFilename)
{
    CndWorld worldHeader;
    tFileHandle fh;
    size_t i;
    uint32_t fileSize;
    size_t written;
    int result;

    written = 0;

    memcpy(worldHeader.aCopyright, aCopyrightSymbol, sizeof(worldHeader.aCopyright));
    strncpy(worldHeader.aName, pFilename, STD_ARRAYLEN(worldHeader.aName) - 1);
    worldHeader.aName[STD_ARRAYLEN(worldHeader.aName) - 1] = 0;
    stdUtil_ToLower(worldHeader.aName);

    worldHeader.state = pWorld->state;
    worldHeader.state &= ~SITH_WORLD_STATE_INITIALIZED;
    worldHeader.version = SITHWORLD_CNDVERSION;
    worldHeader.gravity = pWorld->gravity;
    worldHeader.fog = pWorld->fog;
    worldHeader.ceilingSkyHeight = pWorld->ceilingSkyHeight;
    worldHeader.horizonDistance  = pWorld->horizonDistance;
    rdVector_Copy2(&worldHeader.horizonSkyOffset, &pWorld->horizonSkyOffset);
    rdVector_Copy2(&worldHeader.ceilingSkyOffset, &pWorld->ceilingSkyOffset);
    for ( i = 0; i < STD_ARRAYLEN(worldHeader.distancesLOD); ++i )
    {
        worldHeader.distancesLOD[i] = pWorld->distancesLOD[i];
    }

    worldHeader.numSounds                     = pWorld->numSounds;
    worldHeader.numMaterials                  = pWorld->numMaterials;
    worldHeader.sizeMaterials                 = pWorld->sizeMaterials;
    worldHeader.aMaterials                    = 0;
    worldHeader.apMatArray                    = 0;
    worldHeader.numVertices                   = pWorld->numVertices;
    worldHeader.aVertices                     = 0;
    worldHeader.aTransformedVertices          = 0;
    worldHeader.aVertexRenderTickIds          = 0;
    worldHeader.numTexVertices                = pWorld->numTexVertices;
    worldHeader.aTexVerticies                 = 0;
    worldHeader.aVertDynamicLights            = 0;
    worldHeader.aVertDynamicLightsRenderTicks = 0;
    worldHeader.numAdjoins                    = pWorld->numAdjoins;
    worldHeader.aAdjoins                      = 0;
    worldHeader.numSurfaces                   = pWorld->numSurfaces;
    worldHeader.aSurfaces                     = 0;
    worldHeader.numSectors                    = pWorld->numSectors;
    worldHeader.aSectors                      = 0;
    worldHeader.numAIClasses                  = pWorld->numAIClasses;
    worldHeader.sizeAIClasses                 = pWorld->sizeAIClasses;
    worldHeader.aAIClasses                    = 0;
    worldHeader.numModels                     = pWorld->numModels;
    worldHeader.sizeModels                    = pWorld->sizeModels;
    worldHeader.aModels                       = 0;
    worldHeader.numSprites                    = pWorld->numSprites;
    worldHeader.sizeSprites                   = pWorld->sizeSprites;
    worldHeader.aSprites                      = 0;
    worldHeader.numKeyframes                  = pWorld->numKeyframes;
    worldHeader.sizeKeyframes                 = pWorld->sizeKeyframes;
    worldHeader.aKeyframes                    = 0;
    worldHeader.numPuppetClasses              = pWorld->numPuppetClasses;
    worldHeader.sizePuppetClasses             = pWorld->sizePuppetClasses;
    worldHeader.aPuppetClasses                = 0;
    worldHeader.numSoundClasses               = pWorld->numSoundClasses;
    worldHeader.sizeSoundClasses              = pWorld->sizeSoundClasses;
    worldHeader.aSoundClasses                 = 0;
    worldHeader.numCogScripts                 = pWorld->numCogScripts;
    worldHeader.sizeCogScripts                = pWorld->sizeCogScripts;
    worldHeader.aCogScripts                   = 0;
    worldHeader.numCogs                       = pWorld->numCogs;
    worldHeader.sizeCogs                      = pWorld->sizeCogs;
    worldHeader.aCogs                         = 0;
    worldHeader.numThingTemplates             = pWorld->numThingTemplates;
    worldHeader.sizeThingTemplates            = pWorld->sizeThingTemplates;
    worldHeader.aThingTemplates               = 0;
    worldHeader.numThings                     = pWorld->numThings;
    worldHeader.lastThingIdx                  = 0;
    worldHeader.aThings                       = 0;
    worldHeader.sizePVS                       = pWorld->sizePVS;
    worldHeader.aPVS                          = 0;

    fh = sith_pHS->pFileOpen(pFilename, "wb");
    if ( !fh )
    {
        goto error;
    }

    written = sith_pHS->pFileWrite(fh, &fileSize, sizeof(fileSize));
    if ( written != sizeof(fileSize) )
    {
        goto error;
    }

    written = sith_pHS->pFileWrite(fh, &worldHeader, sizeof(CndWorld));
    if ( written != sizeof(CndWorld) )
    {
        goto error;
    }

    for ( i = 0; i < STD_ARRAYLEN(aBinarySectionParsers); ++i )
    {
        result = aBinarySectionParsers[i].pfWrite(fh, pWorld);
        if ( result )
        {
            goto error;
        }
    }

    fileSize = sith_pHS->pFileTell(fh);
    result   = sith_pHS->pFileSeek(fh, 0, 0);
    if ( result )
    {
        goto error;
    }

    written = sith_pHS->pFileWrite(fh, &fileSize, sizeof(fileSize));
    if ( written != sizeof(fileSize) )
    {
        goto error;
    }

    if ( fh )
    {
        sith_pHS->pFileClose(fh);
    }

    return 0;

error:
    if ( fh )
    {
        sith_pHS->pFileClose(fh);
    }

    return 1;
}

int J3DAPI sithWorld_LoadEntryBinary(SithWorld* pWorld, const char* pFilePath)
{
    uint32_t startTime;
    tFileHandle fh;
    uint32_t fileSize;
    size_t numRead;
    size_t i;
    float endPos;
    uint32_t currentsectionReadFirstTime;
    int result;
    uint32_t sectionReadLastTime;
    uint32_t endTime;

    // Clear input world
    memset(pWorld, 0, sizeof(SithWorld));

    startTime = stdPlatform_GetTimeMsec();

    // open cnd file
    fh = sith_pHS->pFileOpen(pFilePath, "rb");
    if ( !fh )
    {
        goto error;
    }

    sithWorld_UpdateLoadProgress(0.0f);
    numRead = sith_pHS->pFileRead(fh, &fileSize, sizeof(fileSize));
    if ( numRead != sizeof(fileSize) )
    {
        goto error;
    }

    endPos = (float)fileSize / 100.0f;
    sithWorld_UpdateLoadProgress((float)sith_pHS->pFileTell(fh) / endPos);

    // Read world header
    numRead = sith_pHS->pFileRead(fh, pWorld, sizeof(CndWorld));
    if ( numRead != sizeof(CndWorld) )
    {
        goto error;
    }

    sithWorld_UpdateLoadProgress((float)sith_pHS->pFileTell(fh) / endPos);
    strncpy(pWorld->aName, stdFnames_FindMedName(pFilePath), STD_ARRAYLEN(pWorld->aName) - 1);
    pWorld->aName[STD_ARRAYLEN(pWorld->aName) - 1] = 0;
    stdUtil_ToLower(pWorld->aName);

    if ( memcmp(pWorld->aCopyright, aCopyrightSymbol, STD_ARRAYLEN(aCopyrightSymbol)) )
    {
        goto error;
    }

    if ( pWorld->version != SITHWORLD_CNDVERSION )
    {
#line 1866
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Invalid version %.2f for file '%s'.\n", pWorld->version, pFilePath, 0, 0);
        goto error;
    }

    pWorld->state &= ~SITH_WORLD_STATE_INITIALIZED;
    for ( i = 0; i < STD_ARRAYLEN(aBinarySectionParsers); ++i )
    {
        currentsectionReadFirstTime = stdPlatform_GetTimeMsec();
        sithWorld_UpdateLoadProgress((float)sith_pHS->pFileTell(fh) / endPos);
        strcpy(aCurSection, aBinarySectionParsers[i].pName);
        Sleep(0);

        result = aBinarySectionParsers[i].pfRead(fh, pWorld);
        if ( result )
        {
            goto error;
        }

        sectionReadLastTime = stdPlatform_GetTimeMsec();
    }

    if ( fh )
    {
        sith_pHS->pFileClose(fh);
    }

    // Success
    endTime = stdPlatform_GetTimeMsec();
#line 1896
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "%f seconds to load %s.\n", (endTime - startTime) / 1000.0, pWorld->aName, 0, 0);

    sithWorld_UpdateLoadProgress(100.0f);
    return 0;

error:
    if ( fh )
    {
        sith_pHS->pFileClose(fh);
    }

    return 1;
}
