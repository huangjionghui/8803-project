#include "FlyInclude.h"

void steelwheelChangeStudyToNormal(void);
void SteelwheelStudyProc(void);
void remoteKeyInit(void);

//tPANEL_TAB DefaultPanelTab = {	  //6507B
//	{0xaa,0x55},
//	0x01,
//	{'A','V','7','5','0','7','-','0'},
//	0x00,
//	{0x36,0x35,0x3b,0x3c},
//	{
//		KB_DIMM,0x03,0x38,0x39,0x02,0x37,0x34,
//		0x10,0x11,0x12,0x13,0x3a,KB_DVD_OPEN,KB_NAVI,
//		KB_DEST,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
//	},
//	800,	//LCD_RGB_Wide
//	480,	//LCD_RGB_High
//};

//tPANEL_TAB DefaultPanelTab = {	  //6507B Android面板
//	{0xaa,0x55},
//	0x01,
//	{'A','V','7','5','0','7','-','0'},
//	0x00,
//	{0x36,0x35,0x3b,0x3c},
//	{
//		KB_DIMM,0x03,0x38,0x39,KB_ANDROID_MENU,0x37,0x34,
//			0x10,KB_ANDROID_BACK,KB_ANDROID_HOME,0x13,0x3a,KB_DVD_OPEN,KB_NAVI,
//			KB_ANDROID_BACK,KB_ANDROID_HOME,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
//	},
//	800,	//LCD_RGB_Wide
//	480,	//LCD_RGB_High
//};

tPANEL_TAB DefaultPanelTab = {	  //6507B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','0','7','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_DEST,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab07 = {	  //6507B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','0','7','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_DEST,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab47 = {	  //6547B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','4','7','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_DEST,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab10 = {	  //6510B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','1','0','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_DEST,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab66 = {	  //6566B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','6','6','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_ANDROID_HOME,
			KB_ANDROID_BACK,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab48 = {	  //6548B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','4','8','-','0'},
	0x00,
	{KB_VOL_DEC,KB_VOL_INC,KB_TUNE_INC,KB_TUNE_DEC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_BACK,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_ANDROID_MENU,KB_ANDROID_HOME,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab97 = {	  //6597B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','9','7','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_NAVI,KB_AV,KB_ANDROID_HOME,KB_MUTE,KB_DVD_OPEN,
			KB_ANDROID_MENU,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_ANDROID_MENU,KB_ANDROID_HOME,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab62 = {	  //6562B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','6','2','-','0'},
	0x00,
	{KB_TUNE_DEC,KB_TUNE_INC,KB_VOL_DEC,KB_VOL_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_ANDROID_HOME,KB_DEST,
			KB_ANDROID_BACK,KB_AV,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

tPANEL_TAB PanelTab81 = {	  //6581B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','8','1','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_ANDROID_MENU,KB_ANDROID_HOME,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};
tPANEL_TAB PanelTab25 = {	  //6525B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','2','5','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_DEST,KB_LOCAL,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};
tPANEL_TAB PanelTab201 = {	  //6525B	Android 已XXOO
	{0xaa,0x55},
	0x01,
	{'A','V','7','5','2','5','-','0'},
	0x00,
	{KB_VOL_INC,KB_VOL_DEC,KB_TUNE_DEC,KB_TUNE_INC},
	{
		KB_DIMM,KB_ANDROID_HOME,KB_SEEK_INC,KB_SEEK_DEC,KB_ANDROID_MENU,KB_MUTE,KB_DVD_OPEN,
			KB_NAVI,KB_DEST,KB_ANDROID_BACK,KB_ANDROID_BACK,KB_TUNE_DOWN,KB_AV,KB_DEST,
			KB_ANDROID_BACK,KB_ANDROID_HOME,KB_SETUP,KB_TUNE_DOWN,0x00,0x00,0x00
	},
	800,	//LCD_RGB_Wide
	480,	//LCD_RGB_High
};

void panelChangePanel(BYTE iWhichPanel)
{
	if (0xFF == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&GlobalShareMmapInfo.pShareMemoryCommonData->tPanelTab,sizeof(tPANEL_TAB));
	}
	else if (7 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab07,sizeof(tPANEL_TAB));
	}
	else if (47 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab47,sizeof(tPANEL_TAB));
	}
	else if (10 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab10,sizeof(tPANEL_TAB));
	}
	else if (66 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab66,sizeof(tPANEL_TAB));
	}
	else if (48 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab48,sizeof(tPANEL_TAB));
	}
	else if (97 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab97,sizeof(tPANEL_TAB));
	}
	else if (62 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab62,sizeof(tPANEL_TAB));
	}
	else if (25 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab25,sizeof(tPANEL_TAB));
	}
	else if (81 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab81,sizeof(tPANEL_TAB));
	}
	else if (201 == iWhichPanel)
	{
		memcpy(&pGlobalHardwareInfo->tPanelTab,&PanelTab201,sizeof(tPANEL_TAB));
	}
}

//Android07按键板对应
//EJECT 7	Media 13	SEEK+ 3	SEEK- 4	HOME 2	MUTE 6	NAVI 8	DEST 9	MENU 5	BACK 10	TUNE 12

//tPANEL_TAB DefaultPanelTab = {	  //8016-A
//	{0xaa,0x55},
//	0x01,
//	{'A','V','8','0','1','6','-','A'},
//	0x00,
//	{0x35,0x36,0x3C,0x3B},
//	{
//		0x00,0x39,0x38,0x10,0x33,0x37,0x00,
//			0x00,0x00,0x00,0x00,0x00,0x00,0x13,
//			0x08,0x11,0x34,0x3A,0x00,0x00,0x00
//	},
//	800,	//LCD_RGB_Wide
//	480,	//LCD_RGB_High
//};

BYTE key_Tab_short[3][7] = { //0表示无键位置.
	{0x01,0x02,0x03,0x04,0x05,0x06,0x13},//ADC1
	{0x07,0x08,0x09,0x0a,0x0b,0x0c,0x14},//ADC2
	{0x0d,0x0e,0x0f,0x10,0x11,0x12,0x15}//ADC3
};

