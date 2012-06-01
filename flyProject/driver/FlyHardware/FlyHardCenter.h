#ifndef FLY_HARDWARE_H_
#define FLY_HARDWARE_H_

#define 	MISC_DYNAMIC_MINOR 		255				//动态设备号
#define 	DEVICE_NAME 			"FlyHardware"		//驱动名
#define     BUF_MAX_SIZE            300

//打火延时恢复
#define LOW_VOLTAGE_DELAY	1000

//返回给HAL的消息
#define		KEY_MESSAGE_BUFF_SIZE	256
#define		KEY_MESSAGE_BUFF_LENGTH	3

#define		RADIO_MESSAGE_BUFF_SIZE		64
#define		RADIO_MESSAGE_BUFF_LENGTH	10

#define		SYSTEM_MESSAGE_BUFF_SIZE		64
#define		SYSTEM_MESSAGE_BUFF_LENGTH		256

#define		AC_MESSAGE_BUFF_SIZE		64
#define		AC_MESSAGE_BUFF_LENGTH		256

#define		SERVICE_MESSAGE_BUFF_SIZE		64
#define		SERVICE_MESSAGE_BUFF_LENGTH		256

#define DATA_BUFF_LENGTH_FROM_MCU		128	//立即处理

#define SHORT_KEY_MIN	50
#define KEY_SCAN_SPACE	30
#define KEY_ADC_COUNT	5

#define  REMOTE_R_UP     2200
#define  REMOTE_R_BASE   122

#define PANEL_ADC_CHANNEL1	14
#define PANEL_ADC_CHANNEL2	9
#define PANEL_ADC_CHANNEL3	8

#define STEEL_ADC_CHANNEL1	1	//5
#define STEEL_ADC_CHANNEL2	0	//4

#define STEEL_ADC_STUDY_DANCE		10	//学习时限制抖动范围
#define STEEL_ADC_STUDY_DISTANCE	50	//学习时与初始值的最小差别
#define STEEL_ADC_CHANGE_DANCE		40	//方向盘转换后的左右最大摆动范围
#define STEEL_ADC_CHANGE_MAX		1000//采样动作临界值

typedef struct _IRKEY_TAB{
	BYTE IRKEY_Value[IRKEYTABSIZE]; //
	BYTE IRKEY_Port[IRKEYTABSIZE];	 //1 Remote1,  2 Remote2
	UINT IRKEY_AD_Min[IRKEYTABSIZE];
	UINT IRKEY_AD_Max[IRKEYTABSIZE];
	BYTE CarTypeID[2];
	BYTE size;		//有效方向盘按键数量
}IRKEY_TAB, *P_IRKEY_TAB;

//旋钮中断处理
typedef struct _FLY_KEY_ENCODER_INFO
{
	BYTE curEncodeValueLeft;
	BYTE curEncodeValueRight;

	UINT iEncoderLeftIncCount;
	UINT iEncoderLeftDecCount;
	UINT iEncoderRightIncCount;
	UINT iEncoderRightDecCount;
	
	UINT32 time_out;
	BOOL bTimeOutRun;
	struct work_struct encoder_work;
}FLY_KEY_ENCODER_INFO, *P_FLY_KEY_ENCODER_INFO;

typedef struct _FLY_IIC_INFO
{
	UINT32 time_out;
	struct work_struct iic_work;
}FLY_IIC_INFO, *P_FLY_IIC_INFO;

typedef struct fly_AN15887_info{
	BYTE mainAudioInput;
	BYTE mainVideoInput;
	BYTE secondAudioInput;
	BYTE secondVideoInput;
	BYTE regData[4];
}FLY_AN15887_INFO,*P_AN_15887_INFO;

struct fly_hardware_info{
	tPANEL_TAB tPanelTab; 

	FLY_KEY_ENCODER_INFO FlyKeyEncoderInfo;
	FLY_IIC_INFO FlyIICInfo;
	
	ULONG nowTimer;

	UINT CurrentAD[5];
	ULONG lastConversionTime;

	IRKEY_TAB remoteTab;
	UINT remote1ADCS;
	UINT remote2ADCS;

	BOOL remoteStudyStart;
	BYTE remoteStudyStep;
	BYTE remoteStudyCount;
	UINT remoteStudyNormalAD1;
	UINT remoteStudyNormalAD2;

	UINT remoteStudyCurrent1;//方向盘学习用
	UINT remoteStudyCurrent2;

	UINT iKeyDemoState;
	ULONG iKeyDemoInnerTime;

	BYTE iPreLCDPWM;
	BYTE iCurLCDPWM;

