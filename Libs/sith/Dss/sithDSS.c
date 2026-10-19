#include "sithDSS.h"

#include <rdroid/Engine/rdPuppet.h>

#include <sith/AI/sithAIMove.h>
#include <sith/Cog/sithCog.h>
#include <sith/Devices/sithComm.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithFX.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithPlayerControls.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Gameplay/sithVehicleControls.h>
#include <sith/Gameplay/sithWhip.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>


// Defined in sithRender.c. Declared here rather than in sithRender.h: adding it to that header
// shifts VC5's x87 scheduling in sithAIUtil_CheckWpntMoveAlign (90.06% -> 80.30%).
extern int sithRender_bResetCameraAspect;

static rdPuppetTrackCallback sithDSS_aPuppetCallbacks[7] = {
    &sithPuppet_DefaultCallback,
    &sithPlayerControls_PuppetCallback,
    &sithVehicleControls_PuppetCallback,
    &sithAIMove_PuppetCallback,
    &sithWhip_WhipClimbPuppetCallback,
    &sithWhip_ClimbDismountPuppetCallback,
    &sithWhip_WhipFirePuppetCallback
};

int J3DAPI sithDSS_SurfaceStatus(const SithSurface* pSurf, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t i;

#line 101
    SITH_ASSERTREL(pSurf);
    SITHDSS_PUSHINT16(sithSurface_GetSurfaceIndex(pSurf));
    SITHDSS_PUSHINT32(pSurf->flags);
    if ( pSurf->face.pMaterial )
    {
        SITHDSS_PUSHINT32(pSurf->face.pMaterial->num);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

    SITHDSS_PUSHINT16(pSurf->face.matCelNum);
    SITHDSS_PUSHFLOAT(pSurf->face.texVertOffset.x);
    SITHDSS_PUSHFLOAT(pSurf->face.texVertOffset.y);
    SITHDSS_PUSHFLOAT(pSurf->face.extraLight.red);
    SITHDSS_PUSHFLOAT(pSurf->face.extraLight.green);
    SITHDSS_PUSHFLOAT(pSurf->face.extraLight.blue);
    SITHDSS_PUSHINT32(pSurf->face.flags);
    SITHDSS_PUSHINT32(pSurf->face.geometryMode);
    SITHDSS_PUSHINT32(pSurf->face.lightingMode);
    if ( pSurf->pAdjoin )
    {
        SITHDSS_PUSHINT32(pSurf->pAdjoin->flags);
    }

    for ( i = 0; i < pSurf->face.numVertices; ++i )
    {
        SITHDSS_PUSHFLOAT(pSurf->aIntensities[i].alpha);
    }

    sithMulti_message.type   = SITHDSS_SURFACESTATUS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSurfaceStatus(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    SithSurface* pSurf;
    size_t index;
    int curcelNumber;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 176
    SITH_ASSERTREL(pMsg && sithWorld_pCurrentWorld);

    index = SITHDSS_POPUINT16();
    if ( index < 0 || index >= sithWorld_pCurrentWorld->numSurfaces )
    {
#line 182
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "DSS::SurfaceStatus..Surface %d out of range.\n", index, 0, 0, 0);
        return 0;
    }

    pSurf = &sithWorld_pCurrentWorld->aSurfaces[index];
    pSurf->flags = SITHDSS_POPINT32();

    index = SITHDSS_POPINT32();
    if ( index >= 0 )
    {
        pSurf->face.pMaterial = sithMaterial_GetMaterialByIndex(index);
    }

    curcelNumber = SITHDSS_POPUINT16();
    if ( curcelNumber != -1 && pSurf->face.pMaterial && curcelNumber < (int)pSurf->face.pMaterial->numCels )
    {
        pSurf->face.matCelNum = curcelNumber;
    }
    else
    {
        pSurf->face.matCelNum = -1;
    }

    pSurf->face.texVertOffset.x  = SITHDSS_POPFLOAT();
    pSurf->face.texVertOffset.y  = SITHDSS_POPFLOAT();
    pSurf->face.extraLight.red   = SITHDSS_POPFLOAT();
    pSurf->face.extraLight.green = SITHDSS_POPFLOAT();
    pSurf->face.extraLight.blue  = SITHDSS_POPFLOAT();
    pSurf->face.flags            = SITHDSS_POPINT32();
    pSurf->face.geometryMode     = SITHDSS_POPINT32();
    pSurf->face.lightingMode     = SITHDSS_POPINT32();

    if ( pSurf->pAdjoin )
    {
        pSurf->pAdjoin->flags = SITHDSS_POPINT32();
    }

    for ( index = 0; index < pSurf->face.numVertices; ++index )
    {
        pSurf->aIntensities[index].alpha = SITHDSS_POPFLOAT();
    }

    return 1;
}

int J3DAPI sithDSS_SectorStatus(const SithSector* pSector, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 253
    SITH_ASSERTREL(pSector);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pSector));
    SITHDSS_PUSHINT32(pSector->flags);
    SITHDSS_PUSHFLOAT(pSector->ambientLight.red);
    SITHDSS_PUSHFLOAT(pSector->ambientLight.green);
    SITHDSS_PUSHFLOAT(pSector->ambientLight.blue);
    SITHDSS_PUSHFLOAT(pSector->extraLight.red);
    SITHDSS_PUSHFLOAT(pSector->extraLight.green);
    SITHDSS_PUSHFLOAT(pSector->extraLight.blue);
    if ( (pSector->flags & SITH_SECTOR_USETHRUST) != 0 )
    {
        SITHDSS_PUSHVEC3(&pSector->thrust);
    }

    sithMulti_message.type = SITHDSS_SECTORSTATUS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_SyncSeenSectors(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t idx;

    if ( !sithWorld_pCurrentWorld )
    {
        return 1;
    }

    for ( idx = 0; idx < sithWorld_pCurrentWorld->numSectors; ++idx )
    {
        SithSector* pSector = &sithWorld_pCurrentWorld->aSectors[idx];
        uint8_t bSeen;
        if ( (pSector->flags & SITH_SECTOR_SEEN) != 0 )
        {
            bSeen = 1;
        }
        else
        {
            bSeen = 0;
        }

        SITHDSS_PUSHUINT8(bSeen);
    }

    sithMulti_message.type = SITHDSS_SYNCSEENSECTORS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncSeenSecors(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    size_t idx;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    if ( !sithWorld_pCurrentWorld )
    {
        return 0;
    }

    for ( idx = 0; idx < sithWorld_pCurrentWorld->numSectors; ++idx )
    {
        SithSector* pSector = &sithWorld_pCurrentWorld->aSectors[idx];
        uint8_t bSeen = SITHDSS_POPUINT8();
        if ( bSeen )
        {
            pSector->flags |= SITH_SECTOR_SEEN;
        }
        else
        {
            pSector->flags &= ~SITH_SECTOR_SEEN;
        }
    }

    return 1;
}

int J3DAPI sithDSS_ProcessSectorStatus(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    size_t sectorIdx;
    SithSector* pSector;
    SithSectorFlag oldFlags;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    sectorIdx = SITHDSS_POPUINT16();
    if ( sectorIdx >= sithWorld_pCurrentWorld->numSectors )
    {
#line 363
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sector %d out of range for DSS:SectorStatus.\n", sectorIdx, 0, 0, 0);
        return 0;
    }

    pSector = &sithWorld_pCurrentWorld->aSectors[sectorIdx];

    oldFlags = pSector->flags;
    pSector->flags = SITHDSS_POPINT32();
    if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 && (oldFlags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_HideSectorAdjoins(pSector);
    }
    else if ( (oldFlags & SITH_SECTOR_ADJOINSOFF) != 0 && (pSector->flags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_ShowSectorAdjoins(pSector);
    }

    pSector->ambientLight.red   = SITHDSS_POPFLOAT();
    pSector->ambientLight.green = SITHDSS_POPFLOAT();
    pSector->ambientLight.blue  = SITHDSS_POPFLOAT();
    pSector->ambientLight.alpha = 0.0f;

    pSector->extraLight.red   = SITHDSS_POPFLOAT();
    pSector->extraLight.green = SITHDSS_POPFLOAT();
    pSector->extraLight.blue  = SITHDSS_POPFLOAT();
    pSector->extraLight.alpha = 0.0f;

    if ( (pSector->flags & SITH_SECTOR_USETHRUST) != 0 )
    {
        SITHDSS_POPVEC3(&pSector->thrust);
    }
    else
    {
        memset(&pSector->thrust, 0, sizeof(pSector->thrust));
    }

    return 1;
}

int J3DAPI sithDSS_SectorFlags(const SithSector* pSector, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 422
    SITH_ASSERTREL(pSector);
    SITHDSS_PUSHINT16(sithSector_GetSectorIndex(pSector));
    SITHDSS_PUSHINT32(pSector->flags);
    sithMulti_message.type = SITHDSS_SECTORFLAGS;
    SITHDSS_ENDOUT;

    if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 )
    {
        return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 0);
    }

    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSectorFlags(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    int sectorIdx;
    SithSector* pSector;
    SithSectorFlag oldFlags;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    sectorIdx = SITHDSS_POPUINT16();
    pSector   = sithSector_GetSectorEx(sithWorld_pCurrentWorld, sectorIdx);
    if ( !pSector )
    {
#line 470
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Sector %d out of range for DSS:SectorStatus.\n", sectorIdx, 0, 0, 0);
        return 0;
    }

    oldFlags = pSector->flags;
    pSector->flags = SITHDSS_POPUINT32();
    if ( (pSector->flags & SITH_SECTOR_ADJOINSOFF) != 0 && (oldFlags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_HideSectorAdjoins(pSector);
    }
    else if ( (oldFlags & SITH_SECTOR_ADJOINSOFF) != 0 && (pSector->flags & SITH_SECTOR_ADJOINSOFF) == 0 )
    {
        sithSector_ShowSectorAdjoins(pSector);
    }

    return 1;
}

int J3DAPI sithDSS_AIStatus(const SithAIControlBlock* pLocal, DPID idTo, signed int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t i;
    size_t j;

#line 507
    SITH_ASSERTREL(pLocal && pLocal->pOwner && pLocal->pClass);

    SITHDSS_PUSHINT16(pLocal->pOwner->idx);
    SITHDSS_PUSHINT16(pLocal->pClass - sithWorld_pCurrentWorld->aAIClasses);
    SITHDSS_PUSHINT32(pLocal->mode);
    SITHDSS_PUSHINT32(pLocal->submode);
    SITHDSS_PUSHINT32(pLocal->msecNextUpdate);

    SITHDSS_PUSHINT16(sithThing_GetThingIndex(pLocal->goalThing));
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown6);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown5);
    SITHDSS_PUSHINT32(pLocal->unknown150);

    SITHDSS_PUSHINT16(sithThing_GetThingIndex(pLocal->pTargetThing));
    SITHDSS_PUSHVEC3(&pLocal->targetPos);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown122);
    SITHDSS_PUSHINT32(pLocal->msecAttackStart);
    SITHDSS_PUSHFLOAT(pLocal->moveSpeed);

    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        SITHDSS_PUSHVEC3(&pLocal->movePos);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 )
    {
        SITHDSS_PUSHVEC3(&pLocal->goalLVec);
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        SITHDSS_PUSHINT16(sithThing_GetThingIndex(pLocal->pFleeFromThing));
    }

    SITHDSS_PUSHVEC3(&pLocal->homePos);
    SITHDSS_PUSHVEC3(&pLocal->homeOrient);

    for ( i = 0; i < pLocal->numInstincts; ++i )
    {
        SITHDSS_PUSHINT32(pLocal->aInstinctStates[i].flags);
        SITHDSS_PUSHINT32(pLocal->aInstinctStates[i].msecNextUpdate);
        for ( j = 0; j < STD_ARRAYLEN(pLocal->aInstinctStates[i].aParams); ++j )
        {
            SITHDSS_PUSHFLOAT(pLocal->aInstinctStates[i].aParams[j]);
        }
    }

    SITHDSS_PUSHINT32(pLocal->msecFireWaitTime);
    SITHDSS_PUSHINT32(pLocal->msecPauseMoveUntil);
    SITHDSS_PUSHINT32(pLocal->numFrames);
    for ( i = 0; i < pLocal->numFrames; ++i )
    {
        SITHDSS_PUSHVEC3(&pLocal->aFrames[i]);
    }

    SITHDSS_PUSHINT32(pLocal->allowedSurfaceTypes);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown3);
    SITHDSS_PUSHVEC3(&pLocal->vecUnknown);
    SITHDSS_PUSHFLOAT(pLocal->maxHomeDist);

    sithMulti_message.type   = SITHDSS_AISTATUS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessAIStatus(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    SithWorld* pWorld;
    size_t thingIdx;
    SithThing* pThing;
    SithAIControlBlock* pLocal;
    size_t i;
    size_t j;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    pWorld = sithWorld_pCurrentWorld;
#line 617
    SITH_ASSERTREL(pWorld && pMsg);

    thingIdx = SITHDSS_POPUINT16();
    pThing   = sithThing_GetThingByIndex(thingIdx);
    if ( !pThing )
    {
#line 623
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "DSS::AiStatus - Bad thing index %d.\n", thingIdx, 0, 0, 0);
        return 0;
    }

    if ( pThing->controlType != SITH_CT_AI || !pThing->controlInfo.aiControl.pLocal )
    {
#line 629
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "DSS::AiStatus - Non-AI thing index %d.\n", thingIdx, 0, 0, 0);
        return 0;
    }

    pLocal = pThing->controlInfo.aiControl.pLocal;
    thingIdx = SITHDSS_POPUINT16();
    if ( thingIdx < 0 || thingIdx >= pWorld->numAIClasses )
    {
        return 0;
    }

    pLocal->pClass       = &pWorld->aAIClasses[thingIdx];
    pLocal->numInstincts = pLocal->pClass->numInstincts;
    pLocal->mode         = SITHDSS_POPINT32();
    pLocal->submode      = SITHDSS_POPINT32();
    pLocal->msecNextUpdate = SITHDSS_POPINT32();

    thingIdx = SITHDSS_POPUINT16();
    pLocal->goalThing = sithThing_GetThingByIndex(thingIdx);
    SITHDSS_POPVEC3(&pLocal->vecUnknown6);
    SITHDSS_POPVEC3(&pLocal->vecUnknown5);
    pLocal->unknown150 = SITHDSS_POPINT32();
    pLocal->unknown141 = 0;

    thingIdx = SITHDSS_POPUINT16();
    pLocal->pTargetThing = sithThing_GetThingByIndex(thingIdx);
    SITHDSS_POPVEC3(&pLocal->targetPos);
    SITHDSS_POPVEC3(&pLocal->vecUnknown122);
    pLocal->msecAttackStart = SITHDSS_POPINT32();
    pLocal->unknown124      = 0;
    pLocal->moveSpeed       = SITHDSS_POPFLOAT();

    if ( (pLocal->mode & SITHAI_MODE_MOVING) != 0 )
    {
        SITHDSS_POPVEC3(&pLocal->movePos);
    }

    if ( (pLocal->mode & SITHAI_MODE_TURNING) != 0 )
    {
        SITHDSS_POPVEC3(&pLocal->goalLVec);
    }

    if ( (pLocal->mode & SITHAI_MODE_FLEEING) != 0 )
    {
        thingIdx = SITHDSS_POPUINT16();
    pLocal->pFleeFromThing = sithThing_GetThingByIndex(thingIdx);
    }

    SITHDSS_POPVEC3(&pLocal->homePos);
    SITHDSS_POPVEC3(&pLocal->homeOrient);

    for ( i = 0; i < pLocal->numInstincts; ++i )
    {
        pLocal->aInstinctStates[i].flags          = SITHDSS_POPINT32();
        pLocal->aInstinctStates[i].msecNextUpdate = SITHDSS_POPINT32();
        for ( j = 0; j < STD_ARRAYLEN(pLocal->aInstinctStates[i].aParams); ++j )
        {
            pLocal->aInstinctStates[i].aParams[j] = SITHDSS_POPFLOAT();
        }
    }

    pLocal->msecFireWaitTime   = SITHDSS_POPINT32();
    pLocal->msecPauseMoveUntil = SITHDSS_POPINT32();
    pLocal->numFrames          = SITHDSS_POPINT32();
