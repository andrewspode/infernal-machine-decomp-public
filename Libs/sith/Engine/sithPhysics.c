#include "sithPhysics.h"

#include <sith/AI/sithAIAwareness.h>
#ifdef J3D_DEBUG
#include <sith/Devices/sithConsole.h>
#endif
#include <sith/Devices/sithControl.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithActor.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithSoundClass.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_1, 28) /* end count-fit */
#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/std.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SITHPHYSICS_ABS(x)                ((x) < 0.0f ? -(x) : (x))
#define SITHPHYSICS_SIGN(x)               ((x) < 0.0f ? -1 : 1)
#define SITHPHYSICS_CLIPPRECISION3(v) \
    (v)->x = stdMath_ClipNearZero((v)->x); \
    (v)->y = stdMath_ClipNearZero((v)->y); \
    (v)->z = stdMath_ClipNearZero((v)->z)
#define SITHPHYSICS_ISZERO3(v)            ((v)->x == 0.0f && (v)->y == 0.0f && (v)->z == 0.0f)

// Side classification of a track face vertex relative to the face center
#define SITHPHYSICS_TRACKVERT_LEFT  0x1
#define SITHPHYSICS_TRACKVERT_RIGHT 0x2
#define SITHPHYSICS_TRACKVERT_FRONT 0x4
#define SITHPHYSICS_TRACKVERT_BACK  0x8

struct sSithPhysicsTrackFaceData
{
    rdVector3 aVertices[4];
    float aForwardDists[4];    // Vertex distance from center along the forward axis
    float aRightDists[4];      // Vertex distance from center along the right axis
    int aVertexSides[4];       // SITHPHYSICS_TRACKVERT_* flags
    rdVector3 aEdges[4];
    float aEdgeForwardDots[4];
    float aEdgeRightDots[4];
    size_t nVertices;
    rdVector3 frontLeft;
    rdVector3 frontRight;
    rdVector3 backLeft;
    rdVector3 backRight;
    rdVector3 front;           // Middle of front edge
    rdVector3 back;            // Middle of back edge
    rdVector3 middle;
    rdVector3 center;          // Average of face vertices
    rdVector3 forward;         // Normalized back to front direction
    rdVector3 right;
    float distanceToFront;
    float distanceFromBack;
};

static void J3DAPI sithPhysics_UpdateThingPhysics(SithThing* pThing, float secDeltaTime);
static __inline void J3DAPI sithPhysics_GetAttachedSurfaceInfo(SithThing* pThing, rdVector3* pOutNormal, float* pOutDistance, float* pOutDragFactor);
static void J3DAPI sithPhysics_InitMineCarExhaustDefault(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo);

int sithPhysics_dword_58540C = 0;
float sithPhysics_flt_585410 = 0;

void J3DAPI sithPhysics_FindFloor(SithThing* pThing, int bNoThingStateUpdate)
{
    float radius = 0.0f;
    SithPhysicsFlags physFlags = pThing->moveInfo.physics.flags;
    int curSearchFlags = 0;
    rdVector3 moveNorm;
    float moveDelta;
    SithCollision* pCollision;

    if ( !pThing )
    {
#line 205
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NULL thing pointer in FindFloor()", 0, 0, 0, 0);
        return;
    }

    if ( pThing->moveType != SITH_MT_PHYSICS )
    {
#line 210
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing %s not physics type in FindFloor()", pThing->aName, 0, 0, 0);
        return;
    }

    if ( !pThing->pInSector )
    {
        return;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
    {
        if ( pThing->type == SITH_THING_PLAYER )
        {
            sithPhysics_FindWaterSurface(pThing);
        }

        return;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        if ( pThing->attach.flags )
        {
            sithThing_DetachThing(pThing);
        }

        return;
    }

    curSearchFlags |= 0x2800;
    if ( pThing->type == SITH_THING_PLAYER && (physFlags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP)) == 0 )
    {
        curSearchFlags |= 0x200;
    }

    if ( (physFlags & SITH_PF_WALLSTICK) != 0 )
    {
        rdVector_Neg3(&moveNorm, &pThing->orient.uvec);
        curSearchFlags |= 0x02;
    }
    else if ( (physFlags & SITH_PF_RAFT) != 0 )
    {
        rdVector_Neg3(&moveNorm, &rdroid_zVector3);
        curSearchFlags |= 0x01;
    }
    else
    {
        curSearchFlags |= 0x10;
        rdVector_Neg3(&moveNorm, &rdroid_zVector3);
    }

    if ( !bNoThingStateUpdate && !pThing->attach.flags )
    {
        moveDelta = sithPhysics_GetThingHeight(pThing);
        if ( moveDelta == 0.0f )
        {
            moveDelta = pThing->collide.movesize + 0.005f;
        }
    }
    else
    {
        moveDelta = sithPhysics_GetThingHeight(pThing);
        if ( (physFlags & (SITH_PF_FLOORSTICK | SITH_PF_WALLSTICK)) != 0 )
        {
            moveDelta = RD_MULTIPLY(moveDelta, 2.5f);
        }
        else
        {
            moveDelta = RD_MULTIPLY(moveDelta, 1.1f);
        }
    }

    if ( moveDelta <= 0.0f )
    {
    }
    else
    {
        for ( ;; )
        {
            sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDelta, radius, curSearchFlags);
            while ( (pCollision = sithCollision_PopStack()) != NULL )
            {
                if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
                {
                    if ( (physFlags & SITH_PF_RAFT) != 0 )
                    {
                        if ( (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
                            && (pCollision->pSurfaceCollided->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
                        {
                            sithThing_AttachThingToSurface(pThing, pCollision->pSurfaceCollided, bNoThingStateUpdate);
                            sithCollision_DecreaseStackLevel();
                            return;
                        }
                    }
                    else
                    {
                        sithCollision_DecreaseStackLevel();
                        sithThing_AttachThingToSurface(pThing, pCollision->pSurfaceCollided, bNoThingStateUpdate);
                        return;
                    }
                }
                else if ( (pCollision->type & SITHCOLLISION_THING) != 0 )
                {
                    SithThing* pHitThing = pCollision->pThingCollided;
                    rdVector3 hitDir;
#line 309
                    SITH_ASSERTREL(pHitThing);
                    if ( pHitThing == pThing )
                    {
                        continue;
                    }

                    if ( !pCollision->pFaceCollided || !pCollision->pMeshCollided )
                    {
#line 316
                        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Hit standon object but not enough collide info to attach.\n", 0, 0, 0, 0);
                        sithCollision_DecreaseStackLevel();
                        return;
                    }

                    if ( (curSearchFlags & 0x10) != 0 )
                    {
                        rdMatrix_TransformVector34(&hitDir, &pCollision->pFaceCollided->normal, &pCollision->pThingCollided->orient);
                        if ( rdVector_Dot3(&hitDir, &rdroid_zVector3) < 0.6f )
                        {
                            continue;
                        }
                    }

                    sithThing_AttachThingToThingFace(pThing, pHitThing, pCollision->pFaceCollided, pCollision->pMeshCollided->apVertices, bNoThingStateUpdate);
                    sithCollision_DecreaseStackLevel();
                    return;
                }
                else if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
                {
                    if ( (physFlags & SITH_PF_RAFT) != 0 )
                    {
#line 339
                        SITH_ASSERTREL(pCollision->pSurfaceCollided);
                        SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin);
                        if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
                        {
                            sithThing_AttachThingToSurface(pThing, pCollision->pSurfaceCollided, bNoThingStateUpdate);
                            sithCollision_DecreaseStackLevel();
                            return;
                        }
                    }
                }
            }

            sithCollision_DecreaseStackLevel();
            if ( radius == 0.0f && (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER) && pThing->collide.movesize != 0.0f )
            {
                if ( (physFlags & SITH_PF_RAFT) == 0 )
                {
                    if ( pThing->type == SITH_THING_PLAYER && pThing->moveStatus == SITHPLAYERMOVE_RUNNING && sithPuppet_bPlayerLeapForward )
                    {
                        radius   = pThing->collide.movesize;
                        moveDelta = moveDelta - radius;
                        moveDelta = J3DMAX(moveDelta, 0.05f);
                    }
                    else
                    {
                        radius   = 0.005f;
                        moveDelta = moveDelta - radius;
                        moveDelta = J3DMAX(moveDelta, 0.05f);
                    }

                    continue;
                }
                else
                {
                    radius   = 0.001f;
                    moveDelta = moveDelta * 2.0f;
                    continue;
                }
            }

            break;
        }
    }

    if ( pThing->attach.flags )
    {
        sithThing_DetachThing(pThing);
        if ( (physFlags & SITH_PF_JEEP) == 0 && (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && !pThing->thingInfo.actorInfo.bForceMovePlay )
        {
            if ( pThing->type == SITH_THING_PLAYER )
            {
                if ( (physFlags & SITH_PF_RAFT) == 0 && (pThing->moveStatus == SITHPLAYERMOVE_CRAWL_STILL || pThing->moveInfo.physics.height < 0.09f) )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_NORMAL);
                    pThing->collide.movesize = 0.04f;
                    pThing->moveInfo.physics.flags &= ~SITH_PF_ALIGNSURFACE;
                    pThing->moveInfo.physics.flags |= SITH_PF_ALIGNUP;
                    pThing->moveInfo.physics.height = 0.09f;
                    sithInventory_SetSwimmingInventory(pThing, 1);
                }
                else if ( (pThing->type == SITH_THING_PLAYER && pThing->moveStatus == SITHPLAYERMOVE_RUNNING && sithPuppet_bPlayerLeapForward == 1) || pThing->moveStatus == SITHPLAYERMOVE_LEAPFWD )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_LEAPFWD;
                    sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LEAPLEFT, NULL);
                    sithPuppet_bPlayerLeapForward = 0;
                    sithPlayerActions_LeapForward(pThing);
                    return;
                }
            }

            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        }
    }
}

void J3DAPI sithPhysics_FindWaterSurface(SithThing* pThing)
{
    SithCollision* pCollision;

    pThing->attach.distToWaterSurface = 0.1f;
    sithCollision_SearchForCollisions(pThing->pInSector, pThing, &pThing->pos, &rdroid_zVector3, 0.1f, 0.0f, 1);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
#line 471
            SITH_ASSERTREL(pCollision->pSurfaceCollided);
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin);
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
            {
                if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
                {
                    sithSoundClass_PlayModeFirst(pThing, SITHSOUNDCLASS_EXITWATER);
                }

                pThing->moveInfo.physics.flags |= SITH_PF_ONWATERSURFACE;
                pThing->attach.distToWaterSurface = pCollision->distance;
                sithCollision_DecreaseStackLevel();
                return;
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    pThing->moveInfo.physics.flags &= ~SITH_PF_ONWATERSURFACE;
}

void J3DAPI sithPhysics_UpdateThing(SithThing* pThing, float secDeltaTime)
{
    int bDoUpdate = 1;  // unreferenced; the debug build's frame keeps its slot
    float secElapsed = secDeltaTime;  // unreferenced; the debug build's frame keeps its slot
    SithPhysicsInfo* pPhysics;

#line 522
    SITH_ASSERTREL(sithThing_ValidateThingPointer(sithWorld_pCurrentWorld, pThing));
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    if ( !pThing->pInSector )
    {
        return;
    }

    if ( pThing->type == SITH_THING_ACTOR && pThing->thingInfo.actorInfo.bForceMovePlay == 1 )
    {
        return;
    }

    if ( (pThing->moveInfo.physics.flags & SITH_PF_NOUPDATE) != 0 )
    {
        return;
    }

    pPhysics = &pThing->moveInfo.physics;
    memset(&pPhysics->deltaVelocity, 0, sizeof(rdVector3));
    memset(&pPhysics->gravityForce, 0, sizeof(rdVector3));
    if ( (pThing->type == SITH_THING_ACTOR || pThing->type == SITH_THING_PLAYER) && (pThing->thingInfo.actorInfo.flags & SITH_AF_IMMOBILE) != 0 )
    {
        memset(&pPhysics->thrust, 0, sizeof(rdVector3));
    }

    if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 && pThing->moveStatus == SITHPLAYERMOVE_HANGING )
    {
        sithPhysics_UpdateClimbingThingPhysics(pThing, secDeltaTime);
    }
    else if ( (pPhysics->flags & SITH_PF_JEEP) != 0 )
    {
        sithPhysics_UpdateJeepPhysics(pThing, secDeltaTime);
        if ( ((pThing->idx + sithMain_frameNumber) & 3) == 0 )
        {
            sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 2, 4.0f, pThing);
        }
    }
    else if ( (pPhysics->flags & SITH_PF_MINECAR) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        sithPhysics_UpdateMineCarPhysics(pThing, secDeltaTime);
        if ( ((pThing->idx + sithMain_frameNumber) & 7) == 0 )
        {
            sithAIAwareness_CreateTransmittingEvent(pThing->pInSector, &pThing->pos, 2, 2.0f, pThing);
        }
    }
    else if ( (pPhysics->flags & SITH_PF_RAFT) != 0 && (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        sithPhysics_UpdateRaftPhysics(pThing, secDeltaTime);
    }
    else if ( (pThing->attach.flags & (SITH_ATTACH_SURFACE | SITH_ATTACH_THINGFACE)) != 0 )
    {
        sithPhysics_UpdateAttachedThingPhysics(pThing, secDeltaTime);
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        sithPhysics_UpdateClimbingThingPhysics(pThing, secDeltaTime);
    }
    else if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        if ( pThing->type != SITH_THING_PLAYER && (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
        {
            sithPhysics_UpdateThingPhysics(pThing, secDeltaTime);
        }
        else
        {
            sithPhysics_UpdateUnderwaterThingPhysics(pThing, secDeltaTime);
        }
    }
    else if ( pThing->type == SITH_THING_PLAYER )
    {
        sithPhysics_UpdatePlayerPhysics(pThing, secDeltaTime);
    }
    else
    {
        sithPhysics_UpdateThingPhysics(pThing, secDeltaTime);
    }
}

void J3DAPI sithPhysics_ApplyForce(SithThing* pThing, const rdVector3* force)
{
    rdVector3 acc;
    float invMass;

#line 642
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    if ( pThing->moveType != SITH_MT_PHYSICS || pThing->moveInfo.physics.mass <= 0.0f )
    {
        return;
    }

    if ( SITHPHYSICS_ISZERO3(force) )
    {
        return;
    }

    invMass = D3DVAL(1.0f / (pThing->moveInfo.physics.mass));
    rdVector_Scale3(&acc, force, invMass);
    if ( acc.z > 0.5f )
    {
        sithThing_DetachThing(pThing);
    }

    rdVector_Add3Acc(&pThing->moveInfo.physics.velocity, &acc);
    pThing->moveInfo.physics.flags |= SITH_PF_UNKNOWN_8000;
}

void J3DAPI sithPhysics_SetThingLook(SithThing* pThing, const rdVector3* pNormal, float secDeltaTime)
{
    float dot;

    dot = 1.0f - rdVector_Dot3(pNormal, &pThing->orient.uvec);
    dot = stdMath_ClipNearZero(dot);
    if ( dot != 0.0f )
    {
        if ( secDeltaTime == 0.0f )
        {
            rdVector_Copy3(&pThing->orient.uvec, pNormal);
            rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec);
            rdVector_Normalize3Acc(&pThing->orient.rvec);
            rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);
            pThing->moveInfo.physics.flags |= SITH_PF_ALIGNED;
            return;
        }
        else
        {
            rdVector_ScaleAdd3Acc(&pThing->orient.uvec, pNormal, RD_MULTIPLY(secDeltaTime, 10.0f));
            rdVector_Normalize3Acc(&pThing->orient.uvec);
            rdVector_Cross3(&pThing->orient.lvec, &pThing->orient.uvec, &pThing->orient.rvec);
            rdVector_Normalize3Acc(&pThing->orient.lvec);
            rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &pThing->orient.uvec);
        }
    }
    else
    {
        pThing->moveInfo.physics.flags |= SITH_PF_ALIGNED;
    }
}

void J3DAPI sithPhysics_ApplyDrag(rdVector3* pVelocity, float drag, float mag, float secDeltaTime)
{
    float dragScale;

    if ( mag != 0.0f && rdVector_Len3(pVelocity) < mag )
    {
        memset(pVelocity, 0, sizeof(rdVector3));
    }
    else if ( drag != 0.0f )
    {
        dragScale = RD_MULTIPLY(drag, secDeltaTime);
        dragScale = J3DMIN(1.0f, dragScale);

        rdVector_ScaleAdd3Acc(pVelocity, pVelocity, -dragScale);
        SITHPHYSICS_CLIPPRECISION3(pVelocity);
    }
}

int J3DAPI sithPhysics_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum)
{
    int curflags;
    float value;

#line 791
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    switch ( adjNum )
    {
        case SITHTHING_ARG_SURFDRAG:
            value = atof(pArg->argValue);
            if ( value < 0.0f )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.surfDrag = value;
            break;

        case SITHTHING_ARG_AIRDRAG:
            value = atof(pArg->argValue);
            if ( value < 0.0f )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.airDrag = value;
            break;

        case SITHTHING_ARG_STATICDRAG:
            value = atof(pArg->argValue);
            if ( value < 0.0f )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.staticDrag = value;
            break;

        case SITHTHING_ARG_MASS:
            value = atof(pArg->argValue);
            if ( value < 0.0f )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.mass = value;
            break;

        case SITHTHING_ARG_HEIGHT:
            value = atof(pArg->argValue);
            if ( value < 0.0f )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.height = value;
            break;

        case SITHTHING_ARG_PHYSFLAGS:
            if ( sscanf(pArg->argValue, "%x", &curflags) != 1 )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.flags = curflags;
            break;

        case SITHTHING_ARG_MAXROTVEL:
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f || pThing->moveType != SITH_MT_PHYSICS )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.maxRotationVelocity = value;
            break;

        case SITHTHING_ARG_MAXVEL:
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f || pThing->moveType != SITH_MT_PHYSICS )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.maxVelocity = value;
            break;

        case SITHTHING_ARG_VEL:
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pThing->moveInfo.physics.velocity.x, &pThing->moveInfo.physics.velocity.y, &pThing->moveInfo.physics.velocity.z) != 3 )
            {
                goto bad_arg;
            }
            break;

        case SITHTHING_ARG_ANGVEL:
            if ( sscanf(pArg->argValue, "(%f/%f/%f)", &pThing->moveInfo.physics.angularVelocity.x, &pThing->moveInfo.physics.angularVelocity.y, &pThing->moveInfo.physics.angularVelocity.z) != 3 )
            {
                goto bad_arg;
            }
            break;

        case SITHTHING_ARG_ORIENTSPEED:
            value = D3DVAL(atof(pArg->argValue));
            if ( value < 0.0f || pThing->moveType != SITH_MT_PHYSICS )
            {
                goto bad_arg;
            }

            pThing->moveInfo.physics.orientSpeed = value;
            break;

        case SITHTHING_ARG_BUOYANCY:
            value = D3DVAL(atof(pArg->argValue));
            pThing->moveInfo.physics.buoyancy = value;
            break;

        default:
            return 0;
    }

    return 1;

bad_arg:
#line 881
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad argument %s=%s line %d.\n", pArg->argName, pArg->argValue, stdConffile_GetLineNumber(), 0);
    return 0;
}

void J3DAPI sithPhysics_ResetThingMovement(SithThing* pThing)
{
#line 900
    SITH_ASSERTREL((pThing != NULL) && (pThing->moveType == SITH_MT_PHYSICS));
    memset(&pThing->moveInfo.physics.velocity, 0, sizeof(rdVector3));
    memset(&pThing->moveInfo.physics.angularVelocity, 0, sizeof(rdVector3));
    memset(&pThing->moveInfo.physics.rotThrust, 0, sizeof(rdVector3));
    memset(&pThing->moveInfo.physics.thrust, 0, sizeof(rdVector3));
    memset(&pThing->moveInfo.physics.deltaVelocity, 0, sizeof(rdVector3));
    memset(&pThing->moveDir, 0, sizeof(rdVector3));
}

float J3DAPI sithPhysics_GetThingHeight(const SithThing* pThing)
{
    float height;

#line 931
    SITH_ASSERTREL(pThing->moveType == SITH_MT_PHYSICS);
    height = pThing->moveInfo.physics.height;
    if ( height != 0.0f )
    {
        return height;
    }

    if ( pThing->renderData.type == RD_THING_MODEL3 )
    {
        height = pThing->renderData.data.pModel3->insertOffset.z;
    }

    height = J3DMAX(height, pThing->collide.movesize + 0.005f);
    return height;
}


// Updates physics include gravity effect on thing
// Retail inlines this body into sithPhysics_UpdateAttachedThingPhysics below
// (0x481922-0x481FDE) while sithPhysics_UpdateThing above still calls it out of
// line: VC5's /Ob1 per-caller budget expands one copy of a body this size but
// not two (definition order does not decide it).
static __inline void J3DAPI sithPhysics_UpdateThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics;
    SithPhysicsFlags flags;
    rdMatrix34 mat;
    rdVector3 vecThrust;
    rdVector3 angVelocityDelta;
    int bCurUnused;
    static float sithPhysics_flt_538BE0 = 0.2f;

    pPhysics = &pThing->moveInfo.physics;
    flags    = pPhysics->flags;
    bCurUnused  = 0;
    memset(&vecThrust, 0, sizeof(rdVector3));
    memset(&pPhysics->gravityForce, 0, sizeof(rdVector3));

    if ( (flags & SITH_PF_RAFT) != 0 )
    {
        int currentfrontJoint = sithThing_GetThingJointIndex(pThing, "inraftfr");
        int backJoint  = sithThing_GetThingJointIndex(pThing, "inraftbk");
        pThing->renderData.apTweakedAngles[currentfrontJoint].x = pThing->renderData.apTweakedAngles[currentfrontJoint].x - RD_MULTIPLY(stdMath_NormalizeAngleAcute(pThing->renderData.apTweakedAngles[currentfrontJoint].x), 0.5f);
        pThing->renderData.apTweakedAngles[backJoint].x  = pThing->renderData.apTweakedAngles[backJoint].x - RD_MULTIPLY(stdMath_NormalizeAngleAcute(pThing->renderData.apTweakedAngles[backJoint].x), 0.5f);
        sithPhysics_SetThingLook(pThing, &rdroid_zVector3, sithPhysics_flt_538BE0 * secDeltaTime);
    }

    if ( (flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
        {
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, pPhysics->airDrag + 0.2f, 0.0f, secDeltaTime);
        }

        rdVector_ScaleAdd3Acc(&pPhysics->angularVelocity, &pPhysics->rotThrust, secDeltaTime);
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity);
        SITHPHYSICS_CLIPPRECISION3(&pPhysics->angularVelocity);
    }

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
    {
        rdVector_Scale3(&angVelocityDelta, &pPhysics->angularVelocity, secDeltaTime);
    }
    else
    {
        memset(&angVelocityDelta, 0, sizeof(rdVector3));
    }

    if ( !SITHPHYSICS_ISZERO3(&angVelocityDelta) )
    {
        rdMatrix_BuildRotate34(&mat, &angVelocityDelta);
        sithCollision_RotateThing(pThing, &mat);
        if ( (flags & SITH_PF_FLY) != 0 )
        {
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &mat);
        }

        if ( ((pThing->idx + sithMain_frameNumber) & 7) == 0 )
        {
            rdMatrix_Normalize34(&pThing->orient);
        }
    }

    if ( pPhysics->airDrag != 0.0f )
    {
        sithPhysics_ApplyDrag(&pPhysics->velocity, pPhysics->airDrag, 0.0f, secDeltaTime);
    }

    if ( (flags & SITH_PF_USETHRUST) != 0 )
    {
        if ( (flags & SITH_PF_FLY) == 0 && pThing->moveStatus != SITHPLAYERMOVE_FALLING )
        {
            rdVector_Scale3Acc(&pPhysics->thrust, 0.3f);
        }

        rdVector_Scale3(&vecThrust, &pPhysics->thrust, secDeltaTime);
        rdMatrix_TransformVector34Acc(&vecThrust, &pThing->orient);
    }

    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (flags & SITH_PF_NOTHRUST) == 0 )
    {
        rdVector_ScaleAdd3Acc(&vecThrust, &pThing->pInSector->thrust, secDeltaTime);
    }

    if ( pPhysics->mass != 0.0f
        && (flags & SITH_PF_USEGRAVITY) != 0
        && (pThing->pInSector->flags & SITH_SECTOR_NOGRAVITY) == 0 )
    {
        float gravity = RD_MULTIPLY(sithWorld_pCurrentWorld->gravity, secDeltaTime);
        if ( (pPhysics->flags & SITH_PF_PARTIALGRAVITY) != 0 )
        {
            gravity = RD_MULTIPLY(gravity, 0.5f);
        }

        vecThrust.z = vecThrust.z - gravity;
        pPhysics->gravityForce.z = -gravity;
    }

    rdVector_Add3Acc(&pPhysics->velocity, &vecThrust);
    SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }
}

