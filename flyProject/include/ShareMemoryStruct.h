#ifndef FLY_SHARE_MEMORY_STRUCT_H_
#define FLY_SHARE_MEMORY_STRUCT_H_


typedef unsigned char  BYTE;
typedef unsigned int   UINT;
typedef unsigned short WORD;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef unsigned long  DWORD;
typedef unsigned long  ULONG;

typedef int   BOOL;
typedef int   INT;
typedef float FLOAT;


#ifndef FALSE
#define FALSE   0
#endif 

#ifndef TRUE
#define TRUE   1
#endif 

#ifndef DISANBLE
#define DISANBLE   0
#endif 

#ifndef ENABLE
#define ENABLE   1
#endif 

#define SHARE_MAP_SIZE	4096*4

#define STRUCT_OFFSET(stru_name, element) (unsigned long)&((struct stru_name*)0)->element

#define CONSOLE_DEBUG	1

#define SEND_ACC_OFF_TO_USER	1

/***************************************************************************************************************
���³��汾ʱ����
***************************************************************************************************************/
//�ڴ�����ʹ�ú�ķ����ǣ�����ʹ����������������еġ��磺PCB_8803_DISP_V1��PCB_8803_AMP_V2��ֻ��Ϊһ�������Ŵ���
#include "board_def.h"

/***************************************************************************************************************
���ϳ��汾ʱ����
***************************************************************************************************************/

enum eLedFlashError{eLEDNormal=0,eLEDPreSleepTimeOut,eLEDEnterSleepTimeOut,eLEDControlMCUSleepTimeOut};
enum audio_source{Init=0,MediaCD,CDC,RADIO
				,AUX,IPOD,TV,MediaMP3
				,SRADIO,A2DP,EXT_TEL,GR_AUDIO
				,BACK,FRONT_CAMERA,BT,GPS
				,BT_RING};

#define EVENT_TRANS_LIST_MAX	256
#define DEBUG_BUFF_LINE_LENGTH	256

#define COLOR_STEP_COUNT	11	//��Ƶ���ڵȼ�
#define RADIO_COUNTRY_ID	6	//��������������
#define IRKEYTABSIZE	0xFF	//�����̴�С
#define IRKEY_STUDY_COUNT	32

#define MCU_SOFT_VERSION_MAX	64

#define MODULE_SOFT_VERSION_MAX	64

#define SCAR_MODEL_MAX 64   //���ͳ���
#define ID_FOR_FILE_STORE	0x95270001

//�����ڴ��õ�ID��
enum enumShareMemoryID{
	SHARE_MEMORY_COMMON = 0,
	SHARE_MEMORY_GLOBAL,
	SHARE_MEMORY_DVD,
	SHARE_MEMORY_BT,
	SHARE_MEMORY_RADIO,
	SHARE_MEMORY_AUDIO,
	SHARE_MEMORY_VIDEO,
	SHARE_MEMORY_KEY,
	SHARE_MEMORY_SYSTEM,
	SHARE_MEMORY_EXDISPLAY,
	SHARE_MEMORY_XMRADIO,
	SHARE_MEMORY_TPMS,
	SHARE_MEMORY_TV,
	SHARE_MEMORY_AC,

	SHARE_MEMORY_MAX,
};

typedef struct _tPANEL_TAB{
	BYTE valid[2];  //�Ƿ��Ѿ���ʼ��  55h AAh
	BYTE type;	 //���汾��0x01 �������� 3��7����
	BYTE PanelName[8];//�������  �硰AV6000XX��
	BYTE slid;	//01  ����	    00 �̶�
	BYTE EK_Value[4];   //���α�ʾ 0x80~0x83��ֵ��Ӧ��ϵ
	BYTE KeyValue[30]; //��Ӧ����е�1��21��ֵ��Ӧ����ֵ ,Ԥ�����ֿռ�9BYTESΪ�Ժ�����
	UINT16 LCD_RGB_Wide;	   
	UINT16 LCD_RGB_High;	   
}tPANEL_TAB;

typedef struct _IRKEY_STUDY_TAB{
	BYTE Value[IRKEYTABSIZE];
	BYTE Port[IRKEYTABSIZE];
	UINT AD[IRKEYTABSIZE];
}IRKEY_STUDY_TAB, *P_IRKEY_STUDY_TAB;

typedef struct _FLY_SILENCE_POWEROFF_INFO
{
	BOOL bNextTimeToPowerOff;
	time_t timeFirstPowerOn;
	time_t timeLastUserAccOff;//���һ�������ػ�
}FLY_SILENCE_POWEROFF_INFO, *P_FLY_SILENCE_POWEROFF_INFO;

