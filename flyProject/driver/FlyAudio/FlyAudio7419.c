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
#include <linux/proc_fs.h>

#include "../../include/ShareMemoryStruct.h"

#if AUDIO_RADIO_CHIP_SEL == AUDIO_RADIO_7419_7541

#include "FlyAudio7419.h"
#include "tda7419_data.h"
#include "../include/fly_soc.h"
#include "../include/driver_def.h"
#include "gps-sound.h"

#include "../include/fly_soc_iic.h"

struct audio_info *pGlobalAudioInfo = NULL;

static void setTheThreadStatus(P_AUDIO_INFO pFlyAudioInfo,BOOL bAlive);
static BOOL createFlyAudioThread(P_AUDIO_INFO pFlyAudioInfo);
static void Tda7419_Input(P_AUDIO_INFO pFlyAudioInfo,BYTE channel,BYTE InputGain);

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

static void actualIICWrite(P_AUDIO_INFO pFlyAudioInfo,BYTE busID, BYTE chipAddW, BYTE *buf, UINT size)
{
	INT ret = -1;
	UINT i=0;

	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest)
	{
		return;
	}

	DBG0("\nIIC %d start write:", busID);
	for (i=0; i<size; i++)
	{
		DBG0("%02X ", buf[i]);
	}
	DBG0("\n");
	
	
	ret = SOC_I2C_Send(busID,chipAddW>>1,&buf[0],size);
	if (ret < 0)
		DBG0("IIC write error:%d!\n", ret);
}

static void control7386Mute(P_AUDIO_INFO pFlyAudioInfo,BOOL bMute)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverbAMPMute = bMute;
	ipcDriverStart(IPC_DRIVER_HARDWARE,IPC_DRIVER_EVENT_AMP_MUTE);
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

	//complete(&pFlyAudioInfo->buffCompToHal);
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

static void returnAudioVolume(P_AUDIO_INFO pFlyAudioInfo,BYTE iVolume)
{
	BYTE buf[2] = {0x12,0x00};
	buf[1] = iVolume;

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

static void I2C_Write_Tda7419(P_AUDIO_INFO pFlyAudioInfo,BYTE ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen)
{
	BYTE buff[300];

	ulRegAddr &= 0x1F;
	ulRegAddr |= 0x60;

	buff[0] = ulRegAddr;
	memcpy(&buff[1],pRegValBuf,uiValBufLen);
	
	actualIICWrite(pFlyAudioInfo,I2_1_ID,FLY7419_ADDR_W,&buff[0],uiValBufLen+1);
}

static void TDA7419_Mute_Control(P_AUDIO_INFO pFlyAudioInfo,BOOL bMute) // 7419 静音的特殊处理
{					   
	if (bMute)
		TDA7419_Para[2] = TDA7419_Para[2] & 0xFE;
	else
		TDA7419_Para[2] = TDA7419_Para[2] | 0x01;
		
	I2C_Write_Tda7419(pFlyAudioInfo,0x02, &TDA7419_Para[2], 1);
}


static void TDA7419_Navi_Mix(P_AUDIO_INFO pFlyAudioInfo,BYTE para)
{
	//INT i;
	BYTE temp;
	temp = 0;
	
	if(para)
		temp = 0xF0 | 0x00 | 0x00 | 0x00 | 0x00;//0xf0	
	else
		temp = 0xF0 | 0x00 | 0x00 | 0x02 | 0x01;//0xff

	TDA7419_Para[9] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo, 0x09, &TDA7419_Para[9], 1);

	para = para * 60 / GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax;

	if (para >= 60)
		para = 60;

	temp = VolumeMask[para];
	temp = temp & 0x7F;
	TDA7419_Para[14] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x0e, &TDA7419_Para[14], 1);
}

static void Tda7419_Mute(P_AUDIO_INFO pFlyAudioInfo,BOOL para)
{
	DBG("Tda7419_Mute para:%d\n", para);
	if(para){

		TDA7419_Mute_Control(pFlyAudioInfo,TRUE);

		if (pFlyAudioInfo->sFlyAudioInfo.preMainMute)
		{
			DBG0("preMainMute:%d  globlaMute:%d",pFlyAudioInfo->sFlyAudioInfo.preMainMute,GlobalShareMmapInfo.pShareMemoryCommonData->bMute);
			control7386Mute(pFlyAudioInfo,TRUE);
		}
		else
		{
			control7386Mute(pFlyAudioInfo,FALSE);
		}

		msleep(300);
	}
	else{

		TDA7419_Mute_Control(pFlyAudioInfo,FALSE);

		control7386Mute(pFlyAudioInfo,FALSE);
		msleep(100);
	}
}
static void TDA7419_Volume(P_AUDIO_INFO pFlyAudioInfo,BYTE Volume)
{
	BYTE temp;

	Volume = Volume * 60 / GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax;

	if (Volume > 60) 
		Volume = 60;
		
	if(EXT_TEL == pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput){
		Volume = Volume + 6;
		if (Volume > sizeof(VolumeMask))
			Volume = sizeof(VolumeMask) - 1;
	}
	
	temp = VolumeMask[Volume];
	temp = temp & 0x7F;
	TDA7419_Para[3] = temp;

	I2C_Write_Tda7419(pFlyAudioInfo,0x03, &TDA7419_Para[3], 1);
}

