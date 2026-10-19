#include "Sound.h"

#include <rdroid/Math/rdVector.h>

#include "AudioLib.h"
#include "Driver.h"


#include <std/General/stdUtil.h>





// General module constants
#define SOUND_INIT_SOUNDINFO_BUFFERSIZE 16

// Sound bank constants
#define SOUNDBANK_STATIC_NUM 0
#define SOUNDBANK_NORMAL_NUM 1
#define SOUNDBANK_NUMBANKS   2

// Sound playback constants
#define SOUND_MAXCREATERETRIES 4
#define SOUND_SERMAGIC 0x12345678u

// Sound load constants and macros
#define SOUND_LOADSTATICIDXMASK 0x12344321 // Mask for static sound index

// Sound handle constants & helper macros
#define SOUND_HANDLE_ENTROPY_MAX   1111111 // Note, max 555555 handles per handle type (sound or channel)
#define SOUND_HANDLE_MIN           1234
#define SOUND_HANDLE_MAX           (SOUND_HANDLE_MIN + SOUND_HANDLE_ENTROPY_MAX)

#define SOUND_IS_SOUNDINFOHANDLE(handle) ( (handle) >= SOUND_HANDLE_MIN && (handle) < SOUND_HANDLE_MAX && ((handle) & 1) == 0 )
#define SOUND_IS_CHANNELHANDLE(handle) ( (handle) >= SOUND_HANDLE_MIN && (handle) < SOUND_HANDLE_MAX && ((handle) & 1) != 0 )

// Logging macros
    //J3DLOG_DEBUG(Sound_pHS, format, ##__VA_ARGS__)

    //J3DLOG_STATUS(Sound_pHS, format, ##__VA_ARGS__)

    //J3DLOG_MESSAGE(Sound_pHS, format, ##__VA_ARGS__)

    //J3DLOG_WARNING(Sound_pHS, format, ##__VA_ARGS__)

    //J3DLOG_ERROR(Sound_pHS, format, ##__VA_ARGS__)

#define SOUNDLOG_FATAL(message) \
    Sound_pHS->pAssert(message, J3D_FILE, __LINE__);
    //J3DLOG_FATAL(Sound_pHS, message)

#define SOUND_ASSERTREL(condition) \
    J3D_ASSERTREL(condition, Sound_pHS)

// Sound module state enum
typedef enum eSoundModuleState
{
    SOUNDSTATE_NONE        = 0,
    SOUNDSTATE_INITIALIZED = 1,
    SOUNDSTATE_STARTUP     = 2,
    SOUNDSTATE_OPEN        = 3,
} SoundModuleState;

// Note don't change the int types of vars that are being serialized on save/restore

// Module state
static SoundGetThingInfoCallback Sound_pfGetThingInfoCallback = NULL;

// Sound fade vars
static SoundFade Sound_aFades[48] = { 0 };

// Soundbank vars
static int Sound_handleEntropy = 0;

// Sound and soundbank load vars
static uint8_t Sound_loadBuffer[10000] = { 0 };

// Module options
static int Sound_bNoSound3D = 0;

// Misc vars
static HWND Sound_hwnd = NULL;
static LPDIRECTSOUND Sound_pDirectSound = NULL;

// 3D sound vars
static SoundCalcListenerSoundMixFunc Sound_pfCalcListenerSoundMix = NULL;

static int Sound_pausedRefCount = 0;
static int Sound_bNoLipSync = 0;
static uint32_t Sound_msecPauseStartTime = 0;
static uint32_t Sound_msecElapsed = 0;
static int Sound_bGlobalFocusBuf = 0;

// Save/Restore vars
static char Sound_sndFilename[256] = { 0 };

static int Sound_bNoSoundCompression = 0;
static size_t Sound_maxChannels = 0;
static int Sound_bLoadEnabled = 0;

// Loaded sound data buffer vars
static size_t Sound_cacheBlockSize = 0x200000;
static float Sound_maxVolume = 1.0f;
static uint8_t* soundbank_apSoundCache[2] = { 0 };
static size_t soundbank_aUsedCacheSizes[2] = { 0 };
static size_t soundbank_aCacheSizes[2] = { 0 };
static size_t soundbank_lastImportedBankNum = 0;

// Loaded sound info vars
static SoundInfo* soundbank_apSoundInfos[2] = { 0 };
static int soundbank_aNumSounds[2] = { 0 };
static int soundbank_aSizeSounds[2] = { 0 };

// Playing sound vars
static tSoundChannel* Sound_apChannels = NULL;
static int Sound_numChannels = 0;
static size_t Sound_sizeChannels = 0;

// In-memory file buffer vars
static uint8_t* Sound_pMemfileBuf = NULL;
static size_t Sound_memfileSize = 0;
static size_t Sound_memfilePos = 0;

static tHostServices* Sound_pHS = NULL;
static SoundModuleState Sound_state = SOUNDSTATE_NONE;
static int Sound_bReverseSound = 0;
static rdVector3 Sound_curUpdatePos = { 0 };

tSoundHandle J3DAPI Sound_GenerateSoundHandle(void);
tSoundChannelHandle J3DAPI Sound_GenerateChannelHandle(void);

size_t J3DAPI Sound_GetFreeCache(size_t bankNum, size_t requiredSize);
void J3DAPI Sound_IncreaseFreeCache(size_t bankNum, size_t nSize);
size_t J3DAPI Sound_WriteSoundFilepathToBank(size_t bankNum, const char* pName, size_t len);
size_t J3DAPI Sound_GetDeltaTime(void);

uint8_t* J3DAPI Sound_GetSoundBufferData(LPDIRECTSOUNDBUFFER pDSBuf, size_t* pDataSize, uint32_t* pbCompressed);
int J3DAPI Sound_MemFileSeek(tFileHandle fh, int offset, int origin);
size_t J3DAPI Sound_MemFileRead(tFileHandle fh, void* pOutData, size_t nRead);
size_t J3DAPI Sound_MemFileWrite(tFileHandle fh, const void* pData, size_t size);
tFileHandle J3DAPI Sound_MemFileReset(void);
void J3DAPI Sound_MemFileFree(void);

void J3DAPI Sound_StopAllNonStaticSounds(void);

int J3DAPI Sound_ExportBank(tFileHandle fh, size_t bankNum);
int J3DAPI Sound_ImportBank(tFileHandle fh, size_t bankNum);
int J3DAPI Sound_ReadFile(tFileHandle fh, void* pData, size_t size);

int J3DAPI Sound_Initialize(tHostServices* pHS)
{
    Sound_pHS = pHS;
    if ( Sound_state != SOUNDSTATE_NONE )
    {
        Sound_pHS->pErrorPrint("Sound_Initialize: module already in initialized state.");
        return 1;
    }

    Sound_state = SOUNDSTATE_INITIALIZED;
    return 0;
}

void J3DAPI Sound_Uninitialize(void)
{
    if ( Sound_state == SOUNDSTATE_INITIALIZED )
    {
        Sound_state = SOUNDSTATE_NONE;
    }

    Sound_pHS = NULL;
}

int J3DAPI Sound_Startup(void)
{
    if ( Sound_state == SOUNDSTATE_STARTUP )
    {
        Sound_pHS->pErrorPrint("Sound_Startup: module already in startup state.");
        return 0;
    }

    if ( Sound_state != SOUNDSTATE_INITIALIZED )
    {
        if ( Sound_pHS )
        {
            Sound_pHS->pErrorPrint("Sound_Startup: module hasn't been initialized, yet.");
        }

        return 0;
    }

    Sound_state = SOUNDSTATE_STARTUP;
    return 1;
}

void J3DAPI Sound_Shutdown(void)
{
    if ( Sound_state == SOUNDSTATE_STARTUP )
    {
        Sound_state = SOUNDSTATE_INITIALIZED;
    }
}

int J3DAPI Sound_Open(SoundOpenFlags flags, size_t maxSoundChannels, SoundGetThingInfoCallback pfGetThingInfo, SoundCalcListenerSoundMixFunc pfCalcListenerSoundMix, HWND hwnd)
{
    if ( Sound_state == SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Open: module already in open state.");
        return 0;
    }

    if ( Sound_state != SOUNDSTATE_STARTUP )
    {
        Sound_pHS->pErrorPrint("Sound_Open: module not in startup state.");
        return 0;
    }

    Sound_bNoSound3D             = (flags & SOUNDOPEN_NO3DSOUND) != 0;
    Sound_bGlobalFocusBuf        = (flags & SOUNDOPEN_GLOBALFOCUS) != 0;
    Sound_hwnd                   = hwnd;
    Sound_pDirectSound           = (flags & SOUNDOPEN_HWNDISDIRECTSOUND) != 0 ? (LPDIRECTSOUND)hwnd : 0;
    Sound_pfCalcListenerSoundMix = pfCalcListenerSoundMix;

    // Open sound driver
    if ( !SoundDriver_Open(Sound_bNoSound3D, Sound_bGlobalFocusBuf, Sound_hwnd, Sound_pDirectSound, Sound_pfCalcListenerSoundMix, Sound_GetSoundBufferData, Sound_pHS->pErrorPrint) )
    {
        return 0;
    }

    Sound_msecElapsed = SoundDriver_GetTimeMsec();
    memset(Sound_aFades, 0, sizeof(Sound_aFades));

    Sound_pausedRefCount         = 0;
    Sound_handleEntropy          = 0;
    Sound_maxChannels            = maxSoundChannels;
    Sound_pfGetThingInfoCallback = pfGetThingInfo;
    Sound_bNoSoundCompression    = (flags & SOUNDOPEN_NOSOUNDCOMPRESSION) != 0;
    Sound_bNoLipSync             = (flags & SOUNDOPEN_NOLIPSYNC) != 0;

    Sound_Reset(1);
    Sound_EnableLoad(1);

    Sound_state = SOUNDSTATE_OPEN;
    return 1;
}

void J3DAPI Sound_Close(void)
{
    if ( Sound_state == SOUNDSTATE_OPEN )
    {
        Sound_Reset(1);
        SoundDriver_Close();

        if ( Sound_apChannels )
        {
            Sound_pHS->pFree(Sound_apChannels);
            Sound_apChannels = NULL;
        }

        Sound_sizeChannels = 0;
        Sound_numChannels  = Sound_sizeChannels;
        Sound_state = SOUNDSTATE_STARTUP;
    }
}

