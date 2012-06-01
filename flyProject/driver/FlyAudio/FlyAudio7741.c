#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/poll.h>

#include "../../include/ShareMemoryStruct.h"

#if AUDIO_RADIO_CHIP_SEL == AUDIO_RADIO_7741_7000

#include "FlyAudio7741.h"
#include "SAF7741_data.h"
#include "../include/fly_soc.h"
#include "../include/driver_def.h"
#include "gps-sound.h"

#include "../include/fly_soc_iic.h"

struct audio_info *pGlobalAudioInfo = NULL;

static BOOL createFlyAudioThread(P_AUDIO_INFO pFlyAudioInfo);
static void SAF7741_Input(P_AUDIO_INFO pFlyAudioInfo,BYTE channel,BYTE InputGain);
static void setTheThreadStatus(P_AUDIO_INFO pFlyAudioInfo,BOOL bAlive);

static UINT32 forU8ToU32LSB(BYTE *p)
{
	UINT32 iTemp = 0;
	iTemp = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
	return iTemp;
}

static void forU32TopU8LSB(UINT32 data,BYTE *p)
{
	p[0] = data;
	data = data >> 8;p[1] = data;
	data = data >> 8;p[2] = data;
	data = data >> 8;p[3] = data;
}

static BOOL I2C_Write_SAF7741(P_AUDIO_INFO pFlyAudioInfo, UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	INT ret = -1;

	BYTE buff[300];

	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest)
	{
		return;
	}

	buff[2] = ulRegAddr & 0xff;
	buff[1] = (ulRegAddr>>8) & 0xff;
	buff[0] = (ulRegAddr>>16) & 0xff;
	memcpy(&buff[3],pRegValBuf,uiValBufLen);

	ret = SOC_I2C_Send(I2_1_ID,(SAF7741_ADDR_W>>1),buff,uiValBufLen+3);
	if (ret < 0) 
	{
		DBG0("IIC write error:%d!\n", ret);
		return FALSE;
	}

	//DBG0("\nFlyAduio SAF7741 IIC Write-->");
	//for (i = 0; i < uiValBufLen+3; i++)
	//{
	//	DBG0(" %2X",buff[i]);
	//}
		
	return TRUE;
}

static BOOL I2C_Read_SAF7741(P_AUDIO_INFO pFlyAudioInfo, UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	INT ret = -1;

	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest)
	{
		return;
	}

	//ret = I2C_Rec_SAF7741(I2_1_ID,SAF7741_ADDR_R>>1,ulRegAddr,pRegValBuf,uiValBufLen);
	ret = SOC_I2C_Rec_SAF7741(I2_1_ID,(SAF7741_ADDR_R>>1),ulRegAddr,pRegValBuf,uiValBufLen);
	if (ret < 0) 
	{
		DBG0("IIC write error:%d!\n", ret);
		return FALSE;
	}
	//DBG0("\nFlyAduio SAF7741 IIC Read-->");
	//for (i = 0; i < uiValBufLen+3; i++)
	//{
	//	DBG0(" %2X",pRegValBuf[i]);
	//}

	return TRUE;
}

static void SendToSAF7741UsingPortByLength(P_AUDIO_INFO pFlyAudioInfo, BYTE *p,BYTE level)
{
	UINT len;
	BYTE *p1,*p2,*p3;
	UINT regAddr;

	p2 = p;
	p3 = p2 + (1+1+3+3);
	len = p3[7];
	p1 = p3 + (1+1+3+3) + level*(5+len*2);

	regAddr = (p1[2] << 16) + (p1[3] << 8) + p1[4];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&p1[5],p1[1]-3);

	regAddr = (p2[2] << 16) + (p2[3] << 8) + p2[4];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&p2[5],p2[1]-3);

	regAddr = (p3[2] << 16) + (p3[3] << 8) + p3[4];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&p3[5],p3[1]-3);
}

static void SendToSAF7741NormalWriteData(P_AUDIO_INFO pFlyAudioInfo, BYTE *pData)
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
			I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&pData[3],iLength - 3);
			//msleep(1);
			pData += iLength;
		}
	}
}

static void control7386Mute(P_AUDIO_INFO pFlyAudioInfo,BOOL bMute)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverbAMPMute = bMute;
	ipcDriverStart(IPC_DRIVER_HARDWARE,IPC_DRIVER_EVENT_AMP_MUTE);
}

static void control7741Reset(P_AUDIO_INFO pFlyAudioInfo)
{
	DBG0("\n---------7741 reset--------------");
	ipcDriverStart(IPC_DRIVER_HARDWARE,IPC_DRIVER_EVENT_RESET_7741);

	msleep(10);
}

static void control7386AMPOn(P_AUDIO_INFO pFlyAudioInfo,BOOL bOn)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverAMPOn = bOn;
	ipcDriverStart(IPC_DRIVER_HARDWARE,IPC_DRIVER_EVENT_AMP_ONOFF);
}

static void flyAudioReturnToHal(P_AUDIO_INFO pFlyAudioInfo, BYTE *buf, UINT16 len)
{

	INT  i   = 0;
	BYTE crc = 0;

	crc = len +1;
	for (i=0; i<len; i++){
		crc += buf[i];
	}

	pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalHx][0] = 0xFF;
	pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalHx][1] = 0x55;
	pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalHx][2] = len+1;
	memcpy(&pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalHx][3], buf, len);
	pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalHx][3+len] = crc;

	pFlyAudioInfo->buffToHalHx++;
	if (pFlyAudioInfo->buffToHalHx >= USER_BUF_MAX)
		pFlyAudioInfo->buffToHalHx = 0;

	wake_up_interruptible(&pFlyAudioInfo->read_wait);
}

