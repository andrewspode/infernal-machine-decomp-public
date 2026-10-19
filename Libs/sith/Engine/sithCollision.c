#include "sithCollision.h"

#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <sith/Cog/sithCog.h>
#ifdef J3D_DEBUG
#include <sith/Devices/sithConsole.h>
#endif
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithItem.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdUtil.h>

#define SITHCOLLISION_STACKSIZE 4
#define SITHCOLLISION_ABS(x) ((x) < 0.0f ? -(x) : (x))

static int stackLevel = -1;

static SithCollideResult aCollideResults[SITH_THING_NUMTYPES][SITH_THING_NUMTYPES];
static CollisionHandlerSurface aThingSurfaceCollideResults[SITH_THING_NUMTYPES];

static size_t aNumSectors[SITHCOLLISION_STACKSIZE];
static SithSector* apSearchedSectors[SITHCOLLISION_STACKSIZE][256];

static size_t aNumStackEntries[SITHCOLLISION_STACKSIZE];
static SithCollision aCollisions[SITHCOLLISION_STACKSIZE][512];

static int bCollideStartup;

SithSurface* J3DAPI sithCollision_RaycastSector(SithSector* pStartSector, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, SithCollisionType* hitType);

static __inline SithCollision* J3DAPI sithCollision_PopClosest(void);

int sithCollision_dword_17F1090;
rdVector3 sithCollision_vec17F10A0;

static __inline void J3DAPI sithCollision_ResetCurStack(void);
static __inline float J3DAPI sithCollision_BuildCollisionList(SithSector* pSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags);
static __inline int J3DAPI sithCollision_CheckSectorSearched(const SithSector* pSector);
static __inline int J3DAPI sithCollision_AddSearchedSector(SithSector* pSector);
static __inline int J3DAPI sithCollision_IsThingCollidable(const SithThing* pThing, int searchFlags);
static __inline int J3DAPI sithCollision_CanThingsCollide(const SithThing* pThing1, const SithThing* pThing2, int collflags);
static __inline int J3DAPI sithCollision_CheckSearchFilter(SithThing* pThing1, SithThing* pThing2);
static __inline int J3DAPI sithCollision_CanPassAdjoin(SithSurfaceAdjoinFlag adjflags, int colflags);

void J3DAPI sithCollision_Startup(void)
{
    if ( bCollideStartup )
    {
#line 140
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Collide system already started.. chill.\n", 0, 0, 0, 0);
        return;
    }

    memset(aCollideResults, 0, sizeof(aCollideResults));
    memset(aThingSurfaceCollideResults, 0, sizeof(aThingSurfaceCollideResults));

    sithCollision_AddCollisionHandler(SITH_THING_ACTOR, SITH_THING_ACTOR, sithActor_ActorCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_ACTOR, SITH_THING_PLAYER, sithActor_ActorCollisionHandler, sithCollision_ActorPlayerSearchFilter);

    sithCollision_AddCollisionHandler(SITH_THING_PLAYER, SITH_THING_PLAYER, sithCollision_ThingCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_PLAYER, SITH_THING_COG, sithCollision_ThingCollisionHandler, 0);

    sithCollision_AddCollisionHandler(SITH_THING_ACTOR, SITH_THING_COG, sithActor_ActorCollisionHandler, 0);

    sithCollision_AddCollisionHandler(SITH_THING_DEBRIS, SITH_THING_ACTOR, sithCollision_ParticleAndActorCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_DEBRIS, SITH_THING_PLAYER, sithCollision_ParticleAndActorCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_DEBRIS, SITH_THING_DEBRIS, sithCollision_ThingCollisionHandler, 0);

    sithCollision_AddCollisionHandler(SITH_THING_WEAPON, SITH_THING_ACTOR, sithWeapon_ThingCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_WEAPON, SITH_THING_PLAYER, sithWeapon_ThingCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_WEAPON, SITH_THING_DEBRIS, sithWeapon_ThingCollisionHandler, 0);
    sithCollision_AddCollisionHandler(SITH_THING_WEAPON, SITH_THING_COG, sithWeapon_ThingCollisionHandler, 0);

    sithCollision_AddCollisionHandler(SITH_THING_ITEM, SITH_THING_PLAYER, sithItem_PlayerCollisionHandler, 0);

    sithCollision_AddSurfaceCollisionHandler(SITH_THING_ACTOR, sithActor_SurfaceCollisionHandler);
    sithCollision_AddSurfaceCollisionHandler(SITH_THING_WEAPON, sithWeapon_SurfaceCollisionHandler);

    sithCollision_AddCollisionHandler(SITH_THING_COG, SITH_THING_COG, sithCollision_ThingCollisionHandler, 0);

    bCollideStartup = 1;
}

void J3DAPI sithCollision_Shutdown(void)
{
    if ( !bCollideStartup )
    {
#line 195
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Collide system not started.\n", 0, 0, 0, 0);
        return;
    }

    bCollideStartup = 0;
}

void J3DAPI sithCollision_AddCollisionHandler(SithThingType type1, SithThingType type2, CollisionHandlerThing pProcessFunc, int (J3DAPI* pSearchFunc)(SithThing*, SithThing*))
{
#line 229
    SITH_ASSERTREL((type1 >= SITH_THING_FREE) && (type1 < SITH_THING_NUMTYPES));
#line 230
    SITH_ASSERTREL((type2 >= SITH_THING_FREE) && (type2 < SITH_THING_NUMTYPES));
#line 231
    SITH_ASSERTREL(pProcessFunc != NULL);

    if ( aCollideResults[type1][type2].pProcessFunc || aCollideResults[type2][type1].pProcessFunc )
    {
#line 239
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning- overriding previous collision handler for types %d and %d.\n", type1, type2, 0, 0);
    }

    aCollideResults[type1][type2].pProcessFunc = pProcessFunc;
    aCollideResults[type1][type2].pSearchFunc = pSearchFunc;
    aCollideResults[type1][type2].bDifferentTypHandler = 0;
    if ( type1 != type2 )
    {
        aCollideResults[type2][type1].pProcessFunc = pProcessFunc;
        aCollideResults[type2][type1].pSearchFunc = pSearchFunc;
        aCollideResults[type2][type1].bDifferentTypHandler = 1;
    }
}

void J3DAPI sithCollision_AddSurfaceCollisionHandler(SithThingType type, CollisionHandlerSurface fpHandler)
{
#line 277
    SITH_ASSERTREL((type >= SITH_THING_FREE) && (type < SITH_THING_NUMTYPES));
    if ( aThingSurfaceCollideResults[type] )
    {
#line 284
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Warning- overriding previous collision handler for type %d.\n", type, 0, 0, 0);
    }

    aThingSurfaceCollideResults[type] = fpHandler;
}

/* Linker-discarded (no retail caller; the debug build has no caller either). Body from Indy's debug build
   (orig/Indy3D_debug.exe 0x4D57D8, /Od-identical modulo local slots). Name is OpenJKDF2's for the
   same body. Its asserts/prints sit at debug lines 319/323, inside the retail __LINE__ gap 284..373. */
SithSurface* J3DAPI sithCollision_RaycastFromCamera(const rdVector2* pScreenPos)
{
    rdCamera* pCamera;
    rdVector3 dest;
    rdVector3 moveNorm;
    SithCollisionType hitType;

    pCamera = &sithCamera_pCurCamera->rdCamera;
#line 319
    SITH_ASSERTREL(pCamera && pCamera->pCanvas);
    if ( !sithCamera_pCurCamera->pSector )
    {
#line 323
        stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Unable to process query, don't have a valid camera sector right now.\n", 0, 0, 0, 0);
        return NULL;
    }

    dest.x = pScreenPos->x;
    dest.z = -pScreenPos->y;
    dest.y = pCamera->focalLength;
    dest.x -= pCamera->pCanvas->center.x;
    dest.z += pCamera->pCanvas->center.y;
    rdMatrix_TransformPoint34Acc(&dest, &sithCamera_pCurCamera->orient);
    rdVector_Sub3(&moveNorm, &dest, &sithCamera_pCurCamera->orient.dvec);
    rdVector_Normalize3Acc(&moveNorm);
    return sithCollision_RaycastSector(sithCamera_pCurCamera->pSector, &sithCamera_pCurCamera->orient.dvec, &moveNorm, 200.0f, 0.0f, &hitType);
}

SithCollision* J3DAPI sithCollision_PopStack(void)
{
    SithCollision* pCollision;

#line 373
    SITH_ASSERTREL(stackLevel >= 0);
#line 374
    SITH_ASSERTREL(stackLevel < SITHCOLLISION_STACKSIZE);
    pCollision = sithCollision_PopClosest();
    if ( pCollision )
    {
        pCollision->bEnumerated = 1;
    }
    else
    {
        sithCollision_ResetCurStack();
    }

    return pCollision;
}

SithSector* J3DAPI sithCollision_FindSectorInRadius(SithSector* pStartSector, const rdVector3* startPos, rdVector3* endPos, float radius)
{
    SithSector* sector;
    float moveDist;
    SithCollision* pCollision;
    rdVector3 moveNorm;

#line 422
    SITH_ASSERTREL(startPos && endPos && pStartSector);
    if ( sithIntersect_IsSphereInSector(sithWorld_pCurrentWorld, endPos, 0.0f, pStartSector) )
    {
        return pStartSector; // Shall returned sector be null here?
    }

    rdVector_Sub3(&moveNorm, endPos, startPos);
    moveDist = rdVector_Normalize3Acc(&moveNorm);

    sector = pStartSector;
    sithCollision_SearchForCollisions(pStartSector, NULL, startPos, &moveNorm, moveDist, radius, 1);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {

        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
#line 442
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin != NULL);
            sector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
        }
        else
        {
            rdVector_ScaleAdd3(endPos, &moveNorm, pCollision->distance, startPos);
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return sector;
}

SithSector* J3DAPI sithCollision_FindSectorAtThing(SithThing* pThing, SithSector* pStartSector, const rdVector3* startPos, rdVector3* endPos, float radius)
{
    SithSector* sector;
    float moveDist;
    SithCollision* pCollision;
    rdVector3 moveNorm;

#line 494
    SITH_ASSERTREL(startPos && endPos && pStartSector);
    rdVector_Sub3(&moveNorm, endPos, startPos);
    moveDist = rdVector_Normalize3Acc(&moveNorm);

    sector = pStartSector;
    sithCollision_SearchForCollisions(pStartSector, NULL, startPos, &moveNorm, moveDist, radius, 0);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {

        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_NOPLAYERMOVE) == 0
            && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
#line 512
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin != NULL);
            sector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
        }
        else
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided == pThing )
            {
                continue;
            }

            rdVector_ScaleAdd3(endPos, &moveNorm, pCollision->distance - 0.001f, startPos);
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return sector;
}

