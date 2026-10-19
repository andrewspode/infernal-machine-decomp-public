#include "rdPuppet.h"

#include <rdroid/Engine/rdThing.h>
#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/Raster/rdCache.h>

#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>

#include <math.h>

rdPuppet* J3DAPI rdPuppet_New(rdThing* pParent)
{
    rdPuppet* pPuppet;

#line 68
    pPuppet = (rdPuppet*)STDMALLOC(sizeof(rdPuppet));
    memset(pPuppet, 0, sizeof(rdPuppet));
    if ( !pPuppet )
    {
#line 73
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for rdPuppet.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdPuppet_NewEntry(pPuppet, pParent);
    pParent->pPuppet = pPuppet;
    return pPuppet;
}

void J3DAPI rdPuppet_NewEntry(rdPuppet* pPuppet, rdThing* parent)
{
    int trackNum;
    rdPuppetTrack* pTrack;

#line 106
    RD_ASSERTREL(pPuppet!=NULL);

    pPuppet->bPaused = 0;
    pPuppet->pThing  = parent;
    pTrack = pPuppet->aTracks;
    for ( trackNum = 0; trackNum < RDPUPPET_MAX_TRACKS; ++trackNum )
    {
        pTrack->curFrame  = 0.0f;
        pTrack->prevFrame = 0.0f;
        rdPuppet_RemoveTrack(pPuppet, trackNum);
        pTrack++;
    }
}

void J3DAPI rdPuppet_Free(rdPuppet* pPuppet)
{
    if ( pPuppet )
    {
        rdPuppet_FreeEntry(pPuppet);
        stdMemory_Free(pPuppet);
        return;
    }

#line 146
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: attempt to free NULL puppet ptr.\n", 0, 0, 0, 0);
}

void J3DAPI rdPuppet_FreeEntry(rdPuppet* pPuppet)
{
}

/* Linker-discarded (no caller in either image); the debug build keeps it at 0x00512EC3, between Free and AddTrack. */
void J3DAPI rdPuppet_SetPause(rdPuppet* pPuppet, int bPaused)
{
#line 184
    RD_ASSERTREL(pPuppet!=NULL);
    pPuppet->bPaused = bPaused;
}

int J3DAPI rdPuppet_AddTrack(rdPuppet* pPuppet, rdKeyframe* pKFTrack, int lowPriority, int highPriority)
{
    size_t curTrack;
    rdPuppetTrack* pTrack;

#line 216
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pKFTrack!=NULL);

    // Find non-playing track
    for ( curTrack = 0; curTrack < STD_ARRAYLEN(pPuppet->aTracks); ++curTrack )
    {
        if ( !pPuppet->aTracks[curTrack].status )
        {
            break;
        }
    }

    if ( curTrack >= STD_ARRAYLEN(pPuppet->aTracks) )
    {
        // No track was found, try find fading track
        for ( curTrack = 0; curTrack < RDPUPPET_TRACK_FADEOUT; ++curTrack )
        {
            if ( (pPuppet->aTracks[curTrack].status & RDPUPPET_TRACK_FADEOUT) != 0
                && (pPuppet->aTracks[curTrack].status & (RDPUPPET_TRACK_FADEOUT_PAUSE_ON_LAST_FRAME | RDPUPPET_TRACK_PAUSE_ON_LAST_FRAME)) == 0 )
            {
                rdPuppet_RemoveTrack(pPuppet, curTrack);
                break;
            }
        }
    }
    if ( curTrack >= STD_ARRAYLEN(pPuppet->aTracks) )
    {
        return -1;
    }

    pTrack = &pPuppet->aTracks[curTrack];
    pTrack->fps          = pKFTrack->fps;
    pTrack->pKFTrack     = pKFTrack;
    pTrack->playSpeed    = 0.0f;
    pTrack->lowPriority  = lowPriority;
    pTrack->highPriority = highPriority;
    pTrack->status      |= RDPUPPET_TRACK_UNKNOWN_1;

    rdPuppet_ResetTrack(pPuppet, curTrack);
    return curTrack;
}