UINT xxxxCount = 0;

bool adcGetFromSuwei(u32 channel, u32 *value)
{
	bool bRet = TRUE;

	ADC_Get_Values(channel,value);
	xxxxCount++;
	if (xxxxCount == 200)
	{
		xxxxCount = 0;
		DBG("\nADC %d %d %d %d %d"
			,pGlobalHardwareInfo->CurrentAD[0]
		,pGlobalHardwareInfo->CurrentAD[1]
		,pGlobalHardwareInfo->CurrentAD[2]
		,pGlobalHardwareInfo->CurrentAD[3]
		,pGlobalHardwareInfo->CurrentAD[4]);
	}	

	return bRet; 
}

static void resetKeyEncoderValues(void)
{
	pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftIncCount  = 0;
	pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftDecCount  = 0;
	pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightIncCount = 0;
	pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightDecCount = 0;
}

BYTE RemoteKEY_TabL[] = {
	KB_AV,KB_SEEK_INC2,KB_SEEK_DEC2,
	KB_MUTE,KB_VOL_INC,KB_VOL_DEC,
	KB_CALL_OUT,KB_CALL_REJECT,KB_CALL_INOUT,
	KB_RADIO,KB_SCAN,KB_PAUSE,KB_SLEEP,KB_FM,
	KB_SEEK_INC2,KB_SEEK_DEC2,KB_SPEECH_IDENTIFY_START,
};
BYTE RemoteKEY_TabS[] = {
	KB_AV,KB_SEEK_INC,KB_SEEK_DEC,
	KB_MUTE,KB_VOL_INC,KB_VOL_DEC,
	KB_CALL_OUT,KB_CALL_REJECT,KB_CALL_INOUT,
	KB_RADIO,KB_SCAN,KB_PAUSE,KB_SLEEP,KB_FM,
	KB_SEEK_INC2,KB_SEEK_DEC2,KB_SPEECH_IDENTIFY_END,
};

BYTE proKeyID[5];
ULONG probKeyDownTime[5];
UINT probKeyDownCount[5];
BOOL proKeyContinuePro[5];

void processorFlyAudioKey(BYTE channel,BYTE key,BOOL bDown);

/*
上拉电阻：12K
键值电阻：    2.1K / 5.2K / 10K / 18.4K  / 37.5K  / 120K
7521有一个特殊电阻2.7K代替2.1K
*/

UINT SelectADValue;
BYTE SelectRemote;
UINT lastADValue[5];
BOOL longThanShortTimer[5];
ULONG lastChangeADTimer[5];
	
