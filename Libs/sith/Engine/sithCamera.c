/* sithTime.h first: VC5 folds the earlier-declared global as the fmul memory
   operand, and retail folds sithTime_frameTimeFlex against sithCamera_camInfo
 */
#include <sith/Gameplay/sithTime.h>
#include "sithCamera.h"

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Math/rdMatrix.h>
/* Retail calls rdVector_Len3 out of line from both camera fade paths but expands
   the other helpers (Dot3 in Update's roll) inline. */
#define RDVECTOR_NO_INLINE_LEN3
#include <rdroid/Math/rdVector.h>
#undef RDVECTOR_NO_INLINE_LEN3

#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithRender.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

rdVector3 sithCamera_camSpot;
SithCameraInfo sithCamera_camInfo;
rdMatrix34 sithCamera_orbCamOrient;
rdVector3 sithCamera_camLookSpot;
SithCamera sithCamera_aCameras[7];

static size_t sithCamera_aCycleCamIndices[3] = { SITHCAMERA_INTCAMERANUM, SITHCAMERA_EXTCAMERANUM, SITHCAMERA_ORBCAMERANUM };

static rdVector3 sithCamera_vecExtCameraLookOffset             = { { 0.0f }, { 0.02f }, { 0.02f } };
static rdVector3 sithCamera_vecExtCameraPrevLookOffset = { { 0.0f }, { 0.0f }, { 0.0f } };
static rdVector3 sithCamera_vecExtCameraDefaultLookOffset  = { { 0.0f }, { 0.02f }, { 0.02f } };

/* Unreferenced original storage: initialized camera offsets that nothing in the
   image reaches (whole-image dword scan of every 4-byte slot in the block). The
   values are the retail bytes; only the names are invented, after the address.
 */
static rdVector3 sithCamera_vecUnused_52A088 = { { 0.0f }, { 0.0f }, { 0.04f } };
static rdVector3 sithCamera_vecUnused_52A098 = { { 0.0f }, { -0.1f }, { 0.04f } };

static rdVector3 sithCamera_vecExtCameraOffset                 = { { 0.0f }, { -0.2f }, { 0.065f } };
static rdVector3 sithCamera_vecCameraOffsetWeaponDrawn   = { { 0.0f }, { -0.22f }, { 0.15f } };
static rdVector3 sithCamera_vecExtCameraDefaultOffset      = { { 0.0f }, { -0.2f }, { 0.065f } };

static rdVector3 sithCamera_vecUnused_52A0D8 = { { 0.0f }, { -0.2f }, { 0.125f } };
static rdVector3 sithCamera_vecUnused_52A0E8 = { { 0.01f }, { -0.002f }, { 0.02f } };
static rdVector3 sithCamera_vecUnused_52A0F8 = { { 0.01f }, { 0.002f }, { 0.02f } };

static rdVector3 sithCamera_vecCameraOffsetJeep          = { { 0.0f }, { -0.2f }, { 0.14f } };

static rdVector3 sithCamera_vecUnused_52A118 = { { 0.0f }, { -0.22f }, { 0.055f } };

static rdVector3 sithCamera_vecCameraOffsetClimbingUp    = { { 0.0f }, { -0.2f }, { -0.01f } };
static rdVector3 sithCamera_vecCameraOffsetClimbDown     = { { 0.0f }, { -0.2f }, { 0.165f } };
static rdVector3 sithCamera_vecCameraOffsetClimbingLeft  = { { 0.1f }, { -0.2f }, { 0.065f } };
static rdVector3 sithCamera_vecCameraOffsetClimbingRight = { { -0.1f }, { -0.2f }, { 0.065f } };
static rdVector3 sithCamera_vecCameraOffsetSlideForward  = { { 0.165f }, { 0.01f }, { 0.17f } };
static rdVector3 sithCamera_vecCameraOffsetSlideBackward = { { 0.165f }, { -0.2f }, { 0.03f } };
static rdVector3 sithCamera_vecCameraOffsetJumpRollback  = { { 0.0f }, { -0.3f }, { 0.065f } };

static rdVector3 sithCamera_vecOffsetCamType40  = { { 0.0f }, { 1.0f }, { 1.0f } };

/* Unreferenced original storage: see the vector block above. */
static float sithCamera_unused_52A1A4 = 0.2f;

static rdVector3 sithCamera_idleCamOffset = { { 0.0f }, { -0.2f }, { 0.065f } };
static rdMatrix34 sithCamera_idleCamOrient = { { { 0.0f } } };
static float sithCamera_jeepInterpSpeed = 0.2f;

/* Unreferenced original storage: 28 bytes of 8-aligned .bss between
   sithCamera_idleCamOrient and sithCamera_pCurCamera that nothing in the image
   reaches (whole-image dword scan). */
static uint8_t sithCamera_aUnused_561300[28] = { 0 };

SithCamera* sithCamera_pCurCamera = NULL;
int sithCamera_bExtCameraLookMode = 0;

static float sithCamera_secFocusThingFadeInTime = 0.0f;
static int sithCamera_bFocusThingInvisible = 0;
static int sithCamera_bUpdateCameraOffset = 0;

static int bStartup = 0;
static int sithCamera_bOpen = 0;
static float sithCamera_secLookOffsetTranslateDelta = 0.0f;

SithSector* J3DAPI sithCamera_SearchSectorInRadius(int a1, SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float unused, int flags);

int J3DAPI sithCamera_Startup(void)
{
#line 159
    SITH_ASSERTREL(bStartup == 0);

    // 1st person camera
    sithCamera_NewEntry(&sithCamera_aCameras[SITHCAMERA_INTCAMERANUM], 0, SITHCAMERA_INTERNAL, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);

    // 3rd person camera
    sithCamera_NewEntry(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], 0, SITHCAMERA_EXTERNAL, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    rdVector_Copy3(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].offset, &sithCamera_vecExtCameraOffset);
    sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM].interpSpeed = 0.35f;

    sithCamera_NewEntry(&sithCamera_aCameras[SITHCAMERA_CINEMACAMERANUM], 0, SITHCAMERA_CINEMATIC, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_aCameras[SITHCAMERA_CINEMACAMERANUM].interpSpeed = 1.0f;

    // Idle camera
    sithCamera_NewEntry(&sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM], 0, SITHCAMERA_IDLE, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_NewEntry(&sithCamera_aCameras[SITHCAMERA_UNKNONWCAMERANUM], 0, SITHCAMERA_UNKNOWN_40, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);
    sithCamera_NewEntry(&sithCamera_aCameras[SITHCAMERA_ORBCAMERANUM], 0, SITHCAMERA_ORBITAL, SITHCAMERA_FOVDEFAULT, 1.0f, NULL, NULL, NULL);

    bStartup = 1;
    return 1;
}

void J3DAPI sithCamera_Shutdown(void)
{
    int i;
    if ( !bStartup )
    {
        return;
    }

    for ( i = 0; i < 7; ++i )
    {
        sithCamera_FreeEntry(&sithCamera_aCameras[i]);
    }

    bStartup = 0;
    sithCamera_pCurCamera = NULL;
}

