#GNU_PATH = /home/marshall/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin
SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)
#OBJS = writer.o
#ARCH = arm64
#CROSS_COMPILE = $(GNU_PATH)/aarch64-none-linux-gnu-
#CROSS_COMPILE = aarch64-none-linux-gnu-
#CFLAGS = -Wall
#CC = gcc

# ==== Use this to compile natively ====
all:
$(TARGET) : $(OBJS)
#	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS)

#$(OBJS):
#	$(CC) -c $(SRC)

# ==== Use this to compile for arm64 ====
#all: 
#	aarch64-none-linux-gnu-gcc $(SRC) -o $(TARGET)


#	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)
#	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

#all: 

#writer:
#	$(CC) $(SRC) -o $(TARGET)

#writer.o:
$(OBJS):
	$(CC) -c $(SRC)

clean:
	-rm -f *.o $(TARGET) *.elf *.map