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
#include <sys/mman.h>

#include <unistd.h>
#include <cutils/properties.h>  

#include "saveFile.h"

//定义全局变量
struct save_file_struct *saveFileInfo = NULL;

static int open_save_file(const char *path, int flag)
{
	int fd = -1;

	if (fd < 0){
		fd = open(path, flag, 0777);
		if (fd < 0){

			return - 1;
		}
	}
	return fd;
}

static int close_save_file(int fd)
{
	if (fd > 0){
		if(close(fd) < 0){
			return -1;
		}
	}
	
	return 0;
}
static int write_to_save_file(int fd, BYTE *buf, unsigned long len)
{
	int ret=-1;
	if (fd > 0){
		ret = write(fd, buf, len);
		if (ret > 0){
			return ret; 
		}
	}
	
	return -1;
}

unsigned long get_save_file_size(const char *path)
{
	struct stat file_stat;
	if (!stat(path, &file_stat))
		return file_stat.st_size;
	else 
		return 0;
}

static int is_already_exist(const char *path)
{
	if (access(path, F_OK) == -1)
		return -1;
	else
		return 0;
}

static unsigned long read_from_save_file(int fd, BYTE *buf, unsigned long len)
{
	long long ret=-1;
	if (fd > 0){
		ret = read(fd, buf, len);
		if (ret > 0){
			return ret; 
		}
	}
	
	return 0;
}


void writeSaveFileData(const char *path, BYTE *buf, UINT32 size)
{
	int fd = -1;
	int flag = 0;
	
	if (!strcmp("/data/fly/.savefile", path))
		flag = O_TRUNC|O_CREAT|O_WRONLY;
	else
		flag = O_WRONLY;
		
	fd = open_save_file(path,flag);
	if (fd > 0){
		write_to_save_file(fd,(BYTE*)buf, size);
		close_save_file(fd);
	}
}

UINT32 readSaveFileData(const char *path,BYTE *buf, UINT32 size)
{
	int fd = -1;
	UINT32 len = 0;
	UINT32 i;	
	if (!is_already_exist(path)){
	
		fd = open_save_file(path,O_RDONLY);
		if (fd > 0){
			len = read_from_save_file(fd, buf, size);
			close_save_file(fd);
		}
	}
	for(i=0;i<len;i++)
	{
		DBG0(debugOneData(" ",buf[i]);)
	}

	return len;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
static void debugParaWriteToConfig(void)
{
	if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgGlobalLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgGlobalLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgKeyLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgKeyLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgAudioLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgAudioLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgBtLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgBtLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgDVDLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgDVDLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgACLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgACLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgVideoLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgVideoLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgSystemLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgSystemLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgRadioLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgRadioLevel = saveFileInfo->debugParaValue;
	}
//	else if (0 == memcmp(saveFileInfo->debugParaName,"FlyCarbodyMsgLevel",saveFileInfo->debugiParaLength))
//	{
//		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgLevel = saveFileInfo->debugParaValue;
//	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgXMRadioLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgXMRadioLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgServiceHalLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgServiceHalLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgExdisplayLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgExdisplayLevel = saveFileInfo->debugParaValue;
	}
//	else if (0 == memcmp(saveFileInfo->debugParaName,"FlyExBoxMsgLevel",saveFileInfo->debugiParaLength))
//	{
//		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgLevel = saveFileInfo->debugParaValue;
//	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgTpmsLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTpmsLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"debugMsgTVLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->debugMsgTVLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"FlyAssistDisplayMsgLevel",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->FlyAssistDisplayMsgLevel = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"FlyIngoreMsgResponse",saveFileInfo->debugiParaLength))
	{
		if (saveFileInfo->debugParaValue)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->FlyIngoreMsgResponse = TRUE;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->FlyIngoreMsgResponse = FALSE;
		}
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"bOSDDemoMode",saveFileInfo->debugiParaLength))
	{
		if (saveFileInfo->debugParaValue)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode = TRUE;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bOSDDemoMode = FALSE;
		}
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"bIngoreIPCEventMsgResponse",saveFileInfo->debugiParaLength))
	{
		if (saveFileInfo->debugParaValue)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bIngoreIPCEventMsgResponse = TRUE;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bIngoreIPCEventMsgResponse = FALSE;
		}
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"bACCOffLightOn",saveFileInfo->debugiParaLength))
	{
		if (saveFileInfo->debugParaValue)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bACCOffLightOn = TRUE;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bACCOffLightOn = FALSE;
		}
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"IICSpeadOnAudio",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->IICSpeadOnAudio = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"bSystemResetUseExtConfig",saveFileInfo->debugiParaLength))
	{
		if (saveFileInfo->debugParaValue)
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bSystemResetUseExtConfig = TRUE;
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->bSystemResetUseExtConfig = FALSE;
		}
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"iSystemResetAtLeastDays",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetAtLeastDays = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"iSystemResetOnHour",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetOnHour = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"iSystemResetOnMinute",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetOnMinute = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"iSystemResetInnerMin",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetInnerMin = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"iSystemResetPowerOffMin",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetPowerOffMin = saveFileInfo->debugParaValue;
	}
	else if (0 == memcmp((const void *)saveFileInfo->debugParaName,"iSystemResetCanRunLess",saveFileInfo->debugiParaLength))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iSystemResetCanRunLess = saveFileInfo->debugParaValue;
	}
	else
	{
		//DBG0(debugString("\n None Compare!");)
	}
}

