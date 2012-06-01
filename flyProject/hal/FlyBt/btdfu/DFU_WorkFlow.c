#include "BCSP.h"
#include "DFU.h"
#include "BCCMD.h"
#include "DFU_WorkFlow.h"


U8 BT_CMD_QUERY_VERSION[10];
U8 BT_CMD_DO_DFU[10];
U8 BT_CMD_POWER_ON[10];
U8 BT_CMD_QUERY_VERSION_LEN;
U8 BT_CMD_DO_DFU_LEN;
U8 BT_CMD_POWER_ON_LEN;

DFU_Data curDFU;
BYTE wr_point =0;
BYTE rd_point =0;

U32 LastRxRecTime=0;
U32 LastCheckTime=0;
U32 StartDFUTime=0;
U32 timenow=0;

void sendCMdata(const U8 *pBuf, U8 len)
{
	U8 i=0;

	for (i=0; i<len; i++)
	{
		PutChar(pBuf[i]);
	}
}

void init_DFU(U32 File_size, U32 OrgBaudRate, U32 HighSpeedBaudRate, U16 MAX_PACKET_SIZE, U8 CmdType)
{
	SetBaudRateRegByBaudRate(OrgBaudRate);

	OSTimeDly(100);

	curDFU.curBlock = 0;
	curDFU.curCmd = 0;
	curDFU.curStage = UPGRADE_STAGE_IDLE;
	curDFU.fileSizeDownloaded = 0;
	curDFU.fileSize = File_size;
	curDFU.PollTimeout = 0;
	curDFU.TransferSizePerBlock = 0;
	curDFU.ErrCode = err_NO_ERROR;
	curDFU.curEvent = UPGRADE_EVENT_NO_EVENT;
	curDFU.generalTimer = 0;
	curDFU.BaudRateDetect = BAUDRATE_LOW_BAUDRATE_DETECTING;
	curDFU.OrgBaudRate = OrgBaudRate;
	curDFU.HighSpeedBaudRate = HighSpeedBaudRate;
	curDFU.DFU_MAX_TRANSFER_SIZE = MAX_PACKET_SIZE;

	curDFU.AdvErrCode = 0;
	curDFU.oldStage = 0;
	curDFU.bAckPending = 0;

	curDFU.CmdType = CmdType;
	
	init_BCCMD();


	// kick up DFU progress.....
	if(BT_CMD_POWER_ON_LEN)
	{
		sendCMdata((const U8*)BT_CMD_POWER_ON, BT_CMD_POWER_ON_LEN);
		OSTimeDly(100);
	}
	
	sendCMdata((const U8*)BT_CMD_QUERY_VERSION, BT_CMD_QUERY_VERSION_LEN);
	OSTimeDly(100);
	
	sendCMdata((const U8*)BT_CMD_DO_DFU, BT_CMD_DO_DFU_LEN);

	curDFU.generalTimer = 1500/SYSTEM_POLLING_INTERVAL_BY_MS;	// 5 sec;
	//OutPutDebugMSG_SetTimer();
}

