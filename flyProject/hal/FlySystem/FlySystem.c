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

#include <hardware_legacy/power.h>

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_SYSTEM
#define LOCAL_HAL_NAME		"flySystem Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"

#include "FlySystem.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"


struct flysystem_struct_info *pFlySystemInfo = NULL;

static void LCD_LCDBright_PWM(struct flysystem_struct_info *pFlySystemInfo,BYTE Duty);
static void returnSystemStandbyStatus(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn);
static void controlToMCUStandbyStatus(struct flysystem_struct_info *pFlySystemInfo,BOOL bStandby);

static void controlToMCULCDLightEn(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn);
static void FlySystemInfoProcessor(struct flysystem_struct_info *pFlySystemInfo, BYTE *p, UINT len);
static void flyAudioReturnToUser(struct flysystem_struct_info *pFlySystemInfo, BYTE *buf, UINT16 len);
static void systemControlPower(struct flysystem_struct_info *pFlySystemInfo,BOOL bPower);
static void returnSystembHaveTV(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn);
static void returnSystembHaveTPMS(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn);

static void actualLCDIdle(void)
{
	BOOL bOn = FALSE;
	BYTE buff[] = {SHARE_MEMORY_SYSTEM,MSG_SYSTEM_CON_LCDIDLE,0};

	if (pFlySystemInfo->bControlLCDIdleNormal
		&& pFlySystemInfo->bControlLCDIdleBackVideo)//基本法则
	{
		bOn = TRUE;
	}

	if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveRecMCUACCOff
		|| pFlyAllInOneInfo->pMemory_Share_Common->bNeedWinCEPowerOff
		|| pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg
		|| pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg)//通用法则
	{
		bOn = FALSE;
	}
	else if (pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow && pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn)
	{
		bOn = TRUE;
	}
	else if (pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)
	{
		bOn = FALSE;
	}
	else if (0 == pFlySystemInfo->SystemCarbodyInfo.curLightBrightDuty)
	{
		bOn = FALSE;
	}

	if (!pFlySystemInfo->bControlLCDIdleBackVideo)//临时处置
	{
		bOn = FALSE;
	}

	if (bOn)
	{
		buff[2] = 1;
		writeDataToHardware(buff, sizeof(buff));

		controlToMCULCDLightEn(pFlySystemInfo,TRUE);

		DBG0(debugString("\nFlyAudio System LCDIdle On");)
	}
	else
	{
		if (!pFlyAllInOneInfo->pMemory_Share_Common->bACCOffLightOn)
		{
			buff[2] = 0;
			writeDataToHardware(buff, sizeof(buff));

			controlToMCULCDLightEn(pFlySystemInfo,FALSE);

			DBG0(debugString("\nFlyAudio System LCDIdle Off");)
		}
	}
}

static void LCD_Idle_Normal(BOOL bOn)
{
	if (bOn)
	{
		pFlySystemInfo->bControlLCDIdleNormal = TRUE;
	}
	else
	{
		pFlySystemInfo->bControlLCDIdleNormal = FALSE;
	}
	actualLCDIdle();
}

static void LCD_Idle_Back(BOOL bOn)
{
	if (bOn)
	{
		pFlySystemInfo->bControlLCDIdleBackVideo = TRUE;
	}
	else
	{
		pFlySystemInfo->bControlLCDIdleBackVideo = FALSE;
	}
	actualLCDIdle();
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	DBG1(debugBuf("\nSystemHAL read from hardware:",buf,length);)
	if (MSG_SYSTEM_TRANS_MCU == buf[0])
	{
		if (0x00 == buf[1])
		{
			FlySystemInfoProcessor(pFlySystemInfo, &buf[2], length-2);	
		}
	}
	else if (MSG_SYSTEM_TRANS_NORMAL == buf[0])
	{
		if (MSG_SYSTEM_RES_SUSPEND_RESUME == buf[1])
		{
			if (0x01 == buf[2])//唤醒回来的恢复工作
			{
				systemControlPower(pFlySystemInfo,TRUE);
				//fly_set_screen_state(1);
				//debugString("\nsystem fly_set_screen_state 1");
				pFlyAllInOneInfo->pMemory_Share_Common->bKeyDemoMode = FALSE;

				resetPowerUp();

				//低功耗和待机的关系
				pFlySystemInfo->bStandbyStatusWithACCOff = FALSE;
				pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusWithACCOff = FALSE;

				pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus
					= pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusLast;
				pFlySystemInfo->bStandbyStatus
					= pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus;

				returnSystemStandbyStatus(pFlySystemInfo
					,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);
				controlToMCUStandbyStatus(pFlySystemInfo
					,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);

				Sleep(1000);
				LCD_LCDBright_PWM(pFlySystemInfo,pFlySystemInfo->SystemCarbodyInfo.curLightBrightDuty);
				actualLCDIdle();
				
				debugOneData("\nyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy",pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);
				returnSystembHaveTV(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule);
				returnSystembHaveTPMS(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule);
				
				pFlySystemInfo->bSendPingStart = FALSE;
				pFlyAllInOneInfo->pMemory_Share_Common->bNeedWinCEPowerOff = FALSE;
				pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg = FALSE;
				PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				
				ipcStartEvent(EVENT_AUTO_CLR_RESUME_ID);
				debugString("\nread Power On Pre Proc Done!");
			}
			else if (0x00 == buf[2])
			{
				//debugString("\nsystem fly_set_screen_state 0");
				//fly_set_screen_state(0);
			}
		}
		else
		{
			flyAudioReturnToUser(pFlySystemInfo,&buf[1],length-1);
		}
	}
}

UINT32 checkShellBaby(UINT32 iRandom)
{
	iRandom = iRandom * 37211237;
	iRandom = iRandom + 95277259;
	iRandom = iRandom * 16300361;
	return iRandom;
}

//old
//const BYTE LCD_LED_NIGHT_PARA[4]={0,35,45,55};	//20100411_
//const BYTE LCD_LED_DAY_PARA[4]={0,60,71,100};	//20100411_passed

const BYTE LCD_LED_NIGHT_PARA[4]={0,89,115,140};	//20100411_
const BYTE LCD_LED_DAY_PARA[4]={0,153,181,230};	//20100411_passed

//温敏电阻转换
const UINT RTCalculationTable[]=
{
	1656084,1195595,871853,641890,476929,
	357480,270209,205899,158117,122334,95334,
	74812,59105,47000,37611,30283,24528,19982,
	16370,13485,11168,9297,7780,6542,5529,4694,
	4005,3432,2955,2555,2219,1935,1695,1490
};
const INT RTTemperature[]=
{
	-40,-35,-30,-25,-20,-15,-10,-5,0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125
};
static INT RTExchangeToTemperature(UINT iAD)
{
	UINT i;
	UINT iRT;
	iRT = (10000*iAD)/(1023-iAD);//上拉10K，下拉温敏电阻，10位AD

	if (iRT > RTCalculationTable[0])
	{
		return RTTemperature[0];
	}
	for (i = 0;i < sizeof(RTCalculationTable) - 1;i++)
	{
		if (iRT <= RTCalculationTable[i] && iRT >= RTCalculationTable[i+1])
		{
			return RTTemperature[i] + (INT)((double)(RTCalculationTable[i] - iRT)/(RTCalculationTable[i]-RTCalculationTable[i+1])*5);
		}
	}
	return RTTemperature[sizeof(RTTemperature-1)];
}

