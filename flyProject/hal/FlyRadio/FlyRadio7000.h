#ifndef FLY_RADIO_H_
#define FLY_RADIO_H_

#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/types_def.h"

#define RADIO_RDS 1
#define RADIO_RDS_DEBUG_MSG 1

//RDS
#define RADIORDS_ID	    0x03

#define RADIORDS_ID_TA	0x12
#define RADIORDS_ID_PS	0x13
#define RADIORDS_ID_PTY	0x14
#define RADIORDS_ID_RT	0x15
#define RADIORDS_ID_AF	0x16

//#define SAF7741_ADDR_R     0x39
//#define SAF7741_ADDR_W     0x38
//
//#define TEF7000_1_ADDR_R   0xC138
//#define TEF7000_1_ADDR_W   0xC038
//
//#define TEF7000_2_ADDR_R   0xC538
//#define TEF7000_2_ADDR_W   0xC438

//SCAN STOP
#define FM_SCAN_STOP_LEVEL          22
#define FM_SCAN_STOP_NOISE          20
#define FM_SCAN_STOP_OFS            5
#define AM_SCAN_STOP_LEVEL          28

#define GROUP_A     0
#define GROUP_B     1

enum enumRadioMode{FM1=0,FM2,AM};
enum enumRadioArea{AREA_EU=0,AREA_USA_WB,AREA_OIRT,AREA_JAPAN,AREA_SAM};
enum enumRadioStepDirection{STEP_BACKWARD=0,STEP_FORWARD,STEP_NONE};
enum enumRadioStepMode{STEP_MANUAL=0,STEP_SCAN};

//typedef struct user_read_struct_info{
//	BYTE  buffToUser[USER_BUF_LEN];
//};

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
	UINT16 rdsdec_flag_recv[2];// ��ʾ ���ڲ����Ľ�Ŀ��GROUP ��־λ �����Ŀ���� 0A �� 3B �� ��ʱrdsdec_flag_recv[0]�ĵ�һλ��1 rdsdec_flag_recv[1]�ĵ���λ��1 		
	UINT16 rdsdec_flag_pi;	 //Programme Identification ÿ����Ŀֻ��Ψһ��PI ǰ��λ�� ��������
	//B15-B12	Country code
	//B11-B8	Programme type in terms of area coverage
	//B7-B0		Programme reference number
	BYTE  rdsdec_flag_pty; //0��ʾnone����Ŀ����
	BYTE  rdsdec_flag_tp;	  //0��ʾû��Я����Traffic Programme (TP) Flag	 ��ʾ ��Ŀ���� ��ͨ��Ϣ
	BYTE  rdsdec_flag_ms;	  //0�����࣬1�����࣬ music/speech ��ʾ���ڲ����Ľ�Ŀ�� ���ֻ���������Ŀ
	BYTE  rdsdec_flag_di[4];	 //	  Decoder Identification �����־λ ��ʱû���õ�
	BYTE  rdsdec_flag_ta;	  //0��ʾ�ޣ� Traffic Announcement (TP) Flag //TP �� TA ͬʱΪ 1����ʾTraffic report in progress on this service����ʱ�ص��������� ���˱���

	BYTE  rdsdec_flag_eon;  //0��ʾ�ޣ���ʾ����Enhanced Other Networks (EON) ��Ŀ ��ʱû�� ���������
	/* DEF_SUPPORT_0 */
	UINT16   rds_ecc;	    // Extended Country Code  ������ȷ�� ĳ������ ��ΪPI��ǰ��λ ������һЩ���ҹ��õ�
	UINT16   rds_language_code;	  //���Ա���

	/* DEF_SUPPORT_0 */
	BYTE rdsdec_ps[2][8];   //Programme Service (PS) Name  ��Ŀ����
	BYTE rdsdec_ps_dis[8];
	//��ʵֻ��һ����Ч�����ı�ʱ����һ����д��
	BYTE rdsdec_af[8];      //Alternative Frequency List��AF
	/* DEF_SUPPORT_2 */
	BYTE rdsdec_rt_a[2][64]; //RadioText (RT) ���� GROUP_A�� TEXT_A �� TEXT_B
	BYTE rdsdec_rt_a_dis[64];
	//��ʵֻ��һ����Ч�����ı�ʱ����һ����д��
	BYTE rdsdec_rt_b[32]; // for Decode RadioText (RT) ���� GROUP_B�� TEXT_A �� TEXT_B
	//��ʵֻ��һ����Ч�����ı�ʱ����һ����д��

	/* DEF_SUPPORT_4 */	   //������ RDS ��ʱ�� У��
	UINT16 rds_clock_year;
	BYTE  rds_clock_month;
	BYTE  rds_clock_day;
	BYTE  rds_clock_wd; // weekday 1~7
	BYTE  rds_clock_hour;
	BYTE  rds_clock_min;
	BYTE  rds_clock_ofs;

	BYTE rdsdec_ptyn[2][8];   // for Decode �ɱ�̵Ľ�Ŀ������
	//��ʵֻ��һ����Ч�����ı�ʱ����һ����д��
}FLY_RDS_INFO, *P_FLY_RDS_INFO;

//typedef struct _FLY_TDA7541_IO_INFO
//{
//	ULONG nowTimer;
//	ULONG lastTimer;
//	UINT iSSTOPDecCount;
//}FLY_TDA7541_IO_INFO, *P_FLY_TDA7541_IO_INFO;

typedef struct flyradio_struct_info
{
	BOOL bOpen;
	BOOL bPowerUp;
	
	BOOL bPreMute;
	BOOL bCurMute;

	//MAIN�߳�
	BOOL bKillRadioMainThread;
	sem_t           MainThread_sem;

	//SCAN�߳�
	BOOL bKillRadioScanThread;
	pthread_mutex_t ScanThreadMutex;
	pthread_cond_t  ScanThreadCond;

	BOOL bScanThreadRunAgain;
	BOOL bRDSThreadGO;

	//RDSREC�߳�
	BOOL bKillRadioRDSRecThread;
	pthread_mutex_t RDSRecThreadMutex;
	pthread_cond_t  RDSRecThreadCond;
	BOOL bRDSThreadRunAgain;
	//RDS��Ϣ
	BYTE rdsdec_buf[8];// Groupe = 4 blocks ;  8 chars

	////RDS��Ϣ
	//BYTE rdsdec_buf[8];// Groupe = 4 blocks ;  8 chars

	//FLY_TDA7541_IO_INFO TDA7541_IO_info;

	FLY_RADIO_INFO radioInfo;
	FLY_RDS_INFO RDSInfo;
}FLYRADIO_STRUCT_INFO,*P_FLYRADIO_STRUCT_INFO;

extern void readFromMmapPrintf(BYTE *buf,UINT length);
extern BOOL I2C_Read_SAF7741(UINT ulRegAddr, BYTE *pRegValBuf, UINT uiValBufLen);
extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT len);
extern INT  flyReadData(BYTE *buf, UINT len);

extern void *radio_rdsrec_thread(void *arg);
extern void flyRadioReturnToUserPutToBuff(BYTE *buf, UINT16 len);
extern void RDSParaInit(void);

extern void FlyRadio_SAF7741_TEF7000_Init(void);
extern void FlyRadio_ChangeToFMAM(BYTE mode);
extern void FlyRadio_Set_Freq(BYTE mode,UINT freq);

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