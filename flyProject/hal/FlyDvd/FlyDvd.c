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
#include <unistd.h>

#define LOCAL_HAL_ID		HAL_DEVICE_NAME_DVD
#define LOCAL_HAL_NAME		"flydvd Stub"
#define LOCAL_HAL_AUTOHR	"FlyAudio"
#define CURRENT_SHARE_MEMORY_ID	SHARE_MEMORY_DVD

#define SERIAL_NAME			"/dev/tcc-uart3"    
#define SERIAL_BAUDRATE      57600     

#include "code_data_offset_all.h"
#include "FlyDvd.h"

struct flydvd_struct_info *pForyouDVDInfo = NULL;


#if DVD_DEBUG
#define DVD_DEBUG_FILE_PATH "/data/fly/.dvdfile"
#include "dvd_debug.c"	
#endif 

#include "../../include/allInOneOthers.c"
#include "../../include/commonFunc.c"
#include "../../include/HalApi.c"
#include "../../include/serial.c"


static unsigned short get_unicode(unsigned short int gb) 
{ 
	return gb_uni_table[gb];
} 

static int gb2312_to_unicode(unsigned short int *unicode, BYTE *gb2312, UINT32 len) 
{ 
	UINT32 i,j; 
	unsigned short iTmp; 
	 
	for(i=0,j=0;i<len;j++) 
	{ 
		if ((unsigned char)gb2312[i]<=0x80) 
		{ 
			unicode[j]=gb2312[i]; 
			i++; 
		}
		else 
		{ 
			unicode[j]=get_unicode(*(unsigned short int*)(gb2312+i)); 
			i+=2; 
		} 
	} 
	return j*2; 
} 


void readFromhardwareProc(BYTE *buf,UINT length)
{
	//DBG0(debugBuf("AudioHAL read from hardware",buf,length);)
}

void ipcEventProcProc(UINT32 sourceEvent)
{
	if (ipcWhatEventOn(EVENT_AUTO_CLR_STANDBY_ID))
	{
		if (pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)
		{
			control_DVD_IR_CMD(pForyouDVDInfo, 0x17);//STOP

			DBG0(debugString("\n---=======))))))))");)
#if DVD_DEBUG
			dvd_debugString("\n---=======))))))))");
#endif
			Sleep(1000);
			DVD_Reset_On(pForyouDVDInfo);
		}
		else
		{
			DBG0(debugString("\nDVD-HAL EVENT_AUTO_CLR_STANDBY_ID");)
#if DVD_DEBUG
			dvd_debugString("\nDVD-HAL EVENT_AUTO_CLR_STANDBY_ID");
#endif 
			DVD_Reset_On(pForyouDVDInfo);
			Sleep(100);
			DVD_Reset_Off(pForyouDVDInfo);
		}
	}
	
	switch (sourceEvent)
	{
		case EVENT_AUTO_CLR_SUSPEND_ID:
			control_DVD_IR_CMD(pForyouDVDInfo, 0x17);//STOP

			debugString("\n EVENT_AUTO_CLR_SUSPEND_ID");
#if DVD_DEBUG
			dvd_debugString("\n EVENT_AUTO_CLR_SUSPEND_ID");
#endif 
			pForyouDVDInfo->bDVDReturnDataToJNIResume = FALSE;
			break;
			
		case EVENT_AUTO_CLR_RESUME_ID:
			tcflush(pForyouDVDInfo->flydvd_fd, TCIFLUSH);
			pForyouDVDInfo->bDVDReturnDataToJNIResume = TRUE;
			break;
			
		default:
			break;
	}
		
	PostSignal(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain);
}

void DVD_Reset_On(struct flydvd_struct_info *pForyouDVDInfo)
{

	BYTE buff[] = {SHARE_MEMORY_DVD,MSG_DVD_CON_RESET_ON};
	writeDataToHardware(buff, sizeof(buff));
	
	pForyouDVDInfo->bAutoResetControlOn = FALSE;
	pForyouDVDInfo->iAutoResetControlTime = GetTickCount();
	DBG0(debugString("ForyouDVD Reset On\n");)
#if DVD_DEBUG
	dvd_debugString("ForyouDVD Reset On\n");
#endif 
}

void DVD_Reset_Off(struct flydvd_struct_info *pForyouDVDInfo)
{
	BYTE buff[] = {SHARE_MEMORY_DVD,MSG_DVD_CON_RESET_OFF};
	writeDataToHardware(buff, sizeof(buff));

	pForyouDVDInfo->bAutoResetControlOn = TRUE;
	pForyouDVDInfo->iAutoResetControlTime = GetTickCount();
	DBG0(debugString("ForyouDVD Reset Off\n");)
#if DVD_DEBUG
	dvd_debugString("ForyouDVD Reset Off\n");
#endif 
}

void DVD_LEDControl_On(struct flydvd_struct_info *pForyouDVDInfo)
{
	BYTE buff[] = {SHARE_MEMORY_DVD,MSG_DVD_CON_LIGHT,1};
	writeDataToHardware(buff, sizeof(buff));
	
	DBG0(debugString("ForyouDVD LEDControl On\n");)
}

void DVD_LEDControl_Off(struct flydvd_struct_info *pForyouDVDInfozz)
{
	BYTE buff[] = {SHARE_MEMORY_DVD,MSG_DVD_CON_LIGHT,0};
	writeDataToHardware(buff, sizeof(buff));
	
	DBG0(debugString("ForyouDVD LEDControl Off\n");)
}

void listFileFolderClearAll(struct flydvd_struct_info *pForyouDVDInfo,BYTE bFile)
{
	if (bFile)
	{
		if (pForyouDVDInfo->sForyouDVDInfo.pFileTreeList)
		{
			free(pForyouDVDInfo->sForyouDVDInfo.pFileTreeList);
			pForyouDVDInfo->sForyouDVDInfo.pFileTreeList = NULL;
		}
	}
	else
	{
		if (pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList)
		{
			free(pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList);
			pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList = NULL;
		}
	}
}
void listFileFolderNewAll(struct flydvd_struct_info *pForyouDVDInfo,BYTE bFile, UINT16 iCount)
{
	UINT16 i = 0;
	
	if (bFile)
	{
		listFileFolderClearAll(pForyouDVDInfo, TRUE);
		pForyouDVDInfo->sForyouDVDInfo.pFileTreeList = 
			(struct folder_file_tree_list *)malloc(sizeof(struct folder_file_tree_list)*iCount);		
		for (i=0; i<iCount; i++)
		{
			pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[i].bStorage = FALSE;
		}
	}
	else
	{
		listFileFolderClearAll(pForyouDVDInfo, FALSE);
		pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList = 
			(struct folder_file_tree_list *)malloc(sizeof(struct folder_file_tree_list)*iCount);
		for (i=0; i<iCount; i++)
		{
			pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[i].bStorage = FALSE;
		}
	}
}
void listFileFolderStorageOne(struct flydvd_struct_info *pForyouDVDInfo,BOOL bFile, UINT16 iCount, 
				UINT16 parentIndex, UINT16 extension, BYTE *name, UINT16 nameLength)
{
	UINT16 i = 0;
	BYTE temp = 0;
	unsigned short int sWideChar[512];
	UINT iWideCharLength;
	if (bFile)
	{
		if (iCount < pForyouDVDInfo->sForyouDVDInfo.pFileCount)
		{
			pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].bStorage = TRUE;
			pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].bFolder  = FALSE;
			pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].parentFolderIndex = parentIndex;
			pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].extension = extension;
			
			if (nameLength < 256-1)
			{
				if (!pForyouDVDInfo->bFlyaudioDVD)//华阳碟机要交换一下
				{
					memcpy(pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name, name, nameLength);
					for (i=0; i<nameLength; i+=2)
					{
						temp = pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i];
						pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i] = 
							pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i+1];
						pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i+1] = temp;
					}

					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[nameLength]   = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[nameLength+1] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].nameLength = nameLength+2;
				}
				else
				{
					memcpy(pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name,name,nameLength);
					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[nameLength] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[nameLength+1] = 0;
					
					iWideCharLength = gb2312_to_unicode(sWideChar
						,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name,nameLength);

					//debugBuf("\nDVD Src",pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name,nameLength);
					//debugBuf("\nDVD Obj",sWideChar,iWideCharLength);

					memcpy(pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name,sWideChar,iWideCharLength);
					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[iWideCharLength] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[iWideCharLength+1] = 0;

					pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].nameLength = iWideCharLength + 2;
				}
			}
			else
			{
				pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].nameLength = 0;
			}
		}
	}
	else
	{
		if (iCount < pForyouDVDInfo->sForyouDVDInfo.pFolderCount)
		{
			pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].bStorage = TRUE;
			pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].bFolder  = TRUE;
			pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].parentFolderIndex = parentIndex;
			pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].extension = 0xFF;
			
			if (nameLength < 256-1)
			{
				if (!pForyouDVDInfo->bFlyaudioDVD)//华阳碟机要交换一下
				{
					memcpy(pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name, name, nameLength);
					for (i=0; i<nameLength; i+=2)
					{
						temp = pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[i];
						pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[i] = 
							pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[i+1];
						pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[i+1] = temp;
					}
					
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[nameLength]   = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[nameLength+1] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].nameLength = nameLength+2;
				}
				else
				{
					memcpy(pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name,name,nameLength);
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[nameLength] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[nameLength+1] = 0;

					iWideCharLength = gb2312_to_unicode(sWideChar
						,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name,nameLength);

					memcpy(pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name,sWideChar,iWideCharLength);
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[iWideCharLength] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].name[iWideCharLength+1] = 0;
					pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].nameLength = iWideCharLength + 2;
				}
			}
			else
			{
				pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[iCount].nameLength = 0;
			}
		}
	}
}

UINT16 getListFolderFileSelectParentIndex(struct flydvd_struct_info *pForyouDVDInfo,UINT16 index)
{
	UINT16 returnIndex = 0;

	if (0 == index)
	{
	}
	else
	{
		if (index < pForyouDVDInfo->sForyouDVDInfo.pFolderCount)
		{
			if (pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[index].bStorage)
			{
				returnIndex = pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[index].parentFolderIndex;
			}
		}
	}
	return returnIndex;
}

//返回同目录下文件或文件夹数量
 UINT16 getSelectParentFolderFileCount(struct flydvd_struct_info *pForyouDVDInfo,UINT16 parentIndex, BOOL bFolder)
{
	UINT16 i = 0;
	UINT16 iCount = 0;
	
	if (bFolder)
	{
		for (i=0; i<pForyouDVDInfo->sForyouDVDInfo.pFolderCount; i++)
		{
			if (pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[i].parentFolderIndex == parentIndex)
			{
				iCount++;
			}
		}
	}
	else
	{
		for (i=0; i<pForyouDVDInfo->sForyouDVDInfo.pFileCount; i++)
		{
			if (pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[i].parentFolderIndex == parentIndex)
			{
				iCount++;
			}
		}
	}
	
	return iCount;
}