// update player physics in the air or water/aeterium
// local variable allocation has failed, the output may be wrong!
void J3DAPI sithPhysics_UpdatePlayerPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics;
    size_t numSteps;
    rdMatrix34 mat;
    rdVector3 vecThrust;
    rdVector3 angVelocityDelta;
    int bCurUnused;
    SithPhysicsFlags flags;
    size_t i;
    static float sithPhysics_flt_538BE4 = 0.2f;

    pPhysics = &pThing->moveInfo.physics;
    flags    = pPhysics->flags;
    bCurUnused  = 0;
    memset(&pPhysics->gravityForce, 0, sizeof(rdVector3));

    if ( (flags & SITH_PF_RAFT) != 0 )
    {
        int currentfrontJoint = sithThing_GetThingJointIndex(pThing, "inraftfr");
        int backJoint  = sithThing_GetThingJointIndex(pThing, "inraftbk");
        pThing->renderData.apTweakedAngles[currentfrontJoint].x = pThing->renderData.apTweakedAngles[currentfrontJoint].x - RD_MULTIPLY(stdMath_NormalizeAngleAcute(pThing->renderData.apTweakedAngles[currentfrontJoint].x), 0.5f);
        pThing->renderData.apTweakedAngles[backJoint].x  = pThing->renderData.apTweakedAngles[backJoint].x - RD_MULTIPLY(stdMath_NormalizeAngleAcute(pThing->renderData.apTweakedAngles[backJoint].x), 0.5f);
        sithPhysics_SetThingLook(pThing, &rdroid_zVector3, sithPhysics_flt_538BE4 * secDeltaTime);
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
    {
        pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        if ( (pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0) || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
        {
            pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
            if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
            {
                sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
            }
        }
        else
        {
            pThing->moveStatus = SITHPLAYERMOVE_FALLING;
        }
    }

    if ( (flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
        {
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, pPhysics->airDrag + 0.2f, 0.0f, secDeltaTime);
        }

        rdVector_ScaleAdd3Acc(&pPhysics->angularVelocity, &pPhysics->rotThrust, secDeltaTime);
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity);
        SITHPHYSICS_CLIPPRECISION3(&pPhysics->angularVelocity);
    }

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
    {
        rdVector_Scale3(&angVelocityDelta, &pPhysics->angularVelocity, secDeltaTime);
    }
    else
    {
        memset(&angVelocityDelta, 0, sizeof(rdVector3));
    }

    if ( !SITHPHYSICS_ISZERO3(&angVelocityDelta) )
    {
        rdMatrix_BuildRotate34(&mat, &angVelocityDelta);
        sithCollision_RotateThing(pThing, &mat);
        if ( (flags & SITH_PF_FLY) != 0 )
        {
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &mat);
        }

        if ( ((pThing->idx + sithMain_frameNumber) & 7) == 0 )
        {
            rdMatrix_Normalize34(&pThing->orient);
        }
    }

    if ( (flags & SITH_PF_FLY) == 0 && pThing->moveStatus != SITHPLAYERMOVE_FALLING )
    {
        rdVector_Scale3Acc(&pPhysics->thrust, 0.3f);
    }

    secDeltaTime = secDeltaTime + pPhysics->physicsRolloverFrames;
    numSteps = lrintf(secDeltaTime * 50.0f);
    pPhysics->physicsRolloverFrames = secDeltaTime - RD_MULTIPLY(D3DVAL(numSteps), 0.02f);
    for ( i = 0; i < numSteps; ++i )
    {
        memset(&vecThrust, 0, sizeof(rdVector3));
        if ( pPhysics->airDrag != 0.0f )
        {
            sithPhysics_ApplyDrag(&pPhysics->velocity, pPhysics->airDrag, 0.0f, 0.02f);
        }

        if ( (flags & SITH_PF_USETHRUST) != 0 )
        {
            rdVector_Scale3(&vecThrust, &pPhysics->thrust, 0.02f);
            rdMatrix_TransformVector34Acc(&vecThrust, &pThing->orient);
        }

        if ( pPhysics->mass != 0.0f
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
            && (flags & SITH_PF_NOTHRUST) == 0 )
        {
            rdVector_ScaleAdd3Acc(&vecThrust, &pThing->pInSector->thrust, 0.02f);
        }

        if ( pPhysics->mass != 0.0f
            && (flags & SITH_PF_USEGRAVITY) != 0
            && (pThing->pInSector->flags & SITH_SECTOR_NOGRAVITY) == 0 )
        {
            float gravity = RD_MULTIPLY(sithWorld_pCurrentWorld->gravity, 0.02f);
            if ( (pPhysics->flags & SITH_PF_PARTIALGRAVITY) != 0 )
            {
                gravity = RD_MULTIPLY(gravity, 0.5f);
            }

            vecThrust.z = vecThrust.z - gravity;
            pPhysics->gravityForce.z = -gravity;
        }

        rdVector_Add3Acc(&pPhysics->velocity, &vecThrust);
        rdVector_ScaleAdd3Acc(&pPhysics->deltaVelocity, &pPhysics->velocity, 0.02f);
    }
}

void J3DAPI sithPhysics_UpdateUnderwaterThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics;
    SithPhysicsFlags flags;
    rdMatrix34 mat;
    rdVector3 vecThrust;
    rdVector3 angVelocityDelta;
    int bCurUnused;

    pPhysics = &pThing->moveInfo.physics;
    flags    = pPhysics->flags;
    bCurUnused  = 0;
    memset(&vecThrust, 0, sizeof(rdVector3));
    memset(&pPhysics->gravityForce, 0, sizeof(rdVector3));

    if ( (flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
        {
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, pPhysics->airDrag + 0.2f, 0.0f, secDeltaTime);
        }

        rdVector_ScaleAdd3Acc(&pPhysics->angularVelocity, &pPhysics->rotThrust, secDeltaTime);
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity);
        SITHPHYSICS_CLIPPRECISION3(&pPhysics->angularVelocity);
    }

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
    {
        rdVector_Scale3(&angVelocityDelta, &pPhysics->angularVelocity, secDeltaTime);
    }
    else
    {
        memset(&angVelocityDelta, 0, sizeof(rdVector3));
    }

    if ( !SITHPHYSICS_ISZERO3(&angVelocityDelta) )
    {
        rdMatrix_BuildRotate34(&mat, &angVelocityDelta);
        sithCollision_RotateThing(pThing, &mat);
        if ( ((pThing->idx + sithMain_frameNumber) & 7) == 0 )
        {
            rdMatrix_Normalize34(&pThing->orient);
        }
    }

    if ( pPhysics->airDrag != 0.0f )
    {
        sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->airDrag, 4.0f), 0.0f, secDeltaTime);
    }

    if ( (flags & SITH_PF_USETHRUST) != 0 )
    {
        rdVector_Scale3Acc(&pPhysics->thrust, 0.6f);
        rdVector_Scale3(&vecThrust, &pPhysics->thrust, secDeltaTime);
        rdMatrix_TransformVector34Acc(&vecThrust, &pThing->orient);
    }

    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (flags & SITH_PF_NOTHRUST) == 0 )
    {
        rdVector_ScaleAdd3Acc(&vecThrust, &pThing->pInSector->thrust, secDeltaTime);
    }

    if ( ((flags & SITH_PF_ONWATERSURFACE) == 0 || (pThing->flags & SITH_TF_DYING) != 0) && (flags & SITH_PF_USEGRAVITY) != 0 )
    {
        float gravity = RD_MULTIPLY(sithWorld_pCurrentWorld->gravity, secDeltaTime);
        gravity = RD_MULTIPLY(pPhysics->buoyancy, gravity);
        vecThrust.z = vecThrust.z - gravity;
        pPhysics->gravityForce.z = pPhysics->gravityForce.z - gravity;
    }

    rdVector_Add3Acc(&pPhysics->velocity, &vecThrust);
    SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    if ( (pPhysics->flags & SITH_PF_ONWATERSURFACE) != 0 && pPhysics->thrust.z >= 0.0f )
    {
        float surfaceDist    = pThing->attach.distToWaterSurface - 0.051f;
        float maxSurfaceMove = RD_MULTIPLY(0.01f, secDeltaTime);
        if ( pPhysics->deltaVelocity.z > 0.0f && pPhysics->deltaVelocity.z < maxSurfaceMove )
        {
            pPhysics->deltaVelocity.z = 0.0f;
        }

        if ( surfaceDist != 0.0f )
        {
            surfaceDist = J3DMIN(surfaceDist, maxSurfaceMove);
            rdVector_ScaleAdd3Acc(&pPhysics->deltaVelocity, &rdroid_zVector3, surfaceDist);
            sithPuppet_StopMode(pThing, SITHPUPPETSUBMODE_FIRE4, 0.0f);
        }
    }
}

void J3DAPI sithPhysics_UpdateClimbingThingPhysics(SithThing* pThing, float secDeltaTime)
{
    SithPhysicsInfo* pCurrentPhysics = &pThing->moveInfo.physics;
    float thrustScale;
    float climbScale = 0.1f;
    rdVector3 currentvel;

    if ( !SITHPHYSICS_ISZERO3(&pCurrentPhysics->thrust) )
    {
        thrustScale = RD_MULTIPLY(secDeltaTime, climbScale);
        rdVector_Scale3(&currentvel, &pCurrentPhysics->thrust, thrustScale);
        SITHPHYSICS_CLIPPRECISION3(&currentvel);
        if ( !SITHPHYSICS_ISZERO3(&currentvel) )
        {
            rdMatrix_TransformVector34Acc(&currentvel, &pThing->orient);
        }

        if ( pCurrentPhysics->mass != 0.0f
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
            && (pCurrentPhysics->flags & SITH_PF_NOTHRUST) == 0 )
        {
            rdVector_ScaleAdd3Acc(&currentvel, &pThing->pInSector->thrust, secDeltaTime);
        }

        rdVector_Copy3(&pCurrentPhysics->velocity, &currentvel);
        rdVector_Scale3(&pCurrentPhysics->deltaVelocity, &pCurrentPhysics->velocity, secDeltaTime);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_2, 26) /* end count-fit */
void J3DAPI sithPhysics_UpdateAttachedThingPhysics(SithThing* pThing, float secDeltaTime)
{
    float dragFactor;
    float distance;
    rdVector3 thrustVel;
    rdVector3 normal;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    rdFace* pAttachedFace = pThing->attach.pFace;  // unreferenced; the debug build's frame keeps its slot
    float floorSlopeDot = 1.0f;

    memset(&thrustVel, 0, sizeof(rdVector3));
    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;

    sithPhysics_GetAttachedSurfaceInfo(pThing, &normal, &distance, &dragFactor);

    floorSlopeDot = rdVector_Dot3(&rdroid_zVector3, &normal);
    if ( (pPhysics->flags & SITH_PF_ALIGNUP) != 0 && floorSlopeDot < 1.0f )
    {
#line 1499
        SITH_ASSERTREL(floorSlopeDot > ((float)0.0));
        distance = D3DVAL(distance / floorSlopeDot);
    }

    if ( (pPhysics->flags & SITH_PF_ALIGNED) == 0 )
    {
        if ( (pPhysics->flags & SITH_PF_ALIGNSURFACE) != 0 )
        {
            sithPhysics_SetThingLook(pThing, &normal, RD_MULTIPLY(pPhysics->orientSpeed, secDeltaTime));
        }
        else if ( (pPhysics->flags & SITH_PF_ALIGNUP) != 0 )
        {
            sithPhysics_SetThingLook(pThing, &rdroid_zVector3, RD_MULTIPLY(pPhysics->orientSpeed, secDeltaTime));
        }
        else
        {
            pPhysics->flags |= SITH_PF_ALIGNED;
        }
    }

    if ( pThing->moveStatus != SITHPLAYERMOVE_LEAPFWD
        && floorSlopeDot < 0.8f
        && floorSlopeDot > 0.69f
        && (pThing->type == SITH_THING_PLAYER || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0) )
    {
        rdVector3 vec;
        rdVector3 dir;
        float dot;
        rdVector_Copy3(&vec, &pThing->orient.lvec);
        vec.z = 0.0f;
        rdVector_Normalize3Acc(&vec);

        rdVector_Copy3(&dir, &normal);
        dir.z = 0.0f;
        rdVector_Normalize3Acc(&dir);

        dot = rdVector_Dot3(&dir, &vec);
        if ( dot >= 0.0f )
        {
            pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNFORWARD;
        }
        else
        {
            rdVector_Neg3Acc(&dir);
            pThing->moveStatus = SITHPLAYERMOVE_SLIDEDOWNBACK;
        }

        rdVector_Copy3(&pThing->orient.lvec, &dir);
        rdVector_Cross3(&pThing->orient.rvec, &pThing->orient.lvec, &rdroid_zVector3);
        rdVector_Normalize3Acc(&pThing->orient.rvec);
        rdVector_Copy3(&pThing->orient.uvec, &rdroid_zVector3);

        rdVector_Copy3(&thrustVel, &normal);
        thrustVel.z *= -1.0f;
        rdVector_Normalize3Acc(&thrustVel);
        rdVector_Copy3(&pPhysics->velocity, &thrustVel);

        dragFactor = STDMATH_CLAMP(dragFactor, 0.25f, 1.0f);
        sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, dragFactor), 0.0f, secDeltaTime);
        rdVector_Scale3Acc(&pPhysics->velocity, 0.5f);
        SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
        {
            rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
        }

        return;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNFORWARD || pThing->moveStatus == SITHPLAYERMOVE_SLIDEDOWNBACK )
    {
        sithPhysics_ResetThingMovement(pThing);
        if ( (pThing == sithPlayer_pLocalPlayerThing || (pThing->thingInfo.actorInfo.flags & SITH_AF_NOSLOPEMOVE) != 0)
            && (pThing->moveInfo.physics.flags & (SITH_PF_MINECAR | SITH_PF_RAFT | SITH_PF_JEEP | SITH_PF_UNKNOWN_8000000)) == 0 )
        {
            if ( (pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0) || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_SWIMIDLE;
                if ( pThing->pPuppetState->moveMode != SITHPUPPET_MOVEMODE_SWIM )
                {
                    sithPuppet_SetMoveMode(pThing, SITHPUPPET_MOVEMODE_SWIM);
                }
            }
            else
            {
                pThing->moveStatus = SITHPLAYERMOVE_STILL;
            }
        }
    }

    if ( (pPhysics->flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
        {
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, pPhysics->surfDrag + 0.2f, 0.0f, secDeltaTime);
        }

        pPhysics->angularVelocity.y = RD_MULTIPLY(pPhysics->rotThrust.y, secDeltaTime) + pPhysics->angularVelocity.y;
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, (-pPhysics->maxRotationVelocity), pPhysics->maxRotationVelocity);
        SITHPHYSICS_CLIPPRECISION3(&pPhysics->angularVelocity);
    }

    if ( pPhysics->angularVelocity.y != 0.0f )
    {
        rdVector3 vec;
        rdMatrix34 rotMat;
        rdVector_Scale3(&vec, &pPhysics->angularVelocity, secDeltaTime);
        rdMatrix_BuildRotate34(&rotMat, &vec);
        sithCollision_RotateThing(pThing, &rotMat);
        if ( dragFactor < 1.0f )
        {
            rdVector3 dir;
            float velFactor = 1.0f - dragFactor;
            rdMatrix_TransformVector34(&dir, &pPhysics->velocity, &rotMat);
            rdVector_Scale3Acc(&pPhysics->velocity, velFactor);
            rdVector_ScaleAdd3Acc(&pPhysics->velocity, &dir, dragFactor);
        }
        else
        {
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &rotMat);
        }

        rdMatrix_Normalize34(&pThing->orient);
    }

    dragFactor = STDMATH_CLAMP(dragFactor, 0.25f, 1.0f);

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_UNKNOWN_8000) != 0 )
        {
            pPhysics->flags &= ~SITH_PF_UNKNOWN_8000;
        }
        else if ( SITHPHYSICS_ISZERO3(&pPhysics->thrust)
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) == 0
            && dragFactor > 0.8f )
        {
            sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, dragFactor), RD_MULTIPLY(pPhysics->staticDrag, dragFactor), secDeltaTime);
        }
        else
        {
            sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, dragFactor), 0.0f, secDeltaTime);
        }
    }

    if ( (pPhysics->flags & SITH_PF_USETHRUST) != 0 && !SITHPHYSICS_ISZERO3(&pPhysics->thrust) )
    {
        float thrustScale = RD_MULTIPLY(secDeltaTime, dragFactor);
        if ( (pPhysics->flags & SITH_PF_CROUCHING) != 0 )
        {
            thrustScale = RD_MULTIPLY(secDeltaTime, 0.8f);
        }

        rdVector_Scale3(&thrustVel, &pPhysics->thrust, thrustScale);
        SITHPHYSICS_CLIPPRECISION3(&thrustVel);

        if ( !SITHPHYSICS_ISZERO3(&thrustVel) )
        {
            rdMatrix_TransformVector34Acc(&thrustVel, &pThing->orient);
        }
    }

    if ( pPhysics->mass != 0.0f
        && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) != 0
        && (pPhysics->flags & SITH_PF_NOTHRUST) == 0 )
    {
        if ( pThing->pInSector->thrust.z > RD_MULTIPLY(sithWorld_pCurrentWorld->gravity, pPhysics->mass) )
        {
            sithThing_DetachThing(pThing);
            sithPhysics_UpdateThingPhysics(pThing, secDeltaTime);
            return;
        }

        rdVector_ScaleAdd3Acc(&thrustVel, &pThing->pInSector->thrust, secDeltaTime);
    }

    rdVector_Add3Acc(&pPhysics->velocity, &thrustVel);

    if ( pThing->type == SITH_THING_PLAYER && (pPhysics->flags & SITH_PF_USEGRAVITY) != 0 && floorSlopeDot < 1.0f )
    {
        if ( dragFactor < 0.8f || !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
        {
            float slopeGravity = 1.0f - dragFactor;
            slopeGravity = STDMATH_CLAMP(slopeGravity, 0.2f, 0.8f);
            slopeGravity = RD_MULTIPLY(RD_MULTIPLY(sithWorld_pCurrentWorld->gravity, secDeltaTime), slopeGravity);
            pPhysics->velocity.z = pPhysics->velocity.z - slopeGravity;
        }
    }

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        float surfDot;
        surfDot = rdVector_Dot3(&pPhysics->velocity, &normal);
        surfDot = stdMath_ClipNearZero(surfDot);

        if ( ((pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0) || (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0) && surfDot != 0.0f ) // the null test covers only the UNDERWATER arm, as the debug code has it
        {
            rdVector_ScaleAdd3Acc(&pPhysics->velocity, &normal, (-surfDot));
        }
    }

    SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    distance = distance - sithPhysics_GetThingHeight(pThing);
    distance = stdMath_ClipNearZero(distance);

    if ( distance != 0.0f )
    {
        float maxStep;
        maxStep = RD_MULTIPLY(1.5f, secDeltaTime);
        if ( pThing->type == SITH_THING_PLAYER )
        {
            if ( fabs(distance) >= 0.04f )
            {
                maxStep = RD_MULTIPLY(0.5f, secDeltaTime);
            }

            distance = STDMATH_CLAMP(distance, (-maxStep), maxStep);
        }
        else
        {
            distance = STDMATH_CLAMP(distance, (-maxStep), maxStep);
        }

        if ( (pPhysics->flags & SITH_PF_ALIGNUP) != 0 )
        {
            rdVector_ScaleAdd3Acc(&pPhysics->deltaVelocity, &rdroid_zVector3, (-distance));
        }
        else
        {
            rdVector_ScaleAdd3Acc(&pPhysics->deltaVelocity, &normal, (-distance));
        }
    }
}

