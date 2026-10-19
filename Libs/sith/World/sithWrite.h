#ifndef SITH_SITHWRITE_H
#define SITH_SITHWRITE_H
#include <j3dcore/j3d.h>
#include <sith/types.h>

J3D_EXTERN_C_START

int J3DAPI sithWrite_Write(const SithWorld* pWorld, const char* pFilename);
int J3DAPI sithWrite_WriteStaticThingsListText(const SithWorld* pWorld);
int J3DAPI sithWrite_WriteThingArgs(const SithThing* pThing, const SithThing* pBasedOn, int bWriteOrient);
int J3DAPI sithWrite_WriteThingPhysicsArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WriteThingPathMovementArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WriteActorThingArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WriteWeaponThingArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WriteExplosionThingArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WtiteItemThingArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WriteParticleThingArgs(const SithThing* pThing, const SithThing* pBasedOn);
int J3DAPI sithWrite_WriteAIArgs(const SithThing* pThing, const SithThing* pBasedOn);

J3D_EXTERN_C_END

#endif // SITH_SITHWRITE_H