int J3DAPI rdPuppet_RemoveTrack(rdPuppet* pPuppet, size_t track)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 281
    RD_ASSERTREL((track>=0)&&(track<8));
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pTrack!=NULL);

    if ( pTrack->pfCallback )
    {
        pTrack->pfCallback(pPuppet->pThing->pThing, track, (rdKeyMarkerType)0); // notify finished
    }

    pTrack->status     = 0;
    pTrack->pKFTrack   = NULL;
    pTrack->pfCallback = NULL;
    return 1;
}

int J3DAPI rdPuppet_SetStatus(rdPuppet* pPuppet, size_t track, rdPuppetTrackStatus status)
{
#line 314
    RD_ASSERTREL(pPuppet);
    RD_ASSERTREL((track>=0)&&(track<8));
    pPuppet->aTracks[track].status |= status;
    return 1;
}

void J3DAPI rdPuppet_SetCallback(rdPuppet* pPuppet, size_t track, rdPuppetTrackCallback pfCallback)
{
#line 326
    RD_ASSERTREL(pPuppet);
    RD_ASSERTREL((track>=0)&&(track<8));
    pPuppet->aTracks[track].pfCallback = pfCallback;
}

int J3DAPI rdPuppet_PlayTrack(rdPuppet* pPuppet, size_t track)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 352
    RD_ASSERTREL((track>=0)&&(track<8));
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pTrack!=NULL);

    pTrack->status |= RDPUPPET_TRACK_PLAYING;
    pTrack->status &= ~RDPUPPET_TRACK_PAUSED;
    pTrack->playSpeed = 1.0f;
    return 1;
}

int J3DAPI rdPuppet_FadeInTrack(rdPuppet* pPuppet, size_t track, float speed)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 388
    RD_ASSERTREL((track>=0)&&(track<8));
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pTrack!=NULL);

    pTrack->status &= ~RDPUPPET_TRACK_FADEOUT;
    pTrack->status |= RDPUPPET_TRACK_FADEIN | RDPUPPET_TRACK_PLAYING;
    if ( speed > 0.0f )
    {
        pTrack->fadeSpeed = 1.0f / speed;
    }
    else
    {
        pTrack->fadeSpeed = 1.0f;
    }

    return 1;
}

int J3DAPI rdPuppet_FadeOutTrack(rdPuppet* pPuppet, size_t track, float speed)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 427
    RD_ASSERTREL((track>=0)&&(track<8));
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pTrack!=NULL);

    pTrack->status &= ~RDPUPPET_TRACK_FADEIN;
    pTrack->status |= RDPUPPET_TRACK_FADEOUT;
    if ( speed > 0.0f )
    {
        pTrack->fadeSpeed = 1.0f / speed;
    }
    else
    {
        pTrack->fadeSpeed = 1.0f;
    }

    return 1;
}

void J3DAPI rdPuppet_SetTrackSpeed(rdPuppet* pPuppet, size_t track, float fps)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 463
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pTrack!=NULL);
    pTrack->fps = fps;
}

/* Linker-discarded (no caller in either image); the debug build keeps it at 0x00513492, after SetTrackSpeed. Status
   bit 0x1000 marks a track with noise. */
void J3DAPI rdPuppet_SetTrackNoise(rdPuppet* pPuppet, size_t track, float noise)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 489
    RD_ASSERTREL((track>=0)&&(track<8));
    RD_ASSERTREL(pPuppet!=NULL);
    RD_ASSERTREL(pTrack!=NULL);
    pTrack->noise = noise;
    if ( noise != 0.0f )
    {
        pTrack->status |= 0x1000;
    }
    else
    {
        pTrack->status &= ~0x1000;
    }
}

/* Linker-discarded (no caller in either image); the debug build keeps it at 0x0051354A, after SetTrackNoise. */
void J3DAPI rdPuppet_SetTrackPriority(rdPuppet* pPuppet, size_t track, int lowPri, int highPri)
{
    rdPuppetTrack* pTrack;

#line 529
    RD_ASSERTREL(pPuppet != NULL);
    RD_ASSERTREL((track>=0)&&(track<8));

    pTrack = &pPuppet->aTracks[track];
#line 534
    RD_ASSERTREL(pTrack != NULL);
    pTrack->lowPriority  = lowPri;
    pTrack->highPriority = highPri;
}

