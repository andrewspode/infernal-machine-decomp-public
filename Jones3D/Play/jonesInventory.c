#include "jonesInventory.h"

#include <sith/Cog/sithCog.h>
#include <sith/Dss/sithGamesave.h>
#include <sith/Gameplay/sithInventory.h>
#include <sith/Gameplay/sithOverlayMap.h>
#include <sith/Gameplay/sithPlayerActions.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithWeapon.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdMath.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(jonesInventory_countfit_1, 15)
CF_PROTO(jonesInventory_countfit_2, 15)
CF_PROTO(jonesInventory_countfit_3, 15)
CF_PROTO(jonesInventory_countfit_4, 15)
CF_PROTO(jonesInventory_countfit_5, 15)
CF_PROTO(jonesInventory_countfit_6, 15)
CF_PROTO(jonesInventory_countfit_7, 15)
CF_PROTO(jonesInventory_countfit_8, 15)
CF_PROTO(jonesInventory_countfit_9, 15)
CF_PROTO(jonesInventory_countfit_10, 15)
CF_PROTO(jonesInventory_countfit_11, 15)
CF_PROTO(jonesInventory_countfit_12, 0)
/* end count-fit */

static SithGameStatistics* jonesInventory_pGameStatistics;
static uint32_t jonesInventory_msecStatisticsGameTime;

// Retail compiled the item functions below without optimization (EBP frames,
// stack homes for every temporary and 0xCC alignment fill), unlike the
// statistics functions that follow.
#pragma optimize("", off)

int J3DAPI jonesInventory_Open(void)
{
    sprintf(std_genBuffer, "misc\\%s", "items.dat");
    return jonesInventory_Load(std_genBuffer);
}

int J3DAPI jonesInventory_Load(const char* pFilename)
{
    SithCog* pCog;
    rdModel3* pInventoryModel;
    rdModel3* pItemModel;
    SithInventoryTypeFlag itemFlags;
    unsigned int typeId;
    float min;
    size_t i;
    float max;

    // J3D_ASSERTREL is MSVC <assert.h>'s `||` form: retail continues with
    // `edx` after the assert call, which neither `if (!cond)` nor the old
    // ternary macro (a value temp this /Od block does not have) produces.
    //
#line 61
    STD_ASSERTREL(pFilename);

    if ( !stdConffile_Open(pFilename) )
    {
        return 1;
    }

    // `for (;;)`, not `while (1)`: /Od materialises a `while` condition
    // (`mov reg,1; test reg,reg; je`), which retail does not have, and that
    // temporary also shifts the scratch-register rotation for the rest of the
    // function. Retail's layout (/Od keeps source order): the ReadArgs test is
    // one `je` to the close, the "end" test is `jne; jmp close` (an empty
    // then-arm jumping over its else), both parse errors `jmp` past the
    // close's `return 0` to the print (a `break` of this loop), and each
    // registered item ends in a `jmp` back to the ReadArgs call (`continue`).
    for ( ;; )
    {
        if ( stdConffile_ReadArgs() )
        {
            itemFlags       = 0;
            pCog            = NULL;
            pItemModel      = NULL;
            pInventoryModel = NULL;
            if ( !strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
            {
                // "end": leave through the close below.
            }
            else
            {
                if ( stdConffile_entry.numArgs < 4 )
                {
                    break;
                }

                typeId = atoi(stdConffile_entry.aArgs[1].argValue);
                if ( typeId < 0 || typeId >= 200 )
                {
                    break;
                }

                min = D3DVAL(atof(stdConffile_entry.aArgs[2].argValue));
                max = D3DVAL(atof(stdConffile_entry.aArgs[3].argValue));
                sscanf(stdConffile_entry.aArgs[4].argValue, "%x", &itemFlags);

                for ( i = 5; i < stdConffile_entry.numArgs; ++i )
                {
                    if ( !strcmp(stdConffile_entry.aArgs[i].argName, "cog") )
                    {
                        pCog = sithCog_GetCog(stdConffile_entry.aArgs[i].argValue);
                    }
                    else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "invmodel") )
                    {
                        pInventoryModel = sithModel_GetModel(stdConffile_entry.aArgs[i].argValue);
                    }
                    else if ( !strcmp(stdConffile_entry.aArgs[i].argName, "model") )
                    {
                        pItemModel = sithModel_GetModel(stdConffile_entry.aArgs[i].argValue);
                    }
                }

                sithInventory_RegisterType(typeId, stdConffile_entry.aArgs[0].argValue, min, max, itemFlags, pCog, pInventoryModel, pItemModel);
                continue;
            }
        }

        stdConffile_Close();
        return 0;
    }

