#define RDMATRIX_NO_INLINE // Retail calls rdMatrix_Copy34 out of line at 0x0048D8E8 and 0x0048D9E2 in sithFX_CreateRaftRipple
#include "sithFX.h"

#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithParticle.h>
#include <sith/Engine/sithPhysics.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Engine/rdThing.h>
#define RDMATH_INLINE_DISTANCEPOINTTOPLANE
#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Primitives/rdPolyline.h>

#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithFX_countfit_1, 17)
CF_PROTO(sithFX_countfit_2, 17)
CF_PROTO(sithFX_countfit_3, 17)
CF_PROTO(sithFX_countfit_4, 3)
/* end count-fit */

/* sithFX_lastChalkMarkNum (.data 0x00539458, retail bytes ffffffff) and the 4.0f
   tunable at 0x0053945C that sithFX_CreateBubble's lifetime reads
   (fadd dword ptr [0x0053945C] at 0x0048D3B0) open this TU's .data block. */
int sithFX_lastChalkMarkNum = -1;
float sithFX_flt_53945C = 4.0f;

const char* sithFX_aChalkMarkNames[35] =
{
    "+1947_mark",
    "+alien_mark",
    "+ankh_mark",
    "+ark_mark",
    "+cccr_mark",
    "+check_mark",
    "+cir_mark",
    "+cross_mark",
    "+eye_mark",
    "+five_mark",
    "+grail_mark",
    "+guy_mark",
    "+happy_mark",
    "+hawk_mark",
    "+here_mark",
    "+indy_mark",
    "+indy2_mark",
    "+la_mark",
    "+love_mark",
    "+portrait_mark",
    "+pound_mark",
    "+pyramid_mark",
    "+question_mark",
    "+r2d2_mark",
    "+skull_mark",
    "+snake_mark",
    "+square_mark",
    "+star_mark",
    "+stones_mark",
    "+sunrise_mark",
    "+tentacle_mark",
    "+tictactoe_mark",
    "+triangle_mark",
    "+usa_mark",
    "+x_mark"
};

/* The .data tunables 0x005394EC-0x00539540 (21 slots) are function statics of
   their readers, declared after the locals of the block that reads them, as in
   sithPhysics/sithAIMove/sithCollision: retail's .data order is the readers'
   definition order (CreateFairyDustDelux, CreateRaftWake, CreatePaddleSplash,
   the discarded CreateTireFx, CreateMineCarSparks), and CreateRaftWake loads its
   plane offset over the local pos (`fld [0x005394F8]` kept on the stack), which
   needs a symbol declared after pos. The values, the storage and the order are
   retail bytes; the names are inferred. */

/* The zero-initialized definitions below are declared in retail's .bss order
   (0x00585430-0x00585464). sithFX_aChalkMarks stays a tentative definition
   because retail satisfies it in the image's far common tail at 0x017F10C0. */
rdVector3 sithFx_leftTireStartPosOffset = { 0 };
rdVector3 sithFX_rowRippleLastPos    = { 0 };

/* Read only by the linker-discarded sithFx_CreateTireFx (debug build): retail's otherwise
   unreferenced 8-aligned .bss at 0x00585420 and 0x00585440. */
rdVector3 sithFx_rightTireStartPosOffset = { 0 };

unsigned int sithFX_msecLastCreatedFairyDustDeluxTime = 0;
int sithFX_numChalkMarks = 0;
size_t sithFX_fariyDustSizeFactor         = 0;
size_t sithFX_newFairyDustDeluxSizeFactor = 0;
int sithFX_fairyDustDeluxSizeFactor    = 0;
float sithFX_secLastCreatedRowRipple = 0.0f;

SithThing* sithFX_aChalkMarks[10];

void J3DAPI sithFX_Reset(void)
{
    sithFX_numChalkMarks    = 0;
    sithFX_lastChalkMarkNum = -1;
    sithFX_ClearChalkMarks();
}

void J3DAPI sithFX_ClearChalkMarks(void)
{
    int i;

    for ( i = 0; i < 10; ++i )
    {
        sithFX_aChalkMarks[i] = NULL;
    }
}

void J3DAPI sithFX_DestroyFairyDustDeluxe(SithThing* pThing)
{
    SithFairyDustUserBlock* pCurFairydust;
    int i;

    if ( !pThing || !pThing->userblock.pFairydust )
    {
        return;
    }

    pCurFairydust = pThing->userblock.pFairydust;
    for ( i = 0; i < 4; ++i )
    {
        if ( pCurFairydust->aDusts[i] && pCurFairydust->aDusts[i]->type != SITH_THING_FREE && (pCurFairydust->aDusts[i]->flags & SITH_TF_DESTROYED) == 0 )
        {
            sithParticle_DestroyParticle(pCurFairydust->aDusts[i]);
        }

        pCurFairydust->aDusts[i] = NULL;
    }

    stdMemory_Free(pThing->userblock.pFairydust);
    pThing->userblock.pFairydust = NULL;
}

void J3DAPI sithFX_CreateFairyDust(const rdVector3* pos, SithSector* pSector)
{
    SithThing* pDust;

    SithThing* pTemplate = sithTemplate_GetTemplate("+imp2_trail");
    if ( pTemplate )
    {
        pDust = sithThing_CreateThingAtPos(pTemplate, pos, &pTemplate->orient, pSector, NULL);
    }

    // Retail bug: pDust is uninitialized when the template is missing
    if ( pDust )
    {
        sithFX_fariyDustSizeFactor = sithFX_SetFairyDustSize(pDust->renderData.data.pParticle, sithFX_fariyDustSizeFactor);
    }
}

int J3DAPI sithFX_SetFairyDustSize(rdParticle* pParticle, int sizeFactor)
{
    if ( !sizeFactor )
    {
        sizeFactor = 2;
    }
    else
    {
        --sizeFactor;
    }

    pParticle->size = D3DVAL((sizeFactor * 0.0025) + 0.005);
    pParticle->sizeHalf = pParticle->size / 2.0f;
    return sizeFactor;
}

