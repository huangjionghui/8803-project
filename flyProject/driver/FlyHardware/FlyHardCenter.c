#include "FlyInclude.h"

#include "../include/fly_soc_iic.h"





struct fly_hardware_info *pGlobalHardwareInfo = NULL;

static void freeHardwareStruct(struct fly_hardware_info *pHardwareInfo);

void actualIICWrite(BYTE busID, BYTE chipAddW, BYTE *buf, UINT size)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest && I2_1_ID == busID)
	{
		return;
	}
	SOC_I2C_Send(busID,chipAddW>>1,&buf[0],size);
}

void actualIICRead(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest && I2_1_ID == busID)
	{
		return;
	}
	SOC_I2C_Rec(busID,chipAddW>>1,subAddr,&buf[0],size);
}

void actualIICReadSimple(BYTE busID, BYTE chipAddW, BYTE *buf, UINT size)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest && I2_1_ID == busID)
	{
		return;
	}
	SOC_I2C_Rec_Simple(busID,chipAddW>>1,&buf[0],size);
}

void actualIICReadSAF7741(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest && I2_1_ID == busID)
	{
		return;
	}
	SOC_I2C_Rec_SAF7741(busID,chipAddW>>1,subAddr,&buf[0],size);
}

void actualIICWriteTEF7000(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest && I2_1_ID == busID)
	{
		return;
	}
	SOC_I2C_Send_TEF7000(busID,chipAddW>>1,subAddr,&buf[0],size);
}

void actualIICReadTEF7000(BYTE busID, BYTE chipAddW, UINT subAddr, BYTE *buf, UINT size)
{
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreSendNormalIICTest && I2_1_ID == busID)
	{
		return;
	}
	SOC_I2C_Rec_TEF7000(busID,chipAddW>>1,subAddr,&buf[0],size);
}

////////////////////////////////////////以下打火处理
void voltageShakeInit(void)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowHardware = 80;
}

void voltageShakeProc(BYTE iBatteryVoltage)
{
	//电压抖动处理
	if (iBatteryVoltage
		< GlobalShareMmapInfo.pShareMemoryCommonData->iNeedProcVoltageShakeRadio)//低于收音机
	{
		pGlobalHardwareInfo->iProcVoltageShakeDelayTime = GetTickCount();
		GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowRadio = TRUE;
		DBG0("\nFlyAudio System Radio Voltage Low");
	}
	if (iBatteryVoltage
		< GlobalShareMmapInfo.pShareMemoryCommonData->iNeedProcVoltageShakeAudio)//低于音频
	{
		pGlobalHardwareInfo->iProcVoltageShakeDelayTime = GetTickCount();
		if (!GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio)
		{
			GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio = TRUE;
			ipcStartEvent(EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID);
		}
		DBG0("\nFlyAudio System Audio Voltage Low");
	}
	if (iBatteryVoltage
		< GlobalShareMmapInfo.pShareMemoryCommonData->iNeedProcVoltageShakeHardware)//低于音频
	{
		pGlobalHardwareInfo->iProcVoltageShakeDelayTime = GetTickCount();
		GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowHardware = TRUE;
		DBG0("\nFlyAudio System Hardware Voltage Low");
	}
}

void voltageShakeThread(void)
{
	if (pGlobalHardwareInfo->iProcVoltageShakeDelayTime)//之前电压不正常
	{
		if (GetTickCount() - pGlobalHardwareInfo->iProcVoltageShakeDelayTime >= LOW_VOLTAGE_DELAY)//电压正常一段时间后
		{
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowRadio)//收音机
			{
				GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowRadio = FALSE;
				ipcStartEvent(EVENT_GLOBAL_BATTERY_RECOVERY_RADIO_ID);
				DBG0("\n FlyAudio System Radio Voltage After Low Proc");
			}
			if (GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio)//音频
			{
				GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowAudio = FALSE;
				ipcStartEvent(EVENT_GLOBAL_BATTERY_RECOVERY_AUDIO_ID);
				DBG0("\nFlyAudio System Audio Voltage After Low Proc");
			}

			pGlobalHardwareInfo->iProcVoltageShakeDelayTime = 0;
		}
	}
}

