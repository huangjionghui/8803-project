#ifndef FLY_XMRADIO_H_
#define FLY_XMRADIO_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

#define BUF_MAX_LEN 4096

typedef struct flyxmradio_struct_info{

	pthread_mutex_t userReadMutex;			//ª•≥‚À¯
	sem_t           userReadSem;			//∂¡–≈∫≈¡ø
	
	UINT  buffToUserHx;
	UINT  buffToUserLx;
	BYTE  buffToUser[BUF_MAX_LEN];
	
	UINT  xmradioInfoFrameStatus;
	UINT  xmradioInfoFrameLengthMax;
	UINT  xmradioInfoFrameLength;
	BYTE  xmradioInfoFrameCheckSum;
	BYTE  xmradioInfoFrameBuff[BUF_MAX_LEN];
}FLY_XMRADIO_INFO,*P_FLY_XMRADIO_INFO;


extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);

#endif