CC = $(CROSS_COMPILE)gcc
FLAGS=-Wall -Werror

.PHONY : all
all: writer

writer: aarch64-writer_o
	$(CC) $(FLAGS)  aarch64-writer.o -o aarch64-writer

aarch64-writer_o:
	$(CC) $(FLAGS) -c writer.c -o aarch64-writer.o

.PHONY: clean
clean:
	rm -f *.o aarch64-writer