#include "sithEvent.h"

#include <sith/Gameplay/sithTime.h>

#include <std/General/stdUtil.h>

SithEvent aEventBuffer[256] = { 0 };
size_t aFreeEventBufferIdxs[256] = { 0 };
SithEventTask aTasks[5] = { 0 };
size_t numFreeEventBuffers = 0;
SithEvent* sithEvent_pFirstQueuedEvent = NULL;
int bEventStartup = 0;
int bOpen = 0;

void J3DAPI sithEvent_ResetFreeBufferTable(void);
SithEvent* J3DAPI sithEvent_Create(void);
void J3DAPI sithEvent_AddEvent(SithEvent* pEvent);

int J3DAPI sithEvent_Startup(void)
{
    if ( !bEventStartup )
    {
        memset(aTasks, 0, sizeof(aTasks));
        sithEvent_Reset();

        bEventStartup = 1;
        return 1;
    }

#line 97
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already initialized!\n", 0, 0, 0, 0);
    return 0;
}

void J3DAPI sithEvent_Shutdown(void)
{
    if ( bEventStartup )
    {
        bEventStartup = 0;
        return;
    }

#line 121
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: System already shutdown!\n", 0, 0, 0, 0);
}

void J3DAPI sithEvent_Open(void)
{
    if ( !bOpen )
    {
        bOpen = 1;
    }
}

void J3DAPI sithEvent_Close(void)
{
    if ( bOpen )
    {
        sithEvent_Reset();
        bOpen = 0;
    }
}

void J3DAPI sithEvent_Reset(void)
{
    memset(aEventBuffer, 0, sizeof(aEventBuffer));
    sithEvent_ResetFreeBufferTable();
    sithEvent_pFirstQueuedEvent = NULL;
}

int J3DAPI sithEvent_CreateEvent(size_t taskId, SithEventParams* params, uint32_t when)
{
    SithEvent* pEvent;

#line 182
    SITH_ASSERTREL((taskId > 0) && (taskId < 5) && (when >= 0));

    pEvent = sithEvent_Create();
    if ( !pEvent )
    {
#line 188
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of space in event buffer array.  Event lost.\n", 0, 0, 0, 0);
        return 0;
    }

    pEvent->msecEventTime = sithTime_msecGameTime + when;
    pEvent->taskNum       = taskId;
    memcpy(&pEvent->params, params, sizeof(pEvent->params));
    sithEvent_AddEvent(pEvent);
    return 1;
}

void J3DAPI sithEvent_FreeEvent(SithEvent* pEvent)
{
    int index;

#line 227
    SITH_ASSERTREL((numFreeEventBuffers < (256)));
#line 228
    SITH_ASSERTREL(pEvent != NULL);

    index = pEvent - aEventBuffer;
#line 232
    SITH_ASSERTREL((index >= 0) && (index < 256));

    memset(pEvent, 0, sizeof(SithEvent));
    aFreeEventBufferIdxs[numFreeEventBuffers++] = index;
}

int J3DAPI sithEvent_RegisterTask(size_t taskId, SithEventProcess pfProcess, uint32_t frequency, SithEventTaskMode startMode)
{
#line 270
    SITH_ASSERTREL(bEventStartup);
#line 271
    SITH_ASSERTREL((taskId > 0) && (taskId < 5) && (frequency >= 0));
#line 272
    SITH_ASSERTREL((startMode >= SITHEVENT_TASKDISABLED) && (startMode <= SITHEVENT_TASKONDEMAND));

    aTasks[taskId].pfProcess            = pfProcess;
    aTasks[taskId].msecLastIntervalTime = sithTime_msecGameTime;
    aTasks[taskId].unknown10            = 0;
    aTasks[taskId].msecFrequency        = frequency;
    aTasks[taskId].startMode            = startMode;
    return 1;
}

void J3DAPI sithEvent_Process(void)
{
    size_t processed = 0; // Useless
    size_t taskNum;
    SithEvent* event;
    int bProcessed;

    // Process tasks
    for ( taskNum = 1; taskNum < STD_ARRAYLEN(aTasks); ++taskNum )
    {
        if ( aTasks[taskNum].startMode == SITHEVENT_TASKINTERVAL
            && sithTime_msecGameTime - aTasks[taskNum].msecLastIntervalTime > aTasks[taskNum].msecFrequency )
        {
            bProcessed = aTasks[taskNum].pfProcess(sithTime_msecGameTime - aTasks[taskNum].msecLastIntervalTime, NULL);
            if ( bProcessed )
            {
                ++processed;
                aTasks[taskNum].msecLastIntervalTime = sithTime_msecGameTime;
            }
        }
    }

    // Process queued events
    while ( (event = sithEvent_pFirstQueuedEvent) != NULL && event->msecEventTime < sithTime_msecGameTime )
    {
        taskNum = sithEvent_pFirstQueuedEvent->taskNum;
#line 330
        SITH_ASSERTREL(aTasks[taskNum].pfProcess);

        sithEvent_pFirstQueuedEvent = event->pNextEvent;
        aTasks[taskNum].pfProcess(0, &event->params);

        sithEvent_FreeEvent(event);
    }
}

void J3DAPI sithEvent_ResetFreeBufferTable(void)
{
    int i;

    for ( i = STD_ARRAYLEN(aFreeEventBufferIdxs); i >= 0; --i )
    {
        aFreeEventBufferIdxs[STD_ARRAYLEN(aFreeEventBufferIdxs) - i] = i - 1;
    }

    numFreeEventBuffers = STD_ARRAYLEN(aFreeEventBufferIdxs);
}

SithEvent* J3DAPI sithEvent_Create(void)
{
    if ( numFreeEventBuffers == 0 )
    {
#line 396
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Out of event buffer space.\n", 0, 0, 0, 0);
        return NULL;
    }

#line 400
    SITH_ASSERT(aFreeEventBufferIdxs[numFreeEventBuffers - 1] >= 0);
    --numFreeEventBuffers;
    return &aEventBuffer[aFreeEventBufferIdxs[numFreeEventBuffers]];
}

void J3DAPI sithEvent_AddEvent(SithEvent* pEvent)
{
    SithEvent* pNextEvent = sithEvent_pFirstQueuedEvent;
    SithEvent* pPrevEvent = NULL;
    uint32_t eventMsec = pEvent->msecEventTime;

#line 429
    SITH_ASSERTREL(pEvent != NULL);

    while ( pNextEvent && pNextEvent->msecEventTime <= eventMsec )
    {
        pPrevEvent = pNextEvent;
        pNextEvent = pNextEvent->pNextEvent;
    }

    if ( !pPrevEvent )
    {
        sithEvent_pFirstQueuedEvent = pEvent;
    }
    else
    {
        pPrevEvent->pNextEvent = pEvent;
    }

    pEvent->pNextEvent = pNextEvent;
}