static void debugParaProcessor(BYTE iByte)
{
	if ('\r' == iByte || '\n' == iByte)
	{
		saveFileInfo->debugParaName[saveFileInfo->debugiParaLength] = 0x00;
		if (FALSE == saveFileInfo->debugbReadParaName)
		{
			debugParaWriteToConfig();
		}
		saveFileInfo->debugbReadParaName = TRUE;
		saveFileInfo->debugiParaLength = 0;		
		saveFileInfo->debugParaValue = 0;
		saveFileInfo->debugReadParaEnd = FALSE;
	}
	else if ('/' == iByte)
	{
		saveFileInfo->debugReadParaEnd = TRUE;
	}
	else
	{
		if (!saveFileInfo->debugReadParaEnd)
		{
			if ('=' == iByte)
			{
				saveFileInfo->debugbReadParaName = FALSE;
			}
			else if (FALSE == saveFileInfo->debugbReadParaName)
			{
				if (iByte >= '0' && iByte <= '9')
				{
					saveFileInfo->debugParaValue *= 10;
					saveFileInfo->debugParaValue += iByte - '0';
				}
			}
			else if ((iByte >= 'a' && iByte <= 'z')
				|| (iByte >= '0' && iByte <= '9')
				|| (iByte >= 'A' && iByte <= 'Z')
				|| (iByte == '_')
				)
			{
				if (saveFileInfo->debugiParaLength < DEBUG_BUFF_LINE_LENGTH - 1)
				{
					saveFileInfo->debugParaName[saveFileInfo->debugiParaLength++] = iByte;
				}
			}
		}
	}
}

BOOL debugParaReadFromFile(void)//0ResidentFlash	1SDMEM
{
	int fd = -1;
	UINT i;
	UINT32 iLength = 0;
	BYTE *pBuf =NULL;
	UINT32 read_table_len = 0;
	
	saveFileInfo = (struct save_file_struct *)malloc(sizeof(struct save_file_struct) + sizeof(BYTE)*256);
	if(saveFileInfo == NULL)
	{
		DBG0(debugString("\n read FlyDriversDebug.txt malloc FALSE!");)
		return FALSE;
	}
	
/* 	fd = open("/mnt/sdcard/tflash/FlyDriversDebug.txt",O_RDONLY ,0777);
	if (fd < 0)
	{
		DBG0(debugString("\n /mnt/sdcard/tflash/FlyDriversDebug.txt is not exit!");)
		return FALSE;
	} */

	saveFileInfo->debugbReadParaName = TRUE;
	saveFileInfo->debugiParaLength = 0;
	saveFileInfo->debugParaValue = 0;

	iLength = get_save_file_size("/mnt/sdcard/tflash/FlyDriversDebug.txt");
	pBuf = (BYTE *)malloc(iLength * sizeof(BYTE));
	if(pBuf == NULL)
	{
		DBG0(debugString("\n read FlyDriversDebug.txt malloc FALSE!");)
		return FALSE;
	}
	read_table_len = readSaveFileData("/mnt/sdcard/tflash/FlyDriversDebug.txt",pBuf,iLength);
//	DBG0(debugOneData(" " , read_table_len);)
	if(read_table_len > 0)
	{
		for (i = 0;i < iLength;i++)
		{
			debugParaProcessor(pBuf[i]);
		}
		pFlyAllInOneInfo->pMemory_Share_Common->bSuccessReadDebugFileFromSDMEM = TRUE;
		DBG0(debugString("\n read FlyDriversDebug.txt success");)
		free(pBuf);
		pBuf =NULL;
		free(saveFileInfo);
		saveFileInfo = NULL;
		return TRUE;
	}
	else
	{
		DBG0(debugString("\n read FlyDriversDebug.txt false");)
	}
//	close(fd);
	free(pBuf);
	pBuf =NULL;
	
	free(saveFileInfo);
	saveFileInfo = NULL;
	
	return FALSE;
} 

