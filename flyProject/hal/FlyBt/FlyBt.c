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

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_BT
#define LOCAL_HAL_NAME		"flybt Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_BT

#define SERIAL_NAME			"/dev/tcc-uart1"    
#define SERIAL_BAUDRATE      9600     

#include "FlyBt.h"
#include "btdfu/btdfu.h"
#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"
#include "../../include/serial.c"

P_FLY_BT_INFO pGlobalFlyBtInfo = NULL;
static void controlBTDeletePairedDevice(P_FLY_BT_INFO pFlyBtInfo);
static void controlBTAudioTrans(P_FLY_BT_INFO pFlyBtInfo);
static void controlBTCallStatusChange(P_FLY_BT_INFO pFlyBtInfo,BYTE para);

void readFromhardwareProc(BYTE *buf,UINT length)
{
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	P_FLY_BT_INFO pFlyBtInfo = pGlobalFlyBtInfo;
	switch (sourceEvent)
	{
	case EVENT_AUTO_CLR_RESUME_ID:
		pFlyBtInfo->bPowerUp = FALSE;
#if FOR_BT_DFU
#else
		tcflush(pFlyBtInfo->flybt_fd, TCIFLUSH);
#endif
		break;
	case EVENT_AUTO_CLR_SUSPEND_ID:
		if (pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus)//切回手机
		{
			controlBTAudioTrans(pFlyBtInfo);
		}
		if (pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bRecWinCEFactoryMsg = FALSE;
			controlBTDeletePairedDevice(pFlyBtInfo);
			//加延时3毫秒左右
		}
		break;
	case EVENT_AUTO_CLR_STANDBY_ID:
		//在线程里面处理
		break;

	default:
		break;
	}
	
	PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
}


static void collex_PowerControl_On(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[] = {SHARE_MEMORY_BT,MSG_BT_CON_POWER,1};
	writeDataToHardware(buff, sizeof(buff));

	DBG0(debugString("\nCollex PowerControl On");)
}

static void collex_PowerControl_Off(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[] = {SHARE_MEMORY_BT,MSG_BT_CON_POWER,0};
	writeDataToHardware(buff, sizeof(buff));

	DBG0(debugString("\nCollex PowerControl Off");)
}

static void collex_ResetControl_On(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[] = {SHARE_MEMORY_BT,MSG_BT_CON_RESET_ON};
	writeDataToHardware(buff, sizeof(buff));
	
	pFlyBtInfo->iAutoResetControlTime = GetTickCount();
	DBG0(debugString("\nCollex ResetControl On");)
}

static void collex_ResetControl_Off(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[] = {SHARE_MEMORY_BT,MSG_BT_CON_RESET_OFF};
	writeDataToHardware(buff, sizeof(buff));

	pFlyBtInfo->iAutoResetControlTime = GetTickCount();
	DBG0(debugString("\nCollex ResetControl Off");)
}

static void collex_ChipEnableControl_Normal(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[] = {SHARE_MEMORY_BT,MSG_BT_CON_CE_NORMAL};
	writeDataToHardware(buff, sizeof(buff));

	DBG0(debugString("\nCollex ChipEnableControl On");)
}

static void collex_ChipEnableControl_Update(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[] = {SHARE_MEMORY_BT,MSG_BT_CON_CE_UPDATE};
	writeDataToHardware(buff, sizeof(buff));

	DBG0(debugString("\nCollex ChipEnableControl Off");)
}

static void phoneListClearAll(P_FLY_BT_INFO pFlyBtInfo)
{
	P_COLLEX_BT_PHONE_LIST p,nextP;
	p = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	while(p)
	{
		nextP = p->Next;
		free(p);
		p = nextP;
	}
	pFlyBtInfo->sCollexBTInfo.pBTPhoneList = NULL;
}

static void phoneListClearSelectList(P_FLY_BT_INFO pFlyBtInfo,BYTE iWhichList)
{
	P_COLLEX_BT_PHONE_LIST p,nextP,lastP;
	p = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	while(p)
	{
		nextP = p->Next;
		if (iWhichList == p->iWhichPhoneList)
		{
			if (p == pFlyBtInfo->sCollexBTInfo.pBTPhoneList)
			{
				pFlyBtInfo->sCollexBTInfo.pBTPhoneList = p->Next;
			}
			else
			{
				lastP->Next = p->Next;
			}
			free(p);
		}
		lastP = p;
		p = nextP;
	}
}

P_COLLEX_BT_PHONE_LIST phoneListNewOne(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf,UINT len)
{
	P_COLLEX_BT_PHONE_LIST newP = (P_COLLEX_BT_PHONE_LIST)malloc(sizeof(COLLEX_BT_PHONE_LIST));

	if ((len-1+2) <= (COLLEX_PHONE_NUMBER+COLLEX_PHONE_NAME))
	{
		memcpy(newP->cPhoneNumberName,&buf[1],len-1);
		newP->cPhoneNumberName[len-1] = '\r';newP->cPhoneNumberName[len] = '\n';
		newP->iPhoneNumberNameLength = len+1;
	} 
	else
	{
		newP->cPhoneNumberName[0] = '\r';newP->cPhoneNumberName[1] = '\n';
		newP->iPhoneNumberNameLength = 2;
	}

	UINT readOffset = 1;
	UINT writeOffset;
	newP->iWhichPhoneList = buf[0];

	writeOffset = 0;
	while (writeOffset < COLLEX_PHONE_NUMBER && readOffset < len && (!(buf[readOffset] == '+' && writeOffset != 0)))
	{
		newP->cPhoneNumber[writeOffset++] = buf[readOffset++];
	}

	readOffset++;
	newP->iPhoneType = buf[readOffset++];

	readOffset++;
	writeOffset = 0;
	while (writeOffset < COLLEX_PHONE_NAME && readOffset < len && (!(buf[readOffset] == '\r' && buf[readOffset+1] == '\n')))
	{
		newP->cPhoneName[writeOffset++] = buf[readOffset++];
		newP->iPhoneNameLength = writeOffset;
	}

	newP->Next = NULL;

	return newP;
}

static P_COLLEX_BT_PHONE_LIST phoneEachListByNumber(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf,UINT len)
{
	P_COLLEX_BT_PHONE_LIST p;
	p = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	while (p){
		if (!memcmp(p->cPhoneNumber,buf,len))
			break;
		else
			p = p->Next;
	}
	
	return p;
}

static void phoneListAdd(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf,UINT len)
{
	P_COLLEX_BT_PHONE_LIST p;

	P_COLLEX_BT_PHONE_LIST newP = phoneListNewOne(pFlyBtInfo,buf,len);//添加一个电话

	if (pFlyBtInfo->sCollexBTInfo.pBTPhoneList)
	{
		p = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;//添加到链表中
		while (p->Next)
		{
			p = p->Next;
		}
		p->Next = newP;
	}
	else
	{
		pFlyBtInfo->sCollexBTInfo.pBTPhoneList = newP;
	}
}

static void phoneListReplace(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf,UINT len,UINT Count)//Count为某类别的Count
{
	P_COLLEX_BT_PHONE_LIST pLast,pCurrent;
	UINT iCount;
	pLast = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	pCurrent = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	iCount = 0;

	P_COLLEX_BT_PHONE_LIST newP = phoneListNewOne(pFlyBtInfo,buf,len);//添加一个电话


	if (pCurrent)//有
	{
		while (pCurrent)
		{
			if (pCurrent->iWhichPhoneList == newP->iWhichPhoneList)//此类别
			{
				if (iCount == Count)//找到序号
				{
					if (pLast == pCurrent)//只会在链表头部出现
					{
						newP->Next = pCurrent->Next;
						pFlyBtInfo->sCollexBTInfo.pBTPhoneList = newP;
						free(pCurrent);
						pCurrent = NULL;
					}
					else
					{
						newP->Next = pCurrent->Next;
						pLast->Next = newP;
						free(pCurrent);
						pCurrent = NULL;
					}
					return;
				}
				iCount++;
			}
			pLast = pCurrent;
			pCurrent = pCurrent->Next;
		}
		pLast->Next = newP;
	}
	else//无，则直接添加
	{
		pFlyBtInfo->sCollexBTInfo.pBTPhoneList = newP;
	}
}

P_COLLEX_BT_PHONE_LIST phoneListGetSelectOne(P_FLY_BT_INFO pFlyBtInfo,BYTE iWhichSelect,BYTE iCount)
{
	P_COLLEX_BT_PHONE_LIST p;
	UINT iCountCurrent = 0;

	p = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	while (p)
	{
		if (iWhichSelect == p->iWhichPhoneList)
		{
			if (iCountCurrent == iCount)
			{
				return p;
			}
			iCountCurrent++;
		}
		p = p->Next;
	}

	return NULL;
}

UINT phoneListGetSelectCount(P_FLY_BT_INFO pFlyBtInfo,BYTE iWhichSelect)
{
	P_COLLEX_BT_PHONE_LIST p;
	BYTE iCount = 0;

	p = pFlyBtInfo->sCollexBTInfo.pBTPhoneList;
	while (p)
	{
		if (iWhichSelect == p->iWhichPhoneList)
		{
			iCount++;
		}
		p = p->Next;
	}

	return iCount;
}

static UINT16 removeSpecialChar(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf, UINT16 len)
{
	UINT16 ret = len;
	UINT16 i = 0;
	
	if (buf[len-2] == 0x0D && buf[len-1] == 0x0A)
	{
		if (buf[len-6] == 0x2F)
		{
			buf[len-6] = 0x0D;
			buf[len-5] = 0x0A;
			ret = len-4;
		}
	}
	return ret;
}

static void flyAudioReturnToUser(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf, UINT16 len)
{
	UINT dwLength;

	dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	if (dwLength)
	{
		DBG1(debugBuf("\nBT-HAL write  bytes to User OK:", buf,len);)
	}
	else
	{
		DBG1(debugBuf("\nBT-HAL write  bytes to User Error:", buf,len);)
	}
}


