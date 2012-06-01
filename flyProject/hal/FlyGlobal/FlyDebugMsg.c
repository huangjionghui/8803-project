#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <semaphore.h>
#include <cutils/log.h> 
#include <asm/termbits.h>
#include <cutils/atomic.h>
#include <hardware/hardware.h>  

#include "FlyGlobal.h"
#include "FlyDebugMsg.h"
#include "FlyGlobal.h"
#include "../../include/allInOneOthers.h"
#include "../../include/commonFunc.h"
extern struct flyAllInOne *pFlyAllInOneInfo;


//#ifdef 	LDH
/*********************************************************************************
**函数名称:		DemoStrStart	
**函数功能:
**入口参数:		iRow: 行数
**返回参数:
**********************************************************************************/
void demoStrStart(P_FLY_GLOBAL_INFO pGlobalInfo,UINT iRow)
{
	if (iRow >= OSD_DEBUG_LINES)
	{
		iRow = 0;
	}
	pGlobalInfo->iOSDDemoStrRow = iRow;
	pGlobalInfo->iOSDDemoStrLength = 0;
}
//#endif


//#ifdef 	LDH
/*********************************************************************************
**函数名称:		DemoStrAdd
**函数功能:
**入口参数:
**返回参数:
*********************************************************************************/
void demoStrAdd(P_FLY_GLOBAL_INFO pGlobalInfo,BYTE iAscii)
{
	if (pGlobalInfo->iOSDDemoStrLength < OSD_DEBUG_WIDTH)
	{
		pGlobalInfo->sDemoStr[pGlobalInfo->iOSDDemoStrRow][pGlobalInfo->iOSDDemoStrLength] = iAscii;
		pGlobalInfo->iOSDDemoStrLength++;
		pGlobalInfo->iDemoStrLength[pGlobalInfo->iOSDDemoStrRow] = pGlobalInfo->iOSDDemoStrLength;
	}
}
//#endif


//#ifdef 	LDH
/*********************************************************************************
**函数名称:		demoOSDHostTemperature	
**函数功能:		主机温度
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDHostTemperature(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	INT iTemp = pFlyAllInOneInfo->pMemory_Share_Common->iHostTemperature;//温度全局变量
	//DBG0(debugOneData("\n2012-3-2 19:19  iHostTemperature:",iTemp);)
	/****************************************************/
	demoStrStart(pGlobalInfo,OSD_DEBUG_HOST_TEMPERATURE);
	/***************************/
	demoStrAdd(pGlobalInfo,'H');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'s');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'T');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'m');
	demoStrAdd(pGlobalInfo,'p');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,':');
	/***************************/
	if (iTemp < 0)
	{
		demoStrAdd(pGlobalInfo,'-');
		iTemp = 0 - iTemp;
	}

	iTemp %= 1000;

	/*****************************************/
	demoStrAdd(pGlobalInfo,iTemp/100 + '0');
	demoStrAdd(pGlobalInfo,iTemp%100/10 + '0');
	demoStrAdd(pGlobalInfo,iTemp%10 + '0');
	/*****************************************/
	//demoStrAdd(pGlobalInfo,';');
}
//#endif

//#ifdef 	LDH
/*********************************************************************************
**函数名称:		demoOSDDVDSoftVersion
**函数功能:		DVD版本
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDDVDSoftVersion(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	UINT i;
	/***********************************************/
	demoStrStart(pGlobalInfo,OSD_DEBUG_SOFTVERSION);//Start
	/***********************************************/
	demoStrAdd(pGlobalInfo,'V');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'s');
	demoStrAdd(pGlobalInfo,'i');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'n');
	/***********************************************************************************/
	//DBG0(debugOneData("\n2012-3-2 19:19  iDVDSoftwareVersionLength:",pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersionLength);)
	if (pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersionLength)//DVD软件版本
	{
		demoStrAdd(pGlobalInfo,' ');
		demoStrAdd(pGlobalInfo,'D');
		demoStrAdd(pGlobalInfo,'V');
		demoStrAdd(pGlobalInfo,'D');
		demoStrAdd(pGlobalInfo,':');
		for (i = 0;i < pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersionLength;i++)
		{
			demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iDVDSoftwareVersion[i]);
		}
	}
	/**********************************************************************************************************/
	//DBG0(debugOneData("\n2012-3-2 19:19  iBTSoftwareVersionLength:",pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersionLength);)
	if (pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersionLength)//BT软件版本
	{
		demoStrAdd(pGlobalInfo,' ');
		demoStrAdd(pGlobalInfo,'B');
		demoStrAdd(pGlobalInfo,'T');
		demoStrAdd(pGlobalInfo,':');
		for (i = 0;i < pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersionLength;i++)
		{
			demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iBTSoftwareVersion[i]);
		}
	}
	//demoStrAdd(pGlobalInfo,';');
}

