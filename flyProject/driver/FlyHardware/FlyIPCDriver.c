#include "FlyInclude.h"


static void ipcDriverHardware(BYTE enumWhat)
{
	struct fly_hardware_info *pHardwareInfo = pGlobalHardwareInfo;

	printk("\nHardware ipcDriverHardware Start");

	switch (enumWhat)
	{
	case IPC_DRIVER_EVENT_MAIN_AUDIO_INPUT:
		switchMainAudioInput(GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverMainAudioInput);
		if (BACK != GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverMainAudioInput)
		{
			switchSecondAudioInput(GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverMainAudioInput);
		}	
		break;

	case IPC_DRIVER_EVENT_RESET_7741:
		ioControl7741Reset(TRUE);
		msleep(100);
		ioControl7741Reset(FALSE);
		break;

	case IPC_DRIVER_EVENT_AMP_MUTE:
		ioControlAMPMute(GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverbAMPMute);
		break;

	case IPC_DRIVER_EVENT_AMP_ONOFF:
		ioControlAMPOn(GlobalShareMmapInfo.pShareMemoryCommonData->ipcDriverAMPOn);
		break;
	default:
		break;
	}
}

void ipcDriverFirstInit(void)
{
	global_fops._p_ipcDriver[IPC_DRIVER_HARDWARE] = ipcDriverHardware;
}