typedef struct _FLY_RESTORE_DATA
{
	//�ļ��Ƿ񱣴�ID
	UINT32 iIDForFileStore;

	//������ע���
	UINT dwFreqFM1;
	UINT dwFreqFM2;
	UINT dwFreqAM;

	//��Ƶ����
	BYTE iVideoParaColor;
	BYTE iVideoParaHue;
	BYTE iVideoParaContrast;
	BYTE iVideoParaBrightness;

	//�ƹ�
	BOOL bLightDetectEnable;
	BOOL bDayNight;
	BYTE iLCDLightBrightDay;
	BYTE iLCDLightBrightNight;
	BYTE iPanelLightBright;

	//����
	BYTE iRadioIDUser;
	BYTE iRadioIDDriver;

	//����
	BYTE iDVDRegionCode;

	//�ⲿ�绰
	BYTE iExtTelVolume;

	//����
	BYTE iBTCallVolume;

	//������
	BOOL bSteerWheelOn;
	UINT iRemoteDataUseWhat;//���0~N-1
	BOOL bRemoteUseStudyOn;

	//�Ƿ����ⲿ���ţ������ף�
	BOOL bHaveExtAmplifierGMC;
	
	//�ɸ��ⲿ��
	BOOL bHaveFlyAudioExtAMP;
	
	IRKEY_STUDY_TAB remoteStudyTab;

	BYTE iUseWhichPanel;//0xFF��ʾ��ȡ�������������

	BYTE iUARTDebugMsgOn;
}FLY_RESTORE_DATA;

typedef void (*_t_ipcStartEvent)(UINT32 sourceEvent);
typedef void (*_t_ipcClearEvent)(UINT32 sourceEvent);
typedef BOOL (*_t_ipcWhatEventOn)(UINT32 sourceEvent);

typedef void (*_t_debugString)(char *fmt);
typedef void (*_t_debugBuf)(char *fmt, BYTE *buf, UINT len);
typedef void (*_t_debugChar)(char *fmt, BYTE *buf, UINT len);
typedef void (*_t_debugOneData)(char *fmt, UINT32 data);
typedef void (*_t_debugTwoData)(char *fmt, UINT32 dataOne, UINT32 dataTwo);
typedef void (*_t_debugThreeData)(char *fmt, UINT32 dataOne, UINT32 dataTwo, UINT32 dataThree);

typedef void (*_t_ipcExchangeEvent)(UINT32 sourceEvent,BYTE objectHAL);
typedef void (*_t_ipcEventProcProc)(UINT32 sourceEvent);

typedef void (*_t_msgWriteToSerial)(BYTE msgQueueID,BYTE *pData,UINT length);
typedef void (*_t_msgReadTpmsFromSerial)(BYTE msgQueueID,BYTE *pData,UINT length);
typedef void (*_t_msgReadTvFromSerial)(BYTE msgQueueID,BYTE *pData,UINT length);

typedef void (*_t_readFromhardwareProc)(BYTE *buf,UINT length);
typedef UINT (*_t_readDataFromHardwareNoBlock)(BYTE *pData,UINT length);
typedef BOOL (*_t_writeDataToHardware)(BYTE *buf, UINT32 len);

typedef UINT32 (*_t_writeToJNIBuff)(BYTE iWhatHAL,BYTE *p,UINT32 length);
typedef UINT32 (*_t_readFromJNIBuff)(BYTE iWhatHAL,BYTE *p,UINT32 length);

typedef struct _PROCESS_SERVIAL_HAL
{
	_t_debugString _p_debugString;
	_t_debugBuf _p_debugBuf;
	_t_debugChar _p_debugChar;
	_t_debugOneData _p_debugOneData;
	_t_debugTwoData _p_debugTwoData;
	_t_debugThreeData _p_debugThreeData;

	_t_ipcStartEvent _p_ipcStartEvent;
	_t_ipcClearEvent _p_ipcClearEvent;
	_t_ipcWhatEventOn _p_ipcWhatEventOn;

	_t_readDataFromHardwareNoBlock _p_readDataFromHardwareNoBlock;
	_t_writeDataToHardware _p_writeDataToHardware;

	_t_writeToJNIBuff _p_writeToJNIBuff;
	_t_readFromJNIBuff _p_readFromJNIBuff;
	
	_t_msgWriteToSerial _p_msgWriteToSerial;
	_t_msgReadTpmsFromSerial _p_msgReadTpmsFromSerial;
	_t_msgReadTvFromSerial _p_msgReadTvFromSerial;
}PROCESS_SERVIAL_HAL;

typedef struct _PROCESS_OTHERS_HAL
{
	BOOL bHave;
	_t_ipcEventProcProc _p_ipcEventProcProc;
	_t_readFromhardwareProc _p_readFromhardwareProc;
}PROCESS_OTHERS_HAL;