//#endif


/*********************************************************************************
**函数名称:		demoOSDDriversCompileTime	
**函数功能:		驱动编译时间
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDDriversCompileTime(P_FLY_GLOBAL_INFO pGlobalInfo)
{

	//#ifdef 	LDH
	UINT i;
	/*****************************************************/
	demoStrStart(pGlobalInfo,OSD_DEBUG_COMPILE_TIME_ERROR);
	/*****************************************************/
	demoStrAdd(pGlobalInfo,'D');
	demoStrAdd(pGlobalInfo,'r');
	/*****************************************************/
	if (pFlyAllInOneInfo->pMemory_Share_Common->bCheckShellBabyError)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'i');
	}
	/*****************************************************/
	demoStrAdd(pGlobalInfo,'v');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'s');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,' ');
	/*****************************************************************************************************/
	for(i = 0;i < strlen(__TIME__);i++)
	{
		demoStrAdd(pGlobalInfo,__TIME__[i]);//pGlobalInfo->iDriverCompTIME[i]
	}
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	for(i = 0;i < strlen(__DATE__);i++)
	{
		demoStrAdd(pGlobalInfo,__DATE__[i]);//pGlobalInfo->iDriverCompDATE
	}
	/***************************************************************************************************/
	//准备进入待机，断掉与LPC的IIC通信
	if (pFlyAllInOneInfo->pMemory_Share_Common->bNoMoreToSendDataWhenToSleep)
	{
		demoStrAdd(pGlobalInfo,'T');
		demoStrAdd(pGlobalInfo,'o');
		demoStrAdd(pGlobalInfo,' ');
		demoStrAdd(pGlobalInfo,'S');
		demoStrAdd(pGlobalInfo,'l');
		demoStrAdd(pGlobalInfo,'e');
		demoStrAdd(pGlobalInfo,'e');
		demoStrAdd(pGlobalInfo,'p');
		demoStrAdd(pGlobalInfo,' ');
	}
	//demoStrAdd(pGlobalInfo,';');
	//#endif
}