int J3DAPI Sound_Save(tFileHandle fh)
{
    if ( Sound_state < SOUNDSTATE_INITIALIZED )
    {
        Sound_pHS->pErrorPrint("Sound_Save: module is not initialized.\n");
        return 0;
    }

    {
    uint32_t curvalue;

    int channelNumber = Sound_numChannels;
    tSoundChannel* pCurrentChannel = &Sound_apChannels[channelNumber];

    // section marker (header magic)
    curvalue = SOUND_SERMAGIC;
    if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
    {
        return 0;
    }

    curvalue = Sound_pausedRefCount;
    if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
    {
        return 0;
    }

    curvalue = Sound_GetDeltaTime();
    if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
    {
        return 0;
    }

    curvalue = SoundDriver_GetTimeMsec() - Sound_msecPauseStartTime;
    if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
    {
        return 0;
    }

    while ( (--pCurrentChannel, channelNumber-- != 0) )
    {
        // TODO: Save also 2D sounds 
        if ( pCurrentChannel->handle != SOUND_INVALIDHANDLE
            && ((pCurrentChannel->flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
                || (pCurrentChannel->flags & SOUND_CHANNEL_FAR) != 0)
            && (pCurrentChannel->flags & SOUND_CHANNEL_3DSOUND) != 0 )
        {
            SoundInfo* pSndInfo = Sound_GetSoundInfo(pCurrentChannel->hSnd);
            if ( pSndInfo )
            {
                uint8_t* aSndCache = soundbank_apSoundCache[pSndInfo->bankNum];

                // TODO: Make sure name max len should be less or equal than STD_ARRAYLEN(Sound_sndFilename)
                curvalue = strlen((const char*)&aSndCache[pSndInfo->filePathOffset]) + 1;
                if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
                {
                    return 0;
                }

                // Write filename
                if ( Sound_pHS->pFileWrite(fh, &aSndCache[pSndInfo->filePathOffset], curvalue) != curvalue )
                {
                    return 0;
                }

                // Write sound bank num
                curvalue = pSndInfo->bankNum;
                if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
                {
                    return 0;
                }

                // Write sound index
                curvalue = pSndInfo->idx;
                if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
                {
                    return 0;
                }

                curvalue = pSndInfo->hSnd;
                if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->handle, sizeof(pCurrentChannel->handle)) != sizeof(pCurrentChannel->handle) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->priority, sizeof(pCurrentChannel->priority)) != sizeof(pCurrentChannel->priority) )
                {

                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->thingId, sizeof(pCurrentChannel->thingId)) != sizeof(pCurrentChannel->thingId) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->playPos.x, sizeof(pCurrentChannel->playPos.x)) != sizeof(pCurrentChannel->playPos.x) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->playPos.y, sizeof(pCurrentChannel->playPos.y)) != sizeof(pCurrentChannel->playPos.y) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->playPos.z, sizeof(pCurrentChannel->playPos.z)) != sizeof(pCurrentChannel->playPos.z) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->volume, sizeof(pCurrentChannel->volume)) != sizeof(pCurrentChannel->volume) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->pitch, sizeof(pCurrentChannel->pitch)) != sizeof(pCurrentChannel->pitch) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->sampleRate, sizeof(pCurrentChannel->sampleRate)) != sizeof(pCurrentChannel->sampleRate) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->flags, sizeof(pCurrentChannel->flags)) != sizeof(pCurrentChannel->flags) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->guid, sizeof(pCurrentChannel->guid)) != sizeof(pCurrentChannel->guid) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->minRadius, sizeof(pCurrentChannel->minRadius)) != sizeof(pCurrentChannel->minRadius) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->maxRadius, sizeof(pCurrentChannel->maxRadius)) != sizeof(pCurrentChannel->maxRadius) )
                {
                    return 0;
                }

                if ( Sound_pHS->pFileWrite(fh, &pCurrentChannel->envflags, sizeof(pCurrentChannel->envflags)) != sizeof(pCurrentChannel->envflags) )
                {
                    return 0;
                }
            }
        }
    }

    // Marks end of list
    curvalue = 0;
    if ( Sound_pHS->pFileWrite(fh, &curvalue, sizeof(curvalue)) != sizeof(curvalue) )
    {
        return 0;
    }

    if ( Sound_pHS->pFileWrite(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    if ( Sound_pHS->pFileWrite(fh, Sound_aFades, sizeof(Sound_aFades)) != sizeof(Sound_aFades) )
    {
        return 0;
    }

    return 1;
}
}

int J3DAPI Sound_Restore(tFileHandle fh)
{
    uint32_t value;

    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        // Sound module is not opened, read out the sound list anyway so the file read offset is moved to the end of the sound section
        Sound_pHS->pErrorPrint("Sound_Restore: module is not open.\n");

        if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
        {
            return 0;
        }

        if ( value == SOUND_SERMAGIC )
        {
            if ( Sound_pHS->pFileRead(fh, &Sound_pausedRefCount, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            Sound_pausedRefCount = 0; // ???

            if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
            {
                return 0;
            }

            Sound_msecElapsed = SoundDriver_GetTimeMsec() - value;

            if ( Sound_pHS->pFileRead(fh, &Sound_msecPauseStartTime, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            Sound_msecPauseStartTime += SoundDriver_GetTimeMsec();

            // Read sound file name length
            if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
            {
                return 0;
            }

            while ( value != 0 )
            {
                if ( (int)value <= (int)STD_ARRAYLEN(Sound_sndFilename) )
                {
                    uint32_t bankNum;
                    int32_t priority;
                    int32_t thingId;
                    float posX;
                    float posY;
                    float posZ;
                    float volume;
                    float pitch;
                    float sampleRate;
                    int32_t guid;
                    float minRadius;
                    float maxRadius;
                    tSoundChannelFlag chflags;
                    SoundEnvFlags envflags;

                    // Read filename
                    if ( Sound_pHS->pFileRead(fh, Sound_sndFilename, value) != value )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &bankNum, sizeof(bankNum)) != sizeof(bankNum) )
                    {
                        return 0;
                    }

                    // Read sound index
                    if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
                    {
                        return 0;
                    }

                    // Read sound handle
                    if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
                    {
                        return 0;
                    }

                    // Read channel handle
                    if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &priority, sizeof(priority)) != sizeof(priority) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &thingId, sizeof(thingId)) != sizeof(thingId) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &posX, sizeof(posX)) != sizeof(posX) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &posY, sizeof(posY)) != sizeof(posY) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &posZ, sizeof(posZ)) != sizeof(posZ) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &volume, sizeof(volume)) != sizeof(volume) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &pitch, sizeof(pitch)) != sizeof(pitch) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &sampleRate, sizeof(sampleRate)) != sizeof(sampleRate) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &chflags, sizeof(int32_t)) != sizeof(int32_t) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &guid, sizeof(guid)) != sizeof(guid) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &minRadius, sizeof(minRadius)) != sizeof(minRadius) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &maxRadius, sizeof(maxRadius)) != sizeof(maxRadius) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &envflags, sizeof(int32_t)) != sizeof(int32_t) )
                    {
                        return 0;
                    }

                }

                // Read next sound filename
                if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
                {
                    return 0;
                }

            }

            // Read handle entropy and fades
            if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            // Read fades
            if ( Sound_pHS->pFileRead(fh, Sound_aFades, sizeof(Sound_aFades)) != sizeof(Sound_aFades) )
            {
                return 0;
            }

        }
        else
        {
            // We're not at the ser sound position, roll the read position back for sizeof(value)
            Sound_pHS->pFileSeek(fh, -(int)sizeof(value), 1);
            return 0;
        }

        return 1;
    }
    else
    {
        rdVector3 listnerPos;

        if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
        {
            return 0;
        }

        if ( value == SOUND_SERMAGIC )
        {
            // Set initial listener position
            listnerPos.x = 100000.0f;
            listnerPos.y = 100000.0f;
            listnerPos.z = 100000.0f;
            Sound_StopAllSounds();
            Sound_Update(&listnerPos, NULL, NULL, NULL, NULL);

            if ( Sound_pHS->pFileRead(fh, &Sound_pausedRefCount, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            Sound_pausedRefCount = 0; // ???

            if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
            {
                return 0;
            }

            Sound_msecElapsed = SoundDriver_GetTimeMsec() - value;

            if ( Sound_pHS->pFileRead(fh, &Sound_msecPauseStartTime, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            Sound_msecPauseStartTime += SoundDriver_GetTimeMsec();

            // Read sound file name length
            if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
            {
                return 0;
            }

            while ( value != 0 )
            {
                if ( (int)value <= (int)STD_ARRAYLEN(Sound_sndFilename) )
                {
                    uint32_t bankNum;
                    int32_t priority;
                    int32_t thingId;
                    float posX;
                    float posY;
                    float posZ;
                    float volume;
                    float pitch;
                    float sampleRate;
                    int32_t guid;
                    float minRadius;
                    float maxRadius;
                    tSoundChannelFlag chflags;
                    SoundEnvFlags envflags;
                    tSoundHandle hSnd;

                    // Read filename
                    if ( Sound_pHS->pFileRead(fh, Sound_sndFilename, value) != value )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &bankNum, sizeof(bankNum)) != sizeof(bankNum) )
                    {
                        return 0;
                    }

                    // Read sound index
                    if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
                    {
                        return 0;
                    }

                    // Read sound handle
                    if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
                    {
                        return 0;
                    }

                    if ( bankNum == SOUNDBANK_STATIC_NUM )
                    {
                        hSnd = Sound_LoadStatic(Sound_sndFilename, &value);
                    }
                    else
                    {
                        hSnd = Sound_Load(Sound_sndFilename, &value);
                    }

                    // Read channel handle
                    if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &priority, sizeof(priority)) != sizeof(priority) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &thingId, sizeof(thingId)) != sizeof(thingId) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &posX, sizeof(posX)) != sizeof(posX) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &posY, sizeof(posY)) != sizeof(posY) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &posZ, sizeof(posZ)) != sizeof(posZ) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &volume, sizeof(volume)) != sizeof(volume) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &pitch, sizeof(pitch)) != sizeof(pitch) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &sampleRate, sizeof(sampleRate)) != sizeof(sampleRate) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &chflags, sizeof(int32_t)) != sizeof(int32_t) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &guid, sizeof(guid)) != sizeof(guid) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &minRadius, sizeof(minRadius)) != sizeof(minRadius) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &maxRadius, sizeof(maxRadius)) != sizeof(maxRadius) )
                    {
                        return 0;
                    }

                    if ( Sound_pHS->pFileRead(fh, &envflags, sizeof(int32_t)) != sizeof(int32_t) )
                    {
                        return 0;
                    }

                    if ( (chflags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) == (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND) )
                    {
                        tSoundChannel* pChannel = Sound_GetChannel(Sound_PlayPos(hSnd, volume, priority, chflags, posX, posY, posZ, guid, minRadius, maxRadius, envflags));
                        if ( pChannel )
                        {
                            pChannel->thingId = thingId;
                            pChannel->flags |= SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND;
                        }
                    }
                    else if ( (chflags & SOUND_CHANNEL_3DSOUND) != 0 )
                    {
                        chflags |= SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING;
                        Sound_PlayPos(hSnd, volume, priority, chflags, posX, posY, posZ, guid, minRadius, maxRadius, envflags);
                    }
                }

                // Read next sound filename
                if ( Sound_pHS->pFileRead(fh, &value, sizeof(value)) != sizeof(value) )
                {
                    return 0;
                }

            }

            // Read handle entropy and fades
            if ( Sound_pHS->pFileRead(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
            {
                return 0;
            }

            // Read fades
            if ( Sound_pHS->pFileRead(fh, Sound_aFades, sizeof(Sound_aFades)) != sizeof(Sound_aFades) )
            {
                return 0;
            }

        }
        else
        {
            // We're not at the ser sound position, roll the read position back for sizeof(value)
            Sound_pHS->pFileSeek(fh, -(int)sizeof(value), 1);
            return 0;
        }

        return 1;
    }
}

void J3DAPI Sound_Pause(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Pause: module is not open.\n");
        return;
    }

    if ( Sound_pausedRefCount == 0 )
    {
        // Pause all channels
        size_t i = Sound_numChannels;
        tSoundChannel* pChannel = &Sound_apChannels[i];

        Sound_msecPauseStartTime = SoundDriver_GetTimeMsec();

        while ( (pChannel--, i-- != 0) )
        {
            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                if ( (pChannel->flags & SOUND_CHANNEL_FAR) == 0 )
                {
                    pChannel->flags |= SOUND_CHANNEL_PAUSED;
                    SoundDriver_Stop(pChannel->pDSoundBuffer);
                }
            }

        }
    }

    // Increment pause reference count
    ++Sound_pausedRefCount;
}

void J3DAPI Sound_Resume(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Resume: module is not open.\n");
        return;
    }

    if ( Sound_pausedRefCount == 0 )
    {
        return;
    }

    if ( --Sound_pausedRefCount <= 0 )
    {
        // Resume all paused channels
        size_t i = Sound_numChannels;
        tSoundChannel* pChannel = &Sound_apChannels[i];

        Sound_msecElapsed += SoundDriver_GetTimeMsec() - Sound_msecPauseStartTime;

        while ( (pChannel--, i-- != 0) )
        {
            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                if ( (pChannel->flags & SOUND_CHANNEL_PAUSED) != 0 )
                {
                    pChannel->flags &= ~SOUND_CHANNEL_PAUSED;
                    SoundDriver_Play(
                        pChannel->pDSoundBuffer,
                        (pChannel->flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
                    );
                }
            }

        }
    }
}

void J3DAPI Sound_Set3DGlobals(float distanceFactor, float minDistance, float maxDistance, float folloverFactor, float dopplerFactor)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Set3DGlobals: module is not open.\n");
        return;
    }

    SoundDriver_SetGlobals(distanceFactor, minDistance, maxDistance, folloverFactor, dopplerFactor);
}

int J3DAPI Sound_Has3DHW(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Has3DHW: module is not open.\n");
        return 0;
    }

    return SoundDriver_Has3DHW();
}

