#include <sys/types.h> 
#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>  
#define LOCAL_HAL_ID		HAL_DEVICE_NAME_SERVICEHAL
#define LOCAL_HAL_NAME		"FlyServiceHal Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_COMMON

#define HAL_SAVE_FILE_PATH "/data/fly/.savefile"
//#define HAL_SAVE_STEELWHEEL_PATH "/data/fly/zzzz1"
//#define HAL_SAVE_PANEL_PATH "/data/fly/zzzz2"

#include "FlyServiceHal.h"
#include "../../include/allInOneService.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"
#include "../../include/saveFile.c"

#undef DEBUG_TPMS
//#define DEBUG_TPMS 1

#ifdef DEBUG_TPMS
#define SERIAL_NAME			"/dev/tcc-uart3" 
#define SERIAL_BAUDRATE      19200   
#include "../../include/serial.c"
#endif

static int xxxxxxxxxxxxxxx = 0;

struct flyServiceHal_struct_info *pFlyServiceHalInfo = NULL;

static void paraInit(void);
static void paraRead(void);
static void paraWrite(void);


/********************************************************************************
**函数名称：
**函数功能：返回数据给用户
**函数参数：
**返 回 值：
**********************************************************************************/
static void flyAudioReturnToUser(BYTE *buf, UINT16 len)
{
	UINT dwLength;

	DBG0(debugBuf("\nService-HAL ToJNI:",buf,len);)

		dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nService-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nService-HAL write  bytes to User Error:", buf,len);)
	}
}
 
void ipcEventExchangeProc(UINT32 sourceEvent)
{
	UINT i;
	DBG0(debugOneData("\nServiceHAL IPC Exchange ID",sourceEvent);)

	switch (sourceEvent)
	{
	case EVENT_AUTO_CLR_SUSPEND_ID:
	case EVENT_AUTO_CLR_RESUME_ID:

	case EVENT_AUTO_CLR_PARA_INIT_ID:
	case EVENT_AUTO_CLR_PARA_READ_ID:
	case EVENT_AUTO_CLR_PARA_WRITE_ID:

	case EVENT_AUTO_CLR_STANDBY_ID:
		for (i = 0;i < SHARE_MEMORY_MAX;i++)
		{
			ipcExchangeEvent(sourceEvent,i);
		}
		ipcClearEvent(sourceEvent);
#ifdef DEBUG_TPMS
		tcflush(pFlyServiceHalInfo->flySerialHal_fd, TCIFLUSH);
#endif
		break;

	case EVENT_GLOBAL_TEST_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_COMMON);
		break;
	case EVENT_GLOBAL_DATA_CHANGE_VOLUME:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;

	case EVENT_GLOBAL_REQ_USER_ACTION:
	case EVENT_GLOBAL_REQ_USER_ACCON:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_COMMON);
		break;
	case EVENT_GLOBAL_RES_USER_ACTION:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

	case EVENT_GLOBAL_BTCALLSTATUS_CHANGE_ID:
		DBG0(debugString("servicehal --------\n");)
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;

	case EVENT_GLOBAL_PHONEDETECT_CHANGE_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;
	case EVENT_GLOBAL_PHONECALLSTATUS_CHANGE_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;

	case EVENT_GLOBAL_RADIO_CHANGE_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_RADIO);
		break;

		//倒车事件
	case EVENT_GLOBAL_BACKDETECT_CHANGE_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_VIDEO);
		break;
	case EVENT_GLOBAL_BACKDETECT_RETURN_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;
	case EVENT_GLOBAL_BACK_LOW_VOLUME_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;
	case EVENT_GLOBAL_BACK_DELAY_LIGHT_ON_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

		//Standby事件
	case EVENT_GLOBAL_KEY_STANDBY_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;


	case EVENT_GLOBAL_REMOTE_STUDY_START_ID:
	case EVENT_GLOBAL_REMOTE_STUDY_STOP_ID:
	case EVENT_GLOBAL_REMOTE_STUDY_CLEAR_ID:

	case EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_FINISH_ID:
	case EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_KEY);
		break;

	case EVENT_GLOBAL_PANEL_KEY_USE_IT_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_KEY);
		break;

	case EVENT_GLOBAL_REMOTE_USE_IT_ID:
		if (dealSteelWheelData(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat))
		{   
			pFlyAllInOneInfo->pMemory_Share_Common->bRemoteDataHave = TRUE;
		}
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_KEY);
		break;



	case EVENT_GLOBAL_REMOTE_STUDY_RETURN_START_ID:
	case EVENT_GLOBAL_REMOTE_STUDY_RETURN_FINISH_ID:
	case EVENT_GLOBAL_REMOTE_STUDY_RETURN_WAIT_ID:

	case EVENT_GLOBAL_BREAKDETECT_CHANGE_ID:
	case EVENT_GLOBAL_INNER_AMP_ON_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

	//OSD调试信息显示事件
	case EVENT_GLOBAL_DEMO_OSD_START_ID:
	case EVENT_GLOBAL_DEMO_OSD_DISPLAY_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_GLOBAL);
		break;
	case EVENT_GLOBAL_DEMO_KEY_START_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_KEY);
		break;

	//电压低事件
	case EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_RADIO);
		break;
	case EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;

	//收音机天线事件
	case EVENT_GLOBAL_RADIO_ANTENNA_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_RADIO);
		break;
	//外部CDC小盒切换事件
	case EVENT_GLOBAL_EXBOX_INPUT_CHANGE_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_TV);
		break;

		//静音事件收音机
	case EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID:
	case EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;
	case EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_RADIO);
		break;
		//静音事件蓝牙
	case EVENT_GLOBAL_BT_MUTE_REQ_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;
	case EVENT_GLOBAL_BT_MUTE_IN_OK_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_BT);
		break;

		//碟机区域码事件
	case EVENT_GLOBAL_DVD_REGION_SET_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_DVD);
		break;

		//AUX操作事件
	case EVENT_GLOBAL_AUX_CHECK_START_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_VIDEO);
		break;
	case EVENT_GLOBAL_AUX_CHECK_RETURN_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

	//版本号返回
	case EVENT_GLOBAL_RETURN_DVD_VERSION_ID:
	case EVENT_GLOBAL_RETURN_BT_VERSION_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

		//LED闪烁指示错误
	//case EVENT_GLOBAL_ERROR_LEDFLASH_ID:
	//	eventInterExchangeEvent(pGlobalInfo,(enumGlobalDataChange)iEventWhat,pGlobalInfo->hDispatchLEDTestThreadEvent,TRUE);
	//	break;

		//偷偷地起来
	case EVENT_GLOBAL_SILENCE_POWER_UP_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_DVD);
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_AUDIO);
		break;

		//外部TV小盒存在事件
	case EVENT_GLOBAL_TVBOX_EXIST_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_TV);
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

		//外部TPMS小盒存在事件
	case EVENT_GLOBAL_TPMSBOX_EXIST_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_TPMS);
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

		//应用程序的Ping
	case EVENT_GLOBAL_USER_PING_START_ID:
	case EVENT_GLOBAL_USER_PING_WORK_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

		//强制复位
	case EVENT_GLOBAL_FORCE_RESET_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

	case EVENT_TOUCH_TIMEOUT_RETURN_ID:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_SYSTEM);
		break;

	//测试
	default:
		ipcExchangeEvent(sourceEvent,SHARE_MEMORY_COMMON);
		break;
	}
}

