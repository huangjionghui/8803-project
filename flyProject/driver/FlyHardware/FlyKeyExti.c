#include "FlyInclude.h"

void EncoderIDExchange(BYTE index)
{
	GlobalShareMmapInfo.pShareMemoryCommonData->iKeyIndex = index;
	GlobalShareMmapInfo.pShareMemoryCommonData->iKeyValue = pGlobalHardwareInfo->tPanelTab.EK_Value[index - 0x80];

	if (index >= 0x80 && index <= 0x83)
	{
		if (KB_VOL_INC == pGlobalHardwareInfo->tPanelTab.EK_Value[index - 0x80])
		{
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftIncCount++;
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftDecCount = 0;
		}
		else if (KB_VOL_DEC == pGlobalHardwareInfo->tPanelTab.EK_Value[index - 0x80])
		{
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftDecCount++;
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftIncCount = 0;
		}
		else if (KB_TUNE_INC == pGlobalHardwareInfo->tPanelTab.EK_Value[index - 0x80])
		{
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightIncCount++;
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightDecCount = 0;
		}
		else if (KB_TUNE_DEC == pGlobalHardwareInfo->tPanelTab.EK_Value[index - 0x80])
		{
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightDecCount++;
			pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightIncCount = 0;
		}
	}
}

irqreturn_t EncoderL1_io_isr(int irq, void *dev_id)
{
	DBG_ISR("ISRL1");

	pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft = pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft << 4;

	if (SOC_IO_Input(ENCODER_L1_G,ENCODER_L1_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft |= (1 << 2);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}

	if (SOC_IO_Input(ENCODER_L2_G,ENCODER_L2_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft |= (1 << 0);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}
	DBG_ISR("@L1:%X",pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft);

	if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x04 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x45 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x51 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x10)
	{
		EncoderIDExchange(0x80);
	}
	else if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x01 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x15 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x54 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x40)
	{
		EncoderIDExchange(0x81);
	}

	schedule_work(&pGlobalHardwareInfo->FlyKeyEncoderInfo.encoder_work);

	return IRQ_HANDLED;
}

irqreturn_t EncoderL2_io_isr(int irq, void *dev_id)
{
	DBG_ISR("ISRL2");


	pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft = pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft << 4;

	if (SOC_IO_Input(ENCODER_L1_G,ENCODER_L1_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft |= (1 << 2);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}

	if (SOC_IO_Input(ENCODER_L2_G,ENCODER_L2_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft |= (1 << 0);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}
	DBG_ISR("@L2:%X",pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft);

	if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x04 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x45 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x51 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x10)
	{
		EncoderIDExchange(0x80);
	}
	else if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x01 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x15 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x54 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueLeft == 0x40)
	{
		EncoderIDExchange(0x81);
	}

	schedule_work(&pGlobalHardwareInfo->FlyKeyEncoderInfo.encoder_work);
	return IRQ_HANDLED;
}

irqreturn_t EncoderR1_io_isr(int irq, void *dev_id)
{
	DBG_ISR("ISRR1");


	pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight = pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight << 4;

	if (SOC_IO_Input(ENCODER_R1_G,ENCODER_R1_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight |= (1 << 2);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}

	if (SOC_IO_Input(ENCODER_R2_G,ENCODER_R2_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight |= (1 << 0);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}
	DBG_ISR("@R1:%X",pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight);

	if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x04 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x45 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x51 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x10)
	{
		EncoderIDExchange(0x82);
	}
	else if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x01 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x15 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x54 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x40)
	{
		EncoderIDExchange(0x83);
	}

	schedule_work(&pGlobalHardwareInfo->FlyKeyEncoderInfo.encoder_work);
	return IRQ_HANDLED;
}

irqreturn_t EncoderR2_io_isr(int irq, void *dev_id)
{
	DBG_ISR("ISRR2");


	pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight = pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight << 4;

	if (SOC_IO_Input(ENCODER_R1_G,ENCODER_R1_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight |= (1 << 2);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}

	if (SOC_IO_Input(ENCODER_R2_G,ENCODER_R2_I,0))
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight |= (1 << 0);
		DBG_ISR("1");
	}
	else
	{
		DBG_ISR("0");
	}
	DBG_ISR("@R2:%X",pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight);

	if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x04 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x45 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x51 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x10)
	{
		EncoderIDExchange(0x82);
	}
	else if (pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x01 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x15 
		|| pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x54 || pGlobalHardwareInfo->FlyKeyEncoderInfo.curEncodeValueRight == 0x40)
	{
		EncoderIDExchange(0x83);
	}

	schedule_work(&pGlobalHardwareInfo->FlyKeyEncoderInfo.encoder_work);
	return IRQ_HANDLED;
}


static void workFlyKeyEncoder(struct work_struct *work)
{	
	DBG("\nThreadFlyKeyEncoder start");

	while (pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftIncCount >= 2)
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftIncCount--;
		putKeyMessage(KB_VOL_INC);
	}
	while (pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftDecCount >= 2)
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderLeftDecCount--;
		putKeyMessage(KB_VOL_DEC);
	}
	while (pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightIncCount >= 2)
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightIncCount-=2;
		putKeyMessage(KB_TUNE_INC);
	}
	while (pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightDecCount >= 2)
	{
		pGlobalHardwareInfo->FlyKeyEncoderInfo.iEncoderRightDecCount-=2;
		putKeyMessage(KB_TUNE_DEC);
	}

	pGlobalHardwareInfo->FlyKeyEncoderInfo.bTimeOutRun = TRUE;
	pGlobalHardwareInfo->FlyKeyEncoderInfo.time_out = GetTickCount();
}


void keyExtiFirstInit(void)
{
	INIT_WORK(&pGlobalHardwareInfo->FlyKeyEncoderInfo.encoder_work, workFlyKeyEncoder);

	SOC_IO_Input(ENCODER_L1_G,ENCODER_L1_I,0);
	SOC_IO_Input(ENCODER_L2_G,ENCODER_L2_I,0);
	SOC_IO_Input(ENCODER_R1_G,ENCODER_R1_I,0);
	SOC_IO_Input(ENCODER_R2_G,ENCODER_R2_I,0);
	SOC_IO_ISR_Add(ENCODER_ISR_L1,IRQ_TYPE_EDGE_BOTH,EncoderL1_io_isr,pGlobalHardwareInfo);
	SOC_IO_ISR_Add(ENCODER_ISR_L2,IRQ_TYPE_EDGE_BOTH,EncoderL2_io_isr,pGlobalHardwareInfo);
	SOC_IO_ISR_Add(ENCODER_ISR_R1,IRQ_TYPE_EDGE_BOTH,EncoderR1_io_isr,pGlobalHardwareInfo);
	SOC_IO_ISR_Add(ENCODER_ISR_R2,IRQ_TYPE_EDGE_BOTH,EncoderR2_io_isr,pGlobalHardwareInfo);
	SOC_IO_ISR_Enable(ENCODER_ISR_L1);
	SOC_IO_ISR_Enable(ENCODER_ISR_L2);
	SOC_IO_ISR_Enable(ENCODER_ISR_R1);
	SOC_IO_ISR_Enable(ENCODER_ISR_R2);

}
