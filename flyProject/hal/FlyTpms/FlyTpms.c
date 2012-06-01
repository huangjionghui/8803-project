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

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_TPMS
#define LOCAL_HAL_NAME		"flytpms Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_TPMS

#include "FlyTpms.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"


struct flytpms_struct_info *pFlyTpmsInfo = NULL;

static void returnTPMSPairStatus(struct flytpms_struct_info *pFlyTpmsInfo,BOOL bPair);
static void returnAllInfo(struct flytpms_struct_info *pFlyTpmsInfo,BYTE p,BYTE t,BYTE v);
static void control_TPMSLearnStart(struct flytpms_struct_info *pFlyTpmsInfo);
void *FlyTpmsMainThreadProc(void *arg);

static void FlyTPMS_infoProcessor(struct flytpms_struct_info *pFlyTpmsInfo, BYTE dataOne, BYTE dataTwo)
{
	BYTE position;
	BYTE type;
	BYTE value;
	
	if (dataOne == 0xe1) 
	{
		if (dataTwo == 0x10) 
		{ //Learn
			returnTPMSPairStatus(pFlyTpmsInfo,TRUE);
			DBG0(debugString("\nFlyAudio TPMS Learning");)
		}
		else if (dataTwo == 0x20)
		{ //learn end
			pFlyTpmsInfo->bPrePairStatus = FALSE;
			pFlyTpmsInfo->bCurPairStatus = FALSE;
			returnTPMSPairStatus(pFlyTpmsInfo,FALSE);
			DBG0(debugString("\nFlyAudio TPMS Learn end");)
		}
		else //返回四个轮子的胎压信息
		{
			position = (dataOne>>2)&0x03; //0:FL 1:FR 2:RL 3:RR
			type = dataOne&0x03;	      //0:POWER  1: UPDATA 2:PRESSURE  3:TEMPERATURE
			value = dataTwo;		      //value
			returnAllInfo(pFlyTpmsInfo,position,type,value);
		}
	}
	else
	{
		position = (dataOne>>2)&0x03; //0:FL 1:FR 2:RL 3:RR
		type = dataOne&0x03;	   //0:POWER  1: UPDATA 2:PRESSURE  3:TEMPERATURE
		value = dataTwo;		   //value
		returnAllInfo(pFlyTpmsInfo,position,type,value);
	}
}

void msgReadTpmsFromSerial(BYTE msgQueueID,BYTE *pData,UINT length)
{
	DBG3(debugOneData("\nTPMS-HAL msgQueue msgID ", msgQueueID);)
	DBG3(debugBuf("\nTPMS-HAL read msgQueue data:",pData,length);)
	if (length > 0)
	{
		FlyTPMS_infoProcessor(pFlyTpmsInfo, pData[0], pData[1]);	
	}
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
}

void readFromMmapPrintf(BYTE *buf,UINT length)
{
	//DBG0(debugBuf("AudioHAL read from Mmap",buf,length);)
}

void ipcEventProcProc(UINT32 sourceEvent)
{

}

static void flyAudioReturnToUser(struct flytpms_struct_info *pFlyTpmsInfo,BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nTPMS-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nTPMS-HAL write  bytes to User Error:", buf,len);)
	}
}

static void returnPowerStatus(struct flytpms_struct_info *pFlyTpmsInfo,BOOL bPower)
{
	BYTE buff[2] = {0x01,0x00};//0x00:关 0x01:开
	if (bPower)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyTpmsInfo,buff,2);
}
static void returnWorkMode(struct flytpms_struct_info *pFlyTpmsInfo,BOOL bWork)
{
	BYTE buff[2];
	buff[0] = 0x02;
	if (bWork)
	{
		buff[1] = 0x01;
	} 
	
	flyAudioReturnToUser(pFlyTpmsInfo,buff,2);
}
static void returnTPMSConnectStatus(struct flytpms_struct_info *pFlyTpmsInfo,BOOL bHave)
{
	return;//由System返回
	BYTE buff[2] = {0x03,0x00};
	if (bHave)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyTpmsInfo,buff,2);
}
static void returnTPMSPairStatus(struct flytpms_struct_info *pFlyTpmsInfo,BOOL bPair)
{
	BYTE buff[2]={0x10,0x00};
	if (bPair)
	{
		buff[1] = bPair;
	}

	flyAudioReturnToUser(pFlyTpmsInfo,buff,2);
}
//p 0:FL 1:FR 2:RL 3:RR
//t 0:POWER  1: UPDATA 2:PRESSURE  3:TEMPERATURE
//v value
static void returnAllInfo(struct flytpms_struct_info *pFlyTpmsInfo,BYTE p,BYTE t,BYTE v)
{
	BYTE buff[3];
	buff[0] = 0x20 | p;
	buff[1] = t;
	buff[2] = v;
	flyAudioReturnToUser(pFlyTpmsInfo,buff,3);
}

