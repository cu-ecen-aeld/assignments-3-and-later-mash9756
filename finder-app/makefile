default: 
	gcc -o writer writer.c

CROSS_COMPILE:
	aarch64-none-linux-gnu-gcc -o aarch64-writer writer.c
	file aarch64-writer > /home/admin/Documents/assignment2/assignment-2-mash9756/fileresult.txt

clean:
	rm -f *.o writer aarch64-writer