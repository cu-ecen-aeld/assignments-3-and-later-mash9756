
CC = $(CROSS_COMPILE)gcc
FLAGS=-Wall -Werror
LIBS=-pthread

all: default

default: aesdsocket_o
	gcc $(LIBS) $(FLAGS) aesdsocket.o -o aesdsocket

aesdsocket_o:
	gcc $(LIBS) $(FLAGS) -c aesdsocket.c -o aesdsocket.o

CROSS_COMPILE: CROSS_COMPILE_o
	$(CC) $(FLAGS) aarch64-aesdsocket.o -o aarch64-aesdsocket

CROSS_COMPILE_o:
	$(CC) $(FLAGS) -c aesdsocket.c -o aarch64-aesdsocket.o

clean:
	rm -f *.o aesdsocket aarch64-aesdsocket