
OBJS := robot_app
CC := /home/sam/bbb_ubuntu/bb-kernel/dl/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
LDFLAGS += -Wall -O0
 
all: $(OBJS)

$(OBJS): $(OBJS).o
	$(CC) $(LDFLAGS) $(OBJS).o -o $(OBJS)
	
$(OBJS).o: $(OBJS).c
	$(CC) $(CFLAGS) -c $(OBJS).c
	
clean:
	rm *.o $(OBJS)
