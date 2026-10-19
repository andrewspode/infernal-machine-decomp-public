#define RDVECTOR_NO_INLINE_SET4 // Retail calls rdVector_Set4 out of line at 0x00434E3C and 0x00434ECE in sithThing_Initialize, its only uses in this TU
#define RDVECTOR_NO_INLINE_LEN3 // Retail calls rdVector_Len3 out of line at 0x00435541 in sithThing_ExitWater, its only user in this TU
#define RDMATRIX_NO_INLINE // Retail calls rdMatrix_Copy34 out of line at 0x0043AA55 in sithThing_UpdateQuetzUserBlock

#include "sithThing.h"

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithThing_countfit_1, 15)
CF_PROTO(sithThing_countfit_2, 15)
CF_PROTO(sithThing_countfit_3, 15)
CF_PROTO(sithThing_countfit_4, 13)


/* end count-fit */

#include <rdroid/Engine/rdPuppet.h>
#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdModel3.h>

#include <sith/AI/sithAI.h>
#include <sith/AI/sithAIClass.h>
#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#ifdef J3D_DEBUG
#include <sith/Devices/sithConsole.h>
#endif
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSThing.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPathMove.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithExplosion.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithSprite.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>
#include <sound/Sound.h>

#include <std/General/stdHashtbl.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>


#define SITHTHING_MAXTHINGSTOPURGE 10

#define SITHTHING_ABS(x) ((x) < 0.0f ? -(x) : (x))

// Retail stringifies the free-index asserts as "(numThings <= 2304)", "(thingNum < 2304)"
// and "sithThing_numFreeThings < 2304", so the bound reached #condition already expanded.
#define SITHTHING_MAXTHINGS 2304

#define SITHTHING_PARSEARG_PARSED      0
#define SITHTHING_PARSEARG_NOTPARSED   1
#define SITHTHING_PARSEARG_BADSYNTAX   19

char* sithThing_aStrThingTypes[SITH_THING_NUMTYPES] =
{
    "free",
    "camera",
    "actor",
    "weapon",
    "debris",
    "item",
    "explosion",
    "cog",
    "ghost",
    "corpse",
    "player",
    "particle",
    "hint",
    "sprite",
    "polyline"
};

char* sithThing_aStrThingArgs[77] =
{
    "--invalid--",
    "type",
    "collide",
    "move",
    "size",
    "thingflags",
    "timer",
    "light",
    "attach",
    "soundclass",
    "model3d",
    "sprite",
    "surfdrag",
    "airdrag",
    "staticdrag",
    "mass",
    "height",
    "physflags",
    "maxrotvel",
    "maxvel",
    "vel",
    "angvel",
    "typeflags",
    "health",
    "maxthrust",
    "maxrotthrust",
    "maxheadvel",
    "maxheadyaw",
    "jumpspeed",
    "weapon",
    "damage",
    "mindamage",
    "damageclass",
    "explode",
    "frame",
    "numframes",
    "puppet",
    "blasttime",
    "babytime",
    "force",
    "maxlight",
    "range",
    "flashrgb",
    "expandtime",
    "fadetime",
    "aiclass",
    "cog",
    "respawn",
    "material",
    "rate",
    "count",
    "elementsize",
    "particle",
    "maxhealth",
    "movesize",
    "orientspeed",
    "buoyancy",
    "eyeoffset",
    "minheadpitch",
    "maxheadpitch",
    "fireoffset",
    "lightoffset",
    "lightintensity",
    "debris",
    "creatething",
    "minsize",
    "pitchrange",
    "yawrange",
    "orient",
    "userval",
    "spritething",
    "spritestart",
    "spriteend",
    "collheight",
    "collwidth",
    "voicecolor",
    "perflevel"
};

uint32_t sithThing_curSignature = 1u;
uint16_t sithThing_guidEntropy  = 1u;

static float sithThing_jeepPosOffset = 0.14f;
static float sithThing_raftPosOffset = 0.08f;
static float sithThing_tailPosOffset = 0.18f;

static float sithThing_aWeaponCoefficients[25] =
{
    0.0f,
    0.0f,
    0.0f,
    0.025f, // Pistol
    0.05f,
    0.06f,
    0.06f,
    0.0f,
    0.01f,
    0.0f, // Shotgun
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.05f,
    0.08f,
    0.08f,
    0.03f,
    0.0f
};

static float sithThing_jeepHitMinVelocityZ  = -1.65f;
static float sithThing_quetzStretchMinRatio = 0.3f;
static float sithThing_quetzScaleBase       = 1.0f;
static float sithThing_quetzScalePerAttach  = 0.07f;
static float sithThing_quetzMaxTurnRate     = 25.0f;

// The TU's whole private .bss block, 0x0055EE18-0x005612BC, in retail's address
// order. Every one of these is a zero-initialized definition, so VC5 emits them
// in declaration order instead of splitting the
// block into an identifier-keyed uninitialized group and a declaration-ordered
// one; the 4 bytes at 0x00561264 are rule-3 alignment before the 8-aligned
// 64-byte sithThing_aSyncFlags.
SithThing* sithThing_apUnsyncedThings[16] = { 0 };
tHashTable* sithThing_pParseHashtbl = NULL; // Added: Init to null
SithQuetzAttachInfo* sithThing_prevQuetzAttachInfo = NULL;
size_t sithThing_aFreeThingIdxs[SITHTHING_MAXTHINGS] = { 0 };
SithQuetzAttachInfo* sithThing_curQuetzAttachInfo  = NULL;
SithThingSyncFlags sithThing_aSyncFlags[STD_ARRAYLEN(sithThing_apUnsyncedThings)] = { 0 };
size_t sithThing_numFreeThings = 0;
size_t sithThing_numUnsyncedThings = 0;
static int bThingStartup = 0; // Name from the retail assertion text at 0x00529688
SithThingUnknownFunc sithThing_pfUnknownFunc = NULL; // Fixed: Init to NULL
int sithThing_dword_5612B8 = 0;




void J3DAPI sithThing_InitializeWorldThings(SithWorld* pWorld);
void J3DAPI sithThing_UpdateMove(SithThing* pThing, float secDeltaTime, uint32_t msecDeltaTime);
void J3DAPI sithThing_DestroyDyingThing(SithThing* pThing);
void J3DAPI sithThing_PlayCogDamageSound(SithThing* pThing, SithDamageType hitType);

void J3DAPI sithThing_EnterWater(SithThing* pThing, int bNoSplash);
void J3DAPI sithThing_ExitWater(SithThing* pThing, int bNoSplash);
void J3DAPI sithThing_PurgeGarbageThings(SithWorld* pWorld);

int J3DAPI sithThing_ParseThingArg(const StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing, int adjNum);
int J3DAPI sithThing_ParseType(const char* pType);
int J3DAPI sithThing_ParseThingPlacement(SithWorld* pWorld);

void J3DAPI sithThing_FreeThing(SithWorld* pWorld, SithThing* pThing);
void J3DAPI sithThing_FreeThingIndex(SithWorld* pWorld, size_t thingNum);
int J3DAPI sithThing_GetFreeThingIndex(void);

int J3DAPI sithThing_Startup(void)
{
    int adjNum;

    if ( bThingStartup )
    {
#line 273
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Multiple startup on sithThing_startup().\n", 0, 0, 0, 0);
        return 0;
    }

    sithThing_pParseHashtbl = stdHashtbl_New(154u);
    if ( !sithThing_pParseHashtbl )
    {
#line 283
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not allocate thing parsing hashtable.\n", 0, 0, 0, 0);
        return 0;
    }

    for ( adjNum = 1; adjNum < (int)STD_ARRAYLEN(sithThing_aStrThingArgs); ++adjNum )
    {
        stdHashtbl_Add(sithThing_pParseHashtbl, sithThing_aStrThingArgs[adjNum], (void*)adjNum);
    }

    sithThing_curSignature  = 1;
    bThingStartup = 1;
    return 1;
}

int J3DAPI sithThing_Shutdown(void)
{
    if ( !bThingStartup )
    {
#line 318
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing system not started.\n", 0, 0, 0, 0);
        return 0;
    }

    stdHashtbl_Free(sithThing_pParseHashtbl);
#line 324
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Thing signatures used=%d.\n", sithThing_curSignature - 1, 0, 0, 0);

    bThingStartup = 0;
    return 1;
}

/* PLACEHOLDER, linker-discarded: retail __LINE__ gap 324..376 (Shutdown -> Draw, +22 lines over the TU's per-boundary
   overhead) where the original defined it, and Jedi Knight's retail has it at the same spot
   (sithThing_RegisterUnknownFunc, 0x004CC1D0, right after Shutdown). Retail reads and calls sithThing_pfUnknownFunc
   (0x00433A31) but no retail code writes it, so its only setter was discarded. Name/signature are JK's;
   body from the debug build (0x0043FFF1). */
void J3DAPI sithThing_RegisterUnknownFunc(SithThingUnknownFunc pFunc)
{
    if ( pFunc )
    {
        sithThing_pfUnknownFunc = pFunc;
    }
}

int J3DAPI sithThing_Draw(SithThing* pThing)
{
    int bCurSwapped;
    SithThingSwapEntry* pEntry = NULL;
    rdVector4 curcolor;
    int currentdrawRes;

    if ( pThing->type == SITH_THING_POLYLINE )
    {
        if ( !pThing->renderData.data.pPolyline )
        {
#line 376
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "WARNING: Polyline thing \"%s\" has no polyline data!\n", pThing->aName, 0, 0, 0);
        }
        else
        {
            pThing->renderData.data.pPolyline->face.extraLight.alpha = pThing->alpha;
        }
    }

    if ( pThing->renderData.type == RD_THING_MODEL3 )
    {
        curcolor.x = 0.0f;
        curcolor.y = 0.0f;
        curcolor.z = 0.0f;
        curcolor.alpha = pThing->alpha;

        rdModel3_SetThingColor(&pThing->renderData, &curcolor);
    }

    if ( !pThing->pSwapList )
    {
        switch ( pThing->type )
        {
            case SITH_THING_SPRITE:
                currentdrawRes = sithSprite_Draw(pThing);
                break;

            case SITH_THING_POLYLINE:
                sithFX_UpdatePolyline(pThing);
                currentdrawRes = rdThing_Draw(&pThing->renderData, &pThing->orient);
                sithFX_ResetPolylineTexVertOffset(pThing);
                break;

            case SITH_THING_CAMERA:
            case SITH_THING_GHOST:
            case SITH_THING_HINT:
                currentdrawRes = 0;
                break;

            default:
                currentdrawRes = rdThing_Draw(&pThing->renderData, &pThing->orient);
                break;
        }
    }
    else
    {
        rdModel3_GetThingColor(&pThing->renderData, &curcolor);

        pEntry = pThing->pSwapList;
        while ( pEntry )
        {
            rdModel3_SetModelColor(pEntry->pSrcModel, &curcolor);
            bCurSwapped = rdModel3_SwapMesh(pThing->renderData.data.pModel3, pEntry->meshNum, pEntry->pSrcModel, pEntry->srcMeshNum);
            pEntry = pEntry->pNextEntry;
        }

        currentdrawRes = rdThing_Draw(&pThing->renderData, &pThing->orient);
        curcolor.x = 0.0f;
        curcolor.y = 0.0f;
        curcolor.z = 0.0f;
        curcolor.alpha = 1.0f;

        pEntry = pThing->pSwapList;
        while ( pEntry )
        {
            bCurSwapped = rdModel3_SwapMesh(pThing->renderData.data.pModel3, pEntry->meshNum, pEntry->pSrcModel, pEntry->srcMeshNum);
            rdModel3_SetModelColor(pEntry->pSrcModel, &curcolor);
            pEntry = pEntry->pNextEntry;
        }
    }

    return currentdrawRes;
}

int J3DAPI sithThing_AddSwapEntry(SithThing* pThing, int meshNum, rdModel3* pSrcModel, int meshNumSrc)
{
    SithThingSwapEntry* pNewEntry;
    SithThingSwapEntry* pCurEnt;

#line 441
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pSrcModel);

    if ( meshNum < 0 || meshNumSrc < 0 )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "-1 mesh num passed to sithThing_AddSwapEntry!\n", 0, 0, 0, 0);
        return -1;
    }

    if ( pThing->pSwapList )
    {
        pCurEnt = pThing->pSwapList;
        while ( pCurEnt )
        {
            if ( pCurEnt->meshNum == meshNum )
            {
                pCurEnt->srcMeshNum = meshNumSrc;
                pCurEnt->pSrcModel = pSrcModel;
                return pCurEnt->entryNum;
            }

            pCurEnt = pCurEnt->pNextEntry;
        }
    }

    pNewEntry = (SithThingSwapEntry*)STDMALLOC(sizeof(SithThingSwapEntry));
    if ( !pNewEntry )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't allocate a swap list entry!\n", 0, 0, 0, 0);
        return -1;
    }

    pNewEntry->meshNum    = meshNum;
    pNewEntry->srcMeshNum = meshNumSrc;
    pNewEntry->pSrcModel  = pSrcModel;
    pNewEntry->pNextEntry = NULL;
    pNewEntry->entryNum   = pThing->numSwapEntries++; // This looks like to be an entry signature rather than entry num

    if ( !pThing->pSwapList )
    {
        pThing->pSwapList = pNewEntry;
    }
    else
    {
        pCurEnt = pThing->pSwapList;
        while ( pCurEnt->pNextEntry )
        {
            pCurEnt = pCurEnt->pNextEntry;
        }

        pCurEnt->pNextEntry = pNewEntry;
    }

    return pNewEntry->entryNum;
}

int J3DAPI sithThing_RemoveSwapEntry(SithThing* pThing, int refnum)
{
    int bFinished = 0;
    SithThingSwapEntry* pPrevEntry;
    SithThingSwapEntry* pNextEntry;
    SithThingSwapEntry* pCurrentEntry;

    if ( !pThing->pSwapList )
    {
        return 0;
    }

    if ( refnum < 0 )
    {
#line 501
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Please don't pass -1 as a refnum to sithThing_RemoveSwapEntry!\n", 0, 0, 0, 0);
        return 1;
    }

    pCurrentEntry = pThing->pSwapList;
    pPrevEntry = NULL;

    while ( !bFinished )
    {
        if ( pCurrentEntry->entryNum == refnum )
        {
            pNextEntry = pCurrentEntry->pNextEntry;
            if ( !pPrevEntry )
            {
                pThing->pSwapList = pNextEntry;
                stdMemory_Free(pCurrentEntry);
                bFinished++;
            }
            else
            {
                pPrevEntry->pNextEntry = pNextEntry;
                stdMemory_Free(pCurrentEntry);
                bFinished++;
            }
        }
        else
        {
            pPrevEntry = pCurrentEntry;
            pCurrentEntry = pCurrentEntry->pNextEntry;
            if ( !pCurrentEntry )
            {
                return 0;
            }
        }
    }

    return 0;
}

void J3DAPI sithThing_ResetSwapList(SithThing* pThing)
{
    while ( pThing->pSwapList )
    {
        sithThing_RemoveSwapEntry(pThing, pThing->pSwapList->entryNum);
    }
}

void J3DAPI sithThing_Update(float secDeltaTime, uint32_t msecDeltaTime)
{
    int curIndex;
    int curlastThingIdx;
    SithThing* pThing;

    SithWorld* pWorld = sithWorld_pCurrentWorld;
    curlastThingIdx = pWorld->lastThingIdx;
#line 568
    SITH_ASSERTREL(pWorld != NULL);

    for ( curIndex = 0, pThing = pWorld->aThings; curIndex <= curlastThingIdx; ++curIndex, ++pThing )
    {
        if ( !pThing->type )
        {
            continue;
        }

        if ( (pThing->flags & SITH_TF_DESTROYED) != 0 )
        {
            sithThing_RemoveThing(pWorld, pThing);
            continue;
        }

        if ( pThing->msecLifeLeft )
        {
            if ( pThing->msecLifeLeft <= msecDeltaTime )
            {
                sithThing_DestroyDyingThing(pThing);
            }
            else
            {
                pThing->msecLifeLeft -= msecDeltaTime;
            }
        }

        if ( (pThing->flags & SITH_TF_DISABLED) != 0 )
        {
            continue;
        }

        if ( (pThing->flags & (SITH_TF_TIMERSET | SITH_TF_PULSESET)) != 0 )
        {
            sithCog_UpdateThingTimer(pThing);
        }

        switch ( pThing->controlType )
        {
            case SITH_CT_EXPLOSION:
                sithExplosion_Update(pThing, secDeltaTime);
                break;

            case SITH_CT_AI:
                sithAIMove_Update(pThing, secDeltaTime);
                break;

            case SITH_CT_PARTICLE:
                sithParticle_Update(pThing, secDeltaTime);
                break;
        }

        switch ( pThing->type )
        {
            case SITH_THING_PLAYER:
                sithPlayer_Update(pThing->thingInfo.actorInfo.pPlayer, secDeltaTime);
                // Fall through to actor handling

            case SITH_THING_ACTOR:
                if ( pThing->thingInfo.actorInfo.pThingMeshAttached )
                {
                    SithThing* pThingAttached = pThing->thingInfo.actorInfo.pThingMeshAttached;
                    if ( pThingAttached->type == SITH_THING_PLAYER )
                    {
                        pThing->alpha = pThingAttached->alpha;
                    }
                }

                sithActor_Update(pThing, msecDeltaTime);
                break;

            case SITH_THING_SPRITE:
                if ( pThing->thingInfo.spriteInfo.pThingMeshAttached )
                {
                    SithThing* pMeshThing = pThing->thingInfo.spriteInfo.pThingMeshAttached;
                    if ( pMeshThing->type == SITH_THING_PLAYER )
                    {
                        float alpha = pThing->pTemplate->thingInfo.spriteInfo.alpha;
                        pThing->thingInfo.spriteInfo.alpha = alpha * pMeshThing->alpha; // TODO: why template alpha is multiplied by attached alpha?
                    }
                }

                sithSprite_Update(pThing, msecDeltaTime);
                break;

            case SITH_THING_WEAPON:
                sithWeapon_Update(pThing, secDeltaTime);
                break;

            default:
                break;
        }

        if ( sithThing_pfUnknownFunc && pThing->unknownFlags )
        {
            sithThing_pfUnknownFunc(pThing);
        }

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
                {
                    if ( pThing->type == SITH_THING_PLAYER && pThing != sithPlayer_pLocalPlayerThing )
                    {
                        pThing->moveInfo.physics.flags |= SITH_PF_UNKNOWN_8000;
                    }

                    sithPhysics_UpdateThing(pThing, secDeltaTime);
                }
                break;

            case SITH_MT_PATH:
                sithPathMove_Update(pThing, secDeltaTime);
                break;

            default:
                break;
        }

        // TODO: why is it assumed that pThing is actor here???
        if ( !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            sithThing_UpdateMove(pThing, secDeltaTime, msecDeltaTime);
        }

        sithPuppet_UpdatePuppet(pThing, secDeltaTime);
    }
}

void J3DAPI sithThing_UpdateMove(SithThing* pThing, float secDeltaTime, uint32_t msecDeltaTime)
{
    rdVector3 moveNorm;
    // Function update thing movement

    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    int moveFlags = 0;
    SithThing* pAttachedThing;
    if ( (pThing->attach.flags & SITH_ATTACH_NOMOVE) != 0 )
    {
        return;
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector_Copy3(&pThing->moveDir, &pPhysics->deltaVelocity);
        pThing->distanceMoved = 0.0f;
    }
    else // path
    {
        memset(&pThing->moveDir, 0, sizeof(pThing->moveDir));
        moveFlags |= 0x04;
    }

    if ( pThing->attach.flags
        && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0
        && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_SCROLLING) != 0 )
    {
        rdVector3 scrollDir;

        SithSurface* pSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
#line 742
        SITH_ASSERTREL(pSurface);

        
        sithAnimate_GetSurfaceScrollingDirection(pSurface, &scrollDir);

        rdVector_ScaleAdd3Acc(&pThing->moveDir, &scrollDir, secDeltaTime);
    }

    if ( pThing->moveDir.x != 0.0f || pThing->moveDir.y != 0.0f || pThing->moveDir.z != 0.0f )
    {
        float moveDist = rdVector_Normalize3(&moveNorm, &pThing->moveDir);
        float distanceMoved;

        if ( pThing->type == SITH_THING_PLAYER )
        {
            moveFlags |= 0x200;
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0
            && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83
            && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
        {
            SithSector* pNewSector;

            // Update Jeep move 

            rdVector3 newPos;
            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, sithThing_jeepPosOffset, &pThing->pos);

            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            rdVector_Copy3(&pThing->pos, &newPos);
            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            // Move thing
            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);

            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, -sithThing_jeepPosOffset, &pThing->pos);

            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            rdVector_Copy3(&pThing->pos, &newPos);
            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/0);

            // Update attached things
            for ( pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
            {
                if ( (pAttachedThing->attach.flags & SITH_ATTACH_NOMOVE) != 0 )
                {
                    rdMatrix_TransformVector34(&pAttachedThing->pos, &pAttachedThing->attach.posOffset, &pThing->orient);

                    rdVector_Add3Acc(&pAttachedThing->pos, &pThing->pos);


                    if ( pAttachedThing->pInSector != pThing->pInSector )
                    {
                        sithThing_SetSector(pAttachedThing, pThing->pInSector, /*bNotify=*/0);
                    }
                }
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            rdVector3 newPos;
            SithSector* pNewSector;

            // Update raft move

            rdVector3 normal;
            if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
            {
                rdVector_Copy3(&normal, &pThing->attach.attachedToStructure.pSurfaceAttached->face.normal);
            }
            else
            {
                rdVector_Copy3(&normal, &pThing->orient.uvec);
            }

            
            rdVector_ScaleAdd3(&newPos, &normal, sithThing_raftPosOffset, &pThing->pos);

            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            rdVector_Copy3(&pThing->pos, &newPos);
            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);

            rdVector_ScaleAdd3(&newPos, &normal, -sithThing_raftPosOffset, &pThing->pos);

            pNewSector = sithCollision_FindWaterSector(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            rdVector_Copy3(&pThing->pos, &newPos);
            if ( pNewSector != pThing->pInSector )
            {
                sithThing_SetSector(pThing, pNewSector, /*bNotify=*/0);
            }

            sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/0);
        }
        else if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
        {
            SithQuetzUserBlock* pQuetzUserBlock;
            rdVector3 newPos;
            SithSector* pNewSector;

            // Update tail attach move

            if ( !sithThing_CreateQuetzUserBlock(pThing) )
            {
                return;
            }

            pQuetzUserBlock = pThing->userblock.pQuetz;
#line 853
            SITH_ASSERTREL(!pQuetzUserBlock->bPosFudged);

            pQuetzUserBlock->bPosFudged = 1;
            pQuetzUserBlock->unknown393 = sithThing_tailPosOffset;

            
            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, sithThing_tailPosOffset, &pThing->pos);

            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            rdVector_Copy3(&pThing->pos, &newPos);
            sithThing_SetSector(pThing, pNewSector, /*bNotify=*/1);

            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);

            rdVector_ScaleAdd3(&newPos, &pThing->orient.uvec, -sithThing_tailPosOffset, &pThing->pos);

            pNewSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &newPos, 0.0f);

            rdVector_Copy3(&pThing->pos, &newPos);
            sithThing_SetSector(pThing, pNewSector, /*bNotify=*/1);

            pQuetzUserBlock->bPosFudged = 0;
            sithPhysics_FindFloor(pThing, 0);
        }
        else
        {
            distanceMoved = sithCollision_MoveThing(pThing, &moveNorm, moveDist, moveFlags);
        }

        pThing->distanceMoved = distanceMoved;