void InteractionByCurStage(void)
{
	U16 data[10];

#ifdef DFU_DEBUG
	printf("\t S: %d\n", curDFU.curStage);
#endif

	switch(curDFU.curStage)
	{
	case UPGRADE_STAGE_IDLE_SYNC_RECIEVED:
	case UPGRADE_STAGE_IDLE_CONF_RECIEVED:
		break;
	case UPGRADE_STAGE_MODIFY_VM_DISABLE:
	{
		data[0] = 0x0001;	// Set VM Disable to True (1)  ==>stop VM First
		BCCMDPS_SET_KEY_Value(PSKEY_VM_DISABLE, data, 1);

		break;
	}
	case UPGRADE_STAGE_MODIFY_INITIAL_BOOTMODE:
	{
		data[0] = 0x0000;	// Set initial bootmode to 0 (BCSP)
		BCCMDPS_SET_KEY_Value(PSKEY_INITIAL_BOOTMODE, data, 1);
	
		break;
	}
	case UPGRADE_STAGE_MODIFY_BAUDRATE:
	{
		//data[0] = (U16)(curDFU.HighSpeedBaudRate * 0.004096+0.5);
		//data[0] = 0x004f;
		data[0] = (U16)((curDFU.HighSpeedBaudRate * 4096 + 500000)/1000000);
		BCCMDPS_SET_KEY_Value(PSKEY_UART_BAUDRATE, data, 1);
	
		break;
	}
	case UPGRADE_STAGE_RUNTIMEMODE_READY:
		sendDFUcmd(cmd_DFU_GETDEVICE);
		break;
	case UPGRADE_STAGE_RUNTIMEMODE_GET_DEVICE_INFO:
		sendDFUcmd(cmd_DFU_GETFUNCT);
		break;
	case UPGRADE_STAGE_RUNTIMEMODE_GET_FUNCTION_INFO:
		sendDFUcmd(cmd_DFU_DETACH);
		break;
	case UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK:
		sendDFUcmd(cmd_DFU_RESET);
		break;
	case UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE:
		OSTimeDly(5);
		if (!SetBaudRateRegByBaudRate(curDFU.HighSpeedBaudRate))
		{
			curDFU.ErrCode = err_FLOWCONTROL_CANNOT_SET_HIGHSPEED_BAUDRATE_REG;
			DFU_ERROR_OCCUR();
			return;
		}

		// We change the Baudrate, 
		// so if we set 5 seconds limit to check whether get the BCSPLE Message;
		curDFU.generalTimer = 2000/SYSTEM_POLLING_INTERVAL_BY_MS; 	
		//OutPutDebugMSG_SetTimer();
		break;
	case UPGRADE_STAGE_DFUMODE_READY:
		sendDFUcmd(cmd_DFU_GETDEVICE);
		break;
	case UPGRADE_STAGE_DFUMODE_GET_DEVICE_INFO:
		sendDFUcmd(cmd_DFU_GETFUNCT);
		break;
	case UPGRADE_STAGE_DFUMODE_GET_FUNCTION_INFO:
		sendDFUcmd(cmd_DFU_GETSTATUS);
		break;
	case UPGRADE_STAGE_DFUMODE_READY_DOWNLOAD:
		sendDFUcmd(cmd_DFU_DNLOAD);
		break;
	case UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC:
		// Do nothing but wait DOWNLOAD_SYNC PollTime out
		break;
	case UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC_TIMEOUT:
		sendDFUcmd(cmd_DFU_GETSTATUS);
		break;
	case UPGRADE_STAGE_DFUMODE_PURE_DATA_DOWNLOAD_FINISH:
		sendDFUcmd(cmd_DFU_DNLOAD);
		break;
	case UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC:
		// Do nothing but wait MANIFEST_SYNC PollTime out
		break;
	case UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC_TIMEOUT:
		sendDFUcmd(cmd_DFU_GETSTATUS);
		break;
	case UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE:
		sendDFUcmd(cmd_DFU_RESET);
		break;
	case UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE:
		// Do nothing
		curDFU.generalTimer = 2000/SYSTEM_POLLING_INTERVAL_BY_MS;
		break;
	case UPGRADE_STAGE_RESTORE_VM_DISABLE:
	{
		data[0] = 0x0000;	// Set VM Disable to False(0)  ==>Enable VM
		BCCMDPS_SET_KEY_Value(PSKEY_VM_DISABLE, data, 1);

		break;
	}
	case UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE:
	{
		data[0] = 0x0001;	// Set initial bootmode to 1 (UART)
		BCCMDPS_SET_KEY_Value(PSKEY_INITIAL_BOOTMODE, data, 1);
		
		break;
	}
	case UPGRADE_STAGE_RESTORE_BAUDRATE:
	{
		//data[0] = (U16)(curDFU.OrgBaudRate * 0.004096+0.5);
		data[0] = (U16)((curDFU.OrgBaudRate * 4096 + 500000)/1000000);
		BCCMDPS_SET_KEY_Value(PSKEY_UART_BAUDRATE, data, 1);
	
		break;
	}
	case UPGRADE_STAGE_ALL_PROCESS_DONE:
		BCCMD_BASIC_CMD(VarID_Cold_Reset, 0, 0);

		OSTimeDly(10);
		// We should change the Host BaudRate to normal speed, after this command send;
		if (!SetBaudRateRegByBaudRate(curDFU.OrgBaudRate))
		{
			curDFU.ErrCode = err_FLOWCONTROL_CANNOT_SET_ORGSPEED_BAUDRATE_REG;
			DFU_ERROR_OCCUR();
			return;
		}
		//MCU_REG_RW_UBAUD0 = BTS_NORMAL_SPEED_BAUDRATE;	

		// We change the Baudrate, 
		// so if we set 5 seconds limit to check whether get the BCSPLE Message;
		curDFU.generalTimer = 2000/SYSTEM_POLLING_INTERVAL_BY_MS;
	//OutPutDebugMSG_SetTimer();
		break;
	case UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_AGAIN:
		BCCMD_BASIC_CMD(VarID_Warm_Reset, 0, 0);

#ifdef IGNORE_RX_LOSE
		// In case we lose the RESET \r\n
		curDFU.generalTimer = 2000/SYSTEM_POLLING_INTERVAL_BY_MS;
#endif
		if(BT_CMD_POWER_ON_LEN)
		{
			OSTimeDly(2000);
			sendCMdata((const U8*)BT_CMD_POWER_ON, BT_CMD_POWER_ON_LEN);
			OSTimeDly(100);
		}
		break;
	case UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_OK:
		curDFU.generalTimer = 0;
		fEnterDFUMode = 0;
		
		sendCMdata((const U8*)BT_CMD_QUERY_VERSION, BT_CMD_QUERY_VERSION_LEN);
		break;
	default:
		//curDFU.curEvent = UPGRADE_EVENT_ERROR;
		curDFU.ErrCode = err_FLOWCONTROL_UNKNOWN_STAGE;
		DFU_ERROR_OCCUR();
		break;
	}
}