typedef struct _FLY_SHARE_MEMORY_COMMON_DATA 
{
	UINT32 iGlobalStructSize;
	UINT32 iOSRunTime;

	PROCESS_SERVIAL_HAL processServialHal;
	PROCESS_OTHERS_HAL processOthersHal[SHARE_MEMORY_MAX];

	BYTE sErrorDriverName[8];

	BOOL bEventTransSet[EVENT_TRANS_LIST_MAX];

	FLY_SILENCE_POWEROFF_INFO SilencePowerOffInfo;

	FLY_RESTORE_DATA flyRestoreData;

	BOOL bNoSendAndroidSystemButton;

	BYTE iControlUserAction;

	BOOL bNeedReturnNewVolume;
	BOOL bMute;
	BYTE iVolume;
	BYTE iVolumeMin;
	BYTE iVolumeMax;

	//��������
	BYTE sCarModule[SCAR_MODEL_MAX];
	//������Ϣ����

	//������Ϣ����Ӧ
	BOOL bIngoreIPCEventMsgResponse;

	//������͵͵������
	BOOL bSilencePowerUp;

	//�Ƿ���TVģ��
	BOOL bHaveTVModule;
	//�Ƿ���TPMSģ��
	BOOL bHaveTPMSModule;

	//׼���ػ�ʱ�����ٺ�����ͨ��
	BOOL bPrepareToSleep;

	//ACC OFF
	BOOL bHostCompletePowerUp;
	BOOL bNeedWinCEPowerOff;//�������յ�ACC OFF
	BOOL bRecWinCESleepMsg;//���յ��ػ�
	BOOL bRecWinCEFactoryMsg;//���յ��ָ�����ģʽ
	BOOL bRecWinCERestartMsg;//���յ�����
	//ACC OFF���ٷ�������
	BOOL bNoMoreToSendDataWhenToSleep;
	//����
	BOOL bStandbyStatus;
	BOOL bStandbyStatusLast;
	BOOL bStandbyStatusWithACCOff;
	//OSD DEMOģʽ
	BOOL bOSDDemoMode;
	//����DEMOģʽ
	BOOL bKeyDemoMode;
	//��Դ��ѹ
	BYTE iBatteryVoltage;
	//�Ƿ����
	BOOL bBatteryVoltageLowAudio;
	BOOL bBatteryVoltageLowRadio;
	BOOL bBatteryVoltageLowHardware;
	//�����¶�
	INT iHostTemperature;

	//��ǰͨ��
	BYTE eAudioInput;
	BYTE eVideoInput;

	BYTE eCurAudioInput;//��ǰʵ��ͨ��

	BYTE GPSSpeaker;

	//�绰���
	BOOL bExtPhoneStatusIO;
	BOOL bExtPhoneStatusBUS;
	//ɲ��
	BOOL bBreakStatusIO;
	BOOL bBreakStatusBUS;

	//����
	BOOL bBackDetectEnable;
	BOOL bBackActiveNow;
	BOOL bBackVideoOn;
	BOOL bHaveEverBackVideoOn;
	ULONG iBackDelayLightOnTime;
	ULONG iBackDelayLightOnTimeLong;

	//���
	tPANEL_TAB tPanelTab;
	//������
	UINT iRemoteDataCount;//����N
	BYTE sRemoteDataName[256];
	UINT iRemoteDataNameLength;
	BYTE sRemoteData[1+2+5*IRKEYTABSIZE];
	BOOL bRemoteDataHave;
	//������ѧϰ
	BYTE iRemoteStudyClearID[IRKEY_STUDY_COUNT];
	BYTE iRemoteStudyID;

	//����
	BYTE iBTCallStatus;
	
	//�ⲿ�绰
	BYTE iExtTelCallStatus;

	//����ʱ������
	BYTE iNormalVolume;
	BOOL bNormalMute;

	//AUX
	BOOL bAUXHaveVideo;

	//��MCU֮����Ҫ��Ϣ���ظ�����
	BOOL bHaveRecMCUACCOn;
	BOOL bHaveRecMCUACCOff;

	ULONG iHowLongToRestart;

	//ע������¼�
	BOOL bRestoreRegeditToFactory;//�ָ���������
	//�͵�ѹ�¼�
	UINT iNeedProcVoltageShakeRadio;//0Ϊ�رգ����࿴Ӳ��
	UINT iNeedProcVoltageShakeAudio;//0Ϊ�رգ����࿴Ӳ��
	UINT iNeedProcVoltageShakeHardware;//0Ϊ�رգ����࿴Ӳ��

	//�����¼�
	BOOL bAudioMuteControlable;//��Ƶ�������Ծ���

	//7741�Ƿ��Ѿ���ʼ��
	BOOL b7741AudioInitFinish;
	BOOL b7741RadioInitFinish;
	
	//�Ƿ�����������ߵ�Դ
	BOOL bControlRadioANT;

	//drivers ����ʱ��
	UINT32 iDriverCompYear;
	BYTE iDriverCompMon;
	BYTE iDriverCompDay;
	BYTE iDriverCompHour;
	BYTE iDriverCompMin;
	BYTE iDriverCompSec;

	////////////////////////////SD��������Ϣ//////////////////////////////////////	

	//MCU�汾��
	BYTE iMCUSoftVersion[MCU_SOFT_VERSION_MAX];
	UINT iMCUSoftVersionLength;
	//���ִ���
	BOOL bCheckShellBabyError;
	//DEMOģʽ��
	UINT iLEDTestFlashCount;
	UINT iLEDTestFlashCountSub;
	//ACCOffLightOn
	BOOL bACCOffLightOn;
	//�����ļ��Ƿ��гɹ���SD��
	BOOL bSuccessReadDebugFileFromSDMEM;
	//7419IICƵ��
	UINT IICSpeadOnAudio;
	//�Զ�����
	BOOL bSystemResetUseExtConfig;
	UINT iSystemResetAtLeastDays;
	UINT iSystemResetOnHour;
	UINT iSystemResetOnMinute;
	UINT iSystemResetInnerMin;
	//���������೤ʱ��������͹���
	UINT iSystemResetPowerOffMin;
	//ϵͳ���������ж��ٷ���
	UINT iSystemResetCanRunLess;
	//���������֮�����Ϣ��Ӧ
	BOOL FlyIngoreMsgResponse;
	//ǿ�ƴ�OSD����
	BOOL FlyOSDDemoMode;

	//�����ǻ����������Ǳ���˾����
	BOOL bDVDType;
	
	//��������֮����
	BYTE ipcDriverMainAudioInput;

	BYTE ipcDriverAMPOn;
	BYTE ipcDriverbAMPMute;

	BOOL ipcbMuteBT;

	//��������֮����
	BYTE iDVDSoftwareVersion[MODULE_SOFT_VERSION_MAX];
	UINT iDVDSoftwareVersionLength;
	BYTE iBTSoftwareVersion[MODULE_SOFT_VERSION_MAX];
	UINT iBTSoftwareVersionLength;
	UINT iPanelKeyAD[3];
	UINT iSteelAD[2];
	BYTE iKeyIndex;
	BYTE iKeyValue;
	UINT iKeyCount;

	UINT iIICErrorOnAudio;
	UINT iIICErrorOnRadio;
	UINT iIICErrorOnMCU;

	BOOL bRecUserPingStart;

	//������
	BOOL bMCUIICCommTest;
	
	BOOL bNoMoreSendNormalIICTest;

	BOOL bTempSendKeyIndex;

	BYTE debugMsgServiceHalLevel;
	BYTE debugMsgSystemLevel;
	BYTE debugMsgAudioLevel;
	BYTE debugMsgDVDLevel;
	BYTE debugMsgKeyLevel;
	BYTE debugMsgTVLevel;
	BYTE debugMsgVideoLevel;
	BYTE debugMsgACLevel;
	BYTE debugMsgBtLevel;
	BYTE debugMsgExdisplayLevel;
	BYTE debugMsgGlobalLevel;
	BYTE debugMsgRadioLevel;
	BYTE debugMsgTpmsLevel;
	BYTE debugMsgXMRadioLevel;
	BYTE FlyAssistDisplayMsgLevel;
	UINT iStructSize;

}FLY_SHARE_MEMORY_COMMON_DATA;

