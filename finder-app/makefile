default: 
	gcc -g -Wall -c -o writer.o writer.c
	gcc -g -Wall -I/ writer.o -o writer

CROSS_COMPILE:
	aarch64-none-linux-gnu-gcc -Wall -c -o aarch64-writer.o writer.c
	aarch64-none-linux-gnu-gcc -Wall -I/ aarch64-writer.o -o aarch64-writer

all:
	aarch64-none-linux-gnu-gcc -Wall -c -o aarch64-writer.o writer.c
	aarch64-none-linux-gnu-gcc -Wall -I/ aarch64-writer.o -o aarch64-writer

clean:
	rm -f *.o *.elf *.map writer aarch64-writer