SithSector* J3DAPI sithCollision_FindWaterSector(SithSector* pStartSector, rdVector3* startPos, rdVector3* endPos, float radius)
{
    SithSector* sector;
    float moveDist;
    SithCollision* pCollision;
    rdVector3 moveNorm;

#line 569
    SITH_ASSERTREL(startPos && endPos && pStartSector);
    if ( sithIntersect_IsSphereInSector(sithWorld_pCurrentWorld, endPos, 0.0f, pStartSector) )
    {
        return pStartSector;
    }

    rdVector_Sub3(&moveNorm, endPos, startPos);
    moveDist = rdVector_Normalize3Acc(&moveNorm);

    sector = pStartSector;
    sithCollision_SearchForCollisions(pStartSector, NULL, startPos, &moveNorm, moveDist, radius, 1);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {

        if ( (pCollision->type & (SITHCOLLISION_ADJOINCROSS | SITHCOLLISION_ADJOINTOUCH)) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
            && (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
        {
            rdVector_ScaleAdd3(endPos, &moveNorm, pCollision->distance, startPos);
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
#line 598
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin != NULL);
            sector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
        }
        else // Note, here will return sector which might not be water sector for any collision it finds but adjoin collision
        {
            rdVector_ScaleAdd3(endPos, &moveNorm, pCollision->distance, startPos);
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return sector;
}

/* Retail's linker discarded this function (no retail symbol, address or caller). Its evidence is the
   /Gf literal order: "pCollision->pSurfaceCollided != ((void *)0)" (0x0053B444) is emitted right after
   the FindSector functions' "startPos && endPos && pStartSector" and ahead of MoveThing's literals,
   although retail references it only from MoveThing and CheckDistance; MoveThing alone creates it
   between its "aCollideResults" and "pMirrorAdjoin" asserts. So a function between FindWaterSector and
   MoveThing asserted it first, and the retail __LINE__ anchors leave +92 unexplained lines between
   FindWaterSector (598) and HasLOS (787). The debug build's sub_4D5E37 asserts
   exactly this, and its debug address sits immediately before sub_4D5EB3, which falls in
   that gap. Parameters are inferred; the name is OpenJKDF2's for the same raycast.
   Body: the debug build's sub_4D5E37; Jedi Knight's RaycastSector has the
   same shape (search 0x103, pop, hit type, release the stack level), and the restored lines fill the
   598..787 __LINE__ room (the FallHurt precedent). Linker-discarded in retail. */
SithSurface* J3DAPI sithCollision_RaycastSector(SithSector* pStartSector, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, SithCollisionType* hitType)
{
    SithCollision* pCollision;

    sithCollision_SearchForCollisions(pStartSector, NULL, startPos, moveNorm, moveDist, radius, 0x103);
    pCollision = sithCollision_PopStack();
    if ( pCollision )
    {
#line 645
        SITH_ASSERTREL(pCollision->pSurfaceCollided != NULL);
        *hitType = pCollision->type;
        sithCollision_DecreaseStackLevel();
        return pCollision->pSurfaceCollided;
    }

    sithCollision_DecreaseStackLevel();
    return NULL;
}

/* Linker-discarded: retail __LINE__ gap 598..787 (FindWaterSector -> HasLOS), where the original
   defined the debug build's sub_4D5EB3 right after sub_4D5E37 (adjacent debug addresses). Name is OpenJKDF2's for the
   same function. Body: the debug build's sub_4D5EB3; Jedi Knight's CheckPathClear has the same shape
   (direction and length from endPos - startPos, search 0x12A, clear when nothing is popped). */
int J3DAPI sithCollision_CheckPathClear(SithSector* pStartSector, const rdVector3* startPos, const rdVector3* endPos, float radius)
{
    rdVector3 moveNorm;
    float moveDist;
    SithCollision* pCollision;
    SithThing* pHitThing = NULL;
    SithSurface* pHitSurface = NULL;

    moveNorm.x = endPos->x - startPos->x;
    moveNorm.y = endPos->y - startPos->y;
    moveNorm.z = endPos->z - startPos->z;
    moveDist = rdVector_Normalize3Acc(&moveNorm);
    sithCollision_SearchForCollisions(pStartSector, NULL, startPos, &moveNorm, moveDist, radius, 0x12A);
    pCollision = sithCollision_PopStack();
    if ( pCollision )
    {
        sithCollision_DecreaseStackLevel();
        return 0;
    }
    else
    {
        sithCollision_DecreaseStackLevel();
        return 1;
    }
}

int J3DAPI sithCollision_CanMoveToPos(SithSector* pStartSector, const rdVector3* startPos, const rdVector3* endPos, float radius)
{
    SithThing* pImpactObj = NULL;
    SithSurface* pCollisionFace = NULL;
    SithCollision* pCurCollision;
    int bCanMove;
    rdVector3 normMove;
    float moveDistance;

    bCanMove = 1;
    rdVector_Sub3(&normMove, endPos, startPos);
    moveDistance = rdVector_Normalize3Acc(&normMove);

    sithCollision_SearchForCollisions(pStartSector, NULL, startPos, &normMove, moveDistance, radius, 0x4120);
    while ( (pCurCollision = sithCollision_PopStack()) != NULL )
    {

        if ( (pCurCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && (pCurCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            continue;
        }

        bCanMove = 0;
        break;
    }

    sithCollision_DecreaseStackLevel();
    return bCanMove;
}

int J3DAPI sithCollision_HasLOS(const SithThing* pViewer, const SithThing* pTarget, int flag)
{
    SithThing* pHitObject = NULL;
    SithSurface* pCollisionFace = NULL;
    SithCollision* pCollision;
    int bCurLos;
    rdVector3 moveNormal;
    float distance;
    int flags;

    bCurLos = 1;
    flags = 0x6120;
#line 787
    SITH_ASSERTREL(pViewer && pTarget);
    if ( flag )
    {
        flags &= ~0x100;
    }

    rdVector_Sub3(&moveNormal, &pTarget->pos, &pViewer->pos);
    distance = rdVector_Normalize3Acc(&moveNormal);

    sithCollision_SearchForCollisions(pViewer->pInSector, NULL, &pViewer->pos, &moveNormal, distance, 0.0f, flags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            if ( pCollision->pThingCollided == pTarget )
            {
                bCurLos = 1;
                break;
            }

            if ( pCollision->pThingCollided == pViewer )
            {
                continue;
            }

            bCurLos = 0;
            break;
        }

        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 && (pCollision->pSurfaceCollided->pAdjoin->flags & SITH_ADJOIN_MOVE) != 0 )
        {
            continue;
        }

        bCurLos = 0;
        break;
    }

    sithCollision_DecreaseStackLevel();
    return bCurLos;
}

void J3DAPI sithCollision_RotateThing(SithThing* pThing, const rdMatrix34* pOrient)
{
    rdVector3 dir;
    float moveDist;
    SithThing* pAttachedThing;
    rdVector3 moveNorm;

    moveDist = 0.0f;
    rdMatrix_PreMultiply34(&pThing->orient, pOrient);

    for ( pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing->pNextAttachedThing )
    {
        rdVector_Sub3(&dir, &pAttachedThing->pos, &pThing->pos);
        memcpy(&pAttachedThing->orient.dvec, &dir, sizeof(pAttachedThing->orient.dvec));
        sithCollision_RotateThing(pAttachedThing, pOrient);

        if ( (pAttachedThing->attach.flags & SITH_ATTACH_NOMOVE) == 0 )
        {
            rdVector_Sub3(&moveNorm, &pAttachedThing->orient.dvec, &dir);
            if ( moveNorm.x != 0.0f || moveNorm.y != 0.0f || moveNorm.z != 0.0f )
            {
                moveDist = rdVector_Normalize3Acc(&moveNorm);
                sithCollision_MoveThing(pAttachedThing, &moveNorm, moveDist, 0);
            }
        }

        memset(&pAttachedThing->orient.dvec, 0, sizeof(pAttachedThing->orient.dvec));
    }
}

void J3DAPI sithCollision_ApplyThingOrient(SithThing* pThing, rdMatrix34* pOrient)
{
    SithThing* pAttachedThing;
    rdVector3 moveNorm;
    rdMatrix34 orient;
    float moveDist;

    if ( pThing->pAttachedThing )
    {
        SithThing* pNextAttachedThing;
        rdMatrix34 mat;

        rdMatrix_Normalize34(pOrient);
        rdVector_Copy3(&pOrient->dvec, &pThing->pos);
        rdVector_Copy3(&pThing->orient.dvec, &pThing->pos);

        rdMatrix_InvertOrtho34(&orient, &pThing->orient);

        for ( pAttachedThing = pThing->pAttachedThing; pAttachedThing; pAttachedThing = pAttachedThing = pNextAttachedThing )
        {
            pNextAttachedThing = pAttachedThing->pNextAttachedThing;

            rdVector_Copy3(&pAttachedThing->orient.dvec, &pAttachedThing->pos);

            rdMatrix_Multiply34(&mat, &orient, &pAttachedThing->orient);
            rdMatrix_PostMultiply34(&mat, pOrient);

            rdVector_Sub3(&moveNorm, &mat.dvec, &pAttachedThing->pos);
            moveDist = rdVector_Normalize3Acc(&moveNorm);

            // Clear out position vector
            memset(&mat.dvec, 0, sizeof(mat.dvec));

            if ( moveDist != 0.0f )
            {
                sithCollision_MoveThing(pAttachedThing, &moveNorm, moveDist, 0x40);
            }

            sithCollision_ApplyThingOrient(pAttachedThing, &mat);
            if ( pAttachedThing->moveType == SITH_MT_PHYSICS )
            {
                pAttachedThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNED;
            }
        }
    }
    else if ( ((pThing->idx + sithMain_frameNumber) & 7) == 0 ) // On every 8th frame normalize the orient matrix
    {
        rdMatrix_Normalize34(pOrient);
    }

    memset(&pOrient->dvec, 0, sizeof(pOrient->dvec));
    memcpy(&pThing->orient, pOrient, sizeof(pThing->orient));
}





float J3DAPI sithCollision_MoveThing(SithThing* pThing, const rdVector3* moveNorm, float moveDist, int flags)
{
    SithCollision* pCollision;
    SithThingType type1;
    float v19;
    rdVector3 curthPos;
    int bCollided;
    int moveToResult;
    float searchDist;
    float prevMoveDist;

    rdVector3 dest;
    rdVector3 prevAttPos;
    rdVector3 v40;
    float distMoved;
    SithMoveInfo* pMoveInfo;
    int v43;
    float v44;
    rdVector3 prevPos;
    float v46;
    SithThing* pAttached;
    unsigned int v49;

    rdVector3 moveToPos;
    SithSector* pPrevSector;

    v49 = 0;
    v46 = 0.0f;
    v44 = 0.0f;
    pMoveInfo = &pThing->moveInfo;
#line 1003
    SITH_ASSERTREL(bCollideStartup);
#line 1004
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));

    if ( !(moveDist > 0.0f) )
    {
#line 1009
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "MoveThing called for %s with distance %f.\n", pThing->aName, moveDist, 0, 0);
        return 0.0f;
    }

    rdVector_Copy3(&dest, moveNorm);
    if ( pThing->collide.type == SITH_COLLIDE_NONE )
    {
        flags |= 5u;
    }

    if ( pThing->moveType == SITH_MT_PATH )
    {
        flags |= 0x04;
    }

    if ( (flags & 0x01) == 0 )
    {
        flags |= 0x800;
    }

    if ( pThing->type == SITH_THING_PLAYER )
    {
        flags |= 0x8000;
    }

    for ( pAttached = pThing->pAttachedThing; pAttached; pAttached = pAttached->pNextAttachedThing )
    {
        if ( (pAttached->attach.flags & SITH_ATTACH_NOMOVE) != 0 )
        {
            continue;
        }

        if ( pAttached->type == SITH_THING_PLAYER && pAttached->thingInfo.actorInfo.bForceMovePlay )
        {
            rdVector_Copy3(&prevAttPos, &pAttached->pos);
        }

        distMoved = sithCollision_MoveThing(pAttached, moveNorm, moveDist, 0x40);
        if ( pAttached->type == SITH_THING_PLAYER )
        {
            rdVector3 dAttPos;
            rdVector_Sub3(&dAttPos, &prevAttPos, &pAttached->pos);

            if ( pAttached->thingInfo.actorInfo.bForceMovePlay )
            {
                rdVector_Add3(&pAttached->forceMoveStartPos, &pAttached->forceMoveStartPos, &dAttPos);
            }
        }

        if ( distMoved < moveDist )
        {
            if ( (pAttached->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
            {
                rdVector3 v31;
                float dot;

                rdMatrix_TransformVector34(&v31, &pAttached->attach.pFace->normal, &pThing->orient);
                dot = rdVector_Dot3(&v31, moveNorm);
                if ( stdMath_ClipNearZero(dot) <= 0.0f )
                {
                    continue;
                }
            }

            if ( (pThing->flags & SITH_TF_NOCRUSH) == 0 )
            {
                sithThing_DamageThing(pAttached, pThing, RD_MULTIPLY(moveDist - distMoved, 100.0f), SITH_DAMAGE_IMPACT);
            }

            moveDist = distMoved;
        }
    }

    sithCollision_dword_17F1090 = 0;
    if ( pThing == sithPlayer_pLocalPlayerThing && (pMoveInfo->physics.flags & (SITH_PF_UNKNOWN_8000000 | SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0 )
    {
        switch ( pThing->moveStatus )
        {
            case SITHPLAYERMOVE_WALKING:
            case SITHPLAYERMOVE_RUNNING:
            case SITHPLAYERMOVE_CRAWL_STILL:
            case SITHPLAYERMOVE_UNKNOWN_4:
            case SITHPLAYERMOVE_UNKNOWN_5:
                if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                    break;
                }

                if ( !pThing->attach.flags )
                {
                    break;
                }

                rdVector_Copy3(&prevPos, &pThing->pos);
                if ( pThing->moveStatus == SITHPLAYERMOVE_WALKING || pThing->moveStatus == SITHPLAYERMOVE_RUNNING )
                {
                    rdVector_ScaleAdd3(&moveToPos, moveNorm, pThing->collide.movesize, &prevPos);
                }
                else
                {
                    rdVector_ScaleAdd3(&moveToPos, moveNorm, RD_MULTIPLY(pThing->collide.movesize, 3.0f), &prevPos);
                }

                moveToResult = sithPlayerActions_CanMoveToPos(pThing, &moveToPos, &v43);
                if ( moveToResult == 0 || moveToResult == 3 )
                {
                    if ( pThing->moveStatus == SITHPLAYERMOVE_RUNNING )
                    {
                        if ( moveToResult == 3 )
                        {
                            memset(&pMoveInfo->physics.deltaVelocity, 0, sizeof(pMoveInfo->physics.deltaVelocity));
                            sithPuppet_bPlayerLeapForward = 0;
                            return 0.0f;
                        }

                        if ( sithPuppet_bPlayerLeapForward == 1 )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
                            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPLEFT, 0);
                            sithPuppet_bPlayerLeapForward = 0;
                            return moveDist;
                        }
                    }
                    else
                    {
                        if ( pThing->moveStatus == SITHPLAYERMOVE_WALKING )
                        {
                            pThing->moveStatus = SITHPLAYERMOVE_STILL;
                        }

                        memset(&pMoveInfo->physics.deltaVelocity, 0, sizeof(pMoveInfo->physics.deltaVelocity));
                        sithPuppet_bPlayerLeapForward = 0;
                        sithPhysics_ResetThingMovement(pThing);
                        return 0.0f;
                    }
                }

                break;
        }
    }

    while ( moveDist != 0.0f && v49 < 4 )
    {
        float distance;

        bCollided = 0;
        rdVector_Copy3(&prevPos, &pThing->pos);

        pPrevSector = pThing->pInSector;
        prevMoveDist = moveDist;

        rdVector_Copy3(&v40, &dest);

        searchDist = sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &dest, moveDist, pThing->collide.movesize, flags);
        while ( !bCollided && (pCollision = sithCollision_PopStack()) != NULL )
        {
            distance = pCollision->distance;
            if ( distance != 0.0f )
            {
                rdVector_ScaleAdd3(&pThing->pos, &dest, distance, &prevPos);
            }

            if ( distance < moveDist )
            {
                rdVector_Scale3(&pThing->moveDir, &dest, moveDist - distance);
                if ( pThing->moveType == SITH_MT_PHYSICS
                    && (pThing->moveInfo.physics.flags & SITH_PF_SURFACEBOUNCE) != 0
                    && (pThing->moveInfo.physics.gravityForce.x != 0.0f
                        || pThing->moveInfo.physics.gravityForce.y != 0.0f
                        || pThing->moveInfo.physics.gravityForce.z != 0.0f) )
                {
                    v44 = 1.0f - distance / moveDist;
                    rdVector_ScaleAdd3Acc(&pThing->moveInfo.physics.velocity, &pThing->moveInfo.physics.gravityForce, -v44);
                }
            }
            else
            {
                memset(&pThing->moveDir, 0, sizeof(pThing->moveDir));
            }

            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
                SithThingType type2;
                SithThing* pThingCollided;

#line 1206
                SITH_ASSERTREL(pCollision->pSurfaceCollided == NULL);
#line 1207
                SITH_ASSERTREL(pCollision->pThingCollided != NULL);

                pThingCollided = pCollision->pThingCollided;
                type2 = pThingCollided->type;
                type1 = pThing->type;
#line 1214
                SITH_ASSERTREL(aCollideResults[type1][type2].pProcessFunc != NULL);

                if ( aCollideResults[type1][type2].bDifferentTypHandler )
                {
                    bCollided = aCollideResults[type1][type2].pProcessFunc(pThingCollided, pThing, pCollision, 1);
                }
                else
                {
                    bCollided = aCollideResults[type1][type2].pProcessFunc(pThing, pThingCollided, pCollision, 0);
                }
            }
            else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
            {
#line 1232
                SITH_ASSERTREL(pCollision->pSurfaceCollided != NULL);
#line 1233
                SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin != NULL);
#line 1234
                SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin->pMirrorAdjoin != NULL);
#line 1235
                SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector != NULL);

                rdVector_Copy3(&curthPos, &pThing->pos);
                if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_COGLINKED) != 0 )
                {
                    sithCog_SurfaceSendMessage(pCollision->pSurfaceCollided, pThing, 8);
                }

                sithThing_SetSector(pThing, pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector, 0);
                if ( memcmp(&curthPos, &pThing->pos, sizeof(curthPos)) )
                {
                    bCollided = 1;
                }
                else
                {
                    bCollided = 0;
                }
            }
            else
            {
                SithSurface* pSurfaceCollided;

#line 1259
                SITH_ASSERTREL(pCollision->pThingCollided == NULL);
#line 1260
                SITH_ASSERTREL(pCollision->pSurfaceCollided != NULL);

                pSurfaceCollided = pCollision->pSurfaceCollided;
                if ( aThingSurfaceCollideResults[pThing->type] )
                {
                    bCollided = aThingSurfaceCollideResults[pThing->type](pThing, pSurfaceCollided, pCollision);
                }
                else
                {
                    bCollided = sithCollision_HandleThingHitSurface(pThing, pSurfaceCollided, pCollision);
                }
            }

            if ( v44 != 0.0f )
            {
                v19 = RD_MULTIPLY(sithTime_frameTimeFlex, v44);
                rdVector_Scale3(&pThing->moveDir, &pThing->moveInfo.physics.velocity, v19);
                v44 = 0.0f;
            }
        }

        sithCollision_DecreaseStackLevel();
        if ( !bCollided )
        {
            rdVector_ScaleAdd3(&pThing->pos, &dest, moveDist, &prevPos);

            memset(&pThing->moveDir, 0, sizeof(pThing->moveDir));
            v46 = v46 + moveDist;
            moveDist = 0.0f;
        }
        else
        {
            v46 = v46 + distance;
            if ( pThing->moveDir.x != 0.0f || pThing->moveDir.y != 0.0f || pThing->moveDir.z != 0.0f )
            {
                moveDist = rdVector_Normalize3(&dest, &pThing->moveDir);
                moveDist = stdMath_ClipNearZero(moveDist);
            }
            else
            {
                moveDist = 0.0f;
            }

            ++v49;
        }

        if ( (pThing->flags & SITH_TF_DESTROYED) != 0 )
        {
            return v46;
        }
    }


    if ( v49 >= 4 && pThing->moveType == SITH_MT_PHYSICS )
    {
        sithPhysics_ResetThingMovement(pThing);
    }

    v46 = stdMath_ClipNearZero(v46);

    if ( pThing->collide.type && pThing->moveType == SITH_MT_PHYSICS && (pThing->attach.flags & SITH_ATTACH_TAIL) == 0 )
    {
        if ( pThing->type == SITH_THING_PLAYER && pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
        {
            // A force-moved player is not checked against its sector.
        }
        else if ( !sithIntersect_IsSphereInSector(sithWorld_pCurrentWorld, &pThing->pos, 0.0f, pThing->pInSector) )
        {
            if ( !pThing->msecLifeLeft )
            {
                if ( pThing->type == SITH_THING_PLAYER )
                {
#line 1337
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing %s in wrong sector, pushed back to last good position.\n", pThing->aName, 0, 0, 0);
                }
            }
            else
            {
#line 1346
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing %s fell out of world, destroying.\n", pThing->aName, 0, 0, 0);
                sithThing_DestroyThing(pThing);
            }

            rdVector_Copy3(&pThing->pos, &prevPos);
            rdVector_Copy3(&dest, &v40);
            moveDist = prevMoveDist;
            sithThing_SetSector(pThing, pPrevSector, 0);
        }
    }

    for ( pAttached = pThing->pAttachedThing; pAttached; pAttached = pAttached->pNextAttachedThing )
    {
        if ( (pAttached->attach.flags & SITH_ATTACH_NOMOVE) != 0 && (pAttached->attach.flags & SITH_ATTACH_TAIL) == 0 )
        {
            rdMatrix_TransformVector34(&pAttached->pos, &pAttached->attach.posOffset, &pThing->orient);
            rdVector_Add3Acc(&pAttached->pos, &pThing->pos);

            if ( pAttached->pInSector != pThing->pInSector )
            {
                sithThing_SetSector(pAttached, pThing->pInSector, 0);
            }
        }
    }

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        return v46;
    }

    if ( v46 == 0.0f )
    {
        return 0.0f;
    }

    if ( (flags & 0x40) != 0 )
    {
        return v46;
    }

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_HANGING:
        case SITHPLAYERMOVE_CLIMBIDLE:
        case SITHPLAYERMOVE_WHIPSWINGING:
        case SITHPLAYERMOVE_WHIPCLIMBIDLE:
        case SITHPLAYERMOVE_JUMPFWD:
        case SITHPLAYERMOVE_JUMPUP:
        case SITHPLAYERMOVE_CLIMBING_UP:
        case SITHPLAYERMOVE_CLIMBING_DOWN:
        case SITHPLAYERMOVE_CLIMBING_LEFT:
        case SITHPLAYERMOVE_CLIMBING_RIGHT:
        case SITHPLAYERMOVE_PULLINGUP:
        case SITHPLAYERMOVE_WHIP_CLIMB_START:
        case SITHPLAYERMOVE_JEEP_STILL:
        case SITHPLAYERMOVE_RAFT_STILL:
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
        case SITHPLAYERMOVE_RAFT_STARTPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_ENDPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_ENDPADDLE_RIGHT:
        case SITHPLAYERMOVE_RAFT_STARTPADDLE_RIGHT:
        case SITHPLAYERMOVE_RAFT_PADDLERIGHT_STARTPADDLE_LEFT:
        case SITHPLAYERMOVE_RAFT_PADDLEL_LEFT_STARTPADDLE_RIGHT:
        case SITHPLAYERMOVE_JEEP_BOARDING:
        case SITHPLAYERMOVE_STAND_TO_CRAWL:
        case SITHPLAYERMOVE_CRAWL_TO_STAND:
        case SITHPLAYERMOVE_CLIMB_DOWN_TO_MOUNT:
        case SITHPLAYERMOVE_CLIMB_TO_HANG:
        case SITHPLAYERMOVE_RAFT_BOARDING:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_LEFT:
        case SITHPLAYERMOVE_RAFT_UNBOARDING_RIGHT:
        case SITHPLAYERMOVE_RAFT_UNBOARD_START:
        case SITHPLAYERMOVE_UNKNOWN_82:
        case SITHPLAYERMOVE_UNKNOWN_84:
        case SITHPLAYERMOVE_JEEP_UNBOARDING:
        case SITHPLAYERMOVE_JEWELFLYING:
        case SITHPLAYERMOVE_JEWELFLYING_UNKN1:
        case SITHPLAYERMOVE_JEWELFLYING_UNKN2:
        case SITHPLAYERMOVE_LEAPFWD:
            break;

        default:
            if ( (pThing->moveInfo.physics.flags & SITH_PF_RAFT) == 0
                && (!pThing->pAttachedThing || (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) == 0)
                && (pThing->attach.flags & SITH_ATTACH_TAIL) == 0 )
            {
                if ( pThing->attach.flags && (pThing->attach.flags & SITH_ATTACH_NOMOVE) == 0 )
                {
                    sithPhysics_FindFloor(pThing, 0);
                }
                else if ( (pThing->moveInfo.physics.flags & SITH_PF_FLOORSTICK) != 0 && J3DINRANGE(pThing->moveInfo.physics.velocity.z, -2.0f, 0.2f) )
                {
                    sithPhysics_FindFloor(pThing, 0);
                }
            }

            break;
    }

    return v46;
}

