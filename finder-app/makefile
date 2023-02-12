# CROSS_COMPILE= 
# CC=$(CROSS_COMPILE)gcc
# CFLAGS=-Wall -Werror
# SOURCE=writer.c

# all:
# 	$(CC) $(CFLAGS) $(SOURCE) -o aarch64-writer

# clean:
# 	rm -f *.o *.elf aarch64-writer



CC=gcc
CFLAGS=-Wall -Werror

all: writer.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) writer.c -o writer

clean:
	rm -f *.o writer