static void J3DAPI sithPhysics_UpdateMineCarFx(SithThing* pThing, SithMineCarFxState* pFxState, float secDeltaTime)
{
    float velocity;
    int bInShallowWater;
    int bCurUpdate;
    SithMineCarState* pCurState;
    SithPhysicsInfo* pCurrentPhysics = &pThing->moveInfo.physics;
    static int sithPhysics_bMineCarEngineRunFx = 1;
    static int sithPhysics_bMineCarEngineRumbleFx = 1;
    static int sithPhysics_bMineCarSparksFx = 1;
    static int sithPhysics_bMineCarClatterFx = 1;
    static int sithPhysics_bMineCarRailClackFx = 1;
    static float sithPhysics_flt_538BFC = 1.0f;
    static float sithPhysics_flt_538C00 = 1.15f;
    static float sithPhysics_flt_538C04 = 0.5f;
    static float sithPhysics_flt_538C08 = 0.5f;
    static float sithPhysics_flt_538C0C = 18.0f;
    static float sithPhysics_flt_538C10 = 1e+01f;
    static float sithPhysics_flt_538C14 = 0.05f;
    static float sithPhysics_flt_538C18 = 0.95f;
    static float sithPhysics_flt_538C1C = 0.6f;
    static float sithPhysics_flt_538C20 = 0.3f;
    static float sithPhysics_flt_538C24 = 2e+01f;
    static float sithPhysics_flt_538C28 = 0.5f;
    static float sithPhysics_flt_538C2C = 0.05f;
    static float sithPhysics_flt_538C30 = 2.0f;
    static float sithPhysics_flt_538C34 = 0.5f;
    static struct { float secDelay; rdVector3 pos; } sithPhysics_minecarSparkRight = { 0.05f, { { 0.05f }, { -0.04f }, { -0.09f } } };
    static struct { float secDelay; rdVector3 pos; } sithPhysics_minecarSparkLeft = { 0.05f, { { -0.05f }, { -0.04f }, { -0.09f } } };
    static float sithPhysics_flt_538C58 = 0.1f;
    static float sithPhysics_flt_538C5C = 1.0f;
    static float sithPhysics_flt_538C60 = 0.5f;
    static float sithPhysics_flt_538C64 = 0.2f;
    static float sithPhysics_flt_538C68 = 3.0f;
    static float sithPhysics_flt_538C6C = 0.1f;
    static float sithPhysics_flt_538C70 = 0.1f;
    static float sithPhysics_flt_538C74 = 1.0f;
    static float sithPhysics_flt_538C78 = 0.05f;

    pCurState = &pThing->userblock.pMinecar->state;
    velocity  = sqrtf(rdVector_Dot3(&pCurrentPhysics->velocity, &pCurrentPhysics->velocity));

    bInShallowWater = 0;
    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 && (pThing->attach.attachedToStructure.pSurfaceAttached->flags & SITH_SURFACE_SHALLOWWATER) != 0 )
    {
        bInShallowWater = 1;
    }

    bCurUpdate = 0;
    pFxState->engine.secUpdateTimer -= secDeltaTime;
    if ( pFxState->engine.secUpdateTimer < 0.0f )
    {
        bCurUpdate = 1;
        pFxState->engine.secUpdateTimer = pFxState->engine.secUpdateInterval;
    }

    // Engine
    if ( sithPhysics_bMineCarEngineRunFx )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_BOARDING )
        {
            sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
            pCurState->pEngineAnim = NULL;
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT || pThing->moveStatus == SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT )
        {
            sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
            if ( pCurState->pEngineAnim && pCurState->bEngineAnim )
            {
                sithAnimate_Stop(pCurState->pEngineAnim);
                pCurState->pEngineAnim = NULL;
            }
        }
        else
        {
            float volume;
            float ratio;
            float pitch;

            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_flt_538C04);
            if ( !pCurState->pEngineAnim && pCurState->bEngineAnim )
            {
                rdMaterial* pMat = sithMaterial_Load("minecar_a_ngenfrnt.mat");
                if ( pMat )
                {
                    pCurState->pEngineAnim = sithAnimate_StartMaterialAnim(pMat, sithPhysics_flt_538C0C, SITHANIMATE_LOOP);
                }
            }

            if ( bCurUpdate )
            {
                volume = RD_MULTIPLY(velocity, sithPhysics_flt_538C08) + sithPhysics_flt_538C04;
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, volume);

                ratio = D3DVAL((sithPhysics_flt_538C00 - sithPhysics_flt_538BFC) / pCurrentPhysics->maxVelocity);
                pitch = RD_MULTIPLY(velocity, ratio) + sithPhysics_flt_538BFC;
                pitch = STDMATH_CLAMP(pitch, sithPhysics_flt_538BFC, sithPhysics_flt_538C00);
                sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);

                if ( pCurState->pEngineAnim && pCurState->bEngineAnim )
                {
                    pCurState->pEngineAnim->msecPerFrame = (uint32_t)(1000.0f / RD_MULTIPLY(RD_MULTIPLY(pitch - 1.0f, sithPhysics_flt_538C10) + 1.0f, sithPhysics_flt_538C0C));
                }
            }
        }
    }
    else
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
        if ( pCurState->pEngineAnim && (pThing->type & (SITH_THING_ACTOR | SITH_THING_GHOST)) != 0 )
        {
            sithAnimate_Stop(pCurState->pEngineAnim);
            pCurState->pEngineAnim = NULL;
        }
    }

    // Engine rumble
    if ( sithPhysics_bMineCarEngineRumbleFx && !bInShallowWater )
    {
        if ( velocity != 0.0f )
        {
            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKMETAL, sithPhysics_flt_538C14);
            if ( bCurUpdate )
            {
                float volume = RD_MULTIPLY(velocity, sithPhysics_flt_538C18) + sithPhysics_flt_538C14;
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, volume);
            }
        }
        else if ( !sithSoundClass_IsModeFadingVol(pThing, SITHSOUNDCLASS_RWALKMETAL) )
        {
            sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, 0.0f, sithPhysics_flt_538C1C);
        }
    }
    else
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_RWALKMETAL);
    }

    // Sparks
    if ( sithPhysics_bMineCarSparksFx && !bInShallowWater )
    {
        float sparkChance = RD_MULTIPLY(velocity, sithPhysics_flt_538C28);
        if ( pCurState->maybeMoveSate == 3 || pCurState->maybeMoveSate == 1 )
        {
            if ( SITHPHYSICS_ABS(pCurrentPhysics->angularVelocity.y) > sithPhysics_flt_538C24
                && velocity > sithPhysics_flt_538C20
                && SITH_RAND() < sparkChance )
            {
                float volume = RD_MULTIPLY(velocity - sithPhysics_flt_538C20, sithPhysics_flt_538C30) + sithPhysics_flt_538C2C;
                sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RWALKHARD);
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_RWALKHARD, volume);
                if ( pCurState->maybeMoveSate == 1 )
                {
                    pCurState->bUpdateRightSparks = 1;
                    pCurState->secTimeRightSparks = 0.0f;
                }
                else if ( pCurState->maybeMoveSate == 3 )
                {
                    pCurState->bUpdateLeftSparks = 1;
                    pCurState->secTimeLeftSparks = 0.0f;
                }
            }
        }
        else if ( pCurState->maybeMoveSate == 5 )
        {
            sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKHARD, 0.0f, sithPhysics_flt_538C34);
            pCurState->bUpdateRightSparks = 0;
            pCurState->bUpdateLeftSparks  = 0;
        }

        if ( pCurState->bUpdateSparks && !pCurState->bUpdateRightSparks )
        {
            pCurState->bUpdateRightSparks = 1;
            pCurState->secTimeRightSparks = 0.0f;
        }

        if ( pCurState->bUpdateSparks && !pCurState->bUpdateLeftSparks )
        {
            pCurState->bUpdateLeftSparks = 1;
            pCurState->secTimeLeftSparks = 0.0f;
        }

        if ( pCurState->bUpdateRightSparks )
        {
            pCurState->secTimeRightSparks -= secDeltaTime;
            if ( pCurState->secTimeRightSparks <= 0.0f )
            {
                rdVector3 sparkPos = sithPhysics_minecarSparkRight.pos;
                sithFX_CreateMineCarSparks(pThing, &sparkPos);
                pCurState->secTimeRightSparks = sithPhysics_minecarSparkRight.secDelay;
            }
        }

        if ( pCurState->bUpdateLeftSparks )
        {
            pCurState->secTimeLeftSparks -= secDeltaTime;
            if ( pCurState->secTimeLeftSparks <= 0.0f )
            {
                rdVector3 sparkPos = sithPhysics_minecarSparkLeft.pos;
                sithFX_CreateMineCarSparks(pThing, &sparkPos);
                pCurState->secTimeLeftSparks = sithPhysics_minecarSparkLeft.secDelay;
            }
        }
    }

    // Clatter
    if ( sithPhysics_bMineCarClatterFx && !bInShallowWater )
    {
        if ( pCurState->bOnAdjoinTrack && velocity != 0.0f )
        {
            float volume = RD_MULTIPLY(velocity, sithPhysics_flt_538C5C) + sithPhysics_flt_538C58;
            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LRUNMETAL, volume);
        }
        else if ( !sithSoundClass_IsModeFadingVol(pThing, SITHSOUNDCLASS_LRUNMETAL) )
        {
            sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_LRUNMETAL, 0.0f, sithPhysics_flt_538C60);
        }
    }
    else
    {
        sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_LRUNMETAL, 0.0f, sithPhysics_flt_538C64);
    }

    // Rail clack
    if ( sithPhysics_bMineCarRailClackFx && !bInShallowWater )
    {
        if ( velocity != 0.0f )
        {
            float prevClackState;
            float clackDiff;
            float rate;

            rate           = D3DVAL((sithPhysics_flt_538C68 - sithPhysics_flt_538C6C) / pCurrentPhysics->maxVelocity);
            clackDiff     = RD_MULTIPLY(velocity, rate) + sithPhysics_flt_538C6C;
            clackDiff     = RD_MULTIPLY(clackDiff, secDeltaTime);
            prevClackState = pFxState->railClackState;
            if ( prevClackState > 0.0f )
            {
                pFxState->railClackState = pFxState->railClackState - clackDiff;
            }
            else
            {
                pFxState->railClackState = pFxState->railClackState + clackDiff;
            }

            if ( SITHPHYSICS_SIGN(pFxState->railClackState) != SITHPHYSICS_SIGN(prevClackState) || pFxState->railClackState == 0.0f )
            {
                float volume;
                float dist;
                float pitch;

                if ( pFxState->railClackState >= 0.0f )
                {
                    sithSoundClass_PlayMode(pThing, SITHSOUNDCLASS_LWALKHARD, 1.0f);
                }
                else
                {
                    sithSoundClass_PlayMode(pThing, SITHSOUNDCLASS_LWALKHARD, 0.0f);
                }

                volume = RD_MULTIPLY(velocity, sithPhysics_flt_538C74) + sithPhysics_flt_538C70;
                pitch  = RD_MULTIPLY(velocity, sithPhysics_flt_538C78) + 1.0f;
                pitch  = RD_MULTIPLY(D3DVAL(SITH_RAND()), 0.01f) - 0.005f + pitch;
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKHARD, volume);
                sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKHARD, pitch);

                dist = RD_MULTIPLY(D3DVAL(SITH_RAND()), 0.2f) + pFxState->railClackDistance;
                pFxState->railClackState = RD_MULTIPLY(-SITHPHYSICS_SIGN(prevClackState), dist);
            }
        }
        else
        {
            pFxState->railClackState = pFxState->railClackDistance;
        }
    }
}

static void J3DAPI sithPhysics_UpdateTrackJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    int bCurUpdate;
    float ratio;
    float pitch;
    float velocity;
    float curVolume;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    static int sithPhysics_bUpdateTrackJeepEngineFx = 1;
    static float sithPhysics_truckMinSndPitch = 1.0f;
    static float sithPhysics_truckMaxSndPitch = 2.0f;
    static float sithPhysics_truckMinSndVolume = 0.6f;
    static float sithPhysics_flt_538C8C = 1.0f;
    static float sithPhysics_flt_538C90 = 1.0f;

    if ( (pThing->flags & SITH_TF_DYING) != 0 )
    {
        return;
    }

    bCurUpdate = 0;
    pFxState->secUpdateTimer -= secDeltaTime;
    if ( pFxState->secUpdateTimer < 0.0f )
    {
        bCurUpdate = 1;
        pFxState->secUpdateTimer = pFxState->secUpdateInterval;
    }

    if ( sithPhysics_bUpdateTrackJeepEngineFx )
    {
        sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_truckMinSndVolume);
        if ( bCurUpdate )
        {
            ratio  = D3DVAL((sithPhysics_flt_538C8C - sithPhysics_truckMinSndVolume) / sithPhysics_flt_538C90);
            velocity  = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));
            curVolume = RD_MULTIPLY(velocity, ratio) + sithPhysics_truckMinSndVolume;
            curVolume = STDMATH_CLAMP(curVolume, sithPhysics_truckMinSndVolume, sithPhysics_flt_538C8C);
            sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, curVolume);

            ratio = D3DVAL((sithPhysics_truckMaxSndPitch - sithPhysics_truckMinSndPitch) / sithPhysics_flt_538C90);
            pitch = RD_MULTIPLY(velocity, ratio) + sithPhysics_truckMinSndPitch;
            pitch = STDMATH_CLAMP(pitch, sithPhysics_truckMinSndPitch, sithPhysics_truckMaxSndPitch);
            sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);
        }
    }
    else
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
    }
}

static void J3DAPI sithPhysics_UpdateTrackTruckFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    int bCurUpdate;
    float ratio;
    float pitch;
    float velocity;
    float curVolume;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    static int sithPhysics_bUpdateTrackTruckEngineFx = 1;
    static float sithPhysics_flt_538C98 = 0.52f;
    static float sithPhysics_flt_538C9C = 1.0f;
    static float sithPhysics_flt_538CA0 = 0.5f;
    static float sithPhysics_flt_538CA4 = 1.0f;
    static float sithPhysics_flt_538CA8 = 1.0f;

    if ( (pThing->flags & SITH_TF_DYING) != 0 )
    {
        return;
    }

    bCurUpdate = 0;
    pFxState->secUpdateTimer -= secDeltaTime;
    if ( pFxState->secUpdateTimer < 0.0f )
    {
        bCurUpdate = 1;
        pFxState->secUpdateTimer = pFxState->secUpdateInterval;
    }

    if ( sithPhysics_bUpdateTrackTruckEngineFx )
    {
        sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_flt_538CA0);
        if ( bCurUpdate )
        {
            ratio  = D3DVAL((sithPhysics_flt_538CA4 - sithPhysics_flt_538CA0) / sithPhysics_flt_538CA8);
            velocity  = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));
            curVolume = RD_MULTIPLY(velocity, ratio) + sithPhysics_flt_538CA0;
            curVolume = STDMATH_CLAMP(curVolume, sithPhysics_flt_538CA0, sithPhysics_flt_538CA4);
            sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, curVolume);

            ratio = D3DVAL((sithPhysics_flt_538C9C - sithPhysics_flt_538C98) / sithPhysics_flt_538CA8);
            pitch = RD_MULTIPLY(velocity, ratio) + sithPhysics_flt_538C98;
            sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);
        }
    }
    else
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
    }
}

static void J3DAPI sithPhysics_UpdateTrackVehicleFx(SithThing* pThing, SithMineCarUserBlock* pMineCarUserBlock, float secDeltaTime)
{
    switch ( pMineCarUserBlock->type )
    {
        case SITHMINECAR_PLAYER:
        case SITHMINECAR_DEFAULT:
            sithPhysics_UpdateMineCarFx(pThing, &pMineCarUserBlock->fxstate, secDeltaTime);
            break;

        case SITHMINECAR_JEEP_AI:
            sithPhysics_UpdateTrackJeepFx(pThing, &pMineCarUserBlock->fxstate.engine, secDeltaTime);
            break;

        case SITHMINECAR_KILLTRUCK_AI:
        case SITHMINECAR_TRUCK_AI:
            sithPhysics_UpdateTrackTruckFx(pThing, &pMineCarUserBlock->fxstate.engine, secDeltaTime);
            break;
    }
}
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_3, 4) /* end count-fit */
static void J3DAPI sithPhysics_UpdateVehicleChassis(SithThing* pThing, SithVehicleChassisInfo* pChasisInfo, float secDeltaTime)
{
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    size_t i;

    if ( pChasisInfo->numNodes )
    {
        float circumference;
        float speedRatio;

        circumference = RD_MULTIPLY(2.0f, RD_MULTIPLY(pChasisInfo->wheelRadius, 3.14159f));
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
        {
            float angle;

            speedRatio = D3DVAL(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) / circumference);
            angle = RD_MULTIPLY(RD_MULTIPLY(360.0f, speedRatio), secDeltaTime);
            angle = stdMath_NormalizeAngle(angle);
            if ( rdVector_Dot3(&pPhysics->velocity, &pThing->orient.lvec) < 0.0f )
            {
                angle = RD_MULTIPLY(angle, -1.0f);
            }

            for ( i = 0; i < pChasisInfo->numNodes; ++i )
            {
                if ( pChasisInfo->aWheelNodeNums[i] != -1 )
                {
                    pThing->renderData.apTweakedAngles[pChasisInfo->aWheelNodeNums[i]].pitch -= angle;
                }
            }
        }
    }
}

static void J3DAPI sithPhysics_UpdateExhaustFx(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo, float secDeltaTime)
{
    size_t numPuffs;
    size_t i;
    float moveDist;
    float velLen;
    float secPuffStep;
    float secPuffTime;
    float maxLife;
    float life;
    float lifeScatter;
    float lifeTime;
    float velRight;
    float velUp;
    float startRate;
    float endRate;
    float startX;
    float startY;
    float endX;
    float endY;
    float endSize;
    SithThing* pTemplate;
    SithThing* pPuff;
    rdVector3 pos;
    rdVector3 dir;
    rdVector3 puffPos;
    rdVector3 vel;
    rdVector3 startScale;
    rdVector3 endScale;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;

    if ( !pExhaustInfo->bEnabled )
    {
        return;
    }

    if ( pThing->moveStatus != SITHPLAYERMOVE_MINECAR_BOARDING
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_UNBOARDING_RIGHT
        && pThing->moveStatus != SITHPLAYERMOVE_MINECAR_UNBOARDING_LEFT )
    {
        rdVector_Copy3(&pos, &pThing->pos);
        rdVector_ScaleAdd3Acc(&pos, &pThing->orient.lvec, pExhaustInfo->offsetY);
        rdVector_ScaleAdd3Acc(&pos, &pThing->orient.uvec, pExhaustInfo->offsetZ);
        rdVector_ScaleAdd3Acc(&pos, &pThing->orient.rvec, pExhaustInfo->offsetX);
        rdVector_Add3Acc(&pos, &pPhysics->deltaVelocity);

        rdVector_Normalize3(&dir, &pPhysics->deltaVelocity);
        if ( rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity) < 0.0f )
        {
            rdVector_Scale3Acc(&dir, -0.5f);
        }

        moveDist = sqrtf(rdVector_Dot3(&pPhysics->deltaVelocity, &pPhysics->deltaVelocity));
        velLen   = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));

        numPuffs = (size_t)(moveDist / (pExhaustInfo->moveLength));
        if ( !numPuffs )
        {
            numPuffs = 1;
        }

        if ( numPuffs > 10 )
        {
            numPuffs = 10;
        }

        secPuffStep = D3DVAL(secDeltaTime / (float)numPuffs);
        pTemplate   = sithTemplate_GetTemplate(pExhaustInfo->aExhaustTemplate);

        for ( i = 0; i < numPuffs; ++i )
        {
            if ( pTemplate )
            {
                secPuffTime = RD_MULTIPLY(D3DVAL(i), secPuffStep);
                maxLife     = RD_MULTIPLY(pPhysics->maxVelocity, pExhaustInfo->velScale) + pExhaustInfo->velocity;

                velRight = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pExhaustInfo->scatterX) + pExhaustInfo->dirX;
                velRight = RD_MULTIPLY(velRight, velLen);
                rdVector_Scale3(&vel, &pThing->orient.rvec, velRight);

                velUp = RD_MULTIPLY(D3DVAL(SITH_RAND()), pExhaustInfo->scatterZ - pExhaustInfo->dirZ) + pExhaustInfo->dirZ;
                velUp = RD_MULTIPLY(velUp, velLen);
                rdVector_ScaleAdd3Acc(&vel, &pThing->orient.uvec, velUp);

                rdVector_ScaleAdd3Acc(&pos, &dir, -pExhaustInfo->moveLength);
                rdVector_Copy3(&puffPos, &pos);
                velRight = RD_MULTIPLY(velRight, secPuffTime);
                rdVector_ScaleAdd3Acc(&puffPos, &pThing->orient.rvec, velRight);
                velUp = RD_MULTIPLY(velUp, secPuffTime);
                rdVector_ScaleAdd3Acc(&puffPos, &pThing->orient.uvec, velUp);

                pPuff = sithThing_CreateThingAtPos(pTemplate, &puffPos, &pThing->orient, pThing->pInSector, NULL);
                if ( pPuff )
                {
                    life        = RD_MULTIPLY(pExhaustInfo->velScale, velLen) + pExhaustInfo->velocity;
                    life        = life - secPuffTime;
                    lifeScatter = RD_MULTIPLY(pExhaustInfo->smokeLifeScale, life);
                    lifeScatter = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, lifeScatter);
                    life        = life + lifeScatter;
                    pPuff->msecLifeLeft = (int)RD_MULTIPLY(life, 1000.0f);

                    startRate = D3DVAL((pExhaustInfo->startMaxSize - pExhaustInfo->startMinSize) / maxLife);
                    endRate   = D3DVAL((pExhaustInfo->endMaxSize - pExhaustInfo->endMinSize) / maxLife);
                    startX    = RD_MULTIPLY(startRate, secPuffTime) + pExhaustInfo->startMinSize;
                    startY    = RD_MULTIPLY(startRate, secPuffTime) + pExhaustInfo->startMinSize;
                    endSize   = RD_MULTIPLY(endRate, secPuffTime) + pExhaustInfo->endMinSize;

                    startX  = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pExhaustInfo->startSizeScale) + startX;
                    startY  = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pExhaustInfo->startSizeScale) + startY;
                    endX    = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pExhaustInfo->startSizeScale) + pExhaustInfo->startMaxSize;
                    endY    = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pExhaustInfo->startSizeScale) + pExhaustInfo->startMaxSize;
                    endSize = RD_MULTIPLY(D3DVAL(SITH_RAND()) - 0.5f, pExhaustInfo->endSizeScale) + endSize;

                    startScale.x = startX;
                    startScale.y = startY;
                    startScale.z = endSize;
                    endScale.x   = endX;
                    endScale.y   = endY;
                    endScale.z   = pExhaustInfo->endMaxSize;
                    lifeTime     = life;
                    sithAnimate_StartSpriteSizeAnim(pPuff, &startScale, &endScale, lifeTime);

                    rdVector_Copy3(&pPuff->moveInfo.physics.velocity, &vel);
                }
            }
        }
    }
}

