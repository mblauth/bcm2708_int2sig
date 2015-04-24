obj-m := bcm2708_int2sig.o

all:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(KERNEL_PATH) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=$(CCPREFIX) -C $(KERNEL_PATH) M=$(PWD) clean
