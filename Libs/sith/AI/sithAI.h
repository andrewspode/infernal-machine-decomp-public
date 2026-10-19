#ifndef SITH_SITHAI_H
#define SITH_SITHAI_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

extern int sithAI_bOpen;

#define SITHAI_MAXAIS 256

extern SithAIControlBlock sithAI_aControlBlocks[SITHAI_MAXAIS];

extern int sithAI_lastUsedAIIndex;

int J3DAPI sithAI_Startup(void);
void J3DAPI sithAI_Shutdown(void);
void J3DAPI sithAI_Open(void);
void J3DAPI sithAI_Close(void);

void J3DAPI sithAI_Create(SithThing* pThing);
void J3DAPI sithAI_Free(SithThing* pThing);

void J3DAPI sithAI_Process(float secDeltaTime);
int J3DAPI sithAI_ForceInstinctUpdate(SithAIControlBlock* pLocal, const char* pInstinctName, int bSendModeChangeEvent);

void J3DAPI sithAI_EmitEvent(SithAIControlBlock* pLocal, SithAIEventType event, void* pObject);

void J3DAPI sithAI_Stop(SithThing* pThing);
void J3DAPI sithAI_StopAllAttackingAIs(void);

int J3DAPI sithAI_RegisterInstinct(const char* pName, SithAIInstinctFunc pfInstinct, SithAIMode updateModes, SithAIMode updateBlockModes, SithAIEventType triggerEvents);
SithAIRegisteredInstinct* J3DAPI sithAI_FindInstinct(const char* pInstinctName);
int J3DAPI sithAI_GetInstinctIndex(const SithAIControlBlock* pLocal, const SithAIRegisteredInstinct* pRegInstinct);
int J3DAPI sithAI_HasInstinct(const SithAIControlBlock* pLocal, const char* pName);
int J3DAPI sithAI_EnableInstinct(SithAIControlBlock* pLocal, const char* pInstinctName, int bEnable);

int J3DAPI sithAI_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);
void J3DAPI sithAI_CreateAIFramesFomMarker(SithThing* pNewThing, const SithThing* pMarker, const rdVector3* pivot);
int J3DAPI sithAI_AllocAIFrames(SithAIControlBlock* pLocal, size_t sizeFrames);
int J3DAPI sithAI_AIList(const SithConsoleCommand* pFunc, const char* pArg);
int J3DAPI sithAI_AIStatus(const SithConsoleCommand* pFunc, const char* pArg);


J3D_EXTERN_C_END
#endif // SITH_SITHAI_H
