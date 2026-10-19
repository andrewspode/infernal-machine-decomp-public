#include "sithVoice.h"

#include <sith/Cog/sithCog.h>
#include <sith/Cog/sithCogExec.h>
#include <sith/Devices/sithComm.h>
#include <sith/Devices/sithSoundMixer.h>
#include <sith/Dss/sithDSS.h>
#include <sith/Gameplay/sithTime.h>
#include <sith/Main/sithString.h>
#include <sith/World/sithModel.h>
#include <sith/World/sithThing.h>

#include <rdroid/Engine/rdCamera.h>
#include <rdroid/Primitives/rdFont.h>

#include <sound/Sound.h>

#include <std/General/stdMemory.h>
#include <std/General/stdPlatform.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdDisplay.h>

#include <w32util/wuRegistry.h>

#define SITHVOICE_MSEC_EXTRA_SUBTITLE_SHOW_DURATION  1000
#define SITHVOICE_MAX_LINES_PER_DRAW                 3    // number of lines to draw
#define SITHVOICE_MAX_SUBTITLES                      25

#define SITHVOICE_TEXT_PADDING_X 24.0f
#define SITHVOICE_REF_WIDTH     (RD_REF_WIDTH * RD_REF_WIDTH)

#define SITHVOICE_TEXT_PADDING_Y 12.0f
#define SITHVOICE_REF_HEIGHT     (RD_REF_HEIGHT * RD_REF_HEIGHT)

#define SITHVOICE_FONTFILENAME "mat\\jonesCalisto MT20.gcf"

// Zero-initialized definitions: retail keeps these in declaration order in one 8-aligned .bss block.
static int sithVoice_curSubtitleInfoNum        = 0;
static uint8_t sithVoice_aVoiceHeadHeights[4][4] = { 0 };
static int sithVoice_numSubtitleInfos          = 0;
static float sithVoice_secNextHeadSwapTime     = 0.0f;
static rdFont* sithVoice_pTextFont             = NULL;
static size_t sithVoice_sameHeadHightCounter   = 0;
static int sithVoice_bOpen                     = 0;
static int sithVoice_bStartup                  = 0;
static int sithVoice_bShowText                 = 0;
static int sithVoice_bThingHasSwapHead         = 0;
static int sithVoice_curHeadHeight             = 0;

static float sithVoice_secHeadSwapInterval = 0.1f;
static int sithVoice_dword_52B9A8          = 50; // this is some offset when calculating head pos from lypsync data
static int sithVoice_lastHeadHeight        = -1;

SithVoiceSubtitleInfo sithVoice_aSubtitleInfos[SITHVOICE_MAX_SUBTITLES];

void J3DAPI sithVoice_PlayVoice(SithCog* pCog);
void J3DAPI sithVoice_SetThingVoiceHeads(SithCog* pCog);
void J3DAPI sithVoice_SetVoiceHeadHeight(SithCog* pCog);
void J3DAPI sithVoice_SetThingVoiceColor(SithCog* pCog);
void J3DAPI sithVoice_SetVoiceParams(SithCog* pCog);

void J3DAPI sithVoice_AddSubtitle(int msecSoundLen, const char* pSoundFilename, const char* pSubtitleText, VGradiantColor* pSubtitleColor);
void J3DAPI sithVoice_PurgeDrawnSubtitles(void);

int J3DAPI sithVoice_Startup(void)
{
    sithVoice_pTextFont = NULL;
    sithVoice_bShowText = wuRegistry_GetIntEx("Show Text", 0);
    sithVoice_bStartup  = 1;
    return 1;
}

int J3DAPI sithVoice_Open(void)
{
    if ( !sithVoice_pTextFont )
    {
        sithVoice_pTextFont = rdFont_Load(SITHVOICE_FONTFILENAME);
        if ( !sithVoice_pTextFont )
        {
            goto error;
        }
    }

    memset(sithVoice_aSubtitleInfos, 0, sizeof(sithVoice_aSubtitleInfos));
    sithVoice_curSubtitleInfoNum = 0;
    sithVoice_numSubtitleInfos   = 0;
    sithVoice_bOpen = 1;
    return 1;

error:
    sithVoice_Close();
    sithVoice_bOpen = 0;
    return 0;
}