#line 878
        SITH_ASSERTREL(distanceMoved >= ((float)0.0));
    }
    else
    {
        if ( pThing->moveType == SITH_MT_PHYSICS
            && (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0
            && pThing->attach.attachedToStructure.pThingAttached->moveType == SITH_MT_PATH
            && pThing->moveStatus != SITHPLAYERMOVE_HANGING )
        {
            sithPhysics_FindFloor(pThing, 0);
        }
    }
    if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
    {
        sithThing_UpdateQuetzUserBlock(pThing);
    }
}

void J3DAPI sithThing_DestroyDyingThing(SithThing* pThing)
{
    switch ( pThing->type )
    {
        case SITH_THING_WEAPON:
            sithWeapon_DestroyWeapon(pThing);
            break;

        case SITH_THING_PLAYER:
#line 922
            SITH_ASSERTREL(0);
            break;

        case SITH_THING_PARTICLE:
            sithParticle_DestroyParticle(pThing);
            break;

        case SITH_THING_CORPSE:
            sithActor_DestroyCorpse(pThing);
            break;

        case SITH_THING_ACTOR:
            sithActor_DestroyActor(pThing);
            break;

        case SITH_THING_ITEM:
            sithItem_DestroyItem(pThing);
            break;

        default:
            sithThing_DestroyThing(pThing);
            break;
    }
}

SithThing* J3DAPI sithThing_GetThingParent(const SithThing* pThing)
{
    while ( pThing->pParent && pThing->parentSignature == pThing->pParent->signature )
    {
        pThing = pThing->pParent;
    }

    return (SithThing*)pThing;
}

SithThing* J3DAPI sithThing_GetThingByIndex(int idx)
{
    if ( idx < 0 || idx >= (int)sithWorld_pCurrentWorld->numThings || !sithWorld_pCurrentWorld->aThings[idx].type )
    {
        return NULL;
    }

    return &sithWorld_pCurrentWorld->aThings[idx];
}

int J3DAPI sithThing_GetThingIndex(const SithThing* pThing)
{
    if ( !pThing )
    {
        return -1;
    }

    if ( pThing < sithWorld_pCurrentWorld->aThings || pThing >= &sithWorld_pCurrentWorld->aThings[sithWorld_pCurrentWorld->numThings] )
    {
        return -1;
    }

    if ( pThing->type == SITH_THING_FREE )
    {
        return -1;
    }

    return pThing->idx;
}


SithThing* J3DAPI sithThing_GetGuidThing(int guid)
{
    int i;

    if ( guid < 0 )
    {
        return NULL;
    }

    if ( (guid & 0xFFFF0000) == 0 && guid < (int)sithWorld_pCurrentWorld->numThings && sithWorld_pCurrentWorld->aThings[guid].type )
    {
        return &sithWorld_pCurrentWorld->aThings[guid];
    }

    for ( i = 0; i <= sithWorld_pCurrentWorld->lastThingIdx; ++i )
    {
        if ( sithWorld_pCurrentWorld->aThings[i].guid == guid && sithWorld_pCurrentWorld->aThings[i].type )
        {
            return &sithWorld_pCurrentWorld->aThings[i];
        }
    }

    return NULL;
}

void J3DAPI sithThing_DestroyThing(SithThing* pThing)
{
#line 1079
    SITH_ASSERTREL(pThing != NULL);

    pThing->flags |= SITH_TF_DESTROYED;
    sithCog_ThingSendMessage(pThing, 0, SITHCOG_MSG_REMOVED);
}

float J3DAPI sithThing_DamageThing(SithThing* pThing, const SithThing* pDamageThing, float damage, SithDamageType hitType)
{
#line 1111
    SITH_ASSERTREL(pThing);

    if ( damage <= 0.0f || (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return 0.0f;
    }
    else
    {
        damage = (unsigned int)sithCog_ThingSendMessageEx(pThing, pDamageThing, SITHCOG_MSG_DAMAGED, (int32_t)damage, hitType, 0, 0);
    }

    if ( damage > 0.0f )
    {
        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
            {
                if ( ((hitType & SITH_DAMAGE_RAZOR_ROCK) != 0 || (hitType & SITH_DAMAGE_RAFT_LEAK) != 0)
                    && (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
                {
                    damage = damage - sithActor_DamageRaftActor(pThing, (SithThing*)pDamageThing, damage, hitType);
                }
                else
                {
                    damage = damage - sithActor_DamageActor(pThing, (SithThing*)pDamageThing, damage, hitType);
                }
                break;
            }

            case SITH_THING_WEAPON:
                sithWeapon_DamageWeapon(pThing, pDamageThing, damage);
                break;

            case SITH_THING_COG:
                sithThing_PlayCogDamageSound(pThing, hitType);
                break;

            default:
                break;
        }
    }

    return damage;
}

float J3DAPI sithThing_ScaleCombatDamage(const SithThing* pThing, SithThing* pHitThing)
{
    const SithExplosionInfo* pExplosionInfo;
    float positionX;
    float positionY;
    float positionZ;
    float hitRange;
    int curWeapon;
    float maxAimDist;
    float weaponHitProb;
    SithAIControlBlock* pLocal = NULL;
    float scaleFactor;
    float damage;
    const SithWeaponInfo* pWeaponInfo;
    SithThing* pShooter;

    if ( !pThing )
    {
#line 1223
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad params passed to sithThing_ScaleCombatDamage().\n", 0, 0, 0, 0);
        return 0.0f;
    }

    switch ( pThing->type )
    {
        case SITH_THING_WEAPON:
            pWeaponInfo = &pThing->thingInfo.weaponInfo;
            damage      = pWeaponInfo->damage;
            break;

        case SITH_THING_EXPLOSION:
            pExplosionInfo = &pThing->thingInfo.explosionInfo;
            damage         = pExplosionInfo->damage;
            break;

        default:
            damage = 0.0f;
            break;
    }

    if ( damage == 0.0f )
    {
        return damage;
    }

    pShooter = sithThing_GetThingParent(pThing);
    if ( !pShooter )
    {
        return damage;
    }

    if ( !pHitThing )
    {
        return damage;
    }

    switch ( pHitThing->controlType )
    {
        case SITH_CT_PLAYER:
            break;

        case SITH_CT_AI:
            pLocal = pHitThing->controlInfo.aiControl.pLocal;
            break;

        default:
            return damage;
    }

    switch ( pHitThing->type )
    {
        case SITH_THING_ACTOR:
            // From here on logic is for hit AI actor
            switch ( pShooter->type )
            {
                case SITH_THING_ACTOR:
                    // Retail bug: pWeaponInfo is uninitialized when pThing is an explosion
                    if ( pWeaponInfo->damageType != SITH_DAMAGE_DROWN
                        && pWeaponInfo->damageType != SITH_DAMAGE_CRUSH
                        && (pShooter->thingInfo.actorInfo.flags & SITH_AF_FULLDAMAGE) == 0 )
                    {
                        scaleFactor = 0.5f;
                        damage *= scaleFactor;
                    }
                    break;

                case SITH_THING_PLAYER:
                    // Player is damaging actor
                    curWeapon = sithInventory_GetCurrentWeapon(pShooter);
                    if ( !J3DINRANGE(curWeapon, SITHWEAPON_PISTOL, SITHWEAPON_SHOTGUN) )
                    {
                        goto done;
                    }

                    // Note, pThing should be weapon from here on because the check above makes sure the weapon is not
                    // a grenade or rocket launcher where an explosion could damage the actor
                    maxAimDist = sithWeapon_GetWeaponMaxAimDistance((SithWeaponId)curWeapon);
                    positionX   = pThing->pos.x - pShooter->pos.x;
                    positionY   = pThing->pos.y - pShooter->pos.y;
                    positionZ   = D3DVAL(pThing->pos.z - pShooter->pos.z);
                    hitRange = sqrtf(positionX * positionX + positionY * positionY + positionZ * positionZ) - pShooter->collide.size - pHitThing->collide.size;

                    scaleFactor = sithGetCombatDamageScalar() * 0.25f;
                    if ( maxAimDist * scaleFactor > hitRange )
                    {
                        goto missed;
                    }

                    weaponHitProb = sithThing_aWeaponCoefficients[curWeapon];
                    if ( SITH_RAND() <= weaponHitProb )
                    {
                        goto missed;
                    }

                    if ( pHitThing->moveInfo.physics.mass > 0.0f && pHitThing->moveInfo.physics.mass < 50.0f )
                    {
                        scaleFactor = pHitThing->moveInfo.physics.mass / 50.0f; // TODO: Why?
                        damage *= scaleFactor;
                    }

                    if ( pLocal )
                    {
                        if ( (pLocal->mode & SITHAI_MODE_TOUGHSKIN) != 0 )
                        {
                            scaleFactor = sithGetCombatDamageScalar() * 0.75f;
                            damage *= scaleFactor;
                        }

                        if ( (pLocal->mode & SITHAI_MODE_ARMOREDSKIN) != 0 )
                        {
                            scaleFactor = sithGetCombatDamageScalar() * 0.5f;
                            damage *= scaleFactor;
                        }

                        if ( (pLocal->mode & (SITHAI_MODE_FLEEING | SITHAI_MODE_UNKNOWN_10)) != 0 )
                        {
                            if ( hitRange <= maxAimDist )
                            {
                                scaleFactor = sithGetCombatDamageScalar() * 0.9f;
                                damage *= scaleFactor;
                            }
                            else
                            {
                                scaleFactor = sithGetCombatDamageScalar() * 0.8f;
                                damage *= scaleFactor;
                            }
                        }
                    }
                    break;
            }
            break;

        case SITH_THING_PLAYER:
            // Hit Player
            scaleFactor = sithGetHitAccuarancyScalar();
            damage *= scaleFactor;
            break;
    }

done:
    return damage;

missed:
    if ( pThing->pTemplate )
    {
        damage = pThing->pTemplate->thingInfo.weaponInfo.damage; // TODO: Check that there is no case where pThing is an explosion
    }

    return damage;
}

void J3DAPI sithThing_PlayCogDamageSound(SithThing* pThing, SithDamageType hitType)
{
    tSoundHandle hSnd = 0;
    if ( (hitType & SITH_DAMAGE_IMPACT) != 0 )
    {
        if ( !sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTIMPACT) && (pThing->flags & (SITH_TF_EARTH | SITH_TF_SNOW | SITH_TF_WOOD)) == 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(rand() % 2 + 141)); // gen_ricochet_a.wav or gen_ricochet_b.wav
        }
    }
    else if ( (hitType & SITH_DAMAGE_MACHETE) != 0 && !sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HURTMACHETE) )
    {
        if ( (pThing->flags & SITH_TF_METAL) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(82)); // 0x8052 - gen_machete_hit_metal.wav
        }
        else if ( (pThing->flags & (SITH_TF_EARTH | SITH_TF_SNOW)) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(84)); // 0x8054 - gen_machete_hit_vine.wav
        }
        else if ( (pThing->flags & SITH_TF_WOOD) != 0 )
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(83)); // 0x8053 - gen_machete_hit_wood.wav
        }
        else
        {
            hSnd = Sound_GetSoundHandle(SITHWORLD_STATICINDEX(81)); // 0x8051 - gen_machete_hit_stone.wav
        }
    }

    if ( hSnd )
    {
        sithSoundMixer_PlaySoundThing(hSnd, pThing, 1.0f, 0.5f, 2.5f, (SoundPlayFlag)0);
    }
}

int J3DAPI sithThing_AllocWorldThings(SithWorld* pWorld, size_t numThings)
{
#line 1468
    SITH_ASSERTREL((numThings <= SITHTHING_MAXTHINGS));
    SITH_ASSERTREL(pWorld->aThings == NULL);
    SITH_ASSERTREL(bThingStartup);

#line 1473
    pWorld->aThings = (SithThing*)STDMALLOC(sizeof(SithThing) * numThings);
    if ( !pWorld->aThings )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to allocated mem for %d things.\n", numThings, 0, 0, 0);
        return 1;
    }

    memset(pWorld->aThings, 0, sizeof(SithThing) * numThings);
    pWorld->numThings = numThings;

    sithThing_InitializeWorldThings(pWorld);
    return 0;
}

void J3DAPI sithThing_FreeWorldThings(SithWorld* pWorld)
{
#line 1504
    SITH_ASSERTREL(pWorld);
    if ( pWorld->aThings )
    {
        sithThing_RemoveWorldThings(pWorld);
        stdMemory_Free(pWorld->aThings);

        pWorld->aThings      = NULL;
        pWorld->numThings    = 0;
        pWorld->lastThingIdx = -1;
    }
}

void J3DAPI sithThing_RemoveWorldThings(SithWorld* pWorld)
{
#line 1539
    SITH_ASSERTREL(pWorld);

    if ( pWorld->aThings )
    {
        size_t i;

        for ( i = 0; i < pWorld->numThings; ++i )
        {
            SithThing* pThing = &pWorld->aThings[i];
            if ( pThing->type )
            {
                sithThing_RemoveThing(pWorld, pThing);
            }
        }
    }
}

void J3DAPI sithThing_InitializeWorldThings(SithWorld* pWorld)
{
    int thingNum;

    sithThing_numFreeThings = 0;
    for ( thingNum = pWorld->numThings - 1; thingNum >= 0; --thingNum )
    {
        sithThing_Reset(&pWorld->aThings[thingNum]);

        pWorld->aThings[thingNum].idx  = thingNum;
        pWorld->aThings[thingNum].guid = -1;
        sithThing_FreeThingIndex(pWorld, thingNum);
    }
}

void J3DAPI sithThing_LoadPostProcess(SithWorld* pWorld)
{
    int thingNum;

    sithThing_numFreeThings = 0;
    pWorld->lastThingIdx    = -1;

    for ( thingNum = pWorld->numThings - 1; thingNum >= 0; --thingNum )
    {
        if ( !pWorld->aThings[thingNum].type )
        {
            sithThing_FreeThingIndex(pWorld, thingNum);
        }
        else if ( thingNum > pWorld->lastThingIdx )
        {
            pWorld->lastThingIdx = thingNum;
        }
    }
}

void J3DAPI sithThing_RemoveThing(SithWorld* pWorld, SithThing* pThing)
{
    if ( stdComm_IsGameActive() && stdComm_IsGameHost() && (pThing->guid & 0xFFFF0000) == 0 )
    {
        sithMulti_RemoveStaticThing(pThing->guid);
    }

    sithThing_FreeThing(pWorld, pThing);
    sithThing_FreeThingIndex(pWorld, pThing->idx);
}

void J3DAPI sithThing_FreeThing(SithWorld* pWorld, SithThing* pThing)
{
#line 1666
    SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing));
    SITH_ASSERTREL((pThing->type != SITH_THING_FREE));

    if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
    }

    sithThing_DetachAttachedThings(pThing);

    if ( pThing->pInSector )
    {
        sithThing_ExitSector(pThing);
    }

    if ( pThing->moveType == SITH_MT_PATH && pThing->moveInfo.pathMovement.aFrames )
    {
        stdMemory_Free(pThing->moveInfo.pathMovement.aFrames);
    }

    if ( pThing->controlType == SITH_CT_AI )
    {
        sithAI_Free(pThing);
    }

    if ( pThing->type == SITH_THING_PARTICLE )
    {
        sithParticle_Free(pThing);
    }

    if ( pThing->pPuppetClass )
    {
        sithPuppet_Free(pThing);
    }

    sithThing_ResetSwapList(pThing);
    rdThing_FreeEntry(&pThing->renderData);
    sithSoundMixer_StopAllSoundsThing(pThing);

    if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
    {
        sithVoice_UpdateLipSync(pThing);
    }

    if ( pThing->userblock.pMinecar ) // Note This could UB as different union member could be set
    {
        stdMemory_Free(pThing->userblock.pMinecar);
        pThing->userblock.pMinecar = NULL;
    }

    pThing->type      = SITH_THING_FREE;
    pThing->signature = 0;
    pThing->guid      = -1;
}

void J3DAPI sithThing_Initialize(const SithWorld* pWorld, SithThing* pThing, int bFindFloor)
{
#line 1740
    SITH_ASSERTREL(pThing);

    switch ( pThing->type )
    {
        case SITH_THING_EXPLOSION:
            sithExplosion_Initialize(pThing);
            break;

        case SITH_THING_SPRITE:
            sithSprite_Initialize(pThing);
            break;

        case SITH_THING_PARTICLE:
            sithParticle_Initalize(pThing);
            break;

        case SITH_THING_ITEM:
            sithItem_Initialize(pThing);
            break;

        case SITH_THING_ACTOR:
        {
            if ( pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.red == 0.0f
                && pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.green == 0.0f
                && pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.blue == 0.0f
                && pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.alpha == 0.0f )
            {
                rdVector_Set4(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top, -1.0f, -1.0f, -1.0f, -1.0f);
            }

            pThing->thingInfo.actorInfo.aiState = -1;
            pThing->thingInfo.actorInfo.aiArmedModeState = -1;
            sithWeapon_InitalizeActor(pThing);
            sithActor_SetDifficulty(pThing);
        } break;

        case SITH_THING_PLAYER:
        {
            if ( pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.red == 0.0f
                && pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.green == 0.0f
                && pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.blue == 0.0f
                && pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top.alpha == 0.0f )
            {
                rdVector_Set4(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor.top, -1.0f, -1.0f, -1.0f, -1.0f);
            }

            pThing->thingInfo.actorInfo.aiState = -1;
            pThing->thingInfo.actorInfo.aiArmedModeState = -1;
            sithWeapon_InitalizeActor(pThing);
        } break;

        default:
            break;
    }

    if ( pThing->renderData.pPuppet )
    {
        sithPuppet_New(pThing);
    }

    if ( pThing->controlType == SITH_CT_AI )
    {
        sithAI_Create(pThing);
    }

    if ( pThing->pSoundClass )
    {
        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_CREATE);
    }

    if ( bFindFloor && (pWorld->state & SITH_WORLD_STATE_INITIALIZED) != 0 )
    {
        if ( pThing->pCog && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
        {
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_INITIALIZED, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }

        if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & (SITH_PF_WALLSTICK | SITH_PF_FLOORSTICK)) != 0 )
        {
            sithPhysics_FindFloor(pThing, /*bNoThingStateUpdate=*/1);
        }
    }
}

void J3DAPI sithThing_Reset(SithThing* pThing)
{
    int idx      = pThing->idx;
    uint32_t sig = pThing->signature;

    memset(pThing, 0, sizeof(SithThing));
    pThing->alpha = 1.0f;

    memcpy(&pThing->orient, &rdroid_identMatrix34, sizeof(rdMatrix34));
    rdThing_NewEntry(&pThing->renderData, pThing);

    pThing->idx       = idx;
    pThing->signature = sig;
}

void J3DAPI sithThing_SetSector(SithThing* pThing, SithSector* pSector, int bNotify)
{
#line 1875
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));

    if ( pThing->pInSector )
    {
        if ( pThing->pInSector == pSector )
        {
            return;
        }

        sithThing_ExitSector(pThing);
    }

    sithThing_EnterSector(pThing, pSector, /*bNoWaterSplash=*/0, bNotify);
}

