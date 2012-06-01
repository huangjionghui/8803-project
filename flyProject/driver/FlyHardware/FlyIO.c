
#include "FlyInclude.h"

void controlLCDPWM(BYTE iPWM)
{
	SOC_BL_Set(100 - iPWM);
}

void ioControlLCDIdleStatus(BOOL bStatus)
{
	BOOL ret;
	
#if PCB_8803_DISP_SEL == PCB_8803_DISP_V1
	if (bStatus)
	{
		SOC_IO_Output(SYSTEM_LCD_IDLE_GROUP,SYSTEM_LCD_IDLE_GPIO,0);
	}
	else
	{
		ret = SOC_IO_Input(SYSTEM_LCD_IDLE_GROUP,SYSTEM_LCD_IDLE_GPIO,GPIO_PULL_DISABLE);
		printk("\nLCD IDLE GPIO:%d\n", ret);
	}
#else
	if (bStatus)
	{
		SOC_IO_Output(SYSTEM_LCD_IDLE_GROUP,SYSTEM_LCD_IDLE_GPIO,1);
	}
	else
	{
		SOC_IO_Output(SYSTEM_LCD_IDLE_GROUP,SYSTEM_LCD_IDLE_GPIO,0);
	}
#endif
}

void ioControlAMPOn(BOOL bOn)
{
	BYTE buff[] = {0x00,0x91,0x00};

	if (bOn)
	{
		buff[2] = 0x01;
	}
	LPCCombinDataStream(buff, sizeof(buff));
}

void ioControl3GPowerEn(BOOL bPowerOn)
{
	BYTE buff[] = {0x00,0x80,0x00};

	if (bPowerOn)
	{
		buff[2] = 0x01;
	}
	LPCCombinDataStream(buff, sizeof(buff));

	if (bPowerOn)
	{
		SOC_IO_Output(G3G_POWER_G,G3G_POWER_I,1);
	}
	else
	{
		SOC_IO_Output(G3G_POWER_G,G3G_POWER_I,0);
	}
}

void ioControl3GLED(BOOL bOn)
{
	if (bOn)
	{
		SOC_IO_Output(G3G_LED_G,G3G_LED_I,1);
	}
	else
	{
		SOC_IO_Output(G3G_LED_G,G3G_LED_I,0);
	}
}

void ioControl3GRst(BOOL bRst)
{
	if (bRst)
	{
		SOC_IO_Output(G3G_RST_G,G3G_RST_I,0);
	}
	else
	{
		SOC_IO_Output(G3G_RST_G,G3G_RST_I,1);
	}
}

void ioControlBackCamEn(BOOL bEn)
{
	if (bEn)
	{
		SOC_IO_Output(BACK_CAM_EN_G,BACK_CAM_EN_I,1);
	}
	else
	{
		SOC_IO_Output(BACK_CAM_EN_G,BACK_CAM_EN_I,0);
	}
}
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////串转并芯片
//////////////////////////////////////////////////////////////////

void ioControlUSBHostReset(BOOL bReset)
{
	BYTE buff[2] = {1,0};
	if (bReset)
	{
		pGlobalHardwareInfo->serialToParallelOut &= ~(1<<6);
	}
	else
	{
		pGlobalHardwareInfo->serialToParallelOut |= 1 << 6;
	}
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);

	if (bReset)
	{
		SOC_IO_Output(HUB_RST_G,HUB_RST_I,0);
	}
	else
	{
		SOC_IO_Output(HUB_RST_G,HUB_RST_I,1);
	}
}

void ioControlDVDReset(BYTE iPara)
{

	BYTE buff[2] = {1,0};
	if (iPara)
	{
		pGlobalHardwareInfo->serialToParallelOut |= 1 << 3;
	}
	else
	{
		pGlobalHardwareInfo->serialToParallelOut &= ~(1<<3);
	}
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);

	if (iPara)
	{
		SOC_IO_Output(DVD_RST_G,DVD_RST_I,1);
	}
	else
	{
		SOC_IO_Output(DVD_RST_G,DVD_RST_I,0);
	}
}

void ioControlAMPMute(BOOL bMute)
{
	BYTE buff[2] = {1,0};

#if PCB_8803_DISP_SEL != PCB_8803_DISP_V1//因为PCB_8803_DISP_V1有用做其它用途
	if (bMute)
	{
		SOC_IO_Output(AUDIO_AMP_MUTE_G,AUDIO_AMP_MUTE_I,0);
	}
	else
	{
		SOC_IO_Output(AUDIO_AMP_MUTE_G,AUDIO_AMP_MUTE_I,1);
	}
#endif

	if (bMute)
	{
		pGlobalHardwareInfo->serialToParallelOut &= ~(1<<4);
	}
	else
	{
		pGlobalHardwareInfo->serialToParallelOut |= 1 << 4;
	}
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
}