int J3DAPI sithPhysics_CreateMineCarUserBlock(SithThing* pMineCar)
{
    SithMineCarUserBlock* pUserBlock;

    if ( !pMineCar->userblock.pMinecar )
    {
#line 2455
        pMineCar->userblock.pMinecar = (SithMineCarUserBlock*)STDMALLOC(sizeof(SithMineCarUserBlock));
        if ( !pMineCar->userblock.pMinecar )
        {
#line 2458
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "_CreateMineCarUserBlock failed to allocate.\n", 0, 0, 0, 0);
            return 0;
        }

        pUserBlock = pMineCar->userblock.pMinecar;
        memset(pUserBlock, 0, sizeof(SithMineCarUserBlock));

        if ( stdUtil_StrCmp(pMineCar->aName, "mineplayer") == 0 )
        {
            pUserBlock->type = SITHMINECAR_PLAYER;
            sithPhysics_InitMineCarExhaust(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitMineCarChassis(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitMineCarFxState(pMineCar, &pMineCar->userblock.pMinecar->fxstate);
            sithPhysics_InitMineCarState(pMineCar, &pMineCar->userblock.pMinecar->state);
            pMineCar->userblock.pMinecar->state.bEngineAnim = 1;
        }
        else if ( stdUtil_StrCmp(pMineCar->aName, "killtruk") == 0 )
        {
            pUserBlock->type = SITHMINECAR_KILLTRUCK_AI;
            sithPhysics_InitTrackTruckExhaust(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitTrackTruckChassis(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitTrackTruckState(pMineCar, &pMineCar->userblock.pMinecar->state);
            sithPhysics_InitVehicleFxState(pMineCar, &pMineCar->userblock.pMinecar->fxstate.engine);
        }
        else if ( stdUtil_StrCmp(pMineCar->aName, "commietruck") == 0 )
        {
            pUserBlock->type = SITHMINECAR_TRUCK_AI;
            sithPhysics_InitTrackTruckExhaust(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitTrackTruckChassis(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitTrackTruckState(pMineCar, &pMineCar->userblock.pMinecar->state);
            sithPhysics_InitVehicleFxState(pMineCar, &pMineCar->userblock.pMinecar->fxstate.engine);
        }
        else if ( stdUtil_StrCmp(pMineCar->aName, "jeep_cs") == 0 || stdUtil_StrCmp(pMineCar->aName, "jeep_cs_pyr") == 0 )
        {
            pUserBlock->type = SITHMINECAR_JEEP_AI;
            sithPhysics_InitJeepExhaust(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitTrackJeepChassis(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitMineCarState(pMineCar, &pMineCar->userblock.pMinecar->state);
            sithPhysics_InitVehicleFxState(pMineCar, &pMineCar->userblock.pMinecar->fxstate.engine);
        }
        else if ( stdUtil_StrCmp(pMineCar->aName, "mine_2commcar") == 0 )
        {
            pUserBlock->type = SITHMINECAR_DEFAULT;
            sithPhysics_InitMineCarExhaust(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitMineCarChassis(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitMineCarFxState(pMineCar, &pMineCar->userblock.pMinecar->fxstate);
            sithPhysics_InitMineCarState(pMineCar, &pMineCar->userblock.pMinecar->state);
        }
        else if ( stdUtil_StrCmp(pMineCar->aName, "boulder_mc") == 0 )
        {
            pUserBlock->type = SITHMINECAR_MINECAR_AI;
            sithPhysics_InitMineCarExhaustDefault(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitMineCarChassisDefault(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitMineCarState(pMineCar, &pMineCar->userblock.pMinecar->state);
        }
        else
        {
            pUserBlock->type = SITHMINECAR_BOULDER;
            sithPhysics_InitMineCarExhaustDefault(pMineCar, &pMineCar->userblock.pMinecar->exhaustInfo);
            sithPhysics_InitMineCarChassisDefault(pMineCar, &pMineCar->userblock.pMinecar->chassisInfo);
            sithPhysics_InitMineCarState(pMineCar, &pMineCar->userblock.pMinecar->state);
        }
    }

    return 1;
}

void J3DAPI sithPhysics_InitMineCarFxState(SithThing* pThing, SithMineCarFxState* pFxState)
{
    pFxState->engine.secUpdateInterval = 0.2f;
    pFxState->engine.secUpdateTimer    = 0.0f;
    pFxState->railClackDistance        = 2.0f;
    pFxState->railClackState           = pFxState->railClackDistance;
}

void J3DAPI sithPhysics_InitVehicleFxState(SithThing* pThing, SithVehicleEngineFxState* pFxState)
{
    pFxState->secUpdateInterval = 0.05f;
    pFxState->secUpdateTimer    = 0.0f;
}

void J3DAPI sithPhysics_InitMineCarChassisDefault(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    pChassisInfo->numNodes = 0;
}

void J3DAPI sithPhysics_InitMineCarChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    pChassisInfo->numNodes = 2;
    strncpy(pChassisInfo->aWheelMeshNames[0], "bkwheel", 63);
    pChassisInfo->aWheelMeshNames[0][63] = 0;
    strncpy(pChassisInfo->aWheelMeshNames[1], "frwheel", 63);
    pChassisInfo->aWheelMeshNames[1][63] = 0;
    pChassisInfo->aWheelNodeNums[0] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[0]);
    pChassisInfo->aWheelNodeNums[1] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[1]);
    pChassisInfo->wheelRadius = 0.015f;
}

void J3DAPI sithPhysics_InitTrackTruckChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    pChassisInfo->numNodes = 2;
    strncpy(pChassisInfo->aWheelMeshNames[0], "bkwheel", 63);
    pChassisInfo->aWheelMeshNames[0][63] = 0;
    strncpy(pChassisInfo->aWheelMeshNames[1], "frwheel", 63);
    pChassisInfo->aWheelMeshNames[1][63] = 0;
    pChassisInfo->aWheelNodeNums[0] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[0]);
    pChassisInfo->aWheelNodeNums[1] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[1]);
    pChassisInfo->wheelRadius = 0.0375f;
}

void J3DAPI sithPhysics_InitTrackJeepChassis(SithThing* pThing, SithVehicleChassisInfo* pChassisInfo)
{
    pChassisInfo->numNodes = 4;
    strncpy(pChassisInfo->aWheelMeshNames[0], "brwheel", 63);
    pChassisInfo->aWheelMeshNames[0][63] = 0;
    strncpy(pChassisInfo->aWheelMeshNames[1], "flwheel", 63);
    pChassisInfo->aWheelMeshNames[1][63] = 0;
    strncpy(pChassisInfo->aWheelMeshNames[2], "blwheel", 63);
    pChassisInfo->aWheelMeshNames[2][63] = 0;
    strncpy(pChassisInfo->aWheelMeshNames[3], "frwheel", 63);
    pChassisInfo->aWheelMeshNames[3][63] = 0;
    pChassisInfo->aWheelNodeNums[0] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[0]);
    pChassisInfo->aWheelNodeNums[1] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[1]);
    pChassisInfo->aWheelNodeNums[2] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[2]);
    pChassisInfo->aWheelNodeNums[3] = sithThing_GetThingJointIndex(pThing, pChassisInfo->aWheelMeshNames[3]);
    pChassisInfo->wheelRadius = 0.0375f;
}

void J3DAPI sithPhysics_InitMineCarExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    pExhaustInfo->bEnabled = 1;
    pExhaustInfo->moveLength = 0.008f;
    pExhaustInfo->offsetY = -0.09f;
    pExhaustInfo->offsetZ = -0.034f;
    pExhaustInfo->offsetX = -0.02f;
    strncpy(pExhaustInfo->aExhaustTemplate, "+exhaust", 63);
    pExhaustInfo->aExhaustTemplate[63] = 0;
    pExhaustInfo->velocity = 0.15f;
    pExhaustInfo->velScale = 0.2f;
    pExhaustInfo->smokeLifeScale = 0.2f;
    pExhaustInfo->startMinSize = 0.01f;
    pExhaustInfo->startMaxSize = 0.1f;
    pExhaustInfo->startSizeScale = 0.01f;
    pExhaustInfo->endMinSize = 0.15f;
    pExhaustInfo->endMaxSize = 0.0f;
    pExhaustInfo->endSizeScale = 0.01f;
    pExhaustInfo->scatterX = 0.1f;
    pExhaustInfo->dirX = -0.1f;
    pExhaustInfo->scatterZ = 0.1f;
    pExhaustInfo->dirZ = 0.0f;
}

void J3DAPI sithPhysics_InitJeepExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    pExhaustInfo->bEnabled = 0;
    pExhaustInfo->moveLength = 0.008f;
    pExhaustInfo->offsetY = -0.16f;
    pExhaustInfo->offsetZ = -0.034f;
    pExhaustInfo->offsetX = 0.02f;
    strncpy(pExhaustInfo->aExhaustTemplate, "+exhaust", 63);
    pExhaustInfo->aExhaustTemplate[63] = 0;
    pExhaustInfo->velocity = 0.05f;
    pExhaustInfo->velScale = 0.02f;
    pExhaustInfo->smokeLifeScale = 0.2f;
    pExhaustInfo->startMinSize = 0.01f;
    pExhaustInfo->startMaxSize = 0.1f;
    pExhaustInfo->startSizeScale = 0.01f;
    pExhaustInfo->endMinSize = 0.15f;
    pExhaustInfo->endMaxSize = 0.0f;
    pExhaustInfo->endSizeScale = 0.01f;
    pExhaustInfo->scatterX = 0.1f;
    pExhaustInfo->dirX = 0.0f;
    pExhaustInfo->scatterZ = 0.1f;
    pExhaustInfo->dirZ = 0.0f;
}

void J3DAPI sithPhysics_InitTrackTruckExhaust(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    pExhaustInfo->bEnabled = 1;
    pExhaustInfo->moveLength = 0.008f;
    pExhaustInfo->offsetY = -0.3f;
    pExhaustInfo->offsetZ = -0.1f;
    pExhaustInfo->offsetX = 0.06f;
    strncpy(pExhaustInfo->aExhaustTemplate, "+exhaust", 63);
    pExhaustInfo->aExhaustTemplate[63] = 0;
    pExhaustInfo->velocity = 0.1f;
    pExhaustInfo->velScale = 0.2f;
    pExhaustInfo->smokeLifeScale = 0.2f;
    pExhaustInfo->startMinSize = 0.01f;
    pExhaustInfo->startMaxSize = 0.1f;
    pExhaustInfo->startSizeScale = 0.01f;
    pExhaustInfo->endMinSize = 0.15f;
    pExhaustInfo->endMaxSize = 0.0f;
    pExhaustInfo->endSizeScale = 0.01f;
    pExhaustInfo->scatterX = 0.1f;
    pExhaustInfo->dirX = 0.0f;
    pExhaustInfo->scatterZ = 0.1f;
    pExhaustInfo->dirZ = 0.0f;
}

// Linker-folded in retail onto sithPhysics_InitMineCarChassisDefault (identical body); the debug build keeps it
// here, after the exhaust initialisers.
static void J3DAPI sithPhysics_InitMineCarExhaustDefault(SithThing* pThing, SithVehicleExhaustInfo* pExhaustInfo)
{
    pExhaustInfo->bEnabled = 0;
}

void J3DAPI sithPhysics_InitMineCarState(SithThing* pThing, SithMineCarState* pState)
{
    pState->maybeMoveSate = 0;
    pState->bOnAdjoinTrack = 0;
    pState->surfDrag = pThing->moveInfo.physics.surfDrag + 1.0f;
    pState->pEngineAnim = NULL;
    pState->bEngineAnim = 0;
    pState->bUpdateRightSparks = 0;
    pState->bUpdateLeftSparks = 0;
    pState->secTimeRightSparks = 0.0f;
    pState->secTimeLeftSparks = 0.0f;
    pState->unknown10 = 0.2f;
    pState->unknown9 = 0.3f;
    pState->bUpdateSparks = 0;
}

void J3DAPI sithPhysics_InitTrackTruckState(SithThing* pThing, SithMineCarState* pState)
{
    pState->maybeMoveSate = 0;
    pState->bOnAdjoinTrack = 0;
    pState->surfDrag = pThing->moveInfo.physics.surfDrag + 1.0f;
    pState->pEngineAnim = NULL;
    pState->bEngineAnim = 0;
    pState->bUpdateRightSparks = 0;
    pState->bUpdateLeftSparks = 0;
    pState->secTimeRightSparks = 0.0f;
    pState->secTimeLeftSparks = 0.0f;
    pState->unknown10 = 0.6f;
    pState->unknown9 = 0.8f;
    pState->bUpdateSparks = 0;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_4, 6) /* end count-fit */
void J3DAPI sithPhysics_UpdateMineCarPhysics(SithThing* pThing, float secDeltaTime)
{
    SithMineCarState* pState;
    rdVector3 normal;
    float dragFactor;
    float distance;
    float thrustScale;
    float accel;
    float speed;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    rdFace* pAttachedFace = pThing->attach.pFace;  // unreferenced; the debug build's frame keeps its slot
    float floorSlopeDot = 1.0f;
    static float sithPhysics_flt_538CAC = 0.1f;
    static float sithPhysics_flt_538CB0 = 1.25f;
    static float sithPhysics_flt_538CB4 = 1.0f;
    static float sithPhysics_flt_538CB8 = 4.0f;
    static float sithPhysics_flt_538CBC = 0.2f;
    static float sithPhysics_flt_538CC0 = 4.0f;
    static float sithPhysics_flt_538CC4 = 0.1f;
    static float sithPhysics_flt_538CC8 = 6.0f;

    if ( secDeltaTime > sithPhysics_flt_538CAC )
    {
        secDeltaTime = sithPhysics_flt_538CAC;
    }

    if ( !sithPhysics_CreateMineCarUserBlock(pThing) )
    {
        return;
    }

    pState = &pThing->userblock.pMinecar->state;
    accel  = 0.0f;
    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;

    sithPhysics_GetAttachedSurfaceInfo(pThing, &normal, &distance, &dragFactor);
    floorSlopeDot = rdVector_Dot3(&rdroid_zVector3, &normal);

    dragFactor = STDMATH_CLAMP(dragFactor, 0.25f, 1.0f);

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_UNKNOWN_8000) != 0 )
        {
            pPhysics->flags &= ~SITH_PF_UNKNOWN_8000;
        }
        else if ( SITHPHYSICS_ISZERO3(&pPhysics->thrust)
            && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) == 0
            && dragFactor > 0.8f )
        {
            if ( (pThing->flags & SITH_TF_DYING) != 0 )
            {
                sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(3.0f, dragFactor), RD_MULTIPLY(0.3f, dragFactor), secDeltaTime);
            }
            else
            {
                sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, dragFactor), RD_MULTIPLY(pPhysics->staticDrag, dragFactor), secDeltaTime);
            }
        }
        else
        {
            float deltaDrag;
            float surfDrag;
            float totalDrag;
            float pitch;
            float dragRate;

            pitch = pThing->orient.lvec.z;
            if ( pPhysics->thrust.y < 0.0f )
            {
                pitch = -pitch;
            }

            surfDrag = RD_MULTIPLY(pitch, sithPhysics_flt_538CC8) + 1.0f;
            if ( surfDrag < 0.0f )
            {
                surfDrag = 0.0f;
            }

            totalDrag = pPhysics->surfDrag + surfDrag;
            deltaDrag = totalDrag - pState->surfDrag;
            if ( SITHPHYSICS_ABS(pitch) < sithPhysics_flt_538CC4 )
            {
                if ( deltaDrag >= 0.0f )
                {
                    dragRate = RD_MULTIPLY(sithPhysics_flt_538CBC, secDeltaTime);
                }
                else
                {
                    dragRate = RD_MULTIPLY(sithPhysics_flt_538CB4, secDeltaTime);
                }
            }
            else if ( deltaDrag >= 0.0f )
            {
                dragRate = RD_MULTIPLY(sithPhysics_flt_538CC0, secDeltaTime);
            }
            else
            {
                dragRate = RD_MULTIPLY(sithPhysics_flt_538CB8, secDeltaTime);
            }

            if ( SITHPHYSICS_ABS(deltaDrag) > dragRate )
            {
                totalDrag = RD_MULTIPLY(SITHPHYSICS_SIGN(deltaDrag), dragRate) + pState->surfDrag;
            }

            pState->surfDrag = totalDrag;
            sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(totalDrag, dragFactor), 0.0f, secDeltaTime);
        }
    }

    speed       = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));
    thrustScale = RD_MULTIPLY(secDeltaTime, dragFactor);
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        if ( rdVector_Dot3(&pPhysics->velocity, &pThing->orient.lvec) > 0.0f )
        {
            if ( pPhysics->thrust.y >= 0.0f )
            {
                accel = RD_MULTIPLY(SITHPHYSICS_ABS(pPhysics->thrust.y), thrustScale);
            }
            else
            {
                accel = -RD_MULTIPLY(SITHPHYSICS_ABS(pPhysics->thrust.y), thrustScale);
                if ( SITHPHYSICS_ABS(accel) > speed )
                {
                    memset(&pPhysics->velocity, 0, sizeof(rdVector3));
                    accel = 0.0f;
                }
            }
        }
        else if ( pPhysics->thrust.y < 0.0f )
        {
            accel = RD_MULTIPLY(SITHPHYSICS_ABS(pPhysics->thrust.y), thrustScale);
        }
        else
        {
            accel = -RD_MULTIPLY(SITHPHYSICS_ABS(pPhysics->thrust.y), thrustScale);
            if ( SITHPHYSICS_ABS(accel) > speed )
            {
                memset(&pPhysics->velocity, 0, sizeof(rdVector3));
                accel = 0.0f;
            }
        }
    }
    else
    {
        accel = RD_MULTIPLY(SITHPHYSICS_ABS(pPhysics->thrust.y), thrustScale);
    }

    accel = stdMath_ClipNearZero(accel);
    speed = speed + accel;
    stdMath_ClipNearZero(speed); // The debug build evaluates it and drops the result (no assignment in the original).
    if ( speed > sithPhysics_flt_538CB0 )
    {
        speed = sithPhysics_flt_538CB0;
    }

    sithPhysics_ProcessMineCarTrackMove(pThing, secDeltaTime, speed, &pPhysics->deltaVelocity);

    if ( pPhysics->angularVelocity.y != 0.0f || pPhysics->angularVelocity.x != 0.0f )
    {
        rdVector3 pyr;
        rdMatrix34 rotMat;
        rdVector_Scale3(&pyr, &pPhysics->angularVelocity, secDeltaTime);
        rdMatrix_BuildRotate34(&rotMat, &pyr);
        sithCollision_RotateThing(pThing, &rotMat);
        rdMatrix_Normalize34(&pThing->orient);
    }

    if ( !pState->bUpdateSparks )
    {
        sithPhysics_UpdateVehicleChassis(pThing, &pThing->userblock.pMinecar->chassisInfo, secDeltaTime);
    }

    sithPhysics_UpdateTrackVehicleFx(pThing, pThing->userblock.pMinecar, secDeltaTime);
    sithPhysics_UpdateExhaustFx(pThing, &pThing->userblock.pMinecar->exhaustInfo, secDeltaTime);
}

