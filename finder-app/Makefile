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
#.PHONY : all cleantarget clean
.PHONY : all
all : $(TARGET)
	echo "RUNNING ALL"

#.PHONY : cleantarget
#cleantarget:
#	echo "CLEANTARGET..."
#	-rm -f $(TARGET).o $(TARGET) $(TARGET).elf $(TARGET).map
#	echo "CLEANTARGET DONE"

$(TARGET) : $(OBJS)
	echo "BUILDING $(TARGET)..."
#	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS)
	echo "BUILDING $(TARGET) DONE"



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
#$(OBJS):
#	$(CC) -c $(SRC)

.PHONY: clean # redundant, but makes builds more effecient
clean:
	echo "CLEANING..."
	-rm -f *.o $(TARGET) *.elf *.map
	echo "CLEANING DONE"