#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <cutils/atomic.h>
#include <hardware/hardware.h>  
#include <unistd.h>
#include <sys/mman.h>

#include "allInOneOthers.h"

struct flyAllInOne *pFlyAllInOneInfo = NULL;

UINT32 forU8ToU32LSB(BYTE *p)
{
	UINT32 iTemp = 0;
	iTemp = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
	return iTemp;
}

void forU32TopU8LSB(UINT32 data,BYTE *p)
{
	p[0] = data;
	data = data >> 8;p[1] = data;
	data = data >> 8;p[2] = data;
	data = data >> 8;p[3] = data;
}

_t_debugString debugString;
_t_debugBuf debugBuf;
_t_debugChar debugChar;
_t_debugOneData debugOneData;
_t_debugTwoData debugTwoData;
_t_debugThreeData debugThreeData;

_t_ipcStartEvent ipcStartEvent;
_t_ipcClearEvent ipcClearEvent;
_t_ipcWhatEventOn ipcWhatEventOn;

_t_msgWriteToSerial _p_msgWriteToSerial;
_t_msgReadTpmsFromSerial _p_msgReadTpmsFromSerial;
_t_msgReadTvFromSerial _p_msgReadTvFromSerial;

_t_readDataFromHardwareNoBlock readDataFromHardwareNoBlock;
_t_writeDataToHardware writeDataToHardware;

_t_writeToJNIBuff writeToJNIBuff;
_t_readFromJNIBuff readFromJNIBuff;

BOOL allInOneDeinit(void)
{
	return TRUE;
}

BOOL allInOneInit(void)
{
	int shareMemoryFD;

	int res;
	pthread_t thread_id;

	//创建结构体
	pFlyAllInOneInfo = (struct flyAllInOne*)malloc(sizeof(struct flyAllInOne));
	if (NULL == pFlyAllInOneInfo)
	{
		return FALSE;
	}

	//获取共享内存地址
	shareMemoryFD = open("/dev/FlyMmap", O_RDWR);
	if (shareMemoryFD < 0)
	{
		return FALSE;
	}
	pFlyAllInOneInfo->pMemory_Share_Common
		= (FLY_SHARE_MEMORY_COMMON_DATA *)mmap(NULL, 
		SHARE_MAP_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, shareMemoryFD, 0);

	//设置外部函数指针
	debugString = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugString;
	debugBuf = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugBuf;
	debugChar = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugChar;
	debugOneData = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugOneData;
	debugTwoData = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugTwoData;
	debugThreeData = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugThreeData;

	ipcStartEvent = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_ipcStartEvent;
	ipcClearEvent = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_ipcClearEvent;
	ipcWhatEventOn = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_ipcWhatEventOn;

	writeToJNIBuff = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_writeToJNIBuff;
	readFromJNIBuff = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_readFromJNIBuff;

	readDataFromHardwareNoBlock = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_readDataFromHardwareNoBlock;
	writeDataToHardware = pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_writeDataToHardware;

	//设置本体函数指针
	pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[CURRENT_SHARE_MEMORY_ID]._p_ipcEventProcProc = ipcEventProcProc;
	pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[CURRENT_SHARE_MEMORY_ID]._p_readFromhardwareProc = readFromhardwareProc;
	pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[CURRENT_SHARE_MEMORY_ID].bHave = TRUE;

	return TRUE;
}