static void TDA7419_Loud(P_AUDIO_INFO pFlyAudioInfo,BYTE LoudFreq, BYTE LoudLevel)
{

	BYTE temp;

	if(LoudFreq > LOUDNESS_FREQ_COUNT - 1)
		LoudFreq = LOUDNESS_FREQ_COUNT - 1;
	if(LoudLevel > LOUDNESS_LEVEL_COUNT - 1)
		LoudLevel = LOUDNESS_LEVEL_COUNT - 1;

	temp = TDA7419_Para[1] & 0xC0;
	temp = temp | (LoudFreq << 4) | (0x0f & Loud_Mask[LoudLevel]);
	TDA7419_Para[1] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x01, &TDA7419_Para[1], 1);
}

static void Tda7419_Sub(P_AUDIO_INFO pFlyAudioInfo,BYTE SubFreq, BYTE SubLevel)
{

	BYTE temp;
	if(SubFreq > SUB_FILTER_COUNT - 1)
		SubFreq = SUB_FILTER_COUNT - 1;
	if(SubLevel > SUB_LEVEL_COUNT - 1)
		SubLevel = SUB_LEVEL_COUNT - 1;
		
	//设置增益
	TDA7419_Para[15] = 0x80;
	I2C_Write_Tda7419(pFlyAudioInfo,0x0F, &TDA7419_Para[15], 1);
	
	//设置频率
	temp = TDA7419_Para[8] & 0xFC;
	temp = temp | SubLevel;
	TDA7419_Para[8] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x08, &TDA7419_Para[8], 1);
}

static void Tda7419_Bass(P_AUDIO_INFO pFlyAudioInfo,BYTE BassFreq, BYTE BassLevel)
{

	BYTE temp;
	if(BassFreq > 2)
		BassFreq = 2;
	if(BassLevel > 10)
		BassLevel = 10;
		
	//设置频点
	temp = TDA7419_Para[6] & 0xE0;
	temp = temp | Treble_Middle_Bass_Mask[BassLevel];
	TDA7419_Para[6] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x06, &TDA7419_Para[6], 1);
	
	//设置频率
	temp = TDA7419_Para[8] & 0xCF;
	temp = temp | (BassFreq << 4);
	TDA7419_Para[8] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x08, &TDA7419_Para[8], 1);
}

static void Tda7419_Mid(P_AUDIO_INFO pFlyAudioInfo,BYTE MidFreq, BYTE MidLevel)
{

	BYTE temp;
	if(MidFreq > 2)
		MidFreq = 2;
	if(MidLevel > 10)
		MidLevel = 10;
		
	//设置频点
	temp = TDA7419_Para[5] & 0xE0;
	temp = temp | Treble_Middle_Bass_Mask[MidLevel];
	TDA7419_Para[5] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x05, &TDA7419_Para[5], 1);
	
	//设置频率
	temp = TDA7419_Para[8] & 0xF3;
	temp = temp | (MidFreq << 2);
	TDA7419_Para[8] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x08, &TDA7419_Para[8], 1);	
}

static void Tda7419_Treble(P_AUDIO_INFO pFlyAudioInfo,BYTE TrebFreq, BYTE TrebLevel)
{

	BYTE temp;
	if(TrebFreq > 1)
		TrebFreq = 1;
	if(TrebLevel > 10) 
		TrebLevel = 10;

	temp = TDA7419_Para[4] & 0x80;
	temp = temp | (TrebFreq << 5) | Treble_Middle_Bass_Mask[TrebLevel];

	TDA7419_Para[4] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x04, &TDA7419_Para[4], 1);
}

static void TDA7419_Balance_Fader(P_AUDIO_INFO pFlyAudioInfo,BYTE balance, BYTE Fader)
{

	BYTE temp;
	if(balance > 20) balance = 20;
	if(Fader > 20) Fader = 20;
	
	//LF
	if(balance <= 10 && Fader <= 10)
		TDA7419_Para[0x0A] = Balance_Fader_Mask[0];	
	else{
		temp = LARGER(balance,Fader);
		temp = temp - 10;
		TDA7419_Para[0x0A] = Balance_Fader_Mask[temp];	
	}
	
	//RF
	if(balance >= 10 && Fader <= 10)
		TDA7419_Para[0x0B] = Balance_Fader_Mask[0];
	else{
		temp = LARGER((20 - balance),Fader);
		temp = temp - 10;
		TDA7419_Para[0x0B] = Balance_Fader_Mask[temp];	
	}
	
	//LR
	if(balance <= 10 && Fader >= 10)
		TDA7419_Para[0x0C] = Balance_Fader_Mask[0];
	else{
		temp = LARGER(balance,(20 - Fader));
		temp = temp - 10;
		TDA7419_Para[0x0C] = Balance_Fader_Mask[temp];	
	}
	
	//RR
	if(balance >= 10 && Fader >= 10)
		TDA7419_Para[0x0D] = Balance_Fader_Mask[0];
	else{
		temp = LARGER((20 - balance),(20 - Fader));
		temp = temp - 10;
		TDA7419_Para[0x0D] = Balance_Fader_Mask[temp];	
	}
	
	I2C_Write_Tda7419(pFlyAudioInfo,0x0A, &TDA7419_Para[10], 1);
	I2C_Write_Tda7419(pFlyAudioInfo,0x0B, &TDA7419_Para[11], 1);
	I2C_Write_Tda7419(pFlyAudioInfo,0x0C, &TDA7419_Para[12], 1);
	I2C_Write_Tda7419(pFlyAudioInfo,0x0D, &TDA7419_Para[13], 1);
}

