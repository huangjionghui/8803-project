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

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_TV
#define LOCAL_HAL_NAME		"flytv Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_TV

#include "FlyTv.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"


struct flytv_struct_info *pFlyTvInfo = NULL;
static void control_TVChange(struct flytv_struct_info *pFlyTvInfo);

static void FlyTV_infoProcessor(struct flytv_struct_info *pFlyTvInfo, BYTE dataOne, BYTE dataTwo)
{
	
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	//DBG0(debugBuf("AudioHAL read from hardware",buf,length);)
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	switch (sourceEvent)
	{
		case EVENT_GLOBAL_EXBOX_INPUT_CHANGE_ID:
			if (pFlyAllInOneInfo->pMemory_Share_Common->eAudioInput == TV)
			{
				DBG0(debugString("\nTV xxxxxxxxxxxxxxxx");)
				if (pFlyTvInfo)
					control_TVChange(pFlyTvInfo);
			}
			break;
			
		default:
			pFlyTvInfo->oldAudioInput = 0xFF;
			break;
	}
}

void msgReadTvFromSerial(BYTE msgQueueID,BYTE *pData,UINT length)
{
	DBG3(debugOneData("\nTV-HAL msgQueue msgID ", msgQueueID);)
	DBG3(debugBuf("\nTV-HAL read msgQueue data:",pData,length);)
	
	if (length > 0)
	{
		FlyTV_infoProcessor(pFlyTvInfo, pData[0], pData[1]);	
	}
}

static void flyAudioReturnToUser(struct flytv_struct_info *pFlyTvInfo,BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nTV-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nTV-HAL write  bytes to User Error:", buf,len);)
	}
}

static void returnPowerStatus(struct flytv_struct_info *pFlyTvInfo,BOOL bPower)
{
	BYTE buff[] = {0x01,0x00};//0x00:�� 0x01:��
	if (bPower)
	{
		buff[1] = 0x01;
	} 
	
	flyAudioReturnToUser(pFlyTvInfo,buff,2);
}
static void returnWorkMode(struct flytv_struct_info *pFlyTvInfo,BOOL bWork)
{
	BYTE buff[2]={0x02,0x00};
	if (bWork)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyTvInfo,buff,2);
}
static void returnTVConnectStatus(struct flytv_struct_info *pFlyTvInfo,BOOL bHave)
{
	return;//��System����
	BYTE buff[] = {0x03,0x00};
	if (bHave)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyTvInfo,buff,2);
}

static void TVWriteFile(struct flytv_struct_info *pFlyTvInfo,BYTE *p,UINT length)
{
	//msgQueueWrite(SHARE_MEMORY_TV,SHARE_MEMORY_COMMON,p,length);
	if (pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial)
		pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial(SHARE_MEMORY_TV,p,length);
}
static void TVCmdPrintf(struct flytv_struct_info *pFlyTvInfo,BYTE *buf,UINT len)
{
	BYTE sendBuf[2];
	sendBuf[0] = buf[0];
	sendBuf[1] = buf[1];
	TVWriteFile(pFlyTvInfo,sendBuf,2);
}
static void control_TVCmd(struct flytv_struct_info *pFlyTvInfo,BYTE dataOne,BYTE dataTwo)
{
	BYTE buff[2];
	buff[0] = dataOne;
	buff[1] = dataTwo;
	TVCmdPrintf(pFlyTvInfo,buff,2);
}

static void control_TVChange(struct flytv_struct_info *pFlyTvInfo)
{
	BYTE buf_one[2]={0xF1,0x03};
	TVWriteFile(pFlyTvInfo,buf_one,2);
	
	BYTE buf_two[2]={0xF2,0x00};
	TVWriteFile(pFlyTvInfo,buf_one,2);
	
}

void *ThreadFlyTVProc(void *arg)
{
	int ret;
	struct timeval timenow;
	struct timespec timeout;
	struct flytv_struct_info *pFlyTvInfo = (struct flytv_struct_info *)arg;
	
	while (!pFlyTvInfo->bKillTvMainThread)
	{
		WaitSignedTimeOut(&pFlyTvInfo->TvMainMutex,&pFlyTvInfo->TvMainCond,&pFlyTvInfo->bTvMainThreadRunAgain,1000);

		if (pFlyTvInfo->bPower)
		{
			if (pFlyTvInfo->bHavaTV != pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule)
			{
				pFlyTvInfo->bHavaTV = pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule;
				returnTVConnectStatus(pFlyTvInfo,pFlyTvInfo->bHavaTV);
			}
		}
	}

	pFlyTvInfo->bKillTvMainThread = TRUE;
	DBG1(debugString("\nFlyAudio ThreadFlyTVProc exit");)
	return NULL;
}


