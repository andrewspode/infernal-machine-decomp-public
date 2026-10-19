#ifndef SITH_SITHTHING_H
#define SITH_SITHTHING_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

// Macro generates bitmask from provided thing type
#define SITHTHING_TYPE2MASK(type) (1 << (type)) 

int J3DAPI sithThing_Startup(void);
int J3DAPI sithThing_Shutdown(void);
void J3DAPI sithThing_RegisterUnknownFunc(SithThingUnknownFunc pFunc);

int J3DAPI sithThing_Draw(SithThing* pThing);

int J3DAPI sithThing_AddSwapEntry(SithThing* pThing, int meshNum, rdModel3* pSrcModel, int meshNumSrc);
int J3DAPI sithThing_RemoveSwapEntry(SithThing* pThing, int refnum);
void J3DAPI sithThing_ResetSwapList(SithThing* pThing);

void J3DAPI sithThing_Update(float secDeltaTime, uint32_t msecDeltaTime);

SithThing* J3DAPI sithThing_GetThingParent(const SithThing* pThing);
SithThing* J3DAPI sithThing_GetThingByIndex(int idx);
int J3DAPI sithThing_GetThingIndex(const SithThing* pThing);
SithThing* J3DAPI sithThing_GetGuidThing(int guid);

void J3DAPI sithThing_DestroyThing(SithThing* pThing);

float J3DAPI sithThing_DamageThing(SithThing* pThing, const SithThing* pPurpetrator, float damage, SithDamageType hitType);
float J3DAPI sithThing_ScaleCombatDamage(const SithThing* pThing, SithThing* pHitThing);

int J3DAPI sithThing_AllocWorldThings(SithWorld* pWorld, size_t numThings);
void J3DAPI sithThing_FreeWorldThings(SithWorld* pWorld);
void J3DAPI sithThing_RemoveWorldThings(SithWorld* pWorld);

void J3DAPI sithThing_LoadPostProcess(SithWorld* pWorld);

void J3DAPI sithThing_RemoveThing(SithWorld* pWorld, SithThing* pThing);

void J3DAPI sithThing_Initialize(const SithWorld* pWorld, SithThing* pThing, int bFindFloor);
void J3DAPI sithThing_Reset(SithThing* pThing);

void J3DAPI sithThing_SetSector(SithThing* pThing, SithSector* pSector, int bNotify);
void J3DAPI sithThing_ExitSector(SithThing* pThing);
void J3DAPI sithThing_EnterSector(SithThing* pThing, SithSector* pNewSector, int bNoWaterSplash, int bNoNotify);

SithThing* J3DAPI sithThing_Create(SithThingType type);
void J3DAPI sithThing_SetPositionAndOrient(SithThing* pThing, const rdVector3* pos, const rdMatrix34* pOrient);

int J3DAPI sithThing_SetThingModel(SithThing* pThing, rdModel3* pModel);
void J3DAPI sithThing_SetThingBasedOn(SithThing* pThing, const SithThing* pTemplate);

SithThing* J3DAPI sithThing_CreateThingAtPos(const SithThing* pTemplate, const rdVector3* pos, const rdMatrix34* orient, SithSector* pSector, SithThing* pParent);
SithThing* J3DAPI sithThing_CreateThing(const SithThing* pTemplate, SithThing* pThing);

void J3DAPI sithThing_AttachThingToSurface(SithThing* pThing, SithSurface* pSurface, int bNoImpactUpdate);
void J3DAPI sithThing_AttachThingToClimbSurface(SithThing* pThing, SithSurface* pSurface, int bClimbing);
void J3DAPI sithThing_AttachThingToThingFace(SithThing* pThing, SithThing* pAttachThing, rdFace* pFace, const rdVector3* aVertices, int bNoImpactUpdate);
void J3DAPI sithThing_AddThingToAttachedThings(SithThing* pThing, SithThing* pAttachThing);
void J3DAPI sithThing_AttachThingToThing(SithThing* pThing, SithThing* pAttachThing);
void J3DAPI sithThing_AttachThingToThingMesh(SithThing* pThing, SithThing* pAttachThing, int meshNum);

void J3DAPI sithThing_DetachThing(SithThing* pThing);
void J3DAPI sithThing_DetachAttachedThings(SithThing* pThing);
int J3DAPI sithThing_IsThingAttachedToThing(SithThing* pThing);

int J3DAPI sithThing_ReadStaticThingsListText(SithWorld* pWorld, int bSkip);

int J3DAPI sithThing_WriteStaticThingsListBinary(tFileHandle fh, const SithWorld* pWorld);
int J3DAPI sithThing_ReadStaticThingsListBinary(tFileHandle fh, SithWorld* pWorld);
int J3DAPI sithThing_WriteThingsListBinary(tFileHandle fh, const SithWorld* pWorld, size_t numThings, const SithThing* aThings);
int J3DAPI sithThing_ReadThingsListBinary(tFileHandle fh, SithWorld* pWorld, size_t numThings, SithThing* aThings, void (J3DAPI* pfInitThingFunc)(SithThing*));

int J3DAPI sithThing_ParseArg(StdConffileArg* pArg, SithWorld* pWorld, SithThing* pThing);

int J3DAPI sithThing_ValidateThingPointer(const SithWorld* pWorld, const SithThing* pThing);
uint32_t J3DAPI sithThing_CalcThingChecksum(const SithThing* pTemplate, uint32_t seed);

int J3DAPI sithThing_CanSync(const SithThing* pThing);
void J3DAPI sithThing_SyncThing(SithThing* pThing, SithThingSyncFlags flags);
void sithThing_SyncThings(void); // Processes un-synced things

int J3DAPI sithThing_GetThingMeshIndex(const SithThing* pThing, const char* meshName);
int J3DAPI sithThing_GetThingJointIndex(const SithThing* pThing, const char* pJointName);

int J3DAPI sithThing_CreateQuetzUserBlock(SithThing* pThing);
void J3DAPI sithThing_UpdateQuetzUserBlock(SithThing* pThing);

#ifdef J3D_DEBUG
extern char* sithThing_aStrThingTypes[SITH_THING_NUMTYPES];
extern char* sithThing_aStrThingArgs[77];
#endif

J3D_EXTERN_C_END
#endif // SITH_SITHTHING_H