static void returnMobilePowerMode(P_FLY_BT_INFO pFlyBtInfo,BOOL bPower)
{
	BYTE buff[2]={0x01,0x00};
	if (bPower)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobileWorkMode(P_FLY_BT_INFO pFlyBtInfo,BOOL bWork)
{
	BYTE buff[2]={0x02,0x00};
	if (bWork)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobilePairStatus(P_FLY_BT_INFO pFlyBtInfo,BOOL bPair)
{
	BYTE buff[2]={0x20,0x00};
	if (bPair)
	{
		buff[1] = 0x01;
	} 

	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobileAudioTransfer(P_FLY_BT_INFO pFlyBtInfo,BOOL bModule)
{
	BYTE buff[2]={0x21,0x01};
	if (bModule)
	{
		buff[1] = 0x00;
	} 

	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobileDialStatus(P_FLY_BT_INFO pFlyBtInfo,BYTE para)
{
	BYTE buff[2] = {0x23,0x00};
	if ('A' == para)
	{
		buff[1] = 0x03;
	} 
	else if ('R' == para)
	{
		buff[1] = 0x01;
	}
	else if ('D' == para)
	{
		buff[1] = 0x02;
	}

	
	if (pFlyBtInfo->sCollexBTInfo.cMobileCallStatus == 'H')
	{
		pFlyBtInfo->sCollexBTInfo.bHangStatusCore = 0xFF;
		pFlyBtInfo->sCollexBTInfo.iWaitingCallLen = 0;
		memset(pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber,0,COLLEX_PHONE_NUMBER);
	}
	
	 controlBTCallStatusChange(pFlyBtInfo,buff[1]);

#if COLLEX_AUDIO_TRANS_USE_COLLEX
#else
	if (0x00 == buff[1])
	{
		returnMobileAudioTransfer(pFlyBtInfo,TRUE);
	}
#endif
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

 static void returnBTA2DPbConnect(P_FLY_BT_INFO pFlyBtInfo, BOOL bConnect)
{
	BYTE buff[2]={0x50,0x00};
	if (bConnect)
	{
		buff[1] = 0x01;
	}
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobileLinkStatus(P_FLY_BT_INFO pFlyBtInfo,BOOL bLink)
{
	BYTE buff[2]={0x22,0x00};
	if (bLink)
	{
		buff[1] = 0x01;
	} 
	
	if (FALSE == bLink)
	{
		pFlyBtInfo->sCollexBTInfo.cMobileCallStatus = 'H';
		returnMobileDialStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.cMobileCallStatus);
		
		pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection = FALSE;
		returnBTA2DPbConnect(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection);
	}

	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobileDailInNumber(P_FLY_BT_INFO pFlyBtInfo,BYTE *p,UINT length)
{
	P_COLLEX_BT_PHONE_LIST plist;
	BYTE buff[2+COLLEX_PHONE_NUMBER+COLLEX_PHONE_NAME+1];
	buff[0] = 0x23;
	buff[1] = 0x01;
	
	controlBTCallStatusChange(pFlyBtInfo,buff[1]);
	
	if (length == 0)
	{
		memset(pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber,'\0',
				sizeof(pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber));

		flyAudioReturnToUser(pFlyBtInfo,buff,2);
	}
	else if (length < (COLLEX_PHONE_NUMBER+COLLEX_PHONE_NAME+1))
	{
		if (memcmp(pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber,p,length))
		{
			memcpy(pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber,p,length);
			memcpy(&buff[2],p,length);
			
			plist = phoneEachListByNumber(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber,length);
			if (plist != NULL){
				buff[length+2] = '+';
				length +=3;
				memcpy(&buff[length],plist->cPhoneName,plist->iPhoneNameLength);
				length += (plist->iPhoneNameLength);
				flyAudioReturnToUser(pFlyBtInfo,buff,length);
			}
			else
				flyAudioReturnToUser(pFlyBtInfo,buff,length+2);
		}
	}
}
static void returnMobilePhoneList(P_FLY_BT_INFO pFlyBtInfo,P_COLLEX_BT_PHONE_LIST p)
{
	BYTE buff[128];
	buff[0] = 0x27;

	if (p)
	{
		memcpy(&buff[1],p->cPhoneNumberName,p->iPhoneNumberNameLength);
		flyAudioReturnToUser(pFlyBtInfo,buff,p->iPhoneNumberNameLength+1);
	}
}
static void returnMobileName(P_FLY_BT_INFO pFlyBtInfo,BYTE *p,UINT len)
{
	BYTE buff[64];
	buff[0] = 0x31;
	memcpy(&buff[1],p,len);
	flyAudioReturnToUser(pFlyBtInfo,buff,len+1);
}

////---------------
//static void returnMobileCallInNumber(BYTE *p,UINT len)
//{
//	BYTE buff[64];
//	buff[0] = 0x06;buff[1] = 0x01;
//	memcpy(&buff[2],p,len);
//
//#if GLOBAL_COMM
//	//全局
//	memcpy(pFlyBtInfo->pFlyDriverGlobalInfo->FlyGlobalBTInfo.cMobileCallInNumber,p,len);
//	pFlyBtInfo->pFlyDriverGlobalInfo->FlyGlobalBTInfo.iMobileCallInNumberLength = len;
//	SetEvent(pFlyBtInfo->hHandleGlobalGlobalEvent);
//#endif
//
//	flyAudioReturnToUser(pFlyBtInfo,buff,len+2);
//}

static void returnBTVersion(P_FLY_BT_INFO pFlyBtInfo,BYTE *p,UINT len)
{
	BYTE buff[64];
	buff[0] = 0x2F;
	memcpy(&buff[1],p,len);
	flyAudioReturnToUser(pFlyBtInfo,buff,len+1);
}

static void returnMobileSignalStrength(P_FLY_BT_INFO pFlyBtInfo,BYTE para)
{
	BYTE buff[2]={0x40,0x00};
	buff[1] = para;
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnMobileBattery(P_FLY_BT_INFO pFlyBtInfo,BYTE para)
{
	BYTE buff[2]={0x41,0x00};
	buff[1] = para;
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}

static void returnBTPowerStatus(P_FLY_BT_INFO pFlyBtInfo,BOOL bWork)
{
	BYTE buff[2] = {0x10,0x00};
	if (bWork)
	{
		buff[1] = 0x01;
	} 
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}
static void returnCurrentPhoneBookType(P_FLY_BT_INFO pFlyBtInfo,BYTE type)
{
	BYTE buff[2]={0x25,0x00};
	buff[1] = type;
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}
static void returnBTWorkStatus(P_FLY_BT_INFO pFlyBtInfo,BOOL bWork)
{
	BYTE buff[2]={0x30,0x00};
	if (bWork)
	{
		buff[1] = 0x01;
	}
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}
static void returnBTPhoneBookReadStatus(P_FLY_BT_INFO pFlyBtInfo,BYTE iStatus)
{
	BYTE buff[2]={0x32,0x00};
	buff[1] = iStatus;
	
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}
static void returnBTPhoneBookPageFlush(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[2]={0x33,0x00};
	flyAudioReturnToUser(pFlyBtInfo,buff,2);
}
static void returnWaitingCallNumber(P_FLY_BT_INFO pFlyBtInfo,BYTE *p, UINT len)
{
	BYTE buff[64];
	buff[0] = 0x40;
	memcpy(&buff[1],p,len);
	flyAudioReturnToUser(pFlyBtInfo,buff,len+1);
}
static void returnBtLoadSchedule(P_FLY_BT_INFO pFlyBtInfo,BYTE schedule)
{
	BYTE buff[3];
	buff[0] = 0x80;
	buff[1] = 0x00;
	buff[2] = schedule;
	flyAudioReturnToUser(pFlyBtInfo,buff,3);
}
static void returnBtLoadStatus(P_FLY_BT_INFO pFlyBtInfo,BOOL status)
{
	BYTE buff[3];
	buff[0] = 0x80;
	buff[1] = 0x01;
	buff[2] = !status;
	flyAudioReturnToUser(pFlyBtInfo,buff,3);
}


static void returnMobileStatusInit(P_FLY_BT_INFO pFlyBtInfo)
{
	returnMobilePairStatus(pFlyBtInfo,FALSE);
	returnMobileLinkStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bConnected);
	returnMobileDialStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.cMobileCallStatus);
	//returnMobileAudioTransfer(pFlyBtInfo,!pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus);
	returnMobileName(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sDeviceName,pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);
	returnMobileSignalStrength(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.mobileSignal);
	returnMobileBattery(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.mobileBattery);
	
	returnBTA2DPbConnect(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection);
	
	//返回工作状态
	returnBTWorkStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bWork);

}

static void collexPhoneListInit(P_FLY_BT_INFO pFlyBtInfo)
{
	pFlyBtInfo->sCollexBTInfo.iPhoneListType = 0;
	pFlyBtInfo->sCollexBTInfo._W_iPhoneListType = 0;
	pFlyBtInfo->sCollexBTInfo.bPhoneListMobileReturn = FALSE;
	pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount = 0;
	pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = FALSE;
	pFlyBtInfo->sCollexBTInfo.iPhoneListStart = 0;
	pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount = 5;
	pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;
	phoneListClearAll(pFlyBtInfo);
}

static void collexBTInfoInit(P_FLY_BT_INFO pFlyBtInfo,BOOL bInitAll)
{
	UINT i;

	DBG0(debugString(" INTIT========\n");)
	pFlyBtInfo->sCollexBTInfo.bWork = FALSE;//@@@@以后会改

	pFlyBtInfo->sCollexBTInfo.bPaired = FALSE;
	pFlyBtInfo->sCollexBTInfo.bConnected = FALSE;
	pFlyBtInfo->sCollexBTInfo.bBtPowerOnNeedTOReturnPhone = FALSE;

	pFlyBtInfo->sCollexBTInfo.iPairedStatus = 0;

	pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
	pFlyBtInfo->sCollexBTInfo.iControlReqStep = 0;

	pFlyBtInfo->sCollexBTInfo.iPairedDeviceType = 0;
	memset(pFlyBtInfo->sCollexBTInfo.BDAddress,0,6);

	memset(pFlyBtInfo->sCollexBTInfo.sVersion,'0',8);

	memset(pFlyBtInfo->sCollexBTInfo.sDeviceName,0,COLLEX_PHONE_NAME);
	pFlyBtInfo->sCollexBTInfo.iDeviceNameLength = 0;

	memset(pFlyBtInfo->sCollexBTInfo.sWaitingCallNumber,0,COLLEX_PHONE_NAME);
	pFlyBtInfo->sCollexBTInfo.iWaitingCallLen = 0;
	pFlyBtInfo->sCollexBTInfo.iWaitingCallType = 0;
	memset(pFlyBtInfo->sCollexBTInfo.sCallInPhoneNumber,0,COLLEX_PHONE_NUMBER);

	pFlyBtInfo->sCollexBTInfo.cMobileCallStatus = 'H';
	pFlyBtInfo->sCollexBTInfo.cPerMobileCallstatus = 'H';

	pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus = TRUE;
	pFlyBtInfo->sCollexBTInfo.iAudioConnectionStatusTime = 0;
	pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatusChange = FALSE;
	pFlyBtInfo->sCollexBTInfo.bBTHangStatus = FALSE;
	pFlyBtInfo->sCollexBTInfo.bHangStatusCore = 0xFF;

	pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection = FALSE;

	pFlyBtInfo->sCollexBTInfo.mobileBattery = 0;
	pFlyBtInfo->sCollexBTInfo.mobileSignal = 0;
	pFlyBtInfo->sCollexBTInfo.mobileVolume = 0;

	pFlyBtInfo->sCollexBTInfo.iPhoneListType = 0;
	pFlyBtInfo->sCollexBTInfo._W_iPhoneListType = 0;
	pFlyBtInfo->sCollexBTInfo.bPhoneListMobileReturn = FALSE;
	pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount = 0;
	pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = FALSE;
	pFlyBtInfo->sCollexBTInfo.iPhoneListStart = 0;
	pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount = 5;
	pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;
	
	collexPhoneListInit(pFlyBtInfo);

	pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = FALSE;
	for (i = 0;i < 7;i++)
	{
		pFlyBtInfo->sCollexBTInfo.bPhoneListPhoneReadFinish[i] = FALSE;
	}

	if (pFlyBtInfo->bOpen)
	{
		returnMobileStatusInit(pFlyBtInfo);
	}

}

static void controlBTCallStatusChange(P_FLY_BT_INFO pFlyBtInfo,BYTE para)
{
	if (pFlyBtInfo->sCollexBTInfo.cPerMobileCallstatus != pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
	{
		pFlyBtInfo->sCollexBTInfo.cPerMobileCallstatus = pFlyBtInfo->sCollexBTInfo.cMobileCallStatus;
		//全局,音频切通道
		DBG0(debugString("\n select BT channel");)
		pFlyAllInOneInfo->pMemory_Share_Common->iBTCallStatus= para; 
		ipcStartEvent(EVENT_GLOBAL_BTCALLSTATUS_CHANGE_ID);
	}
}

static void control_WriteToCollex(P_FLY_BT_INFO pFlyBtInfo,BYTE *p,UINT len)
{
	pthread_mutex_lock(&pFlyBtInfo->pBtUartWriteMutex);
	if (pFlyBtInfo->flybt_fd > 0)
	{
		if (serial_write(pFlyBtInfo->flybt_fd, p, len) < 0)
		{
			DBG0(debugBuf("\nwrite to CollexBT fault!:", p,len);)
			pthread_mutex_unlock(&pFlyBtInfo->pBtUartWriteMutex);
			return;
		}
	}
	pthread_mutex_unlock(&pFlyBtInfo->pBtUartWriteMutex);
}

static void controlBTDeletePairedDevice(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[9];
	buff[0] = 'e';
	memset(&buff[1],0xFF,sizeof(BYTE)*6);
	buff[7] = '\r';buff[8] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,9);
}

static void controlBTAudioTrans(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3];
	buff[0] = 't';
	buff[1] = '\r';buff[2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,3);
}

static void controlBTPair(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3];
	buff[0] = 'p';
	buff[1] = '\r';buff[2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,3);
}

static void controlBTReqVersion(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3];
	buff[0] = 'i';
	buff[1] = '\r';buff[2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,3);
}

static void controlBTAnswerCall(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3];
	buff[0] = 'a';
	buff[1] = '\r';buff[2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,3);
}

