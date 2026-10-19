#include "sithCog.h"

#include <sith/AI/sithAIClass.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Cog/sithCogFunction.h>
#include <sith/Cog/sithCogFunctionAI.h>
#include <sith/Cog/sithCogFunctionPlayer.h>
#include <sith/Cog/sithCogFunctionSector.h>
#include <sith/Cog/sithCogFunctionSound.h>
#include <sith/Cog/sithCogFunctionSurface.h>
#include <sith/Cog/sithCogFunctionThing.h>
#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithConsole.h>
#include <sith/Devices/sithSound.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSSCog.h>
#include <sith/Dss/sithMulti.h>
#include <sith/Engine/sithAnimate.h>
#include <sith/Engine/sithPuppet.h>
#include <sith/Gameplay/sithEvent.h>
#include <sith/Gameplay/sithPlayer.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/World/sithMaterial.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithSector.h>
#include <sith/World/sithSurface.h>
#include <sith/World/sithTemplate.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithVoice.h>
#include <sith/World/sithWeapon.h>
#include <sith/World/sithWorld.h>

#include <sound/Sound.h>

#include <std/General/stdConffile.h>
#include <std/General/stdHashtbl.h>
#include <std/General/stdMath.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdComm.h>

#include <stdio.h>
#include <stdlib.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(sithCog_countfit_1, 15)
CF_PROTO(sithCog_countfit_2, 15)
CF_PROTO(sithCog_countfit_3, 15)
CF_PROTO(sithCog_countfit_4, 15)
CF_PROTO(sithCog_countfit_5, 15)
CF_PROTO(sithCog_countfit_6, 15)
CF_PROTO(sithCog_countfit_7, 15)
CF_PROTO(sithCog_countfit_8, 15)
CF_PROTO(sithCog_countfit_9, 15)
CF_PROTO(sithCog_countfit_10, 15)
CF_PROTO(sithCog_countfit_11, 15)
CF_PROTO(sithCog_countfit_12, 15)
CF_PROTO(sithCog_countfit_13, 21)
/* end count-fit */

#define SITHCOG_MAXGLOBALSYMBOLS 1024u
#define SITHCOG_MAXSCRIPTS       256u
#define SITHCOG_SYMVALUESTRLEN   64

static SithCogSectorLink aCogLinkSectors[512] = { 0 };
static SithCogThingLink aThingLinks[1024] = { 0 };
static SithCogSurfaceLink aCogLinkSurfaces[512] = { 0 };
int sithCog_bCogStartup = 0;
static int bCogOpen = 0;
static size_t numThingLinks = 0;
static size_t numSurfLinks = 0;
static size_t numSectorLinks = 0;
tHashTable* sithCog_pHashtable = NULL;

SithCogSymbolTable* sithCog_pSymbolTable;
SithCog* sithCog_pMasterCog;

int J3DAPI sithCog_LinkCog(const SithWorld* pWorld, SithCog* pCog, const SithCogSymbolRef* pRef, const SithCogSymbol* pSymbol);

SithCogScript* J3DAPI sithCog_GetScript(const char* pName);
void J3DAPI sithCog_GetSymbolRefInitializer(const SithWorld* pWorld, const SithCog* pCog, int symIdx, char* pOutString); // debug build: WriteCogsListText calls it before its definition
int J3DAPI sithCog_AddScript(SithCogScript* pScript);
int J3DAPI sithCog_RemoveScript(SithCogScript* pScript);

int J3DAPI sithCog_TimerEventTask(int msecTime, SithEventParams* pParams);
void J3DAPI sithCog_Initialize(void);

int J3DAPI sithCog_Startup(void)
{
    if ( sithCog_bCogStartup )
    {
        return 1;
    }

    sithCog_pSymbolTable = sithCogParse_AllocSymbolTable(SITHCOG_MAXGLOBALSYMBOLS);
    if ( !sithCog_pSymbolTable )
    {
#line 152
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not allocate COG symboltable\n", 0, 0, 0, 0);
        return 1;
    }

    sithCog_pHashtable = stdHashtbl_New(SITHCOG_MAXSCRIPTS);
    if ( !sithCog_pHashtable )
    {
#line 158
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not allocate COG hashtable.\n", 0, 0, 0, 0);
        return 1;
    }

    sithCog_pSymbolTable->firstId = SITHCOG_GLOBALSYMBOLSTARTID;

    sithCog_Initialize();
    sithEvent_RegisterTask(SITHCOG_TASKID, sithCog_TimerEventTask, 0, SITHEVENT_TASKONDEMAND);
    sithCog_bCogStartup = 1;
    return 0;
}

void J3DAPI sithCog_Shutdown(void)
{
    if ( sithCog_pSymbolTable )
    {
        sithCogParse_FreeSymbolTable(sithCog_pSymbolTable);
        sithCog_pSymbolTable = NULL;
    }

    if ( sithCog_pHashtable )
    {
        stdHashtbl_Free(sithCog_pHashtable);
        sithCog_pHashtable = NULL;
    }

    sithCogParse_FreeParseTree();
    sithCog_bCogStartup = 0;
}

