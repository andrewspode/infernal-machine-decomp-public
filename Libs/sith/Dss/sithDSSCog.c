#include "sithDSSCog.h"

#include <sith/Cog/sithCog.h>
#include <sith/Dss/sithDss.h>
#include <sith/Dss/sithMulti.h>
#include <sith/World/sithThing.h>
#include <sith/World/sithWorld.h>

int J3DAPI sithDSSCog_SendMessage(const SithCog* pCog, SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3, DPID idTo)
{
    uint8_t* pCurDSSMsgData_ = sithMulti_message.data;
    unsigned int sendflags = DPSEND_GUARANTEED;
    SithThing* pThing;

    SITHDSS_PUSHINT32(linkId);

    if ( pCog )
    {
        SITHDSS_PUSHINT16(pCog->idx);
    }
    else
    {
        SITHDSS_PUSHINT16(-1);
    }

    SITHDSS_PUSHUINT8(senderType);
    SITHDSS_PUSHUINT8(srcType);

    if ( senderType == SITHCOG_SYM_REF_THING && (pThing = sithThing_GetThingByIndex(senderIdx)) != NULL )
    {
        SITHDSS_PUSHINT32(pThing->guid);
    }
    else
    {
        SITHDSS_PUSHINT32(senderIdx);
    }

    if ( srcType == SITHCOG_SYM_REF_THING && (pThing = sithThing_GetThingByIndex(srcIdx)) != NULL )
    {
        SITHDSS_PUSHINT32(pThing->guid);
    }
    else
    {
        SITHDSS_PUSHINT32(srcIdx);
    }

    SITHDSS_PUSHUINT16((uint8_t)msgType);
    SITHDSS_PUSHINT32(param0);
    SITHDSS_PUSHINT32(param1);
    SITHDSS_PUSHINT32(param2);
    SITHDSS_PUSHINT32(param3);

    sithMulti_message.type = SITHDSS_COGMESSAGE;
    SITHDSS_ENDOUT;

    if ( msgType == SITHCOG_MSG_TOUCHED )
    {
        sendflags = 0;
    }

    return sithMessage_SendMessage(&sithMulti_message, idTo, SITHMESSAGE_STREAM_NET, sendflags);
}

int J3DAPI sithDSSCog_ProcessMessage(const SithMessage* pMsg)
{
    int linkId;
    int cogIdx;
    SithCog* pCog;
    SithCogSymbolRefType senderType;
    SithCogSymbolRefType srcType;
    int senderIdx;
    int srcIdx;
    SithCogMsgType msgType;
    int param0;
    int param1;
    int param2;
    int param3;
    SithThing* pSenderThing;
    SithThing* pThing;
    uint8_t* pCurDSSMsgData_;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 144
    SITH_ASSERTREL(pMsg);

    linkId = SITHDSS_POPINT32();
    cogIdx = SITHDSS_POPUINT16();
    pCog = sithCog_GetCogByIndex(cogIdx);

    senderType = SITHDSS_POPUINT8();
    srcType = SITHDSS_POPUINT8();

    senderIdx = SITHDSS_POPINT32();
    srcIdx = SITHDSS_POPINT32();

    if ( senderType == SITHCOG_SYM_REF_THING )
    {
        pSenderThing = sithThing_GetGuidThing(senderIdx); // senderIdx is a guid
        if ( pSenderThing )
        {
            senderIdx = pSenderThing->idx;
        }
    }

    if ( srcType == SITHCOG_SYM_REF_THING )
    {
        pSenderThing = sithThing_GetGuidThing(srcIdx); // srcIdx is a guid
        if ( pSenderThing )
        {
            srcIdx = pSenderThing->idx;
        }
    }

    msgType = SITHDSS_POPUINT16();
    param0 = SITHDSS_POPINT32();
    param1 = SITHDSS_POPINT32();
    param2 = SITHDSS_POPINT32();
    param3 = SITHDSS_POPINT32();

    if ( !pCog )
    {
        sithCog_BroadcastMessageEx(msgType, senderType, senderIdx, srcType, srcIdx, param0, param1, param2, param3);
        return 1;
    }

    if ( senderType == SITHCOG_SYM_REF_THING )
    {
        pThing = sithThing_GetThingByIndex(senderIdx);
        if ( !pThing || (pThing->flags & SITH_TF_COGLINKED) == 0 )
        {
            return 1;
        }
    }

    sithCog_SendMessageEx(pCog, msgType, senderType, senderIdx, srcType, srcIdx, linkId, param0, param1, param2, param3);
    return 1;
}

