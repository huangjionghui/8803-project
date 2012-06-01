#include "BCSP.h"
#include "DFU.h"
#include "BCCMD.h"
#include "DFU_WorkFlow.h"

U16 g_timesBCSPTx = 0;
U16 g_timesBCSPRx = 0;
U8	g_rxStartSlip=0;
U8	g_rxEndSlip=0;
U8 last_SeqNo=7;

U8 isPACKETHealth=0;


BCSPPacket g_RxPacket;
BCSPPacket g_AckPacket;
BCSPPacket g_TxPacket;

U8	g_swithTx=0;

U8 BCSPFromRawSLIP(U8 *s, U8 start, U8 end)
{
	U8 i;
	U8 BytesPureBCSP =0;
	U8 meetESC = 0;

	for (i=start; i!=end; i++)
	{
		if (s[i] == SLIP_END)	// must be the start or End byte; don't save that to pureBCSPbuf
			continue;
		else if (meetESC)
		{
			meetESC = 0;
			if (s[i] == SLIP_ESC_END)
				g_RxPacket.PureBCSPBuf[BytesPureBCSP++] = SLIP_END;
			else if (s[i] == SLIP_ESC_ESC)
				g_RxPacket.PureBCSPBuf[BytesPureBCSP++] = SLIP_ESC;
		}
		else if (s[i] == SLIP_ESC)
		{
			meetESC = 1;
			continue;
		}
		else
		{
			g_RxPacket.PureBCSPBuf[BytesPureBCSP++] = s[i];
		}

		if (i == 0)
		{
			//printf("=== RawBuf[0]:%c, char:%c\r\n", s[i]/*,g_RxPacket.PureBCSPBuf[BytesPureBCSP-1]*/);
			//printf("");
		}
	}
	// end byte:  0xc0
	g_RxPacket.BytesPureBCSP = BytesPureBCSP;

	g_RxPacket.seq 			= GET_BITS(g_RxPacket.header[0], 0, 3);
	g_RxPacket.ack			= GET_BITS(g_RxPacket.header[0], 3, 3);
	g_RxPacket.crcPresent		= GET_BITS(g_RxPacket.header[0], 6, 1);
	g_RxPacket.protocolType	= GET_BITS(g_RxPacket.header[0], 7, 1);

	g_RxPacket.protocolID		= GET_BITS(g_RxPacket.header[1], 0, 4);
	
	g_RxPacket.payloadLen	= g_RxPacket.header[2];
	g_RxPacket.payloadLen	= ((g_RxPacket.payloadLen << 4) & 0x0ff0)  | GET_BITS(g_RxPacket.header[1], 4, 4);

	g_RxPacket.checksum 	= g_RxPacket.header[3];

	if (g_RxPacket.crcPresent)
	{
		if ( (g_RxPacket.payloadLen+4+2 /*4(Header) + 2(CRC)*/) != g_RxPacket.BytesPureBCSP)
		{
			return 0;
		}
	}
	else
	{
		if ( (g_RxPacket.payloadLen+4 /*4:Header*/) != g_RxPacket.BytesPureBCSP)
		{
			return 0;
		}
	}

	return 1;
}


static void setBCSPHeader(BCSPPacket *pkt)
{
	pkt->PureBCSPBuf[0] = pkt->seq + (SET_BITS(pkt->ack, 3)) + (SET_BITS(pkt->crcPresent, 6)) + (SET_BITS(pkt->protocolType, 7));
#ifdef CRC
	pkt->PureBCSPBuf[0] |= 0x40;
#endif
	pkt->PureBCSPBuf[1] = pkt->protocolID+ (( (pkt->payloadLen) & 0x000f)<<4);
	pkt->PureBCSPBuf[2] = pkt->payloadLen>>4;
	pkt->checksum	      = 0xff - (pkt->PureBCSPBuf[0] + pkt->PureBCSPBuf[1] + pkt->PureBCSPBuf[2]);
	pkt->PureBCSPBuf[3] = pkt->checksum;
}

#ifdef CRC

