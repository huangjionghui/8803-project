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

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_EXDISPLAY
#define LOCAL_HAL_NAME		"flyExdisplay Stub"
#define LOCAL_HAL_AUTOHR	"FlyExdisplay"

#include "FlyExdisplay.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"

struct flyexdisplay_struct_info *pFlyExdisplayInfo = NULL;

static void buffChangeInput(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BYTE eInput);
static void ExDisplayWriteFile(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo, BYTE *p , UINT length);

void readFromhardwareProc(BYTE *buf,UINT length)
{
	//DBG0(debugBuf("ExDisplayHAL read from hardware",buf,length);)
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	DBG0(debugOneData("\n FlyExdisplay resume current mode ----------------->",sourceEvent);)
	DLOGI("FlyExdisplay resume current mode -----------------> :%x", sourceEvent);
	switch(sourceEvent)
	{
		case EVENT_AUTO_CLR_RESUME_ID:
			buffChangeInput(pFlyExdisplayInfo,pFlyExdisplayInfo->ExCurrentState);
			if(pFlyExdisplayInfo->ExCurrentState == eAD_MP3) //MP3情况 当做外部软件另行处理
			{
				BYTE buf[2] = {0x11,0x06};
				ExDisplayWriteFile(pFlyExdisplayInfo, &buf[0] , 2);
				DLOGI("FlyExdisplay MP3_resume mode -----------------> %x,%x", buf[0],buf[1]);
			}
			DLOGI("FlyExdisplay resume current mode -----------------> %x", pFlyExdisplayInfo->ExCurrentState);
		break;
		default:
		
		break;
	}
}

 static void FlyReturnToUser(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo , BYTE *buf, UINT16 len)
 {
	 UINT dwLength;

	 dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 if (dwLength)
	 {
		 DBG0(debugBuf("\nExDisplay-HAL write  bytes to User OK:", buf,len);)
	 }
	 else
	 {
		 DBG0(debugBuf("\nExDisplay-HAL write  bytes to User Error:", buf,len);)
	 }
 }
 
void returnExDisplayPowerMode(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BOOL bPower)
{
	BYTE buf[] = {0x01,0x00};

	if (bPower)
	{
		buf[1] = 0x01;
	}

	FlyReturnToUser(pFlyExdisplayInfo,buf,2);
}

void returnExDisplayWorkMode(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BOOL bWork)
{
	BYTE buf[] = {0x02,0x00};

	if (bWork)
	{
		buf[1] = 0x01;
	}

	FlyReturnToUser(pFlyExdisplayInfo,buf,2);
}

void returnExDisplaybHave(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BOOL bHave)
{
	BYTE buf[] = {0x10,0x00};

	if (bHave)
	{
		buf[1] = 0x01;
	}

	FlyReturnToUser(pFlyExdisplayInfo,buf,2);
}

void returnExplsyCurrentMode(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BYTE mode)
{
	
}

static void buffSendToBuff(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BYTE ID,BYTE *pData,UINT len)
{
	memcpy(pFlyExdisplayInfo->iSendToMCUBuff[ID],pData,len);
	pFlyExdisplayInfo->iSendToMCULength[ID] = len;
}

static void ExDisplayWriteFile(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo, BYTE *p , UINT length)
{
	BYTE buff[256];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	memcpy(&buff[1],p,length);
	writeDataToHardware(buff, length + 1);
	DLOGI(" \n FlyExdisplay write to hardware mode  ------->:%x , %x , %x, %x", buff[1],buff[2],buff[3],buff[4]);
}

/* static void ExDisplayWriteFile(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BYTE *p,UINT length)
{
	DBG0(debugBuf("\n*********Exdisplay write:", p, length);)
	//msgQueueWrite(CURRENT_SHARE_MEMORY_ID,SHARE_MEMORY_COMMON,p,length);
	if (pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial)
		pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial(CURRENT_SHARE_MEMORY_ID,p,length);
} */

static void buffSendToMCU(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BYTE ID)
{
	if (pFlyExdisplayInfo->iSendToMCULength[ID])
	{
		ExDisplayWriteFile(pFlyExdisplayInfo,pFlyExdisplayInfo->iSendToMCUBuff[ID],pFlyExdisplayInfo->iSendToMCULength[ID]);
	//	DLOGI(" \n FlyExdisplay write to hardware mode from buffSendToMCU-------->:%x , %x , %x, %x", 
	//	pFlyExdisplayInfo->iSendToMCUBuff[ID],pFlyExdisplayInfo->iSendToMCUBuff[ID+1],pFlyExdisplayInfo->iSendToMCUBuff[ID+2],pFlyExdisplayInfo->iSendToMCUBuff[ID+3]);
	}
}