void J3DAPI sithFX_UpdateFairyDustUserBlock(SithThing* pThing)
{
    SithFairyDustUserBlock* pCurFairydust;
    int i;

    if ( !pThing || !pThing->pParent )
    {
        return;
    }

    pCurFairydust = pThing->pParent->userblock.pFairydust;
    if ( !pCurFairydust )
    {
        return;
    }

    for ( i = 0; i < 4; ++i )
    {
        if ( pCurFairydust->aDusts[i] && ((pCurFairydust->aDusts[i]->flags & SITH_TF_DESTROYED) != 0 || pCurFairydust->aDusts[i]->type == SITH_THING_FREE) )
        {
            pCurFairydust->aDusts[i] = NULL;
        }
    }

    if ( strcmp(pThing->thingInfo.particleInfo.pMaterial->aName, "aet_4sprite_glow_fairy_dust.mat") == 0 )
    {
        pThing->pos.x = pThing->pParent->pos.x;
        pThing->pos.y = pThing->pParent->pos.y;
    }
}

void J3DAPI sithFX_CreateFairyDustDelux(SithThing* pThing, const rdVector3* pPos)
{
    SithFairyDustUserBlock* pFairydust;
    static unsigned int sithFX_msecFairyDustDeluxDelay = 900;

    if ( !pThing )
    {
        return;
    }

    pFairydust = pThing->userblock.pFairydust;
    if ( !pFairydust ) // Create new
    {
#line 310
        pFairydust = (SithFairyDustUserBlock*)STDMALLOC(sizeof(SithFairyDustUserBlock));
        if ( !pFairydust )
        {
            return;
        }

        memset(pFairydust, 0, sizeof(SithFairyDustUserBlock));
        if ( sithFX_CreateFairyDustDeluxDusts(pThing, pFairydust) )
        {
            pFairydust->bCreateNormalFairyDust = 0;
            sithFX_msecLastCreatedFairyDustDeluxTime = sithTime_msecGameTime;
            pThing->userblock.pFairydust = pFairydust;
        }
        else
        {
            stdMemory_Free(pFairydust);
            pFairydust = NULL;
        }
    }
    else if ( pFairydust->bCreateNormalFairyDust )
    {
        if ( pPos )
        {
            sithFX_CreateFairyDust(pPos, pThing->pInSector);
        }
    }
    else if ( sithTime_msecGameTime - sithFX_msecLastCreatedFairyDustDeluxTime > sithFX_msecFairyDustDeluxDelay )
    {
        pFairydust->bCreateNormalFairyDust = 1;
    }
    else
    {
        sithFX_CreateFairyDustDeluxDusts(pThing, pFairydust);
    }
}

int J3DAPI sithFX_CreateFairyDustDeluxDusts(SithThing* pThing, SithFairyDustUserBlock* pFairydust)
{
    SithThing* pDust;
    rdVector3 curpos;
    int i;

    SithThing* pTemplate = sithTemplate_GetTemplate("+imp2_dust");
    if ( !pTemplate )
    {
        return 0;
    }

    rdVector_Copy3(&curpos, &pThing->pos);

    curpos.z -= sithPhysics_GetThingHeight(pThing);
    curpos.z += D3DVAL(D3DMultiply((float)pFairydust->numUsedDusts, 0.045f) + 0.0225f);

    for ( i = 0; i < 4; ++i )
    {
        if ( pFairydust->aDusts[i] && pFairydust->aDusts[i]->type )
        {
            sithFX_fairyDustDeluxSizeFactor = sithFX_SetFairyDustSize(pFairydust->aDusts[i]->renderData.data.pParticle, sithFX_fairyDustDeluxSizeFactor);
        }
    }

    if ( pFairydust->aDusts[pFairydust->numUsedDusts] && pFairydust->aDusts[pFairydust->numUsedDusts]->type != SITH_THING_FREE )
    {
        return 1;
    }

    pDust = sithThing_CreateThingAtPos(pTemplate, &curpos, &pTemplate->orient, pThing->pInSector, NULL);
    if ( pDust )
    {
        pDust->pParent = pThing;
        sithFX_newFairyDustDeluxSizeFactor = sithFX_SetFairyDustSize(pDust->renderData.data.pParticle, sithFX_newFairyDustDeluxSizeFactor);

        pFairydust->aDusts[pFairydust->numUsedDusts++] = pDust;
        if ( pFairydust->numUsedDusts == 4 )
        {
            pFairydust->numUsedDusts = 0;
        }

        return 1;
    }

    return 0;
}

void J3DAPI sithFX_CreateBubble(SithThing* pThing)
{
    SithThing* pSprite;
    rdVector3 offset;
    float size;
    rdVector3 start;
    rdVector3 end;
    float time;
    rdVector3 vel;
    SithThing* pTemplate;

    if ( pThing->type != SITH_THING_PLAYER )
    {
        return;
    }

    if ( pThing->pInSector && (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) == 0 )
    {
        return;
    }

    pTemplate = sithTemplate_GetTemplate("+bubbles");
    if ( pTemplate )
    {
        rdVector_Set3(&start, 0.001f, 0.001f, 1.0f);

        rdVector_Set3(&end, 0.02f, 0.02f, 0.0f);

        pSprite = sithThing_CreateThingAtPos(pTemplate, &pThing->pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pSprite )
        {
    #line 386
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Couldn't make a bubble thing!\n", 0, 0, 0, 0);
            return;
        }

        rdVector_Scale3(&offset, &pThing->orient.lvec, 0.07f);

        rdVector_Add3Acc(&pSprite->pos, &offset);
        pSprite->pos.z = pSprite->pos.z + pThing->collide.movesize;

        size = pThing->collide.movesize / 3.0f;
        rdVector_Scale3(&offset, &pThing->orient.rvec, size);

        if ( SITH_RANDF() <= 0.5 )
        {
            rdVector_Add3Acc(&pSprite->pos, &offset);
        }
        else
        {
            rdVector_Sub3Acc(&pSprite->pos, &offset);
        }

        pSprite->flags |= SITH_TF_AIRDESTROYED;

        time = ((SITH_RANDF() - 0.5f) * 2.0f) + sithFX_flt_53945C;
        sithAnimate_StartSpriteSizeAnim(pSprite, &start, &end, time);

        vel.x = SITH_RANDF() - 0.5f;
        vel.y = SITH_RANDF() - 0.5f;
        vel.z = SITH_RANDF() + 0.55f;
        rdVector_Scale3Acc(&vel, 0.02f);
        rdVector_Copy3(&pSprite->moveInfo.physics.velocity, &vel);
    }
}

