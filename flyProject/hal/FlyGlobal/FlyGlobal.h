#ifndef FLY_GLOBAL_H_
#define FLY_GLOBAL_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
//#include "../../include/types_def.h"
//#include "../../include/ShareMemoryStruct.h"

#include "../../include/global.h"

#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_GLOBAL

#define GLOBAL_MEMORY_EVENT_COUNT	1	//不能为0
#define GLOBAL_MEMORY_EVENT_SAVE	(((GLOBAL_MEMORY_EVENT_COUNT-1)/8)+1)	//字节数

#define BUF_MAX_LEN 		4096
#define WRITE_DATA          2048
#define READ_DATA           2048

#define OSD_DEBUG_WIDTH		80

enum enumDebugLine{
//UARTDEBUGMSGON = 3,
OSD_DEBUG_COMPILE_TIME_ERROR = 3,
EVENT_GLOBAL_DEMO_KEY_START,
OSD_DEBUG_STATE_ERROR,
OSD_DEBUG_KEYAD_LIST,
OSD_DEBUG_SOFTVERSION,
OSD_DEBUG_HOST_TEMPERATURE,
OSD_DEBUG_BREAK_AND_PHONE_STATUS,
OSD_DEBUG_OTHER_INFO,
OSD_DEBUG_DVD_TYPE,
//OSD_DEBUG_ADCSTATE,
//OSD_DEBUG_GETTICKT,
//OSD_DEBUG_SAMETICKT,
//OSD_DEBUG_GLOBALTIME,
OSD_DEBUG_PANEL_NAME,
OSD_DEBUG_RUN_TIME,
OSD_DEBUG_LINES,
};

typedef struct flyglobal_struct_info{

	INT fd;
	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;
	BOOL bSpecialPower;
	BOOL bFlyGlobalThreadRunning;
	
	pthread_mutex_t MainThreadMutex;
	pthread_cond_t MainThreadCond;
		
	UINT WriteDataBufLen;
	BYTE  WriteDataBuf[WRITE_DATA];
	BYTE ReadDataBuf[READ_DATA];
	UINT ReadDataBufLen;
	BOOL   bKillDispatchpLEDTestShareMemoryReadThread;
	BOOL bMainThreadRunAgain;
	
	BOOL bKillDispatchFlyGlobalMainThread;
//	BOOL bKillDispatchFlyInterThread;
//	BOOL bKillDispatchFlyLEDTestThread
//	BOOL bKillDispatchFlyMsgQueueReadThread;

	UINT  globalInfoFrameStatus;
	UINT  globalInfoFrameLengthMax;
	UINT  globalInfoFrameLength;
	BYTE  globalInfoFrameCheckSum;
	BYTE  globalInfoFrameBuff[BUF_MAX_LEN];
	
	BYTE iDemoStrLength[OSD_DEBUG_LINES];
	BYTE sDemoStr[OSD_DEBUG_LINES][OSD_DEBUG_WIDTH];
	BYTE iGlobalMemoryEventWhat[GLOBAL_MEMORY_EVENT_SAVE];
	
	//FLY_SHARE_MEMORY_COMMON_DATA pMemory_Share_Common;
	
	/***********Add By LDH**********/
	UINT iOSDDemoStrRow;
	UINT iOSDDemoStrLength;
	BYTE iDriverCompTIME[15];
	BYTE iDriverCompDATE[15];
	BOOL OSDDebugADCThreadstate;
	BOOL OSDDebugGetTickstate;
	UINT32  ThreadstateCount;
	UINT32  GetTickstateCount;
	BOOL  bOSDDebugInit;
	UINT32 OldTick;
	UINT32 PerTick;
	/******************************/
	BYTE *pReadPackageData;
}FLY_GLOBAL_INFO, *P_FLY_GLOBAL_INFO;

/* extern void resetPowerProcOnRecMCUWakeup(struct flyglobal_struct_info *pFlyGlobalInfo);

extern void readFromMmapPrintf(BYTE *buf,UINT length);

extern void ipcEventExchangeProc(UINT32 sourceEvent);
extern void ipcEventProcProc(UINT32 sourceEvent);
extern void msgQueueReadProc(BYTE msgQueueID,BYTE *pData,UINT length);  */

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

//#define DBG1(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgSystemLevel>0){CODE}
//#define DBG2(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgSystemLevel>1){CODE}
//#define DBG3(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgSystemLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}
//#define DBG1(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgSystemLevel>0){CODE}
//#define DBG2(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgSystemLevel>1){CODE}
//#define DBG3(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgSystemLevel>2){CODE}
#endif

#endif