void J3DAPI sithThing_ExitSector(SithThing* pThing)
{
    rdVector3 prevPos;

    if ( sithWorld_pCurrentWorld )
    {
#line 1916
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
        SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pThing->pInSector));
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_COGLINKED) != 0 )
    {
        rdVector_Copy3(&prevPos, &pThing->pos);

        if ( (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
        {
            sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_EXITED);
        }

        if ( memcmp(&prevPos, &pThing->pos, sizeof(rdVector3)) != 0 ) // pos not equal
        {
#line 1934
            SITH_ASSERTREL(pThing->pInSector);
            return;
        }
    }

    if ( !pThing->pPrevThingInSector )
    {
        pThing->pInSector->pFirstThingInSector = pThing->pNextThingInSector;
        if ( pThing->pNextThingInSector )
        {
            pThing->pNextThingInSector->pPrevThingInSector = NULL;
        }
    }
    else
    {
        pThing->pPrevThingInSector->pNextThingInSector = pThing->pNextThingInSector;
        if ( pThing->pNextThingInSector )
        {
            pThing->pNextThingInSector->pPrevThingInSector = pThing->pPrevThingInSector;
        }
    }

    pThing->pInSector          = NULL;
    pThing->pPrevThingInSector = NULL;
    pThing->pNextThingInSector = NULL;
}

void J3DAPI sithThing_EnterSector(SithThing* pThing, SithSector* pNewSector, int bNoWaterSplash, int bNoNotify)
{
#line 1983
    SITH_ASSERTREL(pThing->pInSector == NULL);
    SITH_ASSERTREL(pNewSector->pFirstThingInSector != pThing);
    SITH_ASSERTREL(pThing->type != SITH_THING_FREE);

    pThing->pNextThingInSector = pNewSector->pFirstThingInSector;
    if ( pThing->pNextThingInSector )
    {
        pThing->pNextThingInSector->pPrevThingInSector = pThing;
    }

    pThing->pPrevThingInSector      = NULL;
    pNewSector->pFirstThingInSector = pThing;
    pThing->pInSector              = pNewSector;

    if ( (pNewSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pNewSector->flags & SITH_SECTOR_AETHERIUM) != 0 && pThing->type == SITH_THING_PLAYER )
    {
        if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_NOMOVE) == 0 && pThing->moveType == SITH_MT_PHYSICS )
        {
            sithThing_DetachThing(pThing);
        }

        if ( (pThing->flags & SITH_TF_SUBMERGED) == 0 )
        {
            if ( (pNewSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                sithInventory_SetSwimmingInventory(pThing, 0);
                sithPlayer_bInAetheriumSector = 1;
                sithWeapon_DeselectWeapon(pThing);

                pThing->flags &= ~SITH_TF_SHADOW;

            }
            else
            {
                sithThing_EnterWater(pThing, bNoWaterSplash | bNoNotify);
            }
        }
    }
    else if ( (pThing->flags & SITH_TF_SUBMERGED) != 0 )
    {
        if ( (pNewSector->flags & SITH_SECTOR_AETHERIUM) == 0 )
        {
            sithThing_ExitWater(pThing, bNoWaterSplash | bNoNotify);
        }
    }
    else if ( pThing->type == SITH_THING_PLAYER && sithPlayer_bInAetheriumSector == 1 )
    {
        sithInventory_SetSwimmingInventory(pThing, 1);
        sithPlayer_bInAetheriumSector = 0;
        pThing->flags |= SITH_TF_SHADOW;
    }

    if ( !bNoNotify && (pThing->pInSector->flags & SITH_SECTOR_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_SectorSendMessage(pThing->pInSector, pThing, SITHCOG_MSG_ENTERED);
    }
}

void J3DAPI sithThing_EnterWater(SithThing* pThing, int bNoSplash)
{
    pThing->flags |= SITH_TF_SUBMERGED;
    if ( pThing->pPuppetClass )
    {
        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);

        if ( pThing == sithPlayer_pLocalPlayerThing && pThing->moveStatus != SITHPLAYERMOVE_PULLINGUP )
        {
            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
        }
    }

    if ( (pThing->flags & SITH_TF_WATERDESTROYED) != 0 )
    {
        sithThing_DestroyThing(pThing);
        return;
    }

    if ( bNoSplash )
    {
        return;
    }

    if ( pThing->pSoundClass )
    {
        if ( pThing->moveType == SITH_MT_PHYSICS && pThing->moveInfo.physics.velocity.z > 0.75f ) // TODO: note in ExitWater function speed is calculated from velocity
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_ENTERWATERSLOW);
        }
        else
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_ENTERWATER);
        }
    }

    if ( sithPlayer_pLocalPlayerThing && (pThing->flags & SITH_TF_SPLASH) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
    {
        if ( pThing->pCog )
        {
            sithCog_SendMessage(pThing->pCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 1, 0);
        }

        if ( pThing->pCaptureCog )
        {
            sithCog_SendMessage(pThing->pCaptureCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 1, 0);
        }
    }

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        pThing->moveInfo.physics.velocity.z = RD_MULTIPLY(pThing->moveInfo.physics.velocity.z, 0.25f); // Slowdown fall

        if ( pThing->type == SITH_THING_WEAPON
            && (pThing->moveInfo.physics.flags & SITH_PF_USEGRAVITY) == 0
            && (pThing->thingInfo.weaponInfo.damageType & SITH_DAMAGE_IMPACT) != 0 )
        {
            rdVector_Scale3Acc(&pThing->moveInfo.physics.velocity, 0.5f); // Slowdown weapon
        }
    }
}

void J3DAPI sithThing_ExitWater(SithThing* pThing, int bNoSplash)
{
    int bFalling = 0;

    pThing->flags &= ~SITH_TF_SUBMERGED;

    if ( pThing == sithPlayer_pLocalPlayerThing
        && pThing->moveStatus == SITHPLAYERMOVE_SWIMIDLE
        && !pThing->thingInfo.actorInfo.bForceMovePlay
        && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        bFalling = 1;
        pThing->moveStatus = SITHPLAYERMOVE_FALLING;

        sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
        sithPhysics_SetThingLook(pThing, &rdroid_zVector3, 0.0f);
    }

    if ( pThing->pSoundClass )
    {
        if ( pThing->moveType == SITH_MT_PHYSICS && rdVector_Len3(&pThing->moveInfo.physics.velocity) < 0.75f )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_EXITWATERSLOW);
        }
        else
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_EXITWATER);
        }
    }

    if ( (pThing->flags & SITH_TF_AIRDESTROYED) != 0 )
    {
        sithThing_DestroyThing(pThing);
        return;
    }

    if ( bNoSplash )
    {
        return;
    }

    if ( pThing == sithPlayer_pLocalPlayerThing && (pThing->flags & SITH_TF_SPLASH) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 && !bFalling )
    {
        if ( sithPlayer_pLocalPlayerThing->pCog )
        {
            sithCog_SendMessage(sithPlayer_pLocalPlayerThing->pCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }

        if ( sithPlayer_pLocalPlayerThing->pCaptureCog )
        {
            sithCog_SendMessage(sithPlayer_pLocalPlayerThing->pCaptureCog, SITHCOG_MSG_SPLASH, SITHCOG_SYM_REF_THING, pThing->idx, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }
}

SithThing* J3DAPI sithThing_Create(SithThingType type)
{
    SithThing* pThing;

    int thingIndex = sithThing_GetFreeThingIndex();
    if ( thingIndex < 0 && type != SITH_THING_EXPLOSION && type != SITH_THING_DEBRIS && type != SITH_THING_PARTICLE )
    {
        sithThing_PurgeGarbageThings(sithWorld_pCurrentWorld);
        thingIndex = sithThing_GetFreeThingIndex();
    }

    if ( thingIndex < 0 )
    {
#line 2229
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Call to sithThing_Create could not be satisfied - no free objects.\n", 0, 0, 0, 0);
        return NULL;
    }

    pThing = &sithWorld_pCurrentWorld->aThings[thingIndex];
#line 2235
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->type == SITH_THING_FREE);

    sithThing_Reset(pThing);
    pThing->idx = thingIndex;

    if ( !sithThing_guidEntropy )
    {
        sithThing_guidEntropy = 1;
    }

    pThing->guid      = ((sithPlayer_playerNum + 1) << 16) | (sithThing_guidEntropy++ & 0xFFFF);
    pThing->signature = sithThing_curSignature++;

    if ( !sithThing_curSignature ) {
        sithThing_curSignature = 1;
    }

    return pThing;
}

void J3DAPI sithThing_SetPositionAndOrient(SithThing* pThing, const rdVector3* pos, const rdMatrix34* pOrient)
{
#line 2287
    SITH_ASSERTREL(pThing->pInSector == NULL);

    rdVector_Copy3(&pThing->pos, pos);
    memcpy(&pThing->orient, pOrient, sizeof(pThing->orient));
    memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec));
}

int J3DAPI sithThing_SetThingModel(SithThing* pThing, rdModel3* pModel)
{
    rdPuppet* pPuppet;

#line 2303
    SITH_ASSERTREL(pThing && pModel);

    if ( pThing->renderData.type != RD_THING_MODEL3 || pThing->renderData.data.pModel3 != pModel )
    {
        pPuppet = pThing->renderData.pPuppet;
        pThing->renderData.pPuppet = NULL;

        rdThing_FreeEntry(&pThing->renderData);
        rdThing_NewEntry(&pThing->renderData, pThing);

        rdThing_SetModel3(&pThing->renderData, pModel);
        pThing->renderData.pPuppet = pPuppet;

        return 1;
    }

    return 0;
}

void J3DAPI sithThing_SetThingBasedOn(SithThing* pThing, const SithThing* pTemplate)
{
    int guid;
    int j;
    uint32_t cursignature;
    SithThing* rdThingThing;

#line 2353
    SITH_ASSERTREL(pThing != pTemplate);

    j = pThing->idx;
    guid = pThing->guid;
    cursignature = pThing->signature;
    rdThingThing = pThing->renderData.pThing;

    if ( pTemplate )
    {
        memcpy(pThing, pTemplate, sizeof(SithThing));

        if ( pThing->renderData.type == RD_THING_MODEL3 )
        {
            rdThing_SetModel3(&pThing->renderData, pThing->renderData.data.pModel3);
        }
        else if ( pThing->renderData.type == RD_THING_PARTICLE )
        {
            rdThing_SetParticleCloud(&pThing->renderData, pThing->renderData.data.pParticle);
        }
        else if ( pThing->renderData.type == RD_THING_SPRITE3 )
        {
            rdThing_SetSprite3(&pThing->renderData, pThing->renderData.data.pSprite3);
            sithSprite_Create(pThing);
        }

        if ( pThing->pPuppetClass )
        {
            rdPuppet_New(&pThing->renderData);
        }

        if ( pThing->moveType == SITH_MT_PATH && pThing->moveInfo.pathMovement.aFrames )
        {
#line 2391
            SITH_ASSERTREL(pThing->moveInfo.pathMovement.sizeFrames > 0);
            pThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * pThing->moveInfo.pathMovement.sizeFrames);
            memcpy(pThing->moveInfo.pathMovement.aFrames, pTemplate->moveInfo.pathMovement.aFrames, sizeof(SithPathFrame) * pThing->moveInfo.pathMovement.sizeFrames);
        }
    }
    else
    {
        sithThing_Reset(pThing);
    }

    pThing->idx               = j;
    pThing->guid              = guid;
    pThing->signature         = cursignature;
    pThing->renderData.pThing = rdThingThing;
    pThing->pTemplate         = pTemplate;
}

SithThing* J3DAPI sithThing_CreateThingAtPos(const SithThing* pTemplate, const rdVector3* pos, const rdMatrix34* orient, SithSector* pSector, SithThing* pParent)
{
    SithThing* pObj;
    SithThing* pCreateThing;

#line 2438
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));
    SITH_ASSERTREL(pTemplate != NULL);

    pObj = sithThing_Create(pTemplate->type);
    if ( !pObj )
    {
#line 2450
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Failed to create thing from template %s.\n", pTemplate->aName, 0, 0, 0);
        return NULL;
    }

    if ( pTemplate->aName[0] == '_' )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning -- create object from base-class template %s\n", pTemplate->aName, 0, 0, 0);
    }

    sithThing_SetThingBasedOn(pObj, pTemplate);
    sithThing_SetPositionAndOrient(pObj, pos, orient);
    rdMatrix_PreMultiply34(&pObj->orient, &pTemplate->orient);

    sithThing_EnterSector(pObj, pSector, /*bNoWaterSplash=*/1, /*bNoNotify*/0);

    if ( pParent )
    {
        pObj->pParent         = pParent;
        pObj->parentSignature = pParent->signature;
    }

    sithThing_Initialize(sithWorld_pCurrentWorld, pObj, 1);
    if ( pObj->type == SITH_THING_EXPLOSION )
    {
        sithExplosion_CreateSpriteThing(pObj);
    }

    if ( pObj->moveType == SITH_MT_PHYSICS && (pObj->moveInfo.physics.flags & SITH_PF_STARTORIENTMOVE) == 0 )
    {
        rdMatrix_TransformVector34Acc(&pObj->moveInfo.physics.velocity, &pObj->orient);
    }

    if ( pObj->pCog && (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) == 0 )
    {
        sithCog_SendMessage(pObj->pCog, SITHCOG_MSG_CREATED, SITHCOG_SYM_REF_THING, pObj->idx, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    if ( pObj->pCreateThingTemplate && pObj->type != SITH_THING_EXPLOSION )
    {
        pCreateThing = sithThing_CreateThingAtPos(pObj->pCreateThingTemplate, pos, orient, pSector, pParent);
        if ( pCreateThing && (pObj->flags & SITH_TF_REMOTE) != 0 )
        {
            pCreateThing->flags |= SITH_TF_REMOTE;
        }
    }

    return pObj;
}

SithThing* J3DAPI sithThing_CreateThing(const SithThing* pTemplate, SithThing* pMarker)
{
    rdVector3 rotOffset;
    rdVector3 markerPivotPos;
    rdVector3 pos;
    SithSector* pNewSector;
    SithThing* pSpawned;

    rdVector3 offset;
    switch ( pTemplate->renderData.type )
    {
        case RD_THING_MODEL3:
            rdVector_Copy3(&offset, &pTemplate->renderData.data.pModel3->insertOffset);
            break;
        case RD_THING_SPRITE3:
            rdVector_Copy3(&offset, &pTemplate->renderData.data.pSprite3->offset);
            break;
        default:
            memset(&offset, 0, sizeof(offset));
            break;
    }

    
    switch ( pMarker->renderData.type )
    {
        case RD_THING_MODEL3:
            rdVector_Copy3(&markerPivotPos, &pMarker->renderData.data.pModel3->insertOffset);
            break;
        case RD_THING_SPRITE3:
            rdVector_Copy3(&markerPivotPos, &pMarker->renderData.data.pSprite3->offset);
            break;
        default:
            memset(&markerPivotPos, 0, sizeof(markerPivotPos));
            break;
    }

    rdVector_Sub3Acc(&offset, &markerPivotPos);
    
    rdMatrix_TransformVector34(&rotOffset, &offset, &pMarker->orient);

    
    rdVector_Add3(&pos, &pMarker->pos, &rotOffset);
    pNewSector = sithCollision_FindSectorInRadius(pMarker->pInSector, &pMarker->pos, &pos, 0.0f);

    pSpawned = sithThing_CreateThingAtPos(pTemplate, &pos, &pMarker->orient, pNewSector, NULL);
    if ( !pSpawned )
    {
        return NULL;
    }

    switch ( pSpawned->type )
    {
        case SITH_THING_SPRITE:
            sithSprite_Create(pSpawned);
            break;
    }

    if ( pSpawned->moveType == SITH_MT_PATH
        && pMarker->moveType == SITH_MT_PATH
        && pMarker->moveInfo.pathMovement.aFrames
        && !pSpawned->moveInfo.pathMovement.aFrames )
    {
        sithPathMove_Create(pSpawned, pMarker, &offset);
    }

    return pSpawned;
}

void J3DAPI sithThing_AttachThingToSurface(SithThing* pThing, SithSurface* pSurface, int bNoImpactUpdate)
{
    int bNoPrevAttach = 1;
    float impactForce;

#line 2634
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithSurface_ValidateSurfacePointer(pSurface));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && pThing->attach.attachedToStructure.pSurfaceAttached == pSurface )
        {
            return;
        }

        bNoPrevAttach = 0;
        sithThing_DetachThing(pThing);
    }

    if ( (pSurface->flags & SITH_SURFACE_LAVA) != 0 )
    {
        if ( pThing->type == SITH_THING_PLAYER && (sithPuppet_bPlayerLeapForward == 1 || pThing->moveStatus == SITHPLAYERMOVE_LEAPFWD) )
        {
            goto attach;
        }

        if ( (pThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
        {
            goto attach;
        }

        if ( pThing->type == SITH_THING_WEAPON || pThing->type == SITH_THING_ITEM )
        {
            sithThing_DestroyThing(pThing);
        }

        if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
        {
            goto attach;
        }

        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
        {
            goto attach;
        }

        if ( pThing->type == SITH_THING_ACTOR
            && pThing->controlInfo.aiControl.pLocal
            && (pThing->controlInfo.aiControl.pLocal->allowedSurfaceTypes & SITH_SURFACE_LAVA) != 0 )
        {
            goto attach;
        }

        if ( (pSurface->flags & SITH_SURFACE_WATER) != 0 )
        {
            sithActor_KillActor(pThing, NULL, SITH_DAMAGE_COLD_WATER);
        }
        else
        {
            sithActor_KillActor(pThing, NULL, SITH_DAMAGE_LAVA);
        }
    }

    // Handle attaching to killfloor
    if ( (pSurface->flags & SITH_SURFACE_KILLFLOOR) != 0 )
    {
        if ( (pThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) != 0 )
        {
            goto attach;
        }

        if ( pThing->type == SITH_THING_WEAPON )
        {
            sithThing_DestroyThing(pThing);
        }

        if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
        {
            goto attach;
        }

        if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
        {
            goto attach;
        }

        sithActor_KillActor(pThing, NULL, SITH_DAMAGE_IMPACT);
    }

attach:
    pThing->attach.flags = SITH_ATTACH_SURFACE;
    rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &sithWorld_pCurrentWorld->aVertices[*pSurface->face.aVertices]);
    pThing->attach.pFace = &pSurface->face;
    pThing->attach.attachedToStructure.pSurfaceAttached = pSurface;

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;

    if ( (pSurface->flags & SITH_SURFACE_SCROLLING) != 0 && pThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector3 scrollDir;
        sithAnimate_GetSurfaceScrollingDirection(pSurface, &scrollDir);
        rdVector_Sub3Acc(&pThing->moveInfo.physics.velocity, &scrollDir);
    }

    if ( (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_ENTERED);
    }

    if ( !bNoImpactUpdate && bNoPrevAttach )
    {
        impactForce = -rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pSurface->face.normal);

        if ( pThing->type == SITH_THING_ACTOR && pThing->controlInfo.aiControl.pLocal )
        {
            sithAI_EmitEvent(pThing->controlInfo.aiControl.pLocal, SITHAI_EVENT_LAND_FLOOR, pSurface);
        }

        if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_FALLING )
            {
                pThing->moveInfo.pathMovement.mode = 0;
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDWATER);
                if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithVehicleControls_PuppetCallback) >= 0 )
                {
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                    pThing->moveStatus = SITHPLAYERMOVE_LAND;
                }
                else
                {
                    pThing->moveStatus = SITHPLAYERMOVE_RAFT_STILL;
                }

                sithFX_CreateRaftRipple(pThing, 1);
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALL);
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD);
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_RAFT_STILL;
            }
        }
        else if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            if ( pThing->moveInfo.physics.velocity.z < sithThing_jeepHitMinVelocityZ )// Note, negative value means downward movement, i.e.: falling
            {
#ifdef J3D_DEBUG
                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                {
                    char aText[256];
                    sprintf(aText, "Falling Death Attach%f.", pThing->moveInfo.physics.velocity.z);
                    sithConsole_PrintString(aText);
                }
#endif
                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_JEEP_IMPACT;
                    sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                }
            }
            else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_82 ) // SITHPLAYERMOVE_UNKNOWN_82 probably land hard
            {
                sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, 0);
                pThing->moveStatus = SITHPLAYERMOVE_JEEP_STILL;
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHARD);
            }
        }
        else
        {
            // Damage or kill falling thing
            if ( (pThing->moveStatus == SITHPLAYERMOVE_FALLING || !pThing->attach.flags) && impactForce >= 1.45f )
            {
                if ( impactForce >= 1.65f )
                {
                    if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR)
                        && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                    {
                        sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                    }
                }
                else if ( impactForce >= 1.55f )
                {
                    sithThing_DamageThing(pThing, pThing, 500.0f, SITH_DAMAGE_IMPACT);
                }
                else
                {
                    sithThing_DamageThing(pThing, pThing, 200.0f, SITH_DAMAGE_IMPACT);
                }

                if ( pThing->pSoundClass )
                {
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHURT);
                }
            }

            // Play surface land sound fx
            if ( pThing->pSoundClass )
            {
                if ( (pSurface->flags & (SITH_SURFACE_WOODECHO | SITH_SURFACE_ECHO | SITH_SURFACE_WOOD | SITH_SURFACE_SNOW | SITH_SURFACE_WEB | SITH_SURFACE_EARTH | SITH_SURFACE_SHALLOWWATER | SITH_SURFACE_WATER | SITH_SURFACE_METAL | SITH_SURFACE_AETHERIUM | SITH_SURFACE_EARTHECHO)) == 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDHARD);
                }
                else if ( (pSurface->flags & SITH_SURFACE_METAL) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDMETAL);
                }
                else if ( (pSurface->flags & SITH_SURFACE_WATER) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWATER);
                }
                else if ( (pSurface->flags & SITH_SURFACE_SHALLOWWATER) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDPUDDLE);
                }
                else if ( (pSurface->flags & SITH_SURFACE_SNOW) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDSNOW);
                }
                else if ( (pSurface->flags & SITH_SURFACE_WOOD) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWOOD);
                }
                else if ( (pSurface->flags & SITH_SURFACE_EARTH) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDEARTH);
                }
                else if ( (pSurface->flags & SITH_SURFACE_ECHO) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDHARDECHO);
                }
                else if ( (pSurface->flags & SITH_SURFACE_WOODECHO) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWOODECHO);
                }
                else if ( (pSurface->flags & SITH_SURFACE_EARTHECHO) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDEARTHECHO);
                }
                else if ( (pSurface->flags & SITH_SURFACE_AETHERIUM) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDAET);
                }
            }

            if ( (pThing->flags & SITH_TF_DYING) != 0 || (pThing->flags & SITH_TF_DESTROYED) != 0 )
            {
                return;
            }

            if ( pThing->pPuppetClass
                && pThing->moveType == SITH_MT_PHYSICS
                && pThing->moveStatus != SITHPLAYERMOVE_CRAWL_STILL )
            {
                if ( pThing->moveStatus == SITHPLAYERMOVE_FALLING || pThing->moveInfo.physics.deltaVelocity.z < -0.15f )
                {
                    float slopeDot;
                    float dirDot;

                    if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
                    {
                    }
                    else
                    {
                        // Calc surface slope
                        slopeDot = rdVector_Dot3(&pSurface->face.normal, &rdroid_zVector3);
                        if ( slopeDot < 0.8f && slopeDot > 0.69f )// 35 - 45 degrees
                        {
                            rdVector3 lvec;

                            // Slide down the slope

                            // Calculate LVec
                            rdVector3 newLVec;
                            rdVector_Copy3(&lvec, &pThing->orient.lvec);
                            lvec.z = 0.0f;
                            rdVector_Normalize3Acc(&lvec);

                        
                            rdVector_Copy3(&newLVec, &pSurface->face.normal);
                            newLVec.z = 0.0f;
                            rdVector_Normalize3Acc(&newLVec);

                            dirDot = rdVector_Dot3(&newLVec, &lvec);
                            if ( dirDot >= 0.0f )
                            {
                                pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNFORWARD;
                            }
                            else
                            {
                                rdVector_Neg3Acc(&newLVec);
                                pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNBACK;
                            }

                            rdVector_Copy3(&pThing->orient.lvec, &newLVec);

                            // lvec cross rdroid_zVector3
                            // Calculate RVec
                            rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_zVector3);
                            rdVector_Normalize3Acc(&pThing->orient.rvec);

                            rdVector_Copy3(&pThing->orient.uvec, &rdroid_zVector3);
                        }
                        else
                        {
                            if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithPlayerControls_PuppetCallback) >= 0 )
                            {
                                pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                            }

                            pThing->moveStatus = SITHPLAYERMOVE_LAND;
                            if ( (pSurface->flags & SITH_SURFACE_SHALLOWWATER) != 0 || (pSurface->flags & SITH_SURFACE_WATER) != 0 )
                            {
                                sithFX_CreateWaterRipple(pThing);
                            }
                        }
                    }

                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALL);
                    sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD);
                }

                switch ( pThing->moveStatus )
                {
                    case SITHPLAYERMOVE_JUMPFWD:
                    case SITHPLAYERMOVE_JUMPBACK:
                    case SITHPLAYERMOVE_JUMPROLLBACK:
                    case SITHPLAYERMOVE_JUMPROLLFWD:
                    case SITHPLAYERMOVE_JUMPLEFT:
                    case SITHPLAYERMOVE_JUMPRIGHT:
                    case SITHPLAYERMOVE_LEAPFWD:
                        return;

                    default:
                        if ( pThing->moveStatus == SITHPLAYERMOVE_JUMPFWD || pThing->moveStatus == SITHPLAYERMOVE_JUMPBACK )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        }

                        sithPhysics_ResetThingMovement(pThing);
                        break;
                }
            }
        }
    }
}

