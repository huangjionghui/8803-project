#ifndef  FLY_DVD_H_
#define  FLY_DVD_H_

#include <sched.h>
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include "../../include/global.h"

#define FORYOU_DVD_RESET_FLYER	0

#define FORYOU_DVD_BUG_FIX	1

#define SERIAL_BUF_MAX_LEN (1024*4)

#define SYNC_MISSING 0
#define SYNC_FOUND   1
#define SYNC_PARTIAL 2


typedef struct folder_file_tree_list{
	BOOL  bStorage;
	BOOL  bFolder;
	UINT16  parentFolderIndex;
	UINT16  extension;
	BYTE  name[256];
	UINT16  nameLength;
}FOLDER_FILE_TREE_LIST, *P_FOLDER_FILE_TREE_LIST;
typedef struct flyforyou_dvd_info{
			BOOL MechanismInitialize;
			ULONG bDeviceRec89;
	
			UINT16 DVDReqStep;
	
			BOOL LEDFlash;
			BOOL bLEDFlashAble;
	
			BOOL bFilterDiscInFirstPowerUp;
			BOOL bFilterDiscIn;
	
			BOOL bQuickJumpNext;
			UINT16 iQuickJumpNextCount;
			UINT16 iQuickJumpTitlePos;
			ULONG iQuickJumpNextTimer;
			
			BYTE CurrentMechanismDevice;//������ǰ�Ĳ����豸
			BYTE CurrentMechanismStatus[4];//�����֡�������״̬
			BYTE CurrentMechanismType[4];//��ʵ���ֵ������CurrentMechanismDevice
	
			BYTE CurrentReqMechanismCircle;
	
			BYTE MediaDiscType;//���������CD����Ϣ//ý����Ϣ״̬������MP3��Ƭ
			BYTE MediaFileType;
			BYTE MediaVideoInfo;
			BYTE MediaAudioSampleFrequency;
			BYTE MediaAudioCoding;//���������CD����Ϣ
	
			BYTE preVideoAspect;
			BYTE curVideoAspect;
	
			UINT16 CurrentTitle;//���������D2����Ϣ
			UINT16 CurrentChar;
			UINT16 TotalTitle;
			UINT16 TotalChar;
			UINT16 EscapeHour;
			UINT16 EscapeMinute;
			UINT16 EscapeSecond;
			UINT16 TotalHour;
			UINT16 TotalMinute;
			UINT16 TotalSecond;
			BYTE PlayMode;
			BYTE AudioType;
			BYTE PlaySpeed;
			BYTE PlayStatus;
			BYTE DVDRoot;
			BYTE DeviceStatus;//������ȡ���״̬
			BYTE DeviceType;
			BYTE HaveDisc;
			BYTE HaveUSB;
			BYTE HaveSD;//���������D2����Ϣ
			
			BOOL bFinishGetFileFolderEB;
			BOOL bRecFileFolderUseEB;
			
			BOOL pBStartGetFolderFile;
			BOOL pBHaveGetFolderFile;
			BYTE pStartGetFolderFileCount;
			BOOL pReturnE8;
			BOOL pReturnEAFolder;
			BOOL pReturnEAFile;
			UINT16 pFolderCount;
			UINT16 pFileCount;
			UINT16 pRecFolderCount;
			UINT16 pRecFileCount;
			UINT16 pLastRecFolderIndex;
			UINT16 pLastRecFileIndex;
			BOOL pBReqFolderFileCommandActiveNow;
			BOOL pBFolderFileListFolderErrorCheck;
			BOOL pBFolderFileListFileErrorCheck;
			BOOL pBGetFolderFinish;
			BOOL pBGetFileFinish;
			BOOL pBGetFileFolderFinish;
			P_FOLDER_FILE_TREE_LIST pFileTreeList;
			P_FOLDER_FILE_TREE_LIST pFolderTreeList;
			UINT16 pNowInWhatFolder;//�����ã���ǰ���ĸ�Ŀ¼��
			UINT16 pNeedReturnStart;//������ʼ
			UINT16 pNeedReturnCount;//��������
			UINT16 pNowReturnCount;//�ѷ�������
			UINT16 pNowReturnPlayingFileCount;
			UINT16 pNowPlayingInWhatFolder;
	
			BOOL bRecE0AndNeedProc;
			ULONG bNeedSend8CTime;
	
			BYTE iDVDReturnRegionCode;
	
			BYTE iSoftwareVersion[6];
	
			BYTE bDVDRequestState;
			BYTE bDVDResponseState;
			ULONG iDVDStateCheckTime;

#if FORYOU_DVD_BUG_FIX
			BOOL bNeedReturnNoDiscAfterClose;
			ULONG iNeedReturnNoDiscAfterCloseTime;
#endif
}FORYOU_DVD, *P_FORYOU_DVD;