#line 707
    if ( pLocal->numFrames && (pLocal->aFrames = (rdVector3*)STDMALLOC(sizeof(rdVector3) * pLocal->numFrames)) != NULL )
    {
        pLocal->sizeFrames = pLocal->numFrames;
        for ( i = 0; i < pLocal->numFrames; ++i )
        {
            SITHDSS_POPVEC3(&pLocal->aFrames[i]);
        }
    }
    else
    {
        pLocal->sizeFrames = 0;
        pLocal->numFrames  = 0;
    }

    pLocal->allowedSurfaceTypes = SITHDSS_POPINT32();
    SITHDSS_POPVEC3(&pLocal->vecUnknown3);
    SITHDSS_POPVEC3(&pLocal->vecUnknown);
    pLocal->maxHomeDist = SITHDSS_POPFLOAT();
    return 1;
}

int J3DAPI sithDSS_Inventory(const SithThing* pThing, unsigned int inventoryId, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    SithInventoryItem* pCurItems;

#line 756
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL(inventoryId < 200);

    if ( pThing->type != SITH_THING_PLAYER && pThing->type != SITH_THING_ACTOR )
    {
        return 0;
    }

    if ( !pThing->thingInfo.actorInfo.pPlayer )
    {
        return 0;
    }

    pCurItems = pThing->thingInfo.actorInfo.pPlayer->inventory.aItems;

    SITHDSS_PUSHINT16(sithThing_GetThingIndex(pThing));
    SITHDSS_PUSHINT16(inventoryId);
    SITHDSS_PUSHFLOAT(pCurItems[inventoryId].amount);
    SITHDSS_PUSHINT32(pCurItems[inventoryId].status);
    sithMulti_message.type = SITHDSS_INVENTORY;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessInventory(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    SithThing* pThing;
    SithInventoryItem* pCurItems;
    int n;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 813
    SITH_ASSERTREL(pMsg);

    n = SITHDSS_POPUINT16();
    pThing = sithThing_GetThingByIndex(n);
    if ( !pThing )
    {
        goto error;
    }

    if ( pThing->type != SITH_THING_ACTOR && pThing->type != SITH_THING_PLAYER )
    {
        goto error;
    }

    if ( !pThing->thingInfo.actorInfo.pPlayer )
    {
        goto error;
    }

    pCurItems = pThing->thingInfo.actorInfo.pPlayer->inventory.aItems;
    n = SITHDSS_POPUINT16();
    if ( n < 0 || n >= SITHINVENTORY_MAXTYPES )
    {
        goto error;
    }

    pCurItems[n].amount = SITHDSS_POPFLOAT();
    pCurItems[n].status = SITHDSS_POPINT32();
    return 1;

error:
#line 842
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error processing DSS::Inventory message.\n", 0, 0, 0, 0);
    return 0;
}

int J3DAPI sithDSS_AnimStatus(const SithAnimationSlot* pAnim, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];

#line 865
    SITH_ASSERTREL(pAnim);
    SITHDSS_PUSHINT32(pAnim->animID);
    SITHDSS_PUSHINT32(pAnim->flags);

    if ( (pAnim->flags & (SITHANIMATE_THING | SITHANIMATE_SPRITE)) != 0 )
    {
        SITHDSS_PUSHINT16(sithThing_GetThingIndex(pAnim->pThing));
        SITHDSS_PUSHINT32(pAnim->thingSignature);
    }

    if ( (pAnim->flags & SITHANIMATE_CAMERA_ZOOM) != 0 )
    {
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->curValue);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVE) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->curValue);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVEPOS) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHFLOAT(pAnim->secTimeRemaining);
        SITHDSS_PUSHVEC3(&pAnim->thingEndPosition);
    }

    if ( (pAnim->flags & (SITHANIMATE_PUSHITEM | SITHANIMATE_PULLITEM)) != 0 )
    {
        SITHDSS_PUSHINT16(sithThing_GetThingIndex(pAnim->pItemThing));
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->curValue);
        SITHDSS_PUSHINT32(pAnim->curFrame);
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
        SITHDSS_PUSHINT32(pAnim->trackNum);
        SITHDSS_PUSHINT32(pAnim->msecPerFrame);
        SITHDSS_PUSHINT32(sithSurface_GetSurfaceIndex(pAnim->pSurface));
        SITHDSS_PUSHVEC3(&pAnim->direction3);
    }

    if ( (pAnim->flags & SITHANIMATE_SPRITE_SIZE) != 0 )
    {
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->curVector);
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_QUICKTURN) != 0 )
    {
        SITHDSS_PUSHFLOAT(pAnim->curValue);
        SITHDSS_PUSHFLOAT(pAnim->endValue);
        SITHDSS_PUSHFLOAT(pAnim->deltaValue);
    }

    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
    {
#line 941
        SITH_ASSERTREL(pAnim->pSurface);
        SITHDSS_PUSHINT32(sithSurface_GetSurfaceIndex(pAnim->pSurface));
    }

    if ( (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
    {
        SITHDSS_PUSHVEC3(&pAnim->direction3);
        SITHDSS_PUSHFLOAT(pAnim->direction2.x);
        SITHDSS_PUSHFLOAT(pAnim->direction2.y);
    }

    if ( (pAnim->flags & SITHANIMATE_PAGEFLIP) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->msecNextFrameTime);
        SITHDSS_PUSHINT32(pAnim->msecPerFrame);
        SITHDSS_PUSHINT32(pAnim->curFrame);
    }

    if ( (pAnim->flags & SITHANIMATE_LIGHT) != 0 )
    {
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
        SITHDSS_PUSHVEC4(&pAnim->curVector);
    }

    if ( (pAnim->flags & SITHANIMATE_MATERIAL) != 0 )
    {
        SITHDSS_PUSHINT32(pAnim->pMaterial->num);
    }

    if ( (pAnim->flags & SITHANIMATE_SECTOR) != 0 )
    {
        SITHDSS_PUSHINT32(sithSector_GetSectorIndex(pAnim->pSector));
    }

    if ( (pAnim->flags & SITHANIMATE_THING_FADE) != 0 )
    {
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(pAnim->pThing));
        SITHDSS_PUSHINT32(pAnim->thingSignature);
        SITHDSS_PUSHVEC4(&pAnim->startVector);
        SITHDSS_PUSHVEC4(&pAnim->curVector);
        SITHDSS_PUSHVEC4(&pAnim->deltaVector);
        SITHDSS_PUSHVEC4(&pAnim->endVector);
    }

    sithMulti_message.type   = SITHDSS_ANIMSTATUS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessAnimStatus(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    SithAnimationSlot* pAnim;
    int16_t idx16;
    int animIdx;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
    animIdx   = SITHDSS_POPINT32();
    pAnim = sithAnimate_GetAnim(animIdx);
    if ( !pAnim )
    {
        pAnim = sithAnimate_Create();
    }

    if ( !pAnim )
    {
        return 0;
    }

    pAnim->flags = SITHDSS_POPINT32();
    if ( !pAnim->flags )
    {
        sithAnimate_Stop(pAnim);
        return 1;
    }

    if ( (pAnim->flags & (SITHANIMATE_THING | SITHANIMATE_SPRITE)) != 0 )
    {
        idx16 = SITHDSS_POPINT16();
        pAnim->pThing = sithThing_GetThingByIndex(idx16);
        if ( pAnim->pThing && pAnim->pThing->renderData.type == RD_THING_SPRITE3 )
        {
            pAnim->pMaterial = pAnim->pThing->renderData.data.pSprite3->face.pMaterial;
        }

        pAnim->thingSignature = SITHDSS_POPINT32();
    }

    if ( (pAnim->flags & SITHANIMATE_CAMERA_ZOOM) != 0 )
    {
        pAnim->deltaValue = SITHDSS_POPFLOAT();
        pAnim->endValue   = SITHDSS_POPFLOAT();
        pAnim->curValue   = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVE) != 0 )
    {
        pAnim->thingSignature = SITHDSS_POPINT32();
        pAnim->deltaValue     = SITHDSS_POPFLOAT();
        pAnim->endValue       = SITHDSS_POPFLOAT();
        pAnim->curValue       = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_THING_MOVEPOS) != 0 )
    {
        pAnim->thingSignature   = SITHDSS_POPINT32();
        pAnim->secTimeRemaining = SITHDSS_POPFLOAT();
        SITHDSS_POPVEC3(&pAnim->thingEndPosition);
    }

    if ( (pAnim->flags & (SITHANIMATE_PUSHITEM | SITHANIMATE_PULLITEM)) != 0 )
    {
        idx16 = SITHDSS_POPINT16();
        pAnim->pItemThing     = sithThing_GetThingByIndex(idx16);
        pAnim->thingSignature = SITHDSS_POPINT32();
        pAnim->deltaValue     = SITHDSS_POPFLOAT();
        pAnim->endValue       = SITHDSS_POPFLOAT();
        pAnim->curValue       = SITHDSS_POPFLOAT();
        pAnim->curFrame       = SITHDSS_POPINT32();
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
        pAnim->trackNum     = SITHDSS_POPINT32();
        pAnim->msecPerFrame = SITHDSS_POPINT32();
        animIdx = SITHDSS_POPINT32();
        pAnim->pSurface     = sithSurface_GetSurfaceEx(sithWorld_pCurrentWorld, animIdx);
        SITHDSS_POPVEC3(&pAnim->direction3);
    }

    if ( (pAnim->flags & SITHANIMATE_SPRITE_SIZE) != 0 )
    {
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->curVector);
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_QUICKTURN) != 0 )
    {
        pAnim->curValue   = SITHDSS_POPFLOAT();
        pAnim->endValue   = SITHDSS_POPFLOAT();
        pAnim->deltaValue = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_SURFACE) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pSurface = sithSurface_GetSurfaceEx(sithWorld_pCurrentWorld, animIdx);
        if ( pAnim->pSurface )
        {
            pAnim->pMaterial = pAnim->pSurface->face.pMaterial;
        }
    }

    if ( (pAnim->flags & SITHANIMATE_SCROLL) != 0 )
    {
        SITHDSS_POPVEC3(&pAnim->direction3);
        pAnim->direction2.x = SITHDSS_POPFLOAT();
        pAnim->direction2.y = SITHDSS_POPFLOAT();
    }

    if ( (pAnim->flags & SITHANIMATE_PAGEFLIP) != 0 )
    {
        pAnim->msecNextFrameTime = SITHDSS_POPINT32();
        pAnim->msecPerFrame      = SITHDSS_POPINT32();
        pAnim->curFrame          = SITHDSS_POPINT32();
    }

    if ( (pAnim->flags & SITHANIMATE_LIGHT) != 0 )
    {
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
        SITHDSS_POPVEC4(&pAnim->curVector);
    }

    if ( (pAnim->flags & SITHANIMATE_MATERIAL) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pMaterial = sithMaterial_GetMaterialByIndex(animIdx);
    }

    if ( (pAnim->flags & SITHANIMATE_SECTOR) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pSector = sithSector_GetSectorEx(sithWorld_pCurrentWorld, animIdx);
    }

    if ( (pAnim->flags & SITHANIMATE_THING_FADE) != 0 )
    {
        animIdx = SITHDSS_POPINT32();
        pAnim->pThing         = sithThing_GetThingByIndex(animIdx);
        pAnim->thingSignature = SITHDSS_POPINT32();
        SITHDSS_POPVEC4(&pAnim->startVector);
        SITHDSS_POPVEC4(&pAnim->curVector);
        SITHDSS_POPVEC4(&pAnim->deltaVector);
        SITHDSS_POPVEC4(&pAnim->endVector);
    }

    return 1;
}

