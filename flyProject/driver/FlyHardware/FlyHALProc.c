#include "FlyInclude.h"

void messageToRadioHAL(BYTE *p,UINT length)
{
	if (length > RADIO_MESSAGE_BUFF_LENGTH - 1)
	{
		DBG0("\nRADIO_MESSAGE_BUFF_LENGTH");
		return;
	}
	down(&pGlobalHardwareInfo->semRadioMessage);
	pGlobalHardwareInfo->radioMessageBuff[pGlobalHardwareInfo->radioMessageBuffHx][0] = length;
	memcpy(&pGlobalHardwareInfo->radioMessageBuff[pGlobalHardwareInfo->radioMessageBuffHx][1],p,length);
	pGlobalHardwareInfo->radioMessageBuffHx++;
	if (pGlobalHardwareInfo->radioMessageBuffHx >= RADIO_MESSAGE_BUFF_SIZE)
	{
		pGlobalHardwareInfo->radioMessageBuffHx = 0;
	}
	up(&pGlobalHardwareInfo->semRadioMessage);
	wake_up_interruptible(&pGlobalHardwareInfo->read_wait);
}

void messageMCUToSystemHAL(BYTE *p,UINT length)
{
	if (length > SYSTEM_MESSAGE_BUFF_LENGTH - 2)
	{
		DBG0("\nSYSTEM_MESSAGE_BUFF_LENGTH");
		return;
	}
	down(&pGlobalHardwareInfo->semSystemMessage);
	pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffHx][0] = length+1;
	pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffHx][1] = MSG_SYSTEM_TRANS_MCU;
	memcpy(&pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffHx][2],p,length);
	pGlobalHardwareInfo->systemMessageBuffHx++;
	if (pGlobalHardwareInfo->systemMessageBuffHx >= SYSTEM_MESSAGE_BUFF_SIZE)
	{
		pGlobalHardwareInfo->systemMessageBuffHx = 0;
	}
	up(&pGlobalHardwareInfo->semSystemMessage);
	wake_up_interruptible(&pGlobalHardwareInfo->read_wait);
}

void messageNormalToSystemHAL(BYTE *p,UINT length)
{
	if (length > SYSTEM_MESSAGE_BUFF_LENGTH - 2)
	{
		DBG0("\nSYSTEM_MESSAGE_BUFF_LENGTH");
		return;
	}
	down(&pGlobalHardwareInfo->semSystemMessage);
	pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffHx][0] = length+1;
	pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffHx][1] = MSG_SYSTEM_TRANS_NORMAL;
	memcpy(&pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffHx][2],p,length);
	pGlobalHardwareInfo->systemMessageBuffHx++;
	if (pGlobalHardwareInfo->systemMessageBuffHx >= SYSTEM_MESSAGE_BUFF_SIZE)
	{
		pGlobalHardwareInfo->systemMessageBuffHx = 0;
	}
	up(&pGlobalHardwareInfo->semSystemMessage);
	wake_up_interruptible(&pGlobalHardwareInfo->read_wait);
}

void messageToKeyHAL(BYTE *p,UINT length)
{
	if (length > KEY_MESSAGE_BUFF_LENGTH - 1)
	{
		DBG0("\nKEY_MESSAGE_BUFF_LENGTH");
		return;
	}
	down(&pGlobalHardwareInfo->semKeyMessage);
	pGlobalHardwareInfo->keyMessageBuff[pGlobalHardwareInfo->keyMessageBuffHx][0] = length;
	memcpy(&pGlobalHardwareInfo->keyMessageBuff[pGlobalHardwareInfo->keyMessageBuffHx][1],p,length);
	pGlobalHardwareInfo->keyMessageBuffHx++;
	if (pGlobalHardwareInfo->keyMessageBuffHx >= KEY_MESSAGE_BUFF_SIZE)
	{
		pGlobalHardwareInfo->keyMessageBuffHx = 0;
	}
	up(&pGlobalHardwareInfo->semKeyMessage);
	wake_up_interruptible(&pGlobalHardwareInfo->read_wait);
}

void messageToServiceHAL(BYTE *p,UINT length)
{
	if (length > SERVICE_MESSAGE_BUFF_LENGTH - 1)
	{
		DBG0("\nSERVICE_MESSAGE_BUFF_LENGTH");
		return;
	}
	down(&pGlobalHardwareInfo->semServiceMessage);
	pGlobalHardwareInfo->serviceMessageBuff[pGlobalHardwareInfo->serviceMessageBuffHx][0] = length;
	memcpy(&pGlobalHardwareInfo->serviceMessageBuff[pGlobalHardwareInfo->serviceMessageBuffHx][1],p,length);
	pGlobalHardwareInfo->serviceMessageBuffHx++;
	if (pGlobalHardwareInfo->serviceMessageBuffHx >= SERVICE_MESSAGE_BUFF_SIZE)
	{
		pGlobalHardwareInfo->serviceMessageBuffHx = 0;
	}
	up(&pGlobalHardwareInfo->semServiceMessage);
	wake_up_interruptible(&pGlobalHardwareInfo->read_wait);
	DBG("qqqq1");
}

