#ifndef SITH_SITHDSS_H
#define SITH_SITHDSS_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <sith/Dss/sithMulti.h>
#include <sith/types.h>


J3D_EXTERN_C_START

//Helper macros for data serialization / de-serialization

#define SITHDSS_ENDOUT (sithMulti_message.length = pCurDSSMsgData_ - sithMulti_message.data)

#define SITHDSS_ENDIN

#define SITHDSS_PUSHINT8(x) (*(*(int8_t**)&pCurDSSMsgData_)++ = (x))
#define SITHDSS_PUSHUINT8(x) (*pCurDSSMsgData_++ = (x))
#define SITHDSS_PUSHINT16(x) (*(*(int16_t**)&pCurDSSMsgData_)++ = (x))
#define SITHDSS_PUSHUINT16(x) (*(*(uint16_t**)&pCurDSSMsgData_)++ = (x))
#define SITHDSS_PUSHINT32(x) (*(*(int32_t**)&pCurDSSMsgData_)++ = (x))
#define SITHDSS_PUSHUINT32(x) (*(*(uint32_t**)&pCurDSSMsgData_)++ = (x))
#define SITHDSS_PUSHFLOAT(x) (*(*(float**)&pCurDSSMsgData_)++ = (x))
#define SITHDSS_PUSHVEC2(x) sithDSS_WriteVec2(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHVEC3(x) (memcpy(pCurDSSMsgData_, (x), sizeof(rdVector3)), pCurDSSMsgData_ += sizeof(rdVector3))
#define SITHDSS_PUSHVEC4(x) (memcpy(pCurDSSMsgData_, (x), sizeof(rdVector4)), pCurDSSMsgData_ += sizeof(rdVector4))
#define SITHDSS_PUSHMAT34(x) sithDSS_WriteMat34(&pCurDSSMsgData_, (x))
#define SITHDSS_PUSHSTRING(x, len) sithDSS_WriteString(&pCurDSSMsgData_, (x), (len))
#define SITHDSS_PUSHWSTRING(x, len) sithDSS_WriteWString(&pCurDSSMsgData_, (x), (len))

#define SITHDSS_POPINT8() sithDSS_ReadInt8(&pCurDSSMsgData_)
#define SITHDSS_POPUINT8() (*pCurDSSMsgData_++)
#define SITHDSS_POPINT16() (*(*(int16_t**)&pCurDSSMsgData_)++)
#define SITHDSS_POPUINT16() (*(*(uint16_t**)&pCurDSSMsgData_)++)
#define SITHDSS_POPINT32() (*(*(int32_t**)&pCurDSSMsgData_)++)
#define SITHDSS_POPUINT32() (*(*(uint32_t**)&pCurDSSMsgData_)++)
#define SITHDSS_POPFLOAT() (*(*(float**)&pCurDSSMsgData_)++)
#define SITHDSS_POPVEC2(x) sithDSS_ReadVec2(&pCurDSSMsgData_, (x))
#define SITHDSS_POPVEC3(x) (memcpy((x), pCurDSSMsgData_, sizeof(rdVector3)), pCurDSSMsgData_ += sizeof(rdVector3))
#define SITHDSS_POPVEC4(x) (memcpy((x), pCurDSSMsgData_, sizeof(rdVector4)), pCurDSSMsgData_ += sizeof(rdVector4))
#define SITHDSS_POPMAT34(x) sithDSS_ReadMat34(&pCurDSSMsgData_, (x))
#define SITHDSS_POPSTRING(x, len)  sithDSS_ReadString(&pCurDSSMsgData_, (x), (len))
#define SITHDSS_POPWSTRING(x, len) sithDSS_ReadWString(&pCurDSSMsgData_, (x), (len))


