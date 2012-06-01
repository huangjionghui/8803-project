#ifndef FLY_EXDISPLAY_H_
#define FLY_EXDISPLAY_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_EXDISPLAY

enum ExDisplayChannel{eAD_Init=0,eAD_DVD,eAD_Radio,eAD_iPod,eAD_AUX,eAD_TV,eAD_MP3,eAD_XM,eAD_BT};

typedef struct flyexdisplay_struct_info{
	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;
	
	//flyexdisplayÖ÷Ïß³Ì
	BOOL bKillExdisplayMainThread;
	pthread_mutex_t ExdisplayMainMutex;
	pthread_cond_t  ExdisplayMainCond; 
	BOOL ExdisplayMainThreadRunAgain;
	
	BYTE ExCurrentState;
	
	BYTE iSendToMCULength[256];
	BYTE iSendToMCUBuff[256][256];
		
	BYTE iReversed;
}FLY_EXDISPLAY_INFO,*P_FLY_EXDISPLAY_INFO;


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

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTPMSLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTPMSLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTPMSLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTPMSLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTPMSLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTPMSLevel>2){CODE}
#endif

#define DLOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "exdisplay", __VA_ARGS__)
#define DLOGI(...) __android_log_print(ANDROID_LOG_INFO  ,  "exdisplay", __VA_ARGS__)

#endif