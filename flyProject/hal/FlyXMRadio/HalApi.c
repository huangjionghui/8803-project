#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>


#include <sys/select.h>
#include <assert.h>
#include <sys/types.h>
#include <cutils/atomic.h>   
#include <hardware/hardware.h>  

#include "FlyXMRadio.h"
#include "HalApi.h"
#include "../../include/global.h"

//static void flyDestroy(struct global_control_device_t *dev);

 /********************************************************************************
 **函数名称：fly_write()函数
 **函数功能：写数据进设备文件
 **函数参数：
 **返 回 值：成功返回实际写入的数据，失失败返回-1
 **********************************************************************************/
static INT fly_write(BYTE *write_buf, UINT buf_len)  
{  
	INT ret = -1;
		
	if (buf_len > 0)
	{
		//指令处理
		flyCommandProcessor(write_buf, buf_len);
		
		ret = buf_len;
	}
	
	
	return ret;  
} 

 /********************************************************************************
 **函数名称：fly_read()函数
 **函数功能：读出数据
 **函数参数：
 **返 回 值：成功返回实际读得的数据，失败返回-1
 **********************************************************************************/
static INT fly_read(BYTE *read_buf, UINT buf_len)  
{  
	INT i=0;
	BYTE *SendBuf = (BYTE *)read_buf;
	
	return flyReadData(SendBuf, buf_len);
}

 /********************************************************************************
 **函数名称：fly_ioctl()函数
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
static INT fly_ioctl(INT cmd, long *arg)
{

	return 0;
} 

 /********************************************************************************
 **函数名称：fly_close()函数
 **函数功能：关闭函数
 **函数参数：
 **返 回 值：
 **********************************************************************************/
static INT fly_close(void)  
{  
	INT ret = -1;
	
    ret = flyCloseDevice();
	/*
	if (ret == 0)
	{
		flyDestroy();
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
static void flyInitFunctionAPI(struct global_control_device_t *dev, BOOL bEnable)
{
	assert(dev != NULL);
	
	if (bEnable)
	{
		//函数的入口地址
		dev->global_read   = fly_read;  
		dev->global_write  = fly_write;         
		dev->global_ioctl  = fly_ioctl;
		dev->global_close  = fly_close; 
	}
	else
	{        
		dev->global_read   = NULL;
		dev->global_write  = NULL; 
		dev->global_ioctl  = NULL;
		dev->global_close  = NULL; 
	}
}
/********************************************************************************
 **函数名称：
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 /*
static void flyDestroy(struct global_control_device_t *dev)
{
	assert(dev != NULL);
	
	flyInitFunctionAPI(dev,DISANBLE);
	free (dev);
	dev = NULL;
	
	flyDestroyDeviceStruct();
}
*/
/********************************************************************************
 **函数名称：flydvd_open_hardware_device（）函数
 **函数功能：打开设备
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 static INT fly_open_hardware_device(struct global_control_device_t *dev)
 {
	return flyOpenDevice();
 }
 
 /********************************************************************************
 **函数名称：flydvd_device_close（）函数
 **函数功能：关闭函数，释放内存
 **函数参数：
 **返 回 值：
 **********************************************************************************/
INT fly_device_close(struct hw_device_t* device)  
{  
    struct global_control_device_t* ctx = (struct global_control_device_t*)device;  
	
    if (ctx) 
    {  
        free(ctx);  
    }  
    
    //DLOGI("close OK");
	
    return 0;  
} 

/********************************************************************************
 **函数名称：flydvd_device_open（）函数
 **函数功能：
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 static INT fly_device_open(const struct hw_module_t* module, 
									const BYTE* name, struct hw_device_t** device)   
{  
    struct global_control_device_t *dev;  
		
	//为 global_control_device_t 结构分配内存 
    dev = (struct global_control_device_t *)malloc(sizeof(*dev));
	if (dev == NULL)
	{
		//DLOGE("dev malloc error");
		return -1;
	}
    memset(dev, 0, sizeof(*dev));  

	dev->common.tag =  HARDWARE_DEVICE_TAG;  
    dev->common.version = 0;  
    dev->common.module = module;  
    
	//实例化支持的操作 
	dev->common.close = fly_device_close;  
	
	//将实例化后的flydvd_control_device_t地址返回给jni层
    *device = &dev->common;    

	//初始化函数的接口
	flyInitFunctionAPI(dev, ENABLE);
	
	//初始化结构体成员
	flyInitDeviceStruct();

	//打开设备
	return fly_open_hardware_device(dev);
} 

/********************************************************************************
 **函数名称：hw_module_methods_t 结构体
 **函数功能：提供一个OPEN方法
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 static struct hw_module_methods_t device_module_methods = {  
    open:fly_device_open  
};

 
 /********************************************************************************
 **函数名称：fly_module_t结构体
 **函数功能：定义这个对象等于向系统注册了一个ID为SERIAL_HARDWARE_MODULE_ID的stub。
			 注意这里HAL_MODULE_INFO_SYM的名称不能改。
 **函数参数：
 **返 回 值：
 **********************************************************************************/
 const struct global_module_t HAL_MODULE_INFO_SYM = {  

    common: {  
        tag: HARDWARE_MODULE_TAG,  
        version_major: VERSION_MAJOR,  
        version_minor: VERSION_MINOR,  
        id: HAL_DEVICE_NAME_XMRADIO,  
        name:   "flyXMRadio Stub",  
        author: "FlyAudio",  
        methods: &device_module_methods,  //实现了一个open的方法供jni层调用，   
                                         //从而实例化global_control_device_t   
    }  
};