int J3DAPI sithDSS_PuppetStatus(const SithThing* pThing, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t trackCount = 0;
    SithPuppetTrack* pCurTrack = NULL;
    rdPuppet* pPuppet;
    size_t i;

#line 1198
    SITH_ASSERTREL(pThing && pThing->renderData.pPuppet);
    pPuppet = pThing->renderData.pPuppet;

    SITHDSS_PUSHINT32(sithThing_GetThingIndex(pThing));
    for ( i = 0; i < STD_ARRAYLEN(pPuppet->aTracks); ++i )
    {
        SITHDSS_PUSHINT32(pPuppet->aTracks[i].status);
        if ( pPuppet->aTracks[i].status )
        {
#line 1210
            SITH_ASSERTREL(pPuppet->aTracks[i].pKFTrack);

            SITHDSS_PUSHINT32(pPuppet->aTracks[i].pKFTrack->idx);
            SITHDSS_PUSHINT32(pPuppet->aTracks[i].unknown0);
            SITHDSS_PUSHINT16(pPuppet->aTracks[i].lowPriority);
            SITHDSS_PUSHINT16(pPuppet->aTracks[i].highPriority);
            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].fps);
            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].playSpeed);
            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].curFrame);
            SITHDSS_PUSHFLOAT(pPuppet->aTracks[i].prevFrame);

            if ( !pPuppet->aTracks[i].pfCallback )
            {
                SITHDSS_PUSHUINT8(STD_ARRAYLEN(sithDSS_aPuppetCallbacks)); // mark there is no callback
            }
            else
            {
                size_t cbIdx;
                for ( cbIdx = 0; cbIdx < STD_ARRAYLEN(sithDSS_aPuppetCallbacks); ++cbIdx )
                {
                    if ( pPuppet->aTracks[i].pfCallback == sithDSS_aPuppetCallbacks[cbIdx] )
                    {
                        SITHDSS_PUSHUINT8(cbIdx);
                        break;
                    }
                }

                if ( cbIdx == STD_ARRAYLEN(sithDSS_aPuppetCallbacks) )
                {
#line 1237
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Saving Unknown callback for %s\n", pPuppet->aTracks[i].pKFTrack->aName, 0, 0, 0);
                    SITHDSS_PUSHUINT8(STD_ARRAYLEN(sithDSS_aPuppetCallbacks)); // mark there is no callback
                }
            }

            ++trackCount;
        }
    }

    if ( pThing->pPuppetState )
    {
        SITHDSS_PUSHINT16(pThing->pPuppetState->armedMode);
        SITHDSS_PUSHINT16(pThing->pPuppetState->moveMode);
        pCurTrack = pThing->pPuppetState->pFirstTrack;
        SITHDSS_PUSHINT16(trackCount);

        while ( trackCount )
        {
            if ( pCurTrack )
            {
                SITHDSS_PUSHINT32(pCurTrack->trackNum);
                SITHDSS_PUSHUINT32(pCurTrack->submode);
                pCurTrack = pCurTrack->pNextTrack;
            }
            else
            {
                SITHDSS_PUSHUINT32(STD_ARRAYLEN(pPuppet->aTracks)); // mark null track
                SITHDSS_PUSHUINT32(SITH_PUPPET_NUMSUBMODES);        // mark no mode
            }

            --trackCount;
        }
    }

    sithMulti_message.type   = SITHDSS_PUPPETSTATUS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessPuppetStatus(const SithMessage* pMsg)
{
    SithThing* pThing;
    rdPuppet* pPuppet;
    unsigned int mode;
    int bControlsDisabled;
    size_t trackCount;
    size_t i;
    int idx;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1306
    SITH_ASSERTREL(pMsg);

    idx    = SITHDSS_POPINT32();
    pThing = sithThing_GetThingByIndex(idx);
    if ( !pThing )
    {
        goto error;
    }

    if ( !pThing->pPuppetClass || !pThing->pPuppetState || !pThing->renderData.pPuppet )
    {
        goto error;
    }

    pPuppet = pThing->renderData.pPuppet;
    rdPuppet_NewEntry(pPuppet, &pThing->renderData);

    for ( i = 0; i < STD_ARRAYLEN(pPuppet->aTracks); ++i )
    {
        pPuppet->aTracks[i].status = SITHDSS_POPINT32();
        if ( pPuppet->aTracks[i].status )
        {
            size_t cbIdx;

            idx = SITHDSS_POPINT32();
            pPuppet->aTracks[i].pKFTrack     = sithPuppet_GetKeyframeByIndex(idx);
            pPuppet->aTracks[i].unknown0     = SITHDSS_POPINT32();
            pPuppet->aTracks[i].lowPriority  = SITHDSS_POPUINT16();
            pPuppet->aTracks[i].highPriority = SITHDSS_POPUINT16();
            pPuppet->aTracks[i].fps          = SITHDSS_POPFLOAT();
            pPuppet->aTracks[i].playSpeed    = SITHDSS_POPFLOAT();
            pPuppet->aTracks[i].curFrame     = SITHDSS_POPFLOAT();
            pPuppet->aTracks[i].prevFrame    = SITHDSS_POPFLOAT();

            cbIdx = SITHDSS_POPUINT8();
            if ( cbIdx >= STD_ARRAYLEN(sithDSS_aPuppetCallbacks) )
            {
                pPuppet->aTracks[i].pfCallback = NULL;
            }
            else
            {
                pPuppet->aTracks[i].pfCallback = sithDSS_aPuppetCallbacks[cbIdx];
            }
        }
    }

    if ( pThing->pPuppetState )
    {
        mode = SITHDSS_POPUINT16();
        sithPuppet_SetArmedMode(pThing, mode);

        mode = SITHDSS_POPUINT16();
        if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
        {
            bControlsDisabled = pThing->thingInfo.actorInfo.bControlsDisabled;
        }

        sithPuppet_SetMoveMode(pThing, mode);
        if ( pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR )
        {
            pThing->thingInfo.actorInfo.bControlsDisabled = bControlsDisabled;
        }

        trackCount = SITHDSS_POPUINT16();
        while ( trackCount )
        {
            size_t trackNum = SITHDSS_POPINT32();
            SithPuppetSubMode submode = SITHDSS_POPUINT32();
            if ( trackNum < STD_ARRAYLEN(pPuppet->aTracks) )
            {
                SithPuppetClassSubmode* pSubmode = &pThing->pPuppetClass->aModes[pThing->pPuppetState->majorMode][submode];
                int bTrack = sithPuppet_NewTrack(pThing, pSubmode, trackNum, submode);
                if ( bTrack )
                {
                    return 0;
                }
            }

            --trackCount;
        }
    }

    SITHDSS_ENDIN;
    return 1;

error:
#line 1395
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error in DSS::PuppetStatus message format.\n", 0, 0, 0, 0);
    return 0;
}

