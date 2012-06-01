#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/select.h>
#include <sys/types.h> 
#include <cutils/atomic.h>
#include <hardware/hardware.h>  


#define SERIAL_BT_NAME			"/dev/tcc-uart1"    
#define SERIAL_BAUDRATE_OLD      9600
#define SERIAL_BAUDRATE_NEW      115200
#define DUF_FILE_NAME "/mnt/sdcard/tflash/flyaudio/bt/JS_FlyAudio_BT_Update.dfu"



#include "typedef_BT.h"
#include "btdfu.h"
#include "BCSP.h"
#include "DFU_WorkFlow.h"
#include "serial.h"

static U8 *COMPortString; 
static int serial_fd = -1;
static int dfu_fd = -1;
static char serial_thread_status = 0;

//程式使用buffer
#define OS_MAX_TX_SIZE 1100
U8	UARTRawBuf[256];
U8	BCSPAckBuf[20];
U8	BCSPRxBuf[64];
U8	BCSPTxBuf[OS_MAX_TX_SIZE];
U8  CurrCmdType;

void OSTimeDly(U16 n)
{
	usleep(n*1000);
}

U32 GetTimeReg(void)
{
	struct timeval t_now;
	gettimeofday(&t_now, NULL);
	return (U32)(t_now.tv_sec * 1000 + t_now.tv_usec / 1000) & 0x7FFFFFFF;
	
}

U32 CountTimeInterval(U32 StartTime, U32 EndTime)
{
	return (EndTime - StartTime);
}

U8	txTempBuff[300];
unsigned long dwCharWrite = 0;
void PutChar(U8 ch)
{
	int i=0;
	unsigned long written = 0;
	txTempBuff[dwCharWrite++] = ch;

	if ((CurrCmdType <= BT_CMD_SET_GOC || ch == SLIP_END || dwCharWrite == 200 || (dwCharWrite>=2 && ch == '\n' && txTempBuff[dwCharWrite-2] == '\r')) ||
		(CurrCmdType == BT_CMD_SET_DEG || ch == SLIP_END || dwCharWrite == 200 || (dwCharWrite>=3 && ch == 0xff)))
	{
		
		if(bt_serial_write(serial_fd, txTempBuff, dwCharWrite) < 0)
		{
			DLOGD("bt serial write err!\n");
		}
		else
		{
			DLOGD("bt serial write: %02X ", txTempBuff[0]);
		}	
		dwCharWrite = 0;
	}
}
U8 SetBaudRateRegByBaudRate(U32 baudrate)
{
	struct termios serial_newtio;
	
	if (serial_fd > 0)
	{
		//将目前终端机参数保存到结构体变量flydvd_oldtio中
		if(tcgetattr(serial_fd, &serial_newtio) < 0)
		{
			DLOGD("tcgetattr err\n");
			return 0;
		}
	
		bt_serial_baudrate(&serial_newtio, baudrate);
		
		//刷新在串口中的输入输出数据
		//tcflush(serial_fd, TCIFLUSH);

		//设置当前的的串口参数为flydvd_newtio
		if(tcsetattr(serial_fd, TCSANOW, &serial_newtio)<0)
		{
			DLOGD("tcsetattr err\n");
			return 0;
		}
	}
	
	return 1;
}
//读升级文件的内容
void InsertDFUFileData(U8 *destAdd, U32 offset, U16 size)
{
	if (dfu_fd > 0){
		if (read(dfu_fd, destAdd, size) != size){
			DLOGD("DFU Need Size is different to ReadFile Size\n");
		}
	}
}