void J3DAPI sithFX_CreateWaterRipple(SithThing* pThing)
{
    SithThing* pTemplate;
    SithThing* pSprite;
    rdVector3 pos;
    float rippleSize;
    rdVector3 start;

    if ( (pThing->pInSector->flags & SITH_SECTOR_UNDERWATER) != 0 && (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) == 0 )
    {
        return;
    }

    if ( (pThing->pInSector->flags & SITH_SECTOR_AETHERIUM) != 0 )
    {
        return;
    }

    pTemplate = sithTemplate_GetTemplate("+ripples");
    if ( pTemplate )
    {
        rdVector3 end;

        if ( (pThing->moveInfo.physics.flags & SITH_PF_ONWATERSURFACE) != 0 || (pThing->moveInfo.physics.flags & SITH_PF_RAFT) != 0 )
        {
            rippleSize = pThing->collide.movesize;
            rdVector_Set3(&start, rippleSize, rippleSize, 1.0f);

            rippleSize = pThing->collide.movesize * 4.0f;
            rdVector_Set3(&end, rippleSize, rippleSize, 0.0f);

            rdVector_Copy3(&pos, &pThing->pos);
            pos.z += pThing->attach.distToWaterSurface;
            pos.z += 0.001f;
        }
        else
        {
            rippleSize = pThing->collide.movesize / 4.0f;
            rdVector_Set3(&start, rippleSize, rippleSize, 1.0f);

            rippleSize = pThing->collide.movesize * 2.0f;
            rdVector_Set3(&end, rippleSize, rippleSize, 0.0f);

            rdVector_Copy3(&pos, &pThing->pos);
            pos.z = pos.z - pThing->collide.movesize * 2.0f;
        }

        pSprite = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pSprite )
        {
#line 462
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a ripple, no thing space!\n", 0, 0, 0, 0);
            return;
        }

        rdVector_Set3(&pSprite->orient.lvec, 0.0f, 0.0f, 1.0f);
        if ( pSprite )
        {
            sithAnimate_StartSpriteSizeAnim(pSprite, &start, &end, SITH_RANDF() + 1.0f);
        }
    }
}

void J3DAPI sithFX_CreateRaftRipple(SithThing* pThing, int bCreateSplash)
{
    float size;
    rdVector3 sizeStart;
    SithThing* pWaterThing;
    SithThing* pTemplate;
    rdVector3 end;
    rdVector3 ripplePos;
    SithThing* pMistTpl;
    rdMatrix34 orient;
    rdVector3 pyr;
    rdMaterial* pMat;
    rdVector3 pos;

    pTemplate = sithTemplate_GetTemplate("+ripples");
    if ( pTemplate && pThing->attach.flags )
    {
        size = pThing->collide.movesize;
        rdVector_Set3(&sizeStart, size, size, 1.0f);

        size = pThing->collide.movesize * 4.0f;
        rdVector_Set3(&end, size, size, 0.0f);

        memcpy(&ripplePos, &pThing->pos, sizeof(rdVector3));
        ripplePos.z -= rdMath_DistancePointToPlane(&pThing->pos, &pThing->attach.pFace->normal, &pThing->attach.attachedFaceFirstVert);
        ripplePos.z += 0.002f;
        ripplePos.x += D3DVAL((D3DVAL(SITH_RANDF()) - 0.5) * 0.03);
        ripplePos.y += D3DVAL((D3DVAL(SITH_RANDF()) - 0.5) * 0.03);

        pWaterThing = sithThing_CreateThingAtPos(pTemplate, &ripplePos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pWaterThing )
        {
#line 511
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a ripple, no thing space!\n", 0, 0, 0, 0);
            return;
        }

        rdVector_Set3(&pWaterThing->orient.lvec, 0.0f, 0.0f, 1.0f);
        sithAnimate_StartSpriteSizeAnim(pWaterThing, &sizeStart, &end, SITH_RANDF() + 1.5f);
    }

    if ( bCreateSplash )
    {
        pTemplate = sithTemplate_GetTemplate("+raft_splash");
        pMistTpl = sithTemplate_GetTemplate("spritlemist");
        if ( pTemplate && pMistTpl )
        {
            // Sprite sizes
            rdVector_Set3(&sizeStart, 3.5f, 1.5f, 1.0f);
            rdVector_Set3(&end, 3.5f, 1.5f, 1.0f);

            // Create splash thing
            rdVector_Set3(&pos, 0.12f, 0.09f, 0.01f); // right
            rdMatrix_TransformVector34Acc(&pos, &pThing->orient);
            rdVector_Add3Acc(&pos, &ripplePos);

            rdMatrix_Copy34(&orient, &pThing->orient);
            rdVector_Set3(&pyr, 90.0f, 90.0f, 0.0f);
            rdMatrix_PreRotate34(&orient, &pyr);
            memcpy(&orient.uvec, &pThing->attach.pFace->normal, sizeof(rdVector3));

            pWaterThing = sithThing_CreateThingAtPos(pTemplate, &pos, &orient, pThing->pInSector, NULL);
            if ( pWaterThing )
            {
                sithAnimate_StartSpriteSizeAnim(pWaterThing, &sizeStart, &end, 0.5f);
            }

            // Create mist thing
            pos.z += 0.05f;
            sithThing_CreateThingAtPos(pMistTpl, &pos, &pMistTpl->orient, pThing->pInSector, NULL);

            // Create another splash thing
            rdVector_Set3(&pos, -0.12f, 0.09f, 0.01f); // left
            rdMatrix_TransformVector34Acc(&pos, &pThing->orient);
            rdVector_Add3Acc(&pos, &ripplePos);

            rdMatrix_Copy34(&orient, &pThing->orient);
            rdVector_Set3(&pyr, 90.0f, -90.0f, 0.0f);
            rdMatrix_PreRotate34(&orient, &pyr);
            memcpy(&orient.uvec, &pThing->attach.pFace->normal, sizeof(rdVector3));

            pWaterThing = sithThing_CreateThingAtPos(pTemplate, &pos, &orient, pThing->pInSector, NULL);
            if ( pWaterThing )
            {
                sithAnimate_StartSpriteSizeAnim(pWaterThing, &sizeStart, &end, 0.5f);
            }

            // Create another mist thing
            pos.z += 0.05f;
            sithThing_CreateThingAtPos(pMistTpl, &pos, &pMistTpl->orient, pThing->pInSector, NULL);

            pMat = sithMaterial_Load("riv_a4sprite_rocksplash.mat");
            if ( pMat )
            {
                sithAnimate_StartMaterialAnim(pMat, 16.0f, (SithAnimateFlags)0);
            }
        }
    }
}