float J3DAPI sithCollision_SearchForCollisions(SithSector* pStartSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags)
{
    SithSector* pCurSect;
    unsigned int secNum;
    SithSurfaceAdjoin* curi;
    unsigned int numCurSectors;
    SithCollision* pCollision;
    SithSurfaceAdjoin* pAdjoin;
    unsigned int collNum;

#line 1493
    SITH_ASSERTREL((pStartSector) && (startPos) && (moveNorm));
#line 1494
    SITH_ASSERTREL((moveDist >= ((float)0.0)) && (radius >= ((float)0.0)));

    stackLevel++;
#line 1498
    SITH_ASSERTREL(stackLevel >= 0);
#line 1499
    SITH_ASSERTREL(stackLevel < SITHCOLLISION_STACKSIZE);

    sithCollision_ResetCurStack();
    moveDist = sithCollision_BuildCollisionList(pStartSector, pThing, startPos, moveNorm, moveDist, radius, searchFlags);

    for ( collNum = 0, pCollision = aCollisions[stackLevel]; collNum < aNumStackEntries[stackLevel]; ++collNum, ++pCollision )
    {
        if ( pCollision->type == SITHCOLLISION_ADJOINCROSS )
        {
            pAdjoin = pCollision->pSurfaceCollided->pAdjoin;
#line 1517
            SITH_ASSERTREL(pAdjoin);
#line 1518
            SITH_ASSERTREL(pAdjoin->pMirrorAdjoin != NULL);
#line 1519
            SITH_ASSERTREL(pAdjoin->pAdjoinSector != NULL);
            if ( (searchFlags & 0x400) != 0 || pCollision->distance <= moveDist )
            {
                moveDist = sithCollision_BuildCollisionList(pAdjoin->pAdjoinSector, pThing, startPos, moveNorm, moveDist, radius, searchFlags);
            }

            pCollision->bEnumerated = 1;
        }
    }

    if ( moveDist != 0.0f && (searchFlags & 0x800) != 0 )
    {
        numCurSectors = aNumSectors[stackLevel];
        for ( secNum = 0, pCurSect = apSearchedSectors[stackLevel][0]; secNum < numCurSectors; ++secNum, pCurSect = apSearchedSectors[stackLevel][secNum] )
        {
            for ( curi = pCurSect->pFirstAdjoin; curi; curi = curi->pNextAdjoin )
            {
                if ( (curi->flags & SITH_ADJOIN_MOVE) == 0 )
                {
                    continue;
                }

                if ( !curi->pAdjoinSector->pFirstThingInSector )
                {
                    continue;
                }

                if ( sithCollision_CheckSectorSearched(curi->pAdjoinSector) || !sithCollision_AddSearchedSector(curi->pAdjoinSector) )
                {
                    continue;
                }

                moveDist = sithCollision_SearchForThingCollisions(curi->pAdjoinSector, pThing, startPos, moveNorm, moveDist, radius, searchFlags);
            }
        }
    }

    return moveDist;
}