int J3DAPI sithCog_Open(SithWorld* pWorld)
{
    size_t cogNum;
    size_t refNumber;
    SithCog* pCog;
    SithCogSymbolRef* pRef;
    SithCogSymbol* pSymbol;
    int symbolId;
    size_t cogRefNum;

#line 235
    SITH_ASSERTREL(pWorld);
    SITH_ASSERTREL(sithCog_bCogStartup == 1);

    if ( bCogOpen )
    {
#line 240
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already open!\n", 0, 0, 0, 0);
        return 1;
    }

#line 245
    stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Global symbol space %d, used %d.\n", SITHCOG_MAXGLOBALSYMBOLS, sithCog_pSymbolTable->numUsedSymbols, 0, 0);

    // The debug code's static-world guard is `jne; jmp` past the static-cog loop: an empty
    // then-arm (a single site an empty arm explains).
    if ( !sithWorld_pStaticWorld )
    {
    }
    else
    {
        for ( cogNum = 0, pCog = sithWorld_pStaticWorld->aCogs; cogNum < sithWorld_pStaticWorld->numCogs; ++cogNum, ++pCog )
        {
            for ( refNumber = 0, pRef = pCog->pScript->aSymRefs; refNumber < pCog->pScript->numSymbolRefs; ++refNumber, ++pRef )
            {
                symbolId = pRef->symbolId;
#line 261
                SITH_ASSERTREL(symbolId >= 0);

                pSymbol = &pCog->pSymbolTable->aSymbols[symbolId];
                if ( strlen(pRef->aValue) && sithCog_ParseSymbolRef(sithWorld_pStaticWorld, pSymbol, pRef, pRef->aValue) )
                {
#line 270
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s, Invalid Reference.\n", pCog->aName, pRef->aValue, 0, 0);
                }

            }

            sithCog_SendMessage(pCog, SITHCOG_MSG_LOADING, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
        }
    }

    for ( cogNum = 0, pCog = pWorld->aCogs; cogNum < pWorld->numCogs; ++cogNum, ++pCog )
    {
        cogRefNum = 0;
        for ( refNumber = 0, pRef = pCog->pScript->aSymRefs; refNumber < pCog->pScript->numSymbolRefs; ++refNumber, ++pRef )
        {
            symbolId = pRef->symbolId;
#line 292
            SITH_ASSERTREL(symbolId >= 0);

            pSymbol = &pCog->pSymbolTable->aSymbols[symbolId];
            if ( (pRef->bLocal & 1) == 0 )
            {
                if ( strlen(pCog->aSymRefValues[cogRefNum]) )
                {
                    if ( sithCog_ParseSymbolRef(pWorld, pSymbol, pRef, pCog->aSymRefValues[cogRefNum]) )
                    {
#line 303
                        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid Reference  %s.\n", pCog->aName, pRef->aValue, 0, 0);
                    }
                }
                else if ( strlen(pRef->aValue) && sithCog_ParseSymbolRef(pWorld, pSymbol, pRef, pRef->aValue) )
                {
#line 311
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid Reference  %s.\n", pCog->aName, pRef->aValue, 0, 0);
                }

                ++cogRefNum;
                sithCog_LinkCog(pWorld, pCog, pRef, pSymbol);
            }
            else if ( strlen(pRef->aValue) && sithCog_ParseSymbolRef(pWorld, pSymbol, pRef, pRef->aValue) )
            {
#line 328
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Invalid Reference %s.\n", pCog->aName, pRef->aValue, 0, 0);
            }

        }

        sithCog_SendMessage(pCog, SITHCOG_MSG_LOADING, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    bCogOpen = 1;
    return 0;
}

void J3DAPI sithCog_Close(void)
{
    if ( bCogOpen )
    {
        sithCog_BroadcastMessage(SITHCOG_MSG_SHUTDOWN, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0);
    }

    numThingLinks = numSurfLinks = numSectorLinks = 0;
    sithCog_pMasterCog = NULL;
    bCogOpen = 0;
}

void J3DAPI sithCog_FreeWorldCogs(SithWorld* pWorld)
{
    size_t i;

    if ( pWorld->aCogScripts )
    {
        for ( i = 0; i < pWorld->numCogScripts; ++i )
        {
            sithCog_FreeScriptEntry(&pWorld->aCogScripts[i]);
            sithCog_RemoveScript(&pWorld->aCogScripts[i]);
        }

        stdMemory_Free(pWorld->aCogScripts);
        pWorld->aCogScripts    = NULL;
        pWorld->sizeCogScripts = 0;
        pWorld->numCogScripts  = 0;
    }

    if ( pWorld->aCogs )
    {
        for ( i = 0; i < pWorld->numCogs; ++i )
        {
            sithCog_FreeEntry(&pWorld->aCogs[i]);
        }

        stdMemory_Free(pWorld->aCogs);
        pWorld->aCogs    = NULL;
        pWorld->sizeCogs = 0;
        pWorld->numCogs  = 0;
    }
}

void J3DAPI sithCog_FreeScriptEntry(SithCogScript* pScript)
{
    size_t i;

#line 441
    SITH_ASSERTREL(pScript != NULL);

    sithCogParse_FreeSymbolTable(pScript->pSymbolTable);
    pScript->pSymbolTable = NULL;

    for ( i = 0; i < pScript->numSymbolRefs; ++i )
    {
        if ( pScript->aSymRefs[i].pDescription )
        {
            stdMemory_Free(pScript->aSymRefs[i].pDescription);
            pScript->aSymRefs[i].pDescription = NULL;
        }
    }

    if ( pScript->pCode )
    {
        stdMemory_Free(pScript->pCode);
        pScript->pCode = NULL;
    }
}

void J3DAPI sithCog_FreeEntry(SithCog* pCog)
{
    sithCogParse_FreeSymbolTable(pCog->pSymbolTable);
    pCog->pSymbolTable = NULL;

    if ( pCog->aHeap )
    {
        stdMemory_Free(pCog->aHeap);
        pCog->heapSize = 0;
    }
}

void J3DAPI sithCog_UpdateThingTimer(SithThing* pThing)
{
    if ( (pThing->flags & SITH_TF_PULSESET) != 0 && pThing->msecNextPulseTime <= sithTime_msecGameTime )
    {
        pThing->msecNextPulseTime = sithTime_msecGameTime + pThing->msecPulseInterval;
        sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_PULSE);
    }

    if ( (pThing->flags & SITH_TF_TIMERSET) != 0 && pThing->msecTimerTime <= sithTime_msecGameTime )
    {
        pThing->flags &= ~SITH_TF_TIMERSET;
        sithCog_ThingSendMessage(pThing, pThing, SITHCOG_MSG_TIMER);
    }
}

void J3DAPI sithCog_BroadcastMessage(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx)
{
    sithCog_BroadcastMessageEx(msgType, senderType, senderIdx, srcType, srcIdx, 0, 0, 0, 0);
}

void J3DAPI sithCog_BroadcastMessageEx(SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int param0, int param1, int param2, int param3)
{
    SithCog* pCog;
    size_t i;

    if ( sithWorld_pStaticWorld )
    {
        pCog = sithWorld_pStaticWorld->aCogs;
        for ( i = 0; i < sithWorld_pStaticWorld->numCogs; ++pCog, ++i )
        {
            sithCog_SendMessageEx(pCog, msgType, senderType, senderIdx, srcType, srcIdx, 0, param0, param1, param2, param3);
        }
    }

    if ( sithWorld_pCurrentWorld )
    {
        pCog = sithWorld_pCurrentWorld->aCogs;
        for ( i = 0; i < sithWorld_pCurrentWorld->numCogs; ++pCog, ++i )
        {
            sithCog_SendMessageEx(pCog, msgType, senderType, senderIdx, srcType, srcIdx, 0, param0, param1, param2, param3);
        }
    }
}

void J3DAPI sithCog_SendMessage(SithCog* pCog, unsigned int messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId)
{
    SithCogScript* pScript;
    size_t handlerNum;

    if ( !pCog )
    {
        return;
    }

    pScript = pCog->pScript;
#line 631
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(messageType > 0);

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: Message %d delivered, senderType=%d, senderIndex=%d, sourceType=%d, sourceIndex=%d, linkId=%d.\n", pCog->aName, messageType, senderType, senderIdx, srcType, srcIdx, linkId);
        sithConsole_PrintString(std_genBuffer);
    }

    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "Cog %s: Disabled, message ignored.\n", pCog->aName);
            sithConsole_PrintString(std_genBuffer);
        }
        return;
    }

    for ( handlerNum = 0; handlerNum < pScript->numHandlers; ++handlerNum )
    {
        if ( messageType == pScript->aHandlers[handlerNum].type )
        {
            break;
        }
    }

    if ( handlerNum == pScript->numHandlers )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "--Cog %s: Message %d received but ignored.  No handler.\n", pCog->aName, messageType);
            sithConsole_PrintString(std_genBuffer);
        }
        return;
    }

    if ( (pCog->flags & SITHCOG_PAUSED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "--Cog %s: Message %d received but COG is paused.\n", pCog->aName, messageType);
            sithConsole_PrintString(std_genBuffer);
        }
        return;
    }

    if ( messageType != SITHCOG_MSG_STARTUP
        && messageType != SITHCOG_MSG_SHUTDOWN
        && stdComm_IsGameActive()
        && !stdComm_IsGameHost()
        && (pCog->flags & SITHCOG_LOCAL) == 0 )
    {
        if ( messageType == SITHCOG_MSG_PULSE || messageType == SITHCOG_MSG_TIMER )
        {
            return;
        }

        sithDSSCog_SendMessage(pCog, messageType, senderType, senderIdx, srcType, srcIdx, linkId, 0, 0, 0, 0, sithMulti_server.idServer);
    }
    else
    {
        pCog->linkId     = linkId;
        pCog->senderIdx  = senderIdx;
        pCog->senderType = senderType;
        pCog->sourceIdx  = srcIdx;
        pCog->sourceType = srcType;
        pCog->params[0]  = 0;
        pCog->params[1]  = 0;
        pCog->params[2]  = 0;
        pCog->params[3]  = 0;

        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "--Cog %s: Message %d received and accepted for execution.\n", pCog->aName, messageType);
            sithConsole_PrintString(std_genBuffer);
        }

        sithCogExec_ExecuteMessage(pCog, handlerNum);
    }
}

int J3DAPI sithCog_SendMessageEx(SithCog* pCog, SithCogMsgType messageType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3)
{
    SithCogScript* pScript;
    size_t handlerNum;

#line 740
    SITH_ASSERTREL(pCog != NULL);

    pScript = pCog->pScript;
#line 746
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(messageType > 0);

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: MessageEx %d delivered, senderType=%d, senderIndex=%d, sourceType=%d, sourceIndex=%d, linkId=%d, param0=%g, param1=%g, param2=%g, param3=%g.\n", pCog->aName, messageType, senderType, senderIdx, srcType, srcIdx, linkId, param0, param1, param2, param3);
        sithConsole_PrintString(std_genBuffer);
    }

    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "Cog %s: Disabled, MessageEx ignored.\n", pCog->aName);
            sithConsole_PrintString(std_genBuffer);
        }
        return SITHCOG_NORETURNVALUE;
    }

    for ( handlerNum = 0; handlerNum < pScript->numHandlers; ++handlerNum )
    {
        if ( messageType == pScript->aHandlers[handlerNum].type )
        {
            break;
        }
    }

    if ( handlerNum == pScript->numHandlers )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "--Cog %s: MessageEx %d received but ignored.  No handler.\n", pCog->aName, messageType);
            sithConsole_PrintString(std_genBuffer);
        }
        return SITHCOG_NORETURNVALUE;
    }

    if ( (pCog->flags & SITHCOG_PAUSED) != 0 )
    {
        if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
        {
            sprintf(std_genBuffer, "--Cog %s: MessageEx %d received but COG is paused.\n", pCog->aName, messageType);
            sithConsole_PrintString(std_genBuffer);
        }
        return SITHCOG_NORETURNVALUE;
    }

    if ( messageType != SITHCOG_MSG_STARTUP
        && messageType != SITHCOG_MSG_SHUTDOWN
        && stdComm_IsGameActive()
        && !stdComm_IsGameHost()
        && (pCog->flags & SITHCOG_LOCAL) == 0 )
    {
        if ( messageType == SITHCOG_MSG_PULSE || messageType == SITHCOG_MSG_TIMER )
        {
            return 0;
        }

        sithDSSCog_SendMessage(pCog, messageType, senderType, senderIdx, srcType, srcIdx, linkId, param0, param1, param2, param3, sithMulti_server.idServer);
        return 0;
    }

    pCog->linkId      = linkId;
    pCog->senderIdx   = senderIdx;
    pCog->senderType  = senderType;
    pCog->sourceIdx   = srcIdx;
    pCog->sourceType  = srcType;
    pCog->params[0]   = param0;
    pCog->params[1]   = param1;
    pCog->params[2]   = param2;
    pCog->params[3]   = param3;
    pCog->returnValue = SITHCOG_NORETURNVALUE;

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "--Cog %s: MessageEx %d received and accepted for execution.\n", pCog->aName, messageType);
        sithConsole_PrintString(std_genBuffer);
    }

    sithCogExec_ExecuteMessage(pCog, handlerNum);
    return pCog->returnValue;
}

