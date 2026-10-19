#include "sithCogExec.h"

#include <sith/Cog/sithCogParse.h>
#include <sith/Devices/sithConsole.h>
#include <sith/World/sithWorld.h>

#include <rdroid/Math/rdVector.h>

#include <sound/Sound.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

int J3DAPI sithCogExec_Execute(SithCog* pCog)
{
    SithCogScript* pScript;
    int opcode;

#line 92
    SITH_ASSERTREL(pCog != NULL);

    pScript = pCog->pScript;
#line 97
    SITH_ASSERTREL(pCog->execPos < pScript->codeSize);

    pCog->status = SITHCOG_STATUS_EXECUTING;
    while ( pCog->status == SITHCOG_STATUS_EXECUTING )
    {
        opcode = sithCogExec_GetOpCode(pCog);
        switch ( opcode )
        {
            case SITHCOGEXEC_OPCODE_NOP:
#line 113
                SITH_ASSERTREL(0);
                break;

            case SITHCOGEXEC_OPCODE_ASSIGN:
                sithCogExec_AssignOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_RET:
                sithCogExec_RetOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_CALLFUNC:
                sithCogExec_FunctionCallOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_PUSHINT:
            case SITHCOGEXEC_OPCODE_PUSHFLOAT:
            case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
                sithCogExec_PushNumberOp(pCog, opcode);
                break;

            case SITHCOGEXEC_OPCODE_PUSHVECTOR:
                sithCogExec_PushVectorOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_ARRAYIDX:
                sithCogExec_ArrayOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_JNZ:
                sithCogExec_JmpnzOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_JZ:
                sithCogExec_JmpzOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_JMP:
                sithCogExec_JmpOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_CALL:
                sithCogExec_CallOp(pCog);
                break;

            case SITHCOGEXEC_OPCODE_CMPFALSE:
            case SITHCOGEXEC_OPCODE_NEG:
                sithCogExec_NegFalseOps(pCog, opcode);
                break;

            case SITHCOGEXEC_OPCODE_CMPAND:
            case SITHCOGEXEC_OPCODE_CMPOR:
            case SITHCOGEXEC_OPCODE_CMPNE:
            case SITHCOGEXEC_OPCODE_AND:
            case SITHCOGEXEC_OPCODE_OR:
            case SITHCOGEXEC_OPCODE_XOR:
                sithCogExec_IntererOps(pCog, opcode);
                break;

            default:
                sithCogExec_FloatOps(pCog, opcode);
                break;
        }
    }

    return 1;
}

void J3DAPI sithCogExec_ExecuteMessage(SithCog* pCog, int handlerNum)
{
    SithCogScript* pScript;

#line 203
    SITH_ASSERTREL(pCog != NULL);

    pScript = pCog->pScript;
    if ( pScript->aHandlers[handlerNum].codeOffset < 0 )
    {
#line 210
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Message type %d not defined in the code.\n", pCog->aName, handlerNum, 0, 0);
        return;
    }

    if ( pCog->status != SITHCOG_STATUS_STOPPED )
    {
        if ( pCog->status == SITHCOG_STATUS_EXECUTING )
        {
            pCog->status = SITHCOG_STATUS_EXECUTION_SUSPENDED;
        }

        sithCogExec_PushCallstack(pCog);
    }
    else
    {
        sithCogExec_ResetStack(pCog);
    }

    pCog->execPos     = pScript->aHandlers[handlerNum].codeOffset;
    pCog->execMsgType = pScript->aHandlers[handlerNum].type;

    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: execution started.\n", pCog->aName);
        sithConsole_PrintString(std_genBuffer);
    }

    sithCogExec_Execute(pCog);
    if ( pCog->status == SITHCOG_STATUS_EXECUTION_SUSPENDED )
    {
        pCog->status = SITHCOG_STATUS_EXECUTING;
    }
}

