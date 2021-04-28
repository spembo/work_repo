# name of object file
obj-m := adxl345.o 

KDIR := /home/sam/buildroot/bbb_et1/build/linux-adde2ca9f86797071f6e7b2b9e779fa5e4a8f3cd
PWD := $(shell pwd)
ARCH := arm
CROSS_COMPILE := arm-linux-gnueabihf-

all:
	make  ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
				-C $(KDIR) M=$(PWD) modules