void J3DAPI sithVoice_Shutdown(void)
{
    sithVoice_pTextFont = NULL;
    sithVoice_bStartup  = 0;
}

void J3DAPI sithVoice_Close(void)
{
    if ( sithVoice_pTextFont )
    {
        rdFont_Free(sithVoice_pTextFont);
        sithVoice_pTextFont = NULL;
    }

    memset(sithVoice_aSubtitleInfos, 0, sizeof(sithVoice_aSubtitleInfos));
    sithVoice_curSubtitleInfoNum = 0;
    sithVoice_numSubtitleInfos   = 0;
    sithVoice_bOpen = 0;
}

int J3DAPI sithVoice_RegisterCogFunctions(SithCogSymbolTable* pTable)
{
    int bFailed;

    bFailed = sithCog_RegisterFunction(pTable, sithVoice_PlayVoice, "playvoice");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithVoice_SetThingVoiceHeads, "setthingvoiceheads");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithVoice_SetThingVoiceColor, "setthingvoicecolor");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithVoice_SetVoiceParams, "setvoiceparams");
    if ( bFailed )
    {
        goto error;
    }
    bFailed = sithCog_RegisterFunction(pTable, sithVoice_SetVoiceHeadHeight, "setvoiceheadheight");
    if ( bFailed )
    {
        goto error;
    }

    return 0;

error:
    return 1;
}

int J3DAPI sithVoice_PlayThingVoice(SithThing* pThing, tSoundHandle hSnd, float volume)
{
    SithThingSwapEntry* pSwapEntry;
    const char* pFilename;
    wchar_t* pwVoiceLine;
    char* pVoiceLine;
    char aFilename[64];
    SithActorInfo* pActorInfo;
    tSoundChannelHandle hSndChannel;

#line 190
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    pActorInfo = &pThing->thingInfo.actorInfo;
    hSndChannel = pActorInfo->voiceInfo.hSndChannel;
    if ( hSndChannel )
    {
        sithSoundMixer_StopSound(hSndChannel);
    }

    pActorInfo->voiceInfo.voiceHeadInfo.headSwapRefNum = -1;
    sithVoice_secNextHeadSwapTime = 0.0f;

    if ( pThing->pSwapList )
    {
        pSwapEntry = pThing->pSwapList;
        while ( pSwapEntry )
        {
            if ( strncmp(pSwapEntry->pSrcModel->aName, "head", 4u) == 0 )
            {
                sithVoice_bThingHasSwapHead = 1;
                break;
            }

            pSwapEntry = pSwapEntry->pNextEntry;
        }
    }

    // Set voice line subtitle text
    pFilename = Sound_GetSoundFilename(hSnd);
    if ( pFilename )
    {
        strncpy(aFilename, pFilename, STD_ARRAYLEN(aFilename) - 1);
        aFilename[STD_ARRAYLEN(aFilename) - 1] = 0;
        _strupr(aFilename);

        pwVoiceLine = sithString_GetString(aFilename);
        if ( pwVoiceLine )
        {
            pVoiceLine = stdUtil_ToAString(pwVoiceLine);
            if ( pVoiceLine )
            {
                sithVoice_AddSubtitle(Sound_GetLengthMsec(hSnd), aFilename, pVoiceLine, &pActorInfo->voiceInfo.voiceColor);
                stdMemory_Free(pVoiceLine);
            }
        }
    }

    // Reset head swap stuff as the lip sync works per single speaker only
    sithVoice_lastHeadHeight       = -1;
    sithVoice_sameHeadHightCounter = 0;

    // Play thing voice line
    pActorInfo->voiceInfo.hSndChannel = sithSoundMixer_PlaySound(hSnd, volume, 0.0f, SOUNDPLAY_PLAYONCE | SOUNDPLAY_THING_POS);
    if ( pActorInfo->voiceInfo.hSndChannel )
    {
        return 0;
    }

    return 1;
}

