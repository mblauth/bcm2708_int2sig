obj-m := bcm2708_int2sig.o

all:
	@$(MAKE) ARCH=arm CROSS_COMPILE=${CCPREFIX} -C /local/blauth/rpi-kernel/linux M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=${CCPREFIX} -C /local/blauth/rpi-kernel/linux M=$(PWD) clean