static void TPMSWriteFile(struct flytpms_struct_info *pFlyTpmsInfo,BYTE *p,UINT16 length)
{
	DBG3(debugBuf("\n*********TPMS write:", p, length);)
	//msgQueueWrite(SHARE_MEMORY_TPMS,SHARE_MEMORY_COMMON,p,length);
	if (pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial)
		pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial(SHARE_MEMORY_TPMS,p,length);
}

static void TMPSCmdPrintf(struct flytpms_struct_info *pFlyTpmsInfo,BYTE *buf,UINT16 len)
{
	BYTE sendBuf[2];
	sendBuf[0] = buf[0];
	sendBuf[1] = buf[1];
	TPMSWriteFile(pFlyTpmsInfo,sendBuf,2);
}
static void control_TPMSLearnStart(struct flytpms_struct_info *pFlyTpmsInfo)
{
	BYTE buff[2];
	buff[0] = 0xe0;
	buff[1] = 0x01;
	TMPSCmdPrintf(pFlyTpmsInfo,buff,2);
}
static void control_TPMSLearnEnd(struct flytpms_struct_info *pFlyTpmsInfo)
{
	BYTE buff[2];
	buff[0] = 0xe0;
	buff[1] = 0x02;
	TMPSCmdPrintf(pFlyTpmsInfo,buff,2);
}

 static int createThread(struct flytpms_struct_info *pFlyTpmsInfo)
 {
	INT res;
	pthread_t thread_id;
	
	pFlyTpmsInfo->bKillTpmsMainThread = FALSE;
	res = pthread_create(&thread_id, NULL, FlyTpmsMainThreadProc,pFlyTpmsInfo);
	if(res != 0) 
	{
		return -1;
	}
	
	pFlyTpmsInfo->bPowerUp = TRUE;
	return 0;
 }

static void onRightDataProcessor(struct flytpms_struct_info *pFlyTpmsInfo,BYTE *buf, UINT16 len)
{
	switch (buf[0])
	{
		case 0x01://初始化命令
			if (0x01 == buf[1])
			{
				returnPowerStatus(pFlyTpmsInfo,TRUE);
				returnWorkMode(pFlyTpmsInfo,TRUE);
				pFlyTpmsInfo->bPower = TRUE;
				
				if (!pFlyTpmsInfo->bPowerUp)
				{
					createThread(pFlyTpmsInfo);
				}
				PostSignal(&pFlyTpmsInfo->TpmsMainMutex,&pFlyTpmsInfo->TpmsMainCond,&pFlyTpmsInfo->bTpmsMainThreadRunAgain);
			}
			else if (0x00 == buf[1])
			{
				pFlyTpmsInfo->bPower = FALSE;
				returnPowerStatus(pFlyTpmsInfo,FALSE);
				returnWorkMode(pFlyTpmsInfo,TRUE);
			}		
			break;
		
		case 0x03://软件模拟按键
			if (pFlyTpmsInfo->bCurPairStatus)
			{
				pFlyTpmsInfo->bPrePairStatus = FALSE;
			} 
			else
			{
				pFlyTpmsInfo->bPrePairStatus = TRUE;
			}
			PostSignal(&pFlyTpmsInfo->TpmsMainMutex,&pFlyTpmsInfo->TpmsMainCond,&pFlyTpmsInfo->bTpmsMainThreadRunAgain);
			break;
		
		case 0x10://TPMS 配对
			pFlyTpmsInfo->bPrePairStatus = (BOOL)buf[1];
			PostSignal(&pFlyTpmsInfo->TpmsMainMutex,&pFlyTpmsInfo->TpmsMainCond,&pFlyTpmsInfo->bTpmsMainThreadRunAgain);
			break;
		
		case 0xFF:
			break;
			
		default:
			break;

	}
}