static void controlHostFirstAction(BYTE iWhatAction)
{
	BYTE buff[] = {0x2C,0x00};

	buff[1] = iWhatAction;

	flyAudioReturnToUser(buff,sizeof(buff));
}

static void controlHostACCOn(void)
{
	BYTE buff[] = {0x2D,0x00};

	debugOneData("\n############################24@@@@@@@@@@@@@@@",buff[1]);
	flyAudioReturnToUser(buff,sizeof(buff));
}

static void DealRightDataProcessor(BYTE *buff, UINT16 len)
{
	switch (buff[0])
	{
	case 0x2C:
		debugOneData("\n############################23@@@@@@@@@@@@@@@",buff[1]);
		pFlyAllInOneInfo->pMemory_Share_Common->iControlUserAction = buff[1];
		ipcStartEvent(EVENT_GLOBAL_RES_USER_ACTION);
		break;
	default:
		break;
	}
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	BOOL bNeedClear = TRUE;
	BOOL bNeedSetEvent = FALSE;

	//DBG0(debugOneData("\nServiceHAL IPC Read ID",sourceEvent);)

	switch (sourceEvent)
	{
	case EVENT_GLOBAL_TEST_ID:
		ipcClearEvent(sourceEvent);
		break;
	case EVENT_AUTO_CLR_PARA_INIT_ID:
		paraInit();
		break;
	case EVENT_AUTO_CLR_PARA_READ_ID:
		paraRead();
		break;
	case EVENT_AUTO_CLR_PARA_WRITE_ID:
		paraWrite();
		break;

	case EVENT_GLOBAL_REQ_USER_ACTION:
		controlHostFirstAction(pFlyAllInOneInfo->pMemory_Share_Common->iControlUserAction);
		break;
	case EVENT_GLOBAL_REQ_USER_ACCON:
		controlHostACCOn();
		break;

	default:
		bNeedClear = FALSE;
		bNeedSetEvent = TRUE;
		break;
	}

	if (bNeedClear)
	{
		ipcClearEvent(sourceEvent);
	}
	if (bNeedSetEvent)
	{
	}
}

