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
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>  

#include "FlySystem.h"
#include "../../include/allInOneOthers.h"

//以下所有以分钟为单位

#define SYSTEM_RESET_USE_EXT_CONFIG	(pFlyAllInOneInfo->pMemory_Share_Common->bSystemResetUseExtConfig)

#define SYSTEM_RESET_AT_LEAST_DAYS	(pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetAtLeastDays)	
#define SYSTEM_RESET_ON_HOUR		(pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetOnHour)	
#define SYSTEM_RESET_ON_MINUTE		(pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetOnMinute)	

#define SYSTEM_RESET_INNER_MIN		(pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetInnerMin)	

#define SYSTEM_RESET_POWEROFF_MIN	(pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetPowerOffMin)

#define SYSTEM_CAN_RUN_AT_LEAST			(pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetCanRunLess)

UINT32 returnTransToKylinTime(time_t *tTime)//单位，分钟
{
	return *tTime/60;
}

UINT32 returnMinutesToNextByDay(UINT32 now,UINT32 next)
{
	if (next > now)
	{
		return next - now;
	}
	else
	{
		return 24*60 - now + next;
	}
}

time_t timeNow;

UINT32 kylinTimeNow;
UINT32 kylinTimePowerOn;
UINT32 kylinTimeLastAccOff;

UINT32 timeNow_Hour;
UINT32 timeNow_Minute;
UINT32 timeNow_Second;

void resetPrintDebugMsg(void)
{
	struct tm *ptr;

	ptr = localtime(&timeNow);
	debugThreeData("\nSystem RP Now Time:",ptr->tm_hour,ptr->tm_min,ptr->tm_sec);

	ptr = localtime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn);
	debugThreeData("\nSystem RP First Power On Time:",ptr->tm_hour,ptr->tm_min,ptr->tm_sec);

	ptr = localtime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff);
	debugThreeData("\nSystem RP Last ACC Off Time:",ptr->tm_hour,ptr->tm_min,ptr->tm_sec);

	debugOneData("\nSystem RP UserACCOff - PowerOn = "
		,returnTransToKylinTime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff)
		-returnTransToKylinTime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn));
}

void getNowTime(void)
{
	struct tm *ptr;

	timeNow = time(NULL);//现在时间

	kylinTimePowerOn = returnTransToKylinTime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn);
	kylinTimeLastAccOff = returnTransToKylinTime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff);
	kylinTimeNow = returnTransToKylinTime(&timeNow);
	
	if (kylinTimePowerOn > kylinTimeNow
		|| kylinTimeLastAccOff > kylinTimeNow
		|| kylinTimePowerOn > kylinTimeLastAccOff)//异常处理
	{
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn = time(NULL);
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff = time(NULL);
		timeNow = time(NULL);//现在时间

		kylinTimePowerOn = returnTransToKylinTime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn);
		kylinTimeLastAccOff = returnTransToKylinTime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff);
		kylinTimeNow = returnTransToKylinTime(&timeNow);
	}

	ptr = localtime(&timeNow);
	timeNow_Hour = ptr->tm_hour;
	timeNow_Minute = ptr->tm_min;
	timeNow_Second = ptr->tm_sec;

	debugThreeData("System RP Now Time Hour Minute Second",timeNow_Hour,timeNow_Minute,timeNow_Second);
}

void resetPowerOn(void)//开机
{
	if (!SYSTEM_RESET_USE_EXT_CONFIG)
	{
		SYSTEM_RESET_AT_LEAST_DAYS = 7;	//至少多少天
		SYSTEM_RESET_ON_HOUR = 3;	//在什么小时
		SYSTEM_RESET_ON_MINUTE = 45;	//在什么分

		SYSTEM_RESET_INNER_MIN = 60;	//关机后至少多少分钟

		SYSTEM_RESET_POWEROFF_MIN = (5*24*60);	//多长时间最低功耗，分钟

		SYSTEM_CAN_RUN_AT_LEAST = (30*24*60);	//至少能运行多长时间
	}

	pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn = time(NULL);

	resetPrintDebugMsg();
}

void resetPowerOff(void)//关机
{
	//暂时用不上
}

void resetPowerUp(void)//ACC On
{
	pFlyAllInOneInfo->pMemory_Share_Common->bSilencePowerUp = FALSE;

	resetPrintDebugMsg();
}

void resetPowerDown(void)//ACC Off
{
	if (!pFlyAllInOneInfo->pMemory_Share_Common->bSilencePowerUp)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff = time(NULL);
	}

	resetPrintDebugMsg();

	pFlyAllInOneInfo->pMemory_Share_Common->bSilencePowerUp = FALSE;
}

UINT32 resetReturnNextResetTime(void)
{
	UINT32 kylinTimeToRestWant;

	UINT32 timeTail;

	UINT32 timeToReset = 0;

	getNowTime();
	resetPrintDebugMsg();

	timeTail = timeNow_Hour*60 + timeNow_Minute;
	kylinTimeToRestWant = returnMinutesToNextByDay(timeTail,(SYSTEM_RESET_ON_HOUR*60+SYSTEM_RESET_ON_MINUTE));//24小时内到下一个时间点的分钟数
	kylinTimeToRestWant += kylinTimeNow;
	debugOneData("\nSystem RP Time To Next KeyPoint",kylinTimeToRestWant - kylinTimeNow);

	if ((kylinTimeNow - kylinTimePowerOn)/24/60 < SYSTEM_RESET_AT_LEAST_DAYS)
	{
		kylinTimeToRestWant += (SYSTEM_RESET_AT_LEAST_DAYS - ((kylinTimeNow - kylinTimePowerOn)/24/60))*24*60;
	}
	debugOneData("\nSystem RP Time To Next Reset",kylinTimeToRestWant - kylinTimeNow);

	if (kylinTimeToRestWant - kylinTimeNow >= SYSTEM_RESET_POWEROFF_MIN)//大于最长低功耗时间
	{
		timeToReset = SYSTEM_RESET_POWEROFF_MIN;
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.bNextTimeToPowerOff = TRUE;
		debugOneData("\nSystem RP Time To Next PowerOff",timeToReset);
	}
	else
	{
		timeToReset = kylinTimeToRestWant - kylinTimeNow;
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.bNextTimeToPowerOff = FALSE;
	}

	if (kylinTimeNow - kylinTimePowerOn >= SYSTEM_CAN_RUN_AT_LEAST)
	{
		timeToReset = SYSTEM_RESET_INNER_MIN;
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.bNextTimeToPowerOff = FALSE;
		debugOneData("\nSystem RP Have Run More Than SYSTEM_CAN_RUN_LESS",timeToReset);
	}
	if (timeToReset < SYSTEM_RESET_INNER_MIN)
	{
		timeToReset += 24*60;
		pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.bNextTimeToPowerOff = FALSE;
		debugOneData("\nSystem RP Time Roll To Next Day%d",timeToReset);
	}

	debugOneData("\n\nSystem RP Will PowerOn Minutes Later!!!!!!!!",timeToReset);
	timeToReset *= 60;//转换成秒

	return 60*60*24*7;
	//return timeToReset;
}

void resetPowerProcOnRecMCUWakeup(void)//处理
{
	debugString("\nSystem RP Rec MCU WatchDog Wakeup");

	getNowTime();
	resetPrintDebugMsg();

	if (pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.bNextTimeToPowerOff
		|| (kylinTimeNow - kylinTimeLastAccOff >= SYSTEM_RESET_POWEROFF_MIN))
	{
		debugString("\nSystem RP Control To VeryLow Power");
		controlToMCUPowerToVeryLowOff(pFlySystemInfo);
	}
}
