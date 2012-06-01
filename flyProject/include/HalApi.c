#include <fcntl.h>  
#include <errno.h>  
#include <termios.h>
#include <stdio.h>


#include <sys/select.h>
#include <assert.h>
#include <sys/types.h>
#include <cutils/atomic.h>   
#include <hardware/hardware.h>  

#include "HalApi.h"
#include "../../include/global.h"

//static void flyDestroy(struct global_control_device_t *dev);

 /********************************************************************************
 **�������ƣ�fly_write()����
 **�������ܣ�д���ݽ��豸�ļ�
 **����������
 **�� �� ֵ���ɹ�����ʵ��д������ݣ�ʧʧ�ܷ���-1
 **********************************************************************************/
static INT fly_write(BYTE *write_buf, UINT buf_len)  
{  
	INT ret = -1;
		
	if (buf_len > 0)
	{
		//ָ���
		flyCommandProcessor(write_buf, buf_len);
		
		ret = buf_len;
	}
	
	
	return ret;  
} 

 /********************************************************************************
 **�������ƣ�fly_read()����
 **�������ܣ���������
 **����������
 **�� �� ֵ���ɹ�����ʵ�ʶ��õ����ݣ�ʧ�ܷ���-1
 **********************************************************************************/
static INT fly_read(BYTE *read_buf, UINT buf_len)  
{  
	BYTE *SendBuf = (BYTE *)read_buf;
	
	return flyReadData(SendBuf, buf_len);
}

 /********************************************************************************
 **�������ƣ�fly_ioctl()����
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
static INT fly_ioctl(INT cmd, long *arg)
{

	return 0;
} 

 /********************************************************************************
 **�������ƣ�fly_close()����
 **�������ܣ��رպ���
 **����������
 **�� �� ֵ��
 **********************************************************************************/
static INT fly_close()  
{  
	INT ret = -1;
	assert(dev != NULL);
	
    ret = flyCloseDevice();

	return ret;
}

/********************************************************************************
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
static void flyInitFunctionAPI(struct global_control_device_t *dev, BOOL bEnable)
{
	assert(dev != NULL);
	
	if (bEnable)
	{
		//��������ڵ�ַ
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
 **�������ƣ�
 **�������ܣ�
 **����������
 **�� �� ֵ��
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
 **�������ƣ�flydvd_open_hardware_device��������
 **�������ܣ����豸
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 static INT fly_open_hardware_device(struct global_control_device_t *dev)
 {
	assert(dev != NULL);
	
	return flyOpenDevice();
 }
 
 /********************************************************************************
 **�������ƣ�flydvd_device_close��������
 **�������ܣ��رպ������ͷ��ڴ�
 **����������
 **�� �� ֵ��
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
 **�������ƣ�flydvd_device_open��������
 **�������ܣ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
static INT fly_device_open(const struct hw_module_t* module, const char* id, struct hw_device_t** device)   
{  
    struct global_control_device_t *dev;  
		
	//Ϊ global_control_device_t �ṹ�����ڴ� 
    dev = (struct global_control_device_t *)malloc(sizeof(*dev));
	if (dev == NULL)
	{
		//DLOGE("dev malloc error");
		return -1;
	}
    memset(dev, 0, sizeof(*dev));  

	dev->common.tag =  HARDWARE_DEVICE_TAG;  
    dev->common.version = 0;  
    dev->common.module = (struct hw_module_t*)module;  
    
	//ʵ����֧�ֵĲ��� 
	dev->common.close = fly_device_close;  
	
	//��ʵ�������flydvd_control_device_t��ַ���ظ�jni��
    *device = &dev->common;    

	//��ʼ�������Ľӿ�
	flyInitFunctionAPI(dev, ENABLE);
	
	//��ʼ���ṹ���Ա
	flyInitDeviceStruct();

	//���豸
	return fly_open_hardware_device(dev);
} 

/********************************************************************************
 **�������ƣ�hw_module_methods_t �ṹ��
 **�������ܣ��ṩһ��OPEN����
 **����������
 **�� �� ֵ��
 **********************************************************************************/
static struct hw_module_methods_t device_module_methods = {  
    open:fly_device_open
};

 
 /********************************************************************************
 **�������ƣ�fly_module_t�ṹ��
 **�������ܣ�����������������ϵͳע����һ��IDΪSERIAL_HARDWARE_MODULE_ID��stub��
			 ע������HAL_MODULE_INFO_SYM�����Ʋ��ܸġ�
 **����������
 **�� �� ֵ��
 **********************************************************************************/
 const struct global_module_t HAL_MODULE_INFO_SYM = {  

    common: {  
        tag: HARDWARE_MODULE_TAG,  
        version_major: VERSION_MAJOR,  
        version_minor: VERSION_MINOR,  
        id: LOCAL_HAL_ID,  
        name:   LOCAL_HAL_NAME,  
        author: LOCAL_HAL_AUTOHR,  
        methods: &device_module_methods,  //ʵ����һ��open�ķ�����jni����ã�   
                                         //�Ӷ�ʵ����global_control_device_t   
    }  
};