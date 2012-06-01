#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <linux/device.h>
#include <linux/time.h>
#include <linux/kthread.h>

#include "FlyDebug.h"

struct fly_debug_info *pGlobalDebugInfo = NULL;


static void dealDataFromUser(struct fly_debug_info *pDebugInfo, BYTE *buf, UINT len)
{
	printk("%s", buf);
}

static void  initDebugStructInfo(struct fly_debug_info *pDebugInfo)
{

}

static void freeDebugStruct(struct fly_debug_info *pDebugInfo, BYTE freeStep)
{
	//释放设备结构体内存
	switch (freeStep)
	{
		case 0:
			if (pDebugInfo)
			{
				kfree(pDebugInfo);
				pDebugInfo = pGlobalDebugInfo = NULL;
			}
		
		case 1:
			break;
		
		default:
			break;
	
	}
}


static int debug_open(struct inode *inode, struct file *filp)
{
	struct fly_debug_info *pDebugInfo = pGlobalDebugInfo;
	
	//将设备结构体指针赋值给文件私有数据指针
  	filp->private_data = pGlobalDebugInfo;
	
	printk("FlyDebug open OK!\n");
	return 0;
}

static ssize_t debug_write(struct file *filp, const char *buffer, size_t count, loff_t * ppos)
{
	//获得设备结构体指针
	struct fly_debug_info *pDebugInfo = filp->private_data; 
	BYTE localData[256];
	BYTE *pData = NULL;
	BOOL bNewLarge = FALSE;

	if (pDebugInfo == NULL)
	{
		printk("debugCtl_write pDebugInfo is NULL");
		return -1;
	}
	
	if (count < 256)
	{
		pData = localData;
	}
	else
	{
		pData = (BYTE *)kmalloc(count+1, GFP_KERNEL);
		bNewLarge = TRUE;
	}

	//获得用户空间的数据
	if (copy_from_user(pData, buffer, count))
	{
		printk("copy data from user error");
		return -EFAULT;
	}
	else
	{
		pData[count] = '\0';
		dealDataFromUser(pDebugInfo,pData,count);
	}

	if (bNewLarge)
	{
		kfree(pData);
	}

	return count;
}

static ssize_t debug_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	//获得设备结构体指针
	struct fly_debug_info *pDebugInfo = filp->private_data; 
	if (pDebugInfo == NULL)
	{
		printk("debug_read pDebugInfo is NULL");
		return -1;
	}
			
    return 0;
		
}

static int debug_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	
	//获得设备结构体指针
	struct fly_debug_info *pDebugInfo = filp->private_data; 
	if (pDebugInfo == NULL)
	{
		printk("debug_ioctl pDebugInfo is NULL");
		return -1;
	}
	
	return 0;
}

static int debug_release(struct inode *inode, struct file *filp)
{

	printk("FlyDebug close OK!\n");
	return 0;
}

static struct file_operations debug_fops = {
	.owner	 =   THIS_MODULE,
	.open    =   debug_open,
	.write   =   debug_write,
	.read    =   debug_read,
	.ioctl   =   debug_ioctl,
	.release =   debug_release,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &debug_fops,
};

static int __init debug_init(void)
{
	int ret = -1;

	// 动态申请设备结构体的内存
  	pGlobalDebugInfo = (struct fly_debug_info *)kmalloc(sizeof(struct fly_debug_info), GFP_KERNEL);
  	if (pGlobalDebugInfo == NULL)
  	{
		printk("kmalloc error!\n");
    	return ret;
  	}

	//注册设备
	ret = misc_register(&misc);
	if (ret)
	{
		//注册失败，释放设备结构体内存
		freeDebugStruct(pGlobalDebugInfo,0);
		return ret;
	}
	
	//结构体初始化付值
  	initDebugStructInfo(pGlobalDebugInfo);
	
	printk("FlyDebug initialization %s %s \n", __TIME__, __DATE__);
	
	return ret;
}

static void __exit debug_exit(void)
{

	freeDebugStruct(pGlobalDebugInfo,0);
	
	//注销设备
	misc_deregister(&misc);  

	printk("FlyDebug unload\n");
}

module_init(debug_init);
module_exit(debug_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");