static int J3DAPI sithPhysics_GetWaterNormalAtPos(const SithThing* pThing, float* pOutDistance, rdVector3* pOutNormal, const rdVector3* pPos, float secDeltaTime)
{
    int bNoFloor;
    SithSector* pSector;
    SithCollision* pCollision;
    rdVector3 startPos;
    rdVector3 moveNorm;
    static float sithPhysics_flt_538CCC = 1.5f;
    static float sithPhysics_flt_538CD0 = 0.019f;
    static float sithPhysics_flt_538CD4 = 0.45f;
    static float sithPhysics_flt_538CD8 = 0.45f;
    float moveDist;

    bNoFloor = 1;
    moveDist = RD_MULTIPLY(sithPhysics_flt_538CCC, secDeltaTime);
    moveDist = sithPhysics_GetThingHeight(pThing) + moveDist;
    moveDist += sithPhysics_flt_538CD0;

    rdVector_Copy3(&startPos, pPos);
    startPos.z = startPos.z + sithPhysics_flt_538CD0;
    rdVector_Neg3(&moveNorm, &rdroid_zVector3);
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.001f);
    if ( !pSector )
    {
        return 0;
    }

    sithCollision_SearchForCollisions(pSector, NULL, &startPos, &moveNorm, moveDist, 0.0f, 1);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            if ( (pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                rdVector_Copy3(pOutNormal, &pCollision->pSurfaceCollided->face.normal);
                *pOutDistance = pCollision->distance - sithPhysics_flt_538CD0;
                sithCollision_DecreaseStackLevel();
                return 3;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
            && (pCollision->pSurfaceCollided->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            if ( pCollision->pSurfaceCollided->face.normal.z >= sithPhysics_flt_538CD8 )
            {
                rdVector_Copy3(pOutNormal, &pCollision->pSurfaceCollided->face.normal);
                *pOutDistance = pCollision->distance - sithPhysics_flt_538CD0;
                sithCollision_DecreaseStackLevel();
                return 4;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && bNoFloor )
        {
            rdVector_Copy3(pOutNormal, &pCollision->pSurfaceCollided->face.normal);
            bNoFloor = 0;
            if ( pCollision->pSurfaceCollided->face.normal.z >= sithPhysics_flt_538CD4 )
            {
                *pOutDistance = pCollision->distance - sithPhysics_flt_538CD0;
            }
            else
            {
                *pOutDistance = sithPhysics_GetThingHeight(pThing);
            }
        }
    }

    sithCollision_DecreaseStackLevel();
    if ( bNoFloor == 1 )
    {
        *pOutDistance = moveDist - sithPhysics_flt_538CD0;
        rdVector_Copy3(pOutNormal, &rdroid_zVector3);
    }

    return bNoFloor;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_5, 3) /* end count-fit */
static void J3DAPI sithPhysics_GetWaterThrust(SithThing* pThing, rdVector3* a2, rdVector3* pOutThrust, float msDeltaTime)
{
    SithCollision* pCollision;
    float moveDistance;
    float zComp;
    float blend;
    rdVector3 moveNorm;
    SithSector* pSurfSector;
    rdVector3 dir;
    static float sithPhysics_flt_538CDC = 0.05f;

    if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
    {
        return;
    }

    pSurfSector  = NULL;
    moveDistance = sithPhysics_GetThingHeight(pThing) + sithPhysics_flt_538CDC;
    rdVector_Neg3(&moveNorm, &rdroid_zVector3);

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDistance, 0.001f, 1);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
            pSurfSector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
            if ( (pSurfSector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                break;
            }

            if ( (pSurfSector->flags & SITH_SECTOR_USETHRUST) != 0 )
            {
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_WORLD) != 0
            && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_WATER) != 0
            && (pCollision->pSurfaceCollided->pSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
        {
            pSurfSector = pCollision->pSurfaceCollided->pSector;
            break;
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( pSurfSector && (pSurfSector->flags & SITH_SECTOR_USETHRUST) != 0 )
    {
        blend = rdMath_DistancePointToPlane(&pSurfSector->thrust, a2, &rdroid_zeroVector3);
        zComp = rdVector_Dot3(a2, &rdroid_zVector3);
        blend = D3DVAL(blend / zComp);
        dir.x = RD_MULTIPLY(-blend, rdroid_zVector3.x) + pSurfSector->thrust.x;
        dir.y = RD_MULTIPLY(-blend, rdroid_zVector3.y) + pSurfSector->thrust.y;
        dir.z = RD_MULTIPLY(-blend, rdroid_zVector3.z) + pSurfSector->thrust.z;
        rdVector_Normalize3Acc(&dir);

        dir.x = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pSurfSector->thrust, &pSurfSector->thrust)), dir.x);
        dir.y = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pSurfSector->thrust, &pSurfSector->thrust)), dir.y);
        dir.z = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pSurfSector->thrust, &pSurfSector->thrust)), dir.z);
        rdVector_ScaleAdd3Acc(pOutThrust, &dir, msDeltaTime);
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_6, 30) CF_PROTO(sithPhysics_countfit_7, 28) /* end count-fit */
// update physics
void J3DAPI sithPhysics_UpdateRaftPhysics(SithThing* pThing, float secDeltaTime)
{
    size_t waterStatus;
    int aCornerStatus[4];
    float waterDist;
    float surfaceDist;
    float moveScale;
    rdVector3 aFaceNormals[4];
    rdVector3 aCornerNormals[4];
    rdVector3 accel;
    rdVector3 normal;
    rdVector3 aCorners[4];
    rdVector3 centerNormal;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    rdFace* pAttachedFace = pThing->attach.pFace;  // unreferenced; the debug build's frame keeps its slot
    float normalUpDot = 1.0f;
    static float sithPhysics_flt_538CE0 = 0.09f;
    static float sithPhysics_flt_538CE4 = 0.06f;
    static float sithPhysics_flt_538CE8 = 1.0f;
    static float sithPhysics_flt_538CEC = 0.4f;
    rdVector3 avgNormal;
    static float sithPhysics_flt_538CF0 = 0.3f;
    static float sithPhysics_flt_538CF4 = 1.1f;
    static float sithPhysics_flt_538CF8 = 0.05f;

    memset(&accel, 0, sizeof(accel));
    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;

    sithPhysics_GetAttachedSurfaceInfo(pThing, &normal, &surfaceDist, &moveScale);

    normalUpDot = rdVector_Dot3(&rdroid_zVector3, &normal);

    // Probe water surface at the four raft corners and at the center
    aCorners[0].x = sithPhysics_flt_538CE4;
    aCorners[0].y = sithPhysics_flt_538CE0;
    aCorners[0].z = 0.0f;
    rdMatrix_TransformPoint34Acc(&aCorners[0], &pThing->orient);
    rdVector_Add3Acc(&aCorners[0], &pThing->pos);
    aCornerStatus[0] = sithPhysics_GetWaterNormalAtPos(pThing, &waterDist, &aCornerNormals[0], &aCorners[0], secDeltaTime);
    aCorners[0].z = aCorners[0].z - waterDist;

    aCorners[1].x = -sithPhysics_flt_538CE4;
    aCorners[1].y = sithPhysics_flt_538CE0;
    aCorners[1].z = 0.0f;
    rdMatrix_TransformPoint34Acc(&aCorners[1], &pThing->orient);
    rdVector_Add3Acc(&aCorners[1], &pThing->pos);
    aCornerStatus[1] = sithPhysics_GetWaterNormalAtPos(pThing, &waterDist, &aCornerNormals[1], &aCorners[1], secDeltaTime);
    aCorners[1].z = aCorners[1].z - waterDist;

    aCorners[2].x = -sithPhysics_flt_538CE4;
    aCorners[2].y = -sithPhysics_flt_538CE0;
    aCorners[2].z = 0.0f;
    rdMatrix_TransformPoint34Acc(&aCorners[2], &pThing->orient);
    rdVector_Add3Acc(&aCorners[2], &pThing->pos);
    aCornerStatus[2] = sithPhysics_GetWaterNormalAtPos(pThing, &waterDist, &aCornerNormals[2], &aCorners[2], secDeltaTime);
    aCorners[2].z = aCorners[2].z - waterDist;

    aCorners[3].x = sithPhysics_flt_538CE4;
    aCorners[3].y = (-sithPhysics_flt_538CE0);
    aCorners[3].z = 0.0f;
    rdMatrix_TransformPoint34Acc(&aCorners[3], &pThing->orient);
    rdVector_Add3Acc(&aCorners[3], &pThing->pos);
    aCornerStatus[3] = sithPhysics_GetWaterNormalAtPos(pThing, &waterDist, &aCornerNormals[3], &aCorners[3], secDeltaTime);
    aCorners[3].z = aCorners[3].z - waterDist;

    {
        int frontJoint;
        int backJoint;
        rdVector3 edge1;
        rdVector3 edge2;
        rdVector3 faceNormal;
        rdVector3 upDir;
        rdVector3 projNormal;
        rdVector3 sideDir;
        float t;
        float dot;
        float frontAngle;
        float backAngle;
        rdVector3 center;

        rdVector_Copy3(&center, &pThing->pos);
        waterStatus = sithPhysics_GetWaterNormalAtPos(pThing, &waterDist, &centerNormal, &center, secDeltaTime);
        center.z = center.z - waterDist;
        pThing->userval = (float)waterStatus;

        // Average the water surface normal from the four corner triangles
        memset(&avgNormal, 0, sizeof(avgNormal));

        rdVector_Sub3(&edge1, &aCorners[1], &aCorners[0]);
        rdVector_Sub3(&edge2, &center, &aCorners[1]);
        rdVector_Cross3(&faceNormal, &edge1, &edge2);
        rdVector_Normalize3Acc(&faceNormal);
        rdVector_Copy3(&aFaceNormals[0], &faceNormal);
        rdVector_Add3Acc(&avgNormal, &faceNormal);

        rdVector_Sub3(&edge1, &aCorners[2], &aCorners[1]);
        rdVector_Sub3(&edge2, &center, &aCorners[2]);
        rdVector_Cross3(&faceNormal, &edge1, &edge2);
        rdVector_Normalize3Acc(&faceNormal);
        rdVector_Copy3(&aFaceNormals[1], &faceNormal);
        rdVector_Add3Acc(&avgNormal, &faceNormal);

        rdVector_Sub3(&edge1, &aCorners[3], &aCorners[2]);
        rdVector_Sub3(&edge2, &center, &aCorners[3]);
        rdVector_Cross3(&faceNormal, &edge1, &edge2);
        rdVector_Normalize3Acc(&faceNormal);
        rdVector_Copy3(&aFaceNormals[2], &faceNormal);
        rdVector_Add3Acc(&avgNormal, &faceNormal);

        rdVector_Sub3(&edge1, &aCorners[0], &aCorners[3]);
        rdVector_Sub3(&edge2, &center, &aCorners[0]);
        rdVector_Cross3(&faceNormal, &edge1, &edge2);
        rdVector_Normalize3Acc(&faceNormal);
        rdVector_Copy3(&aFaceNormals[3], &faceNormal);
        rdVector_Add3Acc(&avgNormal, &faceNormal);

        rdVector_Normalize3Acc(&avgNormal);

        // Tilt front and back raft joints
        frontJoint = sithThing_GetThingJointIndex(pThing, "inraftfr");
        backJoint  = sithThing_GetThingJointIndex(pThing, "inraftbk");

        t = rdMath_DistancePointToPlane(&avgNormal, &pThing->orient.rvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&upDir, &pThing->orient.rvec, -t, &avgNormal);
        rdVector_Normalize3Acc(&upDir);
        rdVector_Cross3(&sideDir, &upDir, &pThing->orient.rvec);

        t = rdMath_DistancePointToPlane(&aFaceNormals[0], &pThing->orient.rvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&projNormal, &pThing->orient.rvec, -t, &aFaceNormals[0]);
        rdVector_Normalize3Acc(&projNormal);
        dot = rdVector_Dot3(&projNormal, &sideDir);
        frontAngle = stdMath_ArcSin1(dot);
        if ( frontJoint != -1 )
        {
            pThing->renderData.apTweakedAngles[frontJoint].pitch = frontAngle * sithPhysics_flt_538CE8;
        }

        t = rdMath_DistancePointToPlane(&aFaceNormals[2], &pThing->orient.rvec, &rdroid_zeroVector3);
        rdVector_ScaleAdd3(&projNormal, &pThing->orient.rvec, -t, &aFaceNormals[2]);
        rdVector_Normalize3Acc(&projNormal);
        dot = rdVector_Dot3(&projNormal, &sideDir);
        backAngle = stdMath_ArcSin1(dot);
        if ( backJoint != -1 )
        {
            pThing->renderData.apTweakedAngles[backJoint].pitch = backAngle * sithPhysics_flt_538CE8;
        }
    }

    sithPhysics_SetThingLook(pThing, &avgNormal, pPhysics->orientSpeed * secDeltaTime);

    // Angular movement
    if ( (pPhysics->flags & SITH_PF_USEANGULARTHRUST) != 0 )
    {
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
        {
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, pPhysics->surfDrag + sithPhysics_flt_538CEC, 0.0f, secDeltaTime);
        }

        if ( waterStatus >= 3 )
        {
            rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity);
        }

        pPhysics->angularVelocity.y += RD_MULTIPLY(pPhysics->rotThrust.y, secDeltaTime);

        SITHPHYSICS_CLIPPRECISION3(&pPhysics->angularVelocity);
    }

    if ( pPhysics->angularVelocity.y != 0.0f )
    {
        rdVector3 rotDelta;
        rdMatrix34 rotMat;

        rdVector_Scale3(&rotDelta, &pPhysics->angularVelocity, secDeltaTime);
        rdMatrix_BuildRotate34(&rotMat, &rotDelta);
        sithCollision_RotateThing(pThing, &rotMat);
        rdMatrix_Normalize34(&pThing->orient);
    }

    moveScale = STDMATH_CLAMP(moveScale, 0.25f, 1.0f);

    // Linear movement
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_UNKNOWN_8000) != 0 )
        {
            pPhysics->flags &= ~SITH_PF_UNKNOWN_8000;
        }
        else
        {
            if ( SITHPHYSICS_ISZERO3(&pPhysics->thrust) && (pThing->pInSector->flags & SITH_SECTOR_USETHRUST) == 0 && moveScale > 0.8f )
            {
                sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, moveScale), RD_MULTIPLY(pPhysics->staticDrag, moveScale), secDeltaTime);
            }
            else
            {
                sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, moveScale), 0.0f, secDeltaTime);
            }
        }
    }

    if ( (pPhysics->flags & SITH_PF_USETHRUST) != 0 && !SITHPHYSICS_ISZERO3(&pPhysics->thrust) && waterStatus >= 3 )
    {
        float thrustScale = secDeltaTime * moveScale;
        rdVector_Scale3(&accel, &pPhysics->thrust, thrustScale);
        SITHPHYSICS_CLIPPRECISION3(&accel);
        if ( !SITHPHYSICS_ISZERO3(&accel) )
        {
            rdMatrix_TransformVector34Acc(&accel, &pThing->orient);
        }
    }

    if ( normalUpDot < 1.0f )
    {
        rdVector3 slopeAxis;
        rdVector3 slopeDir;
        float slopeForce;

        rdVector_Cross3(&slopeAxis, &normal, &rdroid_zVector3);
        rdVector_Cross3(&slopeDir, &normal, &slopeAxis);
        rdVector_Normalize3Acc(&slopeDir);
        slopeForce = RD_MULTIPLY(1.0f - normalUpDot, sithPhysics_flt_538CF0);
        slopeForce = RD_MULTIPLY(slopeForce, secDeltaTime);
        rdVector_ScaleAdd3Acc(&accel, &slopeDir, slopeForce);
    }

    sithPhysics_GetWaterThrust(pThing, &normal, &accel, secDeltaTime);
    rdVector_Add3Acc(&pPhysics->velocity, &accel);

    // Push away from floor hit at raft corners
    {
        float t;
        rdVector3 surfDir;
        float velDot;

        if ( !aCornerStatus[0] )
        {
            t = rdMath_DistancePointToPlane(&aCornerNormals[0], &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&surfDir, &pThing->orient.uvec, -t, &aCornerNormals[0]);
            rdVector_Normalize3Acc(&surfDir);
            velDot = rdVector_Dot3(&pPhysics->velocity, &surfDir);
            if ( velDot < 0.0f )
            {
                velDot = RD_MULTIPLY(velDot, sithPhysics_flt_538CF4);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &surfDir, -velDot);
            }
        }

        if ( !aCornerStatus[1] )
        {
            t = rdMath_DistancePointToPlane(&aCornerNormals[1], &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&surfDir, &pThing->orient.uvec, -t, &aCornerNormals[1]);
            rdVector_Normalize3Acc(&surfDir);
            velDot = rdVector_Dot3(&pPhysics->velocity, &surfDir);
            if ( velDot < 0.0f )
            {
                velDot = RD_MULTIPLY(velDot, sithPhysics_flt_538CF4);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &surfDir, -velDot);
            }
        }

        if ( !aCornerStatus[2] )
        {
            t = rdMath_DistancePointToPlane(&aCornerNormals[2], &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&surfDir, &pThing->orient.uvec, -t, &aCornerNormals[2]);
            rdVector_Normalize3Acc(&surfDir);
            velDot = rdVector_Dot3(&pPhysics->velocity, &surfDir);
            if ( velDot < 0.0f )
            {
                velDot = RD_MULTIPLY(velDot, sithPhysics_flt_538CF4);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &surfDir, -velDot);
            }
        }

        if ( !aCornerStatus[3] )
        {
            t = rdMath_DistancePointToPlane(&aCornerNormals[3], &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&surfDir, &pThing->orient.uvec, -t, &aCornerNormals[3]);
            rdVector_Normalize3Acc(&surfDir);
            velDot = rdVector_Dot3(&pPhysics->velocity, &surfDir);
            if ( velDot < 0.0f )
            {
                velDot = RD_MULTIPLY(velDot, sithPhysics_flt_538CF4);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &surfDir, -velDot);
            }
        }
    }

    {
        float t;
        rdVector3 velDir;
        float zDot;

        // Keep velocity along the attached surface
        t = rdMath_DistancePointToPlane(&pPhysics->velocity, &normal, &rdroid_zeroVector3);
        zDot = rdVector_Dot3(&normal, &rdroid_zVector3);
        t = D3DVAL(t / zDot);
        rdVector_ScaleAdd3(&velDir, &rdroid_zVector3, -t, &pPhysics->velocity);
        rdVector_Normalize3Acc(&velDir);
        velDir.x = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), velDir.x);
        velDir.y = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), velDir.y);
        velDir.z = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), velDir.z);
        rdVector_Copy3(&pPhysics->velocity, &velDir);
    }

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        float velDot = rdVector_Dot3(&pPhysics->velocity, &normal);
        velDot = stdMath_ClipNearZero(velDot);
        if ( velDot < 0.0f )
        {
            rdVector_ScaleAdd3Acc(&pPhysics->velocity, &normal, -velDot);
        }
    }

    SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    surfaceDist = surfaceDist - sithPhysics_GetThingHeight(pThing);
    surfaceDist = stdMath_ClipNearZero(surfaceDist);
    if ( surfaceDist != 0.0f )
    {
        float maxDelta = RD_MULTIPLY(sithPhysics_flt_538CF8, secDeltaTime);
        surfaceDist = STDMATH_CLAMP(surfaceDist, -maxDelta, maxDelta);
        rdVector_ScaleAdd3Acc(&pPhysics->deltaVelocity, &normal, -surfaceDist);
    }
}

// Retail pushes this assert's __LINE__ (3498) in UpdateAttachedThingPhysics, UpdateMineCarPhysics and
// UpdateRaftPhysics: one function defined here and inlined into all three. Name and interface inferred.
static __inline void J3DAPI sithPhysics_GetAttachedSurfaceInfo(SithThing* pThing, rdVector3* pOutNormal, float* pOutDistance, float* pOutDragFactor)
{
#line 3498
    SITH_ASSERTREL(pThing->attach.flags);
    if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
    {
        SithSurface* pSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
        rdVector_Copy3(pOutNormal, &pThing->attach.pFace->normal);
        *pOutDistance = rdMath_DistancePointToPlane(&pThing->pos, pOutNormal, &pThing->attach.attachedFaceFirstVert);
        *pOutDragFactor = 1.0f;
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_THINGFACE) != 0 )
    {
        rdVector3 vert;
        SithThing* pAttachedThing = pThing->attach.attachedToStructure.pThingAttached;
        rdMatrix_TransformVector34(pOutNormal, &pThing->attach.pFace->normal, &pAttachedThing->orient);
        rdMatrix_TransformVector34(&vert, &pThing->attach.attachedFaceFirstVert, &pAttachedThing->orient);
        rdVector_Add3Acc(&vert, &pAttachedThing->pos);
        *pOutDistance = rdMath_DistancePointToPlane(&pThing->pos, pOutNormal, &vert);
        *pOutDragFactor = 1.0f;
    }
    else if ( (pThing->attach.flags & SITH_ATTACH_CLIMBSURFACE) != 0 )
    {
        SithSurface* pSurface = pThing->attach.attachedToStructure.pSurfaceAttached;
        rdVector_Copy3(pOutNormal, &pThing->attach.pFace->normal);
        *pOutDistance = rdMath_DistancePointToPlane(&pThing->pos, pOutNormal, &pThing->attach.attachedFaceFirstVert);
        *pOutDragFactor = 1.0f;
    }
}

void J3DAPI sithPhysics_ProcessMineCarTrackMove(SithThing* pThing, float secDeltaTime, float speed, rdVector3* pOutMoveDelta)
{
    SithPhysicsTrackFaceData data;
    rdVector3 pos;
    rdVector3 moveDir;
    rdVector3 rightDir;
    rdVector3 curPos;
    rdVector3 newPos;
    rdFace* pCurFace;
    float secRemaining;
    float yaw;
    float pitch;
    int bDone;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    SithMineCarState* pState = &pThing->userblock.pMinecar->state;
    int bReverse = 0;

    if ( !pThing->attach.pFace )
    {
        return;
    }

    rdVector_Copy3(&pos, &pThing->pos);
    pos.z = pos.z - sithPhysics_GetThingHeight(pThing);
    rdVector_Copy3(&curPos, &pos);
    secRemaining = secDeltaTime;
    pCurFace     = pThing->attach.pFace;

    if ( SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        rdVector_Normalize3(&moveDir, &pThing->orient.lvec);
        rdVector_Normalize3(&rightDir, &pThing->orient.rvec);
        if ( pPhysics->thrust.y < 0.0f )
        {
            bReverse = 1;
            rdVector_Neg3Acc(&moveDir);
            rdVector_Neg3Acc(&rightDir);
        }
    }
    else
    {
        rdVector_Normalize3(&moveDir, &pPhysics->velocity);
        rdVector_Cross3(&rightDir, &moveDir, &pThing->orient.uvec);
        if ( rdVector_Dot3(&pPhysics->velocity, &pThing->orient.lvec) < 0.0f )
        {
            bReverse = 1;
        }
    }

    bDone = 0;
    while ( !bDone )
    {
        if ( !sithPhysics_ProcessTrackFace(pThing, pCurFace, &moveDir, &rightDir, &curPos, &data) )
        {
#line 3587
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't process track face during move.\n", 0, 0, 0, 0);
            goto error;
        }

        sithPhysics_MoveToTrackFaceFront(pThing, &data, &secRemaining, &curPos, &newPos, speed);
        if ( secRemaining == 0.0f )
        {
            bDone = 1;
        }
        else
        {
            rdVector3 step;

            rdVector_Scale3(&pPhysics->rotThrust, &data.forward, -1.0f);
            if ( bReverse )
            {
                pPhysics->buoyancy = -1.0f;
            }
            else
            {
                pPhysics->buoyancy = 1.0f;
            }

            rdVector_Copy3(&curPos, &newPos);
            rdVector_Scale3(&step, &data.forward, 0.01f);
            rdVector_Add3Acc(&newPos, &step);
            if ( !sithPhysics_CheckForPointOnTrack(pThing, &newPos, &pCurFace, 1, pCurFace) )
            {
#line 3620
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't find track face ahead of current face during move.\n", 0, 0, 0, 0);
                goto error;
            }

            rdVector_Copy3(&moveDir, &data.forward);
            rdVector_Copy3(&rightDir, &data.right);
        }
    }

    rdVector_Sub3(&moveDir, &newPos, &curPos);
    SITHPHYSICS_CLIPPRECISION3(&moveDir);
    rdVector_Normalize3Acc(&moveDir);

    rdVector_Sub3(pOutMoveDelta, &newPos, &pos);
    SITHPHYSICS_CLIPPRECISION3(pOutMoveDelta);

    rdVector_Scale3(&pPhysics->velocity, &moveDir, speed);

    {
        SithPhysicsTrackFaceData nextData;
        rdVector3 probePos;
        rdVector3 blendDir;
        rdVector3 fwdPart;
        rdVector3 nextPart;
        rdFace* pFace;
        float distToFront;
        float distFromBack;
        float length;
        float t;
        int bBlend;

        bBlend = 1;
        distToFront = -rdMath_DistancePointToPlane(&newPos, &data.forward, &data.front);
        if ( distToFront < 0.0f )
        {
            distToFront = 0.0f;
        }

        distFromBack = rdMath_DistancePointToPlane(&newPos, &data.forward, &data.back);
        if ( distFromBack < 0.0f )
        {
            distFromBack = 0.0f;
        }

        length = rdMath_DistancePointToPlane(&data.front, &data.forward, &data.back);

        if ( distToFront > distFromBack )
        {
            if ( bReverse && pPhysics->buoyancy < 0.0f )
            {
                rdVector_Copy3(&nextData.forward, &pPhysics->rotThrust);
            }
            else if ( !bReverse && pPhysics->buoyancy > 0.0f )
            {
                rdVector_Copy3(&nextData.forward, &pPhysics->rotThrust);
            }
            else
            {
                rdVector3 backFwd;
                rdVector3 backRight;

                rdVector_Scale3(&backFwd, &data.forward, -1.0f);
                rdVector_Scale3(&backRight, &data.right, -1.0f);
                rdVector_ScaleAdd3(&probePos, &backFwd, 0.025f, &data.back);
                if ( !sithPhysics_CheckForPointOnTrack(pThing, &probePos, &pFace, 0, pCurFace) )
                {
#line 3679
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't find track behind during orientation.\n", 0, 0, 0, 0);
                    goto error;
                }

                if ( !sithPhysics_ProcessTrackFace(pThing, pFace, &backFwd, &backRight, &probePos, &nextData) )
                {
#line 3684
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't process track face behind during orientation.\n", 0, 0, 0, 0);
                    goto error;
                }
            }

            if ( length > pState->unknown9 )
            {
                if ( RD_MULTIPLY(pState->unknown10, 0.5f) >= distFromBack )
                {
                    length = pState->unknown10;
                }
                else
                {
                    bBlend = 0;
                }
            }

            if ( bBlend )
            {
                t = 0.5f;
                t = distFromBack / length + t;
                t = STDMATH_CLAMP(t, 0.0f, 1.0f);
                rdVector_Scale3(&fwdPart, &data.forward, t);
                rdVector_Scale3(&nextPart, &nextData.forward, -(1.0f - t));
                rdVector_Add3(&blendDir, &fwdPart, &nextPart);
                rdVector_Normalize3Acc(&blendDir);
            }
            else
            {
                rdVector_Copy3(&blendDir, &data.forward);
            }
        }
        else
        {
            rdVector_ScaleAdd3(&probePos, &data.forward, 0.025f, &data.front);
            if ( !sithPhysics_CheckForPointOnTrack(pThing, &probePos, &pFace, 0, pCurFace) )
            {
#line 3726
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't find track face ahead during orientation.\n", 0, 0, 0, 0);
                goto error;
            }

            if ( !sithPhysics_ProcessTrackFace(pThing, pFace, &data.forward, &data.right, &probePos, &nextData) )
            {
#line 3731
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't process track face ahead during orientation.\n", 0, 0, 0, 0);
                goto error;
            }

            if ( length > pState->unknown9 )
            {
                if ( RD_MULTIPLY(pState->unknown10, 0.5f) >= distToFront )
                {
                    length = pState->unknown10;
                }
                else
                {
                    bBlend = 0;
                }
            }

            if ( bBlend )
            {
                t = 0.5f;
                t = distToFront / length + t;
                t = STDMATH_CLAMP(t, 0.0f, 1.0f);
                rdVector_Scale3(&fwdPart, &data.forward, t);
                rdVector_Scale3(&nextPart, &nextData.forward, 1.0f - t);
                rdVector_Add3(&blendDir, &fwdPart, &nextPart);
                rdVector_Normalize3Acc(&blendDir);
            }
            else
            {
                rdVector_Copy3(&blendDir, &data.forward);
            }
        }

        if ( bReverse )
        {
            rdVector_Neg3Acc(&blendDir);
        }

        {
            rdVector3 lookFlat;
            rdVector3 blendFlat;
            rdVector3 lookPitch;
            rdVector3 blendPitch;
            rdVector3 axis;
            float lookLen;
            float blendLen;

            rdVector_Copy3(&lookFlat, &pThing->orient.lvec);
            lookFlat.z = 0.0f;
            rdVector_Copy3(&blendFlat, &blendDir);
            blendFlat.z = 0.0f;
            rdVector_Normalize3Acc(&blendFlat);
            rdVector_Normalize3Acc(&lookFlat);
            yaw = rdMath_DeltaAngleNormalized(&lookFlat, &blendFlat, &rdroid_zVector3);

            lookLen  = sqrtf(RD_MULTIPLY(lookFlat.x, lookFlat.x) + RD_MULTIPLY(lookFlat.y, lookFlat.y) + RD_MULTIPLY(lookFlat.z, lookFlat.z));
            blendLen = sqrtf(RD_MULTIPLY(blendFlat.x, blendFlat.x) + RD_MULTIPLY(blendFlat.y, blendFlat.y) + RD_MULTIPLY(blendFlat.z, blendFlat.z));

            lookPitch.x = lookLen;
            lookPitch.y = 0.0f;
            lookPitch.z = pThing->orient.lvec.z;
            rdVector_Normalize3Acc(&lookPitch);

            blendPitch.x = blendLen;
            blendPitch.y = 0.0f;
            blendPitch.z = blendDir.z;
            rdVector_Normalize3Acc(&blendPitch);

            rdVector_Set3(&axis, 0.0f, 1.0f, 0.0f);
            pitch = rdMath_DeltaAngleNormalized(&blendPitch, &lookPitch, &axis);
        }
    }

    pPhysics->angularVelocity.y = D3DVAL(yaw / secDeltaTime);
    pPhysics->angularVelocity.x = D3DVAL(pitch / secDeltaTime);

    switch ( pState->maybeMoveSate )
    {
        case 0:
            if ( pPhysics->angularVelocity.y > 20.0f )
            {
                pState->maybeMoveSate = 1;
            }
            else if ( pPhysics->angularVelocity.y < -20.0f )
            {
                pState->maybeMoveSate = 3;
            }
            break;

        case 1:
            pState->maybeMoveSate = 2;
            break;

        case 2:
            if ( pPhysics->angularVelocity.y < 20.0f )
            {
                pState->maybeMoveSate = 5;
            }
            break;

        case 3:
            pState->maybeMoveSate = 4;
            break;

        case 4:
            if ( pPhysics->angularVelocity.y > -20.0f )
            {
                pState->maybeMoveSate = 5;
            }
            break;

        case 5:
            pState->maybeMoveSate = 0;
            break;

        default:
            pState->maybeMoveSate = 0;
            break;
    }

    return;

error:
    memset(&pPhysics->angularVelocity, 0, sizeof(rdVector3));
    memset(&pPhysics->velocity, 0, sizeof(rdVector3));
    memset(&pPhysics->thrust, 0, sizeof(rdVector3));
    memset(pOutMoveDelta, 0, sizeof(rdVector3));
}
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_8, 10) /* end count-fit */
int J3DAPI sithPhysics_ProcessTrackFace(SithThing* pThing, rdFace* pFace, const rdVector3* pMoveDir, const rdVector3* pRightDir, const rdVector3* pPos, SithPhysicsTrackFaceData* pData)
{
    size_t aFrontRights[2]; /* release-chosen type: /Od code is the same for size_t and int */
    rdVector3 forward;
    size_t aFrontLefts[2]; /* release-chosen type: /Od code is the same for size_t and int */
    size_t aBackRights[2]; /* release-chosen type: /Od code is the same for size_t and int */
    rdVector3 right;
    int aBackLefts[2]; /* release-chosen type: /Od code is the same for size_t and int */
    rdVector3 edgeNext;
    rdVector3 edgePrev;
    rdVector3 forwardSum;
    rdVector3 rightSum;
    int prev;
    size_t next;
    float angle;
    size_t nFrontLefts;
    size_t nFrontRights;
    size_t nBackLefts;
    size_t nBackRights;
    size_t i;
    int edgeIdx;

    rdVector_Copy3(&forward, pMoveDir);
    rdVector_Copy3(&right, pRightDir);

    for ( i = 0; i < 4; i++ )
    {
        pData->aForwardDists[i] = 0.0f;
        pData->aRightDists[i]   = 0.0f;
        pData->aVertexSides[i]  = 0;
        pData->nVertices        = 0;
    }

    memset(&pData->center, 0, sizeof(rdVector3));

    for ( i = 0; i < pFace->numVertices; i++ )
    {
        prev = i - 1;
        if ( prev < 0 )
        {
            prev = pFace->numVertices - 1;
        }

        next = (i + 1) % pFace->numVertices;
        rdVector_Sub3(&edgeNext, &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[next]], &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]]);
        rdVector_Sub3(&edgePrev, &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]], &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[prev]]);
        rdVector_Normalize3Acc(&edgeNext);
        rdVector_Normalize3Acc(&edgePrev);

        angle = stdMath_NormalizeAngle(rdMath_DeltaAngleNormalized(&edgeNext, &edgePrev, &pFace->normal));
        if ( angle >= 5.0f && angle <= 355.0f )
        {
            if ( pData->nVertices >= 4 )
            {
#line 3907
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many vertices on track face.\n", 0, 0, 0, 0);
                goto error;
            }

            rdVector_Copy3(&pData->aVertices[pData->nVertices], &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]]);
            rdVector_Add3Acc(&pData->center, &pData->aVertices[pData->nVertices]);
            pData->nVertices++;
        }
    }