static void controlBTHangCall(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3];
	buff[0] = 'h';
	buff[1] = '\r';buff[2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,3);
}

static void controlBTDailNumber(P_FLY_BT_INFO pFlyBtInfo,BYTE *p,UINT len)
{
	BYTE buff[64];
	buff[0] = 'd';
	memcpy(&buff[1],p,len);
	buff[len + 1] = '\r';buff[len + 2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,len + 3);
}

static void controlBTReqMobileName(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3];
	buff[0] = 'q';
	buff[1] = '\r';buff[2] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,3);
}

static void controlBTDTMFKey(P_FLY_BT_INFO pFlyBtInfo,BYTE key)
{
	BYTE buff[4];
	buff[0] = 'k';
	buff[1] = key;
	buff[2] = '\r';buff[3] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,4);
}

static void controlBTSelectPhoneBook(P_FLY_BT_INFO pFlyBtInfo,BYTE phoneBook)
{
	BYTE buff[5];
	buff[0] = 'b';
	buff[1] = '0';
	buff[2] = phoneBook;
	buff[3] = '\r';buff[4] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,5);
}

static void controlBTReqPhoneBookListOne(P_FLY_BT_INFO pFlyBtInfo,BYTE phoneBook)
{
	BYTE buff[5];
	buff[0] = 'b';
	buff[1] = '1';
	buff[2] = '1';
	buff[3] = '\r';buff[4] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,5);
}

static void controlBTReqPhoneBookListFive(P_FLY_BT_INFO pFlyBtInfo,BYTE phoneBook)
{
	BYTE buff[5];
	buff[0] = 'b';
	buff[1] = '2';
	buff[2] = '1';
	buff[3] = '\r';buff[4] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,5);
}

static void controlBTReqPhoneBookListAll(P_FLY_BT_INFO pFlyBtInfo,BYTE phoneBook)
{
	BYTE buff[5];
	buff[0] = 'b';
	buff[1] = '3';
	buff[2] = '0';
	buff[3] = '\r';buff[4] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,5);
}

static void controlBTVolumeControl(P_FLY_BT_INFO pFlyBtInfo,BYTE key)
{
	BYTE buff[4];
	buff[0] = 's';
	buff[1] = key;
	buff[2] = '\r';buff[3] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,4);
}

static void controlConnectA2DP(P_FLY_BT_INFO pFlyBtInfo,BOOL bConnect)
{
	BYTE buff[4]={'x','0','\r','\n'};
	if (bConnect)
	{
		buff[1] = '1';
	}
	
	control_WriteToCollex(pFlyBtInfo,buff,4);
}

static void controlCurrentPhoneBookType(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[5];
	buff[0] = 'b';
	buff[1] = '0'; buff[2] = '0';
	buff[3] = '\r';buff[4] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,5);
}
static void controlConnectActiveMobilePhoneStatus(P_FLY_BT_INFO pFlyBtInfo,BOOL bConnect)
{
	BYTE buff[4];
	buff[0] = 'c';
	if (bConnect)
	{
		buff[1] = '1';
	}
	else
	{
		buff[1] = '0'; 
	}
	buff[2] = '\r';buff[3] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,4);
}
static void controlWaitingCallType(P_FLY_BT_INFO pFlyBtInfo,BYTE type)
{
	BYTE buff[4];
	buff[0] = 'w';
	buff[1] = type+'0';
	buff[2] = '\r';buff[3] = '\n';
	control_WriteToCollex(pFlyBtInfo,buff,4);
}

static void controlBTUseInnerMic(P_FLY_BT_INFO pFlyBtInfo,BOOL bInner)
{
	BYTE buff[5] = {'j','a','1','\r','\n'};

	if (bInner)
	{
		buff[2] = '2';
	}

	control_WriteToCollex(pFlyBtInfo,buff,5);
}

static void controlBTWarmReset(P_FLY_BT_INFO pFlyBtInfo)
{
	BYTE buff[3] = {'r','\r','\n'};

	control_WriteToCollex(pFlyBtInfo,buff,3);
}


static void powerNormalDeInit(P_FLY_BT_INFO pFlyBtInfo)
{
	collexBTInfoInit(pFlyBtInfo,TRUE);
	pFlyBtInfo->bPower = FALSE;
	pFlyBtInfo->bPowerUp = FALSE;
	collex_PowerControl_Off(pFlyBtInfo);
	collex_ChipEnableControl_Normal(pFlyBtInfo);
	collex_ResetControl_On(pFlyBtInfo);
}

static void controlBTPowerControl(P_FLY_BT_INFO pFlyBtInfo,BOOL bPowerOn)
{
	if (bPowerOn)
	{
		collexBTInfoInit(pFlyBtInfo,FALSE);

		//controlMuteIn(pFlyBtInfo,BT_MUTE_X);//嘉实蓝牙特有，缺省当成无输出

		collex_PowerControl_On(pFlyBtInfo);//打开蓝牙电源
		pFlyBtInfo->bPower = TRUE;

		pFlyBtInfo->sCollexBTInfo.cMobileCallStatus = 'H';
		returnMobileDialStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.cMobileCallStatus);
	}
	else
	{
		powerNormalDeInit(pFlyBtInfo);
		collexBTInfoInit(pFlyBtInfo,TRUE);
	}
}

static void controlA2DPButtom(P_FLY_BT_INFO pFlyBtInfo,BYTE iButtom,BYTE bPressed)
{
	BYTE buff[5] = {'n','1','1','\r','\n'};

	buff[1] = iButtom+'0';
	buff[2] = bPressed+'0';
	control_WriteToCollex(pFlyBtInfo,buff,5);
	DBG3(debugBuf("\nA2DP:", buff,5);)
}

void controlMuteInit(P_FLY_BT_INFO pFlyBtInfo)
{
	pthread_mutex_init(&pFlyBtInfo->controlAudioMuteMutex, NULL);
}