////////方向盘数据///////////////////////////////////////////////////////////////////////////////////////
UINT32 forU8ToU32(BYTE *p)
{
	UINT32 iTemp = 0;
	iTemp = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
	return iTemp;
}

/*
8	方向盘头（WHEEL   ）

4	结构体总大小（包括头、尾、数据）
4	方向盘数量
4	方向盘名称长度

N	方向盘名称（Unicode编码、逗号隔开 2C 00）

4	方向盘ID
4	方向盘数据偏移量（从头开始）
4	方向盘数据大小
。。。重复上面三行

N	方向盘数据

8	方向盘尾（WHEEL   ）
*/
//bDefault读取缺省方向盘数据？
//iSteelwheelIndex	0为缺省	1~Count为选择

static void readSteelwheelData(BYTE *p,UINT length,UINT iSteelwheelIndex,BOOL bDefault)
{
	UINT iStructSize=0;
	UINT iRemoteDataCount=0;
	UINT iRemoteNameLength=0;
	UINT iNowRemoteNameIndex=0;
	UINT iRemoteDataOneOffset=0;
	UINT iRemoteDataOneSize=0;
	UINT i=0;
	if ('W' == p[0] && 'H' == p[1] && 'E' == p[2] && 'E' == p[3]
	 && 'L' == p[4] && ' ' == p[5] && ' ' == p[6] && ' ' == p[7]
			&& 'W' == p[length-8] && 'H' == p[length-7] && 'E' == p[length-6] && 'E' == p[length-5]
			&& 'L' == p[length-4] && ' ' == p[length-3] && ' ' == p[length-2] && ' ' == p[length-1])
	{
		iStructSize = forU8ToU32(&p[8]);
		if (iStructSize == length)
		{
			iRemoteDataCount = forU8ToU32(&p[8+4]);
			if (!bDefault)//非缺省
			{
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataCount = iRemoteDataCount;
				DBG0(debugOneData(" iRemoteDataCount " , iRemoteDataCount);)
				DBG0(debugOneData(" iSteelwheelIndex " , iSteelwheelIndex);)
			}
			else
			{			
				return;
			}
			if (iSteelwheelIndex < iRemoteDataCount)
			{
				iRemoteNameLength = forU8ToU32(&p[8+4+4]);
				iNowRemoteNameIndex = 0;
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength = 0;//清0
				for (i = 0;i < iRemoteNameLength;i+=2)
				{
					if (0x2C == p[8+4+4+4+i] && 0x00 == p[8+4+4+4+i+1])
					{
						iNowRemoteNameIndex++;
					}
					if (iNowRemoteNameIndex < iSteelwheelIndex)//小于则等待
					{
					}
					else if (iNowRemoteNameIndex == iSteelwheelIndex)//等于则读取
					{
						pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName[pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength++] = p[8+4+4+4+i];
						pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName[pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength++] = p[8+4+4+4+i+1];
					}
					else//大于则跳出
					{
						break;
					}
				}
				DBG0(debugString("\n stellwheel name ///: ");)
				for(i=0;i<pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength;i++)
				{
					DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName[i]);)
				}

				iRemoteDataOneOffset = forU8ToU32(&p[8+4+4+4+iRemoteNameLength+(12*iSteelwheelIndex)+4]);				
				iRemoteDataOneSize = forU8ToU32(&p[8+4+4+4+iRemoteNameLength+(12*iSteelwheelIndex)+4+4]);
				memcpy(&pFlyAllInOneInfo->pMemory_Share_Common->sRemoteData
					,&p[iRemoteDataOneOffset]
					,iRemoteDataOneSize);
				pFlyAllInOneInfo->pMemory_Share_Common->bRemoteDataHave = TRUE;
				DBG0(debugString("\n stellwheel data  ///: ");)
				for (i = 0;i < iRemoteDataOneSize;i++)
				{
					DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->sRemoteData[i]);)
				}
			}
			else
			{
				iSteelwheelIndex = 0;
				iRemoteNameLength = forU8ToU32(&p[8+4+4]);
				iNowRemoteNameIndex = 0;
				pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength = 0;//清0
				for (i = 0;i < iRemoteNameLength;i+=2)
				{
					if (0x2C == p[8+4+4+4+i] && 0x00 == p[8+4+4+4+i+1])
					{
						iNowRemoteNameIndex++;
					}
					if (iNowRemoteNameIndex < iSteelwheelIndex)//小于则等待
					{
					}
					else if (iNowRemoteNameIndex == iSteelwheelIndex)//等于则读取
					{
						pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName[pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength++] = p[8+4+4+4+i];
						pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName[pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength++] = p[8+4+4+4+i+1];
					}
					else//大于则跳出
					{
						break;
					}
				}
				DBG0(debugString("\n stellwheel name ///: ");)
				for(i=0;i<pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength;i++)
				{
					DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->sRemoteDataName[i]);)
				}

				iRemoteDataOneOffset = forU8ToU32(&p[8+4+4+4+iRemoteNameLength+(12*iSteelwheelIndex)+4]);				
				iRemoteDataOneSize = forU8ToU32(&p[8+4+4+4+iRemoteNameLength+(12*iSteelwheelIndex)+4+4]);
				memcpy(&pFlyAllInOneInfo->pMemory_Share_Common->sRemoteData
					,&p[iRemoteDataOneOffset]
					,iRemoteDataOneSize);
				pFlyAllInOneInfo->pMemory_Share_Common->bRemoteDataHave = TRUE;
				DBG0(debugString("\n stellwheel data  ///: ");)
				for (i = 0;i < iRemoteDataOneSize;i++)
				{
					DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->sRemoteData[i]);)
				}
			}
		}
		else
		{
			pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength = 0;
			pFlyAllInOneInfo->pMemory_Share_Common->bRemoteDataHave = FALSE;
			DBG0(debugString("\n stellwheel data size error  ///: ");)
		}
	}
	else
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iRemoteDataNameLength = 0;
		pFlyAllInOneInfo->pMemory_Share_Common->bRemoteDataHave = FALSE;
		DBG0(debugString("\n stellwheel data header error  ///: ");)
	}
}
///////////////面板数据/////////////////////
static void read_Panel_Data(BYTE *p,UINT length)
{
	UINT i;
	pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.valid[0] = 0xAA;
	pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.valid[1] = 0x55;
	pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.type = p[0];
	DBG0(debugString("\n panel data name  ///: ");)
	for (i = 0;i < 8;i++)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[i] = p[1+i];
		DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[i]);)
	}
	pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.slid = p[9];
	for (i = 0;i < 4;i++)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.EK_Value[i] = p[10+i];
	}
	DBG0(debugString("\n panel data data  ///: ");)
	for (i = 0;i < 4;i++)
	{
		DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.EK_Value[i]);)	
	}
	for (i = 0;i < 30;i++)
	{
		pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.KeyValue[i] = p[14+i];
	}
	DBG0(debugString("\n panel data data  AD ///: ");)
	for (i = 0;i < 8;i++)
	{
		DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.KeyValue[i]);)
	}
	for (i = 0;i < 8;i++)
	{
		DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.KeyValue[i + 8]);)
	}
	for (i = 0;i < 8;i++)
	{
		DBG0(debugOneData("  " , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.KeyValue[i + 16]);)
	}
	pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.LCD_RGB_Wide = p[44] + (p[45] << 8);
	pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.LCD_RGB_High = p[46] + (p[47] << 8);
	DBG0(debugOneData(" \n LCD_RGB_Wide ///:" , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.LCD_RGB_Wide);)
	DBG0(debugOneData(" \n LCD_RGB_High///: " , pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.LCD_RGB_High);)
	 
}