static void FlyKeyADCInit(void)
{
	BYTE i;
		
	for (i = 0;i < 5;i++)
	{
		proKeyID[i] = 0;
		probKeyDownTime[i] = 0;
		probKeyDownCount[i] = 0;
		proKeyContinuePro[i] = FALSE;
		lastADValue[i] = 0;
		longThanShortTimer[i] = 0;
		lastChangeADTimer[i] = 0;
	}
	
	pGlobalHardwareInfo->FlyKeyEncoderInfo.time_out = GetTickCount();
	pGlobalHardwareInfo->FlyIICInfo.time_out = GetTickCount();
	pGlobalHardwareInfo->bFlyKeyADCDelayWorkRunning = TRUE;
}
void FlyKeyADCDelayWork(struct work_struct *work)
{
	BYTE i,j;
	
	if (!pGlobalHardwareInfo->bFlyKeyADCDelayWorkRunning)
	{
		FlyKeyADCInit();
	}
	
	GlobalShareMmapInfo.pShareMemoryCommonData->iOSRunTime =GetTickCount();
	consoleDebugSwitch();
	voltageShakeThread();

	TVP5150StatusCheck();

	systemLCDPWMProc();

	//按键超时与导航页面切换的处理
	touchTimeoutProc();

	//测试
	if (ipcWhatEventOn(EVENT_GLOBAL_RESET_USB_HUB))
	{
		ipcClearEvent(EVENT_GLOBAL_RESET_USB_HUB);
		ioControlUSBHostReset(TRUE);
		msleep(100);
		ioControlUSBHostReset(FALSE);
	}
	if (ipcWhatEventOn(EVENT_GLOBAL_PANEL_KEY_USE_IT_ID))
	{
		ipcClearEvent(EVENT_GLOBAL_PANEL_KEY_USE_IT_ID);
		if (0xFF != GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iUseWhichPanel)
		{
			ipcStartEvent(EVENT_GLOBAL_LED_BLINK);
		}
		panelChangePanel(GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iUseWhichPanel);
	}

	if (ipcWhatEventOn(EVENT_GLOBAL_LED_BLINK))
	{
		ipcClearEvent(EVENT_GLOBAL_LED_BLINK);
		SOC_IO_Output(DVD_LEDC_GROUP,DVD_LEDC_GPIO,1);
		msleep(314);
		SOC_IO_Output(DVD_LEDC_GROUP,DVD_LEDC_GPIO,0);
	}

	//方向盘部分
	if (ipcWhatEventOn(EVENT_GLOBAL_REMOTE_STUDY_START_ID))
	{
		ipcClearEvent(EVENT_GLOBAL_REMOTE_STUDY_START_ID);
		pGlobalHardwareInfo->remoteStudyStart = TRUE;
		pGlobalHardwareInfo->remoteStudyStep = 0;
		pGlobalHardwareInfo->remoteStudyCount = 0;
		printk("FlyAudio Key Steel Study Start\n");
	}
	if (ipcWhatEventOn(EVENT_GLOBAL_REMOTE_STUDY_STOP_ID))
	{
		ipcClearEvent(EVENT_GLOBAL_REMOTE_STUDY_STOP_ID);
		pGlobalHardwareInfo->remoteStudyStart = FALSE;
		ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_RETURN_WAIT_ID);
		ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
		printk("FlyAudio Key Steel Study Stop\n");
	}
	if (ipcWhatEventOn(EVENT_GLOBAL_REMOTE_STUDY_CLEAR_ID))
	{
		ipcClearEvent(EVENT_GLOBAL_REMOTE_STUDY_CLEAR_ID);
		pGlobalHardwareInfo->remoteStudyStart = FALSE;
		for (i = 0;i < IRKEY_STUDY_COUNT;i++)
		{
			if (GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyClearID[i])
			{
				GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Value[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyClearID[i]] = 0;
				GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyClearID[i] = 0;
			}
		}
		ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID);
		ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
		printk("FlyAudio Key Steel Study Clear\n");
	}
	if (ipcWhatEventOn(EVENT_GLOBAL_REMOTE_USE_IT_ID))
	{
		ipcClearEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
		remoteKeyInit();
	}

	if (pGlobalHardwareInfo->bRadioADRead)
	{
		if (GetTickCount() - pGlobalHardwareInfo->iRadioADReadTime >= 1000)
		{
			pGlobalHardwareInfo->bRadioADRead = FALSE;
			complete(&pGlobalHardwareInfo->compRadioADRead);
		}
	}

	if(pGlobalHardwareInfo->FlyKeyEncoderInfo.bTimeOutRun &&
		GetTickCount() - pGlobalHardwareInfo->FlyKeyEncoderInfo.time_out >= 3000)
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.bTimeOutRun = FALSE;
		pGlobalHardwareInfo->FlyKeyEncoderInfo.time_out = GetTickCount();
		resetKeyEncoderValues();
	}

	if(GetTickCount() - pGlobalHardwareInfo->FlyIICInfo.time_out >= 10000)
	{
		pGlobalHardwareInfo->FlyIICInfo.time_out = GetTickCount();
		schedule_work(&pGlobalHardwareInfo->FlyIICInfo.iic_work);
	}

	pGlobalHardwareInfo->bBreakStatusIO = SOC_IO_Input(BREAK_DETECT_G,BREAK_DETECT_I,0);//刹车检测
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bBreakStatusIO != pGlobalHardwareInfo->bBreakStatusIO)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bBreakStatusIO = pGlobalHardwareInfo->bBreakStatusIO;
		ipcStartEvent(EVENT_GLOBAL_BREAKDETECT_CHANGE_ID);
		printk("\nHardware Break Change%d",pGlobalHardwareInfo->bBreakStatusIO);
	}
	pGlobalHardwareInfo->bExtPhoneStatusIO = SOC_IO_Input(PHONE_DETECT_G,PHONE_DETECT_I,0);//外部电话检测
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bExtPhoneStatusIO != pGlobalHardwareInfo->bExtPhoneStatusIO)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bExtPhoneStatusIO = pGlobalHardwareInfo->bExtPhoneStatusIO;
		ipcStartEvent(EVENT_GLOBAL_PHONEDETECT_CHANGE_ID);
		printk("\nHardware ExtPhone Change%d",pGlobalHardwareInfo->bExtPhoneStatusIO);
	}

	if (GlobalShareMmapInfo.pShareMemoryCommonData->bPrepareToSleep)
	{
		goto ADCDelayEnd;
	}

	for (i = 0;i < 3;i++)
	{
		if (0 == i)
		{
			adcGetFromSuwei(PANEL_ADC_CHANNEL1,&pGlobalHardwareInfo->CurrentAD[0]);
		}
		else if (1 == i)
		{
			adcGetFromSuwei(PANEL_ADC_CHANNEL2,&pGlobalHardwareInfo->CurrentAD[1]);
		}
		else if (2 == i)
		{	
			adcGetFromSuwei(PANEL_ADC_CHANNEL3,&pGlobalHardwareInfo->CurrentAD[2]);
		}


		pGlobalHardwareInfo->nowTimer = GetTickCount();

		pGlobalHardwareInfo->CurrentAD[i] = pGlobalHardwareInfo->CurrentAD[i] >> 4;
		GlobalShareMmapInfo.pShareMemoryCommonData->iPanelKeyAD[i] = pGlobalHardwareInfo->CurrentAD[i];

		SelectADValue = pGlobalHardwareInfo->CurrentAD[i];

		if (0x01 == pGlobalHardwareInfo->tPanelTab.type)
		{
			if (
				('2' == pGlobalHardwareInfo->tPanelTab.PanelName[4] && '1' == pGlobalHardwareInfo->tPanelTab.PanelName[5])
				||
				('4' == pGlobalHardwareInfo->tPanelTab.PanelName[4] && '8' == pGlobalHardwareInfo->tPanelTab.PanelName[5])
				)
			{
				if ((SelectADValue>=(47-13))&&(SelectADValue<=(47+13))) SelectADValue=1;   //256--47		1024--188   
				else if ((SelectADValue>=(233-13))&&(SelectADValue<=(233+13))) SelectADValue=6;   //256--233		1024--931   
				else if ((SelectADValue>=(194-13))&&(SelectADValue<=(194+13))) SelectADValue=5;   //256--194		1024--776
				else if ((SelectADValue>=(155-13))&&(SelectADValue<=(155+13))) SelectADValue=4;   //256--155		1024--620
				else if ((SelectADValue>=(116-13))&&(SelectADValue<=(116+13))) SelectADValue=3;   //256--116		1024--465
				else if ((SelectADValue>=(77-13))&&(SelectADValue<=(77+13)))   SelectADValue=2;   //256--77		1024--310
				else if (/*(SelectADValue>=(10-10))&&*/(SelectADValue<=(10+13)))    SelectADValue=7;   //256--10        1024--39
				else SelectADValue=0;//无效数据。
			} 
			else
			{
				if ((SelectADValue>=(38-13))&&(SelectADValue<=(38+13)))   SelectADValue=1;   //256--38		1024--153
				else if ((SelectADValue>=(233-13))&&(SelectADValue<=(233+13))) SelectADValue=6;   //256--233		1024--931   
				else if ((SelectADValue>=(194-13))&&(SelectADValue<=(194+13))) SelectADValue=5;   //256--194		1024--776
				else if ((SelectADValue>=(155-13))&&(SelectADValue<=(155+13))) SelectADValue=4;   //256--155		1024--620
				else if ((SelectADValue>=(116-13))&&(SelectADValue<=(116+13))) SelectADValue=3;   //256--116		1024--465
				else if ((SelectADValue>=(77-13))&&(SelectADValue<=(77+13)))   SelectADValue=2;   //256--77		1024--310
				else if (/*(SelectADValue>=(10-10))&&*/(SelectADValue<=(10+13)))    SelectADValue=7;   //256--10        1024--39
				else SelectADValue=0;//无效数据。
			} 

			if (lastADValue[i] != SelectADValue)//变化
			{
				//RETAILMSG(1, (TEXT("@%d%d"),i,SelectADValue));
				if (lastADValue[i] && longThanShortTimer[i])//达到有效时间后弹起
				{
					processorFlyAudioKey(i,pGlobalHardwareInfo->tPanelTab.KeyValue[key_Tab_short[i][lastADValue[i]-1]-1],FALSE);
				}
				lastADValue[i] = SelectADValue;
				lastChangeADTimer[i] = pGlobalHardwareInfo->nowTimer;
				longThanShortTimer[i] = FALSE;
			}
			//if (lastADValue[i])//有效
			else if (lastADValue[i])//有效
			{
				//RETAILMSG(1, (TEXT("#%d%d"),i,SelectADValue));
				//if ((nowTimer - lastChangeADTimer[i]) >= SHORT_KEY_MIN)//达到有效时间
				//{
				longThanShortTimer[i] = TRUE;
				GlobalShareMmapInfo.pShareMemoryCommonData->iKeyIndex = key_Tab_short[i][lastADValue[i]-1];
				GlobalShareMmapInfo.pShareMemoryCommonData->iKeyValue = pGlobalHardwareInfo->tPanelTab.KeyValue[key_Tab_short[i][lastADValue[i]-1]-1];
				processorFlyAudioKey(i,pGlobalHardwareInfo->tPanelTab.KeyValue[key_Tab_short[i][lastADValue[i]-1]-1],TRUE);
				//}
			}

		}
	}

	//按键DEMO
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bKeyDemoMode)
	{
		DemoKeyMessage();
	}

	if (pGlobalHardwareInfo->remoteStudyStart)
	{
		SteelwheelStudyProc();
	}
	else
	{
		for (i = 3;i < 5;i++)
		{
			if (3 == i)
			{
				adcGetFromSuwei(STEEL_ADC_CHANNEL1,&pGlobalHardwareInfo->CurrentAD[3]);
				SelectRemote = 1;
			}
			else if (4 == i)
			{
				adcGetFromSuwei(STEEL_ADC_CHANNEL2,&pGlobalHardwareInfo->CurrentAD[4]);
				SelectRemote = 2;
			}

			pGlobalHardwareInfo->nowTimer = GetTickCount();

			pGlobalHardwareInfo->CurrentAD[i] = pGlobalHardwareInfo->CurrentAD[i] >> 2;
			GlobalShareMmapInfo.pShareMemoryCommonData->iSteelAD[i-3] = pGlobalHardwareInfo->CurrentAD[i];

			SelectADValue = pGlobalHardwareInfo->CurrentAD[i];

			if ((1 == SelectRemote && SelectADValue >= pGlobalHardwareInfo->remote1ADCS)
				|| (2 == SelectRemote && SelectADValue >= pGlobalHardwareInfo->remote2ADCS))
			{
				SelectADValue = IRKEYTABSIZE;
			}
			else
			{
				for (j = 0;j < pGlobalHardwareInfo->remoteTab.size;j++)
				{
					if (SelectRemote == pGlobalHardwareInfo->remoteTab.IRKEY_Port[j])
					{
						if (SelectADValue >= pGlobalHardwareInfo->remoteTab.IRKEY_AD_Min[j] && SelectADValue <= pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[j])
						{
							SelectADValue = pGlobalHardwareInfo->remoteTab.IRKEY_Value[j];
							break;
						}
					}
				}
				if (j == pGlobalHardwareInfo->remoteTab.size)
				{
					SelectADValue = IRKEYTABSIZE;
				}
			}

			if (lastADValue[i] != SelectADValue)//变化
			{
				if ((lastADValue[i] < sizeof(RemoteKEY_TabS)) && longThanShortTimer[i])//达到有效时间后弹起
				{
					processorFlyAudioKey(i,RemoteKEY_TabS[lastADValue[i]],FALSE);
				}
				lastADValue[i] = SelectADValue;
				lastChangeADTimer[i] = pGlobalHardwareInfo->nowTimer;
				longThanShortTimer[i] = FALSE;
			}
			else if (lastADValue[i] < sizeof(RemoteKEY_TabS))//有效
			{
				if ((pGlobalHardwareInfo->nowTimer - lastChangeADTimer[i]) >= SHORT_KEY_MIN)//达到有效时间
				{
					longThanShortTimer[i] = TRUE;
					processorFlyAudioKey(i,RemoteKEY_TabS[lastADValue[i]],TRUE);
				}
			}
		}
	}

	/*
	if (GetTickCount() - debugUartADTime > 1000)
	{
	debugUartADTime = GetTickCount();
	DBG0(
	"\nAD1:%d AD2:%d AD3:%d AD4:%d AD5:%d"
	,pGlobalHardwareInfo->CurrentAD[0]
	,pGlobalHardwareInfo->CurrentAD[1]
	,pGlobalHardwareInfo->CurrentAD[2]
	,pGlobalHardwareInfo->CurrentAD[3]
	,pGlobalHardwareInfo->CurrentAD[4]
	);
	}
	*/
	