void J3DAPI sithFX_CreateRaftWake(SithThing* pThing)
{
    rdVector3 start;
    rdVector3 pos;
    SithThing* pTemplate = sithTemplate_GetTemplate("+raft_wake");
    if ( pTemplate && pThing->attach.flags )
    {
        rdVector3 end;
        SithThing* pSprite;
        rdVector3 velDir;

        static float sithFX_flt_5394F0 = 0.25f;
        static float sithFX_flt_5394F4 = 0.5f;
        static float sithFX_flt_5394F8 = 0.002f;
        static float sithFX_flt_5394FC = 0.175f;
        static float sithFX_flt_539500 = 0.275f;
        static float sithFX_flt_539504 = 0.1f;
        static float sithFX_flt_539508 = 0.4f;
        rdVector_Set3(&start, sithFX_flt_5394FC, sithFX_flt_539504, sithFX_flt_5394F0);

        
        rdVector_Set3(&end, sithFX_flt_539500, sithFX_flt_539508, 0.0f);

        
        rdVector_Copy3(&pos, &pThing->pos);
        pos.z -= rdMath_DistancePointToPlane(&pThing->pos, &pThing->attach.pFace->normal, &pThing->attach.attachedFaceFirstVert);
        pos.z += sithFX_flt_5394F8;

        pSprite = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pSprite )
        {
#line 626
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a wake, no thing space!\n", 0, 0, 0, 0);
            return;
        }

        rdVector_Set3(&pSprite->orient.lvec, 0.0f, 0.0f, 1.0f);

        
        rdVector_Normalize3(&velDir, &pThing->moveInfo.physics.velocity);
        pSprite->thingInfo.spriteInfo.rollAngle = rdMath_DeltaAngleNormalized(&rdroid_xVector3, &velDir, &rdroid_zVector3) + 180.0f;

        sithAnimate_StartSpriteSizeAnim(pSprite, &start, &end, sithFX_flt_5394F4);
    }
}

void J3DAPI sithFX_CreatePaddleSplash(SithThing* pThing, const rdVector3* pos)
{
    SithThing* pSplash;
    SithThing* template = sithTemplate_GetTemplate("paddlesplash");
    if ( template )
    {
        static float sithFX_flt_53950C = 0.07f;
        pSplash = sithThing_CreateThingAtPos(template, pos, &template->orient, pThing->pInSector, NULL);
        if ( !pSplash )
        {
#line 657
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a paddle splash, no thing space!\n", 0, 0, 0, 0);
            return;
        }

        pSplash->moveInfo.physics.velocity.z = sithFX_flt_53950C;
    }
}