static int SetDFUCmd(U16 CmdType)
{
	memset(BT_CMD_DO_DFU, 0, 10);
	memset(BT_CMD_QUERY_VERSION, 0, 10);
	memset(BT_CMD_POWER_ON, 0, 10);
	BT_CMD_POWER_ON_LEN = 0;
	

	if (CmdType == BT_CMD_SET_COORDIWISE)
	{
		memcpy(BT_CMD_DO_DFU, "du\r\n", 4);
		memcpy(BT_CMD_QUERY_VERSION, "qf\r\n", 4);
		memcpy(BT_CMD_POWER_ON, "so\r\n", 4);

		BT_CMD_DO_DFU_LEN = 4;
		BT_CMD_QUERY_VERSION_LEN = 4;
		BT_CMD_POWER_ON_LEN = 4;

		return 1;
	}
	else if (CmdType == BT_CMD_SET_COLLEX)
	{
		memcpy(BT_CMD_DO_DFU, "r1\r\n", 4);
		memcpy(BT_CMD_QUERY_VERSION, "i\r\n", 3);

		BT_CMD_DO_DFU_LEN = 4;
		BT_CMD_QUERY_VERSION_LEN = 3;

		return 1;
	}
	else if (CmdType == BT_CMD_SET_FLC)
	{
		memcpy(BT_CMD_DO_DFU, "AT#DU\r\n", 7);
		memcpy(BT_CMD_QUERY_VERSION, "COMMON_V\r\n", 10);

		BT_CMD_DO_DFU_LEN = 7;
		BT_CMD_QUERY_VERSION_LEN = 10;

		return 1;
	}
	else if (CmdType == BT_CMD_SET_GOC)
	{
		memcpy(BT_CMD_DO_DFU, "AT-DU\r\n", 7);
		memcpy(BT_CMD_QUERY_VERSION, "COMMON_V\r\n", 10);

		BT_CMD_DO_DFU_LEN = 7;
		BT_CMD_QUERY_VERSION_LEN = 10;

		return 1;
	}
	else if (CmdType == BT_CMD_SET_DEG)
	{
		BT_CMD_DO_DFU[0] = 0x70;
		BT_CMD_DO_DFU[1] = 0x0;
		BT_CMD_DO_DFU[2] = 0xff;
		BT_CMD_QUERY_VERSION[0] = 0x0b;
		BT_CMD_QUERY_VERSION[1] = 0x00;
		BT_CMD_QUERY_VERSION[2] = 0xff;

		BT_CMD_DO_DFU_LEN = 3;
		BT_CMD_QUERY_VERSION_LEN = 3;

		return 1;
	}
	else
	{
		BT_CMD_QUERY_VERSION_LEN = 0;
		BT_CMD_DO_DFU_LEN = 0;

		return 0;
	}
}

void *ThreadComRead(void *arg)
{
	U8 ch;
	int ret = 0;
	U8 *buf = (U8 *)arg;
	
	//tcflush(serial_fd, TCIFLUSH);
	while (serial_thread_status)
	{
		ret = bt_serial_read(serial_fd, &ch, 1);
		if (ret > 0)
		{
			buf[wr_point++] = ch;
			LastRxRecTime = GetTimeReg();
			DLOGD("serial com read:%02X\n", ch);
		}
	}
	
	DLOGD("thread com read exit\n");
	return NULL;
}
void *ThreadLoadTimer(void *arg)
{
	while (serial_thread_status)
	{
		DLOGD("----->load:%d/100, fileSizeDownloaded:%d,fileSize:%d", 
			curDFU.fileSizeDownloaded*100/(curDFU.fileSize-16), curDFU.fileSizeDownloaded, curDFU.fileSize);
		OSTimeDly(500);
	}
	DLOGD("bt load timer thread exit\n");
	return NULL;
}

unsigned char read_bt_dfu_schedule(void)
{
	return curDFU.fileSizeDownloaded*100 / (curDFU.fileSize-16);
}

//打开串口
int Setup_UART(U32 baudrate, U8 *buf)
{
	int res;
	pthread_t thread_id;
	struct serial_struct serial;
	
	memset(&serial, 0, sizeof(struct serial_struct));
	
	serial_fd = bt_serial_open((const char*)COMPortString, baudrate);
	if (serial_fd <= 0){
		DLOGD("open serial com:%s err\n", COMPortString);
		return -1;
	}

	res = pthread_create(&thread_id, NULL,ThreadComRead,buf);
	if(res != 0) 
	{
		DLOGD("create com read thread err!\n");
		return -2;
	}
	serial_thread_status = 1;
	
	return 0;
}