static void Tda7419_Input(P_AUDIO_INFO pFlyAudioInfo,BYTE channel,BYTE InputGain)
{
	BYTE temp;
	BYTE para = 0;//para 为1 需要52795 切换
	
	if(InputGain > 15)
		InputGain = 15;

	switch(channel){

	case Init:
		TDA7419_Para[0] = 0xff;	   //
		break;

	case MediaMP3:
	case BT_RING:
	case MediaCD:
	case AUX :
	case IPOD ://适应USB控制的IPOD
	case TV:
	case CDC:
		temp = (InputGain << 3) + 1;	 //SEL1
		TDA7419_Para[0] = temp;
		break;

	case A2DP:
	case BT:
		temp = (InputGain << 3) + 2;	 //SEL2
		TDA7419_Para[0] = temp;
		break;

	case EXT_TEL:
	case GPS:
		TDA7419_Para[0] = 0xff;	   //
		break;

		temp = (InputGain << 3) + 0;
		TDA7419_Para[0] = temp;
		break;

	case RADIO :
		//para = 1;
		temp = (InputGain << 3) + 3;	//52795选通道  SEL3	RADIO
		TDA7419_Para[0] = temp;				
		break;

	default:
		return;
		break;
	}

#if PCB_8803_AMP_SEL == PCB_8803_AMP_V2
	switch(channel){

	case Init:
		TDA7419_Para[0] = 0xff;	   //
		break;

	case MediaMP3:
	case BT_RING:
	case MediaCD:
	case AUX :
	case IPOD ://适应USB控制的IPOD
	case TV:
	case CDC:
		temp = (InputGain << 3) + 1;	 //SEL1
		TDA7419_Para[0] = temp;
		break;

	case A2DP:
	case BT:
		temp = (InputGain << 3) + 2;	 //SEL2
		TDA7419_Para[0] = temp;
		break;

	case EXT_TEL:
		//case GPRS :
	case GPS:
		TDA7419_Para[0] = 0xff;	   //
		break;

		temp = (InputGain << 3) + 0;
		TDA7419_Para[0] = temp;
		break;

	case RADIO :
		//para = 1;
		temp = (InputGain << 3) + 3;	//52795选通道  SEL3	RADIO
		TDA7419_Para[0] = temp;				
		break;

	default:
		return;
		break;
	}
#endif

	DBG0("\n---===---channel---===:->%d\n", channel);
		
	GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverMainAudioInput = channel;
	ipcDriverStart(IPC_DRIVER_HARDWARE,IPC_DRIVER_EVENT_MAIN_AUDIO_INPUT);

	DBG0("\nFlyAudio 7419 Input ----->%d",channel);

	TDA7419_Para[0] = TDA7419_Para[0] | 0x80;
	I2C_Write_Tda7419(pFlyAudioInfo,0x00, &TDA7419_Para[0], 1);

	//if(channel == EXT_TEL || channel == GPRS)
	if(channel == EXT_TEL || channel == GPS)
		temp = 0x5c;
	else  
		temp = 0x1C;

	TDA7419_Para[0x10] = temp;
	I2C_Write_Tda7419(pFlyAudioInfo,0x10, &TDA7419_Para[0x10], 1);

	if (1 == para){
		//52595channel_switch()
	}
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
	UINT i;

	BOOL bInnerDelayOn = FALSE;
	ULONG iInnerMuteTime = 0;
	ULONG iInnerMuteMax = 0;
	ULONG iInnerDemuteTime = 0;
	ULONG iInnerDemuteMax = 0;

	BOOL bInnerNeedDemute = FALSE;

	BOOL bNeedReturnBTMute = FALSE;

	P_AUDIO_INFO pFlyAudioInfo = (struct audio_info*)arg;
	pFlyAudioInfo->bAudioDelayThreadRunning = TRUE;
	
	DBG("FlyAudio delay thread start\n");
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
		{
			msleep(50);
			pFlyAudioInfo->sFlyAudioInfo.bMuteRadio = FALSE;
		}

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
					Tda7419_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);

					pFlyAudioInfo->sFlyAudioInfo.curMainVolume = 0;
					TDA7419_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);

					//设定静音时间
					iInnerMuteTime = GetTickCount();
					iInnerMuteMax = 628;
				}
				else
				{
					if ((iInnerMuteMax - (GetTickCount() - iInnerMuteTime) < 628))
					{
						iInnerMuteMax = 628;
					}
				}
			}
			else
			{
				bInnerNeedDemute = TRUE;
			}
		}

		if(pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput)
		{//切换通道
		       //MP3跟IPOD同一通道，不需要切换
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
					Tda7419_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);

					pFlyAudioInfo->sFlyAudioInfo.curMainVolume = 0;
					TDA7419_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);

					//设定静音时间
					iInnerMuteTime = GetTickCount();
					iInnerMuteMax = 314;
				}
				else
				{
					if ((iInnerMuteMax - (GetTickCount() - iInnerMuteTime) < 314))
					{
						iInnerMuteMax = 314;
					}
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
			
				if (pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput){
					
					DBG("\nTda7419 DelayThread ChangeInput", pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput);

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

					pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput;
					pFlyAudioInfo->sFlyAudioInfo.dspMainAudioInput = pFlyAudioInfo->sFlyAudioInfo.tmpMainAudioInput;

					GlobalShareMmapInfo.pShareMemoryCommonData->eCurAudioInput = pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput;
					Tda7419_Input(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput,audioChannelGainTab[pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput]);

					//设定恢复时间
					if (0 == iInnerDemuteTime)
					{
						iInnerDemuteTime = GetTickCount();
						iInnerDemuteMax = 100;
					}
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
			Tda7419_Mute(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainMute);
			msleep(50);
		}
			
		if( FALSE == bInnerNeedDemute
			&& FALSE == pFlyAudioInfo->sFlyAudioInfo.curMainMute 
			&& pFlyAudioInfo->sFlyAudioInfo.curMainVolume != pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
		{
			bInnerDelayOn = TRUE;

			//if (bVolumeFaderInOut(pFlyAudioInfo)){
			if (1)
			{
				if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume < pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
				{
						pFlyAudioInfo->sFlyAudioInfo.curMainVolume++;
				}
				else if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume > pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
				{
						pFlyAudioInfo->sFlyAudioInfo.curMainVolume--;		
				}
			}
			else
				pFlyAudioInfo->sFlyAudioInfo.curMainVolume = pFlyAudioInfo->sFlyAudioInfo.preMainVolume;


			if (pFlyAudioInfo->sFlyAudioInfo.curMainVolume == pFlyAudioInfo->sFlyAudioInfo.preMainVolume)
			{
				bInnerDelayOn = FALSE;
				volumeFaderInOut(pFlyAudioInfo,FALSE);
			}

			DBG("\nTda7419 DelayThread Change Volume:",pFlyAudioInfo->sFlyAudioInfo.curMainVolume);

			TDA7419_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);
		}
		else
		{
			bInnerDelayOn = FALSE;
		}

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
					printk("\ncurGPSSpeaker-->%d  curMainVolume-->%d",pFlyAudioInfo->sFlyAudioInfo.curGPSSpeaker,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);
					TDA7419_Navi_Mix(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curGPSSpeaker);
			}

			pFlyAudioInfo->sFlyAudioInfo.dspGPSSpeaker = pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker;
		}
	}
	
	DBG0("\nFLY7419 FlyAudioDelayThread exit");
	pFlyAudioInfo->bAudioDelayThreadRunning = FALSE;
	do_exit(0);
	return 0;
}