static int createThread(struct flytv_struct_info *pFlyTvInfo)
{
	INT res;
	pthread_t thread_id;
	 
	pFlyTvInfo->bKillTvMainThread = FALSE;
	res = pthread_create(&thread_id, NULL, ThreadFlyTVProc,pFlyTvInfo);
	if(res != 0) 
	{
		return -1;
	}
	
	pFlyTvInfo->bPowerUp = TRUE;
	
	return 0;
}

static void onRightDataProcessor(struct flytv_struct_info *pFlyTvInfo,BYTE *buf, UINT16 len)
{
	BYTE tvcmd;
	 switch (buf[0])
	 {
		case 0x01://��ʼ������
			if (0x01 == buf[1])
			{
				returnPowerStatus(pFlyTvInfo,TRUE);
				returnWorkMode(pFlyTvInfo,TRUE);
				pFlyTvInfo->bPower = TRUE;
				
				if (!pFlyTvInfo->bPowerUp)
				{
					createThread(pFlyTvInfo);
				}
				PostSignal(&pFlyTvInfo->TvMainMutex,&pFlyTvInfo->TvMainCond,&pFlyTvInfo->bTvMainThreadRunAgain);
			}
			else if (0x00 == buf[1])
			{
				pFlyTvInfo->bPower = FALSE;
				returnPowerStatus(pFlyTvInfo,FALSE);
				returnWorkMode(pFlyTvInfo,FALSE);
			}	
			break;
			
		case 0x03://
			if (buf[1] == 0x17) 
			{
				tvcmd = 0x50;
			}
			else 
			{
				tvcmd = buf[1];
			}
			
			control_TVCmd(pFlyTvInfo,0xb4,tvcmd);
			break;
			
		case 0xff://�����ã�����ʹ�ò����
			if (0x01 == buf[1])
			{
				
			} 
			else if (0x00 == buf[1])
			{
				
			}
			break;
	 
		default:
			break;
	}
}


 
 /*==========================����Ϊ��������====================================*/
 /******************************************************************************/
 /******************************************************************************/
 /******************************************************************************/
 /*============================================================================*/
 
/********************************************************************************
 **�������ƣ�fly_open_device��������
 **�������ܣ����豸
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 INT flyOpenDevice(void)
 {
	INT ret = HAL_ERROR_RETURN_FD;
	
	pFlyTvInfo->bOpen = TRUE;
	ret = HAL_TV_RETURN_FD;	
	return ret;
 }
 
 /********************************************************************************
 **�������ƣ�fly_init_device_struct��������
 **�������ܣ���ʼ���ṹ����ĳ�Ա
 **����������
 **�� �� ֵ��
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
	UINT32 i,j;
	
	//Ϊ flytv_struct_info �ṹ������ڴ�
	pFlyTvInfo =
		(struct flytv_struct_info *)malloc(sizeof(struct flytv_struct_info));
	if (pFlyTvInfo == NULL)
	{
		//DBG0(debugString("\npFlyTvInfo malloc error");)
		return;
	}
	memset(pFlyTvInfo, 0, sizeof(struct flytv_struct_info));
		
	pthread_mutex_init(&pFlyTvInfo->TvMainMutex, NULL);
	pthread_cond_init(&pFlyTvInfo->TvMainCond, NULL);
	allInOneInit();
	pFlyTvInfo->oldAudioInput = 0xFF;
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTvFromSerial = msgReadTvFromSerial;
	debugOneData("tpms msgReadTvFromSerial addr:", (UINT32)msgReadTvFromSerial);
		
	DBG0(debugString("\nFlyTv hal init\n");)
	DBG0(debugString(__TIME__);)
	DBG0(debugString(__DATE__);)
	DBG0(debugString(" \n");)
 }
 
  /********************************************************************************
 **�������ƣ�flydvd_read()����
 **�������ܣ���������
 **����������
 **�� �� ֵ���ɹ�����ʵ�ʶ��õ����ݣ�ʧ�ܷ���-1
 **********************************************************************************/
 INT flyReadData(BYTE *buf, UINT len)
 {
	 UINT16 dwRead;
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nXXX-HAL return  bytes to User:", buf,dwRead);)
	 return dwRead;
 }
 
 /********************************************************************************
 **�������ƣ�fly_destroy_struct()
 **�������ܣ��ͷ��ڴ�
 **����������
 **�� �� ֵ����
 **********************************************************************************/
void flyDestroyDeviceStruct(void)
{	
	//�ͷ�һ����������
	pthread_cond_destroy(&pFlyTvInfo->TvMainCond);
	
	allInOneDeinit();
	free (pFlyTvInfo);
	pFlyTvInfo = NULL;
}
 /********************************************************************************
 **�������ƣ�fly_close_device()����
 **�������ܣ��رպ���
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	return 0;
 }

 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
	DBG1(debugBuf("\nUser write bytes to TV-HAL:", buf,len);)
	onRightDataProcessor(pFlyTvInfo,&buf[3], buf[2]-1);
}