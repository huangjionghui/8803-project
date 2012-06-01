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
#define VIN_ITU "1"
#define VIN_USB "2" 

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_VIDEO
#define LOCAL_HAL_NAME		"flyVideo Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_VIDEO

#include "FlyVideo.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"

static void powerNormalInit(void);

struct flyvideo_struct_info *pFlyVideoInfo = NULL;

void returnVideoReadyToOpenCamera(void);

static void camHaveBlockInit(void)
{
	property_set("tcc.fly.vin.block", "1");
}

static void cameraPal(BOOL bPal)
{
	if (bPal)
	{
		property_set("tcc.fly.vin.pal", "1");
	}
	else
	{
		property_set("tcc.fly.vin.pal", "0");
	}
}

static void camContinueWithITU(void)
{
	property_set("tcc.fly.vin.type", VIN_ITU);

	property_set("tcc.fly.vin.go", "1");
	returnVideoReadyToOpenCamera();
	DBG0(debugString("\nvideo GO On ITU");)
}

static void camContinueWithUSB(void)
{
	property_set("tcc.fly.vin.type", VIN_USB);

	property_set("tcc.fly.vin.go", "1");
	returnVideoReadyToOpenCamera();
	DBG0(debugString("\nvideo GO On USB");)
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	DBG0(debugBuf("VideoHAL read from hardware",buf,length);)
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	switch (sourceEvent)
	{
	case EVENT_AUTO_CLR_RESUME_ID:
		pFlyVideoInfo->iChannelChangeTime = GetTickCount();//模拟切换，强制延时
		pFlyVideoInfo->bAuxVideoForceReturn = TRUE;
		pFlyVideoInfo->bEnterSuspend = FALSE;
		break;
	case EVENT_AUTO_CLR_SUSPEND_ID:
		pFlyVideoInfo->bEnterSuspend = TRUE;
		break;

	default:
		break;
	}
	PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
}

/********************************************************************************
**函数名称：
**函数功能：返回数据给用户
**函数参数：
**返 回 值：
**********************************************************************************/
static void flyVideoReturnToUser(BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nVideo-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nVideo-HAL write  bytes to User Error:", buf,len);)
	}
}

/******************************************************************************/
/*                            返回给用户的各种信息                        */
/******************************************************************************/
void returnVideoPowerMode(BYTE bPower)
{
	BYTE buff[] = {0x01,0x00};
	buff[1] = bPower;

	flyVideoReturnToUser(buff,2);
}

void returnVideoWorkMode(BYTE bWork)
{
	BYTE buff[] = {0x02,0x00};
	buff[1] = bWork;

	flyVideoReturnToUser(buff,2);
}

void returnVideoInput(BYTE iChannel)
{
	BYTE buff[] = {0x10,0x00};
	buff[1] = iChannel;

	flyVideoReturnToUser(buff,2);
}

void returnDisplayValue(BYTE enumWhat,BYTE iValue)
{
	BYTE buff[3] = {0x11,0x00,0x00};

	buff[1] = enumWhat;
	buff[2] = iValue;

	flyVideoReturnToUser(buff,3);
}

void returnVideoReadyToOpenCamera(void)
{
	BYTE buff[] = {0x20,0x00};

	flyVideoReturnToUser(buff,2);
}

/******************************************************************************/
/*                              控制驱动的各种信息                          */
/******************************************************************************/
void controlVideoInit(void)
{
	BYTE buff[] = {SHARE_MEMORY_VIDEO,MSG_VIDEO_INIT};
	writeDataToHardware(buff, sizeof(buff));

	DBG0(debugString("\nFlyVideo Init");)
}

