# CROSS_COMPILE= 
# CC=$(CROSS_COMPILE)gcc
# CFLAGS=-Wall -Werror
# SOURCE=writer.c

# all:
# 	$(CC) $(CFLAGS) $(SOURCE) -o aarch64-writer

# clean:
# 	rm -f *.o *.elf aarch64-writer

 # Use CROSS_COMPILE=aarch64-none-linux-gnu- to use ARM toolchain (default is gcc)
CC ?= $(CROSS_COMPILE)gcc
FLAGS=-Wall -Werror

# writer recipe
.PHONY : all
all: writer

writer: writer_o
	$(CC) $(FLAGS)  writer.o -o writer

writer_o:
	$(CC) $(FLAGS) -c writer.c -o writer.o

.PHONY: clean
clean:
	rm -f *.o writer