void voltageShakeDeInit(void)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->bBatteryVoltageLowHardware = 0;
}
////////////////////////////////////////以上打火处理


BYTE consoleDebug = 0;
void consoleDebugSwitch(void)
{
#if CONSOLE_DEBUG
	if (consoleDebug != GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iUARTDebugMsgOn)
	{
		consoleDebug = GlobalShareMmapInfo.pShareMemoryCommonData->flyRestoreData.iUARTDebugMsgOn;
		FLY_CONSOLE = consoleDebug;
	}
#endif

}

void cleanSomeGlobalValuesSuspend(void)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->bHaveRecMCUACCOn = FALSE;
	pGlobalHardwareInfo->iTouchTimeoutTime = 0;
}

void cleanSomeGlobalValuesResume(void)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->bHaveRecMCUACCOff = FALSE;
}



static int hardware_open(struct inode *inode, struct file *filp)
{
	//将设备结构体指针赋值给文件私有数据指针
  	filp->private_data = pGlobalHardwareInfo;
	
	DBG0("hardware open OK!\n");
	return 0;
}

static ssize_t hardware_write(struct file *filp, const char *buffer, size_t count, loff_t * ppos)
{
	//获得设备结构体指针
	struct fly_hardware_info *pHardwareInfo = filp->private_data; 
	BYTE localData[256];
	BYTE *pData = NULL;
	BOOL bNewLarge = FALSE;

	//UINT32 i;

	if (pHardwareInfo == NULL)
	{
		DBG0("\nFlyHardware write pHardwareInfo is NULL");
		return -1;
	}
	
	if (count <= 256)
	{
		pData = localData;
	}
	else
	{
		pData = (BYTE *)kmalloc(count, GFP_KERNEL);
		bNewLarge = TRUE;
	}
	
	//获得用户空间的数据
	if (copy_from_user(pData, buffer, count))
	{
		DBG0("copy data from user error");
		return -EFAULT;
	}
	else
	{
		//DBG(
		//	DBG("\nFlyHardware write %d byte:", count);
		//	for (i=0; i<count; i++)
		//	{
		//		DBG("%02X ",pData[i]);
		//	}
		//)
	
		//数据处理
		dealDataFromUser(pData,count);
	}
	
	if (bNewLarge)
	{
		kfree(pData);
	}

	return count;
}

static ssize_t hardware_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	//获得设备结构体指针
	struct fly_hardware_info *pHardwareInfo = filp->private_data; 
		
	BYTE localData[256];
	BYTE *pData = NULL;
	BOOL bNewLarge = FALSE;
	UINT32 returnCount = 0;

	BYTE currentHAL;

	UINT32 i;

	if (pHardwareInfo == NULL)
	{
		DBG0("\nFlyHardware read pHardwareInfo is NULL");
		return -1;
	}

	if (count <= 256)
	{
		pData = localData;
	}
	else
	{
		pData = (BYTE *)kmalloc(count, GFP_KERNEL);
		bNewLarge = TRUE;
	}
	
	//获得用户空间的数据
	if (copy_from_user(pData, buffer, count))
	{
		DBG0("copy data from user error");
		return -EFAULT;
	}
	else
	{
		currentHAL = pData[0];
		DBG("\nFlyHardware read CurHAL ID:%x Block %x", currentHAL,pData[1]);
		
		if (S_NO_BLOCK_ID == pData[1])
		{
			returnCount = noBlockMessageToHAL(currentHAL,&pData[2], count - 2);
			if (returnCount > 0)
			{
				returnCount += 2;
			}
		}

		if (S_BLOCK_ID == pData[1])
		{
			returnCount = blockMessageToHAL(&pData[0],count);
		}
		
	}	

	if (returnCount)
	{
		if (copy_to_user(buffer,pData,returnCount))
		{
			DBG0("copy_to_user error!\n");
			return -EFAULT;
		}
	}
	else
		return -1;

	DBG("\nFlyHardware read %d byte:", returnCount);
		for (i=0; i<returnCount; i++)
		{
			DBG("%02X ",pData[i]);
		}

	if (bNewLarge)
	{
		kfree(pData);
	}

	return returnCount;	
}