#line 116
    stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Parse error reading item definition file %s.\n", pFilename, 0, 0, 0);
    return 1;
}

void J3DAPI jonesInventory_Close(void)
{
}

int J3DAPI jonesInventory_ActivateItem(SithThing* pThing, int typeId)
{
    if ( sithPlayerActions_Activate(pThing) )
    {
        return 1;
    }

    return 0;
}

int J3DAPI jonesInventory_UseItem(SithThing* pPlayer, unsigned int typeId)
{
    SithInventoryType* pItem;
    int currentItem;

    currentItem = sithInventory_GetCurrentItem(pPlayer);
    if ( sithInventory_GetInventory(pPlayer, typeId) != 0.0 && sithInventory_IsInventoryAvailable(pPlayer, typeId) )
    {
        pItem = sithInventory_GetType(typeId);
        if ( pItem->pCog )
        {
            sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_USER0, SITHCOG_SYM_REF_INT, typeId, SITHCOG_SYM_REF_THING, pPlayer->idx, 0);
        }

        return 1;
    }

    return 0;
}

// Discarded by the linker (no caller in either build); restored from the debug build (0x420160, between UseItem and
// ResetGameStatistics). Name inferred.
int J3DAPI jonesInventory_DeactivateItem(SithThing* pThing, int typeId)
{
    int currentItem;
    SithInventoryType* pItem;

    currentItem = sithInventory_GetCurrentItem(pThing);
    if ( typeId != currentItem && sithInventory_GetInventory(pThing, typeId) != 0.0 && sithInventory_IsInventoryAvailable(pThing, typeId) )
    {
        pItem = sithInventory_GetType(typeId);
        if ( pItem->pCog )
        {
            sithCog_SendMessage(pItem->pCog, SITHCOG_MSG_DEACTIVATED, SITHCOG_SYM_REF_INT, typeId, SITHCOG_SYM_REF_THING, pThing->idx, 0);
        }

        return 1;
    }

    return 0;
}

#pragma optimize("", on)

int J3DAPI jonesInventory_ResetGameStatistics(void)
{
    jonesInventory_msecStatisticsGameTime = 0;

    jonesInventory_pGameStatistics = sithGamesave_GetGameStatistics();
    if ( jonesInventory_pGameStatistics && sithGamesave_LockGameStatistics() )
    {
        memset(jonesInventory_pGameStatistics, 0, sizeof(SithGameStatistics));

        jonesInventory_pGameStatistics->aLevelStatistic[0].levelStartIQPoints = 150;
        jonesInventory_pGameStatistics->totalIQPoints = 150;
        jonesInventory_pGameStatistics->curLevelNum   = 0;

        sithGamesave_UnlockGameStatistics();
        sithWeapon_SetActorKilledCallback(jonesInventory_UpdateIQPoints);
        return 0;
    }

    return 1;
}

// JonesHud_Shutdown calls it (debug 0x420280, after ResetGameStatistics); retail folded the empty body (0x004EE700).
// Name inferred.
void J3DAPI jonesInventory_Shutdown(void)
{
}

size_t J3DAPI jonesInventory_GetMaxDifficultyLevel(void)
{
    return 5;
}

