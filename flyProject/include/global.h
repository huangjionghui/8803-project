#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <cutils/log.h>  
#include "types_def.h"



struct global_module_t{
	struct hw_module_t common;
};

struct global_control_device_t{
	struct hw_device_t common;
	
	int (*global_read )(unsigned char *read_buf,  unsigned int buf_len);
	int (*global_write)(unsigned char *write_buf, unsigned int buf_len);
	int (*global_ioctl)(int cmd, long *arg);
	int (*global_close)();
};

struct global_control_context_t{
	struct global_control_device_t device;
};

#endif