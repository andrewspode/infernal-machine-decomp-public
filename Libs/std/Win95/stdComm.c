#include "stdComm.h"

#include <std/General/std.h>
#include <std/General/stdMemory.h>
#include <std/General/stdUtil.h>
#include <std/Win95/stdWin95.h>
#include <stdio.h>
#define STDCOMM_MAX_CONNECTIONS 16U

// A DirectPlay service-provider connection (0x118 bytes, the stride the debug build's GetConnection copies);
// the members are inferred from what DirectPlay's EnumConnections callback reports.
typedef struct sStdCommConnection
{
    wchar_t aName[128];
    GUID guidSP;
    void* pConnection;
    size_t connectionSize;
} StdCommConnection;

/* Unreferenced original storage: retail .rdata 0x00509790-0x005097A0 is an 8-aligned 16-byte GUID
   {BD92BE00-1B1C-11D1-99C9-00A02476AD4B} (a version-1 GUID generated 1997-08-22) directly before
   aDPStatusTbl, and nothing in the image references it. Defined here, with the TU's other file-scope
   data, VC5 emits it exactly there; in stdStrTable (the other neighbour) a file-scope definition lands
   before that TU's 1.5 pool entry, not after it as retail has. Presumably an earlier DirectPlay
   application GUID (the discarded CreateGame's guidApplication comes from stdWin95_GetGuid, i.e.
   wkernel_guid). Name, linkage and purpose are inferred. */
static const GUID stdComm_unused_509790 = { 0xBD92BE00, 0x1B1C, 0x11D1, { 0x99, 0xC9, 0x00, 0xA0, 0x24, 0x76, 0xAD, 0x4B } };

static StdCommPlayerInfo stdComm_aPlayers[STDCOMM_MAX_PLAYERS] = { 0 };
static size_t stdComm_numPlayers = 0;
static LPDIRECTPLAY4 pDirectPlay = NULL;
static int bGameHost = 0;
/* The connection list (16 entries; read only by the linker-discarded GetConnection). */
static StdCommConnection stdComm_aConnections[STDCOMM_MAX_CONNECTIONS] = { 0 };
static StdCommGame stdComm_aGames[STDCOMM_MAX_GAMES] = { 0 };
static size_t stdComm_numConnections = 0;
static int bGameActive = 0;
static size_t stdComm_numGames = 0;
#ifdef J3D_DEBUG
/* Debug-only state (debug .bss 0x0066FCE4 and the 128 bytes at 0x0066FCE8, after numGames): retail's stdComm block
   has no storage for either, and only the two debug-only getters below read them. */
static int stdComm_bLobbied = 0;
static uint8_t stdComm_aLobbyData[128] = { 0 };
#endif