void J3DAPI sithCollision_DecreaseStackLevel(void)
{
#line 1583
    SITH_ASSERTREL(stackLevel >= 0);
#line 1584
    SITH_ASSERTREL(stackLevel < SITHCOLLISION_STACKSIZE);
    --stackLevel;
}
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithCollision_countfit_1, 10) /* end count-fit */
float J3DAPI sithCollision_SearchForThingCollisions(const SithSector* pSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags)
{
    SithThing* currentObj;
    float pHitDistance;
    rdFace* ppHitFace;
    rdModel3Mesh* ppHitMesh;
    SithCollisionType hitType;
    rdVector3 currentSrc;

    ppHitMesh = 0;
    ppHitFace = 0;
    if ( sithWorld_pCurrentWorld )
    {
#line 1627
        SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));
#line 1628
        SITH_ASSERTREL(!pThing || sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    }

    for ( currentObj = pSector->pFirstThingInSector; currentObj; currentObj = currentObj->pNextThingInSector )
    {
        if ( (searchFlags & 8) != 0 && (currentObj->flags & SITH_TF_MOUNTABLE) == 0
            || (searchFlags & 0x10) != 0 && (currentObj->flags & SITH_TF_STANDON) == 0 )
        {
            continue;
        }

        if ( (searchFlags & 0x4000) != 0
            && (currentObj->type == SITH_THING_PLAYER
                || currentObj->type == SITH_THING_ACTOR
                || currentObj->type == SITH_THING_WEAPON
                || currentObj->type == SITH_THING_DEBRIS
                || (currentObj->flags & (SITH_TF_INVISIBLE | SITH_TF_NOWEAPONCOLLIDE)) != 0) )
        {
            continue;
        }

        if ( sithCollision_IsThingCollidable(currentObj, searchFlags) && (!pThing || sithCollision_CanThingsCollide(pThing, currentObj, searchFlags)) )
        {
            hitType = sithIntersect_CheckSphereThingIntersection(
                pThing,
                startPos,
                moveNorm,
                moveDist,
                radius,
                currentObj,
                searchFlags,
                &pHitDistance,
                &ppHitMesh,
                &ppHitFace,
                &currentSrc);
            if ( hitType )
            {
                sithCollision_PushThingCollision(currentObj, pHitDistance, (SithCollisionType)(hitType | SITHCOLLISION_THING), ppHitMesh, ppHitFace, &currentSrc);
                if ( pThing )
                {
                    if ( sithCollision_CheckSearchFilter(pThing, currentObj) )
                    {
                        moveDist = pHitDistance;
                    }
                }
            }
        }
    }

    return moveDist;
}





void J3DAPI sithCollision_SearchForSurfaceCollisions(const SithSector* pSector, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int colflags)
{
    SithSurface* pSurface;
    float pHitDistance;
    SithSurfaceAdjoin* pAdjoin;
    unsigned int surfNum;
    rdVector3 hitNorm;
    rdVector3 v9;
    SithCollisionType hitType;
    SithWorld* pWorld;
    float distance;

    pWorld = sithWorld_pCurrentWorld;
    if ( sithWorld_pCurrentWorld )
    {
#line 1719
        SITH_ASSERTREL(sithSector_ValidateSectorPointer(sithWorld_pCurrentWorld, pSector));
    }
#line 1721
    SITH_ASSERTREL(radius >= ((float)0.0));
#line 1722
    SITH_ASSERTREL(moveDist >= ((float)0.0));
    rdVector_ScaleAdd3(&v9, moveNorm, moveDist, startPos);
    if ( (pSector->flags & SITH_SECTOR_HASCOLLIDEBOX) != 0
        && v9.z - radius > pSector->collideBox.v0.z
        && v9.y - radius > pSector->collideBox.v0.y
        && v9.x - radius > pSector->collideBox.v0.x
        && v9.x + radius < pSector->collideBox.v1.x
        && v9.y + radius < pSector->collideBox.v1.y
        && v9.z + radius < pSector->collideBox.v1.z )
    {
        return;
    }

    for ( surfNum = 0, pSurface = pSector->pFirstSurface; surfNum < pSector->numSurfaces; ++surfNum, ++pSurface )
    {
        pAdjoin = pSurface->pAdjoin;
        if ( (pSurface->flags & SITH_SURFACE_COLLISION) == 0 && !pAdjoin )
        {
            continue;
        }

        if ( !pAdjoin || !sithCollision_CanPassAdjoin(pAdjoin->flags, colflags) )
        {
            if ( (colflags & 4) != 0 || (colflags & 0x10) != 0 && (pSurface->flags & SITH_SURFACE_ISFLOOR) == 0 )
            {
                continue;
            }

            if ( rdMath_DistancePointToPlane(&v9, &pSurface->face.normal, &pWorld->aVertices[*pSurface->face.aVertices]) > radius )
            {
                continue;
            }

            hitType = sithIntersect_CheckSphereFaceIntersectionEx(
                startPos,
                moveNorm,
                moveDist,
                radius,
                &pSurface->face,
                pWorld->aVertices,
                &pHitDistance,
                &hitNorm,
                colflags);
            if ( !hitType )
            {
                continue;
            }

            if ( (colflags & 0x400) != 0 || rdVector_Dot3(moveNorm, &hitNorm) < 0.0f )
            {
                sithCollision_PushSurfaceCollision(pSurface, pHitDistance, (SithCollisionType)(hitType | SITHCOLLISION_WORLD), &hitNorm);
            }
        }
        else
        {
            hitType = sithIntersect_CheckSphereFaceIntersection(
                startPos,
                moveNorm,
                moveDist,
                radius,
                &pSurface->face,
                pWorld->aVertices,
                &pHitDistance,
                colflags);
            if ( !hitType )
            {
                continue;
            }

            if ( ((colflags & 4) == 0 || (colflags & 1) == 0) && !sithCollision_CheckSectorSearched(pAdjoin->pAdjoinSector) )
            {
                sithCollision_PushSurfaceCollision(pSurface, pHitDistance, SITHCOLLISION_ADJOINCROSS, NULL);
            }

            if ( (colflags & 2) != 0 )
            {
                continue;
            }

            if ( sithIntersect_CheckSphereFaceIntersection(startPos, moveNorm, moveDist, 0.0f, &pSurface->face, pWorld->aVertices, &distance, colflags) )
            {
                if ( (colflags & 4) != 0 && (colflags & 1) != 0 && !sithCollision_CheckSectorSearched(pAdjoin->pAdjoinSector) )
                {
                    sithCollision_PushSurfaceCollision(pSurface, pHitDistance, SITHCOLLISION_ADJOINCROSS, NULL);
                }

                sithCollision_PushSurfaceCollision(pSurface, distance, SITHCOLLISION_ADJOINTOUCH, NULL);
            }
        }
    }
}

int J3DAPI sithCollision_HandleThingHitSurface(SithThing* pThing, SithSurface* pSurface, SithCollision* pCollision)
{
    float volume = 1.0f;
    float impactSpeed;
    float absVelZ;
    SithMoveInfo* pMoveInfo;
    static float sithCollision_jeepFallingDeathSpeed = -1.65f;

#line 1828
    SITH_ASSERTREL(pThing != NULL);
#line 1829
    SITH_ASSERTREL(pSurface != NULL);
    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        return 0;
    }

    pMoveInfo = &pThing->moveInfo;
    impactSpeed = -rdVector_Dot3(&pMoveInfo->physics.velocity, &pCollision->hitNorm);

    absVelZ = (float)fabs(pMoveInfo->physics.velocity.z);

    if ( sithCollision_CollideHurt(pThing, &pCollision->hitNorm, pCollision->distance, 0, pSurface) )
    {
        if ( (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 )
        {
            if ( pSurface->msecLastTouched + 500 > sithTime_clockTime )
            {
            }
            else
            {
                pSurface->msecLastTouched = sithTime_clockTime;
                sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_TOUCHED);
            }
        }

        if ( pThing->type == SITH_THING_PLAYER )
        {
            if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
            {
                if ( pThing->moveInfo.physics.velocity.z < sithCollision_jeepFallingDeathSpeed )
                {
#ifdef J3D_DEBUG
                    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                    {
                        char aText[256];
                        sprintf(aText, "Falling Death Wall %f.", pThing->moveInfo.physics.velocity.z);
                        sithConsole_PrintString(aText);
                    }
#endif
                    if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                    {
                        pThing->moveStatus = SITHPLAYERMOVE_JEEP_IMPACT;
                        sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                    }
                }
                else
                {
                    if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_82 )
                    {
                        sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, 0);
                        pThing->moveStatus = SITHPLAYERMOVE_JEEP_STILL;
                        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHARD);
                    }
                }
            }
            else if ( (pThing->moveStatus == SITHPLAYERMOVE_FALLING || !pThing->attach.flags) && absVelZ >= 1.45f )
            {
                if ( absVelZ >= 1.65f )
                {
                    if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                    {
                        sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                    }
                }
                else
                {
                    if ( absVelZ >= 1.55f )
                    {
                        sithThing_DamageThing(pThing, pThing, 500.0f, SITH_DAMAGE_IMPACT);
                    }
                    else
                    {
                        sithThing_DamageThing(pThing, pThing, 200.0f, SITH_DAMAGE_IMPACT);
                    }
                }

                if ( pThing->pSoundClass )
                {
                    sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHURT);
                }
            }
        }

        if ( impactSpeed > 0.15f )
        {
            volume = 1.0f < impactSpeed ? 1.0f : impactSpeed;

            if ( (pSurface->flags & SITH_SURFACE_METAL) != 0 )
            {
                sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITMETAL, volume);
            }

            else if ( (pSurface->flags & SITH_SURFACE_EARTH) != 0 )
            {
                sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITEARTH, volume);
            }
            else
            {
                sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITHARD, volume);
            }

        }

        return 1;
    }

    return 0;
}