//根据目录和子序号，返回全局序号
INT getSelectParentFolderFileInfoBySubIndex(struct flydvd_struct_info *pForyouDVDInfo,UINT16 parentIndex, BOOL bFolder, UINT16 subIndex)
{
	UINT16 i = 0;
	UINT16 iCount = 0;
	
	if (bFolder)
	{
		for (i=0; i<pForyouDVDInfo->sForyouDVDInfo.pFolderCount; i++)
		{
			if (pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[i].parentFolderIndex == parentIndex)
			{
				if (iCount == subIndex)
				{
					return i;
				}
				iCount++;
			}
		}
	}
	else
	{
		for (i=0; i<pForyouDVDInfo->sForyouDVDInfo.pFileCount; i++)
		{
			if (pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[i].parentFolderIndex == parentIndex)
			{
				if (iCount == subIndex)
				{
					return i;
				}
				iCount++;
			}
		}
	}
	
	return -1;
}

 UINT16 getSelectFolderFileIndexByGlobalIndexInParent(struct flydvd_struct_info *pForyouDVDInfo,BOOL bFolder,UINT16 globalIndex)//根据全局序号，返回同目录下的子序号,此处特殊和协议的上一级目录序号有关
 {
	 UINT16 i;
	 UINT16 iParent;
	 UINT16 iFolderCount = 0;
	 UINT16 iFileCount = 0;
	
	 if (bFolder)
	 {
		 iParent = pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[globalIndex].parentFolderIndex;
		 for (i = 0;i < pForyouDVDInfo->sForyouDVDInfo.pFolderCount;i++)
		 {
			 if (i == globalIndex)
			 {
				 if (iParent)
				 {
					 return iFolderCount + 1;
				 }
				 else
				 {
					 return iFolderCount;
				 }
			 }
			 if (iParent == pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[i].parentFolderIndex)
			 {
				 iFolderCount++;
			 }
		 }
	 }
	 else
	 {
		 iParent = pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[globalIndex].parentFolderIndex;
		 iFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, iParent,TRUE);
		 for (i = 0;i < pForyouDVDInfo->sForyouDVDInfo.pFileCount;i++)
		 {
			 if (i == globalIndex)
			 {
				 if (iParent)
				 {
					 return iFolderCount + iFileCount + 1;
				 }
				 else
				 {
					 return iFolderCount + iFileCount;
				 }
			 }
			 if (iParent == pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[i].parentFolderIndex)
			 {
				 iFileCount++;
			 }
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
 static void flyAudioReturnToUser(struct flydvd_struct_info *pForyouDVDInfo,BYTE *buf, UINT16 len)
 {
	 UINT dwLength;
	 
	if (!pForyouDVDInfo->bDVDReturnDataToJNIResume)
		return;

	 dwLength = writeToJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	 if (dwLength)
	 {
		 DBG1(debugBuf("\nDVD-HAL write  bytes to User OK:", buf,len);)
#if DVD_DEBUG
		dvd_debugBuf("\nDVD-HAL write  bytes to User OK:", buf,len);
#endif 
	 }
	 else
	 {
#if DVD_DEBUG
		dvd_debugBuf("\nDVD-HAL write  bytes to User Error:", buf,len);
#endif 
		 DBG1(debugBuf("\nDVD-HAL write  bytes to User Error:", buf,len);)
	 }
 }

 void returnDVDVersion(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len)
{
	BYTE buff[18];
	buff[0] = 0x37;
	memcpy(&buff[1],p,len);
	flyAudioReturnToUser(pForyouDVDInfo, buff,len+1);
}
  void returnDVDDevicePowerMode(struct flydvd_struct_info *pForyouDVDInfo,BYTE bPower)
 {
	BYTE buff[2];
	
	buff[0] = 0x01;
	buff[1] = bPower;
	
#if DVD_DEBUG
	dvd_debugString("\ndvd returnDVDDevicePowerMode");
#endif
 
	flyAudioReturnToUser(pForyouDVDInfo, buff, 2);
 }
  void returnDVDDeviceWorkMode(struct flydvd_struct_info *pForyouDVDInfo,BYTE bWork)
 {
	BYTE buff[2];
	
	buff[0] = 0x02;
	buff[1] = bWork;
	
	flyAudioReturnToUser(pForyouDVDInfo, buff, 2);
 }

 
 
 void returnDVDDevicePlayDevice(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDevice)
 {
	 BYTE buff[2];
	 buff[0] = 0x20;
	 buff[1] = iDevice;
	 flyAudioReturnToUser(pForyouDVDInfo, buff,2);
 }
 
 void returnDVDDeviceActionState(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDevice,BYTE iState)
 {
	 BYTE buff[3];
	 buff[0] = 0x21;
	 buff[1] = iDevice;
	 buff[2] = iState;
	 flyAudioReturnToUser(pForyouDVDInfo, buff,3);
 }
 
 void returnDVDDeviceContent(struct flydvd_struct_info *pForyouDVDInfo,BYTE iContent)
 {
	 BYTE buff[2];
	 buff[0] = 0x22;
	 buff[1] = iContent;
	 flyAudioReturnToUser(pForyouDVDInfo, buff,2);
 }
 
 void returnDVDDeviceMedia(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDisc,BYTE iFile)
 {
	 BYTE buff[3];
	 buff[0] = 0x23;
	 buff[1] = iDisc;buff[2] = iFile;
	 
	flyAudioReturnToUser(pForyouDVDInfo, buff,3);
 }
 
 void returnDVDTotalTitleTrack(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iTitle,UINT16 iTrack)
 {
	 BYTE buff[5];
	 buff[0] = 0x24;
	 if (iTitle == 0xFF || iTrack == 0xFFFF)
	 {
		 buff[1] = 0;buff[2] = 0;buff[3] = 0;buff[4] = 0;
	 } 
	 else
	 {
		 buff[1] = iTitle >> 8;buff[2] = iTitle;buff[3] = iTrack >> 8;buff[4] = iTrack;
	 }
	 flyAudioReturnToUser(pForyouDVDInfo, buff,5);
 }
 
 void returnDVDCurrentTitleTrack(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iTitle,UINT16 iTrack)
 {
	 BYTE buff[5];
	 buff[0] = 0x25;
	 if (iTitle == 0xFF)
	 {
		 iTitle = 0;
	 }
	 if (iTrack == 0xFFFF)
	 {
		 iTrack = 0;
	 }
 
	 buff[1] = iTitle >> 8;buff[2] = iTitle;buff[3] = iTrack >> 8;buff[4] = iTrack;
 
	 if (!pForyouDVDInfo->bFlyaudioDVD
		 && 0 == iTitle && 0 == iTrack)
	 {
		 return;
	 }
	 flyAudioReturnToUser(pForyouDVDInfo, buff,5);
 }
 
 void returnDVDTotalTime(struct flydvd_struct_info *pForyouDVDInfo,BYTE iHour,BYTE iMin,BYTE iSec)
 {
	 BYTE buff[4];
	 buff[0] = 0x26;
	 if (iHour == 86 && iMin == 45 && iSec == 19)
	 {
		 buff[1] = 0;buff[2] = 0;buff[3] = 0;
	 } 
	 else
	 {
		 buff[1] = iHour;buff[2] = iMin;buff[3] = iSec;
	 }
	 flyAudioReturnToUser(pForyouDVDInfo, buff,4);
 }
 
 void returnDVDCurrentTime(struct flydvd_struct_info *pForyouDVDInfo,BYTE iHour,BYTE iMin,BYTE iSec)
 {
	 BYTE buff[4];
	 buff[0] = 0x27;
	 if (iHour == 86 && iMin == 45 && iSec == 19)
	 {
		 buff[1] = 0;buff[2] = 0;buff[3] = 0;
	 } 
	 else
	 {
		 buff[1] = iHour;buff[2] = iMin;buff[3] = iSec;
	 }
	 
	flyAudioReturnToUser(pForyouDVDInfo, buff,4);
 }
 
 void returnDVDPlayStatusSpeed(struct flydvd_struct_info *pForyouDVDInfo,BYTE iPlayStatus,BYTE iPlaySpeed)
 {
	 BYTE buff[3];
	 buff[0] = 0x28;
 
	 buff[1] = iPlayStatus;buff[2] = iPlaySpeed;
	 
	 flyAudioReturnToUser(pForyouDVDInfo, buff,3);
 }
 
 void returnDVDPlayMode(struct flydvd_struct_info *pForyouDVDInfo,BYTE iPlayMode)
 {
	 BYTE buff[2];
	 buff[0] = 0x11;
 
	 buff[1] = iPlayMode;
	 
	 flyAudioReturnToUser(pForyouDVDInfo, buff,2);
 }
 
 void returnDVDCurrentFolderInfo(struct flydvd_struct_info *pForyouDVDInfo,UINT16 totalCount,UINT16 folderCount,BOOL bRoot)
 {
	 BYTE buff[1+2+2+1];
	 buff[0] = 0x12;
 
	 buff[1] = totalCount >> 8;
	 buff[2] = totalCount;
 
	 buff[3] = folderCount >> 8;
	 buff[4] = folderCount;
 
	 if (bRoot)
	 {
		 buff[5] = 1;
	 } 
	 else
	 {
		 buff[5] = 0;
	 }
 
	 flyAudioReturnToUser(pForyouDVDInfo, buff,6);
 }
 
 void returnDVDFileFolderInfo(struct flydvd_struct_info *pForyouDVDInfo, BOOL bFolder,BYTE *pName,UINT16 nameLength,UINT16 index,UINT16 globalIndex)
 {
	 BYTE buff[6+256];
	 buff[0] = 0x13;
 
	 buff[1] = index >> 8;buff[2] = index;
	 buff[3] = globalIndex >> 8;buff[4] = globalIndex;
	 if (bFolder)
	 {
		 buff[5] = 0;
	 }
	 else
	 {
		 buff[5] = 1;
	 }
	 memcpy(&buff[6],pName,nameLength);
 
	 flyAudioReturnToUser(pForyouDVDInfo, buff,nameLength+6);
 }
 
 void returnDVDNowPlayingFileInfo(struct flydvd_struct_info *pForyouDVDInfo,P_FOLDER_FILE_TREE_LIST p,UINT16 index,UINT16 globalIndex)
 {
	 BYTE buff[6+256];
	 buff[0] = 0x2A;
 
	 buff[1] = index >> 8;buff[2] = index;
	 buff[3] = globalIndex >> 8;buff[4] = globalIndex;
	 buff[5] = p->extension;
	 memcpy(&buff[6],p->name,p->nameLength);
	
	 flyAudioReturnToUser(pForyouDVDInfo, buff,p->nameLength+6);
 }
 
 void returnDVDNowPlayingInFolderName(struct flydvd_struct_info *pForyouDVDInfo,P_FOLDER_FILE_TREE_LIST p)
 {
	 BYTE buff[1+256];
	 buff[0] = 0x29;
 
	 memcpy(&buff[1],p->name,p->nameLength);
 
	 flyAudioReturnToUser(pForyouDVDInfo, buff,p->nameLength+1);
 }

 void returnDVDErrorStatus(struct flydvd_struct_info *pForyouDVDInfo,BYTE iError)
{
	BYTE buff[2];
	buff[0] = 0x10;

	buff[1] = iError;

	flyAudioReturnToUser(pForyouDVDInfo, buff,2);
}

void returnDVDUpdateStatus(struct flydvd_struct_info *pForyouDVDInfo,BOOL bUpdater)
{
	BYTE buff[2];
	buff[0] = 0xE7;

	if (bUpdater)
	{
		buff[1] = 0x01;
	}
	else
	{
		buff[1] = 0x00;
	}

	flyAudioReturnToUser(pForyouDVDInfo, buff, sizeof(buff));
}

  /********************************************************************************
  **函数名称：
  **函数功能：
  **函数参数：
  **返 回 值：
  **********************************************************************************/
 static void dvdCmdPrintf(struct flydvd_struct_info *pForyouDVDInfo,BYTE *buf, UINT16 len)
 {
	 UINT16  i = 0;
	 BYTE send_buf[256];
	 BYTE check_sum;


	 send_buf[0] = 0xFF;
	 send_buf[1] = 0x55;
	 send_buf[2] = len;
	 check_sum = len;
	 
	 for (i=0; i<len; i++)
	 {
		 send_buf[3+i] = buf[i];
		 check_sum += buf[i];
	 }
	 
	 send_buf[3+i] = 0xFF - check_sum;
	  
	 if (serial_write(pForyouDVDInfo->flydvd_fd, send_buf, len+4) < 0)
	 {
		DBG0(debugString("\nDVD wiret to uart error");)
#if DVD_DEBUG
		dvd_debugString("\nDVD wiret to uart error");
#endif 
		return;
	 }
	 
 }

 
 
 void control_DVD_Video_Aspect_Radio(struct flydvd_struct_info *pForyouDVDInfo,BYTE para)
 {
	 BYTE buff[] = {0x09,0x00};
 
	 buff[1] = para;
 
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_Set_View_Mode(struct flydvd_struct_info *pForyouDVDInfo,BYTE para)
 {
	 BYTE buff[] = {0x0A,0x00};
 
	 buff[1] = para;
 
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_PlayBack_DisplayInfo_State_Request(struct flydvd_struct_info *pForyouDVDInfo,BOOL bOn)
 {
	 BYTE buff[] = {0x89,0x00};
	 if (bOn)
	 {
		 buff[1] = 0x01;
	 } 
	 else
	 {
		 buff[1] = 0x00;
	 }
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_IR_CMD(struct flydvd_struct_info *pForyouDVDInfo,BYTE IRCMD)
 {
	 BYTE buff[] = {0x6B,0x00};
	 buff[1] = IRCMD;
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_ReqMechanismState(struct flydvd_struct_info *pForyouDVDInfo,BYTE iDevice)
 {
	 BYTE buff[] = {0x83,0x00};
	 buff[1] = iDevice;
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_ReqMediaState(struct flydvd_struct_info *pForyouDVDInfo)
 {
	 BYTE buff[] = {0x85};
	 dvdCmdPrintf(pForyouDVDInfo, buff,1);
 }
 
 void control_DVD_ReqDVDSoftwareVersion(struct flydvd_struct_info *pForyouDVDInfo)
 {
	 BYTE buff[] = {0x9A};
	 dvdCmdPrintf(pForyouDVDInfo, buff,1);
 }
 
 void control_DVD_ReqFileFolderCount(struct flydvd_struct_info *pForyouDVDInfo)
 {
	 BYTE buff[] = {0x9B};
	 dvdCmdPrintf(pForyouDVDInfo, buff,1);
 }
 
 void control_DVD_ReqFileFolderDetailedInfo(struct flydvd_struct_info *pForyouDVDInfo,BYTE bFile,UINT16 iAbs,UINT16 iOffset)
 {
	 BYTE buff[6];
	 buff[0] = 0x9D;
	 buff[1] = bFile;
	 buff[2] = iAbs >> 8;buff[3] = iAbs;
	 buff[4] = iOffset >> 8;buff[5] = iOffset;
	 dvdCmdPrintf(pForyouDVDInfo, buff,6);
 }
 
 void control_DVD_PlayFileByAbsCount(struct flydvd_struct_info *pForyouDVDInfo,UINT16 parentIndex,UINT16 index)
 {
	 BYTE buff[6];
	 buff[0] = 0x70;
	 buff[1] = parentIndex >> 8;buff[2] = parentIndex;
	 buff[3] = index >> 8;buff[4] = index;
	 buff[5] = pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[index].extension;
	 if(pForyouDVDInfo->bFlyaudioDVD)
	 {
		dvdCmdPrintf(pForyouDVDInfo, buff,6);
	 }
	 else
	 {
		dvdCmdPrintf(pForyouDVDInfo, buff,5);
	 } 
 }
 
 void control_DVD_ReqHighLightFileIndex(struct flydvd_struct_info *pForyouDVDInfo)
 {
	 BYTE buff[] = {0x96};
	 dvdCmdPrintf(pForyouDVDInfo, buff,1);
 }
 
 void control_DVD_ReqRegionCode(struct flydvd_struct_info *pForyouDVDInfo)
 {
	 BYTE buff[] = {0x82,0x02};
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_ID3CDText(struct flydvd_struct_info *pForyouDVDInfo,BYTE iWhat,UINT iStart,UINT iCount)
 {
	 BYTE buff[] = {0x8C,0x51,0x00,0x01,0x00,0x01};

	 buff[1] = iWhat;

	 buff[2] = iStart >> 8;
	 buff[3] = iStart;

	 buff[4] = iCount >> 8;
	 buff[5] = iCount;
	 dvdCmdPrintf(pForyouDVDInfo, buff,6);
 }

 void control_DVD_ControlRegionCode(struct flydvd_struct_info *pForyouDVDInfo,BYTE iRegionCode)
 {
	 BYTE buff[] = {0x02,0x09};
	 buff[1] = iRegionCode;
	 dvdCmdPrintf(pForyouDVDInfo, buff,2);
 }
 
 void control_DVD_VideoSetup(struct flydvd_struct_info *pForyouDVDInfo)
 {
	 BYTE buff[] = {0x03,0x00,0x00,0x00,0x14,20,16,9,9};
	 //cmd ,CVBS+PAL,16:9,full screen,standard,contrast,brightness,hue,saturation
	 dvdCmdPrintf(pForyouDVDInfo, buff,9);
 }
 
 void control_DVD_JumpNextN(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iJumpN)
 {
	 BYTE buff[] = {0x4F,0x00,0x00};
	 buff[1] = iJumpN >> 8;
	 buff[2] = iJumpN;
	 dvdCmdPrintf(pForyouDVDInfo, buff,3);
 }
 
 void control_DVD_JumpPrevN(struct flydvd_struct_info *pForyouDVDInfo,UINT16 iJumpN)
 {
	 BYTE buff[] = {0x50,0x00,0x00};
	 buff[1] = iJumpN >> 8;
	 buff[2] = iJumpN;
	 dvdCmdPrintf(pForyouDVDInfo, buff,3);
 }
 
 void structDVDInfoInit(struct flydvd_struct_info *pForyouDVDInfo,BOOL bInitAll)
 {
 
	 if (bInitAll)
	 {
		 pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize = FALSE;
		 pForyouDVDInfo->sForyouDVDInfo.bDeviceRec89 = 0;
	 }

 	DBG0(debugOneData("ForyouDVD para init:\n",bInitAll);)
#if DVD_DEBUG
		dvd_debugOneData("ForyouDVD para init:\n",bInitAll);
#endif 
	 
	 pForyouDVDInfo->sForyouDVDInfo.DVDReqStep = 0;
 
	 pForyouDVDInfo->sForyouDVDInfo.bQuickJumpNext = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = 0;
	 pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextTimer = 0;
 
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[1] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[2] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[0] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[1] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[2] = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[3] = 0xFF;
 
	 pForyouDVDInfo->sForyouDVDInfo.CurrentReqMechanismCircle = 0;
 
	 pForyouDVDInfo->sForyouDVDInfo.MediaDiscType = 0;
	 pForyouDVDInfo->sForyouDVDInfo.MediaFileType = 0;
	 pForyouDVDInfo->sForyouDVDInfo.MediaVideoInfo = 0;
	 pForyouDVDInfo->sForyouDVDInfo.MediaAudioSampleFrequency = 0;
	 pForyouDVDInfo->sForyouDVDInfo.MediaAudioCoding = 0;
 
	 pForyouDVDInfo->sForyouDVDInfo.preVideoAspect = 0xFF;
	 pForyouDVDInfo->sForyouDVDInfo.curVideoAspect = 0xFF;
 
	 pForyouDVDInfo->sForyouDVDInfo.CurrentTitle = -1;
	 pForyouDVDInfo->sForyouDVDInfo.CurrentChar = -1;
	 pForyouDVDInfo->sForyouDVDInfo.TotalTitle = 0;
	 pForyouDVDInfo->sForyouDVDInfo.TotalChar = 0;
	 pForyouDVDInfo->sForyouDVDInfo.EscapeHour = 0;
	 pForyouDVDInfo->sForyouDVDInfo.EscapeMinute = 0;
	 pForyouDVDInfo->sForyouDVDInfo.EscapeSecond = 0;
	 pForyouDVDInfo->sForyouDVDInfo.TotalHour = 0;
	 pForyouDVDInfo->sForyouDVDInfo.TotalMinute = 0;
	 pForyouDVDInfo->sForyouDVDInfo.TotalSecond = 0;
	 pForyouDVDInfo->sForyouDVDInfo.PlayMode = 0;
	 pForyouDVDInfo->sForyouDVDInfo.AudioType = 0;
	 pForyouDVDInfo->sForyouDVDInfo.PlaySpeed = 0;
	 pForyouDVDInfo->sForyouDVDInfo.PlayStatus = 0;
	 pForyouDVDInfo->sForyouDVDInfo.DVDRoot = 0;
	 pForyouDVDInfo->sForyouDVDInfo.DeviceStatus = 0;
	 pForyouDVDInfo->sForyouDVDInfo.DeviceType = 0;
	 pForyouDVDInfo->sForyouDVDInfo.HaveDisc = 0;
	 pForyouDVDInfo->sForyouDVDInfo.HaveUSB = 0;
	 pForyouDVDInfo->sForyouDVDInfo.HaveSD = 0;
 
	 pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pBHaveGetFolderFile = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pReturnE8 = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pReturnEAFolder = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pReturnEAFile = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pFolderCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pFileCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pRecFolderCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pRecFileCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pLastRecFolderIndex = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pLastRecFileIndex = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFileErrorCheck = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pBGetFolderFinish = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pBGetFileFinish = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish = FALSE;
	 listFileFolderClearAll(pForyouDVDInfo,TRUE);
	 listFileFolderClearAll(pForyouDVDInfo,FALSE);
	 pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount = 0;
	 pForyouDVDInfo->sForyouDVDInfo.pNowPlayingInWhatFolder = 0;
 
	 pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.bNeedSend8CTime = 0;

	 pForyouDVDInfo->sForyouDVDInfo.iDVDReturnRegionCode = 0xFF;//缺省
 
	 memset(pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion,0,6);
	
	
	 pForyouDVDInfo->sForyouDVDInfo.bDVDRequestState = 0;
	 pForyouDVDInfo->sForyouDVDInfo.bDVDResponseState = 0;
	 pForyouDVDInfo->sForyouDVDInfo.iDVDStateCheckTime = 0;
#if FORYOU_DVD_BUG_FIX
	 pForyouDVDInfo->sForyouDVDInfo.bNeedReturnNoDiscAfterClose = FALSE;
	 pForyouDVDInfo->sForyouDVDInfo.iNeedReturnNoDiscAfterCloseTime = 0;
#endif
 }

 void transDVDInfoCB(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len)
 {
	 if (p[0] > 3)
	 {
		 return;
	 }
	 if (0x00 == p[0])
	 {
		 DBG3(debugThreeData("ForyouDVD CB:",p[0],p[1],p[2]);)
	 }
	 else if (0x01 == p[0])
	 {
		 DBG3(debugThreeData("ForyouDVD CB:",p[0],p[1],p[2]);)
	 }
	 else if (0x02 == p[0])
	 {
		 DBG3(debugThreeData("ForyouDVD CB:",p[0],p[1],p[2]);)
	 }
	 else if (0x03 == p[0])
	 {
		DBG3(debugThreeData("ForyouDVD CB:",p[0],p[1],p[2]);)
	 }
	 if (pForyouDVDInfo->sForyouDVDInfo.bFilterDiscInFirstPowerUp)//过滤DiscIn
	 {
		 if (0x00 == p[0])//过滤碟机
		 {
			 if (0x00 == p[1])//开机无碟
			 {
				 pForyouDVDInfo->sForyouDVDInfo.bFilterDiscInFirstPowerUp = FALSE;
				 DBG0(debugString("ForyouDVD DVD DiscIn Filter End With Real NoDisc\n");)
			 }
			 else if (0x03 == p[1])//开始过滤
			 {
				 pForyouDVDInfo->sForyouDVDInfo.bFilterDiscIn = TRUE;
				 DBG3(debugString("ForyouDVD DVD DiscIn Filter\n");)
				 return;
			 }
			 else
			 {
				 if (pForyouDVDInfo->sForyouDVDInfo.bFilterDiscIn)//过滤完成
				 {
					 pForyouDVDInfo->sForyouDVDInfo.bFilterDiscInFirstPowerUp = FALSE;
					 DBG3(debugString("ForyouDVD DVD DiscIn Filter End\n");)
				 }
			 }
		 }
	 }
#if FORYOU_DVD_BUG_FIX
	 if (0x00 == p[0])
	 {
		 if (0x03 == p[1])
		 {
			 pForyouDVDInfo->sForyouDVDInfo.bNeedReturnNoDiscAfterClose = TRUE;
			 pForyouDVDInfo->sForyouDVDInfo.iNeedReturnNoDiscAfterCloseTime = GetTickCount();
		 }
	 }
#endif
	 if (3 != p[0])
	 {
		 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] != p[1])
		 {
			 if (0x00 == p[0])//DISC LED Flash
			 {
				 if (0x02 == p[1] && pForyouDVDInfo->sForyouDVDInfo.bLEDFlashAble)
				 {
					 pForyouDVDInfo->sForyouDVDInfo.bLEDFlashAble = FALSE;
					 DBG0(debugString("ForyouDVD LED Flash\n");)
					 pForyouDVDInfo->sForyouDVDInfo.LEDFlash = TRUE;
					 sem_post(&pForyouDVDInfo->LED_Flash_sem);
				 }
				 else
				 {
					 pForyouDVDInfo->sForyouDVDInfo.bLEDFlashAble = TRUE;
				 }
			 }
			 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] = p[1];
			 returnDVDDeviceActionState(pForyouDVDInfo, p[0],pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]]);
		 }
		 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[p[0]] != p[2])
		 {
			 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[p[0]] = p[2];
			 if (p[0] == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice)
			 {
				 returnDVDDeviceContent(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[p[0]]);
			 }
		 }
	 }
 
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] = p[1];
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[p[0]] = p[2];
 
	 if(pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] != 0x09)
	 {
		 if (p[0] == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice)
		 {
			 structDVDInfoInit(pForyouDVDInfo, FALSE);
		 }
	 }
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] = p[1];
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[p[0]] = p[2];
	 //if (3 == p[0])
	 //{
	 //  if (0x09 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[3])//Disc
	 //  {
	 // 	 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice != 0)
	 // 	 {
	 // 		 structDVDInfoInit(FALSE);
	 // 		 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice = 0;//Disc
	 // 		 returnDVDDevicePlayDevice(pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice);
	 // 	 }
	 //  }
	 //  else if (0x08 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[3])
	 //  {
	 // 	 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice != 1)
	 // 	 {
	 // 		 structDVDInfoInit(FALSE);
	 // 		 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice = 1;//USB
	 // 		 returnDVDDevicePlayDevice(pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice);
	 // 	 }
	 //  }
	 //  else
	 //  {
	 // 	 if (1 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice)
	 // 	 {
	 // 		 structDVDInfoInit(FALSE);
	 // 		 control_DVD_IR_CMD(0x95);//切换到DVD
	 // 	 }
	 //  }
	 //}
 }

 void transDVDInfoCD(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len)
 {
	 if(pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3] != 0x09)//启动获取流程
	 {
		 DBG3(debugString("ForyouDVD CD Filter For Mechanism Not Ready\n");)
		 return;
	 }
 
	 if ((pForyouDVDInfo->sForyouDVDInfo.MediaDiscType != (p[0] >> 4))
		 || (pForyouDVDInfo->sForyouDVDInfo.MediaFileType != (p[0] & 0x0F)))
	 {
		 pForyouDVDInfo->sForyouDVDInfo.MediaDiscType = p[0] >> 4;
		 pForyouDVDInfo->sForyouDVDInfo.MediaFileType = p[0] & 0x0F;
		 returnDVDDeviceMedia(pForyouDVDInfo, 
			 pForyouDVDInfo->sForyouDVDInfo.MediaDiscType,
			 pForyouDVDInfo->sForyouDVDInfo.MediaFileType);
	 }
	 pForyouDVDInfo->sForyouDVDInfo.MediaVideoInfo = p[1] >> 6;
	 pForyouDVDInfo->sForyouDVDInfo.MediaAudioSampleFrequency = (p[1] >> 3) & 0x07;
	 pForyouDVDInfo->sForyouDVDInfo.MediaAudioCoding = p[1] & 0x07;
 }

 void transDVDInfoD2(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len)
 {
	 pForyouDVDInfo->sForyouDVDInfo.bDeviceRec89 = GetTickCount();
 
	 pForyouDVDInfo->sForyouDVDInfo.HaveDisc = p[18];
	 pForyouDVDInfo->sForyouDVDInfo.HaveUSB = p[19];
	 pForyouDVDInfo->sForyouDVDInfo.HaveSD = p[20];
 
	 if (pForyouDVDInfo->sForyouDVDInfo.PlayMode != p[12])
	 {
		 pForyouDVDInfo->sForyouDVDInfo.PlayMode = p[12];
		 returnDVDPlayMode(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.PlayMode);
	 }
 
	 pForyouDVDInfo->sForyouDVDInfo.AudioType = p[13];
 
	 if (((pForyouDVDInfo->sForyouDVDInfo.PlaySpeed << 4) + pForyouDVDInfo->sForyouDVDInfo.PlayStatus) != p[14])
	 {
		 pForyouDVDInfo->sForyouDVDInfo.PlaySpeed = p[14] >> 4;
		 pForyouDVDInfo->sForyouDVDInfo.PlayStatus = p[14] & 0x0F;
		 returnDVDPlayStatusSpeed(pForyouDVDInfo, 
			 pForyouDVDInfo->sForyouDVDInfo.PlayStatus,
			 pForyouDVDInfo->sForyouDVDInfo.PlaySpeed);
	 }
 
	 pForyouDVDInfo->sForyouDVDInfo.DVDRoot = p[15];
 
	 pForyouDVDInfo->sForyouDVDInfo.DeviceStatus = p[16];
	 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismType[3] = p[17];
	 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3] != pForyouDVDInfo->sForyouDVDInfo.DeviceStatus
		 || pForyouDVDInfo->sForyouDVDInfo.DeviceType != p[17])
	 {
		 pForyouDVDInfo->sForyouDVDInfo.DeviceType = p[17];
		 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3] = pForyouDVDInfo->sForyouDVDInfo.DeviceStatus;
 
		 if (0x00 == pForyouDVDInfo->sForyouDVDInfo.HaveDisc)
		 {
			 if (0x08 == pForyouDVDInfo->sForyouDVDInfo.DeviceType)
			 {
				 if (0xFF == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[1])
				 {
					 returnDVDDeviceActionState(pForyouDVDInfo, 0x01,pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3]);
				 }
			 }
			 else
			 {
				 if (0xFF == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0])
				 {
					 returnDVDDeviceActionState(pForyouDVDInfo, 0x00,pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3]);
				 }
			 }
		 }
	 }
 
	 if (0x09 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3])
	 {
		 if (pForyouDVDInfo->sForyouDVDInfo.CurrentChar != (p[1] * 256 + p[2])
			 || pForyouDVDInfo->sForyouDVDInfo.CurrentTitle != p[0])
		 {
			 pForyouDVDInfo->sForyouDVDInfo.CurrentTitle = p[0];
			 pForyouDVDInfo->sForyouDVDInfo.CurrentChar = p[1] * 256 + p[2];
			 returnDVDCurrentTitleTrack(pForyouDVDInfo, 
				 pForyouDVDInfo->sForyouDVDInfo.CurrentTitle,
				 pForyouDVDInfo->sForyouDVDInfo.CurrentChar);
		 }
 
		 if (pForyouDVDInfo->sForyouDVDInfo.TotalChar != (p[4] * 256 + p[5])
			 || pForyouDVDInfo->sForyouDVDInfo.TotalTitle != p[3])
		 {
			 pForyouDVDInfo->sForyouDVDInfo.TotalTitle = p[3];
			 pForyouDVDInfo->sForyouDVDInfo.TotalChar = p[4] * 256 + p[5];
			 returnDVDTotalTitleTrack(pForyouDVDInfo, 
				 pForyouDVDInfo->sForyouDVDInfo.TotalTitle,
				 pForyouDVDInfo->sForyouDVDInfo.TotalChar);
		 }
 
		 if (pForyouDVDInfo->sForyouDVDInfo.EscapeSecond != p[8]
		 || pForyouDVDInfo->sForyouDVDInfo.EscapeMinute != p[7]
		 || pForyouDVDInfo->sForyouDVDInfo.EscapeHour != p[6])
		 {
			 pForyouDVDInfo->sForyouDVDInfo.EscapeHour = p[6];
			 pForyouDVDInfo->sForyouDVDInfo.EscapeMinute = p[7];
			 pForyouDVDInfo->sForyouDVDInfo.EscapeSecond = p[8];
			 returnDVDCurrentTime(pForyouDVDInfo, 
				 pForyouDVDInfo->sForyouDVDInfo.EscapeHour,
				 pForyouDVDInfo->sForyouDVDInfo.EscapeMinute,
				 pForyouDVDInfo->sForyouDVDInfo.EscapeSecond);
		 }
 
		 if (pForyouDVDInfo->sForyouDVDInfo.TotalSecond != p[11]
		 || pForyouDVDInfo->sForyouDVDInfo.TotalMinute != p[10]
		 || pForyouDVDInfo->sForyouDVDInfo.TotalHour != p[9])
		 {
			 pForyouDVDInfo->sForyouDVDInfo.TotalHour = p[9];
			 pForyouDVDInfo->sForyouDVDInfo.TotalMinute = p[10];
			 pForyouDVDInfo->sForyouDVDInfo.TotalSecond = p[11];
			 returnDVDTotalTime(pForyouDVDInfo, 
				 pForyouDVDInfo->sForyouDVDInfo.TotalHour,
				 pForyouDVDInfo->sForyouDVDInfo.TotalMinute,
				 pForyouDVDInfo->sForyouDVDInfo.TotalSecond);
		 }
	 }
 
	 if (0x09 == pForyouDVDInfo->sForyouDVDInfo.DeviceType)//Disc
	 {
		 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice != 0)
		 {
			 structDVDInfoInit(pForyouDVDInfo, FALSE);
			 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice = 0;//Disc
			 returnDVDDevicePlayDevice(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice);
		 }
	 }
	 else if (0x08 == pForyouDVDInfo->sForyouDVDInfo.DeviceType)
	 {
		 if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice != 1)
		 {
			 structDVDInfoInit(pForyouDVDInfo, FALSE);
			 pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice = 1;//USB
			 returnDVDDevicePlayDevice(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice);
		 }
	 }
	 else
	 {
		 if (1 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice)
		 {
			 structDVDInfoInit(pForyouDVDInfo, FALSE);
			 control_DVD_IR_CMD(pForyouDVDInfo, 0x95);//切换到DVD
		 }
	 }
 
	 if(pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3] == 0x09)//启动获取流程
	 {
		 if(pForyouDVDInfo->sForyouDVDInfo.MediaDiscType == 0x02)//Clips
		 {
			 if(pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile == FALSE)
			 {
				 if (!pForyouDVDInfo->sForyouDVDInfo.pBHaveGetFolderFile)
				 {
					 pForyouDVDInfo->sForyouDVDInfo.pBHaveGetFolderFile = TRUE;
					 pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = TRUE;
					 pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 0;
					 pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = TRUE;
 
					 pForyouDVDInfo->sForyouDVDInfo.pBGetFolderFinish = FALSE;
					 pForyouDVDInfo->sForyouDVDInfo.pBGetFileFinish = FALSE;
					 
					 DBG0(debugString("ForyouDVD Start get folder and file list\n");)
				 }
				 else
				 {
					DBG3(debugString("already get\n");)
				 }
			 }
			 else
			 {
				 DBG3(debugString("already start\n");)
			 }
		 }
		 else
		 {
			 DBG3(debugString(" not clips\n");)
		 }
	 }
	 else
	 {
		 DBG3(debugString(" not ready\n");)
	 }
 }

 void transDVDInfoD5(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
 {
	 UINT i;
	 UINT iCount;
	 if (0x51 == p[0])
	 {
		 iCount = p[1] * 256 + p[2] - 1;
		 if (pForyouDVDInfo->sForyouDVDInfo.pFileCount)
		 {
			 if (iCount < pForyouDVDInfo->sForyouDVDInfo.pFileCount)
			 {
				 for (i = 3;i < len - 1;i+=2)
				 {
					 if (0 == p[i] && 0 == p[i+1])
					 {
						 break;
					 }
					 else
					 {
						 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i-3] = p[i+1];
						 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i-2] = p[i];
						 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i-1] = 0;
						 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].name[i-0] = 0;
						 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[iCount].nameLength = i - 1 + 2;
					 }
				 }
				 if (iCount == pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount)
				 {
					 returnDVDNowPlayingFileInfo(pForyouDVDInfo,
						 &pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount],
						 getSelectFolderFileIndexByGlobalIndexInParent(pForyouDVDInfo,FALSE,pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount)
						 ,pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount);
				 }
			 }
		 }
	 }
 }

 void transDVDInfoE8(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len)
 {
	DBG3(debugTwoData("\n%%%%%%%%%%%%%%%%%E8 filecount and Foldercount:" ,
		pForyouDVDInfo->sForyouDVDInfo.pFileCount,pForyouDVDInfo->sForyouDVDInfo.pFolderCount);)
		
	if ( pForyouDVDInfo->sForyouDVDInfo.pFileCount != p[2]*256 + p[3] 
		||  pForyouDVDInfo->sForyouDVDInfo.pFolderCount != p[0]*256 + p[1])
	{
		pForyouDVDInfo->sForyouDVDInfo.pReturnE8 = TRUE;
		pForyouDVDInfo->sForyouDVDInfo.pFileCount = p[2]*256 + p[3];
		pForyouDVDInfo->sForyouDVDInfo.pFolderCount = p[0]*256 + p[1];


		
		listFileFolderNewAll(pForyouDVDInfo, TRUE,pForyouDVDInfo->sForyouDVDInfo.pFileCount);
		listFileFolderNewAll(pForyouDVDInfo, FALSE,pForyouDVDInfo->sForyouDVDInfo.pFolderCount);

		pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = TRUE;

		PostSignal(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain);
	}
		
 }

 void transDVDInfoEA(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT16 len)
 {
	//DLOGD("File:%d,C:%d,P:%d,L:%d", p[4], p[0]*256+p[1], p[2]*256+p[3], len-6);
	DBG3(debugBuf("\nFile::", &p[0], 5);)
	
	 if (p[4])
	 {
		 pForyouDVDInfo->sForyouDVDInfo.pReturnEAFile = TRUE;
		 pForyouDVDInfo->sForyouDVDInfo.pLastRecFileIndex = p[0]*256 + p[1];
 
		 listFileFolderStorageOne(pForyouDVDInfo, TRUE,p[0]*256 + p[1],p[2]*256 + p[3],p[5],&p[6],len-6);
 
		 if (pForyouDVDInfo->sForyouDVDInfo.pLastRecFileIndex == pForyouDVDInfo->sForyouDVDInfo.pFileCount - 1)
		 {
			 pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = TRUE;
		 }
	 }
	 else
	 {
		 pForyouDVDInfo->sForyouDVDInfo.pReturnEAFolder = TRUE;
		 
		 pForyouDVDInfo->sForyouDVDInfo.pLastRecFolderIndex = p[0]*256 + p[1];
		 listFileFolderStorageOne(pForyouDVDInfo, FALSE,p[0]*256 + p[1],p[2]*256 + p[3],p[5],&p[6],len-6);
 
		 if (pForyouDVDInfo->sForyouDVDInfo.pLastRecFolderIndex == pForyouDVDInfo->sForyouDVDInfo.pFolderCount - 1)
		 {
			 pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = TRUE;
		 }
	 }
	 
	 if (p[4])
	 {
		 if (pForyouDVDInfo->sForyouDVDInfo.pLastRecFileIndex == pForyouDVDInfo->sForyouDVDInfo.pFileCount - 1)
		 {
			PostSignal(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain);
		 }
	 }
	 else
	 {
		 if (pForyouDVDInfo->sForyouDVDInfo.pLastRecFolderIndex == pForyouDVDInfo->sForyouDVDInfo.pFolderCount - 1)
		 {
			PostSignal(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain);
		 }
	 }
 
 }

 static BOOL isFileStringEmpty(struct flydvd_struct_info *pForyouDVDInfo,P_FOLDER_FILE_TREE_LIST p)
 {
	if (p->nameLength == 0)
		return TRUE;
		
	return FALSE;
 }
 void procDVDInfoE0(struct flydvd_struct_info *pForyouDVDInfo)
 {
	if (isFileStringEmpty(pForyouDVDInfo,&pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount]))
	{
		pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc = TRUE;
		return;
	}
	else
	{
		pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc = FALSE;
	}
		
	 pForyouDVDInfo->sForyouDVDInfo.pNowPlayingInWhatFolder = pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount].parentFolderIndex;
	 returnDVDNowPlayingFileInfo(pForyouDVDInfo, 
		 &pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount],
		 getSelectFolderFileIndexByGlobalIndexInParent(pForyouDVDInfo,FALSE,pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount)
		 ,pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount);
	 returnDVDNowPlayingInFolderName(pForyouDVDInfo, 
		 &pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount].parentFolderIndex]);
 }

 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 static void DealDVDInfo(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p, UINT16 length)
 {
 
	if (0xEA != p[0])
	{
		//debugBuf("ForyouDVD decode:",p,length);
#if DVD_DEBUG
		dvd_debugBuf("\nForyouDVD decode:",p,length);
#endif 
	}

	pForyouDVDInfo->iAutoResetControlTime = GetTickCount();
	//DBG1(debugOneData("\niAutoResetControlTime:", pForyouDVDInfo->iAutoResetControlTime);)
	switch (p[0])
	{
		case 0xDB://初始化OK报告 
			DBG0(debugOneData("======0xDB=========\n",p[1]);)
			
			if (2 == length && 0x00 == p[1])
			{
				pForyouDVDInfo->bFlyaudioDVD = TRUE;
				pFlyAllInOneInfo->pMemory_Share_Common->bDVDType = TRUE;
			}
			else
			{
				pForyouDVDInfo->bFlyaudioDVD = FALSE;
				pFlyAllInOneInfo->pMemory_Share_Common->bDVDType = FALSE;
			}
			pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize = TRUE;
			structDVDInfoInit(pForyouDVDInfo, FALSE);
			
			//control_DVD_IR_CMD(0x17);
			//control_DVD_VideoSetup();
			
			control_DVD_PlayBack_DisplayInfo_State_Request(pForyouDVDInfo,TRUE);
			returnDVDDeviceWorkMode(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize);
			
			pForyouDVDInfo->sForyouDVDInfo.bFilterDiscInFirstPowerUp = FALSE;//新的碟机程序解决了？
			pForyouDVDInfo->sForyouDVDInfo.bFilterDiscIn = FALSE;
			break; 
		case 0xC1:
			if (0x06 == p[1] || 0x07 == p[1] || 0x08 == p[1] || 0x09 == p[1] || 0x0A == p[1])
			{
				returnDVDErrorStatus(pForyouDVDInfo, p[1]);
			}
			break;
		case 0xC2:
			pForyouDVDInfo->sForyouDVDInfo.iDVDReturnRegionCode = p[1];
			if (pForyouDVDInfo->sForyouDVDInfo.iDVDReturnRegionCode
				!= pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode)//设置并重新查询
			{
				control_DVD_ControlRegionCode(pForyouDVDInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode);
				pForyouDVDInfo->sForyouDVDInfo.iDVDReturnRegionCode = 0xFF;//重新查询
			}
			break;
		case 0xCC:
			if (((pForyouDVDInfo->sForyouDVDInfo.PlaySpeed << 4) + pForyouDVDInfo->sForyouDVDInfo.PlayStatus) != p[1])
			{
				pForyouDVDInfo->sForyouDVDInfo.PlaySpeed = p[1] >> 4;
				pForyouDVDInfo->sForyouDVDInfo.PlayStatus = p[1] & 0x0F;
				returnDVDPlayStatusSpeed(pForyouDVDInfo, 
					pForyouDVDInfo->sForyouDVDInfo.PlayStatus,
					pForyouDVDInfo->sForyouDVDInfo.PlaySpeed);
			}
			break;
		case 0xCB:
			transDVDInfoCB(pForyouDVDInfo, &p[1],length-1);
			break;
		case 0xCD:
			transDVDInfoCD(pForyouDVDInfo, &p[1],length-1);
			break;
		case 0xD2:
			transDVDInfoD2(pForyouDVDInfo, &p[1],length-1);
			break;
		case 0xD5:
			transDVDInfoD5(pForyouDVDInfo,&p[1],length-1);
			break;
		case 0xE8:
			DBG3(debugThreeData("\nE8:",p[2],p[3],p[4]);)
			transDVDInfoE8(pForyouDVDInfo, &p[1],length-1);
			break;
		case 0xEA:
			transDVDInfoEA(pForyouDVDInfo, &p[1],length-1);
			break;
		case 0xE0:
			//DBG0(debugThreeData("\nE0:",p[1],p[2],p[3]);)
			if (pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount != (p[1]*256 + p[2]))
			{
				pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount = p[1]*256 + p[2];
				
				if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
				{
					procDVDInfoE0(pForyouDVDInfo);
				}
				else
				{
					pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc = TRUE;
					DBG0(debugString("ForyouDVD Rec E0 But Haven't Rec File Or Folder Info\n");)
				}
			}
			break;
		case 0xE5:
			if (memcmp(pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion,&p[1],6))
			{
				memcpy(pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion,&p[1],6);

				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[0] = 'Y';
				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[1]
				= (pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0] >> 4)/10 + '0';
				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[2]
				= (pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0] >> 4)%10 + '0';

				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[3] = 'M';
				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[4]
				= (pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0] & 0x0F)/10 + '0';
				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[5]
				= (pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0] & 0x0F)%10 + '0';

				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[6] = 'D';
				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[7]
				= pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[1]/10 + '0';
				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[8]
				= pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[1]%10 + '0';

				pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersionLength = 9;
				ipcStartEvent(EVENT_GLOBAL_RETURN_DVD_VERSION_ID);
			}
			break;
		case 0xE7:
			pForyouDVDInfo->bEnterUpdateMode = TRUE;
			returnDVDUpdateStatus(pForyouDVDInfo,pForyouDVDInfo->bEnterUpdateMode);
			break;
		default:
			DBG3(debugString("unHandle!\n");)
			break;
	}
 }
 
 UINT16 selectFolderOrFileByLocalIndex(struct flydvd_struct_info *pForyouDVDInfo,INT inWhatFolder,UINT16 localIndex,BOOL bHaveFolderIndex)
 {
	 UINT16 isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,TRUE);
	 UINT16 isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,FALSE);
	 if (0 == inWhatFolder)//当前在根目录
	 {
		 if (!bHaveFolderIndex)
		 {
			 localIndex = localIndex + isFolderCount;
		 }
		 DBG0(debugOneData("\nroot folder:",isFolderCount);)
		 DBG0(debugOneData("\nfile:", isFileCount);)
		 DBG0(debugOneData("\nsel:",localIndex);)
		 
		 if (localIndex < isFolderCount)//选择的是文件夹
		 {
			 inWhatFolder = 
				 getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, inWhatFolder,TRUE,localIndex);
			 isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,TRUE);
			 isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,FALSE);
			 returnDVDCurrentFolderInfo(pForyouDVDInfo, isFolderCount+1+isFileCount,isFolderCount+1,FALSE);
		 }
		 else if (localIndex < (isFolderCount + isFileCount))//选择的是文件
		 {
			 control_DVD_PlayFileByAbsCount(pForyouDVDInfo, 
				 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, inWhatFolder,FALSE,localIndex - isFolderCount)].parentFolderIndex,
				 getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, inWhatFolder,FALSE,localIndex - isFolderCount));
		 }
	 }
	 else//当前不在根目录
	 {
		 if (!bHaveFolderIndex)
		 {
			 localIndex = localIndex + isFolderCount + 1;
		 }
		 if (0 == localIndex)//选择上一级目录
		 {
			 inWhatFolder = 
				 pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[inWhatFolder].parentFolderIndex;
			 isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,TRUE);
			 isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,FALSE);
			 if (0 == inWhatFolder)//当前在根目录
			 {
				 returnDVDCurrentFolderInfo(pForyouDVDInfo, isFolderCount+isFileCount,isFolderCount,TRUE);
			 }
			 else
			 {
				 returnDVDCurrentFolderInfo(pForyouDVDInfo, isFolderCount+1+isFileCount,isFolderCount+1,FALSE);
			 }
		 }
		 else
		 {
			 if ((localIndex - 1) < isFolderCount)//选择的是文件夹
			 {
				 inWhatFolder = 
					 getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, inWhatFolder,TRUE,localIndex - 1);
				 isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,TRUE);
				 isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo, inWhatFolder,FALSE);
				 returnDVDCurrentFolderInfo(pForyouDVDInfo, isFolderCount+1+isFileCount,isFolderCount+1,FALSE);
			 }
			 else if ((localIndex - 1) < (isFolderCount + isFileCount))//选择的是文件
			 {
				 control_DVD_PlayFileByAbsCount(pForyouDVDInfo, 
					 pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, inWhatFolder,FALSE,localIndex - 1 - isFolderCount)].parentFolderIndex,
					 getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, inWhatFolder,FALSE,localIndex - 1 - isFolderCount));
			 }
		 }
	 }
	 return inWhatFolder;
 }
 