void DFU_WorkFlow(void)
{
#ifdef DFU_DEBUG
	printf("\t E %d @S %d \n", curDFU.curEvent, curDFU.curStage);
#endif
	curDFU.oldStage = curDFU.curStage;

	switch (curDFU.curEvent)
	{
	case UPGRADE_EVENT_NO_EVENT:
		return;
	case UPGRADE_EVENT_BCSP_ACK_RECIEVED:
		// Reset the event
		curDFU.curEvent = UPGRADE_EVENT_NO_EVENT;
		return;
	case UPGRADE_EVENT_BCSP_LE_SYNC_RECIEVED:
		if (curDFU.curStage == UPGRADE_STAGE_IDLE)
		{
			curDFU.curStage = UPGRADE_STAGE_IDLE_SYNC_RECIEVED;
			curDFU.generalTimer = 0;	// reset/End the Timer==> For Detect BaudRate
			
			if (curDFU.BaudRateDetect == BAUDRATE_LOW_BAUDRATE_DETECTING)
				curDFU.BaudRateDetect = BAUDRATE_LOW_BAUDRATE_DETECTED;
			else if (curDFU.BaudRateDetect == BAUDRATE_HIGH_BAUDRATE_DETECTING)
				curDFU.BaudRateDetect = BAUDRATE_HIGH_BAUDRATE_DETECTED;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE
#ifdef IGNORE_RX_LOSE
				|| curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK
#endif
			)
		{
			curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE_SYNC_RECIEVED;
			curDFU.generalTimer = 0;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE
#ifdef IGNORE_RX_LOSE
				|| curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE
#endif
			)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE_SYNC_RECIEVED;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE)
		{
			curDFU.curStage = UPGRADE_STAGE_ALL_PROCESS_DONE_SYNC_RECIEVED;
			curDFU.generalTimer = 0;
		}
		
		BCSP_LE_send_SYNC_RESP();
		// Reset the event
		curDFU.curEvent = UPGRADE_EVENT_NO_EVENT;
		return;	// BCSPLE Event don't go to InteractionByCurStage
	case UPGRADE_EVENT_BCSP_LE_CONF_RECIEVED:
		if (curDFU.curStage == UPGRADE_STAGE_IDLE_SYNC_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_IDLE_CONF_RECIEVED;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE_CONF_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE_SYNC_RECIEVED;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE_SYNC_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE_CONF_RECIEVED;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE_SYNC_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_ALL_PROCESS_DONE_CONF_RECIEVED;
		}

		BCSP_LE_send_CONF_RESP();
		curDFU.generalTimer = 1500/SYSTEM_POLLING_INTERVAL_BY_MS;	// wait 5 second to go ahead
		//OutPutDebugMSG_SetTimer();
		// Reset the event
		curDFU.curEvent = UPGRADE_EVENT_NO_EVENT;
		return;	// BCSPLE Event don't go to InteractionByCurStage

	case UPGRADE_EVENT_MODIFY_VM_DISABLE_OK:
/*		curDFU.curStage = UPGRADE_STAGE_MODIFY_INITIAL_BOOTMODE;*/	/* John VM InitBootMode */
		curDFU.curStage = UPGRADE_STAGE_MODIFY_BAUDRATE;
		break;

	case UPGRADE_EVENT_MODIFY_INITIAL_BOOTMODE_OK:
/*		curDFU.curStage = UPGRADE_STAGE_MODIFY_BAUDRATE;*/	/* John VM InitBootMode */
		curDFU.curStage = UPGRADE_STAGE_MODIFY_VM_DISABLE;
		break;
		
	case UPGRADE_EVENT_MODIFY_BAUDRATE_OK:
		curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_READY;
		break;
	case UPGRADE_EVENT_DFU_GETDEVICE_RESP:
	{
		if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_READY)
		{
			curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_GET_DEVICE_INFO;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_READY)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_GET_DEVICE_INFO;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}
		break;
	}
	case UPGRADE_EVENT_DFU_GETFUNCTION_RESP:
		
		if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_GET_DEVICE_INFO)
		{
			curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_GET_FUNCTION_INFO;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_GET_DEVICE_INFO)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_GET_FUNCTION_INFO;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}

		break;
	case UPGRADE_EVENT_DFU_DETACH_RESP:
		curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK;
		break;
	case UPGRADE_EVENT_DFU_RESET_RESP:
	{
		if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK)
		{
			curDFU.curStage = UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}

		break;
	}
	case UPGRADE_EVENT_DFU_GETSTATUS_RESP_STATE_dfuIDLE:
	{
		if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_GET_FUNCTION_INFO)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_READY_DOWNLOAD;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC_TIMEOUT)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}
		break;
	}
	case UPGRADE_EVENT_DFU_GETSTATUS_RESP_STATE_dfuDNLOAD_IDLE:
	{
		if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC_TIMEOUT)
		{
			if ((curDFU.fileSize - curDFU.fileSizeDownloaded) <= 16)
				curDFU.curStage = UPGRADE_STAGE_DFUMODE_PURE_DATA_DOWNLOAD_FINISH;
			else
				curDFU.curStage = UPGRADE_STAGE_DFUMODE_READY_DOWNLOAD;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}
		
		break;
	}
	case UPGRADE_EVENT_DFU_DOWNLOAD_RESP:
	{
		if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_READY_DOWNLOAD)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC;
			curDFU.generalTimer = (curDFU.PollTimeout/SYSTEM_POLLING_INTERVAL_BY_MS)+2;
			//OutPutDebugMSG_SetTimer();
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_PURE_DATA_DOWNLOAD_FINISH)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC;

