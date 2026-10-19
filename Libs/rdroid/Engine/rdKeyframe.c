#include "rdKeyframe.h"

#include <rdroid/Main/rdroid.h>

#include <std/General/std.h>
#include <std/General/stdConffile.h>
#include <std/General/stdMemory.h>

#include <stdio.h>
#include <string.h>

// The unloader hook (debug build 0x0156C6DC, next to the loader 0x0156C6D8) is retail's unreferenced 4 bytes at
// 0x014DD994 after the loader: only the discarded RegisterUnloader and Free use it.
typedef void (J3DAPI* rdKeyframeUnloadFunc)(rdKeyframe* pKeyframe);

rdKeyframeLoadFunc rdKeyframe_pKeyframeLoader = 0;
rdKeyframeUnloadFunc rdKeyframe_pKeyframeUnloader = 0;

void J3DAPI rdKeyframe_Free(rdKeyframe* pKeyframe);

rdKeyframeLoadFunc J3DAPI rdKeyframe_RegisterLoader(rdKeyframeLoadFunc pFunc)
{
    rdKeyframeLoadFunc pPrevFunc = rdKeyframe_pKeyframeLoader;
    rdKeyframe_pKeyframeLoader = pFunc;
    return pPrevFunc;
}

// Linker-discarded in retail; body from the debug build (0x00517B9C), Jedi Knight's name.
rdKeyframeUnloadFunc J3DAPI rdKeyframe_RegisterUnloader(rdKeyframeUnloadFunc pFunc)
{
    rdKeyframeUnloadFunc pPrevFunc = rdKeyframe_pKeyframeUnloader;
    rdKeyframe_pKeyframeUnloader = pFunc;
    return pPrevFunc;
}

void J3DAPI rdKeyframe_NewEntry(rdKeyframe* pKeyframe)
{
    memset(pKeyframe, 0, sizeof(rdKeyframe));
    strncpy(pKeyframe->aName, "UNKNOWN", sizeof(pKeyframe->aName) - 1);
    pKeyframe->aName[sizeof(pKeyframe->aName) - 1] = 0;
}

// Linker-discarded in retail; body from the debug build (0x00517BEA): the registered loader, else
// allocate (159) and LoadEntry; the allocation failure jumps to the line-171 print and the LoadEntry failure past it to
// the shared Free, both laid after the success return. The first J3D_FILE use of
// retail's /Gf run (the filename literal ahead of every LoadEntry string) is this STDMALLOC.
rdKeyframe* J3DAPI rdKeyframe_Load(const char* pFilename)
{
    rdKeyframe* pKeyframe;

    pKeyframe = NULL;
    if ( rdKeyframe_pKeyframeLoader )
    {
        return rdKeyframe_pKeyframeLoader(pFilename);
    }

#line 159
    pKeyframe = (rdKeyframe*)STDMALLOC(sizeof(rdKeyframe));
    if ( !pKeyframe )
    {
        goto alloc_error;
    }

    if ( rdKeyframe_LoadEntry(pFilename, pKeyframe) )
    {
        return pKeyframe;
    }

    goto error;

alloc_error:
#line 171
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for keyframe '%s'.\n", pFilename, 0, 0, 0);

error:
    rdKeyframe_Free(pKeyframe);
    return NULL;
}