void controlMuteIn(P_FLY_BT_INFO pFlyBtInfo,BYTE iWhat)
{
	UINT TimeoutCount;

	if (pFlyAllInOneInfo->pMemory_Share_Common->bAudioMuteControlable)
	{
		pthread_mutex_lock(&pFlyBtInfo->controlAudioMuteMutex);

		pFlyBtInfo->iControlAudioMuteWhat |= iWhat;

		debugOneData("\nJS BT Mute In Mask",pFlyBtInfo->iControlAudioMuteWhat);

		if (pFlyBtInfo->iControlAudioMuteTime)
		{
			pthread_mutex_unlock(&pFlyBtInfo->controlAudioMuteMutex);
			return;
		}

		debugString("\nJS BT Mute In Start");

		pFlyAllInOneInfo->pMemory_Share_Common->ipcbMuteBT = TRUE;
		ipcStartEvent(EVENT_GLOBAL_BT_MUTE_REQ_ID);//发送进入静音

		TimeoutCount = 0;
		while (!ipcWhatEventOn(EVENT_GLOBAL_BT_MUTE_IN_OK_ID))//等待OK
		{
			TimeoutCount++;
			if (TimeoutCount> 30)
			{
				break;
			}
			Sleep(50);
		}
		ipcClearEvent(EVENT_GLOBAL_BT_MUTE_IN_OK_ID);//清除

		pFlyBtInfo->iControlAudioMuteTime = GetTickCount();

		pthread_mutex_unlock(&pFlyBtInfo->controlAudioMuteMutex);
	}
}

void controlMuteOut(P_FLY_BT_INFO pFlyBtInfo,BYTE iWhat)
{
	if (pFlyAllInOneInfo->pMemory_Share_Common->bAudioMuteControlable)
	{
		pthread_mutex_lock(&pFlyBtInfo->controlAudioMuteMutex);

		pFlyBtInfo->iControlAudioMuteWhat &= ~iWhat;

		debugOneData("\nJS BT Mute Out Mask",pFlyBtInfo->iControlAudioMuteWhat);

		if (pFlyBtInfo->iControlAudioMuteWhat)
		{
			pthread_mutex_unlock(&pFlyBtInfo->controlAudioMuteMutex);
			return;
		}

		debugString("\nJS BT Mute In Stop");

		pFlyBtInfo->iControlAudioMuteTime = 0;
		pFlyAllInOneInfo->pMemory_Share_Common->ipcbMuteBT = FALSE;
		ipcStartEvent(EVENT_GLOBAL_BT_MUTE_REQ_ID);//发送退出静音

		pthread_mutex_unlock(&pFlyBtInfo->controlAudioMuteMutex);
	}
}

static void DealBTInfo(P_FLY_BT_INFO pFlyBtInfo, BYTE *p, UINT32 len)
{
	UINT i;
	UINT16 ret = 0;
	BOOL bMsgHandle = TRUE;
	BOOL bRet = FALSE;
	
	if (len == 0)
		return;
		
	while (*p == 0x0A || *p == 0x0D || *p == 0x00)
	{
		p++;
		len -= 1;
		if (len == 0)
			return;
	}
	
	
	DBG1(debugBuf("\n_*_BT-code:", p, len);)
	
	switch (p[0])
	{
	case 'E':
		DBG0(debugString("HAL BT rece code error\n");)
		break;
		
	case 'C':
		if (pFlyBtInfo->sCollexBTInfo.cMobileCallStatus != 'R')
		{
			pFlyBtInfo->sCollexBTInfo.bHangStatusCore = 0xFF;
			pFlyBtInfo->sCollexBTInfo.cMobileCallStatus = 'R';
			DBG0(debugBuf("\nC-1:message", &p[1],len-1);)
			UINT16 iDailInNumberLength = 0;
			while (iDailInNumberLength < len - 1 && p[iDailInNumberLength + 1] != '\r')
			{
				if (p[iDailInNumberLength + 1] == '+' && iDailInNumberLength)
				{
					DBG0(debugString("break\n");)
					break;
				}
				iDailInNumberLength++;
			}
			
			DBG0(debugBuf("\nC-2:message", &p[1],iDailInNumberLength);)
			returnMobileDailInNumber(pFlyBtInfo,&p[1],iDailInNumberLength);
		}
		break;
		
	case 'B':
		if ('0' == p[1])
		{
			pFlyBtInfo->sCollexBTInfo.iPhoneListType = p[2];
			pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount = 0;

			if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType == pFlyBtInfo->sCollexBTInfo.iPhoneListType)
			{
				controlBTReqPhoneBookListFive(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.iPhoneListType);
			}
		}
		else if ('1' == p[1])
		{
			//debugBuf("\nBT-HAL-b:", &p[2], len-2);
			ret = removeSpecialChar(pFlyBtInfo,&p[2], len-2);
			phoneListReplace(pFlyBtInfo,&p[2],ret,pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount);
			pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount++;
			if (pFlyBtInfo->sCollexBTInfo.iPhoneListType >= '0'
				&& pFlyBtInfo->sCollexBTInfo.iPhoneListType <= '6')
			{
				if (pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount > phoneListGetSelectCount(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.iPhoneListType))
				{
					pFlyBtInfo->sCollexBTInfo.iPhoneListMobileReturnCount = phoneListGetSelectCount(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.iPhoneListType);
				}
			}
			pFlyBtInfo->sCollexBTInfo.bPhoneListMobileReturn = TRUE;
		}
		else if ('2' == p[1])
		{
			if ('1' == p[2])
			{
				if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType == pFlyBtInfo->sCollexBTInfo.iPhoneListType)
				{
					if (pFlyBtInfo->sCollexBTInfo.bPhoneListMobileReturn)
					{
						pFlyBtInfo->sCollexBTInfo.bPhoneListMobileReturn = FALSE;
						controlBTReqPhoneBookListFive(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.iPhoneListType);
					}
					else
					{
						if (pFlyBtInfo->sCollexBTInfo.iPhoneListType >= '0'
							&& pFlyBtInfo->sCollexBTInfo.iPhoneListType <= '6')
						{
							pFlyBtInfo->sCollexBTInfo.bPhoneListPhoneReadFinish[pFlyBtInfo->sCollexBTInfo.iPhoneListType - '0'] = TRUE;
						}
					}
				}
			}
			else if ('3' == p[2] || '6' == p[2])
			{
				if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType == pFlyBtInfo->sCollexBTInfo.iPhoneListType)
				{
					pFlyBtInfo->sCollexBTInfo.iPhoneListType = 0xFF;//蓝牙返回无法获取电话本
					returnBTPhoneBookReadStatus(pFlyBtInfo,p[2] - '0');
				}
			}
		}

		if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType != pFlyBtInfo->sCollexBTInfo.iPhoneListType)
		{
			if ('2' == p[1])
			{
				if ('1' == p[2]
				|| '2' == p[2]
				|| '4' == p[2])
				{
					controlBTSelectPhoneBook(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType);
				}
			}
		}
		else
		{
			pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = TRUE;
			PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		}
		break;
		
	case 'I':
		if (memcmp(&pFlyBtInfo->sCollexBTInfo.sVersion[0],&p[1],8))
		{
			memcpy(&pFlyBtInfo->sCollexBTInfo.sVersion[0],&p[1],8);
			memcpy((void*)&pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersion[0],(void*)&p[1],8);
			pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersionLength = 8;
			ipcStartEvent(EVENT_GLOBAL_RETURN_BT_VERSION_ID);

			returnBTVersion(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sVersion,8);
		}
		break;
		
	case 'D':case 'A':/*case 'R':*/case 'H':
		if (pFlyBtInfo->sCollexBTInfo.cMobileCallStatus != p[0])
		{
			pFlyBtInfo->sCollexBTInfo.bHangStatusCore = 0xFF;
			pFlyBtInfo->sCollexBTInfo.cMobileCallStatus = p[0];
			returnMobileDialStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.cMobileCallStatus);
			PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		}
		break;
		
	case 'U':
		if (pFlyBtInfo->sCollexBTInfo.cAudioConnectionStatus != p[1])
		{
			pFlyBtInfo->sCollexBTInfo.cAudioConnectionStatus = p[1];	
			if (pFlyBtInfo->sCollexBTInfo.cAudioConnectionStatus == '0')//手机接听
			{
				//if (pFlyBtInfo->sCollexBTInfo.cMobileCallStatus != 'H')
					pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus = FALSE;
			} 
			else
			{
				pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus = TRUE;
			}
			
			pFlyBtInfo->sCollexBTInfo.iAudioConnectionStatusTime = GetTickCount();
			pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatusChange = TRUE;
			PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		}
#if COLLEX_AUDIO_TRANS_USE_COLLEX
		//returnMobileAudioTransfer(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus);//根据蓝牙返回同步界面
