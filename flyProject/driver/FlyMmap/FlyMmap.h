#ifndef FLY_MMAP_H_
#define FLY_MMAP_H_

#define 	MISC_DYNAMIC_MINOR 		255				//动态设备号
#define 	DEVICE_NAME 			"FlyMmap"		//驱动名
#define     BUF_MAX_SIZE            300

#include "../../include/ShareMemoryStruct.h"

#define ALLOC_MEMORY_USE_NORMAL	1

typedef struct fly_event_trans_info{
	struct rw_semaphore SemRWEvent;
}FlyEventTrans;

typedef struct fly_share_mmap_info{
	FlyEventTrans EventTransInfo;
}FLY_MMAP_INFO,*P_FLY_MMAP_INFO;


#define DEBUG_MSG_ON  0

#if DEBUG_MSG_ON
#define DBG(CODE) if(1){CODE}
#else
#define DBG(CODE) if(0){CODE}
#endif

#endif