void J3DAPI sithVoice_UpdateLipSync(SithThing* pThing)
{
    uint8_t mouthPosX;
    SithActorInfo* actorData;
    int bSyncInfo = 1;
    uint8_t mouthPositionY;

#line 259
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    actorData = &pThing->thingInfo.actorInfo;
    if ( actorData->voiceInfo.hSndChannel )
    {
        if ( (pThing->flags & (SITH_TF_DYING | SITH_TF_DESTROYED)) != 0 )
        {
            Sound_StopChannel(actorData->voiceInfo.hSndChannel);
        }

        
        bSyncInfo = Sound_GenerateLipSync(actorData->voiceInfo.hSndChannel, &mouthPosX, &mouthPositionY, sithVoice_dword_52B9A8);
        if ( bSyncInfo )
        {
            if ( sithVoice_secNextHeadSwapTime < sithTime_secGameTime )
            {
                sithVoice_curHeadHeight = sithVoice_aVoiceHeadHeights[mouthPositionY >> 5][mouthPosX >> 5];
                if ( sithVoice_curHeadHeight == sithVoice_lastHeadHeight )
                {
                    if ( ++sithVoice_sameHeadHightCounter >= 2 )
                    {
                        switch ( sithVoice_curHeadHeight )
                        {
                            case 0:
                                break;

                            case 1:
                                sithVoice_curHeadHeight = 3;
                                break;

                            case 2:
                                sithVoice_curHeadHeight = 0;
                                break;

                            case 3:
                                sithVoice_curHeadHeight = 1;
                                break;

                            default:
                                break;
                        }

                        sithVoice_sameHeadHightCounter = 0;
                    }
                }
                else
                {
                    sithVoice_sameHeadHightCounter = 0;
                }

                sithVoice_lastHeadHeight = sithVoice_curHeadHeight;
                if ( actorData->voiceInfo.voiceHeadInfo.apSoundHeadModels[sithVoice_curHeadHeight] )
                {
                    actorData->voiceInfo.voiceHeadInfo.headSwapRefNum = sithThing_AddSwapEntry(pThing, actorData->voiceInfo.voiceHeadInfo.headMeshNum, actorData->voiceInfo.voiceHeadInfo.apSoundHeadModels[sithVoice_curHeadHeight], 0);
                }
                else if ( actorData->voiceInfo.voiceHeadInfo.headSwapRefNum != -1 )
                {
                    sithThing_RemoveSwapEntry(pThing, actorData->voiceInfo.voiceHeadInfo.headSwapRefNum);
                    actorData->voiceInfo.voiceHeadInfo.headSwapRefNum = -1;
                }

                sithVoice_secNextHeadSwapTime = sithTime_secGameTime + sithVoice_secHeadSwapInterval;
            }
        }
        else
        {
            if ( actorData->voiceInfo.voiceHeadInfo.headSwapRefNum != -1 )
            {
                if ( sithVoice_bThingHasSwapHead == 1 && actorData->voiceInfo.voiceHeadInfo.apSoundHeadModels[0] ) // TODO: Note that sithVoice_bThingHasSwapHead might be set for other thing than the one passed to this function
                {
                    sithVoice_bThingHasSwapHead = 0;
                    sithThing_AddSwapEntry(pThing, actorData->voiceInfo.voiceHeadInfo.headMeshNum, actorData->voiceInfo.voiceHeadInfo.apSoundHeadModels[0], 0);
                }
                else
                {
                    sithThing_RemoveSwapEntry(pThing, actorData->voiceInfo.voiceHeadInfo.headSwapRefNum);
                }

                actorData->voiceInfo.voiceHeadInfo.headSwapRefNum = -1;
            }

            actorData->voiceInfo.hSndChannel = 0;
            sithVoice_secNextHeadSwapTime = 0.0f;
        }
    }
}

