#ifndef RDROID_RDPUPPET_H
#define RDROID_RDPUPPET_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

rdPuppet* J3DAPI rdPuppet_New(rdThing* pThing);
void J3DAPI rdPuppet_NewEntry(rdPuppet* pPuppet, rdThing* pThing);

void J3DAPI rdPuppet_Free(rdPuppet* pPuppet);
void J3DAPI rdPuppet_FreeEntry(rdPuppet* pPuppet);

int J3DAPI rdPuppet_AddTrack(rdPuppet* pPuppet, rdKeyframe* pKFTrack, int lowPriority, int highPriority);
int J3DAPI rdPuppet_RemoveTrack(rdPuppet* pPuppet, size_t track);

int J3DAPI rdPuppet_SetStatus(rdPuppet* pPuppet, size_t track, rdPuppetTrackStatus status);
void J3DAPI rdPuppet_SetCallback(rdPuppet* pPuppet, size_t track, rdPuppetTrackCallback pfCallback);
int J3DAPI rdPuppet_PlayTrack(rdPuppet* pPuppet, size_t track);
int J3DAPI rdPuppet_FadeInTrack(rdPuppet* pPuppet, size_t track, float speed);
int J3DAPI rdPuppet_FadeOutTrack(rdPuppet* pPuppet, size_t track, float speed);
void J3DAPI rdPuppet_SetTrackSpeed(rdPuppet* pPuppet, size_t track, float fps);
void J3DAPI rdPuppet_SetTrackNoise(rdPuppet* pPuppet, size_t track, float noise);
void J3DAPI rdPuppet_SetTrackPriority(rdPuppet* pPuppet, size_t track, int lowPri, int highPri);

void J3DAPI rdPuppet_AdvanceTrack(rdPuppet* pPuppet, size_t track, float frames);
int J3DAPI rdPuppet_UpdateTracks(rdPuppet* pPuppet, float secDeltaTime);

void J3DAPI rdPuppet_ResetTrack(rdPuppet* pPuppet, size_t track);
void J3DAPI rdPuppet_BuildJointMatrices(rdThing* prdThing, const rdMatrix34* pPlacement);

J3D_EXTERN_C_END
#endif // RDROID_RDPUPPET_H
