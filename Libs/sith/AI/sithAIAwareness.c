#include "sithAIAwareness.h"

#include <rdroid/Math/rdVector.h>

#include <sith/AI/sithAI.h>
#include <sith/Engine/sithIntersect.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>

// An enumerator, not a macro: retail's assertion string keeps the name unexpanded
// ("type < SITHAIAWARENESS_NUMTYPES") and the check is a signed compare.
enum
{
    SITHAIAWARENESS_NUMTYPES = 4
};

static SithAIAwarenessEvent aEvents[SITHAIAWARENESS_MAXEVENTS];
SithAIAwarenessSector* sithAIAwareness_aSectors = NULL;
static size_t numEvents = 0;
static int bOpened = 0;
static size_t curProcessID = 0;

void J3DAPI sithAIAwareness_ProcessEvents(void);
int J3DAPI sithAIAwareness_Update(int msecTime, SithEventParams* pParams);
void J3DAPI sithAIAwareness_ProcessEvent(const SithAIAwarenessEvent* pEvent, const SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float levelAtTransmittingPos, const SithSurfaceAdjoin* pTargetAdjoin, SithThing* pThing);
static float J3DAPI sithAIAwareness_CheckAdjoinDistance(const SithSurfaceAdjoin* pAdjoin, const rdVector3* startPos, rdVector3* pHitPos, float moveDistance);
int J3DAPI sithAIAwareness_CreateEvent(SithSector* pSector, const rdVector3* pos, int type, float levelAtTransmittingPos, SithThing* pThing);

int J3DAPI sithAIAwareness_Open(void)
{
#line 79
    SITH_ASSERTREL(sithWorld_pCurrentWorld);
    if ( bOpened )
    {
        return 0;
    }

#line 83
    sithAIAwareness_aSectors = (SithAIAwarenessSector*)STDMALLOC(sizeof(SithAIAwarenessSector) * sithWorld_pCurrentWorld->numSectors);
    if ( !sithAIAwareness_aSectors )
    {
        return 1;
    }
    memset(sithAIAwareness_aSectors, 0, sizeof(SithAIAwarenessSector) * sithWorld_pCurrentWorld->numSectors);

    numEvents = 0;
    if ( !sithEvent_RegisterTask(SITHAIAWARENESS_TASKID, sithAIAwareness_Update, 500u, SITHEVENT_TASKINTERVAL) )
    {
        return 1;
    }

    bOpened = 1;
    return 0;
}

void J3DAPI sithAIAwareness_Close(void)
{
    if ( !bOpened )
    {
        return;
    }

#line 115
    SITH_ASSERTREL(sithAIAwareness_aSectors);
    stdMemory_Free(sithAIAwareness_aSectors);
    sithAIAwareness_aSectors = NULL;

    // Remove event task
    sithEvent_RegisterTask(SITHAIAWARENESS_TASKID, NULL, 0, SITHEVENT_TASKDISABLED);
    bOpened = 0;
}

int J3DAPI sithAIAwareness_CreateTransmittingEvent(SithSector* pSector, const rdVector3* pos, int type, float transmittingLevel, SithThing* pThing)
{
    if ( !sithAI_bOpen )
    {
        return 0;
    }

#line 148
    SITH_ASSERTREL(type < SITHAIAWARENESS_NUMTYPES);
    return sithAIAwareness_CreateEvent(pSector, pos, type, transmittingLevel, pThing);
}

void J3DAPI sithAIAwareness_ProcessEvents(void)
{
    size_t eventNum;
    SithAIAwarenessEvent* pEvent;

    if ( !numEvents )
    {
        return;
    }

    for ( eventNum = 0, pEvent = aEvents; eventNum < numEvents; ++eventNum, ++pEvent )
    {
        sithAIAwareness_ProcessEvent(pEvent, pEvent->pSector, &pEvent->pos, &pEvent->pos, pEvent->levelAtTransmittingPos, NULL, pEvent->pThing);
    }
}