static BOOL isCheckValueVaild(struct flydvd_struct_info *pForyouDVDInfo,BYTE *search_sync_ptr)
{
	UINT16 i=0;
	BYTE checksum = 0;
	BYTE oldChecksum = 0;
	UINT16 streamLength = 0;
	
	streamLength = *(search_sync_ptr + 2) + 1;
	for(i=0; i<streamLength; i++)
	{
		checksum += *(search_sync_ptr + i + 2);
	}
	
	oldChecksum = (BYTE)(0xFF - *(search_sync_ptr + *(search_sync_ptr + 2)+3));
	if (checksum == oldChecksum)
	{
		return TRUE;
	}
	
	return FALSE;
}

static BYTE *extractPacket(struct flydvd_struct_info *pForyouDVDInfo,BYTE *search_sync_ptr)
{
	UINT16 length = 0;
	BYTE buff[400];
	
	length = *(search_sync_ptr + 2);
	memcpy(buff, search_sync_ptr + 3, length);
	
	if (pForyouDVDInfo->bFlyaudioDVD)
	{
		DealFlyaudioInfo(pForyouDVDInfo,buff, length);
	}
	else
	{
		DealDVDInfo(pForyouDVDInfo, buff, length);
	}
				
	search_sync_ptr = search_sync_ptr + length + 2 + 1;
	
	return search_sync_ptr;
}
 