int J3DAPI sithCog_AllocWorldCogScripts(SithWorld* pWorld, size_t numCogScripts)
{
#line 856
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(!pWorld->aCogScripts);

#line 859
    pWorld->aCogScripts = (SithCogScript*)STDMALLOC(sizeof(SithCogScript) * numCogScripts);
    if ( !pWorld->aCogScripts )
    {
        goto alloc_error;
    }

    memset(pWorld->aCogScripts, 0, sizeof(SithCogScript) * numCogScripts);
    pWorld->sizeCogScripts = numCogScripts;
    pWorld->numCogScripts  = 0;
    return 0;

alloc_error:
#line 871
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory alloc failure initializing COG scripts.\n", 0, 0, 0, 0);
    return 1;
}

int J3DAPI sithCog_AllocWorldCogs(SithWorld* pWorld, size_t sizeCogs)
{
#line 895
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(!pWorld->aCogs);

#line 898
    pWorld->aCogs = (SithCog*)STDMALLOC(sizeof(SithCog) * sizeCogs);
    if ( !pWorld->aCogs )
    {
        goto alloc_error;
    }

    memset(pWorld->aCogs, 0, sizeof(SithCog) * sizeCogs);
    pWorld->sizeCogs = sizeCogs;
    pWorld->numCogs  = 0;
    return 0;

alloc_error:
#line 910
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Memory alloc failure initializing COGs.\n", 0, 0, 0, 0);
    return 1;
}

/* Discarded by the retail linker (its only caller is the debug-only sithWrite.c); restored in full from the
   debug build (0x00427737, between AllocWorldCogs and GetSymbolRefInitializer). Every tested write jumps to
   one `return 1` after the success return, from inside both loops too (debug jmp
   0x004278CA); the per-cog header line's result is not tested. Name inferred
 */
int J3DAPI sithCog_WriteCogsListText(const SithWorld* pWorld)
{
    size_t i;
    size_t symIdx;
    int num = 0;
    SithCog* pCog;
    char aValue[SITHCOG_SYMVALUESTRLEN];

    if ( stdConffile_WriteLine("######### COG placement ########\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: cogs\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World cogs %d\n", pWorld->numCogs) )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("#Num\tScript          Symbol values\n") )
    {
        goto error;
    }

    for ( pCog = pWorld->aCogs, i = 0; i < pWorld->numCogs; ++pCog, ++i )
    {
        if ( (pCog->flags & SITHCOG_CLASS) != 0 )
        {
            continue;
        }

        stdConffile_Printf("%d:\t%s\t", num++, pCog->pScript->aName);
        for ( symIdx = 0; symIdx < pCog->pScript->numSymbolRefs; ++symIdx )
        {
            if ( (pCog->pScript->aSymRefs[symIdx].bLocal & 1) != 0 )
            {
                continue;
            }

            sithCog_GetSymbolRefInitializer(pWorld, pCog, symIdx, aValue);
            if ( stdConffile_Printf("%s ", aValue) )
            {
                goto error;
            }
        }

        if ( stdConffile_WriteLine("\n") )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("\nend\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

void J3DAPI sithCog_GetSymbolRefInitializer(const SithWorld* pWorld, const SithCog* pCog, int symIdx, char* pOutString)
{
    SithCogSymbolRef* pRef;
    SithCogSymbol* pSymbol;
    size_t objectIdx;
    size_t refIdx;
    const char* pFilename;
    int numCogs;
    int cogIdx;
    size_t i;

    pRef      = &pCog->pScript->aSymRefs[symIdx];
    pSymbol   = &pCog->pSymbolTable->aSymbols[pRef->symbolId];
    objectIdx = pSymbol->value.val.intValue;
    refIdx    = objectIdx;

    switch ( pRef->type )
    {
        case SITHCOG_SYM_REF_FLEX:
            sprintf(pOutString, "%f", pSymbol->value.val.floatValue);
            break;

        case SITHCOG_SYM_REF_VECTOR:
            sprintf(pOutString, "(%f/%f/%f)", pSymbol->value.val.vecValue.x, pSymbol->value.val.vecValue.y, pSymbol->value.val.vecValue.z);
            break;

        case SITHCOG_SYM_REF_SOUND:
            objectIdx = STDMATH_CLAMP(objectIdx, 0, pWorld->numSounds);
            if ( objectIdx < 0 || objectIdx >= pWorld->numSounds )
            {
                objectIdx = 0;
#line 979
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid sound initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                sprintf(pOutString, "%s", pSymbol->pName);
            }
            else
            {
                pFilename = Sound_GetSoundFilename(Sound_GetSoundHandle(objectIdx));
                if ( pFilename )
                {
                    sprintf(pOutString, "%s", pFilename);
                }
                else
                {
#line 986
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unable to load sound initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                    sprintf(pOutString, "%s", pSymbol->pName);
                }
            }
            break;

        case SITHCOG_SYM_REF_MATERIAL:
            if ( objectIdx < 0 || objectIdx >= pWorld->numMaterials || !pWorld->aMaterials[objectIdx].aName[0] )
            {
                objectIdx = 0;
#line 996
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid material initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                sprintf(pOutString, "%s", pSymbol->pName);
            }
            else
            {
                sprintf(pOutString, "%s", pWorld->aMaterials[objectIdx].aName);
            }
            break;

        case SITHCOG_SYM_REF_KEYFRAME:
            if ( objectIdx < 0 || objectIdx >= pWorld->numKeyframes )
            {
                objectIdx = 0;
#line 1008
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid keyframe initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                sprintf(pOutString, "%s", pSymbol->pName);
            }
            else
            {
                sprintf(pOutString, "%s", pWorld->aKeyframes[objectIdx].aName);
            }
            break;

        case SITHCOG_SYM_REF_TEMPLATE:
            if ( objectIdx < 0 || objectIdx >= pWorld->numThingTemplates )
            {
                objectIdx = 0;
#line 1020
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid templates initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                sprintf(pOutString, "%s", pSymbol->pName);
            }
            else
            {
                sprintf(pOutString, "%s", pWorld->aThingTemplates[objectIdx].aName);
            }
            break;

        case SITHCOG_SYM_REF_AICLASS:
            if ( objectIdx < 0 || objectIdx >= pWorld->numAIClasses )
            {
                objectIdx = 0;
#line 1032
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI Class initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                sprintf(pOutString, "%s", pSymbol->pName);
            }
            else
            {
                sprintf(pOutString, "%s", pWorld->aAIClasses[objectIdx].aName);
            }
            break;

        case SITHCOG_SYM_REF_MODEL:
            if ( objectIdx < 0 || objectIdx >= pWorld->numModels )
            {
                objectIdx = 0;
#line 1044
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invalid AI Class initializer '%s' in COG '%s'\n", pSymbol->pName, pCog->aName, 0, 0);
                sprintf(pOutString, "%s", pSymbol->pName);
            }
            else
            {
                sprintf(pOutString, "%s", pWorld->aModels[objectIdx].aName);
            }
            break;

        case SITHCOG_SYM_REF_COG:
            cogIdx = objectIdx;
            cogIdx = STDMATH_CLAMP(cogIdx, -1, (int)pWorld->numCogs);
            objectIdx = cogIdx;
            if ( cogIdx < 0 )
            {
                sprintf(pOutString, "%d", cogIdx);
            }
            else
            {
                numCogs = 0;
                for ( i = 0; i < (size_t)cogIdx; ++i )
                {
                    if ( (pWorld->aCogs[i].flags & SITHCOG_CLASS) == 0 )
                    {
                        ++numCogs;
                    }
                }

                sprintf(pOutString, "%d", numCogs);
            }
            break;

        default:
            sprintf(pOutString, "%d", objectIdx);
            break;
    }

    if ( objectIdx != refIdx )
    {
#line 1086
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Invald Reference '%s' in COG '%s'", pCog->aName, pSymbol->pName, 0, 0);
    }
}


int J3DAPI sithCog_ReadCogsListText(SithWorld* pWorld, int bSkip)
{
    size_t cursizeCogs;
    size_t argNum = 0;
    SithCog* pCog;
    SithCogScript* pScript;
    size_t valIdx;
    size_t i;

    if ( bSkip )
    {
        return 1;
    }

#line 1120
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(pWorld->aCogs == NULL);

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[argNum++].argValue, "world") || strcmp(stdConffile_entry.aArgs[argNum++].argValue, "cogs") )
    {
        goto syntax_error;
    }

    cursizeCogs = atoi(stdConffile_entry.aArgs[argNum++].argValue);
    if ( !cursizeCogs )
    {
        return 0;
    }

    if ( sithCog_AllocWorldCogs(pWorld, cursizeCogs) )
    {
        goto alloc_error;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") )
    {
        argNum = 1;
        if ( stdConffile_entry.numArgs < 2 )
        {
            goto syntax_error;
        }

        pCog = sithCog_Load(pWorld, stdConffile_entry.aArgs[argNum++].argValue);
        if ( !pCog )
        {
#line 1149
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Placement of Cog %s failed.\n", stdConffile_entry.aArgs[argNum - 1].argValue, 0, 0, 0);
            continue;
        }

        pScript = pCog->pScript;
        valIdx = 0;
        for ( i = 0; i < pScript->numSymbolRefs; ++i )
        {
            if ( (pScript->aSymRefs[i].bLocal & 1) != 0 )
            {
                continue;
            }

            if ( stdConffile_entry.numArgs <= argNum )
            {
#line 1164
                stdPrintf(sith_pHS->pStatusPrint, J3D_FILE, __LINE__, "Cog %s: Not enough initializers to init referece #%d, line %d.\n", pCog->aName, i, stdConffile_GetLineNumber(), 0);
                continue;
            }

#line 1169
            SITH_ASSERTREL(stdConffile_entry.aArgs[argNum].argValue);
            strncpy(pCog->aSymRefValues[valIdx], stdConffile_entry.aArgs[argNum].argValue, 63);
            pCog->aSymRefValues[valIdx][63] = 0;
            ++argNum;
            ++valIdx;
        }
    }

    return 0;

alloc_error:
#line 1179
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No memory to allocate COG symbole table, line %d.\n", stdConffile_GetLineNumber(), 0, 0, 0);
    goto error;

syntax_error:
#line 1183
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);

error:
    return 1;
}
int J3DAPI sithCog_WriteCogsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    uint32_t aSizes[2];
    size_t i;
    size_t symIdx;
    SithCog* pCog;
    SithCogSymbolRef* pRef;
    size_t sizeNames;
    size_t sizeValues;
    char (*aNames)[64];
    char (*aValues)[SITHCOG_SYMVALUESTRLEN];
    char (*pName)[64];
    char (*pValue)[SITHCOG_SYMVALUESTRLEN];
    size_t numWritten;

    aNames  = NULL;
    aValues = NULL;
    aSizes[0] = 0;
    aSizes[1] = 0;
    for ( i = 0; i < pWorld->numCogs; ++i )
    {
        pCog = &pWorld->aCogs[i];
        if ( (pCog->flags & SITHCOG_CLASS) != 0 )
        {
            continue;
        }

        ++aSizes[0];
        for ( symIdx = 0; symIdx < pCog->pScript->numSymbolRefs; ++symIdx )
        {
            pRef = &pCog->pScript->aSymRefs[symIdx];
            if ( (pRef->bLocal & 1) != 0 )
            {
                continue;
            }

            ++aSizes[1];
        }
    }

    numWritten = sith_pHS->pFileWrite(fh, aSizes, sizeof(aSizes));
    if ( numWritten != sizeof(aSizes) )
    {
        return 1;
    }

    if ( !aSizes[0] )
    {
        return 0;
    }

    sizeNames = aSizes[0] * 64;
