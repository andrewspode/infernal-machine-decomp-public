/*
 * rdModel3K_cpu: the Katmai module's CPU/OS capability probes and enable flags, retail 0x004E2D80-0x004E2F30.
 *
 * NOT MSVC output: Intel C/C++ 4.0 (build 99100, `-O2 -QxM`, like rdModel3K.c). Retail links this as a
 * second Intel object behind rdModel3K.obj: the two SEH scope tables of the probes sit in .data1 at
 * 0x018600CC/0x018600E4, after rdModel3K.obj's string COMDATs, which one Intel compile cannot produce
 * (ICL emits `try_info` into the object's plain _DATA1 section). The boundary at 0x004E2D80 is inferred,
 * and so is the file name (retail has no string naming this source).
 *
 */
#include <excpt.h>

#include "rdModel3K.h"

static int rdModel3K_avail; // retail 0x017E4680
static int rdModel3K_enabled; // retail 0x017E4684

// The three CPU probes, retail 0x004E2D80 / 0x004E2E00 / 0x004E2E80. Their `pushad`/`popad` wrapper, an
// asm block that updates a C local, and the absence of any cpuid intrinsic in MSVC 5/6 and ICL 4.0 make them
// hand-written `__asm` (these three functions only). Names are
// inferred (retail has none).

// Retail 0x004E2D80: does the CPU execute `cpuid`? (an SEH-guarded probe; `0` if it faults)
int J3DAPI rdModel3K_HasCpuid(void)
{
    int ok = 1;
    __try {
        __asm {
            pushad
            cpuid
            popad
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = 0;
    }
    return ok;
}

// Retail 0x004E2E00: does the OS accept an SSE instruction (`orps`)? (`0` if it faults)
int J3DAPI rdModel3K_OsSupportsSSE(void)
{
    int ok = 1;
    __try {
        __asm {
            pushad
            orps xmm1, xmm1
            popad
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ok = 0;
    }
    return ok;
}

// Retail 0x004E2E80: cpuid leaf 1, EDX bit 25 (SSE). Retail's `mov ecx, 0` sits inside the `HasCpuid` branch,
// so the result is uninitialised when `cpuid` is absent (ecx then holds whatever the callee left).
int J3DAPI rdModel3K_CpuHasKatmai(void)
{
    int feature = 0;
    int result;
    if (rdModel3K_HasCpuid()) {
        result = 0;
        __asm {
            pushad
            xor eax, eax
            inc eax
            cpuid
            and edx, 0x2000000
            je skip
            inc feature
        skip:
            popad
        }
        if (feature)
            result = 1;
    }
    return result;
}

int J3DAPI rdModel3K_IsAvailable(void)
{
    if (rdModel3K_CpuHasKatmai() && rdModel3K_OsSupportsSSE())
        rdModel3K_avail = 1;
    return rdModel3K_avail;
}

void J3DAPI rdModel3K_SetEnabled(int bEnable)
{
    if (rdModel3K_avail)
        rdModel3K_enabled = bEnable;
    else
        rdModel3K_enabled = 0;
}

int J3DAPI rdModel3K_IsEnabled(void)
{
    return rdModel3K_enabled;
}