static BYTE *searchSync(struct flydvd_struct_info *pForyouDVDInfo,BYTE *search_sync_ptr, BYTE *search_end_ptr, BYTE *sync_status)
{
	*sync_status = SYNC_MISSING;
	
	for (; search_sync_ptr < search_end_ptr; search_sync_ptr++)
	{
		if (*search_sync_ptr == 0xFF)
		{
			if ((search_end_ptr - search_sync_ptr) > 1)
			{
				if (*(search_sync_ptr+1) == 0x55)
				{
					*sync_status = SYNC_FOUND;
					break;
				}
				else
				{
					*sync_status = SYNC_MISSING;
					break;
				}
			}
			else
			{
				*sync_status = SYNC_PARTIAL;
				break;
			}
		}
	}
	
	return search_sync_ptr;
}
static BYTE *searchDataStream(struct flydvd_struct_info *pForyouDVDInfo,BYTE *search_sync_ptr, BYTE *search_end_ptr)
{	
	BYTE sync_status;
	
	while (search_sync_ptr < search_end_ptr)
	{
		search_sync_ptr = searchSync(pForyouDVDInfo, search_sync_ptr, search_end_ptr, &sync_status);
		
		if (sync_status == SYNC_FOUND)
		{
			if (search_end_ptr - search_sync_ptr > 2)
			{
				//华阳碟机连发两次 FF 55 
				if (0xFF == *(search_sync_ptr+2) && *(search_sync_ptr+3) == 0x55)
				{
					search_sync_ptr += 2;
					DBG2(debugString("Foryou DVD have send two FF 55\n");)
				}
				else
				{
					//是否为完整的数据流
					if ((search_end_ptr - search_sync_ptr) > *(search_sync_ptr+2) + 3)
					{
						if (isCheckValueVaild(pForyouDVDInfo, search_sync_ptr))
						{							
							//提取数据
							//debugString("check right");
							search_sync_ptr = extractPacket(pForyouDVDInfo, search_sync_ptr);
						}
						else
						{
							DBG0(debugString("Foryou DVD CRC Error!\n");)
#if DVD_DEBUG
							dvd_debugString("Foryou DVD CRC Error!\n");;
#endif 
							search_sync_ptr += 2;
						}
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				break;
			}
		}
		else if (sync_status == SYNC_MISSING)
		{
			search_sync_ptr += 1;
		}
		else
		{
			break;
		}
	}
	
	return search_sync_ptr;
}
static void moveDataForNext(struct flydvd_struct_info *pForyouDVDInfo,BYTE *start_buffer_ptr, BYTE *start_data_ptr, UINT16 buffer_num)
{
	UINT16 i;
	
	for (i=0; i<buffer_num; i++)
	{
		*start_buffer_ptr = *start_data_ptr;
		start_buffer_ptr++;
		start_data_ptr++;
	}
}
static void dealDataStream(struct flydvd_struct_info *pForyouDVDInfo,BYTE *search_sync_ptr, BYTE *search_end_ptr)
{
	search_sync_ptr = searchDataStream(pForyouDVDInfo, search_sync_ptr, search_end_ptr);
	
	if (search_sync_ptr < search_end_ptr)
	{
		pForyouDVDInfo->DVDInfoReadBuffLength = (UINT16)(search_end_ptr - search_sync_ptr);
		if (pForyouDVDInfo->DVDInfoReadBuffLength >= SERIAL_BUF_MAX_LEN)
		{
			pForyouDVDInfo->DVDInfoReadBuffLength = 0;
		}
		
		moveDataForNext(pForyouDVDInfo, &pForyouDVDInfo->DVDInfoReadBuff[0],
			search_sync_ptr, pForyouDVDInfo->DVDInfoReadBuffLength);
	}
	else
	{
		pForyouDVDInfo->DVDInfoReadBuffLength = 0;
	}
}

  /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 void *read_serial_thread(void *arg) 
 {
	long ret=-1;
	UINT16 i=0;
	BYTE *search_sync_ptr;
	BYTE *search_end_ptr;
	BYTE buf[SERIAL_BUF_MAX_LEN];
	struct flydvd_struct_info *pForyouDVDInfo = (struct flydvd_struct_info *)arg;
	
	pForyouDVDInfo->DVDInfoReadBuffLength = 0;

	while (!pForyouDVDInfo->bKillReadSerialThread)
	{
		ret = serial_read(pForyouDVDInfo->flydvd_fd, 
			&pForyouDVDInfo->DVDInfoReadBuff[pForyouDVDInfo->DVDInfoReadBuffLength], SERIAL_BUF_MAX_LEN-pForyouDVDInfo->DVDInfoReadBuffLength);
		if (ret > 0)
		{	
		
			pForyouDVDInfo->DVDInfoReadBuffLength += (UINT16)ret;
			
			//debugString("read======");
			DBG1(debugBuf("\nDVD-HAL read bytes form uart:", 
				&pForyouDVDInfo->DVDInfoReadBuff[0], pForyouDVDInfo->DVDInfoReadBuffLength);)
#if DVD_DEBUG
			//dvd_debugBuf("\nDVD-HAL read bytes form uart:", 
				//&pForyouDVDInfo->DVDInfoReadBuff[0], pForyouDVDInfo->DVDInfoReadBuffLength);
#endif 

			//DLOGE("DVD-HAL read bytes form Diver:%s",
			//	bufToString(pForyouDVDInfo->DVDInfoReadBuff,pForyouDVDInfo->DVDInfoReadBuffLength));
		
			
			search_sync_ptr = &pForyouDVDInfo->DVDInfoReadBuff[0];
			search_end_ptr  = &pForyouDVDInfo->DVDInfoReadBuff[pForyouDVDInfo->DVDInfoReadBuffLength];
			
			dealDataStream(pForyouDVDInfo, search_sync_ptr,search_end_ptr);
		}
		else
		{
#if DVD_DEBUG
		dvd_debugString("DVD serial not read byte!\n");
#endif 
			continue;
		}
	}
	
	DBG0(debugString("read serial thread exit\n");)
	
	return NULL;
}

BYTE DVDReqStepChangeAndReq(struct flydvd_struct_info *pForyouDVDInfo, BYTE DVDReqStep)
{
	UINT16 i,j;
	BOOL bCheck = FALSE;
	do 
	{
		if (pForyouDVDInfo->sForyouDVDInfo.pBGetFolderFinish && pForyouDVDInfo->sForyouDVDInfo.pBGetFileFinish)
		{
			if (FALSE == pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
			{
				DBG0(debugString("FlyAudio FolderFileList Start Return!");)
				pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish = TRUE;
				UINT16 isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,TRUE);
				UINT16 isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,FALSE);
				returnDVDCurrentFolderInfo(pForyouDVDInfo, isFolderCount+isFileCount,isFolderCount,TRUE);
				
				pForyouDVDInfo->sForyouDVDInfo.bNeedSend8CTime = GetTickCount();
			}
			
			if (pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc)
			{
				procDVDInfoE0(pForyouDVDInfo);
			}
		}

		if (0 == DVDReqStep)//查播放设备
		{
			DVDReqStep = 2;
			//control_DVD_ReqMechanismState(0x03);
			//debugOneData("step:",DVDReqStep);
			//return DVDReqStep;
		}
		//if (1 == DVDReqStep)//查播放设备状态
		//{
		//	DVDReqStep = 2;
		//	if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice <= 2)
		//	{
		//		control_DVD_ReqMechanismState(pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice);
		//		debugOneData("step:",DVDReqStep);
		//		return DVDReqStep;
		//	}
		//	else if (0xFF == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice)
		//	{
		//		control_DVD_ReqMechanismState(0);
		//		debugOneData("step:",DVDReqStep);
		//		return DVDReqStep;
		//	}
		//}
		if (2 == DVDReqStep)
		{
			DVDReqStep = 3;
			if (0x09 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[3])
			{
				control_DVD_ReqMediaState(pForyouDVDInfo);
				//DBG3(debugOneData("\nstep:",DVDReqStep);)
				return DVDReqStep;
			}
		}
		if (3 == DVDReqStep)
		{
			DVDReqStep = 4;
			if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
			{
				control_DVD_ReqHighLightFileIndex(pForyouDVDInfo);
				//DBG3(debugOneData("step:",DVDReqStep);)
				return DVDReqStep;
			}
		}
		if (4 == DVDReqStep)
		{
			DVDReqStep = 5;
			if (pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck)
			{
				for (i = 0;i < pForyouDVDInfo->sForyouDVDInfo.pFolderCount;i++)
				{
					if (!pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[i].bStorage)
					{
						j = i+1;
						while (j < pForyouDVDInfo->sForyouDVDInfo.pFolderCount &&
							(!pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[j].bStorage))
						{
							j++;
						}
						control_DVD_ReqFileFolderDetailedInfo(pForyouDVDInfo, FALSE,i,j-i);
						return DVDReqStep;
					}
				}
				
				if (i == pForyouDVDInfo->sForyouDVDInfo.pFolderCount)
				{
					DBG3(debugString("FlyAudio FolderList check OK!\n");)
					pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck = FALSE;
					pForyouDVDInfo->sForyouDVDInfo.pBGetFolderFinish = TRUE;
				}
			}
		}
		if (5 == DVDReqStep)
		{
			DVDReqStep = 6;
			if (pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFileErrorCheck)
			{
				for (i = 0;i < pForyouDVDInfo->sForyouDVDInfo.pFileCount;i++)
				{
					if (!pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[i].bStorage)
					{
						j = i+1;
						while (j < pForyouDVDInfo->sForyouDVDInfo.pFileCount &&
							(!pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[j].bStorage))
						{
							j++;
						}
						
						control_DVD_ReqFileFolderDetailedInfo(pForyouDVDInfo, TRUE,i,j-i);
						return DVDReqStep;
						
						
					}
				}
			
				if (i == pForyouDVDInfo->sForyouDVDInfo.pFileCount)
				{
					DBG3(debugString("FlyAudio FileList check OK!\n");)
					pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFileErrorCheck = FALSE;
					pForyouDVDInfo->sForyouDVDInfo.pBGetFileFinish = TRUE;
				}
			}
		}
		if (6 == DVDReqStep)
		{
			DVDReqStep = 7;
			if (0xFF == pForyouDVDInfo->sForyouDVDInfo.iDVDReturnRegionCode)
			{
				control_DVD_ReqRegionCode(pForyouDVDInfo);
				DBG3(debugOneData("\nstep:",DVDReqStep);)
				return DVDReqStep;
			}
		}
		if (7 == DVDReqStep)//确保碟机开启自动发送
		{
			DVDReqStep = 8;
			if (GetTickCount() - pForyouDVDInfo->sForyouDVDInfo.bDeviceRec89 > 3000)
			{
				pForyouDVDInfo->sForyouDVDInfo.bDeviceRec89 = GetTickCount();
				control_DVD_PlayBack_DisplayInfo_State_Request(pForyouDVDInfo, TRUE);
				DBG3(debugOneData("\nstep:",DVDReqStep);)
				return DVDReqStep;
			}
		}
		if (8 == DVDReqStep)//查询碟机程序版本号，确保有一个放在最后一直发送
		{
			DVDReqStep = 0;
			//if (0x00 == pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0])
			//{
				control_DVD_ReqDVDSoftwareVersion(pForyouDVDInfo);
				DBG3(debugOneData("\nstep:",DVDReqStep);)
				return DVDReqStep;
			//}
		}
		//if (9 == DVDReqStep)//循环查询各个设备状态，确保有一个放在最后一直发送
		//{
		//	DVDReqStep = 0;
		//	pForyouDVDInfo->sForyouDVDInfo.CurrentReqMechanismCircle++;
		//	pForyouDVDInfo->sForyouDVDInfo.CurrentReqMechanismCircle %= 2;
		//	control_DVD_ReqMechanismState(pForyouDVDInfo->sForyouDVDInfo.CurrentReqMechanismCircle);
		//	DLOGD("step:",DVDReqStep);
		//	return DVDReqStep;
		//} 	
		if (bCheck)//大意是重新检查一次
		{
			bCheck = FALSE;
		} 
		else
		{
			bCheck = TRUE;
		}
	} while (bCheck);
	DVDReqStep = 0;
	return DVDReqStep;
}

static void procDVDStop(struct flydvd_struct_info *pForyouDVDInfo,BOOL bForceSend)
{
	if (bForceSend || GetTickCount() - pForyouDVDInfo->sForyouDVDInfo.iDVDStateCheckTime >= 3000)//一定要控到STOP
	{
		pForyouDVDInfo->sForyouDVDInfo.iDVDStateCheckTime = GetTickCount();
		if (0x17 == pForyouDVDInfo->sForyouDVDInfo.bDVDRequestState
			&& 0x00 != pForyouDVDInfo->sForyouDVDInfo.bDVDResponseState)
		{
			debugString("\nForyou Force DVD State");
			control_DVD_IR_CMD(pForyouDVDInfo,0x17);
		}
	}
}
static void processForyouDVD(struct flydvd_struct_info *pForyouDVDInfo,INT ret)
{
	if (pForyouDVDInfo->flydvd_fd > 0)//串口OK
	{
		if (pForyouDVDInfo->bPower)
		{
			if (!pForyouDVDInfo->bPowerUp)
			{
				pForyouDVDInfo->bPowerUp = TRUE;
				DVD_LEDControl_Off(pForyouDVDInfo);
#if FORYOU_DVD_RESET_FLYER
				pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize = TRUE;
				returnDVDDeviceWorkMode(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize);
				control_DVD_PlayBack_DisplayInfo_State_Request(pForyouDVDInfo, TRUE);
				if (0xFF != pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0])
				{
					returnDVDDeviceActionState(pForyouDVDInfo, 0,pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0]);
				}
				control_DVD_ReqMechanismState(pForyouDVDInfo, 0);
#else
				if (!pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)//ggggggggggggggggggggggggggg
				{
					DBG0(debugString("DVD Reset....\n");)
#if DVD_DEBUG
					dvd_debugString("DVD Reset....\n");
#endif
					DVD_Reset_On(pForyouDVDInfo);
					Sleep(100);
					DVD_Reset_Off(pForyouDVDInfo);
				}
#endif
			}
		}

		if (pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize)
		{
#if FORYOU_DVD_BUG_FIX
			if (pForyouDVDInfo->sForyouDVDInfo.bNeedReturnNoDiscAfterClose)
			{
				if (GetTickCount() - pForyouDVDInfo->sForyouDVDInfo.iNeedReturnNoDiscAfterCloseTime >= 618*5)
				{
					pForyouDVDInfo->sForyouDVDInfo.bNeedReturnNoDiscAfterClose = FALSE;
					if (0x03 == pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0])
					{
						pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0] = 0x00;
						returnDVDDeviceActionState(pForyouDVDInfo, 0,pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[0]);
					}
				}
			}