int J3DAPI sithDSS_SyncTaskEvents(const SithEvent* pEvent, DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t i;

    SITHDSS_PUSHINT32(pEvent->msecEventTime - sithTime_msecGameTime);
    for ( i = 0; i < 4; ++i )
    {
        SITHDSS_PUSHINT32(((int32_t*)&pEvent->params)[i]);
    }
    SITHDSS_PUSHUINT16(pEvent->taskNum);
    sithMulti_message.type = SITHDSS_SYNCTASKEVENTS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncTaskEvents(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    uint32_t msecDelay;
    SithEventParams param;
    size_t i;
    size_t taskNum;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1428
    SITH_ASSERTREL(pMsg);

    msecDelay = SITHDSS_POPINT32();
    for ( i = 0; i < 4; ++i )
    {
        ((int32_t*)&param)[i] = SITHDSS_POPINT32();
    }
    taskNum   = SITHDSS_POPUINT16();
    sithEvent_CreateEvent(taskNum, &param, msecDelay);
    return 1;
}

int J3DAPI sithDSS_SyncCameras(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t i;
    SithCamera* pCamera;

    SITHDSS_PUSHINT16(sithCamera_pCurCamera - sithCamera_aCameras);
    SITHDSS_PUSHINT32(sithCamera_camInfo.bCurCameraSet);
    SITHDSS_PUSHINT32(sithCamera_camInfo.curCycleCamNum);
    SITHDSS_PUSHVEC3(&sithCamera_camInfo.povShake.vecPosOffset);
    SITHDSS_PUSHVEC3(&sithCamera_camInfo.povShake.vecAngleOffset);
    SITHDSS_PUSHFLOAT(sithCamera_camInfo.povShake.posDelta);
    SITHDSS_PUSHFLOAT(sithCamera_camInfo.povShake.angleDelta);

    for ( i = 0; i < STD_ARRAYLEN(sithCamera_aCameras); ++i )
    {
        pCamera = &sithCamera_aCameras[i];
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(pCamera->pPrimaryFocusThing));
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(pCamera->pSecondaryFocusThing));
        SITHDSS_PUSHFLOAT(pCamera->fov);
    }

