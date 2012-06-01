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

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_KEY
#define LOCAL_HAL_NAME		"flyKey Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_KEY

#include "FlyKey.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"


struct flykey_struct_info *pFlyKeyInfo = NULL;

void SteelwheelStudyRegDataRead(void)
{
	int iLength;
	char cBuff[92];

	memset(&pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab,0,sizeof(IRKEY_STUDY_TAB));

	iLength = property_get("fly.stStudy.AV",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_AV] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_AV] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_AV]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.MUTE",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_MUTE] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_MUTE] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_MUTE]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.VOL_INC",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_INC] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_VOL_INC] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_INC]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.VOL_DEC",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_DEC] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_VOL_DEC] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_DEC]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.SEEK_INC",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_INC] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_SEEK_INC] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_INC]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.SEEK_DEC",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_DEC] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_SEEK_DEC] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_DEC]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.CALL_INOUT",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_INOUT] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_CALL_INOUT] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_INOUT]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.CALL_REJECT",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_REJECT] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_CALL_REJECT] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_REJECT]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	iLength = property_get("fly.stStudy.NAVI",cBuff,"000000");
	if ('1' == cBuff[0])
	{
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_NAVI] = 1;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_NAVI] = '1'==cBuff[1]?1:0;
		pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_NAVI]
		= (cBuff[2]-'0')*1000 + (cBuff[3]-'0')*100 + (cBuff[4]-'0')*10 + (cBuff[5]-'0');
	}

	debugOneData("\nRead fly.stStudy.AV",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_AV]);
	debugOneData("\nRead fly.stStudy.MUTE",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_MUTE]);
	debugOneData("\nRead fly.stStudy.VOL_INC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_INC]);
	debugOneData("\nRead fly.stStudy.VOL_DEC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_DEC]);
	debugOneData("\nRead fly.stStudy.SEEK_INC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_INC]);
	debugOneData("\nRead fly.stStudy.SEEK_DEC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_DEC]);
	debugOneData("\nRead fly.stStudy.CALL_INOUT",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_INOUT]);
	debugOneData("\nRead fly.stStudy.CALL_REJECT",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_REJECT]);
	debugOneData("\nRead fly.stStudy.NAVI",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_NAVI]);
}

