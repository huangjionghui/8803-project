#ifndef FLY_VIDEO_H_
#define FLY_VIDEO_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

enum setDisplay{E_C_COLOR = 0,E_C_HUE,E_C_CONTRAST,E_C_BRIGHTNESS};

typedef struct flyvideo_struct_info{

	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;

	BOOL bKillFlyVideoMainThread;
	pthread_mutex_t MainThreadMutex;
	pthread_cond_t  MainThreadCond; 
	BOOL bMainThreadRunAgain;

	BOOL bBackVideoOn;
	BOOL bBackVideoForceReturn;
	ULONG iBackVideoReturnTime;
	BOOL bBackPreVideoPal;

	BOOL bAuxVideoOn;
	BOOL bAuxVideoForceReturn;
	BOOL bAuxVideoReturnTime;
	BOOL bAuxPreVideoPal;

	BOOL bPreVideoPal;
	BOOL bCurVideoPal;

	BYTE iPreVideoChannel;
	BYTE iTempVideoChannel;
	BYTE iCurVideoChannel;
	BYTE iActualVideoChannel;
	BOOL bChannelChangeNeedEnable;
	ULONG iChannelChangeTime;

	BYTE iTempVideoParaColor;
	BYTE iTempVideoParaHue;
	BYTE iTempVideoParaContrast;
	BYTE iTempVideoParaBrightness;

	BYTE iVideoParaColor;
	BYTE iVideoParaHue;
	BYTE iVideoParaContrast;
	BYTE iVideoParaBrightness;

	BOOL bEnterSuspend;
}FLY_VIDEO_INFO,*P_FLY_VIDEO_INFO;

extern void readFromMmapPrintf(BYTE *buf,UINT length);

extern void ipcEventExchangeProc(UINT32 sourceEvent);
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

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel>0){CODE}
#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel>1){CODE}
#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel>2){CODE}
#endif

#endif