struct flydvd_struct_info{

	BOOL bPower;
	BOOL bPowerUp;

	INT flydvd_fd;

	//�����ڴ�
	FLY_SHARE_MEMORY_DVD_DATA shareMemoryDVDData;	
	
	//�������߳�
	UINT32 DVDInfoReadBuffLength;
	BYTE DVDInfoReadBuff[SERIAL_BUF_MAX_LEN];
	BOOL bKillReadSerialThread;

	//�������߳�
	BOOL bKillSearchDataThread;
	pthread_mutex_t searchDataMutex;
	pthread_cond_t  searchDataCond; 
	BOOL bSearchDataThreadRunAgain;

	//���������߳�
	BOOL bKillLEDFlashThread;
	sem_t           LED_Flash_sem;
					
	BOOL bFlyaudioDVD;
	BOOL bEnterUpdateMode;
		
	struct flyforyou_dvd_info sForyouDVDInfo;
	BOOL bAutoResetControlOn;
	ULONG iAutoResetControlTime;
	volatile BOOL bDVDReturnDataToJNIResume;
};

extern void flyInitDeviceStruct(void);
extern void flyDestroyDeviceStruct(void);
extern INT  flyOpenDevice(void);
extern INT  flyCloseDevice(void);
extern void flyCommandProcessor(BYTE *buf, UINT16 len);
extern INT  flyReadData(BYTE *buf, UINT16 len);


extern void Sleep(UINT32 sTime);
extern ULONG GetTickCount(void);
extern void DVD_Reset_On(struct flydvd_struct_info *pForyouDVDInfo);
extern void DVD_Reset_Off(struct flydvd_struct_info *pForyouDVDInfo);
extern void DVD_LEDControl_On(struct flydvd_struct_info *pForyouDVDInfo);
extern void DVD_LEDControl_Off(struct flydvd_struct_info *pForyouDVDInfo);

extern void returnDVDDevicePlayDevice(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDevice);
extern void returnDVDDeviceActionState(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDevice,BYTE iState);
extern void returnDVDDeviceContent(struct flydvd_struct_info *pForyouDVDInfo,BYTE iContent);
extern void returnDVDDeviceMedia(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDisc,BYTE iFile);
extern void returnDVDTotalTitleTrack(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iTitle,UINT16 iTrack);
extern void returnDVDCurrentTitleTrack(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iTitle,UINT16 iTrack);
extern void returnDVDTotalTime(struct flydvd_struct_info *pForyouDVDInfo,BYTE iHour,BYTE iMin,BYTE iSec);
extern void returnDVDCurrentTime(struct flydvd_struct_info *pForyouDVDInfo,BYTE iHour,BYTE iMin,BYTE iSec);
extern void returnDVDPlayStatusSpeed(struct flydvd_struct_info *pForyouDVDInfo,BYTE iPlayStatus,BYTE iPlaySpeed);
extern void returnDVDPlayMode(struct flydvd_struct_info *pForyouDVDInfo,BYTE iPlayMode);
extern void returnDVDCurrentFolderInfo(struct flydvd_struct_info *pForyouDVDInfo,UINT16 totalCount,UINT16 folderCount,BOOL bRoot);
extern void returnDVDFileFolderInfo(struct flydvd_struct_info *pForyouDVDInfo, BOOL bFolder,BYTE *pName,UINT16 nameLength,UINT16 index,UINT16 globalIndex);
extern void returnDVDNowPlayingFileInfo(struct flydvd_struct_info *pForyouDVDInfo,P_FOLDER_FILE_TREE_LIST p,UINT16 index,UINT16 globalIndex);
extern void returnDVDNowPlayingInFolderName(struct flydvd_struct_info *pForyouDVDInfo,P_FOLDER_FILE_TREE_LIST p);
extern void returnDVDErrorStatus(struct flydvd_struct_info *pForyouDVDInfo,BYTE iError);
extern void returnDVDVersion(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len);
extern void returnDVDDevicePowerMode(struct flydvd_struct_info *pForyouDVDInfo,BYTE bPower);
extern void returnDVDDeviceWorkMode(struct flydvd_struct_info *pForyouDVDInfo,BYTE bWork);