static void readMCUSoftVersion(BYTE *p,UINT length)
{
	BYTE iMCUSoftVersion[MCU_SOFT_VERSION_MAX];
	UINT iMCUSoftVersionLength;

	if ('V' == p[0] && 'E' == p[1] && 'R' == p[2] && 'S' == p[3]
	&& 'I' == p[4] && 'O' == p[5] && 'N' == p[6] && ' ' == p[7]
	&& 'V' == p[length-8] && 'E' == p[length-7] && 'R' == p[length-6] && 'S' == p[length-5]
	&& 'I' == p[length-4] && 'O' == p[length-3] && 'N' == p[length-2] && ' ' == p[length-1])
	{
		DBG0(debugString("\n read MCU version start  ///: ");)
		iMCUSoftVersionLength = forU8ToU32(&p[8]) - 8 - 4 - 8;
		if (iMCUSoftVersionLength < MCU_SOFT_VERSION_MAX)
		{
			DBG0(debugString("\n read MCU version OK  ///: ");)
			memcpy(&pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersion,&p[8+4],iMCUSoftVersionLength);
			pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersionLength = iMCUSoftVersionLength;

			iMCUSoftVersionLength = property_get("fly.CarModule.Version",iMCUSoftVersion,"0");
			if (pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersionLength != iMCUSoftVersionLength
				|| memcmp(&pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersion,iMCUSoftVersion,pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersionLength))
			{
				property_set("fly.CarModule.Version", pFlyAllInOneInfo->pMemory_Share_Common->iMCUSoftVersion);
				DBG0(debugString("\n read MCU version To Property  ///: ");)
			}
		}
	}
	else
	{
		DBG0(debugString("\n read MCU version error  ///: ");)
	}
}