//#ifdef 	LDH
/*********************************************************************************
**函数名称:		demoOSDKeyADDisplay	
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDKeyADDisplay(P_FLY_GLOBAL_INFO pGlobalInfo)
{

	demoStrStart(pGlobalInfo,OSD_DEBUG_KEYAD_LIST);	
	demoStrAdd(pGlobalInfo,'A');
	demoStrAdd(pGlobalInfo,'D');
	demoStrAdd(pGlobalInfo,' ');

	demoStrAdd(pGlobalInfo,'1');
	demoStrAdd(pGlobalInfo,':');

	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[0] /100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[0] %100 /10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[0] %10 + '0');
	demoStrAdd(pGlobalInfo,' ');

	demoStrAdd(pGlobalInfo,'2');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[1] /100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[1] %100 /10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[1] %10 + '0');
	demoStrAdd(pGlobalInfo,' ');

	demoStrAdd(pGlobalInfo,'3');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[2] /100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[2] %100 /10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iPanelKeyAD[2] %10 + '0');
	demoStrAdd(pGlobalInfo,' ');

	demoStrAdd(pGlobalInfo,'4');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[0] /1000 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[0] %1000 /100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[0] %100 /10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[0] %10 + '0');
	demoStrAdd(pGlobalInfo,' ');

	demoStrAdd(pGlobalInfo,'5');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[1] /1000 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[1] %1000 /100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[1] %100 /10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iSteelAD[1] %10 + '0');

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'L');
	demoStrAdd(pGlobalInfo,'i');
	demoStrAdd(pGlobalInfo,'s');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyIndex/100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyIndex%100/10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyIndex%10 + '0');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'V');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'l');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyValue/100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyValue%100/10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyValue%10 + '0');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyCount/1000 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyCount%1000/100 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyCount%100/10 + '0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->iKeyCount%10 + '0');
	//demoStrAdd(pGlobalInfo,';');
}

//#endif

//#ifdef 	LDH
/*********************************************************************************
**函数名称:		demoOSDBreakAndPhoneStatus
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDBreakAndPhoneStatus(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	demoStrStart(pGlobalInfo,OSD_DEBUG_BREAK_AND_PHONE_STATUS);

	demoStrAdd(pGlobalInfo,'F');
	demoStrAdd(pGlobalInfo,'A');
	demoStrAdd(pGlobalInfo,'M');
	demoStrAdd(pGlobalInfo,'P');
	demoStrAdd(pGlobalInfo,':');
	#ifdef LDH
	if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveFlyAudioExtAMP)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}
	#endif
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'T');
	demoStrAdd(pGlobalInfo,'V');
	demoStrAdd(pGlobalInfo,':');
	if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveTVModule)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'T');
	demoStrAdd(pGlobalInfo,'P');
	demoStrAdd(pGlobalInfo,'M');
	demoStrAdd(pGlobalInfo,'S');
	demoStrAdd(pGlobalInfo,':');
	if (pFlyAllInOneInfo->pMemory_Share_Common->bHaveTPMSModule)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'B');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'k');

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'B');
	demoStrAdd(pGlobalInfo,'U');
	demoStrAdd(pGlobalInfo,'S');
	demoStrAdd(pGlobalInfo,':');
	if (pFlyAllInOneInfo->pMemory_Share_Common->bBreakStatusBUS)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'I');
	demoStrAdd(pGlobalInfo,'O');
	demoStrAdd(pGlobalInfo,':');
	if (pFlyAllInOneInfo->pMemory_Share_Common->bBreakStatusIO)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'P');
	demoStrAdd(pGlobalInfo,'h');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,'e');

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'B');
	demoStrAdd(pGlobalInfo,'U');
	demoStrAdd(pGlobalInfo,'S');
	demoStrAdd(pGlobalInfo,':');
	if (pFlyAllInOneInfo->pMemory_Share_Common->bExtPhoneStatusBUS)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}

	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'I');
	demoStrAdd(pGlobalInfo,'O');
	demoStrAdd(pGlobalInfo,':');
	if (pFlyAllInOneInfo->pMemory_Share_Common->bExtPhoneStatusIO)
	{
		demoStrAdd(pGlobalInfo,'1');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'0');
	}
	//demoStrAdd(pGlobalInfo,';');
}

//#endif

#ifdef 	LDH
/*********************************************************************************
**函数名称:		demoOSDStateError
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDStateError(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	BYTE bFirstDelaySend = 1;
	BYTE bFirstIICSend = 1;
	/**********************************************/
	demoStrStart(pGlobalInfo,OSD_DEBUG_STATE_ERROR);
	/**********************************************/
	{
		UINT iEventWhat;
		ULONG iNowTime = GetTickCount();
		for (iEventWhat = 0;iEventWhat < EVENT_GLOBAL_DATA_CHANGE_MAX;iEventWhat++)
		{
			if (pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatStartTime[iEventWhat])
			{
				if (iNowTime - pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatStartTime[iEventWhat] >= 20000)
				{
					if (bFirstDelaySend)
					{
						bFirstDelaySend = FALSE;
						demoStrAdd(pGlobalInfo,'D');
						demoStrAdd(pGlobalInfo,'r');
						demoStrAdd(pGlobalInfo,'i');
						demoStrAdd(pGlobalInfo,'v');
						demoStrAdd(pGlobalInfo,'e');
						demoStrAdd(pGlobalInfo,'r');
						demoStrAdd(pGlobalInfo,' ');
						demoStrAdd(pGlobalInfo,'i');
						demoStrAdd(pGlobalInfo,'s');
						demoStrAdd(pGlobalInfo,' ');
						demoStrAdd(pGlobalInfo,'b');
						demoStrAdd(pGlobalInfo,'u');
						demoStrAdd(pGlobalInfo,'s');
						demoStrAdd(pGlobalInfo,'y');
						demoStrAdd(pGlobalInfo,'2');
						demoStrAdd(pGlobalInfo,'0');
						demoStrAdd(pGlobalInfo,'S');
						demoStrAdd(pGlobalInfo,'?');
					}
					demoStrAdd(pGlobalInfo,' ');
					demoStrAdd(pGlobalInfo,iEventWhat/100 + '0');
					demoStrAdd(pGlobalInfo,iEventWhat%100/10 + '0');
					demoStrAdd(pGlobalInfo,iEventWhat%10 + '0');
					if(pGlobalInfo->hDispatchMainThreadEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'G');
						demoStrAdd(pGlobalInfo,'B');
					}
					else if(pGlobalInfo->hHandleGlobalDVDEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'C');
						demoStrAdd(pGlobalInfo,'D');
					}
					else if(pGlobalInfo->hHandleGlobalAudioEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'A');
						demoStrAdd(pGlobalInfo,'U');
					}
					else if(pGlobalInfo->hHandleGlobalRadioEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'R');
						demoStrAdd(pGlobalInfo,'A');
					}
					else if(pGlobalInfo->hHandleGlobalVideoEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'V');
						demoStrAdd(pGlobalInfo,'O');
					}
					else if(pGlobalInfo->hHandleGlobalSystemEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'S');
						demoStrAdd(pGlobalInfo,'Y');
					}
					else if(pGlobalInfo->hHandleGlobalKeyEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'K');
						demoStrAdd(pGlobalInfo,'Y');
					}
					else if(pGlobalInfo->hHandleGlobalBTEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'F');
						demoStrAdd(pGlobalInfo,'B');
						demoStrAdd(pGlobalInfo,'T');
					}
					else if(pGlobalInfo->hHandleGlobalExBoxEvent
						== pFlyAllInOneInfo->pMemory_Share_Common->iGlobalDataChangeWhatHandle[iEventWhat])
					{
						demoStrAdd(pGlobalInfo,'E');
						demoStrAdd(pGlobalInfo,'x');
						demoStrAdd(pGlobalInfo,'B');
						demoStrAdd(pGlobalInfo,'o');
						demoStrAdd(pGlobalInfo,'x');
					}
					else
					{
						demoStrAdd(pGlobalInfo,'N');
						demoStrAdd(pGlobalInfo,'U');
						demoStrAdd(pGlobalInfo,'L');
						demoStrAdd(pGlobalInfo,'L');
					}
				}
			}
		}
	}
}
#endif


