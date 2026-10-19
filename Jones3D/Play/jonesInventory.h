#ifndef JONES3D_JONESINVENTORY_H
#define JONES3D_JONESINVENTORY_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>

J3D_EXTERN_C_START

int J3DAPI jonesInventory_Open(void);
int J3DAPI jonesInventory_Load(const char* pFilename);
void J3DAPI jonesInventory_Close(void);
int J3DAPI jonesInventory_ActivateItem(SithThing* pThing, int typeId);
int J3DAPI jonesInventory_UseItem(SithThing* pPlayer, unsigned int typeId);
int J3DAPI jonesInventory_ResetGameStatistics(void);
void J3DAPI jonesInventory_Shutdown(void);
size_t J3DAPI jonesInventory_GetMaxDifficultyLevel(void);
void J3DAPI jonesInventory_UpdateSolvedHintsStatistics(void);
void J3DAPI jonesInventory_UpdateIQPoints(size_t difficulty);
int J3DAPI jonesInventory_GetTotalIQPoints(void);
void J3DAPI jonesInventory_AdvanceStatistics(void);
void J3DAPI jonesInventory_AdvanceFoundTreasuresStatistics(void);
void J3DAPI jonesInventory_UpdateGameTimeStatistics(uint32_t msecTime);
void J3DAPI jonesInventory_UpdateLevelStatisticTime(uint32_t elapsedSec, SithLevelStatistic* pLevelStats, uint32_t msecNewTime, uint32_t msecPreviousTime);
void J3DAPI jonesInventory_ResetStatisticsGameTime(void);

J3D_EXTERN_C_END
#endif // JONES3D_JONESINVENTORY_H
