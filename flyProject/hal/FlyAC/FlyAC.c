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
#include <cutils/properties.h>    

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_AC
#define LOCAL_HAL_NAME		"flyAC Stub"
#define LOCAL_HAL_AUTOHR	"FlyAC"

#include "FlyAC.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"

struct flyac_struct_info *pFlyACInfo = NULL;

BYTE Ltemp_H,Ltemp_L,Rtemp_H,Rtemp_L;
BYTE Speed,AC,Auto,ModeUp,ModeFront,ModeDown,Max,Rear,Vane,Off,Dual,LeftHeat,RightHeat;
BYTE AC_MAX,Rear_Lock;
BYTE AcMode;

static void DealTuguanAcData(BYTE *buf);
static void RespondACStatue(void);

void readFromhardwareProc(BYTE *buf,UINT length)
{   
	DBG0(debugBuf("ACHAL read from hardware",buf,length);)
	if (0x20 == buf[0])
	{
		if(0x04 == buf[1])
		{
			DealTuguanAcData(&buf[4]);
//			PostSignal(&pFlyACInfo->MainThreadMutex,&pFlyACInfo->MainThreadCond,&pFlyACInfo->bMainThreadRunAgain);
		}
	}
	
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	switch (sourceEvent)
	{
	case EVENT_AUTO_CLR_RESUME_ID:
		RespondACStatue();
		break;
	default:
		break;
	}

	PostSignal(&pFlyACInfo->MainThreadMutex,&pFlyACInfo->MainThreadCond,&pFlyACInfo->bMainThreadRunAgain);
}

/********************************************************************************
**函数名称：
**函数功能：返回数据给用户
**函数参数：
**返 回 值：
**********************************************************************************/
static void flyACReturnToUser(BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nAC-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nAC-HAL write  bytes to User Error:", buf,len);)
	}
}
/******************************************************************************/
/*                            返回给用户的各种信息                        */
/******************************************************************************/
void returnFACPowerMode(BYTE bPower)
{
	BYTE buff[] = {0x01,0x00};
	buff[1] = bPower;

	flyACReturnToUser(buff,2);
}

void returnFACbInit(BYTE bInit)
{
	BYTE buff[] = {0x02,0x00};
	buff[1] = bInit;

	flyACReturnToUser(buff,2);
}

/******************************************************************************/
/*                              控制驱动的各种信息                          */
/******************************************************************************/
static void FACWriteFile(BYTE *p,UINT length)
{
	//BYTE buff[256];
	//buff[0] = CURRENT_SHARE_MEMORY_ID;
	//buff[1] = 0x10;
	//buff[2] = 0x01;
	//writeDataToHardware(buff, 3);

	//DBG0(debugString("\nFlyAC Write");)
}

static void ReturnLTempToWince(BYTE ltemp_H,BYTE ltemp_L)
{
	BYTE buff[3] = {0x10};

	buff[1] = ltemp_H;
	buff[2] = ltemp_L;

	flyACReturnToUser(buff,3);	
}

static void ReturnRTempToWince(BYTE rtemp_H,BYTE rtemp_L)
{
	BYTE buff[3] = {0x11};

	buff[1] = rtemp_H;
	buff[2] = rtemp_L;

	flyACReturnToUser(buff,3);	
}

static void ReturnSpeedToWince(BYTE speed)
{
	BYTE buff[2] = {0x12};

	buff[1] = speed;

	flyACReturnToUser(buff,2);		
}
static void ReturnAcToWince(BYTE ac)
{
	BYTE buff[2] = {0x13};

	buff[1] = ac;

	flyACReturnToUser(buff,2);		
}
static void ReturnAutoToWince(BYTE pauto)
{
	BYTE buff[2] = {0x14};

	buff[1] = pauto;

	flyACReturnToUser(buff,2);		
}
static void ReturnModeUpToWince(BYTE modeup)
{
	BYTE buff[2] = {0x15};

	buff[1] = modeup;

	flyACReturnToUser(buff,2);		
}
static void ReturnModeFrontToWince(BYTE modefront)
{
	BYTE buff[2] = {0x16};

	buff[1] = modefront;

	flyACReturnToUser(buff,2);		
}

static void ReturnModeDownToWince(BYTE modedown)
{
	BYTE buff[2] = {0x17};

	buff[1] = modedown;

	flyACReturnToUser(buff,2);		
}

