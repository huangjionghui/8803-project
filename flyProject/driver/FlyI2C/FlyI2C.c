#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
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
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <asm/system.h>
#include <linux/stat.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/smp_lock.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <linux/types.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()、kthread_run()
#include <linux/input.h>

#include "FlyI2C.h"

struct i2c_api
{
	struct list_head list;
	struct i2c_client *client;
};

static LIST_HEAD(i2c_list);
static DEFINE_SPINLOCK(i2c_list_lock);

#ifdef USE_I2C_LOCK
static struct mutex i2c_lock;
#endif

static struct file_operations i2c_fops = {
	.owner   =   THIS_MODULE,
};

static struct miscdevice i2c_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,			
	.fops  = &i2c_fops,				
};

static struct i2c_api *get_i2c_bus(int bus_id)
{
	//由于在系统中可能存在多个adapter,因为将每一条I2C总线对应一个编号,下文中称为I2C总线号.
	//这个总线号的PCI中的总线号不同.它和硬件无关,只是软件上便于区分而已
	//对于i2c_add_adapter()而言,它使用的是动态总线号,即由系统给其分析一个总线号

	struct i2c_api *i2c_api;

	spin_lock(&i2c_list_lock);
	list_for_each_entry(i2c_api, &i2c_list, list)
	{
		if (i2c_api->client->adapter->nr == bus_id)
			goto found;
	}
	i2c_api = NULL;

found:
	spin_unlock(&i2c_list_lock);
	return i2c_api;
}

static int i2c_do_transfer(int bus_id, char chip_addr, unsigned int sub_addr, int mode,
						   char *buf, unsigned int size)
{
#define I2C_API_XFER_MODE_SEND            0x0 /* standard send */
#define I2C_API_XFER_MODE_RECV            0x1 /* standard receive */
#define I2C_API_XFER_MODE_SEND_NO_SUBADDR 0x2 /* send with no sub address */
#define I2C_API_XFER_MODE_RECV_NO_SUBADDR 0x3 /* receive with no sub address */

#define I2C_API_XFER_MODE_RECV_SAF7741    0x4 /* receive for SAF7741 */
#define I2C_API_XFER_MODE_SEND_TEF7000    0x5 /* send for TEF7000 */
#define I2C_API_XFER_MODE_RECV_TEF7000    0x6 /* receive for TEF7000 */

	int ret = 0;
	struct i2c_api *i2c_api ;

#ifdef USE_I2C_LOCK
	mutex_lock(&i2c_lock);
#endif

	i2c_api = get_i2c_bus(bus_id);

	if (!i2c_api)
		return -ENODEV;

	i2c_api->client->addr = chip_addr;

	switch (mode)
	{
	case I2C_API_XFER_MODE_SEND:
		{
			struct i2c_adapter *adap = i2c_api->client->adapter;
			struct i2c_msg msg;
			int ret;

			msg.addr = i2c_api->client->addr;
			msg.flags = 0;
			msg.len = size;
			msg.buf = buf;

			ret = i2c_transfer(adap, &msg, 1);

			break;
		}

	case I2C_API_XFER_MODE_RECV:
		{
			struct i2c_adapter *adap = i2c_api->client->adapter;
			struct i2c_msg msg[2];
			int ret;
			char subaddr;

			subaddr = sub_addr & 0xff;

			msg[0].addr = i2c_api->client->addr;
			msg[0].flags = 0;
			msg[0].len = 1;
			msg[0].buf = &subaddr;


			msg[1].addr = i2c_api->client->addr;
			msg[1].flags = I2C_M_RD;
			msg[1].len = size;
			msg[1].buf = buf;


			ret = i2c_transfer(adap, msg, 2);
			break;
		}

	case I2C_API_XFER_MODE_SEND_NO_SUBADDR:
		ret = i2c_master_send(i2c_api->client, buf, size);
		break;

	case I2C_API_XFER_MODE_RECV_NO_SUBADDR:
		//  ret = i2c_master_recv(i2c_api->client, buf, size);
		{
			struct i2c_adapter *adap = i2c_api->client->adapter;
			struct i2c_msg msg[2];
			int ret;

			msg[0].addr = i2c_api->client->addr;
			msg[0].flags = I2C_M_RD;
			msg[0].len = size;
			msg[0].buf = buf;
			ret = i2c_transfer(adap, msg, 1);
			break;
		}
		break;

	case I2C_API_XFER_MODE_RECV_SAF7741:
		{
			struct i2c_adapter *adap = i2c_api->client->adapter;
			struct i2c_msg msg[2];
			int ret;
			char SubAddr[3];
			SubAddr[0] = (sub_addr>>16) & 0xff;
			SubAddr[1] = (sub_addr>>8) & 0xff;
			SubAddr[2] = sub_addr & 0xff;

			msg[0].addr = i2c_api->client->addr;
			msg[0].flags = 0;
			msg[0].len = 3;
			msg[0].buf = SubAddr;

			msg[1].addr = i2c_api->client->addr;
			msg[1].flags = I2C_M_RD;
			msg[1].len = size;
			msg[1].buf = buf;

			ret = i2c_transfer(adap, msg, 2);
			break;
		}
		break;

#define SAF7741_I2C_ADDR_FOR_TEF7000  0x38

	case I2C_API_XFER_MODE_SEND_TEF7000:
		{
			struct i2c_adapter *adap = i2c_api->client->adapter;
			struct i2c_msg msg[2];
			int ret;
			char SubAddr[3];
			SubAddr[0] = 0x00;
			SubAddr[1] = 0xff;
			SubAddr[2] = 0xff;

			msg[0].addr = SAF7741_I2C_ADDR_FOR_TEF7000;
			msg[0].flags = 0;
			msg[0].len = 3;
			msg[0].buf = SubAddr;

			msg[1].addr = i2c_api->client->addr;
			msg[1].flags = 0;
			msg[1].len = size;
			msg[1].buf = buf;

			ret = i2c_transfer(adap, msg, 2);
			break;
		}
		break;

	case I2C_API_XFER_MODE_RECV_TEF7000:
		{
			struct i2c_adapter *adap = i2c_api->client->adapter;
			struct i2c_msg msg[4];
			int ret;
			char SubAddr[3];
			char SubAddr_TEF7000;
			SubAddr[0] = 0x00;
			SubAddr[1] = 0xff;
			SubAddr[2] = 0xff;

			SubAddr_TEF7000 = sub_addr & 0xff;

			msg[0].addr = SAF7741_I2C_ADDR_FOR_TEF7000;
			msg[0].flags = 0;
			msg[0].len = 3;
			msg[0].buf = SubAddr;

			msg[1].addr = i2c_api->client->addr;
			msg[1].flags = 0;
			msg[1].len = 1;
			msg[1].buf = &SubAddr_TEF7000;

			msg[2].addr = SAF7741_I2C_ADDR_FOR_TEF7000;
			msg[2].flags = 0;
			msg[2].len = 3;
			msg[2].buf = SubAddr;

			msg[3].addr = i2c_api->client->addr;
			msg[3].flags = I2C_M_RD;
			msg[3].len = size;
			msg[3].buf = buf;

			ret = i2c_transfer(adap, msg, 4);
			break;
		}
		break;

	default:
		return -EINVAL;
	}
#ifdef USE_I2C_LOCK
	mutex_unlock(&i2c_lock);
#endif
	return ret;
}