static BOOL readbHaveVideo(void)
{
	BYTE buff[] = {CURRENT_SHARE_MEMORY_ID,S_NO_BLOCK_ID,MSG_VIDEO_REQ_HAVE_VIDEO,0x00,0x00};
	readDataFromHardwareNoBlock(buff,sizeof(buff));

	if (buff[3])
	{
		if (buff[4])
		{
			pFlyVideoInfo->bPreVideoPal = TRUE;
		}
		else
		{
			pFlyVideoInfo->bPreVideoPal = FALSE;
		}
		if (pFlyVideoInfo->bCurVideoPal != pFlyVideoInfo->bPreVideoPal)
		{
			pFlyVideoInfo->bCurVideoPal = pFlyVideoInfo->bPreVideoPal;
			cameraPal(pFlyVideoInfo->bPreVideoPal);
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void controlVideoInput(BYTE iChannel)
{
	BYTE buff[] = {SHARE_MEMORY_VIDEO,MSG_VIDEO_CON_INPUT,0x00};
	buff[2] = iChannel;
	writeDataToHardware(buff, sizeof(buff));
}

void controlVideoColorful(BYTE iColor,BYTE iHue,BYTE iContrast,BYTE iBrightness)
{
	BYTE buff[] = {SHARE_MEMORY_VIDEO,MSG_VIDEO_CON_COLORFUL,0x00,0x00,0x00,0x00};
	buff[2] = iColor;
	buff[3] = iHue;
	buff[4] = iContrast;
	buff[5] = iBrightness;
	writeDataToHardware(buff, sizeof(buff));
}

void checkBackCameraVideo(void)
{
	pFlyVideoInfo->bBackVideoOn = readbHaveVideo();
	pFlyVideoInfo->bBackPreVideoPal = pFlyVideoInfo->bPreVideoPal;

	if (pFlyVideoInfo->bBackVideoForceReturn
		|| pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn != pFlyVideoInfo->bBackVideoOn)
	{
		if (!pFlyVideoInfo->bBackVideoOn)
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn)//有视频突然变成无视频
			{
				pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn = pFlyVideoInfo->bBackVideoOn;
				pFlyVideoInfo->bBackVideoForceReturn = TRUE;
				pFlyVideoInfo->iBackVideoReturnTime = GetTickCount();
			}
			else if (GetTickCount() - pFlyVideoInfo->iChannelChangeTime >= 1000
				&& GetTickCount() - pFlyVideoInfo->iBackVideoReturnTime >= 1000)
			{
				pFlyVideoInfo->bBackVideoForceReturn = FALSE;

				pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn = pFlyVideoInfo->bBackVideoOn;
				ipcStartEvent(EVENT_GLOBAL_BACKDETECT_RETURN_ID);
				pFlyVideoInfo->iBackVideoReturnTime = GetTickCount();
			}
		}
		else
		{
			pFlyVideoInfo->bBackVideoForceReturn = FALSE;

			pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn = pFlyVideoInfo->bBackVideoOn;
			ipcStartEvent(EVENT_GLOBAL_BACKDETECT_RETURN_ID);
			pFlyVideoInfo->iBackVideoReturnTime = GetTickCount();

			pFlyAllInOneInfo->pMemory_Share_Common->bHaveEverBackVideoOn = TRUE;
		}
	}
}

void checkAuxVideo(void)
{
	pFlyVideoInfo->bAuxVideoOn = readbHaveVideo();
	pFlyVideoInfo->bAuxPreVideoPal = pFlyVideoInfo->bPreVideoPal;

	if (pFlyVideoInfo->bAuxVideoForceReturn
		|| pFlyAllInOneInfo->pMemory_Share_Common->bAUXHaveVideo != pFlyVideoInfo->bAuxVideoOn)
	{
		if (!pFlyVideoInfo->bAuxVideoOn)
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->bAUXHaveVideo)//有视频突然变成无视频
			{
				pFlyAllInOneInfo->pMemory_Share_Common->bAUXHaveVideo = pFlyVideoInfo->bAuxVideoOn;
				pFlyVideoInfo->bAuxVideoForceReturn = TRUE;
				pFlyVideoInfo->bAuxVideoReturnTime = GetTickCount();
			}
			else if (GetTickCount() - pFlyVideoInfo->iChannelChangeTime >= 1000
				&& GetTickCount() - pFlyVideoInfo->bAuxVideoReturnTime >= 1000)
			{
				pFlyVideoInfo->bAuxVideoForceReturn = FALSE;

				pFlyAllInOneInfo->pMemory_Share_Common->bAUXHaveVideo = pFlyVideoInfo->bAuxVideoOn;
				ipcStartEvent(EVENT_GLOBAL_AUX_CHECK_RETURN_ID);
				pFlyVideoInfo->bAuxVideoReturnTime = GetTickCount();
			}
		}
		else
		{
			pFlyVideoInfo->bAuxVideoForceReturn = FALSE;

			pFlyAllInOneInfo->pMemory_Share_Common->bAUXHaveVideo = pFlyVideoInfo->bAuxVideoOn;
			ipcStartEvent(EVENT_GLOBAL_AUX_CHECK_RETURN_ID);
			pFlyVideoInfo->bAuxVideoReturnTime = GetTickCount();
		}
	}
}
/******************************************************************************/
/*                                各种线程函数                            */
/******************************************************************************/
void *video_main_thread(void *arg) 
{
	INT ret = 0;
	ULONG waitTime = 0;

	debugString("\r\nFlyAudio 5150 ThreadMain Start");

	while (!pFlyVideoInfo->bKillFlyVideoMainThread)
	{
		WaitSignedTimeOut(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain,waitTime);
		waitTime = 0;

		if (pFlyVideoInfo->bEnterSuspend)
		{
			continue;
		}

		if (ipcWhatEventOn(EVENT_GLOBAL_BACKDETECT_CHANGE_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_BACKDETECT_CHANGE_ID);
			if (pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable
				&& pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->bHaveEverBackVideoOn = FALSE;
				pFlyVideoInfo->bBackVideoForceReturn = TRUE;
				pFlyVideoInfo->iChannelChangeTime = GetTickCount();//模拟切换，强制延时
			}
		}

		if (ipcWhatEventOn(EVENT_GLOBAL_AUX_CHECK_START_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_AUX_CHECK_START_ID);
			pFlyVideoInfo->iPreVideoChannel = AUX;
			pFlyVideoInfo->iChannelChangeTime = GetTickCount();//模拟切换，强制延时
			pFlyVideoInfo->bAuxVideoForceReturn = TRUE;
		}

		if (pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable
			&& pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow)
		{
			pFlyVideoInfo->iTempVideoChannel = BACK;

			pFlyVideoInfo->iTempVideoParaColor = COLOR_STEP_COUNT/2;
			pFlyVideoInfo->iTempVideoParaHue = COLOR_STEP_COUNT/2;
			pFlyVideoInfo->iTempVideoParaContrast = COLOR_STEP_COUNT/2;
			pFlyVideoInfo->iTempVideoParaBrightness = COLOR_STEP_COUNT/2;
		}
		else
		{
			pFlyVideoInfo->iTempVideoChannel = pFlyVideoInfo->iPreVideoChannel;

			pFlyVideoInfo->iTempVideoParaColor = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor;
			pFlyVideoInfo->iTempVideoParaHue = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue;
			pFlyVideoInfo->iTempVideoParaContrast = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast;
			pFlyVideoInfo->iTempVideoParaBrightness = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness;
		}

		if (pFlyVideoInfo->iCurVideoChannel != pFlyVideoInfo->iTempVideoChannel)
		{
			pFlyVideoInfo->iCurVideoChannel = pFlyVideoInfo->iTempVideoChannel;
			controlVideoInput(pFlyVideoInfo->iCurVideoChannel);
			if (BACK == pFlyVideoInfo->iCurVideoChannel)//切换后延时，确保视频检测正确
			{
				Sleep(100);
			}
			pFlyVideoInfo->iChannelChangeTime = GetTickCount();

			if (0x00 != pFlyVideoInfo->iCurVideoChannel && 0x80 != pFlyVideoInfo->iCurVideoChannel)
			{
				pFlyVideoInfo->iActualVideoChannel = pFlyVideoInfo->iCurVideoChannel;
			}
		}

		if (pFlyVideoInfo->iVideoParaColor != pFlyVideoInfo->iTempVideoParaColor
			|| pFlyVideoInfo->iVideoParaHue != pFlyVideoInfo->iTempVideoParaHue
			|| pFlyVideoInfo->iVideoParaContrast != pFlyVideoInfo->iTempVideoParaContrast
			|| pFlyVideoInfo->iVideoParaBrightness != pFlyVideoInfo->iTempVideoParaBrightness)
		{
			pFlyVideoInfo->iVideoParaColor = pFlyVideoInfo->iTempVideoParaColor;
			pFlyVideoInfo->iVideoParaHue = pFlyVideoInfo->iTempVideoParaHue;
			pFlyVideoInfo->iVideoParaContrast = pFlyVideoInfo->iTempVideoParaContrast;
			pFlyVideoInfo->iVideoParaBrightness = pFlyVideoInfo->iTempVideoParaBrightness;
			controlVideoColorful(pFlyVideoInfo->iVideoParaColor
				,pFlyVideoInfo->iVideoParaHue
				,pFlyVideoInfo->iVideoParaContrast
				,pFlyVideoInfo->iVideoParaBrightness);
		}

		if (pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable
			&& pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow)
		{
			checkBackCameraVideo();
		}
		else if (pFlyVideoInfo->iActualVideoChannel == AUX)
		{
			checkAuxVideo();
		}

		if (AUX == pFlyVideoInfo->iActualVideoChannel
			|| BACK == pFlyVideoInfo->iActualVideoChannel)
		{
			waitTime = 200;
		}
	}

	debugString("\r\nFlyAudio 5150 ThreadMain exit");
	return 0;
}