int J3DAPI sithAIAwareness_Update(int msecTime, SithEventParams* pParams)
{
    SithThing* pPlayer;
    int aiNum;
    SithAIControlBlock* pLocal;

    pPlayer = sithPlayer_pLocalPlayerThing;
    ++curProcessID;
    if ( !numEvents )
    {
        return 1;
    }

    // Process events
    sithAIAwareness_ProcessEvents();

    // Send sound events
    for ( aiNum = 0, pLocal = sithAI_aControlBlocks; aiNum <= sithAI_lastUsedAIIndex; ++aiNum, ++pLocal )
    {
        SithSector* pSector;
        if ( !pLocal->pClass || !pLocal->pOwner || (pLocal->pOwner->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
        {
            continue;
        }

        pSector = pLocal->pOwner->pInSector;
        if ( !pSector )
        {
            continue;
        }

        if ( sithAIAwareness_aSectors[sithSector_GetSectorIndex(pSector)].processID == curProcessID )
        {
            sithAI_EmitEvent(pLocal, SITHAI_EVENT_SOUND, 0);
        }
    }

    numEvents = 0;
    return 1;
}

int J3DAPI sithAIAwareness_IsInTransmittingRange(const SithAIAwarenessSector* pAISector, int type, const SithThing* pThing)
{
    float distance;

    rdVector3 vec;
    rdVector_Sub3(&vec, &pThing->pos, &pAISector->aEndPos[type]);
    distance = stdMath_ClipNearZero(rdVector_Len3(&vec));
    if ( distance <= pAISector->aLevelAtTransmittingPos[type] )
    {
        return 1;
    }

    return 0;
}

static float J3DAPI sithAIAwareness_CheckAdjoinDistance(const SithSurfaceAdjoin* pAdjoin, const rdVector3* startPos, rdVector3* pHitPos, float moveDistance)
{
    SithSurface* pSurface;
    rdFace* pFace;
    size_t i;
    float hitDist;
    rdVector3 moveNorm;
    int bIntersects;
    float length;
    float dx;
    float dy;
    float dz;

    pSurface = pAdjoin->pAdjoinSurface;
#line 285
    SITH_ASSERTREL(pSurface);

    pFace = &pSurface->face;
#line 287
    SITH_ASSERTREL(pFace);

    rdVector_Neg3(&moveNorm, &pFace->normal);
    bIntersects = sithIntersect_CheckSphereFaceIntersectionEx(startPos, &moveNorm, moveDistance, 0.0f, pFace, sithWorld_pCurrentWorld->aVertices, &hitDist, NULL, 0);
    if ( bIntersects )
    {
        rdVector_ScaleAdd3(pHitPos, &moveNorm, hitDist, startPos);
    }
    else
    {
        hitDist = 3.4028235e38f;
        for ( i = 0; i < pFace->numVertices; ++i )
        {
            dx = startPos->x - sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].x;
            dy = startPos->y - sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].y;
            dz = startPos->z - sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]].z;
            length = stdMath_ClipNearZero(sqrtf(dx * dx + dy * dy + dz * dz));
            if ( length < hitDist )
            {
                hitDist = length;
                rdVector_Copy3(pHitPos, &sithWorld_pCurrentWorld->aVertices[pFace->aVertices[i]]);
            }
        }
    }

    return hitDist;
}

void J3DAPI sithAIAwareness_ProcessEvent(const SithAIAwarenessEvent* pEvent, const SithSector* pSector, const rdVector3* startPos, rdVector3* endPos, float levelAtTransmittingPos, const SithSurfaceAdjoin* pTargetAdjoin, SithThing* pThing)
{
    SithSurfaceAdjoin* pAdjoin;
    SithAIAwarenessSector* pAISector = &sithAIAwareness_aSectors[sithSector_GetSectorIndex(pSector)];

#line 343
    SITH_ASSERTREL(pSector && pEvent);
    SITH_ASSERTREL(levelAtTransmittingPos > ((float)0.0));

    if ( pAISector->processID != curProcessID )
    {
        memset(pAISector, 0, sizeof(SithAIAwarenessSector));
        pAISector->processID = curProcessID;
    }

    if ( pAISector->aLevelAtTransmittingPos[pEvent->type] < levelAtTransmittingPos )
    {
        pAISector->aLevelAtTransmittingPos[pEvent->type] = levelAtTransmittingPos;
        rdVector_Copy3(&pAISector->aStartPos[pEvent->type], startPos);
        rdVector_Copy3(&pAISector->aEndPos[pEvent->type], endPos);
        pAISector->aTransmittingThing[pEvent->type] = pThing;

        for ( pAdjoin = pSector->pFirstAdjoin; pAdjoin; pAdjoin = pAdjoin->pNextAdjoin )
        {
            SithSector* pAdjoinSector;
            rdVector3 hitPos;
            float distance;
            float dTransmittingPos;

            pAdjoinSector = pAdjoin->pAdjoinSector;
            if ( pAdjoin != pTargetAdjoin )
            {
                distance = sithAIAwareness_CheckAdjoinDistance(pAdjoin, endPos, &hitPos, levelAtTransmittingPos);
                dTransmittingPos = levelAtTransmittingPos - distance;
                if ( dTransmittingPos > 0.0f )
                {
                    sithAIAwareness_ProcessEvent(pEvent, pAdjoinSector, startPos, &hitPos, dTransmittingPos, pAdjoin->pMirrorAdjoin, pThing);
                }
            }
        }
    }
}

int J3DAPI sithAIAwareness_CreateEvent(SithSector* pSector, const rdVector3* pos, int type, float levelAtTransmittingPos, SithThing* pThing)
{
    if ( numEvents == SITHAIAWARENESS_MAXEVENTS )
    {
        return 0;
    }

    aEvents[numEvents].pSector = pSector;
    rdVector_Copy3(&aEvents[numEvents].pos, pos);
    aEvents[numEvents].type = type;
    aEvents[numEvents].levelAtTransmittingPos = levelAtTransmittingPos;
    aEvents[numEvents].pThing = pThing;
    numEvents++;
    return 1;
}
