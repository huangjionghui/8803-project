#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <cutils/atomic.h>
#include <hardware/hardware.h>  

#include <unistd.h>
#include <sys/mman.h>

#include "allInOneService.h"

struct flyAllInOne *pFlyAllInOneInfo = NULL;

#define _JNIBuff(index)		pFlyAllInOneInfo->BuffToJNI[index]
#define _JNINow				pFlyAllInOneInfo->buffToJNINow
#define _JNILast			pFlyAllInOneInfo->bufToJNILast
#define _JNILx(index)		pFlyAllInOneInfo->buffToJNILx[index]

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

char *bufToString(BYTE *buf, UINT len)
{
	UINT i; 
	UINT j;
	char str[4];

	for (i=0,j=0; i<len && j < DEBUG_MESSAGE_MAX_LEN - 4; i++,j=j+3)
	{
		snprintf(str, sizeof(str), " %02X", buf[i]);
		memcpy(&pFlyAllInOneInfo->bufToStringBuff[j], str, 3);
	}
	pFlyAllInOneInfo->bufToStringBuff[j] = '\0';

	return &pFlyAllInOneInfo->bufToStringBuff[0];
}

char *bufToChar(BYTE *buf, UINT len)
{
	UINT i; 
	UINT j;
	char str[4];

	for (i=0,j=0; i<len && j < DEBUG_MESSAGE_MAX_LEN - 4; i++,j=j+2)
	{
		snprintf(str, sizeof(str), " %c", buf[i]);
		memcpy(&pFlyAllInOneInfo->bufToStringBuff[j], str, 2);
	}
	pFlyAllInOneInfo->bufToStringBuff[j] = '\0';

	return &pFlyAllInOneInfo->bufToStringBuff[0];
}

char *dataToString(UINT32 iData)
{
	snprintf(pFlyAllInOneInfo->dataToStringBuff, sizeof(pFlyAllInOneInfo->dataToStringBuff), " %d", (int)iData);
	return &pFlyAllInOneInfo->dataToStringBuff[0];
}

static void writeStringToDebug(char *debugBuf)
{
	if (pFlyAllInOneInfo != NULL)
	{
		if (pFlyAllInOneInfo->debug_fd > 0)
		{
			write(pFlyAllInOneInfo->debug_fd, debugBuf, strlen(debugBuf));
		}
	}
}

void debugString(char *fmt)
{
	writeStringToDebug(fmt);
}

void debugBuf(char *fmt, BYTE *buf, UINT len)
{
	strcpy(pFlyAllInOneInfo->buffToStringPrint,fmt);
	strcat(pFlyAllInOneInfo->buffToStringPrint,bufToString(buf,len));
	writeStringToDebug(pFlyAllInOneInfo->buffToStringPrint);
}
void debugChar(char *fmt, BYTE *buf, UINT len)
{
	strcpy(pFlyAllInOneInfo->buffToStringPrint,fmt);
	strcat(pFlyAllInOneInfo->buffToStringPrint,bufToChar(buf,len));
	writeStringToDebug(pFlyAllInOneInfo->buffToStringPrint);
}

void debugOneData(char *fmt, UINT32 data)
{
	strcpy(pFlyAllInOneInfo->buffToStringPrint,fmt);
	strcat(pFlyAllInOneInfo->buffToStringPrint,dataToString(data));
	writeStringToDebug(pFlyAllInOneInfo->buffToStringPrint);
}

void debugTwoData(char *fmt, UINT32 dataOne, UINT32 dataTwo)
{
	strcpy(pFlyAllInOneInfo->buffToStringPrint,fmt);
	strcat(pFlyAllInOneInfo->buffToStringPrint,dataToString(dataOne));
	strcat(pFlyAllInOneInfo->buffToStringPrint,dataToString(dataTwo));
	writeStringToDebug(pFlyAllInOneInfo->buffToStringPrint);
}

void debugThreeData(char *fmt, UINT32 dataOne, UINT32 dataTwo, UINT32 dataThree)
{
	strcpy(pFlyAllInOneInfo->buffToStringPrint,fmt);
	strcat(pFlyAllInOneInfo->buffToStringPrint,dataToString(dataOne));
	strcat(pFlyAllInOneInfo->buffToStringPrint,dataToString(dataTwo));
	strcat(pFlyAllInOneInfo->buffToStringPrint,dataToString(dataThree));
	writeStringToDebug(pFlyAllInOneInfo->buffToStringPrint);
}