#define SIG_IPC_MSG 34
//#define HAL_SERVICE_NAME "flyaudioservice"
#define HAL_SERVICE_NAME "dio.osd.service"
static void xEventSendVolume(BYTE iVolume)
{
	siginfo_t ipc_info;
	struct task_struct *p;

	ipc_info.si_signo = SIG_IPC_MSG;
	ipc_info.si_code  = -1;
	ipc_info.si_int   = iVolume;

	for_each_process(p)
	{
		if (!strcmp(p->comm, HAL_SERVICE_NAME))
		{
			printk("\nsend Volume %d", iVolume);
			send_sig_info(SIG_IPC_MSG, &ipc_info, p);
			break;
		}
	}
}

void FlyAudioMainThread(struct work_struct *work)
{
	P_AUDIO_INFO pFlyAudioInfo = pGlobalAudioInfo;
	
	if (!pFlyAudioInfo->bPowerUp){
	
	}else{
		if (pFlyAudioInfo->bNeedInit){
		
			pFlyAudioInfo->bNeedInit = FALSE;

			I2C_Write_Tda7419(pFlyAudioInfo,0, &TDA7419_Para[0], 8);
			I2C_Write_Tda7419(pFlyAudioInfo,8, &TDA7419_Para[8], 9);

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

			DBG0("\nTda7419 Init OK");
		}
		
		if (ipcWhatEventOn(EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID)){
			ipcClearEvent(EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID);
			
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio){
			
			}else{
				I2C_Write_Tda7419(pFlyAudioInfo,0, &TDA7419_Para[0], 8);
				I2C_Write_Tda7419(pFlyAudioInfo,8, &TDA7419_Para[8], 9);
			}
		}
		
		if (ipcWhatEventOn(EVENT_GLOBAL_DATA_CHANGE_VOLUME)){
			ipcClearEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
			
			if (pFlyAudioInfo->sFlyAudioInfo.preMainMute != GlobalShareMmapInfo.pShareMemoryCommonData->bMute){
				pFlyAudioInfo->sFlyAudioInfo.preMainMute = GlobalShareMmapInfo.pShareMemoryCommonData->bMute;
				DBG("\nTda7419 Global Mute:%d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);
			}
			
			if (pFlyAudioInfo->sFlyAudioInfo.preMainVolume != GlobalShareMmapInfo.pShareMemoryCommonData->iVolume){
				pFlyAudioInfo->sFlyAudioInfo.preMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
				pFlyAudioInfo->sFlyAudioInfo.dspMainVolume = GlobalShareMmapInfo.pShareMemoryCommonData->iVolume;
				DBG("\nTda7419 Global Volume:%d",pFlyAudioInfo->sFlyAudioInfo.preMainVolume);
				//printk("\nTda7419 Global Volume max:%d",GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax);
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
		
		if (TRUE == pFlyAudioInfo->bAudioSleep)
		{
			DBG0("FlyAudio7741 event bStandby-->%d",GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus);
			pFlyAudioInfo->bAudioSleep = FALSE;
			if (TRUE == GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)
			{
				control7386Mute(pFlyAudioInfo,TRUE);

				msleep(100);

				//进入待机
				control7386AMPOn(pFlyAudioInfo,FALSE);
				msleep(100);
			}
			else
			{
				//让功放工作
				control7386AMPOn(pFlyAudioInfo,TRUE);

				msleep(100);
				control7386Mute(pFlyAudioInfo,FALSE);
				msleep(100);
			}
		}
		//DBG("\nbNowOnWhatRunStates:%d",GlobalShareMmapInfo.pShareMemoryCommonData->bNowOnWhatRunStates);
		DBG("\nbBatteryVoltageLowAudio:%d",GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio);
		DBG0("global bMute:%d global iVolume:%d global iVolumeMax:%d\n",
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute,
			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume,
			GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax);

		if (GlobalShareMmapInfo.pShareMemoryCommonData->bStandbyStatus)
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
		else if (GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio)
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
		//else if (!GlobalShareMmapInfo.pShareMemoryCommonData->bNowOnWhatRunStates)
		//	pFlyAudioInfo->sFlyAudioInfo.preMainMute = TRUE;
		else
			pFlyAudioInfo->sFlyAudioInfo.preMainMute = GlobalShareMmapInfo.pShareMemoryCommonData->bMute;
		
		//倒车降低音量
		if (ipcWhatEventOn(EVENT_GLOBAL_BACK_LOW_VOLUME_ID)){
			ipcClearEvent(EVENT_GLOBAL_BACK_LOW_VOLUME_ID);
			volumeFaderInOut(pFlyAudioInfo,TRUE);
		}	
		
		if (GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.bHaveFlyAudioExtAMP)
			pFlyAudioInfo->sFlyAudioInfo.preMainVolume = 55;
		else{
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackDetectEnable
				&& pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != EXT_TEL
				&& pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != BT 
				&& pFlyAudioInfo->sFlyAudioInfo.curMainAudioInput != BT_RING)
			{
				if (GlobalShareMmapInfo.pShareMemoryCommonData->bBackActiveNow)
				{
					if (GlobalShareMmapInfo.pShareMemoryCommonData->iVolume > (GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax / 3))
						pFlyAudioInfo->sFlyAudioInfo.preMainVolume = (GlobalShareMmapInfo.pShareMemoryCommonData->iVolumeMax / 3);
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
			Tda7419_Bass(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curBassFreq,pFlyAudioInfo->sFlyAudioInfo.curBassLevel);
		}
		
		if(pFlyAudioInfo->sFlyAudioInfo.curMidFreq != pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq || pFlyAudioInfo->sFlyAudioInfo.curMidLevel != pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel){
			pFlyAudioInfo->sFlyAudioInfo.curMidFreq = pFlyAudioInfo->sFlyAudioInfo.tmpMidFreq;
			pFlyAudioInfo->sFlyAudioInfo.curMidLevel = pFlyAudioInfo->sFlyAudioInfo.tmpMidLevel;
			Tda7419_Mid(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMidFreq,pFlyAudioInfo->sFlyAudioInfo.curMidLevel);
		}
		
		if(pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq != pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq || pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel != pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel)	{
			pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq = pFlyAudioInfo->sFlyAudioInfo.tmpTrebleFreq;
			pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel = pFlyAudioInfo->sFlyAudioInfo.tmpTrebleLevel;
			Tda7419_Treble(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curTrebleFreq,pFlyAudioInfo->sFlyAudioInfo.curTrebleLevel);
		}
		
		if (pFlyAudioInfo->sFlyAudioInfo.curLoudnessOn != pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn
			|| pFlyAudioInfo->sFlyAudioInfo.curLoudFreq != pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq 
			|| pFlyAudioInfo->sFlyAudioInfo.curLoudLevel != pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel){
			pFlyAudioInfo->sFlyAudioInfo.curLoudnessOn = pFlyAudioInfo->sFlyAudioInfo.tmpLoudnessOn;
			pFlyAudioInfo->sFlyAudioInfo.curLoudFreq = pFlyAudioInfo->sFlyAudioInfo.tmpLoudFreq;
			pFlyAudioInfo->sFlyAudioInfo.curLoudLevel = pFlyAudioInfo->sFlyAudioInfo.tmpLoudLevel;
			if (pFlyAudioInfo->sFlyAudioInfo.curLoudnessOn)
				TDA7419_Loud(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curLoudFreq, pFlyAudioInfo->sFlyAudioInfo.curLoudLevel);
			else
				TDA7419_Loud(pFlyAudioInfo,0, LOUDNESS_LEVEL_COUNT/2);
		}
		
		if((pFlyAudioInfo->sFlyAudioInfo.curBalance != pFlyAudioInfo->sFlyAudioInfo.tmpBalance) || (pFlyAudioInfo->sFlyAudioInfo.curFader != pFlyAudioInfo->sFlyAudioInfo.tmpFader)){
			pFlyAudioInfo->sFlyAudioInfo.curBalance = pFlyAudioInfo->sFlyAudioInfo.tmpBalance;
			pFlyAudioInfo->sFlyAudioInfo.curFader = pFlyAudioInfo->sFlyAudioInfo.tmpFader;
			TDA7419_Balance_Fader(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curBalance, pFlyAudioInfo->sFlyAudioInfo.curFader);
		}
		
		if (pFlyAudioInfo->sFlyAudioInfo.curSubOn != pFlyAudioInfo->sFlyAudioInfo.tmpSubOn
			|| pFlyAudioInfo->sFlyAudioInfo.curSubFilter != pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter
			|| pFlyAudioInfo->sFlyAudioInfo.curSubLevel != pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel){
			pFlyAudioInfo->sFlyAudioInfo.curSubOn = pFlyAudioInfo->sFlyAudioInfo.tmpSubOn;
			pFlyAudioInfo->sFlyAudioInfo.curSubFilter = pFlyAudioInfo->sFlyAudioInfo.tmpSubFilter;
			pFlyAudioInfo->sFlyAudioInfo.curSubLevel = pFlyAudioInfo->sFlyAudioInfo.tmpSubLevel;
			if (pFlyAudioInfo->sFlyAudioInfo.curSubOn)
				Tda7419_Sub(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curSubFilter,pFlyAudioInfo->sFlyAudioInfo.curSubLevel);
			else
				Tda7419_Sub(pFlyAudioInfo,0,0);
		}
	}
}

static void setTheThreadStatus(P_AUDIO_INFO pFlyAudioInfo,BOOL bAlive)
{

	if (bAlive)
	{
		msleep(200);

		//配置功放寄存器
		I2C_Write_Tda7419(pFlyAudioInfo,0, &TDA7419_Para[0], 8);
		I2C_Write_Tda7419(pFlyAudioInfo,8, &TDA7419_Para[8], 9);

		pFlyAudioInfo->sFlyAudioInfo.curMainVolume = 0;
		TDA7419_Volume(pFlyAudioInfo,pFlyAudioInfo->sFlyAudioInfo.curMainVolume);
		msleep(10);


		//让功放工作
		control7386AMPOn(pFlyAudioInfo,TRUE);

		msleep(100);
		control7386Mute(pFlyAudioInfo,FALSE);

		msleep(100);

		schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
		DBG0("\nFlyAudio ready wakeUp\n");
	}
	else
	{
		control7386Mute(pFlyAudioInfo,TRUE);

		msleep(100);

		//进入待机
		control7386AMPOn(pFlyAudioInfo,FALSE);

		msleep(100);

		DBG0("\nFlyAudio ready to sleep\n");
	}	
}

static void FlyAudioParaInitDefault(P_AUDIO_INFO pFlyAudioInfo)
{

	DBG("\nFlyAudio Para Init Default");
	
	memset(&pFlyAudioInfo->sFlyAudioInfo,0xFF,sizeof(FLY_AUDIO_INFO));

	pFlyAudioInfo->sFlyAudioInfo.bAmplifierMute = TRUE;

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
	pFlyAudioInfo->buffToHalHx = 0;
	pFlyAudioInfo->buffToHalLx = 0;
	memset(pFlyAudioInfo->buffToHal, 0, sizeof(pFlyAudioInfo->buffToHal));

	init_completion(&pFlyAudioInfo->buffCompToHal);
	
	
	pFlyAudioInfo->bKillDispatchFlyAudioDelayThread = TRUE;
	init_completion(&pFlyAudioInfo->comFlyAudioDelayThread);
	
	init_waitqueue_head(&pFlyAudioInfo->read_wait);
	
	pFlyAudioInfo->bNeedInit = FALSE;
	pFlyAudioInfo->bPowerUp = FALSE;
	
	pFlyAudioInfo->bAudioNeedSleep = FALSE;
	pFlyAudioInfo->bAudioMainThreadRunning = FALSE;
	pFlyAudioInfo->bAudioDelayThreadRunning = FALSE;
	pFlyAudioInfo->bAudioSleep = FALSE;
}

static void dealDataFromHal(P_AUDIO_INFO pFlyAudioInfo, BYTE *buf, UINT16 len)
{
	int event32;
	switch (buf[0]){
	
		case 0x01:
			if (0x01 == buf[1]){
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
				DBG0("\n========Tda7419 MainMute %d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);

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
		DBG("\n=====Tda7419 MainMute %d",pFlyAudioInfo->sFlyAudioInfo.preMainMute);
		if(pFlyAudioInfo->bPowerUp){
			schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
			flyAudioReturnToHal(pFlyAudioInfo,buf,len);
		}
		break;
		
	case 0x12:
		if (buf[1] <= 60){
			//pFlyAudioInfo->sFlyAudioInfo.preMainVolume = buf[1];
			//pFlyAudioInfo->sFlyAudioInfo.preMainMute = FALSE;

			GlobalShareMmapInfo.pShareMemoryCommonData->iVolume = buf[1];
			GlobalShareMmapInfo.pShareMemoryCommonData->bMute = FALSE;
			returnAudioVolume(pFlyAudioInfo,GlobalShareMmapInfo.pShareMemoryCommonData->iVolume);
			ipcStartEvent(EVENT_GLOBAL_DATA_CHANGE_VOLUME);
			DBG("\n-------Tda7419 MainVolume %d",pFlyAudioInfo->sFlyAudioInfo.preMainVolume);
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x13:
		if (buf[1] < BALANCE_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preBalance = buf[1];

			DBG("\nTda7419 Balance %d",pFlyAudioInfo->sFlyAudioInfo.preBalance);
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x14:
		if (buf[1] < FADER_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preFader = buf[1];
			DBG("\nTda7419 Fader %d",pFlyAudioInfo->sFlyAudioInfo.preFader);

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

			DBG("\nTda7419 Treble Freq %d",pFlyAudioInfo->sFlyAudioInfo.preTrebleFreq);

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

			DBG("\nTda7419 Treble Level %d",pFlyAudioInfo->sFlyAudioInfo.preTrebleLevel);
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

			DBG("\nTda7419 Mid Freq %d",pFlyAudioInfo->sFlyAudioInfo.preMidFreq);

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

			DBG("\nTda7419 Mid Level %d",pFlyAudioInfo->sFlyAudioInfo.preMidLevel);
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

			DBG("\nTda7419 Bass Freq %d",pFlyAudioInfo->sFlyAudioInfo.preBassFreq);

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

			DBG("\nTda7419 Bass Level %d",pFlyAudioInfo->sFlyAudioInfo.preBassLevel);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
		
	case 0x1B:
		if (buf[1] < LOUDNESS_FREQ_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preLoudFreq = buf[1];
			DBG("\nTda7419 Loud Freq %d",pFlyAudioInfo->sFlyAudioInfo.preLoudFreq);

			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
			
	case 0x1C:
		if (buf[1] < LOUDNESS_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preLoudLevel = buf[1];
			DBG("\nTda7419 Loud Level %d",pFlyAudioInfo->sFlyAudioInfo.preLoudLevel);
		
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
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
			DBG("\nTda7419 Sub Filter %d",pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
			
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		break;
	
	case 0x33:
		if (buf[1] < SUB_LEVEL_COUNT){
			pFlyAudioInfo->sFlyAudioInfo.preSubLevel = buf[1];
			DBG("\nTda7419 Sub Level %d",pFlyAudioInfo->sFlyAudioInfo.preSubLevel);
	
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
			DBG0("\nTda7419 GPSSpeaker %d",pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker);
	
			if(pFlyAudioInfo->bPowerUp){
				schedule_work(&pFlyAudioInfo->FlyAudioMainWork);
				flyAudioReturnToHal(pFlyAudioInfo,buf,len);
			}
		}
		else if (0x00 == buf[1]){
			pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker = buf[1];
			//GlobalShareMmapInfo.pShareMemoryCommonData->GPSSpeaker = buf[1];
			DBG0("\nTda7419 GPSSpeaker %d",pFlyAudioInfo->sFlyAudioInfo.preGPSSpeaker);
	
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
			pFlyAudioInfo->bAudioSleep = TRUE;
		}
		else if (event32 == EVENT_AUTO_CLR_SUSPEND_ID)
		{
			DBG0("FlyAudio Driver Event--->SUSPEND");
			setTheThreadStatus(pFlyAudioInfo,FALSE);
		}
		else if (event32 == EVENT_AUTO_CLR_RESUME_ID)
		{
			DBG0("FlyAudio Driver Event--->RESUME");
			setTheThreadStatus(pFlyAudioInfo,TRUE);;
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
	/*
	DBG0("creat main thread\n");
	pFlyAudioInfo->bKillDispatchFlyAudioMainThread = FALSE;
	ret = kernel_thread(FlyAudioMainThread, pFlyAudioInfo,CLONE_KERNEL);
	if (ret < 0){
		DBG0("kernel thread creat FlyAudioMainThread error!\n");
		return FALSE;
	}
	*/
	
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
	memcpy(TDA7419_Para, TDA7741_Init_Data, 18);
	
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
		
	if (pFlyAudioInfo->bAudioNeedSleep)
		return 0;
		
	if (count >= 300 || count <= 0)
		return -1;
	
	//获得用户空间的数据
	if (copy_from_user(buf, buffer, count)){
		DBG0("copy data from user error\n");
		return -EFAULT;
	}else
		dealDataFromHal(pFlyAudioInfo,&buf[3],buf[2]-1);
		
	DBG0("\nHal write %d bytes to AUDIO-Driver:====",count);
	for (i=0; i<count; i++){
		DBG0("%02X ",buf[i]);
	}
	DBG0("\n");

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
	
	//if (!pFlyAudioInfo->bAudioNeedSleep){
	//	wait_for_completion(&pFlyAudioInfo->buffCompToHal);
	//}
	
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

		//如果还有数据可读，再让用户读
		if (pFlyAudioInfo->buffToHalLx != pFlyAudioInfo->buffToHalHx)
			complete(&pFlyAudioInfo->buffCompToHal);

		if (copy_to_user(buffer,buf,dwRead)){
			DBG0("copy_to_user error!\n");
			return -EFAULT;
		}
		
		//DBG0("\nAUDIO-drive return %d bytes to User:",dwRead);
		//for (i=0; i<dwRead; i++)
		//{
		//	DBG0("%02X ",buf[i]);
		//}
		//DBG0("\n");
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
	
	//printk("audio driver have datas to read\n");
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
	
	DBG0("\nFlyAudio TDA7419 init 2012-5-23");
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

int FlyAudio_proc_message_read(char *buffer,
				  char **buffer_location,
				  off_t offset, int buffer_length, int *eof, void *data)
{
	int ret;

	ret = sprintf(buffer, "cv%d pv%d vi%d pi%d cm%d tm%d\n"
			,pGlobalAudioInfo->sFlyAudioInfo.curMainVolume
			,pGlobalAudioInfo->sFlyAudioInfo.preMainVolume
			,pGlobalAudioInfo->sFlyAudioInfo.curMainAudioInput
			,pGlobalAudioInfo->sFlyAudioInfo.preMainAudioInput
			,pGlobalAudioInfo->sFlyAudioInfo.curMainMute
			,pGlobalAudioInfo->sFlyAudioInfo.tmpMainMute);
	return ret;
}

int FlyAudio_proc_tda7419_read(char *buffer,
							   char **buffer_location,
							   off_t offset, int buffer_length, int *eof, void *data)
{
	int ret;

	ret = sprintf(buffer, "TDA7419 REG-->%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n"
		,TDA7419_Para[0]
		,TDA7419_Para[1]
		,TDA7419_Para[2]
		,TDA7419_Para[3]
		,TDA7419_Para[4]
		,TDA7419_Para[5]
		,TDA7419_Para[6]
		,TDA7419_Para[7]
		,TDA7419_Para[8]
		,TDA7419_Para[9]
		,TDA7419_Para[10]
		,TDA7419_Para[11]
		,TDA7419_Para[12]
		,TDA7419_Para[13]
		,TDA7419_Para[14]
		,TDA7419_Para[15]
		,TDA7419_Para[16]
		,TDA7419_Para[17]
		);
	return ret;
}

static struct proc_dir_entry *FlyAudio_proc_dir;
static struct proc_dir_entry *FlyAudio_proc_msg;
static struct proc_dir_entry *FlyAudio_proc_TDA7419_reg;
static int __init audio_init(void)
{
	FlyAudio_proc_dir = proc_mkdir("FlyAudio", NULL);
	if (!FlyAudio_proc_dir) {
		remove_proc_entry("FlyAudio", NULL);
		printk(KERN_ERR "Can't create /proc/FlyAudio\n");
		return -1;
	}

	FlyAudio_proc_msg = create_proc_entry("message", 0666, FlyAudio_proc_dir);
	if (!FlyAudio_proc_msg) {
		printk(KERN_ERR "Can't create /proc/mydir/message\n");
		remove_proc_entry("message", NULL);
		remove_proc_entry("FlyAudio", NULL);
		return -1;
	}

	FlyAudio_proc_TDA7419_reg = create_proc_entry("tda7419", 0666, FlyAudio_proc_dir);
	if (!FlyAudio_proc_msg) {
		printk(KERN_ERR "Can't create /proc/mydir/message\n");
		remove_proc_entry("tda7419", NULL);
		remove_proc_entry("FlyAudio", NULL);
		return -1;
	}

	FlyAudio_proc_msg->read_proc = FlyAudio_proc_message_read;
	FlyAudio_proc_TDA7419_reg->read_proc = FlyAudio_proc_tda7419_read;
	return platform_driver_register(&audio_driver);
}

static void __exit audio_exit(void)
{
	platform_driver_unregister(&audio_driver);
	DBG0("FlyAudio unload\n");
}

module_init(audio_init);
module_exit(audio_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");

#endif