UINT32 blockServiceMessage(BYTE *pData,UINT32 len)
{
	UINT32 length = 0;

	down(&pGlobalHardwareInfo->semServiceMessage);
	if (pGlobalHardwareInfo->serviceMessageBuffLx != pGlobalHardwareInfo->serviceMessageBuffHx)
	{
		length = pGlobalHardwareInfo->serviceMessageBuff[pGlobalHardwareInfo->serviceMessageBuffLx][0];
		if (length > 0)
		{
			memcpy(&pData[0]
			,&(pGlobalHardwareInfo->serviceMessageBuff[pGlobalHardwareInfo->serviceMessageBuffLx][1])
				,length);
			pGlobalHardwareInfo->serviceMessageBuffLx++;
			if (pGlobalHardwareInfo->serviceMessageBuffLx >= SERVICE_MESSAGE_BUFF_SIZE)
			{
				pGlobalHardwareInfo->serviceMessageBuffLx = 0;
			}
		}
	}
	up(&pGlobalHardwareInfo->semServiceMessage);
	//printk("blockServiceMessage %d\n",length);
	return length;
}

UINT32 blockKeyMessage(BYTE *pData,UINT32 len)
{
	UINT32 length = 0;

	down(&pGlobalHardwareInfo->semKeyMessage);
	if (pGlobalHardwareInfo->keyMessageBuffLx != pGlobalHardwareInfo->keyMessageBuffHx)
	{
		length = pGlobalHardwareInfo->keyMessageBuff[pGlobalHardwareInfo->keyMessageBuffLx][0];
		if (length > 0)
		{
			memcpy(&pData[0]
			,&(pGlobalHardwareInfo->keyMessageBuff[pGlobalHardwareInfo->keyMessageBuffLx][1])
				,length);
			pGlobalHardwareInfo->keyMessageBuffLx++;
			if (pGlobalHardwareInfo->keyMessageBuffLx >= KEY_MESSAGE_BUFF_SIZE)
			{
				pGlobalHardwareInfo->keyMessageBuffLx = 0;
			}
		}
	}
	up(&pGlobalHardwareInfo->semKeyMessage);
	//printk("blockKeyMessage %d\n",length);
	return length;
}

UINT32 blockSystemMessage(BYTE *pData,UINT32 len)
{
	UINT32 length = 0;

	down(&pGlobalHardwareInfo->semSystemMessage);
	if (pGlobalHardwareInfo->systemMessageBuffLx != pGlobalHardwareInfo->systemMessageBuffHx)
	{
		length = pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffLx][0];
		if (length > 0)
		{
			memcpy(&pData[0]
			,&(pGlobalHardwareInfo->systemMessageBuff[pGlobalHardwareInfo->systemMessageBuffLx][1])
				,length);
			pGlobalHardwareInfo->systemMessageBuffLx++;
			if (pGlobalHardwareInfo->systemMessageBuffLx >= SYSTEM_MESSAGE_BUFF_SIZE)
			{
				pGlobalHardwareInfo->systemMessageBuffLx = 0;
			}
		}
	}
	up(&pGlobalHardwareInfo->semSystemMessage);
	//printk("blockSystemMessage %d\n",length);
	return length;
}

UINT32 blockAcMessage(BYTE *pData,UINT32 len)
{
	UINT32 length = 0;

	down(&pGlobalHardwareInfo->semAcMessage);
	if (pGlobalHardwareInfo->acMessageBuffLx != pGlobalHardwareInfo->acMessageBuffHx)
	{
		length = pGlobalHardwareInfo->acMessageBuff[pGlobalHardwareInfo->acMessageBuffLx][0];
		if (length > 0)
		{
			memcpy(&pData[0]
			,&(pGlobalHardwareInfo->acMessageBuff[pGlobalHardwareInfo->acMessageBuffLx][1])
				,length);
			pGlobalHardwareInfo->acMessageBuffLx++;
			if (pGlobalHardwareInfo->acMessageBuffLx >= AC_MESSAGE_BUFF_SIZE)
			{
				pGlobalHardwareInfo->acMessageBuffLx = 0;
			}
		}
	}
	up(&pGlobalHardwareInfo->semAcMessage);
	//printk("blockAcMessage %d\n",length);
	return length;
}