#ifdef IGNORE_RX_LOSE
			if (curDFU.PollTimeout< 11000)
				curDFU.PollTimeout = 12000;
#endif
			curDFU.generalTimer = (curDFU.PollTimeout/SYSTEM_POLLING_INTERVAL_BY_MS)+1;
			//OutPutDebugMSG_SetTimer();
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}
		break;
	}
	case UPGRADE_EVENT_RESTORE_VM_DISABLE_OK:
		if (curDFU.curStage == UPGRADE_STAGE_RESTORE_VM_DISABLE)
		{
			curDFU.curStage = UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE;
			break;
		}
		else
			return;

	case UPGRADE_EVENT_RESTORE_INITIAL_BOOTMODE_OK:
		if (curDFU.curStage == UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE)
		{
			curDFU.curStage = UPGRADE_STAGE_ALL_PROCESS_DONE;
			//curDFU.curStage = UPGRADE_STAGE_RESTORE_BAUDRATE;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}
		
		break;
	case UPGRADE_EVENT_RESTORE_BAUDRATE_OK:
		if (curDFU.curStage == UPGRADE_STAGE_RESTORE_BAUDRATE)
		{
			//curDFU.curStage = UPGRADE_STAGE_ALL_PROCESS_DONE;
			//curDFU.curStage = UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE;
			curDFU.curStage = UPGRADE_STAGE_RESTORE_VM_DISABLE;
		}
		else
		{
//			curDFU.ErrCode = err_FLOWCONTROL_WRONG_STAGE;
//			DFU_ERROR_OCCUR();
			return;
		}
		
		break;
	case UPGRADE_EVENT_RECEIVED_ODOA:
		curDFU.curStage = UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_OK;
		break;

	case UPGRADE_EVENT_TIMER_TIMEOUT:
		if (curDFU.curStage == UPGRADE_STAGE_IDLE)
		{
			if (curDFU.BaudRateDetect == BAUDRATE_LOW_BAUDRATE_DETECTING)
			{
				curDFU.BaudRateDetect = BAUDRATE_HIGH_BAUDRATE_DETECTING;
				if (!SetBaudRateRegByBaudRate(curDFU.HighSpeedBaudRate))
				{
					curDFU.ErrCode = err_FLOWCONTROL_CANNOT_SET_HIGHSPEED_BAUDRATE_REG;
					DFU_ERROR_OCCUR();
					return;
				}
				//MCU_REG_RW_UBAUD0 = BTS_HIGH_SPEED_BAUDRATE;	
				curDFU.generalTimer = 2500/SYSTEM_POLLING_INTERVAL_BY_MS;	// 1000*5 = 5 Sec; Check whether got BCSPLE SYNC in tihs 5 seconds.
				//OutPutDebugMSG_SetTimer();
				// Reset the event
				curDFU.curEvent = UPGRADE_EVENT_NO_EVENT;
				return;
			}
			else if (curDFU.BaudRateDetect == BAUDRATE_HIGH_BAUDRATE_DETECTING)
			{
				curDFU.ErrCode = err_FLOWCONTROL_CANNOT_DETECT_BAUDRATE;
				DFU_ERROR_OCCUR();
				return;
			}
		}
		/*
		else if (curDFU.curStage == UPGRADE_STAGE_IDLE_CONF_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_MODIFY_INITIAL_BOOTMODE;
		}
		*/
		else if (curDFU.curStage == UPGRADE_STAGE_IDLE_CONF_RECIEVED)
		{
/*			curDFU.curStage = UPGRADE_STAGE_MODIFY_VM_DISABLE;*/	/* John VM InitBootMode */
			curDFU.curStage = UPGRADE_STAGE_MODIFY_INITIAL_BOOTMODE;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE)
		{
			curDFU.ErrCode = err_FLOWCONTROL_CANNOT_BCSPLE_AFTER_BAUDRATE_SPEEDUP;
			DFU_ERROR_OCCUR();
			return;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE)
		{
			curDFU.ErrCode = err_FLOWCONTROL_CANNOT_BCSPLE_AFTER_BAUDRATE_SPEEDDOWN;
			DFU_ERROR_OCCUR();
		}
		else if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_CHANGE_2_DFUMODE_SYNC_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_READY;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE)
		{
			// Sometimes DFU_RESET won't really reset (become to send BCSP LE)
			// So if we wait it for timeout, let's send DFU_reset  again
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_FINISH_WAIT_BCSP_LE_CONF_RECIEVED)
		{
			//curDFU.curStage = UPGRADE_STAGE_RESTORE_INITIAL_BOOTMODE;
			curDFU.curStage = UPGRADE_STAGE_RESTORE_BAUDRATE;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE_CONF_RECIEVED)
		{
			curDFU.curStage = UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_AGAIN;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC_TIMEOUT;
		}
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC)
		{
			curDFU.curStage = UPGRADE_STAGE_DFUMODE_MANIFEST_SYNC_TIMEOUT;
		}/*
		else if (curDFU.ExpectResponseProtocol == PROTOCOL_DFU || curDFU.ExpectResponseProtocol == PROTOCOL_BCCMD)
		{
			#ifdef DFU_DEBUG
				printf("=== Resend the data don't increse seq\n");
			#endif

			sendBCSPPacket(&g_TxPacket);
			curDFU.generalTimer = 50;

			return;
		}*/
