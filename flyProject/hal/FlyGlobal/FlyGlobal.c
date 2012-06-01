#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>  

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_GLOBAL
#define LOCAL_HAL_NAME		"flyGlobal Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"

#define SERIAL_NAME			"/dev/tcc-uart5"    
#define SERIAL_BAUDRATE      38400     

#include "FlyGlobal.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"
#include "../../include/serial.c"
#include "FlyDebugMsg.h"

struct flyglobal_struct_info *pFlyGlobalInfo = NULL;//全局变量
void ipcEventProcProc(UINT32 sourceEvent)
{
	PostSignal(&pFlyGlobalInfo->MainThreadMutex,&pFlyGlobalInfo->MainThreadCond,&pFlyGlobalInfo->bMainThreadRunAgain);
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	DBG0(debugString("\n FlyGlobal readFromhardwareProc do nothing");)
}
//#ifdef LDH
/*********************************************************************************
**函数名称:		FlyReturnDataToUser
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
static void FlyReturnDataToUser(P_FLY_GLOBAL_INFO pGlobalInfo, BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nGlobal-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nGlobal-HAL write  bytes to User Error:", buf,len);)
	}
}
//#endif

//#ifdef LDH
/**********************************************************************************
**函数名称:		ReturnPowerStatus
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void ReturnPowerMode(P_FLY_GLOBAL_INFO pGlobalInfo,BYTE Power)
{
	BYTE Buff[] = {0X01,0X00};
	if(Power)
	{
		Buff[1] = 0X01;
	}
	else
	{
		Buff[1] = 0X00;
	}
	FlyReturnDataToUser(pGlobalInfo,Buff,2);
}
//#endif

//#ifdef LDH
/**********************************************************************************
**函数名称:		returnGlobalWorkMode
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void ReturnGlobalWorkMode(P_FLY_GLOBAL_INFO pGlobalInfo,BYTE WorkMode)
{
	BYTE Buff[] = {0X02,0X00};
	if(WorkMode)
	{
		Buff[1] = 0X01;
	}
	else
	{
		Buff[1] = 0X00;
	}
	FlyReturnDataToUser(pGlobalInfo,Buff,2);
}
//#endif

//#ifdef LDH
/**********************************************************************************
**函数名称:		ReturnGlobalMemoryEventWhat
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void ReturnGlobalMemoryEventWhat(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	BYTE Buff[] = {0X11,0X00,0X00};
	INT i;
	for (i = 0;i < GLOBAL_MEMORY_EVENT_SAVE;i++)
	{
		if (pGlobalInfo->iGlobalMemoryEventWhat[i])
		{
			Buff[1] = i;
			Buff[2] = pGlobalInfo->iGlobalMemoryEventWhat[i];
			pGlobalInfo->iGlobalMemoryEventWhat[i] = 0;
			FlyReturnDataToUser(pGlobalInfo,Buff,3);
		}
	}
	FlyReturnDataToUser(pGlobalInfo,Buff,3);
}
//#endif
//#ifdef LDH
/**********************************************************************************
**函数名称:		ReturnGlobalOSDDebug
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void ReturnGlobalOSDDebug(P_FLY_GLOBAL_INFO pGlobalInfo,UINT iLine,BYTE *p,UINT length)
{
	BYTE buff[1+1+OSD_DEBUG_WIDTH];
	buff[0] = 0x21;
	buff[1] = iLine;
	memcpy(&buff[2],p,length);
	FlyReturnDataToUser(pGlobalInfo,buff,1+1+length);
}
//#endif
//#ifdef LDH
/**********************************************************************************
**函数名称:		DealRightDataProcessor
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void DealRightDataProcessor(P_FLY_GLOBAL_INFO pGlobalInfo,BYTE *buf, UINT16 len)
{
	DBG0(debugString("\n Global-> DealRightDataProcessor: ");)
	DBG0(debugOneData(" ",buf[0]);)
	switch(buf[0])
	{
		case 0x01:	if(buf[1] == 1)
					{
						ReturnPowerMode(pGlobalInfo,1);
						ReturnGlobalWorkMode(pGlobalInfo,1);
					}
					else
					{
						ReturnPowerMode(pGlobalInfo,0);
					}
					break;
		case 0x10: 	if(buf[1] == 1)
					{
						pFlyAllInOneInfo->pMemory_Share_Common->bRecUserPingStart = TRUE;
						ipcStartEvent(EVENT_GLOBAL_USER_PING_START_ID);
					}
					else
					{
						ipcStartEvent(EVENT_GLOBAL_USER_PING_WORK_ID);
					}
					break;
		//case 0x11:	ReturnGlobalMemoryEventWhat(pGlobalInfo);break;//此协议未实现
	
		case 0x13:
			if (pGlobalInfo->fd > 0)
			{
				if (serial_write(pGlobalInfo->fd, &buf[1], len-1) < 0)
					debugString("global-hal write serial-5 err\n");
				else
					debugBuf("\n global-hal write serial-5 ok:\n", &buf[1], len-1);
			}
			break;
		
		case 0XFF: 	break;
		
		default:	DBG0(debugOneData("Global-> Unhandle Android Command!!",buf[0]);)
					break;
	}
}
//#endif

/**********************************************************************************
**函数名称:		ThreadMain
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void *ThreadMainProcess(void *arg)
{
	UINT i,j;
	UINT ret = 0;
	struct flyglobal_struct_info *pGlobalInfo = (struct flyglobal_struct_info *)arg;
	pGlobalInfo->bFlyGlobalThreadRunning = TRUE;

	pGlobalInfo->OldTick = GetTickCount();
	pGlobalInfo->PerTick = GetTickCount();
	while(!pGlobalInfo->bKillDispatchFlyGlobalMainThread)
	{
		//#ifdef LDH
		if(pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode)
		{
			//ret = waitSignedTimeOut(100);
			ret = WaitSignedTimeOut(&pGlobalInfo->MainThreadMutex,&pGlobalInfo->MainThreadCond,&pGlobalInfo->bMainThreadRunAgain,1500);
		}
		else
		{
			//ret = waitSignedTimeOut(3000);
			ret = WaitSignedTimeOut(&pGlobalInfo->MainThreadMutex,&pGlobalInfo->MainThreadCond,&pGlobalInfo->bMainThreadRunAgain,3000);
		}
		//DBG0(debugOneData("\n2012-3-3 14:26 ret = ",ret);)
	
		if (ipcWhatEventOn(EVENT_GLOBAL_DEMO_OSD_START_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_DEMO_OSD_START_ID);
			//DBG0(debugString("\r\nGlobal Demo Start");)
		}
		if (ipcWhatEventOn(EVENT_GLOBAL_DEMO_OSD_DISPLAY_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_DEMO_OSD_DISPLAY_ID);
		}

		if(ipcWhatEventOn(EVENT_GLOBAL_DEMO_KEY_START_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_DEMO_KEY_START_ID);
			demoOSDGlobalDEMOKEY(pGlobalInfo);
			ReturnGlobalOSDDebug(pGlobalInfo,EVENT_GLOBAL_DEMO_KEY_START,pGlobalInfo->sDemoStr[EVENT_GLOBAL_DEMO_KEY_START],pGlobalInfo->iDemoStrLength[EVENT_GLOBAL_DEMO_KEY_START]);	
		}
		//========================================================
		//if(ret == ETIMEDOUT)
		//{
			//if(pGlobalInfo->OldTick  != GetTickCount())		
			//{
				//pGlobalInfo->OldTick = GetTickCount();
				
				//pGlobalInfo->OSDDebugADCThreadstate = FALSE;
			//}
			//else
			//{
				//pGlobalInfo->ThreadstateCount++;
				//if(pGlobalInfo->ThreadstateCount >= 5)
				//{
					//pGlobalInfo->OSDDebugADCThreadstate = FALSE;
					//pGlobalInfo->OSDDebugADCThreadstate = TRUE;									
					//demoOSDSameTick(pGlobalInfo,pGlobalInfo->OldTick);
					//ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_SAMETICKT,pGlobalInfo->sDemoStr[OSD_DEBUG_SAMETICKT],pGlobalInfo->iDemoStrLength[OSD_DEBUG_SAMETICKT]);			
					//demoOSDGlobalTime(pGlobalInfo);
					//ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_GLOBALTIME,pGlobalInfo->sDemoStr[OSD_DEBUG_GLOBALTIME],pGlobalInfo->iDemoStrLength[OSD_DEBUG_GLOBALTIME]);	
				//}
				//else
				//{
					//pGlobalInfo->OSDDebugADCThreadstate = FALSE;
				//}
			//}			
		//}
		//========================================================
		//if( (GetTickCount() - pGlobalInfo->PerTick) >= 30000)
		//{
			//pGlobalInfo->OSDDebugGetTickstate = TRUE;	
			//pGlobalInfo->GetTickstateCount++;						
		//}
		//pGlobalInfo->PerTick = GetTickCount();	
		//========================================================
		if(1)
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->sErrorDriverName[0]
				|| pFlyAllInOneInfo->pMemory_Share_Common->sErrorDriverName[1]
				|| pFlyAllInOneInfo->pMemory_Share_Common->sErrorDriverName[2])
			{
				for(i=0;i<2;i++)
				{
					DBG0(debugOneData("\r\nGlobal Struct Access Error:%c"
					,pFlyAllInOneInfo->pMemory_Share_Common->sErrorDriverName[i]);)
				}
						//编译时间
			}	
			//=======================================================================
			DBG1(debugOneData("\n bOSDDemoMode = ",pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode);)
			
			DBG1(debugOneData("\r\niPanelKeyAD[0]: ",pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[0]);)
			DBG1(debugOneData("\r\niPanelKeyAD[1]: ",pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[1]);)
			DBG1(debugOneData("\r\niPanelKeyAD[2]: ",pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[2]);)
			DBG1(debugOneData("\r\niSteelAD[0]: ",pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[0]);)
			DBG1(debugOneData("\r\niSteelAD[1]: ",pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[1]);)
			DBG1(debugOneData("\r\niKeyIndex: ",pFlyAllInOneInfo->pMemory_Share_Common->iKeyIndex);)
			DBG1(debugOneData("\r\niKeyValue: ",pFlyAllInOneInfo->pMemory_Share_Common->iKeyValue);)
			DBG1(debugOneData("\r\niKeyCount: ",pFlyAllInOneInfo->pMemory_Share_Common->iKeyCount);)
			//=======================================================================
			
			if((pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode == TRUE) || (pGlobalInfo->OSDDebugADCThreadstate == TRUE) || (pGlobalInfo->OSDDebugGetTickstate == TRUE))
			{			
				if(pGlobalInfo->bOSDDebugInit == TRUE)
				{
					ReturnPowerMode(pGlobalInfo,1);
					ReturnGlobalWorkMode(pGlobalInfo,1);	
					pGlobalInfo->bOSDDebugInit = FALSE;
				}
				//demoOSDUARTDebugMsgOn(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iUARTDebugMsgOn);
				//ReturnGlobalOSDDebug(pGlobalInfo,UARTDEBUGMSGON,pGlobalInfo->sDemoStr[UARTDEBUGMSGON],pGlobalInfo->iDemoStrLength[UARTDEBUGMSGON]);
				demoOSDDriversCompileTime(pGlobalInfo);//编译时间
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_COMPILE_TIME_ERROR,pGlobalInfo->sDemoStr[OSD_DEBUG_COMPILE_TIME_ERROR],pGlobalInfo->iDemoStrLength[OSD_DEBUG_COMPILE_TIME_ERROR]);

				demoOSDKeyADDisplay(pGlobalInfo);//显示AD值、按键序列
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_KEYAD_LIST,pGlobalInfo->sDemoStr[OSD_DEBUG_KEYAD_LIST],pGlobalInfo->iDemoStrLength[OSD_DEBUG_KEYAD_LIST]);
				
				demoOSDDVDSoftVersion(pGlobalInfo);//DVD版本号
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_SOFTVERSION,pGlobalInfo->sDemoStr[OSD_DEBUG_SOFTVERSION],pGlobalInfo->iDemoStrLength[OSD_DEBUG_SOFTVERSION]);
				
				demoOSDHostTemperature(pGlobalInfo);//显示主机温度
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_HOST_TEMPERATURE,pGlobalInfo->sDemoStr[OSD_DEBUG_HOST_TEMPERATURE],pGlobalInfo->iDemoStrLength[OSD_DEBUG_HOST_TEMPERATURE]);

				demoOSDBreakAndPhoneStatus(pGlobalInfo);//显示刹车状态
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_BREAK_AND_PHONE_STATUS,pGlobalInfo->sDemoStr[OSD_DEBUG_BREAK_AND_PHONE_STATUS],pGlobalInfo->iDemoStrLength[OSD_DEBUG_BREAK_AND_PHONE_STATUS]);

				//demoOSDOpenStatus(pGlobalInfo);//显示驱动打开状态
				//ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_INIT_OPEN_STATUS,pGlobalInfo->sDemoStr[OSD_DEBUG_INIT_OPEN_STATUS],pGlobalInfo->iDemoStrLength[OSD_DEBUG_INIT_OPEN_STATUS]);

				demoOSDOtherInfo(pGlobalInfo);//其它调试信息				
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_OTHER_INFO,pGlobalInfo->sDemoStr[OSD_DEBUG_OTHER_INFO],pGlobalInfo->iDemoStrLength[OSD_DEBUG_OTHER_INFO]);

				demoOSDDvdType(pGlobalInfo);//DVD类型
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_DVD_TYPE,pGlobalInfo->sDemoStr[OSD_DEBUG_DVD_TYPE],pGlobalInfo->iDemoStrLength[OSD_DEBUG_DVD_TYPE]);

				demoOSDPanelName(pGlobalInfo);
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_PANEL_NAME,pGlobalInfo->sDemoStr[OSD_DEBUG_PANEL_NAME],pGlobalInfo->iDemoStrLength[OSD_DEBUG_PANEL_NAME]);

				demoOSDRunTime(pGlobalInfo);
				ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_RUN_TIME,pGlobalInfo->sDemoStr[OSD_DEBUG_RUN_TIME],pGlobalInfo->iDemoStrLength[OSD_DEBUG_RUN_TIME]);

				//if(pGlobalInfo->OSDDebugADCThreadstate == TRUE)				
				//{
				//	demoOSDADCThreadState(pGlobalInfo,GetTickCount(),pGlobalInfo->ThreadstateCount);	
				//	ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_ADCSTATE,pGlobalInfo->sDemoStr[OSD_DEBUG_ADCSTATE],pGlobalInfo->iDemoStrLength[OSD_DEBUG_ADCSTATE]);					
									
				//}
				//if(pGlobalInfo->OSDDebugGetTickstate == TRUE)	
				//{
				//	demoOSDGetTickCount(pGlobalInfo,GetTickCount(),pGlobalInfo->GetTickstateCount);
				//	ReturnGlobalOSDDebug(pGlobalInfo,OSD_DEBUG_GETTICKT,pGlobalInfo->sDemoStr[OSD_DEBUG_GETTICKT],pGlobalInfo->iDemoStrLength[OSD_DEBUG_GETTICKT]);	
				
				//}	
			 }				
			}
			
		}
	//#endif
	DBG0(debugString("ThreadMainProcess exit\n");)
	pGlobalInfo->bFlyGlobalThreadRunning = FALSE;
	return NULL;
}

/**********************************************************************************
**函数名称:		flyReadData
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
 INT flyReadData(BYTE *buf, UINT len)
 {
	UINT16 dwRead;
	DBG1(debugBuf("\nGlobal-HAL return  bytes to User Start:", buf,3);)
	dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	DBG1(debugBuf("\nGlobal-HAL return  bytes to User End:", buf,dwRead);)
	return dwRead;		
 }
/**********************************************************************************
**函数名称:		 INT flyOpenDevice(void)
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
INT flyOpenDevice(void)
{
	INT res;
	pthread_t thread_id;
	INT ret = HAL_ERROR_RETURN_FD;

	//#ifdef LDH
	
	pFlyGlobalInfo->bKillDispatchFlyGlobalMainThread = FALSE;
	pFlyGlobalInfo ->OSDDebugGetTickstate = FALSE;
	pFlyGlobalInfo ->OSDDebugADCThreadstate = FALSE;
	pFlyGlobalInfo->GetTickstateCount = 0;
	pFlyGlobalInfo->ThreadstateCount = 0;
	res = pthread_create(&thread_id, NULL,ThreadMainProcess,pFlyGlobalInfo);
	DBG0(debugOneData("\n2012-3-2 19:19 ThreadMainProcess ID:",thread_id);)
	if(res != 0) 
	{
		DBG0(debugString("Global-> pthread_create flase!!");)
		return ret;
	}

	pFlyGlobalInfo->fd = serial_open();
	if (pFlyGlobalInfo->fd <= 0)
	{
		debugString("\nGlobal open serial 5 err");
		return ret;
	}
	
	//#endif
	DBG0(debugString("\npFlyGlobal open ok\n");)	
	ret = HAL_GLOBAL_RETURN_FD;
	return ret;
 }
 /**********************************************************************************
**函数名称:		 INT flyCloseDevice(void)
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
INT flyCloseDevice(void)
{
	if (pFlyGlobalInfo->fd > 0)
	{
		serial_close(pFlyGlobalInfo->fd);
		pFlyGlobalInfo->fd = -1;
	}
	
	return 0;
}
 /********************************************************************************
 **函数名称：fly_destroy_struct()
 **函数功能：释放内存
 **函数参数：
 **返 回 值：无
 **********************************************************************************/
