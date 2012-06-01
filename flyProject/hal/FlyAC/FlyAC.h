#ifndef FLY_AC_H_
#define FLY_AC_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_AC

typedef struct flyac_struct_info{

	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;

	BOOL bKillFlyMainThread;
	pthread_mutex_t MainThreadMutex;
	pthread_cond_t  MainThreadCond; 
	BOOL bMainThreadRunAgain;
}FLY_AC_INFO,*P_FLY_AC_INFO;

extern void ipcEventProcProc(UINT32 sourceEvent);
extern void msgQueueReadProc(BYTE msgQueueID,BYTE *pData,UINT length);

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

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel>0){CODE}
#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel>1){CODE}
#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel>2){CODE}
#endif

#endif