static U16 ubcsp_calc_crc (U8 ch, U16 crc)
{
	/* Calculate the CRC using the above 16 entry lookup table */

	U16 crc_table[16] = {0x0000, 0x1081, 0x2102, 0x3183,0x4204, 0x5285, 0x6306, 0x7387,0x8408, 0x9489, 0xa50a, 0xb58b,0xc60c, 0xd68d, 0xe70e, 0xf78f};

	/* Do this four bits at a time - more code, less space */
	crc = (crc >> 4) ^ crc_table[(crc ^ ch) & 0x000f];
	crc = (crc >> 4) ^ crc_table[(crc ^ (ch >> 4)) & 0x000f];

	return crc;
}

static U16 ubcsp_crc_reverse (U16 crc)
{
	U32 b, rev;

	/* Reserse the bits to compute the actual CRC value */
	for (b = 0, rev=0; b < 16; b++)
	{
		rev = rev << 1;
		rev |= (crc & 1);
		crc = crc >> 1;
	}

	return rev;
}

#endif

static void BCSP2SLIP(BCSPPacket *pkt)
{
	U16 BytesPureBCSP =0;

#ifdef CRC
	pkt->crc = 0xFFFF;
#endif

	PutChar(SLIP_END);
	
#ifdef CRC
	for (BytesPureBCSP=0; BytesPureBCSP<(pkt->payloadLen+6); BytesPureBCSP++)
#else
	for (BytesPureBCSP=0; BytesPureBCSP<(pkt->payloadLen+4); BytesPureBCSP++)
#endif
	{
		if (pkt->PureBCSPBuf[BytesPureBCSP] == SLIP_END)
		{
			PutChar(SLIP_ESC);
			PutChar(SLIP_ESC_END);
		}
		else if (pkt->PureBCSPBuf[BytesPureBCSP] == SLIP_ESC)
		{
			PutChar(SLIP_ESC);
			PutChar(SLIP_ESC_ESC);
		}
		else
		{
			PutChar(pkt->PureBCSPBuf[BytesPureBCSP]);
		}

#ifdef CRC
		if (BytesPureBCSP < pkt->payloadLen+4)
		{
			pkt->crc = ubcsp_calc_crc(pkt->PureBCSPBuf[BytesPureBCSP], pkt->crc);

			if (BytesPureBCSP == (pkt->payloadLen+3))
			{
				pkt->crc = ubcsp_crc_reverse (pkt->crc);

				pkt->PureBCSPBuf[pkt->payloadLen+4] = (U8)(pkt->crc >> 8);
				pkt->PureBCSPBuf[pkt->payloadLen+5] = (U8)(pkt->crc & 0x00ff);
			}
		}
#endif
	}

	PutChar(SLIP_END);

	pkt->BytesPureBCSP 	= BytesPureBCSP;
}

static void sendAck(U8 ack_no)
{
	g_AckPacket.seq = 0;
	g_AckPacket.ack = ack_no;
	g_AckPacket.crcPresent= 0;
	g_AckPacket.protocolType= 0;
	g_AckPacket.protocolID = PROTOCOL_NONE;
	g_AckPacket.payloadLen = 0;

	setBCSPHeader(&g_AckPacket);
	BCSP2SLIP(&g_AckPacket);
	
	//OSTimeDly(10);
	g_timesBCSPTx++;
}

void sendBCSPPacket(BCSPPacket *pkt, U8 IsReSend)
{
	if(IsReSend)
		pkt->seq--;
	setBCSPHeader(pkt);
	pkt->seq++;

	if (pkt->protocolType == 1)
		curDFU.bAckPending =1;

	BCSP2SLIP(pkt);

	
	//OSTimeDly(((pkt->payloadLen%10)+1)*1);
	//OSTimeDly(10);
}


void BCSP_LE_send_SYNC_RESP(void)
{
	g_TxPacket.seq = 0;
	g_TxPacket.ack = 0;
	g_TxPacket.crcPresent= 0;
	g_TxPacket.protocolType= 0;
	g_TxPacket.protocolID = PROTOCOL_BCSP_LE;
	g_TxPacket.payloadLen = 4;
	
	setBCSPHeader(&g_TxPacket);
	
	g_TxPacket.PureBCSPBuf[4] = 0xac;
	g_TxPacket.PureBCSPBuf[5] = 0xaf;
	g_TxPacket.PureBCSPBuf[6] = 0xef;
	g_TxPacket.PureBCSPBuf[7] = 0xee;
	
	BCSP2SLIP(&g_TxPacket);
	//OSTimeDly(10);
	
	g_timesBCSPTx++;
}