/*********************************************************************************
**函数名称:		demoOSDOpenStatus	
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDOpenStatus(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	#ifdef 	LDH
	BOOL bHaveSendInit = FALSE;
	BOOL bHaveSendOpen = FALSE;
	demoStrStart(pGlobalInfo,OSD_DEBUG_INIT_OPEN_STATUS);
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalDriver.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'G');demoStrAdd(pGlobalInfo,'B');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalCarbodyInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'C');demoStrAdd(pGlobalInfo,'B');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalSystemInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'S');demoStrAdd(pGlobalInfo,'Y');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalKeyInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'K');demoStrAdd(pGlobalInfo,'Y');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalBTInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'B');demoStrAdd(pGlobalInfo,'T');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalDVDInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'C');demoStrAdd(pGlobalInfo,'D');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalAudioInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'A');demoStrAdd(pGlobalInfo,'U');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalVideoInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'V');demoStrAdd(pGlobalInfo,'O');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalRadioInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'R');demoStrAdd(pGlobalInfo,'A');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalAssistDisplayInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'A');demoStrAdd(pGlobalInfo,'D');demoStrAdd(pGlobalInfo,' ');
	}
	//if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalXMRadioInfo.bInit)
	//{
	//	bHaveSendInit = TRUE;
	//	demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'S');demoStrAdd(pGlobalInfo,'R');demoStrAdd(pGlobalInfo,' ');
	//}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalTPMSInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'T');demoStrAdd(pGlobalInfo,'P');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalTVInfo.bInit)
	{
		bHaveSendInit = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'T');demoStrAdd(pGlobalInfo,'V');demoStrAdd(pGlobalInfo,' ');
	}
	if (bHaveSendInit)
	{
		demoStrAdd(pGlobalInfo,'N');demoStrAdd(pGlobalInfo,'o');demoStrAdd(pGlobalInfo,'t');
		demoStrAdd(pGlobalInfo,' ');demoStrAdd(pGlobalInfo,'I');demoStrAdd(pGlobalInfo,'n');demoStrAdd(pGlobalInfo,'i');demoStrAdd(pGlobalInfo,'t');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalDriver.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'G');demoStrAdd(pGlobalInfo,'B');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalCarbodyInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'C');demoStrAdd(pGlobalInfo,'B');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalSystemInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'S');demoStrAdd(pGlobalInfo,'Y');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalKeyInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'K');demoStrAdd(pGlobalInfo,'Y');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalBTInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'B');demoStrAdd(pGlobalInfo,'T');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalDVDInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'C');demoStrAdd(pGlobalInfo,'D');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalAudioInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'A');demoStrAdd(pGlobalInfo,'U');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalVideoInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'V');demoStrAdd(pGlobalInfo,'O');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalRadioInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'R');demoStrAdd(pGlobalInfo,'A');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalAssistDisplayInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'A');demoStrAdd(pGlobalInfo,'D');demoStrAdd(pGlobalInfo,' ');
	}
	//if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalXMRadioInfo.bOpen)
	//{
	//	bHaveSendOpen = TRUE;
	//	demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'S');demoStrAdd(pGlobalInfo,'R');demoStrAdd(pGlobalInfo,' ');
	//}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalTPMSInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'T');demoStrAdd(pGlobalInfo,'P');demoStrAdd(pGlobalInfo,' ');
	}
	if (!pGlobalInfo->pFlyDriverGlobalInfo->FlyGlobalTVInfo.bOpen)
	{
		bHaveSendOpen = TRUE;
		demoStrAdd(pGlobalInfo,'F');demoStrAdd(pGlobalInfo,'T');demoStrAdd(pGlobalInfo,'V');demoStrAdd(pGlobalInfo,' ');
	}
	if (bHaveSendOpen)
	{
		demoStrAdd(pGlobalInfo,'N');demoStrAdd(pGlobalInfo,'o');demoStrAdd(pGlobalInfo,'t');
		demoStrAdd(pGlobalInfo,' ');demoStrAdd(pGlobalInfo,'O');demoStrAdd(pGlobalInfo,'p');demoStrAdd(pGlobalInfo,'e');demoStrAdd(pGlobalInfo,'n');
	}
	#endif
	//demoStrAdd(pGlobalInfo,';');
}



//ifdef 	LDH
/*********************************************************************************
**函数名称:		demoOSDOtherInfo
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDOtherInfo(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	demoStrStart(pGlobalInfo,OSD_DEBUG_OTHER_INFO);

	demoStrAdd(pGlobalInfo,'H');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'s');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,':');
#if SHICHAN_SHICHAN
	demoStrAdd(pGlobalInfo,'1');
#else
	demoStrAdd(pGlobalInfo,'0');
#endif
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'N');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'v');
	demoStrAdd(pGlobalInfo,'i');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'M');
	demoStrAdd(pGlobalInfo,'i');
	demoStrAdd(pGlobalInfo,'x');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->GPSSpeaker+'0');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'A');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'d');
	demoStrAdd(pGlobalInfo,'i');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'I');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->eAudioInput/10+'0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->eAudioInput%10+'0');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->eCurAudioInput/10+'0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->eCurAudioInput%10+'0');	
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'V');
	demoStrAdd(pGlobalInfo,'i');
	demoStrAdd(pGlobalInfo,'d');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'I');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->eVideoInput/10+'0');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->eVideoInput%10+'0');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'K');
	demoStrAdd(pGlobalInfo,'W');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->bNoSendAndroidSystemButton+'0');
}
//#endif


/*********************************************************************************
**函数名称:		demoOSDADCThreadState
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
#ifdef 	LDH
void demoOSDADCThreadState(P_FLY_GLOBAL_INFO pGlobalInfo,UINT32 DebugMeg,UINT32 Count)
{
	UINT i = 0;
	static BYTE Flag = 0;
	char StringBuff[16];
	//DBG0(debugString("\n demoOSDADCThreadState");)

	char ThreadStateBuf[] = {"Hardware  ADC Thread  Death!!!  GetTickCount:"};
	char NothingBuf[] = 	  {"                                                    "};
	demoStrStart(pGlobalInfo,OSD_DEBUG_ADCSTATE);
	
	for(i = 0;i < strlen(ThreadStateBuf);i++)
	{
		demoStrAdd(pGlobalInfo,ThreadStateBuf[i]);
	}
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'0');
	demoStrAdd(pGlobalInfo,'X');
	snprintf(StringBuff, 15, "%x", (int)DebugMeg);
	for(i = 0;i < strlen(StringBuff);i++)
	{
		demoStrAdd(pGlobalInfo,StringBuff[i]);
	}
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,' ');
	snprintf(StringBuff, 15, "%d", (int)Count);
	for(i = 0;i < strlen(StringBuff)+1;i++)
	{
		demoStrAdd(pGlobalInfo,StringBuff[i]);
	}
	//demoStrAdd(pGlobalInfo,';');
	/*demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'d');
	demoStrAdd(pGlobalInfo,'w');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'A');
	demoStrAdd(pGlobalInfo,'D');
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'R');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'F');
	demoStrAdd(pGlobalInfo,'l');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'s');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'h');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'y');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'h');
	demoStrAdd(pGlobalInfo,'!');
	demoStrAdd(pGlobalInfo,'!');
	demoStrAdd(pGlobalInfo,'!');
	*/
}

