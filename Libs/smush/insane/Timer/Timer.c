/*
 * insane_timer: INSANE's `TIMER=insane` service (retail 0x004EA7D0-0x004EAE10): up to 19 software timers driven by one
 * time-critical thread that calls `timer_Tick` about every 16 2/3 ms.
 */
#include <windows.h>
#include <setjmp.h>
#include <stdlib.h>
#include "../lec.h"
#include "Timer.h"

typedef struct TimerEntry
{
    TimerCallback pfn;  /* 0x00 (NULL: the slot is free) */
    int interval;       /* 0x04 (ticks) */
    unsigned flags;     /* 0x08 (bit 1: periodic, bit 2: one-shot, bit 3 (4): deferred, bit 4 (8): resync) */
    int param;          /* 0x0C */
    volatile int busy;  /* 0x10 (reentrancy flag shared by the timer thread and the main thread) */
    int elapsed;        /* 0x14 */
    int running;        /* 0x18 */
    int total;          /* 0x1C */
} TimerEntry;

#define TIMER_COUNT 19

int timer_Register(LecShare* pShare, void* pParam);
static void timer_Unregister(void);
static int timer_Startup(void);
static void timer_Shutdown(void);
static int timer_Start(int a, int b);
static DWORD WINAPI timer_Thread(LPVOID pArg);
static void timer_Stop(void);
static void timer_Pause(void);
static void timer_Resume(void);
static int timer_Alloc(TimerCallback pfn, int interval, unsigned flags, int param);
static void timer_Free(int index);
static void timer_SetInterval(int index, int interval);
static void timer_Wait(int ticks);
static int timer_GetTicks(void);
static int timer_GetTickLen(void);
static void timer_Sync(int ticks);
static void timer_Tick(void);
static void timer_Poll(void);
static void timer_RunDeferred(void);

static LecModule timer_module = { timer_Register, timer_Unregister, timer_Startup, timer_Shutdown };
static TimerInterface timer_interface = {
    timer_Alloc, timer_Free, timer_Pause, timer_Resume, timer_SetInterval, timer_Wait, timer_GetTicks, timer_GetTickLen, timer_Sync
};
static int timer_field54D84C = -1;

/* Retail's .bss block (0x017EA028-0x017EA378) is in declaration order, VC6's layout for zero-initialised statics (each
   address measured from the code's references). */
static void* timer_pServices = NULL;
static int timer_rate = 0;
static jmp_buf timer_jmpA = { 0 };
static jmp_buf timer_jmpB = { 0 };
static int timer_ticks = 0;
static LecShare* timer_pShare = NULL;
static int timer_x0B8 = 0;
static int timer_x0BC = 0;
static TimerEntry timer_entries[TIMER_COUNT + 1] = { 0 };
static HANDLE timer_hThread = NULL;
static void* timer_pParam = NULL;
static int timer_tickLen = 0;
static HANDLE timer_hEvent = NULL;
static int timer_reent = 0;
static int timer_inTick = 0;
static int timer_savedReent = 0;
static int timer_refs = 0;
static int timer_paused = 0;
static int timer_x364 = 0;
static volatile int timer_x368 = 0;
static int timer_x36C = 0;
static int timer_x370 = 0;
static int timer_x374 = 0;

int timer_Register(LecShare* pShare, void* pParam)
{
    timer_pShare = pShare;
    timer_pParam = pParam;
    pShare->Publish("LECMODULE=insane_timer", &timer_module);
    timer_pShare->Publish("TIMER=insane", &timer_interface);
    timer_rate = 300;
    return 0;
}

static void timer_Unregister(void)
{
    timer_pShare->Unpublish("TIMER=insane");
    timer_pShare->Unpublish("LECMODULE=insane_timer");
}

static int timer_Startup(void)
{
    timer_pServices = timer_pShare->Subscribe("MEMORY.workspace", timer_pParam);
    if (timer_pServices == NULL) {
        timer_pShare->ReportError("Timer Module: Cannot allocate memory services.");
        return -1;
    }
    return timer_Start(1193180 / timer_rate, 0x10000);
}

static void timer_Shutdown(void)
{
    timer_pShare->Unsubscribe(timer_pServices);
    timer_Stop();
}

static int timer_Start(int a, int b)
{
    int i;
    DWORD id;

    if (timer_refs++ == 0) {
        for (i = 1; i < TIMER_COUNT + 1; i++) {
            timer_entries[i].pfn = NULL;
        }
        timer_tickLen = 0x4DAE;
        timer_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        timer_hThread = CreateThread(NULL, 0, timer_Thread, NULL, 0, &id);
        SetThreadPriority(timer_hThread, THREAD_PRIORITY_TIME_CRITICAL);
        timer_field54D84C = -1;
    }
    return 0;
}

static DWORD WINAPI timer_Thread(LPVOID pArg)
{
    CRITICAL_SECTION cs;
    ldiv_t step;
    int next;
    int frac;
    int delta;
    int timeout;

    InitializeCriticalSection(&cs);
    step = ldiv(1000, 60);
    frac = 0;
    timeout = 0;
    next = timeGetTime();
    while (WaitForSingleObject(timer_hEvent, timeout) != 0) {
        EnterCriticalSection(&cs);
        timer_Tick();
        frac += step.rem;
        next += step.quot;
        if (frac >= 60) {
            frac -= 60;
            next++;
        }
        LeaveCriticalSection(&cs);
        delta = next - timeGetTime();
        timeout = delta > 0 ? delta : 0;
    }
    DeleteCriticalSection(&cs);
    return 0;
}