static void returnAudioPowerMode(P_AUDIO_INFO pFlyAudioInfo,BOOL bPower)
{

	BYTE buf[2] = {0x01,0x00};
	if (bPower)
		buf[1] = 0x01;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudiobInit(P_AUDIO_INFO pFlyAudioInfo,BOOL bInit)
{

	BYTE buf[2] = {0x02,0x00};
	if (bInit)
		buf[1] = 0x01;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainInput(P_AUDIO_INFO pFlyAudioInfo,BYTE Input)
{

	BYTE buf[2] = {0x10,0x00};
	buf[1] = Input;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainMute(P_AUDIO_INFO pFlyAudioInfo,BOOL bMute)
{

	BYTE buf[2] = {0x11,0x00};
	if (bMute)
		buf[1] = 0x01;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainBalance(P_AUDIO_INFO pFlyAudioInfo,BYTE Balance)
{

	BYTE buf[2] = {0x13,0x00};
	buf[1] = Balance;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainFader(P_AUDIO_INFO pFlyAudioInfo,BYTE Fader)
{

	BYTE buf[2] = {0x14,0x00};
	buf[1] = Fader;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainTrebFreq(P_AUDIO_INFO pFlyAudioInfo,BYTE TrebFreq)
{

	BYTE buf[2] = {0x15,0x00};
	buf[1] = TrebFreq;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainTrebLevel(P_AUDIO_INFO pFlyAudioInfo,BYTE TrebLevel)
{

	BYTE buf[2] = {0x16,0x00};
	buf[1] = TrebLevel;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainMidFreq(P_AUDIO_INFO pFlyAudioInfo,BYTE MidFreq)
{

	BYTE buf[2] = {0x17,0x00};
	buf[1] = MidFreq;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainMidLevel(P_AUDIO_INFO pFlyAudioInfo,BYTE MidLevel)
{

	BYTE buf[2] = {0x18,0x00};
	buf[1] = MidLevel;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainBassFreq(P_AUDIO_INFO pFlyAudioInfo,BYTE BassFreq)
{

	BYTE buf[2] = {0x19,0x00};
	buf[1] = BassFreq;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainBassLevel(P_AUDIO_INFO pFlyAudioInfo,BYTE BassLevel)
{

	BYTE buf[2] = {0x1A,0x00};
	buf[1] = BassLevel;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainLoudnessFreq(P_AUDIO_INFO pFlyAudioInfo,BYTE LoudnessFreq)
{

	BYTE buf[2] = {0x1B,0x00};
	buf[1] = LoudnessFreq;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainLoudnessLevel(P_AUDIO_INFO pFlyAudioInfo,BYTE LoudnessLevel)
{

	BYTE buf[2] = {0x1C,0x00};
	buf[1] = LoudnessLevel;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioEQALL(P_AUDIO_INFO pFlyAudioInfo,BYTE *EQ)
{

	BYTE buf[11] = {};
	buf[0] = 0x1D;
	memcpy(&buf[1],EQ,10);

	flyAudioReturnToHal(pFlyAudioInfo,buf,11);
}

static void returnAudioEQ(P_AUDIO_INFO pFlyAudioInfo,BYTE EQ)
{

	BYTE buf[11] = {0x1E,0x00};
	buf[1] = EQ;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainLoudnessOn(P_AUDIO_INFO pFlyAudioInfo,BOOL bLoudnessOn)
{

	BYTE buf[2] = {0x30,0x00};
	if (bLoudnessOn)
		buf[1] = 0x01;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainSimEQ(P_AUDIO_INFO pFlyAudioInfo,BYTE SimEQ)
{

	BYTE buf[2] = {0x21,0x00};
	buf[1] = SimEQ;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainSubOn(P_AUDIO_INFO pFlyAudioInfo,BOOL bSubOn)
{

	BYTE buf[2] = {0x31,0x00};
	if (bSubOn)
		buf[1] = 0x01;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainSubFilter(P_AUDIO_INFO pFlyAudioInfo,BYTE SubFilter)
{
	
	BYTE buf[2] = {0x32,0x00};
	buf[1] = SubFilter;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioMainSubLevel(P_AUDIO_INFO pFlyAudioInfo,BYTE SubLevel)
{

	BYTE buf[2] = {0x33,0x00};
	buf[1] = SubLevel;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudiobSoundOn(P_AUDIO_INFO pFlyAudioInfo,BOOL bOn)
{
	BYTE buf[2] = {0x08,0x00};
	if (bOn)
	{
		buf[1] = 0x01;
	}
	else
	{
		buf[1] = 0x00;
	}

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void returnAudioVolume(P_AUDIO_INFO pFlyAudioInfo,BYTE iVolume)
{
	BYTE buf[2] = {0x12,0x00};
	buf[1] = iVolume;

	flyAudioReturnToHal(pFlyAudioInfo,buf,2);
}

static void volumeFaderInOut(P_AUDIO_INFO pFlyAudioInfo,BOOL bEnable)
{
	if (bEnable)
		pFlyAudioInfo->sFlyAudioInfo.bEnableVolumeFader = TRUE;
	else
		pFlyAudioInfo->sFlyAudioInfo.bEnableVolumeFader = FALSE;
}

static BOOL bVolumeFaderInOut(P_AUDIO_INFO pFlyAudioInfo)
{

	return pFlyAudioInfo->sFlyAudioInfo.bEnableVolumeFader;
}

static void SAF7741_Mute_P(P_AUDIO_INFO pFlyAudioInfo,BOOL para)
{					   
	BYTE reg[5] = {0x0D, 0x10, 0x6D, 0x00, 0x00};//ADSP_Y_Mute_P

	UINT regAddr;

	if(para)
	{
		reg[3] = 0x00;reg[4] = 0x00;
		DBG0("FlyAudio SAF7741:Mute P---->ON");
	}
	else
	{
		reg[3] = 0x08;reg[4] = 0x00;
		DBG0("FlyAudio SAF7741:Mute P---->OFF");
	}

	regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],2);
}

static void SAF7741_Navi_Mix(P_AUDIO_INFO pFlyAudioInfo,BYTE para)
{
	//Navi input AD34
	BYTE reg[6] = {0x0D,0x00,0x6F,0x00,0x00,0x34};//XA_EasyP_Index
	//BYTE reg[6] = {0x0D,0x00,0x6F,0x00,0x00,0x36};//XA_EasyP_Index
	//静音导航	08 00 取消静音
	//BYTE regMUTE_Navi[5] = {0x0D, 0x10, 0x6F, 0x00, 0x00};//YA_Mute_N

	BYTE regOpenMixL[5] = {0x0D, 0x10, 0xC2, 0x0D, 0x78};//ADSP_Y_Sup_NonFL
	BYTE regOpenMixR[5] = {0x0D, 0x10, 0xC3, 0x0D, 0x78};//ADSP_Y_Sup_NonFR	

	BYTE regCloseMixL[5] = {0x0D, 0x10, 0xC2, 0x0F, 0xFF};//ADSP_Y_Sup_NonFL
	BYTE regCloseMixR[5] = {0x0D, 0x10, 0xC3, 0x0F, 0xFF};//ADSP_Y_Sup_NonFR	

	//BYTE regOpenMixL[5] = {0x0D, 0x10, 0xC7, 0x0D, 0x78};//ADSP_Y_Sup_TonFL
	//BYTE regOpenMixR[5] = {0x0D, 0x10, 0xC8, 0x0D, 0x78};//ADSP_Y_Sup_TonFR	
	//BYTE regCloseMixL[5] = {0x0D, 0x10, 0xC7, 0x0F, 0xFF};//ADSP_Y_Sup_TonFL
	//BYTE regCloseMixR[5] = {0x0D, 0x10, 0xC8, 0x0F, 0xFF};//ADSP_Y_Sup_TonFR

	//用GUI主面板的控
	BYTE regGain[5] = {0x0D, 0x10,0x4E, 0x00, 0x00};//ADSP_Y_Vol_Nav

	UINT regAddr;

	regGain[3] = SAF7741_InputGain_Data_Mix[para*2+0];
	regGain[4] = SAF7741_InputGain_Data_Mix[para*2+1];

	if(para)
	{
		regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
		I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],3);
		regAddr = (regOpenMixL[0] << 16) + (regOpenMixL[1] << 8) + regOpenMixL[2];
		I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&regOpenMixL[3],2);
		regAddr = (regOpenMixR[0] << 16) + (regOpenMixR[1] << 8) + regOpenMixR[2];
		I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&regOpenMixR[3],2);
		regAddr = (regGain[0] << 16) + (regGain[1] << 8) + regGain[2];
		I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&regGain[3],2);

		DBG0("FlyAudio SAF7741:Navi Mix---->ON");
	}
	else
	{
		regAddr = (regCloseMixL[0] << 16) + (regCloseMixL[1] << 8) + regCloseMixL[2];
		I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&regCloseMixL[3],2);
		regAddr = (regCloseMixR[0] << 16) + (regCloseMixR[1] << 8) + regCloseMixR[2];
		I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&regCloseMixR[3],2);

		DBG0("FlyAudio SAF7741:Navi Mix---->OFF");
	}
}

static void SAF7741_Mute(P_AUDIO_INFO pFlyAudioInfo,BOOL para)
{
	DBG0("SAF7741_Mute para:%d\n", para);
	if(para)
	{
		SAF7741_Mute_P(pFlyAudioInfo,TRUE);

		if (pFlyAudioInfo->sFlyAudioInfo.preMainMute)
		{
			DBG("preMainMute-->%d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);
			DBG("globlaMute-->%d",GlobalShareMmapInfo.pShareMemoryCommonData->bMute);
			control7386Mute(pFlyAudioInfo,TRUE);
		}
		else
			control7386Mute(pFlyAudioInfo,FALSE);
	}
	else
	{
		SAF7741_Mute_P(pFlyAudioInfo,FALSE);

		control7386Mute(pFlyAudioInfo,FALSE);
	}
}
static void SAF7741_Volume(P_AUDIO_INFO pFlyAudioInfo,BYTE Volume)
{
	BYTE reg1[5] = {0x0D, 0x10, 0x50, 0x00, 0x00};//ADSP_Y_Vol_Main1P
	BYTE reg2[5] = {0x0D, 0x10, 0x51, 0x00, 0x00};//ADSP_Y_Vol_Main1S
	UINT regAddr;

	if (Volume > 60) 
		Volume = 60;
		
	if(EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput)
	{
		Volume = Volume + 6;
		if (Volume > (sizeof(SAF7741_Volume_Data)/4)) Volume = (sizeof(SAF7741_Volume_Data)/4) - 1;
	}

	reg1[3] = SAF7741_Volume_Data[4*Volume+0];
	reg1[4] = SAF7741_Volume_Data[4*Volume+1];
	reg2[3] = SAF7741_Volume_Data[4*Volume+2];
	reg2[4] = SAF7741_Volume_Data[4*Volume+3];

	regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg1[3],2);
	regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg2[3],2);
}

static void SAF7741_Loud(P_AUDIO_INFO pFlyAudioInfo,BYTE LoudFreq, BYTE LoudLevel)
{
	//BYTE reg[5] = {0x0D,0x13,0x02,0x00,0x00};  //ADSP_Y_Loudf_MaxBstB
	//UINT regAddr;

	//DBG0("\nFlyAudio SAF7741:Set Loud Freq---->%d",LoudFreq);
	//DBG0("\nFlyAudio SAF7741:Set Loud Level---->%d",LoudLevel);

	//if (LoudFreq > LOUDNESS_FREQ_COUNT) LoudFreq = (LOUDNESS_FREQ_COUNT - 1);
	//if (LoudLevel > LOUDNESS_LEVEL_COUNT) LoudLevel = (LOUDNESS_LEVEL_COUNT - 1);

	//reg[3] = SAF7741_Loud_Data[LoudLevel*2+0]; 
	//reg[4] = SAF7741_Loud_Data[LoudLevel*2+1];
	//		
	//SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_Loud_Freq,LoudFreq);

	//regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
	//I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],2);
}

static void SAF7741_Sub(P_AUDIO_INFO pFlyAudioInfo,BYTE curSubFilter, BYTE SubLevel)
{
	//BYTE reg1[5] = {0x0D,0x00,0x6F,0x00,0xE4};  //Open the SubWoofer
	//UINT regAddr;
	//DBG0("\nFlyAudio SAF7741:Set Sub Filter-->%d,Level-->%d",curSubFilter,SubLevel);

	//if (curSubFilter > SUB_FILTER_COUNT) curSubFilter = (SUB_FILTER_COUNT - 1);
	//if (SubLevel > SUB_LEVEL_COUNT) SubLevel = (SUB_LEVEL_COUNT - 1);

	//regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
	//I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg1[3],2);

	//if (Sub_CutFREQ_80 == curSubFilter)
	//{
	//	SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_subwoofer_80,SubLevel);
	//}
	//else if (Sub_CutFREQ_120 == curSubFilter)
	//{
	//	SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_subwoofer_120,SubLevel);
	//}
	//else if (Sub_CutFREQ_160 == curSubFilter)
	//{
	//	SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_subwoofer_160,SubLevel);
	//}

}

static void SAF7741_DesScalBMT_P(P_AUDIO_INFO pFlyAudioInfo)
{
	BYTE reg[5] = {0x0D, 0x10, 0x77, 0x00, 0x00};//ADSP_Y_Vol_DesScalBMTP
	UINT regAddr;

	UINT Temp1,Temp2;
	Temp1 = LARGER(pFlyAudioInfo->sFlyAudioInfo.curBassLevel,pFlyAudioInfo->sFlyAudioInfo.curMidLevel);
	Temp2 = LARGER(Temp1,pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel);

	reg[3] = SAF7741_DesScalBMT_Data[Temp2*2+0];
	reg[4] = SAF7741_DesScalBMT_Data[Temp2*2+1];

	//DBG0("\nFlyAudio SAF7741:Updata DesScalBMT---->%d",Temp2);

	regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],2);
}

static void SAF7741_Bass(P_AUDIO_INFO pFlyAudioInfo,BYTE BassFreq, BYTE BassLevel)
{
	BYTE reg[5] = {0x0D, 0x14, 0x25, 0x00, 0x00};//ADSP_Y_BMT_GbasP
	UINT regAddr;

	if (BassFreq > (BASS_FREQ_COUNT - 1)) BassFreq = (BASS_FREQ_COUNT - 1);
	if (BassLevel > (BASS_LEVEL_COUNT - 1)) BassLevel = BASS_LEVEL_COUNT - 1;

	reg[3] = SAF7741_Bass_Data_P[BassLevel*2+0];  //60Hz,80Hz,100Hz数据一样
	reg[4] = SAF7741_Bass_Data_P[BassLevel*2+1];

	//DBG0("\nFlyAudio SAF7741:Set Bass Freq-->%d,Level-->%d",BassFreq,BassLevel);

	SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)BassFreqsel,BassFreq);

	regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],2);

	SAF7741_DesScalBMT_P(pFlyAudioInfo);
}

static void SAF7741_Mid(P_AUDIO_INFO pFlyAudioInfo,BYTE MidFreq, BYTE MidLevel)
{
	//DBG0("\nFlyAudio SAF7741:Set Mid Freq-->%d,Level-->%d",MidFreq,MidLevel);

	if (MidFreq > (MID_FREQ_COUNT - 1)) MidFreq = (MID_FREQ_COUNT - 1);
	if (MidLevel > (MID_LEVEL_COUNT - 1)) MidLevel = MID_LEVEL_COUNT - 1;

	if (MidFreq==M_FREQ_500)
	{		
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_Mid_500_Data_P,MidLevel);
	}
	else if (MidFreq==M_FREQ_1000)
	{
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_Mid_1000_Data_P,MidLevel);	
	}
	else if (MidFreq==M_FREQ_1500)
	{
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_Mid_1500_Data_P,MidLevel);
	}

	SAF7741_DesScalBMT_P(pFlyAudioInfo);
}

static void SAF7741_Treble(P_AUDIO_INFO pFlyAudioInfo,BYTE TrebFreq, BYTE TrebLevel)
{
	//DBG0("\nFlyAudio SAF7741:Set Treble Freq-->%d,Level-->%d",TrebFreq,TrebLevel);

	if (TrebFreq > (TREB_FREQ_COUNT - 1)) TrebFreq = TREB_FREQ_COUNT - 1;
	if (TrebLevel > (TREB_LEVEL_COUNT - 1)) TrebLevel = TREB_LEVEL_COUNT - 1;

	if (TrebFreq==T_FREQ_10K)
	{		
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_Treble_10k_Data_P,TrebLevel); 
	}
	else if (TrebFreq==T_FREQ_12K)
	{
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_Treble_12k_Data_P,TrebLevel); 	
	}

	SAF7741_DesScalBMT_P(pFlyAudioInfo);
}