//获取NANDFLASH数据 面板
 BOOL dealPanelData(void) 
{
	UINT i=0;
	UINT32 maxlen=0 ;
	UINT32 read_table_len=0 ;
	maxlen = get_save_file_size("/flydata/panelwheelData");
	
	UINT iTotalTypeCount=0;
	UINT iTypeID=0,iTypeStart=0,iTypeCount=0;
	
	DBG0(debugOneData(" " , maxlen);)
	
	//BYTE p[maxlen + 8]; 
	BYTE *p = NULL;
	p = (BYTE *)malloc(maxlen * (sizeof(BYTE)+8));
	if(p == NULL)
	{
		DBG0(debugString("\n /flydata/panelwheelData malloc FALSE!");)
		return FALSE;
	}
	read_table_len = readSaveFileData("/flydata/panelwheelData",p,maxlen);
	if(read_table_len)
	{
		
		DBG0(debugOneData(" " , read_table_len);)
		
		////处理开始
		iTotalTypeCount = forU8ToU32(&p[24]);
		DBG0(debugString("\n deal nandflash data begin   ///: ");)
		if (3 == iTotalTypeCount)
		{
			for (i = 0;i < 3;i++)
			{
				iTypeID = forU8ToU32(&p[24+4+4*3*i]);
				iTypeStart = forU8ToU32(&p[24+4+4*3*i+4]);
				iTypeCount = forU8ToU32(&p[24+4+4*3*i+8]);
				if (1 == iTypeID)
				{
					readMCUSoftVersion(&p[iTypeStart + 24],iTypeCount);
				}
				else if (2 == iTypeID)
				{
					read_Panel_Data(&p[iTypeStart + 24],iTypeCount);
				}
				
			}
			DBG0(debugString("\n read panelData success!!");)
		}
		free(p);
		p = NULL;
		return TRUE;
	}
	DBG0(debugString("\n read panelData false!!");)
	free(p);
	p = NULL;
	return FALSE;
}
 
