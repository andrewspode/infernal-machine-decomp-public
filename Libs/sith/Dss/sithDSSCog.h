#ifndef SITH_SITHDSSCOG_H
#define SITH_SITHDSSCOG_H
#include <j3dcore/j3d.h>

#include <sith/Devices/sithComm.h>
#include <sith/types.h>
#include <sith/Main/sithMain.h>


J3D_EXTERN_C_START

int J3DAPI sithDSSCog_SendMessage(const SithCog* pCog, SithCogMsgType msgType, SithCogSymbolRefType senderType, int senderIdx, SithCogSymbolRefType srcType, int srcIdx, int linkId, int param0, int param1, int param2, int param3, DPID idTo);
int J3DAPI sithDSSCog_ProcessMessage(const SithMessage* pMsg);

int J3DAPI sithDSSCog_SyncCogState(const SithCog* pCog, DPID idTO, SithMessageStream outstream);
int J3DAPI sithDSSCog_ProcessCogState(const SithMessage* pMsg);

J3D_EXTERN_C_END
#endif // SITH_SITHDSSCOG_H