#endif			
		break;
		
	case 'Q':
		if((pFlyBtInfo->sCollexBTInfo.iDeviceNameLength != len - 3)
			||(memcmp(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],&p[1],len-3)))
		{
			//DBG0(debugOneData("\niDeviceNameLength-Q:",pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);)
			memset(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],0,COLLEX_PHONE_NAME);
			memcpy(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],&p[1],len-3);
			pFlyBtInfo->sCollexBTInfo.iDeviceNameLength = len - 3;

			pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
			returnMobilePairStatus(pFlyBtInfo,FALSE);//联通
			returnMobileName(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sDeviceName,pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);
		}
		break;
		
	case 'P':
		pFlyBtInfo->sCollexBTInfo.iPairedStatus = p[1];
		if ('R' == p[1])
		{
			returnMobilePairStatus(pFlyBtInfo,TRUE);
		}
		else if ('0' == p[1])//配对失败
		{
			pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
		}
		PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		//其它暂不处理
		break;
		
	case 'X':
		if (p[1] == '3')
		{
			if (p[2] == '0')
			{
				DBG0(debugString("\r\nCollexBT Control Enter Mute");)
				
				controlMuteIn(pFlyBtInfo,BT_MUTE_X);
			}
			else if (p[2] == '1')
			{
				DBG0(debugString("\r\nCollexBT Control Exit Mute");)
				
				controlMuteOut(pFlyBtInfo,BT_MUTE_X);
			}
		}
		else if (p[1] == '0')
		{
			pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection = FALSE;
		}
		else
		{
			pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection = TRUE;
		}
		returnBTA2DPbConnect(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection);
		break;
		
	case 'Y':
		pFlyBtInfo->sCollexBTInfo.iPairedDeviceType = p[1];
		memcpy(&pFlyBtInfo->sCollexBTInfo.BDAddress[0],&p[2],6);
		pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
		if ('1' == p[1] || '2' == p[1])
		{
			if (len > (2+6+2))
			{
				memset(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],0,COLLEX_PHONE_NAME);
				memcpy(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],&p[8],len-(2+6+2));
				pFlyBtInfo->sCollexBTInfo.iDeviceNameLength = len - (2+6+2);

				DBG0(debugOneData("iDeviceNameLength-Y:",pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);)
				returnMobilePairStatus(pFlyBtInfo,FALSE);
				returnMobileName(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sDeviceName,pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);
			}
		}
		break;
		
	case 'S':
		if (p[2] == '0')
		{
			pFlyBtInfo->sCollexBTInfo.bPaired = FALSE;
		} 
		else
		{
			pFlyBtInfo->sCollexBTInfo.bPaired = TRUE;

			if (p[3] == '1')//配对后才有有效连接
			{
				pFlyBtInfo->sCollexBTInfo.bConnected = TRUE;
				pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
				pFlyBtInfo->sCollexBTInfo.bBtPowerOnNeedTOReturnPhone = TRUE;
			}
		}

		if (p[3] == '0')
		{
			pFlyBtInfo->sCollexBTInfo.bConnected = FALSE;

			memset(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],0,COLLEX_PHONE_NAME);
			pFlyBtInfo->sCollexBTInfo.iDeviceNameLength = 0;
			collexPhoneListInit(pFlyBtInfo);
		} 

		returnMobileLinkStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bConnected);
		break;
		
	case 'J':
		if (p[1] == '0')
		{
			pFlyBtInfo->sCollexBTInfo.mobileBattery = p[2] - '0';
			returnMobileBattery(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.mobileBattery);
		}
		else if (p[1] == '1')
		{
			pFlyBtInfo->sCollexBTInfo.mobileSignal = p[2] - '0';
			returnMobileSignalStrength(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.mobileSignal);
		}
		else if (p[1] == '2')
		{
			pFlyBtInfo->sCollexBTInfo.mobileVolume = (p[2] - '0') * 10 + (p[3] - '0');
		}
		break;

	case 'W':
		if ('A' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus 
			&& 0 == pFlyBtInfo->sCollexBTInfo.iWaitingCallLen)
		{
			memset(&pFlyBtInfo->sCollexBTInfo.sWaitingCallNumber[0],0,COLLEX_PHONE_NAME);
			memcpy(&pFlyBtInfo->sCollexBTInfo.sWaitingCallNumber[0],
				&p[1],len-3);
			pFlyBtInfo->sCollexBTInfo.iWaitingCallLen = len - 3;
			returnWaitingCallNumber(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sWaitingCallNumber,pFlyBtInfo->sCollexBTInfo.iWaitingCallLen);
		}
		break;

	default:
		bMsgHandle = FALSE;
		bRet = TRUE;
		DBG1(debugTwoData("\n unHandle-->", p[0], p[1]);)
		break;
	}

	if (bMsgHandle && !pFlyBtInfo->sCollexBTInfo.bWork)//蓝牙工作
	{
		pFlyBtInfo->sCollexBTInfo.bWork = TRUE;
		returnBTWorkStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bWork);
	}
	
	//一定时间没有返回，蓝牙自动复位
	if (FALSE == bRet)
	{
		pFlyBtInfo->iAutoResetControlTime = GetTickCount();
	}

}

static BYTE *findOneDataStream(P_FLY_BT_INFO pFlyBtInfo,BYTE *search_sync_ptr, BYTE *search_end_ptr, BYTE *sync_status)
{
	UINT16 i = 0;
	UINT16 len = 0;
	*sync_status = SYNC_MISSING;
	
	for (i=0; (search_sync_ptr + i) < search_end_ptr; i++)
	{
		if (*(search_sync_ptr + i) == 0x0D)
		{
			if (search_end_ptr - (search_sync_ptr + i) > 0)
			{
				if (*(search_sync_ptr + 1 + i) == 0x0A)
				{	
					len = i+2;
					*sync_status = SYNC_FOUND;
					DealBTInfo(pFlyBtInfo,search_sync_ptr, len);
					break;
				}
				else
				{
					i++;
				}
			}
			else
			{
				*sync_status = SYNC_PARTIAL;
				break;
			}
		}
	}

	return (search_sync_ptr+len);
}

BYTE *searchDataStream(P_FLY_BT_INFO pFlyBtInfo,BYTE *search_sync_ptr, BYTE *search_end_ptr)
{
	BYTE sync_status;
	
	while (search_sync_ptr < search_end_ptr)
	{
		search_sync_ptr = findOneDataStream(pFlyBtInfo,search_sync_ptr, search_end_ptr, &sync_status);
		if (sync_status != SYNC_FOUND)
		{
			break;
		}
	}
	
	return search_sync_ptr;
}

void moveDataForNext(P_FLY_BT_INFO pFlyBtInfo,BYTE *start_buffer_ptr, BYTE *start_data_ptr, UINT16 buffer_num)
{
	UINT16 i = 0;
	
	for (i=0; i<buffer_num; i++)
	{
		*start_buffer_ptr = *start_data_ptr;
		start_buffer_ptr++;
		start_data_ptr++;
	}
}

void dealDataStream(P_FLY_BT_INFO pFlyBtInfo,BYTE *search_sync_ptr, BYTE *search_end_ptr)
{
	UINT16 i = 0;
	search_sync_ptr = searchDataStream(pFlyBtInfo,search_sync_ptr, search_end_ptr);
	if (search_sync_ptr < search_end_ptr)
	{
		pFlyBtInfo->BTInfoReadBuffLength = (UINT16)(search_end_ptr - search_sync_ptr);
		if (pFlyBtInfo->BTInfoReadBuffLength >= SERIAL_BUF_MAX_LEN)
		{
			//一直没有发0X0D,0X0A，把数据全扔掉
			pFlyBtInfo->BTInfoReadBuffLength = 0;
		}
		else
		{
			moveDataForNext(pFlyBtInfo,&pFlyBtInfo->BTInfoReadBuff[0],search_sync_ptr, pFlyBtInfo->BTInfoReadBuffLength);
		}
	}
	else
	{
		pFlyBtInfo->BTInfoReadBuffLength = 0;
	}
}


void *readSerialThread(void *arg)
{
	long ret=-1;
	BYTE *search_sync_ptr;
	BYTE *search_end_ptr;
	P_FLY_BT_INFO pFlyBtInfo = (P_FLY_BT_INFO )arg;
	
	pFlyBtInfo->BTInfoReadBuffLength = 0;

	while (!pFlyBtInfo->bKillReadSerialThread)
	{
		if (pFlyBtInfo->flybt_fd > 0)
		{
			ret = serial_read(pFlyBtInfo->flybt_fd, 
				&pFlyBtInfo->BTInfoReadBuff[pFlyBtInfo->BTInfoReadBuffLength], SERIAL_BUF_MAX_LEN-pFlyBtInfo->BTInfoReadBuffLength);
			if (ret > 0)
			{	
				pFlyBtInfo->BTInfoReadBuffLength += (UINT16)ret;
				//DBG0(debugChar("\nBT-uart recv data:", 
				//pFlyBtInfo->BTInfoReadBuff,pFlyBtInfo->BTInfoReadBuffLength);)
				//DBG0(debugBuf("\n__U-uart recv data:", 
				//pFlyBtInfo->BTInfoReadBuff,pFlyBtInfo->BTInfoReadBuffLength);)
				
				if (pFlyBtInfo->BTInfoReadBuffLength >= SERIAL_BUF_MAX_LEN)
				{
					//一直没有发0X0D,0X0A，把数据全扔掉
					pFlyBtInfo->BTInfoReadBuffLength = 0;
				}
				else
				{
					search_sync_ptr = &pFlyBtInfo->BTInfoReadBuff[0];
					search_end_ptr  = &pFlyBtInfo->BTInfoReadBuff[pFlyBtInfo->BTInfoReadBuffLength];
					dealDataStream(pFlyBtInfo,search_sync_ptr,search_end_ptr);
				}
			}
			else
			{
				DBG1(debugOneData("BT-Uart Continue",ret);)
			}
		}
		else
		{
			Sleep(10);
		}
	}

	DBG0(debugString("\n read serial thread exit");)

	return NULL;
}

