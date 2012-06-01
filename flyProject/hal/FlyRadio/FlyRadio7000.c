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

#include "../../include/ShareMemoryStruct.h"
#if AUDIO_RADIO_CHIP_SEL == AUDIO_RADIO_7741_7000

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_RADIO
#define LOCAL_HAL_NAME		"flyradio Stub"
#define LOCAL_HAL_AUTOHR	"Flyradio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_RADIO

#include "FlyRadio7000.h"
#include "SAF7741Radio_Data.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"


struct flyradio_struct_info *pFlyRadioInfo = NULL;

/******************************************************************************/
/*                                  各种通信ID                               */
/******************************************************************************/

void msgQueueReadProc(BYTE msgQueueID,BYTE *pData,UINT length)
{
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
	UINT16 temp;
	DBG0(debugBuf("\nSAF7741 RadioHAL read from hardware---->",buf,length);)
	if (SHARE_MEMORY_RADIO == buf[0])
	{
		//if (MSG_RADIO_CON_AD_GET == buf[1])
		//{
		//	temp = buf[2];
		//	temp = (temp<<8) | buf[3];
		//	pFlyRadioInfo->RadioAD = 0;
		//	pFlyRadioInfo->RadioAD = temp;

		//	pFlyRadioInfo->bRadioADReturn = TRUE;
		//}
		if (MSG_RADIO_TDA7541_RDS_ID == buf[1])
		{
			memcpy(&pFlyRadioInfo->rdsdec_buf, buf, length);
			PostSignal(&pFlyRadioInfo->RDSRecThreadMutex,&pFlyRadioInfo->RDSRecThreadCond,&pFlyRadioInfo->bRDSThreadRunAgain);
		}
	}
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	DBG0(debugOneData("\nFlyRadio SAF7741 HAL IPC Read ID---->",sourceEvent);)
	switch (sourceEvent)
	{
	case EVENT_AUTO_CLR_SUSPEND_ID:
		break;
	case EVENT_AUTO_CLR_RESUME_ID:
		pFlyAllInOneInfo->pMemory_Share_Common->b7741RadioInitFinish = FALSE;
		break;
	default:
		break;
	}

	sem_post(&pFlyRadioInfo->MainThread_sem);//激活一次
}

/******************************************************************************/
/*                                 各种IO操作                             */
/******************************************************************************/
static int waitSignedTimeOut(UINT32 iTimeOutMs)
{
	int ret = 0;
	struct timeval timenow;
	struct timespec timeout;
	UINT32 iSecnod,iMSecnod;

	pthread_mutex_lock(&pFlyRadioInfo->ScanThreadMutex);
	gettimeofday(&timenow, NULL);

	iSecnod = iTimeOutMs / 1000;
	iMSecnod = iTimeOutMs % 1000;

	timeout.tv_sec = timenow.tv_sec + iSecnod;
	timeout.tv_nsec = (timenow.tv_usec + iMSecnod*1000)*1000;

	while (timeout.tv_nsec >= 1000000000)
	{
		timeout.tv_nsec -= 1000000000;
		timeout.tv_sec++;
	}
	ret = pthread_cond_timedwait(&pFlyRadioInfo->ScanThreadCond,&pFlyRadioInfo->ScanThreadMutex,&timeout);
	pthread_mutex_unlock(&pFlyRadioInfo->ScanThreadMutex);

	return ret;
}

static void control_radio_ant(BYTE ant_id, BOOL bOn)
{
	BYTE buff[3];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = ant_id;
	buff[2] = bOn;
	writeDataToHardware(buff, 3);

	DBG0(debugString("\nFlyRadio SAF7741 ANT Control");)
}

static void radioANTControl(BOOL bOn)
{
	if (bOn)
	{
		control_radio_ant(MSG_RADIO_CON_ANT1,1);
		control_radio_ant(MSG_RADIO_CON_ANT2,1);
	}
	else
	{
		control_radio_ant(MSG_RADIO_CON_ANT1,0);
		control_radio_ant(MSG_RADIO_CON_ANT2,0);
	}
}

/******************************************************************************/
/*                                各种I2C操作                             */
/******************************************************************************/
//static UINT SAF7741CreateI2CRegAddr(UINT addr)
//{
//	UINT regAddr;
//	UINT regAddr1,regAddr2,regAddr3;
//
//	regAddr1 = addr & 0xff;
//	regAddr2 = (addr & 0xff00) >> 8;
//	regAddr3 = (addr & 0xff0000) >> 16;
//
//	regAddr = (regAddr1 << 16) + (regAddr2 << 8) + regAddr3;
//	return regAddr;
//}

BOOL I2C_Write_SAF7741(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	BYTE buff[256];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = MSG_RADIO_CON_SAF7741_ID;
	buff[2] = (ulRegAddr>>16) & 0xff;
	buff[3] = (ulRegAddr>>8) & 0xff;
	buff[4] = ulRegAddr & 0xff;
	
	memcpy(&buff[5],pRegValBuf,uiValBufLen);
	writeDataToHardware(buff, uiValBufLen+5);

	//DBG0("\nFlyAduio SAF7741 IIC Write-->");
	//for (i = 0; i < uiValBufLen+3; i++)
	//{
	//	DBG0(" %2X",buff[i]);
	//}
	return TRUE;
}

BOOL I2C_Read_SAF7741(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	char i;
	BYTE buff[256];

	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = S_NO_BLOCK_ID;
	buff[2] = MSG_RADIO_CON_SAF7741_ID;
	buff[3] = (ulRegAddr>>16) & 0xff;
	buff[4] = (ulRegAddr>>8) & 0xff;
	buff[5] = ulRegAddr & 0xff;

	memset(&buff[6],0,uiValBufLen);
	readDataFromHardwareNoBlock(buff,uiValBufLen+6);

	memcpy(pRegValBuf,&buff[6],uiValBufLen);

	//DBG2(debugString("\nFlyRadio SAF7741 I2C read-->");)
	//	for (i = 0; i < uiValBufLen+6; i++)
	//	{
	//		DBG2(debugOneData("",buff[i]);)
	//	}

	return TRUE;
}

static void SendToSAF7741UsingPortByLength(BYTE *p,BYTE level)
{
	UINT len;
	BYTE *p1,*p2,*p3;
	UINT regAddr;

	p2 = p;
	p3 = p2 + (1+1+3+3);
	len = p3[7];
	p1 = p3 + (1+1+3+3) + level*(5+len*2);
	regAddr = (p1[2] << 16) + (p1[3] << 8) + p1[4];
	I2C_Write_SAF7741(regAddr,&p1[5],p1[1]-3);

	regAddr = (p2[2] << 16) + (p2[3] << 8) + p2[4];
	I2C_Write_SAF7741(regAddr,&p2[5],p2[1]-3);

	regAddr = (p3[2] << 16) + (p3[3] << 8) + p3[4];
	I2C_Write_SAF7741(regAddr,&p3[5],p3[1]-3);

}

static void SendToSAF7741NormalWriteData(BYTE *pData)
{
	BYTE MChipAdd;
	UINT iLength;
	UINT dataCnt = 0;
	UINT regAddr;

	while (*pData)
	{
		dataCnt++;
		MChipAdd = *pData++;
		if(MChipAdd != SAF7741_ADDR_W)break;
		iLength = *pData++;
		if(pData[0] == 0x00 && pData[1] == 0xFF && pData[2] == 0xFF)
		{
			pData += iLength;
			continue;
		}
		else
		{
			regAddr = (pData[0] << 16) + (pData[1] << 8) + pData[2];
			I2C_Write_SAF7741(regAddr,&pData[3],iLength - 3);
			//Sleep(1);
			pData += iLength;
		}
	}
}

static BOOL I2C_Write_TEF7000(BYTE TEF7000_ID, BYTE ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	BYTE buff[256];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = TEF7000_ID;
	buff[2] = ulRegAddr;

	memcpy(&buff[3],pRegValBuf,uiValBufLen);
	writeDataToHardware(buff, uiValBufLen+3);

	//DBG0("\nFlyAduio TEF7000 IIC Write-->");
	//for (i = 0; i < uiValBufLen+3; i++)
	//{
	//	DBG0(" %2X",buff[i]);
	//}
	return TRUE;
}

BOOL I2C_Read_TEF7000(BYTE TEF7000_ID, BYTE ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	BYTE buff[256];
	buff[0] = CURRENT_SHARE_MEMORY_ID;
	buff[1] = S_NO_BLOCK_ID;
	buff[2] = TEF7000_ID;
	buff[3] = ulRegAddr;

	memset(&buff[4],0,uiValBufLen);
	readDataFromHardwareNoBlock(buff,uiValBufLen+4);

	//DBG0("\nFlyAduio SAF7741 IIC Read-->",ulRegAddr);
	//for (i = 0; i < uiValBufLen; i++)
	//{
	//	DBG0(" %2X",buff[i+4]);
	//}

	return TRUE;
}

UINT rU8ComboToU32(BYTE *p,BYTE len)
{
	BYTE i;
	UINT data = 0;
	for(i = 0; i < len; i++)
	{
		data = (data << 8) + p[i];	
	}
	return data;
}

