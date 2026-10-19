#ifndef SITH_SITHVOICE_H
#define SITH_SITHVOICE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

int J3DAPI sithVoice_Startup(void);
void J3DAPI sithVoice_Shutdown(void);

int J3DAPI sithVoice_Open(void);
void J3DAPI sithVoice_Close(void);

int J3DAPI sithVoice_RegisterCogFunctions(SithCogSymbolTable* pTbl);

int J3DAPI sithVoice_PlayThingVoice(SithThing* pThing, tSoundHandle hSnd, float volume);
void J3DAPI sithVoice_UpdateLipSync(SithThing* pThing);

int J3DAPI sithVoice_GetShowText(void);
void J3DAPI sithVoice_ShowText(int bShow);

void J3DAPI sithVoice_Draw(void);

int J3DAPI sithVoice_SyncVoiceState(DPID idTo, unsigned int outstream);
int J3DAPI sithVoice_ProcessVoiceState(const SithMessage* pMsg);

const rdFont* J3DAPI sithVoice_GetTextFont(void);

J3D_EXTERN_C_END
#endif // SITH_SITHVOICE_H