void BCSP_LE_send_CONF_RESP(void)
{
	g_TxPacket.seq = 0;
	g_TxPacket.ack = 0;
	g_TxPacket.crcPresent= 0;
	g_TxPacket.protocolType= 0;
	g_TxPacket.protocolID = PROTOCOL_BCSP_LE;
	g_TxPacket.payloadLen = 4;
	
	setBCSPHeader(&g_TxPacket);
	
	g_TxPacket.PureBCSPBuf[4] = 0xde;
	g_TxPacket.PureBCSPBuf[5] = 0xad;
	g_TxPacket.PureBCSPBuf[6] = 0xd0;
	g_TxPacket.PureBCSPBuf[7] = 0xd0;
	
	BCSP2SLIP(&g_TxPacket);

	//OSTimeDly(10);
	g_timesBCSPTx++;
}

/*
static U8 IsBCSPHeaderHealth(BCSPPacket *pkt)
{
	if ( (pkt->PureBCSPBuf[0] + pkt->PureBCSPBuf[1] + pkt->PureBCSPBuf[2] + pkt->PureBCSPBuf[3])%256 == 0xff)
		return 1;
	else
		return 0;
}
*/

void HandleBCSPPacket(void)
{
	g_timesBCSPRx++;

	isPACKETHealth = 1;

	if (g_RxPacket.protocolType)
	{
#ifdef DFU_DEBUG
		printf("==> Last SeqNo:%d,rec_Seq:%d\n", last_SeqNo, g_RxPacket.seq);
#endif

		sendAck((g_RxPacket.seq+1)%8);

		if (last_SeqNo == g_RxPacket.seq)
		{
#ifdef DFU_DEBUG
			printf("P: %d\n", g_RxPacket.protocolID);
			//printf("\t\t===Resend Packet~~Just S_ACK\n");
			printf("\t\t===Resend Packet~~Don't care\n");
#endif
			return;
		}

#ifdef DFU_DEBUG
		printf("P: %d\n", g_RxPacket.protocolID);
		printf("\tS_ACK\n");
#endif
		last_SeqNo = g_RxPacket.seq;
		//OSTimeDly(10);
	}

	if ((g_RxPacket.ack != g_TxPacket.seq)
#ifndef IGNORE_RX_LOSE
		&& (g_RxPacket.protocolID!=PROTOCOL_BCSP_LE) 
#endif
		)
	{
#ifdef DFU_DEBUG
		printf("\t\tRx ack:%d not match Tx:seq:%d, Rx Ch:%d\n", g_RxPacket.ack, g_TxPacket.seq, g_RxPacket.protocolID);
		curDFU.ErrCode = 1;
		curDFU.AdvErrCode = 0x888;
#endif
		return;
	}

	switch (g_RxPacket.protocolID)
	{
		case PROTOCOL_NONE:
			// this must be ack packet
			//curDFU.curEvent = UPGRADE_EVENT_BCSP_ACK_RECIEVED;
			//break;
#ifdef DFU_DEBUG
			printf("\tRec_ACK\n");
#endif

			curDFU.bAckPending =0;
			curDFU.generalTimer = 0;	// cancel the timeout;	

			return;

		case PROTOCOL_BCSP_LE:
			if (g_RxPacket.payload[0]==0xda && g_RxPacket.payload[1]==0xdc && 
				g_RxPacket.payload[2]==0xed && g_RxPacket.payload[3]==0xed)
			{
				curDFU.curEvent = UPGRADE_EVENT_BCSP_LE_SYNC_RECIEVED;
			}
			else if (g_RxPacket.payload[0]==0xad && g_RxPacket.payload[1]==0xef && 
				g_RxPacket.payload[2]==0xac && g_RxPacket.payload[3]==0xed)
			{
				curDFU.curEvent = UPGRADE_EVENT_BCSP_LE_CONF_RECIEVED;

				last_SeqNo=7;
			}
#ifdef IGNORE_RX_LOSE
			else
			{
				if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE 
					|| curDFU.curStage == UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE
					|| curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE
					|| curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK
					|| curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE
				)
				{
					curDFU.curEvent = UPGRADE_EVENT_BCSP_LE_SYNC_RECIEVED;
				}
				else
					curDFU.curEvent = UPGRADE_EVENT_BCSP_LE_CONF_RECIEVED;
			}
#endif
			break;

		case PROTOCOL_BCCMD:
			curDFU.generalTimer = 0;

			if (curDFU.ExpectResponseProtocol == PROTOCOL_BCCMD)
			{
				curDFU.ExpectResponseProtocol = PROTOCOL_NONE;
			}
			else
			{
				#ifdef DFU_DEBUG
					printf("This is a interesting situation 1\n");
				#endif
			}

			handleBCCMDResponse(g_RxPacket.payload, g_RxPacket.payloadLen);
			break;
		case PROTOCOL_DFU:
			if (curDFU.curStage != UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC && curDFU.curStage != UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC)
			{
				curDFU.generalTimer = 0;
			}

			if (curDFU.ExpectResponseProtocol == PROTOCOL_DFU)
			{
				curDFU.ExpectResponseProtocol = PROTOCOL_NONE;
			}
			else
			{
				#ifdef DFU_DEBUG
					printf("This is a interesting situation 2\n");
				#endif
			}

			handleDFUResponse(g_RxPacket.payload, g_RxPacket.payloadLen);
			break;

		default:
			return;
	}

	DFU_WorkFlow();
}