static void timer_Stop(void)
{
    if (--timer_refs == 0) {
        SetEvent(timer_hEvent);
        WaitForSingleObject(timer_hThread, INFINITE);
        CloseHandle(timer_hThread);
        CloseHandle(timer_hEvent);
    }
}

static void timer_Pause(void)
{
    timer_paused++;
}

static void timer_Resume(void)
{
    timer_paused--;
}

static int timer_Alloc(TimerCallback pfn, int interval, unsigned flags, int param)
{
    int i;

    for (i = 1; i < TIMER_COUNT + 1; i++) {
        if (timer_entries[i].pfn == NULL) {
            timer_entries[i].busy = 0;
            timer_entries[i].elapsed = 0;
            timer_entries[i].param = param;
            timer_entries[i].flags = flags;
            timer_entries[i].interval = interval;
            timer_entries[i].pfn = pfn;
            timer_entries[i].running = 0;
            timer_entries[i].total = 0;
            return i;
        }
    }
    return 0;
}

static void timer_Free(int index)
{
    if (index > 0 && index < TIMER_COUNT + 1) {
        timer_entries[index].pfn = NULL;
    }
}

static void timer_SetInterval(int index, int interval)
{
    if (index > 0 && index < TIMER_COUNT + 1) {
        timer_entries[index].interval = interval;
        timer_entries[index].elapsed = 0;
    }
}

static void timer_Wait(int ticks)
{
    int start = timer_ticks;
    int end = start + ticks;
    int cur = start;

    while (cur >= start && cur < end) {
        timer_GetTicks();
        cur = timer_ticks;
    }
}

static int timer_GetTicks(void)
{
    return timer_ticks;
}

static int timer_GetTickLen(void)
{
    return timer_tickLen;
}

static void timer_Sync(int ticks)
{
    int i;

    for (i = 1; i < TIMER_COUNT + 1; i++) {
        TimerEntry* p = &timer_entries[i];

        if (p->pfn && (p->flags & 8)) {
            if (ticks > 0) {
                p->elapsed += ticks - p->total;
            }
            p->total = 0;
        }
    }
}

static void timer_Tick(void)
{
    int i;

    timer_ticks += timer_tickLen;
    if (timer_paused == 0) {
        timer_savedReent = timer_reent;
        timer_inTick = 1;
        timer_reent = 0;
        for (i = 1; i < TIMER_COUNT + 1; i++) {
            int step = timer_tickLen;

            if (timer_entries[i].pfn) {
                timer_entries[i].elapsed += step;
                timer_entries[i].total += step;
                if ((timer_entries[i].flags & 5) == 1 && timer_entries[i].interval != 0) {
                    while (timer_entries[i].elapsed >= timer_entries[i].interval) {
                        timer_entries[i].elapsed -= timer_entries[i].interval;
                        if (timer_entries[i].flags & 2) {
                            timer_entries[i].interval = 0;
                            timer_entries[i].pfn(i, timer_entries[i].param);
                            break;
                        }
                        timer_entries[i].pfn(i, timer_entries[i].param);
                    }
                }
            }
        }
        for (i = 1; i < TIMER_COUNT + 1; i++) {
            if (timer_entries[i].pfn && !(timer_entries[i].flags & 5) && timer_entries[i].interval != 0 && timer_entries[i].busy == 0) {
                timer_entries[i].busy = 1;
                while (timer_entries[i].elapsed >= timer_entries[i].interval) {
                    timer_entries[i].elapsed -= timer_entries[i].interval;
                    if (timer_entries[i].flags & 2) {
                        timer_entries[i].interval = 0;
                        timer_entries[i].pfn(i, timer_entries[i].param);
                        break;
                    }
                    timer_entries[i].pfn(i, timer_entries[i].param);
                }
                timer_entries[i].busy = 0;
            }
        }
        timer_inTick = 0;
        timer_reent = timer_savedReent;
        if (timer_x368 == 0 && timer_x364 == 0) {
            timer_x368 = 1;
            timer_Poll();
            timer_x368 = 0;
        }
    }
}

static void timer_Poll(void)
{
    if (timer_x374 == 0 || --timer_x374 == 0) {
        if (setjmp(timer_jmpA) == 0) {
            if (timer_x36C) {
                timer_x36C = 0;
                timer_x370 = 1;
                longjmp(timer_jmpB, 1);
            } else {
                timer_x36C = 0;
                timer_x370 = 1;
            }
            timer_RunDeferred();
            timer_x370 = 0;
            timer_x0B8 = timer_x0BC;
            longjmp(timer_jmpA, 2);
        }
        timer_reent = 0;
    }
}

static void timer_RunDeferred(void)
{
    int i;

    for (i = 1; i < TIMER_COUNT + 1; i++) {
        timer_entries[i].running = 1;
        if (!timer_entries[i].pfn || (timer_entries[i].flags & 5) != 4 || timer_entries[i].interval == 0) {
            timer_entries[i].running = 0;
            continue;
        }
        while (timer_entries[i].elapsed >= timer_entries[i].interval) {
            timer_entries[i].elapsed -= timer_entries[i].interval;
            if (timer_entries[i].flags & 2) {
                timer_entries[i].interval = 0;
                timer_entries[i].pfn(i, timer_entries[i].param);
                break;
            }
            timer_entries[i].pfn(i, timer_entries[i].param);
        }
        timer_entries[i].running = 0;
    }
}