#line 3916
    SITH_ASSERTREL(( pData->nVertices == 3 ) || ( pData->nVertices == 4 ));
    rdVector_Scale3Acc(&pData->center, D3DVAL(1.0f / pData->nVertices)); /* spelling-fit */

    if ( pData->nVertices == 4 )
    {
        edgeIdx = 0;
        for ( i = 0; i < pData->nVertices; i++ )
        {
            next = (i + 1) % pData->nVertices;
            rdVector_Sub3(&pData->aEdges[i], &pData->aVertices[next], &pData->aVertices[i]);
            rdVector_Normalize3Acc(&pData->aEdges[i]);
            pData->aEdgeForwardDots[i] = rdVector_Dot3(&pData->aEdges[i], pMoveDir);
            pData->aEdgeRightDots[i]   = rdVector_Dot3(&pData->aEdges[i], pRightDir);
        }

        memset(&forwardSum, 0, sizeof(rdVector3));
        memset(&rightSum, 0, sizeof(rdVector3));

        if ( SITHPHYSICS_ABS(pData->aEdgeForwardDots[0]) > SITHPHYSICS_ABS(pData->aEdgeForwardDots[1]) )
        {
            rdVector_ScaleAdd3Acc(&forwardSum, &pData->aEdges[0], SITHPHYSICS_SIGN(pData->aEdgeForwardDots[0]));
            rdVector_ScaleAdd3Acc(&forwardSum, &pData->aEdges[2], SITHPHYSICS_SIGN(pData->aEdgeForwardDots[2]));
            rdVector_ScaleAdd3Acc(&rightSum, &pData->aEdges[1], SITHPHYSICS_SIGN(pData->aEdgeRightDots[1]));
            rdVector_ScaleAdd3Acc(&rightSum, &pData->aEdges[3], SITHPHYSICS_SIGN(pData->aEdgeRightDots[3]));
        }
        else
        {
            rdVector_ScaleAdd3Acc(&forwardSum, &pData->aEdges[1], SITHPHYSICS_SIGN(pData->aEdgeForwardDots[1]));
            rdVector_ScaleAdd3Acc(&forwardSum, &pData->aEdges[3], SITHPHYSICS_SIGN(pData->aEdgeForwardDots[3]));
            rdVector_ScaleAdd3Acc(&rightSum, &pData->aEdges[0], SITHPHYSICS_SIGN(pData->aEdgeRightDots[0]));
            rdVector_ScaleAdd3Acc(&rightSum, &pData->aEdges[2], SITHPHYSICS_SIGN(pData->aEdgeRightDots[2]));
        }

        rdVector_Normalize3Acc(&forwardSum);
        rdVector_Normalize3Acc(&rightSum);
        rdVector_Cross3(&forward, &rdroid_zVector3, &rightSum);
        rdVector_Cross3(&right, &forwardSum, &rdroid_zVector3);
    }

    for ( i = 0; i < pData->nVertices; i++ )
    {
        pData->aForwardDists[i] = rdMath_DistancePointToPlane(&pData->aVertices[i], &forward, &pData->center);
        pData->aRightDists[i] = rdMath_DistancePointToPlane(&pData->aVertices[i], &right, &pData->center);
    }

    for ( i = 0; i < pData->nVertices; i++ )
    {
        if ( pData->aRightDists[i] <= 0.0f )
        {
            pData->aVertexSides[i] |= SITHPHYSICS_TRACKVERT_LEFT;
        }
        else
        {
            pData->aVertexSides[i] |= SITHPHYSICS_TRACKVERT_RIGHT;
        }

        if ( pData->aForwardDists[i] <= 0.0f )
        {
            pData->aVertexSides[i] |= SITHPHYSICS_TRACKVERT_BACK;
        }
        else
        {
            pData->aVertexSides[i] |= SITHPHYSICS_TRACKVERT_FRONT;
        }
    }

    nFrontRights = nFrontLefts = nBackRights = nBackLefts = 0;
    for ( i = 0; i < pData->nVertices; i++ )
    {
        if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_FRONT) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_RIGHT) )
        {
#line 4004
            SITH_ASSERTREL(nFrontRights < 2);
            aFrontRights[nFrontRights++] = i;
        }
        else if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_FRONT) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_LEFT) )
        {
#line 4010
            SITH_ASSERTREL(nFrontLefts < 2);
            aFrontLefts[nFrontLefts++] = i;
        }
        else if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_BACK) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_RIGHT) )
        {
#line 4016
            SITH_ASSERTREL(nBackRights < 2);
            aBackRights[nBackRights++] = i;
        }
        else if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_BACK) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_LEFT) )
        {
#line 4022
            SITH_ASSERTREL(nBackLefts < 2);
            aBackLefts[nBackLefts++] = i;
        }
    }

    if ( nFrontRights == 2 )
    {
        if ( nFrontLefts == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aRightDists[aFrontRights[0]]) < SITHPHYSICS_ABS(pData->aRightDists[aFrontRights[1]]) )
            {
                pData->aVertexSides[aFrontRights[0]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_LEFT;
            }
            else
            {
                pData->aVertexSides[aFrontRights[1]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_LEFT;
            }
        }
        else if ( nBackRights == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aForwardDists[aFrontRights[0]]) < SITHPHYSICS_ABS(pData->aForwardDists[aFrontRights[1]]) )
            {
                pData->aVertexSides[aFrontRights[0]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_RIGHT;
            }
            else
            {
                pData->aVertexSides[aFrontRights[1]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_RIGHT;
            }
        }
        else
        {
#line 4059
            SITH_ASSERTREL(0);
        }
    }
    else if ( nFrontLefts == 2 )
    {
        if ( nFrontRights == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aRightDists[aFrontLefts[0]]) < SITHPHYSICS_ABS(pData->aRightDists[aFrontLefts[1]]) )
            {
                pData->aVertexSides[aFrontLefts[0]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_RIGHT;
            }
            else
            {
                pData->aVertexSides[aFrontLefts[1]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_RIGHT;
            }
        }
        else if ( nBackLefts == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aForwardDists[aFrontLefts[0]]) < SITHPHYSICS_ABS(pData->aForwardDists[aFrontLefts[1]]) )
            {
                pData->aVertexSides[aFrontLefts[0]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_LEFT;
            }
            else
            {
                pData->aVertexSides[aFrontLefts[1]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_LEFT;
            }
        }
        else
        {
#line 4088
            SITH_ASSERTREL(0);
        }
    }

    if ( nBackRights == 2 )
    {
        if ( nBackLefts == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aRightDists[aBackRights[0]]) < SITHPHYSICS_ABS(pData->aRightDists[aBackRights[1]]) )
            {
                pData->aVertexSides[aBackRights[0]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_LEFT;
            }
            else
            {
                pData->aVertexSides[aBackRights[1]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_LEFT;
            }
        }
        else if ( nFrontRights == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aForwardDists[aBackRights[0]]) < SITHPHYSICS_ABS(pData->aForwardDists[aBackRights[1]]) )
            {
                pData->aVertexSides[aBackRights[0]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_RIGHT;
            }
            else
            {
                pData->aVertexSides[aBackRights[1]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_RIGHT;
            }
        }
        else
        {
#line 4118
            SITH_ASSERTREL(0);
        }
    }
    else if ( nBackLefts == 2 )
    {
        if ( nBackRights == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aRightDists[aBackLefts[0]]) < SITHPHYSICS_ABS(pData->aRightDists[aBackLefts[1]]) )
            {
                pData->aVertexSides[aBackLefts[0]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_RIGHT;
            }
            else
            {
                pData->aVertexSides[aBackLefts[1]] = SITHPHYSICS_TRACKVERT_BACK | SITHPHYSICS_TRACKVERT_RIGHT;
            }
        }
        else if ( nFrontLefts == 0 )
        {
            if ( SITHPHYSICS_ABS(pData->aForwardDists[aBackLefts[0]]) < SITHPHYSICS_ABS(pData->aForwardDists[aBackLefts[1]]) )
            {
                pData->aVertexSides[aBackLefts[0]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_LEFT;
            }
            else
            {
                pData->aVertexSides[aBackLefts[1]] = SITHPHYSICS_TRACKVERT_FRONT | SITHPHYSICS_TRACKVERT_LEFT;
            }
        }
        else
        {
#line 4147
            SITH_ASSERTREL(0);
        }
    }

    for ( i = 0; i < pData->nVertices; i++ )
    {
        if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_FRONT) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_RIGHT) )
        {
            rdVector_Copy3(&pData->frontRight, &pData->aVertices[i]);
        }
        else if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_FRONT) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_LEFT) )
        {
            rdVector_Copy3(&pData->frontLeft, &pData->aVertices[i]);
        }
        else if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_BACK) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_RIGHT) )
        {
            rdVector_Copy3(&pData->backRight, &pData->aVertices[i]);
        }
        else if ( (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_BACK) && (pData->aVertexSides[i] & SITHPHYSICS_TRACKVERT_LEFT) )
        {
            rdVector_Copy3(&pData->backLeft, &pData->aVertices[i]);
        }
    }

    if ( pData->nVertices == 3 )
    {
        if ( nBackLefts == 0 )
        {
            rdVector_Copy3(&pData->backLeft, &pData->frontLeft);
        }
        else if ( nFrontLefts == 0 )
        {
            rdVector_Copy3(&pData->frontLeft, &pData->backLeft);
        }
        else if ( nBackRights == 0 )
        {
            rdVector_Copy3(&pData->backRight, &pData->frontRight);
        }
        else if ( nFrontRights == 0 )
        {
            rdVector_Copy3(&pData->frontRight, &pData->backRight);
        }
    }

    rdVector_Add3(&pData->back, &pData->backRight, &pData->backLeft);
    rdVector_Scale3Acc(&pData->back, 0.5f);
    rdVector_Add3(&pData->front, &pData->frontRight, &pData->frontLeft);
    rdVector_Scale3Acc(&pData->front, 0.5f);

    if ( pData->nVertices == 3 )
    {
        rdVector3 probeDir;
        rdVector3 probePos;
        rdFace* pFoundFace;

        rdVector_Sub3(&probeDir, &pData->front, &pData->back);
        rdVector_Normalize3Acc(&probeDir);
        rdVector_Scale3Acc(&probeDir, 0.025f);
        rdVector_Add3(&probePos, &probeDir, &pData->front);
        if ( !sithPhysics_CheckForPointOnTrack(pThing, &probePos, &pFoundFace, 0, pFace) )
        {
            rdVector3 leftEdge;

            rdVector_Sub3(&leftEdge, &pData->backLeft, &pData->frontLeft);
            if ( SITHPHYSICS_ISZERO3(&leftEdge) )
            {
                rdVector_Add3(&pData->front, &pData->frontRight, &pData->backRight);
                rdVector_Scale3Acc(&pData->front, 0.5f);
            }
            else
            {
                rdVector_Add3(&pData->front, &pData->frontLeft, &pData->backLeft);
                rdVector_Scale3Acc(&pData->front, 0.5f);
            }
        }
    }

    rdVector_Add3(&pData->middle, &pData->back, &pData->front);
    rdVector_Scale3Acc(&pData->middle, 0.5f);
    rdVector_Sub3(&pData->forward, &pData->front, &pData->back);
    rdVector_Normalize3Acc(&pData->forward);
    rdVector_Cross3(&pData->right, &pData->forward, &rdroid_zVector3);
    rdVector_Normalize3Acc(&pData->right);

    pData->distanceToFront  = -rdMath_DistancePointToPlane(pPos, &pData->forward, &pData->front);
    pData->distanceFromBack = rdMath_DistancePointToPlane(pPos, &pData->forward, &pData->back);
    return 1;

error:
    return 0;
}

