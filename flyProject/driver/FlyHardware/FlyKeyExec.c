#include "FlyInclude.h"

void returnKeyID(BYTE keyID)
{
	BYTE buff[] = {MSG_KEY_RES_KEYID,0x00};

	buff[1] = keyID;
	messageToKeyHAL(buff,sizeof(buff));
}

void DemoKeyMessage(void)
{
	UINT iRemain;
	ULONG nowTimer;

	nowTimer = GetTickCount();

	if (nowTimer - pGlobalHardwareInfo->iKeyDemoInnerTime >= 500)
	{
		pGlobalHardwareInfo->iKeyDemoInnerTime = nowTimer;
		pGlobalHardwareInfo->iKeyDemoState++;

		iRemain = pGlobalHardwareInfo->iKeyDemoState % 128;//音量
		if (iRemain < 8)
		{
			putKeyMessage(KB_VOL_INC);
		}
		else if (iRemain >= 64 && iRemain < 72)
		{
			putKeyMessage(KB_VOL_DEC);
		}

		iRemain = pGlobalHardwareInfo->iKeyDemoState % 17;//AV切换
		if (0 == iRemain)
		{
			putKeyMessage(KB_AV);
		}

		iRemain = pGlobalHardwareInfo->iKeyDemoState % 157;//进出碟
		if (0 == iRemain)
		{
			putKeyMessage(KB_DVD_OPEN);
		}
	}
}

static BOOL volumeChange(BYTE keyID)
{
	BOOL bRet = FALSE;

	//DBG0("GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax:%d\n",GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax);

	if (KB_VOL_INC == keyID){
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bCheckShellBabyError)
		{
			return TRUE;
		}
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackDetectEnable
			&& GlobalShareMmapInfo.pShareMemoryCommonData->bBackActiveNow)
		{
			return TRUE;
		}
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->bMute)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
			if (0 == GlobalShareMmapInfo.pShareMemoryCommonData->iVolume)
			{
				GlobalShareMmapInfo.pShareMemoryCommonData->iVolume++;
			}
		}
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->iVolume < GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume++;
		}
		else
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax;
		}

		if (0 == GlobalShareMmapInfo.pShareMemoryCommonData->iVolume)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = TRUE;
		}

		bRet = TRUE;
	}
	else if (KB_VOL_DEC == keyID){
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackDetectEnable
			&& GlobalShareMmapInfo.pShareMemoryCommonData->bBackActiveNow)
		{
			return TRUE;
		}
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->bMute)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
		}
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->iVolume > GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMin)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume--;
		}
		else
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMin;
		}

		if (0 == GlobalShareMmapInfo.pShareMemoryCommonData->iVolume)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = TRUE;
		}

		bRet = TRUE;
	}
	else if (KB_MUTE == keyID)
	{
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackDetectEnable
			&& GlobalShareMmapInfo.pShareMemoryCommonData->bBackActiveNow)
		{
			return TRUE;
		}
		else
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = !GlobalShareMmapInfo.pShareMemoryCommonData->bMute;
		}

		if (0 == GlobalShareMmapInfo.pShareMemoryCommonData->iVolume)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = TRUE;
		}

		bRet = TRUE;
	}

	if (bRet)
	{
		ipcStartEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
		ipcDriverStart(IPC_DRIVER_AUDIO,IPC_DRIVER_EVENT_AUDIO_NORMAL);
	}

	return bRet;
}

static BOOL keyRespondMessage(BYTE keyID)
{
	BOOL bRet = FALSE;
	switch (keyID)
	{
	case KB_ANDROID_MENU:
	case KB_LOCAL:
		if (!GlobalShareMmapInfo.pShareMemoryCommonData->bNoSendAndroidSystemButton)
		{
			SOC_Key_Report(KEY_MENU, KEY_PRESSED_RELEASED);
		}
		bRet = TRUE;
		break;

	case KB_ANDROID_BACK:
	case KB_SETUP:
		if (!GlobalShareMmapInfo.pShareMemoryCommonData->bNoSendAndroidSystemButton)
		{
			SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
		}
		bRet = TRUE;
		break;

	case KB_ANDROID_HOME:
	case KB_MENU:
		if (!GlobalShareMmapInfo.pShareMemoryCommonData->bNoSendAndroidSystemButton)
		{
			SOC_Key_Report(KEY_HOME, KEY_PRESSED_RELEASED);
		}
		bRet = TRUE;
		break;

	default:
		break;
	}

	return bRet;
}

