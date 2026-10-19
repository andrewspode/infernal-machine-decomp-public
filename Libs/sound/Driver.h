#ifndef SOUND_DRIVER_H
#define SOUND_DRIVER_H
#include <j3dcore/j3d.h>
#include <sound/types.h>

#include <dsound.h>

J3D_EXTERN_C_START

int J3DAPI SoundDriver_Open(int bNoSound3D, int bGlobalFocus, HWND hwnd, LPDIRECTSOUND pDirectSound, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, SoundDriverGetSoundBufferDataFunc pfGetSoundBufferData, tPrintfFunc pfLogError);
void J3DAPI SoundDriver_Close(void);

void J3DAPI SoundDriver_SetMaxVolume(float volume);
LPDIRECTSOUNDBUFFER J3DAPI SoundDriver_CreateAndPlay(int samplesPerSec, int nBitsPerSample, int numChannels, const uint8_t* pSoundData, size_t dataSize, tSoundChannelFlag* pFlags, LPDIRECTSOUNDBUFFER pSBuffer);
void J3DAPI SoundDriver_Release(LPDIRECTSOUNDBUFFER pDSBuf);
void J3DAPI SoundDriver_Play(LPDIRECTSOUNDBUFFER pDSBuf, int bLoop);
void J3DAPI SoundDriver_Stop(LPDIRECTSOUNDBUFFER pDSBuf);

void J3DAPI SoundDriver_SetVolume(LPDIRECTSOUNDBUFFER pDSBuf, float volume);
float J3DAPI SoundDriver_GetVolume(LPDIRECTSOUNDBUFFER pDSBuf);

void J3DAPI SoundDriver_SetPan(LPDIRECTSOUNDBUFFER pDSoundBuf, float pan);
float J3DAPI SoundDriver_GetPan(LPDIRECTSOUNDBUFFER pDSBuf);

void J3DAPI SoundDriver_SetFrequency(LPDIRECTSOUNDBUFFER pDSBuf, float freq);
float J3DAPI SoundDriver_GetFrequency(LPDIRECTSOUNDBUFFER pDSBuf);

size_t J3DAPI SoundDriver_GetCurrentPosition(LPDIRECTSOUNDBUFFER pDSoundBuf);
tSoundChannelFlag J3DAPI SoundDriver_GetStatusAndCaps(LPDIRECTSOUNDBUFFER pDSBuffer);

void J3DAPI SoundDriver_SetListenerPosition(const rdVector3* pPos, const rdVector3* pVelocity, const rdVector3* pTopOrient, const rdVector3* pFrontOrient, const rdVector3* pRightOrient);
void J3DAPI SoundDriver_ListenerCommitDeferred(void);

void J3DAPI SoundDriver_SetPosAndVelocity(LPDIRECTSOUNDBUFFER pBuffer, float x, float y, float z, float velX, float velY, float velZ, float minDistance, float maxDistance, SoundEnvFlags envflags);

int J3DAPI SoundDriver_Update3DSound(LPDIRECTSOUNDBUFFER* ppDSBuf, float x, float y, float z, float volume, float pitch, tSoundChannelFlag* pChannelFlags, float minRadius, float maxRadius, SoundEnvFlags envflags);

void J3DAPI SoundDriver_SetGlobals(float distanceFactor, float minDistance, float maxDistance, float rolloffFactor, float dopplerFactor);

int J3DAPI SoundDriver_Has3DHW(void);
int J3DAPI SoundDriver_Use3DCaps(void);
LPDIRECTSOUND J3DAPI SoundDriver_GetDSound(void);

LPDIRECTSOUND J3DAPI SoundDriver_CreateDirectSound(int bNoSound3D);
void J3DAPI SoundDriver_ReleaseDirectSound(LPDIRECTSOUND pDSound);

void J3DAPI SoundDriver_CreateListener(void);
void J3DAPI SoundDriver_ReleaseListener(void);

void J3DAPI SoundDriver_SetOuputFormat(LPDIRECTSOUND pDSound, uint32_t nSamplesPerSec, uint32_t nBitsPerSample, uint32_t numChannels);

LPDIRECTSOUNDBUFFER J3DAPI SoundDriver_GetPrimaryDSBuffer(LPDIRECTSOUND pDSound);
void J3DAPI SoundDriver_ReleasePrimaryDSBuffer(void);

int J3DAPI SoundDriver_GetDecibelVolume(float volume);
int J3DAPI SoundDriver_GetIntPan(float pan);

float J3DAPI SoundDriver_GetVolumeFromDecibels(int dB);
float J3DAPI SoundDriver_GetPanFromTable(int pan);

uint32_t J3DAPI SoundDriver_GetTimeMsec(void);
void J3DAPI SoundDriver_Sleep(unsigned int dwMilliseconds);

J3D_EXTERN_C_END
#endif // SOUND_DRIVER_H