static void SAF7741_Balance_P(P_AUDIO_INFO pFlyAudioInfo,BYTE ibalance)
{
	//DBG0("\nFlyAudio SAF7741:Set Balance---->%d",ibalance);

	BYTE reg1[5] = {0x0D, 0x10, 0x25, 0x00, 0x00};//ADSP_Y_Vol_BalPL
	BYTE reg2[5] = {0x0D, 0x10, 0x26, 0x00, 0x00};//ADSP_Y_Vol_BalPR
	UINT regAddr;

	if(ibalance > (BALANCE_LEVEL_COUNT - 1)) ibalance = BALANCE_LEVEL_COUNT - 1;

	reg1[3] = SAF7741_Balance_Fader_Data_P[4*ibalance+0];
	reg1[4] = SAF7741_Balance_Fader_Data_P[4*ibalance+1];
	reg2[3] = SAF7741_Balance_Fader_Data_P[4*ibalance+2];
	reg2[4] = SAF7741_Balance_Fader_Data_P[4*ibalance+3];

	regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg1[3],2);
	regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg2[3],2);
}

void SAF7741_Fader_P(P_AUDIO_INFO pFlyAudioInfo,BYTE ifader)
{ 
	//DBG0("\nFlyAudio SAF7741:Set Fader---->%d",ifader);

	BYTE reg1[5] = {0x0D, 0x10, 0x23, 0x00, 0x00};//YA_Vol_FadF
	BYTE reg2[5] = {0x0D, 0x10, 0x24, 0x00, 0x00};//YA_Vol_FadR
	UINT regAddr;

	if (ifader > (FADER_LEVEL_COUNT - 1)) ifader = FADER_LEVEL_COUNT - 1; 

	reg1[3] = SAF7741_Balance_Fader_Data_P[4*ifader+0];
	reg1[4] = SAF7741_Balance_Fader_Data_P[4*ifader+1];
	reg2[3] = SAF7741_Balance_Fader_Data_P[4*ifader+2];
	reg2[4] = SAF7741_Balance_Fader_Data_P[4*ifader+3];

	regAddr = (reg1[0] << 16) + (reg1[1] << 8) + reg1[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg1[3],2);
	regAddr = (reg2[0] << 16) + (reg2[1] << 8) + reg2[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg2[3],2);
}

static void SAF7741_DesScalGEq(P_AUDIO_INFO pFlyAudioInfo)
{
	BYTE reg[5] = {0x0D, 0x10, 0x7A, 0x00, 0x00};//YA_Vol_DesScalGEq
	UINT regAddr;

	UINT Temp1,Temp2;
	Temp1 = LARGER(pFlyAudioInfo->sFlyAudioInfo.curEQ[0], pFlyAudioInfo->sFlyAudioInfo.curEQ[1]);
	Temp2 = LARGER(Temp1, pFlyAudioInfo->sFlyAudioInfo.curEQ[2]);
	Temp1 = LARGER(Temp2, pFlyAudioInfo->sFlyAudioInfo.curEQ[3]);
	Temp2 = LARGER(Temp1, pFlyAudioInfo->sFlyAudioInfo.curEQ[4]);
	Temp1 = LARGER(Temp2, pFlyAudioInfo->sFlyAudioInfo.curEQ[5]);
	Temp2 = LARGER(Temp1, pFlyAudioInfo->sFlyAudioInfo.curEQ[6]);
	Temp1 = LARGER(Temp2, pFlyAudioInfo->sFlyAudioInfo.curEQ[7]);
	Temp2 = LARGER(Temp1, pFlyAudioInfo->sFlyAudioInfo.curEQ[8]);

	//DBG0("\nFlyAudio SAF7741:Updata DesScalGEq---->%d",Temp2);

	reg[3] = SAF7741_DesScalGEq_Data[Temp2*2+0];
	reg[4] = SAF7741_DesScalGEq_Data[Temp2*2+1];

	regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],2);
}

void SAF7741_EQ_P(P_AUDIO_INFO pFlyAudioInfo,UINT sel,UINT level)
{	
	//DBG0("\nFlyAudio SAF7741:Set EQ Band-->%d,level-->%d",sel,level);

	if (sel == 0)
	{
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_geq_32_FL,level);
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_geq_32_FR,level);
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_geq_32_RL,level);
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_geq_32_RR,level);
	}
	else
	{
		SendToSAF7741UsingPortByLength(pFlyAudioInfo,(BYTE *)SAF7741_geq_freq_sel[sel],level);
		SAF7741_DesScalGEq(pFlyAudioInfo);
	}
}

void SAF7741_Pos_P(P_AUDIO_INFO pFlyAudioInfo,UINT pos)
{	
	////DBG0("\nFlyAudio SAF7741:Set Pos---->%d",pos);

	//if(0 == pos)
	//{
	//	SendToSAF7741NormalWriteData(pFlyAudioInfo,delay_not);	
	//}
	//else if(1 == pos)
	//{
	//	SendToSAF7741NormalWriteData(pFlyAudioInfo,delay_pos1);	
	//}
	//else if(2 == pos)
	//{
	//	SendToSAF7741NormalWriteData(pFlyAudioInfo,delay_pos2);	
	//}
}

static void SAF7741_Input(P_AUDIO_INFO pFlyAudioInfo,BYTE channel,BYTE InputGain)
{
	BYTE reg[6] = {0x0D, 0x00, 0x6F, 0x00, 0x00, 0x00};//XA_EasyP_Index
	BYTE regGain[5] = {0x0D, 0x10, 0x2D, 0x00, 0x00};//ADSP_Y_Vol_SrcScalP
	UINT regAddr;

	DBG0("\nFlyAudio SAF7741:input---->");

	switch(channel)
	{
	case Init:
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x29;	//I2S 2  空出来没用的 
		DBG0("Init");
		break;
	case IPOD :                                     //AIN1
	case MediaMP3:
	case AUX :										
	case TV:
	case BT_RING:
	case CDC:
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x1D;
		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
		DBG0("IPOD/AUX / TV / CDC / MediaMP3");
		break;
	case A2DP:									    	
	case BT:  
#if PCB_8803_DISP_SEL == PCB_8803_DISP_V1
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x21;  //AIN2
		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
		
#else if PCB_8803_DISP_SEL == PCB_8803_DISP_V2
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x1D;
		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
#endif
		DBG0("A2DP / BT");
		break;
	case EXT_TEL:									//AIN4
	case GR_AUDIO:
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x31;
		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
		DBG0("EXT_TEL / GPS");
		break;
	case MediaCD://DVD	SPDIF1
//#if PCB_8803_AMP_SEL == PCB_8803_AMP_V1
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x2B;	//SPDIF 
		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
//#else if PCB_8803_AMP_SEL == PCB_8803_AMP_V2
//		//reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x1D;
//		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x2B;	//SPDIF 
//		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
//		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
//#endif
		DBG0("MediaCD");
		break;
	case RADIO:										//Radio1
		reg[3] = 0x00;reg[4] = 0x00;reg[5] = 0x13;
		regGain[3]=SAF7741_InputGain_Data_PS[InputGain*2+0];
		regGain[4]=SAF7741_InputGain_Data_PS[InputGain*2+1];
		DBG0("RADIO");
		break;
	default:
		DBG0("xxxxxxx");
		return;
		break;
	}

	GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverMainAudioInput = channel;
	ipcDriverStart(IPC_DRIVER_HARDWARE,IPC_DRIVER_EVENT_MAIN_AUDIO_INPUT);

	regAddr = (reg[0] << 16) + (reg[1] << 8) + reg[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&reg[3],3);
	regAddr = (regGain[0] << 16) + (regGain[1] << 8) + regGain[2];
	I2C_Write_SAF7741(pFlyAudioInfo,regAddr,&regGain[3],2);
	
	//schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
}

#define ADSP_X_Gsa_Bar321                          0x0D0AFB
#define ADSP_X_Gsa_Bar654                          0x0D0AFA
#define ADSP_X_Gsa_Bar987                          0x0D0AF9

void SAF7741_ReadGraphicalSpectrumAnalyzer(P_AUDIO_INFO pFlyAudioInfo)//寄存器地址，字节MSB，先传低字节,7741先接受高字节
{
	//BYTE i;
	//BYTE Value[3];//7741先发高字节，Prima先存低字节
	//BYTE commandValue[] = {0x01,0xF0,1,2,3,4,5,6,7,8,9};
	//return;
	//I2C_Read_SAF7741(pFlyAudioInfo, ADSP_X_Gsa_Bar321, Value, 3);
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[0] = Value[2];
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[1] = Value[1];
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[2] = Value[0];
	////Sleep(10);
	//I2C_Read_SAF7741(pFlyAudioInfo, ADSP_X_Gsa_Bar654, Value, 3);
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[3] = Value[2];
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[4] = Value[1];
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[5] = Value[0];
	////Sleep(10);
	//I2C_Read_SAF7741(pFlyAudioInfo, ADSP_X_Gsa_Bar987, Value, 3);
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[6] = Value[2];
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[7] = Value[1];
	//pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[8] = Value[0];
	////Sleep(10);

	//DBG0("\r\nFlyAudio SAF7741 Read GraphicalSpectrumAnalyzer-->");
	//	for(i = 0;i <9;i++)
	//	{
	//		DBG0("  %d",pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[i]);
	//	}

	//	memcpy(&commandValue[2],&pFlyAudioInfo->sFlyAudioInfo.GraphicalSpectrumAnalyzerValue[0],9);
	//	flyAudioReturnToHal(pFlyAudioInfo,commandValue,11);
}

static int FlyGPSAudioThread(void *arg)
{
	P_AUDIO_INFO pFlyAudioInfo = (struct audio_info*)arg;
	BOOL bHaveGPSAudio;

	printk("\nFlyGPSAudioThread Start");

	while (!pFlyAudioInfo->bKillDispatchFlyGPSAudioThread)
	{
		bHaveGPSAudio = GPS_sound_status();
		printk("\nFlyGPSAudioThread Get %d",bHaveGPSAudio);
		if (1 == bHaveGPSAudio)
		{
			returnAudiobSoundOn(pFlyAudioInfo,TRUE);
			//pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker = 1;
		}
		else if (2 == bHaveGPSAudio)
		{
			returnAudiobSoundOn(pFlyAudioInfo,FALSE);
			//pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker = 0;
		}
		//GlobalShareMmapInfo.pShareMemoryCommonData->GPSSpeaker = pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker;
		//schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
	}

	do_exit(0);
	return 0;
}

static void ipcDriverAudio(BYTE enumWhat)
{
	P_AUDIO_INFO pFlyAudioInfo = (struct audio_info*)pGlobalAudioInfo;

	schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
}

