#ifndef FLY_KEY_H_
#define FLY_KEY_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

struct flykey_struct_info{
	BYTE iReserved;
};

extern void readFromMmapPrintf(BYTE *buf,UINT length);

extern void ipcEventExchangeProc(UINT32 sourceEvent);
extern void ipcEventProcProc(UINT32 sourceEvent);
extern void msgQueueReadProc(BYTE msgQueueID,BYTE *pData,UINT length);

extern void readFromhardwarePrintf(BYTE *buf,UINT length);
extern void readFromhardwareProc(BYTE *buf,UINT length);

extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);

#define DEBUG_MSG_ON 0

#if DEBUG_MSG_ON
#define DBG0(CODE) if(1){CODE}

#define DBG1(CODE) if(1){CODE}
#define DBG2(CODE) if(1){CODE}
#define DBG3(CODE) if(1){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel>0){CODE}
#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel>1){CODE}
#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel>2){CODE}
#endif

#endif