void J3DAPI Sound_Set3DHWState(int bNo3DSound)
{
    // Store original file functions
    tFileReadFunc pFileRead   = Sound_pHS->pFileRead;
    tFileWriteFunc pCurFileWrite = Sound_pHS->pFileWrite;
    tFileSeekFunc pFileSeek   = Sound_pHS->pFileSeek;

    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Set3DHWState: module is not open.\n");
        return;
    }

    if ( !Sound_Has3DHW() )
    {
        return;
    }

    if ( (bNo3DSound && !Sound_bNoSound3D) || (!bNo3DSound && Sound_bNoSound3D) )
    {
        return;
    }

    // Toggle 3D sound state
    Sound_bNoSound3D ^= 1u;

    // Set write functions to memory file
    Sound_pHS->pFileWrite = Sound_MemFileWrite;

    // Save current sound state to memory file
    Sound_Save(Sound_MemFileReset());

    // Restore original file functions
    Sound_pHS->pFileWrite = pCurFileWrite;

    // Reopen sound driver
    Sound_StopAllSounds();
    SoundDriver_Close();

    SoundDriver_Open(
        Sound_bNoSound3D,
        Sound_bGlobalFocusBuf,
        Sound_hwnd,
        Sound_pDirectSound,
        Sound_pfCalcListenerSoundMix,
        Sound_GetSoundBufferData,
        Sound_pHS->pErrorPrint
    );

    // Set read functions to memory file
    Sound_pHS->pFileRead = Sound_MemFileRead;
    Sound_pHS->pFileSeek = Sound_MemFileSeek;

    // Restore sound state from memory file
    Sound_Restore(Sound_MemFileReset());

    // Restore original file functions
    Sound_pHS->pFileRead = pFileRead;
    Sound_pHS->pFileSeek = pFileSeek;
    Sound_MemFileFree();

}

int J3DAPI Sound_Get3DHWState(void)
{
    if ( Sound_state != SOUNDSTATE_OPEN )
    {
        Sound_pHS->pErrorPrint("Sound_Get3DHWState: module is not open.\n");
        return 0;
    }

    if ( !Sound_Has3DHW() )
    {
        return 0;
    }

    return Sound_bNoSound3D == 0;
}

size_t J3DAPI Sound_GetMemoryUsage(void)
{
    return soundbank_aCacheSizes[SOUNDBANK_STATIC_NUM]
        + soundbank_aCacheSizes[SOUNDBANK_NORMAL_NUM]
        + (sizeof(SoundInfo) * (soundbank_aSizeSounds[SOUNDBANK_STATIC_NUM] + soundbank_aSizeSounds[SOUNDBANK_NORMAL_NUM]))
        + (Sound_sizeChannels * sizeof(tSoundChannel));
}

tSoundHandle J3DAPI Sound_Load(const char* pFilepath, uint32_t* sndIdx)
{
    const uint8_t* pSoundData;
    int nRead;
    SoundInfo* pSndInfo;
    size_t curPathPos;
    uint32_t extraDataOffset;
    tFileHandle fh;
    const uint8_t* pFilename;
    int wavType;
    size_t bankNum;
    size_t j;
    uint32_t soundDataOffset;
    int i;

    // Extract filename from file path
    curPathPos = strlen(pFilepath);
    pFilename  = (const uint8_t*)&pFilepath[curPathPos];

    // Set bank number; for a static load the actual index is stored in the second element
    bankNum = sndIdx && *sndIdx == SOUND_LOADSTATICIDXMASK ? (sndIdx = (uint32_t*)sndIdx[1], SOUNDBANK_STATIC_NUM) : SOUNDBANK_NORMAL_NUM;

    while ( (--pFilename, curPathPos-- != 0) )
    {
        if ( *pFilename == '/' || *pFilename == '\\' )
        {
            break;
        }
    }

    // Skip '/' or '\'
    ++pFilename;

    // Search for sound in the soundbank
    for ( i = 0; i < SOUNDBANK_NUMBANKS; ++i )
    {
        j = soundbank_aNumSounds[i];
        pSndInfo = &soundbank_apSoundInfos[i][j];

        while ( (pSndInfo--, j-- != 0) )
        {
            if ( soundbank_apSoundCache[i]
                && streqi((const char*)pFilename, (const char*)&soundbank_apSoundCache[i][pSndInfo->nameOffset]) )
            {
                // Found sound in the bank, return it's handle and index
                if ( sndIdx )
                {
                    *sndIdx = pSndInfo->idx;
                }
                return pSndInfo->hSnd;
            }
        }
    }

    // Ok not found, let's load it

    if ( !Sound_bLoadEnabled )
    {
        Sound_pHS->pErrorPrint("Sound_Load: Can't load %s because loads are not currently allowed.\n", pFilepath);
        return SOUND_INVALIDHANDLE;
    }

    fh = Sound_pHS->pFileOpen(pFilepath, "rb");
    if ( !fh )
    {
        Sound_pHS->pErrorPrint("Sound_Load: Can't load %s.\n", pFilepath);
        return SOUND_INVALIDHANDLE;
    }

    // Read WAV header into buffer
    nRead = Sound_pHS->pFileRead(fh, Sound_loadBuffer, STD_ARRAYLEN(Sound_loadBuffer));
    if ( nRead < 50 ) // 50 - is WAV header size?
    {
        return SOUND_INVALIDHANDLE;
    }

    // Allocate sound info array if not already allocated or if we need to increase the size
    while ( soundbank_aNumSounds[bankNum] >= soundbank_aSizeSounds[bankNum] )
    {
        SoundInfo* aSndInfos;

        if ( !soundbank_aSizeSounds[bankNum] )
        {
            soundbank_aSizeSounds[bankNum] = SOUND_INIT_SOUNDINFO_BUFFERSIZE;
        }
        else
        {
            soundbank_aSizeSounds[bankNum] *= 2; // double size
        }

        aSndInfos = (SoundInfo*)Sound_pHS->pRealloc(soundbank_apSoundInfos[bankNum], sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
        if ( !aSndInfos )
        {
            Sound_pHS->pErrorPrint("Sound_Load: Couldn't realloc %d bytes.", sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
            Sound_Reset(1);
            return SOUND_INVALIDHANDLE;
        }

        soundbank_apSoundInfos[bankNum] = aSndInfos;
    }

    // Parse WAV header
    pSndInfo = &soundbank_apSoundInfos[bankNum][soundbank_aNumSounds[bankNum]];
    pSoundData = AudioLib_ParseWaveFileHeader(
        Sound_loadBuffer,
        &wavType,
        &pSndInfo->sampleRate,
        &pSndInfo->sempleBitSize,
        &pSndInfo->numChannels,
        NULL,
        &pSndInfo->dataSize,
        &extraDataOffset,
        &soundDataOffset
    );

    if ( wavType == 3 || wavType == 6 || wavType == 5 ) // 6  - IndyWV,  3 - WAV, 5 - FORM AIFFCOM
    {
        // Set last imported bank number.. TODO: This is kind of hacky shouldn't we set it to soundbank_lastImportedBankNum = bankNum?
        if ( soundbank_lastImportedBankNum == SOUNDBANK_STATIC_NUM )
        {
            soundbank_lastImportedBankNum = SOUNDBANK_NORMAL_NUM;
        }

        // Assign sound info
        pSndInfo->hSnd           = Sound_GenerateSoundHandle();
        pSndInfo->bankNum        = bankNum;
        pSndInfo->filePathOffset = Sound_WriteSoundFilepathToBank(pSndInfo->bankNum, pFilepath, strlen(pFilepath) + 1); // + 1 is terminating zero

        // Check if we have enough space in the sound cache to write the sound data
        if ( !pSndInfo->filePathOffset )
        {
            goto error;
        }

        // Assign name offset & compressed flag
        pSndInfo->nameOffset  = pSndInfo->filePathOffset + (pFilename - (const uint8_t*)pFilepath);
        pSndInfo->bCompressed = wavType == 6; // 6 - IndyWV

        // Move file pointer to sound data offset
        Sound_pHS->pFileSeek(fh, pSoundData - Sound_loadBuffer, 0);

        if ( pSndInfo->bCompressed || Sound_bNoSoundCompression )
        {
            // Compressed Sound data read

            pSndInfo->dataOffset = Sound_GetFreeCache(pSndInfo->bankNum, pSndInfo->dataSize);
            if ( !pSndInfo->dataOffset )
            {
                goto error;
            }

            if ( Sound_pHS->pFileRead(fh, &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset], pSndInfo->dataSize) != pSndInfo->dataSize )
            {
                return SOUND_INVALIDHANDLE;
            }
        }
        else
        {
            size_t dataSize;
            tAudioCompressorState compressor;
            size_t compressedSize;

            // Uncompressed sound data read

            dataSize = pSndInfo->dataSize;
            pSndInfo->dataOffset = Sound_GetFreeCache(pSndInfo->bankNum, pSndInfo->dataSize + dataSize); // Reserve space for original data size + compressed data size
            if ( !pSndInfo->dataOffset )
            {
                goto error;
            }

            // Read uncompressed data
            if ( Sound_pHS->pFileRead(fh, &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset] + dataSize, pSndInfo->dataSize) != pSndInfo->dataSize )
            {
                return SOUND_INVALIDHANDLE;
            }

            // Compress read data
            AudioLib_ResetCompressor(&compressor);
            compressedSize = AudioLib_Compress(
                &compressor,
                ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->compressedData,
                &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset] + dataSize,
                pSndInfo->dataSize,
                pSndInfo->numChannels
            ) + sizeof(uint32_t);     //  sizeof((uint32_t) is size of tAudioCompressedData::uncompressedSize

            ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize = pSndInfo->dataSize;

            // Now assign compressed data size
            pSndInfo->dataSize = compressedSize;

            // Increase free cache size to size of data size
            Sound_IncreaseFreeCache(pSndInfo->bankNum, pSndInfo->dataSize + dataSize - compressedSize);
            pSndInfo->bCompressed = 1;
        }

        // We're done close file and do final load post process
        Sound_pHS->pFileClose(fh);
        pSndInfo->pLipSyncData = NULL;

        if ( sndIdx )
        {
            pSndInfo->idx = *sndIdx;
        }
        else
        {
            pSndInfo->idx = SOUND_NOSOUNDINDEX; // default idx as sound idx was not provided
        }

        ++soundbank_aNumSounds[bankNum];
        return pSndInfo->hSnd;
    }

    // Unsupported WAV type
    Sound_pHS->pErrorPrint("Sound_Load: %s is not a WAV or WV file.\n", pFilepath);

error:
    // Not a WAV file, or not enough space in the sound cache for the file path or the sound data
    Sound_pHS->pFileClose(fh);
    return SOUND_INVALIDHANDLE;
}

tSoundHandle J3DAPI Sound_LoadStatic(const char* pFilename, uint32_t* sndIdx)
{
    tSoundHandle hwndSnd;

    uint32_t aSndIdx[2] = { SOUND_LOADSTATICIDXMASK, 0 };
    aSndIdx[1] = (uint32_t)sndIdx;

    hwndSnd = Sound_Load(pFilename, aSndIdx);
    soundbank_lastImportedBankNum = SOUNDBANK_STATIC_NUM;
    return hwndSnd;
}

void J3DAPI Sound_Reset(int bResetAllBanks)
{
    int bankNum;

    if ( bResetAllBanks )
    {
        Sound_StopAllSounds();

        for ( bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
        {
            if ( soundbank_apSoundInfos[bankNum] )
            {
                int i;

                for ( i = 0; i < soundbank_aNumSounds[bankNum]; ++i )
                {
                    if ( soundbank_apSoundInfos[bankNum][i].pLipSyncData )
                    {
                        Sound_pHS->pFree(soundbank_apSoundInfos[bankNum][i].pLipSyncData);
                    }
                }

                Sound_pHS->pFree(soundbank_apSoundInfos[bankNum]);
                soundbank_apSoundInfos[bankNum] = NULL;
            }

            soundbank_aSizeSounds[bankNum] = 0;
            soundbank_aNumSounds[bankNum]  = 0;

            if ( soundbank_apSoundCache[bankNum] )
            {
                Sound_pHS->pFree(soundbank_apSoundCache[bankNum]);
                soundbank_apSoundCache[bankNum] = NULL;
            }

            soundbank_aCacheSizes[bankNum]     = 0;
            soundbank_aUsedCacheSizes[bankNum] = 0;
        }

        Sound_handleEntropy = 0;
    }
    else
    {
        Sound_StopAllNonStaticSounds();

        if ( soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM] )
        {
            int i;

            for ( i = 0; i < soundbank_aNumSounds[SOUNDBANK_NORMAL_NUM]; ++i )
            {
                if ( soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM][i].pLipSyncData )
                {
                    Sound_pHS->pFree(soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM][i].pLipSyncData);
                }
            }

            Sound_pHS->pFree(soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM]);
            soundbank_apSoundInfos[SOUNDBANK_NORMAL_NUM] = NULL;
        }

        soundbank_aNumSounds[SOUNDBANK_NORMAL_NUM] = soundbank_aSizeSounds[SOUNDBANK_NORMAL_NUM] = 0;

        if ( soundbank_apSoundCache[SOUNDBANK_NORMAL_NUM] )
        {
            Sound_pHS->pFree(soundbank_apSoundCache[SOUNDBANK_NORMAL_NUM]);
            soundbank_apSoundCache[SOUNDBANK_NORMAL_NUM] = NULL;
        }

        soundbank_aUsedCacheSizes[SOUNDBANK_NORMAL_NUM] = soundbank_aCacheSizes[SOUNDBANK_NORMAL_NUM] = 0;
    }

    soundbank_lastImportedBankNum = SOUNDBANK_STATIC_NUM;
    Sound_bLoadEnabled = 1;
}

