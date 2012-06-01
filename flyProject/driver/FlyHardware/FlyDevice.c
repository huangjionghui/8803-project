#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include "FlyDevice.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");

struct platform_device *fly_device;

static int __init fly_device_init(void)
{
	int ret = 0;
	
	//分配结构体
	fly_device = platform_device_alloc(DEVICE_NAME,-1);
	
	//注册设备
	ret = platform_device_add(fly_device);
	if (ret)
	{
		platform_device_put(fly_device);
		printk("fly device register success\n");
	}
	
	printk("fly device register failed\n");
	return ret;
}

static void fly_device_exit(void)
{
	printk("fly device unregister\n");
	platform_device_unregister(fly_device);
}

module_init(fly_device_init);
module_exit(fly_device_exit);