ADCDelayEnd:if (GlobalShareMmapInfo.pShareMemoryCommonData->bPrepareToSleep)
			{
				schedule_delayed_work(&pGlobalHardwareInfo->adc_delay_work, 100);
			}
			else
			{
				schedule_delayed_work(&pGlobalHardwareInfo->adc_delay_work, 5);
			}
}

typedef struct _KEY_ATTRIBUTION
{
	BYTE KeyID;
	BOOL DownAttribution;
	BOOL LongAttribution;
	UINT LongStartCount;
	UINT LongSpaceCount;
	BOOL UpAttribution;
}KEY_ATTRIBUTION, *P_KEY_ATTRIBUTION;

KEY_ATTRIBUTION tKeyAttribution[] = 
{
	{KB_MUTE,TRUE,TRUE,1200,100000,TRUE},
	{KB_TUNE_DOWN,FALSE,TRUE,1200,100000,TRUE},
	{KB_AV,TRUE,TRUE,800,1000,FALSE},
	{KB_VOL_INC,TRUE,TRUE,300,50,FALSE},
	{KB_VOL_DEC,TRUE,TRUE,300,50,FALSE},
	{0,FALSE,FALSE,0,0,FALSE}
};

void processorFlyAudioKey(BYTE channel,BYTE key,BOOL bDown)
{
	BYTE keyID;
	UINT i;
	//printk("\nchannel%d,key%d,bDown%d %d %d %d %d %d %d %d",channel,key,bDown
	//	,pGlobalHardwareInfo->CurrentAD[0]
	//,pGlobalHardwareInfo->CurrentAD[1]
	//,pGlobalHardwareInfo->CurrentAD[2]
	//,pGlobalHardwareInfo->CurrentAD[3]
	//,pGlobalHardwareInfo->CurrentAD[4]
	//,GlobalShareMmapInfo.pShareMemoryCommonData->iKeyIndex
	//	,GlobalShareMmapInfo.pShareMemoryCommonData->iKeyValue);

	if (bDown)
	{
		if (KB_MUTE == key)
		{
			if (0 == pGlobalHardwareInfo->iMutePressTime)
			{
				pGlobalHardwareInfo->iMutePressTime = GetTickCount();
			}
			else
			{
				if (GetTickCount() - pGlobalHardwareInfo->iMutePressTime >= 4000)
				{
					pGlobalHardwareInfo->iMutePressTime = GetTickCount();

					ipcStartEvent(EVENT_GLOBAL_LED_BLINK);
					
					LPCControlReset();
				}
			}
		}
		else
		{
			pGlobalHardwareInfo->iMutePressTime = 0;
		}
	}
	else
	{
		pGlobalHardwareInfo->iMutePressTime = 0;
	}

	if (FALSE == bDown)//弹起
	{
		i = 0;
		while (1)
		{
			if (proKeyID[channel] == tKeyAttribution[i].KeyID)//符合
			{
				if (tKeyAttribution[i].UpAttribution)//有弹起特性
				{
					if (KB_MUTE == proKeyID[channel])//KB_MUTE按键处理
					{
						if (proKeyContinuePro[channel])//如果没有屏蔽
						{
							if (GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)//待机状态下只有一种选择
							{
								putKeyMessage(KB_SLEEP);
							}
							else if ((pGlobalHardwareInfo->nowTimer - probKeyDownTime[channel]) < tKeyAttribution[i].LongStartCount)
							{
								putKeyMessage(KB_MUTE);
							}
							else
							{
								putKeyMessage(KB_SLEEP);
							}
						}
					}
					else
					{
						keyID = proKeyID[channel];
						if (KB_TUNE_DOWN == keyID)
						{
							if (proKeyContinuePro[channel])
							{
								putKeyMessage(keyID);
							}
						}
						else
						{
							putKeyMessage(keyID);
						}
					}
				}
				break;
			}
			else if (0 == tKeyAttribution[i].KeyID)//表里没有则缺省
			{
				break;
			}
			i++;
		}
		probKeyDownTime[channel] = 0;//清零
		probKeyDownCount[channel] = 0;//清零
		proKeyContinuePro[channel] = FALSE;
	}
	else//按下
	{
		proKeyID[channel] = key;
		if (0 == probKeyDownTime[channel])//初次按下
		{
			proKeyContinuePro[channel] = TRUE;
			probKeyDownTime[channel] = pGlobalHardwareInfo->nowTimer;
			probKeyDownCount[channel] = 0;

			i = 0;
			while (1)
			{
				if (proKeyID[channel] == tKeyAttribution[i].KeyID)//符合
				{
					if (tKeyAttribution[i].DownAttribution)//有按下特性
					{
						if (KB_MUTE == proKeyID[channel])
						{
							if (proKeyContinuePro[channel])
							{
								if (GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)//待机状态下只有一种选择
								{
									proKeyContinuePro[channel] = FALSE;
									keyID = KB_SLEEP;
									putKeyMessage(keyID);
								}
							}
						}
						else
						{
							keyID = proKeyID[channel];
							putKeyMessage(keyID);
						}
					}
					break;
				}
				else if (0 == tKeyAttribution[i].KeyID)//表里没有则缺省
				{
					keyID = proKeyID[channel];
					putKeyMessage(keyID);
					break;
				}
				i++;
			}
		}
		else//连续按下
		{
			i = 0;
			while (1)
			{
				if (proKeyID[channel] == tKeyAttribution[i].KeyID)//符合
				{
					if (tKeyAttribution[i].LongAttribution)//有长按特性
					{
						if ((pGlobalHardwareInfo->nowTimer - probKeyDownTime[channel]) >= tKeyAttribution[i].LongStartCount)
						{
							keyID = proKeyID[channel];
							if (0 == probKeyDownCount[channel])//长按启动
							{
								if (KB_MUTE == keyID)//只做进入待机
								{
									if (proKeyContinuePro[channel])
									{
										if (!GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)
										{
											proKeyContinuePro[channel] = FALSE;
											keyID = KB_SLEEP;
											putKeyMessage(keyID);
										}
									}
								}
								else if (KB_TUNE_DOWN == keyID)
								{
									if (proKeyContinuePro[channel])
									{
										proKeyContinuePro[channel] = FALSE;
										keyID = KB_TUNE_DOWN_LONG;
										putKeyMessage(keyID);
									}
								}
								else
								{
									putKeyMessage(keyID);
								}
								probKeyDownCount[channel]++;
							}
							else//长按连续
							{
								UINT iStepTemp = (pGlobalHardwareInfo->nowTimer - probKeyDownTime[channel] - tKeyAttribution[i].LongStartCount)/tKeyAttribution[i].LongSpaceCount;
								while (probKeyDownCount[channel] <= iStepTemp)
								{
									putKeyMessage(keyID);
									probKeyDownCount[channel]++;
								}
							}
						}
					}
					break;
				}
				else if (0 == tKeyAttribution[i].KeyID)//表里没有则缺省
				{
					break;
				}
				i++;
			}
		}
	}
}


