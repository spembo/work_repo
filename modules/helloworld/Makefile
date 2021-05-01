#obj-m := robot_app.o

#ARCH := arm
#KERN_DIR := /home/sam/bbb_ubuntu/bb-kernel/KERNEL
#HOST_KERN_DIR := /lib/modules/$(shell uname -r)/build/


CC := /home/sam/bbb_ubuntu/bb-kernel/dl/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++
LDFLAGS += -Wall -O0
 
all: helloworld

helloworld: helloworld.o
	$(CC) $(LDFLAGS) helloworld.o -o helloworld
	
helloworld.o: helloworld.c
	$(CC) $(CFLAGS) -c helloworld.c
	
clean:
	rm *.o helloworld