int J3DAPI sithCamera_Open(rdCanvas* pCanvas, float aspect)
{
#line 231
    SITH_ASSERTREL(bStartup == 1);
    if ( sithCamera_bOpen )
    {
        return 0;
    }

    sithCamera_SetCameraCanvas(&sithCamera_aCameras[SITHCAMERA_INTCAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_aCameras[SITHCAMERA_CINEMACAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_aCameras[SITHCAMERA_UNKNONWCAMERANUM], aspect, pCanvas);
    sithCamera_SetCameraCanvas(&sithCamera_aCameras[SITHCAMERA_ORBCAMERANUM], aspect, pCanvas);

    sithCamera_ResetAllCameras();
    sithCamera_Update(sithCamera_pCurCamera);

    sithCamera_bOpen = 1;
    return 1;
}

void J3DAPI sithCamera_Close(void)
{
    if ( !sithCamera_bOpen )
        return;

    sithCamera_pCurCamera = NULL;
    sithCamera_bOpen        = 0;
}

void J3DAPI sithCamera_ResetAllCameras(void)
{
    sithCamera_camInfo.bCurCameraSet = 0;
    sithCamera_camInfo.stateFlags &= ~SITHCAMERA_STATE_CUTSCENE;

    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_INTCAMERANUM], sithWorld_pCurrentWorld->pCameraFocusThing, NULL);
    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], sithWorld_pCurrentWorld->pCameraFocusThing, NULL);
    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_IDLECAMERANUM], sithWorld_pCurrentWorld->pCameraFocusThing, sithWorld_pCurrentWorld->pCameraFocusThing);
    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_UNKNONWCAMERANUM], sithWorld_pCurrentWorld->pCameraFocusThing, sithWorld_pCurrentWorld->pCameraFocusThing);
    sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_ORBCAMERANUM], sithWorld_pCurrentWorld->pCameraFocusThing, sithWorld_pCurrentWorld->pCameraFocusThing);

    rdVector_Copy3(&sithCamera_vecExtCameraOffset, &sithCamera_vecExtCameraDefaultOffset);
    rdVector_Copy3(&sithCamera_vecExtCameraLookOffset, &sithCamera_vecExtCameraDefaultLookOffset);

    // Set default FOV for 3rd person camera
    sithCamera_SetCameraFOV(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], SITHCAMERA_FOVDEFAULT);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        // Set 1st person camera
        sithCamera_SetCurrentCamera(&sithCamera_aCameras[SITHCAMERA_INTCAMERANUM]);
    }
    else if ( sithPlayer_pLocalPlayerThing )
    {
        if ( !sithCamera_pCurCamera )
        {
            // Set 3rd person camera
            sithCamera_SetCameraFocus(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM], sithPlayer_pLocalPlayerThing, NULL);
            sithCamera_SetCurrentCamera(&sithCamera_aCameras[SITHCAMERA_EXTCAMERANUM]);
        }
    }
}

/* PLACEHOLDER for a function the retail linker discarded (/OPT:REF). Nothing in the image
   references it, so its own bytes are gone, but its "Error allocating memory for camera.\n"
   literal survives at 0x0052A204 inside this TU's /Gf string block, because rdCamera_New
   references the same pooled string. The body is cut to that literal's use (minimal placeholder
   for a discarded function). Its position between ResetAllCameras and NewEntry is what places the
   literal between "bStartup == 1" and "Warning: attempt to free NULL camera ptr.\n" in the
   string pool, which is retail's order. */
