#ifndef FLY_RADIO_H_
#define FLY_RADIO_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

#define BUF_MAX_LEN 4096
#define INIT_DATA_SIZE				144

static const BYTE DefaultData[INIT_DATA_SIZE] = {
	0xCA, 0x12, 0x60, 0xD1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0x68, 0x08, 0xC5, 0xE4, 0x3D, 0x46, 0x53, 0x6F, 0x40, 
	0x5F, 0xEC, 0xA9, 0x91, 0xE8, 0x17, 0x6B, 0x89, 0x0D, 0x79, 0x8B, 0x04, 0x05, 0xE2, 0x0E, 0x93, 0x2D, 0xFF, 0x14, 0x29, 
	0xB1, 0x50, 0x6A, 0x49, 0x2E, 0x49, 0xA9, 0x31, 0xC8, 0xD7, 0x31, 0x74, 0x0D, 0x7D, 0x57, 0xFA, 0x05, 0xC0, 0x23, 0xFF, 
	0x00, 0x67, 0xCD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x64, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xD1, 0xC5, 0xC0, 0xCC, 0xC8, 0x9C, 0x99, 0xFF, 0x3A, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0x04, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x05, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1B, 0x4F, 0xE6, 0x89, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0x1C, 0xF2, 0x47}; 
static BYTE FM_Test[8]={0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe};//FM test registers
static BYTE AM_Test[8]={0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe,0xfe};//AM test registers

//TDA7541 寄存器
#define AUTO_INCREMENT_MODE			0x20

#define RADIO_RDS 1
#define RADIO_RDS_DEBUG_MSG 0

#define RADIO_RDS_DISPLAY_DEBUG	0

#define RADIORDS_ID_TA				0x30
#define RADIORDS_ID_PS				0x21
#define RADIORDS_ID_RT				0x22
#define RADIORDS_ID_PTY				0x23

#define GROUP_A     0
#define GROUP_B     1

#define TDA7541_ID					0x03

#define IO_SSTOP_IN
#define FM_SUM						141
#define AM_SUM						142
#define AN_SUM						143

#define TUNER_SEEK_MUTE_CTROL_ADDR0_0			0,0
#define TUNER_PARA_AREA_ADDR0_23				0,23
#define TUNER_TESTING_CTROL_ADDR24_31			24,31


#define	TDA7541_ADDR0_REG			0,0
#define	TDA7541_ADDR1_REG			1,1
#define	TDA7541_ADDR2_REG			2,2
#define	TDA7541_ADDR3_REG			3,3
#define	TDA7541_ADDR4_REG			4,4
#define	TDA7541_ADDR5_REG			5,5
#define	TDA7541_ADDR10_REG			10,10
#define	TDA7541_ADDR11_REG			11,11
#define	TDA7541_ADDR13_REG			13,13
#define	TDA7541_ADDR14_REG			14,14
#define	TDA7541_ADDR15_REG			15,15
#define	TDA7541_ADDR16_REG			16,16
#define	TDA7541_ADDR17_REG			17,17
#define	TDA7541_ADDR18_REG			18,18
#define	TDA7541_ADDR19_REG			19,19
#define	TDA7541_ADDR25_REG			25,25

#define FM_REG_OFFSET		10
#define AM_REG_OFFSET		34
#define OIRT_EU_ANUM		58
#define JPN_ANUM			59
#define EU_FREQ_OFFSET		60
#define OIRT_FREQ_OFFSET	68
#define JAPAN_FREQ_OFFSET	76
#define EU_TV_OFFSET		84
#define OIRT_TV_OFFSET		92
#define JAPAN_TV_OFFSET		100
#define EU_SM_OFFSET		108
#define OIRT_SM_OFFSET		116
#define JAPAN_SM_OFFSET     124
#define OIRT_IQ_OFFESET		132
#define OIRT_TVOP			134
#define OIRT_SEP			135

#define TV					0
#define SMETER				1
#define IF 					10700

