
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt
TARGET ?= aesdsocket
SOURCES = aesdsocket.c

all: default

default: CROSS_COMPILE

CROSS_COMPILE: CROSS_COMPILE_o
	$(CC) $(CFLAGS) $(TARGET).o -o $(TARGET) $(LDFLAGS)

CROSS_COMPILE_o:
	$(CC) $(CFLAGS) -c $(SOURCES) -o $(TARGET).o $(LDFLAGS)

clean:
	rm -f *.o aesdsocket