#ifdef IGNORE_RX_LOSE
		else if (curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_AGAIN)
		{
			/* 
			   After send BCCMD RESET,  BC return SC000\r\n But 8202T lose \r\n
			   so can't go ahead into UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_OK Stage
			*/
			//OutPutDebugMSG_StageEvent();
			sendCMdata((const U8*)BT_CMD_QUERY_VERSION, BT_CMD_QUERY_VERSION_LEN);
			return;
		}
#endif
		else if (curDFU.curStage == UPGRADE_STAGE_DFUMODE_READY_DOWNLOAD)
		{
			curDFU.ErrCode = 0x444;
			return;
		}			
		else if (curDFU.curStage == UPGRADE_STAGE_RUNTIMEMODE_DETACH_OK || curDFU.curStage == UPGRADE_STAGE_DFUMODE_DOWNLOAD_COMPLETE/*UPGRADE_STAGE_DFUMODE_DOWNLOAD_SYNC_TIMEOUT*/)
		{
			curDFU.ErrCode = 0x555;
			return;
		}
		else
		{
			// if not specify which Stage is, then just re-do again the InteractionByStage()
			// Like resend DFU cmd or Resend BCCMD
			//curDFU.bAckPending = 1;

			//OutPutDebugMSG_StageEvent();
			
			sendBCSPPacket(&g_TxPacket, 1);
			//BCSP2SLIP(&g_TxPacket);
			curDFU.generalTimer = 1000/SYSTEM_POLLING_INTERVAL_BY_MS;
			//OutPutDebugMSG_SetTimer();
			curDFU.ExpectResponseProtocol = g_TxPacket.protocolID;

			return;
		}
		
		break;
	case UPGRADE_EVENT_ERROR:
		//DEBUG("== Error No : %ull occurs at %ul~~\r\n", curDFU.ErrCode, curDFU.curStage);
		
		DFU_ERROR_OCCUR();

		return;
	case UPGRADE_EVENT_REDO:
		curDFU.curStage = curDFU.curStage;
		
		break;
	}

	//OutPutDebugMSG_StageEvent();

	InteractionByCurStage();

	// Reset the event
	curDFU.curEvent = UPGRADE_EVENT_NO_EVENT;
}