void J3DAPI sithVoice_PlayVoice(SithCog* pCog)
{
    int guid;
    int curWait;
    float volume;
    tSoundHandle hSnd;
    SithThing* pThing;
    SithActorInfo* pActorData;
    tSoundChannelHandle hwndChannel;

    curWait  = sithCogExec_PopInt(pCog);
    volume = sithCogExec_PopFlex(pCog);
    hSnd   = sithCogExec_PopSound(pCog);
    pThing = sithCogExec_PopThing(pCog);

    if ( !pThing || !hSnd )
    {
#line 364
        stdPrintf(sith_pHS->pErrorPrint, J3D_FILE, __LINE__, "Cog %s: Bad params passed to PlayVoice().\n", pCog->aName, 0, 0, 0);
        sithCogExec_PushInt(pCog, -1); // Note, no return in retail
    }

#line 368
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));
    SITH_ASSERTREL(pThing->renderData.data.pModel3 != NULL);

    pActorData = &pThing->thingInfo.actorInfo;
    if ( strcmpi(pThing->renderData.data.pModel3->aName, "aet_gy.3do") == 0 )
    {
        hwndChannel = sithSoundMixer_PlaySound(hSnd, 1.0f, 0.0f, SOUNDPLAY_PLAYTHINGONCE | SOUNDPLAY_THING_POS);
        if ( hwndChannel )
        {
            guid = hwndChannel ? Sound_GetChannelGUID(hwndChannel) : -1;
        }
    }
    else if ( !sithVoice_PlayThingVoice(pThing, hSnd, volume) )
    {
        guid = pActorData->voiceInfo.hSndChannel ? Sound_GetChannelGUID(pActorData->voiceInfo.hSndChannel) : -1;
    }

    if ( guid != -1 )
    {
        if ( curWait )
        {
            pCog->status = SITHCOG_STATUS_WAITING_SOUND_TO_STOP;
            pCog->statusParams[0] = guid;
        }

        sithCogExec_PushInt(pCog, guid);
    }
    else
    {
        sithCogExec_PushInt(pCog, -1);
    }
}

void J3DAPI sithVoice_SetThingVoiceHeads(SithCog* pCog)
{
    const char* apModelNames[4];
    const char* pMeshName;
    SithThing* pThing;
    SithActorInfo* pActor;
    int i;

    for ( i = 3; i >= 0; --i )
    {
        apModelNames[i] = sithCogExec_PopString(pCog); // Head Model Names
    }

    pMeshName = sithCogExec_PopString(pCog); // thing model head mesh name
    pThing = sithCogExec_PopThing(pCog);
    if ( !pThing )
    {
        return;
    }

#line 424
    SITH_ASSERTREL((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR));

    pActor = &pThing->thingInfo.actorInfo;
    for ( i = 0; i < 4; ++i )
    {
        pActor->voiceInfo.voiceHeadInfo.apSoundHeadModels[i] = sithModel_GetModel(apModelNames[i]);
    }

    pActor->voiceInfo.voiceHeadInfo.headMeshNum = sithThing_GetThingMeshIndex(pThing, pMeshName);
}

void J3DAPI sithVoice_SetVoiceHeadHeight(SithCog* pCog)
{
    unsigned int paHeights[4];
    unsigned int index;
    int i;

    for ( i = 3; i >= 0; --i )
    {
        paHeights[i] = sithCogExec_PopInt(pCog);
        paHeights[i] = STDMATH_CLAMP(paHeights[i], 0, 3);
    }

    index = sithCogExec_PopInt(pCog);
    index = STDMATH_CLAMP(index, 0, 3);

    for ( i = 0; i < 4; ++i )
    {
        sithVoice_aVoiceHeadHeights[index][i] = (uint8_t)paHeights[i]; // It should be ok to cast since it's clamped to 3
    }
}