static void ReturnMaxToWince(BYTE max)
{
	BYTE buff[2] = {0x18};

	buff[1] = max;

	flyACReturnToUser(buff,2);		
}

static void ReturnRearToWince(BYTE rear)
{
	BYTE buff[2] = {0x19};

	buff[1] = rear;

	flyACReturnToUser(buff,2);		
}
static void ReturnVaneToWince(BYTE vane)
{
	BYTE buff[2] = {0x1a};

	buff[1] = vane;

	flyACReturnToUser(buff,2);		
}

static void ReturnOffToWince(BYTE off)
{
	BYTE buff[2] = {0x1b};

	buff[1] = off;

	flyACReturnToUser(buff,2);		
}

static void ReturnDualToWince(BYTE dual)
{
	BYTE buff[2] = {0x1c};

	buff[1] = dual;

	flyACReturnToUser(buff,2);		

}

static void ReturnleftHeatToWince(BYTE leftHeat)
{
	BYTE buff[2] = {0x1d};

	buff[1] = leftHeat;

	flyACReturnToUser(buff,2);		
}
static void ReturnRightHeatToWince(BYTE rightHeat)
{
	BYTE buff[2] = {0x1e};

	buff[1] = rightHeat;

	flyACReturnToUser(buff,2);		
}
static void ReturnAcMax(BYTE acMax)
{
	BYTE buff[2] = {0x1f};

	buff[1] = acMax;

	flyACReturnToUser(buff,2);	
}
static void ReturnRearLock(BYTE rearLock)
{
	BYTE buff[2] = {0x20};

	buff[1] = rearLock;

	flyACReturnToUser(buff,2);	
}
static void ReturnAcMode(BYTE acMode)
{
	BYTE buff[2] = {0x21};

	buff[1] = acMode;

	flyACReturnToUser(buff,2);	
}