#endif
			//DBG1(debugString("\nFlyAudio processForyouDVD-> procDVDStop LDH:");)
			procDVDStop(pForyouDVDInfo,FALSE);

			if (pForyouDVDInfo->sForyouDVDInfo.bNeedSend8CTime)//CD-Text
			{
				if (GetTickCount() - pForyouDVDInfo->sForyouDVDInfo.bNeedSend8CTime >= 100)
				{
					pForyouDVDInfo->sForyouDVDInfo.bNeedSend8CTime = 0;
					if (
						((pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0] << 8) + pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[1])
						>= ((((11 << 4) + 11) << 8) + 11)
						)//确定版本号，11.11.11之后的都要有
					{
						if (0x03 == pForyouDVDInfo->sForyouDVDInfo.MediaFileType)//CDDA
						{
							control_DVD_ID3CDText(pForyouDVDInfo,0x51,1,pForyouDVDInfo->sForyouDVDInfo.pFileCount);
						}
					}
				}
			}
			if (ipcWhatEventOn(EVENT_GLOBAL_DVD_REGION_SET_ID))
			{
				ipcClearEvent(EVENT_GLOBAL_DVD_REGION_SET_ID);
				control_DVD_ControlRegionCode(pForyouDVDInfo,pFlyAllInOneInfo->pMemory_Share_Common->flyRestoreData.iDVDRegionCode);
				pForyouDVDInfo->sForyouDVDInfo.iDVDReturnRegionCode = 0xFF;//重新查询
			}


			if (pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos)
			{
				DBG2(debugString("\nForyou Have Jump");)
				if (GetTickCount() - pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextTimer >= 618)
				{
					DBG2(debugString("\nForyou Have Jump On Time");)
					if (0x02 == pForyouDVDInfo->sForyouDVDInfo.MediaDiscType)//Clips
					{
						if (1 == pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount)
						{
							if (pForyouDVDInfo->sForyouDVDInfo.bQuickJumpNext)
							{
								control_DVD_IR_CMD(pForyouDVDInfo,0x18);
							}
							else
							{
								control_DVD_IR_CMD(pForyouDVDInfo,0x19);
							}
						}
						else
						{
							selectFolderOrFileByLocalIndex(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowPlayingInWhatFolder,pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos-1,FALSE);
						}
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount = 0;
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = 0;
					}
					else if (pForyouDVDInfo->sForyouDVDInfo.bQuickJumpNext)
					{
						DBG2(debugOneData("\nForyou Have Jump Next ",pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos);)
						if (1 == pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos)
						{
							control_DVD_IR_CMD(pForyouDVDInfo,0x18);
						}
						else
						{
							control_DVD_JumpNextN(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos);
						}
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount = 0;
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = 0;
					}
					else
					{
						DBG2(debugOneData("\nForyou Have Jump Prev ",pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos);)
						if (1 == pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos)
						{
							control_DVD_IR_CMD(pForyouDVDInfo,0x19);
						}
						else
						{
							control_DVD_JumpPrevN(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos);
						}
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount = 0;
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = 0;
					}
				}
			}

			if (pForyouDVDInfo->sForyouDVDInfo.curVideoAspect !=pForyouDVDInfo->sForyouDVDInfo.preVideoAspect)
			{
				//视频比例切换
				pForyouDVDInfo->sForyouDVDInfo.curVideoAspect = pForyouDVDInfo->sForyouDVDInfo.preVideoAspect;

				control_DVD_IR_CMD(pForyouDVDInfo, 0x17);
				Sleep(100);
				if (0x00 == pForyouDVDInfo->sForyouDVDInfo.curVideoAspect)
				{
					control_DVD_Set_View_Mode(pForyouDVDInfo, 0x04);
					Sleep(1000);
					control_DVD_Video_Aspect_Radio(pForyouDVDInfo, 0x01);
					Sleep(1000);
				}
				else if (0x01 == pForyouDVDInfo->sForyouDVDInfo.curVideoAspect)
				{
					control_DVD_Set_View_Mode(pForyouDVDInfo, 0x04);
					Sleep(1000);
					control_DVD_Video_Aspect_Radio(pForyouDVDInfo, 0x00);
					Sleep(1000);
				}
				else if (0x02 == pForyouDVDInfo->sForyouDVDInfo.curVideoAspect)
				{
					control_DVD_Set_View_Mode(pForyouDVDInfo, 0x00);
					Sleep(1000);//xxxxxxxxxxxxxxxxxxxxxxxxxxx
					//control_DVD_Video_Aspect_Radio(0x01);
					//Sleep(1000);
				}

				control_DVD_IR_CMD(pForyouDVDInfo, 0x14);
			}

			if (pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile)
			{
				if (0 == pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount)//控制状态跳转
				{
					if (pForyouDVDInfo->sForyouDVDInfo.pReturnE8)
					{
						if (pForyouDVDInfo->sForyouDVDInfo.pFolderCount)
						{
							pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 1;
						} 
						else if (pForyouDVDInfo->sForyouDVDInfo.pFileCount)
						{
							pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 2;
						}
						else
						{
							pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 3;
						}
					}
				}
				else if (1 == pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount)
				{
					if (pForyouDVDInfo->sForyouDVDInfo.pReturnEAFolder)
					{
						if (pForyouDVDInfo->sForyouDVDInfo.pFileCount)
						{
							pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 2;
						}
						else
						{
							pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 3;
						}
					}
				}
				else if (2 == pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount)
				{
					if (pForyouDVDInfo->sForyouDVDInfo.pReturnEAFile)
					{
						pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount = 3;
					}
				}

				if (0 == pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount)//执行状态操作
				{
					if (ETIMEDOUT == ret || pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow)
					{
						control_DVD_ReqFileFolderCount(pForyouDVDInfo);
					}
				}
				else if (1 == pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount)
				{
					if (ETIMEDOUT == ret || pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow)
					{
						control_DVD_ReqFileFolderDetailedInfo(pForyouDVDInfo, 0,0,pForyouDVDInfo->sForyouDVDInfo.pFolderCount);
					}				
				}
				else if (2 == pForyouDVDInfo->sForyouDVDInfo.pStartGetFolderFileCount)
				{
					if (ETIMEDOUT == ret || pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow)
					{
						control_DVD_ReqFileFolderDetailedInfo(pForyouDVDInfo, 1,0,pForyouDVDInfo->sForyouDVDInfo.pFileCount);
					}
				}
				else
				{
					DBG0(debugString("ForyouDVD End get folder and file list\n");)
					pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc = TRUE;
					 
					pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = FALSE;
					pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck = TRUE;
					pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFileErrorCheck = TRUE;
				}

				pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = FALSE;
			}
			else if (ETIMEDOUT == ret)//如果超时退出
			{
				pForyouDVDInfo->sForyouDVDInfo.DVDReqStep = DVDReqStepChangeAndReq(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.DVDReqStep);
			}
			else
			{

			}
		}
	}
}

