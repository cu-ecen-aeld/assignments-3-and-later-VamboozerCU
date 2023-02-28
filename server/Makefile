TARGET ?= aesdsocket
OBJECTS ?= aesdsocket.o
INCLUDES ?= -I.
LDFLAGS ?=
CFLAGS ?= -g -Wall -Wextra #-Werror 
LIBS ?= -pthread -lrt
CC ?= $(CROSS_COMPILE)gcc

.PHONY: all
all: $(TARGET)
	@echo "RUNNING ALL"

$(TARGET): $(OBJECTS)
	@echo "BUILDING $(TARGET)..."
	$(CC) $(CFLAGS) $(LIBS) -o $(TARGET) $(OBJECTS)
	@echo "BUILDING $(TARGET) DONE"

aesdsocket.o: aesdsocket.c
	@echo "BUILDING aesdsocket..."
	$(CC) $(CFLAGS) $(INCLUDES) -c aesdsocket.c
	@echo "BUILDING aesdsocket DONE"

.PHONY: clean distclean mrproper
clean distclean mrproper:
	@echo "CLEANING..."
	-rm -f *.o $(TARGET) *.elf *.map
	@echo "CLEANING DONE"