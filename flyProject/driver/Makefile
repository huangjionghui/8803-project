PWD := $(shell pwd)
KERNELDIR = /home/android2.3_newdesk/sDK_2306_20110812/kernel
CROSS_COMPILE = /home/flyaudio_android/release_fae_version/toolchain/arm-eabi-4.4.0/bin/arm-eabi-
CC = $(CROSS_COMPILE)gcc

obj-m += FlyADC/
obj-m += FlyDebug/
obj-m += FlyAudio/
obj-m += FlyHardware/
obj-m += FlyI2C/
obj-m += FlyMmap/

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
	
rm-dirs  := $(addprefix $(PWD)/,.tmp_versions)
rm-files := $(addprefix $(PWD)/,modules.order Module.symvers)
.PHONY:clean
clean:
	@-rm -f  $(rm-files)
	@-rm -fr $(rm-dirs)        
	@find \
		\( -name '*.[oas]' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.gcno' \) -type f -print | xargs rm -f
	@rm -fr `find -name modules.order`