enum eSithDSSType
{
    SITHDSS_POS                 = 1,
    SITHDSS_CHAT                = 2,
    SITHDSS_SECTORFLAGS         = 3,
    SITHDSS_FIRE                = 4,
    SITHDSS_DEATH               = 5,
    SITHDSS_DAMAGE              = 6,
    SITHDSS_SETMODEL            = 7,
    SITHDSS_COGMESSAGE          = 8,
    SITHDSS_PLAYKEY             = 9,
    SITHDSS_PLAYSOUND           = 10,
    SITHDSS_THINGSTATE          = 11,
    SITHDSS_THINGFULLDESC       = 12,
    SITHDSS_COGSTATE            = 13,
    SITHDSS_SURFACESTATUS       = 14,
    SITHDSS_AISTATUS            = 15,
    SITHDSS_INVENTORY           = 16,
    SITHDSS_ANIMSTATUS          = 17,
    SITHDSS_SECTORSTATUS        = 18,
    SITHDSS_PLAYKEYMODE         = 19,
    SITHDSS_PATHMOVE            = 20,
    SITHDSS_PUPPETSTATUS        = 21,
    SITHDSS_ATTACHMENT          = 22,
    SITHDSS_SYNCTASKEVENTS      = 23,
    SITHDSS_SYNCCAMERAS         = 24,
    SITHDSS_TAKE                = 25,
    SITHDSS_TAKE2               = 26,
    SITHDSS_STOPKEY             = 27,
    SITHDSS_STOPSOUND           = 28,
    SITHDSS_CREATETHING         = 29,
    SITHDSS_WHIPSTATUS          = 30,
    SITHDSS_SYNCGAMESTATE       = 31,
    SITHDSS_SYNCPLAYERS         = 32,
    SITHDSS_WELCOME             = 33,
    SITHDSS_JOIN                = 34,
    SITHDSS_DESTROYTHING        = 35,
    SITHDSS_JOINSTATUS          = 36,
    SITHDSS_PLAYSOUNDMODE       = 37,
    SITHDSS_PING                = 38,
    SITHDSS_PONG                = 39,
    SITHDSS_MOVEPOS             = 40,
    SITHDSS_CHECKMEMBERSHIPLIST = 41,
    SITHDSS_QUIT                = 42,
    SITHDSS_ENDSECTION          = 44,
    SITHDSS_VOICESTATE          = 45,
    SITHDSS_VEHICLECONTROLSTATE = 46,
    SITHDSS_HUDSTATE            = 47,
    SITHDSS_SYNCWPNT            = 48,
    SITHDSS_SYNCSEENSECTORS     = 49,
};

int J3DAPI sithDSS_SurfaceStatus(const SithSurface* pSurf, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSurfaceStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_SectorStatus(const SithSector* pSector, DPID a2, unsigned int outstream);
int J3DAPI sithDSS_SyncSeenSectors(DPID idTo, unsigned int outstream);

int J3DAPI sithDSS_ProcessSyncSeenSecors(const SithMessage* pMsg);
int J3DAPI sithDSS_ProcessSectorStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_SectorFlags(const SithSector* pSector, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSectorFlags(const SithMessage* pMsg);

int J3DAPI sithDSS_AIStatus(const SithAIControlBlock* pLocal, DPID idTo, signed int outstream);
int J3DAPI sithDSS_ProcessAIStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_Inventory(const SithThing* pThing, unsigned int inventoryId, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessInventory(const SithMessage* pMsg);

int J3DAPI sithDSS_AnimStatus(const SithAnimationSlot* pAnim, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessAnimStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_PuppetStatus(const SithThing* pThing, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessPuppetStatus(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncTaskEvents(const SithEvent* pEvent, DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSyncTaskEvents(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncCameras(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSyncCameras(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncGameState(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessSyncGameState(const SithMessage* pMsg);

int J3DAPI sithDSS_SyncVehicleControlState(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessVehicleControlsState(const SithMessage* pMsg);

int J3DAPI sithDSS_SendEndSection(DPID idTo, unsigned int outstream);
int J3DAPI sithDSS_ProcessEndSection(const SithMessage* pMsg);

J3D_EXTERN_C_END
#endif // SITH_SITHDSS_H