int J3DAPI sithPhysics_CheckForPointOnTrack(SithThing* pThing, const rdVector3* pPos, rdFace** ppOutFoundFace, int bUpdateState, rdFace* pPrevFace)
{
    rdVector3 startPos;
    rdVector3 moveNorm;
    SithMineCarState* pState;
    SithSector* pCurSector;
    SithCollision* pCollision;
    SithSurface* pTrackSurface;
    int bExists;
    int options;

    pState = &pThing->userblock.pMinecar->state;
    rdVector_Set3(&moveNorm, 0.0f, 0.0f, -1.0f);
    rdVector_Copy3(&startPos, pPos);
    rdVector_ScaleAdd3Acc(&startPos, &rdroid_zVector3, 0.2f);
    options = 1;

    pCurSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPos, 0.0f);
    if ( !pCurSector )
    {
        return 0;
    }

    pTrackSurface = NULL;
    bExists   = 0;
    sithCollision_SearchForCollisions(pCurSector, NULL, &startPos, &moveNorm, 0.5f, 0.001f, options);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_TRACK) != 0 )
            {
                bExists   = 1;
                pTrackSurface = pCollision->pSurfaceCollided;
                break;
            }
        }
        else if ( (pCollision->type & SITHCOLLISION_ADJOINCROSS) != 0 )
        {
            if ( pCollision->pSurfaceCollided && (pCollision->pSurfaceCollided->flags & SITH_SURFACE_TRACK) != 0 )
            {
                bExists   = 1;
                pTrackSurface = pCollision->pSurfaceCollided;
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    if ( bExists && ppOutFoundFace )
    {
        SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;  // unreferenced; the debug build's frame keeps its slot
        if ( pPrevFace && pPrevFace == &pTrackSurface->face )
        {
#line 4323
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Holy Smokes! CheckForPointOnTrack found prev face.\n", 0, 0, 0, 0);
        }

        *ppOutFoundFace = &pTrackSurface->face;
        if ( bUpdateState )
        {
            if ( pCollision->pSurfaceCollided->pAdjoin )
            {
                pState->bOnAdjoinTrack = 1;
            }
            else
            {
                pState->bOnAdjoinTrack = 0;
            }
        }
    }

    return bExists;
}

void J3DAPI sithPhysics_MoveToTrackFaceFront(SithThing* pThing, SithPhysicsTrackFaceData* pData, float* pSecDeltaTime, rdVector3* pStartPos, rdVector3* pOutPos, float speed)
{
    float curratio;
    float moveDist;
    float overshoot;

    moveDist  = speed * *pSecDeltaTime;
    overshoot = moveDist - pData->distanceToFront;
    if ( overshoot <= 0.0f )
    {
        rdVector_ScaleAdd3(pOutPos, &pData->forward, overshoot, &pData->front);
        *pSecDeltaTime = 0.0f;
    }
    else
    {
        rdVector_Copy3(pOutPos, &pData->front);
        curratio = D3DVAL(pData->distanceToFront / moveDist);
        *pSecDeltaTime = *pSecDeltaTime - curratio * *pSecDeltaTime;
    }
}

// function might check for jeep surface and thing face collision
static int J3DAPI sithPhysics_UpdateJeepWheelContact(SithThing* pThing, SithJeepWheel* pWheel, const rdVector3* pPos)
{
    int bFound = 0;
    int curSearchFlag;
    SithCollision* pCollision;
    SithCollision* pCurCollision;
    SithCollision* pNextHit;
    SithSector* pSector;
    float searchDist;
    float curHitLen;
    rdVector3 startPoint;
    rdVector3 moveNorm;
    static float sithPhysics_flt_538CFC = 0.2f;
    static float sithPhysics_flt_538D00 = 0.0025f;
    static float sithPhysics_flt_538D04 = 0.6f;
    static float sithPhysics_flt_538D08 = 1.0f;
    static float sithPhysics_flt_538D0C = 1.2f;

    searchDist = RD_MULTIPLY(sithPhysics_flt_538CFC, 2.0f);
    pWheel->contactState = 0;
    rdVector_Copy3(&pWheel->unknown3, pPos);
    rdVector_Copy3(&pWheel->impulse, pPos);
    pWheel->pMaterial = NULL;

    rdVector_ScaleAdd3(&startPoint, &pThing->orient.uvec, sithPhysics_flt_538CFC, pPos);
    pSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &startPoint, 0.001f);
    if ( !pSector )
    {
        return 0;
    }

    curSearchFlag     = 0;
    pCurCollision = NULL;
    pNextHit  = NULL;
    rdVector_Neg3(&moveNorm, &pThing->orient.uvec);
    sithCollision_SearchForCollisions(pSector, NULL, &startPoint, &moveNorm, searchDist, pWheel->unknown13, curSearchFlag);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 && (pCollision->type & SITHCOLLISION_FACE) != 0 )
        {
            if ( !bFound )
            {
                pCurCollision = pCollision;
                bFound = 1;
            }
            else
            {
                pNextHit = pCollision;
                break;
            }
        }
    }

    if ( bFound )
    {
        pWheel->pMaterial = pCurCollision->pSurfaceCollided->face.pMaterial;
        if ( pCurCollision->pSurfaceCollided->face.normal.z >= 0.0f )
        {
            curHitLen = -(sithPhysics_flt_538CFC - pCurCollision->distance - pWheel->unknown13);
            pWheel->unknown1 = curHitLen;
            if ( curHitLen > sithPhysics_flt_538D00 )
            {
                pWheel->contactState = 2;
            }
            else if ( pCurCollision->pSurfaceCollided->face.normal.z >= sithPhysics_flt_538D04 )
            {
                if ( SITHPHYSICS_ABS(curHitLen) < sithPhysics_flt_538D00 )
                {
                    pWheel->contactState = 1;
                    rdVector_ScaleAdd3(&pWheel->impulse, &pThing->orient.uvec, -curHitLen, &pWheel->impulse);
                    rdVector_Copy3(&pWheel->contactNormal, &pCurCollision->pSurfaceCollided->face.normal);
                }
                else
                {
                    pWheel->contactState = 3;
                    rdVector_ScaleAdd3(&pWheel->impulse, &pThing->orient.uvec, -curHitLen, &pWheel->impulse);
                    rdVector_Copy3(&pWheel->contactNormal, &pCurCollision->pSurfaceCollided->face.normal);
                }
            }
            else
            {
                pWheel->contactState = 4;
                rdVector_Copy3(&pWheel->contactNormal, &pCurCollision->pSurfaceCollided->face.normal);
                if ( curHitLen < 0.0f )
                {
                    curHitLen = RD_MULTIPLY(curHitLen, sithPhysics_flt_538D08);
                    rdVector_ScaleAdd3(&pWheel->impulse, &pThing->orient.uvec, -curHitLen, &pWheel->impulse);
                }

                if ( pNextHit )
                {
                    pWheel->maxImpulse = -(sithPhysics_flt_538CFC - pNextHit->distance - pWheel->unknown13);
                }
                else if ( (pThing->attach.flags & SITH_ATTACH_SURFACE) != 0 )
                {
                    pWheel->maxImpulse = rdMath_DistancePointToPlane(&pWheel->impulse, &pThing->attach.pFace->normal, &pThing->attach.attachedFaceFirstVert);
                    pWheel->maxImpulse = RD_MULTIPLY(pWheel->maxImpulse, sithPhysics_flt_538D0C);
                }
                else
                {
                    pWheel->maxImpulse = FLT_MAX;
                }
            }
        }
        else
        {
            pWheel->contactState = 2;
            pWheel->unknown1 = FLT_MAX;
        }
    }
    else
    {
        pWheel->contactState = 2;
        pWheel->unknown1 = FLT_MAX;
    }

    sithCollision_DecreaseStackLevel();
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_9, 8) /* end count-fit */
static int J3DAPI sithPhysics_UpdateJeepWheels(SithThing* pThing, SithJeepState* pState, float secDeltaTime)
{
    float scale;
    float scale2;
    rdVector3 pos;
    rdVector3 dir;
    rdVector3 side;
    rdVector3 push;
    rdVector3 wheelSide;
    rdVector3 wheelTorque;
    float dist;
    float dot;
    float halfX;
    float halfY;
    float halfZ;
    float step;
    float maxImpulse;
    float minImpact;
    int bPrevWheelsLoose;
    int numFree;
    int numContact;
    size_t i;
    SithJeepWheel* pWheel;
    static float sithPhysics_flt_538D10 = 0.2f;
    static float sithPhysics_flt_538D14 = 1.1f;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    static float sithPhysics_flt_538D18 = 1.0f;
    static float sithPhysics_flt_538D1C = 0.7f;
    static float sithPhysics_flt_538D20 = -0.2f;

    pState->numTorqueWheels = 0;
    memset(&pState->torque, 0, sizeof(rdVector3));

    halfX = RD_MULTIPLY(pState->wheelOffset.x, 0.5f);
    halfY = RD_MULTIPLY(pState->wheelOffset.y, 0.5f);
    halfZ = RD_MULTIPLY(pState->wheelOffset.z, 0.5f);

    rdVector_Set3(&pos, D3DVAL(-halfX), halfY, -halfZ); /* spelling-fit */
    rdMatrix_TransformPoint34Acc(&pos, &pThing->orient);
    rdVector_Add3(&pos, &pos, &pThing->pos);
    sithPhysics_UpdateJeepWheelContact(pThing, &pState->aWheels[0], &pos);

    rdVector_Set3(&pos, halfX, halfY, -halfZ);
    rdMatrix_TransformPoint34Acc(&pos, &pThing->orient);
    rdVector_Add3(&pos, &pos, &pThing->pos);
    sithPhysics_UpdateJeepWheelContact(pThing, &pState->aWheels[3], &pos);

    rdVector_Set3(&pos, -halfX, -halfY, -halfZ);
    rdMatrix_TransformPoint34Acc(&pos, &pThing->orient);
    rdVector_Add3(&pos, &pos, &pThing->pos);
    sithPhysics_UpdateJeepWheelContact(pThing, &pState->aWheels[1], &pos);

    rdVector_Set3(&pos, halfX, -halfY, -halfZ);
    rdMatrix_TransformPoint34Acc(&pos, &pThing->orient);
    rdVector_Add3(&pos, &pos, &pThing->pos);
    sithPhysics_UpdateJeepWheelContact(pThing, &pState->aWheels[2], &pos);

    bPrevWheelsLoose = pState->bWheelsLoose;
    numFree    = 0;
    numContact = 0;
    for ( i = 0; i < 4; i++ )
    {
        if ( pState->aWheels[i].contactState == 2 )
        {
            numFree++;
        }
        else if ( pState->aWheels[i].contactState == 4 )
        {
            numContact++;
        }
    }

    if ( numFree == 4 )
    {
        pState->bWheelsLoose = 1;
    }
    else if ( numFree == 3 && numContact == 1 )
    {
        pState->bWheelsLoose = 1;
    }
    else if ( numFree == 2 && numContact == 2 )
    {
        pState->bWheelsLoose = 1;
    }
    else
    {
        pState->bWheelsLoose = 0;
    }

    if ( !bPrevWheelsLoose && pState->bWheelsLoose )
    {
        pState->secWheelsLooseTime = 0.0f;
    }
    else if ( bPrevWheelsLoose && pState->bWheelsLoose )
    {
        pState->secWheelsLooseTime += secDeltaTime;
    }

    step       = RD_MULTIPLY(sithPhysics_flt_538D10, secDeltaTime);
    maxImpulse = RD_MULTIPLY(sithPhysics_flt_538D14, secDeltaTime);
    pPhysics->thrust.z = 0.0f;

    if ( !pState->bWheelsLoose )
    {
        minImpact = 1.0f;

        if ( pState->aWheels[3].contactState == 2 && pState->aWheels[0].contactState == 2 )
        {
            scale = J3DMIN(step, pState->aWheels[3].unknown1);
            scale = RD_MULTIPLY(scale, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[3].impulse, &pThing->orient.uvec, -scale);
            pState->aWheels[3].unknown1 -= scale;

            scale2 = J3DMIN(step, pState->aWheels[0].unknown1);
            scale2 = RD_MULTIPLY(scale2, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[0].impulse, &pThing->orient.uvec, -scale2);
            pState->aWheels[0].unknown1 -= scale2;
        }

        if ( pState->aWheels[2].contactState == 2 && pState->aWheels[1].contactState == 2 )
        {
            scale = J3DMIN(step, pState->aWheels[2].unknown1);
            scale = RD_MULTIPLY(scale, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[2].impulse, &pThing->orient.uvec, -scale);
            pState->aWheels[2].unknown1 -= scale;

            scale2 = J3DMIN(step, pState->aWheels[1].unknown1);
            scale2 = RD_MULTIPLY(scale2, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[1].impulse, &pThing->orient.uvec, -scale2);
            pState->aWheels[1].unknown1 -= scale2;
        }

        if ( pState->aWheels[0].contactState == 2 && pState->aWheels[1].contactState == 2 )
        {
            scale = J3DMIN(step, pState->aWheels[0].unknown1);
            scale = RD_MULTIPLY(scale, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[0].impulse, &pThing->orient.uvec, -scale);
            pState->aWheels[0].unknown1 -= scale;

            scale2 = J3DMIN(step, pState->aWheels[1].unknown1);
            scale2 = RD_MULTIPLY(scale2, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[1].impulse, &pThing->orient.uvec, -scale2);
            pState->aWheels[1].unknown1 -= scale2;
        }

        if ( pState->aWheels[3].contactState == 2 && pState->aWheels[2].contactState == 2 )
        {
            scale = J3DMIN(step, pState->aWheels[3].unknown1);
            scale = RD_MULTIPLY(scale, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[3].impulse, &pThing->orient.uvec, -scale);
            pState->aWheels[3].unknown1 -= scale;

            scale2 = J3DMIN(step, pState->aWheels[2].unknown1);
            scale2 = RD_MULTIPLY(scale2, SITHPHYSICS_ABS(pThing->orient.uvec.z));
            rdVector_ScaleAdd3Acc(&pState->aWheels[2].impulse, &pThing->orient.uvec, -scale2);
            pState->aWheels[2].unknown1 -= scale2;
        }

        pWheel = &pState->aWheels[3];
        if ( pWheel->contactState == 4 )
        {
            rdVector_Cross3(&side, &rdroid_zVector3, &pWheel->contactNormal);
            rdVector_Cross3(&push, &side, &pWheel->contactNormal);
            scale = RD_MULTIPLY(-push.z, maxImpulse);
            scale = J3DMIN(scale, pWheel->maxImpulse);
            rdVector_ScaleAdd3Acc(&pWheel->impulse, &push, scale);

            dist = rdMath_DistancePointToPlane(&pWheel->contactNormal, &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&dir, &pThing->orient.uvec, -dist, &pWheel->contactNormal);
            rdVector_Normalize3Acc(&dir);

            dot = rdVector_Dot3(&pPhysics->velocity, &dir);
            if ( dot < 0.0f )
            {
                dot = RD_MULTIPLY(sithPhysics_flt_538D1C - pWheel->contactNormal.z, dot);
                dot = RD_MULTIPLY(dot, sithPhysics_flt_538D18);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &dir, -dot);
                if ( dot < minImpact )
                {
                    minImpact = dot;
                }
            }
        }

        pWheel = &pState->aWheels[0];
        if ( pWheel->contactState == 4 )
        {
            rdVector_Cross3(&side, &rdroid_zVector3, &pWheel->contactNormal);
            rdVector_Cross3(&push, &side, &pWheel->contactNormal);
            scale = RD_MULTIPLY(-push.z, maxImpulse);
            scale = J3DMIN(scale, pWheel->maxImpulse);
            rdVector_ScaleAdd3Acc(&pWheel->impulse, &push, scale);

            dist = rdMath_DistancePointToPlane(&pWheel->contactNormal, &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&dir, &pThing->orient.uvec, -dist, &pWheel->contactNormal);
            rdVector_Normalize3Acc(&dir);

            dot = rdVector_Dot3(&pPhysics->velocity, &dir);
            if ( dot < 0.0f )
            {
                dot = RD_MULTIPLY(sithPhysics_flt_538D1C - pWheel->contactNormal.z, dot);
                dot = RD_MULTIPLY(dot, sithPhysics_flt_538D18);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &dir, -dot);
                if ( dot < minImpact )
                {
                    minImpact = dot;
                }
            }
        }

        pWheel = &pState->aWheels[1];
        if ( pWheel->contactState == 4 )
        {
            rdVector_Cross3(&side, &rdroid_zVector3, &pWheel->contactNormal);
            rdVector_Cross3(&push, &side, &pWheel->contactNormal);
            scale = RD_MULTIPLY(-push.z, maxImpulse);
            scale = J3DMIN(scale, pWheel->maxImpulse);
            rdVector_ScaleAdd3Acc(&pWheel->impulse, &push, scale);

            dist = rdMath_DistancePointToPlane(&pWheel->contactNormal, &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&dir, &pThing->orient.uvec, -dist, &pWheel->contactNormal);
            rdVector_Normalize3Acc(&dir);

            dot = rdVector_Dot3(&pPhysics->velocity, &dir);
            if ( dot < 0.0f )
            {
                dot = RD_MULTIPLY(sithPhysics_flt_538D1C - pWheel->contactNormal.z, dot);
                dot = RD_MULTIPLY(dot, sithPhysics_flt_538D18);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &dir, -dot);
                if ( dot < minImpact )
                {
                    minImpact = dot;
                }
            }
        }

        pWheel = &pState->aWheels[2];
        if ( pWheel->contactState == 4 )
        {
            rdVector_Cross3(&side, &rdroid_zVector3, &pWheel->contactNormal);
            rdVector_Cross3(&push, &side, &pWheel->contactNormal);
            scale = RD_MULTIPLY(-push.z, maxImpulse);
            scale = J3DMIN(scale, pWheel->maxImpulse);
            rdVector_ScaleAdd3Acc(&pWheel->impulse, &push, scale);

            dist = rdMath_DistancePointToPlane(&pWheel->contactNormal, &pThing->orient.uvec, &rdroid_zeroVector3);
            rdVector_ScaleAdd3(&dir, &pThing->orient.uvec, -dist, &pWheel->contactNormal);
            rdVector_Normalize3Acc(&dir);

            dot = rdVector_Dot3(&pPhysics->velocity, &dir);
            if ( dot < 0.0f )
            {
                dot = RD_MULTIPLY(sithPhysics_flt_538D1C - pWheel->contactNormal.z, dot);
                dot = RD_MULTIPLY(dot, sithPhysics_flt_538D18);
                rdVector_ScaleAdd3Acc(&pPhysics->velocity, &dir, -dot);
                if ( dot < minImpact )
                {
                    minImpact = dot;
                }
            }
        }

        if ( minImpact < sithPhysics_flt_538D20 )
        {
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_LANDHARD);
            sithPuppet_PlayMode(pThing, SITHPUPPETSUBMODE_LAND, NULL);
        }
    }

    rdVector_Copy3(&pState->impulse, &pState->aWheels[3].impulse);
    rdVector_Add3Acc(&pState->impulse, &pState->aWheels[0].impulse);
    rdVector_Add3Acc(&pState->impulse, &pState->aWheels[1].impulse);
    rdVector_Add3Acc(&pState->impulse, &pState->aWheels[2].impulse);
    rdVector_Scale3Acc(&pState->impulse, 0.25f);

    pWheel = &pState->aWheels[3];
    if ( pWheel->contactState == 1 || pWheel->contactState == 3 )
    {
        rdVector_Cross3(&wheelSide, &pThing->orient.lvec, &pWheel->contactNormal);
        rdVector_Cross3(&wheelTorque, &pWheel->contactNormal, &wheelSide);
        rdVector_Add3Acc(&pState->torque, &wheelTorque);
        pState->numTorqueWheels++;
    }

    pWheel = &pState->aWheels[0];
    if ( pWheel->contactState == 1 || pWheel->contactState == 3 )
    {
        rdVector_Cross3(&wheelSide, &pThing->orient.lvec, &pWheel->contactNormal);
        rdVector_Cross3(&wheelTorque, &pWheel->contactNormal, &wheelSide);
        rdVector_Add3Acc(&pState->torque, &wheelTorque);
        pState->numTorqueWheels++;
    }

    pWheel = &pState->aWheels[1];
    if ( pWheel->contactState == 1 || pWheel->contactState == 3 )
    {
        rdVector_Cross3(&wheelSide, &pThing->orient.lvec, &pWheel->contactNormal);
        rdVector_Cross3(&wheelTorque, &pWheel->contactNormal, &wheelSide);
        rdVector_Add3Acc(&pState->torque, &wheelTorque);
        pState->numTorqueWheels++;
    }

    pWheel = &pState->aWheels[2];
    if ( pWheel->contactState == 1 || pWheel->contactState == 3 )
    {
        rdVector_Cross3(&wheelSide, &pThing->orient.lvec, &pWheel->contactNormal);
        rdVector_Cross3(&wheelTorque, &pWheel->contactNormal, &wheelSide);
        rdVector_Add3Acc(&pState->torque, &wheelTorque);
        pState->numTorqueWheels++;
    }

    rdVector_Normalize3Acc(&pState->torque);
    return 1;
}

static void J3DAPI sithPhysics_GetJeepChassisNormal(SithThing* pThing, SithJeepState* pState, rdVector3* pOutNormal, float secDeltaTime)
{
    rdVector3 faceNormal;
    float dot;
    float rotScale;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    static float sithPhysics_flt_538D24 = 4e+04f;
    static float sithPhysics_flt_538D28 = 0.1f;

    memset(pOutNormal, 0, sizeof(rdVector3));

    {
        rdVector3 edge1;
        rdVector3 edge2;

        if ( pState->aWheels[3].contactState && pState->aWheels[0].contactState && pState->aWheels[1].contactState )
        {
            rdVector_Sub3(&edge1, &pState->aWheels[0].impulse, &pState->aWheels[3].impulse);
            rdVector_Sub3(&edge2, &pState->aWheels[1].impulse, &pState->aWheels[0].impulse);
            rdVector_Cross3(&faceNormal, &edge1, &edge2);
            rdVector_Normalize3Acc(&faceNormal);
            rdVector_Add3Acc(pOutNormal, &faceNormal);
        }

        if ( pState->aWheels[0].contactState && pState->aWheels[1].contactState && pState->aWheels[2].contactState )
        {
            rdVector_Sub3(&edge1, &pState->aWheels[1].impulse, &pState->aWheels[0].impulse);
            rdVector_Sub3(&edge2, &pState->aWheels[2].impulse, &pState->aWheels[1].impulse);
            rdVector_Cross3(&faceNormal, &edge1, &edge2);
            rdVector_Normalize3Acc(&faceNormal);
            rdVector_Add3Acc(pOutNormal, &faceNormal);
        }

        if ( pState->aWheels[1].contactState && pState->aWheels[2].contactState && pState->aWheels[3].contactState )
        {
            rdVector_Sub3(&edge1, &pState->aWheels[2].impulse, &pState->aWheels[1].impulse);
            rdVector_Sub3(&edge2, &pState->aWheels[3].impulse, &pState->aWheels[2].impulse);
            rdVector_Cross3(&faceNormal, &edge1, &edge2);
            rdVector_Normalize3Acc(&faceNormal);
            rdVector_Add3Acc(pOutNormal, &faceNormal);
        }

        if ( pState->aWheels[2].contactState && pState->aWheels[3].contactState && pState->aWheels[0].contactState )
        {
            rdVector_Sub3(&edge1, &pState->aWheels[3].impulse, &pState->aWheels[2].impulse);
            rdVector_Sub3(&edge2, &pState->aWheels[0].impulse, &pState->aWheels[3].impulse);
            rdVector_Cross3(&faceNormal, &edge1, &edge2);
            rdVector_Normalize3Acc(&faceNormal);
            rdVector_Add3Acc(pOutNormal, &faceNormal);
        }
    }

    rdVector_Normalize3Acc(pOutNormal);
    rdVector_ScaleAdd3(&pState->centerOfMass, pOutNormal, RD_MULTIPLY(pState->wheelOffset.z, 0.5f), &pState->impulse);

    rotScale = RD_MULTIPLY(sithPhysics_flt_538D24, secDeltaTime);
    if ( pState->aWheels[3].contactState == 4 || pState->aWheels[0].contactState == 4 || pState->aWheels[1].contactState == 4 || pState->aWheels[2].contactState == 4 )
    {
        rdVector3 rearDelta;
        rdVector3 frontDelta;

        rdVector_Sub3(&rearDelta, &pState->aWheels[3].impulse, &pState->aWheels[2].impulse);
        dot = D3DVAL(rdVector_Dot3(&pThing->orient.rvec, &rearDelta));
        if ( pState->aWheels[3].contactState == 4 && pState->aWheels[2].contactState != 4 )
        {
            dot = RD_MULTIPLY(2.0f - pState->aWheels[3].contactNormal.z, dot);
            dot = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), dot);
            pPhysics->angularVelocity.y -= RD_MULTIPLY(dot, rotScale);
        }
        else if ( pState->aWheels[2].contactState == 4 && pState->aWheels[3].contactState != 4 )
        {
            dot = RD_MULTIPLY(2.0f - pState->aWheels[2].contactNormal.z, dot);
            dot = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), dot);
            pPhysics->angularVelocity.y -= RD_MULTIPLY(dot, rotScale);
        }

        rdVector_Sub3(&frontDelta, &pState->aWheels[0].impulse, &pState->aWheels[1].impulse);
        dot = D3DVAL(rdVector_Dot3(&pThing->orient.rvec, &frontDelta));
        if ( pState->aWheels[0].contactState == 4 && pState->aWheels[1].contactState != 4 )
        {
            dot = RD_MULTIPLY(2.0f - pState->aWheels[0].contactNormal.z, dot);
            dot = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), dot);
            pPhysics->angularVelocity.y -= RD_MULTIPLY(dot, rotScale);
        }
        else if ( pState->aWheels[1].contactState == 4 && pState->aWheels[0].contactState != 4 )
        {
            dot = RD_MULTIPLY(2.0f - pState->aWheels[1].contactNormal.z, dot);
            dot = RD_MULTIPLY(sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)), dot);
            pPhysics->angularVelocity.y -= RD_MULTIPLY(dot, rotScale);
        }
    }

    if ( pState->bWheelsLoose && pThing->attach.flags )
    {
        rdVector3 edge2;
        rdVector3 edge1;
        rdVector3 axis;
        edge2.x = pOutNormal->x;
        edge2.y = pOutNormal->y;
        edge2.z = 0.0f;
        rdVector_Normalize3Acc(&edge2);
        rdVector_Cross3(&edge1, &edge2, pOutNormal);
        rdVector_Cross3(&axis, pOutNormal, &edge1);
        rdVector_Normalize3Acc(&axis);
        rdVector_ScaleAdd3Acc(&pPhysics->velocity, &axis, RD_MULTIPLY(sithPhysics_flt_538D28, secDeltaTime));
    }
}

static void J3DAPI sithPhysics_ResetJeepWheels(SithThing* pThing, SithJeepState* pState)
{
    size_t i;

    pState->wheelOffset.x = 0.14f;
    pState->wheelOffset.y = 0.22f;
    pState->wheelOffset.z = 0.18f;
    for ( i = 0; i < 4; ++i )
    {
        pState->aWheels[i].unknown13 = 0.01f;
    }
}

// Linker-folded in retail onto sithPhysics_InitVehicleFxState (identical body); the debug build keeps this copy after
// ResetJeepWheels and calls it from CreateJeepUserBlock.
static void J3DAPI sithPhysics_InitJeepFxState(SithThing* pThing, SithVehicleEngineFxState* pFxState)
{
    pFxState->secUpdateInterval = 0.05f;
    pFxState->secUpdateTimer    = 0.0f;
}