/*********************************************************************************
**函数名称:		demoOSDADCThreadState
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDGetTickCount(P_FLY_GLOBAL_INFO pGlobalInfo,UINT32 DebugMeg,UINT32 Count)
{

	UINT i = 0;
	//DBG0(debugString("\n demoOSDGetTickCount");)
	char StringBuff[16];
	char ThreadStateBuf[] = {"GetTickCount Wait For Long Time !!! GetTickCount:"};
	char NothingBuf[] = 	  {"									"};
	demoStrStart(pGlobalInfo,OSD_DEBUG_GETTICKT);
	for(i = 0;i < strlen(ThreadStateBuf);i++)
	{
		demoStrAdd(pGlobalInfo,ThreadStateBuf[i]);
	}

	
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'0');
	demoStrAdd(pGlobalInfo,'X');
	snprintf(StringBuff, 15, "%x", (int)DebugMeg);

	for(i = 0;i < strlen(StringBuff);i++)
	{
		demoStrAdd(pGlobalInfo,StringBuff[i]);
	}
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'u');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,'t');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,' ');
	snprintf(StringBuff, 15, "%d", (int)Count);
	for(i = 0;i < strlen(StringBuff);i++)
	{
		demoStrAdd(pGlobalInfo,StringBuff[i]);
	}
	//demoStrAdd(pGlobalInfo,';');
}


/*********************************************************************************
**函数名称:		demoOSDSameTick
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDSameTick(P_FLY_GLOBAL_INFO pGlobalInfo,UINT32 Count)
{
	UINT i = 0;
	//DBG0(debugString("\n demoOSDGetTickCount");)
	char StringBuff[16];
	char ThreadStateBuf[] = {"The Same TickCount:"};
	char NothingBuf[] = 	{"					  "};
	demoStrStart(pGlobalInfo,OSD_DEBUG_SAMETICKT);
	for(i = 0;i < strlen(ThreadStateBuf);i++)
	{
		demoStrAdd(pGlobalInfo,ThreadStateBuf[i]);
	}
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'0');
	demoStrAdd(pGlobalInfo,'X');
	snprintf(StringBuff, 15, "%x", (int)Count);
	for(i = 0;i < strlen(StringBuff);i++)
	{
		demoStrAdd(pGlobalInfo,StringBuff[i]);
	}
	//demoStrAdd(pGlobalInfo,';');
}


/*********************************************************************************
**函数名称:		demoOSDSameTick
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDGlobalTime(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	UINT i = 0;	
	char ThreadStateBuf[] = {"2012 - 3 - 27 10:40"};
	demoStrStart(pGlobalInfo,OSD_DEBUG_GLOBALTIME);
	for(i = 0;i < strlen(ThreadStateBuf);i++)
	{
		demoStrAdd(pGlobalInfo,ThreadStateBuf[i]);
	}	
	//demoStrAdd(pGlobalInfo,';');
}
#endif

/*********************************************************************************
**函数名称:		demoOSDSameTick
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
void demoOSDGlobalDEMOKEY(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	UINT i = 0;	
	char ThreadStateBuf[] = {"EVENT_GLOBAL_DEMO_KEY_START"};
	demoStrStart(pGlobalInfo,EVENT_GLOBAL_DEMO_KEY_START); 
	for(i = 0;i < strlen(ThreadStateBuf);i++)
	{
		demoStrAdd(pGlobalInfo,ThreadStateBuf[i]);	   
	}	 
	//demoStrAdd(pGlobalInfo,';');
	
}
/*********************************************************************************
**函数名称:		iUARTDebugMsgOn
**函数功能:
**入口参数:
**返回参数:
**********************************************************************************/
#ifdef LDH
void demoOSDUARTDebugMsgOn(P_FLY_GLOBAL_INFO pGlobalInfo,BYTE UARTDebugMsgOn)
{
	UINT i = 0;	
	char ThreadStateBuf[] = {"iUARTDebugMsgOn"};
	DBG0(debugOneData("\n UARTDebugMsgOn = ",UARTDebugMsgOn);)
	demoStrStart(pGlobalInfo,UARTDEBUGMSGON); 
	for(i = 0;i < strlen(ThreadStateBuf);i++)
	{
		demoStrAdd(pGlobalInfo,ThreadStateBuf[i]);	   
	}	 
	demoStrAdd(pGlobalInfo,' ');	
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,UARTDebugMsgOn+'0');
	//demoStrAdd(pGlobalInfo,';');
}
#endif
void demoOSDPanelName(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	UINT i;

	demoStrStart(pGlobalInfo,OSD_DEBUG_PANEL_NAME);

	demoStrAdd(pGlobalInfo,'P');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'n');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'l');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[0]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[1]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[2]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[3]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[4]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[5]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[6]);
	demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->tPanelTab.PanelName[7]);
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'N');
	demoStrAdd(pGlobalInfo,'a');
	demoStrAdd(pGlobalInfo,'m');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,':');
	
	i = 0;
	while (i < SCAR_MODEL_MAX)
	{
		if (0x00 == pFlyAllInOneInfo->pMemory_Share_Common->sCarModule[i])
		{
			break;
		}
		demoStrAdd(pGlobalInfo,pFlyAllInOneInfo->pMemory_Share_Common->sCarModule[i]);
		i++;
	}

	//demoStrAdd(pGlobalInfo,';');
}

