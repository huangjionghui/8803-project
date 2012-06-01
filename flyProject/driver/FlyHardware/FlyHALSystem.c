#include "FlyInclude.h"

//#include "power.h"
extern void request_suspend_state(int state);

void systemLCDPWMstatus(BOOL status)
{
	if (status)
		SOC_IO_Input(SYSTEM_LCD_PWM_GROUP,SYSTEM_LCD_PWM_GPIO,0);
	else
		SOC_IO_Input(SYSTEM_LCD_PWM_GROUP,SYSTEM_LCD_PWM_GPIO,1);
}
void systemControlAccOn(void)
{
	BYTE buff[] = {MSG_SYSTEM_RES_SUSPEND_RESUME,0x01};

	messageNormalToSystemHAL(buff,sizeof(buff));
}

void systemControlAccOff(void)
{
	printk("JQilin request_suspend_state 3\n");
	request_suspend_state(3);
	//SOC_PWR_ShutDown();
}

UINT noBlockSystemMessage(BYTE *pData,UINT length)
{
	UINT returnCount = 0;

	return returnCount;
}

void systemLCDPWMProc(void)
{
	BYTE iSub;

	if (pGlobalHardwareInfo->iCurLCDPWM != pGlobalHardwareInfo->iPreLCDPWM)
	{
		if (pGlobalHardwareInfo->iCurLCDPWM < pGlobalHardwareInfo->iPreLCDPWM)
		{
			iSub = pGlobalHardwareInfo->iPreLCDPWM - pGlobalHardwareInfo->iCurLCDPWM;
			if (iSub > 15)
			{
				pGlobalHardwareInfo->iCurLCDPWM+=15;
			}
			else if (iSub > 10)
			{
				pGlobalHardwareInfo->iCurLCDPWM+=10;
			}
			else if (iSub > 5)
			{
				pGlobalHardwareInfo->iCurLCDPWM+=5;
			}
			else
			{
				pGlobalHardwareInfo->iCurLCDPWM++;
			}
		}
		else if (pGlobalHardwareInfo->iCurLCDPWM > pGlobalHardwareInfo->iPreLCDPWM)
		{
			iSub = pGlobalHardwareInfo->iCurLCDPWM - pGlobalHardwareInfo->iPreLCDPWM;
			if (iSub > 15)
			{
				pGlobalHardwareInfo->iCurLCDPWM-=15;
			}
			else if (iSub > 10)
			{
				pGlobalHardwareInfo->iCurLCDPWM-=10;
			}
			else if (iSub > 5)
			{
				pGlobalHardwareInfo->iCurLCDPWM-=5;
			}
			else
			{
				pGlobalHardwareInfo->iCurLCDPWM--;
			}
		}
		SOC_BL_Set(pGlobalHardwareInfo->iCurLCDPWM);
	}
}

void dealSystemCommand(BYTE *buf, UINT len)
{
	UINT16 pwmFreq;
	BYTE   duty;

	switch (buf[0])
	{

	case MSG_SYSTEM_CON_SUSPEND_RESUME:
		if (0x01 == buf[1])
		{
			printk("JQilin request_suspend_state 0\n");
			request_suspend_state(0);

			sleepOnProcWakeup();

			LPCPowerOnOK();

			touchTimeoutInitClear();
		}
		else
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bPrepareToSleep = TRUE;
			DBG0("\nControl To MCU Power Off");

			sleepOnProcSleep();

			LPCControlHowLongToPowerOn(GlobalShareMmapInfo.pShareMemoryCommonData->iHowLongToRestart);
			LPCControlToSleep();
			msleep(100);
			//LPCControlReset();
			//LPCControlTurnOff();
			systemControlAccOff();
		}
		break;

	case MSG_SYSTEM_CON_FAN: 
		SOC_IO_Output(SYSTEM_FAN_CONT_GROUP,SYSTEM_FAN_GPIO,buf[1]);
		break;

	case MSG_SYSTEM_CON_PANEPEN:
		SOC_IO_Output(SYSTEM_PANNE_PEN_GROUP,SYSTEM_PANNE_PEN,buf[1]);
		break;

	case MSG_SYSTEM_CON_LCDIDLE:
		ioControlLCDIdleStatus(buf[1]);
		break;

	case MSG_SYSTEM_CON_LCDPWM:
		pGlobalHardwareInfo->iPreLCDPWM = buf[1];
		//SOC_BL_Set(buf[1]);
		printk("\n LCD PWM is:%d", buf[1]);
		break;

	case MSG_SYSTEM_TRANS_MCU://¸úLPCÍ¨ÐÅ
		LPCCombinDataStream(&buf[1], len-1);
		break;

	default:
		break;
	}
}
