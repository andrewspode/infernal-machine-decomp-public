#ifndef SOUND_SOUND_H
#define SOUND_SOUND_H
#include <j3dcore/j3d.h>
#include <sound/types.h>

J3D_EXTERN_C_START

#define SOUND_INVALIDHANDLE       0u
#define SOUND_INVALIDGUID         0u

#define SOUND_NOSOUNDINDEX          0x12345678u // Index or ID marking sound not belonging to any specific entity
#define SOUND_INVALISOUNDINFOINDEX  0u

#define SOUND_ALLTHINGSOUNDHANDLE   0u // Handle for all thing sounds, used in functions to modify thing sounds, e.g. Sound_SetVolumeThing

int J3DAPI Sound_Initialize(tHostServices* pHS);
void J3DAPI Sound_Uninitialize(void);

int J3DAPI Sound_Startup(void);
void J3DAPI Sound_Shutdown(void);

int J3DAPI Sound_Open(SoundOpenFlags flags, size_t maxSoundChannels, SoundGetThingInfoCallback pfGetThingInfo, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, HWND hwnd);
void J3DAPI Sound_Close(void);

int J3DAPI Sound_Save(tFileHandle fh);
int J3DAPI Sound_Restore(tFileHandle fh);

void J3DAPI Sound_Pause(void);
void J3DAPI Sound_Resume(void);

void J3DAPI Sound_Set3DGlobals(float distanceFactor, float minDistance, float maxDistance, float folloverFactor, float dopplerFactor);

int J3DAPI Sound_Has3DHW(void);
void J3DAPI Sound_Set3DHWState(int bNo3DSound);
int J3DAPI Sound_Get3DHWState(void);

size_t J3DAPI Sound_GetMemoryUsage(void);
tSoundHandle J3DAPI Sound_Load(const char* filePath, uint32_t* sndIdx); // sndIdx is probably ID that is assign to loaded sound. When sound was previously already loaded then it's index is returned via sndIdx.
tSoundHandle J3DAPI Sound_LoadStatic(const char* pFilename, uint32_t* sndIdx); // sndIdx is probably ID that is assign to loaded sound. When sound was previously already loaded then it's index is returned via sndIdx.

void J3DAPI Sound_Reset(int bClearAll);
void J3DAPI Sound_EnableLoad(int bEnable);

size_t J3DAPI Sound_ExportStaticBank(tFileHandle fh);
int J3DAPI Sound_ImportStaticBank(tFileHandle fh);

size_t J3DAPI Sound_ExportNormalBank(tFileHandle fh);
int J3DAPI Sound_ImportNormalBank(tFileHandle fh);

int J3DAPI Sound_SkipSoundFileSection(tFileHandle fh);

tSoundHandle J3DAPI Sound_GetSoundHandle(uint32_t idx);
size_t J3DAPI Sound_GetSoundIndex(tSoundHandle hSnd);

tSoundChannelHandle J3DAPI Sound_GetChannelHandle(int guid);
int J3DAPI Sound_GetChannelGUID(tSoundChannelHandle hChannel);

const char* J3DAPI Sound_GetSoundFilename(tSoundHandleType handle); // handle is either tSoundChannelHandle or tSoundHandle
size_t J3DAPI Sound_GetLengthMsec(tSoundHandleType handle); // handle is either tSoundChannelHandle or tSoundHandle

void J3DAPI Sound_SetMaxVolume(float maxVolume);
float J3DAPI Sound_GetMaxVolume(void);

tSoundChannelHandle J3DAPI Sound_Play(tSoundHandle hSnd, float volume, float pan, int priority, tSoundChannelFlag flags, int guid);
tSoundChannelHandle J3DAPI Sound_PlayPos(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag channelflags, float x, float y, float z, int guid, float minRadius, float maxRadius, SoundEnvFlags envflags);
tSoundChannelHandle J3DAPI Sound_PlayThing(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag flags, int thingID, int guid, float minRadius, float maxRadius);

void J3DAPI Sound_StopChannel(tSoundChannelHandle hChannel);
void J3DAPI Sound_StopAllSounds(void);

void J3DAPI Sound_StopThing(int thingId, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to stop all thing sounds
void J3DAPI Sound_SetVolume(tSoundChannelHandle hChannel, float volume);
void J3DAPI Sound_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime);

void J3DAPI Sound_SetVolumeThing(int thingId, tSoundHandleType handle, float volume); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to set volume to all thing sounds
void J3DAPI Sound_FadeVolumeThing(int thingId, tSoundHandleType handle, float startVolume, float endVolume); // handle is either tSoundChannelHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to fade all thing sounds
int J3DAPI Sound_IsThingFadingVol(int thingId, tSoundHandleType handle); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to check if any of thing sounds is fading volume

void J3DAPI Sound_SetPitch(tSoundChannelHandle hChannel, float pitch);
float J3DAPI Sound_GetPitch(tSoundChannelHandle hChannel);
void J3DAPI Sound_FadePitch(tSoundChannelHandle hChannel, float pitch, float secFadeTime);

void J3DAPI Sound_SetPitchThing(int thingId, tSoundHandleType handle, float pitch); // handle is either tSoundChannelHandle, tSoundHandle, or SOUND_ALLTHINGSOUNDHANDLE in which case will cause to set pitch to all thing sounds
int J3DAPI Sound_IsThingFadingPitch(int thingId, tSoundHandleType handle); // debug build: called by sithSoundMixer_IsThingFadingPitch (both discarded in retail)

tSoundChannelFlag J3DAPI Sound_GetChannelFlags(tSoundChannelHandle hChannel);

void J3DAPI Sound_Update(const rdVector3* pPos, const rdVector3* pVelocity, rdVector3* pTopOrient, rdVector3* pFrontOrient, const rdVector3* pRightOrient); // Retail takes a fifth vector argument (both callers pass NULL)
int J3DAPI Sound_GenerateLipSync(tSoundChannelHandle hChannel, uint8_t* pMouthPosX, uint8_t* pMouthPosY, int a4);

void J3DAPI Sound_SetReverseSound(int bReverse);
void J3DAPI Sound_SoundDump(void);

SoundInfo* J3DAPI Sound_GetSoundInfo(tSoundHandle hSnd);
tSoundChannel* J3DAPI Sound_GetChannel(tSoundChannelHandle hChannel);
tSoundChannel* J3DAPI Sound_GetChannelBySoundHandle(tSoundHandle hSnd);

J3D_EXTERN_C_END
#endif // SOUND_SOUND_H