void flyRadioReturnToUserPutToBuff(BYTE *buf, UINT16 len)
 {
	 UINT dwLength;

	 dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 if (dwLength)
	 {
		 DBG1(debugBuf("\nRADIO-HAL write  bytes to User OK:", buf,len);)
	 }
	 else
	 {
		 DBG1(debugBuf("\nRADIO-HAL write  bytes to User Error:", buf,len);)
	 }
 }
 
 /******************************************************************************/
 /*                            返回给用户的各种信息                        */
 /******************************************************************************/
 void returnRadioPowerMode(BOOL bOn)
 {
	 BYTE buf[] = {0x01,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioInitStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x02,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioMode(BYTE eMode)
 {
	 BYTE buf[] = {0x20,0x00};

	 buf[1] = eMode;

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioFreq(UINT iFreq)
 {
	 BYTE buf[] = {0x10,0x00,0x00};

	 buf[1] = iFreq >> 8;
	 buf[2] = iFreq;

	 flyRadioReturnToUserPutToBuff(buf,3);
 }

 void returnRadioAFStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x16,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioTAStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x17,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioScanCtrl(BYTE cmd)
 {
	 BYTE buf[] = {0x13,0x00};

	 buf[1] = cmd;

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioMuteStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x15,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioRDSWorkStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x30,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioHaveSearched(BOOL bHave)
 {
	 BYTE buf[] = {0x14,0x00};

	 if (bHave)
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

 void returnRadioBlinkingStatus(BOOL bOn)
 {
	 BYTE buf[] = {0x18,0x00};

	 if (bOn)
	 {
		 buf[1] = 1;
	 }
	 else
	 {
		 buf[1] = 0;
	 }

	 flyRadioReturnToUserPutToBuff(buf,2);
 }

/******************************************************************************/
/*                          各种SAF7741 TEF7000操作                        */
/******************************************************************************/
 void FlyRadio_Set_Freq(BYTE mode,UINT freq)
 {
	 UINT temp;
	 BYTE Freq[] = {0x00,0x00};
	 BYTE IIC_RDS1_CTR[5] = {0x00, 0x00, 0x35, 0x00, 0x20};
	 BYTE IIC_RDS2_CTR[5] = {0x00, 0x00, 0x3D, 0x00, 0x20};
	 UINT regAddr;

	 DBG2(debugOneData("\nFlyRadio SAF7741 set freq freq---->",freq);)
	 DBG2(debugOneData(" mode---->",mode);)
	 
	 if(mode != AM)
	 {
		 //Set Freq
		 temp = (freq / 5);
		 Freq[0] = (BYTE)(temp >> 8);
		 Freq[1] = (BYTE)temp;
		 //Set Band-->FM Standrad World
		 Freq[0] = (Freq[0] & 0x1F) | 0x80; 

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2,0x30,Freq,2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,Freq,2);

		 regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);
		 regAddr = (IIC_RDS1_CTR[0] << 16) + (IIC_RDS1_CTR[1] << 8) + IIC_RDS1_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS1_CTR[3],2);
	 }
	 else
	 {
		 ////tuner1
		 ////Set Freq
		 //Freq[0] = (BYTE)(freq >> 8);
		 //Freq[1] = (BYTE)freq;
		 ////Set Band-->FM Standrad World
		 //Freq[0] = (Freq[0] & 0x1F) | 0x20; 

		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2,0x30,Freq,2);

		 //regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 //I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);

		 //TUNER2
		 //Set Freq
		 Freq[0] = (BYTE)(freq >> 8);
		 Freq[1] = (BYTE)freq;
		 //Set Band-->FM Standrad World
		 Freq[0] = (Freq[0] & 0x1F) | 0x20; 

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,Freq,2);

		 //regAddr = (IIC_RDS1_CTR[0] << 16) + (IIC_RDS1_CTR[1] << 8) + IIC_RDS1_CTR[2];
		 //I2C_Write_SAF7741(regAddr,&IIC_RDS1_CTR[3],2);

		 regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);
	 }
 }

 void FlyRadio_ChangeToFMAM(BYTE mode)
 {
	 BYTE Freq[] = {0x00,0x00};
	 UINT temp;
	 BYTE IIC_RDS1_CTR[5] = {0x00, 0x00, 0x35, 0x00, 0x20};
	 BYTE IIC_RDS2_CTR[5] = {0x00, 0x00, 0x3D, 0x00, 0x20};
	 UINT regAddr;
	 BYTE Y2_FPHD_ERR_W_Fst[5] = {0x05, 0x11, 0x22, 0x02, 0x00};

	 BYTE TEF7000_1_FM_1[4] = {0x88, 0x66, 0x80, 0xC0};
	 BYTE TEF7000_2_FM_1[4] = {0x88, 0x66, 0x80, 0xC0};
	 BYTE TEF7000_1_FM_2[2] = {0x88, 0x66};
	 BYTE TEF7000_2_FM_2[2] = {0x88, 0x66};

	 BYTE TEF7000_1_AM_1[4] = {0x88, 0x66, 0x80, 0xC0};
	 BYTE TEF7000_2_AM_1[2] = {0x22, 0x0A};
	 BYTE TEF7000_2_AM_2[4] = {0x22, 0x0A, 0x00, 0x00};

	 if(mode != AM)
	 {
		 DBG2(debugString("\nFlyRadio SAF7741 TEF7000 ChangeToFMAM-->FM");)

		 //获得最新频点
		 temp = (*pFlyRadioInfo->radioInfo.pCurRadioFreq / 5);
		 Freq[0] = (BYTE)(temp >> 8);
		 Freq[1] = (BYTE)temp;
		 Freq[0] = (Freq[0] & 0x1F) | 0x80;
		 TEF7000_1_FM_1[0] = Freq[0];
		 TEF7000_1_FM_1[1] = Freq[1];
		 TEF7000_2_FM_1[0] = Freq[0];
		 TEF7000_2_FM_1[1] = Freq[1];
		 TEF7000_2_FM_2[0] = Freq[0];
		 TEF7000_2_FM_2[1] = Freq[1];
		 TEF7000_1_FM_2[0] = Freq[0];
		 TEF7000_1_FM_2[1] = Freq[1];

		 SendToSAF7741NormalWriteData(SAF7741_Radio_Init_FM_1);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0xD0, TEF7000_1_FM_1, 4);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2, 0xD0, TEF7000_2_FM_1, 4);

		 regAddr = (Y2_FPHD_ERR_W_Fst[0] << 16) + (Y2_FPHD_ERR_W_Fst[1] << 8) + Y2_FPHD_ERR_W_Fst[2];
		 I2C_Write_SAF7741(regAddr,&Y2_FPHD_ERR_W_Fst[2],2);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2, 0x30, TEF7000_2_FM_2, 2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2, 0x30, TEF7000_2_FM_2, 2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);

		 regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);

		 regAddr = (IIC_RDS1_CTR[0] << 16) + (IIC_RDS1_CTR[1] << 8) + IIC_RDS1_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS1_CTR[3],2);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1, 0x30, TEF7000_1_FM_2, 2);

		 SendToSAF7741NormalWriteData(SAF7741_Radio_Init_FM_2);
	 }
	 else
	 {
		 DBG2(debugString("\nFlyRadio SAF7741 TEF7000 ChangeToFMAM-->AM");)
		 ////TUNER 1

		 Freq[0] = (BYTE)(*pFlyRadioInfo->radioInfo.pCurRadioFreq >> 8);
		 Freq[1] = (BYTE)*pFlyRadioInfo->radioInfo.pCurRadioFreq;
		 Freq[0] = (Freq[0] & 0x1F) | 0x20; 
		 TEF7000_1_AM_1[0] = Freq[0];
		 TEF7000_1_AM_1[1] = Freq[1];
		 TEF7000_2_AM_1[0] = Freq[0];
		 TEF7000_2_AM_1[1] = Freq[1];
		 TEF7000_2_AM_2[0] = Freq[0];
		 TEF7000_2_AM_2[1] = Freq[1];

		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);

		 //SendToSAF7741NormalWriteData(SAF7741_Radio_Init_AM_1);

		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0xD0,TEF7000_1_AM_1,4);
		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2,0xD0,TEF7000_2_AM_2,4);

		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 //I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);

		 //SendToSAF7741NormalWriteData(SAF7741_Radio_Init_AM_2);

		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 //I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);

		 //SendToSAF7741NormalWriteData(SAF7741_Radio_Init_AM_3);



		 //TUNER 2
		 //I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);

		 SendToSAF7741NormalWriteData(SAF7741_Radio_Init_AM_1);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2,0xD0,TEF7000_1_AM_1,4);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0xD0,TEF7000_2_AM_2,4);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //regAddr = (IIC_RDS1_CTR[0] << 16) + (IIC_RDS1_CTR[1] << 8) + IIC_RDS1_CTR[2];
		 //I2C_Write_SAF7741(regAddr,&IIC_RDS1_CTR[3],2);

		 regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);

		 SendToSAF7741NormalWriteData(SAF7741_Radio_Init_AM_2);

		 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0x30,TEF7000_2_AM_1,2);
		 //regAddr = (IIC_RDS1_CTR[0] << 16) + (IIC_RDS1_CTR[1] << 8) + IIC_RDS1_CTR[2];
		 //I2C_Write_SAF7741(regAddr,&IIC_RDS1_CTR[3],2);

		 regAddr = (IIC_RDS2_CTR[0] << 16) + (IIC_RDS2_CTR[1] << 8) + IIC_RDS2_CTR[2];
		 I2C_Write_SAF7741(regAddr,&IIC_RDS2_CTR[3],2);

		 SendToSAF7741NormalWriteData(SAF7741_Radio_Init_AM_3);

	 }
	 pFlyRadioInfo->bCurMute = TRUE;//触发恢复静音状态
 }

 static UINT RadioStepFreqGenerate(BYTE eMode,UINT iFreq,BYTE eForward,BYTE eStepMode)
 {
	 //DBG2(debugOneData("\nFlyRadio SAF7741 Freq Generate Mode-->",eMode);)
  //   DBG2(debugOneData("\nFlyRadio SAF7741 Freq Generate Freq-->",iFreq);)
  //   DBG2(debugOneData("\nFlyRadio SAF7741 Freq Generate Forward-->",eForward);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 Freq Generate StepMode-->",eStepMode);)
	 if (STEP_FORWARD == eForward)//Forward
	 {	
		 if(AM != eMode)
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqFMManualStep;
			 }
			 else//自动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqFMScanStep;
			 }
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqFMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqFMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;

		 }
		 else
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqAMManualStep;
			 }
			 else//自动
			 {
				 iFreq += pFlyRadioInfo->radioInfo.iFreqAMScanStep;
			 }
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqAMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqAMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
		 }
	 }
	 else if (STEP_BACKWARD == eForward)//Backward
	 {
		 if(AM != eMode)
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqFMManualStep;
			 }
			 else//自动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqFMScanStep;
			 }
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqFMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMax;
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqFMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMax;
		 }
		 else
		 {
			 if(STEP_MANUAL == eStepMode)//手动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqAMManualStep;
			 }
			 else// if(RadioScanStatus == 1)//自动
			 {
				 iFreq -= pFlyRadioInfo->radioInfo.iFreqAMScanStep;
			 }
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqAMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMax;
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqAMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMax;
		 }
	 }
	 else {
		 if(AM != eMode)
		 {
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqFMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqFMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqFMMin;
		 }
		 else
		 {
			 if(iFreq > pFlyRadioInfo->radioInfo.iFreqAMMax)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
			 if(iFreq < pFlyRadioInfo->radioInfo.iFreqAMMin)	iFreq = pFlyRadioInfo->radioInfo.iFreqAMMin;
		 }
	 }
	 return iFreq;
 }

 static void FlyRadio_Mute(BOOL ctrl)
 {
	 if(ctrl)
	 {
		 while(ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID) || ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID))
		 {
			 Sleep(10);
		 }
		 ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID);//发送进入静音
		 while (!ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID))//等待OK
		 {
			 Sleep(10);
		 }
		 ipcClearEvent(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID);//清除
	 }
	 else
	 {
		 ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID);//发送退出静音
	 }
 }

 static BOOL bRadioSignalGood(BYTE radioMode,UINT *pLevel)
 {
	 BYTE bGood = 0;
	 BYTE bGoodLevel;
	 BYTE bNoiseLevel;
	 UINT regAddr;

	 BYTE regFMLevel[6] = {0x03,0x00,0xD9,0x00,0x00,0x00};//TDSP1E_X_wFDLE_1_LvlQpd_out//[-0,2..1>, i.e. [-20dBμV .. 100dBμV>.
	 UINT regdataFMLevel;
	 BYTE regFMUSNoise[6] = {0x01,0x01,0x51,0x00,0x00,0x00};//TDSP1_X_wFDUN_1_SlwAvg_out//Full Scale按比例0-1
	 UINT regdataFMUSNoise;
	 BYTE regFMOFS[6] = {0x03,0x00,0xE9,0x00,0x00,0x00};//TDSP1E_X_wFIOF_1_Ofs_out
	 UINT regdataFMOFS;

	 BYTE regAMLevel[6] = {0x03,0x00,0xAF,0x00,0x00,0x00};//TDSP1E_X_wADLE_1_LvlFlt_out
	 UINT regdataAMLevel;
	 BYTE regAMOFS[6] = {0x03,0x00,0x73,0x00,0x00,0x00};//TDSP1E_X_wAIOF_1_Ofs_out

	 if(radioMode != AM)
	 {
		 regAddr = (regFMLevel[0] << 16) + (regFMLevel[1] << 8) + regFMLevel[2];
		 I2C_Read_SAF7741(regAddr,&regFMLevel[3],3);
		 regdataFMLevel = rU8ComboToU32(&regFMLevel[3],3);
		 //DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 regdataFMLevel-->",regdataFMLevel);)
		 if(regdataFMLevel >= 0x00800000)regdataFMLevel = 0;
		 bGoodLevel = (BYTE)(regdataFMLevel*1.0*100/0x00800000);
		 DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 bGoodLevel-->",bGoodLevel);)

		 regAddr = (regFMUSNoise[0] << 16) + (regFMUSNoise[1] << 8) + regFMUSNoise[2];
		 I2C_Read_SAF7741(regAddr,&regFMUSNoise[3],3);
		 regdataFMUSNoise = rU8ComboToU32(&regFMUSNoise[3],3);
		 //DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 CMD regdataFMUSNoise-->",regdataFMUSNoise);)
		 bNoiseLevel = (BYTE) (regdataFMUSNoise*1.0*100/0x01000000);
		 DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 bNoiseLevel-->",bNoiseLevel);)

		 regAddr = (regFMOFS[0] << 16) + (regFMOFS[1] << 8) + regFMOFS[2];
		 I2C_Read_SAF7741(regAddr,&regFMOFS[3],3);
		 regdataFMOFS = rU8ComboToU32(&regFMOFS[3],3);
		 //DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 CMD regdataFMOFS-->",regdataFMOFS);)
		 if(regdataFMOFS >= 0x00800000)regdataFMOFS = 0x01000000 - regdataFMOFS;
		 regdataFMOFS = (BYTE)(regdataFMOFS*1.0*100/0x00800000);
		 DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 regdataFMOFS-->",regdataFMOFS);)

		 *pLevel = bGoodLevel;

		 if((bGoodLevel > FM_SCAN_STOP_LEVEL) && 
			(bNoiseLevel < FM_SCAN_STOP_NOISE) && 
			(regdataFMOFS < FM_SCAN_STOP_OFS))
		 {
			 return TRUE;
		 }
	 }	
	 else
	 {
		 regAddr = (regAMLevel[0] << 16) + (regAMLevel[1] << 8) + regAMLevel[2];
		 I2C_Read_SAF7741(regAddr,&regAMLevel[3],3);
		 regdataAMLevel = rU8ComboToU32(&regAMLevel[3],3);
		 DBG2(debugOneData("\nFlyRadio SAF7741 TEF7000 CMD regdataAMLevel-->",regdataAMLevel);)
		 if(regdataAMLevel >= 0x00800000)regdataAMLevel = 0;
		 bGoodLevel = (BYTE)(regdataAMLevel*1.0*100/0x00800000);//regdataAMLevel*1.0/0x00800000*100;

		 *pLevel = bGoodLevel;

		 if(bGoodLevel > AM_SCAN_STOP_LEVEL)//48
		 {
			 return TRUE;
		 }
	 }
	 return FALSE;
 }

 void FlyRadioJumpNewFreqParaInit(void)
 {
#if RADIO_RDS
	 RDSParaInit();
#endif
 }

 static void buttomJumpFreqAndReturn(void)//界面按键跳频用
 {
	 *pFlyRadioInfo->radioInfo.pPreRadioFreq = 
		 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode
		 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
		 ,pFlyRadioInfo->radioInfo.eButtomStepDirection
		 ,STEP_MANUAL);

	 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
 }

 static void FlyRadio_Scaning(BOOL Bon)
 {
	 BYTE Y1_FCIB_1_BWSetMan[5] = {0x01, 0x11, 0xA5, 0x00, 0x00};
	 BYTE X1_FCIB_1_FixSet[6] = {0x01, 0x03, 0x1E, 0x00, 0x00, 0x00};
	 UINT regAddr;

	 if (Bon)
	 {
		 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
		 {
			 Y1_FCIB_1_BWSetMan[3] = 0x07;
			 Y1_FCIB_1_BWSetMan[4] = 0xFF;
			 regAddr = (Y1_FCIB_1_BWSetMan[0] << 16) + (Y1_FCIB_1_BWSetMan[1] << 8) + Y1_FCIB_1_BWSetMan[2];
			 I2C_Write_SAF7741(regAddr,&Y1_FCIB_1_BWSetMan[3],2);

			 X1_FCIB_1_FixSet[3] = 0x00;
			 X1_FCIB_1_FixSet[4] = 0x00;
			 X1_FCIB_1_FixSet[5] = 0x07;
			 regAddr = (X1_FCIB_1_FixSet[0] << 16) + (X1_FCIB_1_FixSet[1] << 8) + X1_FCIB_1_FixSet[2];
			 I2C_Write_SAF7741(regAddr,&X1_FCIB_1_FixSet[3],3);
			 I2C_Write_SAF7741(regAddr,&X1_FCIB_1_FixSet[3],3);
		 }
	 }
	 else
	 {
		 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
		 {
			 X1_FCIB_1_FixSet[3] = 0x00;
			 X1_FCIB_1_FixSet[4] = 0x00;
			 X1_FCIB_1_FixSet[5] = 0x08;
			 regAddr = (X1_FCIB_1_FixSet[0] << 16) + (X1_FCIB_1_FixSet[1] << 8) + X1_FCIB_1_FixSet[2];
			 I2C_Write_SAF7741(regAddr,&X1_FCIB_1_FixSet[3],3);

			 Y1_FCIB_1_BWSetMan[3] = 0x00;
			 Y1_FCIB_1_BWSetMan[4] = 0x00;
			 regAddr = (Y1_FCIB_1_BWSetMan[0] << 16) + (Y1_FCIB_1_BWSetMan[1] << 8) + Y1_FCIB_1_BWSetMan[2];
			 I2C_Write_SAF7741(regAddr,&Y1_FCIB_1_BWSetMan[3],2);
		 }
	 }
 }

 static UINT GetCorrectScanStartFreq(UINT *pFreq)
 {
	 UINT base;
	 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
	 {
		 if (((*pFreq - pFlyRadioInfo->radioInfo.iFreqFMMin) % pFlyRadioInfo->radioInfo.iFreqFMScanStep) != 0)
		 {
			 base = 1;
		 }
		 else
		 {
			 base = 0;
		 }
		 *pFreq = pFlyRadioInfo->radioInfo.iFreqFMMin + (base + (*pFreq - pFlyRadioInfo->radioInfo.iFreqFMMin)/pFlyRadioInfo->radioInfo.iFreqFMScanStep)*pFlyRadioInfo->radioInfo.iFreqFMScanStep;
	 }
	 else
	 {
		 if (((*pFreq - pFlyRadioInfo->radioInfo.iFreqAMMin) % pFlyRadioInfo->radioInfo.iFreqAMScanStep) != 0)
		 {
			 base = 1;
		 }
		 else
		 {
			 base = 0;
		 }		
		 *pFreq = pFlyRadioInfo->radioInfo.iFreqAMMin + (base + (*pFreq - pFlyRadioInfo->radioInfo.iFreqAMMin)/pFlyRadioInfo->radioInfo.iFreqAMScanStep)*pFlyRadioInfo->radioInfo.iFreqAMScanStep;
	 }
	 /******************************************************************************/
	 //if (((*pFreq - pTDA7541RadioInfo->radioInfo.iFreqFMMin) % pTDA7541RadioInfo->radioInfo.iFreqFMScanStep) != 0)
	 //{
	 //	if (AM != pTDA7541RadioInfo->radioInfo.eCurRadioMode)
	 //	{
	 //		*pFreq = *pFreq + pTDA7541RadioInfo->radioInfo.iFreqFMScanStep;
	 //	}
	 //	else
	 //	{
	 //		*pFreq = *pFreq + pTDA7541RadioInfo->radioInfo.iFreqAMScanStep;
	 //	}

	 //}
	 /******************************************************************************/
	 return *pFreq;
 }

 /******************************************************************************/
 /*                               各种数据初始化                           */
 /******************************************************************************/
 void FlyRadio_SAF7741_TEF7000_Init(void)
 {
	 DBG2(debugString("\nFlyRadio SAF7741 TEF7000 init");)
	 BYTE TEF7000_1_Init_Data[7] = {0x86, 0xD6, 0x80, 0xC0, 0x00, 0x00, 0x00};
	 BYTE TEF7000_2_Init_Data[7] = {0x86, 0xD6, 0x80, 0xC0, 0x00, 0x00, 0x00};
	 //SendToSAF7741NormalWriteData(SAF7741_Radio_Init_Data);
	 //INFO: Initialise Tuner1 module....
	 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID1,0xD0,TEF7000_1_Init_Data,7);
	 //INFO: Initialise Tuner2 module....
	 I2C_Write_TEF7000(MSG_RADIO_CON_TEF7000_ID2,0xD0,TEF7000_2_Init_Data,7);
 }

 static void RegDataReadRadio(void)
 {
	 DBG2(debugString("\nFlyRadio SAF7741 read the freq last save !");)

	 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM1;
	 if ((pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 > 10800) || (pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 < 8750))
	 {
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
	 }
	 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM2;
	 if ((pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 > 10800) || (pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 < 8750))
	 {
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
	 }
	 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqAM;
	 if ((pFlyRadioInfo->radioInfo.iPreRadioFreqAM > 1620) || (pFlyRadioInfo->radioInfo.iPreRadioFreqAM < 522))
	 {
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 522;
	 }

	 //DBG2(debugOneData("\nFlyRadio SAF7741 read freq FM1---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 read freq FM2---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 read freq AM---->",pFlyRadioInfo->radioInfo.iPreRadioFreqAM);)
 }

 static void RegDataWriteRadio(void)
 {
	 DBG2(debugString("\nFlyRadio SAF7741 write the freq for next read !");)

	 pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM1 = pFlyRadioInfo->radioInfo.iPreRadioFreqFM1;
	 pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM2 = pFlyRadioInfo->radioInfo.iPreRadioFreqFM2;
	 pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqAM = pFlyRadioInfo->radioInfo.iPreRadioFreqAM;

	 //DBG2(debugOneData("\nFlyRadio SAF7741 write freq FM1---->",pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.dwFreqFM1);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 write freq FM2---->",pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 write freq AM---->",pFlyRadioInfo->radioInfo.iPreRadioFreqAM);)
 }

 static void radioIDChangePara(BYTE iID)
 {
	 if (0x00 == iID)//China
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 522;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1620;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 9;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 9;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 522;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 522;
		 /*******************************************************/
	 }
	 else if (0x01 == iID)//USA
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10790;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 20;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 530;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1720;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 530;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 530;
		 /*******************************************************/
	 }
	 else if (0x02 == iID)//S.Amer-1
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 530;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1710;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 530;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 530;
		 /*******************************************************/
	 }
	 else if (0x03 == iID)//S.Amer-2
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 10;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 10;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 520;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1600;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 5;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 520;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 520;
		 /*******************************************************/
	 }
	 else if (0x04 == iID)//KOREA
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8810;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10790;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 20;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 531;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1620;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 9;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 9;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8810;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8810;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8810;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8810;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 531;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 531;
		 /*******************************************************/
	 }
	 else if (0x05 == iID)//Thailand
	 {
		 pFlyRadioInfo->radioInfo.iFreqFMMin = 8750;
		 pFlyRadioInfo->radioInfo.iFreqFMMax = 10800;
		 pFlyRadioInfo->radioInfo.iFreqFMManualStep = 5;
		 pFlyRadioInfo->radioInfo.iFreqFMScanStep = 25;

		 pFlyRadioInfo->radioInfo.iFreqAMMin = 531;
		 pFlyRadioInfo->radioInfo.iFreqAMMax = 1602;
		 pFlyRadioInfo->radioInfo.iFreqAMManualStep = 9;
		 pFlyRadioInfo->radioInfo.iFreqAMScanStep = 9;

		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM1 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = 8750;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqFM2 = 8750;
		 /*******************************************************/
		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = 531;
		 /*******************************************************/
		 //pTDA7541RadioInfo->radioInfo.iCurRadioFreqAM = 531;
		 /*******************************************************/
	 }
	 if (AM == pFlyRadioInfo->radioInfo.ePreRadioMode)
	 {
		 returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqAM);
	 }
	 else if (FM1 == pFlyRadioInfo->radioInfo.ePreRadioMode)
	 {
		 returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);
	 }
	 else if (FM2 == pFlyRadioInfo->radioInfo.ePreRadioMode)
	 {
		 returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);
	 }
 }

 static void powerOnNormalInit(void)
 {
	 pFlyRadioInfo->bPowerUp = FALSE;

	 pFlyRadioInfo->bPreMute = FALSE;
	 pFlyRadioInfo->bCurMute = TRUE;

	 radioIDChangePara(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDDriver);

	 RegDataReadRadio();

	 pFlyRadioInfo->radioInfo.ePreRadioMode = FM1;
	 pFlyRadioInfo->radioInfo.eCurRadioMode = FM1;

	 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqFM1;
	 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqFM1;

	 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
	 pFlyRadioInfo->radioInfo.bCurScaning = FALSE;
	 pFlyRadioInfo->radioInfo.eScanDirection = STEP_FORWARD;	
	 pFlyRadioInfo->radioInfo.bScanRepeatFlag = FALSE;

	 pFlyRadioInfo->radioInfo.bPreStepButtomDown = FALSE;
	 /*********************************************************/
	 //pTDA7541RadioInfo->radioInfo.bCurStepButtomDown = FALSE;
	 /*********************************************************/
	 pFlyRadioInfo->radioInfo.eButtomStepDirection = STEP_FORWARD;
	 pFlyRadioInfo->radioInfo.iButtomStepCount = 0;

	 pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = FALSE;
	 pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = FALSE;

	 /*********************************************************/
	 //IO_Control_Init(pTDA7541RadioInfo);
	 /*********************************************************/

	 //DBG2(debugOneData("\nFlyRadio SAF7741 (RadioParaInit) iCurRadioFreqFM1---->",pFlyRadioInfo->radioInfo.iCurRadioFreqFM1);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 (RadioParaInit) iCurRadioFreqFM2---->",pFlyRadioInfo->radioInfo.iCurRadioFreqFM2);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 (RadioParaInit) iCurRadioFreqAM---->",pFlyRadioInfo->radioInfo.iCurRadioFreqAM);)
	 //DBG2(debugOneData("\nFlyRadio SAF7741 (RadioParaInit) pCurRadioFreq---->",*pFlyRadioInfo->radioInfo.pCurRadioFreq);)
 }

 static void powerOnFirstInit(void)
 {
	 pFlyRadioInfo->bOpen = FALSE;

	 pFlyRadioInfo->bKillRadioMainThread = TRUE;
	 sem_init(&pFlyRadioInfo->MainThread_sem, 0, 0);

	 pFlyRadioInfo->bKillRadioScanThread = TRUE;
	 pthread_mutex_init(&pFlyRadioInfo->ScanThreadMutex, NULL);
	 pthread_cond_init(&pFlyRadioInfo->ScanThreadCond, NULL);

	 pFlyRadioInfo->bScanThreadRunAgain = FALSE;
	 pFlyRadioInfo->bRDSThreadGO = FALSE;

	 pFlyRadioInfo->bKillRadioRDSRecThread = TRUE;
	 pthread_mutex_init(&pFlyRadioInfo->RDSRecThreadMutex, NULL);
	 pthread_cond_init(&pFlyRadioInfo->RDSRecThreadCond, NULL);
 }

 /******************************************************************************/
 /*                                各种线程函数                            */
 /******************************************************************************/
 void *radio_main_thread(void *arg)
 {
	 static UINT RadioScanStatus;

	 while (!pFlyRadioInfo->bKillRadioMainThread)
	 {
		 sem_wait(&pFlyRadioInfo->MainThread_sem);;

		 DBG2(debugString("\nFlyRadio SAF7741 MainThread Running!");)

		 //if (ipcWhatEventOn(EVENT_GLOBAL_REG_RESTORE_RADIO_ID))
		 //{
			// ipcClearEvent(EVENT_GLOBAL_REG_RESTORE_RADIO_ID);
			// if (pFlyAllInOneInfo->pMemory_Share_Common.bNeedRestoreRegeditRadio)
			// {
			//	 pFlyAllInOneInfo->pMemory_Share_Common.bNeedRestoreRegeditRadio = FALSE;
			//	 if (pFlyAllInOneInfo->pMemory_Share_Common.bRestoreRegeditToFactory)
			//	 {
			//		 radioIDChangePara(pFlyAllInOneInfo->pMemory_Share_Common.iRadioIDUser);
			//		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = pFlyRadioInfo->radioInfo.iFreqFMMin;
			//		 pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = pFlyRadioInfo->radioInfo.iFreqFMMin;
			//		 pFlyRadioInfo->radioInfo.iPreRadioFreqAM = pFlyRadioInfo->radioInfo.iFreqAMMin;
			//		 pFlyRadioInfo->radioInfo.iCurRadioFreqFM1 = pFlyRadioInfo->radioInfo.iFreqFMMin;
			//		 pFlyRadioInfo->radioInfo.iCurRadioFreqFM2 = pFlyRadioInfo->radioInfo.iFreqFMMin;
			//		 pFlyRadioInfo->radioInfo.iCurRadioFreqAM = pFlyRadioInfo->radioInfo.iFreqAMMin;
			//	 }
			//	 RegDataWriteRadio();
			// }
		 //}

		 if (FALSE == pFlyRadioInfo->bPowerUp)
		 {
		 }
		 else
		 {
			 if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCESleepMsg)
			 {
				 continue;
			 }

			 if (!pFlyAllInOneInfo->pMemory_Share_Common->b7741RadioInitFinish)
			 {
				 pFlyAllInOneInfo->pMemory_Share_Common->iNeedProcVoltageShakeRadio = 85;

				 while (!pFlyAllInOneInfo->pMemory_Share_Common->b7741AudioInitFinish)
				 {
					 Sleep(10);
				 }

				 FlyRadio_SAF7741_TEF7000_Init();
				 
				 FlyRadio_ChangeToFMAM(pFlyRadioInfo->radioInfo.eCurRadioMode);
				 if (*pFlyRadioInfo->radioInfo.pCurRadioFreq != *pFlyRadioInfo->radioInfo.pPreRadioFreq)
				 {
					 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
				 }
				 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

				 pFlyAllInOneInfo->pMemory_Share_Common->b7741RadioInitFinish = TRUE;

				 pFlyRadioInfo->bRDSThreadGO = TRUE;

				 pFlyRadioInfo->bPowerUp = TRUE;
			 }
			 if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_CHANGE_ID))
			 {
				 ipcClearEvent(EVENT_GLOBAL_RADIO_CHANGE_ID);
				 DBG2(debugString("\nFlyRadio SAF7741 ID Change!");)
				 radioIDChangePara(pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iRadioIDUser);
			 }

			 if (ipcWhatEventOn(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID))
			 {
				 ipcClearEvent(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID);
				 if (pFlyAllInOneInfo->pMemory_Share_Common->eCurAudioInput == RADIO)
				 {
					 while (!pFlyAllInOneInfo->pMemory_Share_Common->b7741AudioInitFinish)
					 {
						 Sleep(10);
					 }

					 FlyRadio_SAF7741_TEF7000_Init();
					 
					 FlyRadio_ChangeToFMAM(pFlyRadioInfo->radioInfo.eCurRadioMode);
					 if (*pFlyRadioInfo->radioInfo.pCurRadioFreq != *pFlyRadioInfo->radioInfo.pPreRadioFreq)
					 {
						 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
					 }
					 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

					 pFlyAllInOneInfo->pMemory_Share_Common->b7741RadioInitFinish = TRUE;
				 }
			 }

			 //if (ipcWhatEventOn(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID))
			 //{
				// DBG2(debugString("\nFlyRadio SAF7741 Voltage After Low Proc");)
				// ipcClearEvent(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID);

				// if (pFlyAllInOneInfo->pMemory_Share_Common->eCurAudioInput == RADIO)
				// {
				//	 FlyRadio_SAF7741_TEF7000_Init();

				//	 FlyRadio_ChangeToFMAM(pFlyRadioInfo->radioInfo.eCurRadioMode);
				//	 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);
				// }
			 //}

			 if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_ANTENNA_ID))
			 {
				 DBG2(debugString("\nFlyRadio SAF7741 ANTENNA ctrl");)
				 ipcClearEvent(EVENT_GLOBAL_RADIO_ANTENNA_ID);
				 if (pFlyAllInOneInfo->pMemory_Share_Common->eAudioInput == RADIO)
				 {
					 radioANTControl(TRUE);
				 }
				 else
				 {
					 radioANTControl(FALSE);
				 }
			 }
			 pthread_mutex_lock(&pFlyRadioInfo->ScanThreadMutex);
			 pthread_cond_signal(&pFlyRadioInfo->ScanThreadCond);
			 pFlyRadioInfo->bScanThreadRunAgain = TRUE;
			 pthread_mutex_unlock(&pFlyRadioInfo->ScanThreadMutex);
			 //SetEvent(pTDA7541RadioInfo->hDispatchScanThreadEvent);
		 }
	 }
	 DBG2(debugString("\nFlyRadio SAF7741 MainThread Prepare exit!");)

	 pFlyRadioInfo->bPowerUp = FALSE;
	 pFlyAllInOneInfo->pMemory_Share_Common->iNeedProcVoltageShakeRadio = 0;

	 DBG2(debugString("\nFlyRadio SAF7741 MainThread exit!");)
	 return NULL;
 }

 void *radio_scan_thread(void *arg)
 {
	 //INT ret = 0;
	 //struct timeval timenow;
	 //struct timespec timeout;
	 UINT iScanStartFreq = 0;
	 UINT m_RF_Freq;
	 BOOL bHaveSearched = 0;
	 UINT iHaveSearchedLevel = 0;
	 UINT iTempHaveSearchedLevel = 0;
	 ULONG nowTimer,lastTimer;
	 ULONG iThreadFreqStepWaitTime = 0;

	 //ULONG tBlinkingTimerStart,tBlinkingTimerEnd;
	 BYTE iBlinkingTimes;
	 BOOL bBlinkingStatus;

	 DBG2(debugString("\nFlyRadio SAF7741 ScanThread in");)
	 while (!pFlyRadioInfo->bKillRadioScanThread)
	 {
		 WaitSignedTimeOut(&pFlyRadioInfo->ScanThreadMutex,&pFlyRadioInfo->ScanThreadCond,&pFlyRadioInfo->bScanThreadRunAgain,iThreadFreqStepWaitTime);
		 iThreadFreqStepWaitTime = 0;

		 //debugOneData("\nFlyRadio SAF7741 ScanThread Running Start ThreadFreqStepWaitTime---->",iThreadFreqStepWaitTime);

		 if (pFlyRadioInfo->radioInfo.bCurScaning == TRUE && pFlyRadioInfo->radioInfo.bPreScaning == FALSE)//跳出搜台
		 {
			 pFlyRadioInfo->radioInfo.bCurScaning = pFlyRadioInfo->radioInfo.bPreScaning;
			 FlyRadio_Scaning(FALSE);
			 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
			 returnRadioScanCtrl(0x03);//stop

			 pFlyRadioInfo->bCurMute = !pFlyRadioInfo->bPreMute;
		 }
		 //设置波段
		 if (pFlyRadioInfo->radioInfo.eCurRadioMode != pFlyRadioInfo->radioInfo.ePreRadioMode)
		 {
			 //DBG2(debugString("\nFlyRadio SAF7741 ScanThread --------------change mode");)
			 if (pFlyAllInOneInfo->pMemory_Share_Common->bAudioMuteControlable)
			 {
				 while(ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID) || ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID))
				 {
					 Sleep(10);
				 }
				 ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID);//发送进入静音
				 while (!ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID))//等待OK
				 {
					 Sleep(10);
				 }
				 ipcClearEvent(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID);//清除
			 }

			 pFlyRadioInfo->radioInfo.eCurRadioMode = pFlyRadioInfo->radioInfo.ePreRadioMode;			
			 if (FM1 == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqFM1;
				 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqFM1;
			 }
			 else if (FM2 == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqFM2;
				 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqFM2;
			 }
			 else if (AM == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 pFlyRadioInfo->radioInfo.pPreRadioFreq = &pFlyRadioInfo->radioInfo.iPreRadioFreqAM;
				 pFlyRadioInfo->radioInfo.pCurRadioFreq = &pFlyRadioInfo->radioInfo.iCurRadioFreqAM;
			 }
			 FlyRadio_ChangeToFMAM(pFlyRadioInfo->radioInfo.eCurRadioMode);
			 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

			 FlyRadioJumpNewFreqParaInit();

			 if (pFlyAllInOneInfo->pMemory_Share_Common->bAudioMuteControlable)
			 {
				 if (AM == pFlyRadioInfo->radioInfo.eCurRadioMode)
				 {
					 Sleep(314);
				 }
				 ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID);//发送退出静音
			 }
		 }
		 //设置频率
		 if (*pFlyRadioInfo->radioInfo.pCurRadioFreq != *pFlyRadioInfo->radioInfo.pPreRadioFreq)
		 {
			 //DBG2(debugString("\nFlyRadio SAF7741 ScanThread --------------change freq");)

			 //FlyRadio_Mute(TRUE);

			 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
			 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

			 FlyRadioJumpNewFreqParaInit();
			 if (pFlyRadioInfo->bPreMute == FALSE)
			 {
				 FlyRadio_Mute(FALSE);//取消静音
			 }

			 RegDataWriteRadio();
		 }

		 if(pFlyRadioInfo->bPreMute != pFlyRadioInfo->bCurMute)//收音机静音开关
		 {
			 if (pFlyRadioInfo->bPreMute == TRUE)
			 {
				 FlyRadio_Mute(TRUE); // mute
			 }
			 else
			 {
				 FlyRadio_Mute(FALSE);// demute
			 }
			 pFlyRadioInfo->bCurMute = pFlyRadioInfo->bPreMute;
		 }

		 if (pFlyRadioInfo->radioInfo.bPreStepButtomDown)//按下，则持续跳台
		 {
			 if (0 == pFlyRadioInfo->radioInfo.iButtomStepCount)
			 {
				 pFlyRadioInfo->radioInfo.iButtomStepCount++;
				 iThreadFreqStepWaitTime = 314;
			 }
			 else
			 {
				 buttomJumpFreqAndReturn();
				 pFlyRadioInfo->radioInfo.iButtomStepCount++;
				 iThreadFreqStepWaitTime = 100;
			 }
			 continue;//跳到开头
		 }
		 nowTimer = GetTickCount();
		 lastTimer = nowTimer;
		 while(pFlyRadioInfo->radioInfo.bPreScaning)//搜索
		 {
			 nowTimer = GetTickCount();
			 DBG2(debugString("\nFlyRadio SAF7741 ScanThread --------------scaning ....");)	

			 FlyRadio_Mute(TRUE); // mute

			 if (pFlyRadioInfo->radioInfo.bPreScaning == FALSE)
			 {
				 pFlyRadioInfo->radioInfo.bCurScaning = TRUE;
			 }
			 else
			 {
				 if (pFlyRadioInfo->radioInfo.bCurScaning != pFlyRadioInfo->radioInfo.bPreScaning)//起始搜索频率
				 {
					 FlyRadio_Scaning(TRUE);

					 pFlyRadioInfo->radioInfo.bCurScaning = pFlyRadioInfo->radioInfo.bPreScaning;
					 iScanStartFreq = GetCorrectScanStartFreq(pFlyRadioInfo->radioInfo.pPreRadioFreq);
					 bHaveSearched = FALSE;
					 iHaveSearchedLevel = 0;
				 }
					
				 DBG2(debugOneData("\nFlyRadio SAF7741 ScanThread iScanStartFreq-->",iScanStartFreq);)
				 DBG2(debugOneData("   PreRadioFreq-->",*pFlyRadioInfo->radioInfo.pPreRadioFreq);)

				 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
				 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
				 ,pFlyRadioInfo->radioInfo.eScanDirection
				 ,STEP_SCAN);

				 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
				 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

				 if (iScanStartFreq == *pFlyRadioInfo->radioInfo.pPreRadioFreq )//一圈都没好台
				 {
					 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
					 pFlyRadioInfo->radioInfo.bCurScaning = TRUE;

					 FlyRadio_Scaning(FALSE);

					 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
					 returnRadioScanCtrl(0x03);//stop
					 if (pFlyRadioInfo->bPreMute == FALSE)//收到台要出声音
					 {
						 FlyRadio_Mute(FALSE);
					 }
				 }
				 if ((nowTimer - lastTimer) >  157)//定时返回频点
				 {
					 lastTimer = nowTimer;
					 if (pFlyRadioInfo->radioInfo.bPreScaning)
					 {
						 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
					 }
				 }
				 if (AM != pFlyRadioInfo->radioInfo.eCurRadioMode)
				 {
					 Sleep(10);
				 } 
				 else
				 {
					 Sleep(138);
				 }

				 if (bRadioSignalGood(pFlyRadioInfo->radioInfo.eCurRadioMode,&iTempHaveSearchedLevel))
				 {
					 bHaveSearched = TRUE;
					 //DBG0(debugOneData("\n!!!!!!!!!!!!!!!!!!!!!!!!iTempHaveSearchedLevel-->",iTempHaveSearchedLevel);)
					 if (iHaveSearchedLevel > iTempHaveSearchedLevel)//OK
					 {
						 if (STEP_BACKWARD == pFlyRadioInfo->radioInfo.eScanDirection)
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_FORWARD
								 ,STEP_SCAN);
						 } 
						 else
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_BACKWARD
								 ,STEP_SCAN);
						 }

						 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
						 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

						 if (pFlyRadioInfo->bPreMute == FALSE)//收到台要出声音
						 {
							 FlyRadio_Mute(FALSE);
						 }

						 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
						 returnRadioHaveSearched(TRUE);

						 if (pFlyRadioInfo->radioInfo.bScanRepeatFlag == FALSE)
						 {
							 FlyRadio_Scaning(FALSE);

							 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
							 pFlyRadioInfo->radioInfo.bCurScaning = FALSE;
							 returnRadioScanCtrl(0x03);//stop							
							 break;
						 }
						 else
						 {
							 //blinking 5 times
							 iBlinkingTimes = 0;
							 bBlinkingStatus = TRUE;
							 while (pFlyRadioInfo->radioInfo.bPreScaning == TRUE && iBlinkingTimes < 10)
							 {
								 Sleep(500);
								 iBlinkingTimes++;
								 bBlinkingStatus = !bBlinkingStatus;
								 returnRadioBlinkingStatus(bBlinkingStatus);							
							 }
							 returnRadioBlinkingStatus(TRUE);
							 Sleep(500);
							 if (pFlyRadioInfo->radioInfo.bPreScaning == TRUE)
							 {
								 returnRadioScanCtrl(0x04);//repeat
							 }

							 iScanStartFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
							 bHaveSearched = FALSE;
							 iHaveSearchedLevel = 0;

							 ////blinking 5 times
							 //iBlinkingTimes = 0;
							 //bBlinkingStatus = TRUE;
							 //while (pFlyRadioInfo->radioInfo.bPreScaning == TRUE && iBlinkingTimes < 6)
							 //{
								// Sleep(500);
								// iBlinkingTimes++;
								// //bBlinkingStatus = !bBlinkingStatus;
								// //returnRadioBlinkingStatus(bBlinkingStatus);							
							 //}
							 ////returnRadioBlinkingStatus(TRUE);
							 ////Sleep(500);
							 //if (pFlyRadioInfo->radioInfo.bPreScaning == TRUE)
							 //{
								// returnRadioScanCtrl(0x04);//repeat
							 //}

							 //iScanStartFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
							 //bHaveSearched = FALSE;
							 //iHaveSearchedLevel = 0;
						 }
					 }				
				 }
				 else
				 {
					 if (bHaveSearched)
					 {
						 if (STEP_BACKWARD == pFlyRadioInfo->radioInfo.eScanDirection)
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_FORWARD
								 ,STEP_SCAN);
						 } 
						 else
						 {
							 *pFlyRadioInfo->radioInfo.pPreRadioFreq = RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.eCurRadioMode
								 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
								 ,STEP_BACKWARD
								 ,STEP_SCAN);
						 }

						 *pFlyRadioInfo->radioInfo.pCurRadioFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
						 FlyRadio_Set_Freq(pFlyRadioInfo->radioInfo.eCurRadioMode,*pFlyRadioInfo->radioInfo.pCurRadioFreq);

						 if (pFlyRadioInfo->bPreMute == FALSE)//收到台要出声音
						 {
							 FlyRadio_Mute(FALSE);
						 }

						 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
						 returnRadioHaveSearched(TRUE);

						 if (pFlyRadioInfo->radioInfo.bScanRepeatFlag == FALSE)
						 {
							 FlyRadio_Scaning(FALSE);

							 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
							 pFlyRadioInfo->radioInfo.bCurScaning = FALSE;
							 returnRadioScanCtrl(0x03);//stop							
							 break;
						 }
						 else
						 {
							 //blinking 5 times
							 iBlinkingTimes = 0;
							 bBlinkingStatus = TRUE;
							 while (pFlyRadioInfo->radioInfo.bPreScaning == TRUE && iBlinkingTimes < 10)
							 {
								 Sleep(500);
								 iBlinkingTimes++;
								 bBlinkingStatus = !bBlinkingStatus;
								 returnRadioBlinkingStatus(bBlinkingStatus);					
							 }
							 returnRadioBlinkingStatus(TRUE);
							 Sleep(500);
							 if (pFlyRadioInfo->radioInfo.bPreScaning == TRUE)
							 {
								 returnRadioScanCtrl(0x04);//repeat
							 }

							 iScanStartFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
							 bHaveSearched = FALSE;
							 iHaveSearchedLevel = 0;

							 ////收台不闪烁
							 //iBlinkingTimes = 0;
							 //bBlinkingStatus = TRUE;
							 //while (pFlyRadioInfo->radioInfo.bPreScaning == TRUE && iBlinkingTimes < 10)
							 //{
								// Sleep(500);
								// iBlinkingTimes++;
								// //bBlinkingStatus = !bBlinkingStatus;
								// //returnRadioBlinkingStatus(bBlinkingStatus);					
							 //}
							 ////returnRadioBlinkingStatus(TRUE);
							 ////Sleep(500);
							 //if (pFlyRadioInfo->radioInfo.bPreScaning == TRUE)
							 //{
								// returnRadioScanCtrl(0x04);//repeat
							 //}

							 //iScanStartFreq = *pFlyRadioInfo->radioInfo.pPreRadioFreq;
							 //bHaveSearched = FALSE;
							 //iHaveSearchedLevel = 0;
						 }
					 }
				 }
				 iHaveSearchedLevel = iTempHaveSearchedLevel;
			 }
		 }
		 DBG0(debugString("\nFlyRadio SAF7741 ScanThread Running End");)
	}
		DBG0(debugString("\nFlyRadio SAF7741 ScanThread exit!");)
		return NULL;
 }

 /******************************************************************************/
 /*                                创建各种线程                            */
 /******************************************************************************/
 static INT create_thread(void)
 {
	 INT res;
	 pthread_t thread_id;

	 //创建线程 （1）线程ID，
	 //		    （2）线程属性
	 //		    （3）线程函数起始地址
	 //		    （4）线程函数的参数

	 //MAIN线程
	 pFlyRadioInfo->bKillRadioMainThread = FALSE;
	 res = pthread_create(&thread_id,NULL,radio_main_thread,NULL);
	 DBG0(debugOneData("\nFlyRadio SAF7741 radio_main_thread ID---->",thread_id);)
	 if(res != 0) 
	 {
		 pFlyRadioInfo->bKillRadioMainThread = TRUE;
		 return -1;
	 }

	 //SCAN线程
	 pFlyRadioInfo->bKillRadioScanThread = FALSE;
	 res = pthread_create(&thread_id,NULL,radio_scan_thread,NULL);
	 DBG0(debugOneData("\nFlyRadio SAF7741 radio_scan_thread ID---->",thread_id);)
	 if(res != 0) 
	 {
		 pFlyRadioInfo->bKillRadioScanThread = TRUE;
		 return -1;
	 }

	 //RDSREC线程
	 pFlyRadioInfo->bKillRadioRDSRecThread = FALSE;
	 res = pthread_create(&thread_id,NULL,radio_rdsrec_thread,NULL);
	 DBG0(debugOneData("\nFlyRadio SAF7741 radio_rds_thread ID---->",thread_id);)
	 if(res != 0) 
	 {
		 pFlyRadioInfo->bKillRadioScanThread = TRUE;
		 return -1;
	 }

	 return 0;
 }

 /******************************************************************************/
 /*                                  处理数据                              */
 /******************************************************************************/
 static void DealRightDataProcessor(BYTE *buf, UINT16 len)
 {
	 UINT iTemp;

	 switch(buf[0])
	 {
	 case 0x01:
		 if (0x01 == buf[1])//初始化命令开始
			{
				DBG2(debugString("\nFlyRadio SAF7741 driver init");)

				FlyRadioJumpNewFreqParaInit();

				returnRadioPowerMode(TRUE);
				returnRadioInitStatus(TRUE);

				if (FALSE == pFlyRadioInfo->bPowerUp)
				{
					pFlyRadioInfo->bPowerUp = TRUE;
				}
				
				sem_post(&pFlyRadioInfo->MainThread_sem);//激活一次
			}
		 else if (0x00 == buf[1])
			{
				returnRadioPowerMode(FALSE);
			}
		 break;
	 case 0x03://软件模拟按键 1-FM1 2-FM2 3-AM 4-STOP RADIO 5-AF 6-TA
		 switch(buf[1])
			{
		 case 0x01:
		 case 0x02:
		 case 0x03:
			 if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
			 if (pFlyRadioInfo->bPowerUp)//直到之前的扫描停止
				{	
					sem_post(&pFlyRadioInfo->MainThread_sem);
					while (pFlyRadioInfo->radioInfo.bCurScaning)
					{
						Sleep(100);
					}
				}
			 if (0x01 == buf[1])
				{
					pFlyRadioInfo->radioInfo.ePreRadioMode = FM1;
					//DBG2(debugString("\nFlyRadio SAF7741 set mode --------FM1");)
				} 
			 else if (0x02 == buf[1])
				{
					pFlyRadioInfo->radioInfo.ePreRadioMode = FM2;
					//DBG2(debugString("\nFlyRadio SAF7741 set mode --------FM2");)
				}
			 else if (0x03 == buf[1])
				{
					pFlyRadioInfo->radioInfo.ePreRadioMode = AM;
					//DBG2(debugString("\nFlyRadio SAF7741 set mode --------AM");)
				}

			 returnRadioMode(pFlyRadioInfo->radioInfo.ePreRadioMode);
			 if (AM == pFlyRadioInfo->radioInfo.ePreRadioMode)
				{
					returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqAM);
				}
			 else if (FM1 == pFlyRadioInfo->radioInfo.ePreRadioMode)
				{
					returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM1);
				}
			 else if (FM2 == pFlyRadioInfo->radioInfo.ePreRadioMode)
				{
					returnRadioFreq(pFlyRadioInfo->radioInfo.iPreRadioFreqFM2);
				}

			 if (pFlyRadioInfo->bPowerUp)
				{	
					sem_post(&pFlyRadioInfo->MainThread_sem);
				}
			 break;
		 case 0x04:
			 pFlyRadioInfo->radioInfo.bPreScaning = !pFlyRadioInfo->radioInfo.bPreScaning;
			 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			 break;
		 case 0x05:
			 pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = !pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn;
			 returnRadioAFStatus(pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn);
			 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			break;
		 case 0x06:
			 pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = !pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn;
			 returnRadioTAStatus(pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn);
			 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			break;
		 case 0x07:
			 if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
			 if (pFlyRadioInfo->radioInfo.ePreRadioMode == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 *pFlyRadioInfo->radioInfo.pPreRadioFreq = 
					 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode
					 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
					 ,STEP_FORWARD
					 ,STEP_MANUAL);
				 //DBG2(debugOneData("\nFlyRadio SAF7741 set freq value---->",*pFlyRadioInfo->radioInfo.pPreRadioFreq);)
					 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
				 if (pFlyRadioInfo->bPowerUp)
				 {
					 sem_post(&pFlyRadioInfo->MainThread_sem);
				 }
			 }
			 break;
		 case 0x08:
			 if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
			 if (pFlyRadioInfo->radioInfo.ePreRadioMode == pFlyRadioInfo->radioInfo.eCurRadioMode)
			 {
				 *pFlyRadioInfo->radioInfo.pPreRadioFreq = 
					 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode
					 ,*pFlyRadioInfo->radioInfo.pPreRadioFreq
					 ,STEP_BACKWARD
					 ,STEP_MANUAL);
					 //DBG2(debugOneData("\nFlyRadio SAF7741 set freq value---->",*pFlyRadioInfo->radioInfo.pPreRadioFreq);)
					 returnRadioFreq(*pFlyRadioInfo->radioInfo.pPreRadioFreq);
				 if (pFlyRadioInfo->bPowerUp)
					{
						sem_post(&pFlyRadioInfo->MainThread_sem);
					}
			 }
			 break;
		 default:
			 DBG2(debugOneData("\nFlyRadio SAF7741 user command Key unDeal---->",buf[1]);)
			 break;
			}
		 break;
	 case 0x10://设置收音机频率 
		 if (pFlyRadioInfo->radioInfo.bPreScaning)
			{
				pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			}
		 iTemp = buf[1]*256+buf[2];
		 iTemp = 
			 RadioStepFreqGenerate(pFlyRadioInfo->radioInfo.ePreRadioMode,iTemp,STEP_NONE,STEP_MANUAL);
		 if (AM == pFlyRadioInfo->radioInfo.ePreRadioMode)
			{
				pFlyRadioInfo->radioInfo.iPreRadioFreqAM = iTemp;
			}
		 else if (FM1 == pFlyRadioInfo->radioInfo.ePreRadioMode)
			{
				pFlyRadioInfo->radioInfo.iPreRadioFreqFM1 = iTemp;
			}
		 else if (FM2 == pFlyRadioInfo->radioInfo.ePreRadioMode)
			{
				pFlyRadioInfo->radioInfo.iPreRadioFreqFM2 = iTemp;
			}
			//DBG2(debugOneData("\nFlyRadio SAF7741 set freq value---->",iTemp);)
			returnRadioFreq(iTemp);
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x11://频点+
		 if (pFlyRadioInfo->radioInfo.bPreScaning)
			{
				pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			}
		 pFlyRadioInfo->radioInfo.eButtomStepDirection = STEP_FORWARD;
		 if (0x00 == buf[1])
			{
				/**************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/**************************************************/
				buttomJumpFreqAndReturn();
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = TRUE;
				pFlyRadioInfo->radioInfo.iButtomStepCount = 0;
			}
		 else if (0x01 == buf[1])
			{
				/**************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/**************************************************/
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = FALSE;
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x12://频点-
		 if (pFlyRadioInfo->radioInfo.bPreScaning)
			{
				pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			}
		 pFlyRadioInfo->radioInfo.eButtomStepDirection = STEP_BACKWARD;
		 if (0x00 == buf[1])
			{
				/*****************************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/*****************************************************************/
				buttomJumpFreqAndReturn();
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = TRUE;
				pFlyRadioInfo->radioInfo.iButtomStepCount = 0;
			}
		 else if (0x01 == buf[1])
			{
				/*****************************************************************/
				//TDA7541RadioReturnToUser(pTDA7541RadioInfo,pdata,len);
				/*****************************************************************/
				pFlyRadioInfo->radioInfo.bPreStepButtomDown = FALSE;
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x13://扫描控制
		 switch(buf[1])
			{
		 case 0x00:
		 case 0x01:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_FORWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = FALSE;
			 break;
		 case 0x02:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_BACKWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = FALSE;
			 break;
		 case 0x03:
			 pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
			 break;
		 case 0x04:
		 case 0x05:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_FORWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = TRUE;
			 break;
		 case 0x06:
			 pFlyRadioInfo->radioInfo.bPreScaning = TRUE;
			 pFlyRadioInfo->radioInfo.eScanDirection = STEP_BACKWARD;
			 pFlyRadioInfo->radioInfo.bScanRepeatFlag = TRUE;
			 break;
		 default:break;
			}
		 returnRadioScanCtrl(buf[1]);
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
			//DBG2(debugOneData("\nFlyRadio SAF7741 set scan ctrl---->",buf[1]);)
			break;
	 case 0x15://开始收音
		 if (0x00 == buf[1])
			{
				pFlyRadioInfo->bPreMute = TRUE;
				if (pFlyRadioInfo->radioInfo.bPreScaning)
				{
					pFlyRadioInfo->radioInfo.bPreScaning = FALSE;
				}
				returnRadioMuteStatus(FALSE);
			}
		 else if (0x01 == buf[1])
			{
				pFlyRadioInfo->bPreMute = FALSE;
				returnRadioMuteStatus(TRUE);
		    }				
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
		    }			
		 break;
	 case 0x16://AF开关
		 if (0x01 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = TRUE;
				returnRadioAFStatus(TRUE);
			}
		 else if (0x00 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = FALSE;
				returnRadioAFStatus(FALSE);
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x17://TA开关
		 if (0x01 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = TRUE;
				returnRadioTAStatus(TRUE);
			}
		 else if (0x00 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSTAControlOn = FALSE;
				returnRadioTAStatus(FALSE);
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0x30://交通广播开关
		 if (0x01 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = TRUE;
				returnRadioRDSWorkStatus(TRUE);				
			}
		 else if (0x00 == buf[1])
			{
				pFlyRadioInfo->RDSInfo.RadioRDSAFControlOn = FALSE;
				returnRadioRDSWorkStatus(FALSE);
			}
		 if (pFlyRadioInfo->bPowerUp)
			{
				sem_post(&pFlyRadioInfo->MainThread_sem);
			}
		 break;
	 case 0xFF:
		 if (0x01 == buf[1])
			{
				//FRA_PowerUp((DWORD)pTDA7541RadioInfo);
		    } 
		 else if (0x00 == buf[1])
			{
				//FRA_PowerDown((DWORD)pTDA7541RadioInfo);
			}
		 break;
	 default:
		 DBG0(debugOneData("\nFlyRadio SAF7741 user command unhandle---->",buf[1]);)
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
	INT ret = HAL_ERROR_RETURN_FD;
	 //创建线程
	 if (create_thread() == -1)
	 {
		 DBG0(debugString("\nFlyRadio SAF7741 create thread error");)
		 return ret;
	 }
	 DBG0(debugString("\nFlyRadio SAF7741 create all ok");)
	
	 ret = HAL_RADIO_RETURN_FD;
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
	 //为 flyradio_struct_info 结构体分配内存
	 pFlyRadioInfo =
		 (struct flyradio_struct_info *)malloc(sizeof(struct flyradio_struct_info));
	 if (pFlyRadioInfo == NULL)
	 {
		 return;
	 }
	 memset(pFlyRadioInfo, 0, sizeof(struct flyradio_struct_info));

	 powerOnFirstInit();
	 allInOneInit();

	////参数初始化
	powerOnNormalInit();

	pFlyRadioInfo->bPowerUp = FALSE;
	debugString("\nFlyRadio SAF7741 init---->2012-03-17");
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
	 DBG1(debugBuf("\nRADIO-HAL return  bytes Start:", buf,1);)
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nRADIO-HAL return  bytes to User:", buf,dwRead);)
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
	pFlyRadioInfo->bKillRadioMainThread = TRUE;
	pFlyRadioInfo->bKillRadioScanThread = TRUE;
	pFlyRadioInfo->bKillRadioRDSRecThread = TRUE;

	//释放各种信号量
	sem_destroy(&pFlyRadioInfo->MainThread_sem);

	//释放各种条件变量
	pthread_cond_destroy(&pFlyRadioInfo->ScanThreadCond);
	pthread_cond_destroy(&pFlyRadioInfo->RDSRecThreadCond);

	allInOneDeinit();

	free (pFlyRadioInfo);
	pFlyRadioInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	  DBG0(debugString("\nFlyRadio SAF7741 close device !");)
	 pFlyRadioInfo->bKillRadioMainThread = TRUE;
	 pFlyRadioInfo->bKillRadioScanThread = TRUE;
	 pFlyRadioInfo->bKillRadioRDSRecThread = TRUE;

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
	DBG0(debugBuf("\nFlyRadio SAF7741 User write to Radio-HAL---->",buf,len);)
	
	DealRightDataProcessor(&buf[3], buf[2]-1);
}

#endif