void SteelwheelStudyRegDataWrite(void)
{
	char cBuff[] = "000000";

	debugOneData("\nWrite fly.stStudy.AV",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_AV]);
	debugOneData("\nWrite fly.stStudy.MUTE",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_MUTE]);
	debugOneData("\nWrite fly.stStudy.VOL_INC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_INC]);
	debugOneData("\nWrite fly.stStudy.VOL_DEC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_DEC]);
	debugOneData("\nWrite fly.stStudy.SEEK_INC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_INC]);
	debugOneData("\nWrite fly.stStudy.SEEK_DEC",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_DEC]);
	debugOneData("\nWrite fly.stStudy.CALL_INOUT",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_INOUT]);
	debugOneData("\nWrite fly.stStudy.CALL_REJECT",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_REJECT]);
	debugOneData("\nWrite fly.stStudy.NAVI",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_NAVI]);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_AV])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_AV]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_AV]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_AV]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_AV]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_AV]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.AV", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_MUTE])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_MUTE]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_MUTE]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_MUTE]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_MUTE]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_MUTE]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.MUTE", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_INC])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_VOL_INC]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_INC]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_INC]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_INC]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_INC]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.VOL_INC", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_VOL_DEC])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_VOL_DEC]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_DEC]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_DEC]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_DEC]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_VOL_DEC]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.VOL_DEC", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_INC])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_SEEK_INC]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_INC]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_INC]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_INC]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_INC]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.SEEK_INC", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_SEEK_DEC])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_SEEK_DEC]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_DEC]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_DEC]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_DEC]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_SEEK_DEC]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.SEEK_DEC", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_INOUT])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_CALL_INOUT]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_INOUT]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_INOUT]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_INOUT]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_INOUT]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.CALL_INOUT", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_CALL_REJECT])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_CALL_REJECT]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_REJECT]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_REJECT]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_REJECT]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_CALL_REJECT]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.CALL_REJECT", cBuff);

	if (pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Value[KB_NAVI])
	{
		cBuff[0] = '1';
		cBuff[1] = 1==pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.Port[KB_NAVI]?'1':'0';
		cBuff[2] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_NAVI]/1000 + '0';
		cBuff[3] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_NAVI]%1000/100 + '0';
		cBuff[4] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_NAVI]%100/10 + '0';
		cBuff[5] = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab.AD[KB_NAVI]%10 + '0';
	}
	else
	{
		memset(cBuff,'0',sizeof(cBuff));
		cBuff[sizeof(cBuff)-1] = 0;
	}
	property_set("fly.stStudy.NAVI", cBuff);
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	//DBG0(debugOneData("\nKeyHAL IPC Read ID",sourceEvent);)

	switch (sourceEvent)
	{
	case EVENT_AUTO_CLR_PARA_INIT_ID:
		memset(&pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab,0,sizeof(IRKEY_STUDY_TAB));
		SteelwheelStudyRegDataWrite();
		break;
	case EVENT_AUTO_CLR_PARA_READ_ID:
		SteelwheelStudyRegDataRead();
		break;

	case EVENT_AUTO_CLR_SUSPEND_ID:
		if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg)
		{
			memset(&pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.remoteStudyTab,0,sizeof(IRKEY_STUDY_TAB));
			SteelwheelStudyRegDataWrite();
		}
		break;

	case EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_FINISH_ID:
		ipcClearEvent(sourceEvent);

		SteelwheelStudyRegDataWrite();
		ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_RETURN_FINISH_ID);
		break;
	case EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID:
		ipcClearEvent(sourceEvent);

		SteelwheelStudyRegDataWrite();
		ipcStartEvent(EVENT_GLOBAL_REMOTE_STUDY_RETURN_WAIT_ID);
		break;
	default:
		break;
	}
}

static void flyAudioReturnToUserPutToBuff(BYTE *buf, UINT len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nKEY-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nKEY-HAL write  bytes to User Error:", buf,len);)
	}
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	BYTE buff[2] = {0x00,0x00};
	DBG1(debugBuf("\nKeyHAL read from hardware",buf,length);)
	if (MSG_KEY_RES_KEYID == buf[0])
	{
		if (KB_SLEEP == buf[1])
		{
			ipcStartEvent(EVENT_GLOBAL_KEY_STANDBY_ID);	
		}
		else
		{
			buff[1] = buf[1];
			//DBG3(debugString("KEY have data\n");)
			flyAudioReturnToUserPutToBuff(&buff[0], 2);
		}
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
	INT ret = HAL_ERROR_RETURN_FD;

	DBG0(debugString("\nkey open ok");)
	ret = HAL_KEY_RETURN_FD;
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
	INT i,j;
	
	//为 flykey_struct_info 结构体分配内存
	pFlyKeyInfo =
		(struct flykey_struct_info *)malloc(sizeof(struct flykey_struct_info));
	if (pFlyKeyInfo == NULL)
	{
		return;
	}
	memset(pFlyKeyInfo, 0, sizeof(struct flykey_struct_info));
	
	//初始化互斥锁各条件变量
	allInOneInit();
	
	DBG0(debugString("\nFlyKey hal init\n");)
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
	 DBG1(debugBuf("\nKEY-HAL return  bytes Start:", buf,1);)
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nKEY-HAL return  bytes to User:", buf,dwRead);)
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
	allInOneDeinit();
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	
	//DBG0(debugString("\nFly key Close");)
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
	//DBG0(debugBuf("\nUser write %d bytes to KEY-HAL:%s", buf, len);)
	return;
}