static const DXStatus aDPStatusTbl[60] =
{
    { DP_OK,                         "DP_OK"                        },
    { DPERR_ALREADYINITIALIZED,      "DPERR_ALREADYINITIALIZED"     },
    { DPERR_ACCESSDENIED,            "DPERR_ACCESSDENIED"           },
    { DPERR_ACTIVEPLAYERS,           "DPERR_ACTIVEPLAYERS"          },
    { DPERR_BUFFERTOOSMALL,          "DPERR_BUFFERTOOSMALL"         },
    { DPERR_CANTADDPLAYER,           "DPERR_CANTADDPLAYER"          },
    { DPERR_CANTCREATEGROUP,         "DPERR_CANTCREATEGROUP"        },
    { DPERR_CANTCREATEPLAYER,        "DPERR_CANTCREATEPLAYER"       },
    { DPERR_CANTCREATESESSION,       "DPERR_CANTCREATESESSION"      },
    { DPERR_CAPSNOTAVAILABLEYET,     "DPERR_CAPSNOTAVAILABLEYET"    },
    { DPERR_EXCEPTION,               "DPERR_EXCEPTION"              },
    { DPERR_GENERIC,                 "DPERR_GENERIC"                },
    { DPERR_INVALIDFLAGS,            "DPERR_INVALIDFLAGS"           },
    { DPERR_INVALIDOBJECT,           "DPERR_INVALIDOBJECT"          },
    { DPERR_INVALIDPARAM,            "DPERR_INVALIDPARAM"           },
    { DPERR_INVALIDPARAMS,           "DPERR_INVALIDPARAMS"          },
    { DPERR_INVALIDPLAYER,           "DPERR_INVALIDPLAYER"          },
    { DPERR_INVALIDGROUP,            "DPERR_INVALIDGROUP"           },
    { DPERR_NOCAPS,                  "DPERR_NOCAPS"                 },
    { DPERR_NOCONNECTION,            "DPERR_NOCONNECTION"           },
    { DPERR_NOMEMORY,                "DPERR_NOMEMORY"               },
    { DPERR_OUTOFMEMORY,             "DPERR_OUTOFMEMORY"            },
    { DPERR_NOMESSAGES,              "DPERR_NOMESSAGES"             },
    { DPERR_NONAMESERVERFOUND,       "DPERR_NONAMESERVERFOUND"      },
    { DPERR_NOPLAYERS,               "DPERR_NOPLAYERS"              },
    { DPERR_NOSESSIONS,              "DPERR_NOSESSIONS"             },
    { DPERR_PENDING,                 "DPERR_PENDING"                },
    { DPERR_SENDTOOBIG,              "DPERR_SENDTOOBIG"             },
    { DPERR_TIMEOUT,                 "DPERR_TIMEOUT"                },
    { DPERR_UNAVAILABLE,             "DPERR_UNAVAILABLE"            },
    { DPERR_UNSUPPORTED,             "DPERR_UNSUPPORTED"            },
    { DPERR_BUSY,                    "DPERR_BUSY"                   },
    { DPERR_USERCANCEL,              "DPERR_USERCANCEL"             },
    { DPERR_NOINTERFACE,             "DPERR_NOINTERFACE"            },
    { DPERR_CANNOTCREATESERVER,      "DPERR_CANNOTCREATESERVER"     },
    { DPERR_PLAYERLOST,              "DPERR_PLAYERLOST"             },
    { DPERR_SESSIONLOST,             "DPERR_SESSIONLOST"            },
    { DPERR_UNINITIALIZED,           "DPERR_UNINITIALIZED"          },
    { DPERR_NONEWPLAYERS,            "DPERR_NONEWPLAYERS"           },
    { DPERR_INVALIDPASSWORD,         "DPERR_INVALIDPASSWORD"        },
    { DPERR_CONNECTING,              "DPERR_CONNECTING"             },
    { DPERR_BUFFERTOOLARGE,          "DPERR_BUFFERTOOLARGE"         },
    { DPERR_CANTCREATEPROCESS,       "DPERR_CANTCREATEPROCESS"      },
    { DPERR_APPNOTSTARTED,           "DPERR_APPNOTSTARTED"          },
    { DPERR_INVALIDINTERFACE,        "DPERR_INVALIDINTERFACE"       },
    { DPERR_NOSERVICEPROVIDER,       "DPERR_NOSERVICEPROVIDER"      },
    { DPERR_UNKNOWNAPPLICATION,      "DPERR_UNKNOWNAPPLICATION"     },
    { DPERR_NOTLOBBIED,              "DPERR_NOTLOBBIED"             },
    { DPERR_SERVICEPROVIDERLOADED,   "DPERR_SERVICEPROVIDERLOADED"  },
    { DPERR_ALREADYREGISTERED,       "DPERR_ALREADYREGISTERED"      },
    { DPERR_NOTREGISTERED,           "DPERR_NOTREGISTERED"          },
    { DPERR_AUTHENTICATIONFAILED,    "DPERR_AUTHENTICATIONFAILED"   },
    { DPERR_CANTLOADSSPI,            "DPERR_CANTLOADSSPI"           },
    { DPERR_ENCRYPTIONFAILED,        "DPERR_ENCRYPTIONFAILED"       },
    { DPERR_SIGNFAILED,              "DPERR_SIGNFAILED"             },
    { DPERR_CANTLOADSECURITYPACKAGE, "DPERR_CANTLOADSECURITYPACKAGE"},
    { DPERR_ENCRYPTIONNOTSUPPORTED,  "DPERR_ENCRYPTIONNOTSUPPORTED" },
    { DPERR_CANTLOADCAPI,            "DPERR_CANTLOADCAPI"           },
    { DPERR_NOTLOGGEDIN,             "DPERR_NOTLOGGEDIN"            },
    { DPERR_LOGONDENIED,             "DPERR_LOGONDENIED"            }
};

static int J3DAPI stdComm_ProcessSystemMessage(LPDPMSG_GENERIC pMessage, DPID* pSender);
static const char* J3DAPI stdComm_DPGetStatus(HRESULT code);
static void J3DAPI stdComm_SessionToSettings(LPDPSESSIONDESC2 pSession, StdCommGame* pSettings);
static BOOL PASCAL stdComm_EnumPlayersCallback(DPID playerId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext);
static BOOL PASCAL stdComm_EnumSessionsCallback(LPCDPSESSIONDESC2 pSession, LPDWORD pTimeOut, DWORD dwFlags, LPVOID lpContext);