static void J3DAPI sithCollision_VehicleCollisionHandler(SithThing* pThing, SithThing* pHitThing, rdVector3* pForce, rdVector3* pImpactForce, int* a5, const rdVector3* hitNormal, float impactSpeed, float hitImpactSpeed)
{
    float hitDot;
    rdVector3 hitMoveDir;
    int bThingIsVehicle;
    float sumImpactSpeed;
    float objSpeed;
    static float sithCollision_vehicleVerticalImpulse = 0.19f;
    static float sithCollision_vehicleImpactScale = 4.0f;
    static float sithCollision_vehicleDamageSpeed = 0.105f;
    static float sithCollision_jeepDamage = 1.0f;
    static float sithCollision_vehicleMaxSpeed = 1.2f;
    static float sithCollision_vehicleForceScale = 1.2f;
    static float sithCollision_vehicleHitHeight = 0.15f;
    static float sithCollision_vehicleHitDot = 0.707f;
    static float sithCollision_vehicleMinSpeed = 0.4f;
    if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
    {
        bThingIsVehicle = 1;
    }
    else if ( pHitThing->moveType == SITH_MT_PHYSICS && (pHitThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
    {
        bThingIsVehicle = 0;
    }
    else if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
    {
        bThingIsVehicle = 1;
    }
    else
    {
        bThingIsVehicle = 0;
    }

    if ( bThingIsVehicle )
    {
        memset(pForce, 0, sizeof(rdVector3));

        *a5 = 0;
        if ( pHitThing->moveType == SITH_MT_PHYSICS && (pHitThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            hitDot = SITHCOLLISION_ABS(rdVector_Dot3(&pHitThing->orient.lvec, hitNormal));
            sumImpactSpeed = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pHitThing->moveInfo.physics.thrust, &pHitThing->moveInfo.physics.thrust)), sithTime_frameTimeFlex);
            sumImpactSpeed = RD_MULTIPLY(sumImpactSpeed, hitDot);
            sumImpactSpeed = impactSpeed + hitImpactSpeed + sumImpactSpeed;

            rdVector_Scale3(pImpactForce, hitNormal, -RD_MULTIPLY(pHitThing->moveInfo.physics.mass, sumImpactSpeed));

            rdVector_Scale3Acc(pImpactForce, sithCollision_vehicleForceScale);

            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITMETAL, 1.0f);
        }
        else if ( pHitThing->type == SITH_THING_PLAYER )
        {
            float dot;
            float dheight;
            rdVector3 impactDir;

            dheight = pHitThing->pos.z - pThing->pos.z;
            if ( dheight > sithCollision_vehicleHitHeight )
            {
                return;
            }

            rdVector_Sub3(&impactDir, &pHitThing->pos, &pThing->pos);
            impactDir.z = sithPhysics_GetThingHeight(pThing) - sithPhysics_GetThingHeight(pHitThing) + impactDir.z;
            rdVector_Normalize3Acc(&impactDir);

            rdVector_Normalize3(&hitMoveDir, &pThing->moveInfo.physics.velocity);
            dot = rdVector_Dot3(&hitMoveDir, &impactDir);
            if ( dot < sithCollision_vehicleHitDot )
            {
                return;
            }

            if ( sqrtf(rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pThing->moveInfo.physics.velocity)) < sithCollision_vehicleMinSpeed )
            {
                return;
            }

            objSpeed = sqrtf(rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pThing->moveInfo.physics.velocity));
            objSpeed = STDMATH_CLAMP(objSpeed, 0.0f, sithCollision_vehicleMaxSpeed);
            pImpactForce->z = RD_MULTIPLY(RD_MULTIPLY(objSpeed, sithCollision_vehicleVerticalImpulse), pHitThing->moveInfo.physics.mass);
            sithThing_DetachThing(pHitThing);

            rdVector_Copy3(&pHitThing->orient.lvec, hitNormal);

            rdVector_Cross3(&pHitThing->orient.rvec, &pHitThing->orient.lvec, &pHitThing->orient.uvec);
            rdMatrix_Normalize34(&pHitThing->orient);

            sithThing_DamageThing(pHitThing, pThing, pHitThing->thingInfo.actorInfo.maxHealth + 1.0f, SITH_DAMAGE_VEHICLE);
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HITHARD);

            if ( pImpactForce->z < 0.0f )
            {
                pImpactForce->z = 0.0f;
            }

            rdVector_Scale3Acc(pImpactForce, sithCollision_vehicleImpactScale);
        }
        else if ( pHitThing->moveStatus == SITHPLAYERMOVE_RUNOVER )
        {
            memset(pImpactForce, 0, sizeof(rdVector3));
        }
        else if ( pHitThing->type == SITH_THING_ACTOR )
        {
            if ( pHitThing->moveInfo.physics.mass <= 75.0f
                || rdVector_Len3(pImpactForce) / pHitThing->moveInfo.physics.mass >= sithCollision_vehicleDamageSpeed )
            {
                objSpeed = sqrtf(rdVector_Dot3(&pThing->moveInfo.physics.velocity, &pThing->moveInfo.physics.velocity));
                objSpeed = STDMATH_CLAMP(objSpeed, 0.0f, sithCollision_vehicleMaxSpeed);
                pImpactForce->z = RD_MULTIPLY(RD_MULTIPLY(objSpeed, sithCollision_vehicleVerticalImpulse), pHitThing->moveInfo.physics.mass);
                sithThing_DetachThing(pHitThing);

                rdVector_Copy3(&pHitThing->orient.lvec, hitNormal);

                rdVector_Cross3(&pHitThing->orient.rvec, &pHitThing->orient.lvec, &pHitThing->orient.uvec);
                rdMatrix_Normalize34(&pHitThing->orient);

                sithThing_DamageThing(pHitThing, pThing, pHitThing->thingInfo.actorInfo.maxHealth + 1.0f, SITH_DAMAGE_VEHICLE);

                if ( (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_LWALKSNOW);
                }

                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HITHARD);
            }
            else
            {
                sithThing_DamageThing(pHitThing, pThing, sithCollision_jeepDamage, SITH_DAMAGE_VEHICLE);
            }

            if ( pImpactForce->z < 0.0f )
            {
                pImpactForce->z = 0.0f;
            }

            rdVector_Scale3Acc(pImpactForce, sithCollision_vehicleImpactScale);
        }
    }
    else if ( !bThingIsVehicle )
    {
        memset(pImpactForce, 0, sizeof(rdVector3));
        if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            rdVector3 impactDir;

            impactDir.x = pThing->pos.x - pHitThing->pos.x;
            impactDir.y = pThing->pos.y - pHitThing->pos.y;
            impactDir.z = pThing->pos.z - pHitThing->pos.z;
            impactDir.z = impactDir.z - 0.14f;
            rdVector_Normalize3Acc(&impactDir);

            hitDot = SITHCOLLISION_ABS(rdVector_Dot3(&pThing->orient.lvec, &impactDir));
            sumImpactSpeed = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pThing->moveInfo.physics.thrust, &pThing->moveInfo.physics.thrust)), sithTime_frameTimeFlex);
            sumImpactSpeed = RD_MULTIPLY(sumImpactSpeed, hitDot);
            sumImpactSpeed = impactSpeed + hitImpactSpeed + sumImpactSpeed;

            rdVector_Scale3(pForce, &impactDir, RD_MULTIPLY(pThing->moveInfo.physics.mass, sumImpactSpeed));

            rdVector_Scale3Acc(pForce, sithCollision_vehicleForceScale);
            sithSoundClass_PlayModeFirstEx(pHitThing, SITHSOUNDCLASS_HITMETAL, 1.0f);
        }
        else if ( pThing->type == SITH_THING_PLAYER )
        {
            float dot;
            float dheight;
            rdVector3 impactDir;

            dheight = pThing->pos.z - pHitThing->pos.z;
            if ( dheight > sithCollision_vehicleHitHeight )
            {
                return;
            }

            rdVector_Sub3(&impactDir, &pThing->pos, &pHitThing->pos);
            impactDir.z = sithPhysics_GetThingHeight(pHitThing) - sithPhysics_GetThingHeight(pThing) + impactDir.z;
            rdVector_Normalize3Acc(&impactDir);

            rdVector_Normalize3(&hitMoveDir, &pHitThing->moveInfo.physics.velocity);
            dot = rdVector_Dot3(&hitMoveDir, &impactDir);
            if ( dot < sithCollision_vehicleHitDot )
            {
                return;
            }

            if ( sqrtf(rdVector_Dot3(&pHitThing->moveInfo.physics.velocity, &pHitThing->moveInfo.physics.velocity)) < sithCollision_vehicleMinSpeed )
            {
                return;
            }

            objSpeed = sqrtf(rdVector_Dot3(&pHitThing->moveInfo.physics.velocity, &pHitThing->moveInfo.physics.velocity));
            objSpeed = STDMATH_CLAMP(objSpeed, 0.0f, sithCollision_vehicleMaxSpeed);
            pForce->z = RD_MULTIPLY(RD_MULTIPLY(objSpeed, sithCollision_vehicleVerticalImpulse), pThing->moveInfo.physics.mass);
            sithThing_DetachThing(pThing);

            rdVector_Neg3(&pThing->orient.lvec, hitNormal);

            rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec);
            rdMatrix_Normalize34(&pThing->orient);

            sithThing_DamageThing(pThing, pHitThing, pThing->thingInfo.actorInfo.maxHealth + 1.0f, SITH_DAMAGE_VEHICLE);
            sithSoundClass_PlayModeRandom(pHitThing, SITHSOUNDCLASS_HITHARD);

            if ( pForce->z < 0.0f )
            {
                pForce->z = 0.0f;
            }

            rdVector_Scale3Acc(pForce, sithCollision_vehicleImpactScale);
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_RUNOVER )
        {
            memset(pForce, 0, sizeof(rdVector3));
        }
        else if ( pThing->type == SITH_THING_ACTOR )
        {
            if ( pThing->moveInfo.physics.mass <= 75.0f
                || rdVector_Len3(pForce) / pThing->moveInfo.physics.mass >= sithCollision_vehicleDamageSpeed )
            {
                objSpeed = sqrtf(rdVector_Dot3(&pHitThing->moveInfo.physics.velocity, &pHitThing->moveInfo.physics.velocity));
                objSpeed = STDMATH_CLAMP(objSpeed, 0.0f, sithCollision_vehicleMaxSpeed);
                pForce->z = RD_MULTIPLY(RD_MULTIPLY(objSpeed, sithCollision_vehicleVerticalImpulse), pThing->moveInfo.physics.mass);
                sithThing_DetachThing(pThing);

                rdVector_Neg3(&pThing->orient.lvec, hitNormal);

                rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec);
                rdMatrix_Normalize34(&pThing->orient);

                sithThing_DamageThing(pThing, pHitThing, pThing->thingInfo.actorInfo.maxHealth + 1.0f, SITH_DAMAGE_VEHICLE);

                if ( (pHitThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
                {
                    sithSoundClass_PlayModeFirst(pHitThing, SITHSOUNDCLASS_LWALKSNOW);
                }

                sithSoundClass_PlayModeRandom(pHitThing, SITHSOUNDCLASS_HITHARD);
            }
            else
            {
                sithThing_DamageThing(pThing, pHitThing, sithCollision_jeepDamage, SITH_DAMAGE_VEHICLE);
            }

            if ( pForce->z < 0.0f )
            {
                pForce->z = 0.0f;
            }

            rdVector_Scale3Acc(pForce, sithCollision_vehicleImpactScale);
        }
    }
}