void DFU_ERROR_OCCUR(void)
{
	SetBaudRateRegByBaudRate(curDFU.OrgBaudRate);
	fEnterDFUMode = 0;
}

void DoDFU(void)
{
	U8 *s;
	U8 i, b, e, temp_End;

	wr_point=0;
	rd_point=0;
	temp_End=0;

	g_rxEndSlip=0;
	g_rxStartSlip=0;
	b=e=0;

	s = curDFU.UARTRawBuffer;

	StartDFUTime = GetTimeReg();
	LastCheckTime = GetTimeReg();

	while (1)
	{
		U8 isSet = 0;

		polling_BT_DFU();
		
		if (curDFU.ErrCode)
		{
			break;
		}
				
		temp_End = wr_point;

		if (curDFU.curStage == UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_AGAIN)
		{
			if((curDFU.CmdType <= BT_CMD_SET_GOC && ((rd_point != temp_End) && s[temp_End -1] == 0x0a  && s[temp_End -2] == 0x0d)) ||
				(curDFU.CmdType == BT_CMD_SET_DEG && ((rd_point != temp_End) && s[temp_End -1] == 0xff)))
			{
				curDFU.curEvent = UPGRADE_EVENT_RECEIVED_ODOA;
				DFU_WorkFlow();

				break;
			}
		}


		for (i=rd_point; i!=temp_End; i++)
		{
			if (s[i] == 0xc0)
			{
				if ( isSet == 0 || (i == b+1))
				{
					b = i;
					isSet = 1;
					continue;
				}
				else if (i != b)
				{
					// find the End of BCSP
					e = i;
					break;
				}
			}
		}

		// Check whether got BCSP Packet handle it first then check is other timeout event
		if ( (i == temp_End) || /*( ((U8)(e-b)) > 60) ||*/  (e == b) || (s[b] != 0xc0) || (s[e] != 0xc0)) 
		{
			//OSTimeDly(2);
			//continue;
		}
		else
		{
			g_rxStartSlip = b;
			g_rxEndSlip = e;

			if ( BCSPFromRawSLIP(s, g_rxStartSlip, g_rxEndSlip))
			{
#ifdef DFU_DEBUG
				printf("0x%02x <-> 0x%02x\n", b, e);
#endif
				HandleBCSPPacket();
			}
			else
			{
				//OutPutDebugMSG_LOST();
#ifdef IGNORE_RX_LOSE
				if ( (e-b) > 3 && 
					(g_RxPacket.protocolID == PROTOCOL_NONE || 
					g_RxPacket.protocolID == PROTOCOL_DFU || 
					g_RxPacket.protocolID == PROTOCOL_BCCMD))	// Maybe ack packet
				{
					// Handle BCSP Boundary is OK (0xC0 to 0xC0), But lose byte inner the boundary.
					HandleIncompleteBCSPPacket();
				}
#endif
			}

			rd_point = g_rxEndSlip+1;

#ifdef IGNORE_RX_LOSE
			/* 
				ex: C0 78 00 00 C0 FE AC 00 55 00 00 06 00 00 8E 00 00 05 00 3F 6E C0
				Actually is 2 Packet:  
					1. C0 78 00 00 C0 (Not health BCSP Packet)
					2. FE AC 00 55 00 00 06 00 00 8E 00 00 05 00 3F 6E C0 (Fail, No C0 at the first)				              
			*/
			if (rd_point != wr_point && s[rd_point] != 0xC0)
				rd_point = g_rxEndSlip;
#endif

		}

#ifdef IGNORE_RX_LOSE
		if (curDFU.curStage <UPGRADE_STAGE_ALL_PROCESS_DONE_TRY_RESET_AGAIN && IsLoseEndEscapeChar())
		{
			g_rxStartSlip = rd_point;
			g_rxEndSlip = wr_point;
			BCSPFromRawSLIP(s, g_rxStartSlip, g_rxEndSlip);
			
			if ( /*(e-b) > 6 || */ g_RxPacket.protocolID == PROTOCOL_NONE || g_RxPacket.protocolID == PROTOCOL_DFU || g_RxPacket.protocolID == PROTOCOL_BCCMD || g_RxPacket.protocolID == PROTOCOL_BCSP_LE)	// Maybe ack packet
			{
				HandleIncompleteBCSPPacket();
			}			
			
			rd_point = g_rxEndSlip;
		}

/*
		if (IsDFUDown())
		{
			curDFU.ErrCode = 0x666;
			break;
		}*/

		
#endif


		if (curDFU.generalTimer == 1)
		{
			curDFU.curEvent = UPGRADE_EVENT_TIMER_TIMEOUT;
			curDFU.generalTimer = 0;
			DFU_WorkFlow();
		}

//		OSTimeDly(2);
	}
}