void J3DAPI rdPuppet_AdvanceTrack(rdPuppet* pPuppet, size_t track, float frames)
{
    float progress = 0.0f;
    rdPuppetTrack* pTrackEntry = &pPuppet->aTracks[track];
    int bFinish = 0;
    size_t numMarkers;
    size_t i;

#line 568
    RD_ASSERTREL((track>=0)&&(track<8));
    RD_ASSERTREL(pPuppet);
    RD_ASSERTREL(frames >= ((float)0.0));

    if ( !pTrackEntry->pKFTrack )
    {
#line 575
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Track %d does not exist.\n", track, 0, 0, 0);
        return;
    }

    if ( frames == 0.0f )
    {
        return;
    }

    // Advance cur track
    pTrackEntry->curFrame = pTrackEntry->prevFrame + frames;
    if ( pTrackEntry->curFrame >= (float)pTrackEntry->pKFTrack->numFrames )
    {
        if ( (pTrackEntry->status & RDPUPPET_TRACK_NOLOOP) != 0 )
        {
            pTrackEntry->curFrame = (float)pTrackEntry->pKFTrack->numFrames;
            bFinish = 1;
        }
        else if ( (pTrackEntry->status & RDPUPPET_TRACK_PAUSE_ON_LAST_FRAME) != 0 )
        {
            pTrackEntry->curFrame = (float)pTrackEntry->pKFTrack->numFrames;
            pTrackEntry->status |= RDPUPPET_TRACK_PAUSED;
        }
        else if ( (pTrackEntry->status & RDPUPPET_TRACK_FADEOUT_NOLOOP) != 0 )
        {
            pTrackEntry->curFrame = (float)pTrackEntry->pKFTrack->numFrames;
            rdPuppet_FadeOutTrack(pPuppet, track, 0.25f);
            pTrackEntry->status |= RDPUPPET_TRACK_PAUSED;
        }
        else // loop animation
        {
            progress = floorf(D3DVAL(pTrackEntry->curFrame) / D3DVAL(pTrackEntry->pKFTrack->numFrames));
            pTrackEntry->curFrame -= (float)pTrackEntry->pKFTrack->numFrames * progress;
            memset(pTrackEntry->aCurKfNodeEntryNums, 0, sizeof(pTrackEntry->aCurKfNodeEntryNums[0]) * pPuppet->pThing->data.pModel3->numHNodes);
        }
    }

    if ( pTrackEntry->pfCallback && pTrackEntry->pKFTrack->numMarkers )
    {
#line 621
        RD_ASSERTREL(pPuppet->pThing);

        if ( progress == 0.0f )
        {
            if ( pTrackEntry->pKFTrack )
            {
                numMarkers = pTrackEntry->pKFTrack->numMarkers;
                i = 0;
                while ( pTrackEntry->pKFTrack && i < numMarkers )
                {
                    if ( pTrackEntry->pKFTrack->aMarkerFrames[i] > pTrackEntry->curFrame )
                    {
                        break;
                    }

                    if ( pTrackEntry->pKFTrack->aMarkerFrames[i] > pTrackEntry->prevFrame || pTrackEntry->prevFrame == 0.0f )
                    {
                        pTrackEntry->pfCallback(pPuppet->pThing->pThing, track, pTrackEntry->pKFTrack->aMarkerTypes[i]);
                    }

                    ++i;
                }
            }
        }
        else if ( progress > 1.0f )
        {
            if ( pTrackEntry->pKFTrack )
            {
                numMarkers = pTrackEntry->pKFTrack->numMarkers;
                i = 0;
                while ( pTrackEntry->pKFTrack && i < numMarkers )
                {
                    pTrackEntry->pfCallback(pPuppet->pThing->pThing, track, pTrackEntry->pKFTrack->aMarkerTypes[i]);
                    ++i;
                }
            }
        }
        else if ( pTrackEntry->pKFTrack )
        {
            numMarkers = pTrackEntry->pKFTrack->numMarkers;
            i = 0;
            while ( pTrackEntry->pKFTrack && i < numMarkers )
            {
                if ( pTrackEntry->pKFTrack->aMarkerFrames[i] > pTrackEntry->prevFrame || pTrackEntry->pKFTrack->aMarkerFrames[i] <= pTrackEntry->curFrame )
                {
                    pTrackEntry->pfCallback(pPuppet->pThing->pThing, track, pTrackEntry->pKFTrack->aMarkerTypes[i]);
                }

                ++i;
            }
        }
    }

    if ( bFinish )
    {
        rdPuppet_RemoveTrack(pPuppet, track);
    }
    else
    {
        pTrackEntry->prevFrame = pTrackEntry->curFrame;
    }
}

