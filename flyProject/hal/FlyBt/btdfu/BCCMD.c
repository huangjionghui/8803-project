#include "BCCMD.h"
#include "DFU.h"
#include "BCSP.h"
#include "DFU_WorkFlow.h"

#define BCCMD_Timeout			1000

U16 LAST_BCCMD_TYPE = 0;


void init_BCCMD(void)
{
	LAST_BCCMD_TYPE = 0;
}

static void sendBCCMD(U16 totalLen, U16 ExpectRespTimeoutMS)
{
//	if (curDFU.bAckPending == 0)
	{
		// if this Packet it resend packet, Don't Change the Seq_No
//		g_TxPacket.seq++;
	}
		
	g_TxPacket.ack = g_RxPacket.seq + 1;
	g_TxPacket.crcPresent= 0;
	g_TxPacket.protocolType= 1;
	g_TxPacket.protocolID = PROTOCOL_BCCMD;
	g_TxPacket.payloadLen = totalLen;

	sendBCSPPacket(&g_TxPacket, 0);
	curDFU.generalTimer = ExpectRespTimeoutMS/SYSTEM_POLLING_INTERVAL_BY_MS;
	//OutPutDebugMSG_SetTimer();
	curDFU.ExpectResponseProtocol = PROTOCOL_BCCMD;
}

static void Fill_BCCMD_Header(U16 type, U16 length, U16 VarID)
{
	BCCMD_RESP_packet *pkt = (BCCMD_RESP_packet *) g_TxPacket.payload;
	
	pkt->type 	= type;
	pkt->length	= length;
	pkt->seqno	= g_TxPacket.seq;
	pkt->varID	= VarID;
	pkt->status	= 0;	

	LAST_BCCMD_TYPE = type;
}

void BCCMD_BASIC_CMD(U16 VarID, U16 *payload, U16 payloadLen)
{
	U16 length = 0;
	U16 *temp=0;
	U16 i=0;

	length = (payloadLen+5)>9 ? payloadLen+5 : 9;
	Fill_BCCMD_Header(BCCMD_TYPE_SETREQ, length, VarID);

	temp = (U16 *)(g_TxPacket.payload + 10);

	for (i=0; i<length-5; i++)
	{
		if (i<4 && i>=payloadLen)
			temp[i] = 0x0000;
		else
			temp[i] = payload[i];
	}

	if (VarID == VarID_Cold_Reset)
		sendBCCMD(length*2, 0);
	else
		sendBCCMD(length*2, BCCMD_Timeout);
	
}


void BCCMDPS_GET_KEY_Value(U16 key, U16 ValLen)
{
	U16 i=0;
	BCCMDPS_payload *ps = (BCCMDPS_payload *)(g_TxPacket.payload + 10);
	
	Fill_BCCMD_Header(BCCMD_TYPE_GETREQ, 8+ValLen, VarID_PS_ValueReadWrite);

	ps->key = key;
	ps->length = ValLen;
	ps->store = PS_STORE_DEFAULT;

	if (ValLen> 50)
	{
		curDFU.ErrCode = err_BCCMD_PSKEY_VALUE_TOO_BIG_TO_BUFFER;
		return;
	}
	
	for (i=0; i<ValLen; i++)
	{
		ps->value[i] = 0x0000;
	}

	sendBCCMD(2*(8+ValLen), BCCMD_Timeout);
}


void BCCMDPS_GET_KEY_Length(U16 key)
{
	BCCMDPS_payload *ps = (BCCMDPS_payload *)(g_TxPacket.payload + 10);

	Fill_BCCMD_Header(BCCMD_TYPE_GETREQ, 9, VarID_PS_ValueSize);
	
	ps->key = key;
	ps->length = 0;
	ps->store = PS_STORE_DEFAULT;
	ps->value[0] = 0x0000;

	sendBCCMD(18, BCCMD_Timeout);
}