int J3DAPI sithDSSCog_SyncCogState(const SithCog* pCog, DPID idTO, SithMessageStream outstream)
{
    uint8_t* pCurDSSMsgData_ = &sithMulti_message.data[0];
    size_t i;

#line 213
    SITH_ASSERTREL(pCog);

    SITHDSS_PUSHINT32(pCog->idx);
    SITHDSS_PUSHINT32(pCog->status);
    SITHDSS_PUSHINT32(pCog->flags);

    if ( pCog->status )
    {
        SITHDSS_PUSHINT32(pCog->statusParams[0]);
        SITHDSS_PUSHINT32(pCog->statusParams[1]);
        SITHDSS_PUSHUINT32(pCog->execPos);
        SITHDSS_PUSHINT32(pCog->linkId);
        SITHDSS_PUSHINT32(pCog->senderIdx);
        SITHDSS_PUSHINT32(pCog->senderType);
        SITHDSS_PUSHINT32(pCog->sourceIdx);
        SITHDSS_PUSHINT32(pCog->sourceType);
    }

    if ( (pCog->flags & SITHCOG_PULSE_SET) != 0 )
    {
        SITHDSS_PUSHUINT32(pCog->msecPulseInterval);
        SITHDSS_PUSHUINT32(pCog->msecNextPulseTime);
    }

    if ( (pCog->flags & SITHCOG_TIMER_SET) != 0 )
    {
        SITHDSS_PUSHUINT32(pCog->msecTimerTimeout);
    }

    for ( i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        SITHDSS_PUSHUINT8(pCog->pSymbolTable->aSymbols[i].value.type);
    }

    for ( i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        switch ( pCog->pSymbolTable->aSymbols[i].value.type )
        {
        case SITHCOG_VALUE_VECTOR:
            SITHDSS_PUSHVEC3(&pCog->pSymbolTable->aSymbols[i].value.val.vecValue);
            break;
        case SITHCOG_VALUE_FLOAT:
            SITHDSS_PUSHFLOAT(pCog->pSymbolTable->aSymbols[i].value.val.floatValue);
            break;
        case SITHCOG_VALUE_STRING:
            break;
        default:
            SITHDSS_PUSHINT32(pCog->pSymbolTable->aSymbols[i].value.val.intValue);
            break;
        }
    }

    sithMulti_message.type = SITHDSS_COGSTATE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTO, outstream, DPSEND_GUARANTEED);
}

int J3DAPI sithDSSCog_ProcessCogState(const SithMessage* pMsg)
{
    SithCog* pCog;
    int cogIdx;
    uint8_t* pCurDSSMsgData_;
    size_t i;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 300
    SITH_ASSERTREL(pMsg && sithWorld_pCurrentWorld);

    cogIdx = SITHDSS_POPINT32();
    pCog = sithCog_GetCogByIndex(cogIdx);
    if ( !pCog )
    {
        return 0;
    }

    pCog->status = SITHDSS_POPINT32();
    pCog->flags = SITHDSS_POPINT32();

    if ( pCog->status )
    {
        pCog->statusParams[0] = SITHDSS_POPINT32();
        pCog->statusParams[1] = SITHDSS_POPINT32();
        pCog->execPos         = SITHDSS_POPUINT32();
        pCog->linkId          = SITHDSS_POPINT32();
        pCog->senderIdx       = SITHDSS_POPINT32();
        pCog->senderType      = SITHDSS_POPINT32();
        pCog->sourceIdx       = SITHDSS_POPINT32();
        pCog->sourceType      = SITHDSS_POPINT32();
    }

    if ( (pCog->flags & SITHCOG_PULSE_SET) != 0 )
    {
        pCog->msecPulseInterval = SITHDSS_POPUINT32();
        pCog->msecNextPulseTime = SITHDSS_POPUINT32();
    }

    if ( (pCog->flags & SITHCOG_TIMER_SET) != 0 )
    {
        pCog->msecTimerTimeout = SITHDSS_POPUINT32();
    }

    for ( i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        pCog->pSymbolTable->aSymbols[i].value.type = SITHDSS_POPUINT8();
    }

    for ( i = 0; i < pCog->pSymbolTable->numUsedSymbols; ++i )
    {
        switch ( pCog->pSymbolTable->aSymbols[i].value.type )
        {
        case SITHCOG_VALUE_VECTOR:
            SITHDSS_POPVEC3(&pCog->pSymbolTable->aSymbols[i].value.val.vecValue);
            break;
        case SITHCOG_VALUE_FLOAT:
            // Retail copies the raw 4 bytes (no x87 load), like the default case
            pCog->pSymbolTable->aSymbols[i].value.val.intValue = SITHDSS_POPINT32();
            break;
        case SITHCOG_VALUE_STRING:
            break;
        default:
            pCog->pSymbolTable->aSymbols[i].value.val.intValue = SITHDSS_POPINT32();
            break;
        }
    }

    return 1;
}