#ifdef IGNORE_RX_LOSE
void HandleIncompleteBCSPPacket(void)
{
	g_timesBCSPRx++;

	isPACKETHealth = 0;
	//OutPutDebugMSG_Packet(0);

/*
	if (IsBCSPHeaderHealth(&g_RxPacket) )
	{

		if (g_RxPacket.ack != g_TxPacket.seq)
		{
			OutPutDebugMSG_Ack(2);
			g_TxPacket.seq++;
		}
		else
			OutPutDebugMSG_Ack(0);
	}
*/		
/*
	if (g_RxPacket.protocolID == 0 && curDFU.bAckPending)
	{
		g_TxPacket.seq++;
		curDFU.generalTimer = 0;	// cancel the timeout;	
		curDFU.bAckPending =0;
	}
*/
	if (g_RxPacket.protocolType)
	{
		if (curDFU.ExpectResponseProtocol == PROTOCOL_DFU && curDFU.curCmd == cmd_DFU_GETFUNCT)
		{
			// Don't ack, and don't follow up the flow, let the BCore resend again
			//OutPutDebugMSG((const U8 *)"REGET FUNCTION");
			return;
		}
		
		sendAck((g_RxPacket.seq+1)%8);
		if (last_SeqNo == g_RxPacket.seq)
		{
			return;
		}
		last_SeqNo = g_RxPacket.seq;
	}
	else if (curDFU.bAckPending == 0 && (curDFU.ExpectResponseProtocol == PROTOCOL_DFU || curDFU.ExpectResponseProtocol == PROTOCOL_BCCMD))
	{
		sendAck((last_SeqNo+1)%8);
		last_SeqNo++;
	}
		

	/*
	    Two Packet combie to one packet...
	    
			EX:  C0 70 00 00 8F D9 A4 C0 
			       C0 F6 4C 00 BD 00 00 0 00 53 6E C0

			Combie to :
				C0 70 00 00 4C 00 BD 00 00 0 00 53 6E C0
	*/
	if (curDFU.ExpectResponseProtocol == PROTOCOL_DFU && 
		(g_RxPacket.protocolID == PROTOCOL_DFU || 
		(g_RxPacket.protocolID == PROTOCOL_NONE && (g_rxEndSlip-g_rxStartSlip)>7) /*2 packet*/ ||
		(curDFU.bAckPending == 0 && g_RxPacket.protocolID != PROTOCOL_DFU ) /*lose 1st header byte */ ))
	{
		switch (curDFU.curCmd)
		{
			case cmd_DFU_DETACH:
				curDFU.curEvent = UPGRADE_EVENT_DFU_DETACH_RESP;
				break;

			case cmd_DFU_DNLOAD:
					curDFU.curEvent = UPGRADE_EVENT_DFU_DOWNLOAD_RESP;
				break;

			case cmd_DFU_GETSTATUS:
				if (g_RxPacket.ack != g_TxPacket.seq)
					return;
				
				// Don't Get Polling Timeout, because we can't sure is it correct?
				//make the event to move on
				if (curDFU.curStage <= UPGRADE_STAGE_DFUMODE_READY_DOWNLOAD || curDFU.curStage > UPGRADE_STAGE_DFUMODE_PURE_DATA_DOWNLOAD_FINISH)
					curDFU.curEvent = UPGRADE_EVENT_DFU_GETSTATUS_RESP_STATE_dfuIDLE;
				else 
					curDFU.curEvent = UPGRADE_EVENT_DFU_GETSTATUS_RESP_STATE_dfuDNLOAD_IDLE;

				// make a fake and save Polling Timeout...
				curDFU.PollTimeout = 30;

				break;
			
			case cmd_DFU_RESET:
				curDFU.curEvent = UPGRADE_EVENT_DFU_RESET_RESP;
				break;

			case cmd_DFU_GETDEVICE:
				curDFU.curEvent = UPGRADE_EVENT_DFU_GETDEVICE_RESP;
				break;

			case cmd_DFU_GETFUNCT:
				// check this value, not ever platform can accept this value;
				curDFU.TransferSizePerBlock = curDFU.DFU_MAX_TRANSFER_SIZE-1;
				//OutPutDebugMSG_TransferLength();
				curDFU.curEvent = UPGRADE_EVENT_DFU_GETFUNCTION_RESP;
				break;

			default:
				return;
		}

		curDFU.generalTimer = 0;	// cancel the timeout;
		curDFU.ExpectResponseProtocol = PROTOCOL_NONE;
		DFU_WorkFlow();
		
	}
	else if (curDFU.ExpectResponseProtocol == PROTOCOL_BCCMD && 
		(g_RxPacket.protocolID == PROTOCOL_BCCMD || (g_RxPacket.protocolID == PROTOCOL_NONE && (g_rxEndSlip-g_rxStartSlip)>7) /*2 packet*/)
		)
	{
		if (g_RxPacket.ack != g_TxPacket.seq)
			return;


		if (curDFU.curStage == UPGRADE_STAGE_MODIFY_INITIAL_BOOTMODE)
			curDFU.curEvent = UPGRADE_EVENT_MODIFY_INITIAL_BOOTMODE_OK;
		else if (curDFU.curStage == UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE)
			curDFU.curEvent = UPGRADE_EVENT_RESTORE_INITIAL_BOOTMODE_OK;
		else if (curDFU.curStage == UPGRADE_STAGE_MODIFY_BAUDRATE)
			curDFU.curEvent = UPGRADE_EVENT_MODIFY_BAUDRATE_OK;
		else if (curDFU.curStage == UPGRADE_STAGE_RESTORE_BAUDRATE)
			curDFU.curEvent = UPGRADE_EVENT_RESTORE_BAUDRATE_OK;

		curDFU.generalTimer = 0;	// cancel the timeout;
		curDFU.ExpectResponseProtocol = PROTOCOL_NONE;
		DFU_WorkFlow();
	}
	else if (g_RxPacket.protocolID == PROTOCOL_BCSP_LE)
	{
		if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE 
			|| curDFU.curStage == UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE
			|| curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE
#ifdef IGNORE_RX_LOSE
			|| curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK
			|| curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE
#endif
		)
		{
			curDFU.curEvent = UPGRADE_EVENT_BCSP_LE_SYNC_RECIEVED;
		}
		else
			curDFU.curEvent = UPGRADE_EVENT_BCSP_LE_CONF_RECIEVED;
			
		curDFU.generalTimer = 0;	// cancel the timeout;
		curDFU.ExpectResponseProtocol = PROTOCOL_NONE;
		DFU_WorkFlow();
	}
	else if (g_RxPacket.protocolID == 0 && curDFU.bAckPending)
	{
		g_TxPacket.seq++;
		curDFU.generalTimer = 0;	// cancel the timeout;	
		curDFU.bAckPending =0;
	}
}
#endif