void flyDestroyDeviceStruct(void)
{	
	
	//等待串口读线程退出
	pFlyGlobalInfo->bKillDispatchFlyGlobalMainThread = TRUE;
		 
	//释放一个条件变量
	pthread_cond_destroy(&pFlyGlobalInfo->MainThreadCond);
	
	allInOneDeinit();
	
	free(pFlyGlobalInfo);
	pFlyGlobalInfo = NULL;
}
/**********************************************************************************
**函数名称:		flyInitDeviceStruct
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void flyInitDeviceStruct(void)
{
	//#ifdef LDH
	pFlyGlobalInfo = (struct flyglobal_struct_info *)malloc(sizeof(struct flyglobal_struct_info));
	if(pFlyGlobalInfo == NULL)
	{
		return;
	}	
	memset(pFlyGlobalInfo,0,sizeof(struct flyglobal_struct_info));//设置为0

	pthread_mutex_init(&pFlyGlobalInfo->MainThreadMutex, NULL);
	pthread_cond_init(&pFlyGlobalInfo->MainThreadCond, NULL);
	allInOneInit();
	/*********************************************************/
	DBG0(debugString("\n FlyGlobal hal init ");)
	DBG0(debugString(__TIME__);)
	DBG0(debugString(__DATE__);)
	DBG0(debugString(" \n");)
	/*********************************************************/
	pFlyGlobalInfo ->OSDDebugGetTickstate = FALSE;
	pFlyGlobalInfo ->OSDDebugADCThreadstate = FALSE;
	pFlyGlobalInfo->GetTickstateCount = 0;
	pFlyGlobalInfo->ThreadstateCount = 0;
	pFlyGlobalInfo->bOSDDebugInit = TRUE;
	/*********************************************************/
	//#endif
}
/**********************************************************************************
**函数名称:		flyCommandProcessor
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
	//#ifdef 	LDH
	DBG0(debugBuf("\n Global-> User write bytes to  Global-HAL:", buf,len);)
	DealRightDataProcessor(pFlyGlobalInfo,&buf[3], buf[2]-1);
	//#endif
}










