void J3DAPI sithThing_AttachThingToClimbSurface(SithThing* pThing, SithSurface* pSurface, int bClimbing)
{
    int bNoPrevAttach = 1;
    rdVector3 lvec;
    rdVector3 uvec;

#line 2940
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithSurface_ValidateSurfacePointer(pSurface));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    uvec.x = 0.0f;
    uvec.y = 0.0f;
    uvec.z = 1.0f;

    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 && pThing->attach.attachedToStructure.pSurfaceAttached == pSurface )
        {
            return;
        }

        bNoPrevAttach = 0;
        sithThing_DetachThing(pThing);
    }

    pThing->attach.flags = SITH_ATTACH_CLIMBSURFACE;

    rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &sithWorld_pCurrentWorld->aVertices[*pSurface->face.aVertices]);
    pThing->attach.pFace = &pSurface->face;
    pThing->attach.attachedToStructure.pSurfaceAttached = pSurface;

    // Calculate new LVec
    lvec.x = -pSurface->face.normal.x;
    lvec.y = -pSurface->face.normal.y;
    lvec.z = -pSurface->face.normal.z;
    rdVector_Copy3(&pThing->orient.lvec, &lvec);

    // Calculate new RVec
    rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &uvec);
    rdVector_Normalize3Acc(&pThing->orient.rvec);

    // Calculate new UVec
    rdVector_Cross3(&pThing->orient.uvec, &pThing->orient.rvec, &pThing->orient.lvec);

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED; // 0x100 - SITH_PF_ALIGNED

    if ( (pSurface->flags & SITH_SURFACE_SCROLLING) != 0 && pThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector3 scrollDir;
        sithAnimate_GetSurfaceScrollingDirection(pSurface, &scrollDir);
        rdVector_Sub3Acc(&pThing->moveInfo.physics.velocity, &scrollDir);
    }

    if ( (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_ENTERED);
    }
}

void J3DAPI sithThing_AttachThingToThingFace(SithThing* pThing, SithThing* pAttachThing, rdFace* pFace, const rdVector3* aVertices, int bNoImpactUpdate)
{
    int bNewAttach = 1;
    int bReservedVar; // the debug frame holds one more 4-byte local that no statement uses
    rdVector3 wNormal;
    rdVector3 dpos;
    float impactForce;

#line 3012
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pAttachThing));
    SITH_ASSERTREL(pFace && aVertices);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);

    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0
            && pThing->attach.attachedToStructure.pThingAttached == pAttachThing
            && pThing->attach.pFace == pFace )
        {
            return;
        }

        bNewAttach = 0;
        sithThing_DetachThing(pThing);
    }

#line 3030
    SITH_ASSERTREL(!pThing->pNextAttachedThing);
    SITH_ASSERTREL(!pThing->pPrevAttachedThing);
    SITH_ASSERTREL(!pThing->attach.flags);
    SITH_ASSERTREL(!pThing->attach.attachedToStructure.pThingAttached);

    pThing->attach.flags = SITH_ATTACH_THINGFACE;
    rdVector_Copy3(&pThing->attach.attachedFaceFirstVert, &aVertices[*pFace->aVertices]);

    pThing->attach.pFace = pFace;
    pThing->attach.attachedToStructure.pThingAttached = pAttachThing;
    sithThing_AddThingToAttachedThings(pThing, pAttachThing);

    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;

    if ( pAttachThing->moveType == SITH_MT_PHYSICS )
    {
        rdVector_Sub3Acc(&pThing->moveInfo.physics.velocity, &pAttachThing->moveInfo.physics.velocity);
    }
    else if ( pAttachThing->moveType == SITH_MT_PATH )
    {
        rdVector_ScaleAdd3Acc(&pThing->moveInfo.physics.velocity, &pAttachThing->moveInfo.pathMovement.vecDeltaPos, -pAttachThing->moveInfo.pathMovement.moveVel);

    }

    
    rdVector_Sub3(&dpos, &pThing->pos, &pAttachThing->pos);
    rdMatrix_TransformVectorOrtho34(&pThing->attach.posOffset, &dpos, &pAttachThing->orient);

    if ( (pAttachThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_ThingSendMessage(pAttachThing, pThing, SITHCOG_MSG_ENTERED);
    }

    if ( bNewAttach && !bNoImpactUpdate )
    {
        rdMatrix_TransformVector34(&wNormal, &pFace->normal, &pAttachThing->orient);

        impactForce = -rdVector_Dot3(&pThing->moveInfo.physics.velocity, &wNormal);
        if ( (pThing->moveStatus == SITHPLAYERMOVE_FALLING || !pThing->attach.flags) && impactForce >= 1.45f )
        {
            if ( impactForce >= 1.65f )
            {
                if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                {
                    sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                }
            }
            else if ( impactForce >= 1.55f )
            {
                sithThing_DamageThing(pThing, pThing, 500.0f, SITH_DAMAGE_IMPACT);
            }
            else
            {
                sithThing_DamageThing(pThing, pThing, 200.0f, SITH_DAMAGE_IMPACT);
            }

            if ( pThing->pSoundClass )
            {
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHURT);
            }
        }

        if ( pThing->pSoundClass )
        {
            if ( (pAttachThing->flags & SITH_TF_METAL) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDMETAL);
            }
            else if ( (pAttachThing->flags & SITH_TF_EARTH) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDEARTH);
            }
            else if ( (pAttachThing->flags & SITH_TF_SNOW) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDSNOW);
            }
            else if ( (pAttachThing->flags & SITH_TF_WOOD) != 0 )
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDWOOD);
            }
            else
            {
                sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LANDHARD);
            }
        }

        if ( pThing->pPuppetClass && pThing->moveType == SITH_MT_PHYSICS && pThing->moveStatus != SITHPLAYERMOVE_CRAWL_STILL )
        {
            if ( pThing->moveStatus == SITHPLAYERMOVE_FALLING || pThing->moveInfo.physics.deltaVelocity.z < -0.15f )
            {
                if ( sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, sithPlayerControls_PuppetCallback) >= 0 )
                {
                    pThing->thingInfo.actorInfo.bControlsDisabled = 1;
                }

                pThing->moveStatus = SITHPLAYERMOVE_LAND;
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALL);
                sithPuppet_ClearMode(pThing, SITHPUPPETSUBMODE_FALLFORWARD);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_JUMPFWD )
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }

            sithPhysics_ResetThingMovement(pThing);
        }
    }
}

void J3DAPI sithThing_AddThingToAttachedThings(SithThing* pThing, SithThing* pAttachThing)
{
    pThing->pNextAttachedThing = pAttachThing->pAttachedThing;
    if ( pThing->pNextAttachedThing )
    {
#line 3148
        SITH_ASSERTREL(pThing->pNextAttachedThing->pPrevAttachedThing == NULL);
        pThing->pNextAttachedThing->pPrevAttachedThing = pThing;
    }

    pThing->pPrevAttachedThing   = NULL;
    pAttachThing->pAttachedThing = pThing;
}

void J3DAPI sithThing_AttachThingToThing(SithThing* pThing, SithThing* pAttachThing)
{
    rdVector3 dpos;

#line 3174
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pAttachThing));

    if ( pThing->attach.flags )
    {
        if ( (pThing->attach.flags & SITH_ATTACH_THING) != 0 && pThing->attach.attachedToStructure.pThingAttached == pAttachThing )
        {
            return;
        }

        sithThing_DetachThing(pThing);
    }

#line 3188
    SITH_ASSERTREL(!pThing->pNextAttachedThing);
    SITH_ASSERTREL(!pThing->pPrevAttachedThing);
    SITH_ASSERTREL(!pThing->attach.flags);
    SITH_ASSERTREL(!pThing->attach.attachedToStructure.pThingAttached);

    pThing->attach.flags = SITH_ATTACH_THING;
    pThing->attach.attachedToStructure.pThingAttached = pAttachThing;
    sithThing_AddThingToAttachedThings(pThing, pAttachThing);

    
    rdVector_Sub3(&dpos, &pThing->pos, &pAttachThing->pos);
    rdMatrix_TransformVectorOrtho34(&pThing->attach.posOffset, &dpos, &pAttachThing->orient);

    if ( (pAttachThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & (SITH_TF_DISABLED | SITH_TF_REMOTE)) == 0 )
    {
        sithCog_ThingSendMessage(pAttachThing, pThing, SITHCOG_MSG_ENTERED);
    }
}

void J3DAPI sithThing_AttachThingToThingMesh(SithThing* pThing, SithThing* pAttachThing, int meshNum)
{
    SithActorInfo* pActorInfo;
    SithSpriteInfo* pSpriteInfo;

#line 3221
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(pAttachThing);

    switch ( pThing->type )
    {
        case SITH_THING_SPRITE:
            pSpriteInfo = &pThing->thingInfo.spriteInfo;
            pSpriteInfo->pThingMeshAttached = pAttachThing;
            pSpriteInfo->attachMeshNum      = meshNum;
            break;

        case SITH_THING_EXPLOSION:
            if ( pThing->thingInfo.explosionInfo.pSprite )
            {
                pSpriteInfo = &pThing->thingInfo.explosionInfo.pSprite->thingInfo.spriteInfo;
                pSpriteInfo->pThingMeshAttached = pAttachThing;
                pSpriteInfo->attachMeshNum      = meshNum;
            }
            break;

        case SITH_THING_ACTOR:
            pActorInfo = &pThing->thingInfo.actorInfo;
            pActorInfo->pThingMeshAttached = pAttachThing;
            pActorInfo->attachMeshNum      = meshNum;
            break;

        default:
#line 3248
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Trying to attach a invalid thing type to a thing mesh, not allowed!\n", 0, 0, 0, 0);
            break;
    }
}

