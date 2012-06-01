
#ifndef _FLY_SOC__
#define _FLY_SOC__

//#define SOC_COMPILE

#ifdef SOC_COMPILE
#include "lidbg.h"
#else
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <linux/irq.h>
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
#include <linux/kthread.h> //kthread_create()¡¢kthread_run()
#include <linux/input.h>
#endif


extern unsigned int FLAG_FOR_15S_OFF;

//IO
//group - 0-A;1-B;2-C;3-D;4-E;5-F;6-G;
//flag - GPIO_PULLUP/GPIO_PULLDOWN/GPIO_PULL_DISABLE
void  SOC_IO_Output(u32 group, u32 index, bool status);
bool  SOC_IO_Input(u32 group, u32 index, u32 flag);


//IO_IRQ
#if 0 //#include <linux/irq.h>
//interrupt_type
#define IRQ_TYPE_NONE		0x00000000	/* Default, unspecified type */
#define IRQ_TYPE_EDGE_RISING	0x00000001	/* Edge rising type */
#define IRQ_TYPE_EDGE_FALLING	0x00000002	/* Edge falling type */
#define IRQ_TYPE_EDGE_BOTH (IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH	0x00000004	/* Level high type */
#define IRQ_TYPE_LEVEL_LOW	0x00000008	/* Level low type */
#endif

#ifndef SOC_COMPILE
typedef irqreturn_t (*pinterrupt_isr)(int irq, void *dev_id);
#endif

bool SOC_IO_ISR_Add(u32 irq, u32 interrupt_type, pinterrupt_isr func, void *dev);//set port as input first
bool SOC_IO_ISR_Enable(u32 irq);
bool SOC_IO_ISR_Disable(u32 irq);
bool SOC_IO_ISR_Del (u32 irq);


//AD
bool SOC_ADC_Get (u32 channel , u32 *value);


//KEY
#if 0 //linux/input.h
//key_value

KEY_MENU,   KEY_HOME,  KEY_BACK,
        KEY_DOWN,   KEY_UP,  KEY_RIGHT, KEY_LEFT,
        KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_PAUSE, KEY_MUTE,
        KEY_POWER, KEY_SLEEP, KEY_WAKEUP

#endif
#ifndef SOC_COMPILE
        //type
#define KEY_RELEASED    (0)
#define KEY_PRESSED      (1)
#define KEY_PRESSED_RELEASED   ( 2)
#endif
        void SOC_Key_Report(u32 key_value, u32 type);


//BL
//level : 0~255
void SOC_BL_Set( u32 level);


//PWR
void SOC_PWR_ShutDown(void);//power-down

#endif