int J3DAPI sithCollision_ThingCollisionHandler(SithThing* pSrcThing, SithThing* pThingCollided, SithCollision* pCollision, int bSecondThingIsSource)
{
    float hitImpact;
    float z;
    rdVector3 moveNorm;
    static float sithCollision_tailForceScale = 1.2f;
    SithThing* pThing;
    float moveDist;
    float totalMass;
    float mass;
    SithThing* pHitThing;
    float hitMass;
    float impactSpeed;
    float hitImpactSpeed;
    rdVector3 hitNorm;
    rdVector3 pImpactForce;
    rdVector3 pForce;
    float totalImpactSpeed;
    float dot;
    float distMoved;
    float moveDot;

    if ( pSrcThing->type == SITH_THING_COG
        && pThingCollided->type == SITH_THING_COG
        && (pSrcThing->flags & SITH_TF_MOVABLE) == 0
        && (pThingCollided->flags & SITH_TF_MOVABLE) == 0 )
    {
        return 0;
    }

    rdVector_Copy3(&hitNorm, &pCollision->hitNorm);

    if ( bSecondThingIsSource )
    {
        pThing = pThingCollided;
        pHitThing = pSrcThing;
    }
    else
    {
        pThing = pSrcThing;
        pHitThing = pThingCollided;
    }

    if ( (pThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 && pThing->type != SITH_THING_ITEM )
    {
        sithCog_ThingSendMessage(pThing, pHitThing, SITHCOG_MSG_TOUCHED);
    }

    if ( (pHitThing->flags & SITH_TF_COGLINKED) != 0 && (pThing->flags & SITH_TF_REMOTE) == 0 && pHitThing->type != SITH_THING_ITEM )
    {
        sithCog_ThingSendMessage(pHitThing, pThing, SITHCOG_MSG_TOUCHED);
    }

    if ( pThing->type == SITH_THING_PLAYER && pHitThing->type == SITH_THING_ITEM )
    {
        return 1;
    }

    // Handle jeep stop obj collision
    if ( (pHitThing->flags & SITH_TF_JEEPSTOP) != 0 )
    {

        // If colliding object is not player or player is not jeep player there is no collision and return
        if ( pThing->type == SITH_THING_PLAYER && (pThing->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            dot = rdVector_Dot3(&pHitThing->orient.lvec, &pThing->moveInfo.physics.velocity);
            if ( dot >= 0.0f )
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    if ( pThing->moveType != SITH_MT_PHYSICS || pThing->moveInfo.physics.mass == 0.0f )
    {
        if ( pHitThing->moveType != SITH_MT_PHYSICS || pHitThing->moveInfo.physics.mass == 0.0f )
        {
            return 1;
        }
        else if ( pHitThing->moveType == SITH_MT_PHYSICS && (pHitThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
        {
            sithPhysics_ResetThingMovement(pHitThing);
            return 0;
        }
        else if ( (pHitThing->pAttachedThing && (pHitThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0
            || (pHitThing->attach.flags & SITH_ATTACH_TAIL) != 0)
            && !streqi(pThing->aName, "elevpltfrm") )
        {
            return 0;
        }
        else
        {
            moveDot = rdVector_Dot3(&pThing->moveDir, &hitNorm);
            moveDot = stdMath_ClipNearZero(moveDot);
            if ( moveDot >= 0.0f )
            {
                return 0;
            }
            else
            {
                moveDist = -moveDot;
                rdVector_Neg3(&moveNorm, &hitNorm);
                moveDist = RD_MULTIPLY(moveDist, 1.0001f);
                distMoved = sithCollision_MoveThing(pHitThing, &moveNorm, moveDist, 0);

                if ( distMoved < moveDist )
                {
                    if ( (pThing->flags & SITH_TF_NOCRUSH) == 0 )
                    {
                        sithThing_DamageThing(pHitThing, pThing, (moveDist - distMoved) * 100.0f, SITH_DAMAGE_IMPACT);
                    }

                    memset(&pThing->moveDir, 0, sizeof(pThing->moveDir));
                }

                return 1;
            }
        }
    }
    else if ( pHitThing->moveType != SITH_MT_PHYSICS || pHitThing->moveInfo.physics.mass == 0.0f )
    {
        hitImpact = -rdVector_Dot3(&pThing->moveInfo.physics.velocity, &hitNorm);
        z = pThing->moveInfo.physics.velocity.z;

        if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & SITH_PF_MINECAR) != 0 )
        {
            if ( pThing->controlType == SITH_CT_AI )
            {
                sithCog_ThingSendMessage(pThing, 0, SITHCOG_MSG_BLOCKED);
            }

            sithPhysics_ResetThingMovement(pThing);
            return 1;
        }
        else if ( (pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 || (pThing->attach.flags & SITH_ATTACH_TAIL) != 0)
            && !streqi(pHitThing->aName, "elevpltfrm") )
        {
            return 0;
        }
        else if ( sithCollision_CollideHurt(pThing, &hitNorm, pCollision->distance, 0, 0) )
        {
            if ( hitImpact > 0.15f || z < -0.15f )
            {
                hitImpact = hitImpact > (float)fabs(z) ? hitImpact : ((float)fabs(z));

                if ( pThing->type == SITH_THING_PLAYER
                    && (pThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_RAFT | SITH_PF_MINECAR)) == 0
                    && !pSrcThing->attach.flags
                    && hitImpact >= 1.45f )
                {
                    if ( hitImpact >= 1.65f )
                    {
                        if ( (pSrcThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                        {
                            sithActor_KillActor(pSrcThing, pSrcThing, SITH_DAMAGE_IMPACT);
                        }
                    }
                    else
                    {
                        if ( hitImpact >= 1.55f )
                        {
                            sithThing_DamageThing(pSrcThing, pSrcThing, 500.0f, SITH_DAMAGE_IMPACT);
                        }
                        else
                        {
                            sithThing_DamageThing(pSrcThing, pSrcThing, 200.0f, SITH_DAMAGE_IMPACT);
                        }
                    }

                    if ( pSrcThing->pSoundClass )
                    {
                        sithSoundClass_PlayModeRandom(pSrcThing, SITHSOUNDCLASS_LANDHURT);
                    }
                }

                hitImpact = 1.0f < hitImpact ? 1.0f : hitImpact;

                if ( (pHitThing->flags & SITH_TF_METAL) != 0 )
                {
                    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITMETAL, hitImpact);
                }
                else if ( (pHitThing->flags & SITH_TF_EARTH) != 0 )
                {
                    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITEARTH, hitImpact);
                }
                else if ( (pHitThing->flags & SITH_TF_NOSOUND) == 0 )
                {
                    sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_HITHARD, hitImpact);
                }
            }

            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        int a5;
        float hitDot;

        a5 = 1;
        mass = pThing->moveInfo.physics.mass;

        impactSpeed = -rdVector_Dot3(&pThing->moveInfo.physics.velocity, &hitNorm);

        hitMass = pHitThing->moveInfo.physics.mass;

        hitImpactSpeed = rdVector_Dot3(&pHitThing->moveInfo.physics.velocity, &hitNorm);
        totalImpactSpeed = impactSpeed + hitImpactSpeed;
        totalImpactSpeed = stdMath_ClipNearZero(totalImpactSpeed);

        totalMass = mass + hitMass;
#line 2418
        SITH_ASSERTREL(totalMass > ((float)0.0));
        if ( totalImpactSpeed <= 0.0f )
        {
            return 0;
        }
        else
        {
            if ( (pThing->moveInfo.physics.flags & SITH_PF_SURFACEBOUNCE) == 0 )
            {
                totalImpactSpeed = RD_MULTIPLY(totalImpactSpeed, 0.5f);
            }

            if ( (pHitThing->moveInfo.physics.flags & SITH_PF_SURFACEBOUNCE) == 0 )
            {
                totalImpactSpeed = RD_MULTIPLY(totalImpactSpeed, 0.5f);
            }

            rdVector_Scale3(&pForce, &hitNorm, totalImpactSpeed);

            rdVector_Scale3Acc(&pForce, (RD_MULTIPLY(RD_MULTIPLY(mass, hitMass), 2.0f)) / (mass + hitMass));

            rdVector_Neg3(&pImpactForce, &pForce);

            if ( pThing->moveType == SITH_MT_PHYSICS && (pThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) != 0 )
            {
                sithCollision_VehicleCollisionHandler(pThing, pHitThing, &pForce, &pImpactForce, &a5, &hitNorm, impactSpeed, hitImpactSpeed);
            }
            else if ( pHitThing->moveType == SITH_MT_PHYSICS && (pHitThing->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) != 0 )
            {
                sithCollision_VehicleCollisionHandler(pThing, pHitThing, &pForce, &pImpactForce, &a5, &hitNorm, impactSpeed, hitImpactSpeed);
            }

            else if ( pThing->pAttachedThing && (pThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0
                || (pThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                hitDot = SITHCOLLISION_ABS(rdVector_Dot3(&pHitThing->orient.lvec, &hitNorm));
                totalImpactSpeed = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pHitThing->moveInfo.physics.thrust, &pHitThing->moveInfo.physics.thrust)), sithTime_frameTimeFlex);
                totalImpactSpeed = RD_MULTIPLY(totalImpactSpeed, hitDot);

                totalImpactSpeed = impactSpeed + hitImpactSpeed + totalImpactSpeed;

                rdVector_Scale3(&pImpactForce, &hitNorm, -RD_MULTIPLY(pHitThing->moveInfo.physics.mass, totalImpactSpeed));

                rdVector_Scale3Acc(&pImpactForce, sithCollision_tailForceScale);
                memset(&pForce, 0, sizeof(pForce));

                a5 = 0;
            }

            else if ( pHitThing->pAttachedThing && (pHitThing->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0
                || (pHitThing->attach.flags & SITH_ATTACH_TAIL) != 0 )
            {
                hitDot = SITHCOLLISION_ABS(rdVector_Dot3(&pThing->orient.lvec, &hitNorm));
                totalImpactSpeed = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pThing->moveInfo.physics.thrust, &pThing->moveInfo.physics.thrust)), sithTime_frameTimeFlex);
                totalImpactSpeed = RD_MULTIPLY(totalImpactSpeed, hitDot);

                totalImpactSpeed = impactSpeed + hitImpactSpeed + totalImpactSpeed;

                rdVector_Scale3(&pForce, &hitNorm, RD_MULTIPLY(pThing->moveInfo.physics.mass, totalImpactSpeed));

                rdVector_Scale3Acc(&pForce, sithCollision_tailForceScale);
                memset(&pImpactForce, 0, sizeof(pImpactForce));
            }

            sithPhysics_ApplyForce(pThing, &pForce);
            sithPhysics_ApplyForce(pHitThing, &pImpactForce);

            if ( a5 )
            {
                return sithCollision_CollideHurt(pThing, &hitNorm, pCollision->distance, 0, 0);
            }
            else
            {
                return 0;
            }
        }
    }
}

int J3DAPI sithCollision_CollideHurt(SithThing* pThing, const rdVector3* pHitNorm, float colDistance, int a4, const SithSurface* pHitSurf)
{
    float impactDot;
    float v36;
    SithMoveInfo* pMoveInfo;
    float hitDot;
    float impactScale;
    float amount;

#line 2511
    SITH_ASSERTREL(pThing);
    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
        return 0;
    }

    pMoveInfo = &pThing->moveInfo;

    hitDot = -rdVector_Dot3(&pThing->moveDir, pHitNorm);
    hitDot = stdMath_ClipNearZero(hitDot);
    if ( hitDot <= 0.0f )
    {
        return 0;
    }

    if ( (pMoveInfo->physics.flags & SITH_PF_SURFACEBOUNCE) != 0 )
    {
        impactScale = 1.9f;
    }
    else
    {
        impactScale = 1.0001f;
    }

    if ( colDistance == 0.0f && sithCollision_dword_17F1090 )
    {
        hitDot = -rdVector_Dot3(&pThing->moveDir, pHitNorm);
        if ( hitDot > 0.0f )
        {
            rdVector_ScaleAdd3Acc(&pThing->moveDir, pHitNorm, hitDot);
        }
        else
        {
            return 0;
        }

        impactDot = -rdVector_Dot3(&pMoveInfo->physics.velocity, pHitNorm);

        if ( (pMoveInfo->physics.flags & SITH_PF_RAFT) != 0 && pHitSurf && (pHitSurf->flags & SITH_SURFACE_WATER) != 0 )
        {
            rdVector3 zImpactVel;
            float impact;
            float zHitDot;

            impact = rdMath_DistancePointToPlane(&pMoveInfo->physics.velocity, pHitNorm, &rdroid_zeroVector3);
            zHitDot = rdVector_Dot3(pHitNorm, &rdroid_zVector3);
            impact = D3DVAL(impact / zHitDot);

            zImpactVel.x = RD_MULTIPLY(-impact, rdroid_zVector3.x) + pMoveInfo->physics.velocity.x;
            zImpactVel.y = RD_MULTIPLY(-impact, rdroid_zVector3.y) + pMoveInfo->physics.velocity.y;
            zImpactVel.z = RD_MULTIPLY(-impact, rdroid_zVector3.z) + pMoveInfo->physics.velocity.z;
            rdVector_Normalize3Acc(&zImpactVel);

            zImpactVel.x = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pMoveInfo->physics.velocity, &pMoveInfo->physics.velocity)), zImpactVel.x);
            zImpactVel.y = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pMoveInfo->physics.velocity, &pMoveInfo->physics.velocity)), zImpactVel.y);
            zImpactVel.z = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pMoveInfo->physics.velocity, &pMoveInfo->physics.velocity)), zImpactVel.z);

            rdVector_Copy3(&pMoveInfo->physics.velocity, &zImpactVel);
        }
        else
        {
            rdVector_ScaleAdd3Acc(&pMoveInfo->physics.velocity, pHitNorm, impactDot);
        }

        v36 = -rdVector_Dot3(pHitNorm, &sithCollision_vec17F10A0);
        rdVector_ScaleAdd3Acc(&sithCollision_vec17F10A0, pHitNorm, v36);
        rdVector_Normalize3Acc(&sithCollision_vec17F10A0);

        impactDot = -rdVector_Dot3(&pMoveInfo->physics.velocity, &sithCollision_vec17F10A0);

        rdVector_ScaleAdd3Acc(&pMoveInfo->physics.velocity, &sithCollision_vec17F10A0, impactDot);

        hitDot = -rdVector_Dot3(&pThing->moveDir, &sithCollision_vec17F10A0);
        if ( hitDot > 0.0f )
        {
            rdVector_ScaleAdd3Acc(&pThing->moveDir, &sithCollision_vec17F10A0, hitDot);
        }
        else
        {
            return 1;
        }
    }

    rdVector_Copy3(&sithCollision_vec17F10A0, pHitNorm);
    sithCollision_dword_17F1090 = 1;

    impactDot = -rdVector_Dot3(&pMoveInfo->physics.velocity, pHitNorm);
    if ( impactDot > 0.0f )
    {
        if ( (pMoveInfo->physics.flags & SITH_PF_RAFT) != 0 && pHitSurf && (pHitSurf->flags & SITH_SURFACE_WATER) != 0 )
        {
            rdVector3 newVel;
            float impact;
            float zHitDot;

            impact = rdMath_DistancePointToPlane(&pMoveInfo->physics.velocity, pHitNorm, &rdroid_zeroVector3);
            zHitDot = rdVector_Dot3(pHitNorm, &rdroid_zVector3);
            impact = D3DVAL(impact / zHitDot);

            newVel.x = RD_MULTIPLY(-impact, rdroid_zVector3.x) + pMoveInfo->physics.velocity.x;
            newVel.y = RD_MULTIPLY(-impact, rdroid_zVector3.y) + pMoveInfo->physics.velocity.y;
            newVel.z = RD_MULTIPLY(-impact, rdroid_zVector3.z) + pMoveInfo->physics.velocity.z;
            rdVector_Normalize3Acc(&newVel);

            newVel.x = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pMoveInfo->physics.velocity, &pMoveInfo->physics.velocity)), newVel.x);
            newVel.y = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pMoveInfo->physics.velocity, &pMoveInfo->physics.velocity)), newVel.y);
            newVel.z = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pMoveInfo->physics.velocity, &pMoveInfo->physics.velocity)), newVel.z);
            rdVector_Copy3(&pMoveInfo->physics.velocity, &newVel);
        }
        else
        {
            amount = impactDot * impactScale;
            rdVector_ScaleAdd3Acc(&pMoveInfo->physics.velocity, pHitNorm, amount);
        }

        if ( !a4 && impactDot > 1.51f )
        {
            // Indy's debug build tests this and does nothing (Jedi Knight calls sithCollision_FallHurt here).
        }
    }

    amount = hitDot * impactScale;
    rdVector_ScaleAdd3Acc(&pThing->moveDir, pHitNorm, amount);
    return 1;
}