typedef struct _FLY_SHARE_MEMORY_DVD_DATA 
{
	BYTE sDVDVersion[32];

	UINT iStructSize;
}FLY_SHARE_MEMORY_DVD_DATA;

enum enumBlockID{
	S_NO_BLOCK_ID = 0,
	S_BLOCK_ID
};
enum enumOperationID{
	SHARE_MEMORY_ID = 0,
	IPC_ID,
};

enum enumEventTrans{
	EVENT_GLOBAL_TEST_ID = 0,

	//�����¼�
	EVENT_GLOBAL_DATA_CHANGE_VOLUME,// 1

	EVENT_GLOBAL_REQ_USER_ACTION,// 2
	EVENT_GLOBAL_RES_USER_ACTION,// 3
	EVENT_GLOBAL_REQ_USER_ACCON,// 4

	//�����¼�
	EVENT_GLOBAL_BTCALLSTATUS_CHANGE_ID,// 5
	//�������¼�
	EVENT_GLOBAL_RADIO_CHANGE_ID,// 6
	//ɲ���¼�
	EVENT_GLOBAL_BREAKDETECT_CHANGE_ID,// 7
	//�ⲿ�绰�¼�
	EVENT_GLOBAL_PHONEDETECT_CHANGE_ID,// 8
	EVENT_GLOBAL_PHONECALLSTATUS_CHANGE_ID,// 9
	//�����¼�
	EVENT_GLOBAL_BACKDETECT_CHANGE_ID,// 10
	EVENT_GLOBAL_BACKDETECT_RETURN_ID,// 11
	EVENT_GLOBAL_BACK_LOW_VOLUME_ID,
	EVENT_GLOBAL_BACK_DELAY_LIGHT_ON_ID,

