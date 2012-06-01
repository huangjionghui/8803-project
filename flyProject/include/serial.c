#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>


#include <pthread.h>
#include <sys/times.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/types.h>
#include <cutils/atomic.h>   
#include <hardware/hardware.h>  

#include "serial.h"


//�������
struct termios serial_oldtio;
struct termios serial_newtio;
fd_set read_fds;
	
 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 static int set_serial_opts(int fd, int nSpeed, int nBits, char nEvent, int nStop)
 {
	int ret = -1;
	
	if (fd < 0)
		return -1;
	
	//��Ŀǰ�ն˻��������浽�ṹ�����flydvd_oldtio��
	if (tcgetattr(fd, &serial_oldtio) != 0)
	{
		goto fail;
	}
	
	//�ṹ��serial_newtio��0
	memset(&serial_newtio, 0, sizeof(serial_newtio));
			
	//���ô��ڲ���
	serial_newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	serial_newtio.c_oflag &= ~OPOST;
	serial_newtio.c_iflag &= ~(ICRNL | IGNCR);
	serial_newtio.c_cflag |= CLOCAL | CREAD;           //����������
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
	
	switch (nSpeed)
	{
		case 9600:
			cfsetispeed(&serial_newtio, B9600);
			cfsetospeed(&serial_newtio, B9600);
			break;
		
		case 19200:
			cfsetispeed(&serial_newtio, B19200);
			cfsetospeed(&serial_newtio, B19200);
			break;
			
		case 38400:
			cfsetispeed(&serial_newtio, B38400);
			cfsetospeed(&serial_newtio, B38400);
			break;
		
		case 57600:
			cfsetispeed(&serial_newtio, B57600);
			cfsetospeed(&serial_newtio, B57600);
			break;
			
		case 115200:
			cfsetispeed(&serial_newtio, B115200);
			cfsetospeed(&serial_newtio, B115200);
			break;
			
		default:
			cfsetispeed(&serial_newtio, B9600);
			cfsetospeed(&serial_newtio, B9600);
			break;
	}
	
	if (nStop == 1)
		serial_newtio.c_cflag &= ~CSTOPB;
	else if (nStop == 2)
		serial_newtio.c_cflag = CSTOPB;
		
	//ˢ���ڴ����е������������
	tcflush(fd, TCIFLUSH);
			
	//���õ�ǰ�ĵĴ��ڲ���Ϊflydvd_newtio
	if (tcsetattr(fd, TCSANOW, &serial_newtio) != 0)
	{
		goto fail;
	}
	
	ret = 0;
			
	fail:
		return ret;
 }
 
 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 int serial_write(int fd, unsigned char *buf, int len)
 {
	int ret = -1;
	
	if (fd < 0)
		return ret;
		
	//д������
	ret = write(fd, buf, len);
	
	return ret;
 }
 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 long serial_read(int fd, unsigned char *buf, int len)
 {
	long ret = -1;
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
				//��ȡ����
				ret = read(fd, buf, len);
			}
			break;	
		}
		
	return ret;
 }
 
 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
int serial_open(void)
{
	int fd = -1;
	int res = 0;
	pthread_t thread_id;
	
	fd = open(SERIAL_NAME,O_RDWR | O_NOCTTY | O_NDELAY);
	//fd = open("/dev/ttyS0",O_RDWR | O_NOCTTY | O_NDELAY);
	//fd = open("/dev/ttyS0",O_RDWR 
	//				| O_NOCTTY  | O_NDELAY | O_NONBLOCK);
	if (fd > 0)     
	{  
		if (set_serial_opts(fd, SERIAL_BAUDRATE, 8, 'N', 1) == -1)
		{
			return -1;
		}
		
		FD_ZERO(&read_fds);
	}

	return fd;
}
 /********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
int serial_close(int fd)
{
	if (fd < 0)
		return -1;
		
	//�ָ��ɵĶ˿ڲ���
	tcsetattr(fd, TCSANOW, &serial_oldtio);
		
	//�رմ���
	if (!close(fd))
	{
		return 0;
	}	
	
	return -1;
}