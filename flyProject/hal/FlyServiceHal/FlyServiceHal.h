#ifndef SERVICE_HAL_H_
#define SERVICE_HAL_H_

#include "../../include/global.h"

struct flyServiceHal_struct_info{
	
	BOOL bExBoxEnable;
	
	INT flySerialHal_fd;
	BOOL bKillCentralHalReadThread;
	
	BYTE exBoxInfoFrameStatus;
	BYTE bRevData[2];
		
	BYTE serialReadBuff[1024];
	UINT32 serialReadBuffLen;
};

extern void ipcEventExchangeProc(UINT32 sourceEvent);
extern void ipcEventProcProc(UINT32 sourceEvent);

extern void readFromhardwareProc(BYTE *buf,UINT length);

extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);

#define  DEBUG_MSG_ON 0

#if DEBUG_MSG_ON
#define DBG0(CODE) if(1){CODE}

#define DBG1(CODE) if(1){CODE}
#define DBG2(CODE) if(1){CODE}
#define DBG3(CODE) if(1){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgServiceLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgServiceLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgServiceLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}
#endif

#endif