void J3DAPI sithVoice_SetThingVoiceColor(SithCog* pCog)
{
    rdVector3 aHeadColors[4];
    SithThing* pThing;
    SithActorInfo* pActorInfo;
    int i;

    for ( i = 3; i >= 0; --i )
    {
        sithCogExec_PopVector(pCog, &aHeadColors[i]);
    }

    pThing = sithCogExec_PopThing(pCog);
#line 471
    SITH_ASSERTREL(pThing && ((pThing->type == SITH_THING_PLAYER) || (pThing->type == SITH_THING_ACTOR)));

    pActorInfo = &pThing->thingInfo.actorInfo;
    for ( i = 0; i < 4; ++i )
    {
        (&pActorInfo->voiceInfo.voiceColor.top)[i].red   = aHeadColors[i].red;
        (&pActorInfo->voiceInfo.voiceColor.top)[i].green = aHeadColors[i].green;
        (&pActorInfo->voiceInfo.voiceColor.top)[i].blue  = aHeadColors[i].blue;
        (&pActorInfo->voiceInfo.voiceColor.top)[i].alpha = 1.0f;
    }
}

void J3DAPI sithVoice_SetVoiceParams(SithCog* pCog)
{
    sithVoice_dword_52B9A8 = sithCogExec_PopInt(pCog);
    sithVoice_secHeadSwapInterval = sithCogExec_PopFlex(pCog);
}

int J3DAPI sithVoice_GetShowText(void)
{
    // TODO: refactor function to return bool
    return sithVoice_bShowText;
}

void J3DAPI sithVoice_ShowText(int bShow)
{
    // TODO: refactor tfunction to accept bool
    sithVoice_bShowText = bShow;
}

void J3DAPI sithVoice_AddSubtitle(int msecSoundLen, const char* pSoundFilename, const char* pSubtitleText, VGradiantColor* pSubtitleColor)
{
    const char* pCurLine = pSubtitleText;
    unsigned int curTime = stdPlatform_GetTimeMsec();
    float fontWidth;
    int numLines;
    unsigned int msecEndTime;
    unsigned int msecStartTime;
    int lineIdx;
    size_t textWidth;
    size_t lineWidth;
    char aCurVoiceLine[256];
    int numSubtitles;
    char aVoiceLines[256];
    unsigned int msecDuration;
    uint32_t width;
    const char* pLineEnd;
    float widthRatio;
    uint32_t height;
    int count;
    const char* pText;

    if ( !pSubtitleText || !pSoundFilename || !sithVoice_bShowText )
    {
        return;
    }

    if ( sithVoice_numSubtitleInfos > 0 && strcmp(pSoundFilename, sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].aSoundFileName) == 0 )
    {
        numSubtitles = sithVoice_numSubtitleInfos;
        memset(aCurVoiceLine, 0, sizeof(aCurVoiceLine));
        memset(aVoiceLines, 0, sizeof(aVoiceLines));

        while ( strcmp(pSoundFilename, sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].aSoundFileName) == 0
            && sithVoice_numSubtitleInfos > 0
            && curTime < sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].msecEndTime )
        {
            if ( strlen(aCurVoiceLine) == 0 )
            {
                strcpy(aCurVoiceLine, sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos - 1].aSubtitleText);
            }

            strcat(aVoiceLines, aCurVoiceLine);
            memset(aCurVoiceLine, 0, sizeof(aCurVoiceLine));
            --sithVoice_numSubtitleInfos;
        }

        if ( strcmp(aVoiceLines, pSubtitleText) == 0 )
        {
            msecStartTime = stdPlatform_GetTimeMsec();
            while ( sithVoice_numSubtitleInfos < numSubtitles )
            {
                msecDuration = sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime - sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecStartTime;
                sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecStartTime   = msecStartTime;
                sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime     = msecStartTime + msecDuration;
                sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecShowEndTime = sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime + SITHVOICE_MSEC_EXTRA_SUBTITLE_SHOW_DURATION;
                ++sithVoice_numSubtitleInfos;
            }

            return;
        }

        sithVoice_numSubtitleInfos = numSubtitles;
    }

    lineIdx = 0;
    while ( pCurLine )
    {
        pLineEnd = NULL;

        if ( pCurLine == pSubtitleText )
        {
            textWidth = rdFont_GetTextWidth(pSubtitleText, sithVoice_pTextFont);
            pText = pCurLine;
            stdDisplay_GetBackBufferSize(&width, &height);
            widthRatio = D3DVAL(width * SITHVOICE_TEXT_PADDING_X / (double)SITHVOICE_REF_WIDTH);
            fontWidth = 1.0f - widthRatio;

            count = 0;
            while ( pText )
            {
                pText = rdFont_GetWrapLine(pText, sithVoice_pTextFont, 2.0f * fontWidth - 1.0f);
                ++count;
            }
            numLines = count;

            msecStartTime = stdPlatform_GetTimeMsec();
        }
        else
        {
            msecStartTime = msecEndTime;
        }

        msecEndTime = msecStartTime + msecSoundLen / numLines;
        ++lineIdx;

        pLineEnd = rdFont_GetWrapLine(pCurLine, sithVoice_pTextFont, 2.0f * fontWidth - 1.0f);
        if ( sithVoice_numSubtitleInfos == STD_ARRAYLEN(sithVoice_aSubtitleInfos) )
        {
            sithVoice_PurgeDrawnSubtitles();
        }

        if ( !pLineEnd )
        {
            lineWidth = rdFont_GetTextWidth(pCurLine, sithVoice_pTextFont);
            strcpy(sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aSubtitleText, pCurLine);
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime = msecEndTime;
            pCurLine = NULL;
        }
        else
        {
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime = msecEndTime;
            strncpy(sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aSubtitleText, pCurLine, pLineEnd - pCurLine);
            pCurLine = pLineEnd;
        }

        sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecStartTime   = msecStartTime;
        sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecShowEndTime = sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime + SITHVOICE_MSEC_EXTRA_SUBTITLE_SHOW_DURATION;
        strcpy(sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aSoundFileName, pSoundFilename);

        if ( pSubtitleColor->top.red == -1.0f )
        {
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[0].red = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[0].green = 0.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[0].blue = 0.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[0].alpha = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[1].red = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[1].green = 0.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[1].blue = 0.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[1].alpha = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[2].red = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[2].green = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[2].blue = 0.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[2].alpha = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[3].red = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[3].green = 1.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[3].blue = 0.0f;
            sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[3].alpha = 1.0f;
        }
        else
        {
            int i;
            for ( i = 0; i < 4; ++i )
            {
                rdVector_Copy4(&sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[i], &(&pSubtitleColor->top)[i]);
            }
        }

        ++sithVoice_numSubtitleInfos;
    }
}

