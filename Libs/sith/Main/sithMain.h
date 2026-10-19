#ifndef SITH_SITHMAIN_H
#define SITH_SITHMAIN_H
#include <j3dcore/j3d.h>

#include <sith/types.h>
#include <sith/Main/sithMain.h>

#include <std/types.h>
#include <std/General/std.h>

J3D_EXTERN_C_START

// Retail uses only the float scale 1.0f / RAND_MAX: every user object has a float 1/32767 literal in its VC5 .rdata block, none a double
// The cast operand is parenthesised like every other macro operand, and the scaled product is converted to float as a whole (retail schedules the rand() result store before a following pointer load: sithAIInstinct Dodge 0x491DB8, HoverDrift 0x4901B8)
#define SITH_RAND() ((float)(((float)rand()) * (1.0f / RAND_MAX)))
#define SITH_RANDF() ((float)SITH_RAND())



#define SITHLOG_FATAL(message) \
    J3DLOG_FATAL(sith_pHS, message)

#define SITH_ASSERT(condition) \
    J3D_ASSERT(condition, sith_pHS )

#define SITH_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, sith_pHS )

/**
 * Checks if current game frame number + offset is N-th frame (like every 4th, 8th, or 16th frame).
 * Uses the global frame counter sithMain_frameNumber with an optional offset.
 *
 * @param offset Shifts current game frame for which frame is considered the "N-th" one
 * @param n      N-th frame, i.e.: on every n-th frame (e.g.: every 2nd, 4th, 8th, 16th frame, etc.)
 * @return       True on every N-th frame
 */
#define SITH_ISFRAMECYCLE(offset, n) ((((offset) + sithMain_frameNumber) & ((n)-1)) == 0)


extern tHostServices* sith_pHS;

extern int sith_bDrawPlayerMoveBounds;

// TODO: Rename all to be prefixed only by 'sith'

extern size_t sithMain_frameNumber;

extern SithMode sith_mode;

extern size_t sithMain_curRenderTick;

void J3DAPI sithSetServices(tHostServices* pHS);
void J3DAPI sithClearServices(void);

int J3DAPI sithStartup(void);
void J3DAPI sithShutdown(void);

void J3DAPI sithSetPerformanceLevel(size_t level);
int J3DAPI sithGetPerformanceLevel(void);

int J3DAPI sithOpenStatic(const char* pFilename);
void J3DAPI sithCloseStatic(void);

int J3DAPI sithOpenNormal(const char* pWorldName, const wchar_t* wszPlayerName);
void J3DAPI sithOpenPostProcess(void);
int J3DAPI sithOpenMulti(const char* pFilename, const wchar_t* pwPlayerName);

int J3DAPI sithOpen(const wchar_t* wszPlayerName);
void J3DAPI sithClose(void);

void J3DAPI sithUpdate(void);
void J3DAPI sithDrawScene(void);
void J3DAPI sithAdvanceRenderTick(void);

void J3DAPI sithSetGameDifficulty(int difficulty);
int J3DAPI sithGetGameDifficulty(void);

float J3DAPI sithGetHitAccuarancyScalar(void);
float J3DAPI sithGetCombatDamageScalar(void);
float J3DAPI sithGetIMPDamageScalar(void);

void J3DAPI sithMakeDirs(void);

const char* J3DAPI sithGetAutoSaveFilePrefix(void);
const char* J3DAPI sithGetSaveGamesDir(void);
const char* J3DAPI sithGetQuickSaveFilePrefix(void);
const char* J3DAPI sithGetScreenShotsDir(void);
const char* J3DAPI sithGetScreenShotFileTemplate(void);
void J3DAPI sithSetOpenCallback(SithOpenCallback pfCallback);
const char* J3DAPI sithGetCurrentWorldSaveName(void);
const char* J3DAPI sithGetLevelSaveFilename(int levelNum);

J3D_EXTERN_C_END
#endif // SITH_SITHMAIN_H