void J3DAPI sithThing_DetachThing(SithThing* pThing)
{
    if ( sithWorld_pCurrentWorld )
    {
#line 3272
        SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    }
    if ( (pThing->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 )
    {
        SithThing* pAttached = pThing->attach.attachedToStructure.pThingAttached;
        SITH_ASSERTREL(pAttached);

        if ( pThing->moveType == SITH_MT_PHYSICS )
        {
            if ( pAttached->moveType == SITH_MT_PHYSICS )
            {
                rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &pAttached->moveInfo.physics.velocity);
            }

            else if ( pAttached->moveType == SITH_MT_PATH )
            {
                rdVector_ScaleAdd3Acc(&pThing->moveInfo.physics.velocity, &pAttached->moveInfo.pathMovement.vecDeltaPos, pAttached->moveInfo.pathMovement.moveVel);

            }
        }

        if ( pThing->type == SITH_THING_PLAYER
            && (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0
            && (pThing->attach.pFace->flags & RD_FF_3DO_WHIP_AIM) != 0
            && (sithWhip_GetWhipSwingThing() || sithWhip_GetWhipClimbThing()) )
        {
            sithWhip_Reset();
            sithWeapon_SendMessageAim(pThing, /*bAim*/0);
        }

        if ( (pAttached->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
        {
            sithCog_ThingSendMessage(pAttached, pThing, SITHCOG_MSG_EXITED);
        }

        if ( pThing->pPrevAttachedThing )
        {
            pThing->pPrevAttachedThing->pNextAttachedThing = pThing->pNextAttachedThing;
            if ( pThing->pNextAttachedThing )
            {
                pThing->pNextAttachedThing->pPrevAttachedThing = pThing->pPrevAttachedThing;
            }
        }
        else
        {
            pAttached->pAttachedThing = pThing->pNextAttachedThing;
            if ( pThing->pNextAttachedThing )
            {
                pThing->pNextAttachedThing->pPrevAttachedThing = NULL;
            }
        }

        pThing->pPrevAttachedThing = NULL;
        pThing->pNextAttachedThing = NULL;
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 || (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        SithSurface* pSurf = pThing->attach.attachedToStructure.pSurfaceAttached;
#line 3333
        SITH_ASSERTREL(pSurf);

        if ( (pSurf->flags & SITH_SURFACE_SCROLLING) != 0 && pThing->moveType == SITH_MT_PHYSICS )
        {
            rdVector3 scrollDir;
            sithAnimate_GetSurfaceScrollingDirection(pSurf, &scrollDir);
            rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &scrollDir);
        }

        if ( pThing->type == SITH_THING_PLAYER && (pSurf->flags & SITH_SURFACE_WHIPAIM) != 0 && (sithWhip_GetWhipSwingThing() || sithWhip_GetWhipClimbThing()) )
        {
            sithWhip_Reset();
            sithWeapon_SendMessageAim(pThing, /*bAim*/0);
        }

        if ( (pSurf->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
        {
            sithCog_SurfaceSendMessage(pSurf, pThing, SITHCOG_MSG_EXITED);
        }
    }

    memset(&pThing->attach, 0, sizeof(pThing->attach));
}

void J3DAPI sithThing_DetachAttachedThings(SithThing* pThing)
{
    SithThing* pNextAttach;
    SithThing* attach;

#line 3380
    SITH_ASSERTREL(pThing);

    for ( attach = pThing->pAttachedThing; attach; attach = pNextAttach )
    {
        pNextAttach = attach->pNextAttachedThing;
        sithThing_DetachThing(attach);
    }
}

/* Linker-discarded: retail __LINE__ gap 3380..3444 (sithThing_DetachAttachedThings -> sithThing_PurgeGarbageThings), where the
   the original defined it; Jedi Knight's IsAttachFlagsAnd6 (0x004CE560, flags & 6) sits at the same spot. Body from the
   debug build (0x00444EB8, assert line 3409). */
int J3DAPI sithThing_IsThingAttachedToThing(SithThing* pThing)
{
#line 3409
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));

    if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 || (pThing->attach.flags & SITH_ATTACH_THING) != 0 )
    {
        return 1;
    }

    return 0;
}

void J3DAPI sithThing_PurgeGarbageThings(SithWorld* pWorld)
{
    size_t count = 0;
    size_t curthingCount;
    SithThing* pThing;

#line 3444
    SITH_ASSERTREL(pWorld && pWorld->aThings);

    curthingCount = 0;
    for ( pThing = pWorld->aThings; curthingCount < pWorld->numThings; ++curthingCount, ++pThing )
    {
        if ( (pThing->flags & SITH_TF_DESTROYED) != 0 || (pThing->type == SITH_THING_DEBRIS || pThing->type == SITH_THING_PARTICLE) && pThing->msecLifeLeft )
        {
#line 3455
            stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Garbage purging thing %d.\n", pThing->idx, 0, 0, 0);
            sithThing_RemoveThing(pWorld, pThing);
        }

        if ( count++ > SITHTHING_MAXTHINGSTOPURGE )
        {
            break;
        }
    }
}

int J3DAPI sithThing_ReadStaticThingsListText(SithWorld* pWorld, int bSkip)
{
#line 3486
    SITH_ASSERTREL(pWorld != NULL);
    sithThing_curSignature = 1;

    if ( !bSkip )
    {
#line 3493
        SITH_ASSERTREL(pWorld->aThings == NULL);
        SITH_ASSERTREL(pWorld->numThings == 0);
    }
    else
    {
        sithThing_FreeWorldThings(pWorld);
        // TODO: Hmm I thing there's bug here, should return, no?
    }

    stdConffile_ReadArgs();
    if ( !streq(stdConffile_entry.aArgs[0].argValue, "world") || !streq(stdConffile_entry.aArgs[1].argValue, "things") )
    {
#line 3506
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading thing template list line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    // Alloc things
    {
    size_t tableSize = atoi(stdConffile_entry.aArgs[2].argValue); // TODO: Check for negative num
#line 3514
    SITH_ASSERT(tableSize >= 0); // debug build: unsigned compare (jae), so the local is a size_t
    if ( sithThing_AllocWorldThings(pWorld, tableSize) )
    {
#line 3519
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Allocation failure while allocating things array.\n", 0, 0, 0, 0);
        return 1;
    }

    // Read things
    while ( stdConffile_ReadArgs() )
    {
        if ( streq(stdConffile_entry.aArgs[0].argValue, "end") )
        {
            break;
        }

        if ( sithThing_ParseThingPlacement(pWorld) )
        {
#line 3531
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error parsing thing placement, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        }
    }

    sithThing_LoadPostProcess(pWorld);
    return 0;
}
}


int J3DAPI sithThing_WriteStaticThingsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    return sithThing_WriteThingsListBinary(fh, pWorld, pWorld->numThings, pWorld->aThings);
}

int J3DAPI sithThing_ReadStaticThingsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t nCurThings = pWorld->numThings; // Note,sithThing_AllocWorldThings doesn't reset numThings to 0 but cache it anyway

    int error;
    if ( sithThing_AllocWorldThings(pWorld, nCurThings) )
    {
        return 1;
    }

    error = sithThing_ReadThingsListBinary(fh, pWorld, nCurThings, pWorld->aThings, NULL);
    if ( error )
    {
        return error;
    }

    // Init things
    {
        size_t i = 0;  // block scope: the debug build's frame slot order follows it
        for ( ; i < nCurThings; ++i )
    {
        SithThing* pThing = &pWorld->aThings[i];
        pThing->guid      = pThing->idx;
        pThing->signature = sithThing_curSignature++;
        if ( pThing->perfLevel > sithGetPerformanceLevel() )
        {
            sithThing_FreeThing(pWorld, pThing);
        }
    }
    }

    sithThing_LoadPostProcess(pWorld);
    return 0;
}

// Binary layout of the things list. Every offset below is verified against retail's
// fill code in sithThing_WriteThingsListBinary (0x00437ACE-0x00438243) and against
// sithThing_ReadThingsListBinary's reads (0x00438A85-0x004392D9).

typedef struct sSithThingBinaryHeader // 0x2C, written after the thing records
{
    size_t numPhysicsThings;
    size_t numPathThings;
    size_t numPathFrames;
    size_t numActorThings;
    size_t numWeaponThings;
    size_t numExplosionThings;
    size_t numItemThings;
    size_t numParticleThings;
    size_t numHintThings;
    size_t numAIThings;
    size_t numAIFrames;
} SithThingBinaryHeader;

typedef struct sSithThingBinaryRecord // 0x238
{
    char aTemplateName[64];       // 0x000
    char aName[64];               // 0x040
    rdVector3 pos;                // 0x080
    rdVector3 pyr;                // 0x08C
    int unknown38;                // 0x098 Neither written by the writer nor read by the reader
    int sectorNum;                // 0x09C
    SithThingType type;           // 0x0A0
    SithThingFlag flags;          // 0x0A4
    SithThingMoveType moveType;   // 0x0A8
    SithControlType controlType;  // 0x0AC
    rdVector4 color;              // 0x0B0
    rdVector4 emitColor;          // 0x0C0
    uint32_t msecLifeLeft;        // 0x0D0
    rdThingType renderType;       // 0x0D4
    char aRenderName[64];         // 0x0D8 model3d, sprite or particle name
    char aPuppetName[64];         // 0x118
    char aSoundClassName[64];     // 0x158
    char aCreateThingName[64];    // 0x198
    char aCogName[64];            // 0x1D8
    SithCollide collide;          // 0x218
    int perfLevel;                // 0x234
} SithThingBinaryRecord;

typedef struct sSithPhysicsBinaryRecord // 0x40
{
    SithPhysicsFlags flags;       // 0x00
    float mass;                   // 0x04
    float height;                 // 0x08
    float airDrag;                // 0x0C
    float surfDrag;               // 0x10
    float staticDrag;             // 0x14
    rdVector3 angularVelocity;    // 0x18
    rdVector3 velocity;           // 0x24
    float maxRotationVelocity;    // 0x30
    float maxVelocity;            // 0x34
    float orientSpeed;            // 0x38
    float buoyancy;               // 0x3C
} SithPhysicsBinaryRecord;

typedef struct sSithActorBinaryRecord // 0x11C
{
    SithActorFlag flags;            // 0x000
    char aWeaponTemplateName[64];   // 0x004
    float health;                   // 0x044
    float maxHealth;                // 0x048
    float maxThrust;                // 0x04C
    float maxRotVelocity;           // 0x050
    float maxHeadVelocity;          // 0x054
    float maxHeadYaw;               // 0x058
    float jumpSpeed;                // 0x05C
    rdVector3 eyeOffset;            // 0x060
    float minHeadPitch;             // 0x06C
    float maxHeadPitch;             // 0x070
    rdVector3 fireOffset;           // 0x074
    rdVector3 lightOffset;          // 0x080
    rdVector4 headLightIntensity;   // 0x08C
    VGradiantColor voiceColor;      // 0x09C
    char aExplodeTemplateName[64];  // 0x0DC
} SithActorBinaryRecord;

typedef struct sSithWeaponBinaryRecord // 0x5C
{
    SithWeaponFlag flags;             // 0x00
    char aExplosionTemplateName[64];  // 0x04
    float damage;                     // 0x44
    float minDamage;                  // 0x48
    float rate;                       // 0x4C
    SithDamageType damageType;        // 0x50
    float range;                      // 0x54
    float force;                      // 0x58
} SithWeaponBinaryRecord;

typedef struct sSithExplosionBinaryRecord // 0x480
{
    SithExplosionFlag flags;       // 0x000
    float damage;                  // 0x004
    SithDamageType damageType;     // 0x008
    float range;                   // 0x00C
    float force;                   // 0x010
    uint32_t msecBlastTime;        // 0x014
    uint32_t msecBabyTime;         // 0x018
    uint32_t msecExpandTime;       // 0x01C
    uint32_t msecFadeTime;         // 0x020
    float maxLight;                // 0x024
    char aDebrisNames[16][64];     // 0x028
    rdVector3 spriteStart;         // 0x428
    rdVector3 spriteEnd;           // 0x434
    char aSpriteTemplateName[64];  // 0x440
} SithExplosionBinaryRecord;

typedef struct sSithItemBinaryRecord // 0x8
{
    SithItemFlag flags;            // 0x0
    float secRespawnInterval;      // 0x4
} SithItemBinaryRecord;

typedef struct sSithHintBinaryRecord // 0x4
{
    float userval;                 // 0x0
} SithHintBinaryRecord;

typedef struct sSithParticleBinaryRecord // 0x64
{
    SithParticleFlag flags;        // 0x00
    float growthSpeed;             // 0x04
    float minRadius;               // 0x08
    float maxRadius;               // 0x0C
    float size;                    // 0x10
    float timeoutRate;             // 0x14
    size_t numParticles;           // 0x18
    float pitchRange;              // 0x1C
    float yawRange;                // 0x20
    char aMaterialName[64];        // 0x24
} SithParticleBinaryRecord;

typedef struct sSithAIBinaryRecord // 0x44
{
    char aClassName[64];           // 0x00
    size_t numFrames;              // 0x40
} SithAIBinaryRecord;

int J3DAPI sithThing_WriteThingsListBinary(tFileHandle fh, const SithWorld* pWorld, size_t numThings, const SithThing* aThings)
{
    rdVector3 pyr;
    int error = 1;
    size_t nWritten;
    size_t i;
    SithThingBinaryHeader header;
    SithThingBinaryRecord* aRecords = NULL;
    SithPhysicsBinaryRecord* aPhysics = NULL;
    size_t* aPathFrameCounts = NULL;
    SithPathFrame* aPathFrames = NULL;
    SithActorBinaryRecord* aActors = NULL;
    SithWeaponBinaryRecord* aWeapons = NULL;
    SithExplosionBinaryRecord* aExplosions = NULL;
    SithItemBinaryRecord* aItems = NULL;
    SithHintBinaryRecord* aHints = NULL;
    SithParticleBinaryRecord* aParticles = NULL;
    SithAIBinaryRecord* aAIs = NULL;
    rdVector3* aAIFrames = NULL;
    size_t sizeRecords;
    size_t sizePhysics;
    size_t sizePathFrameCounts;
    size_t sizePathFrames;
    size_t sizeActors;
    size_t sizeWeapons;
    size_t sizeExplosions;
    size_t sizeItems;
    size_t sizeHints;
    size_t sizeParticles;
    size_t sizeAIs;
    size_t sizeAIFrames;
    const SithThing* pThing;
    SithThingBinaryRecord* pRec;
    SithPhysicsBinaryRecord* pPhysics;
    size_t* pPathFrameCount;
    SithPathFrame* pPathFrame;
    SithActorBinaryRecord* pActor;
    SithWeaponBinaryRecord* pWeapon;
    SithExplosionBinaryRecord* pExplosion;
    SithItemBinaryRecord* pItem;
    SithHintBinaryRecord* pHint;
    SithParticleBinaryRecord* pParticle;
    SithAIBinaryRecord* pAI;
    rdVector3* pAIFrame;
    size_t debrisNum;

    sizeRecords = numThings * sizeof(SithThingBinaryRecord);
#line 3745
    aRecords = (SithThingBinaryRecord*)STDMALLOC(sizeRecords);
    if ( !aRecords )
    {
        goto error;
    }

    memset(aRecords, 0, sizeRecords);
    memset(&header, 0, sizeof(header));

    for ( i = 0; i < numThings; ++i )
    {
        pThing = &aThings[i];

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                ++header.numPhysicsThings;
                break;

            case SITH_MT_PATH:
                ++header.numPathThings;
                header.numPathFrames += pThing->moveInfo.pathMovement.numFrames;
                break;

            default:
                break;
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                ++header.numActorThings;
                break;

            case SITH_THING_WEAPON:
                ++header.numWeaponThings;
                break;

            case SITH_THING_EXPLOSION:
                ++header.numExplosionThings;
                break;

            case SITH_THING_ITEM:
                ++header.numItemThings;
                break;

            case SITH_THING_HINT:
                ++header.numHintThings;
                break;

            case SITH_THING_PARTICLE:
                ++header.numParticleThings;
                break;

            default:
                break;
        }

        switch ( pThing->controlType )
        {
            case SITH_CT_AI:
                ++header.numAIThings;
                if ( pThing->controlInfo.aiControl.pLocal && pThing->controlInfo.aiControl.pLocal->numFrames )
                {
                    header.numAIFrames += pThing->controlInfo.aiControl.pLocal->numFrames;
                }

                break;

            default:
                break;
        }
    }

    sizePhysics = header.numPhysicsThings * sizeof(SithPhysicsBinaryRecord);
#line 3804
    aPhysics = (SithPhysicsBinaryRecord*)STDMALLOC(sizePhysics);
    if ( !aPhysics )
    {
        goto error;
    }

    memset(aPhysics, 0, sizePhysics);
    pPhysics = aPhysics;

    sizePathFrameCounts = header.numPathThings * sizeof(size_t);
#line 3810
    aPathFrameCounts = (size_t*)STDMALLOC(sizePathFrameCounts);
    if ( !aPathFrameCounts )
    {
        goto error;
    }

    memset(aPathFrameCounts, 0, sizePathFrameCounts);
    pPathFrameCount = aPathFrameCounts;

    sizePathFrames = header.numPathFrames * sizeof(SithPathFrame);
#line 3816
    aPathFrames = (SithPathFrame*)STDMALLOC(sizePathFrames);
    if ( !aPathFrames )
    {
        goto error;
    }

    memset(aPathFrames, 0, sizePathFrames);
    pPathFrame = aPathFrames;

    sizeActors = header.numActorThings * sizeof(SithActorBinaryRecord);
#line 3822
    aActors = (SithActorBinaryRecord*)STDMALLOC(sizeActors);
    if ( !aActors )
    {
        goto error;
    }

    memset(aActors, 0, sizeActors);
    pActor = aActors;

    sizeWeapons = header.numWeaponThings * sizeof(SithWeaponBinaryRecord);
#line 3828
    aWeapons = (SithWeaponBinaryRecord*)STDMALLOC(sizeWeapons);
    if ( !aWeapons )
    {
        goto error;
    }

    memset(aWeapons, 0, sizeWeapons);
    pWeapon = aWeapons;

    sizeExplosions = header.numExplosionThings * sizeof(SithExplosionBinaryRecord);
#line 3834
    aExplosions = (SithExplosionBinaryRecord*)STDMALLOC(sizeExplosions);
    if ( !aExplosions )
    {
        goto error;
    }

    memset(aExplosions, 0, sizeExplosions);
    pExplosion = aExplosions;

    sizeItems = header.numItemThings * sizeof(SithItemBinaryRecord);
#line 3840
    aItems = (SithItemBinaryRecord*)STDMALLOC(sizeItems);
    if ( !aItems )
    {
        goto error;
    }

    memset(aItems, 0, sizeItems);
    pItem = aItems;

    sizeHints = header.numHintThings * sizeof(SithHintBinaryRecord);
#line 3846
    aHints = (SithHintBinaryRecord*)STDMALLOC(sizeHints);
    if ( !aHints )
    {
        goto error;
    }

    memset(aHints, 0, sizeHints);
    pHint = aHints;

    sizeParticles = header.numParticleThings * sizeof(SithParticleBinaryRecord);
#line 3852
    aParticles = (SithParticleBinaryRecord*)STDMALLOC(sizeParticles);
    if ( !aParticles )
    {
        goto error;
    }

    memset(aParticles, 0, sizeParticles);
    pParticle = aParticles;

    sizeAIs = header.numAIThings * sizeof(SithAIBinaryRecord);
#line 3858
    aAIs = (SithAIBinaryRecord*)STDMALLOC(sizeAIs);
    if ( !aAIs )
    {
        goto error;
    }

    memset(aAIs, 0, sizeAIs);
    pAI = aAIs;

    sizeAIFrames = header.numAIFrames * sizeof(rdVector3);
#line 3864
    aAIFrames = (rdVector3*)STDMALLOC(sizeAIFrames);
    if ( !aAIFrames )
    {
        goto error;
    }

    memset(aAIFrames, 0, sizeAIFrames);
    pAIFrame = aAIFrames;

    for ( i = 0; i < numThings; ++i )
    {
        pThing = &aThings[i];
        pRec = &aRecords[i];

        rdMatrix_ExtractAngles34(&pThing->orient, &pyr);

        if ( pThing->pTemplate )
        {
            strncpy(pRec->aTemplateName, pThing->pTemplate->aName, STD_ARRAYLEN(pRec->aTemplateName) - 1);
            pRec->aTemplateName[STD_ARRAYLEN(pRec->aTemplateName) - 1] = 0;
        }

        strncpy(pRec->aName, pThing->aName, STD_ARRAYLEN(pRec->aName) - 1);
        pRec->aName[STD_ARRAYLEN(pRec->aName) - 1] = 0;

        rdVector_Copy3(&pRec->pos, &pThing->pos);
        rdVector_Copy3(&pRec->pyr, &pyr);

        pRec->sectorNum = sithSector_GetSectorIndexEx(pWorld, pThing->pInSector);

        pRec->flags = pThing->flags;
        if ( (pRec->flags & SITH_TF_SEEN) != 0 )
        {
            pRec->flags &= ~SITH_TF_SEEN;
        }

        pRec->type        = pThing->type;
        pRec->moveType    = pThing->moveType;
        pRec->controlType = pThing->controlType;

        rdVector_Copy4(&pRec->color, &pThing->light.color);
        rdVector_Copy4(&pRec->emitColor, &pThing->light.emitColor);

        pRec->msecLifeLeft = pThing->msecLifeLeft;
        pRec->collide      = pThing->collide;
        pRec->renderType   = pThing->renderData.type;
        pRec->perfLevel    = pThing->perfLevel;

        switch ( pThing->renderData.type )
        {
            case RD_THING_MODEL3:
                if ( !pThing->pTemplate || pThing->renderData.data.pModel3 != pThing->pTemplate->renderData.data.pModel3 )
                {
                    strncpy(pRec->aRenderName, pThing->renderData.data.pModel3->aName, STD_ARRAYLEN(pRec->aRenderName) - 1);
                    pRec->aRenderName[STD_ARRAYLEN(pRec->aRenderName) - 1] = 0;
                }
                break;

            case RD_THING_SPRITE3:
                strncpy(pRec->aRenderName, pThing->renderData.data.pSprite3->aName, STD_ARRAYLEN(pRec->aRenderName) - 1);
                pRec->aRenderName[STD_ARRAYLEN(pRec->aRenderName) - 1] = 0;
                break;

            case RD_THING_PARTICLE:
                strncpy(pRec->aRenderName, pThing->renderData.data.pParticle->aName, STD_ARRAYLEN(pRec->aRenderName) - 1);
                pRec->aRenderName[STD_ARRAYLEN(pRec->aRenderName) - 1] = 0;
                break;

            default:
                break;
        }

        if ( pThing->pPuppetClass && (!pThing->pTemplate || pThing->pPuppetClass != pThing->pTemplate->pPuppetClass) )
        {
            strncpy(pRec->aPuppetName, pThing->pPuppetClass->aName, STD_ARRAYLEN(pRec->aPuppetName) - 1);
            pRec->aPuppetName[STD_ARRAYLEN(pRec->aPuppetName) - 1] = 0;
        }

        if ( pThing->pSoundClass )
        {
            strncpy(pRec->aSoundClassName, pThing->pSoundClass->aName, STD_ARRAYLEN(pRec->aSoundClassName) - 1);
            pRec->aSoundClassName[STD_ARRAYLEN(pRec->aSoundClassName) - 1] = 0;
        }

        if ( pThing->pCreateThingTemplate )
        {
            strncpy(pRec->aCreateThingName, pThing->pCreateThingTemplate->aName, STD_ARRAYLEN(pRec->aCreateThingName) - 1);
            pRec->aCreateThingName[STD_ARRAYLEN(pRec->aCreateThingName) - 1] = 0;
        }

        if ( pThing->pCog && (!pThing->pTemplate || pThing->pCog != pThing->pTemplate->pCog) )
        {
            strncpy(pRec->aCogName, pThing->pCog->pScript->aName, STD_ARRAYLEN(pRec->aCogName) - 1);
            pRec->aCogName[STD_ARRAYLEN(pRec->aCogName) - 1] = 0;
        }

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                pPhysics->flags      = pThing->moveInfo.physics.flags;
                pPhysics->mass       = pThing->moveInfo.physics.mass;
                pPhysics->height     = pThing->moveInfo.physics.height;
                pPhysics->airDrag    = pThing->moveInfo.physics.airDrag;
                pPhysics->surfDrag   = pThing->moveInfo.physics.surfDrag;
                pPhysics->staticDrag = pThing->moveInfo.physics.staticDrag;
                rdVector_Copy3(&pPhysics->angularVelocity, &pThing->moveInfo.physics.angularVelocity);
                rdVector_Copy3(&pPhysics->velocity, &pThing->moveInfo.physics.velocity);
                pPhysics->maxRotationVelocity = pThing->moveInfo.physics.maxRotationVelocity;
                pPhysics->maxVelocity         = pThing->moveInfo.physics.maxVelocity;
                pPhysics->orientSpeed         = pThing->moveInfo.physics.orientSpeed;
                pPhysics->buoyancy            = pThing->moveInfo.physics.buoyancy;
                ++pPhysics;
                break;

            case SITH_MT_PATH:
                *pPathFrameCount = pThing->moveInfo.pathMovement.numFrames;
                if ( *pPathFrameCount )
                {
                    memcpy(pPathFrame, pThing->moveInfo.pathMovement.aFrames, sizeof(SithPathFrame) * *pPathFrameCount);
                    pPathFrame += *pPathFrameCount;
                }

                ++pPathFrameCount;
                break;

            default:
                break;
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                pActor->flags     = pThing->thingInfo.actorInfo.flags;
                pActor->health    = pThing->thingInfo.actorInfo.health;
                pActor->maxHealth = pThing->thingInfo.actorInfo.maxHealth;
                if ( pThing->thingInfo.actorInfo.pWeaponTemplate )
                {
                    strncpy(pActor->aWeaponTemplateName, pThing->thingInfo.actorInfo.pWeaponTemplate->aName, STD_ARRAYLEN(pActor->aWeaponTemplateName) - 1);
                    pActor->aWeaponTemplateName[STD_ARRAYLEN(pActor->aWeaponTemplateName) - 1] = 0;
                }

                pActor->maxThrust       = pThing->thingInfo.actorInfo.maxThrust;
                pActor->maxRotVelocity  = pThing->thingInfo.actorInfo.maxRotVelocity;
                pActor->maxHeadVelocity = pThing->thingInfo.actorInfo.maxHeadVelocity;
                pActor->maxHeadYaw      = pThing->thingInfo.actorInfo.maxHeadYaw;
                pActor->jumpSpeed       = pThing->thingInfo.actorInfo.jumpSpeed;
                rdVector_Copy3(&pActor->eyeOffset, &pThing->thingInfo.actorInfo.eyeOffset);
                pActor->minHeadPitch = pThing->thingInfo.actorInfo.minHeadPitch;
                pActor->maxHeadPitch = pThing->thingInfo.actorInfo.maxHeadPitch;
                rdVector_Copy3(&pActor->fireOffset, &pThing->thingInfo.actorInfo.fireOffset);
                rdVector_Copy3(&pActor->lightOffset, &pThing->thingInfo.actorInfo.lightOffset);
                rdVector_Copy4(&pActor->headLightIntensity, &pThing->thingInfo.actorInfo.headLightIntensity);
                memcpy(&pActor->voiceColor, &pThing->thingInfo.actorInfo.voiceInfo.voiceColor, sizeof(VGradiantColor));
                if ( pThing->thingInfo.actorInfo.pExplodeTemplate )
                {
                    strncpy(pActor->aExplodeTemplateName, pThing->thingInfo.actorInfo.pExplodeTemplate->aName, STD_ARRAYLEN(pActor->aExplodeTemplateName) - 1);
                    pActor->aExplodeTemplateName[STD_ARRAYLEN(pActor->aExplodeTemplateName) - 1] = 0;
                }

                ++pActor;
                break;

            case SITH_THING_WEAPON:
                pWeapon->flags = pThing->thingInfo.weaponInfo.flags;
                if ( pThing->thingInfo.weaponInfo.pExplosionTemplate
                    && (!pThing->pTemplate || pThing->thingInfo.weaponInfo.pExplosionTemplate != pThing->pTemplate->thingInfo.weaponInfo.pExplosionTemplate) )
                {
                    strncpy(pWeapon->aExplosionTemplateName, pThing->thingInfo.weaponInfo.pExplosionTemplate->aName, STD_ARRAYLEN(pWeapon->aExplosionTemplateName) - 1);
                    pWeapon->aExplosionTemplateName[STD_ARRAYLEN(pWeapon->aExplosionTemplateName) - 1] = 0;
                }

                pWeapon->damage     = pThing->thingInfo.weaponInfo.damage;
                pWeapon->minDamage  = pThing->thingInfo.weaponInfo.minDamage;
                pWeapon->rate       = pThing->thingInfo.weaponInfo.rate;
                pWeapon->damageType = pThing->thingInfo.weaponInfo.damageType;
                pWeapon->range      = pThing->thingInfo.weaponInfo.range;
                pWeapon->force      = pThing->thingInfo.weaponInfo.force;
                ++pWeapon;
                break;

            case SITH_THING_EXPLOSION:
                pExplosion->flags          = pThing->thingInfo.explosionInfo.flags;
                pExplosion->damage         = pThing->thingInfo.explosionInfo.damage;
                pExplosion->damageType     = pThing->thingInfo.explosionInfo.damageType;
                pExplosion->range          = pThing->thingInfo.explosionInfo.range;
                pExplosion->force          = pThing->thingInfo.explosionInfo.force;
                pExplosion->msecBlastTime  = pThing->thingInfo.explosionInfo.msecBlastTime;
                pExplosion->msecBabyTime   = pThing->thingInfo.explosionInfo.msecBabyTime;
                pExplosion->msecExpandTime = pThing->thingInfo.explosionInfo.msecExpandTime;
                pExplosion->msecFadeTime   = pThing->thingInfo.explosionInfo.msecFadeTime;
                pExplosion->maxLight       = pThing->thingInfo.explosionInfo.maxLight;
                rdVector_Copy3(&pExplosion->spriteStart, &pThing->thingInfo.explosionInfo.spriteStart);
                rdVector_Copy3(&pExplosion->spriteEnd, &pThing->thingInfo.explosionInfo.spriteEnd);
                if ( pThing->thingInfo.explosionInfo.aSpriteTemplateName ) // Note, retail tests the array itself, which is never null
                {
                    strncpy(pExplosion->aSpriteTemplateName, pThing->thingInfo.explosionInfo.aSpriteTemplateName, STD_ARRAYLEN(pExplosion->aSpriteTemplateName) - 1);
                    pExplosion->aSpriteTemplateName[STD_ARRAYLEN(pExplosion->aSpriteTemplateName) - 1] = 0;
                }

                for ( debrisNum = 0; debrisNum < STD_ARRAYLEN(pThing->thingInfo.explosionInfo.apDebries); ++debrisNum )
                {
                    if ( pThing->thingInfo.explosionInfo.apDebries[debrisNum] )
                    {
                        strncpy(pExplosion->aDebrisNames[debrisNum], pThing->thingInfo.explosionInfo.apDebries[debrisNum]->aName, STD_ARRAYLEN(pExplosion->aDebrisNames[debrisNum]) - 1);
                        pExplosion->aDebrisNames[debrisNum][STD_ARRAYLEN(pExplosion->aDebrisNames[debrisNum]) - 1] = 0;
                    }
                }

                ++pExplosion;
                break;

            case SITH_THING_ITEM:
                pItem->flags              = pThing->thingInfo.itemInfo.flags;
                pItem->secRespawnInterval = pThing->thingInfo.itemInfo.secRespawnInterval;
                ++pItem;
                break;

            case SITH_THING_HINT:
                pHint->userval = pThing->userval;
                ++pHint;
                break;

            case SITH_THING_PARTICLE:
                pParticle->flags        = pThing->thingInfo.particleInfo.flags;
                pParticle->growthSpeed  = pThing->thingInfo.particleInfo.growthSpeed;
                pParticle->minRadius    = pThing->thingInfo.particleInfo.minRadius;
                pParticle->maxRadius    = pThing->thingInfo.particleInfo.maxRadius;
                pParticle->size         = pThing->thingInfo.particleInfo.size;
                pParticle->timeoutRate  = pThing->thingInfo.particleInfo.timeoutRate;
                pParticle->numParticles = pThing->thingInfo.particleInfo.numParticles;
                pParticle->pitchRange   = pThing->thingInfo.particleInfo.pitchRange;
                pParticle->yawRange     = pThing->thingInfo.particleInfo.yawRange;
                if ( pThing->thingInfo.particleInfo.pMaterial )
                {
                    strncpy(pParticle->aMaterialName, pThing->thingInfo.particleInfo.pMaterial->aName, STD_ARRAYLEN(pParticle->aMaterialName) - 1);
                    pParticle->aMaterialName[STD_ARRAYLEN(pParticle->aMaterialName) - 1] = 0;
                }

                ++pParticle;
                break;

            default:
                break;
        }

        switch ( pThing->controlType )
        {
            case SITH_CT_AI:
                if ( pThing->controlInfo.aiControl.pClass )
                {
                    strncpy(pAI->aClassName, pThing->controlInfo.aiControl.pClass->aName, STD_ARRAYLEN(pAI->aClassName) - 1);
                    pAI->aClassName[STD_ARRAYLEN(pAI->aClassName) - 1] = 0;
                }

                if ( pThing->controlInfo.aiControl.pLocal && pThing->controlInfo.aiControl.pLocal->numFrames )
                {
                    pAI->numFrames = pThing->controlInfo.aiControl.pLocal->numFrames;
                    memcpy(pAIFrame, pThing->controlInfo.aiControl.pLocal->aFrames, sizeof(rdVector3) * pAI->numFrames);
                    pAIFrame += pAI->numFrames;
                }

                ++pAI;
                break;

            default:
                break;
        }
    }

    nWritten = sith_pHS->pFileWrite(fh, aRecords, sizeRecords);
    if ( nWritten != sizeRecords )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, &header, sizeof(header));
    if ( nWritten != sizeof(header) )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aPhysics, sizePhysics);
    if ( nWritten != sizePhysics )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aPathFrameCounts, sizePathFrameCounts);
    if ( nWritten != sizePathFrameCounts )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aPathFrames, sizePathFrames);
    if ( nWritten != sizePathFrames )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aActors, sizeActors);
    if ( nWritten != sizeActors )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aWeapons, sizeWeapons);
    if ( nWritten != sizeWeapons )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aExplosions, sizeExplosions);
    if ( nWritten != sizeExplosions )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aItems, sizeItems);
    if ( nWritten != sizeItems )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aHints, sizeHints);
    if ( nWritten != sizeHints )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aParticles, sizeParticles);
    if ( nWritten != sizeParticles )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aAIs, sizeAIs);
    if ( nWritten != sizeAIs )
    {
        goto error;
    }

    nWritten = sith_pHS->pFileWrite(fh, aAIFrames, sizeAIFrames);
    if ( nWritten != sizeAIFrames )
    {
        goto error;
    }

    error = 0;