/* Linker-discarded: the debug build's head of the module (0x00501040..0x005010FE, before Send), every body read
   from its unoptimised code; no caller in either image. Names follow Jedi Knight's DirectPlay module order (EarlyInit,
   Startup, Destroy, InitConnection, CloseConnection) and are inferred. The two getters after the first read storage
   retail's .bss does not have (a flag and a 128-byte object): debug-only. */
int J3DAPI stdComm_EarlyInit(void)
{
    return 1;
}

#ifdef J3D_DEBUG
int J3DAPI stdComm_IsLobbied(void)
{
    return stdComm_bLobbied;
}

uint8_t* J3DAPI stdComm_GetLobbyData(void)
{
    return stdComm_aLobbyData;
}
#endif

int J3DAPI stdComm_Startup(void)
{
    return 0;
}

void J3DAPI stdComm_Shutdown(void)
{
}

int J3DAPI stdComm_InitConnection(int connNum)
{
    return 1;
}

void J3DAPI stdComm_CloseConnection(void)
{
}

size_t J3DAPI stdComm_GetNumConnections(void)
{
    return stdComm_numConnections;
}

int J3DAPI stdComm_GetConnection(size_t connNum, StdCommConnection* pConnection)
{
    if ( connNum > stdComm_numConnections ) {
        return 1;
    }

    memcpy(pConnection, &stdComm_aConnections[connNum], sizeof(StdCommConnection));
}

size_t J3DAPI stdComm_GetNumGames(void)
{
    return stdComm_numGames;
}

int J3DAPI stdComm_GetGame(size_t gameNum, StdCommGame* pGame)
{
    if ( gameNum > stdComm_numGames ) {
        return 1;
    }

    memcpy(pGame, &stdComm_aGames[gameNum], sizeof(StdCommGame));
    return 0;
}

int32_t J3DAPI stdComm_Send(DPID idFrom, DPID idTo, const void* pData, uint32_t size, uint32_t flags)
{
    HRESULT hr;
    DWORD dpFlags;

#line 463
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive);

    if ( flags )
    {
        dpFlags = flags;
    }
    else
    {
        dpFlags = 0;
    }

    hr = IDirectPlayX_Send(pDirectPlay, idFrom, idTo, dpFlags, (LPVOID)pData, size);
    if ( hr < DP_OK )
    {
#line 472
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->send returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
    }

    return hr;
}

int32_t J3DAPI stdComm_Receive(DPID* pSender, void* pData, size_t* pLength)
{
    HRESULT hr;
    DPID idTo;
    int curresult;

#line 506
    STD_ASSERTREL(pDirectPlay && bGameActive);
    STD_ASSERTREL(pData && pLength && pSender);

    for ( ;; )
    {
        hr = IDirectPlayX_Receive(pDirectPlay, pSender, &idTo, DPRECEIVE_ALL, pData, pLength);
        if ( hr < DP_OK )
        {
            if ( hr == DPERR_NOMESSAGES )
            {
                return -1;
            }

#line 517
            stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->Receive from id %x returned %s.\n", idTo, stdComm_DPGetStatus(hr), 0, 0);
            return -2;
        }

        if ( *pSender == 0 )
        {
            curresult = stdComm_ProcessSystemMessage((LPDPMSG_GENERIC)pData, pSender);
            if ( curresult == -1 )
            {
                continue;
            }

            return curresult;
        }

        return 0;
    }
}

/* Linker-discarded in retail (no retail symbol, address or caller). Body: the debug build's function
   (0x005012D2, original lines 562..605), read from its unoptimised disassembly; the /Gf literal-order evidence
   shows it existed. */