/////////////////////////////////////////////////////////////////////////方向盘按键部分

//SKE06-32
//BYTE remoteData[] = {5,1,0,750,40,2,1,263,40,2,2,512,40,1,4,230,40,1,5,0,40};
//BYTE remoteData[] = {
//5
//,0x06,0x32
//,1,0,0x02,0xee,40
//,2,1,0x01,0x07,40
//,2,2,0x02,0x00,40
//,1,4,0x00,0xe6,40
//,1,5,0x00,0x00,40};

//SKE25
//BYTE remoteData[] = {
//8
//,2,0,0,40
//,1,1,0,40
//,1,2,133,40
//,2,3,593,40
//,1,4,320,40
//,1,5,598,40
//,2,7,126,40
//,2,8,319,40};
BYTE remoteData[] = {
	8
	,0x25,0x00
	,2,0,0x00,0x00,40
	,1,1,0x00,0x00,40
	,1,2,0x00,0x85,40
	,2,3,0x02,0x51,40
	,1,4,0x01,0x40,40
	,1,5,0x02,0x56,40
	,2,7,0x00,0x7E,40
	,2,8,0x01,0x3F,40};

void SteelwheelKeyADCSInit(void)
{
	UINT i;

	pGlobalHardwareInfo->remote1ADCS = 0;pGlobalHardwareInfo->remote2ADCS = 0;//有些车型有下拉电阻
	for (i=0;i<pGlobalHardwareInfo->remoteTab.size;i++)
	{
		if (pGlobalHardwareInfo->remoteTab.IRKEY_Port[i] == 1)
		{
			if (pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i] > pGlobalHardwareInfo->remote1ADCS)
			{
				pGlobalHardwareInfo->remote1ADCS = pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i];
			}
		}
		else if (pGlobalHardwareInfo->remoteTab.IRKEY_Port[i] == 2)
		{
			if (pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i] > pGlobalHardwareInfo->remote2ADCS)
			{
				pGlobalHardwareInfo->remote2ADCS = pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i];
			}
		}
	}
	if(pGlobalHardwareInfo->remote1ADCS == 0 || pGlobalHardwareInfo->remote1ADCS > 1000)pGlobalHardwareInfo->remote1ADCS = 1000;
	if(pGlobalHardwareInfo->remote2ADCS == 0 || pGlobalHardwareInfo->remote2ADCS > 1000)pGlobalHardwareInfo->remote2ADCS = 1000;
	DBG0("\nremote1ADCS:%d remote2ADCS:%d",pGlobalHardwareInfo->remote1ADCS,pGlobalHardwareInfo->remote2ADCS);
}