void *ledFlashThread(void *arg)
{
	struct flydvd_struct_info *pForyouDVDInfo = (struct flydvd_struct_info *)arg;
	
	while (!pForyouDVDInfo->bKillLEDFlashThread)
	{
		sem_wait(&pForyouDVDInfo->LED_Flash_sem);
		DVD_LEDControl_On(pForyouDVDInfo);Sleep(365);DVD_LEDControl_Off(pForyouDVDInfo);Sleep(365);
		DVD_LEDControl_On(pForyouDVDInfo);Sleep(365);DVD_LEDControl_Off(pForyouDVDInfo);Sleep(365);
		DVD_LEDControl_On(pForyouDVDInfo);Sleep(365);DVD_LEDControl_Off(pForyouDVDInfo);Sleep(365);
		DVD_LEDControl_On(pForyouDVDInfo);Sleep(365);DVD_LEDControl_Off(pForyouDVDInfo);Sleep(365);
	}

	return NULL;
}


void *searchDataThread(void *arg)
{	
	INT ret = 0;
	UINT16 reqDVDStep = 0;
	INT dwRead = 0;
	struct timeval timenow;
	struct timespec timeout;
	struct flydvd_struct_info *pForyouDVDInfo = (struct flydvd_struct_info *)arg;
	BYTE buff[10];
	
	DBG3(debugString("searchDataThread start\n");)
	while (!pForyouDVDInfo->bKillSearchDataThread)
	{
		if (pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile)
		{
			ret = WaitSignedTimeOut(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain,500);
		}
		else
		{
			ret = WaitSignedTimeOut(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain,615);
		}
		
		if (pFlyAllInOneInfo->pMemory_Share_Common->bSilencePowerUp)
		{
			DVD_Reset_On(pForyouDVDInfo);
			continue;
		}

		if (!pForyouDVDInfo->bEnterUpdateMode
			&& pForyouDVDInfo->bAutoResetControlOn
			&& !pForyouDVDInfo->bFlyaudioDVD)//碟机死机的复位
		{
			if (GetTickCount() - pForyouDVDInfo->iAutoResetControlTime >= 150*1000)
			{
				DBG0(debugOneData("\nDVD reset time:",GetTickCount() - pForyouDVDInfo->iAutoResetControlTime);)
#if DVD_DEBUG
				dvd_debugOneData("\nDVD reset time:",GetTickCount() - pForyouDVDInfo->iAutoResetControlTime);
#endif 
				DVD_Reset_On(pForyouDVDInfo);
				Sleep(100);
				DVD_Reset_Off(pForyouDVDInfo);
			}
		}
		
		if (pForyouDVDInfo->bFlyaudioDVD)
		{
			processFlyaudioDVD(pForyouDVDInfo, ret);
		}
		else
		{
			processForyouDVD(pForyouDVDInfo, ret);
		}
	}
	DBG0(debugString("searchDataThread exit\n");)
	return NULL;
}