/******************************************************************************/
/*                                  处理数据                              */
/******************************************************************************/
static void DealRightDataProcessor(BYTE *buff, UINT16 len)
{
switch (buff[0])
{
	case 0x01:
		if (0x01 == buff[1])
		{
			returnDisplayValue(E_C_COLOR,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor);
			returnDisplayValue(E_C_HUE,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue);
			returnDisplayValue(E_C_CONTRAST,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast);
			returnDisplayValue(E_C_BRIGHTNESS,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness);

			returnVideoPowerMode(TRUE);
			returnVideoWorkMode(TRUE);
		}
		else
		{
			returnVideoPowerMode(FALSE);
		}
		break;
	case 0x10:
		if (FRONT_CAMERA == buff[1])
		{
			camContinueWithUSB();
		}
		else if (AUX == buff[1])
		{
			cameraPal(pFlyVideoInfo->bAuxPreVideoPal);
			camContinueWithITU();
		}
		else if (BACK == buff[1])
		{
			cameraPal(pFlyVideoInfo->bBackPreVideoPal);
			camContinueWithITU();
		}
		else
		{
			pFlyVideoInfo->bPreVideoPal = FALSE;
			cameraPal(pFlyVideoInfo->bPreVideoPal);
			camContinueWithITU();
		}
		pFlyVideoInfo->iPreVideoChannel = buff[1];

		pFlyAllInOneInfo->pMemory_Share_Common->eVideoInput = buff[1];

		returnVideoInput(buff[1]);
		PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
		break;
	case 0x11:
		switch (buff[1])
		{
		case 0x01:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor < COLOR_STEP_COUNT - 1)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor++;
			} 
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor = COLOR_STEP_COUNT - 1;
			}
			returnDisplayValue(E_C_COLOR,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x02:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor >= COLOR_STEP_COUNT)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor = COLOR_STEP_COUNT - 1;
			} 
			else if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor--;
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor = 0;
			}
			returnDisplayValue(E_C_COLOR,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x03:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue < COLOR_STEP_COUNT - 1)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue++;
			} 
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue = COLOR_STEP_COUNT - 1;
			}
			returnDisplayValue(E_C_HUE,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x04:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue >= COLOR_STEP_COUNT)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue = COLOR_STEP_COUNT - 1;
			} 
			else if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue--;
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue = 0;
			}
			returnDisplayValue(E_C_HUE,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x05:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast < COLOR_STEP_COUNT - 1)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast++;
			} 
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast = COLOR_STEP_COUNT - 1;
			}
			returnDisplayValue(E_C_CONTRAST,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x06:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast >= COLOR_STEP_COUNT)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast = COLOR_STEP_COUNT - 1;
			} 
			else if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast--;
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast = 0;
			}
			returnDisplayValue(E_C_CONTRAST,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x07:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness < COLOR_STEP_COUNT - 1)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness++;
			} 
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness = COLOR_STEP_COUNT - 1;
			}
			returnDisplayValue(E_C_BRIGHTNESS,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		case 0x08:
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness >= COLOR_STEP_COUNT)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness = COLOR_STEP_COUNT - 1;
			} 
			else if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness--;
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness = 0;
			}
			returnDisplayValue(E_C_BRIGHTNESS,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness);
			PostSignal(&pFlyVideoInfo->MainThreadMutex,&pFlyVideoInfo->MainThreadCond,&pFlyVideoInfo->bMainThreadRunAgain);
			break;
		default:
			break;
		}
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
	INT res;
	pthread_t thread_id;
	
	pFlyVideoInfo->bOpen = TRUE;
	
	pFlyVideoInfo->bKillFlyVideoMainThread = FALSE;
	res = pthread_create(&thread_id,NULL,video_main_thread,NULL);
	if(res != 0) 
	{
		debugString("video_main_thread error");
		return -1;
	}

	return HAL_VIDEO_RETURN_FD;
}