void ipcStartEvent(UINT32 sourceEvent)
{
	pFlyAllInOneInfo->pMemory_Share_Common->bEventTransSet[sourceEvent] = TRUE;
	ipcEventExchangeProc(sourceEvent);
}

void ipcExchangeEvent(UINT32 sourceEvent,BYTE objectHAL)
{
	if (pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[objectHAL].bHave)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[objectHAL]._p_ipcEventProcProc(sourceEvent);
	}
}

void ipcClearEvent(UINT32 sourceEvent)
{
	pFlyAllInOneInfo->pMemory_Share_Common->bEventTransSet[sourceEvent] = FALSE;
}

BOOL ipcWhatEventOn(UINT32 sourceEvent)
{
	return pFlyAllInOneInfo->pMemory_Share_Common->bEventTransSet[sourceEvent];
}

#define SIG_IPC_MSG 44
void IPCMessageHandler(int signo, siginfo_t *info, void *context)
{
	//debugOneData("\nIPC MSG start:", info->si_int);
	ipcStartEvent(info->si_int);
}
static void setIPCMode(void)
{
	struct sigaction ipc_sa;
	
	//ipc_sa.sa_handler = IPCMessageHandler;
	ipc_sa.sa_sigaction = IPCMessageHandler;
	sigemptyset(&ipc_sa.sa_mask);
	ipc_sa.sa_flags = SA_RESTART | SA_SIGINFO;
	ipc_sa.sa_restorer = NULL;
	sigaction(SIG_IPC_MSG, &ipc_sa, NULL);
}

BOOL writeDataToHardware(BYTE *buf, UINT32 len)
{
	BOOL status = FALSE;

	if (pFlyAllInOneInfo == NULL || pFlyAllInOneInfo->hardware_fd < 0)
	{
		return FALSE;
	}

	if (write(pFlyAllInOneInfo->hardware_fd, buf, len) > 0)
	{
		status = TRUE;
	}

	return status;
}

UINT readDataFromHardwareNoBlock(BYTE *pData,UINT length)
{
	int ret;

	if (pFlyAllInOneInfo == NULL || pFlyAllInOneInfo->hardware_fd < 0)
	{
		return 0;
	}

	ret = read(pFlyAllInOneInfo->hardware_fd,pData,length);

	return ret;
}

void hardwareReadProc(BYTE iWhatHAL,BYTE *p,UINT32 length)
{
	//debugOneData("\nhardwareReadProc read ID:", iWhatHAL);
	if (pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[iWhatHAL].bHave)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[iWhatHAL]._p_readFromhardwareProc(p,length);
	}
}

void *hardwareThread(void *arg)
{
	INT ret;

	UINT32 sourceEvent;

	BYTE buff[256];
	
	struct pollfd read_fds;
	
	memset(&read_fds, 0, sizeof(read_fds));
	read_fds.fd = pFlyAllInOneInfo->hardware_fd;
	read_fds.events = POLLIN|POLLRDNORM;
	
	//buff[0]由驱动返回，告诉HAL是发给谁的
	buff[1] = S_BLOCK_ID;

	while (!pFlyAllInOneInfo->bKillhardwareThread)
	{
	
		if (poll(&read_fds, 1, -1) > 0)
		{
			if ((read_fds.revents & (POLLIN|POLLRDNORM)) == (POLLIN|POLLRDNORM))
			{
				ret = read(pFlyAllInOneInfo->hardware_fd, buff, 256);
				if (ret > 2)
				{	
					hardwareReadProc(buff[0],&buff[2],ret-2);
				}
			}
		}
	}

	return NULL;
}

void initToJNIBuff(void)
{
	UINT i;

	for (i = 0;i < SHARE_MEMORY_MAX;i++)
	{
		sem_init(&pFlyAllInOneInfo->buffToJNIReadSem[i], 0, 0);	
	}
	pthread_mutex_init(&pFlyAllInOneInfo->buffToJNIMutex, NULL);
}

UINT32 dropLastData(void)
{
	UINT32 iNext = 0;

	if (_JNIBuff(_JNILast))//长度有效
	{
		iNext =  _JNILast + _JNIBuff(_JNILast);
	}
	if (iNext >= BUFF_TO_JNI_MAX_LENGTH)
	{
		iNext = 0;
	}

	return iNext;
}