void *FlyTpmsMainThreadProc(void *arg)
{
	INT ret;
	struct timeval timenow;
	struct timespec timeout;
	struct flytpms_struct_info *pFlyTpmsInfo = (struct flytpms_struct_info *)arg;
	
	while (!pFlyTpmsInfo->bKillTpmsMainThread)
	{
		WaitSignedTimeOut(&pFlyTpmsInfo->TpmsMainMutex,&pFlyTpmsInfo->TpmsMainCond,&pFlyTpmsInfo->bTpmsMainThreadRunAgain,1000);
		
		if (pFlyTpmsInfo->bPower)
		{
			if(pFlyTpmsInfo->bPrePairStatus != pFlyTpmsInfo->bCurPairStatus)
			{
				pFlyTpmsInfo->bCurPairStatus = pFlyTpmsInfo->bPrePairStatus;
				if (pFlyTpmsInfo->bCurPairStatus)
				{
					//进入配对
					control_TPMSLearnStart(pFlyTpmsInfo);
				}
				else
				{
					//取消配对
					control_TPMSLearnEnd(pFlyTpmsInfo);
				}
			}

			if (pFlyTpmsInfo->bHavaTMPS != pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule)
			{
				pFlyTpmsInfo->bHavaTMPS = pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule;
				returnTPMSConnectStatus(pFlyTpmsInfo,pFlyTpmsInfo->bHavaTMPS);
			}
		}
		
	}
	
	pFlyTpmsInfo->bKillTpmsMainThread = TRUE;
	DBG0(debugString("FlyTpmsMainThreadProc exit\n");)
	return NULL;
}
 
 /*==========================以下为导出函数====================================*/
 /******************************************************************************/
 /******************************************************************************/
 /******************************************************************************/
 /*============================================================================*/
 
/********************************************************************************
 **函数名称：fly_open_device（）函数
 **函数功能：打开设备
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyOpenDevice(void)
 {
	pFlyTpmsInfo->bOpen = TRUE;
	return HAL_TPMS_RETURN_FD;
 }
 
 /********************************************************************************
 **函数名称：fly_init_device_struct（）函数
 **函数功能：初始化结构体里的成员
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
	UINT32 i,j;
	
	//为 flytpms_struct_info 结构体分配内存
	pFlyTpmsInfo =
		(struct flytpms_struct_info *)malloc(sizeof(struct flytpms_struct_info));
	if (pFlyTpmsInfo == NULL)
	{
		return;
	}
	memset(pFlyTpmsInfo, 0, sizeof(struct flytpms_struct_info));
		
	pthread_mutex_init(&pFlyTpmsInfo->TpmsMainMutex, NULL);
	pthread_cond_init(&pFlyTpmsInfo->TpmsMainCond, NULL);
	allInOneInit();
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTpmsFromSerial = msgReadTpmsFromSerial;
	debugOneData("tpms msgReadTpmsFromSerial addr:", (UINT32)msgReadTpmsFromSerial);
		
	DBG0(debugString("\nFlyTpms hal init\n");)
	DBG0(debugString(__TIME__);)
	DBG0(debugString(__DATE__);)
	DBG0(debugString(" \n");)
 }
 
  /********************************************************************************
 **函数名称：flydvd_read()函数
 **函数功能：读出数据
 **函数参数：
 **返 回 值：成功返回实际读得的数据，失败返回-1
 **********************************************************************************/
 INT flyReadData(BYTE *buf, UINT len)
 {
	 UINT16 dwRead;
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nTPMS-HAL return  bytes to User:", buf,dwRead);)
	 return dwRead;
 }
 
 /********************************************************************************
 **函数名称：fly_destroy_struct()
 **函数功能：释放内存
 **函数参数：
 **返 回 值：无
 **********************************************************************************/
void flyDestroyDeviceStruct(void)
{		
	//释放一个条件变量
	pthread_cond_destroy(&pFlyTpmsInfo->TpmsMainCond);
	allInOneDeinit();
	
	free (pFlyTpmsInfo);
	pFlyTpmsInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	return 0;
 }

 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
	DBG1(debugBuf("\nUser write bytes to TPMS-HAL:", buf,len);)
	onRightDataProcessor(pFlyTpmsInfo,&buf[3], buf[2]-1);
}