HRESULT J3DAPI stdComm_CreateGame(const StdCommGame* pSettings)
{
    DPSESSIONDESC2 sessionDesc;
    wchar_t aSessionName[128];
    HRESULT hr;

#line 562
    STD_ASSERT(pDirectPlay);
#line 563
    STD_ASSERT(pSettings);

    memset(&sessionDesc, 0, sizeof(sessionDesc));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;
    sessionDesc.dwFlags |= DPSESSION_KEEPALIVE;
    sessionDesc.dwFlags |= DPSESSION_NODATAMESSAGES;
    sessionDesc.dwFlags |= DPSESSION_NOPRESERVEORDER;
    sessionDesc.dwFlags |= DPSESSION_OPTIMIZELATENCY;
    sessionDesc.dwFlags |= DPSESSION_MIGRATEHOST;
    sessionDesc.dwFlags |= DPSESSION_JOINDISABLED;
    sessionDesc.guidApplication = *stdWin95_GetGuid();
    sessionDesc.dwMaxPlayers = pSettings->maxPlayers;

    _snwprintf(aSessionName, STD_ARRAYLEN(aSessionName), L"%s:%S", pSettings->aSessionName, pSettings->aSomething);
    sessionDesc.lpszSessionName = aSessionName;
    if ( wcslen(pSettings->aPassword) > 0 )
    {
        sessionDesc.lpszPassword = (LPWSTR)pSettings->aPassword;
    }

    sessionDesc.dwUser1 = pSettings->opt1;
    sessionDesc.dwUser3 = pSettings->opt2;
    sessionDesc.dwUser4 = pSettings->opt3;

    hr = IDirectPlayX_Open(pDirectPlay, &sessionDesc, DPOPEN_CREATE);
    if ( hr < DP_OK )
    {
#line 605
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->Open returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
    }
    else
    {
        bGameHost   = 1;
        bGameActive = 1;
        return 0;
    }

    IDirectPlayX_Close(pDirectPlay);
    bGameHost   = 0;
    bGameActive = 0;
    return hr;
}

HRESULT J3DAPI stdComm_SetGameParams(StdCommGame* pSettings)
{
    DWORD size;
    LPDPSESSIONDESC2 pSesDesc;
    wchar_t aSessionName[128];
    HRESULT hr;

#line 629
    STD_ASSERT(pDirectPlay);
#line 630
    STD_ASSERT(bGameActive && bGameHost);

    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, NULL, &size);
    if ( hr != DPERR_BUFFERTOOSMALL )
    {
#line 636
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay::GetSessionDesc size query returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        return hr;
    }

#line 641
    pSesDesc = (LPDPSESSIONDESC2)STDMALLOC(size);
    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, pSesDesc, &size);
    if ( hr < DP_OK )
    {
#line 647
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetSessionDesc returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        return hr;
    }

    pSesDesc->dwMaxPlayers = pSettings->maxPlayers;
    _snwprintf(aSessionName, STD_ARRAYLEN(aSessionName), L"%s:%S", pSettings->aSessionName, pSettings->aSomething);
    pSesDesc->lpszSessionName = aSessionName;
    if ( wcslen(pSettings->aPassword) > 0 )
    {
        pSesDesc->lpszPassword = pSettings->aPassword;
    }

    pSesDesc->dwUser1 = pSettings->opt1;
    pSesDesc->dwUser3 = pSettings->opt2;
    pSesDesc->dwUser4 = pSettings->opt3;

    pSesDesc->dwFlags |= DPSESSION_DIRECTPLAYPROTOCOL;
    pSesDesc->dwFlags |= DPSESSION_KEEPALIVE;
    pSesDesc->dwFlags |= DPSESSION_NODATAMESSAGES;
    pSesDesc->dwFlags |= DPSESSION_NOPRESERVEORDER;
    pSesDesc->dwFlags |= DPSESSION_OPTIMIZELATENCY;
    pSesDesc->dwFlags |= DPSESSION_MIGRATEHOST;

    hr = IDirectPlayX_SetSessionDesc(pDirectPlay, pSesDesc, 0);
    if ( hr < DP_OK )
    {
#line 678
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "SetSessionDesc returned %s (flags=%x).\n", stdComm_DPGetStatus(hr), pSesDesc->dwFlags, 0, 0);
        return hr;
    }

    return DP_OK;
}

/* Linker-discarded; name and signature inferred. The debug build has this function
   (0x0050179F, original lines 733..798) between SetGameParams and JoinGame: it re-reads the session description,
   rewrites the text after the ':' of the session name with the `%S` argument (the `aSomething` field), sets
   dwMaxPlayers and toggles DPSESSION_JOINDISABLED, then SetSessionDesc. Returns 1 on any failure. Not in retail or
   in Jedi Knight's text; parameter names ours. */