static BOOL keyCHLastNext(BYTE keyID)
{
	BOOL bRet = FALSE;

	if (MediaMP3 == GlobalShareMmapInfo.pShareMemoryCommonData->eAudioInput)
	{
		switch (keyID)
		{
		case KB_TUNE_INC:
		case KB_SEEK_INC:
			SOC_Key_Report(KEY_NEXTSONG, KEY_PRESSED_RELEASED);
			bRet = TRUE;
			break;
		case KB_TUNE_DEC:
		case KB_SEEK_DEC:
			SOC_Key_Report(KEY_PREVIOUSSONG, KEY_PRESSED_RELEASED);
			bRet = TRUE;
			break;

		default:
			break;
		}
	}

	return bRet;
}

UINT sTestKeyPassword;
void testKeyPassword(void)
{
	BOOL bNeedBlink = TRUE;

	sTestKeyPassword *= 10;
	sTestKeyPassword += GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
	sTestKeyPassword %= 10000;
	if (9521 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bOSDDemoMode = TRUE;
	}
	else if (9822 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bMCUIICCommTest = TRUE;
	}
	else if (9823 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreToSendDataWhenToSleep = TRUE;
	}
	else if (9824 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bTempSendKeyIndex = TRUE;
	}
	else if (9825 == sTestKeyPassword)
	{
		ipcStartEvent(EVENT_GLOBAL_RESET_USB_HUB);
	}
	else if (9831 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iUARTDebugMsgOn = 1;
	}
	else if (9830 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iUARTDebugMsgOn = 0;
	}
	else if (9841 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest = FALSE;
	}
	else if (9840 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest = TRUE;
	}
	else if (9851 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bOSDDemoMode = TRUE;
	}
	else if (9850 == sTestKeyPassword)
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->bOSDDemoMode = FALSE;
	}
	else
	{
		bNeedBlink = FALSE;
	}

	if (bNeedBlink)
	{
		printk("\nDemoKey%dStart",sTestKeyPassword);
		ipcStartEvent(EVENT_GLOBAL_LED_BLINK);
	}
}

void xxxxxxxxxxxxreturnKeyIndex(BYTE iKeyIndex,BYTE iKeyValue)
{
	BYTE buff[15] = {
		0xF0,'K','e','y'
		,'I','0','0','0'
		,'V','0','0','0'
		,'^','_','^'};

		buff[5] = iKeyIndex/100 + '0';
		buff[6] = iKeyIndex%100/10 + '0';
		buff[7] = iKeyIndex%10 + '0';
		buff[9] = iKeyValue/100 + '0';
		buff[10] = iKeyValue%100/10 + '0';
		buff[11] = iKeyValue%10 + '0';
		messageNormalToSystemHAL(buff,15);
}

void putKeyMessage(BYTE keyID)
{
	DBG0("\nKey:%x",keyID);

	GlobalShareMmapInfo.pShareMemoryCommonData->iKeyCount++;

	touchTimeoutInitClear();

	if (GlobalShareMmapInfo.pShareMemoryCommonData->bTempSendKeyIndex)
	{
		xxxxxxxxxxxxreturnKeyIndex(GlobalShareMmapInfo.pShareMemoryCommonData->iKeyIndex
			,GlobalShareMmapInfo.pShareMemoryCommonData->iKeyValue);
		return;
	}

	if (KB_SLEEP == keyID)
	{
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bNeedWinCEPowerOff
			|| GlobalShareMmapInfo.pShareMemoryCommonData->bBackActiveNow
			|| (GlobalShareMmapInfo.pShareMemoryCommonData->bNoSendAndroidSystemButton && !GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)
			)
		{
			return;
		}
	}

	if (volumeChange(keyID))
	{
		returnKeyID(KB_NP);
	}
	else
	{
		testKeyPassword();

		if (keyRespondMessage(keyID))
		{
			returnKeyID(KB_NP);
		}
		else if (keyCHLastNext(keyID))
		{
			returnKeyID(keyID);//仍然发送上去
		}
		else
		{
			returnKeyID(keyID);
		}
	}
}

void touchTimeoutInitClear(void)
{
	pGlobalHardwareInfo->iTouchTimeoutTime = GetTickCount();
}

void touchTimeoutProc(void)
{
	if (pGlobalHardwareInfo->sFLAG_FOR_15S_OFF != FLAG_FOR_15S_OFF)
	{
		pGlobalHardwareInfo->sFLAG_FOR_15S_OFF = FLAG_FOR_15S_OFF;
		pGlobalHardwareInfo->iTouchTimeoutTime = GetTickCount();
	}

	if (pGlobalHardwareInfo->iTouchTimeoutTime)
	{
		if (GetTickCount() - pGlobalHardwareInfo->iTouchTimeoutTime >= 15000)
		{
			printk("\n Fly No Key Touch 15S Time Out\n");
			pGlobalHardwareInfo->iTouchTimeoutTime = 0;
			ipcStartEvent(EVENT_TOUCH_TIMEOUT_RETURN_ID);
		}
	}
}
