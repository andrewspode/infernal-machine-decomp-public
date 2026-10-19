#include "sithCogFunction.h"

#include <rdroid/Math/rdMatrix.h>
#include <rdroid/Math/rdVector.h>
#include <rdroid/types.h>

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithControl.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithCamera.h>
#include <sith/Engine/sithCollision.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithMain.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <std/General/stdFnames.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>















void J3DAPI sithCogFunction_GetSenderID(SithCog* pCog);
void J3DAPI sithCogFunction_GetSenderRef(SithCog* pCog);
void J3DAPI sithCogFunction_GetSenderType(SithCog* pCog);
void J3DAPI sithCogFunction_GetSourceRef(SithCog* pCog);
void J3DAPI sithCogFunction_GetSourceType(SithCog* pCog);
void J3DAPI sithCogFunction_GetSysDate(SithCog* pCog);
void J3DAPI sithCogFunction_GetSysTime(SithCog* pCog);
void J3DAPI sithCogFunction_Sleep(SithCog* pCog);
void J3DAPI sithCogFunction_Print(SithCog* pCog);
void J3DAPI sithCogFunction_PrintVector(SithCog* pCog);
void J3DAPI sithCogFunction_PrintFlex(SithCog* pCog);
void J3DAPI sithCogFunction_PrintInt(SithCog* pCog);
void J3DAPI sithCogFunction_PrintHex(SithCog* pCog);
void J3DAPI sithCogFunction_SurfaceAnim(SithCog* pCog);
void J3DAPI sithCogFunction_MaterialAnim(SithCog* pCog);
void J3DAPI sithCogFunction_StopMaterialAnim(SithCog* pCog);
void J3DAPI sithCogFunction_StopAnim(SithCog* pCog);
void J3DAPI sithCogFunction_StopSurfaceAnim(SithCog* pCog);
void J3DAPI sithCogFunction_GetSurfaceAnim(SithCog* pCog);
void J3DAPI sithCogFunction_LoadTemplate(SithCog* pCog);
void J3DAPI sithCogFunction_LoadKeyframe(SithCog* pCog);
void J3DAPI sithCogFunction_LoadModel(SithCog* pCog);
void J3DAPI sithCogFunction_SetPulse(SithCog* pCog);
void J3DAPI sithCogFunction_SetTimer(SithCog* pCog);
void J3DAPI sithCogFunction_SetTimerEx(SithCog* pCog);
void J3DAPI sithCogFunction_KillTimerEx(SithCog* pCog);
void J3DAPI sithCogFunction_Reset(SithCog* pCog);
void J3DAPI sithCogFunction_VectorSet(SithCog* pCog);
void J3DAPI sithCogFunction_VectorAdd(SithCog* pCog);
void J3DAPI sithCogFunction_VectorSub(SithCog* pCog);
void J3DAPI sithCogFunction_VectorDot(SithCog* pCog);
void J3DAPI sithCogFunction_VectorCross(SithCog* pCog);
void J3DAPI sithCogFunction_VectorLen(SithCog* pCog);
void J3DAPI sithCogFunction_VectorScale(SithCog* pCog);
void J3DAPI sithCogFunction_VectorDist(SithCog* pCog);
void J3DAPI sithCogFunction_VectorEqual(SithCog* pCog);
void J3DAPI sithCogFunction_VectorRotate(SithCog* pCog);
void J3DAPI sithCogFunction_VectorTransformToOrient(SithCog* pCog);
void J3DAPI sithCogFunction_SendMessage(SithCog* pCog);
void J3DAPI sithCogFunction_SendMessageEx(SithCog* pCog);
void J3DAPI sithCogFunction_GetKeyLen(SithCog* pCog);
void J3DAPI sithCogFunction_GetSithMode(SithCog* pCog);
void J3DAPI sithCogFunction_GetGameTime(SithCog* pCog);
void J3DAPI sithCogFunction_GetFlexGameTime(SithCog* pCog);
void J3DAPI sithCogFunction_GetDifficulty(SithCog* pCog);
void J3DAPI sithCogFunction_SetSubModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_ClearSubModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_GetSubModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_SetDebugModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_ClearDebugModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_GetDebugModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_BitSet(SithCog* pCog);
void J3DAPI sithCogFunction_BitTest(SithCog* pCog);
void J3DAPI sithCogFunction_BitClear(SithCog* pCog);
void J3DAPI sithCogFunction_GetLevelTime(SithCog* pCog);
void J3DAPI sithCogFunction_GetThingCount(SithCog* pCog);
void J3DAPI sithCogFunction_GetThingTemplateCount(SithCog* pCog);
void J3DAPI sithCogFunction_GetGravity(SithCog* pCog);
void J3DAPI sithCogFunction_SetGravity(SithCog* pCog);
void J3DAPI sithCogFunction_ReturnEx(SithCog* pCog);
void J3DAPI sithCogFunction_GetParam(SithCog* pCog);
void J3DAPI sithCogFunction_SetParam(SithCog* pCog);
void J3DAPI sithCogFunction_VectorX(SithCog* pCog);
void J3DAPI sithCogFunction_VectorY(SithCog* pCog);
void J3DAPI sithCogFunction_VectorZ(SithCog* pCog);
void J3DAPI sithCogFunction_VectorNorm(SithCog* pCog);
void J3DAPI sithCogFunction_SetMaterialCel(SithCog* pCog);
void J3DAPI sithCogFunction_GetMaterialCel(SithCog* pCog);
void J3DAPI sithCogFunction_SetInvFlags(SithCog* pCog);
void J3DAPI sithCogFunction_SetMapModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_GetMapModelFlags(SithCog* pCog);
void J3DAPI sithCogFunction_ClearMapModeFlags(SithCog* pCog);
void J3DAPI sithCogFunction_SetCameraFocus(SithCog* pCog);
void J3DAPI sithCogFunction_SetCameraSecondaryFocus(SithCog* pCog);
void J3DAPI sithCogFunction_GetPrimaryFocus(SithCog* pCog);
void J3DAPI sithCogFunction_GetSecondaryFocus(SithCog* pCog);
void J3DAPI sithCogFunction_SetCameraMode(SithCog* pCog);
void J3DAPI sithCogFunction_GetCameraMode(SithCog* pCog);
void J3DAPI sithCogFunction_CycleCamera(SithCog* pCog);
void J3DAPI sithCogFunction_SetPOVShake(SithCog* pCog);
void J3DAPI sithCogFunction_HeapNew(SithCog* pCog);
void J3DAPI sithCogFunction_HeapSet(SithCog* pCog);
void J3DAPI sithCogFunction_HeapGet(SithCog* pCog);
void J3DAPI sithCogFunction_HeapFree(SithCog* pCog);
void J3DAPI sithCogFunction_GetSelfCog(SithCog* pCog);
void J3DAPI sithCogFunction_GetMasterCog(SithCog* pCog);
void J3DAPI sithCogFunction_SetMasterCog(SithCog* pCog);
void J3DAPI sithCogFunction_GetCogByIndex(SithCog* pCog);
void J3DAPI sithCogFunction_FireProjectile(SithCog* pCog);
void J3DAPI sithCogFunction_SendTrigger(SithCog* pCog);
void J3DAPI sithCogFunction_ActivateWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_DeactivateWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_DeactivateCurrentWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_SetFireWait(SithCog* pCog);
void J3DAPI sithCogFunction_SetMountWait(SithCog* pCog);
void J3DAPI sithCogFunction_SetAimWait(SithCog* pCog);
void J3DAPI sithCogFunction_SelectWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_SelectWeaponWait(SithCog* pCog);
void J3DAPI sithCogFunction_DeselectWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_DeselectWeaponWait(SithCog* pCog);
void J3DAPI sithCogFunction_SetCurWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_GetCurWeapon(SithCog* pCog);
void J3DAPI sithCogFunction_GetCameraStateFlags(SithCog* pCog);
void J3DAPI sithCogFunction_SetCameraStateFlags(SithCog* pCog);
void J3DAPI sithCogFunction_IsMulti(SithCog* pCog);
void J3DAPI sithCogFunction_IsServer(SithCog* pCog);
void J3DAPI sithCogFunction_AutoSavegame(SithCog* pCog);
void J3DAPI sithCogFunction_GetHintSolved(SithCog* pCog);
void J3DAPI sithCogFunction_SetHintSolved(SithCog* pCog);
void J3DAPI sithCogFunction_SetHintUnsolved(SithCog* pCog);
void J3DAPI sithCogFunction_Rand(SithCog* pCog);
void J3DAPI sithCogFunction_RandBetween(SithCog* pCog);
void J3DAPI sithCogFunction_RandVec(SithCog* pCog);
void J3DAPI sithCogFunction_Round(SithCog* pCog);
void J3DAPI sithCogFunction_Truncate(SithCog* pCog);
void J3DAPI sithCogFunction_Abs(SithCog* pCog);
void J3DAPI sithCogFunction_Pow(SithCog* pCog);
void J3DAPI sithCogFunction_Sin(SithCog* pCog);
void J3DAPI sithCogFunction_Cos(SithCog* pCog);
void J3DAPI sithCogFunction_ArcTan(SithCog* pCog);
void J3DAPI sithCogFunction_SetWeaponModel(SithCog* pCog);
void J3DAPI sithCogFunction_ResetWeaponModel(SithCog* pCog);
void J3DAPI sithCogFunction_LoadHolsterModel(SithCog* pCog);
void J3DAPI sithCogFunction_SetHolsterModel(SithCog* pCog);
void J3DAPI sithCogFunction_ResetHolsterModel(SithCog* pCog);
void J3DAPI sithCogFunction_GetLastPistol(SithCog* pCog);
void J3DAPI sithCogFunction_GetLastRifle(SithCog* pCog);
void J3DAPI sithCogFunction_InEditor(SithCog* pCog);
void J3DAPI sithCogFunction_CopyPlayerHolsters(SithCog* pCog);
void J3DAPI sithCogFunction_GetPerformanceLevel(SithCog* pCog);
void J3DAPI sithCogFunction_IsLevelName(SithCog* pCog);
void J3DAPI sithCogFunction_SetFog(SithCog* pCog);
void J3DAPI sithCogFunction_FindNewSector(SithCog* pCog);
void J3DAPI sithCogFunction_FindNewSectorFromThing(SithCog* pCog);
void J3DAPI sithCogFunction_DebugPrint(SithCog* pCog);
void J3DAPI sithCogFunction_DebugFlex(SithCog* pCog);
void J3DAPI sithCogFunction_DebugInt(SithCog* pCog);
void J3DAPI sithCogFunction_DebugLocalSymbols(SithCog* pCog);
void J3DAPI sithCogFunction_DebugWaitForKey(SithCog* pCog);