// Text of the debug build (debug 0x517c87, /Od): one test per `if`, the open (398),
// EOF (403), syntax (408) and allocation (413) prints as blocks after the success
// return, the middle two ending `goto error` on the shared Close. Exception (C) labels
// plus the debug-route ones.
int J3DAPI rdKeyframe_LoadEntry(const char* pFilename, rdKeyframe* pKeyframe)
{
    float fps;
    size_t numJoints;
    size_t numMarkers;
    size_t i;
    size_t numNodes;
    size_t nodeNum;
    int nodeIdx;
    size_t numEntries;
    rdKeyframeNode* pNode;
    size_t entryNum;
    rdKeyframeNodeEntry* pEntry;
    int num;
    float x, y, z, pitch, yaw, roll;
    float dx, dy, dz, dpitch, dyaw, droll;
    char aMeshName[64];

    rdKeyframe_NewEntry(pKeyframe);
    strncpy(pKeyframe->aName, stdFileFromPath(pFilename), sizeof(pKeyframe->aName) - 1);
    pKeyframe->aName[sizeof(pKeyframe->aName) - 1] = 0;

    if ( !stdConffile_Open(pFilename) )
    {
        goto open_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " flags %x", &pKeyframe->flags) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " type %x", &pKeyframe->type) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " frames %d", &pKeyframe->numFrames) != 1 )
    {
        goto syntax_error;
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " fps %f", &fps) != 1 )
    {
        goto syntax_error;
    }

    pKeyframe->fps = fps;
    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " joints %d", &numJoints) != 1 )
    {
        goto syntax_error;
    }

#line 263
    pKeyframe->aNodes = (rdKeyframeNode*)STDMALLOC(sizeof(rdKeyframeNode) * numJoints);
    if ( !pKeyframe->aNodes )
    {
        goto alloc_error;
    }

    memset(pKeyframe->aNodes, 0, sizeof(rdKeyframeNode) * numJoints);
    pKeyframe->numJoints = numJoints;
    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
    {
        goto syntax_error;
    }

    // The debug code tests `je markers; jmp nodes` (0x517e98): an empty then-arm
    // (a single site whose jump target is the end of the following block).
    if ( strcmp(std_genBuffer, "markers") )
    {
    }
    else
    {
        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_aLine, " markers %d", &numMarkers) != 1 )
        {
            goto syntax_error;
        }

        if ( numMarkers > RDKEYFRAME_MAX_MARKERS )
        {
            goto syntax_error;
        }

        pKeyframe->numMarkers = numMarkers;
        for ( i = 0; i < numMarkers; ++i )
        {
            if ( !stdConffile_ReadLine() )
            {
                goto syntax_error;
            }

            if ( sscanf(stdConffile_aLine, "%f %d", &pKeyframe->aMarkerFrames[i], &pKeyframe->aMarkerTypes[i]) != 2 )
            {
                goto syntax_error;
            }
        }

        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_aLine, " section: %s", std_genBuffer) != 1 )
        {
            goto syntax_error;
        }
    }

    if ( !stdConffile_ReadLine() )
    {
        goto syntax_error;
    }

    if ( sscanf(stdConffile_aLine, " nodes %d", &numNodes) != 1 )
    {
        goto syntax_error;
    }

    for ( nodeNum = 0; nodeNum < numNodes; ++nodeNum )
    {
        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_aLine, " node %d", &nodeIdx) != 1 )
        {
            goto syntax_error;
        }

        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_aLine, " mesh name %s", aMeshName) != 1 )
        {
            goto syntax_error;
        }

        pNode = &pKeyframe->aNodes[nodeIdx];
        strncpy(pNode->aMeshName, aMeshName, sizeof(pNode->aMeshName) - 1);
        pNode->aMeshName[sizeof(pNode->aMeshName) - 1] = 0;

        if ( !stdConffile_ReadLine() )
        {
            goto syntax_error;
        }

        if ( sscanf(stdConffile_aLine, " entries %d", &numEntries) != 1 )
        {
            goto syntax_error;
        }

        pNode->nodeNum = nodeIdx;
        pNode->numEntries = numEntries;