static int createThread(struct flydvd_struct_info *pForyouDVDInfo)
{
	INT ret = -1;
	INT res;
	pthread_t thread_id;
	
	pForyouDVDInfo->bKillLEDFlashThread = FALSE;
	res = pthread_create(&thread_id,NULL,ledFlashThread,pForyouDVDInfo);
	DBG0(debugOneData("\nledFlashThread ID:",thread_id);)
	if(res != 0) 
	{
		return ret;
	}

	pForyouDVDInfo->bKillSearchDataThread = FALSE;
	res = pthread_create(&thread_id,NULL,searchDataThread,pForyouDVDInfo);
	DBG0(debugOneData("\nsearchDataThread ID:",thread_id);)
	if(res != 0) 
	{
		return ret;
	}

	pForyouDVDInfo->bKillReadSerialThread = FALSE;
	res = pthread_create(&thread_id, NULL,read_serial_thread,pForyouDVDInfo);
	DBG0(debugOneData("\nread_serial_thread ID:",thread_id);)
	if(res != 0) 
	{
		return ret;
	}
	
	return 0;
}

  
 static void onRightDataProcessor(struct flydvd_struct_info *pForyouDVDInfo,BYTE *buf, UINT16 len)
 {
	switch (buf[0])
	{
		case 0x01:
			if (0x01 == buf[1])
			{
				DBG0(debugString("\nDVD init");)
#if DVD_DEBUG
				dvd_debugString("\nDVD init");
				dvd_debugOneData("\nDVD powerUp status:",pForyouDVDInfo->bPowerUp);
#endif
				pForyouDVDInfo->bPower = TRUE;
				returnDVDDevicePowerMode(pForyouDVDInfo, pForyouDVDInfo->bPower);
				
				if (!pForyouDVDInfo->bPowerUp)
				{
					createThread(pForyouDVDInfo);
				}
				else if (pForyouDVDInfo->sForyouDVDInfo.pFileCount != 0 ||
							pForyouDVDInfo->sForyouDVDInfo.pFolderCount != 0)
				{
					
					returnDVDDeviceMedia(pForyouDVDInfo, 
							pForyouDVDInfo->sForyouDVDInfo.MediaDiscType,
								pForyouDVDInfo->sForyouDVDInfo.MediaFileType);
			 
					returnDVDCurrentTitleTrack(pForyouDVDInfo, 
							pForyouDVDInfo->sForyouDVDInfo.CurrentTitle,
								pForyouDVDInfo->sForyouDVDInfo.CurrentChar);
								

					returnDVDNowPlayingFileInfo(pForyouDVDInfo, 
							&pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount],
							getSelectFolderFileIndexByGlobalIndexInParent(pForyouDVDInfo,FALSE,pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount)
							,pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount);
					
					returnDVDNowPlayingInFolderName(pForyouDVDInfo, 
							&pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount].parentFolderIndex]);
				}
			}
			else if (0x00 == buf[1])
			{
				pForyouDVDInfo->bPower = FALSE;
#if DVD_DEBUG
				dvd_debugString("\ndvd start to returnDVDDevicePowerMode");
#endif

				returnDVDDevicePowerMode(pForyouDVDInfo, pForyouDVDInfo->bPower);
			}
			break;
		case 0x10:
			pForyouDVDInfo->sForyouDVDInfo.bDVDRequestState = buf[1];
			pForyouDVDInfo->sForyouDVDInfo.iDVDStateCheckTime = GetTickCount();

			if (0x18 == buf[1])//Next
			{
				if (0x02 == pForyouDVDInfo->sForyouDVDInfo.MediaDiscType
					&& pForyouDVDInfo->sForyouDVDInfo.PlayMode != 0x01
					&& pForyouDVDInfo->sForyouDVDInfo.PlayMode != 0x30)//Clips碟片直接选取
				{
					if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
					{
						pForyouDVDInfo->sForyouDVDInfo.bQuickJumpNext = TRUE;
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount++;
						if (0 == pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos)
						{
							pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = pForyouDVDInfo->sForyouDVDInfo.CurrentChar + 1;
						}
						else
						{
							pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos++;
						}
						if (pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos
							>= pForyouDVDInfo->sForyouDVDInfo.TotalChar)
						{
							pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = pForyouDVDInfo->sForyouDVDInfo.TotalChar;
						}
						returnDVDCurrentTitleTrack(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.CurrentTitle
							,pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos);
					}
				}
				else
				{
					control_DVD_IR_CMD(pForyouDVDInfo,0x18);
				}
				pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextTimer = GetTickCount();
			}
			else if (0x19 == buf[1])//Prev
			{
				if (0x02 == pForyouDVDInfo->sForyouDVDInfo.MediaDiscType
					&& pForyouDVDInfo->sForyouDVDInfo.PlayMode != 0x01
					&& pForyouDVDInfo->sForyouDVDInfo.PlayMode != 0x30)//Clips碟片直接选取
				{
					if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
					{
						pForyouDVDInfo->sForyouDVDInfo.bQuickJumpNext = FALSE;
						pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextCount++;
						if (0 == pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos)
						{
							pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = pForyouDVDInfo->sForyouDVDInfo.CurrentChar - 1;
						}
						else
						{
							pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos--;
						}				
						if (0 == pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos)
						{
							pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos = 1;
						}
						returnDVDCurrentTitleTrack(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.CurrentTitle
							,pForyouDVDInfo->sForyouDVDInfo.iQuickJumpTitlePos);
					}
				}
				else
				{
					control_DVD_IR_CMD(pForyouDVDInfo,0x19);
				}
				pForyouDVDInfo->sForyouDVDInfo.iQuickJumpNextTimer = GetTickCount();
			}
			else
			{
				control_DVD_IR_CMD(pForyouDVDInfo,buf[1]);
				Sleep(100);
			}
			break;
			//case 0x11:
			//	if (0x00 == buf[1])
			//	{
			//		control_DVD_IR_CMD(pForyouDVDInfo,0x62);
			//	} 
			//	else if (0x01 == buf[1])
			//	{
			//		control_DVD_IR_CMD(pForyouDVDInfo,0x64);
			//	}
			//	else if (0x02 == buf[1])
			//	{
			//		control_DVD_IR_CMD(pForyouDVDInfo,0x65);
			//	}
			//	else if (0x03 == buf[1])
			//	{
			//		control_DVD_IR_CMD(pForyouDVDInfo,0x63);
			//	}
			//	break;
		case 0x12://为了不引起混乱，只有接口处把根目录计算在数量内
			pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder
				= selectFolderOrFileByLocalIndex(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,(UINT16)(buf[1]*256 + buf[2]),TRUE);
			break;
		case 0x13://为了不引起混乱，只有接口处把根目录计算在数量内
			if (0 == pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder)//当前在根目录
			{
				pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart = (UINT16)(buf[2]*256 + buf[3]);
				pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount = buf[1];
			}
			else
			{
				if (0 == (UINT16)(buf[2]*256 + buf[3]))//从上一级目录开始
				{
					if (buf[1])
					{
						BYTE nameFolder[] = {0,0};
						returnDVDFileFolderInfo(pForyouDVDInfo, TRUE,nameFolder,2,0,0);//上一级目录
						pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart = (UINT16)(buf[2]*256 + buf[3]);
						pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount = buf[1] - 1;
					}
					else
					{
						pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart = (UINT16)(buf[2]*256 + buf[3]);
						pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount = 0;
					}
				}
				else
				{
					pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart = (UINT16)(buf[2]*256 + buf[3] - 1);
					pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount = buf[1];
				}
			}
			pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount = 0;

			if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)//已完成接收
			{
				if (pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount != pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount)//需要返回
				{
					UINT16 isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,TRUE);
					UINT16 isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,FALSE);
					INT isWhere;
					while (pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount != pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount)//需要返回
					{
						DBG2(debugString(" More");)
							if ((pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount) < isFolderCount)//返回文件夹
							{
								isWhere = getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,TRUE
									,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount);
								DBG2(debugOneData("\nFolder:",isWhere);)
									if (-1 != isWhere)
									{
										if (0 == pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder)//当前是根目录
										{
											returnDVDFileFolderInfo(pForyouDVDInfo, TRUE
												,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[isWhere].name
												,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[isWhere].nameLength
												,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount
												,isWhere);
										}
										else
										{
											returnDVDFileFolderInfo(pForyouDVDInfo, TRUE
												,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[isWhere].name
												,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[isWhere].nameLength
												,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount + 1
												,isWhere);
										}
										pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount++;
									}
									else
									{
										pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount = pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount;
									}
							}
							else if ((pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount) < (isFolderCount + isFileCount))//返回文件
							{
								isWhere = getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,FALSE
									,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount - isFolderCount);
								DBG2(debugOneData("\nFile:",isWhere);)
									if (-1 != isWhere)
									{
										if (0 == pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder)//当前是根目录
										{
											returnDVDFileFolderInfo(pForyouDVDInfo, FALSE
												,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[isWhere].name
												,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[isWhere].nameLength
												,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount
												,isWhere);
										}
										else
										{
											returnDVDFileFolderInfo(pForyouDVDInfo, FALSE
												,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[isWhere].name
												,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[isWhere].nameLength
												,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount + 1
												,isWhere);
										}
										pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount++;
									}
									else
									{
										pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount = pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount;
									}
							}
							else
							{
								pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount = pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount;
							}
					}
				}
			}
			break;
		case 0x15:
			if (0x00 == buf[1] || 0x01 == buf[1] || 0x02 == buf[1])
			{
				pForyouDVDInfo->sForyouDVDInfo.preVideoAspect = buf[1];
				PostSignal(&pForyouDVDInfo->searchDataMutex,&pForyouDVDInfo->searchDataCond,&pForyouDVDInfo->bSearchDataThreadRunAgain);
			}
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
	INT res;
	pthread_t thread_id;
	INT ret = HAL_ERROR_RETURN_FD;
	
	debugString("\nFlyDVD start open");
	
	if (pForyouDVDInfo->flydvd_fd)
	{
		return ret;
	}
	
	pForyouDVDInfo->flydvd_fd = serial_open();
	if(pForyouDVDInfo->flydvd_fd > 0)
	{
		debugString("\r\nFlyDVD  open ok ");
		ret = HAL_DVD_RETURN_FD;
	}
		
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
			
	if (pForyouDVDInfo)
	{
		return;
	}
		
	//为 flydvd_struct_info 结构体分配内存
	pForyouDVDInfo =
		(struct flydvd_struct_info *)malloc(sizeof(struct flydvd_struct_info));
	if (pForyouDVDInfo == NULL)
	{
		return;
	}
	memset(pForyouDVDInfo, 0, sizeof(struct flydvd_struct_info));
	
	sem_init(&pForyouDVDInfo->LED_Flash_sem, 0, 0);
	pthread_mutex_init(&pForyouDVDInfo->searchDataMutex, NULL);
	pthread_cond_init(&pForyouDVDInfo->searchDataCond, NULL);

	allInOneInit();
	
	pForyouDVDInfo->bDVDReturnDataToJNIResume = TRUE;
	pForyouDVDInfo->bPowerUp = FALSE;
	
	structDVDInfoInit(pForyouDVDInfo, TRUE);
		
	DBG0(debugString("\nFlyDvd hal init\n");)
	DBG0(debugString(__TIME__);)
	DBG0(debugString(__DATE__);)
	DBG0(debugString(" \n");)

