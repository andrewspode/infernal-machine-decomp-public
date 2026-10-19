#ifndef SOUND_AUDIOLIB_H
#define SOUND_AUDIOLIB_H
#include <j3dcore/j3d.h>
#include <sound/types.h>
#include <stdio.h>

J3D_EXTERN_C_START

// Returns pointer to snd data
const uint8_t* J3DAPI AudioLib_ParseWaveFileHeader(const uint8_t* pData, int* pType, uint32_t* pSampleRate, uint32_t* pBitsPerSample, uint32_t* pNumChannels, uint32_t* pExtraInfo, uint32_t* pSoundDataSize, uint32_t* pExtraDataOffset, uint32_t* pSoundDataOffset);

int J3DAPI AudioLib_Compress(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, const uint8_t* pInBuffer, int size, unsigned int numChannels);
void J3DAPI AudioLib_ResetCompressor(tAudioCompressorState* pState);
void J3DAPI AudioLib_Uncompress(tAudioCompressorState* pCompressorState, uint8_t* pOutSndData, const uint8_t* pCompressedData, unsigned int size);
int J3DAPI AudioLib_GetMouthPosition(uint8_t* pData, uint32_t time, uint8_t* pMouthPosX, uint8_t* pMouthPosY);
int J3DAPI AudioLib_GenerateLipSynchBlock(uint8_t* pOutData, const uint8_t* pSndData, unsigned int a3, uint8_t a4, uint8_t a5, int sampleRate, int bitsPerSample, int numChannels, int a9, int sndDataSize);
// ADPCM compression
int J3DAPI AudioLib_CompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutBuffer, int16_t* a3, int a4, unsigned int numChannels, int a6, int bStateInitialized);
// ADPCM decompress
void J3DAPI AudioLib_UncompressBlock(tAudioCompressorState* pCompressorState, uint8_t* pOutData, const uint8_t* pInData, int size, unsigned int numChannels, int bStateInited);
int J3DAPI AudioLib_WVSMCompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize, FILE* pFile);
int J3DAPI AudioLib_WVSMUncompressBlock(uint8_t* pOutBuffer, const uint8_t* pInBuffer, int blockSize);

J3D_EXTERN_C_END
#endif // SOUND_AUDIOLIB_H
