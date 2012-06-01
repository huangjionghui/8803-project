#ifndef FLY_SYSTEM_H_
#define FLY_SYSTEM_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/global.h"

#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_SYSTEM

#define CHECK_SHELL_BABY_INNER_TIME	(61800*10)

enum enumControlHostAction{
	CONTROL_TO_SLEEP,
	CONTROL_TO_RESET,
	CONTROL_TO_RESET_FACTORY
};

typedef struct _FLY_SYSTEM_CARBODY_INFO
	{
		BYTE preLightBrightDuty;//LCD背光亮度
		BYTE tmpLightBrightDuty;
		BYTE curLightBrightDuty;

		ULONG iSendShellBabyTime;
		UINT32 iShellBabySend;
		UINT32 iShellBabyRec;

		BOOL bFanControlOn;

		BYTE iLEDBlinkOnWhat;
		BYTE iLEDBlinkOnWhatSub;
		ULONG iLEDBlinkOnTime;
	}FLY_SYSTEM_CARBODY_INFO, *P_FLY_SYSTEM_CARBODY_INFO;
	
struct flysystem_struct_info{
	
	BOOL bOpen;
	BOOL bPower;
	BOOL bSpecialPower;
	BOOL bUserPowerUp;
	
	BOOL bKillDispatchFlyMainThread;
	BOOL bFlyMainThreadRunning;
	pthread_mutex_t MainThreadMutex;
	pthread_cond_t  MainThreadCond; 
	BOOL bMainThreadRunAgain;

	ULONG iProcACCOffTime;

	BOOL bSendPingStart;
	ULONG iSendPingTimer;

	BOOL bStandbyStatusWithACCOff;
	BOOL bStandbyStatus;
	
	BOOL bHaveTVModule;
	BOOL bHaveTPMSModule;

	BOOL bIICTestHaveSend;
	ULONG iIICTestRecTime;
	BYTE iIICTestSendData[8];

	BOOL bControlLCDIdleNormal;
	BOOL bControlLCDIdleBackVideo;

	FLY_SYSTEM_CARBODY_INFO SystemCarbodyInfo;
};

extern void resetPowerOn(void);//开机
extern void resetPowerOff(void);//关机
extern void resetPowerUp(void);//ACC On
extern void resetPowerDown(void);//ACC Off

extern void resetPowerProcOnRecMCUWakeup(void);

extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);

extern struct flysystem_struct_info *pFlySystemInfo;

#define DEBUG_MSG_ON 0

#if DEBUG_MSG_ON
#define DBG0(CODE) if(1){CODE}

#define DBG1(CODE) if(1){CODE}
#define DBG2(CODE) if(1){CODE}
#define DBG3(CODE) if(1){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}
//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel>2){CODE}
#endif

#endif