	//��尴���¼�
	EVENT_GLOBAL_PANEL_KEY_USE_IT_ID,// 12
	//�������¼�
	EVENT_GLOBAL_REMOTE_STUDY_START_ID,	// 13		//������ѧϰ����
	EVENT_GLOBAL_REMOTE_STUDY_STOP_ID,// 14
	EVENT_GLOBAL_REMOTE_STUDY_CLEAR_ID,// 15
	EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_WAIT_ID,// 16
	EVENT_GLOBAL_REMOTE_STUDY_PRE_RETURN_FINISH_ID,// 17
	EVENT_GLOBAL_REMOTE_STUDY_RETURN_WAIT_ID,	// 18//������ѧϰ����
	EVENT_GLOBAL_REMOTE_STUDY_RETURN_START_ID,// 19
	EVENT_GLOBAL_REMOTE_STUDY_RETURN_FINISH_ID,// 20
	EVENT_GLOBAL_REMOTE_USE_IT_ID,	// 21			//����������ʹ��
	//���Ŵ��¼�	
	EVENT_GLOBAL_INNER_AMP_ON_ID,// 23
	//DEMO OSD�¼�
	EVENT_GLOBAL_DEMO_OSD_START_ID,// 24
	EVENT_GLOBAL_DEMO_OSD_DISPLAY_ID,// 25
	//DEMO KEY�¼�
	EVENT_GLOBAL_DEMO_KEY_START_ID,// 26
	//��Դ�ָ��¼�
	EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID,// 27
	EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID,// 28

	//�����¼�
	EVENT_GLOBAL_RADIO_MUTE_IN_REQ_ID,
	EVENT_GLOBAL_RADIO_MUTE_IN_OK_ID,
	EVENT_GLOBAL_RADIO_MUTE_OUT_REQ_ID,
	//EVENT_GLOBAL_RADIO_MUTE_OUT_OK_ID,//û������ɶ�ô�

	EVENT_GLOBAL_BT_MUTE_REQ_ID,
	EVENT_GLOBAL_BT_MUTE_IN_OK_ID,
	//EVENT_GLOBAL_BT_MUTE_OUT_OK_ID,//û������ɶ�ô�

	//����������
	EVENT_GLOBAL_DVD_REGION_SET_ID,

	//AUX�����¼�
	EVENT_GLOBAL_AUX_CHECK_START_ID,
	EVENT_GLOBAL_AUX_CHECK_RETURN_ID,

	//�����������¼�
	EVENT_GLOBAL_RADIO_ANTENNA_ID,
	//�ⲿCDCС���л��¼�
	EVENT_GLOBAL_EXBOX_INPUT_CHANGE_ID,

	//�汾�ŷ���
	EVENT_GLOBAL_RETURN_DVD_VERSION_ID,
	EVENT_GLOBAL_RETURN_BT_VERSION_ID,

	//LED��˸ָʾ����
	EVENT_GLOBAL_ERROR_LEDFLASH_ID,

	//͵͵������
	EVENT_GLOBAL_SILENCE_POWER_UP_ID,

	//TV�д���������Ϣ
	EVENT_GLOBAL_TVBOX_EXIST_ID,

	//TPMS�д���������Ϣ
	EVENT_GLOBAL_TPMSBOX_EXIST_ID,

	//Ӧ�ó����Ping
	EVENT_GLOBAL_USER_PING_START_ID,
	EVENT_GLOBAL_USER_PING_WORK_ID,

	//��λ��Ϣ
	EVENT_GLOBAL_FORCE_RESET_ID,

	//������
	EVENT_GLOBAL_FORCE_LCD_LIGHT_ON_ID,

	//������
	EVENT_GLOBAL_LED_BLINK,
	EVENT_GLOBAL_RESET_USB_HUB,

	//�����¼�
	EVENT_GLOBAL_KEY_STANDBY_ID,

	EVENT_AUTO_CLR_STANDBY_ID,

	//�޲�����ʱ�����¼�
	EVENT_TOUCH_TIMEOUT_RETURN_ID,

	//�������¼�
	EVENT_AUTO_CLR_SUSPEND_ID,
	EVENT_AUTO_CLR_RESUME_ID,