	//视频
	BYTE camCurVideoChannel;
	BYTE camParaColor;
	BYTE camParaHue;
	BYTE camParaContrast;
	BYTE camParaBrightness;

	BYTE camPreVideoPal;
	BYTE camCurVideoPal;

	BOOL tvp5150RegIsReWrite[256];
	BYTE tvp5150RegReWriteData[256];

	FLY_AN15887_INFO sFlyAN15887Info;
	
	BOOL bFlyKeyADCDelayWorkRunning;
	struct delayed_work adc_delay_work;


	BYTE buffFromMCU[DATA_BUFF_LENGTH_FROM_MCU];
	BYTE buffFromMCUProcessorStatus;
	UINT buffFromMCUFrameLength;
	UINT buffFromMCUFrameLengthMax;
	BYTE buffFromMCUCRC;
	BYTE buffFromMCUBak[DATA_BUFF_LENGTH_FROM_MCU];

	//收音头读AD值特殊处理
	BOOL bRadioADRead;
	ULONG iRadioADReadTime;
	UINT iRadioADValue;
	struct completion compRadioADRead;
	
	//串转并
	BYTE serialToParallelOut;

	//与HAL同步的状态
	BYTE keyMessageBuff[KEY_MESSAGE_BUFF_SIZE][KEY_MESSAGE_BUFF_LENGTH];
	UINT keyMessageBuffLx;
	UINT keyMessageBuffHx;
	struct semaphore semKeyMessage;
	BYTE radioMessageBuff[RADIO_MESSAGE_BUFF_SIZE][RADIO_MESSAGE_BUFF_LENGTH];
	UINT radioMessageBuffLx;
	UINT radioMessageBuffHx;
	struct semaphore semRadioMessage;
	BYTE systemMessageBuff[SYSTEM_MESSAGE_BUFF_SIZE][SYSTEM_MESSAGE_BUFF_LENGTH];
	UINT systemMessageBuffLx;
	UINT systemMessageBuffHx;
	struct semaphore semSystemMessage;
	BYTE acMessageBuff[AC_MESSAGE_BUFF_SIZE][AC_MESSAGE_BUFF_LENGTH];
	UINT acMessageBuffLx;
	UINT acMessageBuffHx;
	struct semaphore semAcMessage;
	BYTE serviceMessageBuff[SERVICE_MESSAGE_BUFF_SIZE][SERVICE_MESSAGE_BUFF_LENGTH];
	UINT serviceMessageBuffLx;
	UINT serviceMessageBuffHx;
	struct semaphore semServiceMessage;

	wait_queue_head_t read_wait;

	BOOL bBreakStatusIO;
	BOOL bExtPhoneStatusIO;

	BOOL bNeedWakeupThread;

	//打火处理
	DWORD iProcVoltageShakeDelayTime;

	UINT sFLAG_FOR_15S_OFF;

	struct early_suspend early_suspend;

	//长按6SMute复位
	ULONG iMutePressTime;

	ULONG iTouchTimeoutTime;
};

extern struct fly_hardware_info *pGlobalHardwareInfo;

void actualIICWrite(BYTE busID, BYTE chipAddW, BYTE *buf, UINT size);
void actualIICRead(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size);
void actualIICReadSimple(BYTE busID, BYTE chipAddW, BYTE *buf, UINT size);
void actualIICReadSAF7741(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size);
void actualIICWriteTEF7000(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size);
void actualIICReadTEF7000(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size);

void voltageShakeThread(void);
void sleepOnProcSleep(void);
void sleepOnProcWakeup(void);
void voltageShakeProc(BYTE iBatteryVoltage);

void consoleDebugSwitch(void);

#define LPC_SYSTEM_TYPE 0x00
#define LPC_SOUND_TYPE  0X01
#define LPC_KEY_TYPE	0X02
#define LPC_CAN_TYPE	0X03


#define DEBUG_MSG_ON  0
#if DEBUG_MSG_ON
#define DBG0(fmt,arg...) printk(fmt,##arg)
#define DBG(fmt, arg...) printk(fmt,##arg)
#define DBG_ISR(CODE) printk(fmt,##arg)
//#define DBG(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
//#define DBG_ISR(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DBG0(fmt,arg...) printk(fmt,##arg)
#define DBG(fmt, arg...) do { (void)(fmt); } while(0)
#define DBG_ISR(fmt, arg...) do { (void)(fmt); } while(0)
//#define DBG(fmt, arg...) printk(fmt,##arg)
//#define DBG_ISR(fmt, arg...) printk(fmt,##arg)
#endif

#endif