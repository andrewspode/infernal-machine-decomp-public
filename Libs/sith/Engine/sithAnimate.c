#define RDVECTOR_NO_INLINE_LEN3

#include "sithAnimate.h"

#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMath.h>
#include <std/General/stdUtil.h>

/**
* One item-animation frame table: the maximum valid index into aFrames, followed
* by the frame numbers themselves. Retail keeps the two in a single 8-aligned
* object, which is why aFrames starts four bytes after the index (VC5 aligns a
* standalone array of this size to eight bytes).
*/
typedef struct sSithAnimateItemAnimFrames
{
    size_t lastFrameIdx;
    size_t aFrames[13];
} SithAnimateItemAnimFrames;


static int bOpen = 0; // Added: Init. to false

static int lastUsedAnimIndex; // can be -1
static SithAnimationSlot aAnims[512];

static size_t numFreeAnims;
static size_t aFreeAnimNums[STD_ARRAYLEN(aAnims)];

/**
* Animation data for the `in_pull_block.key` animation frames.
* These values precisely coordinate the movement of a pulled object with the player's pulling animation:
* - pullItemAnimFrames.lastFrameIdx: Maximum index for the aPullItemAnimFrames data
* - pullItemAnimFrames.aFrames: Specifies frame numbers in the in_pull_block.key animation where the pulled object
*   should be repositioned i.e.: pulled/moved
* - aPullItemDistances: Defines the exact distance the pulled object should be moved at each animation frame,
*   where distance is the target position relative to the starting point (negative values = pulled move),
*   and offsetFactor is used with frameDelta to adjust the movement between animation frames
*/
static SithAnimateItemAnimFrames pullItemAnimFrames = { 11u, { 0u, 0u, 29u, 38u, 54u, 72u, 110u, 120u, 128u, 135u, 155u, 163u } };
static float aPullItemDistances[22] =
{
    0.0f, 0.0f,
    0.0f, -0.00190222f,
    -0.01712f, -0.00308813f,
    -0.06653f, 0.0f,
    -0.06653f, -0.00282553f,
    -0.17390001f, 0.0f,
    -0.17390001f, -0.00110625f,
    -0.18275f, -0.00265571f,
    -0.20134f, 0.0f,
    -0.20134f, 0.000335f,
    -0.2f, 0.0f
};

/**
* Animation data for the `in_push_block.key` animation frames.
* These values precisely coordinate the movement of a pushed object with the player's pushing animation:
* - pushItemAnimFrames.lastFrameIdx: Maximum index for the aPushItemAnimFrames data
* - pushItemAnimFrames.aFrames: Specifies frame numbers in the in_push_block.key animation where the pushed object
*   should be repositioned i.e.: pushed/moved
* - aPushItemDistances: Defines the exact distance the pushed object should be moved at each animation frame,
*   where distance is the target position relative to the starting point (positive values = pushed forward),
*   and offsetFactor is used with frameDelta to adjust the movement between animation frames
*/
static SithAnimateItemAnimFrames pushItemAnimFrames = { 12u, { 0u, 0u, 12u, 25u, 35u, 51u, 66u, 85u, 99u, 108u, 121u, 129u, 151u } };
static float aPushItemDistances[24] =
{
    0.0f, 0.0f,
    0.0f, 0.00167f,
    0.02171f, 0.002697f,
    0.04868f, 0.0f,
    0.04868f, 0.00214133f,
    0.0808f, 0.0030821f,
    0.13936f, 0.0f,
    0.13936f, 0.00213889f,
    0.15861f, 0.00298615f,
    0.19743f, 0.0f,
    0.19743f, 0.00012682f,
    0.201f, 0.0f
};


static void J3DAPI sithAnimate_BuildScrollFaceRotate(rdMatrix34* pOrient, const rdFace* pFace, SithSurfaceFlag surfflags);
static float J3DAPI sithAnimate_ExtractUVAngle(float v, float u);

void J3DAPI sithAnimate_UpdateCameraZoomAnim(SithAnimationSlot* pAnim, float secTimeDelta);
void J3DAPI sithAnimate_UpdateSpriteSizeAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateLightAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateThingMoveAnim(SithAnimationSlot* pAnim, float secTimeDelta);
void J3DAPI sithAnimate_UpdateThingMovePosAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateThingFadeAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateSurfaceScrollAnim(SithAnimationSlot* pAnim, float secDeltaTime, int animNum);
void J3DAPI sithAnimate_UpdateSlideSkyAnim(SithAnimationSlot* pAnim, int skyType, float secDeltaTime, int animNum);
void J3DAPI sithAnimate_UpdatePageFlipAnim(SithAnimationSlot* pAnim);
void J3DAPI sithAnimate_UpdatePushItemAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdatePullItemAnim(SithAnimationSlot* pAnim, float secDeltaTime);
void J3DAPI sithAnimate_UpdateThingQuickTurnAnim(SithAnimationSlot* pAnim, float secDeltaTime);

void J3DAPI sithAnimate_FreeAllAnims(void);
void J3DAPI sithAnimate_FreeAnim(SithAnimationSlot* pAnim);
SithAnimationSlot* J3DAPI sithAnimate_NewAnim(void);

size_t J3DAPI sithAnimate_GetAnimFrameIndex(float frame, size_t size, const size_t* aAnimFrames);
float J3DAPI sithAnimate_GetPuppetTrackCurFrame(SithThing* pThing, int trackNum);
SithSurface* J3DAPI sithAnimate_GetThingFloorSurface(const SithThing* pThing);
void J3DAPI sithAnimate_ExitFloorSurface(const SithThing* pThing, const SithSurface* pSurface);
void J3DAPI sithAnimate_FindAndEnterFloorSurface(const SithThing* pSrcThing, const SithSurface* pSurf);


void J3DAPI sithAnimate_Startup(void)
{
    sithAnimate_FreeAllAnims();
}

void J3DAPI sithAnimate_Shutdown(void)
{}

int J3DAPI sithAnimate_Open(void)
{
    sithAnimate_FreeAllAnims();
    bOpen = 1;
    return 0;
}

void J3DAPI sithAnimate_Close(void)
{
    sithAnimate_FreeAllAnims();
    bOpen = 0;
}

void J3DAPI sithAnimate_Reset(void)
{
    sithAnimate_FreeAllAnims();
}