void setup_DataBuffer(U8 *UARTRawBuf, U8 * BCSPRxBuf, U8 * BCSPTxBuf, U8 * BCSPAckBuf)
{
	curDFU.UARTRawBuffer = UARTRawBuf;

	g_RxPacket.PureBCSPBuf = BCSPRxBuf;
	g_TxPacket.PureBCSPBuf = BCSPTxBuf;
	g_AckPacket.PureBCSPBuf = BCSPAckBuf;

	g_TxPacket.header = g_TxPacket.PureBCSPBuf;
	g_RxPacket.header = g_RxPacket.PureBCSPBuf;
	g_AckPacket.header = g_AckPacket.PureBCSPBuf;

	g_TxPacket.payload	= g_TxPacket.PureBCSPBuf+4;
	g_RxPacket.payload	= g_RxPacket.PureBCSPBuf+4;
	g_AckPacket.payload	= g_AckPacket.PureBCSPBuf+4;
}

void polling_BT_DFU(void)
{
	U32  TimeDiffByMs;

	timenow = GetTimeReg();
	TimeDiffByMs = CountTimeInterval(LastCheckTime, timenow);

	if (TimeDiffByMs > SYSTEM_POLLING_INTERVAL_BY_MS)
	{
		if (curDFU.generalTimer > 1)
		{
			curDFU.generalTimer = (curDFU.generalTimer > (TimeDiffByMs/SYSTEM_POLLING_INTERVAL_BY_MS)) ? (curDFU.generalTimer - (TimeDiffByMs/SYSTEM_POLLING_INTERVAL_BY_MS)) : 1;
		}

		LastCheckTime = timenow;
	}

/*
	if (SYSTEM_POLLING_INTERVAL_BY_MS == 1)
	{
		if (curDFU.generalTimer > 1)
		{
			curDFU.generalTimer = (curDFU.generalTimer > TimeDiffByMs) ? (curDFU.generalTimer-TimeDiffByMs) : 1;
		}
		LastCheckTime = timenow;
	}
	else if (TimeDiffByMs > SYSTEM_POLLING_INTERVAL_BY_MS)
	{
		if (curDFU.generalTimer > 1)
		{
			curDFU.generalTimer--;
			//curDFU.generalTimer = (curDFU.generalTimer > (timenow - LastCheckTime)) ? (curDFU.generalTimer - (timenow - LastCheckTime)) : 1;
		}

		LastCheckTime = timenow;
	}
*/

}

U8 IsLoseEndEscapeChar(void)
{
	U8 *s;

	timenow = GetTimeReg();
	
	s = curDFU.UARTRawBuffer;

	if ( CountTimeInterval(LastRxRecTime, timenow) > 150 && (wr_point - rd_point) > 3 &&  s[rd_point] == 0xc0 && s[wr_point] != 0xC0)
	{
		return 1;
	}
	else
		return 0;

}

U8 IsDFUDown(void)
{
	timenow = GetTimeReg();

	if (StartDFUTime == 0 || LastRxRecTime)
		return 0;

	if (timenow < LastRxRecTime)
		return 0;

	// if DFU execute exceed 380 second or LastRX is over 25 second ago, return 1;
	if ( ( CountTimeInterval(StartDFUTime, timenow)>600000) || (CountTimeInterval(LastRxRecTime, timenow)>25000))
	{
#ifdef DFU_DEBUG
		printf("Duration from Start:%ld, from lastRx:%ld\r\n", CountTimeInterval(StartDFUTime, timenow), CountTimeInterval(LastRxRecTime, timenow));
#endif
		return 1;
	}
	else
		return 0;
}


