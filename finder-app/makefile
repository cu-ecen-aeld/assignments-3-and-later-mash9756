CC:=$(CROSS_COMPILE)gcc

default: 
	gcc -g -Wall -c -o writer.o writer.c
	gcc -g -Wall -I/ writer.o -o writer

CROSS_COMPILE:
	$(CC) -Wall -c -o aarch64-writer.o writer.c
	$(CC) -Wall -I/ aarch64-writer.o -o aarch64-writer

all:
	$(CC) -Wall -c -o aarch64-writer.o writer.c
	$(CC) -Wall -I/ aarch64-writer.o -o aarch64-writer

clean:
	rm -f *.o *.elf *.map writer aarch64-writer