void J3DAPI jonesInventory_UpdateSolvedHintsStatistics(void)
{
    int curlevelNum;
    int numSeen;
    int numSeenHintsLvl;
    int numHints;

    if ( sithGamesave_LockGameStatistics() )
    {
        curlevelNum = jonesInventory_pGameStatistics->curLevelNum;
        sithOverlayMap_GetNumSeenHints(&numSeen, &numHints);
        if ( jonesInventory_pGameStatistics->aLevelStatistic[curlevelNum].numSeenHints == numSeen )
        {
            sithGamesave_UnlockGameStatistics();
            return;
        }

        numSeenHintsLvl = jonesInventory_pGameStatistics->aLevelStatistic[curlevelNum].numSeenHints;
        jonesInventory_pGameStatistics->aLevelStatistic[curlevelNum].numSeenHints = numSeen;
        jonesInventory_pGameStatistics->totalLevelStats.numSeenHints += numSeen - numSeenHintsLvl;
        sithGamesave_UnlockGameStatistics();
    }
}

void J3DAPI jonesInventory_UpdateIQPoints(size_t difficulty)
{
    int penalty;

    if ( sithGamesave_LockGameStatistics() )
    {
        if ( jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates < 20 )
        {
            difficulty = STDMATH_CLAMP(difficulty, 0, 5);
            penalty    = 5 - difficulty;
            if ( jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates + penalty > 20 )
            {
                penalty = 20 - jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates;
                jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates = 20;
            }
            else
            {
                jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numIQUpdates += penalty;
            }

            jonesInventory_pGameStatistics->totalIQPoints -= penalty;
            jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].difficultyPenalty += penalty;
            jonesInventory_pGameStatistics->totalLevelStats.difficultyPenalty += penalty;
        }

        if ( jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].difficultyPenalty < 0 )
        {
            jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].difficultyPenalty = 0;
        }

        sithGamesave_UnlockGameStatistics();
    }
}

int J3DAPI jonesInventory_GetTotalIQPoints(void)
{
    int numFoundTreasures;
    int retVal;
    int numHints;
    int numSeen;

    sithOverlayMap_GetNumSeenHints(&numSeen, &numHints);
    numFoundTreasures = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numFoundTreasures;
    if ( numHints > 0 )
    {
        int hintPenalty;

        hintPenalty = numSeen * 10 / numHints;
        if ( !hintPenalty && numSeen > 0 )
        {
            hintPenalty = 1;
        }

        retVal = jonesInventory_pGameStatistics->totalIQPoints - hintPenalty + numFoundTreasures * 10 / 10;
    }
    else
    {
        retVal = jonesInventory_pGameStatistics->totalIQPoints + numFoundTreasures * 10 / 10;
    }

    return retVal;
}

void J3DAPI jonesInventory_AdvanceStatistics(void)
{
    int numSeenHintsLvl;
    int numFoundTreasures;
    int curhintPoints;
    int nSeenHints;
    int numTotalHints;

    jonesInventory_UpdateGameTimeStatistics(sithTime_msecGameTime);
    if ( sithGamesave_LockGameStatistics() )
    {
        sithOverlayMap_GetNumSeenHints(&nSeenHints, &numTotalHints);

        numSeenHintsLvl   = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numSeenHints;
        numFoundTreasures = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numFoundTreasures;

        // Note, the max treasure items to be found in level is capped to 10
        if ( numTotalHints > 0 )
        {
            curhintPoints = nSeenHints * 10 / numTotalHints;
            if ( !curhintPoints && nSeenHints > 0 )
            {
                curhintPoints = 1;
            }

            jonesInventory_pGameStatistics->totalIQPoints += 40 - curhintPoints + numFoundTreasures * 10 / 10;
        }
        else
        {
            jonesInventory_pGameStatistics->totalIQPoints += 40 + numFoundTreasures * 10 / 10;
        }

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].iqPoints = jonesInventory_pGameStatistics->totalIQPoints
            - jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].levelStartIQPoints;

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum + 1].levelStartIQPoints = jonesInventory_pGameStatistics->totalIQPoints;

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numSeenHints = nSeenHints;
        jonesInventory_pGameStatistics->totalLevelStats.numSeenHints += nSeenHints - numSeenHintsLvl;

        jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum + 1].curElapsedSec = jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].curElapsedSec;

        ++jonesInventory_pGameStatistics->curLevelNum;
        sithGamesave_UnlockGameStatistics();
    }

    jonesInventory_msecStatisticsGameTime = 0;
}