int J3DAPI rdPuppet_UpdateTracks(rdPuppet* pPuppet, float secDeltaTime)
{
    size_t numUpdatedTracks = 0;
    size_t trackNum;
    rdPuppetTrack* pTrack;

#line 716
    RD_ASSERTREL(pPuppet!=NULL);
    if ( !pPuppet->bPaused )
    {
        for ( trackNum = 0, pTrack = pPuppet->aTracks; trackNum < RDPUPPET_MAX_TRACKS; ++trackNum, ++pTrack )
        {
            if ( !pTrack->status )
            {
                continue;
            }

            ++numUpdatedTracks;

            if ( (pTrack->status & RDPUPPET_TRACK_PAUSED) == 0 )
            {
                rdPuppet_AdvanceTrack(pPuppet, trackNum, pTrack->fps * secDeltaTime);
            }

            if ( (pTrack->status & RDPUPPET_TRACK_FADEIN) != 0 )
            {
                pTrack->playSpeed += pTrack->fadeSpeed * secDeltaTime;
                if ( pTrack->playSpeed >= 1.0f )
                {
                    pTrack->playSpeed = 1.0f;
                    pTrack->status &= ~RDPUPPET_TRACK_FADEIN;
                }
            }
            else if ( (pTrack->status & RDPUPPET_TRACK_FADEOUT) != 0 )
            {
                pTrack->playSpeed -= pTrack->fadeSpeed * secDeltaTime;
                if ( pTrack->playSpeed <= 0.0f )
                {
                    if ( (pTrack->status & RDPUPPET_TRACK_FADEOUT_PAUSE_ON_LAST_FRAME) != 0 )
                    {
                        pTrack->status &= ~RDPUPPET_TRACK_FADEOUT;
                        pTrack->status |= RDPUPPET_TRACK_PAUSED;
                    }
                    else
                    {
                        rdPuppet_RemoveTrack(pPuppet, trackNum);
                    }
                }
            }
        }
    }

    return numUpdatedTracks;
}

void J3DAPI rdPuppet_ResetTrack(rdPuppet* pPuppet, size_t track)
{
    rdPuppetTrack* pTrack = &pPuppet->aTracks[track];

#line 788
    RD_ASSERTREL((track>=0)&&(track<8));

    memset(pTrack->aCurKfNodeEntryNums, 0, sizeof(pTrack->aCurKfNodeEntryNums[0]) * pPuppet->pThing->data.pModel3->numHNodes);
    pTrack->curFrame  = 0.0f;
    pTrack->prevFrame = 0.0f;
    pTrack->status    = RDPUPPET_TRACK_PLAYING | RDPUPPET_TRACK_UNKNOWN_1;
}