void J3DAPI Sound_EnableLoad(int bEnable)
{
    Sound_bLoadEnabled = bEnable;
}

size_t J3DAPI Sound_ExportStaticBank(tFileHandle fh)
{
    return Sound_ExportBank(fh, SOUNDBANK_STATIC_NUM);
}

int J3DAPI Sound_ImportStaticBank(tFileHandle fh)
{
    return Sound_ImportBank(fh, SOUNDBANK_STATIC_NUM);
}

size_t J3DAPI Sound_ExportNormalBank(tFileHandle fh)
{
    return Sound_ExportBank(fh, SOUNDBANK_NORMAL_NUM);
}

int J3DAPI Sound_ImportNormalBank(tFileHandle fh)
{
    return Sound_ImportBank(fh, SOUNDBANK_NORMAL_NUM);
}

int J3DAPI Sound_SkipSoundFileSection(tFileHandle fh)
{
    uint32_t sizeInfo;

    uint32_t numSounds;
    if ( Sound_pHS->pFileRead(fh, &numSounds, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    
    if ( Sound_pHS->pFileRead(fh, &sizeInfo, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    Sound_pHS->pFileSeek(fh, sizeof(SoundInfo) * numSounds, 1); // skip sound info list
    Sound_pHS->pFileSeek(fh, sizeInfo, 1);         // skip sound data list
    Sound_pHS->pFileSeek(fh, sizeof(uint32_t), 1); // skip next handle
    return 1;
}

tSoundHandle J3DAPI Sound_GetSoundHandle(uint32_t idx)
{
    {
        int bankNum;
        for ( bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
        {
            int i = soundbank_aNumSounds[bankNum];
            SoundInfo* pSndInfo = &soundbank_apSoundInfos[bankNum][i];

            while ( (pSndInfo--, i-- != 0) )
            {
                if ( pSndInfo->hSnd && pSndInfo->idx == idx )
                {
                    return pSndInfo->hSnd;
                }
            }
        }
    }

    return SOUND_INVALIDHANDLE;
}

size_t J3DAPI Sound_GetSoundIndex(tSoundHandle hSnd)
{
    {
        int bankNum;
        for ( bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
        {
            int i = soundbank_aNumSounds[bankNum];
            SoundInfo* pSndInfo = &soundbank_apSoundInfos[bankNum][i];

            if ( hSnd == SOUND_INVALIDHANDLE )
            {
                return SOUND_INVALISOUNDINFOINDEX;
            }

            while ( pSndInfo--, i-- != 0 )
            {
                if ( pSndInfo->hSnd == hSnd )
                {
                    return pSndInfo->idx;
                }
            }
        }
    }

    return SOUND_INVALISOUNDINFOINDEX;
}

tSoundChannelHandle J3DAPI Sound_GetChannelHandle(int guid)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE && pChannel->guid == guid )
        {
            return pChannel->handle;
        }
    }

    return SOUND_INVALIDHANDLE;
}

int J3DAPI Sound_GetChannelGUID(tSoundChannelHandle hChannel)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( !hChannel )
    {
        return SOUND_INVALIDGUID;
    }

    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle == hChannel )
        {
            return pChannel->guid;
        }
    }

    return SOUND_INVALIDGUID;
}

/* Linker-discarded (no caller in either build): debug 0x0052DA78, the setter twin of GetChannelGUID. Name from Jones3DRE. */
void J3DAPI Sound_SetChannelGUID(tSoundChannelHandle hChannel, int guid)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( !hChannel )
    {
        return;
    }

    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle == hChannel )
        {
            pChannel->guid = guid;
            return;
        }
    }
}

const char* J3DAPI Sound_GetSoundFilename(tSoundHandleType handle)
{
    SoundInfo* pSndInfo;
    tSoundChannel* pChan = Sound_GetChannel(handle);
    if ( pChan )
    {
        pSndInfo = Sound_GetSoundInfo(pChan->hSnd);
    }
    else
    {
        pSndInfo = Sound_GetSoundInfo(handle);
    }

    if ( !pSndInfo )
    {
        return NULL;
    }

    return (const char*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->nameOffset];
}

size_t J3DAPI Sound_GetLengthMsec(tSoundHandleType handle)
{
    uint32_t dataSize;

    SoundInfo* pSndInfo;
    tSoundChannel* pCurChannel = Sound_GetChannel(handle);
    if ( pCurChannel )
    {
        pSndInfo = Sound_GetSoundInfo(pCurChannel->hSnd);
    }
    else
    {
        pSndInfo = Sound_GetSoundInfo(handle);
    }

    if ( !pSndInfo )
    {
        return 0;
    }

    if ( pSndInfo->bCompressed )
    {
        dataSize = ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize; // decompressed size is stored in the first 4 bytes of the compressed data
    }
    else
    {
        dataSize = pSndInfo->dataSize;
    }

    if ( !pSndInfo->sampleRate || pSndInfo->sempleBitSize < 8 || !pSndInfo->numChannels )
    {
        Sound_pHS->pErrorPrint("Sound_GetLengthMsec: Bad header data for handle: %p (rate:%d bits:%d chans:%d)\n", handle, pSndInfo->sampleRate, pSndInfo->sempleBitSize, pSndInfo->numChannels);
        return 0;
    }

    return 1000 * dataSize / ((pSndInfo->sempleBitSize >> 3) * pSndInfo->numChannels) / pSndInfo->sampleRate;
}

void J3DAPI Sound_SetMaxVolume(float maxVolume)
{
    if ( maxVolume < 0.0 )
    {
        maxVolume = 0.0f;
    }
    else if ( maxVolume > 1.0 )
    {
        maxVolume = 1.0f;
    }

    if ( maxVolume != Sound_maxVolume )
    {
        size_t i = Sound_numChannels;
        tSoundChannel* pChannel = &Sound_apChannels[i];

        Sound_maxVolume = maxVolume;
        SoundDriver_SetMaxVolume(Sound_maxVolume);

        while ( pChannel--, i-- != 0 )
        {
            if ( pChannel->handle != SOUND_INVALIDHANDLE )
            {
                SoundDriver_SetVolume(pChannel->pDSoundBuffer, pChannel->volume * Sound_maxVolume);
            }
        }
    }
}

float J3DAPI Sound_GetMaxVolume(void)
{
    return Sound_maxVolume;
}

