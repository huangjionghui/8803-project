#ifndef ALL_IN_ONE_H_
#define ALL_IN_ONE_H_

#include <sched.h>
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/global.h"

#define DEBUG_MESSAGE_MAX_LEN 256

#define BUFF_TO_JNI_MAX_LENGTH	(1024*4)
#define BUFF_TO_JNI_ROLLBACK	0

struct flyAllInOne{
	volatile FLY_SHARE_MEMORY_COMMON_DATA *pMemory_Share_Common;

	//存储所有HAL给JNI的消息，存储格式，Length HALID Message
	//Length为Length + HALID + Message的长度
	//有效数据Message
	BYTE BuffToJNI[BUFF_TO_JNI_MAX_LENGTH];
	BOOL bBuffReadHave[SHARE_MEMORY_MAX];
	UINT32 bufToJNILast;
	UINT32 buffToJNINow;
	UINT32 buffToJNILx[SHARE_MEMORY_MAX];
	sem_t buffToJNIReadSem[SHARE_MEMORY_MAX];
	pthread_mutex_t buffToJNIMutex;

	int debug_fd;
	char dataToStringBuff[16];
	char bufToStringBuff[DEBUG_MESSAGE_MAX_LEN];
	char buffToStringPrint[DEBUG_MESSAGE_MAX_LEN*2];

	int mmap_fd;
	BOOL bKillmmapThread;

	int hardware_fd;
	BOOL bKillhardwareThread;
};

extern BOOL allInOneDeinit(void);
extern BOOL allInOneInit(void);

extern void debugString(char *fmt);
extern void debugBuf(char *fmt, BYTE *buf, UINT len);
extern void debugChar(char *fmt, BYTE *buf, UINT len);
extern void debugOneData(char *fmt, UINT32 data);
extern void debugTwoData(char *fmt, UINT32 dataOne, UINT32 dataTwo);
extern void debugThreeData(char *fmt, UINT32 dataOne, UINT32 dataTwo, UINT32 dataThree);

extern void ipcStartEvent(UINT32 sourceEvent);
extern void ipcClearEvent(UINT32 sourceEvent);
extern BOOL ipcWhatEventOn(UINT32 sourceEvent);

extern UINT32 writeToJNIBuff(BYTE iWhatHAL,BYTE *p,UINT32 length);
extern UINT32 readFromJNIBuff(BYTE iWhatHAL,BYTE *p,UINT32 length);

extern BOOL writeDataToHardware(BYTE *buf, UINT32 len);

extern UINT readDataFromHardwareNoBlock(BYTE *pData,UINT length);

//内部HAL函数导入
extern void ipcEventExchangeProc(UINT32 sourceEvent);
extern void ipcEventProcProc(UINT32 sourceEvent);

#endif