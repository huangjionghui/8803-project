#ifndef FLY_TV_H_
#define FLY_TV_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

struct flytv_struct_info{

	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;
	BOOL bSpecialPower;

	BOOL bHavaTV;
	
	BYTE oldAudioInput;
	
	//TVÖ÷Ïß³Ì
	BOOL bKillTvMainThread;
	pthread_cond_t  TvMainCond; 
	pthread_mutex_t TvMainMutex;			
	BOOL bTvMainThreadRunAgain;
};


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

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel>2){CODE}
#endif

#endif