/* Linker-discarded (/OPT:REF). Its only surviving trace is the orphaned pair of
   read-only pool constants 1.51f (0x00507AEC) and 45.0f (0x00507AF0), which sit
   between 1.9f sithCollision_HandleThingHitSurface and -0.025f
   sithCollision_FindActivatedThing in VC5's first-use pool order. Signature and
   body are the debug build's sub_4DA7DB, which Jedi Knight's
   sithCollision_FallHurt (the name) agrees with: void (thing, speed), damage
   applied only above 1.0. Locals are float, as the 116-byte pool requires.
 */
void J3DAPI sithCollision_FallHurt(SithThing* pThing, float speed)
{
    float damage;

    speed = speed - 1.51f;
    damage = RD_MULTIPLY(RD_MULTIPLY(speed, speed), 45.0f);
    if ( damage > 1.0f )
    {
        sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_HITDAMAGED);
        sithThing_DamageThing(pThing, pThing, damage, SITH_DAMAGE_CRUSH);
    }
}

SithThing* J3DAPI sithCollision_FindActivatedThing(SithThing* pThing, int* pbFoundFloorItem)
{
    rdVector3 moveNorm;
    SithThing* pThingCollided;
    SithSector* pSector;
    float radius;
    float moveDist;
    SithCollision* pCollision;
    rdVector3 startPos;
    int curWeaponID;
    int searchFlags;

    searchFlags = 0x04;
    curWeaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    moveDist = 0.08f;
    if ( curWeaponID && curWeaponID != SITHWEAPON_ZIPPO )
    {
        return 0;
    }

#line 2690
    SITH_ASSERTREL(pThing);
    radius = 0.025f;
    if ( !pThing->pInSector )
    {
        return 0;
    }

    rdVector_Copy3(&moveNorm, &pThing->orient.lvec);
    rdVector_Copy3(&startPos, &pThing->pos);
    startPos.z = startPos.z + radius;

    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, NULL, &startPos, &moveNorm, moveDist, radius, searchFlags);
    pCollision = sithCollision_PopStack();
    if ( pCollision )
    {
        while ( pCollision )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
#line 2718
                SITH_ASSERTREL(pCollision->pSurfaceCollided == NULL);
#line 2719
                SITH_ASSERTREL(pCollision->pThingCollided != NULL);
                pThingCollided = pCollision->pThingCollided;
                if ( pThingCollided->type == SITH_THING_ITEM )
                {
                    sithCollision_DecreaseStackLevel();
                    *pbFoundFloorItem = 0;
                    return pThingCollided;
                }
            }

            pCollision = sithCollision_PopStack();
        }
    }

    sithCollision_DecreaseStackLevel();
    moveNorm.x = 0.0f;
    moveNorm.y = 0.0f;
    moveNorm.z = -1.0f;

    rdVector_ScaleAdd3(&startPos, &pThing->orient.lvec, 0.025f, &pThing->pos);
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, NULL, &startPos, &moveNorm, moveDist, radius, searchFlags);
    pCollision = sithCollision_PopStack();
    if ( pCollision )
    {
        while ( pCollision )
        {
            if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
            {
#line 2752
                SITH_ASSERTREL(pCollision->pSurfaceCollided == NULL);
#line 2753
                SITH_ASSERTREL(pCollision->pThingCollided != NULL);
                pThingCollided = pCollision->pThingCollided;
                if ( pThingCollided->type == SITH_THING_ITEM )
                {
                    sithCollision_DecreaseStackLevel();
                    *pbFoundFloorItem = 1;
                    return pThingCollided;
                }
            }

            pCollision = sithCollision_PopStack();
        }
    }

    sithCollision_DecreaseStackLevel();
    return 0;
}

static __inline float J3DAPI sithCollision_BuildCollisionList(SithSector* pSector, SithThing* pThing, const rdVector3* startPos, const rdVector3* moveNorm, float moveDist, float radius, int searchFlags)
{
    float v8;

    if ( sithCollision_CheckSectorSearched(pSector) || !sithCollision_AddSearchedSector(pSector) )
    {
        return moveDist;
    }

    v8 = moveDist;
    if ( (searchFlags & 1) == 0 )
    {
        v8 = sithCollision_SearchForThingCollisions(pSector, pThing, startPos, moveNorm, moveDist, radius, searchFlags);
    }

    if ( (searchFlags & 0x8000) != 0 )
    {
        sithCollision_SearchForSurfaceCollisions(pSector, startPos, moveNorm, moveDist, radius, searchFlags);
    }
    else
    {
        sithCollision_SearchForSurfaceCollisions(pSector, startPos, moveNorm, v8, radius, searchFlags);
    }

    return v8;
}

static __inline int J3DAPI sithCollision_IsThingCollidable(const SithThing* pThing, int searchFlags)
{
    if ( pThing->collide.type == SITH_COLLIDE_NONE || (pThing->flags & (SITH_TF_DISABLED | SITH_TF_DESTROYED)) != 0 )
    {
        return 0;
    }

    if ( (searchFlags & 0x2000) != 0 && pThing->type != SITH_THING_COG && !streq(pThing->aName, "killtruk") )
    {
        return 0;
    }

    return 1;
}

