#include "FlyInclude.h"

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////输出
///////////////////////////////////////////////////////////////

void LPCCombinDataStream(BYTE *p, UINT len)
{
	UINT i=0;
	BYTE checksum = 0;
	BYTE bufData[256];
	BYTE *buf;
	bool bMalloc = FALSE;
	if (3+len+1 > 255)
	{
		buf = (BYTE *)kmalloc(sizeof(BYTE)*(4+len), GFP_KERNEL);
		bMalloc = TRUE;
	}
	else
	{
		buf = bufData;
	}

	buf[0] = 0xFF;
	buf[1] = 0x55;
	buf[2] = len+1;
	checksum = buf[2];
	for (i=0; i<len; i++)
	{
		buf[3+i] = p[i];
		checksum += p[i];
	}

	buf[3+i] = checksum;

	printk("ToMCU:%x %x %x\n",p[0],p[1],p[2]);
	if (!GlobalShareMmapInfo.pShareMemoryCommonData->bNoMoreToSendDataWhenToSleep)
	{
		actualIICWrite(I2_0_ID,MCU_ADDR_W,buf,3+i+1);
	}
	else
	{
		printk("ToMcu Blocked\n");
	}

	if (bMalloc)
	{
		kfree(buf);
	}
}

void LPCPowerOnOK(void)
{
	BYTE buff[] = {0x00,0x00,0x00};

	DBG0("\nControl To MCU Power On######################");
	LPCCombinDataStream(buff, 3);
}

void LPCControlHowLongToPowerOn(ULONG iTime)
{
	BYTE buff[] = {0x00,0x98,0x00,0x00,0x00,0x00};

	buff[5] = (BYTE)(iTime);
	iTime = iTime >> 8;
	buff[4] = (BYTE)(iTime);
	iTime = iTime >> 8;
	buff[3] = (BYTE)(iTime);
	iTime = iTime >> 8;
	buff[2] = (BYTE)(iTime);

	LPCCombinDataStream(buff, 6);
}

void LPCControlToSleep(void)
{
	BYTE buff[] = {0x00,0x01,0x00};

	LPCCombinDataStream(buff, 3);
}
void LPCControlReset(void)
{
	BYTE buff[] = {0x00,0x03,0x04};

	LPCCombinDataStream(buff, 3);
}
static void LPCControlTurnOff(void)
{
	BYTE buff[] = {0x00,0x01,0x00};

	LPCCombinDataStream(buff, 3);
}

static void LPCAmpliferPowerMode(BOOL bPower)
{
	BYTE buff[] = {0x00,0x91,0x00};

	if (bPower)
	{
		buff[2] = 0x01;
	}

	LPCCombinDataStream(buff, 3);
}