void *ThreadCollexBTMainProc(void *arg)
{
	struct timeval timenow;
	struct timespec timeout;
	P_FLY_BT_INFO pFlyBtInfo = (P_FLY_BT_INFO )arg;
	
	while (!pFlyBtInfo->bKillCollexBTMainThread)
	{
		
		if (pFlyBtInfo->sCollexBTInfo._W_bPairing || pFlyBtInfo->iControlAudioMuteTime)
		{
			WaitSignedTimeOut(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain,314);
		}
		else
		{
			WaitSignedTimeOut(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain,1000);
		}
		

		if (pFlyBtInfo->iBTSelfReturnHandDownTime
			&& GetTickCount() - pFlyBtInfo->iBTSelfReturnHandDownTime >= 618)
		{
			pFlyBtInfo->iBTSelfReturnHandDownTime = 0;
			if ('H' != pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
			{
				pFlyBtInfo->sCollexBTInfo.cMobileCallStatus = 'H';
				returnMobileDialStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.cMobileCallStatus);
			}
		}
		
		//DBG0(debugOneData("\nbConnected:",pFlyBtInfo->sCollexBTInfo.bConnected);)
		//DBG0(debugOneData("\niPairedStatus:",pFlyBtInfo->sCollexBTInfo.iPairedStatus);)
		
		//BT电源控制
		if (pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)
		{
			if (pFlyBtInfo->currentPower)
			{
				if (pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus)//切回手机
				{
					controlBTAudioTrans(pFlyBtInfo);
				}
				Sleep(100);
				
				pFlyBtInfo->currentPower = FALSE;
				controlBTPowerControl(pFlyBtInfo,FALSE);
			}
		}
		else if (pFlyBtInfo->controlPower)
		{
			if (!pFlyBtInfo->currentPower)
			{
				pFlyBtInfo->currentPower = TRUE;
				controlBTPowerControl(pFlyBtInfo,TRUE);
			}
		}
		else if (!pFlyBtInfo->controlPower)
		{
			if (pFlyBtInfo->currentPower)
			{							
				controlMuteIn(pFlyBtInfo,BT_MUTE_POWER);
			
				if (pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus)//切回手机
				{
					controlBTAudioTrans(pFlyBtInfo);
					Sleep(314);
				}

				pFlyAllInOneInfo->pMemory_Share_Common->iBTCallStatus= 0; 
				ipcStartEvent(EVENT_GLOBAL_BTCALLSTATUS_CHANGE_ID);
					
				pFlyBtInfo->currentPower = FALSE;
				controlBTPowerControl(pFlyBtInfo,FALSE);

				controlMuteOut(pFlyBtInfo,BT_MUTE_POWER);
			}
		}
		
		//BT复位
		if (pFlyBtInfo->bPower)
		{
			if (!pFlyBtInfo->bPowerUp && !pFlyBtInfo->bUpdater)
			{
				DBG0(debugString("\nCollex Reset");)
				
				collex_ChipEnableControl_Normal(pFlyBtInfo);
				collex_ResetControl_On(pFlyBtInfo);
				collex_PowerControl_Off(pFlyBtInfo);
				Sleep(100);
				collex_PowerControl_On(pFlyBtInfo);
				Sleep(100);
				collex_ResetControl_Off(pFlyBtInfo);
				Sleep(314);

				pFlyBtInfo->bPowerUp = TRUE;
			}
		}
		
		if (!pFlyBtInfo->bUpdater)
		{
			controlBTReqVersion(pFlyBtInfo);
			controlBTUseInnerMic(pFlyBtInfo,TRUE);
		}
		
		//蓝牙打开
		if (pFlyBtInfo->bPowerUp)
		{
			DBG2(debugString("\nBT Power have be On");)
			if (!pFlyBtInfo->sCollexBTInfo.bWork)//查询蓝牙工作状态
			{
				DBG2(debugString("\nBT serial comm have not data return\n");)
				//controlBTReqVersion(pFlyBtInfo);
				//controlBTUseInnerMic(pFlyBtInfo,TRUE);
			}
			else//控制蓝牙复位
			{
				if (!pFlyBtInfo->bUpdater)//蓝牙死机
				{
					if ((GetTickCount() - pFlyBtInfo->iAutoResetControlTime) >= 1000
						&& (GetTickCount() - pFlyBtInfo->iAutoResetControlTime) < 10*1000)
					{
						if (pFlyBtInfo->sCollexBTInfo.bConnected)
						{
							if (0 == pFlyBtInfo->sCollexBTInfo.iControlReqStep)
							{
								//DBG0(debugOneData("iControlReqStep-1:",pFlyBtInfo->sCollexBTInfo.iControlReqStep);)
								pFlyBtInfo->sCollexBTInfo.iControlReqStep = 1;
								//controlBTReqMobileName(pFlyBtInfo);//~~~~~~~~~
							}
							else if (1 == pFlyBtInfo->sCollexBTInfo.iControlReqStep)
							{
								//DBG0(debugOneData("iControlReqStep-2:",pFlyBtInfo->sCollexBTInfo.iControlReqStep);)
								pFlyBtInfo->sCollexBTInfo.iControlReqStep = 0;
								if (!pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection)
								{
									controlConnectA2DP(pFlyBtInfo,TRUE);
								}
							}
						}
						else
						{
							controlBTReqVersion(pFlyBtInfo);
						}
					}
					else if ((GetTickCount() - pFlyBtInfo->iAutoResetControlTime) > 10*1000)
					{
						DBG0(debugString("tick count-----\n");)
						collex_ResetControl_On(pFlyBtInfo);
						collex_PowerControl_Off(pFlyBtInfo);
						Sleep(100);
						collex_PowerControl_On(pFlyBtInfo);
						Sleep(100);
						collex_ResetControl_Off(pFlyBtInfo);
						controlBTPowerControl(pFlyBtInfo,FALSE);
						Sleep(314);
						controlBTPowerControl(pFlyBtInfo,TRUE);
					}
				}
			}
			
			
			if (pFlyBtInfo->sCollexBTInfo.iAudioConnectionStatusTime)
			{
				if (GetTickCount() - pFlyBtInfo->sCollexBTInfo.iAudioConnectionStatusTime > 415)
				{
					debugOneData("\ntime:",GetTickCount() - pFlyBtInfo->sCollexBTInfo.iAudioConnectionStatusTime);
					
					pFlyBtInfo->sCollexBTInfo.iAudioConnectionStatusTime = 0;
					/*
					if ('A' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
					{
						returnMobileAudioTransfer(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus);//根据蓝牙返回同步界面
					}
					*/
					if ('H' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)//这代码有问题?
					{
						if (FALSE == pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus)
						{
							controlBTAudioTrans(pFlyBtInfo);					
						}
					}		

					
					if (pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatusChange)
					{
						//if ('A' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
						{
							pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatusChange = FALSE;
							if (!pFlyBtInfo->sCollexBTInfo.bBTHangStatus && 
									'A' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
								returnMobileAudioTransfer(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus);//根据蓝牙返回同步界面
							pFlyBtInfo->sCollexBTInfo.bBTHangStatus = FALSE;
						}
					}
				}
			}
			

			if (pFlyBtInfo->sCollexBTInfo._W_bPairing)//iPairedStatus 0初始化 1发送了清空 2发送了配对
			{
				if (pFlyBtInfo->sCollexBTInfo.bConnected)
				{
					controlConnectActiveMobilePhoneStatus(pFlyBtInfo,FALSE);
				}
				else if (pFlyBtInfo->sCollexBTInfo.bPaired)
				{
					controlBTDeletePairedDevice(pFlyBtInfo);//删除之前的配对信息
				}
				else if (pFlyBtInfo->sCollexBTInfo.iPairedStatus == 0)
				{
					controlBTDeletePairedDevice(pFlyBtInfo);//删除之前的配对信息
					pFlyBtInfo->sCollexBTInfo.iPairedStatus = 1;
				}
				else if (pFlyBtInfo->sCollexBTInfo.iPairedStatus == '0')
				{
					pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
					//返回需要配对状态 
					returnMobilePairStatus(pFlyBtInfo,TRUE);
				}
				else
				{
					if (pFlyBtInfo->sCollexBTInfo.iPairedStatus != 'R'
						&& pFlyBtInfo->sCollexBTInfo.iPairedStatus != '1')//新蓝牙模块，某个手机的问题
					{
						if (pFlyBtInfo->sCollexBTInfo.iPairedStatus != 2)
						{
							controlBTPair(pFlyBtInfo);
							pFlyBtInfo->sCollexBTInfo.iPairedStatus = 2;//表示已发送
						}
					}
				}
			}
			if (pFlyBtInfo->sCollexBTInfo.bConnected)
			{
				if (pFlyBtInfo->sCollexBTInfo.bBtPowerOnNeedTOReturnPhone){
					pFlyBtInfo->sCollexBTInfo._W_iPhoneListType = '2';
					pFlyBtInfo->sCollexBTInfo.bBtPowerOnNeedTOReturnPhone = FALSE;
					controlBTReqPhoneBookListAll(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType);
					returnCurrentPhoneBookType(pFlyBtInfo,0x02);
				}
				
				
				if (0 == pFlyBtInfo->sCollexBTInfo.iDeviceNameLength)
				{
					//DBG0(debugOneData("===iDeviceNameLength:",pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);)
					controlBTReqMobileName(pFlyBtInfo);
				}
				else if (pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn)//需要返回电话本
				{
					DBG3(debugString("\nThreadCollexBTMainProc need return phonebook");)
						//if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType == pFlyBtInfo->sCollexBTInfo.iPhoneListType)
						if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType >= '1'
							&& pFlyBtInfo->sCollexBTInfo._W_iPhoneListType <= '6')//返回某个电话本
						{
								while (pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent != pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount)//没有返回足够数量
								{
									DBG3(debugString("\nneedreturnmore");)
									if (0 == pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent)//每页开始返回时
									{
										UINT iSelectCount = phoneListGetSelectCount(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType);
										if (pFlyBtInfo->sCollexBTInfo.iPhoneListStart >= iSelectCount)//判断是否超过
										{
											if (pFlyBtInfo->sCollexBTInfo.iPhoneListStart)//跳过全都没有的情况
											{
												pFlyBtInfo->sCollexBTInfo.iPhoneListStart = iSelectCount - 1;
											}
										}
									}
									P_COLLEX_BT_PHONE_LIST p;
									p = phoneListGetSelectOne(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType,pFlyBtInfo->sCollexBTInfo.iPhoneListStart+pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent);
									if (p)
									{
										DBG3(debugOneData("\nindex:%d",pFlyBtInfo->sCollexBTInfo.iPhoneListStart+pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent);)
											if (0 == pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent)//返回开始前刷新页面
											{
												returnBTPhoneBookPageFlush(pFlyBtInfo);
											}
											returnMobilePhoneList(pFlyBtInfo,p);
											pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent++;
									}
									else
									{
										break;
									}
								}
						}
						pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = FALSE;
						if (pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush)
						{

								if (pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent == pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount)
								{
									returnBTPhoneBookReadStatus(pFlyBtInfo,1);
									pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = FALSE;
								}
								else if (pFlyBtInfo->sCollexBTInfo.bPhoneListPhoneReadFinish[pFlyBtInfo->sCollexBTInfo._W_iPhoneListType - '0'])
								{
									if (pFlyBtInfo->sCollexBTInfo.iPhoneListStart+pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent
										>= phoneListGetSelectCount(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType))
									{
										returnBTPhoneBookReadStatus(pFlyBtInfo,1);
										pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = FALSE;
									}
								}
						}
				}
				else if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType != pFlyBtInfo->sCollexBTInfo.iPhoneListType)//电话本不匹配
				{
					if (0xFF != pFlyBtInfo->sCollexBTInfo.iPhoneListType)
					{
						controlBTSelectPhoneBook(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType);
					}
				}
			}
			else
			{
				
				//清空用户名
				//memset(&pFlyBtInfo->sCollexBTInfo.sDeviceName[0],0,COLLEX_PHONE_NAME);
				//pFlyBtInfo->sCollexBTInfo.iDeviceNameLength = 10;
				//returnMobileName(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.sDeviceName,pFlyBtInfo->sCollexBTInfo.iDeviceNameLength);
				
				
				if (pFlyBtInfo->sCollexBTInfo.bWork)//尝试连接
				{
					if (pFlyBtInfo->sCollexBTInfo.bPaired && 'R' != pFlyBtInfo->sCollexBTInfo.iPairedStatus)//不是正在配对状态
					{
						controlConnectActiveMobilePhoneStatus(pFlyBtInfo,TRUE);
					}
				}
			}
		}
		
		//if (pFlyBtInfo->iControlAudioMuteTime && GetTickCount() - pFlyBtInfo->iControlAudioMuteTime >= 5000)
		//{
		//	DBG2(debugString("\nCollexBT Auto Exit Mute");)
		//	controlMuteOut(pFlyBtInfo);
		//}
	}

	DBG0(debugString("\nThreadCollexBTMainProc exit");)

	return NULL;
}

 static int createThread(P_FLY_BT_INFO pFlyBtInfo)
 {
	INT ret = -1;
	INT res;
	pthread_t thread_id;
	
	pFlyBtInfo->bKillCollexBTMainThread = FALSE;
	res = pthread_create(&thread_id, NULL,ThreadCollexBTMainProc,pFlyBtInfo);
	DBG0(debugOneData("\nThreadCollexBTMainProc ID:",thread_id);)
	if(res != 0) 
	{
		pFlyBtInfo->bKillCollexBTMainThread = TRUE;
		return ret;
	}

	//读串口线程
	pFlyBtInfo->bKillReadSerialThread = FALSE;
	res = pthread_create(&thread_id, NULL,readSerialThread,pFlyBtInfo);
	DBG0(debugOneData("\nreadSerialThread ID:",thread_id);)
	if(res != 0) 
	{
		pFlyBtInfo->bKillReadSerialThread = TRUE;
		return ret;
	}
	
	pFlyBtInfo->bNeedInit = FALSE;
	return 0;
	
 }
 