int J3DAPI sithCogExec_PopSymbol(SithCog* pCog, SithCogSymbolValue* pVal)
{
    SithCogSymbolValue dest;

#line 260
    SITH_ASSERTREL(pCog != NULL);
#line 261
    SITH_ASSERTREL(pVal != NULL);

    if ( !sithCogExec_PopStack(pCog, pVal) )
    {
        return 0;
    }

    *pVal = *sithCogExec_GetSymbolValue(&dest, pCog, pVal);
    return 1;
}

float J3DAPI sithCogExec_PopFlex(SithCog* pCog)
{
    SithCogSymbolValue value;
    SithCogSymbolValue retVal;
    float result;

#line 301
    SITH_ASSERTREL(pCog != NULL);

    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return 0.0f;
    }

    value = *sithCogExec_GetSymbolValue(&retVal, pCog, &value);
    if ( value.type == SITHCOG_VALUE_INT )
    {
        result = (float)value.val.intValue;
    }
    else if ( value.type == SITHCOG_VALUE_FLOAT )
    {
        result = value.val.floatValue;
    }
    else
    {
#line 319
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not convert value type %d to flex.\n", value.type, 0, 0, 0);
        result = 0.0f;
    }

    return result;
}

int J3DAPI sithCogExec_PopInt(SithCog* pCog)
{
    SithCogSymbolValue value;
    SithCogSymbolValue retVal;
    int result;

#line 349
    SITH_ASSERTREL(pCog != NULL);

    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return -1;
    }

    value = *sithCogExec_GetSymbolValue(&retVal, pCog, &value);
    if ( value.type == SITHCOG_VALUE_FLOAT )
    {
        result = lrintf(value.val.floatValue);
    }
    else if ( value.type == SITHCOG_VALUE_INT )
    {
        result = value.val.intValue;
    }
    else
    {
#line 366
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Could not convert value type %d to INT.\n", value.type, 0, 0, 0);
        result = -1;
    }

    return result;
}

int J3DAPI sithCogExec_PopArray(SithCog* pCog)
{
    SithCogSymbolValue value;

#line 379
    SITH_ASSERTREL(pCog);

    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return 0;
    }

    if ( value.type == SITHCOG_VALUE_SYMBOLID )
    {
        return value.val.intValue;
    }

    return 0;
}

int J3DAPI sithCogExec_PopVector(SithCog* pCog, rdVector3* vec)
{
    SithCogSymbolValue value;
    SithCogSymbolValue retVal;

#line 420
    SITH_ASSERTREL(pCog != NULL);
#line 421
    SITH_ASSERTREL(vec != NULL);

    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        memset(vec, 0, sizeof(rdVector3));
        return 0;
    }

    value = *sithCogExec_GetSymbolValue(&retVal, pCog, &value);
    if ( value.type == SITHCOG_VALUE_VECTOR )
    {
        rdVector_Copy3(vec, &value.val.vecValue);
        return 1;
    }

#line 438
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Expected vector, got int/flex constant.\n", pCog->aName, 0, 0, 0);
    memset(vec, 0, sizeof(rdVector3));
    return 0;
}