BOOL toJNIBuffHaveIdle(UINT32 iLength)//尾部一定要留一个字节，为安全起见
{
	UINT32 iIdles;

	if (iLength > BUFF_TO_JNI_MAX_LENGTH - _JNINow - 1)//尾部已经无足够空间存储一条消息
	{
		_JNIBuff(_JNINow) = BUFF_TO_JNI_ROLLBACK;//设标志位，返回到头部继续
		_JNINow = 0;
	}

	if (_JNINow >= _JNILast)//还剩多少空间
	{
		iIdles = BUFF_TO_JNI_MAX_LENGTH - 1 - (_JNINow - _JNILast);
	}
	else
	{
		iIdles = _JNILast - _JNINow - 1;
	}

	if (iIdles >= iLength)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void moveLxSameLastToIndex(UINT32 index)
{
	UINT32 i;

	for (i = 0;i < SHARE_MEMORY_MAX;i++)
	{
		if (_JNILx(i) == _JNILast)
		{
			_JNILx(i) = index;
		}
	}
	_JNILast = index;
}

UINT32 writeToJNIBuff(BYTE iWhatHAL,BYTE *p,UINT32 length)
{
	//UINT32 i;
	BYTE crc = 0;//校验CRC

	if (!pFlyAllInOneInfo->bBuffReadHave[iWhatHAL])//没有读取，不会写入
	{
		return 0;
	}

	if (length+6 > 0xFF)//超过最大表示范围，无效数据
	{
		return 0;
	}

	//crc = length +1;//不校验
	//for (i=0; i<length; i++)
	//{
	//	crc += p[i];
	//}

	pthread_mutex_lock(&pFlyAllInOneInfo->buffToJNIMutex);

	while (!toJNIBuffHaveIdle(length+6))//空间不够，扔掉最早的数据
	{
		moveLxSameLastToIndex(dropLastData());
	}

	_JNIBuff(_JNINow) = length+6;_JNINow++;
	_JNIBuff(_JNINow) = iWhatHAL;_JNINow++;
	_JNIBuff(_JNINow) = 0xFF;_JNINow++;
	_JNIBuff(_JNINow) = 0x55;_JNINow++;
	_JNIBuff(_JNINow) = length+1;_JNINow++;

	memcpy(&_JNIBuff(_JNINow),p,length);_JNINow += length;

	_JNIBuff(_JNINow) = crc;_JNINow++;

	//debugThreeData("\nwriteToJNI",iWhatHAL,_JNINow,_JNILast);

	//debugThreeData("\nwriteToJNI",_JNINow,_JNILast,_JNIBuff(_JNILast));
	//debugThreeData("\nwriteToJNIBuffLastHAL",_JNINow,_JNILast,_JNIBuff(_JNILast));
	//debugBuf("LastHALData",&_JNIBuff(_JNILast),_JNIBuff(_JNILast));

	pthread_mutex_unlock(&pFlyAllInOneInfo->buffToJNIMutex);

	sem_post(&pFlyAllInOneInfo->buffToJNIReadSem[iWhatHAL]);
	return length;
}

UINT32 jumpToNextData(BYTE iWhatHAL)
{
	UINT32 iNext = 0;

	if (_JNIBuff(_JNILx(iWhatHAL)))
	{
		iNext = _JNILx(iWhatHAL) + _JNIBuff(_JNILx(iWhatHAL));
	}
	if (iNext >= BUFF_TO_JNI_MAX_LENGTH)
	{
		iNext = 0;
	}
	return iNext;
}

void moveLastSameLxToIndex(BYTE iWhatHAL,UINT32 index)
{
	if (_JNILast == _JNILx(iWhatHAL))
	{
		_JNILast = index;
	}
	_JNILx(iWhatHAL) = index;
}

UINT32 readFromJNIBuff(BYTE iWhatHAL,BYTE *p,UINT32 length)
{
	UINT iReturnLength = 0;

	pFlyAllInOneInfo->bBuffReadHave[iWhatHAL] = TRUE;

	do 
	{
		pthread_mutex_lock(&pFlyAllInOneInfo->buffToJNIMutex);

		while (_JNILx(iWhatHAL) != _JNINow)//有数据
		{
			//debugThreeData("\nreadFromJNI",_JNINow,_JNILast,_JNIBuff(_JNILast));
			//debugThreeData("\nreadFromNIBuffLastLength",_JNINow,_JNILast,_JNIBuff(_JNILast));

			if (BUFF_TO_JNI_ROLLBACK == _JNIBuff(_JNILx(iWhatHAL)))//发现标志位，返回到头部继续
			{
				moveLastSameLxToIndex(iWhatHAL,0);
				continue;
			}
			else
			{
				iReturnLength = _JNIBuff(_JNILx(iWhatHAL)) - 2;//有效数据长度

				if(iWhatHAL == _JNIBuff(_JNILx(iWhatHAL)+1))//是自己的
				{
					//debugThreeData("\nreadFromJNI",iWhatHAL,_JNILx(iWhatHAL),_JNILast);

					memcpy(p,&_JNIBuff(_JNILx(iWhatHAL)+2),iReturnLength);//拷贝
					_JNIBuff(_JNILx(iWhatHAL)+1) = SHARE_MEMORY_MAX;//此消息已被发送

					moveLastSameLxToIndex(iWhatHAL,jumpToNextData(iWhatHAL));

					pthread_mutex_unlock(&pFlyAllInOneInfo->buffToJNIMutex);
					return iReturnLength;
				}
				else
				{
					_JNILx(iWhatHAL) = jumpToNextData(iWhatHAL);
				}
			}
		}

		pthread_mutex_unlock(&pFlyAllInOneInfo->buffToJNIMutex);
		sem_wait(&pFlyAllInOneInfo->buffToJNIReadSem[iWhatHAL]);
	} while (1);

	return iReturnLength;
}

BOOL allInOneDeinit(void)
{
	return TRUE;
}

BOOL allInOneInit(void)
{
	int res;
	pthread_t thread_id;

	//创建结构体
	pFlyAllInOneInfo = (struct flyAllInOne*)malloc(sizeof(struct flyAllInOne));
	if (NULL == pFlyAllInOneInfo)
	{
		return FALSE;
	}
	memset(pFlyAllInOneInfo,0,sizeof(struct flyAllInOne));
	
	initToJNIBuff();

	setIPCMode();

	
	//打开和共享内存驱动
	pFlyAllInOneInfo->mmap_fd = open("/dev/FlyMmap",O_RDWR);
	if (pFlyAllInOneInfo->mmap_fd < 0)
	{
		debugString("\nopen FlyMmap Fail!");
		return FALSE;
	}
	
	pFlyAllInOneInfo->pMemory_Share_Common
		= (FLY_SHARE_MEMORY_COMMON_DATA *)mmap(NULL, 
		SHARE_MAP_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, pFlyAllInOneInfo->mmap_fd, 0);

	//设置外部函数指针
	pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[CURRENT_SHARE_MEMORY_ID]._p_ipcEventProcProc = ipcEventProcProc;
	pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[CURRENT_SHARE_MEMORY_ID]._p_readFromhardwareProc = readFromhardwareProc;
	pFlyAllInOneInfo->pMemory_Share_Common->processOthersHal[CURRENT_SHARE_MEMORY_ID].bHave = TRUE;

	//设置本体函数指针
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugString = debugString;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugBuf = debugBuf;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugChar = debugChar;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugOneData = debugOneData;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugTwoData = debugTwoData;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_debugThreeData = debugThreeData;

	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_ipcStartEvent = ipcStartEvent;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_ipcClearEvent = ipcClearEvent;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_ipcWhatEventOn = ipcWhatEventOn;

    pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_writeDataToHardware = writeDataToHardware;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_readDataFromHardwareNoBlock = readDataFromHardwareNoBlock;
	
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_writeToJNIBuff = writeToJNIBuff;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_readFromJNIBuff = readFromJNIBuff;

	
	//打开Debug驱动
	pFlyAllInOneInfo->debug_fd = open("/dev/FlyDebug",O_RDWR | O_NONBLOCK);
	if (pFlyAllInOneInfo->debug_fd < 0)
	{
		return FALSE;
	}
	//此时可以打印调试信息
	debugOneData("\nshare mmap size:", sizeof(FLY_SHARE_MEMORY_COMMON_DATA));

	//打开Hardware驱动
	 pFlyAllInOneInfo->hardware_fd = open("/dev/FlyHardware",O_RDWR);
	if (pFlyAllInOneInfo->hardware_fd < 0)
	{
		debugString("\nopen FlyHardware Fail!");
		return FALSE;
	}
	pFlyAllInOneInfo->bKillhardwareThread = FALSE;
	res = pthread_create(&thread_id,NULL,hardwareThread,NULL);
	debugOneData("hardwareThread ID:",thread_id);
	if(res != 0) 
	{
		return FALSE;
	}

	return TRUE;
}