static void Fan_On(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {SHARE_MEMORY_SYSTEM,MSG_SYSTEM_CON_FAN,1};
	writeDataToHardware(buff, sizeof(buff));
}

static void Fan_Off(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {SHARE_MEMORY_SYSTEM,MSG_SYSTEM_CON_FAN,0};
	writeDataToHardware(buff, sizeof(buff));
}

static void LCD_LCDBright_PWM(struct flysystem_struct_info *pFlySystemInfo,BYTE Duty)
{
	BYTE buff[] = {SHARE_MEMORY_SYSTEM,MSG_SYSTEM_CON_LCDPWM,255};
		
	buff[2] = 255 - Duty;
	
	writeDataToHardware(buff, sizeof(buff));

	DBG0(debugOneData("\nFlyAudio System LCD Set PWM",Duty);)
}

void printfHowLongTime(struct flysystem_struct_info *pFlySystemInfo)
{
	ULONG timeNow;
	ULONG timeLast;
	timeNow = GetTickCount();
	timeLast = timeNow - pFlySystemInfo->iProcACCOffTime;
	pFlySystemInfo->iProcACCOffTime = timeNow;
	DBG0(debugOneData("\nSystem Time Last:",timeLast);)
}

/********************************************************************************
**函数名称：
**函数功能：返回数据给用户
**函数参数：
**返 回 值：
**********************************************************************************/
static void flyAudioReturnToUser(struct flysystem_struct_info *pFlySystemInfo, BYTE *buf, UINT16 len)
{
	UINT dwLength;

	DBG1(debugBuf("\nSystem-HAL ToJNI:",buf,len);)

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nSYSTEM-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nSYSTEM-HAL write  bytes to User Error:", buf,len);)
	}
}
 
