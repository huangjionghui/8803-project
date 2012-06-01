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

#include "FlyDvd.h"
#include "../../include/allInOneOthers.h"
#define FLYAUDIO_DVD_FILE_MAX	2500
#define FLYAUDIO_DVD_FOLDER_MAX	500

BYTE FlyaudioDVDReqStepChangeAndReq(struct flydvd_struct_info *pForyouDVDInfo,BYTE DVDReqStep);

void transFlyaudioInfoCB(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice != p[0])
	{
		pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice = p[0];
		returnDVDDevicePlayDevice(pForyouDVDInfo, pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismDevice);
	}
	if (pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] != p[1])
	{
		if (0x02 == p[1])
		{
			DBG0(debugString("ForyouDVD LED Flash\n");)
			pForyouDVDInfo->sForyouDVDInfo.LEDFlash = TRUE;
			sem_post(&pForyouDVDInfo->LED_Flash_sem);
		}
		pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]] = p[1];
		returnDVDDeviceActionState(pForyouDVDInfo, p[0],p[1]);
		if (0x09 != pForyouDVDInfo->sForyouDVDInfo.CurrentMechanismStatus[p[0]])
		{
			structDVDInfoInit(pForyouDVDInfo, FALSE);
		}
	}
}
void transFlyaudioInfoCC(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	if (pForyouDVDInfo->sForyouDVDInfo.PlayStatus != p[0]
		|| pForyouDVDInfo->sForyouDVDInfo.PlaySpeed != p[1])
	{
		pForyouDVDInfo->sForyouDVDInfo.PlayStatus = p[0];
		pForyouDVDInfo->sForyouDVDInfo.PlaySpeed = p[1];
		returnDVDPlayStatusSpeed(pForyouDVDInfo,
			pForyouDVDInfo->sForyouDVDInfo.PlayStatus,
			pForyouDVDInfo->sForyouDVDInfo.PlaySpeed);
	}
}

void transFlyaudioInfoCD(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	if (pForyouDVDInfo->sForyouDVDInfo.MediaDiscType != p[0]
	|| pForyouDVDInfo->sForyouDVDInfo.MediaFileType != p[1])
	{
		pForyouDVDInfo->sForyouDVDInfo.MediaDiscType = p[0];
		pForyouDVDInfo->sForyouDVDInfo.MediaFileType = p[1];
		returnDVDDeviceMedia(pForyouDVDInfo,
			pForyouDVDInfo->sForyouDVDInfo.MediaDiscType,
			pForyouDVDInfo->sForyouDVDInfo.MediaFileType);
	}
}

void transFlyaudioInfoCE(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	if (pForyouDVDInfo->sForyouDVDInfo.EscapeSecond != p[2]
	|| pForyouDVDInfo->sForyouDVDInfo.EscapeMinute != p[1]
	|| pForyouDVDInfo->sForyouDVDInfo.EscapeHour != p[0])
	{
		pForyouDVDInfo->sForyouDVDInfo.EscapeHour = p[0];
		pForyouDVDInfo->sForyouDVDInfo.EscapeMinute = p[1];
		pForyouDVDInfo->sForyouDVDInfo.EscapeSecond = p[2];
		returnDVDCurrentTime(pForyouDVDInfo,
			pForyouDVDInfo->sForyouDVDInfo.EscapeHour,
			pForyouDVDInfo->sForyouDVDInfo.EscapeMinute,
			pForyouDVDInfo->sForyouDVDInfo.EscapeSecond);
	}
}

void transFlyaudioInfoCF(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
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
}