static __inline int J3DAPI sithCollision_CanThingsCollide(const SithThing* pThing1, const SithThing* pThing2, int collflags)
{
    if ( pThing1 == pThing2 || !aCollideResults[pThing1->type][pThing2->type].pProcessFunc )
    {
        return 0;
    }

    if ( (pThing1->flags & SITH_TF_DYING) != 0 || (pThing2->flags & SITH_TF_DYING) != 0 )
    {
        return 0;
    }

    if ( pThing1->type == SITH_THING_WEAPON
        && (pThing1->thingInfo.weaponInfo.flags & SITH_WF_NOSHOOTERDAMAGE) != 0
        && (pThing1->pParent && pThing2->pParent && pThing1->pParent == pThing2->pParent && pThing1->parentSignature == pThing2->parentSignature
            || pThing1->pParent == pThing2 && pThing1->parentSignature == pThing2->signature) )
    {
        return 0;
    }

    if ( pThing2->type == SITH_THING_WEAPON
        && (pThing2->thingInfo.weaponInfo.flags & SITH_WF_NOSHOOTERDAMAGE) != 0
        && (pThing2->pParent && pThing1->pParent && pThing2->pParent == pThing1->pParent && pThing2->parentSignature == pThing1->parentSignature
            || pThing2->pParent == pThing1 && pThing2->parentSignature == pThing1->signature) )
    {
        return 0;
    }

    if ( (pThing1->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0 && pThing1->attach.attachedToStructure.pThingAttached == pThing2 )
    {
        if ( (pThing1->attach.flags & SITH_ATTACH_NOMOVE) != 0 || (collflags & 0x40) != 0 )
        {
            return 0;
        }

        if ( pThing1->type == SITH_THING_ACTOR && (pThing1->thingInfo.actorInfo.flags & SITH_AF_UNKNOWN_20000000) != 0 )
        {
            return 0;
        }
    }

    if ( (pThing2->attach.flags & (SITH_ATTACH_THING | SITH_ATTACH_THINGFACE)) != 0
        && pThing2->attach.attachedToStructure.pThingAttached == pThing1 )
    {
        if ( (pThing2->attach.flags & SITH_ATTACH_NOMOVE) != 0 || (collflags & 0x40) != 0 )
        {
            return 0;
        }
        if ( pThing2->type == SITH_THING_ACTOR && (pThing2->thingInfo.actorInfo.flags & SITH_AF_UNKNOWN_20000000) != 0 )
        {
            return 0;
        }
    }

    if ( (pThing1->attach.flags & SITH_ATTACH_TAIL) != 0 && (pThing2->attach.flags & SITH_ATTACH_TAIL) != 0 )
    {
        return 0;
    }

    if ( (pThing1->pAttachedThing && (pThing1->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 || (pThing1->attach.flags & SITH_ATTACH_TAIL) != 0)
        && pThing2->controlType == SITH_CT_AI )
    {
        return 0;
    }

    if ( (pThing2->pAttachedThing && (pThing2->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 || (pThing2->attach.flags & SITH_ATTACH_TAIL) != 0)
        && pThing1->controlType == SITH_CT_AI )
    {
        return 0;
    }

    if ( (pThing1->pAttachedThing && (pThing1->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 || (pThing1->attach.flags & SITH_ATTACH_TAIL) != 0)
        && pThing2->type == SITH_THING_PLAYER
        && (pThing2->moveStatus == SITHPLAYERMOVE_JEWELFLYING || pThing2->moveStatus == SITHPLAYERMOVE_JEWELFLYING_UNKN1) )
    {
        return 0;
    }

    if ( (pThing2->pAttachedThing && (pThing2->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 || (pThing2->attach.flags & SITH_ATTACH_TAIL) != 0)
        && pThing1->type == SITH_THING_PLAYER
        && (pThing1->moveStatus == SITHPLAYERMOVE_JEWELFLYING || pThing1->moveStatus == SITHPLAYERMOVE_JEWELFLYING_UNKN1) )
    {
        return 0;
    }

    if ( pThing1->moveType == SITH_MT_PHYSICS
        && (pThing1->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) != 0
        && pThing2->moveStatus == SITHPLAYERMOVE_RUNOVER )
    {
        return 0;
    }

    if ( pThing2->moveType == SITH_MT_PHYSICS
        && (pThing2->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) != 0
        && pThing1->moveStatus == SITHPLAYERMOVE_RUNOVER )
    {
        return 0;
    }

    return 1;
}

static __inline void J3DAPI sithCollision_ResetCurStack(void)
{
    aNumStackEntries[stackLevel] = 0;
    aNumSectors[stackLevel] = 0;
}

static __inline int J3DAPI sithCollision_AddSearchedSector(SithSector* pSector)
{
    if ( aNumSectors[stackLevel] == STD_ARRAYLEN(apSearchedSectors[stackLevel]) )
    {
#line 3042
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to search too many sectors in collision system.\n", 0, 0, 0, 0);
        return 0;
    }

    apSearchedSectors[stackLevel][aNumSectors[stackLevel]++] = pSector;
    return 1;
}

static __inline int J3DAPI sithCollision_CheckSectorSearched(const SithSector* pSector)
{
    unsigned int i;

    for ( i = 0; i < aNumSectors[stackLevel]; ++i )
    {
        if ( apSearchedSectors[stackLevel][i] == pSector )
        {
            return 1;
        }
    }

    return 0;
}

void J3DAPI sithCollision_PushThingCollision(SithThing* pHitThing, float distance, SithCollisionType hitType, rdModel3Mesh* pHitMesh, rdFace* pHitFace, rdVector3* hitNorm)
{
    SithCollision* pCollision;

#line 3107
    SITH_ASSERTREL(((hitType) & (SITHCOLLISION_THING)));
#line 3108
    SITH_ASSERTREL(hitNorm);
    if ( aNumStackEntries[stackLevel] == STD_ARRAYLEN(aCollisions[stackLevel]) )
    {
#line 3112
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Found too many collisions in collision system.\n", 0, 0, 0, 0);
        return;
    }

    pCollision = &aCollisions[stackLevel][aNumStackEntries[stackLevel]++];
    pCollision->pSurfaceCollided = NULL;
    pCollision->bEnumerated      = 0;
    pCollision->type             = hitType;
    pCollision->distance         = distance;
    pCollision->pThingCollided   = pHitThing;
    pCollision->pMeshCollided    = pHitMesh;
    pCollision->pFaceCollided    = pHitFace;

    rdVector_Copy3(&pCollision->hitNorm, hitNorm);
}

void J3DAPI sithCollision_PushSurfaceCollision(SithSurface* pSurf, float distance, SithCollisionType hitType, rdVector3* pHitNorm)
{
    SithCollision* pCollision;

#line 3158
    SITH_ASSERTREL(((hitType) & (SITHCOLLISION_WORLD | SITHCOLLISION_ADJOINTOUCH | SITHCOLLISION_ADJOINCROSS)));
    if ( aNumStackEntries[stackLevel] == STD_ARRAYLEN(aCollisions[stackLevel]) )
    {
#line 3162
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Found too many collisions in collision system.\n", 0, 0, 0, 0);
        return;
    }

    pCollision = &aCollisions[stackLevel][aNumStackEntries[stackLevel]++];
    pCollision->pThingCollided   = NULL;
    pCollision->bEnumerated      = 0;
    pCollision->type             = hitType;
    pCollision->distance         = distance;
    pCollision->pSurfaceCollided = pSurf;
    if ( pHitNorm )
    {
        rdVector_Copy3(&pCollision->hitNorm, pHitNorm);
    }
}

static __inline SithCollision* J3DAPI sithCollision_PopClosest(void)
{
    SithCollision* pCollision;
    float distance;
    unsigned int collNum;
    SithCollision* pClosest;

    pClosest = 0;
    distance = 3.4028235e38f;
    for ( collNum = 0, pCollision = aCollisions[stackLevel]; collNum < aNumStackEntries[stackLevel]; ++pCollision, ++collNum )
    {
        if ( !pCollision->bEnumerated )
        {
            if ( pCollision->distance < distance )
            {
                pClosest = pCollision;
                distance = pCollision->distance;
            }
            else if ( pCollision->distance == distance )
            {
#line 3221
                SITH_ASSERTREL(pClosest != NULL);
                if ( (pClosest->type & (SITHCOLLISION_FACEVERTEX | SITHCOLLISION_FACEEDGE)) != 0 && (pCollision->type & SITHCOLLISION_FACE) != 0 )
                {
                    pClosest = pCollision;
                }
            }
        }
    }

    return pClosest;
}

static __inline int J3DAPI sithCollision_CheckSearchFilter(SithThing* pThing1, SithThing* pThing2)
{
    float dot;

    if ( (pThing2->flags & SITH_TF_JEEPSTOP) != 0 )
    {
        if ( pThing1->type == SITH_THING_PLAYER && (pThing1->moveInfo.physics.flags & SITH_PF_JEEP) != 0 )
        {
            dot = rdVector_Dot3(&pThing2->orient.lvec, &pThing1->moveInfo.physics.velocity);
            if ( dot < 0.0f )
            {
                return 1;
            }
        }

        return 0;
    }

    if ( aCollideResults[pThing1->type][pThing2->type].pSearchFunc )
    {
        return aCollideResults[pThing1->type][pThing2->type].pSearchFunc(pThing1, pThing2);
    }

    return 1;
}

static __inline int J3DAPI sithCollision_CanPassAdjoin(SithSurfaceAdjoinFlag adjflags, int colflags)
{
    if ( (colflags & 4) != 0 )
    {
        return 1;
    }

    if ( (colflags & 0x1100) != 0 && (adjflags & SITH_ADJOIN_VISIBLE) == 0 )
    {
        return 0;
    }

    if ( (colflags & 0x200) != 0 && (colflags & 0x100) == 0 && (adjflags & SITH_ADJOIN_NOPLAYERMOVE) != 0 )
    {
        return 0;
    }

    if ( (colflags & 0x100) == 0 && (adjflags & SITH_ADJOIN_MOVE) == 0 )
    {
        return 0;
    }

    return 1;
}

int J3DAPI sithCollision_ParticleAndActorCollisionHandler(SithThing* pSrcThing, SithThing* pThingCollided, SithCollision* pCollision, int bSecondThingIsSource)
{
    float mass;

    float curdirDot;

    mass = pSrcThing->moveInfo.physics.mass;
    if ( bSecondThingIsSource )
    {
        return sithCollision_ThingCollisionHandler(pSrcThing, pThingCollided, pCollision, bSecondThingIsSource);
    }

    if ( pSrcThing->moveType == SITH_MT_PHYSICS )
    {
        curdirDot = -rdVector_Dot3(&pSrcThing->moveInfo.physics.velocity, &pCollision->hitNorm);
    }

    if ( !sithCollision_ThingCollisionHandler(pSrcThing, pThingCollided, pCollision, bSecondThingIsSource) )
    {
        return 0;
    }

    if ( curdirDot > 0.25f )
    {
        sithThing_DamageThing(pThingCollided, pSrcThing, RD_MULTIPLY(curdirDot, RD_MULTIPLY(mass, 0.3f)), SITH_DAMAGE_IMPACT);
    }

    return 1;
}

float J3DAPI sithCollision_CheckDistance(SithThing* pThing, const rdVector3* moveNorm)
{
    SithThing* pHitThing;
    rdVector3 dir;
    SithSurface* pSurfaceCollided;
    SithCollision* pCollision;
    float distance;
    float moveDist;
    int searchFlags;

    moveDist = 10.0f;
    searchFlags = 0x210;
    distance = moveDist;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, moveNorm, moveDist, pThing->collide.movesize, searchFlags);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {

        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
#line 3398
            SITH_ASSERTREL(pCollision->pThingCollided == NULL);
#line 3399
            SITH_ASSERTREL(pCollision->pSurfaceCollided != NULL);
            pSurfaceCollided = pCollision->pSurfaceCollided;
            if ( (pSurfaceCollided->flags & SITH_SURFACE_ISFLOOR) != 0 && pCollision->distance < distance )
            {
                distance = pCollision->distance;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
        {
            pHitThing = pCollision->pThingCollided;
#line 3414
            SITH_ASSERTREL(pHitThing);
            if ( pHitThing == pThing )
            {
                continue;
            }

            if ( !pCollision->pFaceCollided || !pCollision->pMeshCollided )
            {
#line 3421
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Hit standon object but not enough collide info to attach.\n", 0, 0, 0, 0);
                sithCollision_DecreaseStackLevel();
                return 100.0f;
            }

            rdMatrix_TransformVector34(&dir, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);
            if ( rdVector_Dot3(&dir, &rdroid_zVector3) < 0.6f )
            {
                continue;
            }

            if ( pCollision->distance < distance )
            {
                distance = pCollision->distance;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    return distance;
}

int J3DAPI sithCollision_ActorPlayerSearchFilter(SithThing* pThing1, SithThing* pThing2)
{
    if ( pThing1->type == SITH_THING_PLAYER && (pThing1->moveInfo.physics.flags & (SITH_PF_JEEP | SITH_PF_MINECAR)) != 0 && pThing2->type == SITH_THING_ACTOR )
    {
        return 0;
    }

    if ( (pThing1->pAttachedThing && (pThing1->pAttachedThing->attach.flags & SITH_ATTACH_TAIL) != 0 || (pThing1->attach.flags & SITH_ATTACH_TAIL) != 0)
        && pThing2->type == SITH_THING_PLAYER )
    {
        return 0;
    }

    return 1;
}