extern void control_DVD_Video_Aspect_Radio(struct flydvd_struct_info *pForyouDVDInfo,BYTE para);
extern void control_DVD_Set_View_Mode(struct flydvd_struct_info *pForyouDVDInfo,BYTE para);
extern void control_DVD_PlayBack_DisplayInfo_State_Request(struct flydvd_struct_info *pForyouDVDInfo,BOOL bOn);
extern void control_DVD_IR_CMD(struct flydvd_struct_info *pForyouDVDInfo,BYTE IRCMD);
extern void control_DVD_ReqMechanismState(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDevice);
extern void control_DVD_ReqMediaState(struct flydvd_struct_info *pForyouDVDInfo);
extern void control_DVD_ReqDVDSoftwareVersion(struct flydvd_struct_info *pForyouDVDInfo);
extern void control_DVD_ReqFileFolderCount(struct flydvd_struct_info *pForyouDVDInfo);
extern void control_DVD_ReqFileFolderDetailedInfo(struct flydvd_struct_info *pForyouDVDInfo,BYTE bFile,UINT16 iAbs,UINT16 iOffset);
extern void control_DVD_PlayFileByAbsCount(struct flydvd_struct_info *pForyouDVDInfo,UINT16 parentIndex,UINT16 index);
extern void control_DVD_ReqHighLightFileIndex(struct flydvd_struct_info *pForyouDVDInfo);
extern void control_DVD_ReqRegionCode(struct flydvd_struct_info *pForyouDVDInfo);
extern void control_DVD_ControlRegionCode(struct flydvd_struct_info *pForyouDVDInfo,BYTE iRegionCode);
extern void control_DVD_VideoSetup(struct flydvd_struct_info *pForyouDVDInfo);
extern void control_DVD_JumpNextN(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iJumpN);
extern void control_DVD_JumpPrevN(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iJumpN);
extern void structDVDInfoInit(struct flydvd_struct_info *pForyouDVDInfo,BOOL bInitAll);
extern void control_DVD_ReqDVDSoftwareVersion(struct flydvd_struct_info *pForyouDVDInfo);

extern UINT16 getListFolderFileSelectParentIndex(struct flydvd_struct_info *pForyouDVDInfo,UINT16 index);
extern UINT16 getSelectParentFolderFileCount(struct flydvd_struct_info *pForyouDVDInfo,UINT16 parentIndex, BOOL bFolder);
extern INT getSelectParentFolderFileInfoBySubIndex(struct flydvd_struct_info *pForyouDVDInfo,UINT16 parentIndex, BOOL bFolder, UINT16 subIndex);
extern UINT16 getSelectFolderFileIndexByGlobalIndexInParent(struct flydvd_struct_info *pForyouDVDInfo,BOOL bFolder,UINT16 globalIndex);
extern UINT16 selectFolderOrFileByLocalIndex(struct flydvd_struct_info *pForyouDVDInfo,INT inWhatFolder,UINT16 localIndex,BOOL bHaveFolderIndex);
extern void listFileFolderClearAll(struct flydvd_struct_info *pForyouDVDInfo,BYTE bFile);
extern void listFileFolderNewAll(struct flydvd_struct_info *pForyouDVDInfo,BYTE bFile, UINT16 iCount);
extern void listFileFolderStorageOne(struct flydvd_struct_info *pForyouDVDInfo,BOOL bFile, 
			UINT16 iCount, UINT16 parentIndex, UINT16 extension, BYTE *name, UINT16 nameLength);
			
extern void DealFlyaudioInfo(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT length);
extern void processFlyaudioDVD(struct flydvd_struct_info *pForyouDVDInfo,ULONG WaitReturn);
extern void procDVDInfoE0(struct flydvd_struct_info *pForyouDVDInfo);
extern void returnDVDUpdateStatus(struct flydvd_struct_info *pForyouDVDInfo,BOOL bUpdater);

extern void readFromhardwareProc(BYTE *buf,UINT length);


#define DVD_DEBUG 1

#define DEBUG_MSG_ON 0
#if DEBUG_MSG_ON 

#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(1){CODE}
#define DBG2(CODE) if(1){CODE}
#define DBG3(CODE) if(1){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel>2){CODE}
#else
#define DBG0(CODE) if(1){CODE}
#define DBG1(CODE) if(0){CODE}
#define DBG2(CODE) if(0){CODE}
#define DBG3(CODE) if(0){CODE}

//#define DBG1(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel>0){CODE}
//#define DBG2(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel>1){CODE}
//#define DBG3(CODE) if(pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel>2){CODE}
#endif

#endif
