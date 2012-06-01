#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include "FlyAudioDevice.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc.");

struct platform_device *fly_adudio_device;

static int __init fly_device_init(void)
{
	int ret = 0;
	
	//分配结构体
	fly_adudio_device = platform_device_alloc(DEVICE_NAME,-1);
	
	//注册设备
	ret = platform_device_add(fly_adudio_device);
	if (ret)
	{
		platform_device_put(fly_adudio_device);
		printk("fly aduio device register success\n");
	}
	
	//printk("fly aduio device register failed\n");
	return ret;
}

static void fly_device_exit(void)
{
	printk("fly audio device unregister\n");
	platform_device_unregister(fly_adudio_device);
}

module_init(fly_device_init);
module_exit(fly_device_exit);