void J3DAPI rdPuppet_BuildJointMatrices(rdThing* prdThing, const rdMatrix34* pPlacement)
{
    rdPuppet* pPuppet = prdThing->pPuppet;
    rdModel3* pModel3 = prdThing->data.pModel3;
    size_t trackNum;
    size_t jointNum;
    size_t nodeNum;
    rdPuppetTrack* pTrack;
    rdKeyframe* pKFTrack;
    rdModel3HNode* pNode;
    size_t nodeIdx;
    rdKeyframeNodeEntry* pKfFrame;

    if ( prdThing->bSkipBuildingJoints )
    {
        return;
    }

    if ( pPuppet && !pPuppet->bPaused )
    {
        // Advance track KFNode indices to >= curframe
        for ( trackNum = 0, pTrack = pPuppet->aTracks; trackNum < RDPUPPET_MAX_TRACKS; trackNum++, pTrack++ )
        {
            rdKeyframeNode* pKfNode;

            if ( !pTrack->status )
            {
                continue;
            }

            pKFTrack = pTrack->pKFTrack;
            pKfNode = pKFTrack->aNodes;
            for ( jointNum = 0; jointNum < pKFTrack->numJoints; jointNum++, pKfNode++ )
            {
                int bFinish = 0;

                if ( !pKfNode->numEntries )
                {
                    continue;
                }

                nodeIdx = pTrack->aCurKfNodeEntryNums[pKfNode->nodeNum];
                if ( nodeIdx != pKfNode->numEntries - 1 && pTrack->curFrame >= pKfNode->aEntries[nodeIdx + 1].frame )
                {
                    ++nodeIdx;
                    while ( !bFinish )
                    {
                        if ( nodeIdx == pKfNode->numEntries - 1 )
                        {
                            bFinish = 1;
                        }
                        else if ( pTrack->curFrame < pKfNode->aEntries[nodeIdx + 1].frame )
                        {
                            bFinish = 1;
                        }
                        else
                        {
                            ++nodeIdx;
                        }
                    }

                    pTrack->aCurKfNodeEntryNums[pKfNode->nodeNum] = nodeIdx;
                }
            }
        }

        for ( nodeNum = 0, pNode = pModel3->aHierarchyNodes; nodeNum < pModel3->numHNodes; nodeNum++, pNode++ )
        {
            rdVector3 dkfpos;
            rdVector3 dkfrot;
            float baseDeltaFrames;
            int highPri = 0;
            int lowPri  = 0;
            float playSpeed       = 0.0f;
            float lowPriPlaySpeed = 0.0f;
            rdVector3 newPos;
            rdVector3 newPyr;
            rdVector3 lowPriPos;
            rdVector3 lowPriPyr;

            memset(&newPos, 0, sizeof(newPos));
            memset(&newPyr, 0, sizeof(newPyr));
            memset(&lowPriPos, 0, sizeof(lowPriPos));
            memset(&lowPriPyr, 0, sizeof(lowPriPyr));

            for ( trackNum = 0, pTrack = pPuppet->aTracks; trackNum < RDPUPPET_MAX_TRACKS; trackNum++, pTrack++ )
            {
                int priority;

                pKFTrack = pTrack->pKFTrack;
                if ( !pKFTrack )
                {
                    continue;
                }

                if ( (pKFTrack->type & pNode->type) != 0 )
                {
                    priority = pTrack->highPriority;
                }
                else
                {
                    priority = pTrack->lowPriority;
                }

                if ( !((pTrack->status & RDPUPPET_TRACK_PLAYING) != 0
                    && pKFTrack->aNodes[pNode->num].numEntries
                    && priority >= lowPri
                    && (priority >= highPri || playSpeed < 1.0f)) )
                {
                    continue;
                }

                if ( pKFTrack->numJoints <= pNode->num )
                {
#line 930
                    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "KEY %s playing on model %s - mismatched node count!\n", pKFTrack->aName, pModel3->aName, 0, 0);
                    continue;
                }

                pKfFrame = &pKFTrack->aNodes[pNode->num].aEntries[pTrack->aCurKfNodeEntryNums[pNode->num]];
                baseDeltaFrames = pTrack->curFrame - pKfFrame->frame;
#line 939
                RD_ASSERTREL(baseDeltaFrames >= ((float)0.0));

                if ( (pKfFrame->flags & 1) != 0 ) // dpos
                {
                    rdVector_ScaleAdd3(&dkfpos, &pKfFrame->dpos, baseDeltaFrames, &pKfFrame->pos);
                }
                else
                {
                    rdVector_Copy3(&dkfpos, &pKfFrame->pos);
                }

                if ( (pKfFrame->flags & 2) != 0 ) // drot
                {
                    rdVector_ScaleAdd3(&dkfrot, &pKfFrame->drot, baseDeltaFrames, &pKfFrame->rot);
                }
                else
                {
                    rdVector_Copy3(&dkfrot, &pKfFrame->rot);
                }

                rdVector_Sub3Acc(&dkfpos, &pNode->pos);
                rdVector_Sub3Acc(&dkfrot, &pNode->pyr);

                dkfrot.pitch = stdMath_NormalizeAngleAcute(dkfrot.pitch);
                dkfrot.yaw   = stdMath_NormalizeAngleAcute(dkfrot.yaw);
                dkfrot.roll  = stdMath_NormalizeAngleAcute(dkfrot.roll);

                if ( pTrack->playSpeed < 1.0f )
                {
                    rdVector_Scale3Acc(&dkfpos, pTrack->playSpeed);
                    rdVector_Scale3Acc(&dkfrot, pTrack->playSpeed);
                }

                if ( priority == highPri )
                {
                    rdVector_Add3Acc(&newPos, &dkfpos);
                    rdVector_Add3Acc(&newPyr, &dkfrot);
                    playSpeed += pTrack->playSpeed;
                }
                else if ( priority > highPri )
                {
                    rdVector_Copy3(&lowPriPos, &newPos);
                    rdVector_Copy3(&lowPriPyr, &newPyr);
                    lowPriPlaySpeed = playSpeed;
                    lowPri    = highPri;
                    highPri   = priority;
                    rdVector_Copy3(&newPos, &dkfpos);
                    rdVector_Copy3(&newPyr, &dkfrot);
                    playSpeed = pTrack->playSpeed;
                }
                else if ( priority > lowPri )
                {
                    rdVector_Copy3(&lowPriPos, &dkfpos);
                    rdVector_Copy3(&lowPriPyr, &dkfrot);
                    lowPriPlaySpeed = pTrack->playSpeed;
                    lowPri          = priority;
                }
                else
                {
#line 1000
                    RD_ASSERTREL(priority == lowPri);
                    RD_ASSERTREL(highPri != lowPri);
                    rdVector_Add3Acc(&lowPriPos, &dkfpos);
                    rdVector_Add3Acc(&lowPriPyr, &dkfrot);
                    lowPriPlaySpeed += pTrack->playSpeed;
                }
            }

            if ( playSpeed < 1.0f && lowPriPlaySpeed > 0.0f )
            {
                float factor;
                if ( lowPriPlaySpeed > 1.0f )
                {
                    factor = 1.0f / lowPriPlaySpeed;
                    rdVector_Scale3Acc(&lowPriPos, factor);
                    rdVector_Scale3Acc(&lowPriPyr, factor);
                }

                factor = 1.0f - playSpeed;
                rdVector_ScaleAdd3Acc(&newPos, &lowPriPos, factor);
                rdVector_ScaleAdd3Acc(&newPyr, &lowPriPyr, factor);
            }
            else if ( playSpeed > 1.0f )
            {
                float invSpeed = 1.0f / playSpeed;
                rdVector_Scale3Acc(&newPos, invSpeed);
                rdVector_Scale3Acc(&newPyr, invSpeed);

            }

            newPyr.pitch = stdMath_NormalizeAngleAcute(newPyr.pitch);
            newPyr.yaw   = stdMath_NormalizeAngleAcute(newPyr.yaw);
            newPyr.roll  = stdMath_NormalizeAngleAcute(newPyr.roll);
            rdVector_Add3Acc(&newPos, &pNode->pos);
            rdVector_Add3Acc(&newPyr, &pNode->pyr);

            rdMatrix_Build34(&prdThing->paJointMatrices[nodeNum], &newPyr, &newPos);

            if ( prdThing->apTweakedAngles[nodeNum].x != 0.0f || prdThing->apTweakedAngles[nodeNum].y != 0.0f || prdThing->apTweakedAngles[nodeNum].z != 0.0f )
            {
                rdMatrix_PreRotate34(&prdThing->paJointMatrices[nodeNum], &prdThing->apTweakedAngles[nodeNum]);
            }


        }
    }
    else
    {
        pNode = pModel3->aHierarchyNodes;
        for ( nodeNum = 0; nodeNum < pModel3->numHNodes; ++nodeNum )
        {
            rdMatrix_Copy34(&prdThing->paJointMatrices[nodeNum], &pNode->meshOrient);
            pNode++;
        }
    }

    rdThing_AccumulateMatrices(prdThing, pModel3->aHierarchyNodes, pPlacement);
    prdThing->rdFrameNum = rdCache_GetFrameNum();
}
