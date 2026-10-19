#include "rdCanvas.h"

#include <rdroid/Main/rdroid.h>

#include <std/General/stdMemory.h>

rdCanvas* J3DAPI rdCanvas_New(int flags, tVBuffer* pVBuffer, int left, int top, int right, int bottom)
{
#line 65
    rdCanvas* pCanvas = (rdCanvas*)STDMALLOC(sizeof(rdCanvas));
    if ( !pCanvas )
    {
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for canvas.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdCanvas_NewEntry(pCanvas, flags, pVBuffer, left, top, right, bottom);
    return pCanvas;
}

int J3DAPI rdCanvas_NewEntry(rdCanvas* pCanvas, int flags, tVBuffer* pVBuffer, int left, int top, int right, int bottom)
{
    pCanvas->flags    = flags;
    pCanvas->pVBuffer = pVBuffer;
    if ( (flags & 0x01) != 0 )
    {
        pCanvas->rect.left   = left;
        pCanvas->rect.top    = top;
        pCanvas->rect.right  = right;
        pCanvas->rect.bottom = bottom;
    }
    else
    {
        pCanvas->rect.left   = 0;
        pCanvas->rect.top    = 0;
        pCanvas->rect.right  = pVBuffer->rasterInfo.width - 1;
        pCanvas->rect.bottom = pVBuffer->rasterInfo.height - 1;
    }

    // The debug build's text (no float locals, `/ 2.0f`): every float value goes
    // through the SDK's D3DVAL (`((float)(val))`) and
    // the literal-scaled quotient in the sum is its own node.
    pCanvas->center.x = D3DVAL(D3DVAL(pCanvas->rect.left) + (D3DVAL(pCanvas->rect.right - pCanvas->rect.left + 1) / 2.0f));
    pCanvas->center.y = D3DVAL(D3DVAL(pCanvas->rect.top) + (D3DVAL(pCanvas->rect.bottom - pCanvas->rect.top + 1) / 2.0f));
    return 1;
}

void J3DAPI rdCanvas_Free(rdCanvas* pCanvas)
{
#line 147
    if ( pCanvas )
    {
        rdCanvas_FreeEntry(pCanvas);
        stdMemory_Free(pCanvas);
    }
    else
    {
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL Canvas ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdCanvas_FreeEntry(rdCanvas* pCanvas)
{
}