static void DealTuguanAcData(BYTE *buf)
{
	switch(buf[0])
	{
	case 0x5b://left temperature
		if(Ltemp_L != (BYTE)(buf[1]*10))
		{
			Ltemp_L = buf[1]*10;
			Ltemp_H =(buf[1]*10)>>8;
			ReturnLTempToWince(Ltemp_H,Ltemp_L);		
		}
		break;
	case 0x61://right temperature
		if(Rtemp_L != (BYTE)(buf[1]*10))
		{
			Rtemp_L = buf[1]*10;
			Rtemp_H =(buf[1]*10)>>8;
			ReturnRTempToWince(Rtemp_H,Rtemp_L);
		}
		break;
	case 0x6e://max
		if(Max != (buf[1] & 0x01))
		{
			Max = buf[1] & 0x01;
			ReturnMaxToWince(Max);
		}
		break;
	case 0x71://ac
		if(AC != (buf[1]&0x01))
		{
			AC = buf[1] & 0x01;
			ReturnAcToWince(AC);
		}
		break;	
	case 0x5d://auto
	case 0x63:
		if (0 == memcmp((const void *)pFlyAllInOneInfo->pMemory_Share_Common->sCarModule,"VW_TIGUAN",sizeof("VW_TIGUAN")))
		{
			if((buf[1]&0xf0) == 0)
			{
				if(Auto != 0)
				{
					Auto =0;
					ReturnAutoToWince(0);
				}
			}
			else
			{
				if(buf[1] & 0x10)
				{
					if(Auto != 1)
					{
						Auto = 1;
						ReturnAutoToWince(1);
					}
				}
				else
				{
					if(Auto != 2)
					{
						Auto =2;
						ReturnAutoToWince(2);					
					}
				}
			}
		}
		break;
	case 0x55://dual
		if(( buf[1] ==0 )&&(buf[2] == 0))
		{
			if(Dual != 1)
			{
				Dual =1;
				ReturnDualToWince(1);
			}
		}
		else
		{
			if(Dual != 0)
			{
				Dual =0;
				ReturnDualToWince(0);
			}
		}
		break;
	case 0x5e://mode
		if(buf[1] == 0xC)
		{
			if(ModeUp != 1)
			{
				ModeUp = 1;
				ReturnModeUpToWince(1);
			}
		}
		else
		{
			if(ModeUp != 0)
			{
				ModeUp = 0;
				ReturnModeUpToWince(0);
			}
		}
		if(buf[2] == 0xC)
		{
			if(ModeFront != 1)
			{
				ModeFront = 1;
				ReturnModeFrontToWince(1);
			}
		}
		else
		{
			if(ModeFront != 0)
			{
				ModeFront = 0;
				ReturnModeFrontToWince(0);
			}
		}
		if(buf[3] == 0xC)
		{
			if(ModeDown != 1)
			{
				ModeDown = 1;
				ReturnModeDownToWince(1);
			}
		}
		else
		{
			if(ModeDown != 0)
			{
				ModeDown = 0;
				ReturnModeDownToWince(0);
			}
		}
		if (0 == memcmp((const void *)pFlyAllInOneInfo->pMemory_Share_Common->sCarModule,"VW_MAGOTAN",sizeof("VW_MAGOTAN")))
		{
			if(buf[4] == 0x03)
			{
				if(Auto !=1)
				{
					Auto =1;
					ReturnAutoToWince(Auto);	
				}
			}
			else
			{
				if(Auto !=0)
				{
					Auto =0;
					ReturnAutoToWince(Auto);	
				}
			}
		}
		break;
	case 0x5c://speed
	case 0x62:
		if(Speed != buf[1])
		{
			Speed = buf[1];
			ReturnSpeedToWince(buf[1]);
		}
		break;
	case 0x50://vane
		if(Vane != (buf[1]&1))
		{
			Vane = (buf[1]&0x01);
			ReturnVaneToWince(Vane);
		}
		break;
	case 0x51://vane
		if(buf[1])
		{
			if(Vane != 1)
			{
				Vane =1;
				ReturnVaneToWince(1);
			}
		}
		else
		{
			if(Vane != 0)
			{
				Vane =0;
				ReturnVaneToWince(0);
			}

		}
		break;
	case 0x58://rear
		if(Rear != (buf[1]&0x1))
		{
			Rear = buf[1]&0x1;
			ReturnRearToWince(Rear);
		}
		break;
	case 0x4d://off
		if(buf[1] & 0x10)
		{
			if(Off != 0)
			{
				Off =0;
				ReturnOffToWince(0);

			}
		}
		else
		{
			if(Off != 1)
			{
				Off =1;
				ReturnOffToWince(1);
			}
		}
		break;
	case 0x56:
		break;
	case 0x5f://左座椅加热
		if(LeftHeat!= buf[1])
		{
			LeftHeat =buf[1];
			ReturnleftHeatToWince(buf[1]);
		}
		break;
	case 0x65://右座椅加热
		if(RightHeat!= buf[1])
		{
			RightHeat =buf[1];
			ReturnRightHeatToWince(buf[1]);
		}
		break;
	case 0x67:
		if (0 == memcmp((const void *)pFlyAllInOneInfo->pMemory_Share_Common->sCarModule,"VW_MAGOTAN",sizeof("VW_MAGOTAN")))
		{
			if(buf[1] ==0)
			{
				if(Rear_Lock != 1)
				{
					Rear_Lock =1;
					ReturnRearLock(Rear_Lock);
				}
			}
			else
			{
				if(Rear_Lock != 0)
				{
					Rear_Lock =0;
					ReturnRearLock(Rear_Lock);
				}
			}
		}
		break;	
	case 0x75:
		if (0 == memcmp((const void *)pFlyAllInOneInfo->pMemory_Share_Common->sCarModule,"VW_MAGOTAN",sizeof("VW_MAGOTAN")))
		{
			if(AC_MAX != buf[1])
			{
				AC_MAX = buf[1];
				ReturnAcMax(AC_MAX);
			}
		}
		break;
	default:
		break;
	}

}

/******************************************************************************/
/*                                各种线程函数                            */
/******************************************************************************/
void *ac_main_thread(void *arg) 
{
	INT ret = 0;
//	ULONG waitTime = 1000;
	ULONG waitTime = 0;
	debugString("\r\nFlyAC ThreadMain Start");

	while (!pFlyACInfo->bKillFlyMainThread)
	{
		WaitSignedTimeOut(&pFlyACInfo->MainThreadMutex,&pFlyACInfo->MainThreadCond,&pFlyACInfo->bMainThreadRunAgain,waitTime);
		waitTime = 0;
//		RespondACStatue();
		if(pFlyACInfo->bPower)
		{
		//	returnFACPowerMode(TRUE);
		}
	}
	pFlyACInfo->bKillFlyMainThread = TRUE;
	debugString("\r\nFlyAC ThreadMain exit");
	return 0;
}