void J3DAPI sithVoice_PurgeDrawnSubtitles(void)
{
    int i;
    int j;

    memset(sithVoice_aSubtitleInfos, 0, sizeof(SithVoiceSubtitleInfo) * sithVoice_curSubtitleInfoNum);
    sithVoice_numSubtitleInfos = 0;

    for ( i = sithVoice_curSubtitleInfoNum; i < SITHVOICE_MAX_SUBTITLES; ++i )
    {
        sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecStartTime   = sithVoice_aSubtitleInfos[i].msecStartTime;
        sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecShowEndTime = sithVoice_aSubtitleInfos[i].msecShowEndTime;
        sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].msecEndTime     = sithVoice_aSubtitleInfos[i].msecEndTime;

        strcpy(sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aSubtitleText, sithVoice_aSubtitleInfos[i].aSubtitleText);
        strcpy(sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aSoundFileName, sithVoice_aSubtitleInfos[i].aSoundFileName);

        for ( j = 0; j < 4; ++j )
        {
            rdVector_Copy4(&sithVoice_aSubtitleInfos[sithVoice_numSubtitleInfos].aTextColors[j], &sithVoice_aSubtitleInfos[i].aTextColors[j]);
        }

        ++sithVoice_numSubtitleInfos;
    }

    memset(&sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum], 0, sizeof(SithVoiceSubtitleInfo) * (STD_ARRAYLEN(sithVoice_aSubtitleInfos) - sithVoice_curSubtitleInfoNum));
    sithVoice_curSubtitleInfoNum = 0;
}