static int FlyAudioDelayThread(void *arg)
{
	BOOL bInnerDelayOn = FALSE;
	ULONG iInnerMuteTime = 0;
	ULONG iInnerMuteMax = 0;
	ULONG iInnerDemuteTime = 0;
	ULONG iInnerDemuteMax = 0;

	BOOL bInnerNeedDemute = FALSE;

	BOOL bNeedReturnBTMute = FALSE;

	P_AUDIO_INFO pFlyAudioInfo = (struct audio_info*)arg;
	pFlyAudioInfo->bAudioDelayThreadRunning = TRUE;
	
	DBG0("FlyAudio delay thread start\n");
	while (!pFlyAudioInfo->bKillDispatchFlyAudioDelayThread){
		
		DBG0("\ncv %d pv%d vi%d pi%d cm%d tm%d %d %d %d %d %d"
			,pFlyAudioInfo->sFlyAudioInfo.curMainVolume
			,pFlyAudioInfo->sFlyAudioInfo.preMainVolume
			,pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput
			,pFlyAudioInfo->sFlyAudioInfo.preMainAudioInput
			,pFlyAudioInfo->sFlyAudioInfo.curMainMute
			,pFlyAudioInfo->sFlyAudioInfo.tmpMainMute
			,bInnerDelayOn
			,iInnerMuteTime
			,iInnerMuteMax
			,iInnerDemuteTime
			,iInnerDemuteMax);

		while (TRUE == pFlyAudioInfo->bNeedInit)
		{
			bInnerDelayOn = TRUE;
			msleep(314);
		}

		if (bInnerDelayOn)//处理任务中
		{
			wait_for_completion_timeout(&pFlyAudioInfo->comFlyAudioDelayThread, 10);
		}
		else
		{
			wait_for_completion(&pFlyAudioInfo->comFlyAudioDelayThread);
		}
				
		if (!pFlyAudioInfo->bPowerUp)//等待初始化
		{
			bInnerDelayOn = FALSE;
			continue;
		}

		if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID))//赋值
			pFlyAudioInfo->sFlyAudioInfo.bMuteRadio = TRUE;

		if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID))//赋值
			pFlyAudioInfo->sFlyAudioInfo.bMuteRadio = FALSE;

		if (ipcWhatEventOn(EVENT_GLOBAL_BT_MUTE_REQ_ID))//赋值
		{
			ipcClearEvent(EVENT_GLOBAL_BT_MUTE_REQ_ID);
			pFlyAudioInfo->sFlyAudioInfo.bMuteBT = GlobalShareMmapInfo.pShareMemoryCommonData->ipcbMuteBT;
			if (pFlyAudioInfo->sFlyAudioInfo.bMuteBT)
			{
				bNeedReturnBTMute = TRUE;
			}
			else
			{
				msleep(100);
			}
		}

		if (pFlyAudioInfo->sFlyAudioInfo.bMuteRadio && pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput == RADIO)//收音机静音
		{
			pFlyAudioInfo->sFlyAudioInfo.tmpMainMute = TRUE;
		}
		else if (pFlyAudioInfo->sFlyAudioInfo.bMuteBT && pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput == BT)//蓝牙静音
		{
			pFlyAudioInfo->sFlyAudioInfo.tmpMainMute = TRUE;
		}
		else if (pFlyAudioInfo->sFlyAudioInfo.bMuteBT && pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput == A2DP)//A2DP静音
		{
			pFlyAudioInfo->sFlyAudioInfo.tmpMainMute = TRUE;
		}
		else
		{
			pFlyAudioInfo->sFlyAudioInfo.tmpMainMute = pFlyAudioInfo->sFlyAudioInfo.preMainMute;
		}


		pFlyAudioInfo->sFlyAudioInfo.dspMainMute = pFlyAudioInfo->sFlyAudioInfo.tmpMainMute;
		if(pFlyAudioInfo->sFlyAudioInfo.curMainMute != pFlyAudioInfo->sFlyAudioInfo.tmpMainMute)//直接静音控制
		{
			pFlyAudioInfo->sFlyAudioInfo.curMainMute = pFlyAudioInfo->sFlyAudioInfo.tmpMainMute;
			if (pFlyAudioInfo->sFlyAudioInfo.curMainMute)
			{
				bInnerDelayOn = TRUE;
				if (0 == iInnerMuteTime)
				{
					pFlyAudioInfo->sFlyAudioInfo.curMainMute = TRUE;
					SAF7741_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);

					pFlyAudioInfo->sFlyAudioInfo.curMainVolume = 0;
					SAF7741_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);

					//设定静音时间
					iInnerMuteTime = GetTickCount();
					iInnerMuteMax = 100;
				}
			}
			else
			{
				bInnerNeedDemute = TRUE;
			}
		}

		if(pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput){//切换通道
		
			if ((pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput == MediaMP3 && pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput == IPOD)
				|| (pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput == IPOD && pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput == MediaMP3))
			{
				pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput;
				pFlyAudioInfo->sFlyAudioInfo.dspMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput;
			}
			else//启动
			{
				bInnerDelayOn = TRUE;
				if (0 == iInnerMuteTime)
				{
					volumeFaderInOut(pFlyAudioInfo,TRUE);

					pFlyAudioInfo->sFlyAudioInfo.curMainMute = TRUE;
					SAF7741_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);

					pFlyAudioInfo->sFlyAudioInfo.curMainVolume = 0;
					SAF7741_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);

					//设定静音时间
					iInnerMuteTime = GetTickCount();
					iInnerMuteMax = 100;
				}
			}
		}
		
		if (iInnerMuteTime)//静音延时中
		{
			if (GetTickCount() - iInnerMuteTime < iInnerMuteMax)
			{
				continue;
			}
			else//满足延时时间
			{
				iInnerMuteTime = 0;
				iInnerMuteMax = 0;

				if (pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput)
				{
					DBG("\nSAF7741 DelayThread ChangeInput", pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput);

					//独立音量控制
					if (EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput)//保存当前
						GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iExtTelVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
					else if (BT == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput || BT_RING == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput)
						GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iBTCallVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
					else{
						GlobalShareMmapInfo.pShareMemoryCommonData->iNormalVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
						GlobalShareMmapInfo.pShareMemoryCommonData->bNormalMute = GlobalShareMmapInfo.pShareMemoryCommonData->bMute;
					}

					if (EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput){//恢复以前
						GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iExtTelVolume;
						GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
					}
					else if (BT == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput|| BT_RING == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput){
						GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iBTCallVolume;
						GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
					}
					else{
						GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iNormalVolume;
						GlobalShareMmapInfo.pShareMemoryCommonData->bMute = GlobalShareMmapInfo.pShareMemoryCommonData->bNormalMute;
					}

					if (EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput || BT == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput
						|| BT_RING == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput || EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput
						|| BT == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput || BT_RING == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput){
							ipcStartEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
					}

					//独立音量控制
					pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput;
					pFlyAudioInfo->sFlyAudioInfo.dspMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput;

					GlobalShareMmapInfo.pShareMemoryCommonData->eCurAudioInput = pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput;
					SAF7741_Input(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput,audioChannelGainTab[pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput]);

					/*****************************************************************************************/
					//MasterSlaveAudioMainChannel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput);
					/*****************************************************************************************/

					//设定恢复时间
					if (0 == iInnerDemuteTime)
					{
						iInnerDemuteTime = GetTickCount();
						iInnerDemuteMax = 40;
					}

					//if(pFlyAudioInfo->sFlyAudioInfo.curPos != pFlyAudioInfo->sFlyAudioInfo.prePos)
					//{
					//	DBG0("\r\nSAF7741 DelayThread ChangePos");

					//	pFlyAudioInfo->sFlyAudioInfo.curPos = pFlyAudioInfo->sFlyAudioInfo.prePos;
					//	SAF7741_Pos_P(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curPos);
					//	//设定恢复时间
					//	if (iDelayTime < 314)
					//	{
					//		iDelayTime = 314;
					//	}
					//}
				}
			}
		}
		
		if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID))
			ipcClearEvent(EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID);

		if (ipcWhatEventOn(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID)){//返回
			ipcClearEvent(EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID);
			ipcStartEvent(EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID);
		}
			
		if (bNeedReturnBTMute)//返回
		{
			bNeedReturnBTMute = FALSE;
			ipcStartEvent(EVENT_GLOBAL_BT_MUTE_IN_OK_ID);
		}
		
		if (iInnerDemuteTime)
		{
			if (GetTickCount() - iInnerDemuteTime < iInnerDemuteMax)
			{
				continue;
			}
			else
			{
				iInnerDemuteTime = 0;
				iInnerDemuteMax = 0;
			}
		}

		if (bInnerNeedDemute)
		{
			bInnerNeedDemute = FALSE;
			SAF7741_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);
		}

		if(FALSE == pFlyAudioInfo->sFlyAudioInfo.curMainMute 
			&& pFlyAudioInfo->sFlyAudioInfo.curMainVolume != pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
		{
			bInnerDelayOn = TRUE;

			//if (bVolumeFaderInOut(pFlyAudioInfo)){
			if (1)
			{
				if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume < pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
				{
					if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume < (pFlyAudioInfo->sFlyAudioInfo.preMainVolume - 5))
					{
						pFlyAudioInfo->sFlyAudioInfo.curMainVolume = pFlyAudioInfo->sFlyAudioInfo.curMainVolume + 5;
					}
					else
					{
						pFlyAudioInfo->sFlyAudioInfo.curMainVolume++;
					}
				}
				else if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume > pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
				{
					if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume > (pFlyAudioInfo->sFlyAudioInfo.preMainVolume + 5))
					{
						pFlyAudioInfo->sFlyAudioInfo.curMainVolume = pFlyAudioInfo->sFlyAudioInfo.curMainVolume - 5;
					}
					else
					{
						pFlyAudioInfo->sFlyAudioInfo.curMainVolume--;
					}		
				}
			}
			else
				pFlyAudioInfo->sFlyAudioInfo.curMainVolume = pFlyAudioInfo->sFlyAudioInfo.preMainVolume;


			if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume == pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
			{
				bInnerDelayOn = FALSE;
				volumeFaderInOut(pFlyAudioInfo,FALSE);
			}

			DBG("\nSAF7741 DelayThread Change Volume:",pFlyAudioInfo->sFlyAudioInfo.curMainVolume);

			SAF7741_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);
		}
		else
		{
			bInnerDelayOn = FALSE;
		}

		//if (bInnerNeedDemute)
		//{
		//	bInnerNeedDemute = FALSE;
		//	SAF7741_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);
		//}

		if (FALSE == bInnerDelayOn)
		{
			if ((MediaMP3 == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput|| IPOD == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput))
			{
				pFlyAudioInfo->sFlyAudioInfo.tmpGPSSpeaker = 0;
			}
			else if (
				BT_RING == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput
				|| BT == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput
				|| EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput)//NaviMix处理逻辑
			{
				pFlyAudioInfo->sFlyAudioInfo.tmpGPSSpeaker = 0;
			}
			else if (pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker)
			{
				pFlyAudioInfo->sFlyAudioInfo.tmpGPSSpeaker = pFlyAudioInfo->sFlyAudioInfo.preMainVolume;
			}
			else
			{
				pFlyAudioInfo->sFlyAudioInfo.tmpGPSSpeaker = 0;
			}
			if (pFlyAudioInfo->sFlyAudioInfo.curGPSSpeaker != pFlyAudioInfo->sFlyAudioInfo.tmpGPSSpeaker)
			{
				pFlyAudioInfo->sFlyAudioInfo.curGPSSpeaker = pFlyAudioInfo->sFlyAudioInfo.tmpGPSSpeaker;
				SAF7741_Navi_Mix(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curGPSSpeaker);
			}

			pFlyAudioInfo->sFlyAudioInfo.dspGPSSpeaker = pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker;
		}
	}
	
	DBG0("\nSAF7741 FlyAudioDelayThread exit");
	pFlyAudioInfo->bAudioDelayThreadRunning = FALSE;
	do_exit(0);
	return 0;
}

#define SIG_IPC_MSG 34
#define HAL_SERVICE_NAME "dio.osd.service"
struct task_struct *pUserProcess = NULL;

static void xEventSendVolume(BYTE iVolume)
{
	siginfo_t ipc_info;

	ipc_info.si_signo = SIG_IPC_MSG;
	ipc_info.si_code  = -1;
	ipc_info.si_int   = iVolume;

	if (1)
	//if (NULL == pUserProcess)
	{
		for_each_process(pUserProcess)
		{
			if (!strcmp(pUserProcess->comm, HAL_SERVICE_NAME))
			{
				//printk("\nsend Volume %d", iVolume);
				send_sig_info(SIG_IPC_MSG, &ipc_info, pUserProcess);
				break;
			}
		}
	}
	else
	{
		//printk("\nsend Volume %d", iVolume);
		send_sig_info(SIG_IPC_MSG, &ipc_info, pUserProcess);
	}
}