#line 1234
    aNames = (char (*)[64])STDMALLOC(sizeNames);
    if ( !aNames )
    {
        goto error;
    }

    pName = aNames;
    sizeValues = aSizes[1] * SITHCOG_SYMVALUESTRLEN;
#line 1239
    aValues = (char (*)[SITHCOG_SYMVALUESTRLEN])STDMALLOC(sizeValues);
    if ( !aValues )
    {
        goto error;
    }

    pValue = aValues;
    for ( i = 0; i < pWorld->numCogs; ++i )
    {
        pCog = &pWorld->aCogs[i];
        if ( (pCog->flags & SITHCOG_CLASS) != 0 )
        {
            continue;
        }

        strncpy(*pName, pCog->aName, 63);
        (*pName)[63] = 0;
        ++pName;

        for ( symIdx = 0; symIdx < pCog->pScript->numSymbolRefs; ++symIdx )
        {
            pRef = &pCog->pScript->aSymRefs[symIdx];
            if ( (pRef->bLocal & 1) != 0 )
            {
                continue;
            }

            sithCog_GetSymbolRefInitializer(pWorld, pCog, symIdx, *pValue);
            ++pValue;
        }
    }

    numWritten = sith_pHS->pFileWrite(fh, aNames, sizeNames);
    if ( numWritten != sizeNames )
    {
        goto error;
    }

    numWritten = sith_pHS->pFileWrite(fh, aValues, sizeValues);
    if ( numWritten != sizeValues )
    {
        goto error;
    }

    stdMemory_Free(aNames);
    aNames = NULL;
    stdMemory_Free(aValues);
    aValues = NULL;
    return 0;

error:
    if ( aNames )
    {
        stdMemory_Free(aNames);
    }

    if ( aValues )
    {
        stdMemory_Free(aValues);
    }

    return 1;
}

int J3DAPI sithCog_ReadCogsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    uint32_t aSizes[2];
    size_t nRead;
    size_t sizeNames;
    size_t sizeValues;
    char (*aNames)[64];
    char (*aValues)[SITHCOG_SYMVALUESTRLEN];
    const char (*pValue)[SITHCOG_SYMVALUESTRLEN];
    const char* pName;
    size_t i;
    size_t j;
    size_t valIdx;
    SithCog* pCog;

    aNames = NULL;
    aValues = NULL;
    nRead = sith_pHS->pFileRead(fh, aSizes, sizeof(aSizes));
    if ( nRead != sizeof(aSizes) )
    {
        goto error;
    }

    if ( sithCog_AllocWorldCogs(pWorld, pWorld->numCogs) )
    {
        goto error;
    }

    if ( !aSizes[0] )
    {
        return 0;
    }

    sizeNames = aSizes[0] * 64;
#line 1302
    aNames = (char (*)[64])STDMALLOC(sizeNames);
    if ( !aNames )
    {
        goto error;
    }

    sizeValues = aSizes[1] * SITHCOG_SYMVALUESTRLEN;
#line 1306
    aValues = (char (*)[SITHCOG_SYMVALUESTRLEN])STDMALLOC(sizeValues);
    if ( !aValues )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aNames, sizeNames);
    if ( nRead != sizeNames )
    {
        goto error;
    }

    nRead = sith_pHS->pFileRead(fh, aValues, sizeValues);
    if ( nRead != sizeValues )
    {
        goto error;
    }

    pValue = aValues;
    for ( i = 0; i < aSizes[0]; ++i )
    {
        pName = aNames[i];
        pCog = sithCog_Load(pWorld, pName);
        if ( !pCog )
        {
            goto error;
        }

        valIdx = 0;
        for ( j = 0; j < pCog->pScript->numSymbolRefs; ++j )
        {
            if ( pCog->pScript->aSymRefs[j].bLocal & 1 )
            {
                continue;
            }

            strncpy(pCog->aSymRefValues[valIdx], *pValue, 63);
            pCog->aSymRefValues[valIdx][63] = 0;
            ++valIdx;
            ++pValue;
        }
    }

    // The debug code (0x42852a) tests both pointers here as in the error block; retail's
    // error-entry reload stubs need the plain calls (debug/retail conflict).
    if ( aNames )
    {
        stdMemory_Free(aNames);
    }

    if ( aValues )
    {
        stdMemory_Free(aValues);
    }

    return 0;