tSoundChannelHandle J3DAPI Sound_Play(tSoundHandle hSnd, float volume, float pan, int priority, tSoundChannelFlag flags, int guid)
{
    SoundInfo* pSndInfo;
    tSoundChannel* pChannel;
    tSoundChannel* pLowPriChannel;
    tSoundChannel* pExistingSndChannel;
    LPDIRECTSOUNDBUFFER pSBuffer = NULL;

    if ( Sound_state != SOUNDSTATE_OPEN
        || Sound_pausedRefCount && (flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( SoundDriver_Use3DCaps() && (flags & SOUND_CHANNEL_3DSOUND) == 0 )
    {
        flags |= SOUND_CHANNEL_USE3DCAPS;
    }

    pSndInfo = Sound_GetSoundInfo(hSnd);
    if ( !pSndInfo )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( (flags & SOUND_CHANNEL_PLAYONCE) != 0 )
    {
        pExistingSndChannel = Sound_GetChannelBySoundHandle(hSnd);
        if ( pExistingSndChannel )
        {
            return SOUND_INVALIDHANDLE;
        }
    }

    // Allocate sound channel list if not already allocated
    if ( !Sound_apChannels )
    {
        Sound_apChannels = (tSoundChannel*)Sound_pHS->pMalloc(Sound_maxChannels * sizeof(tSoundChannel));
        if ( !Sound_apChannels )
        {
            return SOUND_INVALIDHANDLE;
        }

        memset(Sound_apChannels, 0, Sound_maxChannels * sizeof(tSoundChannel));
        Sound_numChannels = Sound_sizeChannels = Sound_maxChannels;
    }

    // First scan for a free channel while tracking lowest priority
    {
        int i;

        pChannel = Sound_apChannels;
        pLowPriChannel = NULL;
        for ( i = 0; i < Sound_numChannels; ++i, ++pChannel )
        {
            // Track lowest priority channel seen so far
            if ( pChannel->priority < priority )
            {
                pLowPriChannel = pChannel;
            }

            // Found a free channel - use it immediately
            if ( pChannel->handle == SOUND_INVALIDHANDLE )
            {
                goto found_channel;
            }
        }

        // No free channel found, check if we have a lower priority one to reuse
        if ( !pLowPriChannel )
        {
            return SOUND_INVALIDHANDLE;
        }

        Sound_StopChannel(pLowPriChannel->handle);
        pChannel = pLowPriChannel;
    }

found_channel:
    pExistingSndChannel = Sound_GetChannelBySoundHandle(hSnd);
    if ( pExistingSndChannel )
    {
        pSBuffer = pExistingSndChannel->pDSoundBuffer;
        if ( (pExistingSndChannel->flags & SOUND_CHANNEL_GLOBALFOCUS) != 0 )
        {
            flags |= SOUND_CHANNEL_GLOBALFOCUS;
        }
    }

    if ( pSndInfo->bCompressed )
    {
        flags |= SOUND_CHANNEL_COMPRESSED;
    }

    pChannel->hSnd       = hSnd;
    pChannel->thingId    = 0;
    pChannel->priority   = priority;
    pChannel->volume     = volume;
    pChannel->pitch      = 1.0f;
    pChannel->sampleRate = (float)pSndInfo->sampleRate;
    pChannel->flags      = flags;
    pChannel->guid       = guid;
    pChannel->minRadius  = 0.0f;
    pChannel->maxRadius  = 0.0f;
    pChannel->envflags   = 0;

    // If start far playing then don't play sound
    if ( (flags & SOUND_CHANNEL_STARTFAR) != 0 )
    {
        pChannel->flags &= ~SOUND_CHANNEL_STARTFAR;
        pChannel->flags |= SOUND_CHANNEL_FAR;
        pChannel->pDSoundBuffer = NULL;
    }
    else
    {
        // Create sound buffer and play it
        int nRetries = 0;
        do
        {
            // Try to create and play buffer, else remove low priority channel and re-try, max SOUND_MAXCREATERETRIES times
            pSBuffer = NULL;
            pChannel->pDSoundBuffer = SoundDriver_CreateAndPlay(pSndInfo->sampleRate, pSndInfo->sempleBitSize, pSndInfo->numChannels, &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset], pSndInfo->dataSize, &pChannel->flags, pSBuffer);
            if ( !pChannel->pDSoundBuffer )
            {
                tSoundChannel* pChannelToRemove = NULL;
                int curPriority = priority;
                size_t i = Sound_numChannels;
                tSoundChannel* pCurChannel = &Sound_apChannels[i];

                while ( pCurChannel--, i-- != 0 )
                {
                    if ( pCurChannel->handle != SOUND_INVALIDHANDLE )
                    {
                        if ( pCurChannel->priority < curPriority )
                        {
                            curPriority      = pCurChannel->priority;
                            pChannelToRemove = pCurChannel;
                        }
                    }
                }

                if ( !pChannelToRemove )
                {
                    // No channels to remove, so just break out of the loop
                    nRetries = SOUND_MAXCREATERETRIES;
                }
                else
                {
                    SoundDriver_Release(pChannelToRemove->pDSoundBuffer);
                    pChannelToRemove->pDSoundBuffer = NULL;
                    pChannelToRemove->handle        = SOUND_INVALIDHANDLE;
                    pChannelToRemove->hSnd          = SOUND_INVALIDHANDLE;
                    pChannelToRemove->thingId       = 0;
                    ++nRetries;
                }
            }
        } while ( !pChannel->pDSoundBuffer && nRetries < SOUND_MAXCREATERETRIES );

        // If we still don't have a buffer, give up
        if ( nRetries >= SOUND_MAXCREATERETRIES )
        {
            // Couldn't create sound buffer 
            pChannel->handle  = SOUND_INVALIDHANDLE;
            pChannel->hSnd    = SOUND_INVALIDHANDLE;
            pChannel->thingId = 0;
            return SOUND_INVALIDHANDLE;
        }

        // We have a buffer, set the volume and pan
        SoundDriver_SetVolume(pChannel->pDSoundBuffer, volume * Sound_maxVolume);
        SoundDriver_SetPan(pChannel->pDSoundBuffer, pan);
    }

    // Set the sound handle
    pChannel->handle = Sound_GenerateChannelHandle();
    return pChannel->handle;
}

tSoundChannelHandle J3DAPI Sound_PlayPos(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag channelflags, float x, float y, float z, int guid, float minRadius, float maxRadius, SoundEnvFlags envflags)
{
    tSoundChannelFlag currentnewFlags;
    float distSq;
    tSoundChannelHandle hwndChannel;
    tSoundChannel* pChannel;

    if ( Sound_state != SOUNDSTATE_OPEN || Sound_pausedRefCount )
    {
        return SOUND_INVALIDHANDLE;
    }

    currentnewFlags = 0;
    if ( !SoundDriver_Update3DSound(NULL, x, y, z, volume, 1.0f, &currentnewFlags, minRadius, maxRadius, envflags) )
    {
        if ( (channelflags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
        {
            return SOUND_INVALIDHANDLE;
        }

        channelflags |= SOUND_CHANNEL_STARTFAR;
    }

    if ( SoundDriver_Use3DCaps()
        && (channelflags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)
        && (distSq = ((Sound_curUpdatePos.x - x) * (Sound_curUpdatePos.x - x))
        + ((Sound_curUpdatePos.y - y) * (Sound_curUpdatePos.y - y))
        + ((Sound_curUpdatePos.z - z) * (Sound_curUpdatePos.z - z))) <= 0.25f )
    {
        channelflags |= SOUND_CHANNEL_USE3DCAPS;
    }

    hwndChannel = Sound_Play(hSnd, volume, 0.0f, priority, (tSoundChannelFlag)(channelflags | SOUND_CHANNEL_3DSOUND), guid);
    pChannel = Sound_GetChannel(hwndChannel);
    if ( !hwndChannel )
    {
        return SOUND_INVALIDHANDLE;
    }

    SoundDriver_SetPosAndVelocity(pChannel->pDSoundBuffer, x, y, z, 0.0f, 0.0f, 0.0f, minRadius, maxRadius, envflags);

    pChannel->playPos.x = x;
    pChannel->playPos.y = y;
    pChannel->playPos.z = z;
    pChannel->envflags  = envflags;
    pChannel->minRadius = minRadius;
    pChannel->maxRadius = maxRadius;

    if ( !SoundDriver_Update3DSound(&pChannel->pDSoundBuffer, x, y, z, volume, 1.0f, &pChannel->flags, minRadius, maxRadius, pChannel->envflags) )
    {
        Sound_StopChannel(hwndChannel);
    }

    return hwndChannel;
}

tSoundChannelHandle J3DAPI Sound_PlayThing(tSoundHandle hSnd, float volume, int priority, tSoundChannelFlag flags, int thingID, int guid, float minRadius, float maxRadius)
{
    tSoundChannel* pChan;
    SoundThingInfo thingInfo;
    tSoundChannelHandle hCurChannel;

    if ( Sound_state != SOUNDSTATE_OPEN || Sound_pausedRefCount )
    {
        return SOUND_INVALIDHANDLE;
    }

    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return SOUND_INVALIDHANDLE;
    }

    thingInfo.pos.x    = thingInfo.pos.y = thingInfo.pos.z = 0.0f;
    thingInfo.envflags = 0;

    if ( Sound_pfGetThingInfoCallback )
    {
        Sound_pfGetThingInfoCallback(thingID, &thingInfo);
    }

    // Check if playonce flag is set and if so, check if the sound is already playing
    if ( (flags & SOUND_CHANNEL_PLAYTHINGONCE) != 0 )
    {
        hCurChannel = Sound_numChannels;
        pChan = &Sound_apChannels[hCurChannel];

        while ( (pChan--, hCurChannel-- != 0) )
        {
            if ( pChan->hSnd == hSnd && pChan->thingId == thingID )
            {
                return SOUND_INVALIDHANDLE;
            }
        }
    }

    {
        tSoundChannelFlag newFlags = 0;
        if ( !SoundDriver_Update3DSound(NULL, thingInfo.pos.x, thingInfo.pos.y, thingInfo.pos.z, volume, 1.0f, &newFlags, minRadius, maxRadius, thingInfo.envflags) )
        {
            if ( (flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) != (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
            {
                return SOUND_INVALIDHANDLE;
            }

            flags |= SOUND_CHANNEL_STARTFAR;
        }
    }

    if ( SoundDriver_Use3DCaps() )
    {
        float distSq;
        if ( (distSq = ((Sound_curUpdatePos.x - thingInfo.pos.x) * (Sound_curUpdatePos.x - thingInfo.pos.x))
            + ((Sound_curUpdatePos.y - thingInfo.pos.y) * (Sound_curUpdatePos.y - thingInfo.pos.y))
            + ((Sound_curUpdatePos.z - thingInfo.pos.z) * (Sound_curUpdatePos.z - thingInfo.pos.z))) <= 0.25f )
        {
            flags |= SOUND_CHANNEL_USE3DCAPS;
            if ( (flags & (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING)) == (SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING) )
            {
                flags |= SOUND_CHANNEL_DISABLEHWMIXING;
            }
        }
    }

    hCurChannel = Sound_Play(hSnd, volume, 0.0f, priority, (tSoundChannelFlag)(flags | SOUND_CHANNEL_3DSOUND | (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)), guid);
    if ( !hCurChannel )
    {
        return SOUND_INVALIDHANDLE;
    }

    pChan = Sound_GetChannel(hCurChannel);
    pChan->thingId       = thingID;
    pChan->minRadius     = minRadius;
    pChan->maxRadius     = maxRadius;
    pChan->playPos.x     = thingInfo.pos.x;
    pChan->playPos.y     = thingInfo.pos.y;
    pChan->playPos.z     = thingInfo.pos.z;
    pChan->envflags      = thingInfo.envflags;

    SoundDriver_SetPosAndVelocity(pChan->pDSoundBuffer, thingInfo.pos.x, thingInfo.pos.y, thingInfo.pos.z, thingInfo.velocity.x, thingInfo.velocity.y, thingInfo.velocity.z, minRadius, maxRadius, pChan->envflags);

    if ( !SoundDriver_Update3DSound(&pChan->pDSoundBuffer, thingInfo.pos.x, thingInfo.pos.y, thingInfo.pos.z, volume, 1.0f, &pChan->flags, minRadius, maxRadius, pChan->envflags) )
    {
        Sound_StopChannel(hCurChannel);
    }

    return hCurChannel;
}

void J3DAPI Sound_StopChannel(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_apChannels;
    int i;

    if ( !pChannel || !hChannel )
    {
        return;
    }

    for ( i = 0; i < Sound_numChannels; ++i, ++pChannel )
    {
        if ( pChannel->handle == hChannel )
        {
            SoundDriver_Release(pChannel->pDSoundBuffer);
            pChannel->pDSoundBuffer = NULL;
            pChannel->handle        = SOUND_INVALIDHANDLE;
            pChannel->hSnd          = SOUND_INVALIDHANDLE;
            pChannel->thingId       = 0;
            return;
        }
    }
}

void J3DAPI Sound_StopAllSounds(void)
{
    tSoundChannel* pChannel = Sound_apChannels;
    int i;

    if ( !pChannel )
    {
        return;
    }

    for ( i = 0; i < Sound_numChannels; ++i, ++pChannel )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE )
        {
            SoundDriver_Release(pChannel->pDSoundBuffer);
            pChannel->pDSoundBuffer = NULL;
            pChannel->handle        = SOUND_INVALIDHANDLE;
            pChannel->hSnd          = SOUND_INVALIDHANDLE;
            pChannel->thingId       = 0;
        }
    }
}

void J3DAPI Sound_StopThing(int thingId, tSoundHandleType handle)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_StopChannel(handle);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        Sound_pHS->pErrorPrint("Sound_StopThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate channels in reverse order and stop all channels with specific sound handle and thing id
    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            SoundDriver_Release(pChannel->pDSoundBuffer);
            pChannel->pDSoundBuffer = NULL;
            pChannel->handle        = SOUND_INVALIDHANDLE;
            pChannel->hSnd          = SOUND_INVALIDHANDLE;
            pChannel->thingId       = 0;
        }
    }
}

void J3DAPI Sound_SetVolume(tSoundChannelHandle hChannel, float volume)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        if ( volume <= 0.0f )
        {
            Sound_StopChannel(hChannel);
        }
        else
        {
            pChannel->volume = volume;
            if ( (pChannel->flags & (SOUND_CHANNEL_3DSOUND | SOUND_CHANNEL_HWMIXING)) == SOUND_CHANNEL_3DSOUND )
            {
                return;
            }

            SoundDriver_SetVolume(pChannel->pDSoundBuffer, volume * Sound_maxVolume);
        }
    }
}

/* Linker-discarded (no caller in either build): debug 0x0052E82D, between SetVolume and FadeVolume. Name from Jones3DRE. */
float J3DAPI Sound_GetVolume(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return SoundDriver_GetVolume(pChannel->pDSoundBuffer);
    }

    return 0.0f;
}

void J3DAPI Sound_FadeVolume(tSoundChannelHandle hChannel, float volume, float secFadeTime)
{
    tSoundChannel* pChannel;
    tSoundChannel* pStopChannel;
    SoundFade* pFade;
    SoundFade* pCurFade = &Sound_aFades[STD_ARRAYLEN(Sound_aFades)];
    size_t i = STD_ARRAYLEN(Sound_aFades);
    int nRemaining;

    uint32_t curTime      = Sound_GetDeltaTime();
    uint32_t msecFadeTime = (uint32_t)(secFadeTime * 1000.0);

    pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel )
    {
        return;
    }

    {
        while ( (pCurFade--, i-- != 0) )
        {
            if ( !pCurFade->hChannel )
            {
                pCurFade->hChannel      = hChannel;
                pCurFade->bPitch        = 0;
                pCurFade->msecStartTime = curTime;
                pCurFade->msecEndTime   = curTime + msecFadeTime;
                pCurFade->startValue    = SoundDriver_GetVolume(pChannel->pDSoundBuffer);
                pCurFade->endValue      = volume;

                pChannel->flags |= SOUND_CHANNEL_VOLUMEFADE;
                return;
            }
        }
    }

    // No free fades, so find the one with the longest time remaining
    {
    nRemaining = 999999999;
    pFade = NULL;
    pCurFade = &Sound_aFades[STD_ARRAYLEN(Sound_aFades)];
    i = STD_ARRAYLEN(Sound_aFades);
    {
        while ( (pCurFade--, i-- != 0) )
        {
            if ( pCurFade->hChannel )
            {
                if ( (int)(pCurFade->msecEndTime - curTime) > nRemaining )
                {
                    nRemaining = pCurFade->msecEndTime - curTime;
                    pFade = pCurFade;
                }
            }
            else
            {
                pFade = pCurFade;
                break;
            }
        }
    }

    if ( !pFade )
    {
        Sound_pHS->pErrorPrint("Sound_FadeVolume: No free fades!\n");
        return;
    }

    // Stop the fade on the channel that was fading
    pStopChannel = Sound_GetChannel(pFade->hChannel);
    Sound_SetVolume(pFade->hChannel, pFade->endValue);

    // Set up the new fade
    pFade->hChannel      = hChannel;
    pFade->bPitch        = 0;
    pFade->msecStartTime = curTime;
    pFade->msecEndTime   = curTime + msecFadeTime;
    pFade->startValue    = SoundDriver_GetVolume(pChannel->pDSoundBuffer);
    pFade->endValue      = volume;

    pStopChannel->flags &= ~SOUND_CHANNEL_VOLUMEFADE;
    pChannel->flags |= SOUND_CHANNEL_VOLUMEFADE;
}
}