#line 1471
    SITH_ASSERTREL(sithPlayer_pLocalPlayer);
    sithMulti_message.type   = SITHDSS_SYNCCAMERAS;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncCameras(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    size_t i;
    SithCamera* pCamera;
    size_t cameraNum;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1486
    SITH_ASSERTREL(pMsg);

    cameraNum = SITHDSS_POPUINT16();
    cameraNum = STDMATH_CLAMP(cameraNum, 0, 6);
    sithCamera_pCurCamera     = &sithCamera_aCameras[cameraNum];
    sithCamera_camInfo.bCurCameraSet  = SITHDSS_POPINT32();
    sithCamera_camInfo.curCycleCamNum = SITHDSS_POPINT32();
    SITHDSS_POPVEC3(&sithCamera_camInfo.povShake.vecPosOffset);
    SITHDSS_POPVEC3(&sithCamera_camInfo.povShake.vecAngleOffset);
    sithCamera_camInfo.povShake.posDelta   = SITHDSS_POPFLOAT();
    sithCamera_camInfo.povShake.angleDelta = SITHDSS_POPFLOAT();

    for ( i = 0; i < STD_ARRAYLEN(sithCamera_aCameras); ++i )
    {
        pCamera = &sithCamera_aCameras[i];
        pCamera->pSector = NULL;
        cameraNum = SITHDSS_POPINT32();
        pCamera->pPrimaryFocusThing = sithThing_GetThingByIndex(cameraNum);
        cameraNum = SITHDSS_POPINT32();
        pCamera->pSecondaryFocusThing = sithThing_GetThingByIndex(cameraNum);
        pCamera->fov = SITHDSS_POPFLOAT();
    }

    return 1;
}