void SteelwheelKeyDataInit(BYTE *data)
{
	BYTE i;
	UINT advalue;
	UINT offset;
	UINT a,b;

	pGlobalHardwareInfo->remoteTab.size = data[0];
	if (pGlobalHardwareInfo->remoteTab.size > IRKEYTABSIZE) {
		pGlobalHardwareInfo->remoteTab.size = 0;
	}
	pGlobalHardwareInfo->remoteTab.CarTypeID[0] = data[1];
	pGlobalHardwareInfo->remoteTab.CarTypeID[1] = data[2];

	for(i=0;i<pGlobalHardwareInfo->remoteTab.size;i++) {
		if (data[i*5+3] >= 0x10) {
			pGlobalHardwareInfo->remoteTab.IRKEY_Value[i] = data[i*5+1+3];
			pGlobalHardwareInfo->remoteTab.IRKEY_Port[i] = data[i*5+3] - 0x10;
			advalue = (data[i*5+2+3]<<8) + data[i*5+3+3];
			offset = data[i*5+4+3];
			b = (advalue > offset)?(advalue - offset):0;
			if (((REMOTE_R_UP+REMOTE_R_BASE) * b) <= (1024*REMOTE_R_BASE)) { a = 0;}
			else { 
				a = 1024*((REMOTE_R_UP+REMOTE_R_BASE) * b - 1024*REMOTE_R_BASE);
				a = a / (REMOTE_R_BASE*b + (REMOTE_R_UP-REMOTE_R_BASE) *1024);

			}
			pGlobalHardwareInfo->remoteTab.IRKEY_AD_Min[i] = a;

			b = advalue + offset;
			a = ((REMOTE_R_UP+REMOTE_R_BASE) * b > 1024*REMOTE_R_BASE)?((REMOTE_R_UP+REMOTE_R_BASE) * b - 1024*REMOTE_R_BASE):0;
			pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i] = 1024*a / (REMOTE_R_BASE*b + (REMOTE_R_UP-REMOTE_R_BASE) *1024);
		}	   //1819432960
		else {
			pGlobalHardwareInfo->remoteTab.IRKEY_Value[i] = data[i*5+1+3];
			pGlobalHardwareInfo->remoteTab.IRKEY_Port[i] = data[i*5+3];
			advalue = (data[i*5+2+3]<<8) + data[i*5+3+3];
			offset = data[i*5+4+3];
			pGlobalHardwareInfo->remoteTab.IRKEY_AD_Min[i] = (advalue>offset)?(advalue - offset):0; 
			pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i] = advalue + offset;
		}
	}

	SteelwheelKeyADCSInit();

	for (i = 0;i < pGlobalHardwareInfo->remoteTab.size;i++)
	{
		DBG0("\n Value:%x Port:%d Min:%d Max:%d "
			,pGlobalHardwareInfo->remoteTab.IRKEY_Value[i]
		,pGlobalHardwareInfo->remoteTab.IRKEY_Port[i]
		,pGlobalHardwareInfo->remoteTab.IRKEY_AD_Min[i]
		,pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i]);
	}
}