void J3DAPI sithVoice_Draw(void)
{
    uint32_t width;
    unsigned int curTime;
    SithVoiceSubtitleInfo* pSubInfo;
    float y;
    int lineCount = 0;
    float lineHeight;
    float x;
    uint32_t height;

    lineHeight = (float)sithVoice_pTextFont->lineSpacing / RD_REF_HEIGHT;
    curTime    = stdPlatform_GetTimeMsec();
    if ( !sithVoice_bShowText )
    {
        return;
    }

    // Skip lines that expired
    while ( strlen(sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum].aSubtitleText) > 0
        && curTime > sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum].msecShowEndTime )
    {
        ++sithVoice_curSubtitleInfoNum;
    }

    if ( strlen(sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum].aSubtitleText) == 0 )
    {
        return;
    }

    stdDisplay_GetBackBufferSize(&width, &height);
    x = D3DVAL(width * SITHVOICE_TEXT_PADDING_X / (double)SITHVOICE_REF_WIDTH);
    y = D3DVAL(height * SITHVOICE_TEXT_PADDING_Y / (double)SITHVOICE_REF_HEIGHT);

    if ( curTime >= sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum].msecEndTime
        && strlen(sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum + 1].aSubtitleText) > 0 )
    {
        y = y - (float)(curTime - sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum].msecEndTime) * lineHeight / 1000.0f;
    }

    // Print up
    while ( lineCount < SITHVOICE_MAX_LINES_PER_DRAW )
    {
        pSubInfo = &sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum + lineCount];
        if ( strlen(pSubInfo->aSubtitleText) > 0 && curTime > sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum + lineCount].msecStartTime )
        {
            rdFont_SetFontColor(sithVoice_aSubtitleInfos[sithVoice_curSubtitleInfoNum + lineCount].aTextColors);
            rdFont_DrawTextLine(pSubInfo->aSubtitleText, x, y, rdCamera_pCurCamera->pFrustum->nearPlane, sithVoice_pTextFont, RDFONT_ALIGNLEFT);
            y = y + lineHeight;
        }
        else
        {
            break;
        }

        ++lineCount;
    }
}

int J3DAPI sithVoice_SyncVoiceState(DPID idTo, unsigned int outstream)
{
    uint8_t* pCurDSSMsgData_ = sithMulti_message.data;
    size_t i;
    size_t j;

    SITHDSS_PUSHINT8(sithVoice_bShowText);

    for ( i = 0; i < 4; ++i )
    {
        for ( j = 0; j < 4; ++j )
        {
            SITHDSS_PUSHUINT8(sithVoice_aVoiceHeadHeights[i][j]);
        }
    }

    SITHDSS_PUSHFLOAT(sithVoice_secHeadSwapInterval);
    SITHDSS_PUSHINT32(sithVoice_dword_52B9A8);

    sithMulti_message.type = SITHDSS_VOICESTATE;
    SITHDSS_ENDOUT;
    return sithMessage_SendMessage(&sithMulti_message, idTo, outstream, 1u);
}

int J3DAPI sithVoice_ProcessVoiceState(const SithMessage* pMsg)
{
    uint8_t* pCurDSSMsgData_;
    size_t i;
    size_t j;

    pCurDSSMsgData_ = (uint8_t*)&pMsg->data[0];
#line 773
    SITH_ASSERTREL(pMsg);

    sithVoice_bShowText = SITHDSS_POPUINT8();

    for ( i = 0; i < 4; ++i )
    {
        for ( j = 0; j < 4; ++j )
        {
            sithVoice_aVoiceHeadHeights[i][j] = SITHDSS_POPUINT8();
        }
    }

    sithVoice_secHeadSwapInterval = SITHDSS_POPFLOAT();

    sithVoice_dword_52B9A8 = SITHDSS_POPUINT32();

    SITHDSS_ENDIN;
    return 1;
}

const rdFont* J3DAPI sithVoice_GetTextFont(void)
{
    return sithVoice_pTextFont;
}