void J3DAPI sithFX_CreateRowWaterFx(SithThing* pThing, float secTime)
{
    int bCreateSplashFx;
    SithThing* pTemplate;
    SithThing* pSprite;
    rdVector3 start;
    rdVector3 end;
    rdVector3 ripplePos;
    float xRate;
    float yRate;
    float dist;

    float secDeltaTime = sithTime_secGameTime - secTime;
    float secDelatLastCreated = sithTime_secGameTime - sithFX_secLastCreatedRowRipple;

    switch ( pThing->moveStatus )
    {
        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_LEFT:
            rdVector_Set3(&ripplePos, -0.075f, 0.09f, -sithPhysics_GetThingHeight(pThing));
            xRate = 0.0f;
            yRate = -0.32f;
            ripplePos.x -= D3DMultiply(xRate, secDeltaTime);
            ripplePos.y += D3DMultiply(yRate, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_FORWARD_RIGHT:
            rdVector_Set3(&ripplePos, 0.075f, 0.09f, -sithPhysics_GetThingHeight(pThing));
            xRate = 0.0f;
            yRate = -0.32f;
            ripplePos.x += D3DMultiply(xRate, secDeltaTime);
            ripplePos.y += D3DMultiply(yRate, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_LEFT:
            rdVector_Set3(&ripplePos, -0.07f, -0.04f, -sithPhysics_GetThingHeight(pThing));
            xRate = 0.0f;
            yRate = 0.12f;
            ripplePos.x -= D3DMultiply(xRate, secDeltaTime);
            ripplePos.y += D3DMultiply(yRate, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_PADDLE_BACK_RIGHT:
            rdVector_Set3(&ripplePos, 0.07f, -0.04f, -sithPhysics_GetThingHeight(pThing));
            xRate = 0.0f;
            yRate = 0.12f;
            ripplePos.x += D3DMultiply(xRate, secDeltaTime);
            ripplePos.y += D3DMultiply(yRate, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_TURN_LEFT:
            rdVector_Set3(&ripplePos, -0.07f, -0.04f, -sithPhysics_GetThingHeight(pThing));
            xRate = 0.0f;
            yRate = 0.12f;
            ripplePos.x -= D3DMultiply(xRate, secDeltaTime);
            ripplePos.y += D3DMultiply(yRate, secDeltaTime);
            break;

        case SITHPLAYERMOVE_RAFT_TURN_RIGHT:
            rdVector_Set3(&ripplePos, 0.07f, -0.04f, -sithPhysics_GetThingHeight(pThing));
            xRate = 0.0f;
            yRate = 0.12f;
            ripplePos.x += D3DMultiply(xRate, secDeltaTime);
            ripplePos.y += D3DMultiply(yRate, secDeltaTime);
            break;

        default:
            return;
    }

    rdMatrix_TransformPoint34Acc(&ripplePos, &pThing->orient);
    rdVector_Add3Acc(&ripplePos, &pThing->pos);
    ripplePos.z += 0.001f;

    bCreateSplashFx = 0;
    if ( secDeltaTime == 0.0f || secDelatLastCreated >= 0.2f )
    {
        bCreateSplashFx = 1;
    }
    else
    {
        dist = rdVector_Dist3(&ripplePos, &sithFX_rowRippleLastPos);
        if ( dist > 0.005f )
        {
            bCreateSplashFx = 1;
        }
    }

    if ( bCreateSplashFx )
    {
        // If not underwater sector, set by sithPhysics_GetWaterNormalAtPos. Note 4 is water surface
        if ( (int32_t)pThing->userval == 3 )
        {
            pTemplate = sithTemplate_GetTemplate("+ripples");
            if ( pTemplate )
            {
                rdVector_Set3(&start, 0.005f, 0.005f, 1.0f);

                rdVector_Set3(&end, 0.03f, 0.03f, 0.0f);

                pSprite = sithThing_CreateThingAtPos(pTemplate, &ripplePos, &pTemplate->orient, pThing->pInSector, NULL);
                if ( !pSprite )
                {
#line 785
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a ripple, no thing space!\n", 0, 0, 0, 0);
                    return;
                }

                // Start sprite animation
                rdVector_Copy3(&pSprite->orient.lvec, &pThing->orient.uvec);
                if ( pSprite )
                {
                    sithAnimate_StartSpriteSizeAnim(pSprite, &start, &end, SITH_RANDF() + 1.0f);
                }

                rdVector_Copy3(&sithFX_rowRippleLastPos, &ripplePos);
                sithFX_secLastCreatedRowRipple = sithTime_secGameTime;
            }
        }
    }

    if ( bCreateSplashFx )
    {
        ripplePos.z += 0.008f;
        sithFX_CreatePaddleSplash(pThing, &ripplePos);
    }
}

void J3DAPI sithFX_CreateRipple(SithThing* pThing, float size)
{
    SithThing* pSprite;
    rdVector3 start;
    SithThing* pTemplate = sithTemplate_GetTemplate("+ripples");
    if ( pTemplate && pThing->attach.flags )
    {
        rdVector3 end;
        rdVector3 ripplePos;
        float endSize;

        endSize = D3DVAL(size * 2.0f);
        rdVector_Set3(&start, size, size, 1.0f);

        rdVector_Set3(&end, endSize, endSize, 0.0f);

        
        ripplePos.x = D3DVAL(D3DMultiply(SITH_RANDF() - 0.5f, D3DMultiply(size, 0.5f)));
        ripplePos.y = D3DVAL(D3DMultiply(SITH_RANDF() - 0.5f, D3DMultiply(size, 0.5f)));
        ripplePos.z = 0.002f;

        rdVector_Add3Acc(&ripplePos, &pThing->pos);
        ripplePos.z -= rdMath_DistancePointToPlane(&pThing->pos, &pThing->attach.pFace->normal, &pThing->attach.attachedFaceFirstVert);

        pSprite = sithThing_CreateThingAtPos(pTemplate, &ripplePos, &pTemplate->orient, pThing->pInSector, NULL);
        if ( !pSprite )
        {
#line 848
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a ripple, no thing space!\n", 0, 0, 0, 0);
            return;
        }

        rdVector_Set3(&pSprite->orient.lvec, 0.0f, 0.0f, 1.0f);
        if ( pSprite )
        {
            sithAnimate_StartSpriteSizeAnim(pSprite, &start, &end, 2.5f);
        }
    }
}

/* Discarded by the retail linker (no caller in either image); restored in full from the debug build
   (0x004B2C85, between CreateRipple and CreateMineCarSparks, __LINE__ 926/941). Its eleven tunables are
   retail's otherwise unreferenced .data 0x00539510..0x00539538, and its two start-offset vectors
   retail's unreferenced .bss 0x00585420 and 0x00585440. Name and signature inferred. */
void J3DAPI sithFx_CreateTireFx(SithThing* pThing, const rdVector3* pPosLeft, rdMaterial* pMatLeft, const rdVector3* pPosRigth, rdMaterial* pMatRight)
{
    SithThing* pLeftParticles  = NULL;
    SithThing* pRightParticles = NULL;
    int bLeft  = 0;
    int bRight = 0;
    rdVector3 dir;
    float speed;
    float dot;
    float yawAngle;
    float chance;
    float leftDist;
    float rightDist;
    SithThing* pTemplate;
    rdVector3 pos;
    static float sithFX_flt_539510 = 0.4f;
    static float sithFX_flt_539514 = 1.0f;
    static float sithFX_flt_539518 = 0.01f;
    static float sithFX_flt_53951C = 0.005f;
    static float sithFX_flt_539520 = 0.001f;
    static float sithFX_flt_539524 = 0.1f;
    static float sithFX_flt_539528 = 0.005f;
    static float sithFX_flt_53952C = 0.025f;
    static float sithFX_flt_539530 = 0.007f;
    static float sithFX_flt_539534 = 0.035f;
    static int sithFX_int_539538 = 200;

    speed = rdVector_Normalize3(&dir, &pThing->moveInfo.physics.velocity);
    dot   = rdVector_Dot3(&pThing->orient.rvec, &dir);

    yawAngle = speed * dot;
    yawAngle *= sithFX_flt_539514;
    yawAngle += pThing->moveInfo.physics.angularVelocity.y * sithFX_flt_539518;

    chance = sithFX_flt_539520;
    chance += (dot < 0.0f ? -dot : dot) * sithFX_flt_539524;
    chance += speed * sithFX_flt_539528;

    leftDist = rdVector_Dist3(pPosLeft, &sithFx_leftTireStartPosOffset);
    if ( leftDist >= sithFX_flt_53951C && SITH_RANDF() < chance )
    {
        bLeft = 1;
    }

    rightDist = rdVector_Dist3(pPosRigth, &sithFx_rightTireStartPosOffset);
    if ( rightDist >= sithFX_flt_53951C && SITH_RANDF() < chance )
    {
        bRight = 1;
    }

    pTemplate = sithTemplate_GetTemplate("paddlesplash");
    if ( pTemplate && (bRight || bLeft) )
    {
        if ( pMatLeft )
        {
            rdVector_Copy3(&pos, pPosLeft);
            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.lvec, -sithFX_flt_53952C);
            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.rvec, sithFX_flt_539530);
            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.uvec, sithFX_flt_539534);

            pLeftParticles = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
            if ( !pLeftParticles )
            {
#line 926
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a tirefx, no thing space!\n", 0, 0, 0, 0);
                return;
            }

            pLeftParticles->renderData.data.pParticle->pMaterial = pMatLeft;
        }

        if ( pMatRight )
        {
            rdVector_Copy3(&pos, pPosRigth);
            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.lvec, -sithFX_flt_53952C);
            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.rvec, -sithFX_flt_539530);
            rdVector_ScaleAdd3Acc(&pos, &pThing->orient.uvec, sithFX_flt_539534);

            pRightParticles = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pThing->pInSector, NULL);
            if ( !pRightParticles )
            {
#line 941
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a tirefx, no thing space!\n", 0, 0, 0, 0);
                return;
            }

            pRightParticles->renderData.data.pParticle->pMaterial = pMatRight;
        }

        if ( pLeftParticles )
        {
            rdVector_Scale3(&pLeftParticles->moveInfo.physics.velocity, &pThing->orient.rvec, yawAngle);
            pLeftParticles->moveInfo.physics.velocity.z += speed * sithFX_flt_539510;
            pLeftParticles->msecLifeLeft = sithFX_int_539538;
        }

        if ( pRightParticles )
        {
            rdVector_Scale3(&pRightParticles->moveInfo.physics.velocity, &pThing->orient.rvec, yawAngle);
            pRightParticles->moveInfo.physics.velocity.z += speed * sithFX_flt_539510;
            pRightParticles->msecLifeLeft = sithFX_int_539538;
        }
    }
}