//====================================================//
//================ TDA7541 register bit define========//
//====================================================//
//For Tun_Buff[0]
//Bit2~0
#define TDA7541_ADDR0_REG_ICP					0,2
#define TDA7541_PLL_HICURRENT_0MA       		0x00
#define TDA7541_PLL_HICURRENT_0_5mA     		0x01
#define TDA7541_PLL_HICURRENT_1MA       		0x02
#define TDA7541_PLL_HICURRENT_1_5mA     		0x03
#define TDA7541_PLL_HICURRENT_2MA       		0x04
#define TDA7541_PLL_HICURRENT_2_5mA     		0x05
#define TDA7541_PLL_HICURRENT_3MA       		0x06
#define TDA7541_PLL_HICURRENT_3_5mA     		0x07
//Bit3
#define TDA7541_ADDR0_REG_LDENA					3,3
#define TDA7541_PLL_LOCK_DISABLE				0x00
#define TDA7541_PLL_LOCK_ENABLE					0x08
//Bit4
#define TDA7541_ADDR0_REG_SDM					4,4
#define TDA7541_STEREO_MUTE_DISABLE				0x00
#define TDA7541_STEREO_MUTE_ENABLE				0x10
//Bit5
#define TDA7541_ADDR0_REG_LM					5,5
#define TDA7541_LOCAL_DISABLE					0x00
#define TDA7541_LOCAL_ENABLE					0x20
//Bit6
#define TDA7541_ADDR0_REG_ASFC					6,6
#define TDA7541_ASFC_NORMAL						0x00//weak signal mute and AGC normal mode in FM
#define TDA7541_ASFC_HOLD						0x40//weak signal mute and AGC on hold in FM
//Bit7
#define TDA7541_ADDR0_REG_SEEK					7,7
#define TDA7541_SEEK_OFF						0x00
#define TDA7541_SEEK_ON							0x80

//TPLL COUNTER1 LSB Tun_Buff[1]	 VALUE = 0~255
#define TDA7541_ADDR1_REG_PC_LSB				0,7
//TPLL COUNTER1 MSB Tun_Buff[2]	 VALUE = 0~255
#define TDA7541_ADDR2_REG_PC_MSB				0,7
//TV Tun_Buff[3]
#define TDA7541_ADDR3_REG_TV					0,7

//Sample time and IFC   Tun_Buff[4]
//Bit0
#define TDA7541_ADDR4_REG_TVM					0,0
#define TDA7541_PLL_TV_TRACK					0x00
#define TDA7541_PLL_TV_INDEPEDENT				0x01
//Bit1
#define TDA7541_ADDR4_REG_TVO					1,1
#define TDA7541_PLL_TVOFFSET_DISABLE			0x00
#define TDA7541_PLL_TVOFFSET_ENABLE				0x02
//Bit2
#define TDA7541_ADDR4_REG_ISSENA				2,2
#define TDA7541_ISS_ENABLE                      0x04
#define TDA7541_ISS_DISABLE                     0x00
//Bit4~3  与ADDR25_d6 组合
#define TDA7541_ADDR4_REG_IFS					3,4
#define TDA7541_PLL_TSAMPLE_FM20_48_AM128		0x00
#define TDA7541_PLL_TSAMPLE_FM10_24_AM64		0x08
#define TDA7541_PLL_TSAMPLE_FM5_12_AM32			0x10
#define TDA7541_PLL_TSAMPLE_FM2_56_AM16			0x18
//---expand mode for sampling time
#define TDA7541_PLL_TSAMPLE_FM1_28_AM8			0x00
#define TDA7541_PLL_TSAMPLE_FM0_64_AM4			0x08
#define TDA7541_PLL_TSAMPLE_FM0_32_AM2			0x10
#define TDA7541_PLL_TSAMPLE_FM0_16_AM1			0x18

//Bit6~5 与ADDR25_d7 组合
#define TDA7541_ADDR4_REG_EW					5,6
#define TDA7541_PLL_IFC_EW_FM12_5_AM2K			0x00
#define TDA7541_PLL_IFC_EW_FM25_AM4K			0x20
#define TDA7541_PLL_IFC_EW_FM50_AM8K			0x40
#define TDA7541_PLL_IFC_EW_FM100_AM16K			0x60
//---expand mode for error window
#define TDA7541_PLL_IFC_EW_FM6_25_AM1K			0x60
//Bit7
#define TDA7541_ADDR4_REG_FMON					7,7
#define TDA7541_AMMODE							0x00
#define TDA7541_FMMODE							0x80


//VCO & Pll reference freqency Tun_Buff[5]
//Bit1~0
#define TDA7541_ADDR5_REG_VCOD					0,1
#define TDA7541_VCO_DIVIDER_NOTVALID			0x00
#define TDA7541_VCO_DIVIDER_2					0x01
#define TDA7541_VCO_DIVIDER_3					0x02
#define TDA7541_VCO_DIVIDER_ORIGINAL			0x03
//Bit2
#define TDA7541_ADDR5_REG_VCOI					2,2
#define TDA7541_VCO_PHASE_0						0x00
#define TDA7541_VCO_PHASE_180					0x04
//Bit5~3
#define TDA7541_ADDR5_REG_RC					3,5
#define TDA7541_PLL_REFERECE_2KHz				0x18
#define TDA7541_PLL_REFERECE_9KHz				0x20
#define TDA7541_PLL_REFERECE_10KHz				0x28
#define TDA7541_PLL_REFERECE_25KHz				0x30
#define TDA7541_PLL_REFERECE_50KHz				0x38
//Bit7~6
#define TDA7541_ADDR5_REG_AMD					6,7
#define TDA7541_AM_PREDIVIDER_10				0x00
#define TDA7541_AM_PREDIVIDER_8					0x40
#define TDA7541_AM_PREDIVIDER_6					0x80
#define TDA7541_AM_PREDIVIDER_4					0xc0

