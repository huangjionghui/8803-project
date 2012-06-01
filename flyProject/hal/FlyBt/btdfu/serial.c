#include <fcntl.h>  
#include <errno.h>  
#include <stdio.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <sys/times.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/types.h>
#include <string.h>

#include "serial.h"
#define SET_UNUSUAL 0

//串口相关
struct termios serial_oldtio;
struct termios serial_newtio;
fd_set read_fds;

	
void bt_serial_baudrate(struct termios *serl_ts, int nSpeed)
{
	switch (nSpeed)
	{
		case 9600:
			cfsetispeed(serl_ts, B9600);
			cfsetospeed(serl_ts, B9600);
			break;
		
		case 19200:
			cfsetispeed(serl_ts, B19200);
			cfsetospeed(serl_ts, B19200);
			break;
		
		case 57600:
			cfsetispeed(serl_ts, B57600);
			cfsetospeed(serl_ts, B57600);
			break;
			
		case 115200:
			cfsetispeed(serl_ts, B115200);
			cfsetospeed(serl_ts, B115200);
			break;
			
		default:
			cfsetispeed(serl_ts, B9600);
			cfsetospeed(serl_ts, B9600);
			break;
	}
}
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 static int set_serial_opts(int fd, int nSpeed, int nBits, char nEvent, int nStop)
 {
	int ret = -1;
	struct serial_struct serial_st;
	
	if (fd < 0)
		return -1;
	
	//将目前终端机参数保存到结构体变量flydvd_oldtio中
	if (tcgetattr(fd, &serial_oldtio) != 0)
	{
		goto fail;
	}
	
	//结构体serial_newtio清0
	memset(&serial_newtio, 0, sizeof(serial_newtio));
			
	//设置串口参数
	serial_newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	serial_newtio.c_oflag &= ~OPOST;
	serial_newtio.c_iflag &= ~(ICRNL | IGNCR);
	serial_newtio.c_cflag |= CLOCAL | CREAD;           //启动接收器
	serial_newtio.c_cflag &= ~CSIZE;

	serial_newtio.c_cc[VTIME] = 0; 
	serial_newtio.c_cc[VMIN]  = 0; 
	
	serial_newtio.c_cc[VINTR] = 0; 
	serial_newtio.c_cc[VQUIT] = 0; 
	serial_newtio.c_cc[VERASE] = 0; 
	serial_newtio.c_cc[VKILL] = 0; 
	serial_newtio.c_cc[VEOF] = 0; 
	serial_newtio.c_cc[VTIME] = 1; 
	serial_newtio.c_cc[VMIN] = 0; 
	serial_newtio.c_cc[VSWTC] = 0; 
	serial_newtio.c_cc[VSTART] = 0; 
	serial_newtio.c_cc[VSTOP] = 0; 
	serial_newtio.c_cc[VSUSP] = 0; 
	serial_newtio.c_cc[VEOL] = 0; 
	serial_newtio.c_cc[VREPRINT] = 0; 
	serial_newtio.c_cc[VDISCARD] = 0; 
	serial_newtio.c_cc[VWERASE] = 0; 
	serial_newtio.c_cc[VLNEXT] = 0; 
	serial_newtio.c_cc[VEOL2] = 0; 


	switch (nBits)
	{
		case 7: serial_newtio.c_cflag |= CS7;break;
		case 8: serial_newtio.c_cflag |= CS8;break;
		default:
			serial_newtio.c_cflag |= CS8;break;
	}
	
	
	switch (nEvent)
	{
		case 'O':
			serial_newtio.c_cflag |= PARENB;
			serial_newtio.c_cflag |= PARODD;
			serial_newtio.c_iflag |= (INPCK|ISTRIP);
			break;
			
		case 'E':
			serial_newtio.c_cflag |= PARENB;
			serial_newtio.c_cflag &= ~PARODD;
			serial_newtio.c_iflag |= (INPCK|ISTRIP);
			break;
		
		case 'N':
			serial_newtio.c_cflag &= ~PARENB;
			break;
		
		default:
			serial_newtio.c_cflag &= ~PARENB;
			break;
	}
	
#if SET_UNUSUAL
	cfsetispeed(&serial_newtio,B38400);
    cfsetospeed(&serial_newtio,B38400);

#else
	bt_serial_baudrate(&serial_newtio,nSpeed);
#endif

	if (nStop == 1)
		serial_newtio.c_cflag &= ~CSTOPB;
	else if (nStop == 2)
		serial_newtio.c_cflag = CSTOPB;
		
	//刷新在串口中的输入输出数据
	tcflush(fd, TCIFLUSH);
			
	//设置当前的的串口参数为flydvd_newtio
	if (tcsetattr(fd, TCSANOW, &serial_newtio) != 0)
	{
		goto fail;
	}
	
#if SET_UNUSUAL
	memset(&serial_st, 0, sizeof(struct serial_struct));
	ioctl(fd, TIOCGSERIAL, &serial_st);
	
	serial_st.flags = ASYNC_SPD_CUST;
	serial_st.baud_base = 115200;
    serial_st.custom_divisor = serial_st.baud_base / nSpeed;
	
	ioctl(fd, TIOCSSERIAL, &serial_st);
#endif

	ret = 0;
			
fail:
		return ret;
 }
 
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 int bt_serial_write(int fd, unsigned char *buf, int len)
 {
	int ret = -1;
	
	if (fd < 0)
		return ret;
		
	//写入数据
	ret = write(fd, buf, len);
	
	return ret;
 }
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 long bt_serial_read(int fd, unsigned char *buf, int len)
 {
	long ret = -1;
	
	/*
	struct timeval tv_timeout;

	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	tv_timeout.tv_sec = 1;
	tv_timeout.tv_usec = 0;

	switch (select(fd+1, &read_fds, NULL, NULL, &tv_timeout))
	{
		case -1:
			//DLOGE("select error");
			break;
				
		case 0:
			//DLOGE("select timeout");
			break;
				
		default:
			if (1)
			//if (FD_ISSET(fd, &read_fds))
			{
			*/
				//读取数据
				ret = read(fd, buf, len);
				/*
			}
			break;	
		}
		*/
	return ret;
 }
 
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
int bt_serial_open(const char *serial_name, int baudrate)
{
	int fd = -1;
	int res = 0;
	pthread_t thread_id;
	
	fd = open(serial_name,O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd > 0)     
	{  
		if (set_serial_opts(fd, baudrate, 8, 'N', 1) == -1)
		{
			return -1;
		}
		
		FD_ZERO(&read_fds);
	}
	
	return fd;
}
 /********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
int bt_serial_close(int fd)
{
	if (fd < 0)
		return -1;
		
	//恢复旧的端口参数
	tcsetattr(fd, TCSANOW, &serial_oldtio);
		
	//关闭串口
	if (!close(fd))
	{
		return 0;
	}	
	
	return -1;
}