int J3DAPI stdComm_UpdateGameInfo(const char* pSomething, DWORD maxPlayers, int bJoinEnabled)
{
    int bError = 0;
    DWORD size;
    wchar_t* pSomethingPart;
    LPDPSESSIONDESC2 pSesDesc;
    wchar_t aSessionName[128];
    DWORD dwFlags;
    HRESULT hr;

#line 733
    STD_ASSERT(pDirectPlay);
#line 734
    STD_ASSERT(bGameActive && bGameHost);

    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, NULL, &size);
    if ( hr != DPERR_BUFFERTOOSMALL )
    {
#line 740
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay::GetSessionDesc size query returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        bError = 1;
    }
    else
    {
#line 746
        pSesDesc = (LPDPSESSIONDESC2)STDMALLOC(size);
        if ( !pSesDesc )
        {
            bError = 1;
        }
        else
        {
            hr = IDirectPlayX_GetSessionDesc(pDirectPlay, pSesDesc, &size);
            if ( hr < DP_OK )
            {
#line 757
                stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetSessionDesc returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
                bError = 1;
            }
            else
            {
                dwFlags = pSesDesc->dwFlags;
                if ( bJoinEnabled )
                {
                    pSesDesc->dwFlags &= ~DPSESSION_JOINDISABLED;
                }
                else
                {
                    pSesDesc->dwFlags |= DPSESSION_JOINDISABLED;
                }

                wcsncpy(aSessionName, pSesDesc->lpszSessionName, STD_ARRAYLEN(aSessionName) - 1);
                aSessionName[STD_ARRAYLEN(aSessionName) - 1] = 0;
                pSomethingPart = wcschr(aSessionName, L':');
                if ( !pSomethingPart )
                {
                    bError = 1;
                }
                else
                {
                    ++pSomethingPart;
                    _snwprintf(pSomethingPart, 64, L"%S", pSomething);
                    pSesDesc->lpszSessionName = aSessionName;
                    pSesDesc->dwMaxPlayers = maxPlayers;
                    hr = IDirectPlayX_SetSessionDesc(pDirectPlay, pSesDesc, 0);
                    if ( hr < DP_OK )
                    {
#line 798
                        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "SetSessionDesc returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
                        bError = 1;
                    }
                }
            }
        }
    }

    stdMemory_Free(pSesDesc);
    return bError;
}

int J3DAPI stdComm_JoinGame(size_t gameNum, const wchar_t* pPassword)
{
    DPSESSIONDESC2 sessionDesc;
    HRESULT hr;

#line 832
    STD_ASSERT(pDirectPlay);
#line 833
    STD_ASSERT(!bGameActive);
#line 834
    STD_ASSERT(gameNum < stdComm_numGames);

    memset(&sessionDesc, 0, sizeof(sessionDesc));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidInstance = stdComm_aGames[gameNum].guid;
    if ( pPassword && wcslen(pPassword) )
    {
        sessionDesc.lpszPassword = (LPWSTR)pPassword;
    }

    hr = IDirectPlayX_Open(pDirectPlay, &sessionDesc, DPOPEN_JOIN);
    if ( hr < DP_OK )
    {
#line 851
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->Open returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        IDirectPlayX_Close(pDirectPlay);
        return hr;
    }

    bGameHost   = 0;
    bGameActive = 1;
    return 0;
}

static int J3DAPI stdComm_GetSessionSettings(StdCommGame* pSettings)
{
    LPDPSESSIONDESC2 pSessionDesc;
    HRESULT hr;
    DWORD size;

#line 692
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive);
    STD_ASSERTREL(pSettings);

    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, NULL, &size);
    if ( hr != DPERR_BUFFERTOOSMALL )
    {
#line 700
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay::GetSessionDesc size query returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        return hr;
    }

#line 705
    pSessionDesc = (LPDPSESSIONDESC2)STDMALLOC(size);
    hr = IDirectPlayX_GetSessionDesc(pDirectPlay, pSessionDesc, &size);
    if ( hr < DP_OK )
    {
#line 711
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetSessionDesc returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        return hr;
    }

    stdComm_SessionToSettings(pSessionDesc, pSettings);
    stdMemory_Free(pSessionDesc);
    return hr;
}

void J3DAPI stdComm_CloseGame(void)
{
#line 875
    STD_ASSERTREL(pDirectPlay != NULL);
    if ( bGameActive )
    {
        IDirectPlayX_Close(pDirectPlay);
    }

    bGameHost = bGameActive = 0;
}

int J3DAPI stdComm_RejoinSession(DPID* pPlayerId, const wchar_t* pPlayerName)
{
    StdCommGame settings;
    int result;
    DPSESSIONDESC2 sessiondesc;

#line 893
    STD_ASSERTREL(pDirectPlay != NULL);
    STD_ASSERTREL(bGameActive);

    result = stdComm_GetSessionSettings(&settings);
    if ( result < 0 )
    {
        return result;
    }

    result = IDirectPlayX_Close(pDirectPlay);
    if ( result < DP_OK )
    {
#line 904
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->Close returned %s.\n", stdComm_DPGetStatus(result), 0, 0, 0);
        return result;
    }

    memset(&sessiondesc, 0, sizeof(sessiondesc));
    sessiondesc.dwSize       = sizeof(DPSESSIONDESC2);
    sessiondesc.guidInstance = settings.guid;
    sessiondesc.lpszPassword = settings.aPassword;

    result = IDirectPlayX_Open(pDirectPlay, &sessiondesc, DPOPEN_JOIN);
    if ( result < DP_OK )
    {
#line 916
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->Open returned %s.\n", stdComm_DPGetStatus(result), 0, 0, 0);
        return result;
    }

    *pPlayerId = stdComm_CreatePlayer(pPlayerName);
    if ( *pPlayerId == 0 )
    {
#line 922
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Failed to create new player after rejoining the session.\n", 0, 0, 0, 0);
        return DPERR_GENERIC;
    }

    bGameActive = 1;
    bGameHost   = 0;
    return 0;
}