void msgWriteToSerial(BYTE msgQueueID,BYTE *pData,UINT length)
{
	DBG0(debugOneData("\nServiceHAL msgQueue msgID",msgQueueID);)
	DBG0(debugBuf("ServiceHal msgQueue msgID:", pData, length);)
#ifdef DEBUG_TPMS
	if (msgQueueID == SHARE_MEMORY_TPMS || msgQueueID == SHARE_MEMORY_TV)
	{
		if (pFlyServiceHalInfo->flySerialHal_fd > 0)
		{
			DBG3(debugBuf("\nwrite data to serial:", pData, length);)

			serial_write(pFlyServiceHalInfo->flySerialHal_fd, pData, length);
		}
	}
#endif
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	DBG0(debugBuf("\nServiceHAL read from hardware",buf,length);)
}

static void paraInit(void)
{
	memset(&pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData,0,sizeof(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData));

	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaColor = COLOR_STEP_COUNT/2;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaHue = COLOR_STEP_COUNT/2;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaContrast = COLOR_STEP_COUNT/2;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iVideoParaBrightness = COLOR_STEP_COUNT/2;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight = FALSE;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay = 3;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight = 3;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDDriver = 0;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 9;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bSteerWheelOn = FALSE;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat = 0;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bRemoteUseStudyOn = FALSE;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable = TRUE;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iPanelLightBright = 0xFF;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bHaveExtAmplifierGMC = FALSE;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iBTCallVolume = 15;
	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iExtTelVolume = 15;

	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iUseWhichPanel = 7;

	pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iIDForFileStore = ID_FOR_FILE_STORE;

}

static void paraWrite(void)
{
	writeSaveFileData(HAL_SAVE_FILE_PATH, (BYTE*)&pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData,sizeof(FLY_RESTORE_DATA));
}