error:
    if ( aAIFrames )
    {
        stdMemory_Free(aAIFrames);
    }

    if ( aAIs )
    {
        stdMemory_Free(aAIs);
    }

    if ( aParticles )
    {
        stdMemory_Free(aParticles);
    }

    if ( aHints )
    {
        stdMemory_Free(aHints);
    }

    if ( aItems )
    {
        stdMemory_Free(aItems);
    }

    if ( aExplosions )
    {
        stdMemory_Free(aExplosions);
    }

    if ( aWeapons )
    {
        stdMemory_Free(aWeapons);
    }

    if ( aActors )
    {
        stdMemory_Free(aActors);
    }

    if ( aPhysics )
    {
        stdMemory_Free(aPhysics);
    }

    if ( aPathFrames )
    {
        stdMemory_Free(aPathFrames);
    }

    if ( aPathFrameCounts )
    {
        stdMemory_Free(aPathFrameCounts);
    }

    if ( aRecords )
    {
        stdMemory_Free(aRecords);
    }

    return error;
}

int J3DAPI sithThing_ReadThingsListBinary(tFileHandle fh, SithWorld* pWorld, size_t numThings, SithThing* aThings, void (J3DAPI* pfInitThingFunc)(SithThing*))
{
    int error = 1;
    size_t nRead;
    size_t i;
    SithThingBinaryHeader header;
    SithThingBinaryRecord* aRecords = NULL;
    SithPhysicsBinaryRecord* aPhysics = NULL;
    size_t* aPathFrameCounts = NULL;
    SithPathFrame* aPathFrames = NULL;
    SithActorBinaryRecord* aActors = NULL;
    SithWeaponBinaryRecord* aWeapons = NULL;
    SithExplosionBinaryRecord* aExplosions = NULL;
    SithItemBinaryRecord* aItems = NULL;
    SithHintBinaryRecord* aHints = NULL;
    SithParticleBinaryRecord* aParticles = NULL;
    SithAIBinaryRecord* aAIs = NULL;
    rdVector3* aAIFrames = NULL;
    size_t sizeRecords;
    size_t sizePhysics;
    size_t sizePathFrameCounts;
    size_t sizePathFrames;
    size_t sizeActors;
    size_t sizeWeapons;
    size_t sizeExplosions;
    size_t sizeItems;
    size_t sizeHints;
    size_t sizeParticles;
    size_t sizeAIs;
    size_t sizeAIFrames;
    SithThing* pThing;
    SithThingBinaryRecord* pRec;
    SithPhysicsBinaryRecord* pPhysics;
    size_t* pPathFrameCount;
    SithPathFrame* pPathFrame;
    SithActorBinaryRecord* pActor;
    SithWeaponBinaryRecord* pWeapon;
    SithExplosionBinaryRecord* pExplosion;
    SithItemBinaryRecord* pItem;
    SithHintBinaryRecord* pHint;
    SithParticleBinaryRecord* pParticle;
    SithAIBinaryRecord* pAI;
    rdVector3* pAIFrame;
    rdModel3* pModel;
    SithThing* pTemplate;
    rdParticle* pParticleCloud;
    SithSector* pSector;
    rdMatrix34 orient;
    size_t debrisNum;
    rdSprite3* pSprite;

    sizeRecords = numThings * sizeof(SithThingBinaryRecord);
#line 4190
    aRecords = (SithThingBinaryRecord*)STDMALLOC(sizeRecords);
    if ( !aRecords )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aRecords, sizeRecords);
    if ( nRead != sizeRecords )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, &header, sizeof(header));
    if ( nRead != sizeof(header) )
    {
        goto error;
    }

    sizePhysics = header.numPhysicsThings * sizeof(SithPhysicsBinaryRecord);
#line 4200
    aPhysics = (SithPhysicsBinaryRecord*)STDMALLOC(sizePhysics);
    if ( !aPhysics )
    {
        goto error;
    }

    memset(aPhysics, 0, sizePhysics);
    pPhysics = aPhysics;

    sizePathFrameCounts = header.numPathThings * sizeof(size_t);
#line 4206
    aPathFrameCounts = (size_t*)STDMALLOC(sizePathFrameCounts);
    if ( !aPathFrameCounts )
    {
        goto error;
    }

    pPathFrameCount = aPathFrameCounts;

    sizePathFrames = header.numPathFrames * sizeof(SithPathFrame);
#line 4211
    aPathFrames = (SithPathFrame*)STDMALLOC(sizePathFrames);
    if ( !aPathFrames )
    {
        goto error;
    }

    pPathFrame = aPathFrames;

    sizeActors = header.numActorThings * sizeof(SithActorBinaryRecord);
#line 4216
    aActors = (SithActorBinaryRecord*)STDMALLOC(sizeActors);
    if ( !aActors )
    {
        goto error;
    }

    pActor = aActors;

    sizeWeapons = header.numWeaponThings * sizeof(SithWeaponBinaryRecord);
#line 4221
    aWeapons = (SithWeaponBinaryRecord*)STDMALLOC(sizeWeapons);
    if ( !aWeapons )
    {
        goto error;
    }

    pWeapon = aWeapons;

    sizeExplosions = header.numExplosionThings * sizeof(SithExplosionBinaryRecord);
#line 4226
    aExplosions = (SithExplosionBinaryRecord*)STDMALLOC(sizeExplosions);
    if ( !aExplosions )
    {
        goto error;
    }

    pExplosion = aExplosions;

    sizeItems = header.numItemThings * sizeof(SithItemBinaryRecord);
#line 4231
    aItems = (SithItemBinaryRecord*)STDMALLOC(sizeItems);
    if ( !aItems )
    {
        goto error;
    }

    memset(aItems, 0, sizeItems);
    pItem = aItems;

    sizeHints = header.numHintThings * sizeof(SithHintBinaryRecord);
#line 4237
    aHints = (SithHintBinaryRecord*)STDMALLOC(sizeHints);
    if ( !aHints )
    {
        goto error;
    }

    pHint = aHints;

    sizeParticles = header.numParticleThings * sizeof(SithParticleBinaryRecord);
#line 4242
    aParticles = (SithParticleBinaryRecord*)STDMALLOC(sizeParticles);
    if ( !aParticles )
    {
        goto error;
    }

    memset(aParticles, 0, sizeParticles);
    pParticle = aParticles;

    sizeAIs = header.numAIThings * sizeof(SithAIBinaryRecord);
#line 4248
    aAIs = (SithAIBinaryRecord*)STDMALLOC(sizeAIs);
    if ( !aAIs )
    {
        goto error;
    }

    memset(aAIs, 0, sizeAIs);
    pAI = aAIs;

    sizeAIFrames = header.numAIFrames * sizeof(rdVector3);
#line 4254
    aAIFrames = (rdVector3*)STDMALLOC(sizeAIFrames);
    if ( !aAIFrames )
    {
        goto error;
    }

    memset(aAIFrames, 0, sizeAIFrames);
    pAIFrame = aAIFrames;

    nRead = sith_pHS->pFileRead(fh, aPhysics, sizePhysics);
    if ( nRead != sizePhysics )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aPathFrameCounts, sizePathFrameCounts);
    if ( nRead != sizePathFrameCounts )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aPathFrames, sizePathFrames);
    if ( nRead != sizePathFrames )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aActors, sizeActors);
    if ( nRead != sizeActors )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aWeapons, sizeWeapons);
    if ( nRead != sizeWeapons )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aExplosions, sizeExplosions);
    if ( nRead != sizeExplosions )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aItems, sizeItems);
    if ( nRead != sizeItems )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aHints, sizeHints);
    if ( nRead != sizeHints )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aParticles, sizeParticles);
    if ( nRead != sizeParticles )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aAIs, sizeAIs);
    if ( nRead != sizeAIs )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aAIFrames, sizeAIFrames);
    if ( nRead != sizeAIFrames )
    {
        goto error;
    }

    for ( i = 0; i < numThings; i++ )
    {
        pThing = &aThings[i];
        pRec = &aRecords[i];
        pTemplate = sithTemplate_GetTemplate(pRec->aTemplateName);
        if ( pTemplate )
        {
            sithThing_SetThingBasedOn(pThing, pTemplate);
        }

        rdMatrix_BuildRotate34(&orient, &pRec->pyr);
        sithThing_SetPositionAndOrient(pThing, &pRec->pos, &orient);

        pSector = sithSector_GetSectorEx(pWorld, pRec->sectorNum);
        if ( pSector )
        {
            sithThing_EnterSector(pThing, pSector, 1, 1);
        }

        strncpy(pThing->aName, pRec->aName, STD_ARRAYLEN(pThing->aName) - 1);
        pThing->aName[STD_ARRAYLEN(pThing->aName) - 1] = 0;

        pThing->flags = pRec->flags;
        if ( (pThing->flags & SITH_TF_SEEN) != 0 )
        {
            pThing->flags &= ~SITH_TF_SEEN;
        }

        pThing->type        = pRec->type;
        pThing->moveType    = pRec->moveType;
        pThing->controlType = pRec->controlType;

        rdVector_Copy4(&pThing->light.color, &pRec->color);
        rdVector_Copy4(&pThing->light.emitColor, &pRec->emitColor);
        pThing->light.minRadius = pThing->light.maxRadius = pRec->color.w;

        pThing->msecLifeLeft = pRec->msecLifeLeft;
        pThing->collide      = pRec->collide;
        pThing->perfLevel    = pRec->perfLevel;

        if ( !pfInitThingFunc )
        {
            sithThing_Initialize(pWorld, pThing, 1);
        }

        if ( pRec->aRenderName[0] )
        {
            switch ( pRec->renderType )
            {
                case RD_THING_MODEL3:
                {
                    pModel = sithModel_Load(pRec->aRenderName, 0);
                    if ( pModel )
                    {
                        rdThing_FreeEntry(&pThing->renderData);
                        rdThing_SetModel3(&pThing->renderData, pModel);
                    }
                    else
                    {
#line 4337
                        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not load model '%s'.\n", pRec->aRenderName, 0, 0, 0);
                    }
                }

                break;

                case RD_THING_SPRITE3:
                {
                    pSprite = sithSprite_Load(pWorld, pRec->aRenderName);
                    if ( pSprite )
                    {
                        rdThing_FreeEntry(&pThing->renderData);
                        rdThing_SetSprite3(&pThing->renderData, pSprite);
                    }
                    else
                    {
#line 4347
                        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create sprite %s.\n", pRec->aRenderName, 0, 0, 0);
                    }
                }

                break;

                case RD_THING_PARTICLE:
                {
                    pParticleCloud = sithParticle_Load(pWorld, pRec->aRenderName);
                    if ( pParticleCloud )
                    {
                        rdThing_FreeEntry(&pThing->renderData);
                        rdThing_SetParticleCloud(&pThing->renderData, pParticleCloud);
                    }
                    else
                    {
#line 4357
                        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not load particle '%s'.\n", pRec->aRenderName, 0, 0, 0);
                    }
                }

                break;
            }
        }

        if ( pRec->aPuppetName[0] )
        {
            pThing->pPuppetClass = sithPuppet_LoadPuppetClass(pRec->aPuppetName);
            if ( !pThing->pPuppetClass && !pThing->renderData.pPuppet )
            {
                goto error;
            }
        }

        pThing->pSoundClass          = sithSoundClass_Load(pWorld, pRec->aSoundClassName);
        pThing->pCreateThingTemplate = sithTemplate_GetTemplate(pRec->aCreateThingName);

        if ( pRec->aCogName[0] )
        {
            pThing->pCog = sithCog_Load(pWorld, pRec->aCogName);
            if ( pThing->pCog )
            {
                pThing->pCog->flags |= SITHCOG_CLASS | SITHCOG_LOCAL;
                pThing->flags |= SITH_TF_COGLINKED;
            }
        }

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                pThing->moveInfo.physics.flags      = pPhysics->flags;
                pThing->moveInfo.physics.mass       = pPhysics->mass;
                pThing->moveInfo.physics.height     = pPhysics->height;
                pThing->moveInfo.physics.airDrag    = pPhysics->airDrag;
                pThing->moveInfo.physics.surfDrag   = pPhysics->surfDrag;
                pThing->moveInfo.physics.staticDrag = pPhysics->staticDrag;
                rdVector_Copy3(&pThing->moveInfo.physics.angularVelocity, &pPhysics->angularVelocity);
                rdVector_Copy3(&pThing->moveInfo.physics.velocity, &pPhysics->velocity);
                pThing->moveInfo.physics.maxRotationVelocity = pPhysics->maxRotationVelocity;
                pThing->moveInfo.physics.maxVelocity         = pPhysics->maxVelocity;
                pThing->moveInfo.physics.orientSpeed         = pPhysics->orientSpeed;
                pThing->moveInfo.physics.buoyancy            = pPhysics->buoyancy;
                ++pPhysics;
                break;

            case SITH_MT_PATH:
                if ( *pPathFrameCount )
                {
#line 4403
                    pThing->moveInfo.pathMovement.aFrames = (SithPathFrame*)STDMALLOC(sizeof(SithPathFrame) * *pPathFrameCount);
                    if ( !pThing->moveInfo.pathMovement.aFrames )
                    {
                        goto error;
                    }

                    pThing->moveInfo.pathMovement.sizeFrames = *pPathFrameCount;
                    pThing->moveInfo.pathMovement.numFrames  = *pPathFrameCount;
                    memcpy(pThing->moveInfo.pathMovement.aFrames, pPathFrame, sizeof(SithPathFrame) * *pPathFrameCount);
                    pPathFrame += *pPathFrameCount;
                }

                ++pPathFrameCount;
                break;
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                pThing->thingInfo.actorInfo.flags     = pActor->flags;
                pThing->thingInfo.actorInfo.health    = pActor->health;
                pThing->thingInfo.actorInfo.maxHealth = pActor->maxHealth;
                if ( pActor->aWeaponTemplateName[0] )
                {
                    pThing->thingInfo.actorInfo.pWeaponTemplate = sithTemplate_GetTemplate(pActor->aWeaponTemplateName);
                }

                pThing->thingInfo.actorInfo.maxThrust       = pActor->maxThrust;
                pThing->thingInfo.actorInfo.maxRotVelocity  = pActor->maxRotVelocity;
                pThing->thingInfo.actorInfo.maxHeadVelocity = pActor->maxHeadVelocity;
                pThing->thingInfo.actorInfo.maxHeadYaw      = pActor->maxHeadYaw;
                pThing->thingInfo.actorInfo.jumpSpeed       = pActor->jumpSpeed;
                rdVector_Copy3(&pThing->thingInfo.actorInfo.eyeOffset, &pActor->eyeOffset);
                pThing->thingInfo.actorInfo.minHeadPitch = pActor->minHeadPitch;
                pThing->thingInfo.actorInfo.maxHeadPitch = pActor->maxHeadPitch;
                rdVector_Copy3(&pThing->thingInfo.actorInfo.fireOffset, &pActor->fireOffset);
                rdVector_Copy3(&pThing->thingInfo.actorInfo.lightOffset, &pActor->lightOffset);
                rdVector_Copy4(&pThing->thingInfo.actorInfo.headLightIntensity, &pActor->headLightIntensity);
                memcpy(&pThing->thingInfo.actorInfo.voiceInfo.voiceColor, &pActor->voiceColor, sizeof(VGradiantColor));
                if ( pActor->aExplodeTemplateName[0] )
                {
                    pThing->thingInfo.actorInfo.pExplodeTemplate = sithTemplate_GetTemplate(pActor->aExplodeTemplateName);
                }

                ++pActor;
                break;

            case SITH_THING_WEAPON:
                pThing->thingInfo.weaponInfo.flags = pWeapon->flags;
                if ( pWeapon->aExplosionTemplateName[0] )
                {
                    pThing->thingInfo.weaponInfo.pExplosionTemplate = sithTemplate_GetTemplate(pWeapon->aExplosionTemplateName);
                }

                pThing->thingInfo.weaponInfo.damage     = pWeapon->damage;
                pThing->thingInfo.weaponInfo.minDamage  = pWeapon->minDamage;
                pThing->thingInfo.weaponInfo.rate       = pWeapon->rate;
                pThing->thingInfo.weaponInfo.damageType = pWeapon->damageType;
                pThing->thingInfo.weaponInfo.range      = pWeapon->range;
                pThing->thingInfo.weaponInfo.force      = pWeapon->force;
                ++pWeapon;
                break;

            case SITH_THING_EXPLOSION:
                pThing->thingInfo.explosionInfo.flags          = pExplosion->flags;
                pThing->thingInfo.explosionInfo.damage         = pExplosion->damage;
                pThing->thingInfo.explosionInfo.damageType     = pExplosion->damageType;
                pThing->thingInfo.explosionInfo.range          = pExplosion->range;
                pThing->thingInfo.explosionInfo.force          = pExplosion->force;
                pThing->thingInfo.explosionInfo.msecBlastTime  = pExplosion->msecBlastTime;
                pThing->thingInfo.explosionInfo.msecBabyTime   = pExplosion->msecBabyTime;
                pThing->thingInfo.explosionInfo.msecFadeTime   = pExplosion->msecFadeTime;
                pThing->thingInfo.explosionInfo.msecExpandTime = pExplosion->msecExpandTime;
                pThing->thingInfo.explosionInfo.maxLight       = pExplosion->maxLight;
                rdVector_Copy3(&pThing->thingInfo.explosionInfo.spriteStart, &pExplosion->spriteStart);
                rdVector_Copy3(&pThing->thingInfo.explosionInfo.spriteEnd, &pExplosion->spriteEnd);
                if ( pExplosion->aSpriteTemplateName[0] )
                {
                    strncpy(pThing->thingInfo.explosionInfo.aSpriteTemplateName, pExplosion->aSpriteTemplateName, STD_ARRAYLEN(pThing->thingInfo.explosionInfo.aSpriteTemplateName) - 1);
                    pThing->thingInfo.explosionInfo.aSpriteTemplateName[STD_ARRAYLEN(pThing->thingInfo.explosionInfo.aSpriteTemplateName) - 1] = 0;
                }

                for ( debrisNum = 0; debrisNum < STD_ARRAYLEN(pThing->thingInfo.explosionInfo.apDebries); ++debrisNum )
                {
                    if ( pExplosion->aDebrisNames[debrisNum][0] )
                    {
                        pThing->thingInfo.explosionInfo.apDebries[debrisNum] = sithTemplate_GetTemplate(pExplosion->aDebrisNames[debrisNum]);
                    }
                }

                ++pExplosion;
                break;

            case SITH_THING_ITEM:
                pThing->thingInfo.itemInfo.flags              = pItem->flags;
                pThing->thingInfo.itemInfo.secRespawnInterval = pItem->secRespawnInterval;
                ++pItem;
                break;

            case SITH_THING_HINT:
                pThing->userval = pHint->userval;
                ++pHint;
                break;

            case SITH_THING_PARTICLE:
                pThing->thingInfo.particleInfo.flags        = pParticle->flags;
                pThing->thingInfo.particleInfo.growthSpeed  = pParticle->growthSpeed;
                pThing->thingInfo.particleInfo.minRadius    = pParticle->minRadius;
                pThing->thingInfo.particleInfo.maxRadius    = pParticle->maxRadius;
                pThing->thingInfo.particleInfo.size         = pParticle->size;
                pThing->thingInfo.particleInfo.timeoutRate  = pParticle->timeoutRate;
                pThing->thingInfo.particleInfo.numParticles = pParticle->numParticles;
                pThing->thingInfo.particleInfo.pitchRange   = pParticle->pitchRange;
                pThing->thingInfo.particleInfo.yawRange     = pParticle->yawRange;
                if ( pParticle->aMaterialName[0] )
                {
                    pThing->thingInfo.particleInfo.pMaterial = sithMaterial_Load(pParticle->aMaterialName);
                }

                ++pParticle;
                break;
        }

        switch ( pThing->controlType )
        {
            case SITH_CT_AI:
                if ( pAI->aClassName[0] )
                {
                    pThing->controlInfo.aiControl.pClass = sithAIClass_Load(pWorld, pAI->aClassName);
                    if ( pThing->controlInfo.aiControl.pLocal && pThing->controlInfo.aiControl.pClass )
                    {
                        pThing->controlInfo.aiControl.pLocal->pClass        = pThing->controlInfo.aiControl.pClass;
                        pThing->controlInfo.aiControl.pLocal->numInstincts = pThing->controlInfo.aiControl.pClass->numInstincts;
                    }
                }

                if ( pAI->numFrames )
                {
                    if ( sithAI_AllocAIFrames(pThing->controlInfo.aiControl.pLocal, pAI->numFrames) )
                    {
                        goto error;
                    }

                    memcpy(pThing->controlInfo.aiControl.pLocal->aFrames, pAIFrame, sizeof(rdVector3) * pAI->numFrames);
                    pThing->controlInfo.aiControl.pLocal->numFrames = pAI->numFrames;
                    pAIFrame += pAI->numFrames;
                }

                ++pAI;
                break;
        }

        if ( pfInitThingFunc )
        {
            pfInitThingFunc(pThing);
        }
    }

    error = 0;