void J3DAPI sithFX_CreateMineCarSparks(SithThing* pThing, rdVector3* pSparkPos)
{
    rdVector3 pos;
    SithThing* pSparks;

    SithThing* pThingTemplate = sithTemplate_GetTemplate("minecarsparks");
    if ( pThingTemplate )
    {
        static float sithFX_flt_53953C = 0.5f;
        // Note, the spark offset is transformed in place, so the caller's vector is modified
        rdMatrix_TransformPoint34Acc(pSparkPos, &pThing->orient);
        rdVector_Add3(&pos, &pThing->pos, pSparkPos);

        pSparks = sithThing_CreateThingAtPos(pThingTemplate, &pos, &pThingTemplate->orient, pThing->pInSector, NULL);
        if ( !pSparks )
        {
#line 987
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Can't make a spark, no thing space!\n", 0, 0, 0, 0);
            return;
        }

        pSparks->moveInfo.physics.velocity.z = sithFX_flt_53953C;
    }
}

void J3DAPI sithFX_CreateChalkMark(const rdVector3* pPos, const SithSurface* pAttSurf, SithSector* pSector)
{
    size_t markNum;
    int bSearch;
    int slotIndex;
    SithThing* pTemplate;
    size_t nameIdx;

    if ( sithFX_lastChalkMarkNum + 1 >= (int)STD_ARRAYLEN(sithFX_aChalkMarks) )
    {
        sithFX_lastChalkMarkNum = -1;
    }

    markNum = sithFX_lastChalkMarkNum + 1;
    do
    {
        nameIdx = (size_t)D3DMultiply(D3DVAL(SITH_RAND()), STD_ARRAYLEN(sithFX_aChalkMarkNames) - 1);
        for ( slotIndex = 0, bSearch = 0; slotIndex < sithFX_numChalkMarks && !bSearch; slotIndex++ )
        {
            if ( !sithFX_aChalkMarks[slotIndex] )
            {
                break;
            }

            if ( strcmp(sithFX_aChalkMarks[slotIndex]->aName, sithFX_aChalkMarkNames[nameIdx]) == 0 )
            {
                bSearch = 1;
            }
        }
    } while ( bSearch );

    pTemplate = sithTemplate_GetTemplate(sithFX_aChalkMarkNames[nameIdx]);
    if ( pTemplate )
    {
        if ( sithFX_aChalkMarks[markNum] )
        {
            sithThing_RemoveThing(sithWorld_pCurrentWorld, sithFX_aChalkMarks[markNum]);
            sithFX_aChalkMarks[markNum] = NULL;
        }

        sithFX_aChalkMarks[markNum] = sithFX_CreateThingOnSurface(pTemplate, pPos, pAttSurf, pSector);
        if ( sithFX_aChalkMarks[markNum] )
        {
            ++sithFX_lastChalkMarkNum;
            if ( ++sithFX_numChalkMarks > (int)STD_ARRAYLEN(sithFX_aChalkMarks) )
            {
                sithFX_numChalkMarks = STD_ARRAYLEN(sithFX_aChalkMarks);
            }
        }
    }
}