//地址发送的时候还要左移一位
int I2C_Send_Normal(int bus_id, char chip_addr, char *buf, unsigned int size)
{
	return i2c_do_transfer(bus_id, chip_addr, 0, I2C_API_XFER_MODE_SEND, buf, size);
}

int I2C_Rec_Normal(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
	return i2c_do_transfer(bus_id, chip_addr, sub_addr, I2C_API_XFER_MODE_RECV, buf, size);
}

int I2C_Rec_No_SubAddr(int bus_id, char chip_addr, char *buf, unsigned int size)
{
	return i2c_do_transfer(bus_id, chip_addr, 0, I2C_API_XFER_MODE_RECV_NO_SUBADDR, buf, size);
}

int I2C_Rec_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
	return i2c_do_transfer(bus_id, chip_addr, sub_addr, I2C_API_XFER_MODE_RECV_SAF7741, buf, size);
}

int I2C_Send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
	return i2c_do_transfer(bus_id, chip_addr, sub_addr, I2C_API_XFER_MODE_SEND_TEF7000, buf, size);
}

int I2C_Rec_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
	return i2c_do_transfer(bus_id, chip_addr, sub_addr, I2C_API_XFER_MODE_RECV_TEF7000, buf, size);
}


static int __init i2c_init(void)
{
	//注册设备
	int ret = misc_register(&i2c_misc);

	printk("FlyADC.ko initialized %s  %s \n",__TIME__,__DATE__);

#ifdef USE_I2C_LOCK
	mutex_init(&i2c_lock);
#endif

	return 0 ;
}

static void __exit i2c_exit(void)
{
	//注销设备
	misc_deregister(&i2c_misc);

	printk("FlyI2C exit\n");
}

MODULE_AUTHOR("FlyAudio.Inc");
MODULE_LICENSE("GPL");

module_init(i2c_init);
module_exit(i2c_exit);

EXPORT_SYMBOL_GPL(I2C_Send_Normal);
EXPORT_SYMBOL_GPL(I2C_Rec_Normal);
EXPORT_SYMBOL_GPL(I2C_Rec_No_SubAddr);
EXPORT_SYMBOL_GPL(I2C_Rec_SAF7741);
EXPORT_SYMBOL_GPL(I2C_Send_TEF7000);
EXPORT_SYMBOL_GPL(I2C_Rec_TEF7000);