UINT32 blockRadioMessage(BYTE *pData,UINT32 len)
{
	UINT32 length = 0;

	down(&pGlobalHardwareInfo->semRadioMessage);
	if (pGlobalHardwareInfo->radioMessageBuffLx != pGlobalHardwareInfo->radioMessageBuffHx)
	{
		length = pGlobalHardwareInfo->radioMessageBuff[pGlobalHardwareInfo->radioMessageBuffLx][0];
		if (length > 0)
		{
			memcpy(&pData[0]
			,&(pGlobalHardwareInfo->radioMessageBuff[pGlobalHardwareInfo->radioMessageBuffLx][1])
				,length);
			pGlobalHardwareInfo->radioMessageBuffLx++;
			if (pGlobalHardwareInfo->radioMessageBuffLx >= RADIO_MESSAGE_BUFF_SIZE)
			{
				pGlobalHardwareInfo->radioMessageBuffLx = 0;
			}
		}
	}
	up(&pGlobalHardwareInfo->semRadioMessage);
	//printk("blockRadioMessage %d\n",length);
	return length;
}

UINT blockMessageToHAL(BYTE *pData,UINT length)
{
	UINT returnCount = 0;

	pData[0] = SHARE_MEMORY_COMMON;
	returnCount = blockServiceMessage(&pData[2],length-2);
	if (returnCount)
	{
		returnCount += 2;
		goto readFinish;
	}

	pData[0] = SHARE_MEMORY_KEY;
	returnCount = blockKeyMessage(&pData[2],length-2);
	if (returnCount)
	{
		returnCount += 2;
		goto readFinish;
	}

	pData[0] = SHARE_MEMORY_SYSTEM;
	returnCount = blockSystemMessage(&pData[2],length-2);
	if (returnCount)
	{
		returnCount += 2;
		goto readFinish;
	}

	pData[0] = SHARE_MEMORY_AC;
	returnCount = blockAcMessage(&pData[2],length-2);
	if (returnCount)
	{
		returnCount += 2;
		goto readFinish;
	}

	pData[0] = SHARE_MEMORY_RADIO;
	returnCount = blockRadioMessage(&pData[2],length-2);
	if (returnCount)
	{
		returnCount += 2;
		goto readFinish;
	}

readFinish:return returnCount;
}


UINT32 noBlockMessageToHAL(BYTE currentHAL,BYTE *pData, UINT32 len)
{
	UINT returnCount = 0;
	switch (currentHAL)
	{
	case SHARE_MEMORY_SYSTEM:
		returnCount = noBlockSystemMessage(pData,len);
		break;
	case SHARE_MEMORY_RADIO:
		returnCount = noBlockRadioMessage(pData,len);
		break;
	case SHARE_MEMORY_VIDEO:
		returnCount = noBlockVideoMessage(pData,len);
		break;
	default:
		break;
	}

	return returnCount;
}

void dealDataFromUser(BYTE *buf, UINT len)
{
	switch (buf[0])
	{
	case SHARE_MEMORY_COMMON:
		dealServiceCommand(&buf[1], len-1);
		break;
	case SHARE_MEMORY_DVD://DVD
		dealDVDCommand(&buf[1], len-1);
		break;

	case SHARE_MEMORY_BT://BT
		dealBTCommand(&buf[1], len-1);
		break;

	case SHARE_MEMORY_AUDIO://AUDIO
		dealAudioCommand(&buf[1], len-1);
		break;

	case SHARE_MEMORY_SYSTEM:
		dealSystemCommand(&buf[1], len-1);
		break;

	case SHARE_MEMORY_KEY:
		dealKeyCommand(&buf[1], len-1);
		break;

	case SHARE_MEMORY_RADIO:
		dealRadioCommand(&buf[1], len-1);
		break;

	case SHARE_MEMORY_VIDEO:
		dealVideoCommand(&buf[1], len-1);
		break;
	case SHARE_MEMORY_EXDISPLAY:
		dealExdisplayCommand(&buf[1] ,len-1);
		break;
	default:
		break;
	}
}

void halProcFirstInit(void)
{
	init_waitqueue_head(&pGlobalHardwareInfo->read_wait);

	init_MUTEX(&pGlobalHardwareInfo->semKeyMessage);
	init_MUTEX(&pGlobalHardwareInfo->semRadioMessage);
	init_MUTEX(&pGlobalHardwareInfo->semSystemMessage);
	init_MUTEX(&pGlobalHardwareInfo->semAcMessage);
	init_MUTEX(&pGlobalHardwareInfo->semServiceMessage);

	halRadioFirstInit();
}