error:
#line 1342
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error Reading COGs", 0, 0, 0, 0);
    if ( aNames )
    {
        stdMemory_Free(aNames);
    }

    if ( aValues )
    {
        stdMemory_Free(aValues);
    }

    return 1;
}
int J3DAPI sithCog_ParseSymbolRef(SithWorld* pWorld, SithCogSymbol* pSymbol, const SithCogSymbolRef* pRef, const char* pString)
{
    float x;
    SithThing* pTemplate;
    rdModel3* pModel;

    {
        SithAIClass* pAIClass;
        float y;
        rdMaterial* pMat;
        float z;
        rdKeyframe* pKeyframe;
        tSoundHandle hSnd;

#line 1383
        SITH_ASSERTREL(pSymbol && pRef && pString);

        switch ( pRef->type )
        {
            case SITHCOG_SYM_REF_FLEX:
                pSymbol->value.type = SITHCOG_VALUE_FLOAT;
                pSymbol->value.val.floatValue = D3DVAL(atof(pString));
                break;

            case SITHCOG_SYM_REF_VECTOR:
                pSymbol->value.type = SITHCOG_VALUE_VECTOR;
                if ( sscanf(pString, "(%f/%f/%f)", &x, &y, &z) != 3 )
                {
#line 1396
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Ref '%s' could not be read as a vector, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    memset(&pSymbol->value.val, 0, sizeof(pSymbol->value.val.vecValue));
                    return 1;
                }

                pSymbol->value.val.vecValue.x = x;
                pSymbol->value.val.vecValue.y = y;
                pSymbol->value.val.vecValue.z = z;
                break;

            case SITHCOG_SYM_REF_MATERIAL:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                pMat = sithMaterial_Load(pString);
                if ( pMat )
                {
                    pSymbol->value.val.intValue = pMat->num;
                }
                else
                {
#line 1412
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "MaterialRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    pSymbol->value.val.intValue = -1;
                    return 1;
                }
                break;

            case SITHCOG_SYM_REF_SOUND:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                hSnd = sithSound_Load(pWorld, pString);
                if ( hSnd )
                {
                    pSymbol->value.val.intValue = Sound_GetSoundIndex(hSnd);
                }
                else
                {
                    pSymbol->value.val.intValue = -1;
#line 1426
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "SoundRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    return 1;
                }
                break;

            case SITHCOG_SYM_REF_TEMPLATE:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                pTemplate = sithTemplate_GetTemplate(pString);
                if ( pTemplate )
                {
                    pSymbol->value.val.intValue = pTemplate->idx;
                }
                else
                {
                    pSymbol->value.val.intValue = -1;
#line 1440
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "TemplateRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    return 1;
                }
                break;

            case SITHCOG_SYM_REF_MODEL:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                pModel = sithModel_Load(pString, 1);
                if ( pModel )
                {
                    pSymbol->value.val.intValue = pModel->num;
                }
                else
                {
                    pSymbol->value.val.intValue = -1;
#line 1454
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "ModelRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    return 1;
                }
                break;

            case SITHCOG_SYM_REF_KEYFRAME:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                pKeyframe = sithPuppet_LoadKeyframe(pString);
                if ( pKeyframe )
                {
                    pSymbol->value.val.intValue = pKeyframe->idx;
                }
                else
                {
                    pSymbol->value.val.intValue = -1;
#line 1468
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "KeyframeRef '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    return 1;
                }
                break;

            case SITHCOG_SYM_REF_AICLASS:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                pAIClass = sithAIClass_Load(pWorld, pString);
                if ( pAIClass )
                {
                    pSymbol->value.val.intValue = pAIClass->num;
                }
                else
                {
                    pSymbol->value.val.intValue = -1;
#line 1482
                    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "AIClass '%s' could not be resolved, line %d.\n", pString, stdConffile_GetLineNumber(), 0, 0);
                    return 1;
                }
                break;

            default:
                pSymbol->value.type = SITHCOG_VALUE_INT;
                pSymbol->value.val.intValue = atoi(pString);
                break;
        }

    }

    return 0;
}

int J3DAPI sithCog_LinkCog(const SithWorld* pWorld, SithCog* pCog, const SithCogSymbolRef* pRef, const SithCogSymbol* pSymbol)
{
    int index;

#line 1527
    SITH_ASSERTREL(pCog && pRef && pSymbol);

    index = pSymbol->value.val.intValue;
    if ( index < 0 )
    {
        return 1;
    }

    switch ( pRef->type )
    {
        case SITHCOG_SYM_REF_THING:
            if ( index >= (int)pWorld->numThings )
            {
#line 1537
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not link Cog %s to thing %d, out of range.\n", pCog->aName, index, 0, 0);
                return 1;
            }

            return sithCog_LinkCogToThing(pWorld, pCog, &pWorld->aThings[index], pRef->linkId, pRef->mask);

        case SITHCOG_SYM_REF_SURFACE:
            if ( index >= (int)pWorld->numSurfaces )
            {
#line 1546
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not link Cog %s to surface %d, out of range.\n", pCog->aName, index, 0, 0);
                return 1;
            }

            return sithCog_LinkCogToSurface(pCog, &pWorld->aSurfaces[index], pRef->linkId, pRef->mask);

        case SITHCOG_SYM_REF_SECTOR:
            if ( index >= (int)pWorld->numSectors )
            {
#line 1556
                stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not link Cog %s to sector %d, out of range.\n", pCog->aName, index, 0, 0);
                return 1;
            }

            return sithCog_LinkCogToSector(pWorld, pCog, &pWorld->aSectors[index], pRef->linkId, pRef->mask);

        default:
            return 0;
    }
}

SithCog* J3DAPI sithCog_Load(SithWorld* pWorld, const char* pName)
{
    SithCog* pCog;
    SithCogScript* pScript;

#line 1590
    SITH_ASSERTREL(pWorld && pName);

    if ( pWorld->numCogs >= pWorld->sizeCogs )
    {
#line 1594
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space to load Cog %s.\n", pName, 0, 0, 0);
        return NULL;
    }

    pCog = &pWorld->aCogs[pWorld->numCogs];
    pCog->idx = pWorld->numCogs;
    if ( (pWorld->state & SITH_WORLD_STATE_STATIC) != 0 )
    {
        pCog->idx = SITHWORLD_STATICINDEX(pCog->idx);
    }

    pScript = sithCog_LoadScript(pWorld, pName);
    if ( !pScript )
    {
#line 1611
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Load of cog script %s failed.\n", pName, 0, 0, 0);
        return NULL;
    }

    strncpy(pCog->aName, pScript->aName, 63);
    pCog->aName[63] = 0;
    pCog->pScript = pScript;
    pCog->flags   = pScript->flags;

    pCog->pSymbolTable = sithCogParse_DuplicateSymbolTable(pScript->pSymbolTable);
    if ( !pCog->pSymbolTable )
    {
#line 1625
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not allocate symbol table for cog %s.\n", pCog->aName, 0, 0, 0);
        return NULL;
    }

    ++pWorld->numCogs;
    return pCog;
}

SithCog* J3DAPI sithCog_GetCogByIndex(int idx)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    if ( SITHWORLD_IS_STATICINDEX(idx) )
    {
        pWorld = sithWorld_pStaticWorld;
        idx    = SITHWORLD_FROM_STATICINDEX(idx);
    }

    if ( !pWorld || idx < 0 || idx >= (signed int)pWorld->numCogs )
    {
        return NULL;
    }

    return &pWorld->aCogs[idx];
}

SithCog* J3DAPI sithCog_GetCog(const char* pName)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    size_t curi;
    size_t j;

    if ( !pName )
    {
        return NULL;
    }

    for ( curi = 0; curi < 2; ++curi )
    {
        if ( curi == 1 )
        {
            pWorld = sithWorld_pStaticWorld;
        }

        if ( !pWorld )
        {
            continue;
        }

        for ( j = 0; j < pWorld->numCogs; ++j )
        {
            if ( streqi(pName, pWorld->aCogs[j].aName) )
            {
                return &pWorld->aCogs[j];
            }
        }
    }

    return NULL;
}

int J3DAPI sithCog_LinkCogToThing(const SithWorld* pWorld, SithCog* pCog, SithThing* pThing, int linkId, int mask)
{
#line 1727
    SITH_ASSERTREL(pCog && pThing);

    if ( !sithThing_ValidateThingPointer(pWorld, pThing) || pThing->type == SITH_THING_FREE )
    {
        return 1;
    }

    if ( linkId >= 0 )
    {
#line 1734
        SITH_ASSERTREL(numThingLinks < 1024);

        pThing->flags |= SITH_TF_COGLINKED;
        aThingLinks[numThingLinks].pThing = pThing;
        aThingLinks[numThingLinks].pCog   = pCog;
        aThingLinks[numThingLinks].linkId = linkId;
        aThingLinks[numThingLinks].mask   = mask;
        aThingLinks[numThingLinks].thingSignature = pThing->signature;
        ++numThingLinks;
    }

    return 0;
}

int J3DAPI sithCog_LinkCogToSurface(SithCog* pCog, SithSurface* pSurface, int linkId, int mask)
{
#line 1749
    SITH_ASSERTREL(pCog && pSurface);

    if ( !sithSurface_ValidateSurfacePointer(pSurface) )
    {
        return 1;
    }

    if ( linkId >= 0 )
    {
#line 1758
        SITH_ASSERTREL(numSurfLinks < 512);

        pSurface->flags |= SITH_SURFACE_COGLINKED;
        aCogLinkSurfaces[numSurfLinks].pSurface = pSurface;
        aCogLinkSurfaces[numSurfLinks].pCog     = pCog;
        aCogLinkSurfaces[numSurfLinks].linkId   = linkId;
        aCogLinkSurfaces[numSurfLinks++].mask   = mask;
    }

    return 0;
}

int J3DAPI sithCog_LinkCogToSector(const SithWorld* pWorld, SithCog* pCog, SithSector* pSector, int linkId, int mask)
{
#line 1772
    SITH_ASSERTREL(pCog && pSector);

    if ( !sithSector_ValidateSectorPointer(pWorld, pSector) )
    {
        return 1;
    }

    if ( linkId >= 0 )
    {
#line 1781
        SITH_ASSERTREL(numSectorLinks < 512);

        pSector->flags |= SITH_SECTOR_COGLINKED;
        aCogLinkSectors[numSectorLinks].pSector = pSector;
        aCogLinkSectors[numSectorLinks].pCog    = pCog;
        aCogLinkSectors[numSectorLinks].linkId  = linkId;
        aCogLinkSectors[numSectorLinks++].mask  = mask;
    }

    return 0;
}

void J3DAPI sithCog_FixupLinksToThings(void)
{
    size_t i;
    for ( i = 0; i < numThingLinks; ++i )
    {
        aThingLinks[i].thingSignature = aThingLinks[i].pThing->signature;
    }
}