/********************************************************************************
**函数名称：fly_init_device_struct（）函数
**函数功能：初始化结构体里的成员
**函数参数：
**返 回 值：
**********************************************************************************/
void flyInitDeviceStruct(void)
{

	//为 flyvideo_struct_info 结构体分配内存
	pFlyVideoInfo =
		(struct flyvideo_struct_info *)malloc(sizeof(struct flyvideo_struct_info));
	if (pFlyVideoInfo == NULL)
	{
		//debugString("\r\npFlyVideoInfo malloc error");
		return;
	}
	memset(pFlyVideoInfo, 0, sizeof(struct flyvideo_struct_info));

	pthread_mutex_init(&pFlyVideoInfo->MainThreadMutex, NULL);
	pthread_cond_init(&pFlyVideoInfo->MainThreadCond, NULL);

	allInOneInit();

	camHaveBlockInit();

	DBG0(debugString("\nFlyVideo hal init\n");)
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
	DBG1(debugBuf("\nVideo-HAL return  bytes Start:", buf,1);)
	dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	DBG1(debugBuf("\nVideo-HAL return  bytes to User:", buf,dwRead);)
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
	pFlyVideoInfo->bKillFlyVideoMainThread = TRUE;

	//释放各种条件变量
	pthread_cond_destroy(&pFlyVideoInfo->MainThreadCond);

	allInOneDeinit();
	free (pFlyVideoInfo);
	pFlyVideoInfo = NULL;
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
	pFlyVideoInfo->bKillFlyVideoMainThread = TRUE;

	//DLOGD("Fly Video Close");
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
	DBG1(debugBuf("\nVideo-HAL RecJNI:",buf,len);)
	
	DealRightDataProcessor(&buf[3], buf[2]-1);
}