int J3DAPI stdComm_IsGameActive(void)
{
    return bGameActive;
}

int J3DAPI stdComm_IsGameHost(void)
{
    return bGameHost;
}

/* Linker-discarded; name and signature inferred. The debug build has this function
   (0x00501F1F, original line 953) between IsGameHost and UpdatePlayers: it fills a DPCAPS (dwSize 0x28) through
   IDirectPlay4::GetCaps and returns 1 when there is no interface, no buffer or the call fails. */
int J3DAPI stdComm_GetCaps(LPDPCAPS pCaps)
{
    HRESULT hr;

    if ( !pDirectPlay || !pCaps )
    {
        return 1;
    }

    pCaps->dwSize = sizeof(DPCAPS);
    hr = IDirectPlayX_GetCaps(pDirectPlay, pCaps, 0);
    if ( hr < DP_OK )
    {
#line 953
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "GetCaps returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        return 1;
    }

    return 0;
}

int J3DAPI stdComm_UpdatePlayers(size_t gameNum)
{
    HRESULT hr;

#line 977
    STD_ASSERTREL((pDirectPlay != NULL));

    stdComm_numPlayers = 0;
    memset(stdComm_aPlayers, 0, sizeof(stdComm_aPlayers));

    if ( bGameActive )
    {
        hr = IDirectPlayX_EnumPlayers(pDirectPlay, NULL, stdComm_EnumPlayersCallback, NULL, 0);
    }
    else if ( gameNum < stdComm_numGames )
    {
        hr = IDirectPlayX_EnumPlayers(pDirectPlay, &stdComm_aGames[gameNum].guid, stdComm_EnumPlayersCallback, NULL, DPENUMPLAYERS_SESSION);
    }
    else
    {
        return DPERR_GENERIC;
    }

    if ( hr < DP_OK )
    {
#line 993
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "UpdatePlayers returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
    }

    return hr;
}

/* Linker-discarded. Retail proves the object: stdComm_numGames bounds UpdatePlayers' stdComm_aGames[gameNum]
   (0x004C3064) but no retail instruction stores it. Body and parameters: the debug build's function
   (0x00502096, original lines 1027..1050): the flags are AVAILABLE | PASSWORDREQUIRED (plus ASYNC when bAsync), the
   password is the session password, the window is the enumeration context. Parameter names ours. */
int J3DAPI stdComm_EnumSessions(int bAsync, const wchar_t* pPassword)
{
    DWORD dwFlags;
    DPSESSIONDESC2 sessionDesc;
    HRESULT hr;

#line 1027
    STD_ASSERT(pDirectPlay != NULL);
    dwFlags = DPENUMSESSIONS_AVAILABLE | DPENUMSESSIONS_PASSWORDREQUIRED;
    if ( bAsync )
    {
        dwFlags |= DPENUMSESSIONS_ASYNC;
    }

    stdComm_numGames = 0;
    memset(stdComm_aGames, 0, sizeof(stdComm_aGames));
    memset(&sessionDesc, 0, sizeof(sessionDesc));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidApplication = *stdWin95_GetGuid();
    if ( pPassword && wcslen(pPassword) )
    {
        sessionDesc.lpszPassword = (LPWSTR)pPassword;
    }

    hr = IDirectPlayX_EnumSessions(pDirectPlay, &sessionDesc, 0, stdComm_EnumSessionsCallback, stdWin95_GetWindow(), dwFlags);
    if ( hr < DP_OK )
    {
#line 1050
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->EnumSessions returned %s\n", stdComm_DPGetStatus(hr), 0, 0, 0);
    }

    return hr;
}

/* Linker-discarded; the debug build's function (0x005021AD, original lines 1062..1077): the same enumeration with
   the flags ALL | STOPASYNC, i.e. it stops a running asynchronous enumeration. */
