#ifndef FLY_TPMS_H_
#define FLY_TPMS_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

struct flytpms_struct_info{

	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;
	BOOL bSpecialPower;	
	
	//Ì¥Ñ¹Ö÷Ïß³Ì
	BOOL bKillTpmsMainThread;
	pthread_mutex_t TpmsMainMutex;
	pthread_cond_t  TpmsMainCond; 
	BOOL bTpmsMainThreadRunAgain;

	BOOL bHavaTMPS;
	BOOL bPrePairStatus;
	BOOL bCurPairStatus;
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


#endif