int J3DAPI sithCogFunction_RegisterFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSenderRef, "getsenderref");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSenderType, "getsendertype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSenderID, "getsenderid");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSourceType, "getsourcetype");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSourceRef, "getsourceref");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSysDate, "getsysdate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSysTime, "getsystime");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_InEditor, "ineditor");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetThingCount, "getthingcount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetGravity, "getgravity");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetGravity, "setgravity");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetLevelTime, "getleveltime");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetGameTime, "getgametime");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetFlexGameTime, "getflexgametime");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetThingTemplateCount, "getthingtemplatecount");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetFog, "setfog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_FindNewSector, "findnewsector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_FindNewSectorFromThing, "findnewsectorfromthing");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Sleep, "sleep");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetPulse, "setpulse");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetTimer, "settimer");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetTimerEx, "settimerex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_KillTimerEx, "killtimerex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Reset, "reset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_MaterialAnim, "materialanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_StopMaterialAnim, "stopmaterialanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_StopAnim, "stopanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_StopSurfaceAnim, "stopsurfaceanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSurfaceAnim, "getsurfaceanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SurfaceAnim, "surfaceanim");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetKeyLen, "getkeylen");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_LoadTemplate, "loadtemplate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_LoadKeyframe, "loadkeyframe");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_LoadModel, "loadmodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Print, "print");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_PrintInt, "printint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_PrintFlex, "printflex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_PrintVector, "printvector");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_PrintHex, "printhex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorAdd, "vectoradd");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorSub, "vectorsub");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorDot, "vectordot");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorCross, "vectorcross");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorSet, "vectorset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorLen, "vectorlen");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorScale, "vectorscale");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorDist, "vectordist");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorX, "vectorx");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorY, "vectory");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorZ, "vectorz");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorNorm, "vectornorm");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorEqual, "vectorequal");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorRotate, "vectorrotate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_VectorTransformToOrient, "vectortransformtoorient");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSithMode, "getsithmode");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetDifficulty, "getdifficulty");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetSubModeFlags, "setsubmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSubModeFlags, "getsubmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ClearSubModeFlags, "clearsubmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetDebugModeFlags, "setdebugmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetDebugModeFlags, "getdebugmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ClearDebugModeFlags, "cleardebugmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_BitSet, "bitset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_BitTest, "bittest");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_BitClear, "bitclear");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_FireProjectile, "fireprojectile");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ActivateWeapon, "activateweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DeactivateWeapon, "deactivateweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DeactivateCurrentWeapon, "deactivatecurweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetMountWait, "setmountwait");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetFireWait, "setfirewait");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetAimWait, "setaimwait");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SelectWeapon, "selectweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SelectWeaponWait, "selectweaponwait");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DeselectWeapon, "deselectweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DeselectWeaponWait, "deselectweaponwait");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetCurWeapon, "setcurweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetCurWeapon, "getcurweapon");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetWeaponModel, "setweaponmodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ResetWeaponModel, "resetweaponmodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_LoadHolsterModel, "loadholstermodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetHolsterModel, "setholstermodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ResetHolsterModel, "resetholstermodel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetLastPistol, "getlastpistol");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetLastRifle, "getlastrifle");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_CopyPlayerHolsters, "copyplayerholsters");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SendMessage, "sendmessage");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SendMessageEx, "sendmessageex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ReturnEx, "returnex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetParam, "getparam");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetParam, "setparam");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetInvFlags, "setinvflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetMapModeFlags, "setmapmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetMapModelFlags, "getmapmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ClearMapModeFlags, "clearmapmodeflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetMaterialCel, "getmaterialcel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetMaterialCel, "setmaterialcel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraFocus, "setcamerafocus");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetPrimaryFocus, "getprimaryfocus");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSecondaryFocus, "getsecondaryfocus");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraMode, "setcurrentcamera");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetCameraMode, "getcurrentcamera");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_CycleCamera, "cyclecamera");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetPOVShake, "setpovshake");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraStateFlags, "setcamerastateflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetCameraStateFlags, "getcamerastateflags");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetCameraSecondaryFocus, "setcamerasecondaryfocus");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_HeapNew, "heapnew");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_HeapSet, "heapset");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_HeapGet, "heapget");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_HeapFree, "heapfree");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetSelfCog, "getselfcog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetMasterCog, "getmastercog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetMasterCog, "setmastercog");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetCogByIndex, "getcogbyindex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_IsMulti, "ismulti");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_IsServer, "isserver");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SendTrigger, "sendtrigger");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_AutoSavegame, "autosavegame");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetHintSolved, "gethintsolved");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetHintSolved, "sethintsolved");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_SetHintUnsolved, "sethintunsolved");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Rand, "rand");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_RandBetween, "randbetween");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_RandVec, "randvec");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Round, "round");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Truncate, "truncate");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Abs, "abs");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Pow, "pow");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Sin, "sin");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_Cos, "cos");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_ArcTan, "arctan");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_GetPerformanceLevel, "getperformancelevel");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_IsLevelName, "islevelname");
    if ( bFailed )
    {
        goto error;
    }
#ifdef J3D_DEBUG
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DebugPrint, "debugprint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DebugFlex, "debugflex");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DebugInt, "debugint");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DebugLocalSymbols, "debuglocalsymbols");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithCogFunction_DebugWaitForKey, "debugwaitforkey");
    if ( bFailed )
    {
        goto error;
    }
#endif

    return 0;

error:
    return 1;
}

void J3DAPI sithCogFunction_GetSenderID(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->linkId);
}

void J3DAPI sithCogFunction_GetSenderRef(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->senderIdx);
}

void J3DAPI sithCogFunction_GetSenderType(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->senderType);
}

void J3DAPI sithCogFunction_GetSourceRef(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->sourceIdx);
}

void J3DAPI sithCogFunction_GetSourceType(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->sourceType);
}