static void returnSystemPowerMode(struct flysystem_struct_info *pFlySystemInfo,BOOL bPower)
{
	BYTE buff[] = {0x01,0x00};
	if (bPower)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystembInit(struct flysystem_struct_info *pFlySystemInfo,BOOL bInit)
{
	BYTE buff[] = {0x02,0x00};
	if (bInit)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemBackDetectEnable(struct flysystem_struct_info *pFlySystemInfo,BOOL bEnable)
{
	BYTE buff[] = {0x10,0x00};

	if (bEnable)
	{
		buff[1] = 0x01;
	} 


	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnLightDetectEnable(struct flysystem_struct_info *pFlySystemInfo,BOOL bEnable)
{
	BYTE buff[] = {0x11,0x00};

	if (bEnable)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemLCDBright(struct flysystem_struct_info *pFlySystemInfo,BYTE iBright)
{
	BYTE buff[] = {0x14,0x00};
	buff[1] = iBright;

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystembDayNight(struct flysystem_struct_info *pFlySystemInfo,BOOL bNight)
{
	BYTE buff[] = {0x12,0x00};
	if (bNight)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight)
	{
		returnSystemLCDBright(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight);
	}
	else
	{
		returnSystemLCDBright(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay);
	}
}

static void returnSystembHaveExtAmp(struct flysystem_struct_info *pFlySystemInfo,BOOL bHave)
{
	BYTE buff[] = {0x13,0x00};
	if (bHave)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemPanelBright(struct flysystem_struct_info *pFlySystemInfo,BYTE iBright)
{
	BYTE buff[] = {0x15,0x00};
	buff[1] = iBright;

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemSteelwheelbOn(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x16,0x00};
	if (bOn)
	{
		buff[1] = 0x01;
	}
	
	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemRadioID(struct flysystem_struct_info *pFlySystemInfo,BYTE iID)
{
	BYTE buff[] = {0x17,0x00};
	buff[1] = iID;

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemDemoMode(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x19,0x00};
	if (bOn)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemSteelwheelUseStudy(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x1A,0x00};
	if (bOn)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemSteelStudyStatus(struct flysystem_struct_info *pFlySystemInfo,BYTE iStatus)
{
	BYTE buff[] = {0x1B,0x00};
	buff[1] = iStatus;

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnDVDRegionCode(struct flysystem_struct_info *pFlySystemInfo,BYTE iRegionCode)
{
	BYTE buff[2];
	buff[0] = 0x1E;
	buff[1] = iRegionCode;
	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemBreakStatus(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x20,0x00};

	if (pFlyAllInOneInfo->pMemory_Share_Common->bBreakStatusIO
		|| pFlyAllInOneInfo->pMemory_Share_Common->bBreakStatusBUS)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemBackStatus(struct flysystem_struct_info *pFlySystemInfo,BOOL bBack,BOOL bVideo)
{
	BYTE buff[] = {0x21,0x00,0x00};

	if (bBack)
	{
		buff[1] = 0x01;
	}
	
	if (bVideo)
	{
		buff[2] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,3);
}

static void returnSystemBatteryVotege(struct flysystem_struct_info *pFlySystemInfo,BYTE iVoltage)
{
	BYTE buff[2];
	buff[0] = 0x22;
	buff[1] = iVoltage;
	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemSteelwheelName(struct flysystem_struct_info *pFlySystemInfo,volatile BYTE *p,UINT length)
{
	BYTE buff[1+256];
	buff[0] = 0x24;
	if (length)
	{
		memcpy(&buff[1],(void*)p,length);
		flyAudioReturnToUser(pFlySystemInfo,buff,1+length);
	}
}

static void returnSystembHaveTV(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x25,0x00};

	if (bOn)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystembHaveTPMS(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x26,0x00};

	if (bOn)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystembHave3G(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x27,0x00};

	if (bOn)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystembHaveAuxVideo(struct flysystem_struct_info *pFlySystemInfo,BOOL bHave)
{
	BYTE buff[] = {0x28,0x00};

	if (bHave)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemTouchTimeout(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x2E,0x00};

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

//static void returnSystemACCOn(struct flysystem_struct_info *pFlySystemInfo)
//{
//	BYTE buff[] = {0x30,0x00};
//
//	flyAudioReturnToUser(pFlySystemInfo,buff,2);
//}
//
//static void returnSystemACCOff(struct flysystem_struct_info *pFlySystemInfo)
//{
//	BYTE buff[] = {0x31,0x00};
//
//	flyAudioReturnToUser(pFlySystemInfo,buff,2);
//}

static void returnSystemStandbyStatus(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x32,0x00};

	if (bOn)
	{
		buff[1] = 0x01;
	}

	flyAudioReturnToUser(pFlySystemInfo,buff,2);
}

static void returnSystemMCUSoftwareVersion(struct flysystem_struct_info *pFlySystemInfo,volatile BYTE *p,UINT length)
{
	BYTE buff[1+MCU_SOFT_VERSION_MAX];
	buff[0] = 0x33;
	if (length)
	{
		memcpy(&buff[1],(BYTE*)p,length);
		flyAudioReturnToUser(pFlySystemInfo,buff,1+length);
	}
}

static void returnSystemDVDSoftwareVersion(struct flysystem_struct_info *pFlySystemInfo,volatile BYTE *p,UINT length)
{
	BYTE buff[1+32];
	buff[0] = 0x34;
	memcpy(&buff[1],(BYTE *)p,length);
	flyAudioReturnToUser(pFlySystemInfo,buff,1+length);
}

static void returnSystemBTSoftwareVersion(struct flysystem_struct_info *pFlySystemInfo,volatile BYTE *p,UINT length)
{
	BYTE buff[1+32];
	buff[0] = 0x35;
	memcpy(&buff[1],(BYTE *)p,length);
	flyAudioReturnToUser(pFlySystemInfo,buff,1+length);
}

static void systemWriteFile(struct flysystem_struct_info *pFlySystemInfo,BYTE *pData,UINT length)
{
	BYTE buff[32];
	BYTE *p;
	if (length+2 > 32)
	{
		p = (BYTE*)malloc(sizeof(BYTE)*(length+2));
	}
	else
	{
		p = buff;
	}
	p[0] = SHARE_MEMORY_SYSTEM;
	p[1] = MSG_SYSTEM_TRANS_MCU;
	memcpy(&p[2], pData, length);
	writeDataToHardware(p, length+2);
	if (length+2 > 32)
	{
		free(p);
	}
}

static void controlToMCUTestIIC(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x00,0xFE,0,1,2,3,4,5,6,7};

	systemWriteFile(pFlySystemInfo,buff,10);
}

static void systemControlPower(struct flysystem_struct_info *pFlySystemInfo,BOOL bPower)
{
	BYTE buff[] = {SHARE_MEMORY_SYSTEM,MSG_SYSTEM_CON_SUSPEND_RESUME,0};
	DBG0(debugOneData("\nFlyAudio System Control To MCU Power",bPower););

	if (bPower)
	{
		buff[2] = 0x01;
	}

	writeDataToHardware(buff, sizeof(buff));
}

static void controlToMCUPing(struct flysystem_struct_info *pFlySystemInfo,BOOL bWork)
{
	BYTE buff[] = {0x00,0x02,0x00};

	if (bWork)
	{
		buff[2] = 0x00;
	}
	else
	{
		buff[2] = 0x01;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUUserPing(struct flysystem_struct_info *pFlySystemInfo,BOOL bWork)
{
	BYTE buff[] = {0x00,0x02,0x10};

	if (bWork)
	{
		buff[2] = 0x11;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUReset(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x00,0x03,0x04};

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUDebug(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x00,0x03,0x05};

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCULightDetectEnable(struct flysystem_struct_info *pFlySystemInfo,BOOL bEnable)
{
	BYTE buff[] = {0x00,0x11,0x00};
	if (bEnable)
	{
		buff[2] = 0x01;
	} 

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUbDayNight(struct flysystem_struct_info *pFlySystemInfo,BOOL bNight)
{
	BYTE buff[] = {0x00,0x12,0x00};
	if (bNight)
	{
		buff[2] = 0x01;
	} 

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUbHaveExtAMP(struct flysystem_struct_info *pFlySystemInfo,BOOL bHave)
{
	BYTE buff[] = {0x00,0x13,0x00};
	if (bHave)
	{
		buff[2] = 0x01;
	} 

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUPanelLight(struct flysystem_struct_info *pFlySystemInfo,BYTE iLight)
{
	BYTE buff[] = {0x00,0x15,0x00};

	buff[2] = iLight;

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUResetToFactory(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x00,0x18,0x00};

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUKeyDemoMode(struct flysystem_struct_info *pFlySystemInfo,BOOL bEnable)
{
	BYTE buff[] = {0x00,0x19,0x00};

	if (bEnable)
	{
		buff[2] = 0x01;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUStandbyStatus(struct flysystem_struct_info *pFlySystemInfo,BOOL bStandby)
{
	BYTE buff[] = {0x00,0x32,0x00};

	if (bStandby)
	{
		buff[2] = 0x01;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUAMPOn(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x00,0x91,0x00};

	if (bOn)
	{
		buff[2] = 0x01;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCULCDLightEn(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x00,0x94,0x00};

	if (bOn)
	{
		buff[2] = 1;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUiPodPower(struct flysystem_struct_info *pFlySystemInfo,BOOL bOn)
{
	BYTE buff[] = {0x00,0x95,0x00};

	if (bOn)
	{
		buff[2] = 1;
	}

	systemWriteFile(pFlySystemInfo,buff,3);
}

static void controlToMCUHowLongToPowerOn(struct flysystem_struct_info *pFlySystemInfo,UINT32 iTime)
{
	BYTE buff[] = {0x00,0x98,0x00,0x00,0x00,0x00};

	buff[5] = (BYTE)(iTime);
	iTime = iTime >> 8;
	buff[4] = (BYTE)(iTime);
	iTime = iTime >> 8;
	buff[3] = (BYTE)(iTime);
	iTime = iTime >> 8;
	buff[2] = (BYTE)(iTime);

	systemWriteFile(pFlySystemInfo,buff,6);
}

void controlToMCUPowerToVeryLowOff(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x00,0x99};

	systemWriteFile(pFlySystemInfo,buff,2);
}

static void controlToMCURandom(struct flysystem_struct_info *pFlySystemInfo,UINT32 iRandom)
{
	BYTE buff[] = {0x00,0x96,0x00,0x00,0x00,0x00};

	buff[5] = iRandom;
	iRandom = iRandom >> 8;
	buff[4] = iRandom;
	iRandom = iRandom >> 8;
	buff[3] = iRandom;
	iRandom = iRandom >> 8;
	buff[2] = iRandom;

	systemWriteFile(pFlySystemInfo,buff,6);
}

static void controlHostSecondAction(struct flysystem_struct_info *pFlySystemInfo,BYTE iWhatAction)
{
	ipcStartEvent(EVENT_AUTO_CLR_SUSPEND_ID);
	Sleep(1000);

	debugOneData("\n############################12@@@@@@@@@@@@@@@",iWhatAction);

	if (CONTROL_TO_SLEEP == iWhatAction)
	{
		systemControlPower(pFlySystemInfo,FALSE);
	}
	else if (CONTROL_TO_RESET == iWhatAction)
	{
		controlToMCUReset(pFlySystemInfo);
	}
	else if (CONTROL_TO_RESET_FACTORY == iWhatAction)
	{
		controlToMCUResetToFactory(pFlySystemInfo);
	}
}

static void controlHostFirstAction(struct flysystem_struct_info *pFlySystemInfo,BYTE iWhatAction)
{
	pFlyAllInOneInfo->pMemory_Share_Common->iControlUserAction = iWhatAction;
#if !SEND_ACC_OFF_TO_USER
	controlHostSecondAction(pFlySystemInfo,iWhatAction);
	return;
#endif

	BYTE buff[] = {0x2C,0x00};

	buff[1] = iWhatAction;

	flyAudioReturnToUser(pFlySystemInfo,buff,sizeof(buff));

	ipcStartEvent(EVENT_GLOBAL_REQ_USER_ACTION);

	debugOneData("\n############################11@@@@@@@@@@@@@@@",iWhatAction);
}

static void controlHostACCOn(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[] = {0x2D,0x00};

	debugOneData("\n############################14@@@@@@@@@@@@@@@",buff[1]);
	flyAudioReturnToUser(pFlySystemInfo,buff,sizeof(buff));

	ipcStartEvent(EVENT_GLOBAL_REQ_USER_ACCON);
}

static void processFanControlByTemperature(struct flysystem_struct_info *pFlySystemInfo,INT iTemperature)
{
	if (pFlyAllInOneInfo->pMemory_Share_Common->bKeyDemoMode)
	{
		if (!pFlySystemInfo->SystemCarbodyInfo.bFanControlOn)
		{
			pFlySystemInfo->SystemCarbodyInfo.bFanControlOn = TRUE;
			Fan_On(pFlySystemInfo);
		}
	}
	else if (pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)
	{
		if (pFlySystemInfo->SystemCarbodyInfo.bFanControlOn)
		{
			pFlySystemInfo->SystemCarbodyInfo.bFanControlOn = FALSE;
			Fan_Off(pFlySystemInfo);
		}
	}
	else if (pFlyAllInOneInfo->pMemory_Share_Common->bMute)
	{
		if (pFlySystemInfo->SystemCarbodyInfo.bFanControlOn)
		{
			pFlySystemInfo->SystemCarbodyInfo.bFanControlOn = FALSE;
			Fan_Off(pFlySystemInfo);
		}
	}
	else if (pFlyAllInOneInfo->pMemory_Share_Common->iBatteryVoltage < 120)
	{
		if (pFlySystemInfo->SystemCarbodyInfo.bFanControlOn)
		{
			pFlySystemInfo->SystemCarbodyInfo.bFanControlOn = FALSE;
			Fan_Off(pFlySystemInfo);
		}
	}
	else if (iTemperature > 55)
	{
		if (!pFlySystemInfo->SystemCarbodyInfo.bFanControlOn)
		{
			pFlySystemInfo->SystemCarbodyInfo.bFanControlOn = TRUE;
			Fan_On(pFlySystemInfo);
		}
	}
	else if (iTemperature < 50)
	{
		if (pFlySystemInfo->SystemCarbodyInfo.bFanControlOn)
		{
			pFlySystemInfo->SystemCarbodyInfo.bFanControlOn = FALSE;
			Fan_Off(pFlySystemInfo);
		}
	}
}

static void FlySystemInfoProcessor(struct flysystem_struct_info *pFlySystemInfo, BYTE *buff, UINT len)
{
	if (len > 0)
	{
		switch (buff[0])
		{
			case 0x00:
				break;
				
			case 0x01:
				if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveRecMCUACCOff)
				{
					break;
				}
				pFlyAllInOneInfo->pMemory_Share_Common->bHaveRecMCUACCOff = TRUE;

				actualLCDIdle();

				pFlyAllInOneInfo->pMemory_Share_Common->bNeedWinCEPowerOff = TRUE;
				pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg = TRUE;
				pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat = eLEDPreSleepTimeOut;
				pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhatSub = 0;
				pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnTime = GetTickCount();

				pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow = FALSE;//退出倒车
				ipcStartEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);
				ipcStartEvent(EVENT_GLOBAL_BACKDETECT_RETURN_ID);
				ipcStartEvent(EVENT_GLOBAL_BACKDETECT_CHANGE_ID);

				debugString("\nread ACC OFF Pre Proc Done!");
				PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				break;
				
			//case 0x08:
			//	//if (0x01 == buff[1])
			//	//{
			//	//	returnSystemACCOn(pFlySystemInfo);
			//	//	DBG2(debugString("\nSystem Return To WinCE ACC On!!!!!!!!");)
			//	//	printfHowLongTime(pFlySystemInfo);
			//	//}
			//	break;
			//	
			case 0x09:
				if (0x00 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusWithACCOff = TRUE;
				}
				else if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusWithACCOff = FALSE;
				}
				DBG0(debugOneData("\nSystem Rec MCU PreACC",buff[1]);)
				printfHowLongTime(pFlySystemInfo);
				ipcStartEvent(EVENT_GLOBAL_KEY_STANDBY_ID);
				break;
				
			case 0x11:
				if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable = TRUE;
				}
				else if (0x00 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable = FALSE;
				}
				returnLightDetectEnable(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable);
			
				PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				break;
				
			case 0x12:
				if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight = TRUE;
				}
				else if (0x00 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight = FALSE;
				}
				returnSystembDayNight(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight);
				PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				break;
				
			case 0x13:
				if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bHaveExtAmplifierGMC = TRUE;
				} 
				else
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bHaveExtAmplifierGMC = FALSE;
				}
				returnSystembHaveExtAmp(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bHaveExtAmplifierGMC);
				break;
				
			case 0x15:
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iPanelLightBright = buff[1];
				returnSystemPanelBright(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iPanelLightBright);
				PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				break;
				
			case 0x20:
				if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bBreakStatusBUS = TRUE;
				} 
				else if (0x00 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bBreakStatusBUS = FALSE;
				}
				ipcStartEvent(EVENT_GLOBAL_BREAKDETECT_CHANGE_ID);
				break;
				
			case 0x21:
				if (pFlyAllInOneInfo->pMemory_Share_Common->bNeedWinCEPowerOff)
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow = FALSE;
					ipcStartEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);
					ipcStartEvent(EVENT_GLOBAL_BACKDETECT_RETURN_ID);
				}
				else if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow = TRUE;
					ipcStartEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);

					if (0x00 != pFlyAllInOneInfo->pMemory_Share_Common->eVideoInput && 0x80 != pFlyAllInOneInfo->pMemory_Share_Common->eVideoInput)
					{
						LCD_Idle_Back(FALSE);
						setLightOnBackVideoWithDelay(618);
					}
				} 
				else if (0x00 == buff[1])
				{
					if (BACK == pFlyAllInOneInfo->pMemory_Share_Common->eVideoInput
						&& pFlyAllInOneInfo->pMemory_Share_Common->bHaveEverBackVideoOn)//解决退出倒车视频时画面烂掉的问题
					{
						LCD_Idle_Back(FALSE);
						setLightOnBackVideoWithDelay(618);
					}

					pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow = FALSE;
					ipcStartEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);
					ipcStartEvent(EVENT_GLOBAL_BACKDETECT_RETURN_ID);
				}
				ipcStartEvent(EVENT_GLOBAL_BACKDETECT_CHANGE_ID);
				break;
				
			case 0x22:
				returnSystemBatteryVotege(pFlySystemInfo,buff[1]);
				break;
				
			case 0x23:
				pFlyAllInOneInfo->pMemory_Share_Common->iHostTemperature = RTExchangeToTemperature(buff[1]*256+buff[2]);
				processFanControlByTemperature(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->iHostTemperature);
				break;
				
			case 0x41:case 0x42:case 0x43:
				flyAudioReturnToUser(pFlySystemInfo,buff,len);
				break;
				
			case 0x95:
				if (0x01 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bExtPhoneStatusBUS = TRUE;
				}
				else if (0x00 == buff[1])
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bExtPhoneStatusBUS = FALSE;
				}
				ipcStartEvent(EVENT_GLOBAL_PHONEDETECT_CHANGE_ID);
				break;
				
			case 0x96:
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec = buff[1];
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec = pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec << 8;
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec += buff[2];
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec = pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec << 8;
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec += buff[3];
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec = pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec << 8;
				pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec += buff[4];
				if (pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec == checkShellBaby(pFlySystemInfo->SystemCarbodyInfo.iShellBabySend))
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bCheckShellBabyError = FALSE;
					//DBG1(debugString("\nFlyAudio System Shell Baby OK");)
				}
				else
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bCheckShellBabyError = TRUE;
					//DBG1(debugString("\nFlyAudio System Shell Baby Error");)
				}
				break;

			case 0xFE:
				if (!memcmp(&buff[1],pFlySystemInfo->iIICTestSendData,8))
				{
					pFlySystemInfo->bIICTestHaveSend = FALSE;
					pFlySystemInfo->iIICTestRecTime = 0;
				}
				break;
				
			case 0x97:
				pFlyAllInOneInfo->pMemory_Share_Common->bSilencePowerUp = TRUE;
				ipcStartEvent(EVENT_GLOBAL_SILENCE_POWER_UP_ID);
				resetPowerProcOnRecMCUWakeup();
				break;
				
			default:
				break;
		}
	}
}

BOOL fIICTestFunc(struct flysystem_struct_info *pFlySystemInfo)
{
	BYTE buff[10];

	if (!pFlySystemInfo->bIICTestHaveSend)
	{
		pFlySystemInfo->bIICTestHaveSend = TRUE;
		pFlySystemInfo->iIICTestRecTime = GetTickCount();
		buff[0] = 0x00;
		buff[1] = 0xFE;
		pFlySystemInfo->iIICTestSendData[0]++;
		memcpy(&buff[2],pFlySystemInfo->iIICTestSendData,8);
		systemWriteFile(pFlySystemInfo,buff,2+8);
	}
	else if (pFlySystemInfo->iIICTestRecTime)
	{
		if (GetTickCount() - pFlySystemInfo->iIICTestRecTime > 5000)
		{
			return FALSE;
		}
	}

	return TRUE;
}

void setLightOnBackVideoWithDelay(ULONG iDelayLong)
{
	if (iDelayLong)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTime = GetTickCount();
		pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTimeLong = iDelayLong;
	}
	ipcStartEvent(EVENT_GLOBAL_BACK_DELAY_LIGHT_ON_ID);
}

void *ThreadMain(void *arg)
{
	INT ret = 0;
	struct flysystem_struct_info *pFlySystemInfo = (struct flysystem_struct_info *)arg;

	pFlySystemInfo->bFlyMainThreadRunning = TRUE;
	while (!pFlySystemInfo->bKillDispatchFlyMainThread)
	{
		if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg
			|| pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg
			|| pFlyAllInOneInfo->pMemory_Share_Common->bMCUIICCommTest
			|| pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTime)
		{
			ret = WaitSignedTimeOut(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain,100);
		//	DBG2(debugString("\nFlyAudio System Run Due Break Or Phone Proc");)
		}
		else
		{
			ret = WaitSignedTimeOut(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain,3000);
		}

		if (pFlyAllInOneInfo->pMemory_Share_Common->bMCUIICCommTest)
		{
			if (!fIICTestFunc(pFlySystemInfo))
			{
				debugString("\nMCU IIC Error!!!");
			}
		}
		
		if (pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat)//应用程序处理超时
		{
			if (GetTickCount() - pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnTime > 20000)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iLEDTestFlashCount = pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat;
				pFlyAllInOneInfo->pMemory_Share_Common->iLEDTestFlashCountSub = pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhatSub;
				pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat = 0;
				ipcStartEvent(EVENT_GLOBAL_ERROR_LEDFLASH_ID);
			}
		}
		
		//DBG0(debugString("FlySystem ThreadMain running-----\n");)
		
		if (ipcWhatEventOn(EVENT_TOUCH_TIMEOUT_RETURN_ID))
		{
			ipcClearEvent(EVENT_TOUCH_TIMEOUT_RETURN_ID);
			returnSystemTouchTimeout(pFlySystemInfo);
		}

		if (!pFlySystemInfo->bSendPingStart)//Ping
		{
			DBG0(debugString("FlySystem start Ping\n"););

			pFlySystemInfo->bSendPingStart = TRUE;
			controlToMCUPing(pFlySystemInfo,FALSE);

			controlHostACCOn(pFlySystemInfo);

			controlToMCULightDetectEnable(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable);
			if (!pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable)
			{
				controlToMCUbDayNight(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight);
			}
			controlToMCUbHaveExtAMP(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bHaveExtAmplifierGMC);
			controlToMCUPanelLight(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iPanelLightBright);
			controlToMCUStandbyStatus(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);
		}
		else if (GetTickCount() - pFlySystemInfo->iSendPingTimer > 3000)
		{
			pFlySystemInfo->iSendPingTimer = GetTickCount();
			controlToMCUPing(pFlySystemInfo,TRUE);
		}

		if (GetTickCount() - pFlySystemInfo->SystemCarbodyInfo.iSendShellBabyTime >= CHECK_SHELL_BABY_INNER_TIME)
		{
			if (pFlySystemInfo->SystemCarbodyInfo.iShellBabySend)
			{
				if (pFlySystemInfo->SystemCarbodyInfo.iShellBabyRec == checkShellBaby(pFlySystemInfo->SystemCarbodyInfo.iShellBabySend))
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bCheckShellBabyError = FALSE;
				}
				else
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bCheckShellBabyError = TRUE;
				}
			}
			pFlySystemInfo->SystemCarbodyInfo.iSendShellBabyTime = GetTickCount();
			pFlySystemInfo->SystemCarbodyInfo.iShellBabySend = rand();
			pFlySystemInfo->SystemCarbodyInfo.iShellBabySend = pFlySystemInfo->SystemCarbodyInfo.iShellBabySend << 16;
			pFlySystemInfo->SystemCarbodyInfo.iShellBabySend += rand();
			controlToMCURandom(pFlySystemInfo,pFlySystemInfo->SystemCarbodyInfo.iShellBabySend);
		}

		if (ETIMEDOUT == ret)
		{
		}
		
		if (pFlySystemInfo->bUserPowerUp)
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDDriver
				!= pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDDriver
					= pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser;
				ipcStartEvent(EVENT_GLOBAL_RADIO_CHANGE_ID);
			}
		}

		if (ipcWhatEventOn(EVENT_GLOBAL_KEY_STANDBY_ID))//待机事件
		{
			ipcClearEvent(EVENT_GLOBAL_KEY_STANDBY_ID);
			if (pFlySystemInfo->bStandbyStatusWithACCOff != pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusWithACCOff)
			{
				pFlySystemInfo->bStandbyStatusWithACCOff = pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusWithACCOff;
				if (pFlySystemInfo->bStandbyStatusWithACCOff)
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusLast = pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus;
					pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus = TRUE;
				}
				else
				{
					pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus = pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusLast;
				}
			}
			else if (!pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusWithACCOff)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus =  !pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus;
				pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatusLast = pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus;
			}

			if (pFlySystemInfo->bStandbyStatus != pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)
			{
				pFlySystemInfo->bStandbyStatus = pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus;

				returnSystemStandbyStatus(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);
				controlToMCUStandbyStatus(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);
				DBG0(debugOneData("\nFlyAudio System Standby",pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);)
				
				actualLCDIdle();

				ipcStartEvent(EVENT_AUTO_CLR_STANDBY_ID);
			}
		}

		if (pFlySystemInfo->bUserPowerUp)//等到初始化命令之后才返回
		{
			if (ipcWhatEventOn(EVENT_GLOBAL_TVBOX_EXIST_ID))
			{
				ipcClearEvent(EVENT_GLOBAL_TVBOX_EXIST_ID);
				if(pFlySystemInfo->bHaveTVModule != pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule)
				{
					debugString("\nHave TVTVTVTVTVTVTVTVTVTVTVTV");
					pFlySystemInfo->bHaveTVModule = pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule;
					returnSystembHaveTV(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule);
				}
			}
			if (ipcWhatEventOn(EVENT_GLOBAL_TPMSBOX_EXIST_ID))
			{
				ipcClearEvent(EVENT_GLOBAL_TPMSBOX_EXIST_ID);
				if(pFlySystemInfo->bHaveTPMSModule != pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule)
				{
					pFlySystemInfo->bHaveTPMSModule = pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule;
					returnSystembHaveTPMS(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule);
				}
			}
		}

		if (ipcWhatEventOn(EVENT_GLOBAL_BACKDETECT_RETURN_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_BACKDETECT_RETURN_ID);
			if (pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow)//有视频先返回
			{
				returnSystemBackStatus(pFlySystemInfo
					,pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow
					,pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn);
			}

			setLightOnBackVideoWithDelay(0);
	
			if (!pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow)//无视频后返回
			{
				returnSystemBackStatus(pFlySystemInfo,FALSE,FALSE);
			}
		}		

		if (ipcWhatEventOn(EVENT_GLOBAL_BACK_DELAY_LIGHT_ON_ID))
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTime
				&& (GetTickCount() - pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTime < pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTimeLong))
			{
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iBackDelayLightOnTime = 0;
				ipcClearEvent(EVENT_GLOBAL_BACK_DELAY_LIGHT_ON_ID);

				LCD_Idle_Back(TRUE);
			}
		}

		if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight)//设置亮度
		{
			pFlySystemInfo->SystemCarbodyInfo.preLightBrightDuty = LCD_LED_NIGHT_PARA[pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight];
		}
		else
		{
			pFlySystemInfo->SystemCarbodyInfo.preLightBrightDuty = LCD_LED_DAY_PARA[pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay];
		}

		if (pFlyAllInOneInfo->pMemory_Share_Common->bSilencePowerUp)
		{
			pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty = 0;
		}
		else if (pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable
			&& pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn)//倒车且有视频
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight)
			{
				pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty = LCD_LED_NIGHT_PARA[3];
			}
			else
			{
				pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty = LCD_LED_DAY_PARA[3];
			}
		}
		else if (pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)//待机
		{
			pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty = 0;
		}
		else//正常时
		{
			pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty = pFlySystemInfo->SystemCarbodyInfo.preLightBrightDuty;
		}

		if (ipcWhatEventOn(EVENT_GLOBAL_FORCE_LCD_LIGHT_ON_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_FORCE_LCD_LIGHT_ON_ID);
			pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty = 100;
		}

		if (pFlySystemInfo->SystemCarbodyInfo.curLightBrightDuty != pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty)//设置亮度
		{
			pFlySystemInfo->SystemCarbodyInfo.curLightBrightDuty = pFlySystemInfo->SystemCarbodyInfo.tmpLightBrightDuty;

			LCD_LCDBright_PWM(pFlySystemInfo,pFlySystemInfo->SystemCarbodyInfo.curLightBrightDuty);
			actualLCDIdle();
		}

		if (pFlyAllInOneInfo->pMemory_Share_Common->bHostCompletePowerUp)
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg
				|| pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg)
			{
				resetPowerDown();
				pFlyAllInOneInfo->pMemory_Share_Common->iHowLongToRestart = resetReturnNextResetTime();

				if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg
					|| pFlyAllInOneInfo->pMemory_Share_Common->bRestoreRegeditToFactory)
				{
					ipcStartEvent(EVENT_AUTO_CLR_PARA_INIT_ID);
				}
				ipcStartEvent(EVENT_AUTO_CLR_PARA_WRITE_ID);

				if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg)
				{
					if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg)
					{
						DBG2(debugString("\n System Control To MCU ResetToFactory!!!!!!!!");)
							printfHowLongTime(pFlySystemInfo);
						pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat = eLEDControlMCUSleepTimeOut;
						pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhatSub = 0;
						pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnTime = GetTickCount();

						controlHostFirstAction(pFlySystemInfo,CONTROL_TO_RESET_FACTORY);
					}
					else
					{
						DBG2(debugString("\r\n System Control To MCU Reset!!!!!!!!");)
							printfHowLongTime(pFlySystemInfo);
						pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat = eLEDControlMCUSleepTimeOut;
						pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhatSub = 0;
						pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnTime = GetTickCount();

						controlHostFirstAction(pFlySystemInfo,CONTROL_TO_RESET);
					}
				}
				else if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg)
				{
					pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat = eLEDControlMCUSleepTimeOut;
					pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhatSub = 0;
					pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnTime = GetTickCount();

					controlHostFirstAction(pFlySystemInfo,CONTROL_TO_SLEEP);
				}
				pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg = FALSE;//处理完毕
				//pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg = FALSE;//处理完毕
				pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg = FALSE;//处理完毕
				pFlySystemInfo->bUserPowerUp = FALSE;
			}
		}
	}
	pFlySystemInfo->bFlyMainThreadRunning = FALSE;
	return NULL;
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	BOOL bNeedClear = TRUE;
	BOOL bNeedSetEvent = FALSE;

	switch (sourceEvent)
	{
	case EVENT_GLOBAL_RES_USER_ACTION:
		controlHostSecondAction(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->iControlUserAction);
		break;
	case EVENT_GLOBAL_USER_PING_START_ID:
		controlToMCUUserPing(pFlySystemInfo,FALSE);
		break;
	case EVENT_GLOBAL_USER_PING_WORK_ID:
		controlToMCUUserPing(pFlySystemInfo,TRUE);
		break;
	case EVENT_GLOBAL_FORCE_RESET_ID:
		controlHostFirstAction(pFlySystemInfo,CONTROL_TO_RESET);
		break;
	case EVENT_GLOBAL_RETURN_DVD_VERSION_ID:
		returnSystemDVDSoftwareVersion(pFlySystemInfo
			,pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion
			,pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersionLength);
		break;
	case EVENT_GLOBAL_RETURN_BT_VERSION_ID:
		returnSystemBTSoftwareVersion(pFlySystemInfo
			,pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersion
			,pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersionLength);
		break;
	case EVENT_GLOBAL_AUX_CHECK_RETURN_ID:
		returnSystembHaveAuxVideo(pFlySystemInfo
			,pFlyAllInOneInfo->pMemory_Share_Common->bAUXHaveVideo);
		break;
	case EVENT_GLOBAL_REMOTE_STUDY_RETURN_START_ID:
		returnSystemSteelStudyStatus(pFlySystemInfo,0x01);
		break;
	case EVENT_GLOBAL_REMOTE_STUDY_RETURN_WAIT_ID:
		returnSystemSteelStudyStatus(pFlySystemInfo,0x00);
		break;
	case EVENT_GLOBAL_REMOTE_STUDY_RETURN_FINISH_ID:
		returnSystemSteelStudyStatus(pFlySystemInfo,0x02);
		break;
	case EVENT_GLOBAL_INNER_AMP_ON_ID:
		controlToMCUAMPOn(pFlySystemInfo,TRUE);
		break;
	case EVENT_GLOBAL_BREAKDETECT_CHANGE_ID:
		returnSystemBreakStatus(pFlySystemInfo);
		break;
	case EVENT_GLOBAL_PHONEDETECT_CHANGE_ID:
		if (pFlyAllInOneInfo->pMemory_Share_Common->bExtPhoneStatusBUS || pFlyAllInOneInfo->pMemory_Share_Common->bExtPhoneStatusIO)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->iExtTelCallStatus = 0x03;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->iExtTelCallStatus = 0x00;
		}
		ipcStartEvent(EVENT_GLOBAL_PHONECALLSTATUS_CHANGE_ID);
		DBG2(debugOneData("\nSystem ExtPhone Status Change",pFlyAllInOneInfo->pMemory_Share_Common->iExtTelCallStatus);)
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
		PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
	}
}

