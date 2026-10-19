#include "rdFace.h"

#include <rdroid/Main/rdroid.h>

#include <std/General/std.h>
#include <std/General/stdMemory.h>

/* Linker-discarded (no caller in either image): the Indy debug build's rdFace_New (0x00527A30) and rdFace_Free (0x00527B4C),
   restored from its code; Jedi Knight's rdFace.c has New, NewEntry, Free, FreeEntry in this order. */
rdFace* J3DAPI rdFace_New(void)
{
    rdFace* pFace;

#line 61
    pFace = (rdFace*)STDMALLOC(sizeof(rdFace));
    if ( !pFace )
    {
#line 64
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for face.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdFace_NewEntry(pFace);
#line 71
    stdPrintf(rdroid_pHS->pDebugPrint, J3D_FILE, __LINE__, "New face created.\n", 0, 0, 0, 0);
    return pFace;
}

int J3DAPI rdFace_NewEntry(rdFace* pFace)
{
    pFace->num   = 0;
    pFace->flags = 0;

    pFace->numVertices  = 0;
    pFace->aVertices    = NULL;
    pFace->aTexVertices = NULL;

    pFace->pMaterial = NULL;
    pFace->matCelNum = -1;

    memset(&pFace->normal, 0, sizeof(pFace->normal));
    memset(&pFace->texVertOffset, 0, sizeof(pFace->texVertOffset));

    pFace->extraLight.red   = 0.0f;
    pFace->extraLight.green = 0.0f;
    pFace->extraLight.blue  = 0.0f;
    pFace->extraLight.alpha = 0.0f;
    return 1;
}

void J3DAPI rdFace_Free(rdFace* pFace)
{
    if ( pFace )
    {
        rdFace_FreeEntry(pFace);
        stdMemory_Free(pFace);
    }
    else
    {
#line 136
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL face ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdFace_FreeEntry(rdFace* pFace)
{
    if ( pFace->aVertices ) {
        stdMemory_Free(pFace->aVertices);
    }

    if ( pFace->aTexVertices ) {
        stdMemory_Free(pFace->aTexVertices);
    }
}