int J3DAPI sithCog_ThingSendMessage(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType msgType)
{
    return sithCog_ThingSendMessageEx(pThing, pSrcThing, msgType, 0, 0, 0, 0);
}

int J3DAPI sithCog_ThingSendMessageEx(const SithThing* pThing, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    SithCogThingLink* pLink = aThingLinks;
    SithCogThingLink* pLastLink = &aThingLinks[numThingLinks];
    int status;
    int retVal = 0;
    SithCogSymbolRefType srcRefType;
    int srcIdx;
    int thingMask;

#line 1853
    SITH_ASSERTREL(pThing);

    if ( messageType == SITHCOG_MSG_DAMAGED )
    {
        retVal = param0;
    }

    if ( pSrcThing )
    {
        srcIdx  = pSrcThing->idx;
        srcRefType = SITHCOG_SYM_REF_THING;
        thingMask    = SITHTHING_TYPE2MASK(pSrcThing->type);
    }
    else
    {
        srcIdx  = -1;
        srcRefType = SITHCOG_SYM_REF_NONE;
        thingMask    = SITHTHING_TYPE2MASK(SITH_THING_FREE);
    }

    if ( pThing->pCog )
    {
        if ( messageType == SITHCOG_MSG_DAMAGED )
        {
            status = sithCog_SendMessageEx(pThing->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcRefType, srcIdx, 0, param0, param1, param2, param3);
            if ( status != SITHCOG_NORETURNVALUE )
            {
                param0 = retVal = status;
            }
        }
        else
        {
            status = sithCog_SendMessageEx(pThing->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcRefType, srcIdx, 0, param0, param1, param2, param3);
            if ( status != SITHCOG_NORETURNVALUE )
            {
                retVal += status;
            }
        }
    }

    if ( pThing->pCaptureCog )
    {
        if ( messageType == SITHCOG_MSG_DAMAGED )
        {
            status = sithCog_SendMessageEx(pThing->pCaptureCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcRefType, srcIdx, 0, param0, param1, param2, param3);
            if ( status != SITHCOG_NORETURNVALUE )
            {
                param0 = retVal = status;
            }
        }
        else
        {
            status = sithCog_SendMessageEx(pThing->pCaptureCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcRefType, srcIdx, 0, param0, param1, param2, param3);
            if ( status != SITHCOG_NORETURNVALUE )
            {
                retVal += status;
            }
        }
    }

    while ( pLink < pLastLink )
    {
        if ( pLink->pThing == pThing && pLink->thingSignature == pThing->signature && (pLink->mask & thingMask) != 0 )
        {
            if ( messageType == SITHCOG_MSG_DAMAGED )
            {
                status = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcRefType, srcIdx, 0, param0, param1, param2, param3);
                if ( status != SITHCOG_NORETURNVALUE )
                {
                    param0 = retVal = status;
                }
            }
            else
            {
                status = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_THING, pThing->idx, srcRefType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( status != SITHCOG_NORETURNVALUE )
                {
                    retVal += status;
                }
            }
        }

        ++pLink;
    }

    return retVal;
}

int J3DAPI sithCog_SectorSendMessage(const SithSector* pSector, const SithThing* pThing, SithCogMsgType msgType)
{
    return sithCog_SectorSendMessageEx(pSector, pThing, msgType, 0, 0, 0, 0);
}

int J3DAPI sithCog_SectorSendMessageEx(const SithSector* pSector, const SithThing* pThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    SithCogSectorLink* pLink = aCogLinkSectors;
    SithCogSectorLink* pLastLink = &aCogLinkSectors[numSectorLinks];
    int status;
    int retVal = 0;
    SithCogSymbolRefType srcRefType;
    int srcIdx;
    int thingMask;

#line 2000
    SITH_ASSERTREL(pSector);

    if ( pThing )
    {
        srcIdx  = pThing->idx;
        srcRefType = SITHCOG_SYM_REF_THING;
        thingMask    = SITHTHING_TYPE2MASK(pThing->type);
    }
    else
    {
        srcIdx  = -1;
        srcRefType = SITHCOG_SYM_REF_NONE;
        thingMask    = SITHTHING_TYPE2MASK(SITH_THING_FREE);
    }

    while ( pLink < pLastLink )
    {
        if ( pLink->pSector == pSector && (pLink->mask & thingMask) != 0 )
        {
            if ( messageType == SITHCOG_MSG_DAMAGED )
            {
                status = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_SECTOR, sithSector_GetSectorIndex(pSector), srcRefType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( status != SITHCOG_NORETURNVALUE )
                {
                    param0 = retVal = status;
                }
                else
                {
                    retVal = param0;
                }
            }
            else
            {
                status = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_SECTOR, sithSector_GetSectorIndex(pSector), srcRefType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( status != SITHCOG_NORETURNVALUE )
                {
                    retVal += status;
                }
            }
        }

        ++pLink;
    }

    return retVal;
}

int J3DAPI sithCog_SurfaceSendMessage(const SithSurface* pSurf, const SithThing* pSrcThing, SithCogMsgType messageType)
{
    return sithCog_SurfaceSendMessageEx(pSurf, pSrcThing, messageType, 0, 0, 0, 0);
}

int J3DAPI sithCog_SurfaceSendMessageEx(const SithSurface* pSurf, const SithThing* pSrcThing, SithCogMsgType messageType, int param0, int param1, int param2, int param3)
{
    SithCogSurfaceLink* pLink = aCogLinkSurfaces;
    SithCogSurfaceLink* pLastLink = &aCogLinkSurfaces[numSurfLinks];
    int status;
    int retVal = 0;
    SithCogSymbolRefType srcRefType;
    int srcIdx;
    int thingMask;

#line 2103
    SITH_ASSERTREL(pSurf);

    if ( pSrcThing )
    {
        srcIdx  = pSrcThing->idx;
        srcRefType = SITHCOG_SYM_REF_THING;
        thingMask    = SITHTHING_TYPE2MASK(pSrcThing->type);
    }
    else
    {
        srcIdx  = -1;
        srcRefType = SITHCOG_SYM_REF_NONE;
        thingMask    = SITHTHING_TYPE2MASK(SITH_THING_FREE);
    }

    while ( pLink < pLastLink )
    {
        if ( pLink->pSurface == pSurf && (pLink->mask & thingMask) != 0 )
        {
            if ( messageType == SITHCOG_MSG_DAMAGED )
            {
                status = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_SURFACE, sithSurface_GetSurfaceIndex(pSurf), srcRefType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( status != SITHCOG_NORETURNVALUE )
                {
                    param0 = retVal = status;
                }
                else
                {
                    retVal = param0;
                }
            }
            else
            {
                status = sithCog_SendMessageEx(pLink->pCog, messageType, SITHCOG_SYM_REF_SURFACE, sithSurface_GetSurfaceIndex(pSurf), srcRefType, srcIdx, pLink->linkId, param0, param1, param2, param3);
                if ( status != SITHCOG_NORETURNVALUE )
                {
                    retVal += status;
                }
            }
        }

        ++pLink;
    }

    return retVal;
}

/* Linker-discarded: retail __LINE__ gap 2103..2192 (sithCog_SurfaceSendMessageEx -> sithCog_ReadCogScriptsListText); the
   debug build keeps it there (0x004299CC). */