//For SSTOP threshold Tun_Buff[10]
//Bit0~1
#define TDA7541_ADDR10_REG_MUX					0,1

//Bit4~7
#define TDA7541_ADDR10_REG_SSTH					4,7
#define TDA7541_SSTOP_IFC						0x00
#define TDA7541_SSTOP_IFC_FM0_5_AM1_1			0x10
#define TDA7541_SSTOP_IFC_FM0_7_AM1_4			0x20
#define TDA7541_SSTOP_IFC_FM0_9_AM1_7			0x30
#define TDA7541_SSTOP_IFC_FM1_1_AM2_0			0x40
#define TDA7541_SSTOP_IFC_FM1_3_AM2_3			0x50
#define TDA7541_SSTOP_IFC_FM1_5_AM2_6			0x60
#define TDA7541_SSTOP_IFC_FM1_7_AM2_9			0x70
#define TDA7541_SSTOP_IFC_FM1_9_AM3_2			0x80
#define TDA7541_SSTOP_IFC_FM2_1_AM3_5			0x90
#define TDA7541_SSTOP_IFC_FM2_3_AM3_8			0xA0
#define TDA7541_SSTOP_IFC_FM2_5_AM4_1			0xB0
#define TDA7541_SSTOP_IFC_FM2_7_AM4_4			0xC0
#define TDA7541_SSTOP_IFC_FM2_9_AM4_7			0xD0
#define TDA7541_SSTOP_IFC_FM3_1_AM5_0			0xE0
#define TDA7541_SSTOP_IFC_FM3_2_AM5_3			0xF0

//Tun_Buff[11]
#define TDA7541_ADDR11_WNON						1,1
#define TDA7541_WB_DISABLE						0x00
#define TDA7541_WB_ENABLE						0x02
#define TDA7541_ADDR11_WMTH                     2,4
#define TDA7541_WMTH_STARTMUTE_0                0X00
#define TDA7541_WMTH_STARTMUTE_1                0X04
#define TDA7541_WMTH_STARTMUTE_2                0X08
#define TDA7541_WMTH_STARTMUTE_3                0X0C
#define TDA7541_WMTH_STARTMUTE_4                0X10
#define TDA7541_WMTH_STARTMUTE_5                0X14
#define TDA7541_WMTH_STARTMUTE_6                0X18
#define TDA7541_WMTH_STARTMUTE_7                0X1C

//Tun_Buff[13]
#define TDA7541_ADDR13_REG_XTAL					    0,4
#define TDA7541_XTAL_24_75PF                        0x11
//Bit7~5
#define TDA7541_ADDR13_REG_SMETER					5,7
#define TDA7541_REG_SMETER_SLOPE_090V_20DB			0x00
#define TDA7541_REG_SMETER_SLOPE_095V_20DB			0x20
#define TDA7541_REG_SMETER_SLOPE_100V_20DB			0x40
#define TDA7541_REG_SMETER_SLOPE_105V_20DB			0x60
#define TDA7541_REG_SMETER_SLOPE_110V_20DB			0x80
#define TDA7541_REG_SMETER_SLOPE_115V_20DB			0xA0
#define TDA7541_REG_SMETER_SLOPE_120V_20DB			0xC0
#define TDA7541_REG_SMETER_SLOPE_125V_20DB			0xE0

//Tun_Buff[14]
//Bit3~0
#define  TDA7541_ADDR14_IF2A                    0,3
#define  TDA7541_IF2A_22PF                      0x0A
#define  TDA7541_IF2A_30_8PF                    0x0E
//Bit5~4
#define TDA7541_ADDR14_IF2Q						4,5
#define TDA7541_IF2Q_NOR_AMQ28_FMQ28			0x00
#define TDA7541_IF2Q_R5k_FMQ3					0x10
#define TDA7541_IF2Q_R4_1k_FMQ2_5				0x20
#define TDA7541_IF2Q_R3_3k_FMQ2_05				0x30
//Bit7~6
#define TDA7541_ADDR14_IF1G						6,7
#define TDA7541_IF2_GAIN_9						0x00
#define TDA7541_IF2_GAIN_12						0x40
#define TDA7541_IF2_GAIN_17						0x80
#define TDA7541_IF2_GAIN_21						0xC0

