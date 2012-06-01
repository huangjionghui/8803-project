#ifndef FLY_AUDIO_H_
#define FLY_AUDIO_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

typedef struct flyaudio_struct_info{

	//flyaudio read 
	INT fd;
	BOOL bKillAudioReadThread;
	
}FLY_AUDIO_INFO,*P_FLY_AUDIO_INFO;

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

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel>0){CODE}
#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel>1){CODE}
#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel>2){CODE}
#endif

#endif