SithCamera* J3DAPI sithCamera_New(SithCameraType type, uint32_t a2, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus)
{
    SithCamera* pCamera;

#line 369
    pCamera = (SithCamera*)STDMALLOC(sizeof(SithCamera));
    if ( !pCamera )
    {
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for camera.\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( sithCamera_NewEntry(pCamera, a2, type, fov, aspect, pCanvas, pPrimaryFocus, pSecondaryFocus) )
    {
        return pCamera;
    }

    return NULL;
}

int J3DAPI sithCamera_NewEntry(SithCamera* pCamera, uint32_t a2, SithCameraType type, float fov, float aspect, rdCanvas* pCanvas, SithThing* pPrimaryFocus, SithThing* pSecondaryFocus)
{
    pCamera->type                 = type;
    pCamera->unknown1             = a2;
    pCamera->fov                  = fov;
    pCamera->aspectRatio          = aspect;
    pCamera->pPrimaryFocusThing   = pPrimaryFocus;
    pCamera->pSecondaryFocusThing = pSecondaryFocus;
    pCamera->posInterpState       = -1;
    pCamera->lookInterpState      = -1;
    pCamera->focusInterpState     = -1;

    rdCamera_NewEntry(&pCamera->rdCamera, fov, 0, 0.015625f, 64.0f, aspect);
    rdCamera_SetAttenuation(&pCamera->rdCamera, SITHCAMERA_ATTENUATIONMIN, SITHCAMERA_ATTENUATIONMAX);

    if ( pCanvas )
    {
        rdCamera_SetCanvas(&pCamera->rdCamera, pCanvas);
    }

    memset(&pCamera->lookPos, 0, sizeof(pCamera->lookPos));
    memset(&pCamera->lookPYR, 0, sizeof(pCamera->lookPYR));

    rdMatrix_Copy34(&pCamera->orient, &rdroid_identMatrix34);

    memset(&pCamera->offset, 0, sizeof(pCamera->offset));
    memset(&pCamera->vecUnknown1, 0, sizeof(pCamera->vecUnknown1));

    pCamera->bLookInterp = 0;
    pCamera->bPosInterp = 0;
    return 1;
}

/* PLACEHOLDER, same reading as sithCamera_New above: discarded by /OPT:REF, but its
   "Warning: attempt to free NULL camera ptr.\n" literal survives at 0x0052A22C because
   rdCamera_Free references the same pooled string. Placed before FreeEntry so the literal
   follows sithCamera_New's and precedes "pCamera!=((void *)0)", as retail's pool does. */
void J3DAPI sithCamera_Free(SithCamera* pCamera)
{
    if ( pCamera )
    {
        sithCamera_FreeEntry(pCamera);
        stdMemory_Free(pCamera);
    }
    else
    {
#line 471
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: attempt to free NULL camera ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI sithCamera_FreeEntry(SithCamera* pCamera)
{
#line 490
    SITH_ASSERTREL(pCamera!=NULL);
    rdCamera_FreeEntry(&pCamera->rdCamera);
}

void J3DAPI sithCamera_SetCameraCanvas(SithCamera* pCamera, float aspect, rdCanvas* pCanvas)
{
    rdCamera_SetCanvas(&pCamera->rdCamera, pCanvas);
    if ( pCamera->type == SITHCAMERA_EXTERNAL )
    {
        pCamera->focusInterpState = -1;
    }

    rdCamera_SetAspectRatio(&pCamera->rdCamera, aspect);
    rdCamera_SetAttenuation(&pCamera->rdCamera, SITHCAMERA_ATTENUATIONMIN, SITHCAMERA_ATTENUATIONMAX);
}

int J3DAPI sithCamera_SetCurrentCamera(SithCamera* pCamera)
{
#line 551
    SITH_ASSERTREL(pCamera!=NULL);

    sithCamera_pCurCamera   = pCamera;
    sithCamera_camInfo.bCurCameraSet = 1;

    rdCamera_SetCurrent(&sithCamera_pCurCamera->rdCamera);
    switch ( pCamera->type )
    {
        case SITHCAMERA_INTERNAL:
            sithCamera_camInfo.curCycleCamNum = 0;
            break;

        case SITHCAMERA_EXTERNAL:
            sithCamera_camInfo.curCycleCamNum = 1;
            break;

        case SITHCAMERA_ORBITAL:
            sithCamera_camInfo.curCycleCamNum = 2;
            break;

        case SITHCAMERA_IDLE:
        {
            rdVector3 vecPYR;
            rdMatrix_Copy34(&sithCamera_idleCamOrient, &sithCamera_pCurCamera->pPrimaryFocusThing->orient);

            memset(&vecPYR, 0, sizeof(vecPYR));
            vecPYR.y = -45.0f;
            rdMatrix_PostRotate34(&sithCamera_idleCamOrient, &vecPYR);
            break;
        }

        default:
            break;
    }

    sithCamera_Update(sithCamera_pCurCamera);
    return 1;
}

void J3DAPI sithCamera_CycleCamera(void)
{
    if ( ++sithCamera_camInfo.curCycleCamNum >= STD_ARRAYLEN(sithCamera_aCycleCamIndices) )
    {
        sithCamera_camInfo.curCycleCamNum = 0;
    }

    sithCamera_SetCurrentCamera(&sithCamera_aCameras[sithCamera_aCycleCamIndices[sithCamera_camInfo.curCycleCamNum]]);
}

int J3DAPI sithCamera_SetCurrentToCycleCamera(void)
{
    return sithCamera_SetCurrentCamera(&sithCamera_aCameras[sithCamera_aCycleCamIndices[sithCamera_camInfo.curCycleCamNum]]);
}

/* Linker-discarded (/OPT:REF). Descriptive name: refuses a target camera of lower priority than the
   current one, else sets bCurCameraSet = 2. Body restored from the debug build's text. */
int J3DAPI sithCamera_RequestCameraSwitch(SithCamera* pTargetCamera, float time)
{
#line 652
    SITH_ASSERTREL(pTargetCamera != NULL);
    SITH_ASSERTREL(time >= 0.0f);
    if ( sithCamera_pCurCamera && pTargetCamera->unknown1 < (unsigned int)sithCamera_pCurCamera->unknown1 )
    {
        return 0;
    }

    sithCamera_camInfo.bCurCameraSet = 2;
    return 1;
}

void J3DAPI sithCamera_SetCameraFocus(SithCamera* pCamera, SithThing* pPrimaryFocusThing, SithThing* pSecondaryFocusThing)
{
#line 684
    SITH_ASSERTREL(pCamera);
    if ( pPrimaryFocusThing && pCamera->pPrimaryFocusThing != pPrimaryFocusThing )
    {
        pCamera->pPrimaryFocusThing = pPrimaryFocusThing;
    }

    if ( pSecondaryFocusThing )
    {
        if ( pCamera->pSecondaryFocusThing != pSecondaryFocusThing )
        {
            pCamera->pSecondaryFocusThing = pSecondaryFocusThing;
        }
    }
}

/* PLACEHOLDER pair for two functions the retail linker discarded (/OPT:REF). */
void J3DAPI sithCamera_IncrementFOV(SithCamera* pCamera)
{
#line 703
    SITH_ASSERTREL(pCamera);
    pCamera->fov = pCamera->fov + 1.0f;
    if ( pCamera->fov > SITHCAMERA_FOVMAX )
    {
        pCamera->fov = SITHCAMERA_FOVMAX;
    }

    rdCamera_SetFOV(&pCamera->rdCamera, pCamera->fov);
}

void J3DAPI sithCamera_DecrementFOV(SithCamera* pCamera)
{
#line 714
    SITH_ASSERTREL(pCamera);
    pCamera->fov = pCamera->fov - 1.0f;
    if ( pCamera->fov < SITHCAMERA_FOVMIN )
    {
        pCamera->fov = SITHCAMERA_FOVMIN;
    }

    rdCamera_SetFOV(&pCamera->rdCamera, pCamera->fov);
}

void J3DAPI sithCamera_SetCameraFOV(SithCamera* pCamera, float fov)
{
#line 726
    SITH_ASSERTREL(pCamera);
    if ( fov < SITHCAMERA_FOVMIN || fov > SITHCAMERA_FOVMAX )
        return;

    pCamera->fov = fov;
    rdCamera_SetFOV(&pCamera->rdCamera, pCamera->fov);
}

void J3DAPI sithCamera_SetCameraPosition(SithCamera* pCamera, const rdVector3* pPos)
{
#line 736
    SITH_ASSERTREL(pCamera);
    rdVector_Copy3(&pCamera->pos, pPos);
    pCamera->posInterpState   = 0;
    pCamera->focusInterpState = 0;
}

void J3DAPI sithCamera_GetCameraPosition(const SithCamera* pCamera, rdVector3* pDstPos)
{
#line 747
    SITH_ASSERTREL(pCamera);
    rdVector_Copy3(pDstPos, &pCamera->orient.dvec);
}

void J3DAPI sithCamera_SetExtCameraOffset(const rdVector3* pOffset)
{
    if ( sithCamera_pCurCamera->type != SITHCAMERA_EXTERNAL )
        return;

    rdVector_Copy3(&sithCamera_vecExtCameraOffset, pOffset);
    sithCamera_bUpdateCameraOffset = 1;
}

void J3DAPI sithCamera_SetExtCameraLookOffset(const rdVector3* pOffset)
{
    if ( sithCamera_pCurCamera->type != SITHCAMERA_EXTERNAL )
        return;

    rdVector_Copy3(&sithCamera_vecExtCameraPrevLookOffset, &sithCamera_vecExtCameraLookOffset);

    sithCamera_secLookOffsetTranslateDelta = 0.5f;
    rdVector_Copy3(&sithCamera_vecExtCameraLookOffset, pOffset);
}

void J3DAPI sithCamera_RestoreExtCamera(void)
{
    if ( sithCamera_pCurCamera->type != SITHCAMERA_EXTERNAL )
    {
        return;
    }

    rdVector_Copy3(&sithCamera_vecExtCameraOffset, &sithCamera_vecExtCameraDefaultOffset);
    rdVector_Copy3(&sithCamera_vecExtCameraLookOffset, &sithCamera_vecExtCameraDefaultLookOffset);

    sithCamera_pCurCamera->interpSpeed = 0.35f;
    sithCamera_bUpdateCameraOffset = 0;

    if ( !sithCamera_bExtCameraLookMode )
    {
        sithCamera_bFocusThingInvisible = 0;
    }
}

SithThing* J3DAPI sithCamera_GetPrimaryFocus(const SithCamera* pCamera)
{
#line 801
    SITH_ASSERTREL(pCamera!=NULL);
    return pCamera->pPrimaryFocusThing;
}

SithThing* J3DAPI sithCamera_GetSecondaryFocus(const SithCamera* pCamera)
{
#line 823
    SITH_ASSERTREL(pCamera!=NULL);
    return pCamera->pSecondaryFocusThing;
}

void J3DAPI sithCamera_Update(SithCamera *pCamera)
{
    rdVector3 curCamPos;
    float roll;
    float zDist;
    rdVector3 pos;
    rdMatrix34 newOrient;
    rdVector3 newPYR;
    float zDelta;
    float secFrameTime;
    int bFindSectorOnly;
    float xDelta;
    float angleDelta;
    float yDelta;
    rdVector3 newPos;
    rdVector3 newLookPos;
    float posDelta;
    SithThing *pThing2;
    float interpDelta;
    int curWeaponId;
    float interpSpeed;
    float interpSpeedHalf;
    SithThing *pThing1;
    SithWorld *pWorld;
    rdVector3 lookPos;
    rdVector3 vecPYR;

    secFrameTime = sithTime_frameTimeFlex;
    angleDelta = RD_MULTIPLY(sithTime_frameTimeFlex, sithCamera_camInfo.povShake.angleDelta);
    posDelta = RD_MULTIPLY(sithTime_frameTimeFlex, sithCamera_camInfo.povShake.posDelta);
    interpSpeed = pCamera->interpSpeed;
    interpSpeedHalf = interpSpeed / 2.0f;
    bFindSectorOnly = 0;
    pWorld = sithWorld_pCurrentWorld;

#line 866
    SITH_ASSERTREL(pCamera!=NULL);
    pThing1 = pCamera->pPrimaryFocusThing;
    pThing2 = pCamera->pSecondaryFocusThing;

    switch (pCamera->type)
    {
        case SITHCAMERA_INTERNAL:

#line 876
            SITH_ASSERTREL(pThing1!=NULL);

#line 877
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
            rdMatrix_Copy34(&pCamera->orient, &pThing1->orient);
            if (pThing1->moveType == SITH_MT_PATH && pThing1->renderData.paJointMatrices)
            {
                memcpy(&pCamera->orient, pThing1->renderData.paJointMatrices, sizeof(pCamera->orient));
            }
            else
            {
                if (pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER)
                {
                    rdVector_Copy3(&newPYR, &pThing1->thingInfo.actorInfo.headPYR);
                }
                else
                {
                    memset(&newPYR, 0, sizeof(newPYR));
                }

                if (pThing1->moveType == SITH_MT_PHYSICS)
                {
                    roll = RD_MULTIPLY(rdVector_Dot3(&pThing1->orient.rvec, &pThing1->moveInfo.physics.velocity), 5.0f);
                    newPYR.roll = STDMATH_CLAMP(roll, -8.0f, 8.0f);
                }

                if (pThing1 == sithPlayer_pLocalPlayerThing)
                {
                    rdVector_Add3(&newPYR, &newPYR, &sithCamera_camInfo.povShake.vecAngleOffset);
                }

                rdMatrix_PreRotate34(&pCamera->orient, &newPYR);
                rdMatrix_PostTranslate34(&pCamera->orient, &pThing1->pos);
                if (pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER)
                {
                    rdMatrix_PreTranslate34(&pCamera->orient, &pThing1->thingInfo.actorInfo.eyeOffset);
                    if (pThing1 == sithPlayer_pLocalPlayerThing)
                    {
                        rdMatrix_PreTranslate34(&pCamera->orient, &sithCamera_camInfo.povShake.vecPosOffset);
                    }
                }

                rdMatrix_Normalize34(&pCamera->orient);
            }

            pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.01f);
            break;
        case SITHCAMERA_EXTERNAL:
        case SITHCAMERA_UNKNOWN_100:
        {
            rdVector3 lookVector;
            float distToLook;

#line 935
            SITH_ASSERTREL(pThing1!=NULL);

#line 936
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));

            if (pCamera->type == SITHCAMERA_EXTERNAL || pCamera->type == SITHCAMERA_UNKNOWN_100)
            {
                if ((pThing1->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0)
                {
                    sithCamera_bExtCameraLookMode = 0;
                    pThing1->alpha = 1.0f;
                    sithCamera_RestoreExtCamera();
                    rdMatrix_LookAt(&pCamera->orient, &pCamera->lookPos, &pThing1->pos, 0.0f);

                    if (pThing1->pInSector)
                    {
                        pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->lookPos, 0.0f);
                    }

                    break;
                }

                rdVector_Copy3(&lookVector, &pCamera->lookPos);
                if (pThing1->type == SITH_THING_ACTOR || pThing1->type == SITH_THING_PLAYER)
                {
                    rdVector_Copy3(&newPYR, &pThing1->thingInfo.actorInfo.headPYR);
                }
                else
                {
                    memset(&newPYR, 0, sizeof(newPYR));
                }

                if (pThing1 == sithPlayer_pLocalPlayerThing)
                {
                    newPYR.x = newPYR.x + sithCamera_camInfo.povShake.vecAngleOffset.pitch;
                    newPYR.y = newPYR.y + sithCamera_camInfo.povShake.vecAngleOffset.yaw;
                    newPYR.z = newPYR.z + sithCamera_camInfo.povShake.vecAngleOffset.roll;
                }

                if (pThing1 == sithPlayer_pLocalPlayerThing && !sithCamera_bExtCameraLookMode && !sithCamera_bUpdateCameraOffset)
                {
                    if (pThing1->moveStatus != SITHPLAYERMOVE_SWIMIDLE && (pThing1->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 && (pThing1->pInSector->flags & SITH_SECTOR_AETHERIUM) == 0)
                    {
                        if ((pThing1->moveInfo.physics.flags & SITH_PF_JEEP) != 0 && pThing1->moveStatus != SITHPLAYERMOVE_JEEP_BOARDING)
                        {
                            interpSpeed = sithCamera_jeepInterpSpeed;
                            rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetJeep);
                        }
                        else
                        {
                            switch (pThing1->moveStatus)
                            {
                                case SITHPLAYERMOVE_CLIMBING_UP:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetClimbingUp);
                                    break;

                                case SITHPLAYERMOVE_CLIMBING_DOWN:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetClimbDown);
                                    break;

                                case SITHPLAYERMOVE_CLIMBING_LEFT:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetClimbingLeft);
                                    break;

                                case SITHPLAYERMOVE_CLIMBING_RIGHT:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetClimbingRight);
                                    break;

                                case SITHPLAYERMOVE_SLIDEDOWNFORWARD:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetSlideForward);
                                    break;

                                case SITHPLAYERMOVE_SLIDEDOWNBACK:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetSlideBackward);
                                    break;

                                case SITHPLAYERMOVE_JUMPROLLBACK:
                                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetJumpRollback);
                                    break;

                                default:
                                    curWeaponId = sithInventory_GetCurrentWeapon(pThing1);
                                    if (curWeaponId != SITHWEAPON_NO_WEAPON && curWeaponId != SITHWEAPON_SATCHEL && curWeaponId != SITHWEAPON_ZIPPO)
                                    {
                                        rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetWeaponDrawn);
                                    }
                                    else
                                    {
                                        rdVector_Copy3(&pCamera->offset, &sithCamera_vecExtCameraOffset);
                                    }
                                    break;
                            }
                        }
                    }
                    else if (pThing1->moveStatus == SITHPLAYERMOVE_SWIMIDLE && (pThing1->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0)
                    {
                        rdVector_Copy3(&pCamera->offset, &sithCamera_vecCameraOffsetClimbDown);
                    }
                    else
                    {
                        rdVector_Copy3(&pCamera->offset, &sithCamera_vecExtCameraOffset);
                    }
                }
                else
                {
                    rdVector_Copy3(&pCamera->offset, &sithCamera_vecExtCameraOffset);
                }

                rdMatrix_Copy34(&newOrient, &pThing1->orient);

                if (pCamera->focusInterpState == -1)
                {
                    rdVector_Copy3(&pCamera->focusPos, &pThing1->pos);
                    pCamera->focusInterpState = 0;
                }
                else if (pCamera->focusInterpState == 0)
                {
                    zDist = pCamera->focusPos.z - pThing1->pos.z;
                    if (zDist > 0.01f)
                    {
                        if (zDist > pThing1->collide.movesize * 10.0f) /* spelling-fit */
                        {
                            rdVector_Copy3(&pCamera->focusPos, &pThing1->pos);
                        }
                        else
                        {
                            pCamera->focusInterpState = 1;
                            pCamera->focusPos.x = pThing1->pos.x;
                            pCamera->focusPos.y = pThing1->pos.y;
                            pCamera->secFocusInterpDeltaTime = secFrameTime;

                            interpDelta = interpSpeedHalf - secFrameTime;
                            interpDelta = interpDelta / interpSpeedHalf;
                            zDelta = RD_MULTIPLY(pThing1->pos.z - pCamera->focusPos.z, interpDelta);
                            pCamera->focusPos.z = pThing1->pos.z - zDelta;
                        }
                    }
                    else
                    {
                        rdVector_Copy3(&pCamera->focusPos, &pThing1->pos);
                    }
                }
                else if (pCamera->focusInterpState == 1)
                {
                    pCamera->secFocusInterpDeltaTime = pCamera->secFocusInterpDeltaTime + secFrameTime;
                    interpDelta = interpSpeedHalf - pCamera->secFocusInterpDeltaTime;
                    if (interpDelta <= 0.0f)
                    {
                        rdVector_Copy3(&pCamera->focusPos, &pThing1->pos);
                        pCamera->focusInterpState = 0;
                    }
                    else
                    {
                        if (pCamera->focusPos.z != pThing1->pos.z)
                        {
                            pCamera->secFocusInterpDeltaTime = secFrameTime;
                            interpDelta = interpSpeedHalf - pCamera->secFocusInterpDeltaTime;
                            pCamera->focusPos.x = pThing1->pos.x;
                            pCamera->focusPos.y = pThing1->pos.y;
                        }

                        interpDelta = interpDelta / interpSpeedHalf;
                        zDelta = RD_MULTIPLY(pThing1->pos.z - pCamera->focusPos.z, interpDelta);
                        pCamera->focusPos.z = pThing1->pos.z - zDelta;
                    }
                }

                // Offset camera new newPos
                rdVector_Copy3(&newPos, &pCamera->focusPos);
                rdVector_ScaleAdd3Acc(&newPos, &sithCamera_camInfo.povShake.vecPosOffset, 3.0f);

                // Set camera new orient
                rdMatrix_PreRotate34(&newOrient, &newPYR);
                rdMatrix_PostTranslate34(&newOrient, &newPos);
                rdMatrix_Copy34(&pCamera->orient, &newOrient);

                // Set orient offset
                if (sithCamera_secLookOffsetTranslateDelta == 0.0f)
                {
                    rdMatrix_PreTranslate34(&newOrient, &sithCamera_vecExtCameraLookOffset);
                }
                else
                {
                    rdVector3 newLookOffset;
                    rdVector3 lookOffsetDelta;
                    float secLookOffsetDeltaTime;

                    // Translate orient offset
                    // This happens when switching to first person look
                    secLookOffsetDeltaTime = sithCamera_secLookOffsetTranslateDelta - secFrameTime;
                    if (secLookOffsetDeltaTime > 0.0f)
                    {
                        secLookOffsetDeltaTime = secLookOffsetDeltaTime / 0.5f;
                    }

                    sithCamera_secLookOffsetTranslateDelta -= secFrameTime;

                    if (sithCamera_secLookOffsetTranslateDelta < 0.0f)
                    {
                        rdMatrix_PreTranslate34(&newOrient, &sithCamera_vecExtCameraLookOffset);
                        sithCamera_secLookOffsetTranslateDelta = 0.0f;
                    }
                    else
                    {
                        rdVector_Set3(&lookOffsetDelta,
                            (sithCamera_vecExtCameraLookOffset.x - sithCamera_vecExtCameraPrevLookOffset.x) * (1.0f - secLookOffsetDeltaTime),
                            (sithCamera_vecExtCameraLookOffset.z - sithCamera_vecExtCameraPrevLookOffset.y) * (1.0f - secLookOffsetDeltaTime),
                            (sithCamera_vecExtCameraLookOffset.z - sithCamera_vecExtCameraPrevLookOffset.z) * (1.0f - secLookOffsetDeltaTime));
                        rdVector_Add3(&newLookOffset, &sithCamera_vecExtCameraPrevLookOffset, &lookOffsetDelta);
                        rdMatrix_PreTranslate34(&newOrient, &newLookOffset);
                    }
                }

                // Offset camera orient
                rdMatrix_PreTranslate34(&pCamera->orient, &pCamera->offset);

                // Find camera new sector
                pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &newOrient.dvec, 0.0f);
                rdVector_Copy3(&newLookPos, &pCamera->orient.dvec);

                if (sithCamera_vecExtCameraLookOffset.x != sithCamera_vecExtCameraDefaultLookOffset.x || sithCamera_vecExtCameraLookOffset.y != sithCamera_vecExtCameraDefaultLookOffset.y || sithCamera_vecExtCameraLookOffset.z != sithCamera_vecExtCameraDefaultLookOffset.z)
                {
                    pCamera->pSector = sithCollision_FindSectorInRadius(pCamera->pSector, &newOrient.dvec, &pCamera->orient.dvec, 0.0f);
                    bFindSectorOnly = 1;
                }
                else
                {
                    pCamera->pSector = sithCamera_SearchSectorInRadius(0, pCamera->pSector, &newOrient.dvec, &pCamera->orient.dvec, 0.01f, 0x200);
                }

                rdVector_Copy3(&curCamPos, &pCamera->orient.dvec);
            }
            if (pCamera->posInterpState == -1)
            {
                rdVector_Copy3(&pCamera->pos, &curCamPos);
                rdVector_Copy3(&newPos, &curCamPos);
                pCamera->posInterpState = 0;
                pCamera->secPosInterpDeltaTime = 0.0f;
            }
            else if (pCamera->posInterpState == 1)
            {
                rdVector_Copy3(&newPos, &curCamPos);
                pCamera->secPosInterpDeltaTime = pCamera->secPosInterpDeltaTime + secFrameTime;
                interpDelta = interpSpeed - pCamera->secPosInterpDeltaTime;
                if (interpDelta <= 0.0f)
                {
                    rdVector_Copy3(&pCamera->pos, &newPos);
                    pCamera->posInterpState = 0;
                }
                else
                {
                    if (newPos.x != pCamera->pos.x || newPos.y != pCamera->pos.y || newPos.z != pCamera->pos.z)
                    {
                        pCamera->secPosInterpDeltaTime = secFrameTime;
                        interpDelta = interpSpeed - pCamera->secPosInterpDeltaTime;
                        rdVector_Copy3(&pCamera->pos, &lookVector);
                    }

                    interpDelta = interpDelta / interpSpeed;
                    xDelta = RD_MULTIPLY(newPos.x - pCamera->pos.x, interpDelta);
                    yDelta = RD_MULTIPLY(newPos.y - pCamera->pos.y, interpDelta);
                    zDelta = RD_MULTIPLY(newPos.z - pCamera->pos.z, interpDelta);

                    if (pThing1->moveStatus != SITHPLAYERMOVE_PULLINGUP)
                    {
                        newPos.x = newPos.x - xDelta;
                        newPos.y = newPos.y - yDelta;
                    }

                    newPos.z = newPos.z - zDelta;
                }
            }
            else if (pCamera->posInterpState == 0)
            {
                rdVector_Copy3(&newPos, &curCamPos);

                if (newPos.x != pCamera->pos.x || newPos.y != pCamera->pos.y || newPos.z != pCamera->pos.z)
                {
                    pCamera->posInterpState = 1;
                    pCamera->secPosInterpDeltaTime = secFrameTime;
                    interpDelta = interpSpeed - secFrameTime;
                    interpDelta = interpDelta / interpSpeed;
                    xDelta = RD_MULTIPLY(newPos.x - pCamera->pos.x, interpDelta);
                    yDelta = RD_MULTIPLY(newPos.y - pCamera->pos.y, interpDelta);
                    zDelta = RD_MULTIPLY(newPos.z - pCamera->pos.z, interpDelta);

                    if (pThing1->moveStatus != SITHPLAYERMOVE_PULLINGUP)
                    {
                        newPos.x = newPos.x - xDelta;
                        newPos.y = newPos.y - yDelta;
                    }

                    newPos.z = newPos.z - zDelta;
                }
                else
                {
                    rdVector_Copy3(&pCamera->pos, &curCamPos);
                }
            }

            rdVector_Copy3(&newLookPos, &newOrient.dvec);
            pCamera->pSector = sithCollision_FindSectorInRadius(pCamera->pSector, &pCamera->orient.dvec, &newLookPos, 0.0f);
            if (!bFindSectorOnly)
            {
                pCamera->pSector = sithCamera_SearchSectorInRadius(0, pCamera->pSector, &newLookPos, &newPos, 0.01f, 0x200);
            }
            else
            {
                pCamera->pSector = sithCollision_FindSectorInRadius(pCamera->pSector, &newOrient.dvec, &newPos, 0.0f);
            }

            // Set camera orient based on new newPos and look newPos
            rdMatrix_LookAt(&pCamera->orient, &newPos, &newLookPos, 0.0f);

            rdVector_Copy3(&sithCamera_camSpot, &newPos);
            rdVector_Copy3(&sithCamera_camLookSpot, &newLookPos);

            // Now check if camera is too close to focus thing, and in this cane make foused thing transparent
            {
                rdVector3 lookDir;
                rdVector4 startColor;
                rdVector4 endColor;

                rdVector_Sub3(&lookDir, &newLookPos, &newPos);
                if ((distToLook = rdVector_Len3(&lookDir)) < 0.11f)
                {
                    // Dist too close, translate focus thing to invisible
                    if (!sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible())
                    {
                        endColor.x = 0.0f;
                        endColor.y = 0.0f;
                        endColor.z = 0.0f;
                        endColor.w = 0.0f;
                        startColor.x = 0.0f;
                        startColor.y = 0.0f;
                        startColor.z = 0.0f;
                        startColor.alpha = 1.0f;

                        sithAnimate_StartThingFadeAnim(pThing1, &startColor, &endColor, 0.2f, (SithAnimateFlags)0);
                        sithCamera_bFocusThingInvisible = 1;
                    }
                }
                else
                {
                    sithCamera_secFocusThingFadeInTime = sithCamera_secFocusThingFadeInTime + sithTime_frameTimeFlex;

                    if (sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible() && !sithCamera_bExtCameraLookMode)
                    {
                        // Translate focus thing to visible as cam is far away enough
                        endColor.x = 0.0f;
                        endColor.y = 0.0f;
                        endColor.z = 0.0f;
                        endColor.alpha = 1.0f;
                        startColor.x = 0.0f;
                        startColor.y = 0.0f;
                        startColor.z = 0.0f;
                        startColor.w = 0.0f;

                        sithAnimate_StartThingFadeAnim(pThing1, &startColor, &endColor, 0.2f, (SithAnimateFlags)0);
                        sithCamera_bFocusThingInvisible = 0;
                    }

                    if (pThing1->alpha != 1.0f && !sithCamera_bExtCameraLookMode)
                    {
                        if ((pThing1->moveStatus == SITHPLAYERMOVE_STILL || pThing1->moveStatus == SITHPLAYERMOVE_SWIMIDLE) && sithCamera_secFocusThingFadeInTime > 0.5f && !sithPlayerActions_IsInvisible() && sithInventory_GetCurrentWeapon(pThing1) != SITHWEAPON_IMP2) // Note: Note if pThing1 is not Actor or Player the call to sithInventory_GetCurrentWeapon function crashes app
                        {
                            sithCamera_secFocusThingFadeInTime = 0.0;
                            pThing1->alpha = 1.0;
                        }
                    }
                }
            }
        }
        break;
        case SITHCAMERA_CINEMATIC:
        {

#line 1287
            SITH_ASSERTREL(pThing1 != NULL);

#line 1288
            SITH_ASSERTREL(pThing2 != NULL);

#line 1289
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));