static void paraRead(void)
{
	DBG0(debugString("HAL write iVolumeMax start\n");)
		
	pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable = TRUE;

	readSaveFileData(HAL_SAVE_FILE_PATH,(BYTE*)&pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData,sizeof(FLY_RESTORE_DATA));

	if (ID_FOR_FILE_STORE != pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iIDForFileStore)
	{
		paraInit();
	}
	
	if (dealPanelData())
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iUseWhichPanel = 0xFF;
	}

	pFlyAllInOneInfo->pMemory_Share_Common->bMute = FALSE;
	pFlyAllInOneInfo->pMemory_Share_Common->iVolumeMin = 0;
	pFlyAllInOneInfo->pMemory_Share_Common->iVolumeMax = 60;
	pFlyAllInOneInfo->pMemory_Share_Common->bControlRadioANT = FALSE;

	dealsCarMessageFromFile();

	pFlyAllInOneInfo->pMemory_Share_Common->iVolume = pFlyAllInOneInfo->pMemory_Share_Common->iVolumeMax/2;
}
static void MessageToTPMS(struct flyServiceHal_struct_info *pFlyServiceHalInfo,BYTE dataOne, BYTE dataTwo)
{
	BYTE buff[2];
	buff[0] = dataOne;
	buff[1] = dataTwo;
	
	DBG1(debugBuf("\nmessage to TPMS:", buff, 2);)
	if (pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTpmsFromSerial)
		pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTpmsFromSerial(SHARE_MEMORY_COMMON,buff,2);
}
static void MessageToTV(struct flyServiceHal_struct_info *pFlyServiceHalInfo,BYTE dataOne, BYTE dataTwo)
{
	BYTE buff[2];
	buff[0] = dataOne;
	buff[1] = dataTwo;
	if (pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTvFromSerial)
		pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgReadTvFromSerial(SHARE_MEMORY_COMMON,buff,2);
}
static void FlyExBoxSendCmdToUART(struct flyServiceHal_struct_info *pFlyServiceHalInfo, BYTE dataOne, BYTE dataTwo)
{
#ifdef DEBUG_TPMS
	BYTE p[2];
	p[0] = dataOne;
	p[1] = dataTwo;
	
	DBG3(debugBuf("\nWrite to ExBox:",p, 2);)
	if (pFlyServiceHalInfo->flySerialHal_fd > 0)
		serial_write(pFlyServiceHalInfo->flySerialHal_fd, p, 2);
#endif
}
static void DeviceStatusInfo(struct flyServiceHal_struct_info *pFlyServiceHalInfo, BYTE para)
{
	BYTE deviceTvHave;
	BYTE deviceTireHave;
	
	deviceTvHave = para & (1<<6);
	deviceTireHave = para & (1<<4);
	
	DBG0(debugOneData("\n---TV", deviceTvHave);)
	DBG0(debugOneData("\n---TPMS", deviceTireHave);)
	
	//TV
	if (deviceTvHave)
	{
		//if (!pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule = TRUE;
			ipcStartEvent(EVENT_GLOBAL_TVBOX_EXIST_ID);
			DBG0(debugString("\nTV On");)
		}
		
		//DBG1(debugString("\nFlyAudio TV On");)
	}
	else
	{
		//if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule = FALSE;
			ipcStartEvent(EVENT_GLOBAL_TVBOX_EXIST_ID);
			
			DBG1(debugString("\nTV OFF");)
		}
		
		//DBG1(debugString("\nFlyAudio TV OFF");)
	}
	
	//TPMS
	if (deviceTireHave)
	{
		//if (!pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule = TRUE;
			ipcStartEvent(EVENT_GLOBAL_TPMSBOX_EXIST_ID);
			DBG1(debugString("\nTPMS On");)
		}
		
		//DBG1(debugString("\nFlyAudio TPMS On");)
	}
	else
	{
		//if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule = FALSE;
			ipcStartEvent(EVENT_GLOBAL_TPMSBOX_EXIST_ID);
			DBG1(debugString("\nTPMS OFF");)
		}
		
		//DBG1(debugString("\nFlyAudio TPMS OFF");)
	}
}
static void OnCommRecv(struct flyServiceHal_struct_info *pFlyServiceHalInfo, BYTE *buf, UINT16 len)
{
	UINT16 i;
	
	for (i=0; i<len; i++)
	{
		switch (pFlyServiceHalInfo->exBoxInfoFrameStatus)
		{
		case 0:
			if ((buf[i] & 0xF0) == 0xe0)
			{
				pFlyServiceHalInfo->exBoxInfoFrameStatus = 1;
			}
			else if ((buf[i] & 0xF0) == 0xF0)
			{
				if (buf[i] == 0xF0)
				{
					pFlyServiceHalInfo->exBoxInfoFrameStatus = 5;
				}
				else
				{
					pFlyServiceHalInfo->exBoxInfoFrameStatus = 2;
				}
				
			}
			else if ((buf[i] & 0xF0) == 0x80)
			{
				pFlyServiceHalInfo->exBoxInfoFrameStatus = 3;
			}
			else if (buf[i] == 0xaa)
			{
				pFlyServiceHalInfo->exBoxInfoFrameStatus = 4;
			}
			else
			{
				pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			}
			
			//record data1
			if (pFlyServiceHalInfo->exBoxInfoFrameStatus != 0)
			{
				pFlyServiceHalInfo->bRevData[0] = buf[i];
			}
			break;
			
			
		case 1://TPMS
			pFlyServiceHalInfo->bRevData[1] = buf[i];
			MessageToTPMS(pFlyServiceHalInfo,pFlyServiceHalInfo->bRevData[0],pFlyServiceHalInfo->bRevData[1]);
			pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			break;
			
		case 2://TV
			pFlyServiceHalInfo->bRevData[1] = buf[i];
			MessageToTV(pFlyServiceHalInfo,pFlyServiceHalInfo->bRevData[0],pFlyServiceHalInfo->bRevData[1]);
			pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			break;
			
			
		case 3://CDC
			pFlyServiceHalInfo->bRevData[1] = buf[i];			
			pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			break;
			
			
		case 4:
			pFlyServiceHalInfo->bRevData[1] = buf[i];
			if (buf[i] == 0xaa)
			{
				FlyExBoxSendCmdToUART(pFlyServiceHalInfo,0x55,0x55);
			}
			pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			break;
			
			
		case 5:
			pFlyServiceHalInfo->bRevData[1] = buf[i];
			DBG0(debugBuf("\nbuf:", buf, i);)
			DBG0(debugOneData("\ndjdf:", pFlyServiceHalInfo->bRevData[1]);)
			DeviceStatusInfo(pFlyServiceHalInfo,pFlyServiceHalInfo->bRevData[1]);	
			pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			break;
			
		default:
			pFlyServiceHalInfo->exBoxInfoFrameStatus = 0;
			break;
		}
	}
}
void *centralHalReadThread(void *arg)
{
	INT ret;
	BYTE recvBuf[1024];
	BYTE *syc_ptr_start;
	BYTE *syc_ptr_end;
	
	struct flyServiceHal_struct_info *pFlyServiceHalInfo = (struct flyServiceHal_struct_info *)arg;
	
	BYTE xxxx[6] = {1,2,3,4,5,6};
	pFlyServiceHalInfo->serialReadBuffLen = 0;
	memset(pFlyServiceHalInfo->serialReadBuff, 0, sizeof(pFlyServiceHalInfo->serialReadBuff));
	
	DBG0(debugString("\nCentral hal read thread start");)
	while (!pFlyServiceHalInfo->bKillCentralHalReadThread)
	{
#ifdef DEBUG_TPMS
		if (pFlyServiceHalInfo->flySerialHal_fd > 0)
		{
			ret = serial_read(pFlyServiceHalInfo->flySerialHal_fd, recvBuf, 1024);
			if (ret > 0)
			{
				DBG0(debugBuf("\nservicehal recv serial:", recvBuf,ret);)
				OnCommRecv(pFlyServiceHalInfo, recvBuf,ret);
			}
			else
				DBG0(debugOneData("\nservice hal serial ret :", ret);)
		}
		else
			sleep(100);
#else 
	debugOneData("\nxxxxxxxxxxxxxxx:", xxxxxxxxxxxxxxx);
	sleep(100);
#endif
	}
	
	pFlyServiceHalInfo->bKillCentralHalReadThread = TRUE;
	DBG0(debugString("\nCentral hal read thread exit");)
	return NULL;
}

