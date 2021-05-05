# name of object file
obj-m := tb6612.o 

KDIR := /home/${USER}/bbb_ubuntu/bb-kernel/KERNEL
PWD := $(shell pwd)
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

all:
	make  ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
				-C $(KDIR) M=$(PWD) modules

clean:
	rm -f *.ko *.ko.cmd *.mod *.mod.c *.mod.cmd *.mod.o *.mod.o.cmd *.mod.o *.mod.o.cmd *.o all
