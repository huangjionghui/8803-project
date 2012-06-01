#ifndef    __FLYDEBUGMSG_H__
#define	   __FLYDEBUGMSG_H__

void demoOSDHostTemperature(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDDVDSoftVersion(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDDriversCompileTime(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDKeyADDisplay(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDBreakAndPhoneStatus(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDStateError(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDOpenStatus(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDOtherInfo(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDGetTickCount(P_FLY_GLOBAL_INFO pGlobalInfo,UINT32 DebugMeg,UINT32 Count);
void demoOSDADCThreadState(P_FLY_GLOBAL_INFO pGlobalInfo,UINT32 DebugMeg,UINT32 Count);
void demoOSDSameTick(P_FLY_GLOBAL_INFO pGlobalInfo,UINT32 Count);
void demoOSDGlobalTime(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDGlobalDEMOKEY(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDUARTDebugMsgOn(P_FLY_GLOBAL_INFO pGlobalInfo,BYTE UARTDebugMsgOn);
void demoOSDPanelName(P_FLY_GLOBAL_INFO pGlobalInfo);
void demoOSDRunTime(P_FLY_GLOBAL_INFO pGlobalInfo);
#endif	   
	