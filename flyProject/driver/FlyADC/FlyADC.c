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
#include <linux/sched.h> 
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/interrupt.h>

#include <mach/TCC88xx_Structures.h>
#include <mach/TCC88xx_Physical.h>
#include <mach/globals.h>
#include <mach/clock.h>
#include <mach/gpio.h>
#include <linux/clk.h>


#include "FlyADC.h"
#include "../include/driver_def.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("FlyAudio.Inc");

static volatile PTSADC pPTSADC;
static struct clk	*adc_clock;

static u32 adc_select_channel(u32 channel)
{
	u32 ch = 0;
	switch (channel){
		case 0: ch = 0; break;
		case 1: ch = 1; break;
		case 4: ch = 12; break;
		case 5: ch = 13; break;
		case 8: ch = 4;  break;
		case 9: ch = 5;  break;
		case 14: ch = 10; break;
		default: ch = 255;break;
	}
	
	return ch;
}
static u32 get_adc_values(u32 ch)
{
	u32 channel = 0;
	u32 adc_data = 0;
	u32 counts = 0;
	
	channel = adc_select_channel(ch);
	if (channel == 255)
		return 0xFFF;
	
	BITCLR(pPTSADC->ADCCON, Hw2);
	pPTSADC->ADCCON |=  ADC_ASEL(channel)|ADC_EN_ST_EN;
	ndelay(5);
	
	while (pPTSADC->ADCCON & ADC_EN_ST_EN)
	{
		counts++;
		ndelay(3);
		if (counts >= 10)
			break;
	}
	
    while (!(pPTSADC->ADCCON & ADC_E_FLG_END))
	{
		counts++;
		ndelay(3);
		if (counts >= 20)
			break;
	}
	
	adc_data = pPTSADC->ADCDAT0 & 0x00000FFF;
	
	pPTSADC->ADCDAT0 &= 0xFFFFF000; 
	pPTSADC->ADCCON &=  0xFFFFFF86;
    BITSET(pPTSADC->ADCCON, Hw2);
	ndelay(5);
	
	return adc_data;
}

static void __adc_gpio_init(void)
{
	tcc_gpio_config(TCC_GPE(25),GPIO_FN(1) | GPIO_CD(3));
	tcc_gpio_config(TCC_GPE(24),GPIO_FN(1) | GPIO_CD(3));
	gpio_direction_input(TCC_GPE(25));
	gpio_direction_input(TCC_GPE(24));

}
void ADC_Gpio_Init(void)
{
	__adc_gpio_init();
}
EXPORT_SYMBOL_GPL(ADC_Gpio_Init);

static void adc_register_init(void)
{
	__adc_gpio_init();
	pPTSADC->ADCCON = 0x0;
	pPTSADC->ADCTSC = 0x0;
	
	pPTSADC->ADCDAT0 &= 0xFFFFF000; 
	pPTSADC->ADCDAT1 &= 0xFFFFF000; 
	
	pPTSADC->ADCCON = ADC_RES_12_BIT | ADC_PS_EN | ADC_PS_VAL(5) | ADC_STBY_EN;
	pPTSADC->ADCTSC = ADC_XPEN_DIS | ADC_XMEN_DIS | ADC_YPEN_DIS |
			ADC_YMEN_DIS | ADC_PUON_DIS | ADC_AUTO_DIS | ADC_XY_PST_DIS;
	pPTSADC->ADCDLY = ADC_CLK_SRC_RTC | ADC_DELAY(50);
	
	adc_clock = clk_get(NULL, "adc");
	if (!adc_clock) {
		printk("failed to get adc clock source\n");
		return;
	}
	
	clk_enable(adc_clock);
}

void ADC_Get_Values(u32 channel , u32 *value)
{
	*value = get_adc_values(channel);
}
EXPORT_SYMBOL_GPL(ADC_Get_Values);


static struct file_operations adc_fops = {
    .owner   =   THIS_MODULE,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,			
	.fops  = &adc_fops,				
};

static int __init adc_init(void)
{
	int ret = -1;

	//注册设备
	ret = misc_register(&misc);

	gpio_request(TCC_GPE(25), "adc-1");
	gpio_request(TCC_GPE(24), "adc-2");

	pPTSADC = (volatile PPIC)tcc_p2v(HwTSADC_BASE);
	adc_register_init();
	
	printk("FlyADC.ko initialized %s  %s \n",__TIME__,__DATE__);

	return ret;
}

/*
*函数功能：
*/
static void __exit adc_exit(void)
{

	gpio_free(TCC_GPE(25));
	gpio_free(TCC_GPE(24));
	
	//注销设备
	misc_deregister(&misc);
	
	printk("FlyADC.ko unLoad\n");
}

module_init(adc_init);
module_exit(adc_exit);