//获取NANDFLASH数据 方向盘
 BOOL dealSteelWheelData(UINT iSteelwheelIndex) //方向盘使用第几组数据
{
	UINT i=0;
	UINT32 maxlen=0 ;
	UINT32 read_table_len=0 ;
	maxlen = get_save_file_size("/flydata/panelwheelData");
	
	UINT iTotalTypeCount=0;
	UINT iTypeID=0,iTypeStart=0,iTypeCount=0;
	
	DBG0(debugOneData(" " , maxlen);)
	
	//BYTE p[maxlen + 8]; 
	BYTE *p = NULL;
	p = (BYTE *)malloc(maxlen * (sizeof(BYTE)+8));
	if(p == NULL)
	{
		DBG0(debugString("\n /flydata/panelwheelData malloc FALSE!");)
		return FALSE;
	}
	read_table_len = readSaveFileData("/flydata/panelwheelData",p,maxlen);
	if(read_table_len)
	{
		
		DBG0(debugOneData(" " , read_table_len);)
		
		////处理开始
		iTotalTypeCount = forU8ToU32(&p[24]);
		DBG0(debugString("\n deal nandflash data begin   ///: ");)
		if (3 == iTotalTypeCount)
		{
			for (i = 0;i < 3;i++)
			{
				iTypeID = forU8ToU32(&p[24+4+4*3*i]);
				iTypeStart = forU8ToU32(&p[24+4+4*3*i+4]);
				iTypeCount = forU8ToU32(&p[24+4+4*3*i+8]);
				if (1 == iTypeID)
				{
					//readMCUSoftVersion(&p[iTypeStart + 24],iTypeCount);
				}
				else if (3 == iTypeID)
				{
					readSteelwheelData(&p[iTypeStart + 24],iTypeCount,iSteelwheelIndex,FALSE);
				}
			}
			DBG0(debugString("\n read steelwhellData success!!");)
		}
		free(p);
		p = NULL;
		return TRUE;
	}
	free(p);
	p = NULL;
	DBG0(debugString("\n read steelwhellData false!!");)
	return FALSE;
}  

///////////////////////////////////在这里添加文件数据保存的参数种类//////////////////////////////////////////
static void sCarDataWriteToConfig(void)
{
	int  i;
	if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"sCarModel",saveFileInfo->sCarDataLen))
	{
		memcpy((void *)pFlyAllInOneInfo->pMemory_Share_Common->sCarModule,(void *)saveFileInfo->sCarModuleStr,saveFileInfo->sCarModuleStr_Len);
		DBG0(debugString("\n sCarModel :");)
		for(i=0;i<saveFileInfo->sCarModuleStr_Len;i++)
		{
		DBG0(debugOneData("" , pFlyAllInOneInfo->pMemory_Share_Common->sCarModule[i]);)
		}
	}
	else if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"iVolumeMax",saveFileInfo->sCarDataLen))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iVolumeMax = saveFileInfo->sCarDataValue;
		DBG0(debugString("\n iVolumeMax!!! ");)
		DBG0(debugOneData(" " , pFlyAllInOneInfo->pMemory_Share_Common->iVolumeMax);)
	}
	else if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"bControlRadioANT",saveFileInfo->sCarDataLen))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->bControlRadioANT = saveFileInfo->sCarDataValue;
		DBG0(debugString("\n bControlRadioANT!!! ");)
		DBG0(debugOneData(" " , pFlyAllInOneInfo->pMemory_Share_Common->bControlRadioANT);)
	}
/////////////////后面的功能还没有实现///////////////////////
	else if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"iVolumeMin",saveFileInfo->sCarDataLen))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iVolumeMin = saveFileInfo->sCarDataValue;
	//	DBG0(debugString("\n iVolumeMin!!! ");)
	//	DBG0(debugOneData(" " , read_table_len);)
	}
	else if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"iVolume",saveFileInfo->sCarDataLen))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->iVolume = saveFileInfo->sCarDataValue;
	//	DBG0(debugString("\n iVolume!!! ");)
	}
	else if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"bMute",saveFileInfo->sCarDataLen))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->bMute = saveFileInfo->sCarDataValue;
	//	DBG0(debugString("\n bMute!!! ");)
	}
	else if (0 == memcmp((const void *)saveFileInfo->sCarModuleName,"bNeedReturnNewVolume",saveFileInfo->sCarDataLen))
	{
		pFlyAllInOneInfo->pMemory_Share_Common->bNeedReturnNewVolume = saveFileInfo->sCarDataValue;
	//	DBG0(debugString("\n bNeedReturnNewVolume!!! ");)
	}
	else
	{
	//	DBG0(debugString("\n CarData None Compare!!! ");)
	}
//	DBG0(debugString("\n CarData Compare OK !!! ");)
}

