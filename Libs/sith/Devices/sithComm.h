#ifndef SITH_SITHCOMM_H
#define SITH_SITHCOMM_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>

J3D_EXTERN_C_START

typedef enum eSithMessageStream
{
    SITHMESSAGE_STREAM_NONE = 0x00,
    SITHMESSAGE_STREAM_NET  = 0x01,
    SITHMESSAGE_STREAM_FILE = 0x04,
    SITHMESSAGE_STREAM_ALL  = 0xFF,
} SithMessageStream;

extern DPID sithMessage_localPlayerId;

extern SithMessageStream sithMessage_outputstream;

extern SithMessageStream sithMessage_inputstream;

// NOTE: all sithMessage functions should be in sithComm module
void J3DAPI sithMessage_Startup(void);
void J3DAPI sithMessage_Shutdown(void);

int J3DAPI sithMessage_SendMessage(SithMessage* pMessage, DPID idTo, SithMessageStream outstream, uint32_t dwDPFlags);

void J3DAPI sithMessage_ProcessMessages(void);
int J3DAPI sithMessage_Process(const SithMessage* pMessage);
void J3DAPI sithMessage_StopProcessMessages(void);
void J3DAPI sithMessage_Flush(void);

DPID J3DAPI sithMessage_GetSenderID(void);

int J3DAPI sithMessage_StartGame(StdCommGame* pSettings); // Linker-discarded retail function (category b)
int J3DAPI sithMessage_JoinGame(size_t gameNum, const wchar_t* pPassword);
void J3DAPI sithMessage_CheckMembershipList(DPID idTo);
void J3DAPI sithMessage_CloseGame(void);

void J3DAPI sithMessage_RegisterFunction(int type, SithMessageProcessFunc pFunc);

J3D_EXTERN_C_END
#endif // SITH_SITHCOMM_H