int J3DAPI sithCog_WriteCogScriptsListText(const SithWorld* pWorld)
{
    size_t i;

    if ( stdConffile_WriteLine("########## COG scripts #########\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("Section: cogscripts\n") )
    {
        goto error;
    }

    if ( stdConffile_Printf("World scripts %d\n", pWorld->numCogScripts) )
    {
        goto error;
    }

    for ( i = 0; i < pWorld->numCogScripts; ++i )
    {
        if ( stdConffile_Printf("%d:\t%s\n", i, pWorld->aCogScripts[i].aName) )
        {
            goto error;
        }
    }

    if ( stdConffile_WriteLine("end\n") )
    {
        goto error;
    }

    if ( stdConffile_WriteLine("################################\n\n\n") )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithCog_ReadCogScriptsListText(SithWorld* pWorld, int bSkip)
{
    size_t numScripts;

#line 2192
    SITH_ASSERTREL(pWorld != NULL);

    if ( bSkip )
    {
        return 1;
    }

    stdConffile_ReadArgs();
    if ( strcmp(stdConffile_entry.aArgs[0].argValue, "world") != 0 || strcmp(stdConffile_entry.aArgs[1].argValue, "scripts") != 0 )
    {
        goto error;
    }

    numScripts = atoi(stdConffile_entry.aArgs[2].argValue);
    if ( !numScripts )
    {
        return 0;
    }

#line 2209
    SITH_ASSERTREL(pWorld->aCogScripts == NULL);
    if ( sithCog_AllocWorldCogScripts(pWorld, numScripts) )
    {
        return 1;
    }

    while ( stdConffile_ReadArgs() && strcmp(stdConffile_entry.aArgs[0].argValue, "end") != 0 )
    {
        if ( pWorld->numCogScripts >= pWorld->sizeCogScripts )
        {
#line 2221
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Not enough space to load COG scripts.  Script skipped.\n", 0, 0, 0, 0);
            continue;
        }

        if ( stdConffile_entry.numArgs < 1 )
        {
            goto error;
        }

        if ( !sithCog_LoadScript(pWorld, stdConffile_entry.aArgs[1].argValue) )
        {
#line 2230
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to load COG script %s.\n", stdConffile_entry.aArgs[1].argValue, 0, 0, 0);
        }
    }

    return 0;

error:
#line 2237
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, stdConffile_GetFilename(), 0, 0);
    return 1;
}

int J3DAPI sithCog_WriteCogScriptsListBinary(tFileHandle fh, const SithWorld* pWorld)
{
    size_t written;
    size_t i;
    for ( i = 0; i < pWorld->numCogScripts; ++i )
    {
        written = sith_pHS->pFileWrite(fh, pWorld->aCogScripts[i].aName, STD_ARRAYLEN(pWorld->aCogScripts[i].aName));
        if ( written != STD_ARRAYLEN(pWorld->aCogScripts[i].aName) )
        {
            return 1;
        }
    }

    return 0;
}

int J3DAPI sithCog_ReadCogScriptsListBinary(tFileHandle fh, SithWorld* pWorld)
{
    size_t nRead;
    size_t numCogScripts = pWorld->numCogScripts;
    char szScriptName[64];
    size_t i;

    if ( sithCog_AllocWorldCogScripts(pWorld, pWorld->sizeCogScripts) )
    {
        goto error;
    }

    for ( i = 0; i < numCogScripts; ++i )
    {
        nRead = sith_pHS->pFileRead(fh, szScriptName, STD_ARRAYLEN(szScriptName));
        if ( nRead != STD_ARRAYLEN(szScriptName) )
        {
            goto error;
        }

        if ( !sithCog_LoadScript(pWorld, szScriptName) )
        {
            goto error;
        }
    }

    return 0;

error:
    return 1;
}

SithCogScript* J3DAPI sithCog_LoadScript(SithWorld* pWorld, const char* pName)
{
    char aPath[128];
    SithCogScript* pScript;

#line 2303
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(pWorld != NULL);
    SITH_ASSERTREL(strlen(pName)+32 < 128);

    sprintf(aPath, "%s%c%s", "cog", '\\', pName);

    pScript = sithCog_GetScript(pName);
    if ( pScript )
    {
        return pScript;
    }

    if ( pWorld->numCogScripts >= pWorld->sizeCogScripts )
    {
#line 2320
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "No space left to load script %s.\n", pName, 0, 0, 0);
        return NULL;
    }

    pScript = &pWorld->aCogScripts[pWorld->numCogScripts];
    if ( sithCogParse_Load(aPath, pScript, 0) )
    {
        sithCog_AddScript(pScript);
        ++pWorld->numCogScripts;
        return pScript;
    }

#line 2336
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Load failed for script %s.\n", aPath, 0, 0, 0);
    return NULL;
}

void J3DAPI sithCog_ProcessCog(SithCog* pCog)
{
    if ( (pCog->flags & SITHCOG_DISABLED) != 0 )
    {
        return;
    }

    if ( (pCog->flags & SITHCOG_PULSE_SET) != 0 && sithTime_msecGameTime >= pCog->msecNextPulseTime )
    {
        pCog->msecNextPulseTime = sithTime_msecGameTime + pCog->msecPulseInterval;
        sithCog_SendMessage(pCog, SITHCOG_MSG_PULSE, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    if ( (pCog->flags & SITHCOG_TIMER_SET) != 0 && sithTime_msecGameTime >= pCog->msecTimerTimeout )
    {
        pCog->flags &= ~SITHCOG_TIMER_SET;
        pCog->msecTimerTimeout = 0;
        sithCog_SendMessage(pCog, SITHCOG_MSG_TIMER, SITHCOG_SYM_REF_NONE, 0, SITHCOG_SYM_REF_NONE, 0, 0);
    }

    switch ( pCog->status )
    {
        case SITHCOG_STATUS_SLEEPING:
            if ( (unsigned int)pCog->statusParams[0] < sithTime_msecGameTime )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Waking up due to timer elapse.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;

        case SITHCOG_STATUS_WAITING_PLAYER_TO_STOP:
        {
            SithThing* pThing = sithPlayer_pLocalPlayerThing;
            if ( !pThing || pThing->type == SITH_THING_FREE )
            {
                sprintf(std_genBuffer, "Cog %s was waiting for player to stop, player is gone!", pCog->aName);
                sithConsole_PrintString(std_genBuffer);
                sithCogExec_Execute(pCog);
            }

            if ( pThing->moveStatus == SITHPLAYERMOVE_STILL )
            {
                sithCogExec_Execute(pCog);
            }
            return;
        }

        case SITHCOG_STATUS_WAITING_THING_TO_STOP:
            if ( (sithWorld_pCurrentWorld->aThings[pCog->statusParams[0]].moveInfo.pathMovement.mode & (SITH_PATHMOVE_ROTATE | SITH_PATHMOVE_MOVE)) == 0 )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Waking up due to movement completion.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;

        case SITHCOG_STATUS_WAITING_AI_TO_STOP:
        {
            SithThing* pAIThing = &sithWorld_pCurrentWorld->aThings[pCog->statusParams[0]];
            if ( !pAIThing || pAIThing->type == SITH_THING_FREE )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Was waiting on AI movement completion, but AI no longer exists!", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( !pAIThing->controlInfo.aiControl.pLocal )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Was waiting on AI %s movement completion, but AI control block no longer exists!", pCog->aName, pAIThing->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( (pAIThing->controlInfo.aiControl.pLocal->mode & (SITHAI_MODE_TURNING | SITHAI_MODE_MOVING)) == 0 )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Waking up due to AI movement completion.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;
        }

        case SITHCOG_STATUS_WAITING_AI_HEAD_TRACK:
        {
            SithThing* pHeadThing = &sithWorld_pCurrentWorld->aThings[pCog->statusParams[0]];
            if ( !pHeadThing || pHeadThing->type == SITH_THING_FREE )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Was waiting on AI head tracking, but AI no longer exists!", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( !pHeadThing->controlInfo.aiControl.pLocal )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Was waiting on AI %s head tracking, but AI control block no longer exists!", pCog->aName, pHeadThing->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
                return;
            }

            if ( (pHeadThing->controlInfo.aiControl.pLocal->submode & SITHAI_SUBMODE_HEADTRACKINGMOTION) == 0 )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Waking up due to AI head tracking at max yaw.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;
        }

        case SITHCOG_STATUS_WAITING_ANIMATION_TO_STOP:
        {
            int animId = pCog->statusParams[0];
            SithAnimationSlot* pAnim = sithAnimate_GetAnim(animId);
            if ( !pAnim )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: Waking up due to animation completion.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;
        }

        case SITHCOG_STATUS_WAITING_SOUND_TO_STOP:
        {
            tSoundChannelHandle hChannel = sithSoundMixer_GetChannelHandle(pCog->statusParams[0]);
            if ( (Sound_GetChannelFlags(hChannel) & SOUND_CHANNEL_PLAYING) == 0 )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: waking up due to sound completion.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;
        }

        case SITHCOG_STATUS_WAITING_KEYFRAME_TO_STOP:
        {
            SithThing* pKeyThing = &sithWorld_pCurrentWorld->aThings[pCog->statusParams[1]];
            int trackStatus;
            int trackNum = sithPuppet_GetTrackNumForGUID(pKeyThing, pCog->statusParams[0]);
            if ( trackNum == -1 )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Cog %s: waking up due to keyframe completion.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            else
            {
                trackStatus = pKeyThing->renderData.pPuppet->aTracks[trackNum].status;
                if ( (trackStatus & RDPUPPET_TRACK_PLAYING) == 0 )
                {
                    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                    {
                        sprintf(std_genBuffer, "Cog %s: waking up due to keyframe completion.\n", pCog->aName);
                        sithConsole_PrintString(std_genBuffer);
                    }

                    sithCogExec_Execute(pCog);
                }
            }
            return;
        }

        case SITHCOG_STATUS_WAITING_ACTOR_WEAPON_SELECT_FINISH:
            if ( sithWeapon_SelectWeapon(&sithWorld_pCurrentWorld->aThings[pCog->statusParams[0]], (SithWeaponId)pCog->statusParams[1]) )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Weapon selection successful. Waking up cog %s.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;

        case SITHCOG_STATUS_WAITING_ACTOR_WEAPON_DESELECT_FINISH:
        {
            SithThing* pWeaponThing = &sithWorld_pCurrentWorld->aThings[pCog->statusParams[0]];
            if ( pWeaponThing->thingInfo.actorInfo.weaponInfo.deselectedWeaponID != -1 )
            {
                return;
            }

            if ( sithWeapon_GetMountWait() <= sithTime_secGameTime )
            {
                if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
                {
                    sprintf(std_genBuffer, "Weapon deselection complete. Waking up cog %s.\n", pCog->aName);
                    sithConsole_PrintString(std_genBuffer);
                }

                sithCogExec_Execute(pCog);
            }
            return;
        }

        default:
            return;
    }
}

void J3DAPI sithCog_ProcessCogs(void)
{
    SithWorld* pWorld;
    SithCog* pCog;
    size_t i;

    pWorld = sithWorld_pCurrentWorld;
#line 2660
    SITH_ASSERTREL(pWorld != NULL);

    if ( sith_mode.masterMode == SITH_MODE_UNKNOWN_2 )
    {
        return;
    }

    pCog = pWorld->aCogs;
    for ( i = 0; i < pWorld->numCogs; ++pCog, ++i )
    {
        sithCog_ProcessCog(pCog);
    }

    if ( sithWorld_pStaticWorld )
    {
        pWorld = sithWorld_pStaticWorld;
        pCog = pWorld->aCogs;
        for ( i = 0; i < pWorld->numCogs; ++pCog, ++i )
        {
            sithCog_ProcessCog(pCog);
        }
    }
}

#ifdef J3D_DEBUG
/* Debug-only console command, registered only by the debug build's sithCommand_RegisterCommands: retail __LINE__ gap
   2660..2771 (sithCog_ProcessCogs -> sithCog_AddIntSymbol); the debug build keeps it there (0x0042A6E3). The failure
   path falls off the end after its print. */
int J3DAPI sithCog_CogStatus(const SithConsoleCommand* pFunc, const char* pArg)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    size_t i;
    size_t index;
    SithCog* pCog;
    SithCogSymbol* pSymbol;

    if ( !pWorld || !pArg || sscanf(pArg, "%d", &index) != 1 )
    {
        goto error;
    }

    if ( index > pWorld->numCogs )
    {
        goto error;
    }

    pCog = &pWorld->aCogs[index];
    if ( !pCog->pScript || !pCog->pSymbolTable )
    {
        goto error;
    }

    sprintf(std_genBuffer, "Cog #%d: Name:%s  Script %s\n", index, pCog->aName, pCog->pScript->aName);
    sithConsole_PrintString(std_genBuffer);

    for ( i = 0, pSymbol = pCog->pSymbolTable->aSymbols; i < pCog->pSymbolTable->numUsedSymbols; ++i, ++pSymbol )
    {
        sprintf(std_genBuffer, "  Symbol %d: '%s' ", pSymbol->id, pSymbol->pName ? pSymbol->pName : "<null>");
        if ( pSymbol->value.type == SITHCOG_VALUE_FLOAT )
        {
            sprintf(&std_genBuffer[strlen(std_genBuffer)], " = %f\n", pSymbol->value.val.floatValue);
        }
        else
        {
            sprintf(&std_genBuffer[strlen(std_genBuffer)], " = %d\n", pSymbol->value.val.intValue);
        }

        sithConsole_PrintString(std_genBuffer);
    }

    return 0;

error:
    sithConsole_PrintString("Error, bad parameters.\n");
}
#endif

void J3DAPI sithCog_AddIntSymbol(SithCogSymbolTable* pTbl, int val, const char* pName)
{
    SithCogSymbolValue value;
    SithCogSymbol* pSymbol;

    pSymbol = sithCogParse_AddSymbol(pTbl, pName);
    if ( !pSymbol )
    {
#line 2771
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not add symbol %s to table.\n", pName, 0, 0, 0);
        return;
    }

    value.type = SITHCOG_VALUE_INT;
    value.val.intValue = val;
    sithCogParse_SetSymbolValue(pSymbol, &value);
}

int J3DAPI sithCog_TimerEventTask(int msecTime, SithEventParams* aParams)
{
    SithCog* pCog;

#line 2787
    SITH_ASSERTREL(aParams);
    pCog = sithCog_GetCogByIndex(aParams->idx);
    if ( !pCog )
    {
        return 1;
    }

    sithCog_SendMessageEx(pCog, SITHCOG_MSG_TIMER, SITHCOG_SYM_REF_COG, pCog->idx, SITHCOG_SYM_REF_NONE, 0, aParams->param1, aParams->param2, aParams->param3, 0, 0);
    return 1;
}

SithCogScript* J3DAPI sithCog_GetScript(const char* pName)
{
#line 2806
    SITH_ASSERTREL(pName != NULL);
    SITH_ASSERTREL(sithCog_pHashtable != NULL);
    return (SithCogScript*)stdHashtbl_Find(sithCog_pHashtable, pName);
}

int J3DAPI sithCog_AddScript(SithCogScript* pScript)
{
#line 2814
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(strlen(pScript->aName) > 0);
    SITH_ASSERTREL(sithCog_pHashtable != NULL);
    return stdHashtbl_Add(sithCog_pHashtable, pScript->aName, pScript);
}

int J3DAPI sithCog_RemoveScript(SithCogScript* pScript)
{
#line 2823
    SITH_ASSERTREL(pScript != NULL);
    SITH_ASSERTREL(strlen(pScript->aName) > 0);
    SITH_ASSERTREL(sithCog_pHashtable != NULL);
    return stdHashtbl_Remove(sithCog_pHashtable, pScript->aName);
}

void J3DAPI sithCog_Initialize(void)
{
    SithCogSymbolTable* pTable = sithCog_pSymbolTable;

    sithCogFunction_RegisterFunctions(pTable);
    sithCogFunctionThing_RegisterFunctions(pTable);
    sithCogFunctionAI_RegisterFunctions(pTable);
    sithCogFunctionSurface_RegisterFunctions(pTable);
    sithCogFunctionSound_RegisterFunctions(pTable);
    sithCogFunctionSector_RegisterFunctions(pTable);
    sithCogFunctionPlayer_RegisterFunctions(pTable);
    sithVoice_RegisterCogFunctions(pTable);

    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ACTIVATE, "activate");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ACTIVATE, "activated");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_STARTUP, "startup");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TIMER, "timer");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_BLOCKED, "blocked");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ENTERED, "entered");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_EXITED, "exited");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CROSSED, "crossed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SIGHTED, "sighted");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_DAMAGED, "damaged");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ARRIVED, "arrived");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_KILLED, "killed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_PULSE, "pulse");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TOUCHED, "touched");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CREATED, "created");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_LOADING, "loading");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SELECTED, "selected");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_DESELECTED, "deselected");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CHANGED, "changed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_DEACTIVATED, "deactivated");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SHUTDOWN, "shutdown");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_RESPAWN, "respawn");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_REMOVED, "removed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_AIM, "aim");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_AIEVENT, "aievent");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_CALLBACK, "callback");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TAKEN, "taken");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER0, "user0");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER1, "user1");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER2, "user2");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER3, "user3");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER4, "user4");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER5, "user5");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER6, "user6");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_USER7, "user7");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_NEWPLAYER, "newplayer");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_FIRE, "fire");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_JOIN, "join");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_LEAVE, "leave");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_SPLASH, "splash");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_TRIGGER, "trigger");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_STATECHANGE, "statechange");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_MISSED, "missed");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_BOARDED, "boarded");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_UNBOARDED, "unboarded");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_ARRIVEDWPNT, "arrivedwpnt");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_INITIALIZED, "initialized");
    sithCog_AddIntSymbol(pTable, SITHCOG_MSG_UPDATEWPNTS, "updatewpnts");

    sithCog_AddFloatSymbol(pTable, "global0", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global1", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global2", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global3", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global4", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global5", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global6", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global7", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global8", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global9", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global10", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global11", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global12", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global13", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global14", 0.0f);
    sithCog_AddFloatSymbol(pTable, "global15", 0.0f);
}