error:
    if ( aAIFrames )
    {
        stdMemory_Free(aAIFrames);
    }

    if ( aAIs )
    {
        stdMemory_Free(aAIs);
    }

    if ( aParticles )
    {
        stdMemory_Free(aParticles);
    }

    if ( aHints )
    {
        stdMemory_Free(aHints);
    }

    if ( aItems )
    {
        stdMemory_Free(aItems);
    }

    if ( aExplosions )
    {
        stdMemory_Free(aExplosions);
    }

    if ( aWeapons )
    {
        stdMemory_Free(aWeapons);
    }

    if ( aActors )
    {
        stdMemory_Free(aActors);
    }

    if ( aPhysics )
    {
        stdMemory_Free(aPhysics);
    }

    if ( aPathFrames )
    {
        stdMemory_Free(aPathFrames);
    }

    if ( aPathFrameCounts )
    {
        stdMemory_Free(aPathFrameCounts);
    }

    if ( aRecords )
    {
        stdMemory_Free(aRecords);
    }

    return error;
}

int J3DAPI sithThing_ParseArg(StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing)
{
    size_t adjNumber;
    int parseRes;
    int bParsed = 0;

#line 4601
    SITH_ASSERTREL((pArg!=NULL) && (pThing != NULL));
    SITH_ASSERTREL((strlen(pArg->argName) > 0));

    adjNumber = (size_t)stdHashtbl_Find(sithThing_pParseHashtbl, pArg->argName);
    if ( !adjNumber )
    {
    }
    else
    {
        parseRes = sithThing_ParseThingArg(pArg, pWorld, pThing, adjNumber);
        switch ( parseRes )
        {
            case SITHTHING_PARSEARG_PARSED:
                return 0;

            case SITHTHING_PARSEARG_BADSYNTAX:
                return 1;
        }

        switch ( pThing->type )
        {
            case SITH_THING_ACTOR:
            case SITH_THING_PLAYER:
                bParsed = sithActor_ParseArg(pArg, pThing, adjNumber);
                break;

            case SITH_THING_WEAPON:
                bParsed = sithWeapon_ParseArg(pArg, pThing, adjNumber);
                break;

            case SITH_THING_EXPLOSION:
                bParsed = sithExplosion_ParseArg(pArg, pThing, adjNumber);
                break;

            case SITH_THING_ITEM:
                bParsed = sithItem_ParseArg(pArg, pThing, adjNumber);
                break;

            case SITH_THING_PARTICLE:
                bParsed = sithParticle_ParseArg(pArg, pThing, adjNumber);
                break;
        }

        if ( bParsed )
        {
            return 0;
        }

        switch ( pThing->moveType )
        {
            case SITH_MT_PHYSICS:
                bParsed = sithPhysics_ParseArg(pArg, pThing, adjNumber);
                break;

            case SITH_MT_PATH:
                bParsed = sithPathMove_ParseArg(pArg, pThing, adjNumber);
                break;
        }

        if ( bParsed )
        {
            return 0;
        }

        if ( pThing->controlType == SITH_CT_AI && sithAI_ParseArg(pArg, pThing, adjNumber) )
        {
            return 0;
        }
    }

#line 4668
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad argument thing %s, %s=%s line %d.\n", pThing->aName, pArg->argName, pArg->argValue, stdConffile_GetLineNumber());
    return 0;
}

int J3DAPI sithThing_ParseThingArg(const StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing, int adjNum)
{
    float value;
    int flags;
    int numArgs;
    rdVector3 vec;
    rdModel3* pModel3;

    switch ( adjNum )
    {
        case SITHTHING_ARG_TYPE:
        {
            pThing->type = sithThing_ParseType(pArg->argValue);
            switch ( pThing->type )
            {
                case SITH_THING_ACTOR:
                    pThing->controlType = SITH_CT_AI;
                    break;

                case SITH_THING_EXPLOSION:
                    pThing->controlType = SITH_CT_EXPLOSION;
                    break;

                case SITH_THING_PARTICLE:
                    pThing->controlType = SITH_CT_PARTICLE;
                    break;

                case SITH_THING_SPRITE:
                case SITH_THING_POLYLINE:
                    pThing->controlType = SITH_CT_PLOT;
                    break;

                case SITH_THING_PLAYER:
                    pThing->controlType = SITH_CT_PLAYER;
                    break;
            }

            break;
        }
        case SITHTHING_ARG_AICLASS:
        {
            pThing->controlType = SITH_CT_AI;
            pThing->controlInfo.aiControl.pClass = sithAIClass_Load(pWorld, pArg->argValue);
            if ( pThing->controlInfo.aiControl.pLocal && pThing->controlInfo.aiControl.pClass )
            {
                pThing->controlInfo.aiControl.pLocal->pClass       = pThing->controlInfo.aiControl.pClass;
                pThing->controlInfo.aiControl.pLocal->numInstincts = pThing->controlInfo.aiControl.pClass->numInstincts;
            }

            break;
        }
        case SITHTHING_ARG_COLLIDE:
        {
            flags = atoi(pArg->argValue);
            if ( flags < SITH_COLLIDE_NONE || flags > SITH_COLLIDE_FACE )
            {
                goto bad_syntax;
            }

            pThing->collide.type = flags;
            break;
        }
        case SITHTHING_ARG_COLLWIDTH:
        {
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f )
            {
                goto bad_syntax;
            }

            pThing->collide.width    = value;
            pThing->collide.unkWidth = value;
            break;
        }
        case SITHTHING_ARG_COLLHEIGHT:
        {
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f )
            {
                goto bad_syntax;
            }

            pThing->collide.height    = value;
            pThing->collide.unkHeight = value;
            break;
        }
        case SITHTHING_ARG_MOVE:
        {
            if ( streq(pArg->argValue, "physics") )
            {
                pThing->moveType = SITH_MT_PHYSICS;
            }
            else if ( streq(pArg->argValue, "path") )
            {
                pThing->moveType = SITH_MT_PATH;
            }
            else if ( streq(pArg->argValue, "none") )
            {
                pThing->moveType = SITH_MT_NONE;
            }
            else
            {
                // Couldn't parse move type
                goto bad_syntax;
            }

            break;
        }
        case SITHTHING_ARG_SIZE:
        {
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f )
            {
                goto bad_syntax;
            }

            pThing->collide.movesize = value;
            pThing->collide.size     = value;
            break;
        }
        case SITHTHING_ARG_MOVESIZE:
        {
            if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
            {
                value = D3DVAL(atof(pArg->argValue));
                if ( value < 0.0f )
                {
                    goto bad_syntax;
                }

                pThing->collide.movesize = value;
            }

            break;
        }
        case SITHTHING_ARG_THINGFLAGS:
        {
            if ( sscanf(pArg->argValue, "%x", &flags) != 1 )
            {
                goto bad_syntax;
            }

            pThing->flags = flags;
            break;
        }
        case SITHTHING_ARG_LIGHT:
        {
            value = 0.0f; // range; TODO: There is a bug in engine with light when range is 0.0, consider setting it to 0.01 - thing flat light
            numArgs = sscanf(pArg->argValue, "(%f/%f/%f/%f)", &vec.red, &vec.green, &vec.blue, &value);
            if ( numArgs != 4 )
            {
                numArgs = sscanf(pArg->argValue, "(%f/%f/%f)", &vec.red, &vec.green, &vec.blue);
                if ( numArgs != 3 )
                {
                    goto bad_syntax;
                }
            }

            pThing->light.color.red   = vec.red;
            pThing->light.color.green = vec.green;
            pThing->light.color.blue  = vec.blue;
            pThing->light.color.alpha = value;

            pThing->light.emitColor.red   = vec.red;
            pThing->light.emitColor.green = vec.green;
            pThing->light.emitColor.blue  = vec.blue;
            pThing->light.emitColor.alpha = value;

            pThing->light.maxRadius = value; // TODO: To add light falloff effect maybe calculate max light same way as for sector point light or add additional format for max light range
            pThing->light.minRadius = value;

            pThing->flags |= SITH_TF_EMITLIGHT;
            break;
        }
        case SITHTHING_ARG_TIMER:
        {
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f )
            {
                goto bad_syntax;
            }

            pThing->msecLifeLeft = (int32_t)RD_MULTIPLY(value, 1000.0f);
            break;
        }
        case SITHTHING_ARG_USERVAL:
        {
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f )
            {
                goto bad_syntax;
            }

            pThing->userval = value;
            break;
        }
        case SITHTHING_ARG_MODEL3D:
        {
            rdThing_FreeEntry(&pThing->renderData);

            pModel3 = sithModel_Load(pArg->argValue, /*bSkipDefault=*/0);
            if ( !pModel3 )
            {
#line 4847
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not load model '%s' specified on line %d.\n", pArg->argValue, stdConffile_GetLineNumber(), 0, 0);
                goto bad_syntax;
            }

            rdThing_SetModel3(&pThing->renderData, pModel3);

            if ( pThing->collide.size == 0.0f )
            {
                pThing->collide.size = pThing->renderData.data.pModel3->radius;
            }

            if ( pThing->collide.movesize == 0.0f )
            {
                pThing->collide.movesize = pThing->renderData.data.pModel3->radius;
            }

            if ( pThing->collide.unkWidth == 0.0f )
            {
                pThing->collide.width    = pThing->collide.movesize;
                pThing->collide.unkWidth = pThing->collide.width;
            }

            if ( pThing->collide.unkHeight == 0.0f )
            {
                pThing->collide.height    = pThing->collide.movesize;
                pThing->collide.unkHeight = pThing->collide.height;
            }

            break;
        }
        case SITHTHING_ARG_PARTICLE:
        {
            rdParticle* pParticle = sithParticle_Load(pWorld, pArg->argValue);
            if ( pParticle )
            {
                rdThing_FreeEntry(&pThing->renderData);
                rdThing_SetParticleCloud(&pThing->renderData, pParticle);
            }

            break;
        }
        case SITHTHING_ARG_SPRITE:
        {
            rdSprite3* pSprite3;

            rdThing_FreeEntry(&pThing->renderData);

            pSprite3 = sithSprite_Load(pWorld, pArg->argValue);
            if ( !pSprite3 )
            {
#line 4887
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create sprite %s, line %d.\n", pArg->argValue, stdConffile_GetLineNumber(), 0, 0);
                goto bad_syntax;
            }

            rdThing_SetSprite3(&pThing->renderData, pSprite3);
            break;
        }
        case SITHTHING_ARG_PUPPET:
        {
            pThing->pPuppetClass = sithPuppet_LoadPuppetClass(pArg->argValue);
            if ( !pThing->pPuppetClass )
            {
                break;
            }

            if ( !pThing->renderData.pPuppet )
            {
                rdPuppet_New(&pThing->renderData);
            }

            break;
        }
        case SITHTHING_ARG_SOUNDCLASS:
        {
            pThing->pSoundClass = sithSoundClass_Load(pWorld, pArg->argValue);
            if ( !pThing->pSoundClass )
            {
#line 4908
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not load soundclass %s, line %d.\n", pArg->argValue, stdConffile_GetLineNumber(), 0, 0);
            }

            break;
        }
        case SITHTHING_ARG_COG:
        {
            pThing->pCog = sithCog_Load(pWorld, pArg->argValue);
            if ( pThing->pCog )
            {
                pThing->pCog->flags |= SITHCOG_LOCAL | SITHCOG_CLASS;
                pThing->flags       |= SITH_TF_COGLINKED;
            }

            break;
        }
        case SITHTHING_ARG_CREATETHING:
        {
            pThing->pCreateThingTemplate = sithTemplate_GetTemplate(pArg->argValue);
            break;
        }
        case SITHTHING_ARG_ORIENT:
        {
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &vec.pitch, &vec.yaw, &vec.roll) != 3 )
            {
                goto bad_syntax;
            }

            rdMatrix_BuildRotate34(&pThing->orient, &vec);
            break;
        }
        case SITHTHING_ARG_PERFLEVEL:
        {
            if ( sscanf(pArg->argValue, "%d", &pThing->perfLevel) != 1 )
            {
                goto bad_syntax;
            }

            break;
        }

        default:
            return SITHTHING_PARSEARG_NOTPARSED;
    }

    return SITHTHING_PARSEARG_PARSED;

bad_syntax:
#line 4944
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad argument %s=%s, line %d in %s.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber(), stdConffile_GetFilename());
    return SITHTHING_PARSEARG_BADSYNTAX;
}

int J3DAPI sithThing_ParseType(const char* pType)
{
    int i;

    for ( i = 0; i < (int)STD_ARRAYLEN(sithThing_aStrThingTypes); ++i )
    {
        if ( streq(pType, sithThing_aStrThingTypes[i]) ) //streqi maybe?
        {
            return i;
        }
    }

    return SITH_THING_FREE;
}

int J3DAPI sithThing_ValidateThingPointer(const SithWorld* pWorld, const SithThing* pThing)
{
    if ( !pThing || pThing->idx != pThing - pWorld->aThings || pThing->idx >= STD_ARRAYLEN(sithThing_aFreeThingIdxs) )
    {
        return 0;
    }

    return 1;
}

/* Linker-discarded: retail __LINE__ gap 4944..5107 (sithThing_ParseThingArg -> sithThing_SyncThing); Jedi Knight has it at the
   same spot (0x004CF3C0, before SyncThing). Body from the Indy debug build (0x00448B41; caller sithWorld_CalcWorldChecksum):
   two switches (moveType, then type) hashing 4-byte fields through stdUtil_CalcChecksum (debug 0x004EE323). */
uint32_t J3DAPI sithThing_CalcThingChecksum(const SithThing* pTemplate, uint32_t seed)
{
    uint32_t checksum;

    checksum = seed;
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->flags, sizeof(pTemplate->flags), checksum);
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->type, sizeof(pTemplate->type), checksum);
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveType, sizeof(pTemplate->moveType), checksum);
    checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->controlType, sizeof(pTemplate->controlType), checksum);

    switch ( pTemplate->moveType )
    {
        case SITH_MT_PHYSICS:
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.flags, sizeof(pTemplate->moveInfo.physics.flags), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.airDrag, sizeof(pTemplate->moveInfo.physics.airDrag), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.surfDrag, sizeof(pTemplate->moveInfo.physics.surfDrag), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.staticDrag, sizeof(pTemplate->moveInfo.physics.staticDrag), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.mass, sizeof(pTemplate->moveInfo.physics.mass), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->moveInfo.physics.height, sizeof(pTemplate->moveInfo.physics.height), checksum);
            break;
    }

    switch ( pTemplate->type )
    {
        case SITH_THING_ACTOR:
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.flags, sizeof(pTemplate->thingInfo.actorInfo.flags), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.health, sizeof(pTemplate->thingInfo.actorInfo.health), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxHealth, sizeof(pTemplate->thingInfo.actorInfo.maxHealth), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.jumpSpeed, sizeof(pTemplate->thingInfo.actorInfo.jumpSpeed), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxThrust, sizeof(pTemplate->thingInfo.actorInfo.maxThrust), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxRotVelocity, sizeof(pTemplate->thingInfo.actorInfo.maxRotVelocity), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxHeadVelocity, sizeof(pTemplate->thingInfo.actorInfo.maxHeadVelocity), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.actorInfo.maxHeadYaw, sizeof(pTemplate->thingInfo.actorInfo.maxHeadYaw), checksum);
            break;

        case SITH_THING_WEAPON:
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.flags, sizeof(pTemplate->thingInfo.weaponInfo.flags), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.damage, sizeof(pTemplate->thingInfo.weaponInfo.damage), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.minDamage, sizeof(pTemplate->thingInfo.weaponInfo.minDamage), checksum);
            checksum = stdUtil_CalcChecksum((const uint8_t*)&pTemplate->thingInfo.weaponInfo.range, sizeof(pTemplate->thingInfo.weaponInfo.range), checksum);
            break;
    }

    return checksum;
}

void J3DAPI sithThing_SyncThing(SithThing* pThing, SithThingSyncFlags flags)
{
    size_t i;

    if ( !sithMessage_outputstream )
    {
        return;
    }

    for ( i = 0; i < sithThing_numUnsyncedThings; ++i )
    {
        if ( sithThing_apUnsyncedThings[i] == pThing )
        {
            sithThing_aSyncFlags[i] |= flags;
            return;
        }
    }

    if ( sithThing_numUnsyncedThings == STD_ARRAYLEN(sithThing_apUnsyncedThings) )
    {
#line 5107
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many modified things in a single frame!\n", 0, 0, 0, 0);
    }
    else
    {
        sithThing_apUnsyncedThings[sithThing_numUnsyncedThings] = pThing;
        sithThing_aSyncFlags[sithThing_numUnsyncedThings++] = flags;
    }
}