void remoteKeyInit(void)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.bRemoteUseStudyOn)
	{
		DBG0("\nremote Use Study");
		steelwheelChangeStudyToNormal();
	}
	else
	{
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bRemoteDataHave)
		{
			DBG0("\nremote Use List%d",GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iRemoteDataUseWhat);
			SteelwheelKeyDataInit(GlobalShareMmapInfo.pShareMemoryCommonData->sRemoteData);
		}
		else
		{
			DBG0("\nremote Use Code Default");
			SteelwheelKeyDataInit(remoteData);
		}
	}
	pGlobalHardwareInfo->remoteStudyStart = FALSE;
	pGlobalHardwareInfo->remoteStudyStep = 0;
	pGlobalHardwareInfo->remoteStudyCount = 0;
}

static UINT querryDifferenceSize(UINT a,UINT b)
{
	if (a >= b)
	{
		return a-b;
	}
	else
	{
		return b-a;
	}
}

void SteelwheelStudyProc(void)
{
	UINT iTempReadAD;

	if (0 == pGlobalHardwareInfo->remoteStudyStep)//记录未按下的AD值
	{
		if (0 == pGlobalHardwareInfo->remoteStudyCount)
		{
			pGlobalHardwareInfo->remoteStudyCurrent1 = 1023;
			pGlobalHardwareInfo->remoteStudyCurrent2 = 1023;
		}
		else if (pGlobalHardwareInfo->remoteStudyCount > 50)//抖动太厉害，失败
		{
			pGlobalHardwareInfo->remoteStudyStart = FALSE;
			ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID);
			pGlobalHardwareInfo->remoteStudyStart = FALSE;
			pGlobalHardwareInfo->remoteStudyStep = 0;
			pGlobalHardwareInfo->remoteStudyCount = 0;
			ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
			DBG0("\nFlyAudio Key Steel Study Fail For Dance");
			return;
		}
		else
		{
			adcGetFromSuwei(STEEL_ADC_CHANNEL1,&iTempReadAD);
			iTempReadAD = iTempReadAD >> 2;
			if (querryDifferenceSize(iTempReadAD,pGlobalHardwareInfo->remoteStudyCurrent1) < STEEL_ADC_STUDY_DANCE)//稳定1
			{
				pGlobalHardwareInfo->remoteStudyCurrent1 = iTempReadAD;
				adcGetFromSuwei(STEEL_ADC_CHANNEL2,&iTempReadAD);
				iTempReadAD = iTempReadAD >> 2;
				if (querryDifferenceSize(iTempReadAD,pGlobalHardwareInfo->remoteStudyCurrent2) < STEEL_ADC_STUDY_DANCE)//稳定2
				{
					pGlobalHardwareInfo->remoteStudyCurrent2 = iTempReadAD;

					pGlobalHardwareInfo->remoteStudyNormalAD1 = pGlobalHardwareInfo->remoteStudyCurrent1;
					pGlobalHardwareInfo->remoteStudyNormalAD2 = pGlobalHardwareInfo->remoteStudyCurrent2;
					pGlobalHardwareInfo->remoteStudyStep = 1;//跳转，学习
					pGlobalHardwareInfo->remoteStudyCount = 0;
					ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_RETURN_START_ID);
					DBG0("\nFlyAudio Key Steel Study Start Read %d %d",pGlobalHardwareInfo->remoteStudyCurrent1,pGlobalHardwareInfo->remoteStudyCurrent2);
				}
				else
				{
					pGlobalHardwareInfo->remoteStudyCurrent2 = iTempReadAD;
				}
			}
			else
			{
				pGlobalHardwareInfo->remoteStudyCurrent1 = iTempReadAD;
			}
		}
		pGlobalHardwareInfo->remoteStudyCount++;
	}
	else if (1 == pGlobalHardwareInfo->remoteStudyStep)//学习按键
	{
		if (0 == pGlobalHardwareInfo->remoteStudyCount)
		{
			pGlobalHardwareInfo->remoteStudyCurrent1 = 1023;
			pGlobalHardwareInfo->remoteStudyCurrent2 = 1023;
		}
		else
		{
			adcGetFromSuwei(STEEL_ADC_CHANNEL1,&iTempReadAD);
			iTempReadAD = iTempReadAD >> 2;
			if (querryDifferenceSize(iTempReadAD,pGlobalHardwareInfo->remoteStudyCurrent1) < STEEL_ADC_STUDY_DANCE)//稳定1
			{
				pGlobalHardwareInfo->remoteStudyCurrent1 = iTempReadAD;
				if (querryDifferenceSize(pGlobalHardwareInfo->remoteStudyNormalAD1,pGlobalHardwareInfo->remoteStudyCurrent1) > STEEL_ADC_STUDY_DISTANCE)//有差距，成功
				{
					GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Value[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyID] = 1;
					GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Port[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyID] = 1;
					GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.AD[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyID] = pGlobalHardwareInfo->remoteStudyCurrent1;
					pGlobalHardwareInfo->remoteStudyStep = 2;
					pGlobalHardwareInfo->remoteStudyCount = 0;
					ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_FINISH_ID);
					DBG0("\nFlyAudio Key Steel Study OK1 %d",pGlobalHardwareInfo->remoteStudyCurrent1);
					return;
				}
			}
			else
			{
				pGlobalHardwareInfo->remoteStudyCurrent1 = iTempReadAD;
			}
			adcGetFromSuwei(STEEL_ADC_CHANNEL2,&iTempReadAD);
			iTempReadAD = iTempReadAD >> 2;
			if (querryDifferenceSize(iTempReadAD,pGlobalHardwareInfo->remoteStudyCurrent2) < STEEL_ADC_STUDY_DANCE)//稳定2
			{
				pGlobalHardwareInfo->remoteStudyCurrent2 = iTempReadAD;
				if (querryDifferenceSize(pGlobalHardwareInfo->remoteStudyNormalAD2,pGlobalHardwareInfo->remoteStudyCurrent2) > STEEL_ADC_STUDY_DISTANCE)//有差距，成功
				{
					GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Value[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyID] = 1;
					GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Port[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyID] = 2;
					GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.AD[GlobalShareMmapInfo.pShareMemoryCommonData->iRemoteStudyID] = pGlobalHardwareInfo->remoteStudyCurrent2;
					pGlobalHardwareInfo->remoteStudyStep = 2;
					pGlobalHardwareInfo->remoteStudyCount = 0;
					ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_FINISH_ID);
					DBG0("\nFlyAudio Key Steel Study OK2 %d",pGlobalHardwareInfo->remoteStudyCurrent2);
					return;
				}
			}
			else
			{
				pGlobalHardwareInfo->remoteStudyCurrent2 = iTempReadAD;
			}
		}
		pGlobalHardwareInfo->remoteStudyCount++;
	}
	else if (2 == pGlobalHardwareInfo->remoteStudyStep)
	{
		if (pGlobalHardwareInfo->remoteStudyCount > 5)//长时间不释放，没必要多长时间
		{
			pGlobalHardwareInfo->remoteStudyStart = FALSE;
			ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID);
			pGlobalHardwareInfo->remoteStudyStart = FALSE;
			pGlobalHardwareInfo->remoteStudyStep = 0;
			pGlobalHardwareInfo->remoteStudyCount = 0;
			ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
			DBG0("\nFlyAudio Key Steel Study Finish With Wait TimeOut");
		}
		else
		{
			adcGetFromSuwei(STEEL_ADC_CHANNEL1,&iTempReadAD);
			iTempReadAD = iTempReadAD >> 2;
			if (querryDifferenceSize(iTempReadAD,pGlobalHardwareInfo->remoteStudyNormalAD1) < STEEL_ADC_STUDY_DANCE)//稳定1
			{
				adcGetFromSuwei(STEEL_ADC_CHANNEL2,&iTempReadAD);
				iTempReadAD = iTempReadAD >> 2;
				if (querryDifferenceSize(iTempReadAD,pGlobalHardwareInfo->remoteStudyNormalAD2) < STEEL_ADC_STUDY_DANCE)//稳定2
				{
					pGlobalHardwareInfo->remoteStudyStart = FALSE;
					ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID);
					pGlobalHardwareInfo->remoteStudyStart = FALSE;
					pGlobalHardwareInfo->remoteStudyStep = 0;
					pGlobalHardwareInfo->remoteStudyCount = 0;
					ipcStartEvent(EVENT_GLOBAL_REMOTE_USE_IT_ID);
					DBG0("\nFlyAudio Key Steel Study Finish With Buttom Up");
				}
			}
		}
		pGlobalHardwareInfo->remoteStudyCount++;
	}
}