SithCog* J3DAPI sithCogExec_PopCog(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 467
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
        pWorld = sithWorld_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( !pWorld || index < 0 || index >= pWorld->numCogs )
    {
#line 482
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad cog index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aCogs[index];
}

SithThing* J3DAPI sithCogExec_PopThing(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 512
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( !pWorld || index < 0 || index > pWorld->lastThingIdx || !pWorld->aThings[index].type )
    {
#line 518
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aThings[index];
}

SithThing* J3DAPI sithCogExec_TryPopThing(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 532
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index < 0 )
    {
        return NULL;
    }

    if ( !pWorld || index > pWorld->lastThingIdx || !pWorld->aThings[index].type )
    {
#line 544
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad thing index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aThings[index];
}

SithThing* J3DAPI sithCogExec_PopTemplate(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 576
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
#line 585
        SITH_ASSERTREL(sithWorld_pStaticWorld);
        pWorld = sithWorld_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( !pWorld || index < 0 || index >= (int)pWorld->numThingTemplates )
    {
#line 593
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad template index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aThingTemplates[index];
}

tSoundHandle J3DAPI sithCogExec_PopSound(SithCog* pCog)
{
    int index;

#line 624
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return 0;
    }

    return Sound_GetSoundHandle(index);
}

SithSector* J3DAPI sithCogExec_PopSector(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 655
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( !pWorld || index < 0 || index >= (int)pWorld->numSectors )
    {
#line 661
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad sector index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aSectors[index];
}

SithSurface* J3DAPI sithCogExec_PopSurface(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 691
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( !pWorld || index < 0 || index >= (int)pWorld->numSurfaces )
    {
#line 697
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad surface index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aSurfaces[index];
}

rdMaterial* J3DAPI sithCogExec_PopMaterial(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 727
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
#line 736
        SITH_ASSERTREL(sithWorld_pStaticWorld);
        pWorld = sithWorld_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( !pWorld || index < 0 || index >= (int)pWorld->numMaterials || !pWorld->aMaterials[index].aName[0] )
    {
#line 744
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad material index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aMaterials[index];
}

rdModel3* J3DAPI sithCogExec_PopModel3(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 769
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
#line 778
        SITH_ASSERTREL(sithWorld_pStaticWorld);
        pWorld = sithWorld_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( !pWorld || index < 0 || index >= (int)pWorld->numModels )
    {
#line 786
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad model index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aModels[index];
}

rdKeyframe* J3DAPI sithCogExec_PopKeyframe(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 798
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    if ( SITHWORLD_IS_STATICINDEX(index) )
    {
#line 807
        SITH_ASSERTREL(sithWorld_pStaticWorld);
        pWorld = sithWorld_pStaticWorld;
        index  = SITHWORLD_FROM_STATICINDEX(index);
    }

    if ( !pWorld || index < 0 || index >= (int)pWorld->numKeyframes )
    {
    // Retail prints the model message here as well
#line 815
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad model index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aKeyframes[index];
}

SithAIClass* J3DAPI sithCogExec_PopAIClass(SithCog* pCog)
{
    SithWorld* pWorld = sithWorld_pCurrentWorld;
    int index;

#line 829
    SITH_ASSERTREL(pCog);

    index = sithCogExec_PopInt(pCog);
    if ( index == -1 )
    {
        return NULL;
    }

    if ( !pWorld || index < 0 || index >= (int)pWorld->numAIClasses )
    {
#line 838
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad AI index %d on stack.\n", pCog->aName, index, 0, 0);
        return NULL;
    }

    return &pWorld->aAIClasses[index];
}

SithCogSymbolValue* J3DAPI sithCogExec_GetPointerValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue); // debug build: PopPointer calls it before its definition

/* Linker-discarded: retail __LINE__ gap 838..914 (sithCogExec_PopAIClass -> sithCogExec_PopString). Body from the Indy debug
   build (0x0042C06A, asserts at 877 and 885), the PopString shape; the struct-returning helper is written with an explicit
   destination like GetSymbolValue. */
void* J3DAPI sithCogExec_PopPointer(SithCog* pCog)
{
    SithCogSymbolValue returnVal;
    SithCogSymbolValue retVal;

#line 877
    SITH_ASSERTREL(pCog != NULL);

    if ( !sithCogExec_PopStack(pCog, &returnVal) )
    {
        return NULL;
    }

    returnVal = *sithCogExec_GetPointerValue(&retVal, pCog, &returnVal);
#line 885
    SITH_ASSERTREL(returnVal.type == SITHCOG_VALUE_POINTER);
    return returnVal.val.pointerValue;
}

const char* J3DAPI sithCogExec_PopString(SithCog* pCog)
{
    SithCogSymbolValue value;
    SithCogSymbol* pSymbol;

#line 914
    SITH_ASSERTREL(pCog != NULL);

    if ( !sithCogExec_PopStack(pCog, &value) )
    {
        return NULL;
    }

    if ( value.type == SITHCOG_VALUE_SYMBOLID )
    {
        pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, value.val.intValue);
#line 925
        SITH_ASSERTREL(pSymbol != NULL);

        if ( pSymbol->value.type == SITHCOG_VALUE_STRING )
        {
            value.val.pString = pSymbol->value.val.pString;
        }
        else
        {
            value.val.pString = NULL;
        }

        return value.val.pString;
    }

    return NULL;
}

void J3DAPI sithCogExec_PushStack(SithCog* pCog, SithCogSymbolValue* pValue)
{
#line 964
    SITH_ASSERTREL(pCog != NULL);
#line 965
    SITH_ASSERTREL(pValue != NULL);

    if ( pCog->stackSize == SITHCOGEXEC_STACKSIZE )
    {
#line 969
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Stack overflow in script %s.  Bottom element discarded.\n", pCog->pScript->aName, 0, 0, 0);
        memmove(pCog->stack, &pCog->stack[1], (SITHCOGEXEC_STACKSIZE - 1) * sizeof(SithCogSymbolValue));
        --pCog->stackSize;
    }

#line 974
    SITH_ASSERTREL(pCog->stackSize < 256);

    pCog->stack[pCog->stackSize] = *pValue;
    ++pCog->stackSize;
}

void J3DAPI sithCogExec_PushInt(SithCog* pCog, int val)
{
    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_INT;
    value.val.intValue = val;
    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_PushFlex(SithCog* pCog, float value)
{
    SithCogSymbolValue val;
    val.type = SITHCOG_VALUE_FLOAT;
    val.val.floatValue = value;
    sithCogExec_PushStack(pCog, &val);
}

void J3DAPI sithCogExec_PushVector(SithCog* pCog, const rdVector3* vec)
{
    SithCogSymbolValue value;
    value.type = SITHCOG_VALUE_VECTOR;
    rdVector_Copy3(&value.val.vecValue, vec);
    sithCogExec_PushStack(pCog, &value);
}

SithCogExecOpcode J3DAPI sithCogExec_GetOpCode(SithCog* pCog)
{
    SithCogScript* pScript;
    SithCogExecOpcode opcode;

#line 1088
    SITH_ASSERTREL(pCog != NULL);

    pScript = pCog->pScript;
#line 1093
    SITH_ASSERTREL(pScript != NULL);

    if ( pCog->execPos < pScript->codeSize - 1 )
    {
        opcode = pScript->pCode[pCog->execPos++];
    }
    else
    {
#line 1102
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "End of code reached, cannot get opcode.\n", 0, 0, 0, 0);
        return SITHCOGEXEC_OPCODE_RET;
    }

    return opcode;
}

void J3DAPI sithCogExec_ResetStack(SithCog* pCog)
{
#line 1132
    SITH_ASSERTREL(pCog != NULL);

    if ( pCog->stackSize > 0 )
    {
        pCog->stackSize = 0;
    }
}

void J3DAPI sithCogExec_PushCallstack(SithCog* pCog)
{
#line 1160
    SITH_ASSERTREL(pCog);

    if ( pCog->callDepth == SITHCOGEXEC_CALLSTACKSIZE )
    {
#line 1164
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: tried to exceed max execution depth.\n", pCog->aName, 0, 0, 0);
        return;
    }

    pCog->callstack[pCog->callDepth].execPos     = pCog->execPos;
    pCog->callstack[pCog->callDepth].execStatus  = pCog->status;
    pCog->callstack[pCog->callDepth].statusParam = pCog->statusParams[0];
    pCog->callstack[pCog->callDepth].execMsgType = pCog->execMsgType;
    ++pCog->callDepth;
}

void J3DAPI sithCogExec_PopCallstack(SithCog* pCog)
{
    size_t depth;

#line 1197
    SITH_ASSERTREL(pCog);

    depth = pCog->callDepth;
    if ( pCog->callDepth == 0 )
    {
        pCog->status = SITHCOG_STATUS_STOPPED;
        return;
    }

    pCog->status          = pCog->callstack[depth - 1].execStatus;
    pCog->execPos         = pCog->callstack[depth - 1].execPos;
    pCog->statusParams[0] = pCog->callstack[depth - 1].statusParam;
    pCog->execMsgType     = pCog->callstack[depth - 1].execMsgType;
    --pCog->callDepth;
}

int J3DAPI sithCogExec_PopStack(SithCog* pCog, SithCogSymbolValue* pValue)
{
#line 1243
    SITH_ASSERTREL(pCog != NULL);
#line 1244
    SITH_ASSERTREL(pValue != NULL);

    if ( pCog->stackSize < 1 )
    {
#line 1248
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: PopStack failed -- stack size < 1.\n", pCog->aName, 0, 0, 0);
        return 0;
    }

    *pValue = pCog->stack[--pCog->stackSize];
    return 1;
}

void J3DAPI sithCogExec_FunctionCallOp(SithCog* pCog)
{
    SithCogSymbolValue value;
    SithCogFunctionType pfCogFunction;
    SithCogSymbol* pSymbol = NULL;

    if ( !sithCogExec_PopStack(pCog, &value) || value.type != SITHCOG_VALUE_SYMBOLID )
    {
        goto error;
    }

    pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, value.val.intValue);
    if ( !pSymbol )
    {
        goto error;
    }

    if ( pSymbol->value.type != SITHCOG_VALUE_POINTER )
    {
        goto error;
    }

    pfCogFunction = (SithCogFunctionType)pSymbol->value.val.pointerValue;
    if ( !pfCogFunction )
    {
        goto error;
    }

    pfCogFunction(pCog);
    return;

error:
    if ( pSymbol )
    {
        if ( pSymbol->pName )
        {
#line 1307
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unrecognized or malformed function call, addr %d, symbol %s.\n", pCog->aName, pCog->execPos, pSymbol->pName, 0);
        }
        else
        {
#line 1311
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unrecognized or malformed function call, addr %d\n", pCog->aName, pCog->execPos, 0, 0);
        }
    }
    else
    {
#line 1316
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Unrecognized or malformed function call, addr %d\n", pCog->aName, pCog->execPos, 0, 0);
    }
}

void J3DAPI sithCogExec_RetOp(SithCog* pCog)
{
    if ( (pCog->flags & SITHCOG_DEBUG) != 0 )
    {
        sprintf(std_genBuffer, "Cog %s: Returned from depth %d.\n", pCog->aName, pCog->callDepth);
        sithConsole_PrintString(std_genBuffer);
    }

    sithCogExec_PopCallstack(pCog);
}

void J3DAPI sithCogExec_PushNumberOp(SithCog* pCog, int opcode)
{
    SithCogSymbolValue value;

    value.val.intValue = sithCogExec_GetOpCode(pCog);
    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_PUSHINT:
            value.type = SITHCOG_VALUE_INT;
            break;

        case SITHCOGEXEC_OPCODE_PUSHFLOAT:
            value.type = SITHCOG_VALUE_FLOAT;
            break;

        case SITHCOGEXEC_OPCODE_PUSHSYMBOL:
            value.type = SITHCOG_VALUE_SYMBOLID;
            break;

        default:
#line 1378
            SITH_ASSERTREL(0);
            break;
    }

    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_PushVectorOp(SithCog* pCog)
{
    SithCogSymbolValue value;

#line 1405
    SITH_ASSERTREL(pCog && pCog->pScript);

    rdVector_Copy3(&value.val.vecValue, &pCog->pScript->pCode[pCog->execPos]);
    pCog->execPos += 3;

    value.type = SITHCOG_VALUE_VECTOR;
    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_CallOp(SithCog* pCog)
{
    size_t label;

    if ( pCog->callDepth < SITHCOGEXEC_CALLSTACKSIZE )
    {
        label = sithCogExec_GetOpCode(pCog);
        if ( label >= pCog->pScript->codeSize )
        {
#line 1444
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Script %s: Illegal CALL opcode - undefined label?\n", pCog->aName, 0, 0, 0);
            return;
        }

        sithCogExec_PushCallstack(pCog);
        pCog->execPos = label;
        return;
    }

#line 1454
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Max call depth exceeded.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogExec_JmpnzOp(SithCog* pCog)
{
    int condition;
    size_t jumpTarget = sithCogExec_GetOpCode(pCog);
#line 1478
    SITH_ASSERTREL(jumpTarget < pCog->pScript->codeSize);

    condition = sithCogExec_PopInt(pCog);
    if ( condition )
    {
        pCog->execPos = jumpTarget;
    }
}

void J3DAPI sithCogExec_JmpzOp(SithCog* pCog)
{
    int condition;
    size_t jumpTarget = sithCogExec_GetOpCode(pCog);
#line 1508
    SITH_ASSERTREL(jumpTarget < pCog->pScript->codeSize);

    condition = sithCogExec_PopInt(pCog);
    if ( !condition )
    {
        pCog->execPos = jumpTarget;
    }
}

void J3DAPI sithCogExec_JmpOp(SithCog* pCog)
{
    size_t jumpTarget = sithCogExec_GetOpCode(pCog);
#line 1537
    SITH_ASSERTREL(jumpTarget < pCog->pScript->codeSize);

    pCog->execPos = jumpTarget;
}

void J3DAPI sithCogExec_ArrayOp(SithCog* pCog)
{
    int startIdx = sithCogExec_PopInt(pCog);
    int offset = sithCogExec_PopArray(pCog);

    SithCogSymbolValue newValue;
    newValue.type = SITHCOG_VALUE_SYMBOLID;
    newValue.val.intValue = startIdx + offset;
    sithCogExec_PushStack(pCog, &newValue);
}

void J3DAPI sithCogExec_NegFalseOps(SithCog* pCog, int opcode)
{
    SithCogSymbolValue value;
    int ival;
    float flex;

    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_CMPFALSE:
            ival = sithCogExec_PopInt(pCog);
            value.type = SITHCOG_VALUE_INT;
            value.val.intValue = ival == 0;
            break;

        case SITHCOGEXEC_OPCODE_NEG:
            flex = sithCogExec_PopFlex(pCog);
            value.type = SITHCOG_VALUE_FLOAT;
            value.val.floatValue = -flex;
            break;

        default:
#line 1604
            SITH_ASSERTREL(0);
            break;
    }

    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_AssignOp(SithCog* pCog)
{
    SithCogSymbolValue curvalue;
    SithCogSymbolValue variable;
    SithCogSymbol* pSymbol;

    if ( !sithCogExec_PopSymbol(pCog, &curvalue) )
    {
        goto error;
    }

    if ( !sithCogExec_PopStack(pCog, &variable) )
    {
        goto error;
    }

    if ( variable.type == SITHCOG_VALUE_SYMBOLID )
    {
        pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, variable.val.intValue);
        pSymbol->value = curvalue;
    }
    else
    {
#line 1644
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Illegal assignment attempted.\n", pCog->aName, 0, 0, 0);
    }

    return;

error:
#line 1649
    stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Error in assignment statement.\n", pCog->aName, 0, 0, 0);
}

void J3DAPI sithCogExec_IntererOps(SithCog* pCog, int opcode)
{
    int a;
    SithCogSymbolValue value;
    int b;

    value.type = SITHCOG_VALUE_INT;
    b = sithCogExec_PopInt(pCog);
    a = sithCogExec_PopInt(pCog);

    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_CMPAND:
            value.val.intValue = a && b;
            break;

        case SITHCOGEXEC_OPCODE_CMPOR:
            value.val.intValue = a || b;
            break;

        case SITHCOGEXEC_OPCODE_CMPNE:
            value.val.intValue = a != b;
            break;

        case SITHCOGEXEC_OPCODE_AND:
            value.val.intValue = a & b;
            break;

        case SITHCOGEXEC_OPCODE_OR:
            value.val.intValue = a | b;
            break;

        case SITHCOGEXEC_OPCODE_XOR:
            value.val.intValue = a ^ b;
            break;

        default:
#line 1707
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unrecognized opcode passed to IntegerOps.\n", 0, 0, 0, 0);
            return;
    }

    sithCogExec_PushStack(pCog, &value);
}

void J3DAPI sithCogExec_FloatOps(SithCog* pCog, int opcode)
{
    float a;
    SithCogSymbolValue value;
    float b;

    value.type = SITHCOG_VALUE_FLOAT;
    b = sithCogExec_PopFlex(pCog);
    a = sithCogExec_PopFlex(pCog);

    switch ( opcode )
    {
        case SITHCOGEXEC_OPCODE_ADD:
            value.val.floatValue = a + b;
            break;

        case SITHCOGEXEC_OPCODE_SUB:
            value.val.floatValue = a - b;
            break;

        case SITHCOGEXEC_OPCODE_MUL:
            value.val.floatValue = a * b;
            break;

        case SITHCOGEXEC_OPCODE_DIV:
            if ( b != 0.0f )
            {
                value.val.floatValue = a / b;
            }
            else
            {
                value.val.intValue = 0;
            }
            break;

        case SITHCOGEXEC_OPCODE_MOD:
            value.val.floatValue = ((float)fmod(a, b));
            break;

        case SITHCOGEXEC_OPCODE_CMPGT:
            value.type = SITHCOG_VALUE_INT;
            value.val.intValue = a > b;
            break;

        case SITHCOGEXEC_OPCODE_CMPLS:
            value.type = SITHCOG_VALUE_INT;
            value.val.intValue = a < b;
            break;

        case SITHCOGEXEC_OPCODE_CMPEQ:
            value.type = SITHCOG_VALUE_INT;
            value.val.intValue = a == b;
            break;

        case SITHCOGEXEC_OPCODE_CMPGE:
            value.type = SITHCOG_VALUE_INT;
            value.val.intValue = a >= b;
            break;

        case SITHCOGEXEC_OPCODE_CMPLE:
            value.type = SITHCOG_VALUE_INT;
            value.val.intValue = a <= b;
            break;

        default:
#line 1799
            stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Unrecognized opcode passed to FloatOps.\n", 0, 0, 0, 0);
            return;
    }

    sithCogExec_PushStack(pCog, &value);
}

SithCogSymbolValue* J3DAPI sithCogExec_GetSymbolValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue)
{
    SithCogSymbol* pSymbol;
    SithCogSymbolValue value;

#line 1831
    SITH_ASSERTREL(pCog && pValue);

    if ( pValue->type == SITHCOG_VALUE_SYMBOLID )
    {
        pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, pValue->val.intValue);
#line 1839
        SITH_ASSERTREL(pSymbol != NULL);
        pValue = &pSymbol->value;
    }

    if ( pValue->type == SITHCOG_VALUE_POINTER )
    {
#line 1846
        SITH_ASSERTREL(pValue->val.pointerValue != NULL);
        value.type = SITHCOG_VALUE_INT;
        value.val.intValue = *(int*)pValue->val.pointerValue;
    }
    else
    {
        value = *pValue;
    }

    *pDest = value;
    return pDest;
}

/* Linker-discarded (its only caller is the discarded PopPointer): the debug build's pointer twin of GetSymbolValue
   (0x0042D0A9, asserts at 1895 and 1915), restored from its code; name inferred. A symbol reference yields the symbol's
   pointer value or the address of its value; a pointer value is passed through. */
SithCogSymbolValue* J3DAPI sithCogExec_GetPointerValue(SithCogSymbolValue* pDest, SithCog* pCog, SithCogSymbolValue* pValue)
{
    SithCogSymbol* pSymbol;
    SithCogSymbolValue value;

    value.type = SITHCOG_VALUE_POINTER;
    if ( pValue->type == SITHCOG_VALUE_SYMBOLID )
    {
        pSymbol = sithCogParse_GetSymbolByID(pCog->pSymbolTable, pValue->val.intValue);
#line 1895
        SITH_ASSERTREL(pSymbol != NULL);
        if ( pSymbol->value.type == SITHCOG_VALUE_POINTER )
        {
            value.val.pointerValue = pSymbol->value.val.pointerValue;
        }
        else
        {
            value.val.pointerValue = &pSymbol->value.val;
        }
    }
    else if ( pValue->type == SITHCOG_VALUE_POINTER )
    {
        value = *pValue;
    }
    else
    {
#line 1915
        SITH_ASSERTREL(0);
    }

    *pDest = value;
    return pDest;
}