static void DealRightDataProcessor(struct flysystem_struct_info *pFlySystemInfo,BYTE *buff, UINT16 len)
{
	switch (buff[0])
	{
	case 0x01:
		if (0x01 == buff[1])
		{
			ipcStartEvent(EVENT_GLOBAL_PANEL_KEY_USE_IT_ID);
			ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
			returnSystemSteelwheelName(pFlySystemInfo,
				pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName,
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength);

			returnSystemMCUSoftwareVersion(pFlySystemInfo
				,pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersion
				,pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersionLength);
			
			returnSystemBackDetectEnable(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable);
			returnLightDetectEnable(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bLightDetectEnable);
			returnSystembDayNight(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight);
			returnSystembHaveExtAmp(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bHaveExtAmplifierGMC);
			returnSystemPanelBright(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iPanelLightBright);
			returnSystemSteelwheelbOn(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bSteerWheelOn);
			returnSystemRadioID(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser);
			returnSystemDemoMode(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode);
			returnSystemSteelwheelUseStudy(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bRemoteUseStudyOn);
			returnDVDRegionCode(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode);
			returnSystemBreakStatus(pFlySystemInfo);
			if (pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable)
			{
				returnSystemBackStatus(pFlySystemInfo
					,pFlyAllInOneInfo->pMemory_Share_Common->bBackActiveNow
					,pFlyAllInOneInfo->pMemory_Share_Common->bBackVideoOn);
			}
			else
			{
				returnSystemBackStatus(pFlySystemInfo,FALSE,FALSE);
			}
			returnSystemBatteryVotege(pFlySystemInfo
				,pFlyAllInOneInfo->pMemory_Share_Common->iBatteryVoltage);
			//returnSystembHaveTV(pFlySystemInfo,FALSE);
			//returnSystembHaveTPMS(pFlySystemInfo,FALSE);
			//returnSystembHave3G(pFlySystemInfo,FALSE);



			returnSystemPowerMode(pFlySystemInfo,TRUE);
			returnSystembInit(pFlySystemInfo,TRUE);

			//controlToMCUPowerOn(pFlySystemInfo);

			returnSystemStandbyStatus(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);
			controlToMCUStandbyStatus(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus);

			//if (pFlyAllInOneInfo->pMemory_Share_Common->bNeedWinCEPowerOff)//和WinCE同步关机
			//{
			//	returnSystemACCOff(pFlySystemInfo);
			//}

			pFlySystemInfo->bUserPowerUp = TRUE;
			
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight)
			{
				returnSystemLCDBright(pFlySystemInfo,
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight);
			}
			else
			{
				returnSystemLCDBright(pFlySystemInfo,
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay);
			}
			
		}
		else
		{
			returnSystemPowerMode(pFlySystemInfo,FALSE);
		}

		pFlyAllInOneInfo->pMemory_Share_Common->bHostCompletePowerUp = TRUE;

		PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
		break;
		
	case 0x03:
		if (0x00 == buff[1])
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataCount)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat++;
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat %=(pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataCount);
				ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
				returnSystemSteelwheelName(pFlySystemInfo,
					pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName,
					pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength);
			}
		}
		else if (0x01 == buff[1])
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataCount)
			{
				if (0x00 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat)
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat = 
						pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataCount - 1;
				}
				else
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRemoteDataUseWhat--;
				}
				ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
				returnSystemSteelwheelName(pFlySystemInfo,
					pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName,
					pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength);
			}
		}
		else if (0x02 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser++;
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser %= RADIO_COUNTRY_ID;		
			returnSystemRadioID(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser);
			
			PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
		}
		else if (0x03 == buff[1])
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser--;
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser = RADIO_COUNTRY_ID - 1;		
			}
			returnSystemRadioID(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser);
			
			PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
		}
		else if (0x04 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg = TRUE;
			PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				
			actualLCDIdle();
			DBG0(debugString("\nSystem WinCE Write Reset!!!!!!!!");)
			printfHowLongTime(pFlySystemInfo);
		}
		else if (0x05 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode = !pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode;
			ipcStartEvent(EVENT_GLOBAL_DEMO_OSD_START_ID);
			controlToMCUDebug(pFlySystemInfo);
		}
		else if (0x06 == buff[1])
		{
			if (!(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode == 9 
				|| (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode > 0 && pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode <= 6)))
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 9;	
			}
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode++;
			if (10 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 1;
			}
			else if (7 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 9;
			}
			returnDVDRegionCode(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode);
			ipcStartEvent(EVENT_GLOBAL_DVD_REGION_SET_ID);
		}
		else if (0x07 == buff[1])
		{
			if (!(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode == 9 
				|| (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode > 0 && pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode <= 6)))
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 9;	
			}
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode--;
			if (0 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 9;
			}
			else if (8 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = 6;
			}
			else
			{
			}
			returnDVDRegionCode(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode);
			ipcStartEvent(EVENT_GLOBAL_DVD_REGION_SET_ID);
		}
		break;
		
	case 0x10:
		if (0x01 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable = TRUE;
		}
		else if (0x00 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bBackDetectEnable = FALSE;
		}
		flyAudioReturnToUser(pFlySystemInfo,buff,2);
		ipcStartEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);
		ipcStartEvent(EVENT_GLOBAL_BACKDETECT_CHANGE_ID);
		break;
	
	case 0x11:
		if (0x01 == buff[1])
		{
			controlToMCULightDetectEnable(pFlySystemInfo,TRUE);
		}
		else if (0x00 == buff[1])
		{
			controlToMCULightDetectEnable(pFlySystemInfo,FALSE);
		}
		break;
	
	case 0x12:
		if (0x01 == buff[1])
		{
			controlToMCUbDayNight(pFlySystemInfo,TRUE);
		}
		else if (0x00 == buff[1])
		{
			controlToMCUbDayNight(pFlySystemInfo,FALSE);
		}
		break;
		
	case 0x13:
		if (0x01 == buff[1])
		{
			controlToMCUbHaveExtAMP(pFlySystemInfo,TRUE);
		} 
		else
		{
			controlToMCUbHaveExtAMP(pFlySystemInfo,FALSE);
		}
		break;
		
	case 0x14:
		if (buff[1] < 4)
		{
			if (0 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight = 0;
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay = 0;
			}
			else
			{
				if (0 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight
					|| 0 == pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay)
				{
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight = buff[1];
					pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay = buff[1];
				}
			}
			if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bDayNight)
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightNight = buff[1];
			}
			else
			{
				pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iLCDLightBrightDay = buff[1];
			}
		}
		flyAudioReturnToUser(pFlySystemInfo,buff,len);
		PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
		break;
		
	case 0x15:
		controlToMCUPanelLight(pFlySystemInfo,buff[1]);
		break;
		
	case 0x16:
		if (0x01 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bSteerWheelOn = TRUE;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bSteerWheelOn = FALSE;
		}
		flyAudioReturnToUser(pFlySystemInfo,buff,len);
		PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
		break;
		
	case 0x17:
		if (buff[1] > RADIO_COUNTRY_ID - 1)
		{
			buff[1] = 0;
		}
		if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser != buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser = buff[1];
		}
		flyAudioReturnToUser(pFlySystemInfo,buff,len);
		
		PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
		break;
		
	case 0x18:
		pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg = TRUE;
		pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCERestartMsg = TRUE;
		PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
			
		actualLCDIdle();
		DBG1(debugString("\nSystem WinCE Write Reset To Factory!!!!!!!!");)
		printfHowLongTime(pFlySystemInfo);
		break;
		
	case 0x19:
		if (0x01 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bKeyDemoMode = TRUE;
			controlToMCUKeyDemoMode(pFlySystemInfo,TRUE);
		}
		else if (0x00 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bKeyDemoMode = FALSE;
			controlToMCUKeyDemoMode(pFlySystemInfo,FALSE);
		}
		ipcStartEvent(EVENT_GLOBAL_DEMO_KEY_START_ID);
		break;
	
	case 0x1A:
		if (0x01 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bRemoteUseStudyOn = TRUE;
		} 
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.bRemoteUseStudyOn = FALSE;
		}
		ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
		flyAudioReturnToUser(pFlySystemInfo,buff,len);
		break;
		
	case 0x1B:
		if (buff[1] < IRKEY_STUDY_COUNT)
		{
			if (0x00 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_AV;
			}
			else if (0x01 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_SEEK_INC;
			}
			else if (0x02 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_SEEK_DEC;
			}
			else if (0x03 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_MUTE;
			}
			else if (0x04 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_VOL_INC;
			}
			else if (0x05 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_VOL_DEC;
			}
			else if (0x06 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_CALL_INOUT;
			}
			else if (0x07 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_CALL_REJECT;
			}
			else if (0x08 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyID = KB_NAVI;
			}
			ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_START_ID);
		}
		break;
		
	case 0x1C:
		ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_STOP_ID);
		break;
		
	case 0x1D:
		if (buff[1] < IRKEY_STUDY_COUNT)
		{
			if (0x00 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x00] = KB_AV;
			}
			else if (0x01 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x01] = KB_SEEK_INC;
			}
			else if (0x02 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x02] = KB_SEEK_DEC;
			}
			else if (0x03 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x03] = KB_MUTE;
			}
			else if (0x04 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x04] = KB_VOL_INC;
			}
			else if (0x05 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x05] = KB_VOL_DEC;
			}
			else if (0x06 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x06] = KB_CALL_INOUT;
			}
			else if (0x07 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x07] = KB_CALL_REJECT;
			}
			else if (0x08 == buff[1])
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteStudyClearID[0x08] = KB_NAVI;
			}
			ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_CLEAR_ID);
		}
		break;
		
	case 0x1E:
		if (!(buff[1] == 9 || (buff[1] > 0 && buff[1] <= 6)))
		{
			buff[1] = 9;
		}
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode = buff[1];
		returnDVDRegionCode(pFlySystemInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode);
		ipcStartEvent(EVENT_GLOBAL_DVD_REGION_SET_ID);
		break;
		
	case 0x1F:
		LCD_Idle_Normal(TRUE);//开背光
		break;
		
	case 0x28:
		ipcStartEvent(EVENT_GLOBAL_AUX_CHECK_START_ID);
		break;
	
	case 0x2C:
		debugOneData("\n############################13@@@@@@@@@@@@@@@",buff[1]);
		pFlyAllInOneInfo->pMemory_Share_Common->iControlUserAction = buff[1];
		controlHostSecondAction(pFlySystemInfo,buff[1]);
		break;

	case 0x30:
		break;
		
	case 0x31:
		if (0x01 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg = TRUE;
			pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhat = eLEDEnterSleepTimeOut;
			pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnWhatSub = 0;
			pFlySystemInfo->SystemCarbodyInfo.iLEDBlinkOnTime = GetTickCount();
			PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);
				
			DBG2(debugString("\nSystem WinCE Write Ready Power Off!!!!!!!!");)
			printfHowLongTime(pFlySystemInfo);
		} 
		else
		{
			DBG2(debugString("\nSystem WinCE Write Prepare Power Off!!!!!!!!");)
			printfHowLongTime(pFlySystemInfo);
		}
		break;

	case 0x40:
		if (0x01 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bNoSendAndroidSystemButton = FALSE;
		}
		else if (0x00 == buff[1])
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bNoSendAndroidSystemButton = TRUE;
		}
		break;

	case 0xF0:
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iUseWhichPanel = buff[1];
		ipcStartEvent(EVENT_GLOBAL_PANEL_KEY_USE_IT_ID);
		break;
	default:
		break;
	}
}