void J3DAPI jonesInventory_AdvanceFoundTreasuresStatistics(void)
{
    if ( sithGamesave_LockGameStatistics() )
    {
        ++jonesInventory_pGameStatistics->aLevelStatistic[jonesInventory_pGameStatistics->curLevelNum].numFoundTreasures;
        ++jonesInventory_pGameStatistics->totalLevelStats.numFoundTreasures;
        sithGamesave_UnlockGameStatistics();
    }
}

void J3DAPI jonesInventory_UpdateGameTimeStatistics(uint32_t msecTime)
{
    int levelIdx;
    uint32_t curElapsedSec;

    if ( sithGamesave_LockGameStatistics() )
    {
        levelIdx      = jonesInventory_pGameStatistics->curLevelNum;
        curElapsedSec = jonesInventory_pGameStatistics->aLevelStatistic[levelIdx].curElapsedSec;
        jonesInventory_UpdateLevelStatisticTime(curElapsedSec, &jonesInventory_pGameStatistics->aLevelStatistic[levelIdx], msecTime, jonesInventory_msecStatisticsGameTime);

        curElapsedSec = jonesInventory_pGameStatistics->totalLevelStats.curElapsedSec;
        jonesInventory_UpdateLevelStatisticTime(curElapsedSec, &jonesInventory_pGameStatistics->totalLevelStats, msecTime, jonesInventory_msecStatisticsGameTime);

        jonesInventory_msecStatisticsGameTime = msecTime;
        sithGamesave_UnlockGameStatistics();
    }
}

void J3DAPI jonesInventory_UpdateLevelStatisticTime(uint32_t elapsedSec, SithLevelStatistic* pLevelStats, uint32_t msecNewTime, uint32_t msecPreviousTime)
{
    uint32_t msecElapsed;
    int hours;
    int totalHours;
    int curprevHours;
    int prevMinutes;
    int minutes;
    int currentelapsedTime;
    int totalMinutes;

    if ( msecNewTime < jonesInventory_msecStatisticsGameTime )
    {
        // The game clock wrapped around since the previous update
        msecElapsed = elapsedSec * 1000 + msecNewTime + (0xFFFFFFFF - msecPreviousTime);
    }
    else
    {
        msecElapsed = elapsedSec * 1000 + (msecNewTime - msecPreviousTime);
    }

    elapsedSec = msecElapsed / 1000 % 60;
    minutes    = msecElapsed / 60000 % 60;
    hours      = msecElapsed / 3600000;

    // elapsedTime packs whole hours above the low minutes byte.
    currentelapsedTime  = pLevelStats->elapsedTime;
    curprevHours    = currentelapsedTime >> 8;
    prevMinutes  = (curprevHours << 8) ^ currentelapsedTime;
    totalHours   = hours + curprevHours + (minutes + prevMinutes) / 60;
    totalMinutes = (minutes + prevMinutes) % 60;

    pLevelStats->elapsedTime   = (totalHours << 8) | totalMinutes;
    pLevelStats->curElapsedSec = elapsedSec;
}

void J3DAPI jonesInventory_ResetStatisticsGameTime(void)
{
    jonesInventory_msecStatisticsGameTime = sithTime_msecGameTime;
}