static unsigned int hardware_poll(struct file *filp, poll_table *wait)
{
	struct fly_hardware_info *pHardwareInfo = filp->private_data; 
	unsigned int mask = 0;

	poll_wait(filp, &pHardwareInfo->read_wait, wait);

	if ((pHardwareInfo->serviceMessageBuffLx != pHardwareInfo->serviceMessageBuffHx) ||
		(pHardwareInfo->keyMessageBuffLx != pHardwareInfo->keyMessageBuffHx) ||
		(pHardwareInfo->radioMessageBuffLx != pHardwareInfo->radioMessageBuffHx) ||
		(pHardwareInfo->systemMessageBuffLx != pHardwareInfo->systemMessageBuffHx) ||
		(pHardwareInfo->acMessageBuffLx != pHardwareInfo->acMessageBuffHx))
	{
		mask |= POLLIN|POLLRDNORM;
	}
	
	//printk("FlyHardware driver have datas to read\n");
	return mask;
}

static int hardware_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	//获得设备结构体指针
	struct fly_hardware_info *pHardwareInfo = filp->private_data; 
	if (pHardwareInfo == NULL)
	{
		DBG0("hardware_ioctl pHardwareInfo is NULL");
		return -1;
	}

	return 0;
}

static int hardware_release(struct inode *inode, struct file *filp)
{
	struct fly_hardware_info *pHardwareInfo = filp->private_data; 
	if (pHardwareInfo == NULL)
	{
		DBG0("hardware_release pHardwareInfo is NULL");
		return -1;
	}


	DBG0("FlyHardware close OK!\n");
	return 0;
}


static struct file_operations hardware_fops = {
	.owner	 =   THIS_MODULE,
	.open    =   hardware_open,
	.write   =   hardware_write,
	.read    =   hardware_read,
	.poll    =   hardware_poll,
	.ioctl   =   hardware_ioctl,
	.release =   hardware_release,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &hardware_fops,
};


static int hardware_probe(struct platform_device*dev)
{
	int ret = -1;
	
	//注册设备
	ret = misc_register(&misc);
	if (ret)
	{
		//注册失败，释放设备结构体内存
		freeHardwareStruct(pGlobalHardwareInfo);
		return ret;
	}
	
	DBG0("Board %d\n", PCB_8803_DISP_SEL);
	DBG0("\nFlyHardware initialization %s %s \n", __TIME__, __DATE__);
	return ret;
}

static int hardware_remove(struct platform_device *dev)
{
	//注销设备
	misc_deregister(&misc);

	return 0;
}

static void hardware_shutdown(struct platform_device *dev)
{
	return;
}

static int hardware_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}
static int hardware_resume(struct platform_device *dev)
{
	systemControlAccOn();
	printk("\nhardware resume\n");
	return 0;
}
static void early_hardware_suspend(struct early_suspend *h)
{
	cleanSomeGlobalValuesSuspend();
	GlobalShareMmapInfo.pShareMemoryCommonData->bPrepareToSleep = TRUE;
	printk("early_hardware_suspend\n");
}
static void late_hardware_resume(struct early_suspend *h)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->bPrepareToSleep = FALSE;
	cleanSomeGlobalValuesResume();
	printk("late_hardware_resume");
}

static struct platform_driver hardware_driver = {
	.probe     = hardware_probe,
	.remove    = hardware_remove,
	.shutdown  = hardware_shutdown,
	.suspend   = hardware_suspend,
	.resume    = hardware_resume,
	.driver    = {
		.owner = THIS_MODULE,
		.name  = DEVICE_NAME,
	},
};