	//ע������¼�
	EVENT_AUTO_CLR_PARA_INIT_ID,
	EVENT_AUTO_CLR_PARA_READ_ID,
	EVENT_AUTO_CLR_PARA_WRITE_ID,

	//������ʾ��
	EVENT_GLOBAL_DATA_CHANGE_MAX
};

enum enumMsgQueueID{
	MSG_QUEUE_TEST = 0,
	MSG_QUEUE_RDS_TO_RADIO
};

//����HAL��Driverͨ�Ų���
enum enumDVDMsgID{
	MSG_DVD_INIT,
	MSG_DVD_CON_RESET_ON,
	MSG_DVD_CON_RESET_OFF,
	MSG_DVD_CON_LIGHT,
	MSG_DVD_MAXMSG
};
enum enumBTMsgID{
	MSG_BT_INIT,
	MSG_BT_CON_POWER,
	MSG_BT_CON_RESET_ON,
	MSG_BT_CON_RESET_OFF,
	MSG_BT_CON_CE_NORMAL,
	MSG_BT_CON_CE_UPDATE,
	MSG_BT_MAXMSG
};
enum enumVideoMsgID{
	MSG_VIDEO_INIT,
	MSG_VIDEO_REQ_HAVE_VIDEO,
	MSG_VIDEO_CON_INPUT,
	MSG_VIDEO_CON_COLORFUL,
	MSG_VIDEO_MAXMSG
};
enum enumKeyMsgID{
	MSG_KEY_INIT,
	MSG_KEY_RES_KEYID,
	MSG_KEY_MAXMSG
};
enum enumSystemMsgID{
	MSG_SYSTEM_INIT,
	MSG_SYSTEM_TRANS_MCU,
	MSG_SYSTEM_TRANS_NORMAL,
	MSG_SYSTEM_CON_SUSPEND_RESUME,
	MSG_SYSTEM_RES_SUSPEND_RESUME,
	MSG_SYSTEM_CON_FAN,
	MSG_SYSTEM_CON_PANEPEN,
	MSG_SYSTEM_CON_LCDIDLE,
	MSG_SYSTEM_CON_LCDPWM,
	MSG_SYSTEM_MAXMSG
};
#define MSG_AUDIO_TRANS_EVENT 0xA9

#define MSG_RADIO_CON_ANT1				0x10
#define MSG_RADIO_CON_ANT2				0x11
#define MSG_RADIO_TDA7541_AFMUTE		0x12
#define MSG_RADIO_REQ_TDA7541_SSTOP_ID	0x13
#define MSG_RADIO_CON_AD_GET			0x14
#define MSG_RADIO_CON_TDA7541_ID		0x20
#define MSG_RADIO_CON_TDA7541_EEPROM_ID	0x21
#define MSG_RADIO_CON_TEF7000_ID1		0x21
#define MSG_RADIO_CON_TEF7000_ID2		0x22
#define MSG_RADIO_CON_SAF7741_ID		0x23

#define MSG_RADIO_TDA7541_RDS_ID		0x90

//����HAL��Driverͨ�Ų���

//����Driver֮���ͨ�Ų���
enum enumIPCDriverID{
	IPC_DRIVER_MMAP,
	IPC_DRIVER_HARDWARE,
	IPC_DRIVER_AUDIO,

	IPC_DRIVER_MAX,
};

enum enumDriverIPCID{
	IPC_DRIVER_EVENT_AUDIO_NORMAL,
	IPC_DRIVER_EVENT_RESET_7741,
	IPC_DRIVER_EVENT_AMP_MUTE,
	IPC_DRIVER_EVENT_AMP_ONOFF,
	IPC_DRIVER_EVENT_MAIN_AUDIO_INPUT,
	IPC_DRIVER_EVENT_MAX
};
//����Driver֮���ͨ�Ų���
#define  KB_NP						0
#define  KB_SLEEP					1

#define  KB_MENU					2
#define  KB_AV						3			 
#define  KB_DVD						4
#define  KB_CDC						5
#define  KB_RADIO					6
#define  KB_IPOD					7
#define  KB_BT 						8
#define  KB_AC						9
#define  KB_TV						0x0a
#define  KB_FM1						0x0b
#define  KB_FM2						0x0c
#define  KB_AM						0x0d
#define  KB_FM						0x0e
//	#define  KB_AV_DEC					0x0f			 //20081216

#define  KB_NAVI					0x10
#define  KB_DEST					0x11
#define  KB_LOCAL					0x12
#define  KB_SETUP					0x13
#define  KB_INFO					0x14