static void sCarMessageProcess(BYTE iByte)
{
	if('\r' == iByte || '\n' == iByte)
	{
		saveFileInfo->sCarModuleName[saveFileInfo->sCarDataLen] = 0x00;
		if(FALSE == saveFileInfo->sCarReadModuleName)
		{
			sCarDataWriteToConfig();
		}
		saveFileInfo->sCarReadModuleName = TRUE;
		saveFileInfo->sCarDataLen = 0;
		saveFileInfo->sCarDataValue = 0;
		saveFileInfo->sCarDataEnd = FALSE;
		saveFileInfo->sCarModuleStr_Len = 0;
	}
	else if('/' == iByte)
	{
		saveFileInfo->sCarDataEnd = TRUE;
	}
	else 
	{
		if(!saveFileInfo->sCarDataEnd)
		{
			if('=' == iByte)
			{
				saveFileInfo->sCarReadModuleName = FALSE;
			}
			else if(FALSE == saveFileInfo->sCarReadModuleName)
			{
				if(iByte >= '0' && iByte <= '9')
				{
					saveFileInfo->sCarDataValue *= 10;
					saveFileInfo->sCarDataValue += iByte - '0';
				}
				else if((iByte >= 'a' && iByte <= 'z') 
						|| (iByte >= 'A' && iByte <= 'Z')
						|| (iByte == '_'))
				{
					if(saveFileInfo->sCarModuleStr_Len < DEBUG_BUFF_LINE_LENGTH - 1)
					{
						saveFileInfo->sCarModuleStr[saveFileInfo->sCarModuleStr_Len++] = iByte;
					}
				}
			}
			else if((iByte >= 'a' && iByte <= 'z') 
					|| (iByte >= '0' && iByte <= '9')
					|| (iByte >= 'A' && iByte <= 'Z')
					|| (iByte == '_'))
			{
				if(saveFileInfo->sCarDataLen < DEBUG_BUFF_LINE_LENGTH - 1)
				{
					saveFileInfo->sCarModuleName[saveFileInfo->sCarDataLen++] = iByte;
				}
			}
		}
	}
}

BOOL dealsCarMessageFromFile(void)
{
	int fd = -1;
	UINT i;
	UINT32 iLength = 0;
	BYTE *pBuf = NULL;
	UINT32 read_table_len = 0;
	
	//struct save_file_struct *saveFileInfo = NULL;
	saveFileInfo = (struct save_file_struct *)malloc(sizeof(struct save_file_struct) + sizeof(BYTE)*256);
	if(saveFileInfo == NULL)
	{
		DBG0(debugString("\n flydata/flyhalconfig malloc FALSE !");)
		return FALSE;
	}
	
////////////////////传递到处理函数的参数/////////////	
	//saveFileInfo->sCarModuleName[SCAR_MODEL_MAX];
	saveFileInfo->sCarReadModuleName = TRUE;
	saveFileInfo->sCarDataLen = 0;
	saveFileInfo->sCarDataValue = 0;
	saveFileInfo->sCarDataEnd = FALSE;
	saveFileInfo->sCarModuleStr_Len = 0;
	
	/* fd = open("/flydata/flyhalconfig",O_RDONLY ,0777);
	if(fd < 0)
	{
		DBG0(debugString("\n flydata/flyhalconfig  is not exit!");)
		return FALSE;
	} */
	
	iLength = get_save_file_size("/flydata/flyhalconfig");
	pBuf = (BYTE *)malloc(iLength * sizeof(BYTE));
	if(pBuf == NULL)
	{
		DBG0(debugString("\n flydata/flyhalconfig malloc FALSE!");)
		return FALSE;
	}
	read_table_len = readSaveFileData("/flydata/flyhalconfig",pBuf,iLength);
	if(read_table_len > 0)
	{
		DBG0(debugString("\n CarData Compare!!! ");)
		for (i = 0;i < iLength;i++)
		{
			sCarMessageProcess(pBuf[i]);
		}
		if(saveFileInfo->sCarDataLen > 0)
		{
			sCarDataWriteToConfig();
		}
		DBG0(debugString("\n read flydata/flyhalconfig success");)
		free(saveFileInfo);
		saveFileInfo = NULL;
		free(pBuf);
		pBuf =NULL;
		return TRUE;
	}
	else
	{
		DBG0(debugString("\n read flydata/flyhalconfig false");)
	}
//	close(fd);
	free(pBuf);
	pBuf =NULL;
	
	free(saveFileInfo);
	saveFileInfo = NULL;
	
	return FALSE;
}