void Setup_Timer(void)
{
	int res;
	pthread_t thread_id;

	res = pthread_create(&thread_id, NULL,ThreadLoadTimer,NULL);
	if(res != 0) 
	{
		DLOGD("create load timer thread err!\n");
		return;
	}
}

unsigned long get_file_size(const char *path)
{
	struct stat file_stat;
	if (!stat(path, &file_stat))
		return file_stat.st_size;
	else 
		return 0;
}


int set_bt_dfu(void)
{
	int ret = -1;
	U16 tx_size  = 1000;
	U16 cmd_type = 1;
	U32 bt_dfu_file_size=0;
	U32	org_baudrate = SERIAL_BAUDRATE_OLD;
	U32 high_speed_baudrate = SERIAL_BAUDRATE_NEW;
	
	COMPortString = (U8*)SERIAL_BT_NAME;
	
	/*设定模組使用协议
		CmdType，处理不同协议用的. 
		COW 协议设为 0,
		FIH 协议设为 1,
		FLC 协议设为 2,
		GOC 协议设为 3,
		DEG 协议设为 4
	*/
	if (!SetDFUCmd(cmd_type))
	{
		DLOGD("Command Set Set Fail\n");
		ret = -1;
		goto fail;
	}
	CurrCmdType = cmd_type;

	//设定程式需使用到的 buffer
	memset(UARTRawBuf, 0, 256);
	setup_DataBuffer(UARTRawBuf, BCSPRxBuf, BCSPTxBuf, BCSPAckBuf);
	
	
	//设定 UART
	if (Setup_UART(org_baudrate, curDFU.UARTRawBuffer) < 0)
	{
		DLOGD("Setup_UART err\n");
		goto fail;
	}
	//Setup_Timer();
	
	//打开DFU升级文件
	dfu_fd = open(DUF_FILE_NAME, O_RDONLY, 0777);
	if (dfu_fd <= 0){
		DLOGD("open dfu file err!\n");
		ret = -2;
		goto fail;
	}
	
	bt_dfu_file_size = get_file_size(DUF_FILE_NAME);
	if (!bt_dfu_file_size){
		DLOGD("get dfu file size is zero\n");
		ret = -3;
		goto fail;
	}
	DLOGD("bt dfu file size is :%d", bt_dfu_file_size);
	
	//DFU 功能初始化动作
	init_DFU(bt_dfu_file_size, org_baudrate, high_speed_baudrate, tx_size, cmd_type);
	
	//OSTimeDly(500);
	//开始 DFU 更新，直到更新完成或更新错误时結束
	DoDFU();
	DLOGD("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
	DLOGD("----->load:%d/100, fileSizeDownloaded:%d,fileSize:%d", 
			curDFU.fileSizeDownloaded*100/(curDFU.fileSize-16), curDFU.fileSizeDownloaded, curDFU.fileSize);
	DLOGD("===================%s %s===================", __TIME__, __DATE__);
	if (curDFU.ErrCode)
	{
		unsigned long written = 0;
		U8 i=0;
		DLOGD("\nError occurs during DFU Progressing\r\n");
		DLOGD("\tCurrent Stage is %u\r\n", curDFU.curStage);
		DLOGD("\tCurrent EVENT is %u\r\n", curDFU.curEvent);
		DLOGD("\tError Code is 0x%x\r\n", curDFU.ErrCode);

		if (curDFU.curEvent == UPGRADE_EVENT_ERROR)
		{
			DLOGD("\t\tAdvErrCode is %u\r\n", curDFU.AdvErrCode);
		}
		ret = -4;
		goto fail;
	}
	else
	{
		DLOGD("\nDevice Upgrade Completed !!, total time:%d sec\r\n\r\n\r\n", 
				CountTimeInterval(StartDFUTime, GetTimeReg())/1000);
		ret = 1;
	}
fail:
	DLOGD("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

	serial_thread_status = 0;
	bt_serial_close(serial_fd);
	OSTimeDly(1000);
	close(dfu_fd);
	DLOGD("ret =%d\n", ret);
	return ret;
}