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

#include "FlyXMRadio.h"


//��ӡ������Ϣ
#define  FLY_DEBUG
#define  TAR "FlyXMRadio"

#ifdef  FLY_DEBUG
#define DLOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAR, __VA_ARGS__) 
#define DLOGD(...) __android_log_print(ANDROID_LOG_DEBUG ,  TAR, __VA_ARGS__)
#define DLOGI(...) __android_log_print(ANDROID_LOG_INFO  ,  TAR, __VA_ARGS__)
#define DLOGW(...) __android_log_print(ANDROID_LOG_WARN  ,  TAR, __VA_ARGS__)
#define DLOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAR, __VA_ARGS__)
#else
#define DLOGV(...) void(0)
#define DLOGD(...) void(0)
#define DLOGI(...) void(0)
#define DLOGW(...) void(0)
#define DLOGE(...) void(0)
#endif

struct flyxmradio_struct_info *pFlyXMRadioInfo = NULL;

/********************************************************************************
 **�������ƣ�bufToString()
 **�������ܣ� ����ת�����ַ���
 **����������
 **�� �� ֵ������һ��������ַ���
 **********************************************************************************/
static const BYTE *bufToString(BYTE *buf, UINT len)
{
	UINT i=0; 
	UINT j=0;
	char str[4];
	BYTE message[4096];
	
	memset(message, '\0', sizeof(message));
	
	for (i=0,j=0; i<len; i++,j=j+3)
	{
		snprintf(str, sizeof(str), "%02X ", buf[i]);
		memcpy(&message[j], str, sizeof(str)-1);
	}
	
	return message;
}

 static void flyAudioReturnToUserPutToBuff(BYTE data)
 {
	pFlyXMRadioInfo->buffToUser[pFlyXMRadioInfo->buffToUserHx++] = data;
	if (pFlyXMRadioInfo->buffToUserHx >= BUF_MAX_LEN)
	{
		pFlyXMRadioInfo->buffToUserHx = 0;
	}
 }
 

void *debugThread(void *arg)
{
	UINT i=0;
	while (1)
	{
		for (i=0; i < 10; i++)
		{
			flyAudioReturnToUserPutToBuff(0xAC);
		}

		sleep(2);
		//sem_post(&pFlyXMRadioInfo->userReadSem);
		
	}
	return NULL;
}

 static BOOL createThread(void)
 {
	INT res;
	pthread_t thread_id;

	//�̵߳����ã�
	res = pthread_create(&thread_id, NULL, debugThread,NULL);
    if(res != 0) 
	{
		return FALSE;
    }
	return TRUE;
 }
 /*==========================����Ϊ��������====================================*/
 /******************************************************************************/
 /******************************************************************************/
 /******************************************************************************/
 /*============================================================================*/
 
/********************************************************************************
 **�������ƣ�fly_open_device��������
 **�������ܣ����豸
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 INT flyOpenDevice(void)
 {
	//�̵߳�����
	if (!createThread())
	{
		//DLOGE("create read thread error");
		return -1;
	}
	
	
	//DLOGD("xmradio open ok");
	return 0;
 }
 
 /********************************************************************************
 **�������ƣ�fly_init_device_struct��������
 **�������ܣ���ʼ���ṹ����ĳ�Ա
 **����������
 **�� �� ֵ��
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
 
	//Ϊ flyxmradio_struct_info �ṹ������ڴ�
	pFlyXMRadioInfo =
		(struct flyxmradio_struct_info *)malloc(sizeof(struct flyxmradio_struct_info));
	if (pFlyXMRadioInfo == NULL)
	{
		//DLOGE("pFlyXMRadioInfo malloc error");
		return;
	}
	memset(pFlyXMRadioInfo, 0, sizeof(struct flyxmradio_struct_info));
	
	
	
	pFlyXMRadioInfo->buffToUserHx = 0;
	pFlyXMRadioInfo->buffToUserLx = 0;
	memset(pFlyXMRadioInfo->buffToUser, 0, sizeof(pFlyXMRadioInfo->buffToUser));
	
	pFlyXMRadioInfo->xmradioInfoFrameStatus    = 0;
	pFlyXMRadioInfo->xmradioInfoFrameLengthMax = 0;
	pFlyXMRadioInfo->xmradioInfoFrameLength    = 0;
	pFlyXMRadioInfo->xmradioInfoFrameCheckSum  = 0;
	memset(pFlyXMRadioInfo->xmradioInfoFrameBuff,0,sizeof(pFlyXMRadioInfo->xmradioInfoFrameBuff));
	
	
	//��ʼ������������������
	pthread_mutex_init(&pFlyXMRadioInfo->userReadMutex, NULL);
	sem_init(&pFlyXMRadioInfo->userReadSem, 0, 0);
 }
 
  /********************************************************************************
 **�������ƣ�flydvd_read()����
 **�������ܣ���������
 **����������
 **�� �� ֵ���ɹ�����ʵ�ʶ��õ����ݣ�ʧ�ܷ���-1
 **********************************************************************************/
 INT flyReadData(BYTE *buf, UINT len)
 {
	UINT i = 0;
	UINT dwRead = 0;
	
	//DLOGD("xmradio read...");
	
	sem_wait(&pFlyXMRadioInfo->userReadSem);
	
	pthread_mutex_lock(&pFlyXMRadioInfo->userReadMutex);
	if ((dwRead < len)  && (pFlyXMRadioInfo->buffToUserLx != pFlyXMRadioInfo->buffToUserHx))
	{
		while ((dwRead < len) 
			&& (pFlyXMRadioInfo->buffToUserLx != pFlyXMRadioInfo->buffToUserHx))
		{
			buf[dwRead++] = pFlyXMRadioInfo->buffToUser[pFlyXMRadioInfo->buffToUserLx++];
			if (pFlyXMRadioInfo->buffToUserLx >= BUF_MAX_LEN)
			{
				pFlyXMRadioInfo->buffToUserLx = 0;
			}
		}
		
		//DLOGI("xmradio-HAL return %d bytes to User:%s", dwRead, bufToString(buf,dwRead));
	}
	
	pthread_mutex_unlock(&pFlyXMRadioInfo->userReadMutex);

	return dwRead;
 }
 
 /********************************************************************************
 **�������ƣ�fly_destroy_struct()
 **�������ܣ��ͷ��ڴ�
 **����������
 **�� �� ֵ����
 **********************************************************************************/
void flyDestroyDeviceStruct(void)
{
	
}
 /********************************************************************************
 **�������ƣ�fly_close_device()����
 **�������ܣ��رպ���
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	
	//DLOGD("Fly xmradio Close");
	return -1;
 }

 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT len)
{
	INT i = 0;

	//DLOGD("User write %d bytes to xmradio-HAL:%s", len, bufToString(buf,len));
	
	/*
	checkTheCommand(buf, len);
	*/
}