static UINT steelWheelChangeStudyReturnIndexInTabByID(BYTE iID)
{
	UINT i;
	for (i = 0;i < sizeof(RemoteKEY_TabS);i++)
	{
		if (iID == RemoteKEY_TabS[i])
		{
			return i;
		}
	}
	return 0;
}

void steelwheelChangeStudyToNormal(void)
{
	UINT i,j;
	UINT iWrite = 0;
	UINT iValue;
	UINT iPort;
	UINT iAD;
	UINT iChangeDance;
	UINT iTemp;

	memset(&pGlobalHardwareInfo->remoteTab,0,sizeof(IRKEY_TAB));

	for (i = 0;i < IRKEYTABSIZE;i++)
	{
		if (GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Value[i])
		{
			iValue = i;
			iPort = GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Port[i];
			iAD = GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.AD[i];
			iChangeDance = STEEL_ADC_CHANGE_DANCE;
			for (j = 0;j < IRKEYTABSIZE;j++)
			{
				if (j == i)
				{
					continue;
				}
				else if (0 == GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Value[j])
				{
					continue;
				}
				else if (iPort != GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.Port[j])
				{
					continue;
				}
				else
				{
					iTemp = querryDifferenceSize(iAD,GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.remoteStudyTab.AD[j])/2;
					if (iChangeDance > iTemp)
					{
						iChangeDance = iTemp;
					}
				}
			}
			pGlobalHardwareInfo->remoteTab.IRKEY_Value[iWrite] = steelWheelChangeStudyReturnIndexInTabByID(iValue);
			pGlobalHardwareInfo->remoteTab.IRKEY_Port[iWrite] = iPort;
			pGlobalHardwareInfo->remoteTab.IRKEY_AD_Min[iWrite] = (iAD>iChangeDance)?(iAD - iChangeDance):0; 
			pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[iWrite] = iAD+iChangeDance;
			iWrite++;
		}
	}
	pGlobalHardwareInfo->remoteTab.size = iWrite;
	pGlobalHardwareInfo->remoteTab.CarTypeID[0] = 0x33;
	pGlobalHardwareInfo->remoteTab.CarTypeID[1] = 0x44;

	SteelwheelKeyADCSInit();

	DBG("\nFlyAudio Key Steel Use Study");
	DBG("\nID %x %x",pGlobalHardwareInfo->remoteTab.CarTypeID[0],pGlobalHardwareInfo->remoteTab.CarTypeID[1]);
	for (i = 0;i < pGlobalHardwareInfo->remoteTab.size;i++)
	{
		DBG("\nIndex %x ID %x Port %d Min %d Max %d"
			,pGlobalHardwareInfo->remoteTab.IRKEY_Value[i]
		,RemoteKEY_TabS[pGlobalHardwareInfo->remoteTab.IRKEY_Value[i]]
		,pGlobalHardwareInfo->remoteTab.IRKEY_Port[i]
		,pGlobalHardwareInfo->remoteTab.IRKEY_AD_Min[i]
		,pGlobalHardwareInfo->remoteTab.IRKEY_AD_Max[i]);
	}
}
////////////////////////////////////////////////////////////////以上按键处理


void keyFirstInit(void)
{
	memcpy(&pGlobalHardwareInfo->tPanelTab,&DefaultPanelTab,sizeof(tPANEL_TAB));
	
	pGlobalHardwareInfo->bFlyKeyADCDelayWorkRunning = FALSE;
	INIT_DELAYED_WORK(&pGlobalHardwareInfo->adc_delay_work, FlyKeyADCDelayWork);
	schedule_delayed_work(&pGlobalHardwareInfo->adc_delay_work, 0);
	
	keyExtiFirstInit();
}
