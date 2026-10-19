#ifndef SITH_SITHEVENT_H
#define SITH_SITHEVENT_H
#include <j3dcore/j3d.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

extern SithEvent* sithEvent_pFirstQueuedEvent;

int J3DAPI sithEvent_Startup(void);
void J3DAPI sithEvent_Shutdown(void);

void J3DAPI sithEvent_Open(void);
void J3DAPI sithEvent_Close(void);

void J3DAPI sithEvent_Reset(void);

/**
 * Registers new task for which events can be created.
 * If task is already registered it will override it.
 *
 * @param taskId
 * @param pfProcess - Function which processes the task event
 * @param frequency - The interval frequency when `startMode` is SITHEVENT_TASKINTERVAL. Can be 0.
 * @param startMode
 * @return
 */
int J3DAPI sithEvent_RegisterTask(size_t taskId, SithEventProcess pfProcess, uint32_t frequency, SithEventTaskMode startMode);

/**
 * Creates new event for registered task.
 * If the task is not registered it will assert in the `sithEvent_Process`
 *
 * @param taskId - Task ID for which the event is created
 * @param params - Event parameters
 * @param when   - MSec when the event should execute from current time.
 */
int J3DAPI sithEvent_CreateEvent(size_t taskId, SithEventParams* params, uint32_t when);
void J3DAPI sithEvent_FreeEvent(SithEvent* pEvent);

void J3DAPI sithEvent_Process(void);

J3D_EXTERN_C_END
#endif // SITH_SITHEVENT_H