void transFlyaudioInfoDA(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{	
	if(pForyouDVDInfo->sForyouDVDInfo.PlayMode != p[0])
	{
		pForyouDVDInfo->sForyouDVDInfo.PlayMode = p[0];//播放模式
		returnDVDPlayMode(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.PlayMode);
	}
}
void transFlyaudioInfoEB(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	pForyouDVDInfo->sForyouDVDInfo.bRecFileFolderUseEB = TRUE;
	if (0x00 == p[0])//开始
	{
		listFileFolderNewAll(pForyouDVDInfo,TRUE,FLYAUDIO_DVD_FILE_MAX);//File
		listFileFolderNewAll(pForyouDVDInfo,FALSE,FLYAUDIO_DVD_FOLDER_MAX);//Folder
		pForyouDVDInfo->sForyouDVDInfo.pFileCount = 0;
		pForyouDVDInfo->sForyouDVDInfo.pFolderCount = 0;

		pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = TRUE;

		pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = TRUE;

		pForyouDVDInfo->sForyouDVDInfo.bFinishGetFileFolderEB = FALSE;
	}
	else if (0x01 == p[0])//结束
	{
		pForyouDVDInfo->sForyouDVDInfo.bFinishGetFileFolderEB = TRUE;
	}
}

void transFlyaudioInfoE8(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	pForyouDVDInfo->sForyouDVDInfo.pReturnE8 = TRUE;
	pForyouDVDInfo->sForyouDVDInfo.pFileCount = p[2]*256 + p[3];
	pForyouDVDInfo->sForyouDVDInfo.pFolderCount = p[0]*256 + p[1];

	DBG2(debugOneData("\nFileCount:", pForyouDVDInfo->sForyouDVDInfo.pFileCount);)
	DBG2(debugOneData("\nFolderCount:",pForyouDVDInfo->sForyouDVDInfo.pFolderCount);)

	listFileFolderNewAll(pForyouDVDInfo,TRUE,pForyouDVDInfo->sForyouDVDInfo.pFileCount);
	listFileFolderNewAll(pForyouDVDInfo,FALSE,pForyouDVDInfo->sForyouDVDInfo.pFolderCount);

	pForyouDVDInfo->sForyouDVDInfo.pBReqFolderFileCommandActiveNow = TRUE;

	pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = TRUE;

	pForyouDVDInfo->sForyouDVDInfo.bFinishGetFileFolderEB = TRUE;

	pthread_cond_signal(&pForyouDVDInfo->searchDataCond);
}

void transFlyaudioInfoEA(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	DBG3(debugBuf("\nFile::", &p[0], 5);)
	if (pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile || pForyouDVDInfo->sForyouDVDInfo.pBHaveGetFolderFile)
	{
		pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = FALSE;
		pForyouDVDInfo->sForyouDVDInfo.pBHaveGetFolderFile = TRUE;

		if (p[4])
		{
			if (pForyouDVDInfo->sForyouDVDInfo.bRecFileFolderUseEB)
			{
				if (pForyouDVDInfo->sForyouDVDInfo.pFileCount < p[0]*256 + p[1] + 1)
				{
					pForyouDVDInfo->sForyouDVDInfo.pFileCount = p[0]*256 + p[1] + 1;
				}
				if (pForyouDVDInfo->sForyouDVDInfo.pFileCount > FLYAUDIO_DVD_FILE_MAX)
				{
					pForyouDVDInfo->sForyouDVDInfo.pFileCount = FLYAUDIO_DVD_FILE_MAX;
				}
			}
			listFileFolderStorageOne(pForyouDVDInfo,TRUE,p[0]*256 + p[1],p[2]*256 + p[3],p[5],&p[6],len-6);
		}
		else
		{
			if (pForyouDVDInfo->sForyouDVDInfo.bRecFileFolderUseEB)
			{
				if (pForyouDVDInfo->sForyouDVDInfo.pFolderCount < p[0]*256 + p[1] + 1)
				{
					pForyouDVDInfo->sForyouDVDInfo.pFolderCount = p[0]*256 + p[1] + 1;
				}
				if (pForyouDVDInfo->sForyouDVDInfo.pFolderCount > FLYAUDIO_DVD_FOLDER_MAX)
				{
					pForyouDVDInfo->sForyouDVDInfo.pFolderCount = FLYAUDIO_DVD_FOLDER_MAX;
				}
			}
			listFileFolderStorageOne(pForyouDVDInfo,FALSE,p[0]*256 + p[1],p[2]*256 + p[3],p[5],&p[6],len-6);
		}

		pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck = TRUE;
		pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFileErrorCheck = TRUE;
	}
	else
	{
		DBG2(debugString("\nFile And Folder List Not Init");)
	}

}

void transFlyaudioInfoE0(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT len)
{
	UINT highLightIndex = p[0]*256+p[1];
	DBG0(debugOneData("\nHigh Light:",highLightIndex);)
	if ((highLightIndex == 0) || (pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount != highLightIndex))
	{
		pForyouDVDInfo->sForyouDVDInfo.pNowReturnPlayingFileCount = highLightIndex;
		if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
		{
			procDVDInfoE0(pForyouDVDInfo);
		}
		else
		{
			pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc = TRUE;
			DBG0(debugString("\nForyouDVD Rec E0 But Haven't Finish Rec File Or Folder Info");)
		}
	}
}

void DealFlyaudioInfo(struct flydvd_struct_info *pForyouDVDInfo,BYTE *p,UINT length)
{
	UINT i;

	if ((0xEA != p[0]) && (0xE5 != p[0]))
	{
		DBG0(debugBuf("\nFlyAudio DVD commd:",p,length);)
	}
	
	
	pForyouDVDInfo->iAutoResetControlTime = GetTickCount();

	switch (p[0])
	{
	case 0xDB:
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
		
		structDVDInfoInit(pForyouDVDInfo,FALSE);

		returnDVDDeviceWorkMode(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.MechanismInitialize);
		break;
	case 0xCB:
		transFlyaudioInfoCB(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xCC:
		transFlyaudioInfoCC(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xCD:
		transFlyaudioInfoCD(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xCE:
		transFlyaudioInfoCE(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xCF:
		transFlyaudioInfoCF(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0XDA:
		transFlyaudioInfoDA(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xE8:
		transFlyaudioInfoE8(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xEA:
		transFlyaudioInfoEA(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xEB:
		transFlyaudioInfoEB(pForyouDVDInfo,&p[1],length-1);
		break;
	case 0xE0:
		transFlyaudioInfoE0(pForyouDVDInfo,&p[1],length-1);
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
		DBG2(debugString("\nunHandle!");)
		break;
	}
}

BYTE FlyaudioDVDReqStepChangeAndReq(struct flydvd_struct_info *pForyouDVDInfo,BYTE DVDReqStep)
{
	UINT i,j;
	BOOL bCheck = FALSE;
	do 
	{
		DBG2(debugOneData("\nStep ",DVDReqStep);)

		if (pForyouDVDInfo->sForyouDVDInfo.pBGetFolderFinish && pForyouDVDInfo->sForyouDVDInfo.pBGetFileFinish)
		{
			if (FALSE == pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)
			{
				DBG2(debugString("\nFlyAudio FolderFileList Start Return!");)
				pForyouDVDInfo->sForyouDVDInfo.pBStartGetFolderFile = FALSE;
				pForyouDVDInfo->sForyouDVDInfo.pBHaveGetFolderFile = FALSE;
				pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish = TRUE;
				pForyouDVDInfo->sForyouDVDInfo.bFinishGetFileFolderEB = FALSE;
				UINT isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,TRUE);
				UINT isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,FALSE);
				returnDVDCurrentFolderInfo(pForyouDVDInfo,isFolderCount+isFileCount,isFolderCount,TRUE);

				if (pForyouDVDInfo->sForyouDVDInfo.bRecE0AndNeedProc)
				{
					procDVDInfoE0(pForyouDVDInfo);
				}
			}
		}

		if (0 == DVDReqStep)
		{
			DVDReqStep = 1;
			if (pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck && pForyouDVDInfo->sForyouDVDInfo.bFinishGetFileFolderEB)
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
						control_DVD_ReqFileFolderDetailedInfo(pForyouDVDInfo,FALSE,i,j-i);
						DBG2(debugOneData("<%d>",DVDReqStep);)
						return DVDReqStep;
					}
				}
				if (i == pForyouDVDInfo->sForyouDVDInfo.pFolderCount)
				{
					DBG2(debugString("\nFlyAudio FolderList check OK!");)
						pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFolderErrorCheck = FALSE;
					pForyouDVDInfo->sForyouDVDInfo.pBGetFolderFinish = TRUE;
				}
			}
		}
		if (1 == DVDReqStep)
		{
			DVDReqStep = 2;
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
						control_DVD_ReqFileFolderDetailedInfo(pForyouDVDInfo,TRUE,i,j-i);
						DBG2(debugOneData("\n<%d>",DVDReqStep);)
						return DVDReqStep;
					}
				}
				if (i == pForyouDVDInfo->sForyouDVDInfo.pFileCount)
				{
					DBG2(debugString("\nFlyAudio FileList check OK!");)
						pForyouDVDInfo->sForyouDVDInfo.pBFolderFileListFileErrorCheck = FALSE;
					pForyouDVDInfo->sForyouDVDInfo.pBGetFileFinish = TRUE;
				}
			}
		}
		if (2 == DVDReqStep)//查询碟机程序版本号，确保有一个放在最后一直发送
		{
			DVDReqStep = 0;
			//if (0x00 == pForyouDVDInfo->sForyouDVDInfo.iSoftwareVersion[0])
			//{
			control_DVD_ReqDVDSoftwareVersion(pForyouDVDInfo);
			DBG2(debugOneData("<%d>",DVDReqStep);)
			return DVDReqStep;
			//}
		}
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

void processFlyaudioDVD(struct flydvd_struct_info *pForyouDVDInfo,ULONG WaitReturn)
{
	//if (pForyouDVDInfo->hDVDComm && pForyouDVDInfo->hDVDComm != INVALID_HANDLE_VALUE)//串口OK
	{
		if (pForyouDVDInfo->bPower)
		{
			if (!pForyouDVDInfo->bPowerUp)
			{
				pForyouDVDInfo->bPowerUp = TRUE;
				DVD_LEDControl_Off(pForyouDVDInfo);

				//if (!pFlyAllInOneInfo->pMemory_Share_Common->bStandbyStatus)
				{
					DVD_Reset_On(pForyouDVDInfo);
					Sleep(100);
					DVD_Reset_Off(pForyouDVDInfo);
				}
			}
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

			control_DVD_IR_CMD(pForyouDVDInfo,0x17);
			Sleep(100);
			if (0x00 == pForyouDVDInfo->sForyouDVDInfo.curVideoAspect)
			{
				control_DVD_Set_View_Mode(pForyouDVDInfo,0x04);
				Sleep(1000);
				control_DVD_Video_Aspect_Radio(pForyouDVDInfo,0x01);
				Sleep(1000);
			}
			else if (0x01 == pForyouDVDInfo->sForyouDVDInfo.curVideoAspect)
			{
				control_DVD_Set_View_Mode(pForyouDVDInfo,0x04);
				Sleep(1000);
				control_DVD_Video_Aspect_Radio(pForyouDVDInfo,0x00);
				Sleep(1000);
			}
			else if (0x02 == pForyouDVDInfo->sForyouDVDInfo.curVideoAspect)
			{
				control_DVD_Set_View_Mode(pForyouDVDInfo,0x00);
				Sleep(1000);
				//control_DVD_Video_Aspect_Radio(pForyouDVDInfo,0x01);
				//Sleep(1000);
			}

			control_DVD_IR_CMD(pForyouDVDInfo,0x14);
		}

		if (pForyouDVDInfo->sForyouDVDInfo.pBGetFileFolderFinish)//已完成接收
		{
			if (pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount != pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount)//需要返回
			{
				UINT isFolderCount = getSelectParentFolderFileCount(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,TRUE);
				UINT isFileCount = getSelectParentFolderFileCount(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,FALSE);
				INT isWhere;
				//DBG2(RETAILMSG(1, (TEXT("\r\nFlyAudio ThreadFlyForyouDVDProc Need Return In:%d TotalFolder:%d TotalFile:%d"),pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,isFolderCount,isFileCount));)
					while (pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount != pForyouDVDInfo->sForyouDVDInfo.pNeedReturnCount)//需要返回
					{
						DBG2(debugString("\nMore");)
							if ((pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount) < isFolderCount)//返回文件夹
							{
								isWhere = getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,TRUE
									,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount);
								DBG2(debugOneData("\nFolder:",isWhere);)
									if (-1 != isWhere)
									{
										if (0 == pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder)//当前是根目录
										{
											returnDVDFileFolderInfo(pForyouDVDInfo,TRUE
												,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[isWhere].name
												,pForyouDVDInfo->sForyouDVDInfo.pFolderTreeList[isWhere].nameLength
												,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount
												,isWhere);
										}
										else
										{
											returnDVDFileFolderInfo(pForyouDVDInfo,TRUE
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
								isWhere = getSelectParentFolderFileInfoBySubIndex(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder,FALSE
									,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount - isFolderCount);
								DBG2(debugOneData("\nFile:",isWhere);)
									if (-1 != isWhere)
									{
										if (0 == pForyouDVDInfo->sForyouDVDInfo.pNowInWhatFolder)//当前是根目录
										{
											returnDVDFileFolderInfo(pForyouDVDInfo,FALSE
												,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[isWhere].name
												,pForyouDVDInfo->sForyouDVDInfo.pFileTreeList[isWhere].nameLength
												,pForyouDVDInfo->sForyouDVDInfo.pNeedReturnStart + pForyouDVDInfo->sForyouDVDInfo.pNowReturnCount
												,isWhere);
										}
										else
										{
											returnDVDFileFolderInfo(pForyouDVDInfo,FALSE
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

		if (ETIMEDOUT == WaitReturn)//如果超时退出
		{
			pForyouDVDInfo->sForyouDVDInfo.DVDReqStep = FlyaudioDVDReqStepChangeAndReq(pForyouDVDInfo,pForyouDVDInfo->sForyouDVDInfo.DVDReqStep);
		}
	}
}


