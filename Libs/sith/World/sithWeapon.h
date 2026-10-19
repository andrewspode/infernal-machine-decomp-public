#ifndef SITH_SITHWEAPON_H
#define SITH_SITHWEAPON_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/types.h>
#include <std/General/stdConffile.h>

J3D_EXTERN_C_START

#define SITHWEAPON_HOLSTWEWHIP   1
#define SITHWEAPON_HOLSTERPISTOL 2
#define SITHWEAPON_HOLSTERBACK   3

extern float sithWeapon_secMountWait;

void J3DAPI sithWeapon_Open(void);
void sithWeapon_Close(void); // Retail; body folded into the shared ret at 0x004EE700

void J3DAPI sithWeapon_InitalizeActor(SithThing* pThing);
void J3DAPI sithWeapon_UpdateActorWeaponState(SithThing* pThing, float secDeltaTime);
void J3DAPI sithWeapon_Update(SithThing* pThing, float secDeltaTime);

int J3DAPI sithWeapon_SelectWeapon(SithThing* pThing, SithWeaponId typeId);

int J3DAPI sithWeapon_ParseArg(const StdConffileArg* pArg, SithThing* pThing, int adjNum);

SithThing* J3DAPI sithWeapon_WeaponFire(SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* pFireDir, rdVector3* pFirePos, tSoundHandle hFireSnd, SithPuppetSubMode submode, float extra, SithFireProjectileFlags projectileFlags, float secDeltaTime);
SithThing* J3DAPI sithWeapon_WeaponFireProjectile(SithThing* pShooter, const SithThing* pProjectileTemplate, const rdVector3* pFireDir, rdVector3* pFirePos, tSoundHandle hFireSnd, SithPuppetSubMode submode, float extra, SithFireProjectileFlags flags, float secDeltaTime);

void J3DAPI sithWeapon_DamageWeapon(SithThing* pThing, const SithThing* pPurpetrator, float damage);
int J3DAPI sithWeapon_ThingCollisionHandler(SithThing* pWeapon, SithThing* pThing, SithCollision* pCollision, int bSecondThingIsSource);
int J3DAPI sithWeapon_SurfaceCollisionHandler(SithThing* pThing, SithSurface* pSurf, SithCollision* pStack);

void J3DAPI sithWeapon_DestroyWeapon(SithThing* pWeapon);
SithThing* J3DAPI sithWeapon_CreateWeaponExplosion(SithThing* pWeapon, SithThing* pExplosionTemplate, const rdVector3* pDir, int bRotate);

float J3DAPI sithWeapon_GetMountWait(void);
void J3DAPI sithWeapon_SetMountWait(float secWait);

void J3DAPI sithWeapon_SetFireWait(SithThing* pThing, float waitTime);
void J3DAPI sithWeapon_SetAimWait(SithThing* pThing, float waitTime);

void J3DAPI sithWeapon_ActivateWeapon(SithThing* pThing, SithCog* pCog, float waitTime);
float J3DAPI sithWeapon_DeactivateWeapon(SithThing* pThing, SithCog* pCog);

int J3DAPI sithWeapon_ProcessWeaponControls(SithThing* pThing, float secDeltaTime);

void J3DAPI sithWeapon_DeselectWeapon(SithThing* pThing);
int J3DAPI sithWeapon_IsMountingWeapon(const SithThing* pThing);
int J3DAPI sithWeapon_HasWeaponSelected(const SithThing* pThing);

int J3DAPI sithWeapon_GetAimOrient(rdMatrix34* pOutOrient, SithThing* pShooter, const rdMatrix34* pStartOrient, const rdVector3* pFireOffset, float autoAimFovX, float autoAimFovZ);
SithThing* J3DAPI sithWeapon_FireProjectile(SithThing* pShooter, const SithThing* pProjectile, tSoundHandle hFireSnd, SithPuppetSubMode submode, rdVector3* pFireOffset, rdVector3* pAimError, float extra, SithFireProjectileFlags flags, float autoAimFovX, float autoAimFovZ);

void J3DAPI sithWeapon_DeactivateCurrentWeapon(SithThing* pThing);
int J3DAPI sithWeapon_SelectNextWeapon(SithThing* pThing);
int J3DAPI sithWeapon_SelectPreviousWeapon(SithThing* pThing);

void J3DAPI sithWeapon_SetWeaponModel(SithThing* pThing, SithWeaponId weaponID);

void J3DAPI sithWeapon_ResetWeaponModel(SithThing* pThing);
void J3DAPI sithWeapon_SetHolsterModel(SithThing* pThing, SithWeaponId weaponID, signed int meshNum);
void J3DAPI sithWeapon_ResetHolsterModel(SithThing* pThing, int holsterNum);
void J3DAPI sithWeapon_EnablBloodsplort(int bEnable); // discarded (writer of bGenBloodsplort)

void J3DAPI sithWeapon_SendMessageAim(SithThing* pThing, int bAim);
void J3DAPI sithWeapon_SetActorKilledCallback(SithWeaponActorKilledCallback pfCallback);
int J3DAPI sithWeapon_IsAiming(SithThing* pThing);

void J3DAPI sithWeapon_GenBloodsplort(SithThing* pHitThing);

SithWeaponId J3DAPI sithWeapon_GetLastWeapon(void);
void J3DAPI sithWeapon_SetLastWeapon(SithWeaponId weaponNum);

float J3DAPI sithWeapon_GetWeaponMaxAimDistance(SithWeaponId weaponId);
float J3DAPI sithWeapon_GetWeaponCollideSize(SithWeaponId typeId);

int J3DAPI sithWeapon_Save(tFileHandle fh);
int J3DAPI sithWeapon_Restore(tFileHandle fh);

SithWeaponId J3DAPI sithWeapon_GetWeaponForControlKey(SithControlFunction keyID);

SithThing* J3DAPI sithWeapon_FireProjectileEx(SithThing* pShooter, const SithThing* pProjectile, tSoundHandle hSndFire, SithPuppetSubMode submode, rdVector3* pFirePos, const rdVector3* pTargetPos, float extra, SithFireProjectileFlags flags, float autoAinFovX, float autoAimFovZ, const rdVector3* pFireOffset, int bUseFireOffset);
void J3DAPI sithWeapon_CreateWeaponFireFx(SithThing* pThing, rdVector3* pos);
int J3DAPI sithWeapon_ActivateWhip(SithThing* pThing, SithCog* pCog);

J3D_EXTERN_C_END
#endif // SITH_SITHWEAPON_H
