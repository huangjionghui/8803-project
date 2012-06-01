#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <poll.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <cutils/atomic.h>
#include <hardware/hardware.h>  

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_AUDIO
#define LOCAL_HAL_NAME		"flyaudio Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_AUDIO

#include "FlyAudio.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"


struct flyaudio_struct_info *pFlyAudioInfo = NULL;
static void writeDataToFlyAudioDriver(P_FLY_AUDIO_INFO pFlyAudioInfo, BYTE *buf, UINT len);
static void sendEventToFlyAudioDriver(P_FLY_AUDIO_INFO pFlyAudioInfo,UINT32 sourceEvent);

void ipcEventProcProc(UINT32 sourceEvent)
{
	sendEventToFlyAudioDriver(pFlyAudioInfo, sourceEvent);
}

void readFromhardwareProc(BYTE *buf,UINT length)
{
}

static void sendEventToFlyAudioDriver(P_FLY_AUDIO_INFO pFlyAudioInfo,UINT32 sourceEvent)
{
	UINT i = 0;
	BYTE crc = 0;
	BYTE buff[9] = {0xFF,0x55,0x06,MSG_AUDIO_TRANS_EVENT,0x00,0x00,0x00,0x00,0x00};
	forU32TopU8LSB(sourceEvent, &buff[4]);
	for (i=0; i<6; i++)
	{
		crc += buff[2+i];
	}
	buff[2+i] = crc;
	
	writeDataToFlyAudioDriver(pFlyAudioInfo,buff,9);
}

static void flyAudioReturnToUser(P_FLY_AUDIO_INFO pFlyAudioInfo,BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,&buf[3],len-4);
	if (dwLength)
	{
		DBG1(debugBuf("\nAUDIO-HAL write  bytes to User OK:", &buf[3],len-4);)
	}
	else
	{
		DBG1(debugBuf("\nAUDIO-HAL write  bytes to User Error:", &buf[3],len-4);)
	}
}

static BOOL returnCurrentVolume(BYTE *buff, UINT len)
{
	if (pFlyAllInOneInfo->pMemory_Share_Common->bNeedReturnNewVolume)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->bNeedReturnNewVolume = FALSE;
		buff[0] = 0xFF;buff[1] = 0x55;
		buff[2] = 3;
		buff[3] = 0x12;
		if (pFlyAllInOneInfo->pMemory_Share_Common->bMute)
		{
			buff[4] = 0;
		}
		else
		{
			buff[4] = pFlyAllInOneInfo->pMemory_Share_Common->iVolume;
		}
		buff[5] = buff[2] + buff[3] + buff[4];//CRC
		return TRUE;
	}
	return FALSE;
}

void *FlyAudioRead(void *arg) 
{
	INT ret = -1;
	BYTE buf[300];
	struct pollfd read_fds;
	P_FLY_AUDIO_INFO pFlyAudioInfo = (P_FLY_AUDIO_INFO)arg;
	
	memset(&read_fds, 0, sizeof(read_fds));
	read_fds.fd = pFlyAudioInfo->fd;
	read_fds.events = POLLIN|POLLRDNORM;
	while (!pFlyAudioInfo->bKillAudioReadThread){
		
		if (poll(&read_fds, 1, -1) > 0){
			if ((read_fds.revents & (POLLIN|POLLRDNORM)) == (POLLIN|POLLRDNORM)){
				ret = read(pFlyAudioInfo->fd, buf, 300);
				if (ret > 0){
					
					DBG1(debugBuf("audio hal read:", buf,ret);)
					flyAudioReturnToUser(pFlyAudioInfo, buf, ret);
				}
			}
		}
		else{
			DBG0(debugString("audio hal read err\n");)
		}
	}

	DBG1(debugString("\nTda7419 FlyAudioRead Exit");)
	return NULL;
}

static void writeDataToFlyAudioDriver(P_FLY_AUDIO_INFO pFlyAudioInfo, BYTE *buf, UINT len)
{
	if (write(pFlyAudioInfo->fd, buf, len) < 0)
		DBG0(debugString("\nFlyAudio write data to /dev/FlyAudio error");)
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

	if (pFlyAudioInfo->fd)
	{
		return ret;
	}

	pFlyAudioInfo->fd = open("/dev/FlyAudio", O_RDWR);
	if (pFlyAudioInfo->fd <= 0)
	{
		return ret;
	}

	pFlyAudioInfo->bKillAudioReadThread = FALSE;
	res = pthread_create(&thread_id,NULL,FlyAudioRead,pFlyAudioInfo);
	if(res != 0)
	{
		pFlyAudioInfo->bKillAudioReadThread = TRUE;
		return ret;
	}

	ret = HAL_AUDIO_RETURN_FD;
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

	//为 flyaudio_struct_info 结构体分配内存
	pFlyAudioInfo =
		(struct flyaudio_struct_info *)malloc(sizeof(struct flyaudio_struct_info));
	if (pFlyAudioInfo == NULL)
	{
		return;
	}
	memset(pFlyAudioInfo, 0, sizeof(struct flyaudio_struct_info));

	allInOneInit();//之后可以打印调试信息

	DBG0(debugString("\nFlyAudio hal init\n");)
	DBG0(debugString(__TIME__);)
	DBG0(debugString(__DATE__);)
	DBG0(debugString(" \n");)
	
	return;
 }
 
  /********************************************************************************
 **函数名称：flyaudio_read()函数
 **函数功能：读出数据
 **函数参数：
 **返 回 值：成功返回实际读得的数据，失败返回-1
 **********************************************************************************/
 INT flyReadData(BYTE *buf, UINT len)
 {
	 UINT16 dwRead;
	 do 
	 {
		 if (returnCurrentVolume(buf,len))
		 {
			 DBG1(debugBuf("\nAUDIO-HAL return  bytes Startxxxxxxxx:", buf,len);)
			 break;
		 }
		 
		 DBG1(debugBuf("\nAUDIO-HAL return  bytes Start:", buf,1);)
		 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
		 DBG1(debugBuf("\nAUDIO-HAL return  bytes to User:", buf,dwRead);)
	 } while (0x12 == buf[3]);
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
	pFlyAudioInfo->bKillAudioReadThread = TRUE;
	
	close(pFlyAudioInfo->fd);
	
	allInOneDeinit();

	free (pFlyAudioInfo);
	pFlyAudioInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {	
	DBG0(debugString("\nclose device !");)
	pFlyAudioInfo->bKillAudioReadThread = TRUE;

	DBG0(debugString("\nFly Audio Close");)
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
	DBG0(debugBuf("\nUser write  bytes to Audio-HAL:",buf,len);)
	
	writeDataToFlyAudioDriver(pFlyAudioInfo, buf, len);
}