void sleepOnProcSleep(void)
{
	struct fly_hardware_info *pHardwareInfo = pGlobalHardwareInfo;

	if (pHardwareInfo->bNeedWakeupThread)
	{
		return;
	}
	pHardwareInfo->bNeedWakeupThread = TRUE;
	
	DBG0("\nsleepOnProcSleep Start");

	ioPowerOff();
	
	DBG0("\nsleepOnProcSleep End");
}

void sleepOnProcWakeup(void)
{
	struct fly_hardware_info *pHardwareInfo = pGlobalHardwareInfo;

	if (!pHardwareInfo->bNeedWakeupThread)
	{
		return;
	}
	pHardwareInfo->bNeedWakeupThread = FALSE;
	
	DBG0("\nsleepOnProcWakeup Start");

	ioPowerOn();
	
	soundPowerOn();
	halAudioPowerOn();
	halVideoPowerOn();
	
	DBG0("\nsleepOnProcWakeup End");
}

static void freeHardwareStruct(struct fly_hardware_info *pHardwareInfo)
{
	voltageShakeDeInit();

	pGlobalHardwareInfo->bFlyKeyADCDelayWorkRunning = FALSE;
	cancel_delayed_work(&pGlobalHardwareInfo->adc_delay_work);

	SOC_IO_ISR_Disable(MCU_IIC_REQ_ISR);
	SOC_IO_ISR_Del(MCU_IIC_REQ_ISR);
	cancel_work_sync(&pGlobalHardwareInfo->FlyIICInfo.iic_work);
	msleep(50);


	SOC_IO_ISR_Disable(ENCODER_ISR_L1);
	SOC_IO_ISR_Disable(ENCODER_ISR_L2);
	SOC_IO_ISR_Disable(ENCODER_ISR_R1);
	SOC_IO_ISR_Disable(ENCODER_ISR_R2);
	SOC_IO_ISR_Del(ENCODER_ISR_L1);
	SOC_IO_ISR_Del(ENCODER_ISR_L2);
	SOC_IO_ISR_Del(ENCODER_ISR_R1);
	SOC_IO_ISR_Del(ENCODER_ISR_R2);
	
	cancel_work_sync(&pGlobalHardwareInfo->FlyKeyEncoderInfo.encoder_work);
	msleep(50);

	//释放设备结构体内存
	if (pHardwareInfo)
	{
		kfree(pHardwareInfo); 
		pHardwareInfo = pGlobalHardwareInfo = NULL;
	}
}

static int __init hardware_init(void)
{
	int ret = -1;

	// 动态申请设备结构体的内存
  	pGlobalHardwareInfo = (struct fly_hardware_info *)kmalloc(sizeof(struct fly_hardware_info), GFP_KERNEL);
  	if (pGlobalHardwareInfo == NULL)
  	{
		DBG0("kmalloc error!\n");
    	return ret;
  	}
	memset(pGlobalHardwareInfo,0,sizeof(struct fly_hardware_info));
	
	ipcDriverFirstInit();
		
	ioFirstInit();
	soundFirstInit();
	halProcFirstInit();
	keyFirstInit();
	halVideoFirstInit();
	halAudioFirstInit();
	mcuFirstInit();

	voltageShakeInit();
	pGlobalHardwareInfo->early_suspend.level = 50;
	pGlobalHardwareInfo->early_suspend.suspend = early_hardware_suspend;
	pGlobalHardwareInfo->early_suspend.resume  = late_hardware_resume;
	register_early_suspend(&pGlobalHardwareInfo->early_suspend);
	
	return platform_driver_register(&hardware_driver);
}

static void __exit hardware_exit(void)
{

	freeHardwareStruct(pGlobalHardwareInfo);
	
	platform_driver_unregister(&hardware_driver);

	DBG0("FlyHardware unload\n");
}

module_init(hardware_init);
module_exit(hardware_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");