void J3DAPI Sound_SetVolumeThing(int thingId, tSoundHandleType handle, float volume)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_SetVolume(handle, volume);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        Sound_pHS->pErrorPrint("Sound_SetVolumeThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate channels in reverse order to apply new volume
    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_SetVolume(pChannel->handle, volume);
        }
    }
}

void J3DAPI Sound_FadeVolumeThing(int thingId, tSoundHandleType handle, float startVolume, float endVolume)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_FadeVolume(handle, startVolume, endVolume);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        Sound_pHS->pErrorPrint("Sound_FadeVolumeThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate channels in reverse order to apply volume fade
    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_FadeVolume(pChannel->handle, startVolume, endVolume);
        }
    }
}

int J3DAPI Sound_IsThingFadingVol(int thingId, tSoundHandleType handle)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        return (Sound_GetChannelFlags(handle) & SOUND_CHANNEL_VOLUMEFADE) != 0;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        Sound_pHS->pErrorPrint("Sound_IsThingFadingVol: Don't know what 'sound' is: %p\n", handle);
        return 0;
    }

    // Iterate through channels in reverse order to find matching sound with volume fade
    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle)
            && (pChannel->flags & SOUND_CHANNEL_VOLUMEFADE) != 0 )
        {
            return 1;
        }
    }

    return 0;
}

/* SetPan/GetPan: discarded by the retail linker (no caller in either image), restored in full from the debug
   build (0x0052ED0F, 0x0052ED3F, between IsThingFadingVol and SetPitch). Names inferred
   text / OpenJones3D. */
void J3DAPI Sound_SetPan(tSoundChannelHandle hChannel, float pan)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        SoundDriver_SetPan(pChannel->pDSoundBuffer, pan);
    }
}

float J3DAPI Sound_GetPan(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return SoundDriver_GetPan(pChannel->pDSoundBuffer);
    }

    return 0.0f;
}

void J3DAPI Sound_SetPitch(tSoundChannelHandle hChannel, float pitch)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        pChannel->pitch = pitch;
        SoundDriver_SetFrequency(pChannel->pDSoundBuffer, pitch * pChannel->sampleRate);
    }
}

float J3DAPI Sound_GetPitch(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return pChannel->pitch;
    }

    return 0.0f;
}

void J3DAPI Sound_FadePitch(tSoundChannelHandle hChannel, float pitch, float secFadeTime)
{
    tSoundChannel* pChannel;
    tSoundChannel* pStopChannel;
    SoundFade* pFade;
    SoundFade* pCurFade = &Sound_aFades[STD_ARRAYLEN(Sound_aFades)];
    size_t i = STD_ARRAYLEN(Sound_aFades);
    int nRemaining;

    uint32_t curTime      = Sound_GetDeltaTime();
    uint32_t msecFadeTime = (uint32_t)(secFadeTime * 1000.0);

    pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel )
    {
        return;
    }

    {
        while ( (pCurFade--, i-- != 0) )
        {
            if ( !pCurFade->hChannel )
            {
                pCurFade->hChannel      = hChannel;
                pCurFade->bPitch        = 1;
                pCurFade->msecStartTime = curTime;
                pCurFade->msecEndTime   = curTime + msecFadeTime;
                pCurFade->startValue    = Sound_GetPitch(hChannel);
                pCurFade->endValue      = pitch;

                pChannel->flags |= SOUND_CHANNEL_PITCHFADE;
                return;
            }
        }
    }

    // No free fades, so find the one with the longest time remaining
    {
    nRemaining = 999999999;
    pFade = NULL;
    pCurFade = &Sound_aFades[STD_ARRAYLEN(Sound_aFades)];
    i = STD_ARRAYLEN(Sound_aFades);
    {
        while ( (pCurFade--, i-- != 0) )
        {
            if ( pCurFade->hChannel )
            {
                if ( (int)(pCurFade->msecEndTime - curTime) > nRemaining )
                {
                    nRemaining = pCurFade->msecEndTime - curTime;
                    pFade = pCurFade;
                }
            }
            else
            {
                pFade = pCurFade;
                break;
            }
        }
    }

    if ( !pFade )
    {
        Sound_pHS->pErrorPrint("Sound_FadePitch: No free fades!\n");
        return;
    }

    // Stop the fade on the channel that was fading
    pStopChannel = Sound_GetChannel(pFade->hChannel);
    Sound_SetPitch(pFade->hChannel, pFade->endValue);

    // Set up the new fade
    pFade->hChannel      = hChannel;
    pFade->bPitch        = 1;
    pFade->msecStartTime = curTime;
    pFade->msecEndTime   = curTime + msecFadeTime;
    pFade->startValue    = Sound_GetPitch(hChannel);
    pFade->endValue      = pitch;

    pStopChannel->flags &= ~SOUND_CHANNEL_PITCHFADE;
    pChannel->flags |= SOUND_CHANNEL_PITCHFADE;
}
}

void J3DAPI Sound_SetPitchThing(int thingId, tSoundHandleType handle, float pitch)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        Sound_SetPitch(handle, pitch);
        return;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        Sound_pHS->pErrorPrint("Sound_SetPitchThing: Don't know what 'sound' is: %p\n", handle);
        return;
    }

    // Iterate through channels in reverse order
    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle) )
        {
            Sound_SetPitch(pChannel->handle, pitch);
        }
    }
}

/* Discarded by the retail linker (its only caller, sithSoundMixer_IsThingFadingPitch, was discarded too);
   restored in full from the debug build (0x0052F09B, between SetPitchThing and GetChannelFlags): the
   IsThingFadingVol text with the pitch-fade flag. */
int J3DAPI Sound_IsThingFadingPitch(int thingId, tSoundHandleType handle)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( SOUND_IS_CHANNELHANDLE(handle) )
    {
        return (Sound_GetChannelFlags(handle) & SOUND_CHANNEL_PITCHFADE) != 0;
    }

    if ( handle != SOUND_ALLTHINGSOUNDHANDLE && !SOUND_IS_SOUNDINFOHANDLE(handle) )
    {
        Sound_pHS->pErrorPrint("Sound_IsThingFadingPitch: Don't know what 'sound' is: %p\n", handle);
        return 0;
    }

    while ( (pChannel--, i-- != 0) )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE
            && (pChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
            && pChannel->thingId == thingId
            && (handle == SOUND_ALLTHINGSOUNDHANDLE || pChannel->hSnd == handle)
            && (pChannel->flags & SOUND_CHANNEL_PITCHFADE) != 0 )
        {
            return 1;
        }
    }

    return 0;
}

tSoundChannelFlag J3DAPI Sound_GetChannelFlags(tSoundChannelHandle hChannel)
{
    tSoundChannel* pChannel = Sound_GetChannel(hChannel);
    if ( pChannel )
    {
        return pChannel->flags;
    }

    return 0;
}