void J3DAPI sithCog_AddFloatSymbol(SithCogSymbolTable* pTbl, const char* pName, float value)
{
    SithCogSymbolValue val;
    SithCogSymbol* pSymbol;

    pSymbol = sithCogParse_AddSymbol(pTbl, pName);
    if ( !pSymbol )
    {
#line 2948
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not add symbol %s to table.\n", pName, 0, 0, 0);
        return;
    }

    val.type = SITHCOG_VALUE_FLOAT;
    val.val.floatValue = value;
    sithCogParse_SetSymbolValue(pSymbol, &val);
}

int J3DAPI sithCog_RegisterFunction(SithCogSymbolTable* pTable, SithCogFunctionType pFunction, const char* pName)
{
    SithCogSymbolValue value;
    SithCogSymbol* pSymbol;

    pSymbol = sithCogParse_AddSymbol(pTable, pName);
    if ( !pSymbol )
    {
#line 2986
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not add symbol %s to table.\n", pName, 0, 0, 0);
        return 1;
    }

    value.type = SITHCOG_VALUE_POINTER;
    value.val.pointerValue = (void*)pFunction;
    sithCogParse_SetSymbolValue(pSymbol, &value);
    return 0;
}

int J3DAPI sithCog_IsThingLinked(const SithThing* pThing)
{
    SithCogThingLink* pLink;
    SithCogThingLink* pEndLink;

    pLink    = aThingLinks;
    pEndLink = &aThingLinks[numThingLinks];
    while ( pLink < pEndLink )
    {
        if ( pLink->pThing == pThing )
        {
            return 1;
        }

        ++pLink;
    }

    return 0;
}