#line 1290
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing2));

            rdVector_Copy3(&newPos, &pThing1->pos);
            rdVector_Add3(&newPos, &newPos, &sithCamera_camInfo.povShake.vecPosOffset);

            rdVector_Copy3(&lookPos, &pThing2->pos);

            if (pCamera->bPosInterp)
            {
                if (pCamera->posInterpState == -1)
                {
                    rdVector_Copy3(&pCamera->pos, &newPos);
                    pCamera->posInterpState = 0;
                    pCamera->secPosInterpDeltaTime = 0.0f;
                }
                else if (pCamera->posInterpState == 1)
                {
                    pCamera->secPosInterpDeltaTime = pCamera->secPosInterpDeltaTime + secFrameTime;
                    interpDelta = interpSpeed - pCamera->secPosInterpDeltaTime;
                    if (interpDelta <= 0.0f)
                    {
                        rdVector_Copy3(&pCamera->pos, &newPos);
                        pCamera->posInterpState = 0;
                        pCamera->secPosInterpDeltaTime = 0.0f;
                    }
                    else
                    {
                        interpDelta = interpDelta / interpSpeed;
                        xDelta = RD_MULTIPLY(newPos.x - pCamera->pos.x, interpDelta);
                        yDelta = RD_MULTIPLY(newPos.y - pCamera->pos.y, interpDelta);
                        zDelta = RD_MULTIPLY(newPos.z - pCamera->pos.z, interpDelta);
                        newPos.x = newPos.x - xDelta;
                        newPos.y = newPos.y - yDelta;
                        newPos.z = newPos.z - zDelta;
                    }
                }
                else if (pCamera->posInterpState == 0)
                {
                    if (newPos.x != pCamera->pos.x || newPos.y != pCamera->pos.y || newPos.z != pCamera->pos.z)
                    {
                        pCamera->posInterpState = 1;
                        pCamera->secPosInterpDeltaTime = 0.0f;
                        rdVector_Copy3(&newPos, &pCamera->pos);
                    }
                    else
                    {
                        rdVector_Copy3(&pCamera->pos, &newPos);
                        pCamera->secPosInterpDeltaTime = 0.0f;
                    }
                }
            }
            else
            {
                rdVector_Copy3(&pCamera->pos, &newPos);
            }

            if (pCamera->bLookInterp)
            {
                if (pCamera->lookInterpState == -1)
                {
                    rdVector_Copy3(&pCamera->curLookInterp, &lookPos);
                    pCamera->lookInterpState = 0;
                    pCamera->secLookInterpDeltaTime = 0.0f;
                }
                else if (pCamera->lookInterpState == 1)
                {
                    pCamera->secLookInterpDeltaTime = pCamera->secLookInterpDeltaTime + secFrameTime;
                    interpDelta = interpSpeed - pCamera->secLookInterpDeltaTime;
                    if (interpDelta <= 0.0f)
                    {
                        rdVector_Copy3(&pCamera->curLookInterp, &lookPos);
                        pCamera->lookInterpState = 0;
                        pCamera->secLookInterpDeltaTime = 0.0f;
                    }
                    else
                    {
                        interpDelta = interpDelta / interpSpeed;
                        xDelta = RD_MULTIPLY(lookPos.x - pCamera->curLookInterp.x, interpDelta);
                        yDelta = RD_MULTIPLY(lookPos.y - pCamera->curLookInterp.y, interpDelta);
                        zDelta = RD_MULTIPLY(lookPos.z - pCamera->curLookInterp.z, interpDelta);
                        lookPos.x = lookPos.x - xDelta;
                        lookPos.y = lookPos.y - yDelta;
                        lookPos.z = lookPos.z - zDelta;
                    }
                }
                else if (pCamera->lookInterpState == 0)
                {
                    if (lookPos.x != pCamera->curLookInterp.x || lookPos.y != pCamera->curLookInterp.y || lookPos.z != pCamera->curLookInterp.z)
                    {
                        pCamera->lookInterpState = 1;
                        pCamera->secLookInterpDeltaTime = 0.0f;
                        rdVector_Copy3(&lookPos, &pCamera->curLookInterp);
                    }
                    else
                    {
                        rdVector_Copy3(&pCamera->curLookInterp, &lookPos);
                    }
                }
            }
            else
            {
                rdVector_Copy3(&pCamera->curLookInterp, &lookPos);
            }

            rdMatrix_LookAt(&pCamera->orient, &newPos, &lookPos, 0.0f);
            rdMatrix_PostRotate34(&pCamera->orient, &sithCamera_camInfo.povShake.vecAngleOffset);
            rdMatrix_Normalize34(&pCamera->orient);
            pCamera->pSector = sithCamera_SearchSectorInRadius(0, pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.01f, 0x2200);
        }
        break;

        case SITHCAMERA_IDLE:
        {
            rdVector3 lookDir;
            float distToLook;

#line 1422
            SITH_ASSERTREL(pThing1!=NULL);

#line 1423
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
            rdMatrix_TransformVector34(&pos, &sithCamera_idleCamOffset, &sithCamera_idleCamOrient);

            lookPos.x = 0.0f;
            lookPos.y = 0.0f;
            lookPos.z = 0.05f;

            rdVector_Add3Acc(&lookPos, &pThing1->pos);

            rdVector_Add3(&pos, &pos, &lookPos);
            rdMatrix_LookAt(&pCamera->orient, &pos, &lookPos, 0.0f);

            pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &lookPos, 0.0f);
            pCamera->pSector = sithCamera_SearchSectorInRadius(0, pCamera->pSector, &lookPos, &pCamera->orient.dvec, 0.01f, 0x200);

            rdVector_Sub3(&lookDir, &pCamera->orient.dvec, &lookPos);

            {
                rdVector4 startColor;
                rdVector4 endColor;

                if ((distToLook = rdVector_Len3(&lookDir)) < 0.098165f)
                {
                    if (!sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible())
                    {
                        endColor.x = 0.0f;
                        endColor.y = 0.0f;
                        endColor.z = 0.0f;
                        endColor.w = 0.0f;
                        startColor.x = 0.0f;
                        startColor.y = 0.0f;
                        startColor.z = 0.0f;
                        startColor.alpha = 1.0f;
                        sithAnimate_StartThingFadeAnim(pThing1, &startColor, &endColor, 0.2f, (SithAnimateFlags)0);
                        sithCamera_bFocusThingInvisible = 1;
                    }
                }
                else if (sithCamera_bFocusThingInvisible && !sithPlayerActions_IsInvisible())
                {
                    endColor.x = 0.0f;
                    endColor.y = 0.0f;
                    endColor.z = 0.0f;
                    endColor.alpha = 1.0f;
                    startColor.x = 0.0f;
                    startColor.y = 0.0f;
                    startColor.z = 0.0f;
                    startColor.w = 0.0f;
                    sithAnimate_StartThingFadeAnim(pThing1, &startColor, &endColor, 0.2f, (SithAnimateFlags)0);
                    sithCamera_bFocusThingInvisible = 0;
                }

            }

            rdVector_Set3(&vecPYR, 0.0f, sithTime_frameTimeFlex * 8.0f, 0.0f);
            rdMatrix_PostRotate34(&sithCamera_idleCamOrient, &vecPYR);
            rdMatrix_Normalize34(&sithCamera_idleCamOrient);
        }
        break;

        case SITHCAMERA_UNKNOWN_40:
            rdVector_Normalize3Acc(&sithCamera_vecOffsetCamType40);
            rdVector_Neg3(&pCamera->orient.lvec, &sithCamera_vecOffsetCamType40);
            rdVector_Cross3(&pCamera->orient.rvec, &pCamera->orient.lvec, &rdroid_zVector3);
            rdVector_Cross3(&pCamera->orient.uvec, &pCamera->orient.rvec, &pCamera->orient.lvec);
            rdMatrix_Normalize34(&pCamera->orient);

            rdVector_Scale3(&pCamera->orient.dvec, &sithCamera_vecOffsetCamType40, 0.2f);
            rdMatrix_PostTranslate34(&pCamera->orient, &pThing1->pos);

            pCamera->pSector = sithCamera_SearchSectorInRadius(0, pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.01f, 0x2200);
            break;

        case SITHCAMERA_ORBITAL:

#line 1501
            SITH_ASSERTREL(pThing1!=NULL);

#line 1502
            SITH_ASSERTREL(sithThing_ValidateThingPointer(pWorld, pThing1));
            rdMatrix_Copy34(&pCamera->orient, &sithCamera_orbCamOrient);
            rdMatrix_PostTranslate34(&pCamera->orient, &pThing1->pos);
            pCamera->pSector = sithCollision_FindSectorInRadius(pThing1->pInSector, &pThing1->pos, &pCamera->orient.dvec, 0.01f);
            break;

        default:
            break;
    }

    // Update global newPos & rot delta states
    rdVector_Copy3(&pCamera->lookPos, &pCamera->orient.dvec);
    rdMatrix_ExtractAngles34(&pCamera->orient, &pCamera->lookPYR);
    if (sithCamera_camInfo.povShake.vecPosOffset.x > 0.0f)
    {
        sithCamera_camInfo.povShake.vecPosOffset.x = J3DMAX(sithCamera_camInfo.povShake.vecPosOffset.x - posDelta, 0.0f);
    }
    else if (sithCamera_camInfo.povShake.vecPosOffset.x < 0.0f)
    {
        sithCamera_camInfo.povShake.vecPosOffset.x = J3DMIN(sithCamera_camInfo.povShake.vecPosOffset.x + posDelta, 0.0f);
    }

    if (sithCamera_camInfo.povShake.vecPosOffset.y > 0.0f)
    {
        sithCamera_camInfo.povShake.vecPosOffset.y = J3DMAX(sithCamera_camInfo.povShake.vecPosOffset.y - posDelta, 0.0f);
    }
    else if (sithCamera_camInfo.povShake.vecPosOffset.y < 0.0f)
    {
        sithCamera_camInfo.povShake.vecPosOffset.y = J3DMIN(sithCamera_camInfo.povShake.vecPosOffset.y + posDelta, 0.0f);
    }

    if (sithCamera_camInfo.povShake.vecPosOffset.z > 0.0f)
    {
        sithCamera_camInfo.povShake.vecPosOffset.z = J3DMAX(sithCamera_camInfo.povShake.vecPosOffset.z - posDelta, 0.0f);
    }
    else if (sithCamera_camInfo.povShake.vecPosOffset.z < 0.0f)
    {
        sithCamera_camInfo.povShake.vecPosOffset.z = J3DMIN(sithCamera_camInfo.povShake.vecPosOffset.z + posDelta, 0.0f);
    }

    if (sithCamera_camInfo.povShake.vecAngleOffset.x > 0.0f)
    {
        sithCamera_camInfo.povShake.vecAngleOffset.x = J3DMAX(sithCamera_camInfo.povShake.vecAngleOffset.x - angleDelta, 0.0f);
    }
    else if (sithCamera_camInfo.povShake.vecAngleOffset.x < 0.0f)
    {
        sithCamera_camInfo.povShake.vecAngleOffset.x = J3DMIN(sithCamera_camInfo.povShake.vecAngleOffset.x + angleDelta, 0.0f);
    }

    if (sithCamera_camInfo.povShake.vecAngleOffset.y > 0.0f)
    {
        sithCamera_camInfo.povShake.vecAngleOffset.y = J3DMAX(sithCamera_camInfo.povShake.vecAngleOffset.y - angleDelta, 0.0f);
    }
    else if (sithCamera_camInfo.povShake.vecAngleOffset.y < 0.0f)
    {
        sithCamera_camInfo.povShake.vecAngleOffset.y = J3DMIN(sithCamera_camInfo.povShake.vecAngleOffset.y + angleDelta, 0.0f);
    }

    if (sithCamera_camInfo.povShake.vecAngleOffset.z > 0.0f)
    {
        sithCamera_camInfo.povShake.vecAngleOffset.z = J3DMAX(sithCamera_camInfo.povShake.vecAngleOffset.z - angleDelta, 0.0f);
    }
    else if (sithCamera_camInfo.povShake.vecAngleOffset.z < 0.0f)
    {
        sithCamera_camInfo.povShake.vecAngleOffset.z = J3DMIN(sithCamera_camInfo.povShake.vecAngleOffset.z + angleDelta, 0.0f);
    }
}