void demoOSDRunTime(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	struct tm *ptr;

	demoStrStart(pGlobalInfo,OSD_DEBUG_RUN_TIME);

	ptr = localtime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeFirstPowerOn);
	demoStrAdd(pGlobalInfo,'P');
	demoStrAdd(pGlobalInfo,'o');
	demoStrAdd(pGlobalInfo,'w');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,'r');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'O');
	demoStrAdd(pGlobalInfo,'N');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,ptr->tm_hour/10+'0');
	demoStrAdd(pGlobalInfo,ptr->tm_hour%10+'0');
	demoStrAdd(pGlobalInfo,'h');
	demoStrAdd(pGlobalInfo,ptr->tm_min/10+'0');
	demoStrAdd(pGlobalInfo,ptr->tm_min%10+'0');
	demoStrAdd(pGlobalInfo,'m');
	demoStrAdd(pGlobalInfo,ptr->tm_sec/10+'0');
	demoStrAdd(pGlobalInfo,ptr->tm_sec%10+'0');
	demoStrAdd(pGlobalInfo,'s');

	ptr = localtime(&pFlyAllInOneInfo->pMemory_Share_Common->SilencePowerOffInfo.timeLastUserAccOff);
	demoStrAdd(pGlobalInfo,'A');
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,'C');
	demoStrAdd(pGlobalInfo,' ');
	demoStrAdd(pGlobalInfo,'O');
	demoStrAdd(pGlobalInfo,'F');
	demoStrAdd(pGlobalInfo,'F');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,ptr->tm_hour/10+'0');
	demoStrAdd(pGlobalInfo,ptr->tm_hour%10+'0');
	demoStrAdd(pGlobalInfo,'h');
	demoStrAdd(pGlobalInfo,ptr->tm_min/10+'0');
	demoStrAdd(pGlobalInfo,ptr->tm_min%10+'0');
	demoStrAdd(pGlobalInfo,'m');
	demoStrAdd(pGlobalInfo,ptr->tm_sec/10+'0');
	demoStrAdd(pGlobalInfo,ptr->tm_sec%10+'0');
	demoStrAdd(pGlobalInfo,'s');

}