void FlyAudioMainThread(struct work_struct *work)
{
	BYTE j;
	P_AUDIO_INFO pFlyAudioInfo = pGlobalAudioInfo;

	
	if (!pFlyAudioInfo->bPowerUp)
	{
	}
	else{
		if (pFlyAudioInfo->bNeedInit){

			control7741Reset(pFlyAudioInfo);
			msleep(100);

			GlobalShareMmapInfo.pShareMemoryCommonData->b7741AudioInitFinish = FALSE;
			SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Audio_Init_Data_FM);
			SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Radio_Init_Data_20120120);
			//SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Radio_Init_Data_2011);

			GlobalShareMmapInfo.pShareMemoryCommonData->b7741AudioInitFinish = TRUE;
			DBG0("\nSAF7741 Audio Init OK Offset");

			pFlyAudioInfo->bNeedInit = FALSE;
			pFlyAudioInfo->bPowerUp = TRUE;
			GlobalShareMmapInfo.pShareMemoryCommonData->bAudioMuteControlable = TRUE;//音频芯片替其它驱动控制静音开关

			returnAudiobInit(pFlyAudioInfo, TRUE);

			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			
			if (!GlobalShareMmapInfo.pShareMemoryCommonData->bSilencePowerUp)
			{
				control7386AMPOn(pFlyAudioInfo,TRUE);
			}
			msleep(618);
		
			GlobalShareMmapInfo.pShareMemoryCommonData->iNeedProcVoltageShakeAudio = 80;//低电压

			//DBG0("\nSAF7741 Init OK");
		}

		if (pFlyAudioInfo->bcurDriverStatus != pFlyAudioInfo->bpreDriverStatus)
		{
			pFlyAudioInfo->bcurDriverStatus = pFlyAudioInfo->bpreDriverStatus;
			setTheThreadStatus(pFlyAudioInfo,pFlyAudioInfo->bcurDriverStatus);
		}
		
		if (ipcWhatEventOn(EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID)){
			ipcClearEvent(EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID);
			
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio){
			
			}else{
				SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Audio_Init_Data_FM);
				SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Radio_Init_Data_20120120);
				SAF7741_Input(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput,audioChannelGainTab[pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput]);
				SAF7741_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);
			}
		}
		
		if (ipcWhatEventOn(EVENT_GLOBAL_DATA_CHANGE_VOLUME)){
			ipcClearEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
			
			if (pFlyAudioInfo->sFlyAudioInfo.preMainMute != GlobalShareMmapInfo.pShareMemoryCommonData->bMute){
				pFlyAudioInfo->sFlyAudioInfo.preMainMute = GlobalShareMmapInfo.pShareMemoryCommonData->bMute;
				DBG("\nSAF7741 Global Mute:%d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);
			}
			
			if (pFlyAudioInfo->sFlyAudioInfo.preMainVolume != GlobalShareMmapInfo.pShareMemoryCommonData->iVolume){
				pFlyAudioInfo->sFlyAudioInfo.preMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
				pFlyAudioInfo->sFlyAudioInfo.dspMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
				DBG("\nSAF7741 Global Volume:%d",pFlyAudioInfo->sFlyAudioInfo.preMainVolume);
			}
			GlobalShareMmapInfo.pShareMemoryCommonData->bNeedReturnNewVolume = TRUE;
			returnAudioVolume(pFlyAudioInfo,GlobalShareMmapInfo.pShareMemoryCommonData->iVolume);
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bMute)
			{
				xEventSendVolume(0x80 | GlobalShareMmapInfo.pShareMemoryCommonData->iVolume);
			}
			else
			{
				xEventSendVolume(GlobalShareMmapInfo.pShareMemoryCommonData->iVolume);
			}
		}
		
		if (TRUE == pFlyAudioInfo->xxxxxxxxxxxxxxxxxxx)
		{
			DBG0("FlyAudio7741 event bStandby-->%d",GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus);
			pFlyAudioInfo->xxxxxxxxxxxxxxxxxxx = FALSE;
			if (TRUE == GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)
			{
				control7386Mute(pFlyAudioInfo,TRUE);
				msleep(100);
				control7386AMPOn(pFlyAudioInfo,FALSE);
				msleep(100);
			}
			else
			{
				control7386AMPOn(pFlyAudioInfo,TRUE);
				msleep(100);
				control7386Mute(pFlyAudioInfo,FALSE);
				msleep(100);
			}
		}
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio)
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
		else
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = GlobalShareMmapInfo.pShareMemoryCommonData->bMute;
		
		//倒车降低音量
		if (ipcWhatEventOn(EVENT_GLOBAL_BACK_LOW_VOLUME_ID))
		{
			ipcClearEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);
			volumeFaderInOut(pFlyAudioInfo,TRUE);
		}	
		
		if (GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.bHaveFlyAudioExtAMP)
		{
			pFlyAudioInfo->sFlyAudioInfo.preMainVolume = 55;
		}
		else{
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackDetectEnable
				&& pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != EXT_TEL
				&& pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != BT 
				&& pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != BT_RING)
			{
				if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackActiveNow)
				{
					if (GlobalShareMmapInfo.pShareMemoryCommonData->iVolume > 20)
					{
						pFlyAudioInfo->sFlyAudioInfo.preMainVolume = 20;
					}
					else
						pFlyAudioInfo->sFlyAudioInfo.preMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;			
				}
				else
					pFlyAudioInfo->sFlyAudioInfo.preMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
			}
			else
				pFlyAudioInfo->sFlyAudioInfo.preMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
		}
		
		if (ipcWhatEventOn(EVENT_GLOBAL_BTCALLSTATUS_CHANGE_ID))
		{
			//if (GlobalShareMmapInfo.pShareMemoryCommonData->)
			//DBG0("FlyAudio dd=+++++++++++++++++++++++++\n");
			ipcClearEvent(EVENT_GLOBAL_BTCALLSTATUS_CHANGE_ID);
		}

			
		if (ipcWhatEventOn(EVENT_GLOBAL_PHONECALLSTATUS_CHANGE_ID))
			ipcClearEvent(EVENT_GLOBAL_PHONECALLSTATUS_CHANGE_ID);

		if (GlobalShareMmapInfo.pShareMemoryCommonData->iExtTelCallStatus)//外部电话
			pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput = EXT_TEL;
		else{
			if (0x03 == GlobalShareMmapInfo.pShareMemoryCommonData->iBTCallStatus)//通话中
				pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput = BT;
			else if (0x02 == GlobalShareMmapInfo.pShareMemoryCommonData->iBTCallStatus)//去电中
				pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput = BT;
			else if (0x01 == GlobalShareMmapInfo.pShareMemoryCommonData->iBTCallStatus)//来电中
				pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput = BT_RING;
			else//平时正常
				pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.preMainAudioInput;
		}
		
		//DBG0("pppppppppppppp:%d\n",GlobalShareMmapInfo.pShareMemoryCommonData->iBTCallStatus);
		
		if (GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.bHaveFlyAudioExtAMP){
			pFlyAudioInfo->sFlyAudioInfo.tmpBassFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpBassLevel = BASS_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel = MID_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel = TREB_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn = FALSE;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel = LOUDNESS_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpBalance = BALANCE_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpFader = FADER_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubOn = FALSE;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel = 0;
		}
		else if (pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput == EXT_TEL
			|| pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput == BT 
			|| pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput == BT_RING){
			pFlyAudioInfo->sFlyAudioInfo.tmpBassFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpBassLevel = BASS_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel = MID_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel = TREB_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn = FALSE;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel = LOUDNESS_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpBalance = BALANCE_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpFader = FADER_LEVEL_COUNT/2;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubOn = FALSE;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter = 0;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel = 0;		
		}
		else{
			pFlyAudioInfo->sFlyAudioInfo.tmpBassFreq = pFlyAudioInfo->sFlyAudioInfo.preBassFreq;
			pFlyAudioInfo->sFlyAudioInfo.tmpBassLevel = pFlyAudioInfo->sFlyAudioInfo.preBassLevel;
			pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq = pFlyAudioInfo->sFlyAudioInfo.preMidFreq;
			pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel = pFlyAudioInfo->sFlyAudioInfo.preMidLevel;
			pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq;
			pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn = pFlyAudioInfo->sFlyAudioInfo.preLoudnessOn;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq = pFlyAudioInfo->sFlyAudioInfo.preLoudFreq;
			pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel = pFlyAudioInfo->sFlyAudioInfo.preLoudLevel;
			pFlyAudioInfo->sFlyAudioInfo.tmpBalance = pFlyAudioInfo->sFlyAudioInfo.preBalance;
			pFlyAudioInfo->sFlyAudioInfo.tmpFader = pFlyAudioInfo->sFlyAudioInfo.preFader;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubOn = pFlyAudioInfo->sFlyAudioInfo.preSubOn;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter = pFlyAudioInfo->sFlyAudioInfo.preSubFilter;
			pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel = pFlyAudioInfo->sFlyAudioInfo.preSubLevel;
		}
		
		pFlyAudioInfo->sFlyAudioInfo.dspBassFreq = pFlyAudioInfo->sFlyAudioInfo.preBassFreq;
		pFlyAudioInfo->sFlyAudioInfo.dspBassLevel = pFlyAudioInfo->sFlyAudioInfo.preBassLevel;
		pFlyAudioInfo->sFlyAudioInfo.dspMidFreq = pFlyAudioInfo->sFlyAudioInfo.preMidFreq;
		pFlyAudioInfo->sFlyAudioInfo.dspMidLevel = pFlyAudioInfo->sFlyAudioInfo.preMidLevel;
		pFlyAudioInfo->sFlyAudioInfo.dspTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq;
		pFlyAudioInfo->sFlyAudioInfo.dspTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel;
		pFlyAudioInfo->sFlyAudioInfo.dspLoudnessOn = pFlyAudioInfo->sFlyAudioInfo.preLoudnessOn;
		pFlyAudioInfo->sFlyAudioInfo.dspLoudFreq = pFlyAudioInfo->sFlyAudioInfo.preLoudFreq;
		pFlyAudioInfo->sFlyAudioInfo.dspLoudLevel = pFlyAudioInfo->sFlyAudioInfo.preLoudLevel;
		pFlyAudioInfo->sFlyAudioInfo.dspBalance = pFlyAudioInfo->sFlyAudioInfo.preBalance;
		pFlyAudioInfo->sFlyAudioInfo.dspFader = pFlyAudioInfo->sFlyAudioInfo.preFader;
		pFlyAudioInfo->sFlyAudioInfo.dspSubOn = pFlyAudioInfo->sFlyAudioInfo.preSubOn;
		pFlyAudioInfo->sFlyAudioInfo.dspSubFilter = pFlyAudioInfo->sFlyAudioInfo.preSubFilter;
		pFlyAudioInfo->sFlyAudioInfo.dspSubLevel = pFlyAudioInfo->sFlyAudioInfo.preSubLevel;
		
		/*******************************************************/
		//SetEvent(pFlyAudioInfo->hDispatchExtAmpThreadEvent);//这个任务在延时任务
		/*******************************************************/
		
		complete(&pFlyAudioInfo->comFlyAudioDelayThread);

		if (pFlyAudioInfo->sFlyAudioInfo.curSimEQ != pFlyAudioInfo->sFlyAudioInfo.preSimEQ){
			pFlyAudioInfo->sFlyAudioInfo.curSimEQ = pFlyAudioInfo->sFlyAudioInfo.preSimEQ;
			returnAudioMainSimEQ(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curSimEQ);
		}

		if(pFlyAudioInfo->sFlyAudioInfo.curBassFreq 
			!= pFlyAudioInfo->sFlyAudioInfo.tmpBassFreq 
			|| pFlyAudioInfo->sFlyAudioInfo.curBassLevel 
			!= pFlyAudioInfo->sFlyAudioInfo.tmpBassLevel){				
			pFlyAudioInfo->sFlyAudioInfo.curBassFreq = pFlyAudioInfo->sFlyAudioInfo.tmpBassFreq;
			pFlyAudioInfo->sFlyAudioInfo.curBassLevel = pFlyAudioInfo->sFlyAudioInfo.tmpBassLevel;
			SAF7741_Bass(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curBassFreq,pFlyAudioInfo->sFlyAudioInfo.curBassLevel);
		}
		
		if(pFlyAudioInfo->sFlyAudioInfo.curMidFreq != pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq || pFlyAudioInfo->sFlyAudioInfo.curMidLevel != pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel){
			pFlyAudioInfo->sFlyAudioInfo.curMidFreq = pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq;
			pFlyAudioInfo->sFlyAudioInfo.curMidLevel = pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel;
			SAF7741_Mid(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMidFreq,pFlyAudioInfo->sFlyAudioInfo.curMidLevel);
		}
		
		if(pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq != pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq || pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel != pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel)	{
			pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq;
			pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel;
			SAF7741_Treble(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq,pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel);
		}
		
		if (pFlyAudioInfo->sFlyAudioInfo.curLoudnessOn != pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn
			|| pFlyAudioInfo->sFlyAudioInfo.curLoudFreq != pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq 
			|| pFlyAudioInfo->sFlyAudioInfo.curLoudLevel != pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel){
			pFlyAudioInfo->sFlyAudioInfo.curLoudnessOn = pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn;
			pFlyAudioInfo->sFlyAudioInfo.curLoudFreq = pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq;
			pFlyAudioInfo->sFlyAudioInfo.curLoudLevel = pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel;
			if (pFlyAudioInfo->sFlyAudioInfo.curLoudnessOn)
				SAF7741_Loud(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curLoudFreq, pFlyAudioInfo->sFlyAudioInfo.curLoudLevel);
			else
				SAF7741_Loud(pFlyAudioInfo,0, LOUDNESS_LEVEL_COUNT/2);
		}
		
		if((pFlyAudioInfo->sFlyAudioInfo.curBalance != pFlyAudioInfo->sFlyAudioInfo.tmpBalance) || (pFlyAudioInfo->sFlyAudioInfo.curFader != pFlyAudioInfo->sFlyAudioInfo.tmpFader)){
			pFlyAudioInfo->sFlyAudioInfo.curBalance = pFlyAudioInfo->sFlyAudioInfo.tmpBalance;
			pFlyAudioInfo->sFlyAudioInfo.curFader = pFlyAudioInfo->sFlyAudioInfo.tmpFader;
			SAF7741_Balance_P(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curBalance);
			SAF7741_Fader_P(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curFader);
		}
		
		if (pFlyAudioInfo->sFlyAudioInfo.curSubOn != pFlyAudioInfo->sFlyAudioInfo.tmpSubOn
			|| pFlyAudioInfo->sFlyAudioInfo.curSubFilter != pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter
			|| pFlyAudioInfo->sFlyAudioInfo.curSubLevel != pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel){
			pFlyAudioInfo->sFlyAudioInfo.curSubOn = pFlyAudioInfo->sFlyAudioInfo.tmpSubOn;
			pFlyAudioInfo->sFlyAudioInfo.curSubFilter = pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter;
			pFlyAudioInfo->sFlyAudioInfo.curSubLevel = pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel;
			if (pFlyAudioInfo->sFlyAudioInfo.curSubOn)
				SAF7741_Sub(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curSubFilter,pFlyAudioInfo->sFlyAudioInfo.curSubLevel);
			else
				SAF7741_Sub(pFlyAudioInfo,0,0);
		}

		for(j=0;j<10;j++)
		{
			if(pFlyAudioInfo->sFlyAudioInfo.curEQ[j] != pFlyAudioInfo->sFlyAudioInfo.preEQ[j])
			{
				pFlyAudioInfo->sFlyAudioInfo.curEQ[j] = pFlyAudioInfo->sFlyAudioInfo.preEQ[j];
				SAF7741_EQ_P(pFlyAudioInfo,j,pFlyAudioInfo->sFlyAudioInfo.curEQ[j]);
			}
		} 

		SAF7741_ReadGraphicalSpectrumAnalyzer(pFlyAudioInfo);
	}
}