void J3DAPI stdComm_EnumSessions2(void)
{
    DWORD dwFlags;
    DPSESSIONDESC2 sessionDesc;
    HRESULT hr;

    dwFlags = DPENUMSESSIONS_ALL | DPENUMSESSIONS_STOPASYNC;
#line 1062
    STD_ASSERT(pDirectPlay != NULL);
    stdComm_numGames = 0;
    memset(stdComm_aGames, 0, sizeof(stdComm_aGames));
    memset(&sessionDesc, 0, sizeof(sessionDesc));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidApplication = *stdWin95_GetGuid();
    hr = IDirectPlayX_EnumSessions(pDirectPlay, &sessionDesc, 0, stdComm_EnumSessionsCallback, stdWin95_GetWindow(), dwFlags);
    if ( hr < DP_OK )
    {
#line 1077
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DirectPlay->EnumSessions returned %s\n", stdComm_DPGetStatus(hr), 0, 0, 0);
    }
}

DPID J3DAPI stdComm_CreatePlayer(const wchar_t* pPlayerName)
{
    DPNAME curdpname;
    DPID curid;
    HRESULT hr;

#line 1104
    STD_ASSERTREL(pDirectPlay);
    STD_ASSERTREL(bGameActive);

    curdpname.dwSize        = sizeof(DPNAME);
    curdpname.dwFlags       = 0;
    curdpname.lpszShortName = (LPWSTR)pPlayerName;
    curdpname.lpszLongName  = NULL;

    hr = IDirectPlayX_CreatePlayer(pDirectPlay, &curid, &curdpname, NULL, NULL, 0, 0);
    if ( hr < DP_OK )
    {
#line 1117
        stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "CreatePlayer returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
    }

    return curid;
}

void J3DAPI stdComm_DestroyPlayer(DPID playerId)
{
    HRESULT hr;

#line 1140
    STD_ASSERTREL(pDirectPlay != NULL);
    STD_ASSERTREL(playerId > 0);

    hr = IDirectPlayX_DestroyPlayer(pDirectPlay, playerId);
    if ( hr < DP_OK )
    {
#line 1146
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "DestroyPlayer returned %s.\n", stdComm_DPGetStatus(hr), 0, 0, 0);
        return;
    }

#line 1150
    stdPrintf(std_pHS->pStatusPrint, J3D_FILE, __LINE__, "Player %d destroyed.\n", playerId, 0, 0, 0);
}

size_t J3DAPI stdComm_GetNumPlayers(void)
{
    return stdComm_numPlayers;
}

int J3DAPI stdComm_VerifyPlayer(DPID id)
{
    size_t i;
    for ( i = 0; i < stdComm_numPlayers; ++i )
    {
        if ( id == stdComm_aPlayers[i].id )
        {
            return 0;
        }
    }

    return 1;
}

DPID J3DAPI stdComm_GetPlayerID(size_t playerNum)
{
    return stdComm_aPlayers[playerNum].id;
}

/* PLACEHOLDER, linker-discarded; name inferred. The unreferenced GUID
   stdComm_unused_509790 is byte-identical to the GUID Jedi Knight's DirectPlay_SendLobbyMessage
   (JK 0x004305E0, MotS identical) copies into its lobby message, the only user of that GUID in JK and
   MotS; JK's caller sithMulti_LobbyMessage is itself discarded in Indy. JK orders it after the player
   functions and before the enumeration callbacks; the __LINE__ gap DestroyPlayer -> EnumPlayersCallback
   (1150..1343) has +53 excess lines. Indy renamed JK's DirectPlay_* to stdComm_* (Send, Receive,
   CreatePlayer, ...). The Indy debug build's function at this position (0x00502487, after GetPlayerID; called by
   sithMulti_LobbyMessage) is empty, so the body is empty and returns nothing. */
void J3DAPI stdComm_SendLobbyMessage(void* pData, uint32_t size)
{
}

/* Linker-discarded with its only caller EnumSessions; name invented. The writer of the stdComm_aGames entries
   (StdCommGame records; stdComm_SessionToSettings is the only DPSESSIONDESC2 -> StdCommGame conversion). Body
   is the debug build's function (0x0050248C, original lines 1296..1307). */
static BOOL PASCAL stdComm_EnumSessionsCallback(LPCDPSESSIONDESC2 pSession, LPDWORD pTimeOut, DWORD dwFlags, LPVOID lpContext)
{
    size_t idx;

    if ( (dwFlags & DPESC_TIMEDOUT) != 0 )
    {
        return FALSE;
    }

#line 1296
    STD_ASSERT((pSession != NULL));
    if ( stdComm_numGames < STDCOMM_MAX_GAMES )
    {
        idx = stdComm_numGames++;
        stdComm_SessionToSettings((LPDPSESSIONDESC2)pSession, &stdComm_aGames[idx]);
    }
    else
    {
#line 1307
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Num of active games exceeds COMM_MAX_GAMES.  Some not listed.\n", 0, 0, 0, 0);
    }

    return TRUE;
}