static void buffChangeInput(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo,BYTE eInput)
{
	if (eAD_DVD == eInput)
	{
		buffSendToMCU(pFlyExdisplayInfo,0x20);
		buffSendToMCU(pFlyExdisplayInfo,0x21);
		buffSendToMCU(pFlyExdisplayInfo,0x22);
		buffSendToMCU(pFlyExdisplayInfo,0x23);
		buffSendToMCU(pFlyExdisplayInfo,0x24);
	}
	else if (eAD_Radio == eInput)
	{
		buffSendToMCU(pFlyExdisplayInfo,0x30);
		buffSendToMCU(pFlyExdisplayInfo,0x31);
		buffSendToMCU(pFlyExdisplayInfo,0x32);
		buffSendToMCU(pFlyExdisplayInfo,0x33);
	}
	else if(eAD_iPod == eInput)
	{
		buffSendToMCU(pFlyExdisplayInfo,0x40);
		buffSendToMCU(pFlyExdisplayInfo,0x41);
		buffSendToMCU(pFlyExdisplayInfo,0x42);
	}
	else if(eAD_AUX == eInput)
	{


	}
	else if(eAD_MP3 == eInput)
	{
		buffSendToMCU(pFlyExdisplayInfo,0x50);
		buffSendToMCU(pFlyExdisplayInfo,0x51);
		buffSendToMCU(pFlyExdisplayInfo,0x52);
	}
	 else if(eAD_XM == eInput)
	 {
		 buffSendToMCU(pFlyExdisplayInfo,0x60);
		 buffSendToMCU(pFlyExdisplayInfo,0x61);
		 buffSendToMCU(pFlyExdisplayInfo,0x62);
	 }
	else if(eAD_BT == eInput)
	{
		buffSendToMCU(pFlyExdisplayInfo,0x70);
		buffSendToMCU(pFlyExdisplayInfo,0x71);
	}

}

