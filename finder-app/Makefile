SRC := writer.c
TARGET = writer
OBJS := $(SRC:.c=.o)

all: $(TARGET)
#make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- generic-v8-defconfig
#CROSS_COMPILE=aarch64-none-linux-gnu-

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map