SithThing* J3DAPI sithFX_CreateThingOnSurface(const SithThing* pTemplate, const rdVector3* pPos, const SithSurface* pAttSurf, SithSector* pSector)
{
    rdVector3 offset;
    rdVector3 pos;
    SithThing* pCurThing;

#line 1110
    SITH_ASSERTREL(pTemplate && pAttSurf && pSector);

    
    rdVector_Scale3(&offset, &pAttSurf->face.normal, 0.001f);

    
    rdVector_Add3(&pos, pPos, &offset);

    pCurThing = sithThing_CreateThingAtPos(pTemplate, &pos, &pTemplate->orient, pSector, NULL);
    if ( pCurThing )
    {
        rdVector_Copy3(&pCurThing->pos, &pos);
        rdVector_Copy3(&pCurThing->orient.lvec, &pAttSurf->face.normal);

        // Calculate rvec = lvec X zVector3
        rdVector_Cross3(&pCurThing->orient.rvec, &pCurThing->orient.lvec, &rdroid_zVector3);
        rdVector_Normalize3Acc(&pCurThing->orient.rvec);

        // Calculate uvec = rvec X lvec
        rdVector_Cross3(&pCurThing->orient.uvec, &pCurThing->orient.rvec, &pCurThing->orient.lvec);
        return pCurThing;
    }

    return NULL;
}

void J3DAPI sithFX_UpdatePolyline(SithThing* pThing)
{
    if ( pThing->renderData.type != RD_THING_POLYLINE )
    {
        return;
    }

    if ( !strcmp(pThing->aName, "+laserbeam") )
    {
        return;
    }

    if ( strcmp(pThing->aName, "+lightning") == 0 )
    {
        int numCels;

        pThing->renderData.data.pPolyline->face.texVertOffset.y = pThing->renderData.data.pPolyline->face.texVertOffset.y + ((SITH_RANDF() - 0.7f) * 0.08f);
        numCels = pThing->renderData.data.pPolyline->face.pMaterial->numCels;
        if ( numCels > 1 && SITH_ISFRAMECYCLE(pThing->idx, 4) ) // On every 4th frame random select new cel
        {
            pThing->renderData.data.pPolyline->face.matCelNum = (int32_t)(SITH_RAND() * (double)numCels);
        }
    }
    else if ( strcmp(pThing->aName, "+plcogend") == 0 )
    {
        float lineLen;
        rdVector3 position;
        rdVector3 look;

        rdVector_Copy3(&position, &pThing->pos);
        rdVector_Sub3(&look, &pThing->pParent->pos, &position);
        lineLen = rdVector_Normalize3Acc(&look);

        rdMatrix_BuildFromLook34(&pThing->orient, &look);
        pThing->renderData.data.pPolyline->length = lineLen;
    }
    else if ( strcmp(pThing->aName, "+plcog") == 0 )
    {
        float lineLen;
        rdVector3 position;
        rdVector3 look;

        rdVector_Copy3(&position, &pThing->pos);
        rdVector_Sub3(&look, &pThing->forceMoveStartPos, &position);
        lineLen = rdVector_Normalize3Acc(&look);

        rdMatrix_BuildFromLook34(&pThing->orient, &look);
        pThing->renderData.data.pPolyline->length = lineLen;
    }
}

void J3DAPI sithFX_ResetPolylineTexVertOffset(SithThing* pThing)
{
    if ( pThing->renderData.type != RD_THING_POLYLINE )
    {
        return;
    }

    pThing->renderData.data.pPolyline->face.texVertOffset.x = 0.0f;
    pThing->renderData.data.pPolyline->face.texVertOffset.y = 0.0f;
}