void BCCMDPS_SET_KEY_Value(U16 key, U16 *Value, U16 ValLen)
{
	U16 i=0;
	BCCMDPS_payload *ps = (BCCMDPS_payload *)(g_TxPacket.payload + 10);
	
	Fill_BCCMD_Header(BCCMD_TYPE_SETREQ, 8 + ValLen, VarID_PS_ValueReadWrite);

	ps->key = key;
	ps->length = ValLen;
	ps->store = PS_STORE_DEFAULT;

	if (ValLen > 50)
	{
		curDFU.ErrCode = err_BCCMD_PSKEY_VALUE_TOO_BIG_TO_BUFFER;
		return;
	}
	
	for (i=0; i<ValLen; i++)
	{
		ps->value[i] = Value[i];
	}

	sendBCCMD(2*(8+ValLen), BCCMD_Timeout);
}

static void handleBCCMDResPSValueReadWrite(BCCMD_RESP_packet *pkt)
{
	BCCMDPS_payload *ps = (BCCMDPS_payload *)(g_RxPacket.payload + 10);
	pkt = pkt;

 	if (LAST_BCCMD_TYPE == BCCMD_TYPE_SETREQ)
	{
		if (ps->key == PSKEY_INITIAL_BOOTMODE)
		{
			if (curDFU.curStage == UPGRADE_STAGE_MODIFY_INITIAL_BOOTMODE)
				curDFU.curEvent = UPGRADE_EVENT_MODIFY_INITIAL_BOOTMODE_OK;
			else if (curDFU.curStage == UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE)
				curDFU.curEvent = UPGRADE_EVENT_RESTORE_INITIAL_BOOTMODE_OK;
			else 
			{
				curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 1;
				curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
			}
		}
		else if (ps->key == PSKEY_UART_BAUDRATE)
		{
			if (curDFU.curStage == UPGRADE_STAGE_MODIFY_BAUDRATE)
				curDFU.curEvent = UPGRADE_EVENT_MODIFY_BAUDRATE_OK;
			else if (curDFU.curStage == UPGRADE_STAGE_RESTORE_BAUDRATE)
				curDFU.curEvent = UPGRADE_EVENT_RESTORE_BAUDRATE_OK;
			else 
			{
				curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 2;
				curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
			}
		}
		else if (ps->key == PSKEY_VM_DISABLE)
		{
			if (curDFU.curStage == UPGRADE_STAGE_MODIFY_VM_DISABLE)
				curDFU.curEvent = UPGRADE_EVENT_MODIFY_VM_DISABLE_OK;
			else if (curDFU.curStage == UPGRADE_STAGE_RESTORE_VM_DISABLE)
				curDFU.curEvent = UPGRADE_EVENT_RESTORE_VM_DISABLE_OK;
			else 
			{
				curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 9;
				curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
			}
		}
		else
		{
			curDFU.curEvent = UPGRADE_EVENT_ERROR;
			curDFU.AdvErrCode = 3;
			curDFU.ErrCode = err_BCCMD_SET_PSKEY_VALUE_NOT_THIS_PSKEY;
		}
	}
	else if (LAST_BCCMD_TYPE == BCCMD_TYPE_GETREQ)
	{
	}
	else
	{
		curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 4;
		curDFU.ErrCode = err_BCCMD_MISS_LAST_TYPE;
	}
		
}


static void handleBCCMDResPSLength(BCCMD_RESP_packet *pkt)
{
	//BCCMDPS_payload *ps = (BCCMDPS_payload *)(g_RxPacket.payload + 10);
	pkt = pkt;
}

void handleBCCMDResponse(U8 *BCCMDpayload, U16 len)
{
	BCCMD_RESP_packet *pkt = (BCCMD_RESP_packet *)BCCMDpayload;
	len = len;

	if (pkt->type != BCCMD_TYPE_GETRESP)
	{
		curDFU.ErrCode = err_BCCMD_ERROR_TYPE;

		return;
	}
	else if (pkt->status != BCCMD_STATUS_OK)
	{
		curDFU.ErrCode = err_BCCMD_BASE+pkt->status;
		return;
	}

	switch (pkt->varID)
	{
		case VarID_PS_ValueReadWrite:
#ifdef DFU_DEBUG
				printf("\tRePS_RW\n");
#endif

			handleBCCMDResPSValueReadWrite(pkt);
			break;

		case VarID_PS_ValueSize:
#ifdef DFU_DEBUG
			printf("\tRePS_len\n");
#endif
			handleBCCMDResPSLength(pkt);
			break;

		case VarID_Cold_Reset:
			//handleBCCMDRes
			break;
	}
}