void J3DAPI sithAnimate_Stop(SithAnimationSlot* pAnim)
{
#line 227
    SITH_ASSERTREL(pAnim);
    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 && (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
    {
#line 232
        SITH_ASSERTREL(pAnim->pSurface);
        pAnim->pSurface->flags &= ~SITH_SURFACE_SCROLLING;

        memset(&pAnim->direction3, 0, sizeof(pAnim->direction3));
        memset(&pAnim->direction2, 0, sizeof(pAnim->direction2));
    }

    if ( (pAnim->flags & (SITHANIMATE_THING_MOVEPOS | SITHANIMATE_THING_MOVE)) != 0 && pAnim->pThing && pAnim->pThing->moveType == SITH_MT_PHYSICS )
    {
        pAnim->pThing->moveInfo.physics.flags &= ~SITH_PF_NOUPDATE;
    }

    sithAnimate_FreeAnim(pAnim);
}

int J3DAPI sithAnimate_GetSurfaceAnimIndex(const SithSurface* pSurf)
{
    SithAnimationSlot* pAnim;
    int index;

    pAnim = sithAnimate_GetSurfaceAnim(pSurf);
    if ( !pAnim )
    {
        return -1;
    }

    index = pAnim - aAnims;
    return index;
}

static float J3DAPI sithAnimate_ExtractUVAngle(float v, float u)
{
    float angle;
    double tangent;

    if ( u == 0.0f && v == 0.0f )
    {
        return 0.0f;
    }

    if ( u == 0.0f )
    {
        if ( v > 0.0f )
        {
            return 90.0f;
        }

        return -90.0f;
    }
    else if ( v == 0.0f )
    {
        if ( u > 0.0f )
        {
            return 0.0f;
        }

        return 180.0f;
    }

    tangent = v / u;
    angle = STDMATH_RAD2DEG(D3DVAL(atan(tangent)));
    if ( u < 0.0f && v > 0.0f )
    {
#line 327
        SITH_ASSERTREL(angle < ((float)0.0));
        angle = angle + 180.0f;
    }

    if ( u < 0.0f && v < 0.0f )
    {
#line 334
        SITH_ASSERTREL(angle > ((float)0.0));
        angle -= 180.0f;
    }

#line 338
    SITH_ASSERTREL(angle >= -((float)180.0));
#line 339
    SITH_ASSERTREL(angle <= ((float)180.0));
    return angle;
}

static void J3DAPI sithAnimate_BuildScrollFaceRotate(rdMatrix34* pOrient, const rdFace* pFace, SithSurfaceFlag surfflags)
{
    size_t vertNum = 1;
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    rdVector3 vert;
    rdVector3 nextVert;
    rdVector2 texVert;
    rdVector2 nextTexVert;
    rdVector3 dvert;
    rdVector2 dtexVert;
    rdVector3 dir;
    rdMatrix34 mat;
    rdVector3 angles;
    rdVector3 scale;
    float angle;
    float size;

#line 366
    SITH_ASSERTREL(pFace != NULL);
#line 367
    SITH_ASSERTREL(pFace->numVertices >= 3);

    rdVector_Copy3(&mat.uvec, &pFace->normal);
    rdVector_Copy3(&vert, &pWorld->aVertices[pFace->aVertices[0]]);
    rdVector_Copy3(&nextVert, &pWorld->aVertices[pFace->aVertices[vertNum]]);
    rdVector_Copy2(&texVert, &pWorld->aTexVerticies[pFace->aTexVertices[0]]);
    rdVector_Copy2(&nextTexVert, &pWorld->aTexVerticies[pFace->aTexVertices[vertNum]]);
    dvert.x = vert.x - nextVert.x;
    dvert.y = vert.y - nextVert.y;
    dvert.z = vert.z - nextVert.z;

    while ( rdVector_Len3(&dvert) < 0.0001f )
    {
        if ( ++vertNum == pFace->numVertices )
        {
            vertNum = 0;
        }
        rdVector_Copy3(&nextVert, &pWorld->aVertices[pFace->aVertices[vertNum]]);
        rdVector_Copy2(&nextTexVert, &pWorld->aTexVerticies[pFace->aTexVertices[vertNum]]);
        /* Retail does not recalculate dvert here. */
    }

    dtexVert.x = nextTexVert.x - texVert.x;
    dtexVert.y = nextTexVert.y - texVert.y;
    rdVector_Sub3(&dir, &nextVert, &vert);
    rdVector_Copy3(&mat.rvec, &dir);
    rdVector_Normalize3Acc(&mat.uvec);
    rdVector_Normalize3Acc(&mat.rvec);
    rdVector_Cross3(&mat.lvec, &mat.uvec, &mat.rvec);
    rdVector_Copy3(&mat.dvec, &vert);
    rdMatrix_InvertOrtho34(pOrient, &mat);

    angle = sithAnimate_ExtractUVAngle(dtexVert.y, dtexVert.x);
    size = 320.0f;
    if ( (surfflags & SITH_SURFACE_QUARTERSIZE) != 0 )
    {
        size = size / 4.0f;
    }
    else if ( (surfflags & SITH_SURFACE_HALFSIZE) != 0 )
    {
        size = size / 2.0f;
    }
    else if ( (surfflags & SITH_SURFACE_DOUBLESIZE) != 0 )
    {
        size = size * 2.0f;
    }
    else if ( (surfflags & SITH_SURFACE_QUADRUPLESIZE) != 0 )
    {
        size = size * 4.0f;
    }
    else if ( (surfflags & SITH_SURFACE_EIGHTSIZE) != 0 )
    {
        size = size / 8.0f;
    }

    angles.x = 0.0f;
    angles.y = angle;
    angles.z = 0.0f;
    rdMatrix_PostRotate34(pOrient, &angles);
    scale.x = size;
    scale.y = size;
    scale.z = 1.0f;
    rdMatrix_PostScale34(pOrient, &scale);
}

SithAnimationSlot* J3DAPI sithAnimate_StartScrollSurfaceAnim(SithSurface* pSurf, const rdVector3* velocity)
{
    SithAnimationSlot* pAnim;
    rdMatrix34 orient;
    rdVector2 curdest; // Note, only x and y are read back; the transform below writes a third float past it

#line 455
    SITH_ASSERTREL(pSurf);

    if ( !pSurf->face.pMaterial )
    {
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->flags      = SITHANIMATE_SCROLL | SITHANIMATE_SURFACE;
    pAnim->pSurface   = pSurf;
    rdVector_Copy3(&pAnim->direction3, velocity);

    // Calculate 2D direction

    sithAnimate_BuildScrollFaceRotate(&orient, &pSurf->face, pSurf->flags);


    rdMatrix_TransformVector34((rdVector3*)&curdest, velocity, &orient);

    pAnim->direction2.x = -curdest.x;
    pAnim->direction2.y = -curdest.y;
    pAnim->direction2.x /= pSurf->face.pMaterial->width;
    pAnim->direction2.y /= pSurf->face.pMaterial->height;

    // Mark the surface as scrolling
    pSurf->flags |= SITH_SURFACE_SCROLLING;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSideSkyAnim(SithSurfaceFlag skyType, const rdVector2* pDirection)
{
    SithAnimationSlot* pAnim;

#line 505
    SITH_ASSERTREL(skyType == SITH_SURFACE_HORIZONSKY || skyType == SITH_SURFACE_CEILINGSKY);

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    if ( skyType == SITH_SURFACE_HORIZONSKY )
    {
        pAnim->flags = SITHANIMATE_SKYHORIZON | SITHANIMATE_SCROLL;
    }
    else if ( skyType == SITH_SURFACE_CEILINGSKY )
    {
        pAnim->flags = SITHANIMATE_SKYCEILING | SITHANIMATE_SCROLL;
    }

    rdVector_Copy2(&pAnim->direction2, pDirection);
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSpriteAnim(SithThing* pSprite, float fps, SithAnimateFlags flags)
{
    SithAnimationSlot* pAnim;
    float secPerFrameF;

#line 546
    SITH_ASSERTREL(pSprite && (fps >= ((float)0.0)));
#line 547
    SITH_ASSERTREL(pSprite->renderData.type == RD_THING_SPRITE3);

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pSprite->renderData.matCelNum = 0;
    if ( (flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
    {
        pAnim->curFrame = 2;
    }
    else
    {
        if ( (flags & SITHANIMATE_START_SECOND_FRAME) != 0 )
        {
            pAnim->curFrame = 1;
        }
        else
        {
            pAnim->curFrame = 0;
        }
    }

    pAnim->flags             = (uint16_t)flags | (SITHANIMATE_PAGEFLIP | SITHANIMATE_SPRITE); // (uint16_t)flags - clipped to 0xFFFF
    pAnim->pThing            = pSprite;
    pAnim->thingSignature    = pSprite->signature;
    pAnim->pMaterial         = pSprite->renderData.data.pSprite3->face.pMaterial;
    pAnim->msecPerFrame      = (uint32_t)(secPerFrameF = 1000.0f / fps);
    pAnim->msecNextFrameTime = sithTime_msecGameTime + pAnim->msecPerFrame;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSurfaceAnim(SithSurface* pSurf, float fps, SithAnimateFlags flags)
{
    SithAnimationSlot* pAnim;
    float secPerFrameF;

#line 598
    SITH_ASSERTREL(pSurf);
#line 599
    SITH_ASSERTREL(fps > ((float)0.0));

    if ( !pSurf->face.pMaterial )
    {
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 609
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( (flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
    {
        pAnim->curFrame = 2;
    }
    else
    {
        if ( (flags & SITHANIMATE_START_SECOND_FRAME) != 0 )
        {
            pAnim->curFrame = 1;
        }
        else
        {
            pAnim->curFrame = 0;
        }
    }

    pAnim->flags        = (uint16_t)flags | (SITHANIMATE_PAGEFLIP | SITHANIMATE_SURFACE); // (uint16_t)flags - clipped to 0xFFFF
    pAnim->pSurface     = pSurf;
    pAnim->pMaterial    = pSurf->face.pMaterial;
    pAnim->msecPerFrame = (uint32_t)(secPerFrameF = 1000.0f / fps);

    if ( !pAnim->msecPerFrame )
    {
#line 628
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad parameters to startSurfaceAnim.\n", 0, 0, 0, 0);
        sithAnimate_FreeAnim(pAnim);
        return NULL;
    }

    pAnim->msecNextFrameTime = sithTime_msecGameTime + pAnim->msecPerFrame;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartMaterialAnim(rdMaterial* pMat, float fps, SithAnimateFlags flags)
{
    SithAnimationSlot* pAnim;
    float secPerFrameF;

#line 664
    SITH_ASSERTREL(pMat);
#line 665
    SITH_ASSERTREL(fps > ((float)0.0));

    if ( pMat->numCels < 1 )
    {
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 673
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( (flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
    {
        pAnim->curFrame = 2;
    }
    else
    {
        if ( (flags & SITHANIMATE_START_SECOND_FRAME) != 0 )
        {
            pAnim->curFrame = 1;
        }
        else
        {
            pAnim->curFrame = 0;
        }
    }

    pAnim->pMaterial    = pMat;
    pAnim->flags        = (uint16_t)flags | (SITHANIMATE_PAGEFLIP | SITHANIMATE_MATERIAL); // (uint16_t)flags - clipped to 0xFFFF
    pAnim->msecPerFrame = (uint32_t)(secPerFrameF = 1000.0f / fps);
    if ( !pAnim->msecPerFrame )
    {
#line 692
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Bad parameters to startMaterialAnim.\n", 0, 0, 0, 0);
        sithAnimate_FreeAnim(pAnim);
        return NULL;
    }

    pAnim->msecNextFrameTime = sithTime_msecGameTime + pAnim->msecPerFrame;
    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartSurfaceLightAnim(SithSurface* pSurface, const rdVector3* light, float timeDelta, SithAnimateFlags flags)
{
    float curdRed;
    float dBlue;
    float curdGreen;
    SithAnimationSlot* pAnim;

#line 719
    SITH_ASSERTREL(pSurface && (timeDelta > ((float)0.0)));

    // flags: retail passes 1 from SurfaceLightAnim and 0 from SetSurfaceLight, but never reads it here

    curdRed = light->red - pSurface->face.extraLight.red;
    curdGreen = light->green - pSurface->face.extraLight.green;
    dBlue = light->blue - pSurface->face.extraLight.blue;

    if ( curdRed == 0.0f && curdGreen == 0.0f && dBlue == 0.0f )
    {
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 732
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots.\n", 0, 0, 0, 0);
        return NULL;
    }

    pAnim->pSurface      = pSurface;
    pAnim->flags         = SITHANIMATE_LIGHT | SITHANIMATE_SURFACE;
    pAnim->deltaVector.x = curdRed / timeDelta;
    pAnim->deltaVector.y = curdGreen / timeDelta;
    pAnim->deltaVector.z = dBlue / timeDelta;
    rdVector_Copy4(&pAnim->curVector, &pSurface->face.extraLight);
    rdVector_Copy3((rdVector3*)&pAnim->endVector, light); // Note, alpha not set

    return pAnim;
}


/* Discarded by the retail linker (/OPT:REF): no retail caller survives. Restored in full from the
   debug build (0x0047AB31, between StartSurfaceLightAnim and StartSpriteSizeAnim, called only by
   sithPlayerActions' discarded QuickTurnLeft/QuickTurnRight); its 4.0f is the orphaned pool
   constant at 0x0050759C. */
SithAnimationSlot* J3DAPI sithAnimate_StartThingQuickTurnAnim(SithThing* pThing, int direction)
{
    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->curValue = 0.0f;
    if ( direction == 1 )
    {
        pAnim->endValue = -90.0f;
    }
    else
    {
        pAnim->endValue = 90.0f;
    }

    pAnim->deltaValue     = pAnim->endValue * 4.0f;
    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = SITHANIMATE_THING_QUICKTURN | SITHANIMATE_THING;

    return pAnim;
}

#pragma optimize("", off)
SithAnimationSlot* J3DAPI sithAnimate_StartSpriteSizeAnim(SithThing* pSprite, const rdVector3* start, const rdVector3* end, float timeDelta)
{
    float size;
    SithAnimationSlot* pAnim;

    // SITH_ASSERTREL is <assert.h>'s `||` form: under /Od it costs one more scratch register
    // than `if (!cond)`, which is what puts every later statement of this
    // function on retail's eax->ecx->edx rotation phase.
#line 823
    SITH_ASSERTREL(pSprite);

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->flags       = SITHANIMATE_SPRITE_SIZE | SITHANIMATE_THING;
    pAnim->endVector.x = end->x;    // width
    pAnim->endVector.y = end->y;    // height
    pAnim->endVector.z = end->z;    // alpha
    pAnim->endVector.w = 0.0f;

    pAnim->curVector.x = start->x;  // width
    pAnim->curVector.y = start->y;  // height
    pAnim->curVector.z = start->z;  // alpha
    pAnim->curVector.w = 0.0f;

    size = pAnim->endVector.x - pAnim->curVector.x;
    pAnim->deltaVector.x = size / timeDelta;
    size = pAnim->endVector.y - pAnim->curVector.y;
    pAnim->deltaVector.y = size / timeDelta;
    size = end->z - start->z;
    pAnim->deltaVector.z = size / timeDelta;

    pSprite->thingInfo.spriteInfo.width  = pAnim->curVector.x;
    pSprite->thingInfo.spriteInfo.height = pAnim->curVector.y;
    pSprite->thingInfo.spriteInfo.alpha  = pAnim->curVector.z;

    pAnim->thingSignature = pSprite->signature;
    pAnim->pThing = pSprite;

    return pAnim;
}
#pragma optimize("", on)

SithAnimationSlot* J3DAPI sithAnimate_StartSectorLightAnim(SithSector* pSector, const rdVector3* pNewLight, float timeDelta, SithAnimateFlags flags)
{
    rdVector4 colorDelta;
    SithAnimationSlot* pAnim;

#line 883
    SITH_ASSERTREL(pSector && (timeDelta > ((float)0.0)));
    flags &= SITHANIMATE_LOOP; // clear all but loop flag


    colorDelta.red = pNewLight->red - pSector->extraLight.red;
    colorDelta.green = pNewLight->green - pSector->extraLight.green;
    colorDelta.blue = pNewLight->blue - pSector->extraLight.blue;

    if ( colorDelta.x == 0.0f && colorDelta.y == 0.0f && colorDelta.z == 0.0f )
    {
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 900
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots.\n", 0, 0, 0, 0);
        return NULL;
    }

    pAnim->pSector           = pSector;
    pAnim->flags             = flags | (SITHANIMATE_SECTOR | SITHANIMATE_LIGHT);
    pAnim->deltaVector.red   = colorDelta.red / timeDelta;
    pAnim->deltaVector.green = colorDelta.green / timeDelta;
    pAnim->deltaVector.blue  = colorDelta.blue / timeDelta;
    rdVector_Copy4(&pAnim->startVector, &pSector->extraLight); // Should it set vecCurrent instead?
    rdVector_Copy3((rdVector3*)&pAnim->endVector, pNewLight); // Note, alpha not set

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingLightAnim(SithThing* pThing, const rdVector4* light, float timeDelta, SithAnimateFlags flags)
{
    rdVector4 lightDelta;
    SithAnimationSlot* pCurAnim;

#line 937
    SITH_ASSERTREL(pThing && (timeDelta > ((float)0.0)));

    flags &= SITHANIMATE_LOOP; // Clear all but loop flag


    lightDelta.red   = light->red - pThing->light.color.red;
    lightDelta.green = light->green - pThing->light.color.green;
    lightDelta.blue  = light->blue - pThing->light.color.blue;
    lightDelta.w     = light->w - pThing->light.minRadius; // range delta

    if ( lightDelta.red == 0.0f && lightDelta.green == 0.0f && lightDelta.blue == 0.0f && lightDelta.w == 0.0f )
    {
        return NULL;
    }

    pCurAnim = sithAnimate_NewAnim();
    if ( !pCurAnim )
    {
#line 955
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots.\n", 0, 0, 0, 0);
        return NULL;
    }

    pCurAnim->pThing         = pThing;
    pCurAnim->thingSignature = pThing->signature;
    pCurAnim->flags          = flags | (SITHANIMATE_LIGHT | SITHANIMATE_THING);
    pThing->flags |= SITH_TF_EMITLIGHT;

    pCurAnim->deltaVector.red   = lightDelta.red / timeDelta;
    pCurAnim->deltaVector.green = lightDelta.green / timeDelta;
    pCurAnim->deltaVector.blue  = lightDelta.blue / timeDelta;
    pCurAnim->deltaVector.w     = lightDelta.w / timeDelta; // range

    rdVector_Copy4(&pCurAnim->startVector, &pThing->light.color);
    pCurAnim->startVector.w = pThing->light.minRadius;

    rdVector_Copy4(&pCurAnim->curVector, &pThing->light.color);
    pCurAnim->curVector.w = pCurAnim->startVector.w; // range

    rdVector_Copy4(&pCurAnim->endVector, light);


    return pCurAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingMove(SithThing* pThing, const rdVector3* pDirection, float distance, float timeDelta)
{
    SithAnimationSlot* pAnim;

    if ( !pThing )
    {
#line 987
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing is NULL in sithAnimate_StartThingMove()\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( pDirection->x == 0.0f && pDirection->y == 0.0f && pDirection->z == 0.0f )
    {
#line 992
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Direction is 0 in sithAnimate_StartThingMove(), %s\n", pThing->aName, 0, 0, 0);
        return NULL;
    }

    if ( distance < 0.0f )
    {
#line 997
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Distance < 0, sithAnimate_StartThingMove(), %s\n", pThing->aName, 0, 0, 0);
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        return NULL;
    }

    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = SITHANIMATE_THING_MOVE | SITHANIMATE_THING;
    pAnim->unknown24      = 0;
    pAnim->curValue       = 0.0f;
    pAnim->endValue       = distance;
    rdVector_Copy3(&pAnim->direction3, pDirection);
    pAnim->deltaValue     = distance / timeDelta; // deltaDistance

    // Mark thing to not be affected by physics
    pThing->moveInfo.physics.flags |= SITH_PF_NOUPDATE;

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingMoveToPos(SithThing* pThing, const rdVector3* pPos, float time)
{
    SithAnimationSlot* pAnim;

    if ( !pThing )
    {
#line 1028
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Thing is NULL in sithAnimate_StartThingMoveToPos()\n", 0, 0, 0, 0);
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
        if ( (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }

        return NULL;
    }

    pAnim->pThing           = pThing;
    pAnim->thingSignature   = pThing->signature;
    pAnim->flags            = SITHANIMATE_THING_MOVEPOS | SITHANIMATE_THING;
    pAnim->secTimeRemaining = time;
    rdVector_Copy3(&pAnim->thingEndPosition, pPos);

    if ( pThing->moveType == SITH_MT_PHYSICS )
    {
        // Mark thing to not be affected by physics
        pThing->moveInfo.physics.flags |= SITH_PF_NOUPDATE;
    }

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_StartThingFadeAnim(SithThing* pThing, const rdVector4* startColor, const rdVector4* endColor, float timeDelta, SithAnimateFlags flags)
{
    rdVector4 curcolorDelta;
    rdVector4 initialColor;
    SithAnimationSlot* pAnim;

#line 1066
    SITH_ASSERTREL(pThing && (timeDelta > ((float)0.0)));

    flags &= SITHANIMATE_LOOP; // Clear all but loop flag


    rdVector_Copy4(&initialColor, startColor);
    rdVector_Sub4(&curcolorDelta, endColor, &initialColor);

    if ( curcolorDelta.red == 0.0f && curcolorDelta.green == 0.0f && curcolorDelta.blue == 0.0f && curcolorDelta.alpha == 0.0f )
    {
        return NULL;
    }

    pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 1086
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots.\n", 0, 0, 0, 0);
        return NULL;
    }

    pAnim->pThing         = pThing;
    pAnim->thingSignature = pThing->signature;
    pAnim->flags          = flags | SITHANIMATE_THING_FADE;

    pAnim->deltaVector.red   = curcolorDelta.red / timeDelta;
    pAnim->deltaVector.green = curcolorDelta.green / timeDelta;
    pAnim->deltaVector.blue  = curcolorDelta.blue / timeDelta;
    pAnim->deltaVector.alpha = curcolorDelta.alpha / timeDelta;

    rdVector_Copy4(&pAnim->startVector, &initialColor);
    rdVector_Copy4(&pAnim->curVector, &initialColor);
    rdVector_Copy4(&pAnim->endVector, endColor);

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_CameraZoom(SithCamera* pCamera, float fov, float timeDelta)
{
    SithCamera* pCurCameraRef = pCamera;  // unreferenced; the debug build's frame keeps its slot
    float curFov;
    float endFov;
    float dFov;
    SithAnimationSlot* pZoomAnimSlot;

#line 1127
    SITH_ASSERTREL(timeDelta > ((float)0.0));

    curFov = D3DVAL(sithCamera_pCurCamera->fov);
    endFov = fov;
    dFov = endFov - curFov;
    if ( dFov == 0.0f )
    {
        return NULL;
    }

    pZoomAnimSlot = sithAnimate_NewAnim();
    if ( !pZoomAnimSlot )
    {
#line 1137
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots, CameraZoom.\n", 0, 0, 0, 0);
        return NULL;
    }

    pZoomAnimSlot->pThing         = (SithThing*)pCamera;   // ?? huh, but thing is expected ?
    pZoomAnimSlot->thingSignature = 0;
    pZoomAnimSlot->flags          = SITHANIMATE_CAMERA_ZOOM | SITHANIMATE_THING;
    pZoomAnimSlot->deltaValue     = dFov / timeDelta;
    pZoomAnimSlot->endValue       = fov;
    pZoomAnimSlot->curValue       = curFov;

    return pZoomAnimSlot;
}

SithAnimationSlot* J3DAPI sithAnimate_PushItem(SithThing* pActor, SithThing* pItem, const rdVector3* pDirection, int trackNum)
{
    SithAnimationSlot* pAnim = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 1159
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots, PushItem.\n", 0, 0, 0, 0);
        return NULL;
    }

#line 1163
    SITH_ASSERTREL(pActor);
    pAnim->pThing = pActor;

    if ( pItem )
    {
        pAnim->pItemThing = pItem;

    }

    rdVector_Copy3(&pAnim->direction3, pDirection);
    pAnim->deltaValue     = 0.002f;       // ??
    pAnim->endValue       = 0.2f;                // 2m
    pAnim->curValue       = 0.0f;
    pAnim->thingSignature = pActor->signature;
    pAnim->flags          = SITHANIMATE_THING | SITHANIMATE_PUSHITEM;
    pAnim->curFrame       = 0;
    pAnim->deltaValue     = 0.0f;                // ?? it is set to current animate frame in update
    pAnim->startVector.x  = 0.0f;                // Holds current distance item was pushed for
    pAnim->trackNum       = trackNum;
    pAnim->msecPerFrame   = pItem->collide.type;
    pItem->collide.type   = SITH_COLLIDE_NONE;
    pAnim->pSurface       = sithAnimate_GetThingFloorSurface(pItem);

    if ( pAnim->pSurface )
    {
        sithAnimate_ExitFloorSurface(pItem, pAnim->pSurface);
    }

    return pAnim;
}

SithAnimationSlot* J3DAPI sithAnimate_PullItem(SithThing* pActor, SithThing* pItem, const rdVector3* pDirection, int trackNum)
{
    SithAnimationSlot* pAnim  = sithAnimate_NewAnim();
    if ( !pAnim )
    {
#line 1198
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of animation slots, PullItem.\n", 0, 0, 0, 0);
        return NULL;
    }

#line 1202
    SITH_ASSERTREL(pActor);
    pAnim->pThing = pActor;

    if ( pItem )
    {
        pAnim->pItemThing = pItem;

    }

    rdVector_Copy3(&pAnim->direction3, pDirection);
    pAnim->deltaValue     = 0.002f;       // ???
    pAnim->endValue       = 0.2f;                // 2m
    pAnim->curValue       = 0.0f;
    pAnim->thingSignature = pActor->signature;
    pAnim->flags          = SITHANIMATE_THING | SITHANIMATE_PULLITEM;
    pAnim->curFrame       = 0;
    pAnim->deltaValue     = 0.0f;                // ?? it is set to current animate frame in update
    pAnim->startVector.x  = 0.0f;
    pAnim->trackNum       = trackNum;
    pAnim->msecPerFrame   = pItem->collide.type;
    pItem->collide.type   = SITH_COLLIDE_NONE;
    pAnim->pSurface       = sithAnimate_GetThingFloorSurface(pItem);

    if ( pAnim->pSurface )
    {
        sithAnimate_ExitFloorSurface(pItem, pAnim->pSurface);
    }

    return pAnim;
}

void J3DAPI sithAnimate_Update(float secTimeDelta)
{
    int curLastUsedAnimNum = lastUsedAnimIndex;
    int animNum;
    SithAnimationSlot* pAnim;
    for ( animNum = 0, pAnim = aAnims; animNum <= curLastUsedAnimNum; animNum++, pAnim++ )
    {
        if ( !pAnim->flags )
        {
            continue;
        }

        // Do this check first, as pThing is actually SithCamera here
        if ( pAnim->pThing && (pAnim->flags & SITHANIMATE_CAMERA_ZOOM) != 0 )
        {
            sithAnimate_UpdateCameraZoomAnim(pAnim, secTimeDelta);
            continue;
        }

        if ( pAnim->pThing && (pAnim->pThing->type == SITH_THING_FREE || pAnim->pThing->signature != pAnim->thingSignature) )
        {
            // Thing was freed, remove thing anim
            sithAnimate_FreeAnim(pAnim);
            continue;
        }

        if ( (pAnim->flags & SITHANIMATE_THING_MOVE) != 0 )
        {
            sithAnimate_UpdateThingMoveAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_THING_MOVEPOS) != 0 )
        {
            sithAnimate_UpdateThingMovePosAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_PUSHITEM) != 0 )
        {
            sithAnimate_UpdatePushItemAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_PULLITEM) != 0 )
        {
            sithAnimate_UpdatePullItemAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_SPRITE_SIZE) != 0 )
        {
            sithAnimate_UpdateSpriteSizeAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_THING_QUICKTURN) != 0 )
        {
            sithAnimate_UpdateThingQuickTurnAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
        {
            if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
            {
                sithAnimate_UpdateSurfaceScrollAnim(pAnim, secTimeDelta, animNum);
            }
            else if ( (pAnim->flags & SITHANIMATE_SKYHORIZON) != 0 )
            {
                sithAnimate_UpdateSlideSkyAnim(pAnim, SITH_SURFACE_HORIZONSKY, secTimeDelta, animNum);
            }
            else if ( (pAnim->flags & SITHANIMATE_SKYCEILING) != 0 )
            {
                sithAnimate_UpdateSlideSkyAnim(pAnim, SITH_SURFACE_CEILINGSKY, secTimeDelta, animNum);
            }
        }
        else if ( (pAnim->flags & SITHANIMATE_PAGEFLIP) != 0 && pAnim->msecNextFrameTime <= sithTime_msecGameTime )
        {
            sithAnimate_UpdatePageFlipAnim(pAnim);
        }
        else if ( (pAnim->flags & SITHANIMATE_LIGHT) != 0 )
        {
            sithAnimate_UpdateLightAnim(pAnim, secTimeDelta);
        }
        else if ( (pAnim->flags & SITHANIMATE_THING_FADE) != 0 )
        {
            sithAnimate_UpdateThingFadeAnim(pAnim, secTimeDelta);
        }
    }
}

void J3DAPI sithAnimate_UpdateThingQuickTurnAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    int bFinish = 0;
    float currentnewAngle;

    float newRotAngle = D3DVAL(pAnim->deltaValue * secDeltaTime);
    rdVector3 pyr;
    if ( newRotAngle > 0.0f )
    {
        if ( newRotAngle > 90.0f )
        {
            newRotAngle = 90.0f;
        }
        else if ( newRotAngle < 0.0f && newRotAngle < -90.0f )
        {
            newRotAngle = -90.0f;
        }
    }

    currentnewAngle = pAnim->curValue + newRotAngle;
    if ( pAnim->endValue > 0.0f )
    {
        if ( currentnewAngle >= pAnim->endValue )
        {
            bFinish = 1;
        }
    }
    else if ( pAnim->endValue < 0.0f && currentnewAngle <= pAnim->endValue )
    {
        bFinish = 1;
    }

    pAnim->curValue += newRotAngle;

    pyr.pitch = 0.0f;
    pyr.yaw = newRotAngle;
    pyr.roll = 0.0f;
    rdMatrix_PreRotate34(&pAnim->pThing->orient, &pyr);

    if ( bFinish )
    {
        pAnim->pThing->thingInfo.actorInfo.flags &= ~SITH_AF_CONTROLSDISABLED;
        pAnim->pThing->moveStatus                 = SITHPLAYERMOVE_STILL;
        sithAnimate_Stop(pAnim);
    }
}

SithAnimationSlot* J3DAPI sithAnimate_GetSurfaceAnim(const SithSurface* pSurf)
{
    int lastUsed;
    int i;
    SithAnimationSlot* pAnim;
#line 1349
    SITH_ASSERTREL(pSurf);

    lastUsed = lastUsedAnimIndex;
    for ( i = 0, pAnim = aAnims; i <= lastUsed; ++i, ++pAnim )
    {
        if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 && pAnim->pSurface == pSurf )
        {
            break;
        }
    }
    if ( i > lastUsed )
    {
        return NULL;
    }

    return pAnim;
}

void J3DAPI sithAnimate_GetSurfaceScrollingDirection(const SithSurface* pSurf, rdVector3* result)
{
    SithAnimationSlot* pAnim;

#line 1386
    SITH_ASSERTREL(pSurf && result);

    pAnim = sithAnimate_GetSurfaceAnim(pSurf);
    if ( !pAnim )
    {
        memset(result, 0, sizeof(*result));
    }
    else
    {
        rdVector_Copy3(result, &pAnim->direction3);
    }
}

SithAnimationSlot* J3DAPI sithAnimate_GetAnim(int animID)
{
    int i;
    for ( i = 0; i <= lastUsedAnimIndex; ++i )
    {
        if ( aAnims[i].flags != 0 && aAnims[i].animID == animID )
        {
            return &aAnims[i];
        }
    }

    return NULL;
}

SithAnimationSlot* J3DAPI sithAnimate_GetMaterialAnim(const rdMaterial* pMaterial)
{
    int i;
    for ( i = 0; i <= lastUsedAnimIndex; ++i )
    {
        if ( aAnims[i].pMaterial == pMaterial )
        {
            return &aAnims[i];
        }
    }

    return NULL;
}

SithAnimationSlot* J3DAPI sithAnimate_Create(void)
{
    return sithAnimate_NewAnim();
}

int J3DAPI sithAnimate_Save(unsigned int outstream)
{
    int bCurError = 0;
    int i;

    if ( (sithMessage_outputstream & outstream) == 0 )
    {
        return 1;
    }

    for ( i = 0; i <= lastUsedAnimIndex; ++i )
    {
        if ( aAnims[i].flags != 0 )
        {
            // Hmm pThing cal also be SithCamera type
            if ( (aAnims[i].flags & (SITHANIMATE_SPRITE | SITHANIMATE_THING)) != 0 && aAnims[i].pThing && !sithThing_CanSync(aAnims[i].pThing) )
            {
                continue;
            }

            bCurError = sithDSS_AnimStatus(&aAnims[i], 0, outstream);
            if ( bCurError )
            {
                break;
            }
        }
    }

    return bCurError;
}

void J3DAPI sithAnimate_UpdateCameraZoomAnim(SithAnimationSlot* pAnim, float secTimeDelta)
{
    int bFinish = 0;

    float newFOV = D3DVAL(pAnim->deltaValue) * secTimeDelta + pAnim->curValue;
    if ( pAnim->deltaValue < 0.0f && newFOV < pAnim->endValue ) // End FOV lt new FOV
    {
        newFOV = pAnim->endValue;
        bFinish = 1;
    }
    else if ( pAnim->deltaValue > 0.0f && newFOV > pAnim->endValue ) // End FOV lt new FOV
    {
        newFOV = pAnim->endValue;
        bFinish = 1;
    }

    pAnim->curValue = newFOV;
    sithCamera_SetCameraFOV(sithCamera_pCurCamera, newFOV);

    if ( bFinish )
    {
        sithAnimate_Stop(pAnim);
    }
}

#pragma optimize("", off)
void J3DAPI sithAnimate_UpdateSpriteSizeAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    float curWidth;
    float curHeight;
    float curAlpha;
    int bEndWidth  = 0;
    int bHeightAtTarget = 0;
    int bAlphaDone  = 0;

    if ( !pAnim->pThing )
    {
        sithAnimate_Stop(pAnim);
        return;
    }

    if ( (pAnim->pThing->flags & SITH_TF_DESTROYED) != 0 || (pAnim->pThing->flags & SITH_TF_DYING) != 0 )
    {
        sithAnimate_Stop(pAnim);
        return;
    }

    // Calculate new width
    curWidth = (D3DVAL(pAnim->deltaVector.x) * secDeltaTime) + pAnim->curVector.x;
    pAnim->curVector.x = curWidth;
    if ( pAnim->deltaVector.x < 0.0f )
    {
        if ( curWidth <= pAnim->endVector.x )
        {
            pAnim->curVector.x = pAnim->endVector.x;
            bEndWidth = 1;
        }
    }
    else if ( curWidth >= pAnim->endVector.x )
    {
        pAnim->curVector.x = pAnim->endVector.x;
        bEndWidth = 1;
    }


    // Calculate new height
    curHeight = (D3DVAL(pAnim->deltaVector.y) * secDeltaTime) + pAnim->curVector.y;
    pAnim->curVector.y = curHeight;
    if ( pAnim->deltaVector.y < 0.0f )
    {
        if ( curHeight <= pAnim->endVector.y )
        {
            pAnim->curVector.y = pAnim->endVector.y;
            bHeightAtTarget = 1;
        }
    }
    else if ( curHeight >= pAnim->endVector.y )
    {
        pAnim->curVector.y = pAnim->endVector.y;
        bHeightAtTarget = 1;
    }


    // Calculate new alpha
    curAlpha = (D3DVAL(pAnim->deltaVector.z) * secDeltaTime) + pAnim->curVector.z;
    pAnim->curVector.z = curAlpha;
    if ( pAnim->deltaVector.z < 0.0f )
    {
        if ( curAlpha <= pAnim->endVector.z )
        {
            pAnim->curVector.z = pAnim->endVector.z;
            bAlphaDone = 1;
        }
    }
    else if ( curAlpha >= pAnim->endVector.z )
    {
        pAnim->curVector.z = pAnim->endVector.z;
        bAlphaDone = 1;
    }

    pAnim->pThing->thingInfo.spriteInfo.width  = pAnim->curVector.x;
    pAnim->pThing->thingInfo.spriteInfo.height = pAnim->curVector.y;
    pAnim->pThing->thingInfo.spriteInfo.alpha  = pAnim->curVector.z;

    if ( bEndWidth && bHeightAtTarget && bAlphaDone )
    {
        sithAnimate_Stop(pAnim);
    }
}
#pragma optimize("", on)

void J3DAPI sithAnimate_UpdateLightAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    rdVector4 newLt;
    int bRedEnd = 0;
    int bGreenEnd = 0;
    int bBlueLast = 0;
    int bCurRangeEnd = 0;

    // Calculate new light color
    rdVector_ScaleAdd4(&newLt, &pAnim->deltaVector, secDeltaTime, &pAnim->curVector);

    if ( pAnim->deltaVector.red == 0.0f )
    {
        bRedEnd = 1;
    }

    if ( pAnim->deltaVector.green == 0.0f )
    {
        bGreenEnd = 1;
    }

    if ( pAnim->deltaVector.blue == 0.0f )
    {
        bBlueLast = 1;
    }

    if ( pAnim->deltaVector.w == 0.0f )
    {
        bCurRangeEnd = 1;
    }

    if ( pAnim->deltaVector.red < 0.0f && newLt.red < pAnim->endVector.red )
    {
        newLt.red = pAnim->endVector.red;
        bRedEnd = 1;
    }
    else if ( pAnim->deltaVector.red > 0.0f && newLt.red > pAnim->endVector.red )
    {
        newLt.red = pAnim->endVector.red;
        bRedEnd = 1;
    }

    if ( pAnim->deltaVector.green < 0.0f && newLt.green < pAnim->endVector.green )
    {
        newLt.green = pAnim->endVector.green;
        bGreenEnd = 1;
    }
    else if ( pAnim->deltaVector.green > 0.0f && newLt.green > pAnim->endVector.green )
    {
        newLt.green = pAnim->endVector.green;
        bGreenEnd = 1;
    }

    if ( pAnim->deltaVector.blue < 0.0f && newLt.blue < pAnim->endVector.blue )
    {
        newLt.blue = pAnim->endVector.blue;
        bBlueLast = 1;
    }
    else if ( pAnim->deltaVector.blue > 0.0f && newLt.blue > pAnim->endVector.blue )
    {
        newLt.blue = pAnim->endVector.blue;
        bBlueLast = 1;
    }

    // Check if light range is in range
    if ( pAnim->deltaVector.w < 0.0f && newLt.w < pAnim->endVector.w )
    {
        newLt.w = pAnim->endVector.w;
        bCurRangeEnd = 1;
    }
    else if ( pAnim->deltaVector.w > 0.0f && newLt.w > pAnim->endVector.w )
    {
        newLt.w = pAnim->endVector.w;
        bCurRangeEnd = 1;
    }

    rdVector_Copy4(&pAnim->curVector, &newLt);

    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 && pAnim->pSurface )
    {
        rdVector_Copy4(&pAnim->pSurface->face.extraLight, &newLt);
    }
    else if ( (pAnim->flags & SITHANIMATE_THING) != 0 && pAnim->pThing )
    {
        rdVector_Copy4(&pAnim->pThing->light.color, &newLt);
        pAnim->pThing->light.minRadius = newLt.w;
        pAnim->pThing->light.maxRadius = 1.1f * newLt.w;
    }
    else if ( (pAnim->flags & SITHANIMATE_SECTOR) != 0 && pAnim->pSector )
    {
        rdVector_Copy4(&pAnim->pSector->extraLight, &newLt);
    }

    if ( bRedEnd && bGreenEnd && bBlueLast && bCurRangeEnd )
    {
        if ( (pAnim->flags & SITHANIMATE_LOOP) != 0 )
        {
            rdVector4 newStartVec;

            pAnim->deltaVector.red   = -pAnim->deltaVector.red;
            pAnim->deltaVector.green = -pAnim->deltaVector.green;
            pAnim->deltaVector.blue  = -pAnim->deltaVector.blue;
            pAnim->deltaVector.w     = -pAnim->deltaVector.w;

            rdVector_Copy4(&newStartVec, &pAnim->endVector);
            rdVector_Copy4(&pAnim->endVector, &pAnim->startVector);
            rdVector_Copy4(&pAnim->startVector, &newStartVec);
        }
        else
        {
            sithAnimate_Stop(pAnim);
        }
    }
}

void J3DAPI sithAnimate_UpdateThingMoveAnim(SithAnimationSlot* pAnim, float secTimeDelta)
{
    float currentdistanceMoved;
    int bFinish = 0;
    float moveDist;
    float newDist;

    if ( pAnim->deltaValue == 0.0f )
    {
        bFinish = 1;
    }

    moveDist = D3DVAL(pAnim->deltaValue) * secTimeDelta;
    newDist  = pAnim->curValue + moveDist;
    if ( newDist > pAnim->endValue )
    {
        moveDist = pAnim->endValue - pAnim->curValue;
    }

    currentdistanceMoved = 0.0f;
    if ( ((moveDist < 0.0f ? -moveDist : moveDist) > 0.00001f ? moveDist : 0.0f) != 0.0f )
    {
        currentdistanceMoved = sithCollision_MoveThing(pAnim->pThing, &pAnim->direction3, moveDist, 0);
        if ( currentdistanceMoved == 0.0f )
        {
            // Thing couldn't move to the new location, stop animation
            sithAnimate_Stop(pAnim);

            if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
            {
                sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_BLOCKED);
            }

            if ( pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
            {
                pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            }

            return;
        }
    }

    else
    {
        bFinish = 1;
    }

    // Update current distance
    pAnim->curValue += currentdistanceMoved;

    if ( pAnim->curValue >= pAnim->endValue || bFinish )
    {
        sithAnimate_Stop(pAnim);

        if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_ARRIVED);
        }

        if ( pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }
    }
}

void J3DAPI sithAnimate_UpdateThingMovePosAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    float distance;
    float distTravelled;
    float moveDist;

    // Update current distance
    pAnim->direction3.x = pAnim->thingEndPosition.x - pAnim->pThing->pos.x;
    pAnim->direction3.y = pAnim->thingEndPosition.y - pAnim->pThing->pos.y;
    pAnim->direction3.z = pAnim->thingEndPosition.z - pAnim->pThing->pos.z;

    distance = rdVector_Normalize3Acc(&pAnim->direction3);

    if ( stdMath_ClipNearZero(distance) != 0.0f && stdMath_ClipNearZero(D3DVAL(pAnim->secTimeRemaining)) != 0.0f )
    {
        moveDist = distance / D3DVAL(pAnim->secTimeRemaining) * secDeltaTime;
        if ( (distTravelled = sithCollision_MoveThing(pAnim->pThing, &pAnim->direction3, moveDist, 0)) == 0.0f )
        {
            // Thing couldn't move to the new position, stop animation

            if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
            {
                sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_BLOCKED);
            }

            if ( (pAnim->pThing->type == SITH_THING_PLAYER || pAnim->pThing->type == SITH_THING_ACTOR) && pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
            {
                pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
            }

            sithAnimate_Stop(pAnim);
            return;
        }

        // Update remaining time
        pAnim->secTimeRemaining -= secDeltaTime;
        pAnim->secTimeRemaining = J3DMAX(0.0f, D3DVAL(pAnim->secTimeRemaining));
    }
    else
    {
        // Thing arrived at position, finish animation

        if ( (pAnim->pThing->flags & SITH_TF_COGLINKED) != 0 )
        {
            sithCog_ThingSendMessage(pAnim->pThing, 0, SITHCOG_MSG_ARRIVED);
        }

        if ( (pAnim->pThing->type == SITH_THING_PLAYER || pAnim->pThing->type == SITH_THING_ACTOR) && pAnim->pThing->thingInfo.actorInfo.bControlsDisabled == 1 )
        {
            pAnim->pThing->thingInfo.actorInfo.bControlsDisabled = 0;
        }

        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithAnimate_UpdateThingFadeAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    int bRedFinish = 0;  // unreferenced; the debug build's frame keeps its slot
    int bEndGreen = 0;  // unreferenced; the debug build's frame keeps its slot
    int bBlueFinish = 0;  // unreferenced; the debug build's frame keeps its slot
    int bCurFinish = 0;
    int bCurRangeFinish = 0;  // unreferenced; the debug build's frame keeps its slot

    rdVector4 newColor;
    if ( pAnim->pThing->type == SITH_THING_POLYLINE )
    {
        rdVector_Copy4(&newColor, &pAnim->startVector);
    }

    newColor.alpha = D3DVAL(pAnim->deltaVector.alpha) * secDeltaTime + pAnim->curVector.alpha;
    if ( pAnim->deltaVector.alpha == 0.0f )
    {
        bCurFinish = 1;
    }

    if ( pAnim->deltaVector.alpha < 0.0f && newColor.alpha <= pAnim->endVector.alpha )
    {
        newColor.alpha = pAnim->endVector.alpha;
        bCurFinish = 1;
    }
    else if ( pAnim->deltaVector.alpha > 0.0f && newColor.alpha >= pAnim->endVector.alpha )
    {
        newColor.alpha = pAnim->endVector.alpha;
        bCurFinish = 1;
    }

    // Update cur fade state
    rdVector_Copy4(&pAnim->curVector, &newColor);

    // Set thing new alpha
    pAnim->pThing->alpha = newColor.alpha;

    if ( bCurFinish )
    {
        if ( (pAnim->flags & SITHANIMATE_LOOP) != 0 )
        {
            rdVector4 newStartVec;

            // Loop the animation around
            pAnim->deltaVector.alpha = -pAnim->deltaVector.alpha;

            rdVector_Copy4(&newStartVec, &pAnim->endVector);
            rdVector_Copy4(&pAnim->endVector, &pAnim->startVector);
            rdVector_Copy4(&pAnim->startVector, &newStartVec);
        }
        else
        {
            sithAnimate_Stop(pAnim);
        }
    }
}

void J3DAPI sithAnimate_UpdateSurfaceScrollAnim(SithAnimationSlot* pAnim, float secDeltaTime, int animNum)
{
#line 1960
    SITH_ASSERTREL(pAnim && (pAnim->pSurface));

    if ( !pAnim->pSurface )
    {
        return;
    }

    // Shift the texture coordinates
    pAnim->pSurface->face.texVertOffset.x += D3DVAL(pAnim->direction2.x) * secDeltaTime;
    pAnim->pSurface->face.texVertOffset.y += D3DVAL(pAnim->direction2.y) * secDeltaTime;
    if ( ((animNum + sithMain_frameNumber) & 15) == 0 ) // every 16th frame
    {
        // Wrap texture coordinates
        pAnim->pSurface->face.texVertOffset.x = D3DVAL(fmod(pAnim->pSurface->face.texVertOffset.x, 1024.0f));
        pAnim->pSurface->face.texVertOffset.y = D3DVAL(fmod(pAnim->pSurface->face.texVertOffset.y, 1024.0f));
    }
}

void J3DAPI sithAnimate_UpdateSlideSkyAnim(SithAnimationSlot* pAnim, int skyType, float secDeltaTime, int animNum)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;

#line 1994
    SITH_ASSERTREL(pAnim);
#line 1997
    SITH_ASSERTREL(skyType == SITH_SURFACE_HORIZONSKY || skyType == SITH_SURFACE_CEILINGSKY);


    if ( skyType == SITH_SURFACE_HORIZONSKY )
    {
        pWorld->horizonSkyOffset.x += D3DVAL(pAnim->direction2.x) * secDeltaTime;
        pWorld->horizonSkyOffset.y += D3DVAL(pAnim->direction2.y) * secDeltaTime;
        if ( ((animNum + sithMain_frameNumber) & 15) == 0 )
        {
            // Wrap texture coordinates
            pWorld->horizonSkyOffset.x = D3DVAL(fmod(pWorld->horizonSkyOffset.x, 1024.0f));
            pWorld->horizonSkyOffset.y = D3DVAL(fmod(pWorld->horizonSkyOffset.y, 1024.0f));
        }
    }
    else
    {
        pWorld->ceilingSkyOffset.x += D3DVAL(pAnim->direction2.x) * secDeltaTime;
        pWorld->ceilingSkyOffset.y += D3DVAL(pAnim->direction2.y) * secDeltaTime;
        if ( ((animNum + sithMain_frameNumber) & 15) == 0 )
        {
            // Wrap texture coordinates
            pWorld->ceilingSkyOffset.x = D3DVAL(fmod(pWorld->ceilingSkyOffset.x, 1024.0f));
            pWorld->ceilingSkyOffset.y = D3DVAL(fmod(pWorld->ceilingSkyOffset.y, 1024.0f));
        }
    }
}

void J3DAPI sithAnimate_UpdatePageFlipAnim(SithAnimationSlot* pAnim)
{
    uint32_t numFrames;
    int bFinish = 0;
    uint32_t msecDeltaTime = sithTime_msecGameTime - pAnim->msecNextFrameTime;

#line 2050
    SITH_ASSERTREL(pAnim);
#line 2051
    SITH_ASSERTREL(pAnim->msecPerFrame && pAnim->pMaterial);

    if ( !pAnim->msecPerFrame || !pAnim->pMaterial )
    {
        return;
    }

    numFrames = msecDeltaTime / pAnim->msecPerFrame + 1;
    pAnim->curFrame += numFrames;
    pAnim->msecNextFrameTime = sithTime_msecGameTime + pAnim->msecPerFrame - msecDeltaTime % pAnim->msecPerFrame;

    if ( pAnim->curFrame >= pAnim->pMaterial->numCels )
    {
        if ( (pAnim->flags & SITHANIMATE_LOOP) != 0 )
        {
            size_t startFrame = 0;
            if ( (pAnim->flags & SITHANIMATE_START_SECOND_FRAME) != 0 )
            {
                startFrame = 1;
            }
            else if ( (pAnim->flags & SITHANIMATE_START_THIRD_FRAME) != 0 )
            {
                startFrame = 2;
            }

            pAnim->curFrame = (pAnim->curFrame - startFrame) % (pAnim->pMaterial->numCels - startFrame) + startFrame;
            pAnim->curFrame = STDMATH_CLAMP(pAnim->curFrame, 0, pAnim->pMaterial->numCels - 1);
        }
        else
        {
            if ( (pAnim->flags & SITHANIMATE_FINISH_FIRST_FRAME) != 0 )
            {
                pAnim->curFrame = 0;
                bFinish = 1;
            }
            else
            {
                pAnim->curFrame = pAnim->pMaterial->numCels - 1;
                bFinish = 1;
            }
        }
    }

    if ( (pAnim->flags & SITHANIMATE_SPRITE) != 0 )
    {
#line 2097
        SITH_ASSERTREL(pAnim->pThing);
        pAnim->pThing->renderData.matCelNum = pAnim->curFrame;
    }
    else if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
    {
#line 2102
        SITH_ASSERTREL(pAnim->pSurface);
#line 2105
        SITH_ASSERTREL(pAnim->curFrame < pAnim->pSurface->face.pMaterial->numCels);

        pAnim->pSurface->face.matCelNum = pAnim->curFrame;
        pAnim->pSurface->flags |= SITH_SURFACE_SYNC;
    }
    else if ( (pAnim->flags & SITHANIMATE_MATERIAL) != 0 )
    {
#line 2113
        SITH_ASSERT(pAnim->pMaterial);
#line 2114
        SITH_ASSERTREL(pAnim->curFrame < pAnim->pMaterial->numCels);
        pAnim->pMaterial->curCelNum = pAnim->curFrame;
    }
    else
    {
#line 2121
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unexpected flags for pageflip animation, 0x%x.\n", pAnim->flags, 0, 0, 0);
    }

    if ( bFinish )
    {
        sithAnimate_FreeAnim(pAnim);
    }
}

void J3DAPI sithAnimate_FreeAllAnims(void)
{
    int i;
    memset(aAnims, 0, sizeof(aAnims));

    for ( i = STD_ARRAYLEN(aFreeAnimNums) - 1; i >= 0; --i )
    {
        aFreeAnimNums[i] = STD_ARRAYLEN(aFreeAnimNums) - i - 1;
    }

    numFreeAnims    = STD_ARRAYLEN(aFreeAnimNums);
    lastUsedAnimIndex = 0;
}

void J3DAPI sithAnimate_FreeAnim(SithAnimationSlot* pAnim)
{
    size_t animNum = pAnim - &aAnims[0];
    int i;
#line 2182
    SITH_ASSERTREL(animNum < 512);

#line 2183
    SITH_ASSERTREL(numFreeAnims < 512);

    pAnim->flags = 0;
    aFreeAnimNums[numFreeAnims++] = animNum;

    if ( (int)animNum == lastUsedAnimIndex )
    {
        for ( i = (int)animNum - 1; i >= 0; --i )
        {
            if ( aAnims[i].flags )
            {
                break;
            }
        }

        lastUsedAnimIndex = i;
    }
}

SithAnimationSlot* J3DAPI sithAnimate_NewAnim(void)
{
    size_t animNum;

    if ( numFreeAnims == 0 )
    {
        return NULL;
    }

    animNum = aFreeAnimNums[--numFreeAnims];
    if ( (int)animNum > lastUsedAnimIndex )
    {
        lastUsedAnimIndex = animNum;
    }

    // Reset animation
    memset(&aAnims[animNum], 0, sizeof(SithAnimationSlot));

    // Set animation ID
    aAnims[animNum].animID = ((sithPlayer_playerNum + 1) << 16) | (animNum & 0xFFFF);

    return &aAnims[animNum];
}

void J3DAPI sithAnimate_UpdatePushItemAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    float frameDt;
    rdVector3 motionDirection;
    float moveDistance;
    float distOffset;
    int bCurFinish = 0;
    float distancePushed;
    float curFaceDrag;
    float activeFrm = sithAnimate_GetPuppetTrackCurFrame(pAnim->pThing, pAnim->trackNum);

    if ( activeFrm < pAnim->deltaValue && (moveDistance = pAnim->endValue - pAnim->endVector.x) > 0.0f ) // deltaValue holds last processed keyframe frame
    {
        // Animation completed

        // Make item pushable by removing it's surface drag
        curFaceDrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
        pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

        // One final item push
        distancePushed = sithCollision_MoveThing(pAnim->pItemThing, &motionDirection, moveDistance, 0);

        // Restore item surface drag, and mark animation as finished
        pAnim->pItemThing->moveInfo.physics.surfDrag = curFaceDrag;
        bCurFinish = 1;
    }
    else
    {
        size_t frameIdx;
        size_t keyFrm;

        pAnim->deltaValue = activeFrm;
        frameIdx = sithAnimate_GetAnimFrameIndex(activeFrm, pushItemAnimFrames.lastFrameIdx, &pushItemAnimFrames.aFrames[1]);
        keyFrm = pushItemAnimFrames.aFrames[frameIdx + 1];

        if ( pushItemAnimFrames.aFrames[pushItemAnimFrames.lastFrameIdx] < activeFrm )
        {
            // Animation completed
            bCurFinish = 1;
        }
        else
        {
            float distDuration;
            float distance;

            distance   = aPushItemDistances[2 * frameIdx];
            frameDt = D3DVAL(activeFrm - D3DVAL(keyFrm));
            distOffset = RD_MULTIPLY(aPushItemDistances[2 * frameIdx + 1], frameDt);
            distance   = distance + distOffset;
            distDuration  = distance - pAnim->endVector.x; // endVector.x holds current distance item was pushed for

            rdVector_Copy3(&motionDirection, &pAnim->direction3);
            if ( distDuration > 0.0f )
            {
                if ( distDuration + pAnim->endVector.x > pAnim->endValue ) // endVector.x holds current distance item was pushed for
                {
                    // Animation completed, but we need to push item one last time to it's final position
                    bCurFinish = 1;
                    moveDistance = distDuration + pAnim->endVector.x - pAnim->endValue;
                }
                else
                {
                    moveDistance = distDuration;
                }

                // Push item
                if ( pAnim->pItemThing )
                {
                    // Make item pushable by removing it's surface drag
                    curFaceDrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
                    pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

                    distancePushed = sithCollision_MoveThing(pAnim->pItemThing, &motionDirection, moveDistance, 0);
                    pAnim->endVector.x += distancePushed; // endVector.x holds current distance item was pushed for

                    // Restore item surface drag
                    pAnim->pItemThing->moveInfo.physics.surfDrag = curFaceDrag;
                }
            }
        }
    }

    if ( pAnim->endVector.x >= pAnim->endValue || bCurFinish )// endVector.x holds current distance item was pushed for
    {
        // Animation completed

        pAnim->pItemThing->collide.type = pAnim->msecPerFrame;
        sithAnimate_FindAndEnterFloorSurface(pAnim->pItemThing, pAnim->pSurface);// pSurface is item thing start surface

        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithAnimate_UpdatePullItemAnim(SithAnimationSlot* pAnim, float secDeltaTime)
{
    rdVector3 moveDir;
    float moveDist;
    float frameDelta;
    float distOffset;
    int bFinish = 0;
    float surfdrag;
    float distPulled;
    float frame = sithAnimate_GetPuppetTrackCurFrame(pAnim->pThing, pAnim->trackNum);

    if ( frame < pAnim->deltaValue && (moveDist = pAnim->endValue - pAnim->endVector.x) > 0.0f ) // deltaValue holds current keyframe animation frame
    {
        // Animation completed

        // Make item pushable by removing it's surface drag
        surfdrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
        pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

        // One final item pull
        distPulled = sithCollision_MoveThing(pAnim->pItemThing, &moveDir, moveDist, 0);

        // Restore item surface drag, and mark animation as finished
        pAnim->pItemThing->moveInfo.physics.surfDrag = surfdrag;
        bFinish = 1;
    }
    else
    {
        size_t frameIdx;
        size_t keyFrame;

        pAnim->deltaValue = frame;
        frameIdx = sithAnimate_GetAnimFrameIndex(frame, pullItemAnimFrames.lastFrameIdx, &pullItemAnimFrames.aFrames[1]);
        keyFrame = pullItemAnimFrames.aFrames[frameIdx + 1];
        if ( pullItemAnimFrames.aFrames[pullItemAnimFrames.lastFrameIdx] < frame )
        {
            // Animation completed
            bFinish = 1;
        }
        else
        {
            float distDelta;
            float distance;

            distance   = aPullItemDistances[2 * frameIdx];
            frameDelta = D3DVAL(frame - D3DVAL(keyFrame));
            distOffset = RD_MULTIPLY(aPullItemDistances[2 * frameIdx + 1], frameDelta);
            distance   = D3DVAL(fabs(distance += distOffset));
            distDelta  = distance - pAnim->endVector.x; // endVector.x holds current distance item was pulled for

            rdVector_Copy3(&moveDir, &pAnim->direction3);
            if ( distDelta > 0.0f )
            {
                if ( distDelta + pAnim->endVector.x > pAnim->endValue )// endVector.x holds current distance item was pulled for
                {
                    bFinish = 1;
                    moveDist = distDelta + pAnim->endVector.x - pAnim->endValue;
                }
                else
                {
                    moveDist = distDelta;
                }

                // Pull item
                if ( pAnim->pItemThing )
                {
                    // Make item pullable by removing it's surface drag
                    surfdrag = pAnim->pItemThing->moveInfo.physics.surfDrag;
                    pAnim->pItemThing->moveInfo.physics.surfDrag = 0.0f;

                    distPulled = sithCollision_MoveThing(pAnim->pItemThing, &moveDir, moveDist, 0);
                    pAnim->endVector.x += distPulled; // endVector.x holds current distance item was pulled for

                    // Restore item surface drag
                    pAnim->pItemThing->moveInfo.physics.surfDrag = surfdrag;
                }
            }
        }
    }

    if ( pAnim->endVector.x >= pAnim->endValue || bFinish ) // endVector.x holds current distance item was pulled for
    {
        // Animation completed

        pAnim->pItemThing->collide.type = pAnim->msecPerFrame;
        sithAnimate_FindAndEnterFloorSurface(pAnim->pItemThing, pAnim->pSurface); // pSurface is item thing start surface

        sithAnimate_Stop(pAnim);
    }
}

size_t J3DAPI sithAnimate_GetAnimFrameIndex(float frame, size_t size, const size_t* aAnimFrames)
{
    size_t curframeNum = (size_t)frame;
    size_t index = 0;
    size_t i;
    for ( i = 1; i < size; ++i )
    {
        if ( aAnimFrames[i] > curframeNum )
        {
            break;
        }

        index = i;
    }

    return index;
}

float J3DAPI sithAnimate_GetPuppetTrackCurFrame(SithThing* pThing, int trackNum)
{
    float curFrame = pThing->renderData.pPuppet->aTracks[trackNum].curFrame;
    return curFrame;
}

SithSurface* J3DAPI sithAnimate_GetThingFloorSurface(const SithThing* pThing)
{

    rdVector3 moveNorm;
    float moveDelta;
    float radius;
    SithCollision* pCollision;
    SithSurface* pSurface;
    int searchFlags = 0x10;

    rdVector_Neg3(&moveNorm, &rdroid_zVector3);
    moveDelta = pThing->collide.movesize * 4.0f;
    radius = pThing->collide.movesize / 2.0f;

    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDelta, radius, searchFlags | 0x2802);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            pSurface = pCollision->pSurfaceCollided;
            sithCollision_DecreaseStackLevel();
            return pSurface;
        }
    }

    sithCollision_DecreaseStackLevel();

    return NULL;
}

void J3DAPI sithAnimate_ExitFloorSurface(const SithThing* pThing, const SithSurface* pSurface)
{
    if ( pSurface && pThing && (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 )
    {
        sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_EXITED);
    }
}

void J3DAPI sithAnimate_FindAndEnterFloorSurface(const SithThing* pThing, const SithSurface* pPrevSurf)
{
    rdVector3 moveNorm;
    float moveDelta;
    float radius;
    SithCollision* pCollision;
    SithSurface* pSurface;
    int searchFlags = 0x10;

    rdVector_Neg3(&moveNorm, &rdroid_zVector3);
    moveDelta = pThing->collide.movesize * 4.0f;
    radius = pThing->collide.movesize / 2.0f;

    pSurface = NULL;
    sithCollision_SearchForCollisions(pThing->pInSector, NULL, &pThing->pos, &moveNorm, moveDelta, radius, searchFlags | 0x2802);

    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_WORLD) != 0 )
        {
            pSurface = pCollision->pSurfaceCollided;
            if ( pSurface != pPrevSurf )
            {
                break;
            }
        }
    }

    sithCollision_DecreaseStackLevel();

    // Send entered message to found surface
    if ( pSurface )
    {
        if ( (pSurface->flags & SITH_SURFACE_COGLINKED) != 0 )
        {
            sithCog_SurfaceSendMessage(pSurface, pThing, SITHCOG_MSG_ENTERED);
        }
    }
}