#define  KB_CALL					0x31
#define  KB_CALL_REJECT				0x32
#define  KB_DIMM					0x33
#define  KB_DVD_OPEN				0x34	  //DVD��������Ƽ�
#define  KB_VOL_INC					0x35	  //ÿ��������1
#define  KB_VOL_DEC					0x36	  //ÿ��������1
#define  KB_MUTE					0x37	  //3S˯��
#define  KB_SEEK_INC				0x38	  //0��3��
#define  KB_SEEK_DEC				0x39	  //0.3��
#define  KB_TUNE_DOWN				0x3a
#define  KB_TUNE_INC				0x3b	  //0.2S
#define  KB_TUNE_DEC				0x3c	  //0.2S
#define  KB_SEEK_INC2				0x3d	  //0.3S
#define  KB_SEEK_DEC2				0x3e	  //0.3S
#define  KB_PANEL_ANGLE				0x3f	  //���ǻ���һ��һ�λ����׵�OPEN��
#define  KB_PANEL_OPEN				0x40	  //���ǻ���һ��һ�λ����׵�OPEN��
#define  KB_UP						0x41	  //���ǻ���һ���ֱ������������ҿ���   0.3S
#define  KB_DOWN					0x42
#define  KB_LEFT					0x43
#define  KB_RIGHT					0x44	  //0.3S
#define  KB_ENTER					0x45
#define  KB_CALL_OUT                0x46
#define  KB_CALL_INOUT              0x47
#define  KB_SCAN               		0x48
#define  KB_PAUSE              		0x49
#define  KB_TUNE_DOWN_LONG			0x4a
#define  KB_SPEECH_IDENTIFY_START	0x4b 	
#define  KB_SPEECH_IDENTIFY_END		0x4c

#define  KB_DVD_SEEK_INC            0x60  
#define  KB_DVD_SEEK_DEC            0x61
#define  KB_RADIO_SEEK_INC          0x62
#define  KB_RADIO_SEEK_DEC		    0x63
#define  KB_RADIO_PROG				0x66
#define  KB_SRADIO_SEEK_INC         0x64
#define  KB_SRADIO_SEEK_DEC         0x65

#define	KB_ANDROID_HOME				0xF0
#define	KB_ANDROID_MENU				0xF1
#define KB_ANDROID_BACK				0xF2

#define I2_0_ID     0
#define I2_1_ID     1


//IIC Address
#define SAF7741_ADDR_W				0x38
#define TEF7000_1_ADDR_W			0xC0
#define TEF7000_2_ADDR_W			0xC4

#define FLY7419_ADDR_R				0x89
#define FLY7419_ADDR_W				0x88

#define TDA7541_ADDR_R				0xC5
#define TDA7541_ADDR_W				0xC4

#define MCU_ADDR_R				0xA1
#define MCU_ADDR_W				0xA0

#define CAM_IIC_ADD_W	0xB8
#define CAM_IIC_ADD_R	0xB9

#define PCA9554_ADD_R			0x41
#define PCA9554_ADD_W			0x40

#define AN15887_ADD_W			0x90
#define AN15887_ADD_R			0x91

#ifndef DRIVER_IO_NAME
#define DRIVER_IO_NAME
//��ƽ
#define GPIO_HEIGHT         1
#define GPIO_LOW            0

//��
#define GPIOA   0
#define GPIOB 	1
#define GPIOC 	2
#define GPIOD 	3
#define GPIOE 	4
#define GPIOF	5
#define GPIOG 	6
//���
#define GPIO_INDEX_0 	0
#define GPIO_INDEX_1 	1
#define GPIO_INDEX_2 	2
#define GPIO_INDEX_3 	3
#define GPIO_INDEX_4 	4
#define GPIO_INDEX_5 	5
#define GPIO_INDEX_6 	6
#define GPIO_INDEX_7 	7
#define GPIO_INDEX_8 	8
#define GPIO_INDEX_9 	9
#define GPIO_INDEX_10 	10
#define GPIO_INDEX_11 	11
#define GPIO_INDEX_12 	12
#define GPIO_INDEX_13 	13
#define GPIO_INDEX_14 	14
#define GPIO_INDEX_15 	15
#define GPIO_INDEX_16   16
#define GPIO_INDEX_17	17
#define GPIO_INDEX_18 	18
#define GPIO_INDEX_19 	19
#define GPIO_INDEX_20   20
#define GPIO_INDEX_21   21
#define GPIO_INDEX_22 	22
#define GPIO_INDEX_23 	23
#define GPIO_INDEX_24 	24
#define GPIO_INDEX_25 	25
#define GPIO_INDEX_26   26
#define GPIO_INDEX_27 	27
#define GPIO_INDEX_28 	28
#define GPIO_INDEX_29 	29
#define GPIO_INDEX_30 	30
#define GPIO_INDEX_31   31
#define GPIO_INDEX_32 	32