static void collexBT_Enable(P_FLY_BT_INFO pFlyBtInfo,BOOL bEnable)
 {
	 if (bEnable)
	 {
		 DBG0(debugString("\nCollexBT_Enable Enable!");)

		 collex_PowerControl_Off(pFlyBtInfo);
		 collex_ChipEnableControl_Normal(pFlyBtInfo);
		 collex_ResetControl_On(pFlyBtInfo);
		 
		 pFlyBtInfo->bOpen = TRUE;
	 }
	 else
	 {
		if (pFlyBtInfo->sCollexBTInfo.bConnected)
		{
			//断开手机连接
			pFlyBtInfo->sCollexBTInfo.bConnected = FALSE;
			controlConnectActiveMobilePhoneStatus(pFlyBtInfo,FALSE);
		}
		
		PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		
		collex_PowerControl_Off(pFlyBtInfo);
	 }
 }
 
 
void *ThreadCollexBTLoadSchedule(void *arg)
{
	BYTE ret = 0, old = 0xFF;
	P_FLY_BT_INFO pFlyBtInfo = (P_FLY_BT_INFO )arg;
	
	while(ret != 100 && !pFlyBtInfo->bKillBTLoadThread)
	{
		ret = read_bt_dfu_schedule();
		if (ret != old)
		{
			old = ret;
			returnBtLoadSchedule(pFlyBtInfo,ret);
		}
	
		Sleep(1000);
	}
	return NULL;
}
 