SithThing* J3DAPI sithFX_CreateLaserThing(const SithThing* pSourceThing, const rdVector3* vecOffset, const rdVector3* vecEnd, float baseRadius, float tipRadius, float duration)
{
    SithThing* pBeamThing = NULL;
    SithThing* pTemplate = NULL;
    const char* pMatFilename = "gen_a4sfx_rbbeam_b.mat";
    const char* pTipMat = "gen_a4sfx_rbbeam_b.mat";
    rdVector3 startPnt;
    rdVector3 aimDir;
    rdVector4 colorRgba;
    float length;
    rdPolyline* pPolyline;

    pBeamThing = sithThing_Create(SITH_THING_POLYLINE);
    if ( !pBeamThing )
    {
#line 1252
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create sithThing for Laser\n", 0, 0, 0, 0);
        return NULL;
    }

    pTemplate = sithTemplate_GetTemplate("+laserbeam");
    if ( !pTemplate )
    {
        // Maybe add log
        return NULL;
    }

    sithThing_SetThingBasedOn(pBeamThing, pTemplate);
    pBeamThing->userval = 0.0f;
    pBeamThing->msecLifeLeft = (int32_t)(duration * 1000.0f);

    
    rdVector_Add3(&startPnt, &pSourceThing->pos, vecOffset);

    
    rdVector_Sub3(&aimDir, vecEnd, &startPnt);
    length = rdVector_Normalize3Acc(&aimDir);
    rdMatrix_BuildFromLook34(&pBeamThing->orient, &aimDir);

    rdVector_Set4(&colorRgba, 1.0f, 0.4f, 0.2f, 0.9f);
    pPolyline = rdPolyline_New("+laserbeam", pMatFilename, pTipMat, length, baseRadius, tipRadius, RD_GEOMETRY_FULL, RD_LIGHTING_GOURAUD, &colorRgba);
    if ( !pPolyline )
    {
        sithThing_RemoveThing(sithWorld_pCurrentWorld, pBeamThing);
#line 1290
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create polyline for laser.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdThing_NewEntry(&pBeamThing->renderData, NULL);
    rdThing_SetPolyline(&pBeamThing->renderData, pPolyline);

    // Maybe should call sithFX_ResetPolylineTexVertOffset
    pBeamThing->renderData.data.pPolyline->face.texVertOffset.x = 0.0f;
    pBeamThing->renderData.data.pPolyline->face.texVertOffset.y = 0.0f;

    memset(&pBeamThing->orient.dvec, 0, sizeof(pBeamThing->orient.dvec)); // Reset orientation
    memcpy(&pBeamThing->pos, &startPnt, sizeof(rdVector3));
    sithThing_EnterSector(pBeamThing, pSourceThing->pInSector, 1, 1);
    return pBeamThing;
}

SithThing* J3DAPI sithFX_CreateLightningThing(const SithThing* pSourceThing, const rdVector3* offset, const rdVector3* endPos, float baseRadius, float tipRadius, float duration)
{
    SithThing* pBeamThing = NULL;
    SithThing* pTemplate = NULL;
    const char* pMatFilename = "gen_a4sfx_lightning.mat";
    const char* pTipMat = "gen_a4sfx_lightning.mat";
    rdVector3 startPnt;
    rdVector3 aimDir;
    rdVector4 colorRgba;
    float length;
    rdPolyline* pPolyline;

    pBeamThing = sithThing_Create(SITH_THING_POLYLINE);
    if ( !pBeamThing )
    {
#line 1344
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create sithThing for Lightning\n", 0, 0, 0, 0);
        return NULL;
    }

    pTemplate = sithTemplate_GetTemplate("+lightning");
    if ( !pTemplate )
    {
        return NULL;
    }

    sithThing_SetThingBasedOn(pBeamThing, pTemplate);

    pBeamThing->flags |= SITH_TF_EMITLIGHT;
    pBeamThing->light.color.red   = 0.0f;
    pBeamThing->light.color.green = 0.0f;
    pBeamThing->light.color.blue  = 0.9f;
    pBeamThing->light.color.alpha = 1.0f;

    pBeamThing->msecLifeLeft = (int32_t)(duration * 1000.0f);

    
    rdVector_Add3(&startPnt, &pSourceThing->pos, offset);

    
    rdVector_Sub3(&aimDir, endPos, &startPnt);
    length = rdVector_Normalize3Acc(&aimDir);

    pBeamThing->light.minRadius = length * 2.0f;
    pBeamThing->light.maxRadius = pBeamThing->light.minRadius;

    rdMatrix_BuildFromLook34(&pBeamThing->orient, &aimDir);


    rdVector_Set4(&colorRgba, 1.0f, 0.4f, 0.2f, 0.9f);
    pPolyline = rdPolyline_New("+lightning", pMatFilename, pTipMat, length, baseRadius, tipRadius, RD_GEOMETRY_FULL, RD_LIGHTING_GOURAUD, &colorRgba);
    if ( !pPolyline )
    {
        sithThing_RemoveThing(sithWorld_pCurrentWorld, pBeamThing);
#line 1387
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create polyline for lightning.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdThing_NewEntry(&pBeamThing->renderData, NULL);
    rdThing_SetPolyline(&pBeamThing->renderData, pPolyline);

    memset(&pBeamThing->orient.dvec, 0, sizeof(pBeamThing->orient.dvec)); // Reset orientation
    memcpy(&pBeamThing->pos, &startPnt, sizeof(rdVector3));
    sithThing_EnterSector(pBeamThing, pSourceThing->pInSector, 1, 1);
    return pBeamThing;
}

SithThing* J3DAPI sithFX_CreatePolylineThing(const SithThing* pSourceThing, SithThing* pEndThing, const rdVector3* pEndPos, const rdMaterial* pMaterial, float baseRadius, float tipRadius, float duration)
{
    rdVector3 endPos;
    rdVector3 startPos;
    rdVector3 look;
    float length;
    rdVector4 color;
    SithThing* pThing = NULL;
    SithThing* pTemplate = NULL;
    rdPolyline* pPolyline;

    pThing = sithThing_Create(SITH_THING_POLYLINE);
    if ( !pThing )
    {
#line 1418
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create sithThing for CreatePolylineThing\n", 0, 0, 0, 0);
        return NULL;
    }

    if ( !pMaterial )
    {
#line 1426
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "NULL Material pointer passed to CreatePolylineThing\n", 0, 0, 0, 0);
        sithThing_RemoveThing(sithWorld_pCurrentWorld, pThing);
        return NULL;
    }

    
    if ( pEndThing )
    {
        pTemplate = sithTemplate_GetTemplate("+plcogend");
        if ( !pTemplate )
        {
            return NULL;
        }

        sithThing_SetThingBasedOn(pThing, pTemplate);
        rdVector_Copy3(&endPos, &pEndThing->pos);
        pThing->pParent = pEndThing;
    }
    else
    {
        pTemplate = sithTemplate_GetTemplate("+plcog");
        if ( !pTemplate )
        {
            return NULL;
        }

        sithThing_SetThingBasedOn(pThing, pTemplate);
        rdVector_Copy3(&endPos, pEndPos);
        rdVector_Copy3(&pThing->forceMoveStartPos, &endPos);
    }

    if ( duration > 0.0f )
    {
        pThing->msecLifeLeft = (int32_t)(duration * 1000.0f);
    }

    
    rdVector_Copy3(&startPos, &pSourceThing->pos);

    
    rdVector_Sub3(&look, &endPos, &startPos);
    length = rdVector_Normalize3Acc(&look);
    rdMatrix_BuildFromLook34(&pThing->orient, &look);

    rdVector_Set4(&color, 0.1f, 0.1f, 0.1f, 1.0f);

    pPolyline = rdPolyline_New(pThing->aName, pMaterial->aName, pMaterial->aName, length, baseRadius, tipRadius, RD_GEOMETRY_FULL, RD_LIGHTING_GOURAUD, &color);
    if ( !pPolyline )
    {
        sithThing_RemoveThing(sithWorld_pCurrentWorld, pThing);
#line 1486
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not create polyline for CreatePolylineThing.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdThing_NewEntry(&pThing->renderData, NULL);
    rdThing_SetPolyline(&pThing->renderData, pPolyline);

    memset(&pThing->orient.dvec, 0, sizeof(pThing->orient.dvec)); // Reset orientation
    rdVector_Copy3(&pThing->pos, &startPos);
    sithThing_EnterSector(pThing, pSourceThing->pInSector, 1, 1);

    pThing->alpha = 1.0f;
    return pThing;
}