static signed int J3DAPI sithPhysics_CreateJeepUserBlock(SithThing* pThing)
{
    SithJeepUserBlock* pJeep;

    if ( !pThing->userblock.pJeep )
    {
#line 5168
        pThing->userblock.pJeep = (SithJeepUserBlock*)STDMALLOC(sizeof(SithJeepUserBlock));
        if ( !pThing->userblock.pJeep )
        {
#line 5171
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "_CreateJeepUserBlock failed to allocate.\n", 0, 0, 0, 0);
            return 0;
        }

        pJeep = pThing->userblock.pJeep;
        memset(pJeep, 0, sizeof(SithJeepUserBlock));
        if ( !stdUtil_StrCmp(pThing->aName, "jeepplayer") )
        {
            pJeep->type = 1;
            sithPhysics_ResetJeepWheels(pThing, &pThing->userblock.pJeep->state);
            sithPhysics_InitJeepFxState(pThing, &pThing->userblock.pJeep->fxstate);
            sithPhysics_InitJeepExhaust(pThing, &pThing->userblock.pJeep->exhaustInfo);
        }
        else if ( !stdUtil_StrCmp(pThing->aName, "jeepplayerpyr") )
        {
            pJeep->type = 1;
            sithPhysics_ResetJeepWheels(pThing, &pThing->userblock.pJeep->state);
            sithPhysics_InitJeepFxState(pThing, &pThing->userblock.pJeep->fxstate);
            sithPhysics_InitJeepExhaust(pThing, &pThing->userblock.pJeep->exhaustInfo);
        }
        else
        {
            pJeep->type = 0;
            sithPhysics_ResetJeepWheels(pThing, &pThing->userblock.pJeep->state);
            sithPhysics_InitJeepFxState(pThing, &pThing->userblock.pJeep->fxstate);
        }
    }

    return 1;
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_10, 8) /* end count-fit */
static void J3DAPI sithPhysics_UpdateJeepFx(SithThing* pThing, SithVehicleEngineFxState* pFxState, float secDeltaTime)
{
    SithJeepState* pJeepState;
    SithJeepState* pState;
    float speed;
    float thrustLen;
    int bWheelsLoose;
    int bUpdate;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    static int sithPhysics_dword_538D2C = 1;
    static int sithPhysics_dword_538D30 = 1;
    static int sithPhysics_bJeepMoveFx = 1;
    static int sithPhysics_dword_538D38 = 1;
    rdVector3 moveDir;
    static float sithPhysics_flt_538D3C = 0.5f;
    static float sithPhysics_flt_538D40 = 1.0f;
    static float sithPhysics_flt_538D44 = 1.15f;
    static float sithPhysics_flt_538D48 = 0.1f;
    static float sithPhysics_flt_538D4C = 2.6f;
    static float sithPhysics_flt_538D50 = 3.5f;
    static float sithPhysics_flt_538D54 = 1.0f;
    static float sithPhysics_flt_538D58 = 0.6f;
    static float sithPhysics_flt_538D5C = 0.7f;
    static float sithPhysics_flt_538D60 = 0.9f;
    static float sithPhysics_flt_538D64 = 1.0f;
    static float sithPhysics_flt_538D68 = 1.0f;
    static float sithPhysics_flt_538D6C = 1.3f;
    static float sithPhysics_flt_538D70 = 1.8f;
    static float sithPhysics_flt_538D74 = 2.0f;
    static float sithPhysics_flt_538D78 = 0.15f;
    static float sithPhysics_flt_538D7C = 0.28f;
    static float sithPhysics_flt_538D80 = 0.93f;
    static float sithPhysics_flt_538D84 = -0.9f;
    static float sithPhysics_flt_538D88 = 1.3f;
    static float sithPhysics_flt_538D8C = 1.0f;
    static float sithPhysics_flt_538D90 = 2.0f;
    static float sithPhysics_flt_538D94 = 0.6f;
    static float sithPhysics_flt_538D98 = 0.5f;
    static float sithPhysics_flt_538D9C = 0.5f;
    static float sithPhysics_flt_538DA0 = -0.01f;
    static float sithPhysics_flt_538DA4 = 0.05f;
    static float sithPhysics_flt_538DA8 = 0.65f;

    if ( (pThing->flags & SITH_TF_DYING) != 0 )
    {
        return;
    }

    pJeepState = pState = &pThing->userblock.pJeep->state;
    speed      = rdVector_Normalize3(&moveDir, &pPhysics->velocity);
    thrustLen  = sqrtf(rdVector_Dot3(&pPhysics->thrust, &pPhysics->thrust));

    bWheelsLoose = 0;
    if ( pState->bWheelsLoose )
    {
        bWheelsLoose = 1;
    }

    bUpdate = 0;
    pFxState->secUpdateTimer -= secDeltaTime;
    if ( pFxState->secUpdateTimer < 0.0f )
    {
        bUpdate = 1;
        pFxState->secUpdateTimer = pFxState->secUpdateInterval;
    }

    if ( sithPhysics_dword_538D2C )
    {
        if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING || pThing->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING || pThing->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING )
        {
            sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
        }
        else
        {
            sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_LWALKMETAL, sithPhysics_flt_538D48);
            if ( bUpdate )
            {
                float rate;
                float range;
                float scale;
                float minVolume;
                float maxVolume;
                float minPitch;
                float maxPitch;
                float volume;
                float pitch;
                int engineMode;

                if ( speed >= sithPhysics_flt_538D54 && sithControl_GetKey(SITHCONTROL_ACT1, NULL) )
                {
                    engineMode = 2;
                    rate       = speed - sithPhysics_flt_538D54;
                    range      = 2.0f - sithPhysics_flt_538D54;
                    minPitch   = sithPhysics_flt_538D6C;
                    maxPitch   = sithPhysics_flt_538D74;
                    minVolume  = sithPhysics_flt_538D5C;
                    maxVolume  = sithPhysics_flt_538D64;
                }
                else
                {
                    engineMode = 1;
                    rate       = speed;
                    range      = sithPhysics_flt_538D54;
                    minPitch   = sithPhysics_flt_538D68;
                    maxPitch   = sithPhysics_flt_538D70;
                    minVolume  = sithPhysics_flt_538D58;
                    maxVolume  = sithPhysics_flt_538D60;
                }

                if ( sithPhysics_dword_58540C != engineMode )
                {
                    sithPhysics_dword_58540C = engineMode;
                }

                scale  = D3DVAL((maxVolume - minVolume) / range);
                volume = RD_MULTIPLY(rate, scale) + minVolume;
                volume = STDMATH_CLAMP(volume, minVolume, maxVolume);
                sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_LWALKMETAL, volume);

                scale = D3DVAL((maxPitch - minPitch) / range);
                pitch = RD_MULTIPLY(rate, scale) + minPitch;
                pitch = STDMATH_CLAMP(pitch, minPitch, maxPitch);
                if ( pState->bWheelsLoose )
                {
                    pitch = RD_MULTIPLY(pState->secWheelsLooseTime, sithPhysics_flt_538D50) + pitch;
                    pitch = STDMATH_CLAMP(pitch, sithPhysics_flt_538D40, sithPhysics_flt_538D4C);
                }

                sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_LWALKMETAL, pitch);
            }
        }
    }
    else
    {
        sithSoundClass_StopMode(pThing, SITHSOUNDCLASS_LWALKMETAL);
    }

    if ( (sithPhysics_dword_538D30 || sithPhysics_bJeepMoveFx) && !bWheelsLoose && speed > sithPhysics_flt_538D3C )
    {
        sithSoundClass_PlayModeFirstEx(pThing, SITHSOUNDCLASS_RWALKMETAL, sithPhysics_flt_538D78);
        if ( bUpdate )
        {
            float volume;
            float dot;
            float pitch;

            dot   = rdVector_Dot3(&pThing->orient.lvec, &moveDir);
            pitch = 1.0f;
            if ( sithPhysics_bJeepMoveFx && thrustLen == 0.0f )
            {
                volume = sithPhysics_flt_538D8C;
                pitch  = sithPhysics_flt_538D88;
            }
            else if ( sithPhysics_bJeepMoveFx && dot <= sithPhysics_flt_538D80 && dot >= sithPhysics_flt_538D84 )
            {
                volume = RD_MULTIPLY(speed, sithPhysics_flt_538D7C) + sithPhysics_flt_538D78;
                volume = RD_MULTIPLY(1.0f - dot, sithPhysics_flt_538D90) + volume;
            }
            else if ( sithPhysics_dword_538D30 )
            {
                volume = RD_MULTIPLY(speed, sithPhysics_flt_538D7C) + sithPhysics_flt_538D78;
            }
            else
            {
                volume = sithPhysics_flt_538D78;
            }

            sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, volume);
            sithSoundClass_SetModePitch(pThing, SITHSOUNDCLASS_RWALKMETAL, pitch);
        }
    }
    else if ( !sithSoundClass_IsModeFadingVol(pThing, SITHSOUNDCLASS_RWALKMETAL) )
    {
        sithSoundClass_FadeModeVolume(pThing, SITHSOUNDCLASS_RWALKMETAL, 0.0f, sithPhysics_flt_538D94);
    }

    if ( sithPhysics_dword_538D38 && !bWheelsLoose )
    {
        int i;
        float chance;
        int bSkidding;
        float volume;

        chance    = RD_MULTIPLY(speed, sithPhysics_flt_538D9C);
        bSkidding = 0;
        for ( i = 0; i < 4; i++ )
        {
            if ( pJeepState->aWheels[i].unknown1 < sithPhysics_flt_538DA0 )
            {
                bSkidding = 1;
                break;
            }
        }

        if ( bSkidding && speed > sithPhysics_flt_538D98 && SITH_RAND() < chance )
        {
            volume = RD_MULTIPLY(speed - sithPhysics_flt_538D98, sithPhysics_flt_538DA8) + sithPhysics_flt_538DA4;
            sithSoundClass_PlayModeRandom(pThing, SITHSOUNDCLASS_RWALKHARD);
            sithSoundClass_SetModeVolume(pThing, SITHSOUNDCLASS_RWALKHARD, volume);
        }
    }
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(sithPhysics_countfit_11, 30, 3) CF_PROTO(sithPhysics_countfit_12, 10) /* end count-fit */
void J3DAPI sithPhysics_UpdateJeepPhysics(SithThing* pThing, float secDeltaTime)
{
    SithJeepState* pState;
    float moveFactor;
    rdVector3 accel;
    SithPhysicsInfo* pPhysics = &pThing->moveInfo.physics;
    rdFace* pAttachedFace = pThing->attach.pFace;  // unreferenced; the debug build's frame keeps its slot
    float dragFactor = 1.0f;
    float slopeDot = 1.0f;
    static float sithPhysics_flt_538DAC = 0.997f;
    static float sithPhysics_flt_538DB0 = 1.1f;
    static float sithPhysics_flt_538DB4 = -1.65f;
    static float sithPhysics_flt_538DB8 = 0.3f;
    static float sithPhysics_flt_538DBC = 0.005f;
    static float sithPhysics_flt_538DC0 = 2.0f;
    static float sithPhysics_flt_538DC4 = 0.86f;
    static float sithPhysics_flt_538DC8 = 0.05f;
    static float sithPhysics_flt_538DCC = 0.02f;
    static float sithPhysics_flt_538DD0 = 1.0f;
    static float sithPhysics_flt_538DD4 = 5e+01f;
    static float sithPhysics_flt_538DD8 = 1.4e+02f;
    static float sithPhysics_flt_538DDC = 0.85f;
    static float sithPhysics_flt_538DE0 = 0.6f;
    static float sithPhysics_flt_538DE4 = 0.6946f;
    static float sithPhysics_flt_538DE8 = 0.5f;
    static float sithPhysics_flt_538DEC = 0.927f;
    static float sithPhysics_flt_538DF0 = 2.0f;
    static float sithPhysics_flt_538DF4 = 2.5f;
    static float sithPhysics_flt_538DF8 = 1.25f;
    static float sithPhysics_flt_538DFC = 0.6f;
    rdVector3 normal;
    static float sithPhysics_flt_538E00 = 1.0f;
    static float sithPhysics_flt_538E04 = 0.7f;
    static float sithPhysics_flt_538E08 = 2.2f;
    static float sithPhysics_flt_538E0C = 1.7f;
    static float sithPhysics_flt_538E10 = 1.0f;

    if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_BOARDING || pThing->moveStatus == SITHPLAYERMOVE_JEEP_UNBOARDING )
    {
        return;
    }

    if ( !sithPhysics_CreateJeepUserBlock(pThing) )
    {
        return;
    }

    pState = &pThing->userblock.pJeep->state;
    memset(&accel, 0, sizeof(rdVector3));
    pPhysics->flags &= ~SITH_PF_UNKNOWN_200000;
    pState->bSkidding = 0;

    moveFactor = 1.0f;
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity)
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_82
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83 )
    {
        /* count-fit: placeholder declarations, not reconstructed source */ CF_EXTERNS(sithPhysics_countfit_13, 24) /* end count-fit */ rdVector3 dir;
        float dot;

        rdVector_Normalize3(&dir, &pPhysics->velocity);
        dot = rdVector_Dot3(&pThing->orient.uvec, &dir);
        rdVector_ScaleAdd3Acc(&dir, &pThing->orient.uvec, (-dot));
        rdVector_Normalize3Acc(&dir);

        moveFactor = rdVector_Dot3(&pThing->orient.lvec, &dir);
        moveFactor = SITHPHYSICS_ABS(moveFactor);
        moveFactor = STDMATH_CLAMP(moveFactor, 0.5f, 1.0f);
        if ( moveFactor < sithPhysics_flt_538DAC && sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) > sithPhysics_flt_538DB0 )
        {
            pState->bSkidding = 1;
        }
    }

    if ( pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83 && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_84 && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
    {
        sithPhysics_UpdateJeepWheels(pThing, pState, secDeltaTime);
        sithPhysics_GetJeepChassisNormal(pThing, pState, &normal, secDeltaTime);
        if ( pState->bWheelsLoose )
        {
            pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_82;
        }
        else
        {
            pThing->moveStatus = SITHPLAYERMOVE_JEEP_STILL;
            if ( pThing->moveInfo.physics.velocity.z < sithPhysics_flt_538DB4 )
            {
#ifdef J3D_DEBUG
                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
                {
                    char aText[256];
                    sprintf(aText, "Falling Death Tire %f.", pThing->moveInfo.physics.velocity.z);
                    sithConsole_PrintString(aText);
                }
#endif
                if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
                {
                    pThing->moveStatus = SITHPLAYERMOVE_JEEP_IMPACT;
                    sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
                }
            }
        }
    }

    if ( SITHPHYSICS_ISZERO3(&normal) )
    {
        rdVector_Copy3(&normal, &rdroid_zVector3);
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_STILL )
    {
        sithPhysics_SetThingLook(pThing, &normal, pPhysics->orientSpeed * secDeltaTime);
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_83 )
    {
        /* count-fit: placeholder declarations, not reconstructed source */ CF_EXTERNS(sithPhysics_countfit_14, 24) /* end count-fit */ rdVector3 dir;
        float dot;

        rdVector_ScaleAdd3Acc(&pPhysics->velocity, &pThing->orient.uvec, RD_MULTIPLY(sithPhysics_flt_538DC0, secDeltaTime));
        rdVector_Neg3(&dir, &rdroid_zVector3);
        sithPhysics_SetThingLook(pThing, &dir, sithPhysics_flt_538DB8 * secDeltaTime);

        dot = 1.0f - rdVector_Dot3(&pThing->orient.uvec, &dir);
        if ( SITHPHYSICS_ABS(dot) < sithPhysics_flt_538DBC )
        {
            pThing->moveStatus = SITHPLAYERMOVE_JEEP_IMPACT;
            sithActor_KillActor(pThing, pThing, SITH_DAMAGE_IMPACT);
        }
    }

    if ( pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83 && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_84 && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
    {
        float rollDot = SITHPHYSICS_ABS(rdVector_Dot3(&pThing->orient.rvec, &rdroid_zVector3));
        if ( rollDot > sithPhysics_flt_538DC4 || pThing->orient.uvec.z <= sithPhysics_flt_538DC8 )
        {
#ifdef J3D_DEBUG
            if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) != 0 )
            {
                char aText[256];
                sprintf(aText, "Rollover Death.");
                sithConsole_PrintString(aText);
            }
#endif
            if ( (pThing->thingInfo.actorInfo.flags & SITH_AF_INVULNERABLE) == 0 )
            {
                pThing->moveStatus = SITHPLAYERMOVE_UNKNOWN_83;
                pThing->collide.movesize = sithPhysics_flt_538DCC;
                memset(&pPhysics->rotThrust, 0, sizeof(rdVector3));
                memset(&pPhysics->thrust, 0, sizeof(rdVector3));
            }
        }
    }

    if ( (pPhysics->flags & SITH_PF_USEANGULARTHRUST) != 0 && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_82 )
    {
        if ( !SITHPHYSICS_ISZERO3(&pPhysics->angularVelocity) )
        {
            sithPhysics_ApplyDrag(&pPhysics->angularVelocity, pPhysics->surfDrag, 0.0f, secDeltaTime);
        }

        pPhysics->angularVelocity.y += RD_MULTIPLY(pPhysics->rotThrust.y, secDeltaTime);
        rdMath_ClampVector3Acc(&pPhysics->angularVelocity, -pPhysics->maxRotationVelocity, pPhysics->maxRotationVelocity);
        SITHPHYSICS_CLIPPRECISION3(&pPhysics->angularVelocity);
    }

    if ( pPhysics->angularVelocity.y != 0.0f )
    {
        rdVector3 dir;
        rdMatrix34 rotMat;
        float speed;
        float rotLimit;
        float rangeMin;
        float rangeMax;
        float outStart;
        float outEnd;
        float range;
        float slope;

        rdVector_Scale3(&dir, &pPhysics->angularVelocity, secDeltaTime);
        rdMatrix_BuildRotate34(&rotMat, &dir);
        sithCollision_RotateThing(pThing, &rotMat);

        if ( !pState->bSkidding && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_82 && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83 )
        {
            speed = sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity));
            if ( speed > sithPhysics_flt_538DDC )
            {
                if ( speed > pPhysics->maxVelocity )
                {
                    outEnd   = sithPhysics_flt_538DD0;
                    outStart = sithPhysics_flt_538DD4;
                    rangeMin = pPhysics->maxVelocity;
                    range    = 2.0f - pPhysics->maxVelocity;
                }
                else
                {
                    outEnd   = sithPhysics_flt_538DD4;
                    outStart = sithPhysics_flt_538DD8;
                    rangeMin = 0.0f;
                    range    = pPhysics->maxVelocity - sithPhysics_flt_538DDC;
                }

                rangeMax = speed - rangeMin;
                rangeMax = STDMATH_CLAMP(rangeMax, 0.0f, range);
                slope    = D3DVAL((outEnd - outStart) / range);
                rotLimit = RD_MULTIPLY(slope, rangeMax) + outStart;
            }
            else
            {
                rotLimit = RD_MULTIPLY(sithPhysics_flt_538DD8, 2.0f);
            }

            rotLimit = RD_MULTIPLY(rotLimit, secDeltaTime);
            dir.x = STDMATH_CLAMP(dir.x, (-rotLimit), rotLimit);
            dir.y = STDMATH_CLAMP(dir.y, (-rotLimit), rotLimit);
            dir.z = STDMATH_CLAMP(dir.z, (-rotLimit), rotLimit);
            rdMatrix_BuildRotate34(&rotMat, &dir);
            rdMatrix_TransformVector34Acc(&pPhysics->velocity, &rotMat);
        }

        rdMatrix_Normalize34(&pThing->orient);
    }

    dragFactor = STDMATH_CLAMP(dragFactor, 0.25f, 1.0f);
    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) && pPhysics->surfDrag != 0.0f )
    {
        if ( (pPhysics->flags & SITH_PF_UNKNOWN_8000) != 0 )
        {
            pPhysics->flags &= ~SITH_PF_UNKNOWN_8000;
        }
        else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_82 )
        {
            sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(sithPhysics_flt_538DE0, dragFactor), 0.0f, secDeltaTime);
        }
        else if ( SITHPHYSICS_ISZERO3(&pPhysics->thrust) )
        {
            sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(pPhysics->surfDrag, dragFactor), RD_MULTIPLY(pPhysics->staticDrag, dragFactor), secDeltaTime);
        }
        else
        {
            float drag = pPhysics->surfDrag;
            sithPhysics_ApplyDrag(&pPhysics->velocity, RD_MULTIPLY(drag, dragFactor), 0.0f, secDeltaTime);
        }
    }

    if ( (pPhysics->flags & SITH_PF_USETHRUST) != 0
        && !SITHPHYSICS_ISZERO3(&pPhysics->thrust)
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_82
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83
        && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_84
        && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
    {
        float thrustScale;
        rdVector3 torqueThrust;
        rdVector3 dir;
        rdVector3 sum;

        thrustScale = RD_MULTIPLY(secDeltaTime, dragFactor);
        thrustScale = RD_MULTIPLY(thrustScale, moveFactor);
        rdVector_Scale3(&torqueThrust, &pState->torque, pPhysics->thrust.y);
        dir.x = pPhysics->thrust.x;
        dir.y = 0.0f;
        dir.z = pPhysics->thrust.z;
        if ( !SITHPHYSICS_ISZERO3(&dir) )
        {
            rdMatrix_TransformVector34Acc(&dir, &pThing->orient);
        }

        rdVector_Add3(&sum, &torqueThrust, &dir);
        rdVector_Scale3(&accel, &sum, thrustScale);
        SITHPHYSICS_CLIPPRECISION3(&accel);
    }

    if ( pThing->moveStatus == SITHPLAYERMOVE_JEEP_STILL )
    {
        slopeDot = rdVector_Dot3(&normal, &rdroid_zVector3);
        if ( slopeDot < sithPhysics_flt_538DEC )
        {
            rdVector3 cross;
            float dirDot;
            float fwdSpeed;
            float fwdDirSpeed;
            rdVector3 dir;
            float turn;
            float scale;
            float amount;

            rdVector_Cross3(&cross, &normal, &rdroid_zVector3);
            rdVector_Cross3(&dir, &normal, &cross);
            rdVector_Normalize3Acc(&dir);

            if ( slopeDot < sithPhysics_flt_538DE4 )
            {
                float pushRate;
                float accelRate;
                float accelScale;

                pushRate   = D3DVAL((sithPhysics_flt_538E00 - sithPhysics_flt_538DFC) / (sithPhysics_flt_538DE4 - sithPhysics_flt_538DE8));
                accelRate  = D3DVAL((sithPhysics_flt_585410 - sithPhysics_flt_538E04) / (sithPhysics_flt_538DE4 - sithPhysics_flt_538DE8));
                accelScale = RD_MULTIPLY(sithPhysics_flt_538DE4 - slopeDot, accelRate) + sithPhysics_flt_538E04;
                accelScale = STDMATH_CLAMP(accelScale, sithPhysics_flt_538DFC, sithPhysics_flt_538E00);
                rdVector_Scale3Acc(&accel, accelScale);

                scale  = RD_MULTIPLY(sithPhysics_flt_538DE4 - slopeDot, pushRate) + sithPhysics_flt_538DFC;
                scale  = STDMATH_CLAMP(scale, sithPhysics_flt_538DFC, sithPhysics_flt_538E00);
                amount = RD_MULTIPLY(scale, secDeltaTime);
                rdVector_ScaleAdd3Acc(&accel, &dir, amount);
            }

            fwdSpeed    = stdMath_ClipNearZero(rdVector_Dot3(&pThing->orient.lvec, &pPhysics->velocity));
            dirDot      = stdMath_ClipNearZero(rdVector_Dot3(&pThing->orient.lvec, &dir));
            fwdDirSpeed = stdMath_ClipNearZero(fwdSpeed * dirDot);

            if ( pPhysics->thrust.y == 0.0f || fwdDirSpeed == 0.0f )
            {
                turn = 0.0f;
            }
            else if ( fwdDirSpeed > 0.0f )
            {
                turn = sithPhysics_flt_538DF0;
            }
            else if ( fwdDirSpeed < 0.0f )
            {
                turn = J3DMIN(sithPhysics_flt_538DF4, -(fwdDirSpeed / secDeltaTime));
            }

            scale  = RD_MULTIPLY(sithPhysics_flt_538DF8 - slopeDot, turn);
            scale  = RD_MULTIPLY(scale, secDeltaTime);
            amount = RD_MULTIPLY(scale, dirDot);
            rdVector_ScaleAdd3Acc(&accel, &pThing->orient.lvec, amount);
        }
    }

    rdVector_Add3Acc(&pPhysics->velocity, &accel);
    SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);

    if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_82 )
    {
        float fall;
        if ( pPhysics->velocity.z >= 0.0f && sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) > RD_MULTIPLY(pPhysics->maxVelocity, 0.95f) )
        {
            fall = RD_MULTIPLY(sithPhysics_flt_538E0C, secDeltaTime);
        }
        else
        {
            fall = RD_MULTIPLY(sithPhysics_flt_538E08, secDeltaTime);
        }

        pPhysics->velocity.z = pPhysics->velocity.z - fall;
    }
    else if ( pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_84 || pThing->moveStatus == SITHPLAYERMOVE_UNKNOWN_83 || pThing->moveStatus == SITHPLAYERMOVE_JEEP_IMPACT )
    {
        float fall = RD_MULTIPLY(sithPhysics_flt_538E10, secDeltaTime);
        pPhysics->velocity.z = pPhysics->velocity.z - fall;
    }
    else if ( pState->numTorqueWheels >= 3 && !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        float dot = D3DVAL(rdVector_Dot3(&pPhysics->velocity, &normal)); /* spelling-fit */
        dot = stdMath_ClipNearZero(dot);
        if ( dot < 0.0f )
        {
            rdVector_ScaleAdd3Acc(&pPhysics->velocity, &normal, (-dot));
        }
    }

    SITHPHYSICS_CLIPPRECISION3(&pPhysics->velocity);

    if ( sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) < 0.005f && SITHPHYSICS_ISZERO3(&pPhysics->thrust) )
    {
        memset(&pPhysics->velocity, 0, sizeof(rdVector3));
    }

    if ( !SITHPHYSICS_ISZERO3(&pPhysics->velocity) )
    {
        rdVector_Scale3(&pPhysics->deltaVelocity, &pPhysics->velocity, secDeltaTime);
    }

    if ( pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_83 && pThing->moveStatus != SITHPLAYERMOVE_UNKNOWN_84 && pThing->moveStatus != SITHPLAYERMOVE_JEEP_IMPACT )
    {
        rdVector3 delta;

        rdVector_Sub3(&delta, &pState->centerOfMass, &pThing->pos);
        SITHPHYSICS_CLIPPRECISION3(&delta);
        if ( sqrtf(rdVector_Dot3(&delta, &delta)) > 0.001f )
        {
            rdVector_Add3Acc(&pPhysics->deltaVelocity, &delta);
            if ( pState->numTorqueWheels >= 2
                && delta.z < 0.0f
                && pPhysics->velocity.z > 0.0f
                && sqrtf(rdVector_Dot3(&pPhysics->velocity, &pPhysics->velocity)) < RD_MULTIPLY(pPhysics->maxVelocity, 0.95f) )
            {
                pPhysics->velocity.z = RD_MULTIPLY(delta.z / secDeltaTime, 0.75f) + pPhysics->velocity.z;
            }
        }
    }

    sithPhysics_UpdateJeepFx(pThing, &pThing->userblock.pJeep->fxstate, secDeltaTime);
    sithPhysics_UpdateExhaustFx(pThing, &pThing->userblock.pJeep->exhaustInfo, secDeltaTime);
}
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(sithPhysics_countfit_15, 4) /* end count-fit */