void J3DAPI Sound_Update(const rdVector3* pPos, const rdVector3* pVelocity, rdVector3* pTopOrient, rdVector3* pFrontOrient, const rdVector3* pRightOrient)
{
    int count;
    tSoundChannel* pCurChannel;

    {
        int i;
        SoundFade* pFade;

        for ( i = 0, pFade = Sound_aFades; i < 48; i++, pFade++ )
        {
            if ( pFade->hChannel )
            {
                int dtime = Sound_GetDeltaTime();
                tSoundChannel* pChannel = Sound_GetChannel(pFade->hChannel);
                float newValue;
                if ( !pChannel )
                {
                    pFade->hChannel = 0;
                    continue;
                }

                if ( dtime >= pFade->msecEndTime )
                {
                    if ( !pFade->bPitch )
                    {
                        if ( pFade->endValue == 0.0 )
                        {
                            Sound_StopChannel(pFade->hChannel);
                        }
                        else
                        {
                            Sound_SetVolume(pFade->hChannel, pFade->endValue);
                            pChannel->flags &= ~SOUND_CHANNEL_VOLUMEFADE;
                        }
                    }
                    else
                    {
                        Sound_SetPitch(pFade->hChannel, pFade->endValue);
                        pChannel->flags &= ~SOUND_CHANNEL_PITCHFADE;
                    }

                    pFade->hChannel = 0;
                    continue;
                }

                newValue = (pFade->endValue - pFade->startValue)
                    * (dtime - pFade->msecStartTime)
                    / (pFade->msecEndTime - pFade->msecStartTime)
                    + pFade->startValue;

                if ( newValue != 0.0f )
                {
                    if ( !pFade->bPitch )
                    {
                        Sound_SetVolume(pFade->hChannel, newValue);
                    }
                    else
                    {
                        Sound_SetPitch(pFade->hChannel, newValue);
                    }

                    if ( newValue == pFade->endValue )
                    {
                        pFade->hChannel = 0;
                    }
                }
            }
        }
    }

    count = Sound_numChannels;
    pCurChannel = &Sound_apChannels[count];

    // Retail negates the caller's orientation vectors in place
    if ( pTopOrient && !Sound_bReverseSound )
    {
        rdVector_Neg3Acc(pTopOrient);
    }

    if ( pFrontOrient && !Sound_bReverseSound )
    {
        rdVector_Neg3Acc(pFrontOrient);
    }

    if ( pPos )
    {
        Sound_curUpdatePos.x = pPos->x;
        Sound_curUpdatePos.y = pPos->y;
        Sound_curUpdatePos.z = pPos->z;
    }

    SoundDriver_SetListenerPosition(pPos, pVelocity, pTopOrient, pFrontOrient, pRightOrient);

    while ( pCurChannel--, count-- != 0 )
    {
        if ( pCurChannel->handle != SOUND_INVALIDHANDLE && (pCurChannel->flags & SOUND_CHANNEL_3DSOUND) != 0 )
        {
            SoundThingInfo thngSndInfo;
            if ( (pCurChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) != 0
                && Sound_pfGetThingInfoCallback
                && Sound_pfGetThingInfoCallback(pCurChannel->thingId, &thngSndInfo) )
            {
                pCurChannel->playPos.x  = thngSndInfo.pos.x;
                pCurChannel->playPos.y  = thngSndInfo.pos.y;
                pCurChannel->playPos.z  = thngSndInfo.pos.z;
                pCurChannel->envflags = thngSndInfo.envflags;

                SoundDriver_SetPosAndVelocity(
                    pCurChannel->pDSoundBuffer,
                    thngSndInfo.pos.x,
                    thngSndInfo.pos.y,
                    thngSndInfo.pos.z,
                    thngSndInfo.velocity.x,
                    thngSndInfo.velocity.y,
                    thngSndInfo.velocity.z,
                    pCurChannel->minRadius,
                    pCurChannel->maxRadius,
                    pCurChannel->envflags
                );
            }

            if ( !SoundDriver_Update3DSound(
                &pCurChannel->pDSoundBuffer,
                pCurChannel->playPos.x,
                pCurChannel->playPos.y,
                pCurChannel->playPos.z,
                pCurChannel->volume,
                pCurChannel->pitch,
                &pCurChannel->flags,
                pCurChannel->minRadius,
                pCurChannel->maxRadius,
                pCurChannel->envflags) )
            {
                Sound_StopChannel(count); // Retail bug: passes the loop counter, not the channel handle
            }
            else if ( (pCurChannel->flags & SOUND_CHANNEL_RESTART) != 0 )
            {
                pCurChannel->flags |= SOUND_CHANNEL_LOOP | SOUND_CHANNEL_PLAYING;
                pCurChannel->flags &= ~SOUND_CHANNEL_FAR;

                if ( (pCurChannel->flags & (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND)) == (SOUND_CHANNEL_THING | SOUND_CHANNEL_3DSOUND) )
                {
                    tSoundChannelHandle hChannel;

                    tSoundChannel channel;

                    memcpy(&channel, pCurChannel, sizeof(tSoundChannel));
                    pCurChannel->handle   = SOUND_INVALIDHANDLE;
                    pCurChannel->hSnd     = SOUND_INVALIDHANDLE;
                    pCurChannel->thingId  = 0;
                    if ( pCurChannel->pDSoundBuffer )
                    {
                        SoundDriver_Release(pCurChannel->pDSoundBuffer);
                        pCurChannel->pDSoundBuffer = NULL;
                    }

                    hChannel = Sound_PlayThing(channel.hSnd, channel.volume, channel.priority, channel.flags, channel.thingId, channel.guid, channel.minRadius, channel.maxRadius);
                    if ( hChannel )
                    {
                        {
                            size_t i = Sound_numChannels;
                            tSoundChannel* pChannel = &Sound_apChannels[i];

                            while ( pChannel--, i-- != 0 )
                            {
                                if ( pChannel->handle == hChannel )
                                {
                                    pChannel->flags &= ~SOUND_CHANNEL_RESTART;
                                    pChannel->handle = channel.handle;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Sound_pHS->pErrorPrint("Sound_Update: Error restarting distant sound.\n");
                    }
                }
                else
                {
                    tSoundChannelHandle hChannel;

                    tSoundChannel channel;

                    memcpy(&channel, pCurChannel, sizeof(tSoundChannel));
                    pCurChannel->handle = SOUND_INVALIDHANDLE;
                    if ( pCurChannel->pDSoundBuffer )
                    {
                        SoundDriver_Release(pCurChannel->pDSoundBuffer);
                        pCurChannel->pDSoundBuffer = NULL;
                    }

                    hChannel = Sound_PlayPos(
                        channel.hSnd,
                        channel.volume,
                        channel.priority,
                        channel.flags,
                        channel.playPos.x,
                        channel.playPos.y,
                        channel.playPos.z,
                        channel.guid,
                        channel.minRadius,
                        channel.maxRadius,
                        channel.envflags
                    );

                    if ( hChannel )
                    {
                        {
                            size_t i = Sound_numChannels;
                            tSoundChannel* pChannel = &Sound_apChannels[i];

                            while ( pChannel--, i-- != 0 )
                            {
                                if ( pChannel->handle == hChannel )
                                {
                                    pChannel->flags &= ~SOUND_CHANNEL_RESTART;
                                    pChannel->handle = channel.handle;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Sound_pHS->pErrorPrint("Sound_Update: Error restarting distant sound.\n");
                    }
                }
            }
        }
    }

    SoundDriver_ListenerCommitDeferred();

    // Remove channels that have finished playing
    {
        tSoundChannel* pChannel;
        int i;

        pChannel = Sound_apChannels;
        if ( !pChannel )
        {
            return;
        }

        for ( i = 0; i < Sound_numChannels; ++i, ++pChannel )
        {
            if ( pChannel->handle != SOUND_INVALIDHANDLE
                && (SoundDriver_GetStatusAndCaps(pChannel->pDSoundBuffer) & SOUND_CHANNEL_PLAYING) == 0
                && (pChannel->flags & SOUND_CHANNEL_PAUSED) == 0
                && (pChannel->flags & SOUND_CHANNEL_FAR) == 0 )
            {
                SoundDriver_Release(pChannel->pDSoundBuffer);
                pChannel->pDSoundBuffer = NULL;
                pChannel->handle        = SOUND_INVALIDHANDLE;
                pChannel->hSnd          = SOUND_INVALIDHANDLE;
                pChannel->thingId       = 0;
            }
        }
    }
}

int J3DAPI Sound_GenerateLipSync(tSoundChannelHandle hChannel, uint8_t* pMouthPosX, uint8_t* pMouthPosY, int a4)
{
    tSoundChannel* pChannel;
    SoundInfo* pSndInfo;
    tAudioCompressorState compressor;
    size_t curPos;
    int a2;
    size_t endPos;
    int dataSize;
    uint8_t aBuffer[8192];
    size_t sizeBuffer;
    int bAllocated;
    int genDataSize;
    uint8_t* pSndData;
    size_t sndDataSize;

    pChannel = Sound_GetChannel(hChannel);
    if ( !pChannel )
    {
        goto error;
    }

    pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
    if ( !pSndInfo )
    {
        goto error;
    }

    if ( !pSndInfo->sampleRate )
    {
        goto error;
    }

    if ( pSndInfo->sempleBitSize != 8 && pSndInfo->sempleBitSize != 16 )
    {
        goto error;
    }

    if ( pSndInfo->numChannels < 1 || pSndInfo->numChannels > 2 )
    {
        goto error;
    }

    if ( !pSndInfo->pLipSyncData )
    {
        bAllocated = 0;
        if ( Sound_bNoLipSync )
        {
            goto error;
        }

        Sound_pHS->pDebugPrint("NOTE: Generating lip sync for file %s\n", &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->filePathOffset]);
        sizeBuffer = sizeof(aBuffer);

        if ( pSndInfo->bCompressed )
        {
            sndDataSize = ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize; // decompressed data size is first 4 bytes
            pSndData    = (uint8_t*)Sound_pHS->pMalloc(sndDataSize);
            bAllocated  = 1;

            AudioLib_ResetCompressor(&compressor);
            AudioLib_Uncompress(&compressor, pSndData, ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->compressedData, sndDataSize);
        }
        else
        {
            pSndData    = &soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset];
            sndDataSize = pSndInfo->dataSize;
        }

        genDataSize = AudioLib_GenerateLipSynchBlock(
            aBuffer,
            pSndData,
            60u,
            4,
            4,
            pSndInfo->sampleRate,
            pSndInfo->sempleBitSize,
            pSndInfo->numChannels,
            1,
            sndDataSize
        );

        if ( bAllocated )
        {
            Sound_pHS->pFree(pSndData);
        }

        pSndInfo->pLipSyncData = (uint8_t*)Sound_pHS->pMalloc(genDataSize);
        if ( pSndInfo->pLipSyncData )
        {
            memcpy(pSndInfo->pLipSyncData, aBuffer, genDataSize);
        }
    }

    curPos = SoundDriver_GetCurrentPosition(pChannel->pDSoundBuffer);
    endPos = pSndInfo->numChannels * 25 * (pSndInfo->sempleBitSize >> 3) * pSndInfo->sampleRate / 1000;

    if ( pSndInfo->bCompressed )
    {
        dataSize = ((tAudioCompressedData*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->dataOffset])->uncompressedSize; // decompressed data size is first 4 bytes
    }
    else
    {
        dataSize = pSndInfo->dataSize;
    }

    if ( (int)(curPos + endPos) >= dataSize )
    {
        a2 = -1;
    }
    else
    {
        a2 = 1000 * curPos / (pSndInfo->numChannels * (pSndInfo->sempleBitSize >> 3) * pSndInfo->sampleRate) + a4;
    }

    if ( !AudioLib_GetMouthPosition(pSndInfo->pLipSyncData, a2, pMouthPosX, pMouthPosY) )
    {
        return 1; // success
    }

error:
    if ( pMouthPosX )
    {
        *pMouthPosX = 0;
    }

    if ( pMouthPosY )
    {
        *pMouthPosY = 0;
    }

    return 0;
}

/* One playing channel as GetAllInstanceInfo reports it (0x10 bytes, the debug build's copy stride). */
typedef struct sSoundInstanceInfo
{
    tSoundChannelHandle handle;
    const char* pName;
    int thingId;
    tSoundChannelFlag flags;
} SoundInstanceInfo;

#ifdef J3D_DEBUG
/* Debug build only: the debug build has it at 0x0052FBE3 (between GetDevice3DCaps' neighbours and SetReverseSound);
   its only caller is the debug-only SoundDump; retail has no copy. Name from its own diagnostics. */
size_t J3DAPI Sound_GetAllInstanceInfo(SoundInstanceInfo* pInfo, size_t maxInfos)
{
    size_t numInfos = 0;
    size_t numChannels = Sound_numChannels;
    tSoundChannel* pCurChannel = &Sound_apChannels[numChannels];

    while ( (pCurChannel--, numChannels-- != 0) )
    {
        if ( pCurChannel->handle != SOUND_INVALIDHANDLE )
        {
            SoundInfo* pSndInfo = Sound_GetSoundInfo(pCurChannel->hSnd);
            if ( numInfos >= maxInfos )
            {
                Sound_pHS->pErrorPrint("Sound_GetAllInstanceInfo: Return value not big enough!\n");
                return maxInfos;
            }

            if ( pSndInfo )
            {
                pInfo->pName = (const char*)&soundbank_apSoundCache[pSndInfo->bankNum][pSndInfo->nameOffset];
            }
            else
            {
                Sound_pHS->pErrorPrint("Sound_GetAllInstanceInfo: Unknown sound data for playing sound.\n");
                pInfo->pName = ">ERR:Unknown<";
            }

            pInfo->handle  = pCurChannel->handle;
            pInfo->thingId = pCurChannel->thingId;
            pInfo->flags   = pCurChannel->flags;
            ++pInfo;
            ++numInfos;
        }
    }

    return numInfos;
}
#endif

void J3DAPI Sound_SetReverseSound(int bReverse)
{
    Sound_bReverseSound = bReverse;
}

#ifdef J3D_DEBUG
/* Debug build only: the debug build has it at 0x0052FCEB, after SetReverseSound; its only caller is JonesConsole's
   "sounds" command, which only the debug build registers; retail has no copy. Name from its own diagnostic. */
void J3DAPI Sound_SoundDump(void)
{
    SoundInstanceInfo aInfos[64];
    char aLine[256];
    char aPlaying[32];
    char aPaused[32];
    char aVolFade[32];
    size_t numInfos;
    char a3D[32];
    size_t i;
    char aPitchFade[32];
    char aLoop[32];
    char aFar[32];
    char aThing[32];

    numInfos = Sound_GetAllInstanceInfo(aInfos, STD_ARRAYLEN(aInfos));
    sprintf(aLine, "Sound Dump of %d sounds\n", numInfos, 0, 0, 0);
    Sound_pHS->pStatusPrint(aLine);
    for ( i = 0; i < numInfos; ++i )
    {
        if ( (aInfos[i].flags & SOUND_CHANNEL_PLAYING) != 0 )
        {
            sprintf(aPlaying, "Playing,");
        }
        else
        {
            sprintf(aPlaying, "NotPlay,");
        }

        if ( (aInfos[i].flags & SOUND_CHANNEL_PAUSED) != 0 )
        {
            sprintf(aPaused, "Paused,");
        }
        else
        {
            sprintf(aPaused, "Not Paused,");
        }

        aPaused[0] = 0;
        if ( (aInfos[i].flags & SOUND_CHANNEL_FAR) != 0 )
        {
            sprintf(aFar, "Far,");
        }
        else
        {
            sprintf(aFar, "   ,");
        }

        if ( (aInfos[i].flags & SOUND_CHANNEL_LOOP) != 0 )
        {
            sprintf(aLoop, "Looping,");
        }
        else
        {
            sprintf(aLoop, "       ,");
        }

        if ( (aInfos[i].flags & SOUND_CHANNEL_3DSOUND) != 0 )
        {
            sprintf(a3D, "3D,");
        }
        else
        {
            sprintf(a3D, "2D,");
        }

        if ( (aInfos[i].flags & (SOUND_CHANNEL_3DSOUND | SOUND_CHANNEL_THING)) != 0 )
        {
            sprintf(aThing, "Thing,");
        }
        else
        {
            sprintf(aThing, "     ,");
        }

        if ( (aInfos[i].flags & SOUND_CHANNEL_VOLUMEFADE) != 0 )
        {
            sprintf(aVolFade, "volFade,");
        }
        else
        {
            sprintf(aVolFade, "       ,");
        }

        aVolFade[0] = 0;
        if ( (aInfos[i].flags & SOUND_CHANNEL_PITCHFADE) != 0 )
        {
            sprintf(aPitchFade, "pitchFade,");
        }
        else
        {
            sprintf(aPitchFade, "         ,");
        }

        aPitchFade[0] = 0;
        sprintf(aLine, "%s %d\n \t%s %s %s %s %s %s %s %s", aInfos[i].pName, aInfos[i].handle, aPlaying, aFar, aLoop, a3D, aThing, aVolFade, aPitchFade, aPaused);
        Sound_pHS->pStatusPrint("%s\n", aLine);
    }

    Sound_pHS->pStatusPrint("\n");
}
#endif

tSoundHandle J3DAPI Sound_GenerateSoundHandle(void)
{
    int hNext = Sound_handleEntropy;
    if ( (hNext & 1) != 0 )
    {
        hNext = (hNext + 1) % SOUND_HANDLE_ENTROPY_MAX;
    }

    Sound_handleEntropy = (hNext + 1) % SOUND_HANDLE_ENTROPY_MAX;
    return hNext + SOUND_HANDLE_MIN;
}

tSoundChannelHandle J3DAPI Sound_GenerateChannelHandle(void)
{
    int hNext = Sound_handleEntropy;
    if ( (hNext & 1) == 0 )
    {
        hNext = (hNext + 1) % SOUND_HANDLE_ENTROPY_MAX;
    }

    Sound_handleEntropy = (hNext + 1) % SOUND_HANDLE_ENTROPY_MAX;
    return hNext + SOUND_HANDLE_MIN;
}

size_t J3DAPI Sound_GetFreeCache(size_t bankNum, size_t requiredSize)
{
    uint8_t* pNewCache;
    size_t currentprevCacheSz;
    size_t cacheLen;

    currentprevCacheSz = soundbank_aUsedCacheSizes[bankNum];
    if ( soundbank_aUsedCacheSizes[bankNum] == 0 ) // increment size to avoid 0 size for cache alignment
    {
        ++soundbank_aUsedCacheSizes[bankNum];
    }

    while ( (soundbank_aUsedCacheSizes[bankNum] & 3) != 0 )// // align cache to 8 bytes
    {
        ++soundbank_aUsedCacheSizes[bankNum];
    }

    cacheLen = soundbank_aUsedCacheSizes[bankNum];
    soundbank_aUsedCacheSizes[bankNum] += requiredSize;
    if ( soundbank_aUsedCacheSizes[bankNum] >= soundbank_aCacheSizes[bankNum] )
    {
        while ( soundbank_aUsedCacheSizes[bankNum] >= soundbank_aCacheSizes[bankNum] )
        {
            soundbank_aCacheSizes[bankNum] += Sound_cacheBlockSize;
        }

        pNewCache = (uint8_t*)Sound_pHS->pRealloc(soundbank_apSoundCache[bankNum], soundbank_aCacheSizes[bankNum]);
        if ( pNewCache )
        {
            soundbank_apSoundCache[bankNum] = pNewCache;
        }
        else
        {
            soundbank_aUsedCacheSizes[bankNum] = currentprevCacheSz;
            soundbank_aCacheSizes[bankNum]     = currentprevCacheSz;
            Sound_pHS->pErrorPrint("Sound_Load: Sound cache out of memory! (cacheSize:%d or 0x%08X)\n", soundbank_aCacheSizes[bankNum], soundbank_aCacheSizes[bankNum]);
            return 0;
        }
    }

    return cacheLen;
}

void J3DAPI Sound_IncreaseFreeCache(size_t bankNum, size_t size)
{
    if ( size > soundbank_aUsedCacheSizes[bankNum] )
    {
        soundbank_aUsedCacheSizes[bankNum] = 0;
    }
    else
    {
        soundbank_aUsedCacheSizes[bankNum] -= size;
    }
}

size_t J3DAPI Sound_WriteSoundFilepathToBank(size_t bankNum, const char* pFilepath, size_t len)
{
    size_t writeOffset = Sound_GetFreeCache(bankNum, len);
    if ( writeOffset )
    {
        memcpy(&soundbank_apSoundCache[bankNum][writeOffset], pFilepath, len);
    }

    return writeOffset;
}

uint32_t J3DAPI Sound_GetDeltaTime(void)
{
    return SoundDriver_GetTimeMsec() - Sound_msecElapsed;
}

SoundInfo* J3DAPI Sound_GetSoundInfo(tSoundHandle hSnd)
{
    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return NULL;
    }

    {
        int bankNum;
        for ( bankNum = 0; bankNum < SOUNDBANK_NUMBANKS; ++bankNum )
        {
            int i = soundbank_aNumSounds[bankNum];
            SoundInfo* pCurSndInfo = &soundbank_apSoundInfos[bankNum][i];

            while ( (pCurSndInfo--, i-- != 0) )
            {
                if ( pCurSndInfo->hSnd == hSnd )
                {
                    return pCurSndInfo;
                }
            }
        }
    }

    return NULL;
}

tSoundChannel* J3DAPI Sound_GetChannel(tSoundChannelHandle hChannel)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( !SOUND_IS_CHANNELHANDLE(hChannel) )
    {
        return NULL;
    }

    while ( pChannel--, i-- != 0 )
    {
        if ( pChannel->handle == hChannel )
        {
            return pChannel;
        }
    }

    return NULL;
}

tSoundChannel* J3DAPI Sound_GetChannelBySoundHandle(tSoundHandle hSnd)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    if ( !SOUND_IS_SOUNDINFOHANDLE(hSnd) )
    {
        return NULL;
    }

    while ( pChannel--, i-- != 0 )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE && pChannel->hSnd == hSnd )
        {
            return pChannel;
        }
    }

    return NULL;
}