void ioControlRadioANT(BYTE iPara)
{
	BYTE buff[2] = {1,0};

	if (iPara)
	{
		SOC_IO_Output(RADIO_ANT1_G,RADIO_ANT1_I,1);
	}
	else
	{
		SOC_IO_Output(RADIO_ANT1_G,RADIO_ANT1_I,0);
	}

	if (iPara)
	{
		pGlobalHardwareInfo->serialToParallelOut |= 1 << 2;
	}
	else
	{
		pGlobalHardwareInfo->serialToParallelOut &= ~(1<<2);
	}
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
	printk("ioControlRadioANT : %d\n",iPara);
}

void ioControlRadioAFMUTE(BYTE iPara)
{
	BYTE buff[2] = {1,0};

#if PCB_8803_DISP_SEL != PCB_8803_DISP_V1//因为PCB_8803_DISP_V1有用做其它用途
	if (iPara)
	{
		SOC_IO_Output(RADIO_AF_MUTE_7741_RST_G,RADIO_AF_MUTE_7741_RST_I,1);
	}
	else
	{
		SOC_IO_Output(RADIO_AF_MUTE_7741_RST_G,RADIO_AF_MUTE_7741_RST_I,0);
	}
#endif

	if (iPara)
	{
		pGlobalHardwareInfo->serialToParallelOut |= 1 << 5;
	}
	else
	{
		pGlobalHardwareInfo->serialToParallelOut &= ~(1<<5);
	}
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
}

void ioControl7741Reset(BOOL bOn)
{
	BYTE buff[2] = {1,0};

#if PCB_8803_DISP_SEL != PCB_8803_DISP_V1//因为PCB_8803_DISP_V1有用做其它用途
	if (bOn)
	{
		SOC_IO_Output(RADIO_AF_MUTE_7741_RST_G,RADIO_AF_MUTE_7741_RST_I,0);
	}
	else
	{
		SOC_IO_Output(RADIO_AF_MUTE_7741_RST_G,RADIO_AF_MUTE_7741_RST_I,1);
	}
#endif

	if (bOn)
	{
		pGlobalHardwareInfo->serialToParallelOut &= ~(1<<5);
	}
	else
	{
		pGlobalHardwareInfo->serialToParallelOut |= 1 << 5;
	}
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
}

void serialToParallelParaInit(void)
{
	pGlobalHardwareInfo->serialToParallelOut = 0;
}

void serialToParallelIOInit(void)
{
	BYTE buff[2];
	buff[0] = 1;buff[2] = pGlobalHardwareInfo->serialToParallelOut;//控制初始值
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
	buff[0] = 2;buff[1] = 0;//极性反转控制
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
	buff[0] = 3;buff[1] = 0;//全部输出
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
}

void serialToParallelPowerOn(void)
{
	BYTE buff[2] = {1,0};
	pGlobalHardwareInfo->serialToParallelOut |= 1 << 0;//LED9V_EN
	pGlobalHardwareInfo->serialToParallelOut |= 1 << 1;//ACC_EN
	pGlobalHardwareInfo->serialToParallelOut |= 1 << 7;//GPRS_EN
	pGlobalHardwareInfo->serialToParallelOut |= 1 << 6;//GPRS_RST
	buff[1] = pGlobalHardwareInfo->serialToParallelOut;
	actualIICWrite(I2_1_ID,PCA9554_ADD_W,&buff[0],2);
}

void ioPowerOff(void)
{
	//ioControl3GPowerEn(FALSE);//不断电

	//ioControlUSBHostReset(TRUE);//不复位

	SOC_IO_Output(BT_POWER_GROUP,BT_POWER_GPIO,0);//蓝牙电源

	controlLCDPWM(0);
	SOC_IO_Output(SYSTEM_PANNE_PEN_GROUP,SYSTEM_PANNE_PEN,0);
	//ioControlLCDIdleStatus(1);
	SOC_IO_Output(DVD_LEDC_GROUP,DVD_LEDC_GPIO,0);
}

void ioPowerOn(void)
{
	ADC_Gpio_Init();

	ioControlUSBHostReset(TRUE);
	msleep(10);

	controlLCDPWM(100);
	SOC_IO_Output(SYSTEM_PANNE_PEN_GROUP,SYSTEM_PANNE_PEN,1);
	//ioControlLCDIdleStatus(0);
	SOC_IO_Output(DVD_LEDC_GROUP,DVD_LEDC_GPIO,0);

	serialToParallelIOInit();
	serialToParallelPowerOn();

	SOC_IO_Output(SYSTEM_FAN_CONT_GROUP,SYSTEM_FAN_GPIO,0);

	ioControlUSBHostReset(FALSE);

	msleep(10);
	ioControl3GPowerEn(TRUE);
}

void ioFirstInit(void)
{
	SOC_IO_Output(DVD_LEDC_GROUP,DVD_LEDC_GPIO,0);
	SOC_IO_Output(SYSTEM_FAN_CONT_GROUP,SYSTEM_FAN_GPIO,0);

	serialToParallelParaInit();
	serialToParallelIOInit();
	serialToParallelPowerOn();
}