#line 351
        pNode->aEntries = (rdKeyframeNodeEntry*)STDMALLOC(sizeof(rdKeyframeNodeEntry) * pNode->numEntries);
        if ( !pNode->aEntries )
        {
            goto alloc_error;
        }

        pEntry = pNode->aEntries;
        for ( entryNum = 0; entryNum < pNode->numEntries; ++entryNum )
        {
            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " %d: %f %x %f %f %f %f %f %f", &num, &pEntry->frame, &pEntry->flags, &x, &y, &z, &pitch, &yaw, &roll) != 9 )
            {
                goto syntax_error;
            }

            pEntry->pos.x = x;
            pEntry->pos.y = y;
            pEntry->pos.z = z;
            pEntry->rot.pitch = pitch;
            pEntry->rot.yaw = yaw;
            pEntry->rot.roll = roll;

            if ( !stdConffile_ReadLine() )
            {
                goto eof_error;
            }

            if ( sscanf(stdConffile_aLine, " %f %f %f %f %f %f", &dx, &dy, &dz, &dpitch, &dyaw, &droll) != 6 )
            {
                goto syntax_error;
            }

            pEntry->dpos.x = dx;
            pEntry->dpos.y = dy;
            pEntry->dpos.z = dz;
            pEntry->drot.pitch = dpitch;
            pEntry->drot.yaw = dyaw;
            pEntry->drot.roll = droll;
            ++pEntry;
        }
    }

    stdConffile_Close();
    return 1;

open_error:
#line 398
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid load filename '%s'.\n", pFilename, 0, 0, 0);
    return 0;

eof_error:
#line 403
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Unexpected EOF in '%s'\n", pFilename, 0, 0, 0);
    goto error;

syntax_error:
#line 408
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Invalid syntax '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);
    goto error;

alloc_error:
#line 413
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error: Bad memory allocation for '%s' in '%s'\n", stdConffile_aLine, pFilename, 0, 0);

error:
    stdConffile_Close();
    return 0;
}

/* Linker-discarded (no retail caller): the Indy debug build's keyframe text writer (0x00518327, between LoadEntry and Free),
   restored in full from its code; Jedi Knight's rdKeyframe_Write(fpath, keyframe, creator) is the counterpart (name and
   signature). Its /Gf literals are COMDATs and went with it. */