void J3DAPI sithCogFunction_GetSysDate(SithCog* pCog)
{
    rdVector3 vec;

    SYSTEMTIME date;
    GetLocalTime(&date);

    vec.x = (float)date.wYear;
    vec.y = (float)date.wMonth;
    vec.z = (float)date.wDay;
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_GetSysTime(SithCog* pCog)
{
    rdVector3 vec;

    SYSTEMTIME time;
    GetLocalTime(&time);

    vec.x = (float)time.wHour;
    vec.y = (float)time.wMinute;
    vec.z = (float)time.wSecond;
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_Sleep(SithCog* pCog)
{
    float secWait = sithCogExec_PopFlex(pCog);
    if ( secWait <= 0.0f )
    {
#line 756
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Wait time less than 0 in Sleep(); assuming .1 seconds\n", pCog->aName, secWait, 0, 0);
        secWait = 0.1f;
    }

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: Sleeping for %f seconds.\n", pCog->aName, secWait);
        sithConsole_PrintString(std_genBuffer);
    }

    secWait *= 1000.0f;
    pCog->status = SITHCOG_STATUS_SLEEPING;
    pCog->statusParams[0] = sithTime_msecGameTime + (int)secWait;
}

void J3DAPI sithCogFunction_Print(SithCog* pCog)
{
    const char* pStr = sithCogExec_PopString(pCog);
#ifdef J3D_DEBUG
    if ( pStr )
    {
        sithConsole_PrintString(pStr);
    }
#endif
}

void J3DAPI sithCogFunction_PrintVector(SithCog* pCog)
{
    rdVector3 vec;
    char aVecStr[64];
    if ( sithCogExec_PopVector(pCog, &vec) )
    {
        stdUtil_Format(aVecStr, STD_ARRAYLEN(aVecStr), "<%f %f %f>", vec.x, vec.y, vec.z);
    }
    else
    {
        stdUtil_Format(aVecStr, STD_ARRAYLEN(aVecStr), "Bad vector");
    }
#ifdef J3D_DEBUG
    sithConsole_PrintString(aVecStr);
#endif
}

void J3DAPI sithCogFunction_PrintFlex(SithCog* pCog)
{
    char aText[64];
    float val = sithCogExec_PopFlex(pCog);
#ifdef J3D_DEBUG
    stdUtil_Format(aText, STD_ARRAYLEN(aText), "%f", val);
    sithConsole_PrintString(aText);
#endif
}

void J3DAPI sithCogFunction_PrintInt(SithCog* pCog)
{
    char aText[64];
    int val = sithCogExec_PopInt(pCog);
#ifdef J3D_DEBUG
    stdUtil_Format(aText, STD_ARRAYLEN(aText), "%d", val);
    sithConsole_PrintString(aText);
#endif
}

void J3DAPI sithCogFunction_PrintHex(SithCog* pCog)
{
    char aText[64];
    int val = sithCogExec_PopInt(pCog);
#ifdef J3D_DEBUG
    stdUtil_Format(aText, STD_ARRAYLEN(aText), "0x%x", val);
    sithConsole_PrintString(aText);
#endif
}

void J3DAPI sithCogFunction_SurfaceAnim(SithCog* pCog)
{
    SithAnimationSlot* pAnim;

    SithAnimateFlags flags = sithCogExec_PopInt(pCog);
    float fps              = sithCogExec_PopFlex(pCog);
    SithSurface* pSurf     = sithCogExec_PopSurface(pCog);

    if ( !pSurf )
    {
#line 927
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface index.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        if ( fps <= 0.0f )
        {
#line 933
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal anim speed %f, changed to 15fps.\n", fps, 0, 0, 0);
            fps = 15.0f;
        }

        pAnim = sithAnimate_StartSurfaceAnim(pSurf, fps, flags);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunction_MaterialAnim(SithCog* pCog)
{
    SithAnimationSlot* pAnim;

    int flags        = sithCogExec_PopInt(pCog);
    float fps        = sithCogExec_PopFlex(pCog);
    rdMaterial* pMat = sithCogExec_PopMaterial(pCog);

    if ( !pMat )
    {
#line 973
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad material index.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        if ( fps <= 0.0f )
        {
#line 979
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal anim speed %f, changed to 15fps.\n", fps, 0, 0, 0);
            fps = 15.0f;
        }

        pAnim = sithAnimate_StartMaterialAnim(pMat, fps, (SithAnimateFlags)flags);
        if ( pAnim )
        {
            sithCogExec_PushInt(pCog, pAnim->animID);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunction_StopMaterialAnim(SithCog* pCog)
{
    SithAnimationSlot* pAnim;

    rdMaterial* pMaterial = sithCogExec_PopMaterial(pCog);
    if ( !pMaterial )
    {
#line 1006
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad material in StopMaterialAnim.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pAnim = sithAnimate_GetMaterialAnim(pMaterial);
    if ( pAnim )
    {
        sithAnimate_Stop(pAnim);
    }
}

void J3DAPI sithCogFunction_StopAnim(SithCog* pCog)
{
    int animID               = sithCogExec_PopInt(pCog);
    SithAnimationSlot* pAnim = sithAnimate_GetAnim(animID);
    if ( pAnim )
    {
        sithAnimate_Stop(pAnim);
        if ( sithMessage_outputstream )
        {
            sithDSS_AnimStatus(pAnim, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
        }
    }
}

void J3DAPI sithCogFunction_StopSurfaceAnim(SithCog* pCog)
{
    SithAnimationSlot* pAnim;

    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    if ( pSurf )
    {
        pAnim = sithAnimate_GetSurfaceAnim(pSurf);
        if ( pAnim )
        {
            sithAnimate_Stop(pAnim);
            if ( sithMessage_outputstream )
            {
                sithDSS_AnimStatus(pAnim, SITHMESSAGE_SENDTOJOINEDPLAYERS, SITHMESSAGE_STREAM_ALL);
            }
        }
        else
        {
#line 1081
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Tried to stop anim on surface: no anim\n", 0, 0, 0, 0);
        }
    }
    else
    {
#line 1085
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to call StopSurfaceAnim on an invalid surface.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_GetSurfaceAnim(SithCog* pCog)
{
    SithSurface* pSurf = sithCogExec_PopSurface(pCog);
    int animIdx;
    if ( pSurf )
    {
        animIdx = sithAnimate_GetSurfaceAnimIndex(pSurf);
        sithCogExec_PushInt(pCog, animIdx);
    }
    else
    {
#line 1112
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to call GetSurfaceAnim on an invalid surface.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_LoadTemplate(SithCog* pCog)
{
    const char* pTemplateName = sithCogExec_PopString(pCog);

    SithThing* pTemplate;
    if ( pTemplateName )
    {
        pTemplate = sithTemplate_GetTemplate(pTemplateName);
        if ( pTemplate )
        {
            sithCogExec_PushInt(pCog, pTemplate->idx);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_LoadKeyframe(SithCog* pCog)
{
    const char* pKeyFilename = sithCogExec_PopString(pCog);

    rdKeyframe* pKeyframe;
    if ( pKeyFilename )
    {
        pKeyframe = sithPuppet_LoadKeyframe(pKeyFilename);
        if ( pKeyframe )
        {
            sithCogExec_PushInt(pCog, pKeyframe->idx);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_LoadModel(SithCog* pCog)
{
    const char* pFilename = sithCogExec_PopString(pCog);

    rdModel3* pModel;
    if ( pFilename )
    {
        pModel = sithModel_Load(pFilename, 1);
        if ( pModel )
        {
            sithCogExec_PushInt(pCog, pModel->num);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_SetPulse(SithCog* pCog)
{
    float interval = sithCogExec_PopFlex(pCog);

    if ( interval > 0.0f )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "Cog %s: Pulse set to %f seconds.\n", pCog->aName, interval);
            sithConsole_PrintString(std_genBuffer);
        }

        pCog->flags |= SITHCOG_PULSE_SET;
        pCog->msecPulseInterval = (int)(interval * 1000.0f);
        pCog->msecNextPulseTime = sithTime_msecGameTime + pCog->msecPulseInterval;
    }
    else
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "Cog %s: Pulse disabled.\n", pCog->aName);
            sithConsole_PrintString(std_genBuffer);
        }

        pCog->flags &= ~SITHCOG_PULSE_SET;
    }
}

void J3DAPI sithCogFunction_SetTimer(SithCog* pCog)
{
    float when = sithCogExec_PopFlex(pCog);

    if ( when > 0.0f )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "Cog %s: Timer set for %f seconds.\n", pCog->aName, when);
            sithConsole_PrintString(std_genBuffer);
        }

        pCog->flags |= SITHCOG_TIMER_SET;
        pCog->msecTimerTimeout = sithTime_msecGameTime + (int)D3DVAL(RD_MULTIPLY(when, 1000.0f));
    }
    else
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "Cog %s: Timer cancelled.\n", pCog->aName);
            sithConsole_PrintString(std_genBuffer);
        }

        pCog->flags &= ~SITHCOG_TIMER_SET;
    }
}

void J3DAPI sithCogFunction_SetTimerEx(SithCog* pCog)
{
    int msWhen;

    SithEventParams params;
    params.param3 = sithCogExec_PopInt(pCog);
    params.param2 = sithCogExec_PopInt(pCog);
    params.param1 = sithCogExec_PopInt(pCog);
    params.idx = pCog->idx;
    msWhen = lrintf(sithCogExec_PopFlex(pCog) * 1000.0f);

    if ( msWhen < 0 )
    {
        return;
    }

    sithEvent_CreateEvent(SITHCOG_TASKID, &params, msWhen);
}

void J3DAPI sithCogFunction_KillTimerEx(SithCog* pCog)
{
    SithEvent* currentEvent;
    SithEvent* pCurNextEvent;
    SithEvent* pPrevEvent;

    int timerId = sithCogExec_PopInt(pCog);
    if ( timerId <= 0 )
    {
        return;
    }

    pPrevEvent = NULL;
    for ( currentEvent = sithEvent_pFirstQueuedEvent; currentEvent; pPrevEvent = currentEvent, currentEvent = pCurNextEvent )
    {
        pCurNextEvent = currentEvent->pNextEvent;
        if ( currentEvent->taskNum == SITHCOG_TASKID && currentEvent->params.idx == pCog->idx && currentEvent->params.param1 == timerId )
        {
            if ( pPrevEvent )
            {
                pPrevEvent->pNextEvent = pCurNextEvent;
            }
            else
            {
                sithEvent_pFirstQueuedEvent = pCurNextEvent;
            }

            sithEvent_FreeEvent(currentEvent);
            currentEvent = pPrevEvent;
        }
    }
}

void J3DAPI sithCogFunction_Reset(SithCog* pCog)
{
    pCog->callDepth = 0;
}

void J3DAPI sithCogFunction_VectorSet(SithCog* pCog)
{
    rdVector3 vec;
    float x;
    float y;
    float z;

    z     = sithCogExec_PopFlex(pCog);
    y     = sithCogExec_PopFlex(pCog);
    x     = sithCogExec_PopFlex(pCog);
    vec.x = x;
    vec.y = y;
    vec.z = z;
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_VectorAdd(SithCog* pCog)
{
    rdVector3 cura;
    rdVector3 c;

    rdVector3 curb;
    sithCogExec_PopVector(pCog, &curb);
    sithCogExec_PopVector(pCog, &cura);

    rdVector_Add3(&c, &cura, &curb);
    sithCogExec_PushVector(pCog, &c);
}

void J3DAPI sithCogFunction_VectorSub(SithCog* pCog)
{
    rdVector3 cura;
    rdVector3 c;

    rdVector3 curb;
    sithCogExec_PopVector(pCog, &curb);
    sithCogExec_PopVector(pCog, &cura);

    rdVector_Sub3(&c, &cura, &curb);
    sithCogExec_PushVector(pCog, &c);
}

void J3DAPI sithCogFunction_VectorDot(SithCog* pCog)
{
    rdVector3 cura;
    float c;

    rdVector3 curb;
    sithCogExec_PopVector(pCog, &curb);
    sithCogExec_PopVector(pCog, &cura);

    c = rdVector_Dot3(&cura, &curb);
    sithCogExec_PushFlex(pCog, c);
}

void J3DAPI sithCogFunction_VectorCross(SithCog* pCog)
{
    rdVector3 cura;
    rdVector3 c;

    rdVector3 curb;
    sithCogExec_PopVector(pCog, &curb);
    sithCogExec_PopVector(pCog, &cura);

    rdVector_Cross3(&c, &cura, &curb); // Note: in debug version is 'a cross b' but in release version 'b cross a'
    sithCogExec_PushVector(pCog, &c);
}

void J3DAPI sithCogFunction_VectorLen(SithCog* pCog)
{
    rdVector3 vec;
    float length;
    sithCogExec_PopVector(pCog, &vec);
    length = sqrtf(rdVector_Dot3(&vec, &vec));
    sithCogExec_PushFlex(pCog, length);
}

void J3DAPI sithCogFunction_VectorScale(SithCog* pCog)
{
    rdVector3 b;

    rdVector3 cura;
    float currentscalar = sithCogExec_PopFlex(pCog);
    sithCogExec_PopVector(pCog, &cura);

    rdVector_Scale3(&b, &cura, currentscalar);
    sithCogExec_PushVector(pCog, &b);
}

void J3DAPI sithCogFunction_VectorDist(SithCog* pCog)
{
    rdVector3 currenta;
    float dist;

    rdVector3 currentb;
    sithCogExec_PopVector(pCog, &currentb);
    sithCogExec_PopVector(pCog, &currenta);

    dist = rdVector_Dist3(&currenta, &currentb);
    sithCogExec_PushFlex(pCog, dist);
}

void J3DAPI sithCogFunction_VectorEqual(SithCog* pCog)
{
    rdVector3 a;

    rdVector3 b;
    sithCogExec_PopVector(pCog, &b);
    sithCogExec_PopVector(pCog, &a);

    if ( a.x == b.x && a.y == b.y && a.z == b.z )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_VectorRotate(SithCog* pCog)
{
    rdVector3 vec;

    rdVector3 pyr;
    sithCogExec_PopVector(pCog, &pyr);
    sithCogExec_PopVector(pCog, &vec);

    rdVector_Rotate3Acc(&vec, &pyr);
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_VectorTransformToOrient(SithCog* pCog)
{
    rdVector3 vec;
    int bCurVector = sithCogExec_PopVector(pCog, &vec);
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || !bCurVector )
    {
#line 1701
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad parameters in VectorTransformToOrient() call.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushVector(pCog, &rdroid_zeroVector3);
        return;
    }

    rdMatrix_TransformVector34Acc(&vec, &pThing->orient);
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_SendMessage(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    SithCogMsgType msgType = sithCogExec_PopInt(pCog);
    SithCog* pDstCog       = sithCogExec_PopCog(pCog);
    if ( !pDstCog )
    {
#line 1735
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to send message to invalid cog %s.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( msgType < 0 || msgType >= SITHCOG_NUMMSGTYPES )
    {
#line 1741
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to send invalid message %d\n", pCog->aName, msgType, 0, 0);
        return;
    }

    sithCog_SendMessage(pDstCog, msgType, SITHCOG_SYM_REF_COG, pCog->idx, pCog->sourceType, pCog->sourceIdx, 0);
}

void J3DAPI sithCogFunction_SendMessageEx(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int aParams[4];
    int currentresult;
    SithCogMsgType msg;
    SithCog* pDstCog;

    aParams[3] = sithCogExec_PopInt(pCog);
    aParams[2] = sithCogExec_PopInt(pCog);
    aParams[1] = sithCogExec_PopInt(pCog);
    aParams[0] = sithCogExec_PopInt(pCog);
    msg     = sithCogExec_PopInt(pCog);
    pDstCog = sithCogExec_PopCog(pCog);

    if ( !pDstCog )
    {
#line 1779
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to send message to invalid cog.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( msg < 0 || msg >= SITHCOG_NUMMSGTYPES )
    {
#line 1785
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Trying to send invalid message %d\n", pCog->aName, msg, 0, 0);
        return;
    }

    currentresult = sithCog_SendMessageEx(pDstCog, msg, SITHCOG_SYM_REF_COG, pCog->idx, pCog->sourceType, pCog->sourceIdx, 0, aParams[0], aParams[1], aParams[2], aParams[3]);
    sithCogExec_PushInt(pCog, currentresult);
}

void J3DAPI sithCogFunction_GetKeyLen(SithCog* pCog)
{
    rdKeyframe* pKeyframe = sithCogExec_PopKeyframe(pCog);
    if ( !pKeyframe || pKeyframe->fps == 0.0f )
    {
#line 1822
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad Track reference passed to GetKeyLen.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

#line 1825
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: GetKeyLen %s %d %f.\n", pCog->aName, pKeyframe->aName, pKeyframe->numFrames, pKeyframe->fps);
    sithCogExec_PushFlex(pCog, (float)pKeyframe->numFrames / pKeyframe->fps);
}

void J3DAPI sithCogFunction_GetSithMode(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sith_mode.masterMode);
}

void J3DAPI sithCogFunction_GetGameTime(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithTime_msecGameTime);
}

void J3DAPI sithCogFunction_GetFlexGameTime(SithCog* pCog)
{
    sithCogExec_PushFlex(pCog, sithTime_secGameTime);
}

void J3DAPI sithCogFunction_GetDifficulty(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sith_mode.difficulty);
}

void J3DAPI sithCogFunction_SetSubModeFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sith_mode.subModeFlags |= flags;
}

void J3DAPI sithCogFunction_ClearSubModeFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sith_mode.subModeFlags &= ~flags;
}

void J3DAPI sithCogFunction_GetSubModeFlags(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sith_mode.subModeFlags);
}

void J3DAPI sithCogFunction_SetDebugModeFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sith_mode.debugModeFlags |= flags;
}

void J3DAPI sithCogFunction_ClearDebugModeFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sith_mode.debugModeFlags &= ~flags;
}

void J3DAPI sithCogFunction_GetDebugModeFlags(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sith_mode.debugModeFlags);
}

void J3DAPI sithCogFunction_BitSet(SithCog* pCog)
{
    int flags;
    int curmask = sithCogExec_PopInt(pCog);
    int result;
    flags = sithCogExec_PopInt(pCog);
    result = flags |= curmask;
    sithCogExec_PushInt(pCog, result);
}

void J3DAPI sithCogFunction_BitTest(SithCog* pCog)
{
    int flags;
    int curmask = sithCogExec_PopInt(pCog);
    int result;
    flags = sithCogExec_PopInt(pCog);
    result = flags & curmask;
    sithCogExec_PushInt(pCog, result);
}

void J3DAPI sithCogFunction_BitClear(SithCog* pCog)
{
    int result;
    int mask  = sithCogExec_PopInt(pCog);
    int currentFlags = sithCogExec_PopInt(pCog);
    result = currentFlags &= ~mask;
    sithCogExec_PushInt(pCog, result);
}

void J3DAPI sithCogFunction_GetLevelTime(SithCog* pCog)
{
    float secTime = (float)sithTime_msecGameTime / 1000.0f;
    sithCogExec_PushFlex(pCog, secTime);
}

void J3DAPI sithCogFunction_GetThingCount(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithWorld_pCurrentWorld->numThings);
}

void J3DAPI sithCogFunction_GetThingTemplateCount(SithCog* pCog)
{
    int i;
    int count;
    SithThing* pThing;

    SithWorld* pWorld    = sithWorld_pCurrentWorld;
    SithThing* pTemplate = sithCogExec_PopTemplate(pCog);
    if ( !pTemplate )
    {
#line 2145
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad template reference passed to GetThingTemplateCount.\n", pCog->aName, 0, 0, 0);
        return;
    }

    count = 0;
    for ( i = 0; i < pWorld->lastThingIdx; ++i )
    {
        pThing = &pWorld->aThings[i];
        if ( pThing->type && pThing->type != SITH_THING_CORPSE && pThing->pTemplate == pTemplate )
        {
            ++count;
        }
    }

    sithCogExec_PushInt(pCog, count);
}

void J3DAPI sithCogFunction_GetGravity(SithCog* pCog)
{
    sithCogExec_PushFlex(pCog, sithWorld_pCurrentWorld->gravity);
}

void J3DAPI sithCogFunction_SetGravity(SithCog* pCog)
{
    float gravity = sithCogExec_PopFlex(pCog);
    sithWorld_pCurrentWorld->gravity = gravity;
}

void J3DAPI sithCogFunction_ReturnEx(SithCog* pCog)
{
    int retval = sithCogExec_PopInt(pCog);
    pCog->returnValue = retval;
}

void J3DAPI sithCogFunction_GetParam(SithCog* pCog)
{
    int idx = sithCogExec_PopInt(pCog);
    int val;
    if ( idx >= 0 && idx < (int)STD_ARRAYLEN(pCog->params) )
    {
        val = pCog->params[idx];
        sithCogExec_PushInt(pCog, val);
    }
    else
    {
        sithCogExec_PushInt(pCog, -9999);
    }
}

void J3DAPI sithCogFunction_SetParam(SithCog* pCog)
{
    int val = sithCogExec_PopInt(pCog);
    int num = sithCogExec_PopInt(pCog);
    if ( num >= 0 && num < (int)STD_ARRAYLEN(pCog->params) )
    {
        pCog->params[num] = val;
    }
}

void J3DAPI sithCogFunction_VectorX(SithCog* pCog)
{
    rdVector3 vec;

    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, vec.x);
}

void J3DAPI sithCogFunction_VectorY(SithCog* pCog)
{
    rdVector3 vec;

    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, vec.y);
}

void J3DAPI sithCogFunction_VectorZ(SithCog* pCog)
{
    rdVector3 vec;

    sithCogExec_PopVector(pCog, &vec);
    sithCogExec_PushFlex(pCog, vec.z);
}

void J3DAPI sithCogFunction_VectorNorm(SithCog* pCog)
{
    rdVector3 vec;
    rdVector3 dest;

    sithCogExec_PopVector(pCog, &vec);
    rdVector_Normalize3(&dest, &vec);
    sithCogExec_PushVector(pCog, &dest);
}

void J3DAPI sithCogFunction_SetMaterialCel(SithCog* pCog)
{
    int celCount       = sithCogExec_PopInt(pCog);

    int curCelNum;
    rdMaterial* pCurMat = sithCogExec_PopMaterial(pCog);

    if ( !pCurMat || celCount < 0 || (size_t)celCount >= pCurMat->numCels )
    {
    }
    else
    {
        curCelNum = pCurMat->curCelNum;
        pCurMat->curCelNum = celCount;
        sithCogExec_PushInt(pCog, curCelNum);
        return;
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunction_GetMaterialCel(SithCog* pCog)
{
    rdMaterial* pMat = sithCogExec_PopMaterial(pCog);
    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunction_SetInvFlags(SithCog* pCog)
{
    int flags         = sithCogExec_PopInt(pCog);
    int bin           = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || pThing->type != SITH_THING_PLAYER || !pThing->thingInfo.actorInfo.pPlayer )
    {
#line 2450
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing in SetInvFlags.\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( bin >= SITHINVENTORY_MAXTYPES )
    {
#line 2456
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid inventory id in SetInvFlags.\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithInventory_SetInventoryFlags(pThing, bin, (SithInventoryTypeFlag)flags);
}

void J3DAPI sithCogFunction_SetMapModeFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sith_mode.mapModeFlags |= flags;
}

void J3DAPI sithCogFunction_GetMapModelFlags(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sith_mode.mapModeFlags);
}

void J3DAPI sithCogFunction_ClearMapModeFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sith_mode.mapModeFlags &= ~flags;
}

void J3DAPI sithCogFunction_SetCameraFocus(SithCog* pCog)
{
    SithCamera* pCamera;

    SithThing* pThing = sithCogExec_PopThing(pCog);
    int camNum        = sithCogExec_PopInt(pCog);

    if ( camNum > -1 && camNum < (int)STD_ARRAYLEN(sithCamera_aCameras) && pThing )
    {
        pCamera = &sithCamera_aCameras[camNum];
        if ( camNum == SITHCAMERA_CINEMACAMERANUM && pCamera->bPosInterp && pCamera->pPrimaryFocusThing )
        {
            memcpy(&pCamera->pos, &pCamera->pPrimaryFocusThing->pos, sizeof(pCamera->pos));
            pCamera->posInterpState = 0;
        }

        sithCamera_SetCameraFocus(pCamera, pThing, NULL);
    }
    else
    {
#line 2562
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera or focus in SetCameraFocus().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_SetCameraSecondaryFocus(SithCog* pCog)
{
    SithCamera* pCamera;
    SithThing* pFocus = sithCogExec_PopThing(pCog);
    int camNum        = sithCogExec_PopInt(pCog);

    if ( camNum > -1 && camNum < (int)STD_ARRAYLEN(sithCamera_aCameras) && pFocus )
    {
        pCamera = &sithCamera_aCameras[camNum];
        sithCamera_SetCameraFocus(pCamera, NULL, pFocus);
    }
    else
    {
#line 2583
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera or focus in SetCameraFocus().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_GetPrimaryFocus(SithCog* pCog)
{
    SithThing* pFocus;
    SithCamera* pCamera;

    int camNum = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < (int)STD_ARRAYLEN(sithCamera_aCameras) )
    {
        pCamera = &sithCamera_aCameras[camNum];
        pFocus = sithCamera_GetPrimaryFocus(pCamera);
        if ( pFocus )
        {
            sithCogExec_PushInt(pCog, pFocus->idx);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
#line 2619
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera or focus in GetPrimaryFocus().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_GetSecondaryFocus(SithCog* pCog)
{
    SithThing* pFocus;
    SithCamera* pCamera;

    int camNum = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < (int)STD_ARRAYLEN(sithCamera_aCameras) )
    {
        pCamera = &sithCamera_aCameras[camNum];
        pFocus = sithCamera_GetSecondaryFocus(pCamera);
        if ( pFocus )
        {
            sithCogExec_PushInt(pCog, pFocus->idx);
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
#line 2656
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera or focus in GetSecondaryFocus().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_SetCameraMode(SithCog* pCog)
{
    int camNum = sithCogExec_PopInt(pCog);
    if ( camNum > -1 && camNum < (int)STD_ARRAYLEN(sithCamera_aCameras) )
    {
        sithCamera_SetCurrentCamera(&sithCamera_aCameras[camNum]);
    }
    else
    {
#line 2684
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid camera in SetCameraMode().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_GetCameraMode(SithCog* pCog)
{
    int camNum;
    if ( sithCamera_pCurCamera && (camNum = sithCamera_pCurCamera - sithCamera_aCameras) < (int)STD_ARRAYLEN(sithCamera_aCameras) )
    {
        sithCogExec_PushInt(pCog, camNum);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_CycleCamera(SithCog* pCog)
{
    sithCamera_CycleCamera();
}

void J3DAPI sithCogFunction_SetPOVShake(SithCog* pCog)
{
    rdVector3 angleOffset;

    float angDelta = sithCogExec_PopFlex(pCog);
    float posDelta = sithCogExec_PopFlex(pCog);

    rdVector3 posOfs;
    if ( !sithCogExec_PopVector(pCog, &angleOffset) )
    {
#line 2757
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid vector in SetPOVShake().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !sithCogExec_PopVector(pCog, &posOfs) )
    {
#line 2763
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid vector in SetPOVShake().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithCamera_SetPOVShake(&posOfs, &angleOffset, posDelta, angDelta);
}

void J3DAPI sithCogFunction_HeapNew(SithCog* pCog)
{
    int size = sithCogExec_PopInt(pCog);
    if ( size <= 0 )
    {
        return;
    }

    if ( pCog->aHeap )
    {
        stdMemory_Free(pCog->aHeap);
        pCog->heapSize = 0;
    }

#line 2789
    pCog->aHeap = (SithCogSymbolValue*)STDMALLOC(sizeof(SithCogSymbolValue) * size);
    memset(pCog->aHeap, 0, sizeof(SithCogSymbolValue) * size);
    pCog->heapSize = size;
}

void J3DAPI sithCogFunction_HeapSet(SithCog* pCog)
{
    SithCogSymbolValue val;
    int bValidSym = sithCogExec_PopSymbol(pCog, &val);
    int n       = sithCogExec_PopInt(pCog);

    if ( !bValidSym || n < 0 || n >= pCog->heapSize )
    {
        return;
    }

    pCog->aHeap[n] = val;
}

void J3DAPI sithCogFunction_HeapGet(SithCog* pCog)
{
    SithCogSymbolValue value;

    int num = sithCogExec_PopInt(pCog);
    if ( num < 0 || num >= pCog->heapSize )
    {
        goto error;
    }

#line 2824
    SITH_ASSERTREL(pCog->aHeap);
    value = pCog->aHeap[num];
    sithCogExec_PushStack(pCog, &value);
    return;

error:
#line 2831
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "HeapGet: index %d out of range.\n", num, 0, 0, 0);
    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunction_HeapFree(SithCog* pCog)
{
    if ( pCog->aHeap )
    {
        stdMemory_Free(pCog->aHeap);
        pCog->heapSize = 0;
    }
}

void J3DAPI sithCogFunction_GetSelfCog(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, pCog->idx);
}

void J3DAPI sithCogFunction_GetMasterCog(SithCog* pCog)
{
    SithCog* pMasterCog = sithCog_pMasterCog;
    if ( pMasterCog )
    {
        sithCogExec_PushInt(pCog, pMasterCog->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_SetMasterCog(SithCog* pCog)
{
    SithCog* pMasterCog = sithCogExec_PopCog(pCog);
    sithCog_pMasterCog = pMasterCog;
}

void J3DAPI sithCogFunction_GetCogByIndex(SithCog* pCog)
{
    SithCog* pRetCog;

    // Function retrieves cog from static resources (jones3Dstatic)
    int index = sithCogExec_PopInt(pCog);

    pRetCog = sithCog_GetCogByIndex(SITHWORLD_STATICINDEX(index));
    if ( pRetCog )
    {
        sithCogExec_PushInt(pCog, pRetCog->idx);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_FireProjectile(SithCog* pCog)
{
    rdVector3 fireOffset;
    SithPuppetSubMode submode;
    tSoundHandle hFireSnd;
    SithThing* pProjectileTemplate;
    SithThing* pShooter;
    SithThing* pProjectile;

    float autoAimFovZ = sithCogExec_PopFlex(pCog);
    float autoAimFovX = sithCogExec_PopFlex(pCog);
    SithFireProjectileFlags flags = sithCogExec_PopInt(pCog);
    float extra = sithCogExec_PopFlex(pCog);

    rdVector3 fireError;
    sithCogExec_PopVector(pCog, &fireError);
    sithCogExec_PopVector(pCog, &fireOffset);

    submode = sithCogExec_PopInt(pCog);
    hFireSnd = sithCogExec_PopSound(pCog);
    pProjectileTemplate = sithCogExec_PopTemplate(pCog);
    pShooter = sithCogExec_PopThing(pCog);

    if ( !pShooter )
    {
    }
    else
    {
        pProjectile = sithWeapon_FireProjectile(pShooter, pProjectileTemplate, hFireSnd, submode, &fireOffset, &fireError, extra, flags, autoAimFovX, autoAimFovZ);
        if ( pProjectile )
        {
            sithCogExec_PushInt(pCog, pProjectile->idx);
            return;
        }
    }

    sithCogExec_PushInt(pCog, -1);
}

void J3DAPI sithCogFunction_SendTrigger(SithCog* pCog)
{
    int param3 = sithCogExec_PopInt(pCog);
    int param2 = sithCogExec_PopInt(pCog);
    int param1 = sithCogExec_PopInt(pCog);
    int curparam0 = sithCogExec_PopInt(pCog);

    int srcIdx = sithCogExec_PopInt(pCog);
    SithThing* pCurThing = sithCogExec_PopThing(pCog);

    if ( !pCurThing )
    {
        sithDSSCog_SendMessage(NULL, SITHCOG_MSG_TRIGGER, SITHCOG_SYM_REF_THING, sithPlayer_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, srcIdx, 0, curparam0, param1, param2, param3, SITHMESSAGE_SENDTOJOINEDPLAYERS);
        sithCog_BroadcastMessageEx(SITHCOG_MSG_TRIGGER, SITHCOG_SYM_REF_THING, sithPlayer_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, srcIdx, curparam0, param1, param2, param3);
    }
    else if ( pCurThing->type != SITH_THING_PLAYER
        || !pCurThing->thingInfo.actorInfo.pPlayer
        || (pCurThing->thingInfo.actorInfo.pPlayer->flags & SITH_PLAYER_JOINEDGAME) == 0 )
    {
        return;
    }
    else
    {
        if ( pCurThing == sithPlayer_pLocalPlayerThing )
        {
            sithCog_BroadcastMessageEx(SITHCOG_MSG_TRIGGER, SITHCOG_SYM_REF_THING, sithPlayer_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, srcIdx, curparam0, param1, param2, param3);
        }
        else
        {
            sithDSSCog_SendMessage(NULL, SITHCOG_MSG_TRIGGER, SITHCOG_SYM_REF_THING, sithPlayer_pLocalPlayerThing->idx, SITHCOG_SYM_REF_NONE, srcIdx, 0, curparam0, param1, param2, param3, pCurThing->thingInfo.actorInfo.pPlayer->playerNetId);
        }
    }
}

void J3DAPI sithCogFunction_ActivateWeapon(SithCog* pCog)
{
    float timeToWait  = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && timeToWait >= 0.0f )
    {
        sithWeapon_ActivateWeapon(pThing, pCog, timeToWait);
    }
    else
    {
#line 3068
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad call to ActivateWeapon.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_DeactivateWeapon(SithCog* pCog)
{
    float secTime;
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        secTime = sithWeapon_DeactivateWeapon(pThing, pCog);
        sithCogExec_PushFlex(pCog, secTime);
    }
    else
    {
#line 3096
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad call to DeactivateWeapon.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, -1.0f);
    }
}

void J3DAPI sithCogFunction_DeactivateCurrentWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithWeapon_DeactivateCurrentWeapon(pThing);
    }
}

void J3DAPI sithCogFunction_SetFireWait(SithCog* pCog)
{
    float waitTime    = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && (pThing->type == SITH_THING_PLAYER || pThing->type == SITH_THING_ACTOR) && waitTime >= -1.0f )
    {
        sithWeapon_SetFireWait(pThing, waitTime);
    }
    else
    {
#line 3146
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad call to SetFireWait.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_SetMountWait(SithCog* pCog)
{
    float timeToWait  = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

#line 3169
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL(pThing->type == SITH_THING_PLAYER);
    SITH_ASSERTREL(timeToWait >= -((float)1.0));

    sithWeapon_SetMountWait(timeToWait);
}

void J3DAPI sithCogFunction_SetAimWait(SithCog* pCog)
{
    float timeToWait  = sithCogExec_PopFlex(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

#line 3184
    SITH_ASSERTREL(pThing != NULL);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    SITH_ASSERTREL(timeToWait >= -((float)1.0));

    sithWeapon_SetAimWait(pThing, timeToWait);
}

void J3DAPI sithCogFunction_SelectWeapon(SithCog* pCog)
{
    int weaponID      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
#line 3210
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithCogExec_PushInt(pCog, 1);
        return;
    }

    if ( !sithInventory_IsWeapon(weaponID) )
    {
#line 3221
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad weapon index in SelectWeapon().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( sithWeapon_SelectWeapon(pThing, (SithWeaponId)weaponID) )
    {
        sithCogExec_PushInt(pCog, 1);
        return;
    }

    sithCogExec_PushInt(pCog, 0);
}

void J3DAPI sithCogFunction_SelectWeaponWait(SithCog* pCog)
{
    int typeID        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 3259
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SelectWeaponWait().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !sithInventory_IsWeapon(typeID) )
    {
#line 3266
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad weapon index in SelectWeaponWait().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    // Return the current weapon ID
    sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);

    if ( !sithWeapon_SelectWeapon(pThing, (SithWeaponId)typeID) )
    {
        pCog->status = SITHCOG_STATUS_WAITING_ACTOR_WEAPON_SELECT_FINISH;
        pCog->statusParams[0] = pThing->idx;
        pCog->statusParams[1] = typeID;
    }
}

void J3DAPI sithCogFunction_DeselectWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing )
    {
        sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.weaponInfo.curWeaponID);
        sithWeapon_DeselectWeapon(pThing);
    }
    else
    {
#line 3306
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in DeselectWeapon.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_DeselectWeaponWait(SithCog* pCog)
{
    int weaponID;
    SithInventoryType* pCurType;

    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 3337
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in DeselectWeaponWait().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
        return;
    }

    weaponID = pThing->thingInfo.actorInfo.weaponInfo.curWeaponID;
    if ( weaponID == SITHWEAPON_GRENADE )
    {
        pCurType = &sithInventory_aTypes[weaponID];
        sithCog_SendMessage(pCurType->pCog, SITHCOG_MSG_BLOCKED, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_THING, pThing->idx, 0);
    }
    else if ( weaponID == SITHWEAPON_NO_WEAPON )
    {
        if ( sithWeapon_IsMountingWeapon(pThing) )
        {
            weaponID = sithWeapon_GetLastWeapon();
        }
        else
        {
            // No weapon to deselect
            sithCogExec_PushInt(pCog, 0);
            return;
        }
    }

    // Make the cog wait until the weapon is deselected
    pCog->status          = SITHCOG_STATUS_WAITING_ACTOR_WEAPON_DESELECT_FINISH;
    pCog->statusParams[0] = pThing->idx;
    pCog->statusParams[1] = weaponID;

    sithCogExec_PushInt(pCog, weaponID); // Return the weapon ID of the weapon being deselected
    sithWeapon_DeselectWeapon(pThing);   // Deslect the weapon
}

void J3DAPI sithCogFunction_SetCurWeapon(SithCog* pCog)
{
    int weaponID      = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( pThing && pThing->type == SITH_THING_PLAYER )
    {
        sithInventory_SetCurrentWeapon(pThing, (SithWeaponId)weaponID);
    }
    else
    {
#line 3400
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing called from SetCurWeapon.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_GetCurWeapon(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    int weaponId;
    if ( pThing && pThing->type == SITH_THING_PLAYER )
    {
        weaponId = sithInventory_GetCurrentWeapon(pThing);
        sithCogExec_PushInt(pCog, weaponId);
    }
    else
    {
#line 3430
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid thing called from GetCurWeapon.\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_GetCameraStateFlags(SithCog* pCog)
{
    int flags = sithCamera_GetCameraStateFlags();
    sithCogExec_PushInt(pCog, flags);
}

void J3DAPI sithCogFunction_SetCameraStateFlags(SithCog* pCog)
{
    int flags = sithCogExec_PopInt(pCog);
    sithCamera_SetCameraStateFlags((SithCameraState)flags);
}

void J3DAPI sithCogFunction_IsMulti(SithCog* pCog)
{
    if ( stdComm_IsGameActive() )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_IsServer(SithCog* pCog)
{
    if ( stdComm_IsGameHost() )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_AutoSavegame(SithCog* pCog)
{
    char aSavePath[128];


    stdUtil_Format(aSavePath, STD_ARRAYLEN(aSavePath), "%s%s", sithGetAutoSaveFilePrefix(), sithGetCurrentWorldSaveName());
    stdFnames_ChangeExt(aSavePath, "nds");
    sithGamesave_Save(aSavePath, 1);
}

void J3DAPI sithCogFunction_GetHintSolved(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( pThing && pThing->type == SITH_THING_HINT )
    {
        if ( ((int)pThing->userval & SITH_HINT_SOLVED) != 0 )
        {
            sithCogExec_PushInt(pCog, 1);
        }
        else
        {
            sithCogExec_PushInt(pCog, 0);
        }
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
#line 3574
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad call to GetHintSolved.\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_SetHintSolved(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 3598
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thingRef in SetHintSolved().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->type != SITH_THING_HINT )
    {
#line 3604
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not a hint in SetHintSolved().\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    sithOverlayMap_SetHintSolved(pThing);
}

void J3DAPI sithCogFunction_SetHintUnsolved(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
#line 3631
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thingRef in SetHintUnsolved().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( pThing->type != SITH_THING_HINT )
    {
#line 3637
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Thing %s is not a hint in SetHintUnsolved().\n", pCog->aName, pThing->aName, 0, 0);
        return;
    }

    sithOverlayMap_SetHintUnsolved(pThing);
}

void J3DAPI sithCogFunction_Rand(SithCog* pCog)
{
    float val = SITH_RANDF();
    sithCogExec_PushFlex(pCog, val);
}

void J3DAPI sithCogFunction_RandBetween(SithCog* pCog)
{
    int max = sithCogExec_PopInt(pCog);
    int min = sithCogExec_PopInt(pCog);
    int randVal;
    if ( min > max )
    {
#line 3707
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal params in RandBetween.\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        randVal = rand() % (max + 1 - min) + min;
        sithCogExec_PushInt(pCog, randVal);
    }
}

void J3DAPI sithCogFunction_RandVec(SithCog* pCog)
{
    rdVector3 vec;
    float randX, randY, randZ;
    randX = SITH_RANDF();
    randY = SITH_RANDF();
    randZ = SITH_RANDF();
    vec.x = randX;
    vec.y = randY;
    vec.z = randZ;
    sithCogExec_PushVector(pCog, &vec);
}

void J3DAPI sithCogFunction_Round(SithCog* pCog)
{
    float val  = sithCogExec_PopFlex(pCog);
    float rval = floorf(val);
    if ( val - rval < 0.5 )
    {
        sithCogExec_PushFlex(pCog, rval);
    }
    else
    {
        sithCogExec_PushFlex(pCog, ceilf(val));
    }
}

void J3DAPI sithCogFunction_Truncate(SithCog* pCog)
{
    float result;
    float val = sithCogExec_PopFlex(pCog);
    if ( val >= 0.0f )
    {
        result = floorf(val);
    }
    else
    {
        result = ceilf(val);
    }

    sithCogExec_PushFlex(pCog, result);
}

void J3DAPI sithCogFunction_Abs(SithCog* pCog)
{
    float val = sithCogExec_PopFlex(pCog);
    sithCogExec_PushFlex(pCog, val < 0.0f ? -val : val);
}

void J3DAPI sithCogFunction_Pow(SithCog* pCog)
{
    float curexp  = sithCogExec_PopFlex(pCog);
    float base = sithCogExec_PopFlex(pCog);

    if ( curexp == 0.0 && base == 0.0 )
    {
#line 3861
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: 0 to the 0 power is undefined\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushFlex(pCog, 0.0f);
        return;
    }

    sithCogExec_PushFlex(pCog, (float)pow(base, curexp));
}

void J3DAPI sithCogFunction_Sin(SithCog* pCog)
{
    float degrees = sithCogExec_PopFlex(pCog);

    float sin, cos;
    stdMath_SinCos(degrees, &sin, &cos);
    sithCogExec_PushFlex(pCog, sin);
}

void J3DAPI sithCogFunction_Cos(SithCog* pCog)
{
    float degrees = sithCogExec_PopFlex(pCog);

    float sin, cos;
    stdMath_SinCos(degrees, &sin, &cos);
    sithCogExec_PushFlex(pCog, cos);
}

void J3DAPI sithCogFunction_ArcTan(SithCog* pCog)
{
    float y = sithCogExec_PopFlex(pCog);
    float adjacent = sithCogExec_PopFlex(pCog);

    float degrees = stdMath_ArcTan4(adjacent, y);
    sithCogExec_PushFlex(pCog, degrees);
}

void J3DAPI sithCogFunction_SetWeaponModel(SithCog* pCog)
{
    int typeId        = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);
#line 3958
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 && pThing->type != SITH_THING_PLAYER )
    {
        return;
    }

    if ( !sithInventory_aTypes[typeId].pModel )
    {
#line 3969
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No model available in SetWeaponModel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithWeapon_SetWeaponModel(pThing, (SithWeaponId)typeId);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_ResetWeaponModel(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
#line 3994
    SITH_ASSERTREL(pThing);
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    sithWeapon_ResetWeaponModel(pThing);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_LoadHolsterModel(SithCog* pCog)
{
    rdModel3* pModel;
    int typeId;

    const char* pMdlFilename = sithCogExec_PopString(pCog);
    SithInventoryType* pCurType;
    typeId = sithCogExec_PopInt(pCog);
    pCurType = &sithInventory_aTypes[typeId];

    if ( pMdlFilename )
    {
        pModel = sithModel_GetModel(pMdlFilename);
    }

    if ( pModel && pCurType )
    {
        pCurType->pHolsterModel = pModel;
        sithCogExec_PushInt(pCog, pModel->num);
    }
    else
    {
#line 4035
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No model loaded in LoadHolsterModel().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_SetHolsterModel(SithCog* pCog)
{
    int meshNum       = sithCogExec_PopInt(pCog);
    size_t weaponId   = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 4062
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in SetHolsterModel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( weaponId < SITHWEAPON_FISTS || weaponId > SITHWEAPON_COMSHOTGUN )
    {
#line 4068
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad weaponID SetHolsterModel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( !sithInventory_aTypes[weaponId].pHolsterModel )
    {
#line 4074
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: No model available in SetHolsterModel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithWeapon_SetHolsterModel(pThing, (SithWeaponId)weaponId, meshNum);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_ResetHolsterModel(SithCog* pCog)
{
    size_t holsterNum = sithCogExec_PopInt(pCog);
    SithThing* pThing = sithCogExec_PopThing(pCog);

    if ( !pThing )
    {
#line 4103
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in ResetHolsterModel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    if ( holsterNum < SITHWEAPON_HOLSTWEWHIP || holsterNum > SITHWEAPON_HOLSTERBACK )
    {
#line 4109
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad holster number in ResetHolsterModel().\n", pCog->aName, 0, 0, 0);
        return;
    }

    sithWeapon_ResetHolsterModel(pThing, holsterNum);
    if ( (pCog->flags & SITHCOG_NOSYNC) == 0 && pCog->execMsgType != SITHCOG_MSG_STARTUP && pCog->execMsgType != SITHCOG_MSG_SHUTDOWN )
    {
        sithThing_SyncThing(pThing, SITHTHING_SYNC_STATE);
    }
}

void J3DAPI sithCogFunction_GetLastPistol(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 4136
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetLastPistol().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.pPlayer->inventory.lastPistolID);
    }
}

void J3DAPI sithCogFunction_GetLastRifle(SithCog* pCog)
{
    SithThing* pThing = sithCogExec_PopThing(pCog);
    if ( !pThing || pThing->type != SITH_THING_PLAYER )
    {
#line 4160
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in GetLastRifle().\n", pCog->aName, 0, 0, 0);
    }
    else
    {
        sithCogExec_PushInt(pCog, pThing->thingInfo.actorInfo.pPlayer->inventory.lastRifleID);
    }
}

void J3DAPI sithCogFunction_InEditor(SithCog* pCog)
{
    if ( (sith_mode.debugModeFlags & SITHDEBUG_INEDITOR) != 0 )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_CopyPlayerHolsters(SithCog* pCog)
{
    int meshNum;
    SithThingSwapEntry* pCurSwapList;

    SithThing* pDestThing   = sithCogExec_PopThing(pCog);
    SithThing* pSourceThing = sithCogExec_PopThing(pCog);

    if ( !pSourceThing || pSourceThing->type != SITH_THING_PLAYER || !pDestThing )
    {
#line 4210
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing in CopyPlayerHolsters().\n", pCog->aName, 0, 0, 0);
        return;
    }

#line 4214
    SITH_ASSERTREL(pSourceThing->renderData.data.pModel3 != NULL);
    SITH_ASSERTREL(pDestThing->renderData.data.pModel3 != NULL);

    sithThing_ResetSwapList(pDestThing);

    pCurSwapList = pSourceThing->pSwapList;
    while ( pCurSwapList )
    {
        meshNum = sithThing_GetThingMeshIndex(pDestThing, pSourceThing->renderData.data.pModel3->aGeos[0].aMeshes[pCurSwapList->meshNum].name);
        if ( meshNum != -1 )
        {
            sithThing_AddSwapEntry(pDestThing, meshNum, pCurSwapList->pSrcModel, pCurSwapList->srcMeshNum);
            pCurSwapList = pCurSwapList->pNextEntry;
        }
    }
}

void J3DAPI sithCogFunction_GetPerformanceLevel(SithCog* pCog)
{
    sithCogExec_PushInt(pCog, sithGetPerformanceLevel());
}

void J3DAPI sithCogFunction_IsLevelName(SithCog* pCog)
{
    const char* pName = sithCogExec_PopString(pCog);
    if ( !pName )
    {
#line 4248
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad string in IsLevelName().", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, 0);
        return;
    }

    if ( streqi(sithWorld_pCurrentWorld->aName, pName) )
    {
        sithCogExec_PushInt(pCog, 1);
    }
    else
    {
        sithCogExec_PushInt(pCog, 0);
    }
}

void J3DAPI sithCogFunction_SetFog(SithCog* pCog)
{
    rdVector3 fogColor;
    float end   = sithCogExec_PopFlex(pCog);
    float start = sithCogExec_PopFlex(pCog);
    int bGood    = sithCogExec_PopVector(pCog, &fogColor);
    int bEnable = sithCogExec_PopInt(pCog);

    if ( bGood )
    {
        sithWorld_pCurrentWorld->fog.bEnabled = bEnable;
        memcpy(&sithWorld_pCurrentWorld->fog.color, &fogColor, sizeof(fogColor));
        sithWorld_pCurrentWorld->fog.startDepth = start;
        sithWorld_pCurrentWorld->fog.endDepth   = end;
        sithWorld_pCurrentWorld->state |= SITH_WORLD_STATE_UPDATE_FOG;
    }
    else
    {
#line 4282
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad call to SetFog().\n", pCog->aName, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_FindNewSector(SithCog* pCog)
{
    SithSector* pSector;
    rdVector3 startPos;
    SithSector* pFoundSect;

    rdVector3 endPosition;
    sithCogExec_PopVector(pCog, &endPosition);

    pSector = sithCogExec_PopSector(pCog);
    sithCogExec_PopVector(pCog, &startPos);

    pFoundSect = sithCollision_FindSectorInRadius(pSector, &startPos, &endPosition, 0.0f);
    if ( pFoundSect )
    {
        sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pFoundSect));
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithCogFunction_FindNewSectorFromThing(SithCog* pCog)
{
    SithThing* pThing;
    SithSector* pCurFoundSector;

    rdVector3 endPos;
    sithCogExec_PopVector(pCog, &endPos);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !pThing->pInSector )
    {
#line 4320
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "BAD THING REF IN FindNewSectorFromThing\n", 0, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1);
    }
    else
    {
        pCurFoundSector = sithCollision_FindSectorInRadius(pThing->pInSector, &pThing->pos, &endPos, 0.0f);
        if ( pCurFoundSector )
        {
            sithCogExec_PushInt(pCog, sithSector_GetSectorIndex(pCurFoundSector));
        }
        else
        {
            sithCogExec_PushInt(pCog, -1);
        }
    }
}

#ifdef J3D_DEBUG
/* The five debug-only cog verbs: only the Indy debug build registers "debugprint", "debugflex", "debugint",
   "debuglocalsymbols" and "debugwaitforkey" (the last five registrations of its RegisterFunctions) and implements them
   in this order after FindNewSectorFromThing (debug 0x00498BD9, 0x00498C21, 0x00498C90, 0x00498CFB, 0x0049902D; lines
   4355..4481); retail has none of them. Names are the verb names in this module's convention (evidenced); bodies from
   the debug code. */
void J3DAPI sithCogFunction_DebugPrint(SithCog* pCog)
{
    const char* pStr;

    pStr = sithCogExec_PopString(pCog);
    if ( pStr )
    {
#line 4355
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "%s\n", pStr, 0, 0, 0);
    }
}

void J3DAPI sithCogFunction_DebugFlex(SithCog* pCog)
{
    float flex;
    const char* pStr;

    pStr = sithCogExec_PopString(pCog);
    flex = sithCogExec_PopFlex(pCog);
    if ( pStr )
    {
#line 4371
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "%s (%f)\n", pStr, flex, 0, 0);
    }

    sithCogExec_PushFlex(pCog, flex);
}

void J3DAPI sithCogFunction_DebugInt(SithCog* pCog)
{
    int val;
    const char* pStr;

    pStr = sithCogExec_PopString(pCog);
    val  = sithCogExec_PopInt(pCog);
    if ( pStr )
    {
#line 4390
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "%s (%d)\n", pStr, val, 0, 0);
    }

    sithCogExec_PushInt(pCog, val);
}

void J3DAPI sithCogFunction_DebugLocalSymbols(SithCog* pCog)
{
    int bSkip;
    const char* pStr;
    size_t i;
    size_t j;
    SithCogSymbolRef* pRef;
    SithCogSymbol* pSymbol;

    pStr = sithCogExec_PopString(pCog);
    sprintf(std_genBuffer, "%s.\n", pStr);
#line 4410
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, std_genBuffer, 0, 0, 0, 0);

    if ( pCog->pScript && pCog->pSymbolTable )
    {
        for ( i = 0, pSymbol = pCog->pSymbolTable->aSymbols; i < pCog->pSymbolTable->numUsedSymbols; ++i, ++pSymbol )
        {
            bSkip = 0;
            if ( i < pCog->pScript->numHandlers ) {
                bSkip = 1;
            }

            if ( !bSkip )
            {
                for ( j = 0, pRef = pCog->pScript->aSymRefs; j < pCog->pScript->numSymbolRefs; ++j, ++pRef )
                {
                    if ( pRef->symbolId == i )
                    {
                        if ( (pRef->bLocal & 1) == 0 ) {
                            bSkip = 1;
                        }

                        break;
                    }
                }
            }

            if ( bSkip ) {
                continue;
            }

            if ( pSymbol->value.type == SITHCOG_VALUE_FLOAT )
            {
                sprintf(std_genBuffer, " Symbol %d: '%s' ", pSymbol->id, pSymbol->pName ? pSymbol->pName : "<null>");
                sprintf(std_genBuffer + strlen(std_genBuffer), " = %f\n", pSymbol->value.val.floatValue);
#line 4447
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, std_genBuffer, 0, 0, 0, 0);
            }
            else if ( pSymbol->value.type == SITHCOG_VALUE_VECTOR )
            {
                sprintf(std_genBuffer, " Symbol %d: '%s' ", pSymbol->id, pSymbol->pName ? pSymbol->pName : "<null>");
                sprintf(std_genBuffer + strlen(std_genBuffer), " = %f %f %f\n", pSymbol->value.val.vecValue.x, pSymbol->value.val.vecValue.y, pSymbol->value.val.vecValue.z);
#line 4453
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, std_genBuffer, 0, 0, 0, 0);
            }
            else if ( pSymbol->value.type == SITHCOG_VALUE_INT )
            {
                sprintf(std_genBuffer, " Symbol %d: '%s' ", pSymbol->id, pSymbol->pName ? pSymbol->pName : "<null>");
                sprintf(std_genBuffer + strlen(std_genBuffer), " = %d\n", pSymbol->value.val.intValue);
#line 4459
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, std_genBuffer, 0, 0, 0, 0);
            }
        }
    }

    sithCogExec_PushInt(pCog, 1);
}

void J3DAPI sithCogFunction_DebugWaitForKey(SithCog* pCog)
{
    int key;

    key = 0;
#line 4472
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Waiting for keypress: ", 0, 0, 0, 0);
    while ( !key )
    {
        sithControl_ReadControls();
        key = sithControl_GetKey(0x27, NULL);
        sithControl_FinishRead();
    }

#line 4481
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "\n", 0, 0, 0, 0);
}
#endif
