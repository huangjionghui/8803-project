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
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/poll.h>

#include "FlyMmap.h"
#include "../include/driver_def.h"


FLY_SHARE_MEMORY_COMMON_DATA *pShareMemoryData;

P_FLY_MMAP_INFO pGlobalMmapInfo = NULL;
FLY_GLOBAL_MMAP_STRUCT_INFO GlobalShareMmapInfo;
struct __global_fops global_fops;

UINT32 GetTickCount(void)
{
	struct timespec t_now;

	do_posix_clock_monotonic_gettime(&t_now);
	monotonic_to_bootbased(&t_now);  
	return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;
}

UINT32 forU8ToU32LSB(BYTE *p)
{
	UINT32 iTemp = 0;
	iTemp = (p[3] << 24) + (p[2] << 16) + (p[1] << 8) + p[0];
	return iTemp;
}

void forU32TopU8LSB(UINT32 data,BYTE *p)
{
	p[0] = data;
	data = data >> 8;p[1] = data;
	data = data >> 8;p[2] = data;
	data = data >> 8;p[3] = data;
}

///////////////////////////////////以下驱动之间的事件
void ipcDriverStart(BYTE enumWhatDriver,BYTE enumWhatEvent)
{
	if (enumWhatDriver < IPC_DRIVER_MAX)
	{
		if (global_fops._p_ipcDriver[enumWhatDriver])
		{
			global_fops._p_ipcDriver[enumWhatDriver](enumWhatEvent);
		}
	}
}
///////////////////////////////////以上驱动之间的事件

///////////////////////////////////以下IPC操作
#define SIG_IPC_MSG 44	
#define HAL_SERVICE_NAME "flyaudioservice"
struct task_struct *pUserProcess = NULL;

static void sendEvent(UINT32 sourceEvent)
{
	siginfo_t ipc_info;

	ipc_info.si_signo = SIG_IPC_MSG;
	ipc_info.si_code  = -1;
	ipc_info.si_int   = sourceEvent;

	if (1)
	//if (NULL == pUserProcess)
	{
		for_each_process(pUserProcess)
		{
			if (!strcmp(pUserProcess->comm, HAL_SERVICE_NAME))
			{
				printk("\nsend IPC %ld", sourceEvent);
				send_sig_info(SIG_IPC_MSG, &ipc_info, pUserProcess);
				break;
			}
		}
	}
	else
	{
	}
}

void ipcStartEvent(UINT32 sourceEvent)
{
	sendEvent(sourceEvent);
}

BOOL ipcWhatEventOn(UINT32 sourceEvent)
{
	BOOL bReturn;
	//down_write(&pGlobalMmapInfo->EventTransInfo.SemRWEvent);
	if (GlobalShareMmapInfo.pShareMemoryCommonData->bEventTransSet[sourceEvent])
	{
		bReturn = TRUE;
	}
	else
	{
		bReturn = FALSE;
	}
	//up_write(&pGlobalMmapInfo->EventTransInfo.SemRWEvent);

	return bReturn;
}

void ipcClearEvent(UINT32 sourceEvent)
{
	//down_write(&pGlobalMmapInfo->EventTransInfo.SemRWEvent);
	GlobalShareMmapInfo.pShareMemoryCommonData->bEventTransSet[sourceEvent] = FALSE;
	//up_write(&pGlobalMmapInfo->EventTransInfo.SemRWEvent);
}

///////////////////////////////////以上IPC操作

static int shareMmap_open(struct inode *inode, struct file *filp)
{
	//将设备结构体指针赋值给文件私有数据指针
  	filp->private_data = pGlobalMmapInfo;
	
	printk("shareMmap open OK!\n");
	return 0;
}

static int shareMmap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_pgoff = ((unsigned long)virt_to_phys(GlobalShareMmapInfo.pShareMemoryCommonData)) >> PAGE_SHIFT;  
	
	printk("vma->vm_pgoff = %lx\n",vma->vm_pgoff); 
   
    if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, SHARE_MAP_SIZE,  vma->vm_page_prot))  
	{
		printk("remap_pfn_range error\n");
		return -EAGAIN; 
	}

	return 0;
}

