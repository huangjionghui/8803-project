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

#include "FlyAudio.h"
#include "tda7419_data.h"
#include "../../include/HalApi.c"
#include "../../include/debug.c"
#include "../../include/hardware.c"
#include "../../include/mmap.c"

#define LINFA_HOST_CON_ID		0x01
#define LINFA_FA8200_CON_ID		0x02
#define LINFA_REMOTE_CON_ID		0x03

#define FA8200_DATA_ID 0x01

void writeDataToMcu(P_FLY_TDA7419_INFO pTda7419Info,BYTE *p,UINT length);
#define LARGER(A, B)    ((A) >= (B)? (A):(B))

static void FA8200checkbHaveFA8200(P_FLY_TDA7419_INFO pTda7419Info)
{

	BYTE buff[] = {0x01,FA8200_DATA_ID,0x09};
	
	if(0 == pTda7419Info->pFlyDriverGlobalInfo->FlySystemRunningInfo.bHaveFlyAudioExtAMP)
	{
		writeDataToMcu(pTda7419Info,buff,3);
	}
}

static void FA8200VolumeControl(P_FLY_TDA7419_INFO pTda7419Info,BYTE iVolume)
{
	BYTE  buff[] = {0x01,FA8200_DATA_ID,0x01,0x00};

	buff[3] = iVolume;
	writeDataToMcu(pTda7419Info,buff,4);
}

static void FA8200MuteControl(P_FLY_TDA7419_INFO pTda7419Info,BOOL bMute)
{
	BYTE buff[] = {0x01,FA8200_DATA_ID,0x02,0x00};

	buff[3] = bMute;
	writeDataToMcu(pTda7419Info,buff,4);
}

static void FA8200InputControl(P_FLY_TDA7419_INFO pTda7419Info,BYTE input)
{
	BYTE buff[] = {0x01,FA8200_DATA_ID,0x03,0x00};

	buff[3] = input;
	writeDataToMcu(pTda7419Info,buff,4);
}

static void FA8200EQControl(P_FLY_TDA7419_INFO pTda7419Info,BYTE *pdata)
{
	BYTE buff[12]={0x01,FA8200_DATA_ID,0x04};

	memcpy(&buff[3],pdata,9);
	writeDataToMcu(pTda7419Info,buff,12);
}

static void FA8200NaviMixControl(P_FLY_TDA7419_INFO pTda7419Info,BOOL bMix)
{
	BYTE buff[] = {0x01,FA8200_DATA_ID,0x06,0x00};

	buff[3] = bMix;
	writeDataToMcu(pTda7419Info,buff,4);
}

static void FA8200SoundAudioControl(P_FLY_TDA7419_INFO pTda7419Info,
	BYTE loudFreq,BYTE loudQ,
	BYTE trebFreq,BYTE trebQ,
	BYTE middFreq,BYTE middQ,
	BYTE bassFreq,BYTE bassQ)
{
	BYTE buff[11]={0x01,FA8200_DATA_ID,0x07};
	
	buff[3] = loudFreq;buff[4] = loudQ;
	buff[5] = trebFreq;buff[6] = trebQ;
	buff[7] = middFreq;buff[8] = middQ;
	buff[9] = bassFreq;buff[10] = bassQ;
	writeDataToMcu(pTda7419Info,buff,11);

}

static void FA8200FaderBalanceControl(P_FLY_TDA7419_INFO pTda7419Info,BYTE fader,BYTE balance)
{
	BYTE buff[] = {0x01,FA8200_DATA_ID,0x08,0x00,0x00};
	
	buff[3] = fader;
	buff[4] = balance;
	writeDataToMcu(pTda7419Info,buff,5);
}

static void FA8200MPEGAudioControl(P_FLY_TDA7419_INFO pTda7419Info,BYTE para)
{
	BYTE buff[] = {0x01,FA8200_DATA_ID,0x0B,0x00};

	buff[3] = para;
	writeDataToMcu(pTda7419Info,buff,4);
}

BYTE currentVolume,currentMute,currentInput,currentEQ[9],currentNavi;
BYTE currentLoudFreq,currentLoudLevel,currentTerbFreq,currentTrebLevel,currentMidFreq,currentMidLevel,currentBassFreq,currentBassLevel;
BYTE currentFader,currentBanlace;
BYTE currentMPEGSound;

void FA8200LowVoltageRecovery(void)
{
	currentVolume = 0xFF;currentMute = 0xFF;currentInput = 0xFF;
	memset(&currentEQ[0],0,9);currentNavi = 0xFF;
	currentLoudFreq = 0xFF;currentLoudLevel = 0xFF;
	currentTerbFreq = 0xFF;currentTrebLevel = 0xFF;
	currentMidFreq = 0xFF;currentMidLevel = 0xFF;
	currentBassFreq = 0xFF;currentBassLevel = 0xFF;
	currentFader = 0xFF;currentBanlace = 0xFF;
	currentMPEGSound = 0xFF;
	SetEvent(pTda7419Info->hDispatchExtAmpThreadEvent);
}