static int creatThread(void)
{
	INT res;
	pthread_t thread_id;
		
	pFlyACInfo->bKillFlyMainThread = FALSE;
	res = pthread_create(&thread_id,NULL,ac_main_thread,NULL);
	if(res != 0) 
	{
		debugString("ac_main_thread error");
		return -1;
	}
	pFlyACInfo->bPowerUp = TRUE;
	return 0;
}
/******************************************************************************/
/*                                  处理数据                              */
/******************************************************************************/
static void RespondACStatue(void)
{
	//DBG0(debugString("\nFlyAC write what data to user \n");)
	ReturnRTempToWince(Rtemp_H,Rtemp_L);
	ReturnLTempToWince(Ltemp_H,Ltemp_L);	
	ReturnMaxToWince(Max);	
	ReturnAcToWince(AC);
	ReturnAutoToWince(Auto);
	ReturnDualToWince(Dual);
	ReturnModeUpToWince(ModeUp);
	ReturnModeFrontToWince(ModeFront);
	ReturnModeDownToWince(ModeDown);
	ReturnSpeedToWince(Speed);
	ReturnVaneToWince(Vane);
	ReturnRearToWince(Rear);
	ReturnOffToWince(Off);
	ReturnleftHeatToWince(LeftHeat);
	ReturnRightHeatToWince(RightHeat);
	ReturnAcMax(AC_MAX);
	ReturnRearLock(Rear_Lock);			
	ReturnAcMode(AcMode);
}
static void DealRightDataProcessor(BYTE *buff, UINT16 len)
{
	switch (buff[0])
	{
	case 0x01:
		if(0x01 == buff[1])
		{
			pFlyACInfo->bPower = TRUE;
			returnFACPowerMode(TRUE);
			returnFACbInit(TRUE);
			RespondACStatue();
			
			if (!pFlyACInfo->bPowerUp)
			{
				//creatThread();
			}
			PostSignal(&pFlyACInfo->MainThreadMutex,&pFlyACInfo->MainThreadCond,&pFlyACInfo->bMainThreadRunAgain);
//			DBG0(debugString("\nFlyAC hal init_cammand OK\n");)
		}
		else
		{
			returnFACPowerMode(FALSE);
			pFlyACInfo->bPower = FALSE;
		}
		break;
	case 47://途观的数据
		break;
	default:
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
	creatThread();
	pFlyACInfo->bOpen = TRUE;
	return HAL_AC_RETURN_FD;
}

/********************************************************************************
**函数名称：fly_init_device_struct（）函数
**函数功能：初始化结构体里的成员
**函数参数：
**返 回 值：
**********************************************************************************/
void flyInitDeviceStruct(void)
{

	//为 flyac_struct_info 结构体分配内存
	pFlyACInfo =
		(struct flyac_struct_info *)malloc(sizeof(struct flyac_struct_info));
	if (pFlyACInfo == NULL)
	{
		//debugString("\r\npFlyACInfo malloc error");
		return;
	}
	memset(pFlyACInfo, 0, sizeof(struct flyac_struct_info));

	pthread_mutex_init(&pFlyACInfo->MainThreadMutex, NULL);
	pthread_cond_init(&pFlyACInfo->MainThreadCond, NULL);

	allInOneInit();
//	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTpmsFromSerial = readFromhardwareProc;
	DBG0(debugString("\nFlyAC hal init\n");)
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
	UINT16 i;
	DBG1(debugBuf("\nAC-HAL return  bytes Start:", buf,1);)
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
	//各种线程退出
	//pFlyACInfo->bKillFlyMainThread = TRUE;

	//释放各种条件变量
	pthread_cond_destroy(&pFlyACInfo->MainThreadCond);

	allInOneDeinit();
	free (pFlyACInfo);
	pFlyACInfo = NULL;
}
/********************************************************************************
**函数名称：fly_close_device()函数
**函数功能：关闭函数
**函数参数：
**返 回 值：
**********************************************************************************/
INT flyCloseDevice(void)
{
	debugString("close device !");
//	pFlyACInfo->bKillFlyMainThread = TRUE;

	return TRUE;
}

/********************************************************************************
**函数名称：
**函数功能：
**函数参数：
**返 回 值：
**********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
//	DBG1(debugBuf("\nAC-HAL RecJNI:",buf,len);)
	DealRightDataProcessor(&buf[3], buf[2]-1);
}