static BOOL createThread(struct flysystem_struct_info *pFlySystemInfo)
{
	INT res;
	pthread_t thread_id;

	pFlySystemInfo->bKillDispatchFlyMainThread = FALSE;
	res = pthread_create(&thread_id, NULL, ThreadMain,pFlySystemInfo);
    if(res != 0) 
	{
		return FALSE;
    }

	return TRUE;
}

static BOOL destroyThread(struct flysystem_struct_info *pFlySystemInfo)
{
	INT res;
	pthread_t thread_id;

	pFlySystemInfo->bKillDispatchFlyMainThread = TRUE;
	PostSignal(&pFlySystemInfo->MainThreadMutex,&pFlySystemInfo->MainThreadCond,&pFlySystemInfo->bMainThreadRunAgain);

	return TRUE;
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
	INT ret = HAL_ERROR_RETURN_FD;

	resetPowerOn();

	pFlySystemInfo->bKillDispatchFlyMainThread = FALSE;
	res = pthread_create(&thread_id, NULL, ThreadMain,pFlySystemInfo);
	if(res != 0) 
	{
		return ret;
	}

	systemControlPower(pFlySystemInfo,TRUE);

	DBG0(debugString("\nFlySystem open ok");)
	ret = HAL_SYSTEM_RETURN_FD;
	return ret;
}
 
 /********************************************************************************
 **函数名称：fly_init_device_struct（）函数
 **函数功能：初始化结构体里的成员
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
	//为 flysystem_struct_info 结构体分配内存
	pFlySystemInfo =
		(struct flysystem_struct_info *)malloc(sizeof(struct flysystem_struct_info));
	if (pFlySystemInfo == NULL)
	{
		return;
	}
	memset(pFlySystemInfo, 0, sizeof(struct flysystem_struct_info));
	
	pFlySystemInfo->bControlLCDIdleNormal = TRUE;
	pFlySystemInfo->bControlLCDIdleBackVideo = TRUE;

	pthread_mutex_init(&pFlySystemInfo->MainThreadMutex, NULL);
	pthread_cond_init(&pFlySystemInfo->MainThreadCond, NULL);
	allInOneInit();

	DBG0(debugString("\nFlySystem hal init ");)
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
	 DBG1(debugBuf("\nSYSTEM-HAL return  bytes Start:", buf,1);)
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nSYSTEM-HAL return  bytes to User:", buf,dwRead);)
	 if (0x21 == buf[3])
	 {
		 debugBuf("\n@@@@@@@@@@@############$$$$$$$$$$$!!!!!!!!!!!",&buf[3],3);
	 }
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
	pthread_cond_destroy(&pFlySystemInfo->MainThreadCond);
	
	allInOneDeinit();

	free (pFlySystemInfo);
	pFlySystemInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
INT flyCloseDevice(void)
{	
	DBG0(debugString("\nsystem close device !");)

	destroyThread(pFlySystemInfo);
	
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
	DBG1(debugBuf("\nSystem Rec JNI:",&buf[3],buf[2]-1);)

	DealRightDataProcessor(pFlySystemInfo, &buf[3], buf[2]-1);
}
