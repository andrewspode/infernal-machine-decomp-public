#ifndef RDROID_RDMODEL3K_H
#define RDROID_RDMODEL3K_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

/*
 * rdModel3K: the retail "Katmai" (Pentium III SSE) mesh renderer at 0x004E1DA0-0x004E2F30 (rdModel3K.c and rdModel3K_cpu.c), compiled by
 * Intel C/C++ 4.0. The five
 * functions below are the retail entry points called by JonesDevDialog_LoadSettings and
 * rdModel3_DrawHNode; the names are descriptive (only the addresses, argument counts and return use are
 * evidenced by those call sites).
 */

int rdModel3K_IsAvailable(void);        // Returns nonzero when the SSE path is available
void rdModel3K_SetEnabled(int bEnable); // Stores the enable flag (only when available)
void rdModel3K_Startup(void);        // Initializes the SSE module
int rdModel3K_IsEnabled(void);        // Returns the enable flag

void J3DAPI rdModel3K_DrawMesh(const rdModel3Mesh* pMesh, const rdMatrix34* pOrient);

J3D_EXTERN_C_END
#endif // RDROID_RDMODEL3K_H