uint8_t* J3DAPI Sound_GetSoundBufferData(LPDIRECTSOUNDBUFFER pDSBuf, size_t* pDataSize, uint32_t* pbCompressed)
{
    size_t i = Sound_numChannels;
    tSoundChannel* pChannel = &Sound_apChannels[i];

    while ( pChannel--, i-- != 0 )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE && pChannel->pDSoundBuffer == pDSBuf )
        {
            SoundInfo* pSoundInfo = Sound_GetSoundInfo(pChannel->hSnd);
            if ( pSoundInfo )
            {
                if ( pDataSize )
                {
                    *pDataSize = pSoundInfo->dataSize;
                }

                if ( pbCompressed )
                {
                    *pbCompressed = pSoundInfo->bCompressed;
                }

                return &soundbank_apSoundCache[pSoundInfo->bankNum][pSoundInfo->dataOffset];
            }
        }
    }

    return NULL;
}

int J3DAPI Sound_MemFileSeek(tFileHandle fh, int offset, int origin)
{
    return 0;
}

size_t J3DAPI Sound_MemFileRead(tFileHandle fh, void* pOutData, size_t nRead)
{

    if ( !Sound_pMemfileBuf || Sound_memfilePos >= Sound_memfileSize )
    {
        return 0;
    }

    if ( Sound_memfilePos + nRead > Sound_memfileSize )
    {
        memcpy(pOutData, &Sound_pMemfileBuf[Sound_memfilePos], Sound_memfileSize - Sound_memfilePos);
        Sound_memfilePos = Sound_memfileSize;
        return Sound_memfileSize - Sound_memfilePos;
    }
    else
    {
        memcpy(pOutData, &Sound_pMemfileBuf[Sound_memfilePos], nRead);
        Sound_memfilePos += nRead;
        return nRead;
    }
}

size_t J3DAPI Sound_MemFileWrite(tFileHandle fh, const void* pData, size_t size)
{

    if ( Sound_memfilePos + size > Sound_memfileSize )
    {
        uint8_t* pBufData;

        Sound_memfileSize += Sound_cacheBlockSize;
        pBufData = (uint8_t*)Sound_pHS->pRealloc(Sound_pMemfileBuf, Sound_memfileSize);
        if ( !pBufData )
        {
            Sound_pHS->pErrorPrint("Sound.MemFileWrite: Couldn't realloc %d.\n", Sound_memfileSize);
            Sound_MemFileFree();
            Sound_MemFileReset();
            return 0;
        }

        Sound_pMemfileBuf = pBufData;
    }

    memcpy(&Sound_pMemfileBuf[Sound_memfilePos], pData, size);
    Sound_memfilePos += size;
    return size;
}

tFileHandle J3DAPI Sound_MemFileReset(void)
{
    Sound_memfilePos = 0;
    return 1;
}

void J3DAPI Sound_MemFileFree(void)
{
    if ( Sound_pMemfileBuf )
    {
        Sound_pHS->pFree(Sound_pMemfileBuf);
        Sound_pMemfileBuf = NULL;
    }

    Sound_memfileSize = 0;
    Sound_memfilePos  = 0;
}

void J3DAPI Sound_StopAllNonStaticSounds(void)
{
    tSoundChannel* pChannel = Sound_apChannels;
    int i;

    if ( !pChannel )
    {
        return;
    }

    for ( i = 0; i < Sound_numChannels; ++i, ++pChannel )
    {
        if ( pChannel->handle != SOUND_INVALIDHANDLE )
        {
            SoundInfo* pSndInfo = Sound_GetSoundInfo(pChannel->hSnd);
            if ( !pSndInfo || pSndInfo->bankNum == SOUNDBANK_STATIC_NUM ) // if static
            {
                continue;
            }

            SoundDriver_Release(pChannel->pDSoundBuffer);
            pChannel->pDSoundBuffer = NULL;
            pChannel->handle        = SOUND_INVALIDHANDLE;
            pChannel->hSnd          = SOUND_INVALIDHANDLE;
            pChannel->thingId       = 0;
        }
    }
}

int J3DAPI Sound_ExportBank(tFileHandle fh, size_t bankNum)
{
    // Write num sound infos
    if ( Sound_pHS->pFileWrite(fh, &soundbank_aNumSounds[bankNum], sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    // Write sound data size
    if ( Sound_pHS->pFileWrite(fh, &soundbank_aUsedCacheSizes[bankNum], sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    // Write sound infos
    if ( Sound_pHS->pFileWrite(fh, soundbank_apSoundInfos[bankNum], sizeof(SoundInfo) * soundbank_aNumSounds[bankNum]) != sizeof(SoundInfo) * soundbank_aNumSounds[bankNum] )
    {
        return 0;
    }

    // Write sound data
    if ( Sound_pHS->pFileWrite(fh, soundbank_apSoundCache[bankNum], soundbank_aUsedCacheSizes[bankNum]) != soundbank_aUsedCacheSizes[bankNum] )
    {
        return 0;
    }

    // Write next handle
    if ( Sound_pHS->pFileWrite(fh, &Sound_handleEntropy, sizeof(uint32_t)) != sizeof(uint32_t) )
    {
        return 0;
    }

    return 1;
}

int J3DAPI Sound_ImportBank(tFileHandle fh, size_t bankNum)
{
    Sound_Reset(bankNum == SOUNDBANK_STATIC_NUM);

    // Read num sound infos
    Sound_ReadFile(fh, &soundbank_aNumSounds[bankNum], sizeof(uint32_t));

    // Allocate sound info cache
    if ( soundbank_aSizeSounds[bankNum] < soundbank_aNumSounds[bankNum] )
    {
        // Realloc sound info cache to import size
        soundbank_aSizeSounds[bankNum] = soundbank_aNumSounds[bankNum];
        if ( soundbank_apSoundInfos[bankNum] )
        {
            Sound_pHS->pFree(soundbank_apSoundInfos[bankNum]);
        }

        soundbank_apSoundInfos[bankNum] = (SoundInfo*)Sound_pHS->pMalloc(sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
        if ( !soundbank_apSoundInfos[bankNum] )
        {
            Sound_pHS->pErrorPrint("Sound:ImportBank: Failed to allocate %d bytes.\n", sizeof(SoundInfo) * soundbank_aSizeSounds[bankNum]);
            return 0;
        }
    }

    // Read sound data size
    Sound_ReadFile(fh, &soundbank_aUsedCacheSizes[bankNum], sizeof(uint32_t));

    // Allocate sound data cache
    if ( soundbank_aCacheSizes[bankNum] < soundbank_aUsedCacheSizes[bankNum] )
    {
        soundbank_aCacheSizes[bankNum] = soundbank_aUsedCacheSizes[bankNum];
        if ( soundbank_apSoundCache[bankNum] )
        {
            Sound_pHS->pFree(soundbank_apSoundCache[bankNum]);
        }

        soundbank_apSoundCache[bankNum] = (uint8_t*)Sound_pHS->pMalloc(soundbank_aCacheSizes[bankNum]);
        if ( !soundbank_apSoundCache[bankNum] )
        {
            Sound_pHS->pErrorPrint("Sound:ImportBank: Failed to allocate %d bytes.\n", soundbank_aCacheSizes[bankNum]);
            return 0;
        }
    }

    // Read sound infos, sound data and next handle
    Sound_ReadFile(fh, soundbank_apSoundInfos[bankNum], sizeof(SoundInfo) * soundbank_aNumSounds[bankNum]);
    Sound_ReadFile(fh, soundbank_apSoundCache[bankNum], soundbank_aUsedCacheSizes[bankNum]);
    Sound_ReadFile(fh, &Sound_handleEntropy, sizeof(uint32_t));

    soundbank_lastImportedBankNum = bankNum;
    return 1;
}

int J3DAPI Sound_ReadFile(tFileHandle fh, void* pData, size_t size)
{
    uint8_t* pCurData = (uint8_t*)pData;

    while ( size )
    {
        size_t nRemaining = size < 0x40000 ? size : 0x40000;

        if ( Sound_pHS->pFileRead(fh, pCurData, nRemaining) != nRemaining )
        {
            return 0;
        }

        SoundDriver_Sleep(20u);

        pCurData += nRemaining;
        size -= nRemaining;
    }

    return 1;
}