//HUB
#define HUB_RST_G	GPIOF
#define HUB_RST_I	GPIO_INDEX_16

//DVD
#define DVD_RST_G	GPIOA
#define DVD_RST_I	GPIO_INDEX_12

#define DVD_LEDC_GROUP		GPIOD
#define DVD_LEDC_GPIO		GPIO_INDEX_12

//CAM_EN
#define BACK_CAM_EN_G	GPIOF
#define BACK_CAM_EN_I	GPIO_INDEX_14
//BT
#define BT_POWER_GROUP 		GPIOA
#define BT_POWER_GPIO		GPIO_INDEX_13
#define BT_RESET_GROUP 		GPIOA
#define BT_RESET_GPIO		GPIO_INDEX_11

//Video
#define CAM_POWER_G	GPIOF
#define CAM_POWER_I	GPIO_INDEX_11

#define CAM_RESET_G	GPIOE
#define CAM_RESET_I	GPIO_INDEX_8

#define VIDEO_IPOD_AUX_BACK_4052_G	GPIOG
#define VIDEO_IPOD_AUX_BACK_4052_I	GPIO_INDEX_5

//Audio
#define AUDIO_AMP_MUTE_G	GPIOG
#define AUDIO_AMP_MUTE_I	GPIO_INDEX_12

#define DAC_RESET_G	GPIOG
#define DAC_RESET_I	GPIO_INDEX_14

#define AUDIO_MP3_BT_4052_G		GPIOE
#define AUDIO_MP3_BT_4052_I		GPIO_INDEX_2

//system
#define SYSTEM_FAN_CONT_GROUP   GPIOE
#define SYSTEM_FAN_GPIO         GPIO_INDEX_6
#define SYSTEM_PANNE_PEN_GROUP  GPIOE
#define SYSTEM_PANNE_PEN        GPIO_INDEX_7

#define SYSTEM_LCD_IDLE_GROUP   GPIOG
#define SYSTEM_LCD_IDLE_GPIO    GPIO_INDEX_2

#define SYSTEM_LCD_PWM_GROUP   GPIOA
#define SYSTEM_LCD_PWM_GPIO    GPIO_INDEX_4


#define SYSTEM_USB_EN_G			GPIOC
#define SYSTEM_USB_EN_I			GPIO_INDEX_31



#define SYSTEM_TEST_LED_G		GPIOF
#define SYSTEM_TEST_LED_I		GPIO_INDEX_15

//Radio
#define RADIO_ANT1_G	GPIOE
#define RADIO_ANT1_I	GPIO_INDEX_3


#define RADIO_AF_MUTE_7741_RST_G	GPIOG
#define RADIO_AF_MUTE_7741_RST_I	GPIO_INDEX_13

#define RADIO_SSTOP_GROUP	GPIOD
#define RADIO_SSTOP_GPIO	GPIO_INDEX_11

//3G
#define G3G_LED_G	GPIOG
#define G3G_LED_I	GPIO_INDEX_4

#define G3G_RST_G	GPIOE
#define G3G_RST_I	GPIO_INDEX_9

#define G3G_POWER_G	GPIOG
#define G3G_POWER_I	GPIO_INDEX_6

//wifi
#define WIFI_POWER_EN_GROUP		GPIOA
#define WIFI_POWER_EN_GPIO		GPIO_INDEX_2
#define WIFI_POWER_GROUP GPIOG
#define WIFI_POWER_GPIO  GPIO_INDEX_11
#define WIFI_PEN_GROUP   GPIOG
#define WIFI_PEN_GPIO    GPIO_INDEX_10


//MCU�жϴ���
#define MCU_IIC_REQ_G	GPIOA
#define MCU_IIC_REQ_I	GPIO_INDEX_5

#define MCU_IIC_REQ_ISR	5

//ɲ���͵绰�жϴ���
#define BREAK_DETECT_G		GPIOE
#define BREAK_DETECT_I		GPIO_INDEX_5
#define BREAK_DETECT_ISR	53

#define PHONE_DETECT_G		GPIOA
#define PHONE_DETECT_I		GPIO_INDEX_14
#define PHONE_DETECT_ISR	14

//��ť�жϴ������

#define ENCODER_L1_G	GPIOD
#define ENCODER_L1_I	GPIO_INDEX_15
#define ENCODER_L2_G	GPIOD
#define ENCODER_L2_I	GPIO_INDEX_19


#define ENCODER_R1_G	GPIOD
#define ENCODER_R1_I	GPIO_INDEX_16
#define ENCODER_R2_G	GPIOD
#define ENCODER_R2_I	GPIO_INDEX_20

#define ENCODER_ISR_L1	24
#define ENCODER_ISR_L2	28
#define ENCODER_ISR_R1	25
#define ENCODER_ISR_R2	29

#endif


#endif