#if DVD_DEBUG
	dvd_debugString("\nFlyDvd hal init\n");
	dvd_debugString(__TIME__);
	dvd_debugString(__DATE__);
	dvd_debugString(" \n");
#endif 
 }
 
  /********************************************************************************
 **函数名称：flydvd_read()函数
 **函数功能：读出数据
 **函数参数：
 **返 回 值：成功返回实际读得的数据，失败返回-1
 **********************************************************************************/
INT flyReadData(BYTE *buf, UINT16 len)
{
	UINT16 dwRead;
	
	dwRead = readFromJNIBuff(CURRENT_SHARE_MEMORY_ID,buf,len);
	DBG1(debugBuf("\nDVD-HAL return  bytes to User:", buf,dwRead);)
#if DVD_DEBUG
	dvd_debugBuf("\nDVD-HAL return  bytes to User ok:", buf,dwRead);
#endif 

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
	
	//等待串口读线程退出
	pForyouDVDInfo->bKillReadSerialThread = TRUE;
	pForyouDVDInfo->bKillSearchDataThread = TRUE;
		 
	//释放一个条件变量
	pthread_cond_destroy(&pForyouDVDInfo->searchDataCond);
	
	allInOneDeinit();
	
	free(pForyouDVDInfo);
	pForyouDVDInfo = NULL;
	
#if DVD_DEBUG
	dvd_debugString("\nDVD HAL destory\n");
#endif 
}
 /********************************************************************************
 **函数名称：fly_close_device()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 INT flyCloseDevice(void)
 {
	
	if (pForyouDVDInfo->flydvd_fd > 0)
	{
		DVD_Reset_On(pForyouDVDInfo);//停止DVD
		
		if (!serial_close(pForyouDVDInfo->flydvd_fd))
		{
			DBG0(debugString("close serial ok!\n");)
			return 0;
		}
	}
	else
	{
		DBG0(debugString("FlyDVD not open\n");) 
	}
	
	DBG0(debugString("\nFlyDVD close");)
#if DVD_DEBUG
	dvd_debugString("\nFlyDVD close");
#endif 

	return -1;
 }

 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
void flyCommandProcessor(BYTE *buf, UINT16 len)
{
	DBG0(debugBuf("\nUser write bytes to DVD-HAL:", buf,len);)
	
#if DVD_DEBUG
	dvd_debugBuf("\nUser write bytes to DVD-HAL:", buf,len);
#endif 

	onRightDataProcessor(pForyouDVDInfo,&buf[3], buf[2]-1);

}