static void setTheThreadStatus(P_AUDIO_INFO pFlyAudioInfo,BOOL bAlive)
{
	if (bAlive)
	{
		control7741Reset(pFlyAudioInfo);
		msleep(200);

		SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Audio_Init_Data_FM);
		SendToSAF7741NormalWriteData(pFlyAudioInfo, SAF7741_Radio_Init_Data_20120120);

		GlobalShareMmapInfo.pShareMemoryCommonData->b7741AudioInitFinish = TRUE;

		while(!GlobalShareMmapInfo.pShareMemoryCommonData->b7741RadioInitFinish)
		{
			msleep(10);
		}

		SAF7741_Input(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput,audioChannelGainTab[pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput]);
		SAF7741_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);
		SAF7741_Balance_P(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curBalance);
		SAF7741_Fader_P(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curFader);
		SAF7741_Bass(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curBassFreq,pFlyAudioInfo->sFlyAudioInfo.curBassLevel);
		SAF7741_Mid(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMidFreq,pFlyAudioInfo->sFlyAudioInfo.curMidLevel);
		SAF7741_Treble(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq,pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel);

		msleep(10);
		
		//让功放工作
		control7386Mute(pFlyAudioInfo,FALSE);
		msleep(100);
		control7386AMPOn(pFlyAudioInfo,TRUE);

		schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
		DBG0("\nFlyAudio ready wakeUp\n");
	}
	else
	{
		GlobalShareMmapInfo.pShareMemoryCommonData->b7741AudioInitFinish = FALSE;

		control7386Mute(pFlyAudioInfo,TRUE);
		msleep(100);
		control7386AMPOn(pFlyAudioInfo,FALSE);

		DBG0("\nFlyAudio ready to sleep\n");
	}
}

static void FlyAudioParaInitDefault(P_AUDIO_INFO pFlyAudioInfo)
{

	DBG("\nFlyAudio Para Init Default");
	
	memset(&pFlyAudioInfo->sFlyAudioInfo,0xFF,sizeof(FLY_AUDIO_INFO));

	memset(&pFlyAudioInfo->sFlyAudioInfo.preEQ,(EQ_MAX/2),10);
	//memset(&pFlyAudioInfo->sFlyAudioInfo.curEQ,6,10);

	pFlyAudioInfo->sFlyAudioInfo.bMuteRadio = FALSE;
	pFlyAudioInfo->sFlyAudioInfo.bMuteBT    = FALSE;

	pFlyAudioInfo->sFlyAudioInfo.preMainVolume = 0;
	pFlyAudioInfo->sFlyAudioInfo.curMainVolume = 0;

	pFlyAudioInfo->sFlyAudioInfo.preMainAudioInput = MediaCD;
	pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput = Init;

	pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;

	pFlyAudioInfo->sFlyAudioInfo.preBassFreq = 0;
	pFlyAudioInfo->sFlyAudioInfo.preBassLevel = BASS_LEVEL_COUNT/2;
	pFlyAudioInfo->sFlyAudioInfo.preMidFreq = 0;
	pFlyAudioInfo->sFlyAudioInfo.preMidLevel = MID_LEVEL_COUNT/2;
	pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = 0;
	pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = TREB_LEVEL_COUNT/2;
	pFlyAudioInfo->sFlyAudioInfo.preLoudFreq = 0;
	pFlyAudioInfo->sFlyAudioInfo.preLoudLevel = LOUDNESS_LEVEL_COUNT/2;
	pFlyAudioInfo->sFlyAudioInfo.preBalance = BALANCE_LEVEL_COUNT/2;
	pFlyAudioInfo->sFlyAudioInfo.preFader = FADER_LEVEL_COUNT/2;
	pFlyAudioInfo->sFlyAudioInfo.preLoudnessOn = TRUE;
	pFlyAudioInfo->sFlyAudioInfo.preSubOn = TRUE;
	pFlyAudioInfo->sFlyAudioInfo.preSubFilter = 0;
	pFlyAudioInfo->sFlyAudioInfo.preSubLevel = 5;

	pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
	pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker = 0;
	pFlyAudioInfo->sFlyAudioInfo.bEnableVolumeFader = FALSE;
}

static void initFirstPowerOn(P_AUDIO_INFO pFlyAudioInfo)
{	
	pFlyAudioInfo->bKillDispatchFlyAudioDelayThread = TRUE;
	init_completion(&pFlyAudioInfo->comFlyAudioDelayThread);

	init_waitqueue_head(&pFlyAudioInfo->read_wait);
	
	pFlyAudioInfo->bNeedInit = FALSE;
	pFlyAudioInfo->bPowerUp  = FALSE;
	
	pFlyAudioInfo->bAudioMainThreadRunning = FALSE;
	pFlyAudioInfo->bAudioDelayThreadRunning = FALSE;
	pFlyAudioInfo->xxxxxxxxxxxxxxxxxxx = FALSE;
	pFlyAudioInfo->bpreDriverStatus = TRUE;
	pFlyAudioInfo->bcurDriverStatus = TRUE;
}