//Tun_Buff[15]
//Bit3~0
#define TDA7541_ADDR15_PH                       0,3
#define TDA7541_PH_3_DEGREE                     0x0A
//Bit7~4
#define	TDA7541_ADDR15_ORT_IQ					4,7

//Tun_Buff[18]
//Bit2~0
#define TDA7541_ADDR18_SBC						0,2
#define TDA7541_SBC_29_PERSENT					0x00
#define TDA7541_SBC_33_PERSENT					0x01
#define TDA7541_SBC_38_PERSENT					0x02
#define TDA7541_SBC_42_PERSENT					0x03
#define TDA7541_SBC_46_PERSENT					0x04
#define TDA7541_SBC_50_PERSENT					0x05
#define TDA7541_SBC_54_PERSENT					0x06
#define TDA7541_SBC_58_PERSENT					0x07


//Tun_Buff[19]
//Bit2~1---MAX_HIGH CUT
#define TDA7541_ADDR19_HIGH_CUT_MAX				1,2
#define TDA7541_HIGH_CUT_MAX_10DB				0x00
#define TDA7541_HIGH_CUT_MAX_5_5DB				0x01
#define TDA7541_HIGH_CUT_MAX_7_5DB				0x02
#define TDA7541_HIGH_CUT_MAX_8_5DB				0x03

// Bit7---DEEMP
#define TDA7541_ADDR19_DEEMP					7,7
#define TDA7541_DEEMP_50US						0x00
#define TDA7541_DEEMP_75US						0x80

//Tun_Buff[20]
//Bit6---MP FAST
#define TDA7541_ADDR20_MPFAST					6,6
#define TDA7541_MPTC_ENABLE						0x00
#define TDA7541_MPTC_DISABLE					0x40

//Tun_Buff[23]
//Bit5
#define TDA7541_ADDR23_VCON						5,5
#define TDA7541_STD_VCO_OFF						0x00
#define TDA7541_STD_VCO_ON						0x20

//Tun_Buff[25]
//Bit4
#define TDA7541_ADDR25_44						4,4


//#define 	EU			0
//#define		USA_WB		1
//#define 	OIRT		2
//#define 	JAPAN		3
//#define		SAM			4
enum enumRadioMode{FM1=0,FM2,AM};
enum enumRadioArea{AREA_EU=0,AREA_USA_WB,AREA_OIRT,AREA_JAPAN,AREA_SAM};
enum enumRadioStepDirection{STEP_BACKWARD=0,STEP_FORWARD,STEP_NONE};
enum enumRadioStepMode{STEP_MANUAL=0,STEP_SCAN};

typedef struct _FLY_RADIO_INFO
{
	UINT iFreqFMMin;
	UINT iFreqFMMax;
	UINT iFreqFMManualStep;
	UINT iFreqFMScanStep;

	UINT iFreqAMMin;
	UINT iFreqAMMax;
	UINT iFreqAMManualStep;
	UINT iFreqAMScanStep;

	BYTE ePreRadioMode;
	BYTE eCurRadioMode;
	BYTE ePreRadioArea;
	BYTE eCurRadioArea;

	UINT iPreRadioFreqFM1;
	UINT iCurRadioFreqFM1;
	UINT iPreRadioFreqFM2;
	UINT iCurRadioFreqFM2;
	UINT iPreRadioFreqAM;
	UINT iCurRadioFreqAM;

	UINT *pPreRadioFreq;
	UINT *pCurRadioFreq;

	BOOL bPreScaning;
	BOOL bCurScaning;
	BYTE eScanDirection;
	BOOL bScanRepeatFlag;

	BOOL bPreStepButtomDown;
	//BOOL bCurStepButtomDown;
	BYTE eButtomStepDirection;
	UINT iButtomStepCount;
}FLY_RADIO_INFO, *P_FLY_RADIO_INFO;