static void controlPowerStatus(P_FLY_BT_INFO pFlyBtInfo, BOOL on)
{
	if (on)
	{
		collex_ChipEnableControl_Normal(pFlyBtInfo);
		collex_ResetControl_On(pFlyBtInfo);
		collex_PowerControl_Off(pFlyBtInfo);
		Sleep(100);
		collex_PowerControl_On(pFlyBtInfo);
		Sleep(100);
		collex_ResetControl_Off(pFlyBtInfo);
	}
	else
	{
		collex_PowerControl_Off(pFlyBtInfo);
		Sleep(100);
		collex_ResetControl_On(pFlyBtInfo);
	}
	Sleep(3000);
}
static BOOL serial_open_flag = FALSE;
static void loadUpdataForBt(P_FLY_BT_INFO pFlyBtInfo)
{
	int ret;
	INT res;
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	
	debugString("load data to bt start...\n");
	pFlyBtInfo->bUpdater = TRUE;
	
	if (pFlyBtInfo->flybt_fd > 0){
		serial_close(pFlyBtInfo->flybt_fd);
		pFlyBtInfo->flybt_fd = -1;
		serial_open_flag = TRUE;
	}
	
	
	pFlyBtInfo->bKillBTLoadThread = FALSE;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr,PTHREAD_CREATE_DETACHED);
	res = pthread_create(&thread_id, &thread_attr,ThreadCollexBTLoadSchedule,pFlyBtInfo);
	if(res != 0) 
	{
		pthread_attr_destroy(&thread_attr);
		pFlyBtInfo->bKillBTLoadThread = TRUE;
		debugString("create bt load thread err\n");
		return;
	}
	pthread_attr_destroy(&thread_attr);
	
	
	ret = set_bt_dfu();
	if (ret > 0)
	{
		debugOneData("load BT seuccess:\n", ret);
		returnBtLoadStatus(pFlyBtInfo,TRUE);
	}
	else
	{
		debugOneData("load BT error:\n",ret);
		returnBtLoadStatus(pFlyBtInfo,FALSE);
	}
	pFlyBtInfo->bKillBTLoadThread = TRUE;
	Sleep(100);
	
	pFlyBtInfo->controlPower = FALSE;
	returnBTPowerStatus(pFlyBtInfo,FALSE);
	
	//controlPowerStatus(pFlyBtInfo, FALSE);
			/*
	if (serial_open_flag)
	{
		pFlyBtInfo->flybt_fd = serial_open();
	}
	*/
	pFlyBtInfo->bUpdater = FALSE;
	PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
}

 static void onRightDataProcessor(P_FLY_BT_INFO pFlyBtInfo,BYTE *buf, UINT16 len)
 {
	debugBuf("\nBT-data:", buf, len);
	
	 switch (buf[0])
	 {
	 case 0x01:
		 if (0x01 == buf[1])
		{
			if (pFlyBtInfo->bNeedInit)
			{
				pFlyBtInfo->controlPower = FALSE;
				pFlyBtInfo->currentPower = FALSE;
				
				createThread(pFlyBtInfo);
				collexBT_Enable(pFlyBtInfo, TRUE);
			}
		
			returnMobileStatusInit(pFlyBtInfo);
			//collexBTInfoInit(pFlyBtInfo,FALSE);
			
			returnMobilePowerMode(pFlyBtInfo,TRUE);//电源开
			returnMobileWorkMode(pFlyBtInfo,TRUE);//初始化正常
		}
		 else if (0x00 == buf[1])
		{
			pFlyBtInfo->controlPower = FALSE;
			pFlyBtInfo->currentPower = FALSE;
			collexBTInfoInit(pFlyBtInfo,TRUE);
			returnMobileWorkMode(pFlyBtInfo,FALSE);//初始化中
			returnMobilePowerMode(pFlyBtInfo,FALSE); //电源关
		}
		 break;
	 case 0x03:
		 if (buf[1] == 0x00)
		{

		}
		 else 
		{
			if ('H' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
			{
				if (0x01 == buf[1])
				{
					controlBTVolumeControl(pFlyBtInfo,'2');
				} 
				else if (0x02 == buf[1])
				{
					controlBTVolumeControl(pFlyBtInfo,'3');
				}
			}
			else
			{
				if (0x01 == buf[1])
				{
					controlBTVolumeControl(pFlyBtInfo,'0');
				} 
				else if (0x02 == buf[1])
				{
					controlBTVolumeControl(pFlyBtInfo,'1');
				}
			}
		 }	 
		 break;
	
	case 0x04:
		if (buf[2] == 0x01)
		{
			controlMuteIn(pFlyBtInfo,BT_MUTE_BOTTOM);

			controlA2DPButtom(pFlyBtInfo,buf[1],buf[2]);
		}
		else
		{
			controlA2DPButtom(pFlyBtInfo,buf[1],buf[2]);
			
			controlMuteOut(pFlyBtInfo,BT_MUTE_BOTTOM);
		}
		break;
			
	case 0x10:
		if (0x01 == buf[1])
		{
			if (pFlyBtInfo->flybt_fd > 0)
			{
				pFlyBtInfo->controlPower = TRUE;
				returnBTPowerStatus(pFlyBtInfo,TRUE);
			}
			else
			{
				pFlyBtInfo->flybt_fd = serial_open();
				if(pFlyBtInfo->flybt_fd >0)
				{
					DBG0(debugString("\r\nFlyBT-com open OK\n");)
					pFlyBtInfo->controlPower = TRUE;
					returnBTPowerStatus(pFlyBtInfo,TRUE);
				}
			}

		}
		else
		{
			if (pFlyBtInfo->flybt_fd > 0)
			{
				serial_close(pFlyBtInfo->flybt_fd);
				pFlyBtInfo->flybt_fd = -1;
				DBG0(debugString("\r\nFlyBT-com close OK\n");)
			}

			pFlyBtInfo->controlPower = FALSE;
			returnBTPowerStatus(pFlyBtInfo,FALSE);
			
			//pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection = FALSE;
			//returnBTA2DPbConnect(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.bStereoDeviceConnection);
		}
		PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		break;


	 case 0x20:
		if (pFlyBtInfo->sCollexBTInfo.bWork)
		{
			if (1 == buf[1])//启动配对 
			{
				pFlyBtInfo->sCollexBTInfo.iPairedStatus = 0;
				pFlyBtInfo->sCollexBTInfo._W_bPairing = TRUE;
				returnMobilePairStatus(pFlyBtInfo,TRUE);
			}
			else
			{
				pFlyBtInfo->sCollexBTInfo._W_bPairing = FALSE;
				if ('R' == pFlyBtInfo->sCollexBTInfo.iPairedStatus)
				{
					controlBTPair(pFlyBtInfo);
				}
				returnMobilePairStatus(pFlyBtInfo,FALSE);
			}
			PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		}
		DBG3(debugOneData("\nFlyAudio WinCE Pair ",buf[1]);)
		 break;

	 case 0x21:
		 if ('A' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
			{
				if (0x01 == buf[1])//手机
				{
					if (pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus)
					{
						//flyAudioReturnToUser(pFlyBtInfo,buf,len);
						
						controlMuteIn(pFlyBtInfo,BT_MUTE_AUDIO_CHANGE);

						controlBTAudioTrans(pFlyBtInfo);				

						controlMuteOut(pFlyBtInfo,BT_MUTE_AUDIO_CHANGE);
					}
					
				} 
				else if (0x00 == buf[1])//模块
				{
					if (!pFlyBtInfo->sCollexBTInfo.bAudioConnectionStatus)
					{
						//flyAudioReturnToUser(pFlyBtInfo,buf,len);
						
						controlMuteIn(pFlyBtInfo,BT_MUTE_AUDIO_CHANGE);

						controlBTAudioTrans(pFlyBtInfo);					
	
						controlMuteOut(pFlyBtInfo,BT_MUTE_AUDIO_CHANGE);
					}
				}	
			}
		 break;

	 case 0x22:
		 if (0x01 == buf[1])
		{
			if (!pFlyBtInfo->sCollexBTInfo.bConnected)
			{	
				controlConnectActiveMobilePhoneStatus(pFlyBtInfo,TRUE);
			}
		}
		 else
		{
			if (pFlyBtInfo->sCollexBTInfo.bConnected)
			{	
				controlConnectActiveMobilePhoneStatus(pFlyBtInfo,FALSE);
			}
		}
		 break;

	 case 0x23:
		if (pFlyBtInfo->sCollexBTInfo.bHangStatusCore != buf[1])
		{
			pFlyBtInfo->sCollexBTInfo.bHangStatusCore = buf[1];
			if (0x02 == buf[1])//呼出电话
			{
				if (pFlyBtInfo->sCollexBTInfo.bConnected)
				{
					if ('H' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
					{
						controlBTDailNumber(pFlyBtInfo,&buf[2],len - 2);
					}
					else
					{
						DBG3(debugString("\ndail but not in H mode");)
					}
				}
				else
				{
					DBG3(debugString("\ndail but not connect");)
				}
			}
			 else if (0x00 == buf[1])
			{
				if ('H' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
				{
					returnMobileDialStatus(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo.cMobileCallStatus);
				}
				controlBTHangCall(pFlyBtInfo);
				pFlyBtInfo->sCollexBTInfo.bBTHangStatus = TRUE;
			}
			 else if (0x01 == buf[1])
			{
				controlBTAnswerCall(pFlyBtInfo);
			}
			
			PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		}
		 break;
	 case 0x24:
		 if (pFlyBtInfo->sCollexBTInfo.bConnected)
			{
				if ('A' == pFlyBtInfo->sCollexBTInfo.cMobileCallStatus)
				{
					controlBTDTMFKey(pFlyBtInfo,buf[1]);
				}
			}
		 break;
	 case 0x25:
		 if (pFlyBtInfo->sCollexBTInfo.bConnected)
			{
				controlCurrentPhoneBookType(pFlyBtInfo);
			}
		 break;

	 case 0x26:
		 if (buf[1] <= 0x06)
			{
				if (pFlyBtInfo->sCollexBTInfo._W_iPhoneListType != (buf[1] + '0'))
				{
					pFlyBtInfo->sCollexBTInfo._W_iPhoneListType = buf[1] + '0';
					pFlyBtInfo->sCollexBTInfo.iPhoneListStart = 0;
					pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount = buf[2];
					pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;

					controlBTSelectPhoneBook(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType);

					pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = TRUE;
					pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = TRUE;
					PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
				}
			}
		 break;
	 case 0x27:
		 if (0x01 == buf[1])
		{
				if (pFlyBtInfo->sCollexBTInfo.iPhoneListStart + pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount
					< phoneListGetSelectCount(pFlyBtInfo,pFlyBtInfo->sCollexBTInfo._W_iPhoneListType))
				{
					pFlyBtInfo->sCollexBTInfo.iPhoneListStart += pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount;
					pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;
					pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = TRUE;
					pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = TRUE;
				}
		 } 
		 else if (0x00 == buf[1])
			{
				if (pFlyBtInfo->sCollexBTInfo.iPhoneListStart > pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount)
				{
					pFlyBtInfo->sCollexBTInfo.iPhoneListStart -= pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCount;
					pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;
					pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = TRUE;
					pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = TRUE;
				}
				else if (pFlyBtInfo->sCollexBTInfo.iPhoneListStart)
				{
					pFlyBtInfo->sCollexBTInfo.iPhoneListStart = 0;
					pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;
					pFlyBtInfo->sCollexBTInfo.bPhoneListNeedReturnFlush = TRUE;
					pFlyBtInfo->sCollexBTInfo.bPhoneListStartReturn = TRUE;
				}
				else
				{
					pFlyBtInfo->sCollexBTInfo.iPhoneListStart = 0;
					pFlyBtInfo->sCollexBTInfo.iPhoneListReturnCurrent = 0;
				}
			}
			PostSignal(&pFlyBtInfo->CollexBTMutex,&pFlyBtInfo->CollexBTCond,&pFlyBtInfo->bCollexBTThreadRunAgain);
		 
		 break;

		 //case 0x40:
		 //	if (0<=buf[1] && buf[1]<4)
		 //	{
		 //		pFlyBtInfo->sCollexBTInfo.iWaitingCallType = buf[1];
		 //		controlWaitingCallType(pFlyBtInfo, pFlyBtInfo->sCollexBTInfo.iWaitingCallType);
		 //	}
		 //	break;
	 case 0x80:
		if (0x00 == buf[1])
		{
			controlBTDeletePairedDevice(pFlyBtInfo);
			Sleep(100);
			controlPowerStatus(pFlyBtInfo,TRUE);
			loadUpdataForBt(pFlyBtInfo);
			controlPowerStatus(pFlyBtInfo,FALSE);
		}
		break;
	 case 0xFF:
		 //if (0x01 == buf[1])
			//{
			//	FBT_PowerUp((DWORD)pFlyBtInfo);
		 //} 
		 //else if (0x00 == buf[1])
			//{
			//	FBT_PowerDown((DWORD)pFlyBtInfo);
			//}
		break;

	 default:
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
/*
	pFlyBtInfo->flybt_fd = serial_open();
	if(pFlyBtInfo->flybt_fd >0)
	{
		DBG0(debugString("\r\nFlyBT open OK\n");)
		ret = HAL_BT_RETURN_FD;
	}
*/	
	return HAL_BT_RETURN_FD;
}
 
 /********************************************************************************
 **函数名称：fly_init_device_struct（）函数
 **函数功能：初始化结构体里的成员
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyInitDeviceStruct(void)
 {
	UINT32 i,j;
	
	
 
	//为 flybt_struct_info 结构体分配内存
	pGlobalFlyBtInfo = (P_FLY_BT_INFO )malloc(sizeof(FLY_BT_INFO));
	if (pGlobalFlyBtInfo == NULL)
	{
		return;
	}
	memset(pGlobalFlyBtInfo, 0, sizeof(FLY_BT_INFO));
	
	pthread_mutex_init(&pGlobalFlyBtInfo->CollexBTMutex, NULL);
	pthread_cond_init(&pGlobalFlyBtInfo->CollexBTCond, NULL);
	pthread_mutex_init(&pGlobalFlyBtInfo->pBtUartWriteMutex, NULL);
	
	pGlobalFlyBtInfo->bNeedInit = TRUE;
	allInOneInit();
	
	controlMuteInit(pGlobalFlyBtInfo);

	DBG0(debugString("\nFlyBT hal init\n");)
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
	 DBG1(debugBuf("\nBT-HAL return  bytes Start:", buf,1);)
	 dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 DBG1(debugBuf("\nBT-HAL return  bytes to User:", buf,dwRead);)
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
	P_FLY_BT_INFO pFlyBtInfo = pGlobalFlyBtInfo;
	
	//等待串口读线程退出
	pFlyBtInfo->bKillReadSerialThread = TRUE;

	//释放一个条件变量
	pthread_cond_destroy(&pFlyBtInfo->CollexBTCond);
	pthread_mutex_destroy(&pFlyBtInfo->CollexBTMutex);
	pthread_mutex_destroy(&pFlyBtInfo->pBtUartWriteMutex);
	allInOneDeinit();
	free (pFlyBtInfo);
	pFlyBtInfo = pGlobalFlyBtInfo = NULL;
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	P_FLY_BT_INFO pFlyBtInfo = pGlobalFlyBtInfo;
	
	 if (pFlyBtInfo->flybt_fd > 0)
	 {
		 if (!serial_close(pFlyBtInfo->flybt_fd))
		 {
			 return 0;
		 }
	 }
	 
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
	DBG1(debugBuf("\nUser write bytes to Bt-HAL:", buf,len);)

	onRightDataProcessor(pGlobalFlyBtInfo,&buf[3], buf[2]-1);
}