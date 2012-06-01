#include "DFU.h"
#include "BCSP.h"
#include "DFU_WorkFlow.h"

U8 	fEnterDFUMode = 0;

U8 DFU_DETACH[8] 		= {0x21, 0x00, 0x88, 0x13, 0x00, 0x00, 0x00, 0x00};
U8 DFU_DNLOAD[8] 		= {0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//U8 DFU_UPLOAD[8] 		= {0xA1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
U8 DFU_GETSTATUS[8] 	= {0xA1, 0x03, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00};
//U8 DFU_CLRSTATUS[8] 	= {0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//U8 DFU_GETSTATE[8] 	= {0xA1, 0x05, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
U8 DFU_ABORT[8] 		= {0x21, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
U8 DFU_RESET[8] 		= {0x41, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
U8 DFU_GETDEVICE[8] 	= {0xC1, 0x08, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00};
U8 DFU_GETFUNCT[8] 	= {0xC1, 0x09, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00};

void myMemCpy( U8 *src, U8 *dest, U16 n)
{
	for( ; n != 0; n--)
		*dest++ = *src++;
}

void sendDFUcmd(U8 cmd)
{
	U16	requestBytes = 0;
	curDFU.curCmd = cmd;

//	if (curDFU.bAckPending == 0)
	{
		// if this Packet it resend packet, Don't Change the Seq_No
//		g_TxPacket.seq++;
	}

	g_TxPacket.ack = g_RxPacket.seq + 1;
	g_TxPacket.crcPresent= 0;
	g_TxPacket.protocolType= 1;
	g_TxPacket.protocolID = PROTOCOL_DFU;
	g_TxPacket.payloadLen = 8;

	switch (cmd)
	{
		case cmd_DFU_DETACH:
			myMemCpy(DFU_DETACH, g_TxPacket.payload, 8);
			break;
			
		case cmd_DFU_DNLOAD:
			myMemCpy(DFU_DNLOAD, g_TxPacket.payload, 8);

			// set which block being download
			g_TxPacket.payload[2] = curDFU.curBlock & 0x00ff;
			g_TxPacket.payload[3] = (curDFU.curBlock>>8) & 0x00ff;

			if ( ((curDFU.fileSize - curDFU.fileSizeDownloaded) == 16) && 
				(curDFU.curStage == UPGRADE_STAGE_DFUMODE_PURE_DATA_DOWNLOAD_FINISH))
			{
				requestBytes = 0;
			}
			else
			{
				if ( (curDFU.fileSize - curDFU.fileSizeDownloaded - 16) >= curDFU.TransferSizePerBlock)
				{
					requestBytes = curDFU.TransferSizePerBlock;
				}
				else
				{
					requestBytes = curDFU.fileSize - curDFU.fileSizeDownloaded - 16;
				}

				if (requestBytes)
					InsertDFUFileData((U8 *)g_TxPacket.payload+8 , curDFU.fileSizeDownloaded, requestBytes);
				
				g_TxPacket.payload[6] = requestBytes & 0x00ff;
				g_TxPacket.payload[7] = (requestBytes>>8) & 0x00ff;
			}

			// Modify the really payload Length;
			g_TxPacket.payloadLen = 8 + requestBytes;
			curDFU.curBlock++;
			curDFU.fileSizeDownloaded+= requestBytes;
			break;
			
		case cmd_DFU_GETSTATUS:
#ifndef IGNORE_RX_LOSE
			curDFU.PollTimeout = 0;
#endif
			myMemCpy(DFU_GETSTATUS, g_TxPacket.payload, 8);
			break;
			
		case cmd_DFU_ABORT:
			myMemCpy(DFU_ABORT, g_TxPacket.payload, 8);
			break;
			
		case cmd_DFU_RESET:
			myMemCpy(DFU_RESET, g_TxPacket.payload, 8);
			break;
			
		case cmd_DFU_GETDEVICE:
			myMemCpy(DFU_GETDEVICE, g_TxPacket.payload, 8);
			break;
			
		case cmd_DFU_GETFUNCT:
			myMemCpy(DFU_GETFUNCT, g_TxPacket.payload, 8);
			break;
	}

#ifdef DFU_DEBUG
	printf("\t=");
#endif
	sendBCSPPacket(&g_TxPacket, 0);
	
	if (cmd == cmd_DFU_DNLOAD)
	{
		curDFU.generalTimer = 4000/SYSTEM_POLLING_INTERVAL_BY_MS;
		//OutPutDebugMSG_SetTimer();
	}
	else	
	{
		if (curDFU.curStage >= 20)
			curDFU.generalTimer = 2000/SYSTEM_POLLING_INTERVAL_BY_MS;
		else
			curDFU.generalTimer = 750/SYSTEM_POLLING_INTERVAL_BY_MS;
		
		//OutPutDebugMSG_SetTimer();
	}

	curDFU.ExpectResponseProtocol = PROTOCOL_DFU;

#ifdef DFU_DEBUG
	printf("=DFUCmd: %d\n", curDFU.curCmd);
#endif

}


void handleDFUResponse(U8 *DFUpayload, U16 len)
{
	DFU_resp_packet *DFUresp = (DFU_resp_packet *)DFUpayload;
	len = len;

	if (DFUresp->status != 0)
	{
		curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 5;
		curDFU.ErrCode = err_DFU_status_errBase + DFUresp->status;

#ifdef DFU_DEBUG
		printf("\tDFU response Not OK: %d\n", curDFU.ErrCode);
#endif
		return;
	}
#ifdef DFU_DEBUG
		printf("\tReCmd: %d\n", curDFU.curCmd);
#endif

	switch (curDFU.curCmd)
	{
		case cmd_DFU_DETACH:
			curDFU.curEvent = UPGRADE_EVENT_DFU_DETACH_RESP;
			break;
		case cmd_DFU_DNLOAD:
			if (DFUresp->respLength == 0)
				curDFU.curEvent = UPGRADE_EVENT_DFU_DOWNLOAD_RESP;
			break;
		//case cmd_DFU_UPLOAD:
		//	break;
		case cmd_DFU_GETSTATUS:
			curDFU.PollTimeout = (DFUresp->respInfo.Staus_Info.PollTimeout[2]<<16) | (DFUresp->respInfo.Staus_Info.PollTimeout[1]<<8) | DFUresp->respInfo.Staus_Info.PollTimeout[0];

			if (curDFU.PollTimeout > 15000)	// Don't wait so long; Just wait for 300 ms
				curDFU.PollTimeout = 300;

			if (DFUresp->respInfo.Staus_Info.Status != status_OK)
			{
				curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 6;
				curDFU.ErrCode = err_DFU_status_errBase + DFUresp->respInfo.Staus_Info.Status;
				
#ifdef DFU_DEBUG
				printf("\tDFU GetStatus Rsp not OK: %d\n", curDFU.ErrCode);
#endif
				return;
			}
			
			if (DFUresp->respInfo.Staus_Info.State == state_dfuIDLE)
				curDFU.curEvent = UPGRADE_EVENT_DFU_GETSTATUS_RESP_STATE_dfuIDLE;
			else if (DFUresp->respInfo.Staus_Info.State == state_dfuDNLOAD_IDLE)
				curDFU.curEvent = UPGRADE_EVENT_DFU_GETSTATUS_RESP_STATE_dfuDNLOAD_IDLE;
			else
			{
				curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 7;
				curDFU.ErrCode = err_DFU_state_appIDE + DFUresp->respInfo.Staus_Info.State;
#ifdef DFU_DEBUG
				printf("\tDFU state fail: %d\n", curDFU.ErrCode);
#endif
			}
			break;
		//case cmd_DFU_CLRSTATUS:
		//	break;
		//case cmd_DFU_GETSTATE:
		//	break;
		//case cmd_DFU_ABORT:
		//	break;

		case cmd_DFU_RESET:
			curDFU.curEvent = UPGRADE_EVENT_DFU_RESET_RESP;
			break;

		case cmd_DFU_GETDEVICE:
			curDFU.curEvent = UPGRADE_EVENT_DFU_GETDEVICE_RESP;
			break;

		case cmd_DFU_GETFUNCT:
			// Detach Time doesn't need this time...Because we do DFU_RESET immediately when we get DFU_DETACH Response
			curDFU.TransferSizePerBlock = (DFUresp->respInfo.FunctionInfo.TransferSize < curDFU.DFU_MAX_TRANSFER_SIZE) ? DFUresp->respInfo.FunctionInfo.TransferSize : curDFU.DFU_MAX_TRANSFER_SIZE;
			//OutPutDebugMSG_TransferLength();
			curDFU.curEvent = UPGRADE_EVENT_DFU_GETFUNCTION_RESP;

			break;
		default:
			curDFU.curEvent = UPGRADE_EVENT_ERROR;
				curDFU.AdvErrCode = 8;
			curDFU.ErrCode = err_DFU_UNKNOWN_CMD_RESP;
#ifdef DFU_DEBUG
			printf("\tDFU unknown cmd RSP: %d\n", curDFU.ErrCode);
#endif

			break;			
	}
}