void J3DAPI sithThing_SyncThings(void)
{
    size_t i;

    for ( i = 0; i < sithThing_numUnsyncedThings; ++i )
    {
        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_MOVEPOS) != 0 )
        {
            sithDSSThing_MovePos(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, DPSEND_GUARANTEED);
        }

        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_FULL) != 0 )
        {
            sithDSSThing_FullDescription(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            return;
        }

        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_STATE) != 0 )
        {
            sithDSSThing_UpdateState(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }

        if ( (sithThing_aSyncFlags[i] & SITHTHING_SYNC_POS) != 0 )
        {
            sithDSSThing_Pos(sithThing_apUnsyncedThings[i], SITHMESSAGE_SENDTOJOINEDPLAYERS, 0x00);
        }
    }

    sithThing_numUnsyncedThings = 0;
}

int J3DAPI sithThing_CanSync(const SithThing* pThing)
{
    if ( pThing->type == SITH_THING_FREE )
    {
        return 0;
    }

    if ( pThing->msecLifeLeft && (pThing->type == SITH_THING_DEBRIS || pThing->type == SITH_THING_PARTICLE) )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithThing_GetThingMeshIndex(const SithThing* pThing, const char* pMeshName)
{
    size_t i;
    int found = 0;

    if ( !pThing->renderData.data.pModel3 || !pMeshName )
    {
        return -1;
    }

    for ( i = 0; i < pThing->renderData.data.pModel3->aGeos[0].numMeshes; ++i )
    {
        if ( streq(pMeshName, pThing->renderData.data.pModel3->aGeos[0].aMeshes[i].name) ) // TODO: Why using streq? Note, sithThing_GetThingJointIndex is using streqi
        {
            found = 1;
            break;
        }
    }

    if ( found )
    {
        return i;
    }

    return -1;
}

int J3DAPI sithThing_GetThingJointIndex(const SithThing* pThing, const char* pJointName)
{
    size_t i;
    int found = 0;

    if ( !pThing->renderData.data.pModel3 || !pJointName )
    {
        return -1;
    }

    for ( i = 0; i < pThing->renderData.data.pModel3->numHNodes; ++i )
    {
        if ( streqi(pJointName, pThing->renderData.data.pModel3->aHierarchyNodes[i].aName) )
        {
            found = 1;
            break;
        }
    }

    if ( found )
    {
        return i;
    }

    return -1;
}

void J3DAPI sithThing_FreeThingIndex(SithWorld* pWorld, size_t thingNum)
{
#line 5251
    SITH_ASSERTREL((thingNum < SITHTHING_MAXTHINGS));
    SITH_ASSERTREL(sithThing_numFreeThings < SITHTHING_MAXTHINGS);

    // Update last used thing index
    if ( thingNum == pWorld->lastThingIdx )
    {
        int i;
        for ( i = thingNum - 1; i >= 0; --i )
        {
            if ( pWorld->aThings[i].type != SITH_THING_FREE )
            {
                break;
            }
        }

        pWorld->lastThingIdx = i;
    }

    sithThing_aFreeThingIdxs[sithThing_numFreeThings++] = thingNum;
}

int J3DAPI sithThing_GetFreeThingIndex(void)
{
    size_t freeThingIndex;

    if ( !sithThing_numFreeThings )
    {
#line 5291
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning: out of object space - Create failure.\n", 0, 0, 0, 0);
        return -1;
    }

    freeThingIndex = sithThing_aFreeThingIdxs[--sithThing_numFreeThings];
#line 5297
    SITH_ASSERTREL((freeThingIndex < sithWorld_pCurrentWorld->numThings));
    if ( (int)freeThingIndex > sithWorld_pCurrentWorld->lastThingIdx )
    {
        sithWorld_pCurrentWorld->lastThingIdx = freeThingIndex;
    }

    return freeThingIndex;
}

int J3DAPI sithThing_ParseThingPlacement(SithWorld* pWorld)
{
    SithThing* pNewThing;
    SithThing* pTemplate;
    rdVector3 pos[1]; // retail/debug assert text passes `pos`; the debug frame has no pointer slot (0x60 = the 12-byte vector)
    rdVector3 pyr;
    rdMatrix34 orient;
    int secNum;
    SithSector* pSector;

    int thingNum = atoi(stdConffile_entry.aArgs[0].argValue);
    int curArg;

#line 5354
    SITH_ASSERTREL((thingNum >= 0) && (thingNum < (int)pWorld->numThings));

    pNewThing = &pWorld->aThings[thingNum];
    SITH_ASSERTREL(pNewThing->type == SITH_THING_FREE);

    pTemplate = sithTemplate_GetTemplate(stdConffile_entry.aArgs[1].argValue);
    if ( !pTemplate && !streq(stdConffile_entry.aArgs[1].argValue, "none") ) {
#line 5364
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Template %s not found, line %d.\n", stdConffile_entry.aArgs[1].argValue, stdConffile_GetLineNumber(), 0, 0);
    }

    if ( stdConffile_entry.numArgs < 10u )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Not enough fields on line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
        return 1;
    }

    
    pos->x = atof(stdConffile_entry.aArgs[3].argValue);
    pos->y = atof(stdConffile_entry.aArgs[4].argValue);
    pos->z = atof(stdConffile_entry.aArgs[5].argValue);

    
    pyr.x = atof(stdConffile_entry.aArgs[6].argValue);
    pyr.y = atof(stdConffile_entry.aArgs[7].argValue);
    pyr.z = atof(stdConffile_entry.aArgs[8].argValue);

    
    rdMatrix_BuildRotate34(&orient, &pyr);

    secNum = atoi(stdConffile_entry.aArgs[9].argValue);
    if ( secNum < 0 || secNum >= (int)pWorld->numSectors )
    {
#line 5391
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid sector number %d specified for thing on line %d.\n", secNum, stdConffile_GetLineNumber(), 0, 0);
        return 1;
    }

    pSector = sithSector_GetSectorEx(pWorld, secNum);
    SITH_ASSERTREL(sithSector_ValidateSectorPointer(pWorld, pSector));

    // Set thing based on template
    sithThing_SetThingBasedOn(pNewThing, pTemplate);

#line 5402
    SITH_ASSERTREL(sithIntersect_IsSphereInSector(pWorld, pos, ((float)0.0), pSector));
    sithThing_SetPositionAndOrient(pNewThing, pos, &orient);
    sithThing_EnterSector(pNewThing, pSector, 1, 1);
    sithThing_Initialize(pWorld, pNewThing, 1);

    pNewThing->guid      = pNewThing->idx; // Note, static thing guid is same as thing index
    pNewThing->signature = sithThing_curSignature++;

    for ( curArg = 10; curArg < (int)stdConffile_entry.numArgs; ++curArg )
    {
        if ( sithThing_ParseArg(&stdConffile_entry.aArgs[curArg], pWorld, pNewThing) )
        {
            // Error
            return 1;
        }
    }

    if ( pNewThing->perfLevel > sithGetPerformanceLevel() )
    {
        sithThing_FreeThing(pWorld, pNewThing);
        return 0;
    }

#line 5433
    SITH_ASSERTREL(pNewThing != NULL); // TODO: Why this check here???
    strncpy(pNewThing->aName, stdConffile_entry.aArgs[2].argValue, STD_ARRAYLEN(pNewThing->aName) - 1);
    pNewThing->aName[STD_ARRAYLEN(pNewThing->aName) - 1] = '\0';
    return 0;
}

int J3DAPI sithThing_CreateQuetzUserBlock(SithThing* pThing)
{
    SithThing* pPrevThing;
    SithThing* pAttachedThing;
    SithQuetzUserBlock* pQuetz;
    float ratio;
    size_t currentattCnt;

    if ( !pThing->userblock.pQuetz )
    {
#line 5459
        pThing->userblock.pQuetz = (SithQuetzUserBlock*)STDMALLOC(sizeof(SithQuetzUserBlock));
        if ( !pThing->userblock.pQuetz )
        {
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "sithThing_CreateQuetzUserBlock failed to allocate.\n", 0, 0, 0, 0);
            return 0;
        }

        pQuetz = pThing->userblock.pQuetz;
        memset(pQuetz, 0, sizeof(SithQuetzUserBlock));

        pQuetz->strike.mode = 0;
        pQuetz->strike.numSnapShots = 0;

        currentattCnt = 0;
        pPrevThing = pThing;
        for ( pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
        {
            if ( (pAttachedThing->attach.flags & SITH_ATTACH_TAIL) == 0 )
            {
                continue;
            }

            pQuetz->aAttachInfos[currentattCnt].unknown0 = -0.045f;
            pQuetz->aAttachInfos[currentattCnt].unknown2 = -0.045f;
            memset(&pQuetz->aAttachInfos[currentattCnt].vecUnknown11, 0, sizeof(pQuetz->aAttachInfos[currentattCnt].vecUnknown11));

            if ( !currentattCnt )
            {
                pQuetz->aAttachInfos[currentattCnt].unknown1           = pAttachedThing->attach.posOffset.z - 0.02f;
                pQuetz->aAttachInfos[currentattCnt].unknown0           = 0.0f;
                pQuetz->aAttachInfos[currentattCnt].vecUnknown11.pitch = 40.0f;
                pQuetz->aAttachInfos[currentattCnt].unknown14          = 0.055f;
            }
            else if ( currentattCnt == 1 )
            {
                pQuetz->aAttachInfos[currentattCnt].unknown1       = -pPrevThing->attach.posOffset.z + pAttachedThing->attach.posOffset.z;
                pQuetz->aAttachInfos[currentattCnt].vecUnknown11.x = 25.0f;
                pQuetz->aAttachInfos[currentattCnt].unknown14      = 0.103f;
            }
            else if ( currentattCnt == 2 )
            {
                pQuetz->aAttachInfos[currentattCnt].unknown1       = -pPrevThing->attach.posOffset.z + pAttachedThing->attach.posOffset.z;
                pQuetz->aAttachInfos[currentattCnt].vecUnknown11.x = 10.0f;
                pQuetz->aAttachInfos[currentattCnt].unknown14      = 0.118f;
            }
            else
            {
                pQuetz->aAttachInfos[currentattCnt].unknown1  = -pPrevThing->attach.posOffset.z + pAttachedThing->attach.posOffset.z;
                pQuetz->aAttachInfos[currentattCnt].unknown14 = 0.12f;
            }

            if ( pQuetz->aAttachInfos[currentattCnt].unknown0 + pQuetz->aAttachInfos[currentattCnt].unknown2 < 0.0f )
            {
                ratio = -(pQuetz->aAttachInfos[currentattCnt].unknown0 + pQuetz->aAttachInfos[currentattCnt].unknown2);
            }
            else
            {
                ratio = pQuetz->aAttachInfos[currentattCnt].unknown0 + pQuetz->aAttachInfos[currentattCnt].unknown2;
            }

            pQuetz->aAttachInfos[currentattCnt].unknown9  = ratio;
            pQuetz->aAttachInfos[currentattCnt].unknown10 = pQuetz->aAttachInfos[currentattCnt].unknown9;

            rdVector_Copy3(&pQuetz->aAttachInfos[currentattCnt].vecDirection, &pPrevThing->orient.lvec);
            pQuetz->aAttachInfos[currentattCnt].vecDirection.z = 0.0f;
            rdVector_Normalize3Acc(&pQuetz->aAttachInfos[currentattCnt].vecDirection);

            rdVector_Copy3(&pQuetz->aAttachInfos[currentattCnt].vecDirectionNextAttach, &pAttachedThing->orient.lvec);
            pQuetz->aAttachInfos[currentattCnt].vecDirectionNextAttach.z = 0.0f;
            rdVector_Normalize3Acc(&pQuetz->aAttachInfos[currentattCnt].vecDirectionNextAttach);

            if ( ++currentattCnt >= STD_ARRAYLEN(pQuetz->aAttachInfos) )
            {
                break;
            }

            ++pQuetz->numAttachInfos;
            pPrevThing = pAttachedThing;
        }
    }

    return 1;
}

void J3DAPI sithThing_UpdateQuetzUserBlock(SithThing* pThing)
{
    SithThing* pPrevThing;
    SithThing* pAttachedThing;
    SithSector* pNewSector;
    rdMatrix34 orient;
    rdVector3 vec;
    rdVector3 offset;
    rdVector3 attachOffset;
    rdVector3 newPos;
    rdVector3 moveNorm;
    rdVector3 dir;
    float dot;
    float moveDist;
    float distMoved;
    float amount;
    float ratio;
    float scale;
    float prevAngle;
    rdVector3 velNorm;
    float angle;
    float nextAngle;
    float firstAngle;
    float totalAngle;
    float avgAngle;
    float sinAngle;
    float cosAngle;
    size_t attachNum;
    int bTurnConsistent;
    int bTurnFlipped;

    if ( !sithThing_CreateQuetzUserBlock(pThing) )
    {
        return;
    }

    rdVector_Normalize3(&velNorm, &pThing->moveInfo.physics.velocity);

    dot = rdVector_Dot3(&pThing->orient.lvec, &velNorm);
    dot = STDMATH_CLAMP(dot, 0.0f, 1.0f);
    amount = RD_MULTIPLY(pThing->distanceMoved, dot);

    attachNum    = 0;
    bTurnFlipped = 0;
    pPrevThing   = pThing;

    sithThing_prevQuetzAttachInfo = NULL;
    sithThing_curQuetzAttachInfo  = pThing->userblock.pQuetz->aAttachInfos;

    for ( pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
    {
        rdVector3 prevDir;
        rdVector3 attachDir;
        if ( (pAttachedThing->attach.flags & SITH_ATTACH_TAIL) == 0 )
        {
            continue;
        }

        ratio = (sithThing_curQuetzAttachInfo->unknown10 -= amount) / sithThing_curQuetzAttachInfo->unknown9;
        ratio = STDMATH_CLAMP(ratio, 0.0f, 1.0f);

        vec.x = 0.0f;
        vec.y = sithThing_curQuetzAttachInfo->unknown0;
        vec.z = 0.0f;
        rdMatrix_TransformVector34(&offset, &vec, &pPrevThing->orient);
        offset.z = offset.z + sithThing_curQuetzAttachInfo->unknown1;

        rdVector_Scale3(&dir, &sithThing_curQuetzAttachInfo->vecDirectionNextAttach, ratio);
        rdVector_ScaleAdd3Acc(&dir, &sithThing_curQuetzAttachInfo->vecDirection, 1.0f - ratio);
        rdVector_Normalize3Acc(&dir);

        rdMatrix_BuildFromLook34(&orient, &dir);
        rdMatrix_PreRotate34(&orient, &sithThing_curQuetzAttachInfo->vecUnknown11);
        rdMatrix_Copy34(&pAttachedThing->orient, &orient);

        vec.x = 0.0f;
        vec.y = sithThing_curQuetzAttachInfo->unknown2;
        vec.z = 0.0f;
        rdMatrix_TransformVector34(&attachOffset, &vec, &pAttachedThing->orient);

        rdVector_Add3(&newPos, &offset, &attachOffset);
        rdVector_Add3Acc(&newPos, &pPrevThing->pos);
        rdVector_Sub3(&moveNorm, &newPos, &pAttachedThing->pos);

        rdVector_Scale3(&pAttachedThing->moveInfo.physics.velocity, &moveNorm, sithTime_fps);
        rdVector_Copy3(&pAttachedThing->moveInfo.physics.deltaVelocity, &moveNorm);
        rdVector_Copy3(&pAttachedThing->moveDir, &moveNorm);

        moveDist = D3DVAL(rdVector_Normalize3Acc(&moveNorm));
        if ( moveDist != 0.0f )
        {
            rdVector3 attachPos;
            rdVector_ScaleAdd3(&attachPos, &pAttachedThing->orient.uvec, sithThing_curQuetzAttachInfo->unknown14, &pAttachedThing->pos);
            pNewSector = sithCollision_FindSectorInRadius(pAttachedThing->pInSector, &pAttachedThing->pos, &attachPos, 0.0f);
            rdVector_Copy3(&pAttachedThing->pos, &attachPos);
            sithThing_SetSector(pAttachedThing, pNewSector, /*bNotify=*/1);

            distMoved = sithCollision_MoveThing(pAttachedThing, &moveNorm, moveDist, 4);

            rdVector_ScaleAdd3(&attachPos, &pAttachedThing->orient.uvec, -sithThing_curQuetzAttachInfo->unknown14, &pAttachedThing->pos);
            pNewSector = sithCollision_FindSectorInRadius(pAttachedThing->pInSector, &pAttachedThing->pos, &attachPos, 0.0f);
            rdVector_Copy3(&pAttachedThing->pos, &attachPos);
            sithThing_SetSector(pAttachedThing, pNewSector, /*bNotify=*/1);
        }

        if ( ratio <= sithThing_quetzStretchMinRatio
            || (!sithThing_prevQuetzAttachInfo && pThing->userblock.pQuetz->unknown391)
            || bTurnFlipped )
        {
            pThing->userblock.pQuetz->unknown391 = 0;

            prevAngle = rdMath_DeltaAngleNormalized(&sithThing_curQuetzAttachInfo->vecDirection, &sithThing_curQuetzAttachInfo->vecDirectionNextAttach, &rdroid_zVector3);

            rdVector_Copy3(&sithThing_curQuetzAttachInfo->vecDirection, &pPrevThing->orient.lvec);
            sithThing_curQuetzAttachInfo->vecDirection.z = 0.0f;
            rdVector_Normalize3Acc(&sithThing_curQuetzAttachInfo->vecDirection);

            rdVector_Copy3(&sithThing_curQuetzAttachInfo->vecDirectionNextAttach, &pAttachedThing->orient.lvec);
            sithThing_curQuetzAttachInfo->vecDirectionNextAttach.z = 0.0f;
            rdVector_Normalize3Acc(&sithThing_curQuetzAttachInfo->vecDirectionNextAttach);

            angle = rdMath_DeltaAngleNormalized(&sithThing_curQuetzAttachInfo->vecDirection, &sithThing_curQuetzAttachInfo->vecDirectionNextAttach, &rdroid_zVector3);

            if ( (prevAngle < 0.0f ? -1 : 1) != (angle < 0.0f ? -1 : 1) )
            {
                bTurnFlipped = 1;
            }
            else
            {
                bTurnFlipped = 0;
            }

            if ( sithThing_prevQuetzAttachInfo )
            {
                stdMath_SinCos(sithThing_prevQuetzAttachInfo->vecUnknown11.x, &sinAngle, &cosAngle);
                sithThing_curQuetzAttachInfo->unknown9 = RD_MULTIPLY(SITHTHING_ABS(sithThing_curQuetzAttachInfo->unknown2), cosAngle);
            }
            else
            {
                sithThing_curQuetzAttachInfo->unknown9 = sithThing_curQuetzAttachInfo->unknown0 < 0.0f ? -sithThing_curQuetzAttachInfo->unknown0 : sithThing_curQuetzAttachInfo->unknown0;
            }

            stdMath_SinCos(sithThing_curQuetzAttachInfo->vecUnknown11.x, &sinAngle, &cosAngle);
            sithThing_curQuetzAttachInfo->unknown9 = RD_MULTIPLY(SITHTHING_ABS(sithThing_curQuetzAttachInfo->unknown2), cosAngle) + sithThing_curQuetzAttachInfo->unknown9;

            if ( sithThing_dword_5612B8 )
            {
                scale = RD_MULTIPLY((float)attachNum, sithThing_quetzScalePerAttach) + sithThing_quetzScaleBase;
                sithThing_curQuetzAttachInfo->unknown9 = RD_MULTIPLY(sithThing_curQuetzAttachInfo->unknown9, scale);
            }

            sithThing_curQuetzAttachInfo->unknown10 = sithThing_curQuetzAttachInfo->unknown9;
        }

        prevDir.x = pPrevThing->orient.lvec.x;
        prevDir.y = pPrevThing->orient.lvec.y;
        prevDir.z = 0.0f;
        rdVector_Normalize3Acc(&prevDir);

        attachDir.x = pAttachedThing->orient.lvec.x;
        attachDir.y = pAttachedThing->orient.lvec.y;
        attachDir.z = 0.0f;
        rdVector_Normalize3Acc(&attachDir);

        if ( !sithThing_prevQuetzAttachInfo )
        {
            firstAngle      = rdMath_DeltaAngleNormalized(&prevDir, &attachDir, &rdroid_zVector3);
            totalAngle      = firstAngle;
            bTurnConsistent = 1;
        }
        else
        {
            nextAngle  = rdMath_DeltaAngleNormalized(&prevDir, &attachDir, &rdroid_zVector3);
            totalAngle = totalAngle + nextAngle;
            if ( (firstAngle < 0.0f ? -1 : 1) != (nextAngle < 0.0f ? -1 : 1) )
            {
                bTurnConsistent = 0;
            }
        }

        pPrevThing = pAttachedThing;
        sithThing_prevQuetzAttachInfo = sithThing_curQuetzAttachInfo;
        sithThing_curQuetzAttachInfo++;
        attachNum++;
    }

    sithThing_dword_5612B8 = 0;
    if ( bTurnConsistent )
    {
        avgAngle = SITHTHING_ABS(totalAngle / (float)pThing->userblock.pQuetz->numAttachInfos);
        if ( avgAngle > sithThing_quetzMaxTurnRate )
        {
            sithThing_dword_5612B8 = 1;
        }
    }
}
