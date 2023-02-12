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