int J3DAPI rdKeyframe_Write(const char* pFilename, const rdKeyframe* pKeyframe, const char* pCreatedFrom)
{
    tFileHandle fh;
    size_t markerIdx;
    const rdKeyframeNodeEntry* pCurEntry;
    size_t numNodes;
    const rdKeyframeNode* pCurNode;
    size_t nodeNum;
    size_t entryIdx;

    fh = rdroid_pHS->pFileOpen(pFilename, "wt+");
    if ( !fh )
    {
        return 0;
    }

    rdroid_pHS->pFilePrintf(fh, "# KEYFRAME '%s' created from '%s'\n\n", pKeyframe->aName, pCreatedFrom);
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: HEADER\n\n");
    rdroid_pHS->pFilePrintf(fh, "FLAGS  0x%04x\n", pKeyframe->flags);
    rdroid_pHS->pFilePrintf(fh, "TYPE   0x%X\n", pKeyframe->type);
    rdroid_pHS->pFilePrintf(fh, "FRAMES %d\n", pKeyframe->numFrames);
    rdroid_pHS->pFilePrintf(fh, "FPS    %.3f\n", pKeyframe->fps);
    rdroid_pHS->pFilePrintf(fh, "JOINTS %d\n", pKeyframe->numJoints);

    if ( pKeyframe->numMarkers )
    {
        rdroid_pHS->pFilePrintf(fh, "\n\n");
        rdroid_pHS->pFilePrintf(fh, "###############\n");
        rdroid_pHS->pFilePrintf(fh, "SECTION: MARKERS\n\n");
        rdroid_pHS->pFilePrintf(fh, "MARKERS %d\n\n", pKeyframe->numMarkers);
        for ( markerIdx = 0; markerIdx < pKeyframe->numMarkers; ++markerIdx )
        {
            rdroid_pHS->pFilePrintf(fh, "%f %d\n", pKeyframe->aMarkerFrames[markerIdx], pKeyframe->aMarkerTypes[markerIdx]);
        }
    }

    rdroid_pHS->pFilePrintf(fh, "\n\n");
    rdroid_pHS->pFilePrintf(fh, "###############\n");
    rdroid_pHS->pFilePrintf(fh, "SECTION: KEYFRAME NODES\n\n");

    numNodes = 0;
    pCurNode = pKeyframe->aNodes;
    for ( nodeNum = 0; nodeNum < pKeyframe->numJoints; ++nodeNum )
    {
        if ( pCurNode->numEntries > 0 )
        {
            ++numNodes;
        }

        ++pCurNode;
    }

    rdroid_pHS->pFilePrintf(fh, "NODES %d\n\n", numNodes);

    pCurNode = pKeyframe->aNodes;
    for ( nodeNum = 0; nodeNum < pKeyframe->numJoints; ++nodeNum )
    {
        if ( pCurNode->numEntries > 0 )
        {
            rdroid_pHS->pFilePrintf(fh, "NODE    %d\n", nodeNum);
            rdroid_pHS->pFilePrintf(fh, "MESH NAME %s\n", pCurNode->aMeshName);
            rdroid_pHS->pFilePrintf(fh, "ENTRIES %d\n", pCurNode->numEntries);
            rdroid_pHS->pFilePrintf(fh, "\n");
            rdroid_pHS->pFilePrintf(fh, "# num:   frame:   flags:           x:           y:           z:           p:           y:           r:\n");
            rdroid_pHS->pFilePrintf(fh, "#                                 dx:          dy:          dz:          dp:          dy:          dr:\n");

            pCurEntry = pCurNode->aEntries;
            for ( entryIdx = 0; entryIdx < pCurNode->numEntries; ++entryIdx )
            {
                rdroid_pHS->pFilePrintf(fh, " %3d:  %7d   0x%04x %12.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n", entryIdx, (int)pCurEntry->frame, pCurEntry->flags, pCurEntry->pos.x, pCurEntry->pos.y, pCurEntry->pos.z, pCurEntry->rot.x, pCurEntry->rot.y, pCurEntry->rot.z);
                rdroid_pHS->pFilePrintf(fh, " %35.8f %12.8f %12.8f %12.8f %12.8f %12.8f\n", pCurEntry->dpos.x, pCurEntry->dpos.y, pCurEntry->dpos.z, pCurEntry->drot.x, pCurEntry->drot.y, pCurEntry->drot.z);
                ++pCurEntry;
            }

            rdroid_pHS->pFilePrintf(fh, "\n");
        }

        ++pCurNode;
    }

    rdroid_pHS->pFileClose(fh);
    return 1;
}

// Linker-discarded in retail; body from the debug build (0x00518783, prints at 573/576), Jedi Knight's name.
void J3DAPI rdKeyframe_Free(rdKeyframe* pKeyframe)
{
    if ( pKeyframe )
    {
        if ( rdKeyframe_pKeyframeUnloader )
        {
            rdKeyframe_pKeyframeUnloader(pKeyframe);
            return;
        }

        rdKeyframe_FreeEntry(pKeyframe);
        stdMemory_Free(pKeyframe);
#line 573
        stdPrintf(rdroid_pHS->pDebugPrint, J3D_FILE, __LINE__, "Keyframe successfully freed.\n", 0, 0, 0, 0);
        return;
    }

#line 576
    stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL keyframe ptr.\n", 0, 0, 0, 0);
}

void J3DAPI rdKeyframe_FreeEntry(rdKeyframe* pKeyframe)
{
    size_t i;
    rdKeyframeNode* pCurNode;

    if ( pKeyframe->aNodes )
    {
        pCurNode = pKeyframe->aNodes;
        for ( i = 0; i < pKeyframe->numJoints; ++i )
        {
            if ( pCurNode->aEntries )
            {
                stdMemory_Free(pCurNode->aEntries);
                pCurNode->aEntries = NULL;
            }
            ++pCurNode;
        }

        stdMemory_Free(pKeyframe->aNodes);
        pKeyframe->aNodes = NULL;
    }
}
