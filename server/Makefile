#GNU_PATH = /home/marshall/Programs/install-lnx/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/bin

TARGET = aesdsocket
SRC := aesdsocket.c
OBJS := $(SRC:.c=.o) # Generate .o files from .c files
CFLAGS += -Wall -Wextra #-Werror

# ==== Use this to have Buildroot compile for us ====
.PHONY : all
#all : $(TARGET)
#	@echo "RUNNING ALL"

#$(TARGET) : $(OBJS)
#	@echo "BUILDING $(TARGET)..."
#	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS)
#	@echo "BUILDING $(TARGET) DONE"

# ==== Use this to compile natively for native architecture ====
#.PHONY : native
#native:
all:
	@echo "RUNNING NATIVE $(TARGET) BUILD..."
#	gcc $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS)
	gcc $(CFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)
	@echo "BUILDING NATIVE $(TARGET) DONE"

# ==== Use this to compile natively for foreign ARM64 architecture ====
.PHONY : arm64
arm64:
	@echo "RUNNING ARM64 $(TARGET) BUILD..."
#	aarch64-none-linux-gnu-gcc $(SRC) -o $(TARGET)
	aarch64-none-linux-gnu-gcc $(CFLAGS) $(INCLUDES) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "BUILDING ARM64 $(TARGET) DONE"

# ==== Use default ====
.PHONY : default
default:
	@echo "WAITING FOR INSTRUCTIONS FOR default TARGET"

# ==== Applies to all build types ====
.PHONY: clean # redundant, but makes builds more effecient
clean:
	@echo "CLEANING..."
	-rm -f *.o $(TARGET) *.elf *.map
	@echo "CLEANING DONE"