void demoOSDDvdType(P_FLY_GLOBAL_INFO pGlobalInfo)
{
	demoStrStart(pGlobalInfo,OSD_DEBUG_DVD_TYPE);
	demoStrAdd(pGlobalInfo,'D');
	demoStrAdd(pGlobalInfo,'V');
	demoStrAdd(pGlobalInfo,'D');
	demoStrAdd(pGlobalInfo,'T');
	demoStrAdd(pGlobalInfo,'y');
	demoStrAdd(pGlobalInfo,'p');
	demoStrAdd(pGlobalInfo,'e');
	demoStrAdd(pGlobalInfo,':');
	demoStrAdd(pGlobalInfo,' ');
	if(pFlyAllInOneInfo->pMemory_Share_Common->bDVDType)
	{

		demoStrAdd(pGlobalInfo,'F');
		demoStrAdd(pGlobalInfo,'L');
		demoStrAdd(pGlobalInfo,'Y');
		demoStrAdd(pGlobalInfo,'A');
		demoStrAdd(pGlobalInfo,'U');
		demoStrAdd(pGlobalInfo,'D');
		demoStrAdd(pGlobalInfo,'I');
		demoStrAdd(pGlobalInfo,'O');
		demoStrAdd(pGlobalInfo,' ');
		demoStrAdd(pGlobalInfo,'D');
		demoStrAdd(pGlobalInfo,'V');
		demoStrAdd(pGlobalInfo,'D');
	}
	else
	{
		demoStrAdd(pGlobalInfo,'F');
		demoStrAdd(pGlobalInfo,'O');
		demoStrAdd(pGlobalInfo,'R');
		demoStrAdd(pGlobalInfo,'Y');
		demoStrAdd(pGlobalInfo,'O');
		demoStrAdd(pGlobalInfo,'U');
		demoStrAdd(pGlobalInfo,' ');
		demoStrAdd(pGlobalInfo,'D');
		demoStrAdd(pGlobalInfo,'V');
		demoStrAdd(pGlobalInfo,'D');
	}
	
}




