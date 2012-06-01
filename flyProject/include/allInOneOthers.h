#ifndef ALL_IN_ONE_H_
#define ALL_IN_ONE_H_

#include <sched.h>
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/global.h"

#define DEBUG_MESSAGE_MAX_LEN 256

struct flyAllInOne{
	volatile FLY_SHARE_MEMORY_COMMON_DATA *pMemory_Share_Common;

	int debug_fd;
	char dataToStringBuff[16];
	char bufToStringBuff[DEBUG_MESSAGE_MAX_LEN];
	char buffToStringPrint[DEBUG_MESSAGE_MAX_LEN*2];

	int mmap_fd;
	BOOL bKillmmapThread;
};

extern struct flyAllInOne *pFlyAllInOneInfo;

extern BOOL allInOneDeinit(void);
extern BOOL allInOneInit(void);

extern _t_debugString debugString;
extern _t_debugBuf debugBuf;
extern _t_debugChar debugChar;
extern _t_debugOneData debugOneData;
extern _t_debugTwoData debugTwoData;
extern _t_debugThreeData debugThreeData;

extern _t_ipcStartEvent ipcStartEvent;
extern _t_ipcClearEvent ipcClearEvent;
extern _t_ipcWhatEventOn ipcWhatEventOn;

extern _t_readDataFromHardwareNoBlock readDataFromHardwareNoBlock;
extern _t_writeDataToHardware writeDataToHardware;

extern _t_writeToJNIBuff writeToJNIBuff;
extern _t_readFromJNIBuff readFromJNIBuff;
//内部HAL函数导入
extern void ipcEventProcProc(UINT32 sourceEvent);
extern void readFromhardwareProc(BYTE *buf,UINT length);

#endif