void *debugThread(void *arg)
{
	UINT i=0;
	UINT32 WaitReturn;
	struct flyexdisplay_struct_info *pFlyExdisplayInfo = (struct flyexdisplay_struct_info*)arg;
	
	//pFlyExdisplayInfo->bPowerUp = TRUE;
	DBG0(debugString("\n FlyExdisplayMainThread running ");)
	while (!pFlyExdisplayInfo->bKillExdisplayMainThread)
	{
		WaitReturn = WaitSignedTimeOut(&pFlyExdisplayInfo->ExdisplayMainMutex,&pFlyExdisplayInfo->ExdisplayMainCond,&pFlyExdisplayInfo->ExdisplayMainThreadRunAgain,1000);
	/*
		for (i=0; i < 10; i++)
		{
			flyAudioReturnToUserPutToBuff(0xAC);
		}
		*/
		sleep(200);
		//sem_post(&pFlyExdisplayInfo->userReadSem);
		
	}
	DBG0(debugString("FlyExdisplayMainThreadProc exit\n");)
	return NULL;
}

 static BOOL createThread(P_FLY_EXDISPLAY_INFO pFlyExdisplayInfo)
 {
	INT res;
	pthread_t thread_id;
	pFlyExdisplayInfo->bKillExdisplayMainThread = FALSE;

	//线程调试用，
	res = pthread_create(&thread_id, NULL, debugThread,pFlyExdisplayInfo);
    if(res != 0) 
	{
		return FALSE;
    }
	pFlyExdisplayInfo->bPowerUp = TRUE;
	return TRUE;
 }
 
 static void checkTheCommand(BYTE *buff,UINT len)
{
	switch (buff[0])
	{
	case 0x01:
		if (0x01 == buff[1])
		{
			returnExDisplayPowerMode(pFlyExdisplayInfo,TRUE);
			returnExDisplayWorkMode(pFlyExdisplayInfo,TRUE);
			returnExDisplaybHave(pFlyExdisplayInfo,TRUE);
			pFlyExdisplayInfo->bPower = TRUE;
			if (!pFlyExdisplayInfo->bPowerUp)
			{
				//createThread(pFlyExdisplayInfo);
			}
			DBG0(debugString("\n exdisplay init ok");)
			PostSignal(&pFlyExdisplayInfo->ExdisplayMainMutex,&pFlyExdisplayInfo->ExdisplayMainCond,&pFlyExdisplayInfo->ExdisplayMainThreadRunAgain);
		}
		else
		{
			returnExDisplayPowerMode(pFlyExdisplayInfo,FALSE);
			pFlyExdisplayInfo->bPower = FALSE;
		}
		break;
	case 0x80:
	case 0x90:
		 buffSendToBuff(pFlyExdisplayInfo,buff[0],&buff[0],len);
		 ExDisplayWriteFile(pFlyExdisplayInfo,&buff[0],len);
	//	 DLOGI(" \n FlyExdisplay write to hardware mode ----------------->:%x , %x , %x, %x", buff[0],buff[1],buff[2],buff[3]);
		 break;
	case 0xFF:
	
		break;
	default:
		buffSendToBuff(pFlyExdisplayInfo,buff[0],&buff[0],len);
	//	ExDisplayWriteFile(pFlyExdisplayInfo,&buff[0],len);
		
	//	DLOGI(" \n FlyExdisplay write to hardware mode ----------------->:%x , %x , %x, %x", buff[0],buff[1],buff[2],buff[3]);
		if (0x11 == buff[0])
		{
			ExDisplayWriteFile(pFlyExdisplayInfo,&buff[0],len);
			buffChangeInput(pFlyExdisplayInfo,buff[1]);
			pFlyExdisplayInfo->ExCurrentState = buff[1];
			//DLOGI("FlyExdisplay write to hardware mode ----------------->:%x", buff[1]);
		//	DLOGI(" \n FlyExdisplay write to hardware mode buff[1]=0x11 ------->:%x , %x , %x, %x", buff[0],buff[1],buff[2],buff[3]);
		}
		else
		{
			buffChangeInput(pFlyExdisplayInfo,pFlyExdisplayInfo->ExCurrentState);
		}
		
		break;
	}
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
	pFlyExdisplayInfo->bOpen = TRUE;
	
	createThread(pFlyExdisplayInfo);
	//线程调试用
/*  	if (!createThread(pFlyExdisplayInfo))
	{
		DBG0(debugString("\n create read thread error");)
		return HAL_ERROR_RETURN_FD;
	} */
	 
	DBG0(debugString("\n exdisplay open ok");)
	return HAL_EXDISPLAY_RETURN_FD;
 }
 
 /********************************************************************************
 **函数名称：fly_init_device_struct（）函数
 **函数功能：初始化结构体里的成员
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
 
	// DBG0(debugString("\n begin  Exdisplay hal init\n");)
	//为 flyexdisplay_struct_info 结构体分配内存
	pFlyExdisplayInfo =
		(struct flyexdisplay_struct_info *)malloc(sizeof(struct flyexdisplay_struct_info));
	if (pFlyExdisplayInfo == NULL)
	{
		//DLOGE("pFlyExdisplayInfo malloc error");
		return;
	}
	memset(pFlyExdisplayInfo, 0, sizeof(struct flyexdisplay_struct_info));
	
	//初始化互斥锁各条件变量
	pthread_mutex_init(&pFlyExdisplayInfo->ExdisplayMainMutex, NULL);
	pthread_cond_init(&pFlyExdisplayInfo->ExdisplayMainCond, NULL);
	allInOneInit();
//	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTpmsFromSerial = msgReadTpmsFromSerial;
//	debugOneData("tpms msgReadExdisplayFromSerial addr:", (UINT32)msgReadTpmsFromSerial);
		
	DBG0(debugString("\n Exdisplay hal init\n");)
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
	pthread_cond_destroy(&pFlyExdisplayInfo->ExdisplayMainCond);
	allInOneDeinit();
	free (pFlyExdisplayInfo);
	pFlyExdisplayInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	
	//DLOGD("Fly exdisplay Close");
	return -1;
 }

 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
	DLOGI(" \n FlyExdisplay read_from JNI ----------------->:%x , %x , %x, %x", buf[3],buf[4],buf[5],buf[6]);
	checkTheCommand(&buf[3], len-3);
//	DLOGI(" %x", buf[4]);
//	DLOGI(" %x", buf[5]);
//	DLOGI(" %x", buf[6]);
//	DLOGI("FlyExdisplay write to hardware mode ----------------->:%x", buf[3]);
	
}