int J3DAPI sithDSS_SyncGameState(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    int n;
    const SithWorld* pWorld;
    const SithInventoryType* currentType;

#line 1528
    SITH_ASSERTREL(sithWorld_pStaticWorld && sithWorld_pCurrentWorld);

    if ( sithCog_pMasterCog )
    {
        SITHDSS_PUSHINT32(sithCog_pMasterCog->idx);
    }
    else
    {
        SITHDSS_PUSHINT32(-1);
    }

#line 1536
    SITH_ASSERTREL(sithPlayer_pLocalPlayer);
    SITHDSS_PUSHINT32(sithPlayer_pLocalPlayer->inventory.curItemID);
    SITHDSS_PUSHINT32(sithPlayer_pLocalPlayer->inventory.curWeaponID);

    for ( n = 0; n < (int)STD_ARRAYLEN(sithInventory_aPowerKeybinds); ++n )
    {
        SITHDSS_PUSHINT32(sithInventory_aPowerKeybinds[n].bActivated);
    }

    SITHDSS_PUSHINT32(sithInventory_bSendDeactivateMessage);
    SITHDSS_PUSHINT32(sithInventory_bInitInventory);
    SITHDSS_PUSHINT32(sithInventory_dword_56B750);
    SITHDSS_PUSHINT32(sithInventory_dword_56B754);

    for ( n = 2; n < 11; ++n )
    {
        currentType = &sithInventory_aTypes[n];
        if ( currentType->pHolsterModel )
        {
            SITHDSS_PUSHINT16(sithModel_GetModelIndex(currentType->pHolsterModel));
        }
        else
        {
            SITHDSS_PUSHINT16(-1);
        }
    }

    SITHDSS_PUSHINT32(sithRender_bResetCameraAspect);
    SITHDSS_PUSHINT32(sithThing_GetThingIndex(sithPlayerControls_GetVehicleBoardedThing()));
    SITHDSS_PUSHINT16(sithSurface_GetSurfaceIndex(sithPlayerActions_pCurCollidedSurface));
    SITHDSS_PUSHINT16(sithModel_GetModelIndex(sithPlayerActions_pCurCollidedModel));
    if ( sithPlayerActions_pCurCollidedModel )
    {
        SITHDSS_PUSHINT16(sithPlayerActions_pCurCollidedModelFace - sithPlayerActions_pCurCollidedModel->aGeos[0].aMeshes->aFaces);
    }

    SITHDSS_PUSHINT32(sithPlayer_bPlayerInPor);
    SITHDSS_PUSHFLOAT(sithPlayer_impState);
    SITHDSS_PUSHUINT8(sithPlayer_bInAetheriumSector);
    SITHDSS_PUSHUINT8(sithPlayer_bGuybrush);
    SITHDSS_PUSHINT16(sithPlayer_impFireType);
    SITHDSS_PUSHINT16(sithPlayer_curLevelNum);
    SITHDSS_PUSHUINT8(sithPlayer_bBonusMapBought);
    SITHDSS_PUSHINT32(sithThing_GetThingIndex(sithPlayerActions_pPlasma));
    SITHDSS_PUSHINT16(sithPlayerActions_jewelFlyingPuppetTrackNum);
    SITHDSS_PUSHUINT8(sithPlayerActions_bJewelFlying);
    SITHDSS_PUSHUINT8(sithPlayerActions_bPlayerInvisible);
    SITHDSS_PUSHUINT8(sithPlayerControls_bCutsceneMode);
    SITHDSS_PUSHUINT8(sithPuppet_bPlayerLeapForward);

    SITHDSS_PUSHINT32(sithFX_numChalkMarks);
    SITHDSS_PUSHINT32(sithFX_lastChalkMarkNum);
    for ( n = 0; n < sithFX_numChalkMarks; ++n )
    {
        SITHDSS_PUSHINT32(sithThing_GetThingIndex(sithFX_aChalkMarks[n]));
    }

    pWorld = sithWorld_pStaticWorld;
    for ( n = 0; n < (int)pWorld->numMaterials; ++n )
    {
        if ( pWorld->aMaterials[n].numCels > 1 )
        {
            SITHDSS_PUSHUINT8(pWorld->aMaterials[n].curCelNum);
        }
    }

    pWorld = sithWorld_pCurrentWorld;
    for ( n = 0; n < (int)pWorld->numMaterials; ++n )
    {
        if ( pWorld->aMaterials[n].numCels > 1 )
        {
            SITHDSS_PUSHUINT8(pWorld->aMaterials[n].curCelNum);
        }
    }

    sithMulti_message.type   = SITHDSS_SYNCGAMESTATE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessSyncGameState(const SithMessage* pMsg)
{
    SithWorld* pWorld;
    uint8_t* pCurDSSMsgData_;
    SithInventoryType* pInventoryType;
    int i;
    int16_t idx16;
    int cogIdx;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1638
    SITH_ASSERTREL(pMsg && sithWorld_pStaticWorld && sithWorld_pCurrentWorld);

    cogIdx = SITHDSS_POPINT32();
    sithCog_pMasterCog = sithCog_GetCogByIndex(cogIdx);

#line 1644
    SITH_ASSERTREL(sithPlayer_pLocalPlayer);
    sithPlayer_pLocalPlayer->inventory.curItemID   = SITHDSS_POPINT32();
    sithPlayer_pLocalPlayer->inventory.curWeaponID = SITHDSS_POPINT32();

    for ( i = 0; i < (int)STD_ARRAYLEN(sithInventory_aPowerKeybinds); ++i )
    {
        sithInventory_aPowerKeybinds[i].bActivated = SITHDSS_POPINT32();
    }

    sithInventory_bSendDeactivateMessage = SITHDSS_POPINT32();
    sithInventory_bInitInventory         = SITHDSS_POPINT32();
    sithInventory_dword_56B750           = SITHDSS_POPINT32();
    sithInventory_dword_56B754           = SITHDSS_POPINT32();

    for ( i = 2; i < 11; ++i )
    {
        pInventoryType = &sithInventory_aTypes[i];
        idx16 = SITHDSS_POPINT16();
        if ( idx16 != -1 )
        {
            pInventoryType->pHolsterModel = sithModel_GetModelByIndex(idx16);
        }
        else
        {
            pInventoryType->pHolsterModel = NULL;
        }
    }

    sithRender_bResetCameraAspect = SITHDSS_POPINT32();
    cogIdx = SITHDSS_POPINT32();
    sithPlayerControls_SetVehicleBoardedThing(sithThing_GetThingByIndex(cogIdx));
    idx16 = SITHDSS_POPINT16();
    sithPlayerActions_pCurCollidedSurface = sithSurface_GetSurfaceEx(sithWorld_pCurrentWorld, idx16);
    idx16 = SITHDSS_POPINT16();
    sithPlayerActions_pCurCollidedModel   = sithModel_GetModelByIndex(idx16);
    if ( sithPlayerActions_pCurCollidedModel )
    {
        idx16 = SITHDSS_POPINT16();
        sithPlayerActions_pCurCollidedModelFace = &sithPlayerActions_pCurCollidedModel->aGeos[0].aMeshes->aFaces[idx16];
    }

    sithPlayer_bPlayerInPor       = SITHDSS_POPINT32();
    sithPlayer_impState           = SITHDSS_POPFLOAT();
    sithPlayer_bInAetheriumSector = SITHDSS_POPUINT8();
    sithPlayer_bGuybrush          = SITHDSS_POPUINT8();
    sithPlayer_impFireType        = SITHDSS_POPUINT16();
    sithPlayer_curLevelNum        = SITHDSS_POPUINT16();
    sithPlayer_bBonusMapBought    = SITHDSS_POPUINT8();
    cogIdx = SITHDSS_POPINT32();
    sithPlayerActions_pPlasma     = sithThing_GetThingByIndex(cogIdx);
    sithPlayerActions_jewelFlyingPuppetTrackNum = SITHDSS_POPUINT16();
    sithPlayerActions_bJewelFlying     = SITHDSS_POPUINT8();
    sithPlayerActions_bPlayerInvisible = SITHDSS_POPUINT8();
    sithPlayerControls_bCutsceneMode   = SITHDSS_POPUINT8();
    sithPuppet_bPlayerLeapForward      = SITHDSS_POPUINT8();

    sithFX_numChalkMarks    = SITHDSS_POPINT32();
    sithFX_lastChalkMarkNum = SITHDSS_POPINT32();
    for ( i = 0; i < sithFX_numChalkMarks; ++i )
    {
        cogIdx = SITHDSS_POPINT32();
        if ( cogIdx >= 0 && cogIdx < (int)sithWorld_pCurrentWorld->numThings )
        {
            sithFX_aChalkMarks[i] = &sithWorld_pCurrentWorld->aThings[cogIdx];
        }
    }

    pWorld = sithWorld_pStaticWorld;
    for ( i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        if ( pWorld->aMaterials[i].numCels > 1 )
        {
            pWorld->aMaterials[i].curCelNum = SITHDSS_POPUINT8();
        }
    }

    pWorld = sithWorld_pCurrentWorld;
    for ( i = 0; i < (int)pWorld->numMaterials; ++i )
    {
        if ( pWorld->aMaterials[i].numCels > 1 )
        {
            pWorld->aMaterials[i].curCelNum = SITHDSS_POPUINT8();
        }
    }

    return 1;
}

int J3DAPI sithDSS_SyncVehicleControlState(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    const SithMineCarControlState* pMineCarState = sithVehicleControls_GetMineCarControlState();
    const SithRaftControlState* pRaftStatus       = sithVehicleControls_GetRaftControlState();

    SITHDSS_PUSHFLOAT(pMineCarState->secStoppingElapsedTime);
    SITHDSS_PUSHINT32(pMineCarState->accelerationState);
    SITHDSS_PUSHFLOAT(pMineCarState->secDuckTime);
    SITHDSS_PUSHINT32(pMineCarState->curDuckPuppetTrackNum);
    SITHDSS_PUSHINT32(pMineCarState->bNotDucked);
    SITHDSS_PUSHINT32(pMineCarState->indyLeanState);
    SITHDSS_PUSHINT32(pMineCarState->curPuppetTrack);
    SITHDSS_PUSHINT32(pMineCarState->hBreakSnd);
    SITHDSS_PUSHFLOAT(pMineCarState->secBreakingElapsedTime);
    SITHDSS_PUSHFLOAT(pMineCarState->secUnboardingElapsedTime);
    SITHDSS_PUSHINT32(pMineCarState->unbaordState);

    SITHDSS_PUSHINT32(pRaftStatus->moveStatus);
    SITHDSS_PUSHINT32(pRaftStatus->puppetSubmode);
    SITHDSS_PUSHVEC3(&pRaftStatus->exitSurfPos);
    SITHDSS_PUSHVEC3(&pRaftStatus->exitSurfPYR);
    SITHDSS_PUSHFLOAT(pRaftStatus->moveSize);
    SITHDSS_PUSHFLOAT(pRaftStatus->velocity);
    SITHDSS_PUSHINT32(pRaftStatus->bRowing);
    SITHDSS_PUSHFLOAT(pRaftStatus->secRowStartTime);
    SITHDSS_PUSHFLOAT(pRaftStatus->secUnboardTime);

    sithMulti_message.type = SITHDSS_VEHICLECONTROLSTATE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessVehicleControlsState(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    SithMineCarControlState mineCarStatus;
    SithRaftControlState raftStatus;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1789
    SITH_ASSERTREL(pMsg);

    mineCarStatus.secStoppingElapsedTime  = SITHDSS_POPFLOAT();
    mineCarStatus.accelerationState       = SITHDSS_POPINT32();
    mineCarStatus.secDuckTime             = SITHDSS_POPFLOAT();
    mineCarStatus.curDuckPuppetTrackNum   = SITHDSS_POPINT32();
    mineCarStatus.bNotDucked              = SITHDSS_POPINT32();
    mineCarStatus.indyLeanState           = SITHDSS_POPINT32();
    mineCarStatus.curPuppetTrack          = SITHDSS_POPINT32();
    mineCarStatus.hBreakSnd               = SITHDSS_POPINT32();
    mineCarStatus.secBreakingElapsedTime  = SITHDSS_POPFLOAT();
    mineCarStatus.secUnboardingElapsedTime = SITHDSS_POPFLOAT();
    mineCarStatus.unbaordState            = SITHDSS_POPINT32();

    raftStatus.moveStatus    = SITHDSS_POPINT32();
    raftStatus.puppetSubmode = SITHDSS_POPINT32();
    SITHDSS_POPVEC3(&raftStatus.exitSurfPos);
    SITHDSS_POPVEC3(&raftStatus.exitSurfPYR);
    raftStatus.moveSize        = SITHDSS_POPFLOAT();
    raftStatus.velocity        = SITHDSS_POPFLOAT();
    raftStatus.bRowing         = SITHDSS_POPINT32();
    raftStatus.secRowStartTime = SITHDSS_POPFLOAT();
    raftStatus.secUnboardTime  = SITHDSS_POPFLOAT();

    sithVehicleControls_SetMineCarControlState(&mineCarStatus);
    sithVehicleControls_SetRaftControlState(&raftStatus);
    return 1;
}

int J3DAPI sithDSS_SendEndSection(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    sithMulti_message.type = SITHDSS_ENDSECTION;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSS_ProcessEndSection(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 1834
    SITH_ASSERTREL(pMsg);
    return 1;
}
