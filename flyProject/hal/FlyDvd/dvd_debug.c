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
#include <unistd.h>

#define BUF_LEN 100
#define BUF_LIMIT 256
#define FILE_LIMIT_SIZE 1024*10000*2 //20M
struct dvd_debug_info{   
	int g_fd;
	BYTE temp_buff[BUF_LEN];
	BYTE val_buff[BUF_LIMIT];
};

static struct dvd_debug_info dvd_info={
	.g_fd = -1,
};

static int dvd_is_already_exist(const char *path)
{
	if (access(path, F_OK) == -1)
		return -1;
	else
		return 0;
}
static unsigned long dvd_get_save_file_size(const char *path)
{
	struct stat file_stat;
	if (!stat(path, &file_stat))
		return file_stat.st_size;
	else 
		return 0;
}

static int dvd_open_save_file(const char *path, int flag)
{
	int fd = -1;

	if (fd < 0){
		fd = open(path, flag, 0777);
		if (fd < 0){

			return - 1;
		}
	}
	return fd;
}

static int dvd_close_save_file(int fd)
{
	if (fd > 0){
		if(close(fd) < 0){
			return -1;
		}
	}
	
	return 0;
}
static int dvd_write_to_save_file(int fd, BYTE *buf, unsigned long len)
{
	int ret=-1;
	if (fd > 0){
		ret = write(fd, buf, len);
		if (ret > 0){
			return ret; 
		}
	}
	
	return -1;
}


void dvd_write_save_file(const char *path, BYTE *buf, UINT32 size)
{
	int fd = -1;
	int flag = 0;
	
	if (!dvd_is_already_exist(path)){
		if (dvd_get_save_file_size(path) > FILE_LIMIT_SIZE){
			dvd_close_save_file(dvd_info.g_fd);
			dvd_info.g_fd = -1;
			flag = O_TRUNC|O_CREAT|O_WRONLY;
		}
		else
			flag = O_WRONLY;
	}
	else{
		dvd_info.g_fd = -1;
		flag = O_TRUNC|O_CREAT|O_WRONLY;
	}
	
	if (dvd_info.g_fd < 0)
		dvd_info.g_fd = dvd_open_save_file(path,flag);
		
		
	if (dvd_info.g_fd > 0){
		lseek(dvd_info.g_fd, 0, SEEK_END);
		dvd_write_to_save_file(dvd_info.g_fd,(BYTE*)buf, size);
	}
}
char *dvd_bufToString(BYTE *buf, UINT len)
{
	UINT i; 
	UINT j;
	char str[4];
	
	memset(dvd_info.temp_buff, '\0', sizeof(dvd_info.temp_buff));
	for (i=0,j=0; i<len && j < BUF_LEN - 4; i++,j=j+3)
	{
		snprintf(str, sizeof(str), " %02X", buf[i]);
		memcpy(&dvd_info.temp_buff[j], str, 3);
	}
	dvd_info.temp_buff[j] = '\0';

	return &dvd_info.temp_buff[0];
}

char *dvd_dataToString(UINT32 iData)
{
	memset(dvd_info.temp_buff, '\0', sizeof(dvd_info.temp_buff));
	snprintf(dvd_info.temp_buff, sizeof(dvd_info.temp_buff), " %d", (int)iData);
	return &dvd_info.temp_buff[0];
}

static void dvd_debugString(char *fmt)
{
	if (strlen(fmt) > BUF_LIMIT-10)
		return;
		
	dvd_write_save_file(DVD_DEBUG_FILE_PATH,fmt, strlen(fmt));
}

static void dvd_debugBuf(char *fmt, BYTE *buf, UINT len)
{
	if (strlen(fmt)+len*3 > BUF_LIMIT-10)
		return;
	
	memset(dvd_info.val_buff,'\0',sizeof(dvd_info.val_buff));
	strcpy(dvd_info.val_buff,fmt);
	strcat(dvd_info.val_buff,dvd_bufToString(buf,len));
	dvd_debugString(dvd_info.val_buff);
}
static void dvd_debugOneData(char *fmt, UINT32 data)
{
	if (strlen(fmt) > BUF_LIMIT-10)
		return;
		
	memset(dvd_info.val_buff,'\0',sizeof(dvd_info.val_buff));
	strcpy(dvd_info.val_buff,fmt);
	strcat(dvd_info.val_buff,dvd_dataToString(data));
	dvd_debugString(dvd_info.val_buff);
}