static int shareMmap_release(struct inode *inode, struct file *filp)
{
	struct fly_share_mmap_info *pGlobalMmapInfo = filp->private_data; 
	if (IS_ERR_OR_NULL(pGlobalMmapInfo)){
		printk("shareMmap_release pGlobalMmapInfo is NULL");
		return -1;
	}


	printk("FlyMmap close OK!\n");
	return 0;
}

static struct file_operations shareMmap_fops = {
	.owner	 =   THIS_MODULE,
	.open    =   shareMmap_open,
	.mmap    =   shareMmap_mmap,
	.release =   shareMmap_release,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &shareMmap_fops,
};

static void freeShareMmapStruct(void)
{
	//释放设备结构体内存
	
	if (!IS_ERR_OR_NULL(pGlobalMmapInfo)){
#if !ALLOC_MEMORY_USE_NORMAL
		free_page((ULONG)GlobalShareMmapInfo.pShareMemoryCommonData);
#endif
		kfree(pGlobalMmapInfo); 
		pGlobalMmapInfo = NULL;
	}
}

static int __init shareMmap_init(void)
{
	int ret = -1;
	UINT32 i;

	// 动态申请设备结构体的内存
  	pGlobalMmapInfo = (struct fly_share_mmap_info *)kmalloc(sizeof(struct fly_share_mmap_info), GFP_KERNEL);
  	if (IS_ERR_OR_NULL(pGlobalMmapInfo)){
		printk("kmalloc error!\n");
		freeShareMmapStruct();
    	return ret;
  	}
	memset(pGlobalMmapInfo,0,sizeof(struct fly_share_mmap_info));
	
	memset(&global_fops,0,sizeof(struct __global_fops));
	
#if ALLOC_MEMORY_USE_NORMAL
	pShareMemoryData = (FLY_SHARE_MEMORY_COMMON_DATA *)kmalloc(SHARE_MAP_SIZE, GFP_KERNEL);
	memset(pShareMemoryData,0,SHARE_MAP_SIZE);
	GlobalShareMmapInfo.pShareMemoryCommonData  = (FLY_SHARE_MEMORY_COMMON_DATA *)(PAGE_ALIGN((UINT32)pShareMemoryData));
#else
	pShareMemoryData = (FLY_SHARE_MEMORY_COMMON_DATA *)(__get_free_pages(GFP_KERNEL,2));
	memset(pShareMemoryData,0,PAGE_SIZE*4);
	//GlobalShareMmapInfo.pShareMemoryCommonData  = (FLY_SHARE_MEMORY_COMMON_DATA *)(PAGE_ALIGN((ULONG)pShareMemoryData));
	GlobalShareMmapInfo.pShareMemoryCommonData  = pGlobalMmapInfo->ShareMemoryInfo.pShareMemoryData;
#endif

	//事件
	init_rwsem(&pGlobalMmapInfo->EventTransInfo.SemRWEvent);


	//注册设备
	ret = misc_register(&misc);
	if (ret)
	{
		//注册失败，释放设备结构体内存
		freeShareMmapStruct();
		return ret;
	}

	
	printk("FlyMmap initialization %s %s \n", __TIME__, __DATE__);
	
	return ret;
}

static void __exit shareMmap_exit(void)
{

	freeShareMmapStruct();
	
	//注销设备
	misc_deregister(&misc);  

	printk("FlyMmap unload\n");
}

module_init(shareMmap_init);
module_exit(shareMmap_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");

EXPORT_SYMBOL_GPL(global_fops);
EXPORT_SYMBOL_GPL(GlobalShareMmapInfo);
EXPORT_SYMBOL_GPL(GetTickCount);
EXPORT_SYMBOL_GPL(ipcWhatEventOn);
EXPORT_SYMBOL_GPL(ipcStartEvent);
EXPORT_SYMBOL_GPL(ipcClearEvent);

EXPORT_SYMBOL_GPL(ipcDriverStart);
