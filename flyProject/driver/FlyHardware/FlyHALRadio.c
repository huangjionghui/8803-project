#include "FlyInclude.h"

UINT noBlockRadioMessage(BYTE *pData,UINT length)
{
	UINT regAddr;
	UINT returnCount = 0;
	if (MSG_RADIO_REQ_TDA7541_SSTOP_ID == pData[0])
	{
		if (SOC_IO_Input(RADIO_SSTOP_GROUP, RADIO_SSTOP_GPIO,1))
		{
			pData[1] = 0x01;
		}
		else
		{
			pData[1] = 0x00;
		}
		return 2;
	}
	else if (MSG_RADIO_CON_AD_GET == pData[0])
	{
		pGlobalHardwareInfo->iRadioADReadTime = GetTickCount();
		pGlobalHardwareInfo->bRadioADRead = TRUE;
		systemReqRadioAD();
		wait_for_completion(&pGlobalHardwareInfo->compRadioADRead);
		pData[1] = pGlobalHardwareInfo->iRadioADValue >> 8;
		pData[2] = pGlobalHardwareInfo->iRadioADValue;
		return 3;
	}
	else if (MSG_RADIO_CON_SAF7741_ID == pData[0])
	{
		regAddr = (pData[1] << 16) + (pData[2] << 8) + pData[3];
		actualIICReadSAF7741(I2_1_ID, SAF7741_ADDR_W, regAddr, &pData[4], length-4);
		//printk("\nSAF7741 read-->");
		//for (i = 0; i < length; i++)
		//{
		//	printk(" 0x%2X",pData[i]);
		//}
		return length;
	}
	else if (MSG_RADIO_CON_TEF7000_ID1 == pData[0])
	{
		actualIICReadTEF7000(I2_1_ID, TEF7000_1_ADDR_W, pData[1], &pData[2], length-2);
		return length;
	}
	else if (MSG_RADIO_CON_TEF7000_ID2 == pData[0])
	{
		actualIICReadTEF7000(I2_1_ID, TEF7000_2_ADDR_W, pData[1], &pData[2], length-2);
		return length;
	}
	return returnCount;
}

void dealRadioCommand(BYTE *buf, UINT len)
{
	switch (buf[0])
	{
	case MSG_RADIO_CON_ANT1:
		if (GlobalShareMmapInfo.pShareMemoryCommonData->bControlRadioANT)
		{
			ioControlRadioANT(buf[1]);
		}
		else
		{
			ioControlRadioANT(0);
		}
		break;
	case MSG_RADIO_CON_ANT2:
		//ANT2
		break;
	case MSG_RADIO_TDA7541_AFMUTE:
		ioControlRadioAFMUTE(buf[1]);
		break;
	case MSG_RADIO_CON_TDA7541_ID:
		actualIICWrite(I2_1_ID,TDA7541_ADDR_W,&buf[1],len-1);
		break;
	case MSG_RADIO_CON_SAF7741_ID:
		actualIICWrite(I2_1_ID,SAF7741_ADDR_W,&buf[1],len-1);
		break;
	case MSG_RADIO_CON_TEF7000_ID1:
		actualIICWriteTEF7000(I2_1_ID,TEF7000_1_ADDR_W,buf[1],&buf[1],len-1);
		break;
	case MSG_RADIO_CON_TEF7000_ID2:
		actualIICWriteTEF7000(I2_1_ID,TEF7000_2_ADDR_W,buf[1],&buf[1],len-1);
		break;
	default:
		break;
	}
}


void halRadioFirstInit(void)
{
	//收音头读AD值特殊处理
	init_completion(&pGlobalHardwareInfo->compRadioADRead);
}