typedef struct _FLY_RDS_INFO
{
	BOOL RadioRDSAFControlOn;
	BOOL RadioRDSTAControlOn;

	UINT16 BaseReferencePI;
	UINT16 rdsdec_flag_recv[2];// 表示 正在播出的节目的GROUP 标志位 比如节目发了 0A 和 3B 组 此时rdsdec_flag_recv[0]的第一位置1 rdsdec_flag_recv[1]的第四位置1 		
	UINT16 rdsdec_flag_pi;	 //Programme Identification 每个节目只有唯一的PI 前四位是 地区编码
	//B15-B12	Country code
	//B11-B8	Programme type in terms of area coverage
	//B7-B0		Programme reference number
	BYTE  rdsdec_flag_pty; //0表示none，节目类型
	BYTE  rdsdec_flag_tp;	  //0表示没有携带，Traffic Programme (TP) Flag	 表示 节目带有 交通信息
	BYTE  rdsdec_flag_ms;	  //0语言类，1音乐类， music/speech 表示正在播出的节目是 音乐还是语音节目
	BYTE  rdsdec_flag_di[4];	 //	  Decoder Identification 解码标志位 暂时没有用到
	BYTE  rdsdec_flag_ta;	  //0表示无， Traffic Announcement (TP) Flag //TP 和 TA 同时为 1，表示Traffic report in progress on this service，此时关掉其他动作 听此报告

	BYTE  rdsdec_flag_eon;  //0表示无，表示有无Enhanced Other Networks (EON) 节目 暂时没有 做深入解码
	/* DEF_SUPPORT_0 */
	UINT16   rds_ecc;	    // Extended Country Code  根据它确定 某个国家 因为PI的前四位 可能是一些国家共用的
	UINT16   rds_language_code;	  //语言编码

	/* DEF_SUPPORT_0 */
	BYTE rdsdec_ps[2][8];   //Programme Service (PS) Name  节目名字
	BYTE rdsdec_ps_dis[8];
	//其实只有一组有效，当改变时往另一数组写数
	BYTE rdsdec_af[8];      //Alternative Frequency List—AF
	/* DEF_SUPPORT_2 */
	BYTE rdsdec_rt_a[2][64]; //RadioText (RT) 这是 GROUP_A的 TEXT_A 和 TEXT_B
	BYTE rdsdec_rt_a_dis[64];
	//其实只有一组有效，当改变时往另一数组写数
	BYTE rdsdec_rt_b[32]; // for Decode RadioText (RT) 这是 GROUP_B的 TEXT_A 和 TEXT_B
	//其实只有一组有效，当改变时往另一数组写数

	/* DEF_SUPPORT_4 */	   //以下是 RDS 的时间 校正
	UINT16 rds_clock_year;
	BYTE  rds_clock_month;
	BYTE  rds_clock_day;
	BYTE  rds_clock_wd; // weekday 1~7
	BYTE  rds_clock_hour;
	BYTE  rds_clock_min;
	BYTE  rds_clock_ofs;

	BYTE rdsdec_ptyn[2][8];   // for Decode 可编程的节目类型名
	//其实只有一组有效，当改变时往另一数组写数
}FLY_RDS_INFO, *P_FLY_RDS_INFO;

typedef struct _FLY_TDA7541_IO_INFO
{
	ULONG nowTimer;
	ULONG lastTimer;
	UINT iSSTOPDecCount;
}FLY_TDA7541_IO_INFO, *P_FLY_TDA7541_IO_INFO;

typedef struct flyradio_struct_info
{
	BOOL bPowerUp;
	BOOL bNeedInit;

	//读AD值
	BOOL bRadioADReturn;
	UINT RadioAD;
	
	BOOL bPreMute;
	BOOL bCurMute;

	//MAIN线程
	BOOL bKillRadioMainThread;
	sem_t           MainThread_sem;

	//SCAN线程
	BOOL bKillRadioScanThread;
	pthread_mutex_t ScanThreadMutex;
	pthread_cond_t  ScanThreadCond;
	BOOL bScanThreadRunAgain;

	//RDSREC线程
	BOOL bKillRadioRDSRecThread;
	pthread_mutex_t RDSRecThreadMutex;
	pthread_cond_t  RDSRecThreadCond;
	BOOL bRDSThreadRunAgain;
	//RDS信息
	BYTE rdsdec_buf[8];// Groupe = 4 blocks ;  8 chars

	BYTE mParameterTable[INIT_DATA_SIZE];

	FLY_TDA7541_IO_INFO TDA7541_IO_info;

	FLY_RADIO_INFO radioInfo;
	FLY_RDS_INFO RDSInfo;
}FLYRADIO_STRUCT_INFO,*P_FLYRADIO_STRUCT_INFO;

extern void readFromMmapPrintf(BYTE *buf,UINT length);

extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);

extern void *radio_rdsrec_thread(void *arg);
extern void flyRadioReturnToUserPutToBuff(BYTE *buf, UINT16 len);
extern void RDSParaInit(void);

extern void postRDSSignal(void);
extern int waitRDSSignedTimeOut(UINT32 iTimeOutMs);

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