INT flyOpenDevice(void)
{
	INT res;
	pthread_t thread_id;
	INT ret = HAL_ERROR_RETURN_FD;

	DBG0(debugString("FlyService hal open!\n");)

	 //打开硬件设备   
#ifdef DEBUG_TPMS	 
	pFlyServiceHalInfo->flySerialHal_fd = serial_open();
	if (pFlyServiceHalInfo->flySerialHal_fd <= 0)
	{
		return ret;
	}
#endif

	pFlyServiceHalInfo->bKillCentralHalReadThread = FALSE;
	res = pthread_create(&thread_id,NULL,centralHalReadThread,pFlyServiceHalInfo);
	if(res != 0) 
	{
		DBG0(debugString("\n create central Read Thread error");)
		return ret;
	}

	ret = HAL_SERVICEHAL_RETURN_FD;
	xxxxxxxxxxxxxxx++;
	return ret;
 }
 
void flyInitDeviceStruct(void)
{
	pFlyServiceHalInfo = (struct flyServiceHal_struct_info *)malloc(sizeof(struct flyServiceHal_struct_info));
	if (pFlyServiceHalInfo == NULL)
	{
		return;
	}
	memset(pFlyServiceHalInfo,0,sizeof(struct flyServiceHal_struct_info));
		
	allInOneInit();
	pFlyAllInOneInfo->pMemory_Share_Common->processServialHal._p_msgWriteToSerial = msgWriteToSerial;
	debugOneData("service msgWriteToSerial addr:", (UINT32)msgWriteToSerial);
	
	//初始化共享区
	paraRead();
	
	DBG0(debugString("\nservice hal init\n");)
	DBG0(debugString(__TIME__);)
	DBG0(debugString(__DATE__);)
	DBG0(debugString(" \n");)
	
	xxxxxxxxxxxxxxx++;
}
 
INT flyReadData(BYTE *buf, UINT len)
{
	UINT16 dwRead;
	DBG1(debugBuf("\nServial-HAL return  bytes Start:", buf,1);)
	dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	DBG1(debugBuf("\nServial-HAL return  bytes to User:", buf,dwRead);)
	return dwRead;
}
void flyDestroyDeviceStruct(void)
{
	allInOneDeinit();

	free(pFlyServiceHalInfo);
	pFlyServiceHalInfo = NULL;
}

 INT flyCloseDevice(void)
 {
#ifdef DEBUG_TPMS
	 if (pFlyServiceHalInfo->flySerialHal_fd > 0)
	 {
		if (!serial_close(pFlyServiceHalInfo->flySerialHal_fd))
		 {
#endif
			 return 0;		
#ifdef DEBUG_TPMS			 
		 }
	 }
#endif

	return -1;
	
 }
 
void flyCommandProcessor(BYTE *buf, UINT len)
{
	DBG0(debugBuf("\nService-HAL RecJNI:",buf,len);)

	DealRightDataProcessor(&buf[3], buf[2]-1);
}