void systemReqRadioAD(void)
{
	BYTE buff[] = {0x00,0x93,0x00};

	LPCCombinDataStream(buff, 3);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////输入
///////////////////////////////////////////////////////////////

void LPCToAcHAL(BYTE *p,UINT length)
{
	if (length > AC_MESSAGE_BUFF_LENGTH - 2)
	{
		DBG("\nAC_MESSAGE_BUFF_LENGTH");
		return;
	}
	down(&pGlobalHardwareInfo->semAcMessage);
	pGlobalHardwareInfo->acMessageBuff[pGlobalHardwareInfo->acMessageBuffHx][0] = length+1;
	pGlobalHardwareInfo->acMessageBuff[pGlobalHardwareInfo->acMessageBuffHx][1] = 0x20;
	memcpy(&pGlobalHardwareInfo->acMessageBuff[pGlobalHardwareInfo->acMessageBuffHx][2],p,length);
	pGlobalHardwareInfo->acMessageBuffHx++;
	if (pGlobalHardwareInfo->acMessageBuffHx >= AC_MESSAGE_BUFF_SIZE)
	{
		pGlobalHardwareInfo->acMessageBuffHx = 0;
	}
	up(&pGlobalHardwareInfo->semAcMessage);
	wake_up_interruptible(&pGlobalHardwareInfo->read_wait);
}

static void LPCdealReadFromMCUAll(BYTE *p,UINT length)
{
	UINT i;

	if (0x00 == p[0] && (p[1] <= 0x10 || p[1] == 0xFE || p[1] == 0xA2))
	{
		if (0x01 == p[1])
		{
			DBG0("\nRead From MCU ######################");
		}
		else
		{
			DBG0("\nRead From MCU ");
		}
		for (i = 0;i < length;i++)
		{
			DBG0(" %x",p[i]);
		}
		DBG0("\n");
	}

	switch (p[0])
	{
	case 0x00:
		switch (p[1])
		{
		case 0x90://RDS
			messageToRadioHAL(&p[1],length - 1);
			break;
		case 0x93://Radio AD
			pGlobalHardwareInfo->iRadioADValue = p[2]*256 + p[3];
			pGlobalHardwareInfo->bRadioADRead = FALSE;
			complete(&pGlobalHardwareInfo->compRadioADRead);
			break;
		case 0x92:
			break;
		case 0x22:
			GlobalShareMmapInfo.pShareMemoryCommonData->iBatteryVoltage = p[2];
			voltageShakeProc(GlobalShareMmapInfo.pShareMemoryCommonData->iBatteryVoltage);
			messageMCUToSystemHAL(&p[0],length);
			break;
		default:
			messageMCUToSystemHAL(&p[0],length);
			break;
		}
		break;
	case 0x02:
		putKeyMessage(p[2]);
		break;
	case 0x04:
		LPCToAcHAL(&p[0],length);
		break;
	default:
		break;
	}
}

static BOOL readFromMCUProcessor(BYTE *p,UINT length)
{
	UINT i;

	for (i = 0;i < length;i++)
	{
		switch (pGlobalHardwareInfo->buffFromMCUProcessorStatus)
		{
		case 0:
			if (0xFF == p[i])
			{
				pGlobalHardwareInfo->buffFromMCUProcessorStatus = 1;
			}
			break;
		case 1:
			if (0xFF == p[i])
			{
				pGlobalHardwareInfo->buffFromMCUProcessorStatus = 1;
			}
			else if (0x55 == p[i])
			{
				pGlobalHardwareInfo->buffFromMCUProcessorStatus = 2;
			}
			else
			{
				pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
			}
			break;
		case 2:
			pGlobalHardwareInfo->buffFromMCUProcessorStatus = 3;
			pGlobalHardwareInfo->buffFromMCUFrameLength = 0;
			pGlobalHardwareInfo->buffFromMCUFrameLengthMax = p[i];
			pGlobalHardwareInfo->buffFromMCUCRC = p[i];
			break;
		case 3:
			if (pGlobalHardwareInfo->buffFromMCUFrameLength < (pGlobalHardwareInfo->buffFromMCUFrameLengthMax - 1))
			{
				pGlobalHardwareInfo->buffFromMCU[pGlobalHardwareInfo->buffFromMCUFrameLength] = p[i];
				pGlobalHardwareInfo->buffFromMCUCRC += p[i];
				pGlobalHardwareInfo->buffFromMCUFrameLength++;
			}
			else
			{
				pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
				if (pGlobalHardwareInfo->buffFromMCUCRC == p[i])
				{
					LPCdealReadFromMCUAll(pGlobalHardwareInfo->buffFromMCU,pGlobalHardwareInfo->buffFromMCUFrameLengthMax-1);
				}
				else
				{
					DBG0("\nRead From MCU CRC Error");
				}
			}
			break;
		default:
			pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
			break;
		}
	}

	if (pGlobalHardwareInfo->buffFromMCUProcessorStatus > 1)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL actualReadFromMCU(BYTE *p,UINT length)
{
	UINT i;
	actualIICReadSimple(I2_0_ID,MCU_ADDR_R,p,length);

	DBG("\nIIC Read:");

	for (i=0; i<length; i++)
		DBG(" %x",p[i]);

	DBG("\n");

	if (readFromMCUProcessor(p,length))
	{
		DBG0(" More");
		return TRUE;
	}
	else	
	{
		return FALSE;
	}
}

//MCU的IIC读处理
irqreturn_t MCUIIC_isr(int irq, void *dev_id)
{
	schedule_work(&pGlobalHardwareInfo->FlyIICInfo.iic_work);
	return IRQ_HANDLED;
}

static void workFlyMCUIIC(struct work_struct *work)
{
	BYTE buff[256];

	DBG("ThreadFlyMCUIIC running\n");
	while (!SOC_IO_Input(MCU_IIC_REQ_G,MCU_IIC_REQ_I,0)) 
	{
		actualReadFromMCU(buff,16);
	}

	pGlobalHardwareInfo->FlyIICInfo.time_out = GetTickCount();
}



void mcuFirstInit(void)
{
	INIT_WORK(&pGlobalHardwareInfo->FlyIICInfo.iic_work, workFlyMCUIIC);

	SOC_IO_Input(MCU_IIC_REQ_G,MCU_IIC_REQ_I,0);
	SOC_IO_ISR_Add(MCU_IIC_REQ_ISR,IRQ_TYPE_EDGE_FALLING,MCUIIC_isr,pGlobalHardwareInfo);
	SOC_IO_ISR_Enable(MCU_IIC_REQ_ISR);
}
