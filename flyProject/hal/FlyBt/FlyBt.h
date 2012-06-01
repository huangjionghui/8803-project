#ifndef FLY_BT_H_
#define FLY_BT_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

#define BUF_MAX_LEN 256
#define COLLEX_AUDIO_TRANS_USE_COLLEX 0

#define COLLEX_PHONE_NUMBER	64
#define COLLEX_PHONE_NAME	64
typedef struct collex_bt_phone_list
{
	BYTE iWhichPhoneList;
	BYTE cPhoneNumber[COLLEX_PHONE_NUMBER];
	BYTE iPhoneType;
	BYTE cPhoneName[COLLEX_PHONE_NAME];
	UINT iPhoneNameLength;
	BYTE cPhoneNumberName[COLLEX_PHONE_NUMBER+COLLEX_PHONE_NAME];
	UINT iPhoneNumberNameLength;
	struct collex_bt_phone_list *Next;
}COLLEX_BT_PHONE_LIST, *P_COLLEX_BT_PHONE_LIST;

typedef struct collex_bt
{
	BOOL bWork;
	
	BOOL bPaired;
	BOOL bConnected;
	BYTE iControlReqStep;
	
	BYTE iPairedStatus;
	BOOL _W_bPairing;

	BYTE iPairedDeviceType;
	BYTE BDAddress[6];
	BYTE sVersion[8];

	BYTE sDeviceName[COLLEX_PHONE_NAME];
	UINT iDeviceNameLength;

	BYTE sWaitingCallNumber[COLLEX_PHONE_NAME];
	UINT iWaitingCallLen;
	UINT iWaitingCallType;
	BYTE sCallInPhoneNumber[COLLEX_PHONE_NUMBER];

	BYTE cMobileCallStatus;
	BYTE cPerMobileCallstatus;

	BOOL  bBTHangStatus;
	BYTE  bHangStatusCore;
	BOOL bAudioConnectionStatus;//用于切换通道
	ULONG iAudioConnectionStatusTime;
	BOOL bAudioConnectionStatusChange;
	BYTE cAudioConnectionStatus;

	BOOL bStereoDeviceConnection;

	BYTE mobileBattery;
	BYTE mobileSignal;
	BYTE mobileVolume;
	
	BOOL bBtPowerOnNeedTOReturnPhone;

	BYTE iPhoneListType;
	BYTE _W_iPhoneListType;
	BOOL bPhoneListMobileReturn;
	UINT iPhoneListMobileReturnCount;
	BOOL bPhoneListStartReturn;
	UINT iPhoneListStart;
	UINT iPhoneListReturnCount;
	UINT iPhoneListReturnCurrent;
	P_COLLEX_BT_PHONE_LIST pBTPhoneList;

	BOOL bPhoneListNeedReturnFlush;
	BOOL bPhoneListPhoneReadFinish[7];
}COLLEX_BT, *P_COLLEX_BT;

#define SERIAL_BUF_MAX_LEN 40960

typedef struct flybt_struct_info{

	BOOL bOpen;
	BOOL bPower;
	BOOL bPowerUp;
	BOOL bUpdater;
	BOOL bSpecialPower;
	INT  flybt_fd;
	BOOL bNeedInit;

	//读串口线程
	UINT16 BTInfoReadBuffLength;
	BYTE BTInfoReadBuff[SERIAL_BUF_MAX_LEN];
	BOOL bKillReadSerialThread;

	BOOL bKillBTLoadThread;
	
	//蓝牙主线程
	BOOL bKillCollexBTMainThread;
	pthread_mutex_t CollexBTMutex;
	pthread_cond_t  CollexBTCond; 
	pthread_mutex_t pBtUartWriteMutex;
	BOOL bCollexBTThreadRunAgain;

	//蓝牙电源控制
	BOOL controlPower;
	BOOL currentPower;

	ULONG iAutoResetControlTime;
	ULONG iBTSelfReturnHandDownTime;

	ULONG iControlAudioMuteTime;
	BYTE iControlAudioMuteWhat;
	pthread_mutex_t controlAudioMuteMutex;
	
	COLLEX_BT sCollexBTInfo;
}FLY_BT_INFO,*P_FLY_BT_INFO;

#define BT_MUTE_POWER			(1<<0)
#define BT_MUTE_BOTTOM			(1<<1)
#define BT_MUTE_AUDIO_CHANGE	(1<<2)
#define BT_MUTE_X				(1<<3)

#define SYNC_MISSING 0
#define SYNC_FOUND   1
#define SYNC_PARTIAL 2


extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);


#define DEBUG_MSG_ON 0

#if DEBUG_MSG_ON 
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(1){CODE}
#define DBG2(CODE) if(1){CODE}
#define DBG3(CODE) if(1){CODE}

//#define DBG1(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgAudioLevel>0){CODE}
//#define DBG2(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgAudioLevel>1){CODE}
//#define DBG3(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgAudioLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}

//#define DBG1(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgAudioLevel>0){CODE}
//#define DBG2(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgAudioLevel>1){CODE}
//#define DBG3(CODE) if(pFlyMmapInfo->Memory_Share_Common.debugMsgAudioLevel>2){CODE}
#endif

#endif