static BOOL PASCAL stdComm_EnumPlayersCallback(DPID playerId, DWORD dwPlayerType, LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
{

    if ( dwPlayerType != DPPLAYERTYPE_PLAYER )
    {
        return TRUE;
    }

    if ( stdComm_numPlayers >= STDCOMM_MAX_PLAYERS )
    {
#line 1343
        stdPrintf(std_pHS->pErrorPrint, J3D_FILE, __LINE__, "Too many players.  Player %d not enumerated.\n", playerId, 0, 0, 0);
        return TRUE;
    }

    memset(&stdComm_aPlayers[stdComm_numPlayers], 0, sizeof(StdCommPlayerInfo));
    if ( lpName->lpszShortName )
    {
        wcsncpy(stdComm_aPlayers[stdComm_numPlayers].aName, lpName->lpszShortName, STD_ARRAYLEN(stdComm_aPlayers[stdComm_numPlayers].aName) - 1);
        stdComm_aPlayers[stdComm_numPlayers].aName[STD_ARRAYLEN(stdComm_aPlayers[stdComm_numPlayers].aName) - 1] = 0;
    }

    stdComm_aPlayers[stdComm_numPlayers++].id = playerId;
    return TRUE;
}

static int J3DAPI stdComm_ProcessSystemMessage(LPDPMSG_GENERIC pMessage, DPID* pSender)
{
    LPDPMSG_GENERIC pMsg;
    LPDPMSG_CREATEPLAYERORGROUP pCreate;
    LPDPMSG_DESTROYPLAYERORGROUP pDestroy;

#line 1385
    STD_ASSERTREL(bGameActive && pDirectPlay);
    STD_ASSERTREL(pMessage);

    pMsg = pMessage;
    switch ( pMsg->dwType )
    {
        case DPSYS_HOST: // the current host has left the session, we're now the host
            bGameHost = 1;
            return 8;

        case DPSYS_SESSIONLOST:
            return 1;

        case DPSYS_DESTROYPLAYERORGROUP:
            pDestroy = (LPDPMSG_DESTROYPLAYERORGROUP)pMessage;
            *pSender = pDestroy->dpId;
            return 2;

        case DPSYS_CREATEPLAYERORGROUP:
            pCreate = (LPDPMSG_CREATEPLAYERORGROUP)pMessage;
            *pSender = pCreate->dpId;
            if ( pCreate->dwPlayerType == DPPLAYERTYPE_PLAYER )
            {
                return 5;
            }
            return -1;

        default:
            return -1;
    }
}

static void J3DAPI stdComm_SessionToSettings(LPDPSESSIONDESC2 pSession, StdCommGame* pSettings)
{
    wchar_t aSessionName[128];
    wchar_t* pStr;

#line 1439
    STD_ASSERTREL(pSession && pSettings);

    wcsncpy(aSessionName, pSession->lpszSessionName, STD_ARRAYLEN(aSessionName) - 1);
    aSessionName[STD_ARRAYLEN(aSessionName) - 1] = 0;

    pStr = wcstok(aSessionName, L":\n");
    if ( pStr )
    {
        wcsncpy(pSettings->aSessionName, pStr, STD_ARRAYLEN(pSettings->aSessionName) - 1);
        pSettings->aSessionName[STD_ARRAYLEN(pSettings->aSessionName) - 1] = 0;
    }

    pStr = wcstok(NULL, L"\n");
    if ( pStr )
    {
        STD_TOSTR(pSettings->aSomething, pStr);
    }

    pSettings->guid              = pSession->guidInstance;
    pSettings->maxPlayers        = pSession->dwMaxPlayers;
    pSettings->numCurrentPlayers = pSession->dwCurrentPlayers;
    pSettings->opt1              = pSession->dwUser1;
    pSettings->opt2              = pSession->dwUser3;
    pSettings->opt3              = pSession->dwUser4;
}

static const char* J3DAPI stdComm_DPGetStatus(HRESULT code)
{
    const char* pText;
    int i;
    int numCodes;

    pText = "Unknown Error";
    numCodes = 60;
    for ( i = 0; i < numCodes; i++ )
    {
        if ( aDPStatusTbl[i].code == code )
        {
            pText = aDPStatusTbl[i].text;
            break;
        }
    }

    return pText;
}
