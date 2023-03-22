
CC = $(CROSS_COMPILE)gcc
FLAGS=-Wall -Werror

all: default

default: aesdsocket_o
	gcc $(FLAGS) aesdsocket.o -o aesdsocket -lpthread -lrt

aesdsocket_o:
	gcc $(FLAGS) -c aesdsocket.c -o aesdsocket.o -lpthread -lrt

CROSS_COMPILE: CROSS_COMPILE_o
	$(CC) $(FLAGS) aarch64-aesdsocket.o -o aarch64-aesdsocket

CROSS_COMPILE_o:
	$(CC) $(FLAGS) -c aarch64-aesdsocket.c -o aarch64-aesdsocket.o

clean:
	rm -f *.o aesdsocket aarch64-aesdsocket