void J3DAPI sithCamera_RenderScene(void)
{
    if ( !sithCamera_pCurCamera )
    {
        return;
    }

    rdCamera_SetCurrent(&sithCamera_pCurCamera->rdCamera);
    rdCamera_Update(&sithCamera_pCurCamera->orient);
    sithRender_RenderScene();
}

void J3DAPI sithCamera_SetPOVShake(const rdVector3* posOffset, const rdVector3* angleOffset, float posDelta, float angleDelta)
{
    rdVector_Copy3(&sithCamera_camInfo.povShake.vecPosOffset, posOffset);
    rdVector_Copy3(&sithCamera_camInfo.povShake.vecAngleOffset, angleOffset);

    sithCamera_camInfo.povShake.posDelta   = posDelta;
    sithCamera_camInfo.povShake.angleDelta = angleDelta;
}

SithSector* J3DAPI sithCamera_SearchSectorInRadius(int a1, SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float unused, int flags)
{
    rdVector3 moveNorm;
    float moveDist;
    SithSector* pFoundSector;
    SithCollision* pCollision;
    float radius;

    /* Function searches for collision that blocks camera movement */

    rdVector_Sub3(&moveNorm, endPos, startPos);
    moveDist = rdVector_Normalize3Acc(&moveNorm);

    pFoundSector = pSector;
    radius = 0.014f;
    sithCollision_SearchForCollisions(pSector, NULL, startPos, &moveNorm, moveDist, radius, flags | 0x800);
    while ( (pCollision = sithCollision_PopStack()) != NULL )
    {
        if ( (pCollision->type & SITHCOLLISION_ADJOINTOUCH) != 0 )
        {
#line 1736
            SITH_ASSERTREL(pCollision->pSurfaceCollided->pAdjoin != NULL);
            pFoundSector = pCollision->pSurfaceCollided->pAdjoin->pAdjoinSector;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0
            && (pCollision->pThingCollided->type == SITH_THING_ITEM
                || pCollision->pThingCollided->type == SITH_THING_CORPSE
                || pCollision->distance == 0.0f
                || pCollision->pThingCollided->type == SITH_THING_WEAPON) )
        {
            continue;
        }
        else if ( (pCollision->type & SITHCOLLISION_THING) != 0 && pCollision->pThingCollided == sithPlayer_pLocalPlayerThing )
        {
            continue;
        }
        else
        {
            rdVector_ScaleAdd3(endPos, &moveNorm, pCollision->distance, startPos);
            break;
        }
    }

    sithCollision_DecreaseStackLevel();
    return pFoundSector;
}

void J3DAPI sithCamera_SetCameraStateFlags(SithCameraState flags)
{
    sithCamera_camInfo.stateFlags = flags;
}

SithCameraState J3DAPI sithCamera_GetCameraStateFlags(void)
{
    return sithCamera_camInfo.stateFlags;
}