static void dealDataFromHal(P_AUDIO_INFO pFlyAudioInfo, BYTE *buf, UINT16 len)
{
	int i;
	int event32;
	switch (buf[0]){
	
		case 0x01:
			if (0x01 == buf[1]){
				//DBG0("\n------FlyAudio SAF7741 DRIVER INIT------\n");
				returnAudioMainInput(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMainAudioInput);
				returnAudioMainBalance(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBalance);
				returnAudioMainFader(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preFader);
				returnAudioMainTrebFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq);
				returnAudioMainTrebLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel);
				returnAudioMainMidFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidFreq);
				returnAudioMainMidLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidLevel);
				returnAudioMainBassFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassFreq);
				returnAudioMainBassLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassLevel);
				returnAudioMainLoudnessFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preLoudFreq);
				returnAudioMainLoudnessLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preLoudLevel);
				returnAudioMainLoudnessOn(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preLoudnessOn);
				returnAudioMainSubOn(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preSubOn);
				returnAudioMainSubFilter(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preSubFilter);
				returnAudioMainSubLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
				returnAudioMainSimEQ(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preSimEQ);

				////GlobalShareMmapInfo.pShareMemoryCommonData->bHaveToProcWhenSleepAudio = TRUE;

				returnAudioPowerMode(pFlyAudioInfo,TRUE);

				if (FALSE == pFlyAudioInfo->bPowerUp)
				{
					pFlyAudioInfo->bPowerUp = TRUE;
					pFlyAudioInfo->bNeedInit = TRUE;
				}

				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			}else if (0x00 == buf[1]){
				returnAudioPowerMode(pFlyAudioInfo,FALSE);
			}
			break;
			
		 case 0x03:
			if (0x01 == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preMainMute){
					pFlyAudioInfo->sFlyAudioInfo.preMainMute = FALSE;
					GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
				}else{
					pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
					GlobalShareMmapInfo.pShareMemoryCommonData->bMute = TRUE;
				}
				
				ipcStartEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
				//DBG0("\n========SAF7741 MainMute %d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);

				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainMute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMainMute);
				}
			}
			else if (0x02 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preBalance++;
				if (pFlyAudioInfo->sFlyAudioInfo.preBalance >= BALANCE_LEVEL_COUNT)
					pFlyAudioInfo->sFlyAudioInfo.preBalance = BALANCE_LEVEL_COUNT-1;
		
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainBalance(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBalance);
				}
			}
			else if (0x03 == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preBalance){
					pFlyAudioInfo->sFlyAudioInfo.preBalance--;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainBalance(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBalance);
				}
			}
			else if (0x04 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preFader++;
				if (pFlyAudioInfo->sFlyAudioInfo.preFader >= FADER_LEVEL_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preFader = FADER_LEVEL_COUNT-1;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainFader(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preFader);
				}
			}
			else if (0x05 == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preFader){
					pFlyAudioInfo->sFlyAudioInfo.preFader--;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainFader(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preFader);
				}
			}
			else if (0x06 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq++;
				if (pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq >= TREB_FREQ_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = 0;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainTrebFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq);
				}
			}
			else if (0x07 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel++;
				if (pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel >= TREB_LEVEL_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = TREB_LEVEL_COUNT-1;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainTrebLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel);
				}
			}
			else if (0x08 == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel){
					pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel--;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainTrebLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel);
				}
			}
			else if (0x09 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preMidFreq++;
				if (pFlyAudioInfo->sFlyAudioInfo.preMidFreq >= MID_FREQ_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preMidFreq = 0;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrMidFreq = pFlyAudioInfo->sFlyAudioInfo.preMidFreq;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainMidFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidFreq);
				}
			}
			else if (0x0A == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preMidLevel++;
				if (pFlyAudioInfo->sFlyAudioInfo.preMidLevel >= MID_LEVEL_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preMidLevel = MID_LEVEL_COUNT-1;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrMidLevel = pFlyAudioInfo->sFlyAudioInfo.preMidLevel;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainMidLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidLevel);
				}
			}
			else if (0x0B == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preMidLevel){
					pFlyAudioInfo->sFlyAudioInfo.preMidLevel--;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrMidLevel = pFlyAudioInfo->sFlyAudioInfo.preMidLevel;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainMidLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidLevel);
				}
			}
			else if (0x0C == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preBassFreq++;
				if (pFlyAudioInfo->sFlyAudioInfo.preBassFreq >= BASS_FREQ_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preBassFreq = 0;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrBassFreq = pFlyAudioInfo->sFlyAudioInfo.preBassFreq;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainBassFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassFreq);
				}
			}
			else if (0x0D == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preBassLevel++;
				if (pFlyAudioInfo->sFlyAudioInfo.preBassLevel >= BASS_LEVEL_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preBassLevel = BASS_LEVEL_COUNT-1;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrBassLevel = pFlyAudioInfo->sFlyAudioInfo.preBassLevel;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainBassLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassLevel);
				}
			}
			else if (0x0E == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preBassLevel){
					pFlyAudioInfo->sFlyAudioInfo.preBassLevel--;
				}
				
				pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
				pFlyAudioInfo->sFlyAudioInfo.usrBassLevel = pFlyAudioInfo->sFlyAudioInfo.preBassLevel;
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainBassLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassLevel);
				}
			}
			else if (0x0F == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preLoudFreq++;
				if (pFlyAudioInfo->sFlyAudioInfo.preLoudFreq >= LOUDNESS_FREQ_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preLoudFreq = 0;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainLoudnessFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preLoudFreq);
				}
			}
			else if (0x10 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preLoudLevel++;
				if (pFlyAudioInfo->sFlyAudioInfo.preLoudLevel >= LOUDNESS_LEVEL_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preLoudLevel = LOUDNESS_LEVEL_COUNT-1;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainLoudnessLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preLoudLevel);
				}
			}
			else if (0x11 == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preLoudLevel){
					pFlyAudioInfo->sFlyAudioInfo.preLoudLevel--;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainLoudnessLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preLoudLevel);
				}
			}
			else if (0x12 == buf[1]){
				pFlyAudioInfo->sFlyAudioInfo.preSubLevel++;
				if (pFlyAudioInfo->sFlyAudioInfo.preSubLevel >= SUB_LEVEL_COUNT){
					pFlyAudioInfo->sFlyAudioInfo.preSubLevel = SUB_LEVEL_COUNT-1;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainSubLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
				}
			}
			else if (0x13 == buf[1]){
				if (pFlyAudioInfo->sFlyAudioInfo.preSubLevel){
					pFlyAudioInfo->sFlyAudioInfo.preSubLevel--;
				}
				
				if (pFlyAudioInfo->bPowerUp){
					schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
					returnAudioMainSubLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
				}
			}
			break;
			
	 case 0x10:
		 if (BACK != buf[1])
		 {
			 pFlyAudioInfo->sFlyAudioInfo.preMainAudioInput = buf[1];

			 GlobalShareMmapInfo.pShareMemoryCommonData->eAudioInput = pFlyAudioInfo->sFlyAudioInfo.preMainAudioInput;
			 ipcStartEvent(EVENT_GLOBAL_RADIO_ANTENNA_ID);
			 ipcStartEvent(EVENT_GLOBAL_EXBOX_INPUT_CHANGE_ID);
		 }
		 
		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		 }
		 break;

	case 0x11:
		if(1 == buf[1]){
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = TRUE;
		}else{
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = FALSE;
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
		}
			
		ipcStartEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
		DBG("\n=====SAF7741 MainMute %d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);
		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;
		
	case 0x12:
		if (buf[1] <= 60){
			pFlyAudioInfo->sFlyAudioInfo.preMainVolume = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = FALSE;

			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = buf[1];
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
			returnAudioVolume(pFlyAudioInfo,GlobalShareMmapInfo.pShareMemoryCommonData->iVolume);
			ipcStartEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
			DBG("\n-------SAF7741 MainVolume %d",pFlyAudioInfo->sFlyAudioInfo.preMainVolume);
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x13:
		if (buf[1] < BALANCE_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preBalance = buf[1];

			DBG("\nSAF7741 Balance %d",pFlyAudioInfo->sFlyAudioInfo.preBalance);
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x14:
		if (buf[1] < FADER_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preFader = buf[1];
			DBG("\nSAF7741 Fader %d",pFlyAudioInfo->sFlyAudioInfo.preFader);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x15:
		if (buf[1] < TREB_FREQ_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
			pFlyAudioInfo->sFlyAudioInfo.usrTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq;

			DBG("\nSAF7741 Treble Freq %d",pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x16:
		if (buf[1] < TREB_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
			pFlyAudioInfo->sFlyAudioInfo.usrTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel;

			DBG("\nSAF7741 Treble Level %d",pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel);
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x17:
		if (buf[1] < MID_FREQ_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preMidFreq = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
			pFlyAudioInfo->sFlyAudioInfo.usrMidFreq = pFlyAudioInfo->sFlyAudioInfo.preMidFreq;

			DBG("\nSAF7741 Mid Freq %d",pFlyAudioInfo->sFlyAudioInfo.preMidFreq);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x18:
		if (buf[1] < MID_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preMidLevel = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
			pFlyAudioInfo->sFlyAudioInfo.usrMidLevel = pFlyAudioInfo->sFlyAudioInfo.preMidLevel;

			DBG("\nSAF7741 Mid Level %d",pFlyAudioInfo->sFlyAudioInfo.preMidLevel);
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x19 :
		if (buf[1] < BASS_FREQ_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preBassFreq = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
			pFlyAudioInfo->sFlyAudioInfo.usrBassFreq = pFlyAudioInfo->sFlyAudioInfo.preBassFreq;

			DBG("\nSAF7741 Bass Freq %d",pFlyAudioInfo->sFlyAudioInfo.preBassFreq);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
			
	case 0x1A:
		if (buf[1] < BASS_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preBassLevel = buf[1];
			pFlyAudioInfo->sFlyAudioInfo.preSimEQ = 0;
			pFlyAudioInfo->sFlyAudioInfo.usrBassLevel = pFlyAudioInfo->sFlyAudioInfo.preBassLevel;

			DBG("\nSAF7741 Bass Level %d",pFlyAudioInfo->sFlyAudioInfo.preBassLevel);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x1B:
		if (buf[1] < LOUDNESS_FREQ_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preLoudFreq = buf[1];
			DBG("\nSAF7741 Loud Freq %d",pFlyAudioInfo->sFlyAudioInfo.preLoudFreq);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
			
	case 0x1C:
		if (buf[1] < LOUDNESS_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preLoudLevel = buf[1];
			DBG("\nSAF7741 Loud Level %d",pFlyAudioInfo->sFlyAudioInfo.preLoudLevel);
		
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;

	case  0x1D:
		for(i = 0; i < EQ_COUNT; i++)
		{
			if (buf[i+1] > EQ_MAX) 
			{
				buf[i+1] = EQ_MAX;
			}
			pFlyAudioInfo->sFlyAudioInfo.preEQ[i] = buf[i+1];

		}

		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;

	case  0x1E:
		if ((buf[1] < EQ_COUNT) && (buf[1] > 0))
		{
			if (buf[2] > EQ_MAX)
			{
				buf[2] = EQ_MAX;
			}
			
			pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]] = buf[2];
		}

		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;

	case  0x20:
		if ((buf[1] < EQ_COUNT) && (buf[1] > 0))
		{
			if (buf[1] == 0x00) //UP
			{
				pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]]++;
				if (pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]] > EQ_MAX)
				{
					pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]] = EQ_MAX;
				}
			}
			else if(buf[1] == 0x01) //DOWN
			{
				pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]]--;
				if (pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]] < 0)
				{
					pFlyAudioInfo->sFlyAudioInfo.preEQ[buf[1]] = 0;
				}
			}
		}

		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;

	case 0x21:
		pFlyAudioInfo->sFlyAudioInfo.preSimEQ = buf[1];
		if (0x01 == buf[1]){//缺省
			pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.usrTrebleFreq;
			pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.usrTrebleLevel;
			pFlyAudioInfo->sFlyAudioInfo.preMidFreq = pFlyAudioInfo->sFlyAudioInfo.usrMidFreq;
			pFlyAudioInfo->sFlyAudioInfo.preMidLevel = pFlyAudioInfo->sFlyAudioInfo.usrMidLevel;
			pFlyAudioInfo->sFlyAudioInfo.preBassFreq = pFlyAudioInfo->sFlyAudioInfo.usrBassFreq;
			pFlyAudioInfo->sFlyAudioInfo.preBassLevel = pFlyAudioInfo->sFlyAudioInfo.usrBassLevel;
		}
		else if (0x02 == buf[1]){//古典
			pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = 1;
			pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = 9;
			pFlyAudioInfo->sFlyAudioInfo.preMidFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.preMidLevel = 3;
			pFlyAudioInfo->sFlyAudioInfo.preBassFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.preBassLevel = 10;
		}
		else if (0x03 == buf[1]){//流行
			pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = 7;
			pFlyAudioInfo->sFlyAudioInfo.preMidFreq = 1;
			pFlyAudioInfo->sFlyAudioInfo.preMidLevel = 6;
			pFlyAudioInfo->sFlyAudioInfo.preBassFreq = 1;
			pFlyAudioInfo->sFlyAudioInfo.preBassLevel = 7;
		}
		else if (0x04 == buf[1]){//摇滚
			pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = 8;
			pFlyAudioInfo->sFlyAudioInfo.preMidFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.preMidLevel = 4;
			pFlyAudioInfo->sFlyAudioInfo.preBassFreq = 0;
			pFlyAudioInfo->sFlyAudioInfo.preBassLevel = 9;
		}
		else if (0x05 == buf[1]){//爵士
			pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq = 1;
			pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel = 8;
			pFlyAudioInfo->sFlyAudioInfo.preMidFreq = 1;
			pFlyAudioInfo->sFlyAudioInfo.preMidLevel = 5;
			pFlyAudioInfo->sFlyAudioInfo.preBassFreq = 1;
			pFlyAudioInfo->sFlyAudioInfo.preBassLevel = 8;
		}
	
		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);

			returnAudioMainTrebFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq);
			returnAudioMainTrebLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel);
			returnAudioMainMidFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidFreq);
			returnAudioMainMidLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preMidLevel);
			returnAudioMainBassFreq(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassFreq);
			returnAudioMainBassLevel(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.preBassLevel);
		}
		break;
		
	case 0x30:
		if (0x01 == buf[1])
			pFlyAudioInfo->sFlyAudioInfo.preLoudnessOn = TRUE;
		else
			pFlyAudioInfo->sFlyAudioInfo.preLoudnessOn = FALSE;

		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;
	
	case 0x31:
		if (0x01 == buf[1])
			pFlyAudioInfo->sFlyAudioInfo.preSubOn = TRUE;
		else
			pFlyAudioInfo->sFlyAudioInfo.preSubOn = FALSE;
			
		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;
		
	case 0x32:
		if (buf[1] < SUB_FILTER_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preSubLevel = buf[1];
			DBG("\nSAF7741 Sub Filter %d",pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
			
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
	
	case 0x33:
		if (buf[1] < SUB_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preSubLevel = buf[1];
			DBG("\nSAF7741 Sub Level %d",pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
	
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
	
	case 0x24:
		if (0x01 == buf[1]){
			pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker = buf[1];
			//GlobalShareMmapInfo.pShareMemoryCommonData->GPSSpeaker = buf[1];
			DBG("\nSAF7741 GPSSpeaker-->%d",pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker);
	
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		else if (0x00 == buf[1]){
			pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker = buf[1];
			//GlobalShareMmapInfo.pShareMemoryCommonData->GPSSpeaker = buf[1];
			DBG("\nSAF7741 GPSSpeaker-->%d",pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker);
	
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case MSG_AUDIO_TRANS_EVENT:
		event32 = forU8ToU32LSB(&buf[1]);
		if (event32 == EVENT_AUTO_CLR_STANDBY_ID)
		{
			pFlyAudioInfo->xxxxxxxxxxxxxxxxxxx = TRUE;
		}
		else if (event32 == EVENT_AUTO_CLR_SUSPEND_ID)
		{
			DBG0("FlyAudio Driver Event--->SUSPEND");
			//setTheThreadStatus(pFlyAudioInfo,FALSE);
			pFlyAudioInfo->bpreDriverStatus = FALSE;
		}
		else if (event32 == EVENT_AUTO_CLR_RESUME_ID)
		{
			DBG0("FlyAudio Driver Event--->RESUME");
			//setTheThreadStatus(pFlyAudioInfo,TRUE);
			pFlyAudioInfo->bpreDriverStatus = TRUE;
		}
		schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
		break;
		
	case 0xFF:
		break;
		
	default:
		break;
	}
		
}

static BOOL createFlyAudioThread(P_AUDIO_INFO pFlyAudioInfo)
{
	INIT_WORK(&pFlyAudioInfo->FlyAudioMainWork,FlyAudioMainThread);
	schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
	
	DBG0("creat delay thread\n");
	pFlyAudioInfo->bKillDispatchFlyAudioDelayThread = FALSE;
	pFlyAudioInfo->taskFlyAudioDelayThread = kthread_run(FlyAudioDelayThread, pFlyAudioInfo,"FlyAudioDelayThread");
	if (IS_ERR(pFlyAudioInfo->taskFlyAudioDelayThread)){
		DBG0("kernel thread creat FlyAudioDelayThread error!\n");
		return FALSE;
	}

	printk("\nGPS Status Get Init");
	GPS_sound_init();
	pFlyAudioInfo->bKillDispatchFlyGPSAudioThread = FALSE;
	pFlyAudioInfo->taskFlyGPSAudioThread = kthread_run(FlyGPSAudioThread, pFlyAudioInfo,"FlyGPSAudioThread");
	if (IS_ERR(pFlyAudioInfo->taskFlyAudioDelayThread)){
		DBG0("kernel thread creat FlyGPSAudioThread error!\n");
		return FALSE;
	}

	/*
	printk("\nIPC Driver Init");
	pFlyAudioInfo->bKillDispatchFlyIPCDriverThread = FALSE;
	pFlyAudioInfo->taskFlyIPCDriverThread = kthread_run(FlyIPCDriverThread, pFlyAudioInfo,"FlyIPCDriverThread");
	if (IS_ERR(pFlyAudioInfo->taskFlyIPCDriverThread)){
	DBG0("kernel thread creat FlyIPCDriverThread error!\n");
	return FALSE;
	}
	*/

	return TRUE;
}

static void initAudioStruct(P_AUDIO_INFO pFlyAudioInfo)
{
	initFirstPowerOn(pFlyAudioInfo);
	
	if (!createFlyAudioThread(pFlyAudioInfo)){
		DBG0("init audio struct info error\n");
		return;
	}
	
	//control7386Mute(pFlyAudioInfo,TRUE);
	//control4052Input(pFlyAudioInfo,MediaCD);
	
	FlyAudioParaInitDefault(pFlyAudioInfo);

	global_fops._p_ipcDriver[IPC_DRIVER_AUDIO] = ipcDriverAudio;
}

static void freeAudioStruct(P_AUDIO_INFO pFlyAudioInfo)
{

}

static int audio_open(struct inode *inode, struct file *filp)
{

	struct audio_info *pFlyAudioInfo = pGlobalAudioInfo;
	if (IS_ERR_OR_NULL(pFlyAudioInfo))
		return -1;
	
	//将设备结构体指针赋值给文件私有数据指针
  	filp->private_data = pGlobalAudioInfo;
	
	DBG0("audio open OK!\n");
	return 0;
}

static ssize_t audio_write(struct file *filp, const char *buffer, size_t count, loff_t * ppos)
{
	UINT16 i=0;
	BYTE buf[300];
	
	//获得设备结构体指针
	struct audio_info *pFlyAudioInfo = filp->private_data; 
	if (IS_ERR_OR_NULL(pFlyAudioInfo))
		return -1;
				
	if (count >= 300 || count <= 0)
		return -1;
	
	//获得用户空间的数据
	if (copy_from_user(buf, buffer, count)){
		DBG0("copy data from user error\n");
		return -EFAULT;
	}else
		dealDataFromHal(pFlyAudioInfo,&buf[3],buf[2]-1);
		
	DBG("\nHal write %d bytes to AUDIO-Driver:====",count);
	for (i=0; i<count; i++){
		DBG("%02X ",buf[i]);
	}
	DBG("\n");

	return 0;
}
static ssize_t audio_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{

	UINT i = 0;
	UINT dwRead = 0;
	BYTE bufMax = 0;
	BYTE buf[300];

	//获得设备结构体指针
	struct audio_info *pFlyAudioInfo = filp->private_data; 
	if (IS_ERR_OR_NULL(pFlyAudioInfo))
		return -1;

	if (pFlyAudioInfo->buffToHalLx != pFlyAudioInfo->buffToHalHx){	

		//取得一串数据的长度
		bufMax = pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalLx][2] + 3;
		for (i=0; i<bufMax; i++){
			buf[dwRead++] = pFlyAudioInfo->buffToHal[pFlyAudioInfo->buffToHalLx][i];
		}

		//判断存数据的数据结构是否越界
		pFlyAudioInfo->buffToHalLx++;
		if (pFlyAudioInfo->buffToHalLx >= USER_BUF_MAX)
			pFlyAudioInfo->buffToHalLx = 0;

		if (copy_to_user(buffer,buf,dwRead)){
			DBG0("copy_to_user error!\n");
			return -EFAULT;
		}

		DBG("\nAUDIO-drive return %d bytes to User:",dwRead);
		for (i=0; i<dwRead; i++)
		{
			DBG("%02X ",buf[i]);
		}
		DBG("\n");
	}
	else
		return -EFAULT;

	return dwRead;
}

static unsigned int audio_poll(struct file *filp, poll_table *wait)
{
	struct audio_info *pFlyAudioInfo = filp->private_data; 
	unsigned int mask = 0;

	poll_wait(filp, &pFlyAudioInfo->read_wait, wait);

	if (pFlyAudioInfo->buffToHalLx != pFlyAudioInfo->buffToHalHx){
		mask |= POLLIN|POLLRDNORM;
	}

	return mask;
}

static int audio_release(struct inode *inode, struct file *filp)
{

	struct audio_info *pFlyAudioInfo = filp->private_data; 
	if (IS_ERR_OR_NULL(pFlyAudioInfo))
		return -1;


	DBG0("FlyAudio close OK!\n");
	return 0;
}


static struct file_operations audio_fops = {
	.owner	 =   THIS_MODULE,
	.open    =   audio_open,
	.write   =   audio_write,
	.read    =   audio_read,
	.poll    =   audio_poll,
	.release =   audio_release,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &audio_fops,
};


static int audio_probe(struct platform_device*dev)
{

	int ret = -1;
	
	// 动态申请设备结构体的内存
  	pGlobalAudioInfo = (struct audio_info *)kmalloc(sizeof(struct audio_info), GFP_KERNEL);
  	if (pGlobalAudioInfo == NULL){
		DBG0("kmalloc error!\n");
    	return ret;
  	}
	memset(pGlobalAudioInfo,0,sizeof(struct audio_info));
	
	//结构体初始化付值
  	initAudioStruct(pGlobalAudioInfo);

	//注册设备
	ret = misc_register(&misc);
	if (ret){
		//注册失败，释放设备结构体内存
		freeAudioStruct(pGlobalAudioInfo);
		return ret;
	}

	GlobalShareMmapInfo.pShareMemoryCommonData->b7741AudioInitFinish = FALSE;
	DBG0("\nFlyAudio SAF7741 init 2012-4-7");
	return ret;
}

static int audio_remove(struct platform_device *dev)
{
	freeAudioStruct(pGlobalAudioInfo);
		
	//注销设备
	misc_deregister(&misc);  
	return 0;
}

static void audio_shutdown(struct platform_device *dev)
{

}

static int audio_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int audio_resume(struct platform_device *dev)
{
	DBG0("\naudio resume\n");
	return 0;
}

static struct platform_driver audio_driver = {
	.probe     = audio_probe,
	.remove    = audio_remove,
	.shutdown  = audio_shutdown,
	.suspend   = audio_suspend,
	.resume    = audio_resume,
	.driver    = {
		.owner = THIS_MODULE,
		.name  = DEVICE_NAME,
	},
};

static int __init audio_init(void)
{
	return platform_driver_register(&audio_driver);
}

static void __exit audio_exit(void)
{
	platform_driver_unregister